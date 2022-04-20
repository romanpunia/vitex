#include "std/layouts/interface.hlsl"
#include "std/channels/immediate.hlsl"
#include "std/buffers/object.hlsl"

VOutput vs_main(VInput V)
{
	VOutput Result;
	Result.Position = mul(float4(V.Position.xy, 0.0, 1.0), ob_Transform);
	Result.Color = V.Color;
	Result.TexCoord = V.TexCoord;
	Result.UV = Result.Position;

	return Result;
};

float4 ps_main(VOutput V) : SV_Target
{
	float4 Color = V.Color;
	[branch] if (ob_Diffuse > 0)
		Color *= GetDiffuse(V.TexCoord);

	return Color;
};