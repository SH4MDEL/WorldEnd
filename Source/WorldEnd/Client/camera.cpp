#include "camera.h"

Camera::Camera() :
	m_eye{ 0.0f, 0.0f, 0.0f }, m_right{ 1.f, 0.f, 0.f }, m_up { 0.0f, 1.0f, 0.0f}, m_look{ 0.0f, 0.0f, 1.0f },
	m_roll{ 0.f }, m_pitch{ 0.f }, m_yaw{ 0.f }, m_delay{ 0.1f }
{
	XMStoreFloat4x4(&m_viewMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_projMatrix, XMMatrixIdentity());
}

void Camera::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	Utiles::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(CameraInfo)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_cameraBuffer)));

	// 카메라 버퍼 포인터
	m_cameraBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_cameraBufferPointer));
}

void Camera::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	XMStoreFloat4x4(&m_viewMatrix, XMMatrixLookAtLH(XMLoadFloat3(&m_eye), XMLoadFloat3(&Vector3::Add(m_eye, m_look)), XMLoadFloat3(&m_up)));

	XMFLOAT4X4 viewMatrix;
	XMStoreFloat4x4(&viewMatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_viewMatrix)));
	::memcpy(&m_cameraBufferPointer->viewMatrix, &viewMatrix, sizeof(XMFLOAT4X4));

	XMFLOAT4X4 projMatrix;
	XMStoreFloat4x4(&projMatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_projMatrix)));
	::memcpy(&m_cameraBufferPointer->projMatrix, &projMatrix, sizeof(XMFLOAT4X4));

	XMFLOAT3 eye = m_eye;
	::memcpy(&m_cameraBufferPointer->eye, &eye, sizeof(XMFLOAT3));

	D3D12_GPU_VIRTUAL_ADDRESS virtualAddress = m_cameraBuffer->GetGPUVirtualAddress();
	commandList->SetGraphicsRootConstantBufferView((INT)ShaderRegister::Camera, virtualAddress);
}

void Camera::UpdateLocalAxis()
{
	m_look = Vector3::Normalize(m_look);
	m_right = Vector3::Normalize(Vector3::Cross(m_up, m_look));
}

void Camera::Move(const XMFLOAT3& shift)
{
	m_eye = Vector3::Add(m_eye, shift);
}

void Camera::SetPlayer(const shared_ptr<Player>& player)
{
	if (m_player) m_player.reset();
	m_player = player;
	SetEye(m_player->GetPosition());
}

ThirdPersonCamera::ThirdPersonCamera() : Camera{}, m_offset{ CameraSetting::DEFAULT_OFFSET }, m_delay{ 0.1f },
m_shakeValue{ 0.f, 0.f, 0.f }, m_shakingTime{ 0.f }, m_shakingLifeTime{ 0.f }, m_isShaking {false}
{

}

void ThirdPersonCamera::Update(FLOAT timeElapsed)
{
	UpdateCameraShaking(timeElapsed);

	XMFLOAT3 offset{ Vector3::Add(m_offset, m_shakeValue) };
	XMFLOAT3 destination{ Vector3::Add(m_player->GetPosition(), offset) };
	XMFLOAT3 direction{ Vector3::Sub(destination, GetEye()) };
	XMFLOAT3 shift{ Vector3::Mul(direction, timeElapsed / m_delay) };
	SetEye(Vector3::Add(GetEye(), shift));

	// 뷰 프러스텀 업데이트

	// 동차 공간에서의 투영 절두체 꼭짓점들
	static XMVECTORF32 HomogenousPoints[6] =
	{
		{  1.0f,  0.0f, 1.0f, 1.0f },   // 오른쪽 (먼 평면의)
		{ -1.0f,  0.0f, 1.0f, 1.0f },   // 왼쪽
		{  0.0f,  1.0f, 1.0f, 1.0f },   // 위
		{  0.0f, -1.0f, 1.0f, 1.0f },   // 아래

		{ 0.0f, 0.0f, 0.0f, 1.0f },     // 가까운
		{ 0.0f, 0.0f, 1.0f, 1.0f }      // 먼
	};

	XMVECTOR Determinant;
	XMMATRIX matInverse = XMMatrixInverse(&Determinant, XMLoadFloat4x4(&m_projMatrix));

	// World Space에서의 절두체 꼭짓점들을 구한다.
	XMVECTOR Points[6];

	for (size_t i = 0; i < 6; ++i)
	{
		// 점을 변환한다.
		Points[i] = XMVector4Transform(HomogenousPoints[i], matInverse);
	}

	m_viewFrustum.Origin = XMFLOAT3{ 0.f, 0.f, 0.f };
	m_viewFrustum.Orientation = XMFLOAT4{ 0.f, 0.f, 0.f, 1.f };

	// 기울기들을 계산한다.
	Points[0] = XMVectorMultiply(Points[0], XMVectorReciprocal(XMVectorSplatZ(Points[0])));
	Points[1] = XMVectorMultiply(Points[1], XMVectorReciprocal(XMVectorSplatZ(Points[1])));
	Points[2] = XMVectorMultiply(Points[2], XMVectorReciprocal(XMVectorSplatZ(Points[2])));
	Points[3] = XMVectorMultiply(Points[3], XMVectorReciprocal(XMVectorSplatZ(Points[3])));
	// 가까운 평면 거리와 먼 평면 거리를 계산한다.
	Points[4] = XMVectorMultiply(Points[4], XMVectorReciprocal(XMVectorSplatW(Points[4])));
	Points[5] = XMVectorMultiply(Points[5], XMVectorReciprocal(XMVectorSplatW(Points[5])));

	m_viewFrustum.RightSlope = XMVectorGetX(Points[0]);		// 양의 x 기울기
	m_viewFrustum.LeftSlope = XMVectorGetX(Points[1]);		// 음의 x 기울기
	m_viewFrustum.TopSlope = XMVectorGetY(Points[2]);		// 양의 y 기울기
	m_viewFrustum.BottomSlope = XMVectorGetY(Points[3]);	// 음의 y 기울기

	m_viewFrustum.Near = XMVectorGetZ(Points[4]);			// 원평면 z
	m_viewFrustum.Far = XMVectorGetZ(Points[5]);			// 근평면 z

	m_viewFrustum.Transform(m_viewFrustum, XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_viewMatrix)));
}

