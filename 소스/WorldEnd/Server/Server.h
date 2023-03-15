#pragma once
#include "client.h"
#include "monster.h"
#include "object.h"
#include "map.h"

struct TIMER_EVENT {
	INT obj_id;
	INT targat_id;
	std::chrono::system_clock::time_point event_time;
	EventType event_type;

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

	void SendLoginOkPacket(const shared_ptr<Client>& player) const;
	void SendPlayerDataPacket();

	void PlayerCollisionCheck(shared_ptr<Client>& player);

	void Timer();

	INT GetNewId();

	// 플레이어 처리
	void MovePlayer(shared_ptr<Client>& player, XMFLOAT3 velocity);
	void RotatePlayer(shared_ptr<Client>& player, FLOAT yaw);

	void CollideByStatic(shared_ptr<Client>& player, const BoundingOrientedBox& obb);
	void CollideByMoveMent(shared_ptr<Client>& player1, shared_ptr<Client>& player2);
	void CollideByStaticOBB(shared_ptr<Client>& player, const BoundingOrientedBox& obb);

	array<shared_ptr<MovementObject>, MAX_OBJECT> m_clients;

private:
	unique_ptr<GameRoomManager> m_game_room_manager;

	SOCKET				m_server_socket;
	HANDLE				m_handle_iocp;

	vector<thread>		m_worker_threads;
	bool				m_accept;

	concurrency::concurrent_priority_queue<TIMER_EVENT> m_timer_queue;

	Server();
};