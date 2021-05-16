cbuffer Viewer : register(b2)
{
    matrix vb_InvViewProj;
    matrix vb_ViewProj;
    matrix vb_Proj;
    matrix vb_View;
    float3 vb_Position;
    float vb_Far;
    float3 vb_Direction;
    float vb_Near;
};