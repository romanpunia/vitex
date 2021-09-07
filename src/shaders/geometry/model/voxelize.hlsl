#include "std/layouts/vertex.hlsl"
#include "std/channels/gvoxelizer.hlsl"
#include "std/buffers/object.hlsl"

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = Result.UV = GetVoxel(mul(float4(V.Position, 1.0), ob_World));
	Result.Normal = normalize(mul(V.Normal, (float3x3)ob_World));
	Result.Tangent = normalize(mul(V.Tangent, (float3x3)ob_World));
	Result.Bitangent = normalize(mul(V.Bitangent, (float3x3)ob_World));
	Result.TexCoord = V.TexCoord * ob_TexCoord.xy;

	return Result;
}

[maxvertexcount(3)]
void gs_main(triangle VOutput V[3], inout TriangleStream<VOutput> Stream)
{
	uint Dominant = GetDominant(V[0].Normal, V[1].Normal, V[2].Normal);
	[unroll] for (uint i = 0; i < 3; ++i)
		V[i].Position = GetVoxelSpace(V[i].Position, Dominant);
	
	ConvervativeRasterize(V[0].Position, V[1].Position, V[2].Position);
	[unroll] for (uint j = 0; j < 3; ++j)
		Stream.Append(V[j]);
}

void ps_main(VOutput V)
{
	float4 Color = float4(Materials[ob_Mid].Diffuse, 1.0);
	[branch] if (ob_Diffuse > 0)
		Color *= GetDiffuse(V.TexCoord);

	float3 Normal = V.Normal;
	[branch] if (ob_Normal > 0)
		Normal = GetNormal(V.TexCoord, V.Normal, V.Tangent, V.Bitangent);
	
	Compose(V.TexCoord, Color, Normal, V.UV.xyz, ob_Mid);
};