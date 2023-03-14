#include "common.hlsl"

/*
 *  DEBUG_SHADER
 */

struct VS_DEBUG_INPUT
{
	float3 position : POSITION;
	float2 uv : TEXCOORD;
};

struct VS_DEBUG_OUTPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

VS_DEBUG_OUTPUT VS_DEBUG_MAIN(VS_DEBUG_INPUT input)
{
	VS_DEBUG_OUTPUT output;
	output.position = float4(input.position, 1.0f);
	output.uv = input.uv;
	return output;
}

float4 PS_DEBUG_MAIN(VS_DEBUG_OUTPUT input) : SV_TARGET
{
	return g_shadowMap.Sample(g_samplerWrap, input.uv);
}