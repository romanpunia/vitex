#include "workflow/rasterizer"
#include "renderer/gbuffer"

GBuffer Compose(in float2 TexCoord, in float4 Diffuse, in float3 Normal, in float Depth, in float MaterialId)
{
	GBuffer Result;
    Result.Channel0 = Diffuse;
    Result.Channel1.xyz = Normal;
    Result.Channel1.w = MaterialId;
    Result.Channel2 = Depth;
    Result.Channel3.x = GetRoughness(TexCoord);
    Result.Channel3.y = GetMetallic(TexCoord);
    Result.Channel3.z = GetOcclusion(TexCoord);
    Result.Channel3.w = GetEmission(TexCoord);

    return Result;
}