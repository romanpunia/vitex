#include "geometry/emitter/common/quad"

float4 ps_main(VOutputLinear V) : SV_TARGET0
{
    float Threshold = (1.0 - V.Alpha) * (ob_Diffuse ? 1.0 - GetDiffuse(V.TexCoord).w : 1.0) * Materials[ob_Mid].Transparency;
    [branch] if (Threshold > 0.5)
        discard;
    
    return length(V.UV.xyz - vb_Position) / vb_Far;
};