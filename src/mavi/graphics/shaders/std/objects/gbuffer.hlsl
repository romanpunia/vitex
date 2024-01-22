struct GBuffer
{
	float4 DiffuseBuffer : SV_TARGET0;
	float4 NormalBuffer : SV_TARGET1;
	float DepthBuffer : SV_TARGET2;
	float4 SurfaceBuffer : SV_TARGET3;
};