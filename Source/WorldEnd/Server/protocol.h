#pragma once
#include <DirectXMath.h>
#include <chrono>

constexpr short SERVER_PORT = 9000;

constexpr int BUF_SIZE = 5000;
constexpr int NAME_SIZE = 20;
constexpr int ID_SIZE = 30;
constexpr int PASSWORD_SIZE = 30;
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
constexpr int MAX_BOSS_MONSTER = MAX_GAME_ROOM_NUM;
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
constexpr char CS_PACKET_SET_COOLDOWN = 3;
constexpr char CS_PACKET_ATTACK = 4;
constexpr char CS_PACKET_CHANGE_ANIMATION = 5;
constexpr char CS_PACKET_CHANGE_STAMINA = 6;
constexpr char CS_PACKET_INTERACT_OBJECT = 7;

//  ------------ 파티 관련 패킷 (클라이언트 -> 서버)  ------------
constexpr char CS_PACKET_JOIN_PARTY = 8;
constexpr char CS_PACKET_CREATE_PARTY = 9;
constexpr char CS_PACKET_EXIT_PARTY = 10;
constexpr char CS_PACKET_CHANGE_CHARACTER = 11;
constexpr char CS_PACKET_READY = 12;
constexpr char CS_PACKET_ENTER_DUNGEON = 13;
// -----------------------------------------------------------

constexpr char SC_PACKET_LOGIN_FAIL = 0;
constexpr char SC_PACKET_LOGIN_OK = 1;
constexpr char SC_PACKET_ADD_PLAYER = 2;
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

constexpr char SC_PACKET_CHANGE_STAMINA = 13;
constexpr char SC_PACKET_CHANGE_HP = 14;
constexpr char SC_PACKET_SET_INTERACTABLE = 15;
constexpr char SC_PACKET_START_BATTLE = 16;
constexpr char SC_PACKET_WARP_NEXT_FLOOR = 17;
constexpr char SC_PACKET_PLAYER_DEATH = 18;
constexpr char SC_PACKET_ARROW_SHOOT = 19;
constexpr char SC_PACKET_REMOVE_ARROW = 20;
constexpr char SC_PACKET_MONSTER_SHOOT = 21;
constexpr char SC_PACKET_INTERACT_OBJECT = 22;
constexpr char SC_PACKET_ADD_TRIGGER = 23;
constexpr char SC_PACKET_ADD_MAGIC_CIRCLE = 24;

//  ------------ 파티 관련 패킷 (서버 -> 클라이언트)  ------------
constexpr char SC_PACKET_JOIN_FAIL = 25;
constexpr char SC_PACKET_JOIN_OK = 26;
constexpr char SC_PACKET_CREATE_OK = 27;
constexpr char SC_PACKET_CREATE_FAIL = 28;
constexpr char SC_PACKET_ADD_PARTY_MEMBER = 29;
constexpr char SC_PACKET_REMOVE_PARTY_MEMBER = 30;
constexpr char SC_PACKET_CHANGE_HOST = 31;
constexpr char SC_PACKET_CHANGE_CHARACTER = 32;
constexpr char SC_PACKET_PLAYER_READY = 33;
constexpr char SC_PACKET_ENTER_GAME_ROOM = 34;
constexpr char SC_PACKET_ENTER_FAIL = 35;
// -----------------------------------------------------------

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
	DASH, PREPARE_WIDE_SKILL, WIDE_SKILL, ENHANCE,                           // 보스 몬스터
	PREPARE_NORMAL_ATTACK, NORMAL_ATTACK,                              // 보스 몬스터
	PREPARE_ENHANCE_WIDE_SKILL, ENHANCE_WIDE_SKILL,                    // 보스 몬스터
	PREPARE_RUCH_SKILL, RUCH_SKILL,                                    // 보스 몬스터
	PREPARE_ULTIMATE_SKILL, ULTIMATE_SKILL,                            // 보스 몬스터
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

