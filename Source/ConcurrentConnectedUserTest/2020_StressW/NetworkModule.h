#pragma once
#include "Client.h"

class NetworkModule {
public:
	void InitializeNetwork();

	void WorkerThread();
	void ProcessPacket(int ci, unsigned char packet[]);

	void Disconnect(int ci);

	void SendLoginPacket();
	void SendPlayerMovePacket(int connections);

	void AdjustNumberOfClient();
	void TestThread();
	void PointClient(int* size, float** points);

private:

	high_resolution_clock::time_point m_last_connect_time;

	HANDLE m_handle_hiocp;

	array<int, MAX_CLIENTS>      m_client_map;
	array<Client, MAX_CLIENTS>   m_clients;
	array<MONSTER, MAX_INGAME_MONSTER>  m_monsters;

	vector <thread*>             m_worker_threads;
	thread                       m_test_thread;

	atomic_int m_num_connections;
	atomic_int m_client_to_close;

	FLOAT point_cloud[MAX_TEST * 2];
};
