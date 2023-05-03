#include "objectManager.h"

TowerObjectManager::TowerObjectManager(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, 
	const shared_ptr<Shader>& arrowShader, const shared_ptr<Shader>& magicCircleShader, const shared_ptr<InstancingShader>& arrowRainShader) :
	m_arrowShader{ arrowShader }, m_magicCircleShader{ magicCircleShader }, m_arrowRainShader{ arrowRainShader }
{
	for (auto& arrow : m_arrows) {
		arrow = make_unique<Arrow>();
		arrow->SetMesh("MeshArrow");
		arrow->SetMaterials("Archer_WeaponArrow");
	}

	for (auto& arrowRain : m_arrowRains) {
		arrowRain = make_shared<ArrowRain>();
		m_arrowRainShader->SetObject(arrowRain);
	}
	m_arrowRainShader->SetMesh("MeshArrow");
}

void TowerObjectManager::Update(FLOAT timeElapsed)
{
	for (auto& arrow : m_arrows) {
		arrow->Update(timeElapsed);
	}
	for (auto& arrowRain : m_arrowRains) {
		arrowRain->Update(timeElapsed);
	}
}

void TowerObjectManager::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	commandList->SetPipelineState(m_arrowShader->GetPipelineState().Get());
	for (auto& arrow : m_arrows) {
		arrow->Render(commandList);
	}
	commandList->SetPipelineState(m_magicCircleShader->GetPipelineState().Get());
	for (auto& arrowRain : m_arrowRains) {
		arrowRain->RenderMagicCircle(commandList);
	}
	m_arrowRainShader->Render(commandList);
}

void TowerObjectManager::CreateArrow(const shared_ptr<GameObject>& parent, INT arrowId, FLOAT SPEED)
{
	m_arrows[arrowId]->SetPosition(Vector3::Add(parent->GetPosition(), XMFLOAT3{ 0.f, 0.9f, 0.f }));
	
	//XMFLOAT3 vel = Vector3::Normalize(Vector3::Add(parent->GetFront(), XMFLOAT3{ 0.f, 0.5f, 0.f }));
	
	XMFLOAT3 vel = Vector3::Mul(Vector3::Normalize(parent->GetFront()), SPEED);
	vel = Vector3::Add(vel, { 0.f, 0.15f * SPEED, 0.f });
	m_arrows[arrowId]->SetVelocity(vel);

	//m_arrows[arrowId]->SetVelocity(Vector3::Mul(parent->GetFront(), SPEED));


	m_arrows[arrowId]->Rotate(0.f, 0.f, parent->GetYaw());

	m_arrows[arrowId]->SetEnable();
}

void TowerObjectManager::RemoveArrow(INT arrowId)
{
	m_arrows[arrowId]->SetDisable();
	
	XMFLOAT4X4 transform{};
	XMStoreFloat4x4(&transform, XMMatrixIdentity());
	m_arrows[arrowId]->SetTransformMatrix(transform);
}

void TowerObjectManager::CreateArrowRain(const XMFLOAT3& position)
{
	for (auto& arrowRain : m_arrowRains) {
		if (!arrowRain->IsEnable()) {
			arrowRain->SetPosition(position);
			arrowRain->SetEnable();
			return;
		}
	}
}
