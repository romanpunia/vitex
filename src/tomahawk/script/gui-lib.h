#ifndef TH_SCRIPT_GUI_API_H
#define TH_SCRIPT_GUI_API_H

#include "std-lib.h"
#include "../engine/gui.h"

namespace Tomahawk
{
	namespace Script
	{
		TH_OUT bool RegisterGuiElementAPI(VMManager* Engine);
		TH_OUT bool RegisterGuiDocumentAPI(VMManager* Engine);
		TH_OUT bool RegisterGuiEventAPI(VMManager* Engine);
		TH_OUT bool RegisterGuiContextAPI(VMManager* Engine);
	}
}
#endif