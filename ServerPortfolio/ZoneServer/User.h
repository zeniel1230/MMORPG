#pragma once
#include "stdafx.h"
#include"../ServerLibrary/HeaderFiles/OnlyHeaders/DoubleQueue.h"

#include "Unit.h"
#include "Session.h"
#include "PathFinding.h"

#include "packet.h"

#include "FieldTilesData.h"

//=====================================================

//User

//Session과 Unit을 상속받아 쓰는 클래스

//=====================================================

struct INFO
{
	UserInfo	userInfo;
	UnitInfo	unitInfo;
};

struct PacketQueuePair_User
{
	class User* user;
	Packet* packet;

	PacketQueuePair_User(User* _user, Packet* _packet)
	{
		user = _user;
		packet = _packet;
	}
};

class Field;
class Sector;

class User : public Session, public Unit
{
private:
	INFO				m_basicInfo;

	Sector*				m_sector;

	Tile*				m_tile;

	Field*				m_field;
	FieldTilesData*		m_fieldTilesData;

	bool				m_isGetUserList;

	DoubleQueue<PacketQueuePair_User>&	m_doubleQueue;

	//===============================================
	bool				m_isTestClient;

public:
	User(DoubleQueue<PacketQueuePair_User>& _doubleQueue)
		: m_doubleQueue(_doubleQueue)
	{

	}

	~User();

	void Init(SOCKET _listenSocket, HANDLE _handle);
	void OnConnect();
	void Disconnect();
	void Reset();
	void CheckCompletion(ST_OVERLAPPED* _overlapped);
	void Parsing();

	//살아있는지 체크
	void HeartBeatChecked();

	//유저의 정보를 DB에 올리는 함수
	void UpdateInfo();

	//죽음
	void Death();
	//부활 및 첫스폰
	void Respawn(VECTOR2 _spawnPosition);
	//맞음
	void Hit(int _index, int _damage);

	//경험치 획득
	void PlusExp(int _exp);
	//레벨업
	void LevelUp();

	//유저의 정보 받기요청(DB서버로)
	void RequestUserInfo();
	//유저의 정보 받기 성공 -> 유저 정보 전송(클라에게)
	void SendInfo(GetSessionInfoPacket* _packet);
	//유저의 정보 받기 실패
	void RequestUserInfoFailed();
	//Field 입장 시
	void EnterField(Field *_field, int _fieldNum, VECTOR2 _spawnPosition);
	//User의 포지션(이동시 호출됨)
	void SetPosition(Position& _position);

	void LogInUser(LogInPacket* _packet);
	void RegisterUser(RegisterUserPacket* _packet);

	//로그인 중복됨(이미 접속중임)->실패처리
	void LogInDuplicated();
	void LogInSuccess(int _num);
	void LogInFailed();

	void RegisterSuccess();
	void RegisterFailed();

	bool CompareSector(Sector* _sector);

	//테스트용
	void TestClientEnterField(Field* _Field, int _fieldNum, int _dummyNum, VECTOR2 _spawnPosition);

	Tile* GetTile() { return m_tile; }
	INFO* GetInfo() { return &m_basicInfo; }

	Field* GetField() { return m_field; }
	void SetField (Field* _field) { m_field = _field; }

	Sector* GetSector() { return m_sector; }
	void SetSector(Sector* _sector) { m_sector = _sector; }

	bool IsDeath() { return m_basicInfo.unitInfo.state == STATE::DEATH; }

	bool IsTestClient() { return m_isTestClient; }

	void SetState(STATE _state) { m_basicInfo.unitInfo.state = _state; }

	bool GetIsGetUserList() { return m_isGetUserList; }
	void SetIsGetUserList(bool _bool) { m_isGetUserList = _bool; }
};												 

