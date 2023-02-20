#include "Buffer.hlsl"

/*
 *  STANDARD_SHADER
 */
struct VS_STANDARD_INPUT
{
	float3 position : POSITION;
	float2 uv : TEXCOORD;
};

struct VS_STANDARD_OUTPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

VS_STANDARD_OUTPUT VS_STANDARD_MAIN(VS_STANDARD_INPUT input)
{
	VS_STANDARD_OUTPUT output;
	output.position = mul(float4(input.position, 1.0f), worldMatrix);
	output.position = mul(output.position, viewMatrix);
	output.position = mul(output.position, projMatrix);
	output.uv = input.uv;
	return output;
}

[earlydepthstencil]
float4 PS_STANDARD_MAIN(VS_STANDARD_OUTPUT input) : SV_TARGET
{
	return g_baseTexture.Sample(g_samplerWrap, input.uv);
}

/*
 *  INSTANCE_SHADER
 */
struct VS_INSTANCE_INPUT
{
	float3 position : POSITION;
	float4 color : COLOR;
	matrix worldMatrix : INSTANCE;
};

struct VS_INSTANCE_OUTPUT
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

VS_INSTANCE_OUTPUT VS_INSTANCE_MAIN(VS_INSTANCE_INPUT input)
{
	VS_INSTANCE_OUTPUT output;
	output.position = mul(float4(input.position, 1.0f), input.worldMatrix);
	output.position = mul(output.position, viewMatrix);
	output.position = mul(output.position, projMatrix);
	output.color = input.color;
	return output;
}

[earlydepthstencil]
float4 PS_INSTANCE_MAIN(VS_INSTANCE_OUTPUT input) : SV_TARGET
{
	return input.color;
}

/*
 * TEXTUREHIERARCHY_SHADER
 */
struct VS_TEXTUREHIERARCHY_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 biTangent : BITANGENT;
	float2 uv : TEXCOORD;
};

struct VS_TEXTUREHIERARCHY_OUTPUT
{
	float4 position : SV_POSITION;
	float4 shadowPosition : POSITION0;
	float3 positionW : POSITION1;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 biTangent : BITANGENT;
	float2 uv : TEXCOORD;
};

VS_TEXTUREHIERARCHY_OUTPUT VS_TEXTUREHIERARCHY_MAIN(VS_TEXTUREHIERARCHY_INPUT input)
{
	VS_TEXTUREHIERARCHY_OUTPUT output;
	output.position = mul(float4(input.position, 1.0f), worldMatrix);
	output.positionW = output.position.xyz;
	output.position = mul(output.position, viewMatrix);
	output.position = mul(output.position, projMatrix);
	output.shadowPosition = mul(float4(output.positionW, 1.0f), lightView);
	output.shadowPosition = mul(output.shadowPosition, lightProj);
	output.shadowPosition = mul(output.shadowPosition, NDCspace);
	output.normal = mul(input.normal, (float3x3)worldMatrix);
	output.tangent = mul(input.tangent, (float3x3)worldMatrix);
	output.biTangent = mul(input.biTangent, (float3x3)worldMatrix);
	output.uv = input.uv;
	return output;
}

[earlydepthstencil]
float4 PS_TEXTUREHIERARCHY_MAIN(VS_TEXTUREHIERARCHY_OUTPUT input) : SV_TARGET
{
	PhongMaterial material;
	material.m_ambient = float4(0.1f, 0.1f, 0.1f, 1.0f);
	material.m_diffuse = float4(1.0f, 1.0f, 1.0f, 1.0f);
	material.m_specular = float4(0.1f, 0.1f, 0.1f, 0.0f);

	float4 normalColor = float4(0.0f, 0.0f, 0.0f, 1.0f);		// 노말
	float4 metallicColor = float4(0.0f, 0.0f, 0.0f, 0.0f);		// 
	float4 emissionColor = float4(0.0f, 0.0f, 0.0f, 0.0f);		// 발산광

	if (textureMask & MATERIAL_ALBEDO_MAP) material.m_diffuse = g_albedoTexture.Sample(g_samplerWrap, input.uv);
	if (textureMask & MATERIAL_SPECULAR_MAP) material.m_specular = g_specularTexture.Sample(g_samplerWrap, input.uv);
	if (textureMask & MATERIAL_NORMAL_MAP) normalColor = g_normalTexture.Sample(g_samplerWrap, input.uv);
	else normalColor = float4(input.normal, 1.f);
	if (textureMask & MATERIAL_METALLIC_MAP) metallicColor = g_metallicTexture.Sample(g_samplerWrap, input.uv);
	if (textureMask & MATERIAL_EMISSION_MAP) emissionColor = g_emissionTexture.Sample(g_samplerWrap, input.uv);

	float3 normal = normalColor.rgb;
	float4 color = material.m_diffuse + material.m_specular + emissionColor;
	//float3x3 TBN = float3x3(normalize(input.tangent), normalize(input.biTangent), normalize(normal));
	//float3 vNormal = normalize(normal * 2.0f - 1.0f); //[0, 1] → [-1, 1]
	//normal = normalize(mul(vNormal, TBN));
	float shadowFactor = CalcShadowFactor(input.shadowPosition);
	float4 light = Lighting(input.positionW, normal, material, shadowFactor);
	color = lerp(color, light, 0.5);
	return color;
}

