cbuffer RenderBuffer : register(b1)
{
    matrix WorldViewProjection;
    matrix World;
    float3 Diffusion;
    float SurfaceDiffuse;
    float2 TexCoord;
    float SurfaceNormal;
    float Material;
};