#ifndef VI_BINDINGS_H
#define VI_BINDINGS_H
#include "scripting.h"
#include "layer.h"
#include <tuple>
#define VI_TYPEREF(Name, TypeName) static const uint64_t Name = Vitex::Core::OS::File::GetIndex<sizeof(TypeName)>(TypeName); Vitex::Scripting::TypeCache::Set(Name, TypeName)
#define VI_PROMISIFY(MemberFunction, TypeId) Vitex::Scripting::Bindings::Promise::Ify<decltype(&MemberFunction), &MemberFunction>::Id<TypeId>
#define VI_PROMISIFY_REF(MemberFunction, TypeRef) Vitex::Scripting::Bindings::Promise::Ify<decltype(&MemberFunction), &MemberFunction>::Decl<TypeRef>
#define VI_SPROMISIFY(Function, TypeId) Vitex::Scripting::Bindings::Promise::IfyStatic<decltype(&Function), &Function>::Id<TypeId>
#define VI_SPROMISIFY_REF(Function, TypeRef) Vitex::Scripting::Bindings::Promise::IfyStatic<decltype(&Function), &Function>::Decl<TypeRef>
#define VI_ARRAYIFY(MemberFunction, TypeId) Vitex::Scripting::Bindings::Array::Ify<decltype(&MemberFunction), &MemberFunction>::Id<TypeId>
#define VI_ARRAYIFY_REF(MemberFunction, TypeRef) Vitex::Scripting::Bindings::Array::Ify<decltype(&MemberFunction), &MemberFunction>::Decl<TypeRef>
#define VI_SARRAYIFY(Function, TypeId) Vitex::Scripting::Bindings::Array::IfyStatic<decltype(&Function), &Function>::Id<TypeId>
#define VI_SARRAYIFY_REF(Function, TypeRef) Vitex::Scripting::Bindings::Array::IfyStatic<decltype(&Function), &Function>::Decl<TypeRef>
#define VI_ANYIFY(MemberFunction, TypeId) Vitex::Scripting::Bindings::Any::Ify<decltype(&MemberFunction), &MemberFunction>::Id<TypeId>
#define VI_ANYIFY_REF(MemberFunction, TypeRef) Vitex::Scripting::Bindings::Any::Ify<decltype(&MemberFunction), &MemberFunction>::Decl<TypeRef>
#define VI_SANYIFY(Function, TypeId) Vitex::Scripting::Bindings::Any::IfyStatic<decltype(&Function), &Function>::Id<TypeId>
#define VI_SANYIFY_REF(Function, TypeRef) Vitex::Scripting::Bindings::Any::IfyStatic<decltype(&Function), &Function>::Decl<TypeRef>
#define VI_EXPECTIFY(MemberFunction) Vitex::Scripting::Bindings::ExpectsWrapper::Ify<decltype(&MemberFunction), &MemberFunction>::Throws
#define VI_EXPECTIFY_VOID(MemberFunction) Vitex::Scripting::Bindings::ExpectsWrapper::IfyVoid<decltype(&MemberFunction), &MemberFunction>::Throws
#define VI_SEXPECTIFY(Function) Vitex::Scripting::Bindings::ExpectsWrapper::IfyStatic<decltype(&Function), &Function>::Throws
#define VI_SEXPECTIFY_VOID(Function) Vitex::Scripting::Bindings::ExpectsWrapper::IfyStaticVoid<decltype(&Function), &Function>::Throws
#define VI_OPTIONIFY(MemberFunction) Vitex::Scripting::Bindings::OptionWrapper::Ify<decltype(&MemberFunction), &MemberFunction>::Throws
#define VI_OPTIONIFY_VOID(MemberFunction) Vitex::Scripting::Bindings::OptionWrapper::IfyVoid<decltype(&MemberFunction), &MemberFunction>::Throws
#define VI_SOPTIONIFY(Function) Vitex::Scripting::Bindings::OptionWrapper::IfyStatic<decltype(&Function), &Function>::Throws
#define VI_SOPTIONIFY_VOID(Function) Vitex::Scripting::Bindings::OptionWrapper::IfyStaticVoid<decltype(&Function), &Function>::Throws
#ifdef __LP64__
typedef unsigned int as_uint32_t;
typedef unsigned long as_uint64_t;
typedef long as_int64_t;
#else
typedef unsigned long as_uint32_t;
#if !defined(_MSC_VER) && (defined(__GNUC__) || defined(__MWERKS__) || defined(__SUNPRO_CC) || defined(__psp2__))
typedef uint64_t as_uint64_t;
typedef int64_t as_int64_t;
#else
typedef unsigned __int64 as_uint64_t;
typedef __int64 as_int64_t;
#endif
#endif
typedef unsigned int as_size_t;

namespace Vitex
{
	namespace Scripting
	{
		namespace Bindings
		{
			class Promise;

			class Array;

			struct VI_OUT Dynamic
			{
				union
				{
					as_int64_t Integer;
					double Number;
					void* Object;
				};

				int TypeId;

				Dynamic()
				{
					Clean();
				}
				Dynamic(int NewTypeId)
				{
					Clean();
					TypeId = NewTypeId;
				}
				void Clean()
				{
					memset((void*)this, 0, sizeof(*this));
				}
			};

			class VI_OUT_TS Utils
			{
			public:
				template <typename F, typename Tuple, std::size_t... I>
				static constexpr decltype(auto) ApplyPack(F&& f, Tuple&& t, std::index_sequence<I...>)
				{
					return static_cast<F&&>(f)(std::get<I>(static_cast<Tuple&&>(t))...);
				}
				template <typename F, typename Tuple>
				static constexpr decltype(auto) Apply(F&& f, Tuple&& t)
				{
					return Utils::ApplyPack(static_cast<F&&>(f), static_cast<Tuple&&>(t), std::make_index_sequence<std::tuple_size<std::remove_reference_t<Tuple>>::value>{});
				}
				template <typename Lambda, typename... Args>
				static auto CaptureCall(Lambda&& lambda, Args&&... args)
				{
					return [lambda = std::forward<Lambda>(lambda), capture_args = std::forward_as_tuple(args...)](auto&&... original_args) mutable
					{
						return Utils::Apply([&lambda](auto&&... args)
						{
							lambda(std::forward<decltype(args)>(args)...);
						}, std::tuple_cat(std::forward_as_tuple(original_args...), Utils::Apply([](auto&&... args)
						{
							return std::forward_as_tuple(std::move(args)...);
						}, std::move(capture_args))));
					};
				}
			};

