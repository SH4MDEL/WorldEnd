#include "client.h"
#include "stdafx.h"
#include "object.h"
#include "Server.h"

ExpOver::ExpOver()
{
	_wsa_buf.len = BUF_SIZE;
	_wsa_buf.buf = _send_buf;
	_comp_type = OP_RECV;
	ZeroMemory(&_wsa_over, sizeof(_wsa_over));
}

ExpOver::ExpOver(char* packet)
{
	ZeroMemory(&_wsa_over, sizeof(_wsa_over));
	_wsa_buf.buf = _send_buf;
	_wsa_buf.len = packet[0];
	_comp_type = OP_SEND;
	memcpy(_send_buf, packet, packet[0]);
}

ExpOver::ExpOver(char* packet, INT packet_count)
{
	ZeroMemory(&_wsa_over, sizeof(_wsa_over));
	_wsa_buf.buf = _send_buf;
	_wsa_buf.len = packet[0] * packet_count;
	_comp_type = OP_SEND;
	memcpy(_send_buf, packet, static_cast<size_t>(packet[0] * packet_count));
}

Client::Client() : m_socket{}, m_recv_over{}
{
	for (size_t j = 0; j < static_cast<INT>(PlayerType::COUNT); ++j) {
		for (size_t i = 0; i < static_cast<INT>(SkillType::COUNT); ++i) {
			m_skills[j][i] = std::make_shared<Skill>();
		}
	}

	Init();
}

Client::~Client()
{
}

void Client::Init()
{
	m_id = -1;
	m_room_num = -1;
	m_party_num = -1;
	m_position = XMFLOAT3(0.f, 0.f, 0.f);
	m_velocity = XMFLOAT3(0.f, 0.f, 0.f);
	m_yaw = 0.f;
	m_stamina = PlayerSetting::MAX_STAMINA;
	m_interactable = false;
	m_latest_id = 0;
	m_last_move_time = 0;
	m_party_num = -1;
	this->SetTriggerFlag();
	m_user_id.clear();

	m_current_animation = ObjectAnimation::IDLE;
	m_is_invincible = false;
	m_invincible_roll = false;
	m_remain_size = 0;

	// 나중에 DB에서 처리될 것들
	m_town_position = XMFLOAT3{ 25.f, 5.65f, 66.f };
	SetPlayerType(PlayerType::WARRIOR);
	m_name = std::wstring{ L"Player" };

	m_status->SetAtk(PlayerSetting::DEFAULT_ATK);
	m_status->SetMaxHp(PlayerSetting::DEFAULT_HP);
	m_status->SetHp(PlayerSetting::DEFAULT_HP);
	m_status->SetDef(PlayerSetting::DEFAULT_DEF);
	m_status->SetCritRate(PlayerSetting::DEFAULT_CRIT_RATE);
	m_status->SetCritDamage(PlayerSetting::DEFAULT_CRIT_DAMAGE);

	for (int i = 0; i < (INT)PlayerType::COUNT; ++i) {
		m_skills[i][(INT)SkillType::NORMAL]->SetSkillType(0);
		m_skills[i][(INT)SkillType::ULTIMATE]->SetSkillType(0);
	}

}

void Client::DoRecv()
{
	DWORD recv_flag = 0;
	ZeroMemory(&m_recv_over._wsa_over, sizeof(m_recv_over._wsa_over));
	m_recv_over._wsa_buf.buf = reinterpret_cast<char*>(m_recv_over._send_buf + m_remain_size);
	m_recv_over._wsa_buf.len = BUF_SIZE - m_remain_size;
	int ret = WSARecv(m_socket, &m_recv_over._wsa_buf, 1, 0, &recv_flag, &m_recv_over._wsa_over, NULL);
	if (SOCKET_ERROR == ret){
		int error_num = WSAGetLastError();
		if (ERROR_IO_PENDING != error_num){
			//g_server.Disconnect(data.id);
			if (error_num == WSAECONNRESET)
				printf("%d Client Disconnect\n", m_id);
			else ErrorDisplay("do_recv");
		}
	}
}

void Client::DoSend(void* p)
{
	ExpOver* ex_over = new ExpOver{ reinterpret_cast<char*>(p) };
	int retval = WSASend(m_socket, &ex_over->_wsa_buf, 1, 0, 0, &ex_over->_wsa_over, nullptr);
	if (SOCKET_ERROR == retval) {
		int error_num = WSAGetLastError();
		if (ERROR_IO_PENDING != error_num) {
			ErrorDisplay("Send Error");

			if (WSAECONNRESET == error_num) {
				Server& server = Server::GetInstance();
				server.Disconnect(m_id);
			}
		}
	}
}

