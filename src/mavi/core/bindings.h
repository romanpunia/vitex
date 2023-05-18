#ifndef VI_BINDINGS_H
#define VI_BINDINGS_H
#include "scripting.h"
#include "../engine/gui.h"
#define VI_TYPEREF(Name, TypeName) static const uint64_t Name = VI_SHUFFLE(TypeName); Mavi::Scripting::TypeCache::Set(Name, TypeName)
#define VI_PROMISIFY(MemberFunction, TypeId) Mavi::Scripting::Bindings::Promise::Ify<decltype(&MemberFunction), &MemberFunction>::Id<TypeId>
#define VI_PROMISIFY_REF(MemberFunction, TypeRef) Mavi::Scripting::Bindings::Promise::Ify<decltype(&MemberFunction), &MemberFunction>::Decl<TypeRef>
#define VI_SPROMISIFY(Function, TypeId) Mavi::Scripting::Bindings::Promise::IfyStatic<decltype(&Function), &Function>::Id<TypeId>
#define VI_SPROMISIFY_REF(Function, TypeRef) Mavi::Scripting::Bindings::Promise::IfyStatic<decltype(&Function), &Function>::Decl<TypeRef>
#define VI_ARRAYIFY(MemberFunction, TypeId) Mavi::Scripting::Bindings::Array::Ify<decltype(&MemberFunction), &MemberFunction>::Id<TypeId>
#define VI_ARRAYIFY_REF(MemberFunction, TypeRef) Mavi::Scripting::Bindings::Array::Ify<decltype(&MemberFunction), &MemberFunction>::Decl<TypeRef>
#define VI_SARRAYIFY(Function, TypeId) Mavi::Scripting::Bindings::Array::IfyStatic<decltype(&Function), &Function>::Id<TypeId>
#define VI_SARRAYIFY_REF(Function, TypeRef) Mavi::Scripting::Bindings::Array::IfyStatic<decltype(&Function), &Function>::Decl<TypeRef>
#define VI_ANYIFY(MemberFunction, TypeId) Mavi::Scripting::Bindings::Any::Ify<decltype(&MemberFunction), &MemberFunction>::Id<TypeId>
#define VI_ANYIFY_REF(MemberFunction, TypeRef) Mavi::Scripting::Bindings::Any::Ify<decltype(&MemberFunction), &MemberFunction>::Decl<TypeRef>
#define VI_SANYIFY(Function, TypeId) Mavi::Scripting::Bindings::Any::IfyStatic<decltype(&Function), &Function>::Id<TypeId>
#define VI_SANYIFY_REF(Function, TypeRef) Mavi::Scripting::Bindings::Any::IfyStatic<decltype(&Function), &Function>::Decl<TypeRef>
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

