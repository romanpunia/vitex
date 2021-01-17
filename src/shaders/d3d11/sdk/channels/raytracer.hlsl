#include "sdk/objects/material"

StructuredBuffer<Material> Materials : register(t0);
RWTexture3D<float4> DiffuseLumina : register(u1);
RWTexture3D<float4> NormalLumina : register(u2);
RWTexture3D<float4> SurfaceLumina : register(u3);