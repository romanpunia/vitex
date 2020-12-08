#include "sdk/channels/geometry"
#include "sdk/objects/gbuffer"

GBuffer Compose(float2 TexCoord, float4 Diffuse, float3 Normal, float Depth, float MaterialId)
{
	GBuffer Result;
    Result.Channel0 = Diffuse;
    Result.Channel1.xyz = Normal;
    Result.Channel1.w = MaterialId;
    Result.Channel2 = Depth;
    Result.Channel3.x = RoughnessMap.Sample(Sampler, TexCoord).x;
    Result.Channel3.y = MetallicMap.Sample(Sampler, TexCoord).x;
    Result.Channel3.z = OcclusionMap.Sample(Sampler, TexCoord).x;
    Result.Channel3.w = EmissionMap.Sample(Sampler, TexCoord).x;

    return Result;
}
GDepthless ComposeDepthless(float2 TexCoord, float4 Diffuse, float3 Normal, float MaterialId)
{
	GDepthless Result;
    Result.Channel0 = Diffuse;
    Result.Channel1.xyz = Normal;
    Result.Channel1.w = MaterialId;
    Result.Channel3.x = RoughnessMap.Sample(Sampler, TexCoord).x;
    Result.Channel3.y = MetallicMap.Sample(Sampler, TexCoord).x;
    Result.Channel3.z = OcclusionMap.Sample(Sampler, TexCoord).x;
    Result.Channel3.w = EmissionMap.Sample(Sampler, TexCoord).x;

    return Result;
}