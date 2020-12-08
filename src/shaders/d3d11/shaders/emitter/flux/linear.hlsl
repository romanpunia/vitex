#include "common/emitter/linear"

GFlux PS(VOutputLinear V)
{
    float4 Diffuse = float4(1.0, 1.0, 1.0, 1.0);
    if (HasDiffuse > 0)
        Diffuse = GetDiffuse(V.TexCoord);

    Material Mat = GetMaterial(MaterialId);
    return Compose(Diffuse, Mat.Transparency, V.Normal, V.UV.z / V.UV.w);
};