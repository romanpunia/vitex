#include "sdk/layouts/vertex"
#include "sdk/channels/voxelizer"
#include "sdk/buffers/object"

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
    Result.Position = Result.UV = GetVoxel(mul(float4(V.Position, 1.0), World));
	Result.Normal = normalize(mul(V.Normal, (float3x3)World));
	Result.Tangent = normalize(mul(V.Tangent, (float3x3)World));
	Result.Bitangent = normalize(mul(V.Bitangent, (float3x3)World));
	Result.TexCoord = V.TexCoord * TexCoord;

	return Result;
}

[maxvertexcount(3)]
void GS(triangle VOutput V[3], inout TriangleStream<VOutput> Stream)
{
	uint Dominant = GetDominant(V[0].Normal, V[1].Normal, V[2].Normal);
	[unroll] for (uint i = 0; i < 3; ++i)
	{
		VOutput Result = V[i];
        Result.Position = GetVoxelSpace(Result.Position, Dominant);
		Stream.Append(Result);
	}
}

Lumina PS(VOutput V)
{
	float4 Color = float4(Diffuse, 1.0);
	[branch] if (HasDiffuse > 0)
		Color *= GetDiffuse(V.TexCoord);

	float3 Normal = V.Normal;
	[branch] if (HasNormal > 0)
        Normal = GetNormal(V.TexCoord, V.Normal, V.Tangent, V.Bitangent);
    
    return Compose(V.TexCoord, Color, Normal, V.UV.xyz, MaterialId);
};