class BossMonsterAnimation : public ObjectAnimation
{
public:
	static constexpr int ANIMATION_START = 600;
	enum USHORT {
		PREPARE_WIDE_SKILL = ObjectAnimation::END + ANIMATION_START,
		NORMAL_ATTACK, RUCH_SKILL, WIDE_SKILL, ENHANCE_WIDE_SKILL, ULTIMATE_SKILL, ENHANCE
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
	constexpr float BOSD_RUN_SPEED = 4.5f;
	constexpr float BOSD_DASH_SPEED = 7.0f;

	constexpr auto DECREASE_AGRO_LEVEL_TIME = 10s;

	constexpr float ATTACK_RANGE[static_cast<int>(MonsterType::COUNT)]{
		1.5f, 12.f, 8.f };
	constexpr float BOUNDARY_RANGE[static_cast<int>(MonsterType::COUNT)]{
		2.f, 6.f, 2.f };
	constexpr std::chrono::milliseconds
		ATK_COLLISION_TIME[static_cast<int>(MonsterType::COUNT)]{ 300ms, 0ms, 1100ms };
	constexpr std::chrono::milliseconds CAST_COLLISION_TIME = 430ms;
}

namespace LoginSetting
{
	using namespace DirectX;

	constexpr XMFLOAT3 Direction{ -77.4f, 15.f, 108.6f };
	constexpr FLOAT DirectionStart = Direction.x - 25.f;
	constexpr FLOAT DirectionEnd = Direction.x + 25.f;
	constexpr FLOAT EyeOffset = 80.f;
}

namespace VillageSetting
{
	using namespace DirectX;

	constexpr FLOAT TERRAIN_HEIGHT_OFFSET = 2.34f;

	constexpr XMFLOAT3 BilldingStair_A_SIZE { 6.004f, 2.152f, 5.747f };
	constexpr XMFLOAT3 BilldingStair_B_SIZE { 12.42f, 2.385f, 5.99f };
	constexpr XMFLOAT3 BilldingStair_C_SIZE { 3.38f, 2.226f, 5.25f };
	constexpr FLOAT BilldingStair_C_OFFSET = 5.5f;
	constexpr XMFLOAT3 BilldingStair_D_SIZE { 6.829f, 5.543f, 17.21f };
	constexpr XMFLOAT3 BilldingStair_F_SIZE { 6.926f, 3.429f, 8.227f };
	constexpr XMFLOAT3 BilldingStair_G_SIZE { 6.742f, 4.832f, 11.27f };
	constexpr XMFLOAT3 BilldingStair_H_SIZE { 13.35f, 5.036f, 11.76f };

	// 270도 회전된 상태
	constexpr float STAIRS1_LEFT = 66.17f - BilldingStair_H_SIZE.x / 2.f;
	constexpr float STAIRS1_RIGHT = STAIRS1_LEFT + BilldingStair_H_SIZE.x;
	constexpr float STAIRS1_BACK = 36.68f - BilldingStair_H_SIZE.z / 2.f;
	constexpr float STAIRS1_FRONT = STAIRS1_BACK + BilldingStair_H_SIZE.z;
	constexpr float STAIRS1_TOP = 5.65f;
	constexpr float STAIRS1_BOTTOM = STAIRS1_TOP - BilldingStair_H_SIZE.y;

	constexpr float STAIRS2_LEFT = 58.92f - BilldingStair_B_SIZE.x / 2.f;
	constexpr float STAIRS2_RIGHT = STAIRS2_LEFT + BilldingStair_B_SIZE.x;
	constexpr float STAIRS2_BACK = -67.002f - BilldingStair_B_SIZE.z / 2.f;
	constexpr float STAIRS2_FRONT = STAIRS2_BACK + BilldingStair_B_SIZE.z;
	constexpr float STAIRS2_BOTTOM = 5.65f;
	constexpr float STAIRS2_TOP = STAIRS2_BOTTOM + BilldingStair_B_SIZE.y;

	// 180도
	constexpr float STAIRS3_RIGHT = 58.39f - BilldingStair_B_SIZE.x / 2.f;
	constexpr float STAIRS3_LEFT = STAIRS3_RIGHT + BilldingStair_B_SIZE.x;
	constexpr float STAIRS3_BACK = -88.182f - BilldingStair_B_SIZE.z / 2.f;
	constexpr float STAIRS3_FRONT = STAIRS3_BACK + BilldingStair_B_SIZE.z;
	constexpr float STAIRS3_BOTTOM = 5.6f;
	constexpr float STAIRS3_TOP = STAIRS3_BOTTOM + BilldingStair_B_SIZE.y;

