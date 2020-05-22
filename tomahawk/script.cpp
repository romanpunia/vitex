#include "script.h"
#include "wrapper/rest.h"
#include "wrapper/compute.h"
#include "wrapper/graphics.h"
#include "wrapper/audio.h"
#include "wrapper/network.h"
#include "wrapper/engine.h"
#include <angelscript.h>
#include <iostream>
#include <scriptstdstring/scriptstdstring.h>
#include <scriptarray/scriptarray.h>
#include <scriptany/scriptany.h>
#include <scriptdictionary/scriptdictionary.h>
#include <scriptgrid/scriptgrid.h>
#include <scriptmath/scriptmath.h>
#include <scripthandle/scripthandle.h>
#include <scripthelper/scripthelper.h>
#include <scriptbuilder/scriptbuilder.h>
#include <datetime/datetime.h>
#include <weakref/weakref.h>

namespace Tomahawk
{
    namespace Script
    {
        class ByteCodeStream : public asIBinaryStream
        {
        private:
            std::vector<asBYTE> Buffer;
            size_t RPointer, WPointer;

        public:
            ByteCodeStream() : WPointer(0), RPointer(0)
            {
            }
            ByteCodeStream(const std::string& Data) : WPointer(Data.size()), RPointer(0)
            {
                Buffer = std::vector<asBYTE>(Data.begin(), Data.end());
            }
            int Write(const void* Data, asUINT Size)
            {
                if (!Size || !Data)
                    return 0;

                Buffer.resize(Buffer.size() + Size);
                memcpy(&Buffer[WPointer], Data, Size);
                WPointer += Size;

                return (int)Size;
            }
            int Read(void* Data, asUINT Size)
            {
                if (!Data || !Size)
                    return 0;

                Int64 Avail = (Int64)Buffer.size() - (Int64)RPointer;
                if (Avail <= 0)
                    return -1;

                Size = std::min(Size, (size_t)Avail);
                memcpy(Data, &Buffer[RPointer], Size);
                RPointer += Size;

                return Size;
            }
            std::string GetResult()
            {
                return std::string(Buffer.begin(), Buffer.end());
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
        void* VMWTypeInfo::SetUserData(void* Data, UInt64 Type)
        {
            if (!IsValid())
                return nullptr;

            return Info->SetUserData(Data, Type);
        }
        void* VMWTypeInfo::GetUserData(UInt64 Type) const
        {
            if (!IsValid())
                return nullptr;

            return Info->GetUserData(Type);
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
        void* VMWFunction::SetUserData(void* UserData, UInt64 Type)
        {
            if (!IsValid())
                return nullptr;

            return Function->SetUserData(UserData, Type);
        }
        void* VMWFunction::GetUserData(UInt64 Type) const
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
            CScriptAny* Value = nullptr;
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

            auto* Result = Queue.front();
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
        int VMCThread::AtomicInc(int& Value)
        {
            return asAtomicInc(Value);
        }
        int VMCThread::AtomicDec(int& Value)
        {
            return asAtomicDec(Value);
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
                return nullptr;

            VMCManager* Engine = Context->GetEngine();
            if (!Engine)
                return nullptr;

            VMCThread* Thread = new VMCThread(Engine, Func);
            Engine->NotifyGarbageCollectorOfNewObject(Thread, Engine->GetTypeInfoByName("thread"));

            return Thread;
        }
        UInt64 VMCThread::GetIdInThread()
        {
            auto* Thread = GetThisThreadSafe();
            if (!Thread)
                return 0;

            return (UInt64)std::hash<std::thread::id>()(std::this_thread::get_id());
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

            VMWTypeInfo TypeInfo = Engine->Global().GetTypeInfoByName(TypeName);
            if (!TypeInfo.IsValid())
                return -1;

            return Set(Ref, TypeInfo.GetTypeId());
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
        VMCAsync* VMCAsync::CreateFilled(Int64 Value)
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
            return CScriptArray::Create(ArrayType.GetTypeInfo());
        }
        VMWArray VMWArray::Create(const VMWTypeInfo& ArrayType, unsigned int Length)
        {
            return CScriptArray::Create(ArrayType.GetTypeInfo(), Length);
        }
        VMWArray VMWArray::Create(const VMWTypeInfo& ArrayType, unsigned int Length, void* DefaultValue)
        {
            return CScriptArray::Create(ArrayType.GetTypeInfo(), Length, DefaultValue);
        }
        VMWArray VMWArray::Create(const VMWTypeInfo& ArrayType, void* ListBuffer)
        {
            return CScriptArray::Create(ArrayType.GetTypeInfo(), ListBuffer);
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
        void VMWAny::Store(Int64& Value)
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
        bool VMWAny::Retrieve(Int64& Value) const
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
        void VMWDictionary::Set(const std::string& Key, Int64& Value)
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
        bool VMWDictionary::Get(const std::string& Key, void* Value, int TypeId) const
        {
            if (!IsValid())
                return false;

            return Dictionary->Get(Key, Value, TypeId);
        }
        bool VMWDictionary::Get(const std::string& Key, Int64& Value) const
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

            return CScriptDictionary::Create(Engine->GetEngine());
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
            return CScriptGrid::Create(GridType.GetTypeInfo());
        }
        VMWGrid VMWGrid::Create(const VMWTypeInfo& GridType, unsigned int Width, unsigned int Height)
        {
            return CScriptGrid::Create(GridType.GetTypeInfo(), Width, Height);
        }
        VMWGrid VMWGrid::Create(const VMWTypeInfo& GridType, unsigned int Width, unsigned int Height, void* DefaultValue)
        {
            return CScriptGrid::Create(GridType.GetTypeInfo(), Width, Height, DefaultValue);
        }
        VMWGrid VMWGrid::Create(const VMWTypeInfo& GridType, void* ListBuffer)
        {
            return CScriptGrid::Create(GridType.GetTypeInfo(), ListBuffer);
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
                return nullptr;

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
        UInt64 VMWGeneric::GetArgQWord(unsigned int Argument)
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
        int VMWGeneric::SetReturnQWord(UInt64 Value)
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

            Engine->SetDefaultNamespace(std::string(Scope).append("::").append(Object).c_str());
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

            Engine->SetDefaultNamespace(std::string(Scope).append("::").append(Object).c_str());
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
        int VMWClass::SetConstructorListAddress(const char* Decl, asSFuncPtr* Value)
        {
            if (!IsValid() || !Decl)
                return -1;

            VMCManager* Engine = Manager->GetEngine();
            if (!Engine)
                return -1;

            return Engine->RegisterObjectBehaviour(Object.c_str(), asBEHAVE_LIST_CONSTRUCT, Decl, *Value, asCALL_GENERIC);
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
        int VMWModule::LoadByteCode(const std::string& Buffer, bool* WasDebugInfoStripped)
        {
            if (!IsValid())
                return -1;

            ByteCodeStream* Stream = new ByteCodeStream(Buffer);
            int R = Mod->LoadByteCode(Stream, WasDebugInfoStripped);
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
        int VMWModule::SaveByteCode(std::string& Buffer, bool StripDebugInfo) const
        {
            if (!IsValid())
                return -1;

            ByteCodeStream* Stream = new ByteCodeStream();
            int R = Mod->SaveByteCode(Stream, StripDebugInfo);
            Buffer.assign(Stream->GetResult());
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
                Info->Pointer = nullptr;
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
        VMWTypeClass VMGlobal::SetStructAddress(const char* Name, size_t Size, UInt64 Flags)
        {
            if (!Manager || !Name)
                return VMWTypeClass(Manager, "", -1);

            VMCManager* Engine = Manager->GetEngine();
            if (!Engine)
                return VMWTypeClass(Manager, Name, -1);

            return VMWTypeClass(Manager, Name, Engine->RegisterObjectType(Name, Size, (asDWORD)Flags));
        }
        VMWTypeClass VMGlobal::SetPodAddress(const char* Name, size_t Size, UInt64 Flags)
        {
            if (!Manager || !Name)
                return VMWTypeClass(Manager, "", -1);

            VMCManager* Engine = Manager->GetEngine();
            if (!Engine)
                return VMWTypeClass(Manager, Name, -1);

            return VMWTypeClass(Manager, Name, Engine->RegisterObjectType(Name, Size, (asDWORD)Flags));
        }
        VMWRefClass VMGlobal::SetClassAddress(const char* Name, UInt64 Flags)
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
                return "int64(" + std::to_string(*(Int64*)(Object)) + "), ";
            else if (TypeId == VMTypeId_UINT8)
                return "uint8(" + std::to_string(*(unsigned char*)(Object)) + "), ";
            else if (TypeId == VMTypeId_UINT16)
                return "uint16(" + std::to_string(*(unsigned short*)(Object)) + "), ";
            else if (TypeId == VMTypeId_UINT32)
                return "uint32(" + std::to_string(*(unsigned int*)(Object)) + "), ";
            else if (TypeId == VMTypeId_UINT64)
                return "uint64(" + std::to_string(*(UInt64*)(Object)) + "), ";
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

        VMCompiler::VMCompiler(VMManager* Engine) : Manager(Engine), Context(nullptr)
        {
            Builder = new CScriptBuilder();
            Builder->SetIncludeCallback(IncludeBase, this);
            Builder->SetPragmaCallback(PragmaBase, this);

			Desc.Exts.push_back(".as");
			Desc.Root = Rest::OS::GetDirectory();

            if (Manager != nullptr)
            {
                Context = Manager->CreateContext();
                Context->SetUserData(this, CompilerUD);
            }
        }
        VMCompiler::~VMCompiler()
        {
            delete Builder;
        }
        int VMCompiler::Prepare(const char* ModuleName)
        {
            if (!Manager || !Builder)
                return -1;

            return Builder->StartNewModule(Manager->GetEngine(), ModuleName);
        }
        int VMCompiler::Build()
        {
            if (!Manager || !Builder)
                return -1;

            return Builder->BuildModule();
        }
        int VMCompiler::BuildWait()
        {
            if (!Manager || !Builder)
                return -1;

            while (true)
            {
                int R = Builder->BuildModule();
                if (R == asBUILD_IN_PROGRESS)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }

                return R;
            }

            return 0;
        }
        int VMCompiler::CompileFromFile(const char* Filename)
        {
            if (!Manager || !Builder)
                return -1;

            return Builder->AddSectionFromFile(Filename);
        }
        int VMCompiler::CompileFromMemory(const char* SectionName, const char* ScriptCode, unsigned int ScriptLength, int LineOffset)
        {
            if (!Manager || !Builder)
                return -1;

            return Builder->AddSectionFromMemory(SectionName, ScriptCode, ScriptLength, LineOffset);
        }
        void VMCompiler::SetIncludeCallback(const IncludeCallback& Callback)
        {
            if (!Manager || !Builder)
                return;

            Include = Callback;
            return Builder->SetIncludeCallback(IncludeBase, this);
        }
        void VMCompiler::SetPragmaCallback(const PragmaCallback& Callback)
        {
            if (!Manager || !Builder)
                return;

            Pragma = Callback;
            return Builder->SetPragmaCallback(PragmaBase, this);
        }
		void VMCompiler::SetIncludeOptions(const Compute::IncludeDesc& NewDesc)
		{
			Desc = NewDesc;
			Desc.Exts.clear();
			Desc.Exts.push_back(".as");
		}
        void VMCompiler::Define(const char* Word)
        {
            if (!Manager || !Builder)
                return;

            return Builder->DefineWord(Word);
        }
        unsigned int VMCompiler::GetSectionsCount() const
        {
            if (!Manager || !Builder)
                return 0;

            return Builder->GetSectionCount();
        }
        std::string VMCompiler::GetSectionName(unsigned int Index) const
        {
            if (!Manager || !Builder)
                return "";

            return Builder->GetSectionName(Index);
        }
        std::vector<std::string> VMCompiler::GetMetadataForType(int TypeId)
        {
            if (!Manager || !Builder)
                return std::vector<std::string>();

            return Builder->GetMetadataForType(TypeId);
        }
        std::vector<std::string> VMCompiler::GetMetadataForFunc(const VMWFunction& Function)
        {
            if (!Manager || !Builder)
                return std::vector<std::string>();

            return Builder->GetMetadataForFunc(Function.GetFunction());
        }
        std::vector<std::string> VMCompiler::GetMetadataForProperty(int Index)
        {
            if (!Manager || !Builder)
                return std::vector<std::string>();

            return Builder->GetMetadataForVar(Index);
        }
        std::vector<std::string> VMCompiler::GetMetadataForTypeMethod(int TypeId, const VMWFunction& Method)
        {
            if (!Manager || !Builder)
                return std::vector<std::string>();

            return Builder->GetMetadataForTypeMethod(TypeId, Method.GetFunction());
        }
        std::vector<std::string> VMCompiler::GetMetadataForTypeProperty(int TypeId, int Index)
        {
            if (!Manager || !Builder)
                return std::vector<std::string>();

            return Builder->GetMetadataForTypeProperty(TypeId, Index);
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
            if (!Manager || !Builder)
                return nullptr;

            return Builder->GetModule();
        }
        int VMCompiler::IncludeBase(const char* Path, const char* From, CScriptBuilder* Builder, void* Param)
        {
            VMCompiler* Compiler = (VMCompiler*)Param;
            if (Compiler->Include)
                return Compiler->Include(Compiler, Path, From);

			Compiler->Desc.Path = Path;
			Compiler->Desc.From = From;

			auto Result = Compute::Preprocessor::ResolveInclude(Compiler->Desc);
			if (Result.Module.empty() || (!Result.IsFile && Result.IsSystem))
				return -1;

            return Compiler->CompileFromFile(Result.Module.c_str());
        }
        int VMCompiler::PragmaBase(const std::string& Pragma, CScriptBuilder& Builder, void* Param)
        {
            VMCompiler* Compiler = (VMCompiler*)Param;
            if (Compiler->Include)
                return Compiler->Pragma(Compiler, Pragma);

            Rest::Stroke Comment(&Pragma);
            Comment.Trim();

            auto Start = Comment.Find('(');
            if (!Start.Found)
                return -1;

            auto End = Comment.ReverseFind(')');
            if (!End.Found)
                return -1;

            if (Comment.StartsWith("define"))
            {
                Rest::Stroke Name(Comment);
                Name.Substring(Start.End, End.Start - Start.End).Trim();
                if (Name.Get()[0] == '\"' && Name.Get()[Name.Size() - 1] == '\"')
                    Name.Substring(1, Name.Size() - 2);

                if (!Name.Empty())
                    Compiler->Define(Name.Value());
            }
            else if (Comment.StartsWith("compile"))
            {
                auto Split = Comment.Find(',', Start.End);
                if (!Split.Found)
                    return -1;

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
                    Compiler->GetManager()->SetProperty(VMProp_ALLOW_UNSAFE_REFERENCES, Result);
                else if (Name.R() == "OPTIMIZE_BYTECODE")
                    Compiler->GetManager()->SetProperty(VMProp_OPTIMIZE_BYTECODE, Result);
                else if (Name.R() == "COPY_SCRIPT_SECTIONS")
                    Compiler->GetManager()->SetProperty(VMProp_COPY_SCRIPT_SECTIONS, Result);
                else if (Name.R() == "MAX_STACK_SIZE")
                    Compiler->GetManager()->SetProperty(VMProp_MAX_STACK_SIZE, Result);
                else if (Name.R() == "USE_CHARACTER_LITERALS")
                    Compiler->GetManager()->SetProperty(VMProp_USE_CHARACTER_LITERALS, Result);
                else if (Name.R() == "ALLOW_MULTILINE_STRINGS")
                    Compiler->GetManager()->SetProperty(VMProp_ALLOW_MULTILINE_STRINGS, Result);
                else if (Name.R() == "ALLOW_IMPLICIT_HANDLE_TYPES")
                    Compiler->GetManager()->SetProperty(VMProp_ALLOW_IMPLICIT_HANDLE_TYPES, Result);
                else if (Name.R() == "BUILD_WITHOUT_LINE_CUES")
                    Compiler->GetManager()->SetProperty(VMProp_BUILD_WITHOUT_LINE_CUES, Result);
                else if (Name.R() == "INIT_GLOBAL_VARS_AFTER_BUILD")
                    Compiler->GetManager()->SetProperty(VMProp_INIT_GLOBAL_VARS_AFTER_BUILD, Result);
                else if (Name.R() == "REQUIRE_ENUM_SCOPE")
                    Compiler->GetManager()->SetProperty(VMProp_REQUIRE_ENUM_SCOPE, Result);
                else if (Name.R() == "SCRIPT_SCANNER")
                    Compiler->GetManager()->SetProperty(VMProp_SCRIPT_SCANNER, Result);
                else if (Name.R() == "INCLUDE_JIT_INSTRUCTIONS")
                    Compiler->GetManager()->SetProperty(VMProp_INCLUDE_JIT_INSTRUCTIONS, Result);
                else if (Name.R() == "STRING_ENCODING")
                    Compiler->GetManager()->SetProperty(VMProp_STRING_ENCODING, Result);
                else if (Name.R() == "PROPERTY_ACCESSOR_MODE")
                    Compiler->GetManager()->SetProperty(VMProp_PROPERTY_ACCESSOR_MODE, Result);
                else if (Name.R() == "EXPAND_DEF_ARRAY_TO_TMPL")
                    Compiler->GetManager()->SetProperty(VMProp_EXPAND_DEF_ARRAY_TO_TMPL, Result);
                else if (Name.R() == "AUTO_GARBAGE_COLLECT")
                    Compiler->GetManager()->SetProperty(VMProp_AUTO_GARBAGE_COLLECT, Result);
                else if (Name.R() == "DISALLOW_GLOBAL_VARS")
                    Compiler->GetManager()->SetProperty(VMProp_ALWAYS_IMPL_DEFAULT_CONSTRUCT, Result);
                else if (Name.R() == "ALWAYS_IMPL_DEFAULT_CONSTRUCT")
                    Compiler->GetManager()->SetProperty(VMProp_ALWAYS_IMPL_DEFAULT_CONSTRUCT, Result);
                else if (Name.R() == "COMPILER_WARNINGS")
                    Compiler->GetManager()->SetProperty(VMProp_COMPILER_WARNINGS, Result);
                else if (Name.R() == "DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE")
                    Compiler->GetManager()->SetProperty(VMProp_DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE, Result);
                else if (Name.R() == "ALTER_SYNTAX_NAMED_ARGS")
                    Compiler->GetManager()->SetProperty(VMProp_ALTER_SYNTAX_NAMED_ARGS, Result);
                else if (Name.R() == "DISABLE_INTEGER_DIVISION")
                    Compiler->GetManager()->SetProperty(VMProp_DISABLE_INTEGER_DIVISION, Result);
                else if (Name.R() == "DISALLOW_EMPTY_LIST_ELEMENTS")
                    Compiler->GetManager()->SetProperty(VMProp_DISALLOW_EMPTY_LIST_ELEMENTS, Result);
                else if (Name.R() == "PRIVATE_PROP_AS_PROTECTED")
                    Compiler->GetManager()->SetProperty(VMProp_PRIVATE_PROP_AS_PROTECTED, Result);
                else if (Name.R() == "ALLOW_UNICODE_IDENTIFIERS")
                    Compiler->GetManager()->SetProperty(VMProp_ALLOW_UNICODE_IDENTIFIERS, Result);
                else if (Name.R() == "HEREDOC_TRIM_MODE")
                    Compiler->GetManager()->SetProperty(VMProp_HEREDOC_TRIM_MODE, Result);
                else if (Name.R() == "MAX_NESTED_CALLS")
                    Compiler->GetManager()->SetProperty(VMProp_MAX_NESTED_CALLS, Result);
                else if (Name.R() == "GENERIC_CALL_MODE")
                    Compiler->GetManager()->SetProperty(VMProp_GENERIC_CALL_MODE, Result);
                else if (Name.R() == "INIT_STACK_SIZE")
                    Compiler->GetManager()->SetProperty(VMProp_INIT_STACK_SIZE, Result);
                else if (Name.R() == "INIT_CALL_STACK_SIZE")
                    Compiler->GetManager()->SetProperty(VMProp_INIT_CALL_STACK_SIZE, Result);
                else if (Name.R() == "MAX_CALL_STACK_SIZE")
                    Compiler->GetManager()->SetProperty(VMProp_MAX_CALL_STACK_SIZE, Result);
            }
            else if (Comment.StartsWith("comment"))
            {
                auto Split = Comment.Find(',', Start.End);
                if (!Split.Found)
                    return -1;

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
                if (!Split.Found)
                    return -1;

                Rest::Stroke Name(Comment);
                Name.Substring(Start.End, Split.Start - Start.End).Trim().ToUpper();
                if (Name.Get()[0] == '\"' && Name.Get()[Name.Size() - 1] == '\"')
                    Name.Substring(1, Name.Size() - 2);

                Rest::Stroke Value(Comment);
                Value.Substring(Split.End, End.Start - Split.End).Trim();
                if (Value.Get()[0] == '\"' && Value.Get()[Value.Size() - 1] == '\"')
                    Value.Substring(1, Value.Size() - 2);

                size_t Result = Value.HasInteger() ? Value.ToUInt64() : 0;
                auto Module = Compiler->GetModule();
                if (Name.R() == "NAME")
                    Module.SetName(Value.Value());
                else if (Name.R() == "NAMESPACE")
                    Module.SetDefaultNamespace(Value.Value());
                else if (Name.R() == "ACCESS_MASK")
                    Module.SetAccessMask(Result);
            }

            return 0;
        }
        VMCompiler* VMCompiler::Get(VMContext* Context)
        {
            if (!Context)
                return nullptr;

            return (VMCompiler*)Context->GetUserData(CompilerUD);
        }
        int VMCompiler::CompilerUD = 554;

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
        int VMContext::EvalStatement(VMCompiler* Compiler, const std::string& Code, void* Return, int ReturnTypeId)
        {
            return EvalStatement(Compiler, Code.c_str(), Code.size(), Return, ReturnTypeId);
        }
        int VMContext::EvalStatement(VMCompiler* Compiler, const char* Buffer, UInt64 Length, void* Return, int ReturnTypeId)
        {
            if (!Manager || !Context || !Compiler || !Buffer || !Length || !Compiler->GetModule().IsValid())
                return -1;

            VMCManager* Engine = Manager->GetEngine();
            std::string Eval = " __EvalStatement__(){\n";
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

            VMCModule* Module = Compiler->GetModule().GetModule();
            if (Type)
                Type->Release();

            VMCFunction* Function = nullptr;
            int R = Module->CompileFunction("__EvalStatement__", Eval.c_str(), -1, 0, &Function);
            if (R < 0)
                return R;

            R = Context->Prepare(Function);
            if (R < 0)
            {
                Function->Release();
                return R;
            }

            R = Context->Execute();
            if (Return != 0 && ReturnTypeId != asTYPEID_VOID)
            {
                if (ReturnTypeId & asTYPEID_OBJHANDLE)
                {
                    if ((*reinterpret_cast<void**>(Return) == 0))
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

            UInt64 Id = VMCThread::GetIdInThread();
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
        int VMContext::SetArgByte(unsigned int Arg, unsigned char Value)
        {
            if (!Context)
                return -1;

            return Context->SetArgByte(Arg, Value);
        }
        int VMContext::SetArgWord(unsigned int Arg, unsigned short Value)
        {
            if (!Context)
                return -1;

            return Context->SetArgWord(Arg, Value);
        }
        int VMContext::SetArgDWord(unsigned int Arg, size_t Value)
        {
            if (!Context)
                return -1;

            return Context->SetArgDWord(Arg, Value);
        }
        int VMContext::SetArgQWord(unsigned int Arg, UInt64 Value)
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
        int VMContext::SetArgObjectAddress(unsigned int Arg, void* Object)
        {
            if (!Context)
                return -1;

            return Context->SetArgObject(Arg, Object);
        }
        int VMContext::SetArgAnyAddress(unsigned int Arg, void* Ptr, int TypeId)
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
        UInt64 VMContext::GetReturnQWord()
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
        void* VMContext::SetUserData(void* Data, UInt64 Type)
        {
            if (!Context)
                return nullptr;

            return Context->SetUserData(Data, Type);
        }
        void* VMContext::GetUserData(UInt64 Type) const
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

            THAWK_ERROR("uncaugth exception raised"
                        "\n\tdescription: %s"
                        "\n\tfunction: %s"
                        "\n\tmodule: %s"
                        "\n\tsource: %s"
                        "\n\tline: %i\n%.*s", Message ? Message : "undefined", Decl ? Decl : "undefined", Mod ? Mod : "undefined", Source ? Source : "undefined", Line, (int)Stack.size(), Stack.c_str());
        }
        int VMContext::ContextUD = 552;

        VMManager::VMManager() : Engine(asCreateScriptEngine()), Globals(this)
        {
            Engine->SetUserData(this, ManagerUD);
            Engine->SetContextCallbacks(RequestContext, ReturnContext, nullptr);
            Engine->SetMessageCallback(asFUNCTION(CompileLogger), this, asCALL_CDECL);
        }
        VMManager::~VMManager()
        {
            if (Engine != nullptr)
                Engine->ShutDownAndRelease();
        }
        void VMManager::EnableAll()
        {
            EnableAny();
            EnableReference();
            EnableWeakReference();
            EnableArray(true);
            EnableString();
            EnableDictionary();
            EnableGrid();
            EnableMath();
            EnableDateTime();
            EnableException();
            EnableRandom();
            EnableThread();
            EnableAsync();
            EnableLibrary();
        }
        void VMManager::EnableString()
        {
            RegisterStdString(Engine);
            RegisterStdStringUtils(Engine);
        }
        void VMManager::EnableArray(bool Default)
        {
            RegisterScriptArray(Engine, Default);
        }
        void VMManager::EnableAny()
        {
            RegisterScriptAny(Engine);
        }
        void VMManager::EnableDictionary()
        {
            RegisterScriptDictionary(Engine);
        }
        void VMManager::EnableGrid()
        {
            RegisterScriptGrid(Engine);
        }
        void VMManager::EnableMath()
        {
            RegisterScriptMath(Engine);
        }
        void VMManager::EnableDateTime()
        {
            RegisterScriptDateTime(Engine);
        }
        void VMManager::EnableException()
        {
            RegisterExceptionRoutines(Engine);
        }
        void VMManager::EnableReference()
        {
            RegisterScriptHandle(Engine);
        }
        void VMManager::EnableWeakReference()
        {
            RegisterScriptWeakRef(Engine);
        }
        void VMManager::EnableRandom()
        {
            Engine->RegisterObjectType("random", 0, asOBJ_REF);
            Engine->RegisterObjectBehaviour("random", asBEHAVE_FACTORY, "random@ f()", asFUNCTION(VMCRandom::Create), asCALL_CDECL);
            Engine->RegisterObjectBehaviour("random", asBEHAVE_ADDREF, "void f()", asMETHOD(VMCRandom, AddRef), asCALL_THISCALL);
            Engine->RegisterObjectBehaviour("random", asBEHAVE_RELEASE, "void f()", asMETHOD(VMCRandom, Release), asCALL_THISCALL);
            Engine->RegisterObjectMethod("random", "void opAssign(const random&)", asMETHODPR(VMCRandom, Assign, (VMCRandom * ), void), asCALL_THISCALL);
            Engine->RegisterObjectMethod("random", "void seed(uint)", asMETHODPR(VMCRandom, Seed, (uint32_t), void), asCALL_THISCALL);
            Engine->RegisterObjectMethod("random", "void seed(uint[]&)", asMETHODPR(VMCRandom, Seed, (CScriptArray * ), void), asCALL_THISCALL);
            Engine->RegisterObjectMethod("random", "int getI()", asMETHOD(VMCRandom, GetI), asCALL_THISCALL);
            Engine->RegisterObjectMethod("random", "uint getU()", asMETHOD(VMCRandom, GetU), asCALL_THISCALL);
            Engine->RegisterObjectMethod("random", "double getD()", asMETHOD(VMCRandom, GetD), asCALL_THISCALL);
            Engine->RegisterObjectMethod("random", "void seedFromTime()", asMETHOD(VMCRandom, SeedFromTime), asCALL_THISCALL);
        }
        void VMManager::EnableThread()
        {
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
            Engine->SetDefaultNamespace("thread");
            Engine->RegisterGlobalFunction("any@ receiveFrom(uint64 timeout)", asFUNCTION(VMCThread::ReceiveInThread), asCALL_CDECL);
            Engine->RegisterGlobalFunction("any@ receiveFromWait()", asFUNCTION(VMCThread::ReceiveWaitInThread), asCALL_CDECL);
            Engine->RegisterGlobalFunction("void sleep(uint64 timeout)", asFUNCTION(VMCThread::SleepInThread), asCALL_CDECL);
            Engine->RegisterGlobalFunction("void sendTo(any&)", asFUNCTION(VMCThread::SendInThread), asCALL_CDECL);
            Engine->RegisterGlobalFunction("thread@ create(thread_callback @func)", asFUNCTION(VMCThread::StartThread), asCALL_CDECL);
            Engine->RegisterGlobalFunction("uint64 id()", asFUNCTION(VMCThread::GetIdInThread), asCALL_CDECL);
            Engine->SetDefaultNamespace("");
        }
        void VMManager::EnableAsync()
        {
            Engine->RegisterObjectType("async", 0, asOBJ_REF | asOBJ_GC);
            Engine->RegisterObjectBehaviour("async", asBEHAVE_ADDREF, "void f()", asMETHOD(VMCAsync, AddRef), asCALL_THISCALL);
            Engine->RegisterObjectBehaviour("async", asBEHAVE_RELEASE, "void f()", asMETHOD(VMCAsync, Release), asCALL_THISCALL);
            Engine->RegisterObjectBehaviour("async", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(VMCAsync, SetGCFlag), asCALL_THISCALL);
            Engine->RegisterObjectBehaviour("async", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(VMCAsync, GetGCFlag), asCALL_THISCALL);
            Engine->RegisterObjectBehaviour("async", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(VMCAsync, GetRefCount), asCALL_THISCALL);
            Engine->RegisterObjectBehaviour("async", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(VMCAsync, EnumReferences), asCALL_THISCALL);
            Engine->RegisterObjectBehaviour("async", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(VMCAsync, ReleaseReferences), asCALL_THISCALL);
            Engine->RegisterObjectMethod("async", "any@ get()", asMETHOD(VMCAsync, Get), asCALL_THISCALL);
            Engine->RegisterObjectMethod("async", "async@ await()", asMETHOD(VMCAsync, Await), asCALL_THISCALL);
        }
        void VMManager::EnableLibrary()
        {
            Wrapper::Rest::Enable(this);
            Wrapper::Compute::Enable(this);
            Wrapper::Network::Enable(this);
            Wrapper::Network::HTTP::Enable(this);
            Wrapper::Network::SMTP::Enable(this);
            Wrapper::Network::BSON::Enable(this);
            Wrapper::Network::MongoDB::Enable(this);
            Wrapper::Audio::Enable(this);
            Wrapper::Graphics::Enable(this);
            Wrapper::Engine::Enable(this);
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
        void VMManager::GetStatistics(size_t* CurrentSize, size_t* TotalDestroyed, size_t* TotalDetected, size_t* NewObjects, size_t* TotalNewDestroyed) const
        {
            size_t asCurrentSize, asTotalDestroyed, asTotalDetected, asNewObjects, asTotalNewDestroyed;
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
            DocumentRoot.assign(Value);
            if (DocumentRoot.empty())
                return Safe.unlock();

            if (!Rest::Stroke(&DocumentRoot).EndsOf("/\\"))
            {
#ifdef THAWK_MICROSOFT
                DocumentRoot.append(1, '\\');
#else
                DocumentRoot.append(1, '/');
#endif
            }
            Safe.unlock();
        }
        std::string VMManager::GetDocumentRoot() const
        {
            return DocumentRoot;
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