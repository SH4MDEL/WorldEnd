#pragma once
#include <DirectXMath.h>
#include <chrono>

constexpr short SERVER_PORT = 9000;

constexpr int BUF_SIZE = 5000;
constexpr int NAME_SIZE = 20;
constexpr int MAX_INGAME_USER = 3;
constexpr int MAX_INGAME_MONSTER = 10;
constexpr int MAX_GAME_ROOM_NUM = 3000;
constexpr int MAX_PARTY_NUM = 1000;
constexpr int MAX_RECORD_NUM = 5;
constexpr int MAX_MONSTER_PLACEMENT = 165;

constexpr int MAX_USER = 10000;
constexpr int MAX_WARRIOR_MONSTER = 30000;
constexpr int MAX_ARCHER_MONSTER = 30000;
constexpr int MAX_WIZARD_MONSTER = 30000;
constexpr int MAX_BOSS_MONSTER = 30000;
constexpr int MAX_OBJECT = MAX_USER + MAX_WARRIOR_MONSTER + MAX_ARCHER_MONSTER + MAX_WIZARD_MONSTER + MAX_BOSS_MONSTER;

constexpr int WARRIOR_MONSTER_START = MAX_USER;
constexpr int WARRIOR_MONSTER_END = MAX_USER + MAX_WARRIOR_MONSTER;

constexpr int ARCHER_MONSTER_START = WARRIOR_MONSTER_END;
constexpr int ARCHER_MONSTER_END = WARRIOR_MONSTER_END + MAX_ARCHER_MONSTER;

constexpr int WIZARD_MONSTER_START = ARCHER_MONSTER_END;
constexpr int WIZARD_MONSTER_END = ARCHER_MONSTER_END + MAX_WIZARD_MONSTER;

constexpr int BOSS_MONSTER_START = WIZARD_MONSTER_END;
constexpr int BOSS_MONSTER_END = WIZARD_MONSTER_END + MAX_BOSS_MONSTER;

constexpr int MAX_ARROW_RAIN = 100;
constexpr int MAX_UNDEAD_GRASP = 100;

constexpr int ARROW_RAIN_START = 0;
constexpr int ARROW_RAIN_END = MAX_ARROW_RAIN;

constexpr int UNDEAD_GRASP_START = ARROW_RAIN_END;
constexpr int UNDEAD_GRASP_END = ARROW_RAIN_END + MAX_UNDEAD_GRASP;

constexpr int MAX_TRIGGER = MAX_ARROW_RAIN + MAX_UNDEAD_GRASP;


constexpr char CS_PACKET_LOGIN = 1;
constexpr char CS_PACKET_PLAYER_MOVE = 2;
constexpr char CS_PACKET_SET_COOLDOWN = 4;
constexpr char CS_PACKET_ATTACK = 5;
constexpr char CS_PACKET_SHOOT = 6;
constexpr char CS_PACKET_CHANGE_ANIMATION = 7;
constexpr char CS_PACKET_CHANGE_STAMINA = 8;
constexpr char CS_PACKET_INTERACT_OBJECT = 9;

constexpr char SC_PACKET_LOGIN_OK = 1;
constexpr char SC_PACKET_ADD_OBJECT = 2;
constexpr char SC_PACKET_REMOVE_PLAYER = 3;
constexpr char SC_PACKET_REMOVE_MONSTER = 4;
constexpr char SC_PACKET_UPDATE_CLIENT = 5;
constexpr char SC_PACKET_ADD_MONSTER = 6;
constexpr char SC_PACKET_UPDATE_MONSTER = 7;
constexpr char SC_PACKET_CHANGE_MONSTER_BEHAVIOR = 8;
constexpr char SC_PACKET_CHANGE_ANIMATION = 9;
constexpr char SC_PACKET_RESET_COOLDOWN = 10;
constexpr char SC_PACKET_CLEAR_FLOOR = 11;
constexpr char SC_PACKET_FAIL_FLOOR = 12;
constexpr char SC_PACKET_MONSTER_HIT = 13;
constexpr char SC_PACKET_CHANGE_STAMINA = 14;
constexpr char SC_PACKET_MONSTER_ATTACK_COLLISION = 15;
constexpr char SC_PACKET_SET_INTERACTABLE = 16;
constexpr char SC_PACKET_START_BATTLE = 17;
constexpr char SC_PACKET_WARP_NEXT_FLOOR = 18;
constexpr char SC_PACKET_PLAYER_DEATH = 19;
constexpr char SC_PACKET_ARROW_SHOOT = 20;
constexpr char SC_PACKET_REMOVE_ARROW = 21;
constexpr char SC_PACKET_MONSTER_SHOOT = 22;
constexpr char SC_PACKET_INTERACT_OBJECT = 23;
constexpr char SC_PACKET_CHANGE_HP = 24;
constexpr char SC_PACKET_ADD_TRIGGER = 25;
constexpr char SC_PACKET_ADD_MAGIC_CIRCLE = 26;

