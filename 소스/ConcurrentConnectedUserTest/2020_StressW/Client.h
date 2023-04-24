#pragma once
#include "stdafx.h"

class Client
{
public:

	void DoSend(void* packet);
	void DoRecv();

	INT& GetID() { return m_id; }
	INT& GetX(){ return m_x; }
	INT& GetY(){ return m_y; }

	UCHAR* GetPacketBuffer() { return m_packet_buf; }
	SOCKET& GetSocket() { return m_client_socket; }
	ExpOver& GetExpOver() { return m_recv_over; }
	INT& GetPrevPacketData() { return m_prev_packet_data; }
	INT& GetCurrentPacketSize() { return m_curr_packet_size; }

	high_resolution_clock::time_point m_last_move_time;
	atomic_bool m_connect;                // atomic_bool변수 말고 그냥 bool변수를 private로 만들어 사용하는 방법도 있지만 
                                          // 멀티스레드 환경에서 안전하게 값을 변경하고 읽기 위해서 atomic_bool변수를 public으로 사용

private:
	INT m_id;
	INT m_x;
	INT m_y;

	UCHAR m_packet_buf[MAX_PACKET_SIZE];
	SOCKET m_client_socket;
	ExpOver m_recv_over;
	INT m_prev_packet_data;
	INT m_curr_packet_size;

};

