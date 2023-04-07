cbuffer cbFilter : register(b0)
{
	int blurRadius;
}

cbuffer cbFade : register(b1)
{
	float fadeAge : packoffset(c0.x);
	float fadeLifetime : packoffset(c0.y);
}

Texture2D g_baseTexture : register(t0);
Texture2D g_subTexture : register(t1);
RWTexture2D<float4> g_output : register(u0);

SamplerState g_samplerClamp : register(s0);