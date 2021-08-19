#include "stdafx.h"

#include "SocketContext.h"
#include "CBuffer.h"
#include "CIOCP.h"

const ULONG ulIocpStopKey = ULONG_MAX;

CIOCP::CIOCP()
{
	m_state = stop;
	m_listenSocket = INVALID_SOCKET;
	m_dwThreadCount = 0;
	m_hIocp = INVALID_HANDLE_VALUE;
	
	m_pSocketContext = NULL;
}

CIOCP::~CIOCP()
{
	if (NULL != m_pSocketContext)
	{
		delete[] m_pSocketContext;
		m_pSocketContext = NULL;
	}
}

BOOL CIOCP::Start(WORD wPort, DWORD dwThreadCount)
{
	m_state = startReady;

	//
	if (dwThreadCount == 0)
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);

		dwThreadCount = si.dwNumberOfProcessors * 2;
	}
	m_dwThreadCount = dwThreadCount;

	//
	if (FALSE == StartWinNet())
	{
		return FALSE;
	}
	if (FALSE == CreateListenSocket(wPort))
	{
		return FALSE;
	}
	if (FALSE == CreateIOCP())
	{
		return FALSE;
	}

	if (FALSE == CreateSocketContext())
	{
		return FALSE;
	}

	if (FALSE == StartWorkerThread())
	{
		return FALSE;
	}
	// 

	m_state = startComplete;
	return TRUE;
}

void CIOCP::Stop()
{
	m_state = stop;

	PostQueuedCompletionStatus(m_hIocp, 0, ulIocpStopKey, NULL);

	WSACleanup();
}

BOOL CIOCP::StartWinNet()
{
	WSADATA wsd;

	if (WSAStartup(MAKEWORD(2, 2), &wsd))
	{
		DWORD dwError = GetLastError();
		// dwError
		return FALSE;
	}

	return TRUE;
}

BOOL CIOCP::CreateListenSocket(WORD wPort)
{
	m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_listenSocket == INVALID_SOCKET)
	{
		DWORD dwError = GetLastError();
		// dwError
		return FALSE;
	}

	SOCKADDR_IN sockAddr;
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(wPort);
	sockAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if (SOCKET_ERROR == bind(m_listenSocket, (SOCKADDR*)&sockAddr, sizeof(sockAddr)))
	{
		Stop();
		return FALSE;
	}

	if (SOCKET_ERROR == listen(m_listenSocket, SOMAXCONN))
	{
		Stop();
		return FALSE;
	}

	return TRUE;
}

BOOL CIOCP::CreateIOCP()
{
	m_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (m_hIocp == NULL)
	{
		DWORD dwError = GetLastError();
		// dwError
		return FALSE;
	}

	return TRUE;
}

BOOL CIOCP::CreateSocketContext()
{
	//////////////////////////////////////////////////
	// Listen Socket
	m_hIocp = CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_listenSocket), m_hIocp, g_dwMaxUserCount, 0);
	if (m_hIocp == NULL)
	{
		return FALSE;
	}

	//////////////////////////////////////////////////
	// Client Socket Pool
	m_pSocketContext = new SocketContext[g_dwMaxUserCount];

	if (m_pSocketContext == NULL)
	{
		return FALSE;
	}

	for (WORD i = 0; i < g_dwMaxUserCount; i++)
	{
		if (FALSE == m_pSocketContext[i].Init(i))
		{
			// Create Fail Client Socket 
			continue;
		}

		m_hIocp = CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_pSocketContext[i].GetSocket()), m_hIocp, i, 0);
		if (m_hIocp == NULL)
		{
			return FALSE;
		}

		DWORD dwBytes = 0;
		
		if (FALSE == m_pSocketContext[i].AcceptPost(m_listenSocket))
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CIOCP::StartWorkerThread()
{
	for (DWORD i = 0; i < m_dwThreadCount; i++)
	{
		HANDLE hThread;
		DWORD dwThreadID = 0;

		if (hThread = CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(WorkerThread), reinterpret_cast<LPVOID>(this), 0, &dwThreadID))
		{
			printf("[%u] IOCP Thread Start\n", dwThreadID);
			CloseHandle(hThread);
		}
		else
		{
			Stop();

			return FALSE;
		}
	}

	return TRUE;
}


void CALLBACK CIOCP::WorkerThread(LPVOID lpParam)
{
	reinterpret_cast<CIOCP*>(lpParam)->Worker();
}

void CIOCP::Worker()
{
	while (TRUE)
	{
		DWORD dwBytes;
		DWORD dwKey;
		LPWSAOVERLAPPED pOV;

		BOOL bResult = GetQueuedCompletionStatus(m_hIocp, &dwBytes, (PULONG_PTR)&dwKey, &pOV, WSA_INFINITE);
		DWORD dwError = GetLastError();

		// CIOCP::Stop { PostQueuedCompletionStatus(m_hIocp, 0, ulIocpStopKey, NULL); }
		if (dwBytes == 0 && dwKey == ulIocpStopKey)
		{
			Stop();
			break;
		}

		// listenSocket ( CreateIoCompletionPort key is g_dwMaxUserCount )
		if (dwKey == g_dwMaxUserCount) // connect
		{
			// AcceptEx overlapped == SocketContext(this)
			SocketContext* socketContext = reinterpret_cast<SocketContext*>(pOV);
			if (socketContext == NULL)
			{
				printf("[ERROR] Critical socketContext is NULL, dwError:%d\n", dwError);
				continue;
			}

			WORD wID = socketContext->GetID();
			printf("[%d]Connect dwKey : %d, byte : %d\n", wID, dwKey, dwBytes);

			m_pSocketContext[wID].AcceptProcess();
			continue;
		}

		// disconnect
		if (dwBytes == 0)
		{
			// TransmitFile을 호출 한 뒤 GetQueuedCompletionStatus에서 통지 받은 뒤에 AcceptEx 호출 해야 에러x
			if (Connect == m_pSocketContext[dwKey].GetContextState())
			{
				m_pSocketContext[dwKey].DisconnectProcess(); // ERROR : 10057(WSAENOTCONN)
				continue;
			}
			else if (TransmitFilePending == m_pSocketContext[dwKey].GetContextState())
			{
				m_pSocketContext[dwKey].AcceptPost(m_listenSocket); // ERROR : 10022(WSAEINVAL)
				continue;
			}
		}

		// send, recv
		if (bResult == TRUE)
		{
			////////////////////////////////////////////////////////////////////////
			m_pSocketContext[dwKey].PacketProcess(pOV, dwBytes);
		}
		else
		{
			printf("[%d] Error : %d, bytes : %d\n", dwKey, dwError, dwBytes);
		}
	}
}

BOOL CIOCP::SendData(WORD wID, WORD wDataSize, PVOID pData)
{
	if (g_dwMaxUserCount <= wID)
	{
		return FALSE;
	}

	if (TCP_SOCKET_BUFFER_SIZE < wDataSize)
	{
		return FALSE;
	}

	m_pSocketContext[wID].SendProcess(wDataSize, pData);

	return TRUE;
}
