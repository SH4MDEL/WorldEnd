#pragma once
#include "stdafx.h"
#include "shader.h"
#include "instancingShader.h"
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
#include "fadeFilter.h"
#include "sobelFilter.h"

struct SceneInfo
{
	XMFLOAT4X4			lightView;	// 뷰변환 행렬
	XMFLOAT4X4			lightProj;	// 투영변환 행렬
	XMFLOAT4X4			NDCspace;	// 카메라 위치
};

enum class SCENETAG : INT {
	GlobalLoadingScene,
	VillageLoadingScene,
	LoginScene,
	TowerLoadingScene,
	TowerScene,
	Count
};

class Scene abstract
{
public:
	Scene() = default;
	virtual ~Scene() = default;

	virtual void OnCreate(const ComPtr<ID3D12Device>& device, 
		const ComPtr<ID3D12GraphicsCommandList>& commandList, 
		const ComPtr<ID3D12RootSignature>& rootSignature, 
		const ComPtr<ID3D12RootSignature>& postRootSignature) = 0;			// 해당 씬으로 변경될 때 호출

	virtual void OnDestroy() = 0;			// 해당 씬에서 탈출할 때 호출
	virtual void ReleaseUploadBuffer() = 0;

	virtual void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) = 0;
	virtual void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) = 0;

	virtual void BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, 
		const ComPtr<ID3D12RootSignature>& rootsignature, const ComPtr<ID3D12RootSignature>& postRootsignature) = 0;
	virtual void DestroyObjects() = 0;
	
	virtual void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) = 0;
	virtual void OnProcessingMouseMessage(UINT message, LPARAM lParam) = 0;
	virtual void OnProcessingKeyboardMessage(FLOAT timeElapsed) = 0;

	virtual void Update(FLOAT timeElapsed) = 0;
	virtual void PreProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) = 0;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) const = 0;
	virtual void PostProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget, UINT threadIndex) = 0;
	virtual void RenderText(const ComPtr< ID2D1DeviceContext2>& deviceContext) = 0;
	virtual void PostRenderText(const ComPtr< ID2D1DeviceContext2>& deviceContext) = 0;
	
	virtual shared_ptr<Shadow> GetShadow() = 0;

	static unordered_map<string, shared_ptr<Mesh>>			m_globalMeshs;
	static unordered_map<string, shared_ptr<Texture>>		m_globalTextures;
	static unordered_map<string, shared_ptr<Materials>>		m_globalMaterials;
	static unordered_map<string, shared_ptr<Shader>>		m_globalShaders;
	static unordered_map<string, shared_ptr<AnimationSet>>	m_globalAnimationSets;

	static unordered_map<string, shared_ptr<Mesh>>			m_meshs;
	static unordered_map<string, shared_ptr<Texture>>		m_textures;
	static unordered_map<string, shared_ptr<Materials>>		m_materials;
	static unordered_map<string, shared_ptr<AnimationSet>>	m_animationSets;
};

class LoadingScene abstract : public Scene
{
public:
	LoadingScene() = default;
	virtual ~LoadingScene() = default;

	virtual void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) override;
	virtual void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) override;
	virtual void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) override;
	virtual void OnProcessingMouseMessage(UINT message, LPARAM lParam) override;
	virtual void OnProcessingKeyboardMessage(FLOAT timeElapsed) override;

	virtual void Update(FLOAT timeElapsed) override;
	virtual void PreProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) override;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) const override;
	virtual void PostProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget, UINT threadIndex) override;
	virtual void RenderText(const ComPtr< ID2D1DeviceContext2>& deviceContext) override;
	virtual void PostRenderText(const ComPtr< ID2D1DeviceContext2>& deviceContext) override;

	virtual shared_ptr<Shadow> GetShadow() override;
};