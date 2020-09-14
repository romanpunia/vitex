#ifndef THAWK_SCRIPT_CORE_H
#define THAWK_SCRIPT_CORE_H

#include "../core/script.h"

namespace Tomahawk
{
	namespace Script
	{
		typedef std::function<void(class VMCAsync*)> AsyncWorkCallback;
		typedef std::function<void(enum VMExecState)> AsyncDoneCallback;

		class THAWK_OUT VMCException
		{
		public:
			static void Throw(const std::string& In);
			static std::string GetException();
		};

		class THAWK_OUT VMCAny
		{
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
			VMCAny(VMCManager* Engine);
			VMCAny(void* Ref, int RefTypeId, VMCManager* Engine);
			int AddRef() const;
			int Release() const;
			VMCAny& operator= (const VMCAny&);
			int CopyFrom(const VMCAny* Other);
			void Store(void* Ref, int RefTypeId);
			void Store(as_int64_t& Value);
			void Store(double& Value);
			bool Retrieve(void* Ref, int RefTypeId) const;
			bool Retrieve(as_int64_t& Value) const;
			bool Retrieve(double& Value) const;
			int GetTypeId() const;
			int GetRefCount();
			void SetFlag();
			bool GetFlag();
			void EnumReferences(VMCManager* Engine);
			void ReleaseAllHandles(VMCManager* Engine);

		protected:
			virtual ~VMCAny();
			void FreeObject();
		};

		class THAWK_OUT VMCArray
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
			VMCArray& operator= (const VMCArray&);
			bool operator== (const VMCArray&) const;
			void InsertAt(as_size_t Index, void* Value);
			void InsertAt(as_size_t Index, const VMCArray& Other);
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
			VMCArray(VMCTypeInfo* T, void* InitBuf);
			VMCArray(as_size_t Length, VMCTypeInfo* T);
			VMCArray(as_size_t Length, void* DefVal, VMCTypeInfo* T);
			VMCArray(const VMCArray& Other);
			virtual ~VMCArray();
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
			static VMCArray* Create(VMCTypeInfo* T);
			static VMCArray* Create(VMCTypeInfo* T, as_size_t Length);
			static VMCArray* Create(VMCTypeInfo* T, as_size_t Length, void* DefaultValue);
			static VMCArray* Create(VMCTypeInfo* T, void* ListBuffer);

		public:
			template <typename T>
			static VMCArray* ComposeFromObjects(VMCTypeInfo* ArrayType, const std::vector<T>& Objects)
			{
				VMCArray* Array = Create(ArrayType, (unsigned int)Objects.size());
				for (size_t i = 0; i < Objects.size(); i++)
					Array->SetValue(i, (void*)&Objects[i]);

				return Array;
			}
			template <typename T>
			static VMCArray* ComposeFromPointers(VMCTypeInfo* ArrayType, const std::vector<T*>& Objects)
			{
				VMCArray* Array = Create(ArrayType, (unsigned int)Objects.size());
				for (size_t i = 0; i < Objects.size(); i++)
					Array->SetValue(i, (void*)&Objects[i]);

				return Array;
			}
			template <typename T>
			static void DecomposeToObjects(const VMCArray*& Array, std::vector<T>* Objects)
			{
				if (!Objects)
					return;

				unsigned int Size = Array.GetSize();
				Objects->reserve(Size);

				for (unsigned int i = 0; i < Size; i++)
				{
					T* Object = (T*)Array.At(i);
					Objects->push_back(*Object);
				}
			}
			template <typename T>
			static void DecomposeToPointers(const VMCArray*& Array, std::vector<T*>* Objects)
			{
				if (!Objects)
					return;

				unsigned int Size = Array.GetSize();
				Objects->reserve(Size);

				for (unsigned int i = 0; i < Size; i++)
				{
					T* Object = (T*)Array.At(i);
					Objects->push_back(Object);
				}
			}
		};

		class THAWK_OUT VMCMapKey
		{
		protected:
			friend class VMCMap;

		protected:
			union
			{
				as_int64_t ValueInt;
				double ValueFlt;
				void* ValueObj;
			};
			int TypeId;

		public:
			VMCMapKey();
			VMCMapKey(VMCManager* Engine, void* Value, int TypeId);
			~VMCMapKey();
			void Set(VMCManager* Engine, void* Value, int TypeId);
			void Set(VMCManager* Engine, const as_int64_t& Value);
			void Set(VMCManager* Engine, const double& Value);
			void Set(VMCManager* Engine, VMCMapKey& Value);
			bool Get(VMCManager* Engine, void* Value, int TypeId) const;
			bool Get(VMCManager* Engine, as_int64_t& Value) const;
			bool Get(VMCManager* Engine, double& Value) const;
			const void* GetAddressOfValue() const;
			int GetTypeId() const;
			void FreeValue(VMCManager* Engine);
			void EnumReferences(VMCManager* Engine);
		};

