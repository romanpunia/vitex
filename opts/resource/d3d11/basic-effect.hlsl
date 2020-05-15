#include <shape-vertex-in.hlsl>
#include <shape-vertex-out.hlsl>
#include <render-buffer.hlsl>

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