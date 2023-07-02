#include <functional>
#include <random>
#include "monster.h"
#include "stdafx.h"
#include "Server.h"

std::random_device rd;
std::default_random_engine dre(rd());
std::uniform_int_distribution<INT> random_percent(1, 100);
std::uniform_int_distribution<INT> random_retarget_time(5, 10);
std::uniform_real_distribution<FLOAT> random_step_back_time(5.f, 7.f);
std::uniform_int_distribution<INT> random_flee_angle(20, 45);


// ------------------------- ����� �� ---------------------------------
std::string DebugBehavior(MonsterBehavior b)
{
	switch (b) {
	case MonsterBehavior::CHASE:
		return std::string("chase");
		break;
	case MonsterBehavior::RETARGET:
		return std::string("retarget");
		break;
	case MonsterBehavior::TAUNT:
		return std::string("taunt");
		break;
	case MonsterBehavior::PREPARE_ATTACK:
		return std::string("prepare_attack");
		break;
	case MonsterBehavior::AIM:
		return std::string("aim");
		break;
	case MonsterBehavior::ATTACK:
		return std::string("attack");
		break;
	case MonsterBehavior::STEP_BACK:
		return std::string("stepBack");
		break;
	case MonsterBehavior::FLEE:
		return std::string("flee");
		break;
	case MonsterBehavior::DELAY:
		return std::string("delay");
		break;
	case MonsterBehavior::DEATH:
		return std::string("death");
		break;
	default:	// ���� �ൿ�̸� COUNT
		return std::string("x");
		break;
	}
}
// ------------------------- ����� �� ---------------------------------


Monster::Monster() : m_target_id{ -1 }, m_current_animation{ ObjectAnimation::IDLE },
	m_current_behavior{ MonsterBehavior::COUNT }, m_aggro_level{ 0 },
	m_last_behavior_id{ 1 }
{
    m_bounding_box.Center = XMFLOAT3(0.028f, 1.27f, 0.f);
	m_bounding_box.Extents = XMFLOAT3(0.8f, 1.3f, 0.6f);
	m_bounding_box.Orientation = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
}

void Monster::Init()
{
	m_target_id = -1;
	m_current_animation = ObjectAnimation::IDLE;
	m_current_behavior = MonsterBehavior::COUNT;
	m_aggro_level = 0;
	m_last_behavior_id = 1;	// 0�� ��ȿ���� ���� �ൿ id
	m_room_num = -1;	// FREE ���ڸ��� ���� ������ �� 
						// Ÿ�̸� �̺�Ʈ�� �������� �� �����Ƿ� �ʱ�ȭ
}

void Monster::Update(FLOAT elapsed_time)
{
	if (State::INGAME != m_state) return;

	DoBehavior(elapsed_time);
}

void Monster::UpdatePosition(const XMFLOAT3& dir, FLOAT elapsed_time, FLOAT speed)
{
	m_velocity = Vector3::Mul(dir, speed);

	// ���� �̵�
	m_position = Vector3::Add(m_position, Vector3::Mul(m_velocity, elapsed_time));

	m_bounding_box.Center = m_position;

	Server& server = Server::GetInstance();
	auto game_room = server.GetGameRoomManager()->GetGameRoom(m_room_num);
	game_room->CheckTriggerCollision(m_id);
}

void Monster::UpdateRotation(const XMFLOAT3& dir)
{
	XMVECTOR z_axis{ XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f) };
	XMVECTOR radian{ XMVector3AngleBetweenNormals(z_axis, XMLoadFloat3(&dir)) };
	FLOAT angle{ XMConvertToDegrees(XMVectorGetX(radian)) };

	XMVECTOR vec = XMQuaternionRotationRollPitchYaw(0.f, XMVectorGetX(radian), 0.f);
	XMFLOAT4 q{};
	XMStoreFloat4(&q, vec);
	m_bounding_box.Orientation = XMFLOAT4{ q.x, q.y, q.z, q.w };

	// degree - Ŭ���̾�Ʈ���� degree �� Rotation �� ó����
	m_yaw = dir.x < 0 ? -angle : angle;
}


XMFLOAT3 Monster::GetDirection(INT id)
{
	if (-1 == id)
		return XMFLOAT3(0.f, 0.f, 0.f);

	Server& server = Server::GetInstance();

	XMFLOAT3 sub = Vector3::Sub(server.m_clients[id]->GetPosition(), m_position);
	sub.y = 0.f;
	return Vector3::Normalize(sub);
}

bool Monster::IsInRange(FLOAT range)
{
	Server& server = Server::GetInstance();
	if (-1 == m_target_id)
		return false;

	FLOAT dist = Vector3::Distance(server.m_clients[m_target_id]->GetPosition(), m_position);

	if (dist <= range)
		return true;

	return false;
}

void Monster::SetDecreaseAggroLevelEvent()
{
	TIMER_EVENT ev{ .event_time = std::chrono::system_clock::now() + 
			MonsterSetting::DECREASE_AGRO_LEVEL_TIME, .obj_id = m_id,
			.event_type = EventType::AGRO_LEVEL_DECREASE,
		.aggro_level = m_aggro_level };

	Server& server = Server::GetInstance();
	server.SetTimerEvent(ev);
}

void Monster::SetBehaviorTimerEvent(MonsterBehavior behavior)
{
	m_current_behavior = behavior;
	
	// ���ͺ��� �ൿ�� ���� �ִϸ��̼� ����
	SetBehaviorAnimation(behavior);

	TIMER_EVENT ev{.obj_id = m_id, .event_type = EventType::BEHAVIOR_CHANGE,
	.aggro_level = m_aggro_level};

	if (std::numeric_limits<BYTE>::max() == m_last_behavior_id)
		m_last_behavior_id = 0;
	ev.latest_id = ++m_last_behavior_id;

	// ���ͺ��� �ൿ�� ���� ���� �ൿ ����
	ev.next_behavior_type = SetNextBehavior(behavior);
	if (MonsterBehavior::COUNT == ev.next_behavior_type) {
		return;
	}

	// ���ͺ��� �ൿ�� ���� �󸶳� �ൿ�� ������ �� ����
	ev.event_time = std::chrono::system_clock::now() + SetBehaviorTime(behavior);


	Server& server = Server::GetInstance();
	server.SetTimerEvent(ev);
}

void Monster::SetTarget(INT id)
{
	if (m_target_id != id)
		m_target_id = id;
}

void Monster::SetAggroLevel(BYTE aggro_level)
{
	// �����Ϸ��� ��׷� ������ �� Ŭ���� �����ϰ� Ÿ�̸� �̺�Ʈ ����
	if (m_aggro_level < aggro_level) {
		m_aggro_level = aggro_level;
		
		SetDecreaseAggroLevelEvent();
	}
}

