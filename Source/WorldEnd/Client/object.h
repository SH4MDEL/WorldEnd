﻿#pragma once
#include "stdafx.h"
#include "mesh.h"
#include "texture.h"
#include "material.h"

#define ANIMATION_TYPE_ONCE				0
#define ANIMATION_TYPE_LOOP				1

#define ANIMATION_EPSILON		0.00165f

enum class AnimationBlending{ NORMAL, BLENDING };

class AnimationController;
class AnimationSet;

class GameObject : public enable_shared_from_this<GameObject>
{
public:
	GameObject();
	virtual ~GameObject();

	virtual void Update(FLOAT timeElapsed);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, GameObject* rootObject);
	virtual void Move(const XMFLOAT3& shift);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);
	virtual void UpdateTransform(XMFLOAT4X4* parentMatrix = nullptr);
	virtual void UpdateAnimationTransform(XMFLOAT4X4* parentMatrix = nullptr);

	void SetMesh(const shared_ptr<Mesh>& mesh);
	void SetTexture(const shared_ptr<Texture>& texture);
	void SetMaterials(const shared_ptr<Materials>& materials);

	virtual void SetPosition(const XMFLOAT3& position);
	void SetScale(FLOAT x, FLOAT y, FLOAT z);
	void SetWorldMatrix(const XMFLOAT4X4& worldMatrix);

	XMFLOAT4X4 GetWorldMatrix() const { return m_worldMatrix; }
	XMFLOAT3 GetPosition() const;
	XMFLOAT3 GetRight() const { return m_right; }
	XMFLOAT3 GetUp() const { return m_up; }
	XMFLOAT3 GetFront() const { return m_front; }
	FLOAT GetRoll() const { return m_roll; }
	FLOAT GetPitch() const { return m_pitch; }
	FLOAT GetYaw() const { return m_yaw; }

	virtual void ReleaseUploadBuffer() const;

	void SetChild(const shared_ptr<GameObject>& child);
	void SetFrameName(string&& frameName) noexcept;
	shared_ptr<GameObject> FindFrame(string frameName);

	virtual void LoadObject(ifstream& in);
	void LoadObject(ifstream& in, const shared_ptr<GameObject>& rootObject);

	void UpdateBoundingBox();
	void SetBoundingBox(const BoundingOrientedBox& boundingBox);

	string GetFrameName() const { return m_frameName; }

	XMFLOAT4X4 GetTransformMatrix() const { return m_transformMatrix; }
	void SetTransformMatrix(XMFLOAT4X4 mat) { m_transformMatrix = mat; }

	XMFLOAT4X4 GetAnimationMatrix() const { return m_animationMatrix; }
	void SetAnimationMatrix(XMFLOAT4X4 mat) { m_animationMatrix = mat; }

	virtual AnimationController* GetAnimationController() const { return nullptr; }

	virtual void UpdateAnimation(FLOAT timeElapsed) {}

protected:
	XMFLOAT4X4					m_transformMatrix;
	XMFLOAT4X4					m_worldMatrix;
	XMFLOAT4X4					m_animationMatrix;

	XMFLOAT3					m_right;		// 로컬 x축
	XMFLOAT3					m_up;			// 로컬 y축
	XMFLOAT3					m_front;		// 로컬 z축

	FLOAT						m_roll;			
	FLOAT						m_pitch;		
	FLOAT						m_yaw;			

	shared_ptr<Mesh>			m_mesh;			// 메쉬
	shared_ptr<Texture>			m_texture;		// 텍스처
	shared_ptr<Materials>		m_materials;	// 재질

	string						m_frameName;	// 현재 프레임의 이름
	shared_ptr<GameObject>		m_parent;
	shared_ptr<GameObject>		m_sibling;
	shared_ptr<GameObject>		m_child;

	BoundingOrientedBox			m_boundingBox;	
};


class Field : public GameObject
{
public:
	Field(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
		INT width, INT length, INT height, INT blockWidth, INT blockLength, INT blockHeight, XMFLOAT3 scale);
	~Field() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList);
	virtual void Move(const XMFLOAT3& shift);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);

	void SetPosition(const XMFLOAT3& position) override;

	XMFLOAT3 GetPosition() const { return m_blocks.front()->GetPosition(); }
	INT GetWidth() const { return m_width; }
	INT GetLength() const { return m_length; }
	XMFLOAT3 GetScale() const { return m_scale; }

	void ReleaseUploadBuffer() const override;

private:
	unique_ptr<FieldMapImage>		m_fieldMapImage;	// 높이맵 이미지
	vector<unique_ptr<GameObject>>	m_blocks;			// 블록들
	INT								m_width;			// 이미지의 가로 길이
	INT								m_length;			// 이미지의 세로 길이
	INT								m_height;
	XMFLOAT3						m_scale;			// 확대 비율
};

class Fence : public GameObject
{
public:
	Fence(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
		INT width, INT length, INT blockWidth, INT blockLength);
	~Fence() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>&commandList);
	virtual void Move(const XMFLOAT3 & shift);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);

	void SetPosition(const XMFLOAT3 & position) override;

	XMFLOAT3 GetPosition() const { return m_blocks.front()->GetPosition(); }
	INT GetWidth() const { return m_width; }
	INT GetLength() const { return m_length; }

	void ReleaseUploadBuffer() const override;

private:
	vector<unique_ptr<GameObject>>	m_blocks;			// 블록들
	INT								m_width;			// 이미지의 가로 길이
	INT								m_length;			// 이미지의 세로 길이
};

class Skybox : public GameObject
{
public:
	Skybox(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
		FLOAT width, FLOAT length, FLOAT depth);
	~Skybox() = default;

	virtual void Update(FLOAT timeElapsed);
};

