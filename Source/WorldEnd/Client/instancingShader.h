#pragma once
#include "stdafx.h"
#include "shader.h"
#include "scene.h"
#include "material.h"

struct InstancingData
{
	XMFLOAT4X4 worldMatrix;
};

class InstancingShader : public Shader
{
public:
	InstancingShader() = default;
	InstancingShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, UINT count);
	~InstancingShader() = default;

	virtual void Update(FLOAT timeElapsed);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const override;

	virtual void Clear() override;

	virtual void CreateShaderVariable(const ComPtr<ID3D12Device>& device);
	virtual void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const override;
	virtual void ReleaseShaderVariable();

	virtual void SetMesh(const string& name);

protected:
	shared_ptr<Mesh>					m_mesh;

	ComPtr<ID3D12Resource>				m_instancingBuffer;
	InstancingData*						m_instancingBufferPointer;
	D3D12_VERTEX_BUFFER_VIEW			m_instancingBufferView;

	UINT								m_instancingCount;
};

class ArrowInstance : public InstancingShader
{
public:
	ArrowInstance(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, UINT count);
	~ArrowInstance() = default;

	virtual void Update(FLOAT timeElapsed);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const override;

	virtual void Clear() override;

	virtual void CreateShaderVariable(const ComPtr<ID3D12Device>& device);
	virtual void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const override;
	virtual void ReleaseShaderVariable();

};