void Monster::SetBehaviorId(BYTE id)
{
	m_last_behavior_id = id;
}

MONSTER_DATA Monster::GetMonsterData() const
{
	return MONSTER_DATA( m_id, m_position, m_velocity, m_yaw, m_hp );
}

bool Monster::ChangeAnimation(BYTE animation)
{
	if (m_current_animation == animation)
		return false;

	m_current_animation = animation;
	return true;
}

void Monster::ChangeBehavior(MonsterBehavior behavior)
{
	SetBehaviorTimerEvent(behavior);

	Server& server = Server::GetInstance();

	auto game_room = server.GetGameRoomManager()->GetGameRoom(m_room_num);
	auto& ids = game_room->GetPlayerIds();

	SC_CHANGE_MONSTER_BEHAVIOR_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_CHANGE_MONSTER_BEHAVIOR;
	packet.behavior = m_current_behavior;
	packet.animation = m_current_animation;
	packet.id = m_id;

	for (INT id : ids) {
		if (-1 == id) continue;

		server.m_clients[id]->DoSend(&packet);
	}
}

void Monster::DoBehavior(FLOAT elapsed_time)
{
	switch (m_current_behavior) {
	case MonsterBehavior::CHASE:
		ChasePlayer(elapsed_time);
		break;
	case MonsterBehavior::RETARGET:
		Retarget();
		break;
	case MonsterBehavior::TAUNT:
		Taunt();
		break;
	case MonsterBehavior::PREPARE_ATTACK:
		PrepareAttack();
		break;
	case MonsterBehavior::ATTACK:
		Attack();
		break;

		// ���� ���� ����
	case MonsterBehavior::BLOCK:

		break;
	case MonsterBehavior::BLOCKIDLE:

		break;

		// �ü� ���� ����
	case MonsterBehavior::AIM: {
		XMFLOAT3 player_dir = GetDirection(m_target_id);
		UpdateRotation(player_dir);
		break;
	}
	case MonsterBehavior::STEP_BACK:
		StepBack(elapsed_time);
		break;
	case MonsterBehavior::FLEE:
		Flee(elapsed_time);
		break;
	case MonsterBehavior::DELAY:
		XMFLOAT3 player_dir = GetDirection(m_target_id);
		UpdateRotation(player_dir);
		break;

		// ������ ����
	case MonsterBehavior::PREPARE_CAST: {
		XMFLOAT3 player_dir = GetDirection(m_target_id);
		UpdateRotation(player_dir);
		break;
	}
	case MonsterBehavior::CAST: {
		XMFLOAT3 player_dir = GetDirection(m_target_id);
		UpdateRotation(player_dir);
		break;
	}
       // ���� ����
	case MonsterBehavior::PREPARE_WIDE_SKILL: {
		XMFLOAT3 player_dir = GetDirection(m_target_id);
		UpdateRotation(player_dir);
		break;
	}
	case MonsterBehavior::WIDE_SKILL: {
		XMFLOAT3 player_dir = GetDirection(m_target_id);
		UpdateRotation(player_dir);
		break;
	}

	case MonsterBehavior::LAUGHING:
		// �ൿ X
		break;

	default:
		return;
	}
}

INT Monster::GetPlayerHighestDamage()
{
	std::sort(m_pl_save_damage.begin(), m_pl_save_damage.end(), [](const m_save_damage_pair& a, const m_save_damage_pair& b) {
		return a.second > b.second;
		});

	return m_pl_save_damage[0].first;
}

void Monster::DecreaseHp(FLOAT damage, INT id)
{
	if (m_hp <= 0)
		return;

	m_hp -= damage;

	if (m_hp <= 0) {
		m_hp = 0.f;
		{
			std::lock_guard<std::mutex> l{ m_state_lock };
			m_state = State::DEATH;
		}
		// ������ ����
		ChangeBehavior(MonsterBehavior::DEATH);
		return;
	}

	// ���� ����ȭ ���� Ÿ���� �κ��� �Ϲ� ���Ϳ��� Ȯ���ϱ� ���� �ڵ�
	//---------------------------------------------------------------------------------------------------------------------------------
	//Server& server = Server::GetInstance();
    //INT pl_highest_damage_id{};
	//server.m_clients[id]->SetSaveDamage(0);
	//m_pl_save_damage.push_back(std::make_pair(id, server.m_clients[id]->GetSaveDamage()));

	//auto pl_save_data = std::find_if(m_pl_save_damage.begin(), m_pl_save_damage.end(), [id](const m_save_damage_pair& pair) {
	//	return pair.first == id;
	//	});

	//if (pl_save_data != m_pl_save_damage.end()) {
	//	// �̹� ����� ���, �ش� �÷��̾��� ���� �������� ������Ʈ
	//	pl_save_data->second += damage;
	//}
	//else {
	//	// ����Ǿ� ���� ���� ���, ���ο� �÷��̾� ������ �߰�
	//	m_pl_save_damage.push_back(std::make_pair(id, server.m_clients[id]->GetSaveDamage()));
	//}

	//std::cout << id << "�÷��̾��� ���� ������ - " << (FLOAT)pl_save_data->second << std::endl;

	//if (m_hp < 800) {
	//	pl_highest_damage_id = GetPlayerHighestDamage();
	//	SetTarget(pl_highest_damage_id);
	//	std::cout << "����� �÷��̾� id - " << pl_highest_damage_id << std::endl;
	//}
	// ---------------------------------------------------------------------------------------------------------------------------------------------
	if (AggroLevel::HIT_AGGRO < m_aggro_level)
		return;

	// ���� ���� �̻� (����� 20%) �������� ������ Ÿ�� ����
	if ((m_max_hp / 5.f - damage) <= std::numeric_limits<FLOAT>::epsilon()) {
		SetTarget(id);
		//SetAggroLevel(AggroLevel::HIT_AGGRO);
	}

}	


void Monster::DecreaseAggroLevel()
{
	m_aggro_level -= 1;
	if (m_aggro_level <= 0) {
		m_aggro_level = 0;
	}
	else {
		SetDecreaseAggroLevelEvent();
	}
	
}

bool Monster::CheckPlayer()
{
	// Ÿ���� ���ų�, Ÿ���� room �� ���ų�, INGAME �� �ƴϸ� false
	if (-1 == m_target_id)
		return false;

	Server& server = Server::GetInstance();
	
	// Ÿ�� �÷��̾ ���� ����Ǹ� �߰� X
	auto game_room = server.GetGameRoomManager()->GetGameRoom(m_room_num);
	auto& ids = game_room->GetPlayerIds();

	if (State::INGAME != server.m_clients[m_target_id]->GetState()) {
		return false;
	}
	else if (std::ranges::find(ids, m_target_id) == ids.end()) {
		return false;
	}
	return true;
}

