#pragma once
#include "scene.h"
#include "framework.h"

class LoginScene : public Scene
{
public:
	enum class State {
		Unused = 0x00,
		OutputOptionUI = 0x01,
		OutputNoticeUI = 0x02,
		SceneLeave = 0x04,
		BlurLevel1 = Unused,
		BlurLevel2 = Unused,
		BlurLevel3 = Unused,
		BlurLevel4 = Unused,
		BlurLevel5 = Unused,
		Bluring = BlurLevel1 | BlurLevel2 | BlurLevel3 | BlurLevel4 | BlurLevel5,
	};
	enum class LightTag : INT {
		Directional,
		Count
	};

	LoginScene() = default;
	LoginScene(const ComPtr<ID3D12Device>& device,
		const ComPtr<ID3D12GraphicsCommandList>& commandList,
		const ComPtr<ID3D12RootSignature>& rootSignature,
		const ComPtr<ID3D12RootSignature>& postRootSignature);
	~LoginScene() override;

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

	bool CheckState(State sceneState) const;
	void SetState(State sceneState);
	void ResetState(State sceneState);

private:
	void BuildUI(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist);
	void BuildOptionUI(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist);
	void BuildLight(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist);

	bool IsBlendObject(const string& objectName);


	// 서버 코드
	void InitServer();
	void ProcessPacket(char* ptr);
	bool TryLogin();
	bool TrySignin();
	bool CheckLength(const string_view& s);

	void RecvLoginOk(char* ptr);
	void RecvLoginFail(char* ptr);
	void RecvSigninOk(char* ptr);
	void RecvSigninFail(char* ptr);

	void ResetFontSize(int type);

private:
	ComPtr<ID3D12Resource>					m_sceneBuffer;
	SceneInfo* m_sceneBufferPointer;

	INT										m_sceneState;

	XMMATRIX								m_NDCspace;

	shared_ptr<Camera>						m_camera;
	shared_ptr<HeightMapTerrain>			m_terrain;

	shared_ptr<LightSystem>					m_lightSystem;
	shared_ptr<Shadow>						m_shadow;

	unique_ptr<BlurFilter>					m_blurFilter;
	unique_ptr<FadeFilter>					m_fadeFilter;

	XMFLOAT4								m_directionalDiffuse;
	XMFLOAT3								m_directionalDirection;

	shared_ptr<UI>							m_titleUI;
	shared_ptr<UI>							m_optionUI;
	shared_ptr<InputTextUI>					m_idBox;
	shared_ptr<InputTextUI>					m_passwordBox;
	shared_ptr<UI>							m_noticeUI;
	shared_ptr<TextUI>						m_noticeTextUI;
};

