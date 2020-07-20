#include "workflow/rasterizer"
#include "renderer/gbuffer"
#include "standard/compress"

GBuffer Compose(in float2 TexCoord, in float3 Diffuse, in float3 Normal, in float Depth, in float MaterialId)
{
	GBuffer Result;
    Result.Diffuse.xyz = Diffuse;
    Result.Diffuse.w = 1.0;
    Result.Normal.xy = EncodeSM(Normal);
    Result.Normal.z = GetRoughness(TexCoord);
    Result.Normal.w = MaterialId;
    Result.Depth.x = Depth;
    Result.Depth.y = GetMetallic(TexCoord);

    return Result;
}
GBuffer ComposeLimpid(in float2 TexCoord, in float4 Diffuse, in float3 Normal, in float Depth, in float MaterialId)
{
	GBuffer Result;
    Result.Diffuse = Diffuse;
    Result.Normal.xy = EncodeSM(Normal);
    Result.Normal.z = GetRoughness(TexCoord);
    Result.Normal.w = MaterialId;
    Result.Depth.x = Depth;
    Result.Depth.y = GetMetallic(TexCoord);

    return Result;
}