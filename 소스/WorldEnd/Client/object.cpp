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
	if (m_animationController)
		m_animationController->AdvanceTime(timeElapsed, this);

	if (m_sibling) m_sibling->Update(timeElapsed);
	if (m_child) m_child->Update(timeElapsed);

	UpdateBoundingBox();
}

void GameObject::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	if (m_animationController)
		m_animationController->UpdateShaderVariables(commandList);

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

void GameObject::SetAnimationSet(const shared_ptr<AnimationSet>& animationSet)
{
	if(m_animationController)
		m_animationController->SetAnimationSet(animationSet, this);
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

			// 스킨메쉬는 메쉬정보까지 담고 있으므로
			// 메쉬까지 읽기만 하고 넘기도록 함
			SetMesh(Scene::m_meshs[meshName]);
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

void GameObject::CreateAnimationController(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, int trackNum)
{
	// 3개의 트랙을 가진 애니메이션 컨트롤러 생성
	m_animationController = make_shared<AnimationController>(trackNum);
	m_animationController->SetSkinnedMeshes(device, commandList, *this);
}

void GameObject::FindAndSetSkinnedMesh(vector<SkinnedMesh*>& skinnedMeshes)
{
	if (m_mesh) {
		if (SKINNED_MESH == m_mesh->GetMeshType()) {
			skinnedMeshes.push_back(static_cast<SkinnedMesh*>(m_mesh.get()));
		}
	}

	if (m_sibling) m_sibling->FindAndSetSkinnedMesh(skinnedMeshes);
	if (m_child) m_child->FindAndSetSkinnedMesh(skinnedMeshes);
}

void GameObject::SetAnimationOnTrack(int animationTrackNumber, int animation)
{
	m_animationController->SetTrackAnimation(animationTrackNumber, animation);
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

// 애니메이션
void AnimationCallbackHandler::Callback(void* callbackData, float trackPosition)
{
	// 콜백 처리
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

XMFLOAT4X4 Animation::GetSRT(int boneNumber, float position)
{
	XMFLOAT4X4 transform;
	XMStoreFloat4x4(&transform, XMMatrixIdentity());
	
	size_t keyFrames = m_keyFrameTimes.size();
	for (size_t i = 0; i < keyFrames - 1; ++i) {
		if ((m_keyFrameTimes[i] <= position) && (position < m_keyFrameTimes[i + 1])) {
			float t = (position - m_keyFrameTimes[i]) / (m_keyFrameTimes[i + 1] - m_keyFrameTimes[i]);
			transform = Matrix::Interpolate(m_keyFrameTransforms[i][boneNumber], m_keyFrameTransforms[i + 1][boneNumber], t);
		}
	}

	if (position >= m_keyFrameTimes[keyFrames - 1])
		transform = m_keyFrameTransforms[keyFrames - 1][boneNumber];

	return transform;
}

AnimationSet::AnimationSet(int nAnimation)
{
	m_animations.resize(nAnimation);
}

AnimationTrack::AnimationTrack()
{
	m_enable = true;
	m_speed = 1.0f;
	m_position = -ANIMATION_EPSILON;
	m_weight = 1.0f;
	m_animation = 0;
	m_type = ANIMATION_TYPE_LOOP;
	m_nCallbackKey = 0;
	m_animationCallbackHandler = nullptr;
}

AnimationTrack::~AnimationTrack()
{
}

void AnimationTrack::SetCallbackKeys(int callbackKeys)
{
	m_nCallbackKey = callbackKeys;
	m_callbackKeys.resize(callbackKeys);
}

void AnimationTrack::SetCallbackKey(int Index, float time, void* data)
{
	m_callbackKeys[Index].m_time = time;
	m_callbackKeys[Index].m_callbackData = data;
}

void AnimationTrack::SetAnimationCallbackHandler(const shared_ptr<AnimationCallbackHandler>& callbackHandler)
{
	m_animationCallbackHandler = callbackHandler;
}

float AnimationTrack::UpdatePosition(float trackPosition, float elapsedTime, float animationLength)
{
	float trackElapsedTime = elapsedTime * m_speed;		// 트랙 재생정도 = 시간의 흐름 * 애니메이션 재생속도

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
		if (m_position > animationLength) 
			m_position = animationLength;
	}

	return m_position;
}

void AnimationTrack::AnimationCallback()
{
	// callback 핸들러가 있다면
	// 트랙에 있는 callbackKeys를 검사함
	// callbackKey 에 정의된 callback 시간과 현재 애니메이션 시간이 같은지 검사
	// callbackKey 에 callback 데이터가 있다면 
	// 해당 callback 동작을 수행

	// ex) 공격 동작 중간에 공격판정을 넣고자 한다면 공격 시작시간과 끝시간을 설정해
	// 시작시간엔 공격 판정이 되도록, 끝시간엔 공격 판정이 끝나도록 callback을 설정하면 됨
	if (m_animationCallbackHandler) {
		for (auto& callbackKey : m_callbackKeys) {
			if (abs(callbackKey.m_time - m_position) <= ANIMATION_EPSILON) {
				if (callbackKey.m_callbackData)
					m_animationCallbackHandler->Callback(callbackKey.m_callbackData, m_position);
				break;
			}
		}
	}
}

AnimationController::AnimationController(int animationTracks)
{
	// 컨트롤러 생성 시 원하는 갯수만큼 트랙 생성
	m_animationTracks.resize(animationTracks);
}

AnimationController::~AnimationController()
{
	for (int i = 0; i < m_skinnedMeshes.size(); ++i) {
		m_skinningBoneTransform[i]->Unmap(0, nullptr);
		m_skinningBoneTransform[i]->Release();
	}
}

void AnimationController::UpdateShaderVariables(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	for (int i = 0; i < m_skinnedMeshes.size(); ++i) {
		m_skinnedMeshes[i]->GetSkinningBoneTransform() = m_skinningBoneTransform[i];
		m_skinnedMeshes[i]->GetMappedSkinningBoneTransform() = m_mappedSkinningBoneTransforms[i];
	}
}

void AnimationController::SetAnimationSet(const shared_ptr<AnimationSet>& animationSet, GameObject* rootObject)
{
	m_animationSet = animationSet;

	// 애니메이션이 영향을 주는 뼈대들을 세팅함
	auto& frameCaches = m_animationSet->GetBoneFramesCaches();
	auto& frameNames = m_animationSet->GetFrameNames();

	if (frameCaches[0] == nullptr) {
		for (int i = 0; i < frameCaches.size(); ++i)
			frameCaches[i] = rootObject->FindFrame(frameNames[i]);
	}
}

void AnimationController::SetSkinnedMeshes(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, GameObject& rootObject)
{
	// 오브젝트로 부터 스킨메쉬를 가져옴
	rootObject.FindAndSetSkinnedMesh(m_skinnedMeshes);

	// 스킨메쉬들의 스킨 뼈대 오브젝트들을 세팅함
	for (auto& mesh : m_skinnedMeshes) {
		auto& boneNames = mesh->GetSkinningBoneNames();
		auto& boneFrames = mesh->GetSkinningBoneFrames();

		for (size_t i = 0; i < mesh->GetSkinningBoneNum(); ++i) {
			boneFrames[i] = rootObject.FindFrame(boneNames[i]);
		}
	}

	int meshCount = m_skinnedMeshes.size();
	m_skinningBoneTransform.resize(meshCount);
	m_mappedSkinningBoneTransforms.resize(meshCount);

	// 스킨메쉬 갯수만큼 스키닝 뼈대 변환 버퍼 리소스를 생성하고 
	// 변수에 매핑함
	UINT elementBytes = (((sizeof(XMFLOAT4X4) * SKINNED_BONES) + 255) & ~255);
	for (size_t i = 0; i < meshCount; ++i) {
		m_skinningBoneTransform[i] = CreateBufferResource(device, commandList, nullptr, elementBytes,
			D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, ComPtr<ID3D12Resource>());
		m_skinningBoneTransform[i]->Map(0, nullptr, (void**)&m_mappedSkinningBoneTransforms[i]);
	}
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

void AnimationController::SetCallbackKeys(int animationTrack, int callbackKeys)
{
	if (!m_animationTracks.empty())
		m_animationTracks[animationTrack].SetCallbackKeys(callbackKeys);
}

void AnimationController::SetCallbackKey(int animationTrack, int keyIndex, float time, void* data)
{
	if (!m_animationTracks.empty())
		m_animationTracks[animationTrack].SetCallbackKey(keyIndex, time, data);
}

void AnimationController::SetAnimationCallbackHandler(int animationTrack, const shared_ptr<AnimationCallbackHandler>& callbackHandler)
{
	if (!m_animationTracks.empty())
		m_animationTracks[animationTrack].SetAnimationCallbackHandler(callbackHandler);

}

void AnimationController::AdvanceTime(float timeElapsed, GameObject* rootGameObject)
{
	m_time += timeElapsed;
	//XMFLOAT3 pos = rootGameObject->GetPosition();
	XMFLOAT4X4 mat = rootGameObject->GetTransformMatrix();

	if (!m_animationTracks.empty()) {
		auto& boneFrameCaches = m_animationSet->GetBoneFramesCaches();

		// 애니메이션 적용을 위해 일단 비워둠
		for (size_t i = 0; auto & boneFrame : boneFrameCaches) {
			boneFrame->SetTransformMatrix(Matrix::Zero());
		}

		// 트랙을 돌면서 트랙이 활성화 되어있으면
		for (auto& track : m_animationTracks) {
			if (track.GetEnable()) {

				// 현재 컨트롤러에 등록된 애니메이션 조합에서 
				// 트랙에 등록된 애니메이션 번호에 해당하는 애니메이션을 가져옴
				shared_ptr<Animation> animation = m_animationSet->GetAnimations()[track.GetAnimation()];

				// 트랙의 애니메이션 재생 위치를 갱신하고 갱신된 위치를 가져옴
				float position = track.UpdatePosition(track.GetPosition(), timeElapsed, animation->GetLength());

				// 현재 애니메이션 조합에 등록된 뼈대 이름들을 탐색
				for (int j = 0; j < boneFrameCaches.size(); ++j) {

					// 해당 뼈대의 변환행렬을 가져오고
					// 애니메이션에서 현재 재생위치에 해당하는 변환행렬을 가져옴
					XMFLOAT4X4 transform = boneFrameCaches[j]->GetTransformMatrix();
					XMFLOAT4X4 animationTransform = animation->GetSRT(j, position);

					transform = Matrix::Add(transform, Matrix::Scale(animationTransform, track.GetWeight()));
					 
					boneFrameCaches[j]->SetTransformMatrix(transform);
				}

				track.AnimationCallback();
			}
		}

		// 기존 변환을 적용
		XMFLOAT4X4 result = rootGameObject->GetTransformMatrix();
		result = Matrix::Mul(result, mat);
		rootGameObject->SetTransformMatrix(result);

		rootGameObject->UpdateTransform(nullptr);
	}
}
