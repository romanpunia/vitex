#include "standard/space-sv"

cbuffer RenderConstant : register(b3)
{
	float2 Texel;
    float Radius;
    float Bokeh;
    float Scale;
    float FocalDepth;
    float NearDistance;
    float NearRange;
    float FarDistance;
    float FarRange;
    float2 Padding;
}

float GetWeight(in float2 TexCoord)
{
    float4 Position = mul(float4(GetPosition(TexCoord, GetDepth(TexCoord)), 1.0), ViewProjection);
	float Near = 1.0 - (Position.z - NearDistance) / NearRange;
	float Far = (Position.z - (FarDistance - FarRange)) / FarRange;

	return saturate(max(Near, Far)) * FocalDepth;
}

void Blur(inout float3 Linear, inout float3 Gamma, in float2 UV, in float Amount, in float X, in float Y)
{
    float2 Offset = UV + Texel * float2(X, Y) * Radius * Amount;
    float3 Color = GetDiffuseLevel(saturate(Offset), 0).xyz;
	float3 Factor = 1.0 + pow(Color, 4.0) * Bokeh;
	Linear += Color * Factor;
	Gamma += Factor;
}

float4 PS(VertexResult V) : SV_TARGET0
{
	float2 UV = GetTexCoord(V.TexCoord);
	float Amount = GetWeight(UV);
    
    [branch] if (Amount <= 0.001)
        return GetDiffuseLevel(UV, 0);

    float3 Linear = 0.0;
    float3 Gamma = 0.000001;
    float x, y;

	[loop] for (y = -7.0 + Scale; y <= 7.0 - Scale; y += 1.0)
    {
		[loop] for (x = -7.0 + Scale; x <= 7.0 - Scale; x += 1.0)
			Blur(Linear, Gamma, UV, Amount, x, y);
    }
    
    [loop] for (x = -5.0 + Scale; x <= 5.0 - Scale; x += 1.0)
    {
        Blur(Linear, Gamma, UV, Amount, x, -8.0 + Scale);
        Blur(Linear, Gamma, UV, Amount, x, 8.0 - Scale);
    }

    [loop] for (y = -5.0 + Scale; y <= 5.0 - Scale; y += 1.0)
    {
        Blur(Linear, Gamma, UV, Amount, -8.0 + Scale, y);
        Blur(Linear, Gamma, UV, Amount, 8.0 - Scale, y);
    }
    
    [loop] for (x = -3.0 + Scale; x <= 3.0 -  Scale; x += 1.0)
    {
        Blur(Linear, Gamma, UV, Amount, x, -9.0 + Scale);
        Blur(Linear, Gamma, UV, Amount, x, 9.0 - Scale);
    }

    [loop] for (y = -3.0 + Scale; y <= 3.0 - Scale; y += 1.0)
    {
        Blur(Linear, Gamma, UV, Amount, -9.0 + Scale, y);
        Blur(Linear, Gamma, UV, Amount, 9.0 - Scale, y);
    }

    return float4(Linear / Gamma, 1.0);
}