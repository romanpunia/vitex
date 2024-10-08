#include "scripting.h"
#include "bindings.h"
#include "vitex.h"
#include <sstream>
#define ADDON_ANY "any"
#ifdef VI_ANGELSCRIPT
#include <angelscript.h>
#include <as_texts.h>
#define COMPILER_BLOCKED_WAIT_US 100
#define THREAD_BLOCKED_WAIT_MS 50

namespace
{
	class CByteCodeStream : public asIBinaryStream
	{
	private:
		Vitex::Core::Vector<asBYTE> Code;
		int ReadPos, WritePos;

	public:
		CByteCodeStream() : ReadPos(0), WritePos(0)
		{
		}
		CByteCodeStream(const Vitex::Core::Vector<asBYTE>& Data) : Code(Data), ReadPos(0), WritePos(0)
		{
		}
		int Read(void* Ptr, asUINT Size)
		{
			VI_ASSERT(Ptr && Size, "corrupted read");
			memcpy(Ptr, &Code[ReadPos], Size);
			ReadPos += Size;

			return 0;
		}
		int Write(const void* Ptr, asUINT Size)
		{
			VI_ASSERT(Ptr && Size, "corrupted write");
			Code.resize(Code.size() + Size);
			memcpy(&Code[WritePos], Ptr, Size);
			WritePos += Size;

			return 0;
		}
		Vitex::Core::Vector<asBYTE>& GetCode()
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
		Vitex::Core::Vector<Vitex::Core::String> Values;
	};

	struct DClass
	{
		Vitex::Core::Vector<Vitex::Core::String> Props;
		Vitex::Core::Vector<Vitex::Core::String> Interfaces;
		Vitex::Core::Vector<Vitex::Core::String> Types;
		Vitex::Core::Vector<Vitex::Core::String> Funcdefs;
		Vitex::Core::Vector<Vitex::Core::String> Methods;
		Vitex::Core::Vector<Vitex::Core::String> Functions;
	};

	struct DNamespace
	{
		Vitex::Core::UnorderedMap<Vitex::Core::String, DEnum> Enums;
		Vitex::Core::UnorderedMap<Vitex::Core::String, DClass> Classes;
		Vitex::Core::Vector<Vitex::Core::String> Funcdefs;
		Vitex::Core::Vector<Vitex::Core::String> Functions;
	};

	Vitex::Core::String GetCombination(const Vitex::Core::Vector<Vitex::Core::String>& Names, const Vitex::Core::String& By)
	{
		Vitex::Core::String Result;
		for (size_t i = 0; i < Names.size(); i++)
		{
			Result.append(Names[i]);
			if (i + 1 < Names.size())
				Result.append(By);
		}

		return Result;
	}
	Vitex::Core::String GetCombinationAll(const Vitex::Core::Vector<Vitex::Core::String>& Names, const Vitex::Core::String& By, const Vitex::Core::String& EndBy)
	{
		Vitex::Core::String Result;
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
	Vitex::Core::String GetTypeNaming(asITypeInfo* Type)
	{
		const char* Namespace = Type->GetNamespace();
		return (Namespace ? Namespace + Vitex::Core::String("::") : Vitex::Core::String("")) + Type->GetName();
	}
	asITypeInfo* GetTypeNamespacing(asIScriptEngine* Engine, const Vitex::Core::String& Name)
	{
		asITypeInfo* Result = Engine->GetTypeInfoByName(Name.c_str());
		if (Result != nullptr)
			return Result;

		return Engine->GetTypeInfoByName((Name + "@").c_str());
	}
	void DumpNamespace(Vitex::Core::String& Source, const Vitex::Core::String& Naming, DNamespace& Namespace, Vitex::Core::String& Offset)
	{
		if (!Naming.empty())
		{
			Offset.append("\t");
			Source += Vitex::Core::Stringify::Text("namespace %s\n{\n", Naming.c_str());
		}

		for (auto It = Namespace.Enums.begin(); It != Namespace.Enums.end(); It++)
		{
			auto Copy = It;
			Source += Vitex::Core::Stringify::Text("%senum %s\n%s{\n\t%s", Offset.c_str(), It->first.c_str(), Offset.c_str(), Offset.c_str());
			Source += Vitex::Core::Stringify::Text("%s", GetCombination(It->second.Values, ",\n\t" + Offset).c_str());
			Source += Vitex::Core::Stringify::Text("\n%s}\n%s", Offset.c_str(), ++Copy != Namespace.Enums.end() ? "\n" : "");
		}

		if (!Namespace.Enums.empty() && (!Namespace.Classes.empty() || !Namespace.Funcdefs.empty() || !Namespace.Functions.empty()))
			Source += Vitex::Core::Stringify::Text("\n");

		for (auto It = Namespace.Classes.begin(); It != Namespace.Classes.end(); It++)
		{
			auto Copy = It;
			Source += Vitex::Core::Stringify::Text("%sclass %s%s%s%s%s%s\n%s{\n\t%s",
				Offset.c_str(),
				It->first.c_str(),
				It->second.Types.empty() ? "" : "<",
				It->second.Types.empty() ? "" : GetCombination(It->second.Types, ", ").c_str(),
				It->second.Types.empty() ? "" : ">",
				It->second.Interfaces.empty() ? "" : " : ",
				It->second.Interfaces.empty() ? "" : GetCombination(It->second.Interfaces, ", ").c_str(),
				Offset.c_str(), Offset.c_str());
			Source += Vitex::Core::Stringify::Text("%s", GetCombinationAll(It->second.Funcdefs, ";\n\t" + Offset, It->second.Props.empty() && It->second.Methods.empty() ? ";" : ";\n\n\t" + Offset).c_str());
			Source += Vitex::Core::Stringify::Text("%s", GetCombinationAll(It->second.Props, ";\n\t" + Offset, It->second.Methods.empty() ? ";" : ";\n\n\t" + Offset).c_str());
			Source += Vitex::Core::Stringify::Text("%s", GetCombinationAll(It->second.Methods, ";\n\t" + Offset, ";").c_str());
			Source += Vitex::Core::Stringify::Text("\n%s}\n%s", Offset.c_str(), !It->second.Functions.empty() || ++Copy != Namespace.Classes.end() ? "\n" : "");

			if (It->second.Functions.empty())
				continue;

			Source += Vitex::Core::Stringify::Text("%snamespace %s\n%s{\n\t%s", Offset.c_str(), It->first.c_str(), Offset.c_str(), Offset.c_str());
			Source += Vitex::Core::Stringify::Text("%s", GetCombinationAll(It->second.Functions, ";\n\t" + Offset, ";").c_str());
			Source += Vitex::Core::Stringify::Text("\n%s}\n%s", Offset.c_str(), ++Copy != Namespace.Classes.end() ? "\n" : "");
		}

		if (!Namespace.Funcdefs.empty())
		{
			if (!Namespace.Enums.empty() || !Namespace.Classes.empty())
				Source += Vitex::Core::Stringify::Text("\n%s", Offset.c_str());
			else
				Source += Vitex::Core::Stringify::Text("%s", Offset.c_str());
		}

		Source += Vitex::Core::Stringify::Text("%s", GetCombinationAll(Namespace.Funcdefs, ";\n" + Offset, Namespace.Functions.empty() ? ";\n" : "\n\n" + Offset).c_str());
		if (!Namespace.Functions.empty() && Namespace.Funcdefs.empty())
		{
			if (!Namespace.Enums.empty() || !Namespace.Classes.empty())
				Source += Vitex::Core::Stringify::Text("\n");
			else
				Source += Vitex::Core::Stringify::Text("%s", Offset.c_str());
		}

		Source += Vitex::Core::Stringify::Text("%s", GetCombinationAll(Namespace.Functions, ";\n" + Offset, ";\n").c_str());
		if (!Naming.empty())
		{
			Source += Vitex::Core::Stringify::Text("}");
			Offset.erase(Offset.begin());
		}
		else
			Source.erase(Source.end() - 1);
	}
}
#endif

namespace Vitex
{
	namespace Scripting
	{
		static Core::Vector<Core::String> ExtractLinesOfCode(const std::string_view& Code, int Line, int Max)
		{
			Core::Vector<Core::String> Total;
			size_t Start = 0, Size = Code.size();
			size_t Offset = 0, Lines = 0;
			size_t LeftSide = (Max - 1) / 2;
			size_t RightSide = (Max - 1) - LeftSide;
			size_t BaseRightSide = (RightSide > 0 ? RightSide - 1 : 0);

			VI_ASSERT(Max > 0, "max lines count should be at least one");
			while (Offset < Size)
			{
				if (Code[Offset++] != '\n')
				{
					if (Offset != Size)
						continue;
				}

				++Lines;
				if (Lines >= Line - LeftSide && LeftSide > 0)
				{
					Core::String Copy = Core::String(Code.substr(Start, Offset - Start));
					Core::Stringify::ReplaceOf(Copy, "\r\n\t\v", " ");
					Total.push_back(std::move(Copy));
					--LeftSide; --Max;
				}
				else if (Lines == Line)
				{
					Core::String Copy = Core::String(Code.substr(Start, Offset - Start));
					Core::Stringify::ReplaceOf(Copy, "\r\n\t\v", " ");
					Total.insert(Total.begin(), std::move(Copy));
					--Max;
				}
				else if (Lines >= Line + (RightSide - BaseRightSide) && RightSide > 0)
				{
					Core::String Copy = Core::String(Code.substr(Start, Offset - Start));
					Core::Stringify::ReplaceOf(Copy, "\r\n\t\v", " ");
					Total.push_back(std::move(Copy));
					--RightSide; --Max;
				}

				Start = Offset;
				if (!Max)
					break;
			}

			for (auto& Item : Total)
			{
				if (!Core::Stringify::IsEmptyOrWhitespace(Item))
					return Total;
			}

			Total.clear();
			return Total;
		}
		static Core::String CharTrimEnd(const std::string_view& Value)
		{
			Core::String Copy = Core::String(Value);
			Core::Stringify::TrimEnd(Copy);
			return Copy;
		}
		static const char* OrEmpty(const char* Value)
		{
			return Value ? Value : "";
		}

		VirtualException::VirtualException(VirtualError NewErrorCode) : ErrorCode(NewErrorCode)
		{
			if (ErrorCode != VirtualError::SUCCESS)
				Message += VirtualMachine::GetErrorNameInfo(ErrorCode);
		}
		VirtualException::VirtualException(VirtualError NewErrorCode, Core::String&& NewMessage) : ErrorCode(NewErrorCode)
		{
			Message = std::move(NewMessage);
			if (ErrorCode == VirtualError::SUCCESS)
				return;

			Message += " CAUSING ";
			Message += VirtualMachine::GetErrorNameInfo(ErrorCode);
		}
		VirtualException::VirtualException(Core::String&& NewMessage)
		{
			Message = std::move(NewMessage);
		}
		const char* VirtualException::type() const noexcept
		{
			return "virtual_error";
		}
		VirtualError VirtualException::error_code() const noexcept
		{
			return ErrorCode;
		}

		uint64_t TypeCache::Set(uint64_t Id, const std::string_view& Name)
		{
			VI_ASSERT(Id > 0 && !Name.empty(), "id should be greater than zero and name should not be empty");

			using Map = Core::UnorderedMap<uint64_t, std::pair<Core::String, int>>;
			if (!Names)
				Names = Core::Memory::New<Map>();

			(*Names)[Id] = std::make_pair(Name, (int)-1);
			return Id;
		}
		int TypeCache::GetTypeId(uint64_t Id)
		{
			auto It = Names->find(Id);
			if (It == Names->end())
				return -1;

			if (It->second.second < 0)
			{
				VirtualMachine* Engine = VirtualMachine::Get();
				VI_ASSERT(Engine != nullptr, "engine should be set");
				It->second.second = Engine->GetTypeIdByDecl(It->second.first.c_str());
			}

			return It->second.second;
		}
		void TypeCache::Cleanup()
		{
			Core::Memory::Delete(Names);
		}
		Core::UnorderedMap<uint64_t, std::pair<Core::String, int>>* TypeCache::Names = nullptr;

		ExpectsVM<void> Parser::ReplaceInlinePreconditions(const std::string_view& Keyword, Core::String& Data, const std::function<ExpectsVM<Core::String>(const std::string_view& Expression)>& Replacer)
		{
			return ReplacePreconditions(false, Core::String(Keyword) + ' ', Data, Replacer);
		}
		ExpectsVM<void> Parser::ReplaceDirectivePreconditions(const std::string_view& Keyword, Core::String& Data, const std::function<ExpectsVM<Core::String>(const std::string_view& Expression)>& Replacer)
		{
			return ReplacePreconditions(true, Keyword, Data, Replacer);
		}
		ExpectsVM<void> Parser::ReplacePreconditions(bool IsDirective, const std::string_view& Match, Core::String& Code, const std::function<ExpectsVM<Core::String>(const std::string_view& Expression)>& Replacer)
		{
			VI_ASSERT(!Match.empty(), "keyword should not be empty");
			VI_ASSERT(Replacer != nullptr, "replacer callback should not be empty");
			size_t MatchSize = Match.size();
			size_t Offset = 0;

			while (Offset < Code.size())
			{
				char U = Code[Offset];
				if (U == '/' && Offset + 1 < Code.size() && (Code[Offset + 1] == '/' || Code[Offset + 1] == '*'))
				{
					if (Code[++Offset] == '*')
					{
						while (Offset + 1 < Code.size())
						{
							char N = Code[Offset++];
							if (N == '*' && Code[Offset++] == '/')
								break;
						}
					}
					else
					{
						while (Offset < Code.size())
						{
							char N = Code[Offset++];
							if (N == '\r' || N == '\n')
								break;
						}
					}

					continue;
				}
				else if (U == '\"' || U == '\'')
				{
					++Offset;
					while (Offset < Code.size())
					{
						if (Code[Offset++] == U && Core::Stringify::IsNotPrecededByEscape(Code, Offset - 1))
							break;
					}

					continue;
				}
				else if (Code.size() - Offset < MatchSize || memcmp(Code.c_str() + Offset, Match.data(), MatchSize) != 0)
				{
					++Offset;
					continue;
				}

				size_t Start = Offset + MatchSize;
				while (Start < Code.size())
				{
					if (!Core::Stringify::IsWhitespace(Code[Start]))
						break;
					++Start;
				}

				int32_t Brackets = 0;
				size_t End = Start;
				while (End < Code.size())
				{
					char V = Code[End];
					if (V == ')')
					{
						if (--Brackets < 0)
							break;
					}
					else if (V == '"' || V == '\'')
					{
						++End;
						while (End < Code.size() && (Code[End++] != V || !Core::Stringify::IsNotPrecededByEscape(Code, End - 1)));
						--End;
					}
					else if (V == ';')
					{
						if (IsDirective)
							++End;
						break;
					}
					else if (V == '(')
						++Brackets;
					End++;
				}

				if (Brackets > 0)
					return VirtualException(Core::Stringify::Text("unexpected end of file, expected closing ')' symbol, offset %" PRIu64 ":\n%.*s <<<", (uint64_t)End, (int)(End - Start), Code.c_str() + Start));
				
				if (End == Start)
				{
					Offset = End;
					continue;
				}

				auto Expression = Replacer(Code.substr(Start, End - Start));
				if (!Expression)
					return Expression.Error();

				Core::Stringify::ReplacePart(Code, Offset, End, *Expression);
				if (Expression->find(Match) == std::string::npos)
					Offset += Expression->size();
			}

			return Core::Expectation::Met;
		}

		ExpectsVM<void> FunctionFactory::AtomicNotifyGC(const std::string_view& TypeName, void* Object)
		{
			VI_ASSERT(Object != nullptr, "object should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptContext* Context = asGetActiveContext();
			VI_ASSERT(Context != nullptr, "context should be set");

			VirtualMachine* Engine = VirtualMachine::Get(Context->GetEngine());
			VI_ASSERT(Engine != nullptr, "engine should be set");

			TypeInfo Type = Engine->GetTypeInfoByName(TypeName);
			return Engine->NotifyOfNewObject(Object, Type.GetTypeInfo());
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> FunctionFactory::AtomicNotifyGCById(int TypeId, void* Object)
		{
			VI_ASSERT(Object != nullptr, "object should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptContext* Context = asGetActiveContext();
			VI_ASSERT(Context != nullptr, "context should be set");

			VirtualMachine* Engine = VirtualMachine::Get(Context->GetEngine());
			VI_ASSERT(Engine != nullptr, "engine should be set");

			TypeInfo Type = Engine->GetTypeInfoById(TypeId);
			return Engine->NotifyOfNewObject(Object, Type.GetTypeInfo());
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		asSFuncPtr* FunctionFactory::CreateFunctionBase(void(*Base)(), int Type)
		{
			VI_ASSERT(Base != nullptr, "function pointer should be set");
#ifdef VI_ANGELSCRIPT
			asSFuncPtr* Ptr = Core::Memory::New<asSFuncPtr>(Type);
			Ptr->ptr.f.func = reinterpret_cast<asFUNCTION_t>(Base);
			return Ptr;
#else
			return nullptr;
#endif
		}
		asSFuncPtr* FunctionFactory::CreateMethodBase(const void* Base, size_t Size, int Type)
		{
			VI_ASSERT(Base != nullptr, "function pointer should be set");
#ifdef VI_ANGELSCRIPT
			asSFuncPtr* Ptr = Core::Memory::New<asSFuncPtr>(Type);
			Ptr->CopyMethodPtr(Base, Size);
			return Ptr;
#else
			return nullptr;
#endif
		}
		asSFuncPtr* FunctionFactory::CreateDummyBase()
		{
#ifdef VI_ANGELSCRIPT
			return Core::Memory::New<asSFuncPtr>(0);
#else
			return nullptr;
#endif
		}
		void FunctionFactory::ReleaseFunctor(asSFuncPtr** Ptr)
		{
			if (!Ptr || !*Ptr)
				return;
#ifdef VI_ANGELSCRIPT
			Core::Memory::Delete(*Ptr);
#endif
		}
		void FunctionFactory::GCEnumCallback(asIScriptEngine* Engine, void* Reference)
		{
#ifdef VI_ANGELSCRIPT
			if (Reference != nullptr)
				Engine->GCEnumCallback(Reference);
#endif
		}
		void FunctionFactory::GCEnumCallback(asIScriptEngine* Engine, asIScriptFunction* Reference)
		{
			if (!Reference)
				return;
#ifdef VI_ANGELSCRIPT
			Engine->GCEnumCallback(Reference);
			GCEnumCallback(Engine, Reference->GetDelegateFunction());
			GCEnumCallback(Engine, Reference->GetDelegateObject());
			GCEnumCallback(Engine, Reference->GetDelegateObjectType());
#endif
		}
		void FunctionFactory::GCEnumCallback(asIScriptEngine* Engine, FunctionDelegate* Reference)
		{
			if (Reference && Reference->IsValid())
				GCEnumCallback(Engine, Reference->Callback);
		}

		MessageInfo::MessageInfo(asSMessageInfo* Msg) noexcept : Info(Msg)
		{
		}
		std::string_view MessageInfo::GetSection() const
		{
			VI_ASSERT(IsValid(), "message should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Info->section);
#else
			return OrEmpty("");
#endif
		}
		std::string_view MessageInfo::GetText() const
		{
			VI_ASSERT(IsValid(), "message should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Info->message);
#else
			return "";
#endif
		}
		LogCategory MessageInfo::GetType() const
		{
			VI_ASSERT(IsValid(), "message should be valid");
#ifdef VI_ANGELSCRIPT
			return (LogCategory)Info->type;
#else
			return LogCategory::ERR;
#endif
		}
		int MessageInfo::GetRow() const
		{
			VI_ASSERT(IsValid(), "message should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->row;
#else
			return 0;
#endif
		}
		int MessageInfo::GetColumn() const
		{
			VI_ASSERT(IsValid(), "message should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->col;
#else
			return 0;
#endif
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
#ifdef VI_ANGELSCRIPT
			VM = (Info ? VirtualMachine::Get(Info->GetEngine()) : nullptr);
#endif
		}
		void TypeInfo::ForEachProperty(const PropertyCallback& Callback)
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
			VI_ASSERT(Callback, "typeinfo should not be empty");
#ifdef VI_ANGELSCRIPT
			unsigned int Count = Info->GetPropertyCount();
			for (unsigned int i = 0; i < Count; i++)
			{
				FunctionInfo Prop;
				if (GetProperty(i, &Prop))
					Callback(this, &Prop);
			}
#endif
		}
		void TypeInfo::ForEachMethod(const MethodCallback& Callback)
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
			VI_ASSERT(Callback, "typeinfo should not be empty");
#ifdef VI_ANGELSCRIPT
			unsigned int Count = Info->GetMethodCount();
			for (unsigned int i = 0; i < Count; i++)
			{
				Function Method = Info->GetMethodByIndex(i);
				if (Method.IsValid())
					Callback(this, &Method);
			}
#endif
		}
		std::string_view TypeInfo::GetGroup() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Info->GetConfigGroup());
#else
			return "";
#endif
		}
		size_t TypeInfo::GetAccessMask() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetAccessMask();
#else
			return 0;
#endif
		}
		Module TypeInfo::GetModule() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetModule();
#else
			return Module(nullptr);
#endif
		}
		void TypeInfo::AddRef() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			Info->AddRef();
#endif
		}
		void TypeInfo::Release()
		{
			if (!IsValid())
				return;
#ifdef VI_ANGELSCRIPT
			Core::Memory::Release(Info);
#endif
		}
		std::string_view TypeInfo::GetName() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Info->GetName());
#else
			return "";
#endif
		}
		std::string_view TypeInfo::GetNamespace() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Info->GetNamespace());
#else
			return "";
#endif
		}
		TypeInfo TypeInfo::GetBaseType() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetBaseType();
#else
			return TypeInfo(nullptr);
#endif
		}
		bool TypeInfo::DerivesFrom(const TypeInfo& Type) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->DerivesFrom(Type.GetTypeInfo());
#else
			return false;
#endif
		}
		size_t TypeInfo::Flags() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetFlags();
#else
			return 0;
#endif
		}
		size_t TypeInfo::Size() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)Info->GetSize();
#else
			return 0;
#endif
		}
		int TypeInfo::GetTypeId() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetTypeId();
#else
			return -1;
#endif
		}
		int TypeInfo::GetSubTypeId(size_t SubTypeIndex) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetSubTypeId((asUINT)SubTypeIndex);
#else
			return -1;
#endif
		}
		TypeInfo TypeInfo::GetSubType(size_t SubTypeIndex) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetSubType((asUINT)SubTypeIndex);
#else
			return TypeInfo(nullptr);
#endif
		}
		size_t TypeInfo::GetSubTypeCount() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)Info->GetSubTypeCount();
#else
			return 0;
#endif
		}
		size_t TypeInfo::GetInterfaceCount() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)Info->GetInterfaceCount();
#else
			return 0;
#endif
		}
		TypeInfo TypeInfo::GetInterface(size_t Index) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetInterface((asUINT)Index);
#else
			return TypeInfo(nullptr);
#endif
		}
		bool TypeInfo::Implements(const TypeInfo& Type) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->Implements(Type.GetTypeInfo());
#else
			return false;
#endif
		}
		size_t TypeInfo::GetFactoriesCount() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)Info->GetFactoryCount();
#else
			return 0;
#endif
		}
		Function TypeInfo::GetFactoryByIndex(size_t Index) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetFactoryByIndex((asUINT)Index);
#else
			return Function(nullptr);
#endif
		}
		Function TypeInfo::GetFactoryByDecl(const std::string_view& Decl) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			return Info->GetFactoryByDecl(Decl.data());
#else
			return Function(nullptr);
#endif
		}
		size_t TypeInfo::GetMethodsCount() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)Info->GetMethodCount();
#else
			return 0;
#endif
		}
		Function TypeInfo::GetMethodByIndex(size_t Index, bool GetVirtual) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetMethodByIndex((asUINT)Index, GetVirtual);
#else
			return Function(nullptr);
#endif
		}
		Function TypeInfo::GetMethodByName(const std::string_view& Name, bool GetVirtual) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
#ifdef VI_ANGELSCRIPT
			return Info->GetMethodByName(Name.data(), GetVirtual);
#else
			return Function(nullptr);
#endif
		}
		Function TypeInfo::GetMethodByDecl(const std::string_view& Decl, bool GetVirtual) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			return Info->GetMethodByDecl(Decl.data(), GetVirtual);
#else
			return Function(nullptr);
#endif
		}
		size_t TypeInfo::GetPropertiesCount() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)Info->GetPropertyCount();
#else
			return 0;
#endif
		}
		ExpectsVM<void> TypeInfo::GetProperty(size_t Index, FunctionInfo* Out) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			const char* Name;
			asDWORD AccessMask;
			int TypeId, Offset;
			bool IsPrivate;
			bool IsProtected;
			bool IsReference;
			int R = Info->GetProperty((asUINT)Index, &Name, &TypeId, &IsPrivate, &IsProtected, &Offset, &IsReference, &AccessMask);
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
			return FunctionFactory::ToReturn(R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		std::string_view TypeInfo::GetPropertyDeclaration(size_t Index, bool IncludeNamespace) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Info->GetPropertyDeclaration((asUINT)Index, IncludeNamespace));
#else
			return "";
#endif
		}
		size_t TypeInfo::GetBehaviourCount() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)Info->GetBehaviourCount();
#else
			return 0;
#endif
		}
		Function TypeInfo::GetBehaviourByIndex(size_t Index, Behaviours* OutBehaviour) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			asEBehaviours Out;
			asIScriptFunction* Result = Info->GetBehaviourByIndex((asUINT)Index, &Out);
			if (OutBehaviour != nullptr)
				*OutBehaviour = (Behaviours)Out;

			return Result;
#else
			return Function(nullptr);
#endif
		}
		size_t TypeInfo::GetChildFunctionDefCount() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)Info->GetChildFuncdefCount();
#else
			return 0;
#endif
		}
		TypeInfo TypeInfo::GetChildFunctionDef(size_t Index) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetChildFuncdef((asUINT)Index);
#else
			return TypeInfo(nullptr);
#endif
		}
		TypeInfo TypeInfo::GetParentType() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetParentType();
#else
			return TypeInfo(nullptr);
#endif
		}
		size_t TypeInfo::GetEnumValueCount() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)Info->GetEnumValueCount();
#else
			return 0;
#endif
		}
		std::string_view TypeInfo::GetEnumValueByIndex(size_t Index, int* OutValue) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Info->GetEnumValueByIndex((asUINT)Index, OutValue));
#else
			return "";
#endif
		}
		Function TypeInfo::GetFunctionDefSignature() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetFuncdefSignature();
#else
			return Function(nullptr);
#endif
		}
		void* TypeInfo::SetUserData(void* Data, size_t Type)
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->SetUserData(Data, Type);
#else
			return nullptr;
#endif
		}
		void* TypeInfo::GetUserData(size_t Type) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetUserData(Type);
#else
			return nullptr;
#endif
		}
		bool TypeInfo::IsHandle() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return IsHandle(Info->GetTypeId());
#else
			return false;
#endif
		}
		bool TypeInfo::IsValid() const
		{
#ifdef VI_ANGELSCRIPT
			return VM != nullptr && Info != nullptr;
#else
			return false;
#endif
		}
		asITypeInfo* TypeInfo::GetTypeInfo() const
		{
#ifdef VI_ANGELSCRIPT
			return Info;
#else
			return nullptr;
#endif
		}
		VirtualMachine* TypeInfo::GetVM() const
		{
#ifdef VI_ANGELSCRIPT
			return VM;
#else
			return nullptr;
#endif
		}
		bool TypeInfo::IsHandle(int TypeId)
		{
#ifdef VI_ANGELSCRIPT
			return (TypeId & asTYPEID_OBJHANDLE || TypeId & asTYPEID_HANDLETOCONST);
#else
			return false;
#endif
		}
		bool TypeInfo::IsScriptObject(int TypeId)
		{
#ifdef VI_ANGELSCRIPT
			return (TypeId & asTYPEID_SCRIPTOBJECT);
#else
			return false;
#endif
		}

		Function::Function(asIScriptFunction* Base) noexcept : Ptr(Base)
		{
#ifdef VI_ANGELSCRIPT
			VM = (Base ? VirtualMachine::Get(Base->GetEngine()) : nullptr);
#endif
		}
		Function::Function(const Function& Base) noexcept : VM(Base.VM), Ptr(Base.Ptr)
		{
		}
		void Function::AddRef() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			Ptr->AddRef();
#endif
		}
		void Function::Release()
		{
			if (!IsValid())
				return;
#ifdef VI_ANGELSCRIPT
			Core::Memory::Release(Ptr);
#endif
		}
		int Function::GetId() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->GetId();
#else
			return -1;
#endif
		}
		FunctionType Function::GetType() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return (FunctionType)Ptr->GetFuncType();
#else
			return FunctionType::DUMMY;
#endif
		}
		uint32_t* Function::GetByteCode(size_t* Size) const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			asUINT DataSize = 0;
			asDWORD* Data = Ptr->GetByteCode(&DataSize);
			if (Size != nullptr)
				*Size = DataSize;
			return (uint32_t*)Data;
#else
			return nullptr;
#endif
		}
		std::string_view Function::GetModuleName() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Ptr->GetModuleName());
#else
			return "";
#endif
		}
		Module Function::GetModule() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->GetModule();
#else
			return Module(nullptr);
#endif
		}
		std::string_view Function::GetSectionName() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Ptr->GetScriptSectionName());
#else
			return "";
#endif
		}
		std::string_view Function::GetGroup() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Ptr->GetConfigGroup());
#else
			return "";
#endif
		}
		size_t Function::GetAccessMask() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->GetAccessMask();
#else
			return 0;
#endif
		}
		TypeInfo Function::GetObjectType() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->GetObjectType();
#else
			return TypeInfo(nullptr);
#endif
		}
		std::string_view Function::GetObjectName() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Ptr->GetObjectName());
#else
			return "";
#endif
		}
		std::string_view Function::GetName() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Ptr->GetName());
#else
			return "";
#endif
		}
		std::string_view Function::GetNamespace() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Ptr->GetNamespace());
#else
			return "";
#endif
		}
		std::string_view Function::GetDecl(bool IncludeObjectName, bool IncludeNamespace, bool IncludeArgNames) const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Ptr->GetDeclaration(IncludeObjectName, IncludeNamespace, IncludeArgNames));
#else
			return "";
#endif
		}
		bool Function::IsReadOnly() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->IsReadOnly();
#else
			return false;
#endif
		}
		bool Function::IsPrivate() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->IsPrivate();
#else
			return false;
#endif
		}
		bool Function::IsProtected() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->IsProtected();
#else
			return false;
#endif
		}
		bool Function::IsFinal() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->IsFinal();
#else
			return false;
#endif
		}
		bool Function::IsOverride() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->IsOverride();
#else
			return false;
#endif
		}
		bool Function::IsShared() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->IsShared();
#else
			return false;
#endif
		}
		bool Function::IsExplicit() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->IsExplicit();