			class VI_OUT_TS Registry final : public Core::Singletonish
			{
			public:
				static bool ImportCTypes(VirtualMachine* VM);
				static bool ImportAny(VirtualMachine* VM);
				static bool ImportArray(VirtualMachine* VM);
				static bool ImportComplex(VirtualMachine* VM);
				static bool ImportDictionary(VirtualMachine* VM);
				static bool ImportMath(VirtualMachine* VM);
				static bool ImportString(VirtualMachine* VM);
				static bool ImportException(VirtualMachine* VM);
				static bool ImportMutex(VirtualMachine* VM);
				static bool ImportThread(VirtualMachine* VM);
				static bool ImportBuffers(VirtualMachine* Engine);
				static bool ImportRandom(VirtualMachine* VM);
				static bool ImportPromise(VirtualMachine* VM);
				static bool ImportDecimal(VirtualMachine* Engine);
				static bool ImportUInt128(VirtualMachine* Engine);
				static bool ImportUInt256(VirtualMachine* Engine);
				static bool ImportVariant(VirtualMachine* Engine);
				static bool ImportTimestamp(VirtualMachine* Engine);
				static bool ImportConsole(VirtualMachine* Engine);
				static bool ImportSchema(VirtualMachine* Engine);
				static bool ImportClockTimer(VirtualMachine* Engine);
				static bool ImportFileSystem(VirtualMachine* Engine);
				static bool ImportOS(VirtualMachine* Engine);
				static bool ImportSchedule(VirtualMachine* Engine);
				static bool ImportRegex(VirtualMachine* Engine);
				static bool ImportCrypto(VirtualMachine* Engine);
				static bool ImportCodec(VirtualMachine* Engine);
				static bool ImportPreprocessor(VirtualMachine* Engine);
				static bool ImportNetwork(VirtualMachine* Engine);
				static bool ImportHTTP(VirtualMachine* Engine);
				static bool ImportSMTP(VirtualMachine* Engine);
				static bool ImportSQLite(VirtualMachine* Engine);
				static bool ImportPostgreSQL(VirtualMachine* Engine);
				static bool ImportMongoDB(VirtualMachine* Engine);
				static bool ImportVM(VirtualMachine* Engine);
				static bool ImportLayer(VirtualMachine* Engine);
				static bool Cleanup();
			};

			class VI_OUT_TS Imports
			{
			public:
				static void BindSyntax(VirtualMachine* VM, bool Enabled, const std::string_view& Syntax);
				static ExpectsVM<void> GeneratorCallback(Compute::Preprocessor* Base, const std::string_view& Path, Core::String& Code, const std::string_view& Syntax);
			};

			class VI_OUT_TS Tags
			{
			public:
				enum class TagType
				{
					Unknown,
					Type,
					Function,
					PropertyFunction,
					Variable,
					NotType
				};

				struct TagDirective
				{
					Core::UnorderedMap<Core::String, Core::String> Args;
					Core::String Name;
				};

				struct TagDeclaration
				{
					std::string_view Class;
					std::string_view Name;
					Core::String Declaration;
					Core::String Namespace;
					Core::Vector<TagDirective> Directives;
					TagType Type = TagType::Unknown;
				};

			public:
				typedef Core::Vector<TagDeclaration> TagInfo;
				typedef std::function<void(VirtualMachine*, TagInfo&&)> TagCallback;

			public:
				static void BindSyntax(VirtualMachine* VM, bool Enabled, const TagCallback& Callback);
				static ExpectsVM<void> GeneratorCallback(Compute::Preprocessor* Base, const std::string_view& Path, Core::String& Code, VirtualMachine* VM, const TagCallback& Callback);

			private:
				static size_t ExtractField(VirtualMachine* VM, Core::String& Code, size_t Offset, TagDeclaration& Tag);
				static void ExtractDeclaration(VirtualMachine* VM, Core::String& Code, size_t Offset, TagDeclaration& Tag);
				static void AppendDirective(TagDeclaration& Tag, Core::String& Directive);
			};

			class VI_OUT_TS Exception
			{
			public:
				struct VI_OUT Pointer
				{
					Core::String Type;
					Core::String Text;
					Core::String Origin;
					ImmediateContext* Context;

					Pointer();
					Pointer(ImmediateContext* Context);
					Pointer(const std::string_view& Data);
					Pointer(const std::string_view& Type, const std::string_view& Text);
					void LoadExceptionData(const std::string_view& Data);
					const Core::String& GetType() const;
					const Core::String& GetText() const;
					Core::String ToExceptionString() const;
					Core::String What() const;
					Core::String LoadStackHere() const;
					bool Empty() const;
				};

			public:
				static void ThrowAt(ImmediateContext* Context, const Pointer& Data);
				static void Throw(const Pointer& Data);
				static void RethrowAt(ImmediateContext* Context);
				static void Rethrow();
				static bool HasExceptionAt(ImmediateContext* Context);
				static bool HasException();
				static Pointer GetExceptionAt(ImmediateContext* Context);
				static Pointer GetException();
				static ExpectsVM<void> GeneratorCallback(Compute::Preprocessor* Base, const std::string_view& Path, Core::String& Code);
			};

			class VI_OUT_TS ExpectsWrapper
			{
			public:
				static Exception::Pointer TranslateThrow(const std::exception& Error);
				static Exception::Pointer TranslateThrow(const std::error_code& Error);
				static Exception::Pointer TranslateThrow(const std::string_view& Error);
				static Exception::Pointer TranslateThrow(const std::error_condition& Error);
				static Exception::Pointer TranslateThrow(const Core::BasicException& Error);
				static Exception::Pointer TranslateThrow(const Core::String& Error);

