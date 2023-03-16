#pragma once
#include "stdafx.h"
#include "object.h"
#include "client.h"
#include "monster.h"

enum class GameRoomState { EMPTY, ACCEPT, INGAME };

class GameRoom
{
public:
	GameRoom();
	~GameRoom() = default;

	void Update(FLOAT elapsed_time);

	void SetType(EnvironmentType type) { m_type = type; }
	void SetEvent(COLLISION_EVENT event);
	void SetPlayer(INT player_id);
	void SetState(GameRoomState state) { m_state = state; }

	EnvironmentType GetType() const { return m_type; }
	GameRoomState GetState() const { return m_state; }
	mutex& GetStateMutex() { return m_state_lock; }

	void SendMonsterData();
	void SendAddMonster(INT player_id);

	bool FindPlayer(INT player_id);
	void DeletePlayer(INT player_id);

	void InitGameRoom();
	void InitMonsters();
	void InitEnvironment();

	array<INT, MAX_INGAME_USER>& GetPlayerIds() { return m_player_ids; }
	array<INT, MAX_INGAME_MONSTER>& GetMonsters() { return m_monster_ids; }

private:
	array<INT, MAX_INGAME_USER>				m_player_ids;
	array<INT, MAX_INGAME_MONSTER>			m_monster_ids;
	list<COLLISION_EVENT>					m_collision_events;

	EnvironmentType							m_type;
	BYTE									m_floor;
	GameRoomState							m_state;
	mutex									m_state_lock;
};

class Town
{
public:
	Town() = default;
	~Town() = default;

private:
	vector<shared_ptr<GameObject>>	m_structures;
	array<Client, MAX_USER>		m_players;
};

class GameRoomManager
{
public:
	GameRoomManager();
	~GameRoomManager() = default;

	void Update(float elapsed_time);

	void SetEvent(COLLISION_EVENT event, INT player_id);
	void SetPlayer(INT room_num, INT player_id);

	vector<shared_ptr<GameObject>>& GetStructures() { return m_structures; }
	array<shared_ptr<GameRoom>, MAX_GAME_ROOM_NUM>& GetDungeons() { return m_game_rooms; }

	void InitGameRoom(INT room_num);
	void LoadMap();

	bool EnterGameRoom(const shared_ptr<Party>& party);
	void DeletePlayer(INT room_num, INT player_id);

	void SendMonsterData();
	void SendAddMonster(INT player_id);

private:
	array<shared_ptr<GameRoom>, MAX_GAME_ROOM_NUM>	m_game_rooms;
	vector<shared_ptr<GameObject>>					m_structures;

	INT FindEmptyRoom();
	INT FindGameRoomInPlayer(INT player_id);
};