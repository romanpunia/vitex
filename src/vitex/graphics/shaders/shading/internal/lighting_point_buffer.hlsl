cbuffer RenderConstant : register(b3)
{
	matrix LightTransform;
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