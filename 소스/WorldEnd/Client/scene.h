#pragma once
#include "stdafx.h"
#include "shader.h"
#include "object.h"
#include "player.h"
#include "camera.h"
#include "mesh.h"
#include "texture.h"
#include "material.h"

enum class SCENETAG : INT;

class Scene abstract
{
public:
	Scene() = default;
	virtual ~Scene() = default;

	virtual void OnCreate() = 0;			// 해당 씬으로 변경될 때 호출
	virtual void OnDestroy() = 0;			// 해당 씬에서 탈출할 때 호출

	virtual void ReleaseUploadBuffer() = 0;

	virtual void BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature, FLOAT	aspectRatio) = 0;

	virtual void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) const = 0;
	virtual void OnProcessingKeyboardMessage(FLOAT timeElapsed) const = 0;

	virtual void Update(FLOAT timeElapsed) = 0;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const = 0;

	static unordered_map<string, shared_ptr<Mesh>>			m_meshs;
	static unordered_map<string, shared_ptr<Texture>>		m_textures;
	static unordered_map<string, shared_ptr<Materials>>		m_materials;
	static unordered_map<string, shared_ptr<Shader>>		m_shaders;

protected:
};

enum class SCENETAG {
	LoadingScene,
	TowerScene,
	Count
};