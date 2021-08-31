#include "stdafx.h"
#include "CBuffer.h"
#include "CIOCP.h"
#include "CUser.h"
#include "CBuffer.h"
#include "CDB.h"
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
	: m_pIocp(NULL), m_pUser(NULL), m_pPacketBuffer(NULL), m_pDbSendBuffer(NULL), m_pDbResult(NULL)
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
		Stop();
		return FALSE;
	}
	for (WORD i = 0; i < g_dwMaxUserCount; i++)
	{
		m_pUser[i].SetID(i);
	}

	m_pPacketBuffer = new CBuffer;
	if (m_pPacketBuffer == NULL)
	{
		Stop();
		return FALSE;
	}

	m_pDbSendBuffer = new CBuffer;
	if (m_pDbSendBuffer == NULL)
	{
		Stop();
		return FALSE;
	}

	m_pDbResult = new CBuffer;
	if (m_pDbResult == NULL)
	{
		Stop();
		return FALSE;
	}

	m_db = new CDB;
	if (m_db == NULL)
	{
		Stop();
		return FALSE;
	}

	m_db->Start();

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

	if (m_pPacketBuffer != NULL)
	{
		delete m_pPacketBuffer;
		m_pPacketBuffer = NULL;
	}

	if (m_pDbSendBuffer != NULL)
	{
		delete m_pDbSendBuffer;
		m_pDbSendBuffer = NULL;
	}

	if (m_pDbResult != NULL)
	{
		delete m_pDbResult;
		m_pDbResult = NULL;
	}

	if (m_db != NULL)
	{
		delete m_db;
		m_db = NULL;
	}

	if (m_pGameServer != NULL)
	{
		delete m_pGameServer;
		m_pGameServer = NULL;
	}

	return TRUE;
}

BOOL CGameServer::WriteRecvData(StreamData streamData)
{
	return m_pPacketBuffer->WriteData(streamData);
}

void CGameServer::WorkerThread(LPVOID lpParam)
{
	reinterpret_cast<CGameServer*>(lpParam)->Worker();
}

BOOL CGameServer::Worker()
{
	// HACK : WaitForSingleObject ´ë±â ¾Æ´Ò ¶§
	HANDLE recvEventHandle = m_pPacketBuffer->GetQueueEventHandle();

	while (TRUE)
	{
		DWORD dwResult = WaitForSingleObject(recvEventHandle, INFINITE);

		// packet process
		StreamData data;
		while (m_pPacketBuffer->ReadData(data))
		{
			this->PacketProcess(data);
			// CopyMemory(&nData, message.pData, sizeof(int));

			// printf("[%d] %d, %d, %d\n", message.wUserID, message.packetHeader.packetID, message.packetHeader.wPacketSize, nData);
		}

		// db Result
		while (m_pDbResult->ReadData(data))
		{

		}
	}

	return TRUE;
}

BOOL CGameServer::UserConnect(WORD wID)
{
	if (g_dwMaxUserCount <= wID)
	{
		return FALSE;
	}

	m_pUser[wID].OnConnect();
	return TRUE;
}

BOOL CGameServer::UserDisconnect(WORD wID)
{
	if (g_dwMaxUserCount <= wID)
	{
		return FALSE;
	}

	m_pUser[wID].OnDisconnect();
	return TRUE;
}

BOOL CGameServer::CheckInvalidUser(WORD wID)
{
	if (g_dwMaxUserCount <= wID)
	{
		return FALSE;
	}

	if (m_pUser[wID].GetState() < USER_STATE_LOGIN_COMPLETE)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CGameServer::PacketProcess(StreamData& streamData)
{
	//streamData.wUserID;		// m_pUser
	//streamData.pData;		// packetHeader{ unsigned short wPacketSize; PACKET_ID packetID; } + 
	if (FALSE == CheckInvalidUser(streamData.wUserID))
	{
		return FALSE;
	}

	PACKET_HEADER packetHeader;
	CopyMemory(&packetHeader, streamData.pData, sizeof(PACKET_HEADER));
	
	switch (packetHeader.packetID)
	{
	case PACKET_LOGIN: { PacketProcessLogin(streamData.pData); } break;
	case PACKET_LOGOUT: {} break;
		//
	case PACKET_TEST: { PacketProcessTest(streamData.pData); } break;
	default:
		{
			printf("[%d] Invalid Packet packetID:%d, dataSize:%d\n", streamData.wUserID, packetHeader.packetID, packetHeader.wPacketSize);
			return FALSE;
		}
		break;
	}


	return TRUE;
}

BOOL CGameServer::PacketProcessLogin(char* pData)
{
	CS_LOGIN* pPacket = (CS_LOGIN*)pData;

	return 0;
}

BOOL CGameServer::PacketProcessTest(char* pData)
{
	CS_TEST* pPacket = (CS_TEST*)pData;

	return 0;
}

///////////////////////////////////////////////////////////////
void CGameServer::SendToDB(StreamData& streamData)
{
	m_pDbSendBuffer->WriteData(streamData);
}

BOOL CGameServer::DBResultProcess(StreamData& streamData)
{
	return TRUE;
}
