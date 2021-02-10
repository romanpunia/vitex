#ifndef TH_SCRIPT_REST_API_H
#define TH_SCRIPT_REST_API_H

#include "core-api.h"

namespace Tomahawk
{
	namespace Script
	{
		class TH_OUT VMCFormat : public Rest::Object
		{
		public:
			std::vector<std::string> Args;

		public:
			VMCFormat();
			VMCFormat(unsigned char* Buffer);

		public:
			static std::string JSON(void* Ref, int TypeId);
			static std::string Form(const std::string& F, const VMCFormat& Form);
			static void WriteLine(Rest::Console* Base, const std::string& F, VMCFormat* Form);
			static void Write(Rest::Console* Base, const std::string& F, VMCFormat* Form);

		private:
			static void FormatBuffer(VMGlobal& Global, Rest::Stroke& Result, std::string& Offset, void* Ref, int TypeId);
			static void FormatJSON(VMGlobal& Global, Rest::Stroke& Result, void* Ref, int TypeId);
		};

		class TH_OUT VMCVariant
		{
		public:
			static uint64_t GetSize(Rest::Variant& Base);
			static bool Equals(Rest::Variant& Base, const Rest::Variant& Other);
			static bool ImplCast(Rest::Variant& Base);
		};

		class TH_OUT VMCDocument
		{
		public:
			static void Construct(VMCGeneric* Generic);
			static Rest::Document* ConstructBuffer(unsigned char* Buffer);
			static Rest::Document* GetIndex(Rest::Document* Base, const std::string& Name);
			static Rest::Document* GetIndexOffset(Rest::Document* Base, uint64_t Offset);
			static Rest::Document* Set(Rest::Document* Base, const std::string& Name, Rest::Document* Value);
			static Rest::Document* Push(Rest::Document* Base, Rest::Document* Value);
			static VMCArray* GetCollection(Rest::Document* Base, const std::string& Name, bool Here);
			static VMCArray* GetNodes(Rest::Document* Base);
			static VMCArray* GetAttributes(Rest::Document* Base);
			static VMCMap* GetNames(Rest::Document* Base);
			static std::string ToJSON(Rest::Document* Base);
			static std::string ToXML(Rest::Document* Base);
			static std::string ToString(Rest::Document* Base);
			static std::string ToBase64(Rest::Document* Base);
			static int64_t ToInteger(Rest::Document* Base);
			static double ToNumber(Rest::Document* Base);
			static std::string ToDecimal(Rest::Document* Base);
			static bool ToBoolean(Rest::Document* Base);
			static Rest::Document* FromJSON(const std::string& Value);
			static Rest::Document* FromXML(const std::string& Value);
			static Rest::Document* Import(const std::string& Value);
			static Rest::Document* CopyAssign(Rest::Document* Base, const Rest::Variant& Other);
			static bool Equals(Rest::Document* Base, Rest::Document* Other);
		};

		TH_OUT bool RegisterFormatAPI(VMManager* Engine);
		TH_OUT bool RegisterConsoleAPI(VMManager* Engine);
		TH_OUT bool RegisterVariantAPI(VMManager* Engine);
		TH_OUT bool RegisterDocumentAPI(VMManager* Engine);
	}
}
#endif