#pragma once
#include "stdafx.h"
#include "object.h"
#include "skill.h"

enum CompType
{
	OP_RECV, OP_SEND, OP_ACCEPT, OP_COOLDOWN_RESET, OP_MONSTER_REMOVE,
	OP_FLOOR_CLEAR, OP_FLOOR_FAIL, OP_BEHAVIOR_CHANGE, OP_AGRO_REDUCE,
	OP_ATTACK_COLLISION, OP_MONSTER_ATTACK_COLLISION, OP_STAMINA_CHANGE,
	OP_HIT_SCAN, OP_ARROW_SHOOT, OP_ARROW_REMOVE, OP_GAME_ROOM_RESET,
	OP_BATTLE_START, OP_PORTAL_WARP, OP_TRIGGER_COOLDOWN, OP_MULTIPLE_TRIGGER_SET,
	OP_LOGIN_OK, OP_LOGIN_FAIL, OP_SIGNIN_OK, OP_SIGNIN_FAIL
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
	void SetPlayerType(PlayerType type);
	void SetWeaponCenter(const XMFLOAT3& center);
	void SetWeaponOrientation(const XMFLOAT4& orientation);
	void SetStamina(FLOAT stamina);
	void SetLatestId(BYTE id);
	void SetInteractable(bool val);
	void SetCurrentAnimation(USHORT animation);
	void SetLastMoveTime(UINT time);
	void SetPartyNum(SHORT party_num);
	void SetReady(bool value);
	void SetUserId(const std::wstring_view& ws);
	void SetGold(INT gold);
	void SetHpLevel(UCHAR level);
	void SetAtkLevel(UCHAR level);
	void SetDefLevel(UCHAR level);
	void SetCritRateLevel(UCHAR level);
	void SetCritDamageLevel(UCHAR level);
	void SetNormalSkillType(PlayerType player_type, UCHAR type);
	void SetUltimateSkillType(PlayerType player_type, UCHAR type);
	void SetInvincibleRoll(bool invincible_roll) { m_invincible_roll = invincible_roll; }
	void SetViewList(int data) { m_view_list.insert(data);}
	void SetTownPosition(const XMFLOAT3& position);
	void SetTownPosition(FLOAT x, FLOAT y, FLOAT z);

	const SOCKET& GetSocket() const override { return m_socket; }
	ExpOver& GetExpOver() { return m_recv_over; }
	INT GetRemainSize() const { return m_remain_size; }
	PlayerType GetPlayerType() const override { return m_player_type; }
	virtual FLOAT GetSkillRatio(ActionType type) const override;
	const BoundingOrientedBox& GetWeaponBoundingBox() const { return m_weapon_bounding_box; }
	FLOAT GetStamina() const { return m_stamina; }
	BYTE GetLatestId() const { return m_latest_id; }
	bool GetInteractable() const { return m_interactable; }
	virtual USHORT GetCurrentAnimation() const override { return m_current_animation; }
	virtual UINT GetLastMoveTime() const override { return m_last_move_time; }
	SHORT GetPartyNum() const { return m_party_num; }
	bool GetReady() const { return m_is_ready; }
	std::wstring GetUserId() const { return m_user_id; }
	INT GetGold() const { return m_gold; }
	UCHAR GetHpLevel() const { return m_status->GetHpLevel(); }
	UCHAR GetAtkLevel() const { return m_status->GetAtkLevel(); }
	UCHAR GetDefLevel() const { return m_status->GetDefLevel(); }
	UCHAR GetCritRateLevel() const { return m_status->GetCritRateLevel(); }
	UCHAR GetCritDamageLevel() const { return m_status->GetCritDamageLevel(); }
	UCHAR GetNormalSkillType(PlayerType type) const;
	UCHAR GetUltimateSkillType(PlayerType type) const;
	XMFLOAT3 GetTownPosition() const { return m_town_position; }
	INT GetCost(EnhancementType type) const;

	bool GetInvincibleRoll() const { return m_invincible_roll; }
	//const std::unordered_set<INT>& GetViewList() { return m_view_list;}

