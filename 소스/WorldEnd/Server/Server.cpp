#include "Server.h"
#include "stdafx.h"

Server::Server() : disconnect_cnt{ 0 }
{
}

int Server::Network()
{

	WSADATA wsa_data;
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
		return 1;
	g_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (g_socket == INVALID_SOCKET) ErrorDisplay("socket()");

	// bind
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int retval = bind(g_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	if (retval == SOCKET_ERROR) ErrorDisplay("bind()");

	// listen
	retval = listen(g_socket, SOMAXCONN);
	if (retval == SOCKET_ERROR) ErrorDisplay("listen()");

	g_h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_socket), g_h_iocp, 0, 0);

	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	char accept_buf[sizeof(SOCKADDR_IN) * 2 + 32 + 100];
	OverExp accept_ex;
	*(reinterpret_cast<SOCKET*>(&accept_ex._send_buf)) = c_socket;
	ZeroMemory(&accept_ex._wsa_over, sizeof(accept_ex._wsa_over));
	accept_ex._comp_type = OP_ACCEPT;

	bool ret = AcceptEx(g_socket, c_socket, accept_buf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, 0, &accept_ex._wsa_over);


	for (int i = 0; i < 6; ++i)
		m_worker_threads.emplace_back(&Server::WorkerThreads, this);
	for (auto& th : m_worker_threads)
		th.join();

	closesocket(g_socket);
	WSACleanup();
	return 0;
}

void Server::WorkerThreads() 
{
	for (;;) {
		DWORD num_bytes{};
		LONG64 key{};
		WSAOVERLAPPED* over{};
		BOOL ret = GetQueuedCompletionStatus(g_h_iocp, &num_bytes, reinterpret_cast<PULONG_PTR>(&key), &over, INFINITE);
		OverExp* exp_over = reinterpret_cast<OverExp*>(over);
		if (FALSE == ret) {
			if (exp_over->_comp_type == OP_ACCEPT) cout << "Accept Error";
			else {
				cout << "GQCS Error on client[" << key << "]\n";
				//Disconnect(static_cast<int>(key));
				if (exp_over->_comp_type == OP_SEND) delete exp_over;
				continue;
			}
		}
		switch (exp_over->_comp_type) {
		case OP_ACCEPT: {
			SOCKET c_socket = *(reinterpret_cast<SOCKET*>(exp_over->_send_buf));
			CHAR new_id{ GetNewId() };
			if (new_id == -1) {
				std::cout << "Maxmum user overflow. Accept aborted." << std::endl;
			}
			else {
				Session& cl = clients[new_id];
				cl.m_lock.lock();
				cl.m_player_data.id = new_id;
				cl.m_player_data.active_check = true;
				cl.m_socket = c_socket;
				cl.m_ready_check = false;
				constexpr char dummy_name[10] = "dummy\0";
				strcpy_s(cl.m_name, sizeof(dummy_name), dummy_name);
				cl.m_player_type = ePlayerType::SWORD;
				cl.m_prev_size = 0;
				cl.m_recv_over._comp_type = OP_RECV;
				cl.m_recv_over._wsa_buf.buf = reinterpret_cast<char*>(cl.m_recv_over._send_buf);
				cl.m_recv_over._wsa_buf.len = sizeof(cl.m_recv_over._send_buf);
				ZeroMemory(&cl.m_recv_over._wsa_over, sizeof(cl.m_recv_over._wsa_over));
				cl.m_lock.unlock();

				CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), g_h_iocp, new_id, 0);
				cl.DoRecv();
			}
			ZeroMemory(&exp_over->_wsa_over, sizeof(exp_over->_wsa_over));
			c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
			*(reinterpret_cast<SOCKET*>(exp_over->_send_buf)) = c_socket;
			AcceptEx(g_socket, c_socket, exp_over->_send_buf + 8, 0,
				sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, 0, &exp_over->_wsa_over);
			break;
		}
		case OP_RECV: {
			if (num_bytes == 0) {
				//disconnect(static_cast<int>(key));
				continue;
			}
			Session& cl = clients[static_cast<int>(key)];
			int remain_data = num_bytes + cl.m_prev_size;
			char* packet_start = exp_over->_send_buf;
			int packet_size = packet_start[0];

			while (packet_size <= remain_data) {
				ProcessPacket(static_cast<int>(key), packet_start);
				remain_data -= packet_size;
				packet_start += packet_size;
				if (remain_data > 0) packet_size = packet_start[0];
				else break;
			}

			if (0 < remain_data) {
				cl.m_prev_size = remain_data;
				memcpy(&exp_over->_send_buf, packet_start, remain_data);
			}
			cl.DoRecv();
			break;
		}
		case OP_SEND: {
			if (num_bytes != exp_over->_wsa_buf.len) {
				//Disconnect(static_cast<int>(key));
			}
			delete exp_over;
			break;
		}

		}
	}
}