void Monster::UpdateTarget()
{
	if (AggroLevel::NORMAL_AGGRO < m_aggro_level)
		return;

	Server& server = Server::GetInstance();
	m_target_id = server.GetNearTarget(m_id, MonsterSetting::RECOGNIZE_RANGE);

	SetAggroLevel(AggroLevel::NORMAL_AGGRO);
}

void Monster::ChasePlayer(FLOAT elapsed_time)
{
	if (!CheckPlayer()) {
		UpdateTarget();
	}

	// Ÿ������ �÷��̾� �߰�
	XMFLOAT3 player_dir = GetDirection(m_target_id);

	if (Vector3::Equal(player_dir, XMFLOAT3(0.f, 0.f, 0.f)))
		return;

	if(m_monster_type == MonsterType::BOSS)
		UpdatePosition(player_dir, elapsed_time, MonsterSetting::BOSD_RUN_SPEED);
	else
		UpdatePosition(player_dir, elapsed_time, MonsterSetting::WALK_SPEED);
	UpdateRotation(player_dir);
	CollisionCheck();
}

void Monster::Retarget()
{
	// Ÿ���� ���� �� �θ��� �Ÿ��� �ൿ
	// ��� �߰����� �ʰ� ���� �ð����� ���缭���� ��
	UpdateTarget();
}

void Monster::Taunt()
{
	if (!CheckPlayer()) {
		UpdateTarget();
	}

	XMFLOAT3 player_dir = GetDirection(m_target_id);
	UpdateRotation(player_dir);
}

void Monster::PrepareAttack()
{
	if (!CheckPlayer()) {
		UpdateTarget();
	}

	// ���� �� �������� �˸��� ���� �ൿ

	XMFLOAT3 player_dir = GetDirection(m_target_id);
	UpdateRotation(player_dir);
}

void Monster::Attack()
{
	if (!CheckPlayer()) {
		ChangeBehavior(MonsterBehavior::RETARGET);
	}
	// �÷��̾� ����

}

void Monster::CollisionCheck()
{
	Server& server = Server::GetInstance(); 
	
	server.GameRoomObjectCollisionCheck(shared_from_this(), m_room_num);
}

void Monster::RandomTarget()
{
	std::random_device rd;
	std::mt19937 gen(rd());

	std::shuffle(m_pl_random_id.begin(), m_pl_random_id.end(), gen);

	INT random_id = *m_pl_random_id.begin();

	SetTarget(random_id);
	std::cout << "�������� �߰ݴ��ϴ� �÷��̾� - " << random_id << std::endl;
}

void Monster::InitializePosition(INT mon_cnt, MonsterType mon_type, INT random_map, FLOAT mon_pos[])
{
	// ������ �о �ʱ� ��ġ�� ���ϴ� �Լ� �ʿ�
	// ���� �Ŵ������� �ؾ��ϴ� ���̹Ƿ� ���߿� �����Ŵ����� �Űܾ� �ϴ� �Լ�
	// ���� �Ŵ����� ��������, ���� ��ġ ���� ���� �������� ������
	
	Server& server = Server::GetInstance();

	if (random_map == 0)
	{
		if (mon_type == MonsterType::WARRIOR)
			SetPosition(mon_pos[(mon_cnt * 2) + 2], 0, mon_pos[(mon_cnt * 2) + 3]);
		else if (mon_type == MonsterType::ARCHER)
			SetPosition(mon_pos[(mon_cnt * 2) + 4], 0, mon_pos[(mon_cnt * 2) + 5]);
		else if (mon_type == MonsterType::WIZARD)
			SetPosition(mon_pos[(mon_cnt * 2) + 6], 0, mon_pos[(mon_cnt * 2) + 7]);
	}
	else if (random_map == 1)
	{
		if (mon_type == MonsterType::WARRIOR)
			SetPosition(mon_pos[(mon_cnt * 2) + 22], 0, mon_pos[(mon_cnt * 2) + 23]);
		else if (mon_type == MonsterType::ARCHER)
			SetPosition(mon_pos[(mon_cnt * 2) + 24], 0, mon_pos[(mon_cnt * 2) + 25]);
		else if (mon_type == MonsterType::WIZARD)
			SetPosition(mon_pos[(mon_cnt * 2) + 26], 0, mon_pos[(mon_cnt * 2) + 27]);
	}
	else if (random_map == 2)
	{
		if (mon_type == MonsterType::WARRIOR)
			SetPosition(mon_pos[(mon_cnt * 2) + 42], 0, mon_pos[(mon_cnt * 2) + 43]);
		else if (mon_type == MonsterType::ARCHER)
			SetPosition(mon_pos[(mon_cnt * 2) + 44], 0, mon_pos[(mon_cnt * 2) + 45]);
		else if (mon_type == MonsterType::WIZARD)
			SetPosition(mon_pos[(mon_cnt * 2) + 46], 0, mon_pos[(mon_cnt * 2) + 47]);
	}
	else if (random_map == 3)
	{
		if (mon_type == MonsterType::WARRIOR)
			SetPosition(mon_pos[(mon_cnt * 2) + 62], 0, mon_pos[(mon_cnt * 2) + 63]);
		else if (mon_type == MonsterType::ARCHER)
			SetPosition(mon_pos[(mon_cnt * 2) + 64], 0, mon_pos[(mon_cnt * 2) + 65]);
		else if (mon_type == MonsterType::WIZARD)
			SetPosition(mon_pos[(mon_cnt * 2) + 66], 0, mon_pos[(mon_cnt * 2) + 67]);
	}
	else if (random_map == 4)
	{
		if (mon_type == MonsterType::WARRIOR)
			SetPosition(mon_pos[(mon_cnt * 2) + 82], 0, mon_pos[(mon_cnt * 2) + 83]);
		else if (mon_type == MonsterType::ARCHER)
			SetPosition(mon_pos[(mon_cnt * 2) + 84], 0, mon_pos[(mon_cnt * 2) + 85]);
		else if (mon_type == MonsterType::WIZARD)
			SetPosition(mon_pos[(mon_cnt * 2) + 86], 0, mon_pos[(mon_cnt * 2) + 87]);
	}
	else if (random_map == 5)
	{
		if (mon_type == MonsterType::WARRIOR)
			SetPosition(mon_pos[(mon_cnt * 2) + 104], 0, mon_pos[(mon_cnt * 2) + 105]);
		else if (mon_type == MonsterType::ARCHER)
			SetPosition(mon_pos[(mon_cnt * 2) + 106], 0, mon_pos[(mon_cnt * 2) + 107]);
		else if (mon_type == MonsterType::WIZARD)
			SetPosition(mon_pos[(mon_cnt * 2) + 108], 0, mon_pos[(mon_cnt * 2) + 109]);
	}
	else if (random_map == 6)
	{
		if (mon_type == MonsterType::WARRIOR)
			SetPosition(mon_pos[(mon_cnt * 2) + 124], 0, mon_pos[(mon_cnt * 2) + 125]);
		else if (mon_type == MonsterType::ARCHER)
			SetPosition(mon_pos[(mon_cnt * 2) + 126], 0, mon_pos[(mon_cnt * 2) + 127]);
		else if (mon_type == MonsterType::WIZARD)
			SetPosition(mon_pos[(mon_cnt * 2) + 128], 0, mon_pos[(mon_cnt * 2) + 129]);
	}
	else if (random_map == 7)
	{
		if (mon_type == MonsterType::WARRIOR)
			SetPosition(mon_pos[(mon_cnt * 2) + 144], 0, mon_pos[(mon_cnt * 2) + 145]);
		else if (mon_type == MonsterType::ARCHER)
			SetPosition(mon_pos[(mon_cnt * 2) + 146], 0, mon_pos[(mon_cnt * 2) + 147]);
		else if (mon_type == MonsterType::WIZARD)
			SetPosition(mon_pos[(mon_cnt * 2) + 148], 0, mon_pos[(mon_cnt * 2) + 149]);
	}
	else if (random_map == 10)
	{
		if (mon_type == MonsterType::BOSS)
			SetPosition(0, 0, 35);
	}
}

