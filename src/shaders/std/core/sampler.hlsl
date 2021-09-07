float GetAntibleed(float TexCoord)
{
	return smoothstep(0.0, 0.05, TexCoord) * (1.0 - smoothstep(0.95, 1.0, TexCoord));
}
float4 GetSample(Texture2D Texture, float2 TexCoord)
{
	return Texture.Sample(Sampler, TexCoord);
}
float4 GetSampleLevel(Texture2D Texture, float2 TexCoord, float Level)
{
	return Texture.SampleLevel(Sampler, TexCoord, Level);
}
float4 GetSample3(TextureCube Texture, float3 TexCoord)
{
	return Texture.Sample(Sampler, TexCoord);
}
float4 GetSample3Level(TextureCube Texture, float3 TexCoord, float Level)
{
	return Texture.SampleLevel(Sampler, TexCoord, Level);
}