	// 270도
	constexpr float STAIRS4_LEFT = 40.08f - BilldingStair_B_SIZE.x / 2.f;
	constexpr float STAIRS4_RIGHT = STAIRS4_LEFT + BilldingStair_B_SIZE.x;
	constexpr float STAIRS4_BACK = -124.35f - BilldingStair_B_SIZE.z / 2.f;
	constexpr float STAIRS4_FRONT = STAIRS4_BACK + BilldingStair_B_SIZE.z;
	constexpr float STAIRS4_BOTTOM = 5.65f;
	constexpr float STAIRS4_TOP = STAIRS4_BOTTOM + BilldingStair_B_SIZE.y;

	// 270도
	constexpr float STAIRS5_LEFT = 81.01f - BilldingStair_B_SIZE.x / 2.f;
	constexpr float STAIRS5_RIGHT = STAIRS5_LEFT + BilldingStair_B_SIZE.x;
	constexpr float STAIRS5_BACK = -124.285f - BilldingStair_B_SIZE.z / 2.f;
	constexpr float STAIRS5_FRONT = STAIRS5_BACK + BilldingStair_B_SIZE.z;
	constexpr float STAIRS5_BOTTOM = 5.65f;
	constexpr float STAIRS5_TOP = STAIRS5_BOTTOM + BilldingStair_B_SIZE.y;

	// 270도
	constexpr float STAIRS6_LEFT = 22.63f - BilldingStair_G_SIZE.x / 2.f;
	constexpr float STAIRS6_RIGHT = STAIRS6_LEFT + BilldingStair_G_SIZE.x;
	constexpr float STAIRS6_BACK = -104.441f - BilldingStair_G_SIZE.z / 2.f;
	constexpr float STAIRS6_FRONT = STAIRS6_BACK + BilldingStair_G_SIZE.z;
	constexpr float STAIRS6_TOP = 5.65f;
	constexpr float STAIRS6_BOTTOM = STAIRS6_TOP - BilldingStair_G_SIZE.y;

	// 180도 (다리 아래 좌측)
	constexpr float STAIRS7_LEFT = -77.436f + BilldingStair_B_SIZE.x / 2.f;
	constexpr float STAIRS7_RIGHT = STAIRS7_LEFT - BilldingStair_B_SIZE.x;
	constexpr float STAIRS7_BACK = 49.01f - BilldingStair_B_SIZE.z / 2.f;
	constexpr float STAIRS7_FRONT = STAIRS7_BACK + BilldingStair_B_SIZE.z;
	constexpr float STAIRS7_TOP = STAIRS6_BOTTOM;
	constexpr float STAIRS7_BOTTOM = STAIRS7_TOP - BilldingStair_B_SIZE.y + 0.05f;

	// 0도 (다리 아래 우측)
	constexpr float STAIRS8_LEFT = -77.436f + BilldingStair_B_SIZE.x / 2.f;
	constexpr float STAIRS8_RIGHT = STAIRS8_LEFT - BilldingStair_B_SIZE.x;
	constexpr float STAIRS8_BACK = 98.11f + BilldingStair_B_SIZE.z / 2.f;
	constexpr float STAIRS8_FRONT = STAIRS8_BACK - BilldingStair_B_SIZE.z;
	constexpr float STAIRS8_BOTTOM = STAIRS7_BOTTOM;
	constexpr float STAIRS8_TOP = STAIRS8_BOTTOM + BilldingStair_B_SIZE.y + 0.05f;

	// 긴 다리
	constexpr float STAIRS9_LEFT = -77.436f + BilldingStair_B_SIZE.x / 2.f;
	constexpr float STAIRS9_RIGHT = STAIRS9_LEFT - BilldingStair_B_SIZE.x;
	constexpr float STAIRS9_BACK = 125.56f + BilldingStair_B_SIZE.z / 2.f;
	constexpr float STAIRS9_FRONT = 108.59f - BilldingStair_B_SIZE.z / 2.f;
	constexpr float STAIRS9_BOTTOM = STAIRS8_TOP;
	constexpr float STAIRS9_TOP = STAIRS9_BOTTOM + BilldingStair_B_SIZE.y * 4.f + 0.63f;

