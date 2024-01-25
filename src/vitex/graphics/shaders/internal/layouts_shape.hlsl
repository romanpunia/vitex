struct VInput
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD0;
};

struct VOutput
{
	float4 Position : SV_POSITION;
	float4 TexCoord : TEXCOORD0;
};