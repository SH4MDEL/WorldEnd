#pragma once
#include "stdafx.h"
#include "shader.h"
#include "object.h"
#include "player.h"
#include "camera.h"
#include "mesh.h"
#include "texture.h"
#include "Connect.h"

class Scene
{
public:
	Scene() = default;
	~Scene();

	static Scene* Get_Instatnce() {
		if (nullptr == m_instansce)
			m_instansce = new Scene;

		return m_instansce;
	}

	void DoSend();

	void BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature, FLOAT	aspectRatio);
	void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) const;
	void OnProcessingKeyboardMessage(FLOAT timeElapsed) const;
	void Update(FLOAT timeElapsed);
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;

	unique_ptr<Shader>& GetShader(const string& key) { return m_shader[key]; }
	unordered_map<string, unique_ptr<Shader>>& GetShaders() { return m_shader; }
	shared_ptr<Player> GetPlayer() const { return m_player; }
	shared_ptr<Camera> GetCamera() const { return m_camera; }

	void CheckBorderLimit();

	

	PLAYERINFO* GetPlayerInfo() { return m_playerInfo; }

	int GetClientId() const { return m_clientId; }
	SOCKET GetSocket() const { return m_socket; }

	void SetClientId(int id) { m_clientId = id; }
	void SetSocket(SOCKET sock) { m_socket = sock; }

private:
	unordered_map<string, unique_ptr<Shader>>	m_shader;
	unordered_map<string, unique_ptr<Shader>>	m_blending;
	shared_ptr<Player>							m_player;
	shared_ptr<Camera>							m_camera;
	shared_ptr<GameObject>						m_obj;

	static Scene* m_instansce;
	PLAYERINFO* m_playerInfo;

	Connect connect;
	int m_clientId{};
	SOCKET m_socket;
};