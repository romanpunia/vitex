#include "renderer/material"

StructuredBuffer<Material> Materials : register(t0);
Texture2D Channel0 : register(t1);
Texture2D Channel1 : register(t2);
Texture2D Channel2 : register(t3);
Texture2D Channel3 : register(t4);
SamplerState Sampler : register(s0);