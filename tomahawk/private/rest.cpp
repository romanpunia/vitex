#include "rest.h"
#include "../network/bson.h"

using namespace Tomahawk;
using namespace Tomahawk::Script;

VMCFormat::VMCFormat()
{
}
VMCFormat::VMCFormat(unsigned char* Buffer)
{
	VMContext* Context = VMContext::Get();
	if (!Context || !Buffer)
		return;

	VMGlobal& Global = Context->GetManager()->Global();
	unsigned int Length = *(unsigned int*)Buffer;
	Buffer += 4;

	while (Length--)
	{
		if (uintptr_t(Buffer) & 0x3)
			Buffer += 4 - (uintptr_t(Buffer) & 0x3);

		int TypeId = *(int*)Buffer;
		Buffer += sizeof(int);

		Rest::Stroke Result; std::string Offset;
		FormatBuffer(Global, Result, Offset, (void*)Buffer, TypeId);
		Args.push_back(Result.R()[0] == '\n' ? Result.Substring(1).R() : Result.R());

		if (TypeId & VMTypeId_MASK_OBJECT)
		{
			VMWTypeInfo Type = Global.GetTypeInfoById(TypeId);
			if (Type.IsValid() && Type.GetFlags() & VMObjType_VALUE)
				Buffer += Type.GetSize();
			else
				Buffer += sizeof(void*);
		}
		else if (TypeId == 0)
			Buffer += sizeof(void*);
		else
			Buffer += Global.GetSizeOfPrimitiveType(TypeId);
	}
}
std::string VMCFormat::JSON(void* Ref, int TypeId)
{
	VMContext* Context = VMContext::Get();
	if (!Context)
		return "{}";

	VMGlobal& Global = Context->GetManager()->Global();
	Rest::Stroke Result;

	FormatJSON(Global, Result, Ref, TypeId);
	return Result.R();
}
std::string VMCFormat::Form(const std::string& F, const VMCFormat& Form)
{
	Rest::Stroke Buffer = F;
	uint64_t Offset = 0;

	for (auto& Item : Form.Args)
	{
		auto R = Buffer.FindUnescaped('?', Offset);
		if (!R.Found)
			break;

		Buffer.ReplacePart(R.Start, R.End, Item);
		Offset = R.End;
	}

	return Buffer.R();
}
void VMCFormat::WriteLine(Rest::Console* Base, const std::string& F, VMCFormat* Form)
{
	Rest::Stroke Buffer = F;
	uint64_t Offset = 0;

	if (Form != nullptr)
	{
		for (auto& Item : Form->Args)
		{
			auto R = Buffer.FindUnescaped('?', Offset);
			if (!R.Found)
				break;

			Buffer.ReplacePart(R.Start, R.End, Item);
			Offset = R.End;
		}
	}

	Base->sWriteLine(Buffer.R());
}
void VMCFormat::Write(Rest::Console* Base, const std::string& F, VMCFormat* Form)
{
	Rest::Stroke Buffer = F;
	uint64_t Offset = 0;

	if (Form != nullptr)
	{
		for (auto& Item : Form->Args)
		{
			auto R = Buffer.FindUnescaped('?', Offset);
			if (!R.Found)
				break;

			Buffer.ReplacePart(R.Start, R.End, Item);
			Offset = R.End;
		}
	}

	Base->sWrite(Buffer.R());
}
void VMCFormat::FormatBuffer(VMGlobal& Global, Rest::Stroke& Result, std::string& Offset, void* Ref, int TypeId)
{
	if (TypeId < VMTypeId_BOOL || TypeId > VMTypeId_DOUBLE)
	{
		VMWTypeInfo Type = Global.GetTypeInfoById(TypeId);
		if (!Ref)
		{
			Result.Append("null");
			return;
		}

		if (VMWTypeInfo::IsScriptObject(TypeId))
		{
			VMWObject VObject = *(VMCObject**)Ref;
			Rest::Stroke Decl;

			Offset += '\t';
			for (unsigned int i = 0; i < VObject.GetPropertiesCount(); i++)
			{
				const char* Name = VObject.GetPropertyName(i);
				Decl.Append(Offset).fAppend("%s: ", Name ? Name : "");
				FormatBuffer(Global, Decl, Offset, VObject.GetAddressOfProperty(i), VObject.GetPropertyTypeId(i));
				Decl.Append(",\n");
			}

			Offset = Offset.substr(0, Offset.size() - 1);
			if (!Decl.Empty())
				Result.fAppend("\n%s{\n%s\n%s}", Offset.c_str(), Decl.Clip(Decl.Size() - 2).Get(), Offset.c_str());
			else
				Result.Append("{}");
		}
		else if (strcmp(Type.GetName(), "dictionary") == 0)
		{
			VMWDictionary Map = *(VMCDictionary**)Ref;
			Rest::Stroke Decl; std::string Name;

			Offset += '\t';
			for (unsigned int i = 0; i < Map.GetSize(); i++)
			{
				void* ElementRef; int ElementTypeId;
				if (!Map.GetIndex(i, &Name, &ElementRef, &ElementTypeId))
					continue;

				Decl.Append(Offset).fAppend("%s: ", Name.c_str());
				FormatBuffer(Global, Decl, Offset, ElementRef, ElementTypeId);
				Decl.Append(",\n");
			}

			Offset = Offset.substr(0, Offset.size() - 1);
			if (!Decl.Empty())
				Result.fAppend("\n%s{\n%s\n%s}", Offset.c_str(), Decl.Clip(Decl.Size() - 2).Get(), Offset.c_str());
			else
				Result.Append("{}");
		}
		else if (strcmp(Type.GetName(), "array") == 0)
		{
			VMWArray Array = *(VMCArray**)Ref;
			int ArrayTypeId = Array.GetElementTypeId();
			VMWTypeInfo ArrayType = Global.GetTypeInfoById(ArrayTypeId);
			Rest::Stroke Decl;

			Offset += '\t';
			for (unsigned int i = 0; i < Array.GetSize(); i++)
			{
				Decl.Append(Offset);
				FormatBuffer(Global, Decl, Offset, Array.At(i), ArrayTypeId);
				Decl.Append(", ");
			}

			Offset = Offset.substr(0, Offset.size() - 1);
			if (!Decl.Empty())
				Result.fAppend("\n%s[\n%s\n%s]", Offset.c_str(), Decl.Clip(Decl.Size() - 2).Get(), Offset.c_str());
			else
				Result.Append("[]");
		}
		else if (strcmp(Type.GetName(), "string") != 0)
		{
			Rest::Stroke Decl;
			Offset += '\t';

			Type.ForEachProperty([&Decl, &Global, &Offset, Ref, TypeId](VMWTypeInfo* Base, VMFuncProperty* Prop)
			{
				Decl.Append(Offset).fAppend("%s: ", Prop->Name ? Prop->Name : "");
				FormatBuffer(Global, Decl, Offset, Base->GetProperty<void>(Ref, Prop->Offset, TypeId), Prop->TypeId);
				Decl.Append(",\n");
			});

			Offset = Offset.substr(0, Offset.size() - 1);
			if (!Decl.Empty())
				Result.fAppend("\n%s{\n%s\n%s}", Offset.c_str(), Decl.Clip(Decl.Size() - 2).Get(), Offset.c_str());
			else
				Result.fAppend("{}\n", Type.GetName());
		}
		else
			Result.Append(*(std::string*)Ref);
	}
	else
	{
		switch (TypeId)
		{
			case VMTypeId_BOOL:
				Result.fAppend("%s", *(bool*)Ref ? "true" : "false");
				break;
			case VMTypeId_INT8:
				Result.fAppend("%i", *(char*)Ref);
				break;
			case VMTypeId_INT16:
				Result.fAppend("%i", *(short*)Ref);
				break;
			case VMTypeId_INT32:
				Result.fAppend("%i", *(int*)Ref);
				break;
			case VMTypeId_INT64:
				Result.fAppend("%ll", *(int64_t*)Ref);
				break;
			case VMTypeId_UINT8:
				Result.fAppend("%u", *(unsigned char*)Ref);
				break;
			case VMTypeId_UINT16:
				Result.fAppend("%u", *(unsigned short*)Ref);
				break;
			case VMTypeId_UINT32:
				Result.fAppend("%u", *(unsigned int*)Ref);
				break;
			case VMTypeId_UINT64:
				Result.fAppend("%llu", *(uint64_t*)Ref);
				break;
			case VMTypeId_FLOAT:
				Result.fAppend("%f", *(float*)Ref);
				break;
			case VMTypeId_DOUBLE:
				Result.fAppend("%f", *(double*)Ref);
				break;
			default:
				Result.Append("null");
				break;
		}
	}
}
void VMCFormat::FormatJSON(VMGlobal& Global, Rest::Stroke& Result, void* Ref, int TypeId)
	{
		if (TypeId < VMTypeId_BOOL || TypeId > VMTypeId_DOUBLE)
		{
			VMWTypeInfo Type = Global.GetTypeInfoById(TypeId);
			void* Object = Type.GetInstance<void>(Ref, TypeId);

			if (!Object)
			{
				Result.Append("null");
				return;
			}

			if (VMWTypeInfo::IsScriptObject(TypeId))
			{
				VMWObject VObject = (VMCObject*)Object;
				Rest::Stroke Decl;

				for (unsigned int i = 0; i < VObject.GetPropertiesCount(); i++)
				{
					const char* Name = VObject.GetPropertyName(i);
					Decl.fAppend("\"%s\":", Name ? Name : "");
					FormatJSON(Global, Decl, VObject.GetAddressOfProperty(i), VObject.GetPropertyTypeId(i));
					Decl.Append(",");
				}

				if (!Decl.Empty())
					Result.fAppend("{%s}", Decl.Clip(Decl.Size() - 1).Get());
				else
					Result.Append("{}");
			}
			else if (strcmp(Type.GetName(), "dictionary") == 0)
			{
				VMWDictionary Map = (VMCDictionary*)Object;
				Rest::Stroke Decl; std::string Name;

				for (unsigned int i = 0; i < Map.GetSize(); i++)
				{
					void* ElementRef; int ElementTypeId;
					if (!Map.GetIndex(i, &Name, &ElementRef, &ElementTypeId))
						continue;

					Decl.fAppend("\"%s\":", Name.c_str());
					FormatJSON(Global, Decl, ElementRef, ElementTypeId);
					Decl.Append(",");
				}

				if (!Decl.Empty())
					Result.fAppend("{%s}", Decl.Clip(Decl.Size() - 1).Get());
				else
					Result.Append("{}");
			}
			else if (strcmp(Type.GetName(), "array") == 0)
			{
				VMWArray Array = (VMCArray*)Object;
				int ArrayTypeId = Array.GetElementTypeId();
				VMWTypeInfo ArrayType = Global.GetTypeInfoById(ArrayTypeId);
				Rest::Stroke Decl;

				for (unsigned int i = 0; i < Array.GetSize(); i++)
				{
					FormatJSON(Global, Decl, Array.At(i), ArrayTypeId);
					Decl.Append(",");
				}

				if (!Decl.Empty())
					Result.fAppend("[%s]", Decl.Clip(Decl.Size() - 1).Get());
				else
					Result.Append("[]");
			}
			else if (strcmp(Type.GetName(), "string") != 0)
			{
				Rest::Stroke Decl;
				Type.ForEachProperty([&Decl, &Global, Ref, TypeId](VMWTypeInfo* Base, VMFuncProperty* Prop)
				{
					Decl.fAppend("\"%s\":", Prop->Name ? Prop->Name : "");
					FormatJSON(Global, Decl, Base->GetProperty<void>(Ref, Prop->Offset, TypeId), Prop->TypeId);
					Decl.Append(",");
				});

				if (!Decl.Empty())
					Result.fAppend("{%s}", Decl.Clip(Decl.Size() - 1).Get());
				else
					Result.fAppend("{}", Type.GetName());
			}
			else
				Result.fAppend("\"%s\"", ((std::string*)Object)->c_str());
		}
		else
		{
			switch (TypeId)
			{
				case VMTypeId_BOOL:
					Result.fAppend("%s", *(bool*)Ref ? "true" : "false");
					break;
				case VMTypeId_INT8:
					Result.fAppend("%i", *(char*)Ref);
					break;
				case VMTypeId_INT16:
					Result.fAppend("%i", *(short*)Ref);
					break;
				case VMTypeId_INT32:
					Result.fAppend("%i", *(int*)Ref);
					break;
				case VMTypeId_INT64:
					Result.fAppend("%ll", *(int64_t*)Ref);
					break;
				case VMTypeId_UINT8:
					Result.fAppend("%u", *(unsigned char*)Ref);
					break;
				case VMTypeId_UINT16:
					Result.fAppend("%u", *(unsigned short*)Ref);
					break;
				case VMTypeId_UINT32:
					Result.fAppend("%u", *(unsigned int*)Ref);
					break;
				case VMTypeId_UINT64:
					Result.fAppend("%llu", *(uint64_t*)Ref);
					break;
				case VMTypeId_FLOAT:
					Result.fAppend("%f", *(float*)Ref);
					break;
				case VMTypeId_DOUBLE:
					Result.fAppend("%f", *(double*)Ref);
					break;
			}
		}
	}

