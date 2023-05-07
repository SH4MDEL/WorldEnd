#pragma once
#include "stdafx.h"
#include "object.h"

enum CompType
{
	OP_RECV, OP_SEND, OP_ACCEPT, OP_COOLDOWN_RESET, OP_MONSTER_REMOVE,
	OP_FLOOR_CLEAR, OP_FLOOR_FAIL, OP_BEHAVIOR_CHANGE, OP_AGRO_REDUCE,
	OP_ATTACK_COLLISION, OP_MONSTER_ATTACK_COLLISION, OP_STAMINA_CHANGE,
	OP_HIT_SCAN, OP_ARROW_SHOOT, OP_ARROW_REMOVE, OP_GAME_ROOM_RESET,
	OP_BATTLE_START, OP_PORTAL_WARP, OP_TRIGGER_COOLDOWN, OP_MULTIPLE_TRIGGER_SET
};

class ExpOver {
public:
	WSAOVERLAPPED _wsa_over;
	WSABUF        _wsa_buf;
	char          _send_buf[BUF_SIZE];
	CompType     _comp_type;

	ExpOver();
	ExpOver(char* packet);
	ExpOver(char* packet, INT packet_count);
	~ExpOver() = default;
};

class Client : public MovementObject
{
public:
	Client();
	~Client();

	void Init();

	void DoRecv();
	virtual void DoSend(void* p) override;
	virtual void DoSend(void* p, INT packet_count) override;

	void SetSocket(const SOCKET& socket) { m_socket = socket; }
	void SetExpOver(const ExpOver& over) { m_recv_over = over; }
	void SetRemainSize(INT size) { m_remain_size = size; }
	void SetReadyCheck(bool ready_check) { m_ready_check = ready_check; }
	void SetPlayerType(PlayerType type);
	void SetWeaponCenter(const XMFLOAT3& center);
	void SetWeaponOrientation(const XMFLOAT4& orientation);
	void SetStamina(FLOAT stamina);
	void SetLatestId(BYTE id);
	void SetInteractable(bool val);
	void SetCurrentAnimation(USHORT animation);
	void SetLastMoveTime(UINT time);

	const SOCKET& GetSocket() const override { return m_socket; }
	ExpOver& GetExpOver() { return m_recv_over; }
	INT GetRemainSize() const { return m_remain_size; }
	bool GetReadyCheck() const { return m_ready_check; }
	PlayerType GetPlayerType() const override { return m_player_type; }
	virtual FLOAT GetSkillRatio(ActionType type) const override;
	const BoundingOrientedBox& GetWeaponBoundingBox() const { return m_weapon_bounding_box; }
	FLOAT GetStamina() const { return m_stamina; }
	BYTE GetLatestId() const { return m_latest_id; }
	bool GetInteractable() const { return m_interactable; }
	virtual USHORT GetCurrentAnimation() const override { return m_current_animation; }
	virtual UINT GetLastMoveTime() const override { return m_last_move_time; }

	PLAYER_DATA GetPlayerData() const override;

	void ChangeStamina(FLOAT value);
	virtual void DecreaseHp(FLOAT damage, INT id) override;
	void RestoreCondition();

private:
	// 통신 관련 변수
	SOCKET					m_socket;
	ExpOver					m_recv_over;
	INT						m_remain_size;
	UINT					m_last_move_time;

	bool					m_ready_check;      // 로비에서 준비 

	PlayerType				m_player_type;      // 플레이어 종류
	BoundingOrientedBox		m_weapon_bounding_box;

	FLOAT					m_stamina;
	BYTE					m_latest_id;
	bool					m_interactable;
	USHORT					m_current_animation;

	void SetBoundingBox(PlayerType type);
};

class Party
{
public:
	Party();
	~Party() = default;

	std::array<INT, MAX_INGAME_USER>& GetMembers() { return m_members; }

	bool Join(INT player_id);

private:
	std::array<INT, MAX_INGAME_USER> m_members;
};

class PartyManager
{
public:
	PartyManager();
	~PartyManager() = default;

	// 빈자리 찾기, 몇번 파티에 가입하기 등등의 함수 필요

private:
	std::array<std::shared_ptr<Party>, MAX_PARTY_NUM> m_parties;
};