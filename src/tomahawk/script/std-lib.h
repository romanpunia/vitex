#ifndef TH_SCRIPT_STD_API_H
#define TH_SCRIPT_STD_API_H
#include "../core/script.h"
#define TH_TYPEREF(Name, TypeName) static const uint64_t Name = TH_SHUFFLE(TypeName); STDRegistry::Set(Name, TypeName)
#define TH_PROMISIFY(MemberFunction, TypeId) Tomahawk::Script::STDPromise::Ify<decltype(&MemberFunction), &MemberFunction>::Id<TypeId>
#define TH_PROMISIFY_REF(MemberFunction, TypeRef) Tomahawk::Script::STDPromise::Ify<decltype(&MemberFunction), &MemberFunction>::Decl<TypeRef>
#define TH_ARRAYIFY(MemberFunction, TypeId) Tomahawk::Script::STDArray::Ify<decltype(&MemberFunction), &MemberFunction>::Id<TypeId>
#define TH_ARRAYIFY_REF(MemberFunction, TypeRef) Tomahawk::Script::STDArray::Ify<decltype(&MemberFunction), &MemberFunction>::Decl<TypeRef>
#define TH_ANYIFY(MemberFunction, TypeId) Tomahawk::Script::STDAny::Ify<decltype(&MemberFunction), &MemberFunction>::Id<TypeId>
#define TH_ANYIFY_REF(MemberFunction, TypeRef) Tomahawk::Script::STDAny::Ify<decltype(&MemberFunction), &MemberFunction>::Decl<TypeRef>

namespace Tomahawk
{
	namespace Script
	{
		class STDPromise;

		class STDArray;

		class TH_OUT STDRegistry
		{
		public:
			static uint64_t Set(uint64_t Id, const std::string& Name);
			static int GetTypeId(uint64_t Id);
		};

		class TH_OUT STDException
		{
		public:
			static void Throw(const std::string& In);
			static std::string GetException();
		};

		class TH_OUT STDString
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
			static STDArray* Split(const std::string& delim, const std::string& str);
			static std::string Join(const STDArray& array, const std::string& delim);
			static char ToChar(const std::string& Symbol);
		};

		class TH_OUT STDMutex
		{
		private:
			std::mutex Base;
			int Ref;

		public:
			STDMutex();
			void AddRef();
			void Release();
			bool TryLock();
			void Lock();
			void Unlock();

		public:
			static STDMutex* Factory();
		};

		class TH_OUT STDMath
		{
		public:
			static float FpFromIEEE(as_size_t raw);
			static as_size_t FpToIEEE(float fp);
			static double FpFromIEEE(as_uint64_t raw);
			static as_uint64_t FpToIEEE(double fp);
			static bool CloseTo(float a, float b, float epsilon);
			static bool CloseTo(double a, double b, double epsilon);
		};

		class TH_OUT STDAny
		{
			friend STDPromise;

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
			STDAny(VMCManager* Engine);
			STDAny(void* Ref, int RefTypeId, VMCManager* Engine);
			STDAny(const STDAny&);
			int AddRef() const;
			int Release() const;
			STDAny& operator= (const STDAny&);
			int CopyFrom(const STDAny* Other);
			void Store(void* Ref, int RefTypeId);
			bool Retrieve(void* Ref, int RefTypeId) const;
			int GetTypeId() const;
			int GetRefCount();
			void SetFlag();
			bool GetFlag();
			void EnumReferences(VMCManager* Engine);
			void ReleaseAllHandles(VMCManager* Engine);

		protected:
			virtual ~STDAny();
			void FreeObject();

		public:
			static STDAny* Create(int TypeId, void* Ref);
			static STDAny* Create(const char* Decl, void* Ref);
			static void Factory1(VMCGeneric* G);
			static void Factory2(VMCGeneric* G);
			static STDAny& Assignment(STDAny* Other, STDAny* Self);

		public:
			template <typename T, T>
			struct Ify;

