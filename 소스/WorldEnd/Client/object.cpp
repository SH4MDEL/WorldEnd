#include "object.h"

GameObject::GameObject() : m_right{ 1.0f, 0.0f, 0.0f }, m_up{ 0.0f, 1.0f, 0.0f }, m_front{ 0.0f, 0.0f, 1.0f }, m_roll{ 0.0f }, m_pitch{ 0.0f }, m_yaw{ 0.0f }
{
	XMStoreFloat4x4(&m_worldMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_transformMatrix, XMMatrixIdentity());
}

GameObject::~GameObject()
{
	if (m_mesh) m_mesh->ReleaseUploadBuffer();
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
	if (m_material) { m_material->UpdateShaderVariable(commandList); }

	if (m_mesh) m_mesh->Render(commandList);

	if (m_sibling) m_sibling->Render(commandList);
	if (m_child) m_child->Render(commandList);
}

void GameObject::Move(const XMFLOAT3& shift)
{
	SetPosition(Vector3::Add(GetPosition(), shift));
}

void GameObject::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw)
{
	// 회전
	XMMATRIX rotate{ XMMatrixRotationRollPitchYaw(XMConvertToRadians(roll), XMConvertToRadians(pitch), XMConvertToRadians(yaw)) };
	XMMATRIX transformMatrix{ rotate * XMLoadFloat4x4(&m_transformMatrix) };
	XMStoreFloat4x4(&m_transformMatrix, transformMatrix);

	// 로컬 x,y,z축 최신화
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

void GameObject::SetMaterial(const shared_ptr<Material>& material)
{
	if (m_material) m_material.reset();
	m_material = material;
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
		child->m_parent = (shared_ptr<GameObject>)this;
	}
	if (m_child) {
		if (child) child->m_sibling = m_child->m_sibling;
		m_child->m_sibling = child;
	}
	else {
		m_child = child;
	}
}

shared_ptr<GameObject> GameObject::FindFrame(string frameName)
{
	shared_ptr<GameObject> frame;
	if (m_frameName == frameName) return (shared_ptr<GameObject>)this;

	if (m_sibling) if (frame = m_sibling->FindFrame(frameName)) return frame;
	if (m_child) if (frame = m_child->FindFrame(frameName)) return frame;

	return nullptr;
}

void GameObject::LoadGeometry(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const wstring& fileName)
{
	ifstream in{ fileName, std::ios::binary };
	if (!in) return;

	LoadFrameHierarchy(device, commandList, in);
}

void GameObject::LoadFrameHierarchy(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, ifstream& in)
{
	BYTE strLength;
	INT frame, texture;

	unique_ptr<MeshFromFile> mesh = make_unique<MeshFromFile>();
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
			mesh->LoadMesh(device, commandList, in);
			SetBoundingBox(mesh->GetBoundingBox());
		}
		else if (strToken == "<Materials>:") {
			mesh->LoadMaterial(device, commandList, in);
		}
		else if (strToken == "<Children>:") {
			INT childNum = 0;
			in.read((char*)(&childNum), sizeof(INT));
			if (childNum) {
				for (int i = 0; i < childNum; ++i) {
					shared_ptr<GameObject> child = make_shared<GameObject>();
					child->LoadFrameHierarchy(device, commandList, in);
					SetChild(child);
				}
			}
		}
		else if (strToken == "</Frame>") {
			break;
		}
	}
	m_mesh = move(mesh);
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
