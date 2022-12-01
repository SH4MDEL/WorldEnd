#include "stdafx.h"

const int MAX_THREAD = 12;

SOCKET g_s_socket;
HANDLE g_h_iocp;
int m_worker_num;

default_random_engine dre;
uniform_int_distribution<int> uid(1, 10);

enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND };

void err_display(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);
	printf("[%s] %s\n", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}


void error_display(int err_no)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, 0);
	std::wcout << lpMsgBuf << std::endl;
	LocalFree(lpMsgBuf);
}

class OVER_EXP {
public:
	WSAOVERLAPPED _over;
	WSABUF _wsabuf;
	char _send_buf[BUF_SIZE];
	COMP_TYPE _comp_type;
	OVER_EXP()
	{
		_wsabuf.len = BUF_SIZE;
		_wsabuf.buf = _send_buf;
		_comp_type = OP_RECV;
		ZeroMemory(&_over, sizeof(_over));
	}
	OVER_EXP(char* packet)
	{
		_wsabuf.len = packet[0];
		_wsabuf.buf = _send_buf;
		ZeroMemory(&_over, sizeof(_over));
		_comp_type = OP_SEND;
		memcpy(_send_buf, packet, packet[0]);
	}
};


OVER_EXP g_a_over;

enum S_STATE { ST_FREE, ST_ACCEPTED, ST_INGAME };
class SESSION {
	OVER_EXP _recv_over;

public:
	mutex _s_lock;
	S_STATE _state;
	int _id;
	SOCKET _socket;
	char	_name[NAME_SIZE];

	int		_prev_remain;

	XMFLOAT3 pos;

public:
	SESSION()
	{
		_id = -1;
		_socket = 0;
		pos = { 0.0f, 0.0f, 0.0f };
		_name[0] = 0;
		_state = ST_FREE;
		_prev_remain = 0;
	}

	~SESSION() {}

	void do_recv()
	{
		cout << "server do recv" << endl;
		DWORD recv_flag = 0;
		memset(&_recv_over._over,0, sizeof(_recv_over._over));
		_recv_over._wsabuf.len = BUF_SIZE - _prev_remain;
		_recv_over._wsabuf.buf = _recv_over._send_buf + _prev_remain;
		int ret = WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag,
			&_recv_over._over, NULL);
		if (ret != 0 && GetLastError() != WSA_IO_PENDING) {
			cout << "Recv Error - " << ret << endl;
			cout << GetLastError() << endl;
		}

	}

	void do_send(void* packet)
	{
		OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };

		cout << " do send id: " << _id << endl;
		int ret = WSASend(_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
		if (ret != 0) {
			cout << "Send Error - " << ret << endl;
			cout << GetLastError() << endl;
		}
	}
	void send_login_info_packet()
	{
		SC_LOGIN_INFO_PACKET login_info_packet;
		login_info_packet.id = _id;
		login_info_packet.size = sizeof(SC_LOGIN_INFO_PACKET);
		login_info_packet.type = SC_LOGIN_INFO;
		login_info_packet.x = pos.x;
		login_info_packet.y = pos.y;
		login_info_packet.z = pos.z;
		
		cout << "sc 로그인 인포" << endl;
		do_send(&login_info_packet);
	}

	void send_move_packet(int c_id);
};

array<SESSION, MAX_USER + 10> clients;

void SESSION::send_move_packet(int c_id)
{
	SC_MOVE_PLAYER_PACKET move_pl_packet;
	move_pl_packet.id = c_id;
	move_pl_packet.size = sizeof(SC_MOVE_PLAYER_PACKET);
	move_pl_packet.type = SC_MOVE_PLAYER;
	move_pl_packet.x = clients[c_id].pos.x;
	move_pl_packet.y = clients[c_id].pos.y;
	move_pl_packet.z = clients[c_id].pos.z;

	cout << "SC MOVE" << endl;
	do_send(&move_pl_packet);
}

void disconnect(int c_id)
{
	//for (auto& pl : clients) {
	//	{
	//		lock_guard<mutex> ll(pl._s_lock);
	//		if (ST_INGAME != pl._state) continue;
	//	}
	//	if (pl._id == c_id) continue;
	//	SC_REMOVE_PLAYER_PACKET p;
	//	p.id = c_id;
	//	p.size = sizeof(p);
	//	p.type = SC_REMOVE_PLAYER;
	//	pl.do_send(&p);
	//}
	//closesocket(clients[c_id]._socket);

	//lock_guard<mutex> ll(clients[c_id]._s_lock);
	//clients[c_id]._state = ST_FREE;
}

