Geometry LoadGeometry(matrix InvertViewProjection, float2 TexCoord)
{
    float2 Depth = Input3.SampleLevel(State, TexCoord.xy, 0).xy;
    float4 Position = mul(float4(TexCoord.x * 2.0f - 1.0f, 1.0f - TexCoord.y * 2.0f, Depth.x, 1.0f), InvertViewProjection);
    float4 Diffuse = Input1.SampleLevel(State, TexCoord.xy, 0);
    float4 Normal = Input2.SampleLevel(State, TexCoord.xy, 0);

    Geometry Result;
    Result.Diffuse = Diffuse.xyz;
    Result.Normal = Normal.xyz;
    Result.Position = Position.xyz / Position.w;
    Result.Surface = float2(Diffuse.w, Depth.y);
    Result.TexCoord = TexCoord.xy;
    Result.Material = Normal.w;

    return Result;
}

Geometry LoadGeometrySV(matrix InvertViewProjection, float4 UV)
{
    return LoadGeometry(InvertViewProjection, float2(0.5f + 0.5f * UV.x / UV.w, 0.5f - 0.5f * UV.y / UV.w));
}