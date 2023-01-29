#pragma once
#include "stdafx.h"
#include "mesh.h"
#include "texture.h"
#include "material.h"

#define ANIMATION_TYPE_ONCE				0
#define ANIMATION_TYPE_LOOP				1

#define ANIMATION_EPSILON		0.00165f

class AnimationController;
class AnimationSet;

class GameObject
{
public:
	GameObject();
	virtual ~GameObject();

	virtual void Update(FLOAT timeElapsed);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	virtual void Move(const XMFLOAT3& shift);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);
	virtual void UpdateTransform(XMFLOAT4X4* parentMatrix = nullptr);

	void SetMesh(const shared_ptr<Mesh>& mesh);
	void SetTexture(const shared_ptr<Texture>& texture);
	void SetMaterials(const shared_ptr<Materials>& materials);
	void SetAnimationSet(const shared_ptr<AnimationSet>& animations);

	void SetPosition(const XMFLOAT3& position);
	void SetScale(FLOAT x, FLOAT y, FLOAT z);
	void SetWorldMatrix(const XMFLOAT4X4& worldMatrix);

	XMFLOAT4X4 GetWorldMatrix() const { return m_worldMatrix; }
	XMFLOAT3 GetPosition() const;
	XMFLOAT3 GetRight() const { return m_right; }
	XMFLOAT3 GetUp() const { return m_up; }
	XMFLOAT3 GetFront() const { return m_front; }

	virtual void ReleaseUploadBuffer() const;

	void SetChild(const shared_ptr<GameObject>& child);
	shared_ptr<GameObject> FindFrame(string frameName);

	void LoadObject(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, ifstream& in);

	void UpdateBoundingBox();
	void SetBoundingBox(const BoundingOrientedBox& boundingBox);

	//XMFLOAT3 GetPositionX(return XMFLOAT3{ m_worldMatrix._41 };)



	XMFLOAT4X4 GetTransformMatrix() const { return m_transformMatrix; }
	void SetTransformMatrix(XMFLOAT4X4 mat) { m_transformMatrix = mat; }

	// ������Ʈ���� �ִϸ��̼����� �б�, ��Ų�޽� ã��, �ִϸ��̼� ���� �Լ� �߰�
	void CreateAnimationController(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, int trackNum);

	shared_ptr<SkinnedMesh> FindSkinnedMesh(string meshName);
	void FindAndSetSkinnedMesh(vector<SkinnedMesh*>& skinnedMeshes);

	void SetAnimationOnTrack(int animationTrackNumber, int animation);
	void SetAnimationPositionOnTrack(int animationTrackNumber, float position);

	AnimationController* GetAnimationController() const { return m_animationController.get(); }

	void Check() {
		XMFLOAT3 p = GetPosition();
		cout << m_frameName << " : " << p.x << ", " << p.y << ", " << p.z << endl;

		if (m_sibling) m_sibling->Check();
		if (m_child) m_child->Check();
	}
	
	void CheckMat(){
		float x = m_transformMatrix._41;
		float y = m_transformMatrix._42;
		float z = m_transformMatrix._43;
		cout << m_frameName << " : " << x << ", " << y << ", " << z << endl;

		if (m_sibling) m_sibling->CheckMat();
		if (m_child) m_child->CheckMat();
	}

	string GetName() const {
		return m_frameName;
	}

protected:
	XMFLOAT4X4					m_transformMatrix;
	XMFLOAT4X4					m_worldMatrix;
	

	XMFLOAT3					m_right;		// ���� x��
	XMFLOAT3					m_up;			// ���� y��
	XMFLOAT3					m_front;		// ���� z��

	FLOAT						m_roll;			// x�� ȸ����
	FLOAT						m_pitch;		// y�� ȸ����
	FLOAT						m_yaw;			// z�� ȸ����

	shared_ptr<Mesh>			m_mesh;			// �޽�
	shared_ptr<Texture>			m_texture;		// �ؽ�ó
	shared_ptr<Materials>		m_materials;	// ����

	string						m_frameName;	// ���� �������� �̸�
	shared_ptr<GameObject>		m_parent;
	shared_ptr<GameObject>		m_sibling;
	shared_ptr<GameObject>		m_child;

	BoundingOrientedBox			m_boundingBox;	

	shared_ptr<AnimationController>		m_animationController;
};

