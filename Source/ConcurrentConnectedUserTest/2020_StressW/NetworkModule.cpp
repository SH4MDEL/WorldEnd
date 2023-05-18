#include "stdafx.h"
#include "NetworkModule.h"

void NetworkModule::InitializeNetwork()
{
	for (auto& cl : m_clients) {
		cl.m_connect = false;
		cl.GetID() = INVALID_ID;
	}

	for (auto& cl : m_client_map) cl = -1;
	m_num_connections = 0;
	m_last_connect_time = high_resolution_clock::now();

	WSADATA	wsa_data;
	WSAStartup(MAKEWORD(2, 2), &wsa_data);

	m_handle_hiocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, NULL, 0);

	for (int i = 0; i < 6; ++i)
		m_worker_threads.emplace_back(new std::thread{ &NetworkModule::WorkerThread, this });

	m_test_thread = thread{ &NetworkModule::TestThread, this };
}



void NetworkModule::WorkerThread()
{
	while (true) {
		DWORD io_size;
		unsigned long long ci;
		ExpOver* over;
		BOOL ret = GetQueuedCompletionStatus(m_handle_hiocp, &io_size, &ci,
			reinterpret_cast<LPWSAOVERLAPPED*>(&over), INFINITE);
		int client_id = static_cast<int>(ci);

		if (FALSE == ret) {
			int err_no = WSAGetLastError();
			if (64 == err_no) Disconnect(client_id);
			else {
				//ErrorDisplay("GQCS : ", WSAGetLastError());
				Disconnect(client_id);
			}
			if (OP_SEND == over->event_type) delete over;
		}
		if (0 == io_size) {
			Disconnect(client_id);
			continue;
		}
		if (OP_RECV == over->event_type) {
			unsigned char* buf = m_clients[ci].GetExpOver().IOCP_buf;
			unsigned packet_size = m_clients[ci].GetCurrentPacketSize();
			unsigned pacekt_remain_size = m_clients[ci].GetPrevPacketData();
			while (io_size > 0) {
				if (0 == packet_size) packet_size = buf[0];
				if (io_size + pacekt_remain_size >= packet_size) {
					// 패킷 재조립
					unsigned char packet[MAX_PACKET_SIZE];
					memcpy(packet, m_clients[ci].GetPacketBuffer(), pacekt_remain_size);
					memcpy(packet + pacekt_remain_size, buf, packet_size - pacekt_remain_size);
					ProcessPacket(static_cast<int>(ci), packet);
					io_size -= packet_size - pacekt_remain_size;
					buf += packet_size - pacekt_remain_size;
					packet_size = 0; pacekt_remain_size = 0;
				}
				else {
					memcpy(m_clients[ci].GetPacketBuffer() + pacekt_remain_size, buf, io_size);
					pacekt_remain_size += io_size;
					io_size = 0;
				}
			}
			m_clients[ci].GetCurrentPacketSize() = packet_size;
			m_clients[ci].GetPrevPacketData() = pacekt_remain_size;
		
			/*constexpr int MAX_FAME = 60;
			using frame = std::chrono::duration<int32_t, std::ratio<1, MAX_FAME>>;
			using ms = std::chrono::duration<float, std::milli>;
			std::chrono::time_point<std::chrono::steady_clock> fps_timer{ std::chrono::steady_clock::now() };
			frame fps{}, frame_count{};

			while (true) {
			
				fps = duration_cast<frame>(std::chrono::steady_clock::now() - fps_timer);

				if (fps.count() < 1) continue;

				if (frame_count.count() & 1) {
					m_clients[ci].DoRecv();
				}

				frame_count = duration_cast<frame>(frame_count + fps);
				if (frame_count.count() >= MAX_FAME) {
					frame_count = frame::zero();
				}
				else {
				}
				fps_timer = std::chrono::steady_clock::now();
			}*/

			m_clients[ci].DoRecv();
		}
		else if (OP_SEND == over->event_type) {
			if (io_size != over->wsabuf.len) {
				Disconnect(client_id);
			}
			delete over;
		}
		else if (OP_DO_MOVE == over->event_type) {
			delete over;
		}
		else {
			std::cout << "Unknown GQCS event!\n";
			break;
		}
	}
}

