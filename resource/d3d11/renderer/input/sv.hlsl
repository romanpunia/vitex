#include "renderer/material"

StructuredBuffer<Material> Materials : register(t0);
Texture2D NormalMap : register(t1);
Texture2D DepthMap : register(t2);
Texture2D DiffuseMap : register(t3);
Texture2D PassMap : register(t4);
SamplerState Sampler : register(s0);