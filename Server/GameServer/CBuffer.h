#pragma once

struct MessageNode
{
	WORD wUserID; // iocp::socketContext와 Game::user의 식별자
	PACKET_HEADER packetHeader;
	char pData[TCP_SOCKET_BUFFER_SIZE];

	MessageNode()
		: wUserID(0), packetHeader(PACKET_TEST)
	{
		ZeroMemory(pData, sizeof(pData));
	}
};

struct StreamData
{
	WORD wUserID;
	// DWORD dwRecvSize;
	char pData[TCP_SOCKET_BUFFER_SIZE];

	StreamData()
		: wUserID(0)/*, dwRecvSize(0)*/
	{
		ZeroMemory(pData, sizeof(pData));
	}
};

class CBuffer
{
private:
	HANDLE m_hRecvEvent;
	CRITICAL_SECTION m_criticalSection;

	std::queue<StreamData> m_streamQueue;
public:
	CBuffer();
	~CBuffer();

public:
	BOOL Init();
	void Release();

	////////////////////////////////////////////////////////////////////
	BOOL WriteData(StreamData streamData);
	BOOL ReadData(OUT StreamData& outData);
	////////////////////////////////////////////////////////////////////
	
	////////////////////////////////////////////////////////////////////
	HANDLE GetQueueEventHandle() { return m_hRecvEvent; }
	SIZE_T GetStreamQueueCount() { return m_streamQueue.size(); }

private: // 
	std::queue<MessageNode> m_messageQueue;

	BOOL WriteData(MessageNode message);
	BOOL WriteData(WORD wID, WORD wDataSize, PVOID pData);
	// BOOL WriteData(WORD wID, PVOID pData);

	MessageNode ReadData();
};

