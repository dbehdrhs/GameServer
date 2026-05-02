#include "stdafx.h"

#include "SocketContext.h"
#include "CBuffer.h"
#include "CIOCP.h"

const ULONG ulIocpStopKey = ULONG_MAX;

CIOCP::CIOCP()
{
	m_state          = stop;
	m_listenSocket   = INVALID_SOCKET;
	m_dwThreadCount  = 0;
	m_hIocp          = INVALID_HANDLE_VALUE;
	m_pSocketContext = NULL;
}

CIOCP::~CIOCP()
{
	if (m_pSocketContext != NULL)
	{
		delete[] m_pSocketContext;
		m_pSocketContext = NULL;
	}
}

BOOL CIOCP::Start(WORD wPort, DWORD dwThreadCount)
{
	m_state = startReady;

	if (dwThreadCount == 0)
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		dwThreadCount = si.dwNumberOfProcessors * 2;
	}
	m_dwThreadCount = dwThreadCount;

	if (FALSE == StartWinNet())             return FALSE;
	if (FALSE == CreateListenSocket(wPort)) return FALSE;
	if (FALSE == CreateIOCP())              return FALSE;
	if (FALSE == CreateSocketContext())     return FALSE;
	if (FALSE == StartWorkerThread())       return FALSE;

	m_state = startComplete;
	return TRUE;
}

void CIOCP::Stop()
{
	m_state = stop;
	closesocket(m_listenSocket);
	PostQueuedCompletionStatus(m_hIocp, 0, ulIocpStopKey, NULL);
	WSACleanup();
}

BOOL CIOCP::StartWinNet()
{
	WSADATA wsd;
	if (WSAStartup(MAKEWORD(2, 2), &wsd))
	{
		printf("[IOCP] WSAStartup failed: %d\n", GetLastError());
		return FALSE;
	}
	return TRUE;
}

BOOL CIOCP::CreateListenSocket(WORD wPort)
{
	m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_listenSocket == INVALID_SOCKET)
	{
		printf("[IOCP] WSASocket failed: %d\n", GetLastError());
		return FALSE;
	}

	SOCKADDR_IN sockAddr;
	sockAddr.sin_family           = AF_INET;
	sockAddr.sin_port             = htons(wPort);
	sockAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if (SOCKET_ERROR == bind(m_listenSocket, (SOCKADDR*)&sockAddr, sizeof(sockAddr)))
	{
		printf("[IOCP] bind failed: %d\n", GetLastError());
		Stop();
		return FALSE;
	}

	if (SOCKET_ERROR == listen(m_listenSocket, SOMAXCONN))
	{
		printf("[IOCP] listen failed: %d\n", GetLastError());
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
		printf("[IOCP] CreateIoCompletionPort failed: %d\n", GetLastError());
		return FALSE;
	}
	return TRUE;
}

BOOL CIOCP::CreateSocketContext()
{
	// listenSocket의 completion key = g_dwMaxUserCount (클라이언트 key 범위 밖)
	m_hIocp = CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_listenSocket), m_hIocp, g_dwMaxUserCount, 0);
	if (m_hIocp == NULL)
	{
		return FALSE;
	}

	m_pSocketContext = new SocketContext[g_dwMaxUserCount];

	for (WORD i = 0; i < g_dwMaxUserCount; i++)
	{
		if (FALSE == m_pSocketContext[i].Init(i))
		{
			continue;
		}

		m_hIocp = CreateIoCompletionPort(
			reinterpret_cast<HANDLE>(m_pSocketContext[i].GetSocket()), m_hIocp, i, 0);
		if (m_hIocp == NULL)
		{
			return FALSE;
		}

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
		DWORD  dwThreadID = 0;
		HANDLE hThread = CreateThread(NULL, 0,
			reinterpret_cast<LPTHREAD_START_ROUTINE>(WorkerThread),
			reinterpret_cast<LPVOID>(this), 0, &dwThreadID);

		if (hThread)
		{
			printf("[IOCP] Worker thread started: %u\n", dwThreadID);
			CloseHandle(hThread);
		}
		else
		{
			printf("[IOCP] CreateThread failed: %d\n", GetLastError());
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
		DWORD           dwBytes;
		DWORD           dwKey;
		LPWSAOVERLAPPED pOV;

		BOOL  bResult = GetQueuedCompletionStatus(m_hIocp, &dwBytes, (PULONG_PTR)&dwKey, &pOV, WSA_INFINITE);
		DWORD dwError = GetLastError();

		// Stop() → PostQueuedCompletionStatus(ulIocpStopKey) 로 종료 신호 수신
		if (dwBytes == 0 && dwKey == ulIocpStopKey)
		{
			Stop();
			break;
		}

		// completion key == g_dwMaxUserCount: listenSocket AcceptEx 완료
		if (dwKey == g_dwMaxUserCount)
		{
			SocketContext* pSocketContext = reinterpret_cast<SocketContext*>(pOV);
			if (pSocketContext == NULL)
			{
				printf("[IOCP] Critical: null SocketContext on accept, error=%d\n", dwError);
				continue;
			}

			WORD wID = pSocketContext->GetID();
			printf("[%d] Connected (%d bytes)\n", wID, dwBytes);
			m_pSocketContext[wID].AcceptProcess();
			continue;
		}

		// dwBytes == 0: graceful close 또는 연결 끊김
		if (dwBytes == 0)
		{
			if (Connect == m_pSocketContext[dwKey].GetContextState())
			{
				m_pSocketContext[dwKey].DisconnectProcess();
			}
			else if (TransmitFilePending == m_pSocketContext[dwKey].GetContextState())
			{
				// TransmitFile 완료 → AcceptPost로 소켓 재사용 등록
				m_pSocketContext[dwKey].AcceptPost(m_listenSocket);
			}
			continue;
		}

		if (bResult == TRUE)
		{
			m_pSocketContext[dwKey].PacketProcess(pOV, dwBytes);
		}
		else
		{
			printf("[%d] GQCS error=%d bytes=%d\n", dwKey, dwError, dwBytes);
		}
	}
}

BOOL CIOCP::SendData(WORD wID, WORD wDataSize, PVOID pData)
{
	if (g_dwMaxUserCount <= wID)            return FALSE;
	if (TCP_SOCKET_BUFFER_SIZE < wDataSize) return FALSE;

	m_pSocketContext[wID].SendProcess(wDataSize, pData);
	return TRUE;
}