#pragma once
#include <functional>
#include "client.h"
#include "monster.h"
#include "object.h"
#include "map.h"


enum class EventType : char 
{
	COOLDOWN_RESET, BEHAVIOR_CHANGE, AGRO_LEVEL_DECREASE,
	ATTACK_COLLISION, MONSTER_ATTACK_COLLISION, STAMINA_CHANGE,
	HIT_SCAN, ARROW_SHOOT, ARROW_REMOVE, GAME_ROOM_RESET,
	TRIGGER_COOLDOWN, TRIGGER_REMOVE, MULTIPLE_TRIGGER_SET, BATTLE_START,
	
};

struct TIMER_EVENT {
	std::chrono::system_clock::time_point event_time;
	INT obj_id;
	INT target_id;
	XMFLOAT3 position;
	XMFLOAT3 direction;
	EventType event_type;
	TriggerType trigger_type;
	ActionType action_type;
	MonsterBehavior next_behavior_type;
	BYTE latest_id;
	BYTE aggro_level;
	bool is_stamina_increase;
	bool is_valid;

	constexpr bool operator <(const TIMER_EVENT& left)const
	{
		return (event_time > left.event_time);
	}
};

class Server
{
public:
	static Server& GetInstance();

	Server(const Server& server) = delete;
	Server& operator=(const Server& server) = delete;

	~Server();

	void Network();
	void WorkerThread();
	void ProcessPacket(int id, char* p);
	void Disconnect(int id);

	void SendLoginOk(int client_id);
	void SendMoveInGameRoom(int client_id, int room_num);
	void SendPlayerDeath(int client_id);
	void SendChangeAnimation(int client_id, USHORT animation);
	void SendMonsterHit(int client_id, const std::span<int>& receiver,
		const std::span<int>& creater);
	void SendMonsterHit(int client_id, const std::span<int>& receiver,
		int hit_id);
	void SendMonsterAttack(int client_id, const std::span<int>& clients,
		const BoundingOrientedBox& obb);
	void SendMonsterAttack(int monster_id, int player_id);
	void SendArrowShoot(int client_id, int arrow_id);
	void SendRemoveArrow(int client_id, int arrow_id);
	void SendMonsterShoot(int client_id);
	void SendChangeHp(int client_id, FLOAT hp);
	void SendTrigger(int client_id, TriggerType type, const XMFLOAT3& pos);
	void SendMagicCircle(int room_num, const XMFLOAT3& pos, const XMFLOAT3& extent);

	bool IsPlayer(int client_id);
	void GameRoomObjectCollisionCheck(const std::shared_ptr<MovementObject>& object,
			int room_num);
	void ProcessArrow(int client_id, ActionType type);

	void Timer();
	void ProcessEvent(const TIMER_EVENT& ev);

	void SetTimerEvent(const TIMER_EVENT& ev);
	void SetAttackTimerEvent(int id, ActionType attack_type,
		std::chrono::system_clock::time_point attack_time);
	void SetCooldownTimerEvent(int id, ActionType action_type);
	void SetStaminaTimerEvent(int client_id, bool is_increase);
	void SetHitScanTimerEvent(int id, int target_id, ActionType action_type, int arrow_id);
	void SetArrowShootTimerEvent(int id, ActionType attack_type,
		std::chrono::system_clock::time_point attack_time);
	void SetRemoveArrowTimerEvent(int client_id, int arrow_id);
	void SetBattleStartTimerEvent(int client_id);
	void SetTrigger(int client_id, TriggerType type, const XMFLOAT3& pos);
	void SetTrigger(int client_id, TriggerType type, int target_id);

	INT GetNewId();
	INT GetNewMonsterId(MonsterType type);
	INT GetNewTriggerId(TriggerType type);
	GameRoomManager* GetGameRoomManager() { return m_game_room_manager.get(); }
	HANDLE GetIOCPHandle() const { return m_handle_iocp; }

	// 플레이어 처리
	void Move(const std::shared_ptr<Client>& client, XMFLOAT3 position);

	// 오브젝트 처리
	int GetNearTarget(int client_id, float max_range);
	static void MoveObject(const std::shared_ptr<GameObject>& object, XMFLOAT3 position);
	static void RotateBoundingBox(const std::shared_ptr<GameObject>& object);
	void SetPositionOnStairs(const std::shared_ptr<GameObject>& object);

	void CollideObject(const std::shared_ptr<GameObject>& object, const std::span<INT>& ids,
		std::function<void(const std::shared_ptr<GameObject>&, const std::shared_ptr<GameObject>&)> func);
	static void CollideByStatic(const std::shared_ptr<GameObject>& object,
		const std::shared_ptr<GameObject>& static_object);
	static void CollideByMoveMent(const std::shared_ptr<GameObject>& object,
		const std::shared_ptr<GameObject>& movement_object);
	static void CollideByStaticOBB(const std::shared_ptr<GameObject>& object,
		const std::shared_ptr<GameObject>& static_object);

	std::array<std::shared_ptr<MovementObject>, MAX_OBJECT> m_clients;
	std::array<std::shared_ptr<Trigger>, MAX_TRIGGER> m_triggers;

private:
	std::unique_ptr<GameRoomManager> m_game_room_manager;

	SOCKET				m_server_socket;
	HANDLE				m_handle_iocp;

	std::vector<std::thread>	m_worker_threads;
	bool						m_accept;

	concurrency::concurrent_priority_queue<TIMER_EVENT> m_timer_queue;

	Server();
};