void Init_NPC() {
	for (int i = 0; i < MAX_USER + MAX_NPC; ++i)
		clients[i]._id = i;
	for (int i = 0; i < MAX_NPC; ++i) {
		int npc_id = i + MAX_USER;
		clients[npc_id]._state = ST_INGAME;
		clients[npc_id].pos.x = uid(dre) * 2;
		clients[npc_id].pos.y = uid(dre);
		clients[npc_id].pos.z = uid(dre) * 2;
		sprintf_s(clients[npc_id]._name, "NPC-No.%d",i);
	}

}

int Get_new_client_id()
{
	for (int i = 0; i < MAX_USER; ++i) {
		clients[i]._s_lock.lock();
		if (clients[i]._state == ST_FREE) {
			clients[i]._state = ST_ACCEPTED;
			clients[i]._s_lock.unlock();
			return i;
		}
		clients[i]._s_lock.unlock();
	}
	return -1;
}

void SProcess_packet(int c_id, char* packet)
{
	
	switch (packet[1]) {
	case CS_LOGIN: {
		
		CS_LOGIN_PACKET* login_packet = reinterpret_cast<CS_LOGIN_PACKET*>(packet);

		clients[c_id]._s_lock.lock();
		if (clients[c_id]._state == ST_FREE) {
			clients[c_id]._s_lock.unlock();
			break;
		}
		if (clients[c_id]._state == ST_INGAME) {
			clients[c_id]._s_lock.unlock();
			//disconnect(c_id);
			break;
		}
		cout << "프로세스 패킷" << endl;
		// 새로 접속한 플레이어의 초기 위취정보를 설정.
		clients[c_id].pos.x = c_id * 2;
		clients[c_id].pos.y = c_id;
		clients[c_id].pos.y = -(c_id * 2);
		cout << "초기 위치: " << " x: " << clients[c_id].pos.x << ", y: " << clients[c_id].pos.y
			<< ", z: " << clients[c_id].pos.z << endl;

		strcpy_s(clients[c_id]._name, login_packet->name);

		clients[c_id].send_login_info_packet();
		clients[c_id]._state = ST_INGAME;
		clients[c_id]._s_lock.unlock();

		cout << "PlayerID: " << clients[c_id]._id << ". name: " << clients[c_id]._name << endl;

		
		// 현재 접속해 있는 모든 클라이언트들에게 새로운 클라이언트의 정보를 전송
		for (int i = 0; i < MAX_USER; ++i) {
			auto& pl = clients[i];
			//cout << " cl = " << c_id << "pl = " << pl._id << endl;
			if (pl._id == c_id) continue;


			//cout << "스테이트 - " << pl._state << ST_INGAME << endl;
			pl._s_lock.lock();
			if (pl._state != ST_INGAME) {
				pl._s_lock.unlock();
				continue;
			}
			cout << "clear" << endl;
			SC_ADD_PLAYER_PACKET add_pl_packet;
			add_pl_packet.id = c_id;
			strcpy_s(add_pl_packet.name, login_packet->name);
			add_pl_packet.size = sizeof(add_pl_packet);
			add_pl_packet.type = SC_ADD_PLAYER;

			add_pl_packet.x = clients[c_id].pos.x;
			add_pl_packet.y = clients[c_id].pos.y;
			add_pl_packet.z = clients[c_id].pos.z;
			cout << " add2 " << endl;
			pl.do_send(&add_pl_packet);
			pl._s_lock.unlock();

		}
		// 새로 접속한 클라이언트에게 현재 접속해 있는 모든 클라이언트들의 정보를 전송
		for (auto& pl : clients) {
			if (pl._id == c_id) continue;
			cout << " cl = " << c_id << "pl = " << pl._id << endl;
			pl._s_lock.lock();
			if(pl._state != ST_INGAME){
				pl._s_lock.unlock();
				continue;
			}
			cout << "clear2" << endl;
			SC_ADD_PLAYER_PACKET add_pl_packet;
			add_pl_packet.id = c_id;
			strcpy_s(add_pl_packet.name, login_packet->name);
			add_pl_packet.size = sizeof(add_pl_packet);
			add_pl_packet.type = SC_ADD_PLAYER;
			add_pl_packet.x = clients[c_id].pos.x;
			add_pl_packet.y = clients[c_id].pos.y;
			add_pl_packet.z = clients[c_id].pos.z;
			
			cout << "Semd Client " << endl;
			clients[c_id].do_send(&add_pl_packet);
			pl._s_lock.unlock();
		}
		break;
	}
	case CS_MOVE: {
		clients[c_id]._s_lock.lock();
		cout << "cs move" << endl;
		CS_INPUT_KEY_PACKET* input_key_packet = reinterpret_cast<CS_INPUT_KEY_PACKET*>(packet);
		XMFLOAT3 move_dir{ 0,0,0 };
		move_dir.x = clients[c_id].pos.x;
		move_dir.y = clients[c_id].pos.y;
		move_dir.z = clients[c_id].pos.z;

		switch (input_key_packet->direction) {
		case INPUT_KEY_W: move_dir.x += 0.2; break;
		case INPUT_KEY_A: move_dir.z -= 0.2; break;
		case INPUT_KEY_S: move_dir.x -= 0.2; break;
		case INPUT_KEY_D: move_dir.z += 0.2; break;
		}
		clients[c_id].pos.x = move_dir.x;
		clients[c_id].pos.y = move_dir.y;

		clients[c_id]._s_lock.unlock();
		cout << "PlaeyrID: " << clients[c_id]._id << ", name: " << clients[c_id]._name <<
			" pos (" << clients[c_id].pos.x << ", " << clients[c_id].pos.y << ", " << clients[c_id].pos.z
			<< ") " << endl;
		// 작동 중인 모든 클라이언트에게 이동 결과를 알려준다.
		for (int i = 0; i < MAX_USER; ++i) {
			auto& pl = clients[i];
			lock_guard<mutex> ll(pl._s_lock);
			if (pl._state == ST_INGAME) 
				pl.send_move_packet(c_id);
		}
	

		break;
	}
}

}


