#pragma once
#include "scene.h"
#include "framework.h"

class VillageScene : public Scene
{
public:
	enum class State {
		Unused = 0x00,
		DungeonInteract = 0x01,
		SkillInteract = 0x02,
		InhenceInteract = 0x04,
		OutputRoomUI = 0x08,
		OutputPartyUI = 0x10,
		OutputSkillUI = 0x20,
		OutputInhenceUI = 0x40,
		SceneLeave = 0x80,
		BlurLevel1 = Unused,
		BlurLevel2 = Unused,
		BlurLevel3 = Unused,
		BlurLevel4 = Unused,
		BlurLevel5 = Unused,
		Bluring = BlurLevel1 | BlurLevel2 | BlurLevel3 | BlurLevel4 | BlurLevel5,
		OutputUI = OutputRoomUI | OutputPartyUI | OutputSkillUI | OutputInhenceUI,
		CantPlayerControl = OutputUI
	};
	enum class LightTag : INT {
		Directional,
		Count
	};

	VillageScene() = default;
	VillageScene(const ComPtr<ID3D12Device>& device,
		const ComPtr<ID3D12GraphicsCommandList>& commandList,
		const ComPtr<ID3D12RootSignature>& rootSignature,
		const ComPtr<ID3D12RootSignature>& postRootSignature);
	~VillageScene() override;

	void OnResize(const ComPtr<ID3D12Device>& device, UINT width, UINT height) override;

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
	void OnProcessingKeyboardMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;

	void Update(FLOAT timeElapsed) override;

	void PreProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) override;
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) const override;
	void PostProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget, UINT threadIndex) override;
	void RenderText(const ComPtr< ID2D1DeviceContext2>& deviceContext) override;
	void PostRenderText(const ComPtr< ID2D1DeviceContext2>& deviceContext) override;

	shared_ptr<Shadow> GetShadow() override { return m_shadow; }

	void LoadSceneFromFile(wstring fileName, wstring sceneName);
	void LoadObjectFromFile(wstring fileName, const shared_ptr<GameObject>& object);
	void LoadPlayerFromFile(const shared_ptr<Player>& player);
	void LoadNPCFromFile(const shared_ptr<AnimationObject>& npc);

	bool CheckState(State sceneState) const;
	void SetState(State sceneState);
	void ResetState(State sceneState);


	virtual void ProcessPacket(char* ptr);
	void RecvPartyInfo(char* ptr);
	void RecvJoinOk(char* ptr);
	void RecvJoinFail(char* ptr);
	void RecvCreateOk(char* ptr);
	void RecvCreateFail(char* ptr);

	void SendChangePage();
	void SendOpenPartyUI();
	void SendClosePartyUI();
	void SendCreateParty();
	void SendJoinParty();
	void SendEnhancement(EnhancementType type);

private:
	inline void BuildUI();
	inline void BuildInteractUI();
	inline void BulidRoomUI();
	inline void BuildPartyUI();
	inline void BuildSkillSettingUI();
	inline void BuildInhenceUI();
	inline void BuildMainUI();
	inline void BuildLight(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist);

	void UpdateInteract(FLOAT timeElapsed);

	void DrawBoundingBox(BoundingOrientedBox boundingBox, FLOAT roll, FLOAT pitch, FLOAT yaw);

	bool IsBlendObject(const string& objectName);
	bool IsCollideExceptObject(const string& objectName);

	void CollideWithMap();
	void CollideByStaticOBB(const shared_ptr<GameObject>& object, const shared_ptr<GameObject>& staticObject);

	bool MoveOnTerrain();
	bool MoveOnStairs();

	void ChangeCharacter(PlayerType type, shared_ptr<Player>& player);
	void SetSkillSettingUI(PlayerType type);
	void SetSkillUI();
	void UpdateGoldUI();

	void TryEnhancement(EnhancementType type);

protected:
	ComPtr<ID3D12Resource>			m_sceneBuffer;
	SceneInfo*						m_sceneBufferPointer;

	INT								m_sceneState;

	XMMATRIX						m_NDCspace;

	shared_ptr<Player>				m_player;
	shared_ptr<Camera>				m_camera;
	shared_ptr<HeightMapTerrain>	m_terrain;

	shared_ptr<LightSystem>			m_lightSystem;
	shared_ptr<Shadow>				m_shadow;

	unique_ptr<BlurFilter>			m_blurFilter;
	unique_ptr<FadeFilter>			m_fadeFilter;

	shared_ptr<UI>					m_interactUI;
	shared_ptr<TextUI>				m_interactTextUI;

	// Room UI 관련
	shared_ptr<UI>					m_roomUI;
	shared_ptr<ButtonUI>			m_leftArrowUI;
	shared_ptr<ButtonUI>			m_rightArrowUI;
	array<shared_ptr<SwitchUI>, 6>	m_roomSwitchUI;
	array<shared_ptr<TextUI>, 6>	m_roomSwitchTextUI;
	UINT							m_roomPage;

	// Party UI 관련
	shared_ptr<UI>					m_partyUI;
	array<shared_ptr<UI>, 3>		m_partyPlayerUI;
	array<shared_ptr<TextUI>, 3>	m_partyPlayerTextUI;

	// Skill Setting UI 관련
	shared_ptr<UI>					m_skillSettingUI;
	shared_ptr<SwitchUI>			m_skill1SwitchUI;
	shared_ptr<SwitchUI>			m_skill2SwitchUI;
	shared_ptr<SwitchUI>			m_ultimate1SwitchUI;
	shared_ptr<SwitchUI>			m_ultimate2SwitchUI;
	shared_ptr<TextUI>				m_skill1NameUI;
	shared_ptr<TextUI>				m_skill2NameUI;
	shared_ptr<TextUI>				m_ultimate1NameUI;
	shared_ptr<TextUI>				m_ultimate2NameUI;
	shared_ptr<TextUI>				m_skillNameUI;
	shared_ptr<TextUI>				m_skillInfoUI;
	shared_ptr<TextUI>				m_skillSettingGoldTextUI;

	// Inhence UI 관련
	shared_ptr<UI>					m_inhenceUI;
	shared_ptr<SwitchUI>			m_inhenceAttackSwitchUI;
	shared_ptr<SwitchUI>			m_inhenceCritDamageSwtichUI;
	shared_ptr<SwitchUI>			m_inhenceCritProbSwitchUI;
	shared_ptr<SwitchUI>			m_inhenceDefenceSwitchUI;
	shared_ptr<SwitchUI>			m_inhenceHpSwitchUI;
	shared_ptr<TextUI>				m_inhenceGoldTextUI;

	// Main UI
	shared_ptr<UI>					m_mainUI;
	shared_ptr<VertGaugeUI>			m_skillUI;
	shared_ptr<VertGaugeUI>			m_ultimateUI;
	shared_ptr<TextUI>				m_goldTextUI;

	XMFLOAT4						m_directionalDiffuse;
	XMFLOAT3						m_directionalDirection;

	// 플레이어의 충돌을 검사하기 위한 쿼드트리
	unique_ptr<QuadtreeFrustum>		m_quadtree;

	bool							m_onTerrain;
};

