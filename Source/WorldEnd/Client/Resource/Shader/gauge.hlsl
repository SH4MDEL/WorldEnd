#include "common.hlsl"

/*
 *  GAUGE_SHADER
 */

struct VS_GAUGE_INPUT
{
	float3 position : POSITION;
	float2 size : SIZE;
};

struct VS_GAUGE_OUTPUT
{
	float4 position : POSITION;
	float2 size : SIZE;
};

struct GS_GAUGE_OUTPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 uv0 : TEXCOORD;
	float2 uv1 : TEXCOORD;
};

VS_GAUGE_OUTPUT VS_GAUGE_MAIN(VS_GAUGE_INPUT input)
{
	VS_GAUGE_OUTPUT output;

	output.position = mul(float4(input.position, 1.0f), worldMatrix);
	output.size = input.size;
	return output;
}

[maxvertexcount(4)]
void GS_GAUGE_MAIN(point VS_GAUGE_OUTPUT input[1], inout TriangleStream<GS_GAUGE_OUTPUT> outStream)
{
	float3 up = float3(0.0f, 1.0f, 0.0f);
	float3 look = cameraPosition - input[0].position.xyz;
	look = normalize(look);
	float3 right = cross(up, look);
	float halfW = input[0].size.x * 0.5f;
	float halfH = input[0].size.y * 0.5f;
	float4 vertices[4];
	vertices[0] = float4(input[0].position.xyz + halfW * right - halfH * up, 1.0f);
	vertices[1] = float4(input[0].position.xyz + halfW * right + halfH * up, 1.0f);
	vertices[2] = float4(input[0].position.xyz - halfW * right - halfH * up, 1.0f);
	vertices[3] = float4(input[0].position.xyz - halfW * right + halfH * up, 1.0f);
	float2 uv[4] = { float2(0.0f, 1.0f), float2(0.0f, 0.0f), float2(1.0f, 1.0f), float2(1.0f, 0.0f) };

	GS_GAUGE_OUTPUT output;
	for (int i = 0; i < 4; ++i) {
		output.position = mul(vertices[i], viewMatrix);
		output.position = mul(output.position, projMatrix);
		output.normal = look;
		output.uv0 = uv[i];
		output.uv1 = uv[i];
		outStream.Append(output);
	}
}

[earlydepthstencil]
float4 PS_HORZGAUGE_MAIN(GS_GAUGE_OUTPUT input) : SV_TARGET
{
	if (input.uv0.x <= g_age + (1 - g_age) * (g_gauge / g_maxGauge)) {
		return g_baseTexture.Sample(g_samplerWrap, input.uv0);
	}
	return g_subTexture.Sample(g_samplerWrap, input.uv1);
}

[earlydepthstencil]
float4 PS_VERTGAUGE_MAIN(GS_GAUGE_OUTPUT input) : SV_TARGET
{
	if (1 - input.uv0.y <= g_age + (1 - g_age) * (g_gauge / g_maxGauge)) {
		return g_baseTexture.Sample(g_samplerWrap, input.uv0);
	}
	return g_subTexture.Sample(g_samplerWrap, input.uv1);
}