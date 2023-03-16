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

	ExpOver();
	ExpOver(char* packet);
	~ExpOver() = default;
};

class Client : public MovementObject
{
public:
	Client();
	~Client();

	void DoRecv();
	virtual void DoSend(void* p) override;

	void SetSocket(const SOCKET& socket) { m_socket = socket; }
	void SetExpOver(const ExpOver& over) { m_recv_over = over; }
	void SetRemainSize(INT size) { m_remain_size = size; }
	void SetReadyCheck(bool ready_check) { m_ready_check = ready_check; }
	void SetPlayerType(PlayerType type);
	void SetSkillRatio(AttackType type, FLOAT ratio);

	const SOCKET& GetSocket() const override { return m_socket; }
	ExpOver& GetExpOver() { return m_recv_over; }
	INT GetRemainSize() const { return m_remain_size; }
	bool GetReadyCheck() const { return m_ready_check; }
	PlayerType GetPlayerType() const override { return m_player_type; }
	const BoundingOrientedBox& GetWeaponBoundingBox() const { return m_weopon_bounding_box; }
	virtual FLOAT GetSkillRatio(AttackType type) const override;

	PLAYER_DATA GetPlayerData() const override;

private:
	// 통신 관련 변수
	SOCKET					m_socket;
	ExpOver					m_recv_over;
	INT						m_remain_size;

	bool					m_ready_check;      // 로비에서 준비 

	PlayerType				m_player_type;      // 플레이어 종류
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

	array<INT, MAX_INGAME_USER>& GetMembers() { return m_members; }

	bool Join(INT player_id);

private:
	array<INT, MAX_INGAME_USER> m_members;
};

class PartyManager
{
public:
	PartyManager();
	~PartyManager() = default;

	// 빈자리 찾기, 몇번 파티에 가입하기 등등의 함수 필요

private:
	array<shared_ptr<Party>, MAX_PARTY_NUM> m_parties;
};