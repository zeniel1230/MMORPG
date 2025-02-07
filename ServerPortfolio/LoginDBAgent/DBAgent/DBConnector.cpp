#include "DBConnector.h"

#include "MainThread.h"

#ifdef _DEFINE_ODBC_
DBConnector::DBConnector(int _num)
{
	m_sendBuffer = new SendBuffer(30000);

	m_state = READY;
	m_num = _num + 1;

	m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (m_hEvent == NULL)
	{
		int num = WSAGetLastError();

		MYDEBUG("[Event Handle Creating Fail - %d Error]\n", num);
	}

	m_henv = NULL;
	m_hdbc = NULL;
	m_hstmt = NULL;
	m_retcode = NULL;

	m_isConnect = false;
	m_packet = nullptr;

	Thread::Start(this);
}


DBConnector::~DBConnector()
{
}

bool DBConnector::Connect(const char* host,
	const char* username,
	const char* password,
	const char* database,
	const unsigned int port,
	const char* unix_socket,
	const unsigned int client_flag)
{
	// 환경 핸들
	m_retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_henv);

	// 드라이버 환경 설정
	m_retcode = SQLSetEnvAttr(m_henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

	// 연결 핸들
	m_retcode = SQLAllocHandle(SQL_HANDLE_DBC, m_henv, &m_hdbc);

	// ######### 여기 중요      드라이버 이름                            ID                               비번
	m_retcode = SQLConnectA(m_hdbc, (SQLCHAR*) "odbcTest", SQL_NTS, (SQLCHAR*) "admin_odbc", SQL_NTS, (SQLCHAR *) "asd123!", SQL_NTS);

	if (m_retcode != 0)
	{
		printf("[ %d Connector - DB Connect Failed ]\n", m_num);

		return false;
	}
	else
	{
		m_isConnect = true;

		printf("[ %d Connector - DB Connect Success ]\n", m_num);

		return true;
	}
}

void DBConnector::DisConnect()
{
	SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);

	m_isConnect = false;
}

bool DBConnector::Query(const char* query)
{
	if (SQLExecDirect(m_hstmt, (SQLWCHAR*)query, SQL_NTS) != SQL_SUCCESS)
		return false;

	return true;
}

void DBConnector::Login(LogInPacket_DBAgent* _packet)
{
	/*SQLPrepareA(m_hstmt, (SQLCHAR*)"call GetAllAccountTable()", SQL_NTS);

	SQLExecute(m_hstmt);

	SQLLEN len1, len2, len3;
	SQLWCHAR userID[15], password[15];
	SQLINTEGER idx;

	SQLBindCol(m_hstmt, 1, SQL_CHAR, &userID, sizeof(userID), &len1);
	SQLBindCol(m_hstmt, 2, SQL_CHAR, &password, sizeof(password), &len2);
	SQLBindCol(m_hstmt, 3, SQL_INTEGER, &idx, sizeof(idx), &len3);*/

	SQLHDESC hIpd = NULL;
	SQLLEN cbValue = SQL_NTS;

	char str1[256];
	sprintf(str1, "call GetAccount_UsingID(?)");

	SQLPrepareA(m_hstmt, (SQLCHAR*)str1, SQL_NTS);
	SQLBindParameter(m_hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 15, 0, _packet->id, 15, &cbValue);
	SQLExecute(m_hstmt);

	SQLLEN len1, len2, len3;
	SQLWCHAR userID[15], password[15];
	SQLINTEGER idx;

	SQLBindCol(m_hstmt, 1, SQL_CHAR, &userID, sizeof(userID), &len1);
	SQLBindCol(m_hstmt, 2, SQL_CHAR, &password, sizeof(password), &len2);
	SQLBindCol(m_hstmt, 3, SQL_INTEGER, &idx, sizeof(idx), &len3);

	//해당 id가 존재함 - 로그인 가능
	if (SQLFetch(m_hstmt) != SQL_NO_DATA)
	{
		int result = strcmp((const char*)(password), _packet->password);

		//비밀번호 일치
		if (result == 0)
		{
			LogInSuccessPacket* logInSuccessPacket =
				reinterpret_cast<LogInSuccessPacket*>(m_sendBuffer->
					GetBuffer(sizeof(LogInSuccessPacket)));
			logInSuccessPacket->Init(SendCommand::DB2Zone_LOGIN_SUCCESS, sizeof(LogInSuccessPacket));
			logInSuccessPacket->socket = _packet->socket;
			logInSuccessPacket->userIndex = idx;

			m_dbAgent->Send(reinterpret_cast<char*>(logInSuccessPacket),
				logInSuccessPacket->size);
		}
		//비밀번호 불일치
		else
		{
			PacketWithSocket* logInFailedPacket =
				reinterpret_cast<PacketWithSocket*>(m_sendBuffer->
					GetBuffer(sizeof(PacketWithSocket)));
			logInFailedPacket->Init(SendCommand::DB2Zone_LOGIN_FAILED_WRONG_PASSWORD, sizeof(PacketWithSocket));
			logInFailedPacket->socket = _packet->socket;

			m_dbAgent->Send(reinterpret_cast<char*>(logInFailedPacket),
				logInFailedPacket->size);
		}
	}
	//해당 id 존재하지 않음
	else
	{
		PacketWithSocket* logInFailedPacket =
			reinterpret_cast<PacketWithSocket*>(m_sendBuffer->
				GetBuffer(sizeof(PacketWithSocket)));
		logInFailedPacket->Init(SendCommand::DB2Zone_LOGIN_FAILED_INVALID_ID, sizeof(PacketWithSocket));
		logInFailedPacket->socket = _packet->socket;

		m_dbAgent->Send(reinterpret_cast<char*>(logInFailedPacket),
			logInFailedPacket->size);
	}
}