	constexpr float STAIRS10_LEFT = -77.436f + BilldingStair_B_SIZE.x / 2.f;
	constexpr float STAIRS10_RIGHT = STAIRS10_LEFT - BilldingStair_B_SIZE.x;
	constexpr float STAIRS10_BACK = 158.32f + BilldingStair_B_SIZE.z / 2.f;
	constexpr float STAIRS10_FRONT = 152.62f - BilldingStair_B_SIZE.z / 2.f;
	constexpr float STAIRS10_BOTTOM = STAIRS9_TOP;
	constexpr float STAIRS10_TOP = STAIRS10_BOTTOM + BilldingStair_B_SIZE.y * 2.f + 0.15f;

	constexpr float STAIRS11_LEFT = -77.76f + BilldingStair_H_SIZE.x / 2.f;
	constexpr float STAIRS11_RIGHT = STAIRS11_LEFT - BilldingStair_H_SIZE.x;
	constexpr float STAIRS11_BACK = 243.09f - BilldingStair_H_SIZE.z / 2.f;
	constexpr float STAIRS11_FRONT = STAIRS11_BACK + BilldingStair_H_SIZE.z;
	constexpr float STAIRS11_TOP = STAIRS10_TOP;
	constexpr float STAIRS11_BOTTOM = STAIRS11_TOP - BilldingStair_H_SIZE.y;

	// 270도 회전
	constexpr float STAIRS12_LEFT = 154.89f - BilldingStair_F_SIZE.x / 2.f;
	constexpr float STAIRS12_RIGHT = STAIRS12_LEFT + BilldingStair_F_SIZE.x;
	constexpr float STAIRS12_BACK = -118.69f - BilldingStair_F_SIZE.z / 2.f;
	constexpr float STAIRS12_FRONT = STAIRS12_BACK + BilldingStair_F_SIZE.z;
	constexpr float STAIRS12_BOTTOM = STAIRS10_TOP;
	constexpr float STAIRS12_TOP = STAIRS12_BOTTOM + BilldingStair_F_SIZE.y;

	constexpr float STAIRS13_LEFT = -139.71f - BilldingStair_A_SIZE.x / 2.f;
	constexpr float STAIRS13_RIGHT = STAIRS13_LEFT + BilldingStair_A_SIZE.x;
	constexpr float STAIRS13_BACK = 89.905f + BilldingStair_A_SIZE.z / 2.f;
	constexpr float STAIRS13_FRONT = STAIRS13_BACK - BilldingStair_A_SIZE.z;
	constexpr float STAIRS13_BOTTOM = STAIRS5_TOP;
	constexpr float STAIRS13_TOP = STAIRS13_BOTTOM + BilldingStair_A_SIZE.y;

	// 후방
	constexpr float STAIRS14_LEFT = 60.f + BilldingStair_H_SIZE.x / 2.f;
	constexpr float STAIRS14_RIGHT = STAIRS14_LEFT - BilldingStair_H_SIZE.x;
	constexpr float STAIRS14_BACK = -177.92f + BilldingStair_H_SIZE.z / 2.f;
	constexpr float STAIRS14_FRONT = -189.22f - BilldingStair_H_SIZE.z / 2.f;
	constexpr float STAIRS14_TOP = STAIRS5_TOP;
	constexpr float STAIRS14_BOTTOM = -2.12f;

	// 좌측
	constexpr float STAIRS15_LEFT = -77.2f + BilldingStair_H_SIZE.x / 2.f;
	constexpr float STAIRS15_RIGHT = STAIRS15_LEFT - BilldingStair_H_SIZE.x;
	constexpr float STAIRS15_BACK = -18.079f + BilldingStair_H_SIZE.z / 2.f;
	constexpr float STAIRS15_FRONT = STAIRS15_BACK - BilldingStair_H_SIZE.z;
	constexpr float STAIRS15_TOP = STAIRS6_BOTTOM;
	constexpr float STAIRS15_BOTTOM = STAIRS15_TOP - BilldingStair_H_SIZE.y;

