cbuffer Voxelizer : register(b3)
{
    float3 GridCenter;
    float GridStep;
    float3 GridSize;
    float GridMipLevels;
    float4 GridScale;
};