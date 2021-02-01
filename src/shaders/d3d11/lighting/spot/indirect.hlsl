#include "std/channels/voxelizer"
#include "std/core/lighting"
#include "lighting/common/spot/array"

[numthreads(8, 8, 8)]
void CS(uint3 Voxel : SV_DispatchThreadID)
{
};