enum class PlayerType : char { WARRIOR, ARCHER, COUNT };
enum class MonsterType : char { WARRIOR, ARCHER, WIZARD, BOSS, COUNT};
enum class EnvironmentType : char { RAIN, FOG, GAS, TRAP, COUNT };

enum ActionType : char {
	NORMAL_ATTACK, SKILL, ULTIMATE, DASH, ROLL, COUNT
};
enum class MonsterBehavior : char {
	CHASE, RETARGET, TAUNT, PREPARE_ATTACK, ATTACK, DEATH, REMOVE,	// 공용
	BLOCK, BLOCKIDLE,				// 전사 몬스터
	AIM, STEP_BACK, FLEE, DELAY,	// 궁수 몬스터
	PREPARE_CAST, CAST,	LAUGHING,	// 마법사 몬스터
	PREPARE_WIDE_SKILL, WIDE_SKILL, ENHANCE,                           // 보스 몬스터
	PREPARE_ENHANCE_ATTACK, ENHANCE_ATTACK,                            // 보스 몬스터
	PREPARE_ENHANCE_WIDE_SKILL, ENHANCE_WIDE_SKILL,                    // 보스 몬스터
	PREPARE_LETHAL_MOVE, LETHAL_MOVE,                                  // 보스 몬스터
	COUNT
};
enum InteractionType : char {
	BATTLE_STARTER, PORTAL, ENHANCMENT, RECORD_BOARD, NONE
};

// ----- 애니메이션 enum 클래스 -----
// 애니메이션이 100개 이하로 떨어질 것이라 생각하여 100을 단위로 잡음
class ObjectAnimation
{
public:
	enum USHORT {
		IDLE, WALK, RUN, ATTACK, DEATH,
		END
	};
};

class PlayerAnimation : public ObjectAnimation
{
public:
	static constexpr int ANIMATION_START = 100;
	enum USHORT {
		DASH = ObjectAnimation::END + ANIMATION_START,
		SKILL, ULTIMATE, ROLL
	};
};

class MonsterAnimation : public ObjectAnimation
{
public:
	static constexpr int ANIMATION_START = 200;
	enum USHORT {
		LOOK_AROUND = ObjectAnimation::END + ANIMATION_START,
		TAUNT, END
	};
};

class WarriorMonsterAnimation : public MonsterAnimation
{
public:
	static constexpr int ANIMATION_START = 300;
	enum USHORT {
		BLOCK = MonsterAnimation::END - MonsterAnimation::ANIMATION_START + ANIMATION_START,
		BLOCKIDLE
	};
};

class ArcherMonsterAnimation : public MonsterAnimation
{
public:
	static constexpr int ANIMATION_START = 400;
	enum USHORT {
		DRAW = MonsterAnimation::END - MonsterAnimation::ANIMATION_START + ANIMATION_START,
		AIM, WALK_BACKWARD, FLEE
	};
};

class WizardMonsterAnimation : public MonsterAnimation
{
public:
	static constexpr int ANIMATION_START = 500;
	enum USHORT {
		PREPARE_CAST = MonsterAnimation::END - MonsterAnimation::ANIMATION_START + ANIMATION_START,
		CAST, LAUGHING, 
	};
};

class BossMonsterAnimation : public MonsterAnimation
{
public:
	static constexpr int ANIMATION_START = 400;
	enum USHORT {
		PREPARE_WIDE_SKILL = MonsterAnimation::END - MonsterAnimation::ANIMATION_START + ANIMATION_START,
		WIDE_SKILL,
	};
};

// ----------------------------------

enum class TriggerType : UCHAR {
	ARROW_RAIN, UNDEAD_GRASP, COUNT
};

namespace TriggerSetting
{
	using namespace std::literals;

	constexpr std::chrono::milliseconds
		GENTIME[static_cast<int>(TriggerType::COUNT)]{
			0ms, 1000ms
		};

	constexpr std::chrono::milliseconds 
		COOLDOWN[static_cast<int>(TriggerType::COUNT)]{
			600ms, 0ms
		};