#else
			return false;
#endif
		}
		bool Function::IsProperty() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->IsProperty();
#else
			return false;
#endif
		}
		size_t Function::GetArgsCount() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)Ptr->GetParamCount();
#else
			return 0;
#endif
		}
		ExpectsVM<void> Function::GetArg(size_t Index, int* TypeId, size_t* Flags, std::string_view* Name, std::string_view* DefaultArg) const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			asDWORD asFlags;
			const char* asName = "";
			const char* asDefaultArg = "";
			int R = Ptr->GetParam((asUINT)Index, TypeId, &asFlags, &asName, &asDefaultArg);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;
			if (Name != nullptr)
				*Name = asName;
			if (DefaultArg != nullptr)
				*DefaultArg = asDefaultArg;
			return FunctionFactory::ToReturn(R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		int Function::GetReturnTypeId(size_t* Flags) const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			asDWORD asFlags;
			int R = Ptr->GetReturnTypeId(&asFlags);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;

			return R;
#else
			return -1;
#endif
		}
		int Function::GetTypeId() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->GetTypeId();
#else
			return -1;
#endif
		}
		bool Function::IsCompatibleWithTypeId(int TypeId) const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->IsCompatibleWithTypeId(TypeId);
#else
			return false;
#endif
		}
		void* Function::GetDelegateObject() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->GetDelegateObject();
#else
			return nullptr;
#endif
		}
		TypeInfo Function::GetDelegateObjectType() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->GetDelegateObjectType();
#else
			return TypeInfo(nullptr);
#endif
		}
		Function Function::GetDelegateFunction() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->GetDelegateFunction();
#else
			return Function(nullptr);
#endif
		}
		size_t Function::GetPropertiesCount() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)Ptr->GetVarCount();
#else
			return 0;
#endif
		}
		ExpectsVM<void> Function::GetProperty(size_t Index, std::string_view* Name, int* TypeId) const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			const char* asName = "";
			auto Result = Ptr->GetVar((asUINT)Index, &asName, TypeId);
			if (Name != nullptr)
				*Name = asName;
			return FunctionFactory::ToReturn(Result);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		std::string_view Function::GetPropertyDecl(size_t Index, bool IncludeNamespace) const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Ptr->GetVarDecl((asUINT)Index, IncludeNamespace));
#else
			return "";
#endif
		}
		int Function::FindNextLineWithCode(int Line) const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->FindNextLineWithCode(Line);
#else
			return -1;
#endif
		}
		void* Function::SetUserData(void* UserData, size_t Type)
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->SetUserData(UserData, Type);
#else
			return nullptr;
#endif
		}
		void* Function::GetUserData(size_t Type) const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->GetUserData(Type);
#else
			return nullptr;
#endif
		}
		bool Function::IsValid() const
		{
#ifdef VI_ANGELSCRIPT
			return VM != nullptr && Ptr != nullptr;
#else
			return false;
#endif
		}
		asIScriptFunction* Function::GetFunction() const
		{
#ifdef VI_ANGELSCRIPT
			return Ptr;
#else
			return nullptr;
#endif
		}
		VirtualMachine* Function::GetVM() const
		{
#ifdef VI_ANGELSCRIPT
			return VM;
#else
			return nullptr;
#endif
		}

		ScriptObject::ScriptObject(asIScriptObject* Base) noexcept : Object(Base)
		{
		}
		void ScriptObject::AddRef() const
		{
			VI_ASSERT(IsValid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			Object->AddRef();
#endif
		}
		void ScriptObject::Release()
		{
			if (!IsValid())
				return;
#ifdef VI_ANGELSCRIPT
			Core::Memory::Release(Object);
#endif
		}
		TypeInfo ScriptObject::GetObjectType()
		{
			VI_ASSERT(IsValid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return Object->GetObjectType();
#else
			return TypeInfo(nullptr);
#endif
		}
		int ScriptObject::GetTypeId()
		{
			VI_ASSERT(IsValid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return Object->GetTypeId();
#else
			return -1;
#endif
		}
		int ScriptObject::GetPropertyTypeId(size_t Id) const
		{
			VI_ASSERT(IsValid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return Object->GetPropertyTypeId((asUINT)Id);
#else
			return -1;
#endif
		}
		size_t ScriptObject::GetPropertiesCount() const
		{
			VI_ASSERT(IsValid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)Object->GetPropertyCount();
#else
			return 0;
#endif
		}
		std::string_view ScriptObject::GetPropertyName(size_t Id) const
		{
			VI_ASSERT(IsValid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Object->GetPropertyName((asUINT)Id));
#else
			return "";
#endif
		}
		void* ScriptObject::GetAddressOfProperty(size_t Id)
		{
			VI_ASSERT(IsValid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return Object->GetAddressOfProperty((asUINT)Id);
#else
			return nullptr;
#endif
		}
		VirtualMachine* ScriptObject::GetVM() const
		{
			VI_ASSERT(IsValid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return VirtualMachine::Get(Object->GetEngine());
#else
			return nullptr;
#endif
		}
		int ScriptObject::CopyFrom(const ScriptObject& Other)
		{
			VI_ASSERT(IsValid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return Object->CopyFrom(Other.GetObject());
#else
			return -1;
#endif
		}
		void* ScriptObject::SetUserData(void* Data, size_t Type)
		{
			VI_ASSERT(IsValid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return Object->SetUserData(Data, (asPWORD)Type);
#else
			return nullptr;
#endif
		}
		void* ScriptObject::GetUserData(size_t Type) const
		{
			VI_ASSERT(IsValid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return Object->GetUserData((asPWORD)Type);
#else
			return nullptr;
#endif
		}
		bool ScriptObject::IsValid() const
		{
#ifdef VI_ANGELSCRIPT
			return Object != nullptr;
#else
			return false;
#endif
		}
		asIScriptObject* ScriptObject::GetObject() const
		{
#ifdef VI_ANGELSCRIPT
			return Object;
#else
			return nullptr;
#endif
		}

		GenericContext::GenericContext(asIScriptGeneric* Base) noexcept : Generic(Base)
		{
#ifdef VI_ANGELSCRIPT
			VM = (Generic ? VirtualMachine::Get(Generic->GetEngine()) : nullptr);
#endif
		}
		void* GenericContext::GetObjectAddress()
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return Generic->GetObject();
#else
			return nullptr;
#endif
		}
		int GenericContext::GetObjectTypeId() const
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return Generic->GetObjectTypeId();
#else
			return -1;
#endif
		}
		size_t GenericContext::GetArgsCount() const
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)Generic->GetArgCount();
#else
			return 0;
#endif
		}
		int GenericContext::GetArgTypeId(size_t Argument, size_t* Flags) const
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			asDWORD asFlags;
			int R = Generic->GetArgTypeId((asUINT)Argument, &asFlags);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;
			return R;
#else
			return -1;
#endif
		}
		unsigned char GenericContext::GetArgByte(size_t Argument)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return Generic->GetArgByte((asUINT)Argument);
#else
			return 0;
#endif
		}
		unsigned short GenericContext::GetArgWord(size_t Argument)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return Generic->GetArgWord((asUINT)Argument);
#else
			return 0;
#endif
		}
		size_t GenericContext::GetArgDWord(size_t Argument)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return Generic->GetArgDWord((asUINT)Argument);
#else
			return 0;
#endif
		}
		uint64_t GenericContext::GetArgQWord(size_t Argument)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return Generic->GetArgQWord((asUINT)Argument);
#else
			return 0;
#endif
		}
		float GenericContext::GetArgFloat(size_t Argument)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return Generic->GetArgFloat((asUINT)Argument);
#else
			return 0.0f;
#endif
		}
		double GenericContext::GetArgDouble(size_t Argument)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return Generic->GetArgDouble((asUINT)Argument);
#else
			return 0.0;
#endif
		}
		void* GenericContext::GetArgAddress(size_t Argument)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return Generic->GetArgAddress((asUINT)Argument);
#else
			return nullptr;
#endif
		}
		void* GenericContext::GetArgObjectAddress(size_t Argument)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return Generic->GetArgObject((asUINT)Argument);
#else
			return nullptr;
#endif
		}
		void* GenericContext::GetAddressOfArg(size_t Argument)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return Generic->GetAddressOfArg((asUINT)Argument);
#else
			return nullptr;
#endif
		}
		int GenericContext::GetReturnTypeId(size_t* Flags) const
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			asDWORD asFlags;
			int TypeId = Generic->GetReturnTypeId(&asFlags);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;

			return TypeId;
#else
			return -1;
#endif
		}
		ExpectsVM<void> GenericContext::SetReturnByte(unsigned char Value)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Generic->SetReturnByte(Value));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> GenericContext::SetReturnWord(unsigned short Value)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Generic->SetReturnWord(Value));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> GenericContext::SetReturnDWord(size_t Value)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Generic->SetReturnDWord((asDWORD)Value));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> GenericContext::SetReturnQWord(uint64_t Value)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Generic->SetReturnQWord(Value));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> GenericContext::SetReturnFloat(float Value)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Generic->SetReturnFloat(Value));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> GenericContext::SetReturnDouble(double Value)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Generic->SetReturnDouble(Value));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> GenericContext::SetReturnAddress(void* Address)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Generic->SetReturnAddress(Address));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> GenericContext::SetReturnObjectAddress(void* Object)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Generic->SetReturnObject(Object));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		void* GenericContext::GetAddressOfReturnLocation()
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return Generic->GetAddressOfReturnLocation();
#else
			return nullptr;
#endif
		}
		bool GenericContext::IsValid() const
		{
#ifdef VI_ANGELSCRIPT
			return VM != nullptr && Generic != nullptr;
#else
			return false;
#endif
		}
		asIScriptGeneric* GenericContext::GetGeneric() const
		{
#ifdef VI_ANGELSCRIPT
			return Generic;
#else
			return nullptr;
#endif
		}
		VirtualMachine* GenericContext::GetVM() const
		{
#ifdef VI_ANGELSCRIPT
			return VM;
#else
			return nullptr;
#endif
		}

		BaseClass::BaseClass(VirtualMachine* Engine, asITypeInfo* Source, int Type) noexcept : VM(Engine), Type(Source), TypeId(Type)
		{
		}
		ExpectsVM<void> BaseClass::SetFunctionDef(const std::string_view& Decl)
		{
			VI_ASSERT(IsValid(), "class should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " funcdef %i bytes", (void*)this, (int)Decl.size());
			return FunctionFactory::ToReturn(Engine->RegisterFuncdef(Decl.data()));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> BaseClass::SetOperatorCopyAddress(asSFuncPtr* Value, FunctionCall Type)
		{
			VI_ASSERT(IsValid(), "class should be valid");
			VI_ASSERT(Core::Stringify::IsCString(GetTypeName()), "typename should be set");
			VI_ASSERT(Value != nullptr, "value should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");

			Core::String Decl = Core::Stringify::Text("%s& opAssign(const %s &in)", GetTypeName().data(), GetTypeName().data());
			VI_TRACE("[vm] register class 0x%" PRIXPTR " op-copy funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)Type, (int)Decl.size(), (void*)Value);
			return FunctionFactory::ToReturn(Engine->RegisterObjectMethod(GetTypeName().data(), Decl.c_str(), *Value, (asECallConvTypes)Type));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> BaseClass::SetBehaviourAddress(const std::string_view& Decl, Behaviours Behave, asSFuncPtr* Value, FunctionCall Type)
		{
			VI_ASSERT(IsValid(), "class should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
			VI_ASSERT(Core::Stringify::IsCString(GetTypeName()), "typename should be set");
			VI_ASSERT(Value != nullptr, "value should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " behaviour funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)Type, (int)Decl.size(), (void*)Value);
			return FunctionFactory::ToReturn(Engine->RegisterObjectBehaviour(GetTypeName().data(), (asEBehaviours)Behave, Decl.data(), *Value, (asECallConvTypes)Type));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> BaseClass::SetPropertyAddress(const std::string_view& Decl, int Offset)
		{
			VI_ASSERT(IsValid(), "class should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
			VI_ASSERT(Core::Stringify::IsCString(GetTypeName()), "typename should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " property %i bytes at 0x0+%i", (void*)this, (int)Decl.size(), Offset);
			return FunctionFactory::ToReturn(Engine->RegisterObjectProperty(GetTypeName().data(), Decl.data(), Offset));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> BaseClass::SetPropertyStaticAddress(const std::string_view& Decl, void* Value)
		{
			VI_ASSERT(IsValid(), "class should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
			VI_ASSERT(Core::Stringify::IsCString(GetTypeName()), "typename should be set");
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " static property %i bytes at 0x%" PRIXPTR, (void*)this, (int)Decl.size(), (void*)Value);
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
			const char* Namespace = Engine->GetDefaultNamespace();
			const char* Scope = Type->GetNamespace();

			Engine->SetDefaultNamespace(Scope[0] == '\0' ? GetTypeName().data() : Core::String(Scope).append("::").append(GetName()).c_str());
			int R = Engine->RegisterGlobalProperty(Decl.data(), Value);
			Engine->SetDefaultNamespace(Namespace);

			return FunctionFactory::ToReturn(R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> BaseClass::SetOperatorAddress(const std::string_view& Decl, asSFuncPtr* Value, FunctionCall Type)
		{
			return SetMethodAddress(Decl, Value, Type);
		}
		ExpectsVM<void> BaseClass::SetMethodAddress(const std::string_view& Decl, asSFuncPtr* Value, FunctionCall Type)
		{
			VI_ASSERT(IsValid(), "class should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
			VI_ASSERT(Core::Stringify::IsCString(GetTypeName()), "typename should be set");
			VI_ASSERT(Value != nullptr, "value should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)Type, (int)Decl.size(), (void*)Value);
			return FunctionFactory::ToReturn(Engine->RegisterObjectMethod(GetTypeName().data(), Decl.data(), *Value, (asECallConvTypes)Type));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> BaseClass::SetMethodStaticAddress(const std::string_view& Decl, asSFuncPtr* Value, FunctionCall Type)
		{
			VI_ASSERT(IsValid(), "class should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
			VI_ASSERT(Core::Stringify::IsCString(GetTypeName()), "typename should be set");
			VI_ASSERT(Value != nullptr, "value should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " static funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)Type, (int)Decl.size(), (void*)Value);

			const char* Namespace = Engine->GetDefaultNamespace();
			const char* Scope = this->Type->GetNamespace();
			Engine->SetDefaultNamespace(Scope[0] == '\0' ? GetTypeName().data() : Core::String(Scope).append("::").append(GetName()).c_str());
			int R = Engine->RegisterGlobalFunction(Decl.data(), *Value, (asECallConvTypes)Type);
			Engine->SetDefaultNamespace(Namespace);

			return FunctionFactory::ToReturn(R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> BaseClass::SetConstructorAddress(const std::string_view& Decl, asSFuncPtr* Value, FunctionCall Type)
		{
			VI_ASSERT(IsValid(), "class should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
			VI_ASSERT(Core::Stringify::IsCString(GetTypeName()), "typename should be set");
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " constructor funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)Type, (int)Decl.size(), (void*)Value);
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
			return FunctionFactory::ToReturn(Engine->RegisterObjectBehaviour(GetTypeName().data(), asBEHAVE_CONSTRUCT, Decl.data(), *Value, (asECallConvTypes)Type));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> BaseClass::SetConstructorListAddress(const std::string_view& Decl, asSFuncPtr* Value, FunctionCall Type)
		{
			VI_ASSERT(IsValid(), "class should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
			VI_ASSERT(Core::Stringify::IsCString(GetTypeName()), "typename should be set");
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " list-constructor funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)Type, (int)Decl.size(), (void*)Value);
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
			return FunctionFactory::ToReturn(Engine->RegisterObjectBehaviour(GetTypeName().data(), asBEHAVE_LIST_CONSTRUCT, Decl.data(), *Value, (asECallConvTypes)Type));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> BaseClass::SetDestructorAddress(const std::string_view& Decl, asSFuncPtr* Value)
		{
			VI_ASSERT(IsValid(), "class should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
			VI_ASSERT(Core::Stringify::IsCString(GetTypeName()), "typename should be set");
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " destructor funcaddr %i bytes at 0x%" PRIXPTR, (void*)this, (int)Decl.size(), (void*)Value);
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
			return FunctionFactory::ToReturn(Engine->RegisterObjectBehaviour(GetTypeName().data(), asBEHAVE_DESTRUCT, Decl.data(), *Value, asCALL_CDECL_OBJFIRST));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		asITypeInfo* BaseClass::GetTypeInfo() const
		{
			return Type;
		}
		int BaseClass::GetTypeId() const
		{
			return TypeId;
		}
		bool BaseClass::IsValid() const
		{
			return VM != nullptr && TypeId >= 0 && Type != nullptr;
		}
		std::string_view BaseClass::GetTypeName() const
		{
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Type->GetName());
#else
			return "";
#endif
		}
		Core::String BaseClass::GetName() const
		{
			return Core::String(GetTypeName());
		}
		VirtualMachine* BaseClass::GetVM() const
		{
			return VM;
		}
		Core::String BaseClass::GetOperator(Operators Op, const std::string_view& Out, const std::string_view& Args, bool Const, bool Right)
		{
			VI_ASSERT(Core::Stringify::IsCString(Out), "out should be set");
			VI_ASSERT(Core::Stringify::IsCString(Args), "args should be set");
			switch (Op)
			{
				case Operators::Neg:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opNeg()%s", Out.data(), Const ? " const" : "");
				case Operators::Com:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opCom()%s", Out.data(), Const ? " const" : "");
				case Operators::PreInc:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opPreInc()%s", Out.data(), Const ? " const" : "");
				case Operators::PreDec:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opPreDec()%s", Out.data(), Const ? " const" : "");
				case Operators::PostInc:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opPostInc()%s", Out.data(), Const ? " const" : "");
				case Operators::PostDec:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opPostDec()%s", Out.data(), Const ? " const" : "");
				case Operators::Equals:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opEquals(%s)%s", Out.data(), Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::Cmp:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opCmp(%s)%s", Out.data(), Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::Assign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opAssign(%s)%s", Out.data(), Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::AddAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opAddAssign(%s)%s", Out.data(), Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::SubAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opSubAssign(%s)%s", Out.data(), Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::MulAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opMulAssign(%s)%s", Out.data(), Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::DivAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opDivAssign(%s)%s", Out.data(), Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::ModAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opModAssign(%s)%s", Out.data(), Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::PowAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opPowAssign(%s)%s", Out.data(), Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::AndAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opAndAssign(%s)%s", Out.data(), Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::OrAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opOrAssign(%s)%s", Out.data(), Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::XOrAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opXorAssign(%s)%s", Out.data(), Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::ShlAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opShlAssign(%s)%s", Out.data(), Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::ShrAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opShrAssign(%s)%s", Out.data(), Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::UshrAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opUshrAssign(%s)%s", Out.data(), Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::Add:
					return Core::Stringify::Text("%s opAdd%s(%s)%s", Out.data(), Right ? "_r" : "", Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::Sub:
					return Core::Stringify::Text("%s opSub%s(%s)%s", Out.data(), Right ? "_r" : "", Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::Mul:
					return Core::Stringify::Text("%s opMul%s(%s)%s", Out.data(), Right ? "_r" : "", Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::Div:
					return Core::Stringify::Text("%s opDiv%s(%s)%s", Out.data(), Right ? "_r" : "", Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::Mod:
					return Core::Stringify::Text("%s opMod%s(%s)%s", Out.data(), Right ? "_r" : "", Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::Pow:
					return Core::Stringify::Text("%s opPow%s(%s)%s", Out.data(), Right ? "_r" : "", Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::And:
					return Core::Stringify::Text("%s opAnd%s(%s)%s", Out.data(), Right ? "_r" : "", Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::Or:
					return Core::Stringify::Text("%s opOr%s(%s)%s", Out.data(), Right ? "_r" : "", Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::XOr:
					return Core::Stringify::Text("%s opXor%s(%s)%s", Out.data(), Right ? "_r" : "", Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::Shl:
					return Core::Stringify::Text("%s opShl%s(%s)%s", Out.data(), Right ? "_r" : "", Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::Shr:
					return Core::Stringify::Text("%s opShr%s(%s)%s", Out.data(), Right ? "_r" : "", Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::Ushr:
					return Core::Stringify::Text("%s opUshr%s(%s)%s", Out.data(), Right ? "_r" : "", Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::Index:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opIndex(%s)%s", Out.data(), Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::Call:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opCall(%s)%s", Out.data(), Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::Cast:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opCast(%s)%s", Out.data(), Args.empty() ? "" : Args.data(), Const ? " const" : "");
				case Operators::ImplCast:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opImplCast(%s)%s", Out.data(), Args.empty() ? "" : Args.data(), Const ? " const" : "");
				default:
					return "";
			}
		}

		TypeInterface::TypeInterface(VirtualMachine* Engine, asITypeInfo* Source, int Type) noexcept : VM(Engine), Type(Source), TypeId(Type)
		{
		}
		ExpectsVM<void> TypeInterface::SetMethod(const std::string_view& Decl)
		{
			VI_ASSERT(IsValid(), "interface should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
			VI_ASSERT(Core::Stringify::IsCString(GetTypeName()), "typename should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register interface 0x%" PRIXPTR " method %i bytes", (void*)this, (int)Decl.size());
			return FunctionFactory::ToReturn(Engine->RegisterInterfaceMethod(GetTypeName().data(), Decl.data()));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		asITypeInfo* TypeInterface::GetTypeInfo() const
		{
			return Type;
		}
		int TypeInterface::GetTypeId() const
		{
			return TypeId;
		}
		bool TypeInterface::IsValid() const
		{
			return VM != nullptr && TypeId >= 0 && Type != nullptr;
		}
		std::string_view TypeInterface::GetTypeName() const
		{
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Type->GetName());
#else
			return "";
#endif
		}
		Core::String TypeInterface::GetName() const
		{
			return Core::String(GetTypeName());
		}
		VirtualMachine* TypeInterface::GetVM() const
		{
			return VM;
		}

		Enumeration::Enumeration(VirtualMachine* Engine, asITypeInfo* Source, int Type) noexcept : VM(Engine), Type(Source), TypeId(Type)
		{
		}
		ExpectsVM<void> Enumeration::SetValue(const std::string_view& Name, int Value)
		{
			VI_ASSERT(IsValid(), "enum should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
			VI_ASSERT(Core::Stringify::IsCString(GetTypeName()), "typename should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register enum 0x%" PRIXPTR " value %i bytes = %i", (void*)this, (int)Name.size(), Value);
			return FunctionFactory::ToReturn(Engine->RegisterEnumValue(GetTypeName().data(), Name.data(), Value));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		asITypeInfo* Enumeration::GetTypeInfo() const
		{
			return Type;
		}
		int Enumeration::GetTypeId() const
		{
			return TypeId;
		}
		bool Enumeration::IsValid() const
		{
			return VM != nullptr && TypeId >= 0 && Type != nullptr;
		}
		std::string_view Enumeration::GetTypeName() const
		{
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Type->GetName());
#else
			return "";
#endif
		}
		Core::String Enumeration::GetName() const
		{
			return Core::String(GetTypeName());
		}
		VirtualMachine* Enumeration::GetVM() const
		{
			return VM;
		}

		Module::Module(asIScriptModule* Type) noexcept : Mod(Type)
		{
#ifdef VI_ANGELSCRIPT
			VM = (Mod ? VirtualMachine::Get(Mod->GetEngine()) : nullptr);
#endif
		}
		void Module::SetName(const std::string_view& Name)
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
#ifdef VI_ANGELSCRIPT
			Mod->SetName(Name.data());
#endif
		}
		ExpectsVM<void> Module::AddSection(const std::string_view& Name, const std::string_view& Code, int LineOffset)
		{
			VI_ASSERT(IsValid(), "module should be valid");
			return VM->AddScriptSection(Mod, Name, Code, LineOffset);
		}
		ExpectsVM<void> Module::RemoveFunction(const Function& Function)
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Mod->RemoveFunction(Function.GetFunction()));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Module::ResetProperties(asIScriptContext* Context)
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Mod->ResetGlobalVars(Context));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Module::Build()
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			int R = Mod->Build();
			if (R != asBUILD_IN_PROGRESS)
				VM->ClearSections();
			return FunctionFactory::ToReturn(R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Module::LoadByteCode(ByteCodeInfo* Info)
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Info != nullptr, "bytecode should be set");
#ifdef VI_ANGELSCRIPT
			CByteCodeStream* Stream = Core::Memory::New<CByteCodeStream>(Info->Data);
			int R = Mod->LoadByteCode(Stream, &Info->Debug);
			Core::Memory::Delete(Stream);
			return FunctionFactory::ToReturn(R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Module::BindImportedFunction(size_t ImportIndex, const Function& Function)
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Mod->BindImportedFunction((asUINT)ImportIndex, Function.GetFunction()));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Module::UnbindImportedFunction(size_t ImportIndex)
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Mod->UnbindImportedFunction((asUINT)ImportIndex));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Module::BindAllImportedFunctions()
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Mod->BindAllImportedFunctions());
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Module::UnbindAllImportedFunctions()
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Mod->UnbindAllImportedFunctions());
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Module::CompileFunction(const std::string_view& SectionName, const std::string_view& Code, int LineOffset, size_t CompileFlags, Function* OutFunction)
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Code), "code should be set");
			VI_ASSERT(Core::Stringify::IsCString(SectionName), "section name should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptFunction* OutFunc = nullptr;
			int R = Mod->CompileFunction(SectionName.data(), Code.data(), LineOffset, (asDWORD)CompileFlags, &OutFunc);
			if (OutFunction != nullptr)
				*OutFunction = Function(OutFunc);
			return FunctionFactory::ToReturn(R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Module::CompileProperty(const std::string_view& SectionName, const std::string_view& Code, int LineOffset)
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Code), "code should be set");
			VI_ASSERT(Core::Stringify::IsCString(SectionName), "section name should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Mod->CompileGlobalVar(SectionName.data(), Code.data(), LineOffset));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Module::SetDefaultNamespace(const std::string_view& Namespace)
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Namespace), "namespace should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Mod->SetDefaultNamespace(Namespace.data()));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Module::RemoveProperty(size_t Index)
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Mod->RemoveGlobalVar((asUINT)Index));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		void Module::Discard()
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			Mod->Discard();
			Mod = nullptr;
#endif
		}
		void* Module::GetAddressOfProperty(size_t Index)
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return Mod->GetAddressOfGlobalVar((asUINT)Index);
#else
			return nullptr;
#endif
		}
		size_t Module::SetAccessMask(size_t AccessMask)
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return Mod->SetAccessMask((asDWORD)AccessMask);
#else
			return 0;
#endif
		}
		size_t Module::GetFunctionCount() const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return Mod->GetFunctionCount();
#else
			return 0;
#endif
		}
		Function Module::GetFunctionByIndex(size_t Index) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return Mod->GetFunctionByIndex((asUINT)Index);
#else
			return Function(nullptr);
#endif
		}
		Function Module::GetFunctionByDecl(const std::string_view& Decl) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			return Mod->GetFunctionByDecl(Decl.data());
#else
			return Function(nullptr);
#endif
		}
		Function Module::GetFunctionByName(const std::string_view& Name) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
#ifdef VI_ANGELSCRIPT
			return Mod->GetFunctionByName(Name.data());
#else
			return Function(nullptr);
#endif
		}
		int Module::GetTypeIdByDecl(const std::string_view& Decl) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			return Mod->GetTypeIdByDecl(Decl.data());
#else
			return -1;
#endif
		}
		ExpectsVM<size_t> Module::GetImportedFunctionIndexByDecl(const std::string_view& Decl) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			int R = Mod->GetImportedFunctionIndexByDecl(Decl.data());
			return FunctionFactory::ToReturn<size_t>(R, (size_t)R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Module::SaveByteCode(ByteCodeInfo* Info) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Info != nullptr, "bytecode should be set");
#ifdef VI_ANGELSCRIPT
			CByteCodeStream* Stream = Core::Memory::New<CByteCodeStream>();
			int R = Mod->SaveByteCode(Stream, Info->Debug);
			Info->Data = Stream->GetCode();
			Core::Memory::Delete(Stream);
			return FunctionFactory::ToReturn(R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<size_t> Module::GetPropertyIndexByName(const std::string_view& Name) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
#ifdef VI_ANGELSCRIPT
			int R = Mod->GetGlobalVarIndexByName(Name.data());
			return FunctionFactory::ToReturn<size_t>(R, (size_t)R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<size_t> Module::GetPropertyIndexByDecl(const std::string_view& Decl) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			int R = Mod->GetGlobalVarIndexByDecl(Decl.data());
			return FunctionFactory::ToReturn<size_t>(R, (size_t)R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Module::GetProperty(size_t Index, PropertyInfo* Info) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			const char* Name = "";
			const char* Namespace = "";
			bool IsConst = false;
			int TypeId = 0;
			int Result = Mod->GetGlobalVar((asUINT)Index, &Name, &Namespace, &TypeId, &IsConst);

			if (Info != nullptr)
			{
				Info->Name = OrEmpty(Name);
				Info->Namespace = OrEmpty(Namespace);
				Info->TypeId = TypeId;
				Info->IsConst = IsConst;
				Info->ConfigGroup = "";
				Info->Pointer = Mod->GetAddressOfGlobalVar((asUINT)Index);
				Info->AccessMask = GetAccessMask();
			}

			return FunctionFactory::ToReturn(Result);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		size_t Module::GetAccessMask() const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			asDWORD AccessMask = Mod->SetAccessMask(0);
			Mod->SetAccessMask(AccessMask);
			return (size_t)AccessMask;
#else
			return 0;
#endif
		}
		size_t Module::GetObjectsCount() const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return Mod->GetObjectTypeCount();
#else
			return 0;
#endif
		}
		size_t Module::GetPropertiesCount() const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return Mod->GetGlobalVarCount();
#else
			return 0;
#endif
		}
		size_t Module::GetEnumsCount() const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return Mod->GetEnumCount();
#else
			return 0;
#endif
		}
		size_t Module::GetImportedFunctionCount() const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return Mod->GetImportedFunctionCount();
#else
			return 0;
#endif
		}
		TypeInfo Module::GetObjectByIndex(size_t Index) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return Mod->GetObjectTypeByIndex((asUINT)Index);
#else
			return TypeInfo(nullptr);
#endif
		}
		TypeInfo Module::GetTypeInfoByName(const std::string_view& Name) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
#ifdef VI_ANGELSCRIPT
			std::string_view TypeName = Name;
			std::string_view Namespace = "";
			if (!VM->GetTypeNameScope(&TypeName, &Namespace))
				return Mod->GetTypeInfoByName(Name.data());

			VI_ASSERT(Core::Stringify::IsCString(TypeName), "typename should be set");
			VM->BeginNamespace(Core::String(Namespace));
			asITypeInfo* Info = Mod->GetTypeInfoByName(TypeName.data());
			VM->EndNamespace();

			return Info;
#else
			return TypeInfo(nullptr);
#endif
		}
		TypeInfo Module::GetTypeInfoByDecl(const std::string_view& Decl) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			return Mod->GetTypeInfoByDecl(Decl.data());
#else
			return TypeInfo(nullptr);
#endif
		}
		TypeInfo Module::GetEnumByIndex(size_t Index) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return Mod->GetEnumByIndex((asUINT)Index);
#else
			return TypeInfo(nullptr);
#endif
		}
		std::string_view Module::GetPropertyDecl(size_t Index, bool IncludeNamespace) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Mod->GetGlobalVarDeclaration((asUINT)Index, IncludeNamespace));
#else
			return "";
#endif
		}
		std::string_view Module::GetDefaultNamespace() const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Mod->GetDefaultNamespace());
#else
			return "";
#endif
		}
		std::string_view Module::GetImportedFunctionDecl(size_t ImportIndex) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Mod->GetImportedFunctionDeclaration((asUINT)ImportIndex));
#else
			return "";
#endif
		}
		std::string_view Module::GetImportedFunctionModule(size_t ImportIndex) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Mod->GetImportedFunctionSourceModule((asUINT)ImportIndex));
#else
			return "";
#endif
		}
		std::string_view Module::GetName() const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Mod->GetName());
#else
			return "";
#endif
		}
		bool Module::IsValid() const
		{
#ifdef VI_ANGELSCRIPT
			return VM != nullptr && Mod != nullptr;
#else
			return false;
#endif
		}
		asIScriptModule* Module::GetModule() const
		{
#ifdef VI_ANGELSCRIPT
			return Mod;
#else
			return nullptr;
#endif
		}
		VirtualMachine* Module::GetVM() const
		{
#ifdef VI_ANGELSCRIPT
			return VM;
#else
			return nullptr;
#endif
		}

		FunctionDelegate::FunctionDelegate() : Context(nullptr), Callback(nullptr), DelegateType(nullptr), DelegateObject(nullptr)
		{
		}
		FunctionDelegate::FunctionDelegate(const Function& Function) : Context(nullptr), Callback(nullptr), DelegateType(nullptr), DelegateObject(nullptr)
		{
			if (!Function.IsValid())
				return;
#ifdef VI_ANGELSCRIPT
			Context = ImmediateContext::Get();
			Callback = Function.GetFunction();
			DelegateType = Callback->GetDelegateObjectType();
			DelegateObject = Callback->GetDelegateObject();
			AddRefAndInitialize(true);
#endif
		}
		FunctionDelegate::FunctionDelegate(const Function& Function, ImmediateContext* WantedContext) : Context(WantedContext), Callback(nullptr), DelegateType(nullptr), DelegateObject(nullptr)
		{
			if (!Function.IsValid())
				return;
#ifdef VI_ANGELSCRIPT
			if (!Context)
				Context = ImmediateContext::Get();

			Callback = Function.GetFunction();
			DelegateType = Callback->GetDelegateObjectType();
			DelegateObject = Callback->GetDelegateObject();
			AddRefAndInitialize(true);
#endif
		}
		FunctionDelegate::FunctionDelegate(const FunctionDelegate& Other) : Context(Other.Context), Callback(Other.Callback), DelegateType(Other.DelegateType), DelegateObject(Other.DelegateObject)
		{
			AddRef();
		}
		FunctionDelegate::FunctionDelegate(FunctionDelegate&& Other) : Context(Other.Context), Callback(Other.Callback), DelegateType(Other.DelegateType), DelegateObject(Other.DelegateObject)
		{
			Other.Context = nullptr;
			Other.Callback = nullptr;
			Other.DelegateType = nullptr;
			Other.DelegateObject = nullptr;
		}
		FunctionDelegate::~FunctionDelegate()
		{
			Release();
		}
		FunctionDelegate& FunctionDelegate::operator= (const FunctionDelegate& Other)
		{
			if (this == &Other)
				return *this;

			Release();
			Context = Other.Context;
			Callback = Other.Callback;
			DelegateType = Other.DelegateType;
			DelegateObject = Other.DelegateObject;
			AddRef();

			return *this;
		}
		FunctionDelegate& FunctionDelegate::operator= (FunctionDelegate&& Other)
		{
			if (this == &Other)
				return *this;

			Context = Other.Context;
			Callback = Other.Callback;
			DelegateType = Other.DelegateType;
			DelegateObject = Other.DelegateObject;
			Other.Context = nullptr;
			Other.Callback = nullptr;
			Other.DelegateType = nullptr;
			Other.DelegateObject = nullptr;
			return *this;
		}
		ExpectsPromiseVM<Execution> FunctionDelegate::operator()(ArgsCallback&& OnArgs, ArgsCallback&& OnReturn)
		{
			if (!IsValid())
				return ExpectsVM<Execution>(VirtualError::INVALID_CONFIGURATION);

			FunctionDelegate Copy = *this;
			return Context->ResolveCallback(std::move(Copy), std::move(OnArgs), std::move(OnReturn));
		}
		void FunctionDelegate::AddRefAndInitialize(bool IsFirstTime)
		{
			if (!IsValid())
				return;
#ifdef VI_ANGELSCRIPT
			if (DelegateObject != nullptr)
				Context->GetVM()->AddRefObject(DelegateObject, DelegateType);

			if (!IsFirstTime)
				Callback->AddRef();
			Context->AddRef();
#endif
		}
		void FunctionDelegate::AddRef()
		{
			AddRefAndInitialize(false);
		}
		void FunctionDelegate::Release()
		{
			if (!IsValid())
				return;
#ifdef VI_ANGELSCRIPT
			if (DelegateObject != nullptr)
				Context->GetVM()->ReleaseObject(DelegateObject, DelegateType);

			DelegateObject = nullptr;
			DelegateType = nullptr;
			Core::Memory::Release(Callback);
			Core::Memory::Release(Context);
#endif
		}
		bool FunctionDelegate::IsValid() const
		{
			return Context && Callback;
		}
		Function FunctionDelegate::Callable() const
		{
			return Callback;
		}

		Compiler::Compiler(VirtualMachine* Engine) noexcept : Scope(nullptr), VM(Engine), Built(false)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");

			Processor = new Compute::Preprocessor();
			Processor->SetIncludeCallback([this](Compute::Preprocessor* Processor, const Compute::IncludeResult& File, Core::String& Output) -> Compute::ExpectsPreprocessor<Compute::IncludeType>
			{
				VI_ASSERT(VM != nullptr, "engine should be set");
				if (Include)
				{
					auto Status = Include(Processor, File, Output);
					switch (Status.Or(Compute::IncludeType::Unchanged))
					{
						case Compute::IncludeType::Preprocess:
							goto Preprocess;
						case Compute::IncludeType::Virtual:
							return Compute::IncludeType::Virtual;
						case Compute::IncludeType::Error:
							return Compute::IncludeType::Error;
						case Compute::IncludeType::Unchanged:
						default:
							break;
					}
				}

				if (File.Module.empty() || !Scope)
					return Compute::IncludeType::Error;

				if (!File.IsFile && File.IsAbstract)
					return VM->ImportSystemAddon(File.Module) ? Compute::IncludeType::Virtual : Compute::IncludeType::Error;

				if (!VM->ImportFile(File.Module, File.IsRemote, Output))
					return Compute::IncludeType::Error;

			Preprocess:
				if (Output.empty())
					return Compute::IncludeType::Virtual;

				auto Status = VM->GenerateCode(Processor, File.Module, Output);
				if (!Status)
					return Status.Error();
				else if (Output.empty())
					return Compute::IncludeType::Virtual;

				return VM->AddScriptSection(Scope, File.Module, Output) ? Compute::IncludeType::Virtual : Compute::IncludeType::Error;
			});
			Processor->SetPragmaCallback([this](Compute::Preprocessor* Processor, const std::string_view& Name, const Core::Vector<Core::String>& Args) -> Compute::ExpectsPreprocessor<void>
			{
				VI_ASSERT(VM != nullptr, "engine should be set");
				if (Pragma)
				{
					auto Status = Pragma(Processor, Name, Args);
					if (Status)
						return Status;
				}

				if (Name == "compile" && Args.size() == 2)
				{
					const Core::String& Key = Args[0];
					const Core::String& Value = Args[1];
					auto Numeric = Core::FromString<uint64_t>(Value);
					size_t Result = Numeric ? (size_t)*Numeric : 0;
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
					else if (Key == "IGNORE_DUPLICATE_SHARED_INTF")
						VM->SetProperty(Features::IGNORE_DUPLICATE_SHARED_INTF, Result);
					else if (Key == "NO_DEBUG_OUTPUT")
						VM->SetProperty(Features::NO_DEBUG_OUTPUT, Result);
				}
				else if (Name == "comment" && Args.size() == 2)
				{
					const Core::String& Key = Args[0];
					if (Key == "INFO")
						VI_INFO("[asc] %s", Args[1].c_str());
					else if (Key == "TRACE")
						VI_DEBUG("[asc] %s", Args[1].c_str());
					else if (Key == "WARN")
						VI_WARN("[asc] %s", Args[1].c_str());
					else if (Key == "ERR")
						VI_ERR("[asc] %s", Args[1].c_str());
				}
				else if (Name == "modify" && Args.size() == 2)
				{
#ifdef VI_ANGELSCRIPT
					const Core::String& Key = Args[0];
					const Core::String& Value = Args[1];
					auto Numeric = Core::FromString<uint64_t>(Value);
					size_t Result = Numeric ? (size_t)*Numeric : 0;
					if (Key == "NAME")
						Scope->SetName(Value.c_str());
					else if (Key == "NAMESPACE")
						Scope->SetDefaultNamespace(Value.c_str());
					else if (Key == "ACCESS_MASK")
						Scope->SetAccessMask((asDWORD)Result);
#endif
				}
				else if (Name == "cimport" && Args.size() >= 2)
				{
					if (Args.size() == 3)
					{
						auto Status = VM->ImportCLibrary(Args[0]);
						if (Status)
						{
							Status = VM->ImportCFunction({ Args[0] }, Args[1], Args[2]);
							if (Status)
								Define("SOF_" + Args[1]);
							else
								VI_ERR("[asc] %s", Status.Error().what());
						}
						else
							VI_ERR("[asc] %s", Status.Error().what());
					}
					else
					{
						auto Status = VM->ImportCFunction({ }, Args[0], Args[1]);
						if (Status)
							Define("SOF_" + Args[1]);
						else
							VI_ERR("[asc] %s", Status.Error().what());
					}
				}
				else if (Name == "clibrary" && Args.size() >= 1)
				{
					Core::String Directory = Core::OS::Path::GetDirectory(Processor->GetCurrentFilePath().c_str());
					if (Directory.empty())
					{
						auto Working = Core::OS::Directory::GetWorking();
						if (Working)
							Directory = *Working;
					}

					if (!VM->ImportCLibrary(Args[0]))
					{
						auto Path = Core::OS::Path::Resolve(Args[0], Directory, false);
						if (Path && VM->ImportCLibrary(*Path))
						{
							if (Args.size() == 2 && !Args[1].empty())
								Define("SOL_" + Args[1]);
						}
					}
					else if (Args.size() == 2 && !Args[1].empty())
						Define("SOL_" + Args[1]);

				}
				else if (Name == "define" && Args.size() == 1)
					Define(Args[0]);

				return Core::Expectation::Met;
			});
			Processor->Define("VI_VERSION " + Core::ToString((size_t)VERSION));
