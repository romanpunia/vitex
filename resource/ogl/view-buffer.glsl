cbuffer ViewBuffer : register(b2)
{
    matrix InvViewProjection;
    matrix View;
    float4 ViewPosition;
};