void DBConnector::Register(RegisterPacket_DBAgent* _packet)
{
	SQLHDESC hIpd = NULL;
	SQLLEN cbValue = SQL_NTS;

	char str1[256];
	sprintf(str1, "call GetAccount_UsingID(?)");

	SQLPrepareA(m_hstmt, (SQLCHAR*)str1, SQL_NTS);
	SQLBindParameter(m_hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 15, 0, _packet->id, 15, &cbValue);
	SQLExecute(m_hstmt);

	SQLLEN len1, len2, len3;
	SQLWCHAR userID[15], password[15];
	SQLINTEGER idx;

	SQLBindCol(m_hstmt, 1, SQL_CHAR, &userID, sizeof(userID), &len1);
	SQLBindCol(m_hstmt, 2, SQL_CHAR, &password, sizeof(password), &len2);
	SQLBindCol(m_hstmt, 3, SQL_INTEGER, &idx, sizeof(idx), &len3);

	//해당 id가 존재함 - 회원가입 불가능
	if (SQLFetch(m_hstmt) != SQL_NO_DATA)
	{
		PacketWithSocket* RegisterFailedPacket =
			reinterpret_cast<PacketWithSocket*>(m_sendBuffer->
				GetBuffer(sizeof(PacketWithSocket)));
		RegisterFailedPacket->Init(SendCommand::DB2Zone_REGISTER_FAILED, sizeof(PacketWithSocket));
		RegisterFailedPacket->socket = _packet->socket;

		m_dbAgent->Send(reinterpret_cast<char*>(RegisterFailedPacket),
			RegisterFailedPacket->size);
	}
	//해당 id가 존재하지 않음 - 회원가입 가능
	else
	{
		SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);

		SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);

		SQLHDESC hIpd = NULL;
		SQLLEN cbValue = SQL_NTS;

		char str1[256];
		sprintf(str1, "call RegisterAccount(?, ?)");

		SQLPrepareA(m_hstmt, (SQLCHAR*)str1, SQL_NTS);
		SQLBindParameter(m_hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 15, 0, _packet->id, 15, &cbValue);
		SQLBindParameter(m_hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 15, 0, _packet->password, 15, &cbValue);
		SQLExecute(m_hstmt);

		PacketWithSocket* RegisterSuccessPacket =
			reinterpret_cast<PacketWithSocket*>(m_sendBuffer->
				GetBuffer(sizeof(PacketWithSocket)));
		RegisterSuccessPacket->Init(SendCommand::DB2Zone_REGISTER_SUCCESS, sizeof(PacketWithSocket));
		RegisterSuccessPacket->socket = _packet->socket;

		m_dbAgent->Send(reinterpret_cast<char*>(RegisterSuccessPacket),
			RegisterSuccessPacket->size);
	}
}

