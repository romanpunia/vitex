#ifndef ED_BINDINGS_H
#define ED_BINDINGS_H
#include "script.h"
#include "../engine/gui.h"
#define ED_SHUFFLE(Name) Edge::Core::Shuffle<sizeof(Name)>(Name)
#define ED_TYPEREF(Name, TypeName) static const uint64_t Name = ED_SHUFFLE(TypeName); Edge::Script::Bindings::TypeCache::Set(Name, TypeName)
#define ED_PROMISIFY(MemberFunction, TypeId) Edge::Script::Bindings::Promise::Ify<decltype(&MemberFunction), &MemberFunction>::Id<TypeId>
#define ED_PROMISIFY_REF(MemberFunction, TypeRef) Edge::Script::Bindings::Promise::Ify<decltype(&MemberFunction), &MemberFunction>::Decl<TypeRef>
#define ED_SPROMISIFY(Function, TypeId) Edge::Script::Bindings::Promise::IfyStatic<decltype(&Function), &Function>::Id<TypeId>
#define ED_SPROMISIFY_REF(Function, TypeRef) Edge::Script::Bindings::Promise::IfyStatic<decltype(&Function), &Function>::Decl<TypeRef>
#define ED_ARRAYIFY(MemberFunction, TypeId) Edge::Script::Bindings::Array::Ify<decltype(&MemberFunction), &MemberFunction>::Id<TypeId>
#define ED_ARRAYIFY_REF(MemberFunction, TypeRef) Edge::Script::Bindings::Array::Ify<decltype(&MemberFunction), &MemberFunction>::Decl<TypeRef>
#define ED_SARRAYIFY(Function, TypeId) Edge::Script::Bindings::Array::IfyStatic<decltype(&Function), &Function>::Id<TypeId>
#define ED_SARRAYIFY_REF(Function, TypeRef) Edge::Script::Bindings::Array::IfyStatic<decltype(&Function), &Function>::Decl<TypeRef>
#define ED_ANYIFY(MemberFunction, TypeId) Edge::Script::Bindings::Any::Ify<decltype(&MemberFunction), &MemberFunction>::Id<TypeId>
#define ED_ANYIFY_REF(MemberFunction, TypeRef) Edge::Script::Bindings::Any::Ify<decltype(&MemberFunction), &MemberFunction>::Decl<TypeRef>
#define ED_SANYIFY(Function, TypeId) Edge::Script::Bindings::Any::IfyStatic<decltype(&Function), &Function>::Id<TypeId>
#define ED_SANYIFY_REF(Function, TypeRef) Edge::Script::Bindings::Any::IfyStatic<decltype(&Function), &Function>::Decl<TypeRef>

namespace Edge
{
	namespace Script
	{
		namespace Bindings
		{
			class Promise;

			class Array;

			class ED_OUT TypeCache
			{
			public:
				static uint64_t Set(uint64_t Id, const std::string& Name);
				static int GetTypeId(uint64_t Id);
			};

			class ED_OUT Exception
			{
			public:
				static void Throw(const std::string& In);
				static std::string GetException();
			};

