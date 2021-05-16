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

StructuredBuffer<PointLight> PointLights : register(t5);