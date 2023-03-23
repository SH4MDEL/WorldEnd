#include "common.hlsl"

/*
 *  EMITTERPARTICLE_SHADER
 */

struct VS_EMITTERPARTICLE_INPUT
{
	float3 position : POSITION;
	float3 velocity : VELOCITY;
	float age : AGE;
	float lifeTime : LIFETIME;
};

struct GS_EMITTERPARTICLE_OUTPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
	float age : AGE;
	float lifeTime : LIFETIME;
};


VS_EMITTERPARTICLE_INPUT VS_EMITTERPARTICLE_MAIN(VS_EMITTERPARTICLE_INPUT input)
{
	return input;
}

[maxvertexcount(1)]
void GS_EMITTERPARTICLE_STREAMOUTPUT(point VS_EMITTERPARTICLE_INPUT input[1], inout PointStream<VS_EMITTERPARTICLE_INPUT> output)
{
	VS_EMITTERPARTICLE_INPUT particle = input[0];

	particle.position.x = particle.velocity.x * g_age;
	particle.position.y = particle.velocity.y * g_age;
	particle.position.z = particle.velocity.z * g_age;

	output.Append(particle);
}

[maxvertexcount(4)]
void GS_EMITTERPARTICLE_DRAW(point VS_EMITTERPARTICLE_INPUT input[1], inout TriangleStream<GS_EMITTERPARTICLE_OUTPUT> outputStream)
{
	float3 up = float3(0.0f, 1.0f, 0.0f);
	float3 positionW = mul(float4(input[0].position, 1.0f), worldMatrix).xyz;
	float3 look = cameraPosition - positionW;
	look.y = 0.0f;
	look = normalize(look);
	float3 right = cross(up, look);
	float halfW = 0.04f;
	float halfH = 0.04f;
	float4 vertices[4] =
	{
		float4(positionW + (halfW * right) - (halfH * up), 1.0f), // LB
		float4(positionW + (halfW * right) + (halfH * up), 1.0f), // LT
		float4(positionW - (halfW * right) - (halfH * up), 1.0f), // RB
		float4(positionW - (halfW * right) + (halfH * up), 1.0f)  // RT
	};
	float2 uv[4] =
	{
		float2(0.0f, 1.0f),
		float2(0.0f, 0.0f),
		float2(1.0f, 1.0f),
		float2(1.0f, 0.0f)
	};

	GS_EMITTERPARTICLE_OUTPUT output = (GS_EMITTERPARTICLE_OUTPUT)0;
	[unroll]
	for (int i = 0; i < 4; ++i)
	{
		output.position = mul(vertices[i], viewMatrix);
		output.position = mul(output.position, projMatrix);
		output.uv = uv[i];
		output.age = input[0].age;
		output.lifeTime = input[0].lifeTime;
		outputStream.Append(output);
	}
}

float4 PS_EMITTERPARTICLE_MAIN(GS_EMITTERPARTICLE_OUTPUT input) : SV_TARGET
{
	if (g_age < 0.01f) return float4(0.f, 0.f, 0.f, 0.f);
	return float4(1.0f, 0.4f, 0.4f, 0.8f);
}
