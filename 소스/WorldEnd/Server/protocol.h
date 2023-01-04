#pragma once
#include "stdafx.h"

constexpr short SERVER_PORT = 9000;

constexpr int BUF_SIZE = 1600;
constexpr int NAME_SIZE = 20;
constexpr int MAX_USER = 3;

enum class ePlayerType : char {SWORD, BOW};
enum class eAttackType : char { NORMAL, SKILL };
enum class eSceneType : char { LOGIN, LOADING, VILLAGE, PARTY, DUNGEON  };

#pragma pack(push,1)
struct PlayerData
{
	CHAR				id;				// �÷��̾� ���� ��ȣ
	bool				isActive;		// ��ȿ ����
	DirectX::XMFLOAT3	pos;			// ��ġ
	DirectX::XMFLOAT3	velocity;		// �ӵ�
	FLOAT				yaw;			// ȸ����
};

struct ArrowData    
{
	DirectX::XMFLOAT3	pos;		// ��ġ
	DirectX::XMFLOAT3	dir;		// ����
	INT					damage;		// ������
	CHAR				playerid;	// �� ���
};

//////////////////////////////////////////////////////
// Ŭ�󿡼� ������

struct CS_LOGIN_PACKET 
{
	UCHAR size;
	UCHAR type;
	CHAR id;
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
	bool	readycheck;   
};

struct CS_ATTACK_PACKET 
{
	UCHAR size;
	UCHAR type;
	ePlayerType playertype; // ���� ĳ������ ���Ÿ� ĳ������ �������ִ� ����ü ����
	eAttackType attacktype; // �⺻ �������� ��ų �������� �������ִ� ����ü ����
};

struct CS_ARROW_PACKET      // ����Ű�� �������� ����ü�� �������ִ� ��Ŷ
{  
	UCHAR size;
	UCHAR type;
	ArrowData arrowdata;
};

///////////////////////////////////////////////////////////////////////
// �������� Ŭ���

struct SC_LOGIN_OK_PACKET    // �α��� ������ �˷��ִ� ��Ŷ
{  
	UCHAR size;
	UCHAR type;
	PlayerData playerdata;
	ePlayerType playertype;
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
	bool	readycheck;
};

struct SC_PLAYER_SELECT_PACKET // �÷��̾� ���� ���� ��Ŷ 
{
	UCHAR size;
	UCHAR type;
	CHAR  id;
	ePlayerType playertype;
};

struct SC_ARROW_DATA_PACKET    // ����ü ������ �����ִ� ��Ŷ
{
	UCHAR		size;
	UCHAR		type;
	ArrowData	arrowdata;
};

struct SC_SCENE_CHANGE_PACKET    // �� ���� ��Ŷ
{
	UCHAR		size;
	UCHAR		type;
	eSceneType  scenetype;     // �� ������ ��� �ִ� ����ü ����
};
#pragma pack (pop)


