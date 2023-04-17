#pragma once
#include "stdafx.h"
#include "object.h"
#include "client.h"
#include "monster.h"

enum class GameRoomState { EMPTY, ACCEPT, INGAME, CLEAR, COUNT };

class GameRoom : public std::enable_shared_from_this<GameRoom>
{
public:
	GameRoom();
	~GameRoom() = default;

	void Update(FLOAT elapsed_time);
	void StartBattle();
	void WarpNextFloor(INT room_num);

	void SetType(EnvironmentType type) { m_type = type; }
	void SetPlayer(INT player_id);
	void SetState(GameRoomState state) { m_state = state; }

	EnvironmentType GetType() const { return m_type; }
	GameRoomState GetState() const { return m_state; }
	std::mutex& GetStateMutex() { return m_state_lock; }
	std::mutex& GetMonsterMutex() { return m_monster_lock; }
	std::mutex& GetPlayerMutex() { return m_player_lock; }
	std::shared_ptr<GameRoom> GetGameRoom() { return shared_from_this(); }
	BYTE GetMonsterCount() const { return m_monster_count; }
	std::chrono::system_clock::time_point GetStartTime() const { return m_start_time; }
	std::shared_ptr<BattleStarter> GetBattleStarter() const;
	INT GetArrowId();

	void SendMonsterData();
	void SendAddMonster(INT player_id);

	void RemovePlayer(INT player_id, INT room_num);
	void RemoveMonster(INT monster_id);
	void EventCollisionCheck(INT player_id);

	void InitGameRoom(INT room_num);
	void InitMonsters(INT room_num);
	void InitEnvironment();

	std::array<INT, MAX_INGAME_USER>& GetPlayerIds() { return m_player_ids; }
	std::array<INT, MAX_INGAME_MONSTER>& GetMonsterIds() { return m_monster_ids; }

private:
	std::array<INT, MAX_INGAME_USER>			m_player_ids;
	std::array<INT, MAX_INGAME_MONSTER>			m_monster_ids;
	std::chrono::system_clock::time_point		m_start_time;
	std::mutex									m_player_lock;
	std::mutex									m_monster_lock;

	std::shared_ptr<WarpPortal>		m_portal;
	std::shared_ptr<BattleStarter>	m_battle_starter;

	EnvironmentType			m_type;
	std::atomic<BYTE>		m_monster_count;
	BYTE					m_floor;
	GameRoomState			m_state;
	std::mutex				m_state_lock;
	INT						m_arrow_id;
	std::mutex				m_arrow_lock;

	void CollideWithEventObject(INT player_id, InteractableType type);
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

	void SetPlayer(INT room_num, INT player_id);
	bool IsValidRoomNum(INT room_num);

	std::vector<std::shared_ptr<GameObject>>& GetStructures() { return m_structures; }
	std::shared_ptr<GameRoom> GetGameRoom(INT room_num);
	/*std::chrono::system_clock::time_point GetStartTime(INT room_num);
	EnvironmentType GetEnvironment(INT room_num);
	GameRoomState GetRoomState(INT room_num);
	INT GetArrowId(INT room_num);*/


	void InitGameRoom(INT room_num);
	void LoadMap();
	void StartBattle(INT room_num);
	void WarpNextFloor(INT room_num);

	bool EnterGameRoom(const std::shared_ptr<Party>& party);
	void RemovePlayer(INT player_id);
	void RemoveMonster(INT monster_id);
	void EventCollisionCheck(INT room_num, INT player_id);

	void SendMonsterData();
	void SendAddMonster(INT room_num, INT player_id);

private:
	std::array<std::shared_ptr<GameRoom>, MAX_GAME_ROOM_NUM>	m_game_rooms;
	std::vector<std::shared_ptr<GameObject>>					m_structures;

	INT FindEmptyRoom();
};