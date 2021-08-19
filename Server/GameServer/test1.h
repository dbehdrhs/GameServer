#pragma once

class TestClass
{
public:
	TestClass();

	static void CALLBACK WorkerThread(LPVOID lpParam);
};