#pragma once
#include "stdafx.h"
#include "shader.h"
#include "object.h"
#include "player.h"
#include "monster.h"
#include "camera.h"
#include "mesh.h"
#include "texture.h"
#include "material.h"
#include "light.h"
#include "shadow.h"
#include "ui.h"
#include "text.h"
#include "particleSystem.h"
#include "blurFilter.h"
#include "sobelFilter.h"

struct SceneInfo
{
	XMFLOAT4X4			lightView;	// �亯ȯ ���
	XMFLOAT4X4			lightProj;	// ������ȯ ���
	XMFLOAT4X4			NDCspace;	// ī�޶� ��ġ
};

enum class SCENETAG : INT;

class Scene abstract
{
public:
	Scene() = default;
	virtual ~Scene() = default;

	virtual void OnCreate(const ComPtr<ID3D12Device>& device, 
		const ComPtr<ID3D12GraphicsCommandList>& commandList, 
		const ComPtr<ID3D12RootSignature>& rootSignature, 
		const ComPtr<ID3D12RootSignature>& postRootSignature) = 0;			// �ش� ������ ����� �� ȣ��
	virtual void OnDestroy() = 0;			// �ش� ������ Ż���� �� ȣ��
	virtual void ReleaseUploadBuffer() = 0;

	virtual void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) = 0;
	virtual void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) = 0;

	virtual void BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, 
		const ComPtr<ID3D12RootSignature>& rootsignature, const ComPtr<ID3D12RootSignature>& postRootsignature) = 0;
	
	virtual void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) = 0;
	virtual void OnProcessingMouseMessage(UINT message, LPARAM lParam) = 0;
	virtual void OnProcessingKeyboardMessage(FLOAT timeElapsed) = 0;

	virtual void Update(FLOAT timeElapsed) = 0;
	virtual void PreProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) = 0;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) const = 0;
	virtual void PostProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget, UINT threadIndex) = 0;
	virtual void RenderText(const ComPtr< ID2D1DeviceContext2>& deviceContext) = 0;
	
	virtual shared_ptr<Shadow> GetShadow() = 0;

	static unordered_map<string, shared_ptr<Mesh>>			m_meshs;
	static unordered_map<string, shared_ptr<Texture>>		m_textures;
	static unordered_map<string, shared_ptr<Materials>>		m_materials;
	static unordered_map<string, shared_ptr<Shader>>		m_shaders;
	static unordered_map<string, shared_ptr<AnimationSet>>	m_animationSets;


protected:
};

enum class SCENETAG {
	LoadingScene,
	TowerScene,
	Count
};