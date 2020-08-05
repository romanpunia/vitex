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

    [branch] if (HasHeight > 0)
    {
        float3x3 TangentSpace;
        TangentSpace[0] = Result.Tangent;
        TangentSpace[1] = Result.Bitangent;
        TangentSpace[2] = Result.Normal;

        float3 Direction = normalize(ViewPosition - mul(V.Position, World).xyz);
        Result.Direction = mul(Direction, TangentSpace).xy;
    }

	return Result;
}

GBuffer PS(VertexResult V)
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