void NetworkModule::ProcessPacket(int ci, unsigned char packet[])
{
	switch (packet[1]) {
	case SC_PACKET_UPDATE_CLIENT: {
		SC_UPDATE_CLIENT_PACKET* move_packet = reinterpret_cast<SC_UPDATE_CLIENT_PACKET*>(packet);
		if (move_packet->data.id < MAX_CLIENTS) {
			if (move_packet->data.id == -1) return;
			int my_id = m_client_map[move_packet->data.id];
			if (-1 != my_id) {
				m_clients[my_id].GetX() = move_packet->data.pos.x;
				m_clients[my_id].GetY() = move_packet->data.pos.y;
			}
			if (ci == my_id) {
				if (0 != move_packet->move_time) {
					auto d_ms = duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count() - move_packet->move_time;

					if (global_delay < d_ms) global_delay++;
					else if (global_delay > d_ms) global_delay--;
				}
			}
		}
	}
								break;
	case SC_PACKET_LOGIN_OK:
	{
		m_clients[ci].m_connect = true;
		g_active_clients++;
		SC_LOGIN_OK_PACKET* login_packet = reinterpret_cast<SC_LOGIN_OK_PACKET*>(packet);
		int my_id = ci;
		m_client_map[login_packet->player_data.id] = my_id;
		m_clients[my_id].GetID() = login_packet->player_data.id;
		m_clients[my_id].GetX() = login_packet->player_data.pos.x;
		m_clients[my_id].GetY() = login_packet->player_data.pos.y;
	}
	break;
	case SC_PACKET_UPDATE_MONSTER:
	{
		SC_UPDATE_MONSTER_PACKET* monster_packet = reinterpret_cast<SC_UPDATE_MONSTER_PACKET*>(packet);

		MONSTER_DATA monster_data{};

		m_monsters[monster_data.id].id = monster_packet->monster_data.id;
		m_monsters[monster_data.id].x = monster_packet->monster_data.pos.x;
		m_monsters[monster_data.id].y = monster_packet->monster_data.pos.y;
	}
	break;
	case SC_PACKET_ADD_MONSTER:
	{
		SC_ADD_MONSTER_PACKET* mon_add_packet = reinterpret_cast<SC_ADD_MONSTER_PACKET*>(packet);

		MONSTER_DATA monster_data{};

		m_monsters[monster_data.id].id = mon_add_packet->monster_data.id;
		m_monsters[monster_data.id].x = mon_add_packet->monster_data.pos.x;
		m_monsters[monster_data.id].y = mon_add_packet->monster_data.pos.y;
	}
	break;
	case SC_PACKET_ADD_OBJECT: break;
	}
}

void NetworkModule::Disconnect(int ci)
{
	bool status = true;
	if (true == atomic_compare_exchange_strong(&m_clients[ci].m_connect, &status, false)) {
		closesocket(m_clients[ci].m_connect);
		g_active_clients--;
	}

	// 아래 주석은 bool변수 사용한 방법
	/*if (m_clients[ci].GetConnect() == status) {
		closesocket(m_clients[ci].m_connect);
		g_active_clients--;
	}*/
}

void NetworkModule::SendLoginPacket()
{
	CS_LOGIN_PACKET login_packet;
	int temp = m_num_connections;
	sprintf_s(login_packet.name, "%d", temp);
	login_packet.size = sizeof(login_packet);
	login_packet.type = CS_PACKET_LOGIN;
	m_clients[m_num_connections].DoSend(&login_packet);
}

