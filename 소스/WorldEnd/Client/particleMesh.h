#pragma once
#include "mesh.h"

struct EmitterParticleVertex
{
	EmitterParticleVertex() : position{ XMFLOAT3{0.f, 0.f, 0.f} }, velocity{ XMFLOAT3{0.f, 0.f, 0.f} },
		age{ 0.f }, lifeTime{ 0.f } {}
	EmitterParticleVertex(const XMFLOAT3& p, const XMFLOAT3& v, const FLOAT& a, const FLOAT& l) :
		position{ p }, velocity{ v }, age{ a }, lifeTime{ l } { }
	~EmitterParticleVertex() = default;

	XMFLOAT3 position;
	XMFLOAT3 velocity;
	FLOAT age;
	FLOAT lifeTime;
};


class ParticleMesh : public Mesh
{
public:
	ParticleMesh() = default;
	ParticleMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	~ParticleMesh() = default;

	virtual void CreateStreamOutputBuffer(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) = 0;

	virtual void RenderStreamOutput(const ComPtr<ID3D12GraphicsCommandList>& commandList) = 0;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) = 0;

protected:
	ComPtr<ID3D12Resource>				m_streamOutputBuffer;
	D3D12_STREAM_OUTPUT_BUFFER_VIEW		m_streamOutputBufferView;

	ComPtr<ID3D12Resource>				m_drawBuffer;

	ComPtr<ID3D12Resource>				m_filledSizeBuffer;
	ComPtr<ID3D12Resource>				m_filledSizeUploadBuffer;
	ComPtr<ID3D12Resource>				m_filledSizeReadbackBuffer;

	UINT*								m_filledSizeUploadBufferSize;
	UINT*								m_filledSizeReadbackBufferSize;

	BOOL								m_isBinding;

};

class EmitterParticleMesh : public ParticleMesh
{
public:
	EmitterParticleMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	~EmitterParticleMesh() = default;

	void CreateStreamOutputBuffer(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) override;

	void RenderStreamOutput(const ComPtr<ID3D12GraphicsCommandList>& commandList) override;
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) override;
};