void VMCDocument::Construct(VMCGeneric* Generic)
{
	asBYTE* Buffer = (asBYTE*)Generic->GetArgAddress(0);
	*(Rest::Document**)Generic->GetAddressOfReturnLocation() = ConstructBuffer(Buffer);
}
Rest::Document* VMCDocument::GetAny(Rest::Document* Base, const std::string& Name, bool Here)
{
	std::vector<std::string> Names = Rest::Stroke(Name).Split('.');
	if (Names.empty())
		return nullptr;

	Rest::Document* Current = Base->Find(*Names.begin(), Here);
	if (!Current)
		return Current->SetDocument(*Names.begin());

	for (auto It = Names.begin() + 1; It != Names.end(); It++)
	{
		Current = Current->Find(*It, Here);
		if (!Current)
			Current = Current->SetDocument(*It);
	}

	return Current;
}
Rest::Document* VMCDocument::ConstructBuffer(asBYTE* Buffer)
{
	if (!Buffer)
		return nullptr;

	Rest::Document* Result = new Rest::Document();
	VMCContext* Context = asGetActiveContext();
	VMCManager* Manager = Context->GetEngine();
	asUINT Length = *(asUINT*)Buffer;
	Buffer += 4;

	while (Length--)
	{
		if (asPWORD(Buffer) & 0x3)
			Buffer += 4 - (asPWORD(Buffer) & 0x3);

		std::string Name = *(std::string*)Buffer;
		Buffer += sizeof(std::string);

		int TypeId = *(int*)Buffer;
		Buffer += sizeof(int);

		void* Ref = (void*)Buffer;
		if (TypeId >= asTYPEID_BOOL && TypeId <= asTYPEID_DOUBLE)
		{
			switch (TypeId)
			{
				case asTYPEID_BOOL:
					Result->SetBoolean(Name, *(bool*)Ref);
					break;
				case asTYPEID_INT8:
					Result->SetInteger(Name, *(char*)Ref);
					break;
				case asTYPEID_INT16:
					Result->SetInteger(Name, *(short*)Ref);
					break;
				case asTYPEID_INT32:
					Result->SetInteger(Name, *(int*)Ref);
					break;
				case asTYPEID_INT64:
					Result->SetInteger(Name, *(asINT64*)Ref);
					break;
				case asTYPEID_UINT8:
					Result->SetInteger(Name, *(unsigned char*)Ref);
					break;
				case asTYPEID_UINT16:
					Result->SetInteger(Name, *(unsigned short*)Ref);
					break;
				case asTYPEID_UINT32:
					Result->SetInteger(Name, *(unsigned int*)Ref);
					break;
				case asTYPEID_UINT64:
					Result->SetInteger(Name, *(asINT64*)Ref);
					break;
				case asTYPEID_FLOAT:
					Result->SetNumber(Name, *(float*)Ref);
					break;
				case asTYPEID_DOUBLE:
					Result->SetNumber(Name, *(double*)Ref);
					break;
			}
		}
		else
		{
			VMCTypeInfo* Type = Manager->GetTypeInfoById(TypeId);
			if ((TypeId & asTYPEID_MASK_OBJECT) && !(TypeId & asTYPEID_OBJHANDLE) && (Type && Type->GetFlags() & asOBJ_REF))
				Ref = *(void**)Ref;

			if (TypeId & asTYPEID_OBJHANDLE)
				Ref = *(void**)Ref;

			if (!Ref)
				Result->SetNull(Name);
			else if (Type && !strcmp("document", Type->GetName()))
				Result->SetDocument(Name, ((Rest::Document*)Ref)->AddRefAs<Rest::Document>());
			else if (Type && !strcmp("string", Type->GetName()))
				Result->SetString(Name, *(std::string*)Ref);
		}

		if (TypeId & asTYPEID_MASK_OBJECT)
		{
			asITypeInfo *ti = Manager->GetTypeInfoById(TypeId);
			if (ti->GetFlags() & asOBJ_VALUE)
				Buffer += ti->GetSize();
			else
				Buffer += sizeof(void*);
		}
		else if (TypeId == 0)
			Buffer += sizeof(void*);
		else
			Buffer += Manager->GetSizeOfPrimitiveType(TypeId);
	}

	return Result;
}
Rest::Document* VMCDocument::GetIndex(Rest::Document* Base, const std::string& Name)
{
	return Base->FindPath(Name, true);
}
Rest::Document* VMCDocument::SetId(Rest::Document* Base, const std::string& Name, const std::string& Value)
{
	if (Value.size() != 12)
		return nullptr;

	return Base->SetId(Name, (unsigned char*)Value.c_str());
}
std::string VMCDocument::GetDecimal(Rest::Document* Base, const std::string& Name)
{
	int64_t Low;

	Network::BSON::KeyPair Key;
	Key.Mod = Network::BSON::Type_Decimal;
	Key.IsValid = true;
	Key.High = Base->GetDecimal(Name, &Low);
	Key.Low = Low;

	return Key.ToString();
}
std::string VMCDocument::GetId(Rest::Document* Base, const std::string& Name)
{
	Rest::Document* Result = Base->FindPath(Name, true);
	if (!Result)
		return "";

	if (Result->Type != Rest::NodeType_Id || Result->String.size() != 12)
		return "";

	return std::string((const char*)Result->String.c_str(), 12);
}
VMCArray* VMCDocument::FindCollection(Rest::Document* Base, const std::string& Name, bool Here)
{
	VMContext* Context = VMContext::Get();
	if (!Context)
		return nullptr;

	VMManager* Manager = Context->GetManager();
	if (!Manager)
		return nullptr;

	std::vector<Rest::Document*> Nodes = Base->FindCollection(Name, Here);
	for (auto& Node : Nodes)
		Node->AddRef();

	VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<document>");
	return VMWArray::ComposeFromPointers(Type, Nodes).GetArray();
}
VMCArray* VMCDocument::FindCollectionPath(Rest::Document* Base, const std::string& Name, bool Here)
{
	VMContext* Context = VMContext::Get();
	if (!Context)
		return nullptr;

	VMManager* Manager = Context->GetManager();
	if (!Manager)
		return nullptr;

	std::vector<Rest::Document*> Nodes = Base->FindCollectionPath(Name, Here);
	for (auto& Node : Nodes)
		Node->AddRef();

	VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<document>");
	return VMWArray::ComposeFromPointers(Type, Nodes).GetArray();
}
VMCArray* VMCDocument::GetNodes(Rest::Document* Base)
{
	VMContext* Context = VMContext::Get();
	if (!Context)
		return nullptr;

	VMManager* Manager = Context->GetManager();
	if (!Manager)
		return nullptr;

	std::vector<Rest::Document*> Nodes = *Base->GetNodes();
	for (auto& Node : Nodes)
		Node->AddRef();

	VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<document>");
	return VMWArray::ComposeFromPointers(Type, Nodes).GetArray();
}
VMCArray* VMCDocument::GetAttributes(Rest::Document* Base)
{
	VMContext* Context = VMContext::Get();
	if (!Context)
		return nullptr;

	VMManager* Manager = Context->GetManager();
	if (!Manager)
		return nullptr;

	std::vector<Rest::Document*> Nodes = Base->GetAttributes();
	for (auto& Node : Nodes)
		Node->AddRef();

	VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<document>");
	return VMWArray::ComposeFromPointers(Type, Nodes).GetArray();
}
VMCDictionary* VMCDocument::CreateMapping(Rest::Document* Base)
{
	VMContext* Context = VMContext::Get();
	if (!Context)
		return nullptr;

	VMManager* Manager = Context->GetManager();
	if (!Manager)
		return nullptr;

	std::unordered_map<std::string, uint64_t> Mapping = Base->CreateMapping();
	VMWDictionary Map = VMWDictionary::Create(Manager);

	for (auto& Item : Mapping)
	{
		int64_t V = Item.second;
		Map.Set(Item.first, V);
	}

	return Map.GetDictionary();
}
Rest::Document* VMCDocument::SetDocument(Rest::Document* Base, const std::string& Name, Rest::Document* New)
{
	if (New != nullptr)
		New->AddRef();

	return Base->SetDocument(Name, New);
}
Rest::Document* VMCDocument::SetArray(Rest::Document* Base, const std::string& Name, Rest::Document* New)
{
	if (New != nullptr)
		New->AddRef();

	return Base->SetArray(Name, New);
}
std::string VMCDocument::ToJSON(Rest::Document* Base)
{
	std::string Stream;
	Rest::Document::WriteJSON(Base, [&Stream](Rest::DocumentPretty, const char* Buffer, int64_t Length)
	{
		if (Buffer != nullptr && Length > 0)
			Stream.append(Buffer, Length);
	});

	return Stream;
}
std::string VMCDocument::ToXML(Rest::Document* Base)
{
	std::string Stream;
	Rest::Document::WriteXML(Base, [&Stream](Rest::DocumentPretty, const char* Buffer, int64_t Length)
	{
		if (Buffer != nullptr && Length > 0)
			Stream.append(Buffer, Length);
	});

	return Stream;
}
bool VMCDocument::Has(Tomahawk::Rest::Document* Base, const std::string& Name)
{
	return Base->FindPath(Name, true) != nullptr;
}
bool VMCDocument::HasId(Tomahawk::Rest::Document* Base, const std::string& Name)
{
	Rest::Document* Result = Base->FindPath(Name, true);
	return (Result != nullptr && Result->Type == Rest::NodeType_Id);
}
bool VMCDocument::GetNull(Tomahawk::Rest::Document* Base, const std::string& Name)
{
	Rest::Document* Result = Base->FindPath(Name, true);
	if (!Result)
		return false;

	return Result->Type == Rest::NodeType_Null;
}
bool VMCDocument::GetUndefined(Tomahawk::Rest::Document* Base, const std::string& Name)
{
	Rest::Document* Result = Base->FindPath(Name, true);
	if (!Result)
		return true;

	return Result->Type == Rest::NodeType_Undefined;
}
bool VMCDocument::GetBoolean(Tomahawk::Rest::Document* Base, const std::string& Name)
{
	Rest::Document* Result = Base->FindPath(Name, true);
	if (!Result)
		return false;

	return Result->Boolean;
}
int64_t VMCDocument::GetInteger(Tomahawk::Rest::Document* Base, const std::string& Name)
{
	Rest::Document* Result = Base->FindPath(Name, true);
	if (!Result)
		return 0;

	return Result->Integer;
}
double VMCDocument::GetNumber(Tomahawk::Rest::Document* Base, const std::string& Name)
{
	Rest::Document* Result = Base->FindPath(Name, true);
	if (!Result)
		return 0.0;

	return Result->Number;
}
std::string VMCDocument::GetString(Tomahawk::Rest::Document* Base, const std::string& Name)
{
	Rest::Document* Result = Base->FindPath(Name, true);
	if (!Result)
		return "";

	return Result->String;
}
Rest::Document* VMCDocument::FromJSON(const std::string& Value)
{
	size_t Offset = 0;
	return Rest::Document::ReadJSON(Value.size(), [&Value, &Offset](char* Buffer, int64_t Size)
	{
		if (!Buffer || !Size)
			return true;

		size_t Length = Value.size() - Offset;
		if (Size < Length)
			Length = Size;

		if (!Length)
			return false;

		memcpy(Buffer, Value.c_str() + Offset, Length);
		Offset += Length;
		return true;
	});
}
Rest::Document* VMCDocument::FromXML(const std::string& Value)
{
	size_t Offset = 0;
	return Rest::Document::ReadXML(Value.size(), [&Value, &Offset](char* Buffer, int64_t Size)
	{
		if (!Buffer || !Size)
			return true;

		size_t Length = Value.size() - Offset;
		if (Size < Length)
			Length = Size;

		if (!Length)
			return false;

		memcpy(Buffer, Value.c_str() + Offset, Length);
		Offset += Length;
		return true;
	});
}
Rest::Document* VMCDocument::Import(const std::string& Value)
{
	VMManager* Manager = VMManager::Get();
	if (!Manager)
		return nullptr;

	return Manager->ImportJSON(Value);
}

