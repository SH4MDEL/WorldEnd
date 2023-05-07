#pragma once
#include "scene.h"
#include "framework.h"
#include "objectManager.h"

class TowerScene : public Scene
{
public:
	enum class State {
		Unused				= 0x00,
		InitScene			= 0x01,
		EnterScene			= 0x02,
		WarpGate			= 0x04,
		OutputExitUI		= 0x08,
		OutputResult		= 0x10,
		Fading				= 0x20,
		BlurLevel1			= Unused,
		BlurLevel2			= Unused,
		BlurLevel3			= OutputExitUI,
		BlurLevel4			= Unused,
		BlurLevel5			= OutputResult,
		Bluring				= BlurLevel1 | BlurLevel2 | BlurLevel3 | BlurLevel4 | BlurLevel5,
		CantPlayerControl	= OutputExitUI | OutputResult | Fading
	};
	enum class LightTag : INT {
		Directional,
		Corridor1,
		Corridor2,
		Corridor3,
		Room1,
		Room2,
		Room3,
		Room4,
		Count
	};

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
	void DestroyObjects() override;
	
	void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) override;
	void OnProcessingMouseMessage(UINT message, LPARAM lParam) override;
	void OnProcessingKeyboardMessage(FLOAT timeElapsed) override;
	
	void Update(FLOAT timeElapsed) override;
	void PreProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) override;
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) const override;
	void PostProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget, UINT threadIndex) override;
	void RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext) override;
	void PostRenderText(const ComPtr< ID2D1DeviceContext2>& deviceContext) override;

	shared_ptr<Shadow> GetShadow() override { return m_shadow; }
	void LoadSceneFromFile(wstring fileName, wstring sceneName);
	void LoadObjectFromFile(wstring fileName, const shared_ptr<GameObject>& object);
	void LoadPlayerFromFile(const shared_ptr<Player>& player);
	void LoadMonsterFromFile(const shared_ptr<Monster>& monster);

	void SetHpBar(const shared_ptr<AnimationObject>& object);
	void RotateToTarget(const shared_ptr<GameObject>& object, INT targetId);
	void RotateToTarget(const shared_ptr<GameObject>& object);
	INT SetTarget(const shared_ptr<GameObject>& object);

	bool CheckState(State sceneState);
	void SetState(State sceneState);
	void ResetState(State sceneState);

    // 서버 추가 코드
	void InitServer();
	void SendPlayerData();
	void RecvPacket();
	void ProcessPacket(char* ptr);
	void PacketReassembly(char* net_buf, size_t io_byte);
	void RecvLoginOk(char* ptr);
	void RecvAddObject(char* ptr);
	void RecvRemovePlayer(char* ptr);
	void RecvRemoveMonster(char* ptr);
	void RecvUpdateClient(char* ptr);
	void RecvChangeAnimation(char* ptr);
	void RecvAddMonster(char* ptr);
	void RecvUpdateMonster(char* ptr);
	void RecvChangeMonsterBehavior(char* ptr);
	void RecvResetCooldown(char* ptr);
	void RecvClearFloor(char* ptr);
	void RecvFailFloor(char* ptr);
	void RecvMonsterHit(char* ptr);
	void RecvChangeStamina(char* ptr);
	void RecvMonsterAttackCollision(char* ptr);
	void RecvSetInteractable(char* ptr);
	void RecvWarpNextFloor(char* ptr);
	void RecvPlayerDeath(char* ptr);
	void RecvArrowShoot(char* ptr);
	void RecvRemoveArrow(char* ptr);
	void RecvInteractObject(char* ptr);
	void RecvChangeHp(char* ptr);
	void RecvAddTrigger(char* ptr);
	void RecvAddMagicCircle(char* ptr);

private:
	void BuildUI(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist);
	void BuildLight(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist);

	void UpdateLightSystem(FLOAT timeElapsed);

protected:
	ComPtr<ID3D12Resource>								m_sceneBuffer;
	SceneInfo*											m_sceneBufferPointer;

	INT													m_sceneState;

	XMMATRIX											m_lightView;
	XMMATRIX											m_lightProj;
	XMMATRIX											m_NDCspace;

	shared_ptr<Player>									m_player;
	shared_ptr<Camera>									m_camera;

	shared_ptr<WarpGate>								m_gate;
	shared_ptr<GameObject>								m_portal;

	unique_ptr<TowerObjectManager>						m_towerObjectManager;
	shared_ptr<LightSystem>								m_lightSystem;
	shared_ptr<Shadow>									m_shadow;

	unique_ptr<BlurFilter>								m_blurFilter;
	unique_ptr<FadeFilter>								m_fadeFilter;
	unique_ptr<SobelFilter>								m_sobelFilter;

	// UI 관련
	array<shared_ptr<HorzGaugeUI>, MAX_INGAME_USER - 1>	m_hpUI;
	unordered_map<INT, INT>								m_idSet;
	shared_ptr<UI>										m_interactUI;
	shared_ptr<TextUI>									m_interactTextUI;
	shared_ptr<UI>										m_exitUI;
	shared_ptr<UI>										m_resultUI;
	shared_ptr<TextUI>									m_resultTextUI;
	shared_ptr<TextUI>									m_resultRewardTextUI;

	// 서버 추가 코드
	unordered_map<INT, shared_ptr<Player>>	            m_multiPlayers;
	unordered_map<INT, shared_ptr<Monster>>             m_monsters;

	XMFLOAT4											m_directionalDiffuse;
	XMFLOAT3											m_directionalDirection;
	const FLOAT											m_lifeTime = (FLOAT)RoomSetting::BATTLE_DELAY_TIME.count();
	FLOAT												m_age;
};

