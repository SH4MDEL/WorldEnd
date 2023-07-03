#include "object.h"
#include "scene.h"

constexpr int DEFAULT_TRACK_NUM = 3;
constexpr float DEFAULT_WEIGHT_RATIO = 3.f;

GameObject::GameObject() : 
	m_right{ 1.0f, 0.0f, 0.0f }, m_up{ 0.0f, 1.0f, 0.0f }, m_front{ 0.0f, 0.0f, 1.0f }, 
	m_roll{ 0.0f }, m_pitch{ 0.0f }, m_yaw{ 0.0f }
{
	XMStoreFloat4x4(&m_worldMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_transformMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_animationMatrix, XMMatrixIdentity());
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

void GameObject::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	GameObject::UpdateShaderVariable(commandList);

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

void GameObject::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const D3D12_VERTEX_BUFFER_VIEW& instanceBufferView)
{
	GameObject::UpdateShaderVariable(commandList);

	if (m_materials) {
		for (size_t i = 0; const auto& material : m_materials->m_materials) {
			material.UpdateShaderVariable(commandList);
			m_mesh->Render(commandList, i, instanceBufferView);
			++i;
		}
	}
	else {
		if (m_mesh) m_mesh->Render(commandList, instanceBufferView);
	}

	if (m_sibling) m_sibling->Render(commandList, instanceBufferView);
	if (m_child) m_child->Render(commandList, instanceBufferView);
}

void GameObject::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, GameObject* rootObject)
{
	if (m_texture) { m_texture->UpdateShaderVariable(commandList); }
	if (m_materials) {
		for (size_t i = 0; const auto& material : m_materials->m_materials) {
			material.UpdateShaderVariable(commandList);
			m_mesh->Render(commandList, i, rootObject, this);
			++i;
		}
	}
	else {
		if (m_mesh) m_mesh->Render(commandList);
	}

	if (m_sibling) m_sibling->Render(commandList, rootObject);
	if (m_child) m_child->Render(commandList, rootObject);
}

