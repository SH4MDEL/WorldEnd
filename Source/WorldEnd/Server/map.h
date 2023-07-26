#pragma once
#include "stdafx.h"
#include "object.h"
#include "client.h"
#include "monster.h"

enum class GameRoomState { EMPTY, ACCEPT, INGAME, ONBATTLE, CLEAR, COUNT };

class GameRoom : public std::enable_shared_from_this<GameRoom>
{
public:
	GameRoom();
	~GameRoom() = default;

	void Update(FLOAT elapsed_time);
	void InteractObject(InteractionType type);
	void StartBattle();
	void WarpNextFloor(INT room_num);
	void DungeonClear();

	void SetType(EnvironmentType type) { m_type = type; }
	bool SetPlayer(INT room_num, INT player_id);
	void SetState(GameRoomState state) { m_state = state; }
	void SetMonsterCount(INT count) { m_monster_count = count; }

	EnvironmentType GetType() const { return m_type; }
	GameRoomState GetState() const { return m_state; }
	std::mutex& GetStateMutex() { return m_state_lock; }
	std::mutex& GetMonsterMutex() { return m_monster_lock; }
	std::mutex& GetPlayerMutex() { return m_player_lock; }
	std::shared_ptr<GameRoom> GetGameRoom() { return shared_from_this(); }
	BYTE GetMonsterCount() const { return m_monster_count; }
	std::chrono::system_clock::time_point GetStartTime() const { return m_start_time; }
	std::shared_ptr<BattleStarter> GetBattleStarter() const;
	std::shared_ptr<WarpPortal> GetWarpPortal() const;
	INT GetArrowId();

	void SendAddPlayer(INT sender, INT receiver);
	void SendPlayerData();
	void SendMonsterData();
	void SendAddMonster(INT player_id);
	void SendAddMonster();

	void AddTrigger(INT trigger_id);
	void RemoveTrigger(INT trigger_id);
	void RemovePlayer(INT player_id, INT room_num);
	void RemoveMonster(INT monster_id);
	void CheckEventCollision(INT player_id);
	void CheckTriggerCollision(INT id);

	INT GenerateRandomRoom(std::set<INT>& history, INT min, INT max);

	void Init();
	void InitGameRoom(INT room_num);
	void InitMonsters(INT room_num);
	void InitEnvironment();

	std::array<INT, MAX_INGAME_USER>& GetPlayerIds() { return m_ingame_player_ids; }
	std::array<INT, MAX_INGAME_MONSTER>& GetMonsterIds() { return m_monster_ids; }

private:
	void CollideWithEventObject(INT player_id, InteractionType type);
	INT GetPlayerCount();

private:
	std::array<INT, MAX_INGAME_USER>			m_ingame_player_ids;
	std::array<INT, MAX_INGAME_MONSTER>			m_monster_ids;
	std::unordered_set<INT>						m_trigger_list;
	std::mutex									m_player_lock;
	std::mutex									m_monster_lock;
	std::mutex									m_trigger_lock;
	std::chrono::system_clock::time_point		m_start_time;
	std::set<INT>                               m_save_room;        // 나온 방은 중복되서 안불리게 set에 저장해서 구별

	std::shared_ptr<WarpPortal>		m_portal;
	std::shared_ptr<BattleStarter>	m_battle_starter;

	EnvironmentType			m_type;
	std::atomic<BYTE>		m_monster_count;
	BYTE					m_floor;
	GameRoomState			m_state;
	std::mutex				m_state_lock;
	INT						m_arrow_id;
	std::mutex				m_arrow_lock;
	//INT                     m_floor_cnt = 4;                       // 보스 방 치트
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

	bool FillPlayer(INT player_id);
	bool SetPlayer(INT room_num, INT player_id);
	bool IsValidRoomNum(INT room_num);

	std::vector<std::shared_ptr<GameObject>>& GetStructures() { return m_structures; }
	std::vector<std::shared_ptr<GameObject>>& GetInvisibleWalls() { return m_invisible_walls; }
	std::shared_ptr<GameRoom> GetGameRoom(INT room_num);

	void InitGameRoom(INT room_num);
	void LoadMap();
	void InteractObject(INT room_num, InteractionType type);
	void StartBattle(INT room_num);
	void WarpNextFloor(INT room_num);
	void DungeonClear(INT room_num);
		
	bool EnterGameRoom(const std::shared_ptr<Party>& party);
	void RemovePlayer(INT player_id);
	void RemoveMonster(INT monster_id);
	void RemoveTrigger(INT trigger_id, INT room_num);
	void EventCollisionCheck(INT room_num, INT player_id);

	void SendPlayerData();
	void SendMonsterData();
	void SendAddMonster(INT room_num, INT player_id);
	void SendAddMonster(INT room_num);

private:
	INT FindEmptyRoom();
	INT FindCanAccessRoom();

private:
	std::array<std::shared_ptr<GameRoom>, MAX_GAME_ROOM_NUM>	m_game_rooms;
	std::vector<std::shared_ptr<GameObject>>					m_structures;
	std::vector<std::shared_ptr<GameObject>>					m_invisible_walls;
};
