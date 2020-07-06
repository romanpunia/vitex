#include "interface.h"
#include "../network/bson.h"

using namespace Tomahawk::Rest;
using namespace Tomahawk::Compute;
using namespace Tomahawk::Network;

namespace Tomahawk
{
	namespace Script
	{
		namespace Interface
		{
			class Format : public Object
			{
			public:
				std::vector<std::string> Args;

			public:
				Format()
				{
				}
				Format(unsigned char* Buffer)
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
				
			public:
				static std::string JSON(void* Ref, int TypeId)
				{
					VMContext* Context = VMContext::Get();
					if (!Context)
						return "{}";

					VMGlobal& Global = Context->GetManager()->Global();
					Rest::Stroke Result;

					FormatJSON(Global, Result, Ref, TypeId);
					return Result.R();
				}

			private:
				static void FormatBuffer(VMGlobal& Global, Rest::Stroke& Result, std::string& Offset, void* Ref, int TypeId)
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
				static void FormatJSON(VMGlobal& Global, Rest::Stroke& Result, void* Ref, int TypeId)
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
			};

			static std::string Stroke_Form(const std::string& F, const Format& Form)
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

			static bool FileStream_Open(FileStream* Base, const std::string& Path, FileMode Mode)
			{
				return Base->Open(Path.c_str(), Mode);
			}
			static bool FileStream_OpenZ(FileStream* Base, const std::string& Path, FileMode Mode)
			{
				return Base->OpenZ(Path.c_str(), Mode);
			}
			static uint64_t FileStream_Write(FileStream* Base, const std::string& Buffer)
			{
				return Base->Write(Buffer.c_str(), (uint64_t)Buffer.size());
			}
			static std::string FileStream_Read(FileStream* Base, uint64_t Length)
			{
				char* Buffer = (char*)malloc(sizeof(char) * Length);
				if (!Base->Read(Buffer, Length))
				{
					free(Buffer);
					return "";
				}

				std::string Result(Buffer, Length);
				free(Buffer);

				return Result;
			}

			static void FileLogger_Process(FileLogger* Base, VMCAny* Any, VMCFunction* Callback)
			{
				if (!Callback)
				{
					VMWAny(Any).Release();
					return;
				}

				Base->Process([Any, Callback](FileLogger* Base, const char* Buffer, int64_t Size)
				{
					VMContext* Context = VMContext::Get();
					if (!Context)
						return false;

					std::string Data(Buffer, Size);
					Context->PushState();
					Context->Prepare(Callback);
					Context->SetArgObject(0, Base);
					Context->SetArgObject(1, &Data);
					Context->SetArgObject(2, Any);
					Context->Execute();

					bool Result = Context->GetReturnByte();
					Context->PopState();

					return Result;
				});

				VMWAny(Any).Release();
				VMWFunction(Callback).Release();
			}

			static VMCArray* FileTree_GetDirectories(FileTree* Base)
			{
				VMContext* Context = VMContext::Get();
				if (!Context)
					return nullptr;

				VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				for (auto& Node : Base->Directories)
					Factory::AddRef(Node);

				VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<Rest::FileTree>");
				return VMWArray::ComposeFromPointers(Type, Base->Directories).GetArray();
			}
			static VMCArray* FileTree_GetFiles(FileTree* Base)
			{
				VMContext* Context = VMContext::Get();
				if (!Context)
					return nullptr;

				VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<string>");
				return VMWArray::ComposeFromObjects(Type, Base->Files).GetArray();
			}
			static void FileTree_Loop(FileTree* Base, VMCAny* Any, VMCFunction* Callback)
			{
				if (!Callback)
				{
					VMWAny(Any).Release();
					return;
				}

				Base->Loop([Any, Callback](FileTree* Base)
				{
					VMContext* Context = VMContext::Get();
					if (!Context)
						return false;

					Context->PushState();
					Context->Prepare(Callback);
					Context->SetArgObject(0, Base);
					Context->SetArgObject(1, Any);
					Context->Execute();

					bool Result = Context->GetReturnByte();
					Context->PopState();

					return Result;
				});

				VMWAny(Any).Release();
				VMWFunction(Callback).Release();
			}

			static void Console_fWriteLine(Console* Base, const std::string& F, Format* Form)
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
			static void Console_fWrite(Console* Base, const std::string& F, Format* Form)
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

			static bool EventQueue_Task(EventQueue* Base, VMCAny* Any, VMCFunction* Callback, bool Keep)
			{
				if (!Callback)
				{
					VMWAny(Any).Release();
					return false;
				}

				return Base->Task<void>(Any, [Callback](EventQueue* Queue, EventArgs* Args)
				{
					VMContext* Context = VMContext::Get();
					if (!Context)
					{
						VMWAny(Args->Get<VMCAny>()).Release();
						VMWFunction(Callback).Release();
						Args->Data = nullptr;
						Args->Alive = false;
						return;
					}

					Context->PushState();
					Context->Prepare(Callback);
					Context->SetArgObject(0, Queue);
					Context->SetArgObject(1, Args->Get<VMCAny>());
					Context->Execute();

					if (!Context->GetReturnByte())
					{
						Args->Data = nullptr;
						Args->Alive = false;
					}

					VMWAny(Args->Get<VMCAny>()).Release();
					VMWFunction(Callback).Release();
					Context->PopState();
				}, Keep);
			}
			static bool EventQueue_Callback(EventQueue* Base, VMCAny* Any, VMCFunction* Callback, bool Keep)
			{
				if (!Callback)
				{
					VMWAny(Any).Release();
					return false;
				}

				return Base->Callback<void>(Any, [Callback](EventQueue* Queue, EventArgs* Args)
				{
					VMContext* Context = VMContext::Get();
					if (!Context)
					{
						VMWAny(Args->Get<VMCAny>()).Release();
						VMWFunction(Callback).Release();
						Args->Data = nullptr;
						Args->Alive = false;
						return;
					}

					Context->PushState();
					Context->Prepare(Callback);
					Context->SetArgObject(0, Queue);
					Context->SetArgObject(1, Args->Get<VMCAny>());
					Context->Execute();

					if (!Context->GetReturnByte())
					{
						Args->Data = nullptr;
						Args->Alive = false;
					}

					VMWAny(Args->Get<VMCAny>()).Release();
					VMWFunction(Callback).Release();
					Context->PopState();
				}, Keep);
			}
			static bool EventQueue_Interval(EventQueue* Base, VMCAny* Any, uint64_t Ms, VMCFunction* Callback)
			{
				if (!Callback)
				{
					VMWAny(Any).Release();
					return false;
				}

				return Base->Interval<void>(Any, Ms, [Callback](EventQueue* Queue, EventArgs* Args)
				{
					VMContext* Context = VMContext::Get();
					if (!Context)
					{
						VMWAny(Args->Get<VMCAny>()).Release();
						VMWFunction(Callback).Release();
						Args->Data = nullptr;
						Args->Alive = false;
						return;
					}

					Context->PushState();
					Context->Prepare(Callback);
					Context->SetArgObject(0, Queue);
					Context->SetArgObject(1, Args->Get<VMCAny>());
					Context->Execute();

					if (!Context->GetReturnByte())
					{
						VMWAny(Args->Get<VMCAny>()).Release();
						VMWFunction(Callback).Release();
						Args->Data = nullptr;
						Args->Alive = false;
					}

					Context->PopState();
				});
			}
			static bool EventQueue_Timeout(EventQueue* Base, VMCAny* Any, uint64_t Ms, VMCFunction* Callback)
			{
				if (!Callback)
				{
					VMWAny(Any).Release();
					return false;
				}

				return Base->Timeout<void>(Any, Ms, [Callback](EventQueue* Queue, EventArgs* Args)
				{
					VMContext* Context = VMContext::Get();
					if (!Context)
					{
						VMWAny(Args->Get<VMCAny>()).Release();
						VMWFunction(Callback).Release();
						Args->Data = nullptr;
						Args->Alive = false;
						return;
					}

					Context->PushState();
					Context->Prepare(Callback);
					Context->SetArgObject(0, Queue);
					Context->SetArgObject(1, Args->Get<VMCAny>());
					Context->Execute();

					if (!Context->GetReturnByte())
					{
						Args->Data = nullptr;
						Args->Alive = false;
					}

					VMWAny(Args->Get<VMCAny>()).Release();
					VMWFunction(Callback).Release();
					Context->PopState();
				});
			}