			class ED_OUT String
			{
			public:
				static void Construct(std::string* thisPointer);
				static void CopyConstruct(const std::string& other, std::string* thisPointer);
				static void Destruct(std::string* thisPointer);
				static std::string& AddAssignTo(const std::string& str, std::string& dest);
				static bool IsEmpty(const std::string& str);
				static void* ToPtr(const std::string& Value);
				static std::string Reverse(const std::string& Value);
				static std::string& AssignUInt64To(as_uint64_t i, std::string& dest);
				static std::string& AddAssignUInt64To(as_uint64_t i, std::string& dest);
				static std::string AddUInt641(const std::string& str, as_uint64_t i);
				static std::string AddInt641(as_int64_t i, const std::string& str);
				static std::string& AssignInt64To(as_int64_t i, std::string& dest);
				static std::string& AddAssignInt64To(as_int64_t i, std::string& dest);
				static std::string AddInt642(const std::string& str, as_int64_t i);
				static std::string AddUInt642(as_uint64_t i, const std::string& str);
				static std::string& AssignDoubleTo(double f, std::string& dest);
				static std::string& AddAssignDoubleTo(double f, std::string& dest);
				static std::string& AssignFloatTo(float f, std::string& dest);
				static std::string& AddAssignFloatTo(float f, std::string& dest);
				static std::string& AssignBoolTo(bool b, std::string& dest);
				static std::string& AddAssignBoolTo(bool b, std::string& dest);
				static std::string AddDouble1(const std::string& str, double f);
				static std::string AddDouble2(double f, const std::string& str);
				static std::string AddFloat1(const std::string& str, float f);
				static std::string AddFloat2(float f, const std::string& str);
				static std::string AddBool1(const std::string& str, bool b);
				static std::string AddBool2(bool b, const std::string& str);
				static char* CharAt(size_t i, std::string& str);
				static int Cmp(const std::string& a, const std::string& b);
				static int FindFirst(const std::string& sub, size_t start, const std::string& str);
				static int FindFirstOf(const std::string& sub, size_t start, const std::string& str);
				static int FindLastOf(const std::string& sub, size_t start, const std::string& str);
				static int FindFirstNotOf(const std::string& sub, size_t start, const std::string& str);
				static int FindLastNotOf(const std::string& sub, size_t start, const std::string& str);
				static int FindLast(const std::string& sub, int start, const std::string& str);
				static void Insert(size_t pos, const std::string& other, std::string& str);
				static void Erase(size_t pos, int count, std::string& str);
				static size_t Length(const std::string& str);
				static void Resize(size_t l, std::string& str);
				static std::string Replace(const std::string& a, const std::string& b, size_t o, const std::string& base);
				static as_int64_t IntStore(const std::string& val, size_t base, size_t* byteCount);
				static as_uint64_t UIntStore(const std::string& val, size_t base, size_t* byteCount);
				static double FloatStore(const std::string& val, size_t* byteCount);
				static std::string Sub(size_t start, int count, const std::string& str);
				static bool Equals(const std::string& lhs, const std::string& rhs);
				static std::string ToLower(const std::string& Symbol);
				static std::string ToUpper(const std::string& Symbol);
				static std::string ToInt8(char Value);
				static std::string ToInt16(short Value);
				static std::string ToInt(int Value);
				static std::string ToInt64(int64_t Value);
				static std::string ToUInt8(unsigned char Value);
				static std::string ToUInt16(unsigned short Value);
				static std::string ToUInt(unsigned int Value);
				static std::string ToUInt64(uint64_t Value);
				static std::string ToFloat(float Value);
				static std::string ToDouble(double Value);
				static std::string ToPointer(void* Value);
				static Array* Split(const std::string& delim, const std::string& str);
				static std::string Join(const Array& array, const std::string& delim);
				static char ToChar(const std::string& Symbol);
			};

			class ED_OUT Mutex
			{
			private:
				std::mutex Base;
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

			class ED_OUT Math
			{
			public:
				static float FpFromIEEE(uint32_t raw);
				static uint32_t FpToIEEE(float fp);
				static double FpFromIEEE(as_uint64_t raw);
				static as_uint64_t FpToIEEE(double fp);
				static bool CloseTo(float a, float b, float epsilon);
				static bool CloseTo(double a, double b, double epsilon);
			};

			class ED_OUT Any
			{
				friend Promise;

			protected:
				struct ValueStruct
				{
					union
					{
						as_int64_t ValueInt;
						double ValueFlt;
						void* ValueObj;
					};
					int TypeId;
				};

			protected:
				mutable int RefCount;
				mutable bool GCFlag;
				VMCManager* Engine;
				ValueStruct Value;

