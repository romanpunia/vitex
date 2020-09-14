float3 GetLight(in float3 P, in float3 L, in float3 N, in float3 M, in float G, out float3 O)
{
    float3 R = normalize(P + L);
    float RV = 1.0 - dot(R, P);
    float NV = dot(N, P);
    float NL = dot(N, L);
    float NR = dot(N, R);
    float VT = NV * NV;
    float LT = NL * NL;
    float VL = 1.0 + sqrt(1.0 - G + G / VT);
    float I = max(NL, 0.0);
    float3 F = M + (1.0 - M) * RV * RV * RV * RV * RV;

    NR = NR * NR * (G - 1.0) + 1.0;
    VL = VL + VL * sqrt(1.0 - G + G / LT);
    VT = G / (3.141592 * NR * NR); 
    O = I * F * VT / VL * NV;
    
    return max(0.0, I * (1.0 - F) * NL / 3.141592);
}
float3 GetReflection(in float3 P, in float3 L, in float3 N, in float3 M, in float G)
{
    float3 R = normalize(P + L);
    float RV = 1.0 - dot(R, P);
    float NV = dot(N, P);
    float NL = dot(N, L);
    float NR = dot(N, R);
    float VT = NV * NV;
    float LT = NL * NL;
    float VL = 1.0 + sqrt(1.0 - G + G / VT);
    float3 F = M + (1.0 - M) * RV * RV * RV * RV * RV;

    NR = NR * NR * (G - 1.0) + 1.0;
    VL = VL + VL * sqrt(1.0 - G + G / LT);
    VT = G / (3.141592 * NR * NR);
    
    float3 O = F * VT / VL * NV;
    return max(0.0, F + O);
}
float GetFresnel(in float3 E, in float3 N)
{
	return pow(1.0 + dot(E, N), 3.0);
}