#include "rest-api.h"
#include "../network/bson.h"
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

				Rest::Stroke Result; std::string Offset;
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
				else if (strcmp(Type.GetName(), "Map") == 0)
				{
					VMCMap* Map = *(VMCMap**)Ref;
					Rest::Stroke Decl; std::string Name;

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
					Rest::Stroke Decl;

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
					Rest::Stroke Decl;
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
		void VMCFormat::FormatJSON(VMGlobal& Global, Rest::Stroke& Result, void* Ref, int TypeId)
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
				else if (strcmp(Type.GetName(), "Map") == 0)
				{
					VMCMap* Map = (VMCMap*)Object;
					Rest::Stroke Decl; std::string Name;

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
					Rest::Stroke Decl;

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
					Rest::Stroke Decl;
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

		void VMCDocument::Construct(VMCGeneric* Generic)
		{
			unsigned char* Buffer = (unsigned char*)Generic->GetArgAddress(0);
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
		Rest::Document* VMCDocument::ConstructBuffer(unsigned char* Buffer)
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

					if (VMManager::Get(Manager)->IsNullable(TypeId) || !Ref)
					{
						Result->SetNull(Name);
					}
					else if (Type && !strcmp("Document", Type->GetName()))
					{
						Rest::Document* Base = (Rest::Document*)Ref;
						Base->AddRef();

						if (Base->Type == Rest::NodeType_Array)
							Result->SetArray(Name, Base);
						else
							Result->SetDocument(Name, Base);
					}
					else if (Type && !strcmp("String", Type->GetName()))
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
				{
					TH_WARN("[memerr] use nullptr instead of null for initialization lists");
					Buffer += sizeof(void*);
				}
				else
					Buffer += Manager->GetSizeOfPrimitiveType(TypeId);
			}

			return Result;
		}
		Rest::Document* VMCDocument::GetIndex(Rest::Document* Base, const std::string& Name)
		{
			return Base->FindPath(Name, true);
		}
		Rest::Document* VMCDocument::GetIndexOffset(Rest::Document* Base, uint64_t Offset)
		{
			return Base->GetIndex(Offset);
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

			return Result->String;
		}
		VMCArray* VMCDocument::FindCollection(Rest::Document* Base, const std::string& Name, bool Here)
		{
			VMContext* Context = VMContext::Get();
			if (!Context)
				return nullptr;

			VMManager* Manager = Context->GetManager();
			if (!Manager)
				return nullptr;

			VMTypeInfo Type = Manager->Global().GetTypeInfoByDecl("Array<Document@>@");
			return VMCArray::ComposeFromPointers(Type.GetTypeInfo(), Base->FindCollection(Name, Here));
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

			VMTypeInfo Type = Manager->Global().GetTypeInfoByDecl("Array<Document@>@");
			return VMCArray::ComposeFromPointers(Type.GetTypeInfo(), Nodes);
		}
		VMCArray* VMCDocument::GetNodes(Rest::Document* Base)
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
		VMCArray* VMCDocument::GetAttributes(Rest::Document* Base)
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
		VMCMap* VMCDocument::CreateMapping(Rest::Document* Base)
		{
			VMContext* Context = VMContext::Get();
			if (!Context)
				return nullptr;

			VMManager* Manager = Context->GetManager();
			if (!Manager)
				return nullptr;

			std::unordered_map<std::string, uint64_t> Mapping = Base->CreateMapping();
			VMCMap* Map = VMCMap::Create(Manager->GetEngine());

			for (auto& Item : Mapping)
			{
				as_int64_t V = Item.second;
				Map->Set(Item.first, V);
			}

			return Map;
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

			VMRefClass VConsole = Register.SetClassUnmanaged<Rest::Console>("Console");
			VConsole.SetUnmanagedConstructor<Rest::Console>("Console@ f()");
			VConsole.SetMethod("void Hide()", &Rest::Console::Hide);
			VConsole.SetMethod("void Show()", &Rest::Console::Show);
			VConsole.SetMethod("void Clear()", &Rest::Console::Clear);
			VConsole.SetMethod("void Flush()", &Rest::Console::Flush);
			VConsole.SetMethod("void FlushWrite()", &Rest::Console::FlushWrite);
			VConsole.SetMethod("void CaptureTime()", &Rest::Console::CaptureTime);
			VConsole.SetMethod("void WriteLine(const String &in)", &Rest::Console::sWriteLine);
			VConsole.SetMethod("void Write(const String &in)", &Rest::Console::sWrite);
			VConsole.SetMethod("double GetCapturedTime()", &Rest::Console::GetCapturedTime);
			VConsole.SetMethod("String Read(uint64)", &Rest::Console::Read);
			VConsole.SetMethodStatic("Console@+ Get()", &Rest::Console::Get);
			VConsole.SetMethodEx("void WriteLine(const String &in, Format@+)", &VMCFormat::WriteLine);
			VConsole.SetMethodEx("void Write(const String &in, Format@+)", &VMCFormat::Write);

			return true;
		}
		bool RegisterDocumentAPI(VMManager* Engine)
		{
			if (!Engine)
				return false;

			VMGlobal& Register = Engine->Global();

			VMEnum VNodeType = Register.SetEnum("NodeType");
			VNodeType.SetValue("Undefined", Rest::NodeType_Undefined);
			VNodeType.SetValue("Object", Rest::NodeType_Object);
			VNodeType.SetValue("Array", Rest::NodeType_Array);
			VNodeType.SetValue("String", Rest::NodeType_String);
			VNodeType.SetValue("Integer", Rest::NodeType_Integer);
			VNodeType.SetValue("Number", Rest::NodeType_Number);
			VNodeType.SetValue("Boolean", Rest::NodeType_Boolean);
			VNodeType.SetValue("Nullable", Rest::NodeType_Null);
			VNodeType.SetValue("Id", Rest::NodeType_Id);
			VNodeType.SetValue("Decimal", Rest::NodeType_Decimal);

			VMRefClass VDocument = Register.SetClassUnmanaged<Rest::Document>("Document");
			VDocument.SetProperty<Rest::Document>("String Name", &Rest::Document::Name);
			VDocument.SetProperty<Rest::Document>("String String", &Rest::Document::String);
			VDocument.SetProperty<Rest::Document>("NodeType Type", &Rest::Document::Type);
			VDocument.SetProperty<Rest::Document>("int64 Low", &Rest::Document::Low);
			VDocument.SetProperty<Rest::Document>("int64 Integer", &Rest::Document::Integer);
			VDocument.SetProperty<Rest::Document>("double Number", &Rest::Document::Number);
			VDocument.SetProperty<Rest::Document>("bool Boolean", &Rest::Document::Boolean);
			VDocument.SetProperty<Rest::Document>("bool Saved", &Rest::Document::Saved);
			VDocument.SetUnmanagedConstructor<Rest::Document>("Document@ f()");
			VDocument.SetUnmanagedConstructorListEx<Rest::Document>("Document@ f(int &in) {repeat {String, ?}}", &VMCDocument::Construct);
			VDocument.SetLOperatorEx(VMOpFunc_Index, "const Document@+", "const String &in", &VMCDocument::GetIndex);
			VDocument.SetLOperatorEx(VMOpFunc_Index, "const Document@+", "uint64", &VMCDocument::GetIndexOffset);
			VDocument.SetMethod("void Join(Document@+)", &Rest::Document::Join);
			VDocument.SetMethod("void Clear()", &Rest::Document::Clear);
			VDocument.SetMethod("void Save()", &Rest::Document::Save);
			VDocument.SetMethod("Document@ Copy()", &Rest::Document::Copy);
			VDocument.SetMethod("bool IsAttribute()", &Rest::Document::IsAttribute);
			VDocument.SetMethod("bool IsObject()", &Rest::Document::IsObject);
			VDocument.SetMethod("bool Deserialize(const String &in)", &Rest::Document::Deserialize);
			VDocument.SetMethod("uint64 Size()", &Rest::Document::Size);
			VDocument.SetMethod("String GetName()", &Rest::Document::GetName);
			VDocument.SetMethod("String Serialize()", &Rest::Document::Serialize);
			VDocument.SetMethod("Document@+ GetIndex(uint64)", &Rest::Document::GetIndex);
			VDocument.SetMethod("Document@+ SetCast(const String &in, const String &in)", &Rest::Document::SetCast);
			VDocument.SetMethod("Document@+ SetUndefined(const String &in)", &Rest::Document::SetUndefined);
			VDocument.SetMethod("Document@+ SetNull(const String &in)", &Rest::Document::SetNull);
			VDocument.SetMethod("Document@+ SetAttribute(const String &in, const String &in)", &Rest::Document::SetAttribute);
			VDocument.SetMethod("Document@+ SetInteger(const String &in, int64)", &Rest::Document::SetInteger);
			VDocument.SetMethod("Document@+ SetNumber(const String &in, double)", &Rest::Document::SetNumber);
			VDocument.SetMethod("Document@+ SetBoolean(const String &in, bool)", &Rest::Document::SetBoolean);
			VDocument.SetMethod("Document@+ Get(const String &in, bool = true)", &Rest::Document::FindPath);
			VDocument.SetMethod("Document@+ GetParent()", &Rest::Document::GetParent);
			VDocument.SetMethod("Document@+ GetAttribute(const String &in)", &Rest::Document::GetAttribute);
			VDocument.SetMethod<Rest::Document, Rest::Document*, const std::string&>("Document@+ SetDocument(const String &in)", &Rest::Document::SetDocument);
			VDocument.SetMethod<Rest::Document, Rest::Document*, const std::string&>("Document@+ SetArray(const String &in)", &Rest::Document::SetArray);
			VDocument.SetMethod<Rest::Document, Rest::Document*, const std::string&, int64_t, int64_t>("Document@+ SetDecimal(const String &in, int64, int64)", &Rest::Document::SetDecimal);
			VDocument.SetMethod<Rest::Document, Rest::Document*, const std::string&, const std::string&>("Document@+ SetDecimal(const String &in, const String &in)", &Rest::Document::SetDecimal);
			VDocument.SetMethod<Rest::Document, Rest::Document*, const std::string&, const std::string&>("Document@+ SetString(const String &in, const String &in)", &Rest::Document::SetString);
			VDocument.SetMethodEx("Document@+ SetDocument(const String &in, Document@+)", &VMCDocument::SetDocument);
			VDocument.SetMethodEx("Document@+ SetArray(const String &in, Document@+)", &VMCDocument::SetArray);
			VDocument.SetMethodEx("bool GetBoolean(const String &in)", &VMCDocument::GetBoolean);
			VDocument.SetMethodEx("bool GetNull(const String &in)", &VMCDocument::GetNull);
			VDocument.SetMethodEx("bool GetUndefined(const String &in)", &VMCDocument::GetUndefined);
			VDocument.SetMethodEx("uint64 GetInteger(const String &in)", &VMCDocument::GetInteger);
			VDocument.SetMethodEx("double GetNumber(const String &in)", &VMCDocument::GetNumber);
			VDocument.SetMethodEx("String GetString(const String &in)", &VMCDocument::GetString);
			VDocument.SetMethodEx("Document@+ SetId(const String &in, const String &in)", &VMCDocument::SetId);
			VDocument.SetMethodEx("Document@+ GetAny(const String &in, bool = true)", &VMCDocument::GetAny);
			VDocument.SetMethodEx("String GetDecimal(const String &in)", &VMCDocument::GetDecimal);
			VDocument.SetMethodEx("String GetId(const String &in)", &VMCDocument::GetId);
			VDocument.SetMethodEx("Array<Document@>@ FindCollection(const String &in, bool)", &VMCDocument::FindCollection);
			VDocument.SetMethodEx("Array<Document@>@ FindCollectionPath(const String &in, bool)", &VMCDocument::FindCollectionPath);
			VDocument.SetMethodEx("Array<Document@>@ GetAttributes()", &VMCDocument::GetAttributes);
			VDocument.SetMethodEx("Array<Document@>@ GetNodes()", &VMCDocument::GetNodes);
			VDocument.SetMethodEx("Map@ CreateMapping()", &VMCDocument::CreateMapping);
			VDocument.SetMethodEx("bool Has(const String &in)", &VMCDocument::Has);
			VDocument.SetMethodEx("bool HasId(const String &in)", &VMCDocument::HasId);
			VDocument.SetMethodEx("String ToJSON()", &VMCDocument::ToJSON);
			VDocument.SetMethodEx("String ToXML()", &VMCDocument::ToXML);
			VDocument.SetMethodStatic("Document@ FromJSON(const String &in)", &VMCDocument::FromJSON);
			VDocument.SetMethodStatic("Document@ FromXML(const String &in)", &VMCDocument::FromXML);
			VDocument.SetMethodStatic("Document@ Import(const String &in)", &VMCDocument::Import);

			return true;
		}
	}
}