			public:
				Any(VMCManager* Engine) noexcept;
				Any(void* Ref, int RefTypeId, VMCManager* Engine) noexcept;
				Any(const Any&) noexcept;
				int AddRef() const;
				int Release() const;
				Any& operator= (const Any&) noexcept;
				int CopyFrom(const Any* Other);
				void Store(void* Ref, int RefTypeId);
				bool Retrieve(void* Ref, int RefTypeId) const;
				int GetTypeId() const;
				int GetRefCount();
				void SetFlag();
				bool GetFlag();
				void EnumReferences(VMCManager* Engine);
				void ReleaseAllHandles(VMCManager* Engine);

			protected:
				virtual ~Any();
				void FreeObject();

			public:
				static Core::Unique<Any> Create();
				static Core::Unique<Any> Create(int TypeId, void* Ref);
				static Core::Unique<Any> Create(const char* Decl, void* Ref);
				static void Factory1(VMCGeneric* G);
				static void Factory2(VMCGeneric* G);
				static Any& Assignment(Any* Other, Any* Self);

			public:
				template <typename T, T>
				struct Ify;

				template <typename T, T>
				struct IfyStatic;

				template <typename T, typename R, typename ...Args, R(T::* F)(Args...)>
				struct Ify<R(T::*)(Args...), F>
				{
					template <VMTypeId TypeId>
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
					template <VMTypeId TypeId>
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

			class ED_OUT Array
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
				VMCTypeInfo* ObjType;
				SBuffer* Buffer;
				int ElementSize;
				int SubTypeId;

			public:
				void AddRef() const;
				void Release() const;
				VMCTypeInfo* GetArrayObjectType() const;
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
				void Sort(VMCFunction* Less, size_t StartAt, size_t Count);
				void Reverse();
				int Find(void* Value) const;
				int Find(size_t StartAt, void* Value) const;
				int FindByRef(void* Ref) const;
				int FindByRef(size_t StartAt, void* Ref) const;
				void* GetBuffer();
				int GetRefCount();
				void SetFlag();
				bool GetFlag();
				void EnumReferences(VMCManager* Engine);
				void ReleaseAllHandles(VMCManager* Engine);

			protected:
				Array(VMCTypeInfo* T, void* InitBuf) noexcept;
				Array(size_t Length, VMCTypeInfo* T) noexcept;
				Array(size_t Length, void* DefVal, VMCTypeInfo* T) noexcept;
				Array(const Array& Other) noexcept;
				virtual ~Array() noexcept;
				bool Less(const void* A, const void* B, bool Asc, VMCContext* Ctx, SCache* Cache);
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
				bool Equals(const void* A, const void* B, VMCContext* Ctx, SCache* Cache) const;

			public:
				static Core::Unique<Array> Create(VMCTypeInfo* T);
				static Core::Unique<Array> Create(VMCTypeInfo* T, size_t Length);
				static Core::Unique<Array> Create(VMCTypeInfo* T, size_t Length, void* DefaultValue);
				static Core::Unique<Array> Create(VMCTypeInfo* T, void* ListBuffer);
				static void CleanupTypeInfoCache(VMCTypeInfo* Type);
				static bool TemplateCallback(VMCTypeInfo* T, bool& DontGarbageCollect);

