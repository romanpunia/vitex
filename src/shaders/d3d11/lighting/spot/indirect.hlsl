#include "std/channels/raytracer"
#include "std/core/lighting"
#include "lighting/common/spot/buffer"

[numthreads(8, 8, 8)]
void CS(uint3 Thread : SV_DispatchThreadID)
{
};