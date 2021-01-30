#include "geometry/model/common/linear"

float PS(VOutputLinear V) : SV_TARGET0
{
    float Threshold = (HasDiffuse ? 1.0 - GetDiffuse(V.TexCoord).w : 1.0) * GetMaterial(MaterialId).Transparency;
    [branch] if (Threshold > 0.5)
        discard;
    
    return V.UV.z / V.UV.w;
};