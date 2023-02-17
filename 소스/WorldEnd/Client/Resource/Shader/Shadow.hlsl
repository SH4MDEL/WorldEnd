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
	//output.position = mul(output.position, NDCspace);

	return output;
}

// �� �ȼ� ���̴��� ���� �� ��� ���� ������ ����Ǵ� ���ϱ�������
// ���δ�. ���� �н��� ���, �ؽ�ó�� ������ �ʿ䰡 ���� ���ϱ�������
// �ȼ� ���̴��� ������� �ʾƵ� �ȴ�.
//void PS_SHADOW_MAIN(VS_SHADOW_OUTPUT pin)
//{
//	// ���� �ڷḦ �����´�.
//	MaterialData matData = gMaterialData[gMaterialIndex];
//	float4 diffuseAlbedo = matData.DiffuseAlbedo;
//	uint diffuseMapIndex = matData.DiffuseMapIndex;
//
//	// �ؽ�ó �迭�� �ؽ�ó�� �������� ��ȸ�Ѵ�.
//	diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);
//
//	// �ؽ�ó ���İ� 0.1���� ������ �ȼ��� ����Ѵ�. ���̴� �ȿ���
//	// �� ������ �ִ��� ���� �����ϴ� ���� �ٶ����ϴ�. �׷��� ��� ��
//	// ���̴��� ������ �ڵ��� ������ ������ �� �����Ƿ� ȿ�����̴�.
//	clip(diffuseAlbedo.a - 0.1f);
//}