#ifdef VI_MICROSOFT
			Processor->Define("OS_MICROSOFT");
#elif defined(VI_ANDROID)
			Processor->Define("OS_ANDROID");
			Processor->Define("OS_LINUX");
#elif defined(VI_APPLE)
			Processor->Define("OS_APPLE");
			Processor->Define("OS_LINUX");
#ifdef VI_IOS
			Processor->Define("OS_IOS");
#elif defined(VI_OSX)
			Processor->Define("OS_OSX");
#endif
#elif defined(VI_LINUX)
			Processor->Define("OS_LINUX");
#endif
			VM->SetProcessorOptions(Processor);
		}
		Compiler::~Compiler() noexcept
		{
#ifdef VI_ANGELSCRIPT
			if (Scope != nullptr)
				Scope->Discard();
#endif
			Core::Memory::Release(Processor);
		}
		void Compiler::SetIncludeCallback(Compute::ProcIncludeCallback&& Callback)
		{
			Include = std::move(Callback);
		}
		void Compiler::SetPragmaCallback(Compute::ProcPragmaCallback&& Callback)
		{
			Pragma = std::move(Callback);
		}
		void Compiler::Define(const std::string_view& Word)
		{
			Processor->Define(Word);
		}
		void Compiler::Undefine(const std::string_view& Word)
		{
			Processor->Undefine(Word);
		}
		Module Compiler::UnlinkModule()
		{
			Module Result(Scope);
			Scope = nullptr;
			return Result;
		}
		bool Compiler::Clear()
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			if (Scope != nullptr)
				Scope->Discard();
#endif
			if (Processor != nullptr)
				Processor->Clear();

			Scope = nullptr;
			Built = false;
			return true;
		}
		bool Compiler::IsDefined(const std::string_view& Word) const
		{
			return Processor->IsDefined(Word);
		}
		bool Compiler::IsBuilt() const
		{
			return Built;
		}
		bool Compiler::IsCached() const
		{
			return VCache.Valid;
		}
		ExpectsVM<void> Compiler::Prepare(ByteCodeInfo* Info)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(Info != nullptr, "bytecode should be set");
#ifdef VI_ANGELSCRIPT
			if (!Info->Valid || Info->Data.empty())
				return VirtualException(VirtualError::INVALID_ARG);

			auto Result = Prepare(Info->Name, true);
			if (Result)
				VCache = *Info;
			return Result;
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Compiler::Prepare(const Module& Info)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(Info.IsValid(), "module should be set");
#ifdef VI_ANGELSCRIPT
			if (!Info.IsValid())
				return VirtualException(VirtualError::INVALID_ARG);

			Built = true;
			VCache.Valid = false;
			VCache.Name.clear();
			if (Scope != nullptr)
				Scope->Discard();

			Scope = Info.GetModule();
			VM->SetProcessorOptions(Processor);
			return Core::Expectation::Met;
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Compiler::Prepare(const std::string_view& ModuleName, bool Scoped)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(!ModuleName.empty(), "module name should not be empty");
			VI_DEBUG("[vm] prepare %.*s on 0x%" PRIXPTR, (int)ModuleName.size(), ModuleName.data(), (uintptr_t)this);
#ifdef VI_ANGELSCRIPT
			Built = false;
			VCache.Valid = false;
			VCache.Name.clear();
			if (Scope != nullptr)
				Scope->Discard();

			Scope = Scoped ? VM->CreateScopedModule(ModuleName) : VM->CreateModule(ModuleName);
			if (!Scope)
				return VirtualException(VirtualError::INVALID_NAME);

			VM->SetProcessorOptions(Processor);
			return Core::Expectation::Met;
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Compiler::Prepare(const std::string_view& ModuleName, const std::string_view& Name, bool Debug, bool Scoped)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			auto Result = Prepare(ModuleName, Scoped);
			if (!Result)
				return Result;

			VCache.Name = Name;
			if (!VM->GetByteCodeCache(&VCache))
			{
				VCache.Data.clear();
				VCache.Debug = Debug;
				VCache.Valid = false;
			}

			return Result;
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Compiler::SaveByteCode(ByteCodeInfo* Info)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(Scope != nullptr, "module should not be empty");
			VI_ASSERT(Info != nullptr, "bytecode should be set");
			VI_ASSERT(Built, "module should be built");
#ifdef VI_ANGELSCRIPT
			CByteCodeStream* Stream = Core::Memory::New<CByteCodeStream>();
			int R = Scope->SaveByteCode(Stream, !Info->Debug);
			Info->Data = Stream->GetCode();
			Core::Memory::Delete(Stream);
			if (R >= 0)
				VI_DEBUG("[vm] OK save bytecode on 0x%" PRIXPTR, (uintptr_t)this);
			return FunctionFactory::ToReturn(R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Compiler::LoadFile(const std::string_view& Path)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(Scope != nullptr, "module should not be empty");
#ifdef VI_ANGELSCRIPT
			if (VCache.Valid)
				return Core::Expectation::Met;

			auto Source = Core::OS::Path::Resolve(Path);
			if (!Source)
				return VirtualException("path not found: " + Core::String(Path));

			if (!Core::OS::File::IsExists(Source->c_str()))
				return VirtualException("file not found: " + Core::String(Path));

			auto Buffer = Core::OS::File::ReadAsString(*Source);
			if (!Buffer)
				return VirtualException("open file error: " + Core::String(Path));

			Core::String Code = *Buffer;
			auto Status = VM->GenerateCode(Processor, *Source, Code);
			if (!Status)
				return VirtualException(std::move(Status.Error().message()));
			else if (Code.empty())
				return Core::Expectation::Met;
			
			auto Result = VM->AddScriptSection(Scope, *Source, Code);
			if (Result)
				VI_DEBUG("[vm] OK load program on 0x%" PRIXPTR " (file)", (uintptr_t)this);
			return Result;
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Compiler::LoadCode(const std::string_view& Name, const std::string_view& Data)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(Scope != nullptr, "module should not be empty");
			VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
#ifdef VI_ANGELSCRIPT
			if (VCache.Valid)
				return Core::Expectation::Met;

			Core::String Buffer(Data);
			auto Status = VM->GenerateCode(Processor, Name, Buffer);
			if (!Status)
				return VirtualException(std::move(Status.Error().message()));
			else if (Buffer.empty())
				return Core::Expectation::Met;

			auto Result = VM->AddScriptSection(Scope, Name, Buffer);
			if (Result)
				VI_DEBUG("[vm] OK load program on 0x%" PRIXPTR, (uintptr_t)this);
			return Result;
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Compiler::LoadByteCodeSync(ByteCodeInfo* Info)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(Scope != nullptr, "module should not be empty");
			VI_ASSERT(Info != nullptr, "bytecode should be set");
#ifdef VI_ANGELSCRIPT
			int R = 0;
			CByteCodeStream* Stream = Core::Memory::New<CByteCodeStream>(Info->Data);
			while ((R = Scope->LoadByteCode(Stream, &Info->Debug)) == asBUILD_IN_PROGRESS)
				std::this_thread::sleep_for(std::chrono::microseconds(COMPILER_BLOCKED_WAIT_US));
			Core::Memory::Delete(Stream);
			if (R >= 0)
				VI_DEBUG("[vm] OK load bytecode on 0x%" PRIXPTR, (uintptr_t)this);
			return FunctionFactory::ToReturn(R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> Compiler::CompileSync()
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(Scope != nullptr, "module should not be empty");
#ifdef VI_ANGELSCRIPT
			if (VCache.Valid)
				return LoadByteCodeSync(&VCache);

			int R = 0;
			while ((R = Scope->Build()) == asBUILD_IN_PROGRESS)
				std::this_thread::sleep_for(std::chrono::microseconds(COMPILER_BLOCKED_WAIT_US));

			VM->ClearSections();
			Built = (R >= 0);
			if (!Built)
				return FunctionFactory::ToReturn(R);

			VI_DEBUG("[vm] OK compile on 0x%" PRIXPTR, (uintptr_t)this);
			if (VCache.Name.empty())
				return FunctionFactory::ToReturn(R);

			auto Status = SaveByteCode(&VCache);
			if (Status)
				VM->SetByteCodeCache(&VCache);
			return Status;
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsPromiseVM<void> Compiler::LoadByteCode(ByteCodeInfo* Info)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(Scope != nullptr, "module should not be empty");
			VI_ASSERT(Info != nullptr, "bytecode should be set");
#ifdef VI_ANGELSCRIPT
			return Core::Cotask<ExpectsVM<void>>(std::bind(&Compiler::LoadByteCodeSync, this, Info));
#else
			return ExpectsPromiseVM<void>(VirtualException(VirtualError::NOT_SUPPORTED));
#endif
		}
		ExpectsPromiseVM<void> Compiler::Compile()
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(Scope != nullptr, "module should not be empty");
#ifdef VI_ANGELSCRIPT
			return Core::Cotask<ExpectsVM<void>>(std::bind(&Compiler::CompileSync, this));
#else
			return ExpectsPromiseVM<void>(VirtualException(VirtualError::NOT_SUPPORTED));
#endif
		}
		ExpectsPromiseVM<void> Compiler::CompileFile(const std::string_view& Name, const std::string_view& ModuleName, const std::string_view& EntryName)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			auto Status = Prepare(ModuleName, Name);
			if (!Status)
				return ExpectsPromiseVM<void>(Status);

			Status = LoadFile(Name);
			if (!Status)
				return ExpectsPromiseVM<void>(Status);

			return Compile();
#else
			return ExpectsPromiseVM<void>(VirtualException(VirtualError::NOT_SUPPORTED));
#endif
		}
		ExpectsPromiseVM<void> Compiler::CompileMemory(const std::string_view& Buffer, const std::string_view& ModuleName, const std::string_view& EntryName)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(!Buffer.empty(), "buffer should not be empty");
#ifdef VI_ANGELSCRIPT
			Core::String Name = "anonymous:" + Core::ToString(Counter++);
			auto Status = Prepare(ModuleName, "anonymous");
			if (!Status)
				return ExpectsPromiseVM<void>(Status);

			Status = LoadCode("anonymous", Buffer);
			if (!Status)
				return ExpectsPromiseVM<void>(Status);

			return Compile();
#else
			return ExpectsPromiseVM<void>(VirtualException(VirtualError::NOT_SUPPORTED));
#endif
		}
		ExpectsPromiseVM<Function> Compiler::CompileFunction(const std::string_view& Buffer, const std::string_view& Returns, const std::string_view& Args, Core::Option<size_t>&& FunctionId)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(!Buffer.empty(), "buffer should not be empty");
			VI_ASSERT(Scope != nullptr, "module should not be empty");
			VI_ASSERT(Built, "module should be built");
#ifdef VI_ANGELSCRIPT
			Core::String Code = Core::String(Buffer);
			Core::String Name = " __vfunc" + Core::ToString(FunctionId ? *FunctionId : (Counter + 1));
			auto Status = VM->GenerateCode(Processor, Name, Code);
			if (!Status)
				return ExpectsPromiseVM<Function>(VirtualException(std::move(Status.Error().message())));

			Core::String Eval;
			Eval.append(Returns.empty() ? "void" : Returns);
			Eval.append(Name);
			Eval.append("(");
			Eval.append(Args);
			Eval.append("){");

			if (!FunctionId)
				++Counter;

			if (!Returns.empty() && Returns != "void")
			{
				size_t Offset = Buffer.size();
				while (Offset > 0)
				{
					char U = Buffer[Offset - 1];
					if (U == '\"' || U == '\'')
					{
						--Offset;
						while (Offset > 0)
						{
							if (Buffer[--Offset] == U)
								break;
						}

						continue;
					}
					else if (U == ';' && Offset < Buffer.size())
						break;
					--Offset;
				}

				if (Offset > 0)
					Eval.append(Buffer.substr(0, Offset));

				size_t Size = Returns.size();
				Eval.append("return ");
				if (Returns[Size - 1] == '@')
				{
					Eval.append("@");
					Eval.append(Returns.data(), Size - 1);
				}
				else
					Eval.append(Returns);
				
				Eval.append("(");
				Eval.append(Buffer.substr(Offset));
				if (Eval.back() == ';')
					Eval.erase(Eval.end() - 1);
				Eval.append(");}");
			}
			else
			{
				Eval.append(Buffer);
				if (Eval.back() == ';')
					Eval.erase(Eval.end() - 1);
				Eval.append(";}");
			}

			asIScriptModule* Source = GetModule().GetModule();
			return Core::Cotask<ExpectsVM<Function>>([Source, Eval = std::move(Eval)]() mutable
			{
				asIScriptFunction* FunctionPointer = nullptr; int R = 0;
				while ((R = Source->CompileFunction("__vfunc", Eval.c_str(), -1, asCOMP_ADD_TO_MODULE, &FunctionPointer)) == asBUILD_IN_PROGRESS)
					std::this_thread::sleep_for(std::chrono::microseconds(COMPILER_BLOCKED_WAIT_US));
				return FunctionFactory::ToReturn<Function>(R, Function(FunctionPointer));
			});
#else
			return ExpectsPromiseVM<Function>(VirtualException(VirtualError::NOT_SUPPORTED));
