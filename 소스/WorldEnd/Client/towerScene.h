#pragma once
#include "scene.h"
#include "framework.h"

class TowerScene : public Scene
{
public:
	TowerScene();
	~TowerScene() override;

	void OnCreate(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& rootSignature) override;
	void OnDestroy() override;

	void ReleaseUploadBuffer() override;

	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) override;
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) override;

	void BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature) override;
	void CreateLight(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist);
	
	void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) const override;
	void OnProcessingClickMessage(LPARAM lParam) const override;
	void OnProcessingKeyboardMessage(FLOAT timeElapsed) const override;
	
	void Update(FLOAT timeElapsed) override;
	void Render(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) const override;
	void RenderShadow(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) override;
	void RenderText(const ComPtr< ID2D1DeviceContext2>& deviceContext) override;

	void LoadSceneFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName, wstring sceneName);
	void LoadObjectFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName, const shared_ptr<GameObject>& object);
	void LoadPlayerFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Player>& player);
	void LoadMonsterFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Monster>& monster);

    // 서버 추가 코드
	void InitServer(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void SendPlayerData();
	void RecvPacket(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void ProcessPacket(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, char* ptr);
	void PacketReassembly(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, char* net_buf, size_t io_byte);
	void RecvLoginOkPacket(char* ptr);
	void RecvAddPlayerPacket(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, char* ptr);
	void RecvUpdateClient(char* ptr);
	void RecvAttackPacket(char* ptr);
	void RecvAddMonsterPacket(char* ptr);
	void RecvUpdateMonster(char* ptr);
	void RecvChangeAnimation(char* ptr);

protected:
	ComPtr<ID3D12Resource>					m_sceneBuffer;
	SceneInfo*								m_sceneBufferPointer;

	XMMATRIX								m_lightView;
	XMMATRIX								m_lightProj;
	XMMATRIX								m_NDCspace;

	vector<shared_ptr<GameObject>>			m_object;

	shared_ptr<Player>						m_player;
	shared_ptr<Camera>						m_camera;

	shared_ptr<LightSystem>					m_lightSystem;
	unique_ptr<Shadow>						m_shadow;

	// 서버 추가 코드
	unordered_map<INT, shared_ptr<Player>>	            m_multiPlayers;
	unordered_map<INT, shared_ptr<Monster>>             m_monsters;
};

