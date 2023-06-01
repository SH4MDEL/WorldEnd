#include "common.hlsl"

/*
 * STATICOBJECT_SHADER
 */
struct VS_STATICOBJECT_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 biTangent : BITANGENT;
	float2 uv : TEXCOORD;
};

struct VS_STATICOBJECT_OUTPUT
{
	float4 position : SV_POSITION;
	float4 shadowPosition : POSITION0;
	float3 positionW : POSITION1;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 biTangent : BITANGENT;
	float2 uv : TEXCOORD;
};

VS_STATICOBJECT_OUTPUT VS_STATICOBJECT_MAIN(VS_STATICOBJECT_INPUT input)
{
	VS_STATICOBJECT_OUTPUT output;
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
float4 PS_STATICOBJECT_MAIN(VS_STATICOBJECT_OUTPUT input) : SV_TARGET
{
	PhongMaterial material;
	material.m_ambient = float4(0.1f, 0.1f, 0.1f, 1.0f);
	material.m_diffuse = albedoColor;
	material.m_specular = float4(0.1f, 0.1f, 0.1f, 0.0f);

	float4 normalColor = float4(0.0f, 0.0f, 0.0f, 1.0f);		// ³ë¸»
	float4 metallicColor = float4(0.0f, 0.0f, 0.0f, 0.0f);		// 
	float4 emissionColor = float4(0.0f, 0.0f, 0.0f, 0.0f);		// ¹ß»ê±¤

	if (textureMask & MATERIAL_ALBEDO_MAP) material.m_diffuse *= g_albedoTexture.Sample(g_samplerWrap, input.uv);
	if (textureMask & MATERIAL_SPECULAR_MAP) material.m_specular = g_specularTexture.Sample(g_samplerWrap, input.uv);
	if (textureMask & MATERIAL_NORMAL_MAP) normalColor = g_normalTexture.Sample(g_samplerWrap, input.uv);
	else normalColor = float4(input.normal, 1.f);
	if (textureMask & MATERIAL_METALLIC_MAP) metallicColor = g_metallicTexture.Sample(g_samplerWrap, input.uv);
	if (textureMask & MATERIAL_EMISSION_MAP) emissionColor = g_emissionTexture.Sample(g_samplerWrap, input.uv);

	float3 normal = normalColor.rgb;
	float4 color = material.m_diffuse + material.m_specular + emissionColor;
	float3x3 TBN = float3x3(normalize(input.tangent), normalize(input.biTangent), normalize(normal));
	float3 vNormal = normalize(normal * 2.0f - 1.0f); //[0, 1] ¡æ [-1, 1]
	normal = normalize(mul(vNormal, TBN));
	float shadowFactor = CalcShadowFactor(input.shadowPosition);
	//shadowFactor = 1.0f;
	float4 light = Lighting(input.positionW, normal, material, shadowFactor);
	color = lerp(color, light, 0.5);
	return color;
}