void DBConnector::GetUserInfo(RequireUserInfoPacket_DBAgent* _packet)
{
	SQLHDESC hIpd = NULL;
	SQLLEN cbValue = SQL_NTS;

	int tempNum = _packet->userIndex;

	char str1[256];
	sprintf(str1, "call GetUserInfo(?)");

	SQLPrepareA(m_hstmt, (SQLCHAR*)str1, SQL_NTS);
	SQLBindParameter(m_hstmt, 1, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, 0, 0, &tempNum, 0, &cbValue);
	SQLExecute(m_hstmt);

	SQLLEN len[11];
	SQLWCHAR userName[20];
	SQLINTEGER idx, level, curHp, maxHp, curMp, maxMp, curExp, maxExp, atk, def;

	SQLBindCol(m_hstmt, 1, SQL_INTEGER, &idx, sizeof(idx), &len[0]);
	SQLBindCol(m_hstmt, 2, SQL_CHAR, &userName, sizeof(userName), &len[1]);
	SQLBindCol(m_hstmt, 3, SQL_INTEGER, &level, sizeof(level), &len[2]);
	SQLBindCol(m_hstmt, 4, SQL_INTEGER, &curHp, sizeof(curHp), &len[3]);
	SQLBindCol(m_hstmt, 5, SQL_INTEGER, &maxHp, sizeof(maxHp), &len[4]);
	SQLBindCol(m_hstmt, 6, SQL_INTEGER, &curMp, sizeof(curMp), &len[5]);
	SQLBindCol(m_hstmt, 7, SQL_INTEGER, &maxMp, sizeof(maxMp), &len[6]);
	SQLBindCol(m_hstmt, 8, SQL_INTEGER, &curExp, sizeof(curExp), &len[7]);
	SQLBindCol(m_hstmt, 9, SQL_INTEGER, &maxExp, sizeof(maxExp), &len[8]);
	SQLBindCol(m_hstmt, 10, SQL_INTEGER, &atk, sizeof(atk), &len[9]);
	SQLBindCol(m_hstmt, 11, SQL_INTEGER, &def, sizeof(def), &len[10]);

	//해당 id 존재함
	if (SQLFetch(m_hstmt) != SQL_NO_DATA)
	{
		//성공
		GetSessionInfoPacket* sessionInfoPacket =
			reinterpret_cast<GetSessionInfoPacket*>(m_sendBuffer->
				GetBuffer(sizeof(GetSessionInfoPacket)));
		sessionInfoPacket->Init(SendCommand::DB2Zone_GET_USER_DATA_SUCCESS, sizeof(GetSessionInfoPacket));
		sessionInfoPacket->socket = _packet->socket;

		sessionInfoPacket->info.userInfo.userID = _packet->userIndex;
		sessionInfoPacket->info.unitInfo.fieldNum = 0;
		sprintf(sessionInfoPacket->info.userInfo.userName, (const char*)userName);
		sessionInfoPacket->info.unitInfo.level = level;
		sessionInfoPacket->info.unitInfo.hp.currentValue = curHp;
		sessionInfoPacket->info.unitInfo.hp.maxValue = maxHp;
		sessionInfoPacket->info.unitInfo.mp.currentValue = curMp;
		sessionInfoPacket->info.unitInfo.mp.maxValue = maxMp;
		sessionInfoPacket->info.unitInfo.exp.currentValue = curExp;
		sessionInfoPacket->info.unitInfo.exp.maxValue = maxExp;
		sessionInfoPacket->info.unitInfo.atk = atk;
		sessionInfoPacket->info.unitInfo.def = def;

		m_dbAgent->Send(reinterpret_cast<char*>(sessionInfoPacket),
			sessionInfoPacket->size);
	}
	else
	{
		//실패
		PacketWithSocket* GetSessionInfoFailedPacket =
			reinterpret_cast<PacketWithSocket*>(m_sendBuffer->
				GetBuffer(sizeof(PacketWithSocket)));
		GetSessionInfoFailedPacket->Init(SendCommand::DB2Zone_GET_USER_DATA_FAILED, sizeof(PacketWithSocket));
		GetSessionInfoFailedPacket->socket = _packet->socket;

		m_dbAgent->Send(reinterpret_cast<char*>(GetSessionInfoFailedPacket),
			GetSessionInfoFailedPacket->size);
	}
}

