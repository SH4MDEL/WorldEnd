#include "camera.h"

Camera::Camera() :
	m_eye{ 0.0f, 0.0f, 0.0f }, m_right{ 1.f, 0.f, 0.f }, m_up { 0.0f, 1.0f, 0.0f}, m_look{ 0.0f, 0.0f, 1.0f },
	m_roll{ 0.f }, m_pitch{ 0.f }, m_yaw{ 0.f }, m_delay{ 0.0f }
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

ThirdPersonCamera::ThirdPersonCamera() : Camera{}, m_offset{ 0.0f, 1.0f, -8.0f }, m_delay{ 0.1f }
{

}

void ThirdPersonCamera::Update(FLOAT timeElapsed)
{
	XMFLOAT3 destination{ Vector3::Add(m_player->GetPosition(), m_offset) };
	XMFLOAT3 direction{ Vector3::Sub(destination, GetEye()) };
	XMFLOAT3 shift{ Vector3::Mul(direction, timeElapsed * 1 / m_delay) };
	SetEye(Vector3::Add(GetEye(), shift));
}

void ThirdPersonCamera::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw)
{
	XMMATRIX rotate{ XMMatrixIdentity() };
	if (roll != 0.0f)
	{
		m_roll += roll;
		rotate *= XMMatrixRotationAxis(XMLoadFloat3(&m_look), XMConvertToRadians(roll));
	}
	if (pitch != 0.0f)
	{
		if (m_pitch + pitch > MAX_PITCH)
		{
			rotate *= XMMatrixRotationAxis(XMLoadFloat3(&m_right), XMConvertToRadians(MAX_PITCH - m_pitch));
			m_pitch = MAX_PITCH;
		}
		else if (m_pitch + pitch < MIN_PITCH)
		{
			rotate *= XMMatrixRotationAxis(XMLoadFloat3(&m_right), XMConvertToRadians(MIN_PITCH - m_pitch));
			m_pitch = MIN_PITCH;
		}
		else {
			rotate *= XMMatrixRotationAxis(XMLoadFloat3(&m_right), XMConvertToRadians(pitch));
			m_pitch += pitch;
		}
	}
	if (yaw != 0.0f)
	{
		m_yaw += yaw;
		rotate *= XMMatrixRotationAxis(XMLoadFloat3(&m_up), XMConvertToRadians(yaw));
	}
	XMStoreFloat3(&m_offset, XMVector3TransformNormal(XMLoadFloat3(&m_offset), rotate));
	XMFLOAT3 look{ Vector3::Sub(m_player->GetPosition(), m_eye) };
	if (Vector3::Length(look)) m_look = look;
}