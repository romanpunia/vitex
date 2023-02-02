#ifndef TH_BINDINGS_H
#define TH_BINDINGS_H
#include "script.h"
#include "../engine/gui.h"
#define TH_SHUFFLE(Name) Tomahawk::Core::Shuffle<sizeof(Name)>(Name)
#define TH_TYPEREF(Name, TypeName) static const uint64_t Name = TH_SHUFFLE(TypeName); Tomahawk::Script::Bindings::TypeCache::Set(Name, TypeName)
#define TH_PROMISIFY(MemberFunction, TypeId) Tomahawk::Script::Bindings::Promise::Ify<decltype(&MemberFunction), &MemberFunction>::Id<TypeId>
#define TH_PROMISIFY_REF(MemberFunction, TypeRef) Tomahawk::Script::Bindings::Promise::Ify<decltype(&MemberFunction), &MemberFunction>::Decl<TypeRef>
#define TH_SPROMISIFY(Function, TypeId) Tomahawk::Script::Bindings::Promise::IfyStatic<decltype(&Function), &Function>::Id<TypeId>
#define TH_SPROMISIFY_REF(Function, TypeRef) Tomahawk::Script::Bindings::Promise::IfyStatic<decltype(&Function), &Function>::Decl<TypeRef>
#define TH_ARRAYIFY(MemberFunction, TypeId) Tomahawk::Script::Bindings::Array::Ify<decltype(&MemberFunction), &MemberFunction>::Id<TypeId>
#define TH_ARRAYIFY_REF(MemberFunction, TypeRef) Tomahawk::Script::Bindings::Array::Ify<decltype(&MemberFunction), &MemberFunction>::Decl<TypeRef>
#define TH_SARRAYIFY(Function, TypeId) Tomahawk::Script::Bindings::Array::IfyStatic<decltype(&Function), &Function>::Id<TypeId>
#define TH_SARRAYIFY_REF(Function, TypeRef) Tomahawk::Script::Bindings::Array::IfyStatic<decltype(&Function), &Function>::Decl<TypeRef>
#define TH_ANYIFY(MemberFunction, TypeId) Tomahawk::Script::Bindings::Any::Ify<decltype(&MemberFunction), &MemberFunction>::Id<TypeId>
#define TH_ANYIFY_REF(MemberFunction, TypeRef) Tomahawk::Script::Bindings::Any::Ify<decltype(&MemberFunction), &MemberFunction>::Decl<TypeRef>
#define TH_SANYIFY(Function, TypeId) Tomahawk::Script::Bindings::Any::IfyStatic<decltype(&Function), &Function>::Id<TypeId>
#define TH_SANYIFY_REF(Function, TypeRef) Tomahawk::Script::Bindings::Any::IfyStatic<decltype(&Function), &Function>::Decl<TypeRef>

namespace Tomahawk
{
	namespace Script
	{
		namespace Bindings
		{
			class Promise;

			class Array;

			class TH_OUT TypeCache
			{
			public:
				static uint64_t Set(uint64_t Id, const std::string& Name);
				static int GetTypeId(uint64_t Id);
			};

			class TH_OUT Exception
			{
			public:
				static void Throw(const std::string& In);
				static std::string GetException();
			};

			class TH_OUT String
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
				static char* CharAt(unsigned int i, std::string& str);
				static int Cmp(const std::string& a, const std::string& b);
				static int FindFirst(const std::string& sub, as_size_t start, const std::string& str);
				static int FindFirstOf(const std::string& sub, as_size_t start, const std::string& str);
				static int FindLastOf(const std::string& sub, as_size_t start, const std::string& str);
				static int FindFirstNotOf(const std::string& sub, as_size_t start, const std::string& str);
				static int FindLastNotOf(const std::string& sub, as_size_t start, const std::string& str);
				static int FindLast(const std::string& sub, int start, const std::string& str);
				static void Insert(unsigned int pos, const std::string& other, std::string& str);
				static void Erase(unsigned int pos, int count, std::string& str);
				static as_size_t Length(const std::string& str);
				static void Resize(as_size_t l, std::string& str);
				static std::string Replace(const std::string& a, const std::string& b, uint64_t o, const std::string& base);
				static as_int64_t IntStore(const std::string& val, as_size_t base, as_size_t* byteCount);
				static as_uint64_t UIntStore(const std::string& val, as_size_t base, as_size_t* byteCount);
				static double FloatStore(const std::string& val, as_size_t* byteCount);
				static std::string Sub(as_size_t start, int count, const std::string& str);
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
				static Array* Split(const std::string& delim, const std::string& str);
				static std::string Join(const Array& array, const std::string& delim);
				static char ToChar(const std::string& Symbol);
			};

