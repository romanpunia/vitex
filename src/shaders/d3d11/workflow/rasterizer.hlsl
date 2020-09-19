#include "renderer/input/base"
#include "renderer/buffer/viewer"
#pragma warning(disable: 4000)

float GetRoughness(in float2 TexCoord)
{
    return RoughnessMap.Sample(Sampler, TexCoord).x;
}
float GetMetallic(in float2 TexCoord)
{
    return MetallicMap.Sample(Sampler, TexCoord).x;
}
float GetOcclusion(in float2 TexCoord)
{
    return OcclusionMap.Sample(Sampler, TexCoord).x;
}
float GetEmission(in float2 TexCoord)
{
    return EmissionMap.Sample(Sampler, TexCoord).x;
}
float2 GetParallax(in float2 TexCoord, in float3 Direction, in float Amount, in float Bias)
{
    float Steps = lerp(24.0, 2.0, pow(1.0 - abs(dot(float3(0.0, 0.0, 1.0), Direction)), 4));
    float Step = 1.0 / Steps;
    float Depth = 0.0;
    float2 Delta = Direction.xy * Amount / Steps;
    float2 Result = TexCoord;
    float Sample = HeightMap.SampleLevel(Sampler, Result, 0).x + Bias;
    
    [loop] for (float i = 0; i < Steps; i++)
    {
        [branch] if (Depth >= Sample)
            break;

        Result -= Delta;
        Sample = HeightMap.SampleLevel(Sampler, Result, 0).x + Bias;  
        Depth += Step;
    }

    float2 Origin = Result + Delta;
    float Depth1 = Sample - Depth;
    float Depth2 = HeightMap.SampleLevel(Sampler, Origin, 0).x + Bias - Depth + Step;
    float Weight = Depth1 / (Depth1 - Depth2);
    
    return Origin * Weight + Result * (1.0 - Weight);
}
float3 GetNormal(in float2 TexCoord, in float3 Normal, in float3 Tangent, in float3 Bitangent)
{
    float3 Result = NormalMap.Sample(Sampler, TexCoord).xyz * 2.0 - 1.0;
    return normalize(Result.x * Tangent + Result.y * Bitangent + Result.z * Normal);
}
float4 GetDiffuse(in float2 TexCoord)
{
    return DiffuseMap.Sample(Sampler, TexCoord);
}
float4 GetDiffuseLevel(in float2 TexCoord, in float Level)
{
    return DiffuseMap.SampleLevel(Sampler, TexCoord, Level);
}
float4 GetSample(in Texture2D Texture, in float2 TexCoord)
{
    return Texture.Sample(Sampler, TexCoord);
}
float4 GetSampleLevel(in Texture2D Texture, in float2 TexCoord, in float Level)
{
    return Texture.SampleLevel(Sampler, TexCoord, Level);
}
Material GetMaterial(in float Material_Id)
{
    return Materials[Material_Id];
}