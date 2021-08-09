#ifndef TH_SCRIPT_GUI_API_H
#define TH_SCRIPT_GUI_API_H

#include "std-lib.h"
#include "../engine/gui.h"

namespace Tomahawk
{
	namespace Script
	{
#ifdef TH_WITH_RMLUI
		class TH_OUT GUIListener : public Engine::GUI::Listener
		{
		private:
			VMCFunction* Source;
			VMContext* Context;

		public:
			GUIListener(VMCFunction* NewCallback);
			GUIListener(const std::string& FunctionName);
			virtual ~GUIListener() override;

		private:
			Engine::GUI::EventCallback Bind(VMCFunction* Callback);
		};
#endif
		TH_OUT bool GUIRegisterVariant(VMManager* Engine);
		TH_OUT bool GUIRegisterControl(VMManager* Engine);
		TH_OUT bool GUIRegisterModel(VMManager* Engine);
		TH_OUT bool GUIRegisterContext(VMManager* Engine);
	}
}
#endif