#include "loginScene.h"

LoginScene::LoginScene()
{
}

LoginScene::~LoginScene()
{
}

void LoginScene::OnCreate(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postRootSignature)
{
}

void LoginScene::OnDestroy()
{
}

void LoginScene::ReleaseUploadBuffer()
{
}

void LoginScene::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
}

void LoginScene::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
}

void LoginScene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature, const ComPtr<ID3D12RootSignature>& postRootSignature)
{
}

void LoginScene::OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime)
{
}

void LoginScene::OnProcessingMouseMessage(UINT message, LPARAM lParam)
{
}

void LoginScene::OnProcessingKeyboardMessage(FLOAT timeElapsed)
{
}

void LoginScene::Update(FLOAT timeElapsed)
{
}

void LoginScene::PreProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex)
{
}

void LoginScene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) const
{
}

void LoginScene::PostProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget, UINT threadIndex)
{
}

void LoginScene::RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext)
{
}