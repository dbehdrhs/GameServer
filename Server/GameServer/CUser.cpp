#include "stdafx.h"
#include "CUser.h"


CUser::CUser()
	: m_userState(USER_STATE_READY), m_wID(0)
{
}


CUser::~CUser()
{
}

BOOL CUser::PacketProcessLogin(CS_LOGIN * pPacket)
{
	if (m_userState != USER_STATE_CONNECT)
	{
		printf("[%d] Invalid User State : %d\n", m_wID, m_userState);
		return FALSE;
	}

	m_userState = USER_STATE_LOGIN_COMPLETE;
	printf("[%d] Login Complete : %s\n", m_wID, pPacket->szID);
	return TRUE;
}

BOOL CUser::PacketProcessLogout()
{
	m_userState = USER_STATE_READY;
	return TRUE;
}

BOOL CUser::PacketProcessTest(CS_TEST * pPacket)
{
	printf("[%d] CUser::PacketProcessTest(), %d, %d, %d\n", m_wID, pPacket->nAccountID, pPacket->nCharacterID, pPacket->nItemID);

	return TRUE;
}

BOOL CUser::DBProces(PVOID pData)
{
	return TRUE;
}
