
cbuffer cbFilter : register(b0)
{
	int blurRadius;
}

Texture2D g_input : register(t0);
RWTexture2D<float4> g_output : register(u0);

static const int maxBlurRadius = 5;
groupshared float4 g_cache[256 + 2 * maxBlurRadius];

[numthreads(256, 1, 1)]
void CS_HORZBLUR_MAIN(int3 groupThreadID : SV_GroupThreadID,
				  int3 dispatchThreadID : SV_DispatchThreadID)
{
	// Put in an array for each indexing.
	float weights[11] = { 0.05f, 0.05f, 0.1f, 0.1f, 0.1f, 0.2f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f };

	//
	// Fill local thread storage to reduce bandwidth.  To blur 
	// N pixels, we will need to load N + 2 * BlurRadius pixels
	// due to the blur radius.
	//
	
	// This thread group runs N threads.  To get the extra 2 * BlurRadius pixels, 
	// have 2 * BlurRadius threads sample an extra pixel.
	if (groupThreadID.x < blurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
		int x = max(dispatchThreadID.x - blurRadius, 0);
		g_cache[groupThreadID.x] = g_input[int2(x, dispatchThreadID.y)];
	}
	if (groupThreadID.x >= 256 - blurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
		int x = min(dispatchThreadID.x + blurRadius, g_input.Length.x - 1);
		g_cache[groupThreadID.x + 2 * blurRadius] = g_input[int2(x, dispatchThreadID.y)];
	}

	// Clamp out of bound samples that occur at image borders.
	g_cache[groupThreadID.x + blurRadius] = g_input[min(dispatchThreadID.xy, g_input.Length.xy - 1)];

	// Wait for all threads to finish.
	GroupMemoryBarrierWithGroupSync();

	//
	// Now blur each pixel.
	//

	float4 blurColor = float4(0, 0, 0, 0);

	for (int i = -blurRadius; i <= blurRadius; ++i)
	{
		int k = groupThreadID.x + blurRadius + i;

		blurColor += weights[i + blurRadius] * g_cache[k];
	}

	g_output[dispatchThreadID.xy] = blurColor;

}

[numthreads(1, 256, 1)]
void CS_VERTBLUR_MAIN(int3 groupThreadID : SV_GroupThreadID,
	int3 dispatchThreadID : SV_DispatchThreadID)
{
	// Put in an array for each indexing.
	float weights[11] = { 0.05f, 0.05f, 0.1f, 0.1f, 0.1f, 0.2f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f };

	//
	// Fill local thread storage to reduce bandwidth.  To blur 
	// N pixels, we will need to load N + 2 * BlurRadius pixels
	// due to the blur radius.
	//

	// This thread group runs N threads.  To get the extra 2 * BlurRadius pixels, 
	// have 2 * BlurRadius threads sample an extra pixel.
	if (groupThreadID.y < blurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
		int y = max(dispatchThreadID.y - blurRadius, 0);
		g_cache[groupThreadID.y] = g_input[int2(dispatchThreadID.x, y)];
	}
	if (groupThreadID.y >= 256 - blurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
		int y = min(dispatchThreadID.y + blurRadius, g_input.Length.y - 1);
		g_cache[groupThreadID.y + 2 * blurRadius] = g_input[int2(dispatchThreadID.x, y)];
	}

	// Clamp out of bound samples that occur at image borders.
	g_cache[groupThreadID.y + blurRadius] = g_input[min(dispatchThreadID.xy, g_input.Length.xy - 1)];


	// Wait for all threads to finish.
	GroupMemoryBarrierWithGroupSync();

	//
	// Now blur each pixel.
	//

	float4 blurColor = float4(0, 0, 0, 0);

	for (int i = -blurRadius; i <= blurRadius; ++i)
	{
		int k = groupThreadID.y + blurRadius + i;

		blurColor += weights[i + blurRadius] * g_cache[k];
	}

	g_output[dispatchThreadID.xy] = blurColor;
}