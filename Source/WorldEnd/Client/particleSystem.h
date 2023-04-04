#pragma once
#include "singleton.h"
#include "particle.h"
#include "shader.h"

class ParticleSystem
{
public:
	ParticleSystem(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, 
		const shared_ptr<ParticleShader>& emitterParticleShader);

	enum class Type : int { EMITTER, COUNT };

	void Update(FLOAT timeElapsed);
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList);

	void CreateParticle(Type type, XMFLOAT3 position);

private:
	shared_ptr<ParticleShader>								m_emitterShader;
	array<unique_ptr<EmitterParticle>, MAX_PARTICLE_COUNT>	m_emitterParticles;
};