void Client::DoSend(void* p, INT packet_count)
{
	ExpOver* ex_over = new ExpOver{ reinterpret_cast<char*>(p), packet_count };
	int retval = WSASend(m_socket, &ex_over->_wsa_buf, 1, 0, 0, &ex_over->_wsa_over, nullptr);
	if (SOCKET_ERROR == retval) {
		int error_num = WSAGetLastError();
		if (ERROR_IO_PENDING != error_num) {
			ErrorDisplay("Send Error");

			if (WSAECONNRESET == error_num) {
				Server& server = Server::GetInstance();
				server.Disconnect(m_id);
			}
		}
	}
}

void Client::SetPlayerType(PlayerType type)
{
	m_player_type = type;
	SetBoundingBox(type);
}

void Client::SetWeaponCenter(const XMFLOAT3& center)
{
	m_weapon_bounding_box.Center = center;
}

void Client::SetWeaponOrientation(const XMFLOAT4& orientation)
{
	m_weapon_bounding_box.Orientation = orientation;
}

void Client::SetStamina(FLOAT stamina)
{
	m_stamina = stamina;
}

void Client::SetLatestId(BYTE id)
{
	m_latest_id = id;
}

void Client::SetInteractable(bool val)
{
	m_interactable = val;
}

void Client::SetCurrentAnimation(USHORT animation)
{
	m_current_animation = animation;
}

void Client::SetLastMoveTime(UINT time)
{
	m_last_move_time = time;
}

void Client::SetPartyNum(SHORT party_num)
{
	m_party_num = party_num;
}

void Client::SetUserId(const std::wstring_view& ws)
{
	m_user_id = ws.data();
}

void Client::SetGold(INT gold)
{
	m_gold = gold;
}

void Client::SetHpLevel(UCHAR level)
{
	m_status->SetHpLevel(level);
}

void Client::SetAtkLevel(UCHAR level)
{
	m_status->SetAtkLevel(level);
}

void Client::SetDefLevel(UCHAR level)
{
	m_status->SetDefLevel(level);
}

void Client::SetCritRateLevel(UCHAR level)
{
	m_status->SetCritRateLevel(level);
}

void Client::SetCritDamageLevel(UCHAR level)
{
	m_status->SetCritDamageLevel(level);
}

void Client::LevelUpEnhancement(EnhancementType type)
{
	switch (type) {
	case EnhancementType::HP:
		m_status->SetHpLevel(m_status->GetHpLevel() + 1);
		break;
	case EnhancementType::ATK:
		m_status->SetAtkLevel(m_status->GetAtkLevel() + 1);
		break;
	case EnhancementType::DEF:
		m_status->SetDefLevel(m_status->GetDefLevel() + 1);
		break;
	case EnhancementType::CRIT_RATE:
		m_status->SetCritRateLevel(m_status->GetCritRateLevel() + 1);
		break;
	case EnhancementType::CRIT_DAMAGE:
		m_status->SetCritDamageLevel(m_status->GetCritDamageLevel() + 1);
		break;
	}
}

void Client::ChangeSkill(PlayerType player_type, USHORT skill_type, USHORT changed_type)
{
	m_skills[(INT)player_type][(INT)skill_type]->SetSkillType(changed_type);
}

void Client::SetNormalSkillType(PlayerType player_type, UCHAR type)
{
	m_skills[(INT)player_type][(INT)SkillType::NORMAL]->SetSkillType(type);
}

void Client::SetUltimateSkillType(PlayerType player_type, UCHAR type)
{
	m_skills[(INT)player_type][(INT)SkillType::ULTIMATE]->SetSkillType(type);
}

void Client::SetTownPosition(const XMFLOAT3& position)
{
	m_town_position = position;
}

void Client::SetTownPosition(FLOAT x, FLOAT y, FLOAT z)
{
	SetTownPosition(XMFLOAT3{ x, y, z });
}

void Client::SetInvincible(bool value)
{
	m_is_invincible = value;
}

