#include "standard/space-uv"

cbuffer RenderConstant : register(b3)
{
    float2 Texel;
    float Samples;
    float Intensity;
    float Threshold;
    float Scale;
    float Padding1;
    float Padding2;
}

float4 PS(VertexResult V) : SV_TARGET0
{
    float3 Diffuse = GetDiffuse(V.TexCoord.xy).xyz;
    float3 B = float3(0, 0, 0);

	[loop] for (int x = -Samples; x < Samples; x++)
	{
		[loop] for (int y = -Samples; y < Samples; y++)
		{
			float2 Offset = float2(x, y) * Texel * Scale;
			B += saturate(GetDiffuse(V.TexCoord.xy + Offset).xyz - Threshold);
		}
	}

    B /= 4 * Samples * Samples;
	return float4(Diffuse + B * Intensity, 1.0);
};