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

	void LoadObjectFromFile(wstring fileName, shared_ptr<GameObject> object);

	void CheckBorderLimit();

    // 서버 추가 코드
	void InitServer();
	void SendPlayerData();
	void RecvPacket();
	void ProcessPacket(char* ptr);
	void PacketReassembly(char* net_buf, size_t io_byte);
	void RecvLoginOkPacket(char* ptr);
	void RecvAddPlayerPacket(char* ptr);
	void RecvUpdateClient(char* ptr);
	void RecvAttackPacket(char* ptr);

protected:
	vector<shared_ptr<GameObject>>			m_object;

	shared_ptr<Player>						m_player;
	shared_ptr<Camera>						m_camera;

	// 서버 추가 코드
	unordered_map<INT, shared_ptr<Player>>	m_multiPlayers;


};

