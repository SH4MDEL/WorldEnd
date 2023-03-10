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
#include "text.h"

struct SceneInfo
{
	XMFLOAT4X4			lightView;	// 뷰변환 행렬
	XMFLOAT4X4			lightProj;	// 투영변환 행렬
	XMFLOAT4X4			NDCspace;	// 카메라 위치
};

enum class SCENETAG : INT;

class Scene abstract
{
public:
	Scene() = default;
	virtual ~Scene() = default;

	virtual void OnCreate(const ComPtr<ID3D12Device>& device, 
		const ComPtr<ID3D12GraphicsCommandList>& commandList, 
		const ComPtr<ID3D12RootSignature>& rootSignature) = 0;			// 해당 씬으로 변경될 때 호출
	virtual void OnDestroy() = 0;			// 해당 씬에서 탈출할 때 호출
	virtual void ReleaseUploadBuffer() = 0;

	virtual void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) = 0;
	virtual void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) = 0;

	virtual void BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature) = 0;
	virtual void BuildObjectsByThread(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature, UINT threadIndex) = 0;

	virtual void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) const = 0;
	virtual void OnProcessingKeyboardMessage(FLOAT timeElapsed) const = 0;

	virtual void Update(FLOAT timeElapsed) = 0;
	virtual void Render(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) const = 0;
	virtual void RenderByThread(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) const = 0;
	virtual void RenderShadow(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) = 0;
	virtual void RenderShadowByThread(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) = 0;
	virtual void RenderText(const ComPtr< ID2D1DeviceContext2>& deviceContext) = 0;

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