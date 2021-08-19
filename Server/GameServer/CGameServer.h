#pragma once

class CIOCP;
class CBuffer;
struct StreamData;
class CUser;

class CRoom;

// class CDB;

/******************************
// CGameServer
iocp와 db 사이에서 단일 스레드로 동작
--------     --------------     ------
| IOCP | <-> | GameServer | <-> | DB |
--------     --------------     ------
Channel, Room, User 관리
*******************************/

class CGameServer
{
private:
	static CGameServer* m_pGameServer;
	CIOCP* m_pIocp;
	// CDB* m_db;

	CRoom* m_pRoom;
	CUser* m_pUser;

	CBuffer* m_pRecvBuffer;

public:
	static CGameServer* GetInstance();

public:
	CGameServer();
	~CGameServer();

	BOOL Start(CIOCP* pIocp);
	BOOL Stop();

	CBuffer* GetRecvBuffer() { return m_pRecvBuffer; }
	BOOL WriteRecvData(StreamData streamData);

	//
	BOOL PacketProcess(WORD wID, char* pPacket);
	BOOL PacketProcess(StreamData& streamData);
public:
	static void CALLBACK WorkerThread(LPVOID lpParam);
	BOOL Worker();
};

