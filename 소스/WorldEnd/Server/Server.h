#pragma once
#include "client.h"
#include "monster.h"
#include "object.h"

struct TIMER_EVENT {
	int obj_id;
	int targat_id;
	std::chrono::system_clock::time_point event_time;
	EventType event_type;

	constexpr bool operator <(const TIMER_EVENT& left)const
	{
		return (event_time > left.event_time);
	}
};

struct COLLISION_EVENT {
	int user_id;										// 공격자 id
	CollisionType collision_type;						// 충돌 타입 (일회성, 지속성)
	AttackType attack_type;								// 공격 타입 (기본공격, 스킬)
	BoundingOrientedBox bounding_box;					// 충돌 범위
	std::chrono::system_clock::time_point end_time;		// 충돌 이벤트 종료 시간
};

class Server
{
public:
	Server();
	~Server() = default;
	
	int Network();
	void WorkerThreads();
	void ProcessPacket(const int id, char* p);
	void Disconnect(const int id);

	void SendLoginOkPacket(const Client& player) const;
	void SendPlayerDataPacket();

	void PlayerCollisionCheck(Client& player);

	void Timer();

	CHAR GetNewId();

	// 플레이어 처리
	void MovePlayer(Client& player, XMFLOAT3 velocity);
	void RotatePlayer(Client& player, FLOAT yaw);

	void CollideByStatic(Client& player, const BoundingOrientedBox& obb);
	void CollideByMoveMent(Client& player1, Client& player2);
	void CollideByStaticOBB(Client& player, const BoundingOrientedBox& obb);

	array<Client, MAX_USER>				m_clients;

private:
	vector<thread>                          m_worker_threads;
	bool									m_accept;

	concurrency::concurrent_priority_queue<TIMER_EVENT>	 m_timer_queue;
};