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


// ------------------------- 디버그 용 ---------------------------------
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
	default:	// 없는 행동이면 COUNT
		return std::string("x");
		break;
	}
}
// ------------------------- 디버그 용 ---------------------------------


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
	m_last_behavior_id = 1;	// 0은 유효하지 않은 행동 id
	m_room_num = -1;	// FREE 되자마자 몬스터 가져갈 시 
						// 타이머 이벤트가 남아있을 수 있으므로 초기화
}

void Monster::Update(FLOAT elapsed_time)
{
	if (State::INGAME != m_state) return;

	DoBehavior(elapsed_time);

	Server& server = Server::GetInstance();
	auto game_room = server.GetGameRoomManager()->GetGameRoom(m_room_num);
	game_room->CheckTriggerCollision(m_id);
}

void Monster::UpdatePosition(const XMFLOAT3& dir, FLOAT elapsed_time, FLOAT speed)
{
	m_velocity = Vector3::Mul(dir, speed);

	// 몬스터 이동
	m_position = Vector3::Add(m_position, Vector3::Mul(m_velocity, elapsed_time));

	m_bounding_box.Center = m_position;
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

	// degree - 클라이언트에서 degree 로 Rotation 을 처리함
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
	
	// 몬스터별로 행동에 따라 애니메이션 설정
	SetBehaviorAnimation(behavior);

	TIMER_EVENT ev{.obj_id = m_id, .event_type = EventType::BEHAVIOR_CHANGE,
	.aggro_level = m_aggro_level};

	if (std::numeric_limits<BYTE>::max() == m_last_behavior_id)
		m_last_behavior_id = 0;
	ev.latest_id = ++m_last_behavior_id;

	// 몬스터별로 행동에 따라 다음 행동 설정
	ev.next_behavior_type = SetNextBehavior(behavior);
	if (MonsterBehavior::COUNT == ev.next_behavior_type) {
		return;
	}

	// 몬스터별로 행동에 따라 얼마나 행동을 지속할 지 결정
	ev.event_time = std::chrono::system_clock::now() + SetBehaviorTime(behavior);


	Server& server = Server::GetInstance();
	server.SetTimerEvent(ev);
}

void Monster::SetTarget(INT id)
{
	m_target_id = id;
}

void Monster::SetAggroLevel(BYTE aggro_level)
{
	// 변경하려는 어그로 레벨이 더 클때만 변경하고 타이머 이벤트 생성
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

		// 전사 몬스터 전용
	case MonsterBehavior::BLOCK:

		break;
	case MonsterBehavior::BLOCKIDLE:

		break;

		// 궁수 몬스터 전용
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

		// 마법사 몬스터
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
	case MonsterBehavior::LAUGHING:
		// 행동 X
		break;

	default:
		return;
	}
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
		// 죽은것 전송
		ChangeBehavior(MonsterBehavior::DEATH);
		return;
	}

	if (AggroLevel::HIT_AGGRO < m_aggro_level)
		return;

	// 일정 비율 이상 데미지가 들어오면 타겟 변경
	if ((m_max_hp / 11.f - damage) <= std::numeric_limits<FLOAT>::epsilon()) {
		SetTarget(id);
		SetAggroLevel(AggroLevel::HIT_AGGRO);
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
	// 타겟이 없거나, 타겟이 room 에 없거나, INGAME 이 아니면 false
	if (-1 == m_target_id)
		return false;

	Server& server = Server::GetInstance();
	
	// 타겟 플레이어가 접속 종료되면 추격 X
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

	// 타게팅한 플레이어 추격
	XMFLOAT3 player_dir = GetDirection(m_target_id);

	if (Vector3::Equal(player_dir, XMFLOAT3(0.f, 0.f, 0.f)))
		return;

	UpdatePosition(player_dir, elapsed_time, MonsterSetting::WALK_SPEED);
	UpdateRotation(player_dir);
	CollisionCheck();
}

void Monster::Retarget()
{
	// 타게팅 변경 중 두리번 거리는 행동
	// 계속 추격하지 않고 일정 시간마다 멈춰서도록 함
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

	// 공격 전 공격함을 알리기 위한 행동

	XMFLOAT3 player_dir = GetDirection(m_target_id);
	UpdateRotation(player_dir);
}

void Monster::Attack()
{
	if (!CheckPlayer()) {
		ChangeBehavior(MonsterBehavior::RETARGET);
	}
	// 플레이어 공격

}

void Monster::CollisionCheck()
{
	Server& server = Server::GetInstance(); 
	
	server.GameRoomObjectCollisionCheck(shared_from_this(), m_room_num);
}

