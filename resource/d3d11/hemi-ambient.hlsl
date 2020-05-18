float3 HemiAmbient(float3 Up, float3 Down, float Height)
{
    float Intensity = Height * 0.5f + 0.5f;
    return Intensity * Up + Down;
}