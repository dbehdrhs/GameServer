#include "stdafx.h"
#include "CBuffer.h"
#include "CIOCP.h"
#include "CUser.h"
#include "CDB.h"
#include "CGameServer.h"

CGameServer* CGameServer::m_pGameServer = NULL;

CGameServer* CGameServer::GetInstance()
{
	if (m_pGameServer == NULL)
	{
		m_pGameServer = new CGameServer;
	}
	return m_pGameServer;
}

CGameServer::CGameServer()
	: m_pIocp(NULL), m_db(NULL), m_pUser(NULL),
	  m_pPacketBuffer(NULL), m_pDbSendBuffer(NULL), m_pDbResult(NULL)
{
	ZeroMemory(m_handlers, sizeof(m_handlers));
}

CGameServer::~CGameServer()
{
	Stop();
}

BOOL CGameServer::Start(CIOCP* pIocp)
{
	m_pIocp = pIocp;

	RegisterHandlers();

	m_pUser = new CUser[g_dwMaxUserCount];
	for (WORD i = 0; i < g_dwMaxUserCount; i++)
	{
		m_pUser[i].SetID(i);
	}

	m_pPacketBuffer = new CBuffer;
	m_pDbSendBuffer = new CBuffer;
	m_pDbResult     = new CBuffer;

	m_db = new CDB;
	if (FALSE == m_db->Start())
	{
		Stop();
		return FALSE;
	}

	DWORD  dwThreadID = 0;
	HANDLE hThread = CreateThread(NULL, 0,
		reinterpret_cast<LPTHREAD_START_ROUTINE>(WorkerThread),
		reinterpret_cast<LPVOID>(this), 0, &dwThreadID);

	if (hThread)
	{
		printf("[GameServer] Worker thread started: %u\n", dwThreadID);
		CloseHandle(hThread);
	}
	else
	{
		printf("[GameServer] CreateThread failed: %d\n", GetLastError());
		Stop();
		return FALSE;
	}

	return TRUE;
}

BOOL CGameServer::Stop()
{
	if (m_pUser != NULL)         { delete[] m_pUser;        m_pUser         = NULL; }
	if (m_pPacketBuffer != NULL) { delete m_pPacketBuffer;  m_pPacketBuffer = NULL; }
	if (m_pDbSendBuffer != NULL) { delete m_pDbSendBuffer;  m_pDbSendBuffer = NULL; }
	if (m_pDbResult != NULL)     { delete m_pDbResult;      m_pDbResult     = NULL; }
	if (m_db != NULL)            { delete m_db;             m_db            = NULL; }
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
	HANDLE handles[2] = {
		m_pPacketBuffer->GetQueueEventHandle(),
		m_pDbResult->GetQueueEventHandle(),
	};

	while (TRUE)
	{
		WaitForMultipleObjects(2, handles, FALSE, INFINITE);

		StreamData data;

		while (m_pPacketBuffer->ReadData(data))
		{
			PacketProcess(data);
		}

		while (m_pDbResult->ReadData(data))
		{
			DBResultProcess(data);
		}
	}

	return TRUE;
}

BOOL CGameServer::UserConnect(WORD wID)
{
	if (g_dwMaxUserCount <= wID) return FALSE;
	m_pUser[wID].OnConnect();
	return TRUE;
}

BOOL CGameServer::UserDisconnect(WORD wID)
{
	if (g_dwMaxUserCount <= wID) return FALSE;
	m_pUser[wID].OnDisconnect();
	return TRUE;
}

BOOL CGameServer::IsUserLoggedIn(WORD wID)
{
	if (g_dwMaxUserCount <= wID) return FALSE;
	return m_pUser[wID].GetState() >= USER_STATE_LOGIN_COMPLETE;
}