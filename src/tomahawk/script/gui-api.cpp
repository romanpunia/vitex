#include "gui-api.h"
#ifndef ANGELSCRIPT_H 
#include <angelscript.h>
#endif

namespace Tomahawk
{
	namespace Script
	{
		bool RegisterGuiElementAPI(VMManager* Engine)
		{
			if (!Engine)
				return false;

			VMGlobal& Register = Engine->Global();

			VMTypeClass VElement = Register.SetStructUnmanaged<Engine::GUI::IElement>("GuiElement");

			return true;
		}
		bool RegisterGuiDocumentAPI(VMManager* Engine)
		{
			if (!Engine)
				return false;

			VMGlobal& Register = Engine->Global();

			VMTypeClass VDocument = Register.SetStructUnmanaged<Engine::GUI::IElementDocument>("GuiDocument");

			return true;
		}
		bool RegisterGuiEventAPI(VMManager* Engine)
		{
			if (!Engine)
				return false;

			VMGlobal& Register = Engine->Global();

			VMTypeClass VEvent = Register.SetStructUnmanaged<Engine::GUI::IEvent>("GuiEvent");

			return true;
		}
		bool RegisterGuiContextAPI(VMManager* Engine)
		{
			if (!Engine)
				return false;

			VMGlobal& Register = Engine->Global();

			VMRefClass VContext = Register.SetClassUnmanaged<Engine::GUI::Context>("GuiContext");

			return true;
		}
	}
}