void DBConnector::UpdateUser(UpdateUserPacket* _packet)
{
	SQLRETURN retcode;

	SQLHDESC hIpd = NULL;
	SQLLEN cbValue = SQL_NTS;

	char str1[256];
	sprintf(str1, "call GetUserInfo(?)");

	SQLPrepareA(m_hstmt, (SQLCHAR*)str1, SQL_NTS);
	SQLBindParameter(m_hstmt, 1, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, 0, 0, &_packet->userIndex, 0, &cbValue);
	SQLExecute(m_hstmt);

	/*SQLLEN len[11];
	SQLWCHAR userName[15];
	SQLINTEGER idx, level, curHp, maxHp, curMp, maxMp, curExp, maxExp, atk, def;

	SQLBindCol(m_hstmt, 1, SQL_INTEGER, &idx, sizeof(idx), &len[0]);
	SQLBindCol(m_hstmt, 2, SQL_CHAR, &userName, sizeof(userName), &len[1]);
	SQLBindCol(m_hstmt, 3, SQL_INTEGER, &level, sizeof(level), &len[2]);
	SQLBindCol(m_hstmt, 1, SQL_INTEGER, &curHp, sizeof(curHp), &len[3]);
	SQLBindCol(m_hstmt, 2, SQL_INTEGER, &maxHp, sizeof(maxHp), &len[4]);
	SQLBindCol(m_hstmt, 3, SQL_INTEGER, &curMp, sizeof(curMp), &len[5]);
	SQLBindCol(m_hstmt, 1, SQL_INTEGER, &maxMp, sizeof(maxMp), &len[6]);
	SQLBindCol(m_hstmt, 2, SQL_INTEGER, &curExp, sizeof(curExp), &len[7]);
	SQLBindCol(m_hstmt, 3, SQL_INTEGER, &maxExp, sizeof(maxExp), &len[8]);
	SQLBindCol(m_hstmt, 1, SQL_INTEGER, &atk, sizeof(atk), &len[9]);
	SQLBindCol(m_hstmt, 2, SQL_INTEGER, &def, sizeof(def), &len[10]);*/

	if ((SQLFetch(m_hstmt) != SQL_NO_DATA))
	{
		SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);

		SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);

		char str2[256];
		sprintf(str2, "call UpdateUser(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

		retcode = SQLPrepareA(m_hstmt, (SQLCHAR*)str2, SQL_NTS);

		SQLBindParameter(m_hstmt, 1, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, 0, 0, &_packet->userIndex, 0, &cbValue);
		SQLBindParameter(m_hstmt, 2, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, 0, 0, &_packet->unitInfo.level, 0, &cbValue);
		SQLBindParameter(m_hstmt, 3, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, 0, 0, &_packet->unitInfo.hp.currentValue, 0, &cbValue);
		SQLBindParameter(m_hstmt, 4, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, 0, 0, &_packet->unitInfo.hp.maxValue, 0, &cbValue);
		SQLBindParameter(m_hstmt, 5, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, 0, 0, &_packet->unitInfo.mp.currentValue, 0, &cbValue);
		SQLBindParameter(m_hstmt, 6, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, 0, 0, &_packet->unitInfo.mp.maxValue, 0, &cbValue);
		SQLBindParameter(m_hstmt, 7, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, 0, 0, &_packet->unitInfo.exp.currentValue, 0, &cbValue);
		SQLBindParameter(m_hstmt, 8, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, 0, 0, &_packet->unitInfo.exp.maxValue, 0, &cbValue);
		SQLBindParameter(m_hstmt, 9, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, 0, 0, &_packet->unitInfo.atk, 0, &cbValue);
		SQLBindParameter(m_hstmt, 10, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, 0, 0, &_packet->unitInfo.def, 0, &cbValue);
		SQLExecute(m_hstmt);

		printf("%d \n", retcode);
		if (retcode != 0)
		{
			//실패
			PacketWithSocket* updateUserFailedPacket =
				reinterpret_cast<PacketWithSocket*>(m_sendBuffer->
					GetBuffer(sizeof(PacketWithSocket)));
			updateUserFailedPacket->Init(SendCommand::DB2Zone_UPDATE_USER_FAILED, sizeof(PacketWithSocket));
			updateUserFailedPacket->socket = _packet->socket;

			m_dbAgent->Send(reinterpret_cast<char*>(updateUserFailedPacket),
				updateUserFailedPacket->size);
		}

		PacketWithSocket* updateUserSuccessPacket =
			reinterpret_cast<PacketWithSocket*>(m_sendBuffer->
				GetBuffer(sizeof(PacketWithSocket)));
		updateUserSuccessPacket->Init(SendCommand::DB2Zone_UPDATE_USER_SUCCESS, sizeof(PacketWithSocket));
		updateUserSuccessPacket->socket = _packet->socket;

		m_dbAgent->Send(reinterpret_cast<char*>(updateUserSuccessPacket),
			updateUserSuccessPacket->size);
	}
	else
	{
		//실패
		PacketWithSocket* updateUserFailedPacket =
			reinterpret_cast<PacketWithSocket*>(m_sendBuffer->
				GetBuffer(sizeof(PacketWithSocket)));
		updateUserFailedPacket->Init(SendCommand::DB2Zone_UPDATE_USER_FAILED, sizeof(PacketWithSocket));
		updateUserFailedPacket->socket = _packet->socket;

		m_dbAgent->Send(reinterpret_cast<char*>(updateUserFailedPacket),
			updateUserFailedPacket->size);
	}
}