void Monster::InitializePosition(INT mon_cnt, MonsterType mon_type, INT random_map)
{
	// 파일을 읽어서 초기 위치를 정하는 함수 필요
	// 던전 매니저에서 해야하는 일이므로 나중에 던전매니저로 옮겨야 하는 함수
	// 던전 매니저는 지형지물, 몬스터 배치 등의 던전 정보들을 관리함
	using namespace std;

	ifstream in{ "MonsterPos.txt" };

	float mon_pos[MAX_MONSTER_PLACEMENT]{};

	for (int i = 0; i < MAX_MONSTER_PLACEMENT; ++i) {
		in >> mon_pos[i];
	}

	Server& server = Server::GetInstance();

	if (random_map == 0)
	{
		if (mon_type == MonsterType::WARRIOR)
			SetPosition(mon_pos[(mon_cnt * 2) + 2], 0, mon_pos[(mon_cnt * 2) + 3]);
		else if (mon_type == MonsterType::ARCHER)
			SetPosition(mon_pos[(mon_cnt * 2) + 6], 0, mon_pos[(mon_cnt * 2) + 7]);
		else if (mon_type == MonsterType::WIZARD)
			SetPosition(mon_pos[(mon_cnt * 2) + 8], 0, mon_pos[(mon_cnt * 2) + 9]);
	}
	else if (random_map == 1)
	{
		if (mon_type == MonsterType::WARRIOR)
			SetPosition(mon_pos[(mon_cnt * 2) + 22], 0, mon_pos[(mon_cnt * 2) + 23]);
		else if (mon_type == MonsterType::ARCHER)
			SetPosition(mon_pos[(mon_cnt * 2) + 24], 0, mon_pos[(mon_cnt * 2) + 25]);
		else if (mon_type == MonsterType::WIZARD)
			SetPosition(mon_pos[(mon_cnt * 2) + 28], 0, mon_pos[(mon_cnt * 2) + 29]);
	}
}

WarriorMonster::WarriorMonster()
{
	m_max_hp = 2.f;
	m_damage = 20;
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

	// 공격 범위 내로 들어왔을 때 공격 전환이 가능하면 공격 전환
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
	// 배회 추가 필요
	// 배회중 일정 시간이 지나면 공격 대기로 진입
	// 공격 대기는 머리위로 칼을 들어올리고 기모으는 상태
	// 해당 상태에서 일정시간 지나면 칼을 내려치면서 공격하도록 해야 함

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
			temp = MonsterBehavior::PREPARE_ATTACK;	// 공격 이후 아직 공격 가능하면 
													// 배회할 것
													// 아직은 배회가 없으므로 공격 대기
		}
		else {
			temp = MonsterBehavior::CHASE;
		}
		break;
	case MonsterBehavior::DEATH:
		temp = MonsterBehavior::REMOVE;
		break;
	default:	// 없는 행동이면 COUNT
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
	m_max_hp = 2.f;
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

	// 경계 범위 내로 들어왔을 때 공격중이 아니라면 도망

	if (IsInRange(m_boundary_range)) {
		if (!DoesAttack()) {
			if (MonsterBehavior::STEP_BACK != m_current_behavior) {
				ChangeBehavior(MonsterBehavior::STEP_BACK);
				m_step_back_time = 0.f;
				m_max_step_back_time = random_step_back_time(dre);
			}
		}
	}
	// 공격 범위 내로 들어왔을 때 공격으로 전환이 가능하면 공격 전환
	else if (IsInRange(m_attack_range)) {
		if (CanSwapAttackBehavior()) {
			ChangeBehavior(MonsterBehavior::PREPARE_ATTACK);
		}
		// 뒷걸음질 치던 도중 일정 거리 벌어지면 대기로 전환
		else if (MonsterBehavior::STEP_BACK == m_current_behavior) {
			if (!IsInRange(m_recover_attack_range)) {
				ChangeBehavior(MonsterBehavior::DELAY);
			}
		}
	}

	// 뒷걸음질 치던 도중 일정시간이 지나면 도망
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

	// 타게팅한 플레이어 추격
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
	
	// 전사 몬스터가 있다면 전사 몬스터 쪽으로 회피
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
			temp = MonsterBehavior::DELAY;	// 공격 후 딜레이
			// 공격이 빗나가면 일정 확률로 TAUNT(Stomp) 행동 진입 필요
			// (빗나간것에 대해 화나서 발구르기)
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
	default:	// 없는 행동이면 COUNT
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
		m_current_animation = ArcherMonsterAnimation::IDLE;
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
		time = 700ms;
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
	m_max_hp = 2.f;
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

	// 경계범위 내에 들어오면 근접공격
	// 공격범위 내에 들어오면 마법 공격
	if (CanSwapAttackBehavior()) {
		if (IsInRange(m_boundary_range)) {
			ChangeBehavior(MonsterBehavior::PREPARE_ATTACK);
		}
		else if (IsInRange(m_attack_range)) {
			ChangeBehavior(MonsterBehavior::PREPARE_CAST);
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
	default:	// 없는 행동이면 COUNT
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
		time = 4000ms;
		break;
	case MonsterBehavior::DELAY:
		time = 3000ms;
		break;
	}
	return time;
}