/*
 *  SKIN_ANIMATION_SHADER
 */
struct VS_SKINNED_STANDARD_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 biTangent : BITANGENT;
	float2 uv : TEXCOORD;
	int4 indices : BONEINDEX;
	float4 weights : BONEWEIGHT;
};

VS_TEXTUREHIERARCHY_OUTPUT VS_SKINNED_ANIMATION_MAIN(VS_SKINNED_STANDARD_INPUT input)
{
	VS_TEXTUREHIERARCHY_OUTPUT output;

	// 스킨 메쉬
	if (input.weights[0] > 0) {
		// 정점이 영향을 받는 뼈마다 오프셋 * 애니메이션 변환행렬을 전부 더합
		float4x4 mat = (float4x4)0.0f;
		for (int i = 0; i < 2; ++i) {
			mat += input.weights[i] * mul(boneOffsets[input.indices[i]], boneTransforms[input.indices[i]]);
		}

		output.position = mul(float4(input.position, 1.0f), mat);
		output.positionW = output.position.xyz;
		output.position = mul(output.position, viewMatrix);
		output.position = mul(output.position, projMatrix);
		output.shadowPosition = mul(float4(output.positionW, 1.0f), lightView);
		output.shadowPosition = mul(output.shadowPosition, lightProj);
		output.shadowPosition = mul(output.shadowPosition, NDCspace);
		output.normal = mul(input.normal, (float3x3)mat);
		output.tangent = mul(input.tangent, (float3x3)mat);
		output.biTangent = mul(input.biTangent, (float3x3)mat);
		output.uv = input.uv;
	}
	// 일반 메쉬
	else {
		output.position = mul(float4(input.position, 1.0f), worldMatrix);
		output.positionW = output.position.xyz;
		output.position = mul(output.position, viewMatrix);
		output.position = mul(output.position, projMatrix);
		output.shadowPosition = mul(float4(output.positionW, 1.0f), lightView);
		output.shadowPosition = mul(output.shadowPosition, lightProj);
		output.shadowPosition = mul(output.shadowPosition, NDCspace);
		output.normal = mul(input.normal, (float3x3)worldMatrix);
		output.tangent = mul(input.tangent, (float3x3)worldMatrix);
		output.biTangent = mul(input.biTangent, (float3x3)worldMatrix);
		output.uv = input.uv;
	}

	return output;
}

/*
 *  DETAIL_SHADER
 */

	struct VS_TERRAIN_INPUT
{
	float3 position : POSITION;
	float2 uv0 : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
};

struct VS_TERRAIN_OUTPUT
{
	float4 position : SV_POSITION;
	float2 uv0 : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
};

VS_TERRAIN_OUTPUT VS_TERRAIN_MAIN(VS_TERRAIN_INPUT input)
{
	VS_TERRAIN_OUTPUT output;

	output.position = mul(float4(input.position, 1.0f), worldMatrix);
	output.position = mul(output.position, viewMatrix);
	output.position = mul(output.position, projMatrix);
	output.uv0 = input.uv0;
	output.uv1 = input.uv1;
	return output;
}

[earlydepthstencil]
float4 PS_TERRAIN_MAIN(VS_TERRAIN_OUTPUT input) : SV_TARGET
{
	float4 baseTexColor = g_baseTexture.Sample(g_samplerWrap, input.uv0);
	float4 subTexColor = g_subTexture.Sample(g_samplerWrap, input.uv1);

	float4 color = saturate((baseTexColor * 0.7f) + (subTexColor * 0.3f));

	return color;
}

/*
 *  SKYBOX_SHADER
 */
struct VS_SKYBOX_INPUT
{
	float3 position : POSITION;
};

struct VS_SKYBOX_OUTPUT
{
	float3	positionL : POSITION;
	float4	position : SV_POSITION;
};

