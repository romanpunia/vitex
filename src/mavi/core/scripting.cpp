#include "scripting.h"
#include "bindings.h"
#include "../mavi.h"
#include <iostream>
#define ADDON_ANY "any"
#ifdef VI_ANGELSCRIPT
#include <angelscript.h>
#include <as_texts.h>
#ifdef VI_JIT
#include <ascompiler/compiler.h>
#endif

namespace
{
	class CByteCodeStream : public asIBinaryStream
	{
	private:
		Mavi::Core::Vector<asBYTE> Code;
		int ReadPos, WritePos;

	public:
		CByteCodeStream() : ReadPos(0), WritePos(0)
		{
		}
		CByteCodeStream(const Mavi::Core::Vector<asBYTE>& Data) : Code(Data), ReadPos(0), WritePos(0)
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
		Mavi::Core::Vector<asBYTE>& GetCode()
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
		Mavi::Core::Vector<Mavi::Core::String> Values;
	};

	struct DClass
	{
		Mavi::Core::Vector<Mavi::Core::String> Props;
		Mavi::Core::Vector<Mavi::Core::String> Interfaces;
		Mavi::Core::Vector<Mavi::Core::String> Types;
		Mavi::Core::Vector<Mavi::Core::String> Funcdefs;
		Mavi::Core::Vector<Mavi::Core::String> Methods;
		Mavi::Core::Vector<Mavi::Core::String> Functions;
	};

	struct DNamespace
	{
		Mavi::Core::UnorderedMap<Mavi::Core::String, DEnum> Enums;
		Mavi::Core::UnorderedMap<Mavi::Core::String, DClass> Classes;
		Mavi::Core::Vector<Mavi::Core::String> Funcdefs;
		Mavi::Core::Vector<Mavi::Core::String> Functions;
	};

	Mavi::Core::String GetCombination(const Mavi::Core::Vector<Mavi::Core::String>& Names, const Mavi::Core::String& By)
	{
		Mavi::Core::String Result;
		for (size_t i = 0; i < Names.size(); i++)
		{
			Result.append(Names[i]);
			if (i + 1 < Names.size())
				Result.append(By);
		}

		return Result;
	}
	Mavi::Core::String GetCombinationAll(const Mavi::Core::Vector<Mavi::Core::String>& Names, const Mavi::Core::String& By, const Mavi::Core::String& EndBy)
	{
		Mavi::Core::String Result;
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
	Mavi::Core::String GetTypeNaming(asITypeInfo* Type)
	{
		const char* Namespace = Type->GetNamespace();
		return (Namespace ? Namespace + Mavi::Core::String("::") : Mavi::Core::String("")) + Type->GetName();
	}
	asITypeInfo* GetTypeNamespacing(asIScriptEngine* Engine, const Mavi::Core::String& Name)
	{
		asITypeInfo* Result = Engine->GetTypeInfoByName(Name.c_str());
		if (Result != nullptr)
			return Result;

		return Engine->GetTypeInfoByName((Name + "@").c_str());
	}
	void DumpNamespace(Mavi::Core::String& Source, const Mavi::Core::String& Naming, DNamespace& Namespace, Mavi::Core::String& Offset)
	{
		if (!Naming.empty())
		{
			Offset.append("\t");
			Source += Mavi::Core::Stringify::Text("namespace %s\n{\n", Naming.c_str());
		}

		for (auto It = Namespace.Enums.begin(); It != Namespace.Enums.end(); It++)
		{
			auto Copy = It;
			Source += Mavi::Core::Stringify::Text("%senum %s\n%s{\n\t%s", Offset.c_str(), It->first.c_str(), Offset.c_str(), Offset.c_str());
			Source += Mavi::Core::Stringify::Text("%s", GetCombination(It->second.Values, ",\n\t" + Offset).c_str());
			Source += Mavi::Core::Stringify::Text("\n%s}\n%s", Offset.c_str(), ++Copy != Namespace.Enums.end() ? "\n" : "");
		}

		if (!Namespace.Enums.empty() && (!Namespace.Classes.empty() || !Namespace.Funcdefs.empty() || !Namespace.Functions.empty()))
			Source += Mavi::Core::Stringify::Text("\n");

		for (auto It = Namespace.Classes.begin(); It != Namespace.Classes.end(); It++)
		{
			auto Copy = It;
			Source += Mavi::Core::Stringify::Text("%sclass %s%s%s%s%s%s\n%s{\n\t%s",
				Offset.c_str(),
				It->first.c_str(),
				It->second.Types.empty() ? "" : "<",
				It->second.Types.empty() ? "" : GetCombination(It->second.Types, ", ").c_str(),
				It->second.Types.empty() ? "" : ">",
				It->second.Interfaces.empty() ? "" : " : ",
				It->second.Interfaces.empty() ? "" : GetCombination(It->second.Interfaces, ", ").c_str(),
				Offset.c_str(), Offset.c_str());
			Source += Mavi::Core::Stringify::Text("%s", GetCombinationAll(It->second.Funcdefs, ";\n\t" + Offset, It->second.Props.empty() && It->second.Methods.empty() ? ";" : ";\n\n\t" + Offset).c_str());
			Source += Mavi::Core::Stringify::Text("%s", GetCombinationAll(It->second.Props, ";\n\t" + Offset, It->second.Methods.empty() ? ";" : ";\n\n\t" + Offset).c_str());
			Source += Mavi::Core::Stringify::Text("%s", GetCombinationAll(It->second.Methods, ";\n\t" + Offset, ";").c_str());
			Source += Mavi::Core::Stringify::Text("\n%s}\n%s", Offset.c_str(), !It->second.Functions.empty() || ++Copy != Namespace.Classes.end() ? "\n" : "");

			if (It->second.Functions.empty())
				continue;

			Source += Mavi::Core::Stringify::Text("%snamespace %s\n%s{\n\t%s", Offset.c_str(), It->first.c_str(), Offset.c_str(), Offset.c_str());
			Source += Mavi::Core::Stringify::Text("%s", GetCombinationAll(It->second.Functions, ";\n\t" + Offset, ";").c_str());
			Source += Mavi::Core::Stringify::Text("\n%s}\n%s", Offset.c_str(), ++Copy != Namespace.Classes.end() ? "\n" : "");
		}

		if (!Namespace.Funcdefs.empty())
		{
			if (!Namespace.Enums.empty() || !Namespace.Classes.empty())
				Source += Mavi::Core::Stringify::Text("\n%s", Offset.c_str());
			else
				Source += Mavi::Core::Stringify::Text("%s", Offset.c_str());
		}

		Source += Mavi::Core::Stringify::Text("%s", GetCombinationAll(Namespace.Funcdefs, ";\n" + Offset, Namespace.Functions.empty() ? ";\n" : "\n\n" + Offset).c_str());
		if (!Namespace.Functions.empty() && Namespace.Funcdefs.empty())
		{
			if (!Namespace.Enums.empty() || !Namespace.Classes.empty())
				Source += Mavi::Core::Stringify::Text("\n");
			else
				Source += Mavi::Core::Stringify::Text("%s", Offset.c_str());
		}

		Source += Mavi::Core::Stringify::Text("%s", GetCombinationAll(Namespace.Functions, ";\n" + Offset, ";\n").c_str());
		if (!Naming.empty())
		{
			Source += Mavi::Core::Stringify::Text("}");
			Offset.erase(Offset.begin());
		}
		else
			Source.erase(Source.end() - 1);
	}
}
#endif

namespace Mavi
{
	namespace Scripting
	{
		static Core::Vector<Core::String> ExtractLinesOfCode(const Core::String& Code, int Line, int Max)
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
					Core::String Copy = Code.substr(Start, Offset - Start);
					Core::Stringify::ReplaceOf(Copy, "\r\n\t\v", " ");
					Total.push_back(std::move(Copy));
					--LeftSide; --Max;
				}
				else if (Lines == Line)
				{
					Core::String Copy = Code.substr(Start, Offset - Start);
					Core::Stringify::ReplaceOf(Copy, "\r\n\t\v", " ");
					Total.insert(Total.begin(), std::move(Copy));
					--Max;
				}
				else if (Lines >= Line + (RightSide - BaseRightSide) && RightSide > 0)
				{
					Core::String Copy = Code.substr(Start, Offset - Start);
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
		static Core::String CharTrimEnd(const char* Value)
		{
			Core::String Copy = Value;
			Core::Stringify::TrimEnd(Copy);
			return Value;
		}

		uint64_t TypeCache::Set(uint64_t Id, const Core::String& Name)
		{
			VI_ASSERT(Id > 0 && !Name.empty(), "id should be greater than zero and name should not be empty");

			using Map = Core::Mapping<Core::UnorderedMap<uint64_t, std::pair<Core::String, int>>>;
			if (!Names)
				Names = VI_NEW(Map);

			Names->Map[Id] = std::make_pair(Name, (int)-1);
			return Id;
		}
		int TypeCache::GetTypeId(uint64_t Id)
		{
			auto It = Names->Map.find(Id);
			if (It == Names->Map.end())
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
			VI_DELETE(Mapping, Names);
			Names = nullptr;
		}
		Core::Mapping<Core::UnorderedMap<uint64_t, std::pair<Core::String, int>>>* TypeCache::Names = nullptr;

		bool Parser::ReplaceInlinePreconditions(const Core::String& Keyword, Core::String& Data, const std::function<Core::String(const Core::String& Expression)>& Replacer)
		{
			return ReplacePreconditions(false, Keyword + ' ', Data, Replacer);
		}
		bool Parser::ReplaceDirectivePreconditions(const Core::String& Keyword, Core::String& Data, const std::function<Core::String(const Core::String& Expression)>& Replacer)
		{
			return ReplacePreconditions(true, Keyword, Data, Replacer);
		}
		bool Parser::ReplacePreconditions(bool IsDirective, const Core::String& Match, Core::String& Code, const std::function<Core::String(const Core::String& Expression)>& Replacer)
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
				else if (Code.size() - Offset < MatchSize || memcmp(Code.c_str() + Offset, Match.c_str(), MatchSize) != 0)
				{
					++Offset;
					continue;
				}

				size_t Start = Offset + MatchSize;
				while (Start < Code.size())
				{
					char& V = Code[Start];
					if (!isspace(V))
						break;
					++Start;
				}

				bool IsClosed = false;
				int32_t Indexers = 0;
				int32_t Brackets = 0;
				int32_t Braces = 0;
				int32_t Quotes = 0;
				size_t End = Start;
				while (End < Code.size())
				{
					char& V = Code[End];
					if (V == ')')
					{
						if (--Brackets < 0)
						{
							if (Indexers <= 0 && Braces <= 0 && Quotes <= 0)
								break;

							VI_ERR("[generator] unexpected symbol '%c', offset %" PRIu64 ":\n%.*s <<<", V, (uint64_t)End, (int)(End - Start), Code.c_str() + Start);
							return false;
						}
					}
					else if (V == '}')
					{
						if (--Braces < 0)
						{
							if (Indexers <= 0 && Brackets <= 0 && Quotes <= 0)
								break;

							VI_ERR("[generator] unexpected symbol '%c', offset %" PRIu64 ":\n%.*s <<<", V, (uint64_t)End, (int)(End - Start), Code.c_str() + Start);
							return false;
						}
					}
					else if (V == ']')
					{
						if (--Indexers < 0)
						{
							if (Brackets <= 0 && Braces <= 0 && Quotes <= 0)
								break;

							VI_ERR("[generator] unexpected symbol '%c', offset %" PRIu64 ":\n%.*s <<<", V, (uint64_t)End, (int)(End - Start), Code.c_str() + Start);
							return false;
						}
					}
					else if (V == '"' || V == '\'')
					{
						++End; ++Quotes;
						while (End < Code.size())
						{
							if (Code[End++] == V && Core::Stringify::IsNotPrecededByEscape(Code, End - 1))
							{
								--Quotes;
								break;
							}
						}
						--End;
					}
					else if (V == '(')
						++Brackets;
					else if (V == '{')
						++Braces;
					else if (V == '[')
						++Indexers;
					else if (V == ';')
					{
						IsClosed = true;
						break;
					}
					else if (Brackets == 0 && Braces == 0 && Indexers == 0)
					{
						if (!isalnum(V) && V != '.' && V != ' ' && V != '_')
						{
							if (V != ':' || End + 1 >= Code.size() || Code[End + 1] != ':')
								break;
							else
								++End;
						}
					}
					End++;
				}

				if (Brackets + Braces + Indexers + Quotes > 0)
				{
					const char* Type = "termination character";
					if (Brackets > 0)
						Type = "')' symbol";
					else if (Braces > 0)
						Type = "'}' symbol";
					else if (Indexers > 0)
						Type = "']' symbol";
					else if (Quotes > 0)
						Type = "closing quote symbol";
					VI_ERR("[generator] unexpected end of file, expected %s, offset %" PRIu64 ":\n%.*s <<<", Type, (uint64_t)End, (int)(End - Start), Code.c_str() + Start);
					return false;
				}
				else if (End == Start)
				{
					Offset = End;
					continue;
				}

				Core::String Expression = Replacer(Code.substr(Start, End - Start));
				if (IsDirective && IsClosed)
					++End;

				Core::Stringify::ReplacePart(Code, Offset, End, Expression);
				if (Expression.find(Match) == std::string::npos)
					Offset += Expression.size();
			}

			return true;
		}

