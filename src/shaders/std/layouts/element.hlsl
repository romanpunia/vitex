struct VInput
{
	uint Position : SV_VERTEXID;
};

struct VOutput
{
	float4 Position : SV_POSITION;
	float4 Color : TEXCOORD0;
	float2 TexCoord : TEXCOORD1;
	float Scale : TEXCOORD2;
	float Rotation : TEXCOORD3;
};

struct VOutputOpaque
{
	float4 Position : SV_POSITION;
    float4 UV : TEXCOORD0;
	float4 Color : TEXCOORD1;
    float3 Normal : NORMAL0;
    float3 Tangent : TANGENT0;
    float3 Bitangent : BINORMAL0;
	float2 TexCoord : TEXCOORD2;
	float Scale : TEXCOORD3;
	float Rotation : TEXCOORD4;
};

struct VOutputCubic
{
	float4 Position : SV_POSITION;
	float4 UV : TEXCOORD0;
    float3 Normal : NORMAL0;
	float2 TexCoord : TEXCOORD1;
	float Rotation : TEXCOORD2;
	float Scale : TEXCOORD3;
	float Alpha : TEXCOORD4;
	uint RenderTarget : SV_RenderTargetArrayIndex;
};

struct VOutputLinear
{
	float4 Position : SV_POSITION;
	float4 UV : TEXCOORD0;
    float3 Normal : NORMAL0;
	float2 TexCoord : TEXCOORD1;
	float Rotation : TEXCOORD2;
	float Scale : TEXCOORD3;
	float Alpha : TEXCOORD4;
};