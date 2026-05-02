#include "stdafx.h"
#include "CBuffer.h"
#include "CIOCP.h"
#include "CUser.h"
#include "CGameServer.h"

void CGameServer::SendToDB(StreamData& streamData)
{
	m_pDbSendBuffer->WriteData(streamData);
}

BOOL CGameServer::DBResultProcess(StreamData& streamData)
{
	PACKET_HEADER header;
	CopyMemory(&header, streamData.pData, sizeof(PACKET_HEADER));

	if (header.packetID == PACKET_LOGIN)
	{
		SC_LOGIN* pResult = (SC_LOGIN*)streamData.pData;
		printf("[%d] Login %s\n", streamData.wUserID, pResult->bSuccess ? "SUCCESS" : "FAIL");

		if (FALSE == pResult->bSuccess)
		{
			m_pUser[streamData.wUserID].OnDisconnect();
		}

		m_pIocp->SendData(streamData.wUserID, sizeof(SC_LOGIN), streamData.pData);
	}

	return TRUE;
}