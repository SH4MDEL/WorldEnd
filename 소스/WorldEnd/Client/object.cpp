#include "object.h"
#include "scene.h"

GameObject::GameObject() : m_right{ 1.0f, 0.0f, 0.0f }, m_up{ 0.0f, 1.0f, 0.0f }, m_front{ 0.0f, 0.0f, 1.0f }, m_roll{ 0.0f }, m_pitch{ 0.0f }, m_yaw{ 0.0f }
{
	XMStoreFloat4x4(&m_worldMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_transformMatrix, XMMatrixIdentity());
}

GameObject::~GameObject()
{
	//if (m_mesh) m_mesh->ReleaseUploadBuffer();
}

void GameObject::Update(FLOAT timeElapsed)
{
	if (m_sibling) m_sibling->Update(timeElapsed);
	if (m_child) m_child->Update(timeElapsed);

	UpdateBoundingBox();
}

void GameObject::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	XMFLOAT4X4 worldMatrix;
	XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_worldMatrix)));
	commandList->SetGraphicsRoot32BitConstants(0, 16, &worldMatrix, 0);

	if (m_texture) { m_texture->UpdateShaderVariable(commandList); }
	if (m_materials) {
		for (size_t i = 0; const auto& material : m_materials->m_materials) {
			material.UpdateShaderVariable(commandList);
			m_mesh->Render(commandList, i);
			++i;
		}
	}
	else {
		if (m_mesh) m_mesh->Render(commandList);
	}

	if (m_sibling) m_sibling->Render(commandList);
	if (m_child) m_child->Render(commandList);
}

void GameObject::Move(const XMFLOAT3& shift)
{
	SetPosition(Vector3::Add(GetPosition(), shift));
}

void GameObject::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw)
{
	XMMATRIX rotate{ XMMatrixRotationRollPitchYaw(XMConvertToRadians(pitch), XMConvertToRadians(yaw), XMConvertToRadians(roll)) };
	XMMATRIX transformMatrix{ rotate * XMLoadFloat4x4(&m_transformMatrix) };
	XMStoreFloat4x4(&m_transformMatrix, transformMatrix);

	XMStoreFloat3(&m_right, XMVector3TransformNormal(XMLoadFloat3(&m_right), rotate));
	XMStoreFloat3(&m_up, XMVector3TransformNormal(XMLoadFloat3(&m_up), rotate));
	XMStoreFloat3(&m_front, XMVector3TransformNormal(XMLoadFloat3(&m_front), rotate));

	UpdateTransform(nullptr);
}

void GameObject::SetMesh(const shared_ptr<Mesh>& mesh)
{
	if (m_mesh) m_mesh.reset();
	m_mesh = mesh;
}

void GameObject::SetTexture(const shared_ptr<Texture>& texture)
{
	if (m_texture) m_texture.reset();
	m_texture = texture;
}

void GameObject::SetMaterials(const shared_ptr<Materials>& materials)
{
	m_materials = materials;
}

XMFLOAT3 GameObject::GetPosition() const
{
	return XMFLOAT3{ m_worldMatrix._41, m_worldMatrix._42, m_worldMatrix._43 };
}

void GameObject::SetPosition(const XMFLOAT3& position)
{
	m_transformMatrix._41 = position.x;
	m_transformMatrix._42 = position.y;
	m_transformMatrix._43 = position.z;

	UpdateTransform(nullptr);
}

void GameObject::SetScale(FLOAT x, FLOAT y, FLOAT z)
{
	XMMATRIX scale = XMMatrixScaling(x, y, z);
	m_transformMatrix = Matrix::Mul(scale, m_transformMatrix);

	UpdateTransform(nullptr);
}

void GameObject::SetWorldMatrix(const XMFLOAT4X4& worldMatrix)
{
	m_worldMatrix = worldMatrix;
}

void GameObject::UpdateTransform(XMFLOAT4X4* parentMatrix)
{
	m_worldMatrix = (parentMatrix) ? Matrix::Mul(m_transformMatrix, *parentMatrix) : m_transformMatrix;

	if (m_sibling) m_sibling->UpdateTransform(parentMatrix);
	if (m_child) m_child->UpdateTransform(&m_worldMatrix);
}

void GameObject::ReleaseUploadBuffer() const
{
	if (m_mesh) m_mesh->ReleaseUploadBuffer();
	if (m_texture) m_texture->ReleaseUploadBuffer();

	if (m_sibling) m_sibling->ReleaseUploadBuffer();
	if (m_child) m_child->ReleaseUploadBuffer();
}

void GameObject::SetChild(const shared_ptr<GameObject>& child)
{
	if (child) {
		child->m_parent = shared_from_this();
	}
	if (m_child) {
		if (child) child->m_sibling = m_child->m_sibling;
		m_child->m_sibling = child;
	}
	else {
		m_child = child;
	}
}

