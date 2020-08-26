#ifndef SCRIPT_REST_H
#define SCRIPT_REST_H
#include "../script.h"
#ifndef ANGELSCRIPT_H 
#include <angelscript.h>
#endif

class VMCFormat : public Tomahawk::Rest::Object
{
public:
	std::vector<std::string> Args;

public:
	VMCFormat();
	VMCFormat(unsigned char* Buffer);

public:
	static std::string JSON(void* Ref, int TypeId);
	static std::string Form(const std::string& F, const VMCFormat& Form);
	static void WriteLine(Tomahawk::Rest::Console* Base, const std::string& F, VMCFormat* Form);
	static void Write(Tomahawk::Rest::Console* Base, const std::string& F, VMCFormat* Form);

private:
	static void FormatBuffer(Tomahawk::Script::VMGlobal& Global, Tomahawk::Rest::Stroke& Result, std::string& Offset, void* Ref, int TypeId);
	static void FormatJSON(Tomahawk::Script::VMGlobal& Global, Tomahawk::Rest::Stroke& Result, void* Ref, int TypeId);
};

class VMCDocument
{
public:
	static void Construct(Tomahawk::Script::VMCGeneric* Generic);
	static Tomahawk::Rest::Document* GetAny(Tomahawk::Rest::Document* Base, const std::string& Name, bool Here);
	static Tomahawk::Rest::Document* ConstructBuffer(asBYTE* Buffer);
	static Tomahawk::Rest::Document* GetIndex(Tomahawk::Rest::Document* Base, const std::string& Name);
	static Tomahawk::Rest::Document* SetId(Tomahawk::Rest::Document* Base, const std::string& Name, const std::string& Value);
	static std::string GetDecimal(Tomahawk::Rest::Document* Base, const std::string& Name);
	static std::string GetId(Tomahawk::Rest::Document* Base, const std::string& Name);
	static VMCArray* FindCollection(Tomahawk::Rest::Document* Base, const std::string& Name, bool Here);
	static VMCArray* FindCollectionPath(Tomahawk::Rest::Document* Base, const std::string& Name, bool Here);
	static VMCArray* GetNodes(Tomahawk::Rest::Document* Base);
	static VMCArray* GetAttributes(Tomahawk::Rest::Document* Base);
	static VMCDictionary* CreateMapping(Tomahawk::Rest::Document* Base);
	static Tomahawk::Rest::Document* SetDocument(Tomahawk::Rest::Document* Base, const std::string& Name, Tomahawk::Rest::Document* New);
	static Tomahawk::Rest::Document* SetArray(Tomahawk::Rest::Document* Base, const std::string& Name, Tomahawk::Rest::Document* New);
	static std::string ToJSON(Tomahawk::Rest::Document* Base);
	static std::string ToXML(Tomahawk::Rest::Document* Base);
	static bool Has(Tomahawk::Rest::Document* Base, const std::string& Name);
	static bool HasId(Tomahawk::Rest::Document* Base, const std::string& Name);
	static bool GetNull(Tomahawk::Rest::Document* Base, const std::string& Name);
	static bool GetUndefined(Tomahawk::Rest::Document* Base, const std::string& Name);
	static bool GetBoolean(Tomahawk::Rest::Document* Base, const std::string& Name);
	static int64_t GetInteger(Tomahawk::Rest::Document* Base, const std::string& Name);
	static double GetNumber(Tomahawk::Rest::Document* Base, const std::string& Name);
	static std::string GetString(Tomahawk::Rest::Document* Base, const std::string& Name);
	static Tomahawk::Rest::Document* FromJSON(const std::string& Value);
	static Tomahawk::Rest::Document* FromXML(const std::string& Value);
	static Tomahawk::Rest::Document* Import(const std::string& Value);
};

void VM_RegisterRest(Tomahawk::Script::VMManager* engine);
#endif