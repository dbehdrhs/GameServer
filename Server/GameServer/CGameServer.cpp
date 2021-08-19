#include "stdafx.h"
#include "CBuffer.h"
#include "CIOCP.h"
#include "CUser.h"
#include "CBuffer.h"
#include "CGameServer.h"

CGameServer* CGameServer::m_pGameServer = NULL;

CGameServer* CGameServer::GetInstance()
{
	if (NULL == m_pGameServer)
	{
		m_pGameServer = new CGameServer;
	}

	return m_pGameServer;
}

CGameServer::CGameServer()
	: m_pIocp(NULL), m_pUser(NULL), m_pRecvBuffer(NULL)
{
}

CGameServer::~CGameServer()
{
	
}

BOOL CGameServer::Start(CIOCP* pIocp)
{
	m_pIocp = pIocp;

	m_pUser = new CUser[g_dwMaxUserCount];
	if (m_pUser == NULL)
	{
		return FALSE;
	}
	for (WORD i = 0; i < g_dwMaxUserCount; i++)
	{
		m_pUser[i].SetID(i);
	}

	m_pRecvBuffer = new CBuffer;
	if (m_pRecvBuffer == NULL)
	{
		return FALSE;
	}

	HANDLE hThread;
	DWORD dwThreadID;
	if (hThread = CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(WorkerThread), reinterpret_cast<LPVOID>(this), 0, &dwThreadID))
	{
		printf("[%u] GameServer Thread Start\n", dwThreadID);
		CloseHandle(hThread);
	}
	else
	{
		Stop();

		return FALSE;
	}

	return TRUE;
}

BOOL CGameServer::Stop()
{
	if (m_pUser != NULL)
	{
		delete[] m_pUser;
		m_pUser = NULL;
	}

	if (m_pRecvBuffer != NULL)
	{
		delete m_pRecvBuffer;
		m_pRecvBuffer = NULL;
	}

	if (m_pGameServer != NULL)
	{
		delete m_pGameServer;
		m_pGameServer = NULL;
	}

	return TRUE;
}

BOOL CGameServer::PacketProcess(WORD wID, char* pPacket)
{
	return FALSE;
}

BOOL CGameServer::WriteRecvData(StreamData streamData)
{
	m_pRecvBuffer->WriteData(streamData);

	char szTitle[126] = { 0, };
	sprintf_s(szTitle, 126, "[%zd] GameServer QueueSize", m_pRecvBuffer->GetStreamQueueCount());
	SetConsoleTitleA(szTitle);

	return TRUE;
}

BOOL CGameServer::PacketProcess(StreamData& streamData)
{
	streamData.wUserID;		// m_pUser
	// streamData.dwRecvSize;	// iocp Recv Size
	streamData.pData;		// packetHeader{ unsigned short wPacketSize; PACKET_ID packetID; } + 

	PACKET_HEADER packetHeader;
	CopyMemory(&packetHeader, streamData.pData, sizeof(PACKET_HEADER));

	switch (packetHeader.packetID)
	{
	case PACKET_LOGIN:
		{
			CS_LOGIN* pPacket = (CS_LOGIN*)streamData.pData;
			m_pUser[streamData.wUserID].PacketProcessLogin(pPacket);
		}
		break;
		//
	case PACKET_TEST:
		{
			CS_TEST* pPacket = (CS_TEST*)streamData.pData;
			m_pUser[streamData.wUserID].PacketProcessTest(pPacket);
		}
		break;
	default:
		{
			printf("[%d] Invalid Packet packetID:%d, dataSize:%d\n", streamData.wUserID, packetHeader.packetID, packetHeader.wPacketSize);
			return FALSE;
		}
		break;
	}
	
	return TRUE;
}

void CGameServer::WorkerThread(LPVOID lpParam)
{
	reinterpret_cast<CGameServer*>(lpParam)->Worker();
}

BOOL CGameServer::Worker()
{
	HANDLE recvEventHandle = m_pRecvBuffer->GetQueueEventHandle();

	while (TRUE)
	{
		DWORD dwResult = WaitForSingleObject(recvEventHandle, INFINITE);

		StreamData data;
		while (m_pRecvBuffer->ReadData(data))
		{
			this->PacketProcess(data);
			// CopyMemory(&nData, message.pData, sizeof(int));

			// printf("[%d] %d, %d, %d\n", message.wUserID, message.packetHeader.packetID, message.packetHeader.wPacketSize, nData);
		}

		// m_pIocp.
		// iocp -> game
		// iocp <- game
	}

	return TRUE;
}