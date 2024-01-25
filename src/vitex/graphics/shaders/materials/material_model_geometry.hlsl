#include "internal/layouts_vertex_instance.hlsl"
#include "internal/channels_gbuffer.hlsl"
#include "internal/buffers_object.hlsl"
#include "internal/buffers_viewer.hlsl"

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = Result.UV = mul(float4(V.Position, 1.0), V.OB_Transform);
	Result.Normal = normalize(mul(V.Normal, (float3x3)V.OB_World));
	Result.Tangent = normalize(mul(V.Tangent, (float3x3)V.OB_World));
	Result.Bitangent = normalize(mul(V.Bitangent, (float3x3)V.OB_World));
	Result.TexCoord = V.TexCoord * V.OB_TexCoord;
    Result.OB_Diffuse = V.OB_Material.x;
    Result.OB_Normal = V.OB_Material.y;
    Result.OB_Height = V.OB_Material.z;
    Result.OB_MaterialId = V.OB_Material.w;
    
	[branch] if (Result.OB_Height > 0)
		Result.Direction = GetDirection(Result.Tangent, Result.Bitangent, Result.Normal, mul(float4(V.Position, 1.0), ob_World), ob_TexCoord.xy);

	return Result;
}

GBuffer ps_main(VOutput V)
{
	Material Mat = Materials[V.OB_MaterialId];
	float2 Coord = V.TexCoord;

	[branch] if (V.OB_Height > 0)
		Coord = GetParallax(Coord, V.Direction, Mat.Height, Mat.Bias);
	
	float4 Color = float4(Mat.Diffuse, 1.0);
	[branch] if (V.OB_Diffuse > 0)
	{
		Color *= GetDiffuse(Coord);
		if (Color.w < 0.001)
			discard;
	}
	
	float3 Normal = V.Normal;
	[branch] if (V.OB_Normal > 0)
		Normal = GetNormal(Coord, V.Normal, V.Tangent, V.Bitangent);
	
	return Compose(Coord, Color, Normal, V.UV.z / V.UV.w, V.OB_MaterialId);
};