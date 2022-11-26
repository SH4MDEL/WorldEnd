#pragma once
#include "stdafx.h"

constexpr char INPUT_KEY_W = 0b1000;
constexpr char INPUT_KEY_S = 0b0100;
constexpr char INPUT_KEY_D = 0b0010;
constexpr char INPUT_KEY_A = 0b0001;

struct cs_packet_inputKey {
	UCHAR id;
	UCHAR inputKey;
};

typedef struct sc_packet_PlayerInfo {
	XMVECTOR playerData;
	int id;
}PLAYERINFO;
