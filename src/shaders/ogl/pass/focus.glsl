#include "standard/space-sv"

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

float GetWeight(in float2 TexCoord, in float NearD, in float NearR, in float FarD, in float FarR)
{
    float4 Position = mul(float4(GetPosition(TexCoord, GetDepth(TexCoord)), 1.0), ViewProjection);
	float N = 1.0 - (Position.z - NearD) / NearR;
	float F = (Position.z - (FarD - FarR)) / FarR;

	return saturate(max(N, F));
}

void Blur(inout float3 Linear, inout float3 Gamma, in float2 UV, in float2 Amount, in float X, in float Y)
{
    float2 Offset = UV + float2(X, Y) * Amount;
    float3 Color = GetDiffuseLevel(saturate(Offset), 0).xyz;
	float3 Factor = 1.0 + pow(Color, 4.0) * Bokeh;
	Linear += Color * Factor;
	Gamma += Factor;
}

float4 PS(VertexResult V) : SV_TARGET0
{
	float2 UV = GetTexCoord(V.TexCoord);
	float Amount = Radius * GetWeight(UV, NearDistance, NearRange, FarDistance, FarRange);
    float x = 0.0, y = 0.0;
    
    [branch] if (Amount <= 0.001)
        return GetDiffuseLevel(UV, 0);

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