void GameObject::SetFrameName(string&& frameName) noexcept
{
	m_frameName = move(frameName);
}

shared_ptr<GameObject> GameObject::FindFrame(string frameName)
{
	shared_ptr<GameObject> frame;
	if (m_frameName == frameName) return shared_from_this();

	if (m_sibling) if (frame = m_sibling->FindFrame(frameName)) return frame;
	if (m_child) if (frame = m_child->FindFrame(frameName)) return frame;

	return nullptr;
}

void GameObject::LoadObject(ifstream& in)
{
	BYTE strLength;
	INT frame, texture;

	while (1) {
		in.read((char*)(&strLength), sizeof(BYTE));
		string strToken(strLength, '\0');
		in.read((&strToken[0]), sizeof(char) * strLength);

		if (strToken == "<Frame>:") {
			in.read((char*)(&frame), sizeof(INT));
			in.read((char*)(&texture), sizeof(INT));

			in.read((char*)(&strLength), sizeof(BYTE));
			m_frameName.resize(strLength);
			in.read((&m_frameName[0]), sizeof(char) * strLength);
		}
		else if (strToken == "<Transform>:") {
			XMFLOAT3 position, rotation, scale;
			XMFLOAT4 qrotation;

			in.read((char*)(&position), sizeof(FLOAT) * 3);
			in.read((char*)(&rotation), sizeof(FLOAT) * 3);
			in.read((char*)(&scale), sizeof(FLOAT) * 3);
			in.read((char*)(&qrotation), sizeof(FLOAT) * 4);
		}
		else if (strToken == "<TransformMatrix>:") {
			in.read((char*)(&m_transformMatrix), sizeof(FLOAT) * 16);
		}
		else if (strToken == "<Mesh>:") {
			in.read((char*)(&strLength), sizeof(BYTE));
			string meshName(strLength, '\0');
			in.read(&meshName[0], sizeof(CHAR) * strLength);
			SetMesh(Scene::m_meshs[meshName]);
			SetBoundingBox(m_mesh->GetBoundingBox());
		}
		else if (strToken == "<SkinningInfo>:") {		// 스킨메쉬 정보
			in.read((char*)(&strLength), sizeof(BYTE));
			string meshName(strLength, '\0');
			in.read(&meshName[0], sizeof(CHAR) * strLength);
			//SetMesh(Scene::m_meshs[meshName]);
			//SetBoundingBox(m_mesh->GetBoundingBox());
			// 
			// 스킨 메쉬를 파일에서 읽기만 처리함
			// 스킨 메쉬에 대한 처리는 아직 하지 않음
			// 스킨 메쉬가 여러개이므로 이를 어떻게 처리할지 생각할 필요 있음
		}
		else if (strToken == "<Materials>:") {
			in.read((char*)(&strLength), sizeof(BYTE));
			string materialName(strLength, '\0');
			in.read(&materialName[0], sizeof(CHAR) * strLength);
			SetMaterials(Scene::m_materials[materialName]);
		}
		else if (strToken == "<Children>:") {
			INT childNum = 0;
			in.read((char*)(&childNum), sizeof(INT));
			if (childNum) {
				for (int i = 0; i < childNum; ++i) {
					auto child = make_shared<GameObject>();
					child->LoadObject(in);
					SetChild(child);
				}
			}
		}
		else if (strToken == "</Frame>") {
			break;
		}
	}
}

void GameObject::UpdateBoundingBox()
{
	m_boundingBox.Center = GetPosition();
}

void GameObject::SetBoundingBox(const BoundingOrientedBox& boundingBox)
{
	m_boundingBox = boundingBox;
}

Field::Field(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
	INT width, INT length, INT height, INT blockWidth, INT blockLength, INT blockHeight, XMFLOAT3 scale)
	: m_width{ width }, m_length{ length }, m_height{ height }, m_scale{ scale }
{
	// 높이맵이미지 로딩
	m_fieldMapImage = make_unique<FieldMapImage>(m_width, m_length, m_height, m_scale);

	// 가로, 세로 블록의 개수
	if (height == 0) {
		int widthBlockCount{ m_width / blockWidth };
		int lengthBlockCount{ m_length / blockLength };

		// 블록 생성
		for (int z = 0; z < lengthBlockCount; ++z) {
			for (int x = 0; x < widthBlockCount; ++x)
			{
				int xStart{ x * (blockWidth - 1) };
				int zStart{ z * (blockLength - 1) };
				unique_ptr<GameObject> block{ make_unique<GameObject>() };
				auto mesh = make_shared<FieldMapGridMesh>(device, commandList, xStart, zStart, blockWidth, blockLength, m_scale, m_fieldMapImage.get());
				block->SetMesh(mesh);
				m_blocks.push_back(move(block));
			}
		}
	}
	else if (width == 0) {
		int lengthBlockCount{ m_length / blockLength };
		int heightBlockCount{ m_height / blockHeight };
	}
}

