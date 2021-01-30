#include "std/channels/raytracer"
#include "std/core/lighting"
#include "lighting/common/line/buffer"

StructuredBuffer<LineLight> Lights : register(t5);

[numthreads(8, 8, 8)]
void CS(uint3 Thread : SV_DispatchThreadID)
{
    
    

};