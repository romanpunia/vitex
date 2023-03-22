#include "scripting.h"
#include "bindings.h"
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
			ED_ASSERT(Ptr && Size, 0, "corrupted read");
			memcpy(Ptr, &Code[ReadPos], Size);
			ReadPos += Size;

			return 0;
		}
		int Write(const void* Ptr, asUINT Size)
		{
			ED_ASSERT(Ptr && Size, 0, "corrupted write");
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
	void DumpNamespace(Edge::Core::FileStream* Stream, const std::string& Naming, DNamespace& Namespace, std::string& Offset)
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

namespace Edge
{
	namespace Scripting
	{
		static bool GenerateSourceCode(Compute::Preprocessor* Base, const std::string& Path, std::string& Buffer)
		{
			if (!Base->Process(Path, Buffer))
				return false;

			return Bindings::Registry::MakePostprocess(Buffer);
		}

		int FunctionFactory::AtomicNotifyGC(const char* TypeName, void* Object)
		{
			ED_ASSERT(TypeName != nullptr, -1, "typename should be set");
			ED_ASSERT(Object != nullptr, -1, "object should be set");

			asIScriptContext* Context = asGetActiveContext();
			ED_ASSERT(Context != nullptr, -1, "context should be set");

			VirtualMachine* Engine = VirtualMachine::Get(Context->GetEngine());
			ED_ASSERT(Engine != nullptr, -1, "engine should be set");

			TypeInfo Type = Engine->GetTypeInfoByName(TypeName);
			return Engine->NotifyOfNewObject(Object, Type.GetTypeInfo());
		}
		asSFuncPtr* FunctionFactory::CreateFunctionBase(void(*Base)(), int Type)
		{
			ED_ASSERT(Base != nullptr, nullptr, "function pointer should be set");
			asSFuncPtr* Ptr = ED_NEW(asSFuncPtr, Type);
			Ptr->ptr.f.func = reinterpret_cast<asFUNCTION_t>(Base);
			return Ptr;
		}
		asSFuncPtr* FunctionFactory::CreateMethodBase(const void* Base, size_t Size, int Type)
		{
			ED_ASSERT(Base != nullptr, nullptr, "function pointer should be set");
			asSFuncPtr* Ptr = ED_NEW(asSFuncPtr, Type);
			Ptr->CopyMethodPtr(Base, Size);
			return Ptr;
		}
		asSFuncPtr* FunctionFactory::CreateDummyBase()
		{
			return ED_NEW(asSFuncPtr, 0);
		}
		void FunctionFactory::ReleaseFunctor(asSFuncPtr** Ptr)
		{
			if (!Ptr || !*Ptr)
				return;

			ED_DELETE(asSFuncPtr, *Ptr);
			*Ptr = nullptr;
		}

		MessageInfo::MessageInfo(asSMessageInfo* Msg) noexcept : Info(Msg)
		{
		}
		const char* MessageInfo::GetSection() const
		{
			ED_ASSERT(IsValid(), nullptr, "message should be valid");
			return Info->section;
		}
		const char* MessageInfo::GetText() const
		{
			ED_ASSERT(IsValid(), nullptr, "message should be valid");
			return Info->message;
		}
		LogCategory MessageInfo::GetType() const
		{
			ED_ASSERT(IsValid(), LogCategory::INFORMATION, "message should be valid");
			return (LogCategory)Info->type;
		}
		int MessageInfo::GetRow() const
		{
			ED_ASSERT(IsValid(), -1, "message should be valid");
			return Info->row;
		}
		int MessageInfo::GetColumn() const
		{
			ED_ASSERT(IsValid(), -1, "message should be valid");
			return Info->col;
		}
		asSMessageInfo* MessageInfo::GetMessageInfo() const
		{
			return Info;
		}
		bool MessageInfo::IsValid() const
		{
			return Info != nullptr;
		}

		TypeInfo::TypeInfo(asITypeInfo* TypeInfo) noexcept : Info(TypeInfo)
		{
			VM = (Info ? VirtualMachine::Get(Info->GetEngine()) : nullptr);
		}
		void TypeInfo::ForEachProperty(const PropertyCallback& Callback)
		{
			ED_ASSERT_V(IsValid(), "typeinfo should be valid");
			ED_ASSERT_V(Callback, "typeinfo should not be empty");

			unsigned int Count = Info->GetPropertyCount();
			for (unsigned int i = 0; i < Count; i++)
			{
				FunctionInfo Prop;
				if (GetProperty(i, &Prop) >= 0)
					Callback(this, &Prop);
			}
		}
		void TypeInfo::ForEachMethod(const MethodCallback& Callback)
		{
			ED_ASSERT_V(IsValid(), "typeinfo should be valid");
			ED_ASSERT_V(Callback, "typeinfo should not be empty");

			unsigned int Count = Info->GetMethodCount();
			for (unsigned int i = 0; i < Count; i++)
			{
				Function Method = Info->GetMethodByIndex(i);
				if (Method.IsValid())
					Callback(this, &Method);
			}
		}
		const char* TypeInfo::GetGroup() const
		{
			ED_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetConfigGroup();
		}
		size_t TypeInfo::GetAccessMask() const
		{
			ED_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetAccessMask();
		}
		Module TypeInfo::GetModule() const
		{
			ED_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetModule();
		}
		int TypeInfo::AddRef() const
		{
			ED_ASSERT(IsValid(), -1, "typeinfo should be valid");
			return Info->AddRef();
		}
		int TypeInfo::Release()
		{
			if (!IsValid())
				return -1;

			int R = Info->Release();
			if (R <= 0)
				Info = nullptr;

			return R;
		}
		const char* TypeInfo::GetName() const
		{
			ED_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetName();
		}
		const char* TypeInfo::GetNamespace() const
		{
			ED_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetNamespace();
		}
		TypeInfo TypeInfo::GetBaseType() const
		{
			ED_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetBaseType();
		}
		bool TypeInfo::DerivesFrom(const TypeInfo& Type) const
		{
			ED_ASSERT(IsValid(), false, "typeinfo should be valid");
			return Info->DerivesFrom(Type.GetTypeInfo());
		}
		size_t TypeInfo::GetFlags() const
		{
			ED_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetFlags();
		}
		unsigned int TypeInfo::GetSize() const
		{
			ED_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetSize();
		}
		int TypeInfo::GetTypeId() const
		{
			ED_ASSERT(IsValid(), -1, "typeinfo should be valid");
			return Info->GetTypeId();
		}
		int TypeInfo::GetSubTypeId(unsigned int SubTypeIndex) const
		{
			ED_ASSERT(IsValid(), -1, "typeinfo should be valid");
			return Info->GetSubTypeId(SubTypeIndex);
		}
		TypeInfo TypeInfo::GetSubType(unsigned int SubTypeIndex) const
		{
			ED_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetSubType(SubTypeIndex);
		}
		unsigned int TypeInfo::GetSubTypeCount() const
		{
			ED_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetSubTypeCount();
		}
		unsigned int TypeInfo::GetInterfaceCount() const
		{
			ED_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetInterfaceCount();
		}
		TypeInfo TypeInfo::GetInterface(unsigned int Index) const
		{
			ED_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetInterface(Index);
		}
		bool TypeInfo::Implements(const TypeInfo& Type) const
		{
			ED_ASSERT(IsValid(), false, "typeinfo should be valid");
			return Info->Implements(Type.GetTypeInfo());
		}
		unsigned int TypeInfo::GetFactoriesCount() const
		{
			ED_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetFactoryCount();
		}
		Function TypeInfo::GetFactoryByIndex(unsigned int Index) const
		{
			ED_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetFactoryByIndex(Index);
		}
		Function TypeInfo::GetFactoryByDecl(const char* Decl) const
		{
			ED_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetFactoryByDecl(Decl);
		}
		unsigned int TypeInfo::GetMethodsCount() const
		{
			ED_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetMethodCount();
		}
		Function TypeInfo::GetMethodByIndex(unsigned int Index, bool GetVirtual) const
		{
			ED_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetMethodByIndex(Index, GetVirtual);
		}
		Function TypeInfo::GetMethodByName(const char* Name, bool GetVirtual) const
		{
			ED_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetMethodByName(Name, GetVirtual);
		}
		Function TypeInfo::GetMethodByDecl(const char* Decl, bool GetVirtual) const
		{
			ED_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetMethodByDecl(Decl, GetVirtual);
		}
		unsigned int TypeInfo::GetPropertiesCount() const
		{
			ED_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetPropertyCount();
		}
		int TypeInfo::GetProperty(unsigned int Index, FunctionInfo* Out) const
		{
			ED_ASSERT(IsValid(), -1, "typeinfo should be valid");

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
		const char* TypeInfo::GetPropertyDeclaration(unsigned int Index, bool IncludeNamespace) const
		{
			ED_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetPropertyDeclaration(Index, IncludeNamespace);
		}
		unsigned int TypeInfo::GetBehaviourCount() const
		{
			ED_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetBehaviourCount();
		}
		Function TypeInfo::GetBehaviourByIndex(unsigned int Index, Behaviours* OutBehaviour) const
		{
			ED_ASSERT(IsValid(), nullptr, "typeinfo should be valid");

			asEBehaviours Out;
			asIScriptFunction* Result = Info->GetBehaviourByIndex(Index, &Out);
			if (OutBehaviour != nullptr)
				*OutBehaviour = (Behaviours)Out;

			return Result;
		}
		unsigned int TypeInfo::GetChildFunctionDefCount() const
		{
			ED_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetChildFuncdefCount();
		}
		TypeInfo TypeInfo::GetChildFunctionDef(unsigned int Index) const
		{
			ED_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetChildFuncdef(Index);
		}
		TypeInfo TypeInfo::GetParentType() const
		{
			ED_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetParentType();
		}
		unsigned int TypeInfo::GetEnumValueCount() const
		{
			ED_ASSERT(IsValid(), 0, "typeinfo should be valid");
			return Info->GetEnumValueCount();
		}
		const char* TypeInfo::GetEnumValueByIndex(unsigned int Index, int* OutValue) const
		{
			ED_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetEnumValueByIndex(Index, OutValue);
		}
		Function TypeInfo::GetFunctionDefSignature() const
		{
			ED_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetFuncdefSignature();
		}
		void* TypeInfo::SetUserData(void* Data, size_t Type)
		{
			ED_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->SetUserData(Data, Type);
		}
		void* TypeInfo::GetUserData(size_t Type) const
		{
			ED_ASSERT(IsValid(), nullptr, "typeinfo should be valid");
			return Info->GetUserData(Type);
		}
		bool TypeInfo::IsHandle() const
		{
			ED_ASSERT(IsValid(), false, "typeinfo should be valid");
			return IsHandle(Info->GetTypeId());
		}
		bool TypeInfo::IsValid() const
		{
			return VM != nullptr && Info != nullptr;
		}
		asITypeInfo* TypeInfo::GetTypeInfo() const
		{
			return Info;
		}
		VirtualMachine* TypeInfo::GetVM() const
		{
			return VM;
		}
		bool TypeInfo::IsHandle(int TypeId)
		{
			return (TypeId & asTYPEID_OBJHANDLE || TypeId & asTYPEID_HANDLETOCONST);
		}
		bool TypeInfo::IsScriptObject(int TypeId)
		{
			return (TypeId & asTYPEID_SCRIPTOBJECT);
		}

		Function::Function(asIScriptFunction* Base) noexcept : Ptr(Base)
		{
			VM = (Base ? VirtualMachine::Get(Base->GetEngine()) : nullptr);
		}
		Function::Function(const Function& Base) noexcept : VM(Base.VM), Ptr(Base.Ptr)
		{
		}
		int Function::AddRef() const
		{
			ED_ASSERT(IsValid(), -1, "function should be valid");
			return Ptr->AddRef();
		}
		int Function::Release()
		{
			if (!IsValid())
				return -1;

			int R = Ptr->Release();
			if (R <= 0)
				Ptr = nullptr;

			return R;
		}
		int Function::GetId() const
		{
			ED_ASSERT(IsValid(), -1, "function should be valid");
			return Ptr->GetId();
		}
		FunctionType Function::GetType() const
		{
			ED_ASSERT(IsValid(), FunctionType::DUMMY, "function should be valid");
			return (FunctionType)Ptr->GetFuncType();
		}
		const char* Function::GetModuleName() const
		{
			ED_ASSERT(IsValid(), nullptr, "function should be valid");
			return Ptr->GetModuleName();
		}
		Module Function::GetModule() const
		{
			ED_ASSERT(IsValid(), nullptr, "function should be valid");
			return Ptr->GetModule();
		}
		const char* Function::GetSectionName() const
		{
			ED_ASSERT(IsValid(), nullptr, "function should be valid");
			return Ptr->GetScriptSectionName();
		}
		const char* Function::GetGroup() const
		{
			ED_ASSERT(IsValid(), nullptr, "function should be valid");
			return Ptr->GetConfigGroup();
		}
		size_t Function::GetAccessMask() const
		{
			ED_ASSERT(IsValid(), -1, "function should be valid");
			return Ptr->GetAccessMask();
		}
		TypeInfo Function::GetObjectType() const
		{
			ED_ASSERT(IsValid(), nullptr, "function should be valid");
			return Ptr->GetObjectType();
		}
		const char* Function::GetObjectName() const
		{
			ED_ASSERT(IsValid(), nullptr, "function should be valid");
			return Ptr->GetObjectName();
		}
		const char* Function::GetName() const
		{
			ED_ASSERT(IsValid(), nullptr, "function should be valid");
			return Ptr->GetName();
		}
		const char* Function::GetNamespace() const
		{
			ED_ASSERT(IsValid(), nullptr, "function should be valid");
			return Ptr->GetNamespace();
		}
		const char* Function::GetDecl(bool IncludeObjectName, bool IncludeNamespace, bool IncludeArgNames) const
		{
			ED_ASSERT(IsValid(), nullptr, "function should be valid");
			return Ptr->GetDeclaration(IncludeObjectName, IncludeNamespace, IncludeArgNames);
		}
		bool Function::IsReadOnly() const
		{
			ED_ASSERT(IsValid(), false, "function should be valid");
			return Ptr->IsReadOnly();
		}
		bool Function::IsPrivate() const
		{
			ED_ASSERT(IsValid(), false, "function should be valid");
			return Ptr->IsPrivate();
		}
		bool Function::IsProtected() const
		{
			ED_ASSERT(IsValid(), false, "function should be valid");
			return Ptr->IsProtected();
		}
		bool Function::IsFinal() const
		{
			ED_ASSERT(IsValid(), false, "function should be valid");
			return Ptr->IsFinal();
		}
		bool Function::IsOverride() const
		{
			ED_ASSERT(IsValid(), false, "function should be valid");
			return Ptr->IsOverride();
		}
		bool Function::IsShared() const
		{
			ED_ASSERT(IsValid(), false, "function should be valid");
			return Ptr->IsShared();
		}
		bool Function::IsExplicit() const
		{
			ED_ASSERT(IsValid(), false, "function should be valid");
			return Ptr->IsExplicit();
		}
		bool Function::IsProperty() const
		{
			ED_ASSERT(IsValid(), false, "function should be valid");
			return Ptr->IsProperty();
		}
		unsigned int Function::GetArgsCount() const
		{
			ED_ASSERT(IsValid(), 0, "function should be valid");
			return Ptr->GetParamCount();
		}
		int Function::GetArg(unsigned int Index, int* TypeId, size_t* Flags, const char** Name, const char** DefaultArg) const
		{
			ED_ASSERT(IsValid(), -1, "function should be valid");

			asDWORD asFlags;
			int R = Ptr->GetParam(Index, TypeId, &asFlags, Name, DefaultArg);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;

			return R;
		}
		int Function::GetReturnTypeId(size_t* Flags) const
		{
			ED_ASSERT(IsValid(), -1, "function should be valid");

			asDWORD asFlags;
			int R = Ptr->GetReturnTypeId(&asFlags);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;

			return R;
		}
		int Function::GetTypeId() const
		{
			ED_ASSERT(IsValid(), -1, "function should be valid");
			return Ptr->GetTypeId();
		}
		bool Function::IsCompatibleWithTypeId(int TypeId) const
		{
			ED_ASSERT(IsValid(), false, "function should be valid");
			return Ptr->IsCompatibleWithTypeId(TypeId);
		}
		void* Function::GetDelegateObject() const
		{
			ED_ASSERT(IsValid(), nullptr, "function should be valid");
			return Ptr->GetDelegateObject();
		}
		TypeInfo Function::GetDelegateObjectType() const
		{
			ED_ASSERT(IsValid(), nullptr, "function should be valid");
			return Ptr->GetDelegateObjectType();
		}
		Function Function::GetDelegateFunction() const
		{
			ED_ASSERT(IsValid(), nullptr, "function should be valid");
			return Ptr->GetDelegateFunction();
		}
		unsigned int Function::GetPropertiesCount() const
		{
			ED_ASSERT(IsValid(), 0, "function should be valid");
			return Ptr->GetVarCount();
		}
		int Function::GetProperty(unsigned int Index, const char** Name, int* TypeId) const
		{
			ED_ASSERT(IsValid(), -1, "function should be valid");
			return Ptr->GetVar(Index, Name, TypeId);
		}
		const char* Function::GetPropertyDecl(unsigned int Index, bool IncludeNamespace) const
		{
			ED_ASSERT(IsValid(), nullptr, "function should be valid");
			return Ptr->GetVarDecl(Index, IncludeNamespace);
		}
		int Function::FindNextLineWithCode(int Line) const
		{
			ED_ASSERT(IsValid(), -1, "function should be valid");
			return Ptr->FindNextLineWithCode(Line);
		}
		void* Function::SetUserData(void* UserData, size_t Type)
		{
			ED_ASSERT(IsValid(), nullptr, "function should be valid");
			return Ptr->SetUserData(UserData, Type);
		}
		void* Function::GetUserData(size_t Type) const
		{
			ED_ASSERT(IsValid(), nullptr, "function should be valid");
			return Ptr->GetUserData(Type);
		}
		bool Function::IsValid() const
		{
			return VM != nullptr && Ptr != nullptr;
		}
		asIScriptFunction* Function::GetFunction() const
		{
			return Ptr;
		}
		VirtualMachine* Function::GetVM() const
		{
			return VM;
		}

		ScriptObject::ScriptObject(asIScriptObject* Base) noexcept : Object(Base)
		{
		}
		int ScriptObject::AddRef() const
		{
			ED_ASSERT(IsValid(), 0, "object should be valid");
			return Object->AddRef();
		}
		int ScriptObject::Release()
		{
			if (!IsValid())
				return -1;

			int R = Object->Release();
			Object = nullptr;

			return R;
		}
		TypeInfo ScriptObject::GetObjectType()
		{
			ED_ASSERT(IsValid(), nullptr, "object should be valid");
			return Object->GetObjectType();
		}
		int ScriptObject::GetTypeId()
		{
			ED_ASSERT(IsValid(), 0, "object should be valid");
			return Object->GetTypeId();
		}
		int ScriptObject::GetPropertyTypeId(unsigned int Id) const
		{
			ED_ASSERT(IsValid(), 0, "object should be valid");
			return Object->GetPropertyTypeId(Id);
		}
		unsigned int ScriptObject::GetPropertiesCount() const
		{
			ED_ASSERT(IsValid(), 0, "object should be valid");
			return Object->GetPropertyCount();
		}
		const char* ScriptObject::GetPropertyName(unsigned int Id) const
		{
			ED_ASSERT(IsValid(), nullptr, "object should be valid");
			return Object->GetPropertyName(Id);
		}
		void* ScriptObject::GetAddressOfProperty(unsigned int Id)
		{
			ED_ASSERT(IsValid(), nullptr, "object should be valid");
			return Object->GetAddressOfProperty(Id);
		}
		VirtualMachine* ScriptObject::GetVM() const
		{
			ED_ASSERT(IsValid(), nullptr, "object should be valid");
			return VirtualMachine::Get(Object->GetEngine());
		}
		int ScriptObject::CopyFrom(const ScriptObject& Other)
		{
			ED_ASSERT(IsValid(), -1, "object should be valid");
			return Object->CopyFrom(Other.GetObject());
		}
		void* ScriptObject::SetUserData(void* Data, size_t Type)
		{
			ED_ASSERT(IsValid(), nullptr, "object should be valid");
			return Object->SetUserData(Data, (asPWORD)Type);
		}
		void* ScriptObject::GetUserData(size_t Type) const
		{
			ED_ASSERT(IsValid(), nullptr, "object should be valid");
			return Object->GetUserData((asPWORD)Type);
		}
		bool ScriptObject::IsValid() const
		{
			return Object != nullptr;
		}
		asIScriptObject* ScriptObject::GetObject() const
		{
			return Object;
		}

		GenericContext::GenericContext(asIScriptGeneric* Base) noexcept : Generic(Base)
		{
			VM = (Generic ? VirtualMachine::Get(Generic->GetEngine()) : nullptr);
		}
		void* GenericContext::GetObjectAddress()
		{
			ED_ASSERT(IsValid(), nullptr, "generic should be valid");
			return Generic->GetObject();
		}
		int GenericContext::GetObjectTypeId() const
		{
			ED_ASSERT(IsValid(), -1, "generic should be valid");
			return Generic->GetObjectTypeId();
		}
		int GenericContext::GetArgsCount() const
		{
			ED_ASSERT(IsValid(), -1, "generic should be valid");
			return Generic->GetArgCount();
		}
		int GenericContext::GetArgTypeId(unsigned int Argument, size_t* Flags) const
		{
			ED_ASSERT(IsValid(), -1, "generic should be valid");

			asDWORD asFlags;
			int R = Generic->GetArgTypeId(Argument, &asFlags);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;

			return R;
		}
		unsigned char GenericContext::GetArgByte(unsigned int Argument)
		{
			ED_ASSERT(IsValid(), 0, "generic should be valid");
			return Generic->GetArgByte(Argument);
		}
		unsigned short GenericContext::GetArgWord(unsigned int Argument)
		{
			ED_ASSERT(IsValid(), 0, "generic should be valid");
			return Generic->GetArgWord(Argument);
		}
		size_t GenericContext::GetArgDWord(unsigned int Argument)
		{
			ED_ASSERT(IsValid(), 0, "generic should be valid");
			return Generic->GetArgDWord(Argument);
		}
		uint64_t GenericContext::GetArgQWord(unsigned int Argument)
		{
			ED_ASSERT(IsValid(), 0, "generic should be valid");
			return Generic->GetArgQWord(Argument);
		}
		float GenericContext::GetArgFloat(unsigned int Argument)
		{
			ED_ASSERT(IsValid(), 0.0f, "generic should be valid");
			return Generic->GetArgFloat(Argument);
		}
		double GenericContext::GetArgDouble(unsigned int Argument)
		{
			ED_ASSERT(IsValid(), 0.0, "generic should be valid");
			return Generic->GetArgDouble(Argument);
		}
		void* GenericContext::GetArgAddress(unsigned int Argument)
		{
			ED_ASSERT(IsValid(), nullptr, "generic should be valid");
			return Generic->GetArgAddress(Argument);
		}
		void* GenericContext::GetArgObjectAddress(unsigned int Argument)
		{
			ED_ASSERT(IsValid(), nullptr, "generic should be valid");
			return Generic->GetArgObject(Argument);
		}
		void* GenericContext::GetAddressOfArg(unsigned int Argument)
		{
			ED_ASSERT(IsValid(), nullptr, "generic should be valid");
			return Generic->GetAddressOfArg(Argument);
		}
		int GenericContext::GetReturnTypeId(size_t* Flags) const
		{
			ED_ASSERT(IsValid(), -1, "generic should be valid");

			asDWORD asFlags;
			int R = Generic->GetReturnTypeId(&asFlags);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;

			return R;
		}
		int GenericContext::SetReturnByte(unsigned char Value)
		{
			ED_ASSERT(IsValid(), -1, "generic should be valid");
			return Generic->SetReturnByte(Value);
		}
		int GenericContext::SetReturnWord(unsigned short Value)
		{
			ED_ASSERT(IsValid(), -1, "generic should be valid");
			return Generic->SetReturnWord(Value);
		}
		int GenericContext::SetReturnDWord(size_t Value)
		{
			ED_ASSERT(IsValid(), -1, "generic should be valid");
			return Generic->SetReturnDWord((asDWORD)Value);
		}
		int GenericContext::SetReturnQWord(uint64_t Value)
		{
			ED_ASSERT(IsValid(), -1, "generic should be valid");
			return Generic->SetReturnQWord(Value);
		}
		int GenericContext::SetReturnFloat(float Value)
		{
			ED_ASSERT(IsValid(), -1, "generic should be valid");
			return Generic->SetReturnFloat(Value);
		}
		int GenericContext::SetReturnDouble(double Value)
		{
			ED_ASSERT(IsValid(), -1, "generic should be valid");
			return Generic->SetReturnDouble(Value);
		}
		int GenericContext::SetReturnAddress(void* Address)
		{
			ED_ASSERT(IsValid(), -1, "generic should be valid");
			return Generic->SetReturnAddress(Address);
		}
		int GenericContext::SetReturnObjectAddress(void* Object)
		{
			ED_ASSERT(IsValid(), -1, "generic should be valid");
			return Generic->SetReturnObject(Object);
		}
		void* GenericContext::GetAddressOfReturnLocation()
		{
			ED_ASSERT(IsValid(), nullptr, "generic should be valid");
			return Generic->GetAddressOfReturnLocation();
		}
		bool GenericContext::IsValid() const
		{
			return VM != nullptr && Generic != nullptr;
		}
		asIScriptGeneric* GenericContext::GetGeneric() const
		{
			return Generic;
		}
		VirtualMachine* GenericContext::GetVM() const
		{
			return VM;
		}

		BaseClass::BaseClass(VirtualMachine* Engine, const std::string& Name, int Type) noexcept : VM(Engine), Object(Name), TypeId(Type)
		{
		}
		int BaseClass::SetFunctionDef(const char* Decl)
		{
			ED_ASSERT(IsValid(), -1, "class should be valid");
			ED_ASSERT(Decl != nullptr, -1, "declaration should be set");

			asIScriptEngine* Engine = VM->GetEngine();
			ED_ASSERT(Engine != nullptr, -1, "engine should be set");
			ED_TRACE("[vm] register class 0x%" PRIXPTR " funcdef %i bytes", (void*)this, (int)strlen(Decl));

			return Engine->RegisterFuncdef(Decl);
		}
		int BaseClass::SetOperatorCopyAddress(asSFuncPtr* Value, FunctionCall Type)
		{
			ED_ASSERT(IsValid(), -1, "class should be valid");
			ED_ASSERT(Value != nullptr, -1, "value should be set");

			asIScriptEngine* Engine = VM->GetEngine();
			ED_ASSERT(Engine != nullptr, -1, "engine should be set");

			Core::String Decl = Core::Form("%s& opAssign(const %s &in)", Object.c_str(), Object.c_str());
			ED_TRACE("[vm] register class 0x%" PRIXPTR " op-copy funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)Type, (int)Decl.Size(), (void*)Value);
			return Engine->RegisterObjectMethod(Object.c_str(), Decl.Get(), *Value, (asECallConvTypes)Type);
		}
		int BaseClass::SetBehaviourAddress(const char* Decl, Behaviours Behave, asSFuncPtr* Value, FunctionCall Type)
		{
			ED_ASSERT(IsValid(), -1, "class should be valid");
			ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
			ED_ASSERT(Value != nullptr, -1, "value should be set");

			asIScriptEngine* Engine = VM->GetEngine();
			ED_ASSERT(Engine != nullptr, -1, "engine should be set");
			ED_TRACE("[vm] register class 0x%" PRIXPTR " behaviour funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)Type, (int)strlen(Decl), (void*)Value);

			return Engine->RegisterObjectBehaviour(Object.c_str(), (asEBehaviours)Behave, Decl, *Value, (asECallConvTypes)Type);
		}
		int BaseClass::SetPropertyAddress(const char* Decl, int Offset)
		{
			ED_ASSERT(IsValid(), -1, "class should be valid");
			ED_ASSERT(Decl != nullptr, -1, "declaration should be set");

			asIScriptEngine* Engine = VM->GetEngine();
			ED_ASSERT(Engine != nullptr, -1, "engine should be set");
			ED_TRACE("[vm] register class 0x%" PRIXPTR " property %i bytes at 0x0+%i", (void*)this, (int)strlen(Decl), Offset);

			return Engine->RegisterObjectProperty(Object.c_str(), Decl, Offset);
		}
		int BaseClass::SetPropertyStaticAddress(const char* Decl, void* Value)
		{
			ED_ASSERT(IsValid(), -1, "class should be valid");
			ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
			ED_ASSERT(Value != nullptr, -1, "value should be set");
			ED_TRACE("[vm] register class 0x%" PRIXPTR " static property %i bytes at 0x%" PRIXPTR, (void*)this,  (int)strlen(Decl), (void*)Value);

			asIScriptEngine* Engine = VM->GetEngine();
			ED_ASSERT(Engine != nullptr, -1, "engine should be set");

			asITypeInfo* Info = Engine->GetTypeInfoByName(Object.c_str());
			const char* Namespace = Engine->GetDefaultNamespace();
			const char* Scope = Info->GetNamespace();

			Engine->SetDefaultNamespace(Scope[0] == '\0' ? Object.c_str() : std::string(Scope).append("::").append(Object).c_str());
			int R = Engine->RegisterGlobalProperty(Decl, Value);
			Engine->SetDefaultNamespace(Namespace);

			return R;
		}
		int BaseClass::SetOperatorAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type)
		{
			return SetMethodAddress(Decl, Value, Type);
		}
		int BaseClass::SetMethodAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type)
		{
			ED_ASSERT(IsValid(), -1, "class should be valid");
			ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
			ED_ASSERT(Value != nullptr, -1, "value should be set");

			asIScriptEngine* Engine = VM->GetEngine();
			ED_ASSERT(Engine != nullptr, -1, "engine should be set");
			ED_TRACE("[vm] register class 0x%" PRIXPTR " funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)Type, (int)strlen(Decl), (void*)Value);

			return Engine->RegisterObjectMethod(Object.c_str(), Decl, *Value, (asECallConvTypes)Type);
		}
		int BaseClass::SetMethodStaticAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type)
		{
			ED_ASSERT(IsValid(), -1, "class should be valid");
			ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
			ED_ASSERT(Value != nullptr, -1, "value should be set");

			asIScriptEngine* Engine = VM->GetEngine();
			ED_ASSERT(Engine != nullptr, -1, "engine should be set");
			ED_TRACE("[vm] register class 0x%" PRIXPTR " static funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)Type, (int)strlen(Decl), (void*)Value);

			asITypeInfo* Info = Engine->GetTypeInfoByName(Object.c_str());
			const char* Namespace = Engine->GetDefaultNamespace();
			const char* Scope = Info->GetNamespace();

			Engine->SetDefaultNamespace(Scope[0] == '\0' ? Object.c_str() : std::string(Scope).append("::").append(Object).c_str());
			int R = Engine->RegisterGlobalFunction(Decl, *Value, (asECallConvTypes)Type);
			Engine->SetDefaultNamespace(Namespace);

			return R;

		}
		int BaseClass::SetConstructorAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type)
		{
			ED_ASSERT(IsValid(), -1, "class should be valid");
			ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
			ED_ASSERT(Value != nullptr, -1, "value should be set");
			ED_TRACE("[vm] register class 0x%" PRIXPTR " constructor funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)Type, (int)strlen(Decl), (void*)Value);

			asIScriptEngine* Engine = VM->GetEngine();
			ED_ASSERT(Engine != nullptr, -1, "engine should be set");

			return Engine->RegisterObjectBehaviour(Object.c_str(), asBEHAVE_CONSTRUCT, Decl, *Value, (asECallConvTypes)Type);
		}
		int BaseClass::SetConstructorListAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type)
		{
			ED_ASSERT(IsValid(), -1, "class should be valid");
			ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
			ED_ASSERT(Value != nullptr, -1, "value should be set");
			ED_TRACE("[vm] register class 0x%" PRIXPTR " list-constructor funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)Type, (int)strlen(Decl), (void*)Value);

			asIScriptEngine* Engine = VM->GetEngine();
			ED_ASSERT(Engine != nullptr, -1, "engine should be set");

			return Engine->RegisterObjectBehaviour(Object.c_str(), asBEHAVE_LIST_CONSTRUCT, Decl, *Value, (asECallConvTypes)Type);
		}
		int BaseClass::SetDestructorAddress(const char* Decl, asSFuncPtr* Value)
		{
			ED_ASSERT(IsValid(), -1, "class should be valid");
			ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
			ED_ASSERT(Value != nullptr, -1, "value should be set");
			ED_TRACE("[vm] register class 0x%" PRIXPTR " destructor funcaddr %i bytes at 0x%" PRIXPTR, (void*)this, (int)strlen(Decl), (void*)Value);

			asIScriptEngine* Engine = VM->GetEngine();
			ED_ASSERT(Engine != nullptr, -1, "engine should be set");

			return Engine->RegisterObjectBehaviour(Object.c_str(), asBEHAVE_DESTRUCT, Decl, *Value, asCALL_CDECL_OBJFIRST);
		}
		int BaseClass::GetTypeId() const
		{
			return TypeId;
		}
		bool BaseClass::IsValid() const
		{
			return VM != nullptr && TypeId >= 0;
		}
		std::string BaseClass::GetName() const
		{
			return Object;
		}
		VirtualMachine* BaseClass::GetVM() const
		{
			return VM;
		}
		Core::String BaseClass::GetOperator(Operators Op, const char* Out, const char* Args, bool Const, bool Right)
		{
			switch (Op)
			{
				case Operators::Neg:
					if (Right)
						return "";

					return Core::Form("%s opNeg()%s", Out, Const ? " const" : "");
				case Operators::Com:
					if (Right)
						return "";

					return Core::Form("%s opCom()%s", Out, Const ? " const" : "");
				case Operators::PreInc:
					if (Right)
						return "";

					return Core::Form("%s opPreInc()%s", Out, Const ? " const" : "");
				case Operators::PreDec:
					if (Right)
						return "";

					return Core::Form("%s opPreDec()%s", Out, Const ? " const" : "");
				case Operators::PostInc:
					if (Right)
						return "";

					return Core::Form("%s opPostInc()%s", Out, Const ? " const" : "");
				case Operators::PostDec:
					if (Right)
						return "";

					return Core::Form("%s opPostDec()%s", Out, Const ? " const" : "");
				case Operators::Equals:
					if (Right)
						return "";

					return Core::Form("%s opEquals(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::Cmp:
					if (Right)
						return "";

					return Core::Form("%s opCmp(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::Assign:
					if (Right)
						return "";

					return Core::Form("%s opAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::AddAssign:
					if (Right)
						return "";

					return Core::Form("%s opAddAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::SubAssign:
					if (Right)
						return "";

					return Core::Form("%s opSubAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::MulAssign:
					if (Right)
						return "";

					return Core::Form("%s opMulAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::DivAssign:
					if (Right)
						return "";

					return Core::Form("%s opDivAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::ModAssign:
					if (Right)
						return "";

					return Core::Form("%s opModAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::PowAssign:
					if (Right)
						return "";

					return Core::Form("%s opPowAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::AndAssign:
					if (Right)
						return "";

					return Core::Form("%s opAndAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::OrAssign:
					if (Right)
						return "";

					return Core::Form("%s opOrAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::XOrAssign:
					if (Right)
						return "";

					return Core::Form("%s opXorAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::ShlAssign:
					if (Right)
						return "";

					return Core::Form("%s opShlAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::ShrAssign:
					if (Right)
						return "";

					return Core::Form("%s opShrAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::UshrAssign:
					if (Right)
						return "";

					return Core::Form("%s opUshrAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::Add:
					return Core::Form("%s opAdd%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::Sub:
					return Core::Form("%s opSub%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::Mul:
					return Core::Form("%s opMul%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::Div:
					return Core::Form("%s opDiv%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::Mod:
					return Core::Form("%s opMod%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::Pow:
					return Core::Form("%s opPow%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::And:
					return Core::Form("%s opAnd%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::Or:
					return Core::Form("%s opOr%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::XOr:
					return Core::Form("%s opXor%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::Shl:
					return Core::Form("%s opShl%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::Shr:
					return Core::Form("%s opShr%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::Ushr:
					return Core::Form("%s opUshr%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::Index:
					if (Right)
						return "";

					return Core::Form("%s opIndex(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::Call:
					if (Right)
						return "";

					return Core::Form("%s opCall(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::Cast:
					if (Right)
						return "";

					return Core::Form("%s opCast(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::ImplCast:
					if (Right)
						return "";

					return Core::Form("%s opImplCast(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				default:
					return "";
			}
		}

		TypeInterface::TypeInterface(VirtualMachine* Engine, const std::string& Name, int Type) noexcept : VM(Engine), Object(Name), TypeId(Type)
		{
		}
		int TypeInterface::SetMethod(const char* Decl)
		{
			ED_ASSERT(IsValid(), -1, "interface should be valid");
			ED_ASSERT(Decl != nullptr, -1, "declaration should be set");

			asIScriptEngine* Engine = VM->GetEngine();
			ED_ASSERT(Engine != nullptr, -1, "engine should be set");
			ED_TRACE("[vm] register interface 0x%" PRIXPTR " method %i bytes", (void*)this, (int)strlen(Decl));

			return Engine->RegisterInterfaceMethod(Object.c_str(), Decl);
		}
		int TypeInterface::GetTypeId() const
		{
			return TypeId;
		}
		bool TypeInterface::IsValid() const
		{
			return VM != nullptr && TypeId >= 0;
		}
		std::string TypeInterface::GetName() const
		{
			return Object;
		}
		VirtualMachine* TypeInterface::GetVM() const
		{
			return VM;
		}

		Enumeration::Enumeration(VirtualMachine* Engine, const std::string& Name, int Type) noexcept : VM(Engine), Object(Name), TypeId(Type)
		{
		}
		int Enumeration::SetValue(const char* Name, int Value)
		{
			ED_ASSERT(IsValid(), -1, "enum should be valid");
			ED_ASSERT(Name != nullptr, -1, "name should be set");

			asIScriptEngine* Engine = VM->GetEngine();
			ED_ASSERT(Engine != nullptr, -1, "engine should be set");
			ED_TRACE("[vm] register enum 0x%" PRIXPTR " value %i bytes = %i", (void*)this, (int)strlen(Name), Value);

			return Engine->RegisterEnumValue(Object.c_str(), Name, Value);
		}
		int Enumeration::GetTypeId() const
		{
			return TypeId;
		}
		bool Enumeration::IsValid() const
		{
			return VM != nullptr && TypeId >= 0;
		}
		std::string Enumeration::GetName() const
		{
			return Object;
		}
		VirtualMachine* Enumeration::GetVM() const
		{
			return VM;
		}

		Module::Module(asIScriptModule* Type) noexcept : Mod(Type)
		{
			VM = (Mod ? VirtualMachine::Get(Mod->GetEngine()) : nullptr);
		}
		void Module::SetName(const char* Name)
		{
			ED_ASSERT_V(IsValid(), "module should be valid");
			ED_ASSERT_V(Name != nullptr, "name should be set");
			return Mod->SetName(Name);
		}
		int Module::AddSection(const char* Name, const char* Code, size_t CodeLength, int LineOffset)
		{
			ED_ASSERT(IsValid(), -1, "module should be valid");
			ED_ASSERT(Name != nullptr, -1, "name should be set");
			ED_ASSERT(Code != nullptr, -1, "code should be set");
			return Mod->AddScriptSection(Name, Code, CodeLength, LineOffset);
		}
		int Module::RemoveFunction(const Function& Function)
		{
			ED_ASSERT(IsValid(), -1, "module should be valid");
			return Mod->RemoveFunction(Function.GetFunction());
		}
		int Module::ResetProperties(asIScriptContext* Context)
		{
			ED_ASSERT(IsValid(), -1, "module should be valid");
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Mod->ResetGlobalVars(Context);
		}
		int Module::Build()
		{
			ED_ASSERT(IsValid(), -1, "module should be valid");
			return Mod->Build();
		}
		int Module::LoadByteCode(ByteCodeInfo* Info)
		{
			ED_ASSERT(IsValid(), -1, "module should be valid");
			ED_ASSERT(Info != nullptr, -1, "bytecode should be set");

			CByteCodeStream* Stream = ED_NEW(CByteCodeStream, Info->Data);
			int R = Mod->LoadByteCode(Stream, &Info->Debug);
			ED_DELETE(CByteCodeStream, Stream);

			return R;
		}
		int Module::Discard()
		{
			ED_ASSERT(IsValid(), -1, "module should be valid");
			Mod->Discard();
			Mod = nullptr;

			return 0;
		}
		int Module::BindImportedFunction(size_t ImportIndex, const Function& Function)
		{
			ED_ASSERT(IsValid(), -1, "module should be valid");
			return Mod->BindImportedFunction((asUINT)ImportIndex, Function.GetFunction());
		}
		int Module::UnbindImportedFunction(size_t ImportIndex)
		{
			ED_ASSERT(IsValid(), -1, "module should be valid");
			return Mod->UnbindImportedFunction((asUINT)ImportIndex);
		}
		int Module::BindAllImportedFunctions()
		{
			ED_ASSERT(IsValid(), -1, "module should be valid");
			return Mod->BindAllImportedFunctions();
		}
		int Module::UnbindAllImportedFunctions()
		{
			ED_ASSERT(IsValid(), -1, "module should be valid");
			return Mod->UnbindAllImportedFunctions();
		}
		int Module::CompileFunction(const char* SectionName, const char* Code, int LineOffset, size_t CompileFlags, Function* OutFunction)
		{
			ED_ASSERT(IsValid(), -1, "module should be valid");
			ED_ASSERT(SectionName != nullptr, -1, "section name should be set");
			ED_ASSERT(Code != nullptr, -1, "code should be set");

			asIScriptFunction* OutFunc = nullptr;
			int R = Mod->CompileFunction(SectionName, Code, LineOffset, (asDWORD)CompileFlags, &OutFunc);
			if (OutFunction != nullptr)
				*OutFunction = Function(OutFunc);

			return R;
		}
		int Module::CompileProperty(const char* SectionName, const char* Code, int LineOffset)
		{
			ED_ASSERT(IsValid(), -1, "module should be valid");
			return Mod->CompileGlobalVar(SectionName, Code, LineOffset);
		}
		int Module::SetDefaultNamespace(const char* Namespace)
		{
			ED_ASSERT(IsValid(), -1, "module should be valid");
			return Mod->SetDefaultNamespace(Namespace);
		}
		void* Module::GetAddressOfProperty(size_t Index)
		{
			ED_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetAddressOfGlobalVar((asUINT)Index);
		}
		int Module::RemoveProperty(size_t Index)
		{
			ED_ASSERT(IsValid(), -1, "module should be valid");
			return Mod->RemoveGlobalVar((asUINT)Index);
		}
		size_t Module::SetAccessMask(size_t AccessMask)
		{
			ED_ASSERT(IsValid(), 0, "module should be valid");
			return Mod->SetAccessMask((asDWORD)AccessMask);
		}
		size_t Module::GetFunctionCount() const
		{
			ED_ASSERT(IsValid(), 0, "module should be valid");
			return Mod->GetFunctionCount();
		}
		Function Module::GetFunctionByIndex(size_t Index) const
		{
			ED_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetFunctionByIndex((asUINT)Index);
		}
		Function Module::GetFunctionByDecl(const char* Decl) const
		{
			ED_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetFunctionByDecl(Decl);
		}
		Function Module::GetFunctionByName(const char* Name) const
		{
			ED_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetFunctionByName(Name);
		}
		int Module::GetTypeIdByDecl(const char* Decl) const
		{
			ED_ASSERT(IsValid(), -1, "module should be valid");
			return Mod->GetTypeIdByDecl(Decl);
		}
		int Module::GetImportedFunctionIndexByDecl(const char* Decl) const
		{
			ED_ASSERT(IsValid(), -1, "module should be valid");
			return Mod->GetImportedFunctionIndexByDecl(Decl);
		}
		int Module::SaveByteCode(ByteCodeInfo* Info) const
		{
			ED_ASSERT(IsValid(), -1, "module should be valid");
			ED_ASSERT(Info != nullptr, -1, "bytecode should be set");

			CByteCodeStream* Stream = ED_NEW(CByteCodeStream);
			int R = Mod->SaveByteCode(Stream, Info->Debug);
			Info->Data = Stream->GetCode();
			ED_DELETE(CByteCodeStream, Stream);

			return R;
		}
		int Module::GetPropertyIndexByName(const char* Name) const
		{
			ED_ASSERT(IsValid(), -1, "module should be valid");
			ED_ASSERT(Name != nullptr, -1, "name should be set");

			return Mod->GetGlobalVarIndexByName(Name);
		}
		int Module::GetPropertyIndexByDecl(const char* Decl) const
		{
			ED_ASSERT(IsValid(), -1, "module should be valid");
			ED_ASSERT(Decl != nullptr, -1, "declaration should be set");

			return Mod->GetGlobalVarIndexByDecl(Decl);
		}
		int Module::GetProperty(size_t Index, PropertyInfo* Info) const
		{
			ED_ASSERT(IsValid(), -1, "module should be valid");
			const char* Name = nullptr;
			const char* Namespace = nullptr;
			bool IsConst = false;
			int TypeId = 0;
			int Result = Mod->GetGlobalVar((asUINT)Index, &Name, &Namespace, &TypeId, &IsConst);

			if (Info != nullptr)
			{
				Info->Name = Name;
				Info->Namespace = Namespace;
				Info->TypeId = TypeId;
				Info->IsConst = IsConst;
				Info->ConfigGroup = nullptr;
				Info->Pointer = Mod->GetAddressOfGlobalVar((asUINT)Index);
				Info->AccessMask = GetAccessMask();
			}

			return Result;
		}
		size_t Module::GetAccessMask() const
		{
			ED_ASSERT(IsValid(), 0, "module should be valid");
			asDWORD AccessMask = Mod->SetAccessMask(0);
			Mod->SetAccessMask(AccessMask);
			return (size_t)AccessMask;
		}
		size_t Module::GetObjectsCount() const
		{
			ED_ASSERT(IsValid(), 0, "module should be valid");
			return Mod->GetObjectTypeCount();
		}
		size_t Module::GetPropertiesCount() const
		{
			ED_ASSERT(IsValid(), 0, "module should be valid");
			return Mod->GetGlobalVarCount();
		}
		size_t Module::GetEnumsCount() const
		{
			ED_ASSERT(IsValid(), 0, "module should be valid");
			return Mod->GetEnumCount();
		}
		size_t Module::GetImportedFunctionCount() const
		{
			ED_ASSERT(IsValid(), 0, "module should be valid");
			return Mod->GetImportedFunctionCount();
		}
		TypeInfo Module::GetObjectByIndex(size_t Index) const
		{
			ED_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetObjectTypeByIndex((asUINT)Index);
		}
		TypeInfo Module::GetTypeInfoByName(const char* Name) const
		{
			ED_ASSERT(IsValid(), nullptr, "module should be valid");
			const char* TypeName = Name;
			const char* Namespace = nullptr;
			size_t NamespaceSize = 0;

			if (VM->GetTypeNameScope(&TypeName, &Namespace, &NamespaceSize) != 0)
				return Mod->GetTypeInfoByName(Name);

			VM->BeginNamespace(std::string(Namespace, NamespaceSize).c_str());
			asITypeInfo* Info = Mod->GetTypeInfoByName(TypeName);
			VM->EndNamespace();

			return Info;
		}
		TypeInfo Module::GetTypeInfoByDecl(const char* Decl) const
		{
			ED_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetTypeInfoByDecl(Decl);
		}
		TypeInfo Module::GetEnumByIndex(size_t Index) const
		{
			ED_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetEnumByIndex((asUINT)Index);
		}
		const char* Module::GetPropertyDecl(size_t Index, bool IncludeNamespace) const
		{
			ED_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetGlobalVarDeclaration((asUINT)Index, IncludeNamespace);
		}
		const char* Module::GetDefaultNamespace() const
		{
			ED_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetDefaultNamespace();
		}
		const char* Module::GetImportedFunctionDecl(size_t ImportIndex) const
		{
			ED_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetImportedFunctionDeclaration((asUINT)ImportIndex);
		}
		const char* Module::GetImportedFunctionModule(size_t ImportIndex) const
		{
			ED_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetImportedFunctionSourceModule((asUINT)ImportIndex);
		}
		const char* Module::GetName() const
		{
			ED_ASSERT(IsValid(), nullptr, "module should be valid");
			return Mod->GetName();
		}
		bool Module::IsValid() const
		{
			return VM != nullptr && Mod != nullptr;
		}
		asIScriptModule* Module::GetModule() const
		{
			return Mod;
		}
		VirtualMachine* Module::GetVM() const
		{
			return VM;
		}

		Compiler::Compiler(VirtualMachine* Engine) noexcept : Scope(nullptr), VM(Engine), Context(nullptr), BuiltOK(false)
		{
			ED_ASSERT_V(VM != nullptr, "engine should be set");

			Processor = new Compute::Preprocessor();
			Processor->SetIncludeCallback([this](Compute::Preprocessor* Processor, const Compute::IncludeResult& File, std::string* Out)
			{
				ED_ASSERT(VM != nullptr, false, "engine should be set");
				if (Include && Include(Processor, File, Out))
					return true;

				if (File.Module.empty() || !Scope)
					return false;

				if (!File.IsFile && File.IsSystem)
					return VM->ImportSubmodule(File.Module);

				std::string Buffer;
				if (!VM->ImportFile(File.Module, &Buffer))
					return false;

				if (!GenerateSourceCode(Processor, File.Module, Buffer))
					return false;

				return Scope->AddScriptSection(File.Module.c_str(), Buffer.c_str(), Buffer.size()) >= 0;
			});
			Processor->SetPragmaCallback([this](Compute::Preprocessor* Processor, const std::string& Name, const std::vector<std::string>& Args)
			{
				ED_ASSERT(VM != nullptr, false, "engine should be set");
				if (Pragma && Pragma(Processor, Name, Args))
					return true;

				if (Name == "compile" && Args.size() == 2)
				{
					const std::string& Key = Args[0];
					Core::String Value(&Args[1]);

					size_t Result = Value.HasInteger() ? (size_t)Value.ToUInt64() : 0;
					if (Key == "ALLOW_UNSAFE_REFERENCES")
						VM->SetProperty(Features::ALLOW_UNSAFE_REFERENCES, Result);
					else if (Key == "OPTIMIZE_BYTECODE")
						VM->SetProperty(Features::OPTIMIZE_BYTECODE, Result);
					else if (Key == "COPY_SCRIPT_SECTIONS")
						VM->SetProperty(Features::COPY_SCRIPT_SECTIONS, Result);
					else if (Key == "MAX_STACK_SIZE")
						VM->SetProperty(Features::MAX_STACK_SIZE, Result);
					else if (Key == "USE_CHARACTER_LITERALS")
						VM->SetProperty(Features::USE_CHARACTER_LITERALS, Result);
					else if (Key == "ALLOW_MULTILINE_STRINGS")
						VM->SetProperty(Features::ALLOW_MULTILINE_STRINGS, Result);
					else if (Key == "ALLOW_IMPLICIT_HANDLE_TYPES")
						VM->SetProperty(Features::ALLOW_IMPLICIT_HANDLE_TYPES, Result);
					else if (Key == "BUILD_WITHOUT_LINE_CUES")
						VM->SetProperty(Features::BUILD_WITHOUT_LINE_CUES, Result);
					else if (Key == "INIT_GLOBAL_VARS_AFTER_BUILD")
						VM->SetProperty(Features::INIT_GLOBAL_VARS_AFTER_BUILD, Result);
					else if (Key == "REQUIRE_ENUM_SCOPE")
						VM->SetProperty(Features::REQUIRE_ENUM_SCOPE, Result);
					else if (Key == "SCRIPT_SCANNER")
						VM->SetProperty(Features::SCRIPT_SCANNER, Result);
					else if (Key == "INCLUDE_JIT_INSTRUCTIONS")
						VM->SetProperty(Features::INCLUDE_JIT_INSTRUCTIONS, Result);
					else if (Key == "STRING_ENCODING")
						VM->SetProperty(Features::STRING_ENCODING, Result);
					else if (Key == "PROPERTY_ACCESSOR_MODE")
						VM->SetProperty(Features::PROPERTY_ACCESSOR_MODE, Result);
					else if (Key == "EXPAND_DEF_ARRAY_TO_TMPL")
						VM->SetProperty(Features::EXPAND_DEF_ARRAY_TO_TMPL, Result);
					else if (Key == "AUTO_GARBAGE_COLLECT")
						VM->SetProperty(Features::AUTO_GARBAGE_COLLECT, Result);
					else if (Key == "DISALLOW_GLOBAL_VARS")
						VM->SetProperty(Features::ALWAYS_IMPL_DEFAULT_CONSTRUCT, Result);
					else if (Key == "ALWAYS_IMPL_DEFAULT_CONSTRUCT")
						VM->SetProperty(Features::ALWAYS_IMPL_DEFAULT_CONSTRUCT, Result);
					else if (Key == "COMPILER_WARNINGS")
						VM->SetProperty(Features::COMPILER_WARNINGS, Result);
					else if (Key == "DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE")
						VM->SetProperty(Features::DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE, Result);
					else if (Key == "ALTER_SYNTAX_NAMED_ARGS")
						VM->SetProperty(Features::ALTER_SYNTAX_NAMED_ARGS, Result);
					else if (Key == "DISABLE_INTEGER_DIVISION")
						VM->SetProperty(Features::DISABLE_INTEGER_DIVISION, Result);
					else if (Key == "DISALLOW_EMPTY_LIST_ELEMENTS")
						VM->SetProperty(Features::DISALLOW_EMPTY_LIST_ELEMENTS, Result);
					else if (Key == "PRIVATE_PROP_AS_PROTECTED")
						VM->SetProperty(Features::PRIVATE_PROP_AS_PROTECTED, Result);
					else if (Key == "ALLOW_UNICODE_IDENTIFIERS")
						VM->SetProperty(Features::ALLOW_UNICODE_IDENTIFIERS, Result);
					else if (Key == "HEREDOC_TRIM_MODE")
						VM->SetProperty(Features::HEREDOC_TRIM_MODE, Result);
					else if (Key == "MAX_NESTED_CALLS")
						VM->SetProperty(Features::MAX_NESTED_CALLS, Result);
					else if (Key == "GENERIC_CALL_MODE")
						VM->SetProperty(Features::GENERIC_CALL_MODE, Result);
					else if (Key == "INIT_STACK_SIZE")
						VM->SetProperty(Features::INIT_STACK_SIZE, Result);
					else if (Key == "INIT_CALL_STACK_SIZE")
						VM->SetProperty(Features::INIT_CALL_STACK_SIZE, Result);
					else if (Key == "MAX_CALL_STACK_SIZE")
						VM->SetProperty(Features::MAX_CALL_STACK_SIZE, Result);
				}
				else if (Name == "comment" && Args.size() == 2)
				{
					const std::string& Key = Args[0];
					if (Key == "INFO")
						ED_INFO("[compiler] %s", Args[1].c_str());
					else if (Key == "TRACE")
						ED_DEBUG("[compiler] %s", Args[1].c_str());
					else if (Key == "WARN")
						ED_WARN("[compiler] %s", Args[1].c_str());
					else if (Key == "ERR")
						ED_ERR("[compiler] %s", Args[1].c_str());
				}
				else if (Name == "modify" && Args.size() == 2)
				{
					const std::string& Key = Args[0];
					Core::String Value(&Args[1]);

					size_t Result = Value.HasInteger() ? (size_t)Value.ToUInt64() : 0;
					if (Key == "NAME")
						Scope->SetName(Value.Get());
					else if (Key == "NAMESPACE")
						Scope->SetDefaultNamespace(Value.Get());
					else if (Key == "ACCESS_MASK")
						Scope->SetAccessMask((asDWORD)Result);
				}
				else if (Name == "cimport" && Args.size() >= 2)
				{
					bool Loaded;
					if (Args.size() == 3)
						Loaded = VM->ImportLibrary(Args[0]) && VM->ImportSymbol({ Args[0] }, Args[1], Args[2]);
					else
						Loaded = VM->ImportSymbol({ }, Args[0], Args[1]);

					if (Loaded)
						Define("SOF_" + Args[1]);
				}
				else if (Name == "clibrary" && Args.size() >= 1)
				{
					std::string Directory = Core::OS::Path::GetDirectory(Processor->GetCurrentFilePath().c_str());
					std::string Path1 = Args[0], Path2 = Core::OS::Path::Resolve(Args[0], Directory.empty() ? Core::OS::Directory::Get() : Directory);

					bool Loaded = VM->ImportLibrary(Path1) || VM->ImportLibrary(Path2);
					if (Loaded && Args.size() == 2 && !Args[1].empty())
						Define("SOL_" + Args[1]);
				}
				else if (Name == "define" && Args.size() == 1)
					Define(Args[0]);

				return true;
			});
#ifdef ED_MICROSOFT
			Processor->Define("OS_MICROSOFT");
#elif defined(ED_APPLE)
			Processor->Define("OS_APPLE");
#elif defined(ED_UNIX)
			Processor->Define("OS_UNIX");
#endif
			Context = VM->CreateContext();
			Context->SetUserData(this, CompilerUD);
			VM->SetProcessorOptions(Processor);
		}
		Compiler::~Compiler() noexcept
		{
			ED_RELEASE(Context);
			ED_RELEASE(Processor);
		}
		void Compiler::SetIncludeCallback(const Compute::ProcIncludeCallback& Callback)
		{
			Include = Callback;
		}
		void Compiler::SetPragmaCallback(const Compute::ProcPragmaCallback& Callback)
		{
			Pragma = Callback;
		}
		void Compiler::Define(const std::string& Word)
		{
			Processor->Define(Word);
		}
		void Compiler::Undefine(const std::string& Word)
		{
			Processor->Undefine(Word);
		}
		bool Compiler::Clear()
		{
			ED_ASSERT(VM != nullptr, false, "engine should be set");
			if (Context != nullptr && Context->IsPending())
				return false;

			if (Scope != nullptr)
				Scope->Discard();

			if (Processor != nullptr)
				Processor->Clear();

			Scope = nullptr;
			BuiltOK = false;
			return true;
		}
		bool Compiler::IsDefined(const std::string& Word) const
		{
			return Processor->IsDefined(Word.c_str());
		}
		bool Compiler::IsBuilt() const
		{
			return BuiltOK;
		}
		bool Compiler::IsCached() const
		{
			return VCache.Valid;
		}
		int Compiler::Prepare(ByteCodeInfo* Info)
		{
			ED_ASSERT(VM != nullptr, -1, "engine should be set");
			ED_ASSERT(Info != nullptr, -1, "bytecode should be set");

			if (!Info->Valid || Info->Data.empty())
				return -1;

			int Result = Prepare(Info->Name, true);
			if (Result < 0)
				return Result;

			VCache = *Info;
			return Result;
		}
		int Compiler::Prepare(const std::string& ModuleName, bool Scoped)
		{
			ED_ASSERT(VM != nullptr, -1, "engine should be set");
			ED_ASSERT(!ModuleName.empty(), -1, "module name should not be empty");
			ED_DEBUG("[vm] prepare %s on 0x%" PRIXPTR, ModuleName.c_str(), (uintptr_t)this);

			BuiltOK = false;
			VCache.Valid = false;
			VCache.Name.clear();

			if (Scope != nullptr)
				Scope->Discard();

			if (Scoped)
				Scope = VM->CreateScopedModule(ModuleName);
			else
				Scope = VM->CreateModule(ModuleName);

			if (!Scope)
				return -1;

			Scope->SetUserData(this, CompilerUD);
			VM->SetProcessorOptions(Processor);
			return 0;
		}
		int Compiler::Prepare(const std::string& ModuleName, const std::string& Name, bool Debug, bool Scoped)
		{
			ED_ASSERT(VM != nullptr, -1, "engine should be set");

			int Result = Prepare(ModuleName, Scoped);
			if (Result < 0)
				return Result;

			VCache.Name = Name;
			if (!VM->GetByteCodeCache(&VCache))
			{
				VCache.Data.clear();
				VCache.Debug = Debug;
				VCache.Valid = false;
			}

			return Result;
		}
		int Compiler::SaveByteCode(ByteCodeInfo* Info)
		{
			ED_ASSERT(VM != nullptr, -1, "engine should be set");
			ED_ASSERT(Scope != nullptr, -1, "module should not be empty");
			ED_ASSERT(Info != nullptr, -1, "bytecode should be set");
			ED_ASSERT(BuiltOK, -1, "module should be built");

			CByteCodeStream* Stream = ED_NEW(CByteCodeStream);
			int R = Scope->SaveByteCode(Stream, Info->Debug);
			Info->Data = Stream->GetCode();
			ED_DELETE(CByteCodeStream, Stream);

			if (R >= 0)
				ED_DEBUG("[vm] OK save bytecode on 0x%" PRIXPTR, (uintptr_t)this);

			return R;
		}
		int Compiler::LoadFile(const std::string& Path)
		{
			ED_ASSERT(VM != nullptr, -1, "engine should be set");
			ED_ASSERT(Scope != nullptr, -1, "module should not be empty");

			if (VCache.Valid)
				return 0;

			std::string Source = Core::OS::Path::Resolve(Path.c_str());
			if (!Core::OS::File::IsExists(Source.c_str()))
			{
				ED_ERR("[vm] file %s not found", Source.c_str());
				return -1;
			}

			std::string Buffer = Core::OS::File::ReadAsString(Source);
			if (!GenerateSourceCode(Processor, Source, Buffer))
				return asINVALID_DECLARATION;

			int R = Scope->AddScriptSection(Source.c_str(), Buffer.c_str(), Buffer.size());
			if (R >= 0)
				ED_DEBUG("[vm] OK load program on 0x%" PRIXPTR " (file)", (uintptr_t)this);

			return R;
		}
		int Compiler::LoadCode(const std::string& Name, const std::string& Data)
		{
			ED_ASSERT(VM != nullptr, -1, "engine should be set");
			ED_ASSERT(Scope != nullptr, -1, "module should not be empty");

			if (VCache.Valid)
				return 0;

			std::string Buffer(Data);
			if (!GenerateSourceCode(Processor, Name, Buffer))
				return asINVALID_DECLARATION;

			int R = Scope->AddScriptSection(Name.c_str(), Buffer.c_str(), Buffer.size());
			if (R >= 0)
				ED_DEBUG("[vm] OK load program on 0x%" PRIXPTR, (uintptr_t)this);

			return R;
		}
		int Compiler::LoadCode(const std::string& Name, const char* Data, size_t Size)
		{
			ED_ASSERT(VM != nullptr, -1, "engine should be set");
			ED_ASSERT(Scope != nullptr, -1, "module should not be empty");

			if (VCache.Valid)
				return 0;

			std::string Buffer(Data, Size);
			if (!GenerateSourceCode(Processor, Name, Buffer))
				return asINVALID_DECLARATION;

			int R = Scope->AddScriptSection(Name.c_str(), Buffer.c_str(), Buffer.size());
			if (R >= 0)
				ED_DEBUG("[vm] OK load program on 0x%" PRIXPTR, (uintptr_t)this);

			return R;
		}
		Core::Promise<int> Compiler::Compile()
		{
			ED_ASSERT(VM != nullptr, Core::Promise<int>(-1), "engine should be set");
			ED_ASSERT(Scope != nullptr, Core::Promise<int>(-1), "module should not be empty");

			if (VCache.Valid)
			{
				return LoadByteCode(&VCache).Then<int>([this](int&& R)
				{
					BuiltOK = (R >= 0);
					if (!BuiltOK)
						return R;

					ED_DEBUG("[vm] OK compile on 0x%" PRIXPTR " (cache)", (uintptr_t)this);
					Scope->ResetGlobalVars(Context->GetContext());
					return R;
				});
			}

			return Core::Cotask<int>([this]()
			{
				int R = 0;
				while ((R = Scope->Build()) == asBUILD_IN_PROGRESS)
					std::this_thread::sleep_for(std::chrono::microseconds(100));
				return R;
			}).Then<int>([this](int&& R)
			{
				BuiltOK = (R >= 0);
				if (!BuiltOK)
					return R;

				ED_DEBUG("[vm] OK compile on 0x%" PRIXPTR, (uintptr_t)this);
				Scope->ResetGlobalVars(Context->GetContext());

				if (VCache.Name.empty())
					return R;

				R = SaveByteCode(&VCache);
				if (R < 0)
					return R;

				VM->SetByteCodeCache(&VCache);
				return R;
			});
		}
		Core::Promise<int> Compiler::LoadByteCode(ByteCodeInfo* Info)
		{
			ED_ASSERT(VM != nullptr, Core::Promise<int>(-1), "engine should be set");
			ED_ASSERT(Scope != nullptr, Core::Promise<int>(-1), "module should not be empty");
			ED_ASSERT(Info != nullptr, Core::Promise<int>(-1), "bytecode should be set");

			CByteCodeStream* Stream = ED_NEW(CByteCodeStream, Info->Data);
			return Core::Cotask<int>([this, Stream, Info]()
			{
				int R = 0;
				while ((R = Scope->LoadByteCode(Stream, &Info->Debug)) == asBUILD_IN_PROGRESS)
					std::this_thread::sleep_for(std::chrono::microseconds(100));
				return R;
			}).Then<int>([this, Stream](int&& R)
			{
				ED_DELETE(CByteCodeStream, Stream);
				if (R >= 0)
					ED_DEBUG("[vm] OK load bytecode on 0x%" PRIXPTR, (uintptr_t)this);
				return R;
			});
		}
		Core::Promise<int> Compiler::ExecuteFile(const char* Name, const char* ModuleName, const char* EntryName, ArgsCallback&& OnArgs)
		{
			ED_ASSERT(VM != nullptr, Core::Promise<int>(asINVALID_ARG), "engine should be set");
			ED_ASSERT(Name != nullptr, Core::Promise<int>(asINVALID_ARG), "name should be set");
			ED_ASSERT(ModuleName != nullptr, Core::Promise<int>(asINVALID_ARG), "module name should be set");
			ED_ASSERT(EntryName != nullptr, Core::Promise<int>(asINVALID_ARG), "entry name should be set");

			int R = Prepare(ModuleName, Name);
			if (R < 0)
				return Core::Promise<int>(R);

			R = LoadFile(Name);
			if (R < 0)
				return Core::Promise<int>(R);

			return Compile().Then<Core::Promise<int>>([this, EntryName, OnArgs = std::move(OnArgs)](int&& R) mutable
			{
				if (R < 0)
					return Core::Promise<int>(R);

				return ExecuteEntry(EntryName, std::move(OnArgs));
			});
		}
		Core::Promise<int> Compiler::ExecuteMemory(const std::string& Buffer, const char* ModuleName, const char* EntryName, ArgsCallback&& OnArgs)
		{
			ED_ASSERT(VM != nullptr, Core::Promise<int>(asINVALID_ARG), "engine should be set");
			ED_ASSERT(!Buffer.empty(), Core::Promise<int>(asINVALID_ARG), "buffer should not be empty");
			ED_ASSERT(ModuleName != nullptr, Core::Promise<int>(asINVALID_ARG), "module name should be set");
			ED_ASSERT(EntryName != nullptr, Core::Promise<int>(asINVALID_ARG), "entry name should be set");

			int R = Prepare(ModuleName, "anonymous");
			if (R < 0)
				return Core::Promise<int>(R);

			R = LoadCode("anonymous", Buffer);
			if (R < 0)
				return Core::Promise<int>(R);
			
			return Compile().Then<Core::Promise<int>>([this, EntryName, OnArgs = std::move(OnArgs)](int&& R) mutable
			{
				if (R < 0)
					return Core::Promise<int>(R);

				return ExecuteEntry(EntryName, std::move(OnArgs));
			});
		}
		Core::Promise<int> Compiler::ExecuteEntry(const char* Name, ArgsCallback&& OnArgs)
		{
			ED_ASSERT(VM != nullptr, Core::Promise<int>(asINVALID_ARG), "engine should be set");
			ED_ASSERT(Name != nullptr, Core::Promise<int>(asINVALID_ARG), "name should be set");
			ED_ASSERT(Context != nullptr, Core::Promise<int>(asINVALID_ARG), "context should be set");
			ED_ASSERT(Scope != nullptr, Core::Promise<int>(asINVALID_ARG), "module should not be empty");
			ED_ASSERT(BuiltOK, Core::Promise<int>(asINVALID_ARG), "module should be built");

			asIScriptEngine* Engine = VM->GetEngine();
			ED_ASSERT(Engine != nullptr, Core::Promise<int>(asINVALID_CONFIGURATION), "engine should be set");

			asIScriptFunction* Function = Scope->GetFunctionByName(Name);
			if (!Function)
				return Core::Promise<int>(asNO_FUNCTION);

			return Context->TryExecute(false, Function, std::move(OnArgs));
		}
		Core::Promise<int> Compiler::ExecuteScoped(const std::string& Code, const char* Args, ArgsCallback&& OnArgs)
		{
			return ExecuteScoped(Code.c_str(), Code.size(), Args, std::move(OnArgs));
		}
		Core::Promise<int> Compiler::ExecuteScoped(const char* Buffer, size_t Length, const char* Args, ArgsCallback&& OnArgs)
		{
			ED_ASSERT(VM != nullptr, Core::Promise<int>(asINVALID_ARG), "engine should be set");
			ED_ASSERT(Buffer != nullptr && Length > 0, Core::Promise<int>(asINVALID_ARG), "buffer should not be empty");
			ED_ASSERT(Context != nullptr, Core::Promise<int>(asINVALID_ARG), "context should be set");
			ED_ASSERT(Scope != nullptr, Core::Promise<int>(asINVALID_ARG), "module should not be empty");
			ED_ASSERT(BuiltOK, Core::Promise<int>(asINVALID_ARG), "module should be built");

			std::string Eval = "void __vfbdy(";
			if (Args != nullptr)
				Eval.append(Args);
			Eval.append("){\n");
			Eval.append(Buffer, Length);
			Eval += "\n;}";

			asIScriptModule* Source = GetModule().GetModule();	
			return Core::Cotask<Core::Promise<int>>([this, Source, Eval, OnArgs = std::move(OnArgs)]() mutable
			{
				asIScriptFunction* Function = nullptr; int R = 0;
				while ((R = Source->CompileFunction("__vfbdy", Eval.c_str(), -1, asCOMP_ADD_TO_MODULE, &Function)) == asBUILD_IN_PROGRESS)
					std::this_thread::sleep_for(std::chrono::microseconds(100));

				if (R < 0)
					return Core::Promise<int>(R);

				Core::Promise<int> Result = Context->TryExecute(false, Function, std::move(OnArgs));
				Function->Release();

				return Result;
			}).Then<Core::Promise<int>>([](Core::Promise<int>&& Result)
			{
				return Result;
			});
		}
		VirtualMachine* Compiler::GetVM() const
		{
			return VM;
		}
		ImmediateContext* Compiler::GetContext() const
		{
			return Context;
		}
		Module Compiler::GetModule() const
		{
			return Scope;
		}
		Compute::Preprocessor* Compiler::GetProcessor() const
		{
			return Processor;
		}
		Compiler* Compiler::Get(ImmediateContext* Context)
		{
			if (!Context)
				return nullptr;

			return (Compiler*)Context->GetUserData(CompilerUD);
		}
		int Compiler::CompilerUD = 154;

		ImmediateContext::ImmediateContext(asIScriptContext* Base) noexcept : Context(Base), VM(nullptr)
		{
			ED_ASSERT_V(Base != nullptr, "context should be set");
			VM = VirtualMachine::Get(Base->GetEngine());
			Context->SetUserData(this, ContextUD);
			Context->SetExceptionCallback(asFUNCTION(ExceptionLogger), this, asCALL_CDECL);
		}
		ImmediateContext::~ImmediateContext() noexcept
		{
			while (!Tasks.empty())
			{
				auto& Next = Tasks.front();
				Next.Callback.Release();
				Next.Future.Set(asCONTEXT_NOT_PREPARED);
				Tasks.pop();
			}

			if (Context != nullptr)
			{
				if (VM != nullptr)
					VM->GetEngine()->ReturnContext(Context);
				else
					Context->Release();
			}
		}
		Core::Promise<int> ImmediateContext::TryExecute(bool IsNested, const Function& Function, ArgsCallback&& OnArgs)
		{
			ED_ASSERT(Context != nullptr, Core::Promise<int>(asINVALID_ARG), "context should be set");
			ED_ASSERT(Function.IsValid(), Core::Promise<int>(asINVALID_ARG), "function should be set");

			if (!IsNested && Core::Costate::IsCoroutine())
			{
				Core::Promise<int> Result;
				Core::Schedule::Get()->SetTask([this, Result, Function, OnArgs = std::move(OnArgs)]() mutable
				{
					Result.Set(TryExecute(false, Function, std::move(OnArgs)));
				}, Core::Difficulty::Heavy);
				return Result;
			}

			Exchange.lock();
			if (IsNested)
			{
				bool WantsNesting = Context->GetState() == asEXECUTION_ACTIVE;
				if (WantsNesting)
					Context->PushState();

				int Result = Context->Prepare(Function.GetFunction());
				if (Result < 0)
				{
					Exchange.unlock();
					return Core::Promise<int>(Result);
				}

				Exchange.unlock();
				if (OnArgs)
					OnArgs(this);

				Result = Context->Execute();
				if (WantsNesting)
					Context->PopState();

				return Core::Promise<int>(Result);
			}
			else if (Tasks.empty())
			{
				int Result = Context->Prepare(Function.GetFunction());
				if (Result < 0)
				{
					Exchange.unlock();
					return Core::Promise<int>(Result);
				}
				else
					Tasks.emplace();

				auto Future = Tasks.front().Future;
				Exchange.unlock();

				if (OnArgs)
					OnArgs(this);

				Execute();
				return Future;
			}
			else
			{
				Task Next;
				Next.Args = std::move(OnArgs);
				Next.Callback = Function;
				Next.Callback.AddRef();

				Core::Promise<int> Future = Next.Future;
				Tasks.emplace(std::move(Next));
				Exchange.unlock();

				return Future;
			}
		}
		int ImmediateContext::SetOnException(void(*Callback)(asIScriptContext* Context, void* Object), void* Object)
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetExceptionCallback(asFUNCTION(Callback), Object, asCALL_CDECL);
		}
		int ImmediateContext::Prepare(const Function& Function)
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->Prepare(Function.GetFunction());
		}
		int ImmediateContext::Unprepare()
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->Unprepare();
		}
		int ImmediateContext::Execute()
		{
			int Result = Context->Execute();
			if (Result != asEXECUTION_SUSPENDED)
			{
				Exchange.lock();
				if (!Tasks.empty())
				{
					int Subresult = 0;
					{
						auto Current = Tasks.front();
						Current.Callback.Release();
						Tasks.pop();

						Exchange.unlock();
						Current.Future.Set(Result);
						Exchange.lock();
					}

					while (!Tasks.empty())
					{
						auto Next = Tasks.front();
						Tasks.pop();
						Exchange.unlock();

						Subresult = Context->Prepare(Next.Callback.GetFunction());
						if (Subresult >= 0)
						{
							if (Next.Args)
								Next.Args(this);

							Subresult = Context->Execute();
							if (Subresult == asEXECUTION_SUSPENDED)
								return Result;
						}

						Next.Future.Set(Subresult);
						Exchange.lock();
					}
				}
				Exchange.unlock();
			}

			return Result;
		}
		int ImmediateContext::Abort()
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->Abort();
		}
		int ImmediateContext::Suspend()
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->Suspend();
		}
		Activation ImmediateContext::GetState() const
		{
			ED_ASSERT(Context != nullptr, Activation::UNINITIALIZED, "context should be set");
			return (Activation)Context->GetState();
		}
		std::string ImmediateContext::GetStackTrace(size_t Skips, size_t MaxFrames) const
		{
			std::string Trace = Core::OS::GetStackTrace(Skips, MaxFrames).append("\n");
			ED_ASSERT(Context != nullptr, Trace, "context should be set");

			std::string ThreadId = Core::OS::Process::GetThreadId(std::this_thread::get_id());
			std::stringstream Stream;
			Stream << "vm stack trace (most recent call last)" << (!ThreadId.empty() ? " in thread " : ":\n");
			if (!ThreadId.empty())
				Stream << ThreadId << ":\n";

			for (asUINT TraceIdx = Context->GetCallstackSize(); TraceIdx-- > 0;)
			{
				asIScriptFunction* Function = Context->GetFunction(TraceIdx);
				if (Function != nullptr)
				{
					void* Address = (void*)(uintptr_t)Function->GetId();
					Stream << "#" << TraceIdx << "   ";

					if (Function->GetFuncType() == asFUNC_SCRIPT)
						Stream << "source \"" << (Function->GetScriptSectionName() ? Function->GetScriptSectionName() : "") << "\", line " << Context->GetLineNumber(TraceIdx) << ", in " << Function->GetDeclaration();
					else
						Stream << "source {...native_call...}, in " << Function->GetDeclaration() << " nullptr";

					if (Address != nullptr)
						Stream << " 0x" << Address;
					else
						Stream << " nullptr";
				}
				else
					Stream << "source {...native_call...} [nullptr]";
				Stream << "\n";
			}

			std::string Out(Stream.str());
			return Trace + Out.substr(0, Out.size() - 1);
		}
		int ImmediateContext::PushState()
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->PushState();
		}
		int ImmediateContext::PopState()
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->PopState();
		}
		bool ImmediateContext::IsNested(unsigned int* NestCount) const
		{
			ED_ASSERT(Context != nullptr, false, "context should be set");
			return Context->IsNested(NestCount);
		}
		bool ImmediateContext::IsThrown() const
		{
			ED_ASSERT(Context != nullptr, false, "context should be set");
			const char* Exception = Context->GetExceptionString();
			if (!Exception)
				return false;

			return Exception[0] != '\0';
		}
		bool ImmediateContext::IsPending()
		{
			Exchange.lock();
			bool Pending = !Tasks.empty();
			Exchange.unlock();

			return Pending;
		}
		int ImmediateContext::SetObject(void* Object)
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetObject(Object);
		}
		int ImmediateContext::SetArg8(unsigned int Arg, unsigned char Value)
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetArgByte(Arg, Value);
		}
		int ImmediateContext::SetArg16(unsigned int Arg, unsigned short Value)
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetArgWord(Arg, Value);
		}
		int ImmediateContext::SetArg32(unsigned int Arg, int Value)
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetArgDWord(Arg, Value);
		}
		int ImmediateContext::SetArg64(unsigned int Arg, int64_t Value)
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetArgQWord(Arg, Value);
		}
		int ImmediateContext::SetArgFloat(unsigned int Arg, float Value)
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetArgFloat(Arg, Value);
		}
		int ImmediateContext::SetArgDouble(unsigned int Arg, double Value)
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetArgDouble(Arg, Value);
		}
		int ImmediateContext::SetArgAddress(unsigned int Arg, void* Address)
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetArgAddress(Arg, Address);
		}
		int ImmediateContext::SetArgObject(unsigned int Arg, void* Object)
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetArgObject(Arg, Object);
		}
		int ImmediateContext::SetArgAny(unsigned int Arg, void* Ptr, int TypeId)
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetArgVarType(Arg, Ptr, TypeId);
		}
		int ImmediateContext::GetReturnableByType(void* Return, asITypeInfo* ReturnTypeInfo)
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			ED_ASSERT(Return != nullptr, -1, "return value should be set");
			ED_ASSERT(ReturnTypeInfo != nullptr, -1, "return type info should be set");
			ED_ASSERT(ReturnTypeInfo->GetTypeId() != (int)TypeId::VOIDF, -1, "return value type should not be void");

			void* Address = Context->GetAddressOfReturnValue();
			if (!Address)
				return -1;

			int TypeId = ReturnTypeInfo->GetTypeId();
			asIScriptEngine* Engine = VM->GetEngine();
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
		int ImmediateContext::GetReturnableByDecl(void* Return, const char* ReturnTypeDecl)
		{
			ED_ASSERT(ReturnTypeDecl != nullptr, -1, "return type declaration should be set");
			asIScriptEngine* Engine = VM->GetEngine();
			return GetReturnableByType(Return, Engine->GetTypeInfoByDecl(ReturnTypeDecl));
		}
		int ImmediateContext::GetReturnableById(void* Return, int ReturnTypeId)
		{
			ED_ASSERT(ReturnTypeId != (int)TypeId::VOIDF, -1, "return value type should not be void");
			asIScriptEngine* Engine = VM->GetEngine();
			return GetReturnableByType(Return, Engine->GetTypeInfoById(ReturnTypeId));
		}
		void* ImmediateContext::GetAddressOfArg(unsigned int Arg)
		{
			ED_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetAddressOfArg(Arg);
		}
		unsigned char ImmediateContext::GetReturnByte()
		{
			ED_ASSERT(Context != nullptr, 0, "context should be set");
			return Context->GetReturnByte();
		}
		unsigned short ImmediateContext::GetReturnWord()
		{
			ED_ASSERT(Context != nullptr, 0, "context should be set");
			return Context->GetReturnWord();
		}
		size_t ImmediateContext::GetReturnDWord()
		{
			ED_ASSERT(Context != nullptr, 0, "context should be set");
			return Context->GetReturnDWord();
		}
		uint64_t ImmediateContext::GetReturnQWord()
		{
			ED_ASSERT(Context != nullptr, 0, "context should be set");
			return Context->GetReturnQWord();
		}
		float ImmediateContext::GetReturnFloat()
		{
			ED_ASSERT(Context != nullptr, 0.0f, "context should be set");
			return Context->GetReturnFloat();
		}
		double ImmediateContext::GetReturnDouble()
		{
			ED_ASSERT(Context != nullptr, 0.0, "context should be set");
			return Context->GetReturnDouble();
		}
		void* ImmediateContext::GetReturnAddress()
		{
			ED_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetReturnAddress();
		}
		void* ImmediateContext::GetReturnObjectAddress()
		{
			ED_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetReturnObject();
		}
		void* ImmediateContext::GetAddressOfReturnValue()
		{
			ED_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetAddressOfReturnValue();
		}
		int ImmediateContext::SetException(const char* Info, bool AllowCatch)
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->SetException(Info, AllowCatch);
		}
		int ImmediateContext::GetExceptionLineNumber(int* Column, const char** SectionName)
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->GetExceptionLineNumber(Column, SectionName);
		}
		Function ImmediateContext::GetExceptionFunction()
		{
			ED_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetExceptionFunction();
		}
		const char* ImmediateContext::GetExceptionString()
		{
			ED_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetExceptionString();
		}
		bool ImmediateContext::WillExceptionBeCaught()
		{
			ED_ASSERT(Context != nullptr, false, "context should be set");
			return Context->WillExceptionBeCaught();
		}
		int ImmediateContext::SetLineCallback(void(*Callback)(asIScriptContext* Context, void* Object), void* Object)
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			ED_ASSERT(Callback != nullptr, -1, "callback should be set");

			return Context->SetLineCallback(asFUNCTION(Callback), Object, asCALL_CDECL);
		}
		int ImmediateContext::SetLineCallback(const std::function<void(ImmediateContext*)>& Callback)
		{
			LineCallback = Callback;
			return SetLineCallback(&ImmediateContext::LineLogger, this);
		}
		int ImmediateContext::SetExceptionCallback(const std::function<void(ImmediateContext*)>& Callback)
		{
			ExceptionCallback = Callback;
			return 0;
		}
		void ImmediateContext::ClearLineCallback()
		{
			ED_ASSERT_V(Context != nullptr, "context should be set");
			Context->ClearLineCallback();
			LineCallback = nullptr;
		}
		void ImmediateContext::ClearExceptionCallback()
		{
			ExceptionCallback = nullptr;
		}
		unsigned int ImmediateContext::GetCallstackSize() const
		{
			ED_ASSERT(Context != nullptr, 0, "context should be set");
			return Context->GetCallstackSize();
		}
		Function ImmediateContext::GetFunction(unsigned int StackLevel)
		{
			ED_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetFunction(StackLevel);
		}
		int ImmediateContext::GetLineNumber(unsigned int StackLevel, int* Column, const char** SectionName)
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->GetLineNumber(StackLevel, Column, SectionName);
		}
		int ImmediateContext::GetPropertiesCount(unsigned int StackLevel)
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->GetVarCount(StackLevel);
		}
		const char* ImmediateContext::GetPropertyName(unsigned int Index, unsigned int StackLevel)
		{
			ED_ASSERT(Context != nullptr, nullptr, "context should be set");
			const char* Name = nullptr;
			Context->GetVar(Index, StackLevel, &Name);
			return Name;
		}
		const char* ImmediateContext::GetPropertyDecl(unsigned int Index, unsigned int StackLevel, bool IncludeNamespace)
		{
			ED_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetVarDeclaration(Index, StackLevel, IncludeNamespace);
		}
		int ImmediateContext::GetPropertyTypeId(unsigned int Index, unsigned int StackLevel)
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			int TypeId = -1;
			Context->GetVar(Index, StackLevel, nullptr, &TypeId);
			return TypeId;
		}
		void* ImmediateContext::GetAddressOfProperty(unsigned int Index, unsigned int StackLevel)
		{
			ED_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetAddressOfVar(Index, StackLevel);
		}
		bool ImmediateContext::IsPropertyInScope(unsigned int Index, unsigned int StackLevel)
		{
			ED_ASSERT(Context != nullptr, false, "context should be set");
			return Context->IsVarInScope(Index, StackLevel);
		}
		int ImmediateContext::GetThisTypeId(unsigned int StackLevel)
		{
			ED_ASSERT(Context != nullptr, -1, "context should be set");
			return Context->GetThisTypeId(StackLevel);
		}
		void* ImmediateContext::GetThisPointer(unsigned int StackLevel)
		{
			ED_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetThisPointer(StackLevel);
		}
		std::string ImmediateContext::GetErrorStackTrace()
		{
			Exchange.lock();
			std::string Result = Stacktrace;
			Exchange.unlock();

			return Result;
		}
		Function ImmediateContext::GetSystemFunction()
		{
			ED_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetSystemFunction();
		}
		bool ImmediateContext::IsSuspended() const
		{
			ED_ASSERT(Context != nullptr, false, "context should be set");
			return Context->GetState() == asEXECUTION_SUSPENDED;
		}
		void* ImmediateContext::SetUserData(void* Data, size_t Type)
		{
			ED_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->SetUserData(Data, Type);
		}
		void* ImmediateContext::GetUserData(size_t Type) const
		{
			ED_ASSERT(Context != nullptr, nullptr, "context should be set");
			return Context->GetUserData(Type);
		}
		asIScriptContext* ImmediateContext::GetContext()
		{
			return Context;
		}
		VirtualMachine* ImmediateContext::GetVM()
		{
			return VM;
		}
		ImmediateContext* ImmediateContext::Get(asIScriptContext* Context)
		{
			ED_ASSERT(Context != nullptr, nullptr, "context should be set");
			void* VM = Context->GetUserData(ContextUD);
			return (ImmediateContext*)VM;
		}
		ImmediateContext* ImmediateContext::Get()
		{
			asIScriptContext* Context = asGetActiveContext();
			if (!Context)
				return nullptr;

			return Get(Context);
		}
		void ImmediateContext::LineLogger(asIScriptContext* Context, void*)
		{
			ImmediateContext* Base = ImmediateContext::Get(Context);
			ED_ASSERT_V(Base != nullptr, "context should be set");
			ED_ASSERT_V(Base->LineCallback, "context should be set");

			Base->LineCallback(Base);
		}
		void ImmediateContext::ExceptionLogger(asIScriptContext* Context, void*)
		{
			asIScriptFunction* Function = Context->GetExceptionFunction();
			ImmediateContext* Base = ImmediateContext::Get(Context);
			ED_ASSERT_V(Base != nullptr, "context should be set");

			const char* Message = Context->GetExceptionString();
			if (Message && Message[0] != '\0' && !Context->WillExceptionBeCaught())
			{
				const char* Name = Function->GetName();
				const char* Source = Function->GetModuleName();
				int Line = Context->GetExceptionLineNumber();
				std::string Trace = Base->GetStackTrace(3, 64);

				ED_ERR("[vm] %s:%d %s(): runtime exception thrown\n\tdetails: %s\n\texecution flow dump: %.*s\n",
					   Source ? Source : "log", Line,
					   Name ? Name : "anonymous",
					   Message ? Message : "no additional data",
					   (int)Trace.size(), Trace.c_str());

				Base->Exchange.lock();
				Base->Stacktrace = Trace;
				Base->Exchange.unlock();

				if (Base->ExceptionCallback)
					Base->ExceptionCallback(Base);
			}
			else if (Base->ExceptionCallback)
				Base->ExceptionCallback(Base);
		}
		int ImmediateContext::ContextUD = 152;

		VirtualMachine::VirtualMachine() noexcept : Scope(0), Engine(asCreateScriptEngine()), Imports((uint32_t)Imports::All), Cached(true)
		{
			Include.Exts.push_back(".as");
			Include.Root = Core::OS::Directory::Get();

			Engine->SetUserData(this, ManagerUD);
			Engine->SetContextCallbacks(RequestContext, ReturnContext, nullptr);
			Engine->SetMessageCallback(asFUNCTION(CompileLogger), this, asCALL_CDECL);
			Engine->SetEngineProperty(asEP_INIT_GLOBAL_VARS_AFTER_BUILD, 0);
			Engine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, 1);
			Engine->SetEngineProperty(asEP_DISALLOW_EMPTY_LIST_ELEMENTS, 1);
			Engine->SetEngineProperty(asEP_COMPILER_WARNINGS, 1);
			RegisterSubmodules(this);
		}
		VirtualMachine::~VirtualMachine() noexcept
		{
			for (auto& Core : Kernels)
				Core::OS::Symbol::Unload(Core.second.Handle);

			for (auto& Context : Contexts)
				Context->Release();

			if (Engine != nullptr)
				Engine->ShutDownAndRelease();
			ClearCache();
		}
		int VirtualMachine::SetFunctionDef(const char* Decl)
		{
			ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
			ED_ASSERT(Engine != nullptr, -1, "engine should be set");
			ED_TRACE("[vm] register funcdef %i bytes", (int)strlen(Decl));

			return Engine->RegisterFuncdef(Decl);
		}
		int VirtualMachine::SetFunctionAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type)
		{
			ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
			ED_ASSERT(Value != nullptr, -1, "value should be set");
			ED_ASSERT(Engine != nullptr, -1, "engine should be set");
			ED_TRACE("[vm] register funcaddr(%i) %i bytes at 0x%" PRIXPTR, (int)Type, (int)strlen(Decl), (void*)Value);

			return Engine->RegisterGlobalFunction(Decl, *Value, (asECallConvTypes)Type);
		}
		int VirtualMachine::SetPropertyAddress(const char* Decl, void* Value)
		{
			ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
			ED_ASSERT(Value != nullptr, -1, "value should be set");
			ED_ASSERT(Engine != nullptr, -1, "engine should be set");
			ED_TRACE("[vm] register global %i bytes at 0x%" PRIXPTR, (int)strlen(Decl), (void*)Value);

			return Engine->RegisterGlobalProperty(Decl, Value);
		}
		TypeInterface VirtualMachine::SetInterface(const char* Name)
		{
			ED_ASSERT(Name != nullptr, TypeInterface(nullptr, "", -1), "name should be set");
			ED_ASSERT(Engine != nullptr, TypeInterface(nullptr, "", -1), "engine should be set");
			ED_TRACE("[vm] register interface %i bytes", (int)strlen(Name));

			return TypeInterface(this, Name, Engine->RegisterInterface(Name));
		}
		TypeClass VirtualMachine::SetStructAddress(const char* Name, size_t Size, uint64_t Flags)
		{
			ED_ASSERT(Name != nullptr, TypeClass(nullptr, "", -1), "name should be set");
			ED_ASSERT(Engine != nullptr, TypeClass(nullptr, "", -1), "engine should be set");
			ED_TRACE("[vm] register struct(%i) %i bytes sizeof %i", (int)Flags, (int)strlen(Name), (int)Size);

			return TypeClass(this, Name, Engine->RegisterObjectType(Name, (asUINT)Size, (asDWORD)Flags));
		}
		TypeClass VirtualMachine::SetPodAddress(const char* Name, size_t Size, uint64_t Flags)
		{
			return SetStructAddress(Name, Size, Flags);
		}
		RefClass VirtualMachine::SetClassAddress(const char* Name, uint64_t Flags)
		{
			ED_ASSERT(Name != nullptr, RefClass(nullptr, "", -1), "name should be set");
			ED_ASSERT(Engine != nullptr, RefClass(nullptr, "", -1), "engine should be set");
			ED_TRACE("[vm] register class(%i) %i bytes", (int)Flags, (int)strlen(Name));

			return RefClass(this, Name, Engine->RegisterObjectType(Name, 0, (asDWORD)Flags));
		}
		Enumeration VirtualMachine::SetEnum(const char* Name)
		{
			ED_ASSERT(Name != nullptr, Enumeration(nullptr, "", -1), "name should be set");
			ED_ASSERT(Engine != nullptr, Enumeration(nullptr, "", -1), "engine should be set");
			ED_TRACE("[vm] register enum %i bytes", (int)strlen(Name));

			return Enumeration(this, Name, Engine->RegisterEnum(Name));
		}
		size_t VirtualMachine::GetFunctionsCount() const
		{
			return Engine->GetGlobalFunctionCount();
		}
		Function VirtualMachine::GetFunctionById(int Id) const
		{
			return Engine->GetFunctionById(Id);
		}
		Function VirtualMachine::GetFunctionByIndex(int Index) const
		{
			return Engine->GetGlobalFunctionByIndex(Index);
		}
		Function VirtualMachine::GetFunctionByDecl(const char* Decl) const
		{
			ED_ASSERT(Decl != nullptr, nullptr, "declaration should be set");
			return Engine->GetGlobalFunctionByDecl(Decl);
		}
		size_t VirtualMachine::GetPropertiesCount() const
		{
			return Engine->GetGlobalPropertyCount();
		}
		int VirtualMachine::GetPropertyByIndex(int Index, PropertyInfo* Info) const
		{
			const char* Name = nullptr, * Namespace = nullptr;
			const char* ConfigGroup = nullptr;
			void* Pointer = nullptr;
			bool IsConst = false;
			asDWORD AccessMask = 0;
			int TypeId = 0;
			int Result = Engine->GetGlobalPropertyByIndex(Index, &Name, &Namespace, &TypeId, &IsConst, &ConfigGroup, &Pointer, &AccessMask);

			if (Info != nullptr)
			{
				Info->Name = Name;
				Info->Namespace = Namespace;
				Info->TypeId = TypeId;
				Info->IsConst = IsConst;
				Info->ConfigGroup = ConfigGroup;
				Info->Pointer = Pointer;
				Info->AccessMask = AccessMask;
			}

			return Result;
		}
		int VirtualMachine::GetPropertyIndexByName(const char* Name) const
		{
			ED_ASSERT(Name != nullptr, -1, "name should be set");
			return Engine->GetGlobalPropertyIndexByName(Name);
		}
		int VirtualMachine::GetPropertyIndexByDecl(const char* Decl) const
		{
			ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
			return Engine->GetGlobalPropertyIndexByDecl(Decl);
		}
		size_t VirtualMachine::GetObjectsCount() const
		{
			return Engine->GetObjectTypeCount();
		}
		TypeInfo VirtualMachine::GetObjectByIndex(size_t Index) const
		{
			return Engine->GetObjectTypeByIndex((asUINT)Index);
		}
		size_t VirtualMachine::GetEnumCount() const
		{
			return Engine->GetEnumCount();
		}
		TypeInfo VirtualMachine::GetEnumByIndex(size_t Index) const
		{
			return Engine->GetEnumByIndex((asUINT)Index);
		}
		size_t VirtualMachine::GetFunctionDefsCount() const
		{
			return Engine->GetFuncdefCount();
		}
		TypeInfo VirtualMachine::GetFunctionDefByIndex(int Index) const
		{
			return Engine->GetFuncdefByIndex(Index);
		}
		size_t VirtualMachine::GetModulesCount() const
		{
			return Engine->GetModuleCount();
		}
		asIScriptModule* VirtualMachine::GetModuleById(int Id) const
		{
			return Engine->GetModuleByIndex(Id);
		}
		int VirtualMachine::GetTypeIdByDecl(const char* Decl) const
		{
			ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
			return Engine->GetTypeIdByDecl(Decl);
		}
		const char* VirtualMachine::GetTypeIdDecl(int TypeId, bool IncludeNamespace) const
		{
			return Engine->GetTypeDeclaration(TypeId, IncludeNamespace);
		}
		int VirtualMachine::GetSizeOfPrimitiveType(int TypeId) const
		{
			return Engine->GetSizeOfPrimitiveType(TypeId);
		}
		std::string VirtualMachine::GetObjectView(void* Object, int TypeId)
		{
			if (!Object)
				return "null";

			if (TypeId == (int)TypeId::INT8)
				return "int8(" + std::to_string(*(char*)(Object)) + "), ";
			else if (TypeId == (int)TypeId::INT16)
				return "int16(" + std::to_string(*(short*)(Object)) + "), ";
			else if (TypeId == (int)TypeId::INT32)
				return "int32(" + std::to_string(*(int*)(Object)) + "), ";
			else if (TypeId == (int)TypeId::INT64)
				return "int64(" + std::to_string(*(int64_t*)(Object)) + "), ";
			else if (TypeId == (int)TypeId::UINT8)
				return "uint8(" + std::to_string(*(unsigned char*)(Object)) + "), ";
			else if (TypeId == (int)TypeId::UINT16)
				return "uint16(" + std::to_string(*(unsigned short*)(Object)) + "), ";
			else if (TypeId == (int)TypeId::UINT32)
				return "uint32(" + std::to_string(*(unsigned int*)(Object)) + "), ";
			else if (TypeId == (int)TypeId::UINT64)
				return "uint64(" + std::to_string(*(uint64_t*)(Object)) + "), ";
			else if (TypeId == (int)TypeId::FLOAT)
				return "float(" + std::to_string(*(float*)(Object)) + "), ";
			else if (TypeId == (int)TypeId::DOUBLE)
				return "double(" + std::to_string(*(double*)(Object)) + "), ";

			asITypeInfo* Type = Engine->GetTypeInfoById(TypeId);
			const char* Name = Type->GetName();

			return Core::Form("%s(0x%" PRIXPTR ")", Name ? Name : "unknown", (uintptr_t)Object).R();
		}
		TypeInfo VirtualMachine::GetTypeInfoById(int TypeId) const
		{
			return Engine->GetTypeInfoById(TypeId);
		}
		TypeInfo VirtualMachine::GetTypeInfoByName(const char* Name)
		{
			ED_ASSERT(Name != nullptr, nullptr, "name should be set");
			const char* TypeName = Name;
			const char* Namespace = nullptr;
			size_t NamespaceSize = 0;

			if (GetTypeNameScope(&TypeName, &Namespace, &NamespaceSize) != 0)
				return Engine->GetTypeInfoByName(Name);

			BeginNamespace(std::string(Namespace, NamespaceSize).c_str());
			asITypeInfo* Info = Engine->GetTypeInfoByName(TypeName);
			EndNamespace();

			return Info;
		}
		TypeInfo VirtualMachine::GetTypeInfoByDecl(const char* Decl) const
		{
			ED_ASSERT(Decl != nullptr, nullptr, "declaration should be set");
			return Engine->GetTypeInfoByDecl(Decl);
		}
		void VirtualMachine::SetImports(unsigned int Opts)
		{
			Imports = Opts;
		}
		void VirtualMachine::SetCache(bool Enabled)
		{
			Cached = Enabled;
		}
		void VirtualMachine::ClearCache()
		{
			Sync.General.lock();
			for (auto Data : Datas)
				ED_RELEASE(Data.second);

			Opcodes.clear();
			Datas.clear();
			Files.clear();
			Sync.General.unlock();
		}
		void VirtualMachine::SetCompilerErrorCallback(const WhenErrorCallback& Callback)
		{
			WherError = Callback;
		}
		void VirtualMachine::SetCompilerIncludeOptions(const Compute::IncludeDesc& NewDesc)
		{
			Sync.General.lock();
			Include = NewDesc;
			Sync.General.unlock();
		}
		void VirtualMachine::SetCompilerFeatures(const Compute::Preprocessor::Desc& NewDesc)
		{
			Sync.General.lock();
			Proc = NewDesc;
			Sync.General.unlock();
		}
		void VirtualMachine::SetProcessorOptions(Compute::Preprocessor* Processor)
		{
			ED_ASSERT_V(Processor != nullptr, "preprocessor should be set");
			Sync.General.lock();
			Processor->SetIncludeOptions(Include);
			Processor->SetFeatures(Proc);
			Sync.General.unlock();
		}
		void VirtualMachine::SetCompileCallback(const std::string& Section, CompileCallback&& Callback)
		{
			Sync.General.lock();
			if (Callback != nullptr)
				Callbacks[Section] = std::move(Callback);
			else
				Callbacks.erase(Section);
			Sync.General.unlock();
		}
		bool VirtualMachine::GetByteCodeCache(ByteCodeInfo* Info)
		{
			ED_ASSERT(Info != nullptr, false, "bytecode should be set");
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
		void VirtualMachine::SetByteCodeCache(ByteCodeInfo* Info)
		{
			ED_ASSERT_V(Info != nullptr, "bytecode should be set");
			Info->Valid = true;
			if (!Cached)
				return;

			Sync.General.lock();
			Opcodes[Info->Name] = *Info;
			Sync.General.unlock();
		}
		int VirtualMachine::SetLogCallback(void(*Callback)(const asSMessageInfo* Message, void* Object), void* Object)
		{
			if (!Callback)
				return Engine->ClearMessageCallback();

			return Engine->SetMessageCallback(asFUNCTION(Callback), Object, asCALL_CDECL);
		}
		int VirtualMachine::Log(const char* Section, int Row, int Column, LogCategory Type, const char* Message)
		{
			return Engine->WriteMessage(Section, Row, Column, (asEMsgType)Type, Message);
		}
		ImmediateContext* VirtualMachine::CreateContext()
		{
			return new ImmediateContext(Engine->RequestContext());
		}
		Compiler* VirtualMachine::CreateCompiler()
		{
			return new Compiler(this);
		}
		asIScriptModule* VirtualMachine::CreateScopedModule(const std::string& Name)
		{
			ED_ASSERT(Engine != nullptr, nullptr, "engine should be set");
			ED_ASSERT(!Name.empty(), nullptr, "name should not be empty");

			Sync.General.lock();
			if (!Engine->GetModule(Name.c_str()))
			{
				Sync.General.unlock();
				return Engine->GetModule(Name.c_str(), asGM_ALWAYS_CREATE);
			}

			std::string Result;
			while (Result.size() < 1024)
			{
				Result = Name + std::to_string(Scope++);
				if (!Engine->GetModule(Result.c_str()))
				{
					Sync.General.unlock();
					return Engine->GetModule(Result.c_str(), asGM_ALWAYS_CREATE);
				}
			}

			Sync.General.unlock();
			return nullptr;
		}
		asIScriptModule* VirtualMachine::CreateModule(const std::string& Name)
		{
			ED_ASSERT(Engine != nullptr, nullptr, "engine should be set");
			ED_ASSERT(!Name.empty(), nullptr, "name should not be empty");

			Sync.General.lock();
			asIScriptModule* Result = Engine->GetModule(Name.c_str(), asGM_ALWAYS_CREATE);
			Sync.General.unlock();

			return Result;
		}
		void* VirtualMachine::CreateObject(const TypeInfo& Type)
		{
			return Engine->CreateScriptObject(Type.GetTypeInfo());
		}
		void* VirtualMachine::CreateObjectCopy(void* Object, const TypeInfo& Type)
		{
			return Engine->CreateScriptObjectCopy(Object, Type.GetTypeInfo());
		}
		void* VirtualMachine::CreateEmptyObject(const TypeInfo& Type)
		{
			return Engine->CreateUninitializedScriptObject(Type.GetTypeInfo());
		}
		Function VirtualMachine::CreateDelegate(const Function& Function, void* Object)
		{
			return Engine->CreateDelegate(Function.GetFunction(), Object);
		}
		int VirtualMachine::AssignObject(void* DestObject, void* SrcObject, const TypeInfo& Type)
		{
			return Engine->AssignScriptObject(DestObject, SrcObject, Type.GetTypeInfo());
		}
		void VirtualMachine::ReleaseObject(void* Object, const TypeInfo& Type)
		{
			return Engine->ReleaseScriptObject(Object, Type.GetTypeInfo());
		}
		void VirtualMachine::AddRefObject(void* Object, const TypeInfo& Type)
		{
			return Engine->AddRefScriptObject(Object, Type.GetTypeInfo());
		}
		int VirtualMachine::RefCastObject(void* Object, const TypeInfo& FromType, const TypeInfo& ToType, void** NewPtr, bool UseOnlyImplicitCast)
		{
			return Engine->RefCastObject(Object, FromType.GetTypeInfo(), ToType.GetTypeInfo(), NewPtr, UseOnlyImplicitCast);
		}
		int VirtualMachine::Collect(size_t NumIterations)
		{
			Sync.General.lock();
			int R = Engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DETECT_GARBAGE | asGC_DESTROY_GARBAGE, (asUINT)NumIterations);
			Sync.General.unlock();

			return R;
		}
		void VirtualMachine::GetStatistics(unsigned int* CurrentSize, unsigned int* TotalDestroyed, unsigned int* TotalDetected, unsigned int* NewObjects, unsigned int* TotalNewDestroyed) const
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
		int VirtualMachine::NotifyOfNewObject(void* Object, const TypeInfo& Type)
		{
			return Engine->NotifyGarbageCollectorOfNewObject(Object, Type.GetTypeInfo());
		}
		int VirtualMachine::GetObjectAddress(size_t Index, size_t* SequenceNumber, void** Object, TypeInfo* Type)
		{
			asUINT asSequenceNumber;
			asITypeInfo* OutType = nullptr;
			int Result = Engine->GetObjectInGC((asUINT)Index, &asSequenceNumber, Object, &OutType);

			if (SequenceNumber != nullptr)
				*SequenceNumber = (size_t)asSequenceNumber;

			if (Type != nullptr)
				*Type = TypeInfo(OutType);

			return Result;
		}
		void VirtualMachine::ForwardEnumReferences(void* Reference, const TypeInfo& Type)
		{
			return Engine->ForwardGCEnumReferences(Reference, Type.GetTypeInfo());
		}
		void VirtualMachine::ForwardReleaseReferences(void* Reference, const TypeInfo& Type)
		{
			return Engine->ForwardGCReleaseReferences(Reference, Type.GetTypeInfo());
		}
		void VirtualMachine::GCEnumCallback(void* Reference)
		{
			Engine->GCEnumCallback(Reference);
		}
		bool VirtualMachine::DumpRegisteredInterfaces(const std::string& Where)
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
				std::string Name = Namespace.first;
				std::string Subname = (Namespace.first.empty() ? "" : Name);
				auto Offset = Core::String(&Name).Find("::");

				if (Offset.Found)
				{
					Name = Name.substr(0, (size_t)Offset.Start);
					if (Groups.find(Name) != Groups.end())
					{
						Groups[Name].second.push_back(std::make_pair(Subname, &Namespace.second));
						continue;
					}
				}

				std::string File = Core::OS::Path::Resolve((Path + Core::String(Name).Replace("::", "/").ToLower().R() + ".as").c_str());
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
				ED_SORT(Group.second.second.begin(), Group.second.second.end(), [](const GroupKey& A, const GroupKey& B)
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
				ED_RELEASE(Stream);
			}

			return true;
		}
		bool VirtualMachine::DumpAllInterfaces(const std::string& Where)
		{
			for (auto& Item : Modules)
			{
				if (!ImportSubmodule(Item.first))
					return false;
			}

			return DumpRegisteredInterfaces(Where);
		}
		int VirtualMachine::GetTypeNameScope(const char** TypeName, const char** Namespace, size_t* NamespaceSize) const
		{
			ED_ASSERT(TypeName != nullptr && *TypeName != nullptr, -1, "typename should be set");

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
		int VirtualMachine::BeginGroup(const char* GroupName)
		{
			ED_ASSERT(GroupName != nullptr, -1, "group name should be set");
			return Engine->BeginConfigGroup(GroupName);
		}
		int VirtualMachine::EndGroup()
		{
			return Engine->EndConfigGroup();
		}
		int VirtualMachine::RemoveGroup(const char* GroupName)
		{
			ED_ASSERT(GroupName != nullptr, -1, "group name should be set");
			return Engine->RemoveConfigGroup(GroupName);
		}
		int VirtualMachine::BeginNamespace(const char* Namespace)
		{
			ED_ASSERT(Namespace != nullptr, -1, "namespace name should be set");
			const char* Prev = Engine->GetDefaultNamespace();
			Sync.General.lock();
			if (Prev != nullptr)
				DefaultNamespace = Prev;
			else
				DefaultNamespace.clear();
			Sync.General.unlock();

			return Engine->SetDefaultNamespace(Namespace);
		}
		int VirtualMachine::BeginNamespaceIsolated(const char* Namespace, size_t DefaultMask)
		{
			ED_ASSERT(Namespace != nullptr, -1, "namespace name should be set");
			BeginAccessMask(DefaultMask);
			return BeginNamespace(Namespace);
		}
		int VirtualMachine::EndNamespaceIsolated()
		{
			EndAccessMask();
			return EndNamespace();
		}
		int VirtualMachine::EndNamespace()
		{
			Sync.General.lock();
			int R = Engine->SetDefaultNamespace(DefaultNamespace.c_str());
			Sync.General.unlock();

			return R;
		}
		int VirtualMachine::Namespace(const char* Namespace, const std::function<int(VirtualMachine*)>& Callback)
		{
			ED_ASSERT(Namespace != nullptr, -1, "namespace name should be set");
			ED_ASSERT(Callback, -1, "callback should not be empty");

			int R = BeginNamespace(Namespace);
			if (R < 0)
				return R;

			R = Callback(this);
			if (R < 0)
				return R;

			return EndNamespace();
		}
		int VirtualMachine::NamespaceIsolated(const char* Namespace, size_t DefaultMask, const std::function<int(VirtualMachine*)>& Callback)
		{
			ED_ASSERT(Namespace != nullptr, -1, "namespace name should be set");
			ED_ASSERT(Callback, -1, "callback should not be empty");

			int R = BeginNamespaceIsolated(Namespace, DefaultMask);
			if (R < 0)
				return R;

			R = Callback(this);
			if (R < 0)
				return R;

			return EndNamespaceIsolated();
		}
		size_t VirtualMachine::BeginAccessMask(size_t DefaultMask)
		{
			return Engine->SetDefaultAccessMask((asDWORD)DefaultMask);
		}
		size_t VirtualMachine::EndAccessMask()
		{
			return Engine->SetDefaultAccessMask((asDWORD)VirtualMachine::GetDefaultAccessMask());
		}
		const char* VirtualMachine::GetNamespace() const
		{
			return Engine->GetDefaultNamespace();
		}
		Module VirtualMachine::GetModule(const char* Name)
		{
			ED_ASSERT(Engine != nullptr, Module(nullptr), "engine should be set");
			ED_ASSERT(Name != nullptr, Module(nullptr), "name should be set");

			return Module(Engine->GetModule(Name, asGM_CREATE_IF_NOT_EXISTS));
		}
		int VirtualMachine::SetProperty(Features Property, size_t Value)
		{
			ED_ASSERT(Engine != nullptr, -1, "engine should be set");
			return Engine->SetEngineProperty((asEEngineProp)Property, (asPWORD)Value);
		}
		void VirtualMachine::SetDocumentRoot(const std::string& Value)
		{
			Sync.General.lock();
			Include.Root = Value;
			if (Include.Root.empty())
				return Sync.General.unlock();

			if (!Core::String(&Include.Root).EndsOf("/\\"))
				Include.Root.append(1, ED_PATH_SPLIT);
			Sync.General.unlock();
		}
		std::string VirtualMachine::GetDocumentRoot() const
		{
			return Include.Root;
		}
		std::vector<std::string> VirtualMachine::GetSubmodules() const
		{
			std::vector<std::string> Result;
			for (auto& Module : Modules)
			{
				if (Module.second.Registered)
					Result.push_back(Module.first);
			}

			return Result;
		}
		std::vector<std::string> VirtualMachine::VerifyModules(const std::string& Directory, const Compute::RegexSource& Exp)
		{
			std::vector<std::string> Result;
			if (!Core::OS::Directory::IsExists(Directory.c_str()))
				return Result;

			std::vector<Core::FileEntry> Entries;
			if (!Core::OS::Directory::Scan(Directory, &Entries))
				return Result;

			Compute::RegexResult fResult;
			for (auto& Entry : Entries)
			{
				if (Directory.back() != '/' && Directory.back() != '\\')
					Entry.Path = Directory + '/' + Entry.Path;
				else
					Entry.Path = Directory + Entry.Path;

				if (!Entry.IsDirectory)
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
		bool VirtualMachine::VerifyModule(const std::string& Path)
		{
			ED_ASSERT(Engine != nullptr, false, "engine should be set");
			std::string Source = Core::OS::File::ReadAsString(Path);
			if (Source.empty())
				return true;

			uint32_t Retry = 0;
			Sync.General.lock();
			asIScriptModule* Module = Engine->GetModule("__vfver", asGM_ALWAYS_CREATE);
			Module->AddScriptSection(Path.c_str(), Source.c_str(), Source.size());

			int R = Module->Build();
			while (R == asBUILD_IN_PROGRESS)
			{
				R = Module->Build();
				if (Retry++ > 0)
					std::this_thread::sleep_for(std::chrono::microseconds(100));
			}

			Module->Discard();
			Sync.General.unlock();

			return R >= 0;
		}
		bool VirtualMachine::IsNullable(int TypeId)
		{
			return TypeId == 0;
		}
		bool VirtualMachine::AddSubmodule(const std::string& Name, const std::vector<std::string>& Dependencies, const SubmoduleCallback& Callback)
		{
			ED_ASSERT(!Name.empty(), false, "name should not be empty");
			if (Dependencies.empty() && !Callback)
			{
				std::string Namespace = Name + '/';
				std::vector<std::string> Deps;

				Sync.General.lock();
				for (auto& Item : Modules)
				{
					if (Core::String(&Item.first).StartsWith(Namespace))
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
		bool VirtualMachine::ImportFile(const std::string& Path, std::string* Out)
		{
			if (!(Imports & (uint32_t)Imports::Files))
			{
				ED_ERR("[vm] file import is not allowed");
				return false;
			}

			if (!Core::OS::File::IsExists(Path.c_str()))
				return false;

			if (!Cached)
			{
				if (Out != nullptr)
					Out->assign(Core::OS::File::ReadAsString(Path));

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
			Result = Core::OS::File::ReadAsString(Path);
			if (Out != nullptr)
				Out->assign(Result);

			Sync.General.unlock();
			return true;
		}
		bool VirtualMachine::ImportSymbol(const std::vector<std::string>& Sources, const std::string& Func, const std::string& Decl)
		{
			if (!(Imports & (uint32_t)Imports::CSymbols))
			{
				ED_ERR("[vm] csymbols import is not allowed");
				return false;
			}

			if (!Engine || Decl.empty() || Func.empty())
				return false;

			auto LoadFunction = [this, &Func, &Decl](Kernel& Context, bool Assert) -> bool
			{
				auto Handle = Context.Functions.find(Func);
				if (Handle != Context.Functions.end())
					return true;

				FunctionPtr Function = (FunctionPtr)Core::OS::Symbol::LoadFunction(Context.Handle, Func);
				if (!Function)
				{
					if (Assert)
						ED_ERR("[vm] cannot load shared object function: %s", Func.c_str());

					return false;
				}

				ED_TRACE("[vm] register global funcaddr(%i) %i bytes at 0x%" PRIXPTR, (int)asCALL_CDECL, (int)Decl.size(), (void*)Function);
				if (Engine->RegisterGlobalFunction(Decl.c_str(), asFUNCTION(Function), asCALL_CDECL) < 0)
				{
					if (Assert)
						ED_ERR("[vm] cannot register shared object function: %s", Decl.c_str());

					return false;
				}

				Context.Functions.insert({ Func, (void*)Function });
				ED_DEBUG("[vm] load function %s", Func.c_str());
				return true;
			};

			std::unique_lock<std::mutex> Unique(Sync.General);
			auto It = Kernels.end();
			for (auto& Item : Sources)
			{
				It = Kernels.find(Item);
				if (It != Kernels.end())
					break;
			}

			if (It != Kernels.end())
				return LoadFunction(It->second, true);

			for (auto& Item : Kernels)
			{
				if (LoadFunction(Item.second, false))
					return true;
			}

			ED_ERR("[vm] cannot load shared object function: %s\n\tnot found in any of loaded shared objects", Func.c_str());
			return false;
		}
		bool VirtualMachine::ImportLibrary(const std::string& Path)
		{
			if (!(Imports & (uint32_t)Imports::CLibraries) && !Path.empty())
			{
				ED_ERR("[vm] clibraries import is not allowed");
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
				ED_ERR("[vm] cannot load shared object: %s", Path.c_str());
				return false;
			}

			Kernel Library;
			Library.Handle = Handle;

			Sync.General.lock();
			Kernels.insert({ Name, Library });
			Sync.General.unlock();

			ED_DEBUG("[vm] load library %s", Path.c_str());
			return true;
		}
		bool VirtualMachine::ImportSubmodule(const std::string& Name)
		{
			if (!(Imports & (uint32_t)Imports::Submodules))
			{
				ED_ERR("[vm] submodules import is not allowed");
				return false;
			}

			std::string Target = Name;
			if (Core::String(&Target).EndsWith(".as"))
				Target = Target.substr(0, Target.size() - 3);

			Sync.General.lock();
			auto It = Modules.find(Target);
			if (It == Modules.end())
			{
				Sync.General.unlock();
				ED_ERR("[vm] couldn't find script submodule %s", Name.c_str());
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
					ED_ERR("[vm] couldn't load submodule %s for %s", Item.c_str(), Name.c_str());
					return false;
				}
			}

			ED_DEBUG("[vm] load submodule %s", Name.c_str());
			if (Base.Callback)
				Base.Callback(this);

			return true;
		}
		Core::Schema* VirtualMachine::ImportJSON(const std::string& Path)
		{
			if (!(Imports & (uint32_t)Imports::JSON))
			{
				ED_ERR("[vm] json import is not allowed");
				return nullptr;
			}

			std::string File = Core::OS::Path::Resolve(Path, Include.Root);
			if (!Core::OS::File::IsExists(File.c_str()))
			{
				File = Core::OS::Path::Resolve(Path + ".json", Include.Root);
				if (!Core::OS::File::IsExists(File.c_str()))
				{
					ED_ERR("[vm] %s resource was not found", Path.c_str());
					return nullptr;
				}
			}

			if (!Cached)
			{
				std::string Data = Core::OS::File::ReadAsString(File);
				return Core::Schema::ConvertFromJSON(Data.c_str(), Data.size());
			}

			Sync.General.lock();
			auto It = Datas.find(File);
			if (It != Datas.end())
			{
				Core::Schema* Result = It->second ? It->second->Copy() : nullptr;
				Sync.General.unlock();

				return Result;
			}

			Core::Schema*& Result = Datas[File];
			std::string Data = Core::OS::File::ReadAsString(File);
			Result = Core::Schema::ConvertFromJSON(Data.c_str(), Data.size());

			Core::Schema* Copy = nullptr;
			if (Result != nullptr)
				Copy = Result->Copy();

			Sync.General.unlock();
			return Copy;
		}
		size_t VirtualMachine::GetProperty(Features Property) const
		{
			ED_ASSERT(Engine != nullptr, 0, "engine should be set");
			return (size_t)Engine->GetEngineProperty((asEEngineProp)Property);
		}
		asIScriptEngine* VirtualMachine::GetEngine() const
		{
			return Engine;
		}
		void VirtualMachine::FreeProxy()
		{
			Bindings::Registry::Release();
			CleanupThisThread();
		}
		VirtualMachine* VirtualMachine::Get(asIScriptEngine* Engine)
		{
			ED_ASSERT(Engine != nullptr, nullptr, "engine should be set");
			void* VM = Engine->GetUserData(ManagerUD);
			return (VirtualMachine*)VM;
		}
		VirtualMachine* VirtualMachine::Get()
		{
			asIScriptContext* Context = asGetActiveContext();
			if (!Context)
				return nullptr;

			return Get(Context->GetEngine());
		}
		std::string VirtualMachine::GetLibraryName(const std::string& Path)
		{
			if (Path.empty())
				return Path;

			Core::String Src(Path);
			Core::String::Settle Start = Src.ReverseFindOf("\\/");
			if (Start.Found)
				Src.Substring(Start.End);

			Core::String::Settle End = Src.ReverseFind('.');
			if (End.Found)
				Src.Substring(0, End.End);

			return Src.R();
		}
		asIScriptContext* VirtualMachine::RequestContext(asIScriptEngine* Engine, void* Data)
		{
			VirtualMachine* VM = VirtualMachine::Get(Engine);
			if (!VM)
				return Engine->CreateContext();

			VM->Sync.Pool.lock();
			if (VM->Contexts.empty())
			{
				VM->Sync.Pool.unlock();
				return Engine->CreateContext();
			}

			asIScriptContext* Context = *VM->Contexts.rbegin();
			VM->Contexts.pop_back();
			VM->Sync.Pool.unlock();

			return Context;
		}
		void VirtualMachine::SetMemoryFunctions(void* (*Alloc)(size_t), void(*Free)(void*))
		{
			asSetGlobalMemoryFunctions(Alloc, Free);
		}
		void VirtualMachine::CleanupThisThread()
		{
			asThreadCleanup();
		}
		void VirtualMachine::ReturnContext(asIScriptEngine* Engine, asIScriptContext* Context, void* Data)
		{
			VirtualMachine* VM = VirtualMachine::Get(Engine);
			ED_ASSERT(VM != nullptr, (void)Context->Release(), "engine should be set");

			VM->Sync.Pool.lock();
			VM->Contexts.push_back(Context);
			Context->Unprepare();
			VM->Sync.Pool.unlock();
		}
		void VirtualMachine::CompileLogger(asSMessageInfo* Info, void* This)
		{
			VirtualMachine* Engine = (VirtualMachine*)This;
			const char* Section = (Info->section && Info->section[0] != '\0' ? Info->section : "any");
			if (Engine->WherError)
				Engine->WherError();

			if (Engine != nullptr && !Engine->Callbacks.empty())
			{
				auto It = Engine->Callbacks.find(Section);
				if (It != Engine->Callbacks.end())
				{
					if (Info->type == asMSGTYPE_WARNING)
						return It->second(Core::Form("WARN anonymous.as (%i, %i): %s", Info->row, Info->col, Info->message).R());
					else if (Info->type == asMSGTYPE_INFORMATION)
						return It->second(Core::Form("INFO anonymous.as (%i, %i): %s", Info->row, Info->col, Info->message).R());

					return It->second(Core::Form("ERR anonymous.as (%i, %i): %s", Info->row, Info->col, Info->message).R());
				}
			}

			if (Info->type == asMSGTYPE_WARNING)
				ED_WARN("[compiler]\n\t%s (%i, %i): %s", Section, Info->row, Info->col, Info->message);
			else if (Info->type == asMSGTYPE_INFORMATION)
				ED_INFO("[compiler]\n\t%s (%i, %i): %s", Section, Info->row, Info->col, Info->message);
			else if (Info->type == asMSGTYPE_ERROR)
				ED_ERR("[compiler]\n\t%s (%i, %i): %s", Section, Info->row, Info->col, Info->message);
		}
		void VirtualMachine::RegisterSubmodules(VirtualMachine* Engine)
		{
			Engine->AddSubmodule("std/ctypes", { }, Bindings::Registry::LoadCTypes);
			Engine->AddSubmodule("std/any", { }, Bindings::Registry::LoadAny);
			Engine->AddSubmodule("std/array", { "std/ctypes" }, Bindings::Registry::LoadArray);
			Engine->AddSubmodule("std/complex", { }, Bindings::Registry::LoadComplex);
			Engine->AddSubmodule("std/ref", { }, Bindings::Registry::LoadRef);
			Engine->AddSubmodule("std/weak_ref", { }, Bindings::Registry::LoadWeakRef);
			Engine->AddSubmodule("std/math", { }, Bindings::Registry::LoadMath);
			Engine->AddSubmodule("std/string", { "std/array" }, Bindings::Registry::LoadString);
			Engine->AddSubmodule("std/random", { "std/string" }, Bindings::Registry::LoadRandom);
			Engine->AddSubmodule("std/dictionary", { "std/array", "std/string" }, Bindings::Registry::LoadDictionary);
			Engine->AddSubmodule("std/exception", { "std/string" }, Bindings::Registry::LoadException);
			Engine->AddSubmodule("std/mutex", { }, Bindings::Registry::LoadMutex);
			Engine->AddSubmodule("std/thread", { "std/any", "std/string" }, Bindings::Registry::LoadThread);
			Engine->AddSubmodule("std/promise", { "std/any" }, Bindings::Registry::LoadPromise);
			Engine->AddSubmodule("std/format", { "std/string" }, Bindings::Registry::LoadFormat);
			Engine->AddSubmodule("std/decimal", { "std/string" }, Bindings::Registry::LoadDecimal);
			Engine->AddSubmodule("std/variant", { "std/string", "std/decimal" }, Bindings::Registry::LoadVariant);
			Engine->AddSubmodule("std/timestamp", { "std/string" }, Bindings::Registry::LoadTimestamp);
			Engine->AddSubmodule("std/console", { "std/format" }, Bindings::Registry::LoadConsole);
			Engine->AddSubmodule("std/schema", { "std/array", "std/string", "std/dictionary", "std/variant" }, Bindings::Registry::LoadSchema);
			Engine->AddSubmodule("std/schedule", { "std/ctypes" }, Bindings::Registry::LoadSchedule);
			Engine->AddSubmodule("std/clock_timer", { }, Bindings::Registry::LoadClockTimer);
			Engine->AddSubmodule("std/file_system", { "std/string" }, Bindings::Registry::LoadFileSystem);
			Engine->AddSubmodule("std/os", { "std/file_system", "std/array" }, Bindings::Registry::LoadOS);
			Engine->AddSubmodule("std/vertices", { }, Bindings::Registry::LoadVertices);
			Engine->AddSubmodule("std/vectors", { }, Bindings::Registry::LoadVectors);
			Engine->AddSubmodule("std/shapes", { "std/vectors" }, Bindings::Registry::LoadShapes);
			Engine->AddSubmodule("std/key_frames", { "std/vectors", "std/string" }, Bindings::Registry::LoadKeyFrames);
			Engine->AddSubmodule("std/regex", { "std/string" }, Bindings::Registry::LoadRegex);
			Engine->AddSubmodule("std/crypto", { "std/string" }, Bindings::Registry::LoadCrypto);
			Engine->AddSubmodule("std/geometric", { "std/vectors", "std/vertices", "std/shapes" }, Bindings::Registry::LoadGeometric);
			Engine->AddSubmodule("std/preprocessor", { "std/string" }, Bindings::Registry::LoadPreprocessor);
			Engine->AddSubmodule("std/physics", { "std/string", "std/geometric" }, Bindings::Registry::LoadPhysics);
			Engine->AddSubmodule("std/audio", { "std/string", "std/vectors" }, Bindings::Registry::LoadAudio);
			Engine->AddSubmodule("std/activity", { "std/string", "std/vectors" }, Bindings::Registry::LoadActivity);
			Engine->AddSubmodule("std/graphics", { "std/activity", "std/string", "std/vectors", "std/vertices", "std/shapes", "std/key_frames" }, Bindings::Registry::LoadGraphics);
			Engine->AddSubmodule("std/network", { "std/string", "std/array", "std/dictionary" }, Bindings::Registry::LoadNetwork);
			Engine->AddSubmodule("std/vm", { "std/string" }, Bindings::Registry::LoadVM);
			Engine->AddSubmodule("std/gui/control", { "std/vectors", "std/schema", "std/array" }, Bindings::Registry::LoadUiControl);
			Engine->AddSubmodule("std/gui/model", { "std/gui/control", }, Bindings::Registry::LoadUiModel);
			Engine->AddSubmodule("std/gui/context", { "std/gui/model" }, Bindings::Registry::LoadUiContext);
			Engine->AddSubmodule("std/engine", { "std/schema", "std/key_frames", "std/file_system", "std/graphics", "std/audio", "std/physics", "std/clock_timer", "std/vm", "std/gui/context" }, Bindings::Registry::LoadEngine);
			Engine->AddSubmodule("std/components", { "std/engine" }, Bindings::Registry::LoadComponents);
			Engine->AddSubmodule("std/renderers", { "std/engine" }, Bindings::Registry::LoadRenderers);
			Engine->AddSubmodule("std", { }, nullptr);
		}
		size_t VirtualMachine::GetDefaultAccessMask()
		{
			return 0xFFFFFFFF;
		}
		void* VirtualMachine::GetNullable()
		{
			return nullptr;
		}
		int VirtualMachine::ManagerUD = 553;

		Debugger::Debugger() noexcept : LastFunction(nullptr), VM(nullptr), Action(DebugAction::CONTINUE)
		{
			LastCommandAtStackLevel = 0;
		}
		Debugger::~Debugger() noexcept
		{
			SetEngine(0);
		}
		void Debugger::RegisterToStringCallback(const TypeInfo& Type, ToStringCallback Callback)
		{
			ED_ASSERT_V(ToStringCallbacks.find(Type.GetTypeInfo()) == ToStringCallbacks.end(), "callback should not already be set");
			ToStringCallbacks.insert(std::unordered_map<const asITypeInfo*, ToStringCallback>::value_type(Type.GetTypeInfo(), Callback));
		}
		void Debugger::LineCallback(ImmediateContext* Context)
		{
			ED_ASSERT_V(Context != nullptr, "context should be set");
			asIScriptContext* Base = Context->GetContext();

			ED_ASSERT_V(Base != nullptr, "context should be set");
			ED_ASSERT_V(Base->GetState() == asEXECUTION_ACTIVE, "context should be active");

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
		void Debugger::TakeCommands(ImmediateContext* Context)
		{
			ED_ASSERT_V(Context != nullptr, "context should be set");

			asIScriptContext* Base = Context->GetContext();
			ED_ASSERT_V(Base != nullptr, "context should be set");

			for (;;)
			{
				char Buffer[512];
				Output("[dbg]> ");
				std::cin.getline(Buffer, 512);

				if (InterpretCommand(std::string(Buffer), Context))
					break;
			}
		}
		void Debugger::PrintValue(const std::string& Expression, ImmediateContext* Context)
		{
			ED_ASSERT_V(Context != nullptr, "context should be set");

			asIScriptContext* Base = Context->GetContext();
			ED_ASSERT_V(Base != nullptr, "context should be set");

			asIScriptEngine* Engine = Context->GetVM()->GetEngine();
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

			asIScriptFunction* Function = Base->GetFunction();
			if (!Function)
				return;

			if (Scope.empty())
			{
				for (asUINT n = Function->GetVarCount(); n-- > 0;)
				{
					const char* VarName = 0;
					Base->GetVar(n, 0, &VarName, &TypeId);
					if (Base->IsVarInScope(n) && VarName != 0 && Name == VarName)
					{
						Pointer = Base->GetAddressOfVar(n);
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
						asITypeInfo* Type = Engine->GetTypeInfoById(Base->GetThisTypeId());
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

				asIScriptModule* Mod = Function->GetModule();
				if (Mod != nullptr)
				{
					for (asUINT n = 0; n < Mod->GetGlobalVarCount(); n++)
					{
						const char* VarName = 0, * Namespace = 0;
						Mod->GetGlobalVar(n, &VarName, &Namespace, &TypeId);

						if (Name == VarName && Scope == Namespace)
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
				Stream << ToString(Pointer, TypeId, 3, VM) << std::endl;
				Output(Stream.str());
			}
			else
				Output("Invalid expression. No matching symbol\n");
		}
		void Debugger::ListBreakPoints()
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
		void Debugger::ListMemberProperties(ImmediateContext* Context)
		{
			ED_ASSERT_V(Context != nullptr, "context should be set");

			asIScriptContext* Base = Context->GetContext();
			ED_ASSERT_V(Base != nullptr, "context should be set");

			void* Pointer = Base->GetThisPointer();
			if (Pointer != nullptr)
			{
				std::stringstream Stream;
				Stream << "this = " << ToString(Pointer, Base->GetThisTypeId(), 3, VirtualMachine::Get(Base->GetEngine())) << std::endl;
				Output(Stream.str());
			}
		}
		void Debugger::ListLocalVariables(ImmediateContext* Context)
		{
			ED_ASSERT_V(Context != nullptr, "context should be set");

			asIScriptContext* Base = Context->GetContext();
			ED_ASSERT_V(Base != nullptr, "context should be set");

			asIScriptFunction* Function = Base->GetFunction();
			if (!Function)
				return;

			std::stringstream Stream;
			for (asUINT n = 0; n < Function->GetVarCount(); n++)
			{
				if (Base->IsVarInScope(n))
				{
					int TypeId;
					Base->GetVar(n, 0, 0, &TypeId);
					Stream << Function->GetVarDecl(n) << " = " << ToString(Base->GetAddressOfVar(n), TypeId, 3, VirtualMachine::Get(Base->GetEngine())) << std::endl;
				}
			}
			Output(Stream.str());
		}
		void Debugger::ListGlobalVariables(ImmediateContext* Context)
		{
			ED_ASSERT_V(Context != nullptr, "context should be set");

			asIScriptContext* Base = Context->GetContext();
			ED_ASSERT_V(Base != nullptr, "context should be set");

			asIScriptFunction* Function = Base->GetFunction();
			if (!Function)
				return;

			asIScriptModule* Mod = Function->GetModule();
			if (!Mod)
				return;

			std::stringstream Stream;
			for (asUINT n = 0; n < Mod->GetGlobalVarCount(); n++)
			{
				int TypeId = 0;
				Mod->GetGlobalVar(n, nullptr, nullptr, &TypeId);
				Stream << Mod->GetGlobalVarDeclaration(n) << " = " << ToString(Mod->GetAddressOfGlobalVar(n), TypeId, 3, VirtualMachine::Get(Base->GetEngine())) << std::endl;
			}
			Output(Stream.str());
		}
		void Debugger::ListStatistics(ImmediateContext* Context)
		{
			ED_ASSERT_V(Context != nullptr, "context should be set");

			asIScriptContext* Base = Context->GetContext();
			ED_ASSERT_V(Base != nullptr, "context should be set");

			asIScriptEngine* Engine = Base->GetEngine();
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
		void Debugger::PrintCallstack(ImmediateContext* Context)
		{
			ED_ASSERT_V(Context != nullptr, "context should be set");

			asIScriptContext* Base = Context->GetContext();
			ED_ASSERT_V(Base != nullptr, "context should be set");

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
		void Debugger::AddFuncBreakPoint(const std::string& Function)
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
		void Debugger::AddFileBreakPoint(const std::string& File, int LineNumber)
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
		void Debugger::PrintHelp()
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
		void Debugger::Output(const std::string& Data)
		{
			std::cout << Data;
		}
		void Debugger::SetEngine(VirtualMachine* Engine)
		{
			if (Engine != nullptr && Engine != VM)
			{
				if (VM != nullptr)
					VM->GetEngine()->Release();

				VM = Engine;
				VM->GetEngine()->AddRef();
			}
		}
		bool Debugger::CheckBreakPoint(ImmediateContext* Context)
		{
			ED_ASSERT(Context != nullptr, false, "context should be set");

			asIScriptContext* Base = Context->GetContext();
			ED_ASSERT(Base != nullptr, false, "context should be set");

			const char* Temp = 0;
			int Line = Base->GetLineNumber(0, 0, &Temp);

			std::string File = Temp ? Temp : "";
			size_t R = File.find_last_of("\\/");
			if (R != std::string::npos)
				File = File.substr(R + 1);

			asIScriptFunction* Function = Base->GetFunction();
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
		bool Debugger::InterpretCommand(const std::string& Command, ImmediateContext* Context)
		{
			ED_ASSERT(Context != nullptr, false, "context should be set");

			asIScriptContext* Base = Context->GetContext();
			ED_ASSERT(Base != nullptr, false, "context should be set");

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
						int Number = Core::String(&Line).ToInt();

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
							int NBR = Core::String(&BR).ToInt();
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
		std::string Debugger::ToString(void* Value, unsigned int TypeId, int ExpandMembers, VirtualMachine* Engine)
		{
			if (Value == 0)
				return "<null>";

			if (Engine == 0)
				Engine = VM;

			if (!VM || !Engine)
				return "<null>";

			asIScriptEngine* Base = Engine->GetEngine();
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

				asITypeInfo* T = Base->GetTypeInfoById(TypeId);
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
					asITypeInfo* Type = Object->GetObjectType();
					for (asUINT n = 0; n < Object->GetPropertyCount(); n++)
					{
						if (n == 0)
							Stream << " ";
						else
							Stream << ", ";

						Stream << Type->GetPropertyDeclaration(n) << " = " << ToString(Object->GetAddressOfProperty(n), Object->GetPropertyTypeId(n), ExpandMembers - 1, VirtualMachine::Get(Type->GetEngine()));
					}
				}
			}
			else
			{
				if (TypeId & asTYPEID_OBJHANDLE)
					Value = *(void**)Value;

				asITypeInfo* Type = Base->GetTypeInfoById(TypeId);
				if (Type->GetFlags() & asOBJ_REF)
					Stream << "{" << Value << "}";

				if (Value != nullptr)
				{
					auto It = ToStringCallbacks.find(Type);
					if (It == ToStringCallbacks.end())
					{
						if (Type->GetFlags() & asOBJ_TEMPLATE)
						{
							asITypeInfo* TmpType = Base->GetTypeInfoByName(Type->GetName());
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
		VirtualMachine* Debugger::GetEngine()
		{
			return VM;
		}
	}
}