	constexpr std::chrono::milliseconds
		DURATION[static_cast<int>(TriggerType::COUNT)]{
			4000ms, 300ms
		};

	constexpr DirectX::XMFLOAT3
		EXTENT[static_cast<int>(TriggerType::COUNT)]{
			DirectX::XMFLOAT3{ 3.f, 3.f, 3.f },
			DirectX::XMFLOAT3{ 1.f, 1.f, 1.f }
		};

	constexpr float ARROWRAIN_DIST = EXTENT[static_cast<int>(TriggerType::ARROW_RAIN)].x + 2.f;
}

namespace PlayerSetting
{
	using namespace std::literals;

	constexpr float WALK_SPEED = 4.f;
	constexpr float	RUN_SPEED = 10.f;
	constexpr float	DASH_SPEED = 12.f;
	constexpr float ROLL_SPEED = 10.f;
	constexpr float ARROW_SPEED = 15.5f;
	constexpr float AUTO_TARGET_RANGE = 12.5f;
	constexpr float ARROW_RANGE = 12.f;

	constexpr auto DASH_DURATION = 300ms;
	constexpr float MAX_STAMINA = 120.f;
	constexpr float MINIMUM_DASH_STAMINA = 10.f;
	constexpr float MINIMUM_ROLL_STAMINA = 15.f;
	constexpr float DEFAULT_STAMINA_CHANGE_AMOUNT = 10.f;
	constexpr float ROLL_STAMINA_CHANGE_AMOUNT = 15.f;

	constexpr auto ROLL_COOLDOWN = 3s;
	constexpr auto DASH_COOLDOWN = 1200ms;

	constexpr float SKILL_RATIO[static_cast<int>(PlayerType::COUNT)]{ 1.5f, 1.2f };
	constexpr float ULTIMATE_RATIO[static_cast<int>(PlayerType::COUNT)]{ 2.f, 0.2f };

	constexpr std::chrono::milliseconds
		ATTACK_COLLISION_TIME[static_cast<int>(PlayerType::COUNT)]{ 210ms, 360ms };
	constexpr std::chrono::milliseconds
		SKILL_COLLISION_TIME[static_cast<int>(PlayerType::COUNT)]{ 340ms, 560ms };
	constexpr std::chrono::milliseconds
		ULTIMATE_COLLISION_TIME[static_cast<int>(PlayerType::COUNT)]{ 930ms, 860ms };
	constexpr std::chrono::milliseconds
		ATTACK_COOLDOWN[static_cast<int>(PlayerType::COUNT)]{ 1000ms, 1000ms };
	constexpr std::chrono::seconds
		SKILL_COOLDOWN[static_cast<int>(PlayerType::COUNT)]{ 7s, 5s };
	constexpr std::chrono::seconds
		ULTIMATE_COOLDOWN[static_cast<int>(PlayerType::COUNT)]{ 20s, 15s };
	constexpr DirectX::XMFLOAT3
		ULTIMATE_EXTENT[static_cast<int>(PlayerType::COUNT)]{
			DirectX::XMFLOAT3{ 2.f, 2.f, 2.f },
			DirectX::XMFLOAT3{ 0.f, 0.f, 0.f }
		};

}

namespace MonsterSetting 
{
	using namespace std::literals;

	constexpr float WALK_SPEED = 3.f;
	constexpr float STEP_BACK_SPEED = 1.5f;
	constexpr float FLEE_SPEED = 4.5f;
	constexpr float ARROW_SPEED = 18.f;
	constexpr float RECOGNIZE_RANGE = 26.f;
	constexpr float ARROW_RANGE = 15.f;

	constexpr auto DECREASE_AGRO_LEVEL_TIME = 10s;

	constexpr float ATTACK_RANGE[static_cast<int>(MonsterType::COUNT)]{
		1.5f, 12.f, 8.f };
	constexpr float BOUNDARY_RANGE[static_cast<int>(MonsterType::COUNT)]{
		2.f, 6.f, 2.f };
	constexpr std::chrono::milliseconds
		ATK_COLLISION_TIME[static_cast<int>(MonsterType::COUNT)]{ 300ms, 0ms, 1100ms };
	constexpr std::chrono::milliseconds CAST_COLLISION_TIME = 430ms;
}

namespace RoomSetting
{
	using namespace std::literals;

	constexpr auto RESET_TIME = 5s;
	constexpr float DEFAULT_HEIGHT = 0.f;
	constexpr unsigned char BOSS_FLOOR = 5;
	constexpr int MAX_ARROWS = 30;
	constexpr int MAX_ARROWRAINS = 3;
	constexpr auto ARROW_REMOVE_TIME = 3s;

