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

class Server
{

private:
	array<Session, MAX_USER>				m_clients;
	vector<std::unique_ptr<Monster>>		m_monsters;			

	// 통신 관련 변수
	vector <thread>                         m_worker_threads;
	int 									m_disconnect_cnt;	// 연결 끊긴 인원 수
	bool									m_accept;

	// 게임 관련
	int                                     m_round;
	char                                    m_next_monster_id;

	int                                     m_start_cool_time;
	int                                     m_end_cool_time = 5;
	int                                     m_remain_cool_time;

	bool                                    m_attack_check = false;

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
	void SendMonsterDataPacket();

	void PlayerCollisionCheck (Session& player, const int id);
	void Update();
	void CreateMosnters();

	void Timer();

	CHAR GetNewId() const;
};