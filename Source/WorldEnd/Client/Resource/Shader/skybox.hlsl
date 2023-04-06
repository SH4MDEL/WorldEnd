#include "common.hlsl"

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