	constexpr float STAIRS16_LEFT = -35.998f - BilldingStair_F_SIZE.x / 2.f;
	constexpr float STAIRS16_RIGHT = STAIRS16_LEFT + BilldingStair_F_SIZE.x;
	constexpr float STAIRS16_BACK = 118.67f + BilldingStair_F_SIZE.z / 2.f;
	constexpr float STAIRS16_FRONT = STAIRS16_BACK - BilldingStair_F_SIZE.z;
	constexpr float STAIRS16_BOTTOM = STAIRS1_TOP;
	constexpr float STAIRS16_TOP = STAIRS16_BOTTOM + BilldingStair_F_SIZE.y - 0.3f;

	constexpr float STAIRS17_LEFT = -81.9f - BilldingStair_C_SIZE.x / 2.f;
	constexpr float STAIRS17_RIGHT = STAIRS17_LEFT + BilldingStair_C_SIZE.x;
	constexpr float STAIRS17_BACK = 82.17f - BilldingStair_C_OFFSET + BilldingStair_C_SIZE.z / 2.f;
	constexpr float STAIRS17_FRONT = STAIRS17_BACK - BilldingStair_C_SIZE.z;
	constexpr float STAIRS17_BOTTOM = STAIRS7_BOTTOM;
	constexpr float STAIRS17_TOP = STAIRS17_BOTTOM + BilldingStair_C_SIZE.y;

	constexpr float STAIRS18_LEFT = -81.9f + BilldingStair_C_SIZE.x / 2.f;
	constexpr float STAIRS18_RIGHT = STAIRS18_LEFT - BilldingStair_C_SIZE.x;
	constexpr float STAIRS18_BACK = 82.17f + BilldingStair_C_OFFSET - BilldingStair_C_SIZE.z / 2.f;
	constexpr float STAIRS18_FRONT = STAIRS18_BACK + BilldingStair_C_SIZE.z;
	constexpr float STAIRS18_BOTTOM = STAIRS7_BOTTOM;
	constexpr float STAIRS18_TOP = STAIRS18_BOTTOM + BilldingStair_C_SIZE.y;

	constexpr float STAIRS19_LEFT = -72.72f - BilldingStair_C_SIZE.x / 2.f;
	constexpr float STAIRS19_RIGHT = STAIRS19_LEFT + BilldingStair_C_SIZE.x;
	constexpr float STAIRS19_BACK = 75.24f - BilldingStair_C_OFFSET + BilldingStair_C_SIZE.z / 2.f;
	constexpr float STAIRS19_FRONT = STAIRS19_BACK - BilldingStair_C_SIZE.z;
	constexpr float STAIRS19_BOTTOM = STAIRS7_BOTTOM;
	constexpr float STAIRS19_TOP = STAIRS19_BOTTOM + BilldingStair_C_SIZE.y;

	constexpr float STAIRS20_LEFT = -72.72f + BilldingStair_C_SIZE.x / 2.f;
	constexpr float STAIRS20_RIGHT = STAIRS20_LEFT - BilldingStair_C_SIZE.x;
	constexpr float STAIRS20_BACK = 75.24f + BilldingStair_C_OFFSET - BilldingStair_C_SIZE.z / 2.f;
	constexpr float STAIRS20_FRONT = STAIRS20_BACK + BilldingStair_C_SIZE.z;
	constexpr float STAIRS20_BOTTOM = STAIRS7_BOTTOM;
	constexpr float STAIRS20_TOP = STAIRS20_BOTTOM + BilldingStair_C_SIZE.y;

	constexpr float STAIRS21_LEFT = -84.5f;
	constexpr float STAIRS21_RIGHT = -87.11f;
	constexpr float STAIRS21_BACK = 73.3f;
	constexpr float STAIRS21_FRONT = 79.f;
	constexpr float STAIRS21_BOTTOM = STAIRS17_TOP;
	constexpr float STAIRS21_TOP = 3.4f;

