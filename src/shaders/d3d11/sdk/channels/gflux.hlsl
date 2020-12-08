#include "sdk/channels/geometry"
#include "sdk/objects/gflux"

GFlux Compose(float4 Diffuse, float Alpha, float3 Normal, float Depth)
{
	GFlux Result;
    Result.Channel0 = Depth;
    Result.Channel1.xyz = Diffuse.xyz;
    Result.Channel1.w = 1.0 - Diffuse.w + Alpha * Diffuse.w;
    Result.Channel2.xyz = Normal;
    Result.Channel2.w = 1.0;

    return Result;
}