			class TH_OUT Mutex
			{
			private:
				std::mutex Base;
				int Ref;

			public:
				Mutex();
				void AddRef();
				void Release();
				bool TryLock();
				void Lock();
				void Unlock();

			public:
				static Mutex* Factory();
			};

			class TH_OUT Math
			{
			public:
				static float FpFromIEEE(as_size_t raw);
				static as_size_t FpToIEEE(float fp);
				static double FpFromIEEE(as_uint64_t raw);
				static as_uint64_t FpToIEEE(double fp);
				static bool CloseTo(float a, float b, float epsilon);
				static bool CloseTo(double a, double b, double epsilon);
			};

			class TH_OUT Any
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
				Any(VMCManager* Engine);
				Any(void* Ref, int RefTypeId, VMCManager* Engine);
				Any(const Any&);
				int AddRef() const;
				int Release() const;
				Any& operator= (const Any&);
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

			class TH_OUT Array
			{
			public:
				struct SBuffer
				{
					as_size_t MaxElements;
					as_size_t NumElements;
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
				as_size_t GetSize() const;
				bool IsEmpty() const;
				void Reserve(as_size_t MaxElements);
				void Resize(as_size_t NumElements);
				void* At(as_size_t Index);
				const void* At(as_size_t Index) const;
				void SetValue(as_size_t Index, void* Value);
				Array& operator= (const Array&);
				bool operator== (const Array&) const;
				void InsertAt(as_size_t Index, void* Value);
				void InsertAt(as_size_t Index, const Array& Other);
				void InsertLast(void* Value);
				void RemoveAt(as_size_t Index);
				void RemoveLast();
				void RemoveRange(as_size_t start, as_size_t Count);
				void SortAsc();
				void SortDesc();
				void SortAsc(as_size_t StartAt, as_size_t Count);
				void SortDesc(as_size_t StartAt, as_size_t Count);
				void Sort(as_size_t StartAt, as_size_t Count, bool Asc);
				void Sort(VMCFunction* Less, as_size_t StartAt, as_size_t Count);
				void Reverse();
				int Find(void* Value) const;
				int Find(as_size_t StartAt, void* Value) const;
				int FindByRef(void* Ref) const;
				int FindByRef(as_size_t StartAt, void* Ref) const;
				void* GetBuffer();
				int GetRefCount();
				void SetFlag();
				bool GetFlag();
				void EnumReferences(VMCManager* Engine);
				void ReleaseAllHandles(VMCManager* Engine);

			protected:
				Array(VMCTypeInfo* T, void* InitBuf);
				Array(as_size_t Length, VMCTypeInfo* T);
				Array(as_size_t Length, void* DefVal, VMCTypeInfo* T);
				Array(const Array& Other);
				virtual ~Array();
				bool Less(const void* A, const void* B, bool Asc, VMCContext* Ctx, SCache* Cache);
				void* GetArrayItemPointer(int Index);
				void* GetDataPointer(void* Buffer);
				void Copy(void* Dst, void* Src);
				void Precache();
				bool CheckMaxSize(as_size_t NumElements);
				void Resize(int Delta, as_size_t At);
				void CreateBuffer(SBuffer** Buf, as_size_t NumElements);
				void DeleteBuffer(SBuffer* Buf);
				void CopyBuffer(SBuffer* Dst, SBuffer* Src);
				void Construct(SBuffer* Buf, as_size_t Start, as_size_t End);
				void Destruct(SBuffer* Buf, as_size_t Start, as_size_t End);
				bool Equals(const void* A, const void* B, VMCContext* Ctx, SCache* Cache) const;

			public:
				static Core::Unique<Array> Create(VMCTypeInfo* T);
				static Core::Unique<Array> Create(VMCTypeInfo* T, as_size_t Length);
				static Core::Unique<Array> Create(VMCTypeInfo* T, as_size_t Length, void* DefaultValue);
				static Core::Unique<Array> Create(VMCTypeInfo* T, void* ListBuffer);
				static void CleanupTypeInfoCache(VMCTypeInfo* Type);
				static bool TemplateCallback(VMCTypeInfo* T, bool& DontGarbageCollect);