FLOAT Client::GetSkillRatio(ActionType type) const
{
	FLOAT ratio{};
	switch (type) {
	case ActionType::SKILL: {
		INT skill_type = GetNormalSkillType(m_player_type);
		ratio = PlayerSetting::SKILL_RATIO[static_cast<int>(m_player_type)][skill_type];
		break;
	}
	case ActionType::ULTIMATE: {
		INT skill_type = GetUltimateSkillType(m_player_type);
		ratio = PlayerSetting::ULTIMATE_RATIO[static_cast<int>(m_player_type)][skill_type];
		break;
	}
	default:
		return 1.f;
	}

	return ratio;
}

UCHAR Client::GetNormalSkillType(PlayerType type) const
{
	return m_skills[(INT)type][static_cast<INT>(SkillType::NORMAL)]->GetSkillType();
}

UCHAR Client::GetNormalSkillType() const
{
	return GetNormalSkillType(m_player_type);
}

UCHAR Client::GetUltimateSkillType(PlayerType type) const
{
	return m_skills[(INT)type][static_cast<INT>(SkillType::ULTIMATE)]->GetSkillType();
}

UCHAR Client::GetUltimateSkillType() const
{
	return GetUltimateSkillType(m_player_type);
}

INT Client::GetCost(EnhancementType type) const
{
	INT cost{ PlayerSetting::DEFAULT_ENHANCE_COST };

	switch (type) {
	case EnhancementType::HP:
		cost += PlayerSetting::ENHANCE_COST_INCREASEMENT * m_status->GetHpLevel();
		break;
	case EnhancementType::ATK:
		cost += PlayerSetting::ENHANCE_COST_INCREASEMENT * m_status->GetAtkLevel();
		break;
	case EnhancementType::DEF:
		cost += PlayerSetting::ENHANCE_COST_INCREASEMENT * m_status->GetDefLevel();
		break;
	case EnhancementType::CRIT_RATE:
		cost += PlayerSetting::ENHANCE_COST_INCREASEMENT * m_status->GetCritRateLevel();
		break;
	case EnhancementType::CRIT_DAMAGE:
		cost += PlayerSetting::ENHANCE_COST_INCREASEMENT * m_status->GetCritDamageLevel();
		break;
	}

	return cost;
}

INT Client::GetLevel(EnhancementType type) const
{
	return static_cast<INT>(m_status->GetLevel(type));
}

void Client::ChangeStamina(FLOAT value)
{
	m_stamina += value;
	if (m_stamina <= 0)
		m_stamina = 0;
}

void Client::IncreaseGold(INT value)
{
	m_gold += value;
}

DecreaseState Client::DecreaseHp(FLOAT damage, INT id)
{
	if (m_is_invincible)
		return DecreaseState::NONE;

	if (m_invincible_roll)
		return DecreaseState::NONE;

	if (m_status->GetHp() <= std::numeric_limits<FLOAT>::epsilon())
		return DecreaseState::NONE;

	if (m_invincible_roll == false) 
	{
		bool death = m_status->CalculateHitDamage(damage);

		if (death) {

			// INGAME 에서 State를 바꾸는 것에는 경합이 필요 없으므로 lock 걸지 않음
			m_state = State::DEATH;
			m_current_animation = ObjectAnimation::DEATH;
		}
	}

	return DecreaseState::DECREASE;
}

// 다음 방 넘어갈 때 상태 회복
void Client::RestoreCondition()
{
	m_state = State::INGAME;
	m_current_animation = ObjectAnimation::IDLE;
	m_status->Init();
	m_stamina = PlayerSetting::MAX_STAMINA;
	m_interactable = false;
}

void Client::ToggleInvinsible()
{
	m_is_invincible = (m_is_invincible) ? false : true;
}

void Client::SetBoundingBox(PlayerType type)
{
	switch (type) {
	case PlayerType::WARRIOR:
		m_bounding_box = BoundingOrientedBox{ m_position, XMFLOAT3{0.37f, 0.65f, 0.37f}, XMFLOAT4{0.f, 0.f, 0.f, 1.f } };
		m_weapon_bounding_box = BoundingOrientedBox{ m_position, XMFLOAT3{0.13f, 0.03f, 0.68f}, XMFLOAT4{0.f, 0.f, 0.f, 1.f } };
		break;

	case PlayerType::ARCHER:
		m_bounding_box = BoundingOrientedBox{ m_position, XMFLOAT3{0.37f, 0.65f, 0.37f}, XMFLOAT4{0.f, 0.f, 0.f, 1.f } };
		m_weapon_bounding_box = BoundingOrientedBox{ m_position, XMFLOAT3{0.18f, 0.04f, 0.68f}, XMFLOAT4{0.f, 0.f, 0.f, 1.f } };
		// 화살 바운드 박스로 교체 필요함, 활은 충돌하지 않을 것이기 때문
		break;
	}
}

