#include "rest.h"
#include "../network/bson.h"

namespace Tomahawk
{
    namespace Wrapper
    {
        namespace Rest
        {
			using namespace Tomahawk::Rest;

			static bool FileStream_Open(FileStream* Base, const std::string& Path, FileMode Mode)
			{
				return Base->Open(Path.c_str(), Mode);
			}
			static bool FileStream_OpenZ(FileStream* Base, const std::string& Path, FileMode Mode)
			{
				return Base->OpenZ(Path.c_str(), Mode);
			}
			static UInt64 FileStream_Write(FileStream* Base, const std::string& Buffer)
			{
				return Base->Write(Buffer.c_str(), (UInt64)Buffer.size());
			}
			static std::string FileStream_Read(FileStream* Base, UInt64 Length)
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

			static bool EventQueue_Task(EventQueue* Base, Script::VMCAny* Any, Script::VMCFunction* Callback, bool Keep)
			{
				if (!Callback)
					return false;

				if (Any != nullptr)
					Script::VMWAny(Any).AddRef();

				Script::VMWFunction(Callback).AddRef();
				return Base->Task<void>(Any, [Callback](EventQueue* Queue, EventArgs* Args)
				{
					Script::VMContext* Context = Script::VMContext::Get();
					if (!Context)
					{
						Script::VMWAny(Args->Get<Script::VMCAny>()).Release();
						Script::VMWFunction(Callback).Release();
						Args->Data = nullptr;
						Args->Alive = false;
						return;
					}

					Context->PushState();
					Context->Prepare(Callback);
					Context->SetArgObject(0, Queue);
					Context->SetArgObject(1, Args->Get<Script::VMCAny>());
					Context->Execute();

					if (!Context->GetReturnByte())
					{
						Script::VMWAny(Args->Get<Script::VMCAny>()).Release();
						Script::VMWFunction(Callback).Release();
						Args->Data = nullptr;
						Args->Alive = false;
					}

					Context->PopState();
				}, Keep);
			}
			static bool EventQueue_Callback(EventQueue* Base, Script::VMCAny* Any, Script::VMCFunction* Callback, bool Keep)
			{
				if (!Callback)
					return false;

				if (Any != nullptr)
					Script::VMWAny(Any).AddRef();

				Script::VMWFunction(Callback).AddRef();
				return Base->Callback<void>(Any, [Callback](EventQueue* Queue, EventArgs* Args)
				{
					Script::VMContext* Context = Script::VMContext::Get();
					if (!Context)
					{
						Script::VMWAny(Args->Get<Script::VMCAny>()).Release();
						Script::VMWFunction(Callback).Release();
						Args->Data = nullptr;
						Args->Alive = false;
						return;
					}

					Context->PushState();
					Context->Prepare(Callback);
					Context->SetArgObject(0, Queue);
					Context->SetArgObject(1, Args->Get<Script::VMCAny>());
					Context->Execute();

					if (!Context->GetReturnByte())
					{
						Script::VMWAny(Args->Get<Script::VMCAny>()).Release();
						Script::VMWFunction(Callback).Release();
						Args->Data = nullptr;
						Args->Alive = false;
					}

					Context->PopState();
				}, Keep);
			}
			static bool EventQueue_Interval(EventQueue* Base, Script::VMCAny* Any, UInt64 Ms, Script::VMCFunction* Callback)
			{
				if (!Callback)
					return false;

				if (Any != nullptr)
					Script::VMWAny(Any).AddRef();

				Script::VMWFunction(Callback).AddRef();
				return Base->Interval<void>(Any, Ms, [Callback](EventQueue* Queue, EventArgs* Args)
				{
					Script::VMContext* Context = Script::VMContext::Get();
					if (!Context)
					{
						Script::VMWAny(Args->Get<Script::VMCAny>()).Release();
						Script::VMWFunction(Callback).Release();
						Args->Data = nullptr;
						Args->Alive = false;
						return;
					}

					Context->PushState();
					Context->Prepare(Callback);
					Context->SetArgObject(0, Queue);
					Context->SetArgObject(1, Args->Get<Script::VMCAny>());
					Context->Execute();

					if (!Context->GetReturnByte())
					{
						Script::VMWAny(Args->Get<Script::VMCAny>()).Release();
						Script::VMWFunction(Callback).Release();
						Args->Data = nullptr;
						Args->Alive = false;
					}

					Context->PopState();
				});
			}
			static bool EventQueue_Timeout(EventQueue* Base, Script::VMCAny* Any, UInt64 Ms, Script::VMCFunction* Callback)
			{
				if (!Callback)
					return false;

				if (Any != nullptr)
					Script::VMWAny(Any).AddRef();

				Script::VMWFunction(Callback).AddRef();
				return Base->Timeout<void>(Any, Ms, [Callback](EventQueue* Queue, EventArgs* Args)
				{
					Script::VMContext* Context = Script::VMContext::Get();
					if (!Context)
					{
						Script::VMWAny(Args->Get<Script::VMCAny>()).Release();
						Script::VMWFunction(Callback).Release();
						Args->Data = nullptr;
						Args->Alive = false;
						return;
					}

					Context->PushState();
					Context->Prepare(Callback);
					Context->SetArgObject(0, Queue);
					Context->SetArgObject(1, Args->Get<Script::VMCAny>());
					Context->Execute();

					if (!Context->GetReturnByte())
					{
						Script::VMWAny(Args->Get<Script::VMCAny>()).Release();
						Script::VMWFunction(Callback).Release();
						Args->Data = nullptr;
						Args->Alive = false;
					}

					Context->PopState();
				});
			}

