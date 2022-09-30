cbuffer Voxelizer : register(b3)
{
	matrix vxb_Transform;
	float3 vxb_Center;
	float vxb_Step;
	float3 vxb_Size;
	float vxb_Mips;
	float3 vxb_Scale;
	float vxb_MaxSteps;
	float3 vxb_Lights;
	float vxb_Radiance;
	float vxb_Margin;
	float vxb_Offset;
	float vxb_Angle;
	float vxb_Length;
	float vxb_Distance;
	float vxb_Occlusion;
	float vxb_Specular;
	float vxb_Bleeding;
};