		ExpectsVM<void> FunctionFactory::AtomicNotifyGC(const char* TypeName, void* Object)
		{
			VI_ASSERT(TypeName != nullptr, "typename should be set");
			VI_ASSERT(Object != nullptr, "object should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptContext* Context = asGetActiveContext();
			VI_ASSERT(Context != nullptr, "context should be set");

			VirtualMachine* Engine = VirtualMachine::Get(Context->GetEngine());
			VI_ASSERT(Engine != nullptr, "engine should be set");

			TypeInfo Type = Engine->GetTypeInfoByName(TypeName);
			return Engine->NotifyOfNewObject(Object, Type.GetTypeInfo());
#else
			return VirtualError::NOT_SUPPORTED;
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
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		asSFuncPtr* FunctionFactory::CreateFunctionBase(void(*Base)(), int Type)
		{
			VI_ASSERT(Base != nullptr, "function pointer should be set");
#ifdef VI_ANGELSCRIPT
			asSFuncPtr* Ptr = VI_NEW(asSFuncPtr, Type);
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
			asSFuncPtr* Ptr = VI_NEW(asSFuncPtr, Type);
			Ptr->CopyMethodPtr(Base, Size);
			return Ptr;
#else
			return nullptr;
#endif
		}
		asSFuncPtr* FunctionFactory::CreateDummyBase()
		{
#ifdef VI_ANGELSCRIPT
			return VI_NEW(asSFuncPtr, 0);
#else
			return nullptr;
#endif
		}
		void FunctionFactory::ReleaseFunctor(asSFuncPtr** Ptr)
		{
			if (!Ptr || !*Ptr)
				return;
#ifdef VI_ANGELSCRIPT
			VI_DELETE(asSFuncPtr, *Ptr);
			*Ptr = nullptr;
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
		const char* MessageInfo::GetSection() const
		{
			VI_ASSERT(IsValid(), "message should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->section;
#else
			return "";
#endif
		}
		const char* MessageInfo::GetText() const
		{
			VI_ASSERT(IsValid(), "message should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->message;
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
		const char* TypeInfo::GetGroup() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetConfigGroup();
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
			VI_CLEAR(Info);
#endif
		}
		const char* TypeInfo::GetName() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetName();
#else
			return "";
#endif
		}
		const char* TypeInfo::GetNamespace() const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetNamespace();
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
		Function TypeInfo::GetFactoryByDecl(const char* Decl) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetFactoryByDecl(Decl);
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
		Function TypeInfo::GetMethodByName(const char* Name, bool GetVirtual) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetMethodByName(Name, GetVirtual);
#else
			return Function(nullptr);
#endif
		}
		Function TypeInfo::GetMethodByDecl(const char* Decl, bool GetVirtual) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetMethodByDecl(Decl, GetVirtual);
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
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		const char* TypeInfo::GetPropertyDeclaration(size_t Index, bool IncludeNamespace) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetPropertyDeclaration((asUINT)Index, IncludeNamespace);
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
		const char* TypeInfo::GetEnumValueByIndex(size_t Index, int* OutValue) const
		{
			VI_ASSERT(IsValid(), "typeinfo should be valid");
#ifdef VI_ANGELSCRIPT
			return Info->GetEnumValueByIndex((asUINT)Index, OutValue);
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
			VI_CLEAR(Ptr);
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
		const char* Function::GetModuleName() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->GetModuleName();
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
		const char* Function::GetSectionName() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->GetScriptSectionName();
#else
			return "";
#endif
		}
		const char* Function::GetGroup() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->GetConfigGroup();
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
		const char* Function::GetObjectName() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->GetObjectName();
#else
			return "";
#endif
		}
		const char* Function::GetName() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->GetName();
#else
			return "";
#endif
		}
		const char* Function::GetNamespace() const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->GetNamespace();
#else
			return "";
#endif
		}
		const char* Function::GetDecl(bool IncludeObjectName, bool IncludeNamespace, bool IncludeArgNames) const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->GetDeclaration(IncludeObjectName, IncludeNamespace, IncludeArgNames);
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
		ExpectsVM<void> Function::GetArg(size_t Index, int* TypeId, size_t* Flags, const char** Name, const char** DefaultArg) const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			asDWORD asFlags;
			int R = Ptr->GetParam((asUINT)Index, TypeId, &asFlags, Name, DefaultArg);
			if (Flags != nullptr)
				*Flags = (size_t)asFlags;
			return FunctionFactory::ToReturn(R);
#else
			return VirtualError::NOT_SUPPORTED;
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
		ExpectsVM<void> Function::GetProperty(size_t Index, const char** Name, int* TypeId) const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Ptr->GetVar((asUINT)Index, Name, TypeId));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		const char* Function::GetPropertyDecl(size_t Index, bool IncludeNamespace) const
		{
			VI_ASSERT(IsValid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return Ptr->GetVarDecl((asUINT)Index, IncludeNamespace);
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
			VI_CLEAR(Object);
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
		const char* ScriptObject::GetPropertyName(size_t Id) const
		{
			VI_ASSERT(IsValid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return Object->GetPropertyName((asUINT)Id);
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
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> GenericContext::SetReturnWord(unsigned short Value)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Generic->SetReturnWord(Value));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> GenericContext::SetReturnDWord(size_t Value)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Generic->SetReturnDWord((asDWORD)Value));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> GenericContext::SetReturnQWord(uint64_t Value)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Generic->SetReturnQWord(Value));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> GenericContext::SetReturnFloat(float Value)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Generic->SetReturnFloat(Value));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> GenericContext::SetReturnDouble(double Value)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Generic->SetReturnDouble(Value));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> GenericContext::SetReturnAddress(void* Address)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Generic->SetReturnAddress(Address));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> GenericContext::SetReturnObjectAddress(void* Object)
		{
			VI_ASSERT(IsValid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Generic->SetReturnObject(Object));
#else
			return VirtualError::NOT_SUPPORTED;
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
		ExpectsVM<void> BaseClass::SetFunctionDef(const char* Decl)
		{
			VI_ASSERT(IsValid(), "class should be valid");
			VI_ASSERT(Decl != nullptr, "declaration should be set");

			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " funcdef %i bytes", (void*)this, (int)strlen(Decl));
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RegisterFuncdef(Decl));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> BaseClass::SetOperatorCopyAddress(asSFuncPtr* Value, FunctionCall Type)
		{
			VI_ASSERT(IsValid(), "class should be valid");
			VI_ASSERT(Value != nullptr, "value should be set");

			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");

			Core::String Decl = Core::Stringify::Text("%s& opAssign(const %s &in)", GetTypeName(), GetTypeName());
			VI_TRACE("[vm] register class 0x%" PRIXPTR " op-copy funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)Type, (int)Decl.size(), (void*)Value);
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RegisterObjectMethod(GetTypeName(), Decl.c_str(), *Value, (asECallConvTypes)Type));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> BaseClass::SetBehaviourAddress(const char* Decl, Behaviours Behave, asSFuncPtr* Value, FunctionCall Type)
		{
			VI_ASSERT(IsValid(), "class should be valid");
			VI_ASSERT(Decl != nullptr, "declaration should be set");
			VI_ASSERT(Value != nullptr, "value should be set");

			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " behaviour funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)Type, (int)strlen(Decl), (void*)Value);
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RegisterObjectBehaviour(GetTypeName(), (asEBehaviours)Behave, Decl, *Value, (asECallConvTypes)Type));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> BaseClass::SetPropertyAddress(const char* Decl, int Offset)
		{
			VI_ASSERT(IsValid(), "class should be valid");
			VI_ASSERT(Decl != nullptr, "declaration should be set");

			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " property %i bytes at 0x0+%i", (void*)this, (int)strlen(Decl), Offset);
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RegisterObjectProperty(GetTypeName(), Decl, Offset));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> BaseClass::SetPropertyStaticAddress(const char* Decl, void* Value)
		{
			VI_ASSERT(IsValid(), "class should be valid");
			VI_ASSERT(Decl != nullptr, "declaration should be set");
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " static property %i bytes at 0x%" PRIXPTR, (void*)this, (int)strlen(Decl), (void*)Value);

			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			const char* Namespace = Engine->GetDefaultNamespace();
			const char* Scope = Type->GetNamespace();

			Engine->SetDefaultNamespace(Scope[0] == '\0' ? GetTypeName() : Core::String(Scope).append("::").append(GetName()).c_str());
			int R = Engine->RegisterGlobalProperty(Decl, Value);
			Engine->SetDefaultNamespace(Namespace);

			return FunctionFactory::ToReturn(R);
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> BaseClass::SetOperatorAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type)
		{
			return SetMethodAddress(Decl, Value, Type);
		}
		ExpectsVM<void> BaseClass::SetMethodAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type)
		{
			VI_ASSERT(IsValid(), "class should be valid");
			VI_ASSERT(Decl != nullptr, "declaration should be set");
			VI_ASSERT(Value != nullptr, "value should be set");

			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)Type, (int)strlen(Decl), (void*)Value);
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RegisterObjectMethod(GetTypeName(), Decl, *Value, (asECallConvTypes)Type));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> BaseClass::SetMethodStaticAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type)
		{
			VI_ASSERT(IsValid(), "class should be valid");
			VI_ASSERT(Decl != nullptr, "declaration should be set");
			VI_ASSERT(Value != nullptr, "value should be set");

			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " static funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)Type, (int)strlen(Decl), (void*)Value);
#ifdef VI_ANGELSCRIPT
			const char* Namespace = Engine->GetDefaultNamespace();
			const char* Scope = this->Type->GetNamespace();

			Engine->SetDefaultNamespace(Scope[0] == '\0' ? GetTypeName() : Core::String(Scope).append("::").append(GetName()).c_str());
			int R = Engine->RegisterGlobalFunction(Decl, *Value, (asECallConvTypes)Type);
			Engine->SetDefaultNamespace(Namespace);

			return FunctionFactory::ToReturn(R);
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> BaseClass::SetConstructorAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type)
		{
			VI_ASSERT(IsValid(), "class should be valid");
			VI_ASSERT(Decl != nullptr, "declaration should be set");
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " constructor funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)Type, (int)strlen(Decl), (void*)Value);

			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RegisterObjectBehaviour(GetTypeName(), asBEHAVE_CONSTRUCT, Decl, *Value, (asECallConvTypes)Type));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> BaseClass::SetConstructorListAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type)
		{
			VI_ASSERT(IsValid(), "class should be valid");
			VI_ASSERT(Decl != nullptr, "declaration should be set");
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " list-constructor funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)Type, (int)strlen(Decl), (void*)Value);

			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RegisterObjectBehaviour(GetTypeName(), asBEHAVE_LIST_CONSTRUCT, Decl, *Value, (asECallConvTypes)Type));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> BaseClass::SetDestructorAddress(const char* Decl, asSFuncPtr* Value)
		{
			VI_ASSERT(IsValid(), "class should be valid");
			VI_ASSERT(Decl != nullptr, "declaration should be set");
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " destructor funcaddr %i bytes at 0x%" PRIXPTR, (void*)this, (int)strlen(Decl), (void*)Value);

			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RegisterObjectBehaviour(GetTypeName(), asBEHAVE_DESTRUCT, Decl, *Value, asCALL_CDECL_OBJFIRST));
#else
			return VirtualError::NOT_SUPPORTED;
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
		const char* BaseClass::GetTypeName() const
		{
#ifdef VI_ANGELSCRIPT
			return Type ? Type->GetName() : "";
#else
			return "";
#endif
		}
		Core::String BaseClass::GetName() const
		{
			return GetTypeName();
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

					return Core::Stringify::Text("%s opNeg()%s", Out, Const ? " const" : "");
				case Operators::Com:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opCom()%s", Out, Const ? " const" : "");
				case Operators::PreInc:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opPreInc()%s", Out, Const ? " const" : "");
				case Operators::PreDec:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opPreDec()%s", Out, Const ? " const" : "");
				case Operators::PostInc:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opPostInc()%s", Out, Const ? " const" : "");
				case Operators::PostDec:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opPostDec()%s", Out, Const ? " const" : "");
				case Operators::Equals:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opEquals(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::Cmp:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opCmp(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::Assign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::AddAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opAddAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::SubAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opSubAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::MulAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opMulAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::DivAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opDivAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::ModAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opModAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::PowAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opPowAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::AndAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opAndAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::OrAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opOrAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::XOrAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opXorAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::ShlAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opShlAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::ShrAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opShrAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::UshrAssign:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opUshrAssign(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::Add:
					return Core::Stringify::Text("%s opAdd%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::Sub:
					return Core::Stringify::Text("%s opSub%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::Mul:
					return Core::Stringify::Text("%s opMul%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::Div:
					return Core::Stringify::Text("%s opDiv%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::Mod:
					return Core::Stringify::Text("%s opMod%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::Pow:
					return Core::Stringify::Text("%s opPow%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::And:
					return Core::Stringify::Text("%s opAnd%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::Or:
					return Core::Stringify::Text("%s opOr%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::XOr:
					return Core::Stringify::Text("%s opXor%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::Shl:
					return Core::Stringify::Text("%s opShl%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::Shr:
					return Core::Stringify::Text("%s opShr%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::Ushr:
					return Core::Stringify::Text("%s opUshr%s(%s)%s", Out, Right ? "_r" : "", Args ? Args : "", Const ? " const" : "");
				case Operators::Index:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opIndex(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::Call:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opCall(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::Cast:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opCast(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				case Operators::ImplCast:
					if (Right)
						return "";

					return Core::Stringify::Text("%s opImplCast(%s)%s", Out, Args ? Args : "", Const ? " const" : "");
				default:
					return "";
			}
		}

		TypeInterface::TypeInterface(VirtualMachine* Engine, asITypeInfo* Source, int Type) noexcept : VM(Engine), Type(Source), TypeId(Type)
		{
		}
		ExpectsVM<void> TypeInterface::SetMethod(const char* Decl)
		{
			VI_ASSERT(IsValid(), "interface should be valid");
			VI_ASSERT(Decl != nullptr, "declaration should be set");

			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register interface 0x%" PRIXPTR " method %i bytes", (void*)this, (int)strlen(Decl));
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RegisterInterfaceMethod(GetTypeName(), Decl));
#else
			return VirtualError::NOT_SUPPORTED;
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
		const char* TypeInterface::GetTypeName() const
		{
#ifdef VI_ANGELSCRIPT
			return Type ? Type->GetName() : "";
#else
			return "";
#endif
		}
		Core::String TypeInterface::GetName() const
		{
			return GetTypeName();
		}
		VirtualMachine* TypeInterface::GetVM() const
		{
			return VM;
		}

		Enumeration::Enumeration(VirtualMachine* Engine, asITypeInfo* Source, int Type) noexcept : VM(Engine), Type(Source), TypeId(Type)
		{
		}
		ExpectsVM<void> Enumeration::SetValue(const char* Name, int Value)
		{
			VI_ASSERT(IsValid(), "enum should be valid");
			VI_ASSERT(Name != nullptr, "name should be set");

			asIScriptEngine* Engine = VM->GetEngine();
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register enum 0x%" PRIXPTR " value %i bytes = %i", (void*)this, (int)strlen(Name), Value);
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RegisterEnumValue(GetTypeName(), Name, Value));
#else
			return VirtualError::NOT_SUPPORTED;
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
		const char* Enumeration::GetTypeName() const
		{
#ifdef VI_ANGELSCRIPT
			return Type ? Type->GetName() : "";
#else
			return "";
#endif
		}
		Core::String Enumeration::GetName() const
		{
			return GetTypeName();
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
		void Module::SetName(const char* Name)
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Name != nullptr, "name should be set");
#ifdef VI_ANGELSCRIPT
			Mod->SetName(Name);
#endif
		}
		ExpectsVM<void> Module::AddSection(const char* Name, const char* Code, size_t CodeLength, int LineOffset)
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Name != nullptr, "name should be set");
			VI_ASSERT(Code != nullptr, "code should be set");
			return VM->AddScriptSection(Mod, Name, Code, CodeLength, LineOffset);
		}
		ExpectsVM<void> Module::RemoveFunction(const Function& Function)
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Mod->RemoveFunction(Function.GetFunction()));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> Module::ResetProperties(asIScriptContext* Context)
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Mod->ResetGlobalVars(Context));
#else
			return VirtualError::NOT_SUPPORTED;
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
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> Module::LoadByteCode(ByteCodeInfo* Info)
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Info != nullptr, "bytecode should be set");
#ifdef VI_ANGELSCRIPT
			CByteCodeStream* Stream = VI_NEW(CByteCodeStream, Info->Data);
			int R = Mod->LoadByteCode(Stream, &Info->Debug);
			VI_DELETE(CByteCodeStream, Stream);
			return FunctionFactory::ToReturn(R);
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> Module::BindImportedFunction(size_t ImportIndex, const Function& Function)
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Mod->BindImportedFunction((asUINT)ImportIndex, Function.GetFunction()));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> Module::UnbindImportedFunction(size_t ImportIndex)
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Mod->UnbindImportedFunction((asUINT)ImportIndex));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> Module::BindAllImportedFunctions()
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Mod->BindAllImportedFunctions());
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> Module::UnbindAllImportedFunctions()
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Mod->UnbindAllImportedFunctions());
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> Module::CompileFunction(const char* SectionName, const char* Code, int LineOffset, size_t CompileFlags, Function* OutFunction)
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(SectionName != nullptr, "section name should be set");
			VI_ASSERT(Code != nullptr, "code should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptFunction* OutFunc = nullptr;
			int R = Mod->CompileFunction(SectionName, Code, LineOffset, (asDWORD)CompileFlags, &OutFunc);
			if (OutFunction != nullptr)
				*OutFunction = Function(OutFunc);
			return FunctionFactory::ToReturn(R);
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> Module::CompileProperty(const char* SectionName, const char* Code, int LineOffset)
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Mod->CompileGlobalVar(SectionName, Code, LineOffset));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> Module::SetDefaultNamespace(const char* Namespace)
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Mod->SetDefaultNamespace(Namespace));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> Module::RemoveProperty(size_t Index)
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Mod->RemoveGlobalVar((asUINT)Index));
#else
			return VirtualError::NOT_SUPPORTED;
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
		Function Module::GetFunctionByDecl(const char* Decl) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return Mod->GetFunctionByDecl(Decl);
#else
			return Function(nullptr);
#endif
		}
		Function Module::GetFunctionByName(const char* Name) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return Mod->GetFunctionByName(Name);
#else
			return Function(nullptr);
#endif
		}
		int Module::GetTypeIdByDecl(const char* Decl) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return Mod->GetTypeIdByDecl(Decl);
#else
			return -1;
#endif
		}
		ExpectsVM<size_t> Module::GetImportedFunctionIndexByDecl(const char* Decl) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			int R = Mod->GetImportedFunctionIndexByDecl(Decl);
			return FunctionFactory::ToReturn<size_t>(R, (size_t)R);
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> Module::SaveByteCode(ByteCodeInfo* Info) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Info != nullptr, "bytecode should be set");
#ifdef VI_ANGELSCRIPT
			CByteCodeStream* Stream = VI_NEW(CByteCodeStream);
			int R = Mod->SaveByteCode(Stream, Info->Debug);
			Info->Data = Stream->GetCode();
			VI_DELETE(CByteCodeStream, Stream);
			return FunctionFactory::ToReturn(R);
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<size_t> Module::GetPropertyIndexByName(const char* Name) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Name != nullptr, "name should be set");
#ifdef VI_ANGELSCRIPT
			int R = Mod->GetGlobalVarIndexByName(Name);
			return FunctionFactory::ToReturn<size_t>(R, (size_t)R);
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<size_t> Module::GetPropertyIndexByDecl(const char* Decl) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
			VI_ASSERT(Decl != nullptr, "declaration should be set");
