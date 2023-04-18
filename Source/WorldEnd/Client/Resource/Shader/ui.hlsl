#include "common.hlsl"

/*
 *  UI_SHADER
 */

#define TYPE_STANDARD 0
#define TYPE_BACKGROUND 1
#define TYPE_TEXT 2
#define TYPE_BUTTON 3
#define TYPE_HORZGAUGE 4
#define TYPE_VERTGAUGE 5


struct VS_UI_OUTPUT
{
	float3 position : POSITION;
};

struct GS_UI_OUTPUT
{
	float4 position : SV_POSITION;
	float2 uv    : TEXCOORD;
};

VS_UI_OUTPUT VS_UI_MAIN(uint vid : SV_VertexID)
{
	VS_UI_OUTPUT output;
	output.position = float3(0.f, 0.f, 0.f);
	return output;
}

[maxvertexcount(4)]
void GS_UI_MAIN(point VS_UI_OUTPUT input[1], inout TriangleStream<GS_UI_OUTPUT> outStream)
{
	float3 position = float3(worldMatrix[0][0], worldMatrix[0][1], 1.f);
	float width = worldMatrix[1][0];
	float height = worldMatrix[1][1];

	float4 vertices[4];
	vertices[0] = float4(position, 1.f);
	vertices[0].x -= width; vertices[0].y -= height;
	vertices[1] = float4(position, 1.f);
	vertices[1].x -= width; vertices[1].y += height;
	vertices[2] = float4(position, 1.f);
	vertices[2].x += width; vertices[2].y -= height;
	vertices[3] = float4(position, 1.f);
	vertices[3].x += width; vertices[3].y += height;

	float2 uv[4] = { float2(0.0f, 1.0f), float2(0.0f, 0.0f), float2(1.0f, 1.0f), float2(1.0f, 0.0f) };

	GS_UI_OUTPUT output;
	for (int i = 0; i < 4; ++i) {
		output.position = vertices[i];
		output.uv = uv[i];
		outStream.Append(output);
	}
}

float4 PS_UI_MAIN(GS_UI_OUTPUT input) : SV_TARGET
{
	//if (g_type == TYPE_BACKGROUND) 
	//	return float4(0.f, 0.f, 0.f, 0.f);
	if (g_type == TYPE_HORZGAUGE) {
		if (input.uv.x <= g_age + (1 - g_age) * (g_gauge / g_maxGauge)) {
			return g_baseTexture.Sample(g_samplerWrap, input.uv);
		}
		return g_subTexture.Sample(g_samplerWrap, input.uv);
	}
	if (g_type == TYPE_VERTGAUGE) {
		if (1 - input.uv.y <= g_age + (1 - g_age) * (g_gauge / g_maxGauge)) {
			return g_baseTexture.Sample(g_samplerWrap, input.uv);
		}
		return g_subTexture.Sample(g_samplerWrap, input.uv);
	}
	return g_baseTexture.Sample(g_samplerWrap, input.uv);
}