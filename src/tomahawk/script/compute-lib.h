#ifndef TH_SCRIPT_COMPUTE_API_H
#define TH_SCRIPT_COMPUTE_API_H

#include "std-lib.h"

namespace Tomahawk
{
	namespace Script
	{
		TH_OUT bool CURegisterVertices(VMManager* Engine);
		TH_OUT bool CURegisterRectangle(VMManager* Engine);
		TH_OUT bool CURegisterVector2(VMManager* Engine);
		TH_OUT bool CURegisterVector3(VMManager* Engine);
		TH_OUT bool CURegisterVector4(VMManager* Engine);
	}
}
#endif