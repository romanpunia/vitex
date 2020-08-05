struct VertexBase
{
    float4 Position : POSITION0;
    float2 TexCoord : TEXCOORD0;
    float3 Normal : NORMAL0;
    float3 Tangent : TANGENT0;
    float3 Bitangent : BINORMAL0;
    float4 Index : JOINTBIAS0;
    float4 Bias : JOINTBIAS1;
};

struct VertexResult
{
    float4 Position : SV_POSITION0;
    float2 TexCoord : TEXCOORD0;
    float3 Normal : NORMAL0;
    float3 Tangent : TANGENT0;
    float3 Bitangent : BINORMAL0;
    float4 UV : TEXCOORD1;
    float2 Direction : TEXCOORD2;
};

struct VertexResult360
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
	float4 UV : TEXCOORD1;
	uint RenderTarget : SV_RenderTargetArrayIndex;
};

struct VertexResult90
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
	float4 UV : TEXCOORD1;
};