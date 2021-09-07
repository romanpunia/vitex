#include "std/layouts/vertex.hlsl"
#include "std/channels/gbuffer.hlsl"
#include "std/buffers/object.hlsl"
#include "std/buffers/viewer.hlsl"

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = Result.UV = mul(float4(V.Position, 1.0), ob_WorldViewProj);
	Result.Normal = normalize(mul(V.Normal, (float3x3)ob_World));
	Result.Tangent = normalize(mul(V.Tangent, (float3x3)ob_World));
	Result.Bitangent = normalize(mul(V.Bitangent, (float3x3)ob_World));
	Result.TexCoord = V.TexCoord * ob_TexCoord.xy;

	[branch] if (ob_Height > 0)
		Result.Direction = GetDirection(Result.Tangent, Result.Bitangent, Result.Normal, mul(float4(V.Position, 1.0), ob_World), ob_TexCoord.xy);

	return Result;
}

GBuffer ps_main(VOutput V)
{
	Material Mat = Materials[ob_Mid];
	float2 Coord = V.TexCoord;

	[branch] if (ob_Height > 0)
		Coord = GetParallax(Coord, V.Direction, Mat.Height, Mat.Bias);
	
	float4 Color = float4(Mat.Diffuse, 1.0);
	[branch] if (ob_Diffuse > 0)
		Color *= GetDiffuse(Coord);

	float3 Normal = V.Normal;
	[branch] if (ob_Normal > 0)
		Normal = GetNormal(Coord, V.Normal, V.Tangent, V.Bitangent);
	
	return Compose(Coord, Color, Normal, V.UV.z / V.UV.w, ob_Mid);
};