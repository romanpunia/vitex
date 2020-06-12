Texture2D DiffuseMap : register(t0);
SamplerState Sampler : register(s0);

float4 GetDiffuse(in float2 TexCoord)
{
    return DiffuseMap.Sample(Sampler, TexCoord);
}