#include "database.h"

void HandleDiagnosticRecord(SQLHANDLE handle, SQLSMALLINT type, RETCODE ret)
{
	SQLSMALLINT iRec = 0;
	SQLINTEGER iError;
	WCHAR wszMessage[1000];
	WCHAR wszState[SQL_SQLSTATE_SIZE + 1];
	if (ret == SQL_INVALID_HANDLE) {
		fwprintf(stderr, L"Invalid handle!\n");
		return;
	}
	while (SQLGetDiagRec(type, handle, ++iRec, wszState, &iError, wszMessage,
		(SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT*)NULL) == SQL_SUCCESS) {
		// Hide data truncated..
		if (wcsncmp(wszState, L"01004", 5)) {
			fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
		}
	}
}


DataBase::DataBase() : m_hdbc{}, m_henv{}, m_hstmt{}
{
	setlocale(LC_ALL, "korean");
	std::wcout.imbue(std::locale("korean"));


	SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_henv);
	if (!(SQL_SUCCESS == ret || SQL_SUCCESS_WITH_INFO == ret)) {
		std::cout << "ENV Handle �Ҵ� ����" << std::endl;
		return;
	}

	ret = SQLSetEnvAttr(m_henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);
	if (!(SQL_SUCCESS == ret || SQL_SUCCESS_WITH_INFO == ret)) {
		std::cout << "ODBC ���� ���� ����" << std::endl;
		return;
	}
	
	ret = SQLAllocHandle(SQL_HANDLE_DBC, m_henv, &m_hdbc);
	if (!(SQL_SUCCESS == ret || SQL_SUCCESS_WITH_INFO == ret)) {
		std::cout << "DBC Handle �Ҵ� ����" << std::endl;
		return;
	}

	// ���� �Ӽ� ����, LOGIN Timeout �� 5�ʷ� ����
	SQLSetConnectAttr(m_hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

	ret = SQLConnect(m_hdbc, (SQLWCHAR*) L"WorldEnd", SQL_NTS, nullptr, 0, nullptr, 0);
	if (!(SQL_SUCCESS == ret || SQL_SUCCESS_WITH_INFO == ret)) {
		std::cout << "ODBC ���� ����" << std::endl;
		return;
	}


	std::cout << "DB ���� ����!" << std::endl;
}

DataBase::~DataBase()
{
	SQLDisconnect(m_hdbc);
	SQLFreeHandle(SQL_HANDLE_ENV, m_hdbc);
	SQLFreeHandle(SQL_HANDLE_ENV, m_henv);
}

