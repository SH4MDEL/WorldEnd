#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <winsock.h>
#include <Windows.h>
#include <iostream>
#include <thread>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <chrono>
#include <queue>
#include <array>
#include <memory>
#include <unordered_map>


using namespace std;
using namespace chrono;

extern HWND		hWnd;

const static int MAX_TEST = 10000;
const static int MAX_CLIENTS = MAX_TEST * 2;
const static int INVALID_ID = -1;
const static int MAX_PACKET_SIZE = 255;
const static int MAX_BUFF_SIZE = 255;

constexpr int DELAY_LIMIT = 100;
constexpr int DELAY_LIMIT2 = 150;
constexpr int ACCEPT_DELY = 50;

#pragma comment (lib, "ws2_32.lib")

#include "..\..\WorldEnd\Server\protocol.h"

enum OPTYPE { OP_SEND, OP_RECV, OP_DO_MOVE };

void ErrorDisplay(const char* msg, int err_no);

extern int global_delay;
extern std::atomic_int g_active_clients;

struct ExpOver {
	WSAOVERLAPPED over;
	WSABUF wsabuf;
	unsigned char IOCP_buf[MAX_BUFF_SIZE];
	OPTYPE event_type;
	int event_target;
};

struct MONSTER {
	int id;
	int x;
	int y;
	
	high_resolution_clock::time_point last_move_time;
};




