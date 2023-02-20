#define MAX_LIGHTS 16
#define DIRECTIONAL_LIGHT 1
#define POINT_LIGHT 2
#define SPOT_LIGHT 3

struct PhongMaterial
{
	float4					m_ambient;
	float4					m_diffuse;
	float4					m_specular;
};

struct LIGHT
{
    float4					m_ambient;
    float4					m_diffuse;
    float4					m_specular;
    float3					m_position;
    float 					m_falloff;
    float3					m_direction;
    float 					m_theta; //cos(m_fTheta)
    float3					m_attenuation;
    float					m_phi; //cos(m_fPhi)
    bool					m_enable;
    int 					m_type;
    float					m_range;
    float					padding;
};

cbuffer cbLight : register(b3)
{
    LIGHT       lights[MAX_LIGHTS];
    float4      globalAmbient;
    uint        numLight;
}

float4 DirectionalLight(int index, float3 normal, float3 toCamera, PhongMaterial material)
{
	float3 toLight = -lights[index].m_direction;
	float diffuseFactor = dot(toLight, normal);
	float specularFactor = 0.0f;
	if (diffuseFactor > 0.0f) {
		if (material.m_specular.a != 0.0f) {
			float3 Half = normalize(toCamera + toLight);
			specularFactor = pow(max(dot(Half, normal), 0.0f), material.m_specular.a);
		}
	}

	return((lights[index].m_ambient * material.m_ambient) + (lights[index].m_diffuse * diffuseFactor * material.m_diffuse) + (lights[index].m_specular * specularFactor * material.m_specular));
}

float4 PointLight(int index, float3 position, float3 normal, float3 toCamera, PhongMaterial material)
{
	float3 toLight = lights[index].m_position - position;
	float distance = length(toLight);

	if (distance > lights[index].m_range) return float4(0.0f, 0.0f, 0.0f, 0.0f);

	toLight /= distance;

	float specularFactor = 0.0f;
	float diffuseFactor = dot(toLight, normal);
	if (diffuseFactor > 0.0f) {
		if (material.m_specular.a != 0.0f) {
			float3 Half = normalize(toCamera + toLight);
			specularFactor = pow(max(dot(Half, normal), 0.0f), material.m_specular.a);
		}
	}
	float attenuationFactor = 1.0f / dot(lights[index].m_attenuation, float3(1.0f, distance, distance * distance));

	return(((lights[index].m_ambient * material.m_ambient) + (lights[index].m_diffuse * diffuseFactor * material.m_diffuse) + (lights[index].m_specular * specularFactor * material.m_specular)) * attenuationFactor);

}

float4 SpotLight(int index, float3 position, float3 normal, float3 toCamera, PhongMaterial material)
{
	float3 toLight = lights[index].m_position - position;
	float distance = length(toLight);

	if (distance > lights[index].m_range) return float4(0.0f, 0.0f, 0.0f, 0.0f);
	toLight /= distance;

	float specularFactor = 0.0f;
	float diffuseFactor = dot(toLight, normal);
	if (diffuseFactor > 0.0f) {
		if (material.m_specular.a != 0.0f) {
			float3 Half = normalize(toCamera + toLight);
			specularFactor = pow(max(dot(Half, normal), 0.0f), material.m_specular.a);
		}
	}
	float alpha = max(dot(-toLight, lights[index].m_direction), 0.0f);
	float spotFactor = pow(max(((alpha - lights[index].m_phi) / (lights[index].m_theta - lights[index].m_phi)), 0.0f), lights[index].m_falloff);

	float attenuationFactor = 1.0f / dot(lights[index].m_attenuation, float3(1.0f, distance, distance * distance));

	return(((lights[index].m_ambient * material.m_ambient) + (lights[index].m_diffuse * diffuseFactor * material.m_diffuse) + (lights[index].m_specular * specularFactor * material.m_specular)) * attenuationFactor * spotFactor);
	
}

float4 Lighting(float3 position, float3 normal, PhongMaterial material, float shadowFactor)
{
	float3 toCamera = normalize(cameraPosition - position);

	float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);
	[unroll(MAX_LIGHTS)] 
	for (int i = 0; i < numLight; ++i) {
		if (lights[i].m_enable) {
			if (lights[i].m_type == DIRECTIONAL_LIGHT)
			{
				color += DirectionalLight(i, normal, toCamera, material) * shadowFactor;
			}
			else if (lights[i].m_type == POINT_LIGHT)
			{
				color += PointLight(i, position, normal, toCamera, material);
			}
			else if (lights[i].m_type == SPOT_LIGHT)
			{
				color += SpotLight(i, position, normal, toCamera, material);
			}
		}
	}
	color += (globalAmbient * material.m_ambient);
	color.a = material.m_diffuse.a;

	return color;
}

//***************************************************************************************
// LightingUtil.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Contains API for shader lighting.
//***************************************************************************************

