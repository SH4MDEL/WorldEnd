#include "compute.hlsl"

/*
 *  FADE_SHADER
 */

[numthreads(16, 16, 1)]
void CS_FADE_MAIN(int3 dispatchThreadID : SV_DispatchThreadID)
{
	g_output[dispatchThreadID.xy] = g_baseTexture[dispatchThreadID.xy] * (fadeAge / fadeLifetime);
}