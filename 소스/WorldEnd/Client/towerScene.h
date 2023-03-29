#pragma once
#include "scene.h"
#include "framework.h"

class TowerScene : public Scene
{
public:
	TowerScene();
	~TowerScene() override;

	void OnCreate(const ComPtr<ID3D12Device>& device,
		const ComPtr<ID3D12GraphicsCommandList>& commandList,
		const ComPtr<ID3D12RootSignature>& rootSignature, 
		const ComPtr<ID3D12RootSignature>& postRootSignature) override;
	void OnDestroy() override;

	void ReleaseUploadBuffer() override;

	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) override;
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) override;

	void BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, 
		const ComPtr<ID3D12RootSignature>& rootsignature, const ComPtr<ID3D12RootSignature>& postRootSignature) override;
	void CreateLight(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist);
	
	void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) const override;
	void OnProcessingClickMessage(LPARAM lParam) const override;
	void OnProcessingKeyboardMessage(FLOAT timeElapsed) const override;
	
	void Update(FLOAT timeElapsed) override;
	void RenderShadow(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) override;
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) const override;
	void PostProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget) override;
	void RenderText(const ComPtr< ID2D1DeviceContext2>& deviceContext) override;

	shared_ptr<Shadow> GetShadow() override { return m_shadow; }
	void LoadSceneFromFile(wstring fileName, wstring sceneName);
	void LoadObjectFromFile(wstring fileName, const shared_ptr<GameObject>& object);
	void LoadPlayerFromFile(const shared_ptr<Player>& player);
	void LoadMonsterFromFile(const shared_ptr<Monster>& monster);

    // 서버 추가 코드
	void InitServer();
	void SendPlayerData();
	void RecvPacket();
	void ProcessPacket(char* ptr);
	void PacketReassembly(char* net_buf, size_t io_byte);
	void RecvLoginOkPacket(char* ptr);
	void RecvAddObjectPacket(char* ptr);
	void RecvRemovePlayerPacket(char* ptr);
	void RecvRemoveMonsterPacket(char* ptr);
	void RecvUpdateClient(char* ptr);
	void RecvChangeAnimation(char* ptr);
	void RecvAddMonsterPacket(char* ptr);
	void RecvUpdateMonster(char* ptr);
	void RecvChangeMonsterBehavior(char* ptr);
	void RecvResetCooltime(char* ptr);
	void RecvClearFloor(char* ptr);
	void RecvFailFloor(char* ptr);
	void RecvCreateParticle(char* ptr);
	void RecvChangeStamina(char* ptr);
	void RecvMonsterAttackCollision(char* ptr);

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
	shared_ptr<Shadow>						m_shadow;
	unique_ptr<BlurFilter>					m_blurFilter;
	unique_ptr<SobelFilter>					m_sobelFilter;

	// 서버 추가 코드
	unordered_map<INT, shared_ptr<Player>>	            m_multiPlayers;
	unordered_map<INT, shared_ptr<Monster>>             m_monsters;
};