	constexpr float DOWNSIDE_STAIRS_HEIGHT = 4.42f;
	constexpr float DOWNSIDE_STAIRS_FRONT = -7.f;
	constexpr float DOWNSIDE_STAIRS_BACK = -17.f;

	constexpr float TOPSIDE_STAIRS_HEIGHT = 4.67f;
	constexpr float TOPSIDE_STAIRS_FRONT = 53.f;
	constexpr float TOPSIDE_STAIRS_BACK = 43.f;

	constexpr auto BATTLE_DELAY_TIME = 3s;
	constexpr auto WARP_DELAY_TIME = 1000ms;
	constexpr float EVENT_RADIUS = 1.f;
	constexpr DirectX::XMFLOAT3 START_POSITION { 0.f, -DOWNSIDE_STAIRS_HEIGHT, -45.f };
	//constexpr DirectX::XMFLOAT3 START_POSITION{ 0.f, 0.f, 0.f };
	constexpr DirectX::XMFLOAT3 BATTLE_STARTER_POSITION { 0.f, 0.f, 24.f };
	constexpr DirectX::XMFLOAT3 WARP_PORTAL_POSITION { -1.f, TOPSIDE_STAIRS_HEIGHT, 60.f };
}

#pragma pack(push,1)
struct PLAYER_DATA
{
	INT					id;				// 플레이어 고유 번호
	DirectX::XMFLOAT3	pos;			// 위치
	DirectX::XMFLOAT3	velocity;		// 속도
	FLOAT				yaw;			// 회전각
	FLOAT               hp;
};

struct ARROW_DATA    
{
	DirectX::XMFLOAT3	pos;		// 위치
	DirectX::XMFLOAT3	velocity;	// 방향
	INT					player_id;	// 쏜 사람
};

struct MONSTER_DATA
{
	INT					id;			// 몬스터 고유 번호
	DirectX::XMFLOAT3	pos;		// 위치
	DirectX::XMFLOAT3	velocity;	// 속도
	FLOAT				yaw;		// 회전각
	FLOAT				hp;
};
//////////////////////////////////////////////////////
// 클라에서 서버로

struct CS_LOGIN_PACKET 
{
	UCHAR size;
	UCHAR type;
	CHAR name[NAME_SIZE];
	PlayerType player_type;
};

struct CS_LOGOUT_PACKET 
{
	UCHAR size;
	UCHAR type;
};


struct CS_PLAYER_MOVE_PACKET
{
	UCHAR size;
	UCHAR type;
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 velocity;
	FLOAT yaw;
#ifdef USER_NUM_TEST
	UINT move_time;
#endif
};

struct CS_READY_PACKET      // 파티 준비 완료를 알려주는 패킷
{
	UCHAR size;
	UCHAR type;
	bool ready_check;
};

struct CS_COOLDOWN_PACKET
{
	UCHAR size;
	UCHAR type;
	ActionType cooldown_type;
};

struct CS_ARROW_PACKET       // 공격키를 눌렀을때 투사체를 생성해주는 패킷
{
	UCHAR size;
	UCHAR type;
	ARROW_DATA arrow_data;
};

struct CS_ATTACK_PACKET
{
	UCHAR size;
	UCHAR type;
	ActionType attack_type;
	std::chrono::system_clock::time_point attack_time; 
};

struct CS_SHOOT_PACKET
{
	UCHAR size;
	UCHAR type;
	ActionType attack_type;
	std::chrono::system_clock::time_point attack_time;
};

struct CS_CHANGE_ANIMATION_PACKET
{
	UCHAR size;
	UCHAR type;
	USHORT animation;
};

struct CS_CHANGE_STAMINA_PACKET
{
	UCHAR size;
	UCHAR type;
	bool is_increase;
};

struct CS_INTERACT_OBJECT_PACKET
{
	UCHAR size;
	UCHAR type;
	InteractionType interaction_type;
};

///////////////////////////////////////////////////////////////////////
// 서버에서 클라로

struct SC_LOGIN_OK_PACKET    // 로그인 성공을 알려주는 패킷
{
	UCHAR size;
	UCHAR type;
	CHAR  name[NAME_SIZE];
	PLAYER_DATA player_data;
	PlayerType player_type;
};

