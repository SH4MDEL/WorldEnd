cbuffer cbGameObject : register(b0)
{
	matrix worldMatrix : packoffset(c0);
	float hp : packoffset(c4.x);
	float maxHp : packoffset(c4.y);
};

cbuffer cbCamera : register(b1)
{
	matrix viewMatrix : packoffset(c0);
	matrix projMatrix : packoffset(c4);
	float3 cameraPosition : packoffset(c8);
};

cbuffer cbMaterial : register(b2)
{
	float4 albedoColor : packoffset(c0);
	float4 emissiveColor : packoffset(c1);
	float4 specularColor : packoffset(c2);
	float4 ambientColor : packoffset(c3);
	float glossiness : packoffset(c4.x);
	float smoothness : packoffset(c4.y);
	float metallic : packoffset(c4.z);
	float specularHighlight : packoffset(c4.w);
	float glossyReflection : packoffset(c5.x);
	uint textureMask : packoffset(c5.y);
};

#include "Lighting.hlsl"

SamplerState g_samplerWrap : register(s0);
SamplerState g_samplerClamp : register(s1);

Texture2D g_baseTexture : register(t0);
Texture2D g_subTexture : register(t1);
TextureCube g_skyboxTexture : register(t2);

Texture2D g_albedoTexture : register(t3);
Texture2D g_specularTexture : register(t4);
Texture2D g_normalTexture : register(t5);
Texture2D g_metallicTexture : register(t6);
Texture2D g_emissionTexture : register(t7);
Texture2D g_detailAlbedoTexture : register(t8);
Texture2D g_detailNormalTexture : register(t9);

#define MATERIAL_ALBEDO_MAP			0x01
#define MATERIAL_SPECULAR_MAP		0x02
#define MATERIAL_NORMAL_MAP			0x04
#define MATERIAL_METALLIC_MAP		0x08
#define MATERIAL_EMISSION_MAP		0x10
#define MATERIAL_DETAIL_ALBEDO_MAP	0x20
#define MATERIAL_DETAIL_NORMAL_MAP	0x40