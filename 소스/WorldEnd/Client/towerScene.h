#pragma once
#include "scene.h"

class TowerScene : public Scene
{
public:
	TowerScene();
	~TowerScene() override;

	void OnCreate() override;
	void OnDestroy() override;

	void ReleaseUploadBuffer() override;

	void BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature, FLOAT	aspectRatio) override;
	
	void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) const override;
	void OnProcessingKeyboardMessage(FLOAT timeElapsed) const override;
	
	void Update(FLOAT timeElapsed) override;
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const override;

	void LoadObjectFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName);

	void CheckBorderLimit();

    // 서버 관련 함수들
	void SendPlayerData();
	void RecvPacket();
	void ProcessPacket();
	void RecvLoginOkPacket();
	void RecvUpdateClient();

protected:
	vector<shared_ptr<GameObject>>		m_object;

	shared_ptr<Player>					m_player;
	shared_ptr<Camera>					m_camera;
	shared_ptr<TowerScene>					m_tscene;

	// 다른 플레이어의 id 확인하기 위해서 추가
	INT									                    m_left_other_player_id;
	INT									                    m_right_other_player_id;

	array<shared_ptr<Player>, Setting::MAX_PLAYERS>			m_multiPlayers;
};

