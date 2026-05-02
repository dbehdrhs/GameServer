#pragma once

struct StreamData
{
	WORD wUserID;
	char pData[TCP_SOCKET_BUFFER_SIZE];

	StreamData()
		: wUserID(0)
	{
		ZeroMemory(pData, sizeof(pData));
	}
};

class CBuffer
{
private:
	HANDLE           m_hQueueEvent;
	CRITICAL_SECTION m_criticalSection;

	std::queue<StreamData> m_queue;

public:
	CBuffer();
	~CBuffer();

	BOOL WriteData(StreamData streamData);
	BOOL ReadData(OUT StreamData& outData);

	HANDLE GetQueueEventHandle() { return m_hQueueEvent; }
	SIZE_T GetQueueCount()       { return m_queue.size(); }

private:
	BOOL Init();
	void Release();
};