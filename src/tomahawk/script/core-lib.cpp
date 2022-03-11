#include "core-lib.h"
#ifndef ANGELSCRIPT_H 
#include <angelscript.h>
#endif

namespace Tomahawk
{
	namespace Script
	{
		Core::Decimal DecimalNegate(Core::Decimal& Base)
		{
			return -Base;
		}
		Core::Decimal& DecimalMulEq(Core::Decimal& Base, const Core::Decimal& V)
		{
			Base *= V;
			return Base;
		}
		Core::Decimal& DecimalDivEq(Core::Decimal& Base, const Core::Decimal& V)
		{
			Base /= V;
			return Base;
		}
		Core::Decimal& DecimalAddEq(Core::Decimal& Base, const Core::Decimal& V)
		{
			Base += V;
			return Base;
		}
		Core::Decimal& DecimalSubEq(Core::Decimal& Base, const Core::Decimal& V)
		{
			Base -= V;
			return Base;
		}
		Core::Decimal& DecimalPP(Core::Decimal& Base)
		{
			Base++;
			return Base;
		}
		Core::Decimal& DecimalMM(Core::Decimal& Base)
		{
			Base--;
			return Base;
		}
		bool DecimalEq(Core::Decimal& Base, const Core::Decimal& Right)
		{
			return Base == Right;
		}
		int DecimalCmp(Core::Decimal& Base, const Core::Decimal& Right)
		{
			if (Base == Right)
				return 0;

			return Base > Right ? 1 : -1;
		}
		Core::Decimal DecimalAdd(const Core::Decimal& Left, const Core::Decimal& Right)
		{
			return Left + Right;
		}
		Core::Decimal DecimalSub(const Core::Decimal& Left, const Core::Decimal& Right)
		{
			return Left - Right;
		}
		Core::Decimal DecimalMul(const Core::Decimal& Left, const Core::Decimal& Right)
		{
			return Left * Right;
		}
		Core::Decimal DecimalDiv(const Core::Decimal& Left, const Core::Decimal& Right)
		{
			return Left / Right;
		}
		Core::Decimal DecimalPer(const Core::Decimal& Left, const Core::Decimal& Right)
		{
			return Left % Right;
		}

		Core::DateTime& DateTimeAddEq(Core::DateTime& Base, const Core::DateTime& V)
		{
			Base += V;
			return Base;
		}
		Core::DateTime& DateTimeSubEq(Core::DateTime& Base, const Core::DateTime& V)
		{
			Base -= V;
			return Base;
		}
		Core::DateTime& DateTimeSet(Core::DateTime& Base, const Core::DateTime& V)
		{
			Base = V;
			return Base;
		}
		bool DateTimeEq(Core::DateTime& Base, const Core::DateTime& Right)
		{
			return Base == Right;
		}
		int DateTimeCmp(Core::DateTime& Base, const Core::DateTime& Right)
		{
			if (Base == Right)
				return 0;

			return Base > Right ? 1 : -1;
		}
		Core::DateTime DateTimeAdd(const Core::DateTime& Left, const Core::DateTime& Right)
		{
			return Left + Right;
		}
		Core::DateTime DateTimeSub(const Core::DateTime& Left, const Core::DateTime& Right)
		{
			return Left - Right;
		}

		void ConsoleTrace(Core::Console* Base, uint32_t Frames)
		{
			VMContext* Context = VMContext::Get();
			if (Context != nullptr)
				return Base->WriteLine(Context->GetStackTrace(3, (size_t)Frames));

			TH_ERR("[vm] no active context for stack tracing");
		}

		uint64_t VariantGetSize(Core::Variant& Base)
		{
			return (uint64_t)Base.GetSize();
		}
		bool VariantEquals(Core::Variant& Base, const Core::Variant& Other)
		{
			return Base == Other;
		}
		bool VariantImplCast(Core::Variant& Base)
		{
			return Base;
		}

		STDArray* OSDirectoryScan(const std::string& Value)
		{
			std::vector<Core::ResourceEntry> Resources;
			if (!Core::OS::Directory::Scan(Value, &Resources))
				return nullptr;

			std::vector<std::string> Result;
			Result.reserve(Resources.size());
			for (auto& Item : Resources)
				Result.emplace_back(std::move(Item.Path));

			VMTypeInfo Type = VMManager::Get()->Global().GetTypeInfoByDecl("Array<String>@");
			return STDArray::Compose(Type.GetTypeInfo(), Result);
		}
		STDArray* OSDirectoryGetMounts()
		{
			VMTypeInfo Type = VMManager::Get()->Global().GetTypeInfoByDecl("Array<String>@");
			return STDArray::Compose(Type.GetTypeInfo(), Core::OS::Directory::GetMounts());
		}
		int OSProcessExecute(const std::string& Value)
		{
			return Core::OS::Process::Execute("%s", Value.c_str());
		}

		uint64_t FileStreamRead(Core::FileStream* Base, uint64_t Size, std::string* Result)
		{
			if (Size == 0)
				return 0;

			char* Buffer = (char*)TH_MALLOC(sizeof(char) * Size);
			memset(Buffer, 0, sizeof(char) * Size);
			uint64_t L = Base->Read(Buffer, Size);
			Result->assign(Buffer, (size_t)L);
			TH_FREE(Buffer);

			return L;
		}
		uint64_t GzStreamRead(Core::GzStream* Base, uint64_t Size, std::string* Result)
		{
			if (Size == 0)
				return 0;

			char* Buffer = (char*)TH_MALLOC(sizeof(char) * Size);
			memset(Buffer, 0, sizeof(char) * Size);
			uint64_t L = Base->Read(Buffer, Size);
			Result->assign(Buffer, (size_t)L);
			TH_FREE(Buffer);

			return L;
		}
		uint64_t WebStreamRead(Core::WebStream* Base, uint64_t Size, std::string* Result)
		{
			if (Size == 0)
				return 0;

			char* Buffer = (char*)TH_MALLOC(sizeof(char) * Size);
			memset(Buffer, 0, sizeof(char) * Size);
			uint64_t L = Base->Read(Buffer, Size);
			Result->assign(Buffer, (size_t)L);
			TH_FREE(Buffer);

			return L;
		}

		uint64_t ScheduleSetTimeout(Core::Schedule* Base, uint64_t MS, VMCFunction* Callback)
		{
			if (!Callback)
				return TH_INVALID_EVENT_ID;

			VMContext* Context = VMContext::Get();
			if (!Context)
				return TH_INVALID_EVENT_ID;

			Callback->AddRef();
			Context->AddRef();

			return Base->SetTimeout(MS, [Context, Callback]() mutable
			{
				Context->TryExecuteAsync(Callback, nullptr, nullptr).Await([Context, Callback](int&&)
				{
					Callback->Release();
					Context->Release();
				});
			}, Core::Difficulty::Light);
		}
		bool ScheduleSetTask(Core::Schedule* Base, VMCFunction* Callback)
		{
			if (!Callback)
				return TH_INVALID_EVENT_ID;

			VMContext* Context = VMContext::Get();
			if (!Context)
				return TH_INVALID_EVENT_ID;

			Callback->AddRef();
			Context->AddRef();

			return Base->SetTask([Context, Callback]() mutable
			{
				Context->TryExecuteAsync(Callback, nullptr, nullptr).Await([Context, Callback](int&&)
				{
					Callback->Release();
					Context->Release();
				});
			}, Core::Difficulty::Heavy);
		}

