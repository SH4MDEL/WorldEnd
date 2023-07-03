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
	Init();
}

Client::~Client()
{
}

void Client::Init()
{
	m_id = -1;
	m_room_num = -1;
	m_position = XMFLOAT3(0.f, 0.f, 0.f);
	m_velocity = XMFLOAT3(0.f, 0.f, 0.f);
	m_yaw = 0.f;
	m_stamina = PlayerSetting::MAX_STAMINA;
	m_interactable = false;
	m_latest_id = 0;
	m_last_move_time = 0;
	this->SetTriggerFlag();
	m_is_ready = false;
	m_user_id.clear();

	// ���߿� DB���� ó���� �͵�
	SetPlayerType(PlayerType::WARRIOR);
	m_name = "Player";

	m_status->SetAtk(90.f);
	m_status->SetMaxHp(100.f);
	m_status->SetHp(100.f);
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

void Client::ChangeStamina(FLOAT value)
{
	m_stamina += value;
	if (m_stamina <= 0)
		m_stamina = 0;
}

void Client::DecreaseHp(FLOAT damage, INT id)
{
	if (m_status->GetHp() <= 0)
		return;

	bool death = m_status->CalculateHitDamage(damage);
	if (death) {

		// INGAME ���� State�� �ٲٴ� �Ϳ��� ������ �ʿ� �����Ƿ� lock ���� ����
		m_state = State::DEATH;
		m_current_animation = ObjectAnimation::DEATH;
	}
}

// ���� �� �Ѿ �� ���� ȸ��
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
		// ȭ�� �ٿ�� �ڽ��� ��ü �ʿ���, Ȱ�� �浹���� ���� ���̱� ����
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

	// ��Ƽ���̸�
	if (m_host_id == player_id) {
		// ��Ƽ���� ������
		if (m_member_count > 1) {
			auto it = std::remove(m_members.begin(), m_members.end(), player_id);
			
			if (it != m_members.end()) {
				*it = -1;
				m_member_count -= 1;

				// ��Ƽ�� ����
				m_host_id = m_members[0];
			}
			lock.unlock();

			// �������� ����
			SendChangeHost(*it);
			
			// �����ִ� ��Ƽ������ �����ٰ� ����
			SendExitParty(player_id);
		}
		// ��Ƽ���� ������ ��Ƽ �ʱ�ȭ
		else {
			Init();
			return;
		}
	}
	// ��Ƽ���̸�
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
		// �Ѹ��̶� �غ� �ȵǸ� ���� X
		for (INT id : m_members) {
			if (-1 == id) continue;
			if (id == player_id) continue;

			auto client = dynamic_pointer_cast<Client>(server.m_clients[player_id]);
			if (!client->GetReady()) {
				return;
			}
		}

		// ���ӷ� ����
		auto room_manager = server.GetGameRoomManager();
		bool entered = room_manager->EnterGameRoom(shared_from_this());

		// ���� ���¿� ���� ����
		if (entered) {
			SendEnterGameRoom();

			// ���ӷ뿡 �����ߴٸ� ��Ƽ ��ȯ
			Reset();
		}
		else {
			SendEnterFail();
		}
	}
	else {
		auto client = dynamic_pointer_cast<Client>(server.m_clients[player_id]);
		
		client->SetReady(!client->GetReady());	// ���� �غ� ���� ������

		// �غ� ���� ����
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

	JoinParty(num, player_id);
	SendCreateOk(player_id);

	return true;
}

bool PartyManager::JoinParty(INT party_num, INT player_id)
{
	return m_parties[party_num]->Join(player_id);
}

void PartyManager::ExitParty(INT party_num, INT player_id)
{
	m_parties[party_num]->Exit(player_id);
}

void PartyManager::ChangeCharacter(INT party_num, INT player_id)
{
	m_parties[party_num]->SendChangeCharacter(player_id);
}

void PartyManager::PlayerReady(INT party_num, INT player_id)
{
	m_parties[party_num]->PlayerReady(player_id);
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
