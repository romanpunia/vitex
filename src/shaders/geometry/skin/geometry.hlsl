#include "std/layouts/skin.hlsl"
#include "std/channels/gbuffer.hlsl"
#include "std/buffers/object.hlsl"
#include "std/buffers/viewer.hlsl"
#include "std/buffers/animation.hlsl"

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
		Result.Position = Result.UV = mul(Position, ob_Transform);
		Result.Normal = normalize(mul(mul(float4(V.Normal, 0), Offset).xyz, (float3x3)ob_World));
		Result.Tangent = normalize(mul(mul(float4(V.Tangent, 0), Offset).xyz, (float3x3)ob_World));
		Result.Bitangent = normalize(mul(mul(float4(V.Bitangent, 0), Offset).xyz, (float3x3)ob_World));   
	}
	else
	{
		Result.Position = Result.UV = mul(Position, ob_Transform);
		Result.Normal = normalize(mul(V.Normal, (float3x3)ob_World));
		Result.Tangent = normalize(mul(V.Tangent, (float3x3)ob_World));
		Result.Bitangent = normalize(mul(V.Bitangent, (float3x3)ob_World));
	}

	[branch] if (ob_Height > 0)
		Result.Direction = GetDirection(Result.Tangent, Result.Bitangent, Result.Normal, mul(Position, ob_World), ob_TexCoord.xy);

	return Result;
}

GBuffer ps_main(VOutput V)
{
	Material Mat = Materials[ob_MaterialId];
	float2 Coord = V.TexCoord;

	[branch] if (ob_Height > 0)
		Coord = GetParallax(Coord, V.Direction, Mat.Height, Mat.Bias);
	
	float4 Color = float4(Mat.Diffuse, 1.0);
	[branch] if (ob_Diffuse > 0)
	{
		Color *= GetDiffuse(Coord);
		if (Color.w < 0.001)
			discard;
	}

	float3 Normal = V.Normal;
	[branch] if (ob_Normal > 0)
		Normal = GetNormal(Coord, V.Normal, V.Tangent, V.Bitangent);

	return Compose(Coord, Color, Normal, V.UV.z / V.UV.w, ob_MaterialId);
};