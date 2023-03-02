﻿#pragma once
#include "scene.h"

class TowerScene : public Scene
{
public:
	TowerScene();
	~TowerScene() override;

	void OnCreate() override;
	void OnDestroy() override;

	void ReleaseUploadBuffer() override;

	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) override;
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) override;

	void BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature, FLOAT	aspectRatio) override;
	void CreateLight(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist);
	
	void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) const override;
	void OnProcessingKeyboardMessage(FLOAT timeElapsed) const override;
	
	void Update(FLOAT timeElapsed) override;
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const override;
	void RenderShadow(const ComPtr<ID3D12GraphicsCommandList>& commandList) override;

	void LoadSceneFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, wstring fileName, wstring sceneName);
	void LoadObjectFromFile(wstring fileName, const shared_ptr<GameObject>& object);

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
	void RecvAddMonsterPacket(char* ptr);
	void RecvUpdateMonster(char* ptr);

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

