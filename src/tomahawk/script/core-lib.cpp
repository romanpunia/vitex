#include "core-lib.h"
#ifndef ANGELSCRIPT_H 
#include <angelscript.h>
#endif

namespace Tomahawk
{
	namespace Script
	{
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

				Core::Parser Result; std::string Offset;
				FormatBuffer(Global, Result, Offset, (void*)Buffer, TypeId);
				Args.push_back(Result.R()[0] == '\n' ? Result.Substring(1).R() : Result.R());

				if (TypeId & VMTypeId_MASK_OBJECT)
				{
					VMTypeInfo Type = Global.GetTypeInfoById(TypeId);
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
			Core::Parser Result;

			FormatJSON(Global, Result, Ref, TypeId);
			return Result.R();
		}
		std::string VMCFormat::Form(const std::string& F, const VMCFormat& Form)
		{
			Core::Parser Buffer = F;
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
		void VMCFormat::WriteLine(Core::Console* Base, const std::string& F, VMCFormat* Form)
		{
			Core::Parser Buffer = F;
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
		void VMCFormat::Write(Core::Console* Base, const std::string& F, VMCFormat* Form)
		{
			Core::Parser Buffer = F;
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
		void VMCFormat::FormatBuffer(VMGlobal& Global, Core::Parser& Result, std::string& Offset, void* Ref, int TypeId)
		{
			if (TypeId < VMTypeId_BOOL || TypeId > VMTypeId_DOUBLE)
			{
				VMTypeInfo Type = Global.GetTypeInfoById(TypeId);
				if (!Ref)
				{
					Result.Append("null");
					return;
				}

				if (Global.GetManager()->IsNullable(TypeId))
				{
					if (TypeId == 0)
						TH_WARN("[memerr] use nullptr instead of null for initialization lists");

					Result.Append("null");
				}
				else if (VMTypeInfo::IsScriptObject(TypeId))
				{
					VMObject VObject = *(VMCObject**)Ref;
					Core::Parser Decl;

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
				else if (strcmp(Type.GetName(), "Map") == 0)
				{
					VMCMap* Map = *(VMCMap**)Ref;
					Core::Parser Decl; std::string Name;

					Offset += '\t';
					for (unsigned int i = 0; i < Map->GetSize(); i++)
					{
						void* ElementRef; int ElementTypeId;
						if (!Map->GetIndex(i, &Name, &ElementRef, &ElementTypeId))
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
				else if (strcmp(Type.GetName(), "Array") == 0)
				{
					VMCArray* Array = *(VMCArray**)Ref;
					int ArrayTypeId = Array->GetElementTypeId();
					Core::Parser Decl;

					Offset += '\t';
					for (unsigned int i = 0; i < Array->GetSize(); i++)
					{
						Decl.Append(Offset);
						FormatBuffer(Global, Decl, Offset, Array->At(i), ArrayTypeId);
						Decl.Append(", ");
					}

					Offset = Offset.substr(0, Offset.size() - 1);
					if (!Decl.Empty())
						Result.fAppend("\n%s[\n%s\n%s]", Offset.c_str(), Decl.Clip(Decl.Size() - 2).Get(), Offset.c_str());
					else
						Result.Append("[]");
				}
				else if (strcmp(Type.GetName(), "String") != 0)
				{
					Core::Parser Decl;
					Offset += '\t';

					Type.ForEachProperty([&Decl, &Global, &Offset, Ref, TypeId](VMTypeInfo* Base, VMFuncProperty* Prop)
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
		void VMCFormat::FormatJSON(VMGlobal& Global, Core::Parser& Result, void* Ref, int TypeId)
		{
			if (TypeId < VMTypeId_BOOL || TypeId > VMTypeId_DOUBLE)
			{
				VMTypeInfo Type = Global.GetTypeInfoById(TypeId);
				void* Object = Type.GetInstance<void>(Ref, TypeId);

				if (!Object)
				{
					Result.Append("null");
					return;
				}

				if (VMTypeInfo::IsScriptObject(TypeId))
				{
					VMObject VObject = (VMCObject*)Object;
					Core::Parser Decl;

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
				else if (strcmp(Type.GetName(), "Map") == 0)
				{
					VMCMap* Map = (VMCMap*)Object;
					Core::Parser Decl; std::string Name;

					for (unsigned int i = 0; i < Map->GetSize(); i++)
					{
						void* ElementRef; int ElementTypeId;
						if (!Map->GetIndex(i, &Name, &ElementRef, &ElementTypeId))
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
				else if (strcmp(Type.GetName(), "Array") == 0)
				{
					VMCArray* Array = (VMCArray*)Object;
					int ArrayTypeId = Array->GetElementTypeId();
					Core::Parser Decl;

					for (unsigned int i = 0; i < Array->GetSize(); i++)
					{
						FormatJSON(Global, Decl, Array->At(i), ArrayTypeId);
						Decl.Append(",");
					}

					if (!Decl.Empty())
						Result.fAppend("[%s]", Decl.Clip(Decl.Size() - 1).Get());
					else
						Result.Append("[]");
				}
				else if (strcmp(Type.GetName(), "String") != 0)
				{
					Core::Parser Decl;
					Type.ForEachProperty([&Decl, &Global, Ref, TypeId](VMTypeInfo* Base, VMFuncProperty* Prop)
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

		uint64_t VMCVariant::GetSize(Core::Variant& Base)
		{
			return (uint64_t)Base.GetSize();
		}
		bool VMCVariant::Equals(Core::Variant& Base, const Core::Variant& Other)
		{
			return Base == Other;
		}
		bool VMCVariant::ImplCast(Core::Variant& Base)
		{
			return Base;
		}

		void VMCDocument::Construct(VMCGeneric* Generic)
		{
			unsigned char* Buffer = (unsigned char*)Generic->GetArgAddress(0);
			*(Core::Document**)Generic->GetAddressOfReturnLocation() = ConstructBuffer(Buffer);
		}
		Core::Document* VMCDocument::ConstructBuffer(unsigned char* Buffer)
		{
			if (!Buffer)
				return nullptr;

			Core::Document* Result = Core::Document::Object();
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
							Result->Set(Name, std::move(Core::Var::Boolean(*(bool*)Ref)));
							break;
						case asTYPEID_INT8:
							Result->Set(Name, std::move(Core::Var::Integer(*(char*)Ref)));
							break;
						case asTYPEID_INT16:
							Result->Set(Name, std::move(Core::Var::Integer(*(short*)Ref)));
							break;
						case asTYPEID_INT32:
							Result->Set(Name, std::move(Core::Var::Integer(*(int*)Ref)));
							break;
						case asTYPEID_UINT8:
							Result->Set(Name, std::move(Core::Var::Integer(*(unsigned char*)Ref)));
							break;
						case asTYPEID_UINT16:
							Result->Set(Name, std::move(Core::Var::Integer(*(unsigned short*)Ref)));
							break;
						case asTYPEID_UINT32:
							Result->Set(Name, std::move(Core::Var::Integer(*(unsigned int*)Ref)));
							break;
						case asTYPEID_INT64:
						case asTYPEID_UINT64:
							Result->Set(Name, std::move(Core::Var::Integer(*(asINT64*)Ref)));
							break;
						case asTYPEID_FLOAT:
							Result->Set(Name, std::move(Core::Var::Number(*(float*)Ref)));
							break;
						case asTYPEID_DOUBLE:
							Result->Set(Name, std::move(Core::Var::Number(*(double*)Ref)));
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

					if (VMManager::Get(Manager)->IsNullable(TypeId) || !Ref)
					{
						Result->Set(Name, std::move(Core::Var::Null()));
					}
					else if (Type && !strcmp("Document", Type->GetName()))
					{
						Core::Document* Base = (Core::Document*)Ref;
						if (Base->GetParent() != Result)
							Base->AddRef();

						Result->Set(Name, Base);
					}
					else if (Type && !strcmp("String", Type->GetName()))
						Result->Set(Name, std::move(Core::Var::String(*(std::string*)Ref)));
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
				{
					TH_WARN("[memerr] use nullptr instead of null for initialization lists");
					Buffer += sizeof(void*);
				}
				else
					Buffer += Manager->GetSizeOfPrimitiveType(TypeId);
			}

			return Result;
		}
		Core::Document* VMCDocument::GetIndex(Core::Document* Base, const std::string& Name)
		{
			Core::Document* Result = Base->Fetch(Name);
			if (Result != nullptr)
				return Result;

			return Base->Set(Name, std::move(Core::Var::Undefined()));
		}
		Core::Document* VMCDocument::GetIndexOffset(Core::Document* Base, uint64_t Offset)
		{
			return Base->Get(Offset);
		}
		Core::Document* VMCDocument::Set(Core::Document* Base, const std::string& Name, Core::Document* Value)
		{
			if (Value != nullptr && Value->GetParent() != Base)
				Value->AddRef();

			return Base->Set(Name, Value);
		}
		Core::Document* VMCDocument::Push(Core::Document* Base, Core::Document* Value)
		{
			if (Value != nullptr)
				Value->AddRef();

			return Base->Push(Value);
		}
		VMCArray* VMCDocument::GetCollection(Core::Document* Base, const std::string& Name, bool Deep)
		{
			VMContext* Context = VMContext::Get();
			if (!Context)
				return nullptr;

			VMManager* Manager = Context->GetManager();
			if (!Manager)
				return nullptr;

			std::vector<Core::Document*> Nodes = Base->FetchCollection(Name, Deep);
			for (auto& Node : Nodes)
				Node->AddRef();

			VMTypeInfo Type = Manager->Global().GetTypeInfoByDecl("Array<Document@>@");
			return VMCArray::ComposeFromPointers(Type.GetTypeInfo(), Nodes);
		}
		VMCArray* VMCDocument::GetNodes(Core::Document* Base)
		{
			VMContext* Context = VMContext::Get();
			if (!Context)
				return nullptr;

			VMManager* Manager = Context->GetManager();
			if (!Manager)
				return nullptr;

			VMTypeInfo Type = Manager->Global().GetTypeInfoByDecl("Array<Document@>@");
			return VMCArray::ComposeFromPointers(Type.GetTypeInfo(), *Base->GetNodes());
		}
		VMCArray* VMCDocument::GetAttributes(Core::Document* Base)
		{
			VMContext* Context = VMContext::Get();
			if (!Context)
				return nullptr;

			VMManager* Manager = Context->GetManager();
			if (!Manager)
				return nullptr;

			VMTypeInfo Type = Manager->Global().GetTypeInfoByDecl("Array<Document@>@");
			return VMCArray::ComposeFromPointers(Type.GetTypeInfo(), Base->GetAttributes());
		}
		VMCMap* VMCDocument::GetNames(Core::Document* Base)
		{
			VMContext* Context = VMContext::Get();
			if (!Context)
				return nullptr;

			VMManager* Manager = Context->GetManager();
			if (!Manager)
				return nullptr;

			std::unordered_map<std::string, uint64_t> Mapping = Base->GetNames();
			VMCMap* Map = VMCMap::Create(Manager->GetEngine());

			for (auto& Item : Mapping)
				Map->Set(Item.first, &Item.second, VMTypeId_INT64);

			return Map;
		}
		std::string VMCDocument::ToJSON(Core::Document* Base)
		{
			std::string Stream;
			Core::Document::WriteJSON(Base, [&Stream](Core::VarForm, const char* Buffer, int64_t Length)
			{
				if (Buffer != nullptr && Length > 0)
					Stream.append(Buffer, Length);
			});

			return Stream;
		}
		std::string VMCDocument::ToXML(Core::Document* Base)
		{
			std::string Stream;
			Core::Document::WriteXML(Base, [&Stream](Core::VarForm, const char* Buffer, int64_t Length)
			{
				if (Buffer != nullptr && Length > 0)
					Stream.append(Buffer, Length);
			});

			return Stream;
		}
		std::string VMCDocument::ToString(Core::Document* Base)
		{
			switch (Base->Value.GetType())
			{
				case Core::VarType_Null:
				case Core::VarType_Undefined:
				case Core::VarType_Object:
				case Core::VarType_Array:
				case Core::VarType_Pointer:
					break;
				case Core::VarType_String:
				case Core::VarType_Base64:
					return Base->Value.GetBlob();
				case Core::VarType_Integer:
					return std::to_string(Base->Value.GetInteger());
				case Core::VarType_Number:
					return std::to_string(Base->Value.GetNumber());
				case Core::VarType_Decimal:
					return Base->Value.GetDecimal();
				case Core::VarType_Boolean:
					return Base->Value.GetBoolean() ? "1" : "0";
			}

			return "";
		}
		std::string VMCDocument::ToBase64(Core::Document* Base)
		{
			return Base->Value.GetBlob();
		}
		int64_t VMCDocument::ToInteger(Core::Document* Base)
		{
			return Base->Value.GetInteger();
		}
		double VMCDocument::ToNumber(Core::Document* Base)
		{
			return Base->Value.GetNumber();
		}
		std::string VMCDocument::ToDecimal(Core::Document* Base)
		{
			return Base->Value.GetDecimal();
		}
		bool VMCDocument::ToBoolean(Core::Document* Base)
		{
			return Base->Value.GetBoolean();
		}
		Core::Document* VMCDocument::FromJSON(const std::string& Value)
		{
			size_t Offset = 0;
			return Core::Document::ReadJSON(Value.size(), [&Value, &Offset](char* Buffer, int64_t Size)
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
		Core::Document* VMCDocument::FromXML(const std::string& Value)
		{
			size_t Offset = 0;
			return Core::Document::ReadXML(Value.size(), [&Value, &Offset](char* Buffer, int64_t Size)
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
		Core::Document* VMCDocument::Import(const std::string& Value)
		{
			VMManager* Manager = VMManager::Get();
			if (!Manager)
				return nullptr;

			return Manager->ImportJSON(Value);
		}
		Core::Document* VMCDocument::CopyAssign(Core::Document* Base, const Core::Variant& Other)
		{
			Base->Value = Other;
			return Base;
		}
		bool VMCDocument::Equals(Core::Document* Base, Core::Document* Other)
		{
			if (Other != nullptr)
				return Base->Value == Other->Value;

			Core::VarType Type = Base->Value.GetType();
			return (Type == Core::VarType_Null || Type == Core::VarType_Undefined);
		}

		bool RegisterFormatAPI(VMManager* Engine)
		{
			if (!Engine)
				return false;

			VMGlobal& Register = Engine->Global();

			VMRefClass VFormat = Register.SetClassUnmanaged<VMCFormat>("Format");
			VFormat.SetUnmanagedConstructor<VMCFormat>("Format@ f()");
			VFormat.SetUnmanagedConstructorList<VMCFormat>("Format@ f(int &in) {repeat ?}");
			VFormat.SetMethodStatic("String JSON(const ? &in)", &VMCFormat::JSON);
			
			return true;
		}
		bool RegisterConsoleAPI(VMManager* Engine)
		{
			if (!Engine)
				return false;

			VMGlobal& Register = Engine->Global();

			VMRefClass VConsole = Register.SetClassUnmanaged<Core::Console>("Console");
			VConsole.SetMethod("void Hide()", &Core::Console::Hide);
			VConsole.SetMethod("void Show()", &Core::Console::Show);
			VConsole.SetMethod("void Clear()", &Core::Console::Clear);
			VConsole.SetMethod("void Flush()", &Core::Console::Flush);
			VConsole.SetMethod("void FlushWrite()", &Core::Console::FlushWrite);
			VConsole.SetMethod("void CaptureTime()", &Core::Console::CaptureTime);
			VConsole.SetMethod("void WriteLine(const String &in)", &Core::Console::sWriteLine);
			VConsole.SetMethod("void Write(const String &in)", &Core::Console::sWrite);
			VConsole.SetMethod("double GetCapturedTime()", &Core::Console::GetCapturedTime);
			VConsole.SetMethod("String Read(uint64)", &Core::Console::Read);
			VConsole.SetMethodStatic("Console@+ Get()", &Core::Console::Get);
			VConsole.SetMethodEx("void WriteLine(const String &in, Format@+)", &VMCFormat::WriteLine);
			VConsole.SetMethodEx("void Write(const String &in, Format@+)", &VMCFormat::Write);

			return true;
		}
		bool RegisterVariantAPI(VMManager* Engine)
		{
			if (!Engine)
				return false;

			VMGlobal& Register = Engine->Global();

			VMEnum VVarType = Register.SetEnum("VarType");
			VVarType.SetValue("Null", Core::VarType_Null);
			VVarType.SetValue("Undefined", Core::VarType_Undefined);
			VVarType.SetValue("Object", Core::VarType_Object);
			VVarType.SetValue("Array", Core::VarType_Array);
			VVarType.SetValue("String", Core::VarType_String);
			VVarType.SetValue("Base64", Core::VarType_Base64);
			VVarType.SetValue("Integer", Core::VarType_Integer);
			VVarType.SetValue("Number", Core::VarType_Number);
			VVarType.SetValue("Decimal", Core::VarType_Decimal);
			VVarType.SetValue("Boolean", Core::VarType_Boolean);

			VMTypeClass VVariant = Register.SetStructUnmanaged<Core::Variant>("Variant");
			VVariant.SetConstructor<Core::Variant, const Core::Variant&>("void f(const Variant &in)");
			VVariant.SetMethod("bool Deserialize(const String &in, bool = false)", &Core::Variant::Deserialize);
			VVariant.SetMethod("String Serialize() const", &Core::Variant::Serialize);
			VVariant.SetMethod("String GetDecimal() const", &Core::Variant::GetDecimal);
			VVariant.SetMethod("String GetBlob() const", &Core::Variant::GetBlob);
			VVariant.SetMethod("Address@ GetPointer() const", &Core::Variant::GetPointer);
			VVariant.SetMethod("int64 GetInteger() const", &Core::Variant::GetInteger);
			VVariant.SetMethod("double GetNumber() const", &Core::Variant::GetNumber);
			VVariant.SetMethod("bool GetBoolean() const", &Core::Variant::GetBoolean);
			VVariant.SetMethod("VarType GetType() const", &Core::Variant::GetType);
			VVariant.SetMethod("bool IsObject() const", &Core::Variant::IsObject);
			VVariant.SetMethod("bool IsEmpty() const", &Core::Variant::IsEmpty);
			VVariant.SetMethodEx("uint64 GetSize() const", &VMCVariant::GetSize);
			VVariant.SetOperatorEx(VMOpFunc_Equals, VMOp_Left | VMOp_Const, "bool", "const Variant &in", &VMCVariant::Equals);
			VVariant.SetOperatorEx(VMOpFunc_ImplCast, VMOp_Left | VMOp_Const, "bool", "", &VMCVariant::ImplCast);

			Engine->BeginNamespace("Var");
			Register.SetFunction("Variant Auto(const String &in, bool = false)", &Core::Var::Auto);
			Register.SetFunction("Variant Null()", &Core::Var::Null);
			Register.SetFunction("Variant Undefined()", &Core::Var::Undefined);
			Register.SetFunction("Variant Object()", &Core::Var::Object);
			Register.SetFunction("Variant Array()", &Core::Var::Array);
			Register.SetFunction("Variant Pointer(Address@)", &Core::Var::Pointer);
			Register.SetFunction("Variant Integer(int64)", &Core::Var::Integer);
			Register.SetFunction("Variant Number(double)", &Core::Var::Number);
			Register.SetFunction("Variant Boolean(bool)", &Core::Var::Boolean);
			Register.SetFunction<Core::Variant(const std::string&)>("Variant String(const String &in)", &Core::Var::String);
			Register.SetFunction<Core::Variant(const std::string&)>("Variant Base64(const String &in)", &Core::Var::Base64);
			Register.SetFunction<Core::Variant(const std::string&)>("Variant Decimal(const String &in)", &Core::Var::Decimal);
			Engine->EndNamespace();

			return true;
		}
		bool RegisterDocumentAPI(VMManager* Engine)
		{
			if (!Engine)
				return false;

			VMRefClass VDocument = Engine->Global().SetClassUnmanaged<Core::Document>("Document");
			VDocument.SetProperty<Core::Document>("String Key", &Core::Document::Key);
			VDocument.SetProperty<Core::Document>("Variant Value", &Core::Document::Value);
			VDocument.SetUnmanagedConstructor<Core::Document, const Core::Variant&>("Document@ f(const Variant &in)");
			VDocument.SetUnmanagedConstructorListEx<Core::Document>("Document@ f(int &in) {repeat {String, ?}}", &VMCDocument::Construct);	
			VDocument.SetMethod<Core::Document, Core::Variant, size_t>("Variant GetVar(uint) const", &Core::Document::GetVar);
			VDocument.SetMethod<Core::Document, Core::Variant, const std::string&>("Variant GetVar(const String &in) const", &Core::Document::GetVar);
			VDocument.SetMethod("Document@+ GetParent() const", &Core::Document::GetParent);
			VDocument.SetMethod("Document@+ GetAttribute(const String &in) const", &Core::Document::GetAttribute);
			VDocument.SetMethod<Core::Document, Core::Document*, size_t>("Document@+ Get(uint) const", &Core::Document::Get);
			VDocument.SetMethod<Core::Document, Core::Document*, const std::string&, bool>("Document@+ Get(const String &in, bool = false) const", &Core::Document::Fetch);
			VDocument.SetMethod<Core::Document, Core::Document*, const std::string&>("Document@+ Set(const String &in)", &Core::Document::Set);
			VDocument.SetMethod<Core::Document, Core::Document*, const std::string&, const Core::Variant&>("Document@+ Set(const String &in, const Variant &in)", &Core::Document::Set);
			VDocument.SetMethod<Core::Document, Core::Document*, const std::string&, const Core::Variant&>("Document@+ SetAttribute(const String& in, const Variant &in)", &Core::Document::SetAttribute);
			VDocument.SetMethod<Core::Document, Core::Document*, const Core::Variant&>("Document@+ Push(const Variant &in)", &Core::Document::Push);
			VDocument.SetMethod<Core::Document, Core::Document*, size_t>("Document@+ Pop(uint)", &Core::Document::Pop);
			VDocument.SetMethod<Core::Document, Core::Document*, const std::string&>("Document@+ Pop(const String &in)", &Core::Document::Pop);
			VDocument.SetMethod("Document@ Copy() const", &Core::Document::Copy);
			VDocument.SetMethod("bool Has(const String &in) const", &Core::Document::Has);
			VDocument.SetMethod("bool Has64(const String &in, uint = 12) const", &Core::Document::Has64);
			VDocument.SetMethod("bool IsAttribute() const", &Core::Document::IsAttribute);
			VDocument.SetMethod("bool IsSaved() const", &Core::Document::IsAttribute);
			VDocument.SetMethod("int64 Size() const", &Core::Document::Size);
			VDocument.SetMethod("String GetName() const", &Core::Document::GetName);
			VDocument.SetMethod("void Join(Document@+)", &Core::Document::Join);
			VDocument.SetMethod("void Clear()", &Core::Document::Clear);
			VDocument.SetMethod("void Save()", &Core::Document::Save);
			VDocument.SetMethodEx("Document@+ Set(const String &in, Document@+)", &VMCDocument::Set);
			VDocument.SetMethodEx("Document@+ Push(Document@+)", &VMCDocument::Push);
			VDocument.SetMethodEx("Array<Document@>@ GetCollection(const String &in, bool = false) const", &VMCDocument::GetCollection);
			VDocument.SetMethodEx("Array<Document@>@ GetAttributes() const", &VMCDocument::GetAttributes);
			VDocument.SetMethodEx("Array<Document@>@ GetNodes() const", &VMCDocument::GetNodes);
			VDocument.SetMethodEx("Map@ GetNames() const", &VMCDocument::GetNames);
			VDocument.SetMethodEx("String JSON() const", &VMCDocument::ToJSON);
			VDocument.SetMethodEx("String XML() const", &VMCDocument::ToXML);
			VDocument.SetMethodEx("String Str() const", &VMCDocument::ToString);
			VDocument.SetMethodEx("String B64() const", &VMCDocument::ToBase64);
			VDocument.SetMethodEx("int64 Int() const", &VMCDocument::ToInteger);
			VDocument.SetMethodEx("double Num() const", &VMCDocument::ToNumber);
			VDocument.SetMethodEx("String Dec() const", &VMCDocument::ToDecimal);
			VDocument.SetMethodEx("bool Bool() const", &VMCDocument::ToBoolean);
			VDocument.SetMethodStatic("Document@ Object()", &Core::Document::Object);
			VDocument.SetMethodStatic("Document@ Array()", &Core::Document::Array);
			VDocument.SetMethodStatic("Document@ FromJSON(const String &in)", &VMCDocument::FromJSON);
			VDocument.SetMethodStatic("Document@ FromXML(const String &in)", &VMCDocument::FromXML);
			VDocument.SetMethodStatic("Document@ Import(const String &in)", &VMCDocument::Import);
			VDocument.SetOperatorEx(VMOpFunc_Assign, VMOp_Left, "Document@+", "const Variant &in", &VMCDocument::CopyAssign);
			VDocument.SetOperatorEx(VMOpFunc_Equals, VMOp_Left | VMOp_Const, "bool", "Document@+", &VMCDocument::Equals);
			VDocument.SetOperatorEx(VMOpFunc_Index, VMOp_Left, "Document@+", "const String &in", &VMCDocument::GetIndex);
			VDocument.SetOperatorEx(VMOpFunc_Index, VMOp_Left, "Document@+", "uint64", &VMCDocument::GetIndexOffset);

			return true;
		}
	}
}