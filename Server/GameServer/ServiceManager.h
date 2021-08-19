#pragma once

class CIOCP;
class CGameServer;

class ServiceManager
{
public:
	ServiceManager();
	~ServiceManager();

private:
	CIOCP* m_pIocp;

public:
	BOOL ServiceStart();
};

