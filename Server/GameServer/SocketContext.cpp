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

	// RST 없이 즉시 연결 종료 후 소켓 재사용을 위해 SO_LINGER 설정
	LINGER stLin;
	stLin.l_onoff  = 1;
	stLin.l_linger = 0;

	if (SOCKET_ERROR == setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (char*)&stLin, sizeof(LINGER)))
	{
		return FALSE;
	}

	contextState = AcceptReady;

	return TRUE;
}

void SocketContext::PacketProcess(LPWSAOVERLAPPED pOV, DWORD dwBytes)
{
	if (pOV == &m_recvOverlapped)
	{
		printf("[%d] Recv %d bytes\n", m_wID, dwBytes);
		RecvProcess(dwBytes);
		RecvPost();
	}
	else if (pOV == &m_sendOverlapped)
	{
		printf("[%d] Send %d bytes\n", m_wID, dwBytes);
	}
	else
	{
		printf("[%d] Critical: unknown overlapped ptr recv=%p send=%p got=%p\n",
			m_wID, &m_recvOverlapped, &m_sendOverlapped, pOV);
	}
}

BOOL SocketContext::AcceptPost(SOCKET listenSocket)
{
	printf("[%d] AcceptPost\n", m_wID);

	DWORD dwBytes = 0;
	if (FALSE == AcceptEx(listenSocket, m_socket, m_recvOverlapped.buf,
		0,
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		&dwBytes, this))
	{
		DWORD dwError = GetLastError();
		if (dwError != WSA_IO_PENDING)
		{
			printf("[%d] AcceptEx failed: %d\n", m_wID, dwError);
			return FALSE;
		}
	}

	contextState = AcceptReady;
	return TRUE;
}

BOOL SocketContext::AcceptProcess()
{
	sockaddr_in* localAddr  = NULL;
	sockaddr_in* remoteAddr = NULL;
	int nLocalAddrLen  = 0;
	int nRemoteAddrLen = 0;

	GetAcceptExSockaddrs(m_recvOverlapped.buf,
		0,
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		(sockaddr**)&localAddr,  &nLocalAddrLen,
		(sockaddr**)&remoteAddr, &nRemoteAddrLen);

	CGameServer::GetInstance()->UserConnect(m_wID);

	RecvPost();
	contextState = Connect;

	return TRUE;
}

BOOL SocketContext::DisconnectProcess()
{
	ZeroMemory(reinterpret_cast<PVOID>(this), sizeof(WSAOVERLAPPED));
	ZeroMemory(&m_sockAddrIn, sizeof(m_sockAddrIn));
	ZeroMemory(m_recvOverlapped.buf,  sizeof(m_recvOverlapped.buf));
	ZeroMemory(m_sendOverlapped.buf, sizeof(m_sendOverlapped.buf));

	printf("[%d] DisconnectProcess\n", m_wID);

	// TF_DISCONNECT | TF_REUSE_SOCKET: graceful disconnect 후 소켓 재사용 준비
	// 완료되면 IOCP에서 TransmitFilePending 상태로 감지 → AcceptPost 재호출
	if (FALSE == TransmitFile(m_socket, NULL, 0, 0, this, NULL, TF_DISCONNECT | TF_REUSE_SOCKET))
	{
		DWORD dwError = GetLastError();
		if (dwError != WSA_IO_PENDING)
		{
			printf("[%d] TransmitFile failed: %d\n", m_wID, dwError);
		}
	}

	contextState = TransmitFilePending;

	CGameServer::GetInstance()->UserDisconnect(m_wID);

	return TRUE;
}

BOOL SocketContext::RecvPost()
{
	DWORD dwRecvBytes = 0;
	DWORD dwFlag      = 0;

	int nResult = WSARecv(m_socket, &m_recvOverlapped.wsaBuf, 1,
		&dwRecvBytes, &dwFlag,
		(LPWSAOVERLAPPED)&m_recvOverlapped, NULL);

	if (nResult == SOCKET_ERROR)
	{
		DWORD dwError = GetLastError();
		if (dwError != WSA_IO_PENDING)
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL SocketContext::RecvProcess(DWORD dwRecvByte)
{
	if (TCP_SOCKET_BUFFER_SIZE < dwRecvByte)
	{
		printf("[%d] RecvProcess: oversized packet (%d)\n", m_wID, dwRecvByte);
		return FALSE;
	}

	WORD wReadSize = 0;
	while (0 < dwRecvByte - wReadSize)
	{
		if (TCP_SOCKET_BUFFER_SIZE <= wReadSize)
		{
			printf("[%d] RecvProcess: buffer overrun at offset %d\n", m_wID, wReadSize);
			return FALSE;
		}

		// 패킷 첫 2바이트 = wPacketSize (헤더 포함 전체 크기)
		WORD wDataSize = 0;
		CopyMemory(&wDataSize, &m_recvOverlapped.buf[wReadSize], sizeof(wDataSize));

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
	DWORD dwFlag      = 0;

	int nResult = WSASend(m_socket, &m_sendOverlapped.wsaBuf, 1,
		&dwSendBytes, dwFlag,
		(LPWSAOVERLAPPED)&m_sendOverlapped, NULL);

	if (nResult == SOCKET_ERROR)
	{
		DWORD dwError = GetLastError();
		if (dwError != WSA_IO_PENDING)
		{
			printf("[%d] WSASend failed: %d\n", m_wID, dwError);
		}
	}

	return TRUE;
}

BOOL SocketContext::SendProcess(WORD wDataSize, PVOID pData)
{
	m_sendOverlapped.wsaBuf.len = wDataSize;
	CopyMemory(m_sendOverlapped.buf, pData, wDataSize);
	return SendPost();
}