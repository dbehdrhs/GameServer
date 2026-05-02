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
	CIOCP*   m_pIocp;
	CDB*     m_db;
	CUser*   m_pUser;

	CBuffer* m_pPacketBuffer; // IOCP  → Game
	CBuffer* m_pDbSendBuffer; // Game  → DB
	CBuffer* m_pDbResult;     // DB    → Game

	// 패킷 핸들러 테이블: 새 패킷은 RegisterHandlers()에만 추가
	typedef BOOL (CGameServer::*PacketHandler)(WORD wID, char* pData);
	PacketHandler m_handlers[PACKET_MAX];

	void RegisterHandlers();

public:
	static CGameServer* GetInstance();

	CGameServer();
	~CGameServer();

	BOOL Start(CIOCP* pIocp);
	BOOL Stop();

	BOOL WriteRecvData(StreamData streamData);
	CBuffer* GetDbBuffer()       { return m_pDbSendBuffer; }
	CBuffer* GetDBResultBuffer() { return m_pDbResult; }

	static void CALLBACK WorkerThread(LPVOID lpParam);
	BOOL Worker();

	BOOL UserConnect(WORD wID);
	BOOL UserDisconnect(WORD wID);

private:
	BOOL IsUserLoggedIn(WORD wID);
	BOOL PacketProcess(StreamData& streamData);
	BOOL PacketProcessLogin(WORD wID, char* pData);
	BOOL PacketProcessLogout(WORD wID, char* pData);
	BOOL PacketProcessTest(WORD wID, char* pData);

	void SendToDB(StreamData& streamData);
	BOOL DBResultProcess(StreamData& streamData);
};
