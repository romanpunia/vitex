cbuffer Voxelizer : register(b3)
{
    float3 VxCenter;
    float VxStep;
    float3 VxSize;
    float VxMipLevels;
    float3 VxScale;
    float VxMaxSteps;
    float3 VxLights;
    float VxIntensity;
    float2 VxPadding;
    float VxOcclusion;
    float VxShadows;
};