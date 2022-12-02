#pragma once
#include "stdafx.h"

const char* SERVER_ADDR = "127.0.0.1";

enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND };

class EXP_OVER {
public:
	WSAOVERLAPPED _over;
	WSABUF _wsabuf;
	char _send_buf[BUF_SIZE];
	COMP_TYPE _comp_type;
	EXP_OVER()
	{
		_wsabuf.len = BUF_SIZE;
		_wsabuf.buf = _send_buf;
		_comp_type = OP_RECV;
		ZeroMemory(&_over, sizeof(_over));
	}
	EXP_OVER(char* packet)
	{
		_wsabuf.len = packet[0];
		_wsabuf.buf = _send_buf;
		ZeroMemory(&_over, sizeof(_over));
		_comp_type = OP_SEND;
		memcpy(_send_buf, packet, packet[0]);
	}
};


ObjectInfo my_info;

array<ObjectInfo, MAX_USER> other_players;
array<ObjectInfo, MAX_NPC> npc_info;

int m_clientID = 0;
SOCKET m_c_socket;

EXP_OVER m_recv_over;



bool Init();
bool ConnectTo();

void RecvPacket();
void SendPacket(void* packet); 
void CALLBACK RecvCallBack(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flag);
void CALLBACK SendCallBack(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flag);
void CProcess_packet(char* ptr);
void ProcessThread(char* net_buf, size_t iocp_byte);

bool Init()
{
	// was start
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 0), &WSAData) != 0) {
		cout << "WSA START ERROR!!" << endl;
		return false;
	}

	// socket
	m_c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (m_c_socket == INVALID_SOCKET) {
		cout << "SOCKET INIT ERROR!!" << endl;
		return true;
	}


}

bool ConnectTo()
{
	// connect to ipAddr
	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVERPORT);
	serverAddr.sin_addr.s_addr = inet_addr(SERVERIP);
	int val = connect(m_c_socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (val == SOCKET_ERROR) return false;

	// login ok
	return true;
}

void RecvLoginInfo() {


	char buf[sizeof(SC_LOGIN_INFO_PACKET)]{};
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(m_c_socket, &wsabuf, 1, &recvByte, &recvFlag, &m_recv_over._over, nullptr);

	SC_LOGIN_INFO_PACKET recv_info_packet{};
	memcpy(&recv_info_packet, buf, sizeof(recv_info_packet));

	my_info.m_id = recv_info_packet.id;
	my_info.m_x = recv_info_packet.x;
	my_info.m_y = recv_info_packet.y;
	my_info.m_z = recv_info_packet.z;

	my_info.m_state = OBJ_ST_RUNNING;
	cout << "Recv My Info - id: " << my_info.m_id << ", Pos( x : " << my_info.m_x << ", y : " << my_info.m_y << ", z :"
		<< my_info.m_z << endl;
}

void RecvPacket()
{
	cout << "Recv" << endl;

	DWORD recv_flag = 0;

	memset(&m_recv_over._over, 0, sizeof(m_recv_over._over));
	m_recv_over._wsabuf.len = BUF_SIZE;
	m_recv_over._wsabuf.buf = m_recv_over._send_buf;


	int ret = WSARecv(m_c_socket, &m_recv_over._wsabuf, 1, 0, &recv_flag, &m_recv_over._over, NULL);
	if (ret != 0 && GetLastError() != WSA_IO_PENDING) {
		cout << "Recv Error - " << ret << endl;
		cout << GetLastError() << endl;
	}

	CProcess_packet(m_recv_over._wsabuf.buf);

}

void SendPacket(void* packet)
{
	EXP_OVER* s_data = new EXP_OVER{ reinterpret_cast<char*>(packet) };
	cout << "Send" << endl;
	int ret = WSASend(m_c_socket, &s_data->_wsabuf, 1, 0, 0, &s_data->_over, 0);
	if (ret != 0) {
		cout << "Send Error - " << ret << endl;
		cout << GetLastError() << endl;
	}

}

//void CALLBACK RecvCallBack(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flag)
//{
//	if (num_bytes == 0) {
//		cout << "Num Byte Zero" << endl;
//		return;
//	}
//	cout << "Recv Call back" << endl;
//
//	ProcessThread(m_recv_over._send_buf, num_bytes);
//
//	RecvPacket();
//
//}
//
//
//void CALLBACK SendCallBack(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flag)
//{
//	cout << "Send Call back" << endl;
//	delete over;
//	return;
//}

int my_id;

