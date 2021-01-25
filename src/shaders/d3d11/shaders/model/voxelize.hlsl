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
        V[i].Position = GetVoxelSpace(V[i].Position, Dominant);
             
    float2 Side0 = normalize(V[1].Position.xy - V[0].Position.xy);
    float2 Side1 = normalize(V[2].Position.xy - V[1].Position.xy);
    float2 Side2 = normalize(V[0].Position.xy - V[2].Position.xy);
    V[0].Position.xy += normalize(Side2 - Side0) / GridSize.xy;
    V[1].Position.xy += normalize(Side0 - Side1) / GridSize.xy;
    V[2].Position.xy += normalize(Side1 - Side2) / GridSize.xy;

	[unroll] for (uint j = 0; j < 3; ++j)
        Stream.Append(V[j]);
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