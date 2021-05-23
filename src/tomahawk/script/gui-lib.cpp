#include "gui-lib.h"
#ifndef ANGELSCRIPT_H 
#include <angelscript.h>
#endif

namespace Tomahawk
{
	namespace Script
	{
		bool RegisterGuiElementAPI(VMManager* Engine)
		{
#ifdef TH_WITH_RMLUI
			if (!Engine)
				return false;

			VMGlobal& Register = Engine->Global();

			VMTypeClass VElement = Register.SetStructUnmanaged<Engine::GUI::IElement>("GuiElement");

			return true;
#else
			return false;
#endif
		}
		bool RegisterGuiDocumentAPI(VMManager* Engine)
		{
#ifdef TH_WITH_RMLUI
			if (!Engine)
				return false;

			VMGlobal& Register = Engine->Global();

			VMTypeClass VDocument = Register.SetStructUnmanaged<Engine::GUI::IElementDocument>("GuiDocument");

			return true;
#else
			return false;
#endif
		}
		bool RegisterGuiEventAPI(VMManager* Engine)
		{
#ifdef TH_WITH_RMLUI
			if (!Engine)
				return false;

			VMGlobal& Register = Engine->Global();

			VMTypeClass VEvent = Register.SetStructUnmanaged<Engine::GUI::IEvent>("GuiEvent");

			return true;
#else
			return false;
#endif
		}
		bool RegisterGuiContextAPI(VMManager* Engine)
		{
#ifdef TH_WITH_RMLUI
			if (!Engine)
				return false;

			VMGlobal& Register = Engine->Global();

			VMRefClass VContext = Register.SetClassUnmanaged<Engine::GUI::Context>("GuiContext");

			return true;
#else
			return false;
#endif
		}
	}
}