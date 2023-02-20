#pragma once
#include "stdafx.h"
#include "object.h"
#include "player.h"
#include "camera.h"

class Shader
{
public:
	Shader() {};
	Shader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	virtual ~Shader() = default;

	virtual void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;

	virtual void Update(FLOAT timeElapsed);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader) const;

	shared_ptr<Player> GetPlayer() const { return m_player; }
	shared_ptr<Camera> GetCamera() const { return m_camera; }
	const vector<shared_ptr<GameObject>>& GetObjects() const { return m_gameObjects; }
	ComPtr<ID3D12PipelineState> GetPipelineState() const { return m_pipelineState; }

	void SetPlayer(const shared_ptr<Player>& player);
	void SetCamera(const shared_ptr<Camera>& camera);
	void SetObject(const shared_ptr<GameObject>& object);
	void SetMultiPlayer(INT ID, const shared_ptr<Player>& player);

protected:
	ComPtr<ID3D12PipelineState>				m_pipelineState;
	vector<D3D12_INPUT_ELEMENT_DESC>		m_inputLayout;

	vector<shared_ptr<GameObject>>			m_gameObjects;

	shared_ptr<Player>						m_player;
	shared_ptr<Camera>						m_camera;

	unordered_map<INT, shared_ptr<Player>>	m_multiPlayers;
};

class DetailShader : public Shader
{
public:
	DetailShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~DetailShader() = default;

	virtual void CreatePipelineState(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);

	virtual void Update(FLOAT timeElapsed);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;

	void SetField(const shared_ptr<Field>& field) { m_field = field; }

private:
	shared_ptr<Field>		m_field;
};


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
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;

	virtual void CreateShaderVariable(const ComPtr<ID3D12Device>& device);
	virtual void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const override;
	virtual void ReleaseShaderVariable();

protected:
	unique_ptr<Mesh>					m_mesh;

	ComPtr<ID3D12Resource>				m_instancingBuffer;
	InstancingData*						m_instancingBufferPointer;
	D3D12_VERTEX_BUFFER_VIEW			m_instancingBufferView;

	UINT								m_instancingCount;
};

class TextureHierarchyShader : public Shader
{
public:
	TextureHierarchyShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~TextureHierarchyShader() = default;
};

class SkyboxShader : public Shader
{
public:
	SkyboxShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~SkyboxShader() = default;
};

class BlendingShader : public Shader
{
public:
	BlendingShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~BlendingShader() = default;
};

class HpBarShader : public Shader
{
public:
	HpBarShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~HpBarShader() = default;
};

class ShadowShader : public Shader
{
public:
	ShadowShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~ShadowShader() = default;
};

class UIRenderShader : public Shader
{
public:
	UIRenderShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~UIRenderShader() = default;
};

class SkinnedAnimationShader : public Shader
{
public:
	SkinnedAnimationShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~SkinnedAnimationShader() = default;
};