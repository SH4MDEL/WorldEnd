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

Client::Client() : m_socket{}, m_is_ready{ false }, m_remain_size{ 0 },
	m_recv_over{}
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
	m_is_ready = false;
	m_user_id.clear();

	// 나중에 DB에서 처리될 것들
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

void Client::SetReady(bool value)
{
	m_is_ready = value;
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

void Client::SetNormalSkillType(PlayerType player_type, UCHAR type)
{
	m_skills[(INT)player_type][(INT)SkillType::NORMAL]->SetSkillType(type);
}

void Client::SetUltimateSkillType(PlayerType player_type, UCHAR type)
{
	m_skills[(INT)player_type][(INT)SkillType::ULTIMATE]->SetSkillType(type);
}

FLOAT Client::GetSkillRatio(ActionType type) const
{
	FLOAT ratio{};
	switch (type) {
	case ActionType::SKILL:
		ratio = PlayerSetting::SKILL_RATIO[static_cast<int>(m_player_type)];
		break;
	case ActionType::ULTIMATE:
		ratio = PlayerSetting::ULTIMATE_RATIO[static_cast<int>(m_player_type)];
		break;
	default:
		return 1.f;
	}

	return ratio;
}

UCHAR Client::GetNormalSkillType(PlayerType type) const
{
	return m_skills[(INT)type][static_cast<INT>(SkillType::NORMAL)]->GetSkillType();
}

UCHAR Client::GetUltimateSkillType(PlayerType type) const
{
	return m_skills[(INT)type][static_cast<INT>(SkillType::ULTIMATE)]->GetSkillType();
}

void Client::ChangeStamina(FLOAT value)
{
	m_stamina += value;
	if (m_stamina <= 0)
		m_stamina = 0;
}

void Client::ChangeGold(INT value)
{
	m_gold += value;
}

void Client::DecreaseHp(FLOAT damage, INT id)
{
	if (m_status->GetHp() <= 0)
		return;

	if (m_invincible_roll == false) 
	{
		bool death = m_status->CalculateHitDamage(damage);

		if (death) {

			// INGAME 에서 State를 바꾸는 것에는 경합이 필요 없으므로 lock 걸지 않음
			m_state = State::DEATH;
			m_current_animation = ObjectAnimation::DEATH;
		}
	}
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
	for (INT& id : m_members) {
		if (-1 == id) continue;

		id = -1;
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
		client->SetReady(false);
		id = -1;
	}

	m_host_id = -1;
	m_member_count = static_cast<BYTE>(0);
	m_state = PartyState::EMPTY;
}

bool Party::Join(INT player_id)
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

	std::unique_lock<std::mutex> lock{ m_member_lock };

	// 파티장이면
	if (m_host_id == player_id) {
		// 파티원이 있으면
		if (m_member_count > 1) {
			auto it = std::remove(m_members.begin(), m_members.end(), player_id);
			
			if (it != m_members.end()) {
				*it = -1;
				m_member_count -= 1;

				// 파티장 위임
				m_host_id = m_members[0];
			}
			lock.unlock();

			// 방장임을 전달
			SendChangeHost(*it);
			
			// 남아있는 파티원에게 나갔다고 전송
			SendExitParty(player_id);
		}
		// 파티원이 없으면 파티 초기화
		else {
			Init();
			return;
		}
	}
	// 파티원이면
	else {
		auto it = std::remove(m_members.begin(), m_members.end(), player_id);
		if (it != m_members.end()) {
			*it = -1;
		}
		lock.unlock();

		SendExitParty(player_id);
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

void Party::PlayerReady(INT player_id)
{
	Server& server = Server::GetInstance();

	if (player_id == m_host_id) {
		// 한명이라도 준비가 안되면 입장 X
		for (INT id : m_members) {
			if (-1 == id) continue;
			if (id == player_id) continue;

			auto client = dynamic_pointer_cast<Client>(server.m_clients[player_id]);
			if (!client->GetReady()) {
				return;
			}
		}

		// 게임룸 진입
		auto room_manager = server.GetGameRoomManager();
		bool entered = room_manager->EnterGameRoom(shared_from_this());

		// 진입 상태에 따른 전송
		if (entered) {
			SendEnterGameRoom();

			// 게임룸에 진입했다면 파티 반환
			Reset();
		}
		else {
			SendEnterFail();
		}
	}
	else {
		auto client = dynamic_pointer_cast<Client>(server.m_clients[player_id]);
		
		client->SetReady(!client->GetReady());	// 현재 준비 상태 뒤집기

		// 준비 상태 전송
		SendReady(player_id);
	}
}

void Party::SendJoinOk(INT receiver)
{
	SC_JOIN_OK_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_JOIN_OK;

	Server& server = Server::GetInstance();
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

void Party::SendExitParty(INT exited_id)
{

	SC_REMOVE_PARTY_MEMBER_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_REMOVE_PARTY_MEMBER;
	packet.id = exited_id;

	Server& server = Server::GetInstance();

	for (INT id : m_members) {
		if (-1 == id) continue;

		server.m_clients[id]->DoSend(&packet);
	}
}

void Party::SendReady(INT sender)
{
	Server& server = Server::GetInstance();
	auto client = dynamic_pointer_cast<Client>(server.m_clients[sender]);

	SC_PLAYER_READY_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_PLAYER_READY;
	packet.id = sender;
	packet.is_ready = client->GetReady();

	for (INT id : m_members) {
		if (-1 == id) continue;
		if (id == sender) continue;

		server.m_clients[id]->DoSend(&packet);
	}
}

void Party::SendEnterGameRoom()
{
	Server& server = Server::GetInstance();

	SC_ENTER_GAME_ROOM_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_ENTER_GAME_ROOM;

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

bool PartyManager::CreateParty(INT player_id)
{
	INT num{ FindEmptyParty() };
	if (-1 == num)
		return false;

	JoinParty(num, player_id);		// 방이 비어있다면 항상 성공할 것
	SendCreateOk(player_id);

	return true;
}

bool PartyManager::JoinParty(INT party_num, INT player_id)
{
	bool success = m_parties[party_num]->Join(player_id);
	if (success) {
		std::unique_lock<std::mutex> l{ m_lock };
		m_looking_clients.erase(player_id);	// 현재 파티에 참가한 플레이어는 삭제
		std::unordered_map<INT, INT> clients = m_looking_clients;
		l.unlock();

		Server& server = Server::GetInstance();

		for (const auto& [client_id, page] : clients) {
			SendPartyPage(client_id, page);
		}
	}

	return success;
}

void PartyManager::ExitParty(INT party_num, INT player_id)
{
	m_parties[party_num]->Exit(player_id);

	std::unique_lock<std::mutex> l{ m_lock };
	m_looking_clients.erase(player_id);	// 현재 파티에 참가한 플레이어는 삭제
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

void PartyManager::PlayerReady(INT party_num, INT player_id)
{
	m_parties[party_num]->PlayerReady(player_id);
}

void PartyManager::OpenPartyUI(INT player_id)
{
	std::lock_guard<std::mutex> l{ m_lock };
	m_looking_clients.insert({ player_id, 0 });	// 초기 0 페이지
}

void PartyManager::ClosePartyUI(INT player_id)
{
	std::lock_guard<std::mutex> l{ m_lock };
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
	}

	INT start_num{ GetStartNum(page) };

	for (size_t i = 0; i < MAX_PARTIES_ON_PAGE; ++i) {
		if (i + start_num >= MAX_PARTY_NUM) break;

		INT host_id = m_parties[start_num + i]->GetHostId();
		if (-1 != host_id) {
			auto host = std::dynamic_pointer_cast<Client>(server.m_clients[host_id]);
			packet[i].info.name = host->GetUserId();
			packet[i].info.current_player = m_parties[start_num + i]->GetMemberCount();
			packet[i].info.party_num = i;
		}
	}

	std::cout << "id : " << player_id << "  page : " << page << std::endl;

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

INT PartyManager::GetStartNum(INT page)
{
	return page * MAX_PARTIES_ON_PAGE;
}
