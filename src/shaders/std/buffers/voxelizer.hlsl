cbuffer Voxelizer : register(b3)
{
	matrix vxb_WorldViewProj;
	float3 vxb_Center;
	float vxb_Step;
	float3 vxb_Size;
	float vxb_Mips;
	float3 vxb_Scale;
	float vxb_MaxSteps;
	float3 vxb_Lights;
	float vxb_Intensity;
	float vxb_Distance;
	float vxb_Occlusion;
	float vxb_Shadows;
	float vxb_Bleeding;
};