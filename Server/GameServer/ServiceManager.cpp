#include "stdafx.h"

#include "CIOCP.h"
#include "CGameServer.h"
#include "ServiceManager.h"


ServiceManager::ServiceManager()
	: m_pIocp(NULL)
{
}


ServiceManager::~ServiceManager()
{
}

BOOL ServiceManager::ServiceStart()
{
	// Data Load

	// Server Ready
	m_pIocp = new CIOCP;
	if (m_pIocp == NULL)
	{
		return FALSE;
	}
	m_pIocp->Start(8989);

	CGameServer::GetInstance()->Start(m_pIocp);

	return TRUE;
}