			public:
				template <typename T>
				static Array* Compose(VMCTypeInfo* ArrayType, const std::vector<T>& Objects)
				{
					Array* Array = Create(ArrayType, Objects.size());
					for (size_t i = 0; i < Objects.size(); i++)
						Array->SetValue((size_t)i, (void*)&Objects[i]);

					return Array;
				}
				template <typename T>
				static typename std::enable_if<std::is_pointer<T>::value, std::vector<T>>::type Decompose(Array* Array)
				{
					std::vector<T> Result;
					if (!Array)
						return Result;

					size_t Size = Array->GetSize();
					Result.reserve(Size);

					for (size_t i = 0; i < Size; i++)
						Result.push_back((T)Array->At(i));

					return Result;
				}
				template <typename T>
				static typename std::enable_if<!std::is_pointer<T>::value, std::vector<T>>::type Decompose(Array* Array)
				{
					std::vector<T> Result;
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

				template <typename T, typename R, typename ...Args, std::vector<R>(T::* F)(Args...)>
				struct Ify<std::vector<R>(T::*)(Args...), F>
				{
					template <VMTypeId TypeId>
					static Array* Id(T* Base, Args... Data)
					{
						VMManager* Manager = VMManager::Get();
						ED_ASSERT(Manager != nullptr, nullptr, "manager should be present");

						VMCTypeInfo* Info = Manager->Global().GetTypeInfoById((int)TypeId).GetTypeInfo();
						ED_ASSERT(Info != nullptr, nullptr, "typeinfo should be valid");

						std::vector<R> Source((Base->*F)(Data...));
						return Array::Compose(Info, Source);
					}
					template <uint64_t TypeRef>
					static Array* Decl(T* Base, Args... Data)
					{
						VMManager* Manager = VMManager::Get();
						ED_ASSERT(Manager != nullptr, nullptr, "manager should be present");

						VMCTypeInfo* Info = Manager->Global().GetTypeInfoById(TypeCache::GetTypeId(TypeRef)).GetTypeInfo();
						ED_ASSERT(Info != nullptr, nullptr, "typeinfo should be valid");

						std::vector<R> Source((Base->*F)(Data...));
						return Array::Compose(Info, Source);
					}
				};

				template <typename R, typename ...Args, std::vector<R>(*F)(Args...)>
				struct IfyStatic<std::vector<R>(*)(Args...), F>
				{
					template <VMTypeId TypeId>
					static Array* Id(Args... Data)
					{
						VMManager* Manager = VMManager::Get();
						ED_ASSERT(Manager != nullptr, nullptr, "manager should be present");

						VMCTypeInfo* Info = Manager->Global().GetTypeInfoById((int)TypeId).GetTypeInfo();
						ED_ASSERT(Info != nullptr, nullptr, "typeinfo should be valid");

						std::vector<R> Source((*F)(Data...));
						return Array::Compose(Info, Source);
					}
					template <uint64_t TypeRef>
					static Array* Decl(Args... Data)
					{
						VMManager* Manager = VMManager::Get();
						ED_ASSERT(Manager != nullptr, nullptr, "manager should be present");

						VMCTypeInfo* Info = Manager->Global().GetTypeInfoById(TypeCache::GetTypeId(TypeRef)).GetTypeInfo();
						ED_ASSERT(Info != nullptr, nullptr, "typeinfo should be valid");

						std::vector<R> Source((*F)(Data...));
						return Array::Compose(Info, Source);
					}
				};
			};

			class ED_OUT MapKey
			{
			protected:
				friend class Map;

			protected:
				union
				{
					as_int64_t ValueInt;
					double ValueFlt;
					void* ValueObj;
				};
				int TypeId;

			public:
				MapKey() noexcept;
				MapKey(VMCManager* Engine, void* Value, int TypeId) noexcept;
				~MapKey() noexcept;
				void Set(VMCManager* Engine, void* Value, int TypeId);
				void Set(VMCManager* Engine, MapKey& Value);
				bool Get(VMCManager* Engine, void* Value, int TypeId) const;
				const void* GetAddressOfValue() const;
				int GetTypeId() const;
				void FreeValue(VMCManager* Engine);
				void EnumReferences(VMCManager* Engine);
			};

			class ED_OUT Map
			{
			public:
				typedef std::unordered_map<std::string, MapKey> InternalMap;

			public:
				class LocalIterator
				{
				protected:
					friend class Map;

				protected:
					InternalMap::const_iterator It;
					const Map& Dict;

				public:
					void operator++();
					void operator++(int);
					LocalIterator& operator*();
					bool operator==(const LocalIterator& Other) const;
					bool operator!=(const LocalIterator& Other) const;
					const std::string& GetKey() const;
					int GetTypeId() const;
					bool GetValue(void* Value, int TypeId) const;
					const void* GetAddressOfValue() const;

				protected:
					LocalIterator() noexcept;
					LocalIterator(const Map& Dict, InternalMap::const_iterator It) noexcept;
					LocalIterator& operator= (const LocalIterator&) noexcept
					{
						return *this;
					}
				};

