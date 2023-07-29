#pragma once
#include "stdafx.h"
#include "player.h"

// 카메라 클래스는 두 종류의 중요한 정보를 저장한다
// 1. 카메라 좌표계를 정의하는 속성
// 2. 사야 절두체를 정의하는 속성

namespace CameraSetting
{
	using namespace DirectX;

	constexpr float MAX_PITCH = 30.f;
	constexpr float MIN_PITCH = 0.f;
	constexpr XMFLOAT3 DEFAULT_OFFSET = { 0.f, 1.f, -9.f };

	constexpr float CAMERA_WAITING_TIME = 1.f;
}

struct CameraInfo
{
	XMFLOAT4X4			viewMatrix;	// 뷰변환 행렬
	XMFLOAT4X4			projMatrix;	// 투영변환 행렬
	XMFLOAT3			eye;	// 카메라 위치
};

class Camera
{
public:
	Camera();
	~Camera() = default;

	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void UpdateLocalAxis();

	virtual void Update(FLOAT timeElapsed) = 0;
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw, FLOAT timeElapsed) = 0;

	void Move(const XMFLOAT3& shift);

	XMFLOAT4X4 GetViewMatrix() const { return m_viewMatrix; }
	XMFLOAT4X4 GetProjMatrix() const { return m_projMatrix; }
	BoundingFrustum& GetViewFrustum() { return m_viewFrustum; }
	void SetViewMatrix(const XMFLOAT4X4& viewMatrix) { m_viewMatrix = viewMatrix; }
	void SetProjMatrix(const XMFLOAT4X4& projMatrix) { m_projMatrix = projMatrix; }

	XMFLOAT3 GetEye() const { return m_eye; }
	XMFLOAT3 GetRight() const { return m_right; }
	XMFLOAT3 GetUp() const { return m_up; }
	XMFLOAT3 GetLook() const { return m_look; }
	void SetEye(const XMFLOAT3& eye) { m_eye = eye; UpdateLocalAxis(); }
	void SetLook(const XMFLOAT3& look) { m_look = look; UpdateLocalAxis(); }
	void SetUp(const XMFLOAT3& up) { m_up = up; UpdateLocalAxis(); }

	shared_ptr<Player> GetPlayer() const { return m_player; }
	void SetPlayer(const shared_ptr<Player>& player);

protected:
	XMFLOAT4X4				m_viewMatrix;	// 뷰변환 행렬
	XMFLOAT4X4				m_projMatrix;	// 투영변환 행렬
	BoundingFrustum			m_viewFrustum;	// 뷰 프러스텀

	ComPtr<ID3D12Resource>	m_cameraBuffer;
	CameraInfo*				m_cameraBufferPointer;

	XMFLOAT3				m_eye;
	XMFLOAT3				m_right;
	XMFLOAT3				m_up;
	XMFLOAT3				m_look;

	FLOAT					m_roll;			
	FLOAT					m_pitch;		
	FLOAT					m_yaw;			

	FLOAT					m_delay;

	shared_ptr<Player>		m_player;		// 플레이어
};

class ThirdPersonCamera : public Camera
{
public:
	ThirdPersonCamera();
	~ThirdPersonCamera() = default;

	void Update(FLOAT timeElapsed) override;
	void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw, FLOAT timeElapsed) override;

	XMFLOAT3 GetOffset() const { return m_offset; }
	void SetOffset(const XMFLOAT3& offset) { m_offset = offset; }
	void SetDelay(FLOAT delay) { m_delay = delay; }

	void CameraShaking(FLOAT shakingLifeTime);
	void Reset();
private:
	void UpdateCameraShaking(FLOAT timeElapsed);


private:
	XMFLOAT3	m_offset;
	FLOAT		m_delay;

	XMFLOAT3	m_shakeValue;
	FLOAT		m_shakingLifeTime = 0.5f;
	FLOAT		m_shakingTime;
	BOOL		m_isShaking;
};

class ViewingCamera : public Camera
{
public:
	ViewingCamera();
	~ViewingCamera() = default;

	void Update(FLOAT timeElapsed) override;
	void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw, FLOAT timeElapsed) override;

private:
	bool m_direction;

	float m_viewingValue;
};