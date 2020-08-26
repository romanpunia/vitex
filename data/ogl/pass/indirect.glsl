#include "standard/space-sv"
#include "standard/cook-torrance"
#include "standard/random-float"
#include "standard/ray-march"

cbuffer RenderConstant : register(b3)
{
	float Scale;
	float Intensity;
	float Bias;
	float Radius;
	float Step;
	float Offset;
	float Distance;
	float Fading;
	float Power;
	float IterationCount;
	float Threshold;
	float Padding;
}

float4 PS(VertexResult V) : SV_TARGET0
{
	float2 TexCoord = GetTexCoord(V.TexCoord);
	Fragment Frag = GetFragment(TexCoord);

    [branch] if (Frag.Depth >= 1.0)
        return float4(0.0, 0.0, 0.0, 0.0);

	Material Mat = GetMaterial(Frag.Material);
    float Z = GetOcclusionFactor(Frag, Mat);
    Bounce Ray = GetBounce(Scale, Intensity, Bias, Power * Z, Threshold);
    float T = GetRoughnessLevel(Frag, Mat, 1.0);
	float F = saturate(pow(abs(distance(ViewPosition, Frag.Position) / Distance), Fading));
	float O = T * (Radius + Mat.Radius);
    float3 C = Frag.Diffuse;
    float Count = 0.0;

	[loop] for (float x = -IterationCount; x < IterationCount; x++)
	{
		[loop] for (float y = -IterationCount; y < IterationCount; y++)
		{
			float2 R = RandomFloat2(TexCoord) * Step - float2(y, x) * Offset;
			float2 H = reflect(R, float2(x, y)) * O;
            float3 F1 = float3(TexCoord + H, 0);
            float3 F2 = float3(TexCoord - H, 0);

            [branch] if (RayBounce(Frag, Ray, F1.xy, F1.z))
            {
                C += GetDiffuse(F1.xy).xyz * min(1.0, F1.z);
                Count++;
            }

            [branch] if (RayBounce(Frag, Ray, F2.xy, F2.z))
            {
                C += GetDiffuse(F2.xy).xyz * min(1.0, F2.z);
                Count++;
            }
		}
	}

    [branch] if (Count <= 0.0)
        return float4(0.0, 0.0, 0.0, 0.0);

    float R = GetRoughnessFactor(Frag, Mat);
	float3 M = GetMetallicFactor(Frag, Mat);
	float3 E = normalize(Frag.Position - ViewPosition);
	float3 D = normalize(reflect(E, Frag.Normal)), O;
    float3 G = GetLight(E, D, Frag.Normal, M, R, O);

    return float4(saturate((G + O) * T * C * F / (Count + 1)), 1);
};