				struct SCache
				{
					VMCTypeInfo* DictType;
					VMCTypeInfo* ArrayType;
					VMCTypeInfo* KeyType;
				};

			protected:
				VMCManager* Engine;
				mutable int RefCount;
				mutable bool GCFlag;
				InternalMap Dict;

			public:
				void AddRef() const;
				void Release() const;
				Map& operator= (const Map& Other) noexcept;
				void Set(const std::string& Key, void* Value, int TypeId);
				bool Get(const std::string& Key, void* Value, int TypeId) const;
				bool GetIndex(size_t Index, std::string* Key, void** Value, int* TypeId) const;
				MapKey* operator[](const std::string& Key);
				const MapKey* operator[](const std::string& Key) const;
				int GetTypeId(const std::string& Key) const;
				bool Exists(const std::string& Key) const;
				bool IsEmpty() const;
				size_t GetSize() const;
				bool Delete(const std::string& Key);
				void DeleteAll();
				Array* GetKeys() const;
				LocalIterator Begin() const;
				LocalIterator End() const;
				LocalIterator Find(const std::string& Key) const;
				int GetRefCount();
				void SetGCFlag();
				bool GetGCFlag();
				void EnumReferences(VMCManager* Engine);
				void ReleaseAllReferences(VMCManager* Engine);

			protected:
				Map(VMCManager* Engine) noexcept;
				Map(unsigned char* Buffer) noexcept;
				Map(const Map&) noexcept;
				virtual ~Map() noexcept;
				void Init(VMCManager* Engine);

			public:
				static Core::Unique<Map> Create(VMCManager* Engine);
				static Core::Unique<Map> Create(unsigned char* Buffer);
				static void Cleanup(VMCManager* engine);
				static void Setup(VMCManager* engine);
				static void Factory(VMCGeneric* gen);
				static void ListFactory(VMCGeneric* gen);
				static void KeyConstruct(void* mem);
				static void KeyDestruct(MapKey* obj);
				static MapKey& KeyopAssign(void* ref, int typeId, MapKey* obj);
				static MapKey& KeyopAssign(const MapKey& other, MapKey* obj);
				static MapKey& KeyopAssign(double val, MapKey* obj);
				static MapKey& KeyopAssign(as_int64_t val, MapKey* obj);
				static void KeyopCast(void* ref, int typeId, MapKey* obj);
				static as_int64_t KeyopConvInt(MapKey* obj);
				static double KeyopConvDouble(MapKey* obj);

			public:
				template <typename T>
				static Map* Compose(int TypeId, const std::unordered_map<std::string, T>& Objects)
				{
					auto* Engine = VMManager::Get();
					Map* Data = Create(Engine ? Engine->GetEngine() : nullptr);
					for (auto& Item : Objects)
						Data->Set(Item.first, (void*)&Item.second, TypeId);

					return Data;
				}
				template <typename T>
				static typename std::enable_if<std::is_pointer<T>::value, std::unordered_map<std::string, T>>::type Decompose(int TypeId, Map* Array)
				{
					std::unordered_map<std::string, T> Result;
					Result.reserve(Array->GetSize());

					int SubTypeId = 0;
					size_t Size = Array->GetSize();
					for (size_t i = 0; i < Size; i++)
					{
						std::string Key; void* Value = nullptr;
						if (Array->GetIndex(i, &Key, &Value, &SubTypeId) && SubTypeId == TypeId)
							Result[Key] = (T*)Value;
					}

					return Result;
				}
				template <typename T>
				static typename std::enable_if<!std::is_pointer<T>::value, std::unordered_map<std::string, T>>::type Decompose(int TypeId, Map* Array)
				{
					std::unordered_map<std::string, T> Result;
					Result.reserve(Array->GetSize());

					int SubTypeId = 0;
					size_t Size = Array->GetSize();
					for (size_t i = 0; i < Size; i++)
					{
						std::string Key; void* Value = nullptr;
						if (Array->GetIndex(i, &Key, &Value, &SubTypeId) && SubTypeId == TypeId)
							Result[Key] = *(T*)Value;
					}

					return Result;
				}
			};