			public:
				template <typename T>
				static Array* Compose(VMCTypeInfo* ArrayType, const std::vector<T>& Objects)
				{
					Array* Array = Create(ArrayType, (unsigned int)Objects.size());
					for (size_t i = 0; i < Objects.size(); i++)
						Array->SetValue((as_size_t)i, (void*)&Objects[i]);

					return Array;
				}
				template <typename T>
				static typename std::enable_if<std::is_pointer<T>::value, std::vector<T>>::type Decompose(Array* Array)
				{
					std::vector<T> Result;
					if (!Array)
						return Result;

					unsigned int Size = Array->GetSize();
					Result.reserve(Size);

					for (unsigned int i = 0; i < Size; i++)
						Result.push_back((T)Array->At(i));

					return Result;
				}
				template <typename T>
				static typename std::enable_if<!std::is_pointer<T>::value, std::vector<T>>::type Decompose(Array* Array)
				{
					std::vector<T> Result;
					if (!Array)
						return Result;

					unsigned int Size = Array->GetSize();
					Result.reserve(Size);

					for (unsigned int i = 0; i < Size; i++)
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
						TH_ASSERT(Manager != nullptr, nullptr, "manager should be present");

						VMCTypeInfo* Info = Manager->Global().GetTypeInfoById((int)TypeId).GetTypeInfo();
						TH_ASSERT(Info != nullptr, nullptr, "typeinfo should be valid");

						std::vector<R> Source((Base->*F)(Data...));
						return Array::Compose(Info, Source);
					}
					template <uint64_t TypeRef>
					static Array* Decl(T* Base, Args... Data)
					{
						VMManager* Manager = VMManager::Get();
						TH_ASSERT(Manager != nullptr, nullptr, "manager should be present");

						VMCTypeInfo* Info = Manager->Global().GetTypeInfoById(TypeCache::GetTypeId(TypeRef)).GetTypeInfo();
						TH_ASSERT(Info != nullptr, nullptr, "typeinfo should be valid");

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
						TH_ASSERT(Manager != nullptr, nullptr, "manager should be present");

						VMCTypeInfo* Info = Manager->Global().GetTypeInfoById((int)TypeId).GetTypeInfo();
						TH_ASSERT(Info != nullptr, nullptr, "typeinfo should be valid");

						std::vector<R> Source((*F)(Data...));
						return Array::Compose(Info, Source);
					}
					template <uint64_t TypeRef>
					static Array* Decl(Args... Data)
					{
						VMManager* Manager = VMManager::Get();
						TH_ASSERT(Manager != nullptr, nullptr, "manager should be present");

						VMCTypeInfo* Info = Manager->Global().GetTypeInfoById(TypeCache::GetTypeId(TypeRef)).GetTypeInfo();
						TH_ASSERT(Info != nullptr, nullptr, "typeinfo should be valid");

						std::vector<R> Source((*F)(Data...));
						return Array::Compose(Info, Source);
					}
				};
			};

			class TH_OUT MapKey
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
				MapKey();
				MapKey(VMCManager* Engine, void* Value, int TypeId);
				~MapKey();
				void Set(VMCManager* Engine, void* Value, int TypeId);
				void Set(VMCManager* Engine, MapKey& Value);
				bool Get(VMCManager* Engine, void* Value, int TypeId) const;
				const void* GetAddressOfValue() const;
				int GetTypeId() const;
				void FreeValue(VMCManager* Engine);
				void EnumReferences(VMCManager* Engine);
			};