Party::Party()
{
	Init();
}

void Party::Init()
{
	{
		std::lock_guard<std::mutex> l{ m_member_lock };
		for (INT& id : m_members) {
			if (-1 == id) continue;

			id = -1;
		}
	}

	m_host_id = -1;
	m_member_count = static_cast<BYTE>(0);
	m_state = PartyState::EMPTY;
}

void Party::Reset()
{
	Server& server = Server::GetInstance();

	for (INT& id : m_members) {
		if (-1 == id) continue;

		auto client = dynamic_pointer_cast<Client>(server.m_clients[id]);
		client->SetPartyNum(-1);
		id = -1;
	}

	m_host_id = -1;
	m_member_count = static_cast<BYTE>(0);
	m_state = PartyState::EMPTY;
}

bool Party::TryJoin(INT player_id)
{
	std::unique_lock<std::mutex> lock{ m_member_lock };
	for (INT& id : m_members) {
		if (-1 == id) {
			id = player_id;
			m_member_count += 1;

			if (-1 == m_host_id) {
				m_host_id = player_id;
				return true;
			}
			else {
				lock.unlock();
				SendJoinOk(player_id);
			}

			if (MAX_INGAME_USER == static_cast<INT>(m_member_count)) {
				std::lock_guard<std::mutex> l{ m_state_lock };
				m_state = PartyState::FULL;
			}

			return true;
		}
	}
	return false;
}

void Party::Exit(INT player_id)
{
	Server& server = Server::GetInstance();
	auto client = dynamic_pointer_cast<Client>(server.m_clients[player_id]);
	client->SetPartyNum(-1);

	std::unique_lock<std::mutex> l{ m_member_lock };

	// 파티장이면
	if (m_host_id == player_id) {
		// 파티원이 있으면

		if (m_member_count > 1) {
			auto it = std::find(m_members.begin(), m_members.end(), player_id);
			INT located_num{};

			if (it != m_members.end()) {
				located_num = it - m_members.begin();
				*it = -1;
				m_member_count -= 1;

				// 파티장 위임
				auto host = std::find_if(m_members.begin(), m_members.end(), [](INT id) {
					return id != -1;
					});
				m_host_id = *host;
			}
			l.unlock();

			// 누구나 진입할 수 있어서 보낼 필요 없음
			//// 방장임을 전달
			//// 방장 변경 필요
			//if (-1 != m_host_id) {
			//	SendChangeHost(m_host_id);
			//}
			
			// 남아있는 파티원에게 나갔다고 전송
			SendExitParty(player_id, located_num);
		}
		// 파티원이 없으면 파티 초기화
		else {
			l.unlock(); 

			Init();
			return;
		}
	}
	// 파티원이면
	else {
		auto it = std::find(m_members.begin(), m_members.end(), player_id);
		INT located_num{};

		if (it != m_members.end()) {
			located_num = it - m_members.begin();
			*it = -1;
			m_member_count -= 1;

		}
		l.unlock();

		SendExitParty(player_id, located_num);
	}

	{
		std::lock_guard<std::mutex> lock{ m_state_lock };
		m_state = PartyState::ACCEPT;
	}
}

void Party::SendChangeCharacter(INT player_id)
{
	Server& server = Server::GetInstance();

	SC_CHANGE_CHARACTER_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_CHANGE_CHARACTER;
	packet.id = player_id;
	packet.player_type = server.m_clients[player_id]->GetPlayerType();

	for (INT id : m_members) {
		if (-1 == id) continue;

		server.m_clients[id]->DoSend(&packet);
	}
}

void Party::AddMember(INT player_id)
{
	std::array<INT, MAX_INGAME_USER> ids{};
	{
		std::lock_guard<std::mutex> l{ m_member_lock };
		ids = m_members;
	}

	INT located_num{ -1 };
	for (size_t i = 0; INT id : ids) {
		if (id == player_id) {
			located_num = i;
		}
		++i;
	}

	if (-1 == located_num)
		return;

	for (size_t i = 0; INT id : ids) {
		if (-1 == id) continue;

		// 기존 접속한 플레이어에게 신규 접속한 플레이어 전송
		SendAddMember(player_id, id, located_num);

		// 신규 접속한 플레이어에게 기존 접속해 있던 플레이어 전송
		SendAddMember(id, player_id, i);
		++i;
	}
}