			public:
				template <typename V, typename E>
				static V&& Unwrap(Core::Expects<V, E>&& Subresult, V&& Default, ImmediateContext* Context = ImmediateContext::Get())
				{
					if (Subresult)
						return std::move(*Subresult);

					Exception::ThrowAt(Context, TranslateThrow(Subresult.Error()));
					return std::move(Default);
				}
				template <typename V, typename E>
				static bool UnwrapVoid(Core::Expects<V, E>&& Subresult, ImmediateContext* Context = ImmediateContext::Get())
				{
					if (!Subresult)
						Exception::ThrowAt(Context, TranslateThrow(Subresult.Error()));
					return !!Subresult;
				}

			public:
				template <typename T, T>
				struct Ify;

				template <typename T, T>
				struct IfyVoid;

				template <typename T, T>
				struct IfyStatic;

				template <typename T, T>
				struct IfyStaticVoid;

				template <typename T, typename V, typename E, typename ...Args, Core::Expects<V, E>(T::* F)(Args...)>
				struct Ify<Core::Expects<V, E>(T::*)(Args...), F>
				{
					static V Throws(T* Base, Args... Data)
					{
						Core::Expects<V, E> Subresult((Base->*F)(Data...));
						if (Subresult)
							return *Subresult;
						
						Exception::Throw(TranslateThrow(Subresult.Error()));
						throw false;
					}
				};

				template <typename T, typename V, typename E, typename ...Args, Core::Expects<V, E>(T::* F)(Args...)>
				struct IfyVoid<Core::Expects<V, E>(T::*)(Args...), F>
				{
					static bool Throws(T* Base, Args... Data)
					{
						Core::Expects<V, E> Subresult((Base->*F)(Data...));
						if (!Subresult)
							Exception::Throw(TranslateThrow(Subresult.Error()));
						return !!Subresult;
					}
				};

				template <typename V, typename E, typename ...Args, Core::Expects<V, E>(*F)(Args...)>
				struct IfyStatic<Core::Expects<V, E>(*)(Args...), F>
				{
					static V Throws(Args... Data)
					{
						Core::Expects<V, E> Subresult((*F)(Data...));
						if (Subresult)
							return *Subresult;

						Exception::Throw(TranslateThrow(Subresult.Error()));
						throw false;
					}
				};

				template <typename V, typename E, typename ...Args, Core::Expects<V, E>(*F)(Args...)>
				struct IfyStaticVoid<Core::Expects<V, E>(*)(Args...), F>
				{
					static bool Throws(Args... Data)
					{
						Core::Expects<V, E> Subresult((*F)(Data...));
						if (!Subresult)
							Exception::Throw(TranslateThrow(Subresult.Error()));
						return !!Subresult;
					}
				};
			};

			class VI_OUT_TS OptionWrapper
			{
			public:
				static Exception::Pointer TranslateThrow();

			public:
				template <typename V>
				static V&& Unwrap(Core::Option<V>&& Subresult, V&& Default, ImmediateContext* Context = ImmediateContext::Get())
				{
					if (Subresult)
						return std::move(*Subresult);

					Exception::ThrowAt(Context, TranslateThrow());
					return std::move(Default);
				}
				template <typename V>
				static bool UnwrapVoid(Core::Option<V>&& Subresult, ImmediateContext* Context = ImmediateContext::Get())
				{
					if (!Subresult)
						Exception::ThrowAt(Context, TranslateThrow());
					return !!Subresult;
				}

			public:
				template <typename T, T>
				struct Ify;

				template <typename T, T>
				struct IfyVoid;

				template <typename T, T>
				struct IfyStatic;

				template <typename T, T>
				struct IfyStaticVoid;

				template <typename T, typename V, typename ...Args, Core::Option<V>(T::* F)(Args...)>
				struct Ify<Core::Option<V>(T::*)(Args...), F>
				{
					static V Throws(T* Base, Args... Data)
					{
						Core::Option<V> Subresult((Base->*F)(Data...));
						if (Subresult)
							return *Subresult;

						Exception::Throw(TranslateThrow());
						throw false;
					}
				};

				template <typename T, typename V, typename ...Args, Core::Option<V>(T::* F)(Args...)>
				struct IfyVoid<Core::Option<V>(T::*)(Args...), F>
				{
					static bool Throws(T* Base, Args... Data)
					{
						Core::Option<V> Subresult((Base->*F)(Data...));
						if (!Subresult)
							Exception::Throw(TranslateThrow());
						return !!Subresult;
					}
				};

				template <typename V, typename ...Args, Core::Option<V>(*F)(Args...)>
				struct IfyStatic<Core::Option<V>(*)(Args...), F>
				{
					static V Throws(Args... Data)
					{
						Core::Option<V> Subresult((*F)(Data...));
						if (Subresult)
							return *Subresult;

						Exception::Throw(TranslateThrow());
						throw false;
					}
				};

				template <typename V, typename ...Args, Core::Option<V>(*F)(Args...)>
				struct IfyStaticVoid<Core::Option<V>(*)(Args...), F>
				{
					static bool Throws(Args... Data)
					{
						Core::Option<V> Subresult((*F)(Data...));
						if (!Subresult)
							Exception::Throw(TranslateThrow());
						return !!Subresult;
					}
				};
			};

			class VI_OUT String
			{
			public:
				static std::string_view ImplCastStringView(Core::String& Base);
				static void Create(Core::String* Base);
				static void CreateCopy1(Core::String* Base, const Core::String& Other);
				static void CreateCopy2(Core::String* Base, const std::string_view& Other);
				static void Destroy(Core::String* Base);
				static void PopBack(Core::String& Base);
				static Core::String& Replace1(Core::String& Other, const Core::String& From, const Core::String& To, size_t Start);
				static Core::String& ReplacePart1(Core::String& Other, size_t Start, size_t End, const Core::String& To);
				static bool StartsWith1(const Core::String& Other, const Core::String& Value, size_t Offset);
				static bool EndsWith1(const Core::String& Other, const Core::String& Value);
				static Core::String& Replace2(Core::String& Other, const std::string_view& From, const std::string_view& To, size_t Start);
				static Core::String& ReplacePart2(Core::String& Other, size_t Start, size_t End, const std::string_view& To);
				static bool StartsWith2(const Core::String& Other, const std::string_view& Value, size_t Offset);
				static bool EndsWith2(const Core::String& Other, const std::string_view& Value);
				static Core::String Substring1(Core::String& Base, size_t Offset);
				static Core::String Substring2(Core::String& Base, size_t Offset, size_t Size);
				static Core::String FromPointer(void* Pointer);
				static Core::String FromBuffer(const char* Buffer, size_t MaxSize);
				static char* Index(Core::String& Base, size_t Offset);
				static char* Front(Core::String& Base);
				static char* Back(Core::String& Base);
				static Array* Split(Core::String& Base, const std::string_view& Delimiter);

