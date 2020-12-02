cbuffer Viewer : register(b2)
{
    matrix InvViewProjection;
    matrix ViewProjection;
    matrix Projection;
    matrix View;
    float3 ViewPosition;
    float FarPlane;
    float3 ViewDirection;
    float NearPlane;
};