class Field : public GameObject
{
public:
	Field(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
		INT width, INT length, INT height, INT blockWidth, INT blockLength, INT blockHeight, XMFLOAT3 scale);
	~Field() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	virtual void Move(const XMFLOAT3& shift);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);

	void SetPosition(const XMFLOAT3& position);

	XMFLOAT3 GetPosition() const { return m_blocks.front()->GetPosition(); }
	INT GetWidth() const { return m_width; }
	INT GetLength() const { return m_length; }
	XMFLOAT3 GetScale() const { return m_scale; }

	void ReleaseUploadBuffer() const override;

private:
	unique_ptr<FieldMapImage>		m_fieldMapImage;	// ���̸� �̹���
	vector<unique_ptr<GameObject>>	m_blocks;			// ��ϵ�
	INT								m_width;			// �̹����� ���� ����
	INT								m_length;			// �̹����� ���� ����
	INT								m_height;
	XMFLOAT3						m_scale;			// Ȯ�� ����
};

class Fence : public GameObject
{
public:
	Fence(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
		INT width, INT length, INT blockWidth, INT blockLength);
	~Fence() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>&commandList) const;
	virtual void Move(const XMFLOAT3 & shift);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);

	void SetPosition(const XMFLOAT3 & position);

	XMFLOAT3 GetPosition() const { return m_blocks.front()->GetPosition(); }
	INT GetWidth() const { return m_width; }
	INT GetLength() const { return m_length; }

	void ReleaseUploadBuffer() const override;

private:
	vector<unique_ptr<GameObject>>	m_blocks;			// ��ϵ�
	INT								m_width;			// �̹����� ���� ����
	INT								m_length;			// �̹����� ���� ����
};


class Skybox : public GameObject
{
public:
	Skybox(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
		FLOAT width, FLOAT length, FLOAT depth);
	~Skybox() = default;

	virtual void Update(FLOAT timeElapsed);
};


struct CALLBACKKEY
{
	float	m_time = 0.0f;
	void*	m_callbackData = nullptr;
};

class AnimationCallbackHandler
{
public:
	AnimationCallbackHandler() {}
	~AnimationCallbackHandler() {}

	void Callback(void* callbackData, float trackPosition);
};

class Animation		// �ִϸ��̼� �ൿ ����
{
public:
	Animation(float length, int framePerSecond, int keyFrames, int skinningBones, string name);
	~Animation();

	string GetName() const { return m_animationName; }
	float GetLength() const { return m_length; }
	vector<float>& GetKeyFrameTimes() { return m_keyFrameTimes; }
	vector<vector<XMFLOAT4X4>>& GetKeyFrameTransforms() { return m_keyFrameTransforms; }
	XMFLOAT4X4 GetSRT(int boneNumber, float position);


	void SetName(const string_view s) { m_animationName = s; }
	void SetLength(float length) { m_length = length; }
	void SetFramePerSecond(int i) { m_framePerSecond = i; }
	

private:
	string								m_animationName;
	float								m_length;
	int									m_framePerSecond;

	vector<float>						m_keyFrameTimes;
	vector<vector<XMFLOAT4X4>>			m_keyFrameTransforms;
};

class AnimationSet		// �ִϸ��̼ǵ��� ����
{
public:
	AnimationSet() = default;
	AnimationSet(int nAnimation);
	~AnimationSet() = default;

	vector<shared_ptr<Animation>>& GetAnimations() { return m_animations; }
	vector<shared_ptr<GameObject>>& GetBoneFramesCaches() { return m_animatedBoneFramesCaches; }
	vector<string>& GetFrameNames() { return m_frameNames; }

private:
	vector<shared_ptr<Animation>>		m_animations;
	vector<string>						m_frameNames;

