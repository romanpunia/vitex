#include "standard/space-sv"
#include "standard/random-float"

cbuffer RenderConstant : register(b3)
{
	float2 Texel;
	float Threshold;
	float Gain;
	float Fringe;
	float Bias;
	float Dither;
	float Samples;
	float Rings;
	float FarDistance;
	float FarRange;
	float NearDistance;
	float NearRange;
	float FocalDepth;
	float Intensity;
	float Circular;
}

float3 Aberate(in float2 TexCoord, in float Weight)
{
	float3 Coefficient = float3(0.299, 0.587, 0.114);
	float3 Aberation = float3(
        GetDiffuseLevel(TexCoord + float2(0.0, 1.0) * Texel * Fringe * Weight, 0).r,
	    GetDiffuseLevel(TexCoord + float2(-0.866, -0.5) * Texel * Fringe * Weight, 0).g,
	    GetDiffuseLevel(TexCoord + float2(0.866, -0.5) * Texel * Fringe * Weight, 0).b);
	float Luminance = dot(Aberation * Intensity, Coefficient);

	return Aberation + lerp(0, Aberation, max((Luminance - Threshold) * Gain, 0.0) * Weight);
}

float Weight(in float2 TexCoord)
{
    float4 Position = mul(float4(GetPosition(TexCoord, GetDepth(TexCoord)), 1.0), ViewProjection);
	float Alpha = 1.0f - (Position.z - NearDistance) / NearRange;
	float Gamma = (Position.z - (FarDistance - FarRange)) / FarRange;

	return saturate(max(Alpha, Gamma)) * FocalDepth;
}

float4 PS(VertexResult V) : SV_TARGET0
{
	float2 TexCoord = GetTexCoord(V.TexCoord);
	float Amount = Weight(TexCoord);
	float3 Color = GetDiffuse(TexCoord).xyz;
	float2 Noise = RandomFloat2(TexCoord) * Dither * Amount;
	float Width = Texel.x * Amount + Noise.x;
	float Height = Texel.y * Amount + Noise.y;
	float SampleAmount = 1.0;

	[loop] for (int i = 1; i <= Rings; i++)
	{
		float Count = i * Samples;
		[loop] for (int j = 0; j < Count; j++)
		{
			float Step = 6.28318530f / Count;
			float2 Offset = TexCoord + float2(cos(j * Step) * i * Width, sin(j * Step) * i * Height);

			[branch] if (Weight(Offset) >= Amount)
			{
				Color += Aberate(Offset, Amount) * lerp(1.0f, (float)i / Rings, Bias) * Circular;
				SampleAmount += lerp(1.0f, (float)i / Rings, Bias);
			}
		}
	}

	Color /= SampleAmount;
	return float4(Color, 1);
};