bool Party::IsExist()
{
	return (PartyState::EMPTY != m_state) ? false : true;
}

void Party::SendJoinOk(INT receiver)
{
	SC_JOIN_OK_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_JOIN_OK;

	Server& server = Server::GetInstance();
	server.m_clients[receiver]->DoSend(&packet);
}

void Party::SendAddMember(INT sender, INT receiver, INT locate_num)
{
	Server& server = Server::GetInstance();
	auto client = std::dynamic_pointer_cast<Client>(server.m_clients[sender]);

	SC_ADD_PARTY_MEMBER_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_ADD_PARTY_MEMBER;
	packet.player_type = client->GetPlayerType();
	packet.id = sender;
	packet.locate_num = static_cast<UCHAR>(locate_num);

	std::string name{};
	std::wstring ws{ client->GetUserId() };
	name.assign(ws.begin(), ws.end());
	strcpy_s(packet.name, name.c_str());

	server.m_clients[receiver]->DoSend(&packet);
}

void Party::SendChangeHost(INT receiver)
{
	SC_CHANGE_HOST_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_CHANGE_HOST;

	Server& server = Server::GetInstance();
	server.m_clients[receiver]->DoSend(&packet);
}

void Party::SendExitParty(INT exited_id, INT located_num)
{
	SC_REMOVE_PARTY_MEMBER_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_REMOVE_PARTY_MEMBER;
	packet.id = exited_id;
	packet.locate_num = located_num;

	Server& server = Server::GetInstance();

	for (INT id : m_members) {
		if (-1 == id) continue;

		server.m_clients[id]->DoSend(&packet);
	}
}

void Party::SendEnterDungeon()
{
	Server& server = Server::GetInstance();

	SC_ENTER_DUNGEON_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_ENTER_DUNGEON;

	for (INT id : m_members) {
		if (-1 == id) continue;

		server.m_clients[id]->DoSend(&packet);
	}
}

void Party::SendEnterFail()
{
	Server& server = Server::GetInstance();

	SC_ENTER_FAIL_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_ENTER_FAIL;

	for (INT id : m_members) {
		if (-1 == id) continue;

		server.m_clients[id]->DoSend(&packet);
	}
}

PartyManager::PartyManager()
{
	for (auto& party : m_parties) {
		party = std::make_shared<Party>();
	}
}

INT PartyManager::GetHostId(INT party_num)
{
	return m_parties[party_num]->GetHostId();
}

std::shared_ptr<Party>& PartyManager::GetParty(INT party_num)
{
	return m_parties[party_num];
}

bool PartyManager::CreateParty(INT party_num, INT player_id)
{
	/*INT num{ FindEmptyParty() };
	if (-1 == num)
		return false;*/

	auto& party = m_parties[party_num];

	std::unique_lock<std::mutex> l{ party->GetStateMutex() };
	if (party->GetState() != PartyState::EMPTY) {
		SendCreateFail(player_id);
		l.unlock();
		return false;
	}
	
	party->SetState(PartyState::ACCEPT);
	l.unlock();

	// 생성 전송
	SendCreateOk(player_id);
	//JoinParty(party_num, player_id);

	// 생성된 파티에 참가
	bool success = party->TryJoin(player_id);
	if (success) {
		Server& server = Server::GetInstance();
		auto client = std::dynamic_pointer_cast<Client>(server.m_clients[player_id]);
		client->SetPartyNum(party_num);

		std::unique_lock<std::mutex> l{ m_lock };
		m_looking_clients.erase(player_id);	// 현재 파티에 참가한 플레이어는  ui 확인 리스트에서 삭제
		std::unordered_map<INT, INT> clients = m_looking_clients;
		l.unlock();


		// 파티 ui 변경 전송
		for (const auto& [client_id, page] : clients) {
			SendPartyPage(client_id, page);
		}

		// 파티 멤버 추가 전송
		party->AddMember(player_id);
	}

	return success;
}

