float RandomFloat2(float X, float Y)
{
    return frac(sin(dot(float2(X, Y), float2(12.9898f, 78.233f))) * 43758.5453f);
}