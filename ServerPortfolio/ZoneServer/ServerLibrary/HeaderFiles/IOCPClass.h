#pragma once
#pragma warning(disable:4005)
#include <winsock2.h>

#include "../HeaderFiles/FileLog.h"
#include "../HeaderFiles/Utils.h"
#include "../HeaderFiles/WorkerThread.h"

class IOCPClass
{
private:
	bool			m_failed;

	HANDLE			m_hIOCP;

	int				m_threadCount;
	WorkerThread**	m_workThreadBuffer;

	int				m_conCurrency;

public:
	IOCPClass();
	~IOCPClass();

	void Reset();
	void Associate(SOCKET _socket, unsigned long long _value);

	bool IsFailed() { return m_failed; }

	HANDLE GetIOCPHandle() { return m_hIOCP; }
};