void ThirdPersonCamera::UpdateCameraShaking(FLOAT timeElapsed)
{
	if (m_isShaking) {
		m_shakingTime += timeElapsed;
		if (m_shakingTime < m_shakingLifeTime) {
			int sign = Utiles::GetRandomINT(0, 1);
			if (sign) {
				m_shakeValue = Vector3::Add(m_shakeValue, Vector3::Mul(GetRight(), Utiles::GetRandomFLOAT(20.f * timeElapsed, 30.f * timeElapsed)));
			}
			else {
				m_shakeValue = Vector3::Sub(m_shakeValue, Vector3::Mul(GetRight(), Utiles::GetRandomFLOAT(20.f * timeElapsed, 30.f * timeElapsed)));
			}
			sign = Utiles::GetRandomINT(0, 1);
			if (sign) {
				m_shakeValue = Vector3::Add(m_shakeValue, Vector3::Mul(GetUp(), Utiles::GetRandomFLOAT(20.f * timeElapsed, 30.f * timeElapsed)));
			}
			else {
				m_shakeValue = Vector3::Sub(m_shakeValue, Vector3::Mul(GetUp(), Utiles::GetRandomFLOAT(20.f * timeElapsed, 30.f * timeElapsed)));
			}
		}
		else {
			m_shakingTime = 0.f;
			m_shakingLifeTime = 0.f;
			m_isShaking = false;
			m_shakeValue = { 0.f, 0.f, 0.f };
		}
	}
}

void ThirdPersonCamera::Reset()
{
	m_offset = CameraSetting::DEFAULT_OFFSET;
	m_pitch = { 0.f };
	m_yaw = { 0.f };
	cout << "reset" << endl;
}

void ThirdPersonCamera::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw, FLOAT timeElapsed)
{
	if (timeElapsed >= CameraSetting::CAMERA_WAITING_TIME) return;
	roll *= timeElapsed; pitch *= timeElapsed; yaw *= timeElapsed;

	XMMATRIX rotate{ XMMatrixIdentity() };
	if (roll != 0.f) {
		m_roll += roll;
		rotate *= XMMatrixRotationAxis(XMLoadFloat3(&m_look), XMConvertToRadians(roll));
	}
	if (pitch != 0.f) {
		if (m_pitch + pitch > CameraSetting::MAX_PITCH)
		{
			rotate *= XMMatrixRotationAxis(XMLoadFloat3(&m_right), XMConvertToRadians(CameraSetting::MAX_PITCH - m_pitch));
			m_pitch = CameraSetting::MAX_PITCH;
		}
		else if (m_pitch + pitch < CameraSetting::MIN_PITCH)
		{
			rotate *= XMMatrixRotationAxis(XMLoadFloat3(&m_right), XMConvertToRadians(CameraSetting::MIN_PITCH - m_pitch));
			m_pitch = CameraSetting::MIN_PITCH;
		}
		else {
			rotate *= XMMatrixRotationAxis(XMLoadFloat3(&m_right), XMConvertToRadians(pitch));
			m_pitch += pitch;
		}
	}
	if (yaw != 0.f) {
		m_yaw += yaw;
		rotate *= XMMatrixRotationAxis(XMLoadFloat3(&m_up), XMConvertToRadians(yaw));
	}

	if (m_offset.y < CameraSetting::DEFAULT_OFFSET.y || 
		m_offset.y > CameraSetting::DEFAULT_OFFSET.y + Vector3::Length(CameraSetting::DEFAULT_OFFSET) * sin(XMConvertToRadians(CameraSetting::MAX_PITCH) + 2.f)) Reset();

	XMStoreFloat3(&m_offset, XMVector3TransformNormal(XMLoadFloat3(&m_offset), rotate));
	XMFLOAT3 look{ Vector3::Sub(m_player->GetPosition(), m_eye) };
	if (Vector3::Length(look)) m_look = look;
}

