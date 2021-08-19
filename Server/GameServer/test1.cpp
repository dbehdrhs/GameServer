#include "stdafx.h"
#include "test1.h"

TestClass::TestClass()
{
	DWORD dwThreadID;

	for (int i = 0; i < 10; i++)
	{
		HANDLE thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&WorkerThread, NULL, 0, &dwThreadID);
	}
	
}

void CALLBACK TestClass::WorkerThread(LPVOID lpParam)
{
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	//lpSystemInfo.

	// char a;
	while (true)
	{
		// cin >> a;
	}
}