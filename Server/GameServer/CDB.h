#pragma once

class CDB
{
private:
	BYTE byThreadCount;
public:
	CDB();
	~CDB();

public:
	BOOL Start();
	BOOL Stop();

public:
	void SendDbResult(WORD wID);

public:
	static void CALLBACK WorkerThread(LPVOID lpParam);
	BOOL Worker();

private:
	MYSQL m_mysql;
	char m_szHost[64];
	char m_szUser[64];
	char m_szPassword[64];
	char m_szDB[64];
	unsigned int m_nPort;

	BOOL ConnectDB();

public:
	BOOL QueryLogin(WORD wID, const char* szID, const char* szPassword);
};

