#include "std/layouts/shape"
#include "std/channels/effect"
#include "std/core/position"

cbuffer RenderConstant : register(b3)
{
	float2 Texel;
    float Radius;
    float Bokeh;
    float Scale;
    float NearDistance;
    float NearRange;
    float FarDistance;
    float FarRange;
    float3 Padding;
}

float GetWeight(float2 TexCoord, float NearD, float NearR, float FarD, float FarR)
{
    float4 Position = mul(float4(GetPosition(TexCoord, GetDepth(TexCoord)), 1.0), ViewProjection);
	float N = 1.0 - (Position.z - NearD) / NearR;
	float F = (Position.z - (FarD - FarR)) / FarR;

	return saturate(max(N, F));
}

void Blur(inout float3 Linear, inout float3 Gamma, float2 UV, float2 Amount, float X, float Y)
{
    float2 Offset = UV + float2(X, Y) * Amount;
    float3 Color = GetDiffuse(saturate(Offset), 0).xyz;
	float3 Factor = 1.0 + pow(Color, 4.0) * Bokeh;
	Linear += Color * Factor;
	Gamma += Factor;
}

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = float4(V.Position, 1.0);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 PS(VOutput V) : SV_TARGET0
{
	float2 UV = GetTexCoord(V.TexCoord);
	float Amount = Radius * GetWeight(UV, NearDistance, NearRange, FarDistance, FarRange);
    float x = 0.0, y = 0.0;
    
    [branch] if (Amount <= 0.001)
        return GetDiffuse(UV, 0);

    float2 Offset = Texel * Amount;
    float3 Linear = 0.0;
    float3 Gamma = 0.000001;

	[loop] for (y = -7.0 + Scale; y <= 7.0 - Scale; y += 1.0)
    {
		[loop] for (x = -7.0 + Scale; x <= 7.0 - Scale; x += 1.0)
			Blur(Linear, Gamma, UV, Offset, x, y);
    }
    
    [loop] for (x = -5.0 + Scale; x <= 5.0 - Scale; x += 1.0)
    {
        Blur(Linear, Gamma, UV, Offset, x, -8.0 + Scale);
        Blur(Linear, Gamma, UV, Offset, x, 8.0 - Scale);
    }

    [loop] for (y = -5.0 + Scale; y <= 5.0 - Scale; y += 1.0)
    {
        Blur(Linear, Gamma, UV, Offset, -8.0 + Scale, y);
        Blur(Linear, Gamma, UV, Offset, 8.0 - Scale, y);
    }
    
    [loop] for (x = -3.0 + Scale; x <= 3.0 -  Scale; x += 1.0)
    {
        Blur(Linear, Gamma, UV, Offset, x, -9.0 + Scale);
        Blur(Linear, Gamma, UV, Offset, x, 9.0 - Scale);
    }

    [loop] for (y = -3.0 + Scale; y <= 3.0 - Scale; y += 1.0)
    {
        Blur(Linear, Gamma, UV, Offset, -9.0 + Scale, y);
        Blur(Linear, Gamma, UV, Offset, 9.0 - Scale, y);
    }

    return float4(Linear / Gamma, 1.0);
}