bool DataBase::TryLogin(const USER_INFO& user_info, PLAYER_DATA& player_data)
{
	// ���� �ڵ� �Ҵ�
	SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
	if (!(SQL_SUCCESS == ret || SQL_SUCCESS_WITH_INFO == ret)) {
		std::cout << "Statement Handle �Ҵ� ����" << std::endl;
		return false;
	}

	std::wstring query = std::format(L"EXEC [WorldEnd].[dbo].[try_login] '{0}', '{1}'",
		user_info.user_id, user_info.password);

	ret = SQLExecDirect(m_hstmt, (SQLWCHAR*)query.c_str(), SQL_NTS);
	if (!(SQL_SUCCESS == ret || SQL_SUCCESS_WITH_INFO == ret)) {
		HandleDiagnosticRecord(m_hstmt, SQL_HANDLE_STMT, ret);
		SQLCancel(m_hstmt);
		SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
		return false;
	}


	PLAYER_TABLE player_table{};

	// ���̺� �о bind
	ret = SQLBindCol(m_hstmt, 1, SQL_C_WCHAR, &player_table.user_id, 20, &player_table.cb_user_id);
	ret = SQLBindCol(m_hstmt, 2, SQL_C_WCHAR, &player_table.name, 30, &player_table.cb_name);
	ret = SQLBindCol(m_hstmt, 3, SQL_C_SLONG, &player_table.gold, 4, &player_table.cb_gold);
	ret = SQLBindCol(m_hstmt, 4, SQL_C_TINYINT, &player_table.player_type, 1, &player_table.cb_player_type);
	ret = SQLBindCol(m_hstmt, 5, SQL_C_FLOAT, &player_table.x, 8, &player_table.cb_x);
	ret = SQLBindCol(m_hstmt, 6, SQL_FLOAT, &player_table.y, 8, &player_table.cb_y);
	ret = SQLBindCol(m_hstmt, 7, SQL_FLOAT, &player_table.z, 8, &player_table.cb_z);
	ret = SQLBindCol(m_hstmt, 8, SQL_C_TINYINT, &player_table.hp_level, 1, &player_table.cb_hp_level);
	ret = SQLBindCol(m_hstmt, 9, SQL_C_TINYINT, &player_table.atk_level, 1, &player_table.cb_atk_level);
	ret = SQLBindCol(m_hstmt, 10, SQL_C_TINYINT, &player_table.def_level, 1, &player_table.cb_def_level);
	ret = SQLBindCol(m_hstmt, 11, SQL_C_TINYINT, &player_table.crit_rate_level, 1, &player_table.cb_crit_rate_level);
	ret = SQLBindCol(m_hstmt, 12, SQL_C_TINYINT, &player_table.crit_damage_level, 1, &player_table.cb_crit_damage_level);
	ret = SQLBindCol(m_hstmt, 13, SQL_C_WCHAR, &player_table.normal_skill_name, 30, &player_table.cb_skill);
	ret = SQLBindCol(m_hstmt, 14, SQL_C_WCHAR, &player_table.ultimate_name, 30, &player_table.cb_ultimate);
	ret = SQLBindCol(m_hstmt, 15, SQL_C_BIT, &player_table.logged_in, 1, &player_table.cb_logged_in);

	while (true) {
		ret = SQLFetch(m_hstmt);

		if (SQL_SUCCESS == ret || SQL_SUCCESS_WITH_INFO == ret) {

			player_data.user_id = player_table.user_id;
			player_data.name = player_table.name;

			player_data.gold = player_table.gold;
			player_data.player_type = player_table.player_type;
			player_data.x = player_table.x;
			player_data.y = player_table.y; 
			player_data.y = player_table.y;

			player_data.hp_level = player_table.hp_level;
			player_data.def_level = player_table.def_level;
			player_data.atk_level = player_table.atk_level;
			player_data.crit_rate_level = player_table.crit_rate_level;
			player_data.crit_damage_level = player_table.crit_damage_level;

			player_data.normal_skill_name = player_table.normal_skill_name;
			player_data.ultimate_name = player_table.ultimate_name;

			std::wcout << player_data.user_id << ", " << player_data.name << std::endl;
			std::wcout << player_data.normal_skill_name << ", " << player_data.ultimate_name << std::endl;
			std::cout << "�÷��̾� Ÿ�� : " << (int)player_data.player_type << std::endl;


			// id, password ����ġ �Ǵ� db �� ���� ����
			std::wstring temp_id {L"null"};
			if (temp_id == player_data.user_id) {
				SQLCancel(m_hstmt);
				SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
				std::cout << "id / password �� ��ġ���� �ʽ��ϴ�." << std::endl;
				return false;
			}

			// �̹� ���� ������
			int logged_in = player_table.logged_in;

			if (logged_in) {
				SQLCancel(m_hstmt);
				SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
				std::cout << "�̹� �������� id �Դϴ�" << std::endl;
				return false;
			}

			return true;
		}
		else {
			break;
		}
	}

	std::cout << "FAIL " << std::endl;

	SQLCancel(m_hstmt);
	SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
	
	return false;
}

bool DataBase::CreateAccount(const USER_INFO& user_info)
{
	// ���� �ڵ� �Ҵ�
	SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
	if (!(SQL_SUCCESS == ret || SQL_SUCCESS_WITH_INFO == ret)) {
		std::cout << "Statement Handle �Ҵ� ����" << std::endl;
		return false;
	}

	std::wstring query = std::format(L"EXEC [WorldEnd].[dbo].[create_account] '{0}', '{1}', '{2}'",
		user_info.user_id, user_info.password, user_info.name);

	ret = SQLExecDirect(m_hstmt, (SQLWCHAR*)query.c_str(), SQL_NTS);
	if (!(SQL_SUCCESS == ret || SQL_SUCCESS_WITH_INFO == ret)) {
		HandleDiagnosticRecord(m_hstmt, SQL_HANDLE_STMT, ret);
		SQLCancel(m_hstmt);
		SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
		return false;
	}

	SQLCancel(m_hstmt);
	SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
	return true;
}