void ThirdPersonCamera::CameraShaking(FLOAT shakingLifeTime)
{
	if (!m_isShaking) {
		m_isShaking = true;
		m_shakingLifeTime = shakingLifeTime;
	}
}

ViewingCamera::ViewingCamera() : Camera{}, m_viewingValue{0.f}
{
}

void ViewingCamera::Update(FLOAT timeElapsed)
{
	m_viewingValue += timeElapsed / 10.f;
	XMFLOAT3 destination{ 
		LoginSetting::EyeOffset * cos(m_viewingValue) + LoginSetting::Direction.x, 
		LoginSetting::Direction.y + 20.f,
		LoginSetting::EyeOffset * sin(m_viewingValue) + LoginSetting::Direction.z
	};
	SetEye(destination);
	SetLook(Vector3::Sub(LoginSetting::Direction, m_eye));

	// 뷰 프러스텀 업데이트

	// 동차 공간에서의 투영 절두체 꼭짓점들
	static XMVECTORF32 HomogenousPoints[6] =
	{
		{  1.0f,  0.0f, 1.0f, 1.0f },   // 오른쪽 (먼 평면의)
		{ -1.0f,  0.0f, 1.0f, 1.0f },   // 왼쪽
		{  0.0f,  1.0f, 1.0f, 1.0f },   // 위
		{  0.0f, -1.0f, 1.0f, 1.0f },   // 아래

		{ 0.0f, 0.0f, 0.0f, 1.0f },     // 가까운
		{ 0.0f, 0.0f, 1.0f, 1.0f }      // 먼
	};

	XMVECTOR Determinant;
	XMMATRIX matInverse = XMMatrixInverse(&Determinant, XMLoadFloat4x4(&m_projMatrix));

	// World Space에서의 절두체 꼭짓점들을 구한다.
	XMVECTOR Points[6];

	for (size_t i = 0; i < 6; ++i)
	{
		// 점을 변환한다.
		Points[i] = XMVector4Transform(HomogenousPoints[i], matInverse);
	}

	m_viewFrustum.Origin = XMFLOAT3{ 0.f, 0.f, 0.f };
	m_viewFrustum.Orientation = XMFLOAT4{ 0.f, 0.f, 0.f, 1.f };

	// 기울기들을 계산한다.
	Points[0] = XMVectorMultiply(Points[0], XMVectorReciprocal(XMVectorSplatZ(Points[0])));
	Points[1] = XMVectorMultiply(Points[1], XMVectorReciprocal(XMVectorSplatZ(Points[1])));
	Points[2] = XMVectorMultiply(Points[2], XMVectorReciprocal(XMVectorSplatZ(Points[2])));
	Points[3] = XMVectorMultiply(Points[3], XMVectorReciprocal(XMVectorSplatZ(Points[3])));
	// 가까운 평면 거리와 먼 평면 거리를 계산한다.
	Points[4] = XMVectorMultiply(Points[4], XMVectorReciprocal(XMVectorSplatW(Points[4])));
	Points[5] = XMVectorMultiply(Points[5], XMVectorReciprocal(XMVectorSplatW(Points[5])));

	m_viewFrustum.RightSlope = XMVectorGetX(Points[0]);		// 양의 x 기울기
	m_viewFrustum.LeftSlope = XMVectorGetX(Points[1]);		// 음의 x 기울기
	m_viewFrustum.TopSlope = XMVectorGetY(Points[2]);		// 양의 y 기울기
	m_viewFrustum.BottomSlope = XMVectorGetY(Points[3]);	// 음의 y 기울기

	m_viewFrustum.Near = XMVectorGetZ(Points[4]);			// 원평면 z
	m_viewFrustum.Far = XMVectorGetZ(Points[5]);			// 근평면 z

	m_viewFrustum.Transform(m_viewFrustum, XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_viewMatrix)));
}

void ViewingCamera::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw, FLOAT timeElapsed)
{

}