			class TH_OUT Map
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
					LocalIterator();
					LocalIterator(const Map& Dict, InternalMap::const_iterator It);
					LocalIterator& operator= (const LocalIterator&)
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
				Map& operator= (const Map& Other);
				void Set(const std::string& Key, void* Value, int TypeId);
				bool Get(const std::string& Key, void* Value, int TypeId) const;
				bool GetIndex(size_t Index, std::string* Key, void** Value, int* TypeId) const;
				MapKey* operator[](const std::string& Key);
				const MapKey* operator[](const std::string& Key) const;
				int GetTypeId(const std::string& Key) const;
				bool Exists(const std::string& Key) const;
				bool IsEmpty() const;
				as_size_t GetSize() const;
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
				Map(VMCManager* Engine);
				Map(unsigned char* Buffer);
				Map(const Map&);
				virtual ~Map();
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
			};

			class TH_OUT Grid
			{
			public:
				struct SBuffer
				{
					as_size_t Width;
					as_size_t Height;
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
				as_size_t GetWidth() const;
				as_size_t GetHeight() const;
				void Resize(as_size_t Width, as_size_t Height);
				void* At(as_size_t X, as_size_t Y);
				const void* At(as_size_t X, as_size_t Y) const;
				void  SetValue(as_size_t X, as_size_t Y, void* Value);
				int GetRefCount();
				void SetFlag();
				bool GetFlag();
				void EnumReferences(VMCManager* Engine);
				void ReleaseAllHandles(VMCManager* Engine);

			protected:
				Grid(VMCTypeInfo* T, void* InitBuf);
				Grid(as_size_t W, as_size_t H, VMCTypeInfo* T);
				Grid(as_size_t W, as_size_t H, void* DefVal, VMCTypeInfo* T);
				virtual ~Grid();
				bool CheckMaxSize(as_size_t X, as_size_t Y);
				void CreateBuffer(SBuffer** Buf, as_size_t W, as_size_t H);
				void DeleteBuffer(SBuffer* Buf);
				void Construct(SBuffer* Buf);
				void Destruct(SBuffer* Buf);
				void SetValue(SBuffer* Buf, as_size_t X, as_size_t Y, void* Value);
				void* At(SBuffer* Buf, as_size_t X, as_size_t Y);

			public:
				static Core::Unique<Grid> Create(VMCTypeInfo* T);
				static Core::Unique<Grid> Create(VMCTypeInfo* T, as_size_t Width, as_size_t Height);
				static Core::Unique<Grid> Create(VMCTypeInfo* T, as_size_t Width, as_size_t Height, void* DefaultValue);
				static Core::Unique<Grid> Create(VMCTypeInfo* T, void* ListBuffer);
				static bool TemplateCallback(VMCTypeInfo* TI, bool& DontGarbageCollect);
			};

			class TH_OUT Ref
			{
			protected:
				VMCTypeInfo* Type;
				void* Pointer;

			public:
				Ref();
				Ref(const Ref& Other);
				Ref(void* Ref, VMCTypeInfo* Type);
				Ref(void* Ref, int TypeId);
				~Ref();
				Ref& operator=(const Ref& Other);
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

			class TH_OUT WeakRef
			{
			protected:
				VMCLockableSharedBool* WeakRefFlag;
				VMCTypeInfo* Type;
				void* Ref;

			public:
				WeakRef(VMCTypeInfo* Type);
				WeakRef(const WeakRef& Other);
				WeakRef(void* Ref, VMCTypeInfo* Type);
				~WeakRef();
				WeakRef& operator= (const WeakRef& Other);
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

			class TH_OUT Complex
			{
			public:
				float R;
				float I;

			public:
				Complex();
				Complex(const Complex& Other);
				Complex(float R, float I = 0);
				Complex& operator= (const Complex& Other);
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

			class TH_OUT Thread
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
				Thread(VMCManager* Engine, VMCFunction* Function);
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

			private:
				void Routine();

			public:
				static void Create(VMCGeneric* Generic);
				static Thread* GetThread();
				static uint64_t GetThreadId();
				static void ThreadSleep(uint64_t Timeout);
			};

			class TH_OUT Random
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

			class TH_OUT Promise
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
				Promise(VMCContext* Base, bool IsRef);

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

			class TH_OUT Format : public Core::Object
			{
			public:
				std::vector<std::string> Args;

			public:
				Format();
				Format(unsigned char* Buffer);

			public:
				static std::string JSON(void* Ref, int TypeId);
				static std::string Form(const std::string& F, const Format& Form);
				static void WriteLine(Core::Console* Base, const std::string& F, Format* Form);
				static void Write(Core::Console* Base, const std::string& F, Format* Form);

			private:
				static void FormatBuffer(VMGlobal& Global, Core::Parser& Result, std::string& Offset, void* Ref, int TypeId);
				static void FormatJSON(VMGlobal& Global, Core::Parser& Result, void* Ref, int TypeId);
			};

			class TH_OUT ModelListener : public Engine::GUI::Listener
			{
			private:
				VMCFunction* Source;
				VMContext* Context;

			public:
				ModelListener(VMCFunction* NewCallback);
				ModelListener(const std::string& FunctionName);
				virtual ~ModelListener() override;

			private:
				Engine::GUI::EventCallback Bind(VMCFunction* Callback);
			};

			class TH_OUT_TS Registry
			{
			public:
				static bool LoadPointer(VMManager* Manager);
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
				static bool LoadDateTime(VMManager* Engine);
				static bool LoadConsole(VMManager* Engine);
				static bool LoadSchema(VMManager* Engine);
				static bool LoadVertices(VMManager* Engine);
				static bool LoadRectangle(VMManager* Engine);
				static bool LoadVector2(VMManager* Engine);
				static bool LoadVector3(VMManager* Engine);
				static bool LoadVector4(VMManager* Engine);
				static bool LoadUiModel(VMManager* Engine);
				static bool LoadUiControl(VMManager* Engine);
				static bool LoadUiContext(VMManager* Engine);
				static bool Release();
			};
		}
	}
}
#endif