			template <typename T, typename R, typename ...Args, R(T::* F)(Args...)>
			struct Ify<R(T::*)(Args...), F>
			{
				template <VMTypeId TypeId>
				static STDAny* Id(T* Base, Args... Data)
				{
					R Subresult((Base->*F)(Data...));
					return STDAny::Create((int)TypeId, &Subresult);
				}
				template <uint64_t TypeRef>
				static STDAny* Decl(T* Base, Args... Data)
				{
					R Subresult((Base->*F)(Data...));
					return STDAny::Create(STDRegistry::GetTypeId(TypeRef), &Subresult);
				}
			};
		};

		class TH_OUT STDArray
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
			STDArray& operator= (const STDArray&);
			bool operator== (const STDArray&) const;
			void InsertAt(as_size_t Index, void* Value);
			void InsertAt(as_size_t Index, const STDArray& Other);
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
			STDArray(VMCTypeInfo* T, void* InitBuf);
			STDArray(as_size_t Length, VMCTypeInfo* T);
			STDArray(as_size_t Length, void* DefVal, VMCTypeInfo* T);
			STDArray(const STDArray& Other);
			virtual ~STDArray();
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
			static STDArray* Create(VMCTypeInfo* T);
			static STDArray* Create(VMCTypeInfo* T, as_size_t Length);
			static STDArray* Create(VMCTypeInfo* T, as_size_t Length, void* DefaultValue);
			static STDArray* Create(VMCTypeInfo* T, void* ListBuffer);
			static void CleanupTypeInfoCache(VMCTypeInfo* Type);
			static bool TemplateCallback(VMCTypeInfo* T, bool& DontGarbageCollect);

		public:
			template <typename T>
			static STDArray* Compose(VMCTypeInfo* ArrayType, const std::vector<T>& Objects)
			{
				STDArray* Array = Create(ArrayType, (unsigned int)Objects.size());
				for (size_t i = 0; i < Objects.size(); i++)
					Array->SetValue((as_size_t)i, (void*)&Objects[i]);

				return Array;
			}
			template <typename T>
			static typename std::enable_if<std::is_pointer<T>::value, std::vector<T>>::type Decompose(STDArray* Array)
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
			static typename std::enable_if<!std::is_pointer<T>::value, std::vector<T>>::type Decompose(STDArray* Array)
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

