#include "internal/layouts_skin.hlsl"
#include "internal/channels_gvoxelizer.hlsl"
#include "internal/buffers_object.hlsl"
#include "internal/buffers_animation.hlsl"

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.TexCoord = V.TexCoord * ob_TexCoord.xy;

	float4 Position = float4(V.Position, 1.0);
	[branch] if (ab_Animated > 0)
	{
		matrix Offset =
			mul(ab_Offsets[(int)V.Index.x], V.Bias.x) +
			mul(ab_Offsets[(int)V.Index.y], V.Bias.y) +
			mul(ab_Offsets[(int)V.Index.z], V.Bias.z) +
			mul(ab_Offsets[(int)V.Index.w], V.Bias.w);

		Position = mul(float4(V.Position, 1.0), Offset);
		Result.Position = Result.UV = GetVoxel(mul(Position, ob_World));
		Result.Normal = normalize(mul(mul(float4(V.Normal, 0), Offset).xyz, (float3x3)ob_World));
		Result.Tangent = normalize(mul(mul(float4(V.Tangent, 0), Offset).xyz, (float3x3)ob_World));
		Result.Bitangent = normalize(mul(mul(float4(V.Bitangent, 0), Offset).xyz, (float3x3)ob_World));   
	}
	else
	{
		Result.Position = Result.UV = GetVoxel(mul(Position, ob_World));
		Result.Normal = normalize(mul(V.Normal, (float3x3)ob_World));
		Result.Tangent = normalize(mul(V.Tangent, (float3x3)ob_World));
		Result.Bitangent = normalize(mul(V.Bitangent, (float3x3)ob_World));
	}

	return Result;
}

[maxvertexcount(3)]
void gs_main(triangle VOutput V[3], inout TriangleStream<VOutput> Stream)
{
	uint Dominant = GetVoxelDominant(V[0].Position.xyz, V[1].Position.xyz, V[2].Position.xyz); 
	[unroll] for (uint i = 0; i < 3; ++i)
	{
		VOutput Next = V[i];
		Next.Position = GetVoxelPosition(Next.Position, Dominant);
		Stream.Append(Next);
	}
	
    Stream.RestartStrip();
}

void ps_main(VOutput V)
{
	float4 Color = float4(Materials[ob_MaterialId].Diffuse, 1.0);
	[branch] if (ob_Diffuse > 0)
	{
		Color *= GetDiffuse(V.TexCoord);
		if (Color.w < 0.001)
			discard;
	}

	float3 Normal = V.Normal;
	[branch] if (ob_Normal > 0)
		Normal = GetNormal(V.TexCoord, V.Normal, V.Tangent, V.Bitangent);
	
	Compose(V.TexCoord, Color, Normal, V.UV.xyz, ob_MaterialId);
};