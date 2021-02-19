cbuffer Voxelizer : register(b3)
{
    matrix VxWorldViewProjection;
    float3 VxCenter;
    float VxStep;
    float3 VxSize;
    float VxMipLevels;
    float3 VxScale;
    float VxMaxSteps;
    float3 VxLights;
    float VxIntensity;
    float VxDistance;
    float VxOcclusion;
    float VxShadows;
    float VxBleeding;
};