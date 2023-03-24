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

constexpr int MAX_MONSTER = 10;

constexpr char CS_PACKET_LOGIN = 1;
constexpr char CS_PACKET_PLAYER_MOVE = 2;
constexpr char CS_PACKET_SET_COOLTIME = 4;
constexpr char CS_PACKET_WEAPON_COLLISION = 5;
constexpr char CS_PACKET_CHANGE_ANIMATION = 6;

constexpr char SC_PACKET_LOGIN_OK = 1;
constexpr char SC_PACKET_ADD_OBJECT = 2;
constexpr char SC_PACKET_REMOVE_PLAYER = 3;
constexpr char SC_PACKET_REMOVE_MONSTER = 4;
constexpr char SC_PACKET_UPDATE_CLIENT = 5;
constexpr char SC_PACKET_ADD_MONSTER = 6;
constexpr char SC_PACKET_UPDATE_MONSTER = 7;
constexpr char SC_PACKET_CHANGE_MONSTER_BEHAVIOR = 8;
constexpr char SC_PACKET_CHANGE_ANIMATION = 9;
constexpr char SC_PACKET_RESET_COOLTIME = 10;
constexpr char SC_PACKET_CLEAR_FLOOR = 11;
constexpr char SC_PACKET_FAIL_FLOOR = 12;

enum class PlayerType : char { WARRIOR, ARCHER, UNKNOWN };
enum class AttackType : char { NORMAL, SKILL, ULTIMATE };
enum class MonsterType : char { WARRIOR, ARCHER, WIZARD };
enum class EnvironmentType : char { RAIN, FOG, GAS, TRAP };

enum class CollisionType : char { PERSISTENCE, ONE_OFF, MULTIPLE_TIMES };
enum CooltimeType {
	NORMAL_ATTACK, SKILL, ULTIMATE, ROLL, COUNT
};
enum class MonsterBehavior : char {
	CHASE, RETARGET, TAUNT, PREPARE_ATTACK, ATTACK, DEAD, NONE
};

namespace PlayerSetting
{
	using namespace std::literals;

	constexpr float PLAYER_RUN_SPEED = 10.f;

	constexpr auto WARRIOR_ATTACK_COOLTIME = 1s;
	constexpr auto WARRIOR_SKILL_COOLTIME = 7s;
	constexpr auto WARRIOR_ULTIMATE_COOLTIME = 20s;
}

namespace MonsterSetting 
{
	using namespace std::literals;

	constexpr float WALK_SPEED = 3.f;
	constexpr auto LOOK_AROUND_TIME = 3s;
	constexpr auto RETARGET_TIME = 10s;
	constexpr auto TAUNT_TIME = 2s;
	constexpr auto PREPARE_ATTACK_TIME = 1s;
	constexpr auto ATTACK_TIME = 625ms;
	constexpr auto DEAD_TIME = 2s;
	constexpr auto DECREASE_AGRO_LEVEL_TIME = 10s;

	constexpr float WARRIOR_MONSTER_ATTACK_RANGE = 1.f;
	constexpr float WARRIOR_MONSTER_BORDER_RANGE = 2.f;

}

// ----- 애니메이션 enum 클래스 -----
// 애니메이션이 100개 이하로 떨어질 것이라 생각하여 100을 단위로 잡음
class ObjectAnimation
{
public:
	enum USHORT {
		IDLE, WALK, ATTACK, DEAD,
		END
	};
};

class WarriorAnimation : public ObjectAnimation
{
public:
	static constexpr int ANIMATION_START = 100;
	enum USHORT {
		GUARD = ObjectAnimation::END + ANIMATION_START
	};
};

class ArcherAnimation : public ObjectAnimation
{
public:
	static constexpr int ANIMATION_START = 200;
	enum USHORT {
		AIM = ObjectAnimation::END + ANIMATION_START
	};
};

class MonsterAnimation : public ObjectAnimation
{
public:
	static constexpr int ANIMATION_START = 300;
	enum USHORT {
		LOOK_AROUND = ObjectAnimation::END + ANIMATION_START,
		TAUNT, BLOCK, BLOCKIDLE
	};
};
// ----------------------------------



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

struct CS_COOLTIME_PACKET
{
	UCHAR size;
	UCHAR type;
	CooltimeType cooltime_type;
};

struct CS_ARROW_PACKET       // 공격키를 눌렀을때 투사체를 생성해주는 패킷
{
	UCHAR size;
	UCHAR type;
	ARROW_DATA arrow_data;
};

struct CS_WEAPON_COLLISION_PACKET		// 전사 플레이어가 공격 시 무기의 충돌 프레임에 서버로 보낼 패밋
{
	UCHAR size;
	UCHAR type;
	std::chrono::system_clock::time_point end_time;	// 충돌 판정 종료 시간
	AttackType attack_type;
	CollisionType collision_type;
	FLOAT x, y, z;
};

struct CS_CHANGE_ANIMATION_PACKET
{
	UCHAR size;
	UCHAR type;
	INT	animation_type;
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
	PLAYER_DATA	data[MAX_INGAME_USER];
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
	INT animation_type;
};

struct SC_RESET_COOLTIME_PACKET
{
	UCHAR size;
	UCHAR type;
	CooltimeType cooltime_type;
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

#pragma pack (pop)