WarriorMonster::WarriorMonster()
{
	m_max_hp = 200.f;
	m_damage = 2;
	m_attack_range = 1.5f;
	m_boundary_range = 3.f;
	m_monster_type = MonsterType::WARRIOR;
}

void WarriorMonster::Init()
{
	Monster::Init();

	m_hp = m_max_hp;
}

void WarriorMonster::Update(FLOAT elapsed_time)
{
	Monster::Update(elapsed_time);

	// ���� ���� ���� ������ �� ���� ��ȯ�� �����ϸ� ���� ��ȯ
	if (IsInRange(m_attack_range)) {
		if (CanSwapAttackBehavior()) {
			ChangeBehavior(MonsterBehavior::PREPARE_ATTACK);
		}
	}
}

bool WarriorMonster::CanSwapAttackBehavior()
{
	if ((MonsterBehavior::CHASE == m_current_behavior) ||
		(MonsterBehavior::RETARGET == m_current_behavior))
	{
		return true;
	}
	return false;
}

MonsterBehavior WarriorMonster::SetNextBehavior(MonsterBehavior behavior)
{
	// ��ȸ �߰� �ʿ�
	// ��ȸ�� ���� �ð��� ������ ���� ���� ����
	// ���� ���� �Ӹ����� Į�� ���ø��� ������� ����
	// �ش� ���¿��� �����ð� ������ Į�� ����ġ�鼭 �����ϵ��� �ؾ� ��

	MonsterBehavior temp{};
	switch (behavior) {
	case MonsterBehavior::CHASE: {
		int percent = random_percent(dre);
		if (1 <= percent && percent <= 50) {
			temp = MonsterBehavior::RETARGET;
		}
		else {
			temp = MonsterBehavior::TAUNT;
		}
		break; 
	}
	case MonsterBehavior::RETARGET:
		temp = MonsterBehavior::CHASE;
		break;
	case MonsterBehavior::TAUNT:
		temp = MonsterBehavior::CHASE;
		break;
	case MonsterBehavior::PREPARE_ATTACK:
		temp = MonsterBehavior::ATTACK;
		break;
	case MonsterBehavior::ATTACK:
		if (IsInRange(m_attack_range)) {
			temp = MonsterBehavior::PREPARE_ATTACK;	// ���� ���� ���� ���� �����ϸ� 
													// ��ȸ�� ��
													// ������ ��ȸ�� �����Ƿ� ���� ���
		}
		else {
			temp = MonsterBehavior::CHASE;
		}
		break;
	case MonsterBehavior::DEATH:
		temp = MonsterBehavior::REMOVE;
		break;
	default:	// ���� �ൿ�̸� COUNT
		temp = MonsterBehavior::COUNT;
		break;
	}
	return temp;
}

void WarriorMonster::SetBehaviorAnimation(MonsterBehavior behavior)
{
	switch (behavior) {
	case MonsterBehavior::CHASE:
		m_current_animation = WarriorMonsterAnimation::RUN;
		break;
	case MonsterBehavior::RETARGET:
		m_current_animation = WarriorMonsterAnimation::LOOK_AROUND;
		break;
	case MonsterBehavior::TAUNT:
		m_current_animation = WarriorMonsterAnimation::TAUNT;
		break;
	case MonsterBehavior::PREPARE_ATTACK:
		m_current_animation = WarriorMonsterAnimation::TAUNT;
		break;
	case MonsterBehavior::ATTACK:
		m_current_animation = WarriorMonsterAnimation::ATTACK;
		break;
	case MonsterBehavior::DEATH:
		m_current_animation = WarriorMonsterAnimation::DEATH;
		break;
	case MonsterBehavior::BLOCK:
		m_current_animation = WarriorMonsterAnimation::BLOCK;
		break;
	case MonsterBehavior::BLOCKIDLE:
		m_current_animation = WarriorMonsterAnimation::BLOCKIDLE;
		break;
	}
}

std::chrono::milliseconds WarriorMonster::SetBehaviorTime(MonsterBehavior behavior)
{
	using namespace std::literals;

	std::chrono::milliseconds time{};

	switch (behavior) {
	case MonsterBehavior::CHASE:
		time = 7000ms;
		break;
	case MonsterBehavior::RETARGET:
		time = 3000ms;
		break;
	case MonsterBehavior::TAUNT:
		time = 2000ms;
		break;
	case MonsterBehavior::PREPARE_ATTACK:
		time = 1000ms;
		break;
	case MonsterBehavior::ATTACK:
		time = 625ms;
		break;
	case MonsterBehavior::DEATH:
		time = 2000ms;
		break;
	case MonsterBehavior::BLOCK:
		time = 0ms;
		break;
	case MonsterBehavior::BLOCKIDLE:
		time = 0ms;
		break;
	}

	return time;
}

ArcherMonster::ArcherMonster()
{
	m_max_hp = 150.f;
	m_damage = 20;
	m_attack_range = 12.5f;
	m_recover_attack_range = 8.f;
	m_boundary_range = 3.f;
	m_monster_type = MonsterType::ARCHER;
	m_flee_direction = XMFLOAT3(0.f, 0.f, 0.f);
}