VS_SKYBOX_OUTPUT VS_SKYBOX_MAIN(VS_SKYBOX_INPUT input)
{
	VS_SKYBOX_OUTPUT output;

	output.position = mul(float4(input.position, 1.0f), worldMatrix);
	output.position = mul(output.position, viewMatrix);
	output.position = mul(output.position, projMatrix).xyww;
	output.positionL = input.position;

	return output;
}

[earlydepthstencil]
float4 PS_SKYBOX_MAIN(VS_SKYBOX_OUTPUT input) : SV_TARGET
{
	float4 color = g_skyboxTexture.Sample(g_samplerWrap, input.positionL);
	return color;
}

/*
 *  BLENDING_SHADER
 */

struct VS_BLENDING_INPUT
{
	float3 position : POSITION;
	float2 uv : TEXCOORD;
};

struct VS_BLENDING_OUTPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

VS_BLENDING_OUTPUT VS_BLENDING_MAIN(VS_BLENDING_INPUT input)
{
	VS_BLENDING_OUTPUT output;

	output.position = mul(float4(input.position, 1.0f), worldMatrix);
	output.position = mul(output.position, viewMatrix);
	output.position = mul(output.position, projMatrix);
	output.uv = input.uv;

	return output;
}

float4 PS_BLENDING_MAIN(VS_BLENDING_OUTPUT input) : SV_TARGET
{
	return g_baseTexture.Sample(g_samplerWrap, input.uv);
}

/*
 *  HPBAR_SHADER
 */

struct VS_HPBAR_INPUT
{
	float3 position : POSITION;
	float2 size : SIZE;
};

struct VS_HPBAR_OUTPUT
{
	float4 position : POSITION;
	float2 size : SIZE;
};

struct GS_HPBAR_OUTPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 uv0 : TEXCOORD;
	float2 uv1 : TEXCOORD;
};

VS_HPBAR_OUTPUT VS_HPBAR_MAIN(VS_HPBAR_INPUT input)
{
	VS_HPBAR_OUTPUT output;

	output.position = mul(float4(input.position, 1.0f), worldMatrix);
	output.size = input.size;
	return output;
}

[maxvertexcount(4)]
void GS_HPBAR_MAIN(point VS_HPBAR_OUTPUT input[1], uint primID : SV_PrimitiveID, inout TriangleStream<GS_HPBAR_OUTPUT> outStream)
{
	float3 up = float3(0.0f, 1.0f, 0.0f);
	float3 look = cameraPosition - input[0].position.xyz;
	look = normalize(look);
	float3 right = cross(up, look);
	float halfW = input[0].size.x * 0.5f;
	float halfH = input[0].size.y * 0.5f;
	float4 vertices[4];
	vertices[0] = float4(input[0].position.xyz + halfW * right - halfH * up, 1.0f);
	vertices[1] = float4(input[0].position.xyz + halfW * right + halfH * up, 1.0f);
	vertices[2] = float4(input[0].position.xyz - halfW * right - halfH * up, 1.0f);
	vertices[3] = float4(input[0].position.xyz - halfW * right + halfH * up, 1.0f);
	float2 uv[4] = { float2(0.0f, 1.0f), float2(0.0f, 0.0f), float2(1.0f, 1.0f), float2(1.0f, 0.0f) };

	GS_HPBAR_OUTPUT output;
	for (int i = 0; i < 4; ++i) {
		output.position = mul(vertices[i], viewMatrix);
		output.position = mul(output.position, projMatrix);
		output.normal = look;
		output.uv0 = uv[i];
		output.uv1 = uv[i];
		outStream.Append(output);
	}
}

[earlydepthstencil]
float4 PS_HPBAR_MAIN(GS_HPBAR_OUTPUT input) : SV_TARGET
{
	// 이미지의 16% 구간부터 체력바 시작
	if (input.uv0.x <= 0.16f + (0.84f) * (hp / maxHp)) {
		return g_baseTexture.Sample(g_samplerWrap, input.uv0);
	}
	return g_subTexture.Sample(g_samplerWrap, input.uv1);
}

/*
 *  UI_SHADER
 */

struct VS_UI_INPUT
{
	float3 position : POSITION;
	float2 uv : TEXCOORD;
};

struct VS_UI_OUTPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

VS_UI_OUTPUT VS_UI_MAIN(VS_UI_INPUT input)
{
	VS_UI_OUTPUT output;
	output.position = float4(input.position, 1.0f);
	output.uv = input.uv;
	return output;
}

float4 PS_UI_MAIN(VS_UI_OUTPUT input) : SV_TARGET
{
	return g_shadowMap.Sample(g_samplerWrap, input.uv);
}