#endif
		}
		VirtualMachine* Compiler::GetVM() const
		{
			return VM;
		}
		Module Compiler::GetModule() const
		{
			return Scope;
		}
		Compute::Preprocessor* Compiler::GetProcessor() const
		{
			return Processor;
		}

		DebuggerContext::DebuggerContext(DebugType Type) noexcept : LastContext(nullptr), ForceSwitchThreads(0), LastFunction(nullptr), VM(nullptr), Action(Type == DebugType::Suspended ? DebugAction::Trigger : DebugAction::Continue), InputError(false), Attachable(Type != DebugType::Detach)
		{
			LastCommandAtStackLevel = 0;
			AddDefaultCommands();
		}
		DebuggerContext::~DebuggerContext() noexcept
		{
#ifdef VI_ANGELSCRIPT
			if (VM != nullptr)
				Core::Memory::Release(VM->GetEngine());
#endif
		}
		void DebuggerContext::AddDefaultCommands()
		{
			AddCommand("h, help", "show debugger commands", ArgsType::NoArgs, [this](ImmediateContext* Context, const Core::Vector<Core::String>& Args)
			{
				size_t Max = 0;
				for (auto& Next : Descriptions)
				{
					if (Next.first.size() > Max)
						Max = Next.first.size();
				}

				for (auto& Next : Descriptions)
				{
					size_t Spaces = Max - Next.first.size();
					Output("  ");
					Output(Next.first);
					for (size_t i = 0; i < Spaces; i++)
						Output(" ");
					Output(" - ");
					Output(Next.second);
					Output("\n");
				}
				return false;
			});
			AddCommand("ls, loadsys", "load global system addon", ArgsType::Expression, [this](ImmediateContext* Context, const Core::Vector<Core::String>& Args)
			{
				if (!Args.empty() && !Args[0].empty())
					VM->ImportSystemAddon(Args[0]);
				return false;
			});
			AddCommand("le, loadext", "load global external addon", ArgsType::Expression, [this](ImmediateContext* Context, const Core::Vector<Core::String>& Args)
			{
				if (!Args.empty() && !Args[0].empty())
					VM->ImportAddon(Args[0]);
				return false;
			});
			AddCommand("e, eval", "evaluate script expression", ArgsType::Expression, [this](ImmediateContext* Context, const Core::Vector<Core::String>& Args)
			{
				if (!Args.empty() && !Args[0].empty())
					ExecuteExpression(Context, Args[0], Core::String(), nullptr);

				return false;
			});
			AddCommand("b, break", "add a break point", ArgsType::Array, [this](ImmediateContext* Context, const Core::Vector<Core::String>& Args)
			{
				if (Args.size() > 1)
				{
					if (Args[0].empty())
						goto BreakFailure;

					auto Numeric = Core::FromString<int>(Args[1]);
					if (!Numeric)
						goto BreakFailure;

					AddFileBreakPoint(Args[0], *Numeric);
					return false;
				}
				else if (Args.size() == 1)
				{
					if (Args[0].empty())
						goto BreakFailure;

					auto Numeric = Core::FromString<int>(Args[0]);
					if (!Numeric)
					{
						AddFuncBreakPoint(Args[0]);
						return false;
					}

					std::string_view File = "";
					Context->GetLineNumber(0, 0, &File);
					if (File.empty())
						goto BreakFailure;

					AddFileBreakPoint(File, *Numeric);
					return false;
				}

			BreakFailure:
				Output(
					"  break <file name> <line number>\n"
					"  break <function name | line number>\n");
				return false;
			});
			AddCommand("del, clear", "remove a break point", ArgsType::Expression, [this](ImmediateContext* Context, const Core::Vector<Core::String>& Args)
			{
				if (Args.empty())
				{
				ClearFailure:
					Output("  clear <all | breakpoint number>\n");
					return false;
				}

				if (Args[0] == "all")
				{
					BreakPoints.clear();
					Output("  all break points have been removed\n");
					return false;
				}

				auto Numeric = Core::FromString<uint64_t>(Args[0]);
				if (!Numeric)
					goto ClearFailure;

				size_t Offset = (size_t)*Numeric;
				if (Offset >= BreakPoints.size())
					goto ClearFailure;

				BreakPoints.erase(BreakPoints.begin() + Offset);
				ListBreakPoints();
				return false;
			});
			AddCommand("p, print", "print variable value", ArgsType::Expression, [this](ImmediateContext* Context, const Core::Vector<Core::String>& Args)
			{
				if (Args[0].empty())
				{
					Output("  print <expression>\n");
					return false;
				}

				PrintValue(Args[0], Context);
				return false;
			});
			AddCommand("d, dump", "dump compiled function bytecode by name or declaration", ArgsType::Expression, [this](ImmediateContext* Context, const Core::Vector<Core::String>& Args)
			{
				if (Args[0].empty())
				{
					Output("  dump <function declaration | function name>\n");
					return false;
				}

				PrintByteCode(Args[0], Context);
				return false;
			});
			AddCommand("i, info", "show info about of specific topic", ArgsType::Array, [this](ImmediateContext* Context, const Core::Vector<Core::String>& Args)
			{
				if (Args.empty())
					goto InfoFailure;

				if (Args[0] == "b" || Args[0] == "break")
				{
					ListBreakPoints();
					return false;
				}
				if (Args[0] == "e" || Args[0] == "exception")
				{
					ShowException(Context);
					return false;
				}
				else if (Args[0] == "s" || Args[0] == "stack")
				{
					if (Args.size() > 1)
					{
						auto Numeric = Core::FromString<uint32_t>(Args[1]);
						if (Numeric)
							ListStackRegisters(Context, *Numeric);
						else
							Output("  invalid stack level");
					}
					else
						ListStackRegisters(Context, 0);

					return false;
				}
				else if (Args[0] == "l" || Args[0] == "locals")
				{
					ListLocalVariables(Context);
					return false;
				}
				else if (Args[0] == "g" || Args[0] == "globals")
				{
					ListGlobalVariables(Context);
					return false;
				}
				else if (Args[0] == "m" || Args[0] == "members")
				{
					ListMemberProperties(Context);
					return false;
				}
				else if (Args[0] == "t" || Args[0] == "threads")
				{
					ListThreads();
					return false;
				}
				else if (Args[0] == "c" || Args[0] == "code")
				{
					ListSourceCode(Context);
					return false;
				}
				else if (Args[0] == "a" || Args[0] == "addons")
				{
					ListAddons();
					return false;
				}
				else if (Args[0] == "ri" || Args[0] == "interfaces")
				{
					ListInterfaces(Context);
					return false;
				}
				else if (Args[0] == "gc" || Args[0] == "garbage")
				{
					ListStatistics(Context);
					return false;
				}

			InfoFailure:
				Output(
					"  info b, info break - show breakpoints\n"
					"  info s, info stack <level?> - show stack registers\n"
					"  info e, info exception - show current exception\n"
					"  info l, info locals - show local variables\n"
					"  info m, info members - show member properties\n"
					"  info g, info globals - show global variables\n"
					"  info t, info threads - show suspended threads\n"
					"  info c, info code - show source code section\n"
					"  info a, info addons - show imported addons\n"
					"  info ri, info interfaces - show registered script virtual interfaces\n"
					"  info gc, info garbage - show gc statistics\n");
				return false;
			});
			AddCommand("t, thread", "switch to thread by it's number", ArgsType::Array, [this](ImmediateContext* Context, const Core::Vector<Core::String>& Args)
			{
				if (Args.empty())
				{
				ThreadFailure:
					Output("  thread <thread number>\n");
					return false;
				}

				auto Numeric = Core::FromString<uint64_t>(Args[0]);
				if (!Numeric)
					goto ThreadFailure;

				size_t Index = (size_t)*Numeric;
				if (Index >= Threads.size())
					goto ThreadFailure;

				auto& Thread = Threads[Index];
				if (Thread.Context == Context)
					return false;

				Action = DebugAction::Switch;
				LastContext = Thread.Context;
				return true;
			});
			AddCommand("c, continue", "continue execution", ArgsType::NoArgs, [this](ImmediateContext* Context, const Core::Vector<Core::String>& Args)
			{
				Action = DebugAction::Continue;
				return true;
			});
			AddCommand("s, step", "step into subroutine", ArgsType::NoArgs, [this](ImmediateContext* Context, const Core::Vector<Core::String>& Args)
			{
				Action = DebugAction::StepInto;
				return true;
			});
			AddCommand("n, next", "step over subroutine", ArgsType::NoArgs, [this](ImmediateContext* Context, const Core::Vector<Core::String>& Args)
			{
				Action = DebugAction::StepOver;
				LastCommandAtStackLevel = (unsigned int)Context->GetCallstackSize();
				return true;
			});
			AddCommand("fin, finish", "step out of subroutine", ArgsType::NoArgs, [this](ImmediateContext* Context, const Core::Vector<Core::String>& Args)
			{
				Action = DebugAction::StepOut;
				LastCommandAtStackLevel = (unsigned int)Context->GetCallstackSize();
				return true;
			});
			AddCommand("a, abort", "abort current execution", ArgsType::NoArgs, [](ImmediateContext* Context, const Core::Vector<Core::String>& Args)
			{
				Context->Abort();
				return false;
			});
			AddCommand("bt, backtrace", "show current callstack", ArgsType::NoArgs, [this](ImmediateContext* Context, const Core::Vector<Core::String>& Args)
			{
				PrintCallstack(Context);
				return false;
			});
		}
		void DebuggerContext::AddCommand(const std::string_view& Name, const std::string_view& Description, ArgsType Type, CommandCallback&& Callback)
		{
			Descriptions[Core::String(Name)] = Description;
			for (auto& Command : Core::Stringify::Split(Name, ','))
			{
				Core::Stringify::Trim(Command);
				auto& Data = Commands[Command];
				Data.Callback = std::move(Callback);
				Data.Description = Description;
				Data.Arguments = Type;
			}
		}
		void DebuggerContext::SetInterruptCallback(InterruptCallback&& Callback)
		{
			OnInterrupt = std::move(Callback);
		}
		void DebuggerContext::SetOutputCallback(OutputCallback&& Callback)
		{
			OnOutput = std::move(Callback);
		}
		void DebuggerContext::SetInputCallback(InputCallback&& Callback)
		{
			OnInput = std::move(Callback);
		}
		void DebuggerContext::SetExitCallback(ExitCallback&& Callback)
		{
			OnExit = std::move(Callback);
		}
		void DebuggerContext::AddToStringCallback(const TypeInfo& Type, ToStringCallback&& Callback)
		{
			FastToStringCallbacks[Type.GetTypeInfo()] = std::move(Callback);
		}
		void DebuggerContext::AddToStringCallback(const std::string_view& Type, ToStringTypeCallback&& Callback)
		{
			for (auto& Item : Core::Stringify::Split(Type, ','))
			{
				Core::Stringify::Trim(Item);
				SlowToStringCallbacks[Item] = std::move(Callback);
			}
		}
		void DebuggerContext::LineCallback(asIScriptContext* Base)
		{
#ifdef VI_ANGELSCRIPT
			ImmediateContext* Context = ImmediateContext::Get(Base);
			VI_ASSERT(Context != nullptr, "context should be set");
			auto State = Base->GetState();
			if (State != asEXECUTION_ACTIVE && State != asEXECUTION_EXCEPTION)
			{
				if (State != asEXECUTION_SUSPENDED)
					ClearThread(Context);
				return;
			}

			ThreadData Thread = GetThread(Context);
			if (State == asEXECUTION_EXCEPTION)
				ForceSwitchThreads = 1;

		Retry:
			Core::UMutex<std::recursive_mutex> Unique(ThreadBarrier);
			if (ForceSwitchThreads > 0)
			{
				if (State == asEXECUTION_EXCEPTION)
				{
					Action = DebugAction::Trigger;
					ForceSwitchThreads = 0;
					if (LastContext != Thread.Context)
					{
						LastContext = Context;
						Output("  exception handler caused switch to thread " + Core::OS::Process::GetThreadId(Thread.Id) + " after last continuation\n");
					}
					ShowException(Context);
				}
				else
				{
					Unique.Negate();
					std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_BLOCKED_WAIT_MS));
					goto Retry;
				}
			}
			else if (Action == DebugAction::Continue)
			{
				LastContext = Context;
				if (!CheckBreakPoint(Context))
					return;
			}
			else if (Action == DebugAction::Switch)
			{
				if (LastContext != Thread.Context)
				{
					Unique.Negate();
					std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_BLOCKED_WAIT_MS));
					goto Retry;
				}
				else
				{
					LastContext = Context;
					Action = DebugAction::Trigger;
					Output("  switched to thread " + Core::OS::Process::GetThreadId(Thread.Id) + "\n");
				}
			}
			else if (Action == DebugAction::StepOver)
			{
				LastContext = Context;
				if (Base->GetCallstackSize() > LastCommandAtStackLevel && !CheckBreakPoint(Context))
					return;
			}
			else if (Action == DebugAction::StepOut)
			{
				LastContext = Context;
				if (Base->GetCallstackSize() >= LastCommandAtStackLevel && !CheckBreakPoint(Context))
					return;
			}
			else if (Action == DebugAction::StepInto)
			{
				LastContext = Context;
				CheckBreakPoint(Context);
			}
			else if (Action == DebugAction::Interrupt)
			{
				if (OnInterrupt)
					OnInterrupt(true);

				LastContext = Context;
				Action = DebugAction::Trigger;
				Output("  execution interrupt signal has been raised, moving to input trigger\n");
				PrintCallstack(Context);
			}
			else if (Action == DebugAction::Trigger)
				LastContext = Context;

			Input(Context);
			if (Action == DebugAction::Switch || ForceSwitchThreads > 0)
			{
				Unique.Negate();
				std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_BLOCKED_WAIT_MS));
				goto Retry;
			}

			if (OnInterrupt)
				OnInterrupt(false);
#endif
		}
		void DebuggerContext::ExceptionCallback(asIScriptContext* Base)
		{
#ifdef VI_ANGELSCRIPT
			if (!Base->WillExceptionBeCaught())
				LineCallback(Base);
#endif
		}
		void DebuggerContext::Trigger(ImmediateContext* Context, uint64_t TimeoutMs)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			LineCallback(Context->GetContext());
			if (TimeoutMs > 0)
				std::this_thread::sleep_for(std::chrono::milliseconds(TimeoutMs));
		}
		void DebuggerContext::ThrowInternalException(const std::string_view& Message)
		{
			if (Message.empty())
				return;

			auto Exception = Bindings::Exception::Pointer(Core::String(Message));
			Output("  " + Exception.GetType() + ": " + Exception.GetText() + "\n");
		}
		void DebuggerContext::AllowInputAfterFailure()
		{
			InputError = false;
		}
		void DebuggerContext::Input(ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			VI_ASSERT(Context->GetContext() != nullptr, "context should be set");
			if (InputError)
				return;


			for (;;)
			{
				Core::String Data;
				Output("(dbg) ");
				if (OnInput)
				{
					if (!OnInput(Data))
					{
						InputError = true;
						break;
					}
				}
				else if (!Core::Console::Get()->ReadLine(Data, 1024))
				{
					InputError = true;
					break;
				}
				
				if (InterpretCommand(Data, Context))
					break;
			}
			if (!InputError)
				return;

			for (auto& Thread : Threads)
				Thread.Context->Abort();
		}
		void DebuggerContext::PrintValue(const std::string_view& Expression, ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			VI_ASSERT(Context->GetFunction().IsValid(), "context current function should be set");
#ifdef VI_ANGELSCRIPT
			auto IsGlobalSearchOnly = [](const Core::String& Name) -> bool
			{
				return Core::Stringify::StartsWith(Name, "::");
			};
			auto GetIndexInvocation = [](const Core::String& Name) -> Core::Option<Core::String>
			{
				if (Name.empty() || Name.front() != '[' || Name.back() != ']')
					return Core::Optional::None;

				return Name.substr(1, Name.size() - 2);
			};
			auto GetMethodInvocation = [](const Core::String& Name) -> Core::Option<Core::String>
			{
				if (Name.empty() || Name.front() != '(' || Name.back() != ')')
					return Core::Optional::None;

				return Name.substr(1, Name.size() - 2);
			};
			auto GetNamespaceScope = [](Core::String& Name) -> Core::String
			{
				size_t Position = Name.rfind("::");
				if (Position == std::string::npos)
					return Core::String();

				auto Namespace = Name.substr(0, Position);
				Name.erase(0, Namespace.size() + 2);
				return Namespace;
			};
			auto ParseExpression = [](const std::string_view& Expression) -> Core::Vector<Core::String>
			{
				Core::Vector<Core::String> Stack;
				size_t Start = 0, End = 0;
				while (End < Expression.size())
				{
					char V = Expression[End];
					if (V == '(' || V == '[')
					{
						size_t Offset = End + 1, Nesting = 1;
						while (Nesting > 0 && Offset < Expression.size())
						{
							char N = Expression[Offset++];
							if (N == (V == '(' ? ')' : ']'))
								--Nesting;
							else if (N == V)
								++Nesting;
						}
						End = Offset;
					}
					else if (V == '.')
					{
						Stack.push_back(Core::String(Expression.substr(Start, End - Start)));
						Start = ++End;
					}
					else
						++End;
				}

				if (Start < End)
					Stack.push_back(Core::String(Expression.substr(Start, End - Start)));

				return Stack;
			};
			auto SparsifyStack = [](Core::Vector<Core::String>& Stack) -> bool
			{
			Retry:
				size_t Offset = 0;
				for (auto& Item : Stack)
				{
					auto& Name = Core::Stringify::Trim(Item);
					if (Name.empty() || Name.front() == '[' || Name.front() == '(')
					{
					Next:
						++Offset;
						continue;
					}

					if (Name.back() == ']')
					{
						size_t Position = Name.find('[');
						if (Position == std::string::npos)
							goto Next;

						Core::String Index = Name.substr(Position);
						Name.erase(Position);
						Stack.insert(Stack.begin() + Offset + 1, std::move(Index));
						goto Retry;
					}
					else if (Name.back() == ')')
					{
						size_t Position = Name.find('(');
						if (Position == std::string::npos)
							goto Next;

						Core::String Index = Name.substr(Position);
						Name.erase(Position);
						Stack.insert(Stack.begin() + Offset + 1, std::move(Index));
						goto Retry;
					}
					goto Next;
				}

				return true;
			};

			asIScriptEngine* Engine = Context->GetVM()->GetEngine();
			asIScriptContext* Base = Context->GetContext();
			VI_ASSERT(Base != nullptr, "context should be set");

			auto Stack = ParseExpression(Expression);
			if (Stack.empty() || !SparsifyStack(Stack))
				return Output("  no tokens found in the expression");

			int TopTypeId = asTYPEID_VOID;
			asIScriptFunction* ThisFunction = nullptr;
			void* ThisPointer = nullptr;
			int ThisTypeId = asTYPEID_VOID;
			Core::String Callable;
			Core::String Last;
			size_t Top = 0;

			while (!Stack.empty())
			{
				auto& Name = Stack.front();
				if (Top > 0)
				{
					auto Index = GetIndexInvocation(Name);
					auto Method = GetMethodInvocation(Name);
					if (Index)
					{
						asITypeInfo* Type = Engine->GetTypeInfoById(ThisTypeId);
						if (!Type)
							return Output("  symbol <" + Last + "> type is not iterable\n");

						asIScriptFunction* IndexOperator = nullptr;
						for (asUINT i = 0; i < Type->GetMethodCount(); i++)
						{
							asIScriptFunction* Next = Type->GetMethodByIndex(i);
							if (!strcmp(Next->GetName(), "opIndex"))
							{
								IndexOperator = Next;
								break;
							}
						}

						if (!IndexOperator)
							return Output("  symbol <" + Last + "> does not have opIndex method\n");

						const char* TypeDeclaration = nullptr;
						asITypeInfo* TopType = Engine->GetTypeInfoById(TopTypeId);
						if (TopType != nullptr)
						{
							for (asUINT i = 0; i < TopType->GetPropertyCount(); i++)
							{
								const char* VarName = nullptr;
								TopType->GetProperty(i, &VarName);
								if (Last == VarName)
								{
									TypeDeclaration = TopType->GetPropertyDeclaration(i, true);
									break;
								}
							}
						}

						if (!TypeDeclaration)
						{
							asIScriptModule* Module = Base->GetFunction()->GetModule();
							for (asUINT i = 0; i < Module->GetGlobalVarCount(); i++)
							{
								const char* VarName = nullptr;
								Module->GetGlobalVar(i, &VarName);
								if (Last == VarName)
								{
									TypeDeclaration = Module->GetGlobalVarDeclaration(i, true);
									break;
								}
							}
						}

						if (!TypeDeclaration)
						{
							int VarCount = Base->GetVarCount();
							if (VarCount > 0)
							{
								for (asUINT n = 0; n < (asUINT)VarCount; n++)
								{
									const char* VarName = nullptr;
									Base->GetVar(n, 0, &VarName);
									if (Base->IsVarInScope(n) && VarName != 0 && Name == VarName)
									{
										TypeDeclaration = Base->GetVarDeclaration(n, 0, true);
										break;
									}
								}
							}
						}

						if (!TypeDeclaration)
							return Output("  symbol <" + Last + "> does not have type decl for <" + Name + "> to call operator method\n");

						Core::String Args = TypeDeclaration;
						if (ThisTypeId & asTYPEID_OBJHANDLE)
						{
							ThisPointer = *(void**)ThisPointer;
							Core::Stringify::Replace(Args, "& ", "@ ");
						}
						else
							Core::Stringify::Replace(Args, "& ", "&in ");
						ExecuteExpression(Context, Last + Name, Args, [ThisPointer](ImmediateContext* Context)
						{
							Context->SetArgObject(0, ThisPointer);
						});
						return;
					}
					else if (Method)
					{
						if (!ThisFunction)
							return Output("  symbol <" + Last + "> is not a function\n");

						Core::String Call = ThisFunction->GetName();
						if (!ThisPointer)
						{
							ExecuteExpression(Context, Core::Stringify::Text("%s(%s)", Call.c_str(), Method->c_str()), Core::String(), nullptr);
							return;
						}

						const char* TypeDeclaration = nullptr;
						asITypeInfo* TopType = Engine->GetTypeInfoById(TopTypeId);
						if (TopType != nullptr)
						{
							for (asUINT i = 0; i < TopType->GetPropertyCount(); i++)
							{
								const char* VarName = nullptr;
								TopType->GetProperty(i, &VarName);
								if (Callable == VarName)
								{
									TypeDeclaration = TopType->GetPropertyDeclaration(i, true);
									break;
								}
							}
						}

						if (!TypeDeclaration)
						{
							asIScriptModule* Module = Base->GetFunction()->GetModule();
							for (asUINT i = 0; i < Module->GetGlobalVarCount(); i++)
							{
								const char* VarName = nullptr;
								Module->GetGlobalVar(i, &VarName);
								if (Callable == VarName)
								{
									TypeDeclaration = Module->GetGlobalVarDeclaration(i, true);
									break;
								}
							}
						}

						if (!TypeDeclaration)
						{
							int VarCount = Base->GetVarCount();
							if (VarCount > 0)
							{
								for (asUINT n = 0; n < (asUINT)VarCount; n++)
								{
									const char* VarName = nullptr;
									Base->GetVar(n, 0, &VarName);
									if (Base->IsVarInScope(n) && VarName != 0 && Name == VarName)
									{
										TypeDeclaration = Base->GetVarDeclaration(n, 0, true);
										break;
									}
								}
							}
						}

						if (!TypeDeclaration)
							return Output("  symbol <" + Callable + "> does not have type decl for <" + Last + "> to call method\n");

						Core::String Args = TypeDeclaration;
						if (ThisTypeId & asTYPEID_OBJHANDLE)
						{
							ThisPointer = *(void**)ThisPointer;
							Core::Stringify::Replace(Args, "& ", "@ ");
						}
						else
							Core::Stringify::Replace(Args, "& ", "&in ");
						ExecuteExpression(Context, Core::Stringify::Text("%s.%s(%s)", Callable.c_str(), Call.c_str(), Method->c_str()), Args, [ThisPointer](ImmediateContext* Context)
						{
							Context->SetArgObject(0, ThisPointer);
						});
						return;
					}
					else
					{
						asITypeInfo* Type = Engine->GetTypeInfoById(ThisTypeId);
						if (!Type)
							return Output("  symbol <" + Last + "> type is not iterable\n");

						for (asUINT n = 0; n < Type->GetPropertyCount(); n++)
						{
							const char* PropName = 0;
							int Offset = 0;
							bool IsReference = 0;
							int CompositeOffset = 0;
							bool IsCompositeIndirect = false;
							int TypeId = 0;

							Type->GetProperty(n, &PropName, &TypeId, 0, 0, &Offset, &IsReference, 0, &CompositeOffset, &IsCompositeIndirect);
							if (Name != PropName)
								continue;

							if (ThisTypeId & asTYPEID_OBJHANDLE)
								ThisPointer = *(void**)ThisPointer;

							ThisPointer = (void*)(((asBYTE*)ThisPointer) + CompositeOffset);
							if (IsCompositeIndirect)
								ThisPointer = *(void**)ThisPointer;

							ThisPointer = (void*)(((asBYTE*)ThisPointer) + Offset);
							if (IsReference)
								ThisPointer = *(void**)ThisPointer;

							ThisTypeId = TypeId;
							goto NextIteration;
						}

						for (asUINT n = 0; n < Type->GetMethodCount(); n++)
						{
							asIScriptFunction* MethodFunction = Type->GetMethodByIndex(n);
							if (!strcmp(MethodFunction->GetName(), Name.c_str()))
							{
								ThisFunction = MethodFunction;
								Callable = Last;
								goto NextIteration;
							}
						}

						return Output("  symbol <" + Name + "> was not found in <" + Last + ">\n");
					}
				}
				else
				{
					if (Name == "this")
					{
						asIScriptFunction* Function = Base->GetFunction();
						if (!Function)
							return Output("  no function to be observed\n");

						ThisPointer = Base->GetThisPointer();
						ThisTypeId = Base->GetThisTypeId();
						if (ThisPointer != nullptr && ThisTypeId > 0 && Function->GetObjectType() != nullptr)
							goto NextIteration;

						return Output("  this function is not a method\n");
					}

					bool GlobalOnly = IsGlobalSearchOnly(Name);
					if (!GlobalOnly)
					{
						int VarCount = Base->GetVarCount();
						if (VarCount > 0)
						{
							for (asUINT n = 0; n < (asUINT)VarCount; n++)
							{
								const char* VarName = nullptr;
								Base->GetVar(n, 0, &VarName, &ThisTypeId);
								if (Base->IsVarInScope(n) && VarName != 0 && Name == VarName)
								{
									ThisPointer = Base->GetAddressOfVar(n);
									goto NextIteration;
								}
							}
						}
					}

					asIScriptFunction* Function = Base->GetFunction();
					if (!Function)
						return Output("  no function to be observed\n");

					asIScriptModule* Module = Function->GetModule();
					if (!Module)
						return Output("  no module to be observed\n");

					auto Namespace = GetNamespaceScope(Name);
					for (asUINT n = 0; n < Module->GetGlobalVarCount(); n++)
					{
						const char* VarName = nullptr, *VarNamespace = nullptr;
						Module->GetGlobalVar(n, &VarName, &VarNamespace, &ThisTypeId);
						if (Name == VarName && Namespace == VarNamespace)
						{
							ThisPointer = Module->GetAddressOfGlobalVar(n);
							goto NextIteration;
						}
					}

					for (asUINT n = 0; n < Engine->GetGlobalFunctionCount(); n++)
					{
						asIScriptFunction* GlobalFunction = Engine->GetGlobalFunctionByIndex(n);
						if (!strcmp(GlobalFunction->GetName(), Name.c_str()) && (!GlobalFunction->GetNamespace() || Namespace == GlobalFunction->GetNamespace()))
						{
							ThisFunction = GlobalFunction;
							Callable = Last;
							goto NextIteration;
						}
					}

					Namespace = (Namespace.empty() ? "" : Namespace + "::");
					return Output("  symbol <" + Namespace + Name + "> was not found in " + Core::String(GlobalOnly ? "global" : "global or local") + " scope\n");
				}
			NextIteration:
				Last = Name;
				Stack.erase(Stack.begin());
				if (Top % 2 == 0)
					TopTypeId = ThisTypeId;
				++Top;
			}

			if (ThisFunction != nullptr)
			{
				Output("  ");
				Output(ThisFunction->GetDeclaration(true, true, true));
				Output("\n");
			}
			else if (ThisPointer != nullptr)
			{
				Core::String Indent = "  ";
				Core::StringStream Stream;
				Stream << Indent << ToString(Indent, 3, ThisPointer, ThisTypeId) << std::endl;
				Output(Stream.str());
			}
#endif
		}
		void DebuggerContext::PrintByteCode(const std::string_view& FunctionDecl, ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			VI_ASSERT(Core::Stringify::IsCString(FunctionDecl), "fndecl should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptContext* Base = Context->GetContext();
			VI_ASSERT(Base != nullptr, "context should be set");

			asIScriptFunction* Function = Context->GetContext()->GetFunction();
			if (!Function)
				return Output("  context was not found\n");

			asIScriptModule* Module = Function->GetModule();
			if (!Module)
				return Output("  module was not found\n");

			Function = Module->GetFunctionByName(FunctionDecl.data());
			if (!Function)
			{
				Function = Module->GetFunctionByDecl(FunctionDecl.data());
				if (!Function)
				{
					auto Keys = Core::Stringify::Split(FunctionDecl, "::");
					if (Keys.size() >= 2)
					{
						size_t Length = Keys.size() - 1;
						Core::String TypeName = Keys.front();
						for (size_t i = 1; i < Length; i++)
						{
							TypeName += "::";
							TypeName += Keys[i];
						}

						auto* Type = Module->GetTypeInfoByName(TypeName.c_str());
						if (Type != nullptr)
						{
							Function = Type->GetMethodByName(Keys.back().c_str());
							if (!Function)
								Function = Type->GetMethodByDecl(Keys.back().c_str());
						}
					}
				}
			}

			if (!Function)
				return Output("  function was not found\n");

			Core::StringStream Stream;
			Stream << "  function <" << Function->GetName() << "> disassembly:\n";

			asUINT Offset = 0, Size = 0, Instructions = 0;
			asDWORD* ByteCode = Function->GetByteCode(&Size);
			while (Offset < Size)
			{
				Stream << "  ";
				Offset = (asUINT)ByteCodeLabelToText(Stream, VM, ByteCode, Offset, false, false);
				++Instructions;
			}

			Stream << "  " << Instructions << " instructions (" << Size << " bytes)\n";
			Output(Stream.str());
#endif
		}
		void DebuggerContext::ShowException(ImmediateContext* Context)
		{
			Core::StringStream Stream;
			auto Exception = Bindings::Exception::GetException();
			if (Exception.Empty())
				return;

			Core::String Data = Exception.What();
			auto ExceptionLines = Core::Stringify::Split(Data, '\n');
			if (!Context->WillExceptionBeCaught() && !ExceptionLines.empty())
				ExceptionLines[0] = "uncaught " + ExceptionLines[0];

			for (auto& Line : ExceptionLines)
			{
				if (Line.empty())
					continue;

				if (Line.front() == '#')
					Stream << "    " << Line << "\n";
				else
					Stream << "  " << Line << "\n";
			}

			std::string_view File = "";
			int ColumnNumber = 0;
			int LineNumber = Context->GetExceptionLineNumber(&ColumnNumber, &File);
			if (!File.empty() && LineNumber > 0)
			{
				auto Code = VM->GetSourceCodeAppendixByPath("exception origin", File, LineNumber, ColumnNumber, 5);
				if (Code)
					Stream << *Code << "\n";
			}
			Output(Stream.str());
		}
		void DebuggerContext::ListBreakPoints()
		{
			Core::StringStream Stream;
			for (size_t b = 0; b < BreakPoints.size(); b++)
			{
				if (BreakPoints[b].Function)
					Stream << "  " << b << " - " << BreakPoints[b].Name << std::endl;
				else
					Stream << "  " << b << " - " << BreakPoints[b].Name << ":" << BreakPoints[b].Line << std::endl;
			}
			Output(Stream.str());
		}
		void DebuggerContext::ListThreads()
		{
#ifdef VI_ANGELSCRIPT
			Core::UnorderedSet<Core::String> Ids;
			Core::StringStream Stream;
			size_t Index = 0;
			for (auto& Item : Threads)
			{
				asIScriptContext* Context = Item.Context->GetContext();
				asIScriptFunction* Function = Context->GetFunction();
				if (LastContext == Item.Context)
					Stream << "  * ";
				else
					Stream << "  ";

				Core::String ThreadId = Core::OS::Process::GetThreadId(Item.Id);
				Stream << "#" << Index++ << " " << (Ids.find(ThreadId) != Ids.end() ? "coroutine" : "thread") << " " << ThreadId << ", ";
				Ids.insert(ThreadId);

				if (Function != nullptr)
				{
					if (Function->GetFuncType() == asFUNC_SCRIPT)
						Stream << "source \"" << (Function->GetScriptSectionName() ? Function->GetScriptSectionName() : "") << "\", line " << Context->GetLineNumber() << ", in " << Function->GetDeclaration();
					else
						Stream << "source { native code }, in " << Function->GetDeclaration();
					Stream << " 0x" << Function;
				}
				else
					Stream << "source { native code } [nullptr]";
				Stream << "\n";
			}
			Output(Stream.str());
#endif
		}
		void DebuggerContext::ListAddons()
		{
			for (auto& Name : VM->GetExposedAddons())
				Output("  " + Name + "\n");
		}
		void DebuggerContext::ListStackRegisters(ImmediateContext* Context, uint32_t Level)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptContext* Base = Context->GetContext();
			VI_ASSERT(Base != nullptr, "context should be set");

			asUINT StackSize = Base->GetCallstackSize();
			if (Level >= StackSize)
				return Output("  there are only " + Core::ToString(StackSize) + " stack frames\n");

			Core::StringStream Stream;
			Stream << "  stack frame #" << Level << " values:\n";
			int PropertiesCount = Base->GetVarCount(Level);
			if (PropertiesCount > 0)
			{
				Core::String Indent = "    ";
				for (asUINT n = PropertiesCount; n != (asUINT)-1; n--)
				{
					int TypeId, Offset; bool Heap, Active = Base->IsVarInScope(n, Level);
					if (Base->GetVar(n, Level, nullptr, &TypeId, nullptr, &Heap, &Offset) < 0)
						continue;

					Stream << Indent << "#" << n << " [sp:" << Offset << ";hp:" << Heap << "] " << CharTrimEnd(Base->GetVarDeclaration(n, Level)) << ": ";
					if (Active)
						Stream << ToString(Indent, 3, Base->GetAddressOfVar(n), TypeId) << std::endl;
					else
						Stream << "<uninitialized>" << std::endl;
				}
			}

			bool HasBaseCallState = false;
			if (PropertiesCount <= 0)
				Stream << "  stack arguments are empty\n";

			Stream << "  stack frame #" << Level << " registers:\n";
			asIScriptFunction* CurrentFunction = nullptr;
			asDWORD StackFramePointer, ProgramPointer, StackPointer, StackIndex;
			if (Base->GetCallStateRegisters(Level, &StackFramePointer, &CurrentFunction, &ProgramPointer, &StackPointer, &StackIndex) >= 0)
			{
				void* ThisPointer = Base->GetThisPointer(Level);
				asITypeInfo* ThisType = VM->GetEngine()->GetTypeInfoById(Base->GetThisTypeId(Level));
				const char* SectionName = ""; int ColumnNumber = 0;
				int LineNumber = Base->GetLineNumber(Level, &ColumnNumber, &SectionName);
				Stream << "    [sfp] stack frame pointer: " << StackFramePointer << "\n";
				Stream << "    [pp] program pointer: " << ProgramPointer << "\n";
				Stream << "    [sp] stack pointer: " << StackPointer << "\n";
				Stream << "    [si] stack index: " << StackIndex << "\n";
				Stream << "    [sd] stack depth: " << StackSize - (Level + 1) << "\n";
				Stream << "    [tp] this pointer: 0x" << ThisPointer << "\n";
				Stream << "    [ttp] this type pointer: 0x" << ThisType << " (" << (ThisType ? ThisType->GetName() : "null") << ")" << "\n";
				Stream << "    [sn] section name: " << (SectionName ? SectionName : "?") << "\n";
				Stream << "    [ln] line number: " << LineNumber << "\n";
				Stream << "    [cn] column number: " << ColumnNumber << "\n";
				Stream << "    [ces] context execution state: ";
				switch (Base->GetState())
				{
					case asEXECUTION_FINISHED:
						Stream << "finished\n";
						break;
					case asEXECUTION_SUSPENDED:
						Stream << "suspended\n";
						break;
					case asEXECUTION_ABORTED:
						Stream << "aborted\n";
						break;
					case asEXECUTION_EXCEPTION:
						Stream << "exception\n";
						break;
					case asEXECUTION_PREPARED:
						Stream << "prepared\n";
						break;
					case asEXECUTION_UNINITIALIZED:
						Stream << "uninitialized\n";
						break;
					case asEXECUTION_ACTIVE:
						Stream << "active\n";
						break;
					case asEXECUTION_ERROR:
						Stream << "error\n";
						break;
					case asEXECUTION_DESERIALIZATION:
						Stream << "deserialization\n";
						break;
					default:
						Stream << "invalid\n";
						break;
				}
				HasBaseCallState = true;
			}

			void* ObjectRegister = nullptr; asITypeInfo* ObjectTypeRegister = nullptr;
			asIScriptFunction* CallingSystemFunction = nullptr, * InitialFunction = nullptr;
			asDWORD OriginalStackPointer, InitialArgumentsSize; asQWORD ValueRegister;
			if (Base->GetStateRegisters(Level, &CallingSystemFunction, &InitialFunction, &OriginalStackPointer, &InitialArgumentsSize, &ValueRegister, &ObjectRegister, &ObjectTypeRegister) >= 0)
			{
				Stream << "    [osp] original stack pointer: " << OriginalStackPointer << "\n";
				Stream << "    [vc] value content: " << ValueRegister << "\n";
				Stream << "    [op] object pointer: 0x" << ObjectRegister << "\n";
				Stream << "    [otp] object type pointer: 0x" << ObjectTypeRegister << " (" << (ObjectTypeRegister ? ObjectTypeRegister->GetName() : "null") << ")" << "\n";
				Stream << "    [ias] initial arguments size: " << InitialArgumentsSize << "\n";

				if (InitialFunction != nullptr)
					Stream << "    [if] initial function: " << InitialFunction->GetDeclaration(true, true, true) << "\n";

				if (CallingSystemFunction != nullptr)
					Stream << "    [csf] calling system function: " << CallingSystemFunction->GetDeclaration(true, true, true) << "\n";
			}
			else if (!HasBaseCallState)
				Stream << "  stack registers are empty\n";

			if (HasBaseCallState && CurrentFunction != nullptr)
			{
				asUINT Size = 0; size_t PreviewSize = 40;
				asDWORD* ByteCode = CurrentFunction->GetByteCode(&Size);
				Stream << "    [cf] current function: " << CurrentFunction->GetDeclaration(true, true, true) << "\n";
				if (ByteCode != nullptr && ProgramPointer < Size)
				{
					asUINT PreviewProgramPointerBegin = ProgramPointer - std::min<asUINT>((asUINT)PreviewSize, (asUINT)ProgramPointer);
					asUINT PreviewProgramPointerEnd = ProgramPointer + std::min<asUINT>((asUINT)PreviewSize, (asUINT)Size - (asUINT)ProgramPointer);
					Stream << "  stack frame #" << Level << " disassembly:\n";
					if (PreviewProgramPointerBegin < ProgramPointer)
						Stream << "    ... " << ProgramPointer - PreviewProgramPointerBegin << " bytes of assembly data skipped\n";
					
					while (PreviewProgramPointerBegin < PreviewProgramPointerEnd)
					{
						Stream << "  ";
						PreviewProgramPointerBegin = (asUINT)ByteCodeLabelToText(Stream, VM, ByteCode, PreviewProgramPointerBegin, PreviewProgramPointerBegin == ProgramPointer, false);
					}

					if (ProgramPointer < PreviewProgramPointerEnd)
						Stream << "    ... " << PreviewProgramPointerEnd - ProgramPointer << " more bytes of assembly data\n";
				}
				else
					Stream << "  stack frame #" << Level << " disassembly:\n    ... assembly data is empty\n";
			}
			else
				Stream << "  stack frame #" << Level << " disassembly:\n    ... assembly data is empty\n";

			Output(Stream.str());
#endif
		}
		void DebuggerContext::ListMemberProperties(ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptContext* Base = Context->GetContext();
			VI_ASSERT(Base != nullptr, "context should be set");

			void* Pointer = Base->GetThisPointer();
			if (Pointer != nullptr)
			{
				Core::String Indent = "  ";
				Core::StringStream Stream;
				Stream << Indent << "this = " << ToString(Indent, 3, Pointer, Base->GetThisTypeId()) << std::endl;
				Output(Stream.str());
			}
#endif
		}
		void DebuggerContext::ListLocalVariables(ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptContext* Base = Context->GetContext();
			VI_ASSERT(Base != nullptr, "context should be set");

			asIScriptFunction* Function = Base->GetFunction();
			if (!Function)
				return;

			Core::String Indent = "  ";
			Core::StringStream Stream;
			for (asUINT n = 0; n < Function->GetVarCount(); n++)
			{
				if (!Base->IsVarInScope(n))
					continue;

				int TypeId;
				Base->GetVar(n, 0, 0, &TypeId);
				Stream << Indent << CharTrimEnd(Function->GetVarDecl(n)) << ": " << ToString(Indent, 3, Base->GetAddressOfVar(n), TypeId) << std::endl;
			}
			Output(Stream.str());
#endif
		}
		void DebuggerContext::ListGlobalVariables(ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptContext* Base = Context->GetContext();
			VI_ASSERT(Base != nullptr, "context should be set");

			asIScriptFunction* Function = Base->GetFunction();
			if (!Function)
				return;

			asIScriptModule* Mod = Function->GetModule();
			if (!Mod)
				return;

			Core::String Indent = "  ";
			Core::StringStream Stream;
			for (asUINT n = 0; n < Mod->GetGlobalVarCount(); n++)
			{
				int TypeId = 0;
				Mod->GetGlobalVar(n, nullptr, nullptr, &TypeId);
				Stream << Indent << CharTrimEnd(Mod->GetGlobalVarDeclaration(n)) << ": " << ToString(Indent, 3, Mod->GetAddressOfGlobalVar(n), TypeId) << std::endl;
			}
			Output(Stream.str());
#endif
		}
		void DebuggerContext::ListSourceCode(ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			std::string_view File = "";
			Context->GetLineNumber(0, 0, &File);
			if (File.empty())
				return Output("source code is not available");
			
			auto Code = VM->GetScriptSection(File);
			if (!Code)
				return Output("source code is not available");

			auto Lines = Core::Stringify::Split(*Code, '\n');
			size_t MaxLineSize = Core::ToString(Lines.size()).size(), LineNumber = 0;
			for (auto& Line : Lines)
			{
				size_t LineSize = Core::ToString(++LineNumber).size();
				size_t Spaces = 1 + (MaxLineSize - LineSize);
				Output("  ");
				Output(Core::ToString(LineNumber));
				for (size_t i = 0; i < Spaces; i++)
					Output(" ");
				Output(Line + '\n');
			}
		}
		void DebuggerContext::ListInterfaces(ImmediateContext* Context)
		{
			for (auto& Interface : VM->DumpRegisteredInterfaces(Context))
			{
				Output("  listing generated <" + Interface.first + ">:\n");
				Core::Stringify::Replace(Interface.second, "\t", "  ");
				for (auto& Line : Core::Stringify::Split(Interface.second, '\n'))
					Output("    " + Line + "\n");
				Output("\n");
			}
		}
		void DebuggerContext::ListStatistics(ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptContext* Base = Context->GetContext();
			VI_ASSERT(Base != nullptr, "context should be set");

			asIScriptEngine* Engine = Base->GetEngine();
			asUINT GCCurrSize, GCTotalDestr, GCTotalDet, GCNewObjects, GCTotalNewDestr;
			Engine->GetGCStatistics(&GCCurrSize, &GCTotalDestr, &GCTotalDet, &GCNewObjects, &GCTotalNewDestr);

			Core::StringStream Stream;
			Stream << "  garbage collector" << std::endl;
			Stream << "    current size:          " << GCCurrSize << std::endl;
			Stream << "    total destroyed:       " << GCTotalDestr << std::endl;
			Stream << "    total detected:        " << GCTotalDet << std::endl;
			Stream << "    new objects:           " << GCNewObjects << std::endl;
			Stream << "    new objects destroyed: " << GCTotalNewDestr << std::endl;
			Output(Stream.str());
#endif
		}
		void DebuggerContext::PrintCallstack(ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			VI_ASSERT(Context->GetContext() != nullptr, "context should be set");

			Core::StringStream Stream;
			Stream << Core::ErrorHandling::GetStackTrace(1) << "\n";
			Output(Stream.str());
		}
		void DebuggerContext::AddFuncBreakPoint(const std::string_view& Function)
		{
			size_t B = Function.find_first_not_of(" \t"), E = Function.find_last_not_of(" \t");
			Core::String Actual = Core::String(Function.substr(B, E != Core::String::npos ? E - B + 1 : Core::String::npos));

			Core::StringStream Stream;
			Stream << "  adding deferred break point for function '" << Actual << "'" << std::endl;
			Output(Stream.str());

			BreakPoint Point(Actual, 0, true);
			BreakPoints.push_back(Point);
		}
		void DebuggerContext::AddFileBreakPoint(const std::string_view& File, int LineNumber)
		{
			size_t R = File.find_last_of("\\/");
			Core::String Actual;

			if (R != Core::String::npos)
				Actual = File.substr(R + 1);
			else
				Actual = File;

			size_t B = Actual.find_first_not_of(" \t");
			size_t E = Actual.find_last_not_of(" \t");
			Actual = Actual.substr(B, E != Core::String::npos ? E - B + 1 : Core::String::npos);

			Core::StringStream Stream;
			Stream << "  setting break point in file '" << Actual << "' at line " << LineNumber << std::endl;
			Output(Stream.str());

			BreakPoint Point(Actual, LineNumber, false);
			BreakPoints.push_back(Point);
		}
		void DebuggerContext::Output(const std::string_view& Data)
		{
			if (OnOutput)
				OnOutput(Data);
			else
				Core::Console::Get()->Write(Data);
		}
		void DebuggerContext::SetEngine(VirtualMachine* Engine)
		{
#ifdef VI_ANGELSCRIPT
			if (Engine != nullptr && Engine != VM)
			{
				if (VM != nullptr)
					Core::Memory::Release(VM->GetEngine());

				VM = Engine;
				VM->GetEngine()->AddRef();
			}
#endif
		}
		bool DebuggerContext::CheckBreakPoint(ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptContext* Base = Context->GetContext();
			VI_ASSERT(Base != nullptr, "context should be set");

			const char* Temp = 0;
			int Line = Base->GetLineNumber(0, 0, &Temp);

			Core::String File = Temp ? Temp : "";
			size_t R = File.find_last_of("\\/");
			if (R != Core::String::npos)
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
							Core::StringStream Stream;
							Stream << "  entering function '" << BreakPoints[n].Name << "', transforming it into break point" << std::endl;
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
								Core::StringStream Stream;
								Stream << "  moving break point " << n << " in file '" << File << "' to next line with code at line " << Number << std::endl;
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
					Core::StringStream Stream;
					Stream << "  reached break point " << n << " in file '" << File << "' at line " << Line << std::endl;
					Output(Stream.str());
					return true;
				}
			}
