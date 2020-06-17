#pragma once
#include <vector>
#include <algorithm>

#include "../ServerLibrary/HeaderFiles/OnlyHeaders/ManagerFrame_List.h"
#include "SendBuffer.h"

#include "PathFinding.h"

#include "User.h"
#include "FieldTilesData.h"
#include "MonsterLogicThread.h"
#include "SectorManager.h"

//=====================================================

//Field

//각 Field은 내부의 유저들과 몬스터들을 처리한다.

//=====================================================

class Field : public Manager_List<User>
{
private:
	WORD					m_fieldNum;
	VECTOR2					m_spawnPosition;

	//==================================================

	FieldTilesData			m_fieldTilesData;

	MonsterLogicThread		m_monsterLogicThread;

	SectorManager			m_sectorManager;

	//==================================================

	SendBuffer*				m_sendBuffer;

	std::vector<Sector*>	m_leaveSectorsVec;
	std::vector<Sector*>	m_enterSectorsVec;

	//time_t					start, end;

public:
	Field();
	~Field();

	void Init(int _num, VECTOR2 _spawnPosition);
	void GetMap(const char* _name);
	void InitMonsterThread(HANDLE _handle);

	void FieldSendAll(char * _buffer, int _size);

	void SectorSendAll(std::vector<Sector*>* _sectorsVec, char * _buffer, int _size);

	void SendUserList(User* _user);
	void SendUserList_InRange(User* _user);

	void EnterUser(User* _user);
	void SendEnterUserInfo(User* _user);

	void ExitUser(User* _user);
	void SendExitUserInfo(int _num);

	bool UserAttack(User * _user, int _monsterIndex);

	void RespawnUser(User* _user);

	void UpdateUserSector(User* _user);

	void LeaveSector(User* _user);
	void SendInvisibleUserList(User* _user);
	void SendInvisibleMonsterList(User* _user);

	void EnterSector(User* _user);
	void SendVisibleUserList(User* _user);
	void SendVisibleMonsterList(User* _user);

	void EnterTestClient(User* _user, int _num);

	WORD GetFieldNum() { return m_fieldNum; }

	VECTOR2& GetSpawnPosition() { return m_spawnPosition; }

	FieldTilesData* GetTilesData() { return &m_fieldTilesData; }
};