#ifdef VI_ANGELSCRIPT
			int R = Mod->GetGlobalVarIndexByDecl(Decl);
			return FunctionFactory::ToReturn<size_t>(R, (size_t)R);
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> Module::GetProperty(size_t Index, PropertyInfo* Info) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
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

			return FunctionFactory::ToReturn(Result);
#else
			return VirtualError::NOT_SUPPORTED;
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
		TypeInfo Module::GetTypeInfoByName(const char* Name) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			const char* TypeName = Name;
			const char* Namespace = nullptr;
			size_t NamespaceSize = 0;

			if (!VM->GetTypeNameScope(&TypeName, &Namespace, &NamespaceSize))
				return Mod->GetTypeInfoByName(Name);

			VM->BeginNamespace(Core::String(Namespace, NamespaceSize).c_str());
			asITypeInfo* Info = Mod->GetTypeInfoByName(TypeName);
			VM->EndNamespace();

			return Info;
#else
			return TypeInfo(nullptr);
#endif
		}
		TypeInfo Module::GetTypeInfoByDecl(const char* Decl) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return Mod->GetTypeInfoByDecl(Decl);
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
		const char* Module::GetPropertyDecl(size_t Index, bool IncludeNamespace) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return Mod->GetGlobalVarDeclaration((asUINT)Index, IncludeNamespace);
#else
			return "";
#endif
		}
		const char* Module::GetDefaultNamespace() const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return Mod->GetDefaultNamespace();
#else
			return "";
#endif
		}
		const char* Module::GetImportedFunctionDecl(size_t ImportIndex) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return Mod->GetImportedFunctionDeclaration((asUINT)ImportIndex);
#else
			return "";
#endif
		}
		const char* Module::GetImportedFunctionModule(size_t ImportIndex) const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return Mod->GetImportedFunctionSourceModule((asUINT)ImportIndex);
#else
			return "";