void DBConnector::GetMonsterInfo()
{
	std::vector<MonsterData> monsterDataVec;

	for (int i = 1; i < 1000; i++)
	{
		SQLHDESC hIpd = NULL;
		SQLLEN cbValue = SQL_NTS;

		char str1[256];
		sprintf(str1, "call GetMonsterInfo(?)");

		SQLPrepareA(m_hstmt, (SQLCHAR*)str1, SQL_NTS);
		SQLBindParameter(m_hstmt, 1, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, 0, 0, &i, 0, &cbValue);
		SQLExecute(m_hstmt);

		SQLLEN len[11];
		SQLINTEGER monsterType, hp, attackDamage, patrolRange, dropExp;
		SQLINTEGER attackDelay, attackDistance, moveSpeed, patrolDelay, returnDistance;

		SQLBindCol(m_hstmt, 1, SQL_INTEGER, &monsterType, sizeof(monsterType), &len[0]);
		SQLBindCol(m_hstmt, 2, SQL_INTEGER, &hp, sizeof(hp), &len[1]);
		SQLBindCol(m_hstmt, 3, SQL_INTEGER, &attackDelay, sizeof(attackDelay), &len[2]);
		SQLBindCol(m_hstmt, 4, SQL_INTEGER, &attackDamage, sizeof(attackDamage), &len[3]);
		SQLBindCol(m_hstmt, 5, SQL_INTEGER, &attackDistance, sizeof(attackDistance), &len[4]);
		SQLBindCol(m_hstmt, 6, SQL_INTEGER, &moveSpeed, sizeof(moveSpeed), &len[5]);
		SQLBindCol(m_hstmt, 7, SQL_INTEGER, &patrolRange, sizeof(patrolRange), &len[6]);
		SQLBindCol(m_hstmt, 8, SQL_INTEGER, &patrolDelay, sizeof(patrolDelay), &len[7]);
		SQLBindCol(m_hstmt, 9, SQL_INTEGER, &returnDistance, sizeof(returnDistance), &len[8]);
		SQLBindCol(m_hstmt, 10, SQL_INTEGER, &dropExp, sizeof(dropExp), &len[9]);

		if (SQLFetch(m_hstmt) != SQL_NO_DATA)
		{
			MonsterData tempData;

			tempData.monsterType = monsterType;
			tempData.hp.currentValue = hp;
			tempData.hp.maxValue = hp;
			tempData.attackDelay = attackDelay;
			tempData.attackDamage = attackDamage;
			tempData.attackDistance = attackDistance;
			tempData.moveSpeed = moveSpeed;
			tempData.patrolRange = patrolRange;
			tempData.patrolDelay = patrolDelay;
			tempData.returnDistance = returnDistance;
			tempData.dropExp = dropExp;

			monsterDataVec.push_back(tempData);
		}
		else
		{
			break;
		}
	}

	if (monsterDataVec.size() <= 0) return;

	MonstersInfoPacket* monstersInfoPacket =
		reinterpret_cast<MonstersInfoPacket*>(m_sendBuffer->
			GetBuffer(sizeof(GetSessionInfoPacket)));

	monstersInfoPacket->count = monsterDataVec.size();

	for (int i = 0; i < monsterDataVec.size(); i++)
	{
		monstersInfoPacket->monstersData[i] = monsterDataVec[i];
	}

	monstersInfoPacket->size = (sizeof(MonsterData) * monsterDataVec.size())
		+ sizeof(WORD) + sizeof(Packet);
	monstersInfoPacket->Init(SendCommand::DB2Zone_MONSTERS_DATA, monstersInfoPacket->size);

	m_dbAgent->Send(reinterpret_cast<char*>(monstersInfoPacket),
		monstersInfoPacket->size);
}
#else
DBConnector::DBConnector(int _num)
{
	m_sendBuffer = new SendBuffer(30000);

	m_state = READY;
	m_num = _num + 1;

	m_isConnect = false;
	m_packet = nullptr;

	mysql_init(&m_connect);

	m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (m_hEvent == NULL)
	{
		int num = WSAGetLastError();

		MYDEBUG("[Event Handle Creating Fail - %d Error]\n", num);
	}

	Thread::Start(this);
}

DBConnector::~DBConnector()
{
	DisConnect();
}

bool DBConnector::Connect(const char* host,
	const char* username,
	const char* password,
	const char* database,
	const unsigned int port,
	const char* unix_socket,
	const unsigned int client_flag)
{
	if (&connect == NULL)
	{
		printf("%d Error : %s \n", mysql_errno(&m_connect), mysql_error(&m_connect));
		exit(1);
	}

	if (mysql_real_connect(&m_connect, host, username, password, database, port, unix_socket, client_flag) == NULL)
	{
		m_isConnect = false;

		printf("[ DB Connect Failed ]\n");

		return false;
	}

	m_isConnect = true;

	return true;
}

void DBConnector::DisConnect()
{
	if (&connect != NULL)
	{
		mysql_close(&m_connect);

		m_isConnect = false;
	}
}

bool DBConnector::SelectDB(const char* DBname)
{
	return mysql_select_db(&m_connect, DBname);
}

MYSQL_RES* DBConnector::Query(const char* query)
{
	if (!m_isConnect || &m_connect == NULL)
	{
		printf("%d Error : %s \n", mysql_errno(&m_connect), mysql_error(&m_connect));
		//exit(1);
	}

	if (mysql_query(&m_connect, query))
	{
		printf("%d Error : %s \n", mysql_errno(&m_connect), mysql_error(&m_connect));
		//exit(1);
	}

	m_result = mysql_store_result(&m_connect);

	return m_result;
}

