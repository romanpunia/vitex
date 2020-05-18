struct InfluenceVertexIn
{
    float4 Position : POSITION0;
    float2 TexCoord : TEXCOORD0;
    float3 Normal : NORMAL0;
    float3 Tangent : TANGENT0;
    float3 Bitangent : BINORMAL0;
    float4 Index : JOINTBIAS0;
    float4 Bias : JOINTBIAS1;
};