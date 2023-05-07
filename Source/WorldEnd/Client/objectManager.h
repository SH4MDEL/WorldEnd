#pragma once
#include "player.h"
#include "monster.h"
#include "shader.h"
#include "instancingShader.h"

constexpr int MAX_ARROWRAIN_COUNT = 3;

class TowerObjectManager
{
public:
	TowerObjectManager(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, 
		const shared_ptr<Shader>& arrowShader, const shared_ptr<Shader>& magicCircleShader, const shared_ptr<InstancingShader>& arrowRainShader);

	void Update(FLOAT timeElapsed);
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList);

	void CreateArrow(const shared_ptr<GameObject>& parent, INT arrowId, FLOAT SPEED, ActionType type);
	void RemoveArrow(INT arrowId);

	void CreateArrowRain(const XMFLOAT3& position);

	void CreateMonsterMagicCircle(const XMFLOAT3& position);

private:
	shared_ptr<Shader>											m_arrowShader;
	array<unique_ptr<Arrow>, RoomSetting::MAX_ARROWS>			m_arrows;

	shared_ptr<Shader>											m_magicCircleShader;
	shared_ptr<InstancingShader>								m_arrowRainShader;
	array<shared_ptr<ArrowRain>, RoomSetting::MAX_ARROWRAINS>	m_arrowRains;

	array<shared_ptr<MonsterMagicCircle>, 10>					m_monsterMagicCircles;
};

