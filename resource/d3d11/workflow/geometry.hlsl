#include "workflow/rasterizer"
#include "renderer/gbuffer"

GBuffer Compose(in float2 TexCoord, in float4 Diffuse, in float3 Normal, in float Depth, in float MaterialId)
{
	GBuffer Result;
    Result.Channel0 = Diffuse;
    Result.Channel1.xyz = Normal;
    Result.Channel1.w = MaterialId;
    Result.Channel2.x = Depth;
    Result.Channel2.y = GetMetallic(TexCoord);
    Result.Channel3 = GetRoughness(TexCoord);

    return Result;
}