			template <typename T, typename R, typename ...Args, std::vector<R>(T::* F)(Args...)>
			struct Ify<std::vector<R>(T::*)(Args...), F>
			{
				template <VMTypeId TypeId>
				static STDArray* Id(T* Base, Args... Data)
				{
					VMManager* Manager = VMManager::Get();
					TH_ASSERT(Manager != nullptr, nullptr, "manager should be present");

					VMCTypeInfo* Info = Manager->Global().GetTypeInfoById((int)TypeId).GetTypeInfo();
					TH_ASSERT(Info != nullptr, nullptr, "typeinfo should be valid");

					std::vector<R> Source((Base->*F)(Data...));
					return STDArray::Compose(Info, Source);
				}
				template <uint64_t TypeRef>
				static STDArray* Decl(T* Base, Args... Data)
				{
					VMManager* Manager = VMManager::Get();
					TH_ASSERT(Manager != nullptr, nullptr, "manager should be present");

					VMCTypeInfo* Info = Manager->Global().GetTypeInfoById(STDRegistry::GetTypeId(TypeRef)).GetTypeInfo();
					TH_ASSERT(Info != nullptr, nullptr, "typeinfo should be valid");

					std::vector<R> Source((Base->*F)(Data...));
					return STDArray::Compose(Info, Source);
				}
			};
		};

		class TH_OUT STDIterator
		{
		protected:
			friend class STDMap;

		protected:
			union
			{
				as_int64_t ValueInt;
				double ValueFlt;
				void* ValueObj;
			};
			int TypeId;

		public:
			STDIterator();
			STDIterator(VMCManager* Engine, void* Value, int TypeId);
			~STDIterator();
			void Set(VMCManager* Engine, void* Value, int TypeId);
			void Set(VMCManager* Engine, STDIterator& Value);
			bool Get(VMCManager* Engine, void* Value, int TypeId) const;
			const void* GetAddressOfValue() const;
			int GetTypeId() const;
			void FreeValue(VMCManager* Engine);
			void EnumReferences(VMCManager* Engine);
		};

		class TH_OUT STDMap
		{
		public:
			typedef std::unordered_map<std::string, STDIterator> Map;

		public:
			class Iterator
			{
			protected:
				friend class STDMap;

			protected:
				Map::const_iterator It;
				const STDMap& Dict;

			public:
				void operator++();
				void operator++(int);
				Iterator& operator*();
				bool operator==(const Iterator& Other) const;
				bool operator!=(const Iterator& Other) const;
				const std::string& GetKey() const;
				int GetTypeId() const;
				bool GetValue(void* Value, int TypeId) const;
				const void* GetAddressOfValue() const;

			protected:
				Iterator();
				Iterator(const STDMap& Dict, Map::const_iterator It);
				Iterator& operator= (const Iterator&)
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
			Map Dict;

		public:
			void AddRef() const;
			void Release() const;
			STDMap& operator= (const STDMap& Other);
			void Set(const std::string& Key, void* Value, int TypeId);
			bool Get(const std::string& Key, void* Value, int TypeId) const;
			bool GetIndex(size_t Index, std::string* Key, void** Value, int* TypeId) const;
			STDIterator* operator[](const std::string& Key);
			const STDIterator* operator[](const std::string& Key) const;
			int GetTypeId(const std::string& Key) const;
			bool Exists(const std::string& Key) const;
			bool IsEmpty() const;
			as_size_t GetSize() const;
			bool Delete(const std::string& Key);
			void DeleteAll();
			STDArray* GetKeys() const;
			Iterator Begin() const;
			Iterator End() const;
			Iterator Find(const std::string& Key) const;
			int GetRefCount();
			void SetGCFlag();
			bool GetGCFlag();
			void EnumReferences(VMCManager* Engine);
			void ReleaseAllReferences(VMCManager* Engine);

		protected:
			STDMap(VMCManager* Engine);
			STDMap(unsigned char* Buffer);
			STDMap(const STDMap&);
			virtual ~STDMap();
			void Init(VMCManager* Engine);

		public:
			static STDMap* Create(VMCManager* Engine);
			static STDMap* Create(unsigned char* Buffer);
			static void Cleanup(VMCManager* engine);
			static void Setup(VMCManager* engine);
			static void Factory(VMCGeneric* gen);
			static void ListFactory(VMCGeneric* gen);
			static void KeyConstruct(void* mem);
			static void KeyDestruct(STDIterator* obj);
			static STDIterator& KeyopAssign(void* ref, int typeId, STDIterator* obj);
			static STDIterator& KeyopAssign(const STDIterator& other, STDIterator* obj);
			static STDIterator& KeyopAssign(double val, STDIterator* obj);
			static STDIterator& KeyopAssign(as_int64_t val, STDIterator* obj);
			static void KeyopCast(void* ref, int typeId, STDIterator* obj);
			static as_int64_t KeyopConvInt(STDIterator* obj);
			static double KeyopConvDouble(STDIterator* obj);
		};

		class TH_OUT STDGrid
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
			STDGrid(VMCTypeInfo* T, void* InitBuf);
			STDGrid(as_size_t W, as_size_t H, VMCTypeInfo* T);
			STDGrid(as_size_t W, as_size_t H, void* DefVal, VMCTypeInfo* T);
			virtual ~STDGrid();
			bool CheckMaxSize(as_size_t X, as_size_t Y);
			void CreateBuffer(SBuffer** Buf, as_size_t W, as_size_t H);
			void DeleteBuffer(SBuffer* Buf);
			void Construct(SBuffer* Buf);
			void Destruct(SBuffer* Buf);
			void SetValue(SBuffer* Buf, as_size_t X, as_size_t Y, void* Value);
			void* At(SBuffer* Buf, as_size_t X, as_size_t Y);

		public:
			static STDGrid* Create(VMCTypeInfo* T);
			static STDGrid* Create(VMCTypeInfo* T, as_size_t Width, as_size_t Height);
			static STDGrid* Create(VMCTypeInfo* T, as_size_t Width, as_size_t Height, void* DefaultValue);
			static STDGrid* Create(VMCTypeInfo* T, void* ListBuffer);
			static bool TemplateCallback(VMCTypeInfo* TI, bool& DontGarbageCollect);
		};

		class TH_OUT STDRef
		{
		protected:
			VMCTypeInfo* Type;
			void* Ref;

		public:
			STDRef();
			STDRef(const STDRef& Other);
			STDRef(void* Ref, VMCTypeInfo* Type);
			STDRef(void* Ref, int TypeId);
			~STDRef();
			STDRef& operator=(const STDRef& Other);
			void Set(void* Ref, VMCTypeInfo* Type);
			bool operator== (const STDRef& Other) const;
			bool operator!= (const STDRef& Other) const;
			bool Equals(void* Ref, int TypeId) const;
			void Cast(void** OutRef, int TypeId);
			VMCTypeInfo* GetType() const;
			int GetTypeId() const;
			void* GetRef();
			void EnumReferences(VMCManager* Engine);
			void ReleaseReferences(VMCManager* Engine);
			STDRef& Assign(void* Ref, int TypeId);

		protected:
			void ReleaseHandle();
			void AddRefHandle();

		public:
			static void Construct(STDRef* self);
			static void Construct(STDRef* self, const STDRef& o);
			static void Construct(STDRef* self, void* ref, int typeId);
			static void Destruct(STDRef* self);
		};

		class TH_OUT STDWeakRef
		{
		protected:
			VMCLockableSharedBool* WeakRefFlag;
			VMCTypeInfo* Type;
			void* Ref;

		public:
			STDWeakRef(VMCTypeInfo* Type);
			STDWeakRef(const STDWeakRef& Other);
			STDWeakRef(void* Ref, VMCTypeInfo* Type);
			~STDWeakRef();
			STDWeakRef& operator= (const STDWeakRef& Other);
			bool operator== (const STDWeakRef& Other) const;
			bool operator!= (const STDWeakRef& Other) const;
			STDWeakRef& Set(void* NewRef);
			void* Get() const;
			bool Equals(void* Ref) const;
			VMCTypeInfo* GetRefType() const;

		public:
			static void Construct(VMCTypeInfo* type, void* mem);
			static void Construct2(VMCTypeInfo* type, void* ref, void* mem);
			static void Destruct(STDWeakRef* obj);
			static bool TemplateCallback(VMCTypeInfo* TI, bool&);
		};

		class TH_OUT STDComplex
		{
		public:
			float R;
			float I;

		public:
			STDComplex();
			STDComplex(const STDComplex& Other);
			STDComplex(float R, float I = 0);
			STDComplex& operator= (const STDComplex& Other);
			STDComplex& operator+= (const STDComplex& Other);
			STDComplex& operator-= (const STDComplex& Other);
			STDComplex& operator*= (const STDComplex& Other);
			STDComplex& operator/= (const STDComplex& Other);
			float Length() const;
			float SquaredLength() const;
			STDComplex GetRI() const;
			void SetRI(const STDComplex& In);
			STDComplex GetIR() const;
			void SetIR(const STDComplex& In);
			bool operator== (const STDComplex& Other) const;
			bool operator!= (const STDComplex& Other) const;
			STDComplex operator+ (const STDComplex& Other) const;
			STDComplex operator- (const STDComplex& Other) const;
			STDComplex operator* (const STDComplex& Other) const;
			STDComplex operator/ (const STDComplex& Other) const;

		public:
			static void DefaultConstructor(STDComplex* self);
			static void CopyConstructor(const STDComplex& other, STDComplex* self);
			static void ConvConstructor(float r, STDComplex* self);
			static void InitConstructor(float r, float i, STDComplex* self);
			static void ListConstructor(float* list, STDComplex* self);
		};

		class TH_OUT STDThread
		{
		private:
			static int ContextUD;
			static int EngineListUD;

		private:
			struct
			{
				std::vector<STDAny*> Queue;
				std::condition_variable CV;
				std::mutex Mutex;
			} Pipe[2];

		private:
			std::condition_variable CV;
			std::thread Thread;
			std::mutex Mutex;
			VMCFunction* Function;
			VMManager* Manager;
			VMContext* Context;
			bool GCFlag;
			int Ref;

		public:
			STDThread(VMCManager* Engine, VMCFunction* Function);
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
			static STDThread* GetThread();
			static uint64_t GetThreadId();
			static void ThreadSleep(uint64_t Timeout);
		};

		class TH_OUT STDRandom
		{
		private:
			std::mt19937 Twister;
			std::uniform_real_distribution<double> DDist = std::uniform_real_distribution<double>(0.0, 1.0);
			int Ref = 1;

		public:
			STDRandom();
			void AddRef();
			void Release();
			void SeedFromTime();
			uint32_t GetU();
			int32_t GetI();
			double GetD();
			void Seed(uint32_t Seed);
			void Seed(STDArray* Array);
			void Assign(STDRandom* From);

		public:
			static STDRandom* Create();
		};

		class TH_OUT STDPromise
		{
			friend VMContext;

		private:
			VMContext* Context;
			STDAny* Future;
			bool Flag;
			int Ref;

		private:
			STDPromise(VMCContext* Base);

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
			void* Get();

		private:
			static STDPromise* Create();
			static STDPromise* Jump(STDPromise* Value);

		public:
			template <typename T, T>
			struct Ify;

			template <typename T, typename R, typename ...Args, Core::Async<R>(T::* F)(Args...)>
			struct Ify<Core::Async<R>(T::*)(Args...), F>
			{
				template <VMTypeId TypeId>
				static STDPromise* Id(T* Base, Args... Data)
				{
					STDPromise* Future = STDPromise::Create();
					((Base->*F)(Data...)).Await([Future](R&& Result)
					{
						Future->Set((void*)&Result, (int)TypeId);
					});

					return Jump(Future);
				}
				template <uint64_t TypeRef>
				static STDPromise* Decl(T* Base, Args... Data)
				{
					STDPromise* Future = STDPromise::Create();
					int TypeId = STDRegistry::GetTypeId(TypeRef);
					((Base->*F)(Data...)).Await([Future, TypeId](R&& Result)
					{
						Future->Set((void*)&Result, TypeId);
					});

					return Jump(Future);
				}
			};
		};

		TH_OUT bool STDRegisterAny(VMManager* Manager);
		TH_OUT bool STDRegisterArray(VMManager* Manager);
		TH_OUT bool STDRegisterComplex(VMManager* Manager);
		TH_OUT bool STDRegisterMap(VMManager* Manager);
		TH_OUT bool STDRegisterGrid(VMManager* Manager);
		TH_OUT bool STDRegisterRef(VMManager* Manager);
		TH_OUT bool STDRegisterWeakRef(VMManager* Manager);
		TH_OUT bool STDRegisterMath(VMManager* Manager);
		TH_OUT bool STDRegisterString(VMManager* Manager);
		TH_OUT bool STDRegisterException(VMManager* Manager);
		TH_OUT bool STDRegisterMutex(VMManager* Manager);
		TH_OUT bool STDRegisterThread(VMManager* Manager);
		TH_OUT bool STDRegisterRandom(VMManager* Manager);
		TH_OUT bool STDRegisterPromise(VMManager* Manager);
		TH_OUT bool STDFreeCore();
	}
}
#endif
