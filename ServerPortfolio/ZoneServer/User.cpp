#include "User.h"
#include "Field.h"
#include "Sector.h"

#include "DBConnector.h"

User::~User()
{
}

void User::Init(SOCKET _listenSocket, HANDLE _handle)
{
	Session::Init(_listenSocket, _handle);

	m_field = nullptr;

	m_isGetUserList = false;

	m_isTestClient = false;

	m_sector = nullptr;
}

void User::OnConnect()
{
	Session::OnConnect();

	IsConnectedPacket* isConnectedPacket =
		reinterpret_cast<IsConnectedPacket*>(GetSendBuffer()->
			GetBuffer(sizeof(IsConnectedPacket)));

	isConnectedPacket->Init(SendCommand::Zone2C_ISCONNECTED, sizeof(IsConnectedPacket));
	isConnectedPacket->isConnected = m_isConnected;

	Session::Send(reinterpret_cast<char*>(isConnectedPacket), isConnectedPacket->size);

	//printf("[ isConnectedPacket 전송 완료 ]\n");
}

void User::Disconnect()
{
	Session::Disconnect();

	m_doubleQueue.AddObject(PacketQueuePair_User(this, nullptr));
	SetEvent(m_hEvent);
}

void User::Reset()
{
	Session::Reset();

	m_field = nullptr;

	m_isGetUserList = false;

	m_isTestClient = false;

	m_sector = nullptr;

	m_basicInfo.unitInfo.fieldNum = 0;
}

void User::CheckCompletion(ST_OVERLAPPED* _overlapped)
{
	if (_overlapped == &m_overlapped)
	{
		if (m_overlapped.bytes <= 0)
		{
			//printf("[INFO] BYTES <= 0\n");

			//printf("2 \n");
			Disconnect();

			return;
		}

		m_recvBuffer->Write(m_overlapped.bytes);

		Parsing();

		Recv();
	}
}

void User::Parsing()
{
	int tempNum = 20;

	while (1)
	{
		Packet* packet = reinterpret_cast<Packet*>(GetRecvBuffer()->CanParsing());

		if (packet == nullptr) break;

		m_doubleQueue.AddObject(PacketQueuePair_User(this, packet));
		SetEvent(m_hEvent);

		tempNum--;

		if (tempNum <= 0)
		{
			tempNum = 20;

			break;
		}
	}
}

void User::HeartBeatChecked()
{
	SetChecking(false);

	if (m_heartBeatCheckedCount >= 3)
	{
		if (!m_isTestClient)
		{
			Packet* UpdateInfoPacket = reinterpret_cast<Packet*>(Session::GetSendBuffer()->
				GetBuffer(sizeof(Packet)));
			UpdateInfoPacket->Init(SendCommand::Zone2C_UPDATE_INFO, sizeof(Packet));

			Session::Send(reinterpret_cast<char*>(UpdateInfoPacket), UpdateInfoPacket->size);
		}

		m_heartBeatCheckedCount = 0;

		//printf("[ HeartBeat Checking & Update Success : %d ]\n", m_heartBeatCheckedCount);
	}
	else
	{
		m_heartBeatCheckedCount++;

		//printf("[ HeartBeat Checking Success : %d ]\n", m_heartBeatCheckedCount);
	}
}

void User::UpdateInfo()
{
	printf("[ %d user - INFO UPDATE TO DATABASE START ] \n", m_basicInfo.userInfo.userID);

	UpdateUserPacket* updateUserPacket =
		reinterpret_cast<UpdateUserPacket*>(Session::GetSendBuffer()->
			GetBuffer(sizeof(UpdateUserPacket)));
	updateUserPacket->Init(SendCommand::Zone2DB_UPDATE_USER, sizeof(UpdateUserPacket));
	updateUserPacket->userIndex = m_basicInfo.userInfo.userID;
	updateUserPacket->unitInfo = m_basicInfo.unitInfo;
	updateUserPacket->socket = m_socket;

	DBCONNECTOR->Session::Send(reinterpret_cast<char*>(updateUserPacket), updateUserPacket->size);
}

