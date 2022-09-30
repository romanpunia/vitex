struct LineLight
{
	matrix LightViewProjection[6];
	matrix SkyOffset;
	float3 RlhEmission;
	float RlhHeight;
	float3 MieEmission;
	float MieHeight;
	float3 Lighting;
	float Softness;
	float3 Position;
	float Cascades;
	float2 Padding;
	float Bias;
	float Iterations;
	float ScatterIntensity;
	float PlanetRadius;
	float AtmosphereRadius;
	float MieDirection;
};

StructuredBuffer<LineLight> LineLights : register(t7);