void NetworkModule::SendPlayerMovePacket(int connections)
{
	CS_PLAYER_MOVE_PACKET move_packet{};
	move_packet.size = sizeof(move_packet);
	move_packet.type = CS_PACKET_PLAYER_MOVE;
	switch (rand() % 4) {
	case 0: move_packet.pos.x++; break;
	case 1: move_packet.pos.x--; break;
	case 2: move_packet.pos.z++; break;
	case 3: move_packet.pos.z--; break;
	}
	move_packet.move_time = static_cast<unsigned>(duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count());
	m_clients[connections].DoSend(&move_packet);
}

void NetworkModule::AdjustNumberOfClient()
{
	static int delay_multiplier = 1;
	static int max_limit = MAXINT;
	static bool increasing = true;

	if (g_active_clients >= MAX_TEST) return;
	if (m_num_connections >= MAX_CLIENTS) return;

	auto duration = high_resolution_clock::now() - m_last_connect_time;
	if (ACCEPT_DELY * delay_multiplier > duration_cast<milliseconds>(duration).count()) return;

	int t_delay = global_delay;
	if (DELAY_LIMIT2 < t_delay) {
		if (true == increasing) {
			max_limit = g_active_clients;
			increasing = false;
		}
		if (100 > g_active_clients) return;
		if (ACCEPT_DELY * 10 > duration_cast<milliseconds>(duration).count()) return;
		m_last_connect_time = high_resolution_clock::now();
		Disconnect(m_client_to_close);
		m_client_to_close++;
		return;
	}
	else
		if (DELAY_LIMIT < t_delay) {
			delay_multiplier = 10;
			return;
		}
	if (max_limit - (max_limit / 20) < g_active_clients) return;

	increasing = true;
	m_last_connect_time = high_resolution_clock::now();
	m_clients[m_num_connections].GetSocket() = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN ServerAddr;
	ZeroMemory(&ServerAddr, sizeof(SOCKADDR_IN));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(SERVER_PORT);
	ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	int Result = WSAConnect(m_clients[m_num_connections].GetSocket(), (sockaddr*)&ServerAddr, sizeof(ServerAddr), NULL, NULL, NULL, NULL);
	if (0 != Result) {
		ErrorDisplay("WSAConnect : ", GetLastError());
	}

	m_clients[m_num_connections].GetCurrentPacketSize() = 0;
	m_clients[m_num_connections].GetPrevPacketData() = 0;
	ZeroMemory(&m_clients[m_num_connections].GetExpOver(), sizeof(m_clients[m_num_connections].GetExpOver()));
	m_clients[m_num_connections].GetExpOver().event_type = OP_RECV;
	m_clients[m_num_connections].GetExpOver().wsabuf.buf =
		reinterpret_cast<CHAR*>(m_clients[m_num_connections].GetExpOver().IOCP_buf);
	m_clients[m_num_connections].GetExpOver().wsabuf.len = sizeof(m_clients[m_num_connections].GetExpOver().IOCP_buf);

	CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_clients[m_num_connections].GetSocket()), m_handle_hiocp, m_num_connections, 0);

	SendLoginPacket();

	m_clients[m_num_connections].DoRecv();

	m_num_connections++;
fail_to_connect:
	return;
}

void NetworkModule::TestThread()
{
	while (true) {
		AdjustNumberOfClient();

		for (int i = 0; i < m_num_connections; ++i) {
			if (false == m_clients[i].m_connect) continue;
			if (m_clients[i].m_last_move_time + 1s > high_resolution_clock::now()) continue;
			m_clients[i].m_last_move_time = high_resolution_clock::now();
			SendPlayerMovePacket(i);
		}
	}
}


void NetworkModule::PointClient(int* size, float** points)
{
	int index = 0;
	for (int i = 0; i < m_num_connections; ++i)
		if (true == m_clients[i].m_connect) {
			point_cloud[index * 2] = static_cast<float>(m_clients[i].GetX());
			point_cloud[index * 2 + 1] = static_cast<float>(m_clients[i].GetY());
			index++;
		}

	*size = index;
	*points = point_cloud;
}

