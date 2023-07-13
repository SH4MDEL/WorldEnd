#pragma once
#include "stdafx.h"
#include "protocol.h"
#include <sqlext.h>

struct USER_INFO
{
	std::wstring user_id;
	std::wstring password;
};

struct PLAYER_INFO
{
	std::wstring user_id;
	INT gold;
	INT player_type;
	FLOAT x, y, z;
};

struct SKILL_INFO
{
	std::wstring user_id;
	INT player_type;
	std::wstring normal_skill;
	std::wstring ultimate;
};

struct UPGRADE_INFO
{
	std::wstring user_id;
	BYTE hp, def, atk, crit_rate, crit_damage;
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
	SQLWCHAR name[30];
	
	SQLINTEGER gold;
	SQLINTEGER player_type;
	SQLFLOAT x, y, z;

	SQLINTEGER hp_level;
	SQLINTEGER atk_level;
	SQLINTEGER def_level;
	SQLINTEGER crit_rate_level;
	SQLINTEGER crit_damage_level;

	SQLWCHAR normal_skill_name[30];
	SQLWCHAR ultimate_name[30];
	SQLCHAR logged_in;


	SQLLEN cb_user_id{}, cb_name{}, cb_gold{}, cb_player_type{}, cb_x{}, cb_y{}, cb_z{};
	SQLLEN cb_hp_level{}, cb_atk_level{}, cb_def_level{}, cb_crit_rate_level{}, cb_crit_damage_level{};
	SQLLEN cb_skill{}, cb_ultimate{}, cb_logged_in{};
};

struct SKILL_TABLE 
{
	SQLWCHAR user_id[20];
	SQLINTEGER player_type;
	SQLWCHAR normal_skill_name[30];
	SQLWCHAR ultimate_name[30];

	SQLLEN cb_user_id{}, cb_player_type{};
	SQLLEN cb_skill{}, cb_ultimate{};
};

class DataBase
{
public:
	DataBase();
	~DataBase();

	bool TryLogin(const USER_INFO& user_info, PLAYER_DATA& player_data);
	bool Logout(const std::wstring_view& id);
	bool CreateAccount(const USER_INFO& user_info);
	bool DeleteAccount(const USER_INFO& user_info);
	bool UpdatePlayer(const PLAYER_INFO& player_info);
	bool UpdateSkill(const SKILL_INFO& skill_info);
	bool UpdateUpgrade(const UPGRADE_INFO& upgrade_info);
	bool UpdateGold(std::wstring user_id, int gold);
	bool UpdatePosition(std::wstring user_id, float x, float y, float z);
	bool GetSkillData(std::wstring user_id);

private:
	SQLHENV m_henv;
	SQLHDBC m_hdbc;
	SQLHSTMT m_hstmt;
};

