#include "common.hlsl"

/*
 *  DEBUG_SHADER
 */

struct VS_DEBUG_INPUT
{
	float3 position : POSITION;
	float2 size : SIZE;
};

struct VS_DEBUG_OUTPUT
{
	float4 position : POSITION;
	float2 size : SIZE;
};

struct GS_DEBUG_OUTPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

VS_DEBUG_OUTPUT VS_DEBUG_MAIN(VS_DEBUG_INPUT input)
{
	VS_DEBUG_OUTPUT output;
	output.position = float4(input.position, 1.0f);
	output.size = input.size;
	return output;
}

[maxvertexcount(4)]
void GS_DEBUG_MAIN(point VS_DEBUG_OUTPUT input[1], inout TriangleStream<GS_DEBUG_OUTPUT> outStream)
{
	float4 vertices[4];
	vertices[0] = input[0].position;
	vertices[0].x -= input[0].size.x; vertices[0].y -= input[0].size.y;
	vertices[1] = input[0].position;
	vertices[1].x -= input[0].size.x; vertices[1].y += input[0].size.y;
	vertices[2] = input[0].position;
	vertices[2].x += input[0].size.x; vertices[2].y -= input[0].size.y;
	vertices[3] = input[0].position;
	vertices[3].x += input[0].size.x; vertices[3].y += input[0].size.y;

	float2 uv[4] = { float2(0.0f, 1.0f), float2(0.0f, 0.0f), float2(1.0f, 1.0f), float2(1.0f, 0.0f) };

	GS_DEBUG_OUTPUT output;
	for (int i = 0; i < 4; ++i) {
		output.position = vertices[i];
		output.uv = uv[i];
		outStream.Append(output);
	}
}

float4 PS_DEBUG_MAIN(GS_DEBUG_OUTPUT input) : SV_TARGET
{
	return g_shadowMap.Sample(g_samplerWrap, float3(input.uv, 0));
}