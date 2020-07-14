#pragma once
#include "AcceptorSession.h"

class Acceptor : public AcceptorSession
{
public:
	Acceptor(const char* _ip, const unsigned short _port, HANDLE _handle, int _num);
	~Acceptor();

	void OnAccept();
};

