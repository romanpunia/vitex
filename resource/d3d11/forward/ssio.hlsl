#include "standard/space-sv"
#include "standard/random-float"

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
	float Samples;
	float SampleCount;
	float Padding;
}

float3 Occlude(in float2 TexCoord, in float3 Position, in float3 Normal)
{
	float3 Direction = GetPosition(TexCoord, GetDepth(TexCoord)) - Position;
	float Length = length(Direction) * Scale;
	float Occlusion = max(0.0, dot(normalize(Direction), Normal) * Power - Bias) * Intensity / (1.0f + Length * Length);

	return GetDiffuse(TexCoord).xyz * Occlusion;
}

float4 PS(VertexResult V) : SV_TARGET0
{
	float2 TexCoord = GetTexCoord(V.TexCoord);
	Fragment Frag = GetFragment(TexCoord);
	Material Mat = GetMaterial(Frag.Material);
	float Fade = saturate(pow(abs(distance(ViewPosition.xyz, Frag.Position) / Distance), Fading));
	float AmbientRadius = Radius + Mat.Radius;
    float3 Factor = 0.0f;

	[loop] for (int x = -Samples; x < Samples; x++)
	{
		[loop] for (int y = -Samples; y < Samples; y++)
		{
			float2 Ray = RandomFloat2(TexCoord) * Step - float2(y, x) * Offset;
			float2 Direction = reflect(Ray, float2(x, y)) * AmbientRadius;
			Factor += Mat.Occlusion * (Occlude(TexCoord + Direction, Frag.Position, Frag.Normal) +
				Occlude(TexCoord - Direction, Frag.Position, Frag.Normal));
		}
	}

	return float4(GetDiffuse(TexCoord).xyz * (1 + Factor * Fade / SampleCount), 1);
};