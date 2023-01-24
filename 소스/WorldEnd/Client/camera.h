#pragma once
#include "stdafx.h"
#include "player.h"

// ī�޶� Ŭ������ �� ������ �߿��� ������ �����Ѵ�
// 1. ī�޶� ��ǥ�踦 �����ϴ� �Ӽ�
// 2. ��� ����ü�� �����ϴ� �Ӽ�

#define MAX_PITCH +20
#define MIN_PITCH -10

struct CameraInfo
{
	XMFLOAT4X4			viewMatrix;	// �亯ȯ ���
	XMFLOAT4X4			projMatrix;	// ������ȯ ���
};

//class Camera
//{
//public:
//
//	Camera();
//	~Camera();
//
//	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
//	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList);
//
//	virtual void Update(FLOAT timeElapsed) = 0;
//	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw) = 0;
//	// Set frustum.
//	void SetLens(float fovY, float aspect, float zn, float zf);
//
//	// Define camera space via LookAt parameters.
//	void LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up);
//
//	XMFLOAT4X4 GetView() const { return m_view; }
//	XMFLOAT4X4 GetProj() const { return m_proj; }
//	void SetView(const XMFLOAT4X4& view) { m_view = view; }
//	void SetProj(const XMFLOAT4X4& proj) { m_proj = proj; }
//
//	DirectX::XMFLOAT3 GetEye() const;
//	DirectX::XMFLOAT3 GetRight() const;
//	DirectX::XMFLOAT3 GetUp() const;
//	DirectX::XMFLOAT3 GetLook() const;
//	void SetEye(const XMFLOAT3& eye);
//
//	float GetNearZ() const;
//	float GetFarZ() const;
//	float GetAspect() const;
//	float GetFovY() const;
//	float GetFovX() const;
//
//	float GetNearWindowWidth() const;
//	float GetNearWindowHeight() const;
//	float GetFarWindowWidth() const;
//	float GetFarWindowHeight() const;
//
//
//	// Strafe/Walk the camera a distance d.
//	void Strafe(float d);
//	void Walk(float d);
//
//	// Rotate the camera.
//	void SetPitch(float angle);
//	void SetYaw(float angle);
//
//	// After modifying camera position/orientation, call to rebuild the view matrix.
//	void UpdateViewMatrix();
//
//protected:
//	XMFLOAT4X4 m_view;
//	XMFLOAT4X4 m_proj;
//
//	ComPtr<ID3D12Resource>	m_cameraBuffer;
//	CameraInfo* m_cameraBufferPointer;
//
//	XMFLOAT3 m_eye;
//	XMFLOAT3 m_right;
//	XMFLOAT3 m_up;
//	XMFLOAT3 m_look;
//
//	FLOAT m_nearZ;
//	FLOAT m_farZ;
//	FLOAT m_aspect;
//	FLOAT m_fovY;
//	FLOAT m_nearWindowHeight;
//	FLOAT m_farWindowHeight;
//};
//
//class ThirdPersonCamera : public Camera
//{
//public:
//	ThirdPersonCamera();
//	~ThirdPersonCamera() = default;
//
//	void Update(FLOAT timeElapsed) override;
//	void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw) override;
//
//	XMFLOAT3 GetOffset() const { return m_offset; }
//	void SetOffset(const XMFLOAT3& offset) { m_offset = offset; }
//	void SetDelay(FLOAT delay) { m_delay = delay; }
//
//private:
//	XMFLOAT3	m_offset;
//	FLOAT		m_delay;
//};


class Camera
{
public:
	Camera();
	~Camera() = default;

	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void UpdateLocalAxis();
	virtual void Update(FLOAT timeElapsed) = 0;

	void Move(const XMFLOAT3& shift);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);

	XMFLOAT4X4 GetViewMatrix() const { return m_viewMatrix; }
	XMFLOAT4X4 GetProjMatrix() const { return m_projMatrix; }
	void SetViewMatrix(const XMFLOAT4X4& viewMatrix) { m_viewMatrix = viewMatrix; }
	void SetProjMatrix(const XMFLOAT4X4& projMatrix) { m_projMatrix = projMatrix; }

	XMFLOAT3 GetEye() const { return m_eye; }
	XMFLOAT3 GetAt() const { return m_look; }
	XMFLOAT3 GetUp() const { return m_up; }
	void SetEye(const XMFLOAT3& eye) { m_eye = eye; UpdateLocalAxis(); }
	void SetAt(const XMFLOAT3& at) { m_look = at; UpdateLocalAxis(); }
	void SetUp(const XMFLOAT3& up) { m_up = up; UpdateLocalAxis(); }

	XMFLOAT3 GetU() const { return m_u; }
	XMFLOAT3 GetV() const { return m_v; }
	XMFLOAT3 GetN() const { return m_n; }

	shared_ptr<Player> GetPlayer() const { return m_player; }
	void SetPlayer(const shared_ptr<Player>& player);

protected:
	XMFLOAT4X4				m_viewMatrix;	// �亯ȯ ���
	XMFLOAT4X4				m_projMatrix;	// ������ȯ ���

	ComPtr<ID3D12Resource>	m_cameraBuffer;
	CameraInfo*				m_cameraBufferPointer;

	XMFLOAT3				m_eye;			// ī�޶� ��ġ
	XMFLOAT3				m_look;			// ī�޶� �ٶ󺸴� ����
	XMFLOAT3				m_up;			// ī�޶� Up����

	XMFLOAT3				m_u;			// ���� x��
	XMFLOAT3				m_v;			// ���� y��
	XMFLOAT3				m_n;			// ���� z��

	FLOAT					m_roll;			// x�� ȸ����
	FLOAT					m_pitch;		// y�� ȸ����
	FLOAT					m_yaw;			// z�� ȸ����

	FLOAT					m_delay;		// ������ ������ (0.0 ~ 1.0)

	shared_ptr<Player>		m_player;		// �÷��̾�
};

class ThirdPersonCamera : public Camera
{
public:
	ThirdPersonCamera();
	~ThirdPersonCamera() = default;

	virtual void Update(FLOAT timeElapsed);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);

	XMFLOAT3 GetOffset() const { return m_offset; }
	void SetOffset(const XMFLOAT3& offset) { m_offset = offset; }
	void SetDelay(FLOAT delay) { m_delay = delay; }

private:
	XMFLOAT3	m_offset;
	FLOAT		m_delay;
};