#include "stdafx.h"
#include "CBuffer.h"
#include "CIOCP.h"
#include "CUser.h"
#include "CGameServer.h"

void CGameServer::RegisterHandlers()
{
	m_handlers[PACKET_LOGIN]  = &CGameServer::PacketProcessLogin;
	m_handlers[PACKET_LOGOUT] = &CGameServer::PacketProcessLogout;
	m_handlers[PACKET_TEST]   = &CGameServer::PacketProcessTest;
}

BOOL CGameServer::PacketProcess(StreamData& streamData)
{
	PACKET_HEADER packetHeader;
	CopyMemory(&packetHeader, streamData.pData, sizeof(PACKET_HEADER));

	if (packetHeader.packetID != PACKET_LOGIN)
	{
		if (FALSE == IsUserLoggedIn(streamData.wUserID))
		{
			printf("[%d] Packet dropped - not logged in (packetID:%d)\n",
				streamData.wUserID, packetHeader.packetID);
			return FALSE;
		}
	}

	PACKET_ID id = packetHeader.packetID;

	if (id >= PACKET_MAX || m_handlers[id] == nullptr)
	{
		printf("[%d] Unknown packetID:%d size:%d\n",
			streamData.wUserID, id, packetHeader.wPacketSize);
		return FALSE;
	}

	return (this->*m_handlers[id])(streamData.wUserID, streamData.pData);
}

BOOL CGameServer::PacketProcessLogin(WORD wID, char* pData)
{
	CS_LOGIN* pPacket = (CS_LOGIN*)pData;

	if (FALSE == m_pUser[wID].PacketProcessLogin(pPacket))
	{
		return FALSE;
	}

	StreamData dbData;
	dbData.wUserID = wID;
	CopyMemory(dbData.pData, pData, sizeof(CS_LOGIN));
	SendToDB(dbData);

	return TRUE;
}

BOOL CGameServer::PacketProcessLogout(WORD wID, char* pData)
{
	printf("[%d] User Logout\n", wID);
	m_pUser[wID].PacketProcessLogout();
	return TRUE;
}

BOOL CGameServer::PacketProcessTest(WORD wID, char* pData)
{
	CS_TEST* pPacket = (CS_TEST*)pData;
	return m_pUser[wID].PacketProcessTest(pPacket);
}