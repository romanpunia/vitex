#include "sdk/layouts/skin"
#include "sdk/channels/gbuffer"
#include "sdk/buffers/object"
#include "sdk/buffers/viewer"
#include "sdk/buffers/animation"

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.TexCoord = V.TexCoord * TexCoord;

    float4 Position = float4(V.Position, 1.0);
	[branch] if (HasAnimation > 0)
	{
		matrix Offset =
			mul(Offsets[(int)V.Index.x], V.Bias.x) +
			mul(Offsets[(int)V.Index.y], V.Bias.y) +
			mul(Offsets[(int)V.Index.z], V.Bias.z) +
			mul(Offsets[(int)V.Index.w], V.Bias.w);

        Position = mul(float4(V.Position, 1.0), Offset);
		Result.Position = Result.UV = mul(Position, WorldViewProjection);
		Result.Normal = normalize(mul(mul(float4(V.Normal, 0), Offset).xyz, (float3x3)World));
		Result.Tangent = normalize(mul(mul(float4(V.Tangent, 0), Offset).xyz, (float3x3)World));
		Result.Bitangent = normalize(mul(mul(float4(V.Bitangent, 0), Offset).xyz, (float3x3)World));   
    }
	else
	{
		Result.Position = Result.UV = mul(Position, WorldViewProjection);
		Result.Normal = normalize(mul(V.Normal, (float3x3)World));
		Result.Tangent = normalize(mul(V.Tangent, (float3x3)World));
		Result.Bitangent = normalize(mul(V.Bitangent, (float3x3)World));
	}

    [branch] if (HasHeight > 0)
        Result.Direction = GetDirection(Result.Tangent, Result.Bitangent, Result.Normal, mul(Position, World), TexCoord);

	return Result;
}

GBuffer PS(VOutput V)
{
    float2 TexCoord = V.TexCoord;
    [branch] if (HasHeight > 0)
        TexCoord = GetParallax(TexCoord, V.Direction, HeightAmount, HeightBias);
    
	float4 Color = float4(Diffuse, 1.0);
	[branch] if (HasDiffuse > 0)
		Color *= GetDiffuse(TexCoord);

	float3 Normal = V.Normal;
	[branch] if (HasNormal > 0)
        Normal = GetNormal(TexCoord, V.Normal, V.Tangent, V.Bitangent);

    return Compose(TexCoord, Color, Normal, V.UV.z / V.UV.w, MaterialId);
};