void Server::ProcessPacket(const int id, char* p) 
{
	char packet_type = p[1];
	Session& cl = clients[id];
	
	switch (packet_type)
	{
	case CS_PACKET_LOGIN:
	{
		CS_LOGIN_PACKET* login_packet = reinterpret_cast<CS_LOGIN_PACKET*>(p);
		strcpy_s(cl.m_name, login_packet->name);
		cl.m_lock.lock();
		SendLoginOkPacket(cl);
		cl.m_state = STATE::ST_INGAME;
		cl.m_lock.unlock();
		cout << login_packet->name << " is connect" << endl;

		// 재접속 시 disconnectCount를 감소시켜야함
		// disconnect_cnt = max(0, disconnect_cnt - 1);
		SendPlayerDataPacket();
		break;
	}
	case CS_PACKET_PLAYER_MOVE:
	{
		CS_PLAYER_MOVE_PACKET* move_packet = reinterpret_cast<CS_PLAYER_MOVE_PACKET*>(p);
		
		cl.m_player_data.pos = move_packet->pos;
		cl.m_player_data.velocity = move_packet->velocity;
		cl.m_player_data.yaw = move_packet->yaw;

		cout << "x: " << cl.m_player_data.pos .x << " y: " << cl.m_player_data.pos.y <<
			" z: " << cl.m_player_data.pos.z << endl;
		SendPlayerDataPacket();
		break;
	}
	case CS_PACKET_PLAYER_ATTACK:
	{
	
		CS_ATTACK_PACKET* attack_packet = reinterpret_cast<CS_ATTACK_PACKET*>(p);

		SendPlayerAttackPacket(cl.m_player_data.id);
			switch (attack_packet->key)
			{
			case INPUT_KEY_E:
			{
				cout << "공격!" << endl;
				auto attack_start_time = std::chrono::system_clock::now();
				while (1)
				{
					auto attack_end_time = std::chrono::system_clock::now();
					auto sec = std::chrono::duration_cast<std::chrono::seconds>(attack_end_time - attack_start_time);
					if (sec.count() > start_cool_time)
					{
						start_cool_time++;
						remain_cool_time = end_cool_time - start_cool_time;
						cout << "남은 공격 쿨타임: " << remain_cool_time << "초" << endl;
					}
					else if (start_cool_time == 5) {
						start_cool_time = 0;
						break;
						
					}
				}

				break;
			}
			
		}

		break;
	}
	
	}
}


void Server::Disconnect(const int id)
{
	Session& cl = clients[id];
	cl.m_lock.lock();
	cl.m_player_data.id = 0;
	cl.m_player_data.active_check = false;
	cl.m_player_data.pos = {};
	cl.m_player_data.velocity = {};
	cl.m_player_data.yaw = 0.0f;
	closesocket(cl.m_socket);
	cl.m_lock.unlock();
}

