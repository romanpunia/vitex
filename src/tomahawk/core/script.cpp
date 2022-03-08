#include "script.h"
#include "../script/std-lib.h"
#include "../script/core-lib.h"
#include "../script/compute-lib.h"
#include "../script/gui-lib.h"
#include <inttypes.h>
#include <iostream>
#include <sstream>
#ifndef ANGELSCRIPT_H 
#include <angelscript.h>
#endif

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
			TH_ASSERT(Ptr && Size, 0, "corrupted read");
			memcpy(Ptr, &Code[ReadPos], Size);
			ReadPos += Size;

			return 0;
		}
		int Write(const void* Ptr, asUINT Size)
		{
			TH_ASSERT(Ptr && Size, 0, "corrupted write");
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

	struct DEnum
	{
		std::vector<std::string> Values;
	};

	struct DClass
	{
		std::vector<std::string> Props;
		std::vector<std::string> Interfaces;
		std::vector<std::string> Types;
		std::vector<std::string> Funcdefs;
		std::vector<std::string> Methods;
		std::vector<std::string> Functions;
	};

	struct DNamespace
	{
		std::unordered_map<std::string, DEnum> Enums;
		std::unordered_map<std::string, DClass> Classes;
		std::vector<std::string> Funcdefs;
		std::vector<std::string> Functions;
	};

	std::string GetCombination(const std::vector<std::string>& Names, const std::string& By)
	{
		std::string Result;
		for (size_t i = 0; i < Names.size(); i++)
		{
			Result.append(Names[i]);
			if (i + 1 < Names.size())
				Result.append(By);
		}

		return Result;
	}
	std::string GetCombinationAll(const std::vector<std::string>& Names, const std::string& By, const std::string& EndBy)
	{
		std::string Result;
		for (size_t i = 0; i < Names.size(); i++)
		{
			Result.append(Names[i]);
			if (i + 1 < Names.size())
				Result.append(By);
			else
				Result.append(EndBy);
		}

		return Result;
	}
	std::string GetTypeNaming(asITypeInfo* Type)
	{
		const char* Namespace = Type->GetNamespace();
		return (Namespace ? Namespace + std::string("::") : std::string("")) + Type->GetName();
	}
	asITypeInfo* GetTypeNamespacing(asIScriptEngine* Engine, const std::string& Name)
	{
		asITypeInfo* Result = Engine->GetTypeInfoByName(Name.c_str());
		if (Result != nullptr)
			return Result;

		return Engine->GetTypeInfoByName((Name + "@").c_str());
	}
	void DumpNamespace(Tomahawk::Core::FileStream* Stream, const std::string& Naming, DNamespace& Namespace, std::string& Offset)
	{
		if (!Naming.empty())
		{
			Offset.append("\t");
			Stream->WriteAny("namespace %s\n{\n", Naming.c_str());
		}

		for (auto It = Namespace.Enums.begin(); It != Namespace.Enums.end(); It++)
		{
			auto Copy = It;
			Stream->WriteAny("%senum %s\n%s{\n\t%s", Offset.c_str(), It->first.c_str(), Offset.c_str(), Offset.c_str());
			Stream->WriteAny("%s", GetCombination(It->second.Values, ",\n\t" + Offset).c_str());
			Stream->WriteAny("\n%s}\n%s", Offset.c_str(), ++Copy != Namespace.Enums.end() ? "\n" : "");
		}

		if (!Namespace.Enums.empty() && (!Namespace.Classes.empty() || !Namespace.Funcdefs.empty() || !Namespace.Functions.empty()))
			Stream->WriteAny("\n");

		for (auto It = Namespace.Classes.begin(); It != Namespace.Classes.end(); It++)
		{
			auto Copy = It;
			Stream->WriteAny("%sclass %s%s%s%s%s%s\n%s{\n\t%s",
				Offset.c_str(),
				It->first.c_str(),
				It->second.Types.empty() ? "" : "<",
				It->second.Types.empty() ? "" : GetCombination(It->second.Types, ", ").c_str(),
				It->second.Types.empty() ? "" : ">",
				It->second.Interfaces.empty() ? "" : " : ",
				It->second.Interfaces.empty() ? "" : GetCombination(It->second.Interfaces, ", ").c_str(),
				Offset.c_str(), Offset.c_str());
			Stream->WriteAny("%s", GetCombinationAll(It->second.Funcdefs, ";\n\t" + Offset, It->second.Props.empty() && It->second.Methods.empty() ? ";" : ";\n\n\t" + Offset).c_str());
			Stream->WriteAny("%s", GetCombinationAll(It->second.Props, ";\n\t" + Offset, It->second.Methods.empty() ? ";" : ";\n\n\t" + Offset).c_str());
			Stream->WriteAny("%s", GetCombinationAll(It->second.Methods, ";\n\t" + Offset, ";").c_str());
			Stream->WriteAny("\n%s}\n%s", Offset.c_str(), !It->second.Functions.empty() || ++Copy != Namespace.Classes.end() ? "\n" : "");

			if (It->second.Functions.empty())
				continue;

			Stream->WriteAny("%snamespace %s\n%s{\n\t%s", Offset.c_str(), It->first.c_str(), Offset.c_str(), Offset.c_str());
			Stream->WriteAny("%s", GetCombinationAll(It->second.Functions, ";\n\t" + Offset, ";").c_str());
			Stream->WriteAny("\n%s}\n%s", Offset.c_str(), ++Copy != Namespace.Classes.end() ? "\n" : "");
		}

		if (!Namespace.Funcdefs.empty())
		{
			if (!Namespace.Enums.empty() || !Namespace.Classes.empty())
				Stream->WriteAny("\n%s", Offset.c_str());
			else
				Stream->WriteAny("%s", Offset.c_str());
		}

		Stream->WriteAny("%s", GetCombinationAll(Namespace.Funcdefs, ";\n" + Offset, Namespace.Functions.empty() ? ";\n" : "\n\n" + Offset).c_str());
		if (!Namespace.Functions.empty() && Namespace.Funcdefs.empty())
		{
			if (!Namespace.Enums.empty() || !Namespace.Classes.empty())
				Stream->WriteAny("\n");
			else
				Stream->WriteAny("%s", Offset.c_str());
		}

		Stream->WriteAny("%s", GetCombinationAll(Namespace.Functions, ";\n" + Offset, ";\n").c_str());
		if (!Naming.empty())
		{
			Stream->WriteAny("}");
			Offset.erase(Offset.begin());
		}
	}
}

namespace Tomahawk
{
	namespace Script
	{
		int VMFuncStore::AtomicNotifyGC(const char* TypeName, void* Object)
		{
			TH_ASSERT(TypeName != nullptr, -1, "typename should be set");
			TH_ASSERT(Object != nullptr, -1, "object should be set");

			VMCContext* Context = asGetActiveContext();
			TH_ASSERT(Context != nullptr, -1, "context should be set");

			VMManager* Engine = VMManager::Get(Context->GetEngine());
			TH_ASSERT(Engine != nullptr, -1, "engine should be set");

			VMTypeInfo Type = Engine->Global().GetTypeInfoByName(TypeName);
			return Engine->NotifyOfNewObject(Object, Type.GetTypeInfo());
		}
		asSFuncPtr* VMFuncStore::CreateFunctionBase(void(*Base)(), int Type)
		{
			TH_ASSERT(Base != nullptr, nullptr, "function pointer should be set");
			asSFuncPtr* Ptr = TH_NEW(asSFuncPtr, Type);
			Ptr->ptr.f.func = reinterpret_cast<asFUNCTION_t>(Base);
			return Ptr;
		}
		asSFuncPtr* VMFuncStore::CreateMethodBase(const void* Base, size_t Size, int Type)
		{
			TH_ASSERT(Base != nullptr, nullptr, "function pointer should be set");
			asSFuncPtr* Ptr = TH_NEW(asSFuncPtr, Type);
			Ptr->CopyMethodPtr(Base, Size);
			return Ptr;
		}
		asSFuncPtr* VMFuncStore::CreateDummyBase()
		{
			return TH_NEW(asSFuncPtr, 0);
		}
		void VMFuncStore::ReleaseFunctor(asSFuncPtr** Ptr)
		{
			if (!Ptr || !*Ptr)
				return;

			TH_DELETE(asSFuncPtr, *Ptr);
			*Ptr = nullptr;
		}

