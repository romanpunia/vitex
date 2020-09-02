#include "script.h"
#include "private/string.h"
#include "private/array.h"
#include "private/any.h"
#include "private/dictionary.h"
#include "private/grid.h"
#include "private/math.h"
#include "private/complex.h"
#include "private/handle.h"
#include "private/datetime.h"
#include "private/weakref.h"
#include "private/rest.h"
#include "private/library.h"
#include "private/jit.h"
#include <iostream>

namespace Tomahawk
{
	namespace Script
	{
		class ByteCodeStream : public asIBinaryStream
		{
		private:
			std::vector<asBYTE> Code;
			int ReadPos, WritePos;

		public:
			ByteCodeStream() : ReadPos(0), WritePos(0)
			{
			}
			ByteCodeStream(const std::vector<asBYTE>& Data) : Code(Data), ReadPos(0), WritePos(0)
			{
			}
			int Read(void* Ptr, asUINT Size)
			{
				if (!Ptr || !Size)
					return 0;

				memcpy(Ptr, &Code[ReadPos], Size);
				ReadPos += Size;

				return 0;
			}
			int Write(const void* Ptr, asUINT Size)
			{
				if (!Ptr || !Size)
					return 0;

				Code.resize(Code.size() + Size);
				memcpy(&Code[WritePos], Ptr, Size);
				WritePos += Size;

				return 0;
			}
			std::vector<asBYTE>& GetCode()
			{
				return Code;
			}
			asUINT GetSize()
			{
				return (asUINT)Code.size();
			}
		};

		asSFuncPtr* VMBind::CreateFunctionBase(void(* Base)(), int Type)
		{
			asSFuncPtr* Ptr = new asSFuncPtr(Type);
			Ptr->ptr.f.func = reinterpret_cast<asFUNCTION_t>(Base);
			return Ptr;
		}
		asSFuncPtr* VMBind::CreateMethodBase(const void* Base, size_t Size, int Type)
		{
			asSFuncPtr* Ptr = new asSFuncPtr(Type);
			Ptr->CopyMethodPtr(Base, Size);
			return Ptr;
		}
		asSFuncPtr* VMBind::CreateDummyBase()
		{
			return new asSFuncPtr(0);
		}
		void VMBind::ReleaseFunctor(asSFuncPtr** Ptr)
		{
			if (!Ptr || !*Ptr)
				return;

			delete *Ptr;
			*Ptr = nullptr;
		}

		VMWMessage::VMWMessage(asSMessageInfo* Msg) : Info(Msg)
		{
		}
		const char* VMWMessage::GetSection() const
		{
			if (!IsValid())
				return nullptr;

			return Info->section;
		}
		const char* VMWMessage::GetMessage() const
		{
			if (!IsValid())
				return nullptr;

			return Info->message;
		}
		VMLogType VMWMessage::GetType() const
		{
			if (!IsValid())
				return VMLogType_INFORMATION;

			return (VMLogType)Info->type;
		}
		int VMWMessage::GetRow() const
		{
			if (!IsValid())
				return -1;

			return Info->row;
		}
		int VMWMessage::GetColumn() const
		{
			if (!IsValid())
				return -1;

			return Info->col;
		}
		asSMessageInfo* VMWMessage::GetMessageInfo() const
		{
			return Info;
		}
		bool VMWMessage::IsValid() const
		{
			return Info != nullptr;
		}