void User::Death()
{
	m_basicInfo.unitInfo.state = STATE::DEATH;

	//죽었다고 패킷 전송해줘야함.
	UserNumPacket* userNumPacket =
		reinterpret_cast<UserNumPacket*>(Session::GetSendBuffer()->
			GetBuffer(sizeof(UserNumPacket)));

	userNumPacket->userIndex = m_basicInfo.userInfo.userID;
	userNumPacket->Init(SendCommand::Zone2C_USER_DEATH, sizeof(UserNumPacket));

	m_field->SectorSendAll(m_sector->GetRoundSectorsVec(), reinterpret_cast<char*>(userNumPacket), userNumPacket->size);
}

void User::Respawn(VECTOR2 _spawnPosition)
{
	m_basicInfo.unitInfo.state = STATE::IDLE;

	m_basicInfo.unitInfo.hp.currentValue = m_basicInfo.unitInfo.hp.maxValue;
	m_basicInfo.unitInfo.mp.currentValue = m_basicInfo.unitInfo.mp.maxValue;

	Unit::SetUnitPosition(_spawnPosition.x, _spawnPosition.y);
	m_tile = m_fieldTilesData->GetTile(
		static_cast<int>(m_basicInfo.unitInfo.position.x),
		static_cast<int>(m_basicInfo.unitInfo.position.y));

	SessionInfoPacket* sessionInfoPacket =
		reinterpret_cast<SessionInfoPacket*>(Session::GetSendBuffer()->
			GetBuffer(sizeof(SessionInfoPacket)));
	sessionInfoPacket->Init(SendCommand::Zone2C_USER_REVIVE, sizeof(SessionInfoPacket));

	sessionInfoPacket->info.userInfo = m_basicInfo.userInfo;
	sessionInfoPacket->info.unitInfo = m_basicInfo.unitInfo = m_unitInfo;

	Session::GetSendBuffer()->Write(sessionInfoPacket->size);

	m_field->SectorSendAll(m_sector->GetRoundSectorsVec(), reinterpret_cast<char*>(sessionInfoPacket), sessionInfoPacket->size);
}

void User::Hit(int _index, int _damage)
{
	if (m_basicInfo.unitInfo.state == STATE::DEATH) return;

	int damage = _damage - m_basicInfo.unitInfo.def;
	if (damage < 0) damage = 0;

	m_basicInfo.unitInfo.hp.currentValue -= damage;

	UserHitPacket* userHitPacket =
		reinterpret_cast<UserHitPacket*>(Session::GetSendBuffer()->
			GetBuffer(sizeof(UserHitPacket)));

	userHitPacket->userIndex = m_basicInfo.userInfo.userID;
	userHitPacket->monsterIndex = _index;
	userHitPacket->damage = damage;
	userHitPacket->Init(SendCommand::Zone2C_USER_HIT, sizeof(UserHitPacket));
	m_field->SectorSendAll(m_sector->GetRoundSectorsVec(), reinterpret_cast<char*>(userHitPacket), userHitPacket->size);

	if (m_basicInfo.unitInfo.hp.currentValue <= 0)
	{
		Death();
	}
}

void User::PlusExp(int _exp)
{
	m_basicInfo.unitInfo.exp.currentValue += _exp;

	UserExpPacket* userExpPacket =
		reinterpret_cast<UserExpPacket*>(Session::GetSendBuffer()->
			GetBuffer(sizeof(UserExpPacket)));

	userExpPacket->exp = _exp;
	userExpPacket->Init(SendCommand::Zone2C_USER_PLUS_EXP, sizeof(UserExpPacket));
	Session::GetSendBuffer()->Write(userExpPacket->size);

	Session::Send(reinterpret_cast<char*>(userExpPacket), userExpPacket->size);

	//이후 레벨업 체크해줘야함
	if (m_basicInfo.unitInfo.exp.currentValue >= m_basicInfo.unitInfo.exp.maxValue)
	{
		LevelUp();
	}
}