void GameObject::Move(const XMFLOAT3& shift)
{
	SetPosition(Vector3::Add(GetPosition(), shift));
	UpdateBoundingBox();
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

void GameObject::SetMesh(const string& name)
{
	if (m_mesh) m_mesh.reset();
	m_mesh = Scene::m_meshs[name];
}

void GameObject::SetMesh(const shared_ptr<Mesh>& mesh)
{
	m_mesh = mesh;
}

void GameObject::SetTexture(const string& name)
{
	if (m_texture) m_texture.reset();
	m_texture = Scene::m_textures[name];
}

void GameObject::SetMaterials(const string& name)
{
	if (m_materials) m_materials.reset();
	m_materials = Scene::m_materials[name];
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
	// 먼저 Transform Matrix를 설정해준다.
	m_transformMatrix = worldMatrix;

	// Transform Matrix의 정보를 통해 World Matrix를 업데이트한다.
	UpdateTransform(nullptr);
}

void GameObject::UpdateTransform(XMFLOAT4X4* parentMatrix)
{
	m_worldMatrix = (parentMatrix) ? Matrix::Mul(m_transformMatrix, *parentMatrix) : m_transformMatrix;
	UpdateBoundingBox();
	XMStoreFloat4(&m_boundingBox.Orientation, XMQuaternionRotationMatrix(XMLoadFloat4x4(&m_transformMatrix)));

	if (m_sibling) m_sibling->UpdateTransform(parentMatrix);
	if (m_child) m_child->UpdateTransform(&m_worldMatrix);
}

void GameObject::UpdateAnimationTransform(XMFLOAT4X4* parentMatrix)
{
	m_animationMatrix = (parentMatrix) ? Matrix::Mul(m_animationMatrix, *parentMatrix) : m_animationMatrix;

	if (m_sibling) m_sibling->UpdateAnimationTransform(parentMatrix);
	if (m_child) m_child->UpdateAnimationTransform(&m_animationMatrix);
}

void GameObject::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	XMFLOAT4X4 worldMatrix;
	XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_worldMatrix)));
	commandList->SetGraphicsRoot32BitConstants((INT)ShaderRegister::GameObject, 16, &worldMatrix, 0);

	if (m_texture) { m_texture->UpdateShaderVariable(commandList); }
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

			SetMesh(meshName);
			SetBoundingBox(m_mesh->GetBoundingBox());
		}
		else if (strToken == "<SkinningInfo>:") {		// 스킨메쉬 정보
			in.read((char*)(&strLength), sizeof(BYTE));
			string meshName(strLength, '\0');
			in.read(&meshName[0], sizeof(CHAR) * strLength);

			// 스킨메쉬는 메쉬정보까지 담고 있으므로
			// 메쉬까지 읽기만 하고 넘기도록 함
			SetMesh(meshName);
			SetBoundingBox(m_mesh->GetBoundingBox());

			// </SkinningInfo>		읽고 넘김
			in.read((char*)(&strLength), sizeof(BYTE));
			strToken.resize(strLength);
			in.read((&strToken[0]), sizeof(char) * strLength);

			// <Mesh>:		읽고 넘김
			in.read((char*)(&strLength), sizeof(BYTE));
			strToken.resize(strLength);
			in.read((&strToken[0]), sizeof(char) * strLength);
			
		}
		else if (strToken == "<Materials>:") {
			in.read((char*)(&strLength), sizeof(BYTE));
			string materialName(strLength, '\0');
			in.read(&materialName[0], sizeof(CHAR) * strLength);
			SetMaterials(materialName);
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

void GameObject::LoadObject(ifstream& in, const shared_ptr<GameObject>& rootObject)
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

			// 루트오브젝트의 해시맵을 채움
			// 이름과 frame 번호를 이용해서 해시맵을 채움
			rootObject->GetAnimationController()->InsertObject(m_frameName, frame, shared_from_this());
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

			if (Scene::m_meshs[meshName]) Scene::m_meshs[meshName]->CreateShaderVariables(rootObject.get());

			SetMesh(meshName);
			SetBoundingBox(m_mesh->GetBoundingBox());
		}
		else if (strToken == "<SkinningInfo>:") {		// 스킨메쉬 정보
			in.read((char*)(&strLength), sizeof(BYTE));
			string meshName(strLength, '\0');
			in.read(&meshName[0], sizeof(CHAR) * strLength);

			Scene::m_meshs[meshName]->CreateShaderVariables(rootObject.get());

			// 스킨메쉬는 메쉬정보까지 담고 있으므로
			// 메쉬까지 읽기만 하고 넘기도록 함
			SetMesh(meshName);
			SetBoundingBox(m_mesh->GetBoundingBox());

			// </SkinningInfo>		읽고 넘김
			in.read((char*)(&strLength), sizeof(BYTE));
			strToken.resize(strLength);
			in.read((&strToken[0]), sizeof(char) * strLength);

			// <Mesh>:		읽고 넘김
			in.read((char*)(&strLength), sizeof(BYTE));
			strToken.resize(strLength);
			in.read((&strToken[0]), sizeof(char) * strLength);

		}
		else if (strToken == "<Materials>:") {
			in.read((char*)(&strLength), sizeof(BYTE));
			string materialName(strLength, '\0');
			in.read(&materialName[0], sizeof(CHAR) * strLength);
			SetMaterials(materialName);
		}
		else if (strToken == "<Children>:") {
			INT childNum = 0;
			in.read((char*)(&childNum), sizeof(INT));
			if (childNum) {
				for (int i = 0; i < childNum; ++i) {
					auto child = make_shared<GameObject>();
					child->LoadObject(in, rootObject);
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
	if (m_mesh) m_boundingBox.Center = Vector3::Add(m_mesh->GetBoundingBox().Center, GetPosition());
	else m_boundingBox.Center = GetPosition();
	//m_boundingBox.Center = GetPosition();
}

void GameObject::SetBoundingBox(const BoundingOrientedBox& boundingBox)
{
	m_boundingBox = boundingBox;
}

HeightMapTerrain::HeightMapTerrain(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
	const wstring& fileName, INT width, INT length, INT blockWidth, INT blockLength, XMFLOAT3 scale)
	: m_width{ width }, m_length{ length }, m_scale{ scale }
{
	// 높이맵 이미지 로딩
	m_heightMapImage = make_unique<HeightMapImage>(fileName, m_width, m_length, m_scale);

	// 가로, 세로 블록의 개수
	int widthBlockCount{ m_width / blockWidth };
	int lengthBlockCount{ m_length / blockLength };

	// 블록 생성
	for (int z = 0; z < lengthBlockCount; ++z) {
		for (int x = 0; x < widthBlockCount; ++x) {
			int xStart{ x * (blockWidth - 1) };
			int zStart{ z * (blockLength - 1) };
			unique_ptr<GameObject> block{ make_unique<GameObject>() };
			block->SetMesh(make_shared<HeightMapGridMesh>(device, commandList, xStart, zStart, blockWidth, blockLength, m_scale, m_heightMapImage.get()));
			m_blocks.push_back(move(block));
		}
	}
}

void HeightMapTerrain::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	if (m_texture) m_texture->UpdateShaderVariable(commandList);
	for (const auto& block : m_blocks)
		block->Render(commandList);
}

void HeightMapTerrain::Move(const XMFLOAT3& shift)
{
	for (auto& block : m_blocks)
		block->Move(shift);
}

void HeightMapTerrain::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw)
{
	for (auto& block : m_blocks)
		block->Rotate(roll, pitch, yaw);
}

