#pragma once

class CIOCP;
class CBuffer;
struct StreamData;
class CUser;
class CDB;

class CGameServer
{
private:
	static CGameServer* m_pGameServer;
	CIOCP* m_pIocp;
	CDB* m_db;

	CUser* m_pUser;

	CBuffer* m_pPacketBuffer; // iocp -> game
	CBuffer* m_pDbSendBuffer; // game -> db
	CBuffer* m_pDbResult; // db -> game

public:
	static CGameServer* GetInstance();

public:
	CGameServer();
	~CGameServer();

	BOOL Start(CIOCP* pIocp);
	BOOL Stop();

	CBuffer* GetRecvBuffer() { return m_pPacketBuffer; }
	BOOL WriteRecvData(StreamData streamData);
	CBuffer* GetDbBuffer() { return m_pDbSendBuffer; }
	CBuffer* GetDBResultBuffer() { return m_pDbResult; }

public:
	static void CALLBACK WorkerThread(LPVOID lpParam);
	BOOL Worker();

	//
	BOOL UserConnect(WORD wID);
	BOOL UserDisconnect(WORD wID);

	//
	BOOL CheckInvalidUser(WORD wID);
	BOOL PacketProcess(StreamData& streamData);
	BOOL PacketProcessLogin(char* pData);
	BOOL PacketProcessTest(char* pData);

	//
	void SendToDB(StreamData& streamData);
	BOOL DBResultProcess(StreamData& streamData);
};

