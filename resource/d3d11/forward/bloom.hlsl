#include "standard/space-uv"

cbuffer RenderConstant : register(b3)
{
	float2 Texel;
	float Intensity;
	float Threshold;
	float2 Scaling;
	float Samples;
	float SampleCount;
}

float4 PS(VertexResult V) : SV_TARGET0
{
    Fragment Frag; Material Mat; float3 Sample = 0.0;
	[loop] for (int x = -Samples; x < Samples; x++)
	{
		[loop] for (int y = -Samples; y < Samples; y++)
		{
			float2 Offset = float2(x, y) / (Texel * Scaling);
			Frag = GetFragment(V.TexCoord.xy + Offset);
			Mat = GetMaterial(Frag.Material);
			Sample += saturate(Frag.Diffuse + Mat.Emission.xyz * Mat.Emission.w - Threshold) * Intensity;
		}
	}

	return float4(Frag.Diffuse + Sample / SampleCount, 1.0f);
};