void ArcherMonster::Init()
{
	Monster::Init();
	m_hp = m_max_hp;
}

void ArcherMonster::Update(FLOAT elapsed_time)
{
	Monster::Update(elapsed_time);

	// ��� ���� ���� ������ �� �������� �ƴ϶�� ����

	if (IsInRange(m_boundary_range)) {
		if (!DoesAttack()) {
			if (MonsterBehavior::STEP_BACK != m_current_behavior) {
				ChangeBehavior(MonsterBehavior::STEP_BACK);
				m_step_back_time = 0.f;
				m_max_step_back_time = random_step_back_time(dre);
			}
		}
	}
	// ���� ���� ���� ������ �� �������� ��ȯ�� �����ϸ� ���� ��ȯ
	else if (IsInRange(m_attack_range)) {
		if (CanSwapAttackBehavior()) {
			ChangeBehavior(MonsterBehavior::PREPARE_ATTACK);
		}
		// �ް����� ġ�� ���� ���� �Ÿ� �������� ���� ��ȯ
		else if (MonsterBehavior::STEP_BACK == m_current_behavior) {
			if (!IsInRange(m_recover_attack_range)) {
				ChangeBehavior(MonsterBehavior::DELAY);
			}
		}
	}

	// �ް����� ġ�� ���� �����ð��� ������ ����
	if (MonsterBehavior::STEP_BACK == m_current_behavior) {
		m_step_back_time += elapsed_time;
		if (m_step_back_time >= m_max_step_back_time) {
			SetFleeDirection();
			ChangeBehavior(MonsterBehavior::FLEE);
		}
	}
}

void ArcherMonster::StepBack(FLOAT elapsed_time)
{
	if (!CheckPlayer()) {
		UpdateTarget();
	}

	// Ÿ������ �÷��̾� �߰�
	XMFLOAT3 player_dir = GetDirection(m_target_id);
	XMFLOAT3 back_dir = Vector3::Sub(XMFLOAT3(0.f, 0.f, 0.f), player_dir);

	if (Vector3::Equal(player_dir, XMFLOAT3(0.f, 0.f, 0.f)))
		return;

	UpdatePosition(back_dir, elapsed_time, MonsterSetting::STEP_BACK_SPEED);
	UpdateRotation(player_dir);
	CollisionCheck();
}

void ArcherMonster::Flee(FLOAT elapsed_time)
{
	Server& server = Server::GetInstance();
	
	// ���� ���Ͱ� �ִٸ� ���� ���� ������ ȸ��
	auto game_room = server.GetGameRoomManager()->GetGameRoom(m_room_num);

	UpdatePosition(m_flee_direction, elapsed_time, MonsterSetting::FLEE_SPEED);
	UpdateRotation(m_flee_direction);
	CollisionCheck();
}

bool ArcherMonster::CanSwapAttackBehavior()
{
	if ((MonsterBehavior::CHASE == m_current_behavior) ||
		(MonsterBehavior::RETARGET == m_current_behavior))
	{
		return true;
	}
	return false;
}

MonsterBehavior ArcherMonster::SetNextBehavior(MonsterBehavior behavior)
{
	MonsterBehavior temp{};
	switch (behavior) {
	case MonsterBehavior::CHASE: {
		int percent = random_percent(dre);
		if (1 <= percent && percent <= 50) {
			temp = MonsterBehavior::RETARGET;
		}
		else {
			temp = MonsterBehavior::TAUNT;
		}
		break;
	}
	case MonsterBehavior::RETARGET:
		temp = MonsterBehavior::CHASE;
		break;
	case MonsterBehavior::TAUNT:
		temp = MonsterBehavior::CHASE;
		break;
	case MonsterBehavior::PREPARE_ATTACK:
		temp = MonsterBehavior::AIM;
		break;
	case MonsterBehavior::AIM:
		temp = MonsterBehavior::ATTACK;
		break;
	case MonsterBehavior::ATTACK:
		if (IsInRange(m_attack_range)) {
			temp = MonsterBehavior::DELAY;	// ���� �� ������
			// ������ �������� ���� Ȯ���� TAUNT(Stomp) �ൿ ���� �ʿ�
			// (�������Ϳ� ���� ȭ���� �߱�����)
		}
		else {
			temp = MonsterBehavior::CHASE;
		}
		break;
	case MonsterBehavior::STEP_BACK:
		temp = MonsterBehavior::COUNT;
		break;
	case MonsterBehavior::FLEE:
		temp = MonsterBehavior::PREPARE_ATTACK;
		break;
	case MonsterBehavior::DELAY:
		temp = MonsterBehavior::RETARGET;
		break;
	case MonsterBehavior::DEATH:
		temp = MonsterBehavior::REMOVE;
		break;
	default:	// ���� �ൿ�̸� COUNT
		temp = MonsterBehavior::COUNT;
		break;
	}
	return temp;
}

void ArcherMonster::SetBehaviorAnimation(MonsterBehavior behavior)
{
	switch (behavior) {
	case MonsterBehavior::CHASE:
		m_current_animation = ArcherMonsterAnimation::WALK;
		break;
	case MonsterBehavior::RETARGET:
		m_current_animation = ArcherMonsterAnimation::LOOK_AROUND;
		break;
	case MonsterBehavior::TAUNT:
		m_current_animation = ArcherMonsterAnimation::TAUNT;
		break;
	case MonsterBehavior::PREPARE_ATTACK:
		m_current_animation = ArcherMonsterAnimation::DRAW;
		break;
	case MonsterBehavior::ATTACK:
		m_current_animation = ArcherMonsterAnimation::ATTACK;
		break;
	case MonsterBehavior::DEATH:
		m_current_animation = ArcherMonsterAnimation::DEATH;
		break;
	case MonsterBehavior::AIM:
		m_current_animation = ArcherMonsterAnimation::AIM;
		break;
	case MonsterBehavior::STEP_BACK:
		m_current_animation = ArcherMonsterAnimation::WALK_BACKWARD;
		break;
	case MonsterBehavior::FLEE:
		m_current_animation = ArcherMonsterAnimation::RUN;
		break;
	case MonsterBehavior::DELAY:
		m_current_animation = ArcherMonsterAnimation::LOOK_AROUND;
		break;
	}
}