			public:
				template <typename T>
				static T FromString(const Core::String& Text, int Base = 10)
				{
					auto Value = Core::FromString<T>(Text, Base);
					return Value ? *Value : (T)0;
				}
			};

			class VI_OUT StringView
			{
			public:
				static Core::String ImplCastString(std::string_view& Base);
				static void Create(std::string_view* Base);
				static void CreateCopy(std::string_view* Base, Core::String& Other);
				static void Assign(std::string_view* Base, Core::String& Other);
				static void Destroy(std::string_view* Base);
				static bool StartsWith(const std::string_view& Other, const std::string_view& Value, size_t Offset);
				static bool EndsWith(const std::string_view& Other, const std::string_view& Value);
				static int Compare1(std::string_view& Base, const Core::String& Other);
				static int Compare2(std::string_view& Base, const std::string_view& Other);
				static Core::String Append1(const std::string_view& Base, const std::string_view& Other);
				static Core::String Append2(const std::string_view& Base, const Core::String& Other);
				static Core::String Append3(const Core::String& Other, const std::string_view& Base);
				static Core::String Append4(const std::string_view& Base, char Other);
				static Core::String Append5(char Other, const std::string_view& Base);
				static Core::String Substring1(std::string_view& Base, size_t Offset);
				static Core::String Substring2(std::string_view& Base, size_t Offset, size_t Size);
				static size_t ReverseFind1(std::string_view& Base, const std::string_view& Other, size_t Offset);
				static size_t ReverseFind2(std::string_view& Base, char Other, size_t Offset);
				static size_t Find1(std::string_view& Base, const std::string_view& Other, size_t Offset);
				static size_t Find2(std::string_view& Base, char Other, size_t Offset);
				static size_t FindFirstOf(std::string_view& Base, const std::string_view& Other, size_t Offset);
				static size_t FindFirstNotOf(std::string_view& Base, const std::string_view& Other, size_t Offset);
				static size_t FindLastOf(std::string_view& Base, const std::string_view& Other, size_t Offset);
				static size_t FindLastNotOf(std::string_view& Base, const std::string_view& Other, size_t Offset);
				static std::string_view FromBuffer(const char* Buffer, size_t MaxSize);
				static char* Index(std::string_view& Base, size_t Offset);
				static char* Front(std::string_view& Base);
				static char* Back(std::string_view& Base);
				static Array* Split(std::string_view& Base, const std::string_view& Delimiter);

			public:
				template <typename T>
				static T FromString(const std::string_view& Text, int Base = 10)
				{
					auto Value = Core::FromString<T>(Text, Base);
					return Value ? *Value : (T)0;
				}
			};

			class VI_OUT Math
			{
			public:
				static float FpFromIEEE(uint32_t raw);
				static uint32_t FpToIEEE(float fp);
				static double FpFromIEEE(as_uint64_t raw);
				static as_uint64_t FpToIEEE(double fp);
				static bool CloseTo(float a, float b, float epsilon);
				static bool CloseTo(double a, double b, double epsilon);
			};

			class VI_OUT Storable
			{
			private:
				friend class Dictionary;

			private:
				Dynamic Value;

			public:
				Storable() noexcept;
				Storable(VirtualMachine* Engine, void* Pointer, int TypeId) noexcept;
				~Storable() noexcept;
				void Set(VirtualMachine* Engine, void* Pointer, int TypeId);
				void Set(VirtualMachine* Engine, Storable& Other);
				bool Get(VirtualMachine* Engine, void* Pointer, int TypeId) const;
				const void* GetAddressOfValue() const;
				int GetTypeId() const;
				void ReleaseReferences(asIScriptEngine* Engine);
				void EnumReferences(asIScriptEngine* Engine);
			};

			class VI_OUT Random
			{
			public:
				static Core::String Getb(uint64_t Size);
				static double Betweend(double Min, double Max);
				static double Magd();
				static double Getd();
				static float Betweenf(float Min, float Max);
				static float Magf();
				static float Getf();
				static uint64_t Betweeni(uint64_t Min, uint64_t Max);
			};

			class VI_OUT Any : public Core::Reference<Any>
			{
				friend Promise;

			private:
				VirtualMachine* Engine;
				Dynamic Value;

			public:
				Any(VirtualMachine* Engine) noexcept;
				Any(void* Ref, int RefTypeId, VirtualMachine* Engine) noexcept;
				Any(const Any&) noexcept;
				~Any() noexcept;
				Any& operator= (const Any&) noexcept;
				int CopyFrom(const Any* Other);
				void Store(void* Ref, int RefTypeId);
				bool Retrieve(void* Ref, int RefTypeId) const;
				void* GetAddressOfObject();
				int GetTypeId() const;
				void EnumReferences(asIScriptEngine* Engine);
				void ReleaseReferences(asIScriptEngine* Engine);

			private:
				void FreeObject();

			public:
				static Core::Unique<Any> Create();
				static Core::Unique<Any> Create(int TypeId, void* Ref);
				static Core::Unique<Any> Create(const char* Decl, void* Ref);
				static Core::Unique<Any> Factory1();
				static void Factory2(asIScriptGeneric* Generic);
				static Any& Assignment(Any* Base, Any* Other);

			public:
				template <typename T, T>
				struct Ify;

				template <typename T, T>
				struct IfyStatic;

				template <typename T, typename R, typename ...Args, R(T::* F)(Args...)>
				struct Ify<R(T::*)(Args...), F>
				{
					template <TypeId TypeId>
					static Any* Id(T* Base, Args... Data)
					{
						R Subresult((Base->*F)(Data...));
						return Any::Create((int)TypeId, &Subresult);
					}
					template <uint64_t TypeRef>
					static Any* Decl(T* Base, Args... Data)
					{
						R Subresult((Base->*F)(Data...));
						return Any::Create(TypeCache::GetTypeId(TypeRef), &Subresult);
					}
				};

