#include "renderer/vertex"
#include "renderer/buffer/object"
#include "workflow/geometry"

VertexResult VS(VertexBase V)
{
	VertexResult Result = (VertexResult)0;
	Result.Position = Result.UV = mul(V.Position, WorldViewProjection);
	Result.Normal = normalize(mul(V.Normal, (float3x3)World));
	Result.Tangent = normalize(mul(V.Tangent, (float3x3)World));
	Result.Bitangent = normalize(mul(V.Bitangent, (float3x3)World));
	Result.TexCoord = V.TexCoord * TexCoord;

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

    return ComposeLimpid(V.TexCoord, Color, Normal, V.UV.z / V.UV.w, MaterialId);
};