		VMMessage::VMMessage(asSMessageInfo* Msg) : Info(Msg)
		{
		}
		const char* VMMessage::GetSection() const
		{
			TH_ASSERT(IsValid(), nullptr, "message should be valid");
			return Info->section;
		}
		const char* VMMessage::GetText() const
		{
			TH_ASSERT(IsValid(), nullptr, "message should be valid");
			return Info->message;
		}
		VMLogType VMMessage::GetType() const
		{
			TH_ASSERT(IsValid(), VMLogType::INFORMATION, "message should be valid");
			return (VMLogType)Info->type;
		}
		int VMMessage::GetRow() const
		{
			TH_ASSERT(IsValid(), -1, "message should be valid");
			return Info->row;
		}
		int VMMessage::GetColumn() const
		{
			TH_ASSERT(IsValid(), -1, "message should be valid");
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
			TH_ASSERT_V(IsValid(), "typeinfo should be valid");
			TH_ASSERT_V(Callback, "typeinfo should not be empty");

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
			TH_ASSERT_V(IsValid(), "typeinfo should be valid");
			TH_ASSERT_V(Callback, "typeinfo should not be empty");

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
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetConfigGroup();
		}
		size_t VMTypeInfo::GetAccessMask() const
		{
			TH_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetAccessMask();
		}
		VMModule VMTypeInfo::GetModule() const
		{
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetModule();
		}
		int VMTypeInfo::AddRef() const
		{
			TH_ASSERT(IsValid(), -1, "typeinfo should be valid");
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
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetName();
		}
		const char* VMTypeInfo::GetNamespace() const
		{
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetNamespace();
		}
		VMTypeInfo VMTypeInfo::GetBaseType() const
		{
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetBaseType();
		}
		bool VMTypeInfo::DerivesFrom(const VMTypeInfo& Type) const
		{
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->DerivesFrom(Type.GetTypeInfo());
		}
		size_t VMTypeInfo::GetFlags() const
		{
			TH_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetFlags();
		}
		unsigned int VMTypeInfo::GetSize() const
		{
			TH_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetSize();
		}
		int VMTypeInfo::GetTypeId() const
		{
			TH_ASSERT(IsValid(), -1, "typeinfo should be valid");
			return Info->GetTypeId();
		}
		int VMTypeInfo::GetSubTypeId(unsigned int SubTypeIndex) const
		{
			TH_ASSERT(IsValid(), -1, "typeinfo should be valid");
			return Info->GetSubTypeId(SubTypeIndex);
		}
		VMTypeInfo VMTypeInfo::GetSubType(unsigned int SubTypeIndex) const
		{
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetSubType(SubTypeIndex);
		}
		unsigned int VMTypeInfo::GetSubTypeCount() const
		{
			TH_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetSubTypeCount();
		}
		unsigned int VMTypeInfo::GetInterfaceCount() const
		{
			TH_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetInterfaceCount();
		}
		VMTypeInfo VMTypeInfo::GetInterface(unsigned int Index) const
		{
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetInterface(Index);
		}
		bool VMTypeInfo::Implements(const VMTypeInfo& Type) const
		{
			TH_ASSERT(IsValid(), false, "typeinfo should be valid");
			return Info->Implements(Type.GetTypeInfo());
		}
		unsigned int VMTypeInfo::GetFactoriesCount() const
		{
			TH_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetFactoryCount();
		}
		VMFunction VMTypeInfo::GetFactoryByIndex(unsigned int Index) const
		{
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetFactoryByIndex(Index);
		}
		VMFunction VMTypeInfo::GetFactoryByDecl(const char* Decl) const
		{
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetFactoryByDecl(Decl);
		}
		unsigned int VMTypeInfo::GetMethodsCount() const
		{
			TH_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetMethodCount();
		}
		VMFunction VMTypeInfo::GetMethodByIndex(unsigned int Index, bool GetVirtual) const
		{
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetMethodByIndex(Index, GetVirtual);
		}
		VMFunction VMTypeInfo::GetMethodByName(const char* Name, bool GetVirtual) const
		{
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetMethodByName(Name, GetVirtual);
		}
		VMFunction VMTypeInfo::GetMethodByDecl(const char* Decl, bool GetVirtual) const
		{
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetMethodByDecl(Decl, GetVirtual);
		}
		unsigned int VMTypeInfo::GetPropertiesCount() const
		{
			TH_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetPropertyCount();
		}
		int VMTypeInfo::GetProperty(unsigned int Index, VMFuncProperty* Out) const
		{
			TH_ASSERT(IsValid(), -1, "typeinfo should be valid");

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
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetPropertyDeclaration(Index, IncludeNamespace);
		}
		unsigned int VMTypeInfo::GetBehaviourCount() const
		{
			TH_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetBehaviourCount();
		}
		VMFunction VMTypeInfo::GetBehaviourByIndex(unsigned int Index, VMBehave* OutBehaviour) const
		{
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");

			asEBehaviours Out;
			VMCFunction* Result = Info->GetBehaviourByIndex(Index, &Out);
			if (OutBehaviour != nullptr)
				*OutBehaviour = (VMBehave)Out;

			return Result;
		}
		unsigned int VMTypeInfo::GetChildFunctionDefCount() const
		{
			TH_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetChildFuncdefCount();
		}
		VMTypeInfo VMTypeInfo::GetChildFunctionDef(unsigned int Index) const
		{
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetChildFuncdef(Index);
		}
		VMTypeInfo VMTypeInfo::GetParentType() const
		{
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetParentType();
		}
		unsigned int VMTypeInfo::GetEnumValueCount() const
		{
			TH_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetEnumValueCount();
		}
		const char* VMTypeInfo::GetEnumValueByIndex(unsigned int Index, int* OutValue) const
		{
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetEnumValueByIndex(Index, OutValue);
		}
		VMFunction VMTypeInfo::GetFunctionDefSignature() const
		{
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetFuncdefSignature();
		}
		void* VMTypeInfo::SetUserData(void* Data, uint64_t Type)
		{
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->SetUserData(Data, Type);
		}
		void* VMTypeInfo::GetUserData(uint64_t Type) const
		{
			TH_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetUserData(Type);
		}
		bool VMTypeInfo::IsHandle() const
		{
			TH_ASSERT(IsValid(), false, "typeinfo should be valid");
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
		VMFunction::VMFunction(const VMFunction& Base) : Function(Base.Function), Manager(Base.Manager)
		{
		}
		int VMFunction::AddRef() const
		{
			TH_ASSERT(IsValid(), -1, "function should be valid");
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
			TH_ASSERT(IsValid(), -1, "function should be valid");
			return Function->GetId();
		}
		VMFuncType VMFunction::GetType() const
		{
			TH_ASSERT(IsValid(), VMFuncType::DUMMY, "function should be valid");
			return (VMFuncType)Function->GetFuncType();
		}
		const char* VMFunction::GetModuleName() const
		{
			TH_ASSERT(IsValid(), nullptr, "function should be valid");
			return Function->GetModuleName();
		}
		VMModule VMFunction::GetModule() const
		{
			TH_ASSERT(IsValid(), nullptr, "function should be valid");
			return Function->GetModule();
		}
		const char* VMFunction::GetSectionName() const
		{
			TH_ASSERT(IsValid(), nullptr, "function should be valid");
			return Function->GetScriptSectionName();
		}
		const char* VMFunction::GetGroup() const
		{
			TH_ASSERT(IsValid(), nullptr, "function should be valid");
			return Function->GetConfigGroup();
		}
		size_t VMFunction::GetAccessMask() const
		{
			TH_ASSERT(IsValid(), -1, "function should be valid");
			return Function->GetAccessMask();
		}
		VMTypeInfo VMFunction::GetObjectType() const
		{
			TH_ASSERT(IsValid(), nullptr, "function should be valid");
			return Function->GetObjectType();
		}
		const char* VMFunction::GetObjectName() const
		{
			TH_ASSERT(IsValid(), nullptr, "function should be valid");
			return Function->GetObjectName();
		}
		const char* VMFunction::GetName() const
		{
			TH_ASSERT(IsValid(), nullptr, "function should be valid");
			return Function->GetName();
		}
		const char* VMFunction::GetNamespace() const
		{
			TH_ASSERT(IsValid(), nullptr, "function should be valid");
			return Function->GetNamespace();
		}
		const char* VMFunction::GetDecl(bool IncludeObjectName, bool IncludeNamespace, bool IncludeArgNames) const
		{
			TH_ASSERT(IsValid(), nullptr, "function should be valid");
			return Function->GetDeclaration(IncludeObjectName, IncludeNamespace, IncludeArgNames);
		}
		bool VMFunction::IsReadOnly() const
		{
			TH_ASSERT(IsValid(), false, "function should be valid");
			return Function->IsReadOnly();
		}
		bool VMFunction::IsPrivate() const
		{
			TH_ASSERT(IsValid(), false, "function should be valid");
			return Function->IsPrivate();
		}
		bool VMFunction::IsProtected() const
		{
			TH_ASSERT(IsValid(), false, "function should be valid");
			return Function->IsProtected();
		}
		bool VMFunction::IsFinal() const
		{
			TH_ASSERT(IsValid(), false, "function should be valid");
			return Function->IsFinal();
		}
		bool VMFunction::IsOverride() const
		{
			TH_ASSERT(IsValid(), false, "function should be valid");
			return Function->IsOverride();
		}
		bool VMFunction::IsShared() const
		{
			TH_ASSERT(IsValid(), false, "function should be valid");
			return Function->IsShared();
		}
		bool VMFunction::IsExplicit() const
		{
			TH_ASSERT(IsValid(), false, "function should be valid");
			return Function->IsExplicit();
		}
		bool VMFunction::IsProperty() const
		{
			TH_ASSERT(IsValid(), false, "function should be valid");
			return Function->IsProperty();
		}
		unsigned int VMFunction::GetArgsCount() const
		{
			TH_ASSERT(IsValid(), 0, "function should be valid");
			return Function->GetParamCount();
		}
		int VMFunction::GetArg(unsigned int Index, int* TypeId, size_t* Flags, const char** Name, const char** DefaultArg) const
		{
			TH_ASSERT(IsValid(), -1, "function should be valid");

			asDWORD asFlags;
			int R = Function->GetParam(Index, TypeId, &asFlags, Name, DefaultArg);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;

			return R;
		}
		int VMFunction::GetReturnTypeId(size_t* Flags) const
		{
			TH_ASSERT(IsValid(), -1, "function should be valid");

			asDWORD asFlags;
			int R = Function->GetReturnTypeId(&asFlags);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;

			return R;
		}
		int VMFunction::GetTypeId() const
		{
			TH_ASSERT(IsValid(), -1, "function should be valid");
			return Function->GetTypeId();
		}
		bool VMFunction::IsCompatibleWithTypeId(int TypeId) const
		{
			TH_ASSERT(IsValid(), false, "function should be valid");
			return Function->IsCompatibleWithTypeId(TypeId);
		}
		void* VMFunction::GetDelegateObject() const
		{
			TH_ASSERT(IsValid(), nullptr, "function should be valid");
			return Function->GetDelegateObject();
		}
		VMTypeInfo VMFunction::GetDelegateObjectType() const
		{
			TH_ASSERT(IsValid(), nullptr, "function should be valid");
			return Function->GetDelegateObjectType();
		}
		VMFunction VMFunction::GetDelegateFunction() const
		{
			TH_ASSERT(IsValid(), nullptr, "function should be valid");
			return Function->GetDelegateFunction();
		}
		unsigned int VMFunction::GetPropertiesCount() const
		{
			TH_ASSERT(IsValid(), 0, "function should be valid");
			return Function->GetVarCount();
		}
		int VMFunction::GetProperty(unsigned int Index, const char** Name, int* TypeId) const
		{
			TH_ASSERT(IsValid(), -1, "function should be valid");
			return Function->GetVar(Index, Name, TypeId);
		}
		const char* VMFunction::GetPropertyDecl(unsigned int Index, bool IncludeNamespace) const
		{
			TH_ASSERT(IsValid(), nullptr, "function should be valid");
			return Function->GetVarDecl(Index, IncludeNamespace);
		}
		int VMFunction::FindNextLineWithCode(int Line) const
		{
			TH_ASSERT(IsValid(), -1, "function should be valid");
			return Function->FindNextLineWithCode(Line);
		}
		void* VMFunction::SetUserData(void* UserData, uint64_t Type)
		{
			TH_ASSERT(IsValid(), nullptr, "function should be valid");
			return Function->SetUserData(UserData, Type);
		}
		void* VMFunction::GetUserData(uint64_t Type) const
		{
			TH_ASSERT(IsValid(), nullptr, "function should be valid");
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
			TH_ASSERT(IsValid(), 0, "object should be valid");
			return Object->AddRef();
		}
		int VMObject::Release()
		{
			if (!IsValid())
				return -1;

			int R = Object->Release();
			Object = nullptr;

			return R;
		}
		VMTypeInfo VMObject::GetObjectType()
		{
			TH_ASSERT(IsValid(), nullptr, "object should be valid");
			return Object->GetObjectType();
		}
		int VMObject::GetTypeId()
		{
			TH_ASSERT(IsValid(), 0, "object should be valid");
			return Object->GetTypeId();
		}
		int VMObject::GetPropertyTypeId(unsigned int Id) const
		{
			TH_ASSERT(IsValid(), 0, "object should be valid");
			return Object->GetPropertyTypeId(Id);
		}
		unsigned int VMObject::GetPropertiesCount() const
		{
			TH_ASSERT(IsValid(), 0, "object should be valid");
			return Object->GetPropertyCount();
		}
		const char* VMObject::GetPropertyName(unsigned int Id) const
		{
			TH_ASSERT(IsValid(), nullptr, "object should be valid");
			return Object->GetPropertyName(Id);
		}
		void* VMObject::GetAddressOfProperty(unsigned int Id)
		{
			TH_ASSERT(IsValid(), nullptr, "object should be valid");
			return Object->GetAddressOfProperty(Id);
		}
		VMManager* VMObject::GetManager() const
		{
			TH_ASSERT(IsValid(), nullptr, "object should be valid");
			return VMManager::Get(Object->GetEngine());
		}
		int VMObject::CopyFrom(const VMObject& Other)
		{
			TH_ASSERT(IsValid(), -1, "object should be valid");
			return Object->CopyFrom(Other.GetObject());
		}
		void* VMObject::SetUserData(void* Data, uint64_t Type)
		{
			TH_ASSERT(IsValid(), nullptr, "object should be valid");
			return Object->SetUserData(Data, (asPWORD)Type);
		}
		void* VMObject::GetUserData(uint64_t Type) const
		{
			TH_ASSERT(IsValid(), nullptr, "object should be valid");
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
			TH_ASSERT(IsValid(), nullptr, "generic should be valid");
			return Generic->GetObject();
		}
		int VMGeneric::GetObjectTypeId() const
		{
			TH_ASSERT(IsValid(), -1, "generic should be valid");
			return Generic->GetObjectTypeId();
		}
		int VMGeneric::GetArgsCount() const
		{
			TH_ASSERT(IsValid(), -1, "generic should be valid");
			return Generic->GetArgCount();
		}
		int VMGeneric::GetArgTypeId(unsigned int Argument, size_t* Flags) const
		{
			TH_ASSERT(IsValid(), -1, "generic should be valid");

			asDWORD asFlags;
			int R = Generic->GetArgTypeId(Argument, &asFlags);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;

			return R;
		}
		unsigned char VMGeneric::GetArgByte(unsigned int Argument)
		{
			TH_ASSERT(IsValid(), 0, "generic should be valid");
			return Generic->GetArgByte(Argument);
		}
		unsigned short VMGeneric::GetArgWord(unsigned int Argument)
		{
			TH_ASSERT(IsValid(), 0, "generic should be valid");
			return Generic->GetArgWord(Argument);
		}
		size_t VMGeneric::GetArgDWord(unsigned int Argument)
		{
			TH_ASSERT(IsValid(), 0, "generic should be valid");
			return Generic->GetArgDWord(Argument);
		}
		uint64_t VMGeneric::GetArgQWord(unsigned int Argument)
		{
			TH_ASSERT(IsValid(), 0, "generic should be valid");
			return Generic->GetArgQWord(Argument);
		}
		float VMGeneric::GetArgFloat(unsigned int Argument)
		{
			TH_ASSERT(IsValid(), 0.0f, "generic should be valid");
			return Generic->GetArgFloat(Argument);
		}
		double VMGeneric::GetArgDouble(unsigned int Argument)
		{
			TH_ASSERT(IsValid(), 0.0, "generic should be valid");
			return Generic->GetArgDouble(Argument);
		}
		void* VMGeneric::GetArgAddress(unsigned int Argument)
		{
			TH_ASSERT(IsValid(), nullptr, "generic should be valid");
			return Generic->GetArgAddress(Argument);
		}
		void* VMGeneric::GetArgObjectAddress(unsigned int Argument)
		{
			TH_ASSERT(IsValid(), nullptr, "generic should be valid");
			return Generic->GetArgObject(Argument);
		}
		void* VMGeneric::GetAddressOfArg(unsigned int Argument)
		{
			TH_ASSERT(IsValid(), nullptr, "generic should be valid");
			return Generic->GetAddressOfArg(Argument);
		}
		int VMGeneric::GetReturnTypeId(size_t* Flags) const
		{
			TH_ASSERT(IsValid(), -1, "generic should be valid");

			asDWORD asFlags;
			int R = Generic->GetReturnTypeId(&asFlags);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;

			return R;
		}
		int VMGeneric::SetReturnByte(unsigned char Value)
		{
			TH_ASSERT(IsValid(), -1, "generic should be valid");
			return Generic->SetReturnByte(Value);
		}
		int VMGeneric::SetReturnWord(unsigned short Value)
		{
			TH_ASSERT(IsValid(), -1, "generic should be valid");
			return Generic->SetReturnWord(Value);
		}
		int VMGeneric::SetReturnDWord(size_t Value)
		{
			TH_ASSERT(IsValid(), -1, "generic should be valid");
			return Generic->SetReturnDWord(Value);
		}
		int VMGeneric::SetReturnQWord(uint64_t Value)
		{
			TH_ASSERT(IsValid(), -1, "generic should be valid");
			return Generic->SetReturnQWord(Value);
		}
		int VMGeneric::SetReturnFloat(float Value)
		{
			TH_ASSERT(IsValid(), -1, "generic should be valid");
			return Generic->SetReturnFloat(Value);
		}
		int VMGeneric::SetReturnDouble(double Value)
		{
			TH_ASSERT(IsValid(), -1, "generic should be valid");
			return Generic->SetReturnDouble(Value);
		}
		int VMGeneric::SetReturnAddress(void* Address)
		{
			TH_ASSERT(IsValid(), -1, "generic should be valid");
			return Generic->SetReturnAddress(Address);
		}
		int VMGeneric::SetReturnObjectAddress(void* Object)
		{
			TH_ASSERT(IsValid(), -1, "generic should be valid");
			return Generic->SetReturnObject(Object);
		}
		void* VMGeneric::GetAddressOfReturnLocation()
		{
			TH_ASSERT(IsValid(), nullptr, "generic should be valid");
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
			TH_ASSERT(IsValid(), -1, "class should be valid");
			TH_ASSERT(Decl != nullptr, -1, "declaration should be set");

			VMCManager* Engine = Manager->GetEngine();
			TH_ASSERT(Engine != nullptr, -1, "engine should be set");

			return Engine->RegisterFuncdef(Decl);;
		}
		int VMClass::SetOperatorCopyAddress(asSFuncPtr* Value)
		{
			TH_ASSERT(IsValid(), -1, "class should be valid");
			TH_ASSERT(Value != nullptr, -1, "value should be set");

			VMCManager* Engine = Manager->GetEngine();
			TH_ASSERT(Engine != nullptr, -1, "engine should be set");

			Core::Parser Decl = Core::Form("%s& opAssign(const %s &in)", Object.c_str(), Object.c_str());
			return Engine->RegisterObjectMethod(Object.c_str(), Decl.Get(), *Value, asCALL_THISCALL);
		}
		int VMClass::SetBehaviourAddress(const char* Decl, VMBehave Behave, asSFuncPtr* Value, VMCall Type)
		{
			TH_ASSERT(IsValid(), -1, "class should be valid");
			TH_ASSERT(Decl != nullptr, -1, "declaration should be set");
			TH_ASSERT(Value != nullptr, -1, "value should be set");

			VMCManager* Engine = Manager->GetEngine();
			TH_ASSERT(Engine != nullptr, -1, "engine should be set");

			return Engine->RegisterObjectBehaviour(Object.c_str(), (asEBehaviours)Behave, Decl, *Value, (asECallConvTypes)Type);
		}
		int VMClass::SetPropertyAddress(const char* Decl, int Offset)
		{
			TH_ASSERT(IsValid(), -1, "class should be valid");
			TH_ASSERT(Decl != nullptr, -1, "declaration should be set");

			VMCManager* Engine = Manager->GetEngine();
			TH_ASSERT(Engine != nullptr, -1, "engine should be set");

			return Engine->RegisterObjectProperty(Object.c_str(), Decl, Offset);
		}
		int VMClass::SetPropertyStaticAddress(const char* Decl, void* Value)
		{
			TH_ASSERT(IsValid(), -1, "class should be valid");
			TH_ASSERT(Decl != nullptr, -1, "declaration should be set");
			TH_ASSERT(Value != nullptr, -1, "value should be set");

			VMCManager* Engine = Manager->GetEngine();
			TH_ASSERT(Engine != nullptr, -1, "engine should be set");

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
			TH_ASSERT(IsValid(), -1, "class should be valid");
			TH_ASSERT(Decl != nullptr, -1, "declaration should be set");
			TH_ASSERT(Value != nullptr, -1, "value should be set");

			VMCManager* Engine = Manager->GetEngine();
			TH_ASSERT(Engine != nullptr, -1, "engine should be set");

			return Engine->RegisterObjectMethod(Object.c_str(), Decl, *Value, (asECallConvTypes)Type);
		}
		int VMClass::SetMethodStaticAddress(const char* Decl, asSFuncPtr* Value, VMCall Type)
		{
			TH_ASSERT(IsValid(), -1, "class should be valid");
			TH_ASSERT(Decl != nullptr, -1, "declaration should be set");
			TH_ASSERT(Value != nullptr, -1, "value should be set");

			VMCManager* Engine = Manager->GetEngine();
			TH_ASSERT(Engine != nullptr, -1, "engine should be set");

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
			TH_ASSERT(IsValid(), -1, "class should be valid");
			TH_ASSERT(Decl != nullptr, -1, "declaration should be set");
			TH_ASSERT(Value != nullptr, -1, "value should be set");

			VMCManager* Engine = Manager->GetEngine();
			TH_ASSERT(Engine != nullptr, -1, "engine should be set");

			return Engine->RegisterObjectBehaviour(Object.c_str(), asBEHAVE_CONSTRUCT, Decl, *Value, (asECallConvTypes)Type);
		}
		int VMClass::SetConstructorListAddress(const char* Decl, asSFuncPtr* Value, VMCall Type)
		{
			TH_ASSERT(IsValid(), -1, "class should be valid");
			TH_ASSERT(Decl != nullptr, -1, "declaration should be set");
			TH_ASSERT(Value != nullptr, -1, "value should be set");

			VMCManager* Engine = Manager->GetEngine();
			TH_ASSERT(Engine != nullptr, -1, "engine should be set");

			return Engine->RegisterObjectBehaviour(Object.c_str(), asBEHAVE_LIST_CONSTRUCT, Decl, *Value, (asECallConvTypes)Type);
		}
		int VMClass::SetDestructorAddress(const char* Decl, asSFuncPtr* Value)
		{
			TH_ASSERT(IsValid(), -1, "class should be valid");
			TH_ASSERT(Decl != nullptr, -1, "declaration should be set");
			TH_ASSERT(Value != nullptr, -1, "value should be set");

			VMCManager* Engine = Manager->GetEngine();
			TH_ASSERT(Engine != nullptr, -1, "engine should be set");

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
				case VMOpFunc::Neg:
					if (Right)
						return "";

					return Core::Form("%s opNeg()%s", Out, Const ? " const" : "");
				case VMOpFunc::Com:
					if (Right)
						return "";

					return Core::Form("%s opCom()%s", Out, Const ? " const" : "");
				case VMOpFunc::PreInc:
					if (Right)
						return "";

					return Core::Form("%s opPreInc()%s", Out, Const ? " const" : "");
				case VMOpFunc::PreDec:
					if (Right)
						return "";

					return Core::Form("%s opPreDec()%s", Out, Const ? " const" : "");
				case VMOpFunc::PostInc:
					if (Right)
						return "";

					return Core::Form("%s opPostInc()%s", Out, Const ? " const" : "");
				case VMOpFunc::PostDec:
					if (Right)
						return "";

					return Core::Form("%s opPostDec()%s", Out, Const ? " const" : "");
				case VMOpFunc::Equals:
					if (Right)
						return "";

					return Core::Form("%s opEquals(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::Cmp:
					if (Right)
						return "";

					return Core::Form("%s opCmp(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::Assign:
					if (Right)
						return "";

					return Core::Form("%s opAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::AddAssign:
					if (Right)
						return "";

					return Core::Form("%s opAddAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::SubAssign:
					if (Right)
						return "";

					return Core::Form("%s opSubAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::MulAssign:
					if (Right)
						return "";

					return Core::Form("%s opMulAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::DivAssign:
					if (Right)
						return "";

					return Core::Form("%s opDivAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::ModAssign:
					if (Right)
						return "";

					return Core::Form("%s opModAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::PowAssign:
					if (Right)
						return "";

					return Core::Form("%s opPowAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::AndAssign:
					if (Right)
						return "";

					return Core::Form("%s opAndAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::OrAssign:
					if (Right)
						return "";

					return Core::Form("%s opOrAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::XOrAssign:
					if (Right)
						return "";

					return Core::Form("%s opXorAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::ShlAssign:
					if (Right)
						return "";

					return Core::Form("%s opShlAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::ShrAssign:
					if (Right)
						return "";

					return Core::Form("%s opShrAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::UshrAssign:
					if (Right)
						return "";

					return Core::Form("%s opUshrAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::Add:
					return Core::Form("%s opAdd%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::Sub:
					return Core::Form("%s opSub%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::Mul:
					return Core::Form("%s opMul%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::Div:
					return Core::Form("%s opDiv%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::Mod:
					return Core::Form("%s opMod%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::Pow:
					return Core::Form("%s opPow%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::And:
					return Core::Form("%s opAnd%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::Or:
					return Core::Form("%s opOr%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::XOr:
					return Core::Form("%s opXor%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::Shl:
					return Core::Form("%s opShl%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::Shr:
					return Core::Form("%s opShr%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::Ushr:
					return Core::Form("%s opUshr%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::Index:
					if (Right)
						return "";

					return Core::Form("%s opIndex(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::Call:
					if (Right)
						return "";

					return Core::Form("%s opCall(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case VMOpFunc::Cast:
					if (Right)
						return "";

					return Core::Form("%s opCast()%s", Out, Const ? " const" : "");
				case VMOpFunc::ImplCast:
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
			TH_ASSERT(IsValid(), -1, "interface should be valid");
			TH_ASSERT(Decl != nullptr, -1, "declaration should be set");

			VMCManager* Engine = Manager->GetEngine();
			TH_ASSERT(Engine != nullptr, -1, "engine should be set");

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
			TH_ASSERT(IsValid(), -1, "enum should be valid");
			TH_ASSERT(Name != nullptr, -1, "name should be set");

			VMCManager* Engine = Manager->GetEngine();
			TH_ASSERT(Engine != nullptr, -1, "engine should be set");

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
			TH_ASSERT_V(IsValid(), "module should be valid");
			TH_ASSERT_V(Name != nullptr, "name should be set");
			return Mod->SetName(Name);
		}
		int VMModule::AddSection(const char* Name, const char* Code, size_t CodeLength, int LineOffset)
		{
			TH_ASSERT(IsValid(), -1, "module should be valid");
			TH_ASSERT(Name != nullptr, -1, "name should be set");
			TH_ASSERT(Code != nullptr, -1, "code should be set");
			return Mod->AddScriptSection(Name, Code, CodeLength, LineOffset);
		}
		int VMModule::RemoveFunction(const VMFunction& Function)
		{
			TH_ASSERT(IsValid(), -1, "module should be valid");
			return Mod->RemoveFunction(Function.GetFunction());
		}
		int VMModule::ResetProperties(VMCContext* Context)
		{
			TH_ASSERT(IsValid(), -1, "module should be valid");
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Mod->ResetGlobalVars(Context);
		}
		int VMModule::Build()
		{
			TH_ASSERT(IsValid(), -1, "module should be valid");
			Manager->Lock();
			int R = Mod->Build();
			Manager->Unlock();

			return R;
		}
		int VMModule::LoadByteCode(VMByteCode* Info)
		{
			TH_ASSERT(IsValid(), -1, "module should be valid");
			TH_ASSERT(Info != nullptr, -1, "bytecode should be set");

			int R = -1;
			Manager->Lock();
			{
				CByteCodeStream* Stream = TH_NEW(CByteCodeStream, Info->Data);
				R = Mod->LoadByteCode(Stream, &Info->Debug);
				TH_DELETE(CByteCodeStream, Stream);
			}
			Manager->Unlock();

			return R;
		}
		int VMModule::Discard()
		{
			TH_ASSERT(IsValid(), -1, "module should be valid");
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
			TH_ASSERT(IsValid(), -1, "module should be valid");
			return Mod->BindImportedFunction((asUINT)ImportIndex, Function.GetFunction());
		}
		int VMModule::UnbindImportedFunction(size_t ImportIndex)
		{
			TH_ASSERT(IsValid(), -1, "module should be valid");
			return Mod->UnbindImportedFunction((asUINT)ImportIndex);
		}
		int VMModule::BindAllImportedFunctions()
		{
			TH_ASSERT(IsValid(), -1, "module should be valid");
			return Mod->BindAllImportedFunctions();
		}
		int VMModule::UnbindAllImportedFunctions()
		{
			TH_ASSERT(IsValid(), -1, "module should be valid");
			return Mod->UnbindAllImportedFunctions();
		}
		int VMModule::CompileFunction(const char* SectionName, const char* Code, int LineOffset, size_t CompileFlags, VMFunction* OutFunction)
		{
			TH_ASSERT(IsValid(), -1, "module should be valid");
			TH_ASSERT(SectionName != nullptr, -1, "section name should be set");
			TH_ASSERT(Code != nullptr, -1, "code should be set");

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
			TH_ASSERT(IsValid(), -1, "module should be valid");
			Manager->Lock();
			int R = Mod->CompileGlobalVar(SectionName, Code, LineOffset);
			Manager->Unlock();

			return R;
		}
		int VMModule::SetDefaultNamespace(const char* NameSpace)
		{
			TH_ASSERT(IsValid(), -1, "module should be valid");
			return Mod->SetDefaultNamespace(NameSpace);
		}
		void* VMModule::GetAddressOfProperty(size_t Index)
		{
			TH_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetAddressOfGlobalVar(Index);
		}
		int VMModule::RemoveProperty(size_t Index)
		{
			TH_ASSERT(IsValid(), -1, "module should be valid");
			return Mod->RemoveGlobalVar(Index);
		}
		size_t VMModule::SetAccessMask(size_t AccessMask)
		{
			TH_ASSERT(IsValid(), 0, "module should be valid");
			return Mod->SetAccessMask((asDWORD)AccessMask);
		}
		size_t VMModule::GetFunctionCount() const
		{
			TH_ASSERT(IsValid(), 0, "module should be valid");
			return Mod->GetFunctionCount();
		}
		VMFunction VMModule::GetFunctionByIndex(size_t Index) const
		{
			TH_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetFunctionByIndex((asUINT)Index);
		}
		VMFunction VMModule::GetFunctionByDecl(const char* Decl) const
		{
			TH_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetFunctionByDecl(Decl);
		}
		VMFunction VMModule::GetFunctionByName(const char* Name) const
		{
			TH_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetFunctionByName(Name);
		}
		int VMModule::GetTypeIdByDecl(const char* Decl) const
		{
			TH_ASSERT(IsValid(), -1, "module should be valid");
			return Mod->GetTypeIdByDecl(Decl);
		}
		int VMModule::GetImportedFunctionIndexByDecl(const char* Decl) const
		{
			TH_ASSERT(IsValid(), -1, "module should be valid");
			return Mod->GetImportedFunctionIndexByDecl(Decl);
		}
		int VMModule::SaveByteCode(VMByteCode* Info) const
		{
			TH_ASSERT(IsValid(), -1, "module should be valid");
			TH_ASSERT(Info != nullptr, -1, "bytecode should be set");

			int R = -1;
			Manager->Lock();
			{
				CByteCodeStream* Stream = TH_NEW(CByteCodeStream);
				R = Mod->SaveByteCode(Stream, Info->Debug);
				Info->Data = Stream->GetCode();
				TH_DELETE(CByteCodeStream, Stream);
			}
			Manager->Unlock();

			return R;
		}
		int VMModule::GetPropertyIndexByName(const char* Name) const
		{
			TH_ASSERT(IsValid(), -1, "module should be valid");
			TH_ASSERT(Name != nullptr, -1, "name should be set");

			return Mod->GetGlobalVarIndexByName(Name);
		}
		int VMModule::GetPropertyIndexByDecl(const char* Decl) const
		{
			TH_ASSERT(IsValid(), -1, "module should be valid");
			TH_ASSERT(Decl != nullptr, -1, "declaration should be set");

			return Mod->GetGlobalVarIndexByDecl(Decl);
		}
		int VMModule::GetProperty(size_t Index, VMProperty* Info) const
		{
			TH_ASSERT(IsValid(), -1, "module should be valid");
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
			TH_ASSERT(IsValid(), 0, "module should be valid");
			asDWORD AccessMask = Mod->SetAccessMask(0);
			Mod->SetAccessMask(AccessMask);
			return (size_t)AccessMask;
		}
		size_t VMModule::GetObjectsCount() const
		{
			TH_ASSERT(IsValid(), 0, "module should be valid");
			return Mod->GetObjectTypeCount();
		}
		size_t VMModule::GetPropertiesCount() const
		{
			TH_ASSERT(IsValid(), 0, "module should be valid");
			return Mod->GetGlobalVarCount();
		}
		size_t VMModule::GetEnumsCount() const
		{
			TH_ASSERT(IsValid(), 0, "module should be valid");
			return Mod->GetEnumCount();
		}
		size_t VMModule::GetImportedFunctionCount() const
		{
			TH_ASSERT(IsValid(), 0, "module should be valid");
			return Mod->GetImportedFunctionCount();
		}
		VMTypeInfo VMModule::GetObjectByIndex(size_t Index) const
		{
			TH_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetObjectTypeByIndex(Index);
		}
		VMTypeInfo VMModule::GetTypeInfoByName(const char* Name) const
		{
			TH_ASSERT(IsValid(), nullptr, "module should be valid");
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
			TH_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetTypeInfoByDecl(Decl);
		}
		VMTypeInfo VMModule::GetEnumByIndex(size_t Index) const
		{
			TH_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetEnumByIndex(Index);
		}
		const char* VMModule::GetPropertyDecl(size_t Index, bool IncludeNamespace) const
		{
			TH_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetGlobalVarDeclaration(Index, IncludeNamespace);
		}
		const char* VMModule::GetDefaultNamespace() const
		{
			TH_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetDefaultNamespace();
		}
		const char* VMModule::GetImportedFunctionDecl(size_t ImportIndex) const
		{
			TH_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetImportedFunctionDeclaration(ImportIndex);
		}
		const char* VMModule::GetImportedFunctionModule(size_t ImportIndex) const
		{
			TH_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetImportedFunctionSourceModule(ImportIndex);
		}
		const char* VMModule::GetName() const
		{
			TH_ASSERT(IsValid(), nullptr, "module should be valid");
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
			TH_ASSERT(Manager != nullptr, -1, "global should be valid");
			TH_ASSERT(Decl != nullptr, -1, "declaration should be set");

			VMCManager* Engine = Manager->GetEngine();
			TH_ASSERT(Engine != nullptr, -1, "engine should be set");

			return Engine->RegisterFuncdef(Decl);
		}
		int VMGlobal::SetFunctionAddress(const char* Decl, asSFuncPtr* Value, VMCall Type)
		{
			TH_ASSERT(Manager != nullptr, -1, "global should be valid");
			TH_ASSERT(Decl != nullptr, -1, "declaration should be set");
			TH_ASSERT(Value != nullptr, -1, "value should be set");

			VMCManager* Engine = Manager->GetEngine();
			TH_ASSERT(Engine != nullptr, -1, "engine should be set");

			return Engine->RegisterGlobalFunction(Decl, *Value, (asECallConvTypes)Type);
		}
		int VMGlobal::SetPropertyAddress(const char* Decl, void* Value)
		{
			TH_ASSERT(Manager != nullptr, -1, "global should be valid");
			TH_ASSERT(Decl != nullptr, -1, "declaration should be set");
			TH_ASSERT(Value != nullptr, -1, "value should be set");

			VMCManager* Engine = Manager->GetEngine();
			TH_ASSERT(Engine != nullptr, -1, "engine should be set");

			return Engine->RegisterGlobalProperty(Decl, Value);
		}
		VMInterface VMGlobal::SetInterface(const char* Name)
		{
			TH_ASSERT(Manager != nullptr, VMInterface(nullptr, "", -1), "global should be valid");
			TH_ASSERT(Name != nullptr, VMInterface(nullptr, "", -1), "name should be set");

			VMCManager* Engine = Manager->GetEngine();
			TH_ASSERT(Engine != nullptr, VMInterface(nullptr, "", -1), "engine should be set");

			return VMInterface(Manager, Name, Engine->RegisterInterface(Name));
		}
		VMTypeClass VMGlobal::SetStructAddress(const char* Name, size_t Size, uint64_t Flags)
		{
			TH_ASSERT(Manager != nullptr, VMTypeClass(nullptr, "", -1), "global should be valid");
			TH_ASSERT(Name != nullptr, VMTypeClass(nullptr, "", -1), "name should be set");

			VMCManager* Engine = Manager->GetEngine();
			TH_ASSERT(Engine != nullptr, VMTypeClass(nullptr, "", -1), "engine should be set");

			return VMTypeClass(Manager, Name, Engine->RegisterObjectType(Name, Size, (asDWORD)Flags));
		}
		VMTypeClass VMGlobal::SetPodAddress(const char* Name, size_t Size, uint64_t Flags)
		{
			return SetStructAddress(Name, Size, Flags);
		}
		VMRefClass VMGlobal::SetClassAddress(const char* Name, uint64_t Flags)
		{
			TH_ASSERT(Manager != nullptr, VMRefClass(nullptr, "", -1), "global should be valid");
			TH_ASSERT(Name != nullptr, VMRefClass(nullptr, "", -1), "name should be set");

			VMCManager* Engine = Manager->GetEngine();
			TH_ASSERT(Engine != nullptr, VMRefClass(nullptr, "", -1), "engine should be set");

			return VMRefClass(Manager, Name, Engine->RegisterObjectType(Name, 0, (asDWORD)Flags));
		}
		VMEnum VMGlobal::SetEnum(const char* Name)
		{
			TH_ASSERT(Manager != nullptr, VMEnum(nullptr, "", -1), "global should be valid");
			TH_ASSERT(Name != nullptr, VMEnum(nullptr, "", -1), "name should be set");

			VMCManager* Engine = Manager->GetEngine();
			TH_ASSERT(Engine != nullptr, VMEnum(nullptr, "", -1), "engine should be set");

			return VMEnum(Manager, Name, Engine->RegisterEnum(Name));
		}
		size_t VMGlobal::GetFunctionsCount() const
		{
			TH_ASSERT(Manager != nullptr, 0, "global should be valid");
			return Manager->GetEngine()->GetGlobalFunctionCount();
		}
		VMFunction VMGlobal::GetFunctionById(int Id) const
		{
			TH_ASSERT(Manager != nullptr, nullptr, "global should be valid");
			return Manager->GetEngine()->GetFunctionById(Id);
		}
		VMFunction VMGlobal::GetFunctionByIndex(int Index) const
		{
			TH_ASSERT(Manager != nullptr, nullptr, "global should be valid");
			return Manager->GetEngine()->GetGlobalFunctionByIndex(Index);
		}
		VMFunction VMGlobal::GetFunctionByDecl(const char* Decl) const
		{
			TH_ASSERT(Manager != nullptr, nullptr, "global should be valid");
			TH_ASSERT(Decl != nullptr, nullptr, "declaration should be set");

			return Manager->GetEngine()->GetGlobalFunctionByDecl(Decl);
		}
		size_t VMGlobal::GetPropertiesCount() const
		{
			TH_ASSERT(Manager != nullptr, 0, "global should be valid");
			return Manager->GetEngine()->GetGlobalPropertyCount();
		}
		int VMGlobal::GetPropertyByIndex(int Index, VMProperty* Info) const
		{
			TH_ASSERT(Manager != nullptr, -1, "global should be valid");
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
			TH_ASSERT(Manager != nullptr, -1, "global should be valid");
			TH_ASSERT(Name != nullptr, -1, "name should be set");

			return Manager->GetEngine()->GetGlobalPropertyIndexByName(Name);
		}
		int VMGlobal::GetPropertyIndexByDecl(const char* Decl) const
		{
			TH_ASSERT(Manager != nullptr, -1, "global should be valid");
			TH_ASSERT(Decl != nullptr, -1, "declaration should be set");

			return Manager->GetEngine()->GetGlobalPropertyIndexByDecl(Decl);
		}
		size_t VMGlobal::GetObjectsCount() const
		{
			TH_ASSERT(Manager != nullptr, 0, "global should be valid");
			return Manager->GetEngine()->GetObjectTypeCount();
		}
		VMTypeInfo VMGlobal::GetObjectByTypeId(int TypeId) const
		{
			TH_ASSERT(Manager != nullptr, nullptr, "global should be valid");
			return Manager->GetEngine()->GetObjectTypeByIndex(TypeId);
		}
		size_t VMGlobal::GetEnumCount() const
		{
			TH_ASSERT(Manager != nullptr, 0, "global should be valid");
			return Manager->GetEngine()->GetEnumCount();
		}
		VMTypeInfo VMGlobal::GetEnumByTypeId(int TypeId) const
		{
			TH_ASSERT(Manager != nullptr, nullptr, "global should be valid");
			return Manager->GetEngine()->GetEnumByIndex(TypeId);
		}
		size_t VMGlobal::GetFunctionDefsCount() const
		{
			TH_ASSERT(Manager != nullptr, 0, "global should be valid");
			return Manager->GetEngine()->GetFuncdefCount();
		}
		VMTypeInfo VMGlobal::GetFunctionDefByIndex(int Index) const
		{
			TH_ASSERT(Manager != nullptr, nullptr, "global should be valid");
			return Manager->GetEngine()->GetFuncdefByIndex(Index);
		}
		size_t VMGlobal::GetModulesCount() const
		{
			TH_ASSERT(Manager != nullptr, 0, "global should be valid");
			return Manager->GetEngine()->GetModuleCount();
		}
		VMCModule* VMGlobal::GetModuleById(int Id) const
		{
			TH_ASSERT(Manager != nullptr, nullptr, "global should be valid");
			return Manager->GetEngine()->GetModuleByIndex(Id);
		}
		int VMGlobal::GetTypeIdByDecl(const char* Decl) const
		{
			TH_ASSERT(Manager != nullptr, -1, "global should be valid");
			TH_ASSERT(Decl != nullptr, -1, "declaration should be set");

			return Manager->GetEngine()->GetTypeIdByDecl(Decl);
		}
		const char* VMGlobal::GetTypeIdDecl(int TypeId, bool IncludeNamespace) const
		{
			TH_ASSERT(Manager != nullptr, nullptr, "global should be valid");
			return Manager->GetEngine()->GetTypeDeclaration(TypeId, IncludeNamespace);
		}
		int VMGlobal::GetSizeOfPrimitiveType(int TypeId) const
		{
			TH_ASSERT(Manager != nullptr, -1, "global should be valid");
			return Manager->GetEngine()->GetSizeOfPrimitiveType(TypeId);
		}
		std::string VMGlobal::GetObjectView(void* Object, int TypeId)
		{
			TH_ASSERT(Manager != nullptr, "null", "global should be valid");
			if (!Object)
				return "null";

			if (TypeId == (int)VMTypeId::INT8)
				return "int8(" + std::to_string(*(char*)(Object)) + "), ";
			else if (TypeId == (int)VMTypeId::INT16)
				return "int16(" + std::to_string(*(short*)(Object)) + "), ";
			else if (TypeId == (int)VMTypeId::INT32)
				return "int32(" + std::to_string(*(int*)(Object)) + "), ";
			else if (TypeId == (int)VMTypeId::INT64)
				return "int64(" + std::to_string(*(int64_t*)(Object)) + "), ";
			else if (TypeId == (int)VMTypeId::UINT8)
				return "uint8(" + std::to_string(*(unsigned char*)(Object)) + "), ";
			else if (TypeId == (int)VMTypeId::UINT16)
				return "uint16(" + std::to_string(*(unsigned short*)(Object)) + "), ";
			else if (TypeId == (int)VMTypeId::UINT32)
				return "uint32(" + std::to_string(*(unsigned int*)(Object)) + "), ";
			else if (TypeId == (int)VMTypeId::UINT64)
				return "uint64(" + std::to_string(*(uint64_t*)(Object)) + "), ";
			else if (TypeId == (int)VMTypeId::FLOAT)
				return "float(" + std::to_string(*(float*)(Object)) + "), ";
			else if (TypeId == (int)VMTypeId::DOUBLE)
				return "double(" + std::to_string(*(double*)(Object)) + "), ";

			VMCTypeInfo* Type = Manager->GetEngine()->GetTypeInfoById(TypeId);
			const char* Name = Type->GetName();

			return Core::Form("%s(0x%" PRIXPTR ")", Name ? Name : "unknown", (uintptr_t)Object).R();
		}
		VMTypeInfo VMGlobal::GetTypeInfoById(int TypeId) const
		{
			TH_ASSERT(Manager != nullptr, nullptr, "global should be valid");
			return Manager->GetEngine()->GetTypeInfoById(TypeId);
		}
		VMTypeInfo VMGlobal::GetTypeInfoByName(const char* Name) const
		{
			TH_ASSERT(Manager != nullptr, nullptr, "global should be valid");
			TH_ASSERT(Name != nullptr, nullptr, "name should be set");

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
			TH_ASSERT(Manager != nullptr, nullptr, "global should be valid");
			TH_ASSERT(Decl != nullptr, nullptr, "declaration should be set");

			return Manager->GetEngine()->GetTypeInfoByDecl(Decl);
		}
		VMManager* VMGlobal::GetManager() const
		{
			return Manager;
		}

		VMCompiler::VMCompiler(VMManager* Engine) : Module(nullptr), Manager(Engine), Context(nullptr), BuiltOK(false)
		{
			TH_ASSERT_V(Manager != nullptr, "engine should be set");

			Processor = new Compute::Preprocessor();
			Processor->SetIncludeCallback([this](Compute::Preprocessor* C, const Compute::IncludeResult& File, std::string* Out)
			{
				TH_ASSERT(Manager != nullptr, false, "engine should be set");
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
				TH_ASSERT(Manager != nullptr, false, "engine should be set");
				if (Pragma && Pragma(C, Name, Args))
					return true;

				if (Name == "compile" && Args.size() == 2)
				{
					const std::string& Key = Args[0];
					Core::Parser Value(&Args[1]);

					size_t Result = Value.HasInteger() ? Value.ToUInt64() : 0;
					if (Key == "ALLOW_UNSAFE_REFERENCES")
						Manager->SetProperty(VMProp::ALLOW_UNSAFE_REFERENCES, Result);
					else if (Key == "OPTIMIZE_BYTECODE")
						Manager->SetProperty(VMProp::OPTIMIZE_BYTECODE, Result);
					else if (Key == "COPY_SCRIPT_SECTIONS")
						Manager->SetProperty(VMProp::COPY_SCRIPT_SECTIONS, Result);
					else if (Key == "MAX_STACK_SIZE")
						Manager->SetProperty(VMProp::MAX_STACK_SIZE, Result);
					else if (Key == "USE_CHARACTER_LITERALS")
						Manager->SetProperty(VMProp::USE_CHARACTER_LITERALS, Result);
					else if (Key == "ALLOW_MULTILINE_STRINGS")
						Manager->SetProperty(VMProp::ALLOW_MULTILINE_STRINGS, Result);
					else if (Key == "ALLOW_IMPLICIT_HANDLE_TYPES")
						Manager->SetProperty(VMProp::ALLOW_IMPLICIT_HANDLE_TYPES, Result);
					else if (Key == "BUILD_WITHOUT_LINE_CUES")
						Manager->SetProperty(VMProp::BUILD_WITHOUT_LINE_CUES, Result);
					else if (Key == "INIT_GLOBAL_VARS_AFTER_BUILD")
						Manager->SetProperty(VMProp::INIT_GLOBAL_VARS_AFTER_BUILD, Result);
					else if (Key == "REQUIRE_ENUM_SCOPE")
						Manager->SetProperty(VMProp::REQUIRE_ENUM_SCOPE, Result);
					else if (Key == "SCRIPT_SCANNER")
						Manager->SetProperty(VMProp::SCRIPT_SCANNER, Result);
					else if (Key == "INCLUDE_JIT_INSTRUCTIONS")
						Manager->SetProperty(VMProp::INCLUDE_JIT_INSTRUCTIONS, Result);
					else if (Key == "STRING_ENCODING")
						Manager->SetProperty(VMProp::STRING_ENCODING, Result);
					else if (Key == "PROPERTY_ACCESSOR_MODE")
						Manager->SetProperty(VMProp::PROPERTY_ACCESSOR_MODE, Result);
					else if (Key == "EXPAND_DEF_ARRAY_TO_TMPL")
						Manager->SetProperty(VMProp::EXPAND_DEF_ARRAY_TO_TMPL, Result);
					else if (Key == "AUTO_GARBAGE_COLLECT")
						Manager->SetProperty(VMProp::AUTO_GARBAGE_COLLECT, Result);
					else if (Key == "DISALLOW_GLOBAL_VARS")
						Manager->SetProperty(VMProp::ALWAYS_IMPL_DEFAULT_CONSTRUCT, Result);
					else if (Key == "ALWAYS_IMPL_DEFAULT_CONSTRUCT")
						Manager->SetProperty(VMProp::ALWAYS_IMPL_DEFAULT_CONSTRUCT, Result);
					else if (Key == "COMPILER_WARNINGS")
						Manager->SetProperty(VMProp::COMPILER_WARNINGS, Result);
					else if (Key == "DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE")
						Manager->SetProperty(VMProp::DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE, Result);
					else if (Key == "ALTER_SYNTAX_NAMED_ARGS")
						Manager->SetProperty(VMProp::ALTER_SYNTAX_NAMED_ARGS, Result);
					else if (Key == "DISABLE_INTEGER_DIVISION")
						Manager->SetProperty(VMProp::DISABLE_INTEGER_DIVISION, Result);
					else if (Key == "DISALLOW_EMPTY_LIST_ELEMENTS")
						Manager->SetProperty(VMProp::DISALLOW_EMPTY_LIST_ELEMENTS, Result);
					else if (Key == "PRIVATE_PROP_AS_PROTECTED")
						Manager->SetProperty(VMProp::PRIVATE_PROP_AS_PROTECTED, Result);
					else if (Key == "ALLOW_UNICODE_IDENTIFIERS")
						Manager->SetProperty(VMProp::ALLOW_UNICODE_IDENTIFIERS, Result);
					else if (Key == "HEREDOC_TRIM_MODE")
						Manager->SetProperty(VMProp::HEREDOC_TRIM_MODE, Result);
					else if (Key == "MAX_NESTED_CALLS")
						Manager->SetProperty(VMProp::MAX_NESTED_CALLS, Result);
					else if (Key == "GENERIC_CALL_MODE")
						Manager->SetProperty(VMProp::GENERIC_CALL_MODE, Result);
					else if (Key == "INIT_STACK_SIZE")
						Manager->SetProperty(VMProp::INIT_STACK_SIZE, Result);
					else if (Key == "INIT_CALL_STACK_SIZE")
						Manager->SetProperty(VMProp::INIT_CALL_STACK_SIZE, Result);
					else if (Key == "MAX_CALL_STACK_SIZE")
						Manager->SetProperty(VMProp::MAX_CALL_STACK_SIZE, Result);
				}
				else if (Name == "comment" && Args.size() == 2)
				{
					const std::string& Key = Args[0];
					if (Key == "INFO")
						TH_INFO("%s", Args[1].c_str());
					else if (Key == "TRACE")
						TH_TRACE("%s", Args[1].c_str());
					else if (Key == "WARN")
						TH_WARN("%s", Args[1].c_str());
					else if (Key == "ERR")
						TH_ERR("%s", Args[1].c_str());
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
					if (!Path.empty())
						Path = Core::OS::Path::ResolveResource(Path, Core::OS::Path::GetDirectory(Processor->GetCurrentFilePath().c_str()));

					Manager->ImportLibrary(Path);
				}
				else if (Name == "define" && Args.size() == 1)
					Define(Args[0]);

				return true;
			});

			Context = Manager->CreateContext();
			Context->SetUserData(this, CompilerUD);
			Manager->SetProcessorOptions(Processor);
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
			TH_ASSERT(Manager != nullptr, false, "engine should be set");
			if (Context != nullptr && Context->IsPending())
				return false;

			Manager->Lock();
			{
				if (Context != nullptr)
					Context->SetOnResume(nullptr);

				if (Module != nullptr)
					Module->Discard();

				Module = nullptr;
				BuiltOK = false;
			}
			Manager->Unlock();

			if (Processor != nullptr)
				Processor->Clear();

			return true;
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
			TH_ASSERT(Manager != nullptr, -1, "engine should be set");
			TH_ASSERT(!ModuleName.empty(), -1, "module name should not be empty");

			BuiltOK = false;
			VCache.Valid = false;
			VCache.Name.clear();

			Manager->Lock();
			if (Module != nullptr)
				Module->Discard();

			if (Scoped)
				Module = Manager->CreateScopedModule(ModuleName, false);
			else
				Module = Manager->CreateModule(ModuleName, false);
			Manager->Unlock();

			if (!Module)
				return -1;

			Module->SetUserData(this, CompilerUD);
			Manager->SetProcessorOptions(Processor);
			return 0;
		}
		int VMCompiler::Prepare(const std::string& ModuleName, const std::string& Name, bool Debug, bool Scoped)
		{
			TH_ASSERT(Manager != nullptr, -1, "engine should be set");

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
			TH_ASSERT(Manager != nullptr, -1, "engine should be set");
			TH_ASSERT(Module != nullptr, -1, "module should not be empty");

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
			TH_ASSERT(Manager != nullptr, -1, "engine should be set");
			TH_ASSERT(Module != nullptr, -1, "module should not be empty");
			TH_ASSERT(Info != nullptr, -1, "bytecode should be set");
			TH_ASSERT(BuiltOK, -1, "module should be built");

			Manager->Lock();
			CByteCodeStream* Stream = TH_NEW(CByteCodeStream);
			int R = Module->SaveByteCode(Stream, Info->Debug);
			Info->Data = Stream->GetCode();
			TH_DELETE(CByteCodeStream, Stream);
			Manager->Unlock();

			return R;
		}
		int VMCompiler::LoadByteCode(VMByteCode* Info)
		{
			TH_ASSERT(Manager != nullptr, -1, "engine should be set");
			TH_ASSERT(Module != nullptr, -1, "module should not be empty");
			TH_ASSERT(Info != nullptr, -1, "bytecode should be set");

			Manager->Lock();
			CByteCodeStream* Stream = TH_NEW(CByteCodeStream, Info->Data);
			int R = Module->LoadByteCode(Stream, &Info->Debug);
			TH_DELETE(CByteCodeStream, Stream);
			Manager->Unlock();

			return R;
		}
		int VMCompiler::LoadFile(const std::string& Path)
		{
			TH_ASSERT(Manager != nullptr, -1, "engine should be set");
			TH_ASSERT(Module != nullptr, -1, "module should not be empty");

			if (VCache.Valid)
				return 0;

			std::string Source = Core::OS::Path::Resolve(Path.c_str());
			if (!Core::OS::File::IsExists(Source.c_str()))
			{
				TH_ERR("file not found");
				return -1;
			}

			std::string Buffer = Core::OS::File::ReadAsString(Source.c_str());
			if (!Processor->Process(Source, Buffer))
				return asINVALID_DECLARATION;

			return Module->AddScriptSection(Source.c_str(), Buffer.c_str(), Buffer.size());
		}
		int VMCompiler::LoadCode(const std::string& Name, const std::string& Data)
		{
			TH_ASSERT(Manager != nullptr, -1, "engine should be set");
			TH_ASSERT(Module != nullptr, -1, "module should not be empty");

			if (VCache.Valid)
				return 0;

			std::string Buffer(Data);
			if (!Processor->Process("", Buffer))
				return asINVALID_DECLARATION;

			return Module->AddScriptSection(Name.c_str(), Buffer.c_str(), Buffer.size());
		}
		int VMCompiler::LoadCode(const std::string& Name, const char* Data, uint64_t Size)
		{
			TH_ASSERT(Manager != nullptr, -1, "engine should be set");
			TH_ASSERT(Module != nullptr, -1, "module should not be empty");

			if (VCache.Valid)
				return 0;

			std::string Buffer(Data, Size);
			if (!Processor->Process("", Buffer))
				return asINVALID_DECLARATION;

			return Module->AddScriptSection(Name.c_str(), Buffer.c_str(), Buffer.size());
		}
		Core::Async<int> VMCompiler::ExecuteFileAsync(const char* Name, const char* ModuleName, const char* EntryName, ArgsCallback&& OnArgs, ResumeCallback&& OnResume)
		{
			TH_ASSERT(Manager != nullptr, asINVALID_ARG, "engine should be set");
			TH_ASSERT(Name != nullptr, asINVALID_ARG, "name should be set");
			TH_ASSERT(ModuleName != nullptr, asINVALID_ARG, "module name should be set");
			TH_ASSERT(EntryName != nullptr, asINVALID_ARG, "entry name should be set");

			int R = Prepare(ModuleName, Name);
			if (R < 0)
				return R;

			R = LoadFile(Name);
			if (R < 0)
				return R;

			R = Compile(true);
			if (R < 0)
				return R;

			return ExecuteEntryAsync(EntryName, std::move(OnArgs), std::move(OnResume));
		}
		Core::Async<int> VMCompiler::ExecuteMemoryAsync(const std::string& Buffer, const char* ModuleName, const char* EntryName, ArgsCallback&& OnArgs, ResumeCallback&& OnResume)
		{
			TH_ASSERT(Manager != nullptr, asINVALID_ARG, "engine should be set");
			TH_ASSERT(!Buffer.empty(), asINVALID_ARG, "buffer should not be empty");
			TH_ASSERT(ModuleName != nullptr, asINVALID_ARG, "module name should be set");
			TH_ASSERT(EntryName != nullptr, asINVALID_ARG, "entry name should be set");

			int R = Prepare(ModuleName, "anonymous");
			if (R < 0)
				return R;

			R = LoadCode("anonymous", Buffer);
			if (R < 0)
				return R;

			R = Compile(true);
			if (R < 0)
				return R;

			return ExecuteEntryAsync(EntryName, std::move(OnArgs), std::move(OnResume));
		}
		Core::Async<int> VMCompiler::ExecuteEntryAsync(const char* Name, ArgsCallback&& OnArgs, ResumeCallback&& OnResume)
		{
			TH_ASSERT(Manager != nullptr, asINVALID_ARG, "engine should be set");
			TH_ASSERT(Name != nullptr, asINVALID_ARG, "name should be set");
			TH_ASSERT(Context != nullptr, asINVALID_ARG, "context should be set");
			TH_ASSERT(Module != nullptr, asINVALID_ARG, "module should not be empty");
			TH_ASSERT(BuiltOK, asINVALID_ARG, "module should be built");

			VMCManager* Engine = Manager->GetEngine();
			TH_ASSERT(Engine != nullptr, asINVALID_CONFIGURATION, "engine should be set");

			VMCFunction* Function = Module->GetFunctionByName(Name);
			if (!Function)
				return asNO_FUNCTION;

			return Context->TryExecuteAsync(Function, std::move(OnArgs), std::move(OnResume));
		}
		Core::Async<int> VMCompiler::ExecuteScopedAsync(const std::string& Code, const char* Args, ArgsCallback&& OnArgs, ResumeCallback&& OnResume)
		{
			return ExecuteScopedAsync(Code.c_str(), (uint64_t)Code.size(), Args, std::move(OnArgs), std::move(OnResume));
		}
		Core::Async<int> VMCompiler::ExecuteScopedAsync(const char* Buffer, uint64_t Length, const char* Args, ArgsCallback&& OnArgs, ResumeCallback&& OnResume)
		{
			TH_ASSERT(Manager != nullptr, asINVALID_ARG, "engine should be set");
			TH_ASSERT(Buffer != nullptr && Length > 0, asINVALID_ARG, "buffer should not be empty");
			TH_ASSERT(Context != nullptr, asINVALID_ARG, "context should be set");
			TH_ASSERT(Module != nullptr, asINVALID_ARG, "module should not be empty");
			TH_ASSERT(BuiltOK, asINVALID_ARG, "module should be built");

			VMCManager* Engine = Manager->GetEngine();
			std::string Eval = "void __vfbdy(";
			if (Args != nullptr)
				Eval.append(Args);
			Eval.append("){\n");
			Eval.append(Buffer, Length);
			Eval += "\n;}";

			VMCModule* Source = GetModule().GetModule();
			VMCFunction* Function = nullptr;

			Manager->Lock();
			int R = Source->CompileFunction("__vfbdy", Eval.c_str(), -1, asCOMP_ADD_TO_MODULE, &Function);
			Manager->Unlock();

			if (R < 0)
				return R;

			Core::Async<int> Result = Context->TryExecuteAsync(Function, std::move(OnArgs), std::move(OnResume));
			Function->Release();

			return Result;
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

		VMContext::VMContext(VMCContext* Base) : Context(Base), Manager(nullptr), Promises(0), Nests(0)
		{
			TH_ASSERT_V(Base != nullptr, "context should be set");
			Manager = VMManager::Get(Base->GetEngine());
			Context->SetUserData(this, ContextUD);
			Context->SetExceptionCallback(asFUNCTION(ExceptionLogger), this, asCALL_CDECL);
		}
		VMContext::~VMContext()
		{
			TH_ASSERT_V(!IsPending(), "there are %i promises still pending", (int)Promises.size());
			while (!Queue.empty())
			{
				Queue.front().Function.Release();
				Queue.pop();
			}

			if (Context != nullptr)
			{
				if (Manager != nullptr)
					Manager->GetEngine()->ReturnContext(Context);
				else
					Context->Release();
			}
		}
		Core::Async<int> VMContext::TryExecuteAsync(const VMFunction& Function, ArgsCallback&& OnArgs, ResumeCallback&& OnResume)
		{
			Core::Async<int> Result;
			TryExecute(Function, std::move(OnArgs), [Result, OnResume = std::move(OnResume)](VMContext* Context, VMPoll State) mutable
			{
				if (OnResume)
					OnResume(Context, State);

				switch (State)
				{
					case VMPoll::Exception:
						Result = (int)VMExecState::EXCEPTION;
						break;
					case VMPoll::Finish:
						Result = (int)VMExecState::FINISHED;
						break;
					default:
						break;
				}
			});

			return Result;
		}
		bool VMContext::Dequeue(bool Unroll, int Status)
		{
			auto& OnResume = Notify[1] ? Notify[1] : Notify[0];
			asEContextState State = (asEContextState)Status;
			if ((!Unroll && Promises.empty() && !Queue.empty()) || (Unroll && !Queue.empty() && !Context->IsNested(nullptr)))
			{
				if (OnResume)
					OnResume(this, VMPoll::Routine);
				
				if (State != asEXECUTION_ACTIVE && State != asEXECUTION_SUSPENDED)
				{
					Exchange.lock();
					if (!Queue.empty())
					{
						Executable Next = std::move(Queue.front());
						Queue.pop();
						Exchange.unlock();

						TryExecute(Next.Function, std::move(Next.Args), std::move(Next.Notify));
						Next.Function.Release();
					}
					else
						Exchange.unlock();
				}
			}
			
			if (!OnResume)
				return Unroll;

			if (!Promises.empty())
				OnResume(this, VMPoll::Continue);
			else if (State == asEXECUTION_FINISHED || State == asEXECUTION_ABORTED)
				OnResume(this, VMPoll::Finish);
			else if (State == asEXECUTION_ERROR)
				OnResume(this, VMPoll::Exception);
			else if (State == asEXECUTION_EXCEPTION)
				OnResume(this, IsThrown() ? VMPoll::Exception : VMPoll::Finish);
			else
				OnResume(this, VMPoll::Continue);

			return true;
		}
		bool VMContext::Enqueue(int Status, const VMFunction& Function, ArgsCallback&& OnArgs, ResumeCallback&& OnResume)
		{
			asEContextState State = (asEContextState)Status;
			if (Promises.empty() && State != asEXECUTION_ACTIVE && State != asEXECUTION_SUSPENDED)
				return false;
			
			Executable Next;
			Next.Notify = std::move(OnResume);
			Next.Args = std::move(OnArgs);
			Next.Function = Function;
			Next.Function.AddRef();

			Exchange.lock();
			Queue.push(std::move(Next));
			Exchange.unlock();
			return true;
		}
		void VMContext::PromiseAwake(STDPromise* Base)
		{
			TH_ASSERT_V(Context != nullptr, "context should be set");
			Exchange.lock();
			if (Base != nullptr)
				Promises.insert(Base);
			Exchange.unlock();
		}
		void VMContext::PromiseSuspend(STDPromise* Base)
		{
			TH_ASSERT_V(Context != nullptr, "context should be set");
			Exchange.lock();
			if (Base != nullptr)
			{
				if (!Base->Future)
				{
					Promises.insert(Base);
					if (Context->GetState() == asEXECUTION_ACTIVE)
						Context->Suspend();
				}
				else
					Promises.erase(Base);
			}
			Exchange.unlock();
		}
		void VMContext::PromiseResume(STDPromise* Base)
		{
			Exchange.lock();
			if (Base != nullptr && Base->Future != nullptr)
			{
				auto It = Promises.find(Base);
				if (It != Promises.end())
				{
					Promises.erase(Base);
					Exchange.unlock();
					return (void)Execute();
				}
			}
			Exchange.unlock();
		}
		int VMContext::SetOnException(void(*Callback)(VMCContext* Context, void* Object), void* Object)
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetExceptionCallback(asFUNCTION(Callback), Object, asCALL_CDECL);
		}
		int VMContext::SetOnResume(const ResumeCallback& OnResume)
		{
			Notify[0] = OnResume;
			return 0;
		}
		int VMContext::Prepare(const VMFunction& Function)
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->Prepare(Function.GetFunction());
		}
		int VMContext::Unprepare()
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->Unprepare();
		}
		int VMContext::TryExecute(const VMFunction& Function, ArgsCallback&& OnArgs, ResumeCallback&& OnResume)
		{
			TH_ASSERT(Context != nullptr, asINVALID_ARG, "context should be set");
			TH_ASSERT(Function.IsValid(), asINVALID_ARG, "function should be set");

			asEContextState State = Context->GetState();
			if (Enqueue((int)State, Function, std::move(OnArgs), std::move(OnResume)))
				return asCONTEXT_ACTIVE;

			bool Nested = Context->IsNested(nullptr);
			if (Nested)
				Context->PushState();

			int Result = Context->Prepare(Function.GetFunction());
			if (Result >= 0)
			{
				if (OnArgs)
					OnArgs(this);

				Notify[1] = std::move(OnResume);
				Result = Execute();
			}

			if (Nested)
			{
				if (Result != asEXECUTION_SUSPENDED)
					Context->PopState();
				else
					Nests++;
			}

			return Result;
		}
		int VMContext::Execute(bool Resumable)
		{
			asEContextState State = Context->GetState();
			if (State != asEXECUTION_SUSPENDED && State != asEXECUTION_PREPARED)
			{
				if (State != asEXECUTION_ACTIVE && Resumable && Dequeue(false, State))
					return State;

				return asEXECUTION_ACTIVE;
			}

			State = (asEContextState)Context->Execute();
			if (Nests > 0 && State != asEXECUTION_SUSPENDED)
			{
				Context->PopState();
				Nests--;
			}

			if (Resumable && Dequeue(false, State))
				return State;
			else
				Dequeue(false, State);

			return State;
		}
		int VMContext::Abort()
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->Abort();
		}
		int VMContext::Suspend()
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->Suspend();
		}
		VMExecState VMContext::GetState() const
		{
			TH_ASSERT(Context != nullptr, VMExecState::UNINITIALIZED, "context should be set");
			return (VMExecState)Context->GetState();
		}
		std::string VMContext::GetStackTrace(size_t Skips, size_t MaxFrames) const
		{
			std::string Trace = Core::OS::GetStackTrace(Skips, MaxFrames).append("\n");
			TH_ASSERT(Context != nullptr, Trace, "context should be set");

			uint64_t ThreadId = STDThread::GetThreadId();
			std::stringstream Stream;
			Stream << "vm stack trace (most recent call last)" << (ThreadId ? " in thread " : ":\n");
			if (ThreadId)
				Stream << ThreadId << ":\n";

			for (asUINT TraceIdx = Context->GetCallstackSize(); TraceIdx-- > 0;)
			{
				VMCFunction* Function = Context->GetFunction(TraceIdx);
				if (Function != nullptr)
				{
					void* Address = (void*)(uintptr_t)Function->GetId();
					Stream << "#" << TraceIdx << "   ";

					if (Function->GetFuncType() == asFUNC_SCRIPT)
						Stream << "source \"" << (Function->GetScriptSectionName() ? Function->GetScriptSectionName() : "") << "\", line " << Context->GetLineNumber(TraceIdx) << ", in " << Function->GetDeclaration();
					else
						Stream << "source {...native_call...}, in " << Function->GetDeclaration() << " [nullptr]";

					if (Address != nullptr)
						Stream << " [0x" << Address << "]";
					else
						Stream << " [nullptr]";
				}
				else
					Stream << "source {...native_call...} [nullptr]";
				Stream << "\n";
			}

			std::string Out(Stream.str());
			return Trace + Out.substr(0, Out.size() - 1);
		}
		int VMContext::PushState()
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->PushState();
		}
		int VMContext::PopState()
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->PopState();
		}
		bool VMContext::IsNested(unsigned int* NestCount) const
		{
			TH_ASSERT(Context != nullptr, false, "context should be set");
			return Context->IsNested(NestCount);
		}
		bool VMContext::IsThrown() const
		{
			TH_ASSERT(Context != nullptr, false, "context should be set");
			const char* Exception = Context->GetExceptionString();
			if (!Exception)
				return false;

			return Exception[0] != '\0';
		}
		bool VMContext::IsPending()
		{
			Exchange.lock();
			bool Result = (!Promises.empty() || !Queue.empty());
			Exchange.unlock();

			return Result;
		}
		int VMContext::SetObject(void* Object)
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetObject(Object);
		}
		int VMContext::SetArg8(unsigned int Arg, unsigned char Value)
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetArgByte(Arg, Value);
		}
		int VMContext::SetArg16(unsigned int Arg, unsigned short Value)
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetArgWord(Arg, Value);
		}
		int VMContext::SetArg32(unsigned int Arg, int Value)
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetArgDWord(Arg, Value);
		}
		int VMContext::SetArg64(unsigned int Arg, int64_t Value)
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetArgQWord(Arg, Value);
		}
		int VMContext::SetArgFloat(unsigned int Arg, float Value)
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetArgFloat(Arg, Value);
		}
		int VMContext::SetArgDouble(unsigned int Arg, double Value)
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetArgDouble(Arg, Value);
		}
		int VMContext::SetArgAddress(unsigned int Arg, void* Address)
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetArgAddress(Arg, Address);
		}
		int VMContext::SetArgObject(unsigned int Arg, void* Object)
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetArgObject(Arg, Object);
		}
		int VMContext::SetArgAny(unsigned int Arg, void* Ptr, int TypeId)
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetArgVarType(Arg, Ptr, TypeId);
		}
		int VMContext::GetReturnableByType(void* Return, VMCTypeInfo* ReturnTypeInfo)
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			TH_ASSERT(Return != nullptr, -1, "return value should be set");
			TH_ASSERT(ReturnTypeInfo != nullptr, -1, "return type info should be set");
			TH_ASSERT(ReturnTypeInfo->GetTypeId() != (int)VMTypeId::VOIDF, -1, "return value type should not be void");

			void* Address = Context->GetAddressOfReturnValue();
			if (!Address)
				return -1;

			int TypeId = ReturnTypeInfo->GetTypeId();
			VMCManager* Engine = Manager->GetEngine();
			if (TypeId & asTYPEID_OBJHANDLE)
			{
				if (*reinterpret_cast<void**>(Return) == nullptr)
				{
					*reinterpret_cast<void**>(Return) = *reinterpret_cast<void**>(Address);
					Engine->AddRefScriptObject(*reinterpret_cast<void**>(Return), ReturnTypeInfo);
					return 0;
				}
			}
			else if (TypeId & asTYPEID_MASK_OBJECT)
				return Engine->AssignScriptObject(Return, Address, ReturnTypeInfo);

			size_t Size = Engine->GetSizeOfPrimitiveType(ReturnTypeInfo->GetTypeId());
			if (!Size)
				return -1;

			memcpy(Return, Address, Size);
			return 0;
		}
		int VMContext::GetReturnableByDecl(void* Return, const char* ReturnTypeDecl)
		{
			TH_ASSERT(ReturnTypeDecl != nullptr, -1, "return type declaration should be set");
			VMCManager* Engine = Manager->GetEngine();
			return GetReturnableByType(Return, Engine->GetTypeInfoByDecl(ReturnTypeDecl));
		}
		int VMContext::GetReturnableById(void* Return, int ReturnTypeId)
		{
			TH_ASSERT(ReturnTypeId != (int)VMTypeId::VOIDF, -1, "return value type should not be void");
			VMCManager* Engine = Manager->GetEngine();
			return GetReturnableByType(Return, Engine->GetTypeInfoById(ReturnTypeId));
		}
		void* VMContext::GetAddressOfArg(unsigned int Arg)
		{
			TH_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetAddressOfArg(Arg);
		}
		unsigned char VMContext::GetReturnByte()
		{
			TH_ASSERT(Context != nullptr, 0, "context should be set");
			return Context->GetReturnByte();
		}
		unsigned short VMContext::GetReturnWord()
		{
			TH_ASSERT(Context != nullptr, 0, "context should be set");
			return Context->GetReturnWord();
		}
		size_t VMContext::GetReturnDWord()
		{
			TH_ASSERT(Context != nullptr, 0, "context should be set");
			return Context->GetReturnDWord();
		}
		uint64_t VMContext::GetReturnQWord()
		{
			TH_ASSERT(Context != nullptr, 0, "context should be set");
			return Context->GetReturnQWord();
		}
		float VMContext::GetReturnFloat()
		{
			TH_ASSERT(Context != nullptr, 0.0f, "context should be set");
			return Context->GetReturnFloat();
		}
		double VMContext::GetReturnDouble()
		{
			TH_ASSERT(Context != nullptr, 0.0, "context should be set");
			return Context->GetReturnDouble();
		}
		void* VMContext::GetReturnAddress()
		{
			TH_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetReturnAddress();
		}
		void* VMContext::GetReturnObjectAddress()
		{
			TH_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetReturnObject();
		}
		void* VMContext::GetAddressOfReturnValue()
		{
			TH_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetAddressOfReturnValue();
		}
		int VMContext::SetException(const char* Info, bool AllowCatch)
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetException(Info, AllowCatch);
		}
		int VMContext::GetExceptionLineNumber(int* Column, const char** SectionName)
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->GetExceptionLineNumber(Column, SectionName);
		}
		VMFunction VMContext::GetExceptionFunction()
		{
			TH_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetExceptionFunction();
		}
		const char* VMContext::GetExceptionString()
		{
			TH_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetExceptionString();
		}
		bool VMContext::WillExceptionBeCaught()
		{
			TH_ASSERT(Context != nullptr, false, "context should be set");
			return Context->WillExceptionBeCaught();
		}
		void VMContext::ClearExceptionCallback()
		{
			TH_ASSERT_V(Context != nullptr, "context should be set");
			return Context->ClearExceptionCallback();
		}
		int VMContext::SetLineCallback(void(*Callback)(VMCContext* Context, void* Object), void* Object)
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			TH_ASSERT(Callback != nullptr, -1, "callback should be set");

			return Context->SetLineCallback(asFUNCTION(Callback), Object, asCALL_CDECL);
		}
		void VMContext::ClearLineCallback()
		{
			TH_ASSERT_V(Context != nullptr, "context should be set");
			return Context->ClearLineCallback();
		}
		unsigned int VMContext::GetCallstackSize() const
		{
			TH_ASSERT(Context != nullptr, 0, "context should be set");
			return Context->GetCallstackSize();
		}
		VMFunction VMContext::GetFunction(unsigned int StackLevel)
		{
			TH_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetFunction(StackLevel);
		}
		int VMContext::GetLineNumber(unsigned int StackLevel, int* Column, const char** SectionName)
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->GetLineNumber(StackLevel, Column, SectionName);
		}
		int VMContext::GetPropertiesCount(unsigned int StackLevel)
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->GetVarCount(StackLevel);
		}
		const char* VMContext::GetPropertyName(unsigned int Index, unsigned int StackLevel)
		{
			TH_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetVarName(Index, StackLevel);
		}
		const char* VMContext::GetPropertyDecl(unsigned int Index, unsigned int StackLevel, bool IncludeNamespace)
		{
			TH_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetVarDeclaration(Index, StackLevel, IncludeNamespace);
		}
		int VMContext::GetPropertyTypeId(unsigned int Index, unsigned int StackLevel)
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->GetVarTypeId(Index, StackLevel);
		}
		void* VMContext::GetAddressOfProperty(unsigned int Index, unsigned int StackLevel)
		{
			TH_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetAddressOfVar(Index, StackLevel);
		}
		bool VMContext::IsPropertyInScope(unsigned int Index, unsigned int StackLevel)
		{
			TH_ASSERT(Context != nullptr, false, "context should be set");
			return Context->IsVarInScope(Index, StackLevel);
		}
		int VMContext::GetThisTypeId(unsigned int StackLevel)
		{
			TH_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->GetThisTypeId(StackLevel);
		}
		void* VMContext::GetThisPointer(unsigned int StackLevel)
		{
			TH_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetThisPointer(StackLevel);
		}
		std::string VMContext::GetErrorStackTrace()
		{
			Except.lock();
			std::string Result = Stack;
			Except.unlock();

			return Result;
		}
		VMFunction VMContext::GetSystemFunction()
		{
			TH_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetSystemFunction();
		}
		bool VMContext::IsSuspended() const
		{
			TH_ASSERT(Context != nullptr, false, "context should be set");
			return Context->GetState() == asEXECUTION_SUSPENDED;
		}
		void* VMContext::SetUserData(void* Data, uint64_t Type)
		{
			TH_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->SetUserData(Data, Type);
		}
		void* VMContext::GetUserData(uint64_t Type) const
		{
			TH_ASSERT(Context != nullptr, nullptr, "context should be set");
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
			TH_ASSERT(Context != nullptr, nullptr, "context should be set");
			void* Manager = Context->GetUserData(ContextUD);
			return (VMContext*)Manager;
		}
		VMContext* VMContext::Get()
		{
			VMCContext* Context = asGetActiveContext();
			if (!Context)
				return nullptr;

			return Get(Context);
		}
		void VMContext::ExceptionLogger(VMCContext* Context, void*)
		{
			VMCFunction* Function = Context->GetExceptionFunction();
			VMContext* Base = VMContext::Get(Context);
			TH_ASSERT_V(Base != nullptr, "context should be set");

			const char* Message = Context->GetExceptionString();
			if (!Message || Message[0] == '\0' || Context->WillExceptionBeCaught())
				return;

			const char* Decl = Function->GetDeclaration();
			const char* Mod = Function->GetModuleName();
			const char* Source = Function->GetScriptSectionName();
			int Line = Context->GetExceptionLineNumber();
			std::string Trace = Base->GetStackTrace(3, 64);

			TH_ERR("uncaught exception"
				"\n\tdescription: %s"
				"\n\tfunction: %s"
				"\n\tmodule: %s"
				"\n\tsource: %s"
				"\n\tline: %i\n%.*s", Message ? Message : "undefined", Decl ? Decl : "undefined", Mod ? Mod : "undefined", Source ? Source : "undefined", Line, (int)Trace.size(), Trace.c_str());

			Base->Except.lock();
			Base->Stack = Trace;
			Base->Except.unlock();
		}
		int VMContext::ContextUD = 152;

		VMManager::VMManager() : Scope(0), Engine(asCreateScriptEngine()), Globals(this), Imports((uint32_t)VMImport::All), Cached(true)
		{
			asSetGlobalMemoryFunctions(Tomahawk::Core::Mem::Malloc, Tomahawk::Core::Mem::Free);
			Include.Exts.push_back(".as");
			Include.Root = Core::OS::Directory::Get();

			Engine->SetUserData(this, ManagerUD);
			Engine->SetContextCallbacks(RequestContext, ReturnContext, nullptr);
			Engine->SetMessageCallback(asFUNCTION(CompileLogger), this, asCALL_CDECL);
			Engine->SetEngineProperty(asEP_INIT_GLOBAL_VARS_AFTER_BUILD, 0);
			Engine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, 1);
			Engine->SetEngineProperty(asEP_DISALLOW_EMPTY_LIST_ELEMENTS, 1);
			Engine->SetEngineProperty(asEP_COMPILER_WARNINGS, 1);

			if (Engine->RegisterObjectType("Address", 0, asOBJ_REF | asOBJ_NOCOUNT) < 0)
				TH_ERR("could not register Address type for script engine");

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
			ClearCache();
		}
		void VMManager::SetImports(unsigned int Opts)
		{
			Imports = Opts;
		}
		void VMManager::SetCache(bool Enabled)
		{
			Cached = Enabled;
		}
		void VMManager::ClearCache()
		{
			Sync.General.lock();
			for (auto Data : Datas)
				TH_RELEASE(Data.second);

			Opcodes.clear();
			Datas.clear();
			Files.clear();
			Sync.General.unlock();
		}
		void VMManager::Lock()
		{
			Sync.General.lock();
		}
		void VMManager::Unlock()
		{
			Sync.General.unlock();
		}
		void VMManager::SetCompilerIncludeOptions(const Compute::IncludeDesc& NewDesc)
		{
			Sync.General.lock();
			Include = NewDesc;
			Sync.General.unlock();
		}
		void VMManager::SetCompilerFeatures(const Compute::Preprocessor::Desc& NewDesc)
		{
			Sync.General.lock();
			Proc = NewDesc;
			Sync.General.unlock();
		}
		void VMManager::SetProcessorOptions(Compute::Preprocessor* Processor)
		{
			TH_ASSERT_V(Processor != nullptr, "preprocessor should be set");
			Sync.General.lock();
			Processor->SetIncludeOptions(Include);
			Processor->SetFeatures(Proc);
			Sync.General.unlock();
		}
		bool VMManager::GetByteCodeCache(VMByteCode* Info)
		{
			TH_ASSERT(Info != nullptr, false, "bytecode should be set");
			if (!Cached)
				return false;

			Sync.General.lock();
			auto It = Opcodes.find(Info->Name);
			if (It == Opcodes.end())
			{
				Sync.General.unlock();
				return false;
			}

			Info->Data = It->second.Data;
			Info->Debug = It->second.Debug;
			Info->Valid = true;

			Sync.General.unlock();
			return true;
		}
		void VMManager::SetByteCodeCache(VMByteCode* Info)
		{
			TH_ASSERT_V(Info != nullptr, "bytecode should be set");
			Info->Valid = true;
			if (!Cached)
				return;

			Sync.General.lock();
			Opcodes[Info->Name] = *Info;
			Sync.General.unlock();
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
			TH_ASSERT(Engine != nullptr, nullptr, "engine should be set");
			TH_ASSERT(!Name.empty(), nullptr, "name should not be empty");

			if (AqLock)
				Sync.General.lock();

			if (!Engine->GetModule(Name.c_str()))
			{
				if (AqLock)
					Sync.General.unlock();

				return Engine->GetModule(Name.c_str(), asGM_ALWAYS_CREATE);
			}

			std::string Result;
			while (Result.size() < 1024)
			{
				Result = Name + std::to_string(Scope++);
				if (!Engine->GetModule(Result.c_str()))
				{
					if (AqLock)
						Sync.General.unlock();

					return Engine->GetModule(Result.c_str(), asGM_ALWAYS_CREATE);
				}
			}

			if (AqLock)
				Sync.General.unlock();

			return nullptr;
		}
		VMCModule* VMManager::CreateModule(const std::string& Name, bool AqLock)
		{
			TH_ASSERT(Engine != nullptr, nullptr, "engine should be set");
			TH_ASSERT(!Name.empty(), nullptr, "name should not be empty");

			if (AqLock)
				Sync.General.lock();

			VMCModule* Result = Engine->GetModule(Name.c_str(), asGM_ALWAYS_CREATE);
			if (AqLock)
				Sync.General.unlock();

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
			Sync.General.lock();
			int R = Engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DETECT_GARBAGE | asGC_DESTROY_GARBAGE, NumIterations);
			Sync.General.unlock();

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
		bool VMManager::DumpRegisteredInterfaces(const std::string& Where)
		{
			std::unordered_map<std::string, DNamespace> Namespaces;
			std::string Path = Core::OS::Path::ResolveDirectory(Where.c_str());
			Core::OS::Directory::Patch(Path);

			if (Path.empty())
				return false;

			asUINT EnumsCount = Engine->GetEnumCount();
			for (asUINT i = 0; i < EnumsCount; i++)
			{
				asITypeInfo* EType = Engine->GetEnumByIndex(i);
				const char* ENamespace = EType->GetNamespace();
				DNamespace& Namespace = Namespaces[ENamespace ? ENamespace : ""];
				DEnum& Enum = Namespace.Enums[EType->GetName()];
				asUINT ValuesCount = EType->GetEnumValueCount();

				for (asUINT j = 0; j < ValuesCount; j++)
				{
					int EValue;
					const char* EName = EType->GetEnumValueByIndex(j, &EValue);
					Enum.Values.push_back(Core::Form("%s = %i", EName ? EName : std::to_string(j).c_str(), EValue).R());
				}
			}

			asUINT ObjectsCount = Engine->GetObjectTypeCount();
			for (asUINT i = 0; i < ObjectsCount; i++)
			{
				asITypeInfo* EType = Engine->GetObjectTypeByIndex(i);
				asITypeInfo* EBase = EType->GetBaseType();
				const char* CNamespace = EType->GetNamespace();
				const char* CName = EType->GetName();
				DNamespace& Namespace = Namespaces[CNamespace ? CNamespace : ""];
				DClass& Class = Namespace.Classes[CName];
				asUINT TypesCount = EType->GetSubTypeCount();
				asUINT InterfacesCount = EType->GetInterfaceCount();
				asUINT FuncdefsCount = EType->GetChildFuncdefCount();
				asUINT PropsCount = EType->GetPropertyCount();
				asUINT FactoriesCount = EType->GetFactoryCount();
				asUINT MethodsCount = EType->GetMethodCount();

				if (EBase != nullptr)
					Class.Interfaces.push_back(GetTypeNaming(EBase));

				for (asUINT j = 0; j < InterfacesCount; j++)
				{
					asITypeInfo* IType = EType->GetInterface(j);
					Class.Interfaces.push_back(GetTypeNaming(IType));
				}

				for (asUINT j = 0; j < TypesCount; j++)
				{
					int STypeId = EType->GetSubTypeId(j);
					const char* SDecl = Engine->GetTypeDeclaration(STypeId, true);
					Class.Types.push_back(std::string("class ") + (SDecl ? SDecl : "__type__"));
				}

				for (asUINT j = 0; j < FuncdefsCount; j++)
				{
					asITypeInfo* FType = EType->GetChildFuncdef(j);
					asIScriptFunction* FFunction = FType->GetFuncdefSignature();
					const char* FNamespace = FType->GetNamespace();
					const char* FDecl = FFunction->GetDeclaration(false, false, true);
					Class.Funcdefs.push_back(std::string("funcdef ") + (FDecl ? FDecl : "void __unnamed" + std::to_string(j) + "__()"));
				}

				for (asUINT j = 0; j < PropsCount; j++)
				{
					const char* PName; int PTypeId; bool PPrivate, PProtected;
					if (EType->GetProperty(j, &PName, &PTypeId, &PPrivate, &PProtected) != 0)
						continue;

					const char* PDecl = Engine->GetTypeDeclaration(PTypeId, true);
					const char* PMod = (PPrivate ? "private " : (PProtected ? "protected " : nullptr));
					Class.Props.push_back(Core::Form("%s%s %s", PMod ? PMod : "", PDecl ? PDecl : "__type__", PName ? PName : ("__unnamed" + std::to_string(j) + "__").c_str()).R());
				}

				for (asUINT j = 0; j < FactoriesCount; j++)
				{
					asIScriptFunction* FFunction = EType->GetFactoryByIndex(j);
					const char* FDecl = FFunction->GetDeclaration(false, false, true);
					Class.Methods.push_back(FDecl ? std::string(FDecl) : "void " + std::string(CName) + "()");
				}

				for (asUINT j = 0; j < MethodsCount; j++)
				{
					asIScriptFunction* FFunction = EType->GetMethodByIndex(j);
					const char* FDecl = FFunction->GetDeclaration(false, false, true);
					Class.Methods.push_back(FDecl ? FDecl : "void __unnamed" + std::to_string(j) + "__()");
				}
			}

			asUINT FunctionsCount = Engine->GetGlobalFunctionCount();
			for (asUINT i = 0; i < FunctionsCount; i++)
			{
				asIScriptFunction* FFunction = Engine->GetGlobalFunctionByIndex(i);
				const char* FNamespace = FFunction->GetNamespace();
				const char* FDecl = FFunction->GetDeclaration(false, false, true);

				if (FNamespace != nullptr && *FNamespace != '\0')
				{
					asITypeInfo* FType = GetTypeNamespacing(Engine, FNamespace);
					if (FType != nullptr)
					{
						const char* CNamespace = FType->GetNamespace();
						const char* CName = FType->GetName();
						DNamespace& Namespace = Namespaces[CNamespace ? CNamespace : ""];
						DClass& Class = Namespace.Classes[CName];
						const char* FDecl = FFunction->GetDeclaration(false, false, true);
						Class.Functions.push_back(FDecl ? FDecl : "void __unnamed" + std::to_string(i) + "__()");
						continue;
					}
				}

				DNamespace& Namespace = Namespaces[FNamespace ? FNamespace : ""];
				Namespace.Functions.push_back(FDecl ? FDecl : "void __unnamed" + std::to_string(i) + "__()");
			}

			asUINT FuncdefsCount = Engine->GetFuncdefCount();
			for (asUINT i = 0; i < FuncdefsCount; i++)
			{
				asITypeInfo* FType = Engine->GetFuncdefByIndex(i);
				if (FType->GetParentType() != nullptr)
					continue;

				asIScriptFunction* FFunction = FType->GetFuncdefSignature();
				const char* FNamespace = FType->GetNamespace();
				DNamespace& Namespace = Namespaces[FNamespace ? FNamespace : ""];
				const char* FDecl = FFunction->GetDeclaration(false, false, true);
				Namespace.Funcdefs.push_back(std::string("funcdef ") + (FDecl ? FDecl : "void __unnamed" + std::to_string(i) + "__()"));
			}

			typedef std::pair<std::string, DNamespace*> GroupKey;
			std::unordered_map<std::string, std::pair<std::string, std::vector<GroupKey>>> Groups;
			for (auto& Namespace : Namespaces)
			{
				std::string Name = (Namespace.first.empty() ? "STD" : Namespace.first);
				std::string Subname = (Namespace.first.empty() ? "" : Name);
				auto Offset = Core::Parser(&Name).Find("::");

				if (Offset.Found)
				{
					Name = Name.substr(0, (size_t)Offset.Start);
					if (Groups.find(Name) != Groups.end())
					{
						Groups[Name].second.push_back(std::make_pair(Subname, &Namespace.second));
						continue;
					}
				}

				std::string File = Core::OS::Path::Resolve((Path + Core::Parser(Name).Replace("::", "/").ToLower().R() + ".as").c_str());
				Core::OS::Directory::Patch(Core::OS::Path::GetDirectory(File.c_str()));

				auto& Source = Groups[Name];
				Source.first = File;
				Source.second.push_back(std::make_pair(Subname, &Namespace.second));
			}

			Core::FileStream* Stream;
			for (auto& Group : Groups)
			{
				Stream = (Core::FileStream*)Core::OS::File::Open(Group.second.first, Core::FileMode::Write_Only);
				if (!Stream)
					return false;

				std::string Offset;
				std::sort(Group.second.second.begin(), Group.second.second.end(), [](const GroupKey& A, const GroupKey& B)
				{
					return A.first.size() < B.first.size();
				});

				auto& List = Group.second.second;
				for (auto It = List.begin(); It != List.end(); It++)
				{
					auto Copy = It;
					DumpNamespace(Stream, It->first, *It->second, Offset);
					if (++Copy != List.end())
						Stream->WriteAny("\n\n");
				}
				TH_RELEASE(Stream);
			}

			return true;
		}
		bool VMManager::DumpAllInterfaces(const std::string& Where)
		{
			for (auto& Item : Modules)
			{
				if (!ImportSubmodule(Item.first))
					return false;
			}

			return DumpRegisteredInterfaces(Where);
		}
		int VMManager::GetTypeNameScope(const char** TypeName, const char** Namespace, size_t* NamespaceSize) const
		{
			TH_ASSERT(TypeName != nullptr && *TypeName != nullptr, -1, "typename should be set");

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
			TH_ASSERT(GroupName != nullptr, -1, "group name should be set");
			return Engine->BeginConfigGroup(GroupName);
		}
		int VMManager::EndGroup()
		{
			return Engine->EndConfigGroup();
		}
		int VMManager::RemoveGroup(const char* GroupName)
		{
			TH_ASSERT(GroupName != nullptr, -1, "group name should be set");
			return Engine->RemoveConfigGroup(GroupName);
		}
		int VMManager::BeginNamespace(const char* Namespace)
		{
			TH_ASSERT(Namespace != nullptr, -1, "namespace name should be set");
			const char* Prev = Engine->GetDefaultNamespace();
			Sync.General.lock();
			if (Prev != nullptr)
				DefaultNamespace = Prev;
			else
				DefaultNamespace.clear();
			Sync.General.unlock();

			return Engine->SetDefaultNamespace(Namespace);
		}
		int VMManager::BeginNamespaceIsolated(const char* Namespace, size_t DefaultMask)
		{
			TH_ASSERT(Namespace != nullptr, -1, "namespace name should be set");
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
			Sync.General.lock();
			int R = Engine->SetDefaultNamespace(DefaultNamespace.c_str());
			Sync.General.unlock();

			return R;
		}
		int VMManager::Namespace(const char* Namespace, const std::function<int(VMGlobal*)>& Callback)
		{
			TH_ASSERT(Namespace != nullptr, -1, "namespace name should be set");
			TH_ASSERT(Callback, -1, "callback should not be empty");

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
			TH_ASSERT(Namespace != nullptr, -1, "namespace name should be set");
			TH_ASSERT(Callback, -1, "callback should not be empty");

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
			TH_ASSERT(Engine != nullptr, VMModule(nullptr), "engine should be set");
			TH_ASSERT(Name != nullptr, VMModule(nullptr), "name should be set");

			return VMModule(Engine->GetModule(Name, asGM_CREATE_IF_NOT_EXISTS));
		}
		int VMManager::SetProperty(VMProp Property, size_t Value)
		{
			TH_ASSERT(Engine != nullptr, -1, "engine should be set");
			return Engine->SetEngineProperty((asEEngineProp)Property, (asPWORD)Value);
		}
		void VMManager::SetDocumentRoot(const std::string& Value)
		{
			Sync.General.lock();
			Include.Root = Value;
			if (Include.Root.empty())
				return Sync.General.unlock();

			if (!Core::Parser(&Include.Root).EndsOf("/\\"))
			{
#ifdef TH_MICROSOFT
				Include.Root.append(1, '\\');
#else
				Include.Root.append(1, '/');
#endif
			}
			Sync.General.unlock();
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
		std::vector<std::string> VMManager::VerifyModules(const std::string& Directory, const Compute::RegexSource& Exp)
		{
			std::vector<std::string> Result;
			if (!Core::OS::Directory::IsExists(Directory.c_str()))
				return Result;

			std::vector<Core::ResourceEntry> Entries;
			if (!Core::OS::Directory::Scan(Directory, &Entries))
				return Result;

			Compute::RegexResult fResult;
			for (auto& Entry : Entries)
			{
				if (Directory.back() != '/' && Directory.back() != '\\')
					Entry.Path = Directory + '/' + Entry.Path;
				else
					Entry.Path = Directory + Entry.Path;

				if (!Entry.Source.IsDirectory)
				{
					if (!Compute::Regex::Match((Compute::RegexSource*)&Exp, fResult, Entry.Path))
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
			TH_ASSERT(Engine != nullptr, false, "engine should be set");
			std::string Source = Core::OS::File::ReadAsString(Path.c_str());
			if (Source.empty())
				return true;

			Sync.General.lock();
			asIScriptModule* Module = Engine->GetModule("__vfver", asGM_ALWAYS_CREATE);
			Module->AddScriptSection(Path.c_str(), Source.c_str(), Source.size());

			int R = Module->Build();
			while (R == asBUILD_IN_PROGRESS)
			{
				std::this_thread::sleep_for(std::chrono::microseconds(100));
				R = Module->Build();
			}

			Module->Discard();
			Sync.General.unlock();

			return R >= 0;
		}
		bool VMManager::IsNullable(int TypeId)
		{
			return TypeId == 0;
		}
		bool VMManager::AddSubmodule(const std::string& Name, const std::vector<std::string>& Dependencies, const SubmoduleCallback& Callback)
		{
			TH_ASSERT(!Name.empty(), false, "name should not be empty");
			if (Dependencies.empty() && !Callback)
			{
				std::string Namespace = Name + '/';
				std::vector<std::string> Deps;

				Sync.General.lock();
				for (auto& Item : Modules)
				{
					if (Core::Parser(&Item.first).StartsWith(Namespace))
						Deps.push_back(Item.first);
				}

				if (Modules.empty())
				{
					Sync.General.unlock();
					return false;
				}

				auto It = Modules.find(Name);
				if (It == Modules.end())
				{
					Submodule Result;
					Result.Dependencies = Deps;
					Modules.insert({ Name, Result });
				}
				else
				{
					It->second.Dependencies = Deps;
					It->second.Registered = false;
				}
			}
			else
			{
				Sync.General.lock();
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
					Modules.insert({ Name, Result });
				}
			}

			Sync.General.unlock();
			return true;
		}
		bool VMManager::ImportFile(const std::string& Path, std::string* Out)
		{
			if (!(Imports & (uint32_t)VMImport::Files))
			{
				TH_ERR("file import is not allowed");
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

			Sync.General.lock();
			auto It = Files.find(Path);
			if (It != Files.end())
			{
				if (Out != nullptr)
					Out->assign(It->second);

				Sync.General.unlock();
				return true;
			}

			std::string& Result = Files[Path];
			Result = Core::OS::File::ReadAsString(Path.c_str());
			if (Out != nullptr)
				Out->assign(Result);

			Sync.General.unlock();
			return true;
		}
		bool VMManager::ImportSymbol(const std::vector<std::string>& Sources, const std::string& Func, const std::string& Decl)
		{
			if (!(Imports & (uint32_t)VMImport::CSymbols))
			{
				TH_ERR("csymbols import is not allowed");
				return false;
			}

			if (!Engine || Decl.empty() || Func.empty())
				return false;

			Sync.General.lock();
			auto Core = Kernels.end();
			for (auto& Item : Sources)
			{
				Core = Kernels.find(Item);
				if (Core != Kernels.end())
					break;
			}

			if (Core == Kernels.end())
			{
				Sync.General.unlock();
				bool Failed = !ImportLibrary("");
				Sync.General.lock();

				if (Failed || (Core = Kernels.find("")) == Kernels.end())
				{
					Sync.General.unlock();
					TH_ERR("cannot load find function in any of presented libraries:\n\t%s", Func.c_str());
					return false;
				}
			}

			auto Handle = Core->second.Functions.find(Func);
			if (Handle != Core->second.Functions.end())
			{
				Sync.General.unlock();
				return true;
			}

			VMObjectFunction Function = (VMObjectFunction)Core::OS::Symbol::LoadFunction(Core->second.Handle, Func);
			if (!Function)
			{
				TH_ERR("cannot load shared object function: %s", Func.c_str());
				Sync.General.unlock();
				return false;
			}

			if (Engine->RegisterGlobalFunction(Decl.c_str(), asFUNCTION(Function), asCALL_CDECL) < 0)
			{
				TH_ERR("cannot register shared object function: %s", Decl.c_str());
				Sync.General.unlock();
				return false;
			}

			Core->second.Functions.insert({ Func, (void*)Function });
			Sync.General.unlock();

			return true;
		}
		bool VMManager::ImportLibrary(const std::string& Path)
		{
			if (!(Imports & (uint32_t)VMImport::CLibraries) && !Path.empty())
			{
				TH_ERR("clibraries import is not allowed");
				return false;
			}

			std::string Name = GetLibraryName(Path);
			if (!Engine)
				return false;

			Sync.General.lock();
			auto Core = Kernels.find(Name);
			if (Core != Kernels.end())
			{
				Sync.General.unlock();
				return true;
			}
			Sync.General.unlock();

			void* Handle = Core::OS::Symbol::Load(Path);
			if (!Handle)
			{
				TH_ERR("cannot load shared object: %s", Path.c_str());
				return false;
			}

			Kernel Library;
			Library.Handle = Handle;

			Sync.General.lock();
			Kernels.insert({ Name, Library });
			Sync.General.unlock();

			return true;
		}
		bool VMManager::ImportSubmodule(const std::string& Name)
		{
			if (!(Imports & (uint32_t)VMImport::Submodules))
			{
				TH_ERR("submodules import is not allowed");
				return false;
			}

			std::string Target = Name;
			if (Core::Parser(&Target).EndsWith(".as"))
				Target = Target.substr(0, Target.size() - 3);

			Sync.General.lock();
			auto It = Modules.find(Target);
			if (It == Modules.end())
			{
				Sync.General.unlock();
				TH_ERR("couldn't find script submodule %s", Name.c_str());
				return false;
			}

			if (It->second.Registered)
			{
				Sync.General.unlock();
				return true;
			}

			Submodule Base = It->second;
			It->second.Registered = true;
			Sync.General.unlock();

			for (auto& Item : Base.Dependencies)
			{
				if (!ImportSubmodule(Item))
				{
					TH_ERR("couldn't load submodule %s for %s", Item.c_str(), Name.c_str());
					return false;
				}
			}

			if (Base.Callback)
				Base.Callback(this);

			return true;
		}
		Core::Document* VMManager::ImportJSON(const std::string& Path)
		{
			if (!(Imports & (uint32_t)VMImport::JSON))
			{
				TH_ERR("json import is not allowed");
				return nullptr;
			}

			std::string File = Core::OS::Path::Resolve(Path, Include.Root);
			if (!Core::OS::File::IsExists(File.c_str()))
			{
				File = Core::OS::Path::Resolve(Path + ".json", Include.Root);
				if (!Core::OS::File::IsExists(File.c_str()))
				{
					TH_ERR("%s resource was not found", Path.c_str());
					return nullptr;
				}
			}

			if (!Cached)
			{
				std::string Data = Core::OS::File::ReadAsString(File.c_str());
				return Core::Document::ReadJSON(Data.c_str(), Data.size());
			}

			Sync.General.lock();
			auto It = Datas.find(File);
			if (It != Datas.end())
			{
				Core::Document* Result = It->second ? It->second->Copy() : nullptr;
				Sync.General.unlock();

				return Result;
			}

			Core::Document*& Result = Datas[File];
			std::string Data = Core::OS::File::ReadAsString(File.c_str());
			Result = Core::Document::ReadJSON(Data.c_str(), Data.size());

			Core::Document* Copy = nullptr;
			if (Result != nullptr)
				Copy = Result->Copy();

			Sync.General.unlock();
			return Copy;
		}
		size_t VMManager::GetProperty(VMProp Property) const
		{
			TH_ASSERT(Engine != nullptr, 0, "engine should be set");
			return (size_t)Engine->GetEngineProperty((asEEngineProp)Property);
		}
		VMCManager* VMManager::GetEngine() const
		{
			return Engine;
		}
		void VMManager::FreeProxy()
		{
			STDFreeCore();
		}
		VMManager* VMManager::Get(VMCManager* Engine)
		{
			TH_ASSERT(Engine != nullptr, nullptr, "engine should be set");
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

			Manager->Sync.Pool.lock();
			if (Manager->Contexts.empty())
			{
				Manager->Sync.Pool.unlock();
				return Engine->CreateContext();
			}

			VMCContext* Context = *Manager->Contexts.rbegin();
			Manager->Contexts.pop_back();
			Manager->Sync.Pool.unlock();

			return Context;
		}
		void VMManager::SetMemoryFunctions(void* (*Alloc)(size_t), void(*Free)(void*))
		{
			asSetGlobalMemoryFunctions(Alloc, Free);
		}
		void VMManager::ReturnContext(VMCManager* Engine, VMCContext* Context, void* Data)
		{
			VMManager* Manager = VMManager::Get(Engine);
			TH_ASSERT(Manager != nullptr, (void)Context->Release(), "engine should be set");

			Manager->Sync.Pool.lock();
			Manager->Contexts.push_back(Context);
			Context->Unprepare();
			Manager->Sync.Pool.unlock();
		}
		void VMManager::CompileLogger(asSMessageInfo* Info, void*)
		{
			if (Info->type == asMSGTYPE_WARNING)
				TH_WARN("\n\t%s (%i, %i): %s", Info->section && Info->section[0] != '\0' ? Info->section : "any", Info->row, Info->col, Info->message);
			else if (Info->type == asMSGTYPE_INFORMATION)
				TH_INFO("\n\t%s (%i, %i): %s", Info->section && Info->section[0] != '\0' ? Info->section : "any", Info->row, Info->col, Info->message);
			else if (Info->type == asMSGTYPE_ERROR)
				TH_ERR("\n\t%s (%i, %i): %s", Info->section && Info->section[0] != '\0' ? Info->section : "any", Info->row, Info->col, Info->message);
		}
		void VMManager::RegisterSubmodules(VMManager* Engine)
		{
			/* Standard library */
			Engine->AddSubmodule("std/any", { }, STDRegisterAny);
			Engine->AddSubmodule("std/array", { }, STDRegisterArray);
			Engine->AddSubmodule("std/complex", { }, STDRegisterComplex);
			Engine->AddSubmodule("std/grid", { }, STDRegisterGrid);
			Engine->AddSubmodule("std/ref", { }, STDRegisterRef);
			Engine->AddSubmodule("std/weakref", { }, STDRegisterWeakRef);
			Engine->AddSubmodule("std/math", { }, STDRegisterMath);
			Engine->AddSubmodule("std/string", { "std/array" }, STDRegisterString);
			Engine->AddSubmodule("std/random", { "std/string" }, STDRegisterRandom);
			Engine->AddSubmodule("std/map", { "std/array", "std/string" }, STDRegisterMap);
			Engine->AddSubmodule("std/exception", { "std/string" }, STDRegisterException);
			Engine->AddSubmodule("std/mutex", { }, STDRegisterMutex);
			Engine->AddSubmodule("std/thread", { "std/any" }, STDRegisterThread);
			Engine->AddSubmodule("std/promise", { "std/any" }, STDRegisterPromise);
			Engine->AddSubmodule("std", { }, nullptr);

			/* Core library */
			Engine->AddSubmodule("ce/format", { "std/string" }, CERegisterFormat);
			Engine->AddSubmodule("ce/decimal", { "std/string" }, CERegisterDecimal);
			Engine->AddSubmodule("ce/variant", { "std/string" }, CERegisterVariant);
			Engine->AddSubmodule("ce/filestate", { }, CERegisterFileState);
			Engine->AddSubmodule("ce/resource", { }, CERegisterResource);
			Engine->AddSubmodule("ce/datetime", { "std/string" }, CERegisterDateTime);
			Engine->AddSubmodule("ce/ticker", { }, CERegisterTicker);
			Engine->AddSubmodule("ce/os", { "std/string", "ce/filestate", "ce/resource" }, CERegisterOS);
			Engine->AddSubmodule("ce/console", { "ce/format" }, CERegisterConsole);
			Engine->AddSubmodule("ce/timer", { }, CERegisterTimer);
			Engine->AddSubmodule("ce/filestream", { "std/string" }, CERegisterFileStream);
			Engine->AddSubmodule("ce/gzstream", { "std/string", "ce/filestream" }, CERegisterGzStream);
			Engine->AddSubmodule("ce/webstream", { "std/string", "ce/filestream" }, CERegisterWebStream);
			Engine->AddSubmodule("ce/schedule", { "std/string" }, CERegisterSchedule);
			Engine->AddSubmodule("ce/document", { "std/array", "std/string", "std/map", "ce/variant" }, CERegisterDocument);
			Engine->AddSubmodule("ce", { }, nullptr);

			/* Compute library */
			Engine->AddSubmodule("cu/vertices", { }, CURegisterVertices);
			Engine->AddSubmodule("cu/rectangle", { }, CURegisterRectangle);
			Engine->AddSubmodule("cu/vector2", { }, CURegisterVector2);
			Engine->AddSubmodule("cu/vector3", { "cu/vector2" }, CURegisterVector3);
			Engine->AddSubmodule("cu/vector4", { "cu/vector3" }, CURegisterVector4);
			Engine->AddSubmodule("cu", { }, nullptr);

			/* GUI library */
			Engine->AddSubmodule("gui/variant", { "std/string", "cu/vector2", "cu/vector3", "cu/vector4" }, GUIRegisterVariant);
			Engine->AddSubmodule("gui/control", { "gui/variant", "ce/document", "std/array" }, GUIRegisterControl);
			Engine->AddSubmodule("gui/model", { "gui/control", }, GUIRegisterModel);
			Engine->AddSubmodule("gui/context", { "gui/model" }, GUIRegisterContext);
			Engine->AddSubmodule("gui", { }, nullptr);
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

		VMDebugger::VMDebugger() : LastFunction(nullptr), Manager(nullptr), Action(DebugAction::CONTINUE)
		{
			LastCommandAtStackLevel = 0;
		}
		VMDebugger::~VMDebugger()
		{
			SetEngine(0);
		}
		void VMDebugger::RegisterToStringCallback(const VMTypeInfo& Type, ToStringCallback Callback)
		{
			TH_ASSERT_V(ToStringCallbacks.find(Type.GetTypeInfo()) == ToStringCallbacks.end(), "callback should not already be set");
			ToStringCallbacks.insert(std::unordered_map<const VMCTypeInfo*, ToStringCallback>::value_type(Type.GetTypeInfo(), Callback));
		}
		void VMDebugger::LineCallback(VMContext* Context)
		{
			TH_ASSERT_V(Context != nullptr, "context should be set");
			VMCContext* Base = Context->GetContext();

			TH_ASSERT_V(Base != nullptr, "context should be set");
			TH_ASSERT_V(Base->GetState() == asEXECUTION_ACTIVE, "context should be active");

			if (Action == DebugAction::CONTINUE)
			{
				if (!CheckBreakPoint(Context))
					return;
			}
			else if (Action == DebugAction::STEP_OVER)
			{
				if (Base->GetCallstackSize() > LastCommandAtStackLevel)
				{
					if (!CheckBreakPoint(Context))
						return;
				}
			}
			else if (Action == DebugAction::STEP_OUT)
			{
				if (Base->GetCallstackSize() >= LastCommandAtStackLevel)
				{
					if (!CheckBreakPoint(Context))
						return;
				}
			}
			else if (Action == DebugAction::STEP_INTO)
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
			TH_ASSERT_V(Context != nullptr, "context should be set");

			VMCContext* Base = Context->GetContext();
			TH_ASSERT_V(Base != nullptr, "context should be set");

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
			TH_ASSERT_V(Context != nullptr, "context should be set");

			VMCContext* Base = Context->GetContext();
			TH_ASSERT_V(Base != nullptr, "context should be set");

			VMCManager* Engine = Context->GetManager()->GetEngine();
			std::string Text = Expression, Scope, Name;
			asUINT Length = 0;

			asETokenClass T = Engine->ParseToken(Text.c_str(), 0, &Length);
			while (T == asTC_IDENTIFIER || (T == asTC_KEYWORD && Length == 2 && Text.compare("::") != 0))
			{
				if (T == asTC_KEYWORD)
				{
					if (Scope.empty() && Name.empty())
						Scope = "::";
					else if (Scope == "::" || Scope.empty())
						Scope = Name;
					else
						Scope += "::" + Name;

					Name.clear();
				}
				else
					Name.assign(Text.c_str(), Length);

				Text = Text.substr(Length);
				T = Engine->ParseToken(Text.c_str(), 0, &Length);
			}

			if (Name.empty())
			{
				Output("Invalid expression. Expected identifier\n");
				return;
			}

			void* Pointer = 0;
			int TypeId = 0;

			VMCFunction* Function = Base->GetFunction();
			if (!Function)
				return;

			if (Scope.empty())
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
					Scope.clear();

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
			TH_ASSERT_V(Context != nullptr, "context should be set");

			VMCContext* Base = Context->GetContext();
			TH_ASSERT_V(Base != nullptr, "context should be set");

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
			TH_ASSERT_V(Context != nullptr, "context should be set");

			VMCContext* Base = Context->GetContext();
			TH_ASSERT_V(Base != nullptr, "context should be set");

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
			TH_ASSERT_V(Context != nullptr, "context should be set");

			VMCContext* Base = Context->GetContext();
			TH_ASSERT_V(Base != nullptr, "context should be set");

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
			TH_ASSERT_V(Context != nullptr, "context should be set");

			VMCContext* Base = Context->GetContext();
			TH_ASSERT_V(Base != nullptr, "context should be set");

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
			TH_ASSERT_V(Context != nullptr, "context should be set");

			VMCContext* Base = Context->GetContext();
			TH_ASSERT_V(Base != nullptr, "context should be set");

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
				Manager->GetEngine()->AddRef();
			}
		}
		bool VMDebugger::CheckBreakPoint(VMContext* Context)
		{
			TH_ASSERT(Context != nullptr, false, "context should be set");

			VMCContext* Base = Context->GetContext();
			TH_ASSERT(Base != nullptr, false, "context should be set");

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
			TH_ASSERT(Context != nullptr, false, "context should be set");

			VMCContext* Base = Context->GetContext();
			TH_ASSERT(Base != nullptr, false, "context should be set");

			if (Command.length() == 0)
				return true;

			switch (Command[0])
			{
				case 'c':
					Action = DebugAction::CONTINUE;
					break;
				case 's':
					Action = DebugAction::STEP_INTO;
					break;
				case 'n':
					Action = DebugAction::STEP_OVER;
					LastCommandAtStackLevel = Base->GetCallstackSize();
					break;
				case 'o':
					Action = DebugAction::STEP_OUT;
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