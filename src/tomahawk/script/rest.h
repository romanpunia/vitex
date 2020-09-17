#ifndef THAWK_SCRIPT_REST_H
#define THAWK_SCRIPT_REST_H

#include "core.h"

namespace Tomahawk
{
	namespace Script
	{
		class VMCFormat : public Rest::Object
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

		class VMCDocument
		{
		public:
			static void Construct(VMCGeneric* Generic);
			static Rest::Document* GetAny(Rest::Document* Base, const std::string& Name, bool Here);
			static Rest::Document* ConstructBuffer(unsigned char* Buffer);
			static Rest::Document* GetIndex(Rest::Document* Base, const std::string& Name);
			static Rest::Document* GetIndexOffset(Rest::Document* Base, uint64_t Offset);
			static Rest::Document* SetId(Rest::Document* Base, const std::string& Name, const std::string& Value);
			static std::string GetDecimal(Rest::Document* Base, const std::string& Name);
			static std::string GetId(Rest::Document* Base, const std::string& Name);
			static VMCArray* FindCollection(Rest::Document* Base, const std::string& Name, bool Here);
			static VMCArray* FindCollectionPath(Rest::Document* Base, const std::string& Name, bool Here);
			static VMCArray* GetNodes(Rest::Document* Base);
			static VMCArray* GetAttributes(Rest::Document* Base);
			static VMCMap* CreateMapping(Rest::Document* Base);
			static Rest::Document* SetDocument(Rest::Document* Base, const std::string& Name, Rest::Document* New);
			static Rest::Document* SetArray(Rest::Document* Base, const std::string& Name, Rest::Document* New);
			static std::string ToJSON(Rest::Document* Base);
			static std::string ToXML(Rest::Document* Base);
			static bool Has(Rest::Document* Base, const std::string& Name);
			static bool HasId(Rest::Document* Base, const std::string& Name);
			static bool GetNull(Rest::Document* Base, const std::string& Name);
			static bool GetUndefined(Rest::Document* Base, const std::string& Name);
			static bool GetBoolean(Rest::Document* Base, const std::string& Name);
			static int64_t GetInteger(Rest::Document* Base, const std::string& Name);
			static double GetNumber(Rest::Document* Base, const std::string& Name);
			static std::string GetString(Rest::Document* Base, const std::string& Name);
			static Rest::Document* FromJSON(const std::string& Value);
			static Rest::Document* FromXML(const std::string& Value);
			static Rest::Document* Import(const std::string& Value);
		};

		THAWK_OUT bool RegisterFormatAPI(VMManager* Engine);
		THAWK_OUT bool RegisterConsoleAPI(VMManager* Engine);
		THAWK_OUT bool RegisterDocumentAPI(VMManager* Engine);
	}
}
#endif