			class ED_OUT Grid
			{
			public:
				struct SBuffer
				{
					size_t Width;
					size_t Height;
					unsigned char Data[1];
				};

			protected:
				mutable int RefCount;
				mutable bool GCFlag;
				VMCTypeInfo* ObjType;
				SBuffer* Buffer;
				int ElementSize;
				int SubTypeId;

			public:
				void AddRef() const;
				void Release() const;
				VMCTypeInfo* GetGridObjectType() const;
				int GetGridTypeId() const;
				int GetElementTypeId() const;
				size_t GetWidth() const;
				size_t GetHeight() const;
				void Resize(size_t Width, size_t Height);
				void* At(size_t X, size_t Y);
				const void* At(size_t X, size_t Y) const;
				void  SetValue(size_t X, size_t Y, void* Value);
				int GetRefCount();
				void SetFlag();
				bool GetFlag();
				void EnumReferences(VMCManager* Engine);
				void ReleaseAllHandles(VMCManager* Engine);

			protected:
				Grid(VMCTypeInfo* T, void* InitBuf) noexcept;
				Grid(size_t W, size_t H, VMCTypeInfo* T) noexcept;
				Grid(size_t W, size_t H, void* DefVal, VMCTypeInfo* T) noexcept;
				virtual ~Grid() noexcept;
				bool CheckMaxSize(size_t X, size_t Y);
				void CreateBuffer(SBuffer** Buf, size_t W, size_t H);
				void DeleteBuffer(SBuffer* Buf);
				void Construct(SBuffer* Buf);
				void Destruct(SBuffer* Buf);
				void SetValue(SBuffer* Buf, size_t X, size_t Y, void* Value);
				void* At(SBuffer* Buf, size_t X, size_t Y);

			public:
				static Core::Unique<Grid> Create(VMCTypeInfo* T);
				static Core::Unique<Grid> Create(VMCTypeInfo* T, size_t Width, size_t Height);
				static Core::Unique<Grid> Create(VMCTypeInfo* T, size_t Width, size_t Height, void* DefaultValue);
				static Core::Unique<Grid> Create(VMCTypeInfo* T, void* ListBuffer);
				static bool TemplateCallback(VMCTypeInfo* TI, bool& DontGarbageCollect);
			};

			class ED_OUT Ref
			{
			protected:
				VMCTypeInfo* Type;
				void* Pointer;

			public:
				Ref() noexcept;
				Ref(const Ref& Other) noexcept;
				Ref(void* Ref, VMCTypeInfo* Type) noexcept;
				Ref(void* Ref, int TypeId) noexcept;
				~Ref() noexcept;
				Ref& operator=(const Ref& Other) noexcept;
				void Set(void* Ref, VMCTypeInfo* Type);
				bool operator== (const Ref& Other) const;
				bool operator!= (const Ref& Other) const;
				bool Equals(void* Ref, int TypeId) const;
				void Cast(void** OutRef, int TypeId);
				VMCTypeInfo* GetType() const;
				int GetTypeId() const;
				void* GetRef();
				void EnumReferences(VMCManager* Engine);
				void ReleaseReferences(VMCManager* Engine);
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

			class ED_OUT WeakRef
			{
			protected:
				VMCLockableSharedBool* WeakRefFlag;
				VMCTypeInfo* Type;
				void* Ref;

			public:
				WeakRef(VMCTypeInfo* Type) noexcept;
				WeakRef(const WeakRef& Other) noexcept;
				WeakRef(void* Ref, VMCTypeInfo* Type) noexcept;
				~WeakRef() noexcept;
				WeakRef& operator= (const WeakRef& Other) noexcept;
				bool operator== (const WeakRef& Other) const;
				bool operator!= (const WeakRef& Other) const;
				WeakRef& Set(void* NewRef);
				void* Get() const;
				bool Equals(void* Ref) const;
				VMCTypeInfo* GetRefType() const;

