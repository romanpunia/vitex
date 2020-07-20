#include "standard/space-uv"

cbuffer RenderConstant : register(b3)
{
    float2 Texel;
    float IterationCount;
    float Blur;
    float Threshold;
    float Power;
    float Discard;
    float Additive;
}

float4 PS(VertexResult V) : SV_TARGET0
{
    float3 C = GetDiffuse(V.TexCoord.xy).xyz;
    float3 N = GetNormal(V.TexCoord.xy);
    float3 B = float3(0, 0, 0);
    float I = 0.0;

	[loop] for (float x = -IterationCount; x < IterationCount; x++)
	{
		[loop] for (float y = -IterationCount; y < IterationCount; y++)
		{
			float2 T = V.TexCoord.xy + float2(x, y) * Texel * Blur;
		    [branch] if (dot(GetNormal(T), N) < Discard)
                continue;

            B += GetPass(T).xyz; I++;
		}
	}

    [branch] if (I <= 0.0)
        return float4(C, 1.0);
    
    float3 R = Threshold + B * Power / I;
    return float4(Additive > 0.0 ? C + R : C * R, 1.0);
};