float2 LoadTexCoord(float4 RawPosition)
{
    float X = 0.5f + 0.5f * RawPosition.x / RawPosition.w;
    float Y = 0.5f - 0.5f * RawPosition.y / RawPosition.w;
    return float2(X, Y);
}