void HeightMapTerrain::SetPosition(const XMFLOAT3& position)
{
	// 지형의 위치 설정은 모든 블록들의 위치를 조정한다는 것임
	for (auto& block : m_blocks)
		block->SetPosition(position);
}

FLOAT HeightMapTerrain::GetHeight(FLOAT x, FLOAT z) const
{
	// 파라미터로 들어온 (x, z)는 플레이어의 위치이다.
	// (x, z)를 대응하는 이미지 좌표로 바꿔줘야한다.

	// 지형의 시작점 반영
	XMFLOAT3 pos{ GetPosition() };
	x -= pos.x;
	z -= pos.z;

	// 회전 반영
	x *= -1;
	z *= -1;

	// 지형의 스케일 반영
	x /= m_scale.x;
	z /= m_scale.z;

	return pos.y + m_heightMapImage->GetHeight(x, z) * 
		m_scale.y * VillageSetting::TERRAIN_HEIGHT_OFFSET;
}

XMFLOAT3 HeightMapTerrain::GetNormal(FLOAT x, FLOAT z) const
{
	// 파라미터로 들어온 (x, z)는 플레이어의 위치이다.

	XMFLOAT3 pos{ GetPosition() };
	x -= pos.x; x /= m_scale.x;
	z -= pos.z; z /= m_scale.z;

	// (x, z) 주변의 노멀 4개를 보간해서 계산
	int ix{ static_cast<int>(x) };
	int iz{ static_cast<int>(z) };
	float fx{ x - ix };
	float fz{ z - iz };

	XMFLOAT3 LT{ m_heightMapImage->GetNormal(ix, iz + 1) };
	XMFLOAT3 LB{ m_heightMapImage->GetNormal(ix, iz) };
	XMFLOAT3 RT{ m_heightMapImage->GetNormal(ix + 1, iz + 1) };
	XMFLOAT3 RB{ m_heightMapImage->GetNormal(ix + 1, iz) };

	XMFLOAT3 bot{ Vector3::Add(Vector3::Mul(LB, 1.0f - fx), Vector3::Mul(RB, fx)) };
	XMFLOAT3 top{ Vector3::Add(Vector3::Mul(LT, 1.0f - fx), Vector3::Mul(RT, fx)) };
	return Vector3::Normalize(Vector3::Add(Vector3::Mul(bot, 1.0f - fz), Vector3::Mul(top, fz)));
}

void HeightMapTerrain::ReleaseUploadBuffer() const
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

GaugeBar::GaugeBar() : m_border{ 0.f } {}

GaugeBar::GaugeBar(FLOAT hpStart) : m_border{ hpStart } {}

void GaugeBar::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	commandList->SetGraphicsRoot32BitConstants((INT)ShaderRegister::GameObject, 1, &(m_gauge), 16);
	commandList->SetGraphicsRoot32BitConstants((INT)ShaderRegister::GameObject, 1, &(m_maxGauge), 17);
	commandList->SetGraphicsRoot32BitConstants((INT)ShaderRegister::GameObject, 1, &(m_border), 18);

	GameObject::Render(commandList);
}

AnimationObject::AnimationObject() : m_currentAnimation{ ObjectAnimation::IDLE },
	m_prevAnimation{ ObjectAnimation::IDLE }
{
	m_animationController = make_unique<AnimationController>(DEFAULT_TRACK_NUM);
}

void AnimationObject::Update(FLOAT timeElapsed)
{
	UpdateAnimation(timeElapsed);

	GameObject::Update(timeElapsed);
}

void AnimationObject::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	if (m_texture) { m_texture->UpdateShaderVariable(commandList); }
	if (m_materials) {
		for (size_t i = 0; const auto & material : m_materials->m_materials) {
			material.UpdateShaderVariable(commandList);
			m_mesh->Render(commandList, i, this, this);
			++i;
		}
	}
	else {
		if (m_mesh) m_mesh->Render(commandList);
	}

	if (m_sibling) m_sibling->Render(commandList, this);
	if (m_child) m_child->Render(commandList, this);
}

