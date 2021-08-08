#ifndef TH_SCRIPT_GUI_API_H
#define TH_SCRIPT_GUI_API_H

#include "std-lib.h"
#include "../engine/gui.h"

namespace Tomahawk
{
	namespace Script
	{
		TH_OUT bool GUIRegisterElement(VMManager* Engine);
		TH_OUT bool GUIRegisterDocument(VMManager* Engine);
		TH_OUT bool GUIRegisterEvent(VMManager* Engine);
		TH_OUT bool GUIRegisterContext(VMManager* Engine);
	}
}
#endif