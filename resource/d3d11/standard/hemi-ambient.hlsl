float3 HemiAmbient(in float3 Up, in float3 Down, in float Height)
{
    float Intensity = Height * 0.5f + 0.5f;
    return Intensity * Up + Down;
}