#include "script.h"
#include "../script/compiler/compiler.h"
#include "../script/std-lib.h"
#include "../script/core-lib.h"
#include "../script/gui-lib.h"
#include <iostream>
#include <sstream>

namespace
{
	class CByteCodeStream : public asIBinaryStream
	{
	private:
		std::vector<asBYTE> Code;
		int ReadPos, WritePos;

	public:
		CByteCodeStream() : ReadPos(0), WritePos(0)
		{
		}
		CByteCodeStream(const std::vector<asBYTE>& Data) : Code(Data), ReadPos(0), WritePos(0)
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

}

namespace Tomahawk
{
	namespace Script
	{
		int VMFuncStore::AtomicNotifyGC(const char* TypeName, void* Object)
		{
			if (!TypeName || !Object)
				return -1;

			VMCContext* Context = asGetActiveContext();
			if (!Context)
				return -1;

			VMManager* Engine = VMManager::Get(Context->GetEngine());
			if (!Engine)
				return -1;

			VMTypeInfo Type = Engine->Global().GetTypeInfoByName(TypeName);
			return Engine->NotifyOfNewObject(Object, Type.GetTypeInfo());
		}
		asSFuncPtr* VMFuncStore::CreateFunctionBase(void(*Base)(), int Type)
		{
			asSFuncPtr* Ptr = new asSFuncPtr(Type);
			Ptr->ptr.f.func = reinterpret_cast<asFUNCTION_t>(Base);
			return Ptr;
		}
		asSFuncPtr* VMFuncStore::CreateMethodBase(const void* Base, size_t Size, int Type)
		{
			asSFuncPtr* Ptr = new asSFuncPtr(Type);
			Ptr->CopyMethodPtr(Base, Size);
			return Ptr;
		}
		asSFuncPtr* VMFuncStore::CreateDummyBase()
		{
			return new asSFuncPtr(0);
		}
		void VMFuncStore::ReleaseFunctor(asSFuncPtr** Ptr)
		{
			if (!Ptr || !*Ptr)
				return;

			delete *Ptr;
			*Ptr = nullptr;
		}

		VMMessage::VMMessage(asSMessageInfo* Msg) : Info(Msg)
		{
		}
		const char* VMMessage::GetSection() const
		{
			if (!IsValid())
				return nullptr;

			return Info->section;
		}
		const char* VMMessage::GetText() const
		{
			if (!IsValid())
				return nullptr;

			return Info->message;
		}
		VMLogType VMMessage::GetType() const
		{
			if (!IsValid())
				return VMLogType_INFORMATION;

			return (VMLogType)Info->type;
		}
		int VMMessage::GetRow() const
		{
			if (!IsValid())
				return -1;

			return Info->row;
		}
		int VMMessage::GetColumn() const
		{
			if (!IsValid())
				return -1;

			return Info->col;
		}
		asSMessageInfo* VMMessage::GetMessageInfo() const
		{
			return Info;
		}
		bool VMMessage::IsValid() const
		{
			return Info != nullptr;
		}

