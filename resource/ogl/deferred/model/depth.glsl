#include "renderer/vertex"
#include "renderer/buffer/object"
#include "geometry/forward"

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

	float Alpha = Mat.Transparency.x;
	[branch] if (HasDiffuse > 0)
		Alpha *= GetDiffuse(V.TexCoord).w;

	[branch] if (Alpha < Mat.Transparency.y)
		discard;

	return float4(V.UV.z / V.UV.w, Alpha, 1.0f, 1.0f);
};