#endif
			return false;
		}
		bool DebuggerContext::Interrupt()
		{
			Core::UMutex<std::recursive_mutex> Unique(ThreadBarrier);
			if (Action != DebugAction::Continue && Action != DebugAction::StepInto && Action != DebugAction::StepOut)
				return false;

			Action = DebugAction::Interrupt;
			return true;
		}
		bool DebuggerContext::InterpretCommand(const std::string_view& Command, ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			VI_ASSERT(Context->GetContext() != nullptr, "context should be set");

			for (auto& Item : Core::Stringify::Split(Command, "&&"))
			{
				Core::Stringify::Trim(Item);
				Core::String Name = Item.substr(0, Item.find(' '));
				auto It = Commands.find(Name);
				if (It == Commands.end())
				{
					Output("  command <" + Name + "> is not a known operation\n");
					return false;
				}

				Core::String Data = (Name.size() == Item.size() ? Core::String() : Item.substr(Name.size()));
				Core::Vector<Core::String> Args;
				switch (It->second.Arguments)
				{
					case ArgsType::Array:
					{
						size_t Offset = 0;
						while (Offset < Data.size())
						{
							char V = Data[Offset];
							if (Core::Stringify::IsWhitespace(V))
							{
								size_t Start = Offset;
								while (++Start < Data.size() && Core::Stringify::IsWhitespace(Data[Start]));

								size_t End = Start;
								while (++End < Data.size() && !Core::Stringify::IsWhitespace(Data[End]) && Data[End] != '\"' && Data[End] != '\'');

								auto Value = Data.substr(Start, End - Start);
								Core::Stringify::Trim(Value);

								if (!Value.empty())
									Args.push_back(Value);
								Offset = End;
							}
							else if (V == '\"' || V == '\'')
								while (++Offset < Data.size() && Data[Offset] != V);
							else
								++Offset;
						}
						break;
					}
					case ArgsType::Expression:
					{
						Core::Stringify::Trim(Data);
						Args.emplace_back(Data);
						break;
					}
					case ArgsType::NoArgs:
					default:
						if (Data.empty())
							break;

						Output("  command <" + Name + "> requires no arguments\n");
						return false;
				}

				if (It->second.Callback(Context, Args))
					return true;
			}

			return false;
		}
		bool DebuggerContext::IsError()
		{
			return InputError;
		}
		bool DebuggerContext::IsAttached()
		{
			return Attachable;
		}
		DebuggerContext::DebugAction DebuggerContext::GetState()
		{
			return Action;
		}
		size_t DebuggerContext::ByteCodeLabelToText(Core::StringStream& Stream, VirtualMachine* VM, void* Program, size_t ProgramPointer, bool Selection, bool Uppercase)
		{
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* Engine = VM ? VM->GetEngine() : nullptr;
			asDWORD* ByteCode = ((asDWORD*)Program) + ProgramPointer;
			asBYTE* BaseCode = (asBYTE*)ByteCode;
			ByteCodeLabel Label = VirtualMachine::GetByteCodeInfo((uint8_t)*BaseCode);
			auto PrintArgument = [&Stream](asBYTE* Offset, uint8_t Size, bool Last)
			{
				switch (Size)
				{
					case sizeof(asBYTE):
						Stream << " %spl:" << *(asBYTE*)Offset;
						if (!Last)
							Stream << ",";
						break;
					case sizeof(asWORD):
						Stream << " %sp:" << *(asWORD*)Offset;
						if (!Last)
							Stream << ",";
						break;
					case sizeof(asDWORD):
						Stream << " %esp:" << *(asDWORD*)Offset;
						if (!Last)
							Stream << ",";
						break;
					case sizeof(asQWORD):
						Stream << " %rdx:" << *(asQWORD*)Offset;
						if (!Last)
							Stream << ",";
						break;
					default:
						break;
				}
			};

			Stream << (Selection ? "> 0x" : "  0x") << (void*)(uintptr_t)ProgramPointer << ": ";
			if (Uppercase)
			{
				Core::String Name = Core::String(Label.Name);
				Stream << Core::Stringify::ToUpper(Name);
			}
			else
				Stream << Label.Name;
			if (!VM)
				goto DefaultPrint;

			switch (*BaseCode)
			{
				case asBC_CALL:
				case asBC_CALLSYS:
				case asBC_CALLBND:
				case asBC_CALLINTF:
				case asBC_Thiscall1:
				{
					auto* Function = Engine->GetFunctionById(asBC_INTARG(ByteCode));
					if (!Function)
						goto DefaultPrint;

					auto* Declaration = Function->GetDeclaration();
					if (!Declaration)
						goto DefaultPrint;

					Stream << " %edi:[" << Declaration << "]";
					break;
				}
				default:
				DefaultPrint:
					PrintArgument(BaseCode + Label.OffsetOfArg0, Label.SizeOfArg0, !Label.SizeOfArg1);
					PrintArgument(BaseCode + Label.OffsetOfArg1, Label.SizeOfArg1, !Label.SizeOfArg2);
					PrintArgument(BaseCode + Label.OffsetOfArg2, Label.SizeOfArg2, true);
					break;
			}

			Stream << "\n";
			return ProgramPointer + Label.Size;
#else
			return ProgramPointer + 1;
#endif
		}
		Core::String DebuggerContext::ToString(int Depth, void* Value, unsigned int TypeId)
		{
			Core::String Indent;
			return ToString(Indent, Depth, Value, TypeId);
		}
		Core::String DebuggerContext::ToString(Core::String& Indent, int Depth, void* Value, unsigned int TypeId)
		{
#ifdef VI_ANGELSCRIPT
			if (Value == 0 || !VM)
				return "null";

			asIScriptEngine* Base = VM->GetEngine();
			if (!Base)
				return "null";

			Core::StringStream Stream;
			if (TypeId == asTYPEID_VOID)
				return "void";
			else if (TypeId == asTYPEID_BOOL)
				return *(bool*)Value ? "true" : "false";
			else if (TypeId == asTYPEID_INT8)
				Stream << (int)*(signed char*)Value;
			else if (TypeId == asTYPEID_INT16)
				Stream << (int)*(signed short*)Value;
			else if (TypeId == asTYPEID_INT32)
				Stream << *(signed int*)Value;
			else if (TypeId == asTYPEID_INT64)
				Stream << *(asINT64*)Value;
			else if (TypeId == asTYPEID_UINT8)
				Stream << (unsigned int)*(unsigned char*)Value;
			else if (TypeId == asTYPEID_UINT16)
				Stream << (unsigned int)*(unsigned short*)Value;
			else if (TypeId == asTYPEID_UINT32)
				Stream << *(unsigned int*)Value;
			else if (TypeId == asTYPEID_UINT64)
				Stream << *(asQWORD*)Value;
			else if (TypeId == asTYPEID_FLOAT)
				Stream << *(float*)Value;
			else if (TypeId == asTYPEID_DOUBLE)
				Stream << *(double*)Value;
			else if ((TypeId & asTYPEID_MASK_OBJECT) == 0)
			{
				asITypeInfo* T = Base->GetTypeInfoById(TypeId);
				Stream << *(asUINT*)Value;

				for (int n = T->GetEnumValueCount(); n-- > 0;)
				{
					int EnumVal;
					const char* EnumName = T->GetEnumValueByIndex(n, &EnumVal);
					if (EnumVal == *(int*)Value)
					{
						Stream << " (" << EnumName <<  ")";
						break;
					}
				}
			}
			else if (TypeId & asTYPEID_SCRIPTOBJECT)
			{
				if (TypeId & asTYPEID_OBJHANDLE)
					Value = *(void**)Value;

				if (!Value || !Depth)
				{
					if (Value != nullptr)
						Stream << "0x" << Value;
					else
						Stream << "null";
					goto Finalize;
				}

				asIScriptObject* Object = (asIScriptObject*)Value;
				asITypeInfo* Type = Object->GetObjectType();
				size_t Size = Object->GetPropertyCount();
				Stream << "0x" << Value << " (" << Type->GetName() << ")";
				if (!Size)
				{
					Stream << " { }";
					goto Finalize;
				}

				Stream << "\n";
				Indent.append("  ");
				for (asUINT n = 0; n < Size; n++)
				{
					Stream << Indent << CharTrimEnd(Type->GetPropertyDeclaration(n)) << ": " << ToString(Indent, Depth - 1, Object->GetAddressOfProperty(n), Object->GetPropertyTypeId(n));
					if (n + 1 < Size)
						Stream << "\n";
				}
				Indent.erase(Indent.end() - 2, Indent.end());
			}
			else
			{
				if (TypeId & asTYPEID_OBJHANDLE)
					Value = *(void**)Value;

				asITypeInfo* Type = Base->GetTypeInfoById(TypeId);
				if (!Value || !Depth)
				{
					if (Value != nullptr)
						Stream << "0x" << Value << " (" << Type->GetName() << ")";
					else
						Stream << "null (" << Type->GetName() << ")";
					goto Finalize;
				}

				auto It1 = FastToStringCallbacks.find(Type);
				if (It1 == FastToStringCallbacks.end())
				{
					if (Type->GetFlags() & asOBJ_TEMPLATE)
						It1 = FastToStringCallbacks.find(Base->GetTypeInfoByName(Type->GetName()));
				}

				if (It1 != FastToStringCallbacks.end())
				{
					Indent.append("  ");
					Stream << It1->second(Indent, Depth, Value);
					Indent.erase(Indent.end() - 2, Indent.end());
					goto Finalize;
				}

				auto It2 = SlowToStringCallbacks.find(Type->GetName());
				if (It2 != SlowToStringCallbacks.end())
				{
					Indent.append("  ");
					Stream << It2->second(Indent, Depth, Value, TypeId);
					Indent.erase(Indent.end() - 2, Indent.end());
					goto Finalize;
				}

				size_t Size = Type->GetPropertyCount();
				Stream << "0x" << Value << " (" << Type->GetName() << ")";
				if (!Size)
				{
					Stream << " { }";
					goto Finalize;
				}

				Stream << "\n";
				Indent.append("  ");
				for (asUINT n = 0; n < Type->GetPropertyCount(); n++)
				{
					int PropTypeId, PropOffset;
					if (Type->GetProperty(n, nullptr, &PropTypeId, nullptr, nullptr, &PropOffset, nullptr, nullptr, nullptr, nullptr) != 0)
						continue;

					Stream << Indent << CharTrimEnd(Type->GetPropertyDeclaration(n)) << ": " << ToString(Indent, Depth - 1, (char*)Value + PropOffset, PropTypeId);
					if (n + 1 < Size)
						Stream << "\n";
				}
				Indent.erase(Indent.end() - 2, Indent.end());
			}

		Finalize:
			return Stream.str();
#else
			return "null";
#endif
		}
		void DebuggerContext::ClearThread(ImmediateContext* Context)
		{
			Core::UMutex<std::recursive_mutex> Unique(Mutex);
			for (auto It = Threads.begin(); It != Threads.end(); It++)
			{
				if (It->Context == Context)
				{
					Threads.erase(It);
					break;
				}
			}
		}
		ExpectsVM<void> DebuggerContext::ExecuteExpression(ImmediateContext* Context, const std::string_view& Code, const std::string_view& Args, ArgsCallback&& OnArgs)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			Core::String Indent = "  ";
			Core::String Eval = "any@ __vfdbgfunc(" + Core::String(Args) + "){return any(" + Core::String(Code.empty() || Code.back() != ';' ? Code : Code.substr(0, Code.size() - 1)) + ");}";
			asIScriptModule* Module = Context->GetFunction().GetModule().GetModule();
			asIScriptFunction* Function = nullptr;
			Bindings::Any* Data = nullptr;
			VM->DetachDebuggerFromContext(Context->GetContext());
			VM->ImportSystemAddon(ADDON_ANY);

			int Result = 0;
			while ((Result = Module->CompileFunction("__vfdbgfunc", Eval.c_str(), -1, asCOMP_ADD_TO_MODULE, &Function)) == asBUILD_IN_PROGRESS)
				std::this_thread::sleep_for(std::chrono::microseconds(COMPILER_BLOCKED_WAIT_US));

			if (Result < 0)
			{
				VM->AttachDebuggerToContext(Context->GetContext());
				Core::Memory::Release(Function);
				return VirtualException((VirtualError)Result);
			}

			Context->DisableSuspends();
			Context->PushState();
			auto Status1 = Context->Prepare(Function);
			if (!Status1)
			{
				Context->PopState();
				Context->EnableSuspends();
				VM->AttachDebuggerToContext(Context->GetContext());
				Core::Memory::Release(Function);
				return Status1;
			}

			if (OnArgs)
				OnArgs(Context);

			auto Status2 = Context->ExecuteNext();
			if (Status2 && *Status2 == Execution::Finished)
			{
				Data = Context->GetReturnObject<Bindings::Any>();
				Output(Indent + ToString(Indent, 3, Data, VM->GetTypeInfoByName("any").GetTypeId()) + "\n");
			}

			Context->PopState();
			Context->EnableSuspends();
			VM->AttachDebuggerToContext(Context->GetContext());
			Core::Memory::Release(Function);
			if (!Status2)
				return Status2.Error();

			return Core::Expectation::Met;
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		DebuggerContext::ThreadData DebuggerContext::GetThread(ImmediateContext* Context)
		{
			Core::UMutex<std::recursive_mutex> Unique(Mutex);
			for (auto& Thread : Threads)
			{
				if (Thread.Context == Context)
				{
					Thread.Id = std::this_thread::get_id();
					return Thread;
				}
			}

			ThreadData Thread;
			Thread.Context = Context;
			Thread.Id = std::this_thread::get_id();
			Threads.push_back(Thread);
			return Thread;
		}
		VirtualMachine* DebuggerContext::GetEngine()
		{
			return VM;
		}

		ImmediateContext::ImmediateContext(asIScriptContext* Base) noexcept : Context(Base), VM(nullptr)
		{
			VI_ASSERT(Base != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			Context->SetUserData(this, ContextUD);
			VM = VirtualMachine::Get(Base->GetEngine());
#endif
		}
		ImmediateContext::~ImmediateContext() noexcept
		{
			if (Executor.Future.IsPending())
				Executor.Future.Set(VirtualException(VirtualError::CONTEXT_NOT_PREPARED));
#ifdef VI_ANGELSCRIPT
			VM->GetEngine()->ReturnContext(Context);
#endif
		}
		ExpectsPromiseVM<Execution> ImmediateContext::ExecuteCall(const Function& Function, ArgsCallback&& OnArgs)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			VI_ASSERT(Function.IsValid(), "function should be set");
			VI_ASSERT(!Core::Costate::IsCoroutine(), "cannot safely execute in coroutine");
#ifdef VI_ANGELSCRIPT
			Core::UMutex<std::recursive_mutex> Unique(Exchange);
			if (!CanExecuteCall())
				return ExpectsPromiseVM<Execution>(VirtualException(VirtualError::CONTEXT_ACTIVE));

			int Result = Context->Prepare(Function.GetFunction());
			if (Result < 0)
				return ExpectsPromiseVM<Execution>(VirtualException((VirtualError)Result));

			if (OnArgs)
				OnArgs(this);

			Executor.Future = ExpectsPromiseVM<Execution>();
			Resume();
			return Executor.Future;
#else
			return ExpectsPromiseVM<Execution>(VirtualException(VirtualError::NOT_SUPPORTED));
#endif
		}
		ExpectsVM<Execution> ImmediateContext::ExecuteInlineCall(const Function& Function, ArgsCallback&& OnArgs)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			VI_ASSERT(Function.IsValid(), "function should be set");
			VI_ASSERT(!Core::Costate::IsCoroutine(), "cannot safely execute in coroutine");
#ifdef VI_ANGELSCRIPT
			Core::UMutex<std::recursive_mutex> Unique(Exchange);
			if (!CanExecuteCall())
				return VirtualException(VirtualError::CONTEXT_ACTIVE);

			DisableSuspends();
			int Result = Context->Prepare(Function.GetFunction());
			if (Result < 0)
			{
				EnableSuspends();
				return VirtualException((VirtualError)Result);
			}
			else if (OnArgs)
				OnArgs(this);

			auto Status = ExecuteNext();
			EnableSuspends();
			return Status;
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<Execution> ImmediateContext::ExecuteSubcall(const Function& Function, ArgsCallback&& OnArgs, ArgsCallback&& OnReturn)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			VI_ASSERT(Function.IsValid(), "function should be set");
			VI_ASSERT(!Core::Costate::IsCoroutine(), "cannot safely execute in coroutine");
#ifdef VI_ANGELSCRIPT
			Core::UMutex<std::recursive_mutex> Unique(Exchange);
			if (!CanExecuteSubcall())
			{
				VI_ASSERT(false, "context should be active");
				return VirtualException(VirtualError::CONTEXT_NOT_PREPARED);
			}

			DisableSuspends();
			Context->PushState();
			int Result = Context->Prepare(Function.GetFunction());
			if (Result < 0)
			{
				Context->PopState();
				EnableSuspends();
				return VirtualException((VirtualError)Result);
			}
			else if (OnArgs)
				OnArgs(this);
			
			auto Status = ExecuteNext();
			if (OnReturn)
				OnReturn(this);
			Context->PopState();
			EnableSuspends();
			return Status;
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<Execution> ImmediateContext::ExecuteNext()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			int R = Context->Execute();
			if (Callbacks.StopExecutions.empty())
				return FunctionFactory::ToReturn<Execution>(R, (Execution)R);

			Core::UMutex<std::recursive_mutex> Unique(Exchange);
			Core::Vector<StopExecutionCallback> Queue;
			Queue.swap(Callbacks.StopExecutions);
			Unique.Negate();
			for (auto& Callback : Queue)
				Callback();

			return FunctionFactory::ToReturn<Execution>(R, (Execution)R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<Execution> ImmediateContext::Resume()
		{
#ifdef VI_ANGELSCRIPT
			auto Status = ExecuteNext();
			if (Status && *Status == Execution::Suspended)
				return Status;

			Core::UMutex<std::recursive_mutex> Unique(Exchange);
			if (!Executor.Future.IsPending())
				return Status;

			if (Status)
				Executor.Future.Set(*Status);
			else
				Executor.Future.Set(Status.Error());
			return Status;
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsPromiseVM<Execution> ImmediateContext::ResolveCallback(FunctionDelegate&& Delegate, ArgsCallback&& OnArgs, ArgsCallback&& OnReturn)
		{
			VI_ASSERT(Delegate.IsValid(), "callback should be valid");
#ifdef VI_ANGELSCRIPT
			if (Callbacks.CallbackResolver)
			{
				Callbacks.CallbackResolver(this, std::move(Delegate), std::move(OnArgs), std::move(OnReturn));
				return ExpectsPromiseVM<Execution>(Execution::Active);
			}

			Core::UMutex<std::recursive_mutex> Unique(Exchange);
			if (CanExecuteCall())
			{
				return ExecuteCall(Delegate.Callable(), std::move(OnArgs)).Then<ExpectsVM<Execution>>([this, OnReturn = std::move(OnReturn)](ExpectsVM<Execution>&& Result) mutable
				{
					if (OnReturn)
						OnReturn(this);
					return Result;
				});
			}

			ImmediateContext* Target = VM->RequestContext();
			return Target->ExecuteCall(Delegate.Callable(), std::move(OnArgs)).Then<ExpectsVM<Execution>>([Target, OnReturn = std::move(OnReturn)](ExpectsVM<Execution>&& Result) mutable
			{
				if (OnReturn)
					OnReturn(Target);

				Target->VM->ReturnContext(Target);
				return Result;
			});
#else
			return ExpectsPromiseVM<Execution>(VirtualException(VirtualError::NOT_SUPPORTED));
#endif
		}
		ExpectsVM<Execution> ImmediateContext::ResolveNotification()
		{
#ifdef VI_ANGELSCRIPT
			if (Callbacks.NotificationResolver)
			{
				Callbacks.NotificationResolver(this);
				return Execution::Active;
			}

			ImmediateContext* Context = this;
			Core::Codefer([Context]()
			{
				if (Context->IsSuspended())
					Context->Resume();
			});
			return Execution::Active;
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::Prepare(const Function& Function)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->Prepare(Function.GetFunction()));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::Unprepare()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->Unprepare());
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::Abort()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->Abort());
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::Suspend()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			if (!IsSuspendable())
			{
				Bindings::Exception::ThrowAt(this, Bindings::Exception::Pointer("async_error", "yield is not allowed in this function call"));
				return VirtualException(VirtualError::CONTEXT_NOT_PREPARED);
			}

			return FunctionFactory::ToReturn(Context->Suspend());
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::PushState()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->PushState());
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::PopState()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->PopState());
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::SetObject(void* Object)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetObject(Object));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::SetArg8(size_t Arg, unsigned char Value)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetArgByte((asUINT)Arg, Value));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::SetArg16(size_t Arg, unsigned short Value)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetArgWord((asUINT)Arg, Value));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::SetArg32(size_t Arg, int Value)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetArgDWord((asUINT)Arg, Value));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::SetArg64(size_t Arg, int64_t Value)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetArgQWord((asUINT)Arg, Value));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::SetArgFloat(size_t Arg, float Value)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetArgFloat((asUINT)Arg, Value));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::SetArgDouble(size_t Arg, double Value)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetArgDouble((asUINT)Arg, Value));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::SetArgAddress(size_t Arg, void* Address)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetArgAddress((asUINT)Arg, Address));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::SetArgObject(size_t Arg, void* Object)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetArgObject((asUINT)Arg, Object));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::SetArgAny(size_t Arg, void* Ptr, int TypeId)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetArgVarType((asUINT)Arg, Ptr, TypeId));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::GetReturnableByType(void* Return, asITypeInfo* ReturnTypeInfo)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			VI_ASSERT(Return != nullptr, "return value should be set");
			VI_ASSERT(ReturnTypeInfo != nullptr, "return type info should be set");