void AnimationObject::UpdateAnimation(FLOAT timeElapsed)
{
	m_animationController->Update(timeElapsed, shared_from_this());
}

void AnimationObject::SetAnimationSet(const shared_ptr<AnimationSet>& animationSet, const string& name)
{
	if (m_animationController)
		m_animationController->SetAnimationSet(animationSet, name);
}

void AnimationObject::SetAnimationOnTrack(int animationTrackNumber, int animation)
{
	m_animationController->SetTrackAnimation(animationTrackNumber, animation);
}

void AnimationObject::ChangeAnimationSettings(AnimationBlending blendingMode, int trackType, int trackType1,
	USHORT blendingAnimation)
{
	m_animationController->SetBlendingMode(blendingMode);

	switch (blendingMode) {
		case AnimationBlending::NORMAL:
			m_animationController->SetTrackType(0, trackType);
			m_animationController->SetTrackPosition(0, 0.f);
			m_animationController->SetTrackWeight(0, 1.f);
			m_animationController->SetTrackEnable(1, false);
			break;

		case AnimationBlending::BLENDING: {
			m_animationController->SetTrackType(0, trackType);
			m_animationController->SetTrackPosition(0, 0.f);
			m_animationController->SetTrackWeight(0, 0.f);

			m_animationController->SetTrackEnable(1, true);
			m_animationController->SetTrackType(1, trackType1);
			m_animationController->SetTrackPosition(1, 0.f);
			m_animationController->SetTrackWeight(1, 1.f);
			break; 
		}
	}
}

void AnimationObject::LoadObject(ifstream& in)
{
	GameObject::LoadObject(in, shared_from_this());
}

Animation::Animation(float length, int framePerSecond, int keyFrames, int skinningBones, string name)
{
	m_length = length;
	m_framePerSecond = framePerSecond;
	m_animationName = name;
	m_keyFrameTimes.resize(keyFrames);
	m_keyFrameTransforms.resize(keyFrames);

	for (auto& transforms : m_keyFrameTransforms) {
		transforms.resize(skinningBones);
	}
}

Animation::~Animation()
{
	m_keyFrameTimes.clear();
	for (auto& transforms : m_keyFrameTransforms)
		transforms.clear();
	m_keyFrameTransforms.clear();
}

XMFLOAT4X4 Animation::GetTransform(int boneNumber, float position)
{
	XMFLOAT4X4 transform;
	XMStoreFloat4x4(&transform, XMMatrixIdentity());
	
	size_t keyFrames = m_keyFrameTimes.size();
	for (size_t i = 0; i < keyFrames - 1; ++i) {
		if ((m_keyFrameTimes[i] <= position) && (position < m_keyFrameTimes[i + 1])) {
			float t = (position - m_keyFrameTimes[i]) / (m_keyFrameTimes[i + 1] - m_keyFrameTimes[i]);
			transform = Matrix::Interpolate(m_keyFrameTransforms[i][boneNumber], m_keyFrameTransforms[i + 1][boneNumber], t);
			break;
		}
	}

	if (position >= m_keyFrameTimes[keyFrames - 1])
		transform = m_keyFrameTransforms[keyFrames - 1][boneNumber];

	return transform;
}

