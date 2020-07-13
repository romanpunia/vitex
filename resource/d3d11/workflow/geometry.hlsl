#include "workflow/rasterizer"
#include "renderer/gbuffer"

GBuffer Compose(in float2 TexCoord, in float3 Diffuse, in float3 Normal, in float Depth, in float MaterialId)
{
	GBuffer Result;
    Result.Diffuse.xyz = Diffuse;
    Result.Diffuse.w = GetRoughness(TexCoord);
    Result.Normal.xyz = Normal;
    Result.Normal.w = MaterialId;
    Result.Depth.x = Depth;
    Result.Depth.y = GetMetallic(TexCoord);

    return Result;
}