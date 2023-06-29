#include "common.hlsl"

/*
 *  TERRAIN_SHADER
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
	float4 detailTexColor = g_subTexture.Sample(g_samplerWrap, input.uv1);

	return saturate((baseTexColor * 0.5f) + (detailTexColor * 0.5f));
}