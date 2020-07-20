float RandomFloat(in float3 Value)
{
    return sin(dot(sin(Value), float3(12.9898, 78.233, 37.719))) * 143758.5453;
}
float RandomFloatXY(float X, float Y)
{
    return frac(sin(dot(float2(X, Y), float2(12.9898f, 78.233f))) * 43758.5453f);
}
float2 RandomFloat2(in float2 Value)
{
	float X = saturate(frac(sin(dot(Value, float2(12.9898f, 78.233f))) * 43758.5453f)) * 2.0 - 1.0;
	float Y = saturate(frac(sin(dot(Value, float2(12.9898f, 78.233f) * 2.0)) * 43758.5453f)) * 2.0 - 1.0;
	return float2(X, Y);
}