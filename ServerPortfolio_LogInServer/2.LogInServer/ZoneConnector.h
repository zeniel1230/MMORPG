#pragma once
#include "../ServerLibrary/HeaderFiles/FileLog.h"
#include "../ServerLibrary/HeaderFiles/OnlyHeaders/IpEndPoint.h"
#include "../ServerLibrary/HeaderFiles/OnlyHeaders/DoubleQueue.h"
#include "../ServerLibrary/HeaderFiles/ClientSession.h"

#include "packet.h"

#include "MainThread.h"

class ZoneConnector : public ClientSession
{
private:
	int m_num;

public:
	ZoneConnector();
	~ZoneConnector();

	void OnConnect();
	void DisConnect();
	void Reset();

	void OnRecv();

	void HeartBeat();

	void AuthenticationSuccess(AuthenticationPacket* _packet);
	void AuthenticationFailed(AuthenticationPacket* _packet);

	void SetZoneNum(int _num) { m_num = _num; }
	int GetZoneNum() { return m_num; }
};