				template <typename R, typename ...Args, R(*F)(Args...)>
				struct IfyStatic<R(*)(Args...), F>
				{
					template <TypeId TypeId>
					static Any* Id(Args... Data)
					{
						R Subresult((*F)(Data...));
						return Any::Create((int)TypeId, &Subresult);
					}
					template <uint64_t TypeRef>
					static Any* Decl(Args... Data)
					{
						R Subresult((*F)(Data...));
						return Any::Create(TypeCache::GetTypeId(TypeRef), &Subresult);
					}
				};
			};

			class VI_OUT Array : public Core::Reference<Array>
			{
			public:
				struct SBuffer
				{
					size_t MaxElements;
					size_t NumElements;
					unsigned char Data[1];
				};

				struct SCache
				{
					asIScriptFunction* Comparator;
					asIScriptFunction* Equals;
					int ComparatorReturnCode;
					int EqualsReturnCode;
				};

			private:
				static int ArrayId;

			private:
				TypeInfo ObjType;
				SBuffer* Buffer;
				size_t ElementSize;
				int SubTypeId;

			public:
				Array(asITypeInfo* T, void* InitBuf) noexcept;
				Array(size_t Length, asITypeInfo* T) noexcept;
				Array(size_t Length, void* DefVal, asITypeInfo* T) noexcept;
				Array(const Array& Other) noexcept;
				~Array() noexcept;
				asITypeInfo* GetArrayObjectType() const;
				int GetArrayTypeId() const;
				int GetElementTypeId() const;
				size_t Size() const;
				size_t Capacity() const;
				bool Empty() const;
				void Reserve(size_t MaxElements);
				void Resize(size_t NumElements);
				void* Front();
				const void* Front() const;
				void* Back();
				const void* Back() const;
				void* At(size_t Index);
				const void* At(size_t Index) const;
				void SetValue(size_t Index, void* Value);
				Array& operator= (const Array&) noexcept;
				bool operator== (const Array&) const;
				void InsertAt(size_t Index, void* Value);
				void InsertAt(size_t Index, const Array& Other);
				void InsertLast(void* Value);
				void RemoveAt(size_t Index);
				void RemoveLast();
				void RemoveRange(size_t start, size_t Count);
				void Swap(size_t Index1, size_t Index2);
				void Sort(asIScriptFunction* Callback);
				void Reverse();
				void Clear();
				size_t Find(void* Value, size_t StartAt) const;
				size_t FindByRef(void* Value, size_t StartAt) const;
				void* GetBuffer();
				void EnumReferences(asIScriptEngine* Engine);
				void ReleaseReferences(asIScriptEngine* Engine);

			private:
				void* GetArrayItemPointer(size_t Index);
				void* GetDataPointer(void* Buffer);
				void Copy(void* Dst, void* Src);
				void Precache();
				bool CheckMaxSize(size_t NumElements);
				void Resize(int64_t Delta, size_t At);
				void CreateBuffer(SBuffer** Buf, size_t NumElements);
				void DeleteBuffer(SBuffer* Buf);
				void CopyBuffer(SBuffer* Dst, SBuffer* Src);
				void Create(SBuffer* Buf, size_t Start, size_t End);
				void Destroy(SBuffer* Buf, size_t Start, size_t End);
				bool Less(const void* A, const void* B, ImmediateContext* Ctx, SCache* Cache);
				bool Equals(const void* A, const void* B, ImmediateContext* Ctx, SCache* Cache) const;
				bool IsEligibleForFind(SCache** Output) const;
				bool IsEligibleForSort(SCache** Output) const;

			public:
				static Core::Unique<Array> Create(asITypeInfo* T);
				static Core::Unique<Array> Create(asITypeInfo* T, size_t Length);
				static Core::Unique<Array> Create(asITypeInfo* T, size_t Length, void* DefaultValue);
				static Core::Unique<Array> Factory(asITypeInfo* T, void* ListBuffer);
				static void CleanupTypeInfoCache(asITypeInfo* Type);
				static bool TemplateCallback(asITypeInfo* T, bool& DontGarbageCollect);
				static int GetId();

			public:
				template <typename T>
				static Array* Compose(const TypeInfo& ArrayType, const Core::Vector<T>& Objects)
				{
					Array* Array = Create(ArrayType.GetTypeInfo(), Objects.size());
					for (size_t i = 0; i < Objects.size(); i++)
						Array->SetValue((size_t)i, (void*)&Objects[i]);

					return Array;
				}
				template <typename T>
				static typename std::enable_if<std::is_pointer<T>::value, Core::Vector<T>>::type Decompose(Array* Array)
				{
					Core::Vector<T> Result;
					if (!Array)
						return Result;

					size_t Size = Array->Size();
					Result.reserve(Size);

					for (size_t i = 0; i < Size; i++)
						Result.push_back((T)Array->At(i));

					return Result;
				}
				template <typename T>
				static typename std::enable_if<!std::is_pointer<T>::value, Core::Vector<T>>::type Decompose(Array* Array)
				{
					Core::Vector<T> Result;
					if (!Array)
						return Result;

					size_t Size = Array->Size();
					Result.reserve(Size);

					for (size_t i = 0; i < Size; i++)
						Result.push_back(*((T*)Array->At(i)));

					return Result;
				}

			public:
				template <typename T, T>
				struct Ify;

				template <typename T, T>
				struct IfyStatic;

				template <typename T, typename R, typename ...Args, Core::Vector<R>(T::* F)(Args...)>
				struct Ify<Core::Vector<R>(T::*)(Args...), F>
				{
					template <TypeId TypeId>
					static Array* Id(T* Base, Args... Data)
					{
						VirtualMachine* VM = VirtualMachine::Get();
						VI_ASSERT(VM != nullptr, "manager should be present");

						auto Info = VM->GetTypeInfoById((int)TypeId);
						VI_ASSERT(Info.IsValid(), "typeinfo should be valid");

						Core::Vector<R> Source((Base->*F)(Data...));
						return Array::Compose(Info, Source);
					}
					template <uint64_t TypeRef>
					static Array* Decl(T* Base, Args... Data)
					{
						VirtualMachine* VM = VirtualMachine::Get();
						VI_ASSERT(VM != nullptr, "manager should be present");

						auto Info = VM->GetTypeInfoById(TypeCache::GetTypeId(TypeRef));
						VI_ASSERT(Info.IsValid(), "typeinfo should be valid");

						Core::Vector<R> Source((Base->*F)(Data...));
						return Array::Compose(Info, Source);
					}
				};

