struct Material
{
    float3 Emission;
    float Intensity;
    float3 Metallic;
    float Micrometal;
    float Microrough;
    float Roughness;
    float Transmittance;
    float Shadowness;
    float Reflectance;
    float Reflection;
    float Occlusion;
    float Radius;
    float Self;
    float Padding1;
    float Padding2;
    float Padding3;
};

struct Geometry
{
    float3 Diffuse;
    float3 Normal;
    float3 Position;
    float2 Surface;
    float2 TexCoord;
    float Material;
};

StructuredBuffer<Material> Materials : register(t0);
Texture2D Input2 : register(t1);
Texture2D Input3 : register(t2);
Texture2D Input1 : register(t3);
SamplerState State : register(s0);