#ifdef VI_ANGELSCRIPT
			VI_ASSERT(ReturnTypeInfo->GetTypeId() != (int)TypeId::VOIDF, "return value type should not be void");
			void* Address = Context->GetAddressOfReturnValue();
			if (!Address)
				return VirtualException(VirtualError::INVALID_OBJECT);

			int TypeId = ReturnTypeInfo->GetTypeId();
			asIScriptEngine* Engine = VM->GetEngine();
			if (TypeId & asTYPEID_OBJHANDLE)
			{
				if (*reinterpret_cast<void**>(Return) == nullptr)
				{
					*reinterpret_cast<void**>(Return) = *reinterpret_cast<void**>(Address);
					Engine->AddRefScriptObject(*reinterpret_cast<void**>(Return), ReturnTypeInfo);
					return Core::Expectation::Met;
				}
			}
			else if (TypeId & asTYPEID_MASK_OBJECT)
				return FunctionFactory::ToReturn(Engine->AssignScriptObject(Return, Address, ReturnTypeInfo));

			size_t Size = Engine->GetSizeOfPrimitiveType(ReturnTypeInfo->GetTypeId());
			if (!Size)
				return VirtualException(VirtualError::INVALID_TYPE);

			memcpy(Return, Address, Size);
			return Core::Expectation::Met;
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::GetReturnableByDecl(void* Return, const std::string_view& ReturnTypeDecl)
		{
			VI_ASSERT(Core::Stringify::IsCString(ReturnTypeDecl), "rtdecl should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* Engine = VM->GetEngine();
			return GetReturnableByType(Return, Engine->GetTypeInfoByDecl(ReturnTypeDecl.data()));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::GetReturnableById(void* Return, int ReturnTypeId)
		{
			VI_ASSERT(ReturnTypeId != (int)TypeId::VOIDF, "return value type should not be void");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* Engine = VM->GetEngine();
			return GetReturnableByType(Return, Engine->GetTypeInfoById(ReturnTypeId));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::SetException(const std::string_view& Info, bool AllowCatch)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			VI_ASSERT(Core::Stringify::IsCString(Info), "info should be set");
#ifdef VI_ANGELSCRIPT
			if (!IsSuspended() && !Executor.DeferredExceptions)
				return FunctionFactory::ToReturn(Context->SetException(Info.data(), AllowCatch));

			Executor.DeferredException.Info = Info;
			Executor.DeferredException.AllowCatch = AllowCatch;
			return Core::Expectation::Met;
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::SetExceptionCallback(void(*Callback)(asIScriptContext* Context, void* Object), void* Object)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetExceptionCallback(asFUNCTION(Callback), Object, asCALL_CDECL));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::SetLineCallback(void(*Callback)(asIScriptContext* Context, void* Object), void* Object)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			VI_ASSERT(Callback != nullptr, "callback should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetLineCallback(asFUNCTION(Callback), Object, asCALL_CDECL));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::StartDeserialization()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->StartDeserialization());
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::FinishDeserialization()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->FinishDeserialization());
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::PushFunction(const Function& Func, void* Object)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->PushFunction(Func.GetFunction(), Object));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::GetStateRegisters(size_t StackLevel, Function* CallingSystemFunction, Function* InitialFunction, uint32_t* OrigStackPointer, uint32_t* ArgumentsSize, uint64_t* ValueRegister, void** ObjectRegister, TypeInfo* ObjectTypeRegister)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asITypeInfo* ObjectTypeRegister1 = nullptr;
			asIScriptFunction* CallingSystemFunction1 = nullptr, * InitialFunction1 = nullptr;
			asDWORD OrigStackPointer1 = 0, ArgumentsSize1 = 0; asQWORD ValueRegister1 = 0;
			int R = Context->GetStateRegisters((asUINT)StackLevel, &CallingSystemFunction1, &InitialFunction1, &OrigStackPointer1, &ArgumentsSize1, &ValueRegister1, ObjectRegister, &ObjectTypeRegister1);
			if (CallingSystemFunction != nullptr) *CallingSystemFunction = CallingSystemFunction1;
			if (InitialFunction != nullptr) *InitialFunction = InitialFunction1;
			if (OrigStackPointer != nullptr) *OrigStackPointer = (uint32_t)OrigStackPointer1;
			if (ArgumentsSize != nullptr) *ArgumentsSize = (uint32_t)ArgumentsSize1;
			if (ValueRegister != nullptr) *ValueRegister = (uint64_t)ValueRegister1;
			if (ObjectTypeRegister != nullptr) *ObjectTypeRegister = ObjectTypeRegister1;
			return FunctionFactory::ToReturn(R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::GetCallStateRegisters(size_t StackLevel, uint32_t* StackFramePointer, Function* CurrentFunction, uint32_t* ProgramPointer, uint32_t* StackPointer, uint32_t* StackIndex)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptFunction* CurrentFunction1 = nullptr;
			asDWORD StackFramePointer1 = 0, ProgramPointer1 = 0, StackPointer1 = 0, StackIndex1 = 0;
			int R = Context->GetCallStateRegisters((asUINT)StackLevel, &StackFramePointer1, &CurrentFunction1, &ProgramPointer1, &StackPointer1, &StackIndex1);
			if (CurrentFunction != nullptr) *CurrentFunction = CurrentFunction1;
			if (StackFramePointer != nullptr) *StackFramePointer = (uint32_t)StackFramePointer1;
			if (ProgramPointer != nullptr) *ProgramPointer = (uint32_t)ProgramPointer1;
			if (StackPointer != nullptr) *StackPointer = (uint32_t)StackPointer1;
			if (StackIndex != nullptr) *StackIndex = (uint32_t)StackIndex1;
			return FunctionFactory::ToReturn(R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::SetStateRegisters(size_t StackLevel, Function CallingSystemFunction, const Function& InitialFunction, uint32_t OrigStackPointer, uint32_t ArgumentsSize, uint64_t ValueRegister, void* ObjectRegister, const TypeInfo& ObjectTypeRegister)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetStateRegisters((asUINT)StackLevel, CallingSystemFunction.GetFunction(), InitialFunction.GetFunction(), (asDWORD)OrigStackPointer, (asDWORD)ArgumentsSize, (asQWORD)ValueRegister, ObjectRegister, ObjectTypeRegister.GetTypeInfo()));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::SetCallStateRegisters(size_t StackLevel, uint32_t StackFramePointer, const Function& CurrentFunction, uint32_t ProgramPointer, uint32_t StackPointer, uint32_t StackIndex)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetCallStateRegisters((asUINT)StackLevel, (asDWORD)StackFramePointer, CurrentFunction.GetFunction(), (asDWORD)ProgramPointer, (asDWORD)StackPointer, (asDWORD)StackIndex));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<size_t> ImmediateContext::GetArgsOnStackCount(size_t StackLevel)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			int Result = Context->GetArgsOnStackCount((asUINT)StackLevel);
			return FunctionFactory::ToReturn<size_t>(Result, (size_t)Result);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<size_t> ImmediateContext::GetPropertiesCount(size_t StackLevel)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			int Result = Context->GetVarCount((asUINT)StackLevel);
			return FunctionFactory::ToReturn<size_t>(Result, (size_t)Result);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> ImmediateContext::GetProperty(size_t Index, size_t StackLevel, std::string_view* Name, int* TypeId, Modifiers* TypeModifiers, bool* IsVarOnHeap, int* StackOffset)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			const char* asName = "";
			asETypeModifiers TypeModifiers1 = asTM_NONE;
			int R = Context->GetVar((asUINT)Index, (asUINT)StackLevel, &asName, TypeId, &TypeModifiers1, IsVarOnHeap, StackOffset);
			if (TypeModifiers != nullptr)
				*TypeModifiers = (Modifiers)TypeModifiers1;
			if (Name != nullptr)
				*Name = asName;
			return FunctionFactory::ToReturn(R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		Function ImmediateContext::GetFunction(size_t StackLevel)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetFunction((asUINT)StackLevel);
#else
			return Function(nullptr);
#endif
		}
		int ImmediateContext::GetLineNumber(size_t StackLevel, int* Column, std::string_view* SectionName)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			const char* asSectionName = "";
			int R = Context->GetLineNumber((asUINT)StackLevel, Column, &asSectionName);
			if (SectionName != nullptr)
				*SectionName = asSectionName;
			return R;
#else
			return 0;
#endif
		}
		void ImmediateContext::SetNotificationResolverCallback(std::function<void(ImmediateContext*)>&& Callback)
		{
			Callbacks.NotificationResolver = std::move(Callback);
		}
		void ImmediateContext::SetCallbackResolverCallback(std::function<void(ImmediateContext*, FunctionDelegate&&, ArgsCallback&&, ArgsCallback&&)>&& Callback)
		{
			Callbacks.CallbackResolver = std::move(Callback);
		}
		void ImmediateContext::SetExceptionCallback(std::function<void(ImmediateContext*)>&& Callback)
		{
			Callbacks.Exception = std::move(Callback);
		}
		void ImmediateContext::SetLineCallback(std::function<void(ImmediateContext*)>&& Callback)
		{
			Callbacks.Line = std::move(Callback);
			SetLineCallback(&VirtualMachine::LineHandler, this);
		}
		void ImmediateContext::AppendStopExecutionCallback(StopExecutionCallback&& Callback)
		{
			VI_ASSERT(Callback != nullptr, "callback should be set");
			Core::UMutex<std::recursive_mutex> Unique(Exchange);
			Callbacks.StopExecutions.push_back(std::move(Callback));
		}
		void ImmediateContext::Reset()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			Context->Unprepare();
			Callbacks = Events();
			Executor = Frame();
			InvalidateStrings();
#endif
		}
		Core::String& ImmediateContext::CopyString(Core::String& Value)
		{
			auto& Copy = Strings.emplace_front(Value);
			return Copy;
		}
		void ImmediateContext::InvalidateString(const std::string_view& Value)
		{
			auto It = Strings.begin();
			while (It != Strings.end())
			{
				if (It->data() == Value.data())
				{
					Strings.erase(It);
					break;
				}
				++It;
			}
		}
		void ImmediateContext::InvalidateStrings()
		{
			Strings.clear();
		}
		void ImmediateContext::DisableSuspends()
		{
			++Executor.DenySuspends;
		}
		void ImmediateContext::EnableSuspends()
		{
			VI_ASSERT(Executor.DenySuspends > 0, "suspends are already enabled");
			--Executor.DenySuspends;
		}
		void ImmediateContext::EnableDeferredExceptions()
		{
			++Executor.DeferredExceptions;
		}
		void ImmediateContext::DisableDeferredExceptions()
		{
			VI_ASSERT(Executor.DeferredExceptions > 0, "deferred exceptions are already disabled");
			--Executor.DeferredExceptions;
		}
		bool ImmediateContext::IsNested(size_t* NestCount) const
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asUINT NestCount1 = 0;
			bool Value = Context->IsNested(&NestCount1);
			if (NestCount != nullptr) *NestCount = (size_t)NestCount1;
			return Value;
#else
			return false;
#endif
		}
		bool ImmediateContext::IsThrown() const
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			const char* Message = Context->GetExceptionString();
			return Message != nullptr && Message[0] != '\0';
#else
			return false;
#endif
		}
		bool ImmediateContext::IsPending()
		{
			Core::UMutex<std::recursive_mutex> Unique(Exchange);
			return Executor.Future.IsPending();
		}
		Execution ImmediateContext::GetState() const
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return (Execution)Context->GetState();
#else
			return Execution::Uninitialized;
#endif
		}
		void* ImmediateContext::GetAddressOfArg(size_t Arg)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetAddressOfArg((asUINT)Arg);
#else
			return nullptr;
#endif
		}
		unsigned char ImmediateContext::GetReturnByte()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetReturnByte();
#else
			return 0;
#endif
		}
		unsigned short ImmediateContext::GetReturnWord()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetReturnWord();
#else
			return 0;
#endif
		}
		size_t ImmediateContext::GetReturnDWord()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetReturnDWord();
#else
			return 0;
#endif
		}
		uint64_t ImmediateContext::GetReturnQWord()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetReturnQWord();
#else
			return 0;
#endif
		}
		float ImmediateContext::GetReturnFloat()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetReturnFloat();
#else
			return 0.0f;
#endif
		}
		double ImmediateContext::GetReturnDouble()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetReturnDouble();
#else
			return 0.0;
#endif
		}
		void* ImmediateContext::GetReturnAddress()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetReturnAddress();
#else
			return nullptr;
#endif
		}
		void* ImmediateContext::GetReturnObjectAddress()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetReturnObject();
#else
			return nullptr;
#endif
		}
		void* ImmediateContext::GetAddressOfReturnValue()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetAddressOfReturnValue();
#else
			return nullptr;
#endif
		}
		int ImmediateContext::GetExceptionLineNumber(int* Column, std::string_view* SectionName)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			const char* asSectionName = "";
			int R = Context->GetExceptionLineNumber(Column, &asSectionName);
			if (SectionName != nullptr)
				*SectionName = asSectionName;
			return R;
#else
			return 0;
#endif
		}
		Function ImmediateContext::GetExceptionFunction()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetExceptionFunction();
#else
			return Function(nullptr);
#endif
		}
		std::string_view ImmediateContext::GetExceptionString()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Context->GetExceptionString());
#else
			return "";
#endif
		}
		bool ImmediateContext::WillExceptionBeCaught()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->WillExceptionBeCaught();
#else
			return false;
#endif
		}
		bool ImmediateContext::HasDeferredException()
		{
			return !Executor.DeferredException.Info.empty();
		}
		bool ImmediateContext::RethrowDeferredException()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			if (!HasDeferredException())
				return false;
#ifdef VI_ANGELSCRIPT
			if (Context->SetException(Executor.DeferredException.Info.c_str(), Executor.DeferredException.AllowCatch) != 0)
				return false;

			Executor.DeferredException.Info.clear();
			return true;
#else
			return false;
#endif
		}
		void ImmediateContext::ClearLineCallback()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			Context->ClearLineCallback();
#endif
			Callbacks.Line = nullptr;
		}
		void ImmediateContext::ClearExceptionCallback()
		{
			Callbacks.Exception = nullptr;
		}
		size_t ImmediateContext::GetCallstackSize() const
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return (size_t)Context->GetCallstackSize();
#else
			return 0;
#endif
		}
		std::string_view ImmediateContext::GetPropertyName(size_t Index, size_t StackLevel)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			const char* Name = "";
			Context->GetVar((asUINT)Index, (asUINT)StackLevel, &Name);
			return OrEmpty(Name);
#else
			return "";
#endif
		}
		std::string_view ImmediateContext::GetPropertyDecl(size_t Index, size_t StackLevel, bool IncludeNamespace)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Context->GetVarDeclaration((asUINT)Index, (asUINT)StackLevel, IncludeNamespace));
#else
			return "";
#endif
		}
		int ImmediateContext::GetPropertyTypeId(size_t Index, size_t StackLevel)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			int TypeId = -1;
			Context->GetVar((asUINT)Index, (asUINT)StackLevel, nullptr, &TypeId);
			return TypeId;
#else
			return -1;
#endif
		}
		void* ImmediateContext::GetAddressOfProperty(size_t Index, size_t StackLevel, bool DontDereference, bool ReturnAddressOfUnitializedObjects)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetAddressOfVar((asUINT)Index, (asUINT)StackLevel, DontDereference, ReturnAddressOfUnitializedObjects);
#else
			return nullptr;
#endif
		}
		bool ImmediateContext::IsPropertyInScope(size_t Index, size_t StackLevel)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->IsVarInScope((asUINT)Index, (asUINT)StackLevel);
#else
			return false;
#endif
		}
		int ImmediateContext::GetThisTypeId(size_t StackLevel)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetThisTypeId((asUINT)StackLevel);
#else
			return -1;
#endif
		}
		void* ImmediateContext::GetThisPointer(size_t StackLevel)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetThisPointer((asUINT)StackLevel);
#else
			return nullptr;
#endif
		}
		Core::Option<Core::String> ImmediateContext::GetExceptionStackTrace()
		{
			Core::UMutex<std::recursive_mutex> Unique(Exchange);
			if (Executor.Stacktrace.empty())
				return Core::Optional::None;

			return Executor.Stacktrace;
		}
		Function ImmediateContext::GetSystemFunction()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetSystemFunction();
#else
			return Function(nullptr);
#endif
		}
		bool ImmediateContext::IsSuspended() const
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetState() == asEXECUTION_SUSPENDED;
#else
			return false;
#endif
		}
		bool ImmediateContext::IsSuspendable() const
		{
			return !IsSuspended() && !IsSyncLocked() && !Executor.DenySuspends;
		}
		bool ImmediateContext::IsSyncLocked() const
		{
#ifdef VI_BINDINGS
			VI_ASSERT(Context != nullptr, "context should be set");
			return Bindings::Mutex::IsAnyLocked((ImmediateContext*)this);
#else
			return false;
#endif
		}
		bool ImmediateContext::CanExecuteCall() const
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			auto State = Context->GetState();
			return State != asEXECUTION_SUSPENDED && State != asEXECUTION_ACTIVE && !Context->IsNested() && !Executor.Future.IsPending();
#else
			return false;
#endif
		}
		bool ImmediateContext::CanExecuteSubcall() const
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetState() == asEXECUTION_ACTIVE;
#else
			return false;
#endif
		}
		void* ImmediateContext::SetUserData(void* Data, size_t Type)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->SetUserData(Data, Type);
#else
			return nullptr;
#endif
		}
		void* ImmediateContext::GetUserData(size_t Type) const
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetUserData(Type);
#else
			return nullptr;
#endif
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
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			void* VM = Context->GetUserData(ContextUD);
			VI_ASSERT(VM != nullptr, "context should be created by virtual machine");
			return (ImmediateContext*)VM;
#else
			return nullptr;
#endif
		}
		ImmediateContext* ImmediateContext::Get()
		{
#ifdef VI_ANGELSCRIPT
			asIScriptContext* Context = asGetActiveContext();
			if (!Context)
				return nullptr;

			return Get(Context);
#else
			return nullptr;
#endif
		}
		int ImmediateContext::ContextUD = 151;

		VirtualMachine::VirtualMachine() noexcept : LastMajorGC(0), Scope(0), Debugger(nullptr), Engine(nullptr), SaveSources(false), SaveCache(true)
		{
			auto Directory = Core::OS::Directory::GetWorking();
			if (Directory)
				Include.Root = *Directory;

			Include.Exts.push_back(".as");
			Include.Exts.push_back(".so");
			Include.Exts.push_back(".dylib");
			Include.Exts.push_back(".dll");
#ifdef VI_ANGELSCRIPT
			Engine = asCreateScriptEngine();
			Engine->SetUserData(this, ManagerUD);
			Engine->SetContextCallbacks(RequestRawContext, ReturnRawContext, nullptr);
			Engine->SetMessageCallback(asFUNCTION(MessageLogger), this, asCALL_CDECL);
			Engine->SetEngineProperty(asEP_INIT_GLOBAL_VARS_AFTER_BUILD, 1);
			Engine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, 1);
			Engine->SetEngineProperty(asEP_DISALLOW_EMPTY_LIST_ELEMENTS, 1);
			Engine->SetEngineProperty(asEP_DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE, 0);
			Engine->SetEngineProperty(asEP_COMPILER_WARNINGS, 1);
			Engine->SetEngineProperty(asEP_INCLUDE_JIT_INSTRUCTIONS, 0);
