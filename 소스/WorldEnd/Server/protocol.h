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
	CHAR				id;				// 플레이어 고유 번호
	bool				isActive;		// 유효 여부
	DirectX::XMFLOAT3	pos;			// 위치
	DirectX::XMFLOAT3	velocity;		// 속도
	FLOAT				yaw;			// 회전각
};

struct ArrowData    
{
	DirectX::XMFLOAT3	pos;		// 위치
	DirectX::XMFLOAT3	dir;		// 방향
	INT					damage;		// 데미지
	CHAR				playerid;	// 쏜 사람
};

//////////////////////////////////////////////////////
// 클라에서 서버로

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

struct CS_READY_PACKET      // 파티 준비 완료를 알려주는 패킷
{
	UCHAR	size;
	UCHAR	type;
	bool	readycheck;   
};

struct CS_ATTACK_PACKET 
{
	UCHAR size;
	UCHAR type;
	ePlayerType playertype; // 근접 캐릭인지 원거리 캐릭인지 구별해주는 열거체 변수
	eAttackType attacktype; // 기본 공격인이 스킬 공격인지 구별해주는 열거체 변수
};

struct CS_ARROW_PACKET      // 공격키를 눌렀을때 투사체를 생성해주는 패킷
{  
	UCHAR size;
	UCHAR type;
	ArrowData arrowdata;
};

///////////////////////////////////////////////////////////////////////
// 서버에서 클라로

struct SC_LOGIN_OK_PACKET    // 로그인 성공을 알려주는 패킷
{  
	UCHAR size;
	UCHAR type;
	PlayerData playerdata;
	ePlayerType playertype;
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
	bool	readycheck;
};

struct SC_PLAYER_SELECT_PACKET // 플레이어 종류 선택 패킷 
{
	UCHAR size;
	UCHAR type;
	CHAR  id;
	ePlayerType playertype;
};

struct SC_ARROW_DATA_PACKET    // 투사체 정보를 보내주는 패킷
{
	UCHAR		size;
	UCHAR		type;
	ArrowData	arrowdata;
};

struct SC_SCENE_CHANGE_PACKET    // 씬 변경 패킷
{
	UCHAR		size;
	UCHAR		type;
	eSceneType  scenetype;     // 씬 정보를 담고 있는 열거체 변수
};
#pragma pack (pop)


