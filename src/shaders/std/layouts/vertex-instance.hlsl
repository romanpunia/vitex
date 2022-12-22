struct VInput
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD0;
	float3 Normal : NORMAL0;
	float3 Tangent : TANGENT0;
	float3 Bitangent : BINORMAL0;
    matrix OB_Transform : OB_TRANSFORM;
    matrix OB_World : OB_WORLD;
    float2 OB_TexCoord : OB_TEXCOORD;
	float4 OB_Material : OB_MATERIAL;
};

struct VOutput
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
	float3 Normal : NORMAL0;
	float3 Tangent : TANGENT0;
	float3 Bitangent : BINORMAL0;
	float4 UV : TEXCOORD1;
	float3 Direction : TEXCOORD2;
    float OB_Diffuse : TEXCOORD3;
    float OB_Normal : TEXCOORD4;
    float OB_Height : TEXCOORD5;
    uint OB_MaterialId : TEXCOORD6;
};

struct VOutputCubic
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
	float4 UV : TEXCOORD1;
    float OB_Diffuse : TEXCOORD2;
    uint OB_MaterialId : TEXCOORD3;
	uint RenderTarget : SV_RenderTargetArrayIndex;
};

struct VOutputLinear
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
	float4 UV : TEXCOORD1;
    float OB_Diffuse : TEXCOORD2;
    uint OB_MaterialId : TEXCOORD3;
};