float3 CookTorrance(float3 V, float3 L, float3 N, float3 M, float G, out float3 O)
{
    float3 R = normalize(V + L);
    float RV = 1.0 - dot(R, V);
    float NV = dot(N, V);
    float NL = dot(N, L);
    float NR = dot(N, R);
    float VT = NV * NV;
    float LT = NL * NL;
    float VL = 1.0 + sqrt(1.0 - G + G / VT);
    float3 F = M + (1.0 - M) * RV * RV * RV * RV * RV;

    NR = NR * NR * (G - 1.0) + 1.0f;
    VL = VL + VL * sqrt(1.0 - G + G / LT);
    VT = G / (3.141592 * NR * NR);
    O = F * VT / VL * NV;

    return max(0.0, (1.0f - F) * NL / 3.141592 + O);
}