		class THAWK_OUT VMCMap
		{
		public:
			typedef std::unordered_map<std::string, VMCMapKey> Map;

		public:
			class Iterator
			{
			protected:
				friend class VMCMap;

			protected:
				Map::const_iterator It;
				const VMCMap& Dict;

			public:
				void operator++();
				void operator++(int);
				Iterator& operator*();
				bool operator==(const Iterator& Other) const;
				bool operator!=(const Iterator& Other) const;
				const std::string& GetKey() const;
				int GetTypeId() const;
				bool GetValue(as_int64_t& Value) const;
				bool GetValue(double& Value) const;
				bool GetValue(void* Value, int TypeId) const;
				const void* GetAddressOfValue() const;

			protected:
				Iterator();
				Iterator(const VMCMap& Dict, Map::const_iterator It);
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
			VMCMap& operator= (const VMCMap& Other);
			void Set(const std::string& Key, void* Value, int TypeId);
			void Set(const std::string& Key, const as_int64_t& Value);
			void Set(const std::string& Key, const double& Value);
			bool Get(const std::string& Key, void* Value, int TypeId) const;
			bool Get(const std::string& Key, as_int64_t& Value) const;
			bool Get(const std::string& Key, double& Value) const;
			bool GetIndex(size_t Index, std::string* Key, void** Value, int* TypeId) const;
			VMCMapKey* operator[](const std::string& Key);
			const VMCMapKey* operator[](const std::string& Key) const;
			int GetTypeId(const std::string& Key) const;
			bool Exists(const std::string& Key) const;
			bool IsEmpty() const;
			as_size_t GetSize() const;
			bool Delete(const std::string& Key);
			void DeleteAll();
			VMCArray* GetKeys() const;
			Iterator Begin() const;
			Iterator End() const;
			Iterator Find(const std::string& Key) const;
			int GetRefCount();
			void SetGCFlag();
			bool GetGCFlag();
			void EnumReferences(VMCManager* Engine);
			void ReleaseAllReferences(VMCManager* Engine);

		protected:
			VMCMap(VMCManager* Engine);
			VMCMap(unsigned char* Buffer);
			virtual ~VMCMap();
			void Init(VMCManager* Engine);

		public:
			static VMCMap* Create(VMCManager* Engine);
			static VMCMap* Create(unsigned char* Buffer);
		};

		class THAWK_OUT VMCGrid
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
			VMCGrid(VMCTypeInfo* T, void* InitBuf);
			VMCGrid(as_size_t W, as_size_t H, VMCTypeInfo* T);
			VMCGrid(as_size_t W, as_size_t H, void* DefVal, VMCTypeInfo* T);
			virtual ~VMCGrid();
			bool CheckMaxSize(as_size_t X, as_size_t Y);
			void CreateBuffer(SBuffer** Buf, as_size_t W, as_size_t H);
			void DeleteBuffer(SBuffer* Buf);
			void Construct(SBuffer* Buf);
			void Destruct(SBuffer* Buf);
			void SetValue(SBuffer* Buf, as_size_t X, as_size_t Y, void* Value);
			void* At(SBuffer* Buf, as_size_t X, as_size_t Y);

		public:
			static VMCGrid* Create(VMCTypeInfo* T);
			static VMCGrid* Create(VMCTypeInfo* T, as_size_t Width, as_size_t Height);
			static VMCGrid* Create(VMCTypeInfo* T, as_size_t Width, as_size_t Height, void* DefaultValue);
			static VMCGrid* Create(VMCTypeInfo* T, void* ListBuffer);
		};

		class THAWK_OUT VMCRef
		{
		protected:
			VMCTypeInfo* Type;
			void* Ref;

		public:
			VMCRef();
			VMCRef(const VMCRef& Other);
			VMCRef(void* Ref, VMCTypeInfo* Type);
			VMCRef(void* Ref, int TypeId);
			~VMCRef();
			VMCRef& operator=(const VMCRef& Other);
			void Set(void* Ref, VMCTypeInfo* Type);
			bool operator== (const VMCRef& Other) const;
			bool operator!= (const VMCRef& Other) const;
			bool Equals(void* Ref, int TypeId) const;
			void Cast(void** OutRef, int TypeId);
			VMCTypeInfo* GetType() const;
			int GetTypeId() const;
			void* GetRef();
			void EnumReferences(VMCManager* Engine);
			void ReleaseReferences(VMCManager* Engine);
			VMCRef& Assign(void* Ref, int TypeId);

		protected:
			void ReleaseHandle();
			void AddRefHandle();
		};

		class THAWK_OUT VMCWeakRef
		{
		protected:
			VMCLockableSharedBool* WeakRefFlag;
			VMCTypeInfo* Type;
			void* Ref;

