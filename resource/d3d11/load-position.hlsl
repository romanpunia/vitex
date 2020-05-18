float3 LoadPosition(float2 TexCoord, float Depth, matrix InvViewProjection)
{
    float4 Position = float4(TexCoord.x * 2.0f - 1.0f, 1.0f - TexCoord.y * 2.0f, Depth, 1.0f);
    Position = mul(Position, InvViewProjection);
    return Position.xyz / Position.w;
}