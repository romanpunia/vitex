cbuffer Voxelizer : register(b3)
{
    float3 VxCenter;
    float VxStep;
    float3 VxSize;
    float VxMipLevels;
    float3 VxScale;
    float VxMaxSteps;
    float VxIntensity;
    float VxOcclusion;
    float VxShadows;
    float VxLights;
};