//// 점광과 점적광에 적용하는 선형 감쇠 계수를 계산한다.
//float CalcAttenuation(float d, float falloffStart, float falloffEnd)
//{
//	return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
//}
//
//// 프레넬 방정식의 슐릭 근사를 구한다.
//// 법선이 n인 표면에서 프레넬 효과에 의해 반사되는 빛의 비율을
//// 빛 벡터 L과 표면 법선 n 사이의 각도에 근거하여 근사한다.
//float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVector)
//{
//	float cosIncidentAngle = saturate(dot(normal, lightVector));
//
//	float f0 = 1.0f - cosIncidentAngle;
//	float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);
//
//	return reflectPercent;
//}
//
//// 눈에 도달한 반사광의 양을 계산한다.
//// 즉 분산 반사와 반영 반사의 합을 구한다.
//float3 BlinnPhong(float3 lightStrength, float3 lightVector, float3 normal, float3 toEye, float4 albedo)
//{
//	const float m = glossiness * 256.0f;
//	const float3 defaultFresnel = float3(0.1f, 0.1f, 0.1f);
//
//	float3 halfVec = normalize(toEye + lightVector);
//
//	float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
//	float3 fresnelFactor = SchlickFresnel(defaultFresnel, halfVec, lightVector);
//
//	float3 specAlbedo = fresnelFactor * roughnessFactor;
//
//	// 반영 반사율 공식이 [0,1] 구간 바깥의 값을 낼 수도 있지만
//	// LDR 렌더링을 구현하므로 반사율을 1 미만으로 낮춘다.
//	specAlbedo = specAlbedo / (specAlbedo + 1.0f);
//
//	return (albedo.rgb + specAlbedo) * lightStrength;
//}
//
//
////---------------------------------------------------------------------------------------
//// 지향광
////---------------------------------------------------------------------------------------
//float3 DirectionalLight(LIGHT light, float3 normal, float3 toEye, float4 albedo)
//{
//    // 빛 벡터는 광선들이 나아가는 방향의 반대 방향을 가리킨다
//    float3 lightVector = -light.direction;
//
//    // 람베르트 코사인 법칙에 따라 빛의 세기를 줄인다.
//    float ndotl = max(dot(lightVector, normal), 0.0f);
//    float3 lightStrength = light.color * ndotl;
//
//    return BlinnPhong(lightStrength, lightVector, normal, toEye, albedo);
//}
//
////---------------------------------------------------------------------------------------
//// 점광
////---------------------------------------------------------------------------------------
//float3 PointLight(LIGHT light, float3 position, float3 normal, float3 toEye, float4 albedo)
//{
//    // 표면에서 광원으로의 벡터
//    float3 lightVector = light.position - position;
//
//    // 광원과 표면 사이의 거리
//    float d = length(lightVector);
//
//    // 범위 판정
//    if (d > light.falloffEnd)
//        return 0.0f;
//
//    // 빛 벡터 정규화
//    lightVector /= d;
//
//    // 람베르트 코사인 법칙에 따라 빛의 세기를 줄인다.
//    float ndotl = max(dot(lightVector, normal), 0.0f);
//    float3 lightStrength = light.color * ndotl;
//
//    // 거리에 따라 빛을 감쇠한다.
//    float att = CalcAttenuation(d, light.falloffStart, light.falloffEnd);
//    lightStrength *= att;
//
//    return BlinnPhong(lightStrength, lightVector, normal, toEye, albedo);
//}
//
////---------------------------------------------------------------------------------------
//// 점적광
////---------------------------------------------------------------------------------------
//float3 SpotLight(LIGHT light, float3 position, float3 normal, float3 toEye, float4 albedo)
//{
//    // 표면에서 광원으로의 벡터
//    float3 lightVector = light.position - position;
//
//    // 광원과 표면 사이의 거리
//    float d = length(lightVector);
//
//    // 범위 판정
//    if (d > light.falloffEnd)
//        return 0.0f;
//
//    // 빛 벡터 정규화
//    lightVector /= d;
//
//    // 람베르트 코사인 법칙에 따라 빛의 세기를 줄인다.
//    float ndotl = max(dot(lightVector, normal), 0.0f);
//    float3 lightStrength = light.color * ndotl;
//
//    // 거리에 따라 빛을 감쇠한다.
//    float att = CalcAttenuation(d, light.falloffStart, light.falloffEnd);
//    lightStrength *= att;
//
//    // 점적광 계수로 비례한다.
//    float spotFactor = pow(max(dot(-lightVector, light.direction), 0.0f), light.power);
//    lightStrength *= spotFactor;
//
//    return BlinnPhong(lightStrength, lightVector, normal, toEye, albedo);
//}
//
//float4 Lighting(float3 pos, float3 normal, float3 shadowFactor, float4 albedo)
//{
//    float3 result = 0;
//    float3 toEye = normalize(cameraPosition - pos);
//
//    [unroll(MAX_LIGHTS)]
//    for (int i = 0; i < numLight; ++i) {
//        if (lights[i].enable) {
//            if (lights[i].type == DIRECTIONAL_LIGHT)
//            {
//                result += shadowFactor * DirectionalLight(lights[i], normal, toEye, albedo);
//            }
//            else if (lights[i].type == POINT_LIGHT)
//            {
//                result += PointLight(lights[i], pos, normal, toEye, albedo);
//            }
//            else if (lights[i].type == SPOT_LIGHT)
//            {
//                result += SpotLight(lights[i], pos, normal, toEye, albedo);
//            }
//        }
//    }
//      
//    return float4(result, albedo.a);
//}