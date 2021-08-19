#pragma once

enum IO_TYPE
{
	IO_RECV,
	IO_SEND,
	IO_END,
};

enum CONTEXT_STATE
{
	AcceptReady = 0,
	Connect,
	TransmitFilePending,
};

struct OverlappedEx : WSAOVERLAPPED
{
	WSABUF wsaBuf;
	char buf[TCP_SOCKET_BUFFER_SIZE];
	IO_TYPE ioType;

	OverlappedEx()
	{
		ZeroMemory(this, sizeof(WSAOVERLAPPED));
		ZeroMemory(buf, sizeof(buf));
		wsaBuf.buf = buf;
		wsaBuf.len = TCP_SOCKET_BUFFER_SIZE;

		ioType = IO_END;
	}
};

class SocketContext : public WSAOVERLAPPED
{
public:
	SocketContext();
	~SocketContext();

private:
	SOCKET m_socket;
	SOCKADDR_IN m_sockAddrIn;
	WORD m_wID; // 프로세스 안에서의 식별자
	CONTEXT_STATE contextState;

	OverlappedEx m_recvOverlapped;
	OverlappedEx m_sendOverlapped;

public:
	BOOL Init(WORD wID);

	WORD GetID() { return m_wID; }
	SOCKET GetSocket() { return m_socket; }
	CONTEXT_STATE& GetContextState() { return contextState; }

	OverlappedEx& GetRecvOverlapped() { return m_recvOverlapped; }
	OverlappedEx& GetSendOverlapped() { return m_sendOverlapped; }


	/////
	void PacketProcess(LPWSAOVERLAPPED pOV, DWORD dwBytes);

	BOOL AcceptPost(SOCKET listenSocket);
	BOOL AcceptProcess();
	BOOL DisconnectProcess();

	BOOL RecvPost();
	BOOL RecvProcess(DWORD dwRecvByte);
	BOOL SendPost();
	BOOL SendProcess(WORD wDataSize, PVOID pData);
};

