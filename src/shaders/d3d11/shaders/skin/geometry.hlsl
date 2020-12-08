#include "sdk/layouts/skin"
#include "sdk/channels/gbuffer"
#include "sdk/buffers/object"
#include "sdk/buffers/viewer"
#include "sdk/buffers/animation"

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.TexCoord = V.TexCoord * TexCoord;

    float4 Position = V.Position;
	[branch] if (HasAnimation > 0)
	{
		matrix Offset =
			mul(Offsets[(int)V.Index.x], V.Bias.x) +
			mul(Offsets[(int)V.Index.y], V.Bias.y) +
			mul(Offsets[(int)V.Index.z], V.Bias.z) +
			mul(Offsets[(int)V.Index.w], V.Bias.w);

        Position = mul(V.Position, Offset);
		Result.Position = Result.UV = mul(Position, WorldViewProjection);
		Result.Normal = normalize(mul(mul(float4(V.Normal, 0), Offset).xyz, (float3x3)World));
		Result.Tangent = normalize(mul(mul(float4(V.Tangent, 0), Offset).xyz, (float3x3)World));
		Result.Bitangent = normalize(mul(mul(float4(V.Bitangent, 0), Offset).xyz, (float3x3)World));   
    }
	else
	{
		Result.Position = Result.UV = mul(V.Position, WorldViewProjection);
		Result.Normal = normalize(mul(V.Normal, (float3x3)World));
		Result.Tangent = normalize(mul(V.Tangent, (float3x3)World));
		Result.Bitangent = normalize(mul(V.Bitangent, (float3x3)World));
	}

    [branch] if (HasHeight > 0)
    {
        float3x3 TangentSpace;
        TangentSpace[0] = Result.Tangent;
        TangentSpace[1] = Result.Bitangent;
        TangentSpace[2] = Result.Normal;
        TangentSpace = transpose(TangentSpace);

        Result.Direction = normalize(ViewPosition - mul(Position, World).xyz);
        Result.Direction = mul(Result.Direction, TangentSpace);
    }

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