void AnimationSet::LoadAnimationSet(ifstream& in, const string& animationSetName)
{
	BYTE strLength{};
	INT frameNum{};

	while (true) {
		in.read((char*)(&strLength), sizeof(BYTE));
		string strToken(strLength, '\0');
		in.read((&strToken[0]), sizeof(char) * strLength);

		if (strToken == "<FrameNames>:") {
			// 애니메이션이 적용될 뼈 개수 resize
			in.read((char*)(&frameNum), sizeof(INT));

			m_frameNames.resize(frameNum);

			// 프레임이름을 담는 벡터에
			// 각 이름을 읽어서 넣음
			for (int i = 0; i < frameNum; ++i) {
				in.read((char*)(&strLength), sizeof(BYTE));
				m_frameNames[i].resize(strLength);
				in.read((char*)(m_frameNames[i].data()), sizeof(char) * strLength);
			}
		}
		else if (strToken == "<AnimationSet>:") {
			// 애니메이션 번호, 이름, 시간, 초당 프레임, 총 프레임
			INT animationNum{};
			in.read((char*)(&animationNum), sizeof(INT));

			in.read((char*)(&strLength), sizeof(BYTE));
			string animationName(strLength, '\0');
			in.read(&animationName[0], sizeof(char) * strLength);

			float animationLength{};
			in.read((char*)(&animationLength), sizeof(float));

			INT framePerSecond{};
			in.read((char*)(&framePerSecond), sizeof(INT));

			INT totalFrames{};
			in.read((char*)(&totalFrames), sizeof(INT));

			auto animation = make_shared<Animation>(animationLength, framePerSecond,
				totalFrames, frameNum, animationName);

			auto& keyFrameTimes = animation->GetKeyFrameTimes();
			auto& keyFrameTransforms = animation->GetKeyFrameTransforms();

			for (int i = 0; i < totalFrames; ++i) {
				in.read((char*)(&strLength), sizeof(BYTE));
				string strToken(strLength, '\0');
				in.read(&strToken[0], sizeof(char) * strLength);

				// 키프레임 번호, 키프레임 시간, 키프레임 행렬들
				if (strToken == "<Transforms>:") {
					INT keyFrameNum{};
					in.read((char*)(&keyFrameNum), sizeof(INT));

					float keyFrameTime{};
					in.read((char*)(&keyFrameTime), sizeof(float));

					keyFrameTimes[i] = keyFrameTime;
					in.read((char*)(keyFrameTransforms[i].data()), sizeof(XMFLOAT4X4) * frameNum);
				}
			}

			m_animations.emplace_back(move(animation));
		}
		else if (strToken == "</AnimationSets>") {
			break;
		}
	}
}

AnimationTrack::AnimationTrack()
{
	m_enable = true;
	m_speed = 1.0f;
	m_position = -ANIMATION_EPSILON;
	m_weight = 1.0f;
	m_animation = 0;
	m_type = ANIMATION_TYPE_LOOP;
}

AnimationTrack::~AnimationTrack()
{
}

void AnimationTrack::IncreaseWeight(float timeElapsed)
{
	m_weight += timeElapsed * DEFAULT_WEIGHT_RATIO;
}

void AnimationTrack::DecreaseWeight(float timeElapsed)
{
	m_weight -= timeElapsed * DEFAULT_WEIGHT_RATIO;
}

float AnimationTrack::UpdatePosition(float trackPosition, float timeElapsed, float animationLength)
{
	float trackElapsedTime = timeElapsed * m_speed;		// 트랙 재생정도 = 시간의 흐름 * 애니메이션 재생속도

	if (ANIMATION_TYPE_LOOP == m_type) {
		if (m_position < 0.0f)
			m_position = 0.0f;
		else {
			m_position = trackPosition + trackElapsedTime;
			if (m_position > animationLength) {
				m_position = -ANIMATION_EPSILON;
				return animationLength;
			}
		}
	}
	else if (ANIMATION_TYPE_ONCE == m_type) {
		m_position = trackPosition + trackElapsedTime;
		if (m_position > animationLength) {
			m_position = animationLength;
		}
	}

	return m_position;
}

AnimationController::AnimationController(int animationTracks)
{
	// 컨트롤러 생성 시 원하는 갯수만큼 트랙 생성
	m_animationTracks.resize(animationTracks);
	m_blendingMode = AnimationBlending::NORMAL;
}

AnimationController::~AnimationController()
{

}

void AnimationController::SetAnimationSet(const shared_ptr<AnimationSet>& animationSet, const string& name)
{
	if (m_animationSet) m_animationSet.reset();
	if (animationSet) m_animationSet = animationSet;
	else m_animationSet = Scene::m_animationSets[name];
}

void AnimationController::SetTrackAnimation(int animationTrack, int animation)
{
	if (!m_animationTracks.empty())
		m_animationTracks[animationTrack].SetAnimation(animation);
}

void AnimationController::SetTrackEnable(int animationTrack, bool enable)
{
	if (!m_animationTracks.empty())
		m_animationTracks[animationTrack].SetEnable(enable);
}

void AnimationController::SetTrackPosition(int animationTrack, float position)
{
	if (!m_animationTracks.empty())
		m_animationTracks[animationTrack].SetPosition(position);
}

void AnimationController::SetTrackSpeed(int animationTrack, float speed)
{
	if (!m_animationTracks.empty())
		m_animationTracks[animationTrack].SetSpeed(speed);
}

void AnimationController::SetTrackWeight(int animationTrack, float weight)
{
	if (!m_animationTracks.empty())
		m_animationTracks[animationTrack].SetWeight(weight);
}

