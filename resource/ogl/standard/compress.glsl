float2 EncodeSC(in float3 N)
{
    return float2(atan2(N.y, N.x) / 3.1415926536, N.z) + 1.0) * 0.5;
}
float3 DecodeSC(in float2 N)
{
    float2 V = N * 2.0 - 1.0, C1;
    sincos(V.x * 3.1415926536, C1.x, C1.y);

    float2 C2 = float2(sqrt(1.0 - V.y * V.y), V.y);
    return float3(C1.y * C2.x, C1.x * C2.x, C2.y);
}
float2 EncodeSM(in float3 N)
{
    float P = sqrt(N.z * 8.0 + 8.0);
    return N.xy / P + 0.5;
}
float3 DecodeSM(in float2 N)
{
    float2 V = N * 4.0 - 2.0;
    float F = dot(N, N);
    float G = sqrt(1.0 - F / 4.0);
    
    return float3(N * G, 1.0 - F / 2.0);
}