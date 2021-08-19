#include "stdafx.h"
#include "CBuffer.h"
#include "CGameServer.h"
#include "SocketContext.h"


SocketContext::SocketContext()
	: m_wID(0), m_socket(INVALID_SOCKET), contextState(AcceptReady)
{
	ZeroMemory(this, sizeof(WSAOVERLAPPED));
	ZeroMemory((void*)&m_sockAddrIn, sizeof(m_sockAddrIn));

	m_recvOverlapped.ioType = IO_RECV;
	m_sendOverlapped.ioType = IO_SEND;
}

SocketContext::~SocketContext()
{
}

BOOL SocketContext::Init(WORD wID)
{
	m_wID = wID;

	m_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_socket == INVALID_SOCKET)
	{
		return FALSE;
	}

	// send buffer에 남아 있더라도 데이터 삭제 후 종료 시킨다.
	LINGER stLin;
	stLin.l_onoff = 1;
	stLin.l_linger = 0;

	if (SOCKET_ERROR == setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (char *)&stLin, sizeof(LINGER)))
	{
		// fail setsockopt(SO_LINGER)
		return FALSE;
	}

	contextState = AcceptReady;
	
	return TRUE;
}

void SocketContext::PacketProcess(LPWSAOVERLAPPED pOV, DWORD dwBytes)
{
	if (pOV == &m_recvOverlapped)
	{
		printf("[%d] Recv Process : %dbytes\n", m_wID, dwBytes);

		RecvProcess(dwBytes);
		RecvPost();
	}
	else if (pOV == &m_sendOverlapped)
	{
		printf("[%d] Send Process : %dbytes\n", m_wID, dwBytes);
		// SendPost();
	}
	else
	{
		printf("[%d] Critial Error invalid overlapped, %p, %p, %p | %p\n", m_wID, this, &m_recvOverlapped, &m_sendOverlapped, pOV);
		//
	}
}

BOOL SocketContext::AcceptPost(SOCKET listenSocket)
{
	printf("[%d]AcceptPost\n", m_wID);

	DWORD dwBytes = 0;
	if (FALSE == AcceptEx(listenSocket, m_socket, m_recvOverlapped.buf, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &dwBytes, this))
	{
		DWORD dwError = GetLastError();
		if (dwError != WSA_IO_PENDING)
		{
			printf("Fail AcceptEx : %d\n", dwError);
			// AcceptEx Fail
			return FALSE;
		}
	}

	contextState = AcceptReady;

	return TRUE;
}

BOOL SocketContext::AcceptProcess()
{
	// 접속 정보 얻어오기
	sockaddr_in* localAddr = NULL;
	sockaddr_in* remoteAddr = NULL;
	int nLocalAddrLen = 0;
	int nRemoteAddrLen = 0;

	GetAcceptExSockaddrs(this->m_recvOverlapped.buf, 0
		, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16
		, (sockaddr**)&localAddr, &nLocalAddrLen
		, (sockaddr**)&remoteAddr, &nRemoteAddrLen);

	RecvPost();
	contextState = Connect;

	return TRUE;
}

BOOL SocketContext::DisconnectProcess()
{
	ZeroMemory(reinterpret_cast<PVOID>(this), sizeof(WSAOVERLAPPED));
	ZeroMemory(&m_sockAddrIn, sizeof(m_sockAddrIn));

	ZeroMemory(m_recvOverlapped.buf, sizeof(m_recvOverlapped.buf));
	ZeroMemory(m_sendOverlapped.buf, sizeof(m_sendOverlapped.buf));

	// ioType = IO_READY;

	printf("[%d]TransmitFile\n", m_wID);
	if (FALSE == TransmitFile(m_socket, NULL, 0, 0, this, NULL, TF_DISCONNECT | TF_REUSE_SOCKET))
	{
		DWORD dwError = GetLastError();
		
		if (dwError != WSA_IO_PENDING)
		{
			printf("TransmitFile Result : %d\n", dwError);
		}
	}

	contextState = TransmitFilePending;

	return TRUE;
}

BOOL SocketContext::RecvPost()
{
	DWORD dwRecvBytes = 0;
	DWORD dwFlag = 0;
	// this->ioType = IO_RECV;

	int nResult = WSARecv(m_socket, &this->m_recvOverlapped.wsaBuf, 1, &dwRecvBytes, &dwFlag, (LPWSAOVERLAPPED)&this->m_recvOverlapped, NULL);

	if (nResult == SOCKET_ERROR)
	{
		DWORD dwError = GetLastError();

		if (dwError != WSA_IO_PENDING)
		{
			// printf("wID : %d, WSARecv : %d\n", m_wID, dwError);
			return FALSE;
		}
	}

	return TRUE;
}

BOOL SocketContext::RecvProcess(DWORD dwRecvByte)
{
	if (TCP_SOCKET_BUFFER_SIZE < dwRecvByte)
	{
		// HACK : 흠....
		printf("[%d] critical error data size is big:%d\n", m_wID, dwRecvByte);
		return FALSE;
	}

	WORD wReadSize = 0;
	while (0 < dwRecvByte - wReadSize)
	{
		if (TCP_SOCKET_BUFFER_SIZE <= wReadSize)
		{
			printf("[%d] critical error data index:%d\n", m_wID, wReadSize);
			return FALSE;
		}

		WORD wDataSize = 0;
		CopyMemory((PVOID)&wDataSize, &m_recvOverlapped.buf[wReadSize], sizeof(wDataSize));
		
		//
		StreamData streamData;
		streamData.wUserID = m_wID;
		CopyMemory(streamData.pData, &m_recvOverlapped.buf[wReadSize], wDataSize);

		CGameServer::GetInstance()->WriteRecvData(streamData);

		wReadSize += wDataSize;
	}

	return TRUE;
}

BOOL SocketContext::SendPost()
{
	DWORD dwSendBytes = 0;
	DWORD dwFlag = 0;
	// this->ioType = IO_SEND;
	
	int nResult = WSASend(m_socket, &this->m_sendOverlapped.wsaBuf, 1, &dwSendBytes, dwFlag, (LPWSAOVERLAPPED)&this->m_sendOverlapped, NULL);

	if (nResult == SOCKET_ERROR)
	{
		DWORD dwError = GetLastError();

		if (dwError != WSA_IO_PENDING)
		{
			printf("wID : %d, WSARecv : %d\n", m_wID, dwError);
		}
	}
	
	return TRUE;
}

BOOL SocketContext::SendProcess(WORD wDataSize, PVOID pData)
{
	m_sendOverlapped.wsaBuf.len = wDataSize;
	CopyMemory((PVOID)m_sendOverlapped.buf, (PVOID)&wDataSize, sizeof(wDataSize));
	CopyMemory((PVOID)m_sendOverlapped.buf[2], pData, wDataSize);

	return SendPost();
}