#endif
		}
		const char* Module::GetName() const
		{
			VI_ASSERT(IsValid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return Mod->GetName();
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
			VI_CLEAR(Callback);
			VI_CLEAR(Context);
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
			Processor->SetIncludeCallback([this](Compute::Preprocessor* Processor, const Compute::IncludeResult& File, Core::String& Output)
			{
				VI_ASSERT(VM != nullptr, "engine should be set");
				switch (Include ? Include(Processor, File, Output) : Compute::IncludeType::Unchanged)
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

				if (File.Module.empty() || !Scope)
					return Compute::IncludeType::Error;

				if (!File.IsFile && File.IsAbstract)
					return VM->ImportSystemAddon(File.Module) ? Compute::IncludeType::Virtual : Compute::IncludeType::Error;

				if (!VM->ImportFile(File.Module, File.IsRemote, Output))
					return Compute::IncludeType::Error;

			Preprocess:
				if (Output.empty())
					return Compute::IncludeType::Virtual;

				if (!VM->GenerateCode(Processor, File.Module, Output))
					return Compute::IncludeType::Error;

				if (Output.empty())
					return Compute::IncludeType::Virtual;

				return VM->AddScriptSection(Scope, File.Module.c_str(), Output.c_str(), Output.size()) ? Compute::IncludeType::Virtual : Compute::IncludeType::Error;
			});
			Processor->SetPragmaCallback([this](Compute::Preprocessor* Processor, const Core::String& Name, const Core::Vector<Core::String>& Args)
			{
				VI_ASSERT(VM != nullptr, "engine should be set");
				if (Pragma && Pragma(Processor, Name, Args))
					return true;

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
						VI_INFO("[compiler] %s", Args[1].c_str());
					else if (Key == "TRACE")
						VI_DEBUG("[compiler] %s", Args[1].c_str());
					else if (Key == "WARN")
						VI_WARN("[compiler] %s", Args[1].c_str());
					else if (Key == "ERR")
						VI_ERR("[compiler] %s", Args[1].c_str());
				}
				else if (Name == "modify" && Args.size() == 2)
				{
					const Core::String& Key = Args[0];
					const Core::String& Value = Args[1];
					auto Numeric = Core::FromString<uint64_t>(Value);
					size_t Result = Numeric ? (size_t)*Numeric : 0;
#ifdef VI_ANGELSCRIPT
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
					bool Loaded;
					if (Args.size() == 3)
						Loaded = VM->ImportCLibrary(Args[0]) && VM->ImportCFunction({ Args[0] }, Args[1], Args[2]);
					else
						Loaded = VM->ImportCFunction({ }, Args[0], Args[1]);

					if (Loaded)
						Define("SOF_" + Args[1]);
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

				return true;
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
			VI_RELEASE(Processor);
		}
		void Compiler::SetIncludeCallback(const Compute::ProcIncludeCallback& Callback)
		{
			Include = Callback;
		}
		void Compiler::SetPragmaCallback(const Compute::ProcPragmaCallback& Callback)
		{
			Pragma = Callback;
		}
		void Compiler::Define(const Core::String& Word)
		{
			Processor->Define(Word);
		}
		void Compiler::Undefine(const Core::String& Word)
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
		bool Compiler::IsDefined(const Core::String& Word) const
		{
			return Processor->IsDefined(Word.c_str());
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
				return VirtualError::INVALID_ARG;

			auto Result = Prepare(Info->Name, true);
			if (Result)
				VCache = *Info;
			return Result;
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> Compiler::Prepare(const Core::String& ModuleName, bool Scoped)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(!ModuleName.empty(), "module name should not be empty");
			VI_DEBUG("[vm] prepare %s on 0x%" PRIXPTR, ModuleName.c_str(), (uintptr_t)this);
#ifdef VI_ANGELSCRIPT
			Built = false;
			VCache.Valid = false;
			VCache.Name.clear();

			if (Scope != nullptr)
				Scope->Discard();

			if (Scoped)
				Scope = VM->CreateScopedModule(ModuleName);
			else
				Scope = VM->CreateModule(ModuleName);

			if (!Scope)
				return VirtualError::INVALID_NAME;

			Scope->SetUserData(this, CompilerUD);
			VM->SetProcessorOptions(Processor);
			return Core::Optional::OK;
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> Compiler::Prepare(const Core::String& ModuleName, const Core::String& Name, bool Debug, bool Scoped)
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
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> Compiler::SaveByteCode(ByteCodeInfo* Info)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(Scope != nullptr, "module should not be empty");
			VI_ASSERT(Info != nullptr, "bytecode should be set");
			VI_ASSERT(Built, "module should be built");
#ifdef VI_ANGELSCRIPT
			CByteCodeStream* Stream = VI_NEW(CByteCodeStream);
			int R = Scope->SaveByteCode(Stream, !Info->Debug);
			Info->Data = Stream->GetCode();
			VI_DELETE(CByteCodeStream, Stream);
			if (R >= 0)
				VI_DEBUG("[vm] OK save bytecode on 0x%" PRIXPTR, (uintptr_t)this);
			return FunctionFactory::ToReturn(R);
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> Compiler::LoadFile(const Core::String& Path)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(Scope != nullptr, "module should not be empty");
#ifdef VI_ANGELSCRIPT
			if (VCache.Valid)
				return Core::Optional::OK;

			auto Source = Core::OS::Path::Resolve(Path.c_str());
			if (!Source)
				return VirtualError::INVALID_ARG;

			if (!Core::OS::File::IsExists(Source->c_str()))
			{
				VI_ERR("[vm] file %s not found", Source->c_str());
				return VirtualError::INVALID_ARG;
			}

			auto Buffer = Core::OS::File::ReadAsString(*Source);
			if (!Buffer)
			{
				VI_ERR("[vm] file %s cannot be opened", Source->c_str());
				return VirtualError::INVALID_OBJECT;
			}

			Core::String Code = *Buffer;
			if (!VM->GenerateCode(Processor, *Source, Code))
				return VirtualError::INVALID_DECLARATION;

			auto Result = VM->AddScriptSection(Scope, Source->c_str(), Code.c_str(), Code.size());
			if (Result)
				VI_DEBUG("[vm] OK load program on 0x%" PRIXPTR " (file)", (uintptr_t)this);
			return Result;
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> Compiler::LoadCode(const Core::String& Name, const Core::String& Data)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(Scope != nullptr, "module should not be empty");
#ifdef VI_ANGELSCRIPT
			if (VCache.Valid)
				return Core::Optional::OK;

			Core::String Buffer(Data);
			if (!VM->GenerateCode(Processor, Name, Buffer))
				return VirtualError::INVALID_DECLARATION;

			auto Result = VM->AddScriptSection(Scope, Name.c_str(), Buffer.c_str(), Buffer.size());
			if (Result)
				VI_DEBUG("[vm] OK load program on 0x%" PRIXPTR, (uintptr_t)this);
			return Result;
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> Compiler::LoadCode(const Core::String& Name, const char* Data, size_t Size)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(Scope != nullptr, "module should not be empty");
#ifdef VI_ANGELSCRIPT
			if (VCache.Valid)
				return Core::Optional::OK;

			Core::String Buffer(Data, Size);
			if (!VM->GenerateCode(Processor, Name, Buffer))
				return VirtualError::INVALID_DECLARATION;

			auto Result = VM->AddScriptSection(Scope, Name.c_str(), Buffer.c_str(), Buffer.size());
			if (Result)
				VI_DEBUG("[vm] OK load program on 0x%" PRIXPTR, (uintptr_t)this);
			return Result;
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsPromiseVM<void> Compiler::LoadByteCode(ByteCodeInfo* Info)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(Scope != nullptr, "module should not be empty");
			VI_ASSERT(Info != nullptr, "bytecode should be set");
#ifdef VI_ANGELSCRIPT
			CByteCodeStream* Stream = VI_NEW(CByteCodeStream, Info->Data);
			return Core::Cotask<ExpectsVM<void>>([this, Stream, Info]()
			{
				int R = 0;
				while ((R = Scope->LoadByteCode(Stream, &Info->Debug)) == asBUILD_IN_PROGRESS)
					std::this_thread::sleep_for(std::chrono::microseconds(100));
				VI_DELETE(CByteCodeStream, Stream);
				if (R >= 0)
					VI_DEBUG("[vm] OK load bytecode on 0x%" PRIXPTR, (uintptr_t)this);
				return FunctionFactory::ToReturn(R);
			});
#else
			return ExpectsPromiseVM<void>(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsPromiseVM<void> Compiler::Compile()
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(Scope != nullptr, "module should not be empty");
#ifdef VI_ANGELSCRIPT
			if (VCache.Valid)
			{
				return LoadByteCode(&VCache).Then<ExpectsVM<void>>([this](ExpectsVM<void>&& Result)
				{
					Built = !!Result;
					if (Built)
						VI_DEBUG("[vm] OK compile on 0x%" PRIXPTR " (cache)", (uintptr_t)this);
					return Result;
				});
			}

			return Core::Cotask<ExpectsVM<void>>([this]()
			{
				int R = 0;
				while ((R = Scope->Build()) == asBUILD_IN_PROGRESS)
					std::this_thread::sleep_for(std::chrono::microseconds(100));

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
			});
#else
			return ExpectsPromiseVM<void>(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsPromiseVM<void> Compiler::CompileFile(const char* Name, const char* ModuleName, const char* EntryName)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(Name != nullptr, "name should be set");
			VI_ASSERT(ModuleName != nullptr, "module name should be set");
			VI_ASSERT(EntryName != nullptr, "entry name should be set");
#ifdef VI_ANGELSCRIPT
			auto Status = Prepare(ModuleName, Name);
			if (!Status)
				return ExpectsPromiseVM<void>(Status);

			Status = LoadFile(Name);
			if (!Status)
				return ExpectsPromiseVM<void>(Status);

			return Compile();
#else
			return ExpectsPromiseVM<void>(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsPromiseVM<void> Compiler::CompileMemory(const Core::String& Buffer, const char* ModuleName, const char* EntryName)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(!Buffer.empty(), "buffer should not be empty");
			VI_ASSERT(ModuleName != nullptr, "module name should be set");
			VI_ASSERT(EntryName != nullptr, "entry name should be set");
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
			return ExpectsPromiseVM<void>(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsPromiseVM<Function> Compiler::CompileFunction(const Core::String& Buffer, const char* Returns, const char* Args, Core::Option<size_t>&& FunctionId)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(!Buffer.empty(), "buffer should not be empty");
			VI_ASSERT(Scope != nullptr, "module should not be empty");
			VI_ASSERT(Built, "module should be built");
#ifdef VI_ANGELSCRIPT
			Core::String Code = Buffer;
			Core::String Name = " __vfunc" + Core::ToString(FunctionId ? *FunctionId : (Counter + 1));
			if (!VM->GenerateCode(Processor, Name, Code))
				return ExpectsPromiseVM<Function>(VirtualError::INVALID_DECLARATION);

			Core::String Eval;
			Eval.append(Returns ? Returns : "void");
			Eval.append(Name);
			Eval.append("(");
			Eval.append(Args ? Args : "");
			Eval.append("){");

			if (!FunctionId)
				++Counter;

			if (Returns != nullptr && strncmp(Returns, "void", 4) != 0)
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

				size_t Size = strlen(Returns);
				Eval.append("return ");
				if (Returns[Size - 1] == '@')
				{
					Eval.append("@");
					Eval.append(Returns, Size - 1);
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
					std::this_thread::sleep_for(std::chrono::microseconds(100));
				return FunctionFactory::ToReturn<Function>(R, Function(FunctionPointer));
			});
#else
			return ExpectsPromiseVM<Function>(VirtualError::NOT_SUPPORTED);
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
		Compiler* Compiler::Get(ImmediateContext* Context)
		{
			if (!Context)
				return nullptr;

			return (Compiler*)Context->GetUserData(CompilerUD);
		}
		int Compiler::CompilerUD = 154;

		DebuggerContext::DebuggerContext(DebugType Type) noexcept : ForceSwitchThreads(0), LastContext(nullptr), LastFunction(nullptr), VM(nullptr), Action(Type == DebugType::Suspended ? DebugAction::Trigger : DebugAction::Continue), InputError(false), Attachable(Type != DebugType::Detach)
		{
			LastCommandAtStackLevel = 0;
			AddDefaultCommands();
			AddDefaultStringifiers();
		}
		DebuggerContext::~DebuggerContext() noexcept
		{
#ifdef VI_ANGELSCRIPT
			if (VM != nullptr)
				VI_RELEASE(VM->GetEngine());
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

					const char* File = 0;
					Context->GetLineNumber(0, 0, &File);
					if (!File)
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
			AddCommand("a, abort", "abort current execution", ArgsType::NoArgs, [this](ImmediateContext* Context, const Core::Vector<Core::String>& Args)
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
		void DebuggerContext::AddDefaultStringifiers()
		{
			AddToStringCallback("string", [](Core::String& Indent, int Depth, void* Object, int TypeId)
			{
				Core::String& Source = *(Core::String*)Object;
				Core::StringStream Stream;
				Stream << "\"" << Source << "\"";
				return Stream.str();
			});
			AddToStringCallback("decimal", [](Core::String& Indent, int Depth, void* Object, int TypeId)
			{
				Core::Decimal& Source = *(Core::Decimal*)Object;
				return Source.ToString();
			});
			AddToStringCallback("uint128", [](Core::String& Indent, int Depth, void* Object, int TypeId)
			{
				Compute::UInt128& Source = *(Compute::UInt128*)Object;
				return Source.ToString();
			});
			AddToStringCallback("uint256", [](Core::String& Indent, int Depth, void* Object, int TypeId)
			{
				Compute::UInt256& Source = *(Compute::UInt256*)Object;
				return Source.ToString();
			});
			AddToStringCallback("variant", [](Core::String& Indent, int Depth, void* Object, int TypeId)
			{
				Core::Variant& Source = *(Core::Variant*)Object;
				return "\"" + Source.Serialize() + "\"";
			});
			AddToStringCallback("any", [this](Core::String& Indent, int Depth, void* Object, int TypeId)
			{
				Bindings::Any* Source = (Bindings::Any*)Object;
				return ToString(Indent, Depth - 1, Source->GetAddressOfObject(), Source->GetTypeId());
			});
			AddToStringCallback("promise, promise_v", [this](Core::String& Indent, int Depth, void* Object, int TypeId)
			{
				Bindings::Promise* Source = (Bindings::Promise*)Object;
				Core::StringStream Stream;
				Stream << "(promise<T>)\n";
				Stream << Indent << "  state = " << (Source->IsPending() ? "pending\n" : "fulfilled\n");
				Stream << Indent << "  data = " << ToString(Indent, Depth - 1, Source->GetAddressOfObject(), Source->GetTypeId());
				return Stream.str();
			});
			AddToStringCallback("array", [this](Core::String& Indent, int Depth, void* Object, int TypeId)
			{
				Bindings::Array* Source = (Bindings::Array*)Object;
				int BaseTypeId = Source->GetElementTypeId();
				size_t Size = Source->Size();
				Core::StringStream Stream;
				Stream << "0x" << (void*)Source << " (array<T>, " << Size << " elements)";

				if (!Depth || !Size)
					return Stream.str();

				if (Size > 128)
				{
					Stream << "\n";
					Indent.append("  ");
					for (size_t i = 0; i < Size; i++)
					{
						Stream << Indent << "[" << i << "]: " << ToString(Indent, Depth - 1, Source->At(i), BaseTypeId);
						if (i + 1 < Size)
							Stream << "\n";
					}
					Indent.erase(Indent.end() - 2, Indent.end());
				}
				else
				{
					Stream << " [";
					for (size_t i = 0; i < Size; i++)
					{
						Stream << ToString(Indent, Depth - 1, Source->At(i), BaseTypeId);
						if (i + 1 < Size)
							Stream << ", ";
					}
					Stream << "]";
				}

				return Stream.str();
			});
			AddToStringCallback("dictionary", [this](Core::String& Indent, int Depth, void* Object, int TypeId)
			{
				Bindings::Dictionary* Source = (Bindings::Dictionary*)Object;
				size_t Size = Source->Size();
				Core::StringStream Stream;
				Stream << "0x" << (void*)Source << " (dictionary, " << Size << " elements)";

				if (!Depth || !Size)
					return Stream.str();

				Stream << "\n";
				Indent.append("  ");
				for (size_t i = 0; i < Size; i++)
				{
					Core::String Name; void* Value; int TypeId;
					if (!Source->GetIndex(i, &Name, &Value, &TypeId))
						continue;

					TypeInfo Type = VM->GetTypeInfoById(TypeId);
					Stream << Indent << CharTrimEnd(Type.IsValid() ? Type.GetName() : "?") << " \"" << Name << "\": " << ToString(Indent, Depth - 1, Value, TypeId);
					if (i + 1 < Size)
						Stream << "\n";
				}

				Indent.erase(Indent.end() - 2, Indent.end());
				return Stream.str();
			});
			AddToStringCallback("schema", [](Core::String& Indent, int Depth, void* Object, int TypeId)
			{
				Core::StringStream Stream;
				Core::Schema* Source = (Core::Schema*)Object;
				if (Source->Value.IsObject())
					Stream << "0x" << (void*)Source << "(schema)\n";

				Core::Schema::ConvertToJSON(Source, [&Indent, &Stream](Core::VarForm Type, const char* Buffer, size_t Size)
				{
					if (Buffer != nullptr && Size > 0)
						Stream << Core::String(Buffer, Size);

					switch (Type)
					{
						case Core::VarForm::Tab_Decrease:
							Indent.erase(Indent.end() - 2, Indent.end());
							break;
						case Core::VarForm::Tab_Increase:
							Indent.append("  ");
							break;
						case Core::VarForm::Write_Space:
							Stream << " ";
							break;
						case Core::VarForm::Write_Line:
							Stream << "\n";
							break;
						case Core::VarForm::Write_Tab:
							Stream << Indent;
							break;
						default:
							break;
					}
				});
				return Stream.str();
			});
#ifdef VI_BINDINGS
			AddToStringCallback("thread", [this](Core::String& Indent, int Depth, void* Object, int TypeId)
			{
				Bindings::Thread* Source = (Bindings::Thread*)Object;
				Core::StringStream Stream;
				Stream << "(thread)\n";
				Stream << Indent << "  id = " << Source->GetId() << "\n";
				Stream << Indent << "  state = " << (Source->IsActive() ? "active" : "suspended");
				return Stream.str();
			});
			AddToStringCallback("char_buffer", [](Core::String& Indent, int Depth, void* Object, int TypeId)
			{
				Bindings::CharBuffer* Source = (Bindings::CharBuffer*)Object;
				size_t Size = Source->Size();

				Core::StringStream Stream;
				Stream << "0x" << (void*)Source << " (char_buffer, " << Size << " bytes) [";

				char* Buffer = (char*)Source->GetPointer(0);
				size_t Count = (Size > 256 ? 256 : Size);
				Core::String Next = "0";

				for (size_t i = 0; i < Count; i++)
				{
					Next[0] = Buffer[i];
					Stream << "0x" << Compute::Codec::HexEncode(Next);
					if (i + 1 < Count)
						Stream << ", ";
				}

				if (Count < Size)
					Stream << ", ...";

				Stream << "]";
				return Stream.str();
			});
#endif
		}
		void DebuggerContext::AddCommand(const Core::String& Name, const Core::String& Description, ArgsType Type, const CommandCallback& Callback)
		{
			Descriptions[Name] = Description;
			for (auto& Command : Core::Stringify::Split(Name, ','))
			{
				Core::Stringify::Trim(Command);
				auto& Data = Commands[Command];
				Data.Callback = Callback;
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
		void DebuggerContext::AddToStringCallback(const Core::String& Type, const ToStringTypeCallback& Callback)
		{
			for (auto& Item : Core::Stringify::Split(Type, ','))
			{
				Core::Stringify::Trim(Item);
				SlowToStringCallbacks[Item] = Callback;
			}
		}
		void DebuggerContext::LineCallback(asIScriptContext* Base)
		{
			ImmediateContext* Context = ImmediateContext::Get(Base);
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
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
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
		void DebuggerContext::ThrowInternalException(const char* Message)
		{
			if (!Message)
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
			if (InputError)
				return;

			asIScriptContext* Base = Context->GetContext();
			VI_ASSERT(Base != nullptr, "context should be set");

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
		}
		void DebuggerContext::PrintValue(const Core::String& Expression, ImmediateContext* Context)
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
			auto ParseExpression = [](const Core::String& Expression) -> Core::Vector<Core::String>
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
						Stack.push_back(Expression.substr(Start, End - Start));
						Start = ++End;
					}
					else
						++End;
				}

				if (Start < End)
					Stack.push_back(Expression.substr(Start, End - Start));

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

			void* TopPointer = nullptr;
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
						ExecuteExpression(Context, Last + Name, Args, [ThisPointer, ThisTypeId](ImmediateContext* Context)
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
						ExecuteExpression(Context, Core::Stringify::Text("%s.%s(%s)", Callable.c_str(), Call.c_str(), Method->c_str()), Args, [ThisPointer, ThisTypeId](ImmediateContext* Context)
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
				{
					TopPointer = ThisPointer;
					TopTypeId = ThisTypeId;
				}
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
		void DebuggerContext::PrintByteCode(const Core::String& FunctionDecl, ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptContext* Base = Context->GetContext();
			VI_ASSERT(Base != nullptr, "context should be set");

			asIScriptFunction* Function = Context->GetContext()->GetFunction();
			if (!Function)
				return Output("  context was not found\n");

			asIScriptModule* Module = Function->GetModule();
			if (!Module)
				return Output("  module was not found\n");

			Function = Module->GetFunctionByName(FunctionDecl.c_str());
			if (!Function)
				Function = Module->GetFunctionByDecl(FunctionDecl.c_str());

			if (!Function)
				return Output("  function was not found\n");

			Core::StringStream Stream;
			Stream << "  function <" << Function->GetName() << "> bytecode instructions:";

			asUINT Size = 0, Calls = 0, Args = 0;
			asDWORD* ByteCode = Function->GetByteCode(&Size);
			for (asUINT i = 0; i < Size; i++)
			{
				asDWORD Value = ByteCode[i];
				if (Value <= std::numeric_limits<uint8_t>::max())
				{
					ByteCodeLabel RightLabel = VirtualMachine::GetByteCodeInfo((uint8_t)Value);
					Stream << "\n    0x" << (void*)(uintptr_t)(i) << ": " << RightLabel.Name << " [bc:" << (int)RightLabel.Id << ";ac:" << (int)RightLabel.Args << "]";
					++Calls;
				}
				else
				{
					Stream << " " << Value;
					++Args;
				}
			}

			Stream << "\n  " << Size << " instructions, " << Calls << " operations, " << Args << " values\n";
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

			const char* File = nullptr;
			int ColumnNumber = 0;
			int LineNumber = Context->GetExceptionLineNumber(&ColumnNumber, &File);
			if (File != nullptr && LineNumber > 0)
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
				asUINT Size = 0; size_t PreviewSize = 33;
				asDWORD* ByteCode = CurrentFunction->GetByteCode(&Size);
				Stream << "    [cf] current function: " << CurrentFunction->GetDeclaration(true, true, true) << "\n";
				if (ByteCode != nullptr && ProgramPointer < Size)
				{
					asUINT Left = std::min<asUINT>((asUINT)PreviewSize, (asUINT)ProgramPointer);
					Stream << "  stack frame #" << Level << " instructions:";
					bool HadInstruction = false;

					if (Left > 0)
					{
						Stream << "\n    ... " << ProgramPointer - Left << " instructions passed";
						for (asUINT i = 1; i < Left; i++)
						{
							asDWORD Value = ByteCode[ProgramPointer - i];
							if (Value <= std::numeric_limits<uint8_t>::max())
							{
								ByteCodeLabel LeftLabel = VirtualMachine::GetByteCodeInfo((uint8_t)Value);
								Stream << "\n    0x" << (void*)(uintptr_t)(ProgramPointer - i) << ": " << LeftLabel.Name << " [bc:" << (int)LeftLabel.Id << ";ac:" << (int)LeftLabel.Args << "]";
								HadInstruction = true;
							}
							else if (!HadInstruction)
							{
								Stream << "\n    ... " << Value;
								HadInstruction = true;
							}
							else
								Stream << " " << Value;
						}
					}

					asUINT Right = std::min<asUINT>((asUINT)PreviewSize, (asUINT)Size - (asUINT)ProgramPointer);
					ByteCodeLabel MainLabel = VirtualMachine::GetByteCodeInfo((uint8_t)ByteCode[ProgramPointer]);
					Stream << "\n  > 0x" << (void*)(uintptr_t)(ProgramPointer) << ": " << MainLabel.Name << " [bc:" << (int)MainLabel.Id << ";ac:" << (int)MainLabel.Args << "]";
					HadInstruction = true;

					if (Right > 0)
					{
						for (asUINT i = 1; i < Right; i++)
						{
							asDWORD Value = ByteCode[ProgramPointer + i];
							if (Value <= std::numeric_limits<uint8_t>::max())
							{
								ByteCodeLabel RightLabel = VirtualMachine::GetByteCodeInfo((uint8_t)Value);
								Stream << "\n    0x" << (void*)(uintptr_t)(ProgramPointer + i) << ": " << RightLabel.Name << " [bc:" << (int)RightLabel.Id << ";ac:" << (int)RightLabel.Args << "]";
								HadInstruction = true;
							}
							else if (!HadInstruction)
							{
								Stream << "\n    ... " << Value;
								HadInstruction = true;
							}
							else
								Stream << " " << Value;
						}
						Stream << "\n    ... " << Size - ProgramPointer << " more instructions\n";
					}
				}
				else
					Stream << "  stack frame #" << Level << " instructions:\n    ... instructions data are empty\n";
			}
			else
				Stream << "  stack frame #" << Level << " instructions:\n    ... instructions data are empty\n";

			Output(Stream.str());
#endif
		}
		void DebuggerContext::ListMemberProperties(ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");

			asIScriptContext* Base = Context->GetContext();
			VI_ASSERT(Base != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
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

			asIScriptContext* Base = Context->GetContext();
			VI_ASSERT(Base != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
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

			asIScriptContext* Base = Context->GetContext();
			VI_ASSERT(Base != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
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
			const char* File = nullptr;
			Context->GetLineNumber(0, 0, &File);

			if (!File)
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

			asIScriptContext* Base = Context->GetContext();
			VI_ASSERT(Base != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
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

			asIScriptContext* Base = Context->GetContext();
			VI_ASSERT(Base != nullptr, "context should be set");

			Core::StringStream Stream;
			Stream << Core::ErrorHandling::GetStackTrace(1) << "\n";
			Output(Stream.str());
		}
		void DebuggerContext::AddFuncBreakPoint(const Core::String& Function)
		{
			size_t B = Function.find_first_not_of(" \t");
			size_t E = Function.find_last_not_of(" \t");
			Core::String Actual = Function.substr(B, E != Core::String::npos ? E - B + 1 : Core::String::npos);

			Core::StringStream Stream;
			Stream << "  adding deferred break point for function '" << Actual << "'" << std::endl;
			Output(Stream.str());

			BreakPoint Point(Actual, 0, true);
			BreakPoints.push_back(Point);
		}
		void DebuggerContext::AddFileBreakPoint(const Core::String& File, int LineNumber)
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
		void DebuggerContext::Output(const Core::String& Data)
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
					VI_RELEASE(VM->GetEngine());

				VM = Engine;
				VM->GetEngine()->AddRef();
			}
#endif
		}
		bool DebuggerContext::CheckBreakPoint(ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");

			asIScriptContext* Base = Context->GetContext();
			VI_ASSERT(Base != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
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
		bool DebuggerContext::InterpretCommand(const Core::String& Command, ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");

			asIScriptContext* Base = Context->GetContext();
			VI_ASSERT(Base != nullptr, "context should be set");

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
							if (std::isspace(V))
							{
								size_t Start = Offset;
								while (++Start < Data.size() && std::isspace(Data[Start]));

								size_t End = Start;
								while (++End < Data.size() && !std::isspace(Data[End]) && Data[End] != '\"' && Data[End] != '\'');

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
		bool DebuggerContext::IsInputIgnored()
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
		ExpectsVM<void> DebuggerContext::ExecuteExpression(ImmediateContext* Context, const Core::String& Code, const Core::String& Args, ArgsCallback&& OnArgs)
		{
			VI_ASSERT(VM != nullptr, "engine should be set");
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			Core::String Indent = "  ";
			Core::String Eval = "any@ __vfdbgfunc(" + Args + "){return any(" + (Code.empty() || Code.back() != ';' ? Code : Code.substr(0, Code.size() - 1)) + ");}";
			asIScriptModule* Module = Context->GetFunction().GetModule().GetModule();
			asIScriptFunction* Function = nullptr;
			Bindings::Any* Data = nullptr;
			VM->DetachDebuggerFromContext(Context->GetContext());
			VM->ImportSystemAddon(ADDON_ANY);

			int Result = 0;
			while ((Result = Module->CompileFunction("__vfdbgfunc", Eval.c_str(), -1, asCOMP_ADD_TO_MODULE, &Function)) == asBUILD_IN_PROGRESS)
				std::this_thread::sleep_for(std::chrono::microseconds(100));

			if (Result < 0)
			{
				VM->AttachDebuggerToContext(Context->GetContext());
				return (VirtualError)Result;
			}

			Context->DisableSuspends();
			Context->PushState();
			auto Status1 = Context->Prepare(Function);
			if (!Status1)
			{
				Context->PopState();
				Context->EnableSuspends();
				VM->AttachDebuggerToContext(Context->GetContext());
				VI_RELEASE(Function);
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
			VI_RELEASE(Function);

			if (!Status2)
				return Status2.Error();

			return Core::Optional::OK;
#else
			return VirtualError::NOT_SUPPORTED;
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
				Executor.Future.Set(VirtualError::CONTEXT_NOT_PREPARED);
			while (Executor.LocalReferences > 0)
				ReleaseLocals();
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
				return ExpectsPromiseVM<Execution>(VirtualError::CONTEXT_ACTIVE);

			int Result = Context->Prepare(Function.GetFunction());
			if (Result < 0)
				return ExpectsPromiseVM<Execution>((VirtualError)Result);

			if (OnArgs)
				OnArgs(this);

			Executor.Future = ExpectsPromiseVM<Execution>();
			Resume();
			return Executor.Future;
#else
			return ExpectsPromiseVM<Execution>(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<Execution> ImmediateContext::ExecuteCallSync(const Function& Function, ArgsCallback&& OnArgs)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			VI_ASSERT(Function.IsValid(), "function should be set");
			VI_ASSERT(!Core::Costate::IsCoroutine(), "cannot safely execute in coroutine");
#ifdef VI_ANGELSCRIPT
			Core::UMutex<std::recursive_mutex> Unique(Exchange);
			if (!CanExecuteCall())
				return VirtualError::CONTEXT_ACTIVE;

			DisableSuspends();
			int Result = Context->Prepare(Function.GetFunction());
			if (Result < 0)
			{
				EnableSuspends();
				return (VirtualError)Result;
			}
			else if (OnArgs)
				OnArgs(this);

			auto Status = ExecuteNext();
			EnableSuspends();
			return Status;
#else
			return VirtualError::NOT_SUPPORTED;
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
				return VirtualError::CONTEXT_NOT_PREPARED;
			}

			DisableSuspends();
			Context->PushState();
			int Result = Context->Prepare(Function.GetFunction());
			if (Result < 0)
			{
				Context->PopState();
				EnableSuspends();
				return (VirtualError)Result;
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
			return VirtualError::NOT_SUPPORTED;
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
			return VirtualError::NOT_SUPPORTED;
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
			return VirtualError::NOT_SUPPORTED;
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
			return ExpectsPromiseVM<Execution>(VirtualError::NOT_SUPPORTED);
#endif
		}
		ExpectsVM<Execution> ImmediateContext::ResolveNotification(void* Promise)
		{
#ifdef VI_ANGELSCRIPT
			if (Callbacks.NotificationResolver)
			{
				Callbacks.NotificationResolver(this, Promise);
				return Execution::Active;
			}

			ImmediateContext* Context = this;
			Bindings::Promise* Base = (Bindings::Promise*)Promise;
			if (Core::Schedule::Get()->SetTask([Context, Base]()
			{
				if (Context->IsSuspended())
					Context->Resume();
				if (Base != nullptr)
					Base->Release();
			}) == Core::INVALID_TASK_ID)
				return VirtualError::CONTEXT_NOT_PREPARED;

			return Execution::Active;
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::Prepare(const Function& Function)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->Prepare(Function.GetFunction()));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::Unprepare()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->Unprepare());
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::Abort()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->Abort());
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::Suspend()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			if (!IsSuspendable())
			{
				Bindings::Exception::ThrowAt(this, Bindings::Exception::Pointer("async_error", "yield is not allowed in this function call"));
				return VirtualError::CONTEXT_NOT_PREPARED;
			}

			return FunctionFactory::ToReturn(Context->Suspend());
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::PushState()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->PushState());
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::PopState()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->PopState());
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::SetObject(void* Object)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetObject(Object));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::SetArg8(size_t Arg, unsigned char Value)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetArgByte((asUINT)Arg, Value));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::SetArg16(size_t Arg, unsigned short Value)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetArgWord((asUINT)Arg, Value));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::SetArg32(size_t Arg, int Value)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetArgDWord((asUINT)Arg, Value));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::SetArg64(size_t Arg, int64_t Value)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetArgQWord((asUINT)Arg, Value));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::SetArgFloat(size_t Arg, float Value)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetArgFloat((asUINT)Arg, Value));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::SetArgDouble(size_t Arg, double Value)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetArgDouble((asUINT)Arg, Value));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::SetArgAddress(size_t Arg, void* Address)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetArgAddress((asUINT)Arg, Address));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::SetArgObject(size_t Arg, void* Object)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetArgObject((asUINT)Arg, Object));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::SetArgAny(size_t Arg, void* Ptr, int TypeId)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetArgVarType((asUINT)Arg, Ptr, TypeId));
#else
			return VirtualError::NOT_SUPPORTED;
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
				return VirtualError::INVALID_OBJECT;

			int TypeId = ReturnTypeInfo->GetTypeId();
			asIScriptEngine* Engine = VM->GetEngine();
			if (TypeId & asTYPEID_OBJHANDLE)
			{
				if (*reinterpret_cast<void**>(Return) == nullptr)
				{
					*reinterpret_cast<void**>(Return) = *reinterpret_cast<void**>(Address);
					Engine->AddRefScriptObject(*reinterpret_cast<void**>(Return), ReturnTypeInfo);
					return Core::Optional::OK;
				}
			}
			else if (TypeId & asTYPEID_MASK_OBJECT)
				return FunctionFactory::ToReturn(Engine->AssignScriptObject(Return, Address, ReturnTypeInfo));

			size_t Size = Engine->GetSizeOfPrimitiveType(ReturnTypeInfo->GetTypeId());
			if (!Size)
				return VirtualError::INVALID_TYPE;

			memcpy(Return, Address, Size);
			return Core::Optional::OK;
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::GetReturnableByDecl(void* Return, const char* ReturnTypeDecl)
		{
			VI_ASSERT(ReturnTypeDecl != nullptr, "return type declaration should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* Engine = VM->GetEngine();
			return GetReturnableByType(Return, Engine->GetTypeInfoByDecl(ReturnTypeDecl));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::GetReturnableById(void* Return, int ReturnTypeId)
		{
			VI_ASSERT(ReturnTypeId != (int)TypeId::VOIDF, "return value type should not be void");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* Engine = VM->GetEngine();
			return GetReturnableByType(Return, Engine->GetTypeInfoById(ReturnTypeId));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::SetException(const char* Info, bool AllowCatch)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			VI_ASSERT(Info != nullptr && Info[0] != '\0', "exception should be set");
#ifdef VI_ANGELSCRIPT
			if (!IsSuspended() && !Executor.DeferredExceptions)
				return FunctionFactory::ToReturn(Context->SetException(Info, AllowCatch));

			Executor.DeferredException.Info = Info;
			Executor.DeferredException.AllowCatch = AllowCatch;
			return VirtualError::SUCCESS;
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::SetExceptionCallback(void(*Callback)(asIScriptContext* Context, void* Object), void* Object)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetExceptionCallback(asFUNCTION(Callback), Object, asCALL_CDECL));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::SetLineCallback(void(*Callback)(asIScriptContext* Context, void* Object), void* Object)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			VI_ASSERT(Callback != nullptr, "callback should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetLineCallback(asFUNCTION(Callback), Object, asCALL_CDECL));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::StartDeserialization()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->StartDeserialization());
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::FinishDeserialization()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->FinishDeserialization());
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::PushFunction(const Function& Func, void* Object)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->PushFunction(Func.GetFunction(), Object));
#else
			return VirtualError::NOT_SUPPORTED;
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
			return VirtualError::NOT_SUPPORTED;
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
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::SetStateRegisters(size_t StackLevel, Function CallingSystemFunction, const Function& InitialFunction, uint32_t OrigStackPointer, uint32_t ArgumentsSize, uint64_t ValueRegister, void* ObjectRegister, const TypeInfo& ObjectTypeRegister)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetStateRegisters((asUINT)StackLevel, CallingSystemFunction.GetFunction(), InitialFunction.GetFunction(), (asDWORD)OrigStackPointer, (asDWORD)ArgumentsSize, (asQWORD)ValueRegister, ObjectRegister, ObjectTypeRegister.GetTypeInfo()));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::SetCallStateRegisters(size_t StackLevel, uint32_t StackFramePointer, const Function& CurrentFunction, uint32_t ProgramPointer, uint32_t StackPointer, uint32_t StackIndex)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Context->SetCallStateRegisters((asUINT)StackLevel, (asDWORD)StackFramePointer, CurrentFunction.GetFunction(), (asDWORD)ProgramPointer, (asDWORD)StackPointer, (asDWORD)StackIndex));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<size_t> ImmediateContext::GetArgsOnStackCount(size_t StackLevel)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			int Result = Context->GetArgsOnStackCount((asUINT)StackLevel);
			return FunctionFactory::ToReturn<size_t>(Result, (size_t)Result);
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<size_t> ImmediateContext::GetPropertiesCount(size_t StackLevel)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			int Result = Context->GetVarCount((asUINT)StackLevel);
			return FunctionFactory::ToReturn<size_t>(Result, (size_t)Result);
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> ImmediateContext::GetProperty(size_t Index, size_t StackLevel, const char** Name, int* TypeId, Modifiers* TypeModifiers, bool* IsVarOnHeap, int* StackOffset)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asETypeModifiers TypeModifiers1 = asTM_NONE;
			int R = Context->GetVar((asUINT)Index, (asUINT)StackLevel, Name, TypeId, &TypeModifiers1, IsVarOnHeap, StackOffset);
			if (TypeModifiers != nullptr) *TypeModifiers = (Modifiers)TypeModifiers1;
			return FunctionFactory::ToReturn(R);
#else
			return VirtualError::NOT_SUPPORTED;
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
		int ImmediateContext::GetLineNumber(size_t StackLevel, int* Column, const char** SectionName)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetLineNumber((asUINT)StackLevel, Column, SectionName);
#else
			return 0;
#endif
		}
		void ImmediateContext::SetNotificationResolverCallback(const std::function<void(ImmediateContext*, void*)>& Callback)
		{
			Callbacks.NotificationResolver = Callback;
		}
		void ImmediateContext::SetCallbackResolverCallback(const std::function<void(ImmediateContext*, FunctionDelegate&&, ArgsCallback&&, ArgsCallback&&)>& Callback)
		{
			Callbacks.CallbackResolver = Callback;
		}
		void ImmediateContext::SetExceptionCallback(const std::function<void(ImmediateContext*)>& Callback)
		{
			Callbacks.Exception = Callback;
		}
		void ImmediateContext::SetLineCallback(const std::function<void(ImmediateContext*)>& Callback)
		{
			Callbacks.Line = Callback;
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
#endif
		}
		void ImmediateContext::AddRefLocals()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptFunction* Function = Context->GetFunction();
			++Executor.LocalReferences;

			if (!Function)
				return;

			asIScriptEngine* Engine = Context->GetEngine();
			asUINT VarsCount = Function->GetVarCount();
			for (asUINT n = 0; n < VarsCount; n++)
			{
				int TypeId;
				if (Context->IsVarInScope(n) && Context->GetVar(n, 0, 0, &TypeId) >= 0 && ((TypeId & asTYPEID_OBJHANDLE) || (TypeId & asTYPEID_SCRIPTOBJECT)))
					Engine->AddRefScriptObject(*(void**)Context->GetAddressOfVar(n), Engine->GetTypeInfoById(TypeId));
			}
#endif
		}
		void ImmediateContext::ReleaseLocals()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			VI_ASSERT(Executor.LocalReferences > 0, "locals are not referenced");
#ifdef VI_ANGELSCRIPT
			asIScriptFunction* Function = Context->GetFunction();
			--Executor.LocalReferences;

			if (!Function)
				return;

			asIScriptEngine* Engine = Context->GetEngine();
			asUINT VarsCount = Function->GetVarCount();
			for (asUINT n = 0; n < VarsCount; n++)
			{
				int TypeId;
				if (Context->IsVarInScope(n) && Context->GetVar(n, 0, 0, &TypeId) >= 0 && ((TypeId & asTYPEID_OBJHANDLE) || (TypeId & asTYPEID_SCRIPTOBJECT)))
					Engine->ReleaseScriptObject(*(void**)Context->GetAddressOfVar(n), Engine->GetTypeInfoById(TypeId));
			}
#endif
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
		int ImmediateContext::GetExceptionLineNumber(int* Column, const char** SectionName)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetExceptionLineNumber(Column, SectionName);
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
		const char* ImmediateContext::GetExceptionString()
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetExceptionString();
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
		const char* ImmediateContext::GetPropertyName(size_t Index, size_t StackLevel)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			const char* Name = nullptr;
			Context->GetVar((asUINT)Index, (asUINT)StackLevel, &Name);
			return Name;
#else
			return "";
#endif
		}
		const char* ImmediateContext::GetPropertyDecl(size_t Index, size_t StackLevel, bool IncludeNamespace)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return Context->GetVarDeclaration((asUINT)Index, (asUINT)StackLevel, IncludeNamespace);
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

		VirtualMachine::VirtualMachine() noexcept : Scope(0), Debugger(nullptr), Engine(nullptr), Translator(nullptr), Imports((uint32_t)Imports::All), SaveSources(false), Cached(true)
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
			RegisterAddons(this);
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
				VI_RELEASE(Context);
#ifdef VI_ANGELSCRIPT
			for (auto& Context : Stacks)
				VI_RELEASE(Context);
#endif
			VI_CLEAR(Debugger);
			CleanupThisThread();
#ifdef VI_ANGELSCRIPT
			if (Engine != nullptr)
			{
				Engine->ShutDownAndRelease();
				Engine = nullptr;
			}
#endif
			SetByteCodeTranslator((unsigned int)TranslationOptions::Disabled);
			ClearCache();
		}
		ExpectsVM<TypeInterface> VirtualMachine::SetInterface(const char* Name)
		{
			VI_ASSERT(Name != nullptr, "name should be set");
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register interface %i bytes", (int)strlen(Name));
#ifdef VI_ANGELSCRIPT
			int TypeId = Engine->RegisterInterface(Name);
			return FunctionFactory::ToReturn<TypeInterface>(TypeId, TypeInterface(this, Engine->GetTypeInfoById(TypeId), TypeId));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<TypeClass> VirtualMachine::SetStructAddress(const char* Name, size_t Size, uint64_t Flags)
		{
			VI_ASSERT(Name != nullptr, "name should be set");
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register struct(%i) %i bytes sizeof %i", (int)Flags, (int)strlen(Name), (int)Size);
#ifdef VI_ANGELSCRIPT
			int TypeId = Engine->RegisterObjectType(Name, (asUINT)Size, (asDWORD)Flags);
			return FunctionFactory::ToReturn<TypeClass>(TypeId, TypeClass(this, Engine->GetTypeInfoById(TypeId), TypeId));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<TypeClass> VirtualMachine::SetPodAddress(const char* Name, size_t Size, uint64_t Flags)
		{
			return SetStructAddress(Name, Size, Flags);
		}
		ExpectsVM<RefClass> VirtualMachine::SetClassAddress(const char* Name, size_t Size, uint64_t Flags)
		{
			VI_ASSERT(Name != nullptr, "name should be set");
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class(%i) %i bytes", (int)Flags, (int)strlen(Name));
#ifdef VI_ANGELSCRIPT
			int TypeId = Engine->RegisterObjectType(Name, (asUINT)Size, (asDWORD)Flags);
			return FunctionFactory::ToReturn<RefClass>(TypeId, RefClass(this, Engine->GetTypeInfoById(TypeId), TypeId));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<TemplateClass> VirtualMachine::SetTemplateClassAddress(const char* Decl, const char* Name, size_t Size, uint64_t Flags)
		{
			VI_ASSERT(Decl != nullptr, "decl should be set");
			VI_ASSERT(Name != nullptr, "name should be set");
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class(%i) %i bytes", (int)Flags, (int)strlen(Decl));
#ifdef VI_ANGELSCRIPT
			int TypeId = Engine->RegisterObjectType(Decl, (asUINT)Size, (asDWORD)Flags);
			return FunctionFactory::ToReturn<TemplateClass>(TypeId, TemplateClass(this, Name));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<Enumeration> VirtualMachine::SetEnum(const char* Name)
		{
			VI_ASSERT(Name != nullptr, "name should be set");
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register enum %i bytes", (int)strlen(Name));
#ifdef VI_ANGELSCRIPT
			int TypeId = Engine->RegisterEnum(Name);
			return FunctionFactory::ToReturn<Enumeration>(TypeId, Enumeration(this, Engine->GetTypeInfoById(TypeId), TypeId));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::SetFunctionDef(const char* Decl)
		{
			VI_ASSERT(Decl != nullptr, "declaration should be set");
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register funcdef %i bytes", (int)strlen(Decl));
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RegisterFuncdef(Decl));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::SetTypeDef(const char* Type, const char* Decl)
		{
			VI_ASSERT(Type != nullptr, "type should be set");
			VI_ASSERT(Decl != nullptr, "declaration should be set");
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register funcdef %i bytes", (int)strlen(Decl));
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RegisterTypedef(Type, Decl));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::SetFunctionAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type)
		{
			VI_ASSERT(Decl != nullptr, "declaration should be set");
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register funcaddr(%i) %i bytes at 0x%" PRIXPTR, (int)Type, (int)strlen(Decl), (void*)Value);
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RegisterGlobalFunction(Decl, *Value, (asECallConvTypes)Type));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::SetPropertyAddress(const char* Decl, void* Value)
		{
			VI_ASSERT(Decl != nullptr, "declaration should be set");
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register global %i bytes at 0x%" PRIXPTR, (int)strlen(Decl), (void*)Value);
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RegisterGlobalProperty(Decl, Value));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::SetStringFactoryAddress(const char* Type, asIStringFactory* Factory)
		{
			VI_ASSERT(Type != nullptr, "declaration should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RegisterStringFactory(Type, Factory));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::GetPropertyByIndex(size_t Index, PropertyInfo* Info) const
		{
#ifdef VI_ANGELSCRIPT
			const char* Name = nullptr, * Namespace = nullptr;
			const char* ConfigGroup = nullptr;
			void* Pointer = nullptr;
			bool IsConst = false;
			asDWORD AccessMask = 0;
			int TypeId = 0;
			int Result = Engine->GetGlobalPropertyByIndex((asUINT)Index, &Name, &Namespace, &TypeId, &IsConst, &ConfigGroup, &Pointer, &AccessMask);
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
			return FunctionFactory::ToReturn(Result);
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<size_t> VirtualMachine::GetPropertyIndexByName(const char* Name) const
		{
			VI_ASSERT(Name != nullptr, "name should be set");
#ifdef VI_ANGELSCRIPT
			int R = Engine->GetGlobalPropertyIndexByName(Name);
			return FunctionFactory::ToReturn<size_t>(R, (size_t)R);
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<size_t> VirtualMachine::GetPropertyIndexByDecl(const char* Decl) const
		{
			VI_ASSERT(Decl != nullptr, "declaration should be set");
#ifdef VI_ANGELSCRIPT
			int R = Engine->GetGlobalPropertyIndexByDecl(Decl);
			return FunctionFactory::ToReturn<size_t>(R, (size_t)R);
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::SetLogCallback(void(*Callback)(const asSMessageInfo* Message, void* Object), void* Object)
		{
#ifdef VI_ANGELSCRIPT
			if (!Callback)
				return FunctionFactory::ToReturn(Engine->ClearMessageCallback());

			return FunctionFactory::ToReturn(Engine->SetMessageCallback(asFUNCTION(Callback), Object, asCALL_CDECL));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::Log(const char* Section, int Row, int Column, LogCategory Type, const char* Message)
		{
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->WriteMessage(Section, Row, Column, (asEMsgType)Type, Message));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::AssignObject(void* DestObject, void* SrcObject, const TypeInfo& Type)
		{
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->AssignScriptObject(DestObject, SrcObject, Type.GetTypeInfo()));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::RefCastObject(void* Object, const TypeInfo& FromType, const TypeInfo& ToType, void** NewPtr, bool UseOnlyImplicitCast)
		{
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RefCastObject(Object, FromType.GetTypeInfo(), ToType.GetTypeInfo(), NewPtr, UseOnlyImplicitCast));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::WriteMessage(const char* Section, int Row, int Column, LogCategory Type, const char* Message)
		{
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->WriteMessage(Section, Row, Column, (asEMsgType)Type, Message));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::GarbageCollect(GarbageCollector Flags, size_t NumIterations)
		{
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->GarbageCollect((asDWORD)Flags, (asUINT)NumIterations));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::PerformFullGarbageCollection()
		{
#ifdef VI_ANGELSCRIPT
			int R = Engine->GarbageCollect(asGC_DETECT_GARBAGE | asGC_FULL_CYCLE, 16);
			if (R < 0)
				return FunctionFactory::ToReturn(R);

			R = Engine->GarbageCollect(asGC_FULL_CYCLE, 16);
			return FunctionFactory::ToReturn(R);
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::NotifyOfNewObject(void* Object, const TypeInfo& Type)
		{
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->NotifyGarbageCollectorOfNewObject(Object, Type.GetTypeInfo()));
#else
			return VirtualError::NOT_SUPPORTED;
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
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::AddScriptSection(asIScriptModule* Module, const char* Name, const char* Code, size_t CodeLength, int LineOffset)
		{
			VI_ASSERT(Name != nullptr, "name should be set");
			VI_ASSERT(Code != nullptr, "code should be set");
			VI_ASSERT(Module != nullptr, "module should be set");
			VI_ASSERT(CodeLength > 0, "code should not be empty");
#ifdef VI_ANGELSCRIPT
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			Sections[Name] = Core::String(Code, CodeLength);
			Unique.Negate();

			return FunctionFactory::ToReturn(Module->AddScriptSection(Name, Code, CodeLength, LineOffset));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::GetTypeNameScope(const char** TypeName, const char** Namespace, size_t* NamespaceSize) const
		{
			VI_ASSERT(TypeName != nullptr && *TypeName != nullptr, "typename should be set");
#ifdef VI_ANGELSCRIPT
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
				return VirtualError::ALREADY_REGISTERED;
			}

			if (Namespace != nullptr)
				*Namespace = Value;
			if (NamespaceSize != nullptr)
				*NamespaceSize = Index - 1;

			*TypeName = Value + Index + 1;
			return Core::Optional::OK;
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::BeginGroup(const char* GroupName)
		{
			VI_ASSERT(GroupName != nullptr, "group name should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->BeginConfigGroup(GroupName));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::EndGroup()
		{
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->EndConfigGroup());
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::RemoveGroup(const char* GroupName)
		{
			VI_ASSERT(GroupName != nullptr, "group name should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->RemoveConfigGroup(GroupName));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::BeginNamespace(const char* Namespace)
		{
			VI_ASSERT(Namespace != nullptr, "namespace name should be set");
#ifdef VI_ANGELSCRIPT
			const char* Prev = Engine->GetDefaultNamespace();
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			if (Prev != nullptr)
				DefaultNamespace = Prev;
			else
				DefaultNamespace.clear();

			Unique.Negate();
			return FunctionFactory::ToReturn(Engine->SetDefaultNamespace(Namespace));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::BeginNamespaceIsolated(const char* Namespace, size_t DefaultMask)
		{
			VI_ASSERT(Namespace != nullptr, "namespace name should be set");
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
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<void> VirtualMachine::SetProperty(Features Property, size_t Value)
		{
			VI_ASSERT(Engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			return FunctionFactory::ToReturn(Engine->SetEngineProperty((asEEngineProp)Property, (asPWORD)Value));
#else
			return VirtualError::NOT_SUPPORTED;
#endif
		}
		ExpectsVM<size_t> VirtualMachine::GetSizeOfPrimitiveType(int TypeId) const
		{
			VI_ASSERT(Engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			int R = Engine->GetSizeOfPrimitiveType(TypeId);
			return FunctionFactory::ToReturn<size_t>(R, (size_t)R);
#else
			return VirtualError::NOT_SUPPORTED;
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
		Function VirtualMachine::GetFunctionByDecl(const char* Decl) const
		{
#ifdef VI_ANGELSCRIPT
			VI_ASSERT(Decl != nullptr, "declaration should be set");
			return Engine->GetGlobalFunctionByDecl(Decl);
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
		int VirtualMachine::GetTypeIdByDecl(const char* Decl) const
		{
			VI_ASSERT(Decl != nullptr, "declaration should be set");
#ifdef VI_ANGELSCRIPT
			return Engine->GetTypeIdByDecl(Decl);
#else
			return -1;
#endif
		}
		const char* VirtualMachine::GetTypeIdDecl(int TypeId, bool IncludeNamespace) const
		{
#ifdef VI_ANGELSCRIPT
			return Engine->GetTypeDeclaration(TypeId, IncludeNamespace);
#else
			return "";
#endif
		}
		Core::Option<Core::String> VirtualMachine::GetScriptSection(const Core::String& Section)
		{
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			auto It = Sections.find(Section);
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
		TypeInfo VirtualMachine::GetTypeInfoByName(const char* Name)
		{
			VI_ASSERT(Name != nullptr, "name should be set");
#ifdef VI_ANGELSCRIPT
			const char* TypeName = Name;
			const char* Namespace = nullptr;
			size_t NamespaceSize = 0;

			if (!GetTypeNameScope(&TypeName, &Namespace, &NamespaceSize))
				return Engine->GetTypeInfoByName(Name);

			BeginNamespace(Core::String(Namespace, NamespaceSize).c_str());
			asITypeInfo* Info = Engine->GetTypeInfoByName(TypeName);
			EndNamespace();

			return Info;
#else
			return TypeInfo(nullptr);
#endif
		}
		TypeInfo VirtualMachine::GetTypeInfoByDecl(const char* Decl) const
		{
			VI_ASSERT(Decl != nullptr, "declaration should be set");
#ifdef VI_ANGELSCRIPT
			return Engine->GetTypeInfoByDecl(Decl);
#else
			return TypeInfo(nullptr);
#endif
		}
		bool VirtualMachine::SetByteCodeTranslator(unsigned int Options)
		{
#ifdef ANGELSCRIPT_JIT
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			asCJITCompiler* Context = (asCJITCompiler*)Translator;
			bool TranslatorActive = (Options != (unsigned int)TranslationOptions::Disabled);
			VI_DELETE(asCJITCompiler, Context);

			Context = TranslatorActive ? VI_NEW(asCJITCompiler, Options) : nullptr;
			Translator = (asIScriptTranslator*)Context;
			if (!Engine)
				return true;

			Engine->SetEngineProperty(asEP_INCLUDE_JIT_INSTRUCTIONS, (asPWORD)TranslatorActive);
			Engine->SetJITCompiler(Context);
			return true;
#else
			return false;
#endif
		}
		void VirtualMachine::SetLibraryProperty(LibraryFeatures Property, size_t Value)
		{
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			LibrarySettings[Property] = Value;
		}
		void VirtualMachine::SetCodeGenerator(const Core::String& Name, GeneratorCallback&& Callback)
		{
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			if (Callback != nullptr)
				Generators[Name] = std::move(Callback);
			else
				Generators.erase(Name);
		}
		void VirtualMachine::SetPreserveSourceCode(bool Enabled)
		{
			SaveSources = Enabled;
		}
		void VirtualMachine::SetImports(unsigned int Opts)
		{
			Imports = Opts;
		}
		void VirtualMachine::SetTsImports(bool Enabled, const char* ImportSyntax)
		{
			Bindings::Imports::BindSyntax(this, Enabled, ImportSyntax);
		}
		void VirtualMachine::SetCache(bool Enabled)
		{
			Cached = Enabled;
		}
		void VirtualMachine::SetExceptionCallback(const std::function<void(ImmediateContext*)>& Callback)
		{
			GlobalException = Callback;
		}
		void VirtualMachine::SetDebugger(DebuggerContext* Context)
		{
			Core::UMutex<std::recursive_mutex> Unique1(Sync.General);
			VI_RELEASE(Debugger);
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
		void VirtualMachine::SetDefaultArrayType(const Core::String& Type)
		{
			VI_ASSERT(Engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			Engine->RegisterDefaultArrayType(Type.c_str());
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
				VI_RELEASE(Data.second);

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
		void VirtualMachine::SetCompilerErrorCallback(const WhenErrorCallback& Callback)
		{
			WhenError = Callback;
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
		void VirtualMachine::SetCompileCallback(const Core::String& Section, CompileCallback&& Callback)
		{
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			if (Callback != nullptr)
				Callbacks[Section] = std::move(Callback);
			else
				Callbacks.erase(Section);
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
			if (!Cached)
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
			if (!Cached)
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
		asIScriptModule* VirtualMachine::CreateScopedModule(const Core::String& Name)
		{
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_ASSERT(!Name.empty(), "name should not be empty");
#ifdef VI_ANGELSCRIPT
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			if (!Engine->GetModule(Name.c_str()))
				return Engine->GetModule(Name.c_str(), asGM_ALWAYS_CREATE);

			Core::String Result;
			while (Result.size() < 1024)
			{
				Result = Name + Core::ToString(Scope++);
				if (!Engine->GetModule(Result.c_str()))
					return Engine->GetModule(Result.c_str(), asGM_ALWAYS_CREATE);
			}
#endif
			return nullptr;
		}
		asIScriptModule* VirtualMachine::CreateModule(const Core::String& Name)
		{
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_ASSERT(!Name.empty(), "name should not be empty");
#ifdef VI_ANGELSCRIPT
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			return Engine->GetModule(Name.c_str(), asGM_ALWAYS_CREATE);
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
				*CurrentSize = (size_t)asCurrentSize;

			if (TotalDestroyed != nullptr)
				*TotalDestroyed = (size_t)asTotalDestroyed;

			if (TotalDetected != nullptr)
				*TotalDetected = (size_t)asTotalDetected;

			if (NewObjects != nullptr)
				*NewObjects = (size_t)asNewObjects;

			if (TotalNewDestroyed != nullptr)
				*TotalNewDestroyed = (size_t)asTotalNewDestroyed;
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
		bool VirtualMachine::GenerateCode(Compute::Preprocessor* Processor, const Core::String& Path, Core::String& InoutBuffer)
		{
			VI_ASSERT(Processor != nullptr, "preprocessor should be set");
			if (InoutBuffer.empty())
				return true;

			VI_TRACE("[vm] preprocessor source code generation at %s (%" PRIu64 " bytes)", Path.empty() ? "<anonymous>" : Path.c_str(), (uint64_t)InoutBuffer.size());
			{
				Core::UMutex<std::recursive_mutex> Unique(Sync.General);
				for (auto& Item : Generators)
				{
					VI_TRACE("[vm] generate source code for %s generator at %s (%" PRIu64 " bytes)", Item.first.c_str(), Path.empty() ? "<anonymous>" : Path.c_str(), (uint64_t)InoutBuffer.size());
					if (!Item.second(Processor, Path, InoutBuffer))
						return false;
				}
			}

			if (Processor->Process(Path, InoutBuffer))
				return true;

			VI_ERR("[vm] preprocessor generator has failed to generate souce code: %s", Path.empty() ? "<anonymous>" : Path.c_str());
			return false;
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
			auto AddFuncdef = [this, &Namespaces](asITypeInfo* FType, asUINT Index)
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
		const char* VirtualMachine::GetNamespace() const
		{
#ifdef VI_ANGELSCRIPT
			return Engine->GetDefaultNamespace();
#else
			return "";
#endif
		}
		Module VirtualMachine::GetModule(const char* Name)
		{
			VI_ASSERT(Engine != nullptr, "engine should be set");
			VI_ASSERT(Name != nullptr, "name should be set");
#ifdef VI_ANGELSCRIPT
			return Module(Engine->GetModule(Name, asGM_CREATE_IF_NOT_EXISTS));
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
		void VirtualMachine::SetModuleDirectory(const Core::String& Value)
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
		bool VirtualMachine::HasLibrary(const Core::String& Name, bool IsAddon)
		{
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			auto It = CLibraries.find(Name);
			if (It == CLibraries.end())
				return false;

			return It->second.IsAddon == IsAddon;
		}
		bool VirtualMachine::HasSystemAddon(const Core::String& Name)
		{
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			auto It = Addons.find(Name);
			if (It == Addons.end())
				return false;

			return It->second.Exposed;
		}
		bool VirtualMachine::HasAddon(const Core::String& Name)
		{
			return HasLibrary(Name, true);
		}
		bool VirtualMachine::IsNullable(int TypeId)
		{
			return TypeId == 0;
		}
		bool VirtualMachine::IsTranslatorSupported()
		{
#ifdef VI_JIT
			return true;
#else
			return false;
#endif
		}
		bool VirtualMachine::HasDebugger()
		{
			return Debugger != nullptr;
		}
		bool VirtualMachine::AddSystemAddon(const Core::String& Name, const Core::Vector<Core::String>& Dependencies, const AddonCallback& Callback)
		{
			VI_ASSERT(!Name.empty(), "name should not be empty");
			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			auto It = Addons.find(Name);
			if (It != Addons.end())
			{
				if (Callback || !Dependencies.empty())
				{
					It->second.Dependencies = Dependencies;
					It->second.Callback = Callback;
					It->second.Exposed = false;
				}
				else
					Addons.erase(It);
			}
			else
			{
				Addon Result;
				Result.Dependencies = Dependencies;
				Result.Callback = Callback;
				Addons.insert({ Name, std::move(Result) });
			}

			if (Name == "*")
				return true;

			auto& Lists = Addons["*"];
			Lists.Dependencies.push_back(Name);
			Lists.Exposed = false;
			return true;
		}
		bool VirtualMachine::ImportFile(const Core::String& Path, bool IsRemote, Core::String& Output)
		{
			if (!(Imports & (uint32_t)Imports::Files))
			{
				VI_ERR("[vm] file import is not allowed");
				return false;
			}
			else if (IsRemote && !(Imports & (uint32_t)Imports::Remotes))
			{
				VI_ERR("[vm] remote file import is not allowed");
				return false;
			}

			if (!IsRemote)
			{
				if (!Core::OS::File::IsExists(Path.c_str()))
					return false;

				if (!Core::Stringify::EndsWith(Path, ".as"))
					return ImportAddon(Path);
			}

			if (!Cached)
			{
				auto Data = Core::OS::File::ReadAsString(Path);
				if (!Data)
					return false;

				Output.assign(*Data);
				return true;
			}

			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			auto It = Files.find(Path);
			if (It != Files.end())
			{
				Output.assign(It->second);
				return true;
			}

			Unique.Negated([&Output, &Path]()
			{
				auto Data = Core::OS::File::ReadAsString(Path);
				if (Data)
					Output.assign(*Data);
			});
			Files.insert(std::make_pair(Path, Output));
			return true;
		}
		bool VirtualMachine::ImportCFunction(const Core::Vector<Core::String>& Sources, const Core::String& Func, const Core::String& Decl)
		{
			if (!(Imports & (uint32_t)Imports::CFunctions))
			{
				VI_ERR("[vm] cfunction import is not allowed");
				return false;
			}

			if (!Engine || Decl.empty() || Func.empty())
				return false;

			auto LoadFunction = [this, &Func, &Decl](CLibrary& Context, bool LogErrors) -> bool
			{
				auto Handle = Context.Functions.find(Func);
				if (Handle != Context.Functions.end())
					return true;

				auto FunctionHandle = Core::OS::Symbol::LoadFunction(Context.Handle, Func);
				if (!FunctionHandle)
				{
					if (LogErrors)
						VI_ERR("[vm] cannot load shared object function: %s", Func.c_str());

					return false;
				}

				FunctionPtr Function = (FunctionPtr)*FunctionHandle;
				if (!Function)
				{
					if (LogErrors)
						VI_ERR("[vm] cannot load shared object function: %s", Func.c_str());

					return false;
				}
#ifdef VI_ANGELSCRIPT
				VI_TRACE("[vm] register global funcaddr(%i) %i bytes at 0x%" PRIXPTR, (int)asCALL_CDECL, (int)Decl.size(), (void*)Function);
				if (Engine->RegisterGlobalFunction(Decl.c_str(), asFUNCTION(Function), asCALL_CDECL) < 0)
				{
					if (LogErrors)
						VI_ERR("[vm] cannot register shared object function: %s", Decl.c_str());

					return false;
				}

				Context.Functions.insert({ Func, { Decl, (void*)Function } });
				VI_DEBUG("[vm] load function %s", Func.c_str());
				return true;
#else
				return false;
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
				return LoadFunction(It->second, true);

			for (auto& Item : CLibraries)
			{
				if (LoadFunction(Item.second, false))
					return true;
			}

			VI_ERR("[vm] cannot load shared object function: %s (not found in any of loaded shared objects)", Func.c_str());
			return false;
		}
		bool VirtualMachine::ImportCLibrary(const Core::String& Path, bool IsAddon)
		{
			if (!(Imports & (uint32_t)Imports::CLibraries) && !Path.empty())
			{
				VI_ERR("[vm] clibraries import is not allowed");
				return false;
			}

			Core::String Name = GetLibraryName(Path);
			if (!Engine)
				return false;

			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			auto Core = CLibraries.find(Name);
			if (Core != CLibraries.end())
				return true;

			Unique.Negate();
			auto Handle = Core::OS::Symbol::Load(Path);
			if (!Handle)
			{
				VI_ERR("[vm] cannot load shared object: %s", Path.c_str());
				return false;
			}

			CLibrary Library;
			Library.Handle = *Handle;
			Library.IsAddon = IsAddon;

			if (Library.IsAddon && !InitializeAddon(Name, Library))
			{
				VI_ERR("[vm] cannot initialize addon library %s", Path.c_str());
				UninitializeAddon(Name, Library);
				Core::OS::Symbol::Unload(*Handle);
				return false;
			}

			Unique.Negate();
			CLibraries.insert({ Name, std::move(Library) });
			VI_DEBUG("[vm] load library %s", Path.c_str());
			return true;
		}
		bool VirtualMachine::ImportSystemAddon(const Core::String& Name)
		{
			if (!(Imports & (uint32_t)Imports::Addons))
			{
				VI_ERR("[vm] system addons import is not allowed: %s", Name.c_str());
				return false;
			}

			Core::String Target = Name;
			if (Core::Stringify::EndsWith(Target, ".as"))
				Target = Target.substr(0, Target.size() - 3);

			Core::UMutex<std::recursive_mutex> Unique(Sync.General);
			auto It = Addons.find(Target);
			if (It == Addons.end())
			{
				VI_ERR("[vm] cannot find script system addon %s", Name.c_str());
				return false;
			}

			if (It->second.Exposed)
				return true;

			Addon Base = It->second;
			It->second.Exposed = true;
			Unique.Negate();

			for (auto& Item : Base.Dependencies)
			{
				if (!ImportSystemAddon(Item))
				{
					VI_ERR("[vm] cannot load system addon %s for %s", Item.c_str(), Name.c_str());
					return false;
				}
			}

			VI_DEBUG("[vm] load system addon %s", Name.c_str());
			if (Base.Callback)
				Base.Callback(this);

			return true;
		}
		bool VirtualMachine::ImportAddon(const Core::String& Name)
		{
			return ImportCLibrary(Name.c_str(), true);
		}
		bool VirtualMachine::InitializeAddon(const Core::String& Path, CLibrary& Library)
		{
			auto ViInitializeHandle = Core::OS::Symbol::LoadFunction(Library.Handle, "ViInitialize");
			if (!ViInitializeHandle)
			{
				VI_ERR("[vm] potential addon library %s does not contain <ViInitialize> function", Path.c_str());
				return false;
			}

			auto ViInitialize = (int(*)(VirtualMachine*))*ViInitializeHandle;
			if (!ViInitialize)
			{
				VI_ERR("[vm] potential addon library %s does not contain <ViInitialize> function", Path.c_str());
				return false;
			}

			int Code = ViInitialize(this);
			if (Code != 0)
			{
				VI_ERR("[vm] addon library %s initialization failed: exit code %i", Path.c_str(), Code);
				return false;
			}

			VI_DEBUG("[vm] addon library %s initializated", Path.c_str());
			Library.Functions.insert({ "ViInitialize", { Core::String(), (void*)ViInitialize } });
			return true;
		}
		void VirtualMachine::UninitializeAddon(const Core::String& Name, CLibrary& Library)
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
		Core::Option<Core::String> VirtualMachine::GetSourceCodeAppendix(const char* Label, const Core::String& Code, uint32_t LineNumber, uint32_t ColumnNumber, size_t MaxLines)
		{
			if (MaxLines % 2 == 0)
				++MaxLines;

			VI_ASSERT(Label != nullptr, "label should be set");
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
		Core::Option<Core::String> VirtualMachine::GetSourceCodeAppendixByPath(const char* Label, const Core::String& Path, uint32_t LineNumber, uint32_t ColumnNumber, size_t MaxLines)
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
			Bindings::Registry::Cleanup();
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
		Core::String VirtualMachine::GetLibraryName(const Core::String& Path)
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
			VI_ASSERT(Base->Callbacks.Line, "context should be set");
			Base->Callbacks.Line(Base);
		}
		void VirtualMachine::ExceptionHandler(asIScriptContext* Context, void*)
		{
			ImmediateContext* Base = ImmediateContext::Get(Context);
			VI_ASSERT(Base != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
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
		const char* VirtualMachine::GetErrorNameInfo(VirtualError Code)
		{
#ifdef VI_ANGELSCRIPT
			size_t Index = (size_t)(-((int)Code));
			if (Index >= sizeof(errorNames) / sizeof(errorNames[0]))
				return nullptr;

			return errorNames[Index];
#else
			return nullptr;
#endif
		}
		ByteCodeLabel VirtualMachine::GetByteCodeInfo(uint8_t Code)
		{
#ifdef VI_ANGELSCRIPT
			auto& Source = asBCInfo[Code];
			ByteCodeLabel Label;
			Label.Name = Source.name;
			Label.Id = Code;
			Label.Args = asBCTypeSize[Source.type];
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
						return It->second(Core::Stringify::Text("WARN at line %i: %s%s", Info->row, Info->message, SourceCode ? SourceCode->c_str() : ""));
					else if (Info->type == asMSGTYPE_INFORMATION)
						return It->second(Core::Stringify::Text("INFO %s", Info->message));

					return It->second(Core::Stringify::Text("ERR at line %i: %s%s", Info->row, Info->message, SourceCode ? SourceCode->c_str() : ""));
				}
			}

			if (Info->type == asMSGTYPE_WARNING)
				VI_WARN("[compiler] %s at line %i: %s%s", Section, Info->row, Info->message, SourceCode ? SourceCode->c_str() : "");
			else if (Info->type == asMSGTYPE_INFORMATION)
				VI_INFO("[compiler] %s", Info->message);
			else if (Info->type == asMSGTYPE_ERROR)
				VI_ERR("[compiler] %s at line %i: %s%s", Section, Info->row, Info->message, SourceCode ? SourceCode->c_str() : "");
#endif
		}
		void VirtualMachine::RegisterAddons(VirtualMachine* Engine)
		{
			Engine->AddSystemAddon("ctypes", { }, Bindings::Registry::ImportCTypes);
			Engine->AddSystemAddon("any", { }, Bindings::Registry::ImportAny);
			Engine->AddSystemAddon("array", { "ctypes" }, Bindings::Registry::ImportArray);
			Engine->AddSystemAddon("complex", { }, Bindings::Registry::ImportComplex);
			Engine->AddSystemAddon("math", { }, Bindings::Registry::ImportMath);
			Engine->AddSystemAddon("string", { "array" }, Bindings::Registry::ImportString);
			Engine->AddSystemAddon("random", { "string" }, Bindings::Registry::ImportRandom);
			Engine->AddSystemAddon("dictionary", { "array", "string" }, Bindings::Registry::ImportDictionary);
			Engine->AddSystemAddon("exception", { "string" }, Bindings::Registry::ImportException);
			Engine->AddSystemAddon("mutex", { }, Bindings::Registry::ImportMutex);
			Engine->AddSystemAddon("thread", { "any", "string" }, Bindings::Registry::ImportThread);
			Engine->AddSystemAddon("buffers", { "string" }, Bindings::Registry::ImportBuffers);
			Engine->AddSystemAddon("promise", { "exception" }, Bindings::Registry::ImportPromise);
			Engine->AddSystemAddon("format", { "string" }, Bindings::Registry::ImportFormat);
			Engine->AddSystemAddon("decimal", { "string" }, Bindings::Registry::ImportDecimal);
			Engine->AddSystemAddon("uint128", { "decimal" }, Bindings::Registry::ImportUInt128);
			Engine->AddSystemAddon("uint256", { "uint128" }, Bindings::Registry::ImportUInt256);
			Engine->AddSystemAddon("variant", { "string", "decimal" }, Bindings::Registry::ImportVariant);
			Engine->AddSystemAddon("timestamp", { "string" }, Bindings::Registry::ImportTimestamp);
			Engine->AddSystemAddon("console", { "format" }, Bindings::Registry::ImportConsole);
			Engine->AddSystemAddon("schema", { "array", "string", "dictionary", "variant" }, Bindings::Registry::ImportSchema);
			Engine->AddSystemAddon("schedule", { "ctypes" }, Bindings::Registry::ImportSchedule);
			Engine->AddSystemAddon("clock", { }, Bindings::Registry::ImportClockTimer);
			Engine->AddSystemAddon("fs", { "string" }, Bindings::Registry::ImportFileSystem);
			Engine->AddSystemAddon("os", { "fs", "array" }, Bindings::Registry::ImportOS);
			Engine->AddSystemAddon("vertices", { }, Bindings::Registry::ImportVertices);
			Engine->AddSystemAddon("vectors", { }, Bindings::Registry::ImportVectors);
			Engine->AddSystemAddon("shapes", { "vectors" }, Bindings::Registry::ImportShapes);
			Engine->AddSystemAddon("key-frames", { "vectors", "string" }, Bindings::Registry::ImportKeyFrames);
			Engine->AddSystemAddon("regex", { "string" }, Bindings::Registry::ImportRegex);
			Engine->AddSystemAddon("crypto", { "string", "schema" }, Bindings::Registry::ImportCrypto);
			Engine->AddSystemAddon("codec", { "string" }, Bindings::Registry::ImportCodec);
			Engine->AddSystemAddon("geometric", { "vectors", "vertices", "shapes" }, Bindings::Registry::ImportGeometric);
			Engine->AddSystemAddon("preprocessor", { "string" }, Bindings::Registry::ImportPreprocessor);
			Engine->AddSystemAddon("physics", { "string", "geometric" }, Bindings::Registry::ImportPhysics);
			Engine->AddSystemAddon("audio", { "string", "vectors", "schema" }, Bindings::Registry::ImportAudio);
			Engine->AddSystemAddon("audio-effects", { "audio" }, Bindings::Registry::ImportAudioEffects);
			Engine->AddSystemAddon("audio-filters", { "audio" }, Bindings::Registry::ImportAudioFilters);
			Engine->AddSystemAddon("activity", { "string", "vectors" }, Bindings::Registry::ImportActivity);
			Engine->AddSystemAddon("graphics", { "activity", "string", "vectors", "vertices", "shapes", "key-frames" }, Bindings::Registry::ImportGraphics);
			Engine->AddSystemAddon("network", { "string", "array", "dictionary", "promise" }, Bindings::Registry::ImportNetwork);
			Engine->AddSystemAddon("http", { "schema", "fs", "promise", "regex", "network" }, Bindings::Registry::ImportHTTP);
			Engine->AddSystemAddon("smtp", { "promise", "network" }, Bindings::Registry::ImportSMTP);
			Engine->AddSystemAddon("postgresql", { "network", "schema" }, Bindings::Registry::ImportPostgreSQL);
			Engine->AddSystemAddon("mongodb", { "network", "schema" }, Bindings::Registry::ImportMongoDB);
			Engine->AddSystemAddon("gui-control", { "vectors", "schema", "array" }, Bindings::Registry::ImportUiControl);
			Engine->AddSystemAddon("gui-model", { "gui-control", }, Bindings::Registry::ImportUiModel);
			Engine->AddSystemAddon("gui-context", { "gui-model" }, Bindings::Registry::ImportUiContext);
			Engine->AddSystemAddon("engine", { "schema", "schedule", "key-frames", "fs", "graphics", "audio", "physics", "clock", "gui-context" }, Bindings::Registry::ImportEngine);
			Engine->AddSystemAddon("components", { "engine" }, Bindings::Registry::ImportComponents);
			Engine->AddSystemAddon("renderers", { "engine" }, Bindings::Registry::ImportRenderers);
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

		EventLoop::Callable::Callable(ImmediateContext* NewContext, void* NewPromise) noexcept : Context(NewContext), Promise(NewPromise)
		{
		}
		EventLoop::Callable::Callable(ImmediateContext* NewContext, FunctionDelegate&& NewDelegate, ArgsCallback&& NewOnArgs, ArgsCallback&& NewOnReturn) noexcept : Delegate(std::move(NewDelegate)), OnArgs(std::move(NewOnArgs)), OnReturn(std::move(NewOnReturn)), Context(NewContext), Promise(nullptr)
		{
		}
		EventLoop::Callable::Callable(const Callable& Other) noexcept : Delegate(Other.Delegate), OnArgs(Other.OnArgs), OnReturn(Other.OnReturn), Context(Other.Context), Promise(Other.Promise)
		{
		}
		EventLoop::Callable::Callable(Callable&& Other) noexcept : Delegate(std::move(Other.Delegate)), OnArgs(std::move(Other.OnArgs)), OnReturn(std::move(Other.OnReturn)), Context(Other.Context), Promise(Other.Promise)
		{
			Other.Context = nullptr;
			Other.Promise = nullptr;
		}
		EventLoop::Callable& EventLoop::Callable::operator= (const Callable& Other) noexcept
		{
			if (this == &Other)
				return *this;

			Delegate = Other.Delegate;
			OnArgs = Other.OnArgs;
			OnReturn = Other.OnReturn;
			Context = Other.Context;
			Promise = Other.Promise;
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
			Promise = Other.Promise;
			Other.Context = nullptr;
			Other.Promise = nullptr;
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

		static thread_local EventLoop* EventHandle = nullptr;
		EventLoop::EventLoop() noexcept : Wake(false)
		{
		}
		void EventLoop::OnNotification(ImmediateContext* Context, void* Promise)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			if (OnEnqueue)
				OnEnqueue(Context);

			Core::UMutex<std::mutex> Unique(Mutex);
			Queue.push(Callable(Context, Promise));
			Waitable.notify_one();
		}
		void EventLoop::OnCallback(ImmediateContext* Context, FunctionDelegate&& Delegate, ArgsCallback&& OnArgs, ArgsCallback&& OnReturn)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			VI_ASSERT(Delegate.IsValid(), "delegate should be valid");
			if (OnEnqueue)
				OnEnqueue(Context);

			Core::UMutex<std::mutex> Unique(Mutex);
			Queue.push(Callable(Context, std::move(Delegate), std::move(OnArgs), std::move(OnReturn)));
			Waitable.notify_one();
		}
		void EventLoop::Wakeup()
		{
			Core::UMutex<std::mutex> Unique(Mutex);
			Wake = true;
			Waitable.notify_one();
		}
		void EventLoop::When(ArgsCallback&& Callback)
		{
			OnEnqueue = std::move(Callback);
		}
		void EventLoop::Listen(ImmediateContext* Context)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			Context->SetNotificationResolverCallback(std::bind(&EventLoop::OnNotification, this, std::placeholders::_1, std::placeholders::_2));
			Context->SetCallbackResolverCallback(std::bind(&EventLoop::OnCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		}
		void EventLoop::Enqueue(ImmediateContext* Context, void* Promise)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			if (OnEnqueue)
				OnEnqueue(Context);

			Core::UMutex<std::mutex> Unique(Mutex);
			Queue.push(Callable(Context, Promise));
			Waitable.notify_one();
		}
		void EventLoop::Enqueue(FunctionDelegate&& Delegate, ArgsCallback&& OnArgs, ArgsCallback&& OnReturn)
		{
			VI_ASSERT(Delegate.IsValid(), "delegate should be valid");
			if (OnEnqueue)
				OnEnqueue(Delegate.Context);

			Core::UMutex<std::mutex> Unique(Mutex);
			Queue.push(Callable(Delegate.Context, std::move(Delegate), std::move(OnArgs), std::move(OnReturn)));
			Waitable.notify_one();
		}
		bool EventLoop::Poll(ImmediateContext* Context, uint64_t TimeoutMs)
		{
			VI_ASSERT(Context != nullptr, "context should be set");
			if (!Bindings::Promise::IsContextBusy(Context) && Queue.empty())
				return false;

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
				auto* Queue = Core::Schedule::Get();
				if (!Queue->GetPolicy().Parallel || !Queue->IsActive())
					return false;
			}

			std::unique_lock<std::mutex> Unique(Mutex);
			Waitable.wait_for(Unique, std::chrono::milliseconds(TimeoutMs), [this]() { return !Queue.empty() || Wake; });
			Wake = false;
			return true;
		}
		size_t EventLoop::Dequeue(VirtualMachine* VM)
		{
			VI_ASSERT(VM != nullptr, "virtual machine should be set");
			std::unique_lock<std::mutex> Unique(Mutex);
			size_t Executions = 0;

			while (!Queue.empty())
			{
				Callable Next = std::move(Queue.front());
				Queue.pop();
				Unique.unlock();

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
						ExecutingContext->ExecuteCall(Next.Delegate.Callable(), std::move(Next.OnArgs)).When([VM, InitiatorContext, ExecutingContext, OnReturn = std::move(OnReturn)](ExpectsVM<Execution>&&) mutable
						{
							OnReturn(ExecutingContext);
							if (ExecutingContext != InitiatorContext)
								VM->ReturnContext(ExecutingContext);
						});
					}
					else
					{
						ExecutingContext->ExecuteCall(Next.Delegate.Callable(), std::move(Next.OnArgs)).When([VM, InitiatorContext, ExecutingContext](ExpectsVM<Execution>&&) mutable
						{
							if (ExecutingContext != InitiatorContext)
								VM->ReturnContext(ExecutingContext);
						});
					}
					++Executions;
				}
				else if (Next.IsNotification())
				{
					Bindings::Promise* Base = (Bindings::Promise*)Next.Promise;
					if (InitiatorContext->IsSuspended())
						InitiatorContext->Resume();
					if (Base != nullptr)
						Base->Release();
					++Executions;
				}
				Unique.lock();
			}

			if (Executions > 0)
				VI_TRACE("[vm] loop process %" PRIu64 " events", (uint64_t)Executions);
			return Executions;
		}
		void EventLoop::Set(EventLoop* ForCurrentThread)
		{
			EventHandle = ForCurrentThread;
		}
		EventLoop* EventLoop::Get()
		{
			return EventHandle;
		}
	}
}