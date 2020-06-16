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
        return float4(1.0, 1.0, 1.0, 1.0);

	float F = saturate(pow(abs(distance(ViewPosition.xyz, Frag.Position) / Distance), Fading));
	float O = Radius + Mat.Radius, Count = 0.0;
    float3 C = 0.0;

	[loop] for (float x = -IterationCount; x < IterationCount; x++)
	{
		[loop] for (float y = -IterationCount; y < IterationCount; y++)
		{
			float2 R = RandomFloat2(TexCoord) * Step - float2(y, x) * Offset;
			float2 D = reflect(R, float2(x, y)) * O;
            float F1 = 0, F2 = 0;

            [branch] if (RayBounce(Frag, Ray, TexCoord + D, F1))
            {
                C += Mat.Occlusion * F1;
                Count++;
            }

            [branch] if (RayBounce(Frag, Ray, TexCoord - D, F2))
            {
                C += Mat.Occlusion * F2;
                Count++;
            }
		}
	}

    [branch] if (Count <= 0.0)
        return float4(1.0, 1.0, 1.0, 1.0);

    return float4(C * F / Count, 1);
};