	void ChangeStamina(FLOAT value);
	void ChangeGold(INT value);
	void LevelUpEnhancement(EnhancementType type);
	void ChangeSkill(PlayerType player_type, USHORT skill_type, USHORT changed_type);
	virtual void DecreaseHp(FLOAT damage, INT id) override;
	void RestoreCondition();

	std::mutex m_vl;                             // 뷰 리스트 전용 락
	std::unordered_set<INT> m_view_list;

private:
	// 통신 관련 변수
	SOCKET					m_socket;
	ExpOver					m_recv_over;
	INT						m_remain_size;
	UINT					m_last_move_time;

	SHORT					m_party_num;
	bool					m_is_ready;      // 로비에서 준비 

	XMFLOAT3				m_town_position;
	PlayerType				m_player_type;      // 플레이어 종류
	BoundingOrientedBox		m_weapon_bounding_box;

	FLOAT					m_stamina;
	BYTE					m_latest_id;
	bool					m_interactable;
	USHORT					m_current_animation;

	std::wstring			m_user_id;
	INT						m_gold;
	bool                    m_invincible_roll = false;

	//std::unordered_set<INT> m_view_list;         // 이 클라의 뷰 리스트
	

	std::array<std::array<std::shared_ptr<Skill>, static_cast<INT>(SkillType::COUNT)>,
		static_cast<INT>(PlayerType::COUNT)> m_skills;

	void SetBoundingBox(PlayerType type);
};

enum class PartyState : char { EMPTY, ACCEPT, FULL };

class Party : public std::enable_shared_from_this<Party>
{
public:
	Party();
	~Party() = default;

	void Init();
	void Reset();

	void SetState(PartyState state) { m_state = state; }
	void SetHost(INT id) { m_host_id = id; }

	std::array<INT, MAX_INGAME_USER>& GetMembers() { return m_members; }
	PartyState GetState() const { return m_state; }
	std::mutex& GetStateMutex() { return m_state_lock; }
	INT GetHostId() const { return m_host_id; }
	CHAR GetMemberCount() const { return m_member_count; }

	bool TryJoin(INT player_id);
	void Exit(INT player_id);
	void PlayerReady(INT player_id);
	void AddMember(INT player_id);
	bool IsExist();

	void SendChangeCharacter(INT player_id);
	void SendEnterDungeon();

private:
	void SendJoinOk(INT receiver);
	void SendAddMember(INT sender, INT receiver, INT locate_num);
	void SendChangeHost(INT receiver);
	void SendExitParty(INT exited_id, INT located_num);
	void SendReady(INT sender);
	void SendEnterFail();

private:
	std::array<INT, MAX_INGAME_USER>	m_members;
	std::mutex							m_member_lock;
	PartyState							m_state;
	std::mutex							m_state_lock;
	INT									m_host_id;		// 파티장 id
	CHAR								m_member_count;
};

class PartyManager
{
public:
	PartyManager();
	~PartyManager() = default;

	INT GetHostId(INT party_num);
	std::shared_ptr<Party>& GetParty(INT party_num);

	bool CreateParty(INT party_num, INT player_id);
	bool JoinParty(INT party_num, INT player_id);
	void ExitParty(INT party_num, INT player_id);
	void ChangeCharacter(INT party_num, INT player_id);
	void PlayerReady(INT party_num, INT player_id);

	void OpenPartyUI(INT player_id);
	void ClosePartyUI(INT player_id);
	void ChangePage(INT player_id, INT page);
	void SendPartyPage(INT player_id, INT page);

private:
	INT FindEmptyParty();
	void SendCreateOk(INT receiver);
	void SendCreateFail(INT receiver);
	void SendJoinFail(INT receiver);

	INT GetStartNum(INT page);

private:
	std::array<std::shared_ptr<Party>, MAX_PARTY_NUM>	m_parties;
	std::unordered_map<INT, INT>						m_looking_clients;	// ui 열고있는 클라이언트,  <id, page>
	std::mutex											m_lock;
};