void Server::SendLoginOkPacket(const Session& player) const
{
	SC_LOGIN_OK_PACKET login_ok_packet{};
	login_ok_packet.size = sizeof(login_ok_packet);
	login_ok_packet.type = SC_PACKET_LOGIN_OK;
	login_ok_packet.player_data.id = player.m_player_data.id;
	login_ok_packet.player_data.active_check = true;
	login_ok_packet.player_data.pos.x = player.m_player_data.pos.x;
	login_ok_packet.player_data.pos.y = player.m_player_data.pos.y;
	login_ok_packet.player_data.pos.z = player.m_player_data.pos.z;
	login_ok_packet.player_data.hp = player.m_player_data.hp;
	strcpy_s(login_ok_packet.name, sizeof(login_ok_packet.name), player.m_name);

	char buf[sizeof(login_ok_packet)];
	memcpy(buf, reinterpret_cast<char*>(&login_ok_packet), sizeof(login_ok_packet));
	WSABUF wsa_buf{ sizeof(buf), buf };
	DWORD sent_byte;

	const int retval = WSASend(player.m_socket, &wsa_buf, 1, &sent_byte, 0, nullptr, nullptr);
	if (retval == SOCKET_ERROR) ErrorDisplay("Send(SC_LOGIN_OK_PACKET) Error");

	login_ok_packet.type = SC_PACKET_ADD_PLAYER;
	memcpy(buf, reinterpret_cast<char*>(&login_ok_packet), sizeof(login_ok_packet));

	// 현재 접속해 있는 모든 클라이언트들에게 새로 로그인한 클라이언트들의 정보를 전송
	for (const auto& other : clients)
	{
		if (!other.m_player_data.active_check) continue;
		if (player.m_player_data.id == other.m_player_data.id) continue;
		cout << "sc packet1 " << endl;
		const int retval = WSASend(other.m_socket, &wsa_buf, 1, &sent_byte, 0, nullptr, nullptr);
		if (retval == SOCKET_ERROR) ErrorDisplay("Send(SC_LOGIN_OK_PACKET) Error");
	}

	// 새로 로그인한 클라이언트에게 현재 접속해 있는 모든 클라이언트들의 정보를 전송
	for (const auto& other : clients)
	{
		if (!other.m_player_data.active_check) continue;
		if (static_cast<int>(other.m_player_data.id) == player.m_player_data.id) continue;

		cout << "sc packet2 " << endl;
		SC_LOGIN_OK_PACKET sub_packet{};
		sub_packet.size = sizeof(sub_packet);
		sub_packet.type = SC_PACKET_ADD_PLAYER;
		sub_packet.player_data = other.m_player_data;
		strcpy_s(sub_packet.name, sizeof(sub_packet.name), other.m_name);
		//sub_packet.ready_check = other.m_ready_check;
		sub_packet.player_type = other.m_player_type;

		memcpy(buf, reinterpret_cast<char*>(&sub_packet), sizeof(sub_packet));
		const int retval = WSASend(player.m_socket, &wsa_buf, 1, &sent_byte, 0, nullptr, nullptr);
		if (retval == SOCKET_ERROR) ErrorDisplay("Send(SC_LOGIN_OK_PACKET) Error");
		cout << (int)other.m_player_data.id << "add player to " << (int)player.m_player_data.id << endl;
	}

	std::cout << "[ id: " << static_cast<int>(login_ok_packet.player_data.id) << " Login Packet Received ]" << std::endl;
	cout << "HP: " << login_ok_packet.player_data.hp << endl;

}

void Server::SendPlayerDataPacket()
{
	
	SC_UPDATE_CLIENT_PACKET update_packet{};
	update_packet.size = sizeof(update_packet);
	update_packet.type = SC_PACKET_UPDATE_CLIENT;
	for (int i = 0; i < MAX_USER; ++i)
		update_packet.data[i] = clients[i].m_player_data;

	for (int i = 1; i < MAX_USER; ++i) 
	{
		if (!update_packet.data[i].active_check)
		{
			update_packet.data[i].id = -1;
		}
	}

	char buf[sizeof(update_packet)];
	memcpy(buf, reinterpret_cast<char*>(&update_packet), sizeof(update_packet));
	WSABUF wsa_buf{ sizeof(buf), buf };
	DWORD sent_byte;

	for (const auto& cl : clients)
	{
		if (!cl.m_player_data.active_check) continue; 
		
		const int retval = WSASend(cl.m_socket, &wsa_buf, 1, &sent_byte, 0, nullptr, nullptr);
		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAECONNRESET)
				std::cout << "[" << static_cast<int>(cl.m_player_data.id) << " Session] Disconnect" << std::endl;
			else ErrorDisplay("Send(SC_PACKET_UPDATE_CLIENT)");
		}
	}
	//cout << "작동중인 모든 클라이언트들에게 이동 결과를 알려줌" << endl;
}

void Server::SendPlayerAttackPacket(int pl_id)
{
	SC_ATTACK_PACKET attack_packet;
	attack_packet.size = sizeof(attack_packet);
	attack_packet.type = SC_PACKET_PLAYER_ATTACK;
	attack_packet.id = pl_id;

	char buf[sizeof(attack_packet)];
	memcpy(buf, reinterpret_cast<char*>(&attack_packet), sizeof(attack_packet));
	WSABUF wsa_buf{ sizeof(buf), buf };
	DWORD sent_byte;

	for (const auto& cl : clients) {
		if (!cl.m_player_data.active_check) continue;
		const int retval = WSASend(cl.m_socket, &wsa_buf, 1, &sent_byte, 0, nullptr, nullptr);
		if (retval == SOCKET_ERROR) ErrorDisplay("Send(SC_ATTACK_PACKET) Error");
	}
}


CHAR Server::GetNewId() const
{
	for (int i = 0; i < MAX_USER; ++i) {
		if (false == clients[i].m_player_data.active_check)
		{
			return i;
		}
	}
	std::cout << "Maximum Number of Clients" << std::endl;
	return -1;
}