struct SC_ADD_OBJECT_PACKET
{
	UCHAR size;
	UCHAR type;
	CHAR  name[NAME_SIZE];
	PLAYER_DATA player_data;
	PlayerType player_type;
};

struct SC_REMOVE_PLAYER_PACKET
{
	UCHAR size;
	UCHAR type;
	INT id;
};

struct SC_REMOVE_MONSTER_PACKET
{
	UCHAR size;
	UCHAR type;
	INT id;
};

struct SC_LOGIN_FAIL_PACKET  // 로그인 실패를 알려주는 패킷
{
	UCHAR size;
	UCHAR type;
};

struct SC_READY_CHECK_PACKET  // 파티 준비 상태를 알려주는 패킷
{
	UCHAR size;
	UCHAR type;
	CHAR id;
	bool ready_check;
};

struct SC_PLAYER_SELECT_PACKET // 플레이어 종류 선택 패킷 
{
	UCHAR size;
	UCHAR type;
	CHAR id;
	PlayerType player_type;
};

struct SC_ARROW_DATA_PACKET    // 투사체 정보를 보내주는 패킷
{
	UCHAR size;
	UCHAR type;
	ARROW_DATA arrow_data;
};

struct SC_UPDATE_CLIENT_PACKET
{
	UCHAR size;
	UCHAR type;
	PLAYER_DATA	data;
#ifdef USER_NUM_TEST
	UINT move_time;
#endif
};

struct SC_ADD_MONSTER_PACKET
{
	UCHAR size;
	UCHAR type;
	MONSTER_DATA monster_data;
	MonsterType monster_type;
};

struct SC_UPDATE_MONSTER_PACKET
{
	UCHAR size;
	UCHAR type;
	MONSTER_DATA monster_data;
};

struct SC_CHANGE_MONSTER_BEHAVIOR_PACKET
{
	UCHAR size;
	UCHAR type;
	INT id;
	MonsterBehavior behavior;
	USHORT animation;
};

struct SC_CHANGE_ANIMATION_PACKET
{
	UCHAR size;
	UCHAR type;
	INT id;
	USHORT animation;
};

struct SC_RESET_COOLDOWN_PACKET
{
	UCHAR size;
	UCHAR type;
	ActionType cooldown_type;
};

struct SC_CLEAR_FLOOR_PACKET
{
	UCHAR size;
	UCHAR type;
	USHORT reward;
};

struct SC_FAIL_FLOOR_PACKET
{
	UCHAR size;
	UCHAR type;
};

struct SC_CREATE_MONSTER_HIT
{
	UCHAR size;
	UCHAR type;
	INT id;		// 몬스터 id
	FLOAT hp;	// 체력
};

struct SC_CHANGE_STAMINA_PACKET
{
	UCHAR size;
	UCHAR type;
	FLOAT stamina;
};

struct SC_MONSTER_ATTACK_COLLISION_PACKET
{
	UCHAR size;
	UCHAR type;
	INT ids[MAX_INGAME_USER];
	FLOAT hps[MAX_INGAME_USER];
};

struct SC_SET_INTERACTABLE_PACKET
{
	UCHAR size;
	UCHAR type;
	bool interactable;
	InteractionType interactable_type;
};

struct SC_START_BATTLE_PACKET
{
	UCHAR size;
	UCHAR type;
};

struct SC_WARP_NEXT_FLOOR_PACKET
{
	UCHAR size;
	UCHAR type;
	BYTE floor;
	// 플레이어의 초기 장소까지 같이 알려줘야 하는지
};

struct SC_PLAYER_DEATH_PACKET
{
	UCHAR size;
	UCHAR type;
	INT id;
};

struct SC_ARROW_SHOOT_PACKET
{
	UCHAR size;
	UCHAR type;
	INT id;
	INT arrow_id;
	ActionType action_type;
};

struct SC_REMOVE_ARROW_PACKET
{
	UCHAR size;
	UCHAR type;
	INT arrow_id;
};

struct SC_INTERACT_OBJECT_PACKET
{
	UCHAR size;
	UCHAR type;
	InteractionType interaction_type;
};

struct SC_CHANGE_HP_PACKET
{
	UCHAR size;
	UCHAR type;
	INT id;
	FLOAT hp;
};

struct SC_ADD_TRIGGER_PACKET
{
	UCHAR size;
	UCHAR type;
	TriggerType trigger_type;
	DirectX::XMFLOAT3 pos;
};

struct SC_ADD_MAGIC_CIRCLE_PACKET
{
	UCHAR size;
	UCHAR type;
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 extent;
};

#pragma pack (pop)


