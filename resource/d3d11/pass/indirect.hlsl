#include "standard/space-sv"
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
	Material Mat = GetMaterial(Frag.Material);
    Bounce Ray = GetBounce(Scale, Intensity, Bias, Power, Threshold);

    [branch] if (Frag.Depth >= 1.0)
        return float4(0.0, 0.0, 0.0, 0.0);

    float Count = 0.0;
    float T = GetRoughnessLevel(Frag, Mat, 1.0);
	float F = saturate(pow(abs(distance(ViewPosition.xyz, Frag.Position) / Distance), Fading));
	float O = T * (Radius + Mat.Radius);
    float3 C = Frag.Diffuse;

	[loop] for (float x = -IterationCount; x < IterationCount; x++)
	{
		[loop] for (float y = -IterationCount; y < IterationCount; y++)
		{
			float2 R = RandomFloat2(TexCoord) * Step - float2(y, x) * Offset;
			float2 D = reflect(R, float2(x, y)) * O;
            float3 F1 = float3(0, 0, 0);
            float3 F2 = float3(0, 0, 0);

            [branch] if (RayBounce(Frag, Ray, TexCoord + D, F1.x))
            {
                C += Mat.Occlusion * GetDiffuse(TexCoord + D).xyz * min(1.0, F1.x);
                Count++;
            }

            [branch] if (RayBounce(Frag, Ray, TexCoord - D, F2.x))
            {
                C += Mat.Occlusion * GetDiffuse(TexCoord - D).xyz * min(1.0, F2.x);
                Count++;
            }
		}
	}

    [branch] if (Count <= 0.0)
        return float4(0.0, 0.0, 0.0, 0.0);

    return float4(T * C * F / (Count + 1), 1);
};