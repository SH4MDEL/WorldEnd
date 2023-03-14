#pragma once
#include "stdafx.h"
#include "object.h"

enum CompType { OP_RECV, OP_SEND, OP_ACCEPT, OP_COOLTIME_RESET };

class ExpOver {
public:
	WSAOVERLAPPED _wsa_over;
	WSABUF        _wsa_buf;
	char          _send_buf[BUF_SIZE];
	CompType     _comp_type;

	ExpOver(CompType comp_type, char num_bytes, void* mess) : _comp_type{ comp_type }
	{
		ZeroMemory(&_wsa_over, sizeof(_wsa_over));
		_wsa_buf.buf = reinterpret_cast<char*>(_send_buf);
		_wsa_buf.len = num_bytes;
		memcpy(_send_buf, mess, num_bytes);
	}

	ExpOver(CompType comp_type) : _comp_type{ comp_type } { }
	ExpOver() : _comp_type{ OP_RECV } { }
	~ExpOver() { }
};

class Client : public MovementObject
{
public:
	Client();
	~Client();

	void DoRecv();
	void DoSend();

	void SetSocket(const SOCKET& socket) { m_socket = socket; }
	void SetExpOver(const ExpOver& over) { m_recv_over = over; }
	void SetRemainSize(INT size) { m_remain_size = size; }
	void SetReadyCheck(bool ready_check) { m_ready_check = ready_check; }
	void SetPlayerType(PlayerType type);
	void SetSkillRatio(AttackType type, FLOAT ratio);

	const SOCKET& GetSocket() const { return m_socket; }
	ExpOver& GetExpOver() { return m_recv_over; }
	INT GetRemainSize() const { return m_remain_size; }
	bool GetReadyCheck() const { return m_ready_check; }
	PlayerType GetPlayerType() const { return m_player_type; }
	const BoundingOrientedBox& GetWeaponBoundingBox() const { return m_weopon_bounding_box; }
	FLOAT GetSkillRatio(AttackType type) const;

	PLAYER_DATA GetPlayerData() const;

private:
	// ��� ���� ����
	SOCKET					m_socket;
	ExpOver					m_recv_over;
	INT						m_remain_size;

	bool					m_ready_check;      // �κ񿡼� �غ� 

	PlayerType				m_player_type;      // �÷��̾� ����
	BoundingOrientedBox		m_weopon_bounding_box;

	FLOAT					m_skill_ratio;
	FLOAT					m_ultimate_ratio;

	void SetBoundingBox(PlayerType type);
};

class Party
{
public:
	Party();
	~Party() = default;

	array<INT, MAX_IN_GAME_USER>& GetMembers() { return m_members; }

	bool Join(INT player_id);

private:
	array<INT, MAX_IN_GAME_USER> m_members;
};

class PartyManager
{
public:
	PartyManager();
	~PartyManager() = default;

	// ���ڸ� ã��, ��� ��Ƽ�� �����ϱ� ����� �Լ� �ʿ�

private:
	array<shared_ptr<Party>, MAX_PARTY_NUM> m_parties;
};