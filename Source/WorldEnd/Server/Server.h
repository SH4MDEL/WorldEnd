﻿#pragma once
#include <functional>
#include "client.h"
#include "monster.h"
#include "object.h"
#include "map.h"


enum class EventType : char 
{
	COOLTIME_RESET, BEHAVIOR_CHANGE, AGRO_LEVEL_DECREASE,
	ATTACK_COLLISION, MONSTER_ATTACK_COLLISION, STAMINA_CHANGE
};

struct TIMER_EVENT {
	std::chrono::system_clock::time_point event_time;
	INT obj_id;
	INT targat_id;
	XMFLOAT3 position;
	EventType event_type;
	ActionType action_type;
	MonsterBehavior next_behavior_type;
	BYTE latest_id;
	BYTE aggro_level;
	CollisionType collision_type;
	bool is_stamina_increase;

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

	void SendLoginOkPacket(int client_id);
	void SendMoveInGameRoom(int client_id);
	void SendDeathPacket(int client_id);

	bool IsInGameRoom(int client_id);
	void GameRoomPlayerCollisionCheck(const std::shared_ptr<Client>& player);

	void Timer();
	void ProcessEvent(const TIMER_EVENT& ev);

	void SetTimerEvent(const TIMER_EVENT& ev);
	void SetAttackTimerEvent(int id, ActionType attack_type, CollisionType collision_type,
		std::chrono::system_clock::time_point attack_time);
	void SetCooltimeTimerEvent(int id, ActionType action_type);
	void SetHitScanTimerEvent(int id);

	INT GetNewId();
	INT GetNewMonsterId(MonsterType type);
	GameRoomManager* GetGameRoomManager() { return m_game_room_manager.get(); }
	HANDLE GetIOCPHandle() const { return m_handle_iocp; }

	// 플레이어 처리
	void Move(const std::shared_ptr<Client>& client, XMFLOAT3 position);

	// 오브젝트 처리
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

private:
	std::unique_ptr<GameRoomManager> m_game_room_manager;

	SOCKET				m_server_socket;
	HANDLE				m_handle_iocp;

	std::vector<std::thread>	m_worker_threads;
	bool						m_accept;

	concurrency::concurrent_priority_queue<TIMER_EVENT> m_timer_queue;

	Server();
};