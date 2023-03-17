#pragma once
#include "singleton.h"
#include "particleMesh.h"

class ParticleSystem : public Singleton<ParticleSystem>
{
public:
	enum class Type : int { EMITTER, COUNT };

	void Update(FLOAT timeElapsed);
	void Render(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);

	void CreateParticle(Type type, XMFLOAT3 position);

private:
	array<EmitterParticleMesh, MAX_PARTICLE_COUNT>	m_emitterMeshs;
};

