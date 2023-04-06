#include "compute.hlsl"

/*
 *  COMPOSITE_SHADER
 */

static const float2 g_texCoords[6] =
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f)
};

struct VS_COMPOSITE_OUTPUT
{
	float4 position    : SV_POSITION;
	float2 uv    : TEXCOORD;
};

VS_COMPOSITE_OUTPUT VS_COMPOSITE_MAIN(uint vid : SV_VertexID)
{
	VS_COMPOSITE_OUTPUT output;

	output.uv = g_texCoords[vid];

	// Map [0,1]^2 to NDC space.
	output.position = float4(2.0f * output.uv.x - 1.0f, 1.0f - 2.0f * output.uv.y, 0.0f, 1.0f);

	return output;
}

float4 PS_COMPOSITE_MAIN(VS_COMPOSITE_OUTPUT input) : SV_Target
{
	float4 c = g_baseTexture.SampleLevel(g_samplerClamp, input.uv, 0.0f);
	float4 e = g_subTexture.SampleLevel(g_samplerClamp, input.uv, 0.0f);

	return c * e;
}
