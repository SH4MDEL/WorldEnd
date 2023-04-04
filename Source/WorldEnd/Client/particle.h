#pragma once
#include "stdafx.h"
#include "particleMesh.h"

class Particle
{
public:
	Particle();
	virtual ~Particle() = default;

	virtual void Update(FLOAT timeElapsed) = 0;
	virtual void RenderStreamOutput(const ComPtr<ID3D12GraphicsCommandList>& commandList) = 0;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) = 0;

	virtual void CreateParticle(XMFLOAT3 position) = 0;

	BOOL IsRunning() { return m_running; }

protected:
	XMFLOAT4X4					m_worldMatrix;

	unique_ptr<ParticleMesh>	m_mesh;
	BOOL						m_running;
};

class EmitterParticle : public Particle
{
public:
	EmitterParticle(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	~EmitterParticle() = default;

	void Update(FLOAT timeElapsed) override;
	void RenderStreamOutput(const ComPtr<ID3D12GraphicsCommandList>& commandList) override;
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) override;

	void CreateParticle(XMFLOAT3 position) override;

private:
	const FLOAT					m_lifeTime = 0.5f;
	FLOAT						m_age;
};