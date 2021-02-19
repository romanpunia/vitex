#include "std/layouts/interface"
#include "std/channels/immediate"
#include "std/buffers/object"

VOutput vs_main(VInput V)
{
	VOutput Result;
	Result.Position = mul(float4(V.Position.xy, 0.0, 1.0), WorldViewProjection);
	Result.Color = V.Color;
	Result.TexCoord = V.TexCoord;
    Result.UV = Result.Position;

	return Result;
};

float4 ps_main(VOutput V) : SV_Target
{
    float4 Color = V.Color;
    [branch] if (HasDiffuse > 0)
        Color *= GetDiffuse(V.TexCoord);

    return Color;
};