			public:
				static void Construct(VMCTypeInfo* type, void* mem);
				static void Construct2(VMCTypeInfo* type, void* ref, void* mem);
				static void Destruct(WeakRef* obj);
				static bool TemplateCallback(VMCTypeInfo* TI, bool&);
			};

			class ED_OUT Complex
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

			class ED_OUT Thread
			{
			private:
				static int ContextUD;
				static int EngineListUD;

			private:
				struct
				{
					std::vector<Any*> Queue;
					std::condition_variable CV;
					std::mutex Mutex;
				} Pipe[2];

			private:
				std::condition_variable CV;
				std::thread Procedure;
				std::mutex Mutex;
				VMCFunction* Function;
				VMManager* Manager;
				VMContext* Context;
				bool GCFlag;
				int Ref;

			public:
				Thread(VMCManager* Engine, VMCFunction* Function) noexcept;
				void EnumReferences(VMCManager* Engine);
				void SetGCFlag();
				void ReleaseReferences(VMCManager* Engine);
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
				std::string GetId() const;

			private:
				void Routine();

			public:
				static void Create(VMCGeneric* Generic);
				static Thread* GetThread();
				static std::string GetThreadId();
				static void ThreadSleep(uint64_t Mills);
				static void ThreadSuspend();
			};

			class ED_OUT Random
			{
			public:
				static std::string Getb(uint64_t Size);
				static double Betweend(double Min, double Max);
				static double Magd();
				static double Getd();
				static float Betweenf(float Min, float Max);
				static float Magf();
				static float Getf();
				static uint64_t Betweeni(uint64_t Min, uint64_t Max);
			};

			class ED_OUT Promise
			{
				friend VMContext;

			private:
				static int PromiseUD;

			private:
				VMCManager* Engine;
				VMContext* Context;
				Any* Future;
				std::mutex Update;
				bool Pending;
				bool Flag;
				int Ref;

			private:
				Promise(VMCContext* Base, bool IsRef) noexcept;

			public:
				void EnumReferences(VMCManager* Engine);
				void ReleaseReferences(VMCManager* Engine);
				void AddRef();
				void Release();
				void SetGCFlag();
				bool GetGCFlag();
				int GetRefCount();
				int Set(void* Ref, int TypeId);
				int Set(void* Ref, const char* TypeId);
				bool To(void* fRef, int TypeId);
				void* GetHandle();
				void* GetValue();

			private:
				int Notify();

			public:
				static std::string GetStatus(VMContext* Context);

			private:
				static Core::Unique<Promise> Create();
				static Core::Unique<Promise> CreateRef();
				static Promise* JumpIf(Promise* Base);

			public:
				template <typename T, T>
				struct Ify;

				template <typename T, T>
				struct IfyStatic;

				template <typename T, typename R, typename ...Args, Core::Promise<R>(T::* F)(Args...)>
				struct Ify<Core::Promise<R>(T::*)(Args...), F>
				{
					template <VMTypeId TypeId>
					static Promise* Id(T* Base, Args... Data)
					{
						Promise* Future = Promise::Create();
						((Base->*F)(Data...)).Await([Future](R&& Result)
						{
							Future->Set((void*)&Result, (int)TypeId);
						});

						return JumpIf(Future);
					}
					template <uint64_t TypeRef>
					static Promise* Decl(T* Base, Args... Data)
					{
						Promise* Future = Promise::CreateRef();
						int TypeId = TypeCache::GetTypeId(TypeRef);
						((Base->*F)(Data...)).Await([Future, TypeId](R&& Result)
						{
							Future->Set((void*)&Result, TypeId);
						});

						return JumpIf(Future);
					}
				};

