#include "particle.h"
#include "particle.h"
#include "particle.h"

Particle::Particle() : m_running{false}
{
	XMStoreFloat4x4(&m_worldMatrix, XMMatrixIdentity());
}

EmitterParticle::EmitterParticle(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
	: m_age { 0.f }
{
	m_mesh = make_unique<EmitterParticleMesh>(device, commandList);
}

void EmitterParticle::Update(FLOAT timeElapsed)
{
	m_age += timeElapsed;
	if (m_age > m_lifeTime) {
		m_age = 0.f;
		m_running = false;
	}
}

void EmitterParticle::RenderStreamOutput(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	XMFLOAT4X4 worldMatrix;
	XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_worldMatrix)));
	commandList->SetGraphicsRoot32BitConstants((INT)ShaderRegister::GameObject, 16, &worldMatrix, 0);
	commandList->SetGraphicsRoot32BitConstants((INT)ShaderRegister::GameObject, 1, &(m_age), 18);

	m_mesh->RenderStreamOutput(commandList);
}

void EmitterParticle::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	m_mesh->Render(commandList);
}

void EmitterParticle::CreateParticle(XMFLOAT3 position)
{
	XMStoreFloat4x4(&m_worldMatrix, XMMatrixIdentity());
	m_worldMatrix._41 = position.x;
	m_worldMatrix._42 = position.y;
	m_worldMatrix._43 = position.z;

	m_age = 0.f;
	m_running = true;
}

PumperParticle::PumperParticle(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	m_mesh = make_unique<PumperParticleMesh>(device, commandList);
}

void PumperParticle::Update(FLOAT timeElapsed)
{
	m_age += timeElapsed;
	if (m_age > m_lifeTime) {
		m_age = 0.f;
		m_running = false;
	}
}

void PumperParticle::RenderStreamOutput(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	XMFLOAT4X4 worldMatrix;
	XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_worldMatrix)));
	commandList->SetGraphicsRoot32BitConstants((INT)ShaderRegister::GameObject, 16, &worldMatrix, 0);
	commandList->SetGraphicsRoot32BitConstants((INT)ShaderRegister::GameObject, 1, &(m_age), 18);

	m_mesh->RenderStreamOutput(commandList);
}

void PumperParticle::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	m_mesh->Render(commandList);
}

void PumperParticle::CreateParticle(XMFLOAT3 position)
{
	XMStoreFloat4x4(&m_worldMatrix, XMMatrixIdentity());
	m_worldMatrix._41 = position.x;
	m_worldMatrix._42 = position.y;
	m_worldMatrix._43 = position.z;

	m_age = 0.f;
	m_running = true;
}