	vector<shared_ptr<GameObject>>		m_animatedBoneFramesCaches;
};

class AnimationTrack		// �ִϸ��̼� ���� Ŭ����
{
public:
	AnimationTrack();
	~AnimationTrack();

	void SetAnimation(int animation) { m_animation = animation; }
	void SetEnable(int enable) { m_enable = enable; }
	void SetSpeed(float speed) { m_speed = speed; }
	void SetPosition(float position) { m_position = position; }
	void SetWeight(float weight) { m_weight = weight; }

	void SetCallbackKeys(int callbackKeys);
	void SetCallbackKey(int keyIndex, float time, void* data);
	void SetAnimationCallbackHandler(const shared_ptr<AnimationCallbackHandler>& callbackHandler);


	bool  GetEnable() const { return m_enable; }
	float GetSpeed() const { return m_speed; }
	float GetPosition() const { return m_position; }
	float GetWeight() const { return m_weight; }
	int   GetAnimation() const { return m_animation; }

	int GetNumberOfCallbackKey() const { return m_nCallbackKey; }
	vector<CALLBACKKEY> GetCallbackKeys() const { return m_callbackKeys; }
	AnimationCallbackHandler* GetAnimationCallbackHandler() const { return m_animationCallbackHandler.get(); }

	float UpdatePosition(float trackPosition, float trackElapsedTime, float animationLength);

	void AnimationCallback();

private:
	bool									m_enable;				// �ش� Ʈ���� Ȱ��ȭ ����
	float									m_speed;				// �ִϸ��̼� ��� �ӵ�
	float									m_position;				// �ִϸ��̼� ��� ��ġ
	float									m_weight;				// �ִϸ��̼� ���� ����ġ
	int										m_animation;			// �ִϸ��̼� ��ȣ
	int										m_type;					// ��� Ÿ��(�ѹ�, �ݺ�)

	int										m_nCallbackKey;			// �ݹ� ����
	vector<CALLBACKKEY>						m_callbackKeys;			// �ݹ� �����̳�

	shared_ptr<AnimationCallbackHandler>	m_animationCallbackHandler;		// �ݺ� �ڵ鷯 �����̳�
};

class AnimationController
{
public:
	AnimationController(int animationTracks);
	~AnimationController();

	void UpdateShaderVariables(const ComPtr<ID3D12GraphicsCommandList>& commandList);

	void SetAnimationSet(const shared_ptr<AnimationSet>& animationSet, GameObject* rootObject);
	void SetSkinnedMeshes(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, GameObject& rootObject);

	void SetTrackAnimation(int animationTrack, int animations);
	void SetTrackEnable(int animationTrack, bool enable);
	void SetTrackPosition(int animationTrack, float position);
	void SetTrackSpeed(int animationTrack, float speed);
	void SetTrackWeight(int animationTrack, float weight);

	void SetCallbackKeys(int animationTrack, int callbackKeys);
	void SetCallbackKey(int animationTrack, int keyIndex, float  time, void* data);
	void SetAnimationCallbackHandler(int animationTrack, const shared_ptr<AnimationCallbackHandler>& callbackHandler);

	AnimationTrack& GetAnimationTrack(int index) { return m_animationTracks[index]; }
	AnimationSet* GetAnimationSet() const { return m_animationSet.get(); }

	void AdvanceTime(float timeElapsed, GameObject* rootGameObject);

private:
	float									m_time;
	vector<AnimationTrack>					m_animationTracks;

	shared_ptr<AnimationSet>				m_animationSet;		// ��ϵ� �ִϸ��̼� ����

	vector<SkinnedMesh*>					m_skinnedMeshes;

	vector<ComPtr<ID3D12Resource>>			m_skinningBoneTransform;
	vector<XMFLOAT4X4*>						m_mappedSkinningBoneTransforms;	// skinmesh �� ���� ��ȯ���
};
