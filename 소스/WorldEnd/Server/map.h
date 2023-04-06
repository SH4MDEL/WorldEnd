#pragma once
#include "stdafx.h"
#include "object.h"
#include "client.h"
#include "monster.h"

enum class GameRoomState { EMPTY, ACCEPT, INGAME };

class GameRoom : public std::enable_shared_from_this<GameRoom>
{
public:
	GameRoom();
	~GameRoom() = default;

	void Update(FLOAT elapsed_time);
	void ArrowCollisionUpdate(FLOAT elapsed_time);

	void SetType(EnvironmentType type) { m_type = type; }
	void SetEvent(COLLISION_EVENT ev);
	void SetPlayer(INT player_id);
	void SetState(GameRoomState state) { m_state = state; }
	void SetMonstersRoomNum(INT room_num);

	EnvironmentType GetType() const { return m_type; }
	GameRoomState GetState() const { return m_state; }
	std::mutex& GetStateMutex() { return m_state_lock; }
	std::shared_ptr<GameRoom> GetGameRoom() { return shared_from_this(); }
	BYTE GetMonsterCount() const { return m_monster_count; }
	std::chrono::system_clock::time_point GetStartTime() const { return m_start_time; }

	void SendMonsterData();
	void SendAddMonster(INT player_id);

	bool FindPlayer(INT player_id);
	void RemovePlayer(INT player_id);
	void RemoveMonster(INT monster_id);
	void DecreaseMonsterCount(INT room_num, BYTE count);

	void InitGameRoom(INT room_num);
	void InitMonsters(INT room_num);
	void InitEnvironment();

	std::array<INT, MAX_INGAME_USER>& GetPlayerIds() { return m_player_ids; }
	std::array<INT, MAX_INGAME_MONSTER>& GetMonsterIds() { return m_monster_ids; }

private:
	std::array<INT, MAX_INGAME_USER>			m_player_ids;
	std::array<INT, MAX_INGAME_MONSTER>			m_monster_ids;
	std::list<COLLISION_EVENT>					m_collision_events;
	// thread-unsafe 하므로 변경 필요함
	std::chrono::system_clock::time_point		m_start_time;

	EnvironmentType			m_type;
	BYTE					m_monster_count;
	BYTE					m_floor;
	GameRoomState			m_state;
	std::mutex				m_state_lock;
};

class Town
{
public:
	Town() = default;
	~Town() = default;

private:
	std::vector<std::shared_ptr<GameObject>>	m_structures;
	std::array<Client, MAX_USER>				m_players;
};

class GameRoomManager
{
public:
	GameRoomManager();
	~GameRoomManager() = default;

	void Update(float elapsed_time);

	void SetEvent(COLLISION_EVENT ev, INT player_id);
	void SetPlayer(INT room_num, INT player_id);

	std::vector<std::shared_ptr<GameObject>>& GetStructures() { return m_structures; }
	std::array<std::shared_ptr<GameRoom>, MAX_GAME_ROOM_NUM>& GetGameRooms() { return m_game_rooms; }
	std::shared_ptr<GameRoom> GetGameRoom(INT room_num) { return m_game_rooms[room_num]->GetGameRoom(); }

	void InitGameRoom(INT room_num);
	void LoadMap();

	bool EnterGameRoom(const std::shared_ptr<Party>& party);
	void RemovePlayer(INT room_num, INT player_id);
	void RemoveMonster(INT room_num, INT monster_id);
	void DecreaseMonsterCount(INT room_num, BYTE count);

	void SendMonsterData();
	void SendAddMonster(INT player_id);

private:
	std::array<std::shared_ptr<GameRoom>, MAX_GAME_ROOM_NUM>	m_game_rooms;
	std::vector<std::shared_ptr<GameObject>>					m_structures;

	INT FindEmptyRoom();
	INT FindGameRoomInPlayer(INT player_id);
};