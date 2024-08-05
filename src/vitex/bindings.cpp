#include "bindings.h"
#include "internal/types.hpp"
#include <sstream>
#ifdef VI_BINDINGS
#include "network.h"
#include "network/http.h"
#include "network/smtp.h"
#include "network/ldb.h"
#include "network/pdb.h"
#include "network/mdb.h"
#include "layer/processors.h"
#endif
#ifdef VI_ANGELSCRIPT
#include <angelscript.h>
#endif

namespace
{
#ifdef VI_ANGELSCRIPT
	class StringFactory : public asIStringFactory
	{
	private:
		static StringFactory* Base;

	public:
		Vitex::Core::UnorderedMap<Vitex::Core::String, std::atomic<int32_t>> Cache;

	public:
		StringFactory()
		{
		}
		~StringFactory()
		{
			VI_ASSERT(Cache.size() == 0, "some strings are still in use");
		}
		const void* GetStringConstant(const char* Buffer, asUINT Length)
		{
			VI_ASSERT(Buffer != nullptr, "buffer must not be null");

			asAcquireSharedLock();
			std::string_view Source(Buffer, Length);
			auto It = Cache.find(Vitex::Core::KeyLookupCast(Source));
			if (It != Cache.end())
			{
				It->second++;
				asReleaseSharedLock();
				return reinterpret_cast<const void*>(&It->first);
			}

			asReleaseSharedLock();
			asAcquireExclusiveLock();
			It = Cache.insert(std::make_pair(std::move(Source), 1)).first;
			asReleaseExclusiveLock();
			return reinterpret_cast<const void*>(&It->first);
		}
		int ReleaseStringConstant(const void* Source)
		{
			VI_ASSERT(Source != nullptr, "source must not be null");
			asAcquireSharedLock();
			auto It = Cache.find(*reinterpret_cast<const Vitex::Core::String*>(Source));
			if (It == Cache.end())
			{
				asReleaseSharedLock();
				return asERROR;
			}
			else if (--It->second > 0)
			{
				asReleaseSharedLock();
				return asSUCCESS;
			}

			asReleaseSharedLock();
			asAcquireExclusiveLock();
			Cache.erase(It);
			asReleaseExclusiveLock();
			return asSUCCESS;
		}
		int GetRawStringData(const void* Source, char* Buffer, asUINT* Length) const
		{
			VI_ASSERT(Source != nullptr, "source must not be null");
			if (Length != nullptr)
				*Length = (asUINT)reinterpret_cast<const Vitex::Core::String*>(Source)->length();

			if (Buffer != nullptr)
				memcpy(Buffer, reinterpret_cast<const Vitex::Core::String*>(Source)->c_str(), reinterpret_cast<const Vitex::Core::String*>(Source)->length());

			return asSUCCESS;
		}

	public:
		static StringFactory* Get()
		{
			if (!Base)
				Base = Vitex::Core::Memory::New<StringFactory>();

			return Base;
		}
		static void Free()
		{
			if (Base != nullptr && Base->Cache.empty())
				Vitex::Core::Memory::Delete(Base);
		}
	};
	StringFactory* StringFactory::Base = nullptr;
#endif
	struct FileLink : public Vitex::Core::FileEntry
	{
		Vitex::Core::String Path;
	};
}

namespace Vitex
{
	namespace Scripting
	{
		namespace Bindings
		{
			void PointerToHandleCast(void* From, void** To, int TypeId)
			{
				if (!(TypeId & (size_t)TypeId::OBJHANDLE))
					return;

				if (!(TypeId & (size_t)TypeId::MASK_OBJECT))
					return;

				*To = From;
			}
			void HandleToHandleCast(void* From, void** To, int TypeId)
			{
				if (!(TypeId & (size_t)TypeId::OBJHANDLE))
					return;

				if (!(TypeId & (size_t)TypeId::MASK_OBJECT))
					return;

				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return;

				auto TypeInfo = VM->GetTypeInfoById(TypeId);
				VM->RefCastObject(From, TypeInfo, TypeInfo, To);
			}
			void* HandleToPointerCast(void* From, int TypeId)
			{
				if (!(TypeId & (size_t)TypeId::OBJHANDLE))
					return nullptr;

				if (!(TypeId & (size_t)TypeId::MASK_OBJECT))
					return nullptr;

				return *reinterpret_cast<void**>(From);
			}

			std::string_view String::ImplCastStringView(Core::String& Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return std::string_view(Context ? Context->CopyString(Base) : Base);
			}
			void String::Create(Core::String* Base)
			{
				new(Base) Core::String();
			}
			void String::CreateCopy1(Core::String* Base, const Core::String& Other)
			{
				new(Base) Core::String(Other);
			}
			void String::CreateCopy2(Core::String* Base, const std::string_view& Other)
			{
				new(Base) Core::String(Other);
			}
			void String::Destroy(Core::String* Base)
			{
				Base->~basic_string();
			}
			void String::PopBack(Core::String& Base)
			{
				if (Base.empty())
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
				else
					Base.pop_back();
			}
			Core::String& String::Replace1(Core::String& Other, const Core::String& From, const Core::String& To, size_t Start)
			{
				return Core::Stringify::Replace(Other, From, To, Start);
			}
			Core::String& String::ReplacePart1(Core::String& Other, size_t Start, size_t End, const Core::String& To)
			{
				return Core::Stringify::ReplacePart(Other, Start, End, To);
			}
			bool String::StartsWith1(const Core::String& Other, const Core::String& Value, size_t Offset)
			{
				return Core::Stringify::StartsWith(Other, Value, Offset);
			}
			bool String::EndsWith1(const Core::String& Other, const Core::String& Value)
			{
				return Core::Stringify::EndsWith(Other, Value);
			}
			Core::String& String::Replace2(Core::String& Other, const std::string_view& From, const std::string_view& To, size_t Start)
			{
				return Core::Stringify::Replace(Other, From, To, Start);
			}
			Core::String& String::ReplacePart2(Core::String& Other, size_t Start, size_t End, const std::string_view& To)
			{
				return Core::Stringify::ReplacePart(Other, Start, End, To);
			}
			bool String::StartsWith2(const Core::String& Other, const std::string_view& Value, size_t Offset)
			{
				return Core::Stringify::StartsWith(Other, Value, Offset);
			}
			bool String::EndsWith2(const Core::String& Other, const std::string_view& Value)
			{
				return Core::Stringify::EndsWith(Other, Value);
			}
			Core::String String::Substring1(Core::String& Base, size_t Offset)
			{
				return Base.substr(Offset);
			}
			Core::String String::Substring2(Core::String& Base, size_t Offset, size_t Size)
			{
				return Base.substr(Offset, Size);
			}
			Core::String String::FromPointer(void* Pointer)
			{
				return Core::Stringify::Text("0x%" PRIXPTR, Pointer);
			}
			Core::String String::FromBuffer(const char* Buffer, size_t MaxSize)
			{
				if (!Buffer)
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_NULLPOINTER));
					return Core::String();
				}

				size_t Size = strnlen(Buffer, MaxSize);
				return Core::String(Buffer, Size);
			}
			char* String::Index(Core::String& Base, size_t Offset)
			{
				if (Offset >= Base.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				return &Base[Offset];
			}
			char* String::Front(Core::String& Base)
			{
				if (Base.empty())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				char& Target = Base.front();
				return &Target;
			}
			char* String::Back(Core::String& Base)
			{
				if (Base.empty())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				char& Target = Base.back();
				return &Target;
			}
			Array* String::Split(Core::String& Base, const std::string_view& Delimiter)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				asITypeInfo* ArrayType = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@").GetTypeInfo();
				Array* Array = Array::Create(ArrayType);

				int Offset = 0, Prev = 0, Count = 0;
				while ((Offset = (int)Base.find(Delimiter, Prev)) != (int)Core::String::npos)
				{
					Array->Resize(Array->Size() + 1);
					((Core::String*)Array->At(Count))->assign(&Base[Prev], Offset - Prev);
					Prev = Offset + (int)Delimiter.size();
					Count++;
				}

				Array->Resize(Array->Size() + 1);
				((Core::String*)Array->At(Count))->assign(&Base[Prev]);
				return Array;
			}

			Core::String StringView::ImplCastString(std::string_view& Base)
			{
				return Core::String(Base);
			}
			void StringView::Create(std::string_view* Base)
			{
				new(Base) std::string_view();
			}
			void StringView::CreateCopy(std::string_view* Base, Core::String& Other)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				new(Base) std::string_view(Context ? Context->CopyString(Other) : Other);
			}
			void StringView::Assign(std::string_view* Base, Core::String& Other)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				*Base = (Context ? Context->CopyString(Other) : Other);
			}
			void StringView::Destroy(std::string_view* Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr)
					Context->InvalidateString(*Base);
				Base->~basic_string_view();
			}
			bool StringView::StartsWith(const std::string_view& Other, const std::string_view& Value, size_t Offset)
			{
				return Core::Stringify::StartsWith(Other, Value, Offset);
			}
			bool StringView::EndsWith(const std::string_view& Other, const std::string_view& Value)
			{
				return Core::Stringify::EndsWith(Other, Value);
			}
			int StringView::Compare1(std::string_view& Base, const Core::String& Other)
			{
				return Base.compare(Other);
			}
			int StringView::Compare2(std::string_view& Base, const std::string_view& Other)
			{
				return Base.compare(Other);
			}
			Core::String StringView::Append1(const std::string_view& Base, const std::string_view& Other)
			{
				Core::String Result = Core::String(Base);
				Result.append(Other);
				return Result;
			}
			Core::String StringView::Append2(const std::string_view& Base, const Core::String& Other)
			{
				Core::String Result = Core::String(Base);
				Result.append(Other);
				return Result;
			}
			Core::String StringView::Append3(const Core::String& Other, const std::string_view& Base)
			{
				Core::String Result = Other;
				Result.append(Base);
				return Result;
			}
			Core::String StringView::Append4(const std::string_view& Base, char Other)
			{
				Core::String Result = Core::String(Base);
				Result.append(1, Other);
				return Result;
			}
			Core::String StringView::Append5(char Other, const std::string_view& Base)
			{
				Core::String Result = Core::String(1, Other);
				Result.append(Base);
				return Result;
			}
			Core::String StringView::Substring1(std::string_view& Base, size_t Offset)
			{
				return Core::String(Base.substr(Offset));
			}
			Core::String StringView::Substring2(std::string_view& Base, size_t Offset, size_t Size)
			{
				return Core::String(Base.substr(Offset, Size));
			}
			size_t StringView::ReverseFind1(std::string_view& Base, const std::string_view& Other, size_t Offset)
			{
				return Base.rfind(Other, Offset);
			}
			size_t StringView::ReverseFind2(std::string_view& Base, char Other, size_t Offset)
			{
				return Base.rfind(Other, Offset);
			}
			size_t StringView::Find1(std::string_view& Base, const std::string_view& Other, size_t Offset)
			{
				return Base.find(Other, Offset);
			}
			size_t StringView::Find2(std::string_view& Base, char Other, size_t Offset)
			{
				return Base.find(Other, Offset);
			}
			size_t StringView::FindFirstOf(std::string_view& Base, const std::string_view& Other, size_t Offset)
			{
				return Base.find_first_of(Other, Offset);
			}
			size_t StringView::FindFirstNotOf(std::string_view& Base, const std::string_view& Other, size_t Offset)
			{
				return Base.find_first_not_of(Other, Offset);
			}
			size_t StringView::FindLastOf(std::string_view& Base, const std::string_view& Other, size_t Offset)
			{
				return Base.find_last_of(Other, Offset);
			}
			size_t StringView::FindLastNotOf(std::string_view& Base, const std::string_view& Other, size_t Offset)
			{
				return Base.find_last_not_of(Other, Offset);
			}
			std::string_view StringView::FromBuffer(const char* Buffer, size_t MaxSize)
			{
				if (!Buffer)
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_NULLPOINTER));
					return std::string_view();
				}

				size_t Size = strnlen(Buffer, MaxSize);
				return std::string_view(Buffer, Size);
			}
			char* StringView::Index(std::string_view& Base, size_t Offset)
			{
				if (Offset >= Base.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				char* Data = (char*)Base.data();
				return &Data[Offset];
			}
			char* StringView::Front(std::string_view& Base)
			{
				if (Base.empty())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				const char& Target = Base.front();
				return (char*)&Target;
			}
			char* StringView::Back(std::string_view& Base)
			{
				if (Base.empty())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				const char& Target = Base.back();
				return (char*)&Target;
			}
			Array* StringView::Split(std::string_view& Base, const std::string_view& Delimiter)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				asITypeInfo* ArrayType = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@").GetTypeInfo();
				Array* Array = Array::Create(ArrayType);

				int Offset = 0, Prev = 0, Count = 0;
				while ((Offset = (int)Base.find(Delimiter, Prev)) != (int)std::string::npos)
				{
					Array->Resize(Array->Size() + 1);
					((Core::String*)Array->At(Count))->assign(&Base[Prev], Offset - Prev);
					Prev = Offset + (int)Delimiter.size();
					Count++;
				}

				Array->Resize(Array->Size() + 1);
				((Core::String*)Array->At(Count))->assign(&Base[Prev]);
				return Array;
			}

			float Math::FpFromIEEE(uint32_t Raw)
			{
				return *reinterpret_cast<float*>(&Raw);
			}
			uint32_t Math::FpToIEEE(float Value)
			{
				return *reinterpret_cast<uint32_t*>(&Value);
			}
			double Math::FpFromIEEE(as_uint64_t Raw)
			{
				return *reinterpret_cast<double*>(&Raw);
			}
			as_uint64_t Math::FpToIEEE(double Value)
			{
				return *reinterpret_cast<as_uint64_t*>(&Value);
			}
			bool Math::CloseTo(float A, float B, float Epsilon)
			{
				if (A == B)
					return true;

				float diff = fabsf(A - B);
				if ((A == 0 || B == 0) && (diff < Epsilon))
					return true;

				return diff / (fabs(A) + fabs(B)) < Epsilon;
			}
			bool Math::CloseTo(double A, double B, double Epsilon)
			{
				if (A == B)
					return true;

				double diff = fabs(A - B);
				if ((A == 0 || B == 0) && (diff < Epsilon))
					return true;

				return diff / (fabs(A) + fabs(B)) < Epsilon;
			}

			void Imports::BindSyntax(VirtualMachine* VM, bool Enabled, const std::string_view& Syntax)
			{
				VI_ASSERT(VM != nullptr, "vm should be set");
				if (Enabled)
					VM->SetCodeGenerator("import-syntax", std::bind(&Bindings::Imports::GeneratorCallback, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, Syntax));
				else
					VM->SetCodeGenerator("import-syntax", nullptr);
			}
			ExpectsVM<void> Imports::GeneratorCallback(Compute::Preprocessor* Base, const std::string_view& Path, Core::String& Code, const std::string_view& Syntax)
			{
				return Parser::ReplaceDirectivePreconditions(Syntax, Code, [Base, &Path](const std::string_view& Text) -> ExpectsVM<Core::String>
				{
					Core::Vector<std::pair<Core::String, Core::TextSettle>> Includes = Core::Stringify::FindInBetweenInCode(Text, "\"", "\"");
					Core::String Output;

					for (auto& Include : Includes)
					{
						auto Result = Base->ResolveFile(Path, Core::Stringify::Trim(Include.first));
						if (!Result)
							return VirtualException(VirtualError::INVALID_DECLARATION, std::move(Result.Error().message()));
						Output += *Result;
					}

					size_t Prev = Core::Stringify::CountLines(Text);
					size_t Next = Core::Stringify::CountLines(Output);
					size_t Diff = (Next < Prev ? Prev - Next : 0);
					Output.append(Diff, '\n');
					return Output;
				});
			}

			void Tags::BindSyntax(VirtualMachine* VM, bool Enabled, const TagCallback& Callback)
			{
				VI_ASSERT(VM != nullptr, "vm should be set");
				if (Enabled)
					VM->SetCodeGenerator("tags-syntax", std::bind(&Bindings::Tags::GeneratorCallback, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, VM, Callback));
				else
					VM->SetCodeGenerator("tags-syntax", nullptr);
			}
			ExpectsVM<void> Tags::GeneratorCallback(Compute::Preprocessor* Base, const std::string_view& Path, Core::String& Code, VirtualMachine* VM, const TagCallback& Callback)
			{
#ifdef VI_ANGELSCRIPT
				if (!Callback)
					return Core::Expectation::Met;

				asIScriptEngine* Engine = VM->GetEngine();
				Core::Vector<TagDeclaration> Declarations;
				TagDeclaration Tag;
				size_t Offset = 0;

				while (Offset < Code.size())
				{
					asUINT Length = 0;
					asETokenClass Type = Engine->ParseToken(&Code[Offset], Code.size() - Offset, &Length);
					if (Type == asTC_COMMENT || Type == asTC_WHITESPACE)
					{
						Offset += Length;
						continue;
					}

					std::string_view Token = std::string_view(&Code[Offset], Length);
					if (Token == "shared" || Token == "abstract" || Token == "mixin" || Token == "external")
					{
						Offset += Length;
						continue;
					}

					if (Tag.Class.empty() && (Token == "class" || Token == "interface"))
					{
						do
						{
							Offset += Length;
							if (Offset >= Code.size())
							{
								Type = asTC_UNKNOWN;
								break;
							}
							Type = Engine->ParseToken(&Code[Offset], Code.size() - Offset, &Length);
						} while (Type == asTC_COMMENT || Type == asTC_WHITESPACE);

						if (Type == asTC_IDENTIFIER)
						{
							Tag.Class = std::string_view(Code).substr(Offset, Length);
							while (Offset < Code.length())
							{
								Engine->ParseToken(&Code[Offset], Code.size() - Offset, &Length);
								if (Code[Offset] == '{')
								{
									Offset += Length;
									break;
								}
								else if (Code[Offset] == ';')
								{
									Tag.Class = std::string_view();
									Offset += Length;
									break;
								}
								Offset += Length;
							}
						}

						continue;
					}

					if (!Tag.Class.empty() && Token == "}")
					{
						Tag.Class = std::string_view();
						Offset += Length;
						continue;
					}

					if (Token == "namespace")
					{
						do
						{
							Offset += Length;
							Type = Engine->ParseToken(&Code[Offset], Code.size() - Offset, &Length);
						} while (Type == asTC_COMMENT || Type == asTC_WHITESPACE);

						if (!Tag.Namespace.empty())
							Tag.Namespace += "::";

						Tag.Namespace += Code.substr(Offset, Length);
						while (Offset < Code.length())
						{
							Engine->ParseToken(&Code[Offset], Code.size() - Offset, &Length);
							if (Code[Offset] == '{')
							{
								Offset += Length;
								break;
							}
							Offset += Length;
						}
						continue;
					}

					if (!Tag.Namespace.empty() && Token == "}")
					{
						size_t Index = Tag.Namespace.rfind("::");
						if (Index != std::string::npos)
							Tag.Namespace.erase(Index);
						else
							Tag.Namespace.clear();
						Offset += Length;
						continue;
					}

					if (Token == "[")
					{
						Offset = ExtractField(VM, Code, Offset, Tag);
						ExtractDeclaration(VM, Code, Offset, Tag);
					}

					Length = 0;
					while (Offset < Code.length() && Code[Offset] != ';' && Code[Offset] != '{')
					{
						Engine->ParseToken(&Code[Offset], Code.size() - Offset, &Length);
						Offset += Length;
					}

					if (Offset < Code.length() && Code[Offset] == '{')
					{
						int Level = 1; ++Offset;
						while (Level > 0 && Offset < Code.size())
						{
							if (Engine->ParseToken(&Code[Offset], Code.size() - Offset, &Length) == asTC_KEYWORD)
							{
								if (Code[Offset] == '{')
									Level++;
								else if (Code[Offset] == '}')
									Level--;
							}
							Offset += Length;
						}
					}
					else
						++Offset;

					if (Tag.Type == TagType::Unknown)
						continue;

					auto& Result = Declarations.emplace_back();
					Result.Class = Tag.Class;
					Result.Name = Tag.Name;
					Result.Declaration = std::move(Tag.Declaration);
					Result.Namespace = Tag.Namespace;
					Result.Directives = std::move(Tag.Directives);
					Result.Type = Tag.Type;

					Core::Stringify::Trim(Result.Declaration);
					Tag.Name = std::string_view();
					Tag.Type = TagType::Unknown;
				}

				Callback(VM, std::move(Declarations));
				return Core::Expectation::Met;
#else
				return VirtualException(VirtualError::NOT_SUPPORTED, "tag generator requires engine support");
#endif
			}
			size_t Tags::ExtractField(VirtualMachine* VM, Core::String& Code, size_t Offset, TagDeclaration& Tag)
			{
#ifdef VI_ANGELSCRIPT
				asIScriptEngine* Engine = VM->GetEngine();
				for (;;)
				{
					Core::String Declaration;
					Code[Offset++] = ' ';

					int Level = 1; asUINT Length = 0;
					while (Level > 0 && Offset < Code.size())
					{
						asETokenClass Type = Engine->ParseToken(&Code[Offset], Code.size() - Offset, &Length);
						if (Type == asTC_KEYWORD)
						{
							if (Code[Offset] == '[')
								Level++;
							else if (Code[Offset] == ']')
								Level--;
						}
						
						if (Level > 0)
							Declaration.append(&Code[Offset], Length);

						if (Type != asTC_WHITESPACE)
						{
							char* Buffer = &Code[Offset];
							for (asUINT i = 0; i < Length; i++)
							{
								if (!Core::Stringify::IsWhitespace(Buffer[i]))
									Buffer[i] = ' ';
							}
						}
						Offset += Length;
					}
					AppendDirective(Tag, Core::Stringify::Trim(Declaration));

					asETokenClass Type = Engine->ParseToken(&Code[Offset], Code.size() - Offset, &Length);
					while (Type == asTC_COMMENT || Type == asTC_WHITESPACE)
					{
						Offset += Length;
						Type = Engine->ParseToken(&Code[Offset], Code.size() - Offset, &Length);
					}

					if (Code[Offset] != '[')
						break;
				}
#endif
				return Offset;
			}
			void Tags::ExtractDeclaration(VirtualMachine* VM, Core::String& Code, size_t Offset, TagDeclaration& Tag)
			{
#ifdef VI_ANGELSCRIPT
				asIScriptEngine* Engine = VM->GetEngine();
				std::string_view Token;
				asUINT Length = 0;
				asETokenClass Type = asTC_WHITESPACE;

				do
				{
					Offset += Length;
					Type = Engine->ParseToken(&Code[Offset], Code.size() - Offset, &Length);
					Token = std::string_view(&Code[Offset], Length);
				} while (Type == asTC_WHITESPACE || Type == asTC_COMMENT || Token == "private" || Token == "protected" || Token == "shared" || Token == "external" || Token == "final" || Token == "abstract");

				if (Type != asTC_KEYWORD && Type != asTC_IDENTIFIER)
					return;

				Token = std::string_view(&Code[Offset], Length);
				if (Token != "interface" && Token != "class" && Token != "enum")
				{
					int NestedParenthesis = 0;
					bool HasParenthesis = false;
					Tag.Declaration.append(&Code[Offset], Length);
					Offset += Length;
					for (; Offset < Code.size();)
					{
						Type = Engine->ParseToken(&Code[Offset], Code.size() - Offset, &Length);
						Token = std::string_view(&Code[Offset], Length);
						if (Type == asTC_KEYWORD)
						{
							if (Token == "{" && NestedParenthesis == 0)
							{
								if (!HasParenthesis)
								{
									Tag.Declaration = Tag.Name;
									Tag.Type = TagType::PropertyFunction;
								}
								else
									Tag.Type = TagType::Function;
								return;
							}
							if ((Token == "=" && !HasParenthesis) || Token == ";")
							{
								if (!HasParenthesis)
								{
									Tag.Declaration = Tag.Name;
									Tag.Type = TagType::Variable;
								}
								else
									Tag.Type = TagType::NotType;
								return;
							}
							else if (Token == "(")
							{
								NestedParenthesis++;
								HasParenthesis = true;
							}
							else if (Token == ")")
								NestedParenthesis--;
						}
						else if (Tag.Name.empty() && Type == asTC_IDENTIFIER)
							Tag.Name = Token;

						if (!HasParenthesis || NestedParenthesis > 0 || Type != asTC_IDENTIFIER || (Token != "final" && Token != "override"))
							Tag.Declaration += Token;
						Offset += Length;
					}
				}
				else
				{
					do
					{
						Offset += Length;
						Type = Engine->ParseToken(&Code[Offset], Code.size() - Offset, &Length);
					} while (Type == asTC_WHITESPACE || Type == asTC_COMMENT);

					if (Type == asTC_IDENTIFIER)
					{
						Tag.Type = TagType::Type;
						Tag.Declaration = std::string_view(&Code[Offset], Length);
						Offset += Length;
						return;
					}
				}
#endif
			}
			void Tags::AppendDirective(TagDeclaration& Tag, Core::String& Directive)
			{
				TagDirective Result;
				size_t Where = Directive.find('(');
				if (Where == Core::String::npos || Directive.back() != ')')
				{
					Result.Name = std::move(Directive);
					Tag.Directives.emplace_back(std::move(Result));
					return;
				}

				Core::Vector<Core::String> Args;
				std::string_view Data = std::string_view(Directive).substr(Where + 1, Directive.size() - Where - 2);
				Result.Name = Directive.substr(0, Where);
				Where = 0;

				size_t Last = 0;
				while (Where < Data.size())
				{
					char V = Data[Where];
					if (V == '\"' || V == '\'')
					{
						while (Where < Data.size() && Data[++Where] != V);
						if (Where + 1 >= Data.size())
						{
							++Where;
							goto AddValue;
						}
					}
					else if (V == ',' || Where + 1 >= Data.size())
					{
					AddValue:
						Core::String Subvalue = Core::String(Data.substr(Last, Where + 1 >= Data.size() ? Core::String::npos : Where - Last));
						Core::Stringify::Trim(Subvalue);
						if (Subvalue.size() >= 2 && Subvalue.front() == Subvalue.back() && (Subvalue.front() == '\"' || Subvalue.front() == '\''))
							Subvalue.erase(Subvalue.size() - 1, 1).erase(0, 1);
						Args.push_back(std::move(Subvalue));
						Last = Where + 1;
					}
					++Where;
				}

				for (auto& KeyValue : Args)
				{
					size_t KeySize = 0;
					char V = KeyValue.empty() ? 0 : KeyValue.front();
					if (V == '\"' || V == '\'')
						while (KeySize < KeyValue.size() && KeyValue[++KeySize] != V);

					KeySize = KeyValue.find('=', KeySize);
					if (KeySize != std::string::npos)
					{
						Core::String Key = KeyValue.substr(0, KeySize - 1);
						Core::String Value = KeyValue.substr(KeySize + 1);
						Core::Stringify::Trim(Key);
						Core::Stringify::Trim(Value);
						Result.Args[Key] = std::move(Value);
					}
					else
						Result.Args[Core::ToString(Result.Args.size())] = std::move(KeyValue);
				}
				Tag.Directives.emplace_back(std::move(Result));
			}

			Exception::Pointer::Pointer() : Context(nullptr)
			{
			}
			Exception::Pointer::Pointer(ImmediateContext* NewContext) : Context(NewContext)
			{
				auto Value = (Context ? Context->GetExceptionString() : std::string_view());
				if (!Value.empty() && (Context ? !Context->WillExceptionBeCaught() : false))
				{
					LoadExceptionData(Value);
					Origin = LoadStackHere();
				}
			}
			Exception::Pointer::Pointer(const std::string_view& Value) : Context(ImmediateContext::Get())
			{
				LoadExceptionData(Value);
				Origin = LoadStackHere();
			}
			Exception::Pointer::Pointer(const std::string_view& NewType, const std::string_view& NewText) : Type(NewType), Text(NewText), Context(ImmediateContext::Get())
			{
				Origin = LoadStackHere();
			}
			void Exception::Pointer::LoadExceptionData(const std::string_view& Value)
			{
				size_t Offset = Value.find(':');
				if (Offset != std::string::npos)
				{
					Type = Value.substr(0, Offset);
					Text = Value.substr(Offset + 1);
				}
				else if (!Value.empty())
				{
					Type = EXCEPTIONCAT_GENERIC;
					Text = Value;
				}
			}
			const Core::String& Exception::Pointer::GetType() const
			{
				return Type;
			}
			const Core::String& Exception::Pointer::GetText() const
			{
				return Text;
			}
			Core::String Exception::Pointer::ToExceptionString() const
			{
				return Empty() ? "" : Type + ":" + Text;
			}
			Core::String Exception::Pointer::What() const
			{
				Core::String Data = Type;
				if (!Text.empty())
				{
					Data.append(": ");
					Data.append(Text);
				}

				Data.append(" ");
				Data.append(Origin.empty() ? LoadStackHere() : Origin);
				return Data;
			}
			Core::String Exception::Pointer::LoadStackHere() const
			{
				Core::String Data;
				if (!Context)
					return Data;

				auto Function = Context->GetExceptionFunction();
				if (!Function.IsValid())
					return Data;

				auto Decl = Function.GetDecl();
				Data.append("\n  in function ");
				Data.append(Decl.empty() ? "<any>" : Decl);

				auto Module = Function.GetModuleName();
				Data.append("\n  in module ");
				Data.append(Module.empty() ? "<anonymous>" : Module);

				int LineNumber = Context->GetExceptionLineNumber();
				if (LineNumber > 0)
				{
					Data.append(":");
					Data.append(Core::ToString(LineNumber));
				}

				return Data;
			}
			bool Exception::Pointer::Empty() const
			{
				return Type.empty() && Text.empty();
			}

			void Exception::ThrowAt(ImmediateContext* Context, const Pointer& Data)
			{
				if (Context != nullptr)
					Context->SetException(Data.ToExceptionString().c_str());
			}
			void Exception::Throw(const Pointer& Data)
			{
				ThrowAt(ImmediateContext::Get(), Data);
			}
			void Exception::RethrowAt(ImmediateContext* Context)
			{
				if (Context != nullptr)
					Context->SetException(Context->GetExceptionString());
			}
			void Exception::Rethrow()
			{
				RethrowAt(ImmediateContext::Get());
			}
			bool Exception::HasExceptionAt(ImmediateContext* Context)
			{
				return Context ? !Context->GetExceptionString().empty() : false;
			}
			bool Exception::HasException()
			{
				return HasExceptionAt(ImmediateContext::Get());
			}
			Exception::Pointer Exception::GetExceptionAt(ImmediateContext* Context)
			{
				if (!Context)
					return Pointer();

				auto Message = Context->GetExceptionString();
				if (Message.empty())
					return Pointer();

				return Pointer(Core::String(Message));
			}
			Exception::Pointer Exception::GetException()
			{
				return GetExceptionAt(ImmediateContext::Get());
			}
			ExpectsVM<void> Exception::GeneratorCallback(Compute::Preprocessor*, const std::string_view& Path, Core::String& Code)
			{
				return Parser::ReplaceInlinePreconditions("throw", Code, [](const std::string_view& Expression) -> ExpectsVM<Core::String>
				{
					Core::String Result = "exception::throw(";
					Result.append(Expression);
					Result.append(1, ')');
					return Result;
				});
			}

			Exception::Pointer ExpectsWrapper::TranslateThrow(const std::exception& Error)
			{
				return Exception::Pointer(EXCEPTIONCAT_STANDARD, Error.what());
			}
			Exception::Pointer ExpectsWrapper::TranslateThrow(const std::error_code& Error)
			{
				return Exception::Pointer(EXCEPTIONCAT_SYSTEM, Core::Copy<Core::String>(Error.message()));
			}
			Exception::Pointer ExpectsWrapper::TranslateThrow(const std::error_condition& Error)
			{
				return Exception::Pointer(EXCEPTIONCAT_SYSTEM, Core::Copy<Core::String>(Error.message()));
			}
			Exception::Pointer ExpectsWrapper::TranslateThrow(const std::string_view& Error)
			{
				return Exception::Pointer(EXCEPTIONCAT_SYSTEM, Error);
			}
			Exception::Pointer ExpectsWrapper::TranslateThrow(const Core::String& Error)
			{
				return Exception::Pointer(EXCEPTIONCAT_SYSTEM, Error);
			}
			Exception::Pointer ExpectsWrapper::TranslateThrow(const Core::BasicException& Error)
			{
				return Exception::Pointer(Error.type(), Error.message());
			}

			Exception::Pointer OptionWrapper::TranslateThrow()
			{
				return Exception::Pointer(EXCEPTION_ACCESSINVALID);
			}

			Any::Any(VirtualMachine* _Engine) noexcept : Engine(_Engine)
			{
				Value.TypeId = 0;
				Value.Integer = 0;
				Engine->NotifyOfNewObject(this, Engine->GetTypeInfoByName(TYPENAME_ANY));
			}
			Any::Any(void* Ref, int RefTypeId, VirtualMachine* _Engine) noexcept : Any(_Engine)
			{
				Store(Ref, RefTypeId);
			}
			Any::Any(const Any& Other) noexcept : Any(Other.Engine)
			{
				if ((Other.Value.TypeId & (size_t)TypeId::MASK_OBJECT))
				{
					auto Info = Engine->GetTypeInfoById(Other.Value.TypeId);
					if (Info.IsValid())
						Info.AddRef();
				}

				Value.TypeId = Other.Value.TypeId;
				if (Value.TypeId & (size_t)TypeId::OBJHANDLE)
				{
					Value.Object = Other.Value.Object;
					Engine->AddRefObject(Value.Object, Engine->GetTypeInfoById(Value.TypeId));
				}
				else if (Value.TypeId & (size_t)TypeId::MASK_OBJECT)
					Value.Object = Engine->CreateObjectCopy(Other.Value.Object, Engine->GetTypeInfoById(Value.TypeId));
				else
					Value.Integer = Other.Value.Integer;
			}
			Any::~Any() noexcept
			{
				FreeObject();
			}
			Any& Any::operator=(const Any& Other) noexcept
			{
				if ((Other.Value.TypeId & (size_t)TypeId::MASK_OBJECT))
				{
					auto Info = Engine->GetTypeInfoById(Other.Value.TypeId);
					if (Info.IsValid())
						Info.AddRef();
				}

				FreeObject();
				Value.TypeId = Other.Value.TypeId;

				if (Value.TypeId & (size_t)TypeId::OBJHANDLE)
				{
					Value.Object = Other.Value.Object;
					Engine->AddRefObject(Value.Object, Engine->GetTypeInfoById(Value.TypeId));
				}
				else if (Value.TypeId & (size_t)TypeId::MASK_OBJECT)
					Value.Object = Engine->CreateObjectCopy(Other.Value.Object, Engine->GetTypeInfoById(Value.TypeId));
				else
					Value.Integer = Other.Value.Integer;

				return *this;
			}
			int Any::CopyFrom(const Any* Other)
			{
				if (Other == 0)
					return (int)VirtualError::INVALID_ARG;

				*this = *(Any*)Other;
				return 0;
			}
			void Any::Store(void* Ref, int RefTypeId)
			{
				if ((RefTypeId & (size_t)TypeId::MASK_OBJECT))
				{
					auto Info = Engine->GetTypeInfoById(RefTypeId);
					if (Info.IsValid())
						Info.AddRef();
				}

				FreeObject();
				Value.TypeId = RefTypeId;

				if (Value.TypeId & (size_t)TypeId::OBJHANDLE)
				{
					Value.Object = *(void**)Ref;
					Engine->AddRefObject(Value.Object, Engine->GetTypeInfoById(Value.TypeId));
				}
				else if (Value.TypeId & (size_t)TypeId::MASK_OBJECT)
				{
					Value.Object = Engine->CreateObjectCopy(Ref, Engine->GetTypeInfoById(Value.TypeId));
				}
				else
				{
					Value.Integer = 0;
					auto Size = Engine->GetSizeOfPrimitiveType(Value.TypeId);
					if (Size)
						memcpy(&Value.Integer, Ref, *Size);
				}
			}
			bool Any::Retrieve(void* Ref, int RefTypeId) const
			{
				if (RefTypeId & (size_t)TypeId::OBJHANDLE)
				{
					if ((Value.TypeId & (size_t)TypeId::MASK_OBJECT))
					{
						if ((Value.TypeId & (size_t)TypeId::HANDLETOCONST) && !(RefTypeId & (size_t)TypeId::HANDLETOCONST))
							return false;

						Engine->RefCastObject(Value.Object, Engine->GetTypeInfoById(Value.TypeId), Engine->GetTypeInfoById(RefTypeId), reinterpret_cast<void**>(Ref));
#ifdef VI_ANGELSCRIPT
						if (*(asPWORD*)Ref == 0)
							return false;
#endif

						return true;
					}
				}
				else if (RefTypeId & (size_t)TypeId::MASK_OBJECT)
				{
					if (Value.TypeId == RefTypeId)
					{
						Engine->AssignObject(Ref, Value.Object, Engine->GetTypeInfoById(Value.TypeId));
						return true;
					}
				}
				else
				{
					auto Size1 = Engine->GetSizeOfPrimitiveType(Value.TypeId);
					auto Size2 = Engine->GetSizeOfPrimitiveType(RefTypeId);
					if (Size1 && Size2 && *Size1 == *Size2)
					{
						memcpy(Ref, &Value.Integer, *Size1);
						return true;
					}
				}

				return false;
			}
			void* Any::GetAddressOfObject()
			{
				if (Value.TypeId & (size_t)TypeId::OBJHANDLE)
					return &Value.Object;
				else if (Value.TypeId & (size_t)TypeId::MASK_OBJECT)
					return Value.Object;
				else if (Value.TypeId <= (size_t)TypeId::DOUBLE || Value.TypeId & (size_t)TypeId::MASK_SEQNBR)
					return &Value.Integer;

				return nullptr;
			}
			int Any::GetTypeId() const
			{
				return Value.TypeId;
			}
			void Any::FreeObject()
			{
				if (Value.TypeId & (size_t)TypeId::MASK_OBJECT)
				{
					auto Type = Engine->GetTypeInfoById(Value.TypeId);
					Engine->ReleaseObject(Value.Object, Type);
					Type.Release();
					Value.Object = 0;
					Value.TypeId = 0;
				}
			}
			void Any::EnumReferences(asIScriptEngine* InEngine)
			{
				if (Value.Object && (Value.TypeId & (size_t)TypeId::MASK_OBJECT))
				{
					auto SubType = Engine->GetTypeInfoById(Value.TypeId);
					if ((SubType.Flags() & (size_t)ObjectBehaviours::REF))
						FunctionFactory::GCEnumCallback(InEngine, Value.Object);
					else if ((SubType.Flags() & (size_t)ObjectBehaviours::VALUE) && (SubType.Flags() & (size_t)ObjectBehaviours::GC))
						Engine->ForwardEnumReferences(Value.Object, SubType);

					auto Type = VirtualMachine::Get(InEngine)->GetTypeInfoById(Value.TypeId);
					FunctionFactory::GCEnumCallback(InEngine, Type.GetTypeInfo());
				}
			}
			void Any::ReleaseReferences(asIScriptEngine*)
			{
				FreeObject();
			}
			Core::Unique<Any> Any::Create()
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				Any* Result = new Any(VM);
				if (!Result)
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));

				return Result;
			}
			Any* Any::Create(int TypeId, void* Ref)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				Any* Result = new Any(Ref, TypeId, VM);
				if (!Result)
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));

				return Result;
			}
			Any* Any::Create(const char* Decl, void* Ref)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				Any* Result = new Any(Ref, VM->GetTypeIdByDecl(Decl), VM);
				if (!Result)
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));

				return Result;
			}
			Any* Any::Factory1()
			{
				Any* Result = new Any(VirtualMachine::Get());
				if (!Result)
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));
				return Result;
			}
			void Any::Factory2(asIScriptGeneric* Generic)
			{
				GenericContext Args = Generic;
				Any* Result = new Any(Args.GetArgAddress(0), Args.GetArgTypeId(0), Args.GetVM());
				if (!Result)
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));
				else
					*(Any**)Args.GetAddressOfReturnLocation() = Result;
			}
			Any& Any::Assignment(Any* Base, Any* Other)
			{
				*Base = *Other;
				return *Base;
			}

			Array::Array(asITypeInfo* Info, void* BufferPtr) noexcept : ObjType(Info), Buffer(nullptr), ElementSize(0), SubTypeId(-1)
			{
				VI_ASSERT(Info && Core::String(ObjType.GetName()) == TYPENAME_ARRAY, "array type is invalid");
#ifdef VI_ANGELSCRIPT
				ObjType.AddRef();
				Precache();

				VirtualMachine* Engine = ObjType.GetVM();
				if (SubTypeId & (size_t)TypeId::MASK_OBJECT)
					ElementSize = sizeof(asPWORD);
				else
					ElementSize = Engine->GetSizeOfPrimitiveType(SubTypeId).Or(0);

				size_t Length = *(asUINT*)BufferPtr;
				if (!CheckMaxSize(Length))
					return;

				if ((ObjType.GetSubTypeId() & (size_t)TypeId::MASK_OBJECT) == 0)
				{
					CreateBuffer(&Buffer, Length);
					if (Length > 0)
						memcpy(At(0), (((asUINT*)BufferPtr) + 1), (size_t)Length * (size_t)ElementSize);
				}
				else if (ObjType.GetSubTypeId() & (size_t)TypeId::OBJHANDLE)
				{
					CreateBuffer(&Buffer, Length);
					if (Length > 0)
						memcpy(At(0), (((asUINT*)BufferPtr) + 1), (size_t)Length * (size_t)ElementSize);

					memset((((asUINT*)BufferPtr) + 1), 0, (size_t)Length * (size_t)ElementSize);
				}
				else if (ObjType.GetSubType().Flags() & (size_t)ObjectBehaviours::REF)
				{
					SubTypeId |= (size_t)TypeId::OBJHANDLE;
					CreateBuffer(&Buffer, Length);
					SubTypeId &= ~(size_t)TypeId::OBJHANDLE;

					if (Length > 0)
						memcpy(Buffer->Data, (((asUINT*)BufferPtr) + 1), (size_t)Length * (size_t)ElementSize);

					memset((((asUINT*)BufferPtr) + 1), 0, (size_t)Length * (size_t)ElementSize);
				}
				else
				{
					CreateBuffer(&Buffer, Length);
					for (size_t n = 0; n < Length; n++)
					{
						unsigned char* SourceObj = (unsigned char*)BufferPtr;
						SourceObj += 4 + n * ObjType.GetSubType().Size();
						Engine->AssignObject(At(n), SourceObj, ObjType.GetSubType());
					}
				}

				if (ObjType.Flags() & (size_t)ObjectBehaviours::GC)
					ObjType.GetVM()->NotifyOfNewObject(this, ObjType);
#endif
			}
			Array::Array(size_t Length, asITypeInfo* Info) noexcept : ObjType(Info), Buffer(nullptr), ElementSize(0), SubTypeId(-1)
			{
				VI_ASSERT(Info && Core::String(ObjType.GetName()) == TYPENAME_ARRAY, "array type is invalid");
#ifdef VI_ANGELSCRIPT
				ObjType.AddRef();
				Precache();

				if (SubTypeId & (size_t)TypeId::MASK_OBJECT)
					ElementSize = sizeof(asPWORD);
				else
					ElementSize = ObjType.GetVM()->GetSizeOfPrimitiveType(SubTypeId).Or(0);

				if (!CheckMaxSize(Length))
					return;

				CreateBuffer(&Buffer, Length);
				if (ObjType.Flags() & (size_t)ObjectBehaviours::GC)
					ObjType.GetVM()->NotifyOfNewObject(this, ObjType);
#endif
			}
			Array::Array(const Array& Other) noexcept : ObjType(Other.ObjType), Buffer(nullptr), ElementSize(0), SubTypeId(-1)
			{
				VI_ASSERT(ObjType.IsValid() && Core::String(ObjType.GetName()) == TYPENAME_ARRAY, "array type is invalid");
				ObjType.AddRef();
				Precache();

				ElementSize = Other.ElementSize;
				if (ObjType.Flags() & (size_t)ObjectBehaviours::GC)
					ObjType.GetVM()->NotifyOfNewObject(this, ObjType);

				CreateBuffer(&Buffer, 0);
				*this = Other;
			}
			Array::Array(size_t Length, void* DefaultValue, asITypeInfo* Info) noexcept : ObjType(Info), Buffer(nullptr), ElementSize(0), SubTypeId(-1)
			{
#ifdef VI_ANGELSCRIPT
				VI_ASSERT(Info && Core::String(Info->GetName()) == TYPENAME_ARRAY, "array type is invalid");
				ObjType.AddRef();
				Precache();

				if (SubTypeId & (size_t)TypeId::MASK_OBJECT)
					ElementSize = sizeof(asPWORD);
				else
					ElementSize = ObjType.GetVM()->GetSizeOfPrimitiveType(SubTypeId).Or(0);

				if (!CheckMaxSize(Length))
					return;

				CreateBuffer(&Buffer, Length);
				if (ObjType.Flags() & (size_t)ObjectBehaviours::GC)
					ObjType.GetVM()->NotifyOfNewObject(this, ObjType);

				for (size_t i = 0; i < Size(); i++)
					SetValue(i, DefaultValue);
#endif
			}
			Array::~Array() noexcept
			{
				if (Buffer)
				{
					DeleteBuffer(Buffer);
					Buffer = nullptr;
				}
				ObjType.Release();
			}
			Array& Array::operator=(const Array& Other) noexcept
			{
				if (&Other != this && Other.GetArrayObjectType() == GetArrayObjectType())
				{
					Resize(Other.Buffer->NumElements);
					CopyBuffer(Buffer, Other.Buffer);
				}

				return *this;
			}
			void Array::SetValue(size_t Index, void* Value)
			{
				void* Ptr = At(Index);
				if (Ptr == 0)
					return;

				if ((SubTypeId & ~(size_t)TypeId::MASK_SEQNBR) && !(SubTypeId & (size_t)TypeId::OBJHANDLE))
					ObjType.GetVM()->AssignObject(Ptr, Value, ObjType.GetSubType());
				else if (SubTypeId & (size_t)TypeId::OBJHANDLE)
				{
					void* Swap = *(void**)Ptr;
					*(void**)Ptr = *(void**)Value;
					ObjType.GetVM()->AddRefObject(*(void**)Value, ObjType.GetSubType());
					if (Swap)
						ObjType.GetVM()->ReleaseObject(Swap, ObjType.GetSubType());
				}
				else if (SubTypeId == (size_t)TypeId::BOOL || SubTypeId == (size_t)TypeId::INT8 || SubTypeId == (size_t)TypeId::UINT8)
					*(char*)Ptr = *(char*)Value;
				else if (SubTypeId == (size_t)TypeId::INT16 || SubTypeId == (size_t)TypeId::UINT16)
					*(short*)Ptr = *(short*)Value;
				else if (SubTypeId == (size_t)TypeId::INT32 || SubTypeId == (size_t)TypeId::UINT32 || SubTypeId == (size_t)TypeId::FLOAT || SubTypeId > (size_t)TypeId::DOUBLE)
					*(int*)Ptr = *(int*)Value;
				else if (SubTypeId == (size_t)TypeId::INT64 || SubTypeId == (size_t)TypeId::UINT64 || SubTypeId == (size_t)TypeId::DOUBLE)
					*(double*)Ptr = *(double*)Value;
			}
			size_t Array::Size() const
			{
				return Buffer->NumElements;
			}
			size_t Array::Capacity() const
			{
				return Buffer->MaxElements;
			}
			bool Array::Empty() const
			{
				return Buffer->NumElements == 0;
			}
			void Array::Reserve(size_t MaxElements)
			{
				if (MaxElements <= Buffer->MaxElements)
					return;

				if (!CheckMaxSize(MaxElements))
					return;

				SBuffer* NewBuffer = Core::Memory::Allocate<SBuffer>(sizeof(SBuffer) - 1 + (size_t)ElementSize * (size_t)MaxElements);
				if (!NewBuffer)
					return Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));

				NewBuffer->NumElements = Buffer->NumElements;
				NewBuffer->MaxElements = MaxElements;
				memcpy(NewBuffer->Data, Buffer->Data, (size_t)Buffer->NumElements * (size_t)ElementSize);
				Core::Memory::Deallocate(Buffer);
				Buffer = NewBuffer;
			}
			void Array::Resize(size_t NumElements)
			{
				if (!CheckMaxSize(NumElements))
					return;

				Resize((int64_t)NumElements - (int64_t)Buffer->NumElements, (size_t)-1);
			}
			void Array::RemoveRange(size_t Start, size_t Count)
			{
				if (Count == 0)
					return;

				if (Buffer == 0 || Start > Buffer->NumElements)
					return Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));

				if (Start + Count > Buffer->NumElements)
					Count = Buffer->NumElements - Start;

				Destroy(Buffer, Start, Start + Count);
				memmove(Buffer->Data + Start * (size_t)ElementSize, Buffer->Data + (Start + Count) * (size_t)ElementSize, (size_t)(Buffer->NumElements - Start - Count) * (size_t)ElementSize);
				Buffer->NumElements -= Count;
			}
			void Array::Resize(int64_t Delta, size_t Where)
			{
				if (Delta < 0)
				{
					if (-Delta > (int64_t)Buffer->NumElements)
						Delta = -(int64_t)Buffer->NumElements;

					if (Where > Buffer->NumElements + Delta)
						Where = Buffer->NumElements + Delta;
				}
				else if (Delta > 0)
				{
					if (!CheckMaxSize(Buffer->NumElements + Delta))
						return;

					if (Where > Buffer->NumElements)
						Where = Buffer->NumElements;
				}

				if (Delta == 0)
					return;

				if (Buffer->MaxElements < Buffer->NumElements + Delta)
				{
					size_t Count = (size_t)Buffer->NumElements + (size_t)Delta, Size = (size_t)ElementSize;
					SBuffer* NewBuffer = Core::Memory::Allocate<SBuffer>(sizeof(SBuffer) - 1 + Size * Count);
					if (!NewBuffer)
						return Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));

					NewBuffer->NumElements = Buffer->NumElements + Delta;
					NewBuffer->MaxElements = NewBuffer->NumElements;
					memcpy(NewBuffer->Data, Buffer->Data, (size_t)Where * (size_t)ElementSize);
					if (Where < Buffer->NumElements)
						memcpy(NewBuffer->Data + (Where + Delta) * (size_t)ElementSize, Buffer->Data + Where * (size_t)ElementSize, (size_t)(Buffer->NumElements - Where) * (size_t)ElementSize);

					Create(NewBuffer, Where, Where + Delta);
					Core::Memory::Deallocate(Buffer);
					Buffer = NewBuffer;
				}
				else if (Delta < 0)
				{
					Destroy(Buffer, Where, Where - Delta);
					memmove(Buffer->Data + Where * (size_t)ElementSize, Buffer->Data + (Where - Delta) * (size_t)ElementSize, (size_t)(Buffer->NumElements - (Where - Delta)) * (size_t)ElementSize);
					Buffer->NumElements += Delta;
				}
				else
				{
					memmove(Buffer->Data + (Where + Delta) * (size_t)ElementSize, Buffer->Data + Where * (size_t)ElementSize, (size_t)(Buffer->NumElements - Where) * (size_t)ElementSize);
					Create(Buffer, Where, Where + Delta);
					Buffer->NumElements += Delta;
				}
			}
			bool Array::CheckMaxSize(size_t NumElements)
			{
				size_t MaxSize = 0xFFFFFFFFul - sizeof(SBuffer) + 1;
				if (ElementSize > 0)
					MaxSize /= (size_t)ElementSize;

				if (NumElements <= MaxSize)
					return true;

				Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_TOOLARGESIZE));
				return false;
			}
			asITypeInfo* Array::GetArrayObjectType() const
			{
				return ObjType.GetTypeInfo();
			}
			int Array::GetArrayTypeId() const
			{
				return ObjType.GetTypeId();
			}
			int Array::GetElementTypeId() const
			{
				return SubTypeId;
			}
			void Array::InsertAt(size_t Index, void* Value)
			{
				if (Index > Buffer->NumElements)
					return Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));

				Resize(1, Index);
				SetValue(Index, Value);
			}
			void Array::InsertAt(size_t Index, const Array& Array)
			{
				if (Index > Buffer->NumElements)
					return Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));

				if (ObjType.GetTypeInfo() != Array.ObjType.GetTypeInfo())
					return Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_TEMPLATEMISMATCH));

				size_t NewSize = Array.Size();
				Resize((int)NewSize, Index);

				if (&Array != this)
				{
					for (size_t i = 0; i < Array.Size(); i++)
					{
						void* Value = const_cast<void*>(Array.At(i));
						SetValue(Index + i, Value);
					}
				}
				else
				{
					for (size_t i = 0; i < Index; i++)
					{
						void* Value = const_cast<void*>(Array.At(i));
						SetValue(Index + i, Value);
					}

					for (size_t i = Index + NewSize, k = 0; i < Array.Size(); i++, k++)
					{
						void* Value = const_cast<void*>(Array.At(i));
						SetValue(Index + Index + k, Value);
					}
				}
			}
			void Array::InsertLast(void* Value)
			{
				InsertAt(Buffer->NumElements, Value);
			}
			void Array::RemoveAt(size_t Index)
			{
				if (Index >= Buffer->NumElements)
					return Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
				Resize(-1, Index);
			}
			void Array::RemoveLast()
			{
				RemoveAt(Buffer->NumElements - 1);
			}
			const void* Array::At(size_t Index) const
			{
				if (Buffer == 0 || Index >= Buffer->NumElements)
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}
				else if ((SubTypeId & (size_t)TypeId::MASK_OBJECT) && !(SubTypeId & (size_t)TypeId::OBJHANDLE))
					return *(void**)(Buffer->Data + (size_t)ElementSize * Index);

				return Buffer->Data + (size_t)ElementSize * Index;
			}
			void* Array::At(size_t Index)
			{
				return const_cast<void*>(const_cast<const Array*>(this)->At(Index));
			}
			void* Array::Front()
			{
				if (Empty())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				return At(0);
			}
			const void* Array::Front() const
			{
				if (Empty())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				return At(0);
			}
			void* Array::Back()
			{
				if (Empty())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				return At(Size() - 1);
			}
			const void* Array::Back() const
			{
				if (Empty())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				return At(Size() - 1);
			}
			void* Array::GetBuffer()
			{
				return Buffer->Data;
			}
			void Array::CreateBuffer(SBuffer** BufferPtr, size_t NumElements)
			{
				*BufferPtr = Core::Memory::Allocate<SBuffer>(sizeof(SBuffer) - 1 + (size_t)ElementSize * (size_t)NumElements);
				if (!*BufferPtr)
					return Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));

				(*BufferPtr)->NumElements = NumElements;
				(*BufferPtr)->MaxElements = NumElements;
				Create(*BufferPtr, 0, NumElements);
			}
			void Array::DeleteBuffer(SBuffer* BufferPtr)
			{
				Destroy(BufferPtr, 0, BufferPtr->NumElements);
				Core::Memory::Deallocate(BufferPtr);
			}
			void Array::Create(SBuffer* BufferPtr, size_t Start, size_t End)
			{
				if ((SubTypeId & (size_t)TypeId::MASK_OBJECT) && !(SubTypeId & (size_t)TypeId::OBJHANDLE))
				{
					void** Max = (void**)(BufferPtr->Data + End * sizeof(void*));
					void** D = (void**)(BufferPtr->Data + Start * sizeof(void*));

					VirtualMachine* Engine = ObjType.GetVM();
					TypeInfo SubType = ObjType.GetSubType();

					for (; D < Max; D++)
					{
						*D = (void*)Engine->CreateObject(SubType);
						if (*D == 0)
						{
							memset(D, 0, sizeof(void*) * (Max - D));
							return;
						}
					}
				}
				else
				{
					void* D = (void*)(BufferPtr->Data + Start * (size_t)ElementSize);
					memset(D, 0, (size_t)(End - Start) * (size_t)ElementSize);
				}
			}
			void Array::Destroy(SBuffer* BufferPtr, size_t Start, size_t End)
			{
				if (SubTypeId & (size_t)TypeId::MASK_OBJECT)
				{
					VirtualMachine* Engine = ObjType.GetVM();
					TypeInfo SubType = ObjType.GetSubType();
					void** Max = (void**)(BufferPtr->Data + End * sizeof(void*));
					void** D = (void**)(BufferPtr->Data + Start * sizeof(void*));

					for (; D < Max; D++)
					{
						if (*D)
							Engine->ReleaseObject(*D, SubType);
					}
				}
			}
			void Array::Reverse()
			{
				size_t Length = Size();
				if (Length >= 2)
				{
					unsigned char Temp[16];
					for (size_t i = 0; i < Length / 2; i++)
					{
						Copy(Temp, GetArrayItemPointer((int)i));
						Copy(GetArrayItemPointer((int)i), GetArrayItemPointer((int)(Length - i - 1)));
						Copy(GetArrayItemPointer((int)(Length - i - 1)), Temp);
					}
				}
			}
			void Array::Clear()
			{
				Resize(0);
			}
			bool Array::operator==(const Array& Other) const
			{
				if (ObjType.GetTypeInfo() != Other.ObjType.GetTypeInfo())
					return false;

				if (Size() != Other.Size())
					return false;

				ImmediateContext* CmpContext = 0;
				bool IsNested = false;

				if (SubTypeId & ~(size_t)TypeId::MASK_SEQNBR)
				{
					CmpContext = ImmediateContext::Get();
					if (CmpContext)
					{
						if (CmpContext->GetVM() == ObjType.GetVM() && CmpContext->PushState())
							IsNested = true;
						else
							CmpContext = 0;
					}

					if (CmpContext == 0)
						CmpContext = ObjType.GetVM()->RequestContext();
				}

				bool IsEqual = true;
				SCache* Cache = reinterpret_cast<SCache*>(ObjType.GetUserData(ArrayId));
				for (size_t n = 0; n < Size(); n++)
				{
					if (!Equals(At(n), Other.At(n), CmpContext, Cache))
					{
						IsEqual = false;
						break;
					}
				}

				if (CmpContext)
				{
					if (IsNested)
					{
						auto State = CmpContext->GetState();
						CmpContext->PopState();
						if (State == Execution::Aborted)
							CmpContext->Abort();
					}
					else
						ObjType.GetVM()->ReturnContext(CmpContext);
				}

				return IsEqual;
			}
			bool Array::Less(const void* A, const void* B, ImmediateContext* Context, SCache* Cache)
			{
				if (SubTypeId & ~(size_t)TypeId::MASK_SEQNBR)
				{
					if (SubTypeId & (size_t)TypeId::OBJHANDLE)
					{
						if (*(void**)A == 0)
							return true;

						if (*(void**)B == 0)
							return false;
					}

					if (!Cache || !Cache->Comparator)
						return false;

					bool IsLess = false;
					Context->ExecuteSubcall(Cache->Comparator, [A, B](ImmediateContext* Context)
					{
						Context->SetObject((void*)A);
						Context->SetArgObject(0, (void*)B);
					}, [&IsLess](ImmediateContext* Context) { IsLess = (Context->GetReturnDWord() < 0); });
					return IsLess;
				}

				switch (SubTypeId)
				{
#define COMPARE(T) *((T*)A) < *((T*)B)
					case (size_t)TypeId::BOOL: return COMPARE(bool);
					case (size_t)TypeId::INT8: return COMPARE(signed char);
					case (size_t)TypeId::UINT8: return COMPARE(unsigned char);
					case (size_t)TypeId::INT16: return COMPARE(signed short);
					case (size_t)TypeId::UINT16: return COMPARE(unsigned short);
					case (size_t)TypeId::INT32: return COMPARE(signed int);
					case (size_t)TypeId::UINT32: return COMPARE(uint32_t);
					case (size_t)TypeId::FLOAT: return COMPARE(float);
					case (size_t)TypeId::DOUBLE: return COMPARE(double);
					default: return COMPARE(signed int);
#undef COMPARE
				}

				return false;
			}
			bool Array::Equals(const void* A, const void* B, ImmediateContext* Context, SCache* Cache) const
			{
				if (SubTypeId & ~(size_t)TypeId::MASK_SEQNBR)
				{
					if (SubTypeId & (size_t)TypeId::OBJHANDLE)
					{
						if (*(void**)A == *(void**)B)
							return true;
					}

					if (Cache && Cache->Equals)
					{
						bool IsMatched = false;
						Context->ExecuteSubcall(Cache->Equals, [A, B](ImmediateContext* Context)
						{
							Context->SetObject((void*)A);
							Context->SetArgObject(0, (void*)B);
						}, [&IsMatched](ImmediateContext* Context) { IsMatched = (Context->GetReturnByte() != 0); });
						return IsMatched;
					}

					if (Cache && Cache->Comparator)
					{
						bool IsMatched = false;
						Context->ExecuteSubcall(Cache->Comparator, [A, B](ImmediateContext* Context)
						{
							Context->SetObject((void*)A);
							Context->SetArgObject(0, (void*)B);
						}, [&IsMatched](ImmediateContext* Context) { IsMatched = (Context->GetReturnDWord() == 0); });
						return IsMatched;
					}

					return false;
				}

				switch (SubTypeId)
				{
#define COMPARE(T) *((T*)A) == *((T*)B)
					case (size_t)TypeId::BOOL: return COMPARE(bool);
					case (size_t)TypeId::INT8: return COMPARE(signed char);
					case (size_t)TypeId::UINT8: return COMPARE(unsigned char);
					case (size_t)TypeId::INT16: return COMPARE(signed short);
					case (size_t)TypeId::UINT16: return COMPARE(unsigned short);
					case (size_t)TypeId::INT32: return COMPARE(signed int);
					case (size_t)TypeId::UINT32: return COMPARE(uint32_t);
					case (size_t)TypeId::FLOAT: return COMPARE(float);
					case (size_t)TypeId::DOUBLE: return COMPARE(double);
					default: return COMPARE(signed int);
#undef COMPARE
				}
			}
			size_t Array::FindByRef(void* Value, size_t StartAt) const
			{
				size_t Length = Size();
				if (SubTypeId & (size_t)TypeId::OBJHANDLE)
				{
					Value = *(void**)Value;
					for (size_t i = StartAt; i < Length; i++)
					{
						if (*(void**)At(i) == Value)
							return i;
					}
				}
				else
				{
					for (size_t i = StartAt; i < Length; i++)
					{
						if (At(i) == Value)
							return i;
					}
				}

				return std::string::npos;
			}
			size_t Array::Find(void* Value, size_t StartAt) const
			{
				SCache* Cache; size_t Count = Size();
				if (!IsEligibleForFind(&Cache) || !Count)
					return std::string::npos;

				ImmediateContext* Context = ImmediateContext::Get();
				for (size_t i = StartAt; i < Count; i++)
				{
					if (Equals(At(i), Value, Context, Cache))
						return i;
				}

				return std::string::npos;
			}
			void Array::Copy(void* Dest, void* Src)
			{
				memcpy(Dest, Src, ElementSize);
			}
			void* Array::GetArrayItemPointer(size_t Index)
			{
				return Buffer->Data + Index * ElementSize;
			}
			void* Array::GetDataPointer(void* BufferPtr)
			{
				if ((SubTypeId & (size_t)TypeId::MASK_OBJECT) && !(SubTypeId & (size_t)TypeId::OBJHANDLE))
					return reinterpret_cast<void*>(*(size_t*)BufferPtr);
				else
					return BufferPtr;
			}
			void Array::Swap(size_t Index1, size_t Index2)
			{
				if (Index1 >= Size() || Index2 >= Size())
					return Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));

				unsigned char Swap[16];
				Copy(Swap, GetArrayItemPointer(Index1));
				Copy(GetArrayItemPointer(Index1), GetArrayItemPointer(Index2));
				Copy(GetArrayItemPointer(Index2), Swap);
			}
			void Array::Sort(asIScriptFunction* Callback)
			{
				SCache* Cache; size_t Count = Size();
				if (!IsEligibleForSort(&Cache) || Count < 2)
					return;

				unsigned char Swap[16];
				ImmediateContext* Context = ImmediateContext::Get();
				if (Callback != nullptr)
				{
					FunctionDelegate Delegate(Callback);
					for (size_t i = 1; i < Count; i++)
					{
						int64_t j = (int64_t)(i - 1);
						Copy(Swap, GetArrayItemPointer(i));
						while (j >= 0)
						{
							void* A = GetDataPointer(Swap), *B = At(j); bool IsLess = false;
							Context->ExecuteSubcall(Delegate.Callable(), [A, B](ImmediateContext* Context)
							{
								Context->SetArgAddress(0, A);
								Context->SetArgAddress(1, B);
							}, [&IsLess](ImmediateContext* Context) { IsLess = (Context->GetReturnByte() > 0); });
							if (!IsLess)
								break;

							Copy(GetArrayItemPointer(j + 1), GetArrayItemPointer(j));
							j--;
						}
						Copy(GetArrayItemPointer(j + 1), Swap);
					}
				}
				else
				{
					for (size_t i = 1; i < Count; i++)
					{
						int64_t j = (int64_t)(i - 1);
						Copy(Swap, GetArrayItemPointer(i));
						while (j >= 0 && Less(GetDataPointer(Swap), At(j), Context, Cache))
						{
							Copy(GetArrayItemPointer(j + 1), GetArrayItemPointer(j));
							j--;
						}
						Copy(GetArrayItemPointer(j + 1), Swap);
					}
				}
			}
			void Array::CopyBuffer(SBuffer* Dest, SBuffer* Src)
			{
				VirtualMachine* Engine = ObjType.GetVM();
				if (SubTypeId & (size_t)TypeId::OBJHANDLE)
				{
					if (Dest->NumElements > 0 && Src->NumElements > 0)
					{
						int Count = (int)(Dest->NumElements > Src->NumElements ? Src->NumElements : Dest->NumElements);
						void** Max = (void**)(Dest->Data + Count * sizeof(void*));
						void** D = (void**)Dest->Data;
						void** S = (void**)Src->Data;

						for (; D < Max; D++, S++)
						{
							void* Swap = *D;
							*D = *S;

							if (*D)
								Engine->AddRefObject(*D, ObjType.GetSubType());

							if (Swap)
								Engine->ReleaseObject(Swap, ObjType.GetSubType());
						}
					}
				}
				else
				{
					if (Dest->NumElements > 0 && Src->NumElements > 0)
					{
						int Count = (int)(Dest->NumElements > Src->NumElements ? Src->NumElements : Dest->NumElements);
						if (SubTypeId & (size_t)TypeId::MASK_OBJECT)
						{
							void** Max = (void**)(Dest->Data + Count * sizeof(void*));
							void** D = (void**)Dest->Data;
							void** S = (void**)Src->Data;

							auto SubType = ObjType.GetSubType();
							for (; D < Max; D++, S++)
								Engine->AssignObject(*D, *S, SubType);
						}
						else
							memcpy(Dest->Data, Src->Data, (size_t)Count * (size_t)ElementSize);
					}
				}
			}
			void Array::Precache()
			{
#ifdef VI_ANGELSCRIPT
				SubTypeId = ObjType.GetSubTypeId();
				if (!(SubTypeId & ~(size_t)TypeId::MASK_SEQNBR))
					return;

				SCache* Cache = reinterpret_cast<SCache*>(ObjType.GetUserData(ArrayId));
				if (Cache)
					return;

				asAcquireExclusiveLock();
				Cache = reinterpret_cast<SCache*>(ObjType.GetUserData(ArrayId));
				if (Cache)
					return asReleaseExclusiveLock();

				Cache = Core::Memory::Allocate<SCache>(sizeof(SCache));
				if (!Cache)
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));
					return asReleaseExclusiveLock();
				}

				memset(Cache, 0, sizeof(SCache));
				bool MustBeConst = (SubTypeId & (size_t)TypeId::HANDLETOCONST) ? true : false;

				auto SubType = ObjType.GetVM()->GetTypeInfoById(SubTypeId);
				if (SubType.IsValid())
				{
					for (size_t i = 0; i < SubType.GetMethodsCount(); i++)
					{
						auto Function = SubType.GetMethodByIndex((int)i);
						if (Function.GetArgsCount() == 1 && (!MustBeConst || Function.IsReadOnly()))
						{
							size_t Flags = 0;
							int ReturnTypeId = Function.GetReturnTypeId(&Flags);
							if (Flags != (size_t)Modifiers::NONE)
								continue;

							bool IsCmp = false, IsEquals = false;
							if (ReturnTypeId == (size_t)TypeId::INT32 && Function.GetName() == "opCmp")
								IsCmp = true;
							if (ReturnTypeId == (size_t)TypeId::BOOL && Function.GetName() == "opEquals")
								IsEquals = true;

							if (!IsCmp && !IsEquals)
								continue;

							int ParamTypeId;
							Function.GetArg(0, &ParamTypeId, &Flags);

							if ((ParamTypeId & ~((size_t)TypeId::OBJHANDLE | (size_t)TypeId::HANDLETOCONST)) != (SubTypeId & ~((size_t)TypeId::OBJHANDLE | (size_t)TypeId::HANDLETOCONST)))
								continue;

							if ((Flags & (size_t)Modifiers::INREF))
							{
								if ((ParamTypeId & (size_t)TypeId::OBJHANDLE) || (MustBeConst && !(Flags & (size_t)Modifiers::CONSTF)))
									continue;
							}
							else if (ParamTypeId & (size_t)TypeId::OBJHANDLE)
							{
								if (MustBeConst && !(ParamTypeId & (size_t)TypeId::HANDLETOCONST))
									continue;
							}
							else
								continue;

							if (IsCmp)
							{
								if (Cache->Comparator || Cache->ComparatorReturnCode)
								{
									Cache->Comparator = 0;
									Cache->ComparatorReturnCode = (int)VirtualError::MULTIPLE_FUNCTIONS;
								}
								else
									Cache->Comparator = Function.GetFunction();
							}
							else if (IsEquals)
							{
								if (Cache->Equals || Cache->EqualsReturnCode)
								{
									Cache->Equals = 0;
									Cache->EqualsReturnCode = (int)VirtualError::MULTIPLE_FUNCTIONS;
								}
								else
									Cache->Equals = Function.GetFunction();
							}
						}
					}
				}

				if (Cache->Equals == 0 && Cache->EqualsReturnCode == 0)
					Cache->EqualsReturnCode = (int)VirtualError::NO_FUNCTION;
				if (Cache->Comparator == 0 && Cache->ComparatorReturnCode == 0)
					Cache->ComparatorReturnCode = (int)VirtualError::NO_FUNCTION;

				ObjType.SetUserData(Cache, ArrayId);
				asReleaseExclusiveLock();
#endif
			}
			void Array::EnumReferences(asIScriptEngine* Engine)
			{
				if (SubTypeId & (size_t)TypeId::MASK_OBJECT)
				{
					void** Data = (void**)Buffer->Data;
					VirtualMachine* VM = VirtualMachine::Get(Engine);
					auto SubType = VM->GetTypeInfoById(SubTypeId);
					if ((SubType.Flags() & (size_t)ObjectBehaviours::REF))
					{
						for (size_t i = 0; i < Buffer->NumElements; i++)
							FunctionFactory::GCEnumCallback(Engine, Data[i]);
					}
					else if ((SubType.Flags() & (size_t)ObjectBehaviours::VALUE) && (SubType.Flags() & (size_t)ObjectBehaviours::GC))
					{
						for (size_t i = 0; i < Buffer->NumElements; i++)
						{
							if (Data[i])
								VM->ForwardEnumReferences(Data[i], SubType);
						}
					}
				}
			}
			void Array::ReleaseReferences(asIScriptEngine*)
			{
				Resize(0);
			}
			Array* Array::Create(asITypeInfo* Info, size_t Length)
			{
				Array* Result = new Array(Length, Info);
				if (!Result)
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));

				return Result;
			}
			Array* Array::Create(asITypeInfo* Info, size_t Length, void* DefaultValue)
			{
				Array* Result = new Array(Length, DefaultValue, Info);
				if (!Result)
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));

				return Result;
			}
			Array* Array::Create(asITypeInfo* Info)
			{
				return Array::Create(Info, (size_t)0);
			}
			Array* Array::Factory(asITypeInfo* Info, void* InitList)
			{
				Array* Result = new Array(Info, InitList);
				if (!Result)
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));

				return Result;
			}
			void Array::CleanupTypeInfoCache(asITypeInfo* TypeContext)
			{
				TypeInfo Type(TypeContext);
				Array::SCache* Cache = reinterpret_cast<Array::SCache*>(Type.GetUserData(ArrayId));
				if (Cache != nullptr)
				{
					Cache->~SCache();
					Core::Memory::Deallocate(Cache);
				}
			}
			bool Array::TemplateCallback(asITypeInfo* InfoContext, bool& DontGarbageCollect)
			{
				TypeInfo Info(InfoContext);
				int TypeId = Info.GetSubTypeId();
				if (TypeId == (size_t)TypeId::VOIDF)
					return false;

				if ((TypeId & (size_t)TypeId::MASK_OBJECT) && !(TypeId & (size_t)TypeId::OBJHANDLE))
				{
					VirtualMachine* Engine = Info.GetVM();
					auto SubType = Engine->GetTypeInfoById(TypeId);
					size_t Flags = SubType.Flags();

					if ((Flags & (size_t)ObjectBehaviours::VALUE) && !(Flags & (size_t)ObjectBehaviours::POD))
					{
						bool Found = false;
						for (size_t i = 0; i < SubType.GetBehaviourCount(); i++)
						{
							Behaviours Properties;
							Function Func = SubType.GetBehaviourByIndex(i, &Properties);
							if (Properties != Behaviours::CONSTRUCT)
								continue;

							if (Func.GetArgsCount() == 0)
							{
								Found = true;
								break;
							}
						}

						if (!Found)
						{
							Engine->WriteMessage(TYPENAME_ARRAY, 0, 0, LogCategory::ERR, "The subtype has no default constructor");
							return false;
						}
					}
					else if ((Flags & (size_t)ObjectBehaviours::REF))
					{
						bool Found = false;
						if (!Engine->GetProperty(Features::DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE))
						{
							for (size_t i = 0; i < SubType.GetFactoriesCount(); i++)
							{
								Function Func = SubType.GetFactoryByIndex(i);
								if (Func.GetArgsCount() == 0)
								{
									Found = true;
									break;
								}
							}
						}

						if (!Found)
						{
							Engine->WriteMessage(TYPENAME_ARRAY, 0, 0, LogCategory::ERR, "The subtype has no default factory");
							return false;
						}
					}

					if (!(Flags & (size_t)ObjectBehaviours::GC))
						DontGarbageCollect = true;
				}
				else if (!(TypeId & (size_t)TypeId::OBJHANDLE))
				{
					DontGarbageCollect = true;
				}
				else
				{
					auto SubType = Info.GetVM()->GetTypeInfoById(TypeId);
					size_t Flags = SubType.Flags();

					if (!(Flags & (size_t)ObjectBehaviours::GC))
					{
						if ((Flags & (size_t)ObjectBehaviours::SCRIPT_OBJECT))
						{
							if ((Flags & (size_t)ObjectBehaviours::NOINHERIT))
								DontGarbageCollect = true;
						}
						else
							DontGarbageCollect = true;
					}
				}

				return true;
			}
			bool Array::IsEligibleForFind(SCache** Output) const
			{
#ifdef VI_ANGELSCRIPT
				SCache* Cache = reinterpret_cast<SCache*>(ObjType.GetUserData(GetId()));
				if (!(SubTypeId & ~asTYPEID_MASK_SEQNBR))
				{
					*Output = Cache;
					return true;
				}

				if (Cache != nullptr && Cache->Equals != nullptr)
				{
					*Output = Cache;
					return true;
				}

				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr)
				{
					if (Cache && Cache->EqualsReturnCode == asMULTIPLE_FUNCTIONS)
						Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_TEMPLATEMULTIPLEEQCOMPARATORS));
					else
						Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_TEMPLATENOEQCOMPARATORS));
				}
#endif
				*Output = nullptr;
				return false;
			}
			bool Array::IsEligibleForSort(SCache** Output) const
			{
#ifdef VI_ANGELSCRIPT
				SCache* Cache = reinterpret_cast<SCache*>(ObjType.GetUserData(GetId()));
				if (!(SubTypeId & ~asTYPEID_MASK_SEQNBR))
				{
					*Output = Cache;
					return true;
				}

				if (Cache != nullptr && Cache->Comparator != nullptr)
				{
					*Output = Cache;
					return true;
				}

				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr)
				{
					if (Cache && Cache->ComparatorReturnCode == asMULTIPLE_FUNCTIONS)
						Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_TEMPLATEMULTIPLECOMPARATORS));
					else
						Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_TEMPLATENOCOMPARATORS));
				}
#endif
				*Output = nullptr;
				return false;
			}
			int Array::GetId()
			{
				return ArrayId;
			}
			int Array::ArrayId = 1431;

			Storable::Storable() noexcept
			{
			}
			Storable::Storable(VirtualMachine* Engine, void* Value, int _TypeId) noexcept
			{
				Set(Engine, Value, _TypeId);
			}
			Storable::~Storable() noexcept
			{
				if (Value.Object && Value.TypeId)
				{
					VirtualMachine* VM = VirtualMachine::Get();
					if (VM != nullptr)
						ReleaseReferences(VM->GetEngine());
				}
			}
			void Storable::ReleaseReferences(asIScriptEngine* Engine)
			{
				if (Value.TypeId & (size_t)TypeId::MASK_OBJECT)
				{
					VirtualMachine* VM = VirtualMachine::Get(Engine);
					VM->ReleaseObject(Value.Object, VM->GetTypeInfoById(Value.TypeId));
					Value.Clean();
				}
			}
			void Storable::EnumReferences(asIScriptEngine* _Engine)
			{
				FunctionFactory::GCEnumCallback(_Engine, Value.Object);
				if (Value.TypeId)
					FunctionFactory::GCEnumCallback(_Engine, VirtualMachine::Get(_Engine)->GetTypeInfoById(Value.TypeId).GetTypeInfo());
			}
			void Storable::Set(VirtualMachine* Engine, void* Pointer, int _TypeId)
			{
				ReleaseReferences(Engine->GetEngine());
				Value.TypeId = _TypeId;

				if (Value.TypeId & (size_t)TypeId::OBJHANDLE)
				{
					Value.Object = *(void**)Pointer;
					Engine->AddRefObject(Value.Object, Engine->GetTypeInfoById(Value.TypeId));
				}
				else if (Value.TypeId & (size_t)TypeId::MASK_OBJECT)
				{
					Value.Object = Engine->CreateObjectCopy(Pointer, Engine->GetTypeInfoById(Value.TypeId));
					if (Value.Object == 0)
						Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_COPYFAIL));
				}
				else
				{
					auto Size = Engine->GetSizeOfPrimitiveType(Value.TypeId);
					if (Size)
						memcpy(&Value.Integer, Pointer, *Size);
				}
			}
			void Storable::Set(VirtualMachine* Engine, Storable& Other)
			{
				if (Other.Value.TypeId & (size_t)TypeId::OBJHANDLE)
					Set(Engine, (void*)&Other.Value.Object, Other.Value.TypeId);
				else if (Other.Value.TypeId & (size_t)TypeId::MASK_OBJECT)
					Set(Engine, (void*)Other.Value.Object, Other.Value.TypeId);
				else
					Set(Engine, (void*)&Other.Value.Integer, Other.Value.TypeId);
			}
			bool Storable::Get(VirtualMachine* Engine, void* Pointer, int _TypeId) const
			{
				if (_TypeId & (size_t)TypeId::OBJHANDLE)
				{
					if ((_TypeId & (size_t)TypeId::MASK_OBJECT))
					{
						if ((Value.TypeId & (size_t)TypeId::HANDLETOCONST) && !(_TypeId & (size_t)TypeId::HANDLETOCONST))
							return false;

						Engine->RefCastObject(Value.Object, Engine->GetTypeInfoById(Value.TypeId), Engine->GetTypeInfoById(_TypeId), reinterpret_cast<void**>(Pointer));
						return true;
					}
				}
				else if (_TypeId & (size_t)TypeId::MASK_OBJECT)
				{
					bool isCompatible = false;
					if ((Value.TypeId & ~((size_t)TypeId::OBJHANDLE | (size_t)TypeId::HANDLETOCONST)) == _TypeId && Value.Object != 0)
						isCompatible = true;

					if (isCompatible)
					{
						Engine->AssignObject(Pointer, Value.Object, Engine->GetTypeInfoById(Value.TypeId));
						return true;
					}
				}
				else
				{
					if (Value.TypeId == _TypeId)
					{
						auto Size = Engine->GetSizeOfPrimitiveType(_TypeId);
						if (Size)
							memcpy(Pointer, &Value.Integer, *Size);
						return true;
					}

					if (_TypeId == (size_t)TypeId::DOUBLE)
					{
						if (Value.TypeId == (size_t)TypeId::INT64)
						{
							*(double*)Pointer = double(Value.Integer);
						}
						else if (Value.TypeId == (size_t)TypeId::BOOL)
						{
							char Local;
							memcpy(&Local, &Value.Integer, sizeof(char));
							*(double*)Pointer = Local ? 1.0 : 0.0;
						}
						else if (Value.TypeId > (size_t)TypeId::DOUBLE && (Value.TypeId & (size_t)TypeId::MASK_OBJECT) == 0)
						{
							int Local;
							memcpy(&Local, &Value.Integer, sizeof(int));
							*(double*)Pointer = double(Local);
						}
						else
						{
							*(double*)Pointer = 0;
							return false;
						}

						return true;
					}
					else if (_TypeId == (size_t)TypeId::INT64)
					{
						if (Value.TypeId == (size_t)TypeId::DOUBLE)
						{
							*(as_int64_t*)Pointer = as_int64_t(Value.Number);
						}
						else if (Value.TypeId == (size_t)TypeId::BOOL)
						{
							char Local;
							memcpy(&Local, &Value.Integer, sizeof(char));
							*(as_int64_t*)Pointer = Local ? 1 : 0;
						}
						else if (Value.TypeId > (size_t)TypeId::DOUBLE && (Value.TypeId & (size_t)TypeId::MASK_OBJECT) == 0)
						{
							int Local;
							memcpy(&Local, &Value.Integer, sizeof(int));
							*(as_int64_t*)Pointer = Local;
						}
						else
						{
							*(as_int64_t*)Pointer = 0;
							return false;
						}

						return true;
					}
					else if (_TypeId > (size_t)TypeId::DOUBLE && (Value.TypeId & (size_t)TypeId::MASK_OBJECT) == 0)
					{
						if (Value.TypeId == (size_t)TypeId::DOUBLE)
						{
							*(int*)Pointer = int(Value.Number);
						}
						else if (Value.TypeId == (size_t)TypeId::INT64)
						{
							*(int*)Pointer = int(Value.Integer);
						}
						else if (Value.TypeId == (size_t)TypeId::BOOL)
						{
							char Local;
							memcpy(&Local, &Value.Integer, sizeof(char));
							*(int*)Pointer = Local ? 1 : 0;
						}
						else if (Value.TypeId > (size_t)TypeId::DOUBLE && (Value.TypeId & (size_t)TypeId::MASK_OBJECT) == 0)
						{
							int Local;
							memcpy(&Local, &Value.Integer, sizeof(int));
							*(int*)Pointer = Local;
						}
						else
						{
							*(int*)Pointer = 0;
							return false;
						}

						return true;
					}
					else if (_TypeId == (size_t)TypeId::BOOL)
					{
						if (Value.TypeId & (size_t)TypeId::OBJHANDLE)
						{
							*(bool*)Pointer = Value.Object ? true : false;
						}
						else if (Value.TypeId & (size_t)TypeId::MASK_OBJECT)
						{
							*(bool*)Pointer = true;
						}
						else
						{
							as_uint64_t Zero = 0;
							auto Size = Engine->GetSizeOfPrimitiveType(Value.TypeId);
							if (Size)
								*(bool*)Pointer = memcmp(&Value.Integer, &Zero, *Size) == 0 ? false : true;
						}

						return true;
					}
				}

				return false;
			}
			const void* Storable::GetAddressOfValue() const
			{
				if ((Value.TypeId & (size_t)TypeId::MASK_OBJECT) && !(Value.TypeId & (size_t)TypeId::OBJHANDLE))
					return Value.Object;

				return reinterpret_cast<const void*>(&Value.Object);
			}
			int Storable::GetTypeId() const
			{
				return Value.TypeId;
			}

			Dictionary::LocalIterator::LocalIterator(const Dictionary& From, InternalMap::const_iterator It) noexcept : It(It), Base(From)
			{
			}
			void Dictionary::LocalIterator::operator++()
			{
				++It;
			}
			void Dictionary::LocalIterator::operator++(int)
			{
				++It;
			}
			Dictionary::LocalIterator& Dictionary::LocalIterator::operator*()
			{
				return *this;
			}
			bool Dictionary::LocalIterator::operator==(const LocalIterator& Other) const
			{
				return It == Other.It;
			}
			bool Dictionary::LocalIterator::operator!=(const LocalIterator& Other) const
			{
				return It != Other.It;
			}
			const Core::String& Dictionary::LocalIterator::GetKey() const
			{
				return It->first;
			}
			int Dictionary::LocalIterator::GetTypeId() const
			{
				return It->second.Value.TypeId;
			}
			bool Dictionary::LocalIterator::GetValue(void* Pointer, int TypeId) const
			{
				return It->second.Get(Base.Engine, Pointer, TypeId);
			}
			const void* Dictionary::LocalIterator::GetAddressOfValue() const
			{
				return It->second.GetAddressOfValue();
			}

			Dictionary::Dictionary(VirtualMachine* _Engine) noexcept : Engine(_Engine)
			{
				Dictionary::SCache* Cache = reinterpret_cast<Dictionary::SCache*>(Engine->GetUserData(DictionaryId));
				Engine->NotifyOfNewObject(this, Cache->DictionaryType);
			}
			Dictionary::Dictionary(unsigned char* Buffer) noexcept : Dictionary(VirtualMachine::Get())
			{
#ifdef VI_ANGELSCRIPT
				Dictionary::SCache& Cache = *reinterpret_cast<Dictionary::SCache*>(Engine->GetUserData(DictionaryId));
				bool KeyAsRef = Cache.KeyType.Flags() & (size_t)ObjectBehaviours::REF ? true : false;
				size_t Length = *(asUINT*)Buffer;
				Buffer += 4;

				while (Length--)
				{
					if (asPWORD(Buffer) & 0x3)
						Buffer += 4 - (asPWORD(Buffer) & 0x3);

					Core::String Name;
					if (KeyAsRef)
					{
						Name = **(Core::String**)Buffer;
						Buffer += sizeof(Core::String*);
					}
					else
					{
						Name = *(Core::String*)Buffer;
						Buffer += sizeof(Core::String);
					}

					int TypeId = *(int*)Buffer;
					Buffer += sizeof(int);

					void* RefPtr = (void*)Buffer;
					if (TypeId >= (size_t)TypeId::INT8 && TypeId <= (size_t)TypeId::DOUBLE)
					{
						as_int64_t Integer64;
						double Double64;

						switch (TypeId)
						{
							case (size_t)TypeId::INT8:
								Integer64 = *(char*)RefPtr;
								break;
							case (size_t)TypeId::INT16:
								Integer64 = *(short*)RefPtr;
								break;
							case (size_t)TypeId::INT32:
								Integer64 = *(int*)RefPtr;
								break;
							case (size_t)TypeId::UINT8:
								Integer64 = *(unsigned char*)RefPtr;
								break;
							case (size_t)TypeId::UINT16:
								Integer64 = *(unsigned short*)RefPtr;
								break;
							case (size_t)TypeId::UINT32:
								Integer64 = *(uint32_t*)RefPtr;
								break;
							case (size_t)TypeId::INT64:
							case (size_t)TypeId::UINT64:
								Integer64 = *(as_int64_t*)RefPtr;
								break;
							case (size_t)TypeId::FLOAT:
								Double64 = *(float*)RefPtr;
								break;
							case (size_t)TypeId::DOUBLE:
								Double64 = *(double*)RefPtr;
								break;
						}

						if (TypeId >= (size_t)TypeId::FLOAT)
							Set(Name, &Double64, (size_t)TypeId::DOUBLE);
						else
							Set(Name, &Integer64, (size_t)TypeId::INT64);
					}
					else
					{
						if ((TypeId & (size_t)TypeId::MASK_OBJECT) && !(TypeId & (size_t)TypeId::OBJHANDLE) && (Engine->GetTypeInfoById(TypeId).Flags() & (size_t)ObjectBehaviours::REF))
							RefPtr = *(void**)RefPtr;

						Set(Name, RefPtr, Engine->IsNullable(TypeId) ? 0 : TypeId);
					}

					if (TypeId & (size_t)TypeId::MASK_OBJECT)
					{
						auto Info = Engine->GetTypeInfoById(TypeId);
						if (Info.Flags() & (size_t)ObjectBehaviours::VALUE)
							Buffer += Info.Size();
						else
							Buffer += sizeof(void*);
					}
					else if (TypeId == 0)
						Buffer += sizeof(void*);
					else
						Buffer += Engine->GetSizeOfPrimitiveType(TypeId).Or(0);
				}
#endif
			}
			Dictionary::Dictionary(const Dictionary& Other) noexcept : Dictionary(Other.Engine)
			{
				for (auto It = Other.Data.begin(); It != Other.Data.end(); ++It)
				{
					auto& Key = It->second;
					if (Key.Value.TypeId & (size_t)TypeId::OBJHANDLE)
						Set(It->first, (void*)&Key.Value.Object, Key.Value.TypeId);
					else if (Key.Value.TypeId & (size_t)TypeId::MASK_OBJECT)
						Set(It->first, (void*)Key.Value.Object, Key.Value.TypeId);
					else
						Set(It->first, (void*)&Key.Value.Integer, Key.Value.TypeId);
				}
			}
			Dictionary::~Dictionary() noexcept
			{
				Clear();
			}
			void Dictionary::EnumReferences(asIScriptEngine* _Engine)
			{
				for (auto It = Data.begin(); It != Data.end(); ++It)
				{
					auto& Key = It->second;
					if (Key.Value.TypeId & (size_t)TypeId::MASK_OBJECT)
					{
						auto SubType = Engine->GetTypeInfoById(Key.Value.TypeId);
						if ((SubType.Flags() & (size_t)ObjectBehaviours::VALUE) && (SubType.Flags() & (size_t)ObjectBehaviours::GC))
							Engine->ForwardEnumReferences(Key.Value.Object, SubType);
						else
							FunctionFactory::GCEnumCallback(_Engine, Key.Value.Object);
					}
				}
			}
			void Dictionary::ReleaseReferences(asIScriptEngine*)
			{
				Clear();
			}
			Dictionary& Dictionary::operator =(const Dictionary& Other) noexcept
			{
				Clear();
				for (auto It = Other.Data.begin(); It != Other.Data.end(); ++It)
				{
					auto& Key = It->second;
					if (Key.Value.TypeId & (size_t)TypeId::OBJHANDLE)
						Set(It->first, (void*)&Key.Value.Object, Key.Value.TypeId);
					else if (Key.Value.TypeId & (size_t)TypeId::MASK_OBJECT)
						Set(It->first, (void*)Key.Value.Object, Key.Value.TypeId);
					else
						Set(It->first, (void*)&Key.Value.Integer, Key.Value.TypeId);
				}

				return *this;
			}
			Storable* Dictionary::operator[](const std::string_view& Key)
			{
				auto It = Data.find(Core::KeyLookupCast(Key));
				if (It != Data.end())
					return &It->second;

				return &Data[Core::String(Key)];
			}
			const Storable* Dictionary::operator[](const std::string_view& Key) const
			{
				auto It = Data.find(Core::KeyLookupCast(Key));
				if (It != Data.end())
					return &It->second;

				Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_ACCESSINVALID));
				return 0;
			}
			Storable* Dictionary::operator[](size_t Index)
			{
				if (Index < Data.size())
				{
					auto It = Data.begin();
					while (Index--)
						It++;
					return &It->second;
				}

				Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_ACCESSINVALID));
				return 0;
			}
			const Storable* Dictionary::operator[](size_t Index) const
			{
				if (Index < Data.size())
				{
					auto It = Data.begin();
					while (Index--)
						It++;
					return &It->second;
				}

				Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_ACCESSINVALID));
				return 0;
			}
			void Dictionary::Set(const std::string_view& Key, void* Value, int TypeId)
			{
				auto It = Data.find(Core::KeyLookupCast(Key));
				if (It == Data.end())
					It = Data.insert(InternalMap::value_type(Key, Storable())).first;

				It->second.Set(Engine, Value, TypeId);
			}
			bool Dictionary::Get(const std::string_view& Key, void* Value, int TypeId) const
			{
				auto It = Data.find(Core::KeyLookupCast(Key));
				if (It != Data.end())
					return It->second.Get(Engine, Value, TypeId);

				return false;
			}
			bool Dictionary::GetIndex(size_t Index, Core::String* Key, void** Value, int* TypeId) const
			{
				if (Index >= Data.size())
					return false;

				auto It = Begin();
				size_t Offset = 0;

				while (Offset != Index)
				{
					++Offset;
					++It;
				}

				if (Key != nullptr)
					*Key = It.GetKey();

				if (Value != nullptr)
					*Value = (void*)It.GetAddressOfValue();

				if (TypeId != nullptr)
					*TypeId = It.GetTypeId();

				return true;
			}
			bool Dictionary::TryGetIndex(size_t Index, Core::String* Key, void* Value, int TypeId) const
			{
				if (Index >= Data.size())
					return false;

				auto It = Begin();
				size_t Offset = 0;

				while (Offset != Index)
				{
					++Offset;
					++It;
				}

				return It.GetValue(Value, TypeId);
			}
			int Dictionary::GetTypeId(const std::string_view& Key) const
			{
				auto It = Data.find(Core::KeyLookupCast(Key));
				if (It != Data.end())
					return It->second.Value.TypeId;

				return -1;
			}
			bool Dictionary::Exists(const std::string_view& Key) const
			{
				auto It = Data.find(Core::KeyLookupCast(Key));
				if (It != Data.end())
					return true;

				return false;
			}
			bool Dictionary::Empty() const
			{
				if (Data.size() == 0)
					return true;

				return false;
			}
			size_t Dictionary::Size() const
			{
				return size_t(Data.size());
			}
			bool Dictionary::Erase(const std::string_view& Key)
			{
				auto It = Data.find(Core::KeyLookupCast(Key));
				if (It != Data.end())
				{
					It->second.ReleaseReferences(Engine->GetEngine());
					Data.erase(It);
					return true;
				}

				return false;
			}
			void Dictionary::Clear()
			{
				asIScriptEngine* Target = Engine->GetEngine();
				for (auto It = Data.begin(); It != Data.end(); ++It)
					It->second.ReleaseReferences(Target);
				Data.clear();
			}
			Array* Dictionary::GetKeys() const
			{
				Dictionary::SCache* Cache = reinterpret_cast<Dictionary::SCache*>(Engine->GetUserData(DictionaryId));
				Array* Array = Array::Create(Cache->ArrayType.GetTypeInfo(), size_t(Data.size()));
				size_t Current = 0;

				for (auto It = Data.begin(); It != Data.end(); ++It)
					*(Core::String*)Array->At(Current++) = It->first;

				return Array;
			}
			Dictionary::LocalIterator Dictionary::Begin() const
			{
				return LocalIterator(*this, Data.begin());
			}
			Dictionary::LocalIterator Dictionary::End() const
			{
				return LocalIterator(*this, Data.end());
			}
			Dictionary::LocalIterator Dictionary::Find(const std::string_view& Key) const
			{
				return LocalIterator(*this, Data.find(Core::KeyLookupCast(Key)));
			}
			Dictionary* Dictionary::Create(VirtualMachine* Engine)
			{
				Dictionary* Result = new Dictionary(Engine);
				if (!Result)
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));

				return Result;
			}
			Dictionary* Dictionary::Create(unsigned char* Buffer)
			{
				Dictionary* Result = new Dictionary(Buffer);
				if (!Result)
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));

				return Result;
			}
			void Dictionary::Setup(VirtualMachine* Engine)
			{
				Dictionary::SCache* Cache = reinterpret_cast<Dictionary::SCache*>(Engine->GetUserData(DictionaryId));
				if (Cache == 0)
				{
					Cache = Core::Memory::New<Dictionary::SCache>();
					Engine->SetUserData(Cache, DictionaryId);
					Engine->SetEngineUserDataCleanupCallback([](asIScriptEngine* Engine)
					{
						VirtualMachine* VM = VirtualMachine::Get(Engine);
						Dictionary::SCache* Cache = reinterpret_cast<Dictionary::SCache*>(VM->GetUserData(DictionaryId));
						Core::Memory::Delete(Cache);
					}, DictionaryId);

					Cache->DictionaryType = Engine->GetTypeInfoByName(TYPENAME_DICTIONARY).GetTypeInfo();
					Cache->ArrayType = Engine->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">").GetTypeInfo();
					Cache->KeyType = Engine->GetTypeInfoByDecl(TYPENAME_STRING).GetTypeInfo();
				}
			}
			void Dictionary::Factory(asIScriptGeneric* Generic)
			{
				GenericContext Args = Generic;
				*(Dictionary**)Args.GetAddressOfReturnLocation() = Dictionary::Create(Args.GetVM());
			}
			void Dictionary::ListFactory(asIScriptGeneric* Generic)
			{
				GenericContext Args = Generic;
				unsigned char* Buffer = (unsigned char*)Args.GetArgAddress(0);
				*(Dictionary**)Args.GetAddressOfReturnLocation() = Dictionary::Create(Buffer);
			}
			void Dictionary::KeyCreate(void* Memory)
			{
				new(Memory) Storable();
			}
			void Dictionary::KeyDestroy(Storable* Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (VM != nullptr)
					Base->ReleaseReferences(VM->GetEngine());

				Base->~Storable();
			}
			Storable& Dictionary::KeyopAssign(Storable* Base, void* RefPtr, int TypeId)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (VM != nullptr)
					Base->Set(VM, RefPtr, TypeId);

				return *Base;
			}
			Storable& Dictionary::KeyopAssign(Storable* Base, const Storable& Other)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (VM != nullptr)
					Base->Set(VM, const_cast<Storable&>(Other));

				return *Base;
			}
			Storable& Dictionary::KeyopAssign(Storable* Base, double Value)
			{
				return KeyopAssign(Base, &Value, (size_t)TypeId::DOUBLE);
			}
			Storable& Dictionary::KeyopAssign(Storable* Base, as_int64_t Value)
			{
				return Dictionary::KeyopAssign(Base, &Value, (size_t)TypeId::INT64);
			}
			void Dictionary::KeyopCast(Storable* Base, void* RefPtr, int TypeId)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (VM != nullptr)
					Base->Get(VM, RefPtr, TypeId);
			}
			as_int64_t Dictionary::KeyopConvInt(Storable* Base)
			{
				as_int64_t Value;
				KeyopCast(Base, &Value, (size_t)TypeId::INT64);
				return Value;
			}
			double Dictionary::KeyopConvDouble(Storable* Base)
			{
				double Value;
				KeyopCast(Base, &Value, (size_t)TypeId::DOUBLE);
				return Value;
			}
			int Dictionary::DictionaryId = 2348;

			Core::String Random::Getb(uint64_t Size)
			{
				return Compute::Codec::HexEncode(*Compute::Crypto::RandomBytes((size_t)Size)).substr(0, (size_t)Size);
			}
			double Random::Betweend(double Min, double Max)
			{
				return Compute::Mathd::Random(Min, Max);
			}
			double Random::Magd()
			{
				return Compute::Mathd::RandomMag();
			}
			double Random::Getd()
			{
				return Compute::Mathd::Random();
			}
			float Random::Betweenf(float Min, float Max)
			{
				return Compute::Mathf::Random(Min, Max);
			}
			float Random::Magf()
			{
				return Compute::Mathf::RandomMag();
			}
			float Random::Getf()
			{
				return Compute::Mathf::Random();
			}
			uint64_t Random::Betweeni(uint64_t Min, uint64_t Max)
			{
				return Compute::Math<uint64_t>::Random(Min, Max);
			}

			Promise::Promise(ImmediateContext* NewContext) noexcept : Engine(nullptr), Context(NewContext), Value(-1)
			{
				VI_ASSERT(Context != nullptr, "context should not be null");
				Context->AddRef();
				Engine = Context->GetVM();
				Engine->NotifyOfNewObject(this, Engine->GetTypeInfoByName(TYPENAME_PROMISE));
			}
			Promise::~Promise() noexcept
			{
				ReleaseReferences(nullptr);
			}
			void Promise::EnumReferences(asIScriptEngine* OtherEngine)
			{
				if (Value.Object != nullptr && (Value.TypeId & (size_t)TypeId::MASK_OBJECT))
				{
					auto SubType = Engine->GetTypeInfoById(Value.TypeId);
					if ((SubType.Flags() & (size_t)ObjectBehaviours::REF))
						FunctionFactory::GCEnumCallback(OtherEngine, Value.Object);
					else if ((SubType.Flags() & (size_t)ObjectBehaviours::VALUE) && (SubType.Flags() & (size_t)ObjectBehaviours::GC))
						Engine->ForwardEnumReferences(Value.Object, SubType);

					auto Type = VirtualMachine::Get(OtherEngine)->GetTypeInfoById(Value.TypeId);
					FunctionFactory::GCEnumCallback(OtherEngine, Type.GetTypeInfo());
				}
				FunctionFactory::GCEnumCallback(OtherEngine, &Delegate);
				FunctionFactory::GCEnumCallback(OtherEngine, Context);
			}
			void Promise::ReleaseReferences(asIScriptEngine*)
			{
				if (Value.TypeId & (size_t)TypeId::MASK_OBJECT)
				{
					auto Type = Engine->GetTypeInfoById(Value.TypeId);
					Engine->ReleaseObject(Value.Object, Type);
					Type.Release();
					Value.Clean();
				}
				if (Delegate.IsValid())
					Delegate.Release();
				Core::Memory::Release(Context);
			}
			int Promise::GetTypeId()
			{
				return Value.TypeId;
			}
			void* Promise::GetAddressOfObject()
			{
				return Retrieve();
			}
			void Promise::When(asIScriptFunction* NewCallback)
			{
				Core::UMutex<std::mutex> Unique(Update);
				Delegate = FunctionDelegate(NewCallback);
				if (Delegate.IsValid() && !IsPending())
				{
					auto DelegateReturn = std::move(Delegate);
					Unique.Negate();
					Context->ResolveCallback(std::move(DelegateReturn), [this](ImmediateContext* Context) { Context->SetArgObject(0, this); }, nullptr);
				}
			}
			void Promise::When(std::function<void(Promise*)>&& NewCallback)
			{
				Core::UMutex<std::mutex> Unique(Update);
				Bounce = std::move(NewCallback);
				if (Bounce && !IsPending())
				{
					auto NativeReturn = std::move(Bounce);
					Unique.Negate();
					NativeReturn(this);
				}
			}
			void Promise::Store(void* RefPointer, int RefTypeId)
			{
				VI_ASSERT(Value.TypeId == PromiseNULL, "promise should be settled only once");
				VI_ASSERT(RefPointer != nullptr || RefTypeId == (size_t)TypeId::VOIDF, "input pointer should not be null");
				VI_ASSERT(Engine != nullptr, "promise is malformed (engine is null)");
				VI_ASSERT(Context != nullptr, "promise is malformed (context is null)");

				Core::UMutex<std::mutex> Unique(Update);
				if (Value.TypeId != PromiseNULL)
					return Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_PROMISEREADY));

				if ((RefTypeId & (size_t)TypeId::MASK_OBJECT))
				{
					auto Type = Engine->GetTypeInfoById(RefTypeId);
					if (Type.IsValid())
						Type.AddRef();
				}

				Value.TypeId = RefTypeId;
				if (Value.TypeId & (size_t)TypeId::OBJHANDLE)
				{
					Value.Object = *(void**)RefPointer;
				}
				else if (Value.TypeId & (size_t)TypeId::MASK_OBJECT)
				{
					Value.Object = Engine->CreateObjectCopy(RefPointer, Engine->GetTypeInfoById(Value.TypeId));
				}
				else if (RefPointer != nullptr)
				{
					Value.Integer = 0;
					auto Size = Engine->GetSizeOfPrimitiveType(Value.TypeId);
					if (Size)
						memcpy(&Value.Integer, RefPointer, *Size);
				}

				bool SuspendOwned = Context->GetUserData(PromiseUD) == (void*)this;
				if (SuspendOwned)
					Context->SetUserData(nullptr, PromiseUD);

				bool WantsResume = (Context->IsSuspended() && SuspendOwned);
				auto NativeReturn = std::move(Bounce);
				auto DelegateReturn = std::move(Delegate);
				Unique.Negate();

				if (NativeReturn)
					NativeReturn(this);

				if (DelegateReturn.IsValid())
					Context->ResolveCallback(std::move(DelegateReturn), [this](ImmediateContext* Context) { Context->SetArgObject(0, this); }, nullptr);

				if (WantsResume)
					Context->ResolveNotification();
			}
			void Promise::Store(void* RefPointer, const char* TypeName)
			{
				VI_ASSERT(Engine != nullptr, "promise is malformed (engine is null)");
				VI_ASSERT(TypeName != nullptr, "typename should not be null");
				Store(RefPointer, Engine->GetTypeIdByDecl(TypeName));
			}
			void Promise::StoreException(const Exception::Pointer& RefValue)
			{
				VI_ASSERT(Context != nullptr, "promise is malformed (context is null)");
				Context->EnableDeferredExceptions();
				Exception::ThrowAt(Context, RefValue);
				Context->DisableDeferredExceptions();
				Store(nullptr, (int)TypeId::VOIDF);
			}
			void Promise::StoreVoid()
			{
				Store(nullptr, (int)TypeId::VOIDF);
			}
			bool Promise::Retrieve(void* RefPointer, int RefTypeId)
			{
				VI_ASSERT(Engine != nullptr, "promise is malformed (engine is null)");
				VI_ASSERT(RefPointer != nullptr, "output pointer should not be null");
				if (Value.TypeId == PromiseNULL)
					return false;

				if (RefTypeId & (size_t)TypeId::OBJHANDLE)
				{
					if ((Value.TypeId & (size_t)TypeId::MASK_OBJECT))
					{
						if ((Value.TypeId & (size_t)TypeId::HANDLETOCONST) && !(RefTypeId & (size_t)TypeId::HANDLETOCONST))
							return false;

						Engine->RefCastObject(Value.Object, Engine->GetTypeInfoById(Value.TypeId), Engine->GetTypeInfoById(RefTypeId), reinterpret_cast<void**>(RefPointer));
#ifdef VI_ANGELSCRIPT
						if (*(asPWORD*)RefPointer == 0)
							return false;
#endif
						return true;
					}
				}
				else if (RefTypeId & (size_t)TypeId::MASK_OBJECT)
				{
					if (Value.TypeId == RefTypeId)
					{
						Engine->AssignObject(RefPointer, Value.Object, Engine->GetTypeInfoById(Value.TypeId));
						return true;
					}
				}
				else
				{
					auto Size1 = Engine->GetSizeOfPrimitiveType(Value.TypeId);
					auto Size2 = Engine->GetSizeOfPrimitiveType(RefTypeId);
					if (Size1 && Size2 && *Size1 == *Size2)
					{
						memcpy(RefPointer, &Value.Integer, *Size1);
						return true;
					}
				}

				return false;
			}
			void Promise::RetrieveVoid()
			{
				Core::UMutex<std::mutex> Unique(Update);
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr && !Context->RethrowDeferredException())
				{
					if (Value.TypeId == PromiseNULL)
						Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_PROMISENOTREADY));
				}
			}
			void* Promise::Retrieve()
			{
				RetrieveVoid();
				if (Value.TypeId == PromiseNULL)
					return nullptr;

				if (Value.TypeId & (size_t)TypeId::OBJHANDLE)
					return &Value.Object;
				else if (Value.TypeId & (size_t)TypeId::MASK_OBJECT)
					return Value.Object;
				else if (Value.TypeId <= (size_t)TypeId::DOUBLE || Value.TypeId & (size_t)TypeId::MASK_SEQNBR)
					return &Value.Integer;

				return nullptr;
			}
			bool Promise::IsPending()
			{
				return Value.TypeId == PromiseNULL;
			}
			Promise* Promise::YieldIf()
			{
				Core::UMutex<std::mutex> Unique(Update);
				if (Value.TypeId == PromiseNULL && Context != nullptr && Context->Suspend())
					Context->SetUserData(this, PromiseUD);

				return this;
			}
			Promise* Promise::CreateFactory(void* _Ref, int TypeId)
			{
				Promise* Future = new Promise(ImmediateContext::Get());
				if (!Future)
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));

				if (TypeId != (size_t)TypeId::VOIDF)
					Future->Store(_Ref, TypeId);
				return Future;
			}
			Promise* Promise::CreateFactoryType(asITypeInfo*)
			{
				Promise* Future = new Promise(ImmediateContext::Get());
				if (!Future)
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));
				return Future;
			}
			Promise* Promise::CreateFactoryVoid()
			{
				Promise* Future = new Promise(ImmediateContext::Get());
				if (!Future)
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));
				return Future;
			}
			bool Promise::TemplateCallback(asITypeInfo* InfoContext, bool& DontGarbageCollect)
			{
				TypeInfo Info(InfoContext);
				int TypeId = Info.GetSubTypeId();
				if (TypeId == (size_t)TypeId::VOIDF)
					return false;

				if ((TypeId & (size_t)TypeId::MASK_OBJECT) && !(TypeId & (size_t)TypeId::OBJHANDLE))
				{
					VirtualMachine* Engine = Info.GetVM();
					auto SubType = Engine->GetTypeInfoById(TypeId);
					size_t Flags = SubType.Flags();

					if (!(Flags & (size_t)ObjectBehaviours::GC))
						DontGarbageCollect = true;
				}
				else if (!(TypeId & (size_t)TypeId::OBJHANDLE))
				{
					DontGarbageCollect = true;
				}
				else
				{
					auto SubType = Info.GetVM()->GetTypeInfoById(TypeId);
					size_t Flags = SubType.Flags();

					if (!(Flags & (size_t)ObjectBehaviours::GC))
					{
						if ((Flags & (size_t)ObjectBehaviours::SCRIPT_OBJECT))
						{
							if ((Flags & (size_t)ObjectBehaviours::NOINHERIT))
								DontGarbageCollect = true;
						}
						else
							DontGarbageCollect = true;
					}
				}

				return true;
			}
			ExpectsVM<void> Promise::GeneratorCallback(Compute::Preprocessor*, const std::string_view&, Core::String& Code)
			{
				Core::Stringify::Replace(Code, "promise<void>", "promise_v");
				return Parser::ReplaceInlinePreconditions("co_await", Code, [](const std::string_view& Expression) -> ExpectsVM<Core::String>
				{
					const char Generator[] = ").yield().unwrap()";
					Core::String Result = "(";
					Result.reserve(sizeof(Generator) + Expression.size());
					Result.append(Expression);
					Result.append(Generator, sizeof(Generator) - 1);
					return Result;
				});
			}
			bool Promise::IsContextPending(ImmediateContext* Context)
			{
				return Context->IsPending() || Context->GetUserData(PromiseUD) != nullptr || Context->IsSuspended();
			}
			bool Promise::IsContextBusy(ImmediateContext* Context)
			{
				return IsContextPending(Context) || Context->GetState() == Execution::Active;
			}
			int Promise::PromiseNULL = -1;
			int Promise::PromiseUD = 559;

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
			Core::Decimal& DecimalFPP(Core::Decimal& Base)
			{
				return ++Base;
			}
			Core::Decimal& DecimalFMM(Core::Decimal& Base)
			{
				return --Base;
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

			int8_t UInt128ToInt8(Compute::UInt128& Value)
			{
				return (int8_t)(uint8_t)Value;
			}
			uint8_t UInt128ToUInt8(Compute::UInt128& Value)
			{
				return (uint8_t)Value;
			}
			int16_t UInt128ToInt16(Compute::UInt128& Value)
			{
				return (int16_t)(uint16_t)Value;
			}
			uint16_t UInt128ToUInt16(Compute::UInt128& Value)
			{
				return (uint16_t)Value;
			}
			int32_t UInt128ToInt32(Compute::UInt128& Value)
			{
				return (int32_t)(uint32_t)Value;
			}
			uint32_t UInt128ToUInt32(Compute::UInt128& Value)
			{
				return (uint32_t)Value;
			}
			int64_t UInt128ToInt64(Compute::UInt128& Value)
			{
				return (int64_t)(uint64_t)Value;
			}
			uint64_t UInt128ToUInt64(Compute::UInt128& Value)
			{
				return (uint64_t)Value;
			}
			Compute::UInt128& UInt128MulEq(Compute::UInt128& Base, const Compute::UInt128& V)
			{
				Base *= V;
				return Base;
			}
			Compute::UInt128& UInt128DivEq(Compute::UInt128& Base, const Compute::UInt128& V)
			{
				Base /= V;
				return Base;
			}
			Compute::UInt128& UInt128AddEq(Compute::UInt128& Base, const Compute::UInt128& V)
			{
				Base += V;
				return Base;
			}
			Compute::UInt128& UInt128SubEq(Compute::UInt128& Base, const Compute::UInt128& V)
			{
				Base -= V;
				return Base;
			}
			Compute::UInt128& UInt128FPP(Compute::UInt128& Base)
			{
				return ++Base;
			}
			Compute::UInt128& UInt128FMM(Compute::UInt128& Base)
			{
				return --Base;
			}
			Compute::UInt128& UInt128PP(Compute::UInt128& Base)
			{
				Base++;
				return Base;
			}
			Compute::UInt128& UInt128MM(Compute::UInt128& Base)
			{
				Base--;
				return Base;
			}
			bool UInt128Eq(Compute::UInt128& Base, const Compute::UInt128& Right)
			{
				return Base == Right;
			}
			int UInt128Cmp(Compute::UInt128& Base, const Compute::UInt128& Right)
			{
				if (Base == Right)
					return 0;

				return Base > Right ? 1 : -1;
			}
			Compute::UInt128 UInt128Add(const Compute::UInt128& Left, const Compute::UInt128& Right)
			{
				return Left + Right;
			}
			Compute::UInt128 UInt128Sub(const Compute::UInt128& Left, const Compute::UInt128& Right)
			{
				return Left - Right;
			}
			Compute::UInt128 UInt128Mul(const Compute::UInt128& Left, const Compute::UInt128& Right)
			{
				return Left * Right;
			}
			Compute::UInt128 UInt128Div(const Compute::UInt128& Left, const Compute::UInt128& Right)
			{
				return Left / Right;
			}
			Compute::UInt128 UInt128Per(const Compute::UInt128& Left, const Compute::UInt128& Right)
			{
				return Left % Right;
			}

			int8_t UInt256ToInt8(Compute::UInt256& Value)
			{
				return (int8_t)(uint8_t)Value;
			}
			uint8_t UInt256ToUInt8(Compute::UInt256& Value)
			{
				return (uint8_t)Value;
			}
			int16_t UInt256ToInt16(Compute::UInt256& Value)
			{
				return (int16_t)(uint16_t)Value;
			}
			uint16_t UInt256ToUInt16(Compute::UInt256& Value)
			{
				return (uint16_t)Value;
			}
			int32_t UInt256ToInt32(Compute::UInt256& Value)
			{
				return (int32_t)(uint32_t)Value;
			}
			uint32_t UInt256ToUInt32(Compute::UInt256& Value)
			{
				return (uint32_t)Value;
			}
			int64_t UInt256ToInt64(Compute::UInt256& Value)
			{
				return (int64_t)(uint64_t)Value;
			}
			uint64_t UInt256ToUInt64(Compute::UInt256& Value)
			{
				return (uint64_t)Value;
			}
			Compute::UInt256& UInt256MulEq(Compute::UInt256& Base, const Compute::UInt256& V)
			{
				Base *= V;
				return Base;
			}
			Compute::UInt256& UInt256DivEq(Compute::UInt256& Base, const Compute::UInt256& V)
			{
				Base /= V;
				return Base;
			}
			Compute::UInt256& UInt256AddEq(Compute::UInt256& Base, const Compute::UInt256& V)
			{
				Base += V;
				return Base;
			}
			Compute::UInt256& UInt256SubEq(Compute::UInt256& Base, const Compute::UInt256& V)
			{
				Base -= V;
				return Base;
			}
			Compute::UInt256& UInt256FPP(Compute::UInt256& Base)
			{
				return ++Base;
			}
			Compute::UInt256& UInt256FMM(Compute::UInt256& Base)
			{
				return --Base;
			}
			Compute::UInt256& UInt256PP(Compute::UInt256& Base)
			{
				Base++;
				return Base;
			}
			Compute::UInt256& UInt256MM(Compute::UInt256& Base)
			{
				Base--;
				return Base;
			}
			bool UInt256Eq(Compute::UInt256& Base, const Compute::UInt256& Right)
			{
				return Base == Right;
			}
			int UInt256Cmp(Compute::UInt256& Base, const Compute::UInt256& Right)
			{
				if (Base == Right)
					return 0;

				return Base > Right ? 1 : -1;
			}
			Compute::UInt256 UInt256Add(const Compute::UInt256& Left, const Compute::UInt256& Right)
			{
				return Left + Right;
			}
			Compute::UInt256 UInt256Sub(const Compute::UInt256& Left, const Compute::UInt256& Right)
			{
				return Left - Right;
			}
			Compute::UInt256 UInt256Mul(const Compute::UInt256& Left, const Compute::UInt256& Right)
			{
				return Left * Right;
			}
			Compute::UInt256 UInt256Div(const Compute::UInt256& Left, const Compute::UInt256& Right)
			{
				return Left / Right;
			}
			Compute::UInt256 UInt256Per(const Compute::UInt256& Left, const Compute::UInt256& Right)
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

			size_t VariantGetSize(Core::Variant& Base)
			{
				return Base.Size();
			}
			bool VariantEquals(Core::Variant& Base, const Core::Variant& Other)
			{
				return Base == Other;
			}
			bool VariantImplCast(Core::Variant& Base)
			{
				return (bool)Base;
			}
			int8_t VariantToInt8(Core::Variant& Base)
			{
				return (int8_t)Base.GetInteger();
			}
			int16_t VariantToInt16(Core::Variant& Base)
			{
				return (int16_t)Base.GetInteger();
			}
			int32_t VariantToInt32(Core::Variant& Base)
			{
				return (int32_t)Base.GetInteger();
			}
			int64_t VariantToInt64(Core::Variant& Base)
			{
				return (int64_t)Base.GetInteger();
			}
			uint8_t VariantToUInt8(Core::Variant& Base)
			{
				return (uint8_t)Base.GetInteger();
			}
			uint16_t VariantToUInt16(Core::Variant& Base)
			{
				return (int16_t)Base.GetInteger();
			}
			uint32_t VariantToUInt32(Core::Variant& Base)
			{
				return (uint32_t)Base.GetInteger();
			}
			uint64_t VariantToUInt64(Core::Variant& Base)
			{
				return (uint64_t)Base.GetInteger();
			}

			int8_t SchemaToInt8(Core::Schema* Base)
			{
				return (int8_t)Base->Value.GetInteger();
			}
			int16_t SchemaToInt16(Core::Schema* Base)
			{
				return (int16_t)Base->Value.GetInteger();
			}
			int32_t SchemaToInt32(Core::Schema* Base)
			{
				return (int32_t)Base->Value.GetInteger();
			}
			int64_t SchemaToInt64(Core::Schema* Base)
			{
				return (int64_t)Base->Value.GetInteger();
			}
			uint8_t SchemaToUInt8(Core::Schema* Base)
			{
				return (uint8_t)Base->Value.GetInteger();
			}
			uint16_t SchemaToUInt16(Core::Schema* Base)
			{
				return (int16_t)Base->Value.GetInteger();
			}
			uint32_t SchemaToUInt32(Core::Schema* Base)
			{
				return (uint32_t)Base->Value.GetInteger();
			}
			uint64_t SchemaToUInt64(Core::Schema* Base)
			{
				return (uint64_t)Base->Value.GetInteger();
			}
			void SchemaNotifyAllReferences(Core::Schema* Base, VirtualMachine* Engine, asITypeInfo* Type)
			{
				Engine->NotifyOfNewObject(Base, Type);
				for (auto* Item : Base->GetChilds())
					SchemaNotifyAllReferences(Item, Engine, Type);
			}
			Core::Schema* SchemaInit(Core::Schema* Base)
			{
				return Base;
			}
			Core::Schema* SchemaConstructCopy(Core::Schema* Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				auto* New = Base->Copy();
				SchemaNotifyAllReferences(New, VM, VM->GetTypeInfoByName(TYPENAME_SCHEMA).GetTypeInfo());
				return New;
			}
			Core::Schema* SchemaConstructBuffer(unsigned char* Buffer)
			{
#ifdef VI_ANGELSCRIPT
				if (!Buffer)
					return nullptr;

				Core::Schema* Result = Core::Var::Set::Object();
				VirtualMachine* VM = VirtualMachine::Get();
				size_t Length = *(asUINT*)Buffer;
				Buffer += 4;

				while (Length--)
				{
					if (asPWORD(Buffer) & 0x3)
						Buffer += 4 - (asPWORD(Buffer) & 0x3);

					Core::String Name = *(Core::String*)Buffer;
					Buffer += sizeof(Core::String);

					int TypeId = *(int*)Buffer;
					Buffer += sizeof(int);

					void* Ref = (void*)Buffer;
					if (TypeId >= (size_t)TypeId::BOOL && TypeId <= (size_t)TypeId::DOUBLE)
					{
						switch (TypeId)
						{
							case (size_t)TypeId::BOOL:
								Result->Set(Name, Core::Var::Boolean(*(bool*)Ref));
								break;
							case (size_t)TypeId::INT8:
								Result->Set(Name, Core::Var::Integer(*(char*)Ref));
								break;
							case (size_t)TypeId::INT16:
								Result->Set(Name, Core::Var::Integer(*(short*)Ref));
								break;
							case (size_t)TypeId::INT32:
								Result->Set(Name, Core::Var::Integer(*(int*)Ref));
								break;
							case (size_t)TypeId::UINT8:
								Result->Set(Name, Core::Var::Integer(*(unsigned char*)Ref));
								break;
							case (size_t)TypeId::UINT16:
								Result->Set(Name, Core::Var::Integer(*(unsigned short*)Ref));
								break;
							case (size_t)TypeId::UINT32:
								Result->Set(Name, Core::Var::Integer(*(uint32_t*)Ref));
								break;
							case (size_t)TypeId::INT64:
							case (size_t)TypeId::UINT64:
								Result->Set(Name, Core::Var::Integer(*(int64_t*)Ref));
								break;
							case (size_t)TypeId::FLOAT:
								Result->Set(Name, Core::Var::Number(*(float*)Ref));
								break;
							case (size_t)TypeId::DOUBLE:
								Result->Set(Name, Core::Var::Number(*(double*)Ref));
								break;
						}
					}
					else
					{
						auto Type = VM->GetTypeInfoById(TypeId);
						if ((TypeId & (size_t)TypeId::MASK_OBJECT) && !(TypeId & (size_t)TypeId::OBJHANDLE) && (Type.IsValid() && Type.Flags() & (size_t)ObjectBehaviours::REF))
							Ref = *(void**)Ref;

						if (TypeId & (size_t)TypeId::OBJHANDLE)
							Ref = *(void**)Ref;

						if (VM->IsNullable(TypeId) || !Ref)
						{
							Result->Set(Name, Core::Var::Null());
						}
						else if (Type.IsValid() && Type.GetName() == TYPENAME_SCHEMA)
						{
							Core::Schema* Base = (Core::Schema*)Ref;
							if (Base->GetParent() != Result)
								Base->AddRef();

							Result->Set(Name, Base);
						}
						else if (Type.IsValid() && Type.GetName() == TYPENAME_STRING)
							Result->Set(Name, Core::Var::String(*(Core::String*)Ref));
						else if (Type.IsValid() && Type.GetName() == TYPENAME_DECIMAL)
							Result->Set(Name, Core::Var::Decimal(*(Core::Decimal*)Ref));
					}

					if (TypeId & (size_t)TypeId::MASK_OBJECT)
					{
						auto Type = VM->GetTypeInfoById(TypeId);
						if (Type.Flags() & (size_t)ObjectBehaviours::VALUE)
							Buffer += Type.Size();
						else
							Buffer += sizeof(void*);
					}
					else if (TypeId != 0)
						Buffer += VM->GetSizeOfPrimitiveType(TypeId).Or(0);
					else
						Buffer += sizeof(void*);
				}

				return Result;
#else
				return nullptr;
#endif
			}
			void SchemaConstruct(asIScriptGeneric* Generic)
			{
				GenericContext Args = Generic;
				VirtualMachine* VM = Args.GetVM();
				unsigned char* Buffer = (unsigned char*)Args.GetArgAddress(0);
				Core::Schema* New = SchemaConstructBuffer(Buffer);
				SchemaNotifyAllReferences(New, VM, VM->GetTypeInfoByName(TYPENAME_SCHEMA).GetTypeInfo());
				*(Core::Schema**)Args.GetAddressOfReturnLocation() = New;
			}
			Core::Schema* SchemaGetIndex(Core::Schema* Base, const std::string_view& Name)
			{
				Core::Schema* Result = Base->Fetch(Name);
				if (Result != nullptr)
					return Result;

				return Base->Set(Name, Core::Var::Undefined());
			}
			Core::Schema* SchemaGetIndexOffset(Core::Schema* Base, size_t Offset)
			{
				return Base->Get(Offset);
			}
			Core::Schema* SchemaSet(Core::Schema* Base, const std::string_view& Name, Core::Schema* Value)
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
			Core::Schema* SchemaFirst(Core::Schema* Base)
			{
				auto& Childs = Base->GetChilds();
				return Childs.empty() ? nullptr : Childs.front();
			}
			Core::Schema* SchemaLast(Core::Schema* Base)
			{
				auto& Childs = Base->GetChilds();
				return Childs.empty() ? nullptr : Childs.back();
			}
			Core::Variant SchemaFirstVar(Core::Schema* Base)
			{
				auto& Childs = Base->GetChilds();
				return Childs.empty() ? Core::Var::Undefined() : Childs.front()->Value;
			}
			Core::Variant SchemaLastVar(Core::Schema* Base)
			{
				auto& Childs = Base->GetChilds();
				return Childs.empty() ? Core::Var::Undefined() : Childs.back()->Value;
			}
			Array* SchemaGetCollection(Core::Schema* Base, const std::string_view& Name, bool Deep)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context)
					return nullptr;

				VirtualMachine* VM = Context->GetVM();
				if (!VM)
					return nullptr;

				Core::Vector<Core::Schema*> Nodes = Base->FetchCollection(Name, Deep);
				for (auto& Node : Nodes)
					Node->AddRef();

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_SCHEMA "@>@");
				return Array::Compose(Type.GetTypeInfo(), Nodes);
			}
			Array* SchemaGetChilds(Core::Schema* Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context)
					return nullptr;

				VirtualMachine* VM = Context->GetVM();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_SCHEMA "@>@");
				return Array::Compose(Type.GetTypeInfo(), Base->GetChilds());
			}
			Array* SchemaGetAttributes(Core::Schema* Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context)
					return nullptr;

				VirtualMachine* VM = Context->GetVM();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_SCHEMA "@>@");
				return Array::Compose(Type.GetTypeInfo(), Base->GetAttributes());
			}
			Dictionary* SchemaGetNames(Core::Schema* Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				Core::UnorderedMap<Core::String, size_t> Mapping = Base->GetNames();
				Dictionary* Map = Dictionary::Create(VM);

				for (auto& Item : Mapping)
				{
					int64_t Index = (int64_t)Item.second;
					Map->Set(Item.first, &Index, (int)TypeId::INT64);
				}

				return Map;
			}
			size_t SchemaGetSize(Core::Schema* Base)
			{
				return Base->Size();
			}
			Core::String SchemaToJSON(Core::Schema* Base)
			{
				Core::String Stream;
				Core::Schema::ConvertToJSON(Base, [&Stream](Core::VarForm, const std::string_view& Buffer) { Stream.append(Buffer); });
				return Stream;
			}
			Core::String SchemaToXML(Core::Schema* Base)
			{
				Core::String Stream;
				Core::Schema::ConvertToXML(Base, [&Stream](Core::VarForm, const std::string_view& Buffer) { Stream.append(Buffer); });
				return Stream;
			}
			Core::String SchemaToString(Core::Schema* Base)
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
					case Core::VarType::Binary:
						return Base->Value.GetBlob();
					case Core::VarType::Integer:
						return Core::ToString(Base->Value.GetInteger());
					case Core::VarType::Number:
						return Core::ToString(Base->Value.GetNumber());
					case Core::VarType::Decimal:
						return Base->Value.GetDecimal().ToString();
					case Core::VarType::Boolean:
						return Base->Value.GetBoolean() ? "1" : "0";
				}

				return "";
			}
			Core::String SchemaToBinary(Core::Schema* Base)
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
			Core::Decimal SchemaToDecimal(Core::Schema* Base)
			{
				return Base->Value.GetDecimal();
			}
			bool SchemaToBoolean(Core::Schema* Base)
			{
				return Base->Value.GetBoolean();
			}
			Core::Schema* SchemaFromJSON(const std::string_view& Value)
			{
				return ExpectsWrapper::Unwrap(Core::Schema::ConvertFromJSON(Value), (Core::Schema*)nullptr);
			}
			Core::Schema* SchemaFromXML(const std::string_view& Value)
			{
				return ExpectsWrapper::Unwrap(Core::Schema::ConvertFromXML(Value), (Core::Schema*)nullptr);
			}
			Core::Schema* SchemaImport(const std::string_view& Value)
			{
				auto Data = Core::OS::File::ReadAsString(Value);
				if (!Data)
					return ExpectsWrapper::UnwrapVoid(std::move(Data)) ? nullptr : nullptr;

				auto Output = Core::Schema::FromJSON(*Data);
				if (Output)
					return *Output;

				Output = Core::Schema::FromXML(Value);
				if (Output)
					return *Output;

				return ExpectsWrapper::Unwrap(Core::Schema::FromJSONB(Core::Vector<char>(Value.begin(), Value.end())), (Core::Schema*)nullptr);
			}
			Core::Schema* SchemaCopyAssign1(Core::Schema* Base, const Core::Variant& Other)
			{
				Base->Value = Other;
				return Base;
			}
			Core::Schema* SchemaCopyAssign2(Core::Schema* Base, Core::Schema* Other)
			{
				Base->Clear();
				if (!Other)
					return Base;

				auto* Copy = Other->Copy();
				Base->Join(Copy, true);
				Copy->Release();
				return Base;
			}
			bool SchemaEquals(Core::Schema* Base, Core::Schema* Other)
			{
				if (Other != nullptr)
					return Base->Value == Other->Value;

				Core::VarType Type = Base->Value.GetType();
				return (Type == Core::VarType::Null || Type == Core::VarType::Undefined);
			}
#ifdef VI_BINDINGS
			Core::ExpectsSystem<void> FromSocketPoll(Network::SocketPoll Poll)
			{
				switch (Poll)
				{
					case Network::SocketPoll::Reset:
						return Core::ExpectsSystem<void>(Core::SystemException("connection reset"));
					case Network::SocketPoll::Timeout:
						return Core::ExpectsSystem<void>(Core::SystemException("connection timed out"));
					case Network::SocketPoll::Cancel:
						return Core::ExpectsSystem<void>(Core::SystemException("connection aborted"));
					case Network::SocketPoll::Finish:
					case Network::SocketPoll::FinishSync:
					case Network::SocketPoll::Next:
					default:
						return Core::Expectation::Met;
				}
			}
			template <typename T>
			Core::String GetComponentName(T* Base)
			{
				return Base->GetName();
			}
			template <typename T>
			void PopulateComponent(RefClass& Class)
			{
				Class.SetMethodEx("string get_name() const", &GetComponentName<T>);
				Class.SetMethod("uint64 get_id() const", &T::GetId);
			}
			Core::VariantArgs ToVariantKeys(Core::Schema* Args)
			{
				Core::VariantArgs Keys;
				if (!Args || Args->Empty())
					return Keys;

				Keys.reserve(Args->Size());
				for (auto* Item : Args->GetChilds())
					Keys[Item->Key] = Item->Value;

				return Keys;
			}

			Mutex::Mutex() noexcept
			{
			}
			bool Mutex::TryLock()
			{
				if (!Base.try_lock())
					return false;

				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr)
					Context->SetUserData((void*)(uintptr_t)((size_t)(uintptr_t)Context->GetUserData(MutexUD) + (size_t)1), MutexUD);

				return true;
			}
			void Mutex::Lock()
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context)
					return Base.lock();

				VirtualMachine* VM = Context->GetVM();
				Context->SetUserData((void*)(uintptr_t)((size_t)(uintptr_t)Context->GetUserData(MutexUD) + (size_t)1), MutexUD);
				if (!VM->HasDebugger())
					return Base.lock();

				while (!TryLock())
					VM->TriggerDebugger(Context, 50);
			}
			void Mutex::Unlock()
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (Context != nullptr)
				{
					size_t Size = (size_t)(uintptr_t)Context->GetUserData(MutexUD);
					if (!Size)
						Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_MUTEXNOTOWNED));
					else
						Context->SetUserData((void*)(uintptr_t)(Size - 1), MutexUD);
				}
				Base.unlock();
			}
			Mutex* Mutex::Factory()
			{
				Mutex* Result = new Mutex();
				if (!Result)
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));

				return Result;
			}
			bool Mutex::IsAnyLocked(ImmediateContext* Context)
			{
				VI_ASSERT(Context != nullptr, "context should be set");
				return (size_t)(uintptr_t)Context->GetUserData(MutexUD) != (size_t)0;
			}
			int Mutex::MutexUD = 538;

			Thread::Thread(VirtualMachine* Engine, asIScriptFunction* Callback) noexcept : VM(Engine), Loop(nullptr), Function(Callback, VM ? VM->RequestContext() : nullptr)
			{
				VI_ASSERT(VM != nullptr, "virtual matchine should be set");
				Engine->NotifyOfNewObject(this, Engine->GetTypeInfoByName(TYPENAME_THREAD));
			}
			Thread::~Thread() noexcept
			{
				ReleaseReferences(nullptr);
			}
			void Thread::ExecutionLoop()
			{
				Thread* Base = this;
				if (!Function.IsValid())
					return Release();

				FunctionDelegate Copy = Function;
				Function.Context->SetUserData(this, ThreadUD);

				Loop = new EventLoop();
				Loop->Listen(Function.Context);
				Loop->Enqueue(std::move(Copy), [Base](ImmediateContext* Context)
				{
					Context->SetArgObject(0, Base);
				}, nullptr);

				EventLoop::Set(Loop);
				while (Loop->Poll(Function.Context, 1000))
					Loop->Dequeue(VM);

				Core::UMutex<std::recursive_mutex> Unique(Mutex);
				Raise = Exception::Pointer(Function.Context);
				VM->ReturnContext(Function.Context);
				Function.Release();
				Core::Memory::Release(Loop);
				Release();
				VirtualMachine::CleanupThisThread();
			}
			bool Thread::Suspend()
			{
				if (!IsActive())
					return false;

				Core::UMutex<std::recursive_mutex> Unique(Mutex);
				return Function.Context->IsSuspended() || !!Function.Context->Suspend();
			}
			bool Thread::Resume()
			{
				Core::UMutex<std::recursive_mutex> Unique(Mutex);
				Loop->Enqueue(Function.Context);
				VI_DEBUG("[vm] resume thread at %s", Core::OS::Process::GetThreadId(Procedure.get_id()).c_str());
				return true;
			}
			void Thread::EnumReferences(asIScriptEngine* Engine)
			{
				for (int i = 0; i < 2; i++)
				{
					for (auto Any : Pipe[i].Queue)
						FunctionFactory::GCEnumCallback(Engine, Any);
				}
				FunctionFactory::GCEnumCallback(Engine, &Function);
			}
			void Thread::ReleaseReferences(asIScriptEngine*)
			{
				Join();
				for (int i = 0; i < 2; i++)
				{
					auto& Source = Pipe[i];
					Core::UMutex<std::mutex> Unique(Source.Mutex);
					for (auto Any : Source.Queue)
						Core::Memory::Release(Any);
					Source.Queue.clear();
				}

				Core::UMutex<std::recursive_mutex> Unique(Mutex);
				if (Function.Context != nullptr)
					VM->ReturnContext(Function.Context);
				Function.Release();
			}
			int Thread::Join(uint64_t Timeout)
			{
				if (std::this_thread::get_id() == Procedure.get_id())
					return -1;

				Core::UMutex<std::recursive_mutex> Unique(Mutex);
				if (!Procedure.joinable())
					return -1;

				VI_DEBUG("[vm] join thread %s", Core::OS::Process::GetThreadId(Procedure.get_id()).c_str());
				Unique.Negate();
				if (VM != nullptr && VM->HasDebugger())
				{
					while (Procedure.joinable() && IsActive())
						VM->TriggerDebugger(nullptr, 50);
				}
				Procedure.join();
				Unique.Negate();
				if (!Raise.Empty())
					Exception::Throw(Raise);
				return 1;
			}
			int Thread::Join()
			{
				if (std::this_thread::get_id() == Procedure.get_id())
					return -1;

				while (true)
				{
					int R = Join(1000);
					if (R == -1 || R == 1)
						return R;
				}

				return 0;
			}
			void Thread::Push(void* Ref, int TypeId)
			{
				Any* Next = new Any(Ref, TypeId, VirtualMachine::Get());
				if (!Next)
					return Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));

				auto& Source = Pipe[(GetThread() == this ? 1 : 0)];
				Core::UMutex<std::mutex> Unique(Source.Mutex);
				Source.Queue.push_back(Next);
				Source.CV.notify_one();
			}
			bool Thread::Pop(void* Ref, int TypeId)
			{
				bool Resolved = false;
				while (!Resolved)
					Resolved = Pop(Ref, TypeId, 1000);

				return true;
			}
			bool Thread::Pop(void* Ref, int TypeId, uint64_t Timeout)
			{
				auto& Source = Pipe[(GetThread() == this ? 0 : 1)];
				std::unique_lock<std::mutex> Guard(Source.Mutex);
				if (!Source.CV.wait_for(Guard, std::chrono::milliseconds(Timeout), [&]
				{
					return Source.Queue.size() != 0;
				}))
					return false;

				Any* Result = Source.Queue.front();
				if (!Result->Retrieve(Ref, TypeId))
					return false;

				Source.Queue.erase(Source.Queue.begin());
				Core::Memory::Release(Result);
				return true;
			}
			bool Thread::IsActive()
			{
				Core::UMutex<std::recursive_mutex> Unique(Mutex);
				return Loop != nullptr && Function.Context != nullptr && Function.Context->GetState() == Execution::Active;
			}
			bool Thread::Start()
			{
				Core::UMutex<std::recursive_mutex> Unique(Mutex);
				if (!Function.IsValid())
					return false;

				Join();
				AddRef();

				Procedure = std::thread(&Thread::ExecutionLoop, this);
				VI_DEBUG("[vm] spawn thread %s", Core::OS::Process::GetThreadId(Procedure.get_id()).c_str());
				return true;
			}
			Thread* Thread::Create(asIScriptFunction* Callback)
			{
				Thread* Result = new Thread(VirtualMachine::Get(), Callback);
				if (!Result)
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));
				return Result;
			}
			Thread* Thread::GetThread()
			{
				auto* Context = ImmediateContext::Get();
				if (!Context)
					return nullptr;

				return static_cast<Thread*>(Context->GetUserData(ThreadUD));
			}
			Core::String Thread::GetId() const
			{
				return Core::OS::Process::GetThreadId(Procedure.get_id());
			}
			void Thread::ThreadSleep(uint64_t Timeout)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(Timeout));
			}
			bool Thread::ThreadSuspend()
			{
				ImmediateContext* Context = ImmediateContext::Get();
				if (!Context)
					return false;

				if (Context->IsSuspended())
					return true;

				return !!Context->Suspend();
			}
			Core::String Thread::GetThreadId()
			{
				return Core::OS::Process::GetThreadId(std::this_thread::get_id());
			}
			int Thread::ThreadUD = 431;

			CharBuffer::CharBuffer() noexcept : CharBuffer(nullptr)
			{
			}
			CharBuffer::CharBuffer(size_t NewSize) noexcept : CharBuffer(nullptr)
			{
				if (NewSize > 0)
					Allocate(NewSize);
			}
			CharBuffer::CharBuffer(char* Pointer) noexcept : Buffer(Pointer), Length(0)
			{
			}
			CharBuffer::~CharBuffer() noexcept
			{
				Deallocate();
			}
			bool CharBuffer::Allocate(size_t NewSize)
			{
				Deallocate();
				if (!NewSize)
					return false;

				Buffer = Core::Memory::Allocate<char>(NewSize);
				if (!Buffer)
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));
					return false;
				}

				memset(Buffer, 0, NewSize);
				Length = NewSize;
				return true;
			}
			void CharBuffer::Deallocate()
			{
				if (Length > 0)
					Core::Memory::Deallocate(Buffer);

				Buffer = nullptr;
				Length = 0;
			}
			bool CharBuffer::SetInt8(size_t Offset, int8_t Value, size_t SetSize)
			{
				if (!Buffer || Offset + SetSize > Length)
					return false;

				memset(Buffer + Offset, (int32_t)Value, SetSize);
				return true;
			}
			bool CharBuffer::SetUint8(size_t Offset, uint8_t Value, size_t SetSize)
			{
				if (!Buffer || Offset + SetSize > Length)
					return false;

				memset(Buffer + Offset, (int32_t)Value, SetSize);
				return true;
			}
			bool CharBuffer::StoreBytes(size_t Offset, const std::string_view& Value)
			{
				return Store(Offset, Value.data(), sizeof(char) * Value.size());
			}
			bool CharBuffer::StoreInt8(size_t Offset, int8_t Value)
			{
				return Store(Offset, (const char*)&Value, sizeof(Value));
			}
			bool CharBuffer::StoreUint8(size_t Offset, uint8_t Value)
			{
				return Store(Offset, (const char*)&Value, sizeof(Value));
			}
			bool CharBuffer::StoreInt16(size_t Offset, int16_t Value)
			{
				return Store(Offset, (const char*)&Value, sizeof(Value));
			}
			bool CharBuffer::StoreUint16(size_t Offset, uint16_t Value)
			{
				return Store(Offset, (const char*)&Value, sizeof(Value));
			}
			bool CharBuffer::StoreInt32(size_t Offset, int32_t Value)
			{
				return Store(Offset, (const char*)&Value, sizeof(Value));
			}
			bool CharBuffer::StoreUint32(size_t Offset, uint32_t Value)
			{
				return Store(Offset, (const char*)&Value, sizeof(Value));
			}
			bool CharBuffer::StoreInt64(size_t Offset, int64_t Value)
			{
				return Store(Offset, (const char*)&Value, sizeof(Value));
			}
			bool CharBuffer::StoreUint64(size_t Offset, uint64_t Value)
			{
				return Store(Offset, (const char*)&Value, sizeof(Value));
			}
			bool CharBuffer::StoreFloat(size_t Offset, float Value)
			{
				return Store(Offset, (const char*)&Value, sizeof(Value));
			}
			bool CharBuffer::StoreDouble(size_t Offset, double Value)
			{
				return Store(Offset, (const char*)&Value, sizeof(Value));
			}
			bool CharBuffer::Interpret(size_t Offset, Core::String& Value, size_t MaxSize) const
			{
				size_t DataSize = 0;
				if (!Buffer || Offset + sizeof(&DataSize) > Length)
					return false;

				char* Data = Buffer + Offset;
				char* Next = Data;
				while (*(Next++) != '\0')
				{
					if (++DataSize > MaxSize)
						return false;
				}

				Value.assign(Data, DataSize);
				return true;
			}
			bool CharBuffer::LoadBytes(size_t Offset, Core::String& Value, size_t ValueSize) const
			{
				Value.resize(ValueSize);
				if (Load(Offset, (char*)Value.c_str(), sizeof(char) * ValueSize))
					return true;

				Value.clear();
				return false;
			}
			bool CharBuffer::LoadInt8(size_t Offset, int8_t& Value) const
			{
				return Load(Offset, (char*)&Value, sizeof(Value));
			}
			bool CharBuffer::LoadUint8(size_t Offset, uint8_t& Value) const
			{
				return Load(Offset, (char*)&Value, sizeof(Value));
			}
			bool CharBuffer::LoadInt16(size_t Offset, int16_t& Value) const
			{
				return Load(Offset, (char*)&Value, sizeof(Value));
			}
			bool CharBuffer::LoadUint16(size_t Offset, uint16_t& Value) const
			{
				return Load(Offset, (char*)&Value, sizeof(Value));
			}
			bool CharBuffer::LoadInt32(size_t Offset, int32_t& Value) const
			{
				return Load(Offset, (char*)&Value, sizeof(Value));
			}
			bool CharBuffer::LoadUint32(size_t Offset, uint32_t& Value) const
			{
				return Load(Offset, (char*)&Value, sizeof(Value));
			}
			bool CharBuffer::LoadInt64(size_t Offset, int64_t& Value) const
			{
				return Load(Offset, (char*)&Value, sizeof(Value));
			}
			bool CharBuffer::LoadUint64(size_t Offset, uint64_t& Value) const
			{
				return Load(Offset, (char*)&Value, sizeof(Value));
			}
			bool CharBuffer::LoadFloat(size_t Offset, float& Value) const
			{
				return Load(Offset, (char*)&Value, sizeof(Value));
			}
			bool CharBuffer::LoadDouble(size_t Offset, double& Value) const
			{
				return Load(Offset, (char*)&Value, sizeof(Value));
			}
			bool CharBuffer::Store(size_t Offset, const char* Data, size_t DataSize)
			{
				if (!Buffer || Offset + DataSize > Length)
					return false;

				memcpy(Buffer + Offset, Data, DataSize);
				return true;
			}
			bool CharBuffer::Load(size_t Offset, char* Data, size_t DataSize) const
			{
				if (!Buffer || Offset + DataSize > Length)
					return false;

				memcpy(Data, Buffer + Offset, DataSize);
				return true;
			}
			void* CharBuffer::GetPointer(size_t Offset) const
			{
				if (Length > 0 && Offset >= Length)
					return nullptr;

				return Buffer + Offset;
			}
			bool CharBuffer::Exists(size_t Offset) const
			{
				return !Length || Offset < Length;
			}
			bool CharBuffer::Empty() const
			{
				return !Buffer;
			}
			size_t CharBuffer::Size() const
			{
				return Length;
			}
			Core::String CharBuffer::ToString(size_t MaxSize) const
			{
				Core::String Data;
				if (!Buffer)
					return Data;

				if (Length > 0)
				{
					Data.assign(Buffer, Length > MaxSize ? MaxSize : Length);
					return Data;
				}

				size_t DataSize = 0;
				char* Next = Buffer;
				while (*(Next++) != '\0')
				{
					if (++DataSize > MaxSize)
					{
						Data.assign(Buffer, DataSize - 1);
						return Data;
					}
				}

				Data.assign(Buffer, DataSize);
				return Data;
			}
			CharBuffer* CharBuffer::Create()
			{
				CharBuffer* Result = new CharBuffer();
				if (!Result)
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));

				return Result;
			}
			CharBuffer* CharBuffer::Create(size_t Size)
			{
				CharBuffer* Result = new CharBuffer(Size);
				if (!Result)
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));

				return Result;
			}
			CharBuffer* CharBuffer::Create(char* Pointer)
			{
				CharBuffer* Result = new CharBuffer(Pointer);
				if (!Result)
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFMEMORY));

				return Result;
			}

			Complex::Complex() noexcept
			{
				R = 0;
				I = 0;
			}
			Complex::Complex(const Complex& Other) noexcept
			{
				R = Other.R;
				I = Other.I;
			}
			Complex::Complex(float _R, float _I) noexcept
			{
				R = _R;
				I = _I;
			}
			bool Complex::operator==(const Complex& Other) const
			{
				return (R == Other.R) && (I == Other.I);
			}
			bool Complex::operator!=(const Complex& Other) const
			{
				return !(*this == Other);
			}
			Complex& Complex::operator=(const Complex& Other) noexcept
			{
				R = Other.R;
				I = Other.I;
				return *this;
			}
			Complex& Complex::operator+=(const Complex& Other)
			{
				R += Other.R;
				I += Other.I;
				return *this;
			}
			Complex& Complex::operator-=(const Complex& Other)
			{
				R -= Other.R;
				I -= Other.I;
				return *this;
			}
			Complex& Complex::operator*=(const Complex& Other)
			{
				*this = *this * Other;
				return *this;
			}
			Complex& Complex::operator/=(const Complex& Other)
			{
				*this = *this / Other;
				return *this;
			}
			float Complex::SquaredLength() const
			{
				return R * R + I * I;
			}
			float Complex::Length() const
			{
				return sqrtf(SquaredLength());
			}
			Complex Complex::operator+(const Complex& Other) const
			{
				return Complex(R + Other.R, I + Other.I);
			}
			Complex Complex::operator-(const Complex& Other) const
			{
				return Complex(R - Other.R, I + Other.I);
			}
			Complex Complex::operator*(const Complex& Other) const
			{
				return Complex(R * Other.R - I * Other.I, R * Other.I + I * Other.R);
			}
			Complex Complex::operator/(const Complex& Other) const
			{
				float Sq = Other.SquaredLength();
				if (Sq == 0)
					return Complex(0, 0);

				return Complex((R * Other.R + I * Other.I) / Sq, (I * Other.R - R * Other.I) / Sq);
			}
			Complex Complex::GetRI() const
			{
				return *this;
			}
			Complex Complex::GetIR() const
			{
				return Complex(R, I);
			}
			void Complex::SetRI(const Complex& Other)
			{
				*this = Other;
			}
			void Complex::SetIR(const Complex& Other)
			{
				R = Other.I;
				I = Other.R;
			}
			void Complex::DefaultConstructor(Complex* Base)
			{
				new(Base) Complex();
			}
			void Complex::CopyConstructor(Complex* Base, const Complex& Other)
			{
				new(Base) Complex(Other);
			}
			void Complex::ConvConstructor(Complex* Base, float Result)
			{
				new(Base) Complex(Result);
			}
			void Complex::InitConstructor(Complex* Base, float Result, float _I)
			{
				new(Base) Complex(Result, _I);
			}
			void Complex::ListConstructor(Complex* Base, float* InitList)
			{
				new(Base) Complex(InitList[0], InitList[1]);
			}

			void ConsoleTrace(Core::Console* Base, uint32_t Frames)
			{
				Base->WriteLine(Core::ErrorHandling::GetStackTrace(1, (size_t)Frames));
			}

			Application::Application(Desc& I, void* NewObject, int NewTypeId) noexcept : Layer::Application(&I), ProcessedEvents(0), InitiatorType(nullptr), InitiatorObject(NewObject)
			{
				VirtualMachine* CurrentVM = VirtualMachine::Get();
				if (CurrentVM != nullptr && InitiatorObject != nullptr && ((NewTypeId & (int)TypeId::OBJHANDLE) || (NewTypeId & (int)TypeId::MASK_OBJECT)))
				{
					InitiatorType = CurrentVM->GetTypeInfoById(NewTypeId).GetTypeInfo();
					if (NewTypeId & (int)TypeId::OBJHANDLE)
						InitiatorObject = *(void**)InitiatorObject;
					CurrentVM->AddRefObject(InitiatorObject, InitiatorType);
				}
				else if (CurrentVM != nullptr && InitiatorObject != nullptr)
				{
					if (NewTypeId != (int)TypeId::VOIDF)
						Exception::Throw(Exception::Pointer(EXCEPTION_INVALIDINITIATOR));
					InitiatorObject = nullptr;
				}

				if (I.Usage & (size_t)Layer::USE_SCRIPTING)
					VM = CurrentVM;

				if (I.Usage & (size_t)Layer::USE_PROCESSING)
				{
					Content = new Layer::ContentManager();
					Content->AddProcessor(new Layer::Processors::AssetProcessor(Content), VI_HASH(TYPENAME_ASSETFILE));
					Content->AddProcessor(new Layer::Processors::SchemaProcessor(Content), VI_HASH(TYPENAME_SCHEMA));
					Content->AddProcessor(new Layer::Processors::ServerProcessor(Content), VI_HASH(TYPENAME_HTTPSERVER));
				}
			}
			Application::~Application() noexcept
			{
				VirtualMachine* CurrentVM = VM ? VM : VirtualMachine::Get();
				if (CurrentVM != nullptr && InitiatorObject != nullptr && InitiatorType != nullptr)
					CurrentVM->ReleaseObject(InitiatorObject, InitiatorType);

				OnDispatch.Release();
				OnPublish.Release();
				OnComposition.Release();
				OnScriptHook.Release();
				OnInitialize.Release();
				OnStartup.Release();
				OnShutdown.Release();
				InitiatorObject = nullptr;
				InitiatorType = nullptr;
				VM = nullptr;
			}
			void Application::SetOnDispatch(asIScriptFunction* Callback)
			{
				OnDispatch = FunctionDelegate(Callback);
			}
			void Application::SetOnPublish(asIScriptFunction* Callback)
			{
				OnPublish = FunctionDelegate(Callback);
			}
			void Application::SetOnComposition(asIScriptFunction* Callback)
			{
				OnComposition = FunctionDelegate(Callback);
			}
			void Application::SetOnScriptHook(asIScriptFunction* Callback)
			{
				OnScriptHook = FunctionDelegate(Callback);
			}
			void Application::SetOnInitialize(asIScriptFunction* Callback)
			{
				OnInitialize = FunctionDelegate(Callback);
			}
			void Application::SetOnStartup(asIScriptFunction* Callback)
			{
				OnStartup = FunctionDelegate(Callback);
			}
			void Application::SetOnShutdown(asIScriptFunction* Callback)
			{
				OnShutdown = FunctionDelegate(Callback);
			}
			void Application::ScriptHook()
			{
				if (!OnScriptHook.IsValid())
					return;

				auto* Context = ImmediateContext::Get();
				VI_ASSERT(Context != nullptr, "application method cannot be called outside of script context");
				Context->ExecuteSubcall(OnScriptHook.Callable(), nullptr);
			}
			void Application::Composition()
			{
				if (!OnComposition.IsValid())
					return;

				auto* Context = ImmediateContext::Get();
				VI_ASSERT(Context != nullptr, "application method cannot be called outside of script context");
				Context->ExecuteSubcall(OnComposition.Callable(), nullptr);
			}
			void Application::Dispatch(Core::Timer* Time)
			{
				auto* Loop = EventLoop::Get();
				if (Loop != nullptr)
					ProcessedEvents = Loop->Dequeue(VM);
				else
					ProcessedEvents = 0;

				if (OnDispatch.IsValid())
				{
					auto* Context = ImmediateContext::Get();
					VI_ASSERT(Context != nullptr, "application method cannot be called outside of script context");
					Context->ExecuteSubcall(OnDispatch.Callable(), [Time](ImmediateContext* Context)
					{
						Context->SetArgObject(0, (void*)Time);
					});
				}
			}
			void Application::Publish(Core::Timer* Time)
			{
				if (!OnPublish.IsValid())
					return;

				auto* Context = ImmediateContext::Get();
				VI_ASSERT(Context != nullptr, "application method cannot be called outside of script context");
				Context->ExecuteSubcall(OnPublish.Callable(), [Time](ImmediateContext* Context)
				{
					Context->SetArgObject(0, (void*)Time);
				});
			}
			void Application::Initialize()
			{
				if (!OnInitialize.IsValid())
					return;

				auto* Context = ImmediateContext::Get();
				VI_ASSERT(Context != nullptr, "application method cannot be called outside of script context");
				Context->ExecuteSubcall(OnInitialize.Callable(), nullptr);
			}
			Core::Promise<void> Application::Startup()
			{
				if (!OnStartup.IsValid())
					return Core::Promise<void>::Null();

				VI_ASSERT(ImmediateContext::Get() != nullptr, "application method cannot be called outside of script context");
				auto Future = OnStartup(nullptr);
				if (!Future.IsPending())
					return Core::Promise<void>::Null();

				return Future.Then(std::function<void(ExpectsVM<Execution>&&)>(nullptr));
			}
			Core::Promise<void> Application::Shutdown()
			{
				if (!OnShutdown.IsValid())
					return Core::Promise<void>::Null();

				VI_ASSERT(ImmediateContext::Get() != nullptr, "application method cannot be called outside of script context");
				auto Future = OnShutdown(nullptr);
				if (!Future.IsPending())
					return Core::Promise<void>::Null();

				return Future.Then(std::function<void(ExpectsVM<Execution>&&)>(nullptr));
			}
			size_t Application::GetProcessedEvents() const
			{
				return ProcessedEvents;
			}
			bool Application::HasProcessedEvents() const
			{
				return ProcessedEvents > 0;
			}
			bool Application::RetrieveInitiatorObject(void* RefPointer, int RefTypeId) const
			{
				VirtualMachine* CurrentVM = VM ? VM : VirtualMachine::Get();
				if (!InitiatorObject || !InitiatorType || !RefPointer || !CurrentVM)
					return false;

				if (RefTypeId & (size_t)TypeId::OBJHANDLE)
				{
					CurrentVM->RefCastObject(InitiatorObject, InitiatorType, CurrentVM->GetTypeInfoById(RefTypeId), reinterpret_cast<void**>(RefPointer));
#ifdef VI_ANGELSCRIPT
					if (*(asPWORD*)RefPointer == 0)
						return false;
#endif
					return true;
				}
				else if (RefTypeId & (size_t)TypeId::MASK_OBJECT)
				{
					auto RefTypeInfo = CurrentVM->GetTypeInfoById(RefTypeId);
					if (InitiatorType == RefTypeInfo.GetTypeInfo())
					{
						CurrentVM->AssignObject(RefPointer, InitiatorObject, InitiatorType);
						return true;
					}
				}

				return false;
			}
			void* Application::GetInitiatorObject() const
			{
				return InitiatorObject;
			}
			bool Application::WantsRestart(int ExitCode)
			{
				return ExitCode == Layer::EXIT_RESTART;
			}

			bool StreamOpen(Core::Stream* Base, const std::string_view& Path, Core::FileMode Mode)
			{
				return ExpectsWrapper::UnwrapVoid(Base->Open(Path, Mode));
			}
			Core::String StreamRead(Core::Stream* Base, size_t Size)
			{
				Core::String Result;
				Result.resize(Size);

				size_t Count = ExpectsWrapper::Unwrap(Base->Read((uint8_t*)Result.data(), Size), (size_t)0);
				if (Count < Size)
					Result.resize(Count);

				return Result;
			}
			Core::String StreamReadLine(Core::Stream* Base, size_t Size)
			{
				Core::String Result;
				Result.resize(Size);

				size_t Count = ExpectsWrapper::Unwrap(Base->ReadLine((char*)Result.data(), Size), (size_t)0);
				if (Count < Size)
					Result.resize(Count);

				return Result;
			}
			size_t StreamWrite(Core::Stream* Base, const std::string_view& Data)
			{
				return ExpectsWrapper::Unwrap(Base->Write((uint8_t*)Data.data(), Data.size()), (size_t)0);
			}

			Core::TaskId ScheduleSetInterval(Core::Schedule* Base, uint64_t Mills, asIScriptFunction* Callback)
			{
				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
					return Core::INVALID_TASK_ID;

				return Base->SetInterval(Mills, [Delegate]() mutable { Delegate(nullptr); });
			}
			Core::TaskId ScheduleSetTimeout(Core::Schedule* Base, uint64_t Mills, asIScriptFunction* Callback)
			{
				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
					return Core::INVALID_TASK_ID;

				return Base->SetTimeout(Mills, [Delegate]() mutable { Delegate(nullptr); });
			}
			bool ScheduleSetImmediate(Core::Schedule* Base, asIScriptFunction* Callback)
			{
				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
					return false;

				return Base->SetTask([Delegate]() mutable { Delegate(nullptr); });
			}
			bool ScheduleSpawn(Core::Schedule* Base, asIScriptFunction* Callback)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return false;

				ImmediateContext* Context = VM->RequestContext();
				if (!Context)
					return false;

				FunctionDelegate Delegate(Callback, Context);
				if (!Delegate.IsValid())
					return false;

				return Base->SetTask([Delegate, Context]() mutable
				{
					Delegate(nullptr, [Context](ImmediateContext*)
					{
						Context->GetVM()->ReturnContext(Context);
					});
				});
			}

			Dictionary* InlineArgsGetArgs(Core::InlineArgs& Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_DICTIONARY "@");
				return Dictionary::Compose<Core::String>(Type.GetTypeId(), Base.Args);
			}
			Array* InlineArgsGetParams(Core::InlineArgs& Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose<Core::String>(Type.GetTypeInfo(), Base.Params);
			}

			Array* OSDirectoryScan(const std::string_view& Path)
			{
				Core::Vector<std::pair<Core::String, Core::FileEntry>> Entries;
				auto Status = Core::OS::Directory::Scan(Path, Entries);
				if (!Status)
					return ExpectsWrapper::UnwrapVoid(std::move(Status)) ? nullptr : nullptr;

				Core::Vector<FileLink> Results;
				Results.reserve(Entries.size());
				for (auto& Item : Entries)
				{
					FileLink Next;
					(*(Core::FileEntry*)&Next) = Item.second;
					Next.Path = std::move(Item.first);
					Results.emplace_back(std::move(Next));
				}

				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_FILELINK ">@");
				return Array::Compose<FileLink>(Type.GetTypeInfo(), Results);
			}
			Array* OSDirectoryGetMounts()
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose<Core::String>(Type.GetTypeInfo(), Core::OS::Directory::GetMounts());
			}
			bool OSDirectoryCreate(const std::string_view& Path)
			{
				return ExpectsWrapper::UnwrapVoid(Core::OS::Directory::Create(Path));
			}
			bool OSDirectoryRemove(const std::string_view& Path)
			{
				return ExpectsWrapper::UnwrapVoid(Core::OS::Directory::Remove(Path));
			}
			bool OSDirectoryIsExists(const std::string_view& Path)
			{
				return Core::OS::Directory::IsExists(Path);
			}
			bool OSDirectoryIsEmpty(const std::string_view& Path)
			{
				return Core::OS::Directory::IsEmpty(Path);
			}
			bool OSDirectorySetWorking(const std::string_view& Path)
			{
				return ExpectsWrapper::UnwrapVoid(Core::OS::Directory::SetWorking(Path));
			}
			bool OSDirectoryPatch(const std::string_view& Path)
			{
				return ExpectsWrapper::UnwrapVoid(Core::OS::Directory::Patch(Path));
			}

			bool OSFileState(const std::string_view& Path, Core::FileEntry& Data)
			{
				return ExpectsWrapper::UnwrapVoid(Core::OS::File::GetState(Path, &Data));
			}
			bool OSFileMove(const std::string_view& From, const std::string_view& To)
			{
				return ExpectsWrapper::UnwrapVoid(Core::OS::File::Move(From, To));
			}
			bool OSFileCopy(const std::string_view& From, const std::string_view& To)
			{
				return ExpectsWrapper::UnwrapVoid(Core::OS::File::Copy(From, To));
			}
			bool OSFileRemove(const std::string_view& Path)
			{
				return ExpectsWrapper::UnwrapVoid(Core::OS::File::Remove(Path));
			}
			bool OSFileIsExists(const std::string_view& Path)
			{
				return Core::OS::File::IsExists(Path);
			}
			bool OSFileWrite(const std::string_view& Path, const std::string_view& Data)
			{
				return ExpectsWrapper::UnwrapVoid(Core::OS::File::Write(Path, (uint8_t*)Data.data(), Data.size()));
			}
			size_t OSFileJoin(const std::string_view& From, Array* Paths)
			{
				return ExpectsWrapper::Unwrap(Core::OS::File::Join(From, Array::Decompose<Core::String>(Paths)), (size_t)0);
			}
			Core::FileState OSFileGetProperties(const std::string_view& Path)
			{
				return ExpectsWrapper::Unwrap(Core::OS::File::GetProperties(Path), Core::FileState());
			}
			Core::Stream* OSFileOpenJoin(const std::string_view& From, Array* Paths)
			{
				return ExpectsWrapper::Unwrap(Core::OS::File::OpenJoin(From, Array::Decompose<Core::String>(Paths)), (Core::Stream*)nullptr);
			}
			Core::Stream* OSFileOpenArchive(const std::string_view& Path, size_t UnarchivedMaxSize)
			{
				return ExpectsWrapper::Unwrap(Core::OS::File::OpenArchive(Path, UnarchivedMaxSize), (Core::Stream*)nullptr);
			}
			Core::Stream* OSFileOpen(const std::string_view& Path, Core::FileMode Mode, bool Async)
			{
				return ExpectsWrapper::Unwrap(Core::OS::File::Open(Path, Mode, Async), (Core::Stream*)nullptr);
			}
			Array* OSFileReadAsArray(const std::string_view& Path)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				auto Data = Core::OS::File::ReadAsArray(Path);
				if (!Data)
					return ExpectsWrapper::UnwrapVoid(std::move(Data)) ? nullptr : nullptr;

				return Array::Compose<Core::String>(Type.GetTypeInfo(), *Data);
			}

			bool OSPathIsRemote(const std::string_view& Path)
			{
				return Core::OS::Path::IsRemote(Path);
			}
			bool OSPathIsRelative(const std::string_view& Path)
			{
				return Core::OS::Path::IsRelative(Path);
			}
			bool OSPathIsAbsolute(const std::string_view& Path)
			{
				return Core::OS::Path::IsAbsolute(Path);
			}
			Core::String OSPathResolve1(const std::string_view& Path)
			{
				return ExpectsWrapper::Unwrap(Core::OS::Path::Resolve(Path), Core::String());
			}
			Core::String OSPathResolveDirectory1(const std::string_view& Path)
			{
				return ExpectsWrapper::Unwrap(Core::OS::Path::ResolveDirectory(Path), Core::String());
			}
			Core::String OSPathResolve2(const std::string_view& Path, const std::string_view& Directory, bool EvenIfExists)
			{
				return ExpectsWrapper::Unwrap(Core::OS::Path::Resolve(Path, Directory, EvenIfExists), Core::String());
			}
			Core::String OSPathResolveDirectory2(const std::string_view& Path, const std::string_view& Directory, bool EvenIfExists)
			{
				return ExpectsWrapper::Unwrap(Core::OS::Path::ResolveDirectory(Path, Directory, EvenIfExists), Core::String());
			}
			Core::String OSPathGetDirectory(const std::string_view& Path, size_t Level)
			{
				return Core::OS::Path::GetDirectory(Path, Level);
			}
			Core::String OSPathGetFilename(const std::string_view& Path)
			{
				return Core::String(Core::OS::Path::GetFilename(Path));
			}
			Core::String OSPathGetExtension(const std::string_view& Path)
			{
				return Core::String(Core::OS::Path::GetExtension(Path));
			}

			int OSProcessExecute(const std::string_view& Command, Core::FileMode Mode, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				FunctionDelegate Delegate(Callback);
				auto ExitCode = Core::OS::Process::Execute(Command, Mode, [Context, &Delegate](const std::string_view& Value) -> bool
				{
					if (!Context || !Delegate.IsValid())
						return true;

					Core::String Copy = Core::String(Value); bool Continue = true;
					Context->ExecuteSubcall(Delegate.Callable(), [&Copy](ImmediateContext* Context)
					{
						Context->SetArgObject(0, (void*)&Copy);
					}, [&Continue](ImmediateContext* Context)
					{
						Continue = (bool)Context->GetReturnByte();
					});
					return Continue;
				});
				return ExpectsWrapper::Unwrap(std::move(ExitCode), -1, Context);
			}
			Core::InlineArgs OSProcessParseArgs(Array* ArgsArray, size_t Opts, Array* FlagsArray)
			{
				Core::UnorderedSet<Core::String> Flags;
				Core::Vector<Core::String> InlineFlags = Array::Decompose<Core::String>(ArgsArray);
				Flags.reserve(InlineFlags.size());

				Core::Vector<Core::String> InlineArgs = Array::Decompose<Core::String>(ArgsArray);
				Core::Vector<char*> Args;
				Args.reserve(InlineArgs.size());

				for (auto& Item : InlineFlags)
					Flags.insert(Item);

				for (auto& Item : InlineArgs)
					Args.push_back((char*)Item.c_str());

				return Core::OS::Process::ParseArgs((int)Args.size(), Args.data(), Opts, Flags);
			}

			void* OSSymbolLoad(const std::string_view& Path)
			{
				return ExpectsWrapper::Unwrap(Core::OS::Symbol::Load(Path), (void*)nullptr);
			}
			void* OSSymbolLoadFunction(void* LibHandle, const std::string_view& Path)
			{
				return ExpectsWrapper::Unwrap(Core::OS::Symbol::LoadFunction(LibHandle, Path), (void*)nullptr);
			}
			bool OSSymbolUnload(void* Handle)
			{
				return ExpectsWrapper::UnwrapVoid(Core::OS::Symbol::Unload(Handle));
			}

			Core::String RegexMatchTarget(Compute::RegexMatch& Base)
			{
				return Base.Pointer ? Base.Pointer : Core::String();
			}

			Array* RegexResultGet(Compute::RegexResult& Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_REGEXMATCH ">@");
				return Array::Compose<Compute::RegexMatch>(Type.GetTypeInfo(), Base.Get());
			}
			Array* RegexResultToArray(Compute::RegexResult& Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose<Core::String>(Type.GetTypeInfo(), Base.ToArray());
			}

			Compute::RegexResult RegexSourceMatch(Compute::RegexSource& Base, const std::string_view& Text)
			{
				Compute::RegexResult Result;
				Compute::Regex::Match(&Base, Result, Text);
				return Result;
			}
			Core::String RegexSourceReplace(Compute::RegexSource& Base, const std::string_view& Text, const std::string_view& To)
			{
				Core::String Copy = Core::String(Text);
				Compute::RegexResult Result;
				Compute::Regex::Replace(&Base, To, Copy);
				return Copy;
			}

			void WebTokenSetAudience(Compute::WebToken* Base, Array* Data)
			{
				Base->SetAudience(Array::Decompose<Core::String>(Data));
			}

			Core::String CryptoSign(Compute::Digest Type, Compute::SignAlg KeyType, const std::string_view& Data, const Compute::PrivateKey& Key)
			{
				return ExpectsWrapper::Unwrap(Compute::Crypto::Sign(Type, KeyType, Data, Key), Core::String());
			}
			bool CryptoVerify(Compute::Digest Type, Compute::SignAlg KeyType, const std::string_view& Data, const std::string_view& Signature, const Compute::PrivateKey& Key)
			{
				return ExpectsWrapper::UnwrapVoid(Compute::Crypto::Verify(Type, KeyType, Data, Signature, Key));
			}
			Core::String CryptoHMAC(Compute::Digest Type, const std::string_view& Data, const Compute::PrivateKey& Key)
			{
				return ExpectsWrapper::Unwrap(Compute::Crypto::HMAC(Type, Data, Key), Core::String());
			}
			Core::String CryptoEncrypt(Compute::Cipher Type, const std::string_view& Data, const Compute::PrivateKey& Key, const Compute::PrivateKey& Salt, int Complexity)
			{
				return ExpectsWrapper::Unwrap(Compute::Crypto::Encrypt(Type, Data, Key, Salt, Complexity), Core::String());
			}
			Core::String CryptoDecrypt(Compute::Cipher Type, const std::string_view& Data, const Compute::PrivateKey& Key, const Compute::PrivateKey& Salt, int Complexity)
			{
				return ExpectsWrapper::Unwrap(Compute::Crypto::Decrypt(Type, Data, Key, Salt, Complexity), Core::String());
			}
			size_t CryptoEncryptStream(Compute::Cipher Type, Core::Stream* From, Core::Stream* To, const Compute::PrivateKey& Key, const Compute::PrivateKey& Salt, asIScriptFunction* Transform, size_t ReadInterval, int Complexity)
			{
				Core::String Intermediate;
				ImmediateContext* Context = ImmediateContext::Get();
				FunctionDelegate Delegate(Transform);
				auto Callback = [Context, &Delegate, &Intermediate](uint8_t** Buffer, size_t* Size) -> void
				{
					if (!Context || !Delegate.IsValid())
						return;

					std::string_view Input((char*)*Buffer, *Size);
					Context->ExecuteSubcall(Delegate.Callable(), [&Input](ImmediateContext* Context)
					{
						Context->SetArgObject(0, (void*)&Input);
					}, [&Intermediate](ImmediateContext* Context)
					{
						auto* Result = Context->GetReturnObject<Core::String>();
						if (Result != nullptr)
							Intermediate = *Result;
					});
					*Buffer = (uint8_t*)Intermediate.data();
					*Size = Intermediate.size();
				};
				return ExpectsWrapper::Unwrap(Compute::Crypto::Encrypt(Type, From, To, Key, Salt, std::move(Callback), ReadInterval, Complexity), (size_t)0);
			}
			size_t CryptoDecryptStream(Compute::Cipher Type, Core::Stream* From, Core::Stream* To, const Compute::PrivateKey& Key, const Compute::PrivateKey& Salt, asIScriptFunction* Transform, size_t ReadInterval, int Complexity)
			{
				Core::String Intermediate;
				ImmediateContext* Context = ImmediateContext::Get();
				FunctionDelegate Delegate(Transform);
				auto Callback = [Context, &Delegate, &Intermediate](uint8_t** Buffer, size_t* Size) -> void
				{
					if (!Context || !Delegate.IsValid())
						return;

					std::string_view Input((char*)*Buffer, *Size);
					Context->ExecuteSubcall(Delegate.Callable(), [&Input](ImmediateContext* Context)
					{
						Context->SetArgObject(0, (void*)&Input);
					}, [&Intermediate](ImmediateContext* Context)
					{
						auto* Result = Context->GetReturnObject<Core::String>();
						if (Result != nullptr)
							Intermediate = *Result;
					});
					*Buffer = (uint8_t*)Intermediate.data();
					*Size = Intermediate.size();
				};
				return ExpectsWrapper::Unwrap(Compute::Crypto::Decrypt(Type, From, To, Key, Salt, std::move(Callback), ReadInterval, Complexity), (size_t)0);
			}

			void IncludeDescAddExt(Compute::IncludeDesc& Base, const std::string_view& Value)
			{
				Base.Exts.push_back(Core::String(Value));
			}
			void IncludeDescRemoveExt(Compute::IncludeDesc& Base, const std::string_view& Value)
			{
				auto It = std::find(Base.Exts.begin(), Base.Exts.end(), Value);
				if (It != Base.Exts.end())
					Base.Exts.erase(It);
			}

			void PreprocessorSetIncludeCallback(Compute::Preprocessor* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				FunctionDelegate Delegate(Callback);
				if (!Context || !Delegate.IsValid())
					return Base->SetIncludeCallback(nullptr);

				Base->SetIncludeCallback([Context, Delegate](Compute::Preprocessor* Base, const Compute::IncludeResult& File, Core::String& Output) -> Compute::ExpectsPreprocessor<Compute::IncludeType>
				{
					Compute::IncludeType Result;
					Context->ExecuteSubcall(Delegate.Callable(), [Base, &File, &Output](ImmediateContext* Context)
					{
						Context->SetArgObject(0, Base);
						Context->SetArgObject(1, (void*)&File);
						Context->SetArgObject(2, &Output);
					}, [&Result](ImmediateContext* Context)
					{
						Result = (Compute::IncludeType)Context->GetReturnDWord();
					});
					return Result;
				});
			}
			void PreprocessorSetPragmaCallback(Compute::Preprocessor* Base, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				FunctionDelegate Delegate(Callback);
				if (!Context || !Delegate.IsValid())
					return Base->SetPragmaCallback(nullptr);

				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				Base->SetPragmaCallback([Type, Context, Delegate](Compute::Preprocessor* Base, const std::string_view& Name, const Core::Vector<Core::String>& Args) -> Compute::ExpectsPreprocessor<void>
				{
					bool Success = false;
					Core::UPtr<Array> Params = Array::Compose<Core::String>(Type.GetTypeInfo(), Args);
					Context->ExecuteSubcall(Delegate.Callable(), [Base, &Name, &Params](ImmediateContext* Context)
					{
						Context->SetArgObject(0, Base);
						Context->SetArgObject(1, (void*)&Name);
						Context->SetArgObject(2, *Params);
					}, [&Success](ImmediateContext* Context)
					{
						Success = (bool)Context->GetReturnByte();
					});
					if (!Success)
						return Compute::PreprocessorException(Compute::PreprocessorError::PragmaNotFound, 0, "pragma name: " + Core::String(Name));

					return Core::Expectation::Met;
				});
			}
			void PreprocessorSetDirectiveCallback(Compute::Preprocessor* Base, const std::string_view& Name, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				FunctionDelegate Delegate(Callback);
				if (!Context || !Delegate.IsValid())
					return Base->SetDirectiveCallback(Name, nullptr);

				Base->SetDirectiveCallback(Name, [Context, Delegate](Compute::Preprocessor* Base, const Compute::ProcDirective& Token, Core::String& Result) -> Compute::ExpectsPreprocessor<void>
				{
					bool Success = false;
					Context->ExecuteSubcall(Delegate.Callable(), [Base, &Token, &Result](ImmediateContext* Context)
					{
						Context->SetArgObject(0, Base);
						Context->SetArgObject(1, (void*)&Token);
						Context->SetArgObject(1, (void*)&Result);
					}, [&Success](ImmediateContext* Context)
					{
						Success = Context->GetReturnByte() > 0;
					});
					if (!Success)
						return Compute::PreprocessorException(Compute::PreprocessorError::DirectiveNotFound, 0, "directive name: " + Token.Name);

					return Core::Expectation::Met;
				});
			}
			bool PreprocessorIsDefined(Compute::Preprocessor* Base, const std::string_view& Name)
			{
				return Base->IsDefined(Name);
			}

			Core::String SocketAddressGetHostname(Network::SocketAddress& Base)
			{
				return ExpectsWrapper::Unwrap(Base.GetHostname(), Core::String());
			}
			Core::String SocketAddressGetIpAddress(Network::SocketAddress& Base)
			{
				return ExpectsWrapper::Unwrap(Base.GetIpAddress(), Core::String());
			}
			uint16_t SocketAddressGetIpPort(Network::SocketAddress& Base)
			{
				return ExpectsWrapper::Unwrap(Base.GetIpPort(), uint16_t());
			}

			Dictionary* LocationGetQuery(Network::Location& Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_DICTIONARY "@");
				return Dictionary::Compose<Core::String>(Type.GetTypeId(), Base.Query);
			}

			Dictionary* CertificateGetExtensions(Network::Certificate& Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_DICTIONARY "@");
				return Dictionary::Compose<Core::String>(Type.GetTypeId(), Base.Extensions);
			}

			Core::Promise<Network::SocketAccept> SocketAcceptDeferred(Network::Socket* Base)
			{
				ImmediateContext* Context = ImmediateContext::Get(); Base->AddRef();
				return Base->AcceptDeferred().Then<Network::SocketAccept>([Context, Base](Core::ExpectsIO<Network::SocketAccept>&& Incoming)
				{
					Base->Release();
					return ExpectsWrapper::Unwrap(std::move(Incoming), Network::SocketAccept(), Context);
				});
			}
			Core::Promise<bool> SocketConnectDeferred(Network::Socket* Base, const Network::SocketAddress& Address)
			{
				ImmediateContext* Context = ImmediateContext::Get(); Base->AddRef();
				return Base->ConnectDeferred(Address).Then<bool>([Context, Base](Core::ExpectsIO<void>&& Status)
				{
					Base->Release();
					return ExpectsWrapper::UnwrapVoid(std::move(Status), Context);
				});
			}
			Core::Promise<bool> SocketCloseDeferred(Network::Socket* Base)
			{
				ImmediateContext* Context = ImmediateContext::Get(); Base->AddRef();
				return Base->CloseDeferred().Then<bool>([Context, Base](Core::ExpectsIO<void>&& Status)
				{
					Base->Release();
					return ExpectsWrapper::UnwrapVoid(std::move(Status), Context);
				});
			}
			Core::Promise<bool> SocketWriteFileDeferred(Network::Socket* Base, Core::FileStream* Stream, size_t Offset, size_t Size)
			{
				ImmediateContext* Context = ImmediateContext::Get(); Base->AddRef();
				return Base->WriteFileDeferred(Stream ? (FILE*)Stream->GetReadable() : nullptr, Offset, Size).Then<bool>([Context, Base](Core::ExpectsIO<size_t>&& Status)
				{
					Base->Release();
					return ExpectsWrapper::UnwrapVoid(std::move(Status), Context);
				});
			}
			Core::Promise<bool> SocketWriteDeferred(Network::Socket* Base, const std::string_view& Data)
			{
				ImmediateContext* Context = ImmediateContext::Get(); Base->AddRef();
				return Base->WriteDeferred((uint8_t*)Data.data(), Data.size()).Then<bool>([Context, Base](Core::ExpectsIO<size_t>&& Status)
				{
					Base->Release();
					return ExpectsWrapper::UnwrapVoid(std::move(Status), Context);
				});
			}
			Core::Promise<Core::String> SocketReadDeferred(Network::Socket* Base, size_t Size)
			{
				ImmediateContext* Context = ImmediateContext::Get(); Base->AddRef();
				return Base->ReadDeferred(Size).Then<Core::String>([Context, Base](Core::ExpectsIO<Core::String>&& Data)
				{
					Base->Release();
					return ExpectsWrapper::Unwrap(std::move(Data), Core::String(), Context);
				});
			}
			Core::Promise<Core::String> SocketReadUntilDeferred(Network::Socket* Base, const std::string_view& Match, size_t MaxSize)
			{
				ImmediateContext* Context = ImmediateContext::Get(); Base->AddRef();
				return Base->ReadUntilDeferred(Core::String(Match), MaxSize).Then<Core::String>([Context, Base](Core::ExpectsIO<Core::String>&& Data)
				{
					Base->Release();
					return ExpectsWrapper::Unwrap(std::move(Data), Core::String(), Context);
				});
			}
			Core::Promise<Core::String> SocketReadUntilChunkedDeferred(Network::Socket* Base, const std::string_view& Match, size_t MaxSize)
			{
				ImmediateContext* Context = ImmediateContext::Get(); Base->AddRef();
				return Base->ReadUntilChunkedDeferred(Core::String(Match), MaxSize).Then<Core::String>([Context, Base](Core::ExpectsIO<Core::String>&& Data)
				{
					Base->Release();
					return ExpectsWrapper::Unwrap(std::move(Data), Core::String(), Context);
				});
			}
			size_t SocketWriteFile(Network::Socket* Base, Core::FileStream* Stream, size_t Offset, size_t Size)
			{
				return ExpectsWrapper::Unwrap(Base->WriteFile((FILE*)Stream->GetReadable(), Offset, Size), (size_t)0);
			}
			size_t SocketWrite(Network::Socket* Base, const std::string_view& Data)
			{
				return ExpectsWrapper::Unwrap(Base->Write((uint8_t*)Data.data(), (int)Data.size()), (size_t)0);
			}
			size_t SocketRead(Network::Socket* Base, Core::String& Data, size_t Size)
			{
				Data.resize(Size);
				return ExpectsWrapper::Unwrap(Base->Read((uint8_t*)Data.data(), Size), (size_t)0);
			}
			size_t SocketReadUntil(Network::Socket* Base, const std::string_view& Data, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				FunctionDelegate Delegate(Callback);
				if (!Context || !Delegate.IsValid())
					return ExpectsWrapper::UnwrapVoid<void, std::error_condition>(std::make_error_condition(std::errc::invalid_argument)) ? 0 : 0;

				Base->AddRef();
				return ExpectsWrapper::Unwrap(Base->ReadUntil(Data, [Base, Context, Delegate](Network::SocketPoll Poll, const uint8_t* Data, size_t Size)
				{
					bool Result = false;
					std::string_view Source = std::string_view((char*)Data, Size);
					Context->ExecuteSubcall(Delegate.Callable(), [Poll, Source](ImmediateContext* Context)
					{
						Context->SetArg32(0, (int)Poll);
						Context->SetArgObject(1, (void*)&Source);
					}, [&Result](ImmediateContext* Context)
					{
						Result = (bool)Context->GetReturnByte();
					});

					Base->Release();
					return Result;
				}), (size_t)0);
			}
			size_t SocketReadUntilChunked(Network::Socket* Base, const std::string_view& Data, asIScriptFunction* Callback)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				FunctionDelegate Delegate(Callback);
				if (!Context || !Delegate.IsValid())
					return ExpectsWrapper::UnwrapVoid<void, std::error_condition>(std::make_error_condition(std::errc::invalid_argument)) ? 0 : 0;

				Base->AddRef();
				return ExpectsWrapper::Unwrap(Base->ReadUntilChunked(Data, [Base, Context, Delegate](Network::SocketPoll Poll, const uint8_t* Data, size_t Size)
				{
					bool Result = false;
					std::string_view Source = std::string_view((char*)Data, Size);
					Context->ExecuteSubcall(Delegate.Callable(), [Poll, Source](ImmediateContext* Context)
					{
						Context->SetArg32(0, (int)Poll);
						Context->SetArgObject(1, (void*)&Source);
					}, [&Result](ImmediateContext* Context)
					{
						Result = (bool)Context->GetReturnByte();
					});

					Base->Release();
					return Result;
				}), (size_t)0);
			}
			
			Core::Promise<Core::String> DNSReverseLookupDeferred(Network::DNS* Base, const std::string_view& Hostname, const std::string_view& Service)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->ReverseLookupDeferred(Hostname, Service).Then<Core::String>([Context](Core::ExpectsSystem<Core::String>&& Address)
				{
					return ExpectsWrapper::Unwrap(std::move(Address), Core::String(), Context);
				});
			}
			Core::Promise<Core::String> DNSReverseAddressLookupDeferred(Network::DNS* Base, const Network::SocketAddress& Address)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->ReverseAddressLookupDeferred(Address).Then<Core::String>([Context](Core::ExpectsSystem<Core::String>&& Address)
				{
					return ExpectsWrapper::Unwrap(std::move(Address), Core::String(), Context);
				});
			}
			Core::Promise<Network::SocketAddress> DNSLookupDeferred(Network::DNS* Base, const std::string_view& Hostname, const std::string_view& Service, Network::DNSType Mode, Network::SocketProtocol Protocol, Network::SocketType Type)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->LookupDeferred(Hostname, Service, Mode, Protocol, Type).Then<Network::SocketAddress>([Context](Core::ExpectsSystem<Network::SocketAddress>&& Address)
				{
					return ExpectsWrapper::Unwrap(std::move(Address), Network::SocketAddress(), Context);
				});
			}

			Network::EpollFd& EpollFdCopy(Network::EpollFd& Base, Network::EpollFd& Other)
			{
				if (&Base == &Other)
					return Base;

				Core::Memory::Release(Base.Base);
				Base = Other;
				if (Base.Base != nullptr)
					Base.Base->AddRef();

				return Base;
			}
			void EpollFdDestructor(Network::EpollFd& Base)
			{
				Core::Memory::Release(Base.Base);
			}

			int EpollHandleWait(Network::EpollHandle& Base, Array* Data, uint64_t Timeout)
			{
				Core::Vector<Network::EpollFd> Fds = Array::Decompose<Network::EpollFd>(Data);
				return Base.Wait(Fds.data(), Fds.size(), Timeout);
			}

			bool MultiplexerWhenReadable(Network::Socket* Base, asIScriptFunction* Callback)
			{
				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
					return false;

				return Network::Multiplexer::Get()->WhenReadable(Base, [Base, Delegate](Network::SocketPoll Poll) mutable
				{
					Delegate([Base, Poll](ImmediateContext* Context)
					{
						Context->SetArgObject(0, Base);
						Context->SetArg32(1, (int)Poll);
					});
				});
			}
			bool MultiplexerWhenWriteable(Network::Socket* Base, asIScriptFunction* Callback)
			{
				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
					return false;

				return Network::Multiplexer::Get()->WhenWriteable(Base, [Base, Delegate](Network::SocketPoll Poll) mutable
				{
					Delegate([Base, Poll](ImmediateContext* Context)
					{
						Context->SetArgObject(0, Base);
						Context->SetArg32(1, (int)Poll);
					});
				});
			}

			void SocketRouterListen1(Network::SocketRouter* Base, const std::string_view& Hostname, const std::string_view& Service, bool Secure)
			{
				ExpectsWrapper::Unwrap(Base->Listen(Hostname, Service, Secure), (Network::RouterListener*)nullptr);
			}
			void SocketRouterListen2(Network::SocketRouter* Base, const std::string_view& Pattern, const std::string_view& Hostname, const std::string_view& Service, bool Secure)
			{
				ExpectsWrapper::Unwrap(Base->Listen(Pattern, Hostname, Service, Secure), (Network::RouterListener*)nullptr);
			}
			void SocketRouterSetListeners(Network::SocketRouter* Base, Dictionary* Data)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_DICTIONARY "@");
				Base->Listeners = Dictionary::Decompose<Network::RouterListener>(Type.GetTypeId(), Data);
			}
			Dictionary* SocketRouterGetListeners(Network::SocketRouter* Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_DICTIONARY "@");
				return Dictionary::Compose<Network::RouterListener>(Type.GetTypeId(), Base->Listeners);
			}
			void SocketRouterSetCertificates(Network::SocketRouter* Base, Dictionary* Data)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_DICTIONARY "@");
				Base->Certificates = Dictionary::Decompose<Network::SocketCertificate>(Type.GetTypeId(), Data);
			}
			Dictionary* SocketRouterGetCertificates(Network::SocketRouter* Base)
			{
				TypeInfo Type = VirtualMachine::Get()->GetTypeInfoByDecl(TYPENAME_DICTIONARY "@");
				return Dictionary::Compose<Network::SocketCertificate>(Type.GetTypeId(), Base->Certificates);
			}

			void SocketServerSetRouter(Network::SocketServer* Server, Network::SocketRouter* Router)
			{
				if (Router != nullptr)
					Router->AddRef();
				Server->SetRouter(Router);
			}
			bool SocketServerConfigure(Network::SocketServer* Server, Network::SocketRouter* Router)
			{
				if (Router != nullptr)
					Router->AddRef();
				return ExpectsWrapper::UnwrapVoid(Server->Configure(Router));
			}
			bool SocketServerListen(Network::SocketServer* Server)
			{
				return ExpectsWrapper::UnwrapVoid(Server->Listen());
			}
			bool SocketServerUnlisten(Network::SocketServer* Server, bool Gracefully)
			{
				return ExpectsWrapper::UnwrapVoid(Server->Unlisten(Gracefully));
			}

			Core::Promise<bool> SocketClientConnectSync(Network::SocketClient* Client, const Network::SocketAddress& Address, int32_t VerifyPeers)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Client->ConnectSync(Address, VerifyPeers).Then<bool>([Context](Core::ExpectsSystem<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> SocketClientConnectAsync(Network::SocketClient* Client, const Network::SocketAddress& Address, int32_t VerifyPeers)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Client->ConnectAsync(Address, VerifyPeers).Then<bool>([Context](Core::ExpectsSystem<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> SocketClientDisconnect(Network::SocketClient* Client)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Client->Disconnect().Then<bool>([Context](Core::ExpectsSystem<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}

			bool SocketConnectionAbort(Network::SocketConnection* Base, int Code, const std::string_view& Message)
			{
				return Base->Abort(Code, "%.*s", (int)Message.size(), Message.data());
			}

			void VMCollectGarbage(uint64_t IntervalMs)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				VirtualMachine* VM = VirtualMachine::Get();
				if (VM != nullptr)
					VM->PerformPeriodicGarbageCollection(IntervalMs);
			}

			template <typename T>
			void* ProcessorDuplicate(T* Base, Layer::AssetCache* Cache, Core::Schema* Args)
			{
				return ExpectsWrapper::Unwrap(Base->Duplicate(Cache, ToVariantKeys(Args)), (void*)nullptr);
			}
			template <typename T>
			void* ProcessorDeserialize(T* Base, Core::Stream* Stream, size_t Offset, Core::Schema* Args)
			{
				return ExpectsWrapper::Unwrap(Base->Deserialize(Stream, Offset, ToVariantKeys(Args)), (void*)nullptr);
			}
			template <typename T>
			bool ProcessorSerialize(T* Base, Core::Stream* Stream, void* Instance, Core::Schema* Args)
			{
				return ExpectsWrapper::UnwrapVoid(Base->Serialize(Stream, Instance, ToVariantKeys(Args)));
			}
			template <typename T>
			void PopulateProcessorBase(RefClass& Class, bool BaseCast = true)
			{
				if (BaseCast)
					Class.SetOperatorEx(Operators::Cast, 0, "void", "?&out", &HandleToHandleCast);

				Class.SetMethod("void free(uptr@)", &Layer::Processor::Free);
				Class.SetMethodEx("uptr@ duplicate(uptr@, schema@+)", &ProcessorDuplicate<T>);
				Class.SetMethodEx("uptr@ deserialize(base_stream@+, usize, schema@+)", &ProcessorDeserialize<T>);
				Class.SetMethodEx("bool serialize(base_stream@+, uptr@, schema@+)", &ProcessorSerialize<T>);
				Class.SetMethod("content_manager@+ get_content() const", &Layer::Processor::GetContent);
			}
			template <typename T, typename... Args>
			void PopulateProcessorInterface(RefClass& Class, const char* Constructor)
			{
				Class.SetConstructor<T, Args...>(Constructor);
				Class.SetDynamicCast<T, Layer::Processor>("base_processor@+", true);
				PopulateProcessorBase<T>(Class, false);
			}

			void* ContentManagerLoad(Layer::ContentManager* Base, Layer::Processor* Source, const std::string_view& Path, Core::Schema* Args)
			{
				return ExpectsWrapper::Unwrap(Base->Load(Source, Path, ToVariantKeys(Args)), (void*)nullptr);
			}
			bool ContentManagerSave(Layer::ContentManager* Base, Layer::Processor* Source, const std::string_view& Path, void* Object, Core::Schema* Args)
			{
				return ExpectsWrapper::UnwrapVoid(Base->Save(Source, Path, Object, ToVariantKeys(Args)));
			}
			void ContentManagerLoadDeferred2(Layer::ContentManager* Base, Layer::Processor* Source, const std::string_view& Path, Core::Schema* Args, asIScriptFunction* Callback)
			{
				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
					return;

				Base->LoadDeferred(Source, Path, ToVariantKeys(Args)).When([Delegate](Layer::ExpectsContent<void*>&& Object) mutable
				{
					void* Address = Object.Or(nullptr);
					Delegate([Address](ImmediateContext* Context)
					{
						Context->SetArgAddress(0, Address);
					});
				});
			}
			void ContentManagerLoadDeferred1(Layer::ContentManager* Base, Layer::Processor* Source, const std::string_view& Path, asIScriptFunction* Callback)
			{
				ContentManagerLoadDeferred2(Base, Source, Path, nullptr, Callback);
			}
			void ContentManagerSaveDeferred2(Layer::ContentManager* Base, Layer::Processor* Source, const std::string_view& Path, void* Object, Core::Schema* Args, asIScriptFunction* Callback)
			{
				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
					return;

				Base->SaveDeferred(Source, Path, Object, ToVariantKeys(Args)).When([Delegate](Layer::ExpectsContent<void>&& Status) mutable
				{
					bool Success = !!Status;
					Delegate([Success](ImmediateContext* Context)
					{
						Context->SetArg8(0, Success);
					});
				});
			}
			void ContentManagerSaveDeferred1(Layer::ContentManager* Base, Layer::Processor* Source, const std::string_view& Path, void* Object, asIScriptFunction* Callback)
			{
				ContentManagerSaveDeferred2(Base, Source, Path, Object, nullptr, Callback);
			}
			bool ContentManagerFindCache1(Layer::ContentManager* Base, Layer::Processor* Source, Layer::AssetCache& Output, const std::string_view& Path)
			{
				auto* Cache = Base->FindCache(Source, Path);
				if (!Cache)
					return false;

				Output = *Cache;
				return true;
			}
			bool ContentManagerFindCache2(Layer::ContentManager* Base, Layer::Processor* Source, Layer::AssetCache& Output, void* Object)
			{
				auto* Cache = Base->FindCache(Source, Object);
				if (!Cache)
					return false;

				Output = *Cache;
				return true;
			}
			Layer::Processor* ContentManagerAddProcessor(Layer::ContentManager* Base, Layer::Processor* Source, uint64_t Id)
			{
				if (!Source)
					return nullptr;

				return Base->AddProcessor(Source, Id);
			}

			Core::String ResourceGetHeaderBlob(Network::HTTP::Resource* Base, const std::string_view& Name)
			{
				auto* Value = Base->GetHeaderBlob(Name);
				return Value ? *Value : Core::String();
			}

			Core::Schema* ContentFrameGetJSON(Network::HTTP::ContentFrame& Base)
			{
				return ExpectsWrapper::Unwrap(Base.GetJSON(), (Core::Schema*)nullptr);
			}
			Core::Schema* ContentFrameGetXML(Network::HTTP::ContentFrame& Base)
			{
				return ExpectsWrapper::Unwrap(Base.GetXML(), (Core::Schema*)nullptr);
			}
			void ContentFramePrepare1(Network::HTTP::ContentFrame& Base, const Network::HTTP::RequestFrame& Target, const std::string_view& LeftoverContent)
			{
				Base.Prepare(Target.Headers, (uint8_t*)LeftoverContent.data(), LeftoverContent.size());
			}
			void ContentFramePrepare2(Network::HTTP::ContentFrame& Base, const Network::HTTP::ResponseFrame& Target, const std::string_view& LeftoverContent)
			{
				Base.Prepare(Target.Headers, (uint8_t*)LeftoverContent.data(), LeftoverContent.size());
			}
			void ContentFramePrepare3(Network::HTTP::ContentFrame& Base, const Network::HTTP::FetchFrame& Target, const std::string_view& LeftoverContent)
			{
				Base.Prepare(Target.Headers, (uint8_t*)LeftoverContent.data(), LeftoverContent.size());
			}
			void ContentFrameAddResource(Network::HTTP::ContentFrame& Base, const Network::HTTP::Resource& Item)
			{
				Base.Resources.push_back(Item);
			}
			void ContentFrameClearResources(Network::HTTP::ContentFrame& Base)
			{
				Base.Resources.clear();
			}
			size_t ContentFrameGetResourcesSize(Network::HTTP::ContentFrame& Base)
			{
				return Base.Resources.size();
			}
			Network::HTTP::Resource ContentFrameGetResource(Network::HTTP::ContentFrame& Base, size_t Index)
			{
				if (Index >= Base.Resources.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return Network::HTTP::Resource();
				}

				return Base.Resources[Index];
			}

			Core::String RequestFrameGetHeaderBlob(Network::HTTP::RequestFrame& Base, const std::string_view& Name)
			{
				auto* Value = Base.GetHeaderBlob(Name);
				return Value ? *Value : Core::String();
			}
			Core::String RequestFrameGetHeader(Network::HTTP::RequestFrame& Base, size_t Index, size_t Subindex)
			{
				if (Index >= Base.Headers.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return Core::String();
				}

				auto It = Base.Headers.begin();
				while (Index-- > 0)
					++It;

				if (Subindex >= It->second.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return Core::String();
				}

				return It->second[Subindex];
			}
			size_t RequestFrameGetHeaderSize(Network::HTTP::RequestFrame& Base, size_t Index)
			{
				if (Index >= Base.Headers.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return 0;
				}

				auto It = Base.Headers.begin();
				while (Index-- > 0)
					++It;

				return It->second.size();
			}
			size_t RequestFrameGetHeadersSize(Network::HTTP::RequestFrame& Base)
			{
				return Base.Headers.size();
			}
			Core::String RequestFrameGetCookieBlob(Network::HTTP::RequestFrame& Base, const std::string_view& Name)
			{
				auto* Value = Base.GetCookieBlob(Name);
				return Value ? *Value : Core::String();
			}
			Core::String RequestFrameGetCookie(Network::HTTP::RequestFrame& Base, size_t Index, size_t Subindex)
			{
				if (Index >= Base.Cookies.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return Core::String();
				}

				auto It = Base.Cookies.begin();
				while (Index-- > 0)
					++It;

				if (Subindex >= It->second.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return Core::String();
				}

				return It->second[Subindex];
			}
			size_t RequestFrameGetCookieSize(Network::HTTP::RequestFrame& Base, size_t Index)
			{
				if (Index >= Base.Cookies.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return 0;
				}

				auto It = Base.Cookies.begin();
				while (Index-- > 0)
					++It;

				return It->second.size();
			}
			size_t RequestFrameGetCookiesSize(Network::HTTP::RequestFrame& Base)
			{
				return Base.Cookies.size();
			}
			void RequestFrameSetMethod(Network::HTTP::RequestFrame& Base, const std::string_view& Value)
			{
				Base.SetMethod(Value);
			}
			Core::String RequestFrameGetMethod(Network::HTTP::RequestFrame& Base)
			{
				return Base.Method;
			}
			Core::String RequestFrameGetVersion(Network::HTTP::RequestFrame& Base)
			{
				return Base.Version;
			}

			Core::String ResponseFrameGetHeaderBlob(Network::HTTP::ResponseFrame& Base, const std::string_view& Name)
			{
				auto* Value = Base.GetHeaderBlob(Name);
				return Value ? *Value : Core::String();
			}
			Core::String ResponseFrameGetHeader(Network::HTTP::ResponseFrame& Base, size_t Index, size_t Subindex)
			{
				if (Index >= Base.Headers.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return Core::String();
				}

				auto It = Base.Headers.begin();
				while (Index-- > 0)
					++It;

				if (Subindex >= It->second.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return Core::String();
				}

				return It->second[Subindex];
			}
			size_t ResponseFrameGetHeaderSize(Network::HTTP::ResponseFrame& Base, size_t Index)
			{
				if (Index >= Base.Headers.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return 0;
				}

				auto It = Base.Headers.begin();
				while (Index-- > 0)
					++It;

				return It->second.size();
			}
			size_t ResponseFrameGetHeadersSize(Network::HTTP::ResponseFrame& Base)
			{
				return Base.Headers.size();
			}
			Network::HTTP::Cookie ResponseFrameGetCookie1(Network::HTTP::ResponseFrame& Base, const std::string_view& Name)
			{
				auto Value = Base.GetCookie(Name);
				return Value ? *Value : Network::HTTP::Cookie();
			}
			Network::HTTP::Cookie ResponseFrameGetCookie2(Network::HTTP::ResponseFrame& Base, size_t Index)
			{
				if (Index >= Base.Cookies.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return Network::HTTP::Cookie();
				}

				return Base.Cookies[Index];
			}
			size_t ResponseFrameGetCookiesSize(Network::HTTP::ResponseFrame& Base)
			{
				return Base.Cookies.size();
			}

			Core::String FetchFrameGetHeaderBlob(Network::HTTP::FetchFrame& Base, const std::string_view& Name)
			{
				auto* Value = Base.GetHeaderBlob(Name);
				return Value ? *Value : Core::String();
			}
			Core::String FetchFrameGetHeader(Network::HTTP::FetchFrame& Base, size_t Index, size_t Subindex)
			{
				if (Index >= Base.Headers.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return Core::String();
				}

				auto It = Base.Headers.begin();
				while (Index-- > 0)
					++It;

				if (Subindex >= It->second.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return Core::String();
				}

				return It->second[Subindex];
			}
			size_t FetchFrameGetHeaderSize(Network::HTTP::FetchFrame& Base, size_t Index)
			{
				if (Index >= Base.Headers.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return 0;
				}

				auto It = Base.Headers.begin();
				while (Index-- > 0)
					++It;

				return It->second.size();
			}
			size_t FetchFrameGetHeadersSize(Network::HTTP::FetchFrame& Base)
			{
				return Base.Headers.size();
			}
			Core::String FetchFrameGetCookieBlob(Network::HTTP::FetchFrame& Base, const std::string_view& Name)
			{
				auto* Value = Base.GetCookieBlob(Name);
				return Value ? *Value : Core::String();
			}
			Core::String FetchFrameGetCookie(Network::HTTP::FetchFrame& Base, size_t Index, size_t Subindex)
			{
				if (Index >= Base.Cookies.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return Core::String();
				}

				auto It = Base.Cookies.begin();
				while (Index-- > 0)
					++It;

				if (Subindex >= It->second.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return Core::String();
				}

				return It->second[Subindex];
			}
			size_t FetchFrameGetCookieSize(Network::HTTP::FetchFrame& Base, size_t Index)
			{
				if (Index >= Base.Cookies.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return 0;
				}

				auto It = Base.Cookies.begin();
				while (Index-- > 0)
					++It;

				return It->second.size();
			}
			size_t FetchFrameGetCookiesSize(Network::HTTP::FetchFrame& Base)
			{
				return Base.Cookies.size();
			}

			Network::HTTP::RouterEntry* RouterGroupGetRoute(Network::HTTP::RouterGroup* Base, size_t Index)
			{
				if (Index >= Base->Routes.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				return Base->Routes[Index];
			}
			size_t RouterGroupGetRoutesSize(Network::HTTP::RouterGroup* Base)
			{
				return Base->Routes.size();
			}

			void RouterEntrySetHiddenFiles(Network::HTTP::RouterEntry* Base, Array* Data)
			{
				if (Data != nullptr)
					Base->HiddenFiles = Array::Decompose<Compute::RegexSource>(Data);
				else
					Base->HiddenFiles.clear();
			}
			Array* RouterEntryGetHiddenFiles(Network::HTTP::RouterEntry* Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_REGEXSOURCE ">@");
				return Array::Compose<Compute::RegexSource>(Type.GetTypeInfo(), Base->HiddenFiles);
			}
			void RouterEntrySetErrorFiles(Network::HTTP::RouterEntry* Base, Array* Data)
			{
				if (Data != nullptr)
					Base->ErrorFiles = Array::Decompose<Network::HTTP::ErrorFile>(Data);
				else
					Base->ErrorFiles.clear();
			}
			Array* RouterEntryGetErrorFiles(Network::HTTP::RouterEntry* Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_HTTPERRORFILE ">@");
				return Array::Compose<Network::HTTP::ErrorFile>(Type.GetTypeInfo(), Base->ErrorFiles);
			}
			void RouterEntrySetMimeTypes(Network::HTTP::RouterEntry* Base, Array* Data)
			{
				if (Data != nullptr)
					Base->MimeTypes = Array::Decompose<Network::HTTP::MimeType>(Data);
				else
					Base->MimeTypes.clear();
			}
			Array* RouterEntryGetMimeTypes(Network::HTTP::RouterEntry* Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_HTTPMIMETYPE ">@");
				return Array::Compose<Network::HTTP::MimeType>(Type.GetTypeInfo(), Base->MimeTypes);
			}
			void RouterEntrySetIndexFiles(Network::HTTP::RouterEntry* Base, Array* Data)
			{
				if (Data != nullptr)
					Base->IndexFiles = Array::Decompose<Core::String>(Data);
				else
					Base->IndexFiles.clear();
			}
			Array* RouterEntryGetIndexFiles(Network::HTTP::RouterEntry* Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose<Core::String>(Type.GetTypeInfo(), Base->IndexFiles);
			}
			void RouterEntrySetTryFiles(Network::HTTP::RouterEntry* Base, Array* Data)
			{
				if (Data != nullptr)
					Base->TryFiles = Array::Decompose<Core::String>(Data);
				else
					Base->TryFiles.clear();
			}
			Array* RouterEntryGetTryFiles(Network::HTTP::RouterEntry* Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose<Core::String>(Type.GetTypeInfo(), Base->TryFiles);
			}
			void RouterEntrySetDisallowedMethodsFiles(Network::HTTP::RouterEntry* Base, Array* Data)
			{
				if (Data != nullptr)
					Base->DisallowedMethods = Array::Decompose<Core::String>(Data);
				else
					Base->DisallowedMethods.clear();
			}
			Array* RouterEntryGetDisallowedMethodsFiles(Network::HTTP::RouterEntry* Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose<Core::String>(Type.GetTypeInfo(), Base->DisallowedMethods);
			}
			Network::HTTP::MapRouter* RouterEntryGetRouter(Network::HTTP::RouterEntry* Base)
			{
				return Base->Router;
			}

			void RouteAuthSetMethods(Network::HTTP::RouterEntry::EntryAuth& Base, Array* Data)
			{
				if (Data != nullptr)
					Base.Methods = Array::Decompose<Core::String>(Data);
				else
					Base.Methods.clear();
			}
			Array* RouteAuthGetMethods(Network::HTTP::RouterEntry::EntryAuth& Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose<Core::String>(Type.GetTypeInfo(), Base.Methods);
			}

			void RouteCompressionSetFiles(Network::HTTP::RouterEntry::EntryCompression& Base, Array* Data)
			{
				if (Data != nullptr)
					Base.Files = Array::Decompose<Compute::RegexSource>(Data);
				else
					Base.Files.clear();
			}
			Array* RouteCompressionGetFiles(Network::HTTP::RouterEntry::EntryCompression& Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_REGEXSOURCE ">@");
				return Array::Compose<Compute::RegexSource>(Type.GetTypeInfo(), Base.Files);
			}

			Network::HTTP::RouterEntry* MapRouterGetBase(Network::HTTP::MapRouter* Base)
			{
				return Base->Base;
			}
			Network::HTTP::RouterGroup* MapRouterGetGroup(Network::HTTP::MapRouter* Base, size_t Index)
			{
				if (Index >= Base->Groups.size())
				{
					Bindings::Exception::Throw(Bindings::Exception::Pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				return Base->Groups[Index];
			}
			size_t MapRouterGetGroupsSize(Network::HTTP::MapRouter* Base)
			{
				return Base->Groups.size();
			}
			Network::HTTP::RouterEntry* MapRouterFetchRoute(Network::HTTP::MapRouter* Base, const std::string_view& Match, Network::HTTP::RouteMode Mode, const std::string_view& Pattern)
			{
				return Base->Route(Match, Mode, Pattern, false);
			}
			bool MapRouterGet2(Network::HTTP::MapRouter* Base, const std::string_view& Match, Network::HTTP::RouteMode Mode, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				auto* Route = MapRouterFetchRoute(Base, Match, Mode, Pattern);
				if (!Route)
					return false;

				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
				{
					Route->Callbacks.Get = nullptr;
					return false;
				}

				Route->Callbacks.Get = [Delegate](Network::HTTP::Connection* Base) mutable
				{
					Delegate([Base](ImmediateContext* Context) { Context->SetArgObject(0, Base); });
					return true;
				};
				return true;
			}
			bool MapRouterGet1(Network::HTTP::MapRouter* Base, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				return MapRouterGet2(Base, "", Network::HTTP::RouteMode::Start, Pattern, Callback);
			}
			bool MapRouterPost2(Network::HTTP::MapRouter* Base, const std::string_view& Match, Network::HTTP::RouteMode Mode, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				auto* Route = MapRouterFetchRoute(Base, Match, Mode, Pattern);
				if (!Route)
					return false;

				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
				{
					Route->Callbacks.Post = nullptr;
					return false;
				}

				Route->Callbacks.Post = [Delegate](Network::HTTP::Connection* Base) mutable
				{
					Delegate([Base](ImmediateContext* Context) { Context->SetArgObject(0, Base); });
					return true;
				};
				return true;
			}
			bool MapRouterPost1(Network::HTTP::MapRouter* Base, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				return MapRouterPost2(Base, "", Network::HTTP::RouteMode::Start, Pattern, Callback);
			}
			bool MapRouterPatch2(Network::HTTP::MapRouter* Base, const std::string_view& Match, Network::HTTP::RouteMode Mode, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				auto* Route = MapRouterFetchRoute(Base, Match, Mode, Pattern);
				if (!Route)
					return false;

				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
				{
					Route->Callbacks.Patch = nullptr;
					return false;
				}

				Route->Callbacks.Patch = [Delegate](Network::HTTP::Connection* Base) mutable
				{
					Delegate([Base](ImmediateContext* Context) { Context->SetArgObject(0, Base); });
					return true;
				};
				return true;
			}
			bool MapRouterPatch1(Network::HTTP::MapRouter* Base, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				return MapRouterPatch2(Base, "", Network::HTTP::RouteMode::Start, Pattern, Callback);
			}
			bool MapRouterDelete2(Network::HTTP::MapRouter* Base, const std::string_view& Match, Network::HTTP::RouteMode Mode, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				auto* Route = MapRouterFetchRoute(Base, Match, Mode, Pattern);
				if (!Route)
					return false;

				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
				{
					Route->Callbacks.Delete = nullptr;
					return false;
				}

				Route->Callbacks.Delete = [Delegate](Network::HTTP::Connection* Base) mutable
				{
					Delegate([Base](ImmediateContext* Context) { Context->SetArgObject(0, Base); });
					return true;
				};
				return true;
			}
			bool MapRouterDelete1(Network::HTTP::MapRouter* Base, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				return MapRouterDelete2(Base, "", Network::HTTP::RouteMode::Start, Pattern, Callback);
			}
			bool MapRouterOptions2(Network::HTTP::MapRouter* Base, const std::string_view& Match, Network::HTTP::RouteMode Mode, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				auto* Route = MapRouterFetchRoute(Base, Match, Mode, Pattern);
				if (!Route)
					return false;

				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
				{
					Route->Callbacks.Options = nullptr;
					return false;
				}

				Route->Callbacks.Options = [Delegate](Network::HTTP::Connection* Base) mutable
				{
					Delegate([Base](ImmediateContext* Context) { Context->SetArgObject(0, Base); });
					return true;
				};
				return true;
			}
			bool MapRouterOptions1(Network::HTTP::MapRouter* Base, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				return MapRouterOptions2(Base, "", Network::HTTP::RouteMode::Start, Pattern, Callback);
			}
			bool MapRouterAccess2(Network::HTTP::MapRouter* Base, const std::string_view& Match, Network::HTTP::RouteMode Mode, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				auto* Route = MapRouterFetchRoute(Base, Match, Mode, Pattern);
				if (!Route)
					return false;

				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
				{
					Route->Callbacks.Access = nullptr;
					return false;
				}

				Route->Callbacks.Access = [Delegate](Network::HTTP::Connection* Base) mutable
				{
					Delegate([Base](ImmediateContext* Context) { Context->SetArgObject(0, Base); });
					return true;
				};
				return true;
			}
			bool MapRouterAccess1(Network::HTTP::MapRouter* Base, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				return MapRouterAccess2(Base, "", Network::HTTP::RouteMode::Start, Pattern, Callback);
			}
			bool MapRouterHeaders2(Network::HTTP::MapRouter* Base, const std::string_view& Match, Network::HTTP::RouteMode Mode, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				auto* Route = MapRouterFetchRoute(Base, Match, Mode, Pattern);
				if (!Route)
					return false;

				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
				{
					Route->Callbacks.Headers = nullptr;
					return false;
				}

				Route->Callbacks.Headers = [Delegate](Network::HTTP::Connection* Base, Core::String& Source) mutable
				{
					Delegate([Base, &Source](ImmediateContext* Context)
					{
						Context->SetArgObject(0, Base);
						Context->SetArgObject(1, &Source);
					}).Wait();
					return true;
				};
				return true;
			}
			bool MapRouterHeaders1(Network::HTTP::MapRouter* Base, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				return MapRouterHeaders2(Base, "", Network::HTTP::RouteMode::Start, Pattern, Callback);
			}
			bool MapRouterAuthorize2(Network::HTTP::MapRouter* Base, const std::string_view& Match, Network::HTTP::RouteMode Mode, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				auto* Route = MapRouterFetchRoute(Base, Match, Mode, Pattern);
				if (!Route)
					return false;

				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
				{
					Route->Callbacks.Authorize = nullptr;
					return false;
				}

				Route->Callbacks.Authorize = [Delegate](Network::HTTP::Connection* Base, Network::HTTP::Credentials* Source) mutable
				{
					Delegate([Base, Source](ImmediateContext* Context)
					{
						Context->SetArgObject(0, Base);
						Context->SetArgObject(1, Source);
					}).Wait();
					return true;
				};
				return true;
			}
			bool MapRouterAuthorize1(Network::HTTP::MapRouter* Base, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				return MapRouterAuthorize2(Base, "", Network::HTTP::RouteMode::Start, Pattern, Callback);
			}
			bool MapRouterWebsocketInitiate2(Network::HTTP::MapRouter* Base, const std::string_view& Match, Network::HTTP::RouteMode Mode, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				auto* Route = MapRouterFetchRoute(Base, Match, Mode, Pattern);
				if (!Route)
					return false;

				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
				{
					Route->Callbacks.WebSocket.Initiate = nullptr;
					return false;
				}

				Route->Callbacks.WebSocket.Initiate = [Delegate](Network::HTTP::Connection* Base) mutable
				{
					Delegate([Base](ImmediateContext* Context) { Context->SetArgObject(0, Base); });
					return true;
				};
				return true;
			}
			bool MapRouterWebsocketInitiate1(Network::HTTP::MapRouter* Base, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				return MapRouterWebsocketInitiate2(Base, "", Network::HTTP::RouteMode::Start, Pattern, Callback);
			}
			bool MapRouterWebsocketConnect2(Network::HTTP::MapRouter* Base, const std::string_view& Match, Network::HTTP::RouteMode Mode, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				auto* Route = MapRouterFetchRoute(Base, Match, Mode, Pattern);
				if (!Route)
					return false;

				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
				{
					Route->Callbacks.WebSocket.Connect = nullptr;
					return false;
				}

				Route->Callbacks.WebSocket.Connect = [Delegate](Network::HTTP::WebSocketFrame* Base) mutable
				{
					Delegate([Base](ImmediateContext* Context) { Context->SetArgObject(0, Base); });
					return true;
				};
				return true;
			}
			bool MapRouterWebsocketConnect1(Network::HTTP::MapRouter* Base, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				return MapRouterWebsocketConnect2(Base, "", Network::HTTP::RouteMode::Start, Pattern, Callback);
			}
			bool MapRouterWebsocketDisconnect2(Network::HTTP::MapRouter* Base, const std::string_view& Match, Network::HTTP::RouteMode Mode, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				auto* Route = MapRouterFetchRoute(Base, Match, Mode, Pattern);
				if (!Route)
					return false;

				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
				{
					Route->Callbacks.WebSocket.Disconnect = nullptr;
					return false;
				}

				Route->Callbacks.WebSocket.Disconnect = [Delegate](Network::HTTP::WebSocketFrame* Base) mutable
				{
					Delegate([Base](ImmediateContext* Context) { Context->SetArgObject(0, Base); });
					return true;
				};
				return true;
			}
			bool MapRouterWebsocketDisconnect1(Network::HTTP::MapRouter* Base, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				return MapRouterWebsocketDisconnect2(Base, "", Network::HTTP::RouteMode::Start, Pattern, Callback);
			}
			bool MapRouterWebsocketReceive2(Network::HTTP::MapRouter* Base, const std::string_view& Match, Network::HTTP::RouteMode Mode, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				auto* Route = MapRouterFetchRoute(Base, Match, Mode, Pattern);
				if (!Route)
					return false;

				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
				{
					Route->Callbacks.WebSocket.Receive = nullptr;
					return false;
				}

				Route->Callbacks.WebSocket.Receive = [Delegate](Network::HTTP::WebSocketFrame* Base, Network::HTTP::WebSocketOp Opcode, const std::string_view& Data) mutable
				{
					Core::String Buffer = Core::String(Data);
					Delegate([Base, Opcode, Buffer](ImmediateContext* Context)
					{
						Context->SetArgObject(0, Base);
						Context->SetArg32(1, (int)Opcode);
						Context->SetArgObject(2, (void*)&Buffer);
					});
					return true;
				};
				return true;
			}
			bool MapRouterWebsocketReceive1(Network::HTTP::MapRouter* Base, const std::string_view& Pattern, asIScriptFunction* Callback)
			{
				return MapRouterWebsocketReceive2(Base, "", Network::HTTP::RouteMode::Start, Pattern, Callback);
			}
			bool WebSocketFrameSetOnConnect(Network::HTTP::WebSocketFrame* Base, asIScriptFunction* Callback)
			{
				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
				{
					Base->Connect = nullptr;
					return false;
				}

				Base->Connect = [Delegate](Network::HTTP::WebSocketFrame* Base) mutable
				{
					Delegate([Base](ImmediateContext* Context) { Context->SetArgObject(0, Base); });
				};
				return true;
			}
			bool WebSocketFrameSetOnBeforeDisconnect(Network::HTTP::WebSocketFrame* Base, asIScriptFunction* Callback)
			{
				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
				{
					Base->BeforeDisconnect = nullptr;
					return false;
				}

				Base->BeforeDisconnect = [Delegate](Network::HTTP::WebSocketFrame* Base) mutable
				{
					Delegate([Base](ImmediateContext* Context) { Context->SetArgObject(0, Base); });
				};
				return true;
			}
			bool WebSocketFrameSetOnDisconnect(Network::HTTP::WebSocketFrame* Base, asIScriptFunction* Callback)
			{
				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
				{
					Base->Disconnect = nullptr;
					return false;
				}

				Base->Disconnect = [Delegate](Network::HTTP::WebSocketFrame* Base) mutable
				{
					Delegate([Base](ImmediateContext* Context) { Context->SetArgObject(0, Base); });
				};
				return true;
			}
			bool WebSocketFrameSetOnReceive(Network::HTTP::WebSocketFrame* Base, asIScriptFunction* Callback)
			{
				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
				{
					Base->Receive = nullptr;
					return false;
				}

				Base->Receive = [Delegate](Network::HTTP::WebSocketFrame* Base, Network::HTTP::WebSocketOp Opcode, const std::string_view& Data) mutable
				{
					Core::String Buffer = Core::String(Data);
					Delegate([Base, Opcode, Buffer](ImmediateContext* Context)
					{
						Context->SetArgObject(0, Base);
						Context->SetArg32(1, (int)Opcode);
						Context->SetArgObject(2, (void*)&Buffer);
					});
					return true;
				};
				return true;
			}
			Core::Promise<bool> WebSocketFrameSend2(Network::HTTP::WebSocketFrame* Base, uint32_t Mask, const std::string_view& Data, Network::HTTP::WebSocketOp Opcode)
			{
				Core::Promise<bool> Result;
				ExpectsWrapper::UnwrapVoid(Base->Send(Mask, Data, Opcode, [Result](Network::HTTP::WebSocketFrame* Base) mutable { Result.Set(true); }));
				return Result;
			}
			Core::Promise<bool> WebSocketFrameSend1(Network::HTTP::WebSocketFrame* Base, const std::string_view& Data, Network::HTTP::WebSocketOp Opcode)
			{
				return WebSocketFrameSend2(Base, 0, Data, Opcode);
			}
			Core::Promise<bool> WebSocketFrameSendClose(Network::HTTP::WebSocketFrame* Base, uint32_t Mask, const std::string_view& Data, Network::HTTP::WebSocketOp Opcode)
			{
				Core::Promise<bool> Result;
				ExpectsWrapper::UnwrapVoid(Base->SendClose([Result](Network::HTTP::WebSocketFrame* Base) mutable { Result.Set(true); }));
				return Result;
			}

			Core::Promise<bool> ConnectionSendHeaders(Network::HTTP::Connection* Base, int StatusCode, bool SpecifyTransferEncoding)
			{
				Core::Promise<bool> Result; ImmediateContext* Context = ImmediateContext::Get();
				bool Sending = Base->SendHeaders(StatusCode, SpecifyTransferEncoding, [Result, Context](Network::HTTP::Connection*, Network::SocketPoll Event) mutable
				{
					Result.Set(ExpectsWrapper::UnwrapVoid(FromSocketPoll(Event), Context));
				});
				if (!Sending)
					Result.Set(ExpectsWrapper::UnwrapVoid(Core::ExpectsSystem<void>(Core::SystemException("cannot send headers: illegal operation")), Context));
				return Result;
			}
			Core::Promise<bool> ConnectionSendChunk(Network::HTTP::Connection* Base, const std::string_view& Chunk)
			{
				Core::Promise<bool> Result; ImmediateContext* Context = ImmediateContext::Get();
				bool Sending = Base->SendChunk(Chunk, [Result, Context](Network::HTTP::Connection*, Network::SocketPoll Event) mutable
				{
					Result.Set(ExpectsWrapper::UnwrapVoid(FromSocketPoll(Event), Context));
				});
				if (!Sending)
					Result.Set(ExpectsWrapper::UnwrapVoid(Core::ExpectsSystem<void>(Core::SystemException("cannot send chunk: illegal operation")), Context));
				return Result;
			}
			Core::Promise<bool> ConnectionSkip(Network::HTTP::Connection* Base)
			{
				Core::Promise<bool> Result;
				Base->Skip([Result](Network::HTTP::Connection*) mutable { Result.Set(true); return true; });
				return Result;
			}
			Core::Promise<Array*> ConnectionStore(Network::HTTP::Connection* Base, bool Eat)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return Core::Promise<Array*>((Array*)nullptr);

				Core::Promise<Array*> Result;
				Core::Vector<Network::HTTP::Resource>* Resources = Core::Memory::New<Core::Vector<Network::HTTP::Resource>>();
				asITypeInfo* Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_HTTPRESOURCEINFO ">@").GetTypeInfo();
				Base->Store([Result, Resources, Type](Network::HTTP::Resource* Next) mutable
				{
					if (Next != nullptr)
					{
						Resources->push_back(*Next);
						return true;
					}

					Result.Set(Array::Compose<Network::HTTP::Resource>(Type, *Resources));
					Core::Memory::Delete(Resources);
					return true;
				}, Eat);
				return Result;
			}
			Core::Promise<Core::String> ConnectionFetch(Network::HTTP::Connection* Base, bool Eat)
			{
				Core::Promise<Core::String> Result;
				Core::String* Data = Core::Memory::New<Core::String>();
				Base->Fetch([Result, Data](Network::HTTP::Connection*, Network::SocketPoll Poll, const std::string_view& Buffer) mutable
				{
					if (Buffer.empty())
					{
						Result.Set(std::move(*Data));
						Core::Memory::Delete(Data);
					}
					else
						Data->append(Buffer);
					return true;
				}, Eat);
				return Result;
			}
			Core::String ConnectionGetPeerIpAddress(Network::HTTP::Connection* Base)
			{
				return ExpectsWrapper::Unwrap(Base->GetPeerIpAddress(), Core::String());
			}
			Network::HTTP::WebSocketFrame* ConnectionGetWebSocket(Network::HTTP::Connection* Base)
			{
				return Base->WebSocket;
			}
			Network::HTTP::RouterEntry* ConnectionGetRoute(Network::HTTP::Connection* Base)
			{
				return Base->Route;
			}
			Network::HTTP::Server* ConnectionGetServer(Network::HTTP::Connection* Base)
			{
				return Base->Root;
			}

			void QueryDecode(Network::HTTP::Query* Base, const std::string_view& ContentType, const std::string_view& Data)
			{
				Base->Decode(ContentType, Data);
			}
			Core::String QueryEncode(Network::HTTP::Query* Base, const std::string_view& ContentType)
			{
				return Base->Encode(ContentType);
			}
			void QuerySetData(Network::HTTP::Query* Base, Core::Schema* Data)
			{
				Core::Memory::Release(Base->Object);
				Base->Object = Data;
				if (Base->Object != nullptr)
					Base->Object->AddRef();
			}
			Core::Schema* QueryGetData(Network::HTTP::Query* Base)
			{
				return Base->Object;
			}

			void SessionSetData(Network::HTTP::Session* Base, Core::Schema* Data)
			{
				Core::Memory::Release(Base->Query);
				Base->Query = Data;
				if (Base->Query != nullptr)
					Base->Query->AddRef();
			}
			Core::Schema* SessionGetData(Network::HTTP::Session* Base)
			{
				return Base->Query;
			}

			Core::Promise<bool> ClientSkip(Network::HTTP::Client* Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->Skip().Then<bool>([Context](Core::ExpectsSystem<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> ClientFetch(Network::HTTP::Client* Base, size_t MaxSize, bool Eat)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->Fetch(MaxSize, Eat).Then<bool>([Context](Core::ExpectsSystem<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> ClientUpgrade(Network::HTTP::Client* Base, const Network::HTTP::RequestFrame& Frame)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->Upgrade(Network::HTTP::RequestFrame(Frame)).Then<bool>([Context](Core::ExpectsSystem<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> ClientSend(Network::HTTP::Client* Base, const Network::HTTP::RequestFrame& Frame)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->Send(Network::HTTP::RequestFrame(Frame)).Then<bool>([Context](Core::ExpectsSystem<void>&& Result) -> bool
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> ClientSendFetch(Network::HTTP::Client* Base, const Network::HTTP::RequestFrame& Frame, size_t MaxSize)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->SendFetch(Network::HTTP::RequestFrame(Frame), MaxSize).Then<bool>([Context](Core::ExpectsSystem<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<Core::Schema*> ClientJSON(Network::HTTP::Client* Base, const Network::HTTP::RequestFrame& Frame, size_t MaxSize)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->JSON(Network::HTTP::RequestFrame(Frame), MaxSize).Then<Core::Schema*>([Context](Core::ExpectsSystem<Core::Schema*>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), (Core::Schema*)nullptr, Context);
				});
			}
			Core::Promise<Core::Schema*> ClientXML(Network::HTTP::Client* Base, const Network::HTTP::RequestFrame& Frame, size_t MaxSize)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->XML(Network::HTTP::RequestFrame(Frame), MaxSize).Then<Core::Schema*>([Context](Core::ExpectsSystem<Core::Schema*>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), (Core::Schema*)nullptr, Context);
				});
			}

			Core::Promise<Network::HTTP::ResponseFrame> HTTPFetch(const std::string_view& Location, const std::string_view& Method, const Network::HTTP::FetchFrame& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Network::HTTP::Fetch(Location, Method, Options).Then<Network::HTTP::ResponseFrame>([Context](Core::ExpectsSystem<Network::HTTP::ResponseFrame>&& Response) -> Network::HTTP::ResponseFrame
				{
					return ExpectsWrapper::Unwrap(std::move(Response), Network::HTTP::ResponseFrame(), Context);
				});
			}

			Array* SMTPRequestGetRecipients(Network::SMTP::RequestFrame* Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_SMTPRECIPIENT ">@");
				return Array::Compose<Network::SMTP::Recipient>(Type.GetTypeInfo(), Base->Recipients);
			}
			void SMTPRequestSetRecipients(Network::SMTP::RequestFrame* Base, Array* Data)
			{
				if (Data != nullptr)
					Base->Recipients = Array::Decompose<Network::SMTP::Recipient>(Data);
				else
					Base->Recipients.clear();
			}
			Array* SMTPRequestGetCCRecipients(Network::SMTP::RequestFrame* Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_SMTPRECIPIENT ">@");
				return Array::Compose<Network::SMTP::Recipient>(Type.GetTypeInfo(), Base->CCRecipients);
			}
			void SMTPRequestSetCCRecipients(Network::SMTP::RequestFrame* Base, Array* Data)
			{
				if (Data != nullptr)
					Base->CCRecipients = Array::Decompose<Network::SMTP::Recipient>(Data);
				else
					Base->CCRecipients.clear();
			}
			Array* SMTPRequestGetBCCRecipients(Network::SMTP::RequestFrame* Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_SMTPRECIPIENT ">@");
				return Array::Compose<Network::SMTP::Recipient>(Type.GetTypeInfo(), Base->BCCRecipients);
			}
			void SMTPRequestSetBCCRecipients(Network::SMTP::RequestFrame* Base, Array* Data)
			{
				if (Data != nullptr)
					Base->BCCRecipients = Array::Decompose<Network::SMTP::Recipient>(Data);
				else
					Base->BCCRecipients.clear();
			}
			Array* SMTPRequestGetAttachments(Network::SMTP::RequestFrame* Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_SMTPATTACHMENT ">@");
				return Array::Compose<Network::SMTP::Attachment>(Type.GetTypeInfo(), Base->Attachments);
			}
			void SMTPRequestSetAttachments(Network::SMTP::RequestFrame* Base, Array* Data)
			{
				if (Data != nullptr)
					Base->Attachments = Array::Decompose<Network::SMTP::Attachment>(Data);
				else
					Base->Attachments.clear();
			}
			Array* SMTPRequestGetMessages(Network::SMTP::RequestFrame* Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose<Core::String>(Type.GetTypeInfo(), Base->Messages);
			}
			void SMTPRequestSetMessages(Network::SMTP::RequestFrame* Base, Array* Data)
			{
				if (Data != nullptr)
					Base->Messages = Array::Decompose<Core::String>(Data);
				else
					Base->Messages.clear();
			}
			void SMTPRequestSetHeader(Network::SMTP::RequestFrame* Base, const std::string_view& Name, const std::string_view& Value)
			{
				if (Value.empty())
					Base->Headers.erase(Core::String(Name));
				else
					Base->Headers[Core::String(Name)] = Value;
			}
			Core::String SMTPRequestGetHeader(Network::SMTP::RequestFrame* Base, const std::string_view& Name)
			{
				auto It = Base->Headers.find(Core::KeyLookupCast(Name));
				return It == Base->Headers.end() ? Core::String() : It->second;
			}

			Core::Promise<bool> SMTPClientSend(Network::SMTP::Client* Base, const Network::SMTP::RequestFrame& Frame)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->Send(Network::SMTP::RequestFrame(Frame)).Then<bool>([Context](Core::ExpectsSystem<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}

			Network::LDB::Response& LDBResponseCopy(Network::LDB::Response& Base, Network::LDB::Response&& Other)
			{
				if (&Base == &Other)
					return Base;

				Base = Other.Copy();
				return Base;
			}
			Array* LDBResponseGetColumns(Network::LDB::Response& Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose(Type.GetTypeInfo(), Base.GetColumns());
			}

			Network::LDB::Response LDBCursorFirst(Network::LDB::Cursor& Base)
			{
				return Base.Size() > 0 ? Base.First().Copy() : Network::LDB::Response();
			}
			Network::LDB::Response LDBCursorLast(Network::LDB::Cursor& Base)
			{
				return Base.Size() > 0 ? Base.Last().Copy() : Network::LDB::Response();
			}
			Network::LDB::Response LDBCursorAt(Network::LDB::Cursor& Base, size_t Index)
			{
				return Index < Base.Size() ? Base.At(Index).Copy() : Network::LDB::Response();
			}
			Network::LDB::Cursor& LDBCursorCopy(Network::LDB::Cursor& Base, Network::LDB::Cursor&& Other)
			{
				if (&Base == &Other)
					return Base;

				Base = Other.Copy();
				return Base;
			}

			class LDBAggregate final : public Network::LDB::Aggregate
			{
			public:
				FunctionDelegate StepDelegate = FunctionDelegate(nullptr);
				FunctionDelegate FinalizeDelegate = FunctionDelegate(nullptr);

			public:
				virtual ~LDBAggregate() = default;
				void Step(const Core::VariantList& Args) override
				{
					StepDelegate.Context->ExecuteSubcall(StepDelegate.Callable(), [&Args](ImmediateContext* Context)
					{
						TypeInfo Type = Context->GetVM()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_VARIANT ">@");
						Core::UPtr<Array> Data = Array::Compose<Core::Variant>(Type, Args);
						Context->SetArgObject(0, *Data);
					});
				}
				Core::Variant Finalize() override
				{
					Core::Variant Result = Core::Var::Undefined();
					FinalizeDelegate.Context->ExecuteSubcall(FinalizeDelegate.Callable(), nullptr, [&Result](ImmediateContext* Context) mutable
					{
						Core::Variant* Target = Context->GetReturnObject<Core::Variant>();
						if (Target != nullptr)
							Result = std::move(*Target);
					});
					return Result;
				}
			};

			class LDBWindow final : public Network::LDB::Window
			{
			public:
				FunctionDelegate StepDelegate = FunctionDelegate(nullptr);
				FunctionDelegate InverseDelegate = FunctionDelegate(nullptr);
				FunctionDelegate ValueDelegate = FunctionDelegate(nullptr);
				FunctionDelegate FinalizeDelegate = FunctionDelegate(nullptr);

			public:
				virtual ~LDBWindow() = default;
				void Step(const Core::VariantList& Args) override
				{
					StepDelegate.Context->ExecuteSubcall(StepDelegate.Callable(), [&Args](ImmediateContext* Context)
					{
						TypeInfo Type = Context->GetVM()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_VARIANT ">@");
						Core::UPtr<Array> Data = Array::Compose<Core::Variant>(Type, Args);
						Context->SetArgObject(0, *Data);
					});
				}
				void Inverse(const Core::VariantList& Args) override
				{
					InverseDelegate.Context->ExecuteSubcall(InverseDelegate.Callable(), [&Args](ImmediateContext* Context)
					{
						TypeInfo Type = Context->GetVM()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_VARIANT ">@");
						Core::UPtr<Array> Data = Array::Compose<Core::Variant>(Type, Args);
						Context->SetArgObject(0, *Data);
					});
				}
				Core::Variant Value() override
				{
					Core::Variant Result = Core::Var::Undefined();
					ValueDelegate.Context->ExecuteSubcall(ValueDelegate.Callable(), nullptr, [&Result](ImmediateContext* Context) mutable
					{
						Core::Variant* Target = Context->GetReturnObject<Core::Variant>();
						if (Target != nullptr)
							Result = std::move(*Target);
					});
					return Result;
				}
				Core::Variant Finalize() override
				{
					Core::Variant Result = Core::Var::Undefined();
					FinalizeDelegate.Context->ExecuteSubcall(FinalizeDelegate.Callable(), nullptr, [&Result](ImmediateContext* Context) mutable
					{
						Core::Variant* Target = Context->GetReturnObject<Core::Variant>();
						if (Target != nullptr)
							Result = std::move(*Target);
					});
					return Result;
				}
			};

			void LDBClusterSetFunction(Network::LDB::Cluster* Base, const std::string_view& Name, uint8_t Args, asIScriptFunction* Callback)
			{
				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
					return;
				
				Base->SetFunction(Name, Args, [Delegate](const Core::VariantList& Args) mutable -> Core::Variant
				{
					Core::Variant Result = Core::Var::Undefined();
					Delegate.Context->ExecuteSubcall(Delegate.Callable(), [&Args](ImmediateContext* Context)
					{
						TypeInfo Type = Context->GetVM()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_VARIANT ">@");
						Core::UPtr<Array> Data = Array::Compose<Core::Variant>(Type, Args);
						Context->SetArgObject(0, *Data);
					}, [&Result](ImmediateContext* Context) mutable
					{
						Core::Variant* Target = Context->GetReturnObject<Core::Variant>();
						if (Target != nullptr)
							Result = std::move(*Target);
					});
					return Result;
				});
			}
			void LDBClusterSetAggregateFunction(Network::LDB::Cluster* Base, const std::string_view& Name, uint8_t Args, asIScriptFunction* StepCallback, asIScriptFunction* FinalizeCallback)
			{
				FunctionDelegate StepDelegate(StepCallback);
				FunctionDelegate FinalizeDelegate(FinalizeCallback);
				if (!StepDelegate.IsValid() || !FinalizeDelegate.IsValid())
					return;

				LDBAggregate* Aggregate = new LDBAggregate();
				Aggregate->StepDelegate = std::move(StepDelegate);
				Aggregate->FinalizeDelegate = std::move(FinalizeDelegate);
				Base->SetAggregateFunction(Name, Args, Aggregate);
			}
			void LDBClusterSetWindowFunction(Network::LDB::Cluster* Base, const std::string_view& Name, uint8_t Args, asIScriptFunction* StepCallback, asIScriptFunction* InverseCallback, asIScriptFunction* ValueCallback, asIScriptFunction* FinalizeCallback)
			{
				FunctionDelegate StepDelegate(StepCallback);
				FunctionDelegate InverseDelegate(InverseCallback);
				FunctionDelegate ValueDelegate(ValueCallback);
				FunctionDelegate FinalizeDelegate(FinalizeCallback);
				if (!StepDelegate.IsValid() || !InverseDelegate.IsValid() || !ValueDelegate.IsValid() || !FinalizeDelegate.IsValid())
					return;

				LDBWindow* Window = new LDBWindow();
				Window->StepDelegate = std::move(StepDelegate);
				Window->InverseDelegate = std::move(InverseDelegate);
				Window->ValueDelegate = std::move(ValueDelegate);
				Window->FinalizeDelegate = std::move(FinalizeDelegate);
				Base->SetWindowFunction(Name, Args, Window);
			}
			Core::Promise<Network::LDB::SessionId> LDBClusterTxBegin(Network::LDB::Cluster* Base, Network::LDB::Isolation Level)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->TxBegin(Level).Then<Network::LDB::SessionId>([Context](Network::LDB::ExpectsDB<Network::LDB::SessionId>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), (Network::LDB::SessionId)nullptr, Context);
				});
			}
			Core::Promise<Network::LDB::SessionId> LDBClusterTxStart(Network::LDB::Cluster* Base, const std::string_view& Command)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->TxStart(Command).Then<Network::LDB::SessionId>([Context](Network::LDB::ExpectsDB<Network::LDB::SessionId>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), (Network::LDB::SessionId)nullptr, Context);
				});
			}
			Core::Promise<bool> LDBClusterTxEnd(Network::LDB::Cluster* Base, const std::string_view& Command, Network::LDB::SessionId Session)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->TxEnd(Command, Session).Then<bool>([Context](Network::LDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> LDBClusterTxCommit(Network::LDB::Cluster* Base, Network::LDB::SessionId Session)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->TxCommit(Session).Then<bool>([Context](Network::LDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> LDBClusterTxRollback(Network::LDB::Cluster* Base, Network::LDB::SessionId Session)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->TxRollback(Session).Then<bool>([Context](Network::LDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> LDBClusterConnect(Network::LDB::Cluster* Base, const std::string_view& Address, size_t Connections)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->Connect(Address, Connections).Then<bool>([Context](Network::LDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> LDBClusterDisconnect(Network::LDB::Cluster* Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->Disconnect().Then<bool>([Context](Network::LDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> LDBClusterFlush(Network::LDB::Cluster* Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->Flush().Then<bool>([Context](Network::LDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<Network::LDB::Cursor> LDBClusterQuery(Network::LDB::Cluster* Base, const std::string_view& Command, size_t Args, Network::LDB::SessionId Session)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->Query(Command, Args, Session).Then<Network::LDB::Cursor>([Context](Network::LDB::ExpectsDB<Network::LDB::Cursor>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::LDB::Cursor(nullptr), Context);
				});
			}
			Core::Promise<Network::LDB::Cursor> LDBClusterEmplaceQuery(Network::LDB::Cluster* Base, const std::string_view& Command, Array* Data, size_t Options, Network::LDB::SessionId Session)
			{
				Core::SchemaList Args;
				for (auto& Item : Array::Decompose<Core::Schema*>(Data))
				{
					Args.emplace_back(Item);
					Item->AddRef();
				}

				ImmediateContext* Context = ImmediateContext::Get();
				return Base->EmplaceQuery(Command, &Args, Options, Session).Then<Network::LDB::Cursor>([Context](Network::LDB::ExpectsDB<Network::LDB::Cursor>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::LDB::Cursor(nullptr), Context);
				});
			}
			Core::Promise<Network::LDB::Cursor> LDBClusterTemplateQuery(Network::LDB::Cluster* Base, const std::string_view& Command, Dictionary* Data, size_t Options, Network::LDB::SessionId Session)
			{
				Core::SchemaArgs Args;
				if (Data != nullptr)
				{
					VirtualMachine* VM = VirtualMachine::Get();
					if (VM != nullptr)
					{
						int TypeId = VM->GetTypeIdByDecl("schema@");
						Args.reserve(Data->Size());

						for (auto It = Data->Begin(); It != Data->End(); ++It)
						{
							Core::Schema* Value = nullptr;
							if (It.GetValue(&Value, TypeId))
							{
								Args[It.GetKey()] = Value;
								Value->AddRef();
							}
						}
					}
				}

				ImmediateContext* Context = ImmediateContext::Get();
				return Base->TemplateQuery(Command, &Args, Options, Session).Then<Network::LDB::Cursor>([Context](Network::LDB::ExpectsDB<Network::LDB::Cursor>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::LDB::Cursor(nullptr), Context);
				});
			}
			Array* LDBClusterWalCheckpoint(Network::LDB::Cluster* Base, Network::LDB::CheckpointMode Mode, const std::string_view& Database)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_LDBCHECKPOINT ">@");
				return Array::Compose(Type.GetTypeInfo(), Base->WalCheckpoint(Mode, Database));
			}

			Core::String LDBUtilsInlineQuery(Core::Schema* Where, Dictionary* WhitelistData, const std::string_view& Default)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				int TypeId = VM ? VM->GetTypeIdByDecl("string") : -1;
				Core::UnorderedMap<Core::String, Core::String> Whitelist = Dictionary::Decompose<Core::String>(TypeId, WhitelistData);
				return ExpectsWrapper::Unwrap(Network::LDB::Utils::InlineQuery(Where, Whitelist, Default), Core::String());
			}

			void LDBDriverSetQueryLog(Network::LDB::Driver* Base, asIScriptFunction* Callback)
			{
				FunctionDelegate Delegate(Callback);
				if (Delegate.IsValid())
				{
					Base->SetQueryLog([Delegate](const std::string_view& Data) mutable
					{
						Core::String Copy = Core::String(Data);
						Delegate([Copy](ImmediateContext* Context)
						{
							Context->SetArgObject(0, (void*)&Copy);
						});
					});
				}
				else
					Base->SetQueryLog(nullptr);
			}
			Core::String LDBDriverEmplace(Network::LDB::Driver* Base, const std::string_view& SQL, Array* Data)
			{
				Core::SchemaList Args;
				for (auto& Item : Array::Decompose<Core::Schema*>(Data))
				{
					Args.emplace_back(Item);
					Item->AddRef();
				}

				return ExpectsWrapper::Unwrap(Base->Emplace(SQL, &Args), Core::String());
			}
			Core::String LDBDriverGetQuery(Network::LDB::Driver* Base, const std::string_view& SQL, Dictionary* Data)
			{
				Core::SchemaArgs Args;
				if (Data != nullptr)
				{
					VirtualMachine* VM = VirtualMachine::Get();
					if (VM != nullptr)
					{
						int TypeId = VM->GetTypeIdByDecl("schema@");
						Args.reserve(Data->Size());

						for (auto It = Data->Begin(); It != Data->End(); ++It)
						{
							Core::Schema* Value = nullptr;
							if (It.GetValue(&Value, TypeId))
							{
								Args[It.GetKey()] = Value;
								Value->AddRef();
							}
						}
					}
				}

				return ExpectsWrapper::Unwrap(Base->GetQuery(SQL, &Args), Core::String());
			}
			Array* LDBDriverGetQueries(Network::LDB::Driver* Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose(Type.GetTypeInfo(), Base->GetQueries());
			}

			Network::PDB::Response& PDBResponseCopy(Network::PDB::Response& Base, Network::PDB::Response&& Other)
			{
				if (&Base == &Other)
					return Base;

				Base = Other.Copy();
				return Base;
			}
			Array* PDBResponseGetColumns(Network::PDB::Response& Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose(Type.GetTypeInfo(), Base.GetColumns());
			}

			Network::PDB::Response PDBCursorFirst(Network::PDB::Cursor& Base)
			{
				return Base.Size() > 0 ? Base.First().Copy() : Network::PDB::Response();
			}
			Network::PDB::Response PDBCursorLast(Network::PDB::Cursor& Base)
			{
				return Base.Size() > 0 ? Base.Last().Copy() : Network::PDB::Response();
			}
			Network::PDB::Response PDBCursorAt(Network::PDB::Cursor& Base, size_t Index)
			{
				return Index < Base.Size() ? Base.At(Index).Copy() : Network::PDB::Response();
			}
			Network::PDB::Cursor& PDBCursorCopy(Network::PDB::Cursor& Base, Network::PDB::Cursor&& Other)
			{
				if (&Base == &Other)
					return Base;

				Base = Other.Copy();
				return Base;
			}

			void PDBClusterSetWhenReconnected(Network::PDB::Cluster* Base, asIScriptFunction* Callback)
			{
				FunctionDelegate Delegate(Callback);
				if (Delegate.IsValid())
				{
					Base->SetWhenReconnected([Base, Delegate](const Core::Vector<Core::String>& Data) mutable -> Core::Promise<bool>
					{
						Core::Promise<bool> Future;
						Delegate([Base, Data](ImmediateContext* Context)
						{
							TypeInfo Type = Context->GetVM()->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
							Context->SetArgObject(0, Base);
							Context->SetArgObject(1, Array::Compose(Type.GetTypeInfo(), Data));
						}, [Future](ImmediateContext* Context) mutable
						{
							Promise* Target = Context->GetReturnObject<Promise>();
							if (!Target)
								return Future.Set(true);

							Target->When([Future](Promise* Target) mutable
							{
								bool Value = true;
								Target->Retrieve(&Value, (int)TypeId::BOOL);
								Future.Set(Value);
							});
						});
						return Future;
					});
				}
				else
					Base->SetWhenReconnected(nullptr);
			}
			uint64_t PDBClusterAddChannel(Network::PDB::Cluster* Base, const std::string_view& Name, asIScriptFunction* Callback)
			{
				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
					return 0;

				return Base->AddChannel(Name, [Base, Delegate](const Network::PDB::Notify& Event) mutable
				{
					Delegate([Base, &Event](ImmediateContext* Context)
					{
						Context->SetArgObject(0, Base);
						Context->SetArgObject(1, (void*)&Event);
					});
				});
			}
			Core::Promise<bool> PDBClusterListen(Network::PDB::Cluster* Base, Array* Data)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				Core::Vector<Core::String> Names = Array::Decompose<Core::String>(Data);
				return Base->Listen(Names).Then<bool>([Context](Network::PDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> PDBClusterUnlisten(Network::PDB::Cluster* Base, Array* Data)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				Core::Vector<Core::String> Names = Array::Decompose<Core::String>(Data);
				return Base->Unlisten(Names).Then<bool>([Context](Network::PDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<Network::PDB::SessionId> PDBClusterTxBegin(Network::PDB::Cluster* Base, Network::PDB::Isolation Level)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->TxBegin(Level).Then<Network::PDB::SessionId>([Context](Network::PDB::ExpectsDB<Network::PDB::SessionId>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), (Network::PDB::SessionId)nullptr, Context);
				});
			}
			Core::Promise<Network::PDB::SessionId> PDBClusterTxStart(Network::PDB::Cluster* Base, const std::string_view& Command)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->TxStart(Command).Then<Network::PDB::SessionId>([Context](Network::PDB::ExpectsDB<Network::PDB::SessionId>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), (Network::PDB::SessionId)nullptr, Context);
				});
			}
			Core::Promise<bool> PDBClusterTxEnd(Network::PDB::Cluster* Base, const std::string_view& Command, Network::PDB::SessionId Session)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->TxEnd(Command, Session).Then<bool>([Context](Network::PDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> PDBClusterTxCommit(Network::PDB::Cluster* Base, Network::PDB::SessionId Session)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->TxCommit(Session).Then<bool>([Context](Network::PDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> PDBClusterTxRollback(Network::PDB::Cluster* Base, Network::PDB::SessionId Session)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->TxRollback(Session).Then<bool>([Context](Network::PDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> PDBClusterConnect(Network::PDB::Cluster* Base, const Network::PDB::Address& Address, size_t Connections)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->Connect(Address, Connections).Then<bool>([Context](Network::PDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> PDBClusterDisconnect(Network::PDB::Cluster* Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->Disconnect().Then<bool>([Context](Network::PDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<Network::PDB::Cursor> PDBClusterQuery(Network::PDB::Cluster* Base, const std::string_view& Command, size_t Args, Network::PDB::SessionId Session)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->Query(Command, Args, Session).Then<Network::PDB::Cursor>([Context](Network::PDB::ExpectsDB<Network::PDB::Cursor>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::PDB::Cursor(), Context);
				});
			}
			Core::Promise<Network::PDB::Cursor> PDBClusterEmplaceQuery(Network::PDB::Cluster* Base, const std::string_view& Command, Array* Data, size_t Options, Network::PDB::Connection* Session)
			{
				Core::SchemaList Args;
				for (auto& Item : Array::Decompose<Core::Schema*>(Data))
				{
					Args.emplace_back(Item);
					Item->AddRef();
				}

				ImmediateContext* Context = ImmediateContext::Get();
				return Base->EmplaceQuery(Command, &Args, Options, Session).Then<Network::PDB::Cursor>([Context](Network::PDB::ExpectsDB<Network::PDB::Cursor>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::PDB::Cursor(), Context);
				});
			}
			Core::Promise<Network::PDB::Cursor> PDBClusterTemplateQuery(Network::PDB::Cluster* Base, const std::string_view& Command, Dictionary* Data, size_t Options, Network::PDB::Connection* Session)
			{
				Core::SchemaArgs Args;
				if (Data != nullptr)
				{
					VirtualMachine* VM = VirtualMachine::Get();
					if (VM != nullptr)
					{
						int TypeId = VM->GetTypeIdByDecl("schema@");
						Args.reserve(Data->Size());

						for (auto It = Data->Begin(); It != Data->End(); ++It)
						{
							Core::Schema* Value = nullptr;
							if (It.GetValue(&Value, TypeId))
							{
								Args[It.GetKey()] = Value;
								Value->AddRef();
							}
						}
					}
				}

				ImmediateContext* Context = ImmediateContext::Get();
				return Base->TemplateQuery(Command, &Args, Options, Session).Then<Network::PDB::Cursor>([Context](Network::PDB::ExpectsDB<Network::PDB::Cursor>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::PDB::Cursor(), Context);
				});
			}

			Core::String PDBUtilsInlineQuery(Network::PDB::Cluster* Client, Core::Schema* Where, Dictionary* WhitelistData, const std::string_view& Default)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				int TypeId = VM ? VM->GetTypeIdByDecl("string") : -1;
				Core::UnorderedMap<Core::String, Core::String> Whitelist = Dictionary::Decompose<Core::String>(TypeId, WhitelistData);
				return ExpectsWrapper::Unwrap(Network::PDB::Utils::InlineQuery(Client, Where, Whitelist, Default), Core::String());
			}

			void PDBDriverSetQueryLog(Network::PDB::Driver* Base, asIScriptFunction* Callback)
			{
				FunctionDelegate Delegate(Callback);
				if (Delegate.IsValid())
				{
					Base->SetQueryLog([Delegate](const std::string_view& Data) mutable
					{
						Core::String Copy = Core::String(Data);
						Delegate([Copy](ImmediateContext* Context)
						{
							Context->SetArgObject(0, (void*)&Copy);
						});
					});
				}
				else
					Base->SetQueryLog(nullptr);
			}
			Core::String PDBDriverEmplace(Network::PDB::Driver* Base, Network::PDB::Cluster* Cluster, const std::string_view& SQL, Array* Data)
			{
				Core::SchemaList Args;
				for (auto& Item : Array::Decompose<Core::Schema*>(Data))
				{
					Args.emplace_back(Item);
					Item->AddRef();
				}

				return ExpectsWrapper::Unwrap(Base->Emplace(Cluster, SQL, &Args), Core::String());
			}
			Core::String PDBDriverGetQuery(Network::PDB::Driver* Base, Network::PDB::Cluster* Cluster, const std::string_view& SQL, Dictionary* Data)
			{
				Core::SchemaArgs Args;
				if (Data != nullptr)
				{
					VirtualMachine* VM = VirtualMachine::Get();
					if (VM != nullptr)
					{
						int TypeId = VM->GetTypeIdByDecl("schema@");
						Args.reserve(Data->Size());

						for (auto It = Data->Begin(); It != Data->End(); ++It)
						{
							Core::Schema* Value = nullptr;
							if (It.GetValue(&Value, TypeId))
							{
								Args[It.GetKey()] = Value;
								Value->AddRef();
							}
						}
					}
				}

				return ExpectsWrapper::Unwrap(Base->GetQuery(Cluster, SQL, &Args), Core::String());
			}
			Array* PDBDriverGetQueries(Network::PDB::Driver* Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose(Type.GetTypeInfo(), Base->GetQueries());
			}

			Network::MDB::Document MDBDocumentConstructBuffer(unsigned char* Buffer)
			{
#ifdef VI_ANGELSCRIPT
				if (!Buffer)
					return Network::MDB::Document::FromEmpty();

				Network::MDB::Document Result = Network::MDB::Document::FromEmpty();
				VirtualMachine* VM = VirtualMachine::Get();
				size_t Length = *(asUINT*)Buffer;
				Buffer += 4;

				while (Length--)
				{
					if (asPWORD(Buffer) & 0x3)
						Buffer += 4 - (asPWORD(Buffer) & 0x3);

					Core::String Name = *(Core::String*)Buffer;
					Buffer += sizeof(Core::String);

					int TypeId = *(int*)Buffer;
					Buffer += sizeof(int);

					void* Ref = (void*)Buffer;
					if (TypeId >= (size_t)TypeId::BOOL && TypeId <= (size_t)TypeId::DOUBLE)
					{
						switch (TypeId)
						{
							case (size_t)TypeId::BOOL:
								Result.SetBoolean(Name, *(bool*)Ref);
								break;
							case (size_t)TypeId::INT8:
								Result.SetInteger(Name, *(char*)Ref);
								break;
							case (size_t)TypeId::INT16:
								Result.SetInteger(Name, *(short*)Ref);
								break;
							case (size_t)TypeId::INT32:
								Result.SetInteger(Name, *(int*)Ref);
								break;
							case (size_t)TypeId::UINT8:
								Result.SetInteger(Name, *(unsigned char*)Ref);
								break;
							case (size_t)TypeId::UINT16:
								Result.SetInteger(Name, *(unsigned short*)Ref);
								break;
							case (size_t)TypeId::UINT32:
								Result.SetInteger(Name, *(uint32_t*)Ref);
								break;
							case (size_t)TypeId::INT64:
							case (size_t)TypeId::UINT64:
								Result.SetInteger(Name, *(int64_t*)Ref);
								break;
							case (size_t)TypeId::FLOAT:
								Result.SetNumber(Name, *(float*)Ref);
								break;
							case (size_t)TypeId::DOUBLE:
								Result.SetNumber(Name, *(double*)Ref);
								break;
						}
					}
					else
					{
						auto Type = VM->GetTypeInfoById(TypeId);
						if ((TypeId & (size_t)TypeId::MASK_OBJECT) && !(TypeId & (size_t)TypeId::OBJHANDLE) && (Type.IsValid() && Type.Flags() & (size_t)ObjectBehaviours::REF))
							Ref = *(void**)Ref;

						if (TypeId & (size_t)TypeId::OBJHANDLE)
							Ref = *(void**)Ref;

						if (VM->IsNullable(TypeId) || !Ref)
						{
							Result.SetNull(Name);
						}
						else if (Type.IsValid() && Type.GetName() == TYPENAME_SCHEMA)
						{
							Core::Schema* Base = (Core::Schema*)Ref;
							Result.SetSchema(Name, Network::MDB::Document::FromSchema(Base));
						}
						else if (Type.IsValid() && Type.GetName() == TYPENAME_STRING)
							Result.SetString(Name, *(Core::String*)Ref);
						else if (Type.IsValid() && Type.GetName() == TYPENAME_DECIMAL)
							Result.SetDecimalString(Name, ((Core::Decimal*)Ref)->ToString());
					}

					if (TypeId & (size_t)TypeId::MASK_OBJECT)
					{
						auto Type = VM->GetTypeInfoById(TypeId);
						if (Type.Flags() & (size_t)ObjectBehaviours::VALUE)
							Buffer += Type.Size();
						else
							Buffer += sizeof(void*);
					}
					else if (TypeId != 0)
						Buffer += VM->GetSizeOfPrimitiveType(TypeId).Or(0);
					else
						Buffer += sizeof(void*);
				}

				return Result;
#else
				return nullptr;
#endif
			}
			void MDBDocumentConstruct(asIScriptGeneric* Generic)
			{
				GenericContext Args = Generic;
				unsigned char* Buffer = (unsigned char*)Args.GetArgAddress(0);
				*(Network::MDB::Document*)Args.GetAddressOfReturnLocation() = MDBDocumentConstructBuffer(Buffer);
			}

			bool MDBStreamTemplateQuery(Network::MDB::Stream& Base, const std::string_view& Command, Dictionary* Data)
			{
				Core::SchemaArgs Args;
				if (Data != nullptr)
				{
					VirtualMachine* VM = VirtualMachine::Get();
					if (VM != nullptr)
					{
						int TypeId = VM->GetTypeIdByDecl("schema@");
						Args.reserve(Data->Size());

						for (auto It = Data->Begin(); It != Data->End(); ++It)
						{
							Core::Schema* Value = nullptr;
							if (It.GetValue(&Value, TypeId))
							{
								Args[It.GetKey()] = Value;
								Value->AddRef();
							}
						}
					}
				}

				return ExpectsWrapper::UnwrapVoid(Base.TemplateQuery(Command, &Args));
			}
			Core::Promise<Network::MDB::Document> MDBStreamExecuteWithReply(Network::MDB::Stream& Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.ExecuteWithReply().Then<Network::MDB::Document>([Context](Network::MDB::ExpectsDB<Network::MDB::Document>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Document(nullptr), Context);
				});
			}
			Core::Promise<bool> MDBStreamExecute(Network::MDB::Stream& Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.Execute().Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}

			Core::String MDBCursorError(Network::MDB::Cursor& Base)
			{
				auto Error = Base.Error();
				return Error ? Error->message() : Core::String();
			}
			Core::Promise<bool> MDBCursorNext(Network::MDB::Cursor& Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.Next().Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}

			Core::Promise<Core::Schema*> MDBResponseFetch(Network::MDB::Response& Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.Fetch().Then<Core::Schema*>([Context](Network::MDB::ExpectsDB<Core::Schema*>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), (Core::Schema*)nullptr, Context);
				});
			}
			Core::Promise<Core::Schema*> MDBResponseFetchAll(Network::MDB::Response& Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.FetchAll().Then<Core::Schema*>([Context](Network::MDB::ExpectsDB<Core::Schema*>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), (Core::Schema*)nullptr, Context);
				});
			}
			Core::Promise<Network::MDB::Property> MDBResponseGetProperty(Network::MDB::Response& Base, const std::string_view& Name)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.GetProperty(Name).Then<Network::MDB::Property>([Context](Network::MDB::ExpectsDB<Network::MDB::Property>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Property(), Context);
				});
			}

			Core::Promise<bool> MDBTransactionBegin(Network::MDB::Transaction& Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.Begin().Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> MDBTransactionRollback(Network::MDB::Transaction& Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.Rollback().Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<Network::MDB::Document> MDBTransactionRemoveMany(Network::MDB::Transaction& Base, Network::MDB::Collection& Collection, const Network::MDB::Document& Match, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.RemoveMany(Collection, Match, Options).Then<Network::MDB::Document>([Context](Network::MDB::ExpectsDB<Network::MDB::Document>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Document(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Document> MDBTransactionRemoveOne(Network::MDB::Transaction& Base, Network::MDB::Collection& Collection, const Network::MDB::Document& Match, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.RemoveOne(Collection, Match, Options).Then<Network::MDB::Document>([Context](Network::MDB::ExpectsDB<Network::MDB::Document>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Document(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Document> MDBTransactionReplaceOne(Network::MDB::Transaction& Base, Network::MDB::Collection& Collection, const Network::MDB::Document& Match, const Network::MDB::Document& Replacement, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.ReplaceOne(Collection, Match, Replacement, Options).Then<Network::MDB::Document>([Context](Network::MDB::ExpectsDB<Network::MDB::Document>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Document(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Document> MDBTransactionInsertMany(Network::MDB::Transaction& Base, Network::MDB::Collection& Collection, Array* Data, const Network::MDB::Document& Options)
			{
				Core::Vector<Network::MDB::Document> List;
				if (Data != nullptr)
				{
					size_t Size = Data->Size();
					List.reserve(Size);
					for (size_t i = 0; i < Size; i++)
						List.emplace_back(((Network::MDB::Document*)Data->At(i))->Copy());
				}

				ImmediateContext* Context = ImmediateContext::Get();
				return Base.InsertMany(Collection, List, Options).Then<Network::MDB::Document>([Context](Network::MDB::ExpectsDB<Network::MDB::Document>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Document(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Document> MDBTransactionInsertOne(Network::MDB::Transaction& Base, Network::MDB::Collection& Collection, const Network::MDB::Document& Result, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.InsertOne(Collection, Result, Options).Then<Network::MDB::Document>([Context](Network::MDB::ExpectsDB<Network::MDB::Document>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Document(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Document> MDBTransactionUpdateMany(Network::MDB::Transaction& Base, Network::MDB::Collection& Collection, const Network::MDB::Document& Match, const Network::MDB::Document& Update, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.UpdateMany(Collection, Match, Update, Options).Then<Network::MDB::Document>([Context](Network::MDB::ExpectsDB<Network::MDB::Document>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Document(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Document> MDBTransactionUpdateOne(Network::MDB::Transaction& Base, Network::MDB::Collection& Collection, const Network::MDB::Document& Match, const Network::MDB::Document& Update, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.UpdateOne(Collection, Match, Update, Options).Then<Network::MDB::Document>([Context](Network::MDB::ExpectsDB<Network::MDB::Document>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Document(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Cursor> MDBTransactionFindMany(Network::MDB::Transaction& Base, Network::MDB::Collection& Collection, const Network::MDB::Document& Match, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.FindMany(Collection, Match, Options).Then<Network::MDB::Cursor>([Context](Network::MDB::ExpectsDB<Network::MDB::Cursor>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Cursor(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Cursor> MDBTransactionFindOne(Network::MDB::Transaction& Base, Network::MDB::Collection& Collection, const Network::MDB::Document& Match, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.FindOne(Collection, Match, Options).Then<Network::MDB::Cursor>([Context](Network::MDB::ExpectsDB<Network::MDB::Cursor>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Cursor(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Cursor> MDBTransactionAggregate(Network::MDB::Transaction& Base, Network::MDB::Collection& Collection, Network::MDB::QueryFlags Flags, const Network::MDB::Document& Pipeline, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.Aggregate(Collection, Flags, Pipeline, Options).Then<Network::MDB::Cursor>([Context](Network::MDB::ExpectsDB<Network::MDB::Cursor>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Cursor(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Response> MDBTransactionTemplateQuery(Network::MDB::Transaction& Base, Network::MDB::Collection& Collection, const std::string_view& Name, Dictionary* Data)
			{
				Core::SchemaArgs Args;
				if (Data != nullptr)
				{
					VirtualMachine* VM = VirtualMachine::Get();
					if (VM != nullptr)
					{
						int TypeId = VM->GetTypeIdByDecl("schema@");
						Args.reserve(Data->Size());

						for (auto It = Data->Begin(); It != Data->End(); ++It)
						{
							Core::Schema* Value = nullptr;
							if (It.GetValue(&Value, TypeId))
							{
								Args[It.GetKey()] = Value;
								Value->AddRef();
							}
						}
					}
				}

				ImmediateContext* Context = ImmediateContext::Get();
				return Base.TemplateQuery(Collection, Name, &Args).Then<Network::MDB::Response>([Context](Network::MDB::ExpectsDB<Network::MDB::Response>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Response(), Context);
				});
			}
			Core::Promise<Network::MDB::Response> MDBTransactionQuery(Network::MDB::Transaction& Base, Network::MDB::Collection& Collection, const Network::MDB::Document& Command)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.Query(Collection, Command).Then<Network::MDB::Response>([Context](Network::MDB::ExpectsDB<Network::MDB::Response>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Response(), Context);
				});
			}
			Core::Promise<Network::MDB::TransactionState> MDBTransactionCommit(Network::MDB::Transaction& Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.Commit().Then<Network::MDB::TransactionState>([Context](Network::MDB::ExpectsDB<Network::MDB::TransactionState>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::TransactionState::Fatal, Context);
				});
			}

			Core::Promise<bool> MDBCollectionRename(Network::MDB::Collection& Base, const std::string_view& DatabaseName, const std::string_view& CollectionName, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.Rename(DatabaseName, CollectionName).Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> MDBCollectionRenameWithOptions(Network::MDB::Collection& Base, const std::string_view& DatabaseName, const std::string_view& CollectionName, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.RenameWithOptions(DatabaseName, CollectionName, Options).Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> MDBCollectionRenameWithRemove(Network::MDB::Collection& Base, const std::string_view& DatabaseName, const std::string_view& CollectionName, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.RenameWithRemove(DatabaseName, CollectionName).Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> MDBCollectionRenameWithOptionsAndRemove(Network::MDB::Collection& Base, const std::string_view& DatabaseName, const std::string_view& CollectionName, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.RenameWithOptionsAndRemove(DatabaseName, CollectionName, Options).Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> MDBCollectionRemove(Network::MDB::Collection& Base, const std::string_view& Name, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.Remove(Options).Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> MDBCollectionRemoveIndex(Network::MDB::Collection& Base, const std::string_view& Name, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.RemoveIndex(Name, Options).Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<Network::MDB::Document> MDBCollectionRemoveMany(Network::MDB::Collection& Base, const Network::MDB::Document& Match, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.RemoveMany(Match, Options).Then<Network::MDB::Document>([Context](Network::MDB::ExpectsDB<Network::MDB::Document>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Document(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Document> MDBCollectionRemoveOne(Network::MDB::Collection& Base, const Network::MDB::Document& Match, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.RemoveOne(Match, Options).Then<Network::MDB::Document>([Context](Network::MDB::ExpectsDB<Network::MDB::Document>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Document(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Document> MDBCollectionReplaceOne(Network::MDB::Collection& Base, const Network::MDB::Document& Match, const Network::MDB::Document& Replacement, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.ReplaceOne(Match, Replacement, Options).Then<Network::MDB::Document>([Context](Network::MDB::ExpectsDB<Network::MDB::Document>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Document(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Document> MDBCollectionInsertMany(Network::MDB::Collection& Base, Array* Data, const Network::MDB::Document& Options)
			{
				Core::Vector<Network::MDB::Document> List;
				if (Data != nullptr)
				{
					size_t Size = Data->Size();
					List.reserve(Size);
					for (size_t i = 0; i < Size; i++)
						List.emplace_back(((Network::MDB::Document*)Data->At(i))->Copy());
				}

				ImmediateContext* Context = ImmediateContext::Get();
				return Base.InsertMany(List, Options).Then<Network::MDB::Document>([Context](Network::MDB::ExpectsDB<Network::MDB::Document>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Document(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Document> MDBCollectionInsertOne(Network::MDB::Collection& Base, const Network::MDB::Document& Result, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.InsertOne(Result, Options).Then<Network::MDB::Document>([Context](Network::MDB::ExpectsDB<Network::MDB::Document>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Document(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Document> MDBCollectionUpdateMany(Network::MDB::Collection& Base, const Network::MDB::Document& Match, const Network::MDB::Document& Update, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.UpdateMany(Match, Update, Options).Then<Network::MDB::Document>([Context](Network::MDB::ExpectsDB<Network::MDB::Document>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Document(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Document> MDBCollectionUpdateOne(Network::MDB::Collection& Base, const Network::MDB::Document& Match, const Network::MDB::Document& Update, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.UpdateOne(Match, Update, Options).Then<Network::MDB::Document>([Context](Network::MDB::ExpectsDB<Network::MDB::Document>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Document(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Document> MDBCollectionFindAndModify(Network::MDB::Collection& Base, const Network::MDB::Document& Match, const Network::MDB::Document& Sort, const Network::MDB::Document& Update, const Network::MDB::Document& Fields, bool Remove, bool Upsert, bool New)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.FindAndModify(Match, Sort, Update, Fields, Remove, Upsert, New).Then<Network::MDB::Document>([Context](Network::MDB::ExpectsDB<Network::MDB::Document>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Document(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Cursor> MDBCollectionFindMany(Network::MDB::Collection& Base, const Network::MDB::Document& Match, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.FindMany(Match, Options).Then<Network::MDB::Cursor>([Context](Network::MDB::ExpectsDB<Network::MDB::Cursor>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Cursor(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Cursor> MDBCollectionFindOne(Network::MDB::Collection& Base, const Network::MDB::Document& Match, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.FindOne(Match, Options).Then<Network::MDB::Cursor>([Context](Network::MDB::ExpectsDB<Network::MDB::Cursor>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Cursor(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Cursor> MDBCollectionFindIndexes(Network::MDB::Collection& Base, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.FindIndexes(Options).Then<Network::MDB::Cursor>([Context](Network::MDB::ExpectsDB<Network::MDB::Cursor>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Cursor(), Context);
				});
			}
			Core::Promise<Network::MDB::Cursor> MDBCollectionAggregate(Network::MDB::Collection& Base, Network::MDB::QueryFlags Flags, const Network::MDB::Document& Pipeline, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.Aggregate(Flags, Pipeline, Options).Then<Network::MDB::Cursor>([Context](Network::MDB::ExpectsDB<Network::MDB::Cursor>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Cursor(), Context);
				});
			}
			Core::Promise<Network::MDB::Response> MDBCollectionTemplateQuery(Network::MDB::Collection& Base, const std::string_view& Name, Dictionary* Data, Network::MDB::Transaction& Session)
			{
				Core::SchemaArgs Args;
				if (Data != nullptr)
				{
					VirtualMachine* VM = VirtualMachine::Get();
					if (VM != nullptr)
					{
						int TypeId = VM->GetTypeIdByDecl("schema@");
						Args.reserve(Data->Size());

						for (auto It = Data->Begin(); It != Data->End(); ++It)
						{
							Core::Schema* Value = nullptr;
							if (It.GetValue(&Value, TypeId))
							{
								Args[It.GetKey()] = Value;
								Value->AddRef();
							}
						}
					}
				}

				ImmediateContext* Context = ImmediateContext::Get();
				return Base.TemplateQuery(Name, &Args, Session).Then<Network::MDB::Response>([Context](Network::MDB::ExpectsDB<Network::MDB::Response>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Response(), Context);
				});
			}
			Core::Promise<Network::MDB::Response> MDBCollectionQuery(Network::MDB::Collection& Base, const Network::MDB::Document& Command, Network::MDB::Transaction& Session)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.Query(Command, Session).Then<Network::MDB::Response>([Context](Network::MDB::ExpectsDB<Network::MDB::Response>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Response(), Context);
				});
			}

			Core::Promise<bool> MDBDatabaseRemoveAllUsers(Network::MDB::Database& Base, const std::string_view& Name)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.RemoveAllUsers().Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> MDBDatabaseRemoveUser(Network::MDB::Database& Base, const std::string_view& Name)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.RemoveUser(Name).Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> MDBDatabaseRemove(Network::MDB::Database& Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.Remove().Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> MDBDatabaseRemoveWithOptions(Network::MDB::Database& Base, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.RemoveWithOptions(Options).Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> MDBDatabaseAddUser(Network::MDB::Database& Base, const std::string_view& Username, const std::string_view& Password, const Network::MDB::Document& Roles, const Network::MDB::Document& Custom)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.AddUser(Username, Password, Roles, Custom).Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> MDBDatabaseHasCollection(Network::MDB::Database& Base, const std::string_view& Name)
			{
				return Base.HasCollection(Name).Then<bool>([](Network::MDB::ExpectsDB<void>&& Result) { return !!Result; });
			}
			Core::Promise<Network::MDB::Collection> MDBDatabaseCreateCollection(Network::MDB::Database& Base, const std::string_view& Name, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.CreateCollection(Name, Options).Then<Network::MDB::Collection>([Context](Network::MDB::ExpectsDB<Network::MDB::Collection>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Collection(nullptr), Context);
				});
			}
			Core::Promise<Network::MDB::Cursor> MDBDatabaseFindCollections(Network::MDB::Database& Base, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.FindCollections(Options).Then<Network::MDB::Cursor>([Context](Network::MDB::ExpectsDB<Network::MDB::Cursor>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Cursor(nullptr), Context);
				});
			}
			Array* MDBDatabaseGetCollectionNames(Network::MDB::Database& Base, const Network::MDB::Document& Options)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose<Core::String>(Type, ExpectsWrapper::Unwrap(Base.GetCollectionNames(Options), Core::Vector<Core::String>()));
			}

			Core::Promise<bool> MDBWatcherNext(Network::MDB::Watcher& Base, Network::MDB::Document& Result)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.Next(Result).Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> MDBWatcherError(Network::MDB::Watcher& Base, Network::MDB::Document& Result)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base.Error(Result).Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}

			Array* MDBConnectionGetDatabaseNames(Network::MDB::Connection* Base, const Network::MDB::Document& Options)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose<Core::String>(Type, ExpectsWrapper::Unwrap(Base->GetDatabaseNames(Options), Core::Vector<Core::String>()));
			}
			Core::Promise<bool> MDBConnectionConnect(Network::MDB::Connection* Base, Network::MDB::Address* Address)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->Connect(Address).Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> MDBConnectionDisconnect(Network::MDB::Connection* Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->Disconnect().Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> MDBConnectionMakeTransaction(Network::MDB::Connection* Base, asIScriptFunction* Callback)
			{
				FunctionDelegate Delegate(Callback);
				if (!Delegate.IsValid())
					return Core::Promise<bool>(false);

				ImmediateContext* Context = ImmediateContext::Get();
				return Base->MakeTransaction([Base, Delegate](Network::MDB::Transaction* Session) mutable -> Core::Promise<bool>
				{
					Core::Promise<bool> Future;
					Delegate([Base, Session](ImmediateContext* Context)
					{
						Context->SetArgObject(0, (void*)&Base);
						Context->SetArgObject(1, (void*)Session);
					}, [Future](ImmediateContext* Context) mutable
					{
						Promise* Target = Context->GetReturnObject<Promise>();
						if (!Target)
							return Future.Set(true);

						Target->When([Future](Promise* Target) mutable
						{
							bool Value = true;
							Target->Retrieve(&Value, (int)TypeId::BOOL);
							Future.Set(Value);
						});
					});
					return Future;
				}).Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<Network::MDB::Cursor> MDBConnectionFindDatabases(Network::MDB::Connection* Base, const Network::MDB::Document& Options)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->FindDatabases(Options).Then<Network::MDB::Cursor>([Context](Network::MDB::ExpectsDB<Network::MDB::Cursor>&& Result)
				{
					return ExpectsWrapper::Unwrap(std::move(Result), Network::MDB::Cursor(nullptr), Context);
				});
			}

			void MDBClusterPush(Network::MDB::Cluster* Base, Network::MDB::Connection* Target)
			{
				Base->Push(&Target);
			}
			Core::Promise<bool> MDBClusterConnect(Network::MDB::Cluster* Base, Network::MDB::Address* Address)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->Connect(Address).Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}
			Core::Promise<bool> MDBClusterDisconnect(Network::MDB::Cluster* Base)
			{
				ImmediateContext* Context = ImmediateContext::Get();
				return Base->Disconnect().Then<bool>([Context](Network::MDB::ExpectsDB<void>&& Result)
				{
					return ExpectsWrapper::UnwrapVoid(std::move(Result), Context);
				});
			}

			void MDBDriverSetQueryLog(Network::MDB::Driver* Base, asIScriptFunction* Callback)
			{
				FunctionDelegate Delegate(Callback);
				if (Delegate.IsValid())
				{
					Base->SetQueryLog([Delegate](const std::string_view& Data) mutable
					{
						Core::String Copy = Core::String(Data);
						Delegate([Copy](ImmediateContext* Context)
						{
							Context->SetArgObject(0, (void*)&Copy);
						});
					});
				}
				else
					Base->SetQueryLog(nullptr);
			}
			Core::String MDBDriverGetQuery(Network::PDB::Driver* Base, Network::PDB::Cluster* Cluster, const std::string_view& SQL, Dictionary* Data)
			{
				Core::SchemaArgs Args;
				if (Data != nullptr)
				{
					VirtualMachine* VM = VirtualMachine::Get();
					if (VM != nullptr)
					{
						int TypeId = VM->GetTypeIdByDecl("schema@");
						Args.reserve(Data->Size());

						for (auto It = Data->Begin(); It != Data->End(); ++It)
						{
							Core::Schema* Value = nullptr;
							if (It.GetValue(&Value, TypeId))
							{
								Args[It.GetKey()] = Value;
								Value->AddRef();
							}
						}
					}
				}

				return ExpectsWrapper::Unwrap(Base->GetQuery(Cluster, SQL, &Args), Core::String());
			}
			Array* MDBDriverGetQueries(Network::MDB::Driver* Base)
			{
				VirtualMachine* VM = VirtualMachine::Get();
				if (!VM)
					return nullptr;

				TypeInfo Type = VM->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose(Type.GetTypeInfo(), Base->GetQueries());
			}
#endif
			bool Registry::Cleanup() noexcept
			{
#ifdef VI_ANGELSCRIPT
				StringFactory::Free();
#endif
				return true;
			}
			bool Registry::BindAddons(VirtualMachine* VM) noexcept
			{
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, "manager should be set");
				VM->AddSystemAddon("ctypes", { }, &ImportCTypes);
				VM->AddSystemAddon("any", { }, &ImportAny);
				VM->AddSystemAddon("array", { "ctypes" }, &ImportArray);
				VM->AddSystemAddon("complex", { }, &ImportComplex);
				VM->AddSystemAddon("math", { }, &ImportMath);
				VM->AddSystemAddon("string", { "array" }, &ImportString);
				VM->AddSystemAddon("random", { "string" }, &ImportRandom);
				VM->AddSystemAddon("dictionary", { "array", "string" }, &ImportDictionary);
				VM->AddSystemAddon("exception", { "string" }, &ImportException);
				VM->AddSystemAddon("mutex", { }, &ImportMutex);
				VM->AddSystemAddon("thread", { "any", "string" }, &ImportThread);
				VM->AddSystemAddon("buffers", { "string" }, &ImportBuffers);
				VM->AddSystemAddon("promise", { "exception" }, &ImportPromise);
				VM->AddSystemAddon("decimal", { "string" }, &ImportDecimal);
				VM->AddSystemAddon("uint128", { "decimal" }, &ImportUInt128);
				VM->AddSystemAddon("uint256", { "uint128" }, &ImportUInt256);
				VM->AddSystemAddon("variant", { "string", "decimal" }, &ImportVariant);
				VM->AddSystemAddon("timestamp", { "string" }, &ImportTimestamp);
				VM->AddSystemAddon("console", { "string" }, &ImportConsole);
				VM->AddSystemAddon("schema", { "array", "string", "dictionary", "variant" }, &ImportSchema);
				VM->AddSystemAddon("schedule", { "ctypes" }, &ImportSchedule);
				VM->AddSystemAddon("clock", { }, &ImportClockTimer);
				VM->AddSystemAddon("fs", { "string" }, &ImportFileSystem);
				VM->AddSystemAddon("os", { "fs", "array", "dictionary" }, &ImportOS);
				VM->AddSystemAddon("regex", { "string" }, &ImportRegex);
				VM->AddSystemAddon("crypto", { "string", "schema" }, &ImportCrypto);
				VM->AddSystemAddon("codec", { "string" }, &ImportCodec);
				VM->AddSystemAddon("preprocessor", { "string" }, &ImportPreprocessor);
				VM->AddSystemAddon("network", { "string", "array", "dictionary", "promise" }, &ImportNetwork);
				VM->AddSystemAddon("http", { "schema", "fs", "promise", "regex", "network" }, &ImportHTTP);
				VM->AddSystemAddon("smtp", { "promise", "network" }, &ImportSMTP);
				VM->AddSystemAddon("sqlite", { "network", "schema" }, &ImportSQLite);
				VM->AddSystemAddon("postgresql", { "network", "schema" }, &ImportPostgreSQL);
				VM->AddSystemAddon("mongodb", { "network", "schema" }, &ImportMongoDB);
				VM->AddSystemAddon("vm", { }, &ImportVM);
				VM->AddSystemAddon("layer", { "schema", "schedule", "fs", "clock" }, &ImportLayer);
				return true;
			}
			bool Registry::BindStringifiers(DebuggerContext* Context) noexcept
			{
				VI_ASSERT(Context != nullptr, "context should be set");
				Context->AddToStringCallback("string", [](Core::String& Indent, int Depth, void* Object, int TypeId)
				{
					Core::String& Source = *(Core::String*)Object;
					Core::StringStream Stream;
					Stream << "\"" << Source << "\"";
					Stream << " (string, " << Source.size() << " chars)";
					return Stream.str();
				});
				Context->AddToStringCallback("string_view", [](Core::String& Indent, int Depth, void* Object, int TypeId)
				{
					std::string_view& Source = *(std::string_view*)Object;
					Core::StringStream Stream;
					Stream << "\"" << Source << "\"";
					Stream << " (string_view, " << Source.size() << " chars)";
					return Stream.str();
				});
				Context->AddToStringCallback("decimal", [](Core::String& Indent, int Depth, void* Object, int TypeId)
				{
					Core::Decimal& Source = *(Core::Decimal*)Object;
					return Source.ToString() + " (decimal)";
				});
				Context->AddToStringCallback("uint128", [](Core::String& Indent, int Depth, void* Object, int TypeId)
				{
					Compute::UInt128& Source = *(Compute::UInt128*)Object;
					return Source.ToString() + " (uint128)";
				});
				Context->AddToStringCallback("uint256", [](Core::String& Indent, int Depth, void* Object, int TypeId)
				{
					Compute::UInt256& Source = *(Compute::UInt256*)Object;
					return Source.ToString() + " (uint256)";
				});
				Context->AddToStringCallback("variant", [](Core::String& Indent, int Depth, void* Object, int TypeId)
				{
					Core::Variant& Source = *(Core::Variant*)Object;
					return "\"" + Source.Serialize() + "\" (variant)";
				});
				Context->AddToStringCallback("any", [Context](Core::String& Indent, int Depth, void* Object, int TypeId)
				{
					Bindings::Any* Source = (Bindings::Any*)Object;
					return Context->ToString(Indent, Depth - 1, Source->GetAddressOfObject(), Source->GetTypeId());
				});
				Context->AddToStringCallback("promise, promise_v", [Context](Core::String& Indent, int Depth, void* Object, int TypeId)
				{
					Bindings::Promise* Source = (Bindings::Promise*)Object;
					Core::StringStream Stream;
					Stream << "(promise<T>)\n";
					Stream << Indent << "  state = " << (Source->IsPending() ? "pending\n" : "fulfilled\n");
					Stream << Indent << "  data = " << Context->ToString(Indent, Depth - 1, Source->GetAddressOfObject(), Source->GetTypeId());
					return Stream.str();
				});
				Context->AddToStringCallback("array", [Context](Core::String& Indent, int Depth, void* Object, int TypeId)
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
							Stream << Indent << "[" << i << "]: " << Context->ToString(Indent, Depth - 1, Source->At(i), BaseTypeId);
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
							Stream << Context->ToString(Indent, Depth - 1, Source->At(i), BaseTypeId);
							if (i + 1 < Size)
								Stream << ", ";
						}
						Stream << "]";
					}

					return Stream.str();
				});
				Context->AddToStringCallback("dictionary", [Context](Core::String& Indent, int Depth, void* Object, int TypeId)
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

						TypeInfo Type = Context->GetEngine()->GetTypeInfoById(TypeId);
						Core::String Typename = Core::String(Type.IsValid() ? Type.GetName() : "?");
						Stream << Indent << Core::Stringify::TrimEnd(Typename) << " \"" << Name << "\": " << Context->ToString(Indent, Depth - 1, Value, TypeId);
						if (i + 1 < Size)
							Stream << "\n";
					}

					Indent.erase(Indent.end() - 2, Indent.end());
					return Stream.str();
				});
				Context->AddToStringCallback("schema", [](Core::String& Indent, int Depth, void* Object, int TypeId)
				{
					Core::StringStream Stream;
					Core::Schema* Source = (Core::Schema*)Object;
					if (Source->Value.IsObject())
						Stream << "0x" << (void*)Source << "(schema)\n";

					Core::Schema::ConvertToJSON(Source, [&Indent, &Stream](Core::VarForm Type, const std::string_view& Buffer)
					{
						if (!Buffer.empty())
							Stream << Buffer;

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
				Context->AddToStringCallback("thread", [](Core::String& Indent, int Depth, void* Object, int TypeId)
				{
					Bindings::Thread* Source = (Bindings::Thread*)Object;
					Core::StringStream Stream;
					Stream << "(thread)\n";
					Stream << Indent << "  id = " << Source->GetId() << "\n";
					Stream << Indent << "  state = " << (Source->IsActive() ? "active" : "suspended");
					return Stream.str();
				});
				Context->AddToStringCallback("char_buffer", [](Core::String& Indent, int Depth, void* Object, int TypeId)
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
				return true;
			}
			bool Registry::ImportCTypes(VirtualMachine* VM) noexcept
			{
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, "manager should be set");
#ifdef VI_64
				VM->SetTypeDef("usize", "uint64");
#else
				VM->SetTypeDef("usize", "uint32");
#endif
				auto VPointer = VM->SetClassAddress("uptr", 0, (uint64_t)ObjectBehaviours::REF | (uint64_t)ObjectBehaviours::NOCOUNT);
				bool HasPointerCast = (!VM->GetLibraryProperty(LibraryFeatures::CTypesNoPointerCast));
				if (HasPointerCast)
				{
					VPointer->SetMethodEx("void opCast(?&out)", &PointerToHandleCast);
					VM->SetFunction("uptr@ to_ptr(?&in)", &HandleToPointerCast);
				}

				return true;
			}
			bool Registry::ImportAny(VirtualMachine* VM) noexcept
			{
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, "manager should be set");

				auto VAny = VM->SetClass<Any>("any", true);
				VAny->SetConstructorEx("any@ f()", &Any::Factory1);
				VAny->SetConstructorEx("any@ f(?&in) explicit", &Any::Factory2);
				VAny->SetConstructorEx("any@ f(const int64&in) explicit", &Any::Factory2);
				VAny->SetConstructorEx("any@ f(const double&in) explicit", &Any::Factory2);
				VAny->SetEnumRefs(&Any::EnumReferences);
				VAny->SetReleaseRefs(&Any::ReleaseReferences);
				VAny->SetMethodEx("any &opAssign(any&in)", &Any::Assignment);
				VAny->SetMethod("void store(?&in)", &Any::Store);
				VAny->SetMethod("bool retrieve(?&out)", &Any::Retrieve);

				return true;
			}
			bool Registry::ImportArray(VirtualMachine* VM) noexcept
			{
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, "manager should be set");
				VM->SetTypeInfoUserDataCleanupCallback(Array::CleanupTypeInfoCache, (size_t)Array::GetId());

				auto ListFactoryCall = Bridge::Function(&Array::Factory);
				{
					auto VArray = VM->SetTemplateClass<Array>("array<class T>", "array<T>", true);
					VArray->SetTemplateCallback(&Array::TemplateCallback);
					VArray->SetFunctionDef("bool array<T>::less_sync(const T&in a, const T&in b)");
					VArray->SetConstructorEx<Array, asITypeInfo*>("array<T>@ f(int&in)", &Array::Create);
					VArray->SetConstructorEx<Array, asITypeInfo*, size_t>("array<T>@ f(int&in, usize) explicit", &Array::Create);
					VArray->SetConstructorEx<Array, asITypeInfo*, size_t, void*>("array<T>@ f(int&in, usize, const T&in)", &Array::Create);
					VArray->SetBehaviourAddress("array<T>@ f(int&in, int&in) {repeat T}", Behaviours::LIST_FACTORY, ListFactoryCall, FunctionCall::CDECLF);
					VArray->SetEnumRefs(&Array::EnumReferences);
					VArray->SetReleaseRefs(&Array::ReleaseReferences);
					VArray->SetMethod("bool opEquals(const array<T>&in) const", &Array::operator==);
					VArray->SetMethod("array<T>& opAssign(const array<T>&in)", &Array::operator=);
					VArray->SetMethod<Array, void*, size_t>("T& opIndex(usize)", &Array::At);
					VArray->SetMethod<Array, const void*, size_t>("const T& opIndex(usize) const", &Array::At);
					VArray->SetMethod<Array, void*>("T& front()", &Array::Front);
					VArray->SetMethod<Array, const void*>("const T& front() const", &Array::Front);
					VArray->SetMethod<Array, void*>("T& back()", &Array::Back);
					VArray->SetMethod<Array, const void*>("const T& back() const", &Array::Back);
					VArray->SetMethod("bool empty() const", &Array::Empty);
					VArray->SetMethod("usize size() const", &Array::Size);
					VArray->SetMethod("usize capacity() const", &Array::Capacity);
					VArray->SetMethod("void reserve(usize)", &Array::Reserve);
					VArray->SetMethod<Array, void, size_t>("void resize(usize)", &Array::Resize);
					VArray->SetMethod("void clear()", &Array::Clear);
					VArray->SetMethod("void push(const T&in)", &Array::InsertLast);
					VArray->SetMethod("void pop()", &Array::RemoveLast);
					VArray->SetMethod<Array, void, size_t, void*>("void insert(usize, const T&in)", &Array::InsertAt);
					VArray->SetMethod<Array, void, size_t, const Array&>("void insert(usize, const array<T>&)", &Array::InsertAt);
					VArray->SetMethod("void erase(usize)", &Array::RemoveAt);
					VArray->SetMethod("void erase(usize, usize)", &Array::RemoveRange);
					VArray->SetMethod("void reverse()", &Array::Reverse);
					VArray->SetMethod("void swap(usize, usize)", &Array::Swap);
					VArray->SetMethod("void sort(less_sync@ = null)", &Array::Sort);
					VArray->SetMethod<Array, size_t, void*, size_t>("usize find(const T&in if_handle_then_const, usize = 0) const", &Array::Find);
					VArray->SetMethod<Array, size_t, void*, size_t>("usize find_ref(const T&in if_handle_then_const, usize = 0) const", &Array::FindByRef);
				}
				FunctionFactory::ReleaseFunctor(&ListFactoryCall);

				VM->SetDefaultArrayType("array<T>");
				VM->BeginNamespace("array");
				VM->SetProperty("const usize npos", &Core::String::npos);
				VM->EndNamespace();

				return true;
			}
			bool Registry::ImportComplex(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, "manager should be set");
				auto ListConstructorCall = Bridge::Function(&Complex::ListConstructor);
				{
					auto VComplex = VM->SetStructAddress("complex", sizeof(Complex), (size_t)ObjectBehaviours::VALUE | (size_t)ObjectBehaviours::POD | Bridge::GetTypeTraits<Complex>() | (size_t)ObjectBehaviours::APP_CLASS_ALLFLOATS);
					VComplex->SetProperty("float R", &Complex::R);
					VComplex->SetProperty("float I", &Complex::I);
					VComplex->SetConstructorEx("void f()", &Complex::DefaultConstructor);
					VComplex->SetConstructorEx("void f(const complex &in)", &Complex::CopyConstructor);
					VComplex->SetConstructorEx("void f(float)", &Complex::ConvConstructor);
					VComplex->SetConstructorEx("void f(float, float)", &Complex::InitConstructor);
					VComplex->SetBehaviourAddress("void f(const int &in) {float, float}", Behaviours::LIST_CONSTRUCT, ListConstructorCall, FunctionCall::CDECL_OBJFIRST);
					VComplex->SetMethod("complex &opAddAssign(const complex &in)", &Complex::operator+=);
					VComplex->SetMethod("complex &opSubAssign(const complex &in)", &Complex::operator-=);
					VComplex->SetMethod("complex &opMulAssign(const complex &in)", &Complex::operator*=);
					VComplex->SetMethod("complex &opDivAssign(const complex &in)", &Complex::operator/=);
					VComplex->SetMethod("bool opEquals(const complex &in) const", &Complex::operator==);
					VComplex->SetMethod("complex opAdd(const complex &in) const", &Complex::operator+);
					VComplex->SetMethod("complex opSub(const complex &in) const", &Complex::operator-);
					VComplex->SetMethod("complex opMul(const complex &in) const", &Complex::operator*);
					VComplex->SetMethod("complex opDiv(const complex &in) const", &Complex::operator/);
					VComplex->SetMethod("float abs() const", &Complex::Length);
					VComplex->SetMethod("complex get_ri() const property", &Complex::GetRI);
					VComplex->SetMethod("complex get_ir() const property", &Complex::GetIR);
					VComplex->SetMethod("void set_ri(const complex &in) property", &Complex::SetRI);
					VComplex->SetMethod("void set_ir(const complex &in) property", &Complex::SetIR);
				}
				FunctionFactory::ReleaseFunctor(&ListConstructorCall);
				return true;
#else
				VI_ASSERT(false, "<complex> is not loaded");
				return false;
#endif
			}
			bool Registry::ImportDictionary(VirtualMachine* VM) noexcept
			{
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, "manager should be set");

				auto VStorable = VM->SetStructAddress("storable", sizeof(Storable), (size_t)ObjectBehaviours::VALUE | (size_t)ObjectBehaviours::ASHANDLE | (size_t)ObjectBehaviours::GC | Bridge::GetTypeTraits<Storable>());
				VStorable->SetConstructorEx("void f()", &Dictionary::KeyCreate);
				VStorable->SetDestructorEx("void f()", &Dictionary::KeyDestroy);
				VStorable->SetEnumRefs(&Storable::EnumReferences);
				VStorable->SetReleaseRefs(&Storable::ReleaseReferences);
				VStorable->SetMethodEx<Storable&, Storable*, const Storable&>("storable &opAssign(const storable&in)", &Dictionary::KeyopAssign);
				VStorable->SetMethodEx<Storable&, Storable*, void*, int>("storable &opHndlAssign(const ?&in)", &Dictionary::KeyopAssign);
				VStorable->SetMethodEx<Storable&, Storable*, const Storable&>("storable &opHndlAssign(const storable&in)", &Dictionary::KeyopAssign);
				VStorable->SetMethodEx<Storable&, Storable*, void*, int>("storable &opAssign(const ?&in)", &Dictionary::KeyopAssign);
				VStorable->SetMethodEx<Storable&, Storable*, double>("storable &opAssign(double)", &Dictionary::KeyopAssign);
				VStorable->SetMethodEx<Storable&, Storable*, as_int64_t>("storable &opAssign(int64)", &Dictionary::KeyopAssign);
				VStorable->SetMethodEx("void opCast(?&out)", &Dictionary::KeyopCast);
				VStorable->SetMethodEx("void opConv(?&out)", &Dictionary::KeyopCast);
				VStorable->SetMethodEx("int64 opConv()", &Dictionary::KeyopConvInt);
				VStorable->SetMethodEx("double opConv()", &Dictionary::KeyopConvDouble);

				auto FactoryCall = Bridge::FunctionGeneric(&Dictionary::Factory);
				auto ListFactoryCall = Bridge::FunctionGeneric(&Dictionary::ListFactory);
				{
					auto VDictionary = VM->SetClass<Dictionary>("dictionary", true);
					VDictionary->SetBehaviourAddress("dictionary@ f()", Behaviours::FACTORY, FactoryCall, FunctionCall::GENERIC);
					VDictionary->SetBehaviourAddress("dictionary @f(int &in) {repeat {string, ?}}", Behaviours::LIST_FACTORY, ListFactoryCall, FunctionCall::GENERIC);
					VDictionary->SetEnumRefs(&Dictionary::EnumReferences);
					VDictionary->SetReleaseRefs(&Dictionary::ReleaseReferences);
					VDictionary->SetMethod("dictionary &opAssign(const dictionary &in)", &Dictionary::operator=);
					VDictionary->SetMethod<Dictionary, Storable*, const std::string_view&>("storable& opIndex(const string_view&in)", &Dictionary::operator[]);
					VDictionary->SetMethod<Dictionary, const Storable*, const std::string_view&>("const storable& opIndex(const string_view&in) const", &Dictionary::operator[]);
					VDictionary->SetMethod<Dictionary, Storable*, size_t>("storable& opIndex(usize)", &Dictionary::operator[]);
					VDictionary->SetMethod<Dictionary, const Storable*, size_t>("const storable& opIndex(usize) const", &Dictionary::operator[]);
					VDictionary->SetMethod("void set(const string_view&in, const ?&in)", &Dictionary::Set);
					VDictionary->SetMethod("bool get(const string_view&in, ?&out) const", &Dictionary::Get);
					VDictionary->SetMethod("bool exists(const string_view&in) const", &Dictionary::Exists);
					VDictionary->SetMethod("bool empty() const", &Dictionary::Empty);
					VDictionary->SetMethod("bool at(usize, string&out, ?&out) const", &Dictionary::TryGetIndex);
					VDictionary->SetMethod("usize size() const", &Dictionary::Size);
					VDictionary->SetMethod("bool erase(const string_view&in)", &Dictionary::Erase);
					VDictionary->SetMethod("void clear()", &Dictionary::Clear);
					VDictionary->SetMethod("array<string>@ get_keys() const", &Dictionary::GetKeys);
					Dictionary::Setup(VM);
				}
				FunctionFactory::ReleaseFunctor(&FactoryCall);
				FunctionFactory::ReleaseFunctor(&ListFactoryCall);
				return true;
			}
			bool Registry::ImportMath(VirtualMachine* VM) noexcept
			{
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, "manager should be set");
				VM->SetFunction<float(*)(uint32_t)>("float fp_from_ieee(uint)", &Math::FpFromIEEE);
				VM->SetFunction<uint32_t(*)(float)>("uint fp_to_ieee(float)", &Math::FpToIEEE);
				VM->SetFunction<double(*)(uint64_t)>("double fp_from_ieee(uint64)", &Math::FpFromIEEE);
				VM->SetFunction<uint64_t(*)(double)>("uint64 fp_to_ieee(double)", &Math::FpToIEEE);
				VM->SetFunction<bool(*)(float, float, float)>("bool close_to(float, float, float = 0.00001f)", &Math::CloseTo);
				VM->SetFunction<bool(*)(double, double, double)>("bool close_to(double, double, double = 0.0000000001)", &Math::CloseTo);
				VM->SetFunction("float rad2degf()", &Compute::Mathf::Rad2Deg);
				VM->SetFunction("float deg2radf()", &Compute::Mathf::Deg2Rad);
				VM->SetFunction("float pi_valuef()", &Compute::Mathf::Pi);
				VM->SetFunction("float clampf(float, float, float)", &Compute::Mathf::Clamp);
				VM->SetFunction("float acotanf(float)", &Compute::Mathf::Acotan);
				VM->SetFunction("float cotanf(float)", &Compute::Mathf::Cotan);
				VM->SetFunction("float minf(float, float)", &Compute::Mathf::Min);
				VM->SetFunction("float maxf(float, float)", &Compute::Mathf::Max);
				VM->SetFunction("float saturatef(float)", &Compute::Mathf::Saturate);
				VM->SetFunction("float lerpf(float, float, float)", &Compute::Mathf::Lerp);
				VM->SetFunction("float strong_lerpf(float, float, float)", &Compute::Mathf::StrongLerp);
				VM->SetFunction("float angle_saturatef(float)", &Compute::Mathf::SaturateAngle);
				VM->SetFunction<float(*)()>("float randomf()", &Compute::Mathf::Random);
				VM->SetFunction("float random_magf()", &Compute::Mathf::RandomMag);
				VM->SetFunction<float(*)(float, float)>("float random_rangef(float, float)", &Compute::Mathf::Random);
				VM->SetFunction<float(*)(float, float, float, float, float)>("float mapf(float, float, float, float, float)", &Compute::Mathf::Map);
				VM->SetFunction("float cosf(float)", &Compute::Mathf::Cos);
				VM->SetFunction("float sinf(float)", &Compute::Mathf::Sin);
				VM->SetFunction("float tanf(float)", &Compute::Mathf::Tan);
				VM->SetFunction("float acosf(float)", &Compute::Mathf::Acos);
				VM->SetFunction("float asinf(float)", &Compute::Mathf::Asin);
				VM->SetFunction("float atanf(float)", &Compute::Mathf::Atan);
				VM->SetFunction("float atan2f(float, float)", &Compute::Mathf::Atan2);
				VM->SetFunction("float expf(float)", &Compute::Mathf::Exp);
				VM->SetFunction("float logf(float)", &Compute::Mathf::Log);
				VM->SetFunction("float log2f(float)", &Compute::Mathf::Log2);
				VM->SetFunction("float log10f(float)", &Compute::Mathf::Log10);
				VM->SetFunction("float powf(float, float)", &Compute::Mathf::Pow);
				VM->SetFunction("float sqrtf(float)", &Compute::Mathf::Sqrt);
				VM->SetFunction("float ceilf(float)", &Compute::Mathf::Ceil);
				VM->SetFunction("float absf(float)", &Compute::Mathf::Abs);
				VM->SetFunction("float floorf(float)", &Compute::Mathf::Floor);
				VM->SetFunction("double rad2degd()", &Compute::Mathd::Rad2Deg);
				VM->SetFunction("double deg2radd()", &Compute::Mathd::Deg2Rad);
				VM->SetFunction("double pi_valued()", &Compute::Mathd::Pi);
				VM->SetFunction("double clampd(double, double, double)", &Compute::Mathd::Clamp);
				VM->SetFunction("double acotand(double)", &Compute::Mathd::Acotan);
				VM->SetFunction("double cotand(double)", &Compute::Mathd::Cotan);
				VM->SetFunction("double mind(double, double)", &Compute::Mathd::Min);
				VM->SetFunction("double maxd(double, double)", &Compute::Mathd::Max);
				VM->SetFunction("double saturated(double)", &Compute::Mathd::Saturate);
				VM->SetFunction("double lerpd(double, double, double)", &Compute::Mathd::Lerp);
				VM->SetFunction("double strong_lerpd(double, double, double)", &Compute::Mathd::StrongLerp);
				VM->SetFunction("double angle_saturated(double)", &Compute::Mathd::SaturateAngle);
				VM->SetFunction<double(*)()>("double randomd()", &Compute::Mathd::Random);
				VM->SetFunction("double random_magd()", &Compute::Mathd::RandomMag);
				VM->SetFunction<double(*)(double, double)>("double random_ranged(double, double)", &Compute::Mathd::Random);
				VM->SetFunction("double mapd(double, double, double, double, double)", &Compute::Mathd::Map);
				VM->SetFunction("double cosd(double)", &Compute::Mathd::Cos);
				VM->SetFunction("double sind(double)", &Compute::Mathd::Sin);
				VM->SetFunction("double tand(double)", &Compute::Mathd::Tan);
				VM->SetFunction("double acosd(double)", &Compute::Mathd::Acos);
				VM->SetFunction("double asind(double)", &Compute::Mathd::Asin);
				VM->SetFunction("double atand(double)", &Compute::Mathd::Atan);
				VM->SetFunction("double atan2d(double, double)", &Compute::Mathd::Atan2);
				VM->SetFunction("double expd(double)", &Compute::Mathd::Exp);
				VM->SetFunction("double logd(double)", &Compute::Mathd::Log);
				VM->SetFunction("double log2d(double)", &Compute::Mathd::Log2);
				VM->SetFunction("double log10d(double)", &Compute::Mathd::Log10);
				VM->SetFunction("double powd(double, double)", &Compute::Mathd::Pow);
				VM->SetFunction("double sqrtd(double)", &Compute::Mathd::Sqrt);
				VM->SetFunction("double ceild(double)", &Compute::Mathd::Ceil);
				VM->SetFunction("double absd(double)", &Compute::Mathd::Abs);
				VM->SetFunction("double floord(double)", &Compute::Mathd::Floor);

				return true;
			}
			bool Registry::ImportString(VirtualMachine* VM) noexcept
			{
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, "manager should be set");
				auto VStringView = VM->SetStructAddress("string_view", sizeof(std::string_view), (size_t)ObjectBehaviours::VALUE | Bridge::GetTypeTraits<std::string_view>() | (size_t)ObjectBehaviours::APP_CLASS_ALLINTS);
				auto VString = VM->SetStructAddress("string", sizeof(Core::String), (size_t)ObjectBehaviours::VALUE | Bridge::GetTypeTraits<Core::String>());
#ifdef VI_ANGELSCRIPT
				VM->SetStringFactoryAddress("string", StringFactory::Get());
#endif
				VString->SetConstructorEx("void f()", &String::Create);
				VString->SetConstructorEx("void f(const string&in)", &String::CreateCopy1);
				VString->SetConstructorEx("void f(const string_view&in)", &String::CreateCopy2);
				VString->SetDestructorEx("void f()", &String::Destroy);
				VString->SetMethod<Core::String, Core::String&, const Core::String&>("string& opAssign(const string&in)", &Core::String::operator=);
				VString->SetMethod<Core::String, Core::String&, const std::string_view&>("string& opAssign(const string_view&in)", &Core::String::operator=);
				VString->SetMethod<Core::String, Core::String&, const Core::String&>("string& opAddAssign(const string&in)", &Core::String::operator+=);
				VString->SetMethod<Core::String, Core::String&, const std::string_view&>("string& opAddAssign(const string_view&in)", &Core::String::operator+=);
				VString->SetMethodEx<Core::String, const Core::String&, const Core::String&>("string opAdd(const string&in) const", &std::operator+);
				VString->SetMethod<Core::String, Core::String&, char>("string& opAddAssign(uint8)", &Core::String::operator+=);
				VString->SetMethodEx<Core::String, const Core::String&, char>("string opAdd(uint8) const", &std::operator+);
				VString->SetMethodEx<Core::String, char, const Core::String&>("string opAdd_r(uint8) const", &std::operator+);
				VString->SetMethod<Core::String, int, const Core::String&>("int opCmp(const string&in) const", &Core::String::compare);
				VString->SetMethod<Core::String, int, const std::string_view&>("int opCmp(const string_view&in) const", &Core::String::compare);
				VString->SetMethodEx("uint8& opIndex(usize)", &String::Index);
				VString->SetMethodEx("const uint8& opIndex(usize) const", &String::Index);
				VString->SetMethodEx("uint8& at(usize)", &String::Index);
				VString->SetMethodEx("const uint8& at(usize) const", &String::Index);
				VString->SetMethodEx("uint8& front()", &String::Front);
				VString->SetMethodEx("const uint8& front() const", &String::Front);
				VString->SetMethodEx("uint8& back()", &String::Back);
				VString->SetMethodEx("const uint8& back() const", &String::Back);
				VString->SetMethod("uptr@ data() const", &Core::String::c_str);
				VString->SetMethod("uptr@ c_str() const", &Core::String::c_str);
				VString->SetMethod("bool empty() const", &Core::String::empty);
				VString->SetMethod("usize size() const", &Core::String::size);
				VString->SetMethod("usize max_size() const", &Core::String::max_size);
				VString->SetMethod("usize capacity() const", &Core::String::capacity);
				VString->SetMethod<Core::String, void, size_t>("void reserve(usize)", &Core::String::reserve);
				VString->SetMethod<Core::String, void, size_t, char>("void resize(usize, uint8 = 0)", &Core::String::resize);
				VString->SetMethod("void shrink_to_fit()", &Core::String::shrink_to_fit);
				VString->SetMethod("void clear()", &Core::String::clear);
				VString->SetMethod<Core::String, Core::String&, size_t, size_t, char>("string& insert(usize, usize, uint8)", &Core::String::insert);
				VString->SetMethod<Core::String, Core::String&, size_t, const Core::String&>("string& insert(usize, const string&in)", &Core::String::insert);
				VString->SetMethod<Core::String, Core::String&, size_t, const std::string_view&>("string& insert(usize, const string_view&in)", &Core::String::insert);
				VString->SetMethod<Core::String, Core::String&, size_t, size_t>("string& erase(usize, usize)", &Core::String::erase);
				VString->SetMethod<Core::String, Core::String&, size_t, char>("string& append(usize, uint8)", &Core::String::append);
				VString->SetMethod<Core::String, Core::String&, const Core::String&>("string& append(const string&in)", &Core::String::append);
				VString->SetMethod<Core::String, Core::String&, const std::string_view&>("string& append(const string_view&in)", &Core::String::append);
				VString->SetMethod("void push(uint8)", &Core::String::push_back);
				VString->SetMethodEx("void pop()", &String::PopBack);
				VString->SetMethodEx("bool starts_with(const string&in, usize = 0) const", &String::StartsWith1);
				VString->SetMethodEx("bool ends_with(const string&in) const", &String::EndsWith1);
				VString->SetMethodEx("string& replace(usize, usize, const string&in)", &String::ReplacePart1);
				VString->SetMethodEx("string& replace_all(const string&in, const string&in, usize)", &String::Replace1);
				VString->SetMethodEx("bool starts_with(const string_view&in, usize = 0) const", &String::StartsWith2);
				VString->SetMethodEx("bool ends_with(const string_view&in) const", &String::EndsWith2);
				VString->SetMethodEx("string& replace(usize, usize, const string_view&in)", &String::ReplacePart2);
				VString->SetMethodEx("string& replace_all(const string_view&in, const string_view&in, usize)", &String::Replace2);
				VString->SetMethodEx("string substring(usize) const", &String::Substring1);
				VString->SetMethodEx("string substring(usize, usize) const", &String::Substring2);
				VString->SetMethodEx("string substr(usize) const", &String::Substring1);
				VString->SetMethodEx("string substr(usize, usize) const", &String::Substring2);
				VString->SetMethodEx("string& trim()", &Core::Stringify::Trim);
				VString->SetMethodEx("string& trim_start()", &Core::Stringify::TrimStart);
				VString->SetMethodEx("string& trim_end()", &Core::Stringify::TrimEnd);
				VString->SetMethodEx("string& to_lower()", &Core::Stringify::ToLower);
				VString->SetMethodEx("string& to_upper()", &Core::Stringify::ToUpper);
				VString->SetMethodEx<Core::String&, Core::String&>("string& reverse()", &Core::Stringify::Reverse);
				VString->SetMethod<Core::String, size_t, const Core::String&, size_t>("usize rfind(const string&in, usize = 0) const", &Core::String::rfind);
				VString->SetMethod<Core::String, size_t, const std::string_view&, size_t>("usize rfind(const string_view&in, usize = 0) const", &Core::String::rfind);
				VString->SetMethod<Core::String, size_t, char, size_t>("usize rfind(uint8, usize = 0) const", &Core::String::rfind);
				VString->SetMethod<Core::String, size_t, const Core::String&, size_t>("usize find(const string&in, usize = 0) const", &Core::String::find);
				VString->SetMethod<Core::String, size_t, const std::string_view&, size_t>("usize find(const string_view&in, usize = 0) const", &Core::String::find);
				VString->SetMethod<Core::String, size_t, char, size_t>("usize find(uint8, usize = 0) const", &Core::String::find);
				VString->SetMethod<Core::String, size_t, const Core::String&, size_t>("usize find_first_of(const string&in, usize = 0) const", &Core::String::find_first_of);
				VString->SetMethod<Core::String, size_t, const Core::String&, size_t>("usize find_first_not_of(const string&in, usize = 0) const", &Core::String::find_first_not_of);
				VString->SetMethod<Core::String, size_t, const Core::String&, size_t>("usize find_last_of(const string&in, usize = 0) const", &Core::String::find_last_of);
				VString->SetMethod<Core::String, size_t, const Core::String&, size_t>("usize find_last_not_of(const string&in, usize = 0) const", &Core::String::find_last_not_of);
				VString->SetMethod<Core::String, size_t, const std::string_view&, size_t>("usize find_first_of(const string_view&in, usize = 0) const", &Core::String::find_first_of);
				VString->SetMethod<Core::String, size_t, const std::string_view&, size_t>("usize find_first_not_of(const string_view&in, usize = 0) const", &Core::String::find_first_not_of);
				VString->SetMethod<Core::String, size_t, const std::string_view&, size_t>("usize find_last_of(const string_view&in, usize = 0) const", &Core::String::find_last_of);
				VString->SetMethod<Core::String, size_t, const std::string_view&, size_t>("usize find_last_not_of(const string_view&in, usize = 0) const", &Core::String::find_last_not_of);
				VString->SetMethodEx("array<string>@ split(const string&in) const", &String::Split);
				VString->SetOperatorEx(Operators::ImplCast, (uint32_t)Position::Const, "string_view", "", &String::ImplCastStringView);
				VM->SetFunction("int8 to_int8(const string&in, int = 10)", &String::FromString<int8_t>);
				VM->SetFunction("int16 to_int16(const string&in, int = 10)", &String::FromString<int16_t>);
				VM->SetFunction("int32 to_int32(const string&in, int = 10)", &String::FromString<int32_t>);
				VM->SetFunction("int64 to_int64(const string&in, int = 10)", &String::FromString<int64_t>);
				VM->SetFunction("uint8 to_uint8(const string&in, int = 10)", &String::FromString<uint8_t>);
				VM->SetFunction("uint16 to_uint16(const string&in, int = 10)", &String::FromString<uint16_t>);
				VM->SetFunction("uint32 to_uint32(const string&in, int = 10)", &String::FromString<uint32_t>);
				VM->SetFunction("uint64 to_uint64(const string&in, int = 10)", &String::FromString<uint64_t>);
				VM->SetFunction("float to_float(const string&in, int = 10)", &String::FromString<float>);
				VM->SetFunction("double to_double(const string&in, int = 10)", &String::FromString<double>);
				VM->SetFunction("string to_string(int8, int = 10)", &Core::ToString<int8_t>);
				VM->SetFunction("string to_string(int16, int = 10)", &Core::ToString<int16_t>);
				VM->SetFunction("string to_string(int32, int = 10)", &Core::ToString<int32_t>);
				VM->SetFunction("string to_string(int64, int = 10)", &Core::ToString<int64_t>);
				VM->SetFunction("string to_string(uint8, int = 10)", &Core::ToString<uint8_t>);
				VM->SetFunction("string to_string(uint16, int = 10)", &Core::ToString<uint16_t>);
				VM->SetFunction("string to_string(uint32, int = 10)", &Core::ToString<uint32_t>);
				VM->SetFunction("string to_string(uint64, int = 10)", &Core::ToString<uint64_t>);
				VM->SetFunction("string to_string(float, int = 10)", &Core::ToString<float>);
				VM->SetFunction("string to_string(double, int = 10)", &Core::ToString<double>);
				VM->SetFunction("string to_string(uptr@, usize)", &String::FromBuffer);
				VM->SetFunction("string handle_id(uptr@)", &String::FromPointer);
				VM->BeginNamespace("string");
				VM->SetProperty("const usize npos", &Core::String::npos);
				VM->EndNamespace();

				VStringView->SetConstructorEx("void f()", &StringView::Create);
				VStringView->SetConstructorEx("void f(const string&in)", &StringView::CreateCopy);
				VStringView->SetDestructorEx("void f()", &StringView::Destroy);
				VStringView->SetMethodEx("string_view& opAssign(const string&in)", &StringView::Assign);
				VStringView->SetMethodEx<Core::String, const std::string_view&, const std::string_view&>("string opAdd(const string_view&in) const", &StringView::Append1);
				VStringView->SetMethodEx<Core::String, const std::string_view&, const Core::String&>("string opAdd(const string&in) const", &StringView::Append2);
				VStringView->SetMethodEx<Core::String, const Core::String&, const std::string_view&>("string opAdd_r(const string&in) const", &StringView::Append3);
				VStringView->SetMethodEx<Core::String, const std::string_view&, char>("string opAdd(uint8) const", &StringView::Append4);
				VStringView->SetMethodEx<Core::String, char, const std::string_view&>("string opAdd_r(uint8) const", &StringView::Append5);
				VStringView->SetMethodEx("int opCmp(const string&in) const", &StringView::Compare1);
				VStringView->SetMethodEx("int opCmp(const string_view&in) const", &StringView::Compare2);
				VStringView->SetMethodEx("const uint8& opIndex(usize) const", &StringView::Index);
				VStringView->SetMethodEx("const uint8& at(usize) const", &StringView::Index);
				VStringView->SetMethodEx("const uint8& front() const", &StringView::Front);
				VStringView->SetMethodEx("const uint8& back() const", &StringView::Back);
				VStringView->SetMethod("uptr@ data() const", &std::string_view::data);
				VStringView->SetMethod("bool empty() const", &std::string_view::empty);
				VStringView->SetMethod("usize size() const", &std::string_view::size);
				VStringView->SetMethod("usize max_size() const", &std::string_view::max_size);
				VStringView->SetMethodEx("bool starts_with(const string_view&in, usize = 0) const", &StringView::StartsWith);
				VStringView->SetMethodEx("bool ends_with(const string_view&in) const", &StringView::EndsWith);
				VStringView->SetMethodEx("string substring(usize) const", &StringView::Substring1);
				VStringView->SetMethodEx("string substring(usize, usize) const", &StringView::Substring2);
				VStringView->SetMethodEx("string substr(usize) const", &StringView::Substring1);
				VStringView->SetMethodEx("string substr(usize, usize) const", &StringView::Substring2);
				VStringView->SetMethodEx("usize rfind(const string_view&in, usize = 0) const", &StringView::ReverseFind1);
				VStringView->SetMethodEx("usize rfind(uint8, usize = 0) const", &StringView::ReverseFind2);
				VStringView->SetMethodEx("usize find(const string_view&in, usize = 0) const", &StringView::Find1);
				VStringView->SetMethodEx("usize find(uint8, usize = 0) const", &StringView::Find2);
				VStringView->SetMethodEx("usize find_first_of(const string_view&in, usize = 0) const", &StringView::FindFirstOf);
				VStringView->SetMethodEx("usize find_first_not_of(const string_view&in, usize = 0) const", &StringView::FindFirstNotOf);
				VStringView->SetMethodEx("usize find_last_of(const string_view&in, usize = 0) const", &StringView::FindLastOf);
				VStringView->SetMethodEx("usize find_last_not_of(const string_view&in, usize = 0) const", &StringView::FindLastNotOf);
				VStringView->SetMethodEx("array<string>@ split(const string_view&in) const", &StringView::Split);
				VStringView->SetOperatorEx(Operators::ImplCast, (uint32_t)Position::Const, "string", "", &StringView::ImplCastString);
				VM->SetFunction("int8 to_int8(const string_view&in)", &StringView::FromString<int8_t>);
				VM->SetFunction("int16 to_int16(const string_view&in)", &StringView::FromString<int16_t>);
				VM->SetFunction("int32 to_int32(const string_view&in)", &StringView::FromString<int32_t>);
				VM->SetFunction("int64 to_int64(const string_view&in)", &StringView::FromString<int64_t>);
				VM->SetFunction("uint8 to_uint8(const string_view&in)", &StringView::FromString<uint8_t>);
				VM->SetFunction("uint16 to_uint16(const string_view&in)", &StringView::FromString<uint16_t>);
				VM->SetFunction("uint32 to_uint32(const string_view&in)", &StringView::FromString<uint32_t>);
				VM->SetFunction("uint64 to_uint64(const string_view&in)", &StringView::FromString<uint64_t>);
				VM->SetFunction("float to_float(const string_view&in)", &StringView::FromString<float>);
				VM->SetFunction("double to_double(const string_view&in)", &StringView::FromString<double>);
				VM->SetFunction("uint64 component_id(const string_view&in)", &Core::OS::File::GetHash);
				VM->BeginNamespace("string_view");
				VM->SetProperty("const usize npos", &std::string_view::npos);
				VM->EndNamespace();

				return true;
			}
			bool Registry::ImportException(VirtualMachine* VM) noexcept
			{
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, "manager should be set");
				auto VExceptionPtr = VM->SetStructTrivial<Exception::Pointer>("exception_ptr");
				VExceptionPtr->SetProperty("string type", &Exception::Pointer::Type);
				VExceptionPtr->SetProperty("string text", &Exception::Pointer::Text);
				VExceptionPtr->SetProperty("string origin", &Exception::Pointer::Origin);
				VExceptionPtr->SetConstructor<Exception::Pointer>("void f()");
				VExceptionPtr->SetConstructor<Exception::Pointer, const std::string_view&>("void f(const string_view&in)");
				VExceptionPtr->SetConstructor<Exception::Pointer, const std::string_view&, const std::string_view&>("void f(const string_view&in, const string_view&in)");
				VExceptionPtr->SetMethod("const string& get_type() const", &Exception::Pointer::GetType);
				VExceptionPtr->SetMethod("const string& get_text() const", &Exception::Pointer::GetText);
				VExceptionPtr->SetMethod("string what() const", &Exception::Pointer::What);
				VExceptionPtr->SetMethod("bool empty() const", &Exception::Pointer::Empty);

				VM->SetCodeGenerator("throw-syntax", &Exception::GeneratorCallback);
				VM->BeginNamespace("exception");
				VM->SetFunction("void throw(const exception_ptr&in)", &Exception::Throw);
				VM->SetFunction("void rethrow()", &Exception::Rethrow);
				VM->SetFunction("exception_ptr unwrap()", &Exception::GetException);
				VM->EndNamespace();
				return true;
			}
			bool Registry::ImportMutex(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, "manager should be set");
				auto VMutex = VM->SetClass<Mutex>("mutex", false);
				VMutex->SetConstructorEx("mutex@ f()", &Mutex::Factory);
				VMutex->SetMethod("bool try_lock()", &Mutex::TryLock);
				VMutex->SetMethod("void lock()", &Mutex::Lock);
				VMutex->SetMethod("void unlock()", &Mutex::Unlock);
				return true;
#else
				VI_ASSERT(false, "<mutex> is not loaded");
				return false;
#endif
			}
			bool Registry::ImportThread(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, "manager should be set");
				auto VThread = VM->SetClass<Thread>("thread", true);
				VThread->SetFunctionDef("void thread_parallel(thread@+)");
				VThread->SetConstructorEx("thread@ f(thread_parallel@)", &Thread::Create);
				VThread->SetEnumRefs(&Thread::EnumReferences);
				VThread->SetReleaseRefs(&Thread::ReleaseReferences);
				VThread->SetMethod("bool is_active()", &Thread::IsActive);
				VThread->SetMethod("bool invoke()", &Thread::Start);
				VThread->SetMethod("bool suspend()", &Thread::Suspend);
				VThread->SetMethod("bool resume()", &Thread::Resume);
				VThread->SetMethod("void push(const ?&in)", &Thread::Push);
				VThread->SetMethod<Thread, bool, void*, int>("bool pop(?&out)", &Thread::Pop);
				VThread->SetMethod<Thread, bool, void*, int, uint64_t>("bool pop(?&out, uint64)", &Thread::Pop);
				VThread->SetMethod<Thread, int, uint64_t>("int join(uint64)", &Thread::Join);
				VThread->SetMethod<Thread, int>("int join()", &Thread::Join);
				VThread->SetMethod("string get_id() const", &Thread::GetId);

				VM->BeginNamespace("this_thread");
				VM->SetFunction("thread@+ get_routine()", &Thread::GetThread);
				VM->SetFunction("string get_id()", &Thread::GetThreadId);
				VM->SetFunction("void sleep(uint64)", &Thread::ThreadSleep);
				VM->SetFunction("bool suspend()", &Thread::ThreadSuspend);
				VM->EndNamespace();
				return true;
#else
				VI_ASSERT(false, "<thread> is not loaded");
				return false;
#endif
			}
			bool Registry::ImportBuffers(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, "manager should be set");
				auto VCharBuffer = VM->SetClass<CharBuffer>("char_buffer", false);
				VCharBuffer->SetConstructorEx<CharBuffer>("char_buffer@ f()", &CharBuffer::Create);
				VCharBuffer->SetConstructorEx<CharBuffer, size_t>("char_buffer@ f(usize)", &CharBuffer::Create);
				VCharBuffer->SetConstructorEx<CharBuffer, char*>("char_buffer@ f(uptr@)", &CharBuffer::Create);
				VCharBuffer->SetMethod("uptr@ get_ptr(usize = 0) const", &CharBuffer::GetPointer);
				VCharBuffer->SetMethod("bool allocate(usize)", &CharBuffer::Allocate);
				VCharBuffer->SetMethod("bool deallocate()", &CharBuffer::Deallocate);
				VCharBuffer->SetMethod("bool exists(usize) const", &CharBuffer::Exists);
				VCharBuffer->SetMethod("bool empty() const", &CharBuffer::Empty);
				VCharBuffer->SetMethod("bool set(usize, int8, usize)", &CharBuffer::SetInt8);
				VCharBuffer->SetMethod("bool set(usize, uint8, usize)", &CharBuffer::SetUint8);
				VCharBuffer->SetMethod("bool store(usize, const string_view&in)", &CharBuffer::StoreBytes);
				VCharBuffer->SetMethod("bool store(usize, int8)", &CharBuffer::StoreInt8);
				VCharBuffer->SetMethod("bool store(usize, uint8)", &CharBuffer::StoreUint8);
				VCharBuffer->SetMethod("bool store(usize, int16)", &CharBuffer::StoreInt16);
				VCharBuffer->SetMethod("bool store(usize, uint16)", &CharBuffer::StoreUint16);
				VCharBuffer->SetMethod("bool store(usize, int32)", &CharBuffer::StoreInt32);
				VCharBuffer->SetMethod("bool store(usize, uint32)", &CharBuffer::StoreUint32);
				VCharBuffer->SetMethod("bool store(usize, int64)", &CharBuffer::StoreInt64);
				VCharBuffer->SetMethod("bool store(usize, uint64)", &CharBuffer::StoreUint64);
				VCharBuffer->SetMethod("bool store(usize, float)", &CharBuffer::StoreFloat);
				VCharBuffer->SetMethod("bool store(usize, double)", &CharBuffer::StoreDouble);
				VCharBuffer->SetMethod("bool interpret(usize, string&out, usize) const", &CharBuffer::Interpret);
				VCharBuffer->SetMethod("bool load(usize, string&out, usize) const", &CharBuffer::LoadBytes);
				VCharBuffer->SetMethod("bool load(usize, int8&out) const", &CharBuffer::LoadInt8);
				VCharBuffer->SetMethod("bool load(usize, uint8&out) const", &CharBuffer::LoadUint8);
				VCharBuffer->SetMethod("bool load(usize, int16&out) const", &CharBuffer::LoadInt16);
				VCharBuffer->SetMethod("bool load(usize, uint16&out) const", &CharBuffer::LoadUint16);
				VCharBuffer->SetMethod("bool load(usize, int32&out) const", &CharBuffer::LoadInt32);
				VCharBuffer->SetMethod("bool load(usize, uint32&out) const", &CharBuffer::LoadUint32);
				VCharBuffer->SetMethod("bool load(usize, int64&out) const", &CharBuffer::LoadInt64);
				VCharBuffer->SetMethod("bool load(usize, uint64&out) const", &CharBuffer::LoadUint64);
				VCharBuffer->SetMethod("bool load(usize, float&out) const", &CharBuffer::LoadFloat);
				VCharBuffer->SetMethod("bool load(usize, double&out) const", &CharBuffer::LoadDouble);
				VCharBuffer->SetMethod("usize size() const", &CharBuffer::Size);
				VCharBuffer->SetMethod("string to_string(usize) const", &CharBuffer::ToString);
				return true;
#else
				VI_ASSERT(false, "<buffers> is not loaded");
				return false;
#endif
			}
			bool Registry::ImportRandom(VirtualMachine* VM) noexcept
			{
				VI_ASSERT(VM != nullptr && VM->GetEngine() != nullptr, "manager should be set");

				VM->BeginNamespace("random");
				VM->SetFunction("string bytes(uint64)", &Random::Getb);
				VM->SetFunction("double betweend(double, double)", &Random::Betweend);
				VM->SetFunction("double magd()", &Random::Magd);
				VM->SetFunction("double getd()", &Random::Getd);
				VM->SetFunction("float betweenf(float, float)", &Random::Betweenf);
				VM->SetFunction("float magf()", &Random::Magf);
				VM->SetFunction("float getf()", &Random::Getf);
				VM->SetFunction("uint64 betweeni(uint64, uint64)", &Random::Betweeni);
				VM->EndNamespace();

				return true;
			}
			bool Registry::ImportPromise(VirtualMachine* VM) noexcept
			{
				VI_ASSERT(VM != nullptr, "manager should be set");
				auto VPromise = VM->SetTemplateClass<Promise>("promise<class T>", "promise<T>", true);
				VPromise->SetTemplateCallback(&Promise::TemplateCallback);
				VPromise->SetEnumRefs(&Promise::EnumReferences);
				VPromise->SetReleaseRefs(&Promise::ReleaseReferences);
				VPromise->SetMethod<Promise, void, void*, int>("void wrap(?&in)", &Promise::Store);
				VPromise->SetMethod("void except(const exception_ptr&in)", &Promise::StoreException);
				VPromise->SetMethod<Promise, void*>("T& unwrap()", &Promise::Retrieve);
				VPromise->SetMethod("promise<T>@+ yield()", &Promise::YieldIf);
				VPromise->SetMethod("bool pending()", &Promise::IsPending);

				auto VPromiseVoid = VM->SetClass<Promise>("promise_v", true);
				VPromiseVoid->SetEnumRefs(&Promise::EnumReferences);
				VPromiseVoid->SetReleaseRefs(&Promise::ReleaseReferences);
				VPromiseVoid->SetMethod("void wrap()", &Promise::StoreVoid);
				VPromiseVoid->SetMethod("void except(const exception_ptr&in)", &Promise::StoreException);
				VPromiseVoid->SetMethod("void unwrap()", &Promise::RetrieveVoid);
				VPromiseVoid->SetMethod("promise_v@+ yield()", &Promise::YieldIf);
				VPromiseVoid->SetMethod("bool pending()", &Promise::IsPending);

				bool HasConstructor = (!VM->GetLibraryProperty(LibraryFeatures::PromiseNoConstructor));
				if (HasConstructor)
				{
					VPromise->SetConstructorEx("promise<T>@ f(int&in)", &Promise::CreateFactoryType);
					VPromiseVoid->SetConstructorEx("promise_v@ f()", &Promise::CreateFactoryVoid);
				}

				bool HasCallbacks = (!VM->GetLibraryProperty(LibraryFeatures::PromiseNoCallbacks));
				if (HasCallbacks)
				{
					VPromise->SetFunctionDef("void promise<T>::when_async(promise<T>@+)");
					VPromise->SetMethod<Promise, void, asIScriptFunction*>("void when(when_async@)", &Promise::When);
					VPromiseVoid->SetFunctionDef("void promise_v::when_async(promise_v@+)");
					VPromiseVoid->SetMethod<Promise, void, asIScriptFunction*>("void when(when_async@)", &Promise::When);
				}

				VM->SetCodeGenerator("await-syntax", &Promise::GeneratorCallback);
				return true;
			}
			bool Registry::ImportDecimal(VirtualMachine* VM) noexcept
			{
				VI_ASSERT(VM != nullptr, "manager should be set");

				auto VDecimal = VM->SetStructTrivial<Core::Decimal>("decimal");
				VDecimal->SetConstructor<Core::Decimal>("void f()");
				VDecimal->SetConstructor<Core::Decimal, int16_t>("void f(int16)");
				VDecimal->SetConstructor<Core::Decimal, uint16_t>("void f(uint16)");
				VDecimal->SetConstructor<Core::Decimal, int32_t>("void f(int32)");
				VDecimal->SetConstructor<Core::Decimal, uint32_t>("void f(uint32)");
				VDecimal->SetConstructor<Core::Decimal, int64_t>("void f(int64)");
				VDecimal->SetConstructor<Core::Decimal, uint64_t>("void f(uint64)");
				VDecimal->SetConstructor<Core::Decimal, float>("void f(float)");
				VDecimal->SetConstructor<Core::Decimal, double>("void f(double)");
				VDecimal->SetConstructor<Core::Decimal, const std::string_view&>("void f(const string_view&in)");
				VDecimal->SetConstructor<Core::Decimal, const Core::Decimal&>("void f(const decimal &in)");
				VDecimal->SetMethod("decimal& truncate(int)", &Core::Decimal::Truncate);
				VDecimal->SetMethod("decimal& round(int)", &Core::Decimal::Round);
				VDecimal->SetMethod("decimal& trim()", &Core::Decimal::Trim);
				VDecimal->SetMethod("decimal& unlead()", &Core::Decimal::Unlead);
				VDecimal->SetMethod("decimal& untrail()", &Core::Decimal::Untrail);
				VDecimal->SetMethod("bool is_nan() const", &Core::Decimal::IsNaN);
				VDecimal->SetMethod("bool is_zero_or_nan() const", &Core::Decimal::IsZeroOrNaN);
				VDecimal->SetMethod("bool is_positive() const", &Core::Decimal::IsPositive);
				VDecimal->SetMethod("bool is_negative() const", &Core::Decimal::IsNegative);
				VDecimal->SetMethod("float to_float() const", &Core::Decimal::ToFloat);
				VDecimal->SetMethod("double to_double() const", &Core::Decimal::ToDouble);
				VDecimal->SetMethod("int8 to_int8() const", &Core::Decimal::ToInt8);
				VDecimal->SetMethod("uint8 to_uint8() const", &Core::Decimal::ToUInt8);
				VDecimal->SetMethod("int16 to_int16() const", &Core::Decimal::ToInt16);
				VDecimal->SetMethod("uint16 to_uint16() const", &Core::Decimal::ToUInt16);
				VDecimal->SetMethod("int32 to_int32() const", &Core::Decimal::ToInt32);
				VDecimal->SetMethod("uint32 to_uint32() const", &Core::Decimal::ToUInt32);
				VDecimal->SetMethod("int64 to_int64() const", &Core::Decimal::ToInt64);
				VDecimal->SetMethod("uint64 to_uint64() const", &Core::Decimal::ToUInt64);
				VDecimal->SetMethod("string to_string() const", &Core::Decimal::ToString);
				VDecimal->SetMethod("string to_exponent() const", &Core::Decimal::ToExponent);
				VDecimal->SetMethod("uint32 decimals() const", &Core::Decimal::Decimals);
				VDecimal->SetMethod("uint32 ints() const", &Core::Decimal::Ints);
				VDecimal->SetMethod("uint32 size() const", &Core::Decimal::Size);
				VDecimal->SetOperatorEx(Operators::Neg, (uint32_t)Position::Const, "decimal", "", &DecimalNegate);
				VDecimal->SetOperatorEx(Operators::MulAssign, (uint32_t)Position::Left, "decimal&", "const decimal &in", &DecimalMulEq);
				VDecimal->SetOperatorEx(Operators::DivAssign, (uint32_t)Position::Left, "decimal&", "const decimal &in", &DecimalDivEq);
				VDecimal->SetOperatorEx(Operators::AddAssign, (uint32_t)Position::Left, "decimal&", "const decimal &in", &DecimalAddEq);
				VDecimal->SetOperatorEx(Operators::SubAssign, (uint32_t)Position::Left, "decimal&", "const decimal &in", &DecimalSubEq);
				VDecimal->SetOperatorEx(Operators::PreInc, (uint32_t)Position::Left, "decimal&", "", &DecimalFPP);
				VDecimal->SetOperatorEx(Operators::PreDec, (uint32_t)Position::Left, "decimal&", "", &DecimalFMM);
				VDecimal->SetOperatorEx(Operators::PostInc, (uint32_t)Position::Left, "decimal&", "", &DecimalPP);
				VDecimal->SetOperatorEx(Operators::PostDec, (uint32_t)Position::Left, "decimal&", "", &DecimalMM);
				VDecimal->SetOperatorEx(Operators::Equals, (uint32_t)Position::Const, "bool", "const decimal &in", &DecimalEq);
				VDecimal->SetOperatorEx(Operators::Cmp, (uint32_t)Position::Const, "int", "const decimal &in", &DecimalCmp);
				VDecimal->SetOperatorEx(Operators::Add, (uint32_t)Position::Const, "decimal", "const decimal &in", &DecimalAdd);
				VDecimal->SetOperatorEx(Operators::Sub, (uint32_t)Position::Const, "decimal", "const decimal &in", &DecimalSub);
				VDecimal->SetOperatorEx(Operators::Mul, (uint32_t)Position::Const, "decimal", "const decimal &in", &DecimalMul);
				VDecimal->SetOperatorEx(Operators::Div, (uint32_t)Position::Const, "decimal", "const decimal &in", &DecimalDiv);
				VDecimal->SetOperatorEx(Operators::Mod, (uint32_t)Position::Const, "decimal", "const decimal &in", &DecimalPer);
				VDecimal->SetMethodStatic("decimal nan()", &Core::Decimal::NaN);
				VDecimal->SetMethodStatic("decimal zero()", &Core::Decimal::Zero);

				return true;
			}
			bool Registry::ImportUInt128(VirtualMachine* VM) noexcept
			{
				VI_ASSERT(VM != nullptr, "manager should be set");

				auto VUInt128 = VM->SetStructTrivial<Compute::UInt128>("uint128", (size_t)ObjectBehaviours::APP_CLASS_ALLINTS);
				VUInt128->SetConstructor<Compute::UInt128>("void f()");
				VUInt128->SetConstructor<Compute::UInt128, int16_t>("void f(int16)");
				VUInt128->SetConstructor<Compute::UInt128, uint16_t>("void f(uint16)");
				VUInt128->SetConstructor<Compute::UInt128, int32_t>("void f(int32)");
				VUInt128->SetConstructor<Compute::UInt128, uint32_t>("void f(uint32)");
				VUInt128->SetConstructor<Compute::UInt128, int64_t>("void f(int64)");
				VUInt128->SetConstructor<Compute::UInt128, uint64_t>("void f(uint64)");
				VUInt128->SetConstructor<Compute::UInt128, const std::string_view&>("void f(const string_view&in)");
				VUInt128->SetConstructor<Compute::UInt128, const Compute::UInt128&>("void f(const uint128 &in)");
				VUInt128->SetMethodEx("int8 to_int8() const", &UInt128ToInt8);
				VUInt128->SetMethodEx("uint8 to_uint8() const", &UInt128ToUInt8);
				VUInt128->SetMethodEx("int16 to_int16() const", &UInt128ToInt16);
				VUInt128->SetMethodEx("uint16 to_uint16() const", &UInt128ToUInt16);
				VUInt128->SetMethodEx("int32 to_int32() const", &UInt128ToInt32);
				VUInt128->SetMethodEx("uint32 to_uint32() const", &UInt128ToUInt32);
				VUInt128->SetMethodEx("int64 to_int64() const", &UInt128ToInt64);
				VUInt128->SetMethodEx("uint64 to_uint64() const", &UInt128ToUInt64);
				VUInt128->SetMethod("decimal to_decimal() const", &Compute::UInt128::ToDecimal);
				VUInt128->SetMethod("string to_string(uint8 = 10, uint32 = 0) const", &Compute::UInt128::ToString);
				VUInt128->SetMethod<Compute::UInt128, const uint64_t&>("const uint64& low() const", &Compute::UInt128::Low);
				VUInt128->SetMethod<Compute::UInt128, const uint64_t&>("const uint64& high() const", &Compute::UInt128::High);
				VUInt128->SetMethod("uint16 size() const", &Compute::UInt128::Bits);
				VUInt128->SetOperatorEx(Operators::MulAssign, (uint32_t)Position::Left, "uint128&", "const uint128 &in", &UInt128MulEq);
				VUInt128->SetOperatorEx(Operators::DivAssign, (uint32_t)Position::Left, "uint128&", "const uint128 &in", &UInt128DivEq);
				VUInt128->SetOperatorEx(Operators::AddAssign, (uint32_t)Position::Left, "uint128&", "const uint128 &in", &UInt128AddEq);
				VUInt128->SetOperatorEx(Operators::SubAssign, (uint32_t)Position::Left, "uint128&", "const uint128 &in", &UInt128SubEq);
				VUInt128->SetOperatorEx(Operators::PreInc, (uint32_t)Position::Left, "uint128&", "", &UInt128FPP);
				VUInt128->SetOperatorEx(Operators::PreDec, (uint32_t)Position::Left, "uint128&", "", &UInt128FMM);
				VUInt128->SetOperatorEx(Operators::PostInc, (uint32_t)Position::Left, "uint128&", "", &UInt128PP);
				VUInt128->SetOperatorEx(Operators::PostDec, (uint32_t)Position::Left, "uint128&", "", &UInt128MM);
				VUInt128->SetOperatorEx(Operators::Equals, (uint32_t)Position::Const, "bool", "const uint128 &in", &UInt128Eq);
				VUInt128->SetOperatorEx(Operators::Cmp, (uint32_t)Position::Const, "int", "const uint128 &in", &UInt128Cmp);
				VUInt128->SetOperatorEx(Operators::Add, (uint32_t)Position::Const, "uint128", "const uint128 &in", &UInt128Add);
				VUInt128->SetOperatorEx(Operators::Sub, (uint32_t)Position::Const, "uint128", "const uint128 &in", &UInt128Sub);
				VUInt128->SetOperatorEx(Operators::Mul, (uint32_t)Position::Const, "uint128", "const uint128 &in", &UInt128Mul);
				VUInt128->SetOperatorEx(Operators::Div, (uint32_t)Position::Const, "uint128", "const uint128 &in", &UInt128Div);
				VUInt128->SetOperatorEx(Operators::Mod, (uint32_t)Position::Const, "uint128", "const uint128 &in", &UInt128Per);
				VUInt128->SetMethodStatic("uint128 min_value()", &Compute::UInt128::Min);
				VUInt128->SetMethodStatic("uint128 max_value()", &Compute::UInt128::Max);

				return true;
			}
			bool Registry::ImportUInt256(VirtualMachine* VM) noexcept
			{
				VI_ASSERT(VM != nullptr, "manager should be set");

				auto VUInt256 = VM->SetStructTrivial<Compute::UInt256>("uint256", (size_t)ObjectBehaviours::APP_CLASS_ALLINTS);
				VUInt256->SetConstructor<Compute::UInt256>("void f()");
				VUInt256->SetConstructor<Compute::UInt256, int16_t>("void f(int16)");
				VUInt256->SetConstructor<Compute::UInt256, uint16_t>("void f(uint16)");
				VUInt256->SetConstructor<Compute::UInt256, int32_t>("void f(int32)");
				VUInt256->SetConstructor<Compute::UInt256, uint32_t>("void f(uint32)");
				VUInt256->SetConstructor<Compute::UInt256, int64_t>("void f(int64)");
				VUInt256->SetConstructor<Compute::UInt256, uint64_t>("void f(uint64)");
				VUInt256->SetConstructor<Compute::UInt256, const Compute::UInt128&>("void f(const uint128&in)");
				VUInt256->SetConstructor<Compute::UInt256, const Compute::UInt128&, const Compute::UInt128&>("void f(const uint128&in, const uint128&in)");
				VUInt256->SetConstructor<Compute::UInt256, const std::string_view&>("void f(const string_view&in)");
				VUInt256->SetConstructor<Compute::UInt256, const Compute::UInt256&>("void f(const uint256 &in)");
				VUInt256->SetMethodEx("int8 to_int8() const", &UInt256ToInt8);
				VUInt256->SetMethodEx("uint8 to_uint8() const", &UInt256ToUInt8);
				VUInt256->SetMethodEx("int16 to_int16() const", &UInt256ToInt16);
				VUInt256->SetMethodEx("uint16 to_uint16() const", &UInt256ToUInt16);
				VUInt256->SetMethodEx("int32 to_int32() const", &UInt256ToInt32);
				VUInt256->SetMethodEx("uint32 to_uint32() const", &UInt256ToUInt32);
				VUInt256->SetMethodEx("int64 to_int64() const", &UInt256ToInt64);
				VUInt256->SetMethodEx("uint64 to_uint64() const", &UInt256ToUInt64);
				VUInt256->SetMethod("decimal to_decimal() const", &Compute::UInt256::ToDecimal);
				VUInt256->SetMethod("string to_string(uint8 = 10, uint32 = 0) const", &Compute::UInt256::ToString);
				VUInt256->SetMethod<Compute::UInt256, const Compute::UInt128&>("const uint128& low() const", &Compute::UInt256::Low);
				VUInt256->SetMethod<Compute::UInt256, const Compute::UInt128&>("const uint128& high() const", &Compute::UInt256::High);
				VUInt256->SetMethod("uint16 size() const", &Compute::UInt256::Bits);
				VUInt256->SetOperatorEx(Operators::MulAssign, (uint32_t)Position::Left, "uint256&", "const uint256 &in", &UInt256MulEq);
				VUInt256->SetOperatorEx(Operators::DivAssign, (uint32_t)Position::Left, "uint256&", "const uint256 &in", &UInt256DivEq);
				VUInt256->SetOperatorEx(Operators::AddAssign, (uint32_t)Position::Left, "uint256&", "const uint256 &in", &UInt256AddEq);
				VUInt256->SetOperatorEx(Operators::SubAssign, (uint32_t)Position::Left, "uint256&", "const uint256 &in", &UInt256SubEq);
				VUInt256->SetOperatorEx(Operators::PreInc, (uint32_t)Position::Left, "uint256&", "", &UInt256FPP);
				VUInt256->SetOperatorEx(Operators::PreDec, (uint32_t)Position::Left, "uint256&", "", &UInt256FMM);
				VUInt256->SetOperatorEx(Operators::PostInc, (uint32_t)Position::Left, "uint256&", "", &UInt256PP);
				VUInt256->SetOperatorEx(Operators::PostDec, (uint32_t)Position::Left, "uint256&", "", &UInt256MM);
				VUInt256->SetOperatorEx(Operators::Equals, (uint32_t)Position::Const, "bool", "const uint256 &in", &UInt256Eq);
				VUInt256->SetOperatorEx(Operators::Cmp, (uint32_t)Position::Const, "int", "const uint256 &in", &UInt256Cmp);
				VUInt256->SetOperatorEx(Operators::Add, (uint32_t)Position::Const, "uint256", "const uint256 &in", &UInt256Add);
				VUInt256->SetOperatorEx(Operators::Sub, (uint32_t)Position::Const, "uint256", "const uint256 &in", &UInt256Sub);
				VUInt256->SetOperatorEx(Operators::Mul, (uint32_t)Position::Const, "uint256", "const uint256 &in", &UInt256Mul);
				VUInt256->SetOperatorEx(Operators::Div, (uint32_t)Position::Const, "uint256", "const uint256 &in", &UInt256Div);
				VUInt256->SetOperatorEx(Operators::Mod, (uint32_t)Position::Const, "uint256", "const uint256 &in", &UInt256Per);
				VUInt256->SetMethodStatic("uint256 min_value()", &Compute::UInt256::Min);
				VUInt256->SetMethodStatic("uint256 max_value()", &Compute::UInt256::Max);

				return true;
			}
			bool Registry::ImportVariant(VirtualMachine* VM) noexcept
			{
				VI_ASSERT(VM != nullptr, "manager should be set");
				auto VVarType = VM->SetEnum("var_type");
				VVarType->SetValue("null_t", (int)Core::VarType::Null);
				VVarType->SetValue("undefined_t", (int)Core::VarType::Undefined);
				VVarType->SetValue("object_t", (int)Core::VarType::Object);
				VVarType->SetValue("array_t", (int)Core::VarType::Array);
				VVarType->SetValue("string_t", (int)Core::VarType::String);
				VVarType->SetValue("binary_t", (int)Core::VarType::Binary);
				VVarType->SetValue("integer_t", (int)Core::VarType::Integer);
				VVarType->SetValue("number_t", (int)Core::VarType::Number);
				VVarType->SetValue("decimal_t", (int)Core::VarType::Decimal);
				VVarType->SetValue("boolean_t", (int)Core::VarType::Boolean);

				auto VVariant = VM->SetStructTrivial<Core::Variant>("variant");
				VVariant->SetConstructor<Core::Variant>("void f()");
				VVariant->SetConstructor<Core::Variant, const Core::Variant&>("void f(const variant &in)");
				VVariant->SetMethod("bool deserialize(const string_view&in, bool = false)", &Core::Variant::Deserialize);
				VVariant->SetMethod("string serialize() const", &Core::Variant::Serialize);
				VVariant->SetMethod("decimal to_decimal() const", &Core::Variant::GetDecimal);
				VVariant->SetMethod("string to_string() const", &Core::Variant::GetBlob);
				VVariant->SetMethod("uptr@ to_pointer() const", &Core::Variant::GetPointer);
				VVariant->SetMethodEx("int8 to_int8() const", &VariantToInt8);
				VVariant->SetMethodEx("uint8 to_uint8() const", &VariantToUInt8);
				VVariant->SetMethodEx("int16 to_int16() const", &VariantToInt16);
				VVariant->SetMethodEx("uint16 to_uint16() const", &VariantToUInt16);
				VVariant->SetMethodEx("int32 to_int32() const", &VariantToInt32);
				VVariant->SetMethodEx("uint32 to_uint32() const", &VariantToUInt32);
				VVariant->SetMethodEx("int64 to_int64() const", &VariantToInt64);
				VVariant->SetMethodEx("uint64 to_uint64() const", &VariantToUInt64);
				VVariant->SetMethod("double to_number() const", &Core::Variant::GetNumber);
				VVariant->SetMethod("bool to_boolean() const", &Core::Variant::GetBoolean);
				VVariant->SetMethod("var_type get_type() const", &Core::Variant::GetType);
				VVariant->SetMethod("bool is_object() const", &Core::Variant::IsObject);
				VVariant->SetMethod("bool empty() const", &Core::Variant::Empty);
				VVariant->SetMethodEx("usize size() const", &VariantGetSize);
				VVariant->SetOperatorEx(Operators::Equals, (uint32_t)Position::Const, "bool", "const variant &in", &VariantEquals);
				VVariant->SetOperatorEx(Operators::ImplCast, (uint32_t)Position::Const, "bool", "", &VariantImplCast);

				VM->BeginNamespace("var");
				VM->SetFunction("variant auto_t(const string_view&in, bool = false)", &Core::Var::Auto);
				VM->SetFunction("variant null_t()", &Core::Var::Null);
				VM->SetFunction("variant undefined_t()", &Core::Var::Undefined);
				VM->SetFunction("variant object_t()", &Core::Var::Object);
				VM->SetFunction("variant array_t()", &Core::Var::Array);
				VM->SetFunction("variant pointer_t(uptr@)", &Core::Var::Pointer);
				VM->SetFunction("variant integer_t(int64)", &Core::Var::Integer);
				VM->SetFunction("variant number_t(double)", &Core::Var::Number);
				VM->SetFunction("variant boolean_t(bool)", &Core::Var::Boolean);
				VM->SetFunction<Core::Variant(const std::string_view&)>("variant string_t(const string_view&in)", &Core::Var::String);
				VM->SetFunction<Core::Variant(const std::string_view&)>("variant binary_t(const string_view&in)", &Core::Var::Binary);
				VM->SetFunction<Core::Variant(const std::string_view&)>("variant decimal_t(const string_view&in)", &Core::Var::DecimalString);
				VM->SetFunction<Core::Variant(const Core::Decimal&)>("variant decimal_t(const decimal &in)", &Core::Var::Decimal);
				VM->EndNamespace();

				return true;
			}
			bool Registry::ImportTimestamp(VirtualMachine* VM) noexcept
			{
				VI_ASSERT(VM != nullptr, "manager should be set");
				auto VDateTime = VM->SetStructTrivial<Core::DateTime>("timestamp");
				VDateTime->SetConstructor<Core::DateTime>("void f()");
				VDateTime->SetConstructor<Core::DateTime, const Core::DateTime&>("void f(const timestamp&in)");
				VDateTime->SetMethod("timestamp& apply_offset(bool = false)", &Core::DateTime::ApplyOffset);
				VDateTime->SetMethod("timestamp& apply_timepoint(bool = false)", &Core::DateTime::ApplyTimepoint);
				VDateTime->SetMethod("timestamp& use_global_time()", &Core::DateTime::UseGlobalTime);
				VDateTime->SetMethod("timestamp& use_local_time()", &Core::DateTime::UseLocalTime);
				VDateTime->SetMethod("timestamp& set_second(uint8)", &Core::DateTime::SetSecond);
				VDateTime->SetMethod("timestamp& set_minute(uint8)", &Core::DateTime::SetMinute);
				VDateTime->SetMethod("timestamp& set_hour(uint8)", &Core::DateTime::SetHour);
				VDateTime->SetMethod("timestamp& set_day(uint8)", &Core::DateTime::SetDay);
				VDateTime->SetMethod("timestamp& set_week(uint8)", &Core::DateTime::SetWeek);
				VDateTime->SetMethod("timestamp& set_month(uint8)", &Core::DateTime::SetMonth);
				VDateTime->SetMethod("timestamp& set_year(uint32)", &Core::DateTime::SetYear);
				VDateTime->SetMethod("string serialize(const string_view&in)", &Core::DateTime::Serialize);
				VDateTime->SetMethod("uint8 second() const", &Core::DateTime::Second);
				VDateTime->SetMethod("uint8 minute() const", &Core::DateTime::Minute);
				VDateTime->SetMethod("uint8 hour() const", &Core::DateTime::Hour);
				VDateTime->SetMethod("uint8 day() const", &Core::DateTime::Day);
				VDateTime->SetMethod("uint8 week() const", &Core::DateTime::Week);
				VDateTime->SetMethod("uint8 month() const", &Core::DateTime::Month);
				VDateTime->SetMethod("uint32 year() const", &Core::DateTime::Year);
				VDateTime->SetMethod("int64 nanoseconds() const", &Core::DateTime::Nanoseconds);
				VDateTime->SetMethod("int64 microseconds() const", &Core::DateTime::Microseconds);
				VDateTime->SetMethod("int64 milliseconds() const", &Core::DateTime::Milliseconds);
				VDateTime->SetMethod("int64 seconds() const", &Core::DateTime::Seconds);
				VDateTime->SetOperatorEx(Operators::AddAssign, (uint32_t)Position::Left, "timestamp&", "const timestamp &in", &DateTimeAddEq);
				VDateTime->SetOperatorEx(Operators::SubAssign, (uint32_t)Position::Left, "timestamp&", "const timestamp &in", &DateTimeSubEq);
				VDateTime->SetOperatorEx(Operators::Equals, (uint32_t)Position::Const, "bool", "const timestamp &in", &DateTimeEq);
				VDateTime->SetOperatorEx(Operators::Cmp, (uint32_t)Position::Const, "int", "const timestamp &in", &DateTimeCmp);
				VDateTime->SetOperatorEx(Operators::Add, (uint32_t)Position::Const, "timestamp", "const timestamp &in", &DateTimeAdd);
				VDateTime->SetOperatorEx(Operators::Sub, (uint32_t)Position::Const, "timestamp", "const timestamp &in", &DateTimeSub);
				VDateTime->SetMethodStatic("timestamp from_nanoseconds(int64)", &Core::DateTime::FromNanoseconds);
				VDateTime->SetMethodStatic("timestamp from_microseconds(int64)", &Core::DateTime::FromMicroseconds);
				VDateTime->SetMethodStatic("timestamp from_milliseconds(int64)", &Core::DateTime::FromMilliseconds);
				VDateTime->SetMethodStatic("timestamp from_seconds(int64)", &Core::DateTime::FromSeconds);
				VDateTime->SetMethodStatic("timestamp from_serialized(const string_view&in, const string_view&in)", &Core::DateTime::FromSerialized);
				VDateTime->SetMethodStatic("string_view format_iso8601_time()", &Core::DateTime::FormatIso8601Time);
				VDateTime->SetMethodStatic("string_view format_web_time()", &Core::DateTime::FormatWebTime);
				VDateTime->SetMethodStatic("string_view format_web_local_time()", &Core::DateTime::FormatWebLocalTime);
				VDateTime->SetMethodStatic("string_view format_compact_time()", &Core::DateTime::FormatCompactTime);
				return true;
			}
			bool Registry::ImportConsole(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr, "manager should be set");
				auto VStdColor = VM->SetEnum("std_color");
				VStdColor->SetValue("black", (int)Core::StdColor::Black);
				VStdColor->SetValue("dark_blue", (int)Core::StdColor::DarkBlue);
				VStdColor->SetValue("dark_green", (int)Core::StdColor::DarkGreen);
				VStdColor->SetValue("light_blue", (int)Core::StdColor::LightBlue);
				VStdColor->SetValue("dark_red", (int)Core::StdColor::DarkRed);
				VStdColor->SetValue("magenta", (int)Core::StdColor::Magenta);
				VStdColor->SetValue("orange", (int)Core::StdColor::Orange);
				VStdColor->SetValue("light_gray", (int)Core::StdColor::LightGray);
				VStdColor->SetValue("gray", (int)Core::StdColor::Gray);
				VStdColor->SetValue("blue", (int)Core::StdColor::Blue);
				VStdColor->SetValue("green", (int)Core::StdColor::Green);
				VStdColor->SetValue("cyan", (int)Core::StdColor::Cyan);
				VStdColor->SetValue("red", (int)Core::StdColor::Red);
				VStdColor->SetValue("pink", (int)Core::StdColor::Pink);
				VStdColor->SetValue("yellow", (int)Core::StdColor::Yellow);
				VStdColor->SetValue("white", (int)Core::StdColor::White);
				VStdColor->SetValue("zero", (int)Core::StdColor::Zero);

				auto VConsole = VM->SetClass<Core::Console>("console", false);
				VConsole->SetMethod("void hide()", &Core::Console::Hide);
				VConsole->SetMethod("void show()", &Core::Console::Show);
				VConsole->SetMethod("void clear()", &Core::Console::Clear);
				VConsole->SetMethod("void allocate()", &Core::Console::Allocate);
				VConsole->SetMethod("void deallocate()", &Core::Console::Deallocate);
				VConsole->SetMethod("void attach()", &Core::Console::Attach);
				VConsole->SetMethod("void detach()", &Core::Console::Detach);
				VConsole->SetMethod("void set_coloring(bool)", &Core::Console::SetColoring);
				VConsole->SetMethod("void color_begin(std_color, std_color = std_color::zero)", &Core::Console::ColorBegin);
				VConsole->SetMethod("void color_end()", &Core::Console::ColorEnd);
				VConsole->SetMethod("void capture_time()", &Core::Console::CaptureTime);
				VConsole->SetMethod("uint64 capture_window(uint32)", &Core::Console::CaptureWindow);
				VConsole->SetMethod("void free_window(uint64, bool)", &Core::Console::FreeWindow);
				VConsole->SetMethod("void emplace_window(uint64, const string_view&in)", &Core::Console::EmplaceWindow);
				VConsole->SetMethod("uint64 capture_element()", &Core::Console::CaptureElement);
				VConsole->SetMethod("void free_element(uint64)", &Core::Console::FreeElement);
				VConsole->SetMethod("void resize_element(uint64, uint32)", &Core::Console::ResizeElement);
				VConsole->SetMethod("void move_element(uint64, uint32)", &Core::Console::MoveElement);
				VConsole->SetMethod("void read_element(uint64, uint32&out, uint32&out)", &Core::Console::ReadElement);
				VConsole->SetMethod("void replace_element(uint64, const string_view&in)", &Core::Console::ReplaceElement);
				VConsole->SetMethod("void spinning_element(uint64, const string_view&in)", &Core::Console::SpinningElement);
				VConsole->SetMethod("void progress_element(uint64, double, double = 0.8)", &Core::Console::ProgressElement);
				VConsole->SetMethod("void spinning_progress_element(uint64, double, double = 0.8)", &Core::Console::SpinningProgressElement);
				VConsole->SetMethod("void clear_element(uint64)", &Core::Console::ClearElement);
				VConsole->SetMethod("void flush()", &Core::Console::Flush);
				VConsole->SetMethod("void flush_write()", &Core::Console::FlushWrite);
				VConsole->SetMethod("void write_size(uint32, uint32)", &Core::Console::WriteSize);
				VConsole->SetMethod("void write_position(uint32, uint32)", &Core::Console::WritePosition);
				VConsole->SetMethod("void write_line(const string_view&in)", &Core::Console::WriteLine);
				VConsole->SetMethod("void write_char(uint8)", &Core::Console::WriteChar);
				VConsole->SetMethod("void write(const string_view&in)", &Core::Console::Write);
				VConsole->SetMethod("double get_captured_time()", &Core::Console::GetCapturedTime);
				VConsole->SetMethod("string read(usize)", &Core::Console::Read);
				VConsole->SetMethod("bool read_screen(uint32 &out, uint32 &out, uint32 &out, uint32 &out)", &Core::Console::ReadScreen);
				VConsole->SetMethod("bool read_line(string&out, usize)", &Core::Console::ReadLine);
				VConsole->SetMethod("uint8 read_char()", &Core::Console::ReadChar);
				VConsole->SetMethodStatic("console@+ get()", &Core::Console::Get);
				VConsole->SetMethodEx("void trace(uint32 = 32)", &ConsoleTrace);

				return true;
#else
				VI_ASSERT(false, "<console> is not loaded");
				return false;
#endif
			}
			bool Registry::ImportSchema(VirtualMachine* VM) noexcept
			{
				VI_ASSERT(VM != nullptr, "manager should be set");
				VI_TYPEREF(Schema, "schema");

				auto VSchema = VM->SetClass<Core::Schema>("schema", true);
				VSchema->SetProperty<Core::Schema>("string key", &Core::Schema::Key);
				VSchema->SetProperty<Core::Schema>("variant value", &Core::Schema::Value);
				VSchema->SetConstructorEx<Core::Schema, Core::Schema*>("schema@ f(schema@+)", &SchemaConstructCopy);
				VSchema->SetGcConstructor<Core::Schema, Schema, const Core::Variant&>("schema@ f(const variant &in)");
				VSchema->SetGcConstructorListEx<Core::Schema>("schema@ f(int &in) { repeat { string, ? } }", &SchemaConstruct);
				VSchema->SetMethod<Core::Schema, Core::Variant, size_t>("variant get_var(usize) const", &Core::Schema::GetVar);
				VSchema->SetMethod<Core::Schema, Core::Variant, const std::string_view&>("variant get_var(const string_view&in) const", &Core::Schema::GetVar);
				VSchema->SetMethod<Core::Schema, Core::Variant, const std::string_view&>("variant get_attribute_var(const string_view&in) const", &Core::Schema::GetAttributeVar);
				VSchema->SetMethod("schema@+ get_parent() const", &Core::Schema::GetParent);
				VSchema->SetMethod("schema@+ get_attribute(const string_view&in) const", &Core::Schema::GetAttribute);
				VSchema->SetMethod<Core::Schema, Core::Schema*, size_t>("schema@+ get(usize) const", &Core::Schema::Get);
				VSchema->SetMethod<Core::Schema, Core::Schema*, const std::string_view&, bool>("schema@+ get(const string_view&in, bool = false) const", &Core::Schema::Fetch);
				VSchema->SetMethod<Core::Schema, Core::Schema*, const std::string_view&>("schema@+ set(const string_view&in)", &Core::Schema::Set);
				VSchema->SetMethod<Core::Schema, Core::Schema*, const std::string_view&, const Core::Variant&>("schema@+ set(const string_view&in, const variant &in)", &Core::Schema::Set);
				VSchema->SetMethod<Core::Schema, Core::Schema*, const std::string_view&, const Core::Variant&>("schema@+ set_attribute(const string& in, const variant &in)", &Core::Schema::SetAttribute);
				VSchema->SetMethod<Core::Schema, Core::Schema*, const Core::Variant&>("schema@+ push(const variant &in)", &Core::Schema::Push);
				VSchema->SetMethod<Core::Schema, Core::Schema*, size_t>("schema@+ pop(usize)", &Core::Schema::Pop);
				VSchema->SetMethod<Core::Schema, Core::Schema*, const std::string_view&>("schema@+ pop(const string_view&in)", &Core::Schema::Pop);
				VSchema->SetMethod("schema@ copy() const", &Core::Schema::Copy);
				VSchema->SetMethod("bool has(const string_view&in) const", &Core::Schema::Has);
				VSchema->SetMethod("bool has_attribute(const string_view&in) const", &Core::Schema::HasAttribute);
				VSchema->SetMethod("bool empty() const", &Core::Schema::Empty);
				VSchema->SetMethod("bool is_attribute() const", &Core::Schema::IsAttribute);
				VSchema->SetMethod("bool is_saved() const", &Core::Schema::IsAttribute);
				VSchema->SetMethod("string get_name() const", &Core::Schema::GetName);
				VSchema->SetMethod("void join(schema@+, bool)", &Core::Schema::Join);
				VSchema->SetMethod("void unlink()", &Core::Schema::Unlink);
				VSchema->SetMethod("void clear()", &Core::Schema::Clear);
				VSchema->SetMethod("void save()", &Core::Schema::Save);
				VSchema->SetMethodEx("variant first_var() const", &SchemaFirstVar);
				VSchema->SetMethodEx("variant last_var() const", &SchemaLastVar);
				VSchema->SetMethodEx("schema@+ first() const", &SchemaFirst);
				VSchema->SetMethodEx("schema@+ last() const", &SchemaLast);
				VSchema->SetMethodEx("schema@+ set(const string_view&in, schema@+)", &SchemaSet);
				VSchema->SetMethodEx("schema@+ push(schema@+)", &SchemaPush);
				VSchema->SetMethodEx("array<schema@>@ get_collection(const string_view&in, bool = false) const", &SchemaGetCollection);
				VSchema->SetMethodEx("array<schema@>@ get_attributes() const", &SchemaGetAttributes);
				VSchema->SetMethodEx("array<schema@>@ get_childs() const", &SchemaGetChilds);
				VSchema->SetMethodEx("dictionary@ get_names() const", &SchemaGetNames);
				VSchema->SetMethodEx("usize size() const", &SchemaGetSize);
				VSchema->SetMethodEx("string to_json() const", &SchemaToJSON);
				VSchema->SetMethodEx("string to_xml() const", &SchemaToXML);
				VSchema->SetMethodEx("string to_string() const", &SchemaToString);
				VSchema->SetMethodEx("string to_binary() const", &SchemaToBinary);
				VSchema->SetMethodEx("int8 to_int8() const", &SchemaToInt8);
				VSchema->SetMethodEx("uint8 to_uint8() const", &SchemaToUInt8);
				VSchema->SetMethodEx("int16 to_int16() const", &SchemaToInt16);
				VSchema->SetMethodEx("uint16 to_uint16() const", &SchemaToUInt16);
				VSchema->SetMethodEx("int32 to_int32() const", &SchemaToInt32);
				VSchema->SetMethodEx("uint32 to_uint32() const", &SchemaToUInt32);
				VSchema->SetMethodEx("int64 to_int64() const", &SchemaToInt64);
				VSchema->SetMethodEx("uint64 to_uint64() const", &SchemaToUInt64);
				VSchema->SetMethodEx("double to_number() const", &SchemaToNumber);
				VSchema->SetMethodEx("decimal to_decimal() const", &SchemaToDecimal);
				VSchema->SetMethodEx("bool to_boolean() const", &SchemaToBoolean);
				VSchema->SetMethodStatic("schema@ from_json(const string_view&in)", &SchemaFromJSON);
				VSchema->SetMethodStatic("schema@ from_xml(const string_view&in)", &SchemaFromXML);
				VSchema->SetMethodStatic("schema@ import_json(const string_view&in)", &SchemaImport);
				VSchema->SetOperatorEx(Operators::Assign, (uint32_t)Position::Left, "schema@+", "const variant &in", &SchemaCopyAssign1);
				VSchema->SetOperatorEx(Operators::Assign, (uint32_t)Position::Left, "schema@+", "schema@+", &SchemaCopyAssign2);
				VSchema->SetOperatorEx(Operators::Equals, (uint32_t)(Position::Left | Position::Const), "bool", "schema@+", &SchemaEquals);
				VSchema->SetOperatorEx(Operators::Index, (uint32_t)Position::Left, "schema@+", "const string_view&in", &SchemaGetIndex);
				VSchema->SetOperatorEx(Operators::Index, (uint32_t)Position::Left, "schema@+", "usize", &SchemaGetIndexOffset);
				VSchema->SetEnumRefsEx<Core::Schema>([](Core::Schema* Base, asIScriptEngine*) { });
				VSchema->SetReleaseRefsEx<Core::Schema>([](Core::Schema* Base, asIScriptEngine*) { });

				VM->BeginNamespace("var::set");
				VM->SetFunction("schema@ auto_t(const string_view&in, bool = false)", &Core::Var::Auto);
				VM->SetFunction("schema@ null_t()", &Core::Var::Set::Null);
				VM->SetFunction("schema@ undefined_t()", &Core::Var::Set::Undefined);
				VM->SetFunction("schema@ object_t()", &Core::Var::Set::Object);
				VM->SetFunction("schema@ array_t()", &Core::Var::Set::Array);
				VM->SetFunction("schema@ pointer_t(uptr@)", &Core::Var::Set::Pointer);
				VM->SetFunction("schema@ integer_t(int64)", &Core::Var::Set::Integer);
				VM->SetFunction("schema@ number_t(double)", &Core::Var::Set::Number);
				VM->SetFunction("schema@ boolean_t(bool)", &Core::Var::Set::Boolean);
				VM->SetFunction<Core::Schema* (const std::string_view&)>("schema@ string_t(const string_view&in)", &Core::Var::Set::String);
				VM->SetFunction<Core::Schema* (const std::string_view&)>("schema@ binary_t(const string_view&in)", &Core::Var::Set::Binary);
				VM->SetFunction<Core::Schema* (const std::string_view&)>("schema@ decimal_t(const string_view&in)", &Core::Var::Set::DecimalString);
				VM->SetFunction<Core::Schema* (const Core::Decimal&)>("schema@ decimal_t(const decimal &in)", &Core::Var::Set::Decimal);
				VM->SetFunction("schema@+ as(schema@+)", &SchemaInit);
				VM->EndNamespace();

				return true;
			}
			bool Registry::ImportClockTimer(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr, "manager should be set");

				auto VTimer = VM->SetClass<Core::Timer>("clock_timer", false);
				VTimer->SetConstructor<Core::Timer>("clock_timer@ f()");
				VTimer->SetMethod("void set_fixed_frames(float)", &Core::Timer::SetFixedFrames);
				VTimer->SetMethod("void begin()", &Core::Timer::Begin);
				VTimer->SetMethod("void finish()", &Core::Timer::Finish);
				VTimer->SetMethod("usize get_frame_index() const", &Core::Timer::GetFrameIndex);
				VTimer->SetMethod("usize get_fixed_frame_index() const", &Core::Timer::GetFixedFrameIndex);
				VTimer->SetMethod("float get_max_frames() const", &Core::Timer::GetMaxFrames);
				VTimer->SetMethod("float get_min_step() const", &Core::Timer::GetMinStep);
				VTimer->SetMethod("float get_frames() const", &Core::Timer::GetFrames);
				VTimer->SetMethod("float get_elapsed() const", &Core::Timer::GetElapsed);
				VTimer->SetMethod("float get_elapsed_mills() const", &Core::Timer::GetElapsedMills);
				VTimer->SetMethod("float get_step() const", &Core::Timer::GetStep);
				VTimer->SetMethod("float get_fixed_step() const", &Core::Timer::GetFixedStep);
				VTimer->SetMethod("bool is_fixed() const", &Core::Timer::IsFixed);

				return true;
#else
				VI_ASSERT(false, "<clock_timer> is not loaded");
				return false;
#endif
			}
			bool Registry::ImportFileSystem(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr, "manager should be set");
				auto VFileMode = VM->SetEnum("file_mode");
				VFileMode->SetValue("read_only", (int)Core::FileMode::Read_Only);
				VFileMode->SetValue("write_only", (int)Core::FileMode::Write_Only);
				VFileMode->SetValue("append_only", (int)Core::FileMode::Append_Only);
				VFileMode->SetValue("read_write", (int)Core::FileMode::Read_Write);
				VFileMode->SetValue("write_read", (int)Core::FileMode::Write_Read);
				VFileMode->SetValue("read_append_write", (int)Core::FileMode::Read_Append_Write);
				VFileMode->SetValue("binary_read_only", (int)Core::FileMode::Binary_Read_Only);
				VFileMode->SetValue("binary_write_only", (int)Core::FileMode::Binary_Write_Only);
				VFileMode->SetValue("binary_append_only", (int)Core::FileMode::Binary_Append_Only);
				VFileMode->SetValue("binary_read_write", (int)Core::FileMode::Binary_Read_Write);
				VFileMode->SetValue("binary_write_read", (int)Core::FileMode::Binary_Write_Read);
				VFileMode->SetValue("binary_read_append_write", (int)Core::FileMode::Binary_Read_Append_Write);

				auto VFileSeek = VM->SetEnum("file_seek");
				VFileSeek->SetValue("begin", (int)Core::FileSeek::Begin);
				VFileSeek->SetValue("current", (int)Core::FileSeek::Current);
				VFileSeek->SetValue("end", (int)Core::FileSeek::End);

				auto VFileState = VM->SetPod<Core::FileState>("file_state");
				VFileState->SetProperty("usize size", &Core::FileState::Size);
				VFileState->SetProperty("usize links", &Core::FileState::Links);
				VFileState->SetProperty("usize permissions", &Core::FileState::Permissions);
				VFileState->SetProperty("usize document", &Core::FileState::Document);
				VFileState->SetProperty("usize device", &Core::FileState::Device);
				VFileState->SetProperty("usize userId", &Core::FileState::UserId);
				VFileState->SetProperty("usize groupId", &Core::FileState::GroupId);
				VFileState->SetProperty("int64 last_access", &Core::FileState::LastAccess);
				VFileState->SetProperty("int64 last_permission_change", &Core::FileState::LastPermissionChange);
				VFileState->SetProperty("int64 last_modified", &Core::FileState::LastModified);
				VFileState->SetProperty("bool exists", &Core::FileState::Exists);
				VFileState->SetConstructor<Core::FileState>("void f()");

				auto VFileEntry = VM->SetStructTrivial<Core::FileEntry>("file_entry");
				VFileEntry->SetConstructor<Core::FileEntry>("void f()");
				VFileEntry->SetProperty("usize size", &Core::FileEntry::Size);
				VFileEntry->SetProperty("int64 last_modified", &Core::FileEntry::LastModified);
				VFileEntry->SetProperty("int64 creation_time", &Core::FileEntry::CreationTime);
				VFileEntry->SetProperty("bool is_referenced", &Core::FileEntry::IsReferenced);
				VFileEntry->SetProperty("bool is_directory", &Core::FileEntry::IsDirectory);
				VFileEntry->SetProperty("bool is_exists", &Core::FileEntry::IsExists);

				auto VFileLink = VM->SetStructTrivial<FileLink>("file_link");
				VFileLink->SetConstructor<FileLink>("void f()");
				VFileLink->SetProperty("string path", &FileLink::Path);
				VFileLink->SetProperty("usize size", &FileLink::Size);
				VFileLink->SetProperty("int64 last_modified", &FileLink::LastModified);
				VFileLink->SetProperty("int64 creation_time", &FileLink::CreationTime);
				VFileLink->SetProperty("bool is_referenced", &FileLink::IsReferenced);
				VFileLink->SetProperty("bool is_directory", &FileLink::IsDirectory);
				VFileLink->SetProperty("bool is_exists", &FileLink::IsExists);

				auto VStream = VM->SetClass<Core::Stream>("base_stream", false);
				VStream->SetMethodEx("bool clear()", &VI_EXPECTIFY_VOID(Core::Stream::Clear));
				VStream->SetMethodEx("bool close()", &VI_EXPECTIFY_VOID(Core::Stream::Close));
				VStream->SetMethodEx("bool seek(file_seek, int64)", &VI_EXPECTIFY_VOID(Core::Stream::Seek));
				VStream->SetMethodEx("bool move(int64)", &VI_EXPECTIFY_VOID(Core::Stream::Move));
				VStream->SetMethodEx("bool flush()", &VI_EXPECTIFY_VOID(Core::Stream::Flush));
				VStream->SetMethodEx("usize size()", &VI_EXPECTIFY(Core::Stream::Size));
				VStream->SetMethodEx("usize tell()", &VI_EXPECTIFY(Core::Stream::Tell));
				VStream->SetMethod("usize virtual_size() const", &Core::Stream::VirtualSize);
				VStream->SetMethod("const string& virtual_name() const", &Core::Stream::VirtualName);
				VStream->SetMethod("void set_virtual_size(usize) const", &Core::Stream::SetVirtualSize);
				VStream->SetMethod("void set_virtual_name(const string_view&in) const", &Core::Stream::SetVirtualName);
				VStream->SetMethod("int32 get_readable_fd() const", &Core::Stream::GetReadableFd);
				VStream->SetMethod("int32 get_writeable_fd() const", &Core::Stream::GetWriteableFd);
				VStream->SetMethod("uptr@ get_readable() const", &Core::Stream::GetReadable);
				VStream->SetMethod("uptr@ get_writeable() const", &Core::Stream::GetWriteable);
				VStream->SetMethod("bool is_sized() const", &Core::Stream::IsSized);
				VStream->SetMethodEx("bool open(const string_view&in, file_mode)", &StreamOpen);
				VStream->SetMethodEx("usize write(const string_view&in)", &StreamWrite);
				VStream->SetMethodEx("string read(usize)", &StreamRead);
				VStream->SetMethodEx("string read_line(usize)", &StreamReadLine);

				auto VMemoryStream = VM->SetClass<Core::MemoryStream>("memory_stream", false);
				VMemoryStream->SetConstructor<Core::MemoryStream>("memory_stream@ f()");
				VMemoryStream->SetMethodEx("bool clear()", &VI_EXPECTIFY_VOID(Core::MemoryStream::Clear));
				VMemoryStream->SetMethodEx("bool close()", &VI_EXPECTIFY_VOID(Core::MemoryStream::Close));
				VMemoryStream->SetMethodEx("bool seek(file_seek, int64)", &VI_EXPECTIFY_VOID(Core::MemoryStream::Seek));
				VMemoryStream->SetMethodEx("bool move(int64)", &VI_EXPECTIFY_VOID(Core::MemoryStream::Move));
				VMemoryStream->SetMethodEx("bool flush()", &VI_EXPECTIFY_VOID(Core::MemoryStream::Flush));
				VMemoryStream->SetMethodEx("usize size()", &VI_EXPECTIFY(Core::MemoryStream::Size));
				VMemoryStream->SetMethodEx("usize tell()", &VI_EXPECTIFY(Core::MemoryStream::Tell));
				VMemoryStream->SetMethod("usize virtual_size() const", &Core::MemoryStream::VirtualSize);
				VMemoryStream->SetMethod("const string& virtual_name() const", &Core::MemoryStream::VirtualName);
				VMemoryStream->SetMethod("void set_virtual_size(usize) const", &Core::MemoryStream::SetVirtualSize);
				VMemoryStream->SetMethod("void set_virtual_name(const string_view&in) const", &Core::MemoryStream::SetVirtualName);
				VMemoryStream->SetMethod("int32 get_readable_fd() const", &Core::MemoryStream::GetReadableFd);
				VMemoryStream->SetMethod("int32 get_writeable_fd() const", &Core::MemoryStream::GetWriteableFd);
				VMemoryStream->SetMethod("uptr@ get_readable() const", &Core::MemoryStream::GetReadable);
				VMemoryStream->SetMethod("uptr@ get_writeable() const", &Core::MemoryStream::GetWriteable);
				VMemoryStream->SetMethod("bool is_sized() const", &Core::MemoryStream::IsSized);
				VMemoryStream->SetMethodEx("bool open(const string_view&in, file_mode)", &StreamOpen);
				VMemoryStream->SetMethodEx("usize write(const string_view&in)", &StreamWrite);
				VMemoryStream->SetMethodEx("string read(usize)", &StreamRead);
				VMemoryStream->SetMethodEx("string read_line(usize)", &StreamReadLine);

				auto VFileStream = VM->SetClass<Core::FileStream>("file_stream", false);
				VFileStream->SetConstructor<Core::FileStream>("file_stream@ f()");
				VFileStream->SetMethodEx("bool clear()", &VI_EXPECTIFY_VOID(Core::FileStream::Clear));
				VFileStream->SetMethodEx("bool close()", &VI_EXPECTIFY_VOID(Core::FileStream::Close));
				VFileStream->SetMethodEx("bool seek(file_seek, int64)", &VI_EXPECTIFY_VOID(Core::FileStream::Seek));
				VFileStream->SetMethodEx("bool move(int64)", &VI_EXPECTIFY_VOID(Core::FileStream::Move));
				VFileStream->SetMethodEx("bool flush()", &VI_EXPECTIFY_VOID(Core::FileStream::Flush));
				VFileStream->SetMethodEx("usize size()", &VI_EXPECTIFY(Core::FileStream::Size));
				VFileStream->SetMethodEx("usize tell()", &VI_EXPECTIFY(Core::FileStream::Tell));
				VFileStream->SetMethod("usize virtual_size() const", &Core::FileStream::VirtualSize);
				VFileStream->SetMethod("const string& virtual_name() const", &Core::FileStream::VirtualName);
				VFileStream->SetMethod("void set_virtual_size(usize) const", &Core::FileStream::SetVirtualSize);
				VFileStream->SetMethod("void set_virtual_name(const string_view&in) const", &Core::FileStream::SetVirtualName);
				VFileStream->SetMethod("int32 get_readable_fd() const", &Core::FileStream::GetReadableFd);
				VFileStream->SetMethod("int32 get_writeable_fd() const", &Core::FileStream::GetWriteableFd);
				VFileStream->SetMethod("uptr@ get_readable() const", &Core::FileStream::GetReadable);
				VFileStream->SetMethod("uptr@ get_writeable() const", &Core::FileStream::GetWriteable);
				VFileStream->SetMethod("bool is_sized() const", &Core::FileStream::IsSized);
				VFileStream->SetMethodEx("bool open(const string_view&in, file_mode)", &StreamOpen);
				VFileStream->SetMethodEx("usize write(const string_view&in)", &StreamWrite);
				VFileStream->SetMethodEx("string read(usize)", &StreamRead);
				VFileStream->SetMethodEx("string read_line(usize)", &StreamReadLine);

				auto VGzStream = VM->SetClass<Core::GzStream>("gz_stream", false);
				VGzStream->SetConstructor<Core::GzStream>("gz_stream@ f()");
				VGzStream->SetMethodEx("bool clear()", &VI_EXPECTIFY_VOID(Core::GzStream::Clear));
				VGzStream->SetMethodEx("bool close()", &VI_EXPECTIFY_VOID(Core::GzStream::Close));
				VGzStream->SetMethodEx("bool seek(file_seek, int64)", &VI_EXPECTIFY_VOID(Core::GzStream::Seek));
				VGzStream->SetMethodEx("bool move(int64)", &VI_EXPECTIFY_VOID(Core::GzStream::Move));
				VGzStream->SetMethodEx("bool flush()", &VI_EXPECTIFY_VOID(Core::GzStream::Flush));
				VGzStream->SetMethodEx("usize size()", &VI_EXPECTIFY(Core::GzStream::Size));
				VGzStream->SetMethodEx("usize tell()", &VI_EXPECTIFY(Core::GzStream::Tell));
				VGzStream->SetMethod("usize virtual_size() const", &Core::GzStream::VirtualSize);
				VGzStream->SetMethod("const string& virtual_name() const", &Core::GzStream::VirtualName);
				VGzStream->SetMethod("void set_virtual_size(usize) const", &Core::GzStream::SetVirtualSize);
				VGzStream->SetMethod("void set_virtual_name(const string_view&in) const", &Core::GzStream::SetVirtualName);
				VGzStream->SetMethod("int32 get_readable_fd() const", &Core::GzStream::GetReadableFd);
				VGzStream->SetMethod("int32 get_writeable_fd() const", &Core::GzStream::GetWriteableFd);
				VGzStream->SetMethod("uptr@ get_readable() const", &Core::GzStream::GetReadable);
				VGzStream->SetMethod("uptr@ get_writeable() const", &Core::GzStream::GetWriteable);
				VGzStream->SetMethod("bool is_sized() const", &Core::GzStream::IsSized);
				VGzStream->SetMethodEx("bool open(const string_view&in, file_mode)", &StreamOpen);
				VGzStream->SetMethodEx("usize write(const string_view&in)", &StreamWrite);
				VGzStream->SetMethodEx("string read(usize)", &StreamRead);
				VGzStream->SetMethodEx("string read_line(usize)", &StreamReadLine);

				auto VWebStream = VM->SetClass<Core::WebStream>("web_stream", false);
				VWebStream->SetConstructor<Core::WebStream, bool>("web_stream@ f(bool)");
				VWebStream->SetMethodEx("bool clear()", &VI_EXPECTIFY_VOID(Core::WebStream::Clear));
				VWebStream->SetMethodEx("bool close()", &VI_EXPECTIFY_VOID(Core::WebStream::Close));
				VWebStream->SetMethodEx("bool seek(file_seek, int64)", &VI_EXPECTIFY_VOID(Core::WebStream::Seek));
				VWebStream->SetMethodEx("bool move(int64)", &VI_EXPECTIFY_VOID(Core::WebStream::Move));
				VWebStream->SetMethodEx("bool flush()", &VI_EXPECTIFY_VOID(Core::WebStream::Flush));
				VWebStream->SetMethodEx("usize size()", &VI_EXPECTIFY(Core::WebStream::Size));
				VWebStream->SetMethodEx("usize tell()", &VI_EXPECTIFY(Core::WebStream::Tell));
				VWebStream->SetMethod("usize virtual_size() const", &Core::WebStream::VirtualSize);
				VWebStream->SetMethod("const string& virtual_name() const", &Core::WebStream::VirtualName);
				VWebStream->SetMethod("void set_virtual_size(usize) const", &Core::WebStream::SetVirtualSize);
				VWebStream->SetMethod("void set_virtual_name(const string_view&in) const", &Core::WebStream::SetVirtualName);
				VWebStream->SetMethod("int32 get_readable_fd() const", &Core::WebStream::GetReadableFd);
				VWebStream->SetMethod("int32 get_writeable_fd() const", &Core::WebStream::GetWriteableFd);
				VWebStream->SetMethod("uptr@ get_readable() const", &Core::WebStream::GetReadable);
				VWebStream->SetMethod("uptr@ get_writeable() const", &Core::WebStream::GetWriteable);
				VWebStream->SetMethod("bool is_sized() const", &Core::WebStream::IsSized);
				VWebStream->SetMethodEx("bool open(const string_view&in, file_mode)", &StreamOpen);
				VWebStream->SetMethodEx("usize write(const string_view&in)", &StreamWrite);
				VWebStream->SetMethodEx("string read(usize)", &StreamRead);
				VWebStream->SetMethodEx("string read_line(usize)", &StreamReadLine);

				auto VProcessStream = VM->SetClass<Core::ProcessStream>("process_stream", false);
				VProcessStream->SetConstructor<Core::ProcessStream>("process_stream@ f()");
				VProcessStream->SetMethodEx("bool clear()", &VI_EXPECTIFY_VOID(Core::ProcessStream::Clear));
				VProcessStream->SetMethodEx("bool close()", &VI_EXPECTIFY_VOID(Core::ProcessStream::Close));
				VProcessStream->SetMethodEx("bool seek(file_seek, int64)", &VI_EXPECTIFY_VOID(Core::ProcessStream::Seek));
				VProcessStream->SetMethodEx("bool move(int64)", &VI_EXPECTIFY_VOID(Core::ProcessStream::Move));
				VProcessStream->SetMethodEx("bool flush()", &VI_EXPECTIFY_VOID(Core::ProcessStream::Flush));
				VProcessStream->SetMethodEx("usize size()", &VI_EXPECTIFY(Core::ProcessStream::Size));
				VProcessStream->SetMethodEx("usize tell()", &VI_EXPECTIFY(Core::ProcessStream::Tell));
				VProcessStream->SetMethod("usize virtual_size() const", &Core::ProcessStream::VirtualSize);
				VProcessStream->SetMethod("const string& virtual_name() const", &Core::ProcessStream::VirtualName);
				VProcessStream->SetMethod("void set_virtual_size(usize) const", &Core::ProcessStream::SetVirtualSize);
				VProcessStream->SetMethod("void set_virtual_name(const string_view&in) const", &Core::ProcessStream::SetVirtualName);
				VProcessStream->SetMethod("int32 get_readable_fd() const", &Core::ProcessStream::GetReadableFd);
				VProcessStream->SetMethod("int32 get_writeable_fd() const", &Core::ProcessStream::GetWriteableFd);
				VProcessStream->SetMethod("uptr@ get_readable() const", &Core::ProcessStream::GetReadable);
				VProcessStream->SetMethod("uptr@ get_writeable() const", &Core::ProcessStream::GetWriteable);
				VProcessStream->SetMethod("usize get_process_id() const", &Core::ProcessStream::GetProcessId);
				VProcessStream->SetMethod("usize get_thread_id() const", &Core::ProcessStream::GetThreadId);
				VProcessStream->SetMethod("int32 get_exit_code() const", &Core::ProcessStream::GetExitCode);
				VProcessStream->SetMethod("bool is_sized() const", &Core::ProcessStream::IsSized);
				VProcessStream->SetMethod("bool is_alive()", &Core::ProcessStream::IsAlive);
				VProcessStream->SetMethodEx("bool open(const string_view&in, file_mode)", &StreamOpen);
				VProcessStream->SetMethodEx("usize write(const string_view&in)", &StreamWrite);
				VProcessStream->SetMethodEx("string read(usize)", &StreamRead);
				VProcessStream->SetMethodEx("string read_line(usize)", &StreamReadLine);

				VStream->SetDynamicCast<Core::Stream, Core::MemoryStream>("memory_stream@+");
				VStream->SetDynamicCast<Core::Stream, Core::FileStream>("file_stream@+");
				VStream->SetDynamicCast<Core::Stream, Core::GzStream>("gz_stream@+");
				VStream->SetDynamicCast<Core::Stream, Core::WebStream>("web_stream@+");
				VStream->SetDynamicCast<Core::Stream, Core::ProcessStream>("process_stream@+");
				VMemoryStream->SetDynamicCast<Core::MemoryStream, Core::Stream>("base_stream@+", true);
				VFileStream->SetDynamicCast<Core::FileStream, Core::Stream>("base_stream@+", true);
				VGzStream->SetDynamicCast<Core::GzStream, Core::Stream>("base_stream@+", true);
				VWebStream->SetDynamicCast<Core::WebStream, Core::Stream>("base_stream@+", true);
				VProcessStream->SetDynamicCast<Core::ProcessStream, Core::Stream>("base_stream@+", true);
				return true;
#else
				VI_ASSERT(false, "<file_system> is not loaded");
				return false;
#endif
			}
			bool Registry::ImportOS(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr, "manager should be set");
				auto VArgsFormat = VM->SetEnum("args_format");
				VArgsFormat->SetValue("key_value", (int)Core::ArgsFormat::KeyValue);
				VArgsFormat->SetValue("flag_value", (int)Core::ArgsFormat::FlagValue);
				VArgsFormat->SetValue("key", (int)Core::ArgsFormat::Key);
				VArgsFormat->SetValue("flag", (int)Core::ArgsFormat::Flag);
				VArgsFormat->SetValue("stop_if_no_match", (int)Core::ArgsFormat::StopIfNoMatch);

				auto VInlineArgs = VM->SetStructTrivial<Core::InlineArgs>("inline_args");
				VInlineArgs->SetProperty<Core::InlineArgs>("string path", &Core::InlineArgs::Path);
				VInlineArgs->SetConstructor<Core::InlineArgs>("void f()");
				VInlineArgs->SetMethod("bool is_enabled(const string_view&in, const string_view&in = string_view()) const", &Core::InlineArgs::IsEnabled);
				VInlineArgs->SetMethod("bool is_disabled(const string_view&in, const string_view&in = string_view()) const", &Core::InlineArgs::IsDisabled);
				VInlineArgs->SetMethod("bool has(const string_view&in, const string_view&in = string_view()) const", &Core::InlineArgs::Has);
				VInlineArgs->SetMethod("string& get(const string_view&in, const string_view&in = string_view()) const", &Core::InlineArgs::Get);
				VInlineArgs->SetMethod("string& get_if(const string_view&in, const string_view&in, const string_view&in) const", &Core::InlineArgs::GetIf);
				VInlineArgs->SetMethodEx("dictionary@ get_args() const", &InlineArgsGetArgs);
				VInlineArgs->SetMethodEx("array<string>@ get_params() const", &InlineArgsGetParams);

				VM->BeginNamespace("os::cpu");
				auto VArch = VM->SetEnum("arch");
				VArch->SetValue("x64", (int)Core::OS::CPU::Arch::X64);
				VArch->SetValue("arm", (int)Core::OS::CPU::Arch::ARM);
				VArch->SetValue("itanium", (int)Core::OS::CPU::Arch::Itanium);
				VArch->SetValue("x86", (int)Core::OS::CPU::Arch::X86);
				VArch->SetValue("unknown", (int)Core::OS::CPU::Arch::Unknown);

				auto VEndian = VM->SetEnum("endian");
				VEndian->SetValue("little", (int)Core::OS::CPU::Endian::Little);
				VEndian->SetValue("big", (int)Core::OS::CPU::Endian::Big);

				auto VCache = VM->SetEnum("cache");
				VCache->SetValue("unified", (int)Core::OS::CPU::Cache::Unified);
				VCache->SetValue("instruction", (int)Core::OS::CPU::Cache::Instruction);
				VCache->SetValue("data", (int)Core::OS::CPU::Cache::Data);
				VCache->SetValue("trace", (int)Core::OS::CPU::Cache::Trace);

				auto VQuantityInfo = VM->SetPod<Core::OS::CPU::QuantityInfo>("quantity_info", (size_t)ObjectBehaviours::APP_CLASS_ALLINTS);
				VQuantityInfo->SetProperty("uint32 logical", &Core::OS::CPU::QuantityInfo::Logical);
				VQuantityInfo->SetProperty("uint32 physical", &Core::OS::CPU::QuantityInfo::Physical);
				VQuantityInfo->SetProperty("uint32 packages", &Core::OS::CPU::QuantityInfo::Packages);
				VQuantityInfo->SetConstructor<Core::OS::CPU::QuantityInfo>("void f()");

				auto VCacheInfo = VM->SetPod<Core::OS::CPU::CacheInfo>("cache_info");
				VCacheInfo->SetProperty("usize size", &Core::OS::CPU::CacheInfo::Size);
				VCacheInfo->SetProperty("usize line_size", &Core::OS::CPU::CacheInfo::LineSize);
				VCacheInfo->SetProperty("uint8 associativity", &Core::OS::CPU::CacheInfo::Associativity);
				VCacheInfo->SetProperty("cache type", &Core::OS::CPU::CacheInfo::Type);
				VCacheInfo->SetConstructor<Core::OS::CPU::CacheInfo>("void f()");

				VM->SetFunction("quantity_info get_quantity_info()", &Core::OS::CPU::GetQuantityInfo);
				VM->SetFunction("cache_info get_cache_info(uint32)", &Core::OS::CPU::GetCacheInfo);
				VM->SetFunction("arch get_arch()", &Core::OS::CPU::GetArch);
				VM->SetFunction("endian get_endianness()", &Core::OS::CPU::GetEndianness);
				VM->SetFunction("usize get_frequency()", &Core::OS::CPU::GetFrequency);
				VM->EndNamespace();

				VM->BeginNamespace("os::directory");
				VM->SetFunction("array<file_state>@ scan(const string_view&in)", &OSDirectoryScan);
				VM->SetFunction("array<string>@ get_mounts(const string_view&in)", &OSDirectoryGetMounts);
				VM->SetFunction("bool create(const string_view&in)", &OSDirectoryCreate);
				VM->SetFunction("bool remove(const string_view&in)", &OSDirectoryRemove);
				VM->SetFunction("bool is_exists(const string_view&in)", &OSDirectoryIsExists);
				VM->SetFunction("bool is_empty(const string_view&in)", &OSDirectoryIsEmpty);
				VM->SetFunction("bool patch(const string_view&in)", &OSDirectoryPatch);
				VM->SetFunction("bool set_working(const string_view&in)", &OSDirectorySetWorking);
				VM->SetFunction("string get_module()", &VI_SEXPECTIFY(Core::OS::Directory::GetModule));
				VM->SetFunction("string get_working()", &VI_SEXPECTIFY(Core::OS::Directory::GetWorking));
				VM->EndNamespace();

				VM->BeginNamespace("os::file");
				VM->SetFunction("bool write(const string_view&in, const string_view&in)", &OSFileWrite);
				VM->SetFunction("bool state(const string_view&in, file_entry &out)", &OSFileState);
				VM->SetFunction("bool move(const string_view&in, const string_view&in)", &OSFileMove);
				VM->SetFunction("bool copy(const string_view&in, const string_view&in)", &OSFileCopy);
				VM->SetFunction("bool remove(const string_view&in)", &OSFileRemove);
				VM->SetFunction("bool is_exists(const string_view&in)", &OSFileIsExists);
				VM->SetFunction("file_state get_properties(const string_view&in)", &OSFileGetProperties);
				VM->SetFunction("array<string>@ read_as_array(const string_view&in)", &OSFileReadAsArray);
				VM->SetFunction("usize join(const string_view&in, array<string>@+)", &OSFileJoin);
				VM->SetFunction("int32 compare(const string_view&in, const string_view&in)", &Core::OS::File::Compare);
				VM->SetFunction("int64 get_hash(const string_view&in)", &Core::OS::File::GetHash);
				VM->SetFunction("base_stream@ open_join(const string_view&in, array<string>@+)", &OSFileOpenJoin);
				VM->SetFunction("base_stream@ open_archive(const string_view&in, usize = 134217728)", &OSFileOpenArchive);
				VM->SetFunction("base_stream@ open(const string_view&in, file_mode, bool = false)", &OSFileOpen);
				VM->SetFunction("string read_as_string(const string_view&in)", &VI_SEXPECTIFY(Core::OS::File::ReadAsString));
				VM->EndNamespace();

				VM->BeginNamespace("os::path");
				VM->SetFunction("bool is_remote(const string_view&in)", &OSPathIsRemote);
				VM->SetFunction("bool is_relative(const string_view&in)", &OSPathIsRelative);
				VM->SetFunction("bool is_absolute(const string_view&in)", &OSPathIsAbsolute);
				VM->SetFunction("string resolve(const string_view&in)", &OSPathResolve1);
				VM->SetFunction("string resolve_directory(const string_view&in)", &OSPathResolveDirectory1);
				VM->SetFunction("string get_directory(const string_view&in, usize = 0)", &OSPathGetDirectory);
				VM->SetFunction("string get_filename(const string_view&in)", &OSPathGetFilename);
				VM->SetFunction("string get_extension(const string_view&in)", &OSPathGetExtension);
				VM->SetFunction("string get_non_existant(const string_view&in)", &Core::OS::Path::GetNonExistant);
				VM->SetFunction("string resolve(const string_view&in, const string_view&in, bool)", &OSPathResolve2);
				VM->SetFunction("string resolve_directory(const string_view&in, const string_view&in, bool)", &OSPathResolveDirectory2);
				VM->EndNamespace();

				VM->BeginNamespace("os::process");
				VM->SetFunctionDef("void process_async(const string_view&in)");
				VM->SetFunction("void abort()", &Core::OS::Process::Abort);
				VM->SetFunction("void exit(int)", &Core::OS::Process::Exit);
				VM->SetFunction("void interrupt()", &Core::OS::Process::Interrupt);
				VM->SetFunction("string get_env(const string_view&in)", &Core::OS::Process::GetEnv);
				VM->SetFunction("string get_shell()", &Core::OS::Process::GetShell);
				VM->SetFunction("process_stream@+ spawn(const string_view&in, file_mode)", &VI_SEXPECTIFY(Core::OS::Process::Spawn));
				VM->SetFunction("int execute(const string_view&in, file_mode, process_async@)", &OSProcessExecute);
				VM->SetFunction("inline_args parse_args(array<string>@+, usize, array<string>@+ = null)", &OSProcessParseArgs);
				VM->EndNamespace();

				VM->BeginNamespace("os::symbol");
				VM->SetFunction("uptr@ load(const string_view&in)", &OSSymbolLoad);
				VM->SetFunction("uptr@ load_function(uptr@, const string_view&in)", &OSSymbolLoadFunction);
				VM->SetFunction("bool unload(uptr@)", &OSSymbolUnload);
				VM->EndNamespace();

				VM->BeginNamespace("os::error");
				VM->SetFunction("int32 get(bool = true)", &Core::OS::Error::Get);
				VM->SetFunction("void clear()", &Core::OS::Error::Clear);
				VM->SetFunction("bool occurred()", &Core::OS::Error::Occurred);
				VM->SetFunction("bool is_error(int32)", &Core::OS::Error::IsError);
				VM->SetFunction("string get_name(int32)", &Core::OS::Error::GetName);
				VM->EndNamespace();

				if (VM->GetLibraryProperty(LibraryFeatures::OsExposeControl) > 0)
				{
					auto VFsOption = VM->SetEnum("access_option");
					VFsOption->SetValue("allow_mem", (int)Core::AccessOption::Mem);
					VFsOption->SetValue("allow_fs", (int)Core::AccessOption::Fs);
					VFsOption->SetValue("allow_gz", (int)Core::AccessOption::Gz);
					VFsOption->SetValue("allow_net", (int)Core::AccessOption::Net);
					VFsOption->SetValue("allow_lib", (int)Core::AccessOption::Lib);
					VFsOption->SetValue("allow_http", (int)Core::AccessOption::Http);
					VFsOption->SetValue("allow_https", (int)Core::AccessOption::Https);
					VFsOption->SetValue("allow_shell", (int)Core::AccessOption::Shell);
					VFsOption->SetValue("allow_env", (int)Core::AccessOption::Env);
					VFsOption->SetValue("allow_addons", (int)Core::AccessOption::Addons);
					VFsOption->SetValue("all", (int)Core::AccessOption::All);

					VM->BeginNamespace("os::control");
					VM->SetFunction("bool set(access_option, bool)", &Core::OS::Control::Set);
					VM->SetFunction("bool has(access_option)", &Core::OS::Control::Has);
					VM->EndNamespace();
				}

				return true;
#else
				VI_ASSERT(false, "<os> is not loaded");
				return false;
#endif
			}
			bool Registry::ImportSchedule(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr, "manager should be set");
				VM->SetTypeDef("task_id", "uint64");

				auto VDifficulty = VM->SetEnum("difficulty");
				VDifficulty->SetValue("async", (int)Core::Difficulty::Async);
				VDifficulty->SetValue("sync", (int)Core::Difficulty::Sync);
				VDifficulty->SetValue("timeout", (int)Core::Difficulty::Timeout);

				auto VDesc = VM->SetStructTrivial<Core::Schedule::Desc>("schedule_policy");
				VDesc->SetProperty("usize preallocated_size", &Core::Schedule::Desc::PreallocatedSize);
				VDesc->SetProperty("usize stack_size", &Core::Schedule::Desc::StackSize);
				VDesc->SetProperty("usize max_coroutines", &Core::Schedule::Desc::MaxCoroutines);
				VDesc->SetProperty("usize max_recycles", &Core::Schedule::Desc::MaxRecycles);
				VDesc->SetProperty("bool parallel", &Core::Schedule::Desc::Parallel);
				VDesc->SetConstructor<Core::Schedule::Desc>("void f()");
				VDesc->SetConstructor<Core::Schedule::Desc, size_t>("void f(usize)");

				auto VSchedule = VM->SetClass<Core::Schedule>("schedule", false);
				VSchedule->SetFunctionDef("void task_async()");
				VSchedule->SetFunctionDef("void task_parallel()");
				VSchedule->SetMethodEx("task_id set_interval(uint64, task_async@)", &ScheduleSetInterval);
				VSchedule->SetMethodEx("task_id set_timeout(uint64, task_async@)", &ScheduleSetTimeout);
				VSchedule->SetMethodEx("bool set_immediate(task_async@)", &ScheduleSetImmediate);
				VSchedule->SetMethodEx("bool spawn(task_parallel@)", &ScheduleSpawn);
				VSchedule->SetMethod("bool clear_timeout(task_id)", &Core::Schedule::ClearTimeout);
				VSchedule->SetMethod("bool trigger_timers()", &Core::Schedule::TriggerTimers);
				VSchedule->SetMethod("bool trigger(difficulty)", &Core::Schedule::Trigger);
				VSchedule->SetMethod("bool dispatch()", &Core::Schedule::Dispatch);
				VSchedule->SetMethod("bool start(const schedule_policy &in)", &Core::Schedule::Start);
				VSchedule->SetMethod("bool stop()", &Core::Schedule::Stop);
				VSchedule->SetMethod("bool wakeup()", &Core::Schedule::Wakeup);
				VSchedule->SetMethod("bool is_active() const", &Core::Schedule::IsActive);
				VSchedule->SetMethod("bool can_enqueue() const", &Core::Schedule::CanEnqueue);
				VSchedule->SetMethod("bool has_tasks(difficulty) const", &Core::Schedule::HasTasks);
				VSchedule->SetMethod("bool has_any_tasks() const", &Core::Schedule::HasAnyTasks);
				VSchedule->SetMethod("bool is_suspended() const", &Core::Schedule::IsSuspended);
				VSchedule->SetMethod("void suspend()", &Core::Schedule::Suspend);
				VSchedule->SetMethod("void resume()", &Core::Schedule::Resume);
				VSchedule->SetMethod("usize get_total_threads() const", &Core::Schedule::GetTotalThreads);
				VSchedule->SetMethod("usize get_thread_global_index()", &Core::Schedule::GetThreadGlobalIndex);
				VSchedule->SetMethod("usize get_thread_local_index()", &Core::Schedule::GetThreadLocalIndex);
				VSchedule->SetMethod("usize get_threads(difficulty) const", &Core::Schedule::GetThreads);
				VSchedule->SetMethod("bool has_parallel_threads(difficulty) const", &Core::Schedule::HasParallelThreads);
				VSchedule->SetMethod("const schedule_policy& get_policy() const", &Core::Schedule::GetPolicy);
				VSchedule->SetMethodStatic("schedule@+ get()", &Core::Schedule::Get);

				return true;
#else
				VI_ASSERT(false, "<schedule> is not loaded");
				return false;
#endif
			}
			bool Registry::ImportRegex(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr, "manager should be set");
				auto VRegexState = VM->SetEnum("regex_state");
				VRegexState->SetValue("preprocessed", (int)Compute::RegexState::Preprocessed);
				VRegexState->SetValue("match_found", (int)Compute::RegexState::Match_Found);
				VRegexState->SetValue("no_match", (int)Compute::RegexState::No_Match);
				VRegexState->SetValue("unexpected_quantifier", (int)Compute::RegexState::Unexpected_Quantifier);
				VRegexState->SetValue("unbalanced_brackets", (int)Compute::RegexState::Unbalanced_Brackets);
				VRegexState->SetValue("internal_error", (int)Compute::RegexState::Internal_Error);
				VRegexState->SetValue("invalid_character_set", (int)Compute::RegexState::Invalid_Character_Set);
				VRegexState->SetValue("invalid_metacharacter", (int)Compute::RegexState::Invalid_Metacharacter);
				VRegexState->SetValue("sumatch_array_too_small", (int)Compute::RegexState::Sumatch_Array_Too_Small);
				VRegexState->SetValue("too_many_branches", (int)Compute::RegexState::Too_Many_Branches);
				VRegexState->SetValue("too_many_brackets", (int)Compute::RegexState::Too_Many_Brackets);

				auto VRegexMatch = VM->SetPod<Compute::RegexMatch>("regex_match");
				VRegexMatch->SetProperty<Compute::RegexMatch>("int64 start", &Compute::RegexMatch::Start);
				VRegexMatch->SetProperty<Compute::RegexMatch>("int64 end", &Compute::RegexMatch::End);
				VRegexMatch->SetProperty<Compute::RegexMatch>("int64 length", &Compute::RegexMatch::Length);
				VRegexMatch->SetProperty<Compute::RegexMatch>("int64 steps", &Compute::RegexMatch::Steps);
				VRegexMatch->SetConstructor<Compute::RegexMatch>("void f()");
				VRegexMatch->SetMethodEx("string target() const", &RegexMatchTarget);

				auto VRegexResult = VM->SetStructTrivial<Compute::RegexResult>("regex_result");
				VRegexResult->SetConstructor<Compute::RegexResult>("void f()");
				VRegexResult->SetMethod("bool empty() const", &Compute::RegexResult::Empty);
				VRegexResult->SetMethod("int64 get_steps() const", &Compute::RegexResult::GetSteps);
				VRegexResult->SetMethod("regex_state get_state() const", &Compute::RegexResult::GetState);
				VRegexResult->SetMethodEx("array<regex_match>@ get() const", &RegexResultGet);
				VRegexResult->SetMethodEx("array<string>@ to_array() const", &RegexResultToArray);

				auto VRegexSource = VM->SetStructTrivial<Compute::RegexSource>("regex_source");
				VRegexSource->SetProperty<Compute::RegexSource>("bool ignoreCase", &Compute::RegexSource::IgnoreCase);
				VRegexSource->SetConstructor<Compute::RegexSource>("void f()");
				VRegexSource->SetConstructor<Compute::RegexSource, const std::string_view&, bool, int64_t, int64_t, int64_t>("void f(const string_view&in, bool = false, int64 = -1, int64 = -1, int64 = -1)");
				VRegexSource->SetMethod("const string& get_regex() const", &Compute::RegexSource::GetRegex);
				VRegexSource->SetMethod("int64 get_max_branches() const", &Compute::RegexSource::GetMaxBranches);
				VRegexSource->SetMethod("int64 get_max_brackets() const", &Compute::RegexSource::GetMaxBrackets);
				VRegexSource->SetMethod("int64 get_max_matches() const", &Compute::RegexSource::GetMaxMatches);
				VRegexSource->SetMethod("int64 get_complexity() const", &Compute::RegexSource::GetComplexity);
				VRegexSource->SetMethod("regex_state get_state() const", &Compute::RegexSource::GetState);
				VRegexSource->SetMethod("bool is_simple() const", &Compute::RegexSource::IsSimple);
				VRegexSource->SetMethodEx("regex_result match(const string_view&in) const", &RegexSourceMatch);
				VRegexSource->SetMethodEx("string replace(const string_view&in, const string_view&in) const", &RegexSourceReplace);

				return true;
#else
				VI_ASSERT(false, "<regex> is not loaded");
				return false;
#endif
			}
			bool Registry::ImportCrypto(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr, "manager should be set");
				VI_TYPEREF(WebToken, "web_token");
				VM->SetClass<Core::Stream>("base_stream", false);

				auto VPrivateKey = VM->SetStructTrivial<Compute::PrivateKey>("private_key");
				VPrivateKey->SetConstructor<Compute::PrivateKey>("void f()");
				VPrivateKey->SetConstructor<Compute::PrivateKey, const std::string_view&>("void f(const string_view&in)");
				VPrivateKey->SetMethod("void clear()", &Compute::PrivateKey::Clear);
				VPrivateKey->SetMethod<Compute::PrivateKey, void, const std::string_view&>("void secure(const string_view&in)", &Compute::PrivateKey::Secure);
				VPrivateKey->SetMethod("string expose_to_heap() const", &Compute::PrivateKey::ExposeToHeap);
				VPrivateKey->SetMethod("usize size() const", &Compute::PrivateKey::Clear);
				VPrivateKey->SetMethodStatic<Compute::PrivateKey, const std::string_view&>("private_key get_plain(const string_view&in)", &Compute::PrivateKey::GetPlain);

				auto VWebToken = VM->SetClass<Compute::WebToken>("web_token", true);
				VWebToken->SetProperty<Compute::WebToken>("schema@ header", &Compute::WebToken::Header);
				VWebToken->SetProperty<Compute::WebToken>("schema@ payload", &Compute::WebToken::Payload);
				VWebToken->SetProperty<Compute::WebToken>("schema@ token", &Compute::WebToken::Token);
				VWebToken->SetProperty<Compute::WebToken>("string refresher", &Compute::WebToken::Refresher);
				VWebToken->SetProperty<Compute::WebToken>("string signature", &Compute::WebToken::Signature);
				VWebToken->SetProperty<Compute::WebToken>("string data", &Compute::WebToken::Data);
				VWebToken->SetGcConstructor<Compute::WebToken, WebToken>("web_token@ f()");
				VWebToken->SetGcConstructor<Compute::WebToken, WebToken, const std::string_view&, const std::string_view&, int64_t>("web_token@ f(const string_view&in, const string_view&in, int64)");
				VWebToken->SetMethod("void unsign()", &Compute::WebToken::Unsign);
				VWebToken->SetMethod("void set_algorithm(const string_view&in)", &Compute::WebToken::SetAlgorithm);
				VWebToken->SetMethod("void set_type(const string_view&in)", &Compute::WebToken::SetType);
				VWebToken->SetMethod("void set_content_type(const string_view&in)", &Compute::WebToken::SetContentType);
				VWebToken->SetMethod("void set_issuer(const string_view&in)", &Compute::WebToken::SetIssuer);
				VWebToken->SetMethod("void set_subject(const string_view&in)", &Compute::WebToken::SetSubject);
				VWebToken->SetMethod("void set_id(const string_view&in)", &Compute::WebToken::SetId);
				VWebToken->SetMethod("void set_expiration(int64)", &Compute::WebToken::SetExpiration);
				VWebToken->SetMethod("void set_not_before(int64)", &Compute::WebToken::SetNotBefore);
				VWebToken->SetMethod("void set_created(int64)", &Compute::WebToken::SetCreated);
				VWebToken->SetMethod("void set_refresh_token(const string_view&in, const private_key &in, const private_key &in)", &Compute::WebToken::SetRefreshToken);
				VWebToken->SetMethod("bool sign(const private_key &in)", &Compute::WebToken::Sign);
				VWebToken->SetMethod("bool is_valid()", &Compute::WebToken::IsValid);
				VWebToken->SetMethodEx("string get_refresh_token(const private_key &in, const private_key &in)", &VI_EXPECTIFY(Compute::WebToken::GetRefreshToken));
				VWebToken->SetMethodEx("void set_audience(array<string>@+)", &WebTokenSetAudience);
				VWebToken->SetEnumRefsEx<Compute::WebToken>([](Compute::WebToken* Base, asIScriptEngine* VM) { });
				VWebToken->SetReleaseRefsEx<Compute::WebToken>([](Compute::WebToken* Base, asIScriptEngine*) { });

				VM->BeginNamespace("ciphers");
				VM->SetFunction("uptr@ des_ecb()", &Compute::Ciphers::DES_ECB);
				VM->SetFunction("uptr@ des_ede()", &Compute::Ciphers::DES_EDE);
				VM->SetFunction("uptr@ des_ede3()", &Compute::Ciphers::DES_EDE3);
				VM->SetFunction("uptr@ des_ede_ecb()", &Compute::Ciphers::DES_EDE_ECB);
				VM->SetFunction("uptr@ des_ede3_ecb()", &Compute::Ciphers::DES_EDE3_ECB);
				VM->SetFunction("uptr@ des_cfb64()", &Compute::Ciphers::DES_CFB64);
				VM->SetFunction("uptr@ des_cfb1()", &Compute::Ciphers::DES_CFB1);
				VM->SetFunction("uptr@ des_cfb8()", &Compute::Ciphers::DES_CFB8);
				VM->SetFunction("uptr@ des_ede_cfb64()", &Compute::Ciphers::DES_EDE_CFB64);
				VM->SetFunction("uptr@ des_ede3_cfb64()", &Compute::Ciphers::DES_EDE3_CFB64);
				VM->SetFunction("uptr@ des_ede3_cfb1()", &Compute::Ciphers::DES_EDE3_CFB1);
				VM->SetFunction("uptr@ des_ede3_cfb8()", &Compute::Ciphers::DES_EDE3_CFB8);
				VM->SetFunction("uptr@ des_ofb()", &Compute::Ciphers::DES_OFB);
				VM->SetFunction("uptr@ des_ede_ofb()", &Compute::Ciphers::DES_EDE_OFB);
				VM->SetFunction("uptr@ des_ede3_ofb()", &Compute::Ciphers::DES_EDE3_OFB);
				VM->SetFunction("uptr@ des_cbc()", &Compute::Ciphers::DES_CBC);
				VM->SetFunction("uptr@ des_ede_cbc()", &Compute::Ciphers::DES_EDE_CBC);
				VM->SetFunction("uptr@ des_ede3_cbc()", &Compute::Ciphers::DES_EDE3_CBC);
				VM->SetFunction("uptr@ des_ede3_wrap()", &Compute::Ciphers::DES_EDE3_Wrap);
				VM->SetFunction("uptr@ desx_cbc()", &Compute::Ciphers::DESX_CBC);
				VM->SetFunction("uptr@ rc4()", &Compute::Ciphers::RC4);
				VM->SetFunction("uptr@ rc4_40()", &Compute::Ciphers::RC4_40);
				VM->SetFunction("uptr@ rc4_hmac_md5()", &Compute::Ciphers::RC4_HMAC_MD5);
				VM->SetFunction("uptr@ idea_ecb()", &Compute::Ciphers::IDEA_ECB);
				VM->SetFunction("uptr@ idea_cfb64()", &Compute::Ciphers::IDEA_CFB64);
				VM->SetFunction("uptr@ idea_ofb()", &Compute::Ciphers::IDEA_OFB);
				VM->SetFunction("uptr@ idea_cbc()", &Compute::Ciphers::IDEA_CBC);
				VM->SetFunction("uptr@ rc2_ecb()", &Compute::Ciphers::RC2_ECB);
				VM->SetFunction("uptr@ rc2_cbc()", &Compute::Ciphers::RC2_CBC);
				VM->SetFunction("uptr@ rc2_40_cbc()", &Compute::Ciphers::RC2_40_CBC);
				VM->SetFunction("uptr@ rc2_64_cbc()", &Compute::Ciphers::RC2_64_CBC);
				VM->SetFunction("uptr@ rc2_cfb64()", &Compute::Ciphers::RC2_CFB64);
				VM->SetFunction("uptr@ rc2_ofb()", &Compute::Ciphers::RC2_OFB);
				VM->SetFunction("uptr@ bf_ecb()", &Compute::Ciphers::BF_ECB);
				VM->SetFunction("uptr@ bf_cbc()", &Compute::Ciphers::BF_CBC);
				VM->SetFunction("uptr@ bf_cfb64()", &Compute::Ciphers::BF_CFB64);
				VM->SetFunction("uptr@ bf_ofb()", &Compute::Ciphers::BF_OFB);
				VM->SetFunction("uptr@ cast5_ecb()", &Compute::Ciphers::CAST5_ECB);
				VM->SetFunction("uptr@ cast5_cbc()", &Compute::Ciphers::CAST5_CBC);
				VM->SetFunction("uptr@ cast5_cfb64()", &Compute::Ciphers::CAST5_CFB64);
				VM->SetFunction("uptr@ cast5_ofb()", &Compute::Ciphers::CAST5_OFB);
				VM->SetFunction("uptr@ rc5_32_12_16_cbc()", &Compute::Ciphers::RC5_32_12_16_CBC);
				VM->SetFunction("uptr@ rc5_32_12_16_ecb()", &Compute::Ciphers::RC5_32_12_16_ECB);
				VM->SetFunction("uptr@ rc5_32_12_16_cfb64()", &Compute::Ciphers::RC5_32_12_16_CFB64);
				VM->SetFunction("uptr@ rc5_32_12_16_ofb()", &Compute::Ciphers::RC5_32_12_16_OFB);
				VM->SetFunction("uptr@ aes_128_ecb()", &Compute::Ciphers::AES_128_ECB);
				VM->SetFunction("uptr@ aes_128_cbc()", &Compute::Ciphers::AES_128_CBC);
				VM->SetFunction("uptr@ aes_128_cfb1()", &Compute::Ciphers::AES_128_CFB1);
				VM->SetFunction("uptr@ aes_128_cfb8()", &Compute::Ciphers::AES_128_CFB8);
				VM->SetFunction("uptr@ aes_128_cfb128()", &Compute::Ciphers::AES_128_CFB128);
				VM->SetFunction("uptr@ aes_128_ofb()", &Compute::Ciphers::AES_128_OFB);
				VM->SetFunction("uptr@ aes_128_ctr()", &Compute::Ciphers::AES_128_CTR);
				VM->SetFunction("uptr@ aes_128_ccm()", &Compute::Ciphers::AES_128_CCM);
				VM->SetFunction("uptr@ aes_128_gcm()", &Compute::Ciphers::AES_128_GCM);
				VM->SetFunction("uptr@ aes_128_xts()", &Compute::Ciphers::AES_128_XTS);
				VM->SetFunction("uptr@ aes_128_wrap()", &Compute::Ciphers::AES_128_Wrap);
				VM->SetFunction("uptr@ aes_128_wrap_pad()", &Compute::Ciphers::AES_128_WrapPad);
				VM->SetFunction("uptr@ aes_128_ocb()", &Compute::Ciphers::AES_128_OCB);
				VM->SetFunction("uptr@ aes_192_ecb()", &Compute::Ciphers::AES_192_ECB);
				VM->SetFunction("uptr@ aes_192_cbc()", &Compute::Ciphers::AES_192_CBC);
				VM->SetFunction("uptr@ aes_192_cfb1()", &Compute::Ciphers::AES_192_CFB1);
				VM->SetFunction("uptr@ aes_192_cfb8()", &Compute::Ciphers::AES_192_CFB8);
				VM->SetFunction("uptr@ aes_192_cfb128()", &Compute::Ciphers::AES_192_CFB128);
				VM->SetFunction("uptr@ aes_192_ofb()", &Compute::Ciphers::AES_192_OFB);
				VM->SetFunction("uptr@ aes_192_ctr()", &Compute::Ciphers::AES_192_CTR);
				VM->SetFunction("uptr@ aes_192_ccm()", &Compute::Ciphers::AES_192_CCM);
				VM->SetFunction("uptr@ aes_192_gcm()", &Compute::Ciphers::AES_192_GCM);
				VM->SetFunction("uptr@ aes_192_wrap()", &Compute::Ciphers::AES_192_Wrap);
				VM->SetFunction("uptr@ aes_192_wrap_pad()", &Compute::Ciphers::AES_192_WrapPad);
				VM->SetFunction("uptr@ aes_192_ocb()", &Compute::Ciphers::AES_192_OCB);
				VM->SetFunction("uptr@ aes_256_ecb()", &Compute::Ciphers::AES_256_ECB);
				VM->SetFunction("uptr@ aes_256_cbc()", &Compute::Ciphers::AES_256_CBC);
				VM->SetFunction("uptr@ aes_256_cfb1()", &Compute::Ciphers::AES_256_CFB1);
				VM->SetFunction("uptr@ aes_256_cfb8()", &Compute::Ciphers::AES_256_CFB8);
				VM->SetFunction("uptr@ aes_256_cfb128()", &Compute::Ciphers::AES_256_CFB128);
				VM->SetFunction("uptr@ aes_256_ofb()", &Compute::Ciphers::AES_256_OFB);
				VM->SetFunction("uptr@ aes_256_ctr()", &Compute::Ciphers::AES_256_CTR);
				VM->SetFunction("uptr@ aes_256_ccm()", &Compute::Ciphers::AES_256_CCM);
				VM->SetFunction("uptr@ aes_256_gcm()", &Compute::Ciphers::AES_256_GCM);
				VM->SetFunction("uptr@ aes_256_xts()", &Compute::Ciphers::AES_256_XTS);
				VM->SetFunction("uptr@ aes_256_wrap()", &Compute::Ciphers::AES_256_Wrap);
				VM->SetFunction("uptr@ aes_256_wrap_pad()", &Compute::Ciphers::AES_256_WrapPad);
				VM->SetFunction("uptr@ aes_256_ocb()", &Compute::Ciphers::AES_256_OCB);
				VM->SetFunction("uptr@ aes_128_cbc_hmac_SHA1()", &Compute::Ciphers::AES_128_CBC_HMAC_SHA1);
				VM->SetFunction("uptr@ aes_256_cbc_hmac_SHA1()", &Compute::Ciphers::AES_256_CBC_HMAC_SHA1);
				VM->SetFunction("uptr@ aes_128_cbc_hmac_SHA256()", &Compute::Ciphers::AES_128_CBC_HMAC_SHA256);
				VM->SetFunction("uptr@ aes_256_cbc_hmac_SHA256()", &Compute::Ciphers::AES_256_CBC_HMAC_SHA256);
				VM->SetFunction("uptr@ aria_128_ecb()", &Compute::Ciphers::ARIA_128_ECB);
				VM->SetFunction("uptr@ aria_128_cbc()", &Compute::Ciphers::ARIA_128_CBC);
				VM->SetFunction("uptr@ aria_128_cfb1()", &Compute::Ciphers::ARIA_128_CFB1);
				VM->SetFunction("uptr@ aria_128_cfb8()", &Compute::Ciphers::ARIA_128_CFB8);
				VM->SetFunction("uptr@ aria_128_cfb128()", &Compute::Ciphers::ARIA_128_CFB128);
				VM->SetFunction("uptr@ aria_128_ctr()", &Compute::Ciphers::ARIA_128_CTR);
				VM->SetFunction("uptr@ aria_128_ofb()", &Compute::Ciphers::ARIA_128_OFB);
				VM->SetFunction("uptr@ aria_128_gcm()", &Compute::Ciphers::ARIA_128_GCM);
				VM->SetFunction("uptr@ aria_128_ccm()", &Compute::Ciphers::ARIA_128_CCM);
				VM->SetFunction("uptr@ aria_192_ecb()", &Compute::Ciphers::ARIA_192_ECB);
				VM->SetFunction("uptr@ aria_192_cbc()", &Compute::Ciphers::ARIA_192_CBC);
				VM->SetFunction("uptr@ aria_192_cfb1()", &Compute::Ciphers::ARIA_192_CFB1);
				VM->SetFunction("uptr@ aria_192_cfb8()", &Compute::Ciphers::ARIA_192_CFB8);
				VM->SetFunction("uptr@ aria_192_cfb128()", &Compute::Ciphers::ARIA_192_CFB128);
				VM->SetFunction("uptr@ aria_192_ctr()", &Compute::Ciphers::ARIA_192_CTR);
				VM->SetFunction("uptr@ aria_192_ofb()", &Compute::Ciphers::ARIA_192_OFB);
				VM->SetFunction("uptr@ aria_192_gcm()", &Compute::Ciphers::ARIA_192_GCM);
				VM->SetFunction("uptr@ aria_192_ccm()", &Compute::Ciphers::ARIA_192_CCM);
				VM->SetFunction("uptr@ aria_256_ecb()", &Compute::Ciphers::ARIA_256_ECB);
				VM->SetFunction("uptr@ aria_256_cbc()", &Compute::Ciphers::ARIA_256_CBC);
				VM->SetFunction("uptr@ aria_256_cfb1()", &Compute::Ciphers::ARIA_256_CFB1);
				VM->SetFunction("uptr@ aria_256_cfb8()", &Compute::Ciphers::ARIA_256_CFB8);
				VM->SetFunction("uptr@ aria_256_cfb128()", &Compute::Ciphers::ARIA_256_CFB128);
				VM->SetFunction("uptr@ aria_256_ctr()", &Compute::Ciphers::ARIA_256_CTR);
				VM->SetFunction("uptr@ aria_256_ofb()", &Compute::Ciphers::ARIA_256_OFB);
				VM->SetFunction("uptr@ aria_256_gcm()", &Compute::Ciphers::ARIA_256_GCM);
				VM->SetFunction("uptr@ aria_256_ccm()", &Compute::Ciphers::ARIA_256_CCM);
				VM->SetFunction("uptr@ camellia_128_ecb()", &Compute::Ciphers::Camellia_128_ECB);
				VM->SetFunction("uptr@ camellia_128_cbc()", &Compute::Ciphers::Camellia_128_CBC);
				VM->SetFunction("uptr@ camellia_128_cfb1()", &Compute::Ciphers::Camellia_128_CFB1);
				VM->SetFunction("uptr@ camellia_128_cfb8()", &Compute::Ciphers::Camellia_128_CFB8);
				VM->SetFunction("uptr@ camellia_128_cfb128()", &Compute::Ciphers::Camellia_128_CFB128);
				VM->SetFunction("uptr@ camellia_128_ofb()", &Compute::Ciphers::Camellia_128_OFB);
				VM->SetFunction("uptr@ camellia_128_ctr()", &Compute::Ciphers::Camellia_128_CTR);
				VM->SetFunction("uptr@ camellia_192_ecb()", &Compute::Ciphers::Camellia_192_ECB);
				VM->SetFunction("uptr@ camellia_192_cbc()", &Compute::Ciphers::Camellia_192_CBC);
				VM->SetFunction("uptr@ camellia_192_cfb1()", &Compute::Ciphers::Camellia_192_CFB1);
				VM->SetFunction("uptr@ camellia_192_cfb8()", &Compute::Ciphers::Camellia_192_CFB8);
				VM->SetFunction("uptr@ camellia_192_cfb128()", &Compute::Ciphers::Camellia_192_CFB128);
				VM->SetFunction("uptr@ camellia_192_ofb()", &Compute::Ciphers::Camellia_192_OFB);
				VM->SetFunction("uptr@ camellia_192_ctr()", &Compute::Ciphers::Camellia_192_CTR);
				VM->SetFunction("uptr@ camellia_256_ecb()", &Compute::Ciphers::Camellia_256_ECB);
				VM->SetFunction("uptr@ camellia_256_cbc()", &Compute::Ciphers::Camellia_256_CBC);
				VM->SetFunction("uptr@ camellia_256_cfb1()", &Compute::Ciphers::Camellia_256_CFB1);
				VM->SetFunction("uptr@ camellia_256_cfb8()", &Compute::Ciphers::Camellia_256_CFB8);
				VM->SetFunction("uptr@ camellia_256_cfb128()", &Compute::Ciphers::Camellia_256_CFB128);
				VM->SetFunction("uptr@ camellia_256_ofb()", &Compute::Ciphers::Camellia_256_OFB);
				VM->SetFunction("uptr@ camellia_256_ctr()", &Compute::Ciphers::Camellia_256_CTR);
				VM->SetFunction("uptr@ chacha20()", &Compute::Ciphers::Chacha20);
				VM->SetFunction("uptr@ chacha20_poly1305()", &Compute::Ciphers::Chacha20_Poly1305);
				VM->SetFunction("uptr@ seed_ecb()", &Compute::Ciphers::Seed_ECB);
				VM->SetFunction("uptr@ seed_cbc()", &Compute::Ciphers::Seed_CBC);
				VM->SetFunction("uptr@ seed_cfb128()", &Compute::Ciphers::Seed_CFB128);
				VM->SetFunction("uptr@ seed_ofb()", &Compute::Ciphers::Seed_OFB);
				VM->SetFunction("uptr@ sm4_ecb()", &Compute::Ciphers::SM4_ECB);
				VM->SetFunction("uptr@ sm4_cbc()", &Compute::Ciphers::SM4_CBC);
				VM->SetFunction("uptr@ sm4_cfb128()", &Compute::Ciphers::SM4_CFB128);
				VM->SetFunction("uptr@ sm4_ofb()", &Compute::Ciphers::SM4_OFB);
				VM->SetFunction("uptr@ sm4_ctr()", &Compute::Ciphers::SM4_CTR);
				VM->EndNamespace();

				VM->BeginNamespace("digests");
				VM->SetFunction("uptr@ md2()", &Compute::Digests::MD2);
				VM->SetFunction("uptr@ md4()", &Compute::Digests::MD4);
				VM->SetFunction("uptr@ md5()", &Compute::Digests::MD5);
				VM->SetFunction("uptr@ md5_sha1()", &Compute::Digests::MD5_SHA1);
				VM->SetFunction("uptr@ blake2b512()", &Compute::Digests::Blake2B512);
				VM->SetFunction("uptr@ blake2s256()", &Compute::Digests::Blake2S256);
				VM->SetFunction("uptr@ sha1()", &Compute::Digests::SHA1);
				VM->SetFunction("uptr@ sha224()", &Compute::Digests::SHA224);
				VM->SetFunction("uptr@ sha256()", &Compute::Digests::SHA256);
				VM->SetFunction("uptr@ sha384()", &Compute::Digests::SHA384);
				VM->SetFunction("uptr@ sha512()", &Compute::Digests::SHA512);
				VM->SetFunction("uptr@ sha512_224()", &Compute::Digests::SHA512_224);
				VM->SetFunction("uptr@ sha512_256()", &Compute::Digests::SHA512_256);
				VM->SetFunction("uptr@ sha3_224()", &Compute::Digests::SHA3_224);
				VM->SetFunction("uptr@ sha3_256()", &Compute::Digests::SHA3_256);
				VM->SetFunction("uptr@ sha3_384()", &Compute::Digests::SHA3_384);
				VM->SetFunction("uptr@ sha3_512()", &Compute::Digests::SHA3_512);
				VM->SetFunction("uptr@ shake128()", &Compute::Digests::Shake128);
				VM->SetFunction("uptr@ shake256()", &Compute::Digests::Shake256);
				VM->SetFunction("uptr@ mdc2()", &Compute::Digests::MDC2);
				VM->SetFunction("uptr@ ripemd160()", &Compute::Digests::RipeMD160);
				VM->SetFunction("uptr@ whirlpool()", &Compute::Digests::Whirlpool);
				VM->SetFunction("uptr@ sm3()", &Compute::Digests::SM3);
				VM->EndNamespace();

				VM->BeginNamespace("signers");
				VM->SetFunction("int32 pk_rsa()", &Compute::Signers::PkRSA);
				VM->SetFunction("int32 pk_dsa()", &Compute::Signers::PkDSA);
				VM->SetFunction("int32 pk_dh()", &Compute::Signers::PkDH);
				VM->SetFunction("int32 pk_ec()", &Compute::Signers::PkEC);
				VM->SetFunction("int32 pkt_sign()", &Compute::Signers::PktSIGN);
				VM->SetFunction("int32 pkt_enc()", &Compute::Signers::PktENC);
				VM->SetFunction("int32 pkt_exch()", &Compute::Signers::PktEXCH);
				VM->SetFunction("int32 pks_rsa()", &Compute::Signers::PksRSA);
				VM->SetFunction("int32 pks_dsa()", &Compute::Signers::PksDSA);
				VM->SetFunction("int32 pks_ec()", &Compute::Signers::PksEC);
				VM->SetFunction("int32 rsa()", &Compute::Signers::RSA);
				VM->SetFunction("int32 rsa2()", &Compute::Signers::RSA2);
				VM->SetFunction("int32 rsa_pss()", &Compute::Signers::RSA_PSS);
				VM->SetFunction("int32 dsa()", &Compute::Signers::DSA);
				VM->SetFunction("int32 dsa1()", &Compute::Signers::DSA1);
				VM->SetFunction("int32 dsa2()", &Compute::Signers::DSA2);
				VM->SetFunction("int32 dsa3()", &Compute::Signers::DSA3);
				VM->SetFunction("int32 dsa4()", &Compute::Signers::DSA4);
				VM->SetFunction("int32 dh()", &Compute::Signers::DH);
				VM->SetFunction("int32 dhx()", &Compute::Signers::DHX);
				VM->SetFunction("int32 ec()", &Compute::Signers::EC);
				VM->SetFunction("int32 sm2()", &Compute::Signers::SM2);
				VM->SetFunction("int32 hmac()", &Compute::Signers::HMAC);
				VM->SetFunction("int32 cmac()", &Compute::Signers::CMAC);
				VM->SetFunction("int32 scrypt()", &Compute::Signers::SCRYPT);
				VM->SetFunction("int32 tls1_prf()", &Compute::Signers::TLS1_PRF);
				VM->SetFunction("int32 hkdf()", &Compute::Signers::HKDF);
				VM->SetFunction("int32 poly1305()", &Compute::Signers::POLY1305);
				VM->SetFunction("int32 siphash()", &Compute::Signers::SIPHASH);
				VM->SetFunction("int32 x25519()", &Compute::Signers::X25519);
				VM->SetFunction("int32 ed25519()", &Compute::Signers::ED25519);
				VM->SetFunction("int32 x448()", &Compute::Signers::X448);
				VM->SetFunction("int32 ed448()", &Compute::Signers::ED448);
				VM->EndNamespace();

				VM->BeginNamespace("crypto");
				VM->SetFunctionDef("string block_transform_sync(const string_view&in)");
				VM->SetFunction("uint8 random_uc()", &Compute::Crypto::RandomUC);
				VM->SetFunction<uint64_t()>("uint64 random()", &Compute::Crypto::Random);
				VM->SetFunction<uint64_t(uint64_t, uint64_t)>("uint64 random(uint64, uint64)", &Compute::Crypto::Random);
				VM->SetFunction("uint64 crc32(const string_view&in)", &Compute::Crypto::CRC32);
				VM->SetFunction("void display_crypto_log()", &Compute::Crypto::DisplayCryptoLog);
				VM->SetFunction("string random_bytes(usize)", &VI_SEXPECTIFY(Compute::Crypto::RandomBytes));
				VM->SetFunction("string generate_private_key(int32, usize = 2048, const string_view&in = string_view())", &VI_SEXPECTIFY(Compute::Crypto::GeneratePrivateKey));
				VM->SetFunction("string generate_public_key(int32, const private_key&in)", &VI_SEXPECTIFY(Compute::Crypto::GeneratePublicKey));
				VM->SetFunction("string checksum_hex(base_stream@, const string_view&in)", &VI_SEXPECTIFY(Compute::Crypto::ChecksumHex));
				VM->SetFunction("string checksum_raw(base_stream@, const string_view&in)", &VI_SEXPECTIFY(Compute::Crypto::ChecksumRaw));
				VM->SetFunction("string hash_hex(uptr@, const string_view&in)", &VI_SEXPECTIFY(Compute::Crypto::HashHex));
				VM->SetFunction("string hash_raw(uptr@, const string_view&in)", &VI_SEXPECTIFY(Compute::Crypto::HashRaw));
				VM->SetFunction("string jwt_sign(const string_view&in, const string_view&in, const private_key &in)", &VI_SEXPECTIFY(Compute::Crypto::JWTSign));
				VM->SetFunction("string jwt_encode(web_token@+, const private_key &in)", &VI_SEXPECTIFY(Compute::Crypto::JWTEncode));
				VM->SetFunction("web_token@ jwt_decode(const string_view&in, const private_key &in)", &VI_SEXPECTIFY(Compute::Crypto::JWTDecode));
				VM->SetFunction("string doc_encrypt(schema@+, const private_key &in, const private_key &in)", &VI_SEXPECTIFY(Compute::Crypto::DocEncrypt));
				VM->SetFunction("schema@ doc_decrypt(const string_view&in, const private_key &in, const private_key &in)", &VI_SEXPECTIFY(Compute::Crypto::DocDecrypt));
				VM->SetFunction("string sign(uptr@, int32, const string_view&in, const private_key &in)", &CryptoSign);
				VM->SetFunction("bool verify(uptr@, int32, const string_view&in, const string_view&in, const private_key &in)", &CryptoVerify);
				VM->SetFunction("string hmac(uptr@, const string_view&in, const private_key &in)", &CryptoHMAC);
				VM->SetFunction("string encrypt(uptr@, const string_view&in, const private_key &in, const private_key &in, int = -1)", &CryptoEncrypt);
				VM->SetFunction("string decrypt(uptr@, const string_view&in, const private_key &in, const private_key &in, int = -1)", &CryptoDecrypt);
				VM->SetFunction("usize encrypt(uptr@, base_stream@, const private_key &in, const private_key &in, block_transform_sync@ = null, usize = 1, int = -1)", &CryptoEncryptStream);
				VM->SetFunction("usize decrypt(uptr@, base_stream@, const private_key &in, const private_key &in, block_transform_sync@ = null, usize = 1, int = -1)", &CryptoDecryptStream);
				VM->EndNamespace();

				return true;
#else
				VI_ASSERT(false, "<crypto> is not loaded");
				return false;
#endif
			}
			bool Registry::ImportCodec(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr, "manager should be set");
				auto VCompression = VM->SetEnum("compression_cdc");
				VCompression->SetValue("none", (int)Compute::Compression::None);
				VCompression->SetValue("best_speed", (int)Compute::Compression::BestSpeed);
				VCompression->SetValue("best_compression", (int)Compute::Compression::BestCompression);
				VCompression->SetValue("default_compression", (int)Compute::Compression::Default);

				VM->BeginNamespace("codec");
				VM->SetFunction("string rotate(const string_view&in, uint64, int8)", &Compute::Codec::Rotate);
				VM->SetFunction("string bep45_encode(const string_view&in)", &Compute::Codec::Bep45Encode);
				VM->SetFunction("string bep45_decode(const string_view&in)", &Compute::Codec::Bep45Decode);
				VM->SetFunction("string base32_encode(const string_view&in)", &Compute::Codec::Base32Encode);
				VM->SetFunction("string base32_decode(const string_view&in)", &Compute::Codec::Base32Decode);
				VM->SetFunction("string base45_encode(const string_view&in)", &Compute::Codec::Base45Encode);
				VM->SetFunction("string base45_decode(const string_view&in)", &Compute::Codec::Base45Decode);
				VM->SetFunction("string compress(const string_view&in, compression_cdc)", &VI_SEXPECTIFY(Compute::Codec::Compress));
				VM->SetFunction("string decompress(const string_view&in)", &VI_SEXPECTIFY(Compute::Codec::Decompress));
				VM->SetFunction<Core::String(const std::string_view&)>("string base64_encode(const string_view&in)", &Compute::Codec::Base64Encode);
				VM->SetFunction<Core::String(const std::string_view&)>("string base64_decode(const string_view&in)", &Compute::Codec::Base64Decode);
				VM->SetFunction<Core::String(const std::string_view&)>("string base64_url_encode(const string_view&in)", &Compute::Codec::Base64URLEncode);
				VM->SetFunction<Core::String(const std::string_view&)>("string base64_url_decode(const string_view&in)", &Compute::Codec::Base64URLDecode);
				VM->SetFunction<Core::String(const std::string_view&, bool)>("string hex_encode(const string_view&in, bool = false)", &Compute::Codec::HexEncode);
				VM->SetFunction<Core::String(const std::string_view&)>("string hex_decode(const string_view&in)", &Compute::Codec::HexDecode);
				VM->SetFunction<Core::String(const std::string_view&)>("string url_encode(const string_view&in)", &Compute::Codec::URLEncode);
				VM->SetFunction<Core::String(const std::string_view&)>("string url_decode(const string_view&in)", &Compute::Codec::URLDecode);
				VM->SetFunction("string decimal_to_hex(uint64)", &Compute::Codec::DecimalToHex);
				VM->SetFunction("string base10_to_base_n(uint64, uint8)", &Compute::Codec::Base10ToBaseN);
				VM->EndNamespace();

				return true;
#else
				VI_ASSERT(false, "<codec> is not loaded");
				return false;
#endif
			}
			bool Registry::ImportPreprocessor(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr, "manager should be set");
				auto VIncludeType = VM->SetEnum("include_type");
				VIncludeType->SetValue("error_t", (int)Compute::IncludeType::Error);
				VIncludeType->SetValue("preprocess_t", (int)Compute::IncludeType::Preprocess);
				VIncludeType->SetValue("unchaned_t", (int)Compute::IncludeType::Unchanged);
				VIncludeType->SetValue("virtual_t", (int)Compute::IncludeType::Virtual);

				auto VProcDirective = VM->SetStructTrivial<Compute::ProcDirective>("proc_directive");
				VProcDirective->SetProperty<Compute::ProcDirective>("string name", &Compute::ProcDirective::Name);
				VProcDirective->SetProperty<Compute::ProcDirective>("string value", &Compute::ProcDirective::Value);
				VProcDirective->SetProperty<Compute::ProcDirective>("usize start", &Compute::ProcDirective::Start);
				VProcDirective->SetProperty<Compute::ProcDirective>("usize end", &Compute::ProcDirective::End);
				VProcDirective->SetProperty<Compute::ProcDirective>("bool found", &Compute::ProcDirective::Found);
				VProcDirective->SetProperty<Compute::ProcDirective>("bool as_global", &Compute::ProcDirective::AsGlobal);
				VProcDirective->SetProperty<Compute::ProcDirective>("bool as_scope", &Compute::ProcDirective::AsScope);
				VProcDirective->SetConstructor<Compute::ProcDirective>("void f()");

				auto VIncludeDesc = VM->SetStructTrivial<Compute::IncludeDesc>("include_desc");
				VIncludeDesc->SetProperty<Compute::IncludeDesc>("string from", &Compute::IncludeDesc::From);
				VIncludeDesc->SetProperty<Compute::IncludeDesc>("string path", &Compute::IncludeDesc::Path);
				VIncludeDesc->SetProperty<Compute::IncludeDesc>("string root", &Compute::IncludeDesc::Root);
				VIncludeDesc->SetConstructor<Compute::IncludeDesc>("void f()");
				VIncludeDesc->SetMethodEx("void add_ext(const string_view&in)", &IncludeDescAddExt);
				VIncludeDesc->SetMethodEx("void remove_ext(const string_view&in)", &IncludeDescRemoveExt);

				auto VIncludeResult = VM->SetStructTrivial<Compute::IncludeResult>("include_result");
				VIncludeResult->SetProperty<Compute::IncludeResult>("string module", &Compute::IncludeResult::Module);
				VIncludeResult->SetProperty<Compute::IncludeResult>("bool is_abstract", &Compute::IncludeResult::IsAbstract);
				VIncludeResult->SetProperty<Compute::IncludeResult>("bool is_remote", &Compute::IncludeResult::IsRemote);
				VIncludeResult->SetProperty<Compute::IncludeResult>("bool is_file", &Compute::IncludeResult::IsFile);
				VIncludeResult->SetConstructor<Compute::IncludeResult>("void f()");

				auto VDesc = VM->SetPod<Compute::Preprocessor::Desc>("preprocessor_desc");
				VDesc->SetProperty<Compute::Preprocessor::Desc>("string multiline_comment_begin", &Compute::Preprocessor::Desc::MultilineCommentBegin);
				VDesc->SetProperty<Compute::Preprocessor::Desc>("string multiline_comment_end", &Compute::Preprocessor::Desc::MultilineCommentEnd);
				VDesc->SetProperty<Compute::Preprocessor::Desc>("string comment_begin", &Compute::Preprocessor::Desc::CommentBegin);
				VDesc->SetProperty<Compute::Preprocessor::Desc>("bool pragmas", &Compute::Preprocessor::Desc::Pragmas);
				VDesc->SetProperty<Compute::Preprocessor::Desc>("bool includes", &Compute::Preprocessor::Desc::Includes);
				VDesc->SetProperty<Compute::Preprocessor::Desc>("bool defines", &Compute::Preprocessor::Desc::Defines);
				VDesc->SetProperty<Compute::Preprocessor::Desc>("bool conditions", &Compute::Preprocessor::Desc::Conditions);
				VDesc->SetConstructor<Compute::Preprocessor::Desc>("void f()");

				auto VPreprocessor = VM->SetClass<Compute::Preprocessor>("preprocessor", false);
				VPreprocessor->SetFunctionDef("include_type proc_include_sync(preprocessor@+, const include_result &in, string &out)");
				VPreprocessor->SetFunctionDef("bool proc_pragma_sync(preprocessor@+, const string_view&in, array<string>@+)");
				VPreprocessor->SetFunctionDef("bool proc_directive_sync(preprocessor@+, const proc_directive&in, string&out)");
				VPreprocessor->SetConstructor<Compute::Preprocessor>("preprocessor@ f(uptr@)");
				VPreprocessor->SetMethod("void set_include_options(const include_desc &in)", &Compute::Preprocessor::SetIncludeOptions);
				VPreprocessor->SetMethod("void set_features(const preprocessor_desc &in)", &Compute::Preprocessor::SetFeatures);
				VPreprocessor->SetMethod("void add_default_definitions()", &Compute::Preprocessor::AddDefaultDefinitions);
				VPreprocessor->SetMethodEx("bool define(const string_view&in)", &VI_EXPECTIFY_VOID(Compute::Preprocessor::Define));
				VPreprocessor->SetMethod("void undefine(const string_view&in)", &Compute::Preprocessor::Undefine);
				VPreprocessor->SetMethod("void clear()", &Compute::Preprocessor::Clear);
				VPreprocessor->SetMethodEx("bool process(const string_view&in, string &out)", &VI_EXPECTIFY_VOID(Compute::Preprocessor::Process));
				VPreprocessor->SetMethodEx("string resolve_file(const string_view&in, const string_view&in)", &VI_EXPECTIFY(Compute::Preprocessor::ResolveFile));
				VPreprocessor->SetMethod("const string& get_current_file_path() const", &Compute::Preprocessor::GetCurrentFilePath);
				VPreprocessor->SetMethodEx("void set_include_callback(proc_include_sync@)", &PreprocessorSetIncludeCallback);
				VPreprocessor->SetMethodEx("void set_pragma_callback(proc_pragma_sync@)", &PreprocessorSetPragmaCallback);
				VPreprocessor->SetMethodEx("void set_directive_callback(const string_view&in, proc_directive_sync@)", &PreprocessorSetDirectiveCallback);
				VPreprocessor->SetMethodEx("bool is_defined(const string_view&in) const", &PreprocessorIsDefined);

				return true;
#else
				VI_ASSERT(false, "<preprocessor> is not loaded");
				return false;
#endif
			}
			bool Registry::ImportNetwork(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr, "manager should be set");
				VI_TYPEREF(SocketAddress, "socket_address");
				VI_TYPEREF(SocketAccept, "socket_accept");
				VI_TYPEREF(SocketListener, "socket_listener");
				VI_TYPEREF(SocketConnection, "socket_connection");
				VI_TYPEREF(SocketServer, "socket_server");
				VI_TYPEREF(Socket, "socket@");
				VI_TYPEREF(String, "string");

				auto VSecureLayerOptions = VM->SetEnum("secure_layer_options");
				VSecureLayerOptions->SetValue("all", (int)Network::SecureLayerOptions::All);
				VSecureLayerOptions->SetValue("ssl_v2", (int)Network::SecureLayerOptions::NoSSL_V2);
				VSecureLayerOptions->SetValue("ssl_v3", (int)Network::SecureLayerOptions::NoSSL_V3);
				VSecureLayerOptions->SetValue("tls_v1", (int)Network::SecureLayerOptions::NoTLS_V1);
				VSecureLayerOptions->SetValue("tls_v1_1", (int)Network::SecureLayerOptions::NoTLS_V1_1);
				VSecureLayerOptions->SetValue("tls_v1_2", (int)Network::SecureLayerOptions::NoTLS_V1_1);
				VSecureLayerOptions->SetValue("tls_v1_3", (int)Network::SecureLayerOptions::NoTLS_V1_1);

				auto VServerState = VM->SetEnum("server_state");
				VServerState->SetValue("working", (int)Network::ServerState::Working);
				VServerState->SetValue("stopping", (int)Network::ServerState::Stopping);
				VServerState->SetValue("idle", (int)Network::ServerState::Idle);

				auto VSocketPoll = VM->SetEnum("socket_poll");
				VSocketPoll->SetValue("next", (int)Network::SocketPoll::Next);
				VSocketPoll->SetValue("reset", (int)Network::SocketPoll::Reset);
				VSocketPoll->SetValue("timeout", (int)Network::SocketPoll::Timeout);
				VSocketPoll->SetValue("cancel", (int)Network::SocketPoll::Cancel);
				VSocketPoll->SetValue("finish", (int)Network::SocketPoll::Finish);
				VSocketPoll->SetValue("finish_sync", (int)Network::SocketPoll::FinishSync);

				auto VSocketProtocol = VM->SetEnum("socket_protocol");
				VSocketProtocol->SetValue("ip", (int)Network::SocketProtocol::IP);
				VSocketProtocol->SetValue("icmp", (int)Network::SocketProtocol::ICMP);
				VSocketProtocol->SetValue("tcp", (int)Network::SocketProtocol::TCP);
				VSocketProtocol->SetValue("udp", (int)Network::SocketProtocol::UDP);
				VSocketProtocol->SetValue("raw", (int)Network::SocketProtocol::RAW);

				auto VSocketType = VM->SetEnum("socket_type");
				VSocketType->SetValue("stream", (int)Network::SocketType::Stream);
				VSocketType->SetValue("datagram", (int)Network::SocketType::Datagram);
				VSocketType->SetValue("raw", (int)Network::SocketType::Raw);
				VSocketType->SetValue("reliably_delivered_message", (int)Network::SocketType::Reliably_Delivered_Message);
				VSocketType->SetValue("sequence_packet_stream", (int)Network::SocketType::Sequence_Packet_Stream);

				auto VDNSType = VM->SetEnum("dns_type");
				VDNSType->SetValue("connect", (int)Network::DNSType::Connect);
				VDNSType->SetValue("listen", (int)Network::DNSType::Listen);

				auto VSocketAddress = VM->SetStructTrivial<Network::SocketAddress>("socket_address");
				VSocketAddress->SetConstructor<Network::SocketAddress>("void f()");
				VSocketAddress->SetConstructor<Network::SocketAddress, const std::string_view&, uint16_t>("void f(const string_view&in, uint16)");
				VSocketAddress->SetMethod("uptr@ get_address4_ptr() const", &Network::SocketAddress::GetAddress4);
				VSocketAddress->SetMethod("uptr@ get_address6_ptr() const", &Network::SocketAddress::GetAddress6);
				VSocketAddress->SetMethod("uptr@ get_address_ptr() const", &Network::SocketAddress::GetRawAddress);
				VSocketAddress->SetMethod("usize get_address_ptr_size() const", &Network::SocketAddress::GetAddressSize);
				VSocketAddress->SetMethod("int32 get_flags() const", &Network::SocketAddress::GetFlags);
				VSocketAddress->SetMethod("int32 get_family() const", &Network::SocketAddress::GetFamily);
				VSocketAddress->SetMethod("int32 get_type() const", &Network::SocketAddress::GetType);
				VSocketAddress->SetMethod("int32 get_protocol() const", &Network::SocketAddress::GetProtocol);
				VSocketAddress->SetMethod("dns_type get_resolver_type() const", &Network::SocketAddress::GetResolverType);
				VSocketAddress->SetMethod("socket_protocol get_protocol_type() const", &Network::SocketAddress::GetProtocolType);
				VSocketAddress->SetMethod("socket_type get_socket_type() const", &Network::SocketAddress::GetSocketType);
				VSocketAddress->SetMethod("bool is_valid() const", &Network::SocketAddress::IsValid);
				VSocketAddress->SetMethodEx("string get_hostname() const", &SocketAddressGetHostname);
				VSocketAddress->SetMethodEx("string get_ip_address() const", &SocketAddressGetIpAddress);
				VSocketAddress->SetMethodEx("uint16 get_ip_port() const", &SocketAddressGetIpPort);

				auto VSocketAccept = VM->SetStructTrivial<Network::SocketAccept>("socket_accept");
				VSocketAccept->SetProperty<Network::SocketAccept>("socket_address address", &Network::SocketAccept::Address);
				VSocketAccept->SetProperty<Network::SocketAccept>("usize fd", &Network::SocketAccept::Fd);
				VSocketAccept->SetConstructor<Network::SocketAccept>("void f()");

				auto VRouterListener = VM->SetStructTrivial<Network::RouterListener>("router_listener");
				VRouterListener->SetProperty<Network::RouterListener>("socket_address address", &Network::RouterListener::Address);
				VRouterListener->SetProperty<Network::RouterListener>("bool is_secure", &Network::RouterListener::IsSecure);
				VRouterListener->SetConstructor<Network::RouterListener>("void f()");

				auto VLocation = VM->SetStructTrivial<Network::Location>("url_location");
				VLocation->SetProperty<Network::Location>("string protocol", &Network::Location::Protocol);
				VLocation->SetProperty<Network::Location>("string username", &Network::Location::Username);
				VLocation->SetProperty<Network::Location>("string password", &Network::Location::Password);
				VLocation->SetProperty<Network::Location>("string hostname", &Network::Location::Hostname);
				VLocation->SetProperty<Network::Location>("string fragment", &Network::Location::Fragment);
				VLocation->SetProperty<Network::Location>("string path", &Network::Location::Path);
				VLocation->SetProperty<Network::Location>("string body", &Network::Location::Body);
				VLocation->SetProperty<Network::Location>("int32 port", &Network::Location::Port);
				VLocation->SetConstructor<Network::Location, const std::string_view&>("void f(const string_view&in)");
				VLocation->SetMethodEx("dictionary@ get_query() const", &LocationGetQuery);

				auto VCertificate = VM->SetStructTrivial<Network::Certificate>("certificate");
				VCertificate->SetProperty<Network::Certificate>("string subject_name", &Network::Certificate::SubjectName);
				VCertificate->SetProperty<Network::Certificate>("string issuer_name", &Network::Certificate::IssuerName);
				VCertificate->SetProperty<Network::Certificate>("string serial_number", &Network::Certificate::SerialNumber);
				VCertificate->SetProperty<Network::Certificate>("string fingerprint", &Network::Certificate::Fingerprint);
				VCertificate->SetProperty<Network::Certificate>("string public_key", &Network::Certificate::PublicKey);
				VCertificate->SetProperty<Network::Certificate>("string not_before_date", &Network::Certificate::NotBeforeDate);
				VCertificate->SetProperty<Network::Certificate>("string not_after_date", &Network::Certificate::NotAfterDate);
				VCertificate->SetProperty<Network::Certificate>("int64 not_before_time", &Network::Certificate::NotBeforeTime);
				VCertificate->SetProperty<Network::Certificate>("int64 not_after_time", &Network::Certificate::NotAfterTime);
				VCertificate->SetProperty<Network::Certificate>("int32 version", &Network::Certificate::Version);
				VCertificate->SetProperty<Network::Certificate>("int32 signature", &Network::Certificate::Signature);
				VCertificate->SetMethodEx("dictionary@ get_extensions() const", &CertificateGetExtensions);
				VCertificate->SetConstructor<Network::Certificate>("void f()");

				auto VCertificateBlob = VM->SetStructTrivial<Network::CertificateBlob>("certificate_blob");
				VCertificateBlob->SetProperty<Network::CertificateBlob>("string certificate", &Network::CertificateBlob::Certificate);
				VCertificateBlob->SetProperty<Network::CertificateBlob>("string private_key", &Network::CertificateBlob::PrivateKey);
				VCertificateBlob->SetConstructor<Network::CertificateBlob>("void f()");

				auto VSocketCertificate = VM->SetStructTrivial<Network::SocketCertificate>("socket_certificate");
				VSocketCertificate->SetProperty<Network::SocketCertificate>("certificate_blob blob", &Network::SocketCertificate::Blob);
				VSocketCertificate->SetProperty<Network::SocketCertificate>("string ciphers", &Network::SocketCertificate::Ciphers);
				VSocketCertificate->SetProperty<Network::SocketCertificate>("uptr@ context", &Network::SocketCertificate::Context);
				VSocketCertificate->SetProperty<Network::SocketCertificate>("secure_layer_options options", &Network::SocketCertificate::Options);
				VSocketCertificate->SetProperty<Network::SocketCertificate>("uint32 verify_peers", &Network::SocketCertificate::VerifyPeers);
				VSocketCertificate->SetConstructor<Network::SocketCertificate>("void f()");

				auto VDataFrame = VM->SetStructTrivial<Network::DataFrame>("socket_data_frame");
				VDataFrame->SetProperty<Network::DataFrame>("string message", &Network::DataFrame::Message);
				VDataFrame->SetProperty<Network::DataFrame>("usize reuses", &Network::DataFrame::Reuses);
				VDataFrame->SetProperty<Network::DataFrame>("int64 start", &Network::DataFrame::Start);
				VDataFrame->SetProperty<Network::DataFrame>("int64 finish", &Network::DataFrame::Finish);
				VDataFrame->SetProperty<Network::DataFrame>("int64 timeout", &Network::DataFrame::Timeout);
				VDataFrame->SetProperty<Network::DataFrame>("bool abort", &Network::DataFrame::Abort);
				VDataFrame->SetConstructor<Network::DataFrame>("void f()");

				auto VSocket = VM->SetClass<Network::Socket>("socket", false);
				auto VEpollFd = VM->SetStruct<Network::EpollFd>("epoll_fd");
				VEpollFd->SetProperty<Network::EpollFd>("socket@ base", &Network::EpollFd::Base);
				VEpollFd->SetProperty<Network::EpollFd>("bool readable", &Network::EpollFd::Readable);
				VEpollFd->SetProperty<Network::EpollFd>("bool writeable", &Network::EpollFd::Writeable);
				VEpollFd->SetProperty<Network::EpollFd>("bool closeable", &Network::EpollFd::Closeable);
				VEpollFd->SetConstructor<Network::EpollFd>("void f()");
				VEpollFd->SetOperatorCopyStatic(&EpollFdCopy);
				VEpollFd->SetDestructorEx("void f()", &EpollFdDestructor);

				auto VEpollHandle = VM->SetStruct<Network::EpollHandle>("epoll_handle");
				VEpollHandle->SetConstructor<Network::EpollHandle, size_t>("void f(usize)");
				VEpollHandle->SetMethod("bool add(socket@+, bool, bool)", &Network::EpollHandle::Add);
				VEpollHandle->SetMethod("bool update(socket@+, bool, bool)", &Network::EpollHandle::Update);
				VEpollHandle->SetMethod("bool remove(socket@+)", &Network::EpollHandle::Remove);
				VEpollHandle->SetMethod("usize capacity() const", &Network::EpollHandle::Capacity);
				VEpollHandle->SetMethodEx("int wait(array<epoll_fd>@+, uint64)", &EpollHandleWait);
				VEpollHandle->SetOperatorMoveCopy<Network::EpollHandle>();
				VEpollHandle->SetDestructor<Network::EpollHandle>("void f()");

				auto VStream = VM->SetClass<Core::FileStream>("file_stream", false);
				VSocket->SetFunctionDef("void socket_accept_async(socket_accept&in)");
				VSocket->SetFunctionDef("void socket_status_async(int)");
				VSocket->SetFunctionDef("void socket_send_async(socket_poll)");
				VSocket->SetFunctionDef("bool socket_recv_async(socket_poll, const string_view&in)");
				VSocket->SetProperty<Network::Socket>("int64 income", &Network::Socket::Income);
				VSocket->SetProperty<Network::Socket>("int64 outcome", &Network::Socket::Outcome);
				VSocket->SetConstructor<Network::Socket>("socket@ f()");
				VSocket->SetConstructor<Network::Socket, socket_t>("socket@ f(usize)");
				VSocket->SetMethod("usize get_fd() const", &Network::Socket::GetFd);
				VSocket->SetMethod("uptr@ get_device() const", &Network::Socket::GetDevice);
				VSocket->SetMethod("bool is_awaiting_readable() const", &Network::Socket::IsAwaitingReadable);
				VSocket->SetMethod("bool is_awaiting_writeable() const", &Network::Socket::IsAwaitingWriteable);
				VSocket->SetMethod("bool is_awaiting_events() const", &Network::Socket::IsAwaitingEvents);
				VSocket->SetMethod("bool is_valid() const", &Network::Socket::IsValid);
				VSocket->SetMethod("bool is_secure() const", &Network::Socket::IsSecure);
				VSocket->SetMethod("void set_io_timeout(uint64)", &Network::Socket::SetIoTimeout);
				VSocket->SetMethodEx("promise<socket_accept>@ accept_deferred()", &VI_SPROMISIFY_REF(SocketAcceptDeferred, SocketAccept));
				VSocket->SetMethodEx("promise<bool>@ connect_deferred(const socket_address&in)", &VI_SPROMISIFY(SocketConnectDeferred, TypeId::BOOL));
				VSocket->SetMethodEx("promise<bool>@ close_deferred()", &VI_SPROMISIFY(SocketCloseDeferred, TypeId::BOOL));
				VSocket->SetMethodEx("promise<bool>@ write_file_deferred(file_stream@+, usize, usize)", &VI_SPROMISIFY(SocketWriteFileDeferred, TypeId::BOOL));
				VSocket->SetMethodEx("promise<bool>@ write_deferred(const string_view&in)", &VI_SPROMISIFY(SocketWriteDeferred, TypeId::BOOL));
				VSocket->SetMethodEx("promise<string>@ read_deferred(usize)", &VI_SPROMISIFY_REF(SocketReadDeferred, String));
				VSocket->SetMethodEx("promise<string>@ read_until_deferred(const string_view&in, usize)", &VI_SPROMISIFY_REF(SocketReadUntilDeferred, String));
				VSocket->SetMethodEx("promise<string>@ read_until_chunked_deferred(const string_view&in, usize)", &VI_SPROMISIFY_REF(SocketReadUntilChunkedDeferred, String));
				VSocket->SetMethodEx("bool accept(socket_accept&out)", &VI_EXPECTIFY_VOID(Network::Socket::Accept));
				VSocket->SetMethodEx("bool connect(const socket_address&in, uint64)", &VI_EXPECTIFY_VOID(Network::Socket::Connect));
				VSocket->SetMethodEx("bool close()", &VI_EXPECTIFY_VOID(Network::Socket::Close));
				VSocket->SetMethodEx("usize write_file(file_stream@+, usize, usize)", &SocketWriteFile);
				VSocket->SetMethodEx("usize write(const string_view&in)", &SocketWrite);
				VSocket->SetMethodEx("usize read(string &out, usize)", &SocketRead);
				VSocket->SetMethodEx("usize read_until(const string_view&in, socket_recv_async@)", &SocketReadUntil);
				VSocket->SetMethodEx("usize read_until_chunked(const string_view&in, socket_recv_async@)", &SocketReadUntilChunked);
				VSocket->SetMethodEx("socket_address get_peer_address() const", &VI_EXPECTIFY(Network::Socket::GetPeerAddress));
				VSocket->SetMethodEx("socket_address get_this_address() const", &VI_EXPECTIFY(Network::Socket::GetThisAddress));
				VSocket->SetMethodEx("bool get_any_flag(int, int, int &out) const", &VI_EXPECTIFY_VOID(Network::Socket::GetAnyFlag));
				VSocket->SetMethodEx("bool get_socket_flag(int, int &out) const", &VI_EXPECTIFY_VOID(Network::Socket::GetSocketFlag));
				VSocket->SetMethodEx("bool set_close_on_exec()", VI_EXPECTIFY_VOID(Network::Socket::SetCloseOnExec));
				VSocket->SetMethodEx("bool set_time_wait(int)", VI_EXPECTIFY_VOID(Network::Socket::SetTimeWait));
				VSocket->SetMethodEx("bool set_any_flag(int, int, int)", VI_EXPECTIFY_VOID(Network::Socket::SetAnyFlag));
				VSocket->SetMethodEx("bool set_socket_flag(int, int)", VI_EXPECTIFY_VOID(Network::Socket::SetSocketFlag));
				VSocket->SetMethodEx("bool set_blocking(bool)", VI_EXPECTIFY_VOID(Network::Socket::SetBlocking));
				VSocket->SetMethodEx("bool set_no_delay(bool)", VI_EXPECTIFY_VOID(Network::Socket::SetNoDelay));
				VSocket->SetMethodEx("bool set_keep_alive(bool)", VI_EXPECTIFY_VOID(Network::Socket::SetKeepAlive));
				VSocket->SetMethodEx("bool set_timeout(int)", VI_EXPECTIFY_VOID(Network::Socket::SetTimeout));
				VSocket->SetMethodEx("bool shutdown(bool = false)", &VI_EXPECTIFY_VOID(Network::Socket::Shutdown));
				VSocket->SetMethodEx("bool open(const socket_address&in)", &VI_EXPECTIFY_VOID(Network::Socket::Open));
				VSocket->SetMethodEx("bool secure(uptr@, const string_view&in)", &VI_EXPECTIFY_VOID(Network::Socket::Secure));
				VSocket->SetMethodEx("bool bind(const socket_address&in)", &VI_EXPECTIFY_VOID(Network::Socket::Bind));
				VSocket->SetMethodEx("bool listen(int)", &VI_EXPECTIFY_VOID(Network::Socket::Listen));
				VSocket->SetMethodEx("bool clear_events(bool)", &VI_EXPECTIFY_VOID(Network::Socket::ClearEvents));
				VSocket->SetMethodEx("bool migrate_to(usize, bool = true)", &VI_EXPECTIFY_VOID(Network::Socket::MigrateTo));

				VM->BeginNamespace("net_packet");
				VM->SetFunction("bool is_data(socket_poll)", &Network::Packet::IsData);
				VM->SetFunction("bool is_skip(socket_poll)", &Network::Packet::IsSkip);
				VM->SetFunction("bool is_done(socket_poll)", &Network::Packet::IsDone);
				VM->SetFunction("bool is_done_sync(socket_poll)", &Network::Packet::IsDoneSync);
				VM->SetFunction("bool is_done_async(socket_poll)", &Network::Packet::IsDoneAsync);
				VM->SetFunction("bool is_timeout(socket_poll)", &Network::Packet::IsTimeout);
				VM->SetFunction("bool is_error(socket_poll)", &Network::Packet::IsError);
				VM->SetFunction("bool is_error_or_skip(socket_poll)", &Network::Packet::IsErrorOrSkip);
				VM->EndNamespace();

				auto VDNS = VM->SetClass<Network::DNS>("dns", false);
				VDNS->SetConstructor<Network::DNS>("dns@ f()");
				VDNS->SetMethodEx("string reverse_lookup(const string_view&in, const string_view&in = string_view())", &VI_EXPECTIFY(Network::DNS::ReverseLookup));
				VDNS->SetMethodEx("promise<string>@ reverse_lookup_deferred(const string_view&in, const string_view&in = string_view())", &VI_SPROMISIFY_REF(DNSReverseLookupDeferred, String));
				VDNS->SetMethodEx("string reverse_address_lookup(const socket_address&in)", &VI_EXPECTIFY(Network::DNS::ReverseAddressLookup));
				VDNS->SetMethodEx("promise<string>@ reverse_address_lookup_deferred(const socket_address&in)", &VI_SPROMISIFY_REF(DNSReverseAddressLookupDeferred, String));
				VDNS->SetMethodEx("socket_address lookup(const string_view&in, const string_view&in, dns_type, socket_protocol = socket_protocol::tcp, socket_type = socket_type::stream)", &VI_EXPECTIFY(Network::DNS::Lookup));
				VDNS->SetMethodEx("promise<socket_address>@ lookup_deferred(const string_view&in, const string_view&in, dns_type, socket_protocol = socket_protocol::tcp, socket_type = socket_type::stream)", &VI_SPROMISIFY_REF(DNSLookupDeferred, SocketAddress));
				VDNS->SetMethodStatic("dns@+ get()", &Network::DNS::Get);

				auto VMultiplexer = VM->SetClass<Network::Multiplexer>("multiplexer", false);
				VMultiplexer->SetFunctionDef("void poll_async(socket@+, socket_poll)");
				VMultiplexer->SetConstructor<Network::Multiplexer>("multiplexer@ f()");
				VMultiplexer->SetConstructor<Network::Multiplexer, uint64_t, size_t>("multiplexer@ f(uint64, usize)");
				VMultiplexer->SetMethod("void rescale(uint64, usize)", &Network::Multiplexer::Rescale);
				VMultiplexer->SetMethod("void activate()", &Network::Multiplexer::Activate);
				VMultiplexer->SetMethod("void deactivate()", &Network::Multiplexer::Deactivate);
				VMultiplexer->SetMethod("int dispatch(uint64)", &Network::Multiplexer::Dispatch);
				VMultiplexer->SetMethod("bool shutdown()", &Network::Multiplexer::Shutdown);
				VMultiplexer->SetMethodEx("bool when_readable(socket@+, poll_async@)", &MultiplexerWhenReadable);
				VMultiplexer->SetMethodEx("bool when_writeable(socket@+, poll_async@)", &MultiplexerWhenWriteable);
				VMultiplexer->SetMethod("bool cancel_events(socket@+, socket_poll = socket_poll::cancel, bool = true)", &Network::Multiplexer::CancelEvents);
				VMultiplexer->SetMethod("bool clear_events(socket@+)", &Network::Multiplexer::ClearEvents);
				VMultiplexer->SetMethod("bool is_listening()", &Network::Multiplexer::IsListening);
				VMultiplexer->SetMethod("usize get_activations()", &Network::Multiplexer::GetActivations);
				VMultiplexer->SetMethodStatic("multiplexer@+ get()", &Network::Multiplexer::Get);

				auto VUplinks = VM->SetClass<Network::Uplinks>("uplinks", false);
				VUplinks->SetConstructor<Network::Uplinks>("uplinks@ f()");
				VUplinks->SetMethod("void set_max_duplicates(usize)", &Network::Uplinks::SetMaxDuplicates);
				VUplinks->SetMethod("bool push_connection(const socket_address&in, socket@+)", &Network::Uplinks::PushConnection);
				VUplinks->SetMethodEx("promise<socket@>@ pop_connection(const socket_address&in)", &VI_PROMISIFY_REF(Network::Uplinks::PopConnection, Socket));
				VUplinks->SetMethod("usize max_duplicates() const", &Network::Uplinks::GetMaxDuplicates);
				VUplinks->SetMethod("usize size() const", &Network::Uplinks::GetSize);
				VUplinks->SetMethodStatic("uplinks@+ get()", &Network::Uplinks::Get);

				auto VSocketListener = VM->SetClass<Network::SocketListener>("socket_listener", true);
				VSocketListener->SetProperty<Network::SocketListener>("string name", &Network::SocketListener::Name);
				VSocketListener->SetProperty<Network::SocketListener>("socket_address source", &Network::SocketListener::Address);
				VSocketListener->SetProperty<Network::SocketListener>("socket@ stream", &Network::SocketListener::Stream);
				VSocketListener->SetGcConstructor<Network::SocketListener, SocketListener, const std::string_view&, const Network::SocketAddress&, bool>("socket_listener@ f(const string_view&in, const socket_address&in, bool)");
				VSocketListener->SetEnumRefsEx<Network::SocketListener>([](Network::SocketListener* Base, asIScriptEngine* VM)
				{
					FunctionFactory::GCEnumCallback(VM, Base->Stream);
				});
				VSocketListener->SetReleaseRefsEx<Network::SocketListener>([](Network::SocketListener* Base, asIScriptEngine*)
				{
					Base->~SocketListener();
				});

				auto VSocketRouter = VM->SetClass<Network::SocketRouter>("socket_router", false);
				VSocketRouter->SetProperty<Network::SocketRouter>("usize max_heap_buffer", &Network::SocketRouter::MaxHeapBuffer);
				VSocketRouter->SetProperty<Network::SocketRouter>("usize max_net_buffer", &Network::SocketRouter::MaxNetBuffer);
				VSocketRouter->SetProperty<Network::SocketRouter>("usize backlog_queue", &Network::SocketRouter::BacklogQueue);
				VSocketRouter->SetProperty<Network::SocketRouter>("usize socket_timeout", &Network::SocketRouter::SocketTimeout);
				VSocketRouter->SetProperty<Network::SocketRouter>("usize max_connections", &Network::SocketRouter::MaxConnections);
				VSocketRouter->SetProperty<Network::SocketRouter>("int64 keep_alive_max_count", &Network::SocketRouter::KeepAliveMaxCount);
				VSocketRouter->SetProperty<Network::SocketRouter>("int64 graceful_time_wait", &Network::SocketRouter::GracefulTimeWait);
				VSocketRouter->SetProperty<Network::SocketRouter>("bool enable_no_delay", &Network::SocketRouter::EnableNoDelay);
				VSocketRouter->SetConstructor<Network::SocketRouter>("socket_router@ f()");
				VSocketRouter->SetMethodEx("void listen(const string_view&in, const string_view&in, bool = false)", &SocketRouterListen1);
				VSocketRouter->SetMethodEx("void listen(const string_view&in, const string_view&in, const string_view&in, bool = false)", &SocketRouterListen2);
				VSocketRouter->SetMethodEx("void set_listeners(dictionary@ data)", &SocketRouterSetListeners);
				VSocketRouter->SetMethodEx("dictionary@ get_listeners() const", &SocketRouterGetListeners);
				VSocketRouter->SetMethodEx("void set_certificates(dictionary@ data)", &SocketRouterSetCertificates);
				VSocketRouter->SetMethodEx("dictionary@ get_certificates() const", &SocketRouterGetCertificates);

				auto VSocketConnection = VM->SetClass<Network::SocketConnection>("socket_connection", true);
				VSocketConnection->SetProperty<Network::SocketConnection>("socket@ stream", &Network::SocketConnection::Stream);
				VSocketConnection->SetProperty<Network::SocketConnection>("socket_listener@ host", &Network::SocketConnection::Host);
				VSocketConnection->SetProperty<Network::SocketConnection>("socket_address address", &Network::SocketConnection::Address);
				VSocketConnection->SetProperty<Network::SocketConnection>("socket_data_frame info", &Network::SocketConnection::Info);
				VSocketConnection->SetGcConstructor<Network::SocketConnection, SocketConnection>("socket_connection@ f()");
				VSocketConnection->SetMethod("void reset(bool)", &Network::SocketConnection::Reset);
				VSocketConnection->SetMethod<Network::SocketConnection, bool>("bool next()", &Network::SocketConnection::Next);
				VSocketConnection->SetMethod<Network::SocketConnection, bool, int>("bool next(int32)", &Network::SocketConnection::Next);
				VSocketConnection->SetMethod<Network::SocketConnection, bool>("bool abort()", &Network::SocketConnection::Abort);
				VSocketConnection->SetMethodEx("bool abort(int, const string_view&in)", &SocketConnectionAbort);
				VSocketConnection->SetEnumRefsEx<Network::SocketConnection>([](Network::SocketConnection* Base, asIScriptEngine* VM)
				{
					FunctionFactory::GCEnumCallback(VM, Base->Stream);
					FunctionFactory::GCEnumCallback(VM, Base->Host);
				});
				VSocketConnection->SetReleaseRefsEx<Network::SocketConnection>([](Network::SocketConnection* Base, asIScriptEngine*)
				{
					Base->~SocketConnection();
				});

				auto VSocketServer = VM->SetClass<Network::SocketServer>("socket_server", true);
				VSocketServer->SetGcConstructor<Network::SocketServer, SocketServer>("socket_server@ f()");
				VSocketServer->SetMethodEx("void set_router(socket_router@+)", &SocketServerSetRouter);
				VSocketServer->SetMethodEx("bool configure(socket_router@+)", &SocketServerConfigure);
				VSocketServer->SetMethodEx("bool listen()", &SocketServerListen);
				VSocketServer->SetMethodEx("bool unlisten(bool)", &SocketServerUnlisten);
				VSocketServer->SetMethod("void set_backlog(usize)", &Network::SocketServer::SetBacklog);
				VSocketServer->SetMethod("usize get_backlog() const", &Network::SocketServer::GetBacklog);
				VSocketServer->SetMethod("server_state get_state() const", &Network::SocketServer::GetState);
				VSocketServer->SetMethod("socket_router@+ get_router() const", &Network::SocketServer::GetRouter);
				VSocketServer->SetEnumRefsEx<Network::SocketServer>([](Network::SocketServer* Base, asIScriptEngine* VM)
				{
					FunctionFactory::GCEnumCallback(VM, Base->GetRouter());
					for (auto* Item : Base->GetActiveClients())
						FunctionFactory::GCEnumCallback(VM, Item);

					for (auto* Item : Base->GetPooledClients())
						FunctionFactory::GCEnumCallback(VM, Item);
				});
				VSocketServer->SetReleaseRefsEx<Network::SocketServer>([](Network::SocketServer* Base, asIScriptEngine*)
				{
					Base->~SocketServer();
				});

				auto VSocketClient = VM->SetClass<Network::SocketClient>("socket_client", false);
				VSocketClient->SetConstructor<Network::SocketClient, int64_t>("socket_client@ f(int64)");
				VSocketClient->SetMethodEx("promise<bool>@ connect_sync(const socket_address&in, int32 = -1)", &VI_SPROMISIFY(SocketClientConnectSync, TypeId::BOOL));
				VSocketClient->SetMethodEx("promise<bool>@ connect_async(const socket_address&in, int32 = -1)", &VI_SPROMISIFY(SocketClientConnectAsync, TypeId::BOOL));
				VSocketClient->SetMethodEx("promise<bool>@ disconnect()", &VI_SPROMISIFY(SocketClientDisconnect, TypeId::BOOL));
				VSocketClient->SetMethod("void apply_reusability(bool)", &Network::SocketClient::ApplyReusability);
				VSocketClient->SetMethod("bool has_stream() const", &Network::SocketClient::HasStream);
				VSocketClient->SetMethod("bool is_secure() const", &Network::SocketClient::IsSecure);
				VSocketClient->SetMethod("bool is_verified() const", &Network::SocketClient::IsVerified);
				VSocketClient->SetMethod("const socket_address& get_peer_address() const", &Network::SocketClient::GetPeerAddress);
				VSocketClient->SetMethod("socket@+ get_stream() const", &Network::SocketClient::GetStream);

				return true;
#else
				VI_ASSERT(false, "<network> is not loaded");
				return false;
#endif
			}
			bool Registry::ImportHTTP(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr, "manager should be set");
				VI_TYPEREF(RouterGroup, "http::route_group");
				VI_TYPEREF(SiteEntry, "http::site_entry");
				VI_TYPEREF(MapRouter, "http::map_router");
				VI_TYPEREF(Server, "http::server");
				VI_TYPEREF(Connection, "http::connection");
				VI_TYPEREF(ResponseFrame, "http::response_frame");
				VI_TYPEREF(ArrayResourceInfo, "array<http::resource_info>@");
				VI_TYPEREF(String, "string");
				VI_TYPEREF(Schema, "schema");

				VM->BeginNamespace("http");
				auto VAuth = VM->SetEnum("auth");
				VAuth->SetValue("granted", (int)Network::HTTP::Auth::Granted);
				VAuth->SetValue("denied", (int)Network::HTTP::Auth::Denied);
				VAuth->SetValue("unverified", (int)Network::HTTP::Auth::Unverified);

				auto VWebSocketOp = VM->SetEnum("websocket_op");
				VWebSocketOp->SetValue("next", (int)Network::HTTP::WebSocketOp::Continue);
				VWebSocketOp->SetValue("text", (int)Network::HTTP::WebSocketOp::Text);
				VWebSocketOp->SetValue("binary", (int)Network::HTTP::WebSocketOp::Binary);
				VWebSocketOp->SetValue("close", (int)Network::HTTP::WebSocketOp::Close);
				VWebSocketOp->SetValue("ping", (int)Network::HTTP::WebSocketOp::Ping);
				VWebSocketOp->SetValue("pong", (int)Network::HTTP::WebSocketOp::Pong);

				auto VWebSocketState = VM->SetEnum("websocket_state");
				VWebSocketState->SetValue("open", (int)Network::HTTP::WebSocketState::Open);
				VWebSocketState->SetValue("receive", (int)Network::HTTP::WebSocketState::Receive);
				VWebSocketState->SetValue("process", (int)Network::HTTP::WebSocketState::Process);
				VWebSocketState->SetValue("close", (int)Network::HTTP::WebSocketState::Close);

				auto VCompressionTune = VM->SetEnum("compression_tune");
				VCompressionTune->SetValue("filtered", (int)Network::HTTP::CompressionTune::Filtered);
				VCompressionTune->SetValue("huffman", (int)Network::HTTP::CompressionTune::Huffman);
				VCompressionTune->SetValue("rle", (int)Network::HTTP::CompressionTune::Rle);
				VCompressionTune->SetValue("fixed", (int)Network::HTTP::CompressionTune::Fixed);
				VCompressionTune->SetValue("defaults", (int)Network::HTTP::CompressionTune::Default);

				auto VRouteMode = VM->SetEnum("route_mode");
				VRouteMode->SetValue("start", (int)Network::HTTP::RouteMode::Start);
				VRouteMode->SetValue("match", (int)Network::HTTP::RouteMode::Match);
				VRouteMode->SetValue("end", (int)Network::HTTP::RouteMode::End);

				auto VErrorFile = VM->SetStructTrivial<Network::HTTP::ErrorFile>("error_file");
				VErrorFile->SetProperty<Network::HTTP::ErrorFile>("string pattern", &Network::HTTP::ErrorFile::Pattern);
				VErrorFile->SetProperty<Network::HTTP::ErrorFile>("int32 status_code", &Network::HTTP::ErrorFile::StatusCode);
				VErrorFile->SetConstructor<Network::HTTP::ErrorFile>("void f()");

				auto VMimeType = VM->SetStructTrivial<Network::HTTP::MimeType>("mime_type");
				VMimeType->SetProperty<Network::HTTP::MimeType>("string extension", &Network::HTTP::MimeType::Extension);
				VMimeType->SetProperty<Network::HTTP::MimeType>("string type", &Network::HTTP::MimeType::Type);
				VMimeType->SetConstructor<Network::HTTP::MimeType>("void f()");

				auto VCredentials = VM->SetStructTrivial<Network::HTTP::Credentials>("credentials");
				VCredentials->SetProperty<Network::HTTP::Credentials>("string token", &Network::HTTP::Credentials::Token);
				VCredentials->SetProperty<Network::HTTP::Credentials>("auth type", &Network::HTTP::Credentials::Type);
				VCredentials->SetConstructor<Network::HTTP::Credentials>("void f()");

				auto VResource = VM->SetStructTrivial<Network::HTTP::Resource>("resource_info");
				VResource->SetProperty<Network::HTTP::Resource>("string path", &Network::HTTP::Resource::Path);
				VResource->SetProperty<Network::HTTP::Resource>("string type", &Network::HTTP::Resource::Type);
				VResource->SetProperty<Network::HTTP::Resource>("string name", &Network::HTTP::Resource::Name);
				VResource->SetProperty<Network::HTTP::Resource>("string key", &Network::HTTP::Resource::Key);
				VResource->SetProperty<Network::HTTP::Resource>("usize length", &Network::HTTP::Resource::Length);
				VResource->SetProperty<Network::HTTP::Resource>("bool is_in_memory", &Network::HTTP::Resource::IsInMemory);
				VResource->SetConstructor<Network::HTTP::Resource>("void f()");
				VResource->SetMethod("string& put_header(const string_view&in, const string_view&in)", &Network::HTTP::Resource::PutHeader);
				VResource->SetMethod("string& set_header(const string_view&in, const string_view&in)", &Network::HTTP::Resource::SetHeader);
				VResource->SetMethod("string compose_header(const string_view&in) const", &Network::HTTP::Resource::ComposeHeader);
				VResource->SetMethodEx("string get_header(const string_view&in) const", &ResourceGetHeaderBlob);
				VResource->SetMethod("const string& get_in_memory_contents() const", &Network::HTTP::Resource::GetInMemoryContents);

				auto VCookie = VM->SetStructTrivial<Network::HTTP::Cookie>("cookie");
				VCookie->SetProperty<Network::HTTP::Cookie>("string name", &Network::HTTP::Cookie::Name);
				VCookie->SetProperty<Network::HTTP::Cookie>("string value", &Network::HTTP::Cookie::Value);
				VCookie->SetProperty<Network::HTTP::Cookie>("string domain", &Network::HTTP::Cookie::Domain);
				VCookie->SetProperty<Network::HTTP::Cookie>("string same_site", &Network::HTTP::Cookie::SameSite);
				VCookie->SetProperty<Network::HTTP::Cookie>("string expires", &Network::HTTP::Cookie::Expires);
				VCookie->SetProperty<Network::HTTP::Cookie>("bool secure", &Network::HTTP::Cookie::Secure);
				VCookie->SetProperty<Network::HTTP::Cookie>("bool http_only", &Network::HTTP::Cookie::HttpOnly);
				VCookie->SetConstructor<Network::HTTP::Cookie>("void f()");
				VCookie->SetMethod("void set_expires(int64)", &Network::HTTP::Cookie::SetExpires);
				VCookie->SetMethod("void set_expired()", &Network::HTTP::Cookie::SetExpired);

				auto VRequestFrame = VM->SetStructTrivial<Network::HTTP::RequestFrame>("request_frame");
				auto VResponseFrame = VM->SetStructTrivial<Network::HTTP::ResponseFrame>("response_frame");
				auto VFetchFrame = VM->SetStructTrivial<Network::HTTP::FetchFrame>("fetch_frame");
				auto VContentFrame = VM->SetStructTrivial<Network::HTTP::ContentFrame>("content_frame");
				VContentFrame->SetProperty<Network::HTTP::ContentFrame>("usize length", &Network::HTTP::ContentFrame::Length);
				VContentFrame->SetProperty<Network::HTTP::ContentFrame>("usize offset", &Network::HTTP::ContentFrame::Offset);
				VContentFrame->SetProperty<Network::HTTP::ContentFrame>("bool exceeds", &Network::HTTP::ContentFrame::Exceeds);
				VContentFrame->SetProperty<Network::HTTP::ContentFrame>("bool limited", &Network::HTTP::ContentFrame::Limited);
				VContentFrame->SetConstructor<Network::HTTP::ContentFrame>("void f()");
				VContentFrame->SetMethod<Network::HTTP::ContentFrame, void, const std::string_view&>("void append(const string_view&in)", &Network::HTTP::ContentFrame::Append);
				VContentFrame->SetMethod<Network::HTTP::ContentFrame, void, const std::string_view&>("void assign(const string_view&in)", &Network::HTTP::ContentFrame::Assign);
				VContentFrame->SetMethod("void finalize()", &Network::HTTP::ContentFrame::Finalize);
				VContentFrame->SetMethod("void cleanup()", &Network::HTTP::ContentFrame::Cleanup);
				VContentFrame->SetMethod("bool is_finalized() const", &Network::HTTP::ContentFrame::IsFinalized);
				VContentFrame->SetMethod("string get_text() const", &Network::HTTP::ContentFrame::GetText);
				VContentFrame->SetMethodEx("schema@ get_json() const", &ContentFrameGetJSON);
				VContentFrame->SetMethodEx("schema@ get_xml() const", &ContentFrameGetXML);
				VContentFrame->SetMethodEx("void prepare(const request_frame&in, const string_view&in)", &ContentFramePrepare1);
				VContentFrame->SetMethodEx("void prepare(const response_frame&in, const string_view&in)", &ContentFramePrepare2);
				VContentFrame->SetMethodEx("void prepare(const fetch_frame&in, const string_view&in)", &ContentFramePrepare3);
				VContentFrame->SetMethodEx("void add_resource(const resource_info&in)", &ContentFrameAddResource);
				VContentFrame->SetMethodEx("void clear_resources()", &ContentFrameClearResources);
				VContentFrame->SetMethodEx("resource_info get_resource(usize) const", &ContentFrameGetResource);
				VContentFrame->SetMethodEx("usize get_resources_size() const", &ContentFrameGetResourcesSize);

				VRequestFrame->SetProperty<Network::HTTP::RequestFrame>("content_frame content", &Network::HTTP::RequestFrame::Content);
				VRequestFrame->SetProperty<Network::HTTP::RequestFrame>("string query", &Network::HTTP::RequestFrame::Query);
				VRequestFrame->SetProperty<Network::HTTP::RequestFrame>("string path", &Network::HTTP::RequestFrame::Path);
				VRequestFrame->SetProperty<Network::HTTP::RequestFrame>("string location", &Network::HTTP::RequestFrame::Location);
				VRequestFrame->SetProperty<Network::HTTP::RequestFrame>("string referrer", &Network::HTTP::RequestFrame::Referrer);
				VRequestFrame->SetProperty<Network::HTTP::RequestFrame>("regex_result match", &Network::HTTP::RequestFrame::Match);
				VRequestFrame->SetProperty<Network::HTTP::RequestFrame>("credentials user", &Network::HTTP::RequestFrame::User);
				VRequestFrame->SetConstructor<Network::HTTP::RequestFrame>("void f()");
				VRequestFrame->SetMethod("string& put_header(const string_view&in, const string_view&in)", &Network::HTTP::RequestFrame::PutHeader);
				VRequestFrame->SetMethod("string& set_header(const string_view&in, const string_view&in)", &Network::HTTP::RequestFrame::SetHeader);
				VRequestFrame->SetMethod("void set_version(uint32, uint32)", &Network::HTTP::RequestFrame::SetVersion);
				VRequestFrame->SetMethod("void cleanup()", &Network::HTTP::RequestFrame::Cleanup);
				VRequestFrame->SetMethod("string compose_header(const string_view&in) const", &Network::HTTP::RequestFrame::ComposeHeader);
				VRequestFrame->SetMethodEx("string get_cookie(const string_view&in) const", &RequestFrameGetCookieBlob);
				VRequestFrame->SetMethodEx("string get_header(const string_view&in) const", &RequestFrameGetHeaderBlob);
				VRequestFrame->SetMethodEx("string get_header(usize, usize = 0) const", &RequestFrameGetHeader);
				VRequestFrame->SetMethodEx("string get_cookie(usize, usize = 0) const", &RequestFrameGetCookie);
				VRequestFrame->SetMethodEx("usize get_headers_size() const", &RequestFrameGetHeadersSize);
				VRequestFrame->SetMethodEx("usize get_header_size(usize) const", &RequestFrameGetHeaderSize);
				VRequestFrame->SetMethodEx("usize get_cookies_size() const", &RequestFrameGetCookiesSize);
				VRequestFrame->SetMethodEx("usize get_cookie_size(usize) const", &RequestFrameGetCookieSize);
				VRequestFrame->SetMethodEx("void set_method(const string_view&in)", &RequestFrameSetMethod);
				VRequestFrame->SetMethodEx("string get_method() const", &RequestFrameGetMethod);
				VRequestFrame->SetMethodEx("string get_version() const", &RequestFrameGetVersion);

				VResponseFrame->SetProperty<Network::HTTP::ResponseFrame>("content_frame content", &Network::HTTP::ResponseFrame::Content);
				VResponseFrame->SetProperty<Network::HTTP::ResponseFrame>("int32 status_code", &Network::HTTP::ResponseFrame::StatusCode);
				VResponseFrame->SetProperty<Network::HTTP::ResponseFrame>("bool error", &Network::HTTP::ResponseFrame::Error);
				VResponseFrame->SetConstructor<Network::HTTP::ResponseFrame>("void f()");
				VResponseFrame->SetMethod("string& put_header(const string_view&in, const string_view&in)", &Network::HTTP::ResponseFrame::PutHeader);
				VResponseFrame->SetMethod("string& set_header(const string_view&in, const string_view&in)", &Network::HTTP::ResponseFrame::SetHeader);
				VResponseFrame->SetMethod<Network::HTTP::ResponseFrame, void, const Network::HTTP::Cookie&>("void set_cookie(const cookie&in)", &Network::HTTP::ResponseFrame::SetCookie);
				VResponseFrame->SetMethod("void cleanup()", &Network::HTTP::ResponseFrame::Cleanup);
				VResponseFrame->SetMethod("string compose_header(const string_view&in) const", &Network::HTTP::ResponseFrame::ComposeHeader);
				VResponseFrame->SetMethodEx("string get_header(const string_view&in) const", &ResponseFrameGetHeaderBlob);
				VResponseFrame->SetMethod("bool is_undefined() const", &Network::HTTP::ResponseFrame::IsUndefined);
				VResponseFrame->SetMethod("bool is_ok() const", &Network::HTTP::ResponseFrame::IsOK);
				VResponseFrame->SetMethodEx("string get_header(usize, usize) const", &ResponseFrameGetHeader);
				VResponseFrame->SetMethodEx("cookie get_cookie(const string_view&in) const", &ResponseFrameGetCookie1);
				VResponseFrame->SetMethodEx("cookie get_cookie(usize) const", &ResponseFrameGetCookie2);
				VResponseFrame->SetMethodEx("usize get_headers_size() const", &ResponseFrameGetHeadersSize);
				VResponseFrame->SetMethodEx("usize get_header_size(usize) const", &ResponseFrameGetHeaderSize);
				VResponseFrame->SetMethodEx("usize get_cookies_size() const", &ResponseFrameGetCookiesSize);

				VFetchFrame->SetProperty<Network::HTTP::FetchFrame>("content_frame content", &Network::HTTP::FetchFrame::Content);
				VFetchFrame->SetProperty<Network::HTTP::FetchFrame>("uint64 timeout", &Network::HTTP::FetchFrame::Timeout);
				VFetchFrame->SetProperty<Network::HTTP::FetchFrame>("uint32 verify_peers", &Network::HTTP::FetchFrame::VerifyPeers);
				VFetchFrame->SetProperty<Network::HTTP::FetchFrame>("usize max_size", &Network::HTTP::FetchFrame::MaxSize);
				VFetchFrame->SetConstructor<Network::HTTP::FetchFrame>("void f()");
				VFetchFrame->SetMethod("void put_header(const string_view&in, const string_view&in)", &Network::HTTP::FetchFrame::PutHeader);
				VFetchFrame->SetMethod("void set_header(const string_view&in, const string_view&in)", &Network::HTTP::FetchFrame::SetHeader);
				VFetchFrame->SetMethod("void cleanup()", &Network::HTTP::FetchFrame::Cleanup);
				VFetchFrame->SetMethod("string compose_header(const string_view&in) const", &Network::HTTP::FetchFrame::ComposeHeader);
				VFetchFrame->SetMethodEx("string get_cookie(const string_view&in) const", &FetchFrameGetCookieBlob);
				VFetchFrame->SetMethodEx("string get_header(const string_view&in) const", &FetchFrameGetHeaderBlob);
				VFetchFrame->SetMethodEx("string get_header(usize, usize = 0) const", &FetchFrameGetHeader);
				VFetchFrame->SetMethodEx("string get_cookie(usize, usize = 0) const", &FetchFrameGetCookie);
				VFetchFrame->SetMethodEx("usize get_headers_size() const", &FetchFrameGetHeadersSize);
				VFetchFrame->SetMethodEx("usize get_header_size(usize) const", &FetchFrameGetHeaderSize);
				VFetchFrame->SetMethodEx("usize get_cookies_size() const", &FetchFrameGetCookiesSize);
				VFetchFrame->SetMethodEx("usize get_cookie_size(usize) const", &FetchFrameGetCookieSize);

				auto VRouteAuth = VM->SetStructTrivial<Network::HTTP::RouterEntry::EntryAuth>("route_auth");
				VRouteAuth->SetProperty<Network::HTTP::RouterEntry::EntryAuth>("string type", &Network::HTTP::RouterEntry::EntryAuth::Type);
				VRouteAuth->SetProperty<Network::HTTP::RouterEntry::EntryAuth>("string realm", &Network::HTTP::RouterEntry::EntryAuth::Realm);
				VRouteAuth->SetConstructor<Network::HTTP::RouterEntry::EntryAuth>("void f()");
				VRouteAuth->SetMethodEx("void set_methods(array<string>@+)", &RouteAuthSetMethods);
				VRouteAuth->SetMethodEx("array<string>@ get_methods() const", &RouteAuthGetMethods);

				auto VRouteCompression = VM->SetStructTrivial<Network::HTTP::RouterEntry::EntryCompression>("route_compression");
				VRouteCompression->SetProperty<Network::HTTP::RouterEntry::EntryCompression>("compression_tune tune", &Network::HTTP::RouterEntry::EntryCompression::Tune);
				VRouteCompression->SetProperty<Network::HTTP::RouterEntry::EntryCompression>("int32 quality_level", &Network::HTTP::RouterEntry::EntryCompression::QualityLevel);
				VRouteCompression->SetProperty<Network::HTTP::RouterEntry::EntryCompression>("int32 memory_level", &Network::HTTP::RouterEntry::EntryCompression::MemoryLevel);
				VRouteCompression->SetProperty<Network::HTTP::RouterEntry::EntryCompression>("usize min_length", &Network::HTTP::RouterEntry::EntryCompression::MinLength);
				VRouteCompression->SetProperty<Network::HTTP::RouterEntry::EntryCompression>("bool enabled", &Network::HTTP::RouterEntry::EntryCompression::Enabled);
				VRouteCompression->SetConstructor<Network::HTTP::RouterEntry::EntryCompression>("void f()");
				VRouteCompression->SetMethodEx("void set_files(array<regex_source>@+)", &RouteCompressionSetFiles);
				VRouteCompression->SetMethodEx("array<regex_source>@ get_files() const", &RouteCompressionGetFiles);

				auto VMapRouter = VM->SetClass<Network::HTTP::MapRouter>("map_router", true);
				auto VRouterEntry = VM->SetClass<Network::HTTP::RouterEntry>("route_entry", false);
				VRouterEntry->SetProperty<Network::HTTP::RouterEntry>("route_auth auths", &Network::HTTP::RouterEntry::Auth);
				VRouterEntry->SetProperty<Network::HTTP::RouterEntry>("route_compression compressions", &Network::HTTP::RouterEntry::Compression);
				VRouterEntry->SetProperty<Network::HTTP::RouterEntry>("string files_directory", &Network::HTTP::RouterEntry::FilesDirectory);
				VRouterEntry->SetProperty<Network::HTTP::RouterEntry>("string char_set", &Network::HTTP::RouterEntry::CharSet);
				VRouterEntry->SetProperty<Network::HTTP::RouterEntry>("string proxy_ip_address", &Network::HTTP::RouterEntry::ProxyIpAddress);
				VRouterEntry->SetProperty<Network::HTTP::RouterEntry>("string access_control_allow_origin", &Network::HTTP::RouterEntry::AccessControlAllowOrigin);
				VRouterEntry->SetProperty<Network::HTTP::RouterEntry>("string redirect", &Network::HTTP::RouterEntry::Redirect);
				VRouterEntry->SetProperty<Network::HTTP::RouterEntry>("string alias", &Network::HTTP::RouterEntry::Alias);
				VRouterEntry->SetProperty<Network::HTTP::RouterEntry>("usize websocket_timeout", &Network::HTTP::RouterEntry::WebSocketTimeout);
				VRouterEntry->SetProperty<Network::HTTP::RouterEntry>("usize static_file_max_age", &Network::HTTP::RouterEntry::StaticFileMaxAge);
				VRouterEntry->SetProperty<Network::HTTP::RouterEntry>("usize level", &Network::HTTP::RouterEntry::Level);
				VRouterEntry->SetProperty<Network::HTTP::RouterEntry>("bool allow_directory_listing", &Network::HTTP::RouterEntry::AllowDirectoryListing);
				VRouterEntry->SetProperty<Network::HTTP::RouterEntry>("bool allow_websocket", &Network::HTTP::RouterEntry::AllowWebSocket);
				VRouterEntry->SetProperty<Network::HTTP::RouterEntry>("bool allow_send_file", &Network::HTTP::RouterEntry::AllowSendFile);
				VRouterEntry->SetProperty<Network::HTTP::RouterEntry>("regex_source location", &Network::HTTP::RouterEntry::Location);
				VRouterEntry->SetConstructor<Network::HTTP::RouterEntry>("route_entry@ f()");
				VRouterEntry->SetMethodEx("void set_hidden_files(array<regex_source>@+)", &RouterEntrySetHiddenFiles);
				VRouterEntry->SetMethodEx("array<regex_source>@ get_hidden_files() const", &RouterEntryGetHiddenFiles);
				VRouterEntry->SetMethodEx("void set_error_files(array<error_file>@+)", &RouterEntrySetErrorFiles);
				VRouterEntry->SetMethodEx("array<error_file>@ get_error_files() const", &RouterEntryGetErrorFiles);
				VRouterEntry->SetMethodEx("void set_mime_types(array<mime_type>@+)", &RouterEntrySetMimeTypes);
				VRouterEntry->SetMethodEx("array<mime_type>@ get_mime_types() const", &RouterEntryGetMimeTypes);
				VRouterEntry->SetMethodEx("void set_index_files(array<string>@+)", &RouterEntrySetIndexFiles);
				VRouterEntry->SetMethodEx("array<string>@ get_index_files() const", &RouterEntryGetIndexFiles);
				VRouterEntry->SetMethodEx("void set_try_files(array<string>@+)", &RouterEntrySetTryFiles);
				VRouterEntry->SetMethodEx("array<string>@ get_try_files() const", &RouterEntryGetTryFiles);
				VRouterEntry->SetMethodEx("void set_disallowed_methods_files(array<string>@+)", &RouterEntrySetDisallowedMethodsFiles);
				VRouterEntry->SetMethodEx("array<string>@ get_disallowed_methods_files() const", &RouterEntryGetDisallowedMethodsFiles);
				VRouterEntry->SetMethodEx("map_router@+ get_router() const", &RouterEntryGetRouter);

				auto VRouterGroup = VM->SetClass<Network::HTTP::RouterGroup>("route_group", false);
				VRouterGroup->SetProperty<Network::HTTP::RouterGroup>("string match", &Network::HTTP::RouterGroup::Match);
				VRouterGroup->SetProperty<Network::HTTP::RouterGroup>("route_mode mode", &Network::HTTP::RouterGroup::Mode);
				VRouterGroup->SetGcConstructor<Network::HTTP::RouterGroup, RouterGroup, const std::string_view&, Network::HTTP::RouteMode>("route_group@ f(const string_view&in, route_mode)");
				VRouterGroup->SetMethodEx("route_entry@+ get_route(usize) const", &RouterGroupGetRoute);
				VRouterGroup->SetMethodEx("usize get_routes_size() const", &RouterGroupGetRoutesSize);

				auto VRouterCookie = VM->SetStructTrivial<Network::HTTP::MapRouter::RouterSession::RouterCookie>("router_cookie");
				VRouterCookie->SetProperty<Network::HTTP::MapRouter::RouterSession::RouterCookie>("string name", &Network::HTTP::MapRouter::RouterSession::RouterCookie::Name);
				VRouterCookie->SetProperty<Network::HTTP::MapRouter::RouterSession::RouterCookie>("string domain", &Network::HTTP::MapRouter::RouterSession::RouterCookie::Domain);
				VRouterCookie->SetProperty<Network::HTTP::MapRouter::RouterSession::RouterCookie>("string path", &Network::HTTP::MapRouter::RouterSession::RouterCookie::Path);
				VRouterCookie->SetProperty<Network::HTTP::MapRouter::RouterSession::RouterCookie>("string same_site", &Network::HTTP::MapRouter::RouterSession::RouterCookie::SameSite);
				VRouterCookie->SetProperty<Network::HTTP::MapRouter::RouterSession::RouterCookie>("uint64 expires", &Network::HTTP::MapRouter::RouterSession::RouterCookie::Expires);
				VRouterCookie->SetProperty<Network::HTTP::MapRouter::RouterSession::RouterCookie>("bool secure", &Network::HTTP::MapRouter::RouterSession::RouterCookie::Secure);
				VRouterCookie->SetProperty<Network::HTTP::MapRouter::RouterSession::RouterCookie>("bool http_only", &Network::HTTP::MapRouter::RouterSession::RouterCookie::HttpOnly);
				VRouterCookie->SetConstructor<Network::HTTP::MapRouter::RouterSession::RouterCookie>("void f()");

				auto VRouterSession = VM->SetStructTrivial<Network::HTTP::MapRouter::RouterSession>("router_session");
				VRouterSession->SetProperty<Network::HTTP::MapRouter::RouterSession>("router_cookie cookie", &Network::HTTP::MapRouter::RouterSession::Cookie);
				VRouterSession->SetProperty<Network::HTTP::MapRouter::RouterSession>("string directory", &Network::HTTP::MapRouter::RouterSession::Directory);
				VRouterSession->SetProperty<Network::HTTP::MapRouter::RouterSession>("uint64 expires", &Network::HTTP::MapRouter::RouterSession::Expires);
				VRouterSession->SetConstructor<Network::HTTP::MapRouter::RouterSession>("void f()");

				auto VConnection = VM->SetClass<Network::HTTP::Connection>("connection", false);
				auto VWebSocketFrame = VM->SetClass<Network::HTTP::WebSocketFrame>("websocket_frame", false);
				VWebSocketFrame->SetFunctionDef("void status_async(websocket_frame@+)");
				VWebSocketFrame->SetFunctionDef("void recv_async(websocket_frame@+, websocket_op, const string&in)");
				VWebSocketFrame->SetMethodEx("bool set_on_connect(status_async@+)", &WebSocketFrameSetOnConnect);
				VWebSocketFrame->SetMethodEx("bool set_on_before_disconnect(status_async@+)", &WebSocketFrameSetOnBeforeDisconnect);
				VWebSocketFrame->SetMethodEx("bool set_on_disconnect(status_async@+)", &WebSocketFrameSetOnDisconnect);
				VWebSocketFrame->SetMethodEx("bool set_on_receive(recv_async@+)", &WebSocketFrameSetOnReceive);
				VWebSocketFrame->SetMethodEx("promise<bool>@ send(const string_view&in, websocket_op)", &VI_SPROMISIFY(WebSocketFrameSend1, TypeId::BOOL));
				VWebSocketFrame->SetMethodEx("promise<bool>@ send(uint32, const string_view&in, websocket_op)", &VI_SPROMISIFY(WebSocketFrameSend2, TypeId::BOOL));
				VWebSocketFrame->SetMethodEx("promise<bool>@ send_close()", &VI_SPROMISIFY(WebSocketFrameSendClose, TypeId::BOOL));
				VWebSocketFrame->SetMethod("void next()", &Network::HTTP::WebSocketFrame::Next);
				VWebSocketFrame->SetMethod("bool is_finished() const", &Network::HTTP::WebSocketFrame::IsFinished);
				VWebSocketFrame->SetMethod("socket@+ get_stream() const", &Network::HTTP::WebSocketFrame::GetStream);
				VWebSocketFrame->SetMethod("connection@+ get_connection() const", &Network::HTTP::WebSocketFrame::GetConnection);

				VMapRouter->SetProperty<Network::SocketRouter>("usize max_heap_buffer", &Network::SocketRouter::MaxHeapBuffer);
				VMapRouter->SetProperty<Network::SocketRouter>("usize max_net_buffer", &Network::SocketRouter::MaxNetBuffer);
				VMapRouter->SetProperty<Network::SocketRouter>("usize backlog_queue", &Network::SocketRouter::BacklogQueue);
				VMapRouter->SetProperty<Network::SocketRouter>("usize socket_timeout", &Network::SocketRouter::SocketTimeout);
				VMapRouter->SetProperty<Network::SocketRouter>("usize max_connections", &Network::SocketRouter::MaxConnections);
				VMapRouter->SetProperty<Network::SocketRouter>("int64 keep_alive_max_count", &Network::SocketRouter::KeepAliveMaxCount);
				VMapRouter->SetProperty<Network::SocketRouter>("int64 graceful_time_wait", &Network::SocketRouter::GracefulTimeWait);
				VMapRouter->SetProperty<Network::SocketRouter>("bool enable_no_delay", &Network::SocketRouter::EnableNoDelay);
				VMapRouter->SetProperty<Network::HTTP::MapRouter>("router_session session", &Network::HTTP::MapRouter::Session);
				VMapRouter->SetProperty<Network::HTTP::MapRouter>("string temporary_directory", &Network::HTTP::MapRouter::TemporaryDirectory);
				VMapRouter->SetProperty<Network::HTTP::MapRouter>("usize max_uploadable_resources", &Network::HTTP::MapRouter::MaxUploadableResources);
				VMapRouter->SetGcConstructor<Network::HTTP::MapRouter, MapRouter>("map_router@ f()");
				VMapRouter->SetMethodEx("void listen(const string_view&in, const string_view&in, bool = false)", &SocketRouterListen1);
				VMapRouter->SetMethodEx("void listen(const string_view&in, const string_view&in, const string_view&in, bool = false)", &SocketRouterListen2);
				VMapRouter->SetMethodEx("void set_listeners(dictionary@ data)", &SocketRouterSetListeners);
				VMapRouter->SetMethodEx("dictionary@ get_listeners() const", &SocketRouterGetListeners);
				VMapRouter->SetMethodEx("void set_certificates(dictionary@ data)", &SocketRouterSetCertificates);
				VMapRouter->SetMethodEx("dictionary@ get_certificates() const", &SocketRouterGetCertificates);
				VMapRouter->SetFunctionDef("void route_async(connection@+)");
				VMapRouter->SetFunctionDef("void authorize_sync(connection@+, credentials&in)");
				VMapRouter->SetFunctionDef("void headers_sync(connection@+, string&out)");
				VMapRouter->SetFunctionDef("void websocket_status_async(websocket_frame@+)");
				VMapRouter->SetFunctionDef("void websocket_recv_async(websocket_frame@+, websocket_op, const string&in)");
				VMapRouter->SetMethod("void sort()", &Network::HTTP::MapRouter::Sort);
				VMapRouter->SetMethod("route_group@+ group(const string_view&in, route_mode)", &Network::HTTP::MapRouter::Group);
				VMapRouter->SetMethod<Network::HTTP::MapRouter, Network::HTTP::RouterEntry*, const std::string_view&, Network::HTTP::RouteMode, const std::string_view&, bool>("route_entry@+ route(const string_view&in, route_mode, const string_view&in, bool)", &Network::HTTP::MapRouter::Route);
				VMapRouter->SetMethod<Network::HTTP::MapRouter, Network::HTTP::RouterEntry*, const std::string_view&, Network::HTTP::RouterGroup*, Network::HTTP::RouterEntry*>("route_entry@+ route(const string_view&in, route_group@+, route_entry@+)", &Network::HTTP::MapRouter::Route);
				VMapRouter->SetMethod("bool remove(route_entry@+)", &Network::HTTP::MapRouter::Remove);
				VMapRouter->SetMethodEx("bool get(const string_view&in, route_async@) const", &MapRouterGet1);
				VMapRouter->SetMethodEx("bool get(const string_view&in, route_mode, const string_view&in, route_async@) const", &MapRouterGet2);
				VMapRouter->SetMethodEx("bool post(const string_view&in, route_async@) const", &MapRouterPost1);
				VMapRouter->SetMethodEx("bool post(const string_view&in, route_mode, const string_view&in, route_async@) const", &MapRouterPost2);
				VMapRouter->SetMethodEx("bool patch(const string_view&in, route_async@) const", &MapRouterPatch1);
				VMapRouter->SetMethodEx("bool patch(const string_view&in, route_mode, const string_view&in, route_async@) const", &MapRouterPatch2);
				VMapRouter->SetMethodEx("bool delete(const string_view&in, route_async@) const", &MapRouterDelete1);
				VMapRouter->SetMethodEx("bool delete(const string_view&in, route_mode, const string_view&in, route_async@) const", &MapRouterDelete2);
				VMapRouter->SetMethodEx("bool options(const string_view&in, route_async@) const", &MapRouterOptions1);
				VMapRouter->SetMethodEx("bool options(const string_view&in, route_mode, const string_view&in, route_async@) const", &MapRouterOptions2);
				VMapRouter->SetMethodEx("bool access(const string_view&in, route_async@) const", &MapRouterAccess1);
				VMapRouter->SetMethodEx("bool access(const string_view&in, route_mode, const string_view&in, route_async@) const", &MapRouterAccess2);
				VMapRouter->SetMethodEx("bool headers(const string_view&in, headers_sync@) const", &MapRouterHeaders1);
				VMapRouter->SetMethodEx("bool headers(const string_view&in, route_mode, const string_view&in, headers_sync@) const", &MapRouterHeaders2);
				VMapRouter->SetMethodEx("bool authorize(const string_view&in, authorize_sync@) const", &MapRouterAuthorize1);
				VMapRouter->SetMethodEx("bool authorize(const string_view&in, route_mode, const string_view&in, authorize_sync@) const", &MapRouterAuthorize2);
				VMapRouter->SetMethodEx("bool websocket_initiate(const string_view&in, route_async@) const", &MapRouterWebsocketInitiate1);
				VMapRouter->SetMethodEx("bool websocket_initiate(const string_view&in, route_mode, const string_view&in, route_async@) const", &MapRouterWebsocketInitiate2);
				VMapRouter->SetMethodEx("bool websocket_connect(const string_view&in, websocket_status_async@) const", &MapRouterWebsocketConnect1);
				VMapRouter->SetMethodEx("bool websocket_connect(const string_view&in, route_mode, const string_view&in, websocket_status_async@) const", &MapRouterWebsocketConnect2);
				VMapRouter->SetMethodEx("bool websocket_disconnect(const string_view&in, websocket_status_async@) const", &MapRouterWebsocketDisconnect1);
				VMapRouter->SetMethodEx("bool websocket_disconnect(const string_view&in, route_mode, const string_view&in, websocket_status_async@) const", &MapRouterWebsocketDisconnect2);
				VMapRouter->SetMethodEx("bool websocket_receive(const string_view&in, websocket_recv_async@) const", &MapRouterWebsocketReceive1);
				VMapRouter->SetMethodEx("bool websocket_receive(const string_view&in, route_mode, const string_view&in, websocket_recv_async@) const", &MapRouterWebsocketReceive2);
				VMapRouter->SetMethodEx("route_entry@+ get_base() const", &MapRouterGetBase);
				VMapRouter->SetMethodEx("route_group@+ get_group(usize) const", &MapRouterGetGroup);
				VMapRouter->SetMethodEx("usize get_groups_size() const", &MapRouterGetGroupsSize);
				VMapRouter->SetEnumRefsEx<Network::HTTP::MapRouter>([](Network::HTTP::MapRouter* Base, asIScriptEngine* VM)
				{
					FunctionFactory::GCEnumCallback(VM, Base->Base);
					for (auto& Group : Base->Groups)
					{
						for (auto& Route : Group->Routes)
							FunctionFactory::GCEnumCallback(VM, Route);
					}
				});
				VMapRouter->SetReleaseRefsEx<Network::HTTP::MapRouter>([](Network::HTTP::MapRouter* Base, asIScriptEngine*)
				{
					Base->~MapRouter();
				});
				VMapRouter->SetDynamicCast<Network::HTTP::MapRouter, Network::SocketRouter>("socket_router@+", true);

				auto VServer = VM->SetClass<Network::HTTP::Server>("server", true);
				VConnection->SetProperty<Network::HTTP::Connection>("file_entry target", &Network::HTTP::Connection::Resource);
				VConnection->SetProperty<Network::HTTP::Connection>("request_frame request", &Network::HTTP::Connection::Request);
				VConnection->SetProperty<Network::HTTP::Connection>("response_frame response", &Network::HTTP::Connection::Response);
				VConnection->SetProperty<Network::SocketConnection>("socket@ stream", &Network::SocketConnection::Stream);
				VConnection->SetProperty<Network::SocketConnection>("socket_listener@ host", &Network::SocketConnection::Host);
				VConnection->SetProperty<Network::SocketConnection>("socket_address address", &Network::SocketConnection::Address);
				VConnection->SetProperty<Network::SocketConnection>("socket_data_frame info", &Network::SocketConnection::Info);
				VConnection->SetMethod<Network::HTTP::Connection, bool>("bool next()", &Network::HTTP::Connection::Next);
				VConnection->SetMethod<Network::HTTP::Connection, bool, int>("bool next(int32)", &Network::HTTP::Connection::Next);
				VConnection->SetMethod<Network::SocketConnection, bool>("bool abort()", &Network::SocketConnection::Abort);
				VConnection->SetMethodEx("bool abort(int32, const string_view&in)", &SocketConnectionAbort);
				VConnection->SetMethod("void reset(bool)", &Network::HTTP::Connection::Reset);
				VConnection->SetMethod("bool is_skip_required() const", &Network::HTTP::Connection::IsSkipRequired);
				VConnection->SetMethodEx("promise<bool>@ send_headers(int32, bool = true) const", &VI_SPROMISIFY(ConnectionSendHeaders, TypeId::BOOL));
				VConnection->SetMethodEx("promise<bool>@ send_chunk(const string_view&in) const", &VI_SPROMISIFY(ConnectionSendChunk, TypeId::BOOL));
				VConnection->SetMethodEx("promise<array<resource_info>@>@ store(bool = false) const", &VI_SPROMISIFY_REF(ConnectionStore, ArrayResourceInfo));
				VConnection->SetMethodEx("promise<string>@ fetch(bool = false) const", &VI_SPROMISIFY_REF(ConnectionFetch, String));
				VConnection->SetMethodEx("promise<bool>@ skip() const", &VI_SPROMISIFY(ConnectionSkip, TypeId::BOOL));
				VConnection->SetMethodEx("string get_peer_ip_address() const", &ConnectionGetPeerIpAddress);
				VConnection->SetMethodEx("websocket_frame@+ get_websocket() const", &ConnectionGetWebSocket);
				VConnection->SetMethodEx("route_entry@+ get_route() const", &ConnectionGetRoute);
				VConnection->SetMethodEx("server@+ get_root() const", &ConnectionGetServer);

				auto VQuery = VM->SetClass<Network::HTTP::Query>("query", false);
				VQuery->SetConstructor<Network::HTTP::Query>("query@ f()");
				VQuery->SetMethod("void clear()", &Network::HTTP::Query::Clear);
				VQuery->SetMethodEx("void decode(const string_view&in, const string_view&in)", &QueryDecode);
				VQuery->SetMethodEx("string encode(const string_view&in)", &QueryEncode);
				VQuery->SetMethodEx("void set_data(schema@+)", &QuerySetData);
				VQuery->SetMethodEx("schema@+ get_data()", &QueryGetData);

				auto VSession = VM->SetClass<Network::HTTP::Session>("session", false);
				VSession->SetProperty<Network::HTTP::Session>("string session_id", &Network::HTTP::Session::SessionId);
				VSession->SetProperty<Network::HTTP::Session>("int64 session_expires", &Network::HTTP::Session::SessionExpires);
				VSession->SetConstructor<Network::HTTP::Session>("session@ f()");
				VSession->SetMethod("void clear()", &Network::HTTP::Session::Clear);
				VSession->SetMethodEx("bool write(connection@+)", &VI_EXPECTIFY_VOID(Network::HTTP::Session::Write));
				VSession->SetMethodEx("bool read(connection@+)", &VI_EXPECTIFY_VOID(Network::HTTP::Session::Read));
				VSession->SetMethodStatic("bool invalidate_cache(const string_view&in)", &VI_SEXPECTIFY_VOID(Network::HTTP::Session::InvalidateCache));
				VSession->SetMethodEx("void set_data(schema@+)", &SessionSetData);
				VSession->SetMethodEx("schema@+ get_data()", &SessionGetData);

				VServer->SetGcConstructor<Network::HTTP::Server, Server>("server@ f()");
				VServer->SetMethod("void update()", &Network::HTTP::Server::Update);
				VServer->SetMethodEx("void set_router(map_router@+)", &SocketServerSetRouter);
				VServer->SetMethodEx("bool configure(map_router@+)", &SocketServerConfigure);
				VServer->SetMethodEx("bool listen()", &SocketServerListen);
				VServer->SetMethodEx("bool unlisten(bool)", &SocketServerUnlisten);
				VServer->SetMethod("void set_backlog(usize)", &Network::SocketServer::SetBacklog);
				VServer->SetMethod("usize get_backlog() const", &Network::SocketServer::GetBacklog);
				VServer->SetMethod("server_state get_state() const", &Network::SocketServer::GetState);
				VServer->SetMethod("map_router@+ get_router() const", &Network::SocketServer::GetRouter);
				VServer->SetEnumRefsEx<Network::HTTP::Server>([](Network::HTTP::Server* Base, asIScriptEngine* VM)
				{
					FunctionFactory::GCEnumCallback(VM, Base->GetRouter());
					for (auto* Item : Base->GetActiveClients())
						FunctionFactory::GCEnumCallback(VM, Item);

					for (auto* Item : Base->GetPooledClients())
						FunctionFactory::GCEnumCallback(VM, Item);
				});
				VServer->SetReleaseRefsEx<Network::HTTP::Server>([](Network::HTTP::Server* Base, asIScriptEngine*)
				{
					Base->~Server();
				});
				VServer->SetDynamicCast<Network::HTTP::Server, Network::SocketServer>("socket_server@+", true);

				auto VClient = VM->SetClass<Network::HTTP::Client>("client", false);
				VClient->SetConstructor<Network::HTTP::Client, int64_t>("client@ f(int64)");
				VClient->SetMethod("bool downgrade()", &Network::HTTP::Client::Downgrade);
				VClient->SetMethodEx("promise<schema@>@ json(const request_frame&in, usize = 65536)", &VI_SPROMISIFY_REF(ClientJSON, Schema));
				VClient->SetMethodEx("promise<schema@>@ xml(const request_frame&in, usize = 65536)", &VI_SPROMISIFY_REF(ClientXML, Schema));
				VClient->SetMethodEx("promise<bool>@ skip()", &VI_SPROMISIFY(ClientSkip, TypeId::BOOL));
				VClient->SetMethodEx("promise<bool>@ fetch(usize = 65536, bool = false)", &VI_SPROMISIFY(ClientFetch, TypeId::BOOL));
				VClient->SetMethodEx("promise<bool>@ upgrade(const request_frame&in)", &VI_SPROMISIFY(ClientUpgrade, TypeId::BOOL));
				VClient->SetMethodEx("promise<bool>@ send(const request_frame&in)", &VI_SPROMISIFY(ClientSend, TypeId::BOOL));
				VClient->SetMethodEx("promise<bool>@ send_fetch(const request_frame&in, usize = 65536)", &VI_SPROMISIFY(ClientSendFetch, TypeId::BOOL));
				VClient->SetMethodEx("promise<bool>@ connect_sync(const socket_address&in, int32 = -1)", &VI_SPROMISIFY(SocketClientConnectSync, TypeId::BOOL));
				VClient->SetMethodEx("promise<bool>@ connect_async(const socket_address&in, int32 = -1)", &VI_SPROMISIFY(SocketClientConnectAsync, TypeId::BOOL));
				VClient->SetMethodEx("promise<bool>@ disconnect()", &VI_SPROMISIFY(SocketClientDisconnect, TypeId::BOOL));
				VClient->SetMethod("bool has_stream() const", &Network::SocketClient::HasStream);
				VClient->SetMethod("bool is_secure() const", &Network::SocketClient::IsSecure);
				VClient->SetMethod("bool is_verified() const", &Network::SocketClient::IsVerified);
				VClient->SetMethod("void apply_reusability(bool)", &Network::SocketClient::ApplyReusability);
				VClient->SetMethod("const socket_address& get_peer_address() const", &Network::SocketClient::GetPeerAddress);
				VClient->SetMethod("socket@+ get_stream() const", &Network::SocketClient::GetStream);
				VClient->SetMethod("websocket_frame@+ get_websocket() const", &Network::HTTP::Client::GetWebSocket);
				VClient->SetMethod("request_frame& get_request() property", &Network::HTTP::Client::GetRequest);
				VClient->SetMethod("response_frame& get_response() property", &Network::HTTP::Client::GetResponse);
				VClient->SetDynamicCast<Network::HTTP::Client, Network::SocketClient>("socket_client@+", true);

				auto VHrmCache = VM->SetClass<Network::HTTP::HrmCache>("hrm_cache", false);
				VHrmCache->SetConstructor<Network::HTTP::HrmCache>("hrm_cache@ f()");
				VHrmCache->SetConstructor<Network::HTTP::HrmCache, size_t>("hrm_cache@ f(usize)");
				VHrmCache->SetMethod("void rescale(usize)", &Network::HTTP::HrmCache::Rescale);
				VHrmCache->SetMethod("void shrink()", &Network::HTTP::HrmCache::Shrink);
				VHrmCache->SetMethodStatic("hrm_cache@+ get()", &Network::HTTP::HrmCache::Get);

				VM->SetFunction("promise<response_frame>@ fetch(const string_view&in, const string_view&in = \"GET\", const fetch_frame&in = fetch_frame())", &VI_SPROMISIFY_REF(HTTPFetch, ResponseFrame));
				VM->EndNamespace();

				return true;
#else
				VI_ASSERT(false, "<http> is not loaded");
				return false;
#endif
			}
			bool Registry::ImportSMTP(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr, "manager should be set");

				VM->BeginNamespace("smtp");
				auto VPriority = VM->SetEnum("priority");
				VPriority->SetValue("high", (int)Network::SMTP::Priority::High);
				VPriority->SetValue("normal", (int)Network::SMTP::Priority::Normal);
				VPriority->SetValue("low", (int)Network::SMTP::Priority::Low);

				auto VRecipient = VM->SetStructTrivial<Network::SMTP::Recipient>("recipient");
				VRecipient->SetProperty<Network::SMTP::Recipient>("string name", &Network::SMTP::Recipient::Name);
				VRecipient->SetProperty<Network::SMTP::Recipient>("string address", &Network::SMTP::Recipient::Address);
				VRecipient->SetConstructor<Network::SMTP::Recipient>("void f()");

				auto VAttachment = VM->SetStructTrivial<Network::SMTP::Attachment>("attachment");
				VAttachment->SetProperty<Network::SMTP::Attachment>("string path", &Network::SMTP::Attachment::Path);
				VAttachment->SetProperty<Network::SMTP::Attachment>("usize length", &Network::SMTP::Attachment::Length);
				VAttachment->SetConstructor<Network::SMTP::Attachment>("void f()");

				auto VRequestFrame = VM->SetStructTrivial<Network::SMTP::RequestFrame>("request_frame");
				VRequestFrame->SetProperty<Network::SMTP::RequestFrame>("string sender_name", &Network::SMTP::RequestFrame::SenderName);
				VRequestFrame->SetProperty<Network::SMTP::RequestFrame>("string sender_address", &Network::SMTP::RequestFrame::SenderAddress);
				VRequestFrame->SetProperty<Network::SMTP::RequestFrame>("string subject", &Network::SMTP::RequestFrame::Subject);
				VRequestFrame->SetProperty<Network::SMTP::RequestFrame>("string charset", &Network::SMTP::RequestFrame::Charset);
				VRequestFrame->SetProperty<Network::SMTP::RequestFrame>("string mailer", &Network::SMTP::RequestFrame::Mailer);
				VRequestFrame->SetProperty<Network::SMTP::RequestFrame>("string receiver", &Network::SMTP::RequestFrame::Receiver);
				VRequestFrame->SetProperty<Network::SMTP::RequestFrame>("string login", &Network::SMTP::RequestFrame::Login);
				VRequestFrame->SetProperty<Network::SMTP::RequestFrame>("string password", &Network::SMTP::RequestFrame::Password);
				VRequestFrame->SetProperty<Network::SMTP::RequestFrame>("priority prior", &Network::SMTP::RequestFrame::Prior);
				VRequestFrame->SetProperty<Network::SMTP::RequestFrame>("bool authenticate", &Network::SMTP::RequestFrame::Authenticate);
				VRequestFrame->SetProperty<Network::SMTP::RequestFrame>("bool no_notification", &Network::SMTP::RequestFrame::NoNotification);
				VRequestFrame->SetProperty<Network::SMTP::RequestFrame>("bool allow_html", &Network::SMTP::RequestFrame::AllowHTML);
				VRequestFrame->SetConstructor<Network::SMTP::RequestFrame>("void f()");
				VRequestFrame->SetMethodEx("void set_header(const string_view&in, const string_view&in)", &SMTPRequestSetHeader);
				VRequestFrame->SetMethodEx("string get_header(const string_view&in)", &SMTPRequestGetHeader);
				VRequestFrame->SetMethodEx("void set_recipients(array<recipient>@+)", &SMTPRequestSetRecipients);
				VRequestFrame->SetMethodEx("array<string>@ get_recipients() const", &SMTPRequestGetRecipients);
				VRequestFrame->SetMethodEx("void set_cc_recipients(array<recipient>@+)", &SMTPRequestSetCCRecipients);
				VRequestFrame->SetMethodEx("array<string>@ get_cc_recipients() const", &SMTPRequestGetCCRecipients);
				VRequestFrame->SetMethodEx("void set_bcc_recipients(array<recipient>@+)", &SMTPRequestSetBCCRecipients);
				VRequestFrame->SetMethodEx("array<string>@ get_bcc_recipients() const", &SMTPRequestGetBCCRecipients);
				VRequestFrame->SetMethodEx("void set_attachments(array<attachment>@+)", &SMTPRequestSetAttachments);
				VRequestFrame->SetMethodEx("array<attachment>@ get_attachments() const", &SMTPRequestGetAttachments);
				VRequestFrame->SetMethodEx("void set_messages(array<string>@+)", &SMTPRequestSetMessages);
				VRequestFrame->SetMethodEx("array<string>@ get_messages() const", &SMTPRequestGetMessages);

				auto VClient = VM->SetClass<Network::SMTP::Client>("client", false);
				VClient->SetConstructor<Network::SMTP::Client, const std::string_view&, int64_t>("client@ f(const string_view&in, int64)");
				VClient->SetMethodEx("promise<bool>@ send(const request_frame&in)", &VI_SPROMISIFY(SMTPClientSend, TypeId::BOOL));
				VClient->SetMethodEx("promise<bool>@ connect_sync(const socket_address&in, int32 = -1)", &VI_SPROMISIFY(SocketClientConnectSync, TypeId::BOOL));
				VClient->SetMethodEx("promise<bool>@ connect_async(const socket_address&in, int32 = -1)", &VI_SPROMISIFY(SocketClientConnectAsync, TypeId::BOOL));
				VClient->SetMethodEx("promise<bool>@ disconnect()", &VI_SPROMISIFY(SocketClientDisconnect, TypeId::BOOL));
				VClient->SetMethod("bool has_stream() const", &Network::SocketClient::HasStream);
				VClient->SetMethod("bool is_secure() const", &Network::SocketClient::IsSecure);
				VClient->SetMethod("bool is_verified() const", &Network::SocketClient::IsVerified);
				VClient->SetMethod("void apply_reusability(bool)", &Network::SocketClient::ApplyReusability);
				VClient->SetMethod("const socket_address& get_peer_address() const", &Network::SocketClient::GetPeerAddress);
				VClient->SetMethod("socket@+ get_stream() const", &Network::SocketClient::GetStream);
				VClient->SetMethod("request_frame& get_request() property", &Network::SMTP::Client::GetRequest);
				VClient->SetDynamicCast<Network::SMTP::Client, Network::SocketClient>("socket_client@+", true);
				VM->EndNamespace();

				return true;
#else
				VI_ASSERT(false, "<smtp> is not loaded");
				return false;
#endif
			}
			bool Registry::ImportSQLite(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr, "manager should be set");
				VI_TYPEREF(Cursor, "ldb::cursor");
				VI_TYPEREF(Connection, "uptr");

				VM->BeginNamespace("ldb");
				auto VIsolation = VM->SetEnum("isolation");
				VIsolation->SetValue("deferred", (int)Network::LDB::Isolation::Deferred);
				VIsolation->SetValue("immediate", (int)Network::LDB::Isolation::Immediate);
				VIsolation->SetValue("exclusive", (int)Network::LDB::Isolation::Exclusive);
				VIsolation->SetValue("default_value", (int)Network::LDB::Isolation::Default);

				auto VQueryOp = VM->SetEnum("query_op");
				VQueryOp->SetValue("transaction_start", (int)Network::LDB::QueryOp::TransactionStart);
				VQueryOp->SetValue("transaction_end", (int)Network::LDB::QueryOp::TransactionEnd);

				auto VCheckpointMode = VM->SetEnum("checkpoint_mode");
				VCheckpointMode->SetValue("passive", (int)Network::LDB::CheckpointMode::Passive);
				VCheckpointMode->SetValue("full", (int)Network::LDB::CheckpointMode::Full);
				VCheckpointMode->SetValue("restart", (int)Network::LDB::CheckpointMode::Restart);
				VCheckpointMode->SetValue("truncate", (int)Network::LDB::CheckpointMode::Truncate);

				auto VCheckpoint = VM->SetStructTrivial<Network::LDB::Checkpoint>("checkpoint");
				VCheckpoint->SetProperty<Network::LDB::Checkpoint>("string database", &Network::LDB::Checkpoint::Database);
				VCheckpoint->SetProperty<Network::LDB::Checkpoint>("uint32 frames_size", &Network::LDB::Checkpoint::FramesSize);
				VCheckpoint->SetProperty<Network::LDB::Checkpoint>("uint32 frames_count", &Network::LDB::Checkpoint::FramesCount);
				VCheckpoint->SetProperty<Network::LDB::Checkpoint>("int32 status", &Network::LDB::Checkpoint::Status);
				VCheckpoint->SetConstructor<Network::LDB::Checkpoint>("void f()");

				auto VRow = VM->SetStructTrivial<Network::LDB::Row>("row", (size_t)ObjectBehaviours::APP_CLASS_ALLINTS);
				auto VColumn = VM->SetStructTrivial<Network::LDB::Column>("column");
				VColumn->SetConstructor<Network::LDB::Column, const Network::LDB::Column&>("void f(const column&in)");
				VColumn->SetMethod("string get_name() const", &Network::LDB::Column::GetName);
				VColumn->SetMethod("variant get() const", &Network::LDB::Column::Get);
				VColumn->SetMethod("schema@ get_inline() const", &Network::LDB::Column::GetInline);
				VColumn->SetMethod("usize index() const", &Network::LDB::Column::Index);
				VColumn->SetMethod("usize size() const", &Network::LDB::Column::Size);
				VColumn->SetMethod("row get_row() const", &Network::LDB::Column::GetRow);
				VColumn->SetMethod("bool nullable() const", &Network::LDB::Column::Nullable);
				VColumn->SetMethod("bool exists() const", &Network::LDB::Column::Exists);

				auto VResponse = VM->SetStruct<Network::LDB::Response>("response");
				VRow->SetConstructor<Network::LDB::Row, const Network::LDB::Row&>("void f(const row&in)");
				VRow->SetMethod("schema@ get_object() const", &Network::LDB::Row::GetObject);
				VRow->SetMethod("schema@ get_array() const", &Network::LDB::Row::GetArray);
				VRow->SetMethod("usize index() const", &Network::LDB::Row::Index);
				VRow->SetMethod("usize size() const", &Network::LDB::Row::Size);
				VRow->SetMethod<Network::LDB::Row, Network::LDB::Column, size_t>("column get_column(usize) const", &Network::LDB::Row::GetColumn);
				VRow->SetMethod<Network::LDB::Row, Network::LDB::Column, const std::string_view&>("column get_column(const string_view&in) const", &Network::LDB::Row::GetColumn);
				VRow->SetMethod("bool exists() const", &Network::LDB::Row::Exists);
				VRow->SetMethod<Network::LDB::Row, Network::LDB::Column, size_t>("column opIndex(usize)", &Network::LDB::Row::GetColumn);
				VRow->SetMethod<Network::LDB::Row, Network::LDB::Column, size_t>("column opIndex(usize) const", &Network::LDB::Row::GetColumn);
				VRow->SetMethod<Network::LDB::Row, Network::LDB::Column, const std::string_view&>("column opIndex(const string_view&in)", &Network::LDB::Row::GetColumn);
				VRow->SetMethod<Network::LDB::Row, Network::LDB::Column, const std::string_view&>("column opIndex(const string_view&in) const", &Network::LDB::Row::GetColumn);

				VResponse->SetConstructor<Network::LDB::Response>("void f()");
				VResponse->SetOperatorCopyStatic(&LDBResponseCopy);
				VResponse->SetDestructor<Network::LDB::Response>("void f()");
				VResponse->SetMethod<Network::LDB::Response, Network::LDB::Row, size_t>("row opIndex(usize)", &Network::LDB::Response::GetRow);
				VResponse->SetMethod<Network::LDB::Response, Network::LDB::Row, size_t>("row opIndex(usize) const", &Network::LDB::Response::GetRow);
				VResponse->SetMethod("schema@ get_array_of_objects() const", &Network::LDB::Response::GetArrayOfObjects);
				VResponse->SetMethod("schema@ get_array_of_arrays() const", &Network::LDB::Response::GetArrayOfArrays);
				VResponse->SetMethod("schema@ get_object(usize = 0) const", &Network::LDB::Response::GetObject);
				VResponse->SetMethod("schema@ get_array(usize = 0) const", &Network::LDB::Response::GetArray);
				VResponse->SetMethodEx("array<string>@ get_columns() const", &LDBResponseGetColumns);
				VResponse->SetMethod("string get_status_text() const", &Network::LDB::Response::GetStatusText);
				VResponse->SetMethod("int32 get_status_code() const", &Network::LDB::Response::GetStatusCode);
				VResponse->SetMethod("usize affected_rows() const", &Network::LDB::Response::AffectedRows);
				VResponse->SetMethod("usize last_inserted_row_id() const", &Network::LDB::Response::LastInsertedRowId);
				VResponse->SetMethod("usize size() const", &Network::LDB::Response::Size);
				VResponse->SetMethod("row get_row(usize) const", &Network::LDB::Response::GetRow);
				VResponse->SetMethod("row front() const", &Network::LDB::Response::Front);
				VResponse->SetMethod("row back() const", &Network::LDB::Response::Back);
				VResponse->SetMethod("response copy() const", &Network::LDB::Response::Copy);
				VResponse->SetMethod("bool empty() const", &Network::LDB::Response::Empty);
				VResponse->SetMethod("bool error() const", &Network::LDB::Response::Error);
				VResponse->SetMethod("bool error_or_empty() const", &Network::LDB::Response::ErrorOrEmpty);
				VResponse->SetMethod("bool exists() const", &Network::LDB::Response::Exists);

				auto VCursor = VM->SetStruct<Network::LDB::Cursor>("cursor");
				VCursor->SetConstructor<Network::LDB::Cursor>("void f()");
				VCursor->SetOperatorCopyStatic(&LDBCursorCopy);
				VCursor->SetDestructor<Network::LDB::Cursor>("void f()");
				VCursor->SetMethod("column opIndex(const string_view&in)", &Network::LDB::Cursor::GetColumn);
				VCursor->SetMethod("column opIndex(const string_view&in) const", &Network::LDB::Cursor::GetColumn);
				VCursor->SetMethod("bool success() const", &Network::LDB::Cursor::Success);
				VCursor->SetMethod("bool empty() const", &Network::LDB::Cursor::Empty);
				VCursor->SetMethod("bool error() const", &Network::LDB::Cursor::Error);
				VCursor->SetMethod("bool error_or_empty() const", &Network::LDB::Cursor::ErrorOrEmpty);
				VCursor->SetMethod("usize size() const", &Network::LDB::Cursor::Size);
				VCursor->SetMethod("usize affected_rows() const", &Network::LDB::Cursor::AffectedRows);
				VCursor->SetMethod("cursor copy() const", &Network::LDB::Cursor::Copy);
				VCursor->SetMethodEx("response first() const", &LDBCursorFirst);
				VCursor->SetMethodEx("response last() const", &LDBCursorLast);
				VCursor->SetMethodEx("response at(usize) const", &LDBCursorAt);
				VCursor->SetMethod("uptr@ get_executor() const", &Network::LDB::Cursor::GetExecutor);
				VCursor->SetMethod("schema@ get_array_of_objects(usize = 0) const", &Network::LDB::Cursor::GetArrayOfObjects);
				VCursor->SetMethod("schema@ get_array_of_arrays(usize = 0) const", &Network::LDB::Cursor::GetArrayOfArrays);
				VCursor->SetMethod("schema@ get_object(usize = 0, usize = 0) const", &Network::LDB::Cursor::GetObject);
				VCursor->SetMethod("schema@ get_array(usize = 0, usize = 0) const", &Network::LDB::Cursor::GetArray);

				auto VCluster = VM->SetClass<Network::LDB::Cluster>("cluster", false);
				VCluster->SetConstructor<Network::LDB::Cluster>("cluster@ f()");
				VCluster->SetFunctionDef("variant constant_sync(array<variant>@+)");
				VCluster->SetFunctionDef("variant finalize_sync()");
				VCluster->SetFunctionDef("variant value_sync()");
				VCluster->SetFunctionDef("void step_sync(array<variant>@+)");
				VCluster->SetFunctionDef("void inverse_sync(array<variant>@+)");
				VCluster->SetMethod("void set_wal_autocheckpoint(uint32)", &Network::LDB::Cluster::SetWalAutocheckpoint);
				VCluster->SetMethod("void set_soft_heap_limit(uint64)", &Network::LDB::Cluster::SetSoftHeapLimit);
				VCluster->SetMethod("void set_hard_heap_limit(uint64)", &Network::LDB::Cluster::SetHardHeapLimit);
				VCluster->SetMethod("void set_shared_cache(bool)", &Network::LDB::Cluster::SetSharedCache);
				VCluster->SetMethod("void set_extensions(bool)", &Network::LDB::Cluster::SetExtensions);
				VCluster->SetMethod("void overload_function(const string_view&in, uint8)", &Network::LDB::Cluster::OverloadFunction);
				VCluster->SetMethodEx("array<checkpoint>@ wal_checkpoint(checkpoint_mode, const string_view&in = string_view())", &LDBClusterWalCheckpoint);
				VCluster->SetMethod("usize free_memory_used(usize)", &Network::LDB::Cluster::FreeMemoryUsed);
				VCluster->SetMethod("usize get_memory_used()", &Network::LDB::Cluster::GetMemoryUsed);
				VCluster->SetMethod("uptr@ get_idle_connection()", &Network::LDB::Cluster::GetIdleConnection);
				VCluster->SetMethod("uptr@ get_busy_connection()", &Network::LDB::Cluster::GetBusyConnection);
				VCluster->SetMethod("uptr@ get_any_connection()", &Network::LDB::Cluster::GetAnyConnection);
				VCluster->SetMethod("const string& get_address() const", &Network::LDB::Cluster::GetAddress);
				VCluster->SetMethod("bool is_connected() const", &Network::LDB::Cluster::IsConnected);
				VCluster->SetMethodEx("void set_function(const string_view&in, uint8, constant_sync@)", &LDBClusterSetFunction);
				VCluster->SetMethodEx("void set_aggregate_function(const string_view&in, uint8, step_sync@, finalize_sync@)", &LDBClusterSetAggregateFunction);
				VCluster->SetMethodEx("void set_window_function(const string_view&in, uint8, step_sync@, inverse_sync@, value_sync@, finalize_sync@)", &LDBClusterSetWindowFunction);
				VCluster->SetMethodEx("promise<uptr@>@ tx_begin(isolation)", &VI_SPROMISIFY_REF(LDBClusterTxBegin, Connection));
				VCluster->SetMethodEx("promise<uptr@>@ tx_start(const string_view&in)", &VI_SPROMISIFY_REF(LDBClusterTxStart, Connection));
				VCluster->SetMethodEx("promise<bool>@ tx_end(const string_view&in, uptr@)", &VI_SPROMISIFY(LDBClusterTxEnd, TypeId::BOOL));
				VCluster->SetMethodEx("promise<bool>@ tx_commit(uptr@)", &VI_SPROMISIFY(LDBClusterTxCommit, TypeId::BOOL));
				VCluster->SetMethodEx("promise<bool>@ tx_rollback(uptr@)", &VI_SPROMISIFY(LDBClusterTxRollback, TypeId::BOOL));
				VCluster->SetMethodEx("promise<bool>@ connect(const string_view&in, usize = 1)", &VI_SPROMISIFY(LDBClusterConnect, TypeId::BOOL));
				VCluster->SetMethodEx("promise<bool>@ disconnect()", &VI_SPROMISIFY(LDBClusterDisconnect, TypeId::BOOL));
				VCluster->SetMethodEx("promise<bool>@ flush()", &VI_SPROMISIFY(LDBClusterFlush, TypeId::BOOL));
				VCluster->SetMethodEx("promise<cursor>@ query(const string_view&in, usize = 0, uptr@ = null)", &VI_SPROMISIFY_REF(LDBClusterQuery, Cursor));
				VCluster->SetMethodEx("promise<cursor>@ emplace_query(const string_view&in, array<schema@>@+, usize = 0, uptr@ = null)", &VI_SPROMISIFY_REF(LDBClusterEmplaceQuery, Cursor));
				VCluster->SetMethodEx("promise<cursor>@ template_query(const string_view&in, dictionary@+, usize = 0, uptr@ = null)", &VI_SPROMISIFY_REF(LDBClusterTemplateQuery, Cursor));

				auto VDriver = VM->SetClass<Network::LDB::Driver>("driver", false);
				VDriver->SetFunctionDef("void query_log_async(const string&in)");
				VDriver->SetConstructor<Network::LDB::Driver>("driver@ f()");
				VDriver->SetMethod("void log_query(const string_view&in)", &Network::LDB::Driver::LogQuery);
				VDriver->SetMethod("void add_constant(const string_view&in, const string_view&in)", &Network::LDB::Driver::AddConstant);
				VDriver->SetMethodEx("bool add_query(const string_view&in, const string_view&in)", &VI_EXPECTIFY_VOID(Network::LDB::Driver::AddQuery));
				VDriver->SetMethodEx("bool add_directory(const string_view&in, const string_view&in = string_view())", &VI_EXPECTIFY_VOID(Network::LDB::Driver::AddDirectory));
				VDriver->SetMethod("bool remove_constant(const string_view&in)", &Network::LDB::Driver::RemoveConstant);
				VDriver->SetMethod("bool remove_query(const string_view&in)", &Network::LDB::Driver::RemoveQuery);
				VDriver->SetMethod("bool load_cache_dump(schema@+)", &Network::LDB::Driver::LoadCacheDump);
				VDriver->SetMethod("schema@ get_cache_dump()", &Network::LDB::Driver::GetCacheDump);
				VDriver->SetMethodEx("void set_query_log(query_log_async@)", &LDBDriverSetQueryLog);
				VDriver->SetMethodEx("string emplace(const string_view&in, array<schema@>@+)", &LDBDriverEmplace);
				VDriver->SetMethodEx("string get_query(const string_view&in, dictionary@+)", &LDBDriverGetQuery);
				VDriver->SetMethodEx("array<string>@ get_queries()", &LDBDriverGetQueries);
				VDriver->SetMethodStatic("driver@+ get()", &Network::LDB::Driver::Get);

				VM->EndNamespace();
				return true;
#else
				VI_ASSERT(false, "<sqlite> is not loaded");
				return false;
#endif
			}
			bool Registry::ImportPostgreSQL(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr, "manager should be set");
				VI_TYPEREF(Connection, "pdb::connection");
				VI_TYPEREF(Cursor, "pdb::cursor");

				VM->BeginNamespace("pdb");
				auto VIsolation = VM->SetEnum("isolation");
				VIsolation->SetValue("serializable", (int)Network::PDB::Isolation::Serializable);
				VIsolation->SetValue("repeatable_read", (int)Network::PDB::Isolation::RepeatableRead);
				VIsolation->SetValue("read_commited", (int)Network::PDB::Isolation::ReadCommited);
				VIsolation->SetValue("read_uncommited", (int)Network::PDB::Isolation::ReadUncommited);
				VIsolation->SetValue("default_value", (int)Network::PDB::Isolation::Default);

				auto VQueryOp = VM->SetEnum("query_op");
				VQueryOp->SetValue("cache_short", (int)Network::PDB::QueryOp::CacheShort);
				VQueryOp->SetValue("cache_mid", (int)Network::PDB::QueryOp::CacheMid);
				VQueryOp->SetValue("cache_long", (int)Network::PDB::QueryOp::CacheLong);

				auto VAddressOp = VM->SetEnum("address_op");
				VAddressOp->SetValue("host", (int)Network::PDB::AddressOp::Host);
				VAddressOp->SetValue("ip", (int)Network::PDB::AddressOp::Ip);
				VAddressOp->SetValue("port", (int)Network::PDB::AddressOp::Port);
				VAddressOp->SetValue("database", (int)Network::PDB::AddressOp::Database);
				VAddressOp->SetValue("user", (int)Network::PDB::AddressOp::User);
				VAddressOp->SetValue("password", (int)Network::PDB::AddressOp::Password);
				VAddressOp->SetValue("timeout", (int)Network::PDB::AddressOp::Timeout);
				VAddressOp->SetValue("encoding", (int)Network::PDB::AddressOp::Encoding);
				VAddressOp->SetValue("options", (int)Network::PDB::AddressOp::Options);
				VAddressOp->SetValue("profile", (int)Network::PDB::AddressOp::Profile);
				VAddressOp->SetValue("fallback_profile", (int)Network::PDB::AddressOp::Fallback_Profile);
				VAddressOp->SetValue("keep_alive", (int)Network::PDB::AddressOp::KeepAlive);
				VAddressOp->SetValue("keep_alive_idle", (int)Network::PDB::AddressOp::KeepAlive_Idle);
				VAddressOp->SetValue("keep_alive_interval", (int)Network::PDB::AddressOp::KeepAlive_Interval);
				VAddressOp->SetValue("keep_alive_count", (int)Network::PDB::AddressOp::KeepAlive_Count);
				VAddressOp->SetValue("tty", (int)Network::PDB::AddressOp::TTY);
				VAddressOp->SetValue("ssl", (int)Network::PDB::AddressOp::SSL);
				VAddressOp->SetValue("ssl_compression", (int)Network::PDB::AddressOp::SSL_Compression);
				VAddressOp->SetValue("ssl_cert", (int)Network::PDB::AddressOp::SSL_Cert);
				VAddressOp->SetValue("ssl_root_cert", (int)Network::PDB::AddressOp::SSL_Root_Cert);
				VAddressOp->SetValue("ssl_key", (int)Network::PDB::AddressOp::SSL_Key);
				VAddressOp->SetValue("ssl_crl", (int)Network::PDB::AddressOp::SSL_CRL);
				VAddressOp->SetValue("require_peer", (int)Network::PDB::AddressOp::Require_Peer);
				VAddressOp->SetValue("require_ssl", (int)Network::PDB::AddressOp::Require_SSL);
				VAddressOp->SetValue("krb_server_name", (int)Network::PDB::AddressOp::KRB_Server_Name);
				VAddressOp->SetValue("service", (int)Network::PDB::AddressOp::Service);

				auto VQueryExec = VM->SetEnum("query_exec");
				VQueryExec->SetValue("empty_query", (int)Network::PDB::QueryExec::Empty_Query);
				VQueryExec->SetValue("command_ok", (int)Network::PDB::QueryExec::Command_OK);
				VQueryExec->SetValue("tuples_ok", (int)Network::PDB::QueryExec::Tuples_OK);
				VQueryExec->SetValue("copy_out", (int)Network::PDB::QueryExec::Copy_Out);
				VQueryExec->SetValue("copy_in", (int)Network::PDB::QueryExec::Copy_In);
				VQueryExec->SetValue("bad_response", (int)Network::PDB::QueryExec::Bad_Response);
				VQueryExec->SetValue("non_fatal_error", (int)Network::PDB::QueryExec::Non_Fatal_Error);
				VQueryExec->SetValue("fatal_error", (int)Network::PDB::QueryExec::Fatal_Error);
				VQueryExec->SetValue("copy_both", (int)Network::PDB::QueryExec::Copy_Both);
				VQueryExec->SetValue("single_tuple", (int)Network::PDB::QueryExec::Single_Tuple);

				auto VFieldCode = VM->SetEnum("field_code");
				VFieldCode->SetValue("severity", (int)Network::PDB::FieldCode::Severity);
				VFieldCode->SetValue("severity_nonlocalized", (int)Network::PDB::FieldCode::Severity_Nonlocalized);
				VFieldCode->SetValue("sql_state", (int)Network::PDB::FieldCode::SQL_State);
				VFieldCode->SetValue("messagep_rimary", (int)Network::PDB::FieldCode::Message_Primary);
				VFieldCode->SetValue("message_detail", (int)Network::PDB::FieldCode::Message_Detail);
				VFieldCode->SetValue("message_hint", (int)Network::PDB::FieldCode::Message_Hint);
				VFieldCode->SetValue("statement_position", (int)Network::PDB::FieldCode::Statement_Position);
				VFieldCode->SetValue("internal_position", (int)Network::PDB::FieldCode::Internal_Position);
				VFieldCode->SetValue("internal_query", (int)Network::PDB::FieldCode::Internal_Query);
				VFieldCode->SetValue("context", (int)Network::PDB::FieldCode::Context);
				VFieldCode->SetValue("schema_name", (int)Network::PDB::FieldCode::Schema_Name);
				VFieldCode->SetValue("table_name", (int)Network::PDB::FieldCode::Table_Name);
				VFieldCode->SetValue("column_name", (int)Network::PDB::FieldCode::Column_Name);
				VFieldCode->SetValue("data_type_name", (int)Network::PDB::FieldCode::Data_Type_Name);
				VFieldCode->SetValue("constraint_name", (int)Network::PDB::FieldCode::Constraint_Name);
				VFieldCode->SetValue("source_file", (int)Network::PDB::FieldCode::Source_File);
				VFieldCode->SetValue("source_line", (int)Network::PDB::FieldCode::Source_Line);
				VFieldCode->SetValue("source_function", (int)Network::PDB::FieldCode::Source_Function);

				auto VTransactionState = VM->SetEnum("transaction_state");
				VTransactionState->SetValue("idle", (int)Network::PDB::TransactionState::Idle);
				VTransactionState->SetValue("active", (int)Network::PDB::TransactionState::Active);
				VTransactionState->SetValue("in_transaction", (int)Network::PDB::TransactionState::In_Transaction);
				VTransactionState->SetValue("in_error", (int)Network::PDB::TransactionState::In_Error);
				VTransactionState->SetValue("none", (int)Network::PDB::TransactionState::None);

				auto VCaching = VM->SetEnum("caching");
				VCaching->SetValue("never", (int)Network::PDB::Caching::Never);
				VCaching->SetValue("miss", (int)Network::PDB::Caching::Miss);
				VCaching->SetValue("cached", (int)Network::PDB::Caching::Cached);

				auto VOidType = VM->SetEnum("oid_type");
				VOidType->SetValue("json", (int)Network::PDB::OidType::JSON);
				VOidType->SetValue("jsonb", (int)Network::PDB::OidType::JSONB);
				VOidType->SetValue("any_array", (int)Network::PDB::OidType::Any_Array);
				VOidType->SetValue("name_array", (int)Network::PDB::OidType::Name_Array);
				VOidType->SetValue("text_array", (int)Network::PDB::OidType::Text_Array);
				VOidType->SetValue("date_array", (int)Network::PDB::OidType::Date_Array);
				VOidType->SetValue("time_array", (int)Network::PDB::OidType::Time_Array);
				VOidType->SetValue("uuid_array", (int)Network::PDB::OidType::UUID_Array);
				VOidType->SetValue("cstring_array", (int)Network::PDB::OidType::CString_Array);
				VOidType->SetValue("bp_char_array", (int)Network::PDB::OidType::BpChar_Array);
				VOidType->SetValue("var_char_array", (int)Network::PDB::OidType::VarChar_Array);
				VOidType->SetValue("bit_array", (int)Network::PDB::OidType::Bit_Array);
				VOidType->SetValue("var_bit_array", (int)Network::PDB::OidType::VarBit_Array);
				VOidType->SetValue("char_array", (int)Network::PDB::OidType::Char_Array);
				VOidType->SetValue("int2_array", (int)Network::PDB::OidType::Int2_Array);
				VOidType->SetValue("int4_array", (int)Network::PDB::OidType::Int4_Array);
				VOidType->SetValue("int8_array", (int)Network::PDB::OidType::Int8_Array);
				VOidType->SetValue("bool_array", (int)Network::PDB::OidType::Bool_Array);
				VOidType->SetValue("float4_array", (int)Network::PDB::OidType::Float4_Array);
				VOidType->SetValue("float8_array", (int)Network::PDB::OidType::Float8_Array);
				VOidType->SetValue("money_array", (int)Network::PDB::OidType::Money_Array);
				VOidType->SetValue("numeric_array", (int)Network::PDB::OidType::Numeric_Array);
				VOidType->SetValue("bytea_array", (int)Network::PDB::OidType::Bytea_Array);
				VOidType->SetValue("any_t", (int)Network::PDB::OidType::Any);
				VOidType->SetValue("name_t", (int)Network::PDB::OidType::Name);
				VOidType->SetValue("text_t", (int)Network::PDB::OidType::Text);
				VOidType->SetValue("date_t", (int)Network::PDB::OidType::Date);
				VOidType->SetValue("time_t", (int)Network::PDB::OidType::Time);
				VOidType->SetValue("uuid_t", (int)Network::PDB::OidType::UUID);
				VOidType->SetValue("cstring_t", (int)Network::PDB::OidType::CString);
				VOidType->SetValue("bp_char_t", (int)Network::PDB::OidType::BpChar);
				VOidType->SetValue("var_char_t", (int)Network::PDB::OidType::VarChar);
				VOidType->SetValue("bit_t", (int)Network::PDB::OidType::Bit);
				VOidType->SetValue("var_bit_t", (int)Network::PDB::OidType::VarBit);
				VOidType->SetValue("char_t", (int)Network::PDB::OidType::Char);
				VOidType->SetValue("int2_t", (int)Network::PDB::OidType::Int2);
				VOidType->SetValue("int4_t", (int)Network::PDB::OidType::Int4);
				VOidType->SetValue("int8_t", (int)Network::PDB::OidType::Int8);
				VOidType->SetValue("bool_t", (int)Network::PDB::OidType::Bool);
				VOidType->SetValue("float4_t", (int)Network::PDB::OidType::Float4);
				VOidType->SetValue("float8_t", (int)Network::PDB::OidType::Float8);
				VOidType->SetValue("money_t", (int)Network::PDB::OidType::Money);
				VOidType->SetValue("numeric_t", (int)Network::PDB::OidType::Numeric);
				VOidType->SetValue("bytea_t", (int)Network::PDB::OidType::Bytea);

				auto VQueryState = VM->SetEnum("query_state");
				VQueryState->SetValue("lost", (int)Network::PDB::QueryState::Lost);
				VQueryState->SetValue("idle", (int)Network::PDB::QueryState::Idle);
				VQueryState->SetValue("idle_in_transaction", (int)Network::PDB::QueryState::IdleInTransaction);
				VQueryState->SetValue("busy", (int)Network::PDB::QueryState::Busy);
				VQueryState->SetValue("busy_in_transaction", (int)Network::PDB::QueryState::BusyInTransaction);

				auto VAddress = VM->SetStructTrivial<Network::PDB::Address>("host_address");
				VAddress->SetConstructor<Network::PDB::Address>("void f()");
				VAddress->SetMethod("void override(const string_view&in, const string_view&in)", &Network::PDB::Address::Override);
				VAddress->SetMethod("bool set(address_op, const string_view&in)", &Network::PDB::Address::Set);
				VAddress->SetMethod<Network::PDB::Address, Core::String, Network::PDB::AddressOp>("string get(address_op) const", &Network::PDB::Address::Get);
				VAddress->SetMethod("string get_address() const", &Network::PDB::Address::GetAddress);
				VAddress->SetMethodStatic("host_address from_url(const string_view&in)", &VI_SEXPECTIFY(Network::PDB::Address::FromURL));

				auto VNotify = VM->SetStructTrivial<Network::PDB::Notify>("notify");
				VNotify->SetConstructor<Network::PDB::Notify, const Network::PDB::Notify&>("void f(const notify&in)");
				VNotify->SetMethod("schema@ get_schema() const", &Network::PDB::Notify::GetSchema);
				VNotify->SetMethod("string get_name() const", &Network::PDB::Notify::GetName);
				VNotify->SetMethod("string get_data() const", &Network::PDB::Notify::GetData);
				VNotify->SetMethod("int32 get_pid() const", &Network::PDB::Notify::GetPid);

				auto VRow = VM->SetStructTrivial<Network::PDB::Row>("row", (size_t)ObjectBehaviours::APP_CLASS_ALLINTS);
				auto VColumn = VM->SetStructTrivial<Network::PDB::Column>("column");
				VColumn->SetConstructor<Network::PDB::Column, const Network::PDB::Column&>("void f(const column&in)");
				VColumn->SetMethod("string get_name() const", &Network::PDB::Column::GetName);
				VColumn->SetMethod("string get_value_text() const", &Network::PDB::Column::GetValueText);
				VColumn->SetMethod("variant get() const", &Network::PDB::Column::Get);
				VColumn->SetMethod("schema@ get_inline() const", &Network::PDB::Column::GetInline);
				VColumn->SetMethod("int32 get_format_id() const", &Network::PDB::Column::GetFormatId);
				VColumn->SetMethod("int32 get_mod_id() const", &Network::PDB::Column::GetModId);
				VColumn->SetMethod("uint64 get_table_id() const", &Network::PDB::Column::GetTableId);
				VColumn->SetMethod("uint64 get_type_id() const", &Network::PDB::Column::GetTypeId);
				VColumn->SetMethod("usize index() const", &Network::PDB::Column::Index);
				VColumn->SetMethod("usize size() const", &Network::PDB::Column::Size);
				VColumn->SetMethod("row get_row() const", &Network::PDB::Column::GetRow);
				VColumn->SetMethod("bool nullable() const", &Network::PDB::Column::Nullable);
				VColumn->SetMethod("bool exists() const", &Network::PDB::Column::Exists);

				auto VResponse = VM->SetStruct<Network::PDB::Response>("response");
				VRow->SetConstructor<Network::PDB::Row, const Network::PDB::Row&>("void f(const row&in)");
				VRow->SetMethod("schema@ get_object() const", &Network::PDB::Row::GetObject);
				VRow->SetMethod("schema@ get_array() const", &Network::PDB::Row::GetArray);
				VRow->SetMethod("usize index() const", &Network::PDB::Row::Index);
				VRow->SetMethod("usize size() const", &Network::PDB::Row::Size);
				VRow->SetMethod<Network::PDB::Row, Network::PDB::Column, size_t>("column get_column(usize) const", &Network::PDB::Row::GetColumn);
				VRow->SetMethod<Network::PDB::Row, Network::PDB::Column, const std::string_view&>("column get_column(const string_view&in) const", &Network::PDB::Row::GetColumn);
				VRow->SetMethod("bool exists() const", &Network::PDB::Row::Exists);
				VRow->SetMethod<Network::PDB::Row, Network::PDB::Column, size_t>("column opIndex(usize)", &Network::PDB::Row::GetColumn);
				VRow->SetMethod<Network::PDB::Row, Network::PDB::Column, size_t>("column opIndex(usize) const", &Network::PDB::Row::GetColumn);
				VRow->SetMethod<Network::PDB::Row, Network::PDB::Column, const std::string_view&>("column opIndex(const string_view&in)", &Network::PDB::Row::GetColumn);
				VRow->SetMethod<Network::PDB::Row, Network::PDB::Column, const std::string_view&>("column opIndex(const string_view&in) const", &Network::PDB::Row::GetColumn);
				
				VResponse->SetConstructor<Network::PDB::Response>("void f()");
				VResponse->SetOperatorCopyStatic(&PDBResponseCopy);
				VResponse->SetDestructor<Network::PDB::Response>("void f()");
				VResponse->SetMethod<Network::PDB::Response, Network::PDB::Row, size_t>("row opIndex(usize)", &Network::PDB::Response::GetRow);
				VResponse->SetMethod<Network::PDB::Response, Network::PDB::Row, size_t>("row opIndex(usize) const", &Network::PDB::Response::GetRow);
				VResponse->SetMethod("schema@ get_array_of_objects() const", &Network::PDB::Response::GetArrayOfObjects);
				VResponse->SetMethod("schema@ get_array_of_arrays() const", &Network::PDB::Response::GetArrayOfArrays);
				VResponse->SetMethod("schema@ get_object(usize = 0) const", &Network::PDB::Response::GetObject);
				VResponse->SetMethod("schema@ get_array(usize = 0) const", &Network::PDB::Response::GetArray);
				VResponse->SetMethodEx("array<string>@ get_columns() const", &PDBResponseGetColumns);
				VResponse->SetMethod("string get_command_status_text() const", &Network::PDB::Response::GetCommandStatusText);
				VResponse->SetMethod("string get_status_text() const", &Network::PDB::Response::GetStatusText);
				VResponse->SetMethod("string get_error_text() const", &Network::PDB::Response::GetErrorText);
				VResponse->SetMethod("string get_error_field(field_code) const", &Network::PDB::Response::GetErrorField);
				VResponse->SetMethod("int32 get_name_index(const string_view&in) const", &Network::PDB::Response::GetNameIndex);
				VResponse->SetMethod("query_exec get_status() const", &Network::PDB::Response::GetStatus);
				VResponse->SetMethod("uint64 get_value_id() const", &Network::PDB::Response::GetValueId);
				VResponse->SetMethod("usize affected_rows() const", &Network::PDB::Response::AffectedRows);
				VResponse->SetMethod("usize size() const", &Network::PDB::Response::Size);
				VResponse->SetMethod("row get_row(usize) const", &Network::PDB::Response::GetRow);
				VResponse->SetMethod("row front() const", &Network::PDB::Response::Front);
				VResponse->SetMethod("row back() const", &Network::PDB::Response::Back);
				VResponse->SetMethod("response copy() const", &Network::PDB::Response::Copy);
				VResponse->SetMethod("uptr@ get() const", &Network::PDB::Response::Get);
				VResponse->SetMethod("bool empty() const", &Network::PDB::Response::Empty);
				VResponse->SetMethod("bool error() const", &Network::PDB::Response::Error);
				VResponse->SetMethod("bool error_or_empty() const", &Network::PDB::Response::ErrorOrEmpty);
				VResponse->SetMethod("bool exists() const", &Network::PDB::Response::Exists);

				auto VConnection = VM->SetClass<Network::PDB::Connection>("connection", false);
				auto VCursor = VM->SetStruct<Network::PDB::Cursor>("cursor");
				VCursor->SetConstructor<Network::PDB::Cursor>("void f()");
				VCursor->SetOperatorCopyStatic(&PDBCursorCopy);
				VCursor->SetDestructor<Network::PDB::Cursor>("void f()");
				VCursor->SetMethod("column opIndex(const string_view&in)", &Network::PDB::Cursor::GetColumn);
				VCursor->SetMethod("column opIndex(const string_view&in) const", &Network::PDB::Cursor::GetColumn);
				VCursor->SetMethod("bool success() const", &Network::PDB::Cursor::Success);
				VCursor->SetMethod("bool empty() const", &Network::PDB::Cursor::Empty);
				VCursor->SetMethod("bool error() const", &Network::PDB::Cursor::Error);
				VCursor->SetMethod("bool error_or_empty() const", &Network::PDB::Cursor::ErrorOrEmpty);
				VCursor->SetMethod("usize size() const", &Network::PDB::Cursor::Size);
				VCursor->SetMethod("usize affected_rows() const", &Network::PDB::Cursor::AffectedRows);
				VCursor->SetMethod("cursor copy() const", &Network::PDB::Cursor::Copy);
				VCursor->SetMethodEx("response first() const", &PDBCursorFirst);
				VCursor->SetMethodEx("response last() const", &PDBCursorLast);
				VCursor->SetMethodEx("response at(usize) const", &PDBCursorAt);
				VCursor->SetMethod("connection@+ get_executor() const", &Network::PDB::Cursor::GetExecutor);
				VCursor->SetMethod("caching get_cache_status() const", &Network::PDB::Cursor::GetCacheStatus);
				VCursor->SetMethod("schema@ get_array_of_objects(usize = 0) const", &Network::PDB::Cursor::GetArrayOfObjects);
				VCursor->SetMethod("schema@ get_array_of_arrays(usize = 0) const", &Network::PDB::Cursor::GetArrayOfArrays);
				VCursor->SetMethod("schema@ get_object(usize = 0, usize = 0) const", &Network::PDB::Cursor::GetObject);
				VCursor->SetMethod("schema@ get_array(usize = 0, usize = 0) const", &Network::PDB::Cursor::GetArray);

				auto VRequest = VM->SetClass<Network::PDB::Request>("request", false);
				VConnection->SetMethod("uptr@ get_base() const", &Network::PDB::Connection::GetBase);
				VConnection->SetMethod("socket@+ get_stream() const", &Network::PDB::Connection::GetStream);
				VConnection->SetMethod("request@+ get_current() const", &Network::PDB::Connection::GetCurrent);
				VConnection->SetMethod("query_state get_state() const", &Network::PDB::Connection::GetState);
				VConnection->SetMethod("transaction_state get_tx_state() const", &Network::PDB::Connection::GetTxState);
				VConnection->SetMethod("bool in_transaction() const", &Network::PDB::Connection::InTransaction);
				VConnection->SetMethod("bool busy() const", &Network::PDB::Connection::Busy);

				VRequest->SetMethod("cursor& get_result()", &Network::PDB::Request::GetResult);
				VRequest->SetMethod("connection@+ get_session() const", &Network::PDB::Request::GetSession);
				VRequest->SetMethod("uint64 get_timing() const", &Network::PDB::Request::GetTiming);
				VRequest->SetMethod("bool pending() const", &Network::PDB::Request::Pending);

				auto VCluster = VM->SetClass<Network::PDB::Cluster>("cluster", false);
				VCluster->SetFunctionDef("promise<bool>@ reconnect_async(cluster@+, array<string>@+)");
				VCluster->SetFunctionDef("void notification_async(cluster@+, const notify&in)");
				VCluster->SetConstructor<Network::PDB::Cluster>("cluster@ f()");
				VCluster->SetMethod("void clear_cache()", &Network::PDB::Cluster::ClearCache);
				VCluster->SetMethod("void set_cache_cleanup(uint64)", &Network::PDB::Cluster::SetCacheCleanup);
				VCluster->SetMethod("void set_cache_duration(query_op, uint64)", &Network::PDB::Cluster::SetCacheDuration);
				VCluster->SetMethod("bool remove_channel(const string_view&in, uint64)", &Network::PDB::Cluster::RemoveChannel);
				VCluster->SetMethod("connection@+ get_connection(query_state)", &Network::PDB::Cluster::GetConnection);
				VCluster->SetMethod("connection@+ get_any_connection()", &Network::PDB::Cluster::GetAnyConnection);
				VCluster->SetMethod("bool is_connected() const", &Network::PDB::Cluster::IsConnected);
				VCluster->SetMethodEx("promise<connection@>@ tx_begin(isolation)", &VI_SPROMISIFY_REF(PDBClusterTxBegin, Connection));
				VCluster->SetMethodEx("promise<connection@>@ tx_start(const string_view&in)", &VI_SPROMISIFY_REF(PDBClusterTxStart, Connection));
				VCluster->SetMethodEx("promise<bool>@ tx_end(const string_view&in, connection@+)", &VI_SPROMISIFY(PDBClusterTxEnd, TypeId::BOOL));
				VCluster->SetMethodEx("promise<bool>@ tx_commit(connection@+)", &VI_SPROMISIFY(PDBClusterTxCommit, TypeId::BOOL));
				VCluster->SetMethodEx("promise<bool>@ tx_rollback(connection@+)", &VI_SPROMISIFY(PDBClusterTxRollback, TypeId::BOOL));
				VCluster->SetMethodEx("promise<bool>@ connect(const host_address&in, usize = 1)", &VI_SPROMISIFY(PDBClusterConnect, TypeId::BOOL));
				VCluster->SetMethodEx("promise<bool>@ disconnect()", &VI_SPROMISIFY(PDBClusterDisconnect, TypeId::BOOL));
				VCluster->SetMethodEx("promise<cursor>@ query(const string_view&in, usize = 0, connection@+ = null)", &VI_SPROMISIFY_REF(PDBClusterQuery, Cursor));
				VCluster->SetMethodEx("void set_when_reconnected(reconnect_async@)", &PDBClusterSetWhenReconnected);
				VCluster->SetMethodEx("uint64 add_channel(const string_view&in, notification_async@)", &PDBClusterAddChannel);
				VCluster->SetMethodEx("promise<bool>@ listen(array<string>@+)", &VI_SPROMISIFY(PDBClusterListen, TypeId::BOOL));
				VCluster->SetMethodEx("promise<bool>@ unlisten(array<string>@+)", &VI_SPROMISIFY(PDBClusterUnlisten, TypeId::BOOL));
				VCluster->SetMethodEx("promise<cursor>@ emplace_query(const string_view&in, array<schema@>@+, usize = 0, connection@+ = null)", &VI_SPROMISIFY_REF(PDBClusterEmplaceQuery, Cursor));
				VCluster->SetMethodEx("promise<cursor>@ template_query(const string_view&in, dictionary@+, usize = 0, connection@+ = null)", &VI_SPROMISIFY_REF(PDBClusterTemplateQuery, Cursor));

				auto VDriver = VM->SetClass<Network::PDB::Driver>("driver", false);
				VDriver->SetFunctionDef("void query_log_async(const string_view&in)");
				VDriver->SetConstructor<Network::PDB::Driver>("driver@ f()");
				VDriver->SetMethod("void log_query(const string_view&in)", &Network::PDB::Driver::LogQuery);
				VDriver->SetMethod("void add_constant(const string_view&in, const string_view&in)", &Network::PDB::Driver::AddConstant);
				VDriver->SetMethodEx("bool add_query(const string_view&in, const string_view&in)", &VI_EXPECTIFY_VOID(Network::PDB::Driver::AddQuery));
				VDriver->SetMethodEx("bool add_directory(const string_view&in, const string_view&in = string_view())", &VI_EXPECTIFY_VOID(Network::PDB::Driver::AddDirectory));
				VDriver->SetMethod("bool remove_constant(const string_view&in)", &Network::PDB::Driver::RemoveConstant);
				VDriver->SetMethod("bool remove_query(const string_view&in)", &Network::PDB::Driver::RemoveQuery);
				VDriver->SetMethod("bool load_cache_dump(schema@+)", &Network::PDB::Driver::LoadCacheDump);
				VDriver->SetMethod("schema@ get_cache_dump()", &Network::PDB::Driver::GetCacheDump);
				VDriver->SetMethodEx("void set_query_log(query_log_async@)", &PDBDriverSetQueryLog);
				VDriver->SetMethodEx("string emplace(cluster@+, const string_view&in, array<schema@>@+)", &PDBDriverEmplace);
				VDriver->SetMethodEx("string get_query(cluster@+, const string_view&in, dictionary@+)", &PDBDriverGetQuery);
				VDriver->SetMethodEx("array<string>@ get_queries()", &PDBDriverGetQueries);
				VDriver->SetMethodStatic("driver@+ get()", &Network::PDB::Driver::Get);

				VM->EndNamespace();
				return true;
#else
				VI_ASSERT(false, "<postgresql> is not loaded");
				return false;
#endif
			}
			bool Registry::ImportMongoDB(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr, "manager should be set");
				VI_TYPEREF(Property, "mdb::doc_property");
				VI_TYPEREF(Document, "mdb::document");
				VI_TYPEREF(Cursor, "mdb::cursor");
				VI_TYPEREF(Response, "mdb::response");
				VI_TYPEREF(Collection, "mdb::collection");
				VI_TYPEREF(TransactionState, "mdb::transaction_state");
				VI_TYPEREF(Schema, "schema@");

				VM->BeginNamespace("mdb");
				auto VQueryFlags = VM->SetEnum("query_flags");
				VQueryFlags->SetValue("none", (int)Network::MDB::QueryFlags::None);
				VQueryFlags->SetValue("tailable_cursor", (int)Network::MDB::QueryFlags::Tailable_Cursor);
				VQueryFlags->SetValue("slave_ok", (int)Network::MDB::QueryFlags::Slave_Ok);
				VQueryFlags->SetValue("oplog_replay", (int)Network::MDB::QueryFlags::Oplog_Replay);
				VQueryFlags->SetValue("no_cursor_timeout", (int)Network::MDB::QueryFlags::No_Cursor_Timeout);
				VQueryFlags->SetValue("await_data", (int)Network::MDB::QueryFlags::Await_Data);
				VQueryFlags->SetValue("exhaust", (int)Network::MDB::QueryFlags::Exhaust);
				VQueryFlags->SetValue("partial", (int)Network::MDB::QueryFlags::Partial);

				auto VType = VM->SetEnum("doc_type");
				VType->SetValue("unknown_t", (int)Network::MDB::Type::Unknown);
				VType->SetValue("uncastable_t", (int)Network::MDB::Type::Uncastable);
				VType->SetValue("null_t", (int)Network::MDB::Type::Null);
				VType->SetValue("document_t", (int)Network::MDB::Type::Document);
				VType->SetValue("array_t", (int)Network::MDB::Type::Array);
				VType->SetValue("string_t", (int)Network::MDB::Type::String);
				VType->SetValue("integer_t", (int)Network::MDB::Type::Integer);
				VType->SetValue("number_t", (int)Network::MDB::Type::Number);
				VType->SetValue("boolean_t", (int)Network::MDB::Type::Boolean);
				VType->SetValue("object_id_t", (int)Network::MDB::Type::ObjectId);
				VType->SetValue("decimal_t", (int)Network::MDB::Type::Decimal);

				auto VTransactionState = VM->SetEnum("transaction_state");
				VTransactionState->SetValue("ok", (int)Network::MDB::TransactionState::OK);
				VTransactionState->SetValue("retry_commit", (int)Network::MDB::TransactionState::Retry_Commit);
				VTransactionState->SetValue("retry", (int)Network::MDB::TransactionState::Retry);
				VTransactionState->SetValue("fatal", (int)Network::MDB::TransactionState::Fatal);

				auto VDocument = VM->SetStruct<Network::MDB::Document>("document");
				auto VProperty = VM->SetStruct<Network::MDB::Property>("doc_property");
				VProperty->SetProperty<Network::MDB::Property>("string name", &Network::MDB::Property::Name);
				VProperty->SetProperty<Network::MDB::Property>("string blob", &Network::MDB::Property::String);
				VProperty->SetProperty<Network::MDB::Property>("doc_type mod", &Network::MDB::Property::Mod);
				VProperty->SetProperty<Network::MDB::Property>("int64 integer", &Network::MDB::Property::Integer);
				VProperty->SetProperty<Network::MDB::Property>("uint64 high", &Network::MDB::Property::High);
				VProperty->SetProperty<Network::MDB::Property>("uint64 low", &Network::MDB::Property::Low);
				VProperty->SetProperty<Network::MDB::Property>("bool boolean", &Network::MDB::Property::Boolean);
				VProperty->SetProperty<Network::MDB::Property>("bool is_valid", &Network::MDB::Property::IsValid);
				VProperty->SetConstructor<Network::MDB::Property>("void f()");
				VProperty->SetOperatorMoveCopy<Network::MDB::Property>();
				VProperty->SetDestructor<Network::MDB::Property>("void f()");
				VProperty->SetMethod("string& to_string()", &Network::MDB::Property::ToString);
				VProperty->SetMethod("document as_document() const", &Network::MDB::Property::AsDocument);
				VProperty->SetMethod("doc_property opIndex(const string_view&in) const", &Network::MDB::Property::At);

				VDocument->SetConstructor<Network::MDB::Document>("void f()");
				VDocument->SetConstructorListEx<Network::MDB::Document>("void f(int &in) { repeat { string, ? } }", &MDBDocumentConstruct);
				VDocument->SetOperatorMoveCopy<Network::MDB::Document>();
				VDocument->SetDestructor<Network::MDB::Document>("void f()");
				VDocument->SetMethod("bool is_valid() const", &Network::MDB::Document::operator bool);
				VDocument->SetMethod("void cleanup()", &Network::MDB::Document::Cleanup);
				VDocument->SetMethod("void join(const document&in)", &Network::MDB::Document::Join);
				VDocument->SetMethod("bool set_schema(const string_view&in, const document&in, usize = 0)", &Network::MDB::Document::SetSchema);
				VDocument->SetMethod("bool set_array(const string_view&in, const document&in, usize = 0)", &Network::MDB::Document::SetArray);
				VDocument->SetMethod("bool set_string(const string_view&in, const string_view&in, usize = 0)", &Network::MDB::Document::SetString);
				VDocument->SetMethod("bool set_integer(const string_view&in, int64, usize = 0)", &Network::MDB::Document::SetInteger);
				VDocument->SetMethod("bool set_number(const string_view&in, double, usize = 0)", &Network::MDB::Document::SetNumber);
				VDocument->SetMethod("bool set_decimal(const string_view&in, uint64, uint64, usize = 0)", &Network::MDB::Document::SetDecimal);
				VDocument->SetMethod("bool set_decimal_string(const string_view&in, const string_view&in, usize = 0)", &Network::MDB::Document::SetDecimalString);
				VDocument->SetMethod("bool set_decimal_integer(const string_view&in, int64, usize = 0)", &Network::MDB::Document::SetDecimalInteger);
				VDocument->SetMethod("bool set_decimal_number(const string_view&in, double, usize = 0)", &Network::MDB::Document::SetDecimalNumber);
				VDocument->SetMethod("bool set_boolean(const string_view&in, bool, usize = 0)", &Network::MDB::Document::SetBoolean);
				VDocument->SetMethod("bool set_object_id(const string_view&in, const string_view&in, usize = 0)", &Network::MDB::Document::SetObjectId);
				VDocument->SetMethod("bool set_null(const string_view&in, usize = 0)", &Network::MDB::Document::SetNull);
				VDocument->SetMethod("bool set_property(const string_view&in, doc_property&in, usize = 0)", &Network::MDB::Document::SetProperty);
				VDocument->SetMethod("bool has_property(const string_view&in) const", &Network::MDB::Document::HasProperty);
				VDocument->SetMethod("bool get_property(const string_view&in, doc_property&out) const", &Network::MDB::Document::GetProperty);
				VDocument->SetMethod("usize count() const", &Network::MDB::Document::Count);
				VDocument->SetMethod("string to_relaxed_json() const", &Network::MDB::Document::ToRelaxedJSON);
				VDocument->SetMethod("string to_extended_json() const", &Network::MDB::Document::ToExtendedJSON);
				VDocument->SetMethod("string to_json() const", &Network::MDB::Document::ToJSON);
				VDocument->SetMethod("string to_indices() const", &Network::MDB::Document::ToIndices);
				VDocument->SetMethod("schema@ to_schema(bool = false) const", &Network::MDB::Document::ToSchema);
				VDocument->SetMethod("document copy() const", &Network::MDB::Document::Copy);
				VDocument->SetMethod("document& persist(bool = true) const", &Network::MDB::Document::Persist);
				VDocument->SetMethod("doc_property opIndex(const string_view&in) const", &Network::MDB::Document::At);
				VDocument->SetMethodStatic("document from_empty()", &Network::MDB::Document::FromEmpty);
				VDocument->SetMethodStatic("document from_schema(schema@+)", &Network::MDB::Document::FromSchema);
				VDocument->SetMethodStatic("document from_json(const string_view&in)", &VI_SEXPECTIFY(Network::MDB::Document::FromJSON));

				auto VAddress = VM->SetStruct<Network::MDB::Address>("host_address");
				VAddress->SetConstructor<Network::MDB::Address>("void f()");
				VAddress->SetOperatorMoveCopy<Network::MDB::Address>();
				VAddress->SetDestructor<Network::MDB::Address>("void f()");
				VAddress->SetMethod("bool is_valid() const", &Network::MDB::Address::operator bool);
				VAddress->SetMethod<Network::MDB::Address, void, const std::string_view&, int64_t>("void set_option(const string_view&in, int64)", &Network::MDB::Address::SetOption);
				VAddress->SetMethod<Network::MDB::Address, void, const std::string_view&, bool>("void set_option(const string_view&in, bool)", &Network::MDB::Address::SetOption);
				VAddress->SetMethod<Network::MDB::Address, void, const std::string_view&, const std::string_view&>("void set_option(const string_view&in, const string_view&in)", &Network::MDB::Address::SetOption);
				VAddress->SetMethod("void set_auth_mechanism(const string_view&in)", &Network::MDB::Address::SetAuthMechanism);
				VAddress->SetMethod("void set_auth_source(const string_view&in)", &Network::MDB::Address::SetAuthSource);
				VAddress->SetMethod("void set_compressors(const string_view&in)", &Network::MDB::Address::SetCompressors);
				VAddress->SetMethod("void set_database(const string_view&in)", &Network::MDB::Address::SetDatabase);
				VAddress->SetMethod("void set_username(const string_view&in)", &Network::MDB::Address::SetUsername);
				VAddress->SetMethod("void set_password(const string_view&in)", &Network::MDB::Address::SetPassword);
				VAddress->SetMethodStatic("host_address from_url(const string_view&in)", &VI_SEXPECTIFY(Network::MDB::Address::FromURL));

				auto VStream = VM->SetStruct<Network::MDB::Stream>("stream");
				VStream->SetConstructor<Network::MDB::Stream>("void f()");
				VStream->SetOperatorMoveCopy<Network::MDB::Stream>();
				VStream->SetDestructor<Network::MDB::Stream>("void f()");
				VStream->SetMethod("bool is_valid() const", &Network::MDB::Stream::operator bool);
				VStream->SetMethodEx("bool remove_many(const document&in, const document&in)", &VI_EXPECTIFY_VOID(Network::MDB::Stream::RemoveMany));
				VStream->SetMethodEx("bool remove_one(const document&in, const document&in)", &VI_EXPECTIFY_VOID(Network::MDB::Stream::RemoveOne));
				VStream->SetMethodEx("bool replace_one(const document&in, const document&in, const document&in)", &VI_EXPECTIFY_VOID(Network::MDB::Stream::ReplaceOne));
				VStream->SetMethodEx("bool insert_one(const document&in, const document&in)", &VI_EXPECTIFY_VOID(Network::MDB::Stream::InsertOne));
				VStream->SetMethodEx("bool update_one(const document&in, const document&in, const document&in)", &VI_EXPECTIFY_VOID(Network::MDB::Stream::UpdateOne));
				VStream->SetMethodEx("bool update_many(const document&in, const document&in, const document&in)", &VI_EXPECTIFY_VOID(Network::MDB::Stream::UpdateMany));
				VStream->SetMethodEx("bool query(const document&in)", &VI_EXPECTIFY_VOID(Network::MDB::Stream::Query));
				VStream->SetMethod("usize get_hint() const", &Network::MDB::Stream::GetHint);
				VStream->SetMethodEx("bool template_query(const string_view&in, dictionary@+)", &MDBStreamTemplateQuery);
				VStream->SetMethodEx("promise<document>@ execute_with_reply()", &VI_SPROMISIFY_REF(MDBStreamExecuteWithReply, Document));
				VStream->SetMethodEx("promise<bool>@ execute()", &VI_SPROMISIFY(MDBStreamExecute, TypeId::BOOL));

				auto VCursor = VM->SetStruct<Network::MDB::Cursor>("cursor");
				VCursor->SetConstructor<Network::MDB::Cursor>("void f()");
				VCursor->SetOperatorMoveCopy<Network::MDB::Cursor>();
				VCursor->SetDestructor<Network::MDB::Cursor>("void f()");
				VCursor->SetMethod("bool is_valid() const", &Network::MDB::Cursor::operator bool);
				VCursor->SetMethod("void set_max_await_time(usize)", &Network::MDB::Cursor::SetMaxAwaitTime);
				VCursor->SetMethod("void set_batch_size(usize)", &Network::MDB::Cursor::SetBatchSize);
				VCursor->SetMethod("void set_limit(int64)", &Network::MDB::Cursor::SetLimit);
				VCursor->SetMethod("void set_hint(usize)", &Network::MDB::Cursor::SetHint);
				VCursor->SetMethodEx("string error() const", &MDBCursorError);
				VCursor->SetMethod("bool empty() const", &Network::MDB::Cursor::Empty);
				VCursor->SetMethod("bool error_or_empty() const", &Network::MDB::Cursor::ErrorOrEmpty);
				VCursor->SetMethod("int64 get_id() const", &Network::MDB::Cursor::GetId);
				VCursor->SetMethod("int64 get_limit() const", &Network::MDB::Cursor::GetLimit);
				VCursor->SetMethod("usize get_max_await_time() const", &Network::MDB::Cursor::GetMaxAwaitTime);
				VCursor->SetMethod("usize get_batch_size() const", &Network::MDB::Cursor::GetBatchSize);
				VCursor->SetMethod("usize get_hint() const", &Network::MDB::Cursor::GetHint);
				VCursor->SetMethod("usize current() const", &Network::MDB::Cursor::Current);
				VCursor->SetMethod("cursor clone() const", &Network::MDB::Cursor::Clone);
				VCursor->SetMethodEx("promise<bool>@ next() const", &VI_SPROMISIFY(MDBCursorNext, TypeId::BOOL));

				auto VResponse = VM->SetStruct<Network::MDB::Response>("response");
				VResponse->SetConstructor<Network::MDB::Response>("void f()");
				VResponse->SetConstructor<Network::MDB::Response, bool>("void f(bool)");
				VResponse->SetConstructor<Network::MDB::Response, Network::MDB::Cursor&>("void f(cursor&in)");
				VResponse->SetConstructor<Network::MDB::Response, Network::MDB::Document&>("void f(document&in)");
				VResponse->SetOperatorMoveCopy<Network::MDB::Response>();
				VResponse->SetDestructor<Network::MDB::Response>("void f()");
				VResponse->SetMethod("bool is_valid() const", &Network::MDB::Response::operator bool);
				VResponse->SetMethod("bool success() const", &Network::MDB::Response::Success);
				VResponse->SetMethod("cursor& get_cursor() const", &Network::MDB::Response::GetCursor);
				VResponse->SetMethod("document& get_document() const", &Network::MDB::Response::GetDocument);
				VResponse->SetMethodEx("promise<schema@>@ fetch() const", &VI_SPROMISIFY_REF(MDBResponseFetch, Schema));
				VResponse->SetMethodEx("promise<schema@>@ fetch_all() const", &VI_SPROMISIFY_REF(MDBResponseFetchAll, Schema));
				VResponse->SetMethodEx("promise<doc_property>@ get_property(const string_view&in) const", &VI_SPROMISIFY_REF(MDBResponseGetProperty, Property));
				VResponse->SetMethodEx("promise<doc_property>@ opIndex(const string_view&in) const", &VI_SPROMISIFY_REF(MDBResponseGetProperty, Property));

				auto VCollection = VM->SetStruct<Network::MDB::Collection>("collection");
				auto VTransaction = VM->SetStruct<Network::MDB::Transaction>("transaction");
				VTransaction->SetConstructor<Network::MDB::Transaction>("void f()");
				VTransaction->SetConstructor<Network::MDB::Transaction, const Network::MDB::Transaction&>("void f(const transaction&in)");
				VTransaction->SetOperatorCopy<Network::MDB::Transaction>();
				VTransaction->SetDestructor<Network::MDB::Transaction>("void f()");
				VTransaction->SetMethod("bool is_valid() const", &Network::MDB::Transaction::operator bool);
				VTransaction->SetMethodEx("bool push(document&in) const", &VI_EXPECTIFY_VOID(Network::MDB::Transaction::Push));
				VTransaction->SetMethodEx("promise<bool>@ begin()", &VI_SPROMISIFY(MDBTransactionBegin, TypeId::BOOL));
				VTransaction->SetMethodEx("promise<bool>@ rollback()", &VI_SPROMISIFY(MDBTransactionRollback, TypeId::BOOL));
				VTransaction->SetMethodEx("promise<document>@ remove_many(collection&in, const document&in, const document&in)", &VI_SPROMISIFY_REF(MDBTransactionRemoveMany, Document));
				VTransaction->SetMethodEx("promise<document>@ remove_one(collection&in, const document&in, const document&in)", &VI_SPROMISIFY_REF(MDBTransactionRemoveOne, Document));
				VTransaction->SetMethodEx("promise<document>@ replace_one(collection&in, const document&in, const document&in, const document&in)", &VI_SPROMISIFY_REF(MDBTransactionReplaceOne, Document));
				VTransaction->SetMethodEx("promise<document>@ insert_many(collection&in, array<document>@+, const document&in)", &VI_SPROMISIFY_REF(MDBTransactionInsertMany, Document));
				VTransaction->SetMethodEx("promise<document>@ insert_one(collection&in, const document&in, const document&in)", &VI_SPROMISIFY_REF(MDBTransactionInsertOne, Document));
				VTransaction->SetMethodEx("promise<document>@ update_many(collection&in, const document&in, const document&in, const document&in)", &VI_SPROMISIFY_REF(MDBTransactionUpdateMany, Document));
				VTransaction->SetMethodEx("promise<document>@ update_one(collection&in, const document&in, const document&in, const document&in)", &VI_SPROMISIFY_REF(MDBTransactionUpdateOne, Document));
				VTransaction->SetMethodEx("promise<cursor>@ find_many(collection&in, const document&in, const document&in)", &VI_SPROMISIFY_REF(MDBTransactionFindMany, Cursor));
				VTransaction->SetMethodEx("promise<cursor>@ find_one(collection&in, const document&in, const document&in)", &VI_SPROMISIFY_REF(MDBTransactionFindOne, Cursor));
				VTransaction->SetMethodEx("promise<cursor>@ aggregate(collection&in, query_flags, const document&in, const document&in)", &VI_SPROMISIFY_REF(MDBTransactionAggregate, Cursor));
				VTransaction->SetMethodEx("promise<response>@ template_query(collection&in, const string_view&in, dictionary@+)", &VI_SPROMISIFY_REF(MDBTransactionTemplateQuery, Response));
				VTransaction->SetMethodEx("promise<response>@ query(collection&in, const document&in)", &VI_SPROMISIFY_REF(MDBTransactionQuery, Response));
				VTransaction->SetMethodEx("promise<transaction_state>@ commit()", &VI_SPROMISIFY_REF(MDBTransactionCommit, TransactionState));

				VCollection->SetConstructor<Network::MDB::Collection>("void f()");
				VCollection->SetOperatorMoveCopy<Network::MDB::Collection>();
				VCollection->SetDestructor<Network::MDB::Collection>("void f()");
				VCollection->SetMethod("bool is_valid() const", &Network::MDB::Collection::operator bool);
				VCollection->SetMethod("string get_name() const", &Network::MDB::Collection::GetName);
				VCollection->SetMethodEx("stream create_stream(document&in) const", &VI_EXPECTIFY(Network::MDB::Collection::CreateStream));
				VCollection->SetMethodEx("promise<bool>@ rename(const string_view&in, const string_view&in) const", &VI_SPROMISIFY(MDBCollectionRename, TypeId::BOOL));
				VCollection->SetMethodEx("promise<bool>@ rename_with_options(const string_view&in, const string_view&in, const document&in) const", &VI_SPROMISIFY(MDBCollectionRenameWithOptions, TypeId::BOOL));
				VCollection->SetMethodEx("promise<bool>@ rename_with_remove(const string_view&in, const string_view&in) const", &VI_SPROMISIFY(MDBCollectionRenameWithRemove, TypeId::BOOL));
				VCollection->SetMethodEx("promise<bool>@ rename_with_options_and_remove(const string_view&in, const string_view&in, const document&in) const", &VI_SPROMISIFY(MDBCollectionRenameWithOptionsAndRemove, TypeId::BOOL));
				VCollection->SetMethodEx("promise<bool>@ remove(const document&in) const", &VI_SPROMISIFY(MDBCollectionRemove, TypeId::BOOL));
				VCollection->SetMethodEx("promise<bool>@ remove_index(const string_view&in, const document&in) const", &VI_SPROMISIFY(MDBCollectionRemoveIndex, TypeId::BOOL));
				VCollection->SetMethodEx("promise<document>@ remove_many(const document&in, const document&in) const", &VI_SPROMISIFY_REF(MDBCollectionRemoveMany, Document));
				VCollection->SetMethodEx("promise<document>@ remove_one(const document&in, const document&in) const", &VI_SPROMISIFY_REF(MDBCollectionRemoveOne, Document));
				VCollection->SetMethodEx("promise<document>@ replace_one(const document&in, const document&in, const document&in) const", &VI_SPROMISIFY_REF(MDBCollectionReplaceOne, Document));
				VCollection->SetMethodEx("promise<document>@ insert_many(array<document>@+, const document&in) const", &VI_SPROMISIFY_REF(MDBCollectionInsertMany, Document));
				VCollection->SetMethodEx("promise<document>@ insert_one(const document&in, const document&in) const", &VI_SPROMISIFY_REF(MDBCollectionInsertOne, Document));
				VCollection->SetMethodEx("promise<document>@ update_many(const document&in, const document&in, const document&in) const", &VI_SPROMISIFY_REF(MDBCollectionUpdateMany, Document));
				VCollection->SetMethodEx("promise<document>@ update_one(const document&in, const document&in, const document&in) const", &VI_SPROMISIFY_REF(MDBCollectionUpdateOne, Document));
				VCollection->SetMethodEx("promise<document>@ find_and_modify(const document&in, const document&in, const document&in, const document&in, bool, bool, bool) const", &VI_SPROMISIFY_REF(MDBCollectionFindAndModify, Document));
#ifdef VI_64
				VCollection->SetMethodEx("promise<usize>@ count_documents(const document&in, const document&in) const", &VI_PROMISIFY(Network::MDB::Collection::CountDocuments, TypeId::INT64));
				VCollection->SetMethodEx("promise<usize>@ count_documents_estimated(const document&in) const", &VI_PROMISIFY(Network::MDB::Collection::CountDocumentsEstimated, TypeId::INT64));
#else
				VCollection->SetMethodEx("promise<usize>@ count_documents(const document&in, const document&in) const", &VI_PROMISIFY(Network::MDB::Collection::CountDocuments, TypeId::INT32));
				VCollection->SetMethodEx("promise<usize>@ count_documents_estimated(const document&in) const", &VI_PROMISIFY(Network::MDB::Collection::CountDocumentsEstimated, TypeId::INT32));
#endif
				VCollection->SetMethodEx("promise<cursor>@ find_indexes(const document&in) const", &VI_SPROMISIFY_REF(MDBCollectionFindIndexes, Cursor));
				VCollection->SetMethodEx("promise<cursor>@ find_many(const document&in, const document&in) const", &VI_SPROMISIFY_REF(MDBCollectionFindMany, Cursor));
				VCollection->SetMethodEx("promise<cursor>@ find_one(const document&in, const document&in) const", &VI_SPROMISIFY_REF(MDBCollectionFindOne, Cursor));
				VCollection->SetMethodEx("promise<cursor>@ aggregate(query_flags, const document&in, const document&in) const", &VI_SPROMISIFY_REF(MDBCollectionAggregate, Cursor));
				VCollection->SetMethodEx("promise<response>@ template_query(const string_view&in, dictionary@+, bool = true, const transaction&in = transaction()) const", &VI_SPROMISIFY_REF(MDBCollectionTemplateQuery, Response));
				VCollection->SetMethodEx("promise<response>@ query(const document&in, const transaction&in = transaction()) const", &VI_SPROMISIFY_REF(MDBCollectionQuery, Response));

				auto VDatabase = VM->SetStruct<Network::MDB::Database>("database");
				VDatabase->SetConstructor<Network::MDB::Database>("void f()");
				VDatabase->SetOperatorMoveCopy<Network::MDB::Database>();
				VDatabase->SetDestructor<Network::MDB::Database>("void f()");
				VDatabase->SetMethod("bool is_valid() const", &Network::MDB::Database::operator bool);
				VDatabase->SetMethod("string get_name() const", &Network::MDB::Database::GetName);
				VDatabase->SetMethod("collection get_collection(const string_view&in) const", &Network::MDB::Database::GetCollection);
				VDatabase->SetMethodEx("promise<bool>@ remove_all_users()", &VI_SPROMISIFY(MDBDatabaseRemoveAllUsers, TypeId::BOOL));
				VDatabase->SetMethodEx("promise<bool>@ remove_user(const string_view&in)", &VI_SPROMISIFY(MDBDatabaseRemoveUser, TypeId::BOOL));
				VDatabase->SetMethodEx("promise<bool>@ remove()", &VI_SPROMISIFY(MDBDatabaseRemove, TypeId::BOOL));
				VDatabase->SetMethodEx("promise<bool>@ remove_with_options(const document&in)", &VI_SPROMISIFY(MDBDatabaseRemoveWithOptions, TypeId::BOOL));
				VDatabase->SetMethodEx("promise<bool>@ add_user(const string_view&in, const string_view&in, const document&in, const document&in)", &VI_SPROMISIFY(MDBDatabaseAddUser, TypeId::BOOL));
				VDatabase->SetMethodEx("promise<bool>@ has_collection(const string_view&in)", &VI_SPROMISIFY(MDBDatabaseHasCollection, TypeId::BOOL));
				VDatabase->SetMethodEx("promise<collection>@ create_collection(const string_view&in, const document&in)", &VI_SPROMISIFY_REF(MDBDatabaseCreateCollection, Collection));
				VDatabase->SetMethodEx("promise<cursor>@ find_collections(const document&in)", &VI_SPROMISIFY_REF(MDBDatabaseFindCollections, Cursor));
				VDatabase->SetMethodEx("array<string>@ get_collection_names(const document&in)", &MDBDatabaseGetCollectionNames);

				auto VConnection = VM->SetClass<Network::MDB::Connection>("connection", false);
				auto VWatcher = VM->SetStruct<Network::MDB::Watcher>("watcher");
				VWatcher->SetConstructor<Network::MDB::Watcher>("void f()");
				VWatcher->SetOperatorMoveCopy<Network::MDB::Watcher>();
				VWatcher->SetDestructor<Network::MDB::Watcher>("void f()");
				VWatcher->SetMethod("bool is_valid() const", &Network::MDB::Watcher::operator bool);
				VWatcher->SetMethodEx("promise<bool>@ next(document&out)", &VI_SPROMISIFY(MDBWatcherNext, TypeId::BOOL));
				VWatcher->SetMethodEx("promise<bool>@ error(document&out)", &VI_SPROMISIFY(MDBWatcherError, TypeId::BOOL));
				VWatcher->SetMethodStatic("watcher from_connection(connection@+, const document&in, const document&in)", &VI_SEXPECTIFY(Network::MDB::Watcher::FromConnection));
				VWatcher->SetMethodStatic("watcher from_database(const database&in, const document&in, const document&in)", &VI_SEXPECTIFY(Network::MDB::Watcher::FromDatabase));
				VWatcher->SetMethodStatic("watcher from_collection(const collection&in, const document&in, const document&in)", &VI_SEXPECTIFY(Network::MDB::Watcher::FromCollection));

				auto VCluster = VM->SetClass<Network::MDB::Cluster>("cluster", false);
				VConnection->SetFunctionDef("promise<bool>@ transaction_async(connection@+, transaction&in)");
				VConnection->SetConstructor<Network::MDB::Connection>("connection@ f()");
				VConnection->SetMethod("void set_profile(const string_view&in)", &Network::MDB::Connection::SetProfile);
				VConnection->SetMethodEx("bool set_server(bool)", &VI_EXPECTIFY_VOID(Network::MDB::Connection::SetServer));
				VConnection->SetMethodEx("transaction& get_session()", &VI_EXPECTIFY(Network::MDB::Connection::GetSession));
				VConnection->SetMethod("database get_database(const string_view&in) const", &Network::MDB::Connection::GetDatabase);
				VConnection->SetMethod("database get_default_database() const", &Network::MDB::Connection::GetDefaultDatabase);
				VConnection->SetMethod("collection get_collection(const string_view&in, const string_view&in) const", &Network::MDB::Connection::GetCollection);
				VConnection->SetMethod("host_address get_address() const", &Network::MDB::Connection::GetAddress);
				VConnection->SetMethod("cluster@+ get_master() const", &Network::MDB::Connection::GetMaster);
				VConnection->SetMethod("bool connected() const", &Network::MDB::Connection::IsConnected);
				VConnection->SetMethodEx("array<string>@ get_database_names(const document&in) const", &MDBConnectionGetDatabaseNames);
				VConnection->SetMethodEx("promise<bool>@ connect(host_address&in)", &VI_SPROMISIFY(MDBConnectionConnect, TypeId::BOOL));
				VConnection->SetMethodEx("promise<bool>@ disconnect()", &VI_SPROMISIFY(MDBConnectionDisconnect, TypeId::BOOL));
				VConnection->SetMethodEx("promise<bool>@ make_transaction(transaction_async@)", &VI_SPROMISIFY(MDBConnectionMakeTransaction, TypeId::BOOL));
				VConnection->SetMethodEx("promise<cursor>@ find_databases(const document&in)", &VI_SPROMISIFY_REF(MDBConnectionFindDatabases, Cursor));

				VCluster->SetConstructor<Network::MDB::Cluster>("cluster@ f()");
				VCluster->SetMethod("void set_profile(const string_view&in)", &Network::MDB::Cluster::SetProfile);
				VCluster->SetMethod("host_address& get_address()", &Network::MDB::Cluster::GetAddress);
				VCluster->SetMethod("connection@+ pop()", &Network::MDB::Cluster::Pop);
				VCluster->SetMethodEx("void push(connection@+)", &MDBClusterPush);
				VCluster->SetMethodEx("promise<bool>@ connect(host_address&in)", &VI_SPROMISIFY(MDBClusterConnect, TypeId::BOOL));
				VCluster->SetMethodEx("promise<bool>@ disconnect()", &VI_SPROMISIFY(MDBClusterDisconnect, TypeId::BOOL));

				auto VDriver = VM->SetClass<Network::MDB::Driver>("driver", false);
				VDriver->SetFunctionDef("void query_log_async(const string&in)");
				VDriver->SetConstructor<Network::MDB::Driver>("driver@ f()");
				VDriver->SetMethod("void add_constant(const string_view&in, const string_view&in)", &Network::MDB::Driver::AddConstant);
				VDriver->SetMethodEx("bool add_query(const string_view&in, const string_view&in)", &VI_EXPECTIFY_VOID(Network::MDB::Driver::AddQuery));
				VDriver->SetMethodEx("bool add_directory(const string_view&in, const string_view&in = string_view())", &VI_EXPECTIFY_VOID(Network::MDB::Driver::AddDirectory));
				VDriver->SetMethod("bool remove_constant(const string_view&in)", &Network::MDB::Driver::RemoveConstant);
				VDriver->SetMethod("bool remove_query(const string_view&in)", &Network::MDB::Driver::RemoveQuery);
				VDriver->SetMethod("bool load_cache_dump(schema@+)", &Network::MDB::Driver::LoadCacheDump);
				VDriver->SetMethod("schema@ get_cache_dump()", &Network::MDB::Driver::GetCacheDump);
				VDriver->SetMethodEx("void set_query_log(query_log_async@)", &MDBDriverSetQueryLog);
				VDriver->SetMethodEx("string get_query(cluster@+, const string_view&in, dictionary@+)", &MDBDriverGetQuery);
				VDriver->SetMethodEx("array<string>@ get_queries()", &MDBDriverGetQueries);
				VDriver->SetMethodStatic("driver@+ get()", &Network::MDB::Driver::Get);

				VM->EndNamespace();
				return true;
#else
				VI_ASSERT(false, "<mongodb> is not loaded");
				return false;
#endif
			}
			bool Registry::ImportVM(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr, "manager should be set");
				VM->BeginNamespace("this_vm");
				VM->SetFunction("void collect_garbage(uint64)", &VMCollectGarbage);
				VM->EndNamespace();

				return true;
#else
				VI_ASSERT(false, "<vm> is not loaded");
				return false;
#endif
			}
			bool Registry::ImportLayer(VirtualMachine* VM) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(VM != nullptr, "manager should be set");
				VI_TYPEREF(ContentManager, "content_manager");
				VI_TYPEREF(AppData, "app_data");
				VI_TYPEREF(ApplicationName, "application");

				auto VApplicationUse = VM->SetEnum("application_use");
				VApplicationUse->SetValue("scripting", (int)Layer::USE_SCRIPTING);
				VApplicationUse->SetValue("processing", (int)Layer::USE_PROCESSING);
				VApplicationUse->SetValue("networking", (int)Layer::USE_NETWORKING);

				auto VApplicationState = VM->SetEnum("application_state");
				VApplicationState->SetValue("terminated_t", (int)Layer::ApplicationState::Terminated);
				VApplicationState->SetValue("staging_t", (int)Layer::ApplicationState::Staging);
				VApplicationState->SetValue("active_t", (int)Layer::ApplicationState::Active);
				VApplicationState->SetValue("restart_t", (int)Layer::ApplicationState::Restart);

				auto VAssetCache = VM->SetStructTrivial<Layer::AssetCache>("asset_cache");
				VAssetCache->SetProperty<Layer::AssetCache>("string path", &Layer::AssetCache::Path);
				VAssetCache->SetProperty<Layer::AssetCache>("uptr@ resource", &Layer::AssetCache::Resource);
				VAssetCache->SetConstructor<Layer::AssetCache>("void f()");

				auto VAssetArchive = VM->SetStructTrivial<Layer::AssetArchive>("asset_archive");
				VAssetArchive->SetProperty<Layer::AssetArchive>("base_stream@ stream", &Layer::AssetArchive::Stream);
				VAssetArchive->SetProperty<Layer::AssetArchive>("string path", &Layer::AssetArchive::Path);
				VAssetArchive->SetProperty<Layer::AssetArchive>("usize length", &Layer::AssetArchive::Length);
				VAssetArchive->SetProperty<Layer::AssetArchive>("usize offset", &Layer::AssetArchive::Offset);
				VAssetArchive->SetConstructor<Layer::AssetArchive>("void f()");

				auto VAssetFile = VM->SetClass<Layer::AssetFile>("asset_file", false);
				VAssetFile->SetConstructor<Layer::AssetFile, char*, size_t>("asset_file@ f(uptr@, usize)");
				VAssetFile->SetMethod("uptr@ get_buffer() const", &Layer::AssetFile::GetBuffer);
				VAssetFile->SetMethod("usize get_size() const", &Layer::AssetFile::Size);

				VM->BeginNamespace("content_series");
				VM->SetFunction<void(Core::Schema*, bool)>("void pack(schema@+, bool)", &Layer::Series::Pack);
				VM->SetFunction<void(Core::Schema*, int32_t)>("void pack(schema@+, int32)", &Layer::Series::Pack);
				VM->SetFunction<void(Core::Schema*, int64_t)>("void pack(schema@+, int64)", &Layer::Series::Pack);
				VM->SetFunction<void(Core::Schema*, uint32_t)>("void pack(schema@+, uint32)", &Layer::Series::Pack);
				VM->SetFunction<void(Core::Schema*, uint64_t)>("void pack(schema@+, uint64)", &Layer::Series::Pack);
				VM->SetFunction<void(Core::Schema*, float)>("void pack(schema@+, float)", &Layer::Series::Pack);
				VM->SetFunction<void(Core::Schema*, double)>("void pack(schema@+, double)", &Layer::Series::Pack);
				VM->SetFunction<void(Core::Schema*, const std::string_view&)>("void pack(schema@+, const string_view&in)", &Layer::Series::Pack);
				VM->SetFunction<bool(Core::Schema*, bool*)>("bool unpack(schema@+, bool &out)", &Layer::Series::Unpack);
				VM->SetFunction<bool(Core::Schema*, int32_t*)>("bool unpack(schema@+, int32 &out)", &Layer::Series::Unpack);
				VM->SetFunction<bool(Core::Schema*, int64_t*)>("bool unpack(schema@+, int64 &out)", &Layer::Series::Unpack);
				VM->SetFunction<bool(Core::Schema*, uint64_t*)>("bool unpack(schema@+, uint64 &out)", &Layer::Series::Unpack);
				VM->SetFunction<bool(Core::Schema*, uint32_t*)>("bool unpack(schema@+, uint32 &out)", &Layer::Series::Unpack);
				VM->SetFunction<bool(Core::Schema*, float*)>("bool unpack(schema@+, float &out)", &Layer::Series::Unpack);
				VM->SetFunction<bool(Core::Schema*, double*)>("bool unpack(schema@+, double &out)", &Layer::Series::Unpack);
				VM->SetFunction<bool(Core::Schema*, Core::String*)>("bool unpack(schema@+, string &out)", &Layer::Series::Unpack);
				VM->EndNamespace();

				auto VContentManager = VM->SetClass<Layer::ContentManager>("content_manager", true);
				auto VProcessor = VM->SetClass<Layer::Processor>("base_processor", false);
				PopulateProcessorBase<Layer::Processor>(*VProcessor);

				VContentManager->SetFunctionDef("void load_result_async(uptr@)");
				VContentManager->SetFunctionDef("void save_result_async(bool)");
				VContentManager->SetGcConstructor<Layer::ContentManager, ContentManager>("content_manager@ f()");
				VContentManager->SetMethod("void clear_cache()", &Layer::ContentManager::ClearCache);
				VContentManager->SetMethod("void clear_archives()", &Layer::ContentManager::ClearArchives);
				VContentManager->SetMethod("void clear_streams()", &Layer::ContentManager::ClearStreams);
				VContentManager->SetMethod("void clear_processors()", &Layer::ContentManager::ClearProcessors);
				VContentManager->SetMethod("void clear_path(const string_view&in)", &Layer::ContentManager::ClearPath);
				VContentManager->SetMethod("void set_environment(const string_view&in)", &Layer::ContentManager::SetEnvironment);
				VContentManager->SetMethodEx("uptr@ load(base_processor@+, const string_view&in, schema@+ = null)", &ContentManagerLoad);
				VContentManager->SetMethodEx("bool save(base_processor@+, const string_view&in, uptr@, schema@+ = null)", &ContentManagerSave);
				VContentManager->SetMethodEx("void load_deferred(base_processor@+, const string_view&in, load_result_async@)", &ContentManagerLoadDeferred1);
				VContentManager->SetMethodEx("void load_deferred(base_processor@+, const string_view&in, schema@+, load_result_async@)", &ContentManagerLoadDeferred2);
				VContentManager->SetMethodEx("void save_deferred(base_processor@+, const string_view&in, uptr@, save_result_async@)", &ContentManagerSaveDeferred1);
				VContentManager->SetMethodEx("void save_deferred(base_processor@+, const string_view&in, uptr@, schema@+, save_result_async@)", &ContentManagerSaveDeferred2);
				VContentManager->SetMethodEx("bool find_cache_info(base_processor@+, asset_cache &out, const string_view&in)", &ContentManagerFindCache1);
				VContentManager->SetMethodEx("bool find_cache_info(base_processor@+, asset_cache &out, uptr@)", &ContentManagerFindCache2);
				VContentManager->SetMethod<Layer::ContentManager, Layer::AssetCache*, Layer::Processor*, const std::string_view&>("uptr@ find_cache(base_processor@+, const string_view&in)", &Layer::ContentManager::FindCache);
				VContentManager->SetMethod<Layer::ContentManager, Layer::AssetCache*, Layer::Processor*, void*>("uptr@ find_cache(base_processor@+, uptr@)", &Layer::ContentManager::FindCache);
				VContentManager->SetMethodEx("base_processor@+ add_processor(base_processor@+, uint64)", &ContentManagerAddProcessor);
				VContentManager->SetMethod<Layer::ContentManager, Layer::Processor*, uint64_t>("base_processor@+ get_processor(uint64)", &Layer::ContentManager::GetProcessor);
				VContentManager->SetMethod<Layer::ContentManager, bool, uint64_t>("bool remove_processor(uint64)", &Layer::ContentManager::RemoveProcessor);
				VContentManager->SetMethodEx("bool import_archive(const string_view&in, bool)", &VI_EXPECTIFY_VOID(Layer::ContentManager::ImportArchive));
				VContentManager->SetMethodEx("bool export_archive(const string_view&in, const string_view&in, const string_view&in = string_view())", &VI_EXPECTIFY_VOID(Layer::ContentManager::ExportArchive));
				VContentManager->SetMethod("uptr@ try_to_cache(base_processor@+, const string_view&in, uptr@)", &Layer::ContentManager::TryToCache);
				VContentManager->SetMethod("bool is_busy() const", &Layer::ContentManager::IsBusy);
				VContentManager->SetMethod("const string& get_environment() const", &Layer::ContentManager::GetEnvironment);
				VContentManager->SetEnumRefsEx<Layer::ContentManager>([](Layer::ContentManager* Base, asIScriptEngine* VM)
				{
					for (auto& Item : Base->GetProcessors())
						FunctionFactory::GCEnumCallback(VM, Item.second);
				});
				VContentManager->SetReleaseRefsEx<Layer::ContentManager>([](Layer::ContentManager* Base, asIScriptEngine*)
				{
					Base->ClearProcessors();
				});

				auto VAppData = VM->SetClass<Layer::AppData>("app_data", true);
				VAppData->SetGcConstructor<Layer::AppData, AppData, Layer::ContentManager*, const std::string_view&>("app_data@ f(content_manager@+, const string_view&in)");
				VAppData->SetMethod("void migrate(const string_view&in)", &Layer::AppData::Migrate);
				VAppData->SetMethod("void set_key(const string_view&in, schema@+)", &Layer::AppData::SetKey);
				VAppData->SetMethod("void set_text(const string_view&in, const string_view&in)", &Layer::AppData::SetText);
				VAppData->SetMethod("schema@ get_key(const string_view&in)", &Layer::AppData::GetKey);
				VAppData->SetMethod("string get_text(const string_view&in)", &Layer::AppData::GetText);
				VAppData->SetMethod("bool has(const string_view&in)", &Layer::AppData::Has);
				VAppData->SetEnumRefsEx<Layer::AppData>([](Layer::AppData* Base, asIScriptEngine* VM) { });
				VAppData->SetReleaseRefsEx<Layer::AppData>([](Layer::AppData* Base, asIScriptEngine*) { });

				auto VApplication = VM->SetClass<Application>("application", true);
				auto VApplicationFrameInfo = VM->SetStructTrivial<Application::Desc::FramesInfo>("application_frame_info");
				VApplicationFrameInfo->SetProperty<Application::Desc::FramesInfo>("float stable", &Application::Desc::FramesInfo::Stable);
				VApplicationFrameInfo->SetProperty<Application::Desc::FramesInfo>("float limit", &Application::Desc::FramesInfo::Limit);
				VApplicationFrameInfo->SetConstructor<Application::Desc::FramesInfo>("void f()");

				auto VApplicationDesc = VM->SetStructTrivial<Application::Desc>("application_desc");
				VApplicationDesc->SetProperty<Application::Desc>("application_frame_info refreshrate", &Application::Desc::Refreshrate);
				VApplicationDesc->SetProperty<Application::Desc>("schedule_policy scheduler", &Application::Desc::Scheduler);
				VApplicationDesc->SetProperty<Application::Desc>("string preferences", &Application::Desc::Preferences);
				VApplicationDesc->SetProperty<Application::Desc>("string environment", &Application::Desc::Environment);
				VApplicationDesc->SetProperty<Application::Desc>("string directory", &Application::Desc::Directory);
				VApplicationDesc->SetProperty<Application::Desc>("usize polling_timeout", &Application::Desc::PollingTimeout);
				VApplicationDesc->SetProperty<Application::Desc>("usize polling_events", &Application::Desc::PollingEvents);
				VApplicationDesc->SetProperty<Application::Desc>("usize threads", &Application::Desc::Threads);
				VApplicationDesc->SetProperty<Application::Desc>("usize usage", &Application::Desc::Usage);
				VApplicationDesc->SetProperty<Application::Desc>("bool daemon", &Application::Desc::Daemon);
				VApplicationDesc->SetConstructor<Application::Desc>("void f()");

				VApplication->SetFunctionDef("void dispatch_sync(clock_timer@+)");
				VApplication->SetFunctionDef("void publish_sync(clock_timer@+)");
				VApplication->SetFunctionDef("void composition_sync()");
				VApplication->SetFunctionDef("void script_hook_sync()");
				VApplication->SetFunctionDef("void initialize_sync()");
				VApplication->SetFunctionDef("void startup_sync()");
				VApplication->SetFunctionDef("void shutdown_sync()");
				VApplication->SetProperty<Layer::Application>("content_manager@ content", &Layer::Application::Content);
				VApplication->SetProperty<Layer::Application>("app_data@ database", &Layer::Application::Database);
				VApplication->SetProperty<Layer::Application>("application_desc control", &Layer::Application::Control);
				VApplication->SetGcConstructor<Application, ApplicationName, Application::Desc&, void*, int>("application@ f(application_desc &in, ?&in)");
				VApplication->SetMethod("void set_on_dispatch(dispatch_sync@)", &Application::SetOnDispatch);
				VApplication->SetMethod("void set_on_publish(publish_sync@)", &Application::SetOnPublish);
				VApplication->SetMethod("void set_on_composition(composition_sync@)", &Application::SetOnComposition);
				VApplication->SetMethod("void set_on_script_hook(script_hook_sync@)", &Application::SetOnScriptHook);
				VApplication->SetMethod("void set_on_initialize(initialize_sync@)", &Application::SetOnInitialize);
				VApplication->SetMethod("void set_on_startup(startup_sync@)", &Application::SetOnStartup);
				VApplication->SetMethod("void set_on_shutdown(shutdown_sync@)", &Application::SetOnShutdown);
				VApplication->SetMethod("int start()", &Layer::Application::Start);
				VApplication->SetMethod("int restart()", &Layer::Application::Restart);
				VApplication->SetMethod("void stop(int = 0)", &Layer::Application::Stop);
				VApplication->SetMethod("application_state get_state() const", &Layer::Application::GetState);
				VApplication->SetMethod("uptr@ get_initiator() const", &Application::GetInitiatorObject);
				VApplication->SetMethod("usize get_processed_events() const", &Application::GetProcessedEvents);
				VApplication->SetMethod("bool has_processed_events() const", &Application::HasProcessedEvents);
				VApplication->SetMethod("bool retrieve_initiator(?&out) const", &Application::RetrieveInitiatorObject);
				VApplication->SetMethodStatic("application@+ get()", &Layer::Application::Get);
				VApplication->SetMethodStatic("bool wants_restart(int)", &Application::WantsRestart);
				VApplication->SetEnumRefsEx<Application>([](Application* Base, asIScriptEngine* VM)
				{
					FunctionFactory::GCEnumCallback(VM, Base->VM);
					FunctionFactory::GCEnumCallback(VM, Base->Content);
					FunctionFactory::GCEnumCallback(VM, Base->Database);
					FunctionFactory::GCEnumCallback(VM, Base->GetInitiatorObject());
					FunctionFactory::GCEnumCallback(VM, &Base->OnDispatch);
					FunctionFactory::GCEnumCallback(VM, &Base->OnPublish);
					FunctionFactory::GCEnumCallback(VM, &Base->OnComposition);
					FunctionFactory::GCEnumCallback(VM, &Base->OnScriptHook);
					FunctionFactory::GCEnumCallback(VM, &Base->OnInitialize);
					FunctionFactory::GCEnumCallback(VM, &Base->OnStartup);
					FunctionFactory::GCEnumCallback(VM, &Base->OnShutdown);
				});
				VApplication->SetReleaseRefsEx<Application>([](Application* Base, asIScriptEngine*)
				{
					Base->~Application();
				});

				return true;
#else
				VI_ASSERT(false, "<layer> is not loaded");
				return false;
#endif
			}
		}
	}
}