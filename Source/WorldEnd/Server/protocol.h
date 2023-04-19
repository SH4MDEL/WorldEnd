﻿#pragma once
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

constexpr int MAX_USER = 10000;
constexpr int MAX_WARRIOR_MONSTER = 30000;
constexpr int MAX_ARCHER_MONSTER = 30000;
constexpr int MAX_WIZARD_MONSTER = 30000;
constexpr int MAX_OBJECT = MAX_USER + MAX_WARRIOR_MONSTER + MAX_ARCHER_MONSTER + MAX_WIZARD_MONSTER;

constexpr int WARRIOR_MONSTER_START = MAX_USER;
constexpr int WARRIOR_MONSTER_END = MAX_USER + MAX_WARRIOR_MONSTER;

constexpr int ARCHER_MONSTER_START = WARRIOR_MONSTER_END;
constexpr int ARCHER_MONSTER_END = WARRIOR_MONSTER_END + MAX_ARCHER_MONSTER;

constexpr int WIZARD_MONSTER_START = ARCHER_MONSTER_END;
constexpr int WIZARD_MONSTER_END = ARCHER_MONSTER_END + MAX_WIZARD_MONSTER;

constexpr int MAX_ARROW_RAIN = 10;


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
constexpr char SC_PACKET_PLAYER_SHOOT = 20;
constexpr char SC_PACKET_REMOVE_ARROW = 21;

enum class PlayerType : char { WARRIOR, ARCHER, COUNT };
enum class MonsterType : char { WARRIOR, ARCHER, WIZARD, COUNT };
enum class EnvironmentType : char { RAIN, FOG, GAS, TRAP, COUNT };

enum ActionType : char {
	NORMAL_ATTACK, SKILL, ULTIMATE, DASH, ROLL, COUNT
};
enum class MonsterBehavior : char {
	CHASE, RETARGET, TAUNT, PREPARE_ATTACK, ATTACK, DEATH, COUNT
};
enum InteractableType : char {
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
		TAUNT, BLOCK, BLOCKIDLE
	};
};
// ----------------------------------

enum TriggerType : UCHAR {
	ARROW_RAIN
};

namespace TriggerSetting
{
	using namespace std::literals;

	constexpr std::chrono::milliseconds 
		COOLDOWN[static_cast<int>(PlayerType::COUNT)]{
			300ms
		};

	constexpr std::chrono::milliseconds
		DURATION[static_cast<int>(PlayerType::COUNT)]{
			2000ms
		};

	constexpr DirectX::XMFLOAT3
		EXTENT[static_cast<int>(PlayerType::COUNT)]{
			DirectX::XMFLOAT3{ 10.65f, 10.65f, 10.65f }
		};
}

namespace PlayerSetting
{
	using namespace std::literals;

	constexpr float WALK_SPEED = 4.f;
	constexpr float	RUN_SPEED = 10.f;
	constexpr float	DASH_SPEED = 12.f;
	constexpr float ROLL_SPEED = 10.f;
	constexpr float ARROW_SPEED = 20.f;
	constexpr float AUTO_TARET_RANGE = 12.f;

	constexpr auto DASH_DURATION = 300ms;
	constexpr float MAX_STAMINA = 120.f;
	constexpr float MINIMUM_DASH_STAMINA = 10.f;
	constexpr float MINIMUM_ROLL_STAMINA = 15.f;
	constexpr float DEFAULT_STAMINA_CHANGE_AMOUNT = 10.f;
	constexpr float ROLL_STAMINA_CHANGE_AMOUNT = 15.f;

	constexpr auto ROLL_COOLDOWN = 3s;
	constexpr auto DASH_COOLDOWN = 1200ms;

	constexpr float SKILL_RATIO[static_cast<int>(PlayerType::COUNT)]{ 1.3f, 1.9f };
	constexpr float ULTIMATE_RATIO[static_cast<int>(PlayerType::COUNT)]{ 2.f, 0.3f };

