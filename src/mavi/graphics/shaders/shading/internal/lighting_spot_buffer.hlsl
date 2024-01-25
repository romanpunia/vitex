cbuffer RenderConstant : register(b3)
{
	matrix LightTransform;
	matrix LightViewProjection;
	float4 Attenuation;
	float3 Direction;
	float Cutoff;
	float3 Position;
	float Range;
	float3 Lighting;
	float Softness;
	float Bias;
	float Iterations;
	float Umbra;
	float Padding;
};