std::chrono::milliseconds ArcherMonster::SetBehaviorTime(MonsterBehavior behavior)
{
	using namespace std::literals;

	std::chrono::milliseconds time{};

	switch (behavior) {
	case MonsterBehavior::CHASE:
		time = 3000ms;
		break;
	case MonsterBehavior::RETARGET:
		time = 2800ms;
		break;
	case MonsterBehavior::TAUNT:
		time = 1800ms;
		break;
	case MonsterBehavior::PREPARE_ATTACK:
		time = 1000ms;
		break;
	case MonsterBehavior::ATTACK:
		time = 500ms;
		break;
	case MonsterBehavior::DEATH:
		time = 2000ms;
		break;
	case MonsterBehavior::AIM:
		time = 3000ms;
		break;
	case MonsterBehavior::STEP_BACK:
		time = 3000ms;
		break;
	case MonsterBehavior::FLEE:
		time = 5000ms;
		break;
	case MonsterBehavior::DELAY:
		time = 1000ms;
		break;
	}
	return time;
}

bool ArcherMonster::DoesAttack()
{
	if ((MonsterBehavior::PREPARE_ATTACK == m_current_behavior) ||
		(MonsterBehavior::ATTACK == m_current_behavior) || 
		(MonsterBehavior::AIM == m_current_behavior) ||
		(MonsterBehavior::FLEE == m_current_behavior))
	{
		return true;
	}
	return false;
}

void ArcherMonster::SetFleeDirection()
{
	Server& server = Server::GetInstance();
	auto game_room = server.GetGameRoomManager()->GetGameRoom(m_room_num);

	XMFLOAT3 sub{};
	FLOAT length{}, min_length{ std::numeric_limits<FLOAT>::max() };
	INT target{ -1 };
	for (int id : game_room->GetMonsterIds()) {
		if (-1 == id) continue;
		if (State::INGAME != server.m_clients[id]->GetState()) continue;
		if (MonsterType::WARRIOR != server.m_clients[id]->GetMonsterType()) continue;

		sub = Vector3::Sub(m_position, server.m_clients[id]->GetPosition());
		length = Vector3::Length(sub);
		if (length < min_length) {
			min_length = length;
			target = id;
		}
	}

	if (-1 != target) {
		m_flee_direction = GetDirection(target);
	}
	else {
		INT angle = random_flee_angle(dre);
		INT percent = random_percent(dre);
		if (percent > 50)
			angle *= -1;

		FLOAT radian = XMConvertToRadians(static_cast<FLOAT>(angle));
		XMVECTOR q = XMQuaternionRotationRollPitchYaw(0.f, radian, 0.f);
		XMFLOAT3 front = GetFront();
		XMStoreFloat3(&m_flee_direction, XMVector3Rotate(XMLoadFloat3(&front), q));
	}
}

WizardMonster::WizardMonster()
{
	m_max_hp = 170.f;
	m_damage = 10.f;
	m_attack_range = 8.f;
	m_boundary_range = 2.5f;
	m_monster_type = MonsterType::WIZARD;
}

void WizardMonster::Init()
{
	Monster::Init();
	m_hp = m_max_hp;
}

void WizardMonster::Update(FLOAT elapsed_time)
{
	Monster::Update(elapsed_time);

	// ���� ���� Ȯ�ο� �ڵ�
	/*Server& server = Server::GetInstance();
	auto game_room = server.GetGameRoomManager()->GetGameRoom(m_room_num);
	auto& ids = game_room->GetPlayerIds();

	for (INT id : ids) {
		if (-1 == id) continue;
		m_pl_random_id.push_back(id);
	}*/

	// ������ ���� ������ ��������
	// ���ݹ��� ���� ������ ���� ����
	if (CanSwapAttackBehavior()) {
		if (IsInRange(m_boundary_range)) {
			ChangeBehavior(MonsterBehavior::PREPARE_ATTACK);
			//RandomTarget();
		}
		else if (IsInRange(m_attack_range)) {
			ChangeBehavior(MonsterBehavior::PREPARE_CAST);
			//RandomTarget();
		}
	}
}

bool WizardMonster::CanSwapAttackBehavior()
{
	if ((MonsterBehavior::CHASE == m_current_behavior) ||
		(MonsterBehavior::RETARGET == m_current_behavior))
	{
		return true;
	}
	return false;
}

MonsterBehavior WizardMonster::SetNextBehavior(MonsterBehavior behavior)
{
	MonsterBehavior temp{};
	switch (behavior) {
	case MonsterBehavior::CHASE: {
		int percent = random_percent(dre);
		if (1 <= percent && percent <= 50) {
			temp = MonsterBehavior::RETARGET;
		}
		else {
			temp = MonsterBehavior::TAUNT;
		}
		break;
	}
	case MonsterBehavior::RETARGET:
		temp = MonsterBehavior::CHASE;
		break;
	case MonsterBehavior::TAUNT:
		temp = MonsterBehavior::CHASE;
		break;
	case MonsterBehavior::PREPARE_ATTACK:
		temp = MonsterBehavior::ATTACK;
		break;
	case MonsterBehavior::ATTACK:
		temp = MonsterBehavior::DELAY;
		break;
	case MonsterBehavior::PREPARE_CAST:
		temp = MonsterBehavior::CAST;
		break;
	case MonsterBehavior::CAST:
		temp = MonsterBehavior::LAUGHING;
		break;
	case MonsterBehavior::LAUGHING:
		temp = MonsterBehavior::DELAY;
		break;
	case MonsterBehavior::DELAY:
		temp = MonsterBehavior::RETARGET;
		break;
	case MonsterBehavior::DEATH:
		temp = MonsterBehavior::REMOVE;
		break;
	default:	// ���� �ൿ�̸� COUNT
		temp = MonsterBehavior::COUNT;
		break;
	}
	return temp;
}

void WizardMonster::SetBehaviorAnimation(MonsterBehavior behavior)
{
	switch (behavior) {
	case MonsterBehavior::CHASE:
		m_current_animation = WizardMonsterAnimation::WALK;
		break;
	case MonsterBehavior::RETARGET:
		m_current_animation = WizardMonsterAnimation::LOOK_AROUND;
		break;
	case MonsterBehavior::TAUNT:
		m_current_animation = WizardMonsterAnimation::TAUNT;
		break;
	case MonsterBehavior::PREPARE_ATTACK:
		m_current_animation = WizardMonsterAnimation::TAUNT;
		break;
	case MonsterBehavior::ATTACK:
		m_current_animation = WizardMonsterAnimation::ATTACK;
		break;
	case MonsterBehavior::DEATH:
		m_current_animation = WizardMonsterAnimation::DEATH;
		break;
	case MonsterBehavior::PREPARE_CAST:
		m_current_animation = WizardMonsterAnimation::PREPARE_CAST;
		break;
	case MonsterBehavior::CAST:
		m_current_animation = WizardMonsterAnimation::CAST;
		break;
	case MonsterBehavior::LAUGHING:
		m_current_animation = WizardMonsterAnimation::LAUGHING;
		break;
	case MonsterBehavior::DELAY:
		m_current_animation = WizardMonsterAnimation::IDLE;
		break;
	}
}

