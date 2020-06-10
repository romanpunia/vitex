#include "geometry/forward"
#include "renderer/gbuffer"

GBuffer Compose(in float2 TexCoord, in float3 Diffuse, in float3 Normal, in float Depth, in float MaterialId)
{
    float2 MR = GetMetallicAndRoughness(TexCoord);

	GBuffer Result;
    Result.Diffuse.xyz = Diffuse;
    Result.Diffuse.w = MR.y;
    Result.Normal.xyz = Normal;
    Result.Normal.w = MaterialId;
    Result.Depth.x = Depth;
    Result.Depth.y = MR.x;

    return Result;
}