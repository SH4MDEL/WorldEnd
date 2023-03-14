#pragma once
#include <DirectXMath.h>
#include <chrono>

constexpr short SERVER_PORT = 9000;

constexpr int BUF_SIZE = 5000;
constexpr int NAME_SIZE = 20;
constexpr int MAX_IN_GAME_USER = 3;
constexpr int MAX_GAME_ROOM_NUM = 3000;
constexpr int MAX_PARTY_NUM = 1000;
constexpr int MAX_RECORD_NUM = 5;

constexpr int MAX_MONSTER = 10;
constexpr int MAX_USER = 3;

constexpr char CS_PACKET_LOGIN = 1;
constexpr char CS_PACKET_PLAYER_MOVE = 2;
constexpr char CS_PACKET_PLAYER_ATTACK = 4;
constexpr char CS_PACKET_WEAPON_COLLISION = 5;
constexpr char CS_PACKET_CHANGE_ANIMATION = 6;

constexpr char SC_PACKET_LOGIN_OK = 1;
constexpr char SC_PACKET_ADD_CHARACTER = 2;
constexpr char SC_PACKET_REMOVE_CHARACTER = 3;
constexpr char SC_PACKET_UPDATE_CLIENT = 4;
constexpr char SC_PACKET_ADD_MONSTER = 6;
constexpr char SC_PACKET_UPDATE_MONSTER = 7;
constexpr char SC_PACKET_MONSTER_ATTACK = 8;
constexpr char SC_PACKET_CHANGE_ANIMATION = 9;

enum class PlayerType : char { WARRIOR, ARCHER, UNKNOWN };
enum class AttackType : char { NORMAL, SKILL, ULTIMATE };
enum class MonsterType : char { WARRIOR, ARCHER, WIZARD };
enum class EnvironmentType : char { RAIN, FOG, GAS, TRAP };

enum EventType : char { EVENT_PLAYER_ATTACK, EVENT_MONSTER_ATTACK};
enum CollisionType : char { PERSISTENCE, ONE_OFF, MULTIPLE_TIMES };

namespace MonsterSetting 
{
	constexpr float MONSTER_WALK_SPEED = 1.2f;
}

// ----- 애니메이션 enum 클래스 -----
// 애니메이션이 100개 이하로 떨어질 것이라 생각하여 100을 단위로 잡음
class ObjectAnimation
{
public:
	enum USHORT {
		IDLE, WALK, ATTACK,
		END
	};
};

class WarriorAnimation : public ObjectAnimation
{
public:
	enum USHORT {
		GUARD = ObjectAnimation::END + 100
	};
};

class ArcherAnimation : public ObjectAnimation
{
public:
	enum USHORT {
		AIM = ObjectAnimation::END + 200
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
	INT	                hp;
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
	INT					hp;
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

struct CS_ATTACK_PACKET
{
	UCHAR size;
	UCHAR type;
	AttackType attack_type; // 기본 공격인이 스킬 공격인지 구별해주는 열거체 변수
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

struct SC_ADD_PLAYER_PACKET
{
	UCHAR size;
	UCHAR type;
	CHAR  name[NAME_SIZE];
	PLAYER_DATA player_data;
	PlayerType player_type;
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
	PLAYER_DATA	data[MAX_USER];
};

struct SC_ADD_MONSTER_PACKET
{
	UCHAR size;
	UCHAR type;
	MONSTER_DATA monster_data;
	MonsterType monster_type;
};

struct SC_MONSTER_UPDATE_PACKET
{
	UCHAR size;
	UCHAR type;
	MONSTER_DATA monster_data;
};

struct SC_CHANGE_ANIMATION_PACKET
{
	UCHAR size;
	UCHAR type;
	INT id;
	INT animation_type;
};

#pragma pack (pop)