		VMWTypeInfo::VMWTypeInfo(VMCTypeInfo* TypeInfo) : Info(TypeInfo)
		{
			Manager = (Info ? VMManager::Get(Info->GetEngine()) : nullptr);
		}
		void VMWTypeInfo::ForEachProperty(const PropertyCallback& Callback)
		{
			if (!Callback || !IsValid())
				return;

			unsigned int Count = Info->GetPropertyCount();
			for (unsigned int i = 0; i < Count; i++)
			{
				VMFuncProperty Prop;
				if (GetProperty(i, &Prop) >= 0)
					Callback(this, &Prop);
			}
		}
		void VMWTypeInfo::ForEachMethod(const MethodCallback& Callback)
		{
			if (!Callback || !IsValid())
				return;

			unsigned int Count = Info->GetMethodCount();
			for (unsigned int i = 0; i < Count; i++)
			{
				VMWFunction Method = Info->GetMethodByIndex(i);
				if (Method.IsValid())
					Callback(this, &Method);
			}
		}
		const char* VMWTypeInfo::GetGroup() const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetConfigGroup();
		}
		size_t VMWTypeInfo::GetAccessMask() const
		{
			if (!IsValid())
				return 0;

			return Info->GetAccessMask();
		}
		VMWModule VMWTypeInfo::GetModule() const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetModule();
		}
		int VMWTypeInfo::AddRef() const
		{
			if (!IsValid())
				return -1;

			return Info->AddRef();
		}
		int VMWTypeInfo::Release()
		{
			if (!IsValid())
				return -1;

			int R = Info->Release();
			if (R <= 0)
				Info = nullptr;

			return R;
		}
		const char* VMWTypeInfo::GetName() const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetName();
		}
		const char* VMWTypeInfo::GetNamespace() const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetNamespace();
		}
		VMWTypeInfo VMWTypeInfo::GetBaseType() const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetBaseType();
		}
		bool VMWTypeInfo::DerivesFrom(const VMWTypeInfo& Type) const
		{
			if (!IsValid())
				return false;

			return Info->DerivesFrom(Type.GetTypeInfo());
		}
		size_t VMWTypeInfo::GetFlags() const
		{
			if (!IsValid())
				return 0;

			return Info->GetFlags();
		}
		unsigned int VMWTypeInfo::GetSize() const
		{
			if (!IsValid())
				return 0;

			return Info->GetSize();
		}
		int VMWTypeInfo::GetTypeId() const
		{
			if (!IsValid())
				return -1;

			return Info->GetTypeId();
		}
		int VMWTypeInfo::GetSubTypeId(unsigned int SubTypeIndex) const
		{
			if (!IsValid())
				return -1;

			return Info->GetSubTypeId(SubTypeIndex);
		}
		VMWTypeInfo VMWTypeInfo::GetSubType(unsigned int SubTypeIndex) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetSubType(SubTypeIndex);
		}
		unsigned int VMWTypeInfo::GetSubTypeCount() const
		{
			if (!IsValid())
				return 0;

			return Info->GetSubTypeCount();
		}
		unsigned int VMWTypeInfo::GetInterfaceCount() const
		{
			if (!IsValid())
				return 0;

			return Info->GetInterfaceCount();
		}
		VMWTypeInfo VMWTypeInfo::GetInterface(unsigned int Index) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetInterface(Index);
		}
		bool VMWTypeInfo::Implements(const VMWTypeInfo& Type) const
		{
			if (!IsValid())
				return false;

			return Info->Implements(Type.GetTypeInfo());
		}
		unsigned int VMWTypeInfo::GetFactoriesCount() const
		{
			if (!IsValid())
				return 0;

			return Info->GetFactoryCount();
		}
		VMWFunction VMWTypeInfo::GetFactoryByIndex(unsigned int Index) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetFactoryByIndex(Index);
		}
		VMWFunction VMWTypeInfo::GetFactoryByDecl(const char* Decl) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetFactoryByDecl(Decl);
		}
		unsigned int VMWTypeInfo::GetMethodsCount() const
		{
			if (!IsValid())
				return 0;

			return Info->GetMethodCount();
		}
		VMWFunction VMWTypeInfo::GetMethodByIndex(unsigned int Index, bool GetVirtual) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetMethodByIndex(Index, GetVirtual);
		}
		VMWFunction VMWTypeInfo::GetMethodByName(const char* Name, bool GetVirtual) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetMethodByName(Name, GetVirtual);
		}
		VMWFunction VMWTypeInfo::GetMethodByDecl(const char* Decl, bool GetVirtual) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetMethodByDecl(Decl, GetVirtual);
		}
		unsigned int VMWTypeInfo::GetPropertiesCount() const
		{
			if (!IsValid())
				return 0;

			return Info->GetPropertyCount();
		}
		int VMWTypeInfo::GetProperty(unsigned int Index, VMFuncProperty* Out) const
		{
			if (!IsValid())
				return -1;

			const char* Name;
			asDWORD AccessMask;
			int TypeId, Offset;
			bool IsPrivate;
			bool IsProtected;
			bool IsReference;
			int Result = Info->GetProperty(Index, &Name, &TypeId, &IsPrivate, &IsProtected, &Offset, &IsReference, &AccessMask);

			if (Out != nullptr)
			{
				Out->Name = Name;
				Out->AccessMask = (size_t)AccessMask;
				Out->TypeId = TypeId;
				Out->Offset = Offset;
				Out->IsPrivate = IsPrivate;
				Out->IsProtected = IsProtected;
				Out->IsReference = IsReference;
			}

			return Result;
		}
		const char* VMWTypeInfo::GetPropertyDeclaration(unsigned int Index, bool IncludeNamespace) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetPropertyDeclaration(Index, IncludeNamespace);
		}
		unsigned int VMWTypeInfo::GetBehaviourCount() const
		{
			if (!IsValid())
				return 0;

			return Info->GetBehaviourCount();
		}
		VMWFunction VMWTypeInfo::GetBehaviourByIndex(unsigned int Index, VMBehave* OutBehaviour) const
		{
			if (!IsValid())
				return nullptr;

			asEBehaviours Out;
			VMCFunction* Result = Info->GetBehaviourByIndex(Index, &Out);
			if (OutBehaviour != nullptr)
				*OutBehaviour = (VMBehave)Out;

			return Result;
		}
		unsigned int VMWTypeInfo::GetChildFunctionDefCount() const
		{
			if (!IsValid())
				return 0;

			return Info->GetChildFuncdefCount();
		}
		VMWTypeInfo VMWTypeInfo::GetChildFunctionDef(unsigned int Index) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetChildFuncdef(Index);
		}
		VMWTypeInfo VMWTypeInfo::GetParentType() const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetParentType();
		}
		unsigned int VMWTypeInfo::GetEnumValueCount() const
		{
			if (!IsValid())
				return 0;

			return Info->GetEnumValueCount();
		}
		const char* VMWTypeInfo::GetEnumValueByIndex(unsigned int Index, int* OutValue) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetEnumValueByIndex(Index, OutValue);
		}
		VMWFunction VMWTypeInfo::GetFunctionDefSignature() const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetFuncdefSignature();
		}
		void* VMWTypeInfo::SetUserData(void* Data, uint64_t Type)
		{
			if (!IsValid())
				return nullptr;

			return Info->SetUserData(Data, Type);
		}
		void* VMWTypeInfo::GetUserData(uint64_t Type) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetUserData(Type);
		}
		bool VMWTypeInfo::IsHandle() const
		{
			if (!IsValid())
				return false;

			return IsHandle(Info->GetTypeId());
		}
		bool VMWTypeInfo::IsValid() const
		{
			return Manager != nullptr && Info != nullptr;
		}
		VMCTypeInfo* VMWTypeInfo::GetTypeInfo() const
		{
			return Info;
		}
		VMManager* VMWTypeInfo::GetManager() const
		{
			return Manager;
		}
		bool VMWTypeInfo::IsHandle(int TypeId)
		{
			return (TypeId & asTYPEID_OBJHANDLE || TypeId & asTYPEID_HANDLETOCONST);
		}
		bool VMWTypeInfo::IsScriptObject(int TypeId)
		{
			return (TypeId & asTYPEID_SCRIPTOBJECT);
		}

		VMWFunction::VMWFunction(VMCFunction* Base) : Function(Base)
		{
			Manager = (Base ? VMManager::Get(Base->GetEngine()) : nullptr);
		}
		int VMWFunction::AddRef() const
		{
			if (!IsValid())
				return -1;

			return Function->AddRef();
		}
		int VMWFunction::Release()
		{
			if (!IsValid())
				return -1;

			int R = Function->Release();
			if (R <= 0)
				Function = nullptr;

			return R;
		}
		int VMWFunction::GetId() const
		{
			if (!IsValid())
				return -1;

			return Function->GetId();
		}
		VMFuncType VMWFunction::GetType() const
		{
			if (!IsValid())
				return VMFuncType_DUMMY;

			return (VMFuncType)Function->GetFuncType();
		}
		const char* VMWFunction::GetModuleName() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetModuleName();
		}
		VMWModule VMWFunction::GetModule() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetModule();
		}
		const char* VMWFunction::GetSectionName() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetScriptSectionName();
		}
		const char* VMWFunction::GetGroup() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetConfigGroup();
		}
		size_t VMWFunction::GetAccessMask() const
		{
			if (!IsValid())
				return -1;

			return Function->GetAccessMask();
		}
		VMWTypeInfo VMWFunction::GetObjectType() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetObjectType();
		}
		const char* VMWFunction::GetObjectName() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetObjectName();
		}
		const char* VMWFunction::GetName() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetName();
		}
		const char* VMWFunction::GetNamespace() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetNamespace();
		}
		const char* VMWFunction::GetDecl(bool IncludeObjectName, bool IncludeNamespace, bool IncludeArgNames) const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetDeclaration(IncludeObjectName, IncludeNamespace, IncludeArgNames);
		}
		bool VMWFunction::IsReadOnly() const
		{
			if (!IsValid())
				return false;

			return Function->IsPrivate();
		}
		bool VMWFunction::IsPrivate() const
		{
			if (!IsValid())
				return false;

			return Function->IsPrivate();
		}
		bool VMWFunction::IsProtected() const
		{
			if (!IsValid())
				return false;

			return Function->IsProtected();
		}
		bool VMWFunction::IsFinal() const
		{
			if (!IsValid())
				return false;

			return Function->IsFinal();
		}
		bool VMWFunction::IsOverride() const
		{
			if (!IsValid())
				return false;

			return Function->IsOverride();
		}
		bool VMWFunction::IsShared() const
		{
			if (!IsValid())
				return false;

			return Function->IsShared();
		}
		bool VMWFunction::IsExplicit() const
		{
			if (!IsValid())
				return false;

			return Function->IsExplicit();
		}
		bool VMWFunction::IsProperty() const
		{
			if (!IsValid())
				return false;

			return Function->IsProperty();
		}
		unsigned int VMWFunction::GetArgsCount() const
		{
			if (!IsValid())
				return 0;

			return Function->GetParamCount();
		}
		int VMWFunction::GetArg(unsigned int Index, int* TypeId, size_t* Flags, const char** Name, const char** DefaultArg) const
		{
			if (!IsValid())
				return -1;

			asDWORD asFlags;
			int R = Function->GetParam(Index, TypeId, &asFlags, Name, DefaultArg);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;

			return R;
		}
		int VMWFunction::GetReturnTypeId(size_t* Flags) const
		{
			if (!IsValid())
				return -1;

			asDWORD asFlags;
			int R = Function->GetReturnTypeId(&asFlags);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;

			return R;
		}
		int VMWFunction::GetTypeId() const
		{
			if (!IsValid())
				return -1;

			return Function->GetTypeId();
		}
		bool VMWFunction::IsCompatibleWithTypeId(int TypeId) const
		{
			if (!IsValid())
				return false;

			return Function->IsCompatibleWithTypeId(TypeId);
		}
		void* VMWFunction::GetDelegateObject() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetDelegateObject();
		}
		VMWTypeInfo VMWFunction::GetDelegateObjectType() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetDelegateObjectType();
		}
		VMWFunction VMWFunction::GetDelegateFunction() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetDelegateFunction();
		}
		unsigned int VMWFunction::GetPropertiesCount() const
		{
			if (!IsValid())
				return 0;

			return Function->GetVarCount();
		}
		int VMWFunction::GetProperty(unsigned int Index, const char** Name, int* TypeId) const
		{
			if (!IsValid())
				return -1;

			return Function->GetVar(Index, Name, TypeId);
		}
		const char* VMWFunction::GetPropertyDecl(unsigned int Index, bool IncludeNamespace) const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetVarDecl(Index, IncludeNamespace);
		}
		int VMWFunction::FindNextLineWithCode(int Line) const
		{
			if (!IsValid())
				return -1;

			return Function->FindNextLineWithCode(Line);
		}
		void* VMWFunction::SetUserData(void* UserData, uint64_t Type)
		{
			if (!IsValid())
				return nullptr;

			return Function->SetUserData(UserData, Type);
		}
		void* VMWFunction::GetUserData(uint64_t Type) const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetUserData(Type);
		}
		bool VMWFunction::IsValid() const
		{
			return Manager != nullptr && Function != nullptr;
		}
		VMCFunction* VMWFunction::GetFunction() const
		{
			return Function;
		}
		VMManager* VMWFunction::GetManager() const
		{
			return Manager;
		}

		VMCRandom::VMCRandom()
		{
			SeedFromTime();
		}
		void VMCRandom::AddRef()
		{
			asAtomicInc(Ref);
		}
		void VMCRandom::Release()
		{
			if (asAtomicDec(Ref) <= 0)
				delete this;
		}
		void VMCRandom::SeedFromTime()
		{
			Seed(static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
		}
		uint32_t VMCRandom::GetU()
		{
			return Twister();
		}
		int32_t VMCRandom::GetI()
		{
			return Twister();
		}
		double VMCRandom::GetD()
		{
			return DDist(Twister);
		}
		void VMCRandom::Seed(uint32_t Seed)
		{
			Twister.seed(Seed);
		}
		void VMCRandom::Seed(VMCArray* Array)
		{
			if (!Array || Array->GetElementTypeId() != asTYPEID_UINT32)
			{
				VMCContext* Context = asGetActiveContext();
				if (Context != nullptr)
					Context->SetException("random::seed array element type not uint32");

				return;
			}

			std::vector<uint32_t> Vector;
			Vector.reserve(Array->GetSize());

			for (unsigned int i = 0; i < Array->GetSize(); i++)
				Vector.push_back(static_cast<uint32_t*>(Array->GetBuffer())[i]);

			std::seed_seq Sq(Vector.begin(), Vector.end());
			Twister.seed(Sq);
		}
		void VMCRandom::Assign(VMCRandom* From)
		{
			if (From != nullptr)
				Twister = From->Twister;
		}
		VMCRandom* VMCRandom::Create()
		{
			return new VMCRandom();
		}

		VMCReceiver::VMCReceiver() : Debug(0)
		{
		}
		void VMCReceiver::Send(VMCAny* Any)
		{
			if (!Any)
				return;

			Any->AddRef();
			Debug += 1;

			{
				std::lock_guard<std::mutex> Guard(Mutex);
				Queue.push_back(Any);
			}

			CV.notify_one();
		}
		void VMCReceiver::Release()
		{
			std::lock_guard<std::mutex> Guard(Mutex);
			for (auto Any : Queue)
			{
				if (Any != nullptr)
					Any->Release();
			}
			Queue.clear();
		}
		void VMCReceiver::EnumReferences(VMCManager* Engine)
		{
			if (!Engine)
				return;

			for (auto Any : Queue)
			{
				if (Any != nullptr)
					Engine->GCEnumCallback(Any);
			}
		}
		int VMCReceiver::GetQueueSize()
		{
			return Queue.size();
		}
		VMCAny* VMCReceiver::ReceiveWait()
		{
			VMCAny* Value = nullptr;
			while (!Value)
				Value = Receive(10000);

			return Value;
		}
		VMCAny* VMCReceiver::Receive(uint64_t Timeout)
		{
			std::unique_lock<std::mutex> Guard(Mutex);
			auto Duration = std::chrono::milliseconds(Timeout);

			if (!CV.wait_for(Guard, Duration, [&]
			{
				return Queue.size() != 0;
			}))
				return nullptr;

			VMCAny* Result = Queue.front();
			Queue.erase(Queue.begin());

			return Result;
		}

		VMCThread::VMCThread(VMCManager* Engine, VMCFunction* Func) : Manager(VMManager::Get(Engine)), Function(Func), Context(nullptr), Ref(1), Active(false), GCFlag(false)
		{
		}
		void VMCThread::StartRoutine(VMCThread* Thread)
		{
			if (Thread != nullptr)
				Thread->StartRoutineThread();
		}
		void VMCThread::StartRoutineThread()
		{
			{
				std::lock_guard<std::mutex> Guard(Mutex);
				if (!Function)
				{
					Active = false;
					Release();
					return;
				}

				if (Context == nullptr)
					Context = Manager->CreateContext();

				if (Context == nullptr)
				{
					Manager->GetEngine()->WriteMessage("", 0, 0, asMSGTYPE_ERROR, "failed to start a thread: no available context");
					Active = false;
					Release();
					return;
				}

				Context->Prepare(Function);
				Context->SetUserData(this, ContextUD);
			}

			Context->Execute();
			{
				std::lock_guard<std::mutex> Guard(Mutex);
				Active = false;
				Context->SetUserData(nullptr, ContextUD);
				delete Context;
				Context = nullptr;
				CV.notify_all();
			}
			Release();
			asThreadCleanup();
		}
		void VMCThread::AddRef()
		{
			GCFlag = false;
			asAtomicInc(Ref);
		}
		void VMCThread::Suspend()
		{
			std::lock_guard<std::mutex> Guard(Mutex);
			if (Context)
				Context->Suspend();
		}
		void VMCThread::Release()
		{
			GCFlag = false;
			if (asAtomicDec(Ref) <= 0)
			{
				if (Thread.joinable())
					Thread.join();

				ReleaseReferences(nullptr);
				delete this;
			}
		}
		void VMCThread::SetGCFlag()
		{
			GCFlag = true;
		}
		bool VMCThread::GetGCFlag()
		{
			return GCFlag;
		}
		int VMCThread::GetRefCount()
		{
			return Ref;
		}
		void VMCThread::EnumReferences(VMCManager* Engine)
		{
			Incoming.EnumReferences(Engine);
			Outgoing.EnumReferences(Engine);
			Engine->GCEnumCallback(Engine);

			if (Context != nullptr)
				Engine->GCEnumCallback(Context);

			if (Function != nullptr)
				Engine->GCEnumCallback(Function);
		}
		int VMCThread::Wait(uint64_t Timeout)
		{
			{
				std::lock_guard<std::mutex> Guard(Mutex);
				if (!Thread.joinable())
					return -1;
			}
			{
				std::unique_lock<std::mutex> Guard(Mutex);
				auto Duration = std::chrono::milliseconds(Timeout);
				if (CV.wait_for(Guard, Duration, [&]
				{
					return !Active;
				}))
				{
					Thread.join();
					return 1;
				}
			}

			return 0;
		}
		int VMCThread::Join()
		{
			while (true)
			{
				int R = Wait(1000);
				if (R == -1 || R == 1)
					return R;
			}

			return 0;
		}
		void VMCThread::Send(VMCAny* Any)
		{
			Incoming.Send(Any);
		}
		VMCAny* VMCThread::ReceiveWait()
		{
			return Outgoing.ReceiveWait();
		}
		VMCAny* VMCThread::Receive(uint64_t Timeout)
		{
			return Outgoing.Receive(Timeout);
		}
		bool VMCThread::IsActive()
		{
			std::lock_guard<std::mutex> Guard(Mutex);
			return !Active;
		}
		bool VMCThread::Start()
		{
			std::lock_guard<std::mutex> Guard(Mutex);
			if (!Function)
				return false;

			if (Active)
				return false;

			Active = true;
			AddRef();
			Thread = std::thread(&VMCThread::StartRoutineThread, this);
			return true;
		}
		void VMCThread::ReleaseReferences(VMCManager*)
		{
			Outgoing.Release();
			Incoming.Release();

			std::lock_guard<std::mutex> Guard(Mutex);
			if (Function)
				Function->Release();

			delete Context;
			Context = nullptr;
			Manager = nullptr;
			Function = nullptr;
		}
		VMCThread* VMCThread::GetThisThread()
		{
			VMCContext* Context = asGetActiveContext();
			if (!Context)
				return nullptr;

			VMCThread* Thread = static_cast<VMCThread*>(Context->GetUserData(ContextUD));
			if (Thread != nullptr)
				return Thread;

			Context->SetException("cannot call global thread messaging in the main thread");
			return nullptr;
		}
		VMCThread* VMCThread::GetThisThreadSafe()
		{
			VMCContext* Context = asGetActiveContext();
			if (!Context)
				return nullptr;

			return static_cast<VMCThread*>(Context->GetUserData(ContextUD));
		}
		int VMCThread::AtomicNotifyGC(const char* TypeName, void* Object)
		{
			if (!TypeName || !Object)
				return -1;

			VMCContext* Context = asGetActiveContext();
			if (!Context)
				return -1;

			VMManager* Engine = VMManager::Get(Context->GetEngine());
			if (!Engine)
				return -1;

			VMWTypeInfo Type = Engine->Global().GetTypeInfoByName(TypeName);
			return Engine->NotifyOfNewObject(Object, Type.GetTypeInfo());
		}
		void VMCThread::SendInThread(VMCAny* Any)
		{
			auto* Thread = GetThisThread();
			if (Thread != nullptr)
				Thread->Outgoing.Send(Any);
		}
		void VMCThread::SleepInThread(uint64_t Timeout)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(Timeout));
		}
		VMCAny* VMCThread::ReceiveWaitInThread()
		{
			auto* Thread = GetThisThread();
			if (!Thread)
				return nullptr;

			return Thread->Incoming.ReceiveWait();
		}
		VMCAny* VMCThread::ReceiveInThread(uint64_t Timeout)
		{
			auto* Thread = GetThisThread();
			if (!Thread)
				return nullptr;

			return Thread->Incoming.Receive(Timeout);
		}
		VMCThread* VMCThread::StartThread(VMCFunction* Func)
		{
			VMCContext* Context = asGetActiveContext();
			if (!Context)
			{
				if (Func)
					Func->Release();

				return nullptr;
			}

			VMCManager* Engine = Context->GetEngine();
			if (!Engine)
			{
				if (Func)
					Func->Release();

				return nullptr;
			}

			VMCThread* Thread = new VMCThread(Engine, Func);
			Engine->NotifyGarbageCollectorOfNewObject(Thread, Engine->GetTypeInfoByName("thread"));

			return Thread;
		}
		uint64_t VMCThread::GetIdInThread()
		{
			auto* Thread = GetThisThreadSafe();
			if (!Thread)
				return 0;

			return (uint64_t)std::hash<std::thread::id>()(std::this_thread::get_id());
		}
		int VMCThread::ContextUD = 550;
		int VMCThread::EngineListUD = 551;

		VMCAsync::VMCAsync(VMCContext* Base) : Context(Base), Any(nullptr), Rejected(false), Ref(1), GCFlag(false)
		{
			if (Context != nullptr)
				Context->AddRef();
		}
		void VMCAsync::Release()
		{
			GCFlag = false;
			if (asAtomicDec(Ref) <= 0)
			{
				ReleaseReferences(nullptr);
				delete this;
			}
		}
		void VMCAsync::AddRef()
		{
			GCFlag = false;
			asAtomicInc(Ref);
		}
		void VMCAsync::EnumReferences(VMCManager* Engine)
		{
			Mutex.lock();
			if (Context != nullptr)
				Engine->GCEnumCallback(Context);

			if (Any != nullptr)
				Engine->GCEnumCallback(Any);
			Mutex.unlock();
		}
		void VMCAsync::ReleaseReferences(VMCManager*)
		{
			Mutex.lock();
			if (Any != nullptr)
			{
				Any->Release();
				Any = nullptr;
			}

			if (Context != nullptr)
			{
				Context->Release();
				Context = nullptr;
			}
			Mutex.unlock();
		}
		void VMCAsync::SetGCFlag()
		{
			GCFlag = true;
		}
		bool VMCAsync::GetGCFlag()
		{
			return GCFlag;
		}
		int VMCAsync::GetRefCount()
		{
			return Ref;
		}
		int VMCAsync::Set(VMCAny* Value)
		{
			if (!Context)
				return -1;

			Mutex.lock();
			if (Any != nullptr)
				Any->Release();

			if (Value != nullptr)
				Value->AddRef();
			else
				Rejected = true;

			Any = Value;
			Mutex.unlock();

			if (Context->GetState() != asEXECUTION_SUSPENDED)
				return asEXECUTION_FINISHED;

			int R = Context->Execute();
			if (Done)
				Done((VMExecState)R);

			return R;
		}
		int VMCAsync::Set(const VMWAny& Value)
		{
			return Set(Value.GetAny());
		}
		int VMCAsync::Set(void* Ref, int TypeId)
		{
			if (!Context)
				return -1;

			return Set(new VMCAny(Ref, TypeId, Context->GetEngine()));
		}
		int VMCAsync::Set(void* Ref, const char* TypeName)
		{
			if (!Context)
				return -1;

			VMManager* Engine = VMManager::Get(Context->GetEngine());
			if (!Engine)
				return -1;

			return Set(Ref, Engine->Global().GetTypeIdByDecl(TypeName));
		}
		bool VMCAsync::GetAny(void* Ref, int TypeId) const
		{
			if (!Any)
				return false;

			return Any->Retrieve(Ref, TypeId);
		}
		VMCAny* VMCAsync::Get() const
		{
			return Any;
		}
		VMCAsync* VMCAsync::Await()
		{
			if (!Context)
				return this;

			if (!Any && !Rejected)
				Context->Suspend();

			return this;
		}
		VMCAsync* VMCAsync::Create(const AsyncWorkCallback& WorkCallback, const AsyncDoneCallback& DoneCallback)
		{
			VMCContext* Context = asGetActiveContext();
			if (!Context)
				return nullptr;

			VMCManager* Engine = Context->GetEngine();
			if (!Engine)
				return nullptr;

			VMCAsync* Async = new VMCAsync(Context);
			Engine->NotifyGarbageCollectorOfNewObject(Async, Engine->GetTypeInfoByName("async"));

			Async->Done = DoneCallback;
			if (WorkCallback)
				WorkCallback(Async);

			return Async;
		}
		VMCAsync* VMCAsync::CreateFilled(void* Ref, int TypeId)
		{
			VMCContext* Context = asGetActiveContext();
			if (!Context)
				return nullptr;

			VMCManager* Engine = Context->GetEngine();
			if (!Engine)
				return nullptr;

			VMCAsync* Async = new VMCAsync(Context);
			Engine->NotifyGarbageCollectorOfNewObject(Async, Engine->GetTypeInfoByName("async"));
			Async->Set(Ref, TypeId);

			return Async;
		}
		VMCAsync* VMCAsync::CreateFilled(void* Ref, const char* TypeName)
		{
			VMCContext* Context = asGetActiveContext();
			if (!Context)
				return nullptr;

			VMCManager* Engine = Context->GetEngine();
			if (!Engine)
				return nullptr;

			VMCAsync* Async = new VMCAsync(Context);
			Engine->NotifyGarbageCollectorOfNewObject(Async, Engine->GetTypeInfoByName("async"));
			Async->Set(Ref, TypeName);

			return Async;
		}
		VMCAsync* VMCAsync::CreateFilled(bool Value)
		{
			return CreateFilled(&Value, VMTypeId_BOOL);
		}
		VMCAsync* VMCAsync::CreateFilled(int64_t Value)
		{
			return CreateFilled(&Value, VMTypeId_INT64);
		}
		VMCAsync* VMCAsync::CreateFilled(double Value)
		{
			return CreateFilled(&Value, VMTypeId_DOUBLE);
		}
		VMCAsync* VMCAsync::CreateEmpty()
		{
			VMCContext* Context = asGetActiveContext();
			if (!Context)
				return nullptr;

			VMCManager* Engine = Context->GetEngine();
			if (!Engine)
				return nullptr;

			VMCAsync* Async = new VMCAsync(Context);
			Engine->NotifyGarbageCollectorOfNewObject(Async, Engine->GetTypeInfoByName("async"));
			Async->Set(nullptr);

			return Async;
		}

		VMWArray::VMWArray(VMCArray* Base) : Array(Base)
		{
		}
		void VMWArray::AddRef() const
		{
			if (!IsValid())
				return;

			return Array->AddRef();
		}
		void VMWArray::Release()
		{
			if (!IsValid())
				return;

			Array->Release();
			Array = nullptr;
		}
		VMWTypeInfo VMWArray::GetArrayObjectType() const
		{
			if (!IsValid())
				return nullptr;

			return Array->GetArrayObjectType();
		}
		int VMWArray::GetArrayTypeId() const
		{
			if (!IsValid())
				return -1;

			return Array->GetArrayTypeId();
		}
		int VMWArray::GetElementTypeId() const
		{
			if (!IsValid())
				return -1;

			return Array->GetElementTypeId();
		}
		unsigned int VMWArray::GetSize() const
		{
			if (!IsValid())
				return 0;

			return Array->GetSize();
		}
		bool VMWArray::IsEmpty() const
		{
			if (!IsValid())
				return true;

			return Array->IsEmpty();
		}
		void VMWArray::Reserve(unsigned int NumElements)
		{
			if (!IsValid())
				return;

			return Array->Reserve(NumElements);
		}
		void VMWArray::Resize(unsigned int NumElements)
		{
			if (!IsValid())
				return;

			return Array->Resize(NumElements);
		}
		void* VMWArray::At(unsigned int Index)
		{
			if (!IsValid())
				return nullptr;

			return Array->At(Index);
		}
		const void* VMWArray::At(unsigned int Index) const
		{
			if (!IsValid())
				return nullptr;

			return Array->At(Index);
		}
		void VMWArray::SetValue(unsigned int Index, void* Value)
		{
			if (!IsValid())
				return;

			return Array->SetValue(Index, Value);
		}
		void VMWArray::InsertAt(unsigned int Index, void* Value)
		{
			if (!IsValid())
				return;

			return Array->InsertAt(Index, Value);
		}
		void VMWArray::RemoveAt(unsigned int Index)
		{
			if (!IsValid())
				return;

			return Array->RemoveAt(Index);
		}
		void VMWArray::InsertLast(void* Value)
		{
			if (!IsValid())
				return;

			return Array->InsertLast(Value);
		}
		void VMWArray::RemoveLast()
		{
			if (!IsValid())
				return;

			return Array->RemoveLast();
		}
		void VMWArray::SortAsc()
		{
			if (!IsValid())
				return;

			return Array->SortAsc();
		}
		void VMWArray::SortAsc(unsigned int StartAt, unsigned int Count)
		{
			if (!IsValid())
				return;

			return Array->SortAsc(StartAt, Count);
		}
		void VMWArray::SortDesc()
		{
			if (!IsValid())
				return;

			return Array->SortDesc();
		}
		void VMWArray::SortDesc(unsigned int StartAt, unsigned int Count)
		{
			if (!IsValid())
				return;

			return Array->SortDesc(StartAt, Count);
		}
		void VMWArray::Sort(unsigned int StartAt, unsigned int Count, bool Asc)
		{
			if (!IsValid())
				return;

			return Array->Sort(StartAt, Count, Asc);
		}
		void VMWArray::Sort(const VMWFunction& Less, unsigned int StartAt, unsigned int Count)
		{
			if (!IsValid())
				return;

			return Array->Sort(Less.GetFunction(), StartAt, Count);
		}
		void VMWArray::Reverse()
		{
			if (!IsValid())
				return;

			return Array->Reverse();
		}
		int VMWArray::Find(void* Value) const
		{
			if (!IsValid())
				return -1;

			return Array->Find(Value);
		}
		int VMWArray::Find(unsigned int StartAt, void* Value) const
		{
			if (!IsValid())
				return -1;

			return Array->Find(StartAt, Value);
		}
		int VMWArray::FindByRef(void* Ref) const
		{
			if (!IsValid())
				return -1;

			return Array->FindByRef(Ref);
		}
		int VMWArray::FindByRef(unsigned int StartAt, void* Ref) const
		{
			if (!IsValid())
				return -1;

			return Array->FindByRef(StartAt, Ref);
		}
		void* VMWArray::GetBuffer()
		{
			if (!IsValid())
				return nullptr;

			return Array->GetBuffer();
		}
		bool VMWArray::IsValid() const
		{
			return Array != nullptr;
		}
		VMCArray* VMWArray::GetArray() const
		{
			return Array;
		}
		VMWArray VMWArray::Create(const VMWTypeInfo& ArrayType)
		{
			return VMCArray::Create(ArrayType.GetTypeInfo());
		}
		VMWArray VMWArray::Create(const VMWTypeInfo& ArrayType, unsigned int Length)
		{
			return VMCArray::Create(ArrayType.GetTypeInfo(), Length);
		}
		VMWArray VMWArray::Create(const VMWTypeInfo& ArrayType, unsigned int Length, void* DefaultValue)
		{
			return VMCArray::Create(ArrayType.GetTypeInfo(), Length, DefaultValue);
		}
		VMWArray VMWArray::Create(const VMWTypeInfo& ArrayType, void* ListBuffer)
		{
			return VMCArray::Create(ArrayType.GetTypeInfo(), ListBuffer);
		}

		VMWAny::VMWAny(VMCAny* Base) : Any(Base)
		{
		}
		int VMWAny::AddRef() const
		{
			if (!IsValid())
				return -1;

			return Any->AddRef();
		}
		int VMWAny::Release()
		{
			if (!IsValid())
				return -1;

			int R = Any->Release();
			if (R <= 0)
				Any = nullptr;

			return R;
		}
		int VMWAny::CopyFrom(const VMWAny& Other)
		{
			if (!IsValid())
				return -1;

			return Any->CopyFrom(Other.GetAny());
		}
		void VMWAny::Store(void* Ref, int RefTypeId)
		{
			if (!IsValid())
				return;

			return Any->Store(Ref, RefTypeId);
		}
		void VMWAny::Store(as_int64_t& Value)
		{
			if (!IsValid())
				return;

			return Any->Store(Value);
		}
		void VMWAny::Store(double& Value)
		{
			if (!IsValid())
				return;

			return Any->Store(Value);
		}
		bool VMWAny::RetrieveAny(void** Ref, int* RefTypeId) const
		{
			if (!IsValid() || !Ref)
				return false;

			if (RefTypeId != nullptr)
				*RefTypeId = Any->GetTypeId();

			*Ref = nullptr;
			return Any->Retrieve((void*)Ref, Any->GetTypeId());
		}
		bool VMWAny::Retrieve(void* Ref, int RefTypeId) const
		{
			if (!IsValid())
				return false;

			return Any->Retrieve(Ref, RefTypeId);
		}
		bool VMWAny::Retrieve(as_int64_t& Value) const
		{
			if (!IsValid())
				return -1;

			return Any->Retrieve(Value);
		}
		bool VMWAny::Retrieve(double& Value) const
		{
			if (!IsValid())
				return -1;

			return Any->Retrieve(Value);
		}
		int VMWAny::GetTypeId() const
		{
			if (!IsValid())
				return -1;

			return Any->GetTypeId();
		}
		bool VMWAny::IsValid() const
		{
			return Any != nullptr;
		}
		VMCAny* VMWAny::GetAny() const
		{
			return Any;
		}
		VMWAny VMWAny::Create(VMManager* Manager, void* Ref, int TypeId)
		{
			if (!Manager)
				return nullptr;

			return new VMCAny(Ref, TypeId, Manager->GetEngine());
		}

		VMWObject::VMWObject(VMCObject* Base) : Object(Base)
		{
		}
		int VMWObject::AddRef() const
		{
			if (!IsValid())
				return 0;

			return Object->AddRef();
		}
		int VMWObject::Release()
		{
			if (!IsValid())
				return 0;

			int R = Object->Release();
			Object = nullptr;

			return R;
		}
		VMCLockableSharedBool* VMWObject::GetWeakRefFlag()
		{
			if (!IsValid())
				return nullptr;

			return Object->GetWeakRefFlag();
		}
		VMWTypeInfo VMWObject::GetObjectType()
		{
			if (!IsValid())
				return nullptr;

			return Object->GetObjectType();
		}
		int VMWObject::GetTypeId()
		{
			if (!IsValid())
				return 0;

			return Object->GetTypeId();
		}
		int VMWObject::GetPropertyTypeId(unsigned int Id) const
		{
			if (!IsValid())
				return 0;

			return Object->GetPropertyTypeId(Id);
		}
		unsigned int VMWObject::GetPropertiesCount() const
		{
			if (!IsValid())
				return 0;

			return Object->GetPropertyCount();
		}
		const char* VMWObject::GetPropertyName(unsigned int Id) const
		{
			if (!IsValid())
				return 0;

			return Object->GetPropertyName(Id);
		}
		void* VMWObject::GetAddressOfProperty(unsigned int Id)
		{
			if (!IsValid())
				return nullptr;

			return Object->GetAddressOfProperty(Id);
		}
		VMManager* VMWObject::GetManager() const
		{
			if (!IsValid())
				return nullptr;

			return VMManager::Get(Object->GetEngine());
		}
		int VMWObject::CopyFrom(const VMWObject& Other)
		{
			if (!IsValid())
				return -1;

			return Object->CopyFrom(Other.GetObject());
		}
		void* VMWObject::SetUserData(void* Data, uint64_t Type)
		{
			if (!IsValid())
				return nullptr;

			return Object->SetUserData(Data, (asPWORD)Type);
		}
		void* VMWObject::GetUserData(uint64_t Type) const
		{
			if (!IsValid())
				return nullptr;

			return Object->GetUserData((asPWORD)Type);
		}
		bool VMWObject::IsValid() const
		{
			return Object != nullptr;
		}
		VMCObject* VMWObject::GetObject() const
		{
			return Object;
		}

		VMWDictionary::VMWDictionary(VMCDictionary* Base) : Dictionary(Base)
		{
		}
		void VMWDictionary::AddRef() const
		{
			if (!IsValid())
				return;

			return Dictionary->AddRef();
		}
		void VMWDictionary::Release()
		{
			if (!IsValid())
				return;

			Dictionary->Release();
			Dictionary = nullptr;
		}
		void VMWDictionary::Set(const std::string& Key, void* Value, int TypeId)
		{
			if (!IsValid())
				return;

			return Dictionary->Set(Key, Value, TypeId);
		}
		void VMWDictionary::Set(const std::string& Key, as_int64_t& Value)
		{
			if (!IsValid())
				return;

			return Dictionary->Set(Key, Value);
		}
		void VMWDictionary::Set(const std::string& Key, double& Value)
		{
			if (!IsValid())
				return;

			return Dictionary->Set(Key, Value);
		}
		bool VMWDictionary::GetIndex(size_t Index, std::string* Key, void** Value, int* TypeId) const
		{
			if (!IsValid() || Index >= Dictionary->GetSize())
				return false;

			auto It = Dictionary->begin();
			size_t Offset = 0;

			while (Offset != Index)
			{
				Offset++;
				It++;
			}

			if (Key != nullptr)
				*Key = It.GetKey();

			if (Value != nullptr)
				*Value = (void*)It.GetAddressOfValue();

			if (TypeId != nullptr)
				*TypeId = It.GetTypeId();

			return true;
		}
		bool VMWDictionary::Get(const std::string& Key, void** Value, int* TypeId) const
		{
			if (!IsValid())
				return false;

			auto It = Dictionary->find(Key);
			if (It == Dictionary->end())
				return false;

			if (Value != nullptr)
				*Value = (void*)It.GetAddressOfValue();

			if (TypeId != nullptr)
				*TypeId = It.GetTypeId();

			return true;
		}
		bool VMWDictionary::Get(const std::string& Key, as_int64_t& Value) const
		{
			if (!IsValid())
				return false;

			return Dictionary->Get(Key, Value);
		}
		bool VMWDictionary::Get(const std::string& Key, double& Value) const
		{
			if (!IsValid())
				return false;

			return Dictionary->Get(Key, Value);
		}
		int VMWDictionary::GetTypeId(const std::string& Key) const
		{
			if (!IsValid())
				return -1;

			return Dictionary->GetTypeId(Key);
		}
		bool VMWDictionary::Exists(const std::string& Key) const
		{
			if (!IsValid())
				return false;

			return Dictionary->Exists(Key);
		}
		bool VMWDictionary::IsEmpty() const
		{
			if (!IsValid())
				return false;

			return Dictionary->IsEmpty();
		}
		unsigned int VMWDictionary::GetSize() const
		{
			if (!IsValid())
				return 0;

			return Dictionary->GetSize();
		}
		bool VMWDictionary::Delete(const std::string& Key)
		{
			if (!IsValid())
				return false;

			return Dictionary->Delete(Key);
		}
		void VMWDictionary::DeleteAll()
		{
			if (!IsValid())
				return;

			return Dictionary->DeleteAll();
		}
		VMWArray VMWDictionary::GetKeys() const
		{
			if (!IsValid())
				return nullptr;

			return Dictionary->GetKeys();
		}
		bool VMWDictionary::IsValid() const
		{
			return Dictionary != nullptr;
		}
		VMCDictionary* VMWDictionary::GetDictionary() const
		{
			return Dictionary;
		}
		VMWDictionary VMWDictionary::Create(VMManager* Engine)
		{
			if (!Engine)
				return nullptr;

			return VMCDictionary::Create(Engine->GetEngine());
		}

		VMWGrid::VMWGrid(VMCGrid* Base) : Grid(Base)
		{
		}
		void VMWGrid::AddRef() const
		{
			if (!IsValid())
				return;

			return Grid->AddRef();
		}
		void VMWGrid::Release()
		{
			if (!IsValid())
				return;

			Grid->Release();
			Grid = nullptr;
		}
		VMWTypeInfo VMWGrid::GetGridObjectType() const
		{
			if (!IsValid())
				return nullptr;

			return Grid->GetGridObjectType();
		}
		int VMWGrid::GetGridTypeId() const
		{
			if (!IsValid())
				return -1;

			return Grid->GetGridTypeId();
		}
		int VMWGrid::GetElementTypeId() const
		{
			if (!IsValid())
				return -1;

			return Grid->GetElementTypeId();
		}
		unsigned int VMWGrid::GetWidth() const
		{
			if (!IsValid())
				return 0;

			return Grid->GetWidth();
		}
		unsigned int VMWGrid::GetHeight() const
		{
			if (!IsValid())
				return 0;

			return Grid->GetHeight();
		}
		void VMWGrid::Resize(unsigned int Width, unsigned int Height)
		{
			if (!IsValid())
				return;

			return Grid->Resize(Width, Height);
		}
		void* VMWGrid::At(unsigned int X, unsigned int Y)
		{
			if (!IsValid())
				return nullptr;

			return Grid->At(X, Y);
		}
		const void* VMWGrid::At(unsigned int X, unsigned int Y) const
		{
			if (!IsValid())
				return nullptr;

			return Grid->At(X, Y);
		}
		void VMWGrid::SetValue(unsigned int X, unsigned int Y, void* Value)
		{
			if (!IsValid())
				return;

			return Grid->SetValue(X, Y, Value);
		}
		bool VMWGrid::IsValid() const
		{
			return Grid != nullptr;
		}
		VMCGrid* VMWGrid::GetGrid() const
		{
			return Grid;
		}
		VMWGrid VMWGrid::Create(const VMWTypeInfo& GridType)
		{
			return VMCGrid::Create(GridType.GetTypeInfo());
		}
		VMWGrid VMWGrid::Create(const VMWTypeInfo& GridType, unsigned int Width, unsigned int Height)
		{
			return VMCGrid::Create(GridType.GetTypeInfo(), Width, Height);
		}
		VMWGrid VMWGrid::Create(const VMWTypeInfo& GridType, unsigned int Width, unsigned int Height, void* DefaultValue)
		{
			return VMCGrid::Create(GridType.GetTypeInfo(), Width, Height, DefaultValue);
		}
		VMWGrid VMWGrid::Create(const VMWTypeInfo& GridType, void* ListBuffer)
		{
			return VMCGrid::Create(GridType.GetTypeInfo(), ListBuffer);
		}

		VMWWeakRef::VMWWeakRef(VMCWeakRef* Base) : WeakRef(Base)
		{
		}
		VMWWeakRef& VMWWeakRef::Set(void* NewRef)
		{
			if (!IsValid())
				return *this;

			WeakRef->Set(NewRef);
			return *this;
		}
		void* VMWWeakRef::Get() const
		{
			if (!IsValid())
				return nullptr;

			return WeakRef->Get();
		}
		bool VMWWeakRef::Equals(void* Ref) const
		{
			if (!IsValid())
				return false;

			return WeakRef->Equals(Ref);
		}
		VMWTypeInfo VMWWeakRef::GetRefType() const
		{
			if (!IsValid())
				return nullptr;

			return WeakRef->GetRefType();
		}
		bool VMWWeakRef::IsValid() const
		{
			return WeakRef != nullptr;
		}
		VMCWeakRef* VMWWeakRef::GetWeakRef() const
		{
			return WeakRef;
		}
		VMWWeakRef VMWWeakRef::Create(void* Ref, const VMWTypeInfo& TypeInfo)
		{
			return new VMCWeakRef(Ref, TypeInfo.GetTypeInfo());
		}

		VMWRef::VMWRef(VMCRef* Base) : Ref(Base)
		{
		}
		void VMWRef::Set(void* Ref1, const VMWTypeInfo& Type)
		{
			if (!IsValid())
				return;

			return Ref->Set(Ref1, Type.GetTypeInfo());
		}
		bool VMWRef::Equals(void* Ref1, int TypeId) const
		{
			if (!IsValid())
				return false;

			return Ref->Equals(Ref1, TypeId);
		}
		void VMWRef::Cast(void** OutRef, int TypeId)
		{
			if (!IsValid())
				return;

			return Ref->Cast(OutRef, TypeId);
		}
		VMWTypeInfo VMWRef::GetType() const
		{
			if (!IsValid())
				return nullptr;

			return Ref->GetType();
		}
		int VMWRef::GetTypeId() const
		{
			if (!IsValid())
				return -1;

			return Ref->GetTypeId();
		}
		void* VMWRef::GetHandle()
		{
			if (!IsValid())
				return nullptr;

			return Ref->GetRef();
		}
		bool VMWRef::IsValid() const
		{
			return Ref != nullptr;
		}
		VMCRef* VMWRef::GetRef() const
		{
			return Ref;
		}
		VMWRef VMWRef::Create(void* Ref, const VMWTypeInfo& TypeInfo)
		{
			return new VMCRef(Ref, TypeInfo.GetTypeInfo());
		}

		VMWGeneric::VMWGeneric(VMCGeneric* Base) : Generic(Base)
		{
			Manager = (Generic ? VMManager::Get(Generic->GetEngine()) : nullptr);
		}
		void* VMWGeneric::GetObjectAddress()
		{
			if (!IsValid())
				return nullptr;

			return Generic->GetObject();
		}
		int VMWGeneric::GetObjectTypeId() const
		{
			if (!IsValid())
				return -1;

			return Generic->GetObjectTypeId();
		}
		int VMWGeneric::GetArgsCount() const
		{
			if (!IsValid())
				return -1;

			return Generic->GetArgCount();
		}
		int VMWGeneric::GetArgTypeId(unsigned int Argument, size_t* Flags) const
		{
			if (!IsValid())
				return -1;

			asDWORD asFlags;
			int R = Generic->GetArgTypeId(Argument, &asFlags);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;

			return R;
		}
		unsigned char VMWGeneric::GetArgByte(unsigned int Argument)
		{
			if (!IsValid())
				return 0;

			return Generic->GetArgByte(Argument);
		}
		unsigned short VMWGeneric::GetArgWord(unsigned int Argument)
		{
			if (!IsValid())
				return 0;

			return Generic->GetArgWord(Argument);
		}
		size_t VMWGeneric::GetArgDWord(unsigned int Argument)
		{
			if (!IsValid())
				return 0;

			return Generic->GetArgDWord(Argument);
		}
		uint64_t VMWGeneric::GetArgQWord(unsigned int Argument)
		{
			if (!IsValid())
				return 0;

			return Generic->GetArgQWord(Argument);
		}
		float VMWGeneric::GetArgFloat(unsigned int Argument)
		{
			if (!IsValid())
				return 0.0f;

			return Generic->GetArgFloat(Argument);
		}
		double VMWGeneric::GetArgDouble(unsigned int Argument)
		{
			if (!IsValid())
				return 0.0;

			return Generic->GetArgDouble(Argument);
		}
		void* VMWGeneric::GetArgAddress(unsigned int Argument)
		{
			if (!IsValid())
				return nullptr;

			return Generic->GetArgAddress(Argument);
		}
		void* VMWGeneric::GetArgObjectAddress(unsigned int Argument)
		{
			if (!IsValid())
				return nullptr;

			return Generic->GetArgObject(Argument);
		}
		void* VMWGeneric::GetAddressOfArg(unsigned int Argument)
		{
			if (!IsValid())
				return nullptr;

			return Generic->GetAddressOfArg(Argument);
		}
		int VMWGeneric::GetReturnTypeId(size_t* Flags) const
		{
			if (!IsValid())
				return -1;

			asDWORD asFlags;
			int R = Generic->GetReturnTypeId(&asFlags);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;

			return R;
		}
		int VMWGeneric::SetReturnByte(unsigned char Value)
		{
			if (!IsValid())
				return -1;

			return Generic->SetReturnByte(Value);
		}
		int VMWGeneric::SetReturnWord(unsigned short Value)
		{
			if (!IsValid())
				return -1;

			return Generic->SetReturnWord(Value);
		}
		int VMWGeneric::SetReturnDWord(size_t Value)
		{
			if (!IsValid())
				return -1;

			return Generic->SetReturnDWord(Value);
		}
		int VMWGeneric::SetReturnQWord(uint64_t Value)
		{
			if (!IsValid())
				return -1;

			return Generic->SetReturnQWord(Value);
		}
		int VMWGeneric::SetReturnFloat(float Value)
		{
			if (!IsValid())
				return -1;

			return Generic->SetReturnFloat(Value);
		}
		int VMWGeneric::SetReturnDouble(double Value)
		{
			if (!IsValid())
				return -1;

			return Generic->SetReturnDouble(Value);
		}
		int VMWGeneric::SetReturnAddress(void* Address)
		{
			if (!IsValid())
				return -1;

			return Generic->SetReturnAddress(Address);
		}
		int VMWGeneric::SetReturnObjectAddress(void* Object)
		{
			if (!IsValid())
				return -1;

			return Generic->SetReturnObject(Object);
		}
		void* VMWGeneric::GetAddressOfReturnLocation()
		{
			if (!IsValid())
				return nullptr;

			return Generic->GetAddressOfReturnLocation();
		}
		bool VMWGeneric::IsValid() const
		{
			return Manager != nullptr && Generic != nullptr;
		}
		VMCGeneric* VMWGeneric::GetGeneric() const
		{
			return Generic;
		}
		VMManager* VMWGeneric::GetManager() const
		{
			return Manager;
		}

		VMWLockableSharedBool::VMWLockableSharedBool(VMCLockableSharedBool* Base) : Bool(Base)
		{
		}
		int VMWLockableSharedBool::AddRef() const
		{
			if (!IsValid())
				return -1;

			return Bool->AddRef();
		}
		int VMWLockableSharedBool::Release()
		{
			if (!IsValid())
				return -1;

			int R = Bool->Release();
			if (R <= 0)
				Bool = nullptr;

			return R;
		}
		bool VMWLockableSharedBool::Get() const
		{
			if (!IsValid())
				return false;

			return Bool->Get();
		}
		void VMWLockableSharedBool::Set(bool Value)
		{
			if (!IsValid())
				return;

			return Bool->Set(Value);
		}
		void VMWLockableSharedBool::Lock() const
		{
			if (!IsValid())
				return;

			return Bool->Lock();
		}
		void VMWLockableSharedBool::Unlock() const
		{
			if (!IsValid())
				return;

			return Bool->Unlock();
		}
		bool VMWLockableSharedBool::IsValid() const
		{
			return Bool != nullptr;
		}
		VMCLockableSharedBool* VMWLockableSharedBool::GetSharedBool() const
		{
			return Bool;
		}

		VMWClass::VMWClass(VMManager* Engine, const std::string& Name, int Type) : Manager(Engine), Object(Name), TypeId(Type)
		{
		}
		int VMWClass::SetFunctionDef(const char* Decl)
		{
			if (!IsValid() || !Decl)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			VMCTypeInfo* Info = Engine->GetTypeInfoByName(Object.c_str());
			const char* Namespace = Engine->GetDefaultNamespace();
			const char* Scope = Info->GetNamespace();

			Engine->SetDefaultNamespace(std::string(Scope).append("::").append(Object).c_str());
			int R = Engine->RegisterFuncdef(Decl);
			Engine->SetDefaultNamespace(Namespace);

			return R;
		}
		int VMWClass::SetOperatorCopyAddress(asSFuncPtr* Value)
		{
			if (!IsValid() || !Value)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			Rest::Stroke Decl = Rest::Form("%s& opAssign(const %s &in)", Object.c_str(), Object.c_str());
			return Engine->RegisterObjectMethod(Object.c_str(), Decl.Get(), *Value, asCALL_THISCALL);
		}
		int VMWClass::SetBehaviourAddress(const char* Decl, VMBehave Behave, asSFuncPtr* Value, VMCall Type)
		{
			if (!IsValid() || !Decl || !Value)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			return Engine->RegisterObjectBehaviour(Object.c_str(), (asEBehaviours)Behave, Decl, *Value, (asECallConvTypes)Type);
		}
		int VMWClass::SetPropertyAddress(const char* Decl, int Offset)
		{
			if (!IsValid() || !Decl)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			return Engine->RegisterObjectProperty(Object.c_str(), Decl, Offset);
		}
		int VMWClass::SetPropertyStaticAddress(const char* Decl, void* Value)
		{
			if (!IsValid() || !Decl || !Value)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			VMCTypeInfo* Info = Engine->GetTypeInfoByName(Object.c_str());
			const char* Namespace = Engine->GetDefaultNamespace();
			const char* Scope = Info->GetNamespace();

			Engine->SetDefaultNamespace(Scope[0] == '\0' ? Object.c_str() : std::string(Scope).append("::").append(Object).c_str());
			int R = Engine->RegisterGlobalProperty(Decl, Value);
			Engine->SetDefaultNamespace(Namespace);

			return R;
		}
		int VMWClass::SetOperatorAddress(const char* Decl, asSFuncPtr* Value, VMCall Type)
		{
			return SetMethodAddress(Decl, Value, Type);
		}
		int VMWClass::SetMethodAddress(const char* Decl, asSFuncPtr* Value, VMCall Type)
		{
			if (!IsValid() || !Decl)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			return Engine->RegisterObjectMethod(Object.c_str(), Decl, *Value, (asECallConvTypes)Type);
		}
		int VMWClass::SetMethodStaticAddress(const char* Decl, asSFuncPtr* Value, VMCall Type)
		{
			if (!IsValid() || !Decl)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			VMCTypeInfo* Info = Engine->GetTypeInfoByName(Object.c_str());
			const char* Namespace = Engine->GetDefaultNamespace();
			const char* Scope = Info->GetNamespace();

			Engine->SetDefaultNamespace(Scope[0] == '\0' ? Object.c_str() : std::string(Scope).append("::").append(Object).c_str());
			int R = Engine->RegisterGlobalFunction(Decl, *Value, (asECallConvTypes)Type);
			Engine->SetDefaultNamespace(Namespace);

			return R;

		}
		int VMWClass::SetConstructorAddress(const char* Decl, asSFuncPtr* Value, VMCall Type)
		{
			if (!IsValid() || !Decl)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			return Engine->RegisterObjectBehaviour(Object.c_str(), asBEHAVE_CONSTRUCT, Decl, *Value, (asECallConvTypes)Type);
		}
		int VMWClass::SetConstructorListAddress(const char* Decl, asSFuncPtr* Value, VMCall Type)
		{
			if (!IsValid() || !Decl)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			return Engine->RegisterObjectBehaviour(Object.c_str(), asBEHAVE_LIST_CONSTRUCT, Decl, *Value, (asECallConvTypes)Type);
		}
		int VMWClass::SetDestructorAddress(const char* Decl, asSFuncPtr* Value)
		{
			if (!IsValid() || !Decl)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			return Engine->RegisterObjectBehaviour(Object.c_str(), asBEHAVE_DESTRUCT, Decl, *Value, asCALL_CDECL_OBJFIRST);
		}
		int VMWClass::GetTypeId() const
		{
			return TypeId;
		}
		bool VMWClass::IsValid() const
		{
			return Manager != nullptr && TypeId >= 0;
		}
		std::string VMWClass::GetName() const
		{
			return Object;
		}
		VMManager* VMWClass::GetManager() const
		{
			return Manager;
		}
		Rest::Stroke VMWClass::GetOperator(VMOpFunc Op, const char* Out, const char* Args, bool Const, bool Right)
		{
			switch (Op)
			{
			case VMOpFunc_Neg:
				if (Right)
					return "";

				return Rest::Form("%s opNeg()%s", Out, Const ? " const" : "");
			case VMOpFunc_Com:
				if (Right)
					return "";

				return Rest::Form("%s opCom()%s", Out, Const ? " const" : "");
			case VMOpFunc_PreInc:
				if (Right)
					return "";

				return Rest::Form("%s opPreInc()%s", Out, Const ? " const" : "");
			case VMOpFunc_PreDec:
				if (Right)
					return "";

				return Rest::Form("%s opPreDec()%s", Out, Const ? " const" : "");
			case VMOpFunc_PostInc:
				if (Right)
					return "";

				return Rest::Form("%s opPostInc()%s", Out, Const ? " const" : "");
			case VMOpFunc_PostDec:
				if (Right)
					return "";

				return Rest::Form("%s opPostDec()%s", Out, Const ? " const" : "");
			case VMOpFunc_Equals:
				if (Right)
					return "";

				return Rest::Form("%s opEquals(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_Cmp:
				if (Right)
					return "";

				return Rest::Form("%s opCmp(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_Assign:
				if (Right)
					return "";

				return Rest::Form("%s opAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_AddAssign:
				if (Right)
					return "";

				return Rest::Form("%s opAddAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_SubAssign:
				if (Right)
					return "";

				return Rest::Form("%s opSubAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_MulAssign:
				if (Right)
					return "";

				return Rest::Form("%s opMulAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_DivAssign:
				if (Right)
					return "";

				return Rest::Form("%s opDivAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_ModAssign:
				if (Right)
					return "";

				return Rest::Form("%s opModAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_PowAssign:
				if (Right)
					return "";

				return Rest::Form("%s opPowAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_AndAssign:
				if (Right)
					return "";

				return Rest::Form("%s opAndAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_OrAssign:
				if (Right)
					return "";

				return Rest::Form("%s opOrAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_XOrAssign:
				if (Right)
					return "";

				return Rest::Form("%s opXorAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_ShlAssign:
				if (Right)
					return "";

				return Rest::Form("%s opShlAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_ShrAssign:
				if (Right)
					return "";

				return Rest::Form("%s opShrAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_UshrAssign:
				if (Right)
					return "";

				return Rest::Form("%s opUshrAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_Add:
				return Rest::Form("%s opAdd%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_Sub:
				return Rest::Form("%s opSub%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_Mul:
				return Rest::Form("%s opMul%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_Div:
				return Rest::Form("%s opDiv%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_Mod:
				return Rest::Form("%s opMod%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_Pow:
				return Rest::Form("%s opPow%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_And:
				return Rest::Form("%s opAnd%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_Or:
				return Rest::Form("%s opOr%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_XOr:
				return Rest::Form("%s opXor%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_Shl:
				return Rest::Form("%s opShl%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_Shr:
				return Rest::Form("%s opShr%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_Ushr:
				return Rest::Form("%s opUshr%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_Index:
				if (Right)
					return "";

				return Rest::Form("%s opIndex(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_Call:
				if (Right)
					return "";

				return Rest::Form("%s opCall(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
			case VMOpFunc_Cast:
				if (Right)
					return "";

				return Rest::Form("%s opCast()%s", Out, Const ? " const" : "");
			case VMOpFunc_ImplCast:
				if (Right)
					return "";

				return Rest::Form("%s opImplCast()%s", Out, Const ? " const" : "");
			default:
				return "";
			}
		}

		VMWInterface::VMWInterface(VMManager* Engine, const std::string& Name, int Type) : Manager(Engine), Object(Name), TypeId(Type)
		{
		}
		int VMWInterface::SetMethod(const char* Decl)
		{
			if (!IsValid() || !Decl)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			return Engine->RegisterInterfaceMethod(Object.c_str(), Decl);
		}
		int VMWInterface::GetTypeId() const
		{
			return TypeId;
		}
		bool VMWInterface::IsValid() const
		{
			return Manager != nullptr && TypeId >= 0;
		}
		std::string VMWInterface::GetName() const
		{
			return Object;
		}
		VMManager* VMWInterface::GetManager() const
		{
			return Manager;
		}

		VMWEnum::VMWEnum(VMManager* Engine, const std::string& Name, int Type) : Manager(Engine), Object(Name), TypeId(Type)
		{
		}
		int VMWEnum::SetValue(const char* Name, int Value)
		{
			if (!IsValid() || !Name)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			return Engine->RegisterEnumValue(Object.c_str(), Name, Value);
		}
		int VMWEnum::GetTypeId() const
		{
			return TypeId;
		}
		bool VMWEnum::IsValid() const
		{
			return Manager != nullptr && TypeId >= 0;
		}
		std::string VMWEnum::GetName() const
		{
			return Object;
		}
		VMManager* VMWEnum::GetManager() const
		{
			return Manager;
		}

		VMWModule::VMWModule(VMCModule* Type) : Mod(Type)
		{
			Manager = (Mod ? VMManager::Get(Mod->GetEngine()) : nullptr);
		}
		void VMWModule::SetName(const char* Name)
		{
			if (!IsValid())
				return;

			return Mod->SetName(Name);
		}
		int VMWModule::AddSection(const char* Name, const char* Code, size_t CodeLength, int LineOffset)
		{
			if (!IsValid())
				return -1;

			return Mod->AddScriptSection(Name, Code, CodeLength, LineOffset);
		}
		int VMWModule::RemoveFunction(const VMWFunction& Function)
		{
			if (!IsValid())
				return -1;

			return Mod->RemoveFunction(Function.GetFunction());
		}
		int VMWModule::ResetProperties(VMCContext* Context)
		{
			if (!IsValid())
				return -1;

			return Mod->ResetGlobalVars(Context);
		}
		int VMWModule::Build()
		{
			if (!IsValid())
				return -1;

			return Mod->Build();
		}
		int VMWModule::LoadByteCode(VMByteCode* Info)
		{
			if (!Info || !IsValid())
				return -1;

			ByteCodeStream* Stream = new ByteCodeStream(Info->Data);
			int R = Mod->LoadByteCode(Stream, &Info->Debug);
			delete Stream;

			return R;
		}
		int VMWModule::Discard()
		{
			if (!IsValid())
				return -1;

			Mod->Discard();
			Mod = nullptr;

			return 0;
		}
		int VMWModule::BindImportedFunction(size_t ImportIndex, const VMWFunction& Function)
		{
			if (!IsValid())
				return -1;

			return Mod->BindImportedFunction((asUINT)ImportIndex, Function.GetFunction());
		}
		int VMWModule::UnbindImportedFunction(size_t ImportIndex)
		{
			if (!IsValid())
				return -1;

			return Mod->UnbindImportedFunction((asUINT)ImportIndex);
		}
		int VMWModule::BindAllImportedFunctions()
		{
			if (!IsValid())
				return -1;

			return Mod->BindAllImportedFunctions();
		}
		int VMWModule::UnbindAllImportedFunctions()
		{
			if (!IsValid())
				return -1;

			return Mod->UnbindAllImportedFunctions();
		}
		int VMWModule::CompileFunction(const char* SectionName, const char* Code, int LineOffset, size_t CompileFlags, VMWFunction* OutFunction)
		{
			if (!IsValid())
				return -1;

			VMCFunction* OutFunc = nullptr;
			int R = Mod->CompileFunction(SectionName, Code, LineOffset, (asDWORD)CompileFlags, &OutFunc);

			if (OutFunction != nullptr)
				*OutFunction = VMWFunction(OutFunc);

			return R;
		}
		int VMWModule::CompileProperty(const char* SectionName, const char* Code, int LineOffset)
		{
			if (!IsValid())
				return -1;

			return Mod->CompileGlobalVar(SectionName, Code, LineOffset);
		}
		int VMWModule::SetDefaultNamespace(const char* NameSpace)
		{
			if (!IsValid())
				return -1;

			return Mod->SetDefaultNamespace(NameSpace);
		}
		void* VMWModule::GetAddressOfProperty(size_t Index)
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetAddressOfGlobalVar(Index);
		}
		int VMWModule::RemoveProperty(size_t Index)
		{
			if (!IsValid())
				return -1;

			return Mod->RemoveGlobalVar(Index);
		}
		size_t VMWModule::SetAccessMask(size_t AccessMask)
		{
			if (!IsValid())
				return 0;

			return Mod->SetAccessMask((asDWORD)AccessMask);
		}
		size_t VMWModule::GetFunctionCount() const
		{
			if (!IsValid())
				return 0;

			return Mod->GetFunctionCount();
		}
		VMWFunction VMWModule::GetFunctionByIndex(size_t Index) const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetFunctionByIndex((asUINT)Index);
		}
		VMWFunction VMWModule::GetFunctionByDecl(const char* Decl) const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetFunctionByDecl(Decl);
		}
		VMWFunction VMWModule::GetFunctionByName(const char* Name) const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetFunctionByName(Name);
		}
		int VMWModule::GetTypeIdByDecl(const char* Decl) const
		{
			if (!IsValid())
				return -1;

			return Mod->GetTypeIdByDecl(Decl);
		}
		int VMWModule::GetImportedFunctionIndexByDecl(const char* Decl) const
		{
			if (!IsValid())
				return -1;

			return Mod->GetImportedFunctionIndexByDecl(Decl);
		}
		int VMWModule::SaveByteCode(VMByteCode* Info) const
		{
			if (!Info || !IsValid())
				return -1;

			ByteCodeStream* Stream = new ByteCodeStream();
			int R = Mod->SaveByteCode(Stream, Info->Debug);
			Info->Data = Stream->GetCode();
			delete Stream;

			return R;
		}
		int VMWModule::GetPropertyIndexByName(const char* Name) const
		{
			if (!IsValid())
				return -1;

			return Mod->GetGlobalVarIndexByName(Name);
		}
		int VMWModule::GetPropertyIndexByDecl(const char* Decl) const
		{
			if (!IsValid())
				return -1;

			return Mod->GetGlobalVarIndexByDecl(Decl);
		}
		int VMWModule::GetProperty(size_t Index, VMProperty* Info) const
		{
			if (!Manager)
				return -1;

			const char* Name = nullptr;
			const char* NameSpace = nullptr;
			bool IsConst = false;
			int TypeId = 0;
			int Result = Mod->GetGlobalVar(Index, &Name, &NameSpace, &TypeId, &IsConst);

			if (Info != nullptr)
			{
				Info->Name = Name;
				Info->NameSpace = NameSpace;
				Info->TypeId = TypeId;
				Info->IsConst = IsConst;
				Info->ConfigGroup = nullptr;
				Info->Pointer = Mod->GetAddressOfGlobalVar(Index);
				Info->AccessMask = GetAccessMask();
			}

			return Result;
		}
		size_t VMWModule::GetAccessMask() const
		{
			if (!Mod)
				return 0;

			asDWORD AccessMask = Mod->SetAccessMask(0);
			Mod->SetAccessMask(AccessMask);
			return (size_t)AccessMask;
		}
		size_t VMWModule::GetObjectsCount() const
		{
			if (!IsValid())
				return 0;

			return Mod->GetObjectTypeCount();
		}
		size_t VMWModule::GetPropertiesCount() const
		{
			if (!IsValid())
				return 0;

			return Mod->GetGlobalVarCount();
		}
		size_t VMWModule::GetEnumsCount() const
		{
			if (!IsValid())
				return 0;

			return Mod->GetEnumCount();
		}
		size_t VMWModule::GetImportedFunctionCount() const
		{
			if (!IsValid())
				return 0;

			return Mod->GetImportedFunctionCount();
		}
		VMWTypeInfo VMWModule::GetObjectByIndex(size_t Index) const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetObjectTypeByIndex(Index);
		}
		VMWTypeInfo VMWModule::GetTypeInfoByName(const char* Name) const
		{
			if (!IsValid())
				return nullptr;

			const char* TypeName = Name;
			const char* Namespace = nullptr;
			size_t NamespaceSize = 0;

			if (Manager->GetTypeNameScope(&TypeName, &Namespace, &NamespaceSize) != 0)
				return Mod->GetTypeInfoByName(Name);

			Manager->BeginNamespace(std::string(Namespace, NamespaceSize).c_str());
			VMCTypeInfo* Info = Mod->GetTypeInfoByName(TypeName);
			Manager->EndNamespace();

			return Info;
		}
		VMWTypeInfo VMWModule::GetTypeInfoByDecl(const char* Decl) const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetTypeInfoByDecl(Decl);
		}
		VMWTypeInfo VMWModule::GetEnumByIndex(size_t Index) const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetEnumByIndex(Index);
		}
		const char* VMWModule::GetPropertyDecl(size_t Index, bool IncludeNamespace) const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetGlobalVarDeclaration(Index, IncludeNamespace);
		}
		const char* VMWModule::GetDefaultNamespace() const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetDefaultNamespace();
		}
		const char* VMWModule::GetImportedFunctionDecl(size_t ImportIndex) const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetImportedFunctionDeclaration(ImportIndex);
		}
		const char* VMWModule::GetImportedFunctionModule(size_t ImportIndex) const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetImportedFunctionSourceModule(ImportIndex);
		}
		const char* VMWModule::GetName() const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetName();
		}
		bool VMWModule::IsValid() const
		{
			return Manager != nullptr && Mod != nullptr;
		}
		VMCModule* VMWModule::GetModule() const
		{
			return Mod;
		}
		VMManager* VMWModule::GetManager() const
		{
			return Manager;
		}

		VMGlobal::VMGlobal(VMManager* Engine) : Manager(Engine)
		{
		}
		int VMGlobal::SetFunctionDef(const char* Decl)
		{
			if (!Manager || !Decl)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			return Engine->RegisterFuncdef(Decl);
		}
		int VMGlobal::SetFunctionAddress(const char* Decl, asSFuncPtr* Value, VMCall Type)
		{
			if (!Manager || !Decl || !Value)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			return Engine->RegisterGlobalFunction(Decl, *Value, (asECallConvTypes)Type);
		}
		int VMGlobal::SetPropertyAddress(const char* Decl, void* Value)
		{
			if (!Manager || !Decl)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			return Engine->RegisterGlobalProperty(Decl, Value);
		}
		VMWInterface VMGlobal::SetInterface(const char* Name)
		{
			if (!Manager || !Name)
				return VMWInterface(Manager, "", -1);

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return VMWInterface(Manager, Name, -1);

			return VMWInterface(Manager, Name, Engine->RegisterInterface(Name));
		}
		VMWTypeClass VMGlobal::SetStructAddress(const char* Name, size_t Size, uint64_t Flags)
		{
			if (!Manager || !Name)
				return VMWTypeClass(Manager, "", -1);

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return VMWTypeClass(Manager, Name, -1);

			return VMWTypeClass(Manager, Name, Engine->RegisterObjectType(Name, Size, (asDWORD)Flags));
		}
		VMWTypeClass VMGlobal::SetPodAddress(const char* Name, size_t Size, uint64_t Flags)
		{
			if (!Manager || !Name)
				return VMWTypeClass(Manager, "", -1);

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return VMWTypeClass(Manager, Name, -1);

			return VMWTypeClass(Manager, Name, Engine->RegisterObjectType(Name, Size, (asDWORD)Flags));
		}
		VMWRefClass VMGlobal::SetClassAddress(const char* Name, uint64_t Flags)
		{
			if (!Manager || !Name)
				return VMWRefClass(Manager, "", -1);

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return VMWRefClass(Manager, Name, -1);

			return VMWRefClass(Manager, Name, Engine->RegisterObjectType(Name, 0, (asDWORD)Flags));
		}
		VMWEnum VMGlobal::SetEnum(const char* Name)
		{
			if (!Manager || !Name)
				return VMWEnum(Manager, "", -1);

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return VMWEnum(Manager, Name, -1);

			return VMWEnum(Manager, Name, Engine->RegisterEnum(Name));
		}
		size_t VMGlobal::GetFunctionsCount() const
		{
			if (!Manager)
				return 0;

			return Manager->GetEngine()->GetGlobalFunctionCount();
		}
		VMWFunction VMGlobal::GetFunctionById(int Id) const
		{
			if (!Manager)
				return nullptr;

			return Manager->GetEngine()->GetFunctionById(Id);
		}
		VMWFunction VMGlobal::GetFunctionByIndex(int Index) const
		{
			if (!Manager)
				return nullptr;

			return Manager->GetEngine()->GetGlobalFunctionByIndex(Index);
		}
		VMWFunction VMGlobal::GetFunctionByDecl(const char* Decl) const
		{
			if (!Manager || !Decl)
				return nullptr;

			return Manager->GetEngine()->GetGlobalFunctionByDecl(Decl);
		}
		size_t VMGlobal::GetPropertiesCount() const
		{
			if (!Manager)
				return 0;

			return Manager->GetEngine()->GetGlobalPropertyCount();
		}
		int VMGlobal::GetPropertyByIndex(int Index, VMProperty* Info) const
		{
			if (!Manager)
				return -1;

			const char* Name = nullptr, * NameSpace = nullptr;
			const char* ConfigGroup = nullptr;
			void* Pointer = nullptr;
			bool IsConst = false;
			asDWORD AccessMask = 0;
			int TypeId = 0;
			int Result = Manager->GetEngine()->GetGlobalPropertyByIndex(Index, &Name, &NameSpace, &TypeId, &IsConst, &ConfigGroup, &Pointer, &AccessMask);

			if (Info != nullptr)
			{
				Info->Name = Name;
				Info->NameSpace = NameSpace;
				Info->TypeId = TypeId;
				Info->IsConst = IsConst;
				Info->ConfigGroup = ConfigGroup;
				Info->Pointer = Pointer;
				Info->AccessMask = AccessMask;
			}

			return Result;
		}
		int VMGlobal::GetPropertyIndexByName(const char* Name) const
		{
			if (!Manager || !Name)
				return -1;

			return Manager->GetEngine()->GetGlobalPropertyIndexByName(Name);
		}
		int VMGlobal::GetPropertyIndexByDecl(const char* Decl) const
		{
			if (!Manager || !Decl)
				return -1;

			return Manager->GetEngine()->GetGlobalPropertyIndexByDecl(Decl);
		}
		size_t VMGlobal::GetObjectsCount() const
		{
			if (!Manager)
				return 0;

			return Manager->GetEngine()->GetObjectTypeCount();
		}
		VMWTypeInfo VMGlobal::GetObjectByTypeId(int TypeId) const
		{
			if (!Manager)
				return nullptr;

			return Manager->GetEngine()->GetObjectTypeByIndex(TypeId);
		}
		size_t VMGlobal::GetEnumCount() const
		{
			if (!Manager)
				return 0;

			return Manager->GetEngine()->GetEnumCount();
		}
		VMWTypeInfo VMGlobal::GetEnumByTypeId(int TypeId) const
		{
			if (!Manager)
				return nullptr;

			return Manager->GetEngine()->GetEnumByIndex(TypeId);
		}
		size_t VMGlobal::GetFunctionDefsCount() const
		{
			if (!Manager)
				return 0;

			return Manager->GetEngine()->GetFuncdefCount();
		}
		VMWTypeInfo VMGlobal::GetFunctionDefByIndex(int Index) const
		{
			if (!Manager)
				return nullptr;

			return Manager->GetEngine()->GetFuncdefByIndex(Index);
		}
		size_t VMGlobal::GetModulesCount() const
		{
			if (!Manager)
				return 0;

			return Manager->GetEngine()->GetModuleCount();
		}
		VMCModule* VMGlobal::GetModuleById(int Id) const
		{
			if (!Manager)
				return nullptr;

			return Manager->GetEngine()->GetModuleByIndex(Id);
		}
		int VMGlobal::GetTypeIdByDecl(const char* Decl) const
		{
			if (!Manager || !Decl)
				return -1;

			return Manager->GetEngine()->GetTypeIdByDecl(Decl);
		}
		const char* VMGlobal::GetTypeIdDecl(int TypeId, bool IncludeNamespace) const
		{
			if (!Manager)
				return nullptr;

			return Manager->GetEngine()->GetTypeDeclaration(TypeId, IncludeNamespace);
		}
		int VMGlobal::GetSizeOfPrimitiveType(int TypeId) const
		{
			if (!Manager)
				return -1;

			return Manager->GetEngine()->GetSizeOfPrimitiveType(TypeId);
		}
		std::string VMGlobal::GetObjectView(void* Object, int TypeId)
		{
			if (!Manager || !Object)
				return "null";

			if (TypeId == VMTypeId_INT8)
				return "int8(" + std::to_string(*(char*)(Object)) + "), ";
			else if (TypeId == VMTypeId_INT16)
				return "int16(" + std::to_string(*(short*)(Object)) + "), ";
			else if (TypeId == VMTypeId_INT32)
				return "int32(" + std::to_string(*(int*)(Object)) + "), ";
			else if (TypeId == VMTypeId_INT64)
				return "int64(" + std::to_string(*(int64_t*)(Object)) + "), ";
			else if (TypeId == VMTypeId_UINT8)
				return "uint8(" + std::to_string(*(unsigned char*)(Object)) + "), ";
			else if (TypeId == VMTypeId_UINT16)
				return "uint16(" + std::to_string(*(unsigned short*)(Object)) + "), ";
			else if (TypeId == VMTypeId_UINT32)
				return "uint32(" + std::to_string(*(unsigned int*)(Object)) + "), ";
			else if (TypeId == VMTypeId_UINT64)
				return "uint64(" + std::to_string(*(uint64_t*)(Object)) + "), ";
			else if (TypeId == VMTypeId_FLOAT)
				return "float(" + std::to_string(*(float*)(Object)) + "), ";
			else if (TypeId == VMTypeId_DOUBLE)
				return "double(" + std::to_string(*(double*)(Object)) + "), ";

			VMCTypeInfo* Type = Manager->GetEngine()->GetTypeInfoById(TypeId);
			const char* Name = Type->GetName();

			return Rest::Form("%s(%d)", Object, Name ? Name : "any").R();
		}
		std::string VMGlobal::GetObjectView(const VMWAny& Any)
		{
			if (!Any.IsValid())
				return GetObjectView(nullptr, -1);

			void* Object = nullptr;
			int TypeId = 0;
			Any.RetrieveAny(&Object, &TypeId);
			return GetObjectView(Object, TypeId);
		}
		VMWTypeInfo VMGlobal::GetTypeInfoById(int TypeId) const
		{
			if (!Manager)
				return nullptr;

			return Manager->GetEngine()->GetTypeInfoById(TypeId);
		}
		VMWTypeInfo VMGlobal::GetTypeInfoByName(const char* Name) const
		{
			if (!Manager || !Name)
				return nullptr;

			const char* TypeName = Name;
			const char* Namespace = nullptr;
			size_t NamespaceSize = 0;

			if (Manager->GetTypeNameScope(&TypeName, &Namespace, &NamespaceSize) != 0)
				return Manager->GetEngine()->GetTypeInfoByName(Name);

			Manager->BeginNamespace(std::string(Namespace, NamespaceSize).c_str());
			VMCTypeInfo* Info = Manager->GetEngine()->GetTypeInfoByName(TypeName);
			Manager->EndNamespace();

			return Info;
		}
		VMWTypeInfo VMGlobal::GetTypeInfoByDecl(const char* Decl) const
		{
			if (!Manager || !Decl)
				return nullptr;

			return Manager->GetEngine()->GetTypeInfoByDecl(Decl);
		}
		VMManager* VMGlobal::GetManager() const
		{
			return Manager;
		}

		VMCompiler::VMCompiler(VMManager* Engine) : Manager(Engine), Context(nullptr), Module(nullptr), Features(0), BuiltOK(false)
		{
			Processor = new Compute::Preprocessor();
			Processor->SetIncludeCallback([this](Compute::Preprocessor* C, const Compute::IncludeResult& File, std::string* Out)
			{
				if (Include && Include(C, File, Out))
					return true;

				if (File.Module.empty() || (!File.IsFile && File.IsSystem) || !Module)
					return false;

				std::string Buffer;
				if (!Manager->ImportFile(File.Module, &Buffer))
					return false;

				if (!C->Process(File.Module, Buffer))
					return false;

				return Module->AddScriptSection(File.Module.c_str(), Buffer.c_str(), Buffer.size()) >= 0;
			});
			Processor->SetPragmaCallback([this](Compute::Preprocessor* C, const std::string& Value)
			{
				if (!Manager)
					return false;

				if (Pragma && Pragma(C, Value))
					return true;

				Rest::Stroke Comment(&Value);
				Comment.Trim();

				auto Start = Comment.Find('(');
				if (!Start.Found)
					return false;

				auto End = Comment.ReverseFind(')');
				if (!End.Found)
					return false;

				if (Comment.StartsWith("define"))
				{
					Rest::Stroke Name(Comment);
					Name.Substring(Start.End, End.Start - Start.End).Trim();
					if (Name.Get()[0] == '\"' && Name.Get()[Name.Size() - 1] == '\"')
						Name.Substring(1, Name.Size() - 2);

					if (!Name.Empty())
						Define(Name.R());
				}
				else if (Comment.StartsWith("compile"))
				{
					auto Split = Comment.Find(',', Start.End);
					if (!Split.Found)
						return false;

					Rest::Stroke Name(Comment);
					Name.Substring(Start.End, Split.Start - Start.End).Trim().ToUpper();
					if (Name.Get()[0] == '\"' && Name.Get()[Name.Size() - 1] == '\"')
						Name.Substring(1, Name.Size() - 2);

					Rest::Stroke Value(Comment);
					Value.Substring(Split.End, End.Start - Split.End).Trim();
					if (Value.Get()[0] == '\"' && Value.Get()[Value.Size() - 1] == '\"')
						Value.Substring(1, Value.Size() - 2);

					size_t Result = Value.HasInteger() ? Value.ToUInt64() : 0;
					if (Name.R() == "ALLOW_UNSAFE_REFERENCES")
						Manager->SetProperty(VMProp_ALLOW_UNSAFE_REFERENCES, Result);
					else if (Name.R() == "OPTIMIZE_BYTECODE")
						Manager->SetProperty(VMProp_OPTIMIZE_BYTECODE, Result);
					else if (Name.R() == "COPY_SCRIPT_SECTIONS")
						Manager->SetProperty(VMProp_COPY_SCRIPT_SECTIONS, Result);
					else if (Name.R() == "MAX_STACK_SIZE")
						Manager->SetProperty(VMProp_MAX_STACK_SIZE, Result);
					else if (Name.R() == "USE_CHARACTER_LITERALS")
						Manager->SetProperty(VMProp_USE_CHARACTER_LITERALS, Result);
					else if (Name.R() == "ALLOW_MULTILINE_STRINGS")
						Manager->SetProperty(VMProp_ALLOW_MULTILINE_STRINGS, Result);
					else if (Name.R() == "ALLOW_IMPLICIT_HANDLE_TYPES")
						Manager->SetProperty(VMProp_ALLOW_IMPLICIT_HANDLE_TYPES, Result);
					else if (Name.R() == "BUILD_WITHOUT_LINE_CUES")
						Manager->SetProperty(VMProp_BUILD_WITHOUT_LINE_CUES, Result);
					else if (Name.R() == "INIT_GLOBAL_VARS_AFTER_BUILD")
						Manager->SetProperty(VMProp_INIT_GLOBAL_VARS_AFTER_BUILD, Result);
					else if (Name.R() == "REQUIRE_ENUM_SCOPE")
						Manager->SetProperty(VMProp_REQUIRE_ENUM_SCOPE, Result);
					else if (Name.R() == "SCRIPT_SCANNER")
						Manager->SetProperty(VMProp_SCRIPT_SCANNER, Result);
					else if (Name.R() == "INCLUDE_JIT_INSTRUCTIONS")
						Manager->SetProperty(VMProp_INCLUDE_JIT_INSTRUCTIONS, Result);
					else if (Name.R() == "STRING_ENCODING")
						Manager->SetProperty(VMProp_STRING_ENCODING, Result);
					else if (Name.R() == "PROPERTY_ACCESSOR_MODE")
						Manager->SetProperty(VMProp_PROPERTY_ACCESSOR_MODE, Result);
					else if (Name.R() == "EXPAND_DEF_ARRAY_TO_TMPL")
						Manager->SetProperty(VMProp_EXPAND_DEF_ARRAY_TO_TMPL, Result);
					else if (Name.R() == "AUTO_GARBAGE_COLLECT")
						Manager->SetProperty(VMProp_AUTO_GARBAGE_COLLECT, Result);
					else if (Name.R() == "DISALLOW_GLOBAL_VARS")
						Manager->SetProperty(VMProp_ALWAYS_IMPL_DEFAULT_CONSTRUCT, Result);
					else if (Name.R() == "ALWAYS_IMPL_DEFAULT_CONSTRUCT")
						Manager->SetProperty(VMProp_ALWAYS_IMPL_DEFAULT_CONSTRUCT, Result);
					else if (Name.R() == "COMPILER_WARNINGS")
						Manager->SetProperty(VMProp_COMPILER_WARNINGS, Result);
					else if (Name.R() == "DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE")
						Manager->SetProperty(VMProp_DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE, Result);
					else if (Name.R() == "ALTER_SYNTAX_NAMED_ARGS")
						Manager->SetProperty(VMProp_ALTER_SYNTAX_NAMED_ARGS, Result);
					else if (Name.R() == "DISABLE_INTEGER_DIVISION")
						Manager->SetProperty(VMProp_DISABLE_INTEGER_DIVISION, Result);
					else if (Name.R() == "DISALLOW_EMPTY_LIST_ELEMENTS")
						Manager->SetProperty(VMProp_DISALLOW_EMPTY_LIST_ELEMENTS, Result);
					else if (Name.R() == "PRIVATE_PROP_AS_PROTECTED")
						Manager->SetProperty(VMProp_PRIVATE_PROP_AS_PROTECTED, Result);
					else if (Name.R() == "ALLOW_UNICODE_IDENTIFIERS")
						Manager->SetProperty(VMProp_ALLOW_UNICODE_IDENTIFIERS, Result);
					else if (Name.R() == "HEREDOC_TRIM_MODE")
						Manager->SetProperty(VMProp_HEREDOC_TRIM_MODE, Result);
					else if (Name.R() == "MAX_NESTED_CALLS")
						Manager->SetProperty(VMProp_MAX_NESTED_CALLS, Result);
					else if (Name.R() == "GENERIC_CALL_MODE")
						Manager->SetProperty(VMProp_GENERIC_CALL_MODE, Result);
					else if (Name.R() == "INIT_STACK_SIZE")
						Manager->SetProperty(VMProp_INIT_STACK_SIZE, Result);
					else if (Name.R() == "INIT_CALL_STACK_SIZE")
						Manager->SetProperty(VMProp_INIT_CALL_STACK_SIZE, Result);
					else if (Name.R() == "MAX_CALL_STACK_SIZE")
						Manager->SetProperty(VMProp_MAX_CALL_STACK_SIZE, Result);
				}
				else if (Comment.StartsWith("comment"))
				{
					auto Split = Comment.Find(',', Start.End);
					if (!Split.Found)
						return false;

					Rest::Stroke Name(Comment);
					Name.Substring(Start.End, Split.Start - Start.End).Trim().ToUpper();
					if (Name.Get()[0] == '\"' && Name.Get()[Name.Size() - 1] == '\"')
						Name.Substring(1, Name.Size() - 2);

					Rest::Stroke Value(Comment);
					Value.Substring(Split.End, End.Start - Split.End).Trim();
					if (Value.Get()[0] == '\"' && Value.Get()[Value.Size() - 1] == '\"')
						Value.Substring(1, Value.Size() - 2);

					if (Name.R() == "INFO")
						THAWK_INFO("%s", Value.Get());
					else if (Name.R() == "WARN")
						THAWK_WARN("%s", Value.Get());
					else if (Name.R() == "ERROR")
						THAWK_ERROR("%s", Value.Get());
				}
				else if (Comment.StartsWith("modify"))
				{
					auto Split = Comment.Find(',', Start.End);
					if (!Split.Found || !Module)
						return false;

					Rest::Stroke Name(Comment);
					Name.Substring(Start.End, Split.Start - Start.End).Trim().ToUpper();
					if (Name.Get()[0] == '\"' && Name.Get()[Name.Size() - 1] == '\"')
						Name.Substring(1, Name.Size() - 2);

					Rest::Stroke Value(Comment);
					Value.Substring(Split.End, End.Start - Split.End).Trim();
					if (Value.Get()[0] == '\"' && Value.Get()[Value.Size() - 1] == '\"')
						Value.Substring(1, Value.Size() - 2);

					size_t Result = Value.HasInteger() ? Value.ToUInt64() : 0;
					if (Name.R() == "NAME")
						Module->SetName(Value.Value());
					else if (Name.R() == "NAMESPACE")
						Module->SetDefaultNamespace(Value.Value());
					else if (Name.R() == "ACCESS_MASK")
						Module->SetAccessMask(Result);
				}
				else if (Comment.StartsWith("enable"))
				{
					Rest::Stroke Name(Comment);
					Name.Substring(Start.End, End.Start - Start.End).Trim();
					if (Name.Get()[0] == '\"' && Name.Get()[Name.Size() - 1] == '\"')
						Name.Substring(1, Name.Size() - 2);

					if (Name.Empty())
						return false;

					bool All = Name.Find("all").Found;
					uint64_t Want = 0;

					if (All || Name.Find("string").Found)
					{
						Want |= VMFeature_String;
						if (!(Features & VMFeature_String) && Features != VMFeature_All)
						{
							THAWK_ERROR("feature \"string\" is not allowed");
							return false;
						}
					}

					if (All || Name.Find("array").Found)
					{
						Want |= VMFeature_Array;
						if (!(Features & VMFeature_Array) && Features != VMFeature_All)
						{
							THAWK_ERROR("feature \"array\" is not allowed");
							return false;
						}
					}

					if (All || Name.Find("any").Found)
					{
						Want |= VMFeature_Any;
						if (!(Features & VMFeature_Any) && Features != VMFeature_All)
						{
							THAWK_ERROR("feature \"any\" is not allowed");
							return false;
						}
					}

					if (All || Name.Find("dictionary").Found)
					{
						Want |= VMFeature_Dictionary;
						if (!(Features & VMFeature_Dictionary) && Features != VMFeature_All)
						{
							THAWK_ERROR("feature \"dictionary\" is not allowed");
							return false;
						}
					}

					if (All || Name.Find("grid").Found)
					{
						Want |= VMFeature_Grid;
						if (!(Features & VMFeature_Grid) && Features != VMFeature_All)
						{
							THAWK_ERROR("feature \"grid\" is not allowed");
							return false;
						}
					}

					if (All || Name.Find("math").Found)
					{
						Want |= VMFeature_Math;
						if (!(Features & VMFeature_Math) && Features != VMFeature_All)
						{
							THAWK_ERROR("feature \"math\" is not allowed");
							return false;
						}
					}

					if (All || Name.Find("exception").Found)
					{
						Want |= VMFeature_Exception;
						if (!(Features & VMFeature_Exception) && Features != VMFeature_All)
						{
							THAWK_ERROR("feature \"exception\" is not allowed");
							return false;
						}
					}

					if (All || Name.Find("type-reference").Found)
					{
						Want |= VMFeature_Reference;
						if (!(Features & VMFeature_Reference) && Features != VMFeature_All)
						{
							THAWK_ERROR("feature \"type-reference\" is not allowed");
							return false;
						}
					}

					if (All || Name.Find("weak-reference").Found)
					{
						Want |= VMFeature_WeakReference;
						if (!(Features & VMFeature_WeakReference) && Features != VMFeature_All)
						{
							THAWK_ERROR("feature \"weak-reference\" is not allowed");
							return false;
						}
					}

					if (All || Name.Find("random").Found)
					{
						Want |= VMFeature_Random;
						if (!(Features & VMFeature_Random) && Features != VMFeature_All)
						{
							THAWK_ERROR("feature \"random\" is not allowed");
							return false;
						}
					}

					if (All || Name.Find("thread").Found)
					{
						Want |= VMFeature_Thread;
						if (!(Features & VMFeature_Thread) && Features != VMFeature_All)
						{
							THAWK_ERROR("feature \"thread\" is not allowed");
							return false;
						}
					}

					if (All || Name.Find("async").Found)
					{
						Want |= VMFeature_Async;
						if (!(Features & VMFeature_Async) && Features != VMFeature_All)
						{
							THAWK_ERROR("feature \"async\" is not allowed");
							return false;
						}
					}

					if (All || Name.Find("complex").Found)
					{
						Want |= VMFeature_Complex;
						if (!(Features & VMFeature_Complex) && Features != VMFeature_All)
						{
							THAWK_ERROR("feature \"complex\" is not allowed");
							return false;
						}
					}

					if (All || Name.Find("rest").Found)
					{
						Want |= VMFeature_Rest;
						if (!(Features & VMFeature_Rest) && Features != VMFeature_All)
						{
							THAWK_ERROR("feature \"rest\" is not allowed");
							return false;
						}
					}

					Manager->Setup(All ? VMFeature_All : Want);
				}

				return true;
			});

			if (Manager != nullptr)
			{
				Context = Manager->CreateContext();
				Context->SetUserData(this, CompilerUD);
				Manager->SetProcessorOptions(Processor);
			}
		}
		VMCompiler::~VMCompiler()
		{
			delete Context;
			delete Processor;
		}
		void VMCompiler::SetIncludeCallback(const Compute::ProcIncludeCallback& Callback)
		{
			Include = Callback;
		}
		void VMCompiler::SetPragmaCallback(const Compute::ProcPragmaCallback& Callback)
		{
			Pragma = Callback;
		}
		void VMCompiler::SetAllowedFeatures(uint64_t NewFeatures)
		{
			Features = NewFeatures;
		}
		void VMCompiler::Define(const std::string& Word)
		{
			Processor->Define(Word);
		}
		void VMCompiler::Undefine(const std::string& Word)
		{
			Processor->Undefine(Word);
		}
		void VMCompiler::Clear()
		{
			if (Module != nullptr)
				Module->Discard();

			Processor->Clear();
			Module = nullptr;
			BuiltOK = false;
		}
		bool VMCompiler::IsDefined(const std::string& Word)
		{
			return Processor->IsDefined(Word.c_str());
		}
		bool VMCompiler::IsBuilt()
		{
			return BuiltOK;
		}
		int VMCompiler::Prepare(const std::string& ModuleName)
		{
			if (!Manager || ModuleName.empty())
				return -1;

			BuiltOK = false;
			VCache.Valid = false;
			VCache.Name.clear();

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			if (Module != nullptr)
				Module->Discard();

			Module = Engine->GetModule(ModuleName.c_str(), asGM_ALWAYS_CREATE);
			if (!Module)
				return -1;

			Module->SetUserData(this, CompilerUD);
			Manager->SetProcessorOptions(Processor);
			return 0;
		}
		int VMCompiler::Prepare(const std::string& ModuleName, const std::string& Name, bool Debug)
		{
			if (!Manager)
				return -1;

			int Result = Prepare(ModuleName);
			if (Result < 0)
				return Result;

			VCache.Name = Name;
			if (!Manager->GetByteCodeCache(&VCache))
			{
				VCache.Data.clear();
				VCache.Debug = Debug;
				VCache.Valid = false;
			}

			return Result;
		}
		int VMCompiler::PrepareScope(const std::string& ModuleName)
		{
			if (!Manager)
				return -1;

			return Prepare(Manager->GetScopedName(ModuleName));
		}
		int VMCompiler::PrepareScope(const std::string& ModuleName, const std::string& Name, bool Debug)
		{
			if (!Manager)
				return -1;

			return Prepare(Manager->GetScopedName(ModuleName), Name, Debug);
		}
		int VMCompiler::Compile(bool Await)
		{
			if (!Manager || !Module)
				return -1;

			if (VCache.Valid)
			{
				int R = LoadByteCode(&VCache);
				while (Await && R == asBUILD_IN_PROGRESS)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					R = LoadByteCode(&VCache);
				}

				BuiltOK = (R >= 0);
				return R;
			}

			int R = Module->Build();
			while (Await && R == asBUILD_IN_PROGRESS)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				R = Module->Build();
			}

			BuiltOK = (R >= 0);
			if (!BuiltOK)
				return R;

			Module->ResetGlobalVars(Context->GetContext());
			if (VCache.Name.empty())
				return R;

			R = SaveByteCode(&VCache);
			if (R < 0)
				return R;

			Manager->SetByteCodeCache(&VCache);
			return 0;
		}
		int VMCompiler::SaveByteCode(VMByteCode* Info)
		{
			if (!Info || !Module || !BuiltOK)
			{
				THAWK_ERROR("module was not built");
				return -1;
			}

			return VMWModule(Module).SaveByteCode(Info);
		}
		int VMCompiler::LoadByteCode(VMByteCode* Info)
		{
			if (!Info || !Module)
				return -1;

			return VMWModule(Module).LoadByteCode(Info);
		}
		int VMCompiler::LoadFile(const std::string& Path)
		{
			if (!Module || !Manager)
			{
				THAWK_ERROR("module was not created");
				return -1;
			}

			if (VCache.Valid)
				return 0;

			std::string Source = Rest::OS::Resolve(Path.c_str());
			if (!Rest::OS::FileExists(Source.c_str()))
			{
				THAWK_ERROR("file not found");
				return -1;
			}

			std::string Buffer = Rest::OS::Read(Source.c_str());
			if (!Processor->Process(Source, Buffer))
				return asINVALID_DECLARATION;

			return Module->AddScriptSection(Source.c_str(), Buffer.c_str(), Buffer.size());
		}
		int VMCompiler::LoadCode(const std::string& Name, const std::string& Data)
		{
			if (!Module)
			{
				THAWK_ERROR("module was not created");
				return -1;
			}

			std::string Buffer(Data);
			if (!Processor->Process("", Buffer))
				return asINVALID_DECLARATION;

			return Module->AddScriptSection(Name.c_str(), Buffer.c_str(), Buffer.size());
		}
		int VMCompiler::LoadCode(const std::string& Name, const char* Data, uint64_t Size)
		{
			if (!Module)
			{
				THAWK_ERROR("module was not created");
				return -1;
			}

			std::string Buffer(Data, Size);
			if (!Processor->Process("", Buffer))
				return asINVALID_DECLARATION;

			return Module->AddScriptSection(Name.c_str(), Buffer.c_str(), Buffer.size());
		}
		int VMCompiler::ExecuteFile(const char* Name, const char* ModuleName, const char* EntryName, void* Return, int ReturnTypeId)
		{
			if (!Name || !ModuleName || !EntryName)
				return VMResult_INVALID_ARG;

			int R = Prepare(ModuleName, Name);
			if (R < 0)
				return R;

			R = LoadFile(Name);
			if (R < 0)
				return R;

			R = Compile(true);
			if (R < 0)
				return R;

			return ExecuteEntry(EntryName, Return, ReturnTypeId);
		}
		int VMCompiler::ExecuteMemory(const std::string& Buffer, const char* ModuleName, const char* EntryName, void* Return, int ReturnTypeId)
		{
			if (Buffer.empty() || !ModuleName || !EntryName)
				return VMResult_INVALID_ARG;

			int R = Prepare(ModuleName, "anonymous");
			if (R < 0)
				return R;

			R = LoadCode("anonymous", Buffer);
			if (R < 0)
				return R;

			R = Compile(true);
			if (R < 0)
				return R;

			return ExecuteEntry(EntryName, Return, ReturnTypeId);
		}
		int VMCompiler::ExecuteEntry(const char* Name, void* Return, int ReturnTypeId)
		{
			if (!BuiltOK || !Manager || !Context || !Name || !Module)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			VMCFunction* Function = Module->GetFunctionByName(Name);
			if (!Function)
				return -1;

			VMCContext* VM = Context->GetContext();
			int R = VM->Prepare(Function);
			if (R < 0)
				return R;

			R = VM->Execute();
			if (Return != 0 && ReturnTypeId != asTYPEID_VOID)
			{
				if (ReturnTypeId & asTYPEID_OBJHANDLE)
				{
					if (*reinterpret_cast<void**>(Return) == nullptr)
					{
						*reinterpret_cast<void**>(Return) = *reinterpret_cast<void**>(Context->GetAddressOfReturnValue());
						Engine->AddRefScriptObject(*reinterpret_cast<void**>(Return), Engine->GetTypeInfoById(ReturnTypeId));
					}
				}
				else if (ReturnTypeId & asTYPEID_MASK_OBJECT)
					Engine->AssignScriptObject(Return, Context->GetAddressOfReturnValue(), Engine->GetTypeInfoById(ReturnTypeId));
				else
					memcpy(Return, Context->GetAddressOfReturnValue(), Engine->GetSizeOfPrimitiveType(ReturnTypeId));
			}

			return R;
		}
		int VMCompiler::ExecuteScoped(const std::string& Code, void* Return, int ReturnTypeId)
		{
			return ExecuteScoped(Code.c_str(), Code.size(), Return, ReturnTypeId);
		}
		int VMCompiler::ExecuteScoped(const char* Buffer, uint64_t Length, void* Return, int ReturnTypeId)
		{
			if (!BuiltOK || !Manager || !Context || !Buffer || !Length || !Module)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			std::string Eval = " __vfbdy(){\n";
			Eval.append(Buffer, Length);
			Eval += "\n;}";
			Eval = Engine->GetTypeDeclaration(ReturnTypeId, true) + Eval;

			VMCTypeInfo* Type = nullptr;
			if (ReturnTypeId & asTYPEID_MASK_OBJECT)
			{
				Type = Engine->GetTypeInfoById(ReturnTypeId);
				if (Type)
					Type->AddRef();
			}

			VMCModule* Module = GetModule().GetModule();
			if (Type)
				Type->Release();

			VMCFunction* Function = nullptr;
			int R = Module->CompileFunction("__vfbdy", Eval.c_str(), -1, asCOMP_ADD_TO_MODULE, &Function);
			if (R < 0)
				return R;

			VMCContext* VM = Context->GetContext();
			R = VM->Prepare(Function);
			if (R < 0)
			{
				Function->Release();
				return R;
			}

			R = VM->Execute();
			if (Return != 0 && ReturnTypeId != asTYPEID_VOID)
			{
				if (ReturnTypeId & asTYPEID_OBJHANDLE)
				{
					if (*reinterpret_cast<void**>(Return) == 0)
					{
						*reinterpret_cast<void**>(Return) = *reinterpret_cast<void**>(Context->GetAddressOfReturnValue());
						Engine->AddRefScriptObject(*reinterpret_cast<void**>(Return), Engine->GetTypeInfoById(ReturnTypeId));
					}
				}
				else if (ReturnTypeId & asTYPEID_MASK_OBJECT)
					Engine->AssignScriptObject(Return, Context->GetAddressOfReturnValue(), Engine->GetTypeInfoById(ReturnTypeId));
				else
					memcpy(Return, Context->GetAddressOfReturnValue(), Engine->GetSizeOfPrimitiveType(ReturnTypeId));
			}

			Function->Release();
			return R;
		}
		VMManager* VMCompiler::GetManager() const
		{
			return Manager;
		}
		VMContext* VMCompiler::GetContext() const
		{
			return Context;
		}
		VMWModule VMCompiler::GetModule() const
		{
			return Module;
		}
		Compute::Preprocessor* VMCompiler::GetProcessor() const
		{
			return Processor;
		}
		VMCompiler* VMCompiler::Get(VMContext* Context)
		{
			if (!Context)
				return nullptr;

			return (VMCompiler*)Context->GetUserData(CompilerUD);
		}
		int VMCompiler::CompilerUD = 154;

		VMContext::VMContext(VMCContext* Base) : Manager(nullptr), Context(Base)
		{
			if (Context != nullptr)
			{
				Manager = VMManager::Get(Base->GetEngine());
				Context->SetUserData(this, ContextUD);
				Context->SetExceptionCallback(asFUNCTION(ExceptionLogger), this, asCALL_CDECL);
			}
		}
		VMContext::~VMContext()
		{
			if (Context != nullptr)
			{
				if (Manager != nullptr)
					Manager->GetEngine()->ReturnContext(Context);
				else
					Context->Release();
			}
		}
		int VMContext::SetExceptionCallback(void(* Callback)(VMCContext* Context, void* Object), void* Object)
		{
			if (!Context)
				return -1;

			return Context->SetExceptionCallback(asFUNCTION(Callback), Object, asCALL_CDECL);
		}
		int VMContext::AddRefVM() const
		{
			if (!Context)
				return -1;

			return Context->AddRef();
		}
		int VMContext::ReleaseVM()
		{
			if (!Context)
				return -1;

			int R = Context->Release();
			if (R <= 0)
				Context = nullptr;

			return R;
		}
		int VMContext::Prepare(const VMWFunction& Function)
		{
			if (!Context)
				return -1;

			return Context->Prepare(Function.GetFunction());
		}
		int VMContext::Unprepare()
		{
			if (!Context)
				return -1;

			return Context->Unprepare();
		}
		int VMContext::Execute()
		{
			if (!Context)
				return -1;

			return Context->Execute();
		}
		int VMContext::Resume()
		{
			if (!IsSuspended())
				return asCONTEXT_ACTIVE;

			return Execute();
		}
		int VMContext::Abort()
		{
			if (!Context)
				return -1;

			return Context->Abort();
		}
		int VMContext::Suspend()
		{
			if (!Context)
				return -1;

			return Context->Suspend();
		}
		VMExecState VMContext::GetState() const
		{
			if (!Context)
				return VMExecState_UNINITIALIZED;

			return (VMExecState)Context->GetState();
		}
		std::string VMContext::GetStackTrace() const
		{
			if (!Context)
				return "";

			std::stringstream Result;
			Result << "--- call stack ---\n";

			for (asUINT n = 0; n < Context->GetCallstackSize(); n++)
			{
				VMCFunction* Function = Context->GetFunction(n);
				if (Function != nullptr)
				{
					if (Function->GetFuncType() == asFUNC_SCRIPT)
						Result << "\t" << (Function->GetScriptSectionName() ? Function->GetScriptSectionName() : "") << " (" << Context->GetLineNumber(n) << "): " << Function->GetDeclaration() << "\n";
					else
						Result << "\t" << "{...native...}: " << Function->GetDeclaration() << "\n";
				}
				else
					Result << "\t{...engine...}\n";
			}

			uint64_t Id = VMCThread::GetIdInThread();
			if (Id > 0)
				Result << "\t{...thread-" << Id << "...}\n";
			else
				Result << "\t{...main-thread...}\n";

			std::string Out(Result.str());
			return Out.substr(0, Out.size() - 1);
		}
		int VMContext::PushState()
		{
			if (!Context)
				return -1;

			return Context->PushState();
		}
		int VMContext::PopState()
		{
			if (!Context)
				return -1;

			return Context->PopState();
		}
		bool VMContext::IsNested(unsigned int* NestCount) const
		{
			if (!Context)
				return false;

			return Context->IsNested(NestCount);
		}
		int VMContext::SetObject(void* Object)
		{
			if (!Context)
				return -1;

			return Context->SetObject(Object);
		}
		int VMContext::SetArg8(unsigned int Arg, unsigned char Value)
		{
			if (!Context)
				return -1;

			return Context->SetArgByte(Arg, Value);
		}
		int VMContext::SetArg16(unsigned int Arg, unsigned short Value)
		{
			if (!Context)
				return -1;

			return Context->SetArgWord(Arg, Value);
		}
		int VMContext::SetArg32(unsigned int Arg, int Value)
		{
			if (!Context)
				return -1;

			return Context->SetArgDWord(Arg, Value);
		}
		int VMContext::SetArg64(unsigned int Arg, int64_t Value)
		{
			if (!Context)
				return -1;

			return Context->SetArgQWord(Arg, Value);
		}
		int VMContext::SetArgFloat(unsigned int Arg, float Value)
		{
			if (!Context)
				return -1;

			return Context->SetArgFloat(Arg, Value);
		}
		int VMContext::SetArgDouble(unsigned int Arg, double Value)
		{
			if (!Context)
				return -1;

			return Context->SetArgDouble(Arg, Value);
		}
		int VMContext::SetArgAddress(unsigned int Arg, void* Address)
		{
			if (!Context)
				return -1;

			return Context->SetArgAddress(Arg, Address);
		}
		int VMContext::SetArgObject(unsigned int Arg, void* Object)
		{
			if (!Context)
				return -1;

			return Context->SetArgObject(Arg, Object);
		}
		int VMContext::SetArgAny(unsigned int Arg, void* Ptr, int TypeId)
		{
			if (!Context)
				return -1;

			return Context->SetArgVarType(Arg, Ptr, TypeId);
		}
		void* VMContext::GetAddressOfArg(unsigned int Arg)
		{
			if (!Context)
				return nullptr;

			return Context->GetAddressOfArg(Arg);
		}
		unsigned char VMContext::GetReturnByte()
		{
			if (!Context)
				return 0;

			return Context->GetReturnByte();
		}
		unsigned short VMContext::GetReturnWord()
		{
			if (!Context)
				return 0;

			return Context->GetReturnWord();
		}
		size_t VMContext::GetReturnDWord()
		{
			if (!Context)
				return 0;

			return Context->GetReturnDWord();
		}
		uint64_t VMContext::GetReturnQWord()
		{
			if (!Context)
				return 0;

			return Context->GetReturnQWord();
		}
		float VMContext::GetReturnFloat()
		{
			if (!Context)
				return 0.0f;

			return Context->GetReturnFloat();
		}
		double VMContext::GetReturnDouble()
		{
			if (!Context)
				return 0.0;

			return Context->GetReturnDouble();
		}
		void* VMContext::GetReturnAddress()
		{
			if (!Context)
				return nullptr;

			return Context->GetReturnAddress();
		}
		void* VMContext::GetReturnObjectAddress()
		{
			if (!Context)
				return nullptr;

			return Context->GetReturnObject();
		}
		void* VMContext::GetAddressOfReturnValue()
		{
			if (!Context)
				return nullptr;

			return Context->GetAddressOfReturnValue();
		}
		int VMContext::SetException(const char* Info, bool AllowCatch)
		{
			if (!Context)
				return -1;

			return Context->SetException(Info, AllowCatch);
		}
		int VMContext::GetExceptionLineNumber(int* Column, const char** SectionName)
		{
			if (!Context)
				return -1;

			return Context->GetExceptionLineNumber(Column, SectionName);
		}
		VMWFunction VMContext::GetExceptionFunction()
		{
			if (!Context)
				return nullptr;

			return Context->GetExceptionFunction();
		}
		const char* VMContext::GetExceptionString()
		{
			if (!Context)
				return nullptr;

			return Context->GetExceptionString();
		}
		bool VMContext::WillExceptionBeCaught()
		{
			if (!Context)
				return false;

			return Context->WillExceptionBeCaught();
		}
		void VMContext::ClearExceptionCallback()
		{
			if (!Context)
				return;

			return Context->ClearExceptionCallback();
		}
		int VMContext::SetLineCallback(void(* Callback)(VMCContext* Context, void* Object), void* Object)
		{
			if (!Context || !Callback)
				return -1;

			return Context->SetLineCallback(asFUNCTION(Callback), Object, asCALL_CDECL);
		}
		void VMContext::ClearLineCallback()
		{
			if (!Context)
				return;

			return Context->ClearLineCallback();
		}
		unsigned int VMContext::GetCallstackSize() const
		{
			if (!Context)
				return 0;

			return Context->GetCallstackSize();
		}
		VMWFunction VMContext::GetFunction(unsigned int StackLevel)
		{
			if (!Context)
				return nullptr;

			return Context->GetFunction(StackLevel);
		}
		int VMContext::GetLineNumber(unsigned int StackLevel, int* Column, const char** SectionName)
		{
			if (!Context)
				return -1;

			return Context->GetLineNumber(StackLevel, Column, SectionName);
		}
		int VMContext::GetPropertiesCount(unsigned int StackLevel)
		{
			if (!Context)
				return -1;

			return Context->GetVarCount(StackLevel);
		}
		const char* VMContext::GetPropertyName(unsigned int Index, unsigned int StackLevel)
		{
			if (!Context)
				return nullptr;

			return Context->GetVarName(Index, StackLevel);
		}
		const char* VMContext::GetPropertyDecl(unsigned int Index, unsigned int StackLevel, bool IncludeNamespace)
		{
			if (!Context)
				return nullptr;

			return Context->GetVarDeclaration(Index, StackLevel, IncludeNamespace);
		}
		int VMContext::GetPropertyTypeId(unsigned int Index, unsigned int StackLevel)
		{
			if (!Context)
				return -1;

			return Context->GetVarTypeId(Index, StackLevel);
		}
		void* VMContext::GetAddressOfProperty(unsigned int Index, unsigned int StackLevel)
		{
			if (!Context)
				return nullptr;

			return Context->GetAddressOfVar(Index, StackLevel);
		}
		bool VMContext::IsPropertyInScope(unsigned int Index, unsigned int StackLevel)
		{
			if (!Context)
				return false;

			return Context->IsVarInScope(Index, StackLevel);
		}
		int VMContext::GetThisTypeId(unsigned int StackLevel)
		{
			if (!Context)
				return -1;

			return Context->GetThisTypeId(StackLevel);
		}
		void* VMContext::GetThisPointer(unsigned int StackLevel)
		{
			if (!Context)
				return nullptr;

			return Context->GetThisPointer(StackLevel);
		}
		VMWFunction VMContext::GetSystemFunction()
		{
			if (!Context)
				return nullptr;

			return Context->GetSystemFunction();
		}
		bool VMContext::IsSuspended() const
		{
			if (!Context)
				return false;

			return Context->GetState() == asEXECUTION_SUSPENDED;
		}
		void* VMContext::SetUserData(void* Data, uint64_t Type)
		{
			if (!Context)
				return nullptr;

			return Context->SetUserData(Data, Type);
		}
		void* VMContext::GetUserData(uint64_t Type) const
		{
			if (!Context)
				return nullptr;

			return Context->GetUserData(Type);
		}
		VMCContext* VMContext::GetContext()
		{
			return Context;
		}
		VMManager* VMContext::GetManager()
		{
			return Manager;
		}
		VMContext* VMContext::Get(VMCContext* Context)
		{
			if (!Context)
				return nullptr;

			void* Manager = Context->GetUserData(ContextUD);
			return (VMContext*)Manager;
		}
		VMContext* VMContext::Get()
		{
			return Get(asGetActiveContext());
		}
		void VMContext::ExceptionLogger(VMCContext* Context, void*)
		{
			VMCFunction* Function = Context->GetExceptionFunction();
			VMContext* Api = VMContext::Get(Context);
			if (!Api)
				return;

			const char* Message = Context->GetExceptionString();
			if (!Message || Message[0] == '\0')
				return;

			if (Context->WillExceptionBeCaught())
				return THAWK_WARN("exception raised");

			const char* Decl = Function->GetDeclaration();
			const char* Mod = Function->GetModuleName();
			const char* Source = Function->GetScriptSectionName();
			int Line = Context->GetExceptionLineNumber();
			std::string Stack = Api ? Api->GetStackTrace() : "";

			THAWK_ERROR("uncaught exception raised"
						"\n\tdescription: %s"
						"\n\tfunction: %s"
						"\n\tmodule: %s"
						"\n\tsource: %s"
						"\n\tline: %i\n%.*s", Message ? Message : "undefined", Decl ? Decl : "undefined", Mod ? Mod : "undefined", Source ? Source : "undefined", Line, (int)Stack.size(), Stack.c_str());
		}
		int VMContext::ContextUD = 152;

		VMManager::VMManager() : Engine(asCreateScriptEngine()), Globals(this), Features(0), Cached(true), Scope(0), JIT(nullptr)
		{
			Include.Exts.push_back(".as");
			Include.Root = Rest::OS::GetDirectory();

			Engine->SetUserData(this, ManagerUD);
			Engine->SetContextCallbacks(RequestContext, ReturnContext, nullptr);
			Engine->SetMessageCallback(asFUNCTION(CompileLogger), this, asCALL_CDECL);
			Engine->SetEngineProperty(asEP_INIT_GLOBAL_VARS_AFTER_BUILD, false);
		}
		VMManager::~VMManager()
		{
			for (auto& Context : Contexts)
				Context->Release();

			if (Engine != nullptr)
				Engine->ShutDownAndRelease();
#ifdef HAS_AS_JIT
			delete JIT;
#endif
		}
		void VMManager::Setup(uint64_t NewFeatures)
		{
			if ((NewFeatures == VMFeature_All || NewFeatures & VMFeature_Any) && !(Features & VMFeature_Any))
			{
				Features |= VMFeature_Any;
				VM_RegisterAny(Engine);
			}

			if ((NewFeatures == VMFeature_All || NewFeatures & VMFeature_Reference) && !(Features & VMFeature_Reference))
			{
				Features |= VMFeature_Reference;
				VM_RegisterHandle(Engine);
			}

			if ((NewFeatures == VMFeature_All || NewFeatures & VMFeature_WeakReference) && !(Features & VMFeature_WeakReference))
			{
				Features |= VMFeature_WeakReference;
				VM_RegisterWeakRef(Engine);
			}

			if ((NewFeatures == VMFeature_All || NewFeatures & VMFeature_Array) && !(Features & VMFeature_Array))
			{
				Features |= VMFeature_Array;
				VM_RegisterArray(Engine, true);
			}

			if ((NewFeatures == VMFeature_All || NewFeatures & VMFeature_String) && !(Features & VMFeature_String))
			{
				Features |= VMFeature_String;
				VM_RegisterString(Engine);
			}

			if ((NewFeatures == VMFeature_All || NewFeatures & VMFeature_Dictionary) && !(Features & VMFeature_Dictionary))
			{
				Features |= VMFeature_Dictionary;
				VM_RegisterDictionary(Engine);
			}

			if ((NewFeatures == VMFeature_All || NewFeatures & VMFeature_Grid) && !(Features & VMFeature_Grid))
			{
				Features |= VMFeature_Grid;
				VM_RegisterGrid(Engine);
			}

			if ((NewFeatures == VMFeature_All || NewFeatures & VMFeature_Math) && !(Features & VMFeature_Math))
			{
				Features |= VMFeature_Math;
				VM_RegisterMath(Engine);
			}

			if ((NewFeatures == VMFeature_All || NewFeatures & VMFeature_Complex) && !(Features & VMFeature_Complex))
			{
				Features |= VMFeature_Complex;
				VM_RegisterComplex(Engine);
			}

			if ((NewFeatures == VMFeature_All || NewFeatures & VMFeature_DateTime) && !(Features & VMFeature_DateTime))
			{
				Features |= VMFeature_DateTime;
				VM_RegisterDateTime(Engine);
			}

			if ((NewFeatures == VMFeature_All || NewFeatures & VMFeature_Exception) && !(Features & VMFeature_Exception))
			{
				Features |= VMFeature_Exception;
				VM_RegisterExceptionRoutines(Engine);
			}

			if ((NewFeatures == VMFeature_All || NewFeatures & VMFeature_Random) && !(Features & VMFeature_Random))
			{
				Features |= VMFeature_Random;
				Engine->RegisterObjectType("random", 0, asOBJ_REF);
				Engine->RegisterObjectBehaviour("random", asBEHAVE_FACTORY, "random@ f()", asFUNCTION(VMCRandom::Create), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("random", asBEHAVE_ADDREF, "void f()", asMETHOD(VMCRandom, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("random", asBEHAVE_RELEASE, "void f()", asMETHOD(VMCRandom, Release), asCALL_THISCALL);
				Engine->RegisterObjectMethod("random", "void opAssign(const random&)", asMETHODPR(VMCRandom, Assign, (VMCRandom *), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("random", "void seed(uint)", asMETHODPR(VMCRandom, Seed, (uint32_t), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("random", "void seed(uint[]&)", asMETHODPR(VMCRandom, Seed, (VMCArray *), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("random", "int getI()", asMETHOD(VMCRandom, GetI), asCALL_THISCALL);
				Engine->RegisterObjectMethod("random", "uint getU()", asMETHOD(VMCRandom, GetU), asCALL_THISCALL);
				Engine->RegisterObjectMethod("random", "double getD()", asMETHOD(VMCRandom, GetD), asCALL_THISCALL);
				Engine->RegisterObjectMethod("random", "void seedFromTime()", asMETHOD(VMCRandom, SeedFromTime), asCALL_THISCALL);
			}

			if ((NewFeatures == VMFeature_All || NewFeatures & VMFeature_Thread) && !(Features & VMFeature_Thread))
			{
				Features |= VMFeature_Thread;
				Engine->RegisterFuncdef("void thread_callback()");
				Engine->RegisterObjectType("thread", 0, asOBJ_REF | asOBJ_GC);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_ADDREF, "void f()", asMETHOD(VMCThread, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_RELEASE, "void f()", asMETHOD(VMCThread, Release), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(VMCThread, SetGCFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(VMCThread, GetGCFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(VMCThread, GetRefCount), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(VMCThread, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(VMCThread, ReleaseReferences), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "bool start()", asMETHOD(VMCThread, Start), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "bool isActive()", asMETHOD(VMCThread, IsActive), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "void suspend()", asMETHOD(VMCThread, Suspend), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "void send(any &in)", asMETHOD(VMCThread, Send), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "any@ receiveWait()", asMETHOD(VMCThread, ReceiveWait), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "any@ receive(uint64)", asMETHOD(VMCThread, Receive), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "int wait(uint64)", asMETHOD(VMCThread, Wait), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "int join()", asMETHOD(VMCThread, Join), asCALL_THISCALL);
				Engine->RegisterGlobalFunction("any@ receiveFrom(uint64 timeout)", asFUNCTION(VMCThread::ReceiveInThread), asCALL_CDECL);
				Engine->RegisterGlobalFunction("any@ receiveFromWait()", asFUNCTION(VMCThread::ReceiveWaitInThread), asCALL_CDECL);
				Engine->RegisterGlobalFunction("void sleep(uint64 timeout)", asFUNCTION(VMCThread::SleepInThread), asCALL_CDECL);
				Engine->RegisterGlobalFunction("void sendTo(any&)", asFUNCTION(VMCThread::SendInThread), asCALL_CDECL);
				Engine->RegisterGlobalFunction("thread@ create_thread(thread_callback @func)", asFUNCTION(VMCThread::StartThread), asCALL_CDECL);
				Engine->RegisterGlobalFunction("uint64 id()", asFUNCTION(VMCThread::GetIdInThread), asCALL_CDECL);
			}

			if ((NewFeatures == VMFeature_All || NewFeatures & VMFeature_Async) && !(Features & VMFeature_Async))
			{
				Features |= VMFeature_Async;
				Engine->RegisterObjectType("async", 0, asOBJ_REF | asOBJ_GC);
				Engine->RegisterObjectBehaviour("async", asBEHAVE_ADDREF, "void f()", asMETHOD(VMCAsync, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("async", asBEHAVE_RELEASE, "void f()", asMETHOD(VMCAsync, Release), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("async", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(VMCAsync, SetGCFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("async", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(VMCAsync, GetGCFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("async", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(VMCAsync, GetRefCount), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("async", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(VMCAsync, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("async", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(VMCAsync, ReleaseReferences), asCALL_THISCALL);
				Engine->RegisterObjectMethod("async", "any@ get()", asMETHOD(VMCAsync, Get), asCALL_THISCALL);
				Engine->RegisterObjectMethod("async", "bool get(?&out)", asMETHODPR(VMCAsync, GetAny, (void*, int) const, bool), asCALL_THISCALL);
				Engine->RegisterObjectMethod("async", "async@ await()", asMETHOD(VMCAsync, Await), asCALL_THISCALL);
			}

			if ((NewFeatures == VMFeature_All || NewFeatures & VMFeature_Rest) && !(Features & VMFeature_Rest))
			{
				if (!(NewFeatures & VMFeature_Any) && NewFeatures != VMFeature_All)
					return Setup(NewFeatures | VMFeature_Any);

				if (!(NewFeatures & VMFeature_Array) && NewFeatures != VMFeature_All)
					return Setup(NewFeatures | VMFeature_Array);

				if (!(NewFeatures & VMFeature_String) && NewFeatures != VMFeature_All)
					return Setup(NewFeatures | VMFeature_String);

				if (!(NewFeatures & VMFeature_Dictionary) && NewFeatures != VMFeature_All)
					return Setup(NewFeatures | VMFeature_Dictionary);

				Features |= VMFeature_Rest;
				VM_RegisterRest(this);
			}
		}
		void VMManager::SetupJIT(unsigned int JITOpts)
		{
#ifdef HAS_AS_JIT
			if (!JIT)
				Engine->SetEngineProperty(asEP_INCLUDE_JIT_INSTRUCTIONS, 1);
			else
				delete JIT;

			JIT = new VMCJITCompiler(JITOpts);
			Engine->SetJITCompiler(JIT);
#else
			THAWK_ERROR("JIT compiler is not supported on this platform");
#endif
		}
		void VMManager::SetCache(bool Enabled)
		{
			Cached = Enabled;
		}
		void VMManager::ClearCache()
		{
			Safe.lock();
			Opcodes.clear();
			Datas.clear();
			Files.clear();
			Safe.unlock();
		}
		void VMManager::Lock()
		{
			Safe.lock();
		}
		void VMManager::Unlock()
		{
			Safe.unlock();
		}
		void VMManager::SetCompilerIncludeOptions(const Compute::IncludeDesc& NewDesc)
		{
			Safe.lock();
			Include = NewDesc;
			Safe.unlock();
		}
		void VMManager::SetCompilerFeatures(const Compute::Preprocessor::Desc& NewDesc)
		{
			Safe.lock();
			Proc = NewDesc;
			Safe.unlock();
		}
		void VMManager::SetProcessorOptions(Compute::Preprocessor* Processor)
		{
			if (!Processor)
				return;

			Safe.lock();
			Processor->SetIncludeOptions(Include);
			Processor->SetFeatures(Proc);
			Safe.unlock();
		}
		bool VMManager::GetByteCodeCache(VMByteCode* Info)
		{
			if (!Info || !Cached)
				return false;

			Safe.lock();
			auto It = Opcodes.find(Info->Name);
			if (It == Opcodes.end())
			{
				Safe.unlock();
				return false;
			}

			Info->Data = It->second.Data;
			Info->Debug = It->second.Debug;
			Info->Valid = true;

			Safe.unlock();
			return true;
		}
		void VMManager::SetByteCodeCache(VMByteCode* Info)
		{
			if (!Info)
				return;

			Info->Valid = true;
			if (!Cached)
				return;

			Safe.lock();
			Opcodes[Info->Name] = *Info;
			Safe.unlock();
		}
		int VMManager::SetLogCallback(void(* Callback)(const asSMessageInfo* Message, void* Object), void* Object)
		{
			if (!Callback)
				return Engine->ClearMessageCallback();

			return Engine->SetMessageCallback(asFUNCTION(Callback), Object, asCALL_CDECL);
		}
		int VMManager::Log(const char* Section, int Row, int Column, VMLogType Type, const char* Message)
		{
			return Engine->WriteMessage(Section, Row, Column, (asEMsgType)Type, Message);
		}
		VMContext* VMManager::CreateContext()
		{
			return new VMContext(Engine->RequestContext());
		}
		VMCompiler* VMManager::CreateCompiler()
		{
			return new VMCompiler(this);
		}
		void* VMManager::CreateObject(const VMWTypeInfo& Type)
		{
			return Engine->CreateScriptObject(Type.GetTypeInfo());
		}
		void* VMManager::CreateObjectCopy(void* Object, const VMWTypeInfo& Type)
		{
			return Engine->CreateScriptObjectCopy(Object, Type.GetTypeInfo());
		}
		void* VMManager::CreateEmptyObject(const VMWTypeInfo& Type)
		{
			return Engine->CreateUninitializedScriptObject(Type.GetTypeInfo());
		}
		VMWFunction VMManager::CreateDelegate(const VMWFunction& Function, void* Object)
		{
			return Engine->CreateDelegate(Function.GetFunction(), Object);
		}
		int VMManager::AssignObject(void* DestObject, void* SrcObject, const VMWTypeInfo& Type)
		{
			return Engine->AssignScriptObject(DestObject, SrcObject, Type.GetTypeInfo());
		}
		void VMManager::ReleaseObject(void* Object, const VMWTypeInfo& Type)
		{
			return Engine->ReleaseScriptObject(Object, Type.GetTypeInfo());
		}
		void VMManager::AddRefObject(void* Object, const VMWTypeInfo& Type)
		{
			return Engine->AddRefScriptObject(Object, Type.GetTypeInfo());
		}
		int VMManager::RefCastObject(void* Object, const VMWTypeInfo& FromType, const VMWTypeInfo& ToType, void** NewPtr, bool UseOnlyImplicitCast)
		{
			return Engine->RefCastObject(Object, FromType.GetTypeInfo(), ToType.GetTypeInfo(), NewPtr, UseOnlyImplicitCast);
		}
		VMWLockableSharedBool VMManager::GetWeakRefFlagOfObject(void* Object, const VMWTypeInfo& Type) const
		{
			return Engine->GetWeakRefFlagOfScriptObject(Object, Type.GetTypeInfo());
		}
		int VMManager::Collect(size_t NumIterations)
		{
			return Engine->GarbageCollect(asGC_FULL_CYCLE, NumIterations);
		}
		void VMManager::GetStatistics(unsigned int* CurrentSize, unsigned int* TotalDestroyed, unsigned int* TotalDetected, unsigned int* NewObjects, unsigned int* TotalNewDestroyed) const
		{
            unsigned int asCurrentSize, asTotalDestroyed, asTotalDetected, asNewObjects, asTotalNewDestroyed;
			Engine->GetGCStatistics(&asCurrentSize, &asTotalDestroyed, &asTotalDetected, &asNewObjects, &asTotalNewDestroyed);

			if (CurrentSize != nullptr)
				*CurrentSize = (size_t)asCurrentSize;

			if (TotalDestroyed != nullptr)
				*TotalDestroyed = (size_t)asTotalDestroyed;

			if (TotalDetected != nullptr)
				*TotalDetected = (size_t)asTotalDetected;

			if (NewObjects != nullptr)
				*NewObjects = (size_t)asNewObjects;

			if (TotalNewDestroyed != nullptr)
				*TotalNewDestroyed = (size_t)asTotalNewDestroyed;
		}
		int VMManager::NotifyOfNewObject(void* Object, const VMWTypeInfo& Type)
		{
			return Engine->NotifyGarbageCollectorOfNewObject(Object, Type.GetTypeInfo());
		}
		int VMManager::GetObjectAddress(size_t Index, size_t* SequenceNumber, void** Object, VMWTypeInfo* Type)
		{
			asUINT asSequenceNumber;
			VMCTypeInfo* OutType = nullptr;
			int Result = Engine->GetObjectInGC(Index, &asSequenceNumber, Object, &OutType);

			if (SequenceNumber != nullptr)
				*SequenceNumber = (size_t)asSequenceNumber;

			if (Type != nullptr)
				*Type = VMWTypeInfo(OutType);

			return Result;
		}
		void VMManager::ForwardEnumReferences(void* Reference, const VMWTypeInfo& Type)
		{
			return Engine->ForwardGCEnumReferences(Reference, Type.GetTypeInfo());
		}
		void VMManager::ForwardReleaseReferences(void* Reference, const VMWTypeInfo& Type)
		{
			return Engine->ForwardGCReleaseReferences(Reference, Type.GetTypeInfo());
		}
		void VMManager::GCEnumCallback(void* Reference)
		{
			Engine->GCEnumCallback(Reference);
		}
		int VMManager::GetTypeNameScope(const char** TypeName, const char** Namespace, size_t* NamespaceSize) const
		{
			if (!TypeName || !*TypeName)
				return -1;

			const char* Value = *TypeName;
			size_t Size = strlen(Value);
			size_t Index = Size - 1;

			while (Index > 0 && Value[Index] != ':' && Value[Index - 1] != ':')
				Index--;

			if (Index < 1)
			{
				if (Namespace != nullptr)
					*Namespace = "";

				if (NamespaceSize != nullptr)
					*NamespaceSize = 0;

				return 1;
			}

			if (Namespace != nullptr)
				*Namespace = Value;

			if (NamespaceSize != nullptr)
				*NamespaceSize = Index - 1;

			*TypeName = Value + Index + 1;
			return 0;
		}
		int VMManager::BeginGroup(const char* GroupName)
		{
			if (!GroupName)
				return -1;

			return Engine->BeginConfigGroup(GroupName);
		}
		int VMManager::EndGroup()
		{
			return Engine->EndConfigGroup();
		}
		int VMManager::RemoveGroup(const char* GroupName)
		{
			if (!GroupName)
				return -1;

			return Engine->RemoveConfigGroup(GroupName);
		}
		int VMManager::BeginNamespace(const char* Namespace)
		{
			if (!Namespace)
				return -1;

			return Engine->SetDefaultNamespace(Namespace);
		}
		int VMManager::BeginNamespaceIsolated(const char* Namespace, size_t DefaultMask)
		{
			if (!Namespace)
				return -1;

			BeginAccessMask(DefaultMask);
			return Engine->SetDefaultNamespace(Namespace);
		}
		int VMManager::EndNamespaceIsolated()
		{
			EndAccessMask();
			return Engine->SetDefaultNamespace("");
		}
		int VMManager::EndNamespace()
		{
			return Engine->SetDefaultNamespace("");
		}
		int VMManager::Namespace(const char* Namespace, const std::function<int(VMGlobal*)>& Callback)
		{
			if (!Namespace || !Callback)
				return -1;

			int R = BeginNamespace(Namespace);
			if (R < 0)
				return R;

			R = Callback(&Globals);
			if (R < 0)
				return R;

			return EndNamespace();
		}
		int VMManager::NamespaceIsolated(const char* Namespace, size_t DefaultMask, const std::function<int(VMGlobal*)>& Callback)
		{
			if (!Namespace || !Callback)
				return -1;

			int R = BeginNamespaceIsolated(Namespace, DefaultMask);
			if (R < 0)
				return R;

			R = Callback(&Globals);
			if (R < 0)
				return R;

			return EndNamespaceIsolated();
		}
		size_t VMManager::BeginAccessMask(size_t DefaultMask)
		{
			return Engine->SetDefaultAccessMask(DefaultMask);
		}
		size_t VMManager::EndAccessMask()
		{
			return Engine->SetDefaultAccessMask(VMManager::GetDefaultAccessMask());
		}
		const char* VMManager::GetNamespace() const
		{
			return Engine->GetDefaultNamespace();
		}
		VMGlobal& VMManager::Global()
		{
			return Globals;
		}
		VMWModule VMManager::Module(const char* Name)
		{
			if (!Engine || !Name)
				return VMWModule(nullptr);

			return VMWModule(Engine->GetModule(Name, asGM_CREATE_IF_NOT_EXISTS));
		}
		int VMManager::AddRefVM() const
		{
			if (!Engine)
				return -1;

			return Engine->AddRef();
		}
		int VMManager::ReleaseVM()
		{
			if (!Engine)
				return -1;

			int R = Engine->Release();
			if (R <= 0)
				Engine = nullptr;

			return R;
		}
		int VMManager::SetProperty(VMProp Property, size_t Value)
		{
			if (!Engine)
				return -1;

			return Engine->SetEngineProperty((asEEngineProp)Property, (asPWORD)Value);
		}
		void VMManager::SetDocumentRoot(const std::string& Value)
		{
			Safe.lock();
			Include.Root = Value;
			if (Include.Root.empty())
				return Safe.unlock();

			if (!Rest::Stroke(&Include.Root).EndsOf("/\\"))
			{
#ifdef THAWK_MICROSOFT
				Include.Root.append(1, '\\');
#else
				Include.Root.append(1, '/');
#endif
			}
			Safe.unlock();
		}
		std::string VMManager::GetScopedName(const std::string& Name)
		{
			if (!Engine || Name.empty())
				return Name;

			if (!Engine->GetModule(Name.c_str()))
				return Name;

			while (true)
			{
				Safe.lock();
				std::string Result = Name + std::to_string(Scope++);
				Safe.unlock();

				if (!Engine->GetModule(Result.c_str()))
					return Result;
			}

			return nullptr;
		}
		std::string VMManager::GetDocumentRoot() const
		{
			return Include.Root;
		}
		bool VMManager::ImportFile(const std::string& Path, std::string* Out)
		{
			if (!Rest::OS::FileExists(Path.c_str()))
				return false;

			if (!Cached)
			{
				if (Out != nullptr)
					Out->assign(Rest::OS::Read(Path.c_str()));

				return true;
			}

			Safe.lock();
			auto It = Files.find(Path);
			if (It != Files.end())
			{
				if (Out != nullptr)
					Out->assign(It->second);

				Safe.unlock();
				return true;
			}

			std::string& Result = Files[Path];
			Result = Rest::OS::Read(Path.c_str());
			if (Out != nullptr)
				Out->assign(Result);

			Safe.unlock();
			return true;
		}
		Rest::Document* VMManager::ImportJSON(const std::string& Path)
		{
			std::string File = Rest::OS::Resolve(Path, Include.Root);
			if (!Rest::OS::FileExists(File.c_str()))
			{
				File = Rest::OS::Resolve(Path + ".json", Include.Root);
				if (!Rest::OS::FileExists(File.c_str()))
					return nullptr;
			}

			if (!Cached)
			{
				std::string Data = Rest::OS::Read(Path.c_str());
				uint64_t Offset = 0;

				return Rest::Document::ReadJSON(Data.size(), [&Data, &Offset](char* Buffer, int64_t Size)
				{
					if (!Buffer || !Size)
						return true;

					if (Offset + Size > Data.size())
						return false;

					memcpy(Buffer, Data.c_str() + Offset, Size);
					Offset += Size;

					return true;
				});
			}

			Safe.lock();
			auto It = Datas.find(File);
			if (It != Datas.end())
			{
				Rest::Document* Result = It->second ? It->second->Copy() : nullptr;
				Safe.unlock();

				return Result;
			}

			Rest::Document*& Result = Datas[File];
			std::string Data = Rest::OS::Read(File.c_str());
			uint64_t Offset = 0;

			Result = Rest::Document::ReadJSON(Data.size(), [&Data, &Offset](char* Buffer, int64_t Size)
			{
				if (!Buffer || !Size)
					return true;

				if (Offset + Size > Data.size())
					return false;

				memcpy(Buffer, Data.c_str() + Offset, Size);
				Offset += Size;

				return true;
			});

			Rest::Document* Copy = nullptr;
			if (Result != nullptr)
				Copy = Result->Copy();

			Safe.unlock();
			return Copy;
		}
		size_t VMManager::GetProperty(VMProp Property) const
		{
			if (!Engine)
				return 0;

			return (size_t)Engine->GetEngineProperty((asEEngineProp)Property);
		}
		VMCManager* VMManager::GetEngine() const
		{
			return Engine;
		}
		void VMManager::FreeProxy()
		{
			VM_FreeStringProxy();
		}
		VMManager* VMManager::Get(VMCManager* Engine)
		{
			if (!Engine)
				return nullptr;

			void* Manager = Engine->GetUserData(ManagerUD);
			return (VMManager*)Manager;
		}
		VMManager* VMManager::Get()
		{
			VMCContext* Context = asGetActiveContext();
			if (!Context)
				return nullptr;

			return Get(Context->GetEngine());
		}
		VMCContext* VMManager::RequestContext(VMCManager* Engine, void* Data)
		{
			VMManager* Manager = VMManager::Get(Engine);
			if (!Manager)
				return Engine->CreateContext();

			Manager->Safe.lock();
			if (Manager->Contexts.empty())
			{
				Manager->Safe.unlock();
				return Engine->CreateContext();
			}

			VMCContext* Context = *Manager->Contexts.rbegin();
			Manager->Contexts.pop_back();
			Manager->Safe.unlock();

			return Context;
		}
		void VMManager::ReturnContext(VMCManager* Engine, VMCContext* Context, void* Data)
		{
			VMManager* Manager = VMManager::Get(Engine);
			if (!Manager)
				return (void)Context->Release();

			Manager->Safe.lock();
			Manager->Contexts.push_back(Context);
			Context->Unprepare();
			Manager->Safe.unlock();
		}
		void VMManager::CompileLogger(asSMessageInfo* Info, void*)
		{
			if (Info->type == asMSGTYPE_WARNING)
				THAWK_WARN("\n\t%s (%i, %i): %s", Info->section && Info->section[0] != '\0' ? Info->section : "any", Info->row, Info->col, Info->message);
			else if (Info->type == asMSGTYPE_INFORMATION)
				THAWK_INFO("\n\t%s (%i, %i): %s", Info->section && Info->section[0] != '\0' ? Info->section : "any", Info->row, Info->col, Info->message);
			else if (Info->type == asMSGTYPE_ERROR)
				THAWK_ERROR("\n\t%s (%i, %i): %s", Info->section && Info->section[0] != '\0' ? Info->section : "any", Info->row, Info->col, Info->message);
		}
		size_t VMManager::GetDefaultAccessMask()
		{
			return 0xFFFFFFFF;
		}
		int VMManager::ManagerUD = 553;

		VMDebugger::VMDebugger() : Action(CONTINUE), LastFunction(nullptr), Manager(nullptr)
		{
			LastCommandAtStackLevel = 0;
		}
		VMDebugger::~VMDebugger()
		{
			SetEngine(0);
		}
		void VMDebugger::RegisterToStringCallback(const VMWTypeInfo& Type, ToStringCallback Callback)
		{
			if (ToStringCallbacks.find(Type.GetTypeInfo()) == ToStringCallbacks.end())
				ToStringCallbacks.insert(std::unordered_map<const VMCTypeInfo*, ToStringCallback>::value_type(Type.GetTypeInfo(), Callback));
		}
		void VMDebugger::LineCallback(VMContext* Context)
		{
			if (!Context)
				return;

			VMCContext* Base = Context->GetContext();
			if (!Base || Base->GetState() != asEXECUTION_ACTIVE)
				return;

			if (Action == CONTINUE)
			{
				if (!CheckBreakPoint(Context))
					return;
			}
			else if (Action == STEP_OVER)
			{
				if (Base->GetCallstackSize() > LastCommandAtStackLevel)
				{
					if (!CheckBreakPoint(Context))
						return;
				}
			}
			else if (Action == STEP_OUT)
			{
				if (Base->GetCallstackSize() >= LastCommandAtStackLevel)
				{
					if (!CheckBreakPoint(Context))
						return;
				}
			}
			else if (Action == STEP_INTO)
				CheckBreakPoint(Context);

			std::stringstream Stream;
			const char* File = nullptr;
			int Number = Base->GetLineNumber(0, 0, &File);

			Stream << (File ? File : "{unnamed}") << ":" << Number << "; " << Base->GetFunction()->GetDeclaration() << std::endl;
			Output(Stream.str());
			TakeCommands(Context);
		}
		void VMDebugger::TakeCommands(VMContext* Context)
		{
			if (!Context)
				return;

			VMCContext* Base = Context->GetContext();
			if (!Base)
				return;

			for (;;)
			{
				char Buffer[512];
				Output("[dbg]> ");
				std::cin.getline(Buffer, 512);

				if (InterpretCommand(std::string(Buffer), Context))
					break;
			}
		}
		void VMDebugger::PrintValue(const std::string& Expression, VMContext* Context)
		{
			if (!Context)
				return;

			VMCContext* Base = Context->GetContext();
			if (!Base)
			{
				Output("No script is running\n");
				return;
			}

			VMCManager* Engine = Context->GetManager()->GetEngine();
			std::string Text = Expression, Scope, Name;
			asUINT Length = 0;

			asETokenClass T = Engine->ParseToken(Text.c_str(), 0, &Length);
			while (T == asTC_IDENTIFIER || (T == asTC_KEYWORD && Length == 2 && Text.compare("::")))
			{
				if (T == asTC_KEYWORD)
				{
					if (Scope == "" && Name == "")
						Scope = "::";
					else if (Scope == "::" || Scope == "")
						Scope = Name;
					else
						Scope += "::" + Name;

					Name.clear();
				}
				else if (T == asTC_IDENTIFIER)
					Name.assign(Text.c_str(), Length);

				Text = Text.substr(Length);
				T = Engine->ParseToken(Text.c_str(), 0, &Length);
			}

			if (Name.size())
			{
				void* Pointer = 0;
				int TypeId = 0;

				VMCFunction* Function = Base->GetFunction();
				if (!Function)
					return;

				if (Scope == "")
				{
					for (asUINT n = Function->GetVarCount(); n-- > 0;)
					{
						if (Base->IsVarInScope(n) && Name == Base->GetVarName(n))
						{
							Pointer = Base->GetAddressOfVar(n);
							TypeId = Base->GetVarTypeId(n);
							break;
						}
					}

					if (!Pointer && Function->GetObjectType())
					{
						if (Name == "this")
						{
							Pointer = Base->GetThisPointer();
							TypeId = Base->GetThisTypeId();
						}
						else
						{
							VMCTypeInfo* Type = Engine->GetTypeInfoById(Base->GetThisTypeId());
							for (asUINT n = 0; n < Type->GetPropertyCount(); n++)
							{
								const char* PropName = 0;
								int Offset = 0;
								bool IsReference = 0;
								int CompositeOffset = 0;
								bool IsCompositeIndirect = false;

								Type->GetProperty(n, &PropName, &TypeId, 0, 0, &Offset, &IsReference, 0, &CompositeOffset, &IsCompositeIndirect);
								if (Name == PropName)
								{
									Pointer = (void*)(((asBYTE*)Base->GetThisPointer()) + CompositeOffset);
									if (IsCompositeIndirect)
										Pointer = *(void**)Pointer;

									Pointer = (void*)(((asBYTE*)Pointer) + Offset);
									if (IsReference)
										Pointer = *(void**)Pointer;

									break;
								}
							}
						}
					}
				}

				if (!Pointer)
				{
					if (Scope.empty())
						Scope = Function->GetNamespace();
					else if (Scope == "::")
						Scope = "";

					VMCModule* Mod = Function->GetModule();
					if (Mod != nullptr)
					{
						for (asUINT n = 0; n < Mod->GetGlobalVarCount(); n++)
						{
							const char* VarName = 0, * NameSpace = 0;
							Mod->GetGlobalVar(n, &VarName, &NameSpace, &TypeId);

							if (Name == VarName && Scope == NameSpace)
							{
								Pointer = Mod->GetAddressOfGlobalVar(n);
								break;
							}
						}
					}
				}

				if (Pointer)
				{
					std::stringstream Stream;
					Stream << ToString(Pointer, TypeId, 3, Manager) << std::endl;
					Output(Stream.str());
				}
				else
					Output("Invalid expression. No matching symbol\n");
			}
			else
				Output("Invalid expression. Expected identifier\n");
		}
		void VMDebugger::ListBreakPoints()
		{
			std::stringstream Stream;
			for (size_t b = 0; b < BreakPoints.size(); b++)
			{
				if (BreakPoints[b].Function)
					Stream << b << " - " << BreakPoints[b].Name << std::endl;
				else
					Stream << b << " - " << BreakPoints[b].Name << ":" << BreakPoints[b].Line << std::endl;
			}
			Output(Stream.str());
		}
		void VMDebugger::ListMemberProperties(VMContext* Context)
		{
			if (!Context)
				return;

			VMCContext* Base = Context->GetContext();
			if (!Base)
			{
				Output("No script is running\n");
				return;
			}

			void* Pointer = Base->GetThisPointer();
			if (Pointer != nullptr)
			{
				std::stringstream Stream;
				Stream << "this = " << ToString(Pointer, Base->GetThisTypeId(), 3, VMManager::Get(Base->GetEngine())) << std::endl;
				Output(Stream.str());
			}
		}
		void VMDebugger::ListLocalVariables(VMContext* Context)
		{
			if (!Context)
				return;

			VMCContext* Base = Context->GetContext();
			if (!Base)
			{
				Output("No script is running\n");
				return;
			}

			VMCFunction* Function = Base->GetFunction();
			if (!Function)
				return;

			std::stringstream Stream;
			for (asUINT n = 0; n < Function->GetVarCount(); n++)
			{
				if (Base->IsVarInScope(n))
					Stream << Function->GetVarDecl(n) << " = " << ToString(Base->GetAddressOfVar(n), Base->GetVarTypeId(n), 3, VMManager::Get(Base->GetEngine())) << std::endl;
			}
			Output(Stream.str());
		}
		void VMDebugger::ListGlobalVariables(VMContext* Context)
		{
			if (!Context)
				return;

			VMCContext* Base = Context->GetContext();
			if (!Base)
			{
				Output("No script is running\n");
				return;
			}

			VMCFunction* Function = Base->GetFunction();
			if (!Function)
				return;

			VMCModule* Mod = Function->GetModule();
			if (!Mod)
				return;

			std::stringstream Stream;
			for (asUINT n = 0; n < Mod->GetGlobalVarCount(); n++)
			{
				int TypeId = 0;
				Mod->GetGlobalVar(n, nullptr, nullptr, &TypeId);
				Stream << Mod->GetGlobalVarDeclaration(n) << " = " << ToString(Mod->GetAddressOfGlobalVar(n), TypeId, 3, VMManager::Get(Base->GetEngine())) << std::endl;
			}
			Output(Stream.str());
		}
		void VMDebugger::ListStatistics(VMContext* Context)
		{
			if (!Context)
				return;

			VMCContext* Base = Context->GetContext();
			if (!Base)
			{
				Output("No script is running\n");
				return;
			}

			VMCManager* Engine = Base->GetEngine();
			asUINT GCCurrSize, GCTotalDestr, GCTotalDet, GCNewObjects, GCTotalNewDestr;
			Engine->GetGCStatistics(&GCCurrSize, &GCTotalDestr, &GCTotalDet, &GCNewObjects, &GCTotalNewDestr);

			std::stringstream Stream;
			Stream << "Garbage collector:" << std::endl;
			Stream << " current size:          " << GCCurrSize << std::endl;
			Stream << " total destroyed:       " << GCTotalDestr << std::endl;
			Stream << " total detected:        " << GCTotalDet << std::endl;
			Stream << " new objects:           " << GCNewObjects << std::endl;
			Stream << " new objects destroyed: " << GCTotalNewDestr << std::endl;
			Output(Stream.str());
		}
		void VMDebugger::PrintCallstack(VMContext* Context)
		{
			if (!Context)
				return;

			VMCContext* Base = Context->GetContext();
			if (!Base)
			{
				Output("No script is running\n");
				return;
			}

			std::stringstream Stream;
			const char* File = nullptr;
			int LineNumber = 0;

			for (asUINT n = 0; n < Base->GetCallstackSize(); n++)
			{
				LineNumber = Base->GetLineNumber(n, 0, &File);
				Stream << (File ? File : "{unnamed}") << ":" << LineNumber << "; " << Base->GetFunction(n)->GetDeclaration() << std::endl;
			}

			Output(Stream.str());
		}
		void VMDebugger::AddFuncBreakPoint(const std::string& Function)
		{
			size_t B = Function.find_first_not_of(" \t");
			size_t E = Function.find_last_not_of(" \t");
			std::string Actual = Function.substr(B, E != std::string::npos ? E - B + 1 : std::string::npos);

			std::stringstream Stream;
			Stream << "Adding deferred break point for function '" << Actual << "'" << std::endl;
			Output(Stream.str());

			BreakPoint Point(Actual, 0, true);
			BreakPoints.push_back(Point);
		}
		void VMDebugger::AddFileBreakPoint(const std::string& File, int LineNumber)
		{
			size_t R = File.find_last_of("\\/");
			std::string Actual;

			if (R != std::string::npos)
				Actual = File.substr(R + 1);
			else
				Actual = File;

			size_t B = Actual.find_first_not_of(" \t");
			size_t E = Actual.find_last_not_of(" \t");
			Actual = Actual.substr(B, E != std::string::npos ? E - B + 1 : std::string::npos);

			std::stringstream Stream;
			Stream << "Setting break point in file '" << Actual << "' at line " << LineNumber << std::endl;
			Output(Stream.str());

			BreakPoint Point(Actual, LineNumber, false);
			BreakPoints.push_back(Point);
		}
		void VMDebugger::PrintHelp()
		{
			Output(" c - Continue\n"
				   " s - Step into\n"
				   " n - Next step\n"
				   " o - Step out\n"
				   " b - Set break point\n"
				   " l - List various things\n"
				   " r - Remove break point\n"
				   " p - Print value\n"
				   " w - Where am I?\n"
				   " a - Abort execution\n"
				   " h - Print this help text\n");
		}
		void VMDebugger::Output(const std::string& Data)
		{
			std::cout << Data;
		}
		void VMDebugger::SetEngine(VMManager* Engine)
		{
			if (Engine != nullptr && Engine != Manager)
			{
				if (Manager != nullptr)
					Manager->GetEngine()->Release();

				Manager = Engine;
				if (Manager != nullptr)
					Manager->GetEngine()->AddRef();
			}
		}
		bool VMDebugger::CheckBreakPoint(VMContext* Context)
		{
			if (!Context)
				return false;

			VMCContext* Base = Context->GetContext();
			if (!Base)
				return false;

			const char* Temp = 0;
			int Line = Base->GetLineNumber(0, 0, &Temp);

			std::string File = Temp ? Temp : "";
			size_t R = File.find_last_of("\\/");
			if (R != std::string::npos)
				File = File.substr(R + 1);

			VMCFunction* Function = Base->GetFunction();
			if (LastFunction != Function)
			{
				for (size_t n = 0; n < BreakPoints.size(); n++)
				{
					if (BreakPoints[n].Function)
					{
						if (BreakPoints[n].Name == Function->GetName())
						{
							std::stringstream Stream;
							Stream << "Entering function '" << BreakPoints[n].Name << "'. Transforming it into break point" << std::endl;
							Output(Stream.str());

							BreakPoints[n].Name = File;
							BreakPoints[n].Line = Line;
							BreakPoints[n].Function = false;
							BreakPoints[n].NeedsAdjusting = false;
						}
					}
					else if (BreakPoints[n].NeedsAdjusting && BreakPoints[n].Name == File)
					{
						int Number = Function->FindNextLineWithCode(BreakPoints[n].Line);
						if (Number >= 0)
						{
							BreakPoints[n].NeedsAdjusting = false;
							if (Number != BreakPoints[n].Line)
							{
								std::stringstream Stream;
								Stream << "Moving break point " << n << " in file '" << File << "' to next line with code at line " << Number << std::endl;
								Output(Stream.str());

								BreakPoints[n].Line = Number;
							}
						}
					}
				}
			}

			LastFunction = Function;
			for (size_t n = 0; n < BreakPoints.size(); n++)
			{
				if (!BreakPoints[n].Function && BreakPoints[n].Line == Line && BreakPoints[n].Name == File)
				{
					std::stringstream Stream;
					Stream << "Reached break point " << n << " in file '" << File << "' at line " << Line << std::endl;
					Output(Stream.str());
					return true;
				}
			}

			return false;
		}
		bool VMDebugger::InterpretCommand(const std::string& Command, VMContext* Context)
		{
			if (!Context)
				return false;

			VMCContext* Base = Context->GetContext();
			if (!Base)
				return false;

			if (Command.length() == 0)
				return true;

			switch (Command[0])
			{
				case 'c':
					Action = CONTINUE;
					break;
				case 's':
					Action = STEP_INTO;
					break;
				case 'n':
					Action = STEP_OVER;
					LastCommandAtStackLevel = Base->GetCallstackSize();
					break;
				case 'o':
					Action = STEP_OUT;
					LastCommandAtStackLevel = Base->GetCallstackSize();
					break;
				case 'b':
				{
					size_t Div = Command.find(':');
					if (Div != std::string::npos && Div > 2)
					{
						std::string File = Command.substr(2, Div - 2);
						std::string Line = Command.substr(Div + 1);
						int Number = atoi(Line.c_str());

						AddFileBreakPoint(File, Number);
					}
					else if (Div == std::string::npos && (Div = Command.find_first_not_of(" \t", 1)) != std::string::npos)
					{
						std::string Function = Command.substr(Div);
						AddFuncBreakPoint(Function);
					}
					else
					{
						Output("Incorrect format for setting break point, expected one of:\n"
							   " b <file name>:<line number>\n"
							   " b <function name>\n");
					}

					return false;
				}
				case 'r':
				{
					if (Command.length() > 2)
					{
						std::string BR = Command.substr(2);
						if (BR == "all")
						{
							BreakPoints.clear();
							Output("All break points have been removed\n");
						}
						else
						{
							int NBR = atoi(BR.c_str());
							if (NBR >= 0 && NBR < (int)BreakPoints.size())
								BreakPoints.erase(BreakPoints.begin() + NBR);

							ListBreakPoints();
						}
					}
					else
					{
						Output("Incorrect format for removing break points, expected:\n"
							   " r <all|number of break point>\n");
					}

					return false;
				}
				case 'l':
				{
					bool WantPrintHelp = false;
					size_t P = Command.find_first_not_of(" \t", 1);
					if (P != std::string::npos)
					{
						if (Command[P] == 'b')
							ListBreakPoints();
						else if (Command[P] == 'v')
							ListLocalVariables(Context);
						else if (Command[P] == 'g')
							ListGlobalVariables(Context);
						else if (Command[P] == 'm')
							ListMemberProperties(Context);
						else if (Command[P] == 's')
							ListStatistics(Context);
						else
						{
							Output("Unknown list option.\n");
							WantPrintHelp = true;
						}
					}
					else
					{
						Output("Incorrect format for list command.\n");
						WantPrintHelp = true;
					}

					if (WantPrintHelp)
					{
						Output("Expected format: \n"
							   " l <list option>\n"
							   "Available options: \n"
							   " b - breakpoints\n"
							   " v - local variables\n"
							   " m - member properties\n"
							   " g - global variables\n"
							   " s - statistics\n");
					}

					return false;
				}
				case 'h':
					PrintHelp();
					return false;
				case 'p':
				{
					size_t P = Command.find_first_not_of(" \t", 1);
					if (P == std::string::npos)
					{
						Output("Incorrect format for print, expected:\n"
							   " p <expression>\n");
					}
					else
						PrintValue(Command.substr(P), Context);

					return false;
				}
				case 'w':
					PrintCallstack(Context);
					return false;
				case 'a':
					if (Base == 0)
					{
						Output("No script is running\n");
						return false;
					}

					Base->Abort();
					break;
				default:
					Output("Unknown command\n");
					return false;
			}

			return true;
		}
		std::string VMDebugger::ToString(void* Value, unsigned int TypeId, int ExpandMembers, VMManager* Engine)
		{
			if (Value == 0)
				return "<null>";

			if (Engine == 0)
				Engine = Manager;

			if (!Manager || !Engine)
				return "<null>";

			VMCManager* Base = Engine->GetEngine();
			if (!Base)
				return "<null>";

			std::stringstream Stream;
			if (TypeId == asTYPEID_VOID)
				return "<void>";
			else if (TypeId == asTYPEID_BOOL)
				return *(bool*)Value ? "true" : "false";
			else if (TypeId == asTYPEID_INT8)
				Stream << (int)*(signed char*)Value;
			else if (TypeId == asTYPEID_INT16)
				Stream << (int)*(signed short*)Value;
			else if (TypeId == asTYPEID_INT32)
				Stream << *(signed int*)Value;
			else if (TypeId == asTYPEID_INT64)
#if defined(_MSC_VER) && _MSC_VER <= 1200
				Stream << "{...}";
#else
				Stream << *(asINT64*)Value;
#endif
			else if (TypeId == asTYPEID_UINT8)
				Stream << (unsigned int)*(unsigned char*)Value;
			else if (TypeId == asTYPEID_UINT16)
				Stream << (unsigned int)*(unsigned short*)Value;
			else if (TypeId == asTYPEID_UINT32)
				Stream << *(unsigned int*)Value;
			else if (TypeId == asTYPEID_UINT64)
#if defined(_MSC_VER) && _MSC_VER <= 1200
				Stream << "{...}";
#else
				Stream << *(asQWORD*)Value;
#endif
			else if (TypeId == asTYPEID_FLOAT)
				Stream << *(float*)Value;
			else if (TypeId == asTYPEID_DOUBLE)
				Stream << *(double*)Value;
			else if ((TypeId & asTYPEID_MASK_OBJECT) == 0)
			{
				Stream << *(asUINT*)Value;
				if (Base != nullptr)
				{
					VMCTypeInfo* T = Base->GetTypeInfoById(TypeId);
					for (int n = T->GetEnumValueCount(); n-- > 0;)
					{
						int EnumVal;
						const char* EnumName = T->GetEnumValueByIndex(n, &EnumVal);

						if (EnumVal == *(int*)Value)
						{
							Stream << ", " << EnumName;
							break;
						}
					}
				}
			}
			else if (TypeId & asTYPEID_SCRIPTOBJECT)
			{
				if (TypeId & asTYPEID_OBJHANDLE)
					Value = *(void**)Value;

				asIScriptObject* Object = (asIScriptObject*)Value;
				Stream << "{" << Object << "}";

				if (Object && ExpandMembers > 0)
				{
					VMCTypeInfo* Type = Object->GetObjectType();
					for (asUINT n = 0; n < Object->GetPropertyCount(); n++)
					{
						if (n == 0)
							Stream << " ";
						else
							Stream << ", ";

						Stream << Type->GetPropertyDeclaration(n) << " = " << ToString(Object->GetAddressOfProperty(n), Object->GetPropertyTypeId(n), ExpandMembers - 1, VMManager::Get(Type->GetEngine()));
					}
				}
			}
			else
			{
				if (TypeId & asTYPEID_OBJHANDLE)
					Value = *(void**)Value;

				if (Engine != nullptr)
				{
					VMCTypeInfo* Type = Base->GetTypeInfoById(TypeId);
					if (Type->GetFlags() & asOBJ_REF)
						Stream << "{" << Value << "}";

					if (Value)
					{
						auto It = ToStringCallbacks.find(Type);
						if (It == ToStringCallbacks.end())
						{
							if (Type->GetFlags() & asOBJ_TEMPLATE)
							{
								VMCTypeInfo* TmpType = Base->GetTypeInfoByName(Type->GetName());
								It = ToStringCallbacks.find(TmpType);
							}
						}

						if (It != ToStringCallbacks.end())
						{
							if (Type->GetFlags() & asOBJ_REF)
								Stream << " ";

							std::string Text = It->second(Value, ExpandMembers, this);
							Stream << Text;
						}
					}
				}
				else
					Stream << "{no engine}";
			}

			return Stream.str();
		}
		VMManager* VMDebugger::GetEngine()
		{
			return Manager;
		}
	}
}