bool PartyManager::JoinParty(INT party_num, INT player_id)
{
	auto& party = m_parties[party_num];

	std::unique_lock<std::mutex> l{ party->GetStateMutex() };
	
	if (party->GetState() != PartyState::ACCEPT) {
		SendJoinFail(player_id);
		l.unlock();
		return false;
	}
	l.unlock();

	bool success = party->TryJoin(player_id);
	if (success) {
		Server& server = Server::GetInstance();
		auto client = std::dynamic_pointer_cast<Client>(server.m_clients[player_id]);
		client->SetPartyNum(party_num);

		std::unique_lock<std::mutex> l{ m_lock };
		m_looking_clients.erase(player_id);	// 현재 파티에 참가한 플레이어는  ui 확인 리스트에서 삭제
		std::unordered_map<INT, INT> clients = m_looking_clients;
		l.unlock();


		// 파티 ui 변경 전송
		for (const auto& [client_id, page] : clients) {
			SendPartyPage(client_id, page);
		}

		// 파티 멤버 추가 전송
		party->AddMember(player_id);
	}

	return success;
}

void PartyManager::ExitParty(INT party_num, INT player_id)
{
	m_parties[party_num]->Exit(player_id);

	std::unique_lock<std::mutex> l{ m_lock };
	std::unordered_map<INT, INT> clients = m_looking_clients;
	l.unlock();

	Server& server = Server::GetInstance();

	for (const auto& [client_id, page] : clients) {
		SendPartyPage(client_id, page);
	}
}

void PartyManager::ChangeCharacter(INT party_num, INT player_id)
{
	m_parties[party_num]->SendChangeCharacter(player_id);
}

void PartyManager::OpenPartyUI(INT player_id)
{
	{
		std::lock_guard<std::mutex> l{ m_lock };
		m_looking_clients.insert({ player_id, 0 });	// 초기 0 페이지
	}
	
	SendPartyPage(player_id, 0);	// 초기 페이지 (0 페이지) 전송
}

void PartyManager::ClosePartyUI(INT player_id)
{
	std::lock_guard<std::mutex> l{ m_lock };
	if(m_looking_clients.contains(player_id))
		m_looking_clients.erase(player_id);
}

void PartyManager::ChangePage(INT player_id, INT page)
{
	std::unique_lock<std::mutex> l{ m_lock };
	m_looking_clients[player_id] = page;
	l.unlock();

	SendPartyPage(player_id, page);
}

void PartyManager::SendPartyPage(INT player_id, INT page)
{
	Server& server = Server::GetInstance();

	SC_PARTY_INFO_PACKET packet[MAX_PARTIES_ON_PAGE];

	for (size_t i = 0; i < MAX_PARTIES_ON_PAGE; ++i) {
		packet[i].size = sizeof(SC_PARTY_INFO_PACKET);
		packet[i].type = SC_PACKET_PARTY_INFO;
		packet[i].info.current_player = 0;
		packet[i].info.party_num = static_cast<CHAR>(i);
	}

	INT start_num{ GetStartNum(page) };

	for (size_t i = 0; i < MAX_PARTIES_ON_PAGE; ++i) {
		if (i + start_num >= MAX_PARTY_NUM) break;

		INT host_id = m_parties[start_num + i]->GetHostId();
		if (-1 != host_id) {
			auto host = std::dynamic_pointer_cast<Client>(server.m_clients[host_id]);

			std::string name{};
			std::wstring ws{ host->GetUserId() };
			name.assign(ws.begin(), ws.end());
			strcpy_s(packet[i].info.host_name, name.c_str());

			packet[i].info.current_player = m_parties[start_num + i]->GetMemberCount();
		}
	}

	server.m_clients[player_id]->DoSend(&packet, MAX_PARTIES_ON_PAGE);
}

INT PartyManager::FindEmptyParty()
{
	for (size_t i = 0; const auto & party : m_parties) {
		std::lock_guard<std::mutex> lock{ party->GetStateMutex() };
		if (PartyState::EMPTY == party->GetState()) {
			party->SetState(PartyState::ACCEPT);
			return i;
		}
		++i;
	}

	return -1;
}

void PartyManager::SendCreateOk(INT receiver)
{
	SC_CREATE_OK_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_CREATE_OK;

	Server& server = Server::GetInstance();
	server.m_clients[receiver]->DoSend(&packet);
}

void PartyManager::SendCreateFail(INT receiver)
{
	SC_CREATE_FAIL_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_CREATE_FAIL;

	Server& server = Server::GetInstance();
	server.m_clients[receiver]->DoSend(&packet);
}

void PartyManager::SendJoinFail(INT receiver)
{
	SC_JOIN_FAIL_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_JOIN_FAIL;

	Server& server = Server::GetInstance();
	server.m_clients[receiver]->DoSend(&packet);
}

INT PartyManager::GetStartNum(INT page)
{
	return page * MAX_PARTIES_ON_PAGE;
}
