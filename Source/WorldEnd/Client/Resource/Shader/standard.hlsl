#include "common.hlsl"

/*
 *  STANDARD_SHADER
 */
struct VS_STANDARD_INPUT
{
	float3 position : POSITION;
};

struct VS_STANDARD_OUTPUT
{
	float4 position : SV_POSITION;
};

VS_STANDARD_OUTPUT VS_STANDARD_MAIN(VS_STANDARD_INPUT input)
{
	VS_STANDARD_OUTPUT output;
	output.position = mul(float4(input.position, 1.0f), worldMatrix);
	output.position = mul(output.position, viewMatrix);
	output.position = mul(output.position, projMatrix);
	return output;
}

[earlydepthstencil]
float4 PS_STANDARD_MAIN(VS_STANDARD_OUTPUT input) : SV_TARGET
{
	return float4(0.f, 0.f, 0.f, 1.f);
}