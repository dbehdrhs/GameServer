#pragma once

struct MessageNode;
class SocketContext;

class CIOCP
{
private:
	enum IocpState : BYTE {
		stop = 0, startReady, startComplete,
	};

	IocpState m_state;
	SOCKET m_listenSocket;
	DWORD m_dwThreadCount;
	HANDLE m_hIocp;

	// DWORD m_dwAcceptKey;
	SocketContext* m_pSocketContext;
public:
	CIOCP();
	~CIOCP();

public:
	BOOL Start(WORD wPort, DWORD dwThreadCount = 0);
	void Stop();

	BOOL StartWinNet();
	BOOL CreateListenSocket(WORD wPort);
	BOOL CreateIOCP();
	BOOL CreateSocketContext();
	BOOL StartWorkerThread();

	// WorkerThread
	static void CALLBACK WorkerThread(LPVOID lpParam);
	void Worker();

	// 
	BOOL SendData(WORD wID, WORD wDataSize, PVOID pData);
};