		Core::Schema* SchemaInit(Core::Schema* Base)
		{
			return Base;
		}
		Core::Schema* SchemaConstructBuffer(unsigned char* Buffer)
		{
			if (!Buffer)
				return nullptr;

			Core::Schema* Result = Core::Var::Set::Object();
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
							Result->Set(Name, Core::Var::Boolean(*(bool*)Ref));
							break;
						case asTYPEID_INT8:
							Result->Set(Name, Core::Var::Integer(*(char*)Ref));
							break;
						case asTYPEID_INT16:
							Result->Set(Name, Core::Var::Integer(*(short*)Ref));
							break;
						case asTYPEID_INT32:
							Result->Set(Name, Core::Var::Integer(*(int*)Ref));
							break;
						case asTYPEID_UINT8:
							Result->Set(Name, Core::Var::Integer(*(unsigned char*)Ref));
							break;
						case asTYPEID_UINT16:
							Result->Set(Name, Core::Var::Integer(*(unsigned short*)Ref));
							break;
						case asTYPEID_UINT32:
							Result->Set(Name, Core::Var::Integer(*(unsigned int*)Ref));
							break;
						case asTYPEID_INT64:
						case asTYPEID_UINT64:
							Result->Set(Name, Core::Var::Integer(*(asINT64*)Ref));
							break;
						case asTYPEID_FLOAT:
							Result->Set(Name, Core::Var::Number(*(float*)Ref));
							break;
						case asTYPEID_DOUBLE:
							Result->Set(Name, Core::Var::Number(*(double*)Ref));
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
						Result->Set(Name, Core::Var::Null());
					}
					else if (Type && !strcmp("Schema", Type->GetName()))
					{
						Core::Schema* Base = (Core::Schema*)Ref;
						if (Base->GetParent() != Result)
							Base->AddRef();

						Result->Set(Name, Base);
					}
					else if (Type && !strcmp("String", Type->GetName()))
						Result->Set(Name, Core::Var::String(*(std::string*)Ref));
				}

