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
	float Bias;
	float Iterations;
	float ScatterIntensity;
	float PlanetRadius;
	float AtmosphereRadius;
	float MieDirection;
	float Umbra;
	float Padding;
};

StructuredBuffer<LineLight> LineLights : register(t7);