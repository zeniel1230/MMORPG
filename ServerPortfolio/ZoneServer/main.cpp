//라이브러리 추가
#pragma comment (lib, "../x64/Debug/1.ServerLibrary.lib")

#include <memory>

#include "../ServerLibrary/HeaderFiles/FileLog.h"
#include "../ServerLibrary/HeaderFiles/Utils.h"
#include "../ServerLibrary/HeaderFiles/IOCPClass.h"

#include "Acceptor.h"

#include "ZoneServer.h"

WSADATA m_wsaData;

IOCPClass* iocpClass;
Acceptor* acceptor;

int portNum;

int main()
{
	std::cout << "<< 몇 번째 Zone인지 입력(1부터) >>" << endl;
	std::cin >> portNum;
	portNum += 30004;
	//port는 30005부터

	if (WSAStartup(MAKEWORD(2, 2), &m_wsaData) != 0)
	{
		printf("[ Failed WSAStartup() ] \n");

		return 0;
	}

	TRYCATCH(iocpClass = new IOCPClass());
	if (iocpClass->IsFailed()) return false;

	ZoneServer* zoneServer = new ZoneServer(*iocpClass);
	if (!zoneServer->Start())
	{
		MYDEBUG("[ Zone Server Initializing Fail ]\n");

		WSACleanup();

		return 0;
	}

	TRYCATCH(acceptor = new Acceptor("192.168.0.13", portNum,
		iocpClass->GetIOCPHandle(), 06));
	if (acceptor->IsFailed()) return false;
	iocpClass->Associate(acceptor->GetListenSocket(),
		(unsigned long long)acceptor->GetListenSocket());

	MYDEBUG("[ 존 서버 Port : %d ]\n", portNum);

	WaitForSingleObject(MainThread::getSingleton()->GetHandle(), INFINITE);

	delete acceptor;
	delete zoneServer;
	delete iocpClass;

	WSACleanup();

	return 0;
}