void User::LevelUp()
{
	int tempExp = m_basicInfo.unitInfo.exp.currentValue - m_basicInfo.unitInfo.exp.maxValue;

	m_basicInfo.unitInfo.level++;
	m_basicInfo.unitInfo.exp.currentValue = tempExp;
	m_basicInfo.unitInfo.hp.maxValue += 50;
	m_basicInfo.unitInfo.hp.currentValue = m_basicInfo.unitInfo.hp.maxValue;
	m_basicInfo.unitInfo.mp.currentValue = m_basicInfo.unitInfo.mp.maxValue;
	m_basicInfo.unitInfo.atk += 5;
	m_basicInfo.unitInfo.def += 2;

	UserNumPacket* userNumPacket =
		reinterpret_cast<UserNumPacket*>(Session::GetSendBuffer()->
			GetBuffer(sizeof(UserNumPacket)));

	userNumPacket->userIndex = m_basicInfo.userInfo.userID;
	userNumPacket->Init(SendCommand::Zone2C_USER_LEVEL_UP, sizeof(UserNumPacket));
	Session::GetSendBuffer()->Write(userNumPacket->size);

	m_field->SectorSendAll(m_sector->GetRoundSectorsVec(), reinterpret_cast<char*>(userNumPacket), userNumPacket->size);

	//다시 레벨업 체크
	if (m_basicInfo.unitInfo.exp.currentValue >= m_basicInfo.unitInfo.exp.maxValue)
	{
		LevelUp();
	}
}

void User::RequestUserInfo()
{
	RequireUserInfoPacket_DBAgent* RequireUserInfoPacket =
		reinterpret_cast<RequireUserInfoPacket_DBAgent*>(Session::GetSendBuffer()->
			GetBuffer(sizeof(RequireUserInfoPacket_DBAgent)));
	RequireUserInfoPacket->Init(SendCommand::Zone2DB_REQUEST_USER_DATA, sizeof(RequireUserInfoPacket_DBAgent));
	RequireUserInfoPacket->userIndex = m_basicInfo.userInfo.userID;
	RequireUserInfoPacket->socket = m_socket;

	DBCONNECTOR->Session::Send(reinterpret_cast<char*>(RequireUserInfoPacket), RequireUserInfoPacket->size);
}

//기본적인 유저의 정보를 보내준다. DB가 있으면 여기서 불러와서 보내줌.
void User::SendInfo(GetSessionInfoPacket* _packet)
{
	int tempNum = m_basicInfo.userInfo.userID;
	m_basicInfo.userInfo = _packet->info.userInfo;
	m_basicInfo.userInfo.userID = tempNum;
	m_basicInfo.unitInfo = _packet->info.unitInfo;
	m_index = m_basicInfo.userInfo.userID;

	SessionInfoPacket* sessionInfoPacket =
		reinterpret_cast<SessionInfoPacket*>(Session::GetSendBuffer()->
			GetBuffer(sizeof(SessionInfoPacket)));
	sessionInfoPacket->Init(SendCommand::Zone2C_REGISTER_USER, sizeof(SessionInfoPacket));
	Session::GetSendBuffer()->Write(sessionInfoPacket->size);

	sessionInfoPacket->info.userInfo = m_basicInfo.userInfo;
	sessionInfoPacket->info.unitInfo = m_unitInfo = m_basicInfo.unitInfo;

	Session::Send(reinterpret_cast<char*>(sessionInfoPacket), sessionInfoPacket->size);

	printf("[ REGISTER_USER 전송 완료 ]\n");
}

void User::RequestUserInfoFailed()
{
	printf("[ GetUserInfo Failed, Disconnect %d Socket User ]\n", m_socket);
	
	Disconnect();
}