std::chrono::milliseconds WizardMonster::SetBehaviorTime(MonsterBehavior behavior)
{
	using namespace std::literals;

	std::chrono::milliseconds time{};

	switch (behavior) {
	case MonsterBehavior::CHASE:
		time = 3000ms;
		break;
	case MonsterBehavior::RETARGET:
		time = 3000ms;
		break;
	case MonsterBehavior::TAUNT:
		time = 1500ms;
		break;
	case MonsterBehavior::PREPARE_ATTACK:
		time = 1500ms;
		break;
	case MonsterBehavior::ATTACK:
		time = 1700ms;
		break;
	case MonsterBehavior::DEATH:
		time = 2000ms;
		break;
	case MonsterBehavior::PREPARE_CAST:
		time = 3000ms;
		break;
	case MonsterBehavior::CAST:
		time = 2000ms;
		break;
	case MonsterBehavior::LAUGHING:
		time = 4300ms;
		break;
	case MonsterBehavior::DELAY:
		time = 3000ms;
		break;
	}
	return time;
}

BossMonster::BossMonster()
{
	m_bounding_box.Center = XMFLOAT3(6.556f, 1.032f, 2.018f);
	m_bounding_box.Extents = XMFLOAT3(1.648f, 2.214f, 2.018f);
	m_max_hp = 1000.f;
	m_damage = 3.f;
	m_attack_range = 5.f;
	m_boundary_range = 4.f;
	m_monster_type = MonsterType::BOSS;
}

void BossMonster::Init()
{
	Monster::Init();
	m_hp = m_max_hp;
}

void BossMonster::Update(FLOAT elapsed_time)
{
	Monster::Update(elapsed_time);


	Server& server = Server::GetInstance();
	auto game_room = server.GetGameRoomManager()->GetGameRoom(m_room_num);
	auto& ids = game_room->GetPlayerIds();

	for (INT id : ids) {
		if (-1 == id) continue;
		m_pl_random_id.push_back(id);
	}

	if (!(m_hp <= m_max_hp / 2.5)) {                                                        // �Ϲ� ����
		if (CanSwapAttackBehavior()) {                                                      // ��� ���� ���� ������ �Ϲ� ����
			if (IsInRange(m_boundary_range) && m_behavior_cnt == 0) {
				//m_damage = 25.f;
				ChangeBehavior(MonsterBehavior::PREPARE_NORMAL_ATTACK);
				RandomTarget(elapsed_time);
				m_behavior_cnt++;
			}
			else if (IsInRange(m_attack_range) && m_behavior_cnt == 1) {                    // ���� ���� ���� ������ ��ų ����
				//m_damage = 35.f;
				ChangeBehavior(MonsterBehavior::PREPARE_WIDE_SKILL);
				RandomTarget(elapsed_time);
				m_behavior_cnt = 0;
			}
		}
	}     
	else {                                                                                  // ����ȭ ����
		if (CanSwapAttackBehavior()) {                                                      // ��� ���� ���� ������ ��ȭ�� �Ϲ� ����
			if (IsInRange(m_boundary_range) && m_enhance_behavior_cnt == 0) {
				ChangeBehavior(MonsterBehavior::PREPARE_ATTACK);
				PlayerHighestDamageTarget();
				m_enhance_behavior_cnt++;
			}
			else if (IsInRange(m_attack_range) && m_enhance_behavior_cnt == 1) {                    // ���� ���� ���� ������ ��ȭ�� ��ų ����
				ChangeBehavior(MonsterBehavior::PREPARE_ENHANCE_WIDE_SKILL);
				PlayerHighestDamageTarget();
				m_attack_range += 2.f;
				m_enhance_behavior_cnt++;
			}
			else if (IsInRange(m_attack_range) && m_enhance_behavior_cnt == 2) {                    // ���� ���� ���� ������ ���� ��ų ����
				ChangeBehavior(MonsterBehavior::PREPARE_RUCH_SKILL);
				PlayerHighestDamageTarget();
				m_attack_range -= 4.f;
				m_enhance_behavior_cnt++;
			}
			else if (IsInRange(m_attack_range) && m_enhance_behavior_cnt == 3) {                    // ���� ���� ���� ������ �ʻ�� ��ų ����
				ChangeBehavior(MonsterBehavior::PREPARE_ULTIMATE_SKILL);
				PlayerHighestDamageTarget();
				m_enhance_behavior_cnt = 0;
			}
		}
	}
}

bool BossMonster::CanSwapAttackBehavior()
{
	if (MonsterBehavior::CHASE == m_current_behavior)
	{
		return true;
	}
	return false;
}

MonsterBehavior BossMonster::SetNextBehavior(MonsterBehavior behavior)
{
	MonsterBehavior temp{};
	switch (behavior) {
	case MonsterBehavior::CHASE:{
		if (m_hp == m_max_hp / 2.5)
			temp = MonsterBehavior::ENHANCE;
		/*else
			temp = MonsterBehavior::DELAY;*/
		break;
	}
	case MonsterBehavior::PREPARE_ATTACK:
		temp = MonsterBehavior::ATTACK;
		break;
	case MonsterBehavior::ATTACK:
		temp = MonsterBehavior::DELAY;
		break;
	case MonsterBehavior::PREPARE_WIDE_SKILL:
		temp = MonsterBehavior::WIDE_SKILL;
		break;
	case MonsterBehavior::WIDE_SKILL:
		temp = MonsterBehavior::DELAY;
		break;
	case MonsterBehavior::PREPARE_NORMAL_ATTACK:
		temp = MonsterBehavior::NORMAL_ATTACK;
		break;
	case MonsterBehavior::NORMAL_ATTACK:
		temp = MonsterBehavior::DELAY;
		break;
	case MonsterBehavior::PREPARE_ENHANCE_WIDE_SKILL:
		temp = MonsterBehavior::ENHANCE_WIDE_SKILL;
		break;
	case MonsterBehavior::ENHANCE_WIDE_SKILL:
		temp = MonsterBehavior::DELAY;
		break;
	case MonsterBehavior::PREPARE_RUCH_SKILL:
		temp = MonsterBehavior::RUCH_SKILL;
		break;
	case MonsterBehavior::RUCH_SKILL:
		temp = MonsterBehavior::DELAY;
		break;
	case MonsterBehavior::PREPARE_ULTIMATE_SKILL:
		temp = MonsterBehavior::ULTIMATE_SKILL;
		break;
	case MonsterBehavior::ULTIMATE_SKILL:
		temp = MonsterBehavior::DELAY;
		break;
	case MonsterBehavior::DELAY:
		temp = MonsterBehavior::CHASE;
		break;
	case MonsterBehavior::DEATH:
		temp = MonsterBehavior::REMOVE;
		break;
	default:	// ���� �ൿ�̸� COUNT
		temp = MonsterBehavior::COUNT;
		break;
	}
	return temp;
}

