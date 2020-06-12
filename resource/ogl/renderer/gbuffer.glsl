struct GBuffer
{
    float4 Diffuse : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float2 Depth : SV_TARGET2;
};