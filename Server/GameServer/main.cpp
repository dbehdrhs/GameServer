// GameServer.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include "test1.h"
#include "ServiceManager.h"

int main()
{
	ServiceManager serviceManager;
	if (FALSE == serviceManager.ServiceStart())
	{
		return 0;
	}
	// TestClass test;

	char a;
	while (true)
	{
		// Sleep(5000);
		cin >> a;
	}

	// Load
	// Start Service
	// Stop Service

    return 0;
}