void BossMonster::SetBehaviorAnimation(MonsterBehavior behavior)
{
	switch (behavior) {
	case MonsterBehavior::CHASE:
		m_current_animation = BossMonsterAnimation::RUN;
		break;
	case MonsterBehavior::PREPARE_ATTACK:
		m_current_animation = BossMonsterAnimation::IDLE;               // �̶� Idle01 �ִϸ��̼� ������ �ɵ�
		break;
	case MonsterBehavior::ATTACK:
		m_current_animation = BossMonsterAnimation::ATTACK;
		break;
	case MonsterBehavior::PREPARE_WIDE_SKILL:
		m_current_animation = BossMonsterAnimation::IDLE; // �̶��� idle01
		break;
	case MonsterBehavior::WIDE_SKILL:
		m_current_animation = BossMonsterAnimation::WIDE_SKILL;
		break;
	case MonsterBehavior::ENHANCE:
		m_current_animation = BossMonsterAnimation::ENHANCE;
		break;
	case MonsterBehavior::PREPARE_NORMAL_ATTACK:
		m_current_animation = BossMonsterAnimation::IDLE;
		break;
	case MonsterBehavior::NORMAL_ATTACK:
		m_current_animation = BossMonsterAnimation::NORMAL_ATTACK;
		break;
	case MonsterBehavior::PREPARE_ENHANCE_WIDE_SKILL:
		m_current_animation = BossMonsterAnimation::IDLE;
		break;
	case MonsterBehavior::ENHANCE_WIDE_SKILL:
		m_current_animation = BossMonsterAnimation::ENHANCE_WIDE_SKILL;
		break;
	case MonsterBehavior::PREPARE_RUCH_SKILL:
		m_current_animation = BossMonsterAnimation::IDLE;
		break;
	case MonsterBehavior::RUCH_SKILL:
		m_current_animation = BossMonsterAnimation::RUCH_SKILL;
		break;
	case MonsterBehavior::PREPARE_ULTIMATE_SKILL:
		m_current_animation = BossMonsterAnimation::IDLE;
		break;
	case MonsterBehavior::ULTIMATE_SKILL:
		m_current_animation = BossMonsterAnimation::ULTIMATE_SKILL;
		break;
	case MonsterBehavior::DEATH:
		m_current_animation = BossMonsterAnimation::DEATH;
		break;
	case MonsterBehavior::DELAY:
		m_current_animation = BossMonsterAnimation::IDLE;               // �̶��� idle01
		break;
	}
}

std::chrono::milliseconds BossMonster::SetBehaviorTime(MonsterBehavior behavior)
{
	using namespace std::literals;

	std::chrono::milliseconds time{};

	switch (behavior) {
	case MonsterBehavior::CHASE:
		time = 700ms;
		break;
	case MonsterBehavior::PREPARE_ATTACK:
		time = 700ms;
		break;
	case MonsterBehavior::ATTACK:
		time = 900ms;
		break;
	case MonsterBehavior::PREPARE_WIDE_SKILL:
		time = 700ms;
		break;
	case MonsterBehavior::WIDE_SKILL:
		time = 1466ms;
		break;
	case MonsterBehavior::ENHANCE:
		time = 4233ms;
		break;
	case MonsterBehavior::PREPARE_NORMAL_ATTACK:
		time = 400ms;
		break;
	case MonsterBehavior::NORMAL_ATTACK:
		time = 900ms;
		break;
	case MonsterBehavior::PREPARE_ENHANCE_WIDE_SKILL:
		time = 400ms;
		break;
	case MonsterBehavior::ENHANCE_WIDE_SKILL:
		time = 1066ms;
		break;
	case MonsterBehavior::PREPARE_RUCH_SKILL:
		time = 400ms;
		break;
	case MonsterBehavior::RUCH_SKILL:
		time = 700ms;
		break;
	case MonsterBehavior::PREPARE_ULTIMATE_SKILL:
		time = 400ms;
		break;
	case MonsterBehavior::ULTIMATE_SKILL:
		time = 2066ms;
		break;
	case MonsterBehavior::DELAY:
		time = 1900ms;
		break;
	case MonsterBehavior::DEATH:
		time = 2233ms;
		break;
	}
	return time;
}

void BossMonster::DecreaseHp(FLOAT damage, INT id)
{
	Server& server = Server::GetInstance();

	if (m_hp <= 0)
		return;

	m_hp -= damage;
	server.m_clients[id]->SetSaveDamage(0);

	if (m_hp <= 0) {
		m_hp = 0.f;
		{
			std::lock_guard<std::mutex> l{ m_state_lock };
			m_state = State::DEATH;
		}
		// ������ ����
		ChangeBehavior(MonsterBehavior::DEATH);
		return;
	}

	m_pl_save_damage.push_back(std::make_pair(id, server.m_clients[id]->GetSaveDamage()));

	auto pl_save_data = std::find_if(m_pl_save_damage.begin(), m_pl_save_damage.end(), [id](const m_save_damage_pair& pair) {
		return pair.first == id;
		});

	if (pl_save_data != m_pl_save_damage.end()) {
		// �̹� ����� ���, �ش� �÷��̾��� ���� �������� ������Ʈ
		pl_save_data->second += damage;
	}
	else {
		// ����Ǿ� ���� ���� ���, ���ο� �÷��̾� ������ �߰�
		m_pl_save_damage.push_back(std::make_pair(id, server.m_clients[id]->GetSaveDamage()));
	}

	//std::cout << id << "�÷��̾��� ���� ������ - " << (FLOAT)pl_save_data->second << std::endl;

	if (AggroLevel::HIT_AGGRO < m_aggro_level)
		return;
}

void BossMonster::RandomTarget(FLOAT elapsed_time)
{
	std::random_device rd;
	std::mt19937 gen(rd());

	std::shuffle(m_pl_random_id.begin(), m_pl_random_id.end(), gen);

	INT random_id = *m_pl_random_id.begin();

	SetTarget(random_id);
	std::cout << "�������� �߰ݴ��ϴ� �÷��̾� - " << random_id << std::endl;
}

void BossMonster::PlayerHighestDamageTarget()
{
	INT pl_highest_damage_id{};

	if (m_hp <= m_max_hp / 2.5) {
		pl_highest_damage_id = GetPlayerHighestDamage();
		SetTarget(pl_highest_damage_id);
		//std::cout << "����� �÷��̾� id - " << pl_highest_damage_id << std::endl;
	}
}

