#include "common/model/linear"

GFlux PS(VOutputLinear V)
{
    float4 Diffuse = (HasDiffuse ? GetDiffuse(V.TexCoord) : float4(1.0, 1.0, 1.0, 1.0));
    return Compose(Diffuse, 1.0 - GetMaterial(MaterialId).Transparency, V.Normal, V.UV.z / V.UV.w);
};