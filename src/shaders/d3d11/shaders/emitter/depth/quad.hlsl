#include "common/emitter/quad"

float4 PS(VOutputLinear V) : SV_TARGET0
{
    return length(V.UV.xyz - ViewPosition) / FarPlane;
};