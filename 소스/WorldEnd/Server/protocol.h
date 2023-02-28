#pragma once
#include "stdafx.h"

constexpr short SERVER_PORT = 9000;

constexpr int BUF_SIZE = 1600;
constexpr int NAME_SIZE = 20;
constexpr int MAX_USER = 3;
constexpr int MAX_MONSTER = 10;

constexpr char CS_PACKET_LOGIN = 1;
constexpr char CS_PACKET_PLAYER_MOVE = 2;
constexpr char CS_PACKET_PLAYER_ATTACK = 4;

constexpr char SC_PACKET_LOGIN_OK = 1;
constexpr char SC_PACKET_ADD_PLAYER = 2;
constexpr char SC_PACKET_UPDATE_CLIENT = 3;
constexpr char SC_PACKET_PLAYER_ATTACK = 4;
constexpr char SC_PACKET_UPDATE_MONSTER = 5;

constexpr char INPUT_KEY_E = 0b1000;

enum class PlayerType : char {SWORD, BOW};
enum class AttackType : char { NORMAL, SKILL };
enum class SceneType : char { LOGIN, LOADING, VILLAGE, PARTY, DUNGEON  };
enum class MonsterType : char {WARRIOR, ARCHER, WIZARD};

enum eEventType : char { EVENT_PLAYER_ATTACK };

#pragma pack(push,1)
struct PlayerData
{
	CHAR				id;				// �÷��̾� ���� ��ȣ
	bool				active_check;		// ��ȿ ����
	DirectX::XMFLOAT3	pos;			// ��ġ
	DirectX::XMFLOAT3	velocity;		// �ӵ�
	FLOAT				yaw;			// ȸ����
	INT                 hp;
};

struct ArrowData    
{
	DirectX::XMFLOAT3	pos;		// ��ġ
	DirectX::XMFLOAT3	dir;		// ����
	INT					damage;		// ������
	CHAR				player_id;	// �� ���
};

struct MonsterData
{
	CHAR				id;			// ���� ���� ��ȣ
	DirectX::XMFLOAT3	pos;		// ��ġ
	DirectX::XMFLOAT3	velocity;	// �ӵ�
	FLOAT				yaw;		// ȸ����
};
//////////////////////////////////////////////////////
// 클라에서 서버로

struct CS_LOGIN_PACKET 
{
	UCHAR size;
	UCHAR type;
	CHAR name[NAME_SIZE];
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
	DirectX::XMFLOAT3	pos;
	DirectX::XMFLOAT3	velocity;
	FLOAT				yaw;
};

struct CS_READY_PACKET      // 파티 준비 완료를 알려주는 패킷
{
	UCHAR	size;
	UCHAR	type;
	bool	ready_check;
};

struct CS_ATTACK_PACKET
{
	UCHAR size;
	UCHAR type;
	//PlayerType player_type; // 근접 캐릭인지 원거리 캐릭인지 구별해주는 열거체 변수
	//AttackType attack_type; // 기본 공격인이 스킬 공격인지 구별해주는 열거체 변수
	CHAR key;
};

struct CS_ARROW_PACKET       // 공격키를 눌렀을때 투사체를 생성해주는 패킷
{
	UCHAR size;
	UCHAR type;
	ArrowData arrow_data;
};

struct CS_INPUT_KEY_PACKET
{
	UCHAR size;
	UCHAR type;
	CHAR  dir;
};
///////////////////////////////////////////////////////////////////////
// 서버에서 클라로

struct SC_LOGIN_OK_PACKET    // 로그인 성공을 알려주는 패킷
{
	UCHAR size;
	UCHAR type;
	CHAR  name[NAME_SIZE];
	PlayerData player_data;
	PlayerType player_type;
};

struct SC_ADD_PLAYER_PACKET
{
	UCHAR size;
	UCHAR type;
	CHAR  name[NAME_SIZE];
	PlayerData player_data;
	PlayerType player_type;
};

struct SC_LOGIN_FAILL_PACKET  // 로그인 실패를 알려주는 패킷
{
	UCHAR	size;
	UCHAR	type;
	CHAR	id;
};

struct SC_READY_CHECK_PACKET  // 파티 준비 완료를 알려주는 패킷
{
	UCHAR	size;
	UCHAR	type;
	CHAR	id;
	bool	ready_check;
};

struct SC_PLAYER_SELECT_PACKET // 플레이어 종류 선택 패킷 
{
	UCHAR size;
	UCHAR type;
	CHAR  id;
	PlayerType player_type;
};

struct SC_ARROW_DATA_PACKET    // ����ü ������ �����ִ� ��Ŷ
{
	UCHAR		size;
	UCHAR		type;
	ArrowData	arrow_data;
};

struct SC_SCENE_CHANGE_PACKET    // �� ���� ��Ŷ
{
	UCHAR		size;
	UCHAR		type;
	SceneType  scene_type;     // �� ������ ��� �ִ� ����ü ����
};

struct SC_UPDATE_CLIENT_PACKET
{
	UCHAR		size;
	UCHAR		type;
	PlayerData	data[MAX_USER];
};

struct SC_ATTACK_PACKET
{
	UCHAR     size;
	UCHAR     type;
	CHAR      id;
	CHAR      key;
};

struct SC_MONSTER_UPDATE_PACKET
{
	UCHAR		size;
	UCHAR		type;
	MonsterData	data;
};

#pragma pack (pop)


