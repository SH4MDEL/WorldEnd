#include "particleSystem.h"

ParticleSystem::ParticleSystem(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, 
	const shared_ptr<ParticleShader>& emitterParticleShader,
	const shared_ptr<ParticleShader>& pumperParticleShader)
	: m_emitterShader{ emitterParticleShader }, m_pumperShader{ pumperParticleShader }
{
	for (auto& particle : m_emitterParticles) {
		particle = make_unique<EmitterParticle>(device, commandList);
	}
	for (auto& particle : m_pumperParticles) {
		particle = make_unique<PumperParticle>(device, commandList);
	}
}

void ParticleSystem::Update(FLOAT timeElapsed)
{
	for (const auto& particle : m_emitterParticles) {
		if (particle->IsRunning()) {
			particle->Update(timeElapsed);
		}
	}
	for (const auto& particle : m_pumperParticles) {
		if (particle->IsRunning()) {
			particle->Update(timeElapsed);
		}
	}
}

void ParticleSystem::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	for (const auto& particle : m_emitterParticles) {
		if (particle->IsRunning()) {
			commandList->SetPipelineState(m_emitterShader->GetStreamPipelineState().Get());
			particle->RenderStreamOutput(commandList);

			commandList->SetPipelineState(m_emitterShader->GetPipelineState().Get());
			particle->Render(commandList);
		}
	}
	for (const auto& particle : m_pumperParticles) {
		if (particle->IsRunning()) {
			commandList->SetPipelineState(m_pumperShader->GetStreamPipelineState().Get());
			particle->RenderStreamOutput(commandList);

			commandList->SetPipelineState(m_pumperShader->GetPipelineState().Get());
			particle->Render(commandList);
		}
	}
}

void ParticleSystem::CreateParticle(Type type, XMFLOAT3 position)
{
	switch (type)
	{
	case Type::EMITTER:
		for (const auto& particle : m_emitterParticles) {
			if (!particle->IsRunning()) {
				particle->CreateParticle(position);
				break;
			}
		}
		break;
	case Type::PUMPER:
		for (const auto& particle : m_pumperParticles) {
			if (!particle->IsRunning()) {
				particle->CreateParticle(position);
				break;
			}
		}
		break;
	}
}
