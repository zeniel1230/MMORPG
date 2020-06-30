#pragma once
#include <algorithm>

#include "../ServerLibrary/HeaderFiles/FileLog.h"
#include "../ServerLibrary/HeaderFiles/OnlyHeaders/Thread.h"

#include "User.h"
#include "SessionManager.h"
#include "DBConnector.h"

class HeartBeatThread : public Thread<HeartBeatThread>
{
private:
	SessionManager*					m_sessionManager;

public:
	HeartBeatThread(SessionManager* _sessionManager);
	~HeartBeatThread();

	void LoopRun();

	void HeartBeat(Packet* _packet);
};

