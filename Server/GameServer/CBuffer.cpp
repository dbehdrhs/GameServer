#include "stdafx.h"
#include "CBuffer.h"


CBuffer::CBuffer()
	: m_hQueueEvent(INVALID_HANDLE_VALUE)
{
	Init();
}

CBuffer::~CBuffer()
{
	Release();
}

BOOL CBuffer::Init()
{
	m_hQueueEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_hQueueEvent == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	InitializeCriticalSection(&m_criticalSection);
	return TRUE;
}

void CBuffer::Release()
{
	CloseHandle(m_hQueueEvent);
	DeleteCriticalSection(&m_criticalSection);
}

BOOL CBuffer::WriteData(StreamData streamData)
{
	EnterCriticalSection(&m_criticalSection);
	m_queue.push(streamData);
	SetEvent(m_hQueueEvent);
	LeaveCriticalSection(&m_criticalSection);
	return TRUE;
}

BOOL CBuffer::ReadData(OUT StreamData& outData)
{
	EnterCriticalSection(&m_criticalSection);

	if (m_queue.empty())
	{
		LeaveCriticalSection(&m_criticalSection);
		return FALSE;
	}

	outData = m_queue.front();
	m_queue.pop();

	LeaveCriticalSection(&m_criticalSection);
	return TRUE;
}