#endif
			SetTsImports(true);
		}
		VirtualMachine::~VirtualMachine() noexcept
		{
			for (auto& Next : CLibraries)
			{
				if (Next.second.IsAddon)
					UninitializeAddon(Next.first, Next.second);
				Core::OS::Symbol::Unload(Next.second.Handle);
			}

			for (auto& Context : Threads)
				Core::Memory::Release(Context);
#ifdef VI_ANGELSCRIPT
			for (auto& Context : Stacks)
				Core::Memory::Release(Context);
#endif
			Core::Memory::Release(Debugger);
			CleanupThisThread();
#ifdef VI_ANGELSCRIPT
			if (Engine != nullptr)
			{
				Engine->ShutDownAndRelease();
				Engine = nullptr;
			}
#endif
			ClearCache();
		}
		ExpectsVM<TypeInterface> VirtualMachine::SetInterface(const std::string_view& Name)
		{
			VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register interface %i bytes", (int)Name.size());
#ifdef VI_ANGELSCRIPT
			int TypeId = Engine->RegisterInterface(Name.data());
			return FunctionFactory::ToReturn<TypeInterface>(TypeId, TypeInterface(this, Engine->GetTypeInfoById(TypeId), TypeId));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<TypeClass> VirtualMachine::SetStructAddress(const std::string_view& Name, size_t Size, uint64_t Flags)
		{
			VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register struct(%i) %i bytes sizeof %i", (int)Flags, (int)Name.size(), (int)Size);
#ifdef VI_ANGELSCRIPT
			int TypeId = Engine->RegisterObjectType(Name.data(), (asUINT)Size, (asDWORD)Flags);
			return FunctionFactory::ToReturn<TypeClass>(TypeId, TypeClass(this, Engine->GetTypeInfoById(TypeId), TypeId));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<TypeClass> VirtualMachine::SetPodAddress(const std::string_view& Name, size_t Size, uint64_t Flags)
		{
			return SetStructAddress(Name, Size, Flags);
		}
		ExpectsVM<RefClass> VirtualMachine::SetClassAddress(const std::string_view& Name, size_t Size, uint64_t Flags)
		{
			VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class(%i) %i bytes", (int)Flags, (int)Name.size());
#ifdef VI_ANGELSCRIPT
			int TypeId = Engine->RegisterObjectType(Name.data(), (asUINT)Size, (asDWORD)Flags);
			return FunctionFactory::ToReturn<RefClass>(TypeId, RefClass(this, Engine->GetTypeInfoById(TypeId), TypeId));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<TemplateClass> VirtualMachine::SetTemplateClassAddress(const std::string_view& Decl, const std::string_view& Name, size_t Size, uint64_t Flags)
		{
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
			VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class(%i) %i bytes", (int)Flags, (int)Decl.size());
#ifdef VI_ANGELSCRIPT
			int TypeId = Engine->RegisterObjectType(Decl.data(), (asUINT)Size, (asDWORD)Flags);
			return FunctionFactory::ToReturn<TemplateClass>(TypeId, TemplateClass(this, Name));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<Enumeration> VirtualMachine::SetEnum(const std::string_view& Name)
		{
			VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register enum %i bytes", (int)Name.size());
#ifdef VI_ANGELSCRIPT
			int TypeId = Engine->RegisterEnum(Name.data());
			return FunctionFactory::ToReturn<Enumeration>(TypeId, Enumeration(this, Engine->GetTypeInfoById(TypeId), TypeId));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::SetFunctionDef(const std::string_view& Decl)
		{
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register funcdef %i bytes", (int)Decl.size());
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RegisterFuncdef(Decl.data()));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::SetTypeDef(const std::string_view& Type, const std::string_view& Decl)
		{
			VI_ASSERT(Core::Stringify::IsCString(Type), "type should be set");
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register funcdef %i bytes", (int)Decl.size());
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RegisterTypedef(Type.data(), Decl.data()));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::SetFunctionAddress(const std::string_view& Decl, asSFuncPtr* Value, FunctionCall Type)
		{
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register funcaddr(%i) %i bytes at 0x%" PRIXPTR, (int)Type, (int)Decl.size(), (void*)Value);
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RegisterGlobalFunction(Decl.data(), *Value, (asECallConvTypes)Type));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::SetPropertyAddress(const std::string_view& Decl, void* Value)
		{
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register global %i bytes at 0x%" PRIXPTR, (int)Decl.size(), (void*)Value);
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RegisterGlobalProperty(Decl.data(), Value));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::SetStringFactoryAddress(const std::string_view& Type, asIStringFactory* Factory)
		{
			VI_ASSERT(Core::Stringify::IsCString(Type), "typename should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RegisterStringFactory(Type.data(), Factory));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::GetPropertyByIndex(size_t Index, PropertyInfo* Info) const
		{
#ifdef VI_ANGELSCRIPT
			const char* Name = "", * Namespace = "";
			const char* ConfigGroup = "";
			void* Pointer = nullptr;
			bool IsConst = false;
			asDWORD AccessMask = 0;
			int TypeId = 0;
			int Result = Engine->GetGlobalPropertyByIndex((asUINT)Index, &Name, &Namespace, &TypeId, &IsConst, &ConfigGroup, &Pointer, &AccessMask);
			if (Info != nullptr)
			{
				Info->Name = OrEmpty(Name);
				Info->Namespace = OrEmpty(Namespace);
				Info->TypeId = TypeId;
				Info->IsConst = IsConst;
				Info->ConfigGroup = OrEmpty(ConfigGroup);
				Info->Pointer = Pointer;
				Info->AccessMask = AccessMask;
			}
			return FunctionFactory::ToReturn(Result);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<size_t> VirtualMachine::GetPropertyIndexByName(const std::string_view& Name) const
		{
			VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
#ifdef VI_ANGELSCRIPT
			int R = Engine->GetGlobalPropertyIndexByName(Name.data());
			return FunctionFactory::ToReturn<size_t>(R, (size_t)R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<size_t> VirtualMachine::GetPropertyIndexByDecl(const std::string_view& Decl) const
		{
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			int R = Engine->GetGlobalPropertyIndexByDecl(Decl.data());
			return FunctionFactory::ToReturn<size_t>(R, (size_t)R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::SetLogCallback(void(*Callback)(const asSMessageInfo* Message, void* Object), void* Object)
		{
#ifdef VI_ANGELSCRIPT
			if (!Callback)
				return FunctionFactory::ToReturn(Engine->ClearMessageCallback());

			return FunctionFactory::ToReturn(Engine->SetMessageCallback(asFUNCTION(Callback), Object, asCALL_CDECL));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::Log(const std::string_view& Section, int Row, int Column, LogCategory Type, const std::string_view& Message)
		{
#ifdef VI_ANGELSCRIPT
			VI_ASSERT(Core::Stringify::IsCString(Section), "section should be set");
			VI_ASSERT(Core::Stringify::IsCString(Message), "message should be set");
			return FunctionFactory::ToReturn(Engine->WriteMessage(Section.data(), Row, Column, (asEMsgType)Type, Message.data()));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::AssignObject(void* DestObject, void* SrcObject, const TypeInfo& Type)
		{
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->AssignScriptObject(DestObject, SrcObject, Type.GetTypeInfo()));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::RefCastObject(void* Object, const TypeInfo& FromType, const TypeInfo& ToType, void** NewPtr, bool UseOnlyImplicitCast)
		{
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RefCastObject(Object, FromType.GetTypeInfo(), ToType.GetTypeInfo(), NewPtr, UseOnlyImplicitCast));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::WriteMessage(const std::string_view& Section, int Row, int Column, LogCategory Type, const std::string_view& Message)
		{
#ifdef VI_ANGELSCRIPT
			VI_ASSERT(Core::Stringify::IsCString(Section), "section should be set");
			VI_ASSERT(Core::Stringify::IsCString(Message), "message should be set");
			return FunctionFactory::ToReturn(Engine->WriteMessage(Section.data(), Row, Column, (asEMsgType)Type, Message.data()));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::GarbageCollect(GarbageCollector Flags, size_t NumIterations)
		{
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->GarbageCollect((asDWORD)Flags, (asUINT)NumIterations));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::PerformPeriodicGarbageCollection(uint64_t IntervalMs)
		{
			int64_t Time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			if (LastMajorGC + (int64_t)IntervalMs > Time)
				return Core::Expectation::Met;

			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			int64_t PrevTime = LastMajorGC.load() + (int64_t)IntervalMs;
			if (PrevTime > Time)
				return Core::Expectation::Met;

			LastMajorGC = Time;
			return PerformFullGarbageCollection();
		}
		ExpectsVM<void> VirtualMachine::PerformFullGarbageCollection()
		{
#ifdef VI_ANGELSCRIPT
			int R = Engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE | asGC_DETECT_GARBAGE, 1);
			return FunctionFactory::ToReturn(R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::NotifyOfNewObject(void* Object, const TypeInfo& Type)
		{
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->NotifyGarbageCollectorOfNewObject(Object, Type.GetTypeInfo()));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::GetObjectAddress(size_t Index, size_t* SequenceNumber, void** Object, TypeInfo* Type)
		{
#ifdef VI_ANGELSCRIPT
			asUINT asSequenceNumber;
			asITypeInfo* OutType = nullptr;
			int Result = Engine->GetObjectInGC((asUINT)Index, &asSequenceNumber, Object, &OutType);
			if (SequenceNumber != nullptr)
				*SequenceNumber = (size_t)asSequenceNumber;
			if (Type != nullptr)
				*Type = TypeInfo(OutType);
			return FunctionFactory::ToReturn(Result);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::AddScriptSection(asIScriptModule* Module, const std::string_view& Name, const std::string_view& Code, int LineOffset)
		{
			VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
			VI_ASSERT(Core::Stringify::IsCString(Code), "code should be set");
			VI_ASSERT(Module != nullptr, "module should be set");
#ifdef VI_ANGELSCRIPT
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			Sections[Core::String(Name)] = Core::String(Code);
			Unique.Negate();

			return FunctionFactory::ToReturn(Module->AddScriptSection(Name.data(), Code.data(), Code.size(), LineOffset));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::GetTypeNameScope(std::string_view* TypeName, std::string_view* Namespace) const
		{
			VI_ASSERT(TypeName != nullptr && Core::Stringify::IsCString(*TypeName), "typename should be set");
#ifdef VI_ANGELSCRIPT
			const char* Value = TypeName->data();
			size_t Size = strlen(Value);
			size_t Index = Size - 1;

			while (Index > 0 && Value[Index] != ':' && Value[Index - 1] != ':')
				Index--;

			if (Index < 1)
			{
				if (Namespace != nullptr)
					*Namespace = "";
				return VirtualException(VirtualError::ALREADY_REGISTERED);
			}

			if (Namespace != nullptr)
				*Namespace = std::string_view(Value, Index - 1);

			*TypeName = std::string_view(Value + Index + 1);
			return Core::Expectation::Met;
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::BeginGroup(const std::string_view& GroupName)
		{
			VI_ASSERT(Core::Stringify::IsCString(GroupName), "group name should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->BeginConfigGroup(GroupName.data()));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::EndGroup()
		{
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->EndConfigGroup());
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::RemoveGroup(const std::string_view& GroupName)
		{
			VI_ASSERT(Core::Stringify::IsCString(GroupName), "group name should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RemoveConfigGroup(GroupName.data()));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::BeginNamespace(const std::string_view& Namespace)
		{
			VI_ASSERT(Core::Stringify::IsCString(Namespace), "namespace should be set");
#ifdef VI_ANGELSCRIPT
			const char* Prev = Engine->GetDefaultNamespace();
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			if (Prev != nullptr)
				DefaultNamespace = Prev;
			else
				DefaultNamespace.clear();

			Unique.Negate();
			return FunctionFactory::ToReturn(Engine->SetDefaultNamespace(Namespace.data()));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::BeginNamespaceIsolated(const std::string_view& Namespace, size_t DefaultMask)
		{
			BeginAccessMask(DefaultMask);
			return BeginNamespace(Namespace);
		}
		ExpectsVM<void> VirtualMachine::EndNamespaceIsolated()
		{
			EndAccessMask();
			return EndNamespace();
		}
		ExpectsVM<void> VirtualMachine::EndNamespace()
		{
#ifdef VI_ANGELSCRIPT
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			return FunctionFactory::ToReturn(Engine->SetDefaultNamespace(DefaultNamespace.c_str()));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<void> VirtualMachine::SetProperty(Features Property, size_t Value)
		{
			VI_ASSERT(Engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->SetEngineProperty((asEEngineProp)Property, (asPWORD)Value));
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<size_t> VirtualMachine::GetSizeOfPrimitiveType(int TypeId) const
		{
			VI_ASSERT(Engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			int R = Engine->GetSizeOfPrimitiveType(TypeId);
			return FunctionFactory::ToReturn<size_t>(R, (size_t)R);
#else
			return VirtualException(VirtualError::NOT_SUPPORTED);
#endif
		}
		size_t VirtualMachine::GetFunctionsCount() const
		{
#ifdef VI_ANGELSCRIPT
			return Engine->GetGlobalFunctionCount();
#else
			return 0;
#endif
		}
		Function VirtualMachine::GetFunctionById(int Id) const
		{
#ifdef VI_ANGELSCRIPT
			return Engine->GetFunctionById(Id);
#else
			return Function(nullptr);
#endif
		}
		Function VirtualMachine::GetFunctionByIndex(size_t Index) const
		{
#ifdef VI_ANGELSCRIPT
			return Engine->GetGlobalFunctionByIndex((asUINT)Index);
#else
			return Function(nullptr);
#endif
		}
		Function VirtualMachine::GetFunctionByDecl(const std::string_view& Decl) const
		{
#ifdef VI_ANGELSCRIPT
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
			return Engine->GetGlobalFunctionByDecl(Decl.data());
#else
			return Function(nullptr);
#endif
		}
		size_t VirtualMachine::GetPropertiesCount() const
		{
#ifdef VI_ANGELSCRIPT
			return Engine->GetGlobalPropertyCount();
#else
			return 0;
#endif
		}
		size_t VirtualMachine::GetObjectsCount() const
		{
#ifdef VI_ANGELSCRIPT
			return Engine->GetObjectTypeCount();
#else
			return 0;
#endif
		}
		TypeInfo VirtualMachine::GetObjectByIndex(size_t Index) const
		{
#ifdef VI_ANGELSCRIPT
			return Engine->GetObjectTypeByIndex((asUINT)Index);
#else
			return TypeInfo(nullptr);
#endif
		}
		size_t VirtualMachine::GetEnumCount() const
		{
#ifdef VI_ANGELSCRIPT
			return Engine->GetEnumCount();
#else
			return 0;
#endif
		}
		TypeInfo VirtualMachine::GetEnumByIndex(size_t Index) const
		{
#ifdef VI_ANGELSCRIPT
			return Engine->GetEnumByIndex((asUINT)Index);
#else
			return TypeInfo(nullptr);
#endif
		}
		size_t VirtualMachine::GetFunctionDefsCount() const
		{
#ifdef VI_ANGELSCRIPT
			return Engine->GetFuncdefCount();
#else
			return 0;
#endif
		}
		TypeInfo VirtualMachine::GetFunctionDefByIndex(size_t Index) const
		{
#ifdef VI_ANGELSCRIPT
			return Engine->GetFuncdefByIndex((asUINT)Index);
#else
			return TypeInfo(nullptr);
#endif
		}
		size_t VirtualMachine::GetModulesCount() const
		{
#ifdef VI_ANGELSCRIPT
			return Engine->GetModuleCount();
#else
			return 0;
#endif
		}
		asIScriptModule* VirtualMachine::GetModuleById(int Id) const
		{
#ifdef VI_ANGELSCRIPT
			return Engine->GetModuleByIndex(Id);
#else
			return nullptr;
#endif
		}
		int VirtualMachine::GetTypeIdByDecl(const std::string_view& Decl) const
		{
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			return Engine->GetTypeIdByDecl(Decl.data());
#else
			return -1;
#endif
		}
		std::string_view VirtualMachine::GetTypeIdDecl(int TypeId, bool IncludeNamespace) const
		{
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Engine->GetTypeDeclaration(TypeId, IncludeNamespace));
#else
			return "";
#endif
		}
		Core::Option<Core::String> VirtualMachine::GetScriptSection(const std::string_view& Section)
		{
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			auto It = Sections.find(Core::KeyLookupCast(Section));
			if (It == Sections.end())
				return Core::Optional::None;

			return It->second;
		}
		TypeInfo VirtualMachine::GetTypeInfoById(int TypeId) const
		{
#ifdef VI_ANGELSCRIPT
			return Engine->GetTypeInfoById(TypeId);
#else
			return TypeInfo(nullptr);
#endif
		}
		TypeInfo VirtualMachine::GetTypeInfoByName(const std::string_view& Name)
		{
			VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
#ifdef VI_ANGELSCRIPT
			std::string_view TypeName = Name;
			std::string_view Namespace = "";
			if (!GetTypeNameScope(&TypeName, &Namespace))
				return Engine->GetTypeInfoByName(Name.data());

			BeginNamespace(Core::String(Namespace));
			asITypeInfo* Info = Engine->GetTypeInfoByName(TypeName.data());
			EndNamespace();

			return Info;
#else
			return TypeInfo(nullptr);
#endif
		}
		TypeInfo VirtualMachine::GetTypeInfoByDecl(const std::string_view& Decl) const
		{
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			return Engine->GetTypeInfoByDecl(Decl.data());
#else
			return TypeInfo(nullptr);
#endif
		}
		void VirtualMachine::SetLibraryProperty(LibraryFeatures Property, size_t Value)
		{
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			LibrarySettings[Property] = Value;
		}
		void VirtualMachine::SetCodeGenerator(const std::string_view& Name, GeneratorCallback&& Callback)
		{
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			auto It = Generators.find(Core::KeyLookupCast(Name));
			if (It != Generators.end())
			{
				if (Callback != nullptr)
					It->second = std::move(Callback);
				else
					Generators.erase(It);
			}
			else if (Callback != nullptr)
				Generators[Core::String(Name)] = std::move(Callback);
			else
				Generators.erase(Core::String(Name));
		}
		void VirtualMachine::SetPreserveSourceCode(bool Enabled)
		{
			SaveSources = Enabled;
		}
		void VirtualMachine::SetTsImports(bool Enabled, const std::string_view& ImportSyntax)
		{
			Bindings::Imports::BindSyntax(this, Enabled, ImportSyntax);
		}
		void VirtualMachine::SetCache(bool Enabled)
		{
			SaveCache = Enabled;
		}
		void VirtualMachine::SetExceptionCallback(std::function<void(ImmediateContext*)>&& Callback)
		{
			GlobalException = std::move(Callback);
		}
		void VirtualMachine::SetDebugger(DebuggerContext* Context)
		{
			Core::UMutex<std::recursive_mutex> Unique1(Sync.General);
			Core::Memory::Release(Debugger);
			Debugger = Context;
			if (Debugger != nullptr)
				Debugger->SetEngine(this);

			Core::UMutex<std::recursive_mutex> Unique2(Sync.Pool);
			for (auto* Next : Stacks)
			{
				if (Debugger != nullptr)
					AttachDebuggerToContext(Next);
				else
					DetachDebuggerFromContext(Next);
			}
		}
		void VirtualMachine::SetDefaultArrayType(const std::string_view& Type)
		{
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_ASSERT(Core::Stringify::IsCString(Type), "type should be set");
#ifdef VI_ANGELSCRIPT
			Engine->RegisterDefaultArrayType(Type.data());
#endif
		}
		void VirtualMachine::SetTypeInfoUserDataCleanupCallback(void(*Callback)(asITypeInfo*), size_t Type)
		{
			VI_ASSERT(Engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			Engine->SetTypeInfoUserDataCleanupCallback(Callback, (asPWORD)Type);
#endif
		}
		void VirtualMachine::SetEngineUserDataCleanupCallback(void(*Callback)(asIScriptEngine*), size_t Type)
		{
			VI_ASSERT(Engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			Engine->SetEngineUserDataCleanupCallback(Callback, (asPWORD)Type);
#endif
		}
		void* VirtualMachine::SetUserData(void* Data, size_t Type)
		{
			VI_ASSERT(Engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			return Engine->SetUserData(Data, (asPWORD)Type);
#else
			return nullptr;
#endif
		}
		void* VirtualMachine::GetUserData(size_t Type) const
		{
			VI_ASSERT(Engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			return Engine->GetUserData((asPWORD)Type);
#else
			return nullptr;
#endif
		}
		void VirtualMachine::ClearCache()
		{
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			for (auto Data : Datas)
				Core::Memory::Release(Data.second);

			Opcodes.clear();
			Datas.clear();
			Files.clear();
		}
		void VirtualMachine::ClearSections()
		{
			if (Debugger != nullptr || SaveSources)
				return;

			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			Sections.clear();
		}
		void VirtualMachine::SetCompilerErrorCallback(WhenErrorCallback&& Callback)
		{
			WhenError = std::move(Callback);
		}
		void VirtualMachine::SetCompilerIncludeOptions(const Compute::IncludeDesc& NewDesc)
		{
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			Include = NewDesc;
		}
		void VirtualMachine::SetCompilerFeatures(const Compute::Preprocessor::Desc& NewDesc)
		{
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			Proc = NewDesc;
		}
		void VirtualMachine::SetProcessorOptions(Compute::Preprocessor* Processor)
		{
			VI_ASSERT(Processor != nullptr, "preprocessor should be set");
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			Processor->SetIncludeOptions(Include);
			Processor->SetFeatures(Proc);
		}
		void VirtualMachine::SetCompileCallback(const std::string_view& Section, CompileCallback&& Callback)
		{
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			auto It = Callbacks.find(Core::KeyLookupCast(Section));
			if (It != Callbacks.end())
			{
				if (Callback != nullptr)
					It->second = std::move(Callback);
				else
					Callbacks.erase(It);
			}
			else if (Callback != nullptr)
				Callbacks[Core::String(Section)] = std::move(Callback);
			else
				Callbacks.erase(Core::String(Section));
		}
		void VirtualMachine::AttachDebuggerToContext(asIScriptContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			if (!Debugger || !Debugger->IsAttached())
				return DetachDebuggerFromContext(Context);
#ifdef VI_ANGELSCRIPT
			Context->SetLineCallback(asMETHOD(DebuggerContext, LineCallback), Debugger, asCALL_THISCALL);
			Context->SetExceptionCallback(asMETHOD(DebuggerContext, ExceptionCallback), Debugger, asCALL_THISCALL);
#endif
		}
		void VirtualMachine::DetachDebuggerFromContext(asIScriptContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			Context->ClearLineCallback();
			Context->SetExceptionCallback(asFUNCTION(VirtualMachine::ExceptionHandler), Context, asCALL_CDECL);
#endif
		}
		bool VirtualMachine::GetByteCodeCache(ByteCodeInfo* Info)
		{
			VI_ASSERT(Info != nullptr, "bytecode should be set");
			if (!SaveCache)
				return false;

			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			auto It = Opcodes.find(Info->Name);
			if (It == Opcodes.end())
				return false;

			Info->Data = It->second.Data;
			Info->Debug = It->second.Debug;
			Info->Valid = true;
			return true;
		}
		void VirtualMachine::SetByteCodeCache(ByteCodeInfo* Info)
		{
			VI_ASSERT(Info != nullptr, "bytecode should be set");
			Info->Valid = true;
			if (!SaveCache)
				return;

			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			Opcodes[Info->Name] = *Info;
		}
		ImmediateContext* VirtualMachine::CreateContext()
		{
#ifdef VI_ANGELSCRIPT
			asIScriptContext* Context = Engine->RequestContext();
			VI_ASSERT(Context != nullptr, "cannot create script context");
			AttachDebuggerToContext(Context);
			return new ImmediateContext(Context);
#else
			return nullptr;
#endif
		}
		Compiler* VirtualMachine::CreateCompiler()
		{
			return new Compiler(this);
		}
		asIScriptModule* VirtualMachine::CreateScopedModule(const std::string_view& Name)
		{
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
#ifdef VI_ANGELSCRIPT
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			if (!Engine->GetModule(Name.data()))
				return Engine->GetModule(Name.data(), asGM_ALWAYS_CREATE);

			Core::String Result;
			while (Result.size() < 1024)
			{
				Result.assign(Name);
				Result.append(Core::ToString(Scope++));
				if (!Engine->GetModule(Result.c_str()))
					return Engine->GetModule(Result.c_str(), asGM_ALWAYS_CREATE);
			}
#endif
			return nullptr;
		}
		asIScriptModule* VirtualMachine::CreateModule(const std::string_view& Name)
		{
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
#ifdef VI_ANGELSCRIPT
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			return Engine->GetModule(Name.data(), asGM_ALWAYS_CREATE);
#else
			return nullptr;
#endif
		}
		void* VirtualMachine::CreateObject(const TypeInfo& Type)
		{
#ifdef VI_ANGELSCRIPT
			return Engine->CreateScriptObject(Type.GetTypeInfo());
#else
			return nullptr;
#endif
		}
		void* VirtualMachine::CreateObjectCopy(void* Object, const TypeInfo& Type)
		{
#ifdef VI_ANGELSCRIPT
			return Engine->CreateScriptObjectCopy(Object, Type.GetTypeInfo());
#else
			return nullptr;
#endif
		}
		void* VirtualMachine::CreateEmptyObject(const TypeInfo& Type)
		{
#ifdef VI_ANGELSCRIPT
			return Engine->CreateUninitializedScriptObject(Type.GetTypeInfo());
#else
			return nullptr;
#endif
		}
		Function VirtualMachine::CreateDelegate(const Function& Function, void* Object)
		{
#ifdef VI_ANGELSCRIPT
			return Engine->CreateDelegate(Function.GetFunction(), Object);
#else
			return Function;
#endif
		}
		void VirtualMachine::ReleaseObject(void* Object, const TypeInfo& Type)
		{
#ifdef VI_ANGELSCRIPT
			Engine->ReleaseScriptObject(Object, Type.GetTypeInfo());
#endif
		}
		void VirtualMachine::AddRefObject(void* Object, const TypeInfo& Type)
		{
#ifdef VI_ANGELSCRIPT
			Engine->AddRefScriptObject(Object, Type.GetTypeInfo());
#endif
		}
		void VirtualMachine::GetStatistics(unsigned int* CurrentSize, unsigned int* TotalDestroyed, unsigned int* TotalDetected, unsigned int* NewObjects, unsigned int* TotalNewDestroyed) const
		{
#ifdef VI_ANGELSCRIPT
			unsigned int asCurrentSize, asTotalDestroyed, asTotalDetected, asNewObjects, asTotalNewDestroyed;
			Engine->GetGCStatistics(&asCurrentSize, &asTotalDestroyed, &asTotalDetected, &asNewObjects, &asTotalNewDestroyed);
			if (CurrentSize != nullptr)
				*CurrentSize = (unsigned int)asCurrentSize;

			if (TotalDestroyed != nullptr)
				*TotalDestroyed = (unsigned int)asTotalDestroyed;

			if (TotalDetected != nullptr)
				*TotalDetected = (unsigned int)asTotalDetected;

			if (NewObjects != nullptr)
				*NewObjects = (unsigned int)asNewObjects;

			if (TotalNewDestroyed != nullptr)
				*TotalNewDestroyed = (unsigned int)asTotalNewDestroyed;
#endif
		}
		void VirtualMachine::ForwardEnumReferences(void* Reference, const TypeInfo& Type)
		{
#ifdef VI_ANGELSCRIPT
			Engine->ForwardGCEnumReferences(Reference, Type.GetTypeInfo());
#endif
		}
		void VirtualMachine::ForwardReleaseReferences(void* Reference, const TypeInfo& Type)
		{
#ifdef VI_ANGELSCRIPT
			Engine->ForwardGCReleaseReferences(Reference, Type.GetTypeInfo());
#endif
		}
		void VirtualMachine::GCEnumCallback(void* Reference)
		{
			FunctionFactory::GCEnumCallback(Engine, Reference);
		}
		void VirtualMachine::GCEnumCallback(asIScriptFunction* Reference)
		{
			FunctionFactory::GCEnumCallback(Engine, Reference);
		}
		void VirtualMachine::GCEnumCallback(FunctionDelegate* Reference)
		{
			FunctionFactory::GCEnumCallback(Engine, Reference);
		}
		bool VirtualMachine::TriggerDebugger(ImmediateContext* Context, uint64_t TimeoutMs)
		{
			if (!Debugger)
				return false;
#ifdef VI_ANGELSCRIPT
			asIScriptContext* Target = (Context ? Context->GetContext() : asGetActiveContext());
			if (!Target)
				return false;

			Debugger->LineCallback(Target);
			if (TimeoutMs > 0)
				std::this_thread::sleep_for(std::chrono::milliseconds(TimeoutMs));

			return true;
#else
			return false;
#endif
		}
		Compute::ExpectsPreprocessor<void> VirtualMachine::GenerateCode(Compute::Preprocessor* Processor, const std::string_view& Path, Core::String& InoutBuffer)
		{
			VI_ASSERT(Processor != nullptr, "preprocessor should be set");
			if (InoutBuffer.empty())
				return Core::Expectation::Met;

			auto Status = Processor->Process(Path, InoutBuffer);
			if (!Status)
				return Status;

			std::string_view TargetPath = Path.empty() ? "<anonymous>" : Path;
			VI_TRACE("[vm] preprocessor source code generation at %.*s (%" PRIu64 " bytes, %" PRIu64 " generators)", (int)TargetPath.size(), TargetPath.data(), (uint64_t)InoutBuffer.size(), (uint64_t)Generators.size());
			{
				Core::UnorderedSet<Core::String> AppliedGenerators;
				Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			Retry:
				size_t CurrentGenerators = Generators.size();
				for (auto& Item : Generators)
				{
					if (AppliedGenerators.find(Item.first) != AppliedGenerators.end())
						continue;

					VI_TRACE("[vm] generate source code for %s generator at %.*s (%" PRIu64 " bytes)", Item.first.c_str(), (int)TargetPath.size(), TargetPath.data(), (uint64_t)InoutBuffer.size());
					auto Status = Item.second(Processor, Path, InoutBuffer);
					if (!Status)
						return Compute::PreprocessorException(Compute::PreprocessorError::ExtensionError, 0, Status.Error().message());

					AppliedGenerators.insert(Item.first);
					if (Generators.size() != CurrentGenerators)
						goto Retry;
				}
			}

			(void)TargetPath;
			return Status;
		}
		Core::UnorderedMap<Core::String, Core::String> VirtualMachine::DumpRegisteredInterfaces(ImmediateContext* Context)
		{
#ifdef VI_ANGELSCRIPT
			Core::UnorderedSet<Core::String> Grouping;
			Core::UnorderedMap<Core::String, Core::String> Sources;
			Core::UnorderedMap<Core::String, DNamespace> Namespaces;
			Core::UnorderedMap<Core::String, Core::Vector<std::pair<Core::String, DNamespace*>>> Groups;
			auto AddGroup = [&Grouping , &Namespaces, &Groups](const Core::String& CurrentName)
			{
				auto& Group = Groups[CurrentName];
				for (auto& Namespace : Namespaces)
				{
					if (Grouping.find(Namespace.first) == Grouping.end())
					{
						Group.push_back(std::make_pair(Namespace.first, &Namespace.second));
						Grouping.insert(Namespace.first);
					}
				}
			};
			auto AddEnum = [&Namespaces](asITypeInfo* EType)
			{
				const char* ENamespace = EType->GetNamespace();
				DNamespace& Namespace = Namespaces[ENamespace ? ENamespace : ""];
				DEnum& Enum = Namespace.Enums[EType->GetName()];
				asUINT ValuesCount = EType->GetEnumValueCount();

				for (asUINT j = 0; j < ValuesCount; j++)
				{
					int EValue;
					const char* EName = EType->GetEnumValueByIndex(j, &EValue);
					Enum.Values.push_back(Core::Stringify::Text("%s = %i", EName ? EName : Core::ToString(j).c_str(), EValue));
				}
			};
			auto AddObject = [this, &Namespaces](asITypeInfo* EType)
			{
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
					Class.Types.push_back(Core::String("class ") + (SDecl ? SDecl : "__type__"));
				}

				for (asUINT j = 0; j < FuncdefsCount; j++)
				{
					asITypeInfo* FType = EType->GetChildFuncdef(j);
					asIScriptFunction* FFunction = FType->GetFuncdefSignature();
					const char* FDecl = FFunction->GetDeclaration(false, false, true);
					Class.Funcdefs.push_back(Core::String("funcdef ") + (FDecl ? FDecl : "void __unnamed" + Core::ToString(j) + "__()"));
				}

				for (asUINT j = 0; j < PropsCount; j++)
				{
					const char* PName; int PTypeId; bool PPrivate, PProtected;
					if (EType->GetProperty(j, &PName, &PTypeId, &PPrivate, &PProtected) != 0)
						continue;

					const char* PDecl = Engine->GetTypeDeclaration(PTypeId, true);
					const char* PMod = (PPrivate ? "private " : (PProtected ? "protected " : nullptr));
					Class.Props.push_back(Core::Stringify::Text("%s%s %s", PMod ? PMod : "", PDecl ? PDecl : "__type__", PName ? PName : ("__unnamed" + Core::ToString(j) + "__").c_str()));
				}

				for (asUINT j = 0; j < FactoriesCount; j++)
				{
					asIScriptFunction* FFunction = EType->GetFactoryByIndex(j);
					const char* FDecl = FFunction->GetDeclaration(false, false, true);
					Class.Methods.push_back(FDecl ? Core::String(FDecl) : "void " + Core::String(CName) + "()");
				}

				for (asUINT j = 0; j < MethodsCount; j++)
				{
					asIScriptFunction* FFunction = EType->GetMethodByIndex(j);
					const char* FDecl = FFunction->GetDeclaration(false, false, true);
					Class.Methods.push_back(FDecl ? FDecl : "void __unnamed" + Core::ToString(j) + "__()");
				}
			};
			auto AddFunction = [this, &Namespaces](asIScriptFunction* FFunction, asUINT Index)
			{
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
						Class.Functions.push_back(FDecl ? FDecl : "void __unnamed" + Core::ToString(Index) + "__()");
						return;
					}
				}

				DNamespace& Namespace = Namespaces[FNamespace ? FNamespace : ""];
				Namespace.Functions.push_back(FDecl ? FDecl : "void __unnamed" + Core::ToString(Index) + "__()");
			};
			auto AddFuncdef = [&Namespaces](asITypeInfo* FType, asUINT Index)
			{
				if (FType->GetParentType() != nullptr)
					return;

				asIScriptFunction* FFunction = FType->GetFuncdefSignature();
				const char* FNamespace = FType->GetNamespace();
				DNamespace& Namespace = Namespaces[FNamespace ? FNamespace : ""];
				const char* FDecl = FFunction->GetDeclaration(false, false, true);
				Namespace.Funcdefs.push_back(Core::String("funcdef ") + (FDecl ? FDecl : "void __unnamed" + Core::ToString(Index) + "__()"));
			};
			
			asUINT EnumsCount = Engine->GetEnumCount();
			for (asUINT i = 0; i < EnumsCount; i++)
				AddEnum(Engine->GetEnumByIndex(i));

			asUINT ObjectsCount = Engine->GetObjectTypeCount();
			for (asUINT i = 0; i < ObjectsCount; i++)
				AddObject(Engine->GetObjectTypeByIndex(i));

			asUINT FunctionsCount = Engine->GetGlobalFunctionCount();
			for (asUINT i = 0; i < FunctionsCount; i++)
				AddFunction(Engine->GetGlobalFunctionByIndex(i), i);

			asUINT FuncdefsCount = Engine->GetFuncdefCount();
			for (asUINT i = 0; i < FuncdefsCount; i++)
				AddFuncdef(Engine->GetFuncdefByIndex(i), i);

			Core::String ModuleName = "__vfinterface.as";
			if (Context != nullptr)
			{
				asIScriptFunction* Function = Context->GetFunction().GetFunction();
				if (Function != nullptr)
				{
					asIScriptModule* Module = Function->GetModule();
					if (Module != nullptr)
					{
						asUINT EnumsCount = Module->GetEnumCount();
						for (asUINT i = 0; i < EnumsCount; i++)
							AddEnum(Module->GetEnumByIndex(i));

						asUINT ObjectsCount = Module->GetObjectTypeCount();
						for (asUINT i = 0; i < ObjectsCount; i++)
							AddObject(Module->GetObjectTypeByIndex(i));

						asUINT FunctionsCount = Module->GetFunctionCount();
						for (asUINT i = 0; i < FunctionsCount; i++)
							AddFunction(Module->GetFunctionByIndex(i), i);

						ModuleName = Module->GetName();
					}
				}
			}

			AddGroup(ModuleName);
			for (auto& Group : Groups)
			{
				Core::String Offset;
				VI_SORT(Group.second.begin(), Group.second.end(), [](const auto& A, const auto& B)
				{
					return A.first.size() < B.first.size();
				});

				auto& Source = Sources[Group.first];
				auto& List = Group.second;
				for (auto It = List.begin(); It != List.end(); It++)
				{
					auto Copy = It;
					DumpNamespace(Source, It->first, *It->second, Offset);
					if (++Copy != List.end())
						Source += "\n\n";
				}
			}

			return Sources;
#else
			return Core::UnorderedMap<Core::String, Core::String>();
#endif
		}
		size_t VirtualMachine::BeginAccessMask(size_t DefaultMask)
		{
#ifdef VI_ANGELSCRIPT
			return Engine->SetDefaultAccessMask((asDWORD)DefaultMask);
#else
			return 0;
#endif
		}
		size_t VirtualMachine::EndAccessMask()
		{
#ifdef VI_ANGELSCRIPT
			return Engine->SetDefaultAccessMask((asDWORD)VirtualMachine::GetDefaultAccessMask());
#else
			return 0;
#endif
		}
		std::string_view VirtualMachine::GetNamespace() const
		{
#ifdef VI_ANGELSCRIPT
			return OrEmpty(Engine->GetDefaultNamespace());
#else
			return "";
#endif
		}
		Module VirtualMachine::GetModule(const std::string_view& Name)
		{
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
#ifdef VI_ANGELSCRIPT
			return Module(Engine->GetModule(Name.data(), asGM_CREATE_IF_NOT_EXISTS));
#else
			return Module(nullptr);
#endif
		}
		size_t VirtualMachine::GetLibraryProperty(LibraryFeatures Property)
		{
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			return LibrarySettings[Property];
		}
		size_t VirtualMachine::GetProperty(Features Property)
		{
			VI_ASSERT(Engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			return (size_t)Engine->GetEngineProperty((asEEngineProp)Property);
#else
			return 0;
#endif
		}
		void VirtualMachine::SetModuleDirectory(const std::string_view& Value)
		{
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			Include.Root = Value;
			if (Include.Root.empty())
				return;

			if (!Core::Stringify::EndsOf(Include.Root, "/\\"))
				Include.Root.append(1, VI_SPLITTER);
		}
		const Core::String& VirtualMachine::GetModuleDirectory() const
		{
			return Include.Root;
		}
		Core::Vector<Core::String> VirtualMachine::GetExposedAddons()
		{
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			Core::Vector<Core::String> Result;
			Result.reserve(Addons.size() + CLibraries.size());
			for (auto& Module : Addons)
			{
				if (Module.second.Exposed)
					Result.push_back(Core::Stringify::Text("system(0x%" PRIXPTR "):%s", (void*)&Module.second, Module.first.c_str()));
			}

			for (auto& Module : CLibraries)
				Result.push_back(Core::Stringify::Text("%s(0x%" PRIXPTR "):%s", Module.second.IsAddon ? "addon" : "clibrary", Module.second.Handle, Module.first.c_str()));

			return Result;
		}
		const Core::UnorderedMap<Core::String, VirtualMachine::Addon>& VirtualMachine::GetSystemAddons() const
		{
			return Addons;
		}
		const Core::UnorderedMap<Core::String, VirtualMachine::CLibrary>& VirtualMachine::GetCLibraries() const
		{
			return CLibraries;
		}
		const Compute::IncludeDesc& VirtualMachine::GetCompileIncludeOptions() const
		{
			return Include;
		}
		bool VirtualMachine::HasLibrary(const std::string_view& Name, bool IsAddon)
		{
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			auto It = CLibraries.find(Core::KeyLookupCast(Name));
			if (It == CLibraries.end())
				return false;

			return It->second.IsAddon == IsAddon;
		}
		bool VirtualMachine::HasSystemAddon(const std::string_view& Name)
		{
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			auto It = Addons.find(Core::KeyLookupCast(Name));
			if (It == Addons.end())
				return false;

			return It->second.Exposed;
		}
		bool VirtualMachine::HasAddon(const std::string_view& Name)
		{
			return HasLibrary(Name, true);
		}
		bool VirtualMachine::IsNullable(int TypeId)
		{
			return TypeId == 0;
		}
		bool VirtualMachine::HasDebugger()
		{
			return Debugger != nullptr;
		}
		bool VirtualMachine::AddSystemAddon(const std::string_view& Name, const Core::Vector<Core::String>& Dependencies, AddonCallback&& Callback)
		{
			VI_ASSERT(!Name.empty(), "name should not be empty");
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			auto It = Addons.find(Core::KeyLookupCast(Name));
			if (It != Addons.end())
			{
				if (Callback || !Dependencies.empty())
				{
					It->second.Dependencies = Dependencies;
					It->second.Callback = std::move(Callback);
					It->second.Exposed = false;
				}
				else
					Addons.erase(It);
			}
			else
			{
				Addon Result;
				Result.Dependencies = Dependencies;
				Result.Callback = std::move(Callback);
				Addons.insert({ Core::String(Name), std::move(Result) });
			}

			if (Name == "*")
				return true;

			auto& Lists = Addons["*"];
			Lists.Dependencies.push_back(Core::String(Name));
			Lists.Exposed = false;
			return true;
		}
		ExpectsVM<void> VirtualMachine::ImportFile(const std::string_view& Path, bool IsRemote, Core::String& Output)
		{
			if (!IsRemote)
			{
				if (!Core::OS::File::IsExists(Path))
					return VirtualException("file not found: " + Core::String(Path));

				if (!Core::Stringify::EndsWith(Path, ".as"))
					return ImportAddon(Path);
			}

			if (!SaveCache)
			{
				auto Data = Core::OS::File::ReadAsString(Path);
				if (!Data)
					return VirtualException(Core::Copy<Core::String>(Data.Error().message()) + ": " + Core::String(Path));

				Output.assign(*Data);
				return Core::Expectation::Met;
			}

			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			auto It = Files.find(Core::KeyLookupCast(Path));
			if (It != Files.end())
			{
				Output.assign(It->second);
				return Core::Expectation::Met;
			}

			Unique.Negate();
			auto Data = Core::OS::File::ReadAsString(Path);
			if (!Data)
				return VirtualException(Core::Copy<Core::String>(Data.Error().message()) + ": " + Core::String(Path));

			Output.assign(*Data);
			Unique.Negate();
			Files.insert(std::make_pair(Path, Output));
			return Core::Expectation::Met;
		}
		ExpectsVM<void> VirtualMachine::ImportCFunction(const Core::Vector<Core::String>& Sources, const std::string_view& Func, const std::string_view& Decl)
		{
			VI_ASSERT(Core::Stringify::IsCString(Func), "func should be set");
			VI_ASSERT(Core::Stringify::IsCString(Decl), "decl should be set");
			if (!Engine || Decl.empty() || Func.empty())
				return VirtualException("import cfunction: invalid argument");

			auto LoadFunction = [this, &Func, &Decl](CLibrary& Context) -> ExpectsVM<void>
			{
				auto Handle = Context.Functions.find(Core::KeyLookupCast(Func));
				if (Handle != Context.Functions.end())
					return Core::Expectation::Met;

				auto FunctionHandle = Core::OS::Symbol::LoadFunction(Context.Handle, Func);
				if (!FunctionHandle)
					return VirtualException(Core::Copy<Core::String>(FunctionHandle.Error().message()) + ": " + Core::String(Func));

				FunctionPtr Function = (FunctionPtr)*FunctionHandle;
				if (!Function)
					return VirtualException("cfunction not found: " + Core::String(Func));
#ifdef VI_ANGELSCRIPT
				VI_TRACE("[vm] register global funcaddr(%i) %i bytes at 0x%" PRIXPTR, (int)asCALL_CDECL, (int)Decl.size(), (void*)Function);
				int Result = Engine->RegisterGlobalFunction(Decl.data(), asFUNCTION(Function), asCALL_CDECL);
				if (Result < 0)
					return VirtualException((VirtualError)Result, "register cfunction error: " + Core::String(Func));

				Context.Functions.insert({ Core::String(Func), { Core::String(Decl), (void*)Function } });
				VI_DEBUG("[vm] load function %.*s", (int)Func.size(), Func.data());
				return Core::Expectation::Met;
#else
				(void)this;
				(void)Decl;
				return VirtualException("cfunction not found: not supported");
#endif
			};

			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			auto It = CLibraries.end();
			for (auto& Item : Sources)
			{
				It = CLibraries.find(Item);
				if (It != CLibraries.end())
					break;
			}

			if (It != CLibraries.end())
				return LoadFunction(It->second);

			for (auto& Item : CLibraries)
			{
				auto Status = LoadFunction(Item.second);
				if (Status)
					return Status;
			}

			return VirtualException("cfunction not found: " + Core::String(Func));
		}
		ExpectsVM<void> VirtualMachine::ImportCLibrary(const std::string_view& Path, bool IsAddon)
		{
			auto Name = GetLibraryName(Path);
			if (!Engine)
				return VirtualException("import clibrary: invalid argument");

			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			auto Core = CLibraries.find(Core::KeyLookupCast(Name));
			if (Core != CLibraries.end())
				return Core::Expectation::Met;

			Unique.Negate();
			auto Handle = Core::OS::Symbol::Load(Path);
			if (!Handle)
				return VirtualException(Core::Copy<Core::String>(Handle.Error().message()) + ": " + Core::String(Path));

			CLibrary Library;
			Library.Handle = *Handle;
			Library.IsAddon = IsAddon;

			if (Library.IsAddon)
			{
				auto Status = InitializeAddon(Name, Library);
				if (!Status)
				{
					UninitializeAddon(Name, Library);
					Core::OS::Symbol::Unload(*Handle);
					return Status;
				}
			}

			Unique.Negate();
			CLibraries.insert({ Core::String(Name), std::move(Library) });
			VI_DEBUG("[vm] load library %.*s", (int)Path.size(), Path.data());
			return Core::Expectation::Met;
		}
		ExpectsVM<void> VirtualMachine::ImportSystemAddon(const std::string_view& Name)
		{
			if (!Core::OS::Control::Has(Core::AccessOption::Addons))
				return VirtualException("import system addon: denied");

			Core::String Target = Core::String(Name);
			if (Core::Stringify::EndsWith(Target, ".as"))
				Target = Target.substr(0, Target.size() - 3);

			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			auto It = Addons.find(Target);
			if (It == Addons.end())
				return VirtualException("system addon not found: " + Core::String(Name));

			if (It->second.Exposed)
				return Core::Expectation::Met;

			Addon Base = It->second;
			It->second.Exposed = true;
			Unique.Negate();

			for (auto& Item : Base.Dependencies)
			{
				auto Status = ImportSystemAddon(Item);
				if (!Status)
					return Status;
			}

			if (Base.Callback)
				Base.Callback(this);

			VI_DEBUG("[vm] load system addon %.*s", (int)Name.size(), Name.data());
			return Core::Expectation::Met;
		}
		ExpectsVM<void> VirtualMachine::ImportAddon(const std::string_view& Name)
		{
			return ImportCLibrary(Name, true);
		}
		ExpectsVM<void> VirtualMachine::InitializeAddon(const std::string_view& Path, CLibrary& Library)
		{
			auto ViInitializeHandle = Core::OS::Symbol::LoadFunction(Library.Handle, "ViInitialize");
			if (!ViInitializeHandle)
				return VirtualException("import user addon: no initialization routine (path = " + Core::String(Path) + ")");

			auto ViInitialize = (int(*)(VirtualMachine*))*ViInitializeHandle;
			if (!ViInitialize)
				return VirtualException("import user addon: no initialization routine (path = " + Core::String(Path) + ")");

			int Code = ViInitialize(this);
			if (Code != 0)
				return VirtualException("import user addon: initialization failed (path = " + Core::String(Path) + ", exit = " + Core::ToString(Code) + ")");

			VI_DEBUG("[vm] addon library %.*s initializated", (int)Path.size(), Path.data());
			Library.Functions.insert({ "ViInitialize", { Core::String(), (void*)ViInitialize } });
			return Core::Expectation::Met;
		}
		void VirtualMachine::UninitializeAddon(const std::string_view& Name, CLibrary& Library)
		{
			auto ViUninitializeHandle = Core::OS::Symbol::LoadFunction(Library.Handle, "ViUninitialize");
			if (!ViUninitializeHandle)
				return;

			auto ViUninitialize = (void(*)(VirtualMachine*))*ViUninitializeHandle;
			if (ViUninitialize != nullptr)
			{
				Library.Functions.insert({ "ViUninitialize", { Core::String(), (void*)ViUninitialize } });
				ViUninitialize(this);
			}
		}
		Core::Option<Core::String> VirtualMachine::GetSourceCodeAppendix(const std::string_view& Label, const std::string_view& Code, uint32_t LineNumber, uint32_t ColumnNumber, size_t MaxLines)
		{
			if (MaxLines % 2 == 0)
				++MaxLines;

			Core::Vector<Core::String> Lines = ExtractLinesOfCode(Code, (int)LineNumber, (int)MaxLines);
			if (Lines.empty())
				return Core::Optional::None;

			Core::String Line = Lines.front();
			Lines.erase(Lines.begin());
			if (Line.empty())
				return Core::Optional::None;

			Core::StringStream Stream;
			size_t TopLines = (Lines.size() % 2 != 0 ? 1 : 0) + Lines.size() / 2;
			size_t TopLine = TopLines < LineNumber ? LineNumber - TopLines - 1 : LineNumber;
			Stream << "\n  last " << (Lines.size() + 1) << " lines of " << Label << " code\n";

			for (size_t i = 0; i < TopLines; i++)
				Stream << "  " << TopLine++ << "  " << Core::Stringify::TrimEnd(Lines[i]) << "\n";
			Stream << "  " << TopLine++ << "  " << Core::Stringify::TrimEnd(Line) << "\n  ";

			ColumnNumber += 1 + (uint32_t)Core::ToString(LineNumber).size();
			for (uint32_t i = 0; i < ColumnNumber; i++)
				Stream << " ";
			Stream << "^";

			for (size_t i = TopLines; i < Lines.size(); i++)
				Stream << "\n  " << TopLine++ << "  " << Core::Stringify::TrimEnd(Lines[i]);

			return Stream.str();
		}
		Core::Option<Core::String> VirtualMachine::GetSourceCodeAppendixByPath(const std::string_view& Label, const std::string_view& Path, uint32_t LineNumber, uint32_t ColumnNumber, size_t MaxLines)
		{
			auto Code = GetScriptSection(Path);
			if (!Code)
				return Code;

			return GetSourceCodeAppendix(Label, *Code, LineNumber, ColumnNumber, MaxLines);
		}
		size_t VirtualMachine::GetProperty(Features Property) const
		{
			VI_ASSERT(Engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			return (size_t)Engine->GetEngineProperty((asEEngineProp)Property);
#else
			return 0;
#endif
		}
		asIScriptEngine* VirtualMachine::GetEngine() const
		{
			return Engine;
		}
		DebuggerContext* VirtualMachine::GetDebugger() const
		{
			return Debugger;
		}
		void VirtualMachine::Cleanup()
		{
			TypeCache::Cleanup();
			CleanupThisThread();
		}
		VirtualMachine* VirtualMachine::Get(asIScriptEngine* Engine)
		{
			VI_ASSERT(Engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			void* VM = Engine->GetUserData(ManagerUD);
			VI_ASSERT(VM != nullptr, "engine should be created by virtual machine");
			return (VirtualMachine*)VM;
#else
			return nullptr;
#endif
		}
		VirtualMachine* VirtualMachine::Get()
		{
#ifdef VI_ANGELSCRIPT
			asIScriptContext* Context = asGetActiveContext();
			if (!Context)
				return nullptr;

			return Get(Context->GetEngine());
#else
			return nullptr;
#endif
		}
		std::string_view VirtualMachine::GetLibraryName(const std::string_view& Path)
		{
			if (Path.empty())
				return Path;

			Core::TextSettle Start = Core::Stringify::ReverseFindOf(Path, "\\/");
			if (!Start.Found)
				return Path;

			return Path.substr(Start.End);
		}
		ImmediateContext* VirtualMachine::RequestContext()
		{
			Core::UMutex<std::recursive_mutex> Unique(Sync.Pool);
			if (Threads.empty())
			{
				Unique.Negate();
				return CreateContext();
			}

			ImmediateContext* Context = *Threads.rbegin();
			Threads.pop_back();
			return Context;
		}
		void VirtualMachine::ReturnContext(ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			Core::UMutex<std::recursive_mutex> Unique(Sync.Pool);
			Threads.push_back(Context);
			Context->Reset();
		}
		asIScriptContext* VirtualMachine::RequestRawContext(asIScriptEngine* Engine, void* Data)
		{
#ifdef VI_ANGELSCRIPT
			VirtualMachine* VM = VirtualMachine::Get(Engine);
			if (!VM || VM->Stacks.empty())
				return Engine->CreateContext();

			Core::UMutex<std::recursive_mutex> Unique(VM->Sync.Pool);
			if (VM->Stacks.empty())
			{
				Unique.Negate();
				return Engine->CreateContext();
			}

			asIScriptContext* Context = *VM->Stacks.rbegin();
			VM->Stacks.pop_back();
			return Context;
#else
			return nullptr;
#endif
		}
		void VirtualMachine::ReturnRawContext(asIScriptEngine* Engine, asIScriptContext* Context, void* Data)
		{
#ifdef VI_ANGELSCRIPT
			VirtualMachine* VM = VirtualMachine::Get(Engine);
			VI_ASSERT(VM != nullptr, "engine should be set");

			Core::UMutex<std::recursive_mutex> Unique(VM->Sync.Pool);
			VM->Stacks.push_back(Context);
			Context->Unprepare();
#endif
		}
		void VirtualMachine::LineHandler(asIScriptContext* Context, void*)
		{
			ImmediateContext* Base = ImmediateContext::Get(Context);
			VI_ASSERT(Base != nullptr, "context should be set");
			VI_ASSERT(Base->Callbacks.Line != nullptr, "callback should be set");
			Base->Callbacks.Line(Base);
		}
		void VirtualMachine::ExceptionHandler(asIScriptContext* Context, void*)
		{
#ifdef VI_ANGELSCRIPT
			ImmediateContext* Base = ImmediateContext::Get(Context);
			VI_ASSERT(Base != nullptr, "context should be set");

			VirtualMachine* VM = Base->GetVM();
			if (VM->Debugger != nullptr)
				return VM->Debugger->ThrowInternalException(Context->GetExceptionString());

			const char* Message = Context->GetExceptionString();
			if (Message && Message[0] != '\0' && !Context->WillExceptionBeCaught())
			{
				Core::String Details = Bindings::Exception::Pointer(Core::String(Message)).What();
				Core::String Trace = Core::ErrorHandling::GetStackTrace(1, 64);
				VI_ERR("[vm] uncaught exception %s, callstack:\n%.*s", Details.empty() ? "unknown" : Details.c_str(), (int)Trace.size(), Trace.c_str());
				Core::UMutex<std::recursive_mutex> Unique(Base->Exchange);
				Base->Executor.Stacktrace = Trace;
				Unique.Negate();

				if (Base->Callbacks.Exception)
					Base->Callbacks.Exception(Base);
				else if (VM->GlobalException)
					VM->GlobalException(Base);
			}
			else if (Base->Callbacks.Exception)
				Base->Callbacks.Exception(Base);
			else if (VM->GlobalException)
				VM->GlobalException(Base);
#endif
		}
		void VirtualMachine::SetMemoryFunctions(void* (*Alloc)(size_t), void(*Free)(void*))
		{
#ifdef VI_ANGELSCRIPT
			asSetGlobalMemoryFunctions(Alloc, Free);
#endif
		}
		void VirtualMachine::CleanupThisThread()
		{
#ifdef VI_ANGELSCRIPT
			asThreadCleanup();
#endif
		}
		std::string_view VirtualMachine::GetErrorNameInfo(VirtualError Code)
		{
			switch (Code)
			{
				case VirtualError::SUCCESS:
					return "OK";
				case VirtualError::ERR:
					return "invalid operation";
				case VirtualError::CONTEXT_ACTIVE:
					return "context is in use";
				case VirtualError::CONTEXT_NOT_FINISHED:
					return "context is not finished";
				case VirtualError::CONTEXT_NOT_PREPARED:
					return "context is not prepared";
				case VirtualError::INVALID_ARG:
					return "invalid argument";
				case VirtualError::NO_FUNCTION:
					return "function not found";
				case VirtualError::NOT_SUPPORTED:
					return "operation not supported";
				case VirtualError::INVALID_NAME:
					return "invalid name argument";
				case VirtualError::NAME_TAKEN:
					return "name is in use";
				case VirtualError::INVALID_DECLARATION:
					return "invalid code declaration";
				case VirtualError::INVALID_OBJECT:
					return "invalid object argument";
				case VirtualError::INVALID_TYPE:
					return "invalid type argument";
				case VirtualError::ALREADY_REGISTERED:
					return "type is already registered";
				case VirtualError::MULTIPLE_FUNCTIONS:
					return "function overload is not deducible";
				case VirtualError::NO_MODULE:
					return "module not found";
				case VirtualError::NO_GLOBAL_VAR:
					return "global variable not found";
				case VirtualError::INVALID_CONFIGURATION:
					return "invalid configuration state";
				case VirtualError::INVALID_INTERFACE:
					return "invalid interface type";
				case VirtualError::CANT_BIND_ALL_FUNCTIONS:
					return "function binding failed";
				case VirtualError::LOWER_ARRAY_DIMENSION_NOT_REGISTERED:
					return "lower array dimension not registered";
				case VirtualError::WRONG_CONFIG_GROUP:
					return "invalid configuration group";
				case VirtualError::CONFIG_GROUP_IS_IN_USE:
					return "configuration group is in use";
				case VirtualError::ILLEGAL_BEHAVIOUR_FOR_TYPE:
					return "illegal type behaviour configuration";
				case VirtualError::WRONG_CALLING_CONV:
					return "illegal function calling convention";
				case VirtualError::BUILD_IN_PROGRESS:
					return "program compiler is in use";
				case VirtualError::INIT_GLOBAL_VARS_FAILED:
					return "global variable initialization failed";
				case VirtualError::OUT_OF_MEMORY:
					return "out of memory";
				case VirtualError::MODULE_IS_IN_USE:
					return "module is in use";
				default:
					return "internal operation failed";
			}
		}
		ByteCodeLabel VirtualMachine::GetByteCodeInfo(uint8_t Code)
		{
#ifdef VI_ANGELSCRIPT
			auto& Source = asBCInfo[Code];
			ByteCodeLabel Label;
			Label.Name = Source.name;
			Label.Code = (uint8_t)Source.bc;
			Label.Size = asBCTypeSize[Source.type];
			Label.OffsetOfStack = Source.stackInc;
			Label.SizeOfArg0 = 0;
			Label.SizeOfArg1 = 0;
			Label.SizeOfArg2 = 0;

			switch (Source.type)
			{
				case asBCTYPE_W_ARG:
				case asBCTYPE_wW_ARG:
				case asBCTYPE_rW_ARG:
					Label.SizeOfArg0 = sizeof(asWORD);
					break;
				case asBCTYPE_DW_ARG:
					Label.SizeOfArg0 = sizeof(asDWORD);
					break;
				case asBCTYPE_wW_DW_ARG:
				case asBCTYPE_rW_DW_ARG:
					Label.SizeOfArg0 = sizeof(asWORD);
					Label.SizeOfArg1 = sizeof(asDWORD);
					break;
				case asBCTYPE_QW_ARG:
					Label.SizeOfArg0 = sizeof(asQWORD);
					break;
				case asBCTYPE_DW_DW_ARG:
					Label.SizeOfArg0 = sizeof(asDWORD);
					Label.SizeOfArg1 = sizeof(asDWORD);
					break;
				case asBCTYPE_wW_rW_rW_ARG:
					Label.SizeOfArg0 = sizeof(asWORD);
					Label.SizeOfArg1 = sizeof(asWORD);
					Label.SizeOfArg2 = sizeof(asWORD);
					break;
				case asBCTYPE_wW_QW_ARG:
					Label.SizeOfArg0 = sizeof(asWORD);
					Label.SizeOfArg1 = sizeof(asQWORD);
					break;
				case asBCTYPE_wW_rW_ARG:
				case asBCTYPE_rW_rW_ARG:
					Label.SizeOfArg0 = sizeof(asWORD);
					Label.SizeOfArg0 = sizeof(asWORD);
					break;
				case asBCTYPE_wW_rW_DW_ARG:
					Label.SizeOfArg0 = sizeof(asWORD);
					Label.SizeOfArg1 = sizeof(asWORD);
					Label.SizeOfArg2 = sizeof(asDWORD);
					break;
				case asBCTYPE_QW_DW_ARG:
					Label.SizeOfArg0 = sizeof(asQWORD);
					Label.SizeOfArg1 = sizeof(asWORD);
					break;
				case asBCTYPE_rW_QW_ARG:
					Label.SizeOfArg0 = sizeof(asWORD);
					Label.SizeOfArg1 = sizeof(asQWORD);
					break;
				case asBCTYPE_W_DW_ARG:
					Label.SizeOfArg0 = sizeof(asWORD);
					break;
				case asBCTYPE_rW_W_DW_ARG:
					Label.SizeOfArg0 = sizeof(asWORD);
					Label.SizeOfArg1 = sizeof(asWORD);
					break;
				case asBCTYPE_rW_DW_DW_ARG:
					Label.SizeOfArg0 = sizeof(asWORD);
					Label.SizeOfArg1 = sizeof(asDWORD);
					Label.SizeOfArg2 = sizeof(asDWORD);
					break;
				default:
					break;
			}

			Label.OffsetOfArg0 = 1;
			Label.OffsetOfArg1 = Label.OffsetOfArg0 + Label.SizeOfArg0;
			Label.OffsetOfArg2 = Label.OffsetOfArg1 + Label.SizeOfArg1;
			return Label;
#else
			return ByteCodeLabel();
#endif
		}
		void VirtualMachine::MessageLogger(asSMessageInfo* Info, void* This)
		{
#ifdef VI_ANGELSCRIPT
			VirtualMachine* Engine = (VirtualMachine*)This;
			const char* Section = (Info->section && Info->section[0] != '\0' ? Info->section : "?");
			if (Engine->WhenError)
				Engine->WhenError();

			auto SourceCode = Engine->GetSourceCodeAppendixByPath("error", Section, Info->row, Info->col, 5);
			if (Engine != nullptr && !Engine->Callbacks.empty())
			{
				auto It = Engine->Callbacks.find(Section);
				if (It != Engine->Callbacks.end())
				{
					if (Info->type == asMSGTYPE_WARNING)
						return It->second(Core::Stringify::Text("WARN %i: %s%s", Info->row, Info->message, SourceCode ? SourceCode->c_str() : ""));
					else if (Info->type == asMSGTYPE_INFORMATION)
						return It->second(Core::Stringify::Text("INFO %s", Info->message));

					return It->second(Core::Stringify::Text("ERR %i: %s%s", Info->row, Info->message, SourceCode ? SourceCode->c_str() : ""));
				}
			}

			if (Info->type == asMSGTYPE_WARNING)
				VI_WARN("[asc] %s:%i: %s%s", Section, Info->row, Info->message, SourceCode ? SourceCode->c_str() : "");
			else if (Info->type == asMSGTYPE_INFORMATION)
				VI_INFO("[asc] %s", Info->message);
			else if (Info->type == asMSGTYPE_ERROR)
				VI_ERR("[asc] %s: %i: %s%s", Section, Info->row, Info->message, SourceCode ? SourceCode->c_str() : "");
#endif
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

		EventLoop::Callable::Callable(ImmediateContext* NewContext) noexcept : Context(NewContext)
		{
		}
		EventLoop::Callable::Callable(ImmediateContext* NewContext, FunctionDelegate&& NewDelegate, ArgsCallback&& NewOnArgs, ArgsCallback&& NewOnReturn) noexcept : Delegate(std::move(NewDelegate)), OnArgs(std::move(NewOnArgs)), OnReturn(std::move(NewOnReturn)), Context(NewContext)
		{
		}
		EventLoop::Callable::Callable(const Callable& Other) noexcept : Delegate(Other.Delegate), OnArgs(Other.OnArgs), OnReturn(Other.OnReturn), Context(Other.Context)
		{
		}
		EventLoop::Callable::Callable(Callable&& Other) noexcept : Delegate(std::move(Other.Delegate)), OnArgs(std::move(Other.OnArgs)), OnReturn(std::move(Other.OnReturn)), Context(Other.Context)
		{
			Other.Context = nullptr;
		}
		EventLoop::Callable& EventLoop::Callable::operator= (const Callable& Other) noexcept
		{
			if (this == &Other)
				return *this;

			Delegate = Other.Delegate;
			OnArgs = Other.OnArgs;
			OnReturn = Other.OnReturn;
			Context = Other.Context;
			return *this;
		}
		EventLoop::Callable& EventLoop::Callable::operator= (Callable&& Other) noexcept
		{
			if (this == &Other)
				return *this;

			Delegate = std::move(Other.Delegate);
			OnArgs = std::move(Other.OnArgs);
			OnReturn = std::move(Other.OnReturn);
			Context = Other.Context;
			Other.Context = nullptr;
			return *this;
		}
		bool EventLoop::Callable::IsNotification() const
		{
			return !Delegate.IsValid();
		}
		bool EventLoop::Callable::IsCallback() const
		{
			return Delegate.IsValid();
		}

		static thread_local EventLoop* InternalLoop = nullptr;
		EventLoop::EventLoop() noexcept : Aborts(false), Wake(false)
		{
		}
		void EventLoop::OnNotification(ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			Core::UMutex<std::mutex> Unique(Mutex);
			Queue.push(Callable(Context));
			Waitable.notify_one();

			auto Ready = std::move(OnEnqueue);
			Unique.Negate();
			if (Ready)
				Ready(Context);
		}
		void EventLoop::OnCallback(ImmediateContext* Context, FunctionDelegate&& Delegate, ArgsCallback&& OnArgs, ArgsCallback&& OnReturn)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			VI_ASSERT(Delegate.IsValid(), "delegate should be valid");
			Core::UMutex<std::mutex> Unique(Mutex);
			Queue.push(Callable(Context, std::move(Delegate), std::move(OnArgs), std::move(OnReturn)));
			Waitable.notify_one();

			auto Ready = std::move(OnEnqueue);
			Unique.Negate();
			if (Ready)
				Ready(Context);
		}
		void EventLoop::Wakeup()
		{
			Core::UMutex<std::mutex> Unique(Mutex);
			Wake = true;
			Waitable.notify_all();
		}
		void EventLoop::Restore()
		{
			Core::UMutex<std::mutex> Unique(Mutex);
			Aborts = false;
		}
		void EventLoop::Abort()
		{
			Core::UMutex<std::mutex> Unique(Mutex);
			Aborts = true;
		}
		void EventLoop::When(ArgsCallback&& Callback)
		{
			OnEnqueue = std::move(Callback);
		}
		void EventLoop::Listen(ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			Context->SetNotificationResolverCallback(std::bind(&EventLoop::OnNotification, this, std::placeholders::_1));
			Context->SetCallbackResolverCallback(std::bind(&EventLoop::OnCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		}
		void EventLoop::Unlisten(ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			Context->SetNotificationResolverCallback(nullptr);
			Context->SetCallbackResolverCallback(nullptr);
		}
		void EventLoop::Enqueue(ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			Core::UMutex<std::mutex> Unique(Mutex);
			Queue.push(Callable(Context));
			Waitable.notify_one();

			auto Ready = std::move(OnEnqueue);
			Unique.Negate();
			if (Ready)
				Ready(Context);
		}
		void EventLoop::Enqueue(FunctionDelegate&& Delegate, ArgsCallback&& OnArgs, ArgsCallback&& OnReturn)
		{
			VI_ASSERT(Delegate.IsValid(), "delegate should be valid");
			ImmediateContext* Context = Delegate.Context;
			Core::UMutex<std::mutex> Unique(Mutex);
			Queue.push(Callable(Delegate.Context, std::move(Delegate), std::move(OnArgs), std::move(OnReturn)));
			Waitable.notify_one();

			auto Ready = std::move(OnEnqueue);
			Unique.Negate();
			if (Ready)
				Ready(Context);
		}
		bool EventLoop::Poll(ImmediateContext* Context, uint64_t TimeoutMs)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			if (!Bindings::Promise::IsContextBusy(Context) && Queue.empty())
				return false;
			else if (!TimeoutMs)
				return true;

			std::unique_lock<std::mutex> Unique(Mutex);
			Waitable.wait_for(Unique, std::chrono::milliseconds(TimeoutMs), [this]() { return !Queue.empty() || Wake; });
			Wake = false;
			return true;
		}
		bool EventLoop::PollExtended(ImmediateContext* Context, uint64_t TimeoutMs)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			if (!Bindings::Promise::IsContextBusy(Context) && Queue.empty())
			{
				if (!Core::Schedule::IsAvailable() || !Core::Schedule::Get()->GetPolicy().Parallel)
					return false;
			}
			else if (!TimeoutMs)
				return true;

			std::unique_lock<std::mutex> Unique(Mutex);
			Waitable.wait_for(Unique, std::chrono::milliseconds(TimeoutMs), [this]() { return !Queue.empty() || Wake; });
			Wake = false;
			return true;
		}
		size_t EventLoop::Dequeue(VirtualMachine* VM, size_t MaxExecutions)
		{
			VI_ASSERT(VM != nullptr, "virtual machine should be set");
			Core::UMutex<std::mutex> Unique(Mutex);
			size_t Executions = 0;

			while (!Queue.empty())
			{
				Callable Next = std::move(Queue.front());
				Queue.pop();
				if (Aborts)
					continue;

				Unique.Negate();
				ImmediateContext* InitiatorContext = Next.Context;
				if (Next.IsCallback())
				{
					ImmediateContext* ExecutingContext = Next.Context;
					if (!Next.Context->CanExecuteCall())
					{
						ExecutingContext = VM->RequestContext();
						Listen(ExecutingContext);
					}

					if (Next.OnReturn)
					{
						ArgsCallback OnReturn = std::move(Next.OnReturn);
						ExecutingContext->ExecuteCall(Next.Delegate.Callable(), std::move(Next.OnArgs)).When([this, VM, InitiatorContext, ExecutingContext, OnReturn = std::move(OnReturn)](ExpectsVM<Execution>&& Status) mutable
						{
							OnReturn(ExecutingContext);
							if (ExecutingContext != InitiatorContext)
								VM->ReturnContext(ExecutingContext);
							AbortIf(std::move(Status));
						});
					}
					else
					{
						ExecutingContext->ExecuteCall(Next.Delegate.Callable(), std::move(Next.OnArgs)).When([this, VM, InitiatorContext, ExecutingContext](ExpectsVM<Execution>&& Status) mutable
						{
							if (ExecutingContext != InitiatorContext)
								VM->ReturnContext(ExecutingContext);
							AbortIf(std::move(Status));
						});
					}
					++Executions;
				}
				else if (Next.IsNotification())
				{
					if (InitiatorContext->IsSuspended())
						AbortIf(InitiatorContext->Resume());
					++Executions;
				}
				Unique.Negate();
				if (MaxExecutions > 0 && Executions >= MaxExecutions)
					break;
			}

			if (Executions > 0)
				VI_TRACE("[vm] loop process %" PRIu64 " events", (uint64_t)Executions);
			return Executions;
		}
		void EventLoop::AbortIf(ExpectsVM<Execution>&& Status)
		{
			if (Status && *Status == Execution::Aborted)
				Abort();
		}
		bool EventLoop::IsAborted()
		{
			return Aborts;
		}
		void EventLoop::Set(EventLoop* ForCurrentThread)
		{
			InternalLoop = ForCurrentThread;
		}
		EventLoop* EventLoop::Get()
		{
			return InternalLoop;
		}
	}
}
