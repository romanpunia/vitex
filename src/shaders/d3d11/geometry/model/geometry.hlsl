#include "std/layouts/vertex"
#include "std/channels/gbuffer"
#include "std/buffers/object"
#include "std/buffers/viewer"

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = Result.UV = mul(float4(V.Position, 1.0), WorldViewProjection);
	Result.Normal = normalize(mul(V.Normal, (float3x3)World));
	Result.Tangent = normalize(mul(V.Tangent, (float3x3)World));
	Result.Bitangent = normalize(mul(V.Bitangent, (float3x3)World));
	Result.TexCoord = V.TexCoord * TexCoord;

    [branch] if (HasHeight > 0)
        Result.Direction = GetDirection(Result.Tangent, Result.Bitangent, Result.Normal, mul(float4(V.Position, 1.0), World), TexCoord);

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