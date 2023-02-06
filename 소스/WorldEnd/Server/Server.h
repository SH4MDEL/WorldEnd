#pragma once
#include "session.h"

class Server
{
	// 통신 관련 변수
private:
	array<Session, MAX_USER>				m_clients;
	vector <thread>                         m_worker_threads;

	INT									    m_disconnect_cnt;	// 연결 끊긴 인원 수

	int                                     m_start_cool_time;
	int                                     m_end_cool_time = 5;
	int                                     m_remain_cool_time;

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

	void PlayerCollisionCheck (Session& player, const int id);

	CHAR GetNewId() const;
};