				if (TypeId & asTYPEID_MASK_OBJECT)
				{
					asITypeInfo* ti = Manager->GetTypeInfoById(TypeId);
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
		void SchemaConstruct(VMCGeneric* Generic)
		{
			unsigned char* Buffer = (unsigned char*)Generic->GetArgAddress(0);
			*(Core::Schema**)Generic->GetAddressOfReturnLocation() = SchemaConstructBuffer(Buffer);
		}
		Core::Schema* SchemaGetIndex(Core::Schema* Base, const std::string& Name)
		{
			Core::Schema* Result = Base->Fetch(Name);
			if (Result != nullptr)
				return Result;

			return Base->Set(Name, Core::Var::Undefined());
		}
		Core::Schema* SchemaGetIndexOffset(Core::Schema* Base, uint64_t Offset)
		{
			return Base->Get(Offset);
		}
		Core::Schema* SchemaSet(Core::Schema* Base, const std::string& Name, Core::Schema* Value)
		{
			if (Value != nullptr && Value->GetParent() != Base)
				Value->AddRef();

			return Base->Set(Name, Value);
		}
		Core::Schema* SchemaPush(Core::Schema* Base, Core::Schema* Value)
		{
			if (Value != nullptr)
				Value->AddRef();

			return Base->Push(Value);
		}
		STDArray* SchemaGetCollection(Core::Schema* Base, const std::string& Name, bool Deep)
		{
			VMContext* Context = VMContext::Get();
			if (!Context)
				return nullptr;

			VMManager* Manager = Context->GetManager();
			if (!Manager)
				return nullptr;

			std::vector<Core::Schema*> Nodes = Base->FetchCollection(Name, Deep);
			for (auto& Node : Nodes)
				Node->AddRef();

			VMTypeInfo Type = Manager->Global().GetTypeInfoByDecl("Array<Schema@>@");
			return STDArray::Compose(Type.GetTypeInfo(), Nodes);
		}
		STDArray* SchemaGetChilds(Core::Schema* Base)
		{
			VMContext* Context = VMContext::Get();
			if (!Context)
				return nullptr;

			VMManager* Manager = Context->GetManager();
			if (!Manager)
				return nullptr;

			VMTypeInfo Type = Manager->Global().GetTypeInfoByDecl("Array<Schema@>@");
			return STDArray::Compose(Type.GetTypeInfo(), Base->GetChilds());
		}
		STDArray* SchemaGetAttributes(Core::Schema* Base)
		{
			VMContext* Context = VMContext::Get();
			if (!Context)
				return nullptr;

			VMManager* Manager = Context->GetManager();
			if (!Manager)
				return nullptr;

			VMTypeInfo Type = Manager->Global().GetTypeInfoByDecl("Array<Schema@>@");
			return STDArray::Compose(Type.GetTypeInfo(), Base->GetAttributes());
		}
		STDMap* SchemaGetNames(Core::Schema* Base)
		{
			VMContext* Context = VMContext::Get();
			if (!Context)
				return nullptr;

			VMManager* Manager = Context->GetManager();
			if (!Manager)
				return nullptr;

			std::unordered_map<std::string, uint64_t> Mapping = Base->GetNames();
			STDMap* Map = STDMap::Create(Manager->GetEngine());

			for (auto& Item : Mapping)
				Map->Set(Item.first, &Item.second, (int)VMTypeId::INT64);

			return Map;
		}
		uint64_t SchemaGetSize(Core::Schema* Base)
		{
			return (uint64_t)Base->Size();
		}
		std::string SchemaToJSON(Core::Schema* Base)
		{
			std::string Stream;
			Core::Schema::WriteJSON(Base, [&Stream](Core::VarForm, const char* Buffer, int64_t Length)
			{
				if (Buffer != nullptr && Length > 0)
					Stream.append(Buffer, Length);
			});

			return Stream;
		}
		std::string SchemaToXML(Core::Schema* Base)
		{
			std::string Stream;
			Core::Schema::WriteXML(Base, [&Stream](Core::VarForm, const char* Buffer, int64_t Length)
			{
				if (Buffer != nullptr && Length > 0)
					Stream.append(Buffer, Length);
			});

			return Stream;
		}
		std::string SchemaToString(Core::Schema* Base)
		{
			switch (Base->Value.GetType())
			{
				case Core::VarType::Null:
				case Core::VarType::Undefined:
				case Core::VarType::Object:
				case Core::VarType::Array:
				case Core::VarType::Pointer:
					break;
				case Core::VarType::String:
				case Core::VarType::Base64:
					return Base->Value.GetBlob();
				case Core::VarType::Integer:
					return std::to_string(Base->Value.GetInteger());
				case Core::VarType::Number:
					return std::to_string(Base->Value.GetNumber());
				case Core::VarType::Decimal:
					return Base->Value.GetDecimal().ToString();
				case Core::VarType::Boolean:
					return Base->Value.GetBoolean() ? "1" : "0";
			}

			return "";
		}
		std::string SchemaToBase64(Core::Schema* Base)
		{
			return Base->Value.GetBlob();
		}
		int64_t SchemaToInteger(Core::Schema* Base)
		{
			return Base->Value.GetInteger();
		}
		double SchemaToNumber(Core::Schema* Base)
		{
			return Base->Value.GetNumber();
		}
		std::string SchemaToDecimal(Core::Schema* Base)
		{
			return Base->Value.GetDecimal().ToString();
		}
		bool SchemaToBoolean(Core::Schema* Base)
		{
			return Base->Value.GetBoolean();
		}
		Core::Schema* SchemaFromJSON(const std::string& Value)
		{
			return Core::Schema::ReadJSON(Value.c_str(), Value.size());
		}
		Core::Schema* SchemaFromXML(const std::string& Value)
		{
			return Core::Schema::ReadXML(Value.c_str());
		}
		Core::Schema* SchemaImport(const std::string& Value)
		{
			VMManager* Manager = VMManager::Get();
			if (!Manager)
				return nullptr;

			return Manager->ImportJSON(Value);
		}
		Core::Schema* SchemaCopyAssign(Core::Schema* Base, const Core::Variant& Other)
		{
			Base->Value = Other;
			return Base;
		}
		bool SchemaEquals(Core::Schema* Base, Core::Schema* Other)
		{
			if (Other != nullptr)
				return Base->Value == Other->Value;

			Core::VarType Type = Base->Value.GetType();
			return (Type == Core::VarType::Null || Type == Core::VarType::Undefined);
		}

		CEFormat::CEFormat()
		{
		}
		CEFormat::CEFormat(unsigned char* Buffer)
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

				if (TypeId & (int)VMTypeId::MASK_OBJECT)
				{
					VMTypeInfo Type = Global.GetTypeInfoById(TypeId);
					if (Type.IsValid() && Type.GetFlags() & (size_t)VMObjType::VALUE)
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
		std::string CEFormat::JSON(void* Ref, int TypeId)
		{
			VMContext* Context = VMContext::Get();
			if (!Context)
				return "{}";

			VMGlobal& Global = Context->GetManager()->Global();
			Core::Parser Result;

			FormatJSON(Global, Result, Ref, TypeId);
			return Result.R();
		}
		std::string CEFormat::Form(const std::string& F, const CEFormat& Form)
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
		void CEFormat::WriteLine(Core::Console* Base, const std::string& F, CEFormat* Form)
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
		void CEFormat::Write(Core::Console* Base, const std::string& F, CEFormat* Form)
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
		void CEFormat::FormatBuffer(VMGlobal& Global, Core::Parser& Result, std::string& Offset, void* Ref, int TypeId)
		{
			if (TypeId < (int)VMTypeId::BOOL || TypeId >(int)VMTypeId::DOUBLE)
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
					STDMap* Map = *(STDMap**)Ref;
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
					STDArray* Array = *(STDArray**)Ref;
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
					case (int)VMTypeId::BOOL:
						Result.fAppend("%s", *(bool*)Ref ? "true" : "false");
						break;
					case (int)VMTypeId::INT8:
						Result.fAppend("%i", *(char*)Ref);
						break;
					case (int)VMTypeId::INT16:
						Result.fAppend("%i", *(short*)Ref);
						break;
					case (int)VMTypeId::INT32:
						Result.fAppend("%i", *(int*)Ref);
						break;
					case (int)VMTypeId::INT64:
						Result.fAppend("%ll", *(int64_t*)Ref);
						break;
					case (int)VMTypeId::UINT8:
						Result.fAppend("%u", *(unsigned char*)Ref);
						break;
					case (int)VMTypeId::UINT16:
						Result.fAppend("%u", *(unsigned short*)Ref);
						break;
					case (int)VMTypeId::UINT32:
						Result.fAppend("%u", *(unsigned int*)Ref);
						break;
					case (int)VMTypeId::UINT64:
						Result.fAppend("%llu", *(uint64_t*)Ref);
						break;
					case (int)VMTypeId::FLOAT:
						Result.fAppend("%f", *(float*)Ref);
						break;
					case (int)VMTypeId::DOUBLE:
						Result.fAppend("%f", *(double*)Ref);
						break;
					default:
						Result.Append("null");
						break;
				}
			}
		}
		void CEFormat::FormatJSON(VMGlobal& Global, Core::Parser& Result, void* Ref, int TypeId)
		{
			if (TypeId < (int)VMTypeId::BOOL || TypeId >(int)VMTypeId::DOUBLE)
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
					STDMap* Map = (STDMap*)Object;
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
					STDArray* Array = (STDArray*)Object;
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
					case (int)VMTypeId::BOOL:
						Result.fAppend("%s", *(bool*)Ref ? "true" : "false");
						break;
					case (int)VMTypeId::INT8:
						Result.fAppend("%i", *(char*)Ref);
						break;
					case (int)VMTypeId::INT16:
						Result.fAppend("%i", *(short*)Ref);
						break;
					case (int)VMTypeId::INT32:
						Result.fAppend("%i", *(int*)Ref);
						break;
					case (int)VMTypeId::INT64:
						Result.fAppend("%ll", *(int64_t*)Ref);
						break;
					case (int)VMTypeId::UINT8:
						Result.fAppend("%u", *(unsigned char*)Ref);
						break;
					case (int)VMTypeId::UINT16:
						Result.fAppend("%u", *(unsigned short*)Ref);
						break;
					case (int)VMTypeId::UINT32:
						Result.fAppend("%u", *(unsigned int*)Ref);
						break;
					case (int)VMTypeId::UINT64:
						Result.fAppend("%llu", *(uint64_t*)Ref);
						break;
					case (int)VMTypeId::FLOAT:
						Result.fAppend("%f", *(float*)Ref);
						break;
					case (int)VMTypeId::DOUBLE:
						Result.fAppend("%f", *(double*)Ref);
						break;
				}
			}
		}

		bool CERegisterFormat(VMManager* Engine)
		{
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();
			Engine->BeginNamespace("CE");
			VMRefClass VFormat = Register.SetClassUnmanaged<CEFormat>("Format");
			VFormat.SetUnmanagedConstructor<CEFormat>("Format@ f()");
			VFormat.SetUnmanagedConstructorList<CEFormat>("Format@ f(int &in) {repeat ?}");
			VFormat.SetMethodStatic("String JSON(const ? &in)", &CEFormat::JSON);
			Engine->EndNamespace();

			return true;
		}
		bool CERegisterDecimal(VMManager* Engine)
		{
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();
			Engine->BeginNamespace("CE");
			VMTypeClass VDecimal = Register.SetStructUnmanaged<Core::Decimal>("Decimal");
			VDecimal.SetConstructor<Core::Decimal>("void f()");
			VDecimal.SetConstructor<Core::Decimal, int32_t>("void f(int)");
			VDecimal.SetConstructor<Core::Decimal, int32_t>("void f(uint)");
			VDecimal.SetConstructor<Core::Decimal, int64_t>("void f(int64)");
			VDecimal.SetConstructor<Core::Decimal, uint64_t>("void f(uint64)");
			VDecimal.SetConstructor<Core::Decimal, float>("void f(float)");
			VDecimal.SetConstructor<Core::Decimal, double>("void f(double)");
			VDecimal.SetConstructor<Core::Decimal, const std::string&>("void f(const String &in)");
			VDecimal.SetConstructor<Core::Decimal, const Core::Decimal&>("void f(const Decimal &in)");
			VDecimal.SetMethod("Decimal& Precise(int)", &Core::Decimal::Precise);
			VDecimal.SetMethod("Decimal& Trim()", &Core::Decimal::Trim);
			VDecimal.SetMethod("Decimal& Unlead()", &Core::Decimal::Unlead);
			VDecimal.SetMethod("Decimal& Untrail()", &Core::Decimal::Untrail);
			VDecimal.SetMethod("bool IsNaN()", &Core::Decimal::IsNaN);
			VDecimal.SetMethod("double ToDouble()", &Core::Decimal::ToDouble);
			VDecimal.SetMethod("int64 ToInt64()", &Core::Decimal::ToInt64);
			VDecimal.SetMethod("String ToString()", &Core::Decimal::ToString);
			VDecimal.SetMethod("String Exp()", &Core::Decimal::Exp);
			VDecimal.SetMethod("String Decimals()", &Core::Decimal::Decimals);
			VDecimal.SetMethod("String Ints()", &Core::Decimal::Ints);
			VDecimal.SetMethod("String Size()", &Core::Decimal::Size);
			VDecimal.SetOperatorEx(VMOpFunc::Neg, (uint32_t)VMOp::Const, "Decimal", "", &DecimalNegate);
			VDecimal.SetOperatorEx(VMOpFunc::MulAssign, (uint32_t)VMOp::Left, "Decimal&", "const Decimal &in", &DecimalMulEq);
			VDecimal.SetOperatorEx(VMOpFunc::DivAssign, (uint32_t)VMOp::Left, "Decimal&", "const Decimal &in", &DecimalDivEq);
			VDecimal.SetOperatorEx(VMOpFunc::AddAssign, (uint32_t)VMOp::Left, "Decimal&", "const Decimal &in", &DecimalAddEq);
			VDecimal.SetOperatorEx(VMOpFunc::SubAssign, (uint32_t)VMOp::Left, "Decimal&", "const Decimal &in", &DecimalSubEq);
			VDecimal.SetOperatorEx(VMOpFunc::PostInc, (uint32_t)VMOp::Left, "Decimal&", "", &DecimalPP);
			VDecimal.SetOperatorEx(VMOpFunc::PostDec, (uint32_t)VMOp::Left, "Decimal&", "", &DecimalMM);
			VDecimal.SetOperatorEx(VMOpFunc::Equals, (uint32_t)VMOp::Const, "bool", "const Decimal &in", &DecimalEq);
			VDecimal.SetOperatorEx(VMOpFunc::Cmp, (uint32_t)VMOp::Const, "int", "const Decimal &in", &DecimalCmp);
			VDecimal.SetOperatorEx(VMOpFunc::Add, (uint32_t)VMOp::Const, "Decimal", "const Decimal &in", &DecimalAdd);
			VDecimal.SetOperatorEx(VMOpFunc::Sub, (uint32_t)VMOp::Const, "Decimal", "const Decimal &in", &DecimalSub);
			VDecimal.SetOperatorEx(VMOpFunc::Mul, (uint32_t)VMOp::Const, "Decimal", "const Decimal &in", &DecimalMul);
			VDecimal.SetOperatorEx(VMOpFunc::Div, (uint32_t)VMOp::Const, "Decimal", "const Decimal &in", &DecimalDiv);
			VDecimal.SetOperatorEx(VMOpFunc::Mod, (uint32_t)VMOp::Const, "Decimal", "const Decimal &in", &DecimalPer);
			VDecimal.SetMethodStatic("CE::Decimal Size(const CE::Decimal &in, const CE::Decimal &in, int)", &Core::Decimal::Divide);
			VDecimal.SetMethodStatic("CE::Decimal NaN()", &Core::Decimal::NaN);
			Engine->EndNamespace();

			return true;
		}
		bool CERegisterVariant(VMManager* Engine)
		{
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();
			Engine->BeginNamespace("CE");
			VMEnum VVarType = Register.SetEnum("VarType");
			VVarType.SetValue("Null", (int)Core::VarType::Null);
			VVarType.SetValue("Undefined", (int)Core::VarType::Undefined);
			VVarType.SetValue("Object", (int)Core::VarType::Object);
			VVarType.SetValue("Array", (int)Core::VarType::Array);
			VVarType.SetValue("String", (int)Core::VarType::String);
			VVarType.SetValue("Base64", (int)Core::VarType::Base64);
			VVarType.SetValue("Integer", (int)Core::VarType::Integer);
			VVarType.SetValue("Number", (int)Core::VarType::Number);
			VVarType.SetValue("Decimal", (int)Core::VarType::Decimal);
			VVarType.SetValue("Boolean", (int)Core::VarType::Boolean);

			VMTypeClass VVariant = Register.SetStructUnmanaged<Core::Variant>("Variant");
			VVariant.SetConstructor<Core::Variant>("void f()");
			VVariant.SetConstructor<Core::Variant, const Core::Variant&>("void f(const Variant &in)");
			VVariant.SetMethod("bool Deserialize(const String &in, bool = false)", &Core::Variant::Deserialize);
			VVariant.SetMethod("String Serialize() const", &Core::Variant::Serialize);
			VVariant.SetMethod("String Dec() const", &Core::Variant::GetDecimal);
			VVariant.SetMethod("String Str() const", &Core::Variant::GetBlob);
			VVariant.SetMethod("Address@ Ptr() const", &Core::Variant::GetPointer);
			VVariant.SetMethod("int64 Int() const", &Core::Variant::GetInteger);
			VVariant.SetMethod("double Num() const", &Core::Variant::GetNumber);
			VVariant.SetMethod("bool Bool() const", &Core::Variant::GetBoolean);
			VVariant.SetMethod("VarType GetType() const", &Core::Variant::GetType);
			VVariant.SetMethod("bool IsObject() const", &Core::Variant::IsObject);
			VVariant.SetMethod("bool IsEmpty() const", &Core::Variant::IsEmpty);
			VVariant.SetMethodEx("uint64 GetSize() const", &VariantGetSize);
			VVariant.SetOperatorEx(VMOpFunc::Equals, (uint32_t)VMOp::Const, "bool", "const Variant &in", &VariantEquals);
			VVariant.SetOperatorEx(VMOpFunc::ImplCast, (uint32_t)VMOp::Const, "bool", "", &VariantImplCast);
			Engine->EndNamespace();

			Engine->BeginNamespace("CE::Var");
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
			Register.SetFunction<Core::Variant(const std::string&)>("Variant Decimal(const String &in)", &Core::Var::DecimalString);
			Engine->EndNamespace();

			return true;
		}
		bool CERegisterFileState(VMManager* Engine)
		{
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();
			Engine->BeginNamespace("CE");
			VMTypeClass VFileState = Register.SetPod<Core::FileState>("FileState");
			VFileState.SetProperty<Core::FileState>("uint64 Size", &Core::FileState::Size);
			VFileState.SetProperty<Core::FileState>("uint64 Links", &Core::FileState::Links);
			VFileState.SetProperty<Core::FileState>("uint64 Permissions", &Core::FileState::Permissions);
			VFileState.SetProperty<Core::FileState>("uint64 IDocument", &Core::FileState::IDocument);
			VFileState.SetProperty<Core::FileState>("uint64 Device", &Core::FileState::Device);
			VFileState.SetProperty<Core::FileState>("uint64 UserId", &Core::FileState::UserId);
			VFileState.SetProperty<Core::FileState>("uint64 GroupId", &Core::FileState::GroupId);
			VFileState.SetProperty<Core::FileState>("int64 LastAccess", &Core::FileState::LastAccess);
			VFileState.SetProperty<Core::FileState>("int64 LastPermissionChange", &Core::FileState::LastPermissionChange);
			VFileState.SetProperty<Core::FileState>("int64 LastModified", &Core::FileState::LastModified);
			VFileState.SetProperty<Core::FileState>("bool Exists", &Core::FileState::Exists);
			Engine->EndNamespace();

			return true;
		}
		bool CERegisterResource(VMManager* Engine)
		{
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();
			Engine->BeginNamespace("CE");
			VMTypeClass VResource = Register.SetPod<Core::Resource>("Resource");
			VResource.SetProperty<Core::Resource>("uint64 Key", &Core::Resource::Size);
			VResource.SetProperty<Core::Resource>("int64 LastModified", &Core::Resource::LastModified);
			VResource.SetProperty<Core::Resource>("int64 CreationTime", &Core::Resource::CreationTime);
			VResource.SetProperty<Core::Resource>("bool IsReferenced", &Core::Resource::IsReferenced);
			VResource.SetProperty<Core::Resource>("bool IsDirectory", &Core::Resource::IsDirectory);
			Engine->EndNamespace();

			return true;
		}
		bool CERegisterDateTime(VMManager* Engine)
		{
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();
			Engine->BeginNamespace("CE");
			VMTypeClass VDateTime = Register.SetStructUnmanaged<Core::DateTime>("DateTime");
			VDateTime.SetConstructor<Core::DateTime>("void f()");
			VDateTime.SetConstructor<Core::DateTime, uint64_t>("void f(uint64)");
			VDateTime.SetConstructor<Core::DateTime, const Core::DateTime&>("void f(const DateTime &in)");
			VDateTime.SetMethod("String Format(const String &in)", &Core::DateTime::Format);
			VDateTime.SetMethod("String Date(const String &in)", &Core::DateTime::Date);
			VDateTime.SetMethod("String Iso8601()", &Core::DateTime::Iso8601);
			VDateTime.SetMethod("DateTime Now()", &Core::DateTime::Now);
			VDateTime.SetMethod("DateTime FromNanoseconds(uint64)", &Core::DateTime::FromNanoseconds);
			VDateTime.SetMethod("DateTime FromMicroseconds(uint64)", &Core::DateTime::FromMicroseconds);
			VDateTime.SetMethod("DateTime FromMilliseconds(uint64)", &Core::DateTime::FromMilliseconds);
			VDateTime.SetMethod("DateTime FromSeconds(uint64)", &Core::DateTime::FromSeconds);
			VDateTime.SetMethod("DateTime FromMinutes(uint64)", &Core::DateTime::FromMinutes);
			VDateTime.SetMethod("DateTime FromHours(uint64)", &Core::DateTime::FromHours);
			VDateTime.SetMethod("DateTime FromDays(uint64)", &Core::DateTime::FromDays);
			VDateTime.SetMethod("DateTime FromWeeks(uint64)", &Core::DateTime::FromWeeks);
			VDateTime.SetMethod("DateTime FromMonths(uint64)", &Core::DateTime::FromMonths);
			VDateTime.SetMethod("DateTime FromYears(uint64)", &Core::DateTime::FromYears);
			VDateTime.SetMethod("DateTime& SetDateSeconds(uint64, bool = true)", &Core::DateTime::SetDateSeconds);
			VDateTime.SetMethod("DateTime& SetDateMinutes(uint64, bool = true)", &Core::DateTime::SetDateMinutes);
			VDateTime.SetMethod("DateTime& SetDateHours(uint64, bool = true)", &Core::DateTime::SetDateHours);
			VDateTime.SetMethod("DateTime& SetDateDay(uint64, bool = true)", &Core::DateTime::SetDateDay);
			VDateTime.SetMethod("DateTime& SetDateWeek(uint64, bool = true)", &Core::DateTime::SetDateWeek);
			VDateTime.SetMethod("DateTime& SetDateMonth(uint64, bool = true)", &Core::DateTime::SetDateMonth);
			VDateTime.SetMethod("DateTime& SetDateYear(uint64, bool = true)", &Core::DateTime::SetDateYear);
			VDateTime.SetMethod("uint64 DateSecond()", &Core::DateTime::DateSecond);
			VDateTime.SetMethod("uint64 DateMinute()", &Core::DateTime::DateMinute);
			VDateTime.SetMethod("uint64 DateHour()", &Core::DateTime::DateHour);
			VDateTime.SetMethod("uint64 DateDay()", &Core::DateTime::DateDay);
			VDateTime.SetMethod("uint64 DateWeek()", &Core::DateTime::DateWeek);
			VDateTime.SetMethod("uint64 DateMonth()", &Core::DateTime::DateMonth);
			VDateTime.SetMethod("uint64 DateYear()", &Core::DateTime::DateYear);
			VDateTime.SetMethod("uint64 Nanoseconds()", &Core::DateTime::Nanoseconds);
			VDateTime.SetMethod("uint64 Microseconds()", &Core::DateTime::Microseconds);
			VDateTime.SetMethod("uint64 Milliseconds()", &Core::DateTime::Milliseconds);
			VDateTime.SetMethod("uint64 Seconds()", &Core::DateTime::Seconds);
			VDateTime.SetMethod("uint64 Minutes()", &Core::DateTime::Minutes);
			VDateTime.SetMethod("uint64 Hours()", &Core::DateTime::Hours);
			VDateTime.SetMethod("uint64 Days()", &Core::DateTime::Days);
			VDateTime.SetMethod("uint64 Weeks()", &Core::DateTime::Weeks);
			VDateTime.SetMethod("uint64 Months()", &Core::DateTime::Months);
			VDateTime.SetMethod("uint64 Years()", &Core::DateTime::Years);
			VDateTime.SetOperatorEx(VMOpFunc::AddAssign, (uint32_t)VMOp::Left, "DateTime&", "const DateTime &in", &DateTimeAddEq);
			VDateTime.SetOperatorEx(VMOpFunc::SubAssign, (uint32_t)VMOp::Left, "DateTime&", "const DateTime &in", &DateTimeSubEq);
			VDateTime.SetOperatorEx(VMOpFunc::Equals, (uint32_t)VMOp::Const, "bool", "const DateTime &in", &DateTimeEq);
			VDateTime.SetOperatorEx(VMOpFunc::Cmp, (uint32_t)VMOp::Const, "int", "const DateTime &in", &DateTimeCmp);
			VDateTime.SetOperatorEx(VMOpFunc::Add, (uint32_t)VMOp::Const, "DateTime", "const DateTime &in", &DateTimeAdd);
			VDateTime.SetOperatorEx(VMOpFunc::Sub, (uint32_t)VMOp::Const, "DateTime", "const DateTime &in", &DateTimeSub);
			Engine->EndNamespace();

			return true;
		}
		bool CERegisterTicker(VMManager* Engine)
		{
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();
			Engine->BeginNamespace("CE");
			VMTypeClass VTicker = Register.SetStructUnmanaged<Core::Ticker>("Ticker");
			VTicker.SetConstructor<Core::Ticker>("void f()");
			VTicker.SetMethod("bool TickEvent(double)", &Core::Ticker::TickEvent);
			VTicker.SetMethod("double GetTime()", &Core::Ticker::GetTime);
			Engine->EndNamespace();

			return true;
		}
		bool CERegisterOS(VMManager* Engine)
		{
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();
			Engine->BeginNamespace("CE::OS::Directory");
			Register.SetFunction("void Set(Address@)", &Core::OS::Directory::Set);
			Register.SetFunction("void Patch(const String &in)", &Core::OS::Directory::Patch);
			Register.SetFunction("bool Create(Address@)", &Core::OS::Directory::Create);
			Register.SetFunction("bool Remove(Address@)", &Core::OS::Directory::Remove);
			Register.SetFunction("bool IsExists(Address@)", &Core::OS::Directory::IsExists);
			Register.SetFunction("String Get()", &Core::OS::Directory::Get);
			Register.SetFunction("Array<String>@ GetMounts()", &OSDirectoryGetMounts);
			Register.SetFunction("Array<String>@ Scan(const String &in)", &OSDirectoryScan);
			Engine->EndNamespace();
			Engine->BeginNamespace("CE::OS::File");
			Register.SetFunction<bool(const char*, const std::string&)>("bool Write(Address@, const String &in)", &Core::OS::File::Write);
			Register.SetFunction("bool State(const String &in, Resource &out)", &Core::OS::File::State);
			Register.SetFunction("bool Move(Address@, Address@)", &Core::OS::File::Move);
			Register.SetFunction("bool Remove(Address@)", &Core::OS::File::Remove);
			Register.SetFunction("bool IsExists(Address@)", &Core::OS::File::IsExists);
			Register.SetFunction("int Compare(const String &in, const String &in)", &Core::OS::File::Compare);
			Register.SetFunction("uint64 GetCheckSum(const String &in)", &Core::OS::File::GetCheckSum);
			Register.SetFunction("FileState GetState(Address@)", &Core::OS::File::GetState);
			Register.SetFunction("String ReadAsString(Address@)", &Core::OS::File::ReadAsString);
			Engine->EndNamespace();
			Engine->BeginNamespace("CE::OS::Path");
			Register.SetFunction<std::string(const char*)>("String Resolve(Address@)", &Core::OS::Path::Resolve);
			Register.SetFunction<std::string(const std::string&, const std::string&)>("String Resolve(const String &in, const String &in)", &Core::OS::Path::Resolve);
			Register.SetFunction<std::string(const char*)>("String ResolveDirectory(Address@)", &Core::OS::Path::ResolveDirectory);
			Register.SetFunction<std::string(const std::string&, const std::string&)>("String ResolveDirectory(const String &in, const String &in)", &Core::OS::Path::ResolveDirectory);
			Register.SetFunction<std::string(const std::string&)>("String ResolveResource(Address@)", &Core::OS::Path::ResolveResource);
			Register.SetFunction<std::string(const std::string&, const std::string&)>("String ResolveResource(const String &in, const String &in)", &Core::OS::Path::ResolveResource);
			Register.SetFunction("String GetDirectory(Address@, uint = 0)", &Core::OS::Path::GetDirectory);
			Engine->EndNamespace();
			Engine->BeginNamespace("CE::OS::Process");
			Register.SetFunction("int Execute(const String &in)", &OSProcessExecute);
			Engine->EndNamespace();
			Engine->BeginNamespace("CE::OS::Symbol");
			Register.SetFunction("Address@ Load(const String &in = \"\")", &Core::OS::Symbol::Load);
			Register.SetFunction("Address@ LoadFunction(Address@, const String &in)", &Core::OS::Symbol::LoadFunction);
			Register.SetFunction("bool Unload(Address@)", &Core::OS::Symbol::Unload);
			Engine->EndNamespace();
			Engine->BeginNamespace("CE::OS::Input");
			Register.SetFunction("bool Text(const String &in, const String &in, const String &in, String &out)", &Core::OS::Input::Text);
			Register.SetFunction("bool Password(const String &in, const String &in, String &out)", &Core::OS::Input::Password);
			Register.SetFunction("bool Save(const String &in, const String &in, const String &in, const String &in, String &out)", &Core::OS::Input::Save);
			Register.SetFunction("bool Open(const String &in, const String &in, const String &in, const String &in, bool, String &out)", &Core::OS::Input::Open);
			Register.SetFunction("bool Folder(const String &in, const String &in, String &out)", &Core::OS::Input::Folder);
			Register.SetFunction("bool Color(const String &in, const String &in, String &out)", &Core::OS::Input::Color);
			Engine->EndNamespace();
			Engine->BeginNamespace("CE::OS::Error");
			Register.SetFunction("int Get()", &Core::OS::Error::Get);
			Register.SetFunction("String GetName(int)", &Core::OS::Error::GetName);
			Engine->EndNamespace();

			return true;
		}
		bool CERegisterConsole(VMManager* Engine)
		{
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();
			Engine->BeginNamespace("CE");
			VMEnum VStdColor = Register.SetEnum("StdColor");
			VStdColor.SetValue("Black", (int)Core::StdColor::Black);
			VStdColor.SetValue("DarkBlue", (int)Core::StdColor::DarkBlue);
			VStdColor.SetValue("DarkGreen", (int)Core::StdColor::DarkGreen);
			VStdColor.SetValue("LightBlue", (int)Core::StdColor::LightBlue);
			VStdColor.SetValue("DarkRed", (int)Core::StdColor::DarkRed);
			VStdColor.SetValue("Magenta", (int)Core::StdColor::Magenta);
			VStdColor.SetValue("Orange", (int)Core::StdColor::Orange);
			VStdColor.SetValue("LightGray", (int)Core::StdColor::LightGray);
			VStdColor.SetValue("Gray", (int)Core::StdColor::Gray);
			VStdColor.SetValue("Blue", (int)Core::StdColor::Blue);
			VStdColor.SetValue("Green", (int)Core::StdColor::Green);
			VStdColor.SetValue("Cyan", (int)Core::StdColor::Cyan);
			VStdColor.SetValue("Red", (int)Core::StdColor::Red);
			VStdColor.SetValue("Pink", (int)Core::StdColor::Pink);
			VStdColor.SetValue("Yellow", (int)Core::StdColor::Yellow);
			VStdColor.SetValue("White", (int)Core::StdColor::White);

			VMRefClass VConsole = Register.SetClassUnmanaged<Core::Console>("Console");
			VConsole.SetMethod("void Hide()", &Core::Console::Hide);
			VConsole.SetMethod("void Show()", &Core::Console::Show);
			VConsole.SetMethod("void Clear()", &Core::Console::Clear);
			VConsole.SetMethod("void Flush()", &Core::Console::Flush);
			VConsole.SetMethod("void FlushWrite()", &Core::Console::FlushWrite);
			VConsole.SetMethod("void SetColoring(bool)", &Core::Console::SetColoring);
			VConsole.SetMethod("void ColorBegin(StdColor, StdColor)", &Core::Console::ColorBegin);
			VConsole.SetMethod("void ColorEnd()", &Core::Console::ColorEnd);
			VConsole.SetMethod("void CaptureTime()", &Core::Console::CaptureTime);
			VConsole.SetMethod("void WriteLine(const String &in)", &Core::Console::sWriteLine);
			VConsole.SetMethod("void Write(const String &in)", &Core::Console::sWrite);
			VConsole.SetMethod("double GetCapturedTime()", &Core::Console::GetCapturedTime);
			VConsole.SetMethod("String Read(uint64)", &Core::Console::Read);
			VConsole.SetMethodStatic("Console@+ Get()", &Core::Console::Get);
			VConsole.SetMethodEx("void Trace(uint32 = 32)", &ConsoleTrace);
			VConsole.SetMethodEx("void WriteLine(const String &in, Format@+)", &CEFormat::WriteLine);
			VConsole.SetMethodEx("void Write(const String &in, Format@+)", &CEFormat::Write);
			Engine->EndNamespace();

			return true;
		}
		bool CERegisterTimer(VMManager* Engine)
		{
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();
			Engine->BeginNamespace("CE");
			VMRefClass VTimer = Register.SetClassUnmanaged<Core::Timer>("Timer");
			VTimer.SetUnmanagedConstructor<Core::Timer>("Timer@ f()");
			VTimer.SetMethod("void SetStepLimitation(double, double)", &Core::Timer::SetStepLimitation);
			VTimer.SetMethod("void Synchronize()", &Core::Timer::Synchronize);
			VTimer.SetMethod("void CaptureTime()", &Core::Timer::CaptureTime);
			VTimer.SetMethod("void Sleep(uint64)", &Core::Timer::Sleep);
			VTimer.SetMethod("double GetTimeIncrement() const", &Core::Timer::GetTimeIncrement);
			VTimer.SetMethod("double GetTickCounter() const", &Core::Timer::GetTickCounter);
			VTimer.SetMethod("double GetFrameCount() const", &Core::Timer::GetFrameCount);
			VTimer.SetMethod("double GetElapsedTime() const", &Core::Timer::GetElapsedTime);
			VTimer.SetMethod("double GetCapturedTime() const", &Core::Timer::GetCapturedTime);
			VTimer.SetMethod("double GetDeltaTime() const", &Core::Timer::GetDeltaTime);
			VTimer.SetMethod("double GetTimeStep() const", &Core::Timer::GetTimeStep);
			Engine->EndNamespace();

			return true;
		}
		bool CERegisterFileStream(VMManager* Engine)
		{
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();
			Engine->BeginNamespace("CE");
			VMEnum VFileMode = Register.SetEnum("FileMode");
			VFileMode.SetValue("ReadOnly", (int)Core::FileMode::Read_Only);
			VFileMode.SetValue("WriteOnly", (int)Core::FileMode::Write_Only);
			VFileMode.SetValue("AppendOnly", (int)Core::FileMode::Append_Only);
			VFileMode.SetValue("ReadWrite", (int)Core::FileMode::Read_Write);
			VFileMode.SetValue("WriteRead", (int)Core::FileMode::Write_Read);
			VFileMode.SetValue("ReadAppendWrite", (int)Core::FileMode::Read_Append_Write);
			VFileMode.SetValue("BinaryReadOnly", (int)Core::FileMode::Binary_Read_Only);
			VFileMode.SetValue("BinaryWriteOnly", (int)Core::FileMode::Binary_Write_Only);
			VFileMode.SetValue("BinaryAppendOnly", (int)Core::FileMode::Binary_Append_Only);
			VFileMode.SetValue("BinaryRead_Write", (int)Core::FileMode::Binary_Read_Write);
			VFileMode.SetValue("BinaryWriteRead", (int)Core::FileMode::Binary_Write_Read);
			VFileMode.SetValue("BinaryReadAppendWrite", (int)Core::FileMode::Binary_Read_Append_Write);

			VMEnum VFileSeek = Register.SetEnum("FileSeek");
			VFileSeek.SetValue("Begin", (int)Core::FileSeek::Begin);
			VFileSeek.SetValue("Current", (int)Core::FileSeek::Current);
			VFileSeek.SetValue("End", (int)Core::FileSeek::End);

			VMRefClass VFileStream = Register.SetClassUnmanaged<Core::FileStream>("FileStream");
			VFileStream.SetUnmanagedConstructor<Core::FileStream>("FileStream@ f()");
			VFileStream.SetMethod("void Clear()", &Core::FileStream::Clear);
			VFileStream.SetMethod("bool Open(Address@, FileMode)", &Core::FileStream::Open);
			VFileStream.SetMethod("bool Close()", &Core::FileStream::Close);
			VFileStream.SetMethod("bool Seek(FileSeek, int64)", &Core::FileStream::Seek);
			VFileStream.SetMethod("bool Move(int64)", &Core::FileStream::Move);
			VFileStream.SetMethodEx("uint64 Read(uint64, String &out)", &FileStreamRead);
			VFileStream.SetMethod("uint64 Write(Address@, uint64)", &Core::FileStream::Write);
			VFileStream.SetMethod("uint64 Tell()", &Core::FileStream::Tell);
			VFileStream.SetMethod("int Flush()", &Core::FileStream::Flush);
			VFileStream.SetMethod("int GetFd()", &Core::FileStream::GetFd);
			VFileStream.SetMethod("uint64 GetSize()", &Core::FileStream::GetSize);
			VFileStream.SetMethod("String GetSource()", &Core::FileStream::GetSource);
			Engine->EndNamespace();

			return true;
		}
		bool CERegisterGzStream(VMManager* Engine)
		{
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();
			Engine->BeginNamespace("CE");
			VMRefClass VGzStream = Register.SetClassUnmanaged<Core::GzStream>("GzStream");
			VGzStream.SetUnmanagedConstructor<Core::GzStream>("GzStream@ f()");
			VGzStream.SetMethod("void Clear()", &Core::GzStream::Clear);
			VGzStream.SetMethod("bool Open(Address@, FileMode)", &Core::GzStream::Open);
			VGzStream.SetMethod("bool Close()", &Core::GzStream::Close);
			VGzStream.SetMethod("bool Seek(FileSeek, int64)", &Core::GzStream::Seek);
			VGzStream.SetMethod("bool Move(int64)", &Core::GzStream::Move);
			VGzStream.SetMethodEx("uint64 Read(uint64, String &out)", &GzStreamRead);
			VGzStream.SetMethod("uint64 Write(Address@, uint64)", &Core::GzStream::Write);
			VGzStream.SetMethod("uint64 Tell()", &Core::GzStream::Tell);
			VGzStream.SetMethod("int Flush()", &Core::GzStream::Flush);
			VGzStream.SetMethod("int GetFd()", &Core::GzStream::GetFd);
			VGzStream.SetMethod("uint64 GetSize()", &Core::GzStream::GetSize);
			VGzStream.SetMethod("String GetSource()", &Core::GzStream::GetSource);
			Engine->EndNamespace();

			return true;
		}
		bool CERegisterWebStream(VMManager* Engine)
		{
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();
			Engine->BeginNamespace("CE");
			VMRefClass VWebStream = Register.SetClassUnmanaged<Core::WebStream>("WebStream");
			VWebStream.SetUnmanagedConstructor<Core::WebStream>("WebStream@ f()");
			VWebStream.SetMethod("void Clear()", &Core::WebStream::Clear);
			VWebStream.SetMethod("bool Open(Address@, FileMode)", &Core::WebStream::Open);
			VWebStream.SetMethod("bool Close()", &Core::WebStream::Close);
			VWebStream.SetMethod("bool Seek(FileSeek, int64)", &Core::WebStream::Seek);
			VWebStream.SetMethod("bool Move(int64)", &Core::WebStream::Move);
			VWebStream.SetMethodEx("uint64 Read(uint64, String &out)", &WebStreamRead);
			VWebStream.SetMethod("uint64 Write(Address@, uint64)", &Core::WebStream::Write);
			VWebStream.SetMethod("uint64 Tell()", &Core::WebStream::Tell);
			VWebStream.SetMethod("int Flush()", &Core::WebStream::Flush);
			VWebStream.SetMethod("int GetFd()", &Core::WebStream::GetFd);
			VWebStream.SetMethod("uint64 GetSize()", &Core::WebStream::GetSize);
			VWebStream.SetMethod("String GetSource()", &Core::WebStream::GetSource);
			Engine->EndNamespace();

			return true;
		}
		bool CERegisterSchedule(VMManager* Engine)
		{
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			VMGlobal& Register = Engine->Global();
			Engine->BeginNamespace("CE");
			VMRefClass VSchedule = Register.SetClassUnmanaged<Core::Schedule>("Schedule");
			VSchedule.SetFunctionDef("void CE::Schedule::TaskCallback()");
			VSchedule.SetMethodEx("uint64 SetTimeout(uint64, TaskCallback@)", &ScheduleSetTimeout);
			VSchedule.SetMethodEx("bool SetTask(TaskCallback@)", &ScheduleSetTask);
			VSchedule.SetMethod("bool ClearTimeout(uint64)", &Core::Schedule::ClearTimeout);
			VSchedule.SetMethod("bool Start(bool, uint64, uint64 = 16, uint64 = 524288)", &Core::Schedule::ClearTimeout);
			VSchedule.SetMethod("bool Stop()", &Core::Schedule::Stop);
			VSchedule.SetMethod("bool Dispatch()", &Core::Schedule::Dispatch);
			VSchedule.SetMethod("bool IsActive()", &Core::Schedule::IsActive);
			VSchedule.SetMethod("bool HasTasks()", &Core::Schedule::HasTasks);
			VSchedule.SetMethod("bool HasTimers()", &Core::Schedule::HasTimers);
			VSchedule.SetMethod("uint64 GetThreads()", &Core::Schedule::GetThreads);
			Engine->EndNamespace();

			return true;
		}
		bool CERegisterSchema(VMManager* Engine)
		{
			TH_ASSERT(Engine != nullptr, false, "manager should be set");
			Engine->BeginNamespace("CE");
			VMRefClass VSchema = Engine->Global().SetClassUnmanaged<Core::Schema>("Schema");
			VSchema.SetProperty<Core::Schema>("String Key", &Core::Schema::Key);
			VSchema.SetProperty<Core::Schema>("Variant Value", &Core::Schema::Value);
			VSchema.SetUnmanagedConstructor<Core::Schema, const Core::Variant&>("Schema@ f(const Variant &in)");
			VSchema.SetUnmanagedConstructorListEx<Core::Schema>("Schema@ f(int &in) {repeat {String, ?}}", &SchemaConstruct);
			VSchema.SetMethod<Core::Schema, Core::Variant, size_t>("Variant GetVar(uint) const", &Core::Schema::GetVar);
			VSchema.SetMethod<Core::Schema, Core::Variant, const std::string&>("Variant GetVar(const String &in) const", &Core::Schema::GetVar);
			VSchema.SetMethod("Schema@+ GetParent() const", &Core::Schema::GetParent);
			VSchema.SetMethod("Schema@+ GetAttribute(const String &in) const", &Core::Schema::GetAttribute);
			VSchema.SetMethod<Core::Schema, Core::Schema*, size_t>("Schema@+ Get(uint) const", &Core::Schema::Get);
			VSchema.SetMethod<Core::Schema, Core::Schema*, const std::string&, bool>("Schema@+ Get(const String &in, bool = false) const", &Core::Schema::Fetch);
			VSchema.SetMethod<Core::Schema, Core::Schema*, const std::string&>("Schema@+ Set(const String &in)", &Core::Schema::Set);
			VSchema.SetMethod<Core::Schema, Core::Schema*, const std::string&, const Core::Variant&>("Schema@+ Set(const String &in, const Variant &in)", &Core::Schema::Set);
			VSchema.SetMethod<Core::Schema, Core::Schema*, const std::string&, const Core::Variant&>("Schema@+ SetAttribute(const String& in, const Variant &in)", &Core::Schema::SetAttribute);
			VSchema.SetMethod<Core::Schema, Core::Schema*, const Core::Variant&>("Schema@+ Push(const Variant &in)", &Core::Schema::Push);
			VSchema.SetMethod<Core::Schema, Core::Schema*, size_t>("Schema@+ Pop(uint)", &Core::Schema::Pop);
			VSchema.SetMethod<Core::Schema, Core::Schema*, const std::string&>("Schema@+ Pop(const String &in)", &Core::Schema::Pop);
			VSchema.SetMethod("Schema@ Copy() const", &Core::Schema::Copy);
			VSchema.SetMethod("bool Has(const String &in) const", &Core::Schema::Has);
			VSchema.SetMethod("bool Has64(const String &in, uint = 12) const", &Core::Schema::Has64);
			VSchema.SetMethod("bool IsEmpty() const", &Core::Schema::IsEmpty);
			VSchema.SetMethod("bool IsAttribute() const", &Core::Schema::IsAttribute);
			VSchema.SetMethod("bool IsSaved() const", &Core::Schema::IsAttribute);
			VSchema.SetMethod("String GetName() const", &Core::Schema::GetName);
			VSchema.SetMethod("void Join(Schema@+)", &Core::Schema::Join);
			VSchema.SetMethod("void Clear()", &Core::Schema::Clear);
			VSchema.SetMethod("void Save()", &Core::Schema::Save);
			VSchema.SetMethodEx("Schema@+ Set(const String &in, Schema@+)", &SchemaSet);
			VSchema.SetMethodEx("Schema@+ Push(Schema@+)", &SchemaPush);
			VSchema.SetMethodEx("Array<Schema@>@ GetCollection(const String &in, bool = false) const", &SchemaGetCollection);
			VSchema.SetMethodEx("Array<Schema@>@ GetAttributes() const", &SchemaGetAttributes);
			VSchema.SetMethodEx("Array<Schema@>@ GetChilds() const", &SchemaGetChilds);
			VSchema.SetMethodEx("Map@ GetNames() const", &SchemaGetNames);
			VSchema.SetMethodEx("uint64 Size() const", &SchemaGetSize);
			VSchema.SetMethodEx("String JSON() const", &SchemaToJSON);
			VSchema.SetMethodEx("String XML() const", &SchemaToXML);
			VSchema.SetMethodEx("String Str() const", &SchemaToString);
			VSchema.SetMethodEx("String B64() const", &SchemaToBase64);
			VSchema.SetMethodEx("int64 Int() const", &SchemaToInteger);
			VSchema.SetMethodEx("double Num() const", &SchemaToNumber);
			VSchema.SetMethodEx("String Dec() const", &SchemaToDecimal);
			VSchema.SetMethodEx("bool Bool() const", &SchemaToBoolean);
			VSchema.SetMethodStatic("CE::Schema@ FromJSON(const String &in)", &SchemaFromJSON);
			VSchema.SetMethodStatic("CE::Schema@ FromXML(const String &in)", &SchemaFromXML);
			VSchema.SetMethodStatic("CE::Schema@ Import(const String &in)", &SchemaImport);
			VSchema.SetOperatorEx(VMOpFunc::Assign, (uint32_t)VMOp::Left, "Schema@+", "const Variant &in", &SchemaCopyAssign);
			VSchema.SetOperatorEx(VMOpFunc::Equals, (uint32_t)(VMOp::Left | VMOp::Const), "bool", "Schema@+", &SchemaEquals);
			VSchema.SetOperatorEx(VMOpFunc::Index, (uint32_t)VMOp::Left, "Schema@+", "const String &in", &SchemaGetIndex);
			VSchema.SetOperatorEx(VMOpFunc::Index, (uint32_t)VMOp::Left, "Schema@+", "uint64", &SchemaGetIndexOffset);
			Engine->EndNamespace();

			VMGlobal& Register = Engine->Global();
			Engine->BeginNamespace("CE::Var::Set");
			Register.SetFunction("CE::Schema@ Auto(const String &in, bool = false)", &Core::Var::Auto);
			Register.SetFunction("CE::Schema@ Null()", &Core::Var::Set::Null);
			Register.SetFunction("CE::Schema@ Undefined()", &Core::Var::Set::Undefined);
			Register.SetFunction("CE::Schema@ Object()", &Core::Var::Set::Object);
			Register.SetFunction("CE::Schema@ Array()", &Core::Var::Set::Array);
			Register.SetFunction("CE::Schema@ Pointer(Address@)", &Core::Var::Set::Pointer);
			Register.SetFunction("CE::Schema@ Integer(int64)", &Core::Var::Set::Integer);
			Register.SetFunction("CE::Schema@ Number(double)", &Core::Var::Set::Number);
			Register.SetFunction("CE::Schema@ Boolean(bool)", &Core::Var::Set::Boolean);
			Register.SetFunction<Core::Schema* (const std::string&)>("CE::Schema@ String(const String &in)", &Core::Var::Set::String);
			Register.SetFunction<Core::Schema* (const std::string&)>("CE::Schema@ Base64(const String &in)", &Core::Var::Set::Base64);
			Register.SetFunction<Core::Schema* (const std::string&)>("CE::Schema@ Decimal(const String &in)", &Core::Var::Set::DecimalString);
			Engine->EndNamespace();
			Engine->BeginNamespace("CE::Var");
			Register.SetFunction("CE::Schema@+ Init(Schema@+)", &SchemaInit);
			Engine->EndNamespace();

			return true;
		}
	}
}
