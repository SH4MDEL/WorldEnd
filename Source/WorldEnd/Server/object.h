#pragma once
#include "stdafx.h"

struct SCORE_DATA
{
	std::string names[MAX_INGAME_USER];
	USHORT score;

	constexpr bool operator <(const SCORE_DATA& left)const
	{
		return (score < left.score);
	}
};

enum class State { ST_FREE, ST_ACCEPT, ST_INGAME, ST_DEAD };

class GameObject
{
public:
	GameObject();
	virtual ~GameObject() = default;

	void SetId(INT id) { m_id = id; }
	void SetPosition(const XMFLOAT3& pos);
	void SetPosition(FLOAT x, FLOAT y, FLOAT z);
	void SetYaw(const FLOAT& yaw) { m_yaw = yaw; }
	void SetBoundingBox(const BoundingOrientedBox& obb) { m_bounding_box = obb; }
	void SetBoundingBoxCenter(const XMFLOAT3& center) { m_bounding_box.Center = center; }
	void SetBoundingBoxOrientation(const XMFLOAT4& orientaion) { m_bounding_box.Orientation = orientaion; }

	XMFLOAT3 GetPosition() const { return m_position; }
	FLOAT GetYaw() const { return m_yaw; }
	const BoundingOrientedBox& GetBoundingBox() const { return m_bounding_box; }
	BoundingOrientedBox& GetBoundingBox() { return m_bounding_box; }
	INT GetId() const { return m_id; }
	virtual State GetState() const { return State::ST_FREE; }

	virtual void SendEvent(INT id, void* c) {}
	virtual void SendEvent(const std::span<INT>& ids, void* c) {}

protected:
	XMFLOAT3				m_position;
	FLOAT					m_yaw;
	BoundingOrientedBox		m_bounding_box;
	INT						m_id;
};

class Npc : public GameObject
{
public:
	Npc() = default;
	virtual ~Npc() = default;

	void SetEventBoundingBox(const BoundingOrientedBox& obb) { m_event_bounding_box = obb; }

	BoundingOrientedBox GetEventBoundingBox() const { return m_event_bounding_box; }

	virtual void SendEvent(INT id, void* c) override {}
	virtual void SendEvent(const std::span<INT>& ids, void* c) override {}

protected:
	BoundingOrientedBox m_event_bounding_box;
};

class RecordBoard : public Npc
{
public:
	RecordBoard();
	virtual ~RecordBoard() = default;

	virtual void SendEvent(INT player_id, void* c) override;

	void SetRecord(const SCORE_DATA& record, INT player_num);

	std::array<SCORE_DATA, MAX_RECORD_NUM>& GetRecord(INT player_num);

private:
	std::array<SCORE_DATA, MAX_RECORD_NUM> m_trio_squad_records;
	std::array<SCORE_DATA, MAX_RECORD_NUM> m_duo_squad_records;
	std::array<SCORE_DATA, MAX_RECORD_NUM> m_solo_squad_records;
};

class Enhancment : public Npc
{
public:
	Enhancment();
	virtual ~Enhancment() = default;

	virtual void SendEvent(INT player_id, void* c) override;
};

class BattleStarter : public Npc
{
public:
	BattleStarter();
	virtual ~BattleStarter() = default;

	void SetIsValid(bool is_valid);

	virtual void SendEvent(INT player_id, void* c) override;
	virtual void SendEvent(const std::span<INT>& ids, void* c) override;

private:
	bool		m_is_valid;
	std::mutex	m_valid_lock;
};

class WarpPortal : public Npc
{
public:
	WarpPortal();
	virtual ~WarpPortal() = default;

	virtual void SendEvent(INT player_id, void* c) override;
	virtual void SendEvent(const std::span<INT>& ids, void* c) override;

	bool GetIsValid() const { return m_is_valid; }
	void SetIsValid(bool is_valid) { m_is_valid = is_valid; }

private:
	bool		m_is_valid;
};

class MovementObject : public GameObject
{
public:
	MovementObject();
	virtual ~MovementObject() = default;

	void SetVelocity(const XMFLOAT3& velocity) { m_velocity = velocity; }
	void SetVelocity(FLOAT x, FLOAT y, FLOAT z);
	void SetStateLock() { m_state_lock.lock(); }
	void SetStateUnLock() { m_state_lock.unlock(); }
	void SetState(State state) { m_state = state; }
	void SetName(std::string name) { m_name = name; }
	void SetName(const char* c);
	void SetMaxHp(FLOAT hp) { m_max_hp = hp; }
	void SetHp(FLOAT hp) { m_hp = hp; }
	void SetDamage(FLOAT damage) { m_damage = damage; }
	void SetRoomNum(USHORT room_num) { m_room_num = room_num; }

	XMFLOAT3 GetVelocity() const { return m_velocity; }
	std::mutex& GetStateMutex() { return m_state_lock; }
	virtual State GetState() const override { return m_state; }
	std::string GetName() const { return m_name; }
	FLOAT GetMaxHp() const { return m_max_hp; }
	FLOAT GetHp() const { return m_hp; }
	FLOAT GetDamage() const { return m_damage; }
	USHORT GetRoomNum() const { return m_room_num; }


	virtual void Update(FLOAT elapsed_time) {};

	virtual PLAYER_DATA GetPlayerData() const { return PLAYER_DATA(); }
	virtual PlayerType GetPlayerType() const { return PlayerType::UNKNOWN; }
	virtual FLOAT GetSkillRatio(AttackType type) const { return 0.f; }
	virtual MONSTER_DATA GetMonsterData() const { return MONSTER_DATA(); }
	virtual MonsterType GetMonsterType() const { return MonsterType::WARRIOR; }
	virtual const SOCKET& GetSocket() const { return SOCKET(); }
	virtual void DoSend(void* p) {}
	virtual void DoSend(void* p, INT packet_count) {}

protected:
	XMFLOAT3	m_velocity;

	std::mutex	m_state_lock;
	State		m_state;

	std::string	m_name;
	FLOAT		m_max_hp;
	FLOAT		m_hp;
	FLOAT		m_damage;

	USHORT		m_room_num;
};

