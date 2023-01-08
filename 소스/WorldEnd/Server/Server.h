#pragma once
#include "session.h"

class Server
{
	// 통신 관련 변수
private:
	array<Session, MAX_USER>				clients;
	vector <thread>                         m_worker_threads;

	INT									    disconnect_cnt;	// 연결 끊긴 인원 수
public:
	Server();
	~Server() = default;
	
	int Network();
	void WorkerThreads();
	void ProcessPacket(const int id, char* p);
	void Disconnect(const int id);

	void SendLoginOkPacket(const Session& player) const;
	void SendPlayerDataPacket();

	CHAR GetNewId() const;
};