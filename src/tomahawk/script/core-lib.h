#ifndef TH_SCRIPT_CORE_API_H
#define TH_SCRIPT_CORE_API_H

#include "std-lib.h"

namespace Tomahawk
{
	namespace Script
	{
		class TH_OUT VMCFormat : public Core::Object
		{
		public:
			std::vector<std::string> Args;

		public:
			VMCFormat();
			VMCFormat(unsigned char* Buffer);

		public:
			static std::string JSON(void* Ref, int TypeId);
			static std::string Form(const std::string& F, const VMCFormat& Form);
			static void WriteLine(Core::Console* Base, const std::string& F, VMCFormat* Form);
			static void Write(Core::Console* Base, const std::string& F, VMCFormat* Form);

		private:
			static void FormatBuffer(VMGlobal& Global, Core::Parser& Result, std::string& Offset, void* Ref, int TypeId);
			static void FormatJSON(VMGlobal& Global, Core::Parser& Result, void* Ref, int TypeId);
		};

		class TH_OUT VMCVariant
		{
		public:
			static uint64_t GetSize(Core::Variant& Base);
			static bool Equals(Core::Variant& Base, const Core::Variant& Other);
			static bool ImplCast(Core::Variant& Base);
		};

		class TH_OUT VMCDocument
		{
		public:
			static void Construct(VMCGeneric* Generic);
			static Core::Document* ConstructBuffer(unsigned char* Buffer);
			static Core::Document* GetIndex(Core::Document* Base, const std::string& Name);
			static Core::Document* GetIndexOffset(Core::Document* Base, uint64_t Offset);
			static Core::Document* Set(Core::Document* Base, const std::string& Name, Core::Document* Value);
			static Core::Document* Push(Core::Document* Base, Core::Document* Value);
			static VMCArray* GetCollection(Core::Document* Base, const std::string& Name, bool Here);
			static VMCArray* GetNodes(Core::Document* Base);
			static VMCArray* GetAttributes(Core::Document* Base);
			static VMCMap* GetNames(Core::Document* Base);
			static std::string ToJSON(Core::Document* Base);
			static std::string ToXML(Core::Document* Base);
			static std::string ToString(Core::Document* Base);
			static std::string ToBase64(Core::Document* Base);
			static int64_t ToInteger(Core::Document* Base);
			static double ToNumber(Core::Document* Base);
			static std::string ToDecimal(Core::Document* Base);
			static bool ToBoolean(Core::Document* Base);
			static Core::Document* FromJSON(const std::string& Value);
			static Core::Document* FromXML(const std::string& Value);
			static Core::Document* Import(const std::string& Value);
			static Core::Document* CopyAssign(Core::Document* Base, const Core::Variant& Other);
			static bool Equals(Core::Document* Base, Core::Document* Other);
		};

		TH_OUT bool RegisterFormatAPI(VMManager* Engine);
		TH_OUT bool RegisterConsoleAPI(VMManager* Engine);
		TH_OUT bool RegisterVariantAPI(VMManager* Engine);
		TH_OUT bool RegisterDocumentAPI(VMManager* Engine);
	}
}
#endif