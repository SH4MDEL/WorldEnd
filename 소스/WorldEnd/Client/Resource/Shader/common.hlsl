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

#define MAX_BONES					100

// bone
cbuffer cbBoneOffsets : register(b5)
{
    float4x4 boneOffsets[MAX_BONES];
};

cbuffer cbBoneTransforms : register(b6)
{
    float4x4 boneTransforms[MAX_BONES];
};
// ------------------------------

#include "lighting.hlsl"


cbuffer cbScene : register(b4)
{
    matrix lightView : packoffset(c0);
    matrix lightProj : packoffset(c4);
    matrix NDCspace : packoffset(c8);
}

SamplerState g_samplerWrap : register(s0);
SamplerComparisonState g_samplerShadow : register(s1);

Texture2D g_baseTexture : register(t0);
Texture2D g_subTexture : register(t1);
TextureCube g_skyboxTexture : register(t2);
Texture2D g_shadowMap : register(t3);

Texture2D g_albedoTexture : register(t4);
Texture2D g_specularTexture : register(t5);
Texture2D g_normalTexture : register(t6);
Texture2D g_metallicTexture : register(t7);
Texture2D g_emissionTexture : register(t8);
Texture2D g_detailAlbedoTexture : register(t9);
Texture2D g_detailNormalTexture : register(t10);

#define MATERIAL_ALBEDO_MAP			0x01
#define MATERIAL_SPECULAR_MAP		0x02
#define MATERIAL_NORMAL_MAP			0x04
#define MATERIAL_METALLIC_MAP		0x08
#define MATERIAL_EMISSION_MAP		0x10
#define MATERIAL_DETAIL_ALBEDO_MAP	0x20
#define MATERIAL_DETAIL_NORMAL_MAP	0x40


float CalcShadowFactor(float4 shadowPosH)
{
    // Complete projection by doing division by w.
    shadowPosH.xyz /= shadowPosH.w;

    // Depth in NDC space.
    float depth = shadowPosH.z;

    uint width, height, numMips;
    g_shadowMap.GetDimensions(0, width, height, numMips);

    // Texel size.
    float dx = 1.0f / (float)width;

    float percentLit = 0.f;
    const float2 offsets[9] =
    {
        float2(-dx,  -dx), float2(0.0f,  -dx), float2(dx,  -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx,  +dx), float2(0.0f,  +dx), float2(dx,  +dx)
    };

    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        percentLit += g_shadowMap.SampleCmpLevelZero(g_samplerShadow,
            shadowPosH.xy + offsets[i], depth).r;
    }

    return percentLit / 9.0f;
}
