#include "renderer/vertex/skin"
#include "renderer/buffer/object"
#include "renderer/buffer/animation"
#include "workflow/geometry"

VertexResult VS(VertexBase V)
{
	VertexResult Result = (VertexResult)0;
	Result.TexCoord = V.TexCoord * TexCoord;

	[branch] if (HasAnimation > 0)
	{
		matrix Offset =
			mul(Offsets[(int)V.Index.x], V.Bias.x) +
			mul(Offsets[(int)V.Index.y], V.Bias.y) +
			mul(Offsets[(int)V.Index.z], V.Bias.z) +
			mul(Offsets[(int)V.Index.w], V.Bias.w);

		Result.Position = Result.UV = mul(mul(V.Position, Offset), WorldViewProjection);
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

	return Result;
}

GBuffer PS(VertexResult V)
{
	float4 Color = float4(Diffuse, 1.0);
	[branch] if (HasDiffuse > 0)
		Color *= GetDiffuse(V.TexCoord);

	float3 Normal = V.Normal;
	[branch] if (HasNormal > 0)
        Normal = GetNormal(V.TexCoord, V.Normal, V.Tangent, V.Bitangent);

    return Compose(V.TexCoord, Color, Normal, V.UV.z / V.UV.w, MaterialId);
};