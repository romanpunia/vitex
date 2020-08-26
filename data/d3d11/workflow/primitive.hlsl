Texture2D DiffuseMap : register(t0);
SamplerState Sampler : register(s0);

float4 GetDiffuse(in float2 TexCoord)
{
    return DiffuseMap.Sample(Sampler, TexCoord);
}
float4 GetDiffuseLevel(in float2 TexCoord, in float Level)
{
    return DiffuseMap.SampleLevel(Sampler, TexCoord, Level);
}