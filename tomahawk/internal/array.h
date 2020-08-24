#ifndef SCRIPT_ARRAY_H
#define SCRIPT_ARRAY_H
#ifndef ANGELSCRIPT_H 
#include <angelscript.h>
#endif
#ifndef AS_USE_STLNAMES
#define AS_USE_STLNAMES 0
#endif
#ifndef AS_USE_ACCESSORS
#define AS_USE_ACCESSORS 0
#endif

struct SArrayBuffer;
struct SArrayCache;

class VMCArray
{
public:
	static void SetMemoryFunctions(asALLOCFUNC_t allocFunc, asFREEFUNC_t freeFunc);
	static VMCArray *Create(asITypeInfo *ot);
	static VMCArray *Create(asITypeInfo *ot, asUINT length);
	static VMCArray *Create(asITypeInfo *ot, asUINT length, void *defaultValue);
	static VMCArray *Create(asITypeInfo *ot, void *listBuffer);
	void AddRef() const;
	void Release() const;
	asITypeInfo *GetArrayObjectType() const;
	int GetArrayTypeId() const;
	int GetElementTypeId() const;
	asUINT GetSize() const;
	bool IsEmpty() const;
	void Reserve(asUINT maxElements);
	void Resize(asUINT numElements);
	void *At(asUINT index);
	const void *At(asUINT index) const;
	void SetValue(asUINT index, void *value);
	VMCArray &operator=(const VMCArray&);
	bool operator==(const VMCArray &) const;
	void InsertAt(asUINT index, void *value);
	void InsertAt(asUINT index, const VMCArray &arr);
	void InsertLast(void *value);
	void RemoveAt(asUINT index);
	void RemoveLast();
	void RemoveRange(asUINT start, asUINT count);
	void SortAsc();
	void SortDesc();
	void SortAsc(asUINT startAt, asUINT count);
	void SortDesc(asUINT startAt, asUINT count);
	void Sort(asUINT startAt, asUINT count, bool asc);
	void Sort(asIScriptFunction *less, asUINT startAt, asUINT count);
	void Reverse();
	int Find(void *value) const;
	int Find(asUINT startAt, void *value) const;
	int FindByRef(void *ref) const;
	int FindByRef(asUINT startAt, void *ref) const;
	void *GetBuffer();
	int  GetRefCount();
	void SetFlag();
	bool GetFlag();
	void EnumReferences(asIScriptEngine *engine);
	void ReleaseAllHandles(asIScriptEngine *engine);

protected:
	mutable int refCount;
	mutable bool gcFlag;
	asITypeInfo *objType;
	SArrayBuffer *buffer;
	int elementSize;
	int subTypeId;

	VMCArray(asITypeInfo *ot, void *initBuf);
	VMCArray(asUINT length, asITypeInfo *ot);
	VMCArray(asUINT length, void *defVal, asITypeInfo *ot);
	VMCArray(const VMCArray &other);
	virtual ~VMCArray();
	bool Less(const void *a, const void *b, bool asc, asIScriptContext *ctx, SArrayCache *cache);
	void *GetArrayItemPointer(int index);
	void *GetDataPointer(void *buffer);
	void Copy(void *dst, void *src);
	void Precache();
	bool CheckMaxSize(asUINT numElements);
	void Resize(int delta, asUINT at);
	void CreateBuffer(SArrayBuffer **buf, asUINT numElements);
	void DeleteBuffer(SArrayBuffer *buf);
	void CopyBuffer(SArrayBuffer *dst, SArrayBuffer *src);
	void Construct(SArrayBuffer *buf, asUINT start, asUINT end);
	void Destruct(SArrayBuffer *buf, asUINT start, asUINT end);
	bool Equals(const void *a, const void *b, asIScriptContext *ctx, SArrayCache *cache) const;
};

void VM_RegisterArray(asIScriptEngine *engine, bool defaultArray);
#endif