void VM_RegisterRest(VMManager* engine)
{
	VMGlobal& Register = engine->Global();

	VMWEnum VNodeType = Register.SetEnum("node_type");
	VNodeType.SetValue("undefined", Rest::NodeType_Undefined);
	VNodeType.SetValue("object", Rest::NodeType_Object);
	VNodeType.SetValue("array", Rest::NodeType_Array);
	VNodeType.SetValue("string", Rest::NodeType_String);
	VNodeType.SetValue("integer", Rest::NodeType_Integer);
	VNodeType.SetValue("number", Rest::NodeType_Number);
	VNodeType.SetValue("boolean", Rest::NodeType_Boolean);
	VNodeType.SetValue("nullable", Rest::NodeType_Null);
	VNodeType.SetValue("id", Rest::NodeType_Id);
	VNodeType.SetValue("decimal", Rest::NodeType_Decimal);

	VMWRefClass VFormat = Register.SetClassUnmanaged<VMCFormat>("format");
	VFormat.SetUnmanagedConstructor<VMCFormat>("format@ f()");
	VFormat.SetUnmanagedConstructorList<VMCFormat>("format@ f(int &in) {repeat ?}");
	VFormat.SetMethodStatic("string JSON(const ? &in)", &VMCFormat::JSON);

	VMWRefClass VConsole = Register.SetClassUnmanaged<Rest::Console>("console");
	VConsole.SetUnmanagedConstructor<Rest::Console>("console@ f()");
	VConsole.SetMethod("void hide()", &Rest::Console::Hide);
	VConsole.SetMethod("void show()", &Rest::Console::Show);
	VConsole.SetMethod("void clear()", &Rest::Console::Clear);
	VConsole.SetMethod("void flush()", &Rest::Console::Flush);
	VConsole.SetMethod("void flushWrite()", &Rest::Console::FlushWrite);
	VConsole.SetMethod("void captureTime()", &Rest::Console::CaptureTime);
	VConsole.SetMethod("void writeLine(const string &in)", &Rest::Console::sWriteLine);
	VConsole.SetMethod("void write(const string &in)", &Rest::Console::sWrite);
	VConsole.SetMethod("double getCapturedTime()", &Rest::Console::GetCapturedTime);
	VConsole.SetMethod("string read(uint64)", &Rest::Console::Read);
	VConsole.SetMethodStatic("console@+ get()", &Rest::Console::Get);
	VConsole.SetMethodEx("void writeLine(const string &in, format@+)", &VMCFormat::WriteLine);
	VConsole.SetMethodEx("void write(const string &in, format@+)", &VMCFormat::Write);

	VMWRefClass VDocument = Register.SetClassUnmanaged<Rest::Document>("document");
	VDocument.SetProperty<Rest::Document>("string name", &Rest::Document::Name);
	VDocument.SetProperty<Rest::Document>("string string", &Rest::Document::String);
	VDocument.SetProperty<Rest::Document>("node_type type", &Rest::Document::Type);
	VDocument.SetProperty<Rest::Document>("int64 low", &Rest::Document::Low);
	VDocument.SetProperty<Rest::Document>("int64 integer", &Rest::Document::Integer);
	VDocument.SetProperty<Rest::Document>("double number", &Rest::Document::Number);
	VDocument.SetProperty<Rest::Document>("bool boolean", &Rest::Document::Boolean);
	VDocument.SetProperty<Rest::Document>("bool saved", &Rest::Document::Saved);
	VDocument.SetUnmanagedConstructor<Rest::Document>("document@ f()");
	VDocument.SetUnmanagedConstructorListEx<Rest::Document>("document@ f(int &in) {repeat {string, ?}}", &VMCDocument::Construct);
	VDocument.SetLOperatorEx(VMOpFunc_Index, "const document@+", "const string &in", &VMCDocument::GetIndex);
	VDocument.SetMethod("void join(document@+)", &Rest::Document::Join);
	VDocument.SetMethod("void clear()", &Rest::Document::Clear);
	VDocument.SetMethod("void save()", &Rest::Document::Save);
	VDocument.SetMethod("document@+ copy()", &Rest::Document::Copy);
	VDocument.SetMethod("bool isAttribute()", &Rest::Document::IsAttribute);
	VDocument.SetMethod("bool isObject()", &Rest::Document::IsObject);
	VDocument.SetMethod("bool deserialize(const string &in)", &Rest::Document::Deserialize);
	VDocument.SetMethod("uint64 size()", &Rest::Document::Size);
	VDocument.SetMethod("string getName()", &Rest::Document::GetName);
	VDocument.SetMethod("string serialize()", &Rest::Document::Serialize);
	VDocument.SetMethod("document@+ getIndex(uint64)", &Rest::Document::GetIndex);
	VDocument.SetMethod("document@+ setCast(const string &in, const string &in)", &Rest::Document::SetCast);
	VDocument.SetMethod("document@+ setUndefined(const string &in)", &Rest::Document::SetUndefined);
	VDocument.SetMethod("document@+ setNull(const string &in)", &Rest::Document::SetNull);
	VDocument.SetMethod("document@+ setAttribute(const string &in, const string &in)", &Rest::Document::SetAttribute);
	VDocument.SetMethod("document@+ setInteger(const string &in, int64)", &Rest::Document::SetInteger);
	VDocument.SetMethod("document@+ setNumber(const string &in, double)", &Rest::Document::SetNumber);
	VDocument.SetMethod("document@+ setBoolean(const string &in, bool)", &Rest::Document::SetBoolean);
	VDocument.SetMethod("document@+ get(const string &in, bool = true)", &Rest::Document::FindPath);
	VDocument.SetMethod("document@+ getParent()", &Rest::Document::GetParent);
	VDocument.SetMethod("document@+ getAttribute(const string &in)", &Rest::Document::GetAttribute);
	VDocument.SetMethod<Rest::Document, Rest::Document*, const std::string&>("document@+ setDocument(const string &in)", &Rest::Document::SetDocument);
	VDocument.SetMethod<Rest::Document, Rest::Document*, const std::string&>("document@+ setArray(const string &in)", &Rest::Document::SetArray);
	VDocument.SetMethod<Rest::Document, Rest::Document*, const std::string&, int64_t, int64_t>("document@+ setDecimal(const string &in, int64, int64)", &Rest::Document::SetDecimal);
	VDocument.SetMethod<Rest::Document, Rest::Document*, const std::string&, const std::string&>("document@+ setDecimal(const string &in, const string &in)", &Rest::Document::SetDecimal);
	VDocument.SetMethod<Rest::Document, Rest::Document*, const std::string&, const std::string&>("document@+ setString(const string &in, const string &in)", &Rest::Document::SetString);
	VDocument.SetMethodEx("document@+ setDocument(const string &in, document@+)", &VMCDocument::SetDocument);
	VDocument.SetMethodEx("document@+ setArray(const string &in, document@+)", &VMCDocument::SetArray);
	VDocument.SetMethodEx("bool getBoolean(const string &in)", &VMCDocument::GetBoolean);
	VDocument.SetMethodEx("bool getNull(const string &in)", &VMCDocument::GetNull);
	VDocument.SetMethodEx("bool getUndefined(const string &in)", &VMCDocument::GetUndefined);
	VDocument.SetMethodEx("uint64 getInteger(const string &in)", &VMCDocument::GetInteger);
	VDocument.SetMethodEx("double getNumber(const string &in)", &VMCDocument::GetNumber);
	VDocument.SetMethodEx("string getString(const string &in)", &VMCDocument::GetString);
	VDocument.SetMethodEx("document@+ setId(const string &in, const string &in)", &VMCDocument::SetId);
	VDocument.SetMethodEx("document@+ getAny(const string &in, bool = true)", &VMCDocument::GetAny);
	VDocument.SetMethodEx("string getDecimal(const string &in)", &VMCDocument::GetDecimal);
	VDocument.SetMethodEx("string getId(const string &in)", &VMCDocument::GetId);
	VDocument.SetMethodEx("array<document@>@ findCollection(const string &in, bool)", &VMCDocument::FindCollection);
	VDocument.SetMethodEx("array<document@>@ findCollectionPath(const string &in, bool)", &VMCDocument::FindCollectionPath);
	VDocument.SetMethodEx("array<document@>@ getAttributes()", &VMCDocument::GetAttributes);
	VDocument.SetMethodEx("array<document@>@ getNodes()", &VMCDocument::GetNodes);
	VDocument.SetMethodEx("dictionary@ createMapping()", &VMCDocument::CreateMapping);
	VDocument.SetMethodEx("string toJSON()", &VMCDocument::ToJSON);
	VDocument.SetMethodEx("string toXML()", &VMCDocument::ToXML);
	VDocument.SetMethodEx("bool has(const string &in)", &VMCDocument::Has);
	VDocument.SetMethodEx("bool hasId(const string &in)", &VMCDocument::HasId);
	VDocument.SetMethodStatic("document@ fromJSON(const string &in)", &VMCDocument::FromJSON);
	VDocument.SetMethodStatic("document@ fromXML(const string &in)", &VMCDocument::FromXML);
	VDocument.SetMethodStatic("document@ load(const string &in)", &VMCDocument::Import);
}