void WorkerThread() 
{
	while (true) {

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


		switch (ex_over->_comp_type) {
		case OP_ACCEPT: {
			SOCKET c_socket = reinterpret_cast<SOCKET>(ex_over->_wsabuf.buf);
			int client_id = Get_new_client_id();
			if (client_id != -1) {
				clients[client_id].pos = { 0.0f,0.0f,0.0f };
				clients[client_id]._id = client_id;
				clients[client_id]._name[0] = 0;
				clients[client_id]._prev_remain = 0;
				clients[client_id]._socket = c_socket;
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket),
					g_h_iocp, client_id, 0);
				clients[client_id].do_recv();
				c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
				cout  << "5번 - " << clients[5]._id << endl;
			}
			else {
				cout << "Max user exceeded.\n";
			}
			ZeroMemory(&ex_over->_over, sizeof(ex_over->_over));
			ex_over->_wsabuf.buf = reinterpret_cast<CHAR*>(c_socket);
			int addr_size = sizeof(SOCKADDR_IN);
			AcceptEx(g_s_socket, c_socket, ex_over->_send_buf, 0, addr_size + 16, addr_size + 16, 0, &ex_over->_over);
			break;
		}
		case OP_RECV: {
			/*if (0 == num_bytes) {
				disconnect(static_cast<int>(key));
			}*/
			cout << "오프 리시브" << endl;
			int remain_data = num_bytes + clients[key]._prev_remain;
			char* p = ex_over->_send_buf;
			while (remain_data > 0) {
				int packet_size = p[0];
				if (packet_size <= remain_data) {
					SProcess_packet(static_cast<int>(key), p);
					p = p + packet_size;
					remain_data = remain_data - packet_size;
				}
				else break;
			}
			clients[key]._prev_remain = remain_data;
			if (remain_data > 0) {
				memcpy(ex_over->_send_buf, p, remain_data);
			}
			clients[key].do_recv();
			break;
		}
		case OP_SEND:
		/*	if (0 == num_bytes)
				disconnect(static_cast<int>(key));*/
			delete ex_over;
			break;
		}
	}


}


int main() {

	Init_NPC();

	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
		return 1;
	g_s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == g_s_socket) err_display("socket()");
	
	m_worker_num = MAX_THREAD;
	
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int retval = bind(g_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	if (retval == SOCKET_ERROR) err_display("socket()");
		

	retval = listen(g_s_socket, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_display("socket()");
	g_h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), g_h_iocp, 9999, 0);
	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_a_over._comp_type = OP_ACCEPT;
	g_a_over._wsabuf.buf = reinterpret_cast<CHAR*>(c_socket);
	AcceptEx(g_s_socket, c_socket, g_a_over._send_buf, 0, sizeof(SOCKADDR_IN) + 16,
			sizeof(SOCKADDR_IN) + 16, NULL, &g_a_over._over);

	vector <thread> m_worker_threads;
	for (int i = 0; i < 6; ++i)
		m_worker_threads.emplace_back(WorkerThread);
	for (auto& th : m_worker_threads)
		th.join();

	HANDLE hThread;

		// 스레드 생성
	//hThread = CreateThread(NULL, 0, ProcssClient,
	//	(LPVOID)m_s_socket, 0, NULL);
	//if (hThread == NULL) { closesocket(m_s_socket); }
	//else { CloseHandle(hThread); }

	


	closesocket(g_s_socket);
	WSACleanup();
}