void AnimationController::SetTrackType(int animationTrack, int type)
{
	if (!m_animationTracks.empty())
		m_animationTracks[animationTrack].SetAnimationType(type);
}

void AnimationController::Update(float timeElapsed, const shared_ptr<GameObject>& rootObject)
{
	if (!m_animationTracks.empty()) {

		// 하나의 애니메이션 행렬을 그대로 저장
		if (m_blendingMode == AnimationBlending::NORMAL) {

			// 트랙을 돌면서 트랙이 활성화 되어있으면
			for (auto& track : m_animationTracks) {
				if (!track.GetEnable()) continue;

				// 현재 컨트롤러에 등록된 애니메이션 조합에서 
				// 트랙에 등록된 애니메이션 번호에 해당하는 애니메이션을 가져옴
				shared_ptr<Animation> animation = m_animationSet->GetAnimations()[track.GetAnimation()];

				// 트랙의 애니메이션 재생 위치를 갱신하고 갱신된 위치를 가져옴
				float position = track.UpdatePosition(track.GetPosition(), timeElapsed, animation->GetLength());

				// 애니메이션 조합은 애니메이션이 적용될 오브젝트의 이름이 나열되어 있음
				// 이를 이용해 애니메이션 변환을 갱신함
				for (const string& str : m_animationSet->GetFrameNames()) {
					
					// 이름으로 해당 뼈대의 번호와 오브젝트를 참조함
					pair<UINT, shared_ptr<GameObject>>& p = m_animationTransforms[str];
					
					// 해당 오브젝트를 찾아가 애니메이션 변환 값을 변경
					p.second->SetAnimationMatrix(animation->GetTransform(p.first, position));
				}

			}
		}

		// 여러 애니메이션 행렬을 섞어서 저장
		else {

			for (auto& bone : m_animationTransforms)
				bone.second.second->SetAnimationMatrix(Matrix::Zero());

			for (auto & track : m_animationTracks) {
				if (!track.GetEnable()) continue; 

				shared_ptr<Animation> animation = m_animationSet->GetAnimations()[track.GetAnimation()];

				float position = track.UpdatePosition(track.GetPosition(), timeElapsed, animation->GetLength());

				for (const string& str : m_animationSet->GetFrameNames()) {
					pair<UINT, shared_ptr<GameObject>>& p = m_animationTransforms[str];
					XMFLOAT4X4 transform = animation->GetTransform(p.first, position);

					p.second->SetAnimationMatrix(Matrix::Add(p.second->GetAnimationMatrix(), 
						Matrix::Scale(transform, track.GetWeight())));
				}
			}

			m_animationTracks[0].IncreaseWeight(timeElapsed);
			m_animationTracks[1].DecreaseWeight(timeElapsed);
			if (m_animationTracks[0].GetWeight() >= 1.f) {
				m_blendingMode = AnimationBlending::NORMAL;
				m_animationTracks[1].SetEnable(false);
			}
			/*else if (ANIMATION_TYPE_ONCE == m_animationTracks[1].GetAnimationType()&&
				m_animationTracks[1].GetPosition() == m_animationSet->GetAnimations()[m_animationTracks[1].GetAnimation()]->GetLength())
			{
				m_blendingMode = AnimationBlending::NORMAL;
				m_animationTracks[1].SetEnable(false);
			}*/
			
		}
		rootObject->UpdateAnimationTransform(nullptr);

	}
}

void AnimationController::InsertObject(string boneName, UINT boneNumber, const shared_ptr<GameObject>& object)
{
	XMFLOAT4X4 transform;
	XMStoreFloat4x4(&transform, XMMatrixIdentity());
	m_animationTransforms.insert({ boneName, make_pair(boneNumber, object) });
}

WarpGate::WarpGate() : m_age{0.f}, m_interect{false}
{
}

void WarpGate::Update(FLOAT timeElapsed)
{
	if (!m_interect) return;

	m_age += timeElapsed;
	if (m_age >= m_lifeTime) {
		m_age = 0;
		m_interect = false;

		auto position = GetPosition();
		position.y = m_originHeight;
		SetPosition(position);

		m_event();
	}
	auto position = GetPosition();
	position.y = m_originHeight + m_maxHeight * (m_age / m_lifeTime);
	SetPosition(position);
}

void WarpGate::SetInterect(function<void()> event)
{
	m_event = event;
	m_interect = true;
	m_originHeight = GetPosition().y;
}