void DBConnector::Login(LogInPacket_DBAgent* _packet)
{
	MYSQL_ROW mysql_row;

	char str1[256];
	sprintf(str1, "select *from accounttable where ID = '%s'", _packet->id);

	m_result = this->Query(str1);

	//해당 id 존재하지 않음
	if ((mysql_row = mysql_fetch_row(m_result)) == NULL)
	{
		PacketWithSocket* logInFailedPacket =
			reinterpret_cast<PacketWithSocket*>(m_sendBuffer->
				GetBuffer(sizeof(PacketWithSocket)));
		logInFailedPacket->Init(SendCommand::DB2Zone_LOGIN_FAILED_INVALID_ID, sizeof(PacketWithSocket));
		logInFailedPacket->socket = _packet->socket;

		m_dbAgent->Send(reinterpret_cast<char*>(logInFailedPacket),
			logInFailedPacket->size);
	}
	//해당 id 존재함
	else
	{
		int result = strcmp(mysql_row[1], _packet->password);

		//비밀번호 일치
		if (result == 0)
		{
			LogInSuccessPacket* logInSuccessPacket =
				reinterpret_cast<LogInSuccessPacket*>(m_sendBuffer->
					GetBuffer(sizeof(LogInSuccessPacket)));
			logInSuccessPacket->Init(SendCommand::DB2Zone_LOGIN_SUCCESS, sizeof(LogInSuccessPacket));
			logInSuccessPacket->socket = _packet->socket;
			logInSuccessPacket->userIndex = atoi(mysql_row[2]);

			m_dbAgent->Send(reinterpret_cast<char*>(logInSuccessPacket),
				logInSuccessPacket->size);
		}
		//비밀번호 불일치
		else
		{
			PacketWithSocket* logInFailedPacket =
				reinterpret_cast<PacketWithSocket*>(m_sendBuffer->
					GetBuffer(sizeof(PacketWithSocket)));
			logInFailedPacket->Init(SendCommand::DB2Zone_LOGIN_FAILED_WRONG_PASSWORD, sizeof(PacketWithSocket));
			logInFailedPacket->socket = _packet->socket;

			m_dbAgent->Send(reinterpret_cast<char*>(logInFailedPacket),
				logInFailedPacket->size);
		}
	}
}

void DBConnector::Register(RegisterPacket_DBAgent* _packet)
{
	MYSQL_ROW mysql_row;

	char str1[256];
	sprintf(str1, "select *from accounttable where ID = '%s'", _packet->id);

	m_result = this->Query(str1);

	//해당 id가 존재하지 않음 - 회원가입 가능
	if ((mysql_row = mysql_fetch_row(m_result)) == NULL)
	{
		m_result = this->Query("select *from accounttable");

		if (m_result)
		{
			rowNum = mysql_num_rows(m_result);
			fieldNum = mysql_num_fields(m_result);
			row = mysql_fetch_row(m_result);
			field = mysql_fetch_fields(m_result); //pfield[iFieldNum].name;
		}
		else
		{
			if (mysql_field_count(&m_connect) == 0)
			{
				rowNum = mysql_affected_rows(&m_connect);
			}
		}

		char str2[256];
		sprintf(str2, "INSERT INTO `accounttable` (`ID`, `PASSWORD`) VALUES('%s', '%s')",
			_packet->id, _packet->password);

		this->Query(str2);

		char str3[1024];
		sprintf(str3, "INSERT INTO `infotable` \
			(`userName`, `level`, `curHp`, `maxHp`, `curMp`, `maxMp`, `curExp`, `maxExp`, `atk`, `def`) \
			VALUES ('%s', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d')",
			_packet->id, 1, 100, 100, 100, 100, 0, 100, 20, 0);

		this->Query(str3);

		PacketWithSocket* RegisterSuccessPacket =
			reinterpret_cast<PacketWithSocket*>(m_sendBuffer->
				GetBuffer(sizeof(PacketWithSocket)));
		RegisterSuccessPacket->Init(SendCommand::DB2Zone_REGISTER_SUCCESS, sizeof(PacketWithSocket));
		RegisterSuccessPacket->socket = _packet->socket;

		m_dbAgent->Send(reinterpret_cast<char*>(RegisterSuccessPacket),
			RegisterSuccessPacket->size);
	}
	//해당 id가 존재함 - 회원가입 불가능
	else
	{
		PacketWithSocket* RegisterFailedPacket =
			reinterpret_cast<PacketWithSocket*>(m_sendBuffer->
				GetBuffer(sizeof(PacketWithSocket)));
		RegisterFailedPacket->Init(SendCommand::DB2Zone_REGISTER_FAILED, sizeof(PacketWithSocket));
		RegisterFailedPacket->socket = _packet->socket;

		m_dbAgent->Send(reinterpret_cast<char*>(RegisterFailedPacket),
			RegisterFailedPacket->size);
	}
}

