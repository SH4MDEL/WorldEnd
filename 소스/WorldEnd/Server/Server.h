#pragma once
#include "session.h"

class Server
{
	// ��� ���� ����
private:
	array<Session, MAX_USER>				clients;
	vector <thread>                         m_worker_threads;

	INT									    disconnect_cnt;	// ���� ���� �ο� ��

	int                                     start_cool_time  {};
	int                                     end_cool_time = 5;
	int                                     remain_cool_time{};

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

	CHAR GetNewId() const;
};