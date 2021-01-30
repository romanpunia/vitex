float2 EncodeSC(float3 N)
{
    return (float2(atan2(N.y, N.x) / 3.1415926536, N.z) + 1.0) * 0.5;
}
float3 DecodeSC(float2 N)
{
    float2 V = N * 2.0 - 1.0, C1;
    sincos(V.x * 3.1415926536, C1.x, C1.y);

    float2 C2 = float2(sqrt(1.0 - V.y * V.y), V.y);
    return float3(C1.y * C2.x, C1.x * C2.x, C2.y);
}
float2 EncodeSM(float3 N)
{
    float2 R = normalize(N.xy) * (sqrt(-N.z * 0.5 + 0.5));
    return R * 0.5 + 0.5;
}
float3 DecodeSM(float2 N)
{
    float4 R = float4(N * 2, 0, 0) + float4(-1, -1, 1, -1);
    float L = dot(R.xyz, -R.xyw);
    R.xy *= sqrt(L);
    R.z = L;

    return R.xyz * 2 + float3(0, 0, -1);
}