//받아온 Field의 Num값을 User에 넣어주고 완료했다는 패킷 보냄. 이후 클라에선 씬 전환해줄듯
void User::EnterField(Field *_field, int _fieldNum, VECTOR2 _spawnPosition)
{
	EnterFieldPacket* enterFieldPacket =
		reinterpret_cast<EnterFieldPacket*>(Session::GetSendBuffer()->
			GetBuffer(sizeof(EnterFieldPacket)));
	enterFieldPacket->fieldNum = _fieldNum;

	enterFieldPacket->position = _spawnPosition;
	m_basicInfo.unitInfo.position.x = _spawnPosition.x;
	m_basicInfo.unitInfo.position.y = _spawnPosition.y;

	enterFieldPacket->Init(SendCommand::Zone2C_ENTER_FIELD, sizeof(EnterFieldPacket));
	Session::GetSendBuffer()->Write(enterFieldPacket->size);

	m_field = _field;
	m_basicInfo.unitInfo.fieldNum = m_field->GetFieldNum();
	m_fieldTilesData = m_field->GetTilesData();

	m_tile = m_fieldTilesData->GetTile(
		static_cast<int>(m_basicInfo.unitInfo.position.x),
		static_cast<int>(m_basicInfo.unitInfo.position.y));

	Session::Send(reinterpret_cast<char*>(enterFieldPacket), enterFieldPacket->size);

	printf("[ ENTER_FIELD 전송 완료 ]\n");
}

void User::SetPosition(Position& _position)
{
	float distance = VECTOR2(m_basicInfo.unitInfo.position - _position).Magnitude();

	/*printf("[%d User : SetPosition (%f, %f) - dir : %f ]\n",
		m_basicInfo.userInfo.userID, _position.x, _position.y, distance);*/

	m_basicInfo.unitInfo.position = _position;
	m_tile = m_fieldTilesData->GetTile(
		static_cast<int>(m_basicInfo.unitInfo.position.x),
		static_cast<int>(m_basicInfo.unitInfo.position.y));
}

void User::LogInUser(LogInPacket* _packet)
{
	LogInPacket_DBAgent* logInPacket_DBAgent =
		reinterpret_cast<LogInPacket_DBAgent*>(Session::GetSendBuffer()->
			GetBuffer(sizeof(LogInPacket_DBAgent)));
	logInPacket_DBAgent->Init(SendCommand::Zone2DB_LOGIN, sizeof(LogInPacket_DBAgent));
	logInPacket_DBAgent->socket = m_socket;
	strcpy_s(logInPacket_DBAgent->id, _packet->id);
	strcpy_s(logInPacket_DBAgent->password, _packet->password);

	DBCONNECTOR->Session::Send(reinterpret_cast<char*>(logInPacket_DBAgent), logInPacket_DBAgent->size);
}

void User::RegisterUser(RegisterUserPacket* _packet)
{
	RegisterPacket_DBAgent* registerPacket_DBAgent =
		reinterpret_cast<RegisterPacket_DBAgent*>(Session::GetSendBuffer()->
			GetBuffer(sizeof(RegisterPacket_DBAgent)));
	registerPacket_DBAgent->Init(SendCommand::Zone2DB_REGISTER, sizeof(RegisterPacket_DBAgent));
	registerPacket_DBAgent->socket = m_socket;
	strcpy_s(registerPacket_DBAgent->id, _packet->id);
	strcpy_s(registerPacket_DBAgent->password, _packet->password);

	DBCONNECTOR->Session::Send(reinterpret_cast<char*>(registerPacket_DBAgent), registerPacket_DBAgent->size);
}

void User::LogInDuplicated()
{
	Packet* LogInFailed = reinterpret_cast<Packet*>(Session::GetSendBuffer()->
		GetBuffer(sizeof(Packet)));
	LogInFailed->Init(SendCommand::Zone2C_LOGIN_FAILED_DUPLICATED, sizeof(Packet));

	Session::Send(reinterpret_cast<char*>(LogInFailed), LogInFailed->size);
}

