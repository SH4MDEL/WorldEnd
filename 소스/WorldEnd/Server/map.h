#pragma once
#include "stdafx.h"
#include "object.h"
#include "session.h"
#include "Server.h"
#include "monster.h"


class Dungeon
{
public:
	Dungeon() = default;
	~Dungeon() = default;

	vector<shared_ptr<GameObject>>& GetStructures() { return m_structures; }
	
	void LoadMap();

	EnvironmentType GetType() const { return m_type; }

	void SetType(EnvironmentType type) { m_type = type; }
	void SetEvent(CollisionEvent event);
	void SetPlayer(CHAR player_id, Session* player);

	void Update(float take_time);

	void SendMonsterData();
	void SendMonsterAdd(CHAR player_id);

	BOOL FindPlayer(CHAR player_id);

	void InitDungeon();

	void InitMonsters();
	void InitEnvironment();

	unordered_map<int, Session*>& GetPlayers() { return m_players; }
	vector<shared_ptr<Monster>>& GetMonsters() { return m_monsters; }

private:
	static vector<shared_ptr<GameObject>>	m_structures;
	unordered_map<int, Session*>			m_players;
	list<CollisionEvent>					m_collision_events;
	vector<shared_ptr<Monster>>				m_monsters;
	EnvironmentType							m_type;
};

class Town
{
public:
	Town() = default;
	~Town() = default;

private:
	static vector<shared_ptr<GameObject>>	m_structures;
	array<Session, MAX_USER>				m_players;
};

class DungeonManager
{
public:
	DungeonManager();
	~DungeonManager() = default;

	int FindDungeonInPlayer(CHAR player_id);
	void SetEvent(CollisionEvent event, CHAR player_id);

	void SetPlayer(int index, CHAR player_id, Session* Player);

	vector<shared_ptr<GameObject>>& GetStructures() { return m_dungeons[0]->GetStructures(); }
	vector<shared_ptr<Dungeon>>& GetDungeons() { return m_dungeons; }

	void InitDungeon(int index);

	void Update(float take_time);

	void SendMonsterData();
	void SendMonsterAdd(CHAR player_id);

private:
	vector<shared_ptr<Dungeon>>		m_dungeons;

};