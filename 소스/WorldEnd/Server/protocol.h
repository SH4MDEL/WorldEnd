#pragma once
#include "stdafx.h"

constexpr int BUF_SIZE = 200;
constexpr int NAME_SIZE = 20;

constexpr int MAX_USER = 10;
constexpr int MAX_NPC = 10;

constexpr char CS_LOGIN = 0;
constexpr char CS_MOVE = 1;
constexpr char SC_LOGIN_INFO = 2;
constexpr char SC_ADD_PLAYER = 3;
constexpr char SC_REMOVE_PLAYER = 4;
constexpr char SC_MOVE_PLAYER = 5;



constexpr char INPUT_KEY_W = 0b1000;
constexpr char INPUT_KEY_S = 0b0100;
constexpr char INPUT_KEY_D = 0b0010;
constexpr char INPUT_KEY_A = 0b0001;

#pragma pack(push,1)
struct CS_LOGIN_PACKET {
	UCHAR size;
	CHAR type;
	CHAR name[NAME_SIZE];
};

struct CS_INPUT_KEY_PACKET {
	UCHAR size;
	CHAR type;
	CHAR direction;
};

struct SC_LOGIN_INFO_PACKET {
	UCHAR size;
	CHAR type;
	SHORT id;
    FLOAT x, y, z;

};

struct SC_ADD_PLAYER_PACKET {
	UCHAR size;
	CHAR type;
	SHORT id;
	CHAR name[NAME_SIZE];
	FLOAT x, y, z;

};

struct SC_MOVE_PLAYER_PACKET {
	UCHAR size;
	CHAR type;
	SHORT id;
	FLOAT x, y, z;

};


typedef struct sc_packet_PlayerInfo {
	XMVECTOR playerData;
	int id;
}PLAYERINFO;

#pragma pack (pop)