		public:
			VMCWeakRef(VMCTypeInfo* Type);
			VMCWeakRef(const VMCWeakRef& Other);
			VMCWeakRef(void* Ref, VMCTypeInfo* Type);
			~VMCWeakRef();
			VMCWeakRef& operator= (const VMCWeakRef& Other);
			bool operator== (const VMCWeakRef& Other) const;
			bool operator!= (const VMCWeakRef& Other) const;
			VMCWeakRef& Set(void* NewRef);
			void* Get() const;
			bool Equals(void* Ref) const;
			VMCTypeInfo* GetRefType() const;
		};

		class THAWK_OUT VMCComplex
		{
		public:
			float R;
			float I;

		public:
			VMCComplex();
			VMCComplex(const VMCComplex& Other);
			VMCComplex(float R, float I = 0);
			VMCComplex& operator= (const VMCComplex& Other);
			VMCComplex& operator+= (const VMCComplex& Other);
			VMCComplex& operator-= (const VMCComplex& Other);
			VMCComplex& operator*= (const VMCComplex& Other);
			VMCComplex& operator/= (const VMCComplex& Other);
			float Length() const;
			float SquaredLength() const;
			VMCComplex GetRI() const;
			void SetRI(const VMCComplex& In);
			VMCComplex GetIR() const;
			void SetIR(const VMCComplex& In);
			bool operator== (const VMCComplex& Other) const;
			bool operator!= (const VMCComplex& Other) const;
			VMCComplex operator+ (const VMCComplex& Other) const;
			VMCComplex operator- (const VMCComplex& Other) const;
			VMCComplex operator* (const VMCComplex& Other) const;
			VMCComplex operator/ (const VMCComplex& Other) const;
		};

		class THAWK_OUT VMCThread
		{
		private:
			static int ContextUD;
			static int EngineListUD;

		private:
			struct
			{
				std::vector<VMCAny*> Queue;
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
			VMCThread(VMCManager* Engine, VMCFunction* Function);
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
			static VMCThread* GetThread();
			static uint64_t GetThreadId();
			static void ThreadSleep(uint64_t Timeout);
		};

		class THAWK_OUT VMCRandom
		{
		private:
			std::mt19937 Twister;
			std::uniform_real_distribution<double> DDist = std::uniform_real_distribution<double>(0.0, 1.0);
			int Ref = 1;

		public:
			VMCRandom();
			void AddRef();
			void Release();
			void SeedFromTime();
			uint32_t GetU();
			int32_t GetI();
			double GetD();
			void Seed(uint32_t Seed);
			void Seed(VMCArray* Array);
			void Assign(VMCRandom* From);

		public:
			static VMCRandom* Create();
		};

		class THAWK_OUT VMCAsync
		{
		private:
			AsyncDoneCallback Done;
			VMCContext* Context;
			VMCAny* Any;
			std::mutex Mutex;
			bool Rejected;
			bool GCFlag;
			int Ref;

		private:
			VMCAsync(VMCContext* Base);

		public:
			void EnumReferences(VMCManager* Engine);
			void ReleaseReferences(VMCManager* Engine);
			void AddRef();
			void Release();
			void SetGCFlag();
			bool GetGCFlag();
			int GetRefCount();
			int Set(VMCAny* Value);
			int Set(void* Ref, int TypeId);
			int Set(void* Ref, const char* TypeName);
			bool GetAny(void* Ref, int TypeId) const;
			VMCAny* Get() const;
			VMCAsync* Await();

		public:
			static VMCAsync* Create(const AsyncWorkCallback& WorkCallback, const AsyncDoneCallback& DoneCallback);
			static VMCAsync* CreatePending();
			static VMCAsync* CreateFilled(void* Ref, int TypeId);
			static VMCAsync* CreateFilled(void* Ref, const char* TypeName);
			static VMCAsync* CreateFilled(bool Value);
			static VMCAsync* CreateFilled(int64_t Value);
			static VMCAsync* CreateFilled(double Value);
			static VMCAsync* CreateEmpty();
		};

		THAWK_OUT bool RegisterAnyAPI(VMManager* Manager);
		THAWK_OUT bool RegisterArrayAPI(VMManager* Manager);
		THAWK_OUT bool RegisterComplexAPI(VMManager* Manager);
		THAWK_OUT bool RegisterMapAPI(VMManager* Manager);
		THAWK_OUT bool RegisterGridAPI(VMManager* Manager);
		THAWK_OUT bool RegisterRefAPI(VMManager* Manager);
		THAWK_OUT bool RegisterWeakRefAPI(VMManager* Manager);
		THAWK_OUT bool RegisterMathAPI(VMManager* Manager);
		THAWK_OUT bool RegisterStringAPI(VMManager* Manager);
		THAWK_OUT bool RegisterExceptionAPI(VMManager* Manager);
		THAWK_OUT bool RegisterThreadAPI(VMManager* Manager);
		THAWK_OUT bool RegisterRandomAPI(VMManager* Manager);
		THAWK_OUT bool RegisterAsyncAPI(VMManager* Manager);
		THAWK_OUT bool FreeCoreAPI();
	}
}
#endif