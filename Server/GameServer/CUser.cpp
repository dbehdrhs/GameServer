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
	if (m_userState != USER_STATE_READY)
	{
		printf("[%d] Invalid User State", m_wID);;
		return FALSE;
	}

	m_userState = USER_STATE_LOGIN_COMPLETE;
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
