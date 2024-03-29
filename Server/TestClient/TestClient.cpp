// TestClient.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"

int main()
{
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET socket;
	sockaddr_in serverAddr;

	socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


	if (socket == INVALID_SOCKET)
	{
		printf("Cannot create socket !!\n");
		return 0;
	}

	//  접속할 서버의 정보를 설정한다.
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	int nResult = inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr.s_addr);
	// serverAddr.sin_addr.s_addr = ::inet_addr("127.0.0.1");  // inet_pton()/*::inet_addr("127.0.0.1");*/
	serverAddr.sin_port = ::htons(8989);

	if (::connect(socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		printf("Cannot connect to server !!\n");
		return 0;
	}

	char sendBuffer[127] = { 0, };
	char recvBuffer[127] = {0, };
	int sentBytes = 0;
	int recvBytes = 0;

	CS_TEST testPacket;
	char testBuffer[127] = { 0, };

	DWORD dwCount = 0;
	//  서버와 통신을 한다.
	while(true)
	{
		testPacket.nAccountID = ++dwCount;
		testPacket.nCharacterID = dwCount;
		testPacket.nItemID = dwCount;

		CopyMemory(sendBuffer, &testPacket, sizeof(testPacket));

		sentBytes = ::send(socket, sendBuffer, sizeof(testPacket), 0);

		
		printf("[%d]%d bytes sent.\n",dwCount, sentBytes);

		Sleep(rand() % 5000);

		// std::cin >> sendBuffer;

		/*std::cin >> sendBuffer;
		sentBytes = ::send(socket, sendBuffer, (int)::strlen(sendBuffer) + 1, 0);*/

		//recvBytes = ::recv(socket, recvBuffer, 127, 0);
		// printf("%d bytes Received\n%s\n", recvBytes, recvBuffer);
		//break;
	}
	/*sentBytes = ::send(socket, sendBuffer, (int)::strlen(sendBuffer) + 1, 0);
	printf("%d bytes sent.\n", sentBytes);*/
	closesocket(socket);

	// std::cin >> sendBuffer;

	WSACleanup();

    return 0;
}

