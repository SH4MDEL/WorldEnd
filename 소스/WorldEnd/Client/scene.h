#pragma once
#include "stdafx.h"
#include "shader.h"
#include "object.h"
#include "player.h"
#include "camera.h"
#include "mesh.h"
#include "texture.h"

class Scene abstract
{
public:
	Scene() = default;
	~Scene() = default;

	virtual void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) const = 0;
	virtual void OnProcessingKeyboardMessage(FLOAT timeElapsed) const = 0;

	virtual void BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature, FLOAT	aspectRatio) = 0;
	virtual void Update(FLOAT timeElapsed) = 0;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const = 0;

	unordered_map<string, shared_ptr<Shader>>& GetShaders() { return m_shader; }

protected:
	unordered_map<string, shared_ptr<Mesh>>			m_mesh;
	unordered_map<string, shared_ptr<Texture>>		m_texture;
	unordered_map<string, shared_ptr<Shader>>		m_shader;
};