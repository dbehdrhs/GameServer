#include "stdafx.h"

#include "mysql.h"
#pragma comment(lib, "libmysql.lib")

#include "CDB.h"
#include "CBuffer.h"
#include "CGameServer.h"
#include "Packet.h"

#define DB_INFO_PATH ".\\Info.ini"

CDB::CDB()
	: byThreadCount(1), m_nPort(3306)
{
	ZeroMemory(m_szHost,     sizeof(m_szHost));
	ZeroMemory(m_szUser,     sizeof(m_szUser));
	ZeroMemory(m_szPassword, sizeof(m_szPassword));
	ZeroMemory(m_szDB,       sizeof(m_szDB));
}

CDB::~CDB()
{
	mysql_close(&m_mysql);
}

BOOL CDB::Start()
{
	// Info.ini에서 DB 접속 정보 로드
	GetPrivateProfileStringA("DB", "Host",     "127.0.0.1", m_szHost,     sizeof(m_szHost),     DB_INFO_PATH);
	GetPrivateProfileStringA("DB", "User",     "root",      m_szUser,     sizeof(m_szUser),     DB_INFO_PATH);
	GetPrivateProfileStringA("DB", "Password", "",          m_szPassword, sizeof(m_szPassword), DB_INFO_PATH);
	GetPrivateProfileStringA("DB", "Schema",   "accountdb", m_szDB,       sizeof(m_szDB),       DB_INFO_PATH);
	m_nPort = GetPrivateProfileIntA("DB", "Port", 3306, DB_INFO_PATH);

	if (FALSE == ConnectDB())
	{
		return FALSE;
	}

	HANDLE hThread = INVALID_HANDLE_VALUE;
	DWORD dwThreadID = 0;

	for (int i = 0; i < byThreadCount; i++)
	{
		hThread = CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(WorkerThread), reinterpret_cast<LPVOID>(this), 0, &dwThreadID);
		if (hThread)
		{
			printf("[%u] DB Thread Start\n", dwThreadID);
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

BOOL CDB::Stop()
{
	mysql_close(&m_mysql);
	return TRUE;
}

BOOL CDB::ConnectDB()
{
	printf("MySQL Client Version: %s\n", mysql_get_client_info());

	mysql_init(&m_mysql);
	if (NULL == mysql_real_connect(&m_mysql, m_szHost, m_szUser, m_szPassword, m_szDB, m_nPort, NULL, 0))
	{
		printf("[DB] Connect Failed: %s\n", mysql_error(&m_mysql));
		return FALSE;
	}

	printf("[DB] Connected to %s:%u/%s\n", m_szHost, m_nPort, m_szDB);
	return TRUE;
}

void CDB::SendDbResult(WORD wID)
{
}

void CDB::WorkerThread(LPVOID lpParam)
{
	reinterpret_cast<CDB*>(lpParam)->Worker();
}

BOOL CDB::Worker()
{
	CBuffer* pDbQueue = CGameServer::GetInstance()->GetDbBuffer();
	HANDLE hEvent = pDbQueue->GetQueueEventHandle();

	while (TRUE)
	{
		WaitForSingleObject(hEvent, INFINITE);

		StreamData data;
		while (pDbQueue->ReadData(data))
		{
			PACKET_HEADER header;
			CopyMemory(&header, data.pData, sizeof(PACKET_HEADER));

			switch (header.packetID)
			{
			case PACKET_LOGIN:
			{
				CS_LOGIN* pLogin = (CS_LOGIN*)data.pData;
				QueryLogin(data.wUserID, pLogin->szID, pLogin->szPassword);
			}
			break;
			default:
				printf("[DB] Unknown packetID: %d\n", header.packetID);
				break;
			}
		}
	}

	return TRUE;
}

BOOL CDB::QueryLogin(WORD wID, const char* szID, const char* szPassword)
{
	// Prepared statement으로 SQL injection 방지
	MYSQL_STMT* pStmt = mysql_stmt_init(&m_mysql);
	if (pStmt == NULL)
	{
		printf("[DB] mysql_stmt_init failed\n");
		return FALSE;
	}

	const char* query = "SELECT id FROM UserAccount WHERE name = ? AND password = ?";
	if (mysql_stmt_prepare(pStmt, query, (unsigned long)strlen(query)))
	{
		printf("[DB] mysql_stmt_prepare failed: %s\n", mysql_stmt_error(pStmt));
		mysql_stmt_close(pStmt);
		return FALSE;
	}

	MYSQL_BIND bind[2];
	ZeroMemory(bind, sizeof(bind));

	unsigned long idLen       = (unsigned long)strlen(szID);
	unsigned long passwordLen = (unsigned long)strlen(szPassword);

	bind[0].buffer_type   = MYSQL_TYPE_STRING;
	bind[0].buffer        = (void*)szID;
	bind[0].buffer_length = 32;
	bind[0].length        = &idLen;

	bind[1].buffer_type   = MYSQL_TYPE_STRING;
	bind[1].buffer        = (void*)szPassword;
	bind[1].buffer_length = 32;
	bind[1].length        = &passwordLen;

	if (mysql_stmt_bind_param(pStmt, bind))
	{
		printf("[DB] mysql_stmt_bind_param failed: %s\n", mysql_stmt_error(pStmt));
		mysql_stmt_close(pStmt);
		return FALSE;
	}

	if (mysql_stmt_execute(pStmt))
	{
		printf("[DB] mysql_stmt_execute failed: %s\n", mysql_stmt_error(pStmt));
		mysql_stmt_close(pStmt);
		return FALSE;
	}

	mysql_stmt_store_result(pStmt);
	BOOL bSuccess = (mysql_stmt_num_rows(pStmt) > 0) ? TRUE : FALSE;
	mysql_stmt_close(pStmt);

	// 로그인 결과를 GameServer 큐에 전달
	SC_LOGIN scLogin;
	scLogin.bSuccess = bSuccess;

	StreamData result;
	result.wUserID = wID;
	CopyMemory(result.pData, &scLogin, sizeof(SC_LOGIN));

	CGameServer::GetInstance()->GetDBResultBuffer()->WriteData(result);

	printf("[DB] QueryLogin [%d] ID:%s Result:%s\n", wID, szID, bSuccess ? "SUCCESS" : "FAIL");
	return TRUE;
}