	constexpr float STAIRS22_LEFT = -90.79f;
	constexpr float STAIRS22_RIGHT = -87.85f;
	constexpr float STAIRS22_BACK = 82.93f;
	constexpr float STAIRS22_FRONT = 77.18f;
	constexpr float STAIRS22_BOTTOM = STAIRS21_TOP;
	constexpr float STAIRS22_TOP = 5.65f;

	constexpr float STAIRS23_LEFT = -70.55f;
	constexpr float STAIRS23_RIGHT = -67.53f;
	constexpr float STAIRS23_BACK = 84.6f;
	constexpr float STAIRS23_FRONT = 79.f;
	constexpr float STAIRS23_BOTTOM = STAIRS19_TOP;
	constexpr float STAIRS23_TOP = 3.08f;

	constexpr float STAIRS24_LEFT = -64.12f;
	constexpr float STAIRS24_RIGHT = -67.1f;
	constexpr float STAIRS24_BACK = 74.6f;
	constexpr float STAIRS24_FRONT = 80.7f;
	constexpr float STAIRS24_BOTTOM = STAIRS23_TOP;
	constexpr float STAIRS24_TOP = 5.65f;

	constexpr float STAIRS25_LEFT = 95.18f;
	constexpr float STAIRS25_RIGHT = 97.89f;
	constexpr float STAIRS25_BACK = -111.f;
	constexpr float STAIRS25_FRONT = -105.33f;
	constexpr float STAIRS25_BOTTOM = 5.65f;
	constexpr float STAIRS25_TOP = 8.f;

	constexpr float STAIRS26_LEFT = 101.5f;
	constexpr float STAIRS26_RIGHT = 98.47f;
	constexpr float STAIRS26_BACK = -100.65f;
	constexpr float STAIRS26_FRONT = -107.12f;
	constexpr float STAIRS26_BOTTOM = STAIRS25_TOP;
	constexpr float STAIRS26_TOP = STAIRS13_TOP;

	constexpr float STAIRS27_LEFT = 143.71f;
	constexpr float STAIRS27_RIGHT = 146.5f;
	constexpr float STAIRS27_BACK = -105.8f;
	constexpr float STAIRS27_FRONT = -100.69f;
	constexpr float STAIRS27_BOTTOM = STAIRS9_TOP;
	constexpr float STAIRS27_TOP = 13.22f;

	constexpr float STAIRS28_LEFT = 149.8f;
	constexpr float STAIRS28_RIGHT = 147.f;
	constexpr float STAIRS28_BACK = -96.7f;
	constexpr float STAIRS28_FRONT = -102.f;
	constexpr float STAIRS28_BOTTOM = STAIRS27_TOP;
	constexpr float STAIRS28_TOP = STAIRS10_TOP;


	constexpr float GATE_LENGTH = 12.33f;
	constexpr float GATE_OFFSET = 2.f;

	constexpr float NORTH_GATE_LEFT = -76.83f - GATE_LENGTH / 2.f;
	constexpr float NORTH_GATE_RIGHT = -76.83f + GATE_LENGTH / 2.f;
	constexpr float NORTH_GATE_FRONT = 271.81f - GATE_OFFSET;

	constexpr float SOUTH_GATE_LEFT = -74.84f + GATE_LENGTH / 2.f;
	constexpr float SOUTH_GATE_RIGHT = -74.84f - GATE_LENGTH / 2.f;
	constexpr float SOUTH_GATE_FRONT = -60.56f + GATE_OFFSET;

	constexpr float WEST_GATE_LEFT = 60.99f - GATE_LENGTH / 2.f;
	constexpr float WEST_GATE_RIGHT = 60.99f + GATE_LENGTH / 2.f;
	constexpr float WEST_GATE_FRONT = -218.59 + GATE_OFFSET;

