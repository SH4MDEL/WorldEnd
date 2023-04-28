#include "objectManager.h"

TowerObjectManager::TowerObjectManager(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, 
	const shared_ptr<Shader>& arrowShader, const shared_ptr<Shader>& magicCircleShader, const shared_ptr<Shader>& arrowRainShader) :
	m_arrowShader{ arrowShader }, m_magicCircleShader{ magicCircleShader }, m_arrowRainShader{ arrowRainShader }
{
	for (auto& arrow : m_arrows) {
		arrow = make_unique<Arrow>();
		arrow->SetMesh("MeshArrow");
		arrow->SetMaterials("Archer_WeaponArrow");
	}

	for (auto& arrowRain : m_arrowRains) {
		arrowRain = make_unique<ArrowRain>();
	}
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
	commandList->SetPipelineState(m_arrowRainShader->GetPipelineState().Get());
	for (auto& arrowRain : m_arrowRains) {
		arrowRain->Render(commandList);
	}
}

void TowerObjectManager::CreateArrow(const shared_ptr<GameObject>& parent, INT arrowId)
{
	m_arrows[arrowId]->SetPosition(Vector3::Add(parent->GetPosition(), XMFLOAT3{ 0.f, 0.9f, 0.f }));

	m_arrows[arrowId]->SetVelocity({ Vector3::Mul(Vector3::Normalize(parent->GetFront()), PlayerSetting::ARROW_SPEED) });
	m_arrows[arrowId]->Rotate(0.f, 0.f, parent->GetYaw());

	m_arrows[arrowId]->SetEnable();
}

void TowerObjectManager::RemoveArrow(INT arrowId)
{
	m_arrows[arrowId]->SetDisable();
}

void TowerObjectManager::CreateArrowRain(const shared_ptr<GameObject>& parent)
{
	for (auto& arrowRain : m_arrowRains) {
		if (!arrowRain->IsEnable()) {
			arrowRain->SetPosition(Vector3::Add(Vector3::Mul(parent->GetFront(), 2.f), parent->GetPosition()));
			arrowRain->SetEnable();
			return;
		}
	}
}
