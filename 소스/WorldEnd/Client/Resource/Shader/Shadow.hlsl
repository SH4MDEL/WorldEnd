#include "Buffer.hlsl"

struct VS_SHADOW_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 biTangent : BITANGENT;
	float2 uv : TEXCOORD;
};

struct VS_SHADOW_OUTPUT
{
	float4 position : SV_POSITION;
};

VS_SHADOW_OUTPUT VS_SHADOW_MAIN(VS_SHADOW_INPUT input)
{
	VS_SHADOW_OUTPUT output;

	output.position = mul(float4(input.position, 1.0f), worldMatrix);
	output.position = mul(output.position, lightView);
	output.position = mul(output.position, lightProj);

	return output;
}

// 이 픽셀 셰이더는 알파 값 기반 투명 패턴이 적용되는 기하구조에만
// 쓰인다. 깊이 패스의 경우, 텍스처를 추출할 필요가 없는 기하구조에는
// 픽셀 셰이더를 사용하지 않아도 된다.
//void PS_SHADOW_MAIN(VS_SHADOW_OUTPUT pin)
//{
//	// 재질 자료를 가져온다.
//	MaterialData matData = gMaterialData[gMaterialIndex];
//	float4 diffuseAlbedo = matData.DiffuseAlbedo;
//	uint diffuseMapIndex = matData.DiffuseMapIndex;
//
//	// 텍스처 배열의 텍스처를 동적으로 조회한다.
//	diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);
//
//	// 텍스처 알파가 0.1보다 작으면 픽셀을 폐기한다. 셰이더 안에서
//	// 이 판정을 최대한 일찍 수행하는 것이 바람직하다. 그러면 폐기 시
//	// 셰이더의 나머지 코드의 실행을 생략할 수 있으므로 효율적이다.
//	clip(diffuseAlbedo.a - 0.1f);
//}

struct VS_ANIMATION_SHADOW_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 biTangent : BITANGENT;
	float2 uv : TEXCOORD;
	int4 indices : BONEINDEX;
	float4 weights : BONEWEIGHT;
};

struct VS_ANIMATION_SHADOW_OUTPUT
{
	float4 position : SV_POSITION;
};

VS_ANIMATION_SHADOW_OUTPUT VS_ANIMATION_SHADOW_MAIN(VS_ANIMATION_SHADOW_INPUT input)
{
	VS_ANIMATION_SHADOW_OUTPUT output;

	if (input.weights[0] > 0) {
		// 정점이 영향을 받는 뼈마다 오프셋 * 애니메이션 변환행렬을 전부 더합
		float4x4 mat = (float4x4)0.0f;
		for (int i = 0; i < 2; ++i) {
			mat += input.weights[i] * mul(boneOffsets[input.indices[i]], boneTransforms[input.indices[i]]);
		}

		output.position = mul(float4(input.position, 1.0f), mat);
		output.position = mul(output.position, lightView);
		output.position = mul(output.position, lightProj);
	}
	else {
		output.position = mul(float4(input.position, 1.0f), worldMatrix);
		output.position = mul(output.position, lightView);
		output.position = mul(output.position, lightProj);
	}

	return output;
}