namespace Mavi
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

			class VI_OUT_TS Registry
			{
			public:
				static bool LoadCTypes(VirtualMachine* VM);
				static bool LoadAny(VirtualMachine* VM);
				static bool LoadArray(VirtualMachine* VM);
				static bool LoadComplex(VirtualMachine* VM);
				static bool LoadDictionary(VirtualMachine* VM);
				static bool LoadRef(VirtualMachine* VM);
				static bool LoadWeakRef(VirtualMachine* VM);
				static bool LoadMath(VirtualMachine* VM);
				static bool LoadString(VirtualMachine* VM);
				static bool LoadException(VirtualMachine* VM);
				static bool LoadMutex(VirtualMachine* VM);
				static bool LoadThread(VirtualMachine* VM);
				static bool LoadRandom(VirtualMachine* VM);
				static bool LoadPromise(VirtualMachine* VM);
				static bool LoadPromiseAsync(VirtualMachine* VM);
				static bool LoadFormat(VirtualMachine* Engine);
				static bool LoadDecimal(VirtualMachine* Engine);
				static bool LoadVariant(VirtualMachine* Engine);
				static bool LoadTimestamp(VirtualMachine* Engine);
				static bool LoadConsole(VirtualMachine* Engine);
				static bool LoadSchema(VirtualMachine* Engine);
				static bool LoadClockTimer(VirtualMachine* Engine);
				static bool LoadFileSystem(VirtualMachine* Engine);
				static bool LoadOS(VirtualMachine* Engine);
				static bool LoadSchedule(VirtualMachine* Engine);
				static bool LoadVertices(VirtualMachine* Engine);
				static bool LoadVectors(VirtualMachine* Engine);
				static bool LoadShapes(VirtualMachine* Engine);
				static bool LoadKeyFrames(VirtualMachine* Engine);
				static bool LoadRegex(VirtualMachine* Engine);
				static bool LoadCrypto(VirtualMachine* Engine);
				static bool LoadGeometric(VirtualMachine* Engine);
				static bool LoadPreprocessor(VirtualMachine* Engine);
				static bool LoadPhysics(VirtualMachine* Engine);
				static bool LoadAudio(VirtualMachine* Engine);
				static bool LoadActivity(VirtualMachine* Engine);
				static bool LoadGraphics(VirtualMachine* Engine);
				static bool LoadNetwork(VirtualMachine* Engine);
				static bool LoadVM(VirtualMachine* Engine);
				static bool LoadEngine(VirtualMachine* Engine);
				static bool LoadComponents(VirtualMachine* Engine);
				static bool LoadRenderers(VirtualMachine* Engine);
				static bool LoadUiModel(VirtualMachine* Engine);
				static bool LoadUiControl(VirtualMachine* Engine);
				static bool LoadUiContext(VirtualMachine* Engine);
				static bool Release();
			};

			class VI_OUT Exception
			{
			public:
				struct Pointer
				{
					Core::String Type;
					Core::String Message;
					Core::String Origin;
					asIScriptContext* Context;

					Pointer();
					Pointer(asIScriptContext* Context);
					Pointer(const Core::String& Data);
					Pointer(const Core::String& Type, const Core::String& Message);
					void LoadExceptionData(const Core::String& Data);
					const Core::String& GetType() const;
					const Core::String& GetMessage() const;
					Core::String ToExceptionString() const;
					Core::String What() const;
					Core::String LoadStackHere() const;
					bool Empty() const;
				};

			public:
				static void Throw(const Pointer& Data);
				static void Rethrow();
				static bool HasException();
				static Pointer GetException();
				static bool GeneratorCallback(const Core::String& Path, Core::String& Code);
			};

			class VI_OUT String
			{
			public:
				static void Construct(Core::String* thisPointer);
				static void CopyConstruct(const Core::String& other, Core::String* thisPointer);
				static void Destruct(Core::String* thisPointer);
				static Core::String& AddAssignTo(const Core::String& str, Core::String& dest);
				static bool IsEmpty(const Core::String& str);
				static void* ToPtr(const Core::String& Value);
				static Core::String Reverse(const Core::String& Value);
				static Core::String& AssignUInt64To(as_uint64_t i, Core::String& dest);
				static Core::String& AddAssignUInt64To(as_uint64_t i, Core::String& dest);
				static Core::String AddUInt641(const Core::String& str, as_uint64_t i);
				static Core::String AddInt641(as_int64_t i, const Core::String& str);
				static Core::String& AssignInt64To(as_int64_t i, Core::String& dest);
				static Core::String& AddAssignInt64To(as_int64_t i, Core::String& dest);
				static Core::String AddInt642(const Core::String& str, as_int64_t i);
				static Core::String AddUInt642(as_uint64_t i, const Core::String& str);
				static Core::String& AssignDoubleTo(double f, Core::String& dest);
				static Core::String& AddAssignDoubleTo(double f, Core::String& dest);
				static Core::String& AssignFloatTo(float f, Core::String& dest);
				static Core::String& AddAssignFloatTo(float f, Core::String& dest);
				static Core::String& AssignBoolTo(bool b, Core::String& dest);
				static Core::String& AddAssignBoolTo(bool b, Core::String& dest);
				static Core::String AddDouble1(const Core::String& str, double f);
				static Core::String AddDouble2(double f, const Core::String& str);
				static Core::String AddFloat1(const Core::String& str, float f);
				static Core::String AddFloat2(float f, const Core::String& str);
				static Core::String AddBool1(const Core::String& str, bool b);
				static Core::String AddBool2(bool b, const Core::String& str);
				static char* CharAt(size_t i, Core::String& str);
				static int Cmp(const Core::String& a, const Core::String& b);
				static int FindFirst(const Core::String& sub, size_t start, const Core::String& str);
				static int FindFirstOf(const Core::String& sub, size_t start, const Core::String& str);
				static int FindLastOf(const Core::String& sub, size_t start, const Core::String& str);
				static int FindFirstNotOf(const Core::String& sub, size_t start, const Core::String& str);
				static int FindLastNotOf(const Core::String& sub, size_t start, const Core::String& str);
				static int FindLast(const Core::String& sub, int start, const Core::String& str);
				static void Insert(size_t pos, const Core::String& other, Core::String& str);
				static void Erase(size_t pos, int count, Core::String& str);
				static size_t Length(const Core::String& str);
				static void Resize(size_t l, Core::String& str);
				static Core::String Replace(const Core::String& a, const Core::String& b, size_t o, const Core::String& base);
				static as_int64_t IntStore(const Core::String& val, size_t base);
				static as_uint64_t UIntStore(const Core::String& val, size_t base);
				static float FloatStore(const Core::String& val);
				static double DoubleStore(const Core::String& val);
				static Core::String Sub(size_t start, int count, const Core::String& str);
				static bool Equals(const Core::String& lhs, const Core::String& rhs);
				static Core::String ToLower(const Core::String& Symbol);
				static Core::String ToUpper(const Core::String& Symbol);
				static Core::String ToInt8(char Value);
				static Core::String ToInt16(short Value);
				static Core::String ToInt(int Value);
				static Core::String ToInt64(int64_t Value);
				static Core::String ToUInt8(unsigned char Value);
				static Core::String ToUInt16(unsigned short Value);
				static Core::String ToUInt(unsigned int Value);
				static Core::String ToUInt64(uint64_t Value);
				static Core::String ToFloat(float Value);
				static Core::String ToDouble(double Value);
				static Core::String ToPointer(void* Value);
				static Array* Split(const Core::String& delim, const Core::String& str);
				static Core::String Join(const Array& array, const Core::String& delim);
				static char ToChar(const Core::String& Symbol);
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

			class VI_OUT Any
			{
				friend Promise;

			protected:
				mutable int RefCount;
				mutable bool GCFlag;
				asIScriptEngine* Engine;
				Dynamic Value;

			public:
				Any(asIScriptEngine* Engine) noexcept;
				Any(void* Ref, int RefTypeId, asIScriptEngine* Engine) noexcept;
				Any(const Any&) noexcept;
				int AddRef() const;
				int Release() const;
				Any& operator= (const Any&) noexcept;
				int CopyFrom(const Any* Other);
				void Store(void* Ref, int RefTypeId);
				bool Retrieve(void* Ref, int RefTypeId) const;
				void* GetAddressOfObject();
				int GetTypeId() const;
				int GetRefCount();
				void SetFlag();
				bool GetFlag();
				void EnumReferences(asIScriptEngine* Engine);
				void ReleaseAllHandles(asIScriptEngine* Engine);

			protected:
				~Any() noexcept;
				void FreeObject();

			public:
				static Core::Unique<Any> Create();
				static Core::Unique<Any> Create(int TypeId, void* Ref);
				static Core::Unique<Any> Create(const char* Decl, void* Ref);
				static void Factory1(asIScriptGeneric* G);
				static void Factory2(asIScriptGeneric* G);
				static Any& Assignment(Any* Other, Any* Self);

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

			class VI_OUT Array
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
					asIScriptFunction* CmpFunc;
					asIScriptFunction* EqFunc;
					int CmpFuncReturnCode;
					int EqFuncReturnCode;
				};

			protected:
				mutable int RefCount;
				mutable bool GCFlag;
				asITypeInfo* ObjType;
				SBuffer* Buffer;
				int ElementSize;
				int SubTypeId;

			public:
				void AddRef() const;
				void Release() const;
				asITypeInfo* GetArrayObjectType() const;
				int GetArrayTypeId() const;
				int GetElementTypeId() const;
				size_t GetSize() const;
				bool IsEmpty() const;
				void Reserve(size_t MaxElements);
				void Resize(size_t NumElements);
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
				void SortAsc();
				void SortDesc();
				void SortAsc(size_t StartAt, size_t Count);
				void SortDesc(size_t StartAt, size_t Count);
				void Sort(size_t StartAt, size_t Count, bool Asc);
				void Sort(asIScriptFunction* Less, size_t StartAt, size_t Count);
				void Reverse();
				int Find(void* Value) const;
				int Find(size_t StartAt, void* Value) const;
				int FindByRef(void* Ref) const;
				int FindByRef(size_t StartAt, void* Ref) const;
				void* GetBuffer();
				int GetRefCount();
				void SetFlag();
				bool GetFlag();
				void EnumReferences(asIScriptEngine* Engine);
				void ReleaseAllHandles(asIScriptEngine* Engine);

			protected:
				Array(asITypeInfo* T, void* InitBuf) noexcept;
				Array(size_t Length, asITypeInfo* T) noexcept;
				Array(size_t Length, void* DefVal, asITypeInfo* T) noexcept;
				Array(const Array& Other) noexcept;
				~Array() noexcept;
				bool Less(const void* A, const void* B, bool Asc, asIScriptContext* Ctx, SCache* Cache);
				void* GetArrayItemPointer(int Index);
				void* GetDataPointer(void* Buffer);
				void Copy(void* Dst, void* Src);
				void Precache();
				bool CheckMaxSize(size_t NumElements);
				void Resize(int Delta, size_t At);
				void CreateBuffer(SBuffer** Buf, size_t NumElements);
				void DeleteBuffer(SBuffer* Buf);
				void CopyBuffer(SBuffer* Dst, SBuffer* Src);
				void Construct(SBuffer* Buf, size_t Start, size_t End);
				void Destruct(SBuffer* Buf, size_t Start, size_t End);
				bool Equals(const void* A, const void* B, asIScriptContext* Ctx, SCache* Cache) const;

			public:
				static Core::Unique<Array> Create(asITypeInfo* T);
				static Core::Unique<Array> Create(asITypeInfo* T, size_t Length);
				static Core::Unique<Array> Create(asITypeInfo* T, size_t Length, void* DefaultValue);
				static Core::Unique<Array> Create(asITypeInfo* T, void* ListBuffer);
				static void CleanupTypeInfoCache(asITypeInfo* Type);
				static bool TemplateCallback(asITypeInfo* T, bool& DontGarbageCollect);

			public:
				template <typename T>
				static Array* Compose(asITypeInfo* ArrayType, const Core::Vector<T>& Objects)
				{
					Array* Array = Create(ArrayType, Objects.size());
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

					size_t Size = Array->GetSize();
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

					size_t Size = Array->GetSize();
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
						VI_ASSERT(VM != nullptr, nullptr, "manager should be present");

						asITypeInfo* Info = VM->GetTypeInfoById((int)TypeId).GetTypeInfo();
						VI_ASSERT(Info != nullptr, nullptr, "typeinfo should be valid");

						Core::Vector<R> Source((Base->*F)(Data...));
						return Array::Compose(Info, Source);
					}
					template <uint64_t TypeRef>
					static Array* Decl(T* Base, Args... Data)
					{
						VirtualMachine* VM = VirtualMachine::Get();
						VI_ASSERT(VM != nullptr, nullptr, "manager should be present");

						asITypeInfo* Info = VM->GetTypeInfoById(TypeCache::GetTypeId(TypeRef)).GetTypeInfo();
						VI_ASSERT(Info != nullptr, nullptr, "typeinfo should be valid");

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
						VI_ASSERT(VM != nullptr, nullptr, "manager should be present");

						asITypeInfo* Info = VM->GetTypeInfoById((int)TypeId).GetTypeInfo();
						VI_ASSERT(Info != nullptr, nullptr, "typeinfo should be valid");

						Core::Vector<R> Source((*F)(Data...));
						return Array::Compose(Info, Source);
					}
					template <uint64_t TypeRef>
					static Array* Decl(Args... Data)
					{
						VirtualMachine* VM = VirtualMachine::Get();
						VI_ASSERT(VM != nullptr, nullptr, "manager should be present");

						asITypeInfo* Info = VM->GetTypeInfoById(TypeCache::GetTypeId(TypeRef)).GetTypeInfo();
						VI_ASSERT(Info != nullptr, nullptr, "typeinfo should be valid");

						Core::Vector<R> Source((*F)(Data...));
						return Array::Compose(Info, Source);
					}
				};
			};

			class VI_OUT Storable
			{
			protected:
				friend class Dictionary;

			protected:
				Dynamic Value;

			public:
				Storable() noexcept;
				Storable(asIScriptEngine* Engine, void* Pointer, int TypeId) noexcept;
				~Storable() noexcept;
				void Set(asIScriptEngine* Engine, void* Pointer, int TypeId);
				void Set(asIScriptEngine* Engine, Storable& Other);
				bool Get(asIScriptEngine* Engine, void* Pointer, int TypeId) const;
				const void* GetAddressOfValue() const;
				int GetTypeId() const;
				void FreeValue(asIScriptEngine* Engine);
				void EnumReferences(asIScriptEngine* Engine);
			};

			class VI_OUT Dictionary
			{
			public:
				typedef Core::UnorderedMap<Core::String, Storable> InternalMap;

			public:
				class LocalIterator
				{
				protected:
					friend class Dictionary;

				protected:
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

				protected:
					LocalIterator() noexcept;
					LocalIterator(const Dictionary& From, InternalMap::const_iterator It) noexcept;
					LocalIterator& operator= (const LocalIterator&) noexcept
					{
						return *this;
					}
				};

				struct SCache
				{
					asITypeInfo* DictionaryType;
					asITypeInfo* ArrayType;
					asITypeInfo* KeyType;
				};

			protected:
				asIScriptEngine* Engine;
				mutable int RefCount;
				mutable bool GCFlag;
				InternalMap Data;

			public:
				void AddRef() const;
				void Release() const;
				Dictionary& operator= (const Dictionary& Other) noexcept;
				void Set(const Core::String& Key, void* Value, int TypeId);
				bool Get(const Core::String& Key, void* Value, int TypeId) const;
				bool GetIndex(size_t Index, Core::String* Key, void** Value, int* TypeId) const;
				Storable* operator[](const Core::String& Key);
				const Storable* operator[](const Core::String& Key) const;
				int GetTypeId(const Core::String& Key) const;
				bool Exists(const Core::String& Key) const;
				bool IsEmpty() const;
				size_t GetSize() const;
				bool Delete(const Core::String& Key);
				void DeleteAll();
				Array* GetKeys() const;
				LocalIterator Begin() const;
				LocalIterator End() const;
				LocalIterator Find(const Core::String& Key) const;
				int GetRefCount();
				void SetGCFlag();
				bool GetGCFlag();
				void EnumReferences(asIScriptEngine* Engine);
				void ReleaseAllReferences(asIScriptEngine* Engine);

			protected:
				Dictionary(asIScriptEngine* Engine) noexcept;
				Dictionary(unsigned char* Buffer) noexcept;
				Dictionary(const Dictionary&) noexcept;
				~Dictionary() noexcept;
				void Init(asIScriptEngine* Engine);

			public:
				static Core::Unique<Dictionary> Create(asIScriptEngine* Engine);
				static Core::Unique<Dictionary> Create(unsigned char* Buffer);
				static void Cleanup(asIScriptEngine* engine);
				static void Setup(asIScriptEngine* engine);
				static void Factory(asIScriptGeneric* gen);
				static void ListFactory(asIScriptGeneric* gen);
				static void KeyConstruct(void* mem);
				static void KeyDestruct(Storable* obj);
				static Storable& KeyopAssign(void* ref, int typeId, Storable* obj);
				static Storable& KeyopAssign(const Storable& other, Storable* obj);
				static Storable& KeyopAssign(double val, Storable* obj);
				static Storable& KeyopAssign(as_int64_t val, Storable* obj);
				static void KeyopCast(void* ref, int typeId, Storable* obj);
				static as_int64_t KeyopConvInt(Storable* obj);
				static double KeyopConvDouble(Storable* obj);

			public:
				template <typename T>
				static Dictionary* Compose(int TypeId, const Core::UnorderedMap<Core::String, T>& Objects)
				{
					auto* Engine = VirtualMachine::Get();
					Dictionary* Data = Create(Engine ? Engine->GetEngine() : nullptr);
					for (auto& Item : Objects)
						Data->Set(Item.first, (void*)&Item.second, TypeId);

					return Data;
				}
				template <typename T>
				static typename std::enable_if<std::is_pointer<T>::value, Core::UnorderedMap<Core::String, T>>::type Decompose(int TypeId, Dictionary* Array)
				{
					Core::UnorderedMap<Core::String, T> Result;
					Result.reserve(Array->GetSize());

					int SubTypeId = 0;
					size_t Size = Array->GetSize();
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
					Result.reserve(Array->GetSize());

					int SubTypeId = 0;
					size_t Size = Array->GetSize();
					for (size_t i = 0; i < Size; i++)
					{
						Core::String Key; void* Value = nullptr;
						if (Array->GetIndex(i, &Key, &Value, &SubTypeId) && SubTypeId == TypeId)
							Result[Key] = *(T*)Value;
					}

					return Result;
				}
			};

			class VI_OUT Ref
			{
			protected:
				asITypeInfo* Type;
				void* Pointer;

			public:
				Ref() noexcept;
				Ref(const Ref& Other) noexcept;
				Ref(void* Ref, asITypeInfo* Type) noexcept;
				Ref(void* Ref, int TypeId) noexcept;
				~Ref() noexcept;
				Ref& operator=(const Ref& Other) noexcept;
				void Set(void* Ref, asITypeInfo* Type);
				bool operator== (const Ref& Other) const;
				bool operator!= (const Ref& Other) const;
				bool Equals(void* Ref, int TypeId) const;
				void Cast(void** OutRef, int TypeId);
				asITypeInfo* GetType() const;
				int GetTypeId() const;
				void* GetAddressOfObject();
				void EnumReferences(asIScriptEngine* Engine);
				void ReleaseReferences(asIScriptEngine* Engine);
				Ref& Assign(void* Ref, int TypeId);

			protected:
				void ReleaseHandle();
				void AddRefHandle();

			public:
				static void Construct(Ref* self);
				static void Construct(Ref* self, const Ref& o);
				static void Construct(Ref* self, void* ref, int typeId);
				static void Destruct(Ref* self);
			};

			class VI_OUT Weak
			{
			protected:
				asILockableSharedBool* WeakRefFlag;
				asITypeInfo* Type;
				void* Ref;

			public:
				Weak(asITypeInfo* Type) noexcept;
				Weak(const Weak& Other) noexcept;
				Weak(void* Ref, asITypeInfo* Type) noexcept;
				~Weak() noexcept;
				Weak& operator= (const Weak& Other) noexcept;
				bool operator== (const Weak& Other) const;
				bool operator!= (const Weak& Other) const;
				Weak& Set(void* NewRef);
				int GetTypeId() const;
				void* GetAddressOfObject();
				void* Get() const;
				bool Equals(void* Ref) const;
				asITypeInfo* GetRefType() const;

			public:
				static void Construct(asITypeInfo* type, void* mem);
				static void Construct2(asITypeInfo* type, void* ref, void* mem);
				static void Destruct(Weak* obj);
				static bool TemplateCallback(asITypeInfo* TI, bool&);
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

			class VI_OUT Promise
			{
			private:
				static int PromiseNULL;
				static int PromiseUD;

			private:
				asIScriptEngine* Engine;
				asIScriptContext* Context;
				asIScriptFunction* Callback;
				std::atomic<int> RefCount;
				std::mutex Update;
				Dynamic Value;

			public:
				void Release();
				void AddRef();
				void EnumReferences(asIScriptEngine* OtherEngine);
				void ReleaseReferences(asIScriptEngine*);
				void SetFlag();
				bool GetFlag();
				int GetRefCount();
				int GetTypeId();
				void* GetAddressOfObject();
				void When(asIScriptFunction* NewCallback);
				void Store(void* RefPointer, int RefTypeId);
				void Store(void* RefPointer, const char* TypeName);
				void StoreVoid();
				bool Retrieve(void* RefPointer, int RefTypeId);
				void RetrieveVoid();
				void* Retrieve();
				bool IsPending();
				Promise* YieldIf();

			private:
				Promise(asIScriptContext* NewContext) noexcept;

			public:
				static Promise* Create();
				static Promise* CreateFactory(void* _Ref, int TypeId);
				static Promise* CreateFactoryVoid();
				static bool TemplateCallback(asITypeInfo* Info, bool& DontGarbageCollect);
				static bool GeneratorCallback(const Core::String& Path, Core::String& Code);
				static Core::String GetStatus(ImmediateContext* Context);

			public:
				template <typename T>
				static Core::Unique<Promise> Compose(Core::Promise<T>&& Value, TypeId Id)
				{
					Promise* Future = Promise::Create();
					Value.When([Future, Id](T&& Result)
					{
						Future->Store((void*)&Result, (int)Id);
					});

					return Future;
				}
				template <typename T>
				static Core::Unique<Promise> Compose(Core::Promise<T>&& Value, const char* TypeName)
				{
					VirtualMachine* Engine = VirtualMachine::Get();
					VI_ASSERT(Engine != nullptr, -1, "engine should be set");
					return Compose<T>(std::move(Value), Engine->GetTypeIdByDecl(TypeName));
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
						Promise* Future = Promise::Create();
						Core::Coasync<void>([Future, Base, Data...]() -> Core::Promise<void>
						{
							auto Indirect = ((Base->*F)(Data...));
							auto&& Result = VI_AWAIT(Indirect);
							Future->Store((void*)&Result, (int)TypeID);
						});

						return Future;
					}
					template <uint64_t TypeRef>
					static Promise* Decl(T* Base, Args... Data)
					{
						Promise* Future = Promise::Create();
						int Id = TypeCache::GetTypeId(TypeRef);
						Core::Coasync<void>([Future, Id, Base, Data...]() -> Core::Promise<void>
						{
							auto Indirect = ((Base->*F)(Data...));
							auto&& Result = VI_AWAIT(Indirect);
							Future->Store((void*)&Result, Id);
						});

						return Future;
					}
				};

				template <typename R, typename ...Args, Core::Promise<R>(*F)(Args...)>
				struct IfyStatic<Core::Promise<R>(*)(Args...), F>
				{
					template <TypeId TypeID>
					static Promise* Id(Args... Data)
					{
						Promise* Future = Promise::Create();
						Core::Coasync<void>([Future, Data...]() -> Core::Promise<void>
						{
							auto Indirect = ((*F)(Data...));
							auto&& Result = VI_AWAIT(Indirect);
							Future->Store((void*)&Result, (int)TypeID);
						});

						return Future;
					}
					template <uint64_t TypeRef>
					static Promise* Decl(Args... Data)
					{
						Promise* Future = Promise::Create();
						int TypeId = TypeCache::GetTypeId(TypeRef);
						Core::Coasync<void>([Future, TypeId, Data...]() -> Core::Promise<void>
						{
							auto Indirect = ((*F)(Data...));
							auto&& Result = VI_AWAIT(Indirect);
							Future->Store((void*)&Result, TypeId);
						});

						return Future;
					}
				};
			};
