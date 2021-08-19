#include "stdafx.h"
#include "CBuffer.h"


CBuffer::CBuffer()
	: m_hRecvEvent(INVALID_HANDLE_VALUE)
{
	Init();
}


CBuffer::~CBuffer()
{
	Release();
}

BOOL CBuffer::Init()
{
	m_hRecvEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (INVALID_HANDLE_VALUE == m_hRecvEvent)
	{
		return FALSE;
	}

	InitializeCriticalSection(&m_criticalSection);

	return TRUE;
}

void CBuffer::Release()
{
	CloseHandle(m_hRecvEvent);
	DeleteCriticalSection(&m_criticalSection);
}

BOOL CBuffer::WriteData(StreamData streamData)
{
	EnterCriticalSection(&m_criticalSection);
	m_streamQueue.push(streamData);
	SetEvent(m_hRecvEvent);

	LeaveCriticalSection(&m_criticalSection);

	return TRUE;
}

BOOL CBuffer::ReadData(OUT StreamData& outData)
{
	EnterCriticalSection(&m_criticalSection);

	if (m_streamQueue.empty() == TRUE)
	{
		LeaveCriticalSection(&m_criticalSection);
		return FALSE;
	}

	outData = m_streamQueue.front();
	m_streamQueue.pop();

	LeaveCriticalSection(&m_criticalSection);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////
BOOL CBuffer::WriteData(MessageNode message)
{
	EnterCriticalSection(&m_criticalSection);

	if (TCP_SOCKET_BUFFER_SIZE < message.packetHeader.wPacketSize)
	{
		LeaveCriticalSection(&m_criticalSection);
		return FALSE;
	}

	m_messageQueue.push(message);
	SetEvent(m_hRecvEvent);

	LeaveCriticalSection(&m_criticalSection);

	return TRUE;
}

BOOL CBuffer::WriteData(WORD wID, WORD wDataSize, PVOID pData)
{
	EnterCriticalSection(&m_criticalSection);

	if (TCP_SOCKET_BUFFER_SIZE < wDataSize)
	{
		LeaveCriticalSection(&m_criticalSection);
		return FALSE;
	}

	MessageNode message;
	message.wUserID = wID;
	message.packetHeader.wPacketSize = wDataSize;
	CopyMemory(message.pData, pData, wDataSize);

	m_messageQueue.push(message);
	SetEvent(m_hRecvEvent);

	LeaveCriticalSection(&m_criticalSection);

	return TRUE;
}

//BOOL CBuffer::WriteData(WORD wID, PVOID pData)
//{
//	EnterCriticalSection(&m_criticalSection);
//
//	StreamData streamData;
//	streamData.wUserID = wID;
//	CopyMemory(streamData.pData, pData, streamData.dwRecvSize);
//
//	m_streamQueue.push(streamData);
//	SetEvent(m_hRecvEvent);
//
//	LeaveCriticalSection(&m_criticalSection);
//
//	return TRUE;
//}

MessageNode CBuffer::ReadData()
{
	EnterCriticalSection(&m_criticalSection);

	if (m_messageQueue.empty() == TRUE)
	{
		LeaveCriticalSection(&m_criticalSection);
		// ResetEvent(m_hRecvEvent);

		return MessageNode();
	}

	MessageNode node = m_messageQueue.front();
	m_messageQueue.pop();
	// ResetEvent(m_hRecvEvent);

	LeaveCriticalSection(&m_criticalSection);


	return node;
}