				template <typename R, typename ...Args, Core::Vector<R>(*F)(Args...)>
				struct IfyStatic<Core::Vector<R>(*)(Args...), F>
				{
					template <TypeId TypeId>
					static Array* Id(Args... Data)
					{
						VirtualMachine* VM = VirtualMachine::Get();
						VI_ASSERT(VM != nullptr, "manager should be present");

						auto Info = VM->GetTypeInfoById((int)TypeId);
						VI_ASSERT(Info.IsValid(), "typeinfo should be valid");

						Core::Vector<R> Source((*F)(Data...));
						return Array::Compose(Info, Source);
					}
					template <uint64_t TypeRef>
					static Array* Decl(Args... Data)
					{
						VirtualMachine* VM = VirtualMachine::Get();
						VI_ASSERT(VM != nullptr, "manager should be present");

						auto Info = VM->GetTypeInfoById(TypeCache::GetTypeId(TypeRef));
						VI_ASSERT(Info.IsValid(), "typeinfo should be valid");

						Core::Vector<R> Source((*F)(Data...));
						return Array::Compose(Info, Source);
					}
				};
			};

			class VI_OUT Dictionary : public Core::Reference<Dictionary>
			{
			public:
				typedef Core::UnorderedMap<Core::String, Storable> InternalMap;

			public:
				class LocalIterator
				{
				private:
					friend class Dictionary;

				private:
					InternalMap::const_iterator It;
					const Dictionary& Base;

				public:
					void operator++();
					void operator++(int);
					LocalIterator& operator*();
					bool operator==(const LocalIterator& Other) const;
					bool operator!=(const LocalIterator& Other) const;
					const Core::String& GetKey() const;
					int GetTypeId() const;
					bool GetValue(void* Value, int TypeId) const;
					const void* GetAddressOfValue() const;

				private:
					LocalIterator() noexcept;
					LocalIterator(const Dictionary& From, InternalMap::const_iterator It) noexcept;
					LocalIterator& operator= (const LocalIterator&) noexcept
					{
						return *this;
					}
				};

				struct SCache
				{
					TypeInfo DictionaryType = TypeInfo(nullptr);
					TypeInfo ArrayType = TypeInfo(nullptr);
					TypeInfo KeyType = TypeInfo(nullptr);
				};

			private:
				static int DictionaryId;

			private:
				VirtualMachine* Engine;
				InternalMap Data;

			public:
				Dictionary(VirtualMachine* Engine) noexcept;
				Dictionary(unsigned char* Buffer) noexcept;
				Dictionary(const Dictionary&) noexcept;
				~Dictionary() noexcept;
				Dictionary& operator= (const Dictionary& Other) noexcept;
				void Set(const std::string_view& Key, void* Value, int TypeId);
				bool Get(const std::string_view& Key, void* Value, int TypeId) const;
				bool GetIndex(size_t Index, Core::String* Key, void** Value, int* TypeId) const;
				bool TryGetIndex(size_t Index, Core::String* Key, void* Value, int TypeId) const;
				Storable* operator[](const std::string_view& Key);
				const Storable* operator[](const std::string_view& Key) const;
				Storable* operator[](size_t);
				const Storable* operator[](size_t) const;
				int GetTypeId(const std::string_view& Key) const;
				bool Exists(const std::string_view& Key) const;
				bool Empty() const;
				size_t Size() const;
				bool Erase(const std::string_view& Key);
				void Clear();
				Array* GetKeys() const;
				LocalIterator Begin() const;
				LocalIterator End() const;
				LocalIterator Find(const std::string_view& Key) const;
				void EnumReferences(asIScriptEngine* Engine);
				void ReleaseReferences(asIScriptEngine* Engine);

			public:
				static Core::Unique<Dictionary> Create(VirtualMachine* Engine);
				static Core::Unique<Dictionary> Create(unsigned char* Buffer);
				static void Setup(VirtualMachine* VM);
				static void Factory(asIScriptGeneric* Generic);
				static void ListFactory(asIScriptGeneric* Generic);
				static void KeyCreate(void* Memory);
				static void KeyDestroy(Storable* Base);
				static Storable& KeyopAssign(Storable* Base, void* Ref, int TypeId);
				static Storable& KeyopAssign(Storable* Base, const Storable& Other);
				static Storable& KeyopAssign(Storable* Base, double Value);
				static Storable& KeyopAssign(Storable* Base, as_int64_t Value);
				static void KeyopCast(Storable* Base, void* Ref, int TypeId);
				static as_int64_t KeyopConvInt(Storable* Base);
				static double KeyopConvDouble(Storable* Base);

			public:
				template <typename T>
				static Dictionary* Compose(int TypeId, const Core::UnorderedMap<Core::String, T>& Objects)
				{
					auto* Engine = VirtualMachine::Get();
					Dictionary* Data = Create(Engine);
					for (auto& Item : Objects)
						Data->Set(Item.first, (void*)&Item.second, TypeId);

					return Data;
				}
				template <typename T>
				static typename std::enable_if<std::is_pointer<T>::value, Core::UnorderedMap<Core::String, T>>::type Decompose(int TypeId, Dictionary* Array)
				{
					Core::UnorderedMap<Core::String, T> Result;
					Result.reserve(Array->Size());

					int SubTypeId = 0;
					size_t Size = Array->Size();
					for (size_t i = 0; i < Size; i++)
					{
						Core::String Key; void* Value = nullptr;
						if (Array->GetIndex(i, &Key, &Value, &SubTypeId) && SubTypeId == TypeId)
							Result[Key] = (T*)Value;
					}

					return Result;
				}
				template <typename T>
				static typename std::enable_if<!std::is_pointer<T>::value, Core::UnorderedMap<Core::String, T>>::type Decompose(int TypeId, Dictionary* Array)
				{
					Core::UnorderedMap<Core::String, T> Result;
					Result.reserve(Array->Size());

					int SubTypeId = 0;
					size_t Size = Array->Size();
					for (size_t i = 0; i < Size; i++)
					{
						Core::String Key; void* Value = nullptr;
						if (Array->GetIndex(i, &Key, &Value, &SubTypeId) && SubTypeId == TypeId)
							Result[Key] = *(T*)Value;
					}

					return Result;
				}
			};

