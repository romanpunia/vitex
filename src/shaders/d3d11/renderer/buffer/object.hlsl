cbuffer Object : register(b1)
{
    matrix WorldViewProjection;
    matrix World;
    float3 Diffuse;
    float HasDiffuse;
    float2 TexCoord;
    float HasNormal;
    float MaterialId;
    float HasHeight;
    float HeightAmount;
    float HeightBias;
    float Alignment;
};