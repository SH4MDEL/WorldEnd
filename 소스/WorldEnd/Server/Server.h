#pragma once
#include "client.h"
#include "monster.h"
#include "object.h"
#include "map.h"


enum class EventType : char { RESET_COOLTIME, CHANGE_BEHAVIOR };

struct TIMER_EVENT {
	std::chrono::system_clock::time_point event_time;
	EventType event_type;
	INT obj_id;
	INT targat_id;
	CooltimeType cooltime_type;
	MonsterBehavior behavior_type;

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

	void PlayerCollisionCheck(const shared_ptr<Client>& player);

	void Timer();
	void ProcessEvent(const TIMER_EVENT& ev);

	void SetTimerEvent(const TIMER_EVENT& ev);

	INT GetNewId();
	INT GetNewMonsterId(MonsterType type);
	GameRoomManager* GetGameRoomManager() { return m_game_room_manager.get(); }

	// 플레이어 처리
	void MovePlayer(const shared_ptr<GameObject>& object, XMFLOAT3 velocity);
	void RotatePlayer(const shared_ptr<GameObject>& object, FLOAT yaw);

	/*void CollisionCheck(const shared_ptr<GameObject>& object, const span<INT> ids,
		void (Server::*callable)(const shared_ptr<GameObject>&, const shared_ptr<GameObject>&));*/
	void CollisionCheck(const shared_ptr<GameObject>& object, const span<INT> ids);
	void CollideByStatic(const shared_ptr<GameObject>& object, const shared_ptr<GameObject>& object1);
	void CollideByMoveMent(const shared_ptr<GameObject>& object, const shared_ptr<GameObject>& object1);
	void CollideByStaticOBB(const shared_ptr<GameObject>& objec, const shared_ptr<GameObject>& object1);

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