			class VI_OUT Promise : public Core::Reference<Promise>
			{
			private:
				static int PromiseNULL;
				static int PromiseUD;

			private:
				VirtualMachine* Engine;
				ImmediateContext* Context;
				FunctionDelegate Delegate;
				std::function<void(Promise*)> Bounce;
				std::mutex Update;
				Dynamic Value;

			public:
				Promise(ImmediateContext* NewContext) noexcept;
				~Promise() noexcept;
				void EnumReferences(asIScriptEngine* OtherEngine);
				void ReleaseReferences(asIScriptEngine*);
				int GetTypeId();
				void* GetAddressOfObject();
				void When(asIScriptFunction* NewCallback);
				void When(std::function<void(Promise*)>&& NewCallback);
				void Store(void* RefPointer, int RefTypeId);
				void Store(void* RefPointer, const char* TypeName);
				void StoreException(const Exception::Pointer& RefValue);
				void StoreVoid();
				bool Retrieve(void* RefPointer, int RefTypeId);
				void RetrieveVoid();
				void* Retrieve();
				bool IsPending();
				Promise* YieldIf();

			public:
				static Promise* CreateFactory(void* _Ref, int TypeId);
				static Promise* CreateFactoryType(asITypeInfo* Type);
				static Promise* CreateFactoryVoid();
				static bool TemplateCallback(asITypeInfo* Info, bool& DontGarbageCollect);
				static ExpectsVM<void> GeneratorCallback(Compute::Preprocessor* Base, const std::string_view& Path, Core::String& Code);
				static bool IsContextPending(ImmediateContext* Context);
				static bool IsContextBusy(ImmediateContext* Context);

			public:
				template <typename T>
				static void DispatchAwaitable(Promise* Future, int TypeId, Core::Promise<T>&& Awaitable)
				{
					Future->AddRef();
					Future->Context->AppendStopExecutionCallback([Future, TypeId, Awaitable]()
					{
						Awaitable.When([Future, TypeId](T&& Result)
						{
							Future->Store((void*)&Result, TypeId);
							Future->Release();
						});
					});
				}
				template <typename T>
				static Core::Unique<Promise> Compose(Core::Promise<T>&& Value, TypeId Id)
				{
					Promise* Future = Promise::CreateFactoryVoid();
					return Promise::DispatchAwaitable<T>(Future, (int)Id, std::move(Value));
				}
				template <typename T>
				static Core::Unique<Promise> Compose(Core::Promise<T>&& Value, const char* TypeName)
				{
					VirtualMachine* Engine = VirtualMachine::Get();
					VI_ASSERT(Engine != nullptr, "engine should be set");
					Promise* Future = Promise::CreateFactoryVoid();
					int TypeId = Engine->GetTypeIdByDecl(TypeName);
					return Promise::DispatchAwaitable<T>(TypeId, TypeId, std::move(Value));
				}

			public:
				template <typename T, T>
				struct Ify;

				template <typename T, T>
				struct IfyStatic;

				template <typename T, typename R, typename ...Args, Core::Promise<R>(T::* F)(Args...)>
				struct Ify<Core::Promise<R>(T::*)(Args...), F>
				{
					template <TypeId TypeID>
					static Promise* Id(T* Base, Args... Data)
					{
						Promise* Future = Promise::CreateFactoryVoid();
						if (!Future)
							return Future;

						Future->Context->EnableDeferredExceptions();
						Promise::DispatchAwaitable<R>(Future, (int)TypeID, ((Base->*F)(Data...)));
						Future->Context->DisableDeferredExceptions();
						return Future;
					}
					template <uint64_t TypeRef>
					static Promise* Decl(T* Base, Args... Data)
					{
						Promise* Future = Promise::CreateFactoryVoid();
						if (!Future)
							return Future;

						int TypeId = TypeCache::GetTypeId(TypeRef);
						Future->Context->EnableDeferredExceptions();
						Promise::DispatchAwaitable<R>(Future, TypeId, ((Base->*F)(Data...)));
						Future->Context->DisableDeferredExceptions();
						return Future;
					}
				};

				template <typename R, typename ...Args, Core::Promise<R>(*F)(Args...)>
				struct IfyStatic<Core::Promise<R>(*)(Args...), F>
				{
					template <TypeId TypeID>
					static Promise* Id(Args... Data)
					{
						Promise* Future = Promise::CreateFactoryVoid();
						if (!Future)
							return Future;

						Future->Context->EnableDeferredExceptions();
						Promise::DispatchAwaitable<R>(Future, (int)TypeID, ((*F)(Data...)));
						Future->Context->DisableDeferredExceptions();
						return Future;
					}
					template <uint64_t TypeRef>
					static Promise* Decl(Args... Data)
					{
						Promise* Future = Promise::CreateFactoryVoid();
						if (!Future)
							return Future;

						int TypeId = TypeCache::GetTypeId(TypeRef);
						Future->Context->EnableDeferredExceptions();
						Promise::DispatchAwaitable<R>(Future, TypeId, ((*F)(Data...)));
						Future->Context->DisableDeferredExceptions();
						return Future;
					}
				};
			};
#ifdef VI_BINDINGS
			class VI_OUT Complex
			{
			public:
				float R;
				float I;

			public:
				Complex() noexcept;
				Complex(const Complex& Other) noexcept;
				Complex(float R, float I = 0) noexcept;
				Complex& operator= (const Complex& Other) noexcept;
				Complex& operator+= (const Complex& Other);
				Complex& operator-= (const Complex& Other);
				Complex& operator*= (const Complex& Other);
				Complex& operator/= (const Complex& Other);
				float Length() const;
				float SquaredLength() const;
				Complex GetRI() const;
				void SetRI(const Complex& In);
				Complex GetIR() const;
				void SetIR(const Complex& In);
				bool operator== (const Complex& Other) const;
				bool operator!= (const Complex& Other) const;
				Complex operator+ (const Complex& Other) const;
				Complex operator- (const Complex& Other) const;
				Complex operator* (const Complex& Other) const;
				Complex operator/ (const Complex& Other) const;