			static Document* Document_SetId(Document* Base, const std::string& Name, const std::string& Value)
			{
				if (Value.size() != 12)
					return nullptr;

				return Factory::MakeAddRef(Base->SetId(Name, (unsigned char*)Value.c_str()));
			}
			static std::string Document_GetDecimal(Document* Base, const std::string& Name)
			{
				Network::BSON::KeyPair Key;
				int64_t Low;
				Key.Mod = Network::BSON::Type_Decimal;
				Key.IsValid = true;
				Key.High = Base->GetDecimal(Name, &Low);
				Key.Low = Low;

				return Key.ToString();
			}
			static std::string Document_GetId(Document* Base, const std::string& Name)
			{
				unsigned char* Id = Base->GetId(Name);
				return std::string((const char*)Id, 12);
			}
			static VMCArray* Document_FindCollection(Document* Base, const std::string& Name, bool Here)
			{
				VMContext* Context = VMContext::Get();
				if (!Context)
					return nullptr;

				VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				std::vector<Document*> Nodes = Base->FindCollection(Name, Here);
				for (auto& Node : Nodes)
					Factory::AddRef(Node);

				VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<Rest::Document>");
				return VMWArray::ComposeFromPointers(Type, Nodes).GetArray();
			}
			static VMCArray* Document_FindCollectionPath(Document* Base, const std::string& Name, bool Here)
			{
				VMContext* Context = VMContext::Get();
				if (!Context)
					return nullptr;

				VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				std::vector<Document*> Nodes = Base->FindCollectionPath(Name, Here);
				for (auto& Node : Nodes)
					Factory::AddRef(Node);

				VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<Rest::Document>");
				return VMWArray::ComposeFromPointers(Type, Nodes).GetArray();
			}
			static VMCArray* Document_GetNodes(Document* Base)
			{
				VMContext* Context = VMContext::Get();
				if (!Context)
					return nullptr;

				VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				std::vector<Document*> Nodes = *Base->GetNodes();
				for (auto& Node : Nodes)
					Factory::AddRef(Node);

				VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<Rest::Document>");
				return VMWArray::ComposeFromPointers(Type, Nodes).GetArray();
			}
			static VMCArray* Document_GetAttributes(Document* Base)
			{
				VMContext* Context = VMContext::Get();
				if (!Context)
					return nullptr;

				VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				std::vector<Document*> Nodes = Base->GetAttributes();
				for (auto& Node : Nodes)
					Factory::AddRef(Node);

				VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<Rest::Document>");
				return VMWArray::ComposeFromPointers(Type, Nodes).GetArray();
			}
			static VMCDictionary* Document_CreateMapping(Document* Base)
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
			static std::string Document_ToJSON(Document* Base)
			{
				std::string Stream;
				Document::WriteJSON(Base, [&Stream](DocumentPretty, const char* Buffer, int64_t Length)
				{
					if (Buffer != nullptr && Length > 0)
						Stream.append(Buffer, Length);
				});

				return Stream;
			}
			static std::string Document_ToXML(Document* Base)
			{
				std::string Stream;
				Document::WriteXML(Base, [&Stream](DocumentPretty, const char* Buffer, int64_t Length)
				{
					if (Buffer != nullptr && Length > 0)
						Stream.append(Buffer, Length);
				});

				return Stream;
			}
			static Document* Document_FromJSON(const std::string& Value)
			{
				size_t Offset = 0;
				return Document::ReadJSON(Value.size(), [&Value, &Offset](char* Buffer, int64_t Size)
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
			static Document* Document_FromXML(const std::string& Value)
			{
				size_t Offset = 0;
				return Document::ReadXML(Value.size(), [&Value, &Offset](char* Buffer, int64_t Size)
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

			static std::string OS_Resolve_1(const std::string& Path)
			{
				return OS::Resolve(Path.c_str());
			}
			static std::string OS_Resolve_2(const std::string& Path, const std::string& Base)
			{
				return OS::Resolve(Path, Base);
			}
			static std::string OS_ResolveDir_1(const std::string& Path)
			{
				return OS::ResolveDir(Path.c_str());
			}
			static std::string OS_ResolveDir_2(const std::string& Path, const std::string& Base)
			{
				return OS::ResolveDir(Path, Base);
			}
			static std::string OS_Read(const std::string& Path)
			{
				return OS::Read(Path.c_str());
			}
			static std::string OS_FileExtention(const std::string& Path)
			{
				return OS::FileExtention(Path.c_str());
			}
			static FileState OS_GetState(const std::string& Path)
			{
				return OS::GetState(Path.c_str());
			}
			static void OS_Run(const std::string& Params)
			{
				return OS::Run("%s", Params.c_str());
			}
			static void OS_SetDirectory(const std::string& Path)
			{
				return OS::SetDirectory(Path.c_str());
			}
			static bool OS_FileExists(const std::string& Path)
			{
				return OS::FileExists(Path.c_str());
			}
			static bool OS_ExecExists(const std::string& Path)
			{
				return OS::ExecExists(Path.c_str());
			}
			static bool OS_DirExists(const std::string& Path)
			{
				return OS::DirExists(Path.c_str());
			}
			static bool OS_CreateDir(const std::string& Path)
			{
				return OS::CreateDir(Path.c_str());
			}
			static bool OS_Write(const std::string& Path, const std::string& Data)
			{
				return OS::Write(Path.c_str(), Data);
			}
			static bool OS_RemoveFile(const std::string& Path)
			{
				return OS::RemoveFile(Path.c_str());
			}
			static bool OS_RemoveDir(const std::string& Path)
			{
				return OS::RemoveDir(Path.c_str());
			}
			static bool OS_Move(const std::string& From, const std::string& To)
			{
				return OS::Move(From.c_str(), To.c_str());
			}
			static bool OS_StateResource(const std::string& Path, Resource& Value)
			{
				return OS::StateResource(Path, &Value);
			}
			static bool OS_FreeProcess(ChildProcess& Value)
			{
				return OS::FreeProcess(&Value);
			}
			static bool OS_AwaitProcess(ChildProcess& Value, int& ExitCode)
			{
				return OS::AwaitProcess(&Value, &ExitCode);
			}
			static ChildProcess OS_SpawnProcess(const std::string& Path, VMCArray* Array)
			{
				std::vector<std::string> Params;
				VMWArray::DecomposeToObjects(Array, &Params);

				ChildProcess Result;
				OS::SpawnProcess(Path, Params, &Result);
				return Result;
			}
			static VMCArray* OS_ScanFiles(const std::string& Path)
			{
				VMContext* Context = VMContext::Get();
				if (!Context)
					return nullptr;

				VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				std::vector<ResourceEntry> Entries;
				if (!OS::ScanDir(Path, &Entries))
					return nullptr;

				VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<Rest::ResourceEntry>");
				return VMWArray::ComposeFromObjects(Type, Entries).GetArray();
			}
			static VMCArray* OS_ScanDir(const std::string& Path)
			{
				VMContext* Context = VMContext::Get();
				if (!Context)
					return nullptr;

				VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				std::vector<DirectoryEntry> Entries;
				OS::Iterate(Path.c_str(), [&Entries](DirectoryEntry* Entry)
				{
					Entries.push_back(*Entry);
					return true;
				});

				VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<Rest::DirectoryEntry>");
				return VMWArray::ComposeFromObjects(Type, Entries).GetArray();
			}
			static VMCArray* OS_GetDiskDrives()
			{
				VMContext* Context = VMContext::Get();
				if (!Context)
					return nullptr;

				VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<string>");
				return VMWArray::ComposeFromObjects(Type, OS::GetDiskDrives()).GetArray();
			}

			static void IncludeDesc_AddExt(IncludeDesc* Base, const std::string& Value)
			{
				Base->Exts.push_back(Value);
			}
			static void IncludeDesc_RemoveExt(IncludeDesc* Base, const std::string& Value)
			{
				for (auto It = Base->Exts.begin(); It != Base->Exts.end(); It++)
				{
					if (*It == Value)
					{
						It = Base->Exts.erase(It);
						if (Base->Exts.empty())
							break;
					}
				}
			}
			static std::string IncludeDesc_GetExt(IncludeDesc* Base, unsigned int Index)
			{
				if (Index >= Base->Exts.size())
					return "";

				return Base->Exts[Index];
			}
			static unsigned int IncludeDesc_GetExtsCount(IncludeDesc* Base)
			{
				return (unsigned int)Base->Exts.size();
			}

			static Vector2 Vector2_Mul_1(Vector2* Base, const Vector2& V)
			{
				return *Base * V;
			}
			static Vector2 Vector2_Mul_2(Vector2* Base, float V)
			{
				return *Base * V;
			}
			static Vector2 Vector2_Div_1(Vector2* Base, const Vector2& V)
			{
				return *Base / V;
			}
			static Vector2 Vector2_Div_2(Vector2* Base, float V)
			{
				return *Base / V;
			}
			static Vector2 Vector2_Add_1(Vector2* Base, const Vector2& V)
			{
				return *Base + V;
			}
			static Vector2 Vector2_Add_2(Vector2* Base, float V)
			{
				return *Base + V;
			}
			static Vector2 Vector2_Sub_1(Vector2* Base, const Vector2& V)
			{
				return *Base - V;
			}
			static Vector2 Vector2_Sub_2(Vector2* Base, float V)
			{
				return *Base - V;
			}
			static Vector2 Vector2_Neg(Vector2* Base)
			{
				return -(*Base);
			}
			static Vector2& Vector2_SubAssign_1(Vector2* Base, const Vector2& V)
			{
				*Base -= V;
				return *Base;
			}
			static Vector2& Vector2_SubAssign_2(Vector2* Base, float V)
			{
				*Base -= V;
				return *Base;
			}
			static Vector2& Vector2_MulAssign_1(Vector2* Base, const Vector2& V)
			{
				*Base *= V;
				return *Base;
			}
			static Vector2& Vector2_MulAssign_2(Vector2* Base, float V)
			{
				*Base *= V;
				return *Base;
			}
			static Vector2& Vector2_DivAssign_1(Vector2* Base, const Vector2& V)
			{
				*Base /= V;
				return *Base;
			}
			static Vector2& Vector2_DivAssign_2(Vector2* Base, float V)
			{
				*Base /= V;
				return *Base;
			}
			static Vector2& Vector2_AddAssign_1(Vector2* Base, const Vector2& V)
			{
				*Base += V;
				return *Base;
			}
			static Vector2& Vector2_AddAssign_2(Vector2* Base, float V)
			{
				*Base += V;
				return *Base;
			}
			static bool Vector2_Equals(Vector2* Base, const Vector2& V)
			{
				return *Base == V;
			}
			static int Vector2_Cmp(Vector2* Base, const Vector2& V)
			{
				if (*Base == V)
					return 0;

				return (*Base > V ? 1 : -1);
			}
			static float Vector2_Index_1(Vector2* Base, int Axis)
			{
				return (*Base)[Axis];
			}
			static float& Vector2_Index_2(Vector2* Base, int Axis)
			{
				return (*Base)[Axis];
			}

			static Vector3 Vector3_Mul_1(Vector3* Base, const Vector3& V)
			{
				return *Base * V;
			}
			static Vector3 Vector3_Mul_2(Vector3* Base, float V)
			{
				return *Base * V;
			}
			static Vector3 Vector3_Div_1(Vector3* Base, const Vector3& V)
			{
				return *Base / V;
			}
			static Vector3 Vector3_Div_2(Vector3* Base, float V)
			{
				return *Base / V;
			}
			static Vector3 Vector3_Add_1(Vector3* Base, const Vector3& V)
			{
				return *Base + V;
			}
			static Vector3 Vector3_Add_2(Vector3* Base, float V)
			{
				return *Base + V;
			}
			static Vector3 Vector3_Sub_1(Vector3* Base, const Vector3& V)
			{
				return *Base - V;
			}
			static Vector3 Vector3_Sub_2(Vector3* Base, float V)
			{
				return *Base - V;
			}
			static Vector3 Vector3_Neg(Vector3* Base)
			{
				return -(*Base);
			}
			static Vector3& Vector3_SubAssign_1(Vector3* Base, const Vector3& V)
			{
				*Base -= V;
				return *Base;
			}
			static Vector3& Vector3_SubAssign_2(Vector3* Base, float V)
			{
				*Base -= V;
				return *Base;
			}
			static Vector3& Vector3_MulAssign_1(Vector3* Base, const Vector3& V)
			{
				*Base *= V;
				return *Base;
			}
			static Vector3& Vector3_MulAssign_2(Vector3* Base, float V)
			{
				*Base *= V;
				return *Base;
			}
			static Vector3& Vector3_DivAssign_1(Vector3* Base, const Vector3& V)
			{
				*Base /= V;
				return *Base;
			}
			static Vector3& Vector3_DivAssign_2(Vector3* Base, float V)
			{
				*Base /= V;
				return *Base;
			}
			static Vector3& Vector3_AddAssign_1(Vector3* Base, const Vector3& V)
			{
				*Base += V;
				return *Base;
			}
			static Vector3& Vector3_AddAssign_2(Vector3* Base, float V)
			{
				*Base += V;
				return *Base;
			}
			static bool Vector3_Equals(Vector3* Base, const Vector3& V)
			{
				return *Base == V;
			}
			static int Vector3_Cmp(Vector3* Base, const Vector3& V)
			{
				if (*Base == V)
					return 0;

				return (*Base > V ? 1 : -1);
			}
			static float Vector3_Index_1(Vector3* Base, int Axis)
			{
				return (*Base)[Axis];
			}
			static float& Vector3_Index_2(Vector3* Base, int Axis)
			{
				return (*Base)[Axis];
			}

			static Vector4 Vector4_Mul_1(Vector4* Base, const Vector4& V)
			{
				return *Base * V;
			}
			static Vector4 Vector4_Mul_2(Vector4* Base, float V)
			{
				return *Base * V;
			}
			static Vector4 Vector4_Div_1(Vector4* Base, const Vector4& V)
			{
				return *Base / V;
			}
			static Vector4 Vector4_Div_2(Vector4* Base, float V)
			{
				return *Base / V;
			}
			static Vector4 Vector4_Add_1(Vector4* Base, const Vector4& V)
			{
				return *Base + V;
			}
			static Vector4 Vector4_Add_2(Vector4* Base, float V)
			{
				return *Base + V;
			}
			static Vector4 Vector4_Sub_1(Vector4* Base, const Vector4& V)
			{
				return *Base - V;
			}
			static Vector4 Vector4_Sub_2(Vector4* Base, float V)
			{
				return *Base - V;
			}
			static Vector4 Vector4_Neg(Vector4* Base)
			{
				return -(*Base);
			}
			static Vector4& Vector4_SubAssign_1(Vector4* Base, const Vector4& V)
			{
				*Base -= V;
				return *Base;
			}
			static Vector4& Vector4_SubAssign_2(Vector4* Base, float V)
			{
				*Base -= V;
				return *Base;
			}
			static Vector4& Vector4_MulAssign_1(Vector4* Base, const Vector4& V)
			{
				*Base *= V;
				return *Base;
			}
			static Vector4& Vector4_MulAssign_2(Vector4* Base, float V)
			{
				*Base *= V;
				return *Base;
			}
			static Vector4& Vector4_DivAssign_1(Vector4* Base, const Vector4& V)
			{
				*Base /= V;
				return *Base;
			}
			static Vector4& Vector4_DivAssign_2(Vector4* Base, float V)
			{
				*Base /= V;
				return *Base;
			}
			static Vector4& Vector4_AddAssign_1(Vector4* Base, const Vector4& V)
			{
				*Base += V;
				return *Base;
			}
			static Vector4& Vector4_AddAssign_2(Vector4* Base, float V)
			{
				*Base += V;
				return *Base;
			}
			static bool Vector4_Equals(Vector4* Base, const Vector4& V)
			{
				return *Base == V;
			}
			static int Vector4_Cmp(Vector4* Base, const Vector4& V)
			{
				if (*Base == V)
					return 0;

				return (*Base > V ? 1 : -1);
			}
			static float Vector4_Index_1(Vector4* Base, int Axis)
			{
				return (*Base)[Axis];
			}
			static float& Vector4_Index_2(Vector4* Base, int Axis)
			{
				return (*Base)[Axis];
			}

			static VMCArray* Matrix4x4_GetRow(Matrix4x4* Base)
			{
				VMContext* Context = VMContext::Get();
				if (!Context)
					return nullptr;

				VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<float>");
				VMWArray Array = VMWArray::Create(Type);

				Array.Resize(16);
				for (unsigned int i = 0; i < 16; i++)
					Array.SetValue(i, &Base->Row[i]);

				return Array.GetArray();
			}
			static Matrix4x4 Matrix4x4_Mul_1(Matrix4x4* Base, const Matrix4x4& V)
			{
				return *Base * V;
			}
			static Matrix4x4 Matrix4x4_Mul_2(Matrix4x4* Base, const Vector4& V)
			{
				return *Base * V;
			}
			static bool Matrix4x4_Equals(Matrix4x4* Base, const Matrix4x4& V)
			{
				return *Base == V;
			}
			static float Matrix4x4_Index_1(Matrix4x4* Base, int Axis)
			{
				if (Axis < 0 || Axis > 15)
					return 0;

				return (*Base)[Axis];
			}
			static float& Matrix4x4_Index_2(Matrix4x4* Base, int Axis)
			{
				if (Axis < 0 || Axis > 15)
					return (*Base)[0];

				return (*Base)[Axis];
			}

			static unsigned int SkinAnimatorClip_GetFramesCount(SkinAnimatorClip* Base)
			{
				return (unsigned int)Base->Keys.size();
			}
			static unsigned int SkinAnimatorClip_GetKeysCount(SkinAnimatorClip* Base, unsigned int Index)
			{
				if (Index >= Base->Keys.size())
					return 0;

				return (unsigned int)Base->Keys[Index].size();
			}
			static AnimatorKey& SkinAnimatorClip_GetKey(SkinAnimatorClip* Base, unsigned int Frame, unsigned int Key)
			{
				if (Frame >= Base->Keys.size() || Key >= Base->Keys[Frame].size())
				{
					if (Base->Keys.empty())
						Base->Keys.emplace_back();

					if (Base->Keys[0].empty())
						Base->Keys[0].emplace_back();

					return Base->Keys[0][0];
				}

				return Base->Keys[Frame][Key];
			}

			static unsigned int KeyAnimatorClip_GetKeysCount(KeyAnimatorClip* Base)
			{
				return (unsigned int)Base->Keys.size();
			}
			static AnimatorKey& KeyAnimatorClip_GetKey(KeyAnimatorClip* Base, unsigned int Index)
			{
				if (Index >= Base->Keys.size())
				{
					if (Base->Keys.empty())
						Base->Keys.emplace_back();

					return Base->Keys[0];
				}

				return Base->Keys[Index];
			}

			static unsigned int Joint_GetChildsCount(Joint* Base)
			{
				return (unsigned int)Base->Childs.size();
			}
			static Joint& Joint_GetChild(Joint* Base, unsigned int Index)
			{
				if (Index >= Base->Childs.size())
				{
					if (Base->Childs.empty())
						Base->Childs.emplace_back();

					return Base->Childs[0];
				}

				return Base->Childs[Index];
			}

			static unsigned short Hybi10PayloadHeader_GetOpcode(Hybi10PayloadHeader* Base)
			{
				return Base->Opcode;
			}
			static unsigned short Hybi10PayloadHeader_GetRsv1(Hybi10PayloadHeader* Base)
			{
				return Base->Rsv1;
			}
			static unsigned short Hybi10PayloadHeader_GetRsv2(Hybi10PayloadHeader* Base)
			{
				return Base->Rsv2;
			}
			static unsigned short Hybi10PayloadHeader_GetRsv3(Hybi10PayloadHeader* Base)
			{
				return Base->Rsv3;
			}
			static unsigned short Hybi10PayloadHeader_GetFin(Hybi10PayloadHeader* Base)
			{
				return Base->Fin;
			}
			static unsigned short Hybi10PayloadHeader_GetPayloadLength(Hybi10PayloadHeader* Base)
			{
				return Base->PayloadLength;
			}
			static unsigned short Hybi10PayloadHeader_GetMask(Hybi10PayloadHeader* Base)
			{
				return Base->Mask;
			}

			static void MD5Hasher_Transform(MD5Hasher* Base, const std::string& Value, unsigned int Length)
			{
				Base->Transform((const unsigned char*)Value.c_str(), Length);
			}

			static std::string RegexBracket_GetPointer(RegexBracket* Base)
			{
				if (!Base->Pointer)
					return "";

				return std::string(Base->Pointer, Base->Length);
			}

			static std::string RegexMatch_GetPointer(RegexMatch* Base)
			{
				if (!Base->Pointer)
					return "";

				return std::string(Base->Pointer, Base->Length);
			}

			static std::string RegexResult_GetPointer(RegexResult* Base)
			{
				if (!Base->Pointer())
					return "";

				return std::string(Base->Pointer(), Base->Length());
			}

			static std::string Regex_Syntax()
			{
				return Regex::Syntax();
			}

			static std::string MathCommon_Sha256Encode(const std::string& Buffer, const std::string& Key, const std::string& IV)
			{
				return MathCommon::Sha256Encode(Buffer.c_str(), Key.c_str(), IV.c_str());
			}
			static std::string MathCommon_Sha256Decode(const std::string& Buffer, const std::string& Key, const std::string& IV)
			{
				return MathCommon::Sha256Decode(Buffer.c_str(), Key.c_str(), IV.c_str());
			}
			static std::string MathCommon_Aes256Encode(const std::string& Buffer, const std::string& Key, const std::string& IV)
			{
				return MathCommon::Aes256Encode(Buffer, Key.c_str(), IV.c_str());
			}
			static std::string MathCommon_Aes256Decode(const std::string& Buffer, const std::string& Key, const std::string& IV)
			{
				return MathCommon::Aes256Decode(Buffer, Key.c_str(), IV.c_str());
			}

			static void Preprocessor_SetIncludeCallbackOnce(Preprocessor* Base, VMCFunction* Callback)
			{
				Base->SetIncludeCallback([Callback](Preprocessor* Base, const IncludeResult& File, std::string* Output)
				{
					VMContext* Context = VMContext::Get();
					if (!Context)
					{
						VMWFunction(Callback).Release();
						return false;
					}

					Context->PushState();
					Context->Prepare(Callback);
					Context->SetArgObject(0, Base);
					Context->SetArgObject(1, &File);
					Context->SetArgObject(2, Output);
					Context->Execute();

					bool Result = Context->GetReturnByte();
					Context->PopState();

					VMWFunction(Callback).Release();
					Base->SetIncludeCallback(nullptr);
					return Result;
				});
			}
			static void Preprocessor_SetPragmaCallbackOnce(Preprocessor* Base, VMCFunction* Callback)
			{
				Base->SetPragmaCallback([Callback](Preprocessor* Base, const std::string& Pragma)
				{
					VMContext* Context = VMContext::Get();
					if (!Context)
					{
						VMWFunction(Callback).Release();
						return false;
					}

					Context->PushState();
					Context->Prepare(Callback);
					Context->SetArgObject(0, Base);
					Context->SetArgObject(1, &Pragma);
					Context->Execute();

					bool Result = Context->GetReturnByte();
					Context->PopState();

					VMWFunction(Callback).Release();
					Base->SetPragmaCallback(nullptr);
					return Result;
				});
			}
			static bool Preprocessor_IsDefined(Preprocessor* Base, const std::string& Name)
			{
				return Base->IsDefined(Name.c_str());
			}

			static Vector3& Transform_GetLocalPosition(Transform* Base)
			{
				return *Base->GetLocalPosition();
			}
			static Vector3& Transform_GetLocalRotation(Transform* Base)
			{
				return *Base->GetLocalRotation();
			}
			static Vector3& Transform_GetLocalScale(Transform* Base)
			{
				return *Base->GetLocalScale();
			}

			static void SoftBodyDesc_InitRope(SoftBody::Desc* Base, int Count, const Vector3& Start, bool StartFixed, const Vector3& End, bool EndFixed)
			{
				Base->Shape.Convex.Enabled = false;
				Base->Shape.Rope.Enabled = true;
				Base->Shape.Patch.Enabled = false;
				Base->Shape.Ellipsoid.Enabled = false;
				Base->Shape.Rope.Count = Count;
				Base->Shape.Rope.Start = Start;
				Base->Shape.Rope.StartFixed = StartFixed;
				Base->Shape.Rope.End = End;
				Base->Shape.Rope.EndFixed = EndFixed;
			}
			static void SoftBodyDesc_InitPatch(SoftBody::Desc* Base, int CountX, int CountY, bool Gen, const Vector3& Corner00, bool Corner00Fixed, const Vector3& Corner01, bool Corner01Fixed, const Vector3& Corner10, bool Corner10Fixed, const Vector3& Corner11, bool Corner11Fixed)
			{
				Base->Shape.Convex.Enabled = false;
				Base->Shape.Rope.Enabled = false;
				Base->Shape.Patch.Enabled = true;
				Base->Shape.Ellipsoid.Enabled = false;
				Base->Shape.Patch.CountX = CountX;
				Base->Shape.Patch.CountY = CountY;
				Base->Shape.Patch.GenerateDiagonals = Gen;
				Base->Shape.Patch.Corner00 = Corner00;
				Base->Shape.Patch.Corner00Fixed = Corner00Fixed;
				Base->Shape.Patch.Corner01 = Corner01;
				Base->Shape.Patch.Corner01Fixed = Corner01Fixed;
				Base->Shape.Patch.Corner10 = Corner10;
				Base->Shape.Patch.Corner10Fixed = Corner10Fixed;
				Base->Shape.Patch.Corner11 = Corner11;
				Base->Shape.Patch.Corner11Fixed = Corner11Fixed;
			}
			static void SoftBodyDesc_InitEllipsoid(SoftBody::Desc* Base, int Count, const Vector3& Center, const Vector3& Radius)
			{
				Base->Shape.Convex.Enabled = false;
				Base->Shape.Rope.Enabled = false;
				Base->Shape.Patch.Enabled = false;
				Base->Shape.Ellipsoid.Enabled = true;
				Base->Shape.Ellipsoid.Count = Count;
				Base->Shape.Ellipsoid.Center = Center;
				Base->Shape.Ellipsoid.Radius = Radius;
			}

			static RigidBody* SliderConstraint_GetFirst(SliderConstraint* Base)
			{
				return RigidBody::Get(Base->GetFirst());
			}
			static RigidBody* SliderConstraint_GetSecond(SliderConstraint* Base)
			{
				return RigidBody::Get(Base->GetSecond());
			}

			static void Simulator_InitShape(Simulator* Base, RigidBody::Desc& Desc, Shape Type)
			{
				Desc.Shape = Base->CreateShape(Type);
			}
			static void Simulator_InitCube(Simulator* Base, RigidBody::Desc& Desc, const Vector3& Scale)
			{
				Desc.Shape = Base->CreateCube(Scale);
			}
			static void Simulator_InitSphere(Simulator* Base, RigidBody::Desc& Desc, float Radius)
			{
				Desc.Shape = Base->CreateSphere(Radius);
			}
			static void Simulator_InitCapsule(Simulator* Base, RigidBody::Desc& Desc, float Radius, float Height)
			{
				Desc.Shape = Base->CreateCapsule(Radius, Height);
			}
			static void Simulator_InitCone(Simulator* Base, RigidBody::Desc& Desc, float Radius, float Height)
			{
				Desc.Shape = Base->CreateCone(Radius, Height);
			}
			static void Simulator_InitCylinder(Simulator* Base, RigidBody::Desc& Desc, const Vector3& Scale)
			{
				Desc.Shape = Base->CreateCylinder(Scale);
			}
			static void Simulator_InitConvex(Simulator* Base, RigidBody::Desc& Desc, VMCArray* Array)
			{
				if (!Array)
					return;

				std::vector<Vector3> Vertices;
				VMWArray::DecomposeToObjects(Array, &Vertices);
				Desc.Shape = Base->CreateConvexHull(Vertices);
			}

			static bool Regex_Match(RegExp& Value, RegexResult& Result, const std::string& Buffer)
			{
				return Regex::Match(&Value, &Result, Buffer);
			}
			static bool Regex_MatchAll(RegExp& Value, RegexResult& Result, const std::string& Buffer)
			{
				return Regex::MatchAll(&Value, &Result, Buffer);
			}

			static int Socket_AcceptAsync(Socket* Base, VMCAny* Any, VMCFunction* Callback)
			{
				VMWFunction(Callback).AddRef();
				VMWAny(Any).AddRef();

				return Base->AcceptAsync([Callback, Any](Socket* Base)
				{
					VMContext* Context = VMContext::Get();
					if (!Context)
					{
						VMWFunction(Callback).Release();
						VMWAny(Any).Release();
						return false;
					}

					Context->PushState();
					Context->Prepare(Callback);
					Context->SetArgObject(0, Base);
					Context->SetArgObject(1, Any);
					Context->Execute();

					bool Result = Context->GetReturnByte();
					Context->PopState();

					VMWFunction(Callback).Release();
					VMWAny(Any).Release();

					return Result;
				});
			}
			static int Socket_Open(Socket* Base, const std::string& Host, int Port, SocketType Type, Address* Result)
			{
				return Base->Open(Host.c_str(), Port, Type, Result);
			}
			static int Socket_Write(Socket* Base, const std::string& Buffer)
			{
				return Base->Write(Buffer.c_str(), (int)Buffer.size());
			}
			static int Socket_WriteAsync(Socket* Base, const std::string& Buffer, VMCAny* Any, VMCFunction* Callback)
			{
				VMWFunction(Callback).AddRef();
				VMWAny(Any).AddRef();

				return Base->WriteAsync(Buffer.c_str(), (int)Buffer.size(), [Callback, Any](Socket* Base, int64_t Size)
				{
					VMContext* Context = VMContext::Get();
					if (!Context)
					{
						VMWFunction(Callback).Release();
						VMWAny(Any).Release();
						return false;
					}

					Context->PushState();
					Context->Prepare(Callback);
					Context->SetArgObject(0, Base);
					Context->SetArgObject(1, Any);
					Context->SetArgObject(2, &Size);
					Context->Execute();

					bool Result = Context->GetReturnByte();
					Context->PopState();

					VMWFunction(Callback).Release();
					VMWAny(Any).Release();

					return Result;
				});
			}
			static int Socket_Read(Socket* Base, std::string& Buffer, int Size)
			{
				if (Size <= 0)
					return 0;

				char* Data = (char*)malloc(sizeof(char) * Size);
				int Result = Base->Read(Data, Size);
				Buffer.assign(Data, (size_t)Size);
				free(Data);

				return Result;
			}
			static int Socket_ReadAsync(Socket* Base, int64_t Size, VMCAny* Any, VMCFunction* Callback)
			{
				VMWFunction(Callback).AddRef();
				VMWAny(Any).AddRef();

				return Base->ReadAsync(Size, [Callback, Any](Socket* Base, const char* Data, int64_t Size)
				{
					VMContext* Context = VMContext::Get();
					if (!Context)
					{
						VMWFunction(Callback).Release();
						VMWAny(Any).Release();
						return false;
					}

					std::string Buffer;
					if (Size > 0)
						Buffer.assign(Data, Size);

					Context->PushState();
					Context->Prepare(Callback);
					Context->SetArgObject(0, Base);
					Context->SetArgObject(1, Any);
					Context->SetArgObject(2, &Buffer);
					Context->SetArgObject(3, &Size);
					Context->Execute();

					bool Result = Context->GetReturnByte();
					Context->PopState();

					VMWFunction(Callback).Release();
					VMWAny(Any).Release();

					return Result;
				});
			}
			static int Socket_ReadUntil(Socket* Base, const std::string& Match, VMCAny* Any, VMCFunction* Callback)
			{
				VMWFunction(Callback).AddRef();
				VMWAny(Any).AddRef();

				return Base->ReadUntil(Match.c_str(), [Callback, Any](Socket* Base, const char* Data, int64_t Size)
				{
					VMContext* Context = VMContext::Get();
					if (!Context)
					{
						VMWFunction(Callback).Release();
						VMWAny(Any).Release();
						return false;
					}

					std::string Buffer;
					if (Size > 0)
						Buffer.assign(Data, Size);

					Context->PushState();
					Context->Prepare(Callback);
					Context->SetArgObject(0, Base);
					Context->SetArgObject(1, Any);
					Context->SetArgObject(2, &Buffer);
					Context->SetArgObject(3, &Size);
					Context->Execute();

					bool Result = Context->GetReturnByte();
					Context->PopState();

					VMWFunction(Callback).Release();
					VMWAny(Any).Release();

					return Result;
				});
			}
			static int Socket_ReadUntilAsync(Socket* Base, const std::string& Match, VMCAny* Any, VMCFunction* Callback)
			{
				VMWFunction(Callback).AddRef();
				VMWAny(Any).AddRef();

				return Base->ReadUntilAsync(Match.c_str(), [Callback, Any](Socket* Base, const char* Data, int64_t Size)
				{
					VMContext* Context = VMContext::Get();
					if (!Context)
					{
						VMWFunction(Callback).Release();
						VMWAny(Any).Release();
						return false;
					}

					std::string Buffer;
					if (Size > 0)
						Buffer.assign(Data, Size);

					Context->PushState();
					Context->Prepare(Callback);
					Context->SetArgObject(0, Base);
					Context->SetArgObject(1, Any);
					Context->SetArgObject(2, &Buffer);
					Context->SetArgObject(3, &Size);
					Context->Execute();

					bool Result = Context->GetReturnByte();
					Context->PopState();

					VMWFunction(Callback).Release();
					VMWAny(Any).Release();

					return Result;
				});
			}

			void EnableRest(VMManager* Manager)
			{
				Manager->Namespace("Rest", [](VMGlobal* Global)
				{
					VMWEnum VEventState = Global->SetEnum("EventState");
					VMWEnum VEventWorkflow = Global->SetEnum("EventWorkflow");
					VMWEnum VEventType = Global->SetEnum("EventType");
					VMWEnum VNodeType = Global->SetEnum("NodeType");
					VMWEnum VDocumentPretty = Global->SetEnum("DocumentPretty");
					VMWEnum VFileMode = Global->SetEnum("FileMode");
					VMWEnum VFileSeek = Global->SetEnum("FileSeek");
					VMWTypeClass VFileState = Global->SetPod<FileState>("FileState");
					VMWTypeClass VResource = Global->SetPod<Resource>("Resource");
					VMWTypeClass VResourceEntry = Global->SetStructUnmanaged<ResourceEntry>("ResourceEntry");
					VMWTypeClass VDirectoryEntry = Global->SetStructUnmanaged<DirectoryEntry>("DirectoryEntry");
					VMWTypeClass VChildProcess = Global->SetStructUnmanaged<ChildProcess>("ChildProcess");
					VMWTypeClass VDateTime = Global->SetStructUnmanaged<DateTime>("DateTime");
					VMWTypeClass VTickTimer = Global->SetStructUnmanaged<TickTimer>("TickTimer");
					VMWRefClass VFormat = Global->SetClassUnmanaged<Format>("Format");
					VMWRefClass VConsole = Global->SetClassUnmanaged<Console>("Console");
					VMWRefClass VTimer = Global->SetClassUnmanaged<Timer>("Timer");
					VMWRefClass VFileStream = Global->SetClassUnmanaged<FileStream>("FileStream");
					VMWRefClass VFileLogger = Global->SetClassUnmanaged<FileLogger>("FileLogger");
					VMWRefClass VFileTree = Global->SetClassUnmanaged<FileTree>("FileTree");
					VMWRefClass VEventQueue = Global->SetClassUnmanaged<EventQueue>("EventQueue");
					VMWRefClass VDocument = Global->SetClassUnmanaged<Document>("Document");
					VMWTypeClass VOS = Global->SetStructUnmanaged<OS>("OS");

					VEventState.SetValue("Working", EventState_Working);
					VEventState.SetValue("Idle", EventState_Idle);
					VEventState.SetValue("Terminated", EventState_Terminated);

					VEventWorkflow.SetValue("Mixed", EventWorkflow_Mixed);
					VEventWorkflow.SetValue("Singlethreaded", EventWorkflow_Singlethreaded);
					VEventWorkflow.SetValue("Multithreaded", EventWorkflow_Multithreaded);
	
					VEventType.SetValue("Events", EventType_Events);
					VEventType.SetValue("Pull", EventType_Pull);
					VEventType.SetValue("Subscribe", EventType_Subscribe);
					VEventType.SetValue("Unsubscribe", EventType_Unsubscribe);
					VEventType.SetValue("Tasks", EventType_Tasks);
					VEventType.SetValue("Timers", EventType_Timers);

					VEventType.SetValue("Array", NodeType_Array);
					VEventType.SetValue("Boolean", NodeType_Boolean);
					VEventType.SetValue("Decimal", NodeType_Decimal);
					VEventType.SetValue("Integer", NodeType_Integer);
					VEventType.SetValue("Number", NodeType_Number);
					VEventType.SetValue("Object", NodeType_Object);
					VEventType.SetValue("Id", NodeType_Id);
					VEventType.SetValue("String", NodeType_String);
					VEventType.SetValue("Null", NodeType_Null);
					VEventType.SetValue("Undefined", NodeType_Undefined);

					VDocumentPretty.SetValue("Dummy", DocumentPretty_Dummy);
					VDocumentPretty.SetValue("TabDecrease", DocumentPretty_Tab_Decrease);
					VDocumentPretty.SetValue("TabIncrease", DocumentPretty_Tab_Increase);
					VDocumentPretty.SetValue("WriteSpace", DocumentPretty_Write_Space);
					VDocumentPretty.SetValue("WriteLine", DocumentPretty_Write_Line);
					VDocumentPretty.SetValue("WriteTab", DocumentPretty_Write_Tab);

					VFileMode.SetValue("ReadOnly", FileMode_Read_Only);
					VFileMode.SetValue("WriteOnly", FileMode_Write_Only);
					VFileMode.SetValue("AppendOnly", FileMode_Append_Only);
					VFileMode.SetValue("ReadWrite", FileMode_Read_Write);
					VFileMode.SetValue("WriteRead", FileMode_Write_Read);
					VFileMode.SetValue("ReadAppendWrite", FileMode_Read_Append_Write);
					VFileMode.SetValue("BinaryReadOnly", FileMode_Binary_Read_Only);
					VFileMode.SetValue("BinaryWriteOnly", FileMode_Binary_Write_Only);
					VFileMode.SetValue("BinaryAppendOnly", FileMode_Binary_Append_Only);
					VFileMode.SetValue("BinaryReadWrite", FileMode_Binary_Read_Write);
					VFileMode.SetValue("BinaryWriteRead", FileMode_Binary_Write_Read);
					VFileMode.SetValue("BinaryReadAppendWrite", FileMode_Binary_Read_Append_Write);

					VFileSeek.SetValue("Begin", FileSeek_Begin);
					VFileSeek.SetValue("Current", FileSeek_Current);
					VFileSeek.SetValue("End", FileSeek_End);

					VFileState.SetProperty("uint64 Size", &FileState::Size);
					VFileState.SetProperty("uint64 Links", &FileState::Links);
					VFileState.SetProperty("uint64 Permissions", &FileState::Permissions);
					VFileState.SetProperty("uint64 IDocument", &FileState::IDocument);
					VFileState.SetProperty("uint64 Device", &FileState::Device);
					VFileState.SetProperty("uint64 UserId", &FileState::UserId);
					VFileState.SetProperty("uint64 GroupId", &FileState::GroupId);
					VFileState.SetProperty("int64 LastAccess", &FileState::LastAccess);
					VFileState.SetProperty("int64 LastPermissionChange", &FileState::LastPermissionChange);
					VFileState.SetProperty("int64 LastModified", &FileState::LastModified);
					VFileState.SetProperty("bool Exists", &FileState::Exists);

					VResource.SetProperty("uint64 Size", &Resource::Size);
					VResource.SetProperty("int64 LastModified", &Resource::LastModified);
					VResource.SetProperty("int64 CreationTime", &Resource::CreationTime);
					VResource.SetProperty("bool IsReferenced", &Resource::IsReferenced);
					VResource.SetProperty("bool IsDirectory", &Resource::IsDirectory);

					VResourceEntry.SetProperty("string Path", &ResourceEntry::Path);
					VResourceEntry.SetProperty("Resource Source", &ResourceEntry::Source);
					VResourceEntry.SetConstructor<ResourceEntry>("void f()");

					VDirectoryEntry.SetProperty("string Path", &DirectoryEntry::Path);
					VDirectoryEntry.SetProperty("bool IsDirectory", &DirectoryEntry::IsDirectory);
					VDirectoryEntry.SetProperty("bool IsGood", &DirectoryEntry::IsGood);
					VDirectoryEntry.SetProperty("uint64 Length", &DirectoryEntry::Length);
					VDirectoryEntry.SetConstructor<DirectoryEntry>("void f()");

					VChildProcess.SetConstructor<ChildProcess>("void f()");
	
					VDateTime.SetConstructor<DateTime>("void f()");
					VDateTime.SetMethod("string Format(const string &in)", &DateTime::Format);
					VDateTime.SetMethod("string Date(const string &in)", &DateTime::Date);
					VDateTime.SetMethod("DateTime Now()", &DateTime::Now);
					VDateTime.SetMethod("DateTime FromNanoseconds(uint64)", &DateTime::FromNanoseconds);
					VDateTime.SetMethod("DateTime FromMicroseconds(uint64)", &DateTime::FromMicroseconds);
					VDateTime.SetMethod("DateTime FromMilliseconds(uint64)", &DateTime::FromMilliseconds);
					VDateTime.SetMethod("DateTime FromSeconds(uint64)", &DateTime::FromSeconds);
					VDateTime.SetMethod("DateTime FromMinutes(uint64)", &DateTime::FromMinutes);
					VDateTime.SetMethod("DateTime FromHours(uint64)", &DateTime::FromHours);
					VDateTime.SetMethod("DateTime FromDays(uint64)", &DateTime::FromDays);
					VDateTime.SetMethod("DateTime FromWeeks(uint64)", &DateTime::FromWeeks);
					VDateTime.SetMethod("DateTime FromMonths(uint64)", &DateTime::FromMonths);
					VDateTime.SetMethod("DateTime FromYears(uint64)", &DateTime::FromYears);
					VDateTime.SetMethod("DateTime& SetDateSeconds(uint64, bool)", &DateTime::SetDateSeconds);
					VDateTime.SetMethod("DateTime& SetDateMinutes(uint64, bool)", &DateTime::SetDateMinutes);
					VDateTime.SetMethod("DateTime& SetDateHours(uint64, bool)", &DateTime::SetDateHours);
					VDateTime.SetMethod("DateTime& SetDateDay(uint64, bool)", &DateTime::SetDateDay);
					VDateTime.SetMethod("DateTime& SetDateWeek(uint64, bool)", &DateTime::SetDateWeek);
					VDateTime.SetMethod("DateTime& SetDateMonth(uint64, bool)", &DateTime::SetDateMonth);
					VDateTime.SetMethod("DateTime& SetDateYear(uint64, bool)", &DateTime::SetDateYear);
					VDateTime.SetMethod("uint64 Nanoseconds()", &DateTime::Nanoseconds);
					VDateTime.SetMethod("uint64 Microseconds()", &DateTime::Microseconds);
					VDateTime.SetMethod("uint64 Milliseconds()", &DateTime::Milliseconds);
					VDateTime.SetMethod("uint64 Seconds()", &DateTime::Seconds);
					VDateTime.SetMethod("uint64 Minutes()", &DateTime::Minutes);
					VDateTime.SetMethod("uint64 Hours()", &DateTime::Hours);
					VDateTime.SetMethod("uint64 Days()", &DateTime::Days);
					VDateTime.SetMethod("uint64 Weeks()", &DateTime::Weeks);
					VDateTime.SetMethod("uint64 Months()", &DateTime::Months);
					VDateTime.SetMethod("uint64 Years()", &DateTime::Years);
					VDateTime.SetMethodStatic("string GetGMTBasedString(uint64)", &DateTime::GetGMTBasedString);
	
					VTickTimer.SetProperty("double Delay", &TickTimer::Delay);
					VTickTimer.SetConstructor<TickTimer>("void f()");
					VTickTimer.SetMethod("bool OnTickEvent(double)", &TickTimer::OnTickEvent);
					VTickTimer.SetMethod("double GetTime()", &TickTimer::GetTime);

					VFormat.SetUnmanagedConstructor<Format>("Format@ f()");
					VFormat.SetUnmanagedConstructorList<Format>("Format@ f(int &in) {repeat ?}");
					VFormat.SetMethodStatic("string JSON(const ? &in)", &Format::JSON);

					VConsole.SetUnmanagedConstructor<Console>("Console@ f()");
					VConsole.SetMethod("void Hide()", &Console::Hide);
					VConsole.SetMethod("void Show()", &Console::Show);
					VConsole.SetMethod("void Clear()", &Console::Clear);
					VConsole.SetMethod("void Flush()", &Console::Flush);
					VConsole.SetMethod("void FlushWrite()", &Console::FlushWrite);
					VConsole.SetMethod("void CaptureTime()", &Console::CaptureTime);
					VConsole.SetMethod("void WriteLine(const string &in)", &Console::sWriteLine);
					VConsole.SetMethodEx("void fWriteLine(const string &in, Format@+)", &Console_fWriteLine);
					VConsole.SetMethod("void Write(const string &in)", &Console::sWrite);
					VConsole.SetMethodEx("void fWrite(const string &in, Format@+)", &Console_fWrite);
					VConsole.SetMethod("double GetCapturedTime()", &Console::GetCapturedTime);
					VConsole.SetMethod("string Read(uint64)", &Console::Read);
					VConsole.SetMethodStatic("Console@+ Get()", &Console::Get);

					VTimer.SetProperty<Timer>("double FrameRelation", &Timer::FrameRelation);
					VTimer.SetProperty<Timer>("double FrameLimit", &Timer::FrameLimit);
					VTimer.SetUnmanagedConstructor<Timer>("Timer@ f()");
					VTimer.SetMethod("void SetStepLimitation(double, double)", &Timer::SetStepLimitation);
					VTimer.SetMethod("void Synchronize()", &Timer::Synchronize);
					VTimer.SetMethod("void CaptureTime()", &Timer::CaptureTime);
					VTimer.SetMethod("void Sleep(uint64)", &Timer::Sleep);
					VTimer.SetMethod("double GetTimeIncrement()", &Timer::GetTimeIncrement);
					VTimer.SetMethod("double GetTickCounter()", &Timer::GetTickCounter);
					VTimer.SetMethod("double GetFrameCount()", &Timer::GetFrameCount);
					VTimer.SetMethod("double GetElapsedTime()", &Timer::GetElapsedTime);
					VTimer.SetMethod("double GetCapturedTime()", &Timer::GetCapturedTime);
					VTimer.SetMethod("double GetDeltaTime()", &Timer::GetDeltaTime);
					VTimer.SetMethod("double GetTimeStep()", &Timer::GetTimeStep);

					VFileStream.SetUnmanagedConstructor<FileStream>("FileStream@ f()");
					VFileStream.SetMethod("void Clear()", &FileStream::Clear);
					VFileStream.SetMethod("bool Close()", &FileStream::Close);
					VFileStream.SetMethod("bool Seek(FileSeek, int64)", &FileStream::Seek);
					VFileStream.SetMethod("bool Move(int64)", &FileStream::Move);
					VFileStream.SetMethod("int Error()", &FileStream::Error);
					VFileStream.SetMethod("int Flush()", &FileStream::Flush);
					VFileStream.SetMethod("int Fd()", &FileStream::Fd);
					VFileStream.SetMethod("uint8 Get()", &FileStream::Get);
					VFileStream.SetMethod("uint8 Put(uint8)", &FileStream::Put);
					VFileStream.SetMethod("uint64 Tell()", &FileStream::Tell);
					VFileStream.SetMethod("uint64 Size()", &FileStream::Size);
					VFileStream.SetMethod("string Filename()", &FileStream::Filename);
					VFileStream.SetMethodEx("bool Open(const string &in, FileMode)", &FileStream_Open);
					VFileStream.SetMethodEx("bool OpenZ(const string &in, FileMode)", &FileStream_OpenZ);
					VFileStream.SetMethodEx("uint64 Write(const string &in)", &FileStream_Write);
					VFileStream.SetMethodEx("string Read(uint64)", &FileStream_Read);

					VFileLogger.SetFunctionDef("bool LogCallback(FileLogger@, const string &in, any@)");
					VFileLogger.SetProperty("FileStream Stream", &FileLogger::Stream);
					VFileLogger.SetProperty("string Path", &FileLogger::Path);
					VFileLogger.SetProperty("string Name", &FileLogger::Name);
					VFileLogger.SetUnmanagedConstructor<FileLogger, const std::string&>("FileLogger@ f(const string &in)");
					VFileLogger.SetMethodEx("void Process(any@, Rest::FileLogger::LogCallback@)", &FileLogger_Process);

					VFileTree.SetFunctionDef("bool TreeCallback(FileTree@, any@)");
					VFileTree.SetProperty("string Path", &FileTree::Path);
					VFileTree.SetUnmanagedConstructor<FileTree, const std::string&>("FileTree@ f(const string &in)");
					VFileTree.SetMethod("uint64 GetFilesCount()", &FileTree::GetFiles);
					VFileTree.SetMethod("FileTree@+ Find(const string &in)", &FileTree::Find);
					VFileTree.SetMethodEx("void Loop(any@, Rest::FileTree::TreeCallback@)", &FileTree_Loop);
					VFileTree.SetMethodEx("array<FileTree@>@ GetDirectories()", &FileTree_GetDirectories);
					VFileTree.SetMethodEx("array<string>@ GetFiles()", &FileTree_GetFiles);

					VEventQueue.SetFunctionDef("bool BaseCallback(EventQueue@+, any@+)");
					VEventQueue.SetUnmanagedConstructor<EventQueue>("EventQueue@ f()");
					VEventQueue.SetMethod("void SetIdleTime(uint64)", &EventQueue::SetIdleTime);
					VEventQueue.SetMethod("void SetState(EventWorkflow)", &EventQueue::SetState);
					VEventQueue.SetMethod("bool Tick()", &EventQueue::Tick);
					VEventQueue.SetMethod("bool Start(EventWorkflow, uint64, uint64)", &EventQueue::Start);
					VEventQueue.SetMethod("bool Stop()", &EventQueue::Stop);
					VEventQueue.SetMethod("bool Expire(uint64)", &EventQueue::Expire);
					VEventQueue.SetMethod("EventState GetState()", &EventQueue::GetState);
					VEventQueue.SetMethodEx("bool Task(any@, Rest::EventQueue::BaseCallback@, bool)", &EventQueue_Task);
					VEventQueue.SetMethodEx("bool Callback(any@, Rest::EventQueue::BaseCallback@, bool)", &EventQueue_Callback);
					VEventQueue.SetMethodEx("bool Interval(any@, uint64, Rest::EventQueue::BaseCallback@)", &EventQueue_Interval);
					VEventQueue.SetMethodEx("bool Timeout(any@, uint64, Rest::EventQueue::BaseCallback@)", &EventQueue_Timeout);

					VDocument.SetProperty<Document>("string Name", &Document::Name);
					VDocument.SetProperty<Document>("string String", &Document::String);
					VDocument.SetProperty<Document>("NodeType Type", &Document::Type);
					VDocument.SetProperty<Document>("int64 Low", &Document::Low);
					VDocument.SetProperty<Document>("int64 Integer", &Document::Integer);
					VDocument.SetProperty<Document>("double Number", &Document::Number);
					VDocument.SetProperty<Document>("bool Boolean", &Document::Boolean);
					VDocument.SetProperty<Document>("bool Saved", &Document::Saved);
					VDocument.SetUnmanagedConstructor<Document>("Document@ f()");
					VDocument.SetMethod("void Join(Document@+)", &Document::Join);
					VDocument.SetMethod("void Clear()", &Document::Clear);
					VDocument.SetMethod("void Save()", &Document::Save);
					VDocument.SetMethod("Document@+ Copy()", &Document::Copy);
					VDocument.SetMethod("bool IsAttribute()", &Document::IsAttribute);
					VDocument.SetMethod("bool IsObject()", &Document::IsObject);
					VDocument.SetMethod("bool Deserialize(const string &in)", &Document::Deserialize);
					VDocument.SetMethod("bool GetBoolean(const string &in)", &Document::GetBoolean);
					VDocument.SetMethod("bool GetNull(const string &in)", &Document::GetNull);
					VDocument.SetMethod("bool GetUndefined(const string &in)", &Document::GetUndefined);
					VDocument.SetMethod("uint64 Size()", &Document::Size);
					VDocument.SetMethod("uint64 GetInteger(const string &in)", &Document::GetInteger);
					VDocument.SetMethod("double GetNumber(const string &in)", &Document::GetNumber);
					VDocument.SetMethod("string GetString(const string &in)", &Document::GetStringBlob);
					VDocument.SetMethod("string GetName()", &Document::GetName);
					VDocument.SetMethod("string Serialize()", &Document::Serialize);
					VDocument.SetMethod("Document@+ GetIndex(uint64)", &Document::GetIndex);
					VDocument.SetMethod("Document@+ Get(const string &in)", &Document::Get);
					VDocument.SetMethod("Document@+ SetCast(const string &in, const string &in)", &Document::SetCast);
					VDocument.SetMethod("Document@+ SetUndefined(const string &in)", &Document::SetUndefined);
					VDocument.SetMethod("Document@+ SetNull(const string &in)", &Document::SetNull);
					VDocument.SetMethod("Document@+ SetAttribute(const string &in, const string &in)", &Document::SetAttribute);
					VDocument.SetMethod("Document@+ SetInteger(const string &in, int64)", &Document::SetInteger);
					VDocument.SetMethod("Document@+ SetNumber(const string &in, double)", &Document::SetNumber);
					VDocument.SetMethod("Document@+ SetBoolean(const string &in, bool)", &Document::SetBoolean);
					VDocument.SetMethod("Document@+ Find(const string &in, bool)", &Document::Find);
					VDocument.SetMethod("Document@+ FindPath(const string &in, bool)", &Document::FindPath);
					VDocument.SetMethod("Document@+ GetParent()", &Document::GetParent);
					VDocument.SetMethod("Document@+ GetAttribute(const string &in)", &Document::GetAttribute);
					VDocument.SetMethod<Document, Document*, const std::string&, Document*>("Document@+ SetDocument(const string &in, Document@+)", &Document::SetDocument);
					VDocument.SetMethod<Document, Document*, const std::string&>("Document@+ SetDocument(const string &in)", &Document::SetDocument);
					VDocument.SetMethod<Document, Document*, const std::string&, Document*>("Document@+ SetArray(const string &in, Document@+)", &Document::SetArray);
					VDocument.SetMethod<Document, Document*, const std::string&>("Document@+ SetArray(const string &in)", &Document::SetArray);
					VDocument.SetMethod<Document, Document*, const std::string&, int64_t, int64_t>("Document@+ SetDecimal(const string &in, int64, int64)", &Document::SetDecimal);
					VDocument.SetMethod<Document, Document*, const std::string&, const std::string&>("Document@+ SetDecimal(const string &in, const string &in)", &Document::SetDecimal);
					VDocument.SetMethod<Document, Document*, const std::string&, const std::string&>("Document@+ SetString(const string &in, const string &in)", &Document::SetString);
					VDocument.SetMethodEx("Document@ SetId(const string &in, const string &in)", &Document_SetId);
					VDocument.SetMethodEx("string GetDecimal(const string &in)", &Document_GetDecimal);
					VDocument.SetMethodEx("string GetId(const string &in)", &Document_GetId);
					VDocument.SetMethodEx("array<Document@>@ FindCollection(const string &in, bool)", &Document_FindCollection);
					VDocument.SetMethodEx("array<Document@>@ FindCollectionPath(const string &in, bool)", &Document_FindCollectionPath);
					VDocument.SetMethodEx("array<Document@>@ GetAttributes()", &Document_GetAttributes);
					VDocument.SetMethodEx("array<Document@>@ GetNodes()", &Document_GetNodes);
					VDocument.SetMethodEx("dictionary@ CreateMapping()", &Document_CreateMapping);
					VDocument.SetMethodEx("string ToJSON()", &Document_ToJSON);
					VDocument.SetMethodEx("string ToXML()", &Document_ToXML);
					VDocument.SetMethodStatic("Document@ FromJSON(const string &in)", &Document_FromJSON);
					VDocument.SetMethodStatic("Document@ FromXML(const string &in)", &Document_FromXML);

					VOS.SetConstructor<OS>("void f()");
					VOS.SetMethodStatic("bool WantTextInput(const string &in, const string &in, const string &in, string &out)", &OS::WantTextInput);
					VOS.SetMethodStatic("bool WantPasswordInput(const string &in, const string &in, string &out)", &OS::WantPasswordInput);
					VOS.SetMethodStatic("bool WantFileSave(const string &in, const string &in, const string &in, const string &in, string &out)", &OS::WantFileSave);
					VOS.SetMethodStatic("bool WantFileOpen(const string &in, const string &in, const string &in, const string &in, bool, string &out)", &OS::WantFileOpen);
					VOS.SetMethodStatic("bool WantFolder(const string &in, const string &in, string &out)", &OS::WantFolder);
					VOS.SetMethodStatic("bool WantColor(const string &in, const string &in, string &out)", &OS::WantColor);
					VOS.SetMethodStatic("int GetError()", &OS::GetError);
					VOS.SetMethodStatic("string GetErrorName(int)", &OS::GetErrorName);
					VOS.SetMethodStatic("string GetDirectory()", &OS::GetDirectory);
					VOS.SetMethodStatic("string FileDirectory(const string &in, int = 0)", &OS::FileDirectory);
					VOS.SetMethodStatic("string Resolve(const string &in)", &OS_Resolve_1);
					VOS.SetMethodStatic("string Resolve(const string &in, const string& in)", &OS_Resolve_2);
					VOS.SetMethodStatic("string ResolveDir(const string &in)", &OS_ResolveDir_1);
					VOS.SetMethodStatic("string ResolveDir(const string &in, const string& in)", &OS_ResolveDir_2);
					VOS.SetMethodStatic("string Read(const string &in)", &OS_Read);
					VOS.SetMethodStatic("string FileExtention(const string &in)", &OS_FileExtention);
					VOS.SetMethodStatic("FileState GetState(const string &in)", &OS_GetState);
					VOS.SetMethodStatic("void Run(const string &in)", &OS_Run);
					VOS.SetMethodStatic("void SetDirectory(const string &in)", &OS_SetDirectory);
					VOS.SetMethodStatic("bool FileExists(const string &in)", &OS_FileExists);
					VOS.SetMethodStatic("bool ExecExists(const string &in)", &OS_ExecExists);
					VOS.SetMethodStatic("bool DirExists(const string &in)", &OS_DirExists);
					VOS.SetMethodStatic("bool CreateDir(const string &in)", &OS_CreateDir);
					VOS.SetMethodStatic("bool Write(const string &in, const string &in)", &OS_Write);
					VOS.SetMethodStatic("bool RemoveFile(const string &in)", &OS_RemoveFile);
					VOS.SetMethodStatic("bool RemoveDir(const string &in)", &OS_RemoveDir);
					VOS.SetMethodStatic("bool Move(const string &in, const string &in)", &OS_Move);
					VOS.SetMethodStatic("bool StateResource(const string &in, Resource &out)", &OS_StateResource);
					VOS.SetMethodStatic("bool FreeProcess(const ChildProcess &in)", &OS_FreeProcess);
					VOS.SetMethodStatic("bool AwaitProcess(const ChildProcess &in, int &out)", &OS_AwaitProcess);
					VOS.SetMethodStatic("ChildProcess SpawnProcess(const string &in, array<string>@+)", &OS_SpawnProcess);
					VOS.SetMethodStatic("array<ResourceEntry>@ ScanFiles(const string &in)", &OS_ScanFiles);
					VOS.SetMethodStatic("array<DirectoryEntry>@ ScanDir(const string &in)", &OS_ScanDir);
					VOS.SetMethodStatic("array<string>@ GetDiskDrives()", &OS_GetDiskDrives);

					Global->SetFunction("string Form(const string &in, const Format &in)", &Stroke_Form);
					return 0;
				});
			}
			void EnableCompute(VMManager* Manager)
			{
				Manager->Namespace("Compute", [](VMGlobal* Global)
				{
					VMWEnum VHybi10Opcode = Global->SetEnum("Hybi10Opcode");
					VMWEnum VShape = Global->SetEnum("Shape");
					VMWEnum VMotionState = Global->SetEnum("MotionState");
					VMWEnum VRegexState = Global->SetEnum("RegexState");
					VMWEnum VRegexFlags = Global->SetEnum("RegexFlags");
					VMWEnum VSoftFeature = Global->SetEnum("SoftFeature");
					VMWEnum VSoftAeroModel = Global->SetEnum("SoftAeroModel");
					VMWEnum VSoftCollision = Global->SetEnum("SoftCollision");
					VMWTypeClass VIncludeDesc = Global->SetStructUnmanaged<IncludeDesc>("IncludeDesc");
					VMWTypeClass VIncludeResult = Global->SetStructUnmanaged<IncludeResult>("IncludeResult");
					VMWTypeClass VVertex = Global->SetPod<Vertex>("Vertex");
					VMWTypeClass VSkinVertex = Global->SetPod<SkinVertex>("SkinVertex");
					VMWTypeClass VShapeVertex = Global->SetPod<ShapeVertex>("ShapeVertex");
					VMWTypeClass VElementVertex = Global->SetPod<ElementVertex>("ElementVertex");
					VMWTypeClass VVector2 = Global->SetStructUnmanaged<Vector2>("Vector2");
					VMWTypeClass VVector3 = Global->SetStructUnmanaged<Vector3>("Vector3");
					VMWTypeClass VVector4 = Global->SetStructUnmanaged<Vector4>("Vector4");
					VMWTypeClass VMatrix4x4 = Global->SetStructUnmanaged<Matrix4x4>("Matrix4x4");
					VMWTypeClass VRay = Global->SetStructUnmanaged<Ray>("Ray");
					VMWTypeClass VQuaternion = Global->SetStructUnmanaged<Quaternion>("Quaternion");
					VMWTypeClass VAnimatorKey = Global->SetStructUnmanaged<AnimatorKey>("AnimatorKey");
					VMWTypeClass VSkinAnimatorClip = Global->SetStructUnmanaged<SkinAnimatorClip>("SkinAnimatorClip");
					VMWTypeClass VKeyAnimatorClip = Global->SetStructUnmanaged<KeyAnimatorClip>("KeyAnimatorClip");
					VMWTypeClass VJoint = Global->SetStructUnmanaged<Joint>("Joint");
					VMWTypeClass VRandomVector2 = Global->SetStructUnmanaged<RandomVector2>("RandomVector2");
					VMWTypeClass VRandomVector3 = Global->SetStructUnmanaged<RandomVector3>("RandomVector3");
					VMWTypeClass VRandomVector4 = Global->SetStructUnmanaged<RandomVector4>("RandomVector4");
					VMWTypeClass VRandomFloat = Global->SetStructUnmanaged<RandomFloat>("RandomFloat");
					VMWTypeClass VHybi10PayloadHeader = Global->SetStructUnmanaged<Hybi10PayloadHeader>("Hybi10PayloadHeader");
					VMWTypeClass VHybi10Request = Global->SetStructUnmanaged<Hybi10Request>("Hybi10Request");
					VMWTypeClass VShapeContact = Global->SetStructUnmanaged<ShapeContact>("ShapeContact");
					VMWTypeClass VRayContact = Global->SetStructUnmanaged<RayContact>("RayContact");
					VMWTypeClass VRegexBracket = Global->SetStructUnmanaged<RegexBracket>("RegexBracket");
					VMWTypeClass VRegexBranch = Global->SetStructUnmanaged<RegexBranch>("RegexBranch");
					VMWTypeClass VRegexMatch = Global->SetStructUnmanaged<RegexMatch>("RegexMatch");
					VMWTypeClass VRegExp = Global->SetStructUnmanaged<RegExp>("RegExp");
					VMWTypeClass VRegexResult = Global->SetStructUnmanaged<RegexResult>("RegexResult");
					VMWTypeClass VMD5Hasher = Global->SetStructUnmanaged<MD5Hasher>("MD5Hasher");
					VMWTypeClass VPreprocessorDesc = Global->SetPod<Preprocessor::Desc>("PreprocessorDesc");
					VMWRefClass VPreprocessor = Global->SetClassUnmanaged<Preprocessor>("Preprocessor");
					VMWRefClass VTransform = Global->SetClassUnmanaged<Transform>("Transform");
					VMWTypeClass VRigidBodyDesc = Global->SetPod<RigidBody::Desc>("RigidBodyDesc");
					VMWRefClass VRigidBody = Global->SetClassUnmanaged<RigidBody>("RigidBody");
					VMWTypeClass VSoftBodyConfig = Global->SetPod<SoftBody::Desc::SConfig>("SoftBodyConfig");
					VMWTypeClass VSoftBodyDesc = Global->SetStructUnmanaged<SoftBody::Desc>("SoftBodyDesc");
					VMWTypeClass VSoftBodyRayCast = Global->SetStructUnmanaged<SoftBody::RayCast>("SoftBodyRayCast");
					VMWRefClass VSoftBody = Global->SetClassUnmanaged<SoftBody>("SoftBody");
					VMWRefClass VSliderConstraint = Global->SetClassUnmanaged<SliderConstraint>("SliderConstraint");
					VMWTypeClass VSliderConstraintDesc = Global->SetPod<SliderConstraint::Desc>("SliderConstraintDesc");
					VMWTypeClass VSimulatorDesc = Global->SetPod<Simulator::Desc>("SimulatorDesc");
					VMWRefClass VSimulator = Global->SetClassUnmanaged<Simulator>("Simulator");

					VHybi10Opcode.SetValue("Text", Hybi10_Opcode_Text);
					VHybi10Opcode.SetValue("Close", Hybi10_Opcode_Close);
					VHybi10Opcode.SetValue("Pong", Hybi10_Opcode_Pong);
					VHybi10Opcode.SetValue("Ping", Hybi10_Opcode_Ping);
					VHybi10Opcode.SetValue("Invalid", Hybi10_Opcode_Invalid);
		
					VShape.SetValue("Box", Shape_Box);
					VShape.SetValue("Triangle", Shape_Triangle);
					VShape.SetValue("Tetrahedral", Shape_Tetrahedral);
					VShape.SetValue("ConvexTriangleMesh", Shape_Convex_Triangle_Mesh);
					VShape.SetValue("ConvexHull", Shape_Convex_Hull);
					VShape.SetValue("ConvexPointCloud", Shape_Convex_Point_Cloud);
					VShape.SetValue("ConvexPolyhedral", Shape_Convex_Polyhedral);
					VShape.SetValue("ImplicitConvexStart", Shape_Implicit_Convex_Start);
					VShape.SetValue("Sphere", Shape_Sphere);
					VShape.SetValue("MultiSphere", Shape_Multi_Sphere);
					VShape.SetValue("Capsule", Shape_Capsule);
					VShape.SetValue("Cone", Shape_Cone);
					VShape.SetValue("Convex", Shape_Convex);
					VShape.SetValue("Cylinder", Shape_Cylinder);
					VShape.SetValue("UniformScaling", Shape_Uniform_Scaling);
					VShape.SetValue("MinkowskiSum", Shape_Minkowski_Sum);
					VShape.SetValue("MinkowskiDifference", Shape_Minkowski_Difference);
					VShape.SetValue("Box2D", Shape_Box_2D);
					VShape.SetValue("Convex2D", Shape_Convex_2D);
					VShape.SetValue("CustomConvex", Shape_Custom_Convex);
					VShape.SetValue("ConcavesStart", Shape_Concaves_Start);
					VShape.SetValue("TriangleMesh", Shape_Triangle_Mesh);
					VShape.SetValue("TriangleMeshScaled", Shape_Triangle_Mesh_Scaled);
					VShape.SetValue("FastConcaveMesh", Shape_Fast_Concave_Mesh);
					VShape.SetValue("Terrain", Shape_Terrain);
					VShape.SetValue("Gimpact", Shape_Gimpact);
					VShape.SetValue("TriangleMeshMultimaterial", Shape_Triangle_Mesh_Multimaterial);
					VShape.SetValue("Empty", Shape_Empty);
					VShape.SetValue("StaticPlane", Shape_Static_Plane);
					VShape.SetValue("CustomConcave", Shape_Custom_Concave);
					VShape.SetValue("ConcavesEnd", Shape_Concaves_End);
					VShape.SetValue("Compound", Shape_Compound);
					VShape.SetValue("Softbody", Shape_Softbody);
					VShape.SetValue("HFFluid", Shape_HF_Fluid);
					VShape.SetValue("HFFluidBouyantConvex", Shape_HF_Fluid_Bouyant_Convex);
					VShape.SetValue("Invalid", Shape_Invalid);
					VShape.SetValue("Count", Shape_Count);

					VMotionState.SetValue("Active", MotionState_Active);
					VMotionState.SetValue("IslandSleeping", MotionState_Island_Sleeping);
					VMotionState.SetValue("DeactivationNeeded", MotionState_Deactivation_Needed);
					VMotionState.SetValue("DisableDeactivation", MotionState_Disable_Deactivation);
					VMotionState.SetValue("DisableSimulation", MotionState_Disable_Simulation);
			
					VRegexState.SetValue("MatchFound", RegexState_Match_Found);
					VRegexState.SetValue("NoMatch", RegexState_No_Match);
					VRegexState.SetValue("UnexpectedQuantifier", RegexState_Unexpected_Quantifier);
					VRegexState.SetValue("UnbalancedBrackets", RegexState_Unbalanced_Brackets);
					VRegexState.SetValue("InternalError", RegexState_Internal_Error);
					VRegexState.SetValue("InvalidCharacterSet", RegexState_Invalid_Character_Set);
					VRegexState.SetValue("InvalidMetacharacter", RegexState_Invalid_Metacharacter);
					VRegexState.SetValue("SumatchArrayTooSmall", RegexState_Sumatch_Array_Too_Small);
					VRegexState.SetValue("TooManyBranches", RegexState_Too_Many_Branches);
					VRegexState.SetValue("TooManyBrackets", RegexState_Too_Many_Brackets);
				
					VRegexFlags.SetValue("None", RegexFlags_None);
					VRegexFlags.SetValue("IgnoreCase", RegexFlags_IgnoreCase);
			
					VSoftFeature.SetValue("None", SoftFeature_None);
					VSoftFeature.SetValue("Node", SoftFeature_Node);
					VSoftFeature.SetValue("Link", SoftFeature_Link);
					VSoftFeature.SetValue("Face", SoftFeature_Face);
					VSoftFeature.SetValue("Tetra", SoftFeature_Tetra);
		
					VSoftAeroModel.SetValue("VPoint", SoftAeroModel_VPoint);
					VSoftAeroModel.SetValue("VTwoSided", SoftAeroModel_VTwoSided);
					VSoftAeroModel.SetValue("VTwoSidedLiftDrag", SoftAeroModel_VTwoSidedLiftDrag);
					VSoftAeroModel.SetValue("VOneSided", SoftAeroModel_VOneSided);
					VSoftAeroModel.SetValue("FTwoSided", SoftAeroModel_FTwoSided);
					VSoftAeroModel.SetValue("FTwoSidedLiftDrag", SoftAeroModel_FTwoSidedLiftDrag);
					VSoftAeroModel.SetValue("FOneSided", SoftAeroModel_FOneSided);
			
					VSoftCollision.SetValue("RVSMask", SoftCollision_RVS_Mask);
					VSoftCollision.SetValue("SDFRS", SoftCollision_SDF_RS);
					VSoftCollision.SetValue("CLRS", SoftCollision_CL_RS);
					VSoftCollision.SetValue("SDFRD", SoftCollision_SDF_RD);
					VSoftCollision.SetValue("SDFRDF", SoftCollision_SDF_RDF);
					VSoftCollision.SetValue("SVSMask", SoftCollision_SVS_Mask);
					VSoftCollision.SetValue("VFSS", SoftCollision_VF_SS);
					VSoftCollision.SetValue("CLSS", SoftCollision_CL_SS);
					VSoftCollision.SetValue("CLSelf", SoftCollision_CL_Self);
					VSoftCollision.SetValue("VFDD", SoftCollision_VF_DD);
					VSoftCollision.SetValue("Default", SoftCollision_Default);
		
					VIncludeDesc.SetProperty("string From", &IncludeDesc::From);
					VIncludeDesc.SetProperty("string Path", &IncludeDesc::Path);
					VIncludeDesc.SetProperty("string Root", &IncludeDesc::Root);
					VIncludeDesc.SetConstructor<IncludeDesc>("void f()");
					VIncludeDesc.SetMethodEx("void AddExt(const string &in)", &IncludeDesc_AddExt);
					VIncludeDesc.SetMethodEx("void RemoveExt(const string &in)", &IncludeDesc_RemoveExt);
					VIncludeDesc.SetMethodEx("string GetExt(uint)", &IncludeDesc_GetExt);
					VIncludeDesc.SetMethodEx("uint GetExtsCount()", &IncludeDesc_GetExtsCount);
	
					VIncludeResult.SetProperty("string Module", &IncludeResult::Module);
					VIncludeResult.SetProperty("bool IsSystem", &IncludeResult::IsSystem);
					VIncludeResult.SetProperty("bool IsFile", &IncludeResult::IsFile);
					VIncludeResult.SetConstructor<IncludeResult>("void f()");
	
					VVertex.SetProperty("float PositionX", &Vertex::PositionX);
					VVertex.SetProperty("float PositionY", &Vertex::PositionY);
					VVertex.SetProperty("float PositionZ", &Vertex::PositionZ);
					VVertex.SetProperty("float TexCoordX", &Vertex::TexCoordX);
					VVertex.SetProperty("float TexCoordY", &Vertex::TexCoordY);
					VVertex.SetProperty("float NormalX", &Vertex::NormalX);
					VVertex.SetProperty("float NormalY", &Vertex::NormalY);
					VVertex.SetProperty("float NormalZ", &Vertex::NormalZ);
					VVertex.SetProperty("float TangentX", &Vertex::TangentX);
					VVertex.SetProperty("float TangentY", &Vertex::TangentY);
					VVertex.SetProperty("float TangentZ", &Vertex::TangentZ);
					VVertex.SetProperty("float BitangentX", &Vertex::BitangentX);
					VVertex.SetProperty("float BitangentY", &Vertex::BitangentY);
					VVertex.SetProperty("float BitangentZ", &Vertex::BitangentZ);
	
					VSkinVertex.SetProperty("float PositionX", &SkinVertex::PositionX);
					VSkinVertex.SetProperty("float PositionY", &SkinVertex::PositionY);
					VSkinVertex.SetProperty("float PositionZ", &SkinVertex::PositionZ);
					VSkinVertex.SetProperty("float TexCoordX", &SkinVertex::TexCoordX);
					VSkinVertex.SetProperty("float TexCoordY", &SkinVertex::TexCoordY);
					VSkinVertex.SetProperty("float NormalX", &SkinVertex::NormalX);
					VSkinVertex.SetProperty("float NormalY", &SkinVertex::NormalY);
					VSkinVertex.SetProperty("float NormalZ", &SkinVertex::NormalZ);
					VSkinVertex.SetProperty("float TangentX", &SkinVertex::TangentX);
					VSkinVertex.SetProperty("float TangentY", &SkinVertex::TangentY);
					VSkinVertex.SetProperty("float TangentZ", &SkinVertex::TangentZ);
					VSkinVertex.SetProperty("float BitangentX", &SkinVertex::BitangentX);
					VSkinVertex.SetProperty("float BitangentY", &SkinVertex::BitangentY);
					VSkinVertex.SetProperty("float BitangentZ", &SkinVertex::BitangentZ);
					VSkinVertex.SetProperty("float JointIndex0", &SkinVertex::JointIndex0);
					VSkinVertex.SetProperty("float JointIndex1", &SkinVertex::JointIndex1);
					VSkinVertex.SetProperty("float JointIndex2", &SkinVertex::JointIndex2);
					VSkinVertex.SetProperty("float JointIndex3", &SkinVertex::JointIndex3);
					VSkinVertex.SetProperty("float JointBias0", &SkinVertex::JointBias0);
					VSkinVertex.SetProperty("float JointBias1", &SkinVertex::JointBias1);
					VSkinVertex.SetProperty("float JointBias2", &SkinVertex::JointBias2);
					VSkinVertex.SetProperty("float JointBias3", &SkinVertex::JointBias3);
				
					VShapeVertex.SetProperty("float PositionX", &ShapeVertex::PositionX);
					VShapeVertex.SetProperty("float PositionY", &ShapeVertex::PositionY);
					VShapeVertex.SetProperty("float PositionZ", &ShapeVertex::PositionZ);
					VShapeVertex.SetProperty("float TexCoordX", &ShapeVertex::TexCoordX);
					VShapeVertex.SetProperty("float TexCoordY", &ShapeVertex::TexCoordY);
	
					VElementVertex.SetProperty("float PositionX", &ElementVertex::PositionX);
					VElementVertex.SetProperty("float PositionY", &ElementVertex::PositionY);
					VElementVertex.SetProperty("float PositionZ", &ElementVertex::PositionZ);
					VElementVertex.SetProperty("float VelocityX", &ElementVertex::VelocityX);
					VElementVertex.SetProperty("float VelocityY", &ElementVertex::VelocityY);
					VElementVertex.SetProperty("float VelocityZ", &ElementVertex::VelocityZ);
					VElementVertex.SetProperty("float ColorX", &ElementVertex::ColorX);
					VElementVertex.SetProperty("float ColorY", &ElementVertex::ColorY);
					VElementVertex.SetProperty("float ColorZ", &ElementVertex::ColorZ);
					VElementVertex.SetProperty("float ColorW", &ElementVertex::ColorW);
					VElementVertex.SetProperty("float Scale", &ElementVertex::Scale);
					VElementVertex.SetProperty("float Rotation", &ElementVertex::Rotation);
					VElementVertex.SetProperty("float Angular", &ElementVertex::Angular);

					VVector2.SetProperty("float X", &Vector2::X);
					VVector2.SetProperty("float Y", &Vector2::Y);
					VVector2.SetConstructor<Vector2>("void f()");
					VVector2.SetConstructor<Vector2, const Vector3&>("void f(const Vector3 &in)");
					VVector2.SetConstructor<Vector2, const Vector4&>("void f(const Vector4 &in)");
					VVector2.SetConstructor<Vector2, float>("void f(float)");
					VVector2.SetConstructor<Vector2, float, float>("void f(float, float)");
					VVector2.SetMethod("float Hypotenuse() const", &Vector2::Hypotenuse);
					VVector2.SetMethod("float DotProduct(const Vector2 &in) const", &Vector2::DotProduct);
					VVector2.SetMethod("float Distance(const Vector2 &in) const", &Vector2::Distance);
					VVector2.SetMethod("float Length() const", &Vector2::Length);
					VVector2.SetMethod("float Cross(const Vector2 &in) const", &Vector2::Cross);
					VVector2.SetMethod("float LookAt(const Vector2 &in) const", &Vector2::LookAt);
					VVector2.SetMethod("float ModLength() const", &Vector2::ModLength);
					VVector2.SetMethod("Vector2 Direction(float) const", &Vector2::Direction);
					VVector2.SetMethod("Vector2 Invert() const", &Vector2::Invert);
					VVector2.SetMethod("Vector2 InvertX() const", &Vector2::InvertX);
					VVector2.SetMethod("Vector2 InvertY() const", &Vector2::InvertY);
					VVector2.SetMethod("Vector2 Normalize() const", &Vector2::Normalize);
					VVector2.SetMethod("Vector2 NormalizeSafe() const", &Vector2::NormalizeSafe);
					VVector2.SetMethod("Vector2 SetX(float) const", &Vector2::SetX);
					VVector2.SetMethod("Vector2 SetY(float) const", &Vector2::SetY);
					VVector2.SetMethod<Vector2, Vector2, const Vector2&>("Vector2 Mul(const Vector2 &in) const", &Vector2::Mul);
					VVector2.SetMethod("Vector2 Add(const Vector2 &in) const", &Vector2::Add);
					VVector2.SetMethod("Vector2 Div(const Vector2 &in) const", &Vector2::Div);
					VVector2.SetMethod("Vector2 Lerp(const Vector2 &in, float) const", &Vector2::Lerp);
					VVector2.SetMethod("Vector2 SphericalLerp(const Vector2 &in, float) const", &Vector2::SphericalLerp);
					VVector2.SetMethod("Vector2 AngularLerp(const Vector2 &in, float) const", &Vector2::AngularLerp);
					VVector2.SetMethod("Vector2 SaturateRotation() const", &Vector2::SaturateRotation);
					VVector2.SetMethod("Vector2 Transform(const Matrix4x4 &in) const", &Vector2::Transform);
					VVector2.SetMethod("Vector2 Abs() const", &Vector2::Abs);
					VVector2.SetMethod("Vector2 Radians() const", &Vector2::Radians);
					VVector2.SetMethod("Vector2 Degrees() const", &Vector2::Degrees);
					VVector2.SetMethod("Vector2 Degrees() const", &Vector2::Degrees);
					VVector2.SetMethod("Vector2 XY() const", &Vector2::XY);
					VVector2.SetMethod("Vector3 XYZ() const", &Vector2::XYZ);
					VVector2.SetMethod("Vector4 XYZW() const", &Vector2::XYZW);
					VVector2.SetMethod("void Set(const Vector2 &in)", &Vector2::Set);
					VVector2.SetMethodStatic("Vector2 Random()", &Vector2::Random);
					VVector2.SetMethodStatic("Vector2 RandomAbs()", &Vector2::RandomAbs);
					VVector2.SetMethodStatic("Vector2 One()", &Vector2::One);
					VVector2.SetMethodStatic("Vector2 Zero()", &Vector2::Zero);
					VVector2.SetMethodStatic("Vector2 Up()", &Vector2::Up);
					VVector2.SetMethodStatic("Vector2 Down()", &Vector2::Down);
					VVector2.SetMethodStatic("Vector2 Left()", &Vector2::Left);
					VVector2.SetMethodStatic("Vector2 Right()", &Vector2::Right);
					VVector2.SetLCOperatorEx(VMOpFunc_Mul, "Vector2", "const Vector2& in", &Vector2_Mul_1);
					VVector2.SetLCOperatorEx(VMOpFunc_Mul, "Vector2", "float", &Vector2_Mul_2);
					VVector2.SetLCOperatorEx(VMOpFunc_Div, "Vector2", "const Vector2& in", &Vector2_Div_1);
					VVector2.SetLCOperatorEx(VMOpFunc_Div, "Vector2", "float", &Vector2_Div_2);
					VVector2.SetLCOperatorEx(VMOpFunc_Add, "Vector2", "const Vector2& in", &Vector2_Add_1);
					VVector2.SetLCOperatorEx(VMOpFunc_Add, "Vector2", "float", &Vector2_Add_2);
					VVector2.SetLCOperatorEx(VMOpFunc_Sub, "Vector2", "const Vector2& in", &Vector2_Sub_1);
					VVector2.SetLCOperatorEx(VMOpFunc_Sub, "Vector2", "float", &Vector2_Sub_2);
					VVector2.SetLCOperatorEx(VMOpFunc_Neg, "Vector2", "", &Vector2_Neg);
					VVector2.SetLCOperatorEx(VMOpFunc_Equals, "bool", "const Vector2 &in", &Vector2_Equals);
					VVector2.SetLCOperatorEx(VMOpFunc_Cmp, "int", "const Vector2 &in", &Vector2_Cmp);
					VVector2.SetLCOperatorEx(VMOpFunc_Index, "float", "int", &Vector2_Index_1);
					VVector2.SetLOperatorEx(VMOpFunc_Index, "float&", "int", &Vector2_Index_2);
					VVector2.SetLOperatorEx(VMOpFunc_MulAssign, "Vector2&", "const Vector2& in", &Vector2_MulAssign_1);
					VVector2.SetLOperatorEx(VMOpFunc_MulAssign, "Vector2&", "float", &Vector2_MulAssign_2);
					VVector2.SetLOperatorEx(VMOpFunc_DivAssign, "Vector2&", "const Vector2& in", &Vector2_DivAssign_1);
					VVector2.SetLOperatorEx(VMOpFunc_DivAssign, "Vector2&", "float", &Vector2_DivAssign_2);
					VVector2.SetLOperatorEx(VMOpFunc_AddAssign, "Vector2&", "const Vector2& in", &Vector2_AddAssign_1);
					VVector2.SetLOperatorEx(VMOpFunc_AddAssign, "Vector2&", "float", &Vector2_AddAssign_2);
					VVector2.SetLOperatorEx(VMOpFunc_SubAssign, "Vector2&", "const Vector2& in", &Vector2_SubAssign_1);
					VVector2.SetLOperatorEx(VMOpFunc_SubAssign, "Vector2&", "float", &Vector2_SubAssign_2);
	
					VVector3.SetProperty("float X", &Vector3::X);
					VVector3.SetProperty("float Y", &Vector3::Y);
					VVector3.SetProperty("float Z", &Vector3::Z);
					VVector3.SetConstructor<Vector3>("void f()");
					VVector3.SetConstructor<Vector3, const Vector2&>("void f(const Vector2 &in)");
					VVector3.SetConstructor<Vector3, const Vector4&>("void f(const Vector4 &in)");
					VVector3.SetConstructor<Vector3, float>("void f(float)");
					VVector3.SetConstructor<Vector3, float, float>("void f(float, float)");
					VVector3.SetConstructor<Vector3, float, float, float>("void f(float, float, float)");
					VVector3.SetMethod("float Hypotenuse() const", &Vector3::Hypotenuse);
					VVector3.SetMethod("float DotProduct(const Vector3 &in) const", &Vector3::DotProduct);
					VVector3.SetMethod("float Distance(const Vector3 &in) const", &Vector3::Distance);
					VVector3.SetMethod("float Length() const", &Vector3::Length);
					VVector3.SetMethod("float ModLength() const", &Vector3::ModLength);
					VVector3.SetMethod("float LookAtXY(const Vector3 &in) const", &Vector3::LookAtXY);
					VVector3.SetMethod("float LookAtXZ(const Vector3 &in) const", &Vector3::LookAtXZ);
					VVector3.SetMethod("Vector3 Invert() const", &Vector3::Invert);
					VVector3.SetMethod("Vector3 InvertX() const", &Vector3::InvertX);
					VVector3.SetMethod("Vector3 InvertY() const", &Vector3::InvertY);
					VVector3.SetMethod("Vector3 InvertZ() const", &Vector3::InvertZ);
					VVector3.SetMethod("Vector3 Cross(const Vector3 &in) const", &Vector3::Cross);
					VVector3.SetMethod("Vector3 Normalize() const", &Vector3::Normalize);
					VVector3.SetMethod("Vector3 NormalizeSafe() const", &Vector3::NormalizeSafe);
					VVector3.SetMethod("Vector3 SetX(float) const", &Vector3::SetX);
					VVector3.SetMethod("Vector3 SetY(float) const", &Vector3::SetY);
					VVector3.SetMethod("Vector3 SetZ(float) const", &Vector3::SetZ);
					VVector3.SetMethod<Vector3, Vector3, const Vector3&>("Vector3 Mul(const Vector3 &in) const", &Vector3::Mul);
					VVector3.SetMethod("Vector3 Add(const Vector3 &in) const", &Vector3::Add);
					VVector3.SetMethod("Vector3 Div(const Vector3 &in) const", &Vector3::Div);
					VVector3.SetMethod("Vector3 Lerp(const Vector3 &in, float) const", &Vector3::Lerp);
					VVector3.SetMethod("Vector3 SphericalLerp(const Vector3 &in, float) const", &Vector3::SphericalLerp);
					VVector3.SetMethod("Vector3 AngularLerp(const Vector3 &in, float) const", &Vector3::AngularLerp);
					VVector3.SetMethod("Vector3 SaturateRotation() const", &Vector3::SaturateRotation);
					VVector3.SetMethod("Vector3 HorizontalDirection() const", &Vector3::HorizontalDirection);
					VVector3.SetMethod("Vector3 DepthDirection() const", &Vector3::DepthDirection);
					VVector3.SetMethod("Vector3 Transform(const Matrix4x4 &in) const", &Vector3::Transform);
					VVector3.SetMethod("Vector3 Abs() const", &Vector3::Abs);
					VVector3.SetMethod("Vector3 Radians() const", &Vector3::Radians);
					VVector3.SetMethod("Vector3 Degrees() const", &Vector3::Degrees);
					VVector3.SetMethod("Vector2 XY() const", &Vector3::XY);
					VVector3.SetMethod("Vector3 XYZ() const", &Vector3::XYZ);
					VVector3.SetMethod("Vector4 XYZW() const", &Vector3::XYZW);
					VVector3.SetMethod("void Set(const Vector3 &in)", &Vector3::Set);
					VVector3.SetMethodStatic("Vector3 Random()", &Vector3::Random);
					VVector3.SetMethodStatic("Vector3 RandomAbs()", &Vector3::RandomAbs);
					VVector3.SetMethodStatic("Vector3 One()", &Vector3::One);
					VVector3.SetMethodStatic("Vector3 Zero()", &Vector3::Zero);
					VVector3.SetMethodStatic("Vector3 Up()", &Vector3::Up);
					VVector3.SetMethodStatic("Vector3 Down()", &Vector3::Down);
					VVector3.SetMethodStatic("Vector3 Left()", &Vector3::Left);
					VVector3.SetMethodStatic("Vector3 Right()", &Vector3::Right);
					VVector3.SetMethodStatic("Vector3 Forward()", &Vector3::Forward);
					VVector3.SetMethodStatic("Vector3 Backward()", &Vector3::Backward);
					VVector3.SetLCOperatorEx(VMOpFunc_Mul, "Vector3", "const Vector3& in", &Vector3_Mul_1);
					VVector3.SetLCOperatorEx(VMOpFunc_Mul, "Vector3", "float", &Vector3_Mul_2);
					VVector3.SetLCOperatorEx(VMOpFunc_Div, "Vector3", "const Vector3& in", &Vector3_Div_1);
					VVector3.SetLCOperatorEx(VMOpFunc_Div, "Vector3", "float", &Vector3_Div_2);
					VVector3.SetLCOperatorEx(VMOpFunc_Add, "Vector3", "const Vector3& in", &Vector3_Add_1);
					VVector3.SetLCOperatorEx(VMOpFunc_Add, "Vector3", "float", &Vector3_Add_2);
					VVector3.SetLCOperatorEx(VMOpFunc_Sub, "Vector3", "const Vector3& in", &Vector3_Sub_1);
					VVector3.SetLCOperatorEx(VMOpFunc_Sub, "Vector3", "float", &Vector3_Sub_2);
					VVector3.SetLCOperatorEx(VMOpFunc_Neg, "Vector3", "", &Vector3_Neg);
					VVector3.SetLCOperatorEx(VMOpFunc_Equals, "bool", "const Vector3 &in", &Vector3_Equals);
					VVector3.SetLCOperatorEx(VMOpFunc_Cmp, "int", "const Vector3 &in", &Vector3_Cmp);
					VVector3.SetLCOperatorEx(VMOpFunc_Index, "float", "int", &Vector3_Index_1);
					VVector3.SetLOperatorEx(VMOpFunc_Index, "float&", "int", &Vector3_Index_2);
					VVector3.SetLOperatorEx(VMOpFunc_MulAssign, "Vector3&", "const Vector3& in", &Vector3_MulAssign_1);
					VVector3.SetLOperatorEx(VMOpFunc_MulAssign, "Vector3&", "float", &Vector3_MulAssign_2);
					VVector3.SetLOperatorEx(VMOpFunc_DivAssign, "Vector3&", "const Vector3& in", &Vector3_DivAssign_1);
					VVector3.SetLOperatorEx(VMOpFunc_DivAssign, "Vector3&", "float", &Vector3_DivAssign_2);
					VVector3.SetLOperatorEx(VMOpFunc_AddAssign, "Vector3&", "const Vector3& in", &Vector3_AddAssign_1);
					VVector3.SetLOperatorEx(VMOpFunc_AddAssign, "Vector3&", "float", &Vector3_AddAssign_2);
					VVector3.SetLOperatorEx(VMOpFunc_SubAssign, "Vector3&", "const Vector3& in", &Vector3_SubAssign_1);
					VVector3.SetLOperatorEx(VMOpFunc_SubAssign, "Vector3&", "float", &Vector3_SubAssign_2);
			
					VVector4.SetProperty("float X", &Vector4::X);
					VVector4.SetProperty("float Y", &Vector4::Y);
					VVector4.SetProperty("float Z", &Vector4::Z);
					VVector4.SetProperty("float W", &Vector4::W);
					VVector4.SetConstructor<Vector4>("void f()");
					VVector4.SetConstructor<Vector4, const Vector2&>("void f(const Vector2 &in)");
					VVector4.SetConstructor<Vector4, const Vector3&>("void f(const Vector3 &in)");
					VVector4.SetConstructor<Vector4, float>("void f(float)");
					VVector4.SetConstructor<Vector4, float, float>("void f(float, float)");
					VVector4.SetConstructor<Vector4, float, float, float>("void f(float, float, float)");
					VVector4.SetConstructor<Vector4, float, float, float, float>("void f(float, float, float, float)");
					VVector4.SetMethod("float ModLength() const", &Vector4::ModLength);
					VVector4.SetMethod("float DotProduct(const Vector4 &in) const", &Vector4::DotProduct);
					VVector4.SetMethod("float Distance(const Vector4 &in) const", &Vector4::Distance);
					VVector4.SetMethod("float Length() const", &Vector4::Length);
					VVector4.SetMethod("Vector4 Invert() const", &Vector4::Invert);
					VVector4.SetMethod("Vector4 InvertX() const", &Vector4::InvertX);
					VVector4.SetMethod("Vector4 InvertY() const", &Vector4::InvertY);
					VVector4.SetMethod("Vector4 InvertZ() const", &Vector4::InvertZ);
					VVector4.SetMethod("Vector4 InvertW() const", &Vector4::InvertW);
					VVector4.SetMethod("Vector4 Lerp(const Vector4 &in, float) const", &Vector4::Lerp);
					VVector4.SetMethod("Vector4 SphericalLerp(const Vector4 &in, float) const", &Vector4::SphericalLerp);
					VVector4.SetMethod("Vector4 AngularLerp(const Vector4 &in, float) const", &Vector4::AngularLerp);
					VVector4.SetMethod("Vector4 SaturateRotation() const", &Vector4::SaturateRotation);
					VVector4.SetMethod("Vector4 Cross(const Vector4 &in) const", &Vector4::Cross);
					VVector4.SetMethod("Vector4 Normalize() const", &Vector4::Normalize);
					VVector4.SetMethod("Vector4 NormalizeSafe() const", &Vector4::NormalizeSafe);
					VVector4.SetMethod("Vector4 Transform(const Matrix4x4 &in) const", &Vector4::Transform);
					VVector4.SetMethod("Vector4 SetX(float) const", &Vector4::SetX);
					VVector4.SetMethod("Vector4 SetY(float) const", &Vector4::SetY);
					VVector4.SetMethod("Vector4 SetZ(float) const", &Vector4::SetZ);
					VVector4.SetMethod("Vector4 SetW(float) const", &Vector4::SetW);
					VVector4.SetMethod<Vector4, Vector4, const Vector4&>("Vector4 Mul(const Vector4 &in) const", &Vector4::Mul);
					VVector4.SetMethod("Vector4 Add(const Vector4 &in) const", &Vector4::Add);
					VVector4.SetMethod("Vector4 Div(const Vector4 &in) const", &Vector4::Div);
					VVector4.SetMethod("Vector4 Abs() const", &Vector4::Abs);
					VVector4.SetMethod("Vector4 Radians() const", &Vector4::Radians);
					VVector4.SetMethod("Vector4 Degrees() const", &Vector4::Degrees);
					VVector4.SetMethod("Vector2 XY() const", &Vector4::XY);
					VVector4.SetMethod("Vector3 XYZ() const", &Vector4::XYZ);
					VVector4.SetMethod("Vector4 XYZW() const", &Vector4::XYZW);
					VVector4.SetMethodStatic("Vector4 Random()", &Vector4::Random);
					VVector4.SetMethodStatic("Vector4 RandomAbs()", &Vector4::RandomAbs);
					VVector4.SetMethodStatic("Vector4 One()", &Vector4::One);
					VVector4.SetMethodStatic("Vector4 Zero()", &Vector4::Zero);
					VVector4.SetMethodStatic("Vector4 UnitW()", &Vector4::UnitW);
					VVector4.SetMethodStatic("Vector4 Up()", &Vector4::Up);
					VVector4.SetMethodStatic("Vector4 Down()", &Vector4::Down);
					VVector4.SetMethodStatic("Vector4 Left()", &Vector4::Left);
					VVector4.SetMethodStatic("Vector4 Right()", &Vector4::Right);
					VVector4.SetMethodStatic("Vector4 Forward()", &Vector4::Forward);
					VVector4.SetMethodStatic("Vector4 Backward()", &Vector4::Backward);
					VVector4.SetLCOperatorEx(VMOpFunc_Mul, "Vector4", "const Vector4& in", &Vector4_Mul_1);
					VVector4.SetLCOperatorEx(VMOpFunc_Mul, "Vector4", "float", &Vector4_Mul_2);
					VVector4.SetLCOperatorEx(VMOpFunc_Div, "Vector4", "const Vector4& in", &Vector4_Div_1);
					VVector4.SetLCOperatorEx(VMOpFunc_Div, "Vector4", "float", &Vector4_Div_2);
					VVector4.SetLCOperatorEx(VMOpFunc_Add, "Vector4", "const Vector4& in", &Vector4_Add_1);
					VVector4.SetLCOperatorEx(VMOpFunc_Add, "Vector4", "float", &Vector4_Add_2);
					VVector4.SetLCOperatorEx(VMOpFunc_Sub, "Vector4", "const Vector4& in", &Vector4_Sub_1);
					VVector4.SetLCOperatorEx(VMOpFunc_Sub, "Vector4", "float", &Vector4_Sub_2);
					VVector4.SetLCOperatorEx(VMOpFunc_Neg, "Vector4", "", &Vector4_Neg);
					VVector4.SetLCOperatorEx(VMOpFunc_Equals, "bool", "const Vector4 &in", &Vector4_Equals);
					VVector4.SetLCOperatorEx(VMOpFunc_Cmp, "int", "const Vector4 &in", &Vector4_Cmp);
					VVector4.SetLCOperatorEx(VMOpFunc_Index, "float", "int", &Vector4_Index_1);
					VVector4.SetLOperatorEx(VMOpFunc_Index, "float&", "int", &Vector4_Index_2);
					VVector4.SetLOperatorEx(VMOpFunc_MulAssign, "Vector4&", "const Vector4& in", &Vector4_MulAssign_1);
					VVector4.SetLOperatorEx(VMOpFunc_MulAssign, "Vector4&", "float", &Vector4_MulAssign_2);
					VVector4.SetLOperatorEx(VMOpFunc_DivAssign, "Vector4&", "const Vector4& in", &Vector4_DivAssign_1);
					VVector4.SetLOperatorEx(VMOpFunc_DivAssign, "Vector4&", "float", &Vector4_DivAssign_2);
					VVector4.SetLOperatorEx(VMOpFunc_AddAssign, "Vector4&", "const Vector4& in", &Vector4_AddAssign_1);
					VVector4.SetLOperatorEx(VMOpFunc_AddAssign, "Vector4&", "float", &Vector4_AddAssign_2);
					VVector4.SetLOperatorEx(VMOpFunc_SubAssign, "Vector4&", "const Vector4& in", &Vector4_SubAssign_1);
					VVector4.SetLOperatorEx(VMOpFunc_SubAssign, "Vector4&", "float", &Vector4_SubAssign_2);
				
					VRay.SetProperty("Vector3 Origin", &Ray::Origin);
					VRay.SetProperty("Vector3 Direction", &Ray::Direction);
					VRay.SetConstructor<Ray>("void f()");
					VRay.SetConstructor<Ray, const Vector3&, const Vector3&>("void f(const Vector3 &in, const Vector3 &in)");
					VRay.SetMethod("Vector3 GetPoint(float) const", &Ray::GetPoint);
					VRay.SetMethod("bool IntersectsPlane(const Vector3 &in, float) const", &Ray::IntersectsPlane);
					VRay.SetMethod("bool IntersectsSphere(const Vector3 &in, float, bool) const", &Ray::IntersectsSphere);
					VRay.SetMethod("bool IntersectsAABBAt(const Vector3 &in, const Vector3 &in) const", &Ray::IntersectsAABBAt);
					VRay.SetMethod("bool IntersectsAABB(const Vector3 &in, const Vector3 &in) const", &Ray::IntersectsAABB);
					VRay.SetMethod("bool IntersectsOBB(const Matrix4x4 &in)", &Ray::IntersectsOBB);

					VMatrix4x4.SetConstructor<Matrix4x4>("void f()");
					VMatrix4x4.SetConstructor<Matrix4x4, const Vector4&, const Vector4&, const Vector4&, const Vector4&>("void f(const Vector4 &in, const Vector4 &in, const Vector4 &in, const Vector4 &in)");
					VMatrix4x4.SetConstructor<Matrix4x4, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float>("void f(float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float)");
					VMatrix4x4.SetMethod<Matrix4x4, Matrix4x4, const Matrix4x4&>("Matrix4x4 Mul(const Matrix4x4 &in) const", &Matrix4x4::Mul);
					VMatrix4x4.SetMethod("Matrix4x4 Invert() const", &Matrix4x4::Invert);
					VMatrix4x4.SetMethod("Matrix4x4 Transpose() const", &Matrix4x4::Transpose);
					VMatrix4x4.SetMethod("Matrix4x4 SetScale(const Vector3 &in) const", &Matrix4x4::SetScale);
					VMatrix4x4.SetMethod("Vector4 Row11() const", &Matrix4x4::Row11);
					VMatrix4x4.SetMethod("Vector4 Row22() const", &Matrix4x4::Row22);
					VMatrix4x4.SetMethod("Vector4 Row33() const", &Matrix4x4::Row33);
					VMatrix4x4.SetMethod("Vector4 Row44() const", &Matrix4x4::Row44);
					VMatrix4x4.SetMethod("Vector3 Up() const", &Matrix4x4::Up);
					VMatrix4x4.SetMethod("Vector3 Right() const", &Matrix4x4::Right);
					VMatrix4x4.SetMethod("Vector3 Forward() const", &Matrix4x4::Forward);
					VMatrix4x4.SetMethod("Vector3 Rotation() const", &Matrix4x4::Rotation);
					VMatrix4x4.SetMethod("Vector3 Position() const", &Matrix4x4::Position);
					VMatrix4x4.SetMethod("Vector3 Scale() const", &Matrix4x4::Scale);
					VMatrix4x4.SetMethod("Vector2 XY() const", &Matrix4x4::XY);
					VMatrix4x4.SetMethod("Vector3 XYZ() const", &Matrix4x4::XYZ);
					VMatrix4x4.SetMethod("Vector4 XYZW() const", &Matrix4x4::XYZW);
					VMatrix4x4.SetMethod("void Identify()", &Matrix4x4::Identify);
					VMatrix4x4.SetMethod("void Set(const Matrix4x4 &in)", &Matrix4x4::Set);
					VMatrix4x4.SetMethodEx("array<float>@ GetRow()", &Matrix4x4_GetRow);
					VMatrix4x4.SetMethodStatic("Matrix4x4 CreateLookAt(const Vector3 &in, const Vector3 &in, const Vector3 &in)", &Matrix4x4::CreateLookAt);
					VMatrix4x4.SetMethodStatic("Matrix4x4 CreateRotationX(float)", &Matrix4x4::CreateRotationX);
					VMatrix4x4.SetMethodStatic("Matrix4x4 CreateRotationY(float)", &Matrix4x4::CreateRotationY);
					VMatrix4x4.SetMethodStatic("Matrix4x4 CreateRotationZ(float)", &Matrix4x4::CreateRotationZ);
					VMatrix4x4.SetMethodStatic("Matrix4x4 CreateScale(const Vector3 &in)", &Matrix4x4::CreateScale);
					VMatrix4x4.SetMethodStatic("Matrix4x4 CreateTranslatedScale(const Vector3 &in, const Vector3 &in)", &Matrix4x4::CreateTranslatedScale);
					VMatrix4x4.SetMethodStatic("Matrix4x4 CreateTranslation(const Vector3 &in)", &Matrix4x4::CreateTranslation);
					VMatrix4x4.SetMethodStatic("Matrix4x4 CreatePerspective(float, float, float, float)", &Matrix4x4::CreatePerspective);
					VMatrix4x4.SetMethodStatic("Matrix4x4 CreatePerspectiveRad(float, float, float, float)", &Matrix4x4::CreatePerspectiveRad);
					VMatrix4x4.SetMethodStatic<Matrix4x4, float, float, float, float, float, float>("Matrix4x4 CreateOrthographic(float, float, float, float, float, float)", &Matrix4x4::CreateOrthographic);
					VMatrix4x4.SetMethodStatic<Matrix4x4, float, float, float, float>("Matrix4x4 CreateOrthographic(float, float, float, float)", &Matrix4x4::CreateOrthographic);
					VMatrix4x4.SetMethodStatic<Matrix4x4, const Vector3&, const Vector3&, const Vector3&>("Matrix4x4 Create(const Vector3 &in, const Vector3 &in, const Vector3 &in)", &Matrix4x4::Create);
					VMatrix4x4.SetMethodStatic<Matrix4x4, const Vector3&, const Vector3&>("Matrix4x4 Create(const Vector3 &in, const Vector3 &in)", &Matrix4x4::Create);
					VMatrix4x4.SetMethodStatic<Matrix4x4, const Vector3&, const Vector3&, const Vector3&>("Matrix4x4 CreateRotation(const Vector3 &in, const Vector3 &in, const Vector3 &in)", &Matrix4x4::CreateRotation);
					VMatrix4x4.SetMethodStatic<Matrix4x4, const Vector3&>("Matrix4x4 CreateRotation(const Vector3 &in)", &Matrix4x4::CreateRotation);
					VMatrix4x4.SetMethodStatic("Matrix4x4 CreateCameraRotation(const Vector3 &in)", &Matrix4x4::CreateCameraRotation);
					VMatrix4x4.SetMethodStatic("Matrix4x4 CreateCamera(const Vector3 &in, const Vector3 &in)", &Matrix4x4::CreateCamera);
					VMatrix4x4.SetMethodStatic("Matrix4x4 CreateOrigin(const Vector3 &in, const Vector3 &in)", &Matrix4x4::CreateOrigin);
					VMatrix4x4.SetMethodStatic("Matrix4x4 CreateCubeMapLookAt(int, const Vector3 &in)", &Matrix4x4::CreateCubeMapLookAt);
					VMatrix4x4.SetMethodStatic("Matrix4x4 CreateLineLightLookAt(const Vector3 &in, const Vector3 &in)", &Matrix4x4::CreateLineLightLookAt);
					VMatrix4x4.SetMethodStatic("Matrix4x4 Identity()", &Matrix4x4::Identity);
					VMatrix4x4.SetLCOperatorEx(VMOpFunc_Mul, "Matrix4x4", "const Matrix4x4& in", &Matrix4x4_Mul_1);
					VMatrix4x4.SetLCOperatorEx(VMOpFunc_Mul, "Matrix4x4", "float", &Matrix4x4_Mul_2);
					VMatrix4x4.SetLCOperatorEx(VMOpFunc_Equals, "bool", "const Matrix4x4 &in", &Matrix4x4_Equals);
					VMatrix4x4.SetLCOperatorEx(VMOpFunc_Index, "float", "int", &Matrix4x4_Index_1);
					VMatrix4x4.SetLOperatorEx(VMOpFunc_Index, "float&", "int", &Matrix4x4_Index_2);

					VQuaternion.SetProperty("float X", &Quaternion::X);
					VQuaternion.SetProperty("float Y", &Quaternion::Y);
					VQuaternion.SetProperty("float Z", &Quaternion::Z);
					VQuaternion.SetProperty("float W", &Quaternion::W);
					VQuaternion.SetConstructor<Quaternion>("void f()");
					VQuaternion.SetConstructor<Quaternion, float, float, float, float>("void f(float, float, float, float)");
					VQuaternion.SetConstructor<Quaternion, const Quaternion&>("void f(const Quaternion &in)");
					VQuaternion.SetConstructor<Quaternion, const Vector3&, float>("void f(const Vector3 &in, float)");
					VQuaternion.SetConstructor<Quaternion, const Vector3&>("void f(const Vector3 &in)");
					VQuaternion.SetConstructor<Quaternion, const Matrix4x4&>("void f(const Matrix4x4 &in)");
					VQuaternion.SetMethod<Quaternion, void, const Vector3&, float>("void Set(const Vector3 &in, float)", &Quaternion::Set);
					VQuaternion.SetMethod<Quaternion, void, const Vector3&>("void Set(const Vector3 &in)", &Quaternion::Set);
					VQuaternion.SetMethod<Quaternion, void, const Matrix4x4&>("void Set(const Matrix4x4 &in)", &Quaternion::Set);
					VQuaternion.SetMethod<Quaternion, void, const Quaternion&>("void Set(const Quaternion &in)", &Quaternion::Set);
					VQuaternion.SetMethod("Quaternion Normalize() const", &Quaternion::Normalize);
					VQuaternion.SetMethod("Quaternion Conjugate() const", &Quaternion::Conjugate);
					VQuaternion.SetMethod<Quaternion, Quaternion, float>("Quaternion Mul(float) const", &Quaternion::Mul);
					VQuaternion.SetMethod<Quaternion, Quaternion, const Quaternion&>("Quaternion Mul(const Quaternion &in) const", &Quaternion::Mul);
					VQuaternion.SetMethod<Quaternion, Quaternion, const Vector3&>("Quaternion Mul(const Vector3 &in) const", &Quaternion::Mul);
					VQuaternion.SetMethod("Quaternion Sub(const Quaternion &in) const", &Quaternion::Sub);
					VQuaternion.SetMethod("Quaternion Add(const Quaternion &in) const", &Quaternion::Add);
					VQuaternion.SetMethod("Quaternion Lerp(const Quaternion &in, float) const", &Quaternion::Lerp);
					VQuaternion.SetMethod("Quaternion SphericalLerp(const Quaternion &in, float) const", &Quaternion::SphericalLerp);
					VQuaternion.SetMethod("Matrix4x4 Rotation() const", &Quaternion::Rotation);
					VQuaternion.SetMethod("Vector3 Euler() const", &Quaternion::Euler);
					VQuaternion.SetMethod("float DotProduct(const Quaternion &in) const", &Quaternion::DotProduct);
					VQuaternion.SetMethod("float Length() const", &Quaternion::Length);
					VQuaternion.SetMethodStatic("Quaternion CreateEulerRotation(const Vector3 &in)", &Quaternion::CreateEulerRotation);
					VQuaternion.SetMethodStatic("Quaternion CreateRotation(const Matrix4x4 &in)", &Quaternion::CreateRotation);
			
					VAnimatorKey.SetProperty("Vector3 Position", &AnimatorKey::Position);
					VAnimatorKey.SetProperty("Vector3 Rotation", &AnimatorKey::Rotation);
					VAnimatorKey.SetProperty("Vector3 Scale", &AnimatorKey::Scale);
					VAnimatorKey.SetProperty("float PlayingSpeed", &AnimatorKey::PlayingSpeed);
					VAnimatorKey.SetConstructor<AnimatorKey>("void f()");
	
					VSkinAnimatorClip.SetProperty("string Name", &SkinAnimatorClip::Name);
					VSkinAnimatorClip.SetProperty("float PlayingSpeed", &SkinAnimatorClip::PlayingSpeed);
					VSkinAnimatorClip.SetConstructor<SkinAnimatorClip>("void f()");
					VSkinAnimatorClip.SetMethodEx("uint GetFramesCount()", &SkinAnimatorClip_GetFramesCount);
					VSkinAnimatorClip.SetMethodEx("uint GetKeysCount(uint)", &SkinAnimatorClip_GetKeysCount);
					VSkinAnimatorClip.SetMethodEx("AnimatorKey& GetKey(uint, uint)", &SkinAnimatorClip_GetKey);
	
					VKeyAnimatorClip.SetProperty("string Name", &KeyAnimatorClip::Name);
					VKeyAnimatorClip.SetProperty("float PlayingSpeed", &KeyAnimatorClip::PlayingSpeed);
					VKeyAnimatorClip.SetConstructor<KeyAnimatorClip>("void f()");
					VKeyAnimatorClip.SetMethodEx("uint GetKeysCount()", &KeyAnimatorClip_GetKeysCount);
					VKeyAnimatorClip.SetMethodEx("AnimatorKey& GetKey(uint)", &KeyAnimatorClip_GetKey);

					VJoint.SetProperty("string Name", &Joint::Name);
					VJoint.SetProperty("Matrix4x4 Transform", &Joint::Transform);
					VJoint.SetProperty("Matrix4x4 BindShape", &Joint::BindShape);
					VJoint.SetProperty("int Index", &Joint::Index);
					VJoint.SetConstructor<Joint>("void f()");
					VJoint.SetMethodEx("uint GetChildsCount()", &Joint_GetChildsCount);
					VJoint.SetMethodEx("Joint& GetChild(uint)", &Joint_GetChild);
		
					VRandomVector2.SetProperty("Vector2 Min", &RandomVector2::Min);
					VRandomVector2.SetProperty("Vector2 Max", &RandomVector2::Max);
					VRandomVector2.SetProperty("bool Intensity", &RandomVector2::Intensity);
					VRandomVector2.SetProperty("float Accuracy", &RandomVector2::Accuracy);
					VRandomVector2.SetConstructor<RandomVector2>("void f()");
					VRandomVector2.SetMethod("Vector2 Generate()", &RandomVector2::Generate);
			
					VRandomVector3.SetProperty("Vector3 Min", &RandomVector3::Min);
					VRandomVector3.SetProperty("Vector3 Max", &RandomVector3::Max);
					VRandomVector3.SetProperty("bool Intensity", &RandomVector3::Intensity);
					VRandomVector3.SetProperty("float Accuracy", &RandomVector3::Accuracy);
					VRandomVector3.SetConstructor<RandomVector3>("void f()");
					VRandomVector3.SetMethod("Vector3 Generate()", &RandomVector3::Generate);
		
					VRandomVector4.SetProperty("Vector4 Min", &RandomVector4::Min);
					VRandomVector4.SetProperty("Vector4 Max", &RandomVector4::Max);
					VRandomVector4.SetProperty("bool Intensity", &RandomVector4::Intensity);
					VRandomVector4.SetProperty("float Accuracy", &RandomVector4::Accuracy);
					VRandomVector4.SetConstructor<RandomVector4>("void f()");
					VRandomVector4.SetMethod("Vector4 Generate()", &RandomVector4::Generate);
			
					VRandomFloat.SetProperty("float Min", &RandomFloat::Min);
					VRandomFloat.SetProperty("float Max", &RandomFloat::Max);
					VRandomFloat.SetProperty("bool Intensity", &RandomFloat::Intensity);
					VRandomFloat.SetProperty("float Accuracy", &RandomFloat::Accuracy);
					VRandomFloat.SetConstructor<RandomFloat>("void f()");
					VRandomFloat.SetMethod("float Generate()", &RandomFloat::Generate);
			
					VHybi10PayloadHeader.SetGetterEx("uint16", "Opcode", &Hybi10PayloadHeader_GetOpcode);
					VHybi10PayloadHeader.SetGetterEx("uint16", "Rsv1", &Hybi10PayloadHeader_GetRsv1);
					VHybi10PayloadHeader.SetGetterEx("uint16", "Rsv2", &Hybi10PayloadHeader_GetRsv2);
					VHybi10PayloadHeader.SetGetterEx("uint16", "Rsv3", &Hybi10PayloadHeader_GetRsv3);
					VHybi10PayloadHeader.SetGetterEx("uint16", "Fin", &Hybi10PayloadHeader_GetFin);
					VHybi10PayloadHeader.SetGetterEx("uint16", "PayloadLength", &Hybi10PayloadHeader_GetPayloadLength);
					VHybi10PayloadHeader.SetGetterEx("uint16", "Mask", &Hybi10PayloadHeader_GetMask);
					VHybi10PayloadHeader.SetConstructor<Hybi10PayloadHeader>("void f()");
	
					VHybi10Request.SetProperty("string Payload", &Hybi10Request::Payload);
					VHybi10Request.SetProperty("int ExitCode", &Hybi10Request::ExitCode);
					VHybi10Request.SetProperty("int Type", &Hybi10Request::Type);
					VHybi10Request.SetMethod("string GetTextType()", &Hybi10Request::GetTextType);
					VHybi10Request.SetMethod("Hybi10Opcode GetEnumType()", &Hybi10Request::GetEnumType);
					VHybi10Request.SetConstructor<Hybi10Request>("void f()");
	
					VShapeContact.SetProperty("Vector3 LocalPoint1", &ShapeContact::LocalPoint1);
					VShapeContact.SetProperty("Vector3 LocalPoint2", &ShapeContact::LocalPoint2);
					VShapeContact.SetProperty("Vector3 PositionWorld1", &ShapeContact::PositionWorld1);
					VShapeContact.SetProperty("Vector3 PositionWorld2", &ShapeContact::PositionWorld2);
					VShapeContact.SetProperty("Vector3 NormalWorld", &ShapeContact::NormalWorld);
					VShapeContact.SetProperty("Vector3 LateralFrictionDirection1", &ShapeContact::LateralFrictionDirection1);
					VShapeContact.SetProperty("Vector3 LateralFrictionDirection2", &ShapeContact::LateralFrictionDirection2);
					VShapeContact.SetProperty("float Distance", &ShapeContact::Distance);
					VShapeContact.SetProperty("float CombinedFriction", &ShapeContact::CombinedFriction);
					VShapeContact.SetProperty("float CombinedRollingFriction", &ShapeContact::CombinedRollingFriction);
					VShapeContact.SetProperty("float CombinedSpinningFriction", &ShapeContact::CombinedSpinningFriction);
					VShapeContact.SetProperty("float CombinedRestitution", &ShapeContact::CombinedRestitution);
					VShapeContact.SetProperty("float AppliedImpulse", &ShapeContact::AppliedImpulse);
					VShapeContact.SetProperty("float AppliedImpulseLateral1", &ShapeContact::AppliedImpulseLateral1);
					VShapeContact.SetProperty("float AppliedImpulseLateral2", &ShapeContact::AppliedImpulseLateral2);
					VShapeContact.SetProperty("float ContactMotion1", &ShapeContact::ContactMotion1);
					VShapeContact.SetProperty("float ContactMotion2", &ShapeContact::ContactMotion2);
					VShapeContact.SetProperty("float ContactCFM", &ShapeContact::ContactCFM);
					VShapeContact.SetProperty("float CombinedContactStiffness", &ShapeContact::CombinedContactStiffness);
					VShapeContact.SetProperty("float ContactERP", &ShapeContact::ContactERP);
					VShapeContact.SetProperty("float CombinedContactDamping", &ShapeContact::CombinedContactDamping);
					VShapeContact.SetProperty("float FrictionCFM", &ShapeContact::FrictionCFM);
					VShapeContact.SetProperty("int PartId1", &ShapeContact::PartId1);
					VShapeContact.SetProperty("int PartId2", &ShapeContact::PartId2);
					VShapeContact.SetProperty("int Index1", &ShapeContact::Index1);
					VShapeContact.SetProperty("int Index2", &ShapeContact::Index2);
					VShapeContact.SetProperty("int ContactPointFlags", &ShapeContact::ContactPointFlags);
					VShapeContact.SetProperty("int LifeTime", &ShapeContact::LifeTime);
					VShapeContact.SetConstructor<ShapeContact>("void f()");
		
					VRayContact.SetProperty("Vector3 HitNormalLocal", &RayContact::HitNormalLocal);
					VRayContact.SetProperty("Vector3 HitNormalWorld", &RayContact::HitNormalWorld);
					VRayContact.SetProperty("Vector3 HitPointWorld", &RayContact::HitPointWorld);
					VRayContact.SetProperty("Vector3 RayFromWorld", &RayContact::RayFromWorld);
					VRayContact.SetProperty("Vector3 RayToWorld", &RayContact::RayToWorld);
					VRayContact.SetProperty("float HitFraction", &RayContact::HitFraction);
					VRayContact.SetProperty("float ClosestHitFraction", &RayContact::ClosestHitFraction);
					VRayContact.SetProperty("bool NormalInWorldSpace", &RayContact::NormalInWorldSpace);
					VRayContact.SetConstructor<RayContact>("void f()");
				
					VRegexBracket.SetGetterEx("string", "Pointer", &RegexBracket_GetPointer);
					VRegexBracket.SetProperty("int Branches", &RegexBracket::Branches);
					VRegexBracket.SetProperty("int BranchesCount", &RegexBracket::BranchesCount);
					VRegexBracket.SetConstructor<RegexBracket>("void f()");
			
					VRegexBranch.SetProperty("int BracketIndex", &RegexBranch::BracketIndex);
					VRegexBranch.SetConstructor<RegexBranch>("void f()");
				
					VRegexMatch.SetGetterEx("string", "Pointer", &RegexMatch_GetPointer);
					VRegexMatch.SetProperty("int Start", &RegexMatch::Start);
					VRegexMatch.SetProperty("int End", &RegexMatch::End);
					VRegexMatch.SetProperty("int Steps", &RegexMatch::Steps);
					VRegexMatch.SetConstructor<RegexMatch>("void f()");
			
					VRegExp.SetProperty("string Regex", &RegExp::Regex);
					VRegExp.SetProperty("int MaxBranches", &RegExp::MaxBranches);
					VRegExp.SetProperty("int MaxBrackets", &RegExp::MaxBrackets);
					VRegExp.SetProperty("int MaxMatches", &RegExp::MaxMatches);
					VRegExp.SetProperty("RegexFlags Flags", &RegExp::Flags);
					VRegExp.SetConstructor<RegExp>("void f()");
			
					VRegexResult.SetConstructor<RegexResult>("void f()");
					VRegexResult.SetMethod("void ResetNextMatch()", &RegexResult::ResetNextMatch);
					VRegexResult.SetMethod("bool HasMatch()", &RegexResult::HasMatch);
					VRegexResult.SetMethod("int Start()", &RegexResult::Start);
					VRegexResult.SetMethod("int End()", &RegexResult::End);
					VRegexResult.SetMethod("int GetSteps()", &RegexResult::GetSteps);
					VRegexResult.SetMethod("int GetMatchesCount()", &RegexResult::GetMatchesCount);
					VRegexResult.SetMethod("RegexState GetState()", &RegexResult::GetState);
					VRegexResult.SetMethodEx("string GetPointer()", &RegexResult_GetPointer);
			
					VMD5Hasher.SetConstructor<MD5Hasher>("void f()");
					VMD5Hasher.SetMethodEx("void Transform(const string &in, uint)", &MD5Hasher_Transform);
					VMD5Hasher.SetMethod<MD5Hasher, void, const std::string&, unsigned int>("void Update(const string &in, uint)", &MD5Hasher::Update);
					VMD5Hasher.SetMethod("void Finalize()", &MD5Hasher::Finalize);
					VMD5Hasher.SetMethod("string ToHex() const", &MD5Hasher::ToHex);
					VMD5Hasher.SetMethod("string ToRaw() const", &MD5Hasher::ToRaw);
	
					VPreprocessorDesc.SetProperty("bool Pragmas", &Preprocessor::Desc::Pragmas);
					VPreprocessorDesc.SetProperty("bool Includes", &Preprocessor::Desc::Includes);
					VPreprocessorDesc.SetProperty("bool Defines", &Preprocessor::Desc::Defines);
					VPreprocessorDesc.SetProperty("bool Conditions", &Preprocessor::Desc::Conditions);
		
					VPreprocessor.SetFunctionDef("bool ProcIncludeCallback(Preprocessor@, const IncludeResult &in, string &in)");
					VPreprocessor.SetFunctionDef("bool ProcPragmaCallback(Preprocessor@, const string &in)");
					VPreprocessor.SetUnmanagedConstructor<Preprocessor>("Preprocessor@ f()");
					VPreprocessor.SetMethod("void SetIncludeOptions(const IncludeDesc &in)", &Preprocessor::SetIncludeOptions);
					VPreprocessor.SetMethod("void Define(const string &in)", &Preprocessor::Define);
					VPreprocessor.SetMethod("void Undefine(const string &in)", &Preprocessor::Undefine);
					VPreprocessor.SetMethod("void Clear()", &Preprocessor::Clear);
					VPreprocessor.SetMethod("bool Process(const string &in, string &in)", &Preprocessor::Process);
					VPreprocessor.SetMethod("void SetFeatures(const PreprocessorDesc &in)", &Preprocessor::SetFeatures);
					VPreprocessor.SetMethodEx("bool IsDefined(const string &in) const", &Preprocessor_IsDefined);
					VPreprocessor.SetMethodEx("void SetIncludeCallbackOnce(Compute::Preprocessor::ProcIncludeCallback@)", &Preprocessor_SetIncludeCallbackOnce);
					VPreprocessor.SetMethodEx("void SetPragmaCallbackOnce(Compute::Preprocessor::ProcPragmaCallback@)", &Preprocessor_SetPragmaCallbackOnce);
					VPreprocessor.SetMethodStatic("IncludeResult ResolveInclude(const IncludeDesc &in)", &Preprocessor::ResolveInclude);
	
					VTransform.SetProperty<Transform>("Vector3 Position", &Transform::Position);
					VTransform.SetProperty<Transform>("Vector3 Rotation", &Transform::Rotation);
					VTransform.SetProperty<Transform>("Vector3 Scale", &Transform::Scale);
					VTransform.SetProperty<Transform>("bool ConstantScale", &Transform::ConstantScale);
					VTransform.SetUnmanagedConstructor<Transform>("Transform@ f()");
					VTransform.SetMethod("void GetRootBasis(Vector3 &out, Vector3 &out, Vector3 &out)", &Transform::GetRootBasis);
					VTransform.SetMethod("void SetRoot(Transform@+)", &Transform::SetRoot);
					VTransform.SetMethod("void SetMatrix(const Matrix4x4 &in)", &Transform::SetMatrix);
					VTransform.SetMethod("void SetLocals(Transform@+)", &Transform::SetLocals);
					VTransform.SetMethod("void Copy(Transform@+)", &Transform::Copy);
					VTransform.SetMethod("void AddChild(Transform@+)", &Transform::AddChild);
					VTransform.SetMethod("void RemoveChild(Transform@+)", &Transform::RemoveChild);
					VTransform.SetMethod("void RemoveChilds()", &Transform::RemoveChilds);
					VTransform.SetMethod("void Synchronize()", &Transform::Synchronize);
					VTransform.SetMethodEx("Vector3& GetLocalPosition()", &Transform_GetLocalPosition);
					VTransform.SetMethodEx("Vector3& GetLocalRotation()", &Transform_GetLocalRotation);
					VTransform.SetMethodEx("Vector3& GetLocalScale()", &Transform_GetLocalScale);
					VTransform.SetMethod("Vector3 Up()", &Transform::Up);
					VTransform.SetMethod("Vector3 Right()", &Transform::Right);
					VTransform.SetMethod("Vector3 Forward()", &Transform::Forward);
					VTransform.SetMethod("Vector3 Direction(const Vector3 &in)", &Transform::Direction);
					VTransform.SetMethod<Transform, Matrix4x4>("Matrix4x4 GetWorld()", &Transform::GetWorld);
					VTransform.SetMethod<Transform, Matrix4x4>("Matrix4x4 GetWorldUnscaled()", &Transform::GetWorldUnscaled);
					VTransform.SetMethod<Transform, Matrix4x4>("Matrix4x4 GetLocal()", &Transform::GetLocal);
					VTransform.SetMethod<Transform, Matrix4x4>("Matrix4x4 GetLocalUnscaled()", &Transform::GetLocalUnscaled);
					VTransform.SetMethod("Matrix4x4 Localize(const Matrix4x4 &in)", &Transform::Localize);
					VTransform.SetMethod("Matrix4x4 Globalize(const Matrix4x4 &in)", &Transform::Globalize);
					VTransform.SetMethod("bool HasRoot(Transform@+)", &Transform::HasRoot);
					VTransform.SetMethod("bool HasChild(Transform@+)", &Transform::HasChild);
					VTransform.SetMethod("int GetChildCount()", &Transform::GetChildCount);
					VTransform.SetMethod("Transform@+ GetChild(int)", &Transform::GetChild);
					VTransform.SetMethod("Transform@+ GetUpperRoot()", &Transform::GetUpperRoot);
					VTransform.SetMethod("Transform@+ GetRoot()", &Transform::GetRoot);

					VRigidBodyDesc.SetProperty("float Anticipation", &RigidBody::Desc::Anticipation);
					VRigidBodyDesc.SetProperty("float Mass", &RigidBody::Desc::Mass);
					VRigidBodyDesc.SetProperty("Vector3 Position", &RigidBody::Desc::Position);
					VRigidBodyDesc.SetProperty("Vector3 Rotation", &RigidBody::Desc::Rotation);
					VRigidBodyDesc.SetProperty("Vector3 Scale", &RigidBody::Desc::Scale);

					VRigidBody.SetMethod<RigidBody, void, const Vector3&>("void Push(const Vector3 &in)", &RigidBody::Push);
					VRigidBody.SetMethod<RigidBody, void, const Vector3&, const Vector3&>("void Push(const Vector3 &in, const Vector3 &in)", &RigidBody::Push);
					VRigidBody.SetMethod<RigidBody, void, const Vector3&, const Vector3&, const Vector3&>("void Push(const Vector3 &in, const Vector3 &in, const Vector3 &in)", &RigidBody::Push);
					VRigidBody.SetMethod<RigidBody, void, const Vector3&>("void PushKinematic(const Vector3 &in)", &RigidBody::PushKinematic);
					VRigidBody.SetMethod<RigidBody, void, const Vector3&, const Vector3&>("void PushKinematic(const Vector3 &in, const Vector3 &in)", &RigidBody::PushKinematic);
					VRigidBody.SetMethod("void Synchronize(Transform@+, bool)", &RigidBody::Synchronize);
					VRigidBody.SetMethod("void SetCollisionFlags(int)", &RigidBody::SetCollisionFlags);
					VRigidBody.SetMethod("void SetActivity(bool)", &RigidBody::SetActivity);
					VRigidBody.SetMethod("void SetAsGhost()", &RigidBody::SetAsGhost);
					VRigidBody.SetMethod("void SetAsNormal()", &RigidBody::SetAsNormal);
					VRigidBody.SetMethod("void SetSelfPointer()", &RigidBody::SetSelfPointer);
					VRigidBody.SetMethod("void SetMass(float)", &RigidBody::SetMass);
					VRigidBody.SetMethod("void SetActivationState(MotionState)", &RigidBody::SetActivationState);
					VRigidBody.SetMethod("void SetAngularDamping(float)", &RigidBody::SetAngularDamping);
					VRigidBody.SetMethod("void SetAngularSleepingThreshold(float)", &RigidBody::SetAngularSleepingThreshold);
					VRigidBody.SetMethod("void SetSpinningFriction(float)", &RigidBody::SetSpinningFriction);
					VRigidBody.SetMethod("void SetContactStiffness(float)", &RigidBody::SetContactStiffness);
					VRigidBody.SetMethod("void SetContactDamping(float)", &RigidBody::SetContactDamping);
					VRigidBody.SetMethod("void SetFriction(float)", &RigidBody::SetFriction);
					VRigidBody.SetMethod("void SetRestitution(float)", &RigidBody::SetRestitution);
					VRigidBody.SetMethod("void SetHitFraction(float)", &RigidBody::SetHitFraction);
					VRigidBody.SetMethod("void SetLinearDamping(float)", &RigidBody::SetLinearDamping);
					VRigidBody.SetMethod("void SetLinearSleepingThreshold(float)", &RigidBody::SetLinearSleepingThreshold);
					VRigidBody.SetMethod("void SetCcdMotionThreshold(float)", &RigidBody::SetCcdMotionThreshold);
					VRigidBody.SetMethod("void SetCcdSweptSphereRadius(float)", &RigidBody::SetCcdSweptSphereRadius);
					VRigidBody.SetMethod("void SetContactProcessingThreshold(float)", &RigidBody::SetContactProcessingThreshold);
					VRigidBody.SetMethod("void SetDeactivationTime(float)", &RigidBody::SetDeactivationTime);
					VRigidBody.SetMethod("void SetRollingFriction(float)", &RigidBody::SetRollingFriction);
					VRigidBody.SetMethod("void SetAngularFactor(const Vector3 &in)", &RigidBody::SetAngularFactor);
					VRigidBody.SetMethod("void SetAnisotropicFriction(const Vector3 &in)", &RigidBody::SetAnisotropicFriction);
					VRigidBody.SetMethod("void SetGravity(const Vector3 &in)", &RigidBody::SetGravity);
					VRigidBody.SetMethod("void SetLinearFactor(const Vector3 &in)", &RigidBody::SetLinearFactor);
					VRigidBody.SetMethod("void SetLinearVelocity(const Vector3 &in)", &RigidBody::SetLinearVelocity);
					VRigidBody.SetMethod("void SetAngularVelocity(const Vector3 &in)", &RigidBody::SetAngularVelocity);
					VRigidBody.SetMethod("MotionState GetActivationState()", &RigidBody::GetActivationState);
					VRigidBody.SetMethod("Shape GetCollisionShapeType()", &RigidBody::GetCollisionShapeType);
					VRigidBody.SetMethod("Vector3 GetAngularFactor()", &RigidBody::GetAngularFactor);
					VRigidBody.SetMethod("Vector3 GetAnisotropicFriction()", &RigidBody::GetAnisotropicFriction);
					VRigidBody.SetMethod("Vector3 GetGravity()", &RigidBody::GetGravity);
					VRigidBody.SetMethod("Vector3 GetLinearFactor()", &RigidBody::GetLinearFactor);
					VRigidBody.SetMethod("Vector3 GetLinearVelocity()", &RigidBody::GetLinearVelocity);
					VRigidBody.SetMethod("Vector3 GetAngularVelocity()", &RigidBody::GetAngularVelocity);
					VRigidBody.SetMethod("Vector3 GetScale()", &RigidBody::GetScale);
					VRigidBody.SetMethod("Vector3 GetPosition()", &RigidBody::GetPosition);
					VRigidBody.SetMethod("Vector3 GetRotation()", &RigidBody::GetRotation);
					VRigidBody.SetMethod("bool IsActive()", &RigidBody::IsActive);
					VRigidBody.SetMethod("bool IsStatic()", &RigidBody::IsStatic);
					VRigidBody.SetMethod("bool IsGhost()", &RigidBody::IsGhost);
					VRigidBody.SetMethod("bool IsColliding()", &RigidBody::IsColliding);
					VRigidBody.SetMethod("float GetSpinningFriction()", &RigidBody::GetSpinningFriction);
					VRigidBody.SetMethod("float GetContactStiffness()", &RigidBody::GetContactStiffness);
					VRigidBody.SetMethod("float GetContactDamping()", &RigidBody::GetContactDamping);
					VRigidBody.SetMethod("float GetAngularDamping()", &RigidBody::GetAngularDamping);
					VRigidBody.SetMethod("float GetAngularSleepingThreshold()", &RigidBody::GetAngularSleepingThreshold);
					VRigidBody.SetMethod("float GetFriction()", &RigidBody::GetFriction);
					VRigidBody.SetMethod("float GetRestitution()", &RigidBody::GetRestitution);
					VRigidBody.SetMethod("float GetHitFraction()", &RigidBody::GetHitFraction);
					VRigidBody.SetMethod("float GetLinearDamping()", &RigidBody::GetLinearDamping);
					VRigidBody.SetMethod("float GetLinearSleepingThreshold()", &RigidBody::GetLinearSleepingThreshold);
					VRigidBody.SetMethod("float GetCcdMotionThreshold()", &RigidBody::GetCcdMotionThreshold);
					VRigidBody.SetMethod("float GetCcdSweptSphereRadius()", &RigidBody::GetCcdSweptSphereRadius);
					VRigidBody.SetMethod("float GetContactProcessingThreshold()", &RigidBody::GetContactProcessingThreshold);
					VRigidBody.SetMethod("float GetDeactivationTime()", &RigidBody::GetDeactivationTime);
					VRigidBody.SetMethod("float GetRollingFriction()", &RigidBody::GetRollingFriction);
					VRigidBody.SetMethod("float GetMass()", &RigidBody::GetMass);
					VRigidBody.SetMethod("int GetCollisionFlags()", &RigidBody::GetCollisionFlags);
					VRigidBody.SetMethod("RigidBodyDesc& GetInitialState()", &RigidBody::GetInitialState);
					VRigidBody.SetMethod("RigidBody@+ Copy()", &RigidBody::Copy);

					VSoftBodyConfig.SetProperty("SoftAeroModel AeroModel", &SoftBody::Desc::SConfig::AeroModel);
					VSoftBodyConfig.SetProperty("float VCF", &SoftBody::Desc::SConfig::VCF);
					VSoftBodyConfig.SetProperty("float DP", &SoftBody::Desc::SConfig::DP);
					VSoftBodyConfig.SetProperty("float DG", &SoftBody::Desc::SConfig::DG);
					VSoftBodyConfig.SetProperty("float LF", &SoftBody::Desc::SConfig::LF);
					VSoftBodyConfig.SetProperty("float PR", &SoftBody::Desc::SConfig::PR);
					VSoftBodyConfig.SetProperty("float VC", &SoftBody::Desc::SConfig::VC);
					VSoftBodyConfig.SetProperty("float DF", &SoftBody::Desc::SConfig::DF);
					VSoftBodyConfig.SetProperty("float MT", &SoftBody::Desc::SConfig::MT);
					VSoftBodyConfig.SetProperty("float CHR", &SoftBody::Desc::SConfig::CHR);
					VSoftBodyConfig.SetProperty("float KHR", &SoftBody::Desc::SConfig::KHR);
					VSoftBodyConfig.SetProperty("float SHR", &SoftBody::Desc::SConfig::SHR);
					VSoftBodyConfig.SetProperty("float AHR", &SoftBody::Desc::SConfig::AHR);
					VSoftBodyConfig.SetProperty("float SRHR_CL", &SoftBody::Desc::SConfig::SRHR_CL);
					VSoftBodyConfig.SetProperty("float SKHR_CL", &SoftBody::Desc::SConfig::SKHR_CL);
					VSoftBodyConfig.SetProperty("float SSHR_CL", &SoftBody::Desc::SConfig::SSHR_CL);
					VSoftBodyConfig.SetProperty("float SR_SPLT_CL", &SoftBody::Desc::SConfig::SR_SPLT_CL);
					VSoftBodyConfig.SetProperty("float SK_SPLT_CL", &SoftBody::Desc::SConfig::SK_SPLT_CL);
					VSoftBodyConfig.SetProperty("float SS_SPLT_CL", &SoftBody::Desc::SConfig::SS_SPLT_CL);
					VSoftBodyConfig.SetProperty("float MaxVolume", &SoftBody::Desc::SConfig::MaxVolume);
					VSoftBodyConfig.SetProperty("float TimeScale", &SoftBody::Desc::SConfig::TimeScale);
					VSoftBodyConfig.SetProperty("float Drag", &SoftBody::Desc::SConfig::Drag);
					VSoftBodyConfig.SetProperty("float MaxStress", &SoftBody::Desc::SConfig::MaxStress);
					VSoftBodyConfig.SetProperty("int Clusters", &SoftBody::Desc::SConfig::Clusters);
					VSoftBodyConfig.SetProperty("int Constraints", &SoftBody::Desc::SConfig::Constraints);
					VSoftBodyConfig.SetProperty("int VIterations", &SoftBody::Desc::SConfig::VIterations);
					VSoftBodyConfig.SetProperty("int PIterations", &SoftBody::Desc::SConfig::PIterations);
					VSoftBodyConfig.SetProperty("int DIterations", &SoftBody::Desc::SConfig::DIterations);
					VSoftBodyConfig.SetProperty("int CIterations", &SoftBody::Desc::SConfig::CIterations);
					VSoftBodyConfig.SetProperty("int Collisions", &SoftBody::Desc::SConfig::Collisions);
			
					VSoftBodyDesc.SetProperty("SoftBodyConfig Config", &SoftBody::Desc::Config);
					VSoftBodyDesc.SetProperty("float Anticipation", &SoftBody::Desc::Anticipation);
					VSoftBodyDesc.SetProperty("Vector3 Position", &SoftBody::Desc::Position);
					VSoftBodyDesc.SetProperty("Vector3 Rotation", &SoftBody::Desc::Rotation);
					VSoftBodyDesc.SetProperty("Vector3 Scale", &SoftBody::Desc::Scale);
					VSoftBodyDesc.SetConstructor<SoftBody::Desc>("void f()");
					VSoftBodyDesc.SetMethodEx("void InitRope(int, const Vector3 &in, bool, const Vector3 &in, bool)", &SoftBodyDesc_InitRope);
					VSoftBodyDesc.SetMethodEx("void InitPatch(int, int, bool, const Vector3 &in, bool, const Vector3 &in, bool, const Vector3 &in, bool, const Vector3 &in, bool)", &SoftBodyDesc_InitPatch);
					VSoftBodyDesc.SetMethodEx("void InitEllipsoid(int, const Vector3 &in, const Vector3 &in)", &SoftBodyDesc_InitEllipsoid);
	
					VSoftBodyRayCast.SetProperty("SoftBody@ Body", &SoftBody::RayCast::Body);
					VSoftBodyRayCast.SetProperty("SoftFeature Feature", &SoftBody::RayCast::Feature);
					VSoftBodyRayCast.SetProperty("float Fraction", &SoftBody::RayCast::Fraction);
					VSoftBodyRayCast.SetProperty("int Index", &SoftBody::RayCast::Index);
					VSoftBodyRayCast.SetConstructor<SoftBody::RayCast>("void f()");
				
					VSoftBody.SetMethod("SoftBody@+ Copy()", &SoftBody::Copy);
					VSoftBody.SetMethod("void Activate(bool)", &SoftBody::Activate);
					VSoftBody.SetMethod("void Synchronize(Transform@+, bool)", &SoftBody::Synchronize);
					VSoftBody.SetMethod("void SetContactStiffnessAndDamping(float, float)", &SoftBody::SetContactStiffnessAndDamping);
					VSoftBody.SetMethod<SoftBody, void, int, RigidBody*, bool, float>("void AddAnchor(int, RigidBody@+, bool, float)", &SoftBody::AddAnchor);
					VSoftBody.SetMethod<SoftBody, void, int, RigidBody*, const Vector3&, bool, float>("void AddAnchor(int, RigidBody@+, const Vector3 &in, bool, float)", &SoftBody::AddAnchor);
					VSoftBody.SetMethod<SoftBody, void, const Vector3&>("void AddForce(const Vector3 &in)", &SoftBody::AddForce);
					VSoftBody.SetMethod<SoftBody, void, const Vector3&, int>("void AddForce(const Vector3 &in, int)", &SoftBody::AddForce);
					VSoftBody.SetMethod("void AddAeroForceToNode(const Vector3 &in, int)", &SoftBody::AddAeroForceToNode);
					VSoftBody.SetMethod("void AddAeroForceToFace(const Vector3 &in, int)", &SoftBody::AddAeroForceToFace);
					VSoftBody.SetMethod<SoftBody, void, const Vector3&>("void AddVelocity(const Vector3 &in)", &SoftBody::AddVelocity);
					VSoftBody.SetMethod<SoftBody, void, const Vector3&, int>("void AddVelocity(const Vector3 &in, int)", &SoftBody::AddVelocity);
					VSoftBody.SetMethod("void SetVelocity(const Vector3 &in)", &SoftBody::SetVelocity);
					VSoftBody.SetMethod("void SetMass(int, float)", &SoftBody::SetMass);
					VSoftBody.SetMethod("void SetTotalMass(float, bool)", &SoftBody::SetTotalMass);
					VSoftBody.SetMethod("void SetTotalDensity(float)", &SoftBody::SetTotalDensity);
					VSoftBody.SetMethod("void SetVolumeMass(float)", &SoftBody::SetVolumeMass);
					VSoftBody.SetMethod("void SetVolumeDensity(float)", &SoftBody::SetVolumeDensity);
					VSoftBody.SetMethod("void Translate(const Vector3 &in)", &SoftBody::Translate);
					VSoftBody.SetMethod("void Rotate(const Vector3 &in)", &SoftBody::Rotate);
					VSoftBody.SetMethod("void Scale(const Vector3 &in)", &SoftBody::Scale);
					VSoftBody.SetMethod("void SetRestLengthScale(float)", &SoftBody::SetRestLengthScale);
					VSoftBody.SetMethod("void SetPose(bool, bool)", &SoftBody::SetPose);
					VSoftBody.SetMethod("float GetMass(int) const", &SoftBody::GetMass);
					VSoftBody.SetMethod("float GetTotalMass() const", &SoftBody::GetTotalMass);
					VSoftBody.SetMethod("float GetRestLengthScale()", &SoftBody::GetRestLengthScale);
					VSoftBody.SetMethod("float GetVolume() const", &SoftBody::GetVolume);
					VSoftBody.SetMethod("int GenerateBendingConstraints(int)", &SoftBody::GenerateBendingConstraints);
					VSoftBody.SetMethod("void RandomizeConstraints()", &SoftBody::RandomizeConstraints);
					VSoftBody.SetMethod("bool CutLink(int, int, float)", &SoftBody::CutLink);
					VSoftBody.SetMethod("bool RayTest(const Vector3 &in, const Vector3 &in, SoftBodyRayCast &in)", &SoftBody::RayTest);
					VSoftBody.SetMethod("void SetWindVelocity(const Vector3 &in)", &SoftBody::SetWindVelocity);
					VSoftBody.SetMethod("Vector3 GetWindVelocity()", &SoftBody::GetWindVelocity);
					VSoftBody.SetMethod("void GetAabb(Vector3 &in, Vector3 &in) const", &SoftBody::GetAabb);
					VSoftBody.SetMethod("void IndicesToPointers(const int &in)", &SoftBody::IndicesToPointers);
					VSoftBody.SetMethod("void SetSpinningFriction(float)", &SoftBody::SetSpinningFriction);
					VSoftBody.SetMethod("Vector3 GetLinearVelocity()", &SoftBody::GetLinearVelocity);
					VSoftBody.SetMethod("Vector3 GetAngularVelocity()", &SoftBody::GetAngularVelocity);
					VSoftBody.SetMethod("Vector3 GetCenterPosition()", &SoftBody::GetCenterPosition);
					VSoftBody.SetMethod("void SetActivity(bool)", &SoftBody::SetActivity);
					VSoftBody.SetMethod("void SetAsGhost()", &SoftBody::SetAsGhost);
					VSoftBody.SetMethod("void SetAsNormal()", &SoftBody::SetAsNormal);
					VSoftBody.SetMethod("void SetSelfPointer()", &SoftBody::SetSelfPointer);
					VSoftBody.SetMethod("void SetActivationState(MotionState)", &SoftBody::SetActivationState);
					VSoftBody.SetMethod("void SetContactStiffness(float)", &SoftBody::SetContactStiffness);
					VSoftBody.SetMethod("void SetContactDamping(float)", &SoftBody::SetContactDamping);
					VSoftBody.SetMethod("void SetFriction(float)", &SoftBody::SetFriction);
					VSoftBody.SetMethod("void SetRestitution(float)", &SoftBody::SetRestitution);
					VSoftBody.SetMethod("void SetHitFraction(float)", &SoftBody::SetHitFraction);
					VSoftBody.SetMethod("void SetCcdMotionThreshold(float)", &SoftBody::SetCcdMotionThreshold);
					VSoftBody.SetMethod("void SetCcdSweptSphereRadius(float)", &SoftBody::SetCcdSweptSphereRadius);
					VSoftBody.SetMethod("void SetContactProcessingThreshold(float)", &SoftBody::SetContactProcessingThreshold);
					VSoftBody.SetMethod("void SetDeactivationTime(float)", &SoftBody::SetDeactivationTime);
					VSoftBody.SetMethod("void SetRollingFriction(float)", &SoftBody::SetRollingFriction);
					VSoftBody.SetMethod("void SetAnisotropicFriction(const Vector3 &in)", &SoftBody::SetAnisotropicFriction);
					VSoftBody.SetMethod("void SetConfig(const SoftBodyConfig &in)", &SoftBody::SetConfig);
					VSoftBody.SetMethod("Shape GetCollisionShapeType()", &SoftBody::GetCollisionShapeType);
					VSoftBody.SetMethod("MotionState GetActivationState()", &SoftBody::GetActivationState);
					VSoftBody.SetMethod("Vector3 GetAnisotropicFriction()", &SoftBody::GetAnisotropicFriction);
					VSoftBody.SetMethod("Vector3 GetScale()", &SoftBody::GetScale);
					VSoftBody.SetMethod("Vector3 GetPosition()", &SoftBody::GetPosition);
					VSoftBody.SetMethod("Vector3 GetRotation()", &SoftBody::GetRotation);
					VSoftBody.SetMethod("bool IsActive()", &SoftBody::IsActive);
					VSoftBody.SetMethod("bool IsStatic()", &SoftBody::IsStatic);
					VSoftBody.SetMethod("bool IsGhost()", &SoftBody::IsGhost);
					VSoftBody.SetMethod("bool IsColliding()", &SoftBody::IsColliding);
					VSoftBody.SetMethod("float GetSpinningFriction()", &SoftBody::GetSpinningFriction);
					VSoftBody.SetMethod("float GetContactStiffness()", &SoftBody::GetContactStiffness);
					VSoftBody.SetMethod("float GetContactDamping()", &SoftBody::GetContactDamping);
					VSoftBody.SetMethod("float GetFriction()", &SoftBody::GetFriction);
					VSoftBody.SetMethod("float GetRestitution()", &SoftBody::GetRestitution);
					VSoftBody.SetMethod("float GetHitFraction()", &SoftBody::GetHitFraction);
					VSoftBody.SetMethod("float GetCcdMotionThreshold()", &SoftBody::GetCcdMotionThreshold);
					VSoftBody.SetMethod("float GetCcdSweptSphereRadius()", &SoftBody::GetCcdSweptSphereRadius);
					VSoftBody.SetMethod("float GetContactProcessingThreshold()", &SoftBody::GetContactProcessingThreshold);
					VSoftBody.SetMethod("float GetDeactivationTime()", &SoftBody::GetDeactivationTime);
					VSoftBody.SetMethod("float GetRollingFriction()", &SoftBody::GetRollingFriction);
					VSoftBody.SetMethod("int GetCollisionFlags()", &SoftBody::GetCollisionFlags);
					VSoftBody.SetMethod("int GetVerticesCount()", &SoftBody::GetVerticesCount);
					VSoftBody.SetMethod("SoftBodyDesc& GetInitialState()", &SoftBody::GetInitialState);
			
					VSliderConstraintDesc.SetProperty("RigidBody@ Target1", &SliderConstraint::Desc::Target1);
					VSliderConstraintDesc.SetProperty("RigidBody@ Target2", &SliderConstraint::Desc::Target2);
					VSliderConstraintDesc.SetProperty("bool UseCollisions", &SliderConstraint::Desc::UseCollisions);
					VSliderConstraintDesc.SetProperty("bool UseLinearPower", &SliderConstraint::Desc::UseLinearPower);
			
					VSliderConstraint.SetMethod("SliderConstraint@+ Copy()", &SliderConstraint::Copy);
					VSliderConstraint.SetMethod("void SetAngularMotorVelocity(float)", &SliderConstraint::SetAngularMotorVelocity);
					VSliderConstraint.SetMethod("void SetLinearMotorVelocity(float)", &SliderConstraint::SetLinearMotorVelocity);
					VSliderConstraint.SetMethod("void SetUpperLinearLimit(float)", &SliderConstraint::SetUpperLinearLimit);
					VSliderConstraint.SetMethod("void SetLowerLinearLimit(float)", &SliderConstraint::SetLowerLinearLimit);
					VSliderConstraint.SetMethod("void SetBreakingImpulseThreshold(float)", &SliderConstraint::SetBreakingImpulseThreshold);
					VSliderConstraint.SetMethod("void SetAngularDampingDirection(float)", &SliderConstraint::SetAngularDampingDirection);
					VSliderConstraint.SetMethod("void SetLinearDampingDirection(float)", &SliderConstraint::SetLinearDampingDirection);
					VSliderConstraint.SetMethod("void SetAngularDampingLimit(float)", &SliderConstraint::SetAngularDampingLimit);
					VSliderConstraint.SetMethod("void SetLinearDampingLimit(float)", &SliderConstraint::SetLinearDampingLimit);
					VSliderConstraint.SetMethod("void SetAngularDampingOrtho(float)", &SliderConstraint::SetAngularDampingOrtho);
					VSliderConstraint.SetMethod("void SetLinearDampingOrtho(float)", &SliderConstraint::SetLinearDampingOrtho);
					VSliderConstraint.SetMethod("void SetUpperAngularLimit(float)", &SliderConstraint::SetUpperAngularLimit);
					VSliderConstraint.SetMethod("void SetLowerAngularLimit(float)", &SliderConstraint::SetLowerAngularLimit);
					VSliderConstraint.SetMethod("void SetMaxAngularMotorForce(float)", &SliderConstraint::SetMaxAngularMotorForce);
					VSliderConstraint.SetMethod("void SetMaxLinearMotorForce(float)", &SliderConstraint::SetMaxLinearMotorForce);
					VSliderConstraint.SetMethod("void SetAngularRestitutionDirection(float)", &SliderConstraint::SetAngularRestitutionDirection);
					VSliderConstraint.SetMethod("void SetLinearRestitutionDirection(float)", &SliderConstraint::SetLinearRestitutionDirection);
					VSliderConstraint.SetMethod("void SetAngularRestitutionLimit(float)", &SliderConstraint::SetAngularRestitutionLimit);
					VSliderConstraint.SetMethod("void SetLinearRestitutionLimit(float)", &SliderConstraint::SetLinearRestitutionLimit);
					VSliderConstraint.SetMethod("void SetAngularRestitutionOrtho(float)", &SliderConstraint::SetAngularRestitutionOrtho);
					VSliderConstraint.SetMethod("void SetLinearRestitutionOrtho(float)", &SliderConstraint::SetLinearRestitutionOrtho);
					VSliderConstraint.SetMethod("void SetAngularSoftnessDirection(float)", &SliderConstraint::SetAngularSoftnessDirection);
					VSliderConstraint.SetMethod("void SetLinearSoftnessDirection(float)", &SliderConstraint::SetLinearSoftnessDirection);
					VSliderConstraint.SetMethod("void SetAngularSoftnessLimit(float)", &SliderConstraint::SetAngularSoftnessLimit);
					VSliderConstraint.SetMethod("void SetLinearSoftnessLimit(float)", &SliderConstraint::SetLinearSoftnessLimit);
					VSliderConstraint.SetMethod("void SetAngularSoftnessOrtho(float)", &SliderConstraint::SetAngularSoftnessOrtho);
					VSliderConstraint.SetMethod("void SetLinearSoftnessOrtho(float)", &SliderConstraint::SetLinearSoftnessOrtho);
					VSliderConstraint.SetMethod("void SetPoweredAngularMotor(bool)", &SliderConstraint::SetPoweredAngularMotor);
					VSliderConstraint.SetMethod("void SetPoweredLinearMotor(bool)", &SliderConstraint::SetPoweredLinearMotor);
					VSliderConstraint.SetMethod("void SetEnabled(bool)", &SliderConstraint::SetEnabled);
					VSliderConstraint.SetMethodEx("RigidBody@+ GetFirst()", &SliderConstraint_GetFirst);
					VSliderConstraint.SetMethodEx("RigidBody@+ GetSecond()", &SliderConstraint_GetSecond);
					VSliderConstraint.SetMethod("float GetAngularMotorVelocity()", &SliderConstraint::GetAngularMotorVelocity);
					VSliderConstraint.SetMethod("float GetLinearMotorVelocity()", &SliderConstraint::GetLinearMotorVelocity);
					VSliderConstraint.SetMethod("float GetUpperLinearLimit()", &SliderConstraint::GetUpperLinearLimit);
					VSliderConstraint.SetMethod("float GetLowerLinearLimit()", &SliderConstraint::GetLowerLinearLimit);
					VSliderConstraint.SetMethod("float GetBreakingImpulseThreshold()", &SliderConstraint::GetBreakingImpulseThreshold);
					VSliderConstraint.SetMethod("float GetAngularDampingDirection()", &SliderConstraint::GetAngularDampingDirection);
					VSliderConstraint.SetMethod("float GetLinearDampingDirection()", &SliderConstraint::GetLinearDampingDirection);
					VSliderConstraint.SetMethod("float GetAngularDampingLimit()", &SliderConstraint::GetAngularDampingLimit);
					VSliderConstraint.SetMethod("float GetLinearDampingLimit()", &SliderConstraint::GetLinearDampingLimit);
					VSliderConstraint.SetMethod("float GetAngularDampingOrtho()", &SliderConstraint::GetAngularDampingOrtho);
					VSliderConstraint.SetMethod("float GetLinearDampingOrtho()", &SliderConstraint::GetLinearDampingOrtho);
					VSliderConstraint.SetMethod("float GetUpperAngularLimit()", &SliderConstraint::GetUpperAngularLimit);
					VSliderConstraint.SetMethod("float GetLowerAngularLimit()", &SliderConstraint::GetLowerAngularLimit);
					VSliderConstraint.SetMethod("float GetMaxAngularMotorForce()", &SliderConstraint::GetMaxAngularMotorForce);
					VSliderConstraint.SetMethod("float GetMaxLinearMotorForce()", &SliderConstraint::GetMaxLinearMotorForce);
					VSliderConstraint.SetMethod("float GetAngularRestitutionDirection()", &SliderConstraint::GetAngularRestitutionDirection);
					VSliderConstraint.SetMethod("float GetLinearRestitutionDirection()", &SliderConstraint::GetLinearRestitutionDirection);
					VSliderConstraint.SetMethod("float GetAngularRestitutionLimit()", &SliderConstraint::GetAngularRestitutionLimit);
					VSliderConstraint.SetMethod("float GetLinearRestitutionLimit()", &SliderConstraint::GetLinearRestitutionLimit);
					VSliderConstraint.SetMethod("float GetAngularRestitutionOrtho()", &SliderConstraint::GetAngularRestitutionOrtho);
					VSliderConstraint.SetMethod("float GetLinearRestitutionOrtho()", &SliderConstraint::GetLinearRestitutionOrtho);
					VSliderConstraint.SetMethod("float GetAngularSoftnessDirection()", &SliderConstraint::GetAngularSoftnessDirection);
					VSliderConstraint.SetMethod("float GetLinearSoftnessDirection()", &SliderConstraint::GetLinearSoftnessDirection);
					VSliderConstraint.SetMethod("float GetAngularSoftnessLimit()", &SliderConstraint::GetAngularSoftnessLimit);
					VSliderConstraint.SetMethod("float GetLinearSoftnessLimit()", &SliderConstraint::GetLinearSoftnessLimit);
					VSliderConstraint.SetMethod("float GetAngularSoftnessOrtho()", &SliderConstraint::GetAngularSoftnessOrtho);
					VSliderConstraint.SetMethod("float GetLinearSoftnessOrtho()", &SliderConstraint::GetLinearSoftnessOrtho);
					VSliderConstraint.SetMethod("bool GetPoweredAngularMotor()", &SliderConstraint::GetPoweredAngularMotor);
					VSliderConstraint.SetMethod("bool GetPoweredLinearMotor()", &SliderConstraint::GetPoweredLinearMotor);
					VSliderConstraint.SetMethod("bool IsConnected()", &SliderConstraint::IsConnected);
					VSliderConstraint.SetMethod("bool IsEnabled()", &SliderConstraint::IsEnabled);
					VSliderConstraint.SetMethod("SliderConstraintDesc& GetInitialState()", &SliderConstraint::GetInitialState);

					VSimulatorDesc.SetProperty("bool EnableSoftBody", &Simulator::Desc::EnableSoftBody);
					VSimulatorDesc.SetProperty("float AirDensity", &Simulator::Desc::AirDensity);
					VSimulatorDesc.SetProperty("float WaterDensity", &Simulator::Desc::WaterDensity);
					VSimulatorDesc.SetProperty("float WaterOffset", &Simulator::Desc::WaterOffset);
					VSimulatorDesc.SetProperty("float MaxDisplacement", &Simulator::Desc::MaxDisplacement);
					VSimulatorDesc.SetProperty("Vector3 WaterNormal", &Simulator::Desc::WaterNormal);

					VSimulator.SetProperty<Simulator>("float TimeSpeed", &Simulator::TimeSpeed);
					VSimulator.SetProperty<Simulator>("int Interpolate", &Simulator::Interpolate);
					VSimulator.SetProperty<Simulator>("bool Active", &Simulator::Active);
					VSimulator.SetUnmanagedConstructor<Simulator, const Simulator::Desc&>("Simulator@ f(const SimulatorDesc &in)");
					VSimulator.SetMethod("void SetGravity(const Vector3 &in)", &Simulator::SetGravity);
					VSimulator.SetMethod<Simulator, void, const Vector3&, bool>("void SetLinearImpulse(const Vector3 &in, bool)", &Simulator::SetLinearImpulse);
					VSimulator.SetMethod<Simulator, void, const Vector3&, bool>("void SetAngularImpulse(const Vector3 &in, bool)", &Simulator::SetAngularImpulse);
					VSimulator.SetMethod<Simulator, void, const Vector3&, bool>("void CreateLinearImpulse(const Vector3 &in, bool)", &Simulator::CreateLinearImpulse);
					VSimulator.SetMethod<Simulator, void, const Vector3&, bool>("void CreateAngularImpulse(const Vector3 &in, bool)", &Simulator::CreateAngularImpulse);
					VSimulator.SetMethod("void AddSoftBody(SoftBody@+)", &Simulator::AddSoftBody);
					VSimulator.SetMethod("void RemoveSoftBody(SoftBody@+)", &Simulator::RemoveSoftBody);
					VSimulator.SetMethod("void AddRigidBody(RigidBody@+)", &Simulator::AddRigidBody);
					VSimulator.SetMethod("void RemoveRigidBody(RigidBody@+)", &Simulator::RemoveRigidBody);
					VSimulator.SetMethod("void AddSliderConstraint(SliderConstraint@+)", &Simulator::AddSliderConstraint);
					VSimulator.SetMethod("void RemoveSliderConstraint(SliderConstraint@+)", &Simulator::RemoveSliderConstraint);
					VSimulator.SetMethod("void RemoveAll()", &Simulator::RemoveAll);
					VSimulator.SetMethod("void Simulate(float)", &Simulator::Simulate);
					VSimulator.SetMethod<Simulator, RigidBody*, const RigidBody::Desc&>("RigidBody@+ CreateRigidBody(const RigidBodyDesc &in)", &Simulator::CreateRigidBody);
					VSimulator.SetMethod<Simulator, SoftBody*, const SoftBody::Desc&>("SoftBody@+ CreateSoftBody(const SoftBodyDesc &in)", &Simulator::CreateSoftBody);
					VSimulator.SetMethod("SliderConstraint@+ CreateSliderConstraint(const SliderConstraintDesc &in)", &Simulator::CreateSliderConstraint);
					VSimulator.SetMethod("float GetMaxDisplacement()", &Simulator::GetMaxDisplacement);
					VSimulator.SetMethod("float GetAirDensity()", &Simulator::GetAirDensity);
					VSimulator.SetMethod("float GetWaterOffset()", &Simulator::GetWaterOffset);
					VSimulator.SetMethod("float GetWaterDensity()", &Simulator::GetWaterDensity);
					VSimulator.SetMethod("Vector3 GetWaterNormal()", &Simulator::GetWaterNormal);
					VSimulator.SetMethod("Vector3 GetGravity()", &Simulator::GetGravity);
					VSimulator.SetMethod("bool HasSoftBodySupport()", &Simulator::HasSoftBodySupport);
					VSimulator.SetMethod("int GetContactManifoldCount()", &Simulator::GetContactManifoldCount);
					VSimulator.SetMethodEx("void InitShape(RigidBodyDesc &in, Shape)", &Simulator_InitShape);
					VSimulator.SetMethodEx("void InitCube(RigidBodyDesc &in, const Vector3 &in)", &Simulator_InitCube);
					VSimulator.SetMethodEx("void InitSphere(RigidBodyDesc &in, float)", &Simulator_InitSphere);
					VSimulator.SetMethodEx("void InitCapsule(RigidBodyDesc &in, float, float)", &Simulator_InitCapsule);
					VSimulator.SetMethodEx("void InitCone(RigidBodyDesc &in, float, float)", &Simulator_InitCone);
					VSimulator.SetMethodEx("void InitCylinder(RigidBodyDesc &in, const Vector3 &in)", &Simulator_InitCylinder);
					VSimulator.SetMethodEx("void InitConvex(RigidBodyDesc &in, array<Vector3>@+)", &Simulator_InitConvex);

					return 0;
				});
				Manager->Namespace("Compute::Regex", [](VMGlobal* Global)
				{
					Global->SetFunction("bool Match(RegExp &in, RegexResult &in, const string &in)", &Regex_Match);
					Global->SetFunction("bool MatchAll(RegExp &in, Compute::RegexResult &in, const string &in)", &Regex_MatchAll);
					Global->SetFunction("RegExp Create(const string &in, RegexFlags, int64, int64, int64)", &Regex::Create);
					Global->SetFunction("string Syntax()", &Regex_Syntax);

					return 0;
				});
				Manager->Namespace("Compute::Math", [](VMGlobal* Global)
				{
					Global->SetFunction("double Rad2Deg()", &Math<double>::Rad2Deg);
					Global->SetFunction("double Deg2Rad()", &Math<double>::Deg2Rad);
					Global->SetFunction("double Pi()", &Math<double>::Pi);
					Global->SetFunction("double Sqrt(double)", &Math<double>::Sqrt);
					Global->SetFunction("double Abs(double)", &Math<double>::Abs);
					Global->SetFunction("double ATan(double)", &Math<double>::ATan);
					Global->SetFunction("double ATan2(double, double)", &Math<double>::ATan2);
					Global->SetFunction("double ACos(double)", &Math<double>::ACos);
					Global->SetFunction("double ASin(double)", &Math<double>::ASin);
					Global->SetFunction("double Cos(double)", &Math<double>::Cos);
					Global->SetFunction("double Sin(double)", &Math<double>::Sin);
					Global->SetFunction("double Tan(double)", &Math<double>::Tan);
					Global->SetFunction("double ACotan(double)", &Math<double>::ACotan);
					Global->SetFunction("double Max(double, double)", &Math<double>::Max);
					Global->SetFunction("double Min(double, double)", &Math<double>::Min);
					Global->SetFunction("double Floor(double)", &Math<double>::Floor);
					Global->SetFunction("double Lerp(double, double, double)", &Math<double>::Lerp);
					Global->SetFunction("double StrongLerp(double, double, double)", &Math<double>::StrongLerp);
					Global->SetFunction("double SaturateAngle(double)", &Math<double>::SaturateAngle);
					Global->SetFunction("double AngluarLerp(double, double, double)", &Math<double>::AngluarLerp);
					Global->SetFunction("double AngleDistance(double, double)", &Math<double>::AngleDistance);
					Global->SetFunction("double Saturate(double)", &Math<double>::Saturate);
					Global->SetFunction("double Round(double)", &Math<double>::Round);
					Global->SetFunction("double RandomMag()", &Math<double>::RandomMag);
					Global->SetFunction("double Pow(double, double)", &Math<double>::Pow);
					Global->SetFunction("double Pow2(double)", &Math<double>::Pow2);
					Global->SetFunction("double Pow3(double)", &Math<double>::Pow3);
					Global->SetFunction("double Pow4(double)", &Math<double>::Pow4);
					Global->SetFunction("double Clamp(double, double)", &Math<double>::Clamp);
					Global->SetFunction("double Select(double, double)", &Math<double>::Select);
					Global->SetFunction("double Cotan(double)", &Math<double>::Cotan);
					Global->SetFunction("double Swap(double &in, double &in)", &Math<double>::Swap);
					Global->SetFunction<double(double, double)>("double Random(double, double)", &Math<double>::Random);
					Global->SetFunction<double()>("double Random()", &Math<double>::Random);

					return 0;
				});
				Manager->Namespace("Compute::Common", [](VMGlobal* Global)
				{
					Global->SetFunction("string Base10ToBaseN(int, uint)", &MathCommon::Base10ToBaseN);
					Global->SetFunction<std::string(const std::string&)>("string URIEncode(const string &in)", &MathCommon::URIEncode);
					Global->SetFunction<std::string(const std::string&)>("string URIDecode(const string &in)", &MathCommon::URIDecode);
					Global->SetFunction("string Encrypt(const string &in, int)", &MathCommon::Encrypt);
					Global->SetFunction("string Decrypt(const string &in, int)", &MathCommon::Decrypt);
					Global->SetFunction("float IsClipping(Matrix4x4, Matrix4x4, float)", &MathCommon::IsClipping);
					Global->SetFunction("bool HasSphereIntersected(Vector3, float, Vector3, float)", &MathCommon::HasSphereIntersected);
					Global->SetFunction("bool HasLineIntersected(float, float, Vector3, Vector3, Vector3 &in)", &MathCommon::HasLineIntersected);
					Global->SetFunction("bool HasLineIntersectedCube(Vector3, Vector3, Vector3, Vector3)", &MathCommon::HasLineIntersectedCube);
					Global->SetFunction<bool(Vector3, Vector3, Vector3)>("bool HasPointIntersectedCube(Vector3, Vector3, Vector3, int)", &MathCommon::HasPointIntersectedCube);
					Global->SetFunction("bool HasPointIntersectedRectangle(Vector3, Vector3, Vector3)", &MathCommon::HasPointIntersectedRectangle);
					Global->SetFunction("bool HasSBIntersected(Transform@+, Transform@+)", &MathCommon::HasSBIntersected);
					Global->SetFunction("bool HasOBBIntersected(Transform@+, Transform@+)", &MathCommon::HasOBBIntersected);
					Global->SetFunction("bool HasAABBIntersected(Transform@+, Transform@+)", &MathCommon::HasAABBIntersected);
					Global->SetFunction("bool IsBase64(uint8)", &MathCommon::IsBase64);
					Global->SetFunction("bool Hex(int8, int &in)", &MathCommon::Hex);
					Global->SetFunction("bool HexToDecimal(const string &in, int, int, int &in)", &MathCommon::HexToDecimal);
					Global->SetFunction("void Randomize()", &MathCommon::Randomize);
					Global->SetFunction("string RandomBytes(int)", &MathCommon::RandomBytes);
					Global->SetFunction("string MD5Hash(const string &in)", &MathCommon::MD5Hash);
					Global->SetFunction("string Sha256Encode(const string &in, const string &in, const string &in)", &MathCommon_Sha256Encode);
					Global->SetFunction("string Sha256Decode(const string &in, const string &in, const string &in)", &MathCommon_Sha256Decode);
					Global->SetFunction("string Aes256Encode(const string &in, const string &in, const string &in)", &MathCommon_Aes256Encode);
					Global->SetFunction("string Aes256Decode(const string &in, const string &in, const string &in)", &MathCommon_Aes256Decode);
					Global->SetFunction<std::string(const std::string&)>("string Base64Encode(const string &in, int)", &MathCommon::Base64Encode);
					Global->SetFunction("string Base64Decode(const string &in)", &MathCommon::Base64Decode);
					Global->SetFunction("string Hybi10Encode(Hybi10Request, bool)", &MathCommon::Hybi10Encode);
					Global->SetFunction("string DecimalToHex(int)", &MathCommon::DecimalToHex);
					Global->SetFunction("Hybi10Request Hybi10Decode(const string &in)", &MathCommon::Hybi10Decode);
					Global->SetFunction("uint8 RandomUC()", &MathCommon::RandomUC);
					Global->SetFunction("int RandomNumber(int, int)", &MathCommon::RandomNumber);
					Global->SetFunction("Ray CreateCursorRay(const Vector3 &in, const Vector2 &in, const Vector2 &in, const Matrix4x4 &in, const Matrix4x4 &in)", &MathCommon::CreateCursorRay);
					Global->SetFunction<bool(const Ray&, const Vector3&, const Vector3&)>("bool CursorRayTest(const Ray &in, const Vector3 &in, const Vector3 &in)", &MathCommon::CursorRayTest);
					Global->SetFunction<bool(Ray&, const Matrix4x4&)>("bool CursorRayTest(Ray &in, const Matrix4x4 &in)", &MathCommon::CursorRayTest);

					return 0;
				});
			}
			void EnableNetwork(VMManager* Manager)
			{
				Manager->Namespace("Network", [](VMGlobal* Global)
				{
					Script::VMWEnum VSecure = Global->SetEnum("Secure");
					Script::VMWEnum VServerState = Global->SetEnum("ServerState");
					Script::VMWEnum VSocketEvent = Global->SetEnum("SocketEvent");
					Script::VMWEnum VSocketType = Global->SetEnum("SocketType");
					Script::VMWTypeClass VAddress = Global->SetStructUnmanaged<Address>("Address");
					//Script::VMWTypeClass VSocket = Global->SetStructUnmanaged<Socket>("Socket");
					Script::VMWTypeClass VHost = Global->SetStructUnmanaged<Host>("Host");

					VSecure.SetValue("Any", Secure_Any);
					VSecure.SetValue("SSLV2", Secure_SSL_V2);
					VSecure.SetValue("SSLV3", Secure_SSL_V3);
					VSecure.SetValue("TLSV1", Secure_TLS_V1);
					VSecure.SetValue("TLSV11", Secure_TLS_V1_1);

					VServerState.SetValue("Working", ServerState_Working);
					VServerState.SetValue("Stopping", ServerState_Stopping);
					VServerState.SetValue("Idle", ServerState_Idle);

					VSocketEvent.SetValue("Read", SocketEvent_Read);
					VSocketEvent.SetValue("Write", SocketEvent_Write);
					VSocketEvent.SetValue("Close", SocketEvent_Close);
					VSocketEvent.SetValue("Timeout", SocketEvent_Timeout);
					VSocketEvent.SetValue("None", SocketEvent_None);

					VSocketType.SetValue("Stream", SocketType_Stream);
					VSocketType.SetValue("Datagram", SocketType_Datagram);
					VSocketType.SetValue("Raw", SocketType_Raw);
					VSocketType.SetValue("ReliablyDeliveredMessage", SocketType_Reliably_Delivered_Message);
					VSocketType.SetValue("SequencePacketStream", SocketType_Sequence_Packet_Stream);

					VAddress.SetProperty("bool Resolved", &Address::Resolved);
					VAddress.SetConstructor<Address>("void f()");
					VAddress.SetMethodStatic("bool Free(Network::Address@)", &Address::Free);
					/*
					VSocket.SetFunctionDef("bool AcceptCallback(Socket@, any@)");
					VSocket.SetFunctionDef("bool WriteCallback(Socket@, any@, int64)");
					VSocket.SetFunctionDef("bool ReadCallback(Socket@, any@, const string &in, int64)");
					VSocket.SetProperty("int Income", &Socket::Income);
					VSocket.SetProperty("int Outcome", &Socket::Outcome);
					VSocket.SetConstructor<Socket>("void f()");
					VSocket.SetMethod("int Bind(Address@)", &Socket::Bind);
					VSocket.SetMethod("int Connect(Address@)", &Socket::Connect);
					VSocket.SetMethod("int Listen(int)", &Socket::Listen);
					VSocket.SetMethod("int Accept(Socket@, Address@)", &Socket::Accept);
					VSocket.SetMethod("int Close(bool)", &Socket::Close);
					VSocket.SetMethod("int CloseOnExec()", &Socket::CloseOnExec);
					VSocket.SetMethod("int Skip(int, int)", &Socket::Skip);
					VSocket.SetMethod("int Clear()", &Socket::Clear);
					VSocket.SetMethod("int SetTimeWait(int)", &Socket::SetTimeWait);
					VSocket.SetMethod("int SetBlocking(bool)", &Socket::SetBlocking);
					VSocket.SetMethod("int SetNodelay(bool)", &Socket::SetNodelay);
					VSocket.SetMethod("int SetKeepAlive(bool)", &Socket::SetKeepAlive);
					VSocket.SetMethod("int SetTimeout(int)", &Socket::SetTimeout);
					VSocket.SetMethod("int SetAsyncTimeout(int)", &Socket::SetAsyncTimeout);
					VSocket.SetMethod("int GetError(int)", &Socket::GetError);
					VSocket.SetMethod("int GetPort()", &Socket::GetPort);
					VSocket.SetMethod("socket_t GetFd()", &Socket::GetFd);
					VSocket.SetMethod("bool IsValid()", &Socket::IsValid);
					VSocket.SetMethod("bool IsAwaiting()", &Socket::IsAwaiting);
					VSocket.SetMethod("bool HasIncomingData()", &Socket::HasIncomingData);
					VSocket.SetMethod("bool HasOutcomingData()", &Socket::HasOutcomingData);
					VSocket.SetMethod("bool HasPendingData()", &Socket::HasPendingData);
					VSocket.SetMethod("string GetRemoteAddress()", &Socket::GetRemoteAddress);
					VSocket.SetMethod("int GetAsyncTimeout()", &Socket::GetAsyncTimeout);
					VSocket.SetMethodEx("int AcceptAsync(any@, Socket::AcceptCallback@)", &Socket_AcceptAsync);
					VSocket.SetMethodEx("int Open(const string &in, int, SocketType, Address@)", &Socket_Open);
					VSocket.SetMethodEx("int Write(const string &in)", &Socket_Write);
					VSocket.SetMethodEx("int WriteAsync(const string &in, any@, Socket::WriteCallback@)", &Socket_WriteAsync);
					VSocket.SetMethodEx("int Read(string &out, int)", &Socket_Read);
					VSocket.SetMethodEx("int ReadAsync(int, any@, Socket::ReadCallback@)", &Socket_ReadAsync);
					VSocket.SetMethodEx("int ReadUntil(const string &in, any@, Socket::ReadCallback@)", &Socket_ReadUntil);
					VSocket.SetMethodEx("int ReadUntilAsync(const string &in, any@, Socket::ReadCallback@)", &Socket_ReadUntilAsync);
					VSocket.SetMethodStatic("string LocalIpAddress()", &Socket::LocalIpAddress);
					*/
					VHost.SetProperty("string Hostname", &Host::Hostname);
					VHost.SetProperty("int Port", &Host::Port);
					VHost.SetProperty("bool Secure", &Host::Secure);
					VHost.SetConstructor<Host>("void f()");

					/*
						TODO: Register types for VM from Tomahawk::Network
							ref Host;
							ref Listener;
							ref Certificate;
							ref DataFrame;
							ref SocketCertificate;
							ref SocketConnection;
							ref SocketRouter;
							ref Multiplexer;
							ref SocketServer;
							ref SocketClient;
					*/

					return 0;
				});
			}
			void EnableHTTP(VMManager* Manager)
			{
				Manager->Namespace("HTTP", [](VMGlobal* Global)
				{
					/*
						TODO: Register types for VM from Tomahawk::Network::HTTP
							enum Auth;
							enum Content;
							enum QueryValue;
							enum WebSocketOp;
							enum WebSocketState;
							enum CompressionTune;
							ref GatewayFile;
							ref ErrorFile;
							ref MimeType;
							ref MimeStatic;
							ref Credentials;
							ref Header;
							ref Resource;
							ref Cookie;
							ref RequestFrame;
							ref ResponseFrame;
							ref WebSocketFrame;
							ref GatewayFrame;
							ref ParserFrame;
							ref QueryToken;
							ref RouteEntry;
							ref SiteEntry;
							ref MapRouter;
							ref Connection;
							ref QueryParameter;
							ref Query;
							ref Session;
							ref Parser;
							ref Util;
							ref Server;
							ref Client;
					*/

					return 0;
				});
			}
			void EnableSMTP(VMManager* Manager)
			{
				Manager->Namespace("SMTP", [](VMGlobal* Global)
				{
					/*
						TODO: Register types for VM from Tomahawk::Network::SMTP
							enum Priority;
							ref Recipient;
							ref Attachment;
							ref RequestFrame;
							ref Client;
					*/

					return 0;
				});
			}
			void EnableBSON(VMManager* Manager)
			{
				Manager->Namespace("BSON", [](VMGlobal* Global)
				{
					/*
						TODO: Register types for VM from Tomahawk::Network::BSON
							enum Type;
							ref KeyPair;
							ref Document;
					*/

					return 0;
				});
			}
			void EnableMongoDB(VMManager* Manager)
			{
				Manager->Namespace("MongoDB", [](VMGlobal* Global)
				{
					/*
						TODO: Register types for VM from Tomahawk::Network::MongoDB
							enum ReadMode;
							enum QueryMode;
							enum InsertMode;
							enum UpdateMode;
							enum RemoveMode;
							enum FindAndModifyMode;
							ref HostList;
							ref SSLOptions;
							ref APMCommandStarted;
							ref APMCommandSucceeded;
							ref APMCommandFailed;
							ref APMServerChanged;
							ref APMServerOpened;
							ref APMServerClosed;
							ref APMTopologyChanged;
							ref APMTopologyOpened;
							ref APMTopologyClosed;
							ref APMServerHeartbeatStarted;
							ref APMServerHeartbeatSucceeded;
							ref APMServerHeartbeatFailed;
							ref APMCallbacks;
							namespace Connector;
							ref URI;
							ref FindAndModifyOptions;
							ref ReadConcern;
							ref WriteConcern;
							ref ReadPreferences;
							ref BulkOperation;
							ref ChangeStream;
							ref Cursor;
							ref Collection;
							ref Database;
							ref ServerDescription;
							ref TopologyDescription;
							ref Client;
							ref ClientPool;
					*/

					return 0;
				});
			}
			void EnableAudio(VMManager* Manager)
			{
				Manager->Namespace("Audio", [](VMGlobal* Global)
				{
					/*
						TODO: Register types for VM from Tomahawk::Audio
							enum SoundDistanceModel;
							enum SoundEx;
							namespace AudioContext;
							ref AudioClip;
							ref AudioSource;
							ref AudioDevice;
					*/

					return 0;
				});
			}
			void EnableEffects(VMManager* Manager)
			{
				Manager->Namespace("Effects", [](VMGlobal* Global)
				{
					/*
						TODO: Register types for VM from Tomahawk::Audio::Effects
					*/

					return 0;
				});
			}
			void EnableFilters(VMManager* Manager)
			{
				Manager->Namespace("Filters", [](VMGlobal* Global)
				{
					/*
						TODO: Register types for VM from Tomahawk::Audio::Filters
					*/

					return 0;
				});
			}
			void EnableGraphics(VMManager* Manager)
			{
				Manager->Namespace("Graphics", [](VMGlobal* Global)
				{
					/*
						TODO: Register types for VM from Tomahawk::Graphics

					*/

					return 0;
				});
			}
			void EnableEngine(VMManager* Manager)
			{
				Manager->Namespace("Engine", [](VMGlobal* Global)
				{
					/*
						TODO: Register types for VM from Tomahawk::Engine

					*/

					return 0;
				});
			}
		}
	}
}