class GaugeBar : public GameObject
{
public:
	GaugeBar();
	GaugeBar(FLOAT hpStart);
	~GaugeBar() = default;

	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) override;

	void SetGauge(FLOAT gauge) { m_gauge = gauge; }
	void SetMaxGauge(FLOAT maxGauge) { m_maxGauge = maxGauge; }

private:
	FLOAT	m_gauge;
	FLOAT	m_maxGauge;

	FLOAT	m_border;
};

class AnimationObject : public GameObject
{
public:
	AnimationObject();
	virtual ~AnimationObject() = default;

	virtual AnimationController* GetAnimationController() const { return m_animationController.get(); }

	virtual bool ChangeAnimation(int animation);
	virtual void SetHpBar(const shared_ptr<HpBar>& hpBar) {};

	virtual FLOAT GetMaxHp() const { return 0.f; }
	virtual FLOAT GetHp() const { return 0.f; }

	virtual void Update(FLOAT timeElapsed) override;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) override;

	virtual void UpdateAnimation(FLOAT timeElapsed) override;

	void SetAnimationSet(const shared_ptr<AnimationSet>& animations);
	void SetAnimationOnTrack(int animationTrackNumber, int animation);

	virtual void LoadObject(ifstream& in) override;

protected:
	unique_ptr<AnimationController>		m_animationController;
	int									m_currentAnimation;
};

struct CALLBACKKEY
{
	float	m_time = 0.0f;
	void* m_callbackData = nullptr;
};

class AnimationCallbackHandler
{
public:
	AnimationCallbackHandler() {}
	virtual ~AnimationCallbackHandler() {}

	virtual void Callback(void* callbackData, float trackPosition);
};

class Animation		// 애니메이션 행동 정의
{
public:
	Animation(float length, int framePerSecond, int keyFrames, int skinningBones, string name);
	~Animation();

	string GetName() const { return m_animationName; }
	float GetLength() const { return m_length; }
	vector<float>& GetKeyFrameTimes() { return m_keyFrameTimes; }
	vector<vector<XMFLOAT4X4>>& GetKeyFrameTransforms() { return m_keyFrameTransforms; }
	XMFLOAT4X4 GetTransform(int boneNumber, float position);

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

class AnimationSet		// 애니메이션들의 집합
{
public:
	AnimationSet() = default;
	~AnimationSet() = default;

	vector<shared_ptr<Animation>>& GetAnimations() { return m_animations; }
	vector<string>& GetFrameNames() { return m_frameNames; }

	void LoadAnimationSet(ifstream& in, const string& animationSetName);

private:
	vector<shared_ptr<Animation>>		m_animations;
	vector<string>						m_frameNames;
};

class AnimationTrack		// 애니메이션 제어 클래스
{
public:
	AnimationTrack();
	~AnimationTrack();

	void SetAnimation(int animation) { m_animation = animation; }
	void SetEnable(int enable) { m_enable = enable; }
	void SetSpeed(float speed) { m_speed = speed; }
	void SetPosition(float position) { m_position = position; }
	void SetWeight(float weight) { m_weight = weight; }
	void SetAnimationType(int type) { m_type = type; }

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

	float UpdatePosition(float trackPosition, float timeElapsed, float animationLength);

	void AnimationCallback();

private:
	bool									m_enable;				// 해당 트랙을 활성화 할지
	float									m_speed;				// 애니메이션 재생 속도
	float									m_position;				// 애니메이션 재생 위치
	float									m_weight;				// 애니메이션 블렌딩 가중치
	int										m_animation;			// 애니메이션 번호
	int										m_type;					// 재생 타입(한번, 반복)

	int										m_nCallbackKey;			// 콜백 갯수
	vector<CALLBACKKEY>						m_callbackKeys;			// 콜백 컨테이너

	shared_ptr<AnimationCallbackHandler>	m_animationCallbackHandler;		// 콜벡 핸들러 컨테이너
};

class AnimationController
{
public:
	AnimationController(int animationTracks);
	~AnimationController();

	void SetAnimationSet(const shared_ptr<AnimationSet>& animationSet);

	void SetTrackAnimation(int animationTrack, int animations);
	void SetTrackEnable(int animationTrack, bool enable);
	void SetTrackPosition(int animationTrack, float position);
	void SetTrackSpeed(int animationTrack, float speed);
	void SetTrackWeight(int animationTrack, float weight);
	void SetTrackType(int animationTrack, int type);

	void SetCallbackKeys(int animationTrack, int callbackKeys);
	void SetCallbackKey(int animationTrack, int keyIndex, float  time, void* data);
	void SetAnimationCallbackHandler(int animationTrack, const shared_ptr<AnimationCallbackHandler>& callbackHandler);

	AnimationTrack& GetAnimationTrack(int index) { return m_animationTracks[index]; }
	AnimationSet* GetAnimationSet() const { return m_animationSet.get(); }

	void Update(float timeElapsed, const shared_ptr<GameObject>& rootObject);

	void SetBlendingMode(AnimationBlending blendingMode) { m_blendingMode = blendingMode; }
	AnimationBlending GetBlendingMode() const { return m_blendingMode; }

	
	// 해시맵 관련 함수
	GameObject* GetGameObject(string boneName) { return m_animationTransforms[boneName].second.get(); }

	void InsertObject(string boneName, UINT boneNumber, const shared_ptr<GameObject>& object);

private:
	float									m_time;
	vector<AnimationTrack>					m_animationTracks;

	shared_ptr<AnimationSet>				m_animationSet;		// 등록된 애니메이션 조합

	AnimationBlending						m_blendingMode;

	unordered_map<string, pair<UINT, shared_ptr<GameObject>>>		m_animationTransforms;
};