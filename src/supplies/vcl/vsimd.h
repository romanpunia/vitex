#ifndef TH_VSIMD_H
#define TH_VSIMD_H
#ifndef TH_NO_SIMD
#include "instrset.h"
#if INSTRSET >= 2
#include "vectorclass.h"
#include "vectormath_trig.h"
#define TH_HAS_SIMD
#define LOD_V2(Name, Ctx) Vec4f (Name)(Ctx.X, Ctx.Y, 0.0f, 0.0f)
#define LOD_AV2(Name, X, Y) Vec4f (Name)(X, Y, 0.0f, 0.0f)
#define LOD_FV2(Name) Vec4f (Name)(X, Y, 0.0f, 0.0f)
#define LOD_V3(Name, Ctx) Vec4f (Name)(Ctx.X, Ctx.Y, Ctx.Z, 0.0f)
#define LOD_AV3(Name, X, Y, Z) Vec4f (Name)(X, Y, Z, 0.0f)
#define LOD_FV3(Name) Vec4f (Name)(X, Y, Z, 0.0f)
#define LOD_V4(Name, Ctx) Vec4f (Name);(Name).load((float*)&Ctx)
#define LOD_AV4(Name, X, Y, Z, W) Vec4f (Name)(X, Y, Z, W)
#define LOD_FV4(Name) Vec4f (Name);(Name).load((float*)this)
#define LOD_V16(Name, Ctx) Vec16f (Name);(Name).load(Ctx.Row)
#define LOD_AV16(Name, A) Vec16f (Name);(Name).load(A)
#define LOD_FV16(Name) Vec16f (Name);(Name).load(Row)
#define LOD_VAR(Name, A) Vec4f (Name);(Name).load(A)
#define LOD_VAL(Name, A) Vec4f (Name)(A)
#endif
#endif
#endif