cbuffer cbGameObject : register(b0)
{
	matrix worldMatrix : packoffset(c0);
    float g_gauge : packoffset(c4.x);
    float g_maxGauge : packoffset(c4.y);
    float g_age : packoffset(c4.z);
    int g_type : packoffset(c4.w);
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
    float2 padding : packoffset(c5.z);
};

#define MAX_BONES 100
cbuffer cbBoneOffsets : register(b3)
{
    float4x4 boneOffsets[MAX_BONES];
};

cbuffer cbBoneTransforms : register(b4)
{
    float4x4 boneTransforms[MAX_BONES];
};

#include "lighting.hlsl"

#define CASCADES_NUM 3
cbuffer cbScene : register(b6)
{
    matrix lightView[CASCADES_NUM];
    matrix lightProj[CASCADES_NUM];
    matrix NDCspace;
}

cbuffer cbFramework : register(b7)
{
    float timeElapsed : packoffset(c0);
}

SamplerState g_samplerWrap : register(s0);
SamplerComparisonState g_samplerShadow : register(s1);

Texture2D g_baseTexture : register(t0);
Texture2D g_subTexture : register(t1);
TextureCube g_skyboxTexture : register(t2);
Texture2DArray g_shadowMap : register(t3);

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

bool IsTextureRange(float4 position)
{
    return (position.x >= 0.f && position.x <= 1.f & position.y >= 0.f && position.y <= 1.f);
}

#define CASCADEMAP_BIAS 0.003
float CalcShadowFactor(float4 shadowPosH)
{
    uint width, height, elements, numMips;
    g_shadowMap.GetDimensions(0, width, height, elements, numMips);

    for (int cascade = 0; cascade < CASCADES_NUM; ++cascade) {
        float4 shadowPos = mul(shadowPosH, lightView[cascade]);
        shadowPos = mul(shadowPos, lightProj[cascade]);

        // Complete projection by doing division by w.
        shadowPos.xyz /= shadowPos.w;

        shadowPos = mul(shadowPos, NDCspace);

        if (!IsTextureRange(shadowPos)) continue;

        // Depth in NDC space.
        float depth = shadowPos.z - CASCADEMAP_BIAS;
        if (depth < 0.f || depth > 1.f) continue;

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
                float3(shadowPos.xy + offsets[i], cascade), depth).r;
        }
   
        return percentLit / 9.0f;
    }
    return 1.f;
}