	constexpr std::chrono::milliseconds
		ATTACK_COLLISION_TIME[static_cast<int>(PlayerType::COUNT)]{ 210ms, 360ms };
	constexpr std::chrono::milliseconds
		SKILL_COLLISION_TIME[static_cast<int>(PlayerType::COUNT)]{ 340ms, 560ms };
	constexpr std::chrono::milliseconds
		ULTIMATE_COLLISION_TIME[static_cast<int>(PlayerType::COUNT)]{ 930ms, 560ms };
	constexpr std::chrono::milliseconds
		ATTACK_COOLDOWN[static_cast<int>(PlayerType::COUNT)]{ 1000ms, 1000ms };
	constexpr std::chrono::seconds
		SKILL_COOLDOWN[static_cast<int>(PlayerType::COUNT)]{ 7s, 2s };
	constexpr std::chrono::seconds
		ULTIMATE_COOLDOWN[static_cast<int>(PlayerType::COUNT)]{ 20s, 2s };
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
	constexpr float ARROW_SPEED = 15.f;
	constexpr float RECOGNIZE_RANGE = 20.f;

	constexpr auto DECREASE_AGRO_LEVEL_TIME = 10s;

	constexpr USHORT BEHAVIOR_ANIMATION[static_cast<int>(MonsterBehavior::COUNT)]{
		ObjectAnimation::RUN,
		MonsterAnimation::LOOK_AROUND,
		MonsterAnimation::TAUNT,
		MonsterAnimation::TAUNT,
		ObjectAnimation::ATTACK,
		ObjectAnimation::DEATH
	};
	constexpr MonsterBehavior NEXT_BEHAVIOR[static_cast<int>(MonsterBehavior::COUNT)][2]{
		{MonsterBehavior::RETARGET, MonsterBehavior::TAUNT}, 
		{MonsterBehavior::CHASE, MonsterBehavior::CHASE},
		{MonsterBehavior::CHASE, MonsterBehavior::CHASE},
		{MonsterBehavior::ATTACK, MonsterBehavior::ATTACK},
		{MonsterBehavior::PREPARE_ATTACK, MonsterBehavior::CHASE},
		{MonsterBehavior::DEATH, MonsterBehavior::DEATH},
	};

	constexpr std::chrono::milliseconds
		BEHAVIOR_TIME[static_cast<int>(MonsterBehavior::COUNT)]{
			7000ms, 3000ms, 2000ms, 1000ms, 625ms, 2000ms
		};
	constexpr float ATTACK_RANGE[static_cast<int>(MonsterType::COUNT)]{
		1.f, 7.f };
	constexpr float BORDER_RANGE[static_cast<int>(MonsterType::COUNT)]{
		2.f, 5.f };
	constexpr std::chrono::milliseconds
		ATK_COLLISION_TIME[static_cast<int>(MonsterType::COUNT)]{ 300ms, 0ms };
}

namespace RoomSetting
{
	using namespace std::literals;

	constexpr auto RESET_TIME = 5s;
	constexpr float DEFAULT_HEIGHT = 0.f;
	constexpr unsigned char BOSS_FLOOR = 5;
	constexpr int MAX_ARROWS = 30;
	constexpr auto ARROW_REMOVE_TIME = 3s;

	constexpr float DOWNSIDE_STAIRS_HEIGHT = 4.4f;
	constexpr float DOWNSIDE_STAIRS_FRONT = -7.f;
	constexpr float DOWNSIDE_STAIRS_BACK = -17.f;

	constexpr float TOPSIDE_STAIRS_HEIGHT = 4.67f;
	constexpr float TOPSIDE_STAIRS_FRONT = 53.f;
	constexpr float TOPSIDE_STAIRS_BACK = 43.f;

	constexpr float EVENT_RADIUS = 1.f;
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
	InteractableType interactable_type;
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
	InteractableType interactable_type;
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

struct SC_PLAYER_SHOOT_PACKET
{
	UCHAR size;
	UCHAR type;
	INT id;
	INT arrow_id;
	INT target_id;
};

struct SC_REMOVE_ARROW_PACKET
{
	UCHAR size;
	UCHAR type;
	INT arrow_id;
};

#pragma pack (pop)


