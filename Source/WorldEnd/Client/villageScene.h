#pragma once
#include "scene.h"
#include "framework.h"

class VillageScene : public Scene
{
public:
	enum class State {
		Unused = 0x00,
		OutputOptionUI = 0x01,
		BlurLevel1 = Unused,
		BlurLevel2 = Unused,
		BlurLevel3 = Unused,
		BlurLevel4 = Unused,
		BlurLevel5 = Unused,
		Bluring = BlurLevel1 | BlurLevel2 | BlurLevel3 | BlurLevel4 | BlurLevel5
	};
	enum class LightTag : INT {
		Directional,
		Count
	};

	VillageScene();
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

	bool CheckState(State sceneState);
	void SetState(State sceneState);
	void ResetState(State sceneState);

private:
	void BuildUI(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist);
	void BuildLight(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist);

	void UpdateLightSystem(FLOAT timeElapsed);
	void UpdateBoundingFrustum();

	void DrawBoundingBox(BoundingOrientedBox boundingBox, FLOAT roll, FLOAT pitch, FLOAT yaw);

	bool IsBlendObject(const string& objectName);
	bool IsCollideExceptObject(const string& objectName);

	void CollideWithMap();
	void CollideByStaticOBB(const shared_ptr<GameObject>& object, const shared_ptr<GameObject>& staticObject);

	bool MoveOnTerrain();
	bool MoveOnStairs();

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

	XMFLOAT4						m_directionalDiffuse;
	XMFLOAT3						m_directionalDirection;

	BoundingFrustum				m_boundingFrustum;

	// 플레이어의 충돌을 검사하기 위한 쿼드트리
	unique_ptr<QuadtreeFrustum>	m_quadtree;

	bool							m_onTerrain;
};

