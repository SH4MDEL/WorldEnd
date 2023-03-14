#include "common.hlsl"

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