	constexpr float EAST_GATE_LEFT = 67.141f + GATE_LENGTH / 2.f;
	constexpr float EAST_GATE_RIGHT = 67.141f - GATE_LENGTH / 2.f;
	constexpr float EAST_GATE_FRONT = 69.09f - GATE_OFFSET;
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

struct MONSTER_DATA
{
	INT					id;			// 몬스터 고유 번호
	DirectX::XMFLOAT3	pos;		// 위치
	FLOAT				yaw;		// 회전각
};
//////////////////////////////////////////////////////
// 클라에서 서버로

struct CS_LOGIN_PACKET 
{
	UCHAR size;
	UCHAR type;
	std::wstring id;
	std::wstring password;
};

struct CS_PLAYER_MOVE_PACKET
{
	UCHAR size;
	UCHAR type;
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 velocity;
	FLOAT yaw;
};

struct CS_COOLDOWN_PACKET
{
	UCHAR size;
	UCHAR type;
	ActionType cooldown_type;
};

struct CS_ATTACK_PACKET
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

struct CS_JOIN_PARTY_PACKET
{
	UCHAR size;
	UCHAR type;
	USHORT party_num;
};

struct CS_CREATE_PARTY_PACKET
{
	UCHAR size;
	UCHAR type;
};

struct CS_EXIT_PARTY_PACKET
{
	UCHAR size;
	UCHAR type;
};

struct CS_CHANGE_CHARACTER_PACKET
{
	UCHAR size;
	UCHAR type;
	PlayerType player_type;
};

struct CS_READY_PACKET
{
	UCHAR size;
	UCHAR type;
	bool is_ready;
};

struct CS_ENTER_DUNGEON_PACKET
{
	UCHAR size;
	UCHAR type;
};

///////////////////////////////////////////////////////////////////////
// 서버에서 클라로

struct SC_LOGIN_OK_PACKET    // 로그인 성공을 알려주는 패킷
{
	UCHAR size;
	UCHAR type;
	std::wstring name;
	INT id;
	DirectX::XMFLOAT3 pos;
	FLOAT hp;
	PlayerType player_type;
};

struct SC_ADD_PLAYER_PACKET
{
	UCHAR size;
	UCHAR type;
	CHAR  name[NAME_SIZE];
	INT id;
	DirectX::XMFLOAT3 pos;
	FLOAT hp;
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

struct SC_UPDATE_CLIENT_PACKET
{
	UCHAR size;
	UCHAR type;
	INT id;
	DirectX::XMFLOAT3 pos;
	FLOAT yaw;
#ifdef USER_NUM_TEST
	UINT move_time;
#endif
};

struct SC_ADD_MONSTER_PACKET
{
	UCHAR size;
	UCHAR type;
	MONSTER_DATA monster_data;
	FLOAT hp;
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

struct SC_CHANGE_STAMINA_PACKET
{
	UCHAR size;
	UCHAR type;
	FLOAT stamina;
};

struct SC_CHANGE_HP_PACKET
{
	UCHAR size;
	UCHAR type;
	INT id;
	FLOAT hp;
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

struct SC_JOIN_FAIL_PACKET
{
	UCHAR size;
	UCHAR type;
};

struct SC_JOIN_OK_PACKET
{
	UCHAR size;
	UCHAR type;
};

struct SC_CREATE_OK_PACKET
{
	UCHAR size;
	UCHAR type;
};

struct SC_CREATE_FAIL_PACKET
{
	UCHAR size;
	UCHAR type;
};

struct SC_ADD_PARTY_MEMBER_PACKET
{
	UCHAR size;
	UCHAR type;
	PlayerType player_type;
	INT id;
	std::string name;
};

struct SC_REMOVE_PARTY_MEMBER_PACKET
{
	UCHAR size;
	UCHAR type;
	INT id;
};

struct SC_CHANGE_HOST_PACKET
{
	UCHAR size;
	UCHAR type;
};

struct SC_CHANGE_CHARACTER_PACKET
{
	UCHAR size;
	UCHAR type;
	INT id;
	PlayerType player_type;
};

struct SC_PLAYER_READY_PACKET
{
	UCHAR size;
	UCHAR type;
	INT id;
	bool is_ready;
};

struct SC_ENTER_GAME_ROOM_PACKET
{
	UCHAR size;
	UCHAR type;
};

struct SC_ENTER_FAIL_PACKET
{
	UCHAR size;
	UCHAR type;
};

#pragma pack (pop)