void CProcess_packet(char* ptr) {
	cout << "CProcess Paceket Type: " << (int)ptr[1] << endl;
	
	switch ( ptr[1])
	{
	case SC_LOGIN_INFO:
	{
		SC_LOGIN_INFO_PACKET* recv_info_packet = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(ptr);
		my_id = recv_info_packet->id;

		my_info.m_id = recv_info_packet->id;
		my_info.m_x = recv_info_packet->x;
		my_info.m_y = recv_info_packet->y;
		my_info.m_z = recv_info_packet->z;

		my_info.m_state = OBJ_ST_RUNNING;
		cout << "Recv My Info - id: " << my_info.m_id << ", Pos( x : " << my_info.m_x << ", y : " << my_info.m_y << ", z :"
			<< my_info.m_z << endl;

		break;
	}// SC_LOGIN_INFO end
	case SC_ADD_PLAYER:
	{
		SC_ADD_PLAYER_PACKET* recv_add_packet = reinterpret_cast<SC_ADD_PLAYER_PACKET*>(ptr);
		int other_pl_id = recv_add_packet->id;

		if (other_pl_id < MAX_USER) {                // 다른 플레이어 추가
			other_players[other_pl_id].m_id = other_pl_id;

			other_players[other_pl_id].m_x = recv_add_packet->x;
			other_players[other_pl_id].m_y = recv_add_packet->y;
			other_players[other_pl_id].m_z = recv_add_packet->z;
			
			other_players[other_pl_id].m_state = OBJ_ST_RUNNING;

			cout << "Recv Other Players Info - id: " << other_players[other_pl_id].m_id
				<< ", Pos( x : " << other_players[other_pl_id].m_x << ", y : " << other_players[other_pl_id].m_y
				<< ", z : )" << other_players[other_pl_id].m_z << endl;
		}
		else if (MAX_USER <= other_pl_id && other_pl_id < MAX_USER + MAX_NPC) {
			int npc_id = other_pl_id - MAX_USER;

			npc_info[npc_id].m_id = other_pl_id;


			npc_info[npc_id].m_x = recv_add_packet->x;
			npc_info[npc_id].m_y = recv_add_packet->y;
			npc_info[npc_id].m_z = recv_add_packet->z;

			npc_info[npc_id].m_state = OBJ_ST_RUNNING;
		}
		else {
			cout << "Exceed Max User." << endl;
		}
		break;
	}// SC_ADD_PLAYER end
	case SC_MOVE_PLAYER: 
	{
		SC_MOVE_PLAYER_PACKET* recv_move_packet = reinterpret_cast<SC_MOVE_PLAYER_PACKET*>(ptr);
		int recv_id = recv_move_packet->id;
		
		if (recv_id == my_info.m_id) {
			// Player 이동
			my_info.m_x = recv_move_packet->x;
			my_info.m_y = recv_move_packet->y;
			my_info.m_z = recv_move_packet->z;
			cout << "My object moves to (" << my_info.m_x << ", " << my_info.m_y << ", " << my_info.m_z << ")." << endl;
		}
		else if (0 <= recv_id && recv_id < MAX_USER && recv_id != my_info.m_id) {
			// 상대 Object 이동
			other_players[recv_id].m_x = recv_move_packet->x;
			other_players[recv_id].m_y = recv_move_packet->y;
			other_players[recv_id].m_z = recv_move_packet->z;
			cout << "Player[" << recv_id << "]'s object moves to("
				<< other_players[recv_id].m_x << ", " << other_players[recv_id].m_y << ", " << other_players[recv_id].m_z << ")." << endl;

			cout << "[TEST] R_PACK POS: " << recv_move_packet->x << ", " << recv_move_packet->y << ", " << recv_move_packet->z << endl;
		}
		else if (MAX_USER <= recv_id && recv_id < MAX_USER + MAX_NPC) {
			// Npc 이동
			int npc_id = recv_id - MAX_USER;

			npc_info[npc_id].m_x = recv_move_packet->x;
			npc_info[npc_id].m_y = recv_move_packet->y;
			npc_info[npc_id].m_z = recv_move_packet->z;
			cout << "NPC[" << npc_id << "]'s object moves to("
				<< npc_info[npc_id].m_x << ", " << npc_info[npc_id].m_y << ", " << npc_info[npc_id].m_z << ")." << endl;
		}
		break;
	}// SC_MOVE_PLAYER end
	}
}


//void ProcessThread(char* net_buf, size_t iocp_byte) 
//{
//	cout << "ProcessThread" << endl;
//	char* ptr = net_buf;
//	static size_t in_packet_size = 0;
//	static size_t saved_packet_size = 0;
//	static char packet_buffer[BUF_SIZE];
//
//	cout << iocp_byte << endl;
//	while (0 != iocp_byte) {
//		if (0 == in_packet_size) in_packet_size = ptr[0];
//		if (iocp_byte + saved_packet_size >= in_packet_size) {
//			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
//			CProcess_packet(packet_buffer);
//			ptr += in_packet_size - saved_packet_size;
//			iocp_byte -= in_packet_size - saved_packet_size;
//			in_packet_size = 0;
//			saved_packet_size = 0;
//		}
//		else {
//			memcpy(packet_buffer + saved_packet_size, ptr, iocp_byte);
//			saved_packet_size += iocp_byte;
//			iocp_byte = 0;
//		}
//	}
//}








class Connect
{
public:
	int m_clientID = 0;
	SOCKET m_c_socket;


public:
	Connect();
	~Connect();

	/*bool Init();
	bool ConnectTo();

	void SendPacket(void* packet);*/

	
 
};

