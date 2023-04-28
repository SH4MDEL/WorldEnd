#pragma once
#include "stdafx.h"
#include "shader.h"

struct InstancingData
{
	XMFLOAT4X4 worldMatrix;
};

class InstancingShader : public Shader
{
public:
	InstancingShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const Mesh& mesh, UINT count);
	~InstancingShader() = default;

	virtual void Update(FLOAT timeElapsed);
	virtual void Render(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) const;

	virtual void Clear() override;

	virtual void CreateShaderVariable(const ComPtr<ID3D12Device>& device);
	virtual void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const override;
	virtual void ReleaseShaderVariable();

protected:
	unique_ptr<Mesh>					m_mesh;

	ComPtr<ID3D12Resource>				m_instancingBuffer;
	InstancingData* m_instancingBufferPointer;
	D3D12_VERTEX_BUFFER_VIEW			m_instancingBufferView;

	UINT								m_instancingCount;
};
