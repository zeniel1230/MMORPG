#pragma once
#include <chrono>

#include "stdafx.h"
#include "../ServerLibrary/HeaderFiles/OnlyHeaders/DoubleQueue.h"
#include "../ServerLibrary/HeaderFiles/ClientSession.h"

#include "UnitInfo.h"
#include "PathFinding.h"
#include "FieldTilesData.h"

#include "packet.h"

//=====================================================

//User

//Session과 Unit을 상속받아 쓰는 클래스

//=====================================================

struct INFO
{
	UserInfo	userInfo;
	UnitInfo	unitInfo;
};

class Field;
class Sector;

class User final : public ClientSession
{
public:
	bool				m_isInHashMap;

	bool				m_isAlreadyDisConnected;

private:
	INFO				m_basicInfo;

	Tile*				m_tile;

	Sector*				m_sector;

	Field*				m_field;
	FieldTilesData*		m_fieldTilesData;

	//===============================================
	bool				m_isGetUserList;

public:
	User();
	~User();

	void OnConnect();
	void DisConnect();
	void DisConnect_ChangeZone(int _num);
	void Reset();

	void OnRecv();

	//유저의 정보를 DB에 올리는 함수
	void UpdateInfo();

	//죽음
	void Death();
	//부활 및 첫스폰
	void Respawn(const VECTOR2& _spawnPosition);
	//맞음
	void Hit(int _index, int _damage);

	//경험치 획득
	void PlusExp(int _exp);
	//레벨업
	void LevelUp();

	//유저의 정보 받기요청(DB서버로)
	void RequestUserInfo(int _num);
	//유저의 정보 받기 성공 -> 유저 정보 전송(클라에게)
	void SendInfo(GetSessionInfoPacket* _packet);
	//유저의 정보 받기 실패
	void RequestUserInfoFailed();
	//Field 입장 시
	void EnterField(Field *_field, int _fieldNum, VECTOR2* _spawnPosition);
	//User의 포지션(이동시 호출됨)
	void SetPosition(Position& _position);

	bool CompareSector(Sector* _sector);

	//테스트용
	void TestClientEnterField(Field* _Field, int _fieldNum, int _dummyNum, VECTOR2* _spawnPosition);

	Tile* GetTile() { return m_tile; }
	INFO* GetInfo() { return &m_basicInfo; }

	Field* GetField() { return m_field; }
	void SetField(Field* _field) { m_field = _field; }

	Sector* GetSector() { return m_sector; }
	void SetSector(Sector* _sector) { m_sector = _sector; }

	bool IsDeath() { return m_basicInfo.unitInfo.state == STATE::DEATH; }

	void SetState(STATE _state) { m_basicInfo.unitInfo.state = _state; }

	bool GetIsGetUserList() { return m_isGetUserList; }
	void SetIsGetUserList(bool _bool) { m_isGetUserList = _bool; }
};

