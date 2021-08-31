#pragma once

class CDB
{
private:
	BYTE byThreadCount;
public:
	CDB();
	~CDB();

public:
	BOOL Start();
	BOOL Stop();

public:
	void SendDbResult(WORD wID);

public:
	static void CALLBACK WorkerThread(LPVOID lpParam);
	BOOL Worker();

public:
	void TestC();
	//test connect/c++
	/*void TestInsert();
	void TestSelect();
	void TestUpdate();*/
};

