#include "common.hlsl"

/*
 *  ANIMATION_SHADER
 */
struct VS_ANIMATION_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 biTangent : BITANGENT;
	float2 uv : TEXCOORD;
	int4 indices : BONEINDEX;
	float4 weights : BONEWEIGHT;
};

struct VS_ANIMATION_OUTPUT
{
	float4 position : SV_POSITION;
	float4 shadowPosition : POSITION0;
	float3 positionW : POSITION1;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 biTangent : BITANGENT;
	float2 uv : TEXCOORD;
};

VS_ANIMATION_OUTPUT VS_ANIMATION_MAIN(VS_ANIMATION_INPUT input)
{
	VS_ANIMATION_OUTPUT output;

	// 스킨 메쉬
	float4x4 mat = (float4x4)0.0f;
	if (input.weights[0] > 0) {
		// 정점이 영향을 받는 뼈마다 오프셋 * 애니메이션 변환행렬을 전부 더함
		for (int i = 0; i < 2; ++i) {
			mat += input.weights[i] * mul(boneOffsets[input.indices[i]], boneTransforms[input.indices[i]]);
		}

		output.position = mul(mul(float4(input.position, 1.0f), mat), worldMatrix);
		output.positionW = output.position.xyz;
		output.position = mul(output.position, viewMatrix);
		output.position = mul(output.position, projMatrix);
		output.shadowPosition = float4(output.positionW, 1.0f);
		output.normal = mul(mul(input.normal, (float3x3)mat), (float3x3)worldMatrix);
		output.tangent = mul(mul(input.tangent, (float3x3)mat), (float3x3)worldMatrix);
		output.biTangent = mul(mul(input.biTangent, (float3x3)mat), (float3x3)worldMatrix);
		output.uv = input.uv;
	}
	// 일반 메쉬, 0번 인덱스에 애니메이션 변환행렬이 넘어오도록 함
	else {
		mat = boneTransforms[0];

		output.position = mul(mul(float4(input.position, 1.0f), mat), worldMatrix);
		output.positionW = output.position.xyz;
		output.position = mul(output.position, viewMatrix);
		output.position = mul(output.position, projMatrix);
		output.shadowPosition = float4(output.positionW, 1.0f);
		output.normal = mul(mul(input.normal, (float3x3)mat), (float3x3)worldMatrix);
		output.tangent = mul(mul(input.tangent, (float3x3)mat), (float3x3)worldMatrix);
		output.biTangent = mul(mul(input.biTangent, (float3x3)mat), (float3x3)worldMatrix);
		output.uv = input.uv;
	}
	return output;
}

[earlydepthstencil]
float4 PS_ANIMATION_MAIN(VS_ANIMATION_OUTPUT input) : SV_TARGET
{
	PhongMaterial material;
	material.m_ambient = float4(0.1f, 0.1f, 0.1f, 1.0f);
	material.m_diffuse = float4(0.5f, 0.5f, 0.5f, 1.0f);
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
	//float shadowFactor = 0.5f;
	float4 light = Lighting(input.positionW, normal, material, shadowFactor);
	color = lerp(color, light, 0.5);
	return color;
}
