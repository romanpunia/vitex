#include "geometry/skin/common/linear.hlsl"

float ps_main(VOutputLinear V) : SV_TARGET0
{
    float Threshold = (ob_Diffuse ? 1.0 - GetDiffuse(V.TexCoord).w : 1.0) * Materials[ob_Mid].Transparency;
    [branch] if (Threshold > 0.5)
        discard;
    
    return V.UV.z / V.UV.w;
};