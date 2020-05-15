#include "shape-vertex-in.glsl"
#include "shape-vertex-out.glsl"
#include "render-buffer.glsl"

Texture2D Input1 : register(t0);
SamplerState State : register(s0);

ShapeVertexOut VS(ShapeVertexIn Input)
{
    ShapeVertexOut Output = (ShapeVertexOut)0;
    Output.Position = mul(Input.Position, WorldViewProjection);
    Output.TexCoord.xy = Input.TexCoord;
    return Output;
}

float4 PS(ShapeVertexOut Input) : SV_TARGET0
{
    float4 Texture = Input1.Sample(State, Input.TexCoord.xy);
    return float4(Texture.xyz * Diffusion, Texture.w);
};