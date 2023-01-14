#ifndef TH_SCRIPT_CORE_API_H
#define TH_SCRIPT_CORE_API_H
#include "std-lib.h"

namespace Tomahawk
{
	namespace Script
	{
		class TH_OUT CEFormat : public Core::Object
		{
		public:
			std::vector<std::string> Args;

		public:
			CEFormat();
			CEFormat(unsigned char* Buffer);

		public:
			static std::string JSON(void* Ref, int TypeId);
			static std::string Form(const std::string& F, const CEFormat& Form);
			static void WriteLine(Core::Console* Base, const std::string& F, CEFormat* Form);
			static void Write(Core::Console* Base, const std::string& F, CEFormat* Form);

		private:
			static void FormatBuffer(VMGlobal& Global, Core::Parser& Result, std::string& Offset, void* Ref, int TypeId);
			static void FormatJSON(VMGlobal& Global, Core::Parser& Result, void* Ref, int TypeId);
		};

		TH_OUT_TS bool CERegisterFormat(VMManager* Engine);
		TH_OUT_TS bool CERegisterDecimal(VMManager* Engine);
		TH_OUT_TS bool CERegisterVariant(VMManager* Engine);
		TH_OUT_TS bool CERegisterFileState(VMManager* Engine);
		TH_OUT_TS bool CERegisterResource(VMManager* Engine);
		TH_OUT_TS bool CERegisterDateTime(VMManager* Engine);
		TH_OUT_TS bool CERegisterOS(VMManager* Engine);
		TH_OUT_TS bool CERegisterConsole(VMManager* Engine);
		TH_OUT_TS bool CERegisterTimer(VMManager* Engine);
		TH_OUT_TS bool CERegisterFileStream(VMManager* Engine);
		TH_OUT_TS bool CERegisterGzStream(VMManager* Engine);
		TH_OUT_TS bool CERegisterWebStream(VMManager* Engine);
		TH_OUT_TS bool CERegisterSchedule(VMManager* Engine);
		TH_OUT_TS bool CERegisterSchema(VMManager* Engine);
	}
}
#endif