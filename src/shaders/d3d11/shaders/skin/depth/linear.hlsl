#include "common/skin/linear"

float PS(VOutputLinear V) : SV_TARGET0
{
    return V.UV.z / V.UV.w;
};