bool DataBase::DeleteAccount(const USER_INFO& user_info)
{
	SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
	if (!(SQL_SUCCESS == ret || SQL_SUCCESS_WITH_INFO == ret)) {
		std::cout << "Statement Handle �Ҵ� ����" << std::endl;
		return false;
	}

	std::wstring query = std::format(L"EXEC [WorldEnd].[dbo].[create_account] '{0}', '{1}'",
		user_info.user_id, user_info.password);

	ret = SQLExecDirect(m_hstmt, (SQLWCHAR*)query.c_str(), SQL_NTS);
	if (!(SQL_SUCCESS == ret || SQL_SUCCESS_WITH_INFO == ret)) {
		HandleDiagnosticRecord(m_hstmt, SQL_HANDLE_STMT, ret);
		SQLCancel(m_hstmt);
		SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
		return false;
	}

	SQLCancel(m_hstmt);
	SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
	return true;
}

bool DataBase::UpdatePlayer(const PLAYER_INFO& player_info)
{
	SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
	if (!(SQL_SUCCESS == ret || SQL_SUCCESS_WITH_INFO == ret)) {
		std::cout << "Statement Handle �Ҵ� ����" << std::endl;
		return false;
	}

	std::wstring query = std::format(L"EXEC [WorldEnd].[dbo].[update_player_info] '{0}', {1},{2},{3},{4},{5}",
		player_info.user_id, player_info.gold, player_info.player_type,
		player_info.x, player_info.y, player_info.z);

	ret = SQLExecDirect(m_hstmt, (SQLWCHAR*)query.c_str(), SQL_NTS);
	if (!(SQL_SUCCESS == ret || SQL_SUCCESS_WITH_INFO == ret)) {
		HandleDiagnosticRecord(m_hstmt, SQL_HANDLE_STMT, ret);
		SQLCancel(m_hstmt);
		SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
		return false;
	}

	SQLCancel(m_hstmt);
	SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
	return true;
}

bool DataBase::UpdateSkill(const SKILL_INFO& skill_info)
{
	SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
	if (!(SQL_SUCCESS == ret || SQL_SUCCESS_WITH_INFO == ret)) {
		std::cout << "Statement Handle �Ҵ� ����" << std::endl;
		return false;
	}

	std::wstring query = 
		std::format(L"EXEC [WorldEnd].[dbo].[update_skill_info] '{0}', {1}, '{2}', '{3}'",
			skill_info.user_id, skill_info.player_type,
			skill_info.normal_skill, skill_info.ultimate);

	ret = SQLExecDirect(m_hstmt, (SQLWCHAR*)query.c_str(), SQL_NTS);
	if (!(SQL_SUCCESS == ret || SQL_SUCCESS_WITH_INFO == ret)) {
		HandleDiagnosticRecord(m_hstmt, SQL_HANDLE_STMT, ret);
		SQLCancel(m_hstmt);
		SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
		return false;
	}

	SQLCancel(m_hstmt);
	SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
	return true;
}

bool DataBase::UpdateUpgrade(const UPGRADE_INFO& upgrade_info)
{
	SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
	if (!(SQL_SUCCESS == ret || SQL_SUCCESS_WITH_INFO == ret)) {
		std::cout << "Statement Handle �Ҵ� ����" << std::endl;
		return false;
	}

	std::wstring query =
		std::format(L"EXEC [WorldEnd].[dbo].[update_upgrade_info] '{0}', {1},{2},{3},{4},{5}",
			upgrade_info.user_id, upgrade_info.hp, upgrade_info.def,
			upgrade_info.atk, upgrade_info.crit_rate, upgrade_info.crit_damage);

	ret = SQLExecDirect(m_hstmt, (SQLWCHAR*)query.c_str(), SQL_NTS);
	if (!(SQL_SUCCESS == ret || SQL_SUCCESS_WITH_INFO == ret)) {
		HandleDiagnosticRecord(m_hstmt, SQL_HANDLE_STMT, ret);
		SQLCancel(m_hstmt);
		SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
		return false;
	}

	SQLCancel(m_hstmt);
	SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
	return true;
}