			static Document* Document_SetDocument_1(Document* Base, const std::string& Name, Document* Value)
			{
				Document* Result = Base->SetDocument(Name, Value);
				Factory::AddRef(Result);

				return Result;
			}
			static Document* Document_SetDocument_2(Document* Base, const std::string& Name)
			{
				Document* Result = Base->SetDocument(Name);
				Factory::AddRef(Result);

				return Result;
			}
			static Document* Document_SetArray_1(Document* Base, const std::string& Name, Document* Value)
			{
				Document* Result = Base->SetArray(Name, Value);
				Factory::AddRef(Result);

				return Result;
			}
			static Document* Document_SetArray_2(Document* Base, const std::string& Name)
			{
				Document* Result = Base->SetArray(Name);
				Factory::AddRef(Result);

				return Result;
			}
			static Document* Document_SetDecimal_1(Document* Base, const std::string& Name, UInt64 High, UInt64 Low)
			{
				Document* Result = Base->SetDecimal(Name, High, Low);
				Factory::AddRef(Result);

				return Result;
			}
			static Document* Document_SetDecimal_2(Document* Base, const std::string& Name, const std::string& Value)
			{
				Document* Result = Base->SetDecimal(Name, Value);
				Factory::AddRef(Result);

				return Result;
			}
			static Document* Document_SetString(Document* Base, const std::string& Name, const std::string& Value)
			{
				Document* Result = Base->SetString(Name, Value);
				Factory::AddRef(Result);

				return Result;
			}
			static Document* Document_SetId(Document* Base, const std::string& Name, const std::string& Value)
			{
				if (Value.size() != 12)
					return nullptr;

				Document* Result = Base->SetId(Name, (unsigned char*)Value.c_str());
				Factory::AddRef(Result);

				return Result;
			}
			static Document* Document_GetIndex(Document* Base, Int64 Index)
			{
				Document* Result = Base->GetIndex(Index);
				Factory::AddRef(Result);

				return Result;
			}
			static Document* Document_Get(Document* Base, const std::string& Name)
			{
				Document* Result = Base->Get(Name);
				Factory::AddRef(Result);

				return Result;
			}
			static Document* Document_SetCast(Document* Base, const std::string& Name, const std::string& Prop)
			{
				Document* Result = Base->SetCast(Name, Prop);
				Factory::AddRef(Result);

				return Result;
			}
			static Document* Document_SetUndefined(Document* Base, const std::string& Name)
			{
				Document* Result = Base->SetUndefined(Name);
				Factory::AddRef(Result);

				return Result;
			}
			static Document* Document_SetNull(Document* Base, const std::string& Name)
			{
				Document* Result = Base->SetNull(Name);
				Factory::AddRef(Result);

				return Result;
			}
			static Document* Document_SetAttribute(Document* Base, const std::string& Name, const std::string& Value)
			{
				Document* Result = Base->SetAttribute(Name, Value);
				Factory::AddRef(Result);

				return Result;
			}
			static Document* Document_SetInteger(Document* Base, const std::string& Name, Int64 Value)
			{
				Document* Result = Base->SetInteger(Name, Value);
				Factory::AddRef(Result);

				return Result;
			}
			static Document* Document_SetNumber(Document* Base, const std::string& Name, Float64 Value)
			{
				Document* Result = Base->SetNumber(Name, Value);
				Factory::AddRef(Result);

				return Result;
			}
			static Document* Document_SetBoolean(Document* Base, const std::string& Name, bool Value)
			{
				Document* Result = Base->SetBoolean(Name, Value);
				Factory::AddRef(Result);

				return Result;
			}
			static Document* Document_GetAttribute(Document* Base, const std::string& Name)
			{
				Document* Result = Base->GetAttribute(Name);
				Factory::AddRef(Result);

				return Result;
			}
			static Document* Document_GetParent(Document* Base)
			{
				Document* Result = Base->GetParent();
				Factory::AddRef(Result);

				return Result;
			}
			static Document* Document_Find(Document* Base, const std::string& Name, bool Here)
			{
				Document* Result = Base->Find(Name, Here);
				Factory::AddRef(Result);

				return Result;
			}
			static Document* Document_FindPath(Document* Base, const std::string& Name, bool Here)
			{
				Document* Result = Base->FindPath(Name, Here);
				Factory::AddRef(Result);

				return Result;
			}
			static std::string Document_GetDecimal(Document* Base, const std::string& Name)
			{
				Network::BSON::KeyPair Key; Int64 Low;
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
			static Script::VMCArray* Document_FindCollection(Document* Base, const std::string& Name, bool Here)
			{
				Script::VMContext* Context = Script::VMContext::Get();
				if (!Context)
					return nullptr;

				Script::VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				std::vector<Document*> Nodes = Base->FindCollection(Name, Here);
				for (auto& Node : Nodes)
					Factory::AddRef(Node);

				Script::VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<Rest::Document>");
				return Script::VMWArray::ComposeFromPointers(Type, Nodes).GetArray();
			}
			static Script::VMCArray* Document_FindCollectionPath(Document* Base, const std::string& Name, bool Here)
			{
				Script::VMContext* Context = Script::VMContext::Get();
				if (!Context)
					return nullptr;

				Script::VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				std::vector<Document*> Nodes = Base->FindCollectionPath(Name, Here);
				for (auto& Node : Nodes)
					Factory::AddRef(Node);

				Script::VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<Rest::Document>");
				return Script::VMWArray::ComposeFromPointers(Type, Nodes).GetArray();
			}
			static Script::VMCArray* Document_GetNodes(Document* Base)
			{
				Script::VMContext* Context = Script::VMContext::Get();
				if (!Context)
					return nullptr;

				Script::VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				std::vector<Document*> Nodes = *Base->GetNodes();
				for (auto& Node : Nodes)
					Factory::AddRef(Node);

				Script::VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<Rest::Document>");
				return Script::VMWArray::ComposeFromPointers(Type, Nodes).GetArray();
			}
			static Script::VMCArray* Document_GetAttributes(Document* Base)
			{
				Script::VMContext* Context = Script::VMContext::Get();
				if (!Context)
					return nullptr;

				Script::VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				std::vector<Document*> Nodes = Base->GetAttributes();
				for (auto& Node : Nodes)
					Factory::AddRef(Node);

				Script::VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<Rest::Document>");
				return Script::VMWArray::ComposeFromPointers(Type, Nodes).GetArray();
			}
			static Script::VMCDictionary* Document_CreateMapping(Document* Base)
			{
				Script::VMContext* Context = Script::VMContext::Get();
				if (!Context)
					return nullptr;

				Script::VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				std::unordered_map<std::string, UInt64> Mapping = Base->CreateMapping();
				Script::VMWDictionary Map = Script::VMWDictionary::Create(Manager);
				
				for (auto& Item : Mapping)
				{
					Int64 V = Item.second;
					Map.Set(Item.first, V);
				}

				return Map.GetDictionary();
			}
			static std::string Document_ToJSON(Document* Base)
			{
				std::string Stream;
				Document::WriteJSON(Base, [&Stream](DocumentPretty, const char* Buffer, Int64 Length)
				{
					if (Buffer != nullptr && Length > 0)
						Stream.append(Buffer, Length);
				});

				return Stream;
			}
			static std::string Document_ToXML(Document* Base)
			{
				std::string Stream;
				Document::WriteXML(Base, [&Stream](DocumentPretty, const char* Buffer, Int64 Length)
				{
					if (Buffer != nullptr && Length > 0)
						Stream.append(Buffer, Length);
				});

				return Stream;
			}
			static Document* Document_FromJSON(const std::string& Value)
			{
				size_t Offset = 0;
				return Document::ReadJSON(Value.size(), [&Value, &Offset](char* Buffer, Int64 Size)
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
				return Document::ReadXML(Value.size(), [&Value, &Offset](char* Buffer, Int64 Size)
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
			static ChildProcess OS_SpawnProcess(const std::string& Path, Script::VMCArray* Array)
			{
				std::vector<std::string> Params;
				Script::VMWArray::DecomposeToObjects(Array, &Params);

				ChildProcess Result;
				OS::SpawnProcess(Path, Params, &Result);
				return Result;
			}
			static Script::VMCArray* OS_ScanFiles(const std::string& Path)
			{
				Script::VMContext* Context = Script::VMContext::Get();
				if (!Context)
					return nullptr;

				Script::VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				std::vector<ResourceEntry> Entries;
				if (!OS::ScanDir(Path, &Entries))
					return nullptr;

				Script::VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<Rest::ResourceEntry>");
				return Script::VMWArray::ComposeFromObjects(Type, Entries).GetArray();
			}
			static Script::VMCArray* OS_ScanDir(const std::string& Path)
			{
				Script::VMContext* Context = Script::VMContext::Get();
				if (!Context)
					return nullptr;

				Script::VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				std::vector<DirectoryEntry> Entries;
				OS::Iterate(Path.c_str(), [&Entries](DirectoryEntry* Entry)
				{
					Entries.push_back(*Entry);
					return true;
				});

				Script::VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<Rest::DirectoryEntry>");
				return Script::VMWArray::ComposeFromObjects(Type, Entries).GetArray();
			}
			static Script::VMCArray* OS_GetDiskDrives()
			{
				Script::VMContext* Context = Script::VMContext::Get();
				if (!Context)
					return nullptr;

				Script::VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				Script::VMWTypeInfo Type = Manager->Global().GetTypeInfoByDecl("array<string>");
				return Script::VMWArray::ComposeFromObjects(Type, OS::GetDiskDrives()).GetArray();
			}

            void Enable(Script::VMManager* Manager)
            {
                Manager->Namespace("Rest", [](Script::VMGlobal* Global)
                {
					Script::VMWEnum VEventState = Global->SetEnum("EventState");
					{
						VEventState.SetValue("Working", EventState_Working);
						VEventState.SetValue("Idle", EventState_Idle);
						VEventState.SetValue("Terminated", EventState_Terminated);
					}
					
					Script::VMWEnum VEventWorkflow = Global->SetEnum("EventWorkflow");
					{
						VEventWorkflow.SetValue("Mixed", EventWorkflow_Mixed);
						VEventWorkflow.SetValue("Singlethreaded", EventWorkflow_Singlethreaded);
						VEventWorkflow.SetValue("Multithreaded", EventWorkflow_Multithreaded);
					}

					Script::VMWEnum VEventType = Global->SetEnum("EventType");
					{
						VEventType.SetValue("Events", EventType_Events);
						VEventType.SetValue("Pull", EventType_Pull);
						VEventType.SetValue("Subscribe", EventType_Subscribe);
						VEventType.SetValue("Unsubscribe", EventType_Unsubscribe);
						VEventType.SetValue("Tasks", EventType_Tasks);
						VEventType.SetValue("Timers", EventType_Timers);
					}

					Script::VMWEnum VNodeType = Global->SetEnum("NodeType");
					{
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
					}

					Script::VMWEnum VDocumentPretty = Global->SetEnum("DocumentPretty");
					{
						VDocumentPretty.SetValue("Dummy", DocumentPretty_Dummy);
						VDocumentPretty.SetValue("TabDecrease", DocumentPretty_Tab_Decrease);
						VDocumentPretty.SetValue("TabIncrease", DocumentPretty_Tab_Increase);
						VDocumentPretty.SetValue("WriteSpace", DocumentPretty_Write_Space);
						VDocumentPretty.SetValue("WriteLine", DocumentPretty_Write_Line);
						VDocumentPretty.SetValue("WriteTab", DocumentPretty_Write_Tab);
					}

					Script::VMWEnum VFileMode = Global->SetEnum("FileMode");
					{
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
					}

					Script::VMWEnum VFileSeek = Global->SetEnum("FileSeek");
					{
						VFileSeek.SetValue("Begin", FileSeek_Begin);
						VFileSeek.SetValue("Current", FileSeek_Current);
						VFileSeek.SetValue("End", FileSeek_End);
					}

					Script::VMWTypeClass VFileState = Global->SetPod<FileState>("FileState");
					{
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
					}

					Script::VMWTypeClass VResource = Global->SetPod<Resource>("Resource");
					{
						VResource.SetProperty("uint64 Size", &Resource::Size);
						VResource.SetProperty("int64 LastModified", &Resource::LastModified);
						VResource.SetProperty("int64 CreationTime", &Resource::CreationTime);
						VResource.SetProperty("bool IsReferenced", &Resource::IsReferenced);
						VResource.SetProperty("bool IsDirectory", &Resource::IsDirectory);
					}

					Script::VMWTypeClass VResourceEntry = Global->SetStructUnmanaged<ResourceEntry>("ResourceEntry");
					{
						VResourceEntry.SetProperty("string Path", &ResourceEntry::Path);
						VResourceEntry.SetProperty("Resource Source", &ResourceEntry::Source);
						VResourceEntry.SetConstructor<ResourceEntry>("void f()");
						VResourceEntry.SetDestructor<ResourceEntry>("void f()");
					}

					Script::VMWTypeClass VDirectoryEntry = Global->SetStructUnmanaged<DirectoryEntry>("DirectoryEntry");
					{
						VDirectoryEntry.SetProperty("string Path", &DirectoryEntry::Path);
						VDirectoryEntry.SetProperty("bool IsDirectory", &DirectoryEntry::IsDirectory);
						VDirectoryEntry.SetProperty("bool IsGood", &DirectoryEntry::IsGood);
						VDirectoryEntry.SetProperty("uint64 Length", &DirectoryEntry::Length);
						VDirectoryEntry.SetConstructor<DirectoryEntry>("void f()");
						VDirectoryEntry.SetDestructor<DirectoryEntry>("void f()");
					}

					Script::VMWTypeClass VChildProcess = Global->SetStructUnmanaged<ChildProcess>("ChildProcess");
					{
						VChildProcess.SetConstructor<ChildProcess>("void f()");
						VChildProcess.SetDestructor<ChildProcess>("void f()");
					}

					Script::VMWTypeClass VDateTime = Global->SetStructUnmanaged<DateTime>("DateTime");
					{
						VDateTime.SetConstructor<DateTime>("void f()");
						VDateTime.SetDestructor<DateTime>("void f()");
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
					}

					Script::VMWTypeClass VTickTimer = Global->SetStructUnmanaged<TickTimer>("TickTimer");
					{
						VTickTimer.SetProperty("double Delay", &TickTimer::Delay);
						VTickTimer.SetConstructor<TickTimer>("void f()");
						VTickTimer.SetDestructor<TickTimer>("void f()");
						VTickTimer.SetMethod("bool OnTickEvent(double)", &TickTimer::OnTickEvent);
						VTickTimer.SetMethod("double GetTime()", &TickTimer::GetTime);
					}

					Script::VMWRefClass VConsole = Global->SetClassUnmanaged<Console>("Console");
					{
						VConsole.SetUnmanagedConstructor<Console>("Console@ f()");
						VConsole.SetMethod("void Hide()", &Console::Hide);
						VConsole.SetMethod("void Show()", &Console::Show);
						VConsole.SetMethod("void Clear()", &Console::Clear);
						VConsole.SetMethod("void Flush()", &Console::Flush);
						VConsole.SetMethod("void FlushWrite()", &Console::FlushWrite);
						VConsole.SetMethod("void CaptureTime()", &Console::CaptureTime);
						VConsole.SetMethod("void WriteLine(const string &in)", &Console::sWriteLine);
						VConsole.SetMethod("void Write(const string &in)", &Console::sWrite);
						VConsole.SetMethod("double GetCapturedTime()", &Console::GetCapturedTime);
						VConsole.SetMethod("string Read(uint64)", &Console::Read);
					}

					Script::VMWRefClass VTimer = Global->SetClassUnmanaged<Timer>("Timer");
					{
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
					}

					Script::VMWRefClass VFileStream = Global->SetClassUnmanaged<FileStream>("FileStream");
					{
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
						VFileStream.SetMethodExternal("bool Open(const string &in, FileMode)", &FileStream_Open);
						VFileStream.SetMethodExternal("bool OpenZ(const string &in, FileMode)", &FileStream_OpenZ);
						VFileStream.SetMethodExternal("uint64 Write(const string &in)", &FileStream_Write);
						VFileStream.SetMethodExternal("string Read(uint64)", &FileStream_Read);
					}

					Script::VMWRefClass VEventQueue = Global->SetClassUnmanaged<EventQueue>("EventQueue");
					{
						VEventQueue.SetFunctionDef("bool BaseCallback(EventQueue@, any@)");
						VEventQueue.SetUnmanagedConstructor<EventQueue>("EventQueue@ f()");
						VEventQueue.SetMethod("void SetIdleTime(uint64)", &EventQueue::SetIdleTime);
						VEventQueue.SetMethod("void SetState(EventWorkflow)", &EventQueue::SetState);
						VEventQueue.SetMethod("bool Tick()", &EventQueue::Tick);
						VEventQueue.SetMethod("bool Start(EventWorkflow, uint64, uint64)", &EventQueue::Start);
						VEventQueue.SetMethod("bool Stop()", &EventQueue::Stop);
						VEventQueue.SetMethod("bool Expire(uint64)", &EventQueue::Expire);
						VEventQueue.SetMethod("EventState GetState()", &EventQueue::GetState);
						VEventQueue.SetMethodExternal("bool Task(any@, Rest::EventQueue::BaseCallback@, bool)", &EventQueue_Task);
						VEventQueue.SetMethodExternal("bool Callback(any@, Rest::EventQueue::BaseCallback@, bool)", &EventQueue_Callback);
						VEventQueue.SetMethodExternal("bool Interval(any@, uint64, Rest::EventQueue::BaseCallback@)", &EventQueue_Interval);
						VEventQueue.SetMethodExternal("bool Timeout(any@, uint64, Rest::EventQueue::BaseCallback@)", &EventQueue_Timeout);
					}

					Script::VMWRefClass VDocument = Global->SetClassUnmanaged<Document>("Document");
					{
						VDocument.SetProperty<Document>("string Name", &Document::Name);
						VDocument.SetProperty<Document>("string String", &Document::String);
						VDocument.SetProperty<Document>("NodeType Type", &Document::Type);
						VDocument.SetProperty<Document>("int64 Low", &Document::Low);
						VDocument.SetProperty<Document>("int64 Integer", &Document::Integer);
						VDocument.SetProperty<Document>("double Number", &Document::Number);
						VDocument.SetProperty<Document>("bool Boolean", &Document::Boolean);
						VDocument.SetProperty<Document>("bool Saved", &Document::Saved);
						VDocument.SetUnmanagedConstructor<Document>("Document@ f()");
						VDocument.SetMethod("void Clear()", &Document::Clear);
						VDocument.SetMethod("void Save()", &Document::Save);
						VDocument.SetMethod("Document@ Copy()", &Document::Copy);
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
						VDocument.SetMethod("string Serialize()", &Document::Serialize);
						VDocument.SetMethodExternal("Document@ GetIndex(uint64)", &Document_GetIndex);
						VDocument.SetMethodExternal("Document@ Get(const string &in)", &Document_Get);
						VDocument.SetMethodExternal("Document@ SetCast(const string &in, const string &in)", &Document_SetCast);
						VDocument.SetMethodExternal("Document@ SetUndefined(const string &in)", &Document_SetUndefined);
						VDocument.SetMethodExternal("Document@ SetNull(const string &in)", &Document_SetNull);
						VDocument.SetMethodExternal("Document@ SetAttribute(const string &in, const string &in)", &Document_SetAttribute);
						VDocument.SetMethodExternal("Document@ SetInteger(const string &in, int64)", &Document_SetInteger);
						VDocument.SetMethodExternal("Document@ SetNumber(const string &in, double)", &Document_SetNumber);
						VDocument.SetMethodExternal("Document@ SetBoolean(const string &in, bool)", &Document_SetBoolean);
						VDocument.SetMethodExternal("Document@ Find(const string &in, bool)", &Document_Find);
						VDocument.SetMethodExternal("Document@ FindPath(const string &in, bool)", &Document_FindPath);
						VDocument.SetMethodExternal("Document@ GetParent()", &Document_GetParent);
						VDocument.SetMethodExternal("Document@ GetAttribute(const string &in)", &Document_GetAttribute);
						VDocument.SetMethodExternal("Document@ SetDocument(const string &in, Document@)", &Document_SetDocument_1);
						VDocument.SetMethodExternal("Document@ SetDocument(const string &in)", &Document_SetDocument_2);
						VDocument.SetMethodExternal("Document@ SetArray(const string &in, Document@)", &Document_SetArray_1);
						VDocument.SetMethodExternal("Document@ SetArray(const string &in)", &Document_SetArray_2);
						VDocument.SetMethodExternal("Document@ SetDecimal(const string &in, int64, int64)", &Document_SetDecimal_1);
						VDocument.SetMethodExternal("Document@ SetDecimal(const string &in, const string &in)", &Document_SetDecimal_2);
						VDocument.SetMethodExternal("Document@ SetString(const string &in, const string &in)", &Document_SetString);
						VDocument.SetMethodExternal("Document@ SetId(const string &in, const string &in)", &Document_SetId);
						VDocument.SetMethodExternal("string GetDecimal(const string &in)", &Document_GetDecimal);
						VDocument.SetMethodExternal("string GetId(const string &in)", &Document_GetId);
						VDocument.SetMethodExternal("array<Document@>@ FindCollection(const string &in, bool)", &Document_FindCollection);
						VDocument.SetMethodExternal("array<Document@>@ FindCollectionPath(const string &in, bool)", &Document_FindCollectionPath);
						VDocument.SetMethodExternal("array<Document@>@ GetAttributes()", &Document_GetAttributes);
						VDocument.SetMethodExternal("array<Document@>@ GetNodes()", &Document_GetNodes);
						VDocument.SetMethodExternal("dictionary@ CreateMapping()", &Document_CreateMapping);
						VDocument.SetMethodExternal("string ToJSON()", &Document_ToJSON);
						VDocument.SetMethodExternal("string ToXML()", &Document_ToXML);
						VDocument.SetMethodStatic("Document@ FromJSON(const string &in)", &Document_FromJSON);
						VDocument.SetMethodStatic("Document@ FromXML(const string &in)", &Document_FromXML);
					}

					Script::VMWTypeClass VOS = Global->SetStructUnmanaged<OS>("OS");
					{
						VOS.SetConstructor<OS>("void f()");
						VOS.SetDestructor<OS>("void f()");
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
						VOS.SetMethodStatic("ChildProcess SpawnProcess(const string &in, array<string>@)", &OS_SpawnProcess);
						VOS.SetMethodStatic("array<ResourceEntry>@ ScanFiles(const string &in)", &OS_ScanFiles);
						VOS.SetMethodStatic("array<DirectoryEntry>@ ScanDir(const string &in)", &OS_ScanDir);
						VOS.SetMethodStatic("array<string>@ GetDiskDrives()", &OS_GetDiskDrives);
					}

                    return 0;
                });
            }
        }
    }
}