			public:
				static void DefaultConstructor(Complex* Base);
				static void CopyConstructor(Complex* Base, const Complex& Other);
				static void ConvConstructor(Complex* Base, float NewR);
				static void InitConstructor(Complex* Base, float NewR, float NewI);
				static void ListConstructor(Complex* Base, float* List);
			};

			class VI_OUT Mutex : public Core::Reference<Mutex>
			{
			private:
				static int MutexUD;

			private:
				std::recursive_mutex Base;

			public:
				Mutex() noexcept;
				~Mutex() = default;
				bool TryLock();
				void Lock();
				void Unlock();

			public:
				static Mutex* Factory();
				static bool IsAnyLocked(ImmediateContext* Context);
			};

			class VI_OUT Thread : public Core::Reference<Thread>
			{
			private:
				static int ThreadUD;

			private:
				struct
				{
					Core::Vector<Any*> Queue;
					std::condition_variable CV;
					std::mutex Mutex;
				} Pipe[2];

			private:
				std::thread Procedure;
				std::recursive_mutex Mutex;
				Exception::Pointer Raise;
				VirtualMachine* VM;
				EventLoop* Loop;
				FunctionDelegate Function;

			public:
				Thread(VirtualMachine* Engine, asIScriptFunction* Function) noexcept;
				~Thread() noexcept;
				void EnumReferences(asIScriptEngine* Engine);
				void ReleaseReferences(asIScriptEngine* Engine);
				bool Suspend();
				bool Resume();
				void Push(void* Ref, int TypeId);
				bool Pop(void* Ref, int TypeId);
				bool Pop(void* Ref, int TypeId, uint64_t Timeout);
				bool IsActive();
				bool Start();
				int Join(uint64_t Timeout);
				int Join();
				Core::String GetId() const;

			private:
				void ExecutionLoop();

			public:
				static Thread* Create(asIScriptFunction* Callback);
				static Thread* GetThread();
				static Core::String GetThreadId();
				static void ThreadSleep(uint64_t Mills);
				static bool ThreadSuspend();
			};

			class VI_OUT CharBuffer : public Core::Reference<CharBuffer>
			{
			private:
				char* Buffer;
				size_t Length;

			public:
				CharBuffer() noexcept;
				CharBuffer(size_t Size) noexcept;
				CharBuffer(char* Pointer) noexcept;
				~CharBuffer() noexcept;
				bool Allocate(size_t Size);
				void Deallocate();
				bool SetInt8(size_t Offset, int8_t Value, size_t Size);
				bool SetUint8(size_t Offset, uint8_t Value, size_t Size);
				bool StoreBytes(size_t Offset, const std::string_view& Value);
				bool StoreInt8(size_t Offset, int8_t Value);
				bool StoreUint8(size_t Offset, uint8_t Value);
				bool StoreInt16(size_t Offset, int16_t Value);
				bool StoreUint16(size_t Offset, uint16_t Value);
				bool StoreInt32(size_t Offset, int32_t Value);
				bool StoreUint32(size_t Offset, uint32_t Value);
				bool StoreInt64(size_t Offset, int64_t Value);
				bool StoreUint64(size_t Offset, uint64_t Value);
				bool StoreFloat(size_t Offset, float Value);
				bool StoreDouble(size_t Offset, double Value);
				bool Interpret(size_t Offset, Core::String& Value, size_t MaxSize) const;
				bool LoadBytes(size_t Offset, Core::String& Value, size_t Size) const;
				bool LoadInt8(size_t Offset, int8_t& Value) const;
				bool LoadUint8(size_t Offset, uint8_t& Value) const;
				bool LoadInt16(size_t Offset, int16_t& Value) const;
				bool LoadUint16(size_t Offset, uint16_t& Value) const;
				bool LoadInt32(size_t Offset, int32_t& Value) const;
				bool LoadUint32(size_t Offset, uint32_t& Value) const;
				bool LoadInt64(size_t Offset, int64_t& Value) const;
				bool LoadUint64(size_t Offset, uint64_t& Value) const;
				bool LoadFloat(size_t Offset, float& Value) const;
				bool LoadDouble(size_t Offset, double& Value) const;
				void* GetPointer(size_t Offset) const;
				bool Exists(size_t Offset) const;
				bool Empty() const;
				size_t Size() const;
				Core::String ToString(size_t MaxSize) const;

			private:
				bool Store(size_t Offset, const char* Data, size_t Size);
				bool Load(size_t Offset, char* Data, size_t Size) const;

			public:
				static CharBuffer* Create();
				static CharBuffer* Create(size_t Size);
				static CharBuffer* Create(char* Pointer);
			};

			class VI_OUT Application final : public Layer::Application
			{
			public:
				FunctionDelegate OnDispatch;
				FunctionDelegate OnPublish;
				FunctionDelegate OnComposition;
				FunctionDelegate OnScriptHook;
				FunctionDelegate OnInitialize;
				FunctionDelegate OnStartup;
				FunctionDelegate OnShutdown;

			private:
				size_t ProcessedEvents;
				asITypeInfo* InitiatorType;
				void* InitiatorObject;

			public:
				Application(Desc& I, void* Object, int TypeId) noexcept;
				virtual ~Application() noexcept override;
				void SetOnDispatch(asIScriptFunction* Callback);
				void SetOnPublish(asIScriptFunction* Callback);
				void SetOnComposition(asIScriptFunction* Callback);
				void SetOnScriptHook(asIScriptFunction* Callback);
				void SetOnInitialize(asIScriptFunction* Callback);
				void SetOnStartup(asIScriptFunction* Callback);
				void SetOnShutdown(asIScriptFunction* Callback);
				void Dispatch(Core::Timer* Time) override;
				void Publish(Core::Timer* Time) override;
				void Composition() override;
				void ScriptHook() override;
				void Initialize() override;
				Core::Promise<void> Startup() override;
				Core::Promise<void> Shutdown() override;
				size_t GetProcessedEvents() const;
				bool HasProcessedEvents() const;
				bool RetrieveInitiatorObject(void* RefPointer, int RefTypeId) const;
				void* GetInitiatorObject() const;

			public:
				static bool WantsRestart(int ExitCode);
			};
#endif
		}
	}
}
#endif