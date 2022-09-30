
float4 GetSample3(TextureCube Texture, float3 TexCoord)
{
	return Texture.Sample(Sampler, TexCoord);
}
float4 GetSample3Level(TextureCube Texture, float3 TexCoord, float Level)
{
	return Texture.SampleLevel(Sampler, TexCoord, Level);
}