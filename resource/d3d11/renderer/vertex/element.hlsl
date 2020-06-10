struct VertexBase
{
	uint Position : SV_VERTEXID;
};

struct VertexResult
{
	float4 Position : SV_POSITION;
	float4 Color : TEXCOORD0;
	float2 TexCoord : TEXCOORD2;
	float Scale : TEXCOORD3;
	float Rotation : TEXCOORD4;
};

struct VertexResult360
{
	float4 Position : SV_POSITION;
	float4 UV : TEXCOORD0;
	float2 TexCoord : TEXCOORD1;
	float Rotation : TEXCOORD2;
	float Scale : TEXCOORD3;
	float Alpha : TEXCOORD4;
	uint RenderTarget : SV_RenderTargetArrayIndex;
};

struct VertexResult90
{
	float4 Position : SV_POSITION;
	float4 UV : TEXCOORD0;
	float2 TexCoord : TEXCOORD1;
	float Rotation : TEXCOORD2;
	float Scale : TEXCOORD3;
	float Alpha : TEXCOORD4;
};