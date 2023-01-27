#pragma once
#include "stdafx.h"

constexpr short SERVER_PORT = 9000;

constexpr int BUF_SIZE = 1600;
constexpr int NAME_SIZE = 20;
constexpr int MAX_USER = 4;

constexpr char CS_PACKET_LOGIN = 1;
constexpr char CS_PACKET_PLAYER_MOVE = 2;
constexpr char CS_PACKET_PLAYER_ATTACK = 3;

constexpr char SC_PACKET_LOGIN_OK = 1;
constexpr char SC_PACKET_ADD_PLAYER = 2;
constexpr char SC_PACKET_UPDATE_CLIENT = 3;
constexpr char SC_PACKET_PLAYER_ATTACK = 4;


constexpr char INPUT_KEY_E = 0b1000;

enum class ePlayerType : char {SWORD, BOW};
enum class eAttackType : char { NORMAL, SKILL };
enum class eSceneType : char { LOGIN, LOADING, VILLAGE, PARTY, DUNGEON  };

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

//////////////////////////////////////////////////////
// Ŭ�󿡼� ������

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
	CHAR type;
	DirectX::XMFLOAT3	pos;
	DirectX::XMFLOAT3	velocity;
	FLOAT				yaw;
};

struct CS_READY_PACKET      // ��Ƽ �غ� �ϷḦ �˷��ִ� ��Ŷ
{
	UCHAR	size;
	UCHAR	type;
	bool	ready_check;   
};

struct CS_ATTACK_PACKET 
{
	UCHAR size;
	UCHAR type;
	//ePlayerType player_type; // ���� ĳ������ ���Ÿ� ĳ������ �������ִ� ����ü ����
	//eAttackType attack_type; // �⺻ �������� ��ų �������� �������ִ� ����ü ����
	CHAR key;
};

struct CS_ARROW_PACKET      // ����Ű�� �������� ����ü�� �������ִ� ��Ŷ
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
// �������� Ŭ���

struct SC_LOGIN_OK_PACKET    // �α��� ������ �˷��ִ� ��Ŷ
{  
	UCHAR size;
	UCHAR type;
	CHAR  name[NAME_SIZE];
	PlayerData player_data;
	ePlayerType player_type;
};

struct SC_ADD_PLAYER_PACKET
{
	UCHAR size;
	UCHAR type;
	CHAR  name[NAME_SIZE];
	PlayerData player_data;
	ePlayerType player_type;
};

struct SC_LOGIN_FAILL_PACKET  // �α��� ���и� �˷��ִ� ��Ŷ
{
	UCHAR	size;
	UCHAR	type;
	CHAR	id;
};

struct SC_READY_CHECK_PACKET  // ��Ƽ �غ� �ϷḦ �˷��ִ� ��Ŷ
{
	UCHAR	size;
	UCHAR	type;
	CHAR	id;
	bool	ready_check;
};

struct SC_PLAYER_SELECT_PACKET // �÷��̾� ���� ���� ��Ŷ 
{
	UCHAR size;
	UCHAR type;
	CHAR  id;
	ePlayerType player_type;
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
	eSceneType  scene_type;     // �� ������ ��� �ִ� ����ü ����
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
};

#pragma pack (pop)


