#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include "protocol.h"

// d3d12 헤더 파일입니다.
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>

using namespace std;

#pragma comment (lib, "WS2_32.LIB")
#pragma comment (lib, "MSWSock.LIB")


#define SERVERPORT 9000
#define MAX_PLAYERS 3

const int MAX_THREAD = 12;

//EXP_OVER	accept_ex;
SOCKET m_s_socket;
HANDLE m_hiocp;
int m_worker_num;

SOCKET client_sock;


const int BUF_SIZE = 2048;


enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND };

float sx, sy, sz;
int client_id;


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

OVER_EXP g_a_over;



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


void Worker() {
	WSABUF s_buf{};
	int retval;
	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	int addrlen;
	char buf[BUF_SIZE + 1];
	DWORD recv_flag = 0;
	WSAOVERLAPPED* s_over = new WSAOVERLAPPED;

	for (;;) {
		
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(m_hiocp, &num_bytes, &key, &over, INFINITE);
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);

		// 데이터 받기
		retval = WSARecv(client_sock, &s_buf, 1, 0, &recv_flag, s_over, 0);
		PLAYERINFO* p_packet = (PLAYERINFO*)&s_buf;
		sx = p_packet->dir;
		client_id = p_packet->id;


		
		if (retval == SOCKET_ERROR) {
			error_display(WSAGetLastError());
			break;
		}
		if (FALSE == ret) {
			if (ex_over->_comp_type == OP_ACCEPT) cout << "Accept Error";
			else {
				cout << "GQCS Error on client[" << key << "]\n";
				disconnect(static_cast<int>(key));
				if (ex_over->_comp_type == OP_SEND) delete ex_over;
				continue;
			}
		}

		switch (s_buf.buf[0])
		{
		case 'w':
			sx += p_packet->dir;
			cout << "[id]: " << client_id << " dir - " << sx << endl;
			break;
		case 's':
			sx += p_packet->dir;
			break;
		case 'a':
			sx += p_packet->dir;
			break;
		case 'd':
			sx += p_packet->dir;
			break;
		default:
			break;
		}

		cout << "[id]: " << client_id << " dir - " << sx << endl;
		/*switch (ex_over->_comp_type) {
		case OP_ACCEPT: {
			int client_id = get_new_client_id();
			if (client_id != -1) {
				{
					lock_guard<mutex> ll(clients[client_id]._s_lock);
					clients[client_id]._state = ST_ALLOC;
				}
				clients[client_id].x = 0;
				clients[client_id].y = 0;
				clients[client_id]._id = client_id;
				clients[client_id]._name[0] = 0;
				clients[client_id]._prev_remain = 0;
				clients[client_id]._socket = g_c_socket;
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_c_socket),
					h_iocp, client_id, 0);
				clients[client_id].do_recv();
				g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			}
			else {
				cout << "Max user exceeded.\n";
			}
			ZeroMemory(&g_a_over._over, sizeof(g_a_over._over));
			int addr_size = sizeof(SOCKADDR_IN);
			AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);
			break;
		}
		case OP_RECV: {
			if (0 == num_bytes) {
				disconnect(static_cast<int>(key));
				if (OP_SEND == ex_over->_comp_type)
					delete ex_over;
			}
			int remain_data = num_bytes + clients[key]._prev_remain;
			char* p = ex_over->_send_buf;
			while (remain_data > 0) {
				int packet_size = p[0];
				if (packet_size <= remain_data) {
					process_packet(static_cast<int>(key), p);
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
			if (0 == num_bytes)
				disconnect(static_cast<int>(key));
			delete ex_over;
			break;
		}*/
	}
	//cout << "값 - " << &s_buf << endl;

	


	/*while (1) {

	

		cout << "값 - " << &s_buf << endl;

	}*/


}



DWORD WINAPI ProcssClient(LPVOID arg) {

}



int main() {

	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
		return 1;
	m_s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == m_s_socket) err_display("socket()");
	
	m_worker_num = MAX_THREAD;
	
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVERPORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int retval = bind(m_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	if (retval == SOCKET_ERROR) err_display("socket()");
		

	retval = listen(m_s_socket, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_display("socket()");

	m_hiocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_s_socket), m_hiocp, 0, 0);
	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, 0, 0, WSA_FLAG_OVERLAPPED);
	g_a_over._comp_type = OP_ACCEPT;

	AcceptEx(m_s_socket, c_socket, g_a_over._send_buf, 0, sizeof(SOCKADDR_IN) + 16,
			sizeof(SOCKADDR_IN) + 16, NULL, &g_a_over._over);
		
	vector <thread> m_worker_threads;
	for (int i = 0; i < 6; ++i)
		m_worker_threads.emplace_back(Worker);
	for (auto& th : m_worker_threads)
		th.join();

	HANDLE hThread;

		// 스레드 생성
	//hThread = CreateThread(NULL, 0, ProcssClient,
	//	(LPVOID)m_s_socket, 0, NULL);
	//if (hThread == NULL) { closesocket(m_s_socket); }
	//else { CloseHandle(hThread); }

	


	closesocket(m_s_socket);
	WSACleanup();
}

