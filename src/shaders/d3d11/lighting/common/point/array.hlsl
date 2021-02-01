struct PointLight
{
	matrix LightWorldViewProjection;
    float4 Attenuation;
	float3 Position;
	float Range;
	float3 Lighting;
	float Distance;
    float Umbra;
	float Softness;
	float Bias;
	float Iterations;
};

cbuffer RenderConstant : register(b3)
{
    float3 LxPadding;
    float LightCount;
};

StructuredBuffer<PointLight> Lights : register(t5);