void User::LogInSuccess(int _num)
{
	m_basicInfo.userInfo.userID = _num;

	Packet* LogInSuccess = reinterpret_cast<Packet*>(Session::GetSendBuffer()->
		GetBuffer(sizeof(Packet)));
	LogInSuccess->Init(SendCommand::Zone2C_LOGIN_SUCCESS, sizeof(Packet));
	printf("[ Login Success ] \n");

	Session::Send(reinterpret_cast<char*>(LogInSuccess), LogInSuccess->size);
}

void User::LogInFailed()
{
	Packet* LogInFailed = reinterpret_cast<Packet*>(Session::GetSendBuffer()->
		GetBuffer(sizeof(Packet)));
	LogInFailed->Init(SendCommand::Zone2C_LOGIN_FAILED, sizeof(Packet));
	printf("[ Login Failed ] \n");

	Session::Send(reinterpret_cast<char*>(LogInFailed), LogInFailed->size);
}

void User::RegisterSuccess()
{
	Packet* RegisterSuccessPacket = reinterpret_cast<Packet*>(Session::GetSendBuffer()->
		GetBuffer(sizeof(Packet)));
	RegisterSuccessPacket->Init(SendCommand::Zone2C_REGISTER_USER_SUCCESS, sizeof(Packet));

	Session::Send(reinterpret_cast<char*>(RegisterSuccessPacket), RegisterSuccessPacket->size);
}

void User::RegisterFailed()
{
	Packet* RegisterFailedPacket = reinterpret_cast<Packet*>(Session::GetSendBuffer()->
		GetBuffer(sizeof(Packet)));
	RegisterFailedPacket->Init(SendCommand::Zone2C_REGISTER_USER_FAILED, sizeof(Packet));

	Session::Send(reinterpret_cast<char*>(RegisterFailedPacket), RegisterFailedPacket->size);
}

bool User::CompareSector(Sector* _sector)
{
	if (m_sector == nullptr) return false;

	if (m_sector == _sector)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//테스트용
void User::TestClientEnterField(Field* _field, int _fieldNum, int _dummyNum, VECTOR2 _spawnPosition)
{
	m_isTestClient = true;

	m_field = _field;
	m_basicInfo.unitInfo.fieldNum = m_field->GetFieldNum();
	m_fieldTilesData = m_field->GetTilesData();

	m_basicInfo.userInfo.userID = _dummyNum;
	strcpy_s(m_basicInfo.userInfo.userName, "TestPlayer");

	Unit::SetUnitInfo(IDLE, 1,
		100, 100, 100, 100, 100, 0, 20, 0);
	Unit::SetUnitPosition(0, 0);

	m_basicInfo.unitInfo.position.x = _spawnPosition.x;
	m_basicInfo.unitInfo.position.y = _spawnPosition.y;

	m_tile = m_fieldTilesData->GetTile(
		static_cast<int>(m_basicInfo.unitInfo.position.x),
		static_cast<int>(m_basicInfo.unitInfo.position.y));

	SessionInfoPacket* sessionInfoPacket =
		reinterpret_cast<SessionInfoPacket*>(Session::GetSendBuffer()->
			GetBuffer(sizeof(SessionInfoPacket)));
	sessionInfoPacket->Init(SendCommand::Zone2C_REGISTER_TEST_USER, sizeof(SessionInfoPacket));
	Session::GetSendBuffer()->Write(sessionInfoPacket->size);

	sessionInfoPacket->info.userInfo = m_basicInfo.userInfo;
	sessionInfoPacket->info.unitInfo = m_unitInfo = m_basicInfo.unitInfo;

	Session::Send(reinterpret_cast<char*>(sessionInfoPacket), sessionInfoPacket->size);
}