				template <typename R, typename ...Args, Core::Promise<R>(*F)(Args...)>
				struct IfyStatic<Core::Promise<R>(*)(Args...), F>
				{
					template <VMTypeId TypeId>
					static Promise* Id(Args... Data)
					{
						Promise* Future = Promise::Create();
						((*F)(Data...)).Await([Future](R&& Result)
						{
							Future->Set((void*)&Result, (int)TypeId);
						});

						return JumpIf(Future);
					}
					template <uint64_t TypeRef>
					static Promise* Decl(Args... Data)
					{
						Promise* Future = Promise::CreateRef();
						int TypeId = TypeCache::GetTypeId(TypeRef);
						((*F)(Data...)).Await([Future, TypeId](R&& Result)
						{
							Future->Set((void*)&Result, TypeId);
						});

						return JumpIf(Future);
					}
				};
			};

			class ED_OUT Format : public Core::Object
			{
			public:
				std::vector<std::string> Args;

			public:
				Format() noexcept;
				Format(unsigned char* Buffer) noexcept;

			public:
				static std::string JSON(void* Ref, int TypeId);
				static std::string Form(const std::string& F, const Format& Form);
				static void WriteLine(Core::Console* Base, const std::string& F, Format* Form);
				static void Write(Core::Console* Base, const std::string& F, Format* Form);

			private:
				static void FormatBuffer(VMGlobal& Global, Core::Parser& Result, std::string& Offset, void* Ref, int TypeId);
				static void FormatJSON(VMGlobal& Global, Core::Parser& Result, void* Ref, int TypeId);
			};

			class ED_OUT ModelListener : public Engine::GUI::Listener
			{
			private:
				VMCFunction* Source;
				VMContext* Context;

			public:
				ModelListener(VMCFunction* NewCallback) noexcept;
				ModelListener(const std::string& FunctionName) noexcept;
				virtual ~ModelListener() noexcept override;

			private:
				Engine::GUI::EventCallback Bind(VMCFunction* Callback);
			};

			class ED_OUT_TS Registry
			{
			public:
				static bool LoadCTypes(VMManager* Manager);
				static bool LoadAny(VMManager* Manager);
				static bool LoadArray(VMManager* Manager);
				static bool LoadComplex(VMManager* Manager);
				static bool LoadMap(VMManager* Manager);
				static bool LoadGrid(VMManager* Manager);
				static bool LoadRef(VMManager* Manager);
				static bool LoadWeakRef(VMManager* Manager);
				static bool LoadMath(VMManager* Manager);
				static bool LoadString(VMManager* Manager);
				static bool LoadException(VMManager* Manager);
				static bool LoadMutex(VMManager* Manager);
				static bool LoadThread(VMManager* Manager);
				static bool LoadRandom(VMManager* Manager);
				static bool LoadPromise(VMManager* Manager);
				static bool LoadFormat(VMManager* Engine);
				static bool LoadDecimal(VMManager* Engine);
				static bool LoadVariant(VMManager* Engine);
				static bool LoadTimestamp(VMManager* Engine);
				static bool LoadConsole(VMManager* Engine);
				static bool LoadSchema(VMManager* Engine);
				static bool LoadTickClock(VMManager* Engine);
				static bool LoadFileSystem(VMManager* Engine);
				static bool LoadOS(VMManager* Engine);
				static bool LoadSchedule(VMManager* Engine);
				static bool LoadVertices(VMManager* Engine);
				static bool LoadVectors(VMManager* Engine);
				static bool LoadShapes(VMManager* Engine);
				static bool LoadKeyFrames(VMManager* Engine);
				static bool LoadRegex(VMManager* Engine);
				static bool LoadCrypto(VMManager* Engine);
				static bool LoadGeometric(VMManager* Engine);
				static bool LoadPreprocessor(VMManager* Engine);
				static bool LoadPhysics(VMManager* Engine);
				static bool LoadAudio(VMManager* Engine);
				static bool LoadActivity(VMManager* Engine);
				static bool LoadGraphics(VMManager* Engine);
				static bool LoadNetwork(VMManager* Engine);
				static bool LoadUiModel(VMManager* Engine);
				static bool LoadUiControl(VMManager* Engine);
				static bool LoadUiContext(VMManager* Engine);
				static bool Release();
			};
		}
	}
}
#endif