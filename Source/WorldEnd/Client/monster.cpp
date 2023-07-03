#include "monster.h"

Monster::Monster() : m_hp{ 200.f }, m_maxHp{ 200.f }, m_enable{ false }
{
	m_boundingBox.Center = XMFLOAT3(0.028f, 1.27f, 0.f);
	m_boundingBox.Extents = XMFLOAT3(0.8f, 1.3f, 0.6f);
	m_boundingBox.Orientation = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
}

void Monster::Update(FLOAT timeElapsed)
{
	
}

void Monster::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw)
{
	m_yaw += yaw;

	GameObject::Rotate(0.f, 0.f, yaw);
}

void Monster::SetPosition(const XMFLOAT3& position)
{
	GameObject::SetPosition(position);

	if (m_hpBar) {
		XMFLOAT3 hpBarPosition = position;
		hpBarPosition.y += 2.3f;
		m_hpBar->SetPosition(hpBarPosition);
	}
}

void Monster::SetMaxHp(FLOAT maxHp)
{
	m_maxHp = maxHp;

	if (m_hpBar) {
		m_hpBar->SetMaxGauge(maxHp);
	}
}

void Monster::SetHp(FLOAT hp)
{
	m_hp = hp;
	if (m_hp <= 0)
		m_hp = 0;

	if (m_hpBar) {
		m_hpBar->SetGauge(m_hp);
	}
}

void Monster::SetVelocity(XMFLOAT3& velocity)
{

}

void Monster::ChangeAnimation(USHORT animation, bool doSend)
{
	if (m_currentAnimation == animation)
		return;

	int start_num{};
	m_animationController->SetTrackSpeed(0, 1.f);
	switch (animation) {
	case ObjectAnimation::IDLE:
		ChangeAnimationSettings(AnimationBlending::BLENDING, ANIMATION_TYPE_LOOP,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;
	case ObjectAnimation::WALK:
		ChangeAnimationSettings(AnimationBlending::BLENDING, ANIMATION_TYPE_LOOP,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;
	case ObjectAnimation::RUN:
		ChangeAnimationSettings(AnimationBlending::BLENDING, ANIMATION_TYPE_LOOP,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		m_animationController->SetTrackSpeed(0, 1.3f);
		break;
	case ObjectAnimation::ATTACK:
		ChangeAnimationSettings(AnimationBlending::NORMAL, ANIMATION_TYPE_ONCE,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;
	case ObjectAnimation::DEATH:
		ChangeAnimationSettings(AnimationBlending::BLENDING, ANIMATION_TYPE_ONCE,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;
	case MonsterAnimation::LOOK_AROUND:
		start_num = MonsterAnimation::ANIMATION_START;
		ChangeAnimationSettings(AnimationBlending::BLENDING, ANIMATION_TYPE_LOOP,
			ANIMATION_TYPE_ONCE, m_currentAnimation);
		break;
	case MonsterAnimation::TAUNT:
		start_num = MonsterAnimation::ANIMATION_START;
		ChangeAnimationSettings(AnimationBlending::BLENDING, ANIMATION_TYPE_LOOP,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;
	case WarriorMonsterAnimation::BLOCK:
		start_num = WarriorMonsterAnimation::ANIMATION_START;
		ChangeAnimationSettings(AnimationBlending::BLENDING, ANIMATION_TYPE_ONCE,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;
	case WarriorMonsterAnimation::BLOCKIDLE:
		start_num = WarriorMonsterAnimation::ANIMATION_START;
		ChangeAnimationSettings(AnimationBlending::BLENDING, ANIMATION_TYPE_LOOP,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;

	case ArcherMonsterAnimation::DRAW:
		start_num = ArcherMonsterAnimation::ANIMATION_START;
		ChangeAnimationSettings(AnimationBlending::BLENDING, ANIMATION_TYPE_ONCE,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;
	case ArcherMonsterAnimation::AIM:
		start_num = ArcherMonsterAnimation::ANIMATION_START;
		ChangeAnimationSettings(AnimationBlending::NORMAL, ANIMATION_TYPE_LOOP,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;
	case ArcherMonsterAnimation::WALK_BACKWARD:
		start_num = ArcherMonsterAnimation::ANIMATION_START;
		ChangeAnimationSettings(AnimationBlending::BLENDING, ANIMATION_TYPE_LOOP,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;
	case ArcherMonsterAnimation::FLEE:
		start_num = ArcherMonsterAnimation::ANIMATION_START;
		ChangeAnimationSettings(AnimationBlending::BLENDING, ANIMATION_TYPE_LOOP,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;

	case WizardMonsterAnimation::PREPARE_CAST:
		start_num = WizardMonsterAnimation::ANIMATION_START;
		ChangeAnimationSettings(AnimationBlending::BLENDING, ANIMATION_TYPE_LOOP,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;
	case WizardMonsterAnimation::CAST:
		start_num = WizardMonsterAnimation::ANIMATION_START;
		ChangeAnimationSettings(AnimationBlending::BLENDING, ANIMATION_TYPE_ONCE,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;
	case WizardMonsterAnimation::LAUGHING:
		start_num = WizardMonsterAnimation::ANIMATION_START;
		ChangeAnimationSettings(AnimationBlending::BLENDING, ANIMATION_TYPE_ONCE,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;
	}
	m_prevAnimation = m_currentAnimation;
	m_currentAnimation = animation;
	m_animationController->SetTrackAnimation(0, animation - start_num);
}


MonsterMagicCircle::MonsterMagicCircle() : m_enable{ false }, 
m_lifeTime{ (FLOAT)(chrono::duration_cast<chrono::seconds>(TriggerSetting::GENTIME[(INT)TriggerType::UNDEAD_GRASP]).count()) }, m_age{0.f}
{}

void MonsterMagicCircle::Update(FLOAT timeElapsed)
{
	if (m_enable) {
		m_age += timeElapsed;
		if (m_age >= m_lifeTime) {
			m_age = 0.f;
			m_enable = false;
			return;
		}
		GameObject::Update(timeElapsed);
	}
}

void MonsterMagicCircle::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	if (m_enable) GameObject::Render(commandList);
}