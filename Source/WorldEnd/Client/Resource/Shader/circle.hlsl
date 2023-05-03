#include "common.hlsl"

/*
 *  CIRCLE_SHADER
 */

struct VS_CIRCLE_INPUT
{
	float3 position : POSITION;
	float2 size : SIZE;
};

struct VS_CIRCLE_OUTPUT
{
	float4 position : POSITION;
	float2 size : SIZE;
};

struct GS_CIRCLE_OUTPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

VS_CIRCLE_OUTPUT VS_CIRCLE_MAIN(VS_CIRCLE_INPUT input)
{
	VS_CIRCLE_OUTPUT output;

	output.position = mul(float4(input.position, 1.0f), worldMatrix);
	output.size = input.size;
	return output;
}

[maxvertexcount(4)]
void GS_CIRCLE_MAIN(point VS_CIRCLE_OUTPUT input[1], inout TriangleStream<GS_CIRCLE_OUTPUT> outStream)
{
	float3 up = float3(0.0f, 1.0f, 0.0f);

	float halfX = input[0].size.x * 0.5f;
	float halfZ = input[0].size.y * 0.5f;
	float4 vertices[4];
	vertices[0] = float4(input[0].position.x - halfX, input[0].position.y, input[0].position.z - halfZ, 1.0f);
	vertices[1] = float4(input[0].position.x - halfX, input[0].position.y, input[0].position.z + halfZ, 1.0f);
	vertices[2] = float4(input[0].position.x + halfX, input[0].position.y, input[0].position.z - halfZ, 1.0f);
	vertices[3] = float4(input[0].position.x + halfX, input[0].position.y, input[0].position.z + halfZ, 1.0f);

	float2 uv[4] = { float2(0.0f, 1.0f), float2(0.0f, 0.0f), float2(1.0f, 1.0f), float2(1.0f, 0.0f) };

	GS_CIRCLE_OUTPUT output;
	for (int i = 0; i < 4; ++i) {
		output.position = mul(vertices[i], viewMatrix);
		output.position = mul(output.position, projMatrix);
		output.uv = uv[i];
		outStream.Append(output);
	}
}

[earlydepthstencil]
float4 PS_CIRCLE_MAIN(GS_CIRCLE_OUTPUT input) : SV_TARGET
{
	return g_baseTexture.Sample(g_samplerWrap, input.uv);
}
