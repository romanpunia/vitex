float GetRangeAttenuation(float3 L, float Distance)
{
    return max(1.0 - length(L) / Distance, 0.0);
}
float GetConeAttenuation(float3 L, float3 LN, float Distance, float3 Axis, float Angle)
{
    return 3.0 * GetRangeAttenuation(L, Distance) * max(1.0 - (1.0 - dot(-LN, Axis)) * 1.0 / (1.0 - Angle), 0.0);
}
float GetSubsurface(float3 N, float3 V, float3 Origin, float3 Scatter)
{
    return saturate(dot(-N, Origin)) * pow(saturate(dot(V, -normalize(Origin + N * Scatter.x))), Scatter.y) * Scatter.z;
}
float3 GetFresnelSchlick(float NdotV, float3 Metallic)
{
    return Metallic + (1.0 - Metallic) * pow(1.0 - NdotV, 5.0);
}
float3 GetBaseReflectivity(float3 Albedo, float3 Metallic)
{
    return lerp(float3(0.04, 0.04, 0.04), Albedo, Metallic);
}
float GetDistributionGGX(float NdotH, float Roughness)
{
    float A = Roughness * Roughness;
    float A2 = A * A;
    float Denom = (NdotH * NdotH * (A2 - 1.0) + 1.0);
    Denom = 3.141592 * Denom * Denom;
	
    return A2 / max(Denom, 0.00001);
}
float GetGeometrySmith(float NdotV, float NdotL, float Roughness)
{
    float R = (Roughness + 1.0);
    float K = (R * R) / 8.0;
    float GGX1 = NdotV / (NdotV * (1.0 - K) + K);
    float GGX2 = NdotL / (NdotL * (1.0 - K) + K);
    
    return GGX1 * GGX2;
}
float3 GetCookTorranceBRDF(float3 N, float3 V, float3 L, float3 Albedo, float3 Metallic, float Roughness)
{
    float3 H = normalize(V + L);
    float NdotV = max(dot(N, V), 0.00001);
    float NdotL = max(dot(N, L), 0.00001);
    float HdotV = max(dot(H, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float D = GetDistributionGGX(NdotH, Roughness);
    float G = GetGeometrySmith(NdotV, NdotL, Roughness);
    float3 M = GetBaseReflectivity(Albedo, Metallic);
    float3 F = GetFresnelSchlick(HdotV, M);
    float3 R = D * G * F;
    float3 K = 1.0 - F;

    K *= 1.0 - M;
    R /= 4.0 * NdotV * NdotL;

    return (K * Albedo / 3.141592 + R) * NdotL;
}
float3 GetSpecularBRDF(float3 N, float3 V, float3 L, float3 Albedo, float3 Metallic, float Roughness)
{
    float NdotV = max(dot(N, V), 0.00001);
    float NdotL = max(dot(N, L), 0.00001);
    float HdotV = max(dot(normalize(V + L), V), 0.0);
    float G = GetGeometrySmith(NdotV, NdotL, Roughness);
    float3 M = GetBaseReflectivity(Albedo, Metallic);
    float3 F = GetFresnelSchlick(HdotV, M);
    
    return G * F;
}