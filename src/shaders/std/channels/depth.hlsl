#include "std/objects/material.hlsl"
#pragma warning(disable: 4000)

StructuredBuffer<Material> Materials : register(t0);
Texture2D DiffuseMap : register(t1);
SamplerState Sampler : register(s0);

float4 GetDiffuse(float2 TexCoord)
{
    return DiffuseMap.Sample(Sampler, TexCoord);
}