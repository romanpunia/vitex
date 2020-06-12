#include "renderer/material"

StructuredBuffer<Material> Materials : register(t0);
Texture2D DiffuseMap : register(t1);
Texture2D NormalMap : register(t2);
Texture2D MetallicMap : register(t3);
Texture2D HeightMap : register(t4);
Texture2D OcclusionMap : register(t5);
Texture2D EmissionMap : register(t6);
SamplerState Sampler : register(s0);