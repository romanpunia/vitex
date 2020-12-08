#include "common/skin/cubic"

float PS(VOutputLinear V) : SV_TARGET0
{
    return length(V.UV.xyz - ViewPosition) / FarPlane;
};