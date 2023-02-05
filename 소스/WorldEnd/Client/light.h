#pragma once
#include "stdafx.h"

#define MAX_LIGHTS 16
#define DIRECTIONAL_LIGHT 1
#define POINT_LIGHT 2
#define SPOT_LIGHT 3

struct Light
{
	XMFLOAT4				m_ambient;
	XMFLOAT4				m_diffuse;
	XMFLOAT4				m_specular;
	XMFLOAT3				m_position;
	FLOAT 					m_falloff;
	XMFLOAT3				m_direction;
	FLOAT 					m_theta;
	XMFLOAT3				m_attenuation;
	FLOAT					m_phi;
	BOOL					m_enable;
	INT						m_type;
	FLOAT					m_range;
	FLOAT					padding;
};

struct LightsInfo
{
	array<Light, MAX_LIGHTS>	m_lights;
	XMFLOAT4					m_globalAmbient;
	UINT						m_numLight;
};

struct LightSystem
{
	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;

	ComPtr<ID3D12Resource>			m_lightBuffer;
	LightsInfo*						m_lightBufferPointer;

	array<Light, MAX_LIGHTS>	m_lights;
	XMFLOAT4					m_globalAmbient;
	UINT						m_numLight;
};