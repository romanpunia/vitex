#include "sdk/layouts/skin"
#include "sdk/channels/voxelizer"
#include "sdk/buffers/object"
#include "sdk/buffers/animation"

VOutput VS(VInput V)
{
    VOutput Result = (VOutput)0;
	Result.TexCoord = V.TexCoord * TexCoord;

    float4 Position = float4(V.Position, 1.0);
	[branch] if (HasAnimation > 0)
	{
		matrix Offset =
			mul(Offsets[(int)V.Index.x], V.Bias.x) +
			mul(Offsets[(int)V.Index.y], V.Bias.y) +
			mul(Offsets[(int)V.Index.z], V.Bias.z) +
			mul(Offsets[(int)V.Index.w], V.Bias.w);

        Position = mul(float4(V.Position, 1.0), Offset);
		Result.Position = Result.UV = GetVoxel(mul(Position, World));
		Result.Normal = normalize(mul(mul(float4(V.Normal, 0), Offset).xyz, (float3x3)World));
		Result.Tangent = normalize(mul(mul(float4(V.Tangent, 0), Offset).xyz, (float3x3)World));
		Result.Bitangent = normalize(mul(mul(float4(V.Bitangent, 0), Offset).xyz, (float3x3)World));   
    }
	else
	{
		Result.Position = Result.UV = GetVoxel(mul(Position, World));
		Result.Normal = normalize(mul(V.Normal, (float3x3)World));
		Result.Tangent = normalize(mul(V.Tangent, (float3x3)World));
		Result.Bitangent = normalize(mul(V.Bitangent, (float3x3)World));
	}

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