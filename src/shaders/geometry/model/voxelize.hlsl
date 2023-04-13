#include "std/layouts/vertex-instance.hlsl"
#include "std/channels/gvoxelizer.hlsl"
#include "std/buffers/object.hlsl"

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = Result.UV = GetVoxel(mul(float4(V.Position, 1.0), V.OB_World));
	Result.Normal = normalize(mul(V.Normal, (float3x3)V.OB_World));
	Result.Tangent = normalize(mul(V.Tangent, (float3x3)V.OB_World));
	Result.Bitangent = normalize(mul(V.Bitangent, (float3x3)V.OB_World));
	Result.TexCoord = V.TexCoord * V.OB_TexCoord.xy;
    Result.OB_Diffuse = V.OB_Material.x;
    Result.OB_Normal = V.OB_Material.y;
    Result.OB_MaterialId = V.OB_Material.w;

	return Result;
}

float GetAvg(float3 Value)
{
	return (Value.x + Value.y + Value.z) / 3.0;
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
	float4 Color = float4(Materials[V.OB_MaterialId].Diffuse, 1.0);
	[branch] if (V.OB_Diffuse > 0)
	{
		Color *= GetDiffuse(V.TexCoord);
		if (Color.w < 0.001)
			discard;
	}

	float3 Normal = V.Normal;
	[branch] if (V.OB_Normal > 0)
		Normal = GetNormal(V.TexCoord, V.Normal, V.Tangent, V.Bitangent);
	
	Compose(V.TexCoord, Color, Normal, V.UV.xyz, V.OB_MaterialId);
};