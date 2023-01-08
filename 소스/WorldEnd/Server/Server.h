#pragma once
#include "session.h"

class Server
{
	// ��� ���� ����
private:
	array<Session, MAX_USER>				clients;
	vector <thread>                         m_worker_threads;

	INT									    disconnect_cnt;	// ���� ���� �ο� ��
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