void Field::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	if (m_texture) m_texture->UpdateShaderVariable(commandList);
	for (const auto& block : m_blocks)
		block->Render(commandList);
}

void Field::Move(const XMFLOAT3& shift)
{
	for (auto& block : m_blocks)
		block->Move(shift);
}

void Field::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw)
{
	for (auto& block : m_blocks)
		block->Rotate(roll, pitch, yaw);
}

void Field::SetPosition(const XMFLOAT3& position)
{
	// 지형의 위치 설정은 모든 블록들의 위치를 조정한다는 것임
	for (auto& block : m_blocks)
		block->SetPosition(position);
}

void Field::ReleaseUploadBuffer() const
{
	if (m_texture) m_texture->ReleaseUploadBuffer();
	for (auto& block : m_blocks) {
		block->ReleaseUploadBuffer();
	}
}

Fence::Fence(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
	INT width, INT length, INT blockWidth, INT blockLength)
	: m_width{ width }, m_length{ length }
{
	// 가로, 세로 블록의 개수
	int widthBlockCount{ m_width / blockWidth };
	int lengthBlockCount{ m_length / blockLength };	

	// 블록 생성
	for (int x = 0; x < widthBlockCount; ++x)
	{
		int xStart{ x * blockWidth - m_width / 2 + blockWidth / 2 };
		auto block{ make_unique<GameObject>() };
		auto mesh = make_shared<TextureRectMesh>(device, commandList, XMFLOAT3{ (float)xStart, 0.f, (float)m_length / 2 }, (float)blockWidth, (float)blockWidth, 0.f);
		block->SetMesh(mesh);
		m_blocks.push_back(move(block));
	}
	for (int x = 0; x < widthBlockCount; ++x)
	{
		int xStart{ x * blockWidth - m_width / 2 + blockWidth / 2 };
		auto block{ make_unique<GameObject>() };
		auto mesh = make_shared<TextureRectMesh>(device, commandList, XMFLOAT3{ (float)xStart, 0.f, (float)-m_length / 2 }, (float)blockWidth, (float)blockWidth, 0.f);
		block->SetMesh(mesh);
		m_blocks.push_back(move(block));
	}
	for (int z = 0; z < lengthBlockCount; ++z)
	{
		int zStart{ z * blockLength - m_length / 2 + blockLength / 2 };
		auto block{ make_unique<GameObject>() };
		auto mesh = make_shared<TextureRectMesh>(device, commandList, XMFLOAT3{ (float)m_width / 2, 0.f, (float)zStart }, 0.f, (float)blockLength, (float)blockLength);
		block->SetMesh(mesh);
		m_blocks.push_back(move(block));
	}
	for (int z = 0; z < lengthBlockCount; ++z)
	{
		int zStart{ z * blockLength - m_length / 2 + blockLength / 2 };
		auto block{ make_unique<GameObject>() };
		auto mesh = make_shared<TextureRectMesh>(device, commandList, XMFLOAT3{ (float)-m_width / 2, 0.f, (float)zStart }, 0.f, (float)blockLength, (float)blockLength);
		block->SetMesh(mesh);
		m_blocks.push_back(move(block));
	}
}

void Fence::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	if (m_texture) m_texture->UpdateShaderVariable(commandList);
	for (const auto& block : m_blocks)
		block->Render(commandList);
}

void Fence::Move(const XMFLOAT3& shift)
{
	for (auto& block : m_blocks)
		block->Move(shift);
}

void Fence::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw)
{
	for (auto& block : m_blocks)
		block->Rotate(roll, pitch, yaw);
}

void Fence::SetPosition(const XMFLOAT3& position)
{
	// 지형의 위치 설정은 모든 블록들의 위치를 조정한다는 것임
	for (auto& block : m_blocks)
		block->SetPosition(position);
}

void Fence::ReleaseUploadBuffer() const
{
	if (m_texture) m_texture->ReleaseUploadBuffer();
	for (auto& block : m_blocks) {
		block->ReleaseUploadBuffer();
	}
}

Skybox::Skybox(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
	FLOAT width, FLOAT length, FLOAT depth) : GameObject()
{
	m_mesh = make_unique<SkyboxMesh>(device, commandList, width, length, depth);
}

void Skybox::Update(FLOAT timeElapsed)
{

}

HpBar::HpBar() {}

void HpBar::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	commandList->SetGraphicsRoot32BitConstants(0, 1, &(m_hp), 16);
	commandList->SetGraphicsRoot32BitConstants(0, 1, &(m_maxHp), 17);

	GameObject::Render(commandList);
}
