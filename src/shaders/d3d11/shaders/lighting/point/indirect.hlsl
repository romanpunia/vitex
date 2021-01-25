#include "sdk/channels/raytracer"
#include "sdk/core/lighting"
#include "common/lighting/point/buffer"

[numthreads(8, 8, 8)]
void CS(uint3 Thread : SV_DispatchThreadID)
{
};