#pragma once
#include "scene.h"

class TowerScene : public Scene
{
public:
	TowerScene();
	~TowerScene() override;

	void OnCreate() override;
	void OnDestroy() override;

	void BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature, FLOAT	aspectRatio) override;
	
	void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) const override;
	void OnProcessingKeyboardMessage(FLOAT timeElapsed) const override;
	
	void Update(FLOAT timeElapsed) override;
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const override;

	unordered_map<string, shared_ptr<Shader>>& GetShaders() { return m_shaders; }

	void CheckBorderLimit();

    // ���� ���� �Լ���
	void SendPlayerData();
	void RecvPacket();
	void ProcessPacket();
	void RecvLoginOkPacket();
	void RecvUpdateClient();

protected:
	// �ٸ� �÷��̾��� id Ȯ���ϱ� ���ؼ� �߰�
	INT									                    m_left_other_player_id;
	INT									                    m_right_other_player_id;

	unordered_map<string, shared_ptr<GameObject>>	        m_object;

	shared_ptr<Player>								        m_player;
	shared_ptr<Camera>								        m_camera;

	array<shared_ptr<Player>, Setting::MAX_PLAYERS>			m_multi_players;

};

