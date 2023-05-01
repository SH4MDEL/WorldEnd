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

enum class State { FREE, ACCEPT, INGAME, DEATH };

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
	virtual State GetState() const { return State::FREE; }

	virtual void SendEvent(INT id, void* c) {}
	virtual void SendEvent(const std::span<INT>& ids, void* c) {}

protected:
	XMFLOAT3				m_position;
	FLOAT					m_yaw;
	BoundingOrientedBox		m_bounding_box;
	INT						m_id;
};

// ----------------- 이벤트 오브젝트 ------------------
//  - NPC 로 사용하는 이벤트 오브젝트(게시판, 포탈, 전투시작 오브젝트, 강화NPC)
//  - Trigger 로 사용하는 이벤트 오브젝트
class EventObject : public GameObject
{
public:
	EventObject() = default;
	virtual ~EventObject() = default;

	void SetEventBoundingBox(const BoundingOrientedBox& obb) { m_event_bounding_box = obb; }
	void SetValid(bool valid) { m_valid = valid; }

	BoundingOrientedBox GetEventBoundingBox() const { return m_event_bounding_box; }
	bool GetValid() const { return m_valid; }

	virtual void SendEvent(INT id, void* c) override {}
	virtual void SendEvent(const std::span<INT>& ids, void* c) override {}


protected:
	BoundingOrientedBox		m_event_bounding_box;
	bool					m_valid;
};

class RecordBoard : public EventObject
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

class Enhancment : public EventObject
{
public:
	Enhancment();
	virtual ~Enhancment() = default;

	virtual void SendEvent(INT player_id, void* c) override;
};

class BattleStarter : public EventObject
{
public:
	BattleStarter();
	virtual ~BattleStarter() = default;

	virtual void SendEvent(INT player_id, void* c) override;
	virtual void SendEvent(const std::span<INT>& ids, void* c) override;

private:
	std::mutex	m_valid_lock;
};

class WarpPortal : public EventObject
{
public:
	WarpPortal();
	virtual ~WarpPortal() = default;

	virtual void SendEvent(INT player_id, void* c) override;
	virtual void SendEvent(const std::span<INT>& ids, void* c) override;
};

//-------------------- 트리거 --------------------
class Trigger : public EventObject
{
public:
	Trigger();
	virtual ~Trigger() = default;

	void SetState(State state) { m_state = state; }

	std::chrono::milliseconds GetCooldown() const { return m_cooldown; }
	TriggerType GetType() const { return m_type; }
	std::mutex& GetStateMutex() { return m_state_lock; }
	State GetState() const { return m_state; }

	void SetCooldownEvent(INT id);
	void SetRemoveEvent(INT id);
	void Activate(INT id);
	void Create(FLOAT damage, INT id);
	void Create(FLOAT damage, INT id, const XMFLOAT3& position);

protected:
	virtual void ProcessTrigger(INT id) = 0;
	virtual bool IsValid(INT id) = 0;

protected:
	std::chrono::milliseconds  m_duration;
	std::chrono::milliseconds  m_cooldown;	// 트리거 연속 발생 쿨타임
	TriggerType				   m_type;
	FLOAT					   m_damage;
	INT						   m_created_id;

	std::mutex	m_state_lock;
	State		m_state;
};

// 화살비 트리거
class ArrowRain : public Trigger
{
public:
	ArrowRain();
	virtual ~ArrowRain() = default;

private:
	virtual void ProcessTrigger(INT id) override;
	virtual bool IsValid(INT id) override;
};

class UndeadGrasp : public Trigger
{
public:
	UndeadGrasp();
	virtual ~UndeadGrasp() = default;

private:
	virtual void ProcessTrigger(INT id) override;
	virtual bool IsValid(INT id) override;
};


// ---------------------- 동체 ----------------------
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
	void SetRoomNum(SHORT room_num) { m_room_num = room_num; }
	void SetTriggerFlag() { m_trigger_flag = 0; }
	void SetTriggerFlag(UCHAR trigger, bool val);

	XMFLOAT3 GetVelocity() const { return m_velocity; }
	std::mutex& GetStateMutex() { return m_state_lock; }
	virtual State GetState() const override { return m_state; }
	std::string GetName() const { return m_name; }
	FLOAT GetMaxHp() const { return m_max_hp; }
	FLOAT GetHp() const { return m_hp; }
	FLOAT GetDamage() const { return m_damage; }
	SHORT GetRoomNum() const { return m_room_num; }
	UCHAR GetTriggerFlag() const { return m_trigger_flag; }

	virtual void Update(FLOAT elapsed_time) {}
	virtual void DecreaseHp(FLOAT damage, INT id) {}
	XMFLOAT3 GetFront() const;
	bool CheckTriggerFlag(UCHAR trigger);

	virtual PLAYER_DATA GetPlayerData() const { return PLAYER_DATA(); }
	virtual PlayerType GetPlayerType() const { return PlayerType::COUNT; }
	virtual FLOAT GetSkillRatio(ActionType type) const { return 0.f; }
	virtual MONSTER_DATA GetMonsterData() const { return MONSTER_DATA(); }
	virtual MonsterType GetMonsterType() const { return MonsterType::COUNT; }
	virtual const SOCKET& GetSocket() const { return SOCKET(); }
	virtual USHORT GetCurrentAnimation() const { return 0; }
	virtual UINT GetLastMoveTime() const { return 0; }
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

	SHORT		m_room_num;
	UCHAR		m_trigger_flag;
};

