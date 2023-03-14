﻿#pragma once
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
	virtual void Render(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	virtual void Render(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader) const;

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

class StaticObjectShader : public Shader
{
public:
	StaticObjectShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~StaticObjectShader() = default;
};

class AnimationShader : public Shader
{
public:
	AnimationShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~AnimationShader() = default;
};

class SkyboxShader : public Shader
{
public:
	SkyboxShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~SkyboxShader() = default;
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

class AnimationShadowShader : public Shader
{
public:
	AnimationShadowShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~AnimationShadowShader() = default;
};

class UIRenderShader : public Shader
{
public:
	UIRenderShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~UIRenderShader() = default;
};