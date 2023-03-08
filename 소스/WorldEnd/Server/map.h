#pragma once
#include "stdafx.h"
#include "object.h"
#include "session.h"

class Dungeon
{
public:
	Dungeon() = default;
	~Dungeon() = default;

	vector<shared_ptr<GameObject>>& GetStructures() { return m_structures; }
	
	void LoadMap();

	EnvironmentType GetType() const { return m_type; }

	void SetType(EnvironmentType type) { m_type = type; }

private:
	static vector<shared_ptr<GameObject>>	m_structures;
	array<Session, MAX_DUNGEON_USER>		m_players;
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

