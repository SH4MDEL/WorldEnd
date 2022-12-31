#pragma once
#include "scene.h"

class TowerScene : public Scene
{
public:
	TowerScene() = default;
	~TowerScene();

	void BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature, FLOAT	aspectRatio) override;
	void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) const override;
	void OnProcessingKeyboardMessage(FLOAT timeElapsed) const override;
	void Update(FLOAT timeElapsed) override;
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const override;

	unordered_map<string, shared_ptr<Shader>>& GetShaders() { return m_shader; }

	void CheckBorderLimit();

protected:
	unordered_map<string, shared_ptr<GameObject>>	m_object;

	shared_ptr<Player>								m_player;
	shared_ptr<Camera>								m_camera;
};

