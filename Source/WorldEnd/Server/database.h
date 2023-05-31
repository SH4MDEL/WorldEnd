#pragma once
#include "stdafx.h"
#include "protocol.h"
#include <sqlext.h>

struct USER_INFO
{
	std::wstring user_id;
	std::wstring password;
	std::wstring name;
};

struct PLAYER_DATA
{
	std::wstring user_id;
	std::wstring name;

	int gold;
	char player_type;
	float x, y, z;

	int hp_level;
	int atk_level;
	int def_level;
	int crit_rate_level;
	int crit_damage_level;

	std::wstring normal_skill_name;
	std::wstring ultimate_name;
};

struct PLAYER_TABLE
{
	SQLWCHAR user_id[20];
	SQLWCHAR name[20];
	
	SQLINTEGER gold;
	SQLINTEGER player_type;
	SQLFLOAT x, y, z;

	SQLINTEGER hp_level;
	SQLINTEGER atk_level;
	SQLINTEGER def_level;
	SQLINTEGER crit_rate_level;
	SQLINTEGER crit_damage_level;

	SQLWCHAR normal_skill_name;
	SQLWCHAR ultimate_name;
	SQLCHAR logged_in;


	SQLLEN cb_user_id{}, cb_name{}, cb_gold{}, cb_player_type{}, cb_x{}, cb_y{}, cb_z{};
	SQLLEN cb_hp_level{}, cb_atk_level{}, cb_def_level{}, cb_crit_rate_level{}, cb_crit_damage_level{};
	SQLLEN cb_skill{}, cb_ultimate{}, cb_logged_in{};
};

class DataBase
{
public:
	DataBase();
	~DataBase();

	bool TryLogin(USER_INFO& user_info, PLAYER_DATA& player_data);

private:
	SQLHENV m_henv;
	SQLHDBC m_hdbc;
	SQLHSTMT m_hstmt;
};

