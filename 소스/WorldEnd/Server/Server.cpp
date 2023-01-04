#include "Server.h"
#include "stdafx.h"

int Server::Network()
{

	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
		return 1;
	g_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == g_socket) ErrorDisplay("socket()");

	// bind
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVERPORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int retval = bind(g_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	if (retval == SOCKET_ERROR) ErrorDisplay("bind()");

	// listen
	retval = listen(g_socket, SOMAXCONN);
	if (retval == SOCKET_ERROR) ErrorDisplay("listen()");

	g_h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_socket), g_h_iocp, 0, 0);
	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, 0, 0, WSA_FLAG_OVERLAPPED);
	OVER_EXP accept_ex;
	accept_ex._comp_type = OP_ACCEPT;

	AcceptEx(g_socket, c_socket, accept_ex._send_buf, 0, sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, NULL, &accept_ex._over);

	vector <thread> m_worker_threads;
	for (int i = 0; i < 6; ++i)
		m_worker_threads.emplace_back(&Server::WorkerThreads, this);
	for (auto& th : m_worker_threads)
		th.join();

	closesocket(g_socket);
	WSACleanup();
	return 0;
}

void Server::WorkerThreads() {
	
	cout << "WorkerThread Start" << endl;
	for (;;) {
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(g_h_iocp, &num_bytes, &key, &over, INFINITE);
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);

		if (FALSE == ret) {
			if (ex_over->_comp_type == OP_ACCEPT) cout << "Accept Error";
			else {
				cout << "GQCS Error on client[" << key << "]\n";
				//disconnect(static_cast<int>(key));
				if (ex_over->_comp_type == OP_SEND) delete ex_over;
				continue;
			}
		}


	}
}


