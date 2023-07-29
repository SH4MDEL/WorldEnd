#pragma once

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

	constexpr XMFLOAT3 BilldingStair_A_SIZE{ 6.004f, 2.152f, 5.747f };
	constexpr XMFLOAT3 BilldingStair_B_SIZE{ 12.42f, 2.385f, 5.99f };
	constexpr XMFLOAT3 BilldingStair_C_SIZE{ 3.38f, 2.226f, 5.25f };
	constexpr FLOAT BilldingStair_C_OFFSET = 5.5f;
	constexpr XMFLOAT3 BilldingStair_D_SIZE{ 6.829f, 5.543f, 17.21f };
	constexpr XMFLOAT3 BilldingStair_F_SIZE{ 6.926f, 3.429f, 8.227f };
	constexpr XMFLOAT3 BilldingStair_G_SIZE{ 6.742f, 4.832f, 11.27f };
	constexpr XMFLOAT3 BilldingStair_H_SIZE{ 13.35f, 5.036f, 11.76f };

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
	constexpr float WEST_GATE_FRONT = -218.59f + GATE_OFFSET;

	constexpr float EAST_GATE_LEFT = 67.141f + GATE_LENGTH / 2.f;
	constexpr float EAST_GATE_RIGHT = 67.141f - GATE_LENGTH / 2.f;
	constexpr float EAST_GATE_FRONT = 69.09f - GATE_OFFSET;

	constexpr XMFLOAT3 GROUND_TILE{ -109.8f, 0.f, 116.28f };
	constexpr XMFLOAT3 GROUND_TILE_OFFSET{ 12.f, 0.f, 27.f };

	constexpr XMFLOAT3 SKILL_NPC{ -42.7098f, 5.7f, 75.8633f };
	constexpr float SKILL_NPC_OFFSET = 2.f;

	constexpr XMFLOAT3 ENHENCE_NPC{ -40.7098f, 5.7f, 77.8633f };
	constexpr float ENHENCE_NPC_OFFSET = 2.f;
}