		VMTypeInfo::VMTypeInfo(VMCTypeInfo* TypeInfo) : Info(TypeInfo)
		{
			Manager = (Info ? VMManager::Get(Info->GetEngine()) : nullptr);
		}
		void VMTypeInfo::ForEachProperty(const PropertyCallback& Callback)
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
		void VMTypeInfo::ForEachMethod(const MethodCallback& Callback)
		{
			if (!Callback || !IsValid())
				return;

			unsigned int Count = Info->GetMethodCount();
			for (unsigned int i = 0; i < Count; i++)
			{
				VMFunction Method = Info->GetMethodByIndex(i);
				if (Method.IsValid())
					Callback(this, &Method);
			}
		}
		const char* VMTypeInfo::GetGroup() const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetConfigGroup();
		}
		size_t VMTypeInfo::GetAccessMask() const
		{
			if (!IsValid())
				return 0;

			return Info->GetAccessMask();
		}
		VMModule VMTypeInfo::GetModule() const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetModule();
		}
		int VMTypeInfo::AddRef() const
		{
			if (!IsValid())
				return -1;

			return Info->AddRef();
		}
		int VMTypeInfo::Release()
		{
			if (!IsValid())
				return -1;

			int R = Info->Release();
			if (R <= 0)
				Info = nullptr;

			return R;
		}
		const char* VMTypeInfo::GetName() const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetName();
		}
		const char* VMTypeInfo::GetNamespace() const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetNamespace();
		}
		VMTypeInfo VMTypeInfo::GetBaseType() const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetBaseType();
		}
		bool VMTypeInfo::DerivesFrom(const VMTypeInfo& Type) const
		{
			if (!IsValid())
				return false;

			return Info->DerivesFrom(Type.GetTypeInfo());
		}
		size_t VMTypeInfo::GetFlags() const
		{
			if (!IsValid())
				return 0;

			return Info->GetFlags();
		}
		unsigned int VMTypeInfo::GetSize() const
		{
			if (!IsValid())
				return 0;

			return Info->GetSize();
		}
		int VMTypeInfo::GetTypeId() const
		{
			if (!IsValid())
				return -1;

			return Info->GetTypeId();
		}
		int VMTypeInfo::GetSubTypeId(unsigned int SubTypeIndex) const
		{
			if (!IsValid())
				return -1;

			return Info->GetSubTypeId(SubTypeIndex);
		}
		VMTypeInfo VMTypeInfo::GetSubType(unsigned int SubTypeIndex) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetSubType(SubTypeIndex);
		}
		unsigned int VMTypeInfo::GetSubTypeCount() const
		{
			if (!IsValid())
				return 0;

			return Info->GetSubTypeCount();
		}
		unsigned int VMTypeInfo::GetInterfaceCount() const
		{
			if (!IsValid())
				return 0;

			return Info->GetInterfaceCount();
		}
		VMTypeInfo VMTypeInfo::GetInterface(unsigned int Index) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetInterface(Index);
		}
		bool VMTypeInfo::Implements(const VMTypeInfo& Type) const
		{
			if (!IsValid())
				return false;

			return Info->Implements(Type.GetTypeInfo());
		}
		unsigned int VMTypeInfo::GetFactoriesCount() const
		{
			if (!IsValid())
				return 0;

			return Info->GetFactoryCount();
		}
		VMFunction VMTypeInfo::GetFactoryByIndex(unsigned int Index) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetFactoryByIndex(Index);
		}
		VMFunction VMTypeInfo::GetFactoryByDecl(const char* Decl) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetFactoryByDecl(Decl);
		}
		unsigned int VMTypeInfo::GetMethodsCount() const
		{
			if (!IsValid())
				return 0;

			return Info->GetMethodCount();
		}
		VMFunction VMTypeInfo::GetMethodByIndex(unsigned int Index, bool GetVirtual) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetMethodByIndex(Index, GetVirtual);
		}
		VMFunction VMTypeInfo::GetMethodByName(const char* Name, bool GetVirtual) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetMethodByName(Name, GetVirtual);
		}
		VMFunction VMTypeInfo::GetMethodByDecl(const char* Decl, bool GetVirtual) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetMethodByDecl(Decl, GetVirtual);
		}
		unsigned int VMTypeInfo::GetPropertiesCount() const
		{
			if (!IsValid())
				return 0;

			return Info->GetPropertyCount();
		}
		int VMTypeInfo::GetProperty(unsigned int Index, VMFuncProperty* Out) const
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
		const char* VMTypeInfo::GetPropertyDeclaration(unsigned int Index, bool IncludeNamespace) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetPropertyDeclaration(Index, IncludeNamespace);
		}
		unsigned int VMTypeInfo::GetBehaviourCount() const
		{
			if (!IsValid())
				return 0;

			return Info->GetBehaviourCount();
		}
		VMFunction VMTypeInfo::GetBehaviourByIndex(unsigned int Index, VMBehave* OutBehaviour) const
		{
			if (!IsValid())
				return nullptr;

			asEBehaviours Out;
			VMCFunction* Result = Info->GetBehaviourByIndex(Index, &Out);
			if (OutBehaviour != nullptr)
				*OutBehaviour = (VMBehave)Out;

			return Result;
		}
		unsigned int VMTypeInfo::GetChildFunctionDefCount() const
		{
			if (!IsValid())
				return 0;

			return Info->GetChildFuncdefCount();
		}
		VMTypeInfo VMTypeInfo::GetChildFunctionDef(unsigned int Index) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetChildFuncdef(Index);
		}
		VMTypeInfo VMTypeInfo::GetParentType() const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetParentType();
		}
		unsigned int VMTypeInfo::GetEnumValueCount() const
		{
			if (!IsValid())
				return 0;

			return Info->GetEnumValueCount();
		}
		const char* VMTypeInfo::GetEnumValueByIndex(unsigned int Index, int* OutValue) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetEnumValueByIndex(Index, OutValue);
		}
		VMFunction VMTypeInfo::GetFunctionDefSignature() const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetFuncdefSignature();
		}
		void* VMTypeInfo::SetUserData(void* Data, uint64_t Type)
		{
			if (!IsValid())
				return nullptr;

			return Info->SetUserData(Data, Type);
		}
		void* VMTypeInfo::GetUserData(uint64_t Type) const
		{
			if (!IsValid())
				return nullptr;

			return Info->GetUserData(Type);
		}
		bool VMTypeInfo::IsHandle() const
		{
			if (!IsValid())
				return false;

			return IsHandle(Info->GetTypeId());
		}
		bool VMTypeInfo::IsValid() const
		{
			return Manager != nullptr && Info != nullptr;
		}
		VMCTypeInfo* VMTypeInfo::GetTypeInfo() const
		{
			return Info;
		}
		VMManager* VMTypeInfo::GetManager() const
		{
			return Manager;
		}
		bool VMTypeInfo::IsHandle(int TypeId)
		{
			return (TypeId & asTYPEID_OBJHANDLE || TypeId & asTYPEID_HANDLETOCONST);
		}
		bool VMTypeInfo::IsScriptObject(int TypeId)
		{
			return (TypeId & asTYPEID_SCRIPTOBJECT);
		}

		VMFunction::VMFunction(VMCFunction* Base) : Function(Base)
		{
			Manager = (Base ? VMManager::Get(Base->GetEngine()) : nullptr);
		}
		int VMFunction::AddRef() const
		{
			if (!IsValid())
				return -1;

			return Function->AddRef();
		}
		int VMFunction::Release()
		{
			if (!IsValid())
				return -1;

			int R = Function->Release();
			if (R <= 0)
				Function = nullptr;

			return R;
		}
		int VMFunction::GetId() const
		{
			if (!IsValid())
				return -1;

			return Function->GetId();
		}
		VMFuncType VMFunction::GetType() const
		{
			if (!IsValid())
				return VMFuncType_DUMMY;

			return (VMFuncType)Function->GetFuncType();
		}
		const char* VMFunction::GetModuleName() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetModuleName();
		}
		VMModule VMFunction::GetModule() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetModule();
		}
		const char* VMFunction::GetSectionName() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetScriptSectionName();
		}
		const char* VMFunction::GetGroup() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetConfigGroup();
		}
		size_t VMFunction::GetAccessMask() const
		{
			if (!IsValid())
				return -1;

			return Function->GetAccessMask();
		}
		VMTypeInfo VMFunction::GetObjectType() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetObjectType();
		}
		const char* VMFunction::GetObjectName() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetObjectName();
		}
		const char* VMFunction::GetName() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetName();
		}
		const char* VMFunction::GetNamespace() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetNamespace();
		}
		const char* VMFunction::GetDecl(bool IncludeObjectName, bool IncludeNamespace, bool IncludeArgNames) const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetDeclaration(IncludeObjectName, IncludeNamespace, IncludeArgNames);
		}
		bool VMFunction::IsReadOnly() const
		{
			if (!IsValid())
				return false;

			return Function->IsPrivate();
		}
		bool VMFunction::IsPrivate() const
		{
			if (!IsValid())
				return false;

			return Function->IsPrivate();
		}
		bool VMFunction::IsProtected() const
		{
			if (!IsValid())
				return false;

			return Function->IsProtected();
		}
		bool VMFunction::IsFinal() const
		{
			if (!IsValid())
				return false;

			return Function->IsFinal();
		}
		bool VMFunction::IsOverride() const
		{
			if (!IsValid())
				return false;

			return Function->IsOverride();
		}
		bool VMFunction::IsShared() const
		{
			if (!IsValid())
				return false;

			return Function->IsShared();
		}
		bool VMFunction::IsExplicit() const
		{
			if (!IsValid())
				return false;

			return Function->IsExplicit();
		}
		bool VMFunction::IsProperty() const
		{
			if (!IsValid())
				return false;

			return Function->IsProperty();
		}
		unsigned int VMFunction::GetArgsCount() const
		{
			if (!IsValid())
				return 0;

			return Function->GetParamCount();
		}
		int VMFunction::GetArg(unsigned int Index, int* TypeId, size_t* Flags, const char** Name, const char** DefaultArg) const
		{
			if (!IsValid())
				return -1;

			asDWORD asFlags;
			int R = Function->GetParam(Index, TypeId, &asFlags, Name, DefaultArg);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;

			return R;
		}
		int VMFunction::GetReturnTypeId(size_t* Flags) const
		{
			if (!IsValid())
				return -1;

			asDWORD asFlags;
			int R = Function->GetReturnTypeId(&asFlags);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;

			return R;
		}
		int VMFunction::GetTypeId() const
		{
			if (!IsValid())
				return -1;

			return Function->GetTypeId();
		}
		bool VMFunction::IsCompatibleWithTypeId(int TypeId) const
		{
			if (!IsValid())
				return false;

			return Function->IsCompatibleWithTypeId(TypeId);
		}
		void* VMFunction::GetDelegateObject() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetDelegateObject();
		}
		VMTypeInfo VMFunction::GetDelegateObjectType() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetDelegateObjectType();
		}
		VMFunction VMFunction::GetDelegateFunction() const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetDelegateFunction();
		}
		unsigned int VMFunction::GetPropertiesCount() const
		{
			if (!IsValid())
				return 0;

			return Function->GetVarCount();
		}
		int VMFunction::GetProperty(unsigned int Index, const char** Name, int* TypeId) const
		{
			if (!IsValid())
				return -1;

			return Function->GetVar(Index, Name, TypeId);
		}
		const char* VMFunction::GetPropertyDecl(unsigned int Index, bool IncludeNamespace) const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetVarDecl(Index, IncludeNamespace);
		}
		int VMFunction::FindNextLineWithCode(int Line) const
		{
			if (!IsValid())
				return -1;

			return Function->FindNextLineWithCode(Line);
		}
		void* VMFunction::SetUserData(void* UserData, uint64_t Type)
		{
			if (!IsValid())
				return nullptr;

			return Function->SetUserData(UserData, Type);
		}
		void* VMFunction::GetUserData(uint64_t Type) const
		{
			if (!IsValid())
				return nullptr;

			return Function->GetUserData(Type);
		}
		bool VMFunction::IsValid() const
		{
			return Manager != nullptr && Function != nullptr;
		}
		VMCFunction* VMFunction::GetFunction() const
		{
			return Function;
		}
		VMManager* VMFunction::GetManager() const
		{
			return Manager;
		}

		VMObject::VMObject(VMCObject* Base) : Object(Base)
		{
		}
		int VMObject::AddRef() const
		{
			if (!IsValid())
				return 0;

			return Object->AddRef();
		}
		int VMObject::Release()
		{
			if (!IsValid())
				return 0;

			int R = Object->Release();
			Object = nullptr;

			return R;
		}
		VMTypeInfo VMObject::GetObjectType()
		{
			if (!IsValid())
				return nullptr;

			return Object->GetObjectType();
		}
		int VMObject::GetTypeId()
		{
			if (!IsValid())
				return 0;

			return Object->GetTypeId();
		}
		int VMObject::GetPropertyTypeId(unsigned int Id) const
		{
			if (!IsValid())
				return 0;

			return Object->GetPropertyTypeId(Id);
		}
		unsigned int VMObject::GetPropertiesCount() const
		{
			if (!IsValid())
				return 0;

			return Object->GetPropertyCount();
		}
		const char* VMObject::GetPropertyName(unsigned int Id) const
		{
			if (!IsValid())
				return 0;

			return Object->GetPropertyName(Id);
		}
		void* VMObject::GetAddressOfProperty(unsigned int Id)
		{
			if (!IsValid())
				return nullptr;

			return Object->GetAddressOfProperty(Id);
		}
		VMManager* VMObject::GetManager() const
		{
			if (!IsValid())
				return nullptr;

			return VMManager::Get(Object->GetEngine());
		}
		int VMObject::CopyFrom(const VMObject& Other)
		{
			if (!IsValid())
				return -1;

			return Object->CopyFrom(Other.GetObject());
		}
		void* VMObject::SetUserData(void* Data, uint64_t Type)
		{
			if (!IsValid())
				return nullptr;

			return Object->SetUserData(Data, (asPWORD)Type);
		}
		void* VMObject::GetUserData(uint64_t Type) const
		{
			if (!IsValid())
				return nullptr;

			return Object->GetUserData((asPWORD)Type);
		}
		bool VMObject::IsValid() const
		{
			return Object != nullptr;
		}
		VMCObject* VMObject::GetObject() const
		{
			return Object;
		}

		VMGeneric::VMGeneric(VMCGeneric* Base) : Generic(Base)
		{
			Manager = (Generic ? VMManager::Get(Generic->GetEngine()) : nullptr);
		}
		void* VMGeneric::GetObjectAddress()
		{
			if (!IsValid())
				return nullptr;

			return Generic->GetObject();
		}
		int VMGeneric::GetObjectTypeId() const
		{
			if (!IsValid())
				return -1;

			return Generic->GetObjectTypeId();
		}
		int VMGeneric::GetArgsCount() const
		{
			if (!IsValid())
				return -1;

			return Generic->GetArgCount();
		}
		int VMGeneric::GetArgTypeId(unsigned int Argument, size_t* Flags) const
		{
			if (!IsValid())
				return -1;

			asDWORD asFlags;
			int R = Generic->GetArgTypeId(Argument, &asFlags);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;

			return R;
		}
		unsigned char VMGeneric::GetArgByte(unsigned int Argument)
		{
			if (!IsValid())
				return 0;

			return Generic->GetArgByte(Argument);
		}
		unsigned short VMGeneric::GetArgWord(unsigned int Argument)
		{
			if (!IsValid())
				return 0;

			return Generic->GetArgWord(Argument);
		}
		size_t VMGeneric::GetArgDWord(unsigned int Argument)
		{
			if (!IsValid())
				return 0;

			return Generic->GetArgDWord(Argument);
		}
		uint64_t VMGeneric::GetArgQWord(unsigned int Argument)
		{
			if (!IsValid())
				return 0;

			return Generic->GetArgQWord(Argument);
		}
		float VMGeneric::GetArgFloat(unsigned int Argument)
		{
			if (!IsValid())
				return 0.0f;

			return Generic->GetArgFloat(Argument);
		}
		double VMGeneric::GetArgDouble(unsigned int Argument)
		{
			if (!IsValid())
				return 0.0;

			return Generic->GetArgDouble(Argument);
		}
		void* VMGeneric::GetArgAddress(unsigned int Argument)
		{
			if (!IsValid())
				return nullptr;

			return Generic->GetArgAddress(Argument);
		}
		void* VMGeneric::GetArgObjectAddress(unsigned int Argument)
		{
			if (!IsValid())
				return nullptr;

			return Generic->GetArgObject(Argument);
		}
		void* VMGeneric::GetAddressOfArg(unsigned int Argument)
		{
			if (!IsValid())
				return nullptr;

			return Generic->GetAddressOfArg(Argument);
		}
		int VMGeneric::GetReturnTypeId(size_t* Flags) const
		{
			if (!IsValid())
				return -1;

			asDWORD asFlags;
			int R = Generic->GetReturnTypeId(&asFlags);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;

			return R;
		}
		int VMGeneric::SetReturnByte(unsigned char Value)
		{
			if (!IsValid())
				return -1;

			return Generic->SetReturnByte(Value);
		}
		int VMGeneric::SetReturnWord(unsigned short Value)
		{
			if (!IsValid())
				return -1;

			return Generic->SetReturnWord(Value);
		}
		int VMGeneric::SetReturnDWord(size_t Value)
		{
			if (!IsValid())
				return -1;

			return Generic->SetReturnDWord(Value);
		}
		int VMGeneric::SetReturnQWord(uint64_t Value)
		{
			if (!IsValid())
				return -1;

			return Generic->SetReturnQWord(Value);
		}
		int VMGeneric::SetReturnFloat(float Value)
		{
			if (!IsValid())
				return -1;

			return Generic->SetReturnFloat(Value);
		}
		int VMGeneric::SetReturnDouble(double Value)
		{
			if (!IsValid())
				return -1;

			return Generic->SetReturnDouble(Value);
		}
		int VMGeneric::SetReturnAddress(void* Address)
		{
			if (!IsValid())
				return -1;

			return Generic->SetReturnAddress(Address);
		}
		int VMGeneric::SetReturnObjectAddress(void* Object)
		{
			if (!IsValid())
				return -1;

			return Generic->SetReturnObject(Object);
		}
		void* VMGeneric::GetAddressOfReturnLocation()
		{
			if (!IsValid())
				return nullptr;

			return Generic->GetAddressOfReturnLocation();
		}
		bool VMGeneric::IsValid() const
		{
			return Manager != nullptr && Generic != nullptr;
		}
		VMCGeneric* VMGeneric::GetGeneric() const
		{
			return Generic;
		}
		VMManager* VMGeneric::GetManager() const
		{
			return Manager;
		}

		VMClass::VMClass(VMManager* Engine, const std::string& Name, int Type) : Manager(Engine), Object(Name), TypeId(Type)
		{
		}
		int VMClass::SetFunctionDef(const char* Decl)
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
		int VMClass::SetOperatorCopyAddress(asSFuncPtr* Value)
		{
			if (!IsValid() || !Value)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			Core::Parser Decl = Core::Form("%s& opAssign(const %s &in)", Object.c_str(), Object.c_str());
			return Engine->RegisterObjectMethod(Object.c_str(), Decl.Get(), *Value, asCALL_THISCALL);
		}
		int VMClass::SetBehaviourAddress(const char* Decl, VMBehave Behave, asSFuncPtr* Value, VMCall Type)
		{
			if (!IsValid() || !Decl || !Value)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			return Engine->RegisterObjectBehaviour(Object.c_str(), (asEBehaviours)Behave, Decl, *Value, (asECallConvTypes)Type);
		}
		int VMClass::SetPropertyAddress(const char* Decl, int Offset)
		{
			if (!IsValid() || !Decl)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			return Engine->RegisterObjectProperty(Object.c_str(), Decl, Offset);
		}
		int VMClass::SetPropertyStaticAddress(const char* Decl, void* Value)
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
		int VMClass::SetOperatorAddress(const char* Decl, asSFuncPtr* Value, VMCall Type)
		{
			return SetMethodAddress(Decl, Value, Type);
		}
		int VMClass::SetMethodAddress(const char* Decl, asSFuncPtr* Value, VMCall Type)
		{
			if (!IsValid() || !Decl)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			return Engine->RegisterObjectMethod(Object.c_str(), Decl, *Value, (asECallConvTypes)Type);
		}
		int VMClass::SetMethodStaticAddress(const char* Decl, asSFuncPtr* Value, VMCall Type)
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
		int VMClass::SetConstructorAddress(const char* Decl, asSFuncPtr* Value, VMCall Type)
		{
			if (!IsValid() || !Decl)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			return Engine->RegisterObjectBehaviour(Object.c_str(), asBEHAVE_CONSTRUCT, Decl, *Value, (asECallConvTypes)Type);
		}
		int VMClass::SetConstructorListAddress(const char* Decl, asSFuncPtr* Value, VMCall Type)
		{
			if (!IsValid() || !Decl)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			return Engine->RegisterObjectBehaviour(Object.c_str(), asBEHAVE_LIST_CONSTRUCT, Decl, *Value, (asECallConvTypes)Type);
		}
		int VMClass::SetDestructorAddress(const char* Decl, asSFuncPtr* Value)
		{
			if (!IsValid() || !Decl)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			return Engine->RegisterObjectBehaviour(Object.c_str(), asBEHAVE_DESTRUCT, Decl, *Value, asCALL_CDECL_OBJFIRST);
		}
		int VMClass::GetTypeId() const
		{
			return TypeId;
		}
		bool VMClass::IsValid() const
		{
			return Manager != nullptr && TypeId >= 0;
		}
		std::string VMClass::GetName() const
		{
			return Object;
		}
		VMManager* VMClass::GetManager() const
		{
			return Manager;
		}
		Core::Parser VMClass::GetOperator(VMOpFunc Op, const char* Out, const char* Args, bool Const, bool Right)
		{
			switch (Op)
			{
				case VMOpFunc_Neg:
					if (Right)
						return "";

					return Core::Form("%s opNeg()%s", Out, Const ? " const" : "");
				case VMOpFunc_Com:
					if (Right)
						return "";

					return Core::Form("%s opCom()%s", Out, Const ? " const" : "");
				case VMOpFunc_PreInc:
					if (Right)
						return "";

					return Core::Form("%s opPreInc()%s", Out, Const ? " const" : "");
				case VMOpFunc_PreDec:
					if (Right)
						return "";

					return Core::Form("%s opPreDec()%s", Out, Const ? " const" : "");
				case VMOpFunc_PostInc:
					if (Right)
						return "";

					return Core::Form("%s opPostInc()%s", Out, Const ? " const" : "");
				case VMOpFunc_PostDec:
					if (Right)
						return "";

					return Core::Form("%s opPostDec()%s", Out, Const ? " const" : "");
				case VMOpFunc_Equals:
					if (Right)
						return "";

					return Core::Form("%s opEquals(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_Cmp:
					if (Right)
						return "";

					return Core::Form("%s opCmp(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_Assign:
					if (Right)
						return "";

					return Core::Form("%s opAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_AddAssign:
					if (Right)
						return "";

					return Core::Form("%s opAddAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_SubAssign:
					if (Right)
						return "";

					return Core::Form("%s opSubAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_MulAssign:
					if (Right)
						return "";

					return Core::Form("%s opMulAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_DivAssign:
					if (Right)
						return "";

					return Core::Form("%s opDivAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_ModAssign:
					if (Right)
						return "";

					return Core::Form("%s opModAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_PowAssign:
					if (Right)
						return "";

					return Core::Form("%s opPowAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_AndAssign:
					if (Right)
						return "";

					return Core::Form("%s opAndAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_OrAssign:
					if (Right)
						return "";

					return Core::Form("%s opOrAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_XOrAssign:
					if (Right)
						return "";

					return Core::Form("%s opXorAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_ShlAssign:
					if (Right)
						return "";

					return Core::Form("%s opShlAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_ShrAssign:
					if (Right)
						return "";

					return Core::Form("%s opShrAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_UshrAssign:
					if (Right)
						return "";

					return Core::Form("%s opUshrAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_Add:
					return Core::Form("%s opAdd%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_Sub:
					return Core::Form("%s opSub%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_Mul:
					return Core::Form("%s opMul%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_Div:
					return Core::Form("%s opDiv%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_Mod:
					return Core::Form("%s opMod%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_Pow:
					return Core::Form("%s opPow%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_And:
					return Core::Form("%s opAnd%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_Or:
					return Core::Form("%s opOr%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_XOr:
					return Core::Form("%s opXor%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_Shl:
					return Core::Form("%s opShl%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_Shr:
					return Core::Form("%s opShr%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_Ushr:
					return Core::Form("%s opUshr%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_Index:
					if (Right)
						return "";

					return Core::Form("%s opIndex(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_Call:
					if (Right)
						return "";

					return Core::Form("%s opCall(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc_Cast:
					if (Right)
						return "";

					return Core::Form("%s opCast()%s", Out, Const ? " const" : "");
				case VMOpFunc_ImplCast:
					if (Right)
						return "";

					return Core::Form("%s opImplCast()%s", Out, Const ? " const" : "");
				default:
					return "";
			}
		}

		VMInterface::VMInterface(VMManager* Engine, const std::string& Name, int Type) : Manager(Engine), Object(Name), TypeId(Type)
		{
		}
		int VMInterface::SetMethod(const char* Decl)
		{
			if (!IsValid() || !Decl)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			return Engine->RegisterInterfaceMethod(Object.c_str(), Decl);
		}
		int VMInterface::GetTypeId() const
		{
			return TypeId;
		}
		bool VMInterface::IsValid() const
		{
			return Manager != nullptr && TypeId >= 0;
		}
		std::string VMInterface::GetName() const
		{
			return Object;
		}
		VMManager* VMInterface::GetManager() const
		{
			return Manager;
		}

		VMEnum::VMEnum(VMManager* Engine, const std::string& Name, int Type) : Manager(Engine), Object(Name), TypeId(Type)
		{
		}
		int VMEnum::SetValue(const char* Name, int Value)
		{
			if (!IsValid() || !Name)
				return -1;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return -1;

			return Engine->RegisterEnumValue(Object.c_str(), Name, Value);
		}
		int VMEnum::GetTypeId() const
		{
			return TypeId;
		}
		bool VMEnum::IsValid() const
		{
			return Manager != nullptr && TypeId >= 0;
		}
		std::string VMEnum::GetName() const
		{
			return Object;
		}
		VMManager* VMEnum::GetManager() const
		{
			return Manager;
		}

		VMModule::VMModule(VMCModule* Type) : Mod(Type)
		{
			Manager = (Mod ? VMManager::Get(Mod->GetEngine()) : nullptr);
		}
		void VMModule::SetName(const char* Name)
		{
			if (!IsValid())
				return;

			return Mod->SetName(Name);
		}
		int VMModule::AddSection(const char* Name, const char* Code, size_t CodeLength, int LineOffset)
		{
			if (!IsValid())
				return -1;

			return Mod->AddScriptSection(Name, Code, CodeLength, LineOffset);
		}
		int VMModule::RemoveFunction(const VMFunction& Function)
		{
			if (!IsValid())
				return -1;

			return Mod->RemoveFunction(Function.GetFunction());
		}
		int VMModule::ResetProperties(VMCContext* Context)
		{
			if (!IsValid())
				return -1;

			return Mod->ResetGlobalVars(Context);
		}
		int VMModule::Build()
		{
			if (!IsValid())
				return -1;

			Manager->Lock();
			int R = Mod->Build();
			Manager->Unlock();

			return R;
		}
		int VMModule::LoadByteCode(VMByteCode* Info)
		{
			int R = -1;
			if (!Info || !IsValid())
				return R;

			Manager->Lock();
			{
				CByteCodeStream* Stream = new CByteCodeStream(Info->Data);
				R = Mod->LoadByteCode(Stream, &Info->Debug);
				delete Stream;
			}
			Manager->Unlock();

			return R;
		}
		int VMModule::Discard()
		{
			if (!IsValid())
				return -1;

			Manager->Lock();
			{
				Mod->Discard();
				Mod = nullptr;
			}
			Manager->Unlock();

			return 0;
		}
		int VMModule::BindImportedFunction(size_t ImportIndex, const VMFunction& Function)
		{
			if (!IsValid())
				return -1;

			return Mod->BindImportedFunction((asUINT)ImportIndex, Function.GetFunction());
		}
		int VMModule::UnbindImportedFunction(size_t ImportIndex)
		{
			if (!IsValid())
				return -1;

			return Mod->UnbindImportedFunction((asUINT)ImportIndex);
		}
		int VMModule::BindAllImportedFunctions()
		{
			if (!IsValid())
				return -1;

			return Mod->BindAllImportedFunctions();
		}
		int VMModule::UnbindAllImportedFunctions()
		{
			if (!IsValid())
				return -1;

			return Mod->UnbindAllImportedFunctions();
		}
		int VMModule::CompileFunction(const char* SectionName, const char* Code, int LineOffset, size_t CompileFlags, VMFunction* OutFunction)
		{
			if (!IsValid())
				return -1;

			VMCFunction* OutFunc = nullptr;
			Manager->Lock();
			int R = Mod->CompileFunction(SectionName, Code, LineOffset, (asDWORD)CompileFlags, &OutFunc);
			Manager->Unlock();

			if (OutFunction != nullptr)
				*OutFunction = VMFunction(OutFunc);

			return R;
		}
		int VMModule::CompileProperty(const char* SectionName, const char* Code, int LineOffset)
		{
			if (!IsValid())
				return -1;

			Manager->Lock();
			int R = Mod->CompileGlobalVar(SectionName, Code, LineOffset);
			Manager->Unlock();

			return R;
		}
		int VMModule::SetDefaultNamespace(const char* NameSpace)
		{
			if (!IsValid())
				return -1;

			return Mod->SetDefaultNamespace(NameSpace);
		}
		void* VMModule::GetAddressOfProperty(size_t Index)
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetAddressOfGlobalVar(Index);
		}
		int VMModule::RemoveProperty(size_t Index)
		{
			if (!IsValid())
				return -1;

			return Mod->RemoveGlobalVar(Index);
		}
		size_t VMModule::SetAccessMask(size_t AccessMask)
		{
			if (!IsValid())
				return 0;

			return Mod->SetAccessMask((asDWORD)AccessMask);
		}
		size_t VMModule::GetFunctionCount() const
		{
			if (!IsValid())
				return 0;

			return Mod->GetFunctionCount();
		}
		VMFunction VMModule::GetFunctionByIndex(size_t Index) const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetFunctionByIndex((asUINT)Index);
		}
		VMFunction VMModule::GetFunctionByDecl(const char* Decl) const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetFunctionByDecl(Decl);
		}
		VMFunction VMModule::GetFunctionByName(const char* Name) const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetFunctionByName(Name);
		}
		int VMModule::GetTypeIdByDecl(const char* Decl) const
		{
			if (!IsValid())
				return -1;

			return Mod->GetTypeIdByDecl(Decl);
		}
		int VMModule::GetImportedFunctionIndexByDecl(const char* Decl) const
		{
			if (!IsValid())
				return -1;

			return Mod->GetImportedFunctionIndexByDecl(Decl);
		}
		int VMModule::SaveByteCode(VMByteCode* Info) const
		{
			int R = -1;
			if (!Info || !IsValid())
				return R;

			Manager->Lock();
			{
				CByteCodeStream* Stream = new CByteCodeStream();
				R = Mod->SaveByteCode(Stream, Info->Debug);
				Info->Data = Stream->GetCode();
				delete Stream;
			}
			Manager->Unlock();

			return R;
		}
		int VMModule::GetPropertyIndexByName(const char* Name) const
		{
			if (!IsValid())
				return -1;

			return Mod->GetGlobalVarIndexByName(Name);
		}
		int VMModule::GetPropertyIndexByDecl(const char* Decl) const
		{
			if (!IsValid())
				return -1;

			return Mod->GetGlobalVarIndexByDecl(Decl);
		}
		int VMModule::GetProperty(size_t Index, VMProperty* Info) const
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
		size_t VMModule::GetAccessMask() const
		{
			if (!Mod)
				return 0;

			asDWORD AccessMask = Mod->SetAccessMask(0);
			Mod->SetAccessMask(AccessMask);
			return (size_t)AccessMask;
		}
		size_t VMModule::GetObjectsCount() const
		{
			if (!IsValid())
				return 0;

			return Mod->GetObjectTypeCount();
		}
		size_t VMModule::GetPropertiesCount() const
		{
			if (!IsValid())
				return 0;

			return Mod->GetGlobalVarCount();
		}
		size_t VMModule::GetEnumsCount() const
		{
			if (!IsValid())
				return 0;

			return Mod->GetEnumCount();
		}
		size_t VMModule::GetImportedFunctionCount() const
		{
			if (!IsValid())
				return 0;

			return Mod->GetImportedFunctionCount();
		}
		VMTypeInfo VMModule::GetObjectByIndex(size_t Index) const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetObjectTypeByIndex(Index);
		}
		VMTypeInfo VMModule::GetTypeInfoByName(const char* Name) const
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
		VMTypeInfo VMModule::GetTypeInfoByDecl(const char* Decl) const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetTypeInfoByDecl(Decl);
		}
		VMTypeInfo VMModule::GetEnumByIndex(size_t Index) const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetEnumByIndex(Index);
		}
		const char* VMModule::GetPropertyDecl(size_t Index, bool IncludeNamespace) const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetGlobalVarDeclaration(Index, IncludeNamespace);
		}
		const char* VMModule::GetDefaultNamespace() const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetDefaultNamespace();
		}
		const char* VMModule::GetImportedFunctionDecl(size_t ImportIndex) const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetImportedFunctionDeclaration(ImportIndex);
		}
		const char* VMModule::GetImportedFunctionModule(size_t ImportIndex) const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetImportedFunctionSourceModule(ImportIndex);
		}
		const char* VMModule::GetName() const
		{
			if (!IsValid())
				return nullptr;

			return Mod->GetName();
		}
		bool VMModule::IsValid() const
		{
			return Manager != nullptr && Mod != nullptr;
		}
		VMCModule* VMModule::GetModule() const
		{
			return Mod;
		}
		VMManager* VMModule::GetManager() const
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
		VMInterface VMGlobal::SetInterface(const char* Name)
		{
			if (!Manager || !Name)
				return VMInterface(Manager, "", -1);

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return VMInterface(Manager, Name, -1);

			return VMInterface(Manager, Name, Engine->RegisterInterface(Name));
		}
		VMTypeClass VMGlobal::SetStructAddress(const char* Name, size_t Size, uint64_t Flags)
		{
			if (!Manager || !Name)
				return VMTypeClass(Manager, "", -1);

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return VMTypeClass(Manager, Name, -1);

			return VMTypeClass(Manager, Name, Engine->RegisterObjectType(Name, Size, (asDWORD)Flags));
		}
		VMTypeClass VMGlobal::SetPodAddress(const char* Name, size_t Size, uint64_t Flags)
		{
			if (!Manager || !Name)
				return VMTypeClass(Manager, "", -1);

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return VMTypeClass(Manager, Name, -1);

			return VMTypeClass(Manager, Name, Engine->RegisterObjectType(Name, Size, (asDWORD)Flags));
		}
		VMRefClass VMGlobal::SetClassAddress(const char* Name, uint64_t Flags)
		{
			if (!Manager || !Name)
				return VMRefClass(Manager, "", -1);

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return VMRefClass(Manager, Name, -1);

			return VMRefClass(Manager, Name, Engine->RegisterObjectType(Name, 0, (asDWORD)Flags));
		}
		VMEnum VMGlobal::SetEnum(const char* Name)
		{
			if (!Manager || !Name)
				return VMEnum(Manager, "", -1);

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return VMEnum(Manager, Name, -1);

			return VMEnum(Manager, Name, Engine->RegisterEnum(Name));
		}
		size_t VMGlobal::GetFunctionsCount() const
		{
			if (!Manager)
				return 0;

			return Manager->GetEngine()->GetGlobalFunctionCount();
		}
		VMFunction VMGlobal::GetFunctionById(int Id) const
		{
			if (!Manager)
				return nullptr;

			return Manager->GetEngine()->GetFunctionById(Id);
		}
		VMFunction VMGlobal::GetFunctionByIndex(int Index) const
		{
			if (!Manager)
				return nullptr;

			return Manager->GetEngine()->GetGlobalFunctionByIndex(Index);
		}
		VMFunction VMGlobal::GetFunctionByDecl(const char* Decl) const
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

			const char* Name = nullptr, *NameSpace = nullptr;
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
		VMTypeInfo VMGlobal::GetObjectByTypeId(int TypeId) const
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
		VMTypeInfo VMGlobal::GetEnumByTypeId(int TypeId) const
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
		VMTypeInfo VMGlobal::GetFunctionDefByIndex(int Index) const
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

			return Core::Form("%s(%d)", Object, Name ? Name : "any").R();
		}
		VMTypeInfo VMGlobal::GetTypeInfoById(int TypeId) const
		{
			if (!Manager)
				return nullptr;

			return Manager->GetEngine()->GetTypeInfoById(TypeId);
		}
		VMTypeInfo VMGlobal::GetTypeInfoByName(const char* Name) const
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
		VMTypeInfo VMGlobal::GetTypeInfoByDecl(const char* Decl) const
		{
			if (!Manager || !Decl)
				return nullptr;

			return Manager->GetEngine()->GetTypeInfoByDecl(Decl);
		}
		VMManager* VMGlobal::GetManager() const
		{
			return Manager;
		}

		VMCompiler::VMCompiler(VMManager* Engine) : Manager(Engine), Context(nullptr), Module(nullptr), BuiltOK(false)
		{
			Processor = new Compute::Preprocessor();
			Processor->SetIncludeCallback([this](Compute::Preprocessor* C, const Compute::IncludeResult& File, std::string* Out)
			{
				if (Include && Include(C, File, Out))
					return true;

				if (File.Module.empty() || !Module)
					return false;

				if (!File.IsFile && File.IsSystem)
					return Manager->ImportSubmodule(File.Module);

				std::string Buffer;
				if (!Manager->ImportFile(File.Module, &Buffer))
					return false;

				if (!C->Process(File.Module, Buffer))
					return false;

				return Module->AddScriptSection(File.Module.c_str(), Buffer.c_str(), Buffer.size()) >= 0;
			});
			Processor->SetPragmaCallback([this](Compute::Preprocessor* C, const std::string& Name, const std::vector<std::string>& Args)
			{
				if (!Manager)
					return false;

				if (Pragma && Pragma(C, Name, Args))
					return true;

				if (Name == "compile" && Args.size() == 2)
				{
					const std::string& Key = Args[0];
					Core::Parser Value(&Args[1]);

					size_t Result = Value.HasInteger() ? Value.ToUInt64() : 0;
					if (Key == "ALLOW_UNSAFE_REFERENCES")
						Manager->SetProperty(VMProp_ALLOW_UNSAFE_REFERENCES, Result);
					else if (Key == "OPTIMIZE_BYTECODE")
						Manager->SetProperty(VMProp_OPTIMIZE_BYTECODE, Result);
					else if (Key == "COPY_SCRIPT_SECTIONS")
						Manager->SetProperty(VMProp_COPY_SCRIPT_SECTIONS, Result);
					else if (Key == "MAX_STACK_SIZE")
						Manager->SetProperty(VMProp_MAX_STACK_SIZE, Result);
					else if (Key == "USE_CHARACTER_LITERALS")
						Manager->SetProperty(VMProp_USE_CHARACTER_LITERALS, Result);
					else if (Key == "ALLOW_MULTILINE_STRINGS")
						Manager->SetProperty(VMProp_ALLOW_MULTILINE_STRINGS, Result);
					else if (Key == "ALLOW_IMPLICIT_HANDLE_TYPES")
						Manager->SetProperty(VMProp_ALLOW_IMPLICIT_HANDLE_TYPES, Result);
					else if (Key == "BUILD_WITHOUT_LINE_CUES")
						Manager->SetProperty(VMProp_BUILD_WITHOUT_LINE_CUES, Result);
					else if (Key == "INIT_GLOBAL_VARS_AFTER_BUILD")
						Manager->SetProperty(VMProp_INIT_GLOBAL_VARS_AFTER_BUILD, Result);
					else if (Key == "REQUIRE_ENUM_SCOPE")
						Manager->SetProperty(VMProp_REQUIRE_ENUM_SCOPE, Result);
					else if (Key == "SCRIPT_SCANNER")
						Manager->SetProperty(VMProp_SCRIPT_SCANNER, Result);
					else if (Key == "INCLUDE_JIT_INSTRUCTIONS")
						Manager->SetProperty(VMProp_INCLUDE_JIT_INSTRUCTIONS, Result);
					else if (Key == "STRING_ENCODING")
						Manager->SetProperty(VMProp_STRING_ENCODING, Result);
					else if (Key == "PROPERTY_ACCESSOR_MODE")
						Manager->SetProperty(VMProp_PROPERTY_ACCESSOR_MODE, Result);
					else if (Key == "EXPAND_DEF_ARRAY_TO_TMPL")
						Manager->SetProperty(VMProp_EXPAND_DEF_ARRAY_TO_TMPL, Result);
					else if (Key == "AUTO_GARBAGE_COLLECT")
						Manager->SetProperty(VMProp_AUTO_GARBAGE_COLLECT, Result);
					else if (Key == "DISALLOW_GLOBAL_VARS")
						Manager->SetProperty(VMProp_ALWAYS_IMPL_DEFAULT_CONSTRUCT, Result);
					else if (Key == "ALWAYS_IMPL_DEFAULT_CONSTRUCT")
						Manager->SetProperty(VMProp_ALWAYS_IMPL_DEFAULT_CONSTRUCT, Result);
					else if (Key == "COMPILER_WARNINGS")
						Manager->SetProperty(VMProp_COMPILER_WARNINGS, Result);
					else if (Key == "DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE")
						Manager->SetProperty(VMProp_DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE, Result);
					else if (Key == "ALTER_SYNTAX_NAMED_ARGS")
						Manager->SetProperty(VMProp_ALTER_SYNTAX_NAMED_ARGS, Result);
					else if (Key == "DISABLE_INTEGER_DIVISION")
						Manager->SetProperty(VMProp_DISABLE_INTEGER_DIVISION, Result);
					else if (Key == "DISALLOW_EMPTY_LIST_ELEMENTS")
						Manager->SetProperty(VMProp_DISALLOW_EMPTY_LIST_ELEMENTS, Result);
					else if (Key == "PRIVATE_PROP_AS_PROTECTED")
						Manager->SetProperty(VMProp_PRIVATE_PROP_AS_PROTECTED, Result);
					else if (Key == "ALLOW_UNICODE_IDENTIFIERS")
						Manager->SetProperty(VMProp_ALLOW_UNICODE_IDENTIFIERS, Result);
					else if (Key == "HEREDOC_TRIM_MODE")
						Manager->SetProperty(VMProp_HEREDOC_TRIM_MODE, Result);
					else if (Key == "MAX_NESTED_CALLS")
						Manager->SetProperty(VMProp_MAX_NESTED_CALLS, Result);
					else if (Key == "GENERIC_CALL_MODE")
						Manager->SetProperty(VMProp_GENERIC_CALL_MODE, Result);
					else if (Key == "INIT_STACK_SIZE")
						Manager->SetProperty(VMProp_INIT_STACK_SIZE, Result);
					else if (Key == "INIT_CALL_STACK_SIZE")
						Manager->SetProperty(VMProp_INIT_CALL_STACK_SIZE, Result);
					else if (Key == "MAX_CALL_STACK_SIZE")
						Manager->SetProperty(VMProp_MAX_CALL_STACK_SIZE, Result);
				}
				else if (Name == "comment" && Args.size() == 2)
				{
					const std::string& Key = Args[0];
					if (Key == "INFO")
						TH_INFO("%s", Args[1].c_str());
					else if (Key == "WARN")
						TH_WARN("%s", Args[1].c_str());
					else if (Key == "ERROR")
						TH_ERROR("%s", Args[1].c_str());
				}
				else if (Name == "modify" && Args.size() == 2)
				{
					const std::string& Key = Args[0];
					Core::Parser Value(&Args[1]);

					size_t Result = Value.HasInteger() ? Value.ToUInt64() : 0;
					if (Key == "NAME")
						Module->SetName(Value.Get());
					else if (Key == "NAMESPACE")
						Module->SetDefaultNamespace(Value.Get());
					else if (Key == "ACCESS_MASK")
						Module->SetAccessMask(Result);
				}
				else if (Name == "cimport" && Args.size() >= 2)
				{
					std::vector<std::string> Sources;
					if (Args.size() > 2)
						Sources.assign(Args.begin() + 2, Args.end());

					Manager->ImportSymbol(Sources, Args[1], Args[0]);
				}
				else if (Name == "clibrary" && Args.size() == 1)
				{
					std::string Path = Args[0];
					if (!Path.empty() && Path.front() == '.')
					{
						std::string Subpath = Core::OS::Path::Resolve(Path, Core::OS::Path::GetDirectory(Processor->GetCurrentFilePath().c_str()));
						if (!Subpath.empty())
							Path = Subpath;
					}

					Manager->ImportLibrary(Path);
				}
				else if (Name == "define" && Args.size() == 1)
					Define(Args[0]);

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
			TH_RELEASE(Context);
			TH_RELEASE(Processor);
		}
		void VMCompiler::SetIncludeCallback(const Compute::ProcIncludeCallback& Callback)
		{
			Include = Callback;
		}
		void VMCompiler::SetPragmaCallback(const Compute::ProcPragmaCallback& Callback)
		{
			Pragma = Callback;
		}
		void VMCompiler::Define(const std::string& Word)
		{
			Processor->Define(Word);
		}
		void VMCompiler::Undefine(const std::string& Word)
		{
			Processor->Undefine(Word);
		}
		bool VMCompiler::Clear()
		{
			if (Context != nullptr && Context->IsPending())
				return false;

			Manager->Lock();
			{
				if (Module != nullptr)
					Module->Discard();

				Module = nullptr;
				BuiltOK = false;
			}
			Manager->Unlock();

			Processor->Clear();
			return true;
		}
		bool VMCompiler::IsPending()
		{
			return Context && Context->IsPending();
		}
		bool VMCompiler::IsDefined(const std::string& Word)
		{
			return Processor->IsDefined(Word.c_str());
		}
		bool VMCompiler::IsBuilt()
		{
			return BuiltOK;
		}
		bool VMCompiler::IsCached()
		{
			return VCache.Valid;
		}
		int VMCompiler::Prepare(const std::string& ModuleName, bool Scoped)
		{
			if (!Manager || ModuleName.empty())
				return -1;

			BuiltOK = false;
			VCache.Valid = false;
			VCache.Name.clear();

			Manager->Lock();
			if (Module != nullptr)
				Module->Discard();

			if (Scoped)
				Module = Manager->CreateScopedModule(ModuleName.c_str(), false);
			else
				Module = Manager->CreateModule(ModuleName.c_str(), false);
			Manager->Unlock();

			if (!Module)
				return -1;

			Module->SetUserData(this, CompilerUD);
			Manager->SetProcessorOptions(Processor);
			return 0;
		}
		int VMCompiler::Prepare(const std::string& ModuleName, const std::string& Name, bool Debug, bool Scoped)
		{
			if (!Manager)
				return -1;

			int Result = Prepare(ModuleName, Scoped);
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
		int VMCompiler::Compile(bool Await)
		{
			if (!Manager || !Module)
				return -1;

			if (VCache.Valid)
			{
				int R = LoadByteCode(&VCache);
				while (Await && R == asBUILD_IN_PROGRESS)
				{
					std::this_thread::sleep_for(std::chrono::microseconds(100));
					R = LoadByteCode(&VCache);
				}

				BuiltOK = (R >= 0);
				if (BuiltOK)
					Module->ResetGlobalVars(Context->GetContext());

				return R;
			}

			Manager->Lock();
			int R = Module->Build();
			Manager->Unlock();

			while (Await && R == asBUILD_IN_PROGRESS)
			{
				std::this_thread::sleep_for(std::chrono::microseconds(100));

				Manager->Lock();
				R = Module->Build();
				Manager->Unlock();
			}

			BuiltOK = (R >= 0);
			if (!BuiltOK)
				return R;

			if (VCache.Name.empty())
			{
				Module->ResetGlobalVars(Context->GetContext());
				return R;
			}

			R = SaveByteCode(&VCache);
			if (R < 0)
			{
				Module->ResetGlobalVars(Context->GetContext());
				return R;
			}

			Module->ResetGlobalVars(Context->GetContext());
			Manager->SetByteCodeCache(&VCache);
			return 0;
		}
		int VMCompiler::SaveByteCode(VMByteCode* Info)
		{
			Manager->Lock();
			if (!Info || !Module || !BuiltOK)
			{
				Manager->Unlock();
				TH_ERROR("module was not built");
				return -1;
			}

			CByteCodeStream* Stream = new CByteCodeStream();
			int R = Module->SaveByteCode(Stream, Info->Debug);
			Info->Data = Stream->GetCode();
			delete Stream;

			Manager->Unlock();
			return R;
		}
		int VMCompiler::LoadByteCode(VMByteCode* Info)
		{
			Manager->Lock();
			if (!Info || !Module)
			{
				Manager->Unlock();
				return -1;
			}

			CByteCodeStream* Stream = new CByteCodeStream(Info->Data);
			int R = Module->LoadByteCode(Stream, &Info->Debug);
			delete Stream;

			Manager->Unlock();
			return R;
		}
		int VMCompiler::LoadFile(const std::string& Path)
		{
			if (!Module || !Manager)
			{
				TH_ERROR("module was not created");
				return -1;
			}

			if (VCache.Valid)
				return 0;

			std::string Source = Core::OS::Path::Resolve(Path.c_str());
			if (!Core::OS::File::IsExists(Source.c_str()))
			{
				TH_ERROR("file not found");
				return -1;
			}

			std::string Buffer = Core::OS::File::ReadAsString(Source.c_str());
			if (!Processor->Process(Source, Buffer))
				return asINVALID_DECLARATION;

			return Module->AddScriptSection(Source.c_str(), Buffer.c_str(), Buffer.size());
		}
		int VMCompiler::LoadCode(const std::string& Name, const std::string& Data)
		{
			if (!Module)
			{
				TH_ERROR("module was not created");
				return -1;
			}

			if (VCache.Valid)
				return 0;

			std::string Buffer(Data);
			if (!Processor->Process("", Buffer))
				return asINVALID_DECLARATION;

			return Module->AddScriptSection(Name.c_str(), Buffer.c_str(), Buffer.size());
		}
		int VMCompiler::LoadCode(const std::string& Name, const char* Data, uint64_t Size)
		{
			if (!Module)
			{
				TH_ERROR("module was not created");
				return -1;
			}

			if (VCache.Valid)
				return 0;

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
			return ExecuteEntry(Name, Return, ReturnTypeId, nullptr);
		}
		int VMCompiler::ExecuteEntry(const char* Name, void* Return, int ReturnTypeId, ArgsCallback&& Callback)
		{
			if (!BuiltOK || !Manager || !Context || !Name || !Module)
				return asINVALID_ARG;

			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return asINVALID_CONFIGURATION;

			VMCFunction* Function = Module->GetFunctionByName(Name);
			if (!Function)
				return asNO_FUNCTION;

			Core::Async<int> Result = Context->Execute(Function, false, std::move(Callback));
			if (Result.IsPending())
				return asEXECUTION_ACTIVE;

			if (Return == 0 || ReturnTypeId == asTYPEID_VOID)
				return Result.Get();

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
				
			return Result.Get();
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
			Manager->Lock();
			int R = Module->CompileFunction("__vfbdy", Eval.c_str(), -1, asCOMP_ADD_TO_MODULE, &Function);
			Manager->Unlock();

			if (R < 0)
				return R;

			Core::Async<int> Result = Context->Execute(Function, false, nullptr);
			if (Result.IsPending())
			{
				Result.Await([Function](int) { Function->Release(); });
				return asEXECUTION_ACTIVE;
			}

			if (Return == 0 || ReturnTypeId == asTYPEID_VOID)
			{
				Function->Release();
				return Result.Get();
			}

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

			Function->Release();
			return Result.Get();
		}
		VMManager* VMCompiler::GetManager() const
		{
			return Manager;
		}
		VMContext* VMCompiler::GetContext() const
		{
			return Context;
		}
		VMModule VMCompiler::GetModule() const
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

		VMContext::VMContext(VMCContext* Base) : Manager(nullptr), Context(Base), Async(0)
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
			if (IsPending())
				TH_ERROR("[asyncerr] %llu operations are still pending", Async.load());

			if (Context != nullptr)
			{
				if (Manager != nullptr)
					Manager->GetEngine()->ReturnContext(Context);
				else
					Context->Release();
			}
			asThreadCleanup();
		}
		Core::Async<int> VMContext::Execute(const VMFunction& Function, bool Nested, ArgsCallback&& Callback)
		{
			asEContextState State = Context->GetState();
			if (State != asEXECUTION_ACTIVE)
				return Core::Async<int>::Store(ExecuteDeferred(Function, Nested, std::move(Callback)));

			Core::Async<int> Result;
			Safe.lock();
			Events.push({ Function, std::move(Callback), Result });
			Safe.unlock();

			return Result;
		}
		int VMContext::ExecuteDeferred(const VMFunction& Function, bool Nested, ArgsCallback&& Callback)
		{
			if (Nested)
				Context->PushState();

			int Result = Context->Prepare(Function.GetFunction());
			if (Result < 0)
			{
				if (Nested)
					Context->PopState();

				Safe.unlock();
				return Result;
			}

			if (Callback)
				Callback(this);

			Result = Resume(true);
			if (Nested)
				Context->PopState();

			return Result;
		}
		int VMContext::SetResumeCallback(ResumeCallback&& Callback)
		{
			Safe.lock();
			Resolve = std::move(Callback);
			Safe.unlock();

			return 0;
		}
		int VMContext::SetExceptionCallback(void(*Callback)(VMCContext* Context, void* Object), void* Object)
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
		int VMContext::Prepare(const VMFunction& Function)
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

			int Result = Context->Execute();
			if (Result != asEXECUTION_FINISHED || Events.empty())
				return Result;

			Safe.lock();
			if (Events.empty())
			{
				Safe.unlock();
				return Result;
			}

			Event Next = Events.front();
			Events.pop();
			Safe.unlock();

			Result = ExecuteDeferred(Next.Function, false, std::move(Next.Callback));
			Next.Result.Set(Result);

			return Result;
		}
		int VMContext::Resume(bool Forced)
		{
			asEContextState State = Context->GetState();
			if (State == asEXECUTION_ACTIVE)
				return asEXECUTION_ABORTED;

			if (!Resolve)
			{
				if (!Forced && State == asEXECUTION_FINISHED)
					return asEXECUTION_ABORTED;

				return Context->Execute();
			}

			if (!Forced && State == asEXECUTION_FINISHED)
			{
				Resolve(true);
				return asEXECUTION_ABORTED;
			}

			State = (asEContextState)Context->Execute();
			Resolve(State == asEXECUTION_FINISHED || State == asEXECUTION_ABORTED || State == asEXECUTION_EXCEPTION || State == asEXECUTION_ERROR);

			return State;
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

			uint64_t Id = VMCThread::GetThreadId();
			if (Id > 0)
				Result << "\t{...thread-" << Id << "...}\n";
			else
				Result << "\t{...thread-main...}\n";

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
		int VMContext::PushCoroutine()
		{
			Async++;
			return 0;
		}
		int VMContext::PopCoroutine()
		{
			if (!Async)
				return VMResult_INVALID_ARG;

			Async--;
			return 0;
		}
		bool VMContext::IsPending()
		{
			return Async > 0;
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
		VMFunction VMContext::GetExceptionFunction()
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
		int VMContext::SetLineCallback(void(*Callback)(VMCContext* Context, void* Object), void* Object)
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
		VMFunction VMContext::GetFunction(unsigned int StackLevel)
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
		VMFunction VMContext::GetSystemFunction()
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
				return TH_WARN("exception raised");

			const char* Decl = Function->GetDeclaration();
			const char* Mod = Function->GetModuleName();
			const char* Source = Function->GetScriptSectionName();
			int Line = Context->GetExceptionLineNumber();
			std::string Stack = Api ? Api->GetStackTrace() : "";

			TH_ERROR("uncaught exception raised"
				"\n\tdescription: %s"
				"\n\tfunction: %s"
				"\n\tmodule: %s"
				"\n\tsource: %s"
				"\n\tline: %i\n%.*s", Message ? Message : "undefined", Decl ? Decl : "undefined", Mod ? Mod : "undefined", Source ? Source : "undefined", Line, (int)Stack.size(), Stack.c_str());
		}
		int VMContext::ContextUD = 152;

		VMManager::VMManager() : Engine(asCreateScriptEngine()), Globals(this), Cached(true), Scope(0), JIT(nullptr), Nullable(0), Imports(VMImport_All)
		{
			Include.Exts.push_back(".as");
			Include.Root = Core::OS::Directory::Get();

			Engine->SetUserData(this, ManagerUD);
			Engine->SetContextCallbacks(RequestContext, ReturnContext, nullptr);
			Engine->SetMessageCallback(asFUNCTION(CompileLogger), this, asCALL_CDECL);
			Engine->SetEngineProperty(asEP_INIT_GLOBAL_VARS_AFTER_BUILD, false);
			Engine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, true);
			Engine->SetEngineProperty(asEP_DISALLOW_EMPTY_LIST_ELEMENTS, true);
			Engine->SetEngineProperty(asEP_COMPILER_WARNINGS, 1);

			if (Engine->RegisterObjectType("Nullable", 0, asOBJ_REF | asOBJ_NOCOUNT) >= 0)
			{
				Engine->RegisterGlobalFunction("Nullable@ get_nullptr() property", asFUNCTION(VMManager::GetNullable), asCALL_CDECL);
				Nullable = Engine->GetTypeIdByDecl("Nullable@");
			}

			if (Engine->RegisterObjectType("Address", 0, asOBJ_REF | asOBJ_NOCOUNT) < 0)
				TH_ERROR("could not register Address type for script engine");

			RegisterSubmodules(this);
		}
		VMManager::~VMManager()
		{
			for (auto& Core : Kernels)
				Core::OS::Symbol::Unload(Core.second.Handle);

			for (auto& Context : Contexts)
				Context->Release();

			if (Engine != nullptr)
				Engine->ShutDownAndRelease();
#ifdef HAS_AS_JIT
			delete (VMCJITCompiler*)JIT;
#endif
			ClearCache();
		}
		void VMManager::SetImports(unsigned int Opts)
		{
			Imports = Opts;
		}
		void VMManager::SetJIT(unsigned int JITOpts)
		{
#ifdef HAS_AS_JIT
			if (!JIT)
				Engine->SetEngineProperty(asEP_INCLUDE_JIT_INSTRUCTIONS, 1);
			else
				delete (VMCJITCompiler*)JIT;

			JIT = new VMCJITCompiler(JITOpts);
			Engine->SetJITCompiler((VMCJITCompiler*)JIT);
#else
			TH_ERROR("JIT compiler is not supported on this platform");
#endif
		}
		void VMManager::SetCache(bool Enabled)
		{
			Cached = Enabled;
		}
		void VMManager::ClearCache()
		{
			Safe.lock();
			for (auto Data : Datas)
				delete Data.second;

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
		int VMManager::SetLogCallback(void(*Callback)(const asSMessageInfo* Message, void* Object), void* Object)
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
		VMCModule* VMManager::CreateScopedModule(const std::string& Name, bool AqLock)
		{
			if (!Engine || Name.empty())
				return nullptr;

			if (AqLock)
				Safe.lock();

			if (!Engine->GetModule(Name.c_str()))
			{
				if (AqLock)
					Safe.unlock();

				return Engine->GetModule(Name.c_str(), asGM_ALWAYS_CREATE);
			}


			std::string Result;
			while (true)
			{
				Result = Name + std::to_string(Scope++);
				if (!Engine->GetModule(Result.c_str()))
				{
					if (AqLock)
						Safe.unlock();

					return Engine->GetModule(Result.c_str(), asGM_ALWAYS_CREATE);
				}
			}

			if (AqLock)
				Safe.unlock();

			return nullptr;
		}
		VMCModule* VMManager::CreateModule(const std::string& Name, bool AqLock)
		{
			if (!Engine || Name.empty())
				return nullptr;

			if (AqLock)
				Safe.lock();

			VMCModule* Result = Engine->GetModule(Name.c_str(), asGM_ALWAYS_CREATE);
			if (AqLock)
				Safe.unlock();

			return Result;
		}
		void* VMManager::CreateObject(const VMTypeInfo& Type)
		{
			return Engine->CreateScriptObject(Type.GetTypeInfo());
		}
		void* VMManager::CreateObjectCopy(void* Object, const VMTypeInfo& Type)
		{
			return Engine->CreateScriptObjectCopy(Object, Type.GetTypeInfo());
		}
		void* VMManager::CreateEmptyObject(const VMTypeInfo& Type)
		{
			return Engine->CreateUninitializedScriptObject(Type.GetTypeInfo());
		}
		VMFunction VMManager::CreateDelegate(const VMFunction& Function, void* Object)
		{
			return Engine->CreateDelegate(Function.GetFunction(), Object);
		}
		int VMManager::AssignObject(void* DestObject, void* SrcObject, const VMTypeInfo& Type)
		{
			return Engine->AssignScriptObject(DestObject, SrcObject, Type.GetTypeInfo());
		}
		void VMManager::ReleaseObject(void* Object, const VMTypeInfo& Type)
		{
			return Engine->ReleaseScriptObject(Object, Type.GetTypeInfo());
		}
		void VMManager::AddRefObject(void* Object, const VMTypeInfo& Type)
		{
			return Engine->AddRefScriptObject(Object, Type.GetTypeInfo());
		}
		int VMManager::RefCastObject(void* Object, const VMTypeInfo& FromType, const VMTypeInfo& ToType, void** NewPtr, bool UseOnlyImplicitCast)
		{
			return Engine->RefCastObject(Object, FromType.GetTypeInfo(), ToType.GetTypeInfo(), NewPtr, UseOnlyImplicitCast);
		}
		int VMManager::Collect(size_t NumIterations)
		{
			Safe.lock();
			int R = Engine->GarbageCollect(asGC_FULL_CYCLE, NumIterations);
			Safe.unlock();

			return R;
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
		int VMManager::NotifyOfNewObject(void* Object, const VMTypeInfo& Type)
		{
			return Engine->NotifyGarbageCollectorOfNewObject(Object, Type.GetTypeInfo());
		}
		int VMManager::GetObjectAddress(size_t Index, size_t* SequenceNumber, void** Object, VMTypeInfo* Type)
		{
			asUINT asSequenceNumber;
			VMCTypeInfo* OutType = nullptr;
			int Result = Engine->GetObjectInGC(Index, &asSequenceNumber, Object, &OutType);

			if (SequenceNumber != nullptr)
				*SequenceNumber = (size_t)asSequenceNumber;

			if (Type != nullptr)
				*Type = VMTypeInfo(OutType);

			return Result;
		}
		void VMManager::ForwardEnumReferences(void* Reference, const VMTypeInfo& Type)
		{
			return Engine->ForwardGCEnumReferences(Reference, Type.GetTypeInfo());
		}
		void VMManager::ForwardReleaseReferences(void* Reference, const VMTypeInfo& Type)
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

			const char* Prev = Engine->GetDefaultNamespace();
			Safe.lock();
			if (Prev != nullptr)
				DefaultNamespace = Prev;
			else
				DefaultNamespace.clear();
			Safe.unlock();

			return Engine->SetDefaultNamespace(Namespace);
		}
		int VMManager::BeginNamespaceIsolated(const char* Namespace, size_t DefaultMask)
		{
			if (!Namespace)
				return -1;

			BeginAccessMask(DefaultMask);
			return BeginNamespace(Namespace);
		}
		int VMManager::EndNamespaceIsolated()
		{
			EndAccessMask();
			return EndNamespace();
		}
		int VMManager::EndNamespace()
		{
			Safe.lock();
			int R = Engine->SetDefaultNamespace(DefaultNamespace.c_str());
			Safe.unlock();

			return R;
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
		VMModule VMManager::Module(const char* Name)
		{
			if (!Engine || !Name)
				return VMModule(nullptr);

			return VMModule(Engine->GetModule(Name, asGM_CREATE_IF_NOT_EXISTS));
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

			if (!Core::Parser(&Include.Root).EndsOf("/\\"))
			{
#ifdef TH_MICROSOFT
				Include.Root.append(1, '\\');
#else
				Include.Root.append(1, '/');
#endif
			}
			Safe.unlock();
		}
		std::string VMManager::GetDocumentRoot() const
		{
			return Include.Root;
		}
		std::vector<std::string> VMManager::GetSubmodules() const
		{
			std::vector<std::string> Result;
			for (auto& Module : Modules)
			{
				if (Module.second.Registered)
					Result.push_back(Module.first);
			}

			return Result;
		}
		std::vector<std::string> VMManager::VerifyModules(const std::string& Directory, const Compute::RegExp& Exp)
		{
			std::vector<std::string> Result;
			if (!Core::OS::Directory::IsExists(Directory.c_str()))
				return Result;

			std::vector<Core::ResourceEntry> Entries;
			if (!Core::OS::Directory::Scan(Directory, &Entries))
				return Result;

			for (auto& Entry : Entries)
			{
				if (Directory.back() != '/' && Directory.back() != '\\')
					Entry.Path = Directory + '/' + Entry.Path;
				else
					Entry.Path = Directory + Entry.Path;

				if (!Entry.Source.IsDirectory)
				{
					if (!Compute::Regex::Match((Compute::RegExp*)&Exp, nullptr, Entry.Path))
						continue;

					if (!VerifyModule(Entry.Path))
						Result.push_back(Entry.Path);
				}
				else
				{
					std::vector<std::string> Merge = VerifyModules(Entry.Path, Exp);
					if (!Merge.empty())
						Result.insert(Result.end(), Merge.begin(), Merge.end());
				}
			}

			return Result;
		}
		bool VMManager::VerifyModule(const std::string& Path)
		{
			if (!Engine)
				return false;

			std::string Source = Core::OS::File::ReadAsString(Path.c_str());
			if (Source.empty())
				return true;

			Safe.lock();
			asIScriptModule* Module = Engine->GetModule("__vfver", asGM_ALWAYS_CREATE);
			Module->AddScriptSection(Path.c_str(), Source.c_str(), Source.size());

			int R = Module->Build();
			while (R == asBUILD_IN_PROGRESS)
			{
				std::this_thread::sleep_for(std::chrono::microseconds(100));
				R = Module->Build();
			}

			Module->Discard();
			Safe.unlock();

			return R >= 0;
		}
		bool VMManager::IsNullable(int TypeId)
		{
			return TypeId == 0 || TypeId == Nullable;
		}
		bool VMManager::AddSubmodule(const std::string& Name, const std::vector<std::string>& Dependencies, const SubmoduleCallback& Callback)
		{
			Safe.lock();
			auto It = Modules.find(Name);
			if (It != Modules.end())
			{
				if (Callback || !Dependencies.empty())
				{
					It->second.Dependencies = Dependencies;
					It->second.Callback = Callback;
					It->second.Registered = false;
				}
				else
					Modules.erase(It);
			}
			else
			{
				Submodule Result;
				Result.Dependencies = Dependencies;
				Result.Callback = Callback;
				Result.Registered = false;
				Modules.insert({ Name, Result });
			}

			Safe.unlock();
			return true;
		}
		bool VMManager::ImportFile(const std::string& Path, std::string* Out)
		{
			if (!(Imports & VMImport_Files))
			{
				TH_ERROR("file import is not allowed");
				return false;
			}

			if (!Core::OS::File::IsExists(Path.c_str()))
				return false;

			if (!Cached)
			{
				if (Out != nullptr)
					Out->assign(Core::OS::File::ReadAsString(Path.c_str()));

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
			Result = Core::OS::File::ReadAsString(Path.c_str());
			if (Out != nullptr)
				Out->assign(Result);

			Safe.unlock();
			return true;
		}
		bool VMManager::ImportSymbol(const std::vector<std::string>& Sources, const std::string& Func, const std::string& Decl)
		{
			if (!(Imports & VMImport_CSymbols))
			{
				TH_ERROR("csymbols import is not allowed");
				return false;
			}

			if (!Engine || Decl.empty() || Func.empty())
				return false;

			Safe.lock();
			auto Core = Kernels.end();
			for (auto& Item : Sources)
			{
				Core = Kernels.find(Item);
				if (Core != Kernels.end())
					break;
			}

			if (Core == Kernels.end())
			{
				Safe.unlock();
				bool Failed = !ImportLibrary("");
				Safe.lock();

				if (Failed || (Core = Kernels.find("")) == Kernels.end())
				{
					TH_ERROR("cannot load find function in any of presented libraries:\n\t%s", Func.c_str());
					return false;
				}
			}

			auto Handle = Core->second.Functions.find(Func);
			if (Handle != Core->second.Functions.end())
			{
				Safe.unlock();
				return true;
			}

			VMObjectFunction Function = (VMObjectFunction)Core::OS::Symbol::LoadFunction(Core->second.Handle, Func.c_str());
			if (!Function)
			{
				TH_ERROR("cannot load shared object function: %s", Func.c_str());
				Safe.unlock();
				return false;
			}

			if (Engine->RegisterGlobalFunction(Decl.c_str(), asFUNCTION(Function), asCALL_CDECL) < 0)
			{
				TH_ERROR("cannot register shared object function: %s", Decl.c_str());
				Safe.unlock();
				return false;
			}

			Core->second.Functions.insert({ Func, (void*)Function });
			Safe.unlock();

			return true;
		}
		bool VMManager::ImportLibrary(const std::string& Path)
		{
			if (!(Imports & VMImport_CLibraries) && !Path.empty())
			{
				TH_ERROR("clibraries import is not allowed");
				return false;
			}

			std::string Name = GetLibraryName(Path);
			if (!Engine)
				return false;

			Safe.lock();
			auto Core = Kernels.find(Name);
			if (Core != Kernels.end())
			{
				Safe.unlock();
				return true;
			}
			Safe.unlock();

			void* Handle = Core::OS::Symbol::Load(Path);
			if (!Handle)
			{
				TH_ERROR("cannot load shared object: %s", Path.c_str());
				return false;
			}

			Kernel Library;
			Library.Handle = Handle;

			Safe.lock();
			Kernels.insert({ Name, Library });
			Safe.unlock();

			return true;
		}
		bool VMManager::ImportSubmodule(const std::string& Name)
		{
			if (!(Imports & VMImport_Submodules))
			{
				TH_ERROR("submodules import is not allowed");
				return false;
			}

			Safe.lock();
			auto It = Modules.find(Name);
			if (It == Modules.end())
			{
				Safe.unlock();
				TH_ERROR("couldn't find script submodule %s", Name.c_str());
				return false;
			}

			if (It->second.Registered)
			{
				Safe.unlock();
				return true;
			}

			Submodule Base = It->second;
			It->second.Registered = true;
			Safe.unlock();

			for (auto& Item : Base.Dependencies)
			{
				if (!ImportSubmodule(Item))
				{
					TH_ERROR("couldn't load submodule %s for %s", Item.c_str(), Name.c_str());
					return false;
				}
			}

			if (Base.Callback)
				Base.Callback(this);

			return true;
		}
		Core::Document* VMManager::ImportJSON(const std::string& Path)
		{
			if (!(Imports & VMImport_JSON))
			{
				TH_ERROR("json import is not allowed");
				return nullptr;
			}

			std::string File = Core::OS::Path::Resolve(Path, Include.Root);
			if (!Core::OS::File::IsExists(File.c_str()))
			{
				File = Core::OS::Path::Resolve(Path + ".json", Include.Root);
				if (!Core::OS::File::IsExists(File.c_str()))
					return nullptr;
			}

			if (!Cached)
			{
				std::string Data = Core::OS::File::ReadAsString(File.c_str());
				uint64_t Offset = 0;

				return Core::Document::ReadJSON(Data.size(), [&Data, &Offset](char* Buffer, int64_t Size)
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
				Core::Document* Result = It->second ? It->second->Copy() : nullptr;
				Safe.unlock();

				return Result;
			}

			Core::Document*& Result = Datas[File];
			std::string Data = Core::OS::File::ReadAsString(File.c_str());
			uint64_t Offset = 0;

			Result = Core::Document::ReadJSON(Data.size(), [&Data, &Offset](char* Buffer, int64_t Size)
			{
				if (!Buffer || !Size)
					return true;

				if (Offset + Size > Data.size())
					return false;

				memcpy(Buffer, Data.c_str() + Offset, Size);
				Offset += Size;

				return true;
			});

			Core::Document* Copy = nullptr;
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
			FreeCoreAPI();
			asThreadCleanup();
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
		std::string VMManager::GetLibraryName(const std::string& Path)
		{
			if (Path.empty())
				return Path;

			Core::Parser Src(Path);
			Core::Parser::Settle Start = Src.ReverseFindOf("\\/");
			if (Start.Found)
				Src.Substring(Start.End);

			Core::Parser::Settle End = Src.ReverseFind('.');
			if (End.Found)
				Src.Substring(0, End.End);

			return Src.R();
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
		void VMManager::SetMemoryFunctions(void*(*Alloc)(size_t), void(*Free)(void*))
		{
			asSetGlobalMemoryFunctions(Alloc, Free);
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
				TH_WARN("\n\t%s (%i, %i): %s", Info->section && Info->section[0] != '\0' ? Info->section : "any", Info->row, Info->col, Info->message);
			else if (Info->type == asMSGTYPE_INFORMATION)
				TH_INFO("\n\t%s (%i, %i): %s", Info->section && Info->section[0] != '\0' ? Info->section : "any", Info->row, Info->col, Info->message);
			else if (Info->type == asMSGTYPE_ERROR)
				TH_ERROR("\n\t%s (%i, %i): %s", Info->section && Info->section[0] != '\0' ? Info->section : "any", Info->row, Info->col, Info->message);
		}
		void VMManager::RegisterSubmodules(VMManager* Engine)
		{
			Engine->AddSubmodule("std/any", { }, RegisterAnyAPI);
			Engine->AddSubmodule("std/array", { }, RegisterArrayAPI);
			Engine->AddSubmodule("std/complex", { }, RegisterComplexAPI);
			Engine->AddSubmodule("std/grid", { }, RegisterGridAPI);
			Engine->AddSubmodule("std/ref", { }, RegisterRefAPI);
			Engine->AddSubmodule("std/weakref", { }, RegisterWeakRefAPI);
			Engine->AddSubmodule("std/math", { }, RegisterMathAPI);
			Engine->AddSubmodule("std/random", { }, RegisterRandomAPI);
			Engine->AddSubmodule("std/string", { "std/array" }, RegisterStringAPI);
			Engine->AddSubmodule("std/map", { "std/array", "std/string" }, RegisterMapAPI);
			Engine->AddSubmodule("std/exception", { "std/string" }, RegisterExceptionAPI);
			Engine->AddSubmodule("std/thread", { "std/any" }, RegisterThreadAPI);
			Engine->AddSubmodule("std/async", { "std/any" }, RegisterAsyncAPI);
			Engine->AddSubmodule("std",
			{
				"std/any",
				"std/array",
				"std/complex",
				"std/grid",
				"std/ref",
				"std/weakref",
				"std/math",
				"std/random",
				"std/string",
				"std/map",
				"std/exception",
				"std/thread",
				"std/async"
			}, nullptr);

			Engine->AddSubmodule("core/format", { "std/string" }, RegisterFormatAPI);
			Engine->AddSubmodule("core/console", { "core/format" }, RegisterConsoleAPI);
			Engine->AddSubmodule("core/variant", { }, RegisterVariantAPI);
			Engine->AddSubmodule("core/document", { "std/array", "std/string", "std/map", "core/variant" }, RegisterDocumentAPI);
			Engine->AddSubmodule("core",
			{
				"core/format",
				"core/console",
				"core/variant",
				"core/document"
			}, nullptr);

			Engine->AddSubmodule("gui/element", { }, RegisterGuiElementAPI);
			Engine->AddSubmodule("gui/document", { }, RegisterGuiDocumentAPI);
			Engine->AddSubmodule("gui/event", { }, RegisterGuiEventAPI);
			Engine->AddSubmodule("gui/context", { }, RegisterGuiContextAPI);
			Engine->AddSubmodule("gui",
			{
				"gui/element",
				"gui/document",
				"gui/event",
				"gui/context"
			}, nullptr);
		}
		size_t VMManager::GetDefaultAccessMask()
		{
			return 0xFFFFFFFF;
		}
		void* VMManager::GetNullable()
		{
			return nullptr;
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
		void VMDebugger::RegisterToStringCallback(const VMTypeInfo& Type, ToStringCallback Callback)
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
							const char* VarName = 0, *NameSpace = 0;
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

				VMCTypeInfo* Type = Base->GetTypeInfoById(TypeId);
				if (Type->GetFlags() & asOBJ_REF)
					Stream << "{" << Value << "}";

				if (Value != nullptr)
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

			return Stream.str();
		}
		VMManager* VMDebugger::GetEngine()
		{
			return Manager;
		}
	}
}