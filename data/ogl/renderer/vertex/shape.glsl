struct VertexBase
{
    float4 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct VertexResult
{
    float4 Position : SV_POSITION;
    float4 TexCoord : TEXCOORD0;
};