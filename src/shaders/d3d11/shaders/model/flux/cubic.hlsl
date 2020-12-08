#include "common/model/cubic"

GFlux PS(VOutputLinear V)
{
    float4 Diffuse = float4(1.0, 1.0, 1.0, 1.0);
    if (HasDiffuse > 0)
        Diffuse = GetDiffuse(V.TexCoord);

    Material Mat = GetMaterial(MaterialId);
    return Compose(Diffuse, Mat.Transparency, V.Normal, length(V.UV.xyz - ViewPosition) / FarPlane);
};