void DBConnector::GetUserInfo(RequireUserInfoPacket_DBAgent* _packet)
{
	MYSQL_ROW mysql_row;

	char str1[256];
	sprintf(str1, "select *from infotable where userID = %d", _packet->userIndex);

	m_result = this->Query(str1);

	//중복이 아님
	if ((mysql_row = mysql_fetch_row(m_result)) == NULL)
	{
		//실패
		PacketWithSocket* GetSessionInfoFailedPacket =
			reinterpret_cast<PacketWithSocket*>(m_sendBuffer->
				GetBuffer(sizeof(PacketWithSocket)));
		GetSessionInfoFailedPacket->Init(SendCommand::DB2Zone_GET_USER_DATA_FAILED, sizeof(PacketWithSocket));
		GetSessionInfoFailedPacket->socket = _packet->socket;

		m_dbAgent->Send(reinterpret_cast<char*>(GetSessionInfoFailedPacket),
			GetSessionInfoFailedPacket->size);
	}
	else
	{
		//성공
		GetSessionInfoPacket* sessionInfoPacket =
			reinterpret_cast<GetSessionInfoPacket*>(m_sendBuffer->
				GetBuffer(sizeof(GetSessionInfoPacket)));
		sessionInfoPacket->Init(SendCommand::DB2Zone_GET_USER_DATA_SUCCESS, sizeof(GetSessionInfoPacket));
		sessionInfoPacket->socket = _packet->socket;

		sessionInfoPacket->info.userInfo.userID = _packet->userIndex;
		sessionInfoPacket->info.unitInfo.fieldNum = 0;
		sprintf(sessionInfoPacket->info.userInfo.userName, "%s", mysql_row[1]);
		sessionInfoPacket->info.unitInfo.level = atoi(mysql_row[2]);
		sessionInfoPacket->info.unitInfo.hp.currentValue = atoi(mysql_row[3]);
		sessionInfoPacket->info.unitInfo.hp.maxValue = atoi(mysql_row[4]);
		sessionInfoPacket->info.unitInfo.mp.currentValue = atoi(mysql_row[5]);
		sessionInfoPacket->info.unitInfo.mp.maxValue = atoi(mysql_row[6]);
		sessionInfoPacket->info.unitInfo.exp.currentValue = atoi(mysql_row[7]);
		sessionInfoPacket->info.unitInfo.exp.maxValue = atoi(mysql_row[8]);
		sessionInfoPacket->info.unitInfo.atk = atoi(mysql_row[9]);
		sessionInfoPacket->info.unitInfo.def = atoi(mysql_row[10]);

		m_dbAgent->Send(reinterpret_cast<char*>(sessionInfoPacket),
			sessionInfoPacket->size);
	}
}

void DBConnector::UpdateUser(UpdateUserPacket* _packet)
{
	MYSQL_ROW mysql_row;

	char str1[256];
	sprintf(str1, "select *from infotable where userID = %d", _packet->userIndex);

	m_result = this->Query(str1);

	//중복이 아님
	if ((mysql_row = mysql_fetch_row(m_result)) == NULL)
	{
		//실패
		PacketWithSocket* updateUserFailedPacket =
			reinterpret_cast<PacketWithSocket*>(m_sendBuffer->
				GetBuffer(sizeof(PacketWithSocket)));
		updateUserFailedPacket->Init(SendCommand::DB2Zone_UPDATE_USER_FAILED, sizeof(PacketWithSocket));
		updateUserFailedPacket->socket = _packet->socket;

		m_dbAgent->Send(reinterpret_cast<char*>(updateUserFailedPacket),
			updateUserFailedPacket->size);
	}
	else
	{
		//성공
		char str2[1024];
		sprintf(str2, "UPDATE infotable SET level = '%d', curHp = '%d', maxHp = '%d', curMp = '%d', maxMp = '%d', \
			curExp = '%d', maxExp = '%d', atk = '%d', def = '%d' WHERE userID = '%d'",
			_packet->unitInfo.level,
			_packet->unitInfo.hp.currentValue, _packet->unitInfo.hp.maxValue,
			_packet->unitInfo.mp.currentValue, _packet->unitInfo.mp.maxValue,
			_packet->unitInfo.exp.currentValue, _packet->unitInfo.exp.maxValue,
			_packet->unitInfo.atk, _packet->unitInfo.def, _packet->userIndex);

		m_result = this->Query(str2);

		PacketWithSocket* updateUserSuccessPacket =
			reinterpret_cast<PacketWithSocket*>(m_sendBuffer->
				GetBuffer(sizeof(PacketWithSocket)));
		updateUserSuccessPacket->Init(SendCommand::DB2Zone_UPDATE_USER_SUCCESS, sizeof(PacketWithSocket));
		updateUserSuccessPacket->socket = _packet->socket;

		m_dbAgent->Send(reinterpret_cast<char*>(updateUserSuccessPacket),
			updateUserSuccessPacket->size);
	}
}

