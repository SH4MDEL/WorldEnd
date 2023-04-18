#include "scene.h"

unordered_map<string, shared_ptr<Mesh>>			Scene::m_globalMeshs;
unordered_map<string, shared_ptr<Texture>>		Scene::m_globalTextures;
unordered_map<string, shared_ptr<Materials>>	Scene::m_globalMaterials;
unordered_map<string, shared_ptr<Shader>>		Scene::m_globalShaders;
unordered_map<string, shared_ptr<AnimationSet>>	Scene::m_globalAnimationSets;

unordered_map<string, shared_ptr<Mesh>>			Scene::m_meshs;
unordered_map<string, shared_ptr<Texture>>		Scene::m_textures;
unordered_map<string, shared_ptr<Materials>>	Scene::m_materials;
unordered_map<string, shared_ptr<AnimationSet>>	Scene::m_animationSets;

void LoadingScene::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) {}
void LoadingScene::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) {}
void LoadingScene::OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) {}
void LoadingScene::OnProcessingMouseMessage(UINT message, LPARAM lParam) {}
void LoadingScene::OnProcessingKeyboardMessage(FLOAT timeElapsed) {}

void LoadingScene::Update(FLOAT timeElapsed) {}
void LoadingScene::PreProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) {}
void LoadingScene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) const {}
void LoadingScene::PostProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget, UINT threadIndex) {}
void LoadingScene::RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext) {}

shared_ptr<Shadow> LoadingScene::GetShadow() { return nullptr; }
