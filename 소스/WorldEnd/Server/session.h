#pragma once
#include "stdafx.h"

enum COMP_TYPE { OP_RECV, OP_SEND, OP_ACCEPT};

class ExpOver {
public:
	WSAOVERLAPPED _wsa_over;
	WSABUF        _wsa_buf;
	char          _send_buf[BUF_SIZE];
	COMP_TYPE     _comp_type;

	ExpOver(COMP_TYPE comp_type, char num_bytes, void* mess) : _comp_type{ comp_type }
	{
		ZeroMemory(&_wsa_over, sizeof(_wsa_over));
		_wsa_buf.buf = reinterpret_cast<char*>(_send_buf);
		_wsa_buf.len = num_bytes;
		memcpy(_send_buf, mess, num_bytes);
	}

	ExpOver(COMP_TYPE comp_type) : _comp_type{ comp_type } { }
	ExpOver() : _comp_type{ OP_RECV } { }
	~ExpOver() { }
};

enum class STATE { ST_FREE, ST_ACCEPT, ST_INGAME };

class Session
{
public:
	// 통신 관련 변수
	SOCKET				m_socket;
	std::mutex			m_lock;
	STATE				m_state;
	ExpOver			    m_recv_over;
	INT					m_prev_size;


	// 게임 관련 변수
	PlayerData			m_player_data;	    // 클라이언트로 보낼 데이터 구조체
	CHAR				m_name[NAME_SIZE];	// 닉네임
	bool                m_ready_check;      // 로비에서 준비 
	PlayerType         m_player_type;      // 플레이어 종류
	BoundingOrientedBox                     m_boundingbox;
	XMFLOAT4X4				                m_worldMatrix;	
public:
	Session();
	~Session();

	void DoRecv();
	void DoSend();

	DirectX::BoundingOrientedBox GetBoundingBox() const;
};

