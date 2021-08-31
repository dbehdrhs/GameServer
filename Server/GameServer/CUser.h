#pragma once

enum USER_STATE : BYTE
{
	USER_STATE_READY = 0,
	USER_STATE_CONNECT,
	USER_STATE_LOGIN_COMPLETE,
};

class CUser
{
private:
	USER_STATE m_userState;
	WORD m_wID;
public:
	CUser();
	~CUser();

	void SetID(WORD wID) { m_wID = wID; }

public:
	USER_STATE GetState() { return m_userState; }
	void OnConnect() { m_userState = USER_STATE_CONNECT; }
	void OnDisconnect() { m_userState = USER_STATE_READY; }

public:
	// 
	BOOL PacketProcessLogin(CS_LOGIN* pPacket);
	BOOL PacketProcessLogout();
	BOOL PacketProcessTest(CS_TEST* pPacket);

	//
	BOOL DBProces(PVOID pData);
};

