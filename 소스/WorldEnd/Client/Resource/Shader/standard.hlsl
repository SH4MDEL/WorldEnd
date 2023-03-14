#include "common.hlsl"

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