void DBConnector::GetMonsterInfo()
{
	MYSQL_ROW mysql_row;

	std::vector<MonsterData> monsterDataVec;

	for (int i = 0; i < 1000; i++)
	{
		char str1[256];
		sprintf(str1, "select *from monstertable where monsterType = %d", i + 1);

		m_result = this->Query(str1);

		if ((mysql_row = mysql_fetch_row(m_result)) == NULL)
		{
			break;
		}
		else
		{
			MonsterData tempData;

			tempData.monsterType = i + 10001;
			tempData.hp.currentValue = atoi(mysql_row[1]);
			tempData.hp.maxValue = atoi(mysql_row[1]);
			tempData.attackDelay = (float)atof(mysql_row[2]);
			tempData.attackDamage = atoi(mysql_row[3]);
			tempData.attackDistance = (float)atof(mysql_row[4]);
			tempData.moveSpeed = (float)atof(mysql_row[5]);
			tempData.patrolRange = atoi(mysql_row[6]);
			tempData.patrolDelay = (float)atof(mysql_row[7]);
			tempData.returnDistance = (float)atof(mysql_row[8]);
			tempData.dropExp = atoi(mysql_row[9]);

			monsterDataVec.push_back(tempData);
		}
	}

	if (monsterDataVec.size() <= 0) return;

	MonstersInfoPacket* monstersInfoPacket =
		reinterpret_cast<MonstersInfoPacket*>(m_sendBuffer->
			GetBuffer(sizeof(GetSessionInfoPacket)));

	monstersInfoPacket->count = monsterDataVec.size();

	for (int i = 0; i < monsterDataVec.size(); i++)
	{
		monstersInfoPacket->monstersData[i] = monsterDataVec[i];
	}

	monstersInfoPacket->size = (sizeof(MonsterData) * monsterDataVec.size())
		+ sizeof(WORD) + sizeof(Packet);
	monstersInfoPacket->Init(SendCommand::DB2Zone_MONSTERS_DATA, monstersInfoPacket->size);

	m_dbAgent->Send(reinterpret_cast<char*>(monstersInfoPacket),
		monstersInfoPacket->size);
}
#endif

void DBConnector::SetDBAgent(DBAgent* _agent)
{
	m_dbAgent = _agent;
}

void DBConnector::SetPacket(Packet* _packet)
{
	m_packet = _packet;

	if (m_dbAgent == nullptr || m_packet == nullptr)
	{
		m_dbAgent = nullptr;
		m_packet = nullptr;

		MYDEBUG("Check");

		return;
	}

	SetEvent(m_hEvent);
}

void DBConnector::LoopRun()
{
	while (1)
	{
		WaitForSingleObject(m_hEvent, INFINITE);

		m_state = ACTIVE;

		SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);

		switch (static_cast<RecvCommand>(m_packet->cmd))
		{
		case RecvCommand::Zone2DB_LOGIN:
		{
			LogInPacket_DBAgent* logInPacket = static_cast<LogInPacket_DBAgent*>(m_packet);
			Login(logInPacket);
			printf("LogIn \n");
		}
		break;
		case RecvCommand::Zone2DB_REGISTER:
		{
			RegisterPacket_DBAgent* RegisterPacket = static_cast<RegisterPacket_DBAgent*>(m_packet);
			Register(RegisterPacket);
			printf("Register \n");
		}
		break;
		case RecvCommand::Zone2DB_REQUEST_USER_DATA:
		{
			RequireUserInfoPacket_DBAgent* packet = static_cast<RequireUserInfoPacket_DBAgent*>(m_packet);
			GetUserInfo(packet);
			printf("GetUserInfo \n");
		}
		break;
		case RecvCommand::Zone2DB_REQUEST_MONSTER_DATA:
		{
			MYDEBUG("[ ZoneServer Connected ]\n");

			GetMonsterInfo();
			printf("GetMonsterInfo \n");
		}
		break;
		case RecvCommand::Zone2DB_UPDATE_USER:
		{
			UpdateUserPacket* packet = static_cast<UpdateUserPacket*>(m_packet);
			UpdateUser(packet);

			printf("Update User \n");
		}
		break;
		case RecvCommand::Zone2DB_HEARTBEAT:
		{
			//MYDEBUG("[ HeartBeat Test ]\n");

			Packet* alivePacket =
				reinterpret_cast<Packet*>(m_sendBuffer->GetBuffer(sizeof(Packet)));
			alivePacket->Init(SendCommand::DB2Zone_ALIVE, sizeof(Packet));

			m_dbAgent->Send(reinterpret_cast<char*>(alivePacket), alivePacket->size);
		}
		break;
		}

		m_dbAgent->GetReceiver()->GetRingBuffer()->Read(m_packet->size);

		SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);

		m_dbAgent = nullptr;
		m_packet = nullptr;
		m_state = READY;

		//여기에 Mainthread Recv 이벤트 호출하면 되겠다.
		//swap?
	}
}