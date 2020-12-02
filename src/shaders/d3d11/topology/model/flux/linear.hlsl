#include "renderer/vertex"
#include "renderer/buffer/object"
#include "workflow/rasterizer"

VertexResult90 VS(VertexBase V)
{
	VertexResult90 Result = (VertexResult90)0;
	Result.Position = Result.UV = mul(V.Position, WorldViewProjection);
	Result.TexCoord = V.TexCoord * TexCoord;

	return Result;
}

float4 PS(VertexResult90 V) : SV_TARGET0
{
    Material Mat = GetMaterial(MaterialId);
	float Alpha = (1.0 - Mat.Transparency);

	[branch] if (HasDiffuse > 0)
		Alpha *= GetDiffuse(V.TexCoord).w;

	return float4(V.UV.z / V.UV.w, 1.0 - Alpha, 1.0, 1.0);
};