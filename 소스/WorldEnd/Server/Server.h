#pragma once
#include "session.h"
#include "monster.h"

struct TimerEvent {
	int obj_id;
	int targat_id;
	std::chrono::system_clock::time_point start_time;
	eEventType event_type;

	constexpr bool operator <(const TimerEvent& left)const
	{
		return (start_time > left.start_time);
	}
};

constexpr float                             g_spawm_stop = 2.0;

class Server
{

private:
	vector<std::unique_ptr<Monster>>		m_monsters;			

	// 통신 관련 변수
	vector <thread>                         m_worker_threads;
	int 									m_disconnect_cnt;	// 연결 끊긴 인원 수
	bool									m_accept;

	// 게임 관련
	int                                     m_floor;
	int                                     m_floor_mob_count[4];
	float                                   m_spawn_stop;

	char                                    m_next_monster_id;

	int                                     m_start_cool_time;
	int                                     m_end_cool_time = 5;
	int                                     m_remain_cool_time;

	bool                                    m_attack_check = false;

	// 플레이어 관련

	UCHAR                                   m_target_id;
	float                                   m_length{ FLT_MAX };

	concurrency::concurrent_priority_queue<TimerEvent> m_timer_queue;
	concurrency::concurrent_queue<ExpOver*>            m_exp_over;

public:
	Server();
	~Server() = default;
	
	int Network();
	void WorkerThreads();
	void ProcessPacket(const int id, char* p);
	void Disconnect(const int id);

	void SendLoginOkPacket(const Session& player) const;
	void SendPlayerDataPacket();
	void SendPlayerAttackPacket(int pl_id);
	void SendMonsterAddPacket();
	void SendMonsterDataPacket();

	void PlayerCollisionCheck(Session& player);
	void Update(float taketime);
	void CreateMonsters(float taketime);

	void Timer();

	UCHAR RecognizePlayer(const XMFLOAT3& mon_pos);
	CHAR GetNewId() const;


	// 플레이어 처리
	void MovePlayer(Session& player, XMFLOAT3 velocity);
	void RotatePlayer(Session& player, FLOAT yaw);

	void CollideByStatic(Session& player1, DirectX::BoundingOrientedBox obb);
	void CollideByMoveMent(Session& player1, Session& player2);

	array<Session, MAX_USER>				m_clients;
};