#ifdef VI_HAS_BINDINGS
			class VI_OUT Mutex
			{
			private:
				std::recursive_mutex Base;
				int Ref;

			public:
				Mutex() noexcept;
				void AddRef();
				void Release();
				bool TryLock();
				void Lock();
				void Unlock();

			public:
				static Mutex* Factory();
			};

			class VI_OUT Thread
			{
			private:
				static int ContextUD;
				static int EngineListUD;

			private:
				struct
				{
					Core::Vector<Any*> Queue;
					std::condition_variable CV;
					std::mutex Mutex;
				} Pipe[2];

			private:
				Exception::Pointer Except;
				std::thread Procedure;
				std::recursive_mutex Mutex;
				asIScriptFunction* Function;
				VirtualMachine* VM;
				ImmediateContext* Context;
				bool Flag;
				int Sparcing;
				int RefCount;

			public:
				Thread(asIScriptEngine* Engine, asIScriptFunction* Function) noexcept;
				void EnumReferences(asIScriptEngine* Engine);
				void SetGCFlag();
				void ReleaseReferences(asIScriptEngine* Engine);
				void AddRef();
				void Release();
				void Suspend();
				void Resume();
				void Push(void* Ref, int TypeId);
				bool Pop(void* Ref, int TypeId);
				bool Pop(void* Ref, int TypeId, uint64_t Timeout);
				bool IsActive();
				bool Start();
				bool GetGCFlag();
				int GetRefCount();
				int Join(uint64_t Timeout);
				int Join();
				Core::String GetId() const;

			private:
				void InvokeRoutine();
				void ResumeRoutine();

			public:
				static void Create(asIScriptGeneric* Generic);
				static Thread* GetThread();
				static Core::String GetThreadId();
				static void ThreadSleep(uint64_t Mills);
				static void ThreadSuspend();
			};

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
				static void DefaultConstructor(Complex* self);
				static void CopyConstructor(const Complex& other, Complex* self);
				static void ConvConstructor(float r, Complex* self);
				static void InitConstructor(float r, float i, Complex* self);
				static void ListConstructor(float* list, Complex* self);
			};

			class VI_OUT Format final : public Core::Reference<Format>
			{
			public:
				Core::Vector<Core::String> Args;

			public:
				Format() noexcept;
				Format(unsigned char* Buffer) noexcept;

			public:
				static Core::String JSON(void* Ref, int TypeId);
				static Core::String Form(const Core::String& F, const Format& Form);
				static void WriteLine(Core::Console* Base, const Core::String& F, Format* Form);
				static void Write(Core::Console* Base, const Core::String& F, Format* Form);

			private:
				static void FormatBuffer(VirtualMachine* VM, Core::Stringify& Result, Core::String& Offset, void* Ref, int TypeId);
				static void FormatJSON(VirtualMachine* VM, Core::Stringify& Result, void* Ref, int TypeId);
			};

			class VI_OUT Application final : public Engine::Application
			{
			public:
				asIScriptFunction* OnScriptHook = nullptr;
				asIScriptFunction* OnKeyEvent = nullptr;
				asIScriptFunction* OnInputEvent = nullptr;
				asIScriptFunction* OnWheelEvent = nullptr;
				asIScriptFunction* OnWindowEvent = nullptr;
				asIScriptFunction* OnCloseEvent = nullptr;
				asIScriptFunction* OnComposeEvent = nullptr;
				asIScriptFunction* OnDispatch = nullptr;
				asIScriptFunction* OnPublish = nullptr;
				asIScriptFunction* OnInitialize = nullptr;
				asIScriptFunction* OnGetGUI = nullptr;
				ImmediateContext* Context;

			public:
				Application(Desc& I) noexcept;
				~Application() noexcept override;
				void ScriptHook() override;
				void KeyEvent(Graphics::KeyCode Key, Graphics::KeyMod Mod, int Virtual, int Repeat, bool Pressed) override;
				void InputEvent(char* Buffer, size_t Length) override;
				void WheelEvent(int X, int Y, bool Normal) override;
				void WindowEvent(Graphics::WindowState NewState, int X, int Y) override;
				void CloseEvent() override;
				void ComposeEvent() override;
				void Dispatch(Core::Timer* Time) override;
				void Publish(Core::Timer* Time) override;
				void Initialize() override;
				Engine::GUI::Context* GetGUI() const override;

			public:
				static bool WantsRestart(int ExitCode);
			};

			class VI_OUT ModelListener : public Core::Reference<ModelListener>
			{
			private:
				Engine::GUI::Listener* Base;
				asIScriptFunction* Source;
				ImmediateContext* Context;

			public:
				ModelListener(asIScriptFunction* NewCallback) noexcept;
				ModelListener(const Core::String& FunctionName) noexcept;
				~ModelListener() noexcept;
				asIScriptFunction* GetCallback();

			private:
				Engine::GUI::EventCallback Bind(asIScriptFunction* Callback);
			};
#endif
		}
	}
}
#endif