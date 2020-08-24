#ifndef SCRIPT_GRID_H
#define SCRIPT_GRID_H
#ifndef ANGELSCRIPT_H 
#include <angelscript.h>
#endif

struct SGridBuffer;

class VMCGrid
{
public:
	static void SetMemoryFunctions(asALLOCFUNC_t allocFunc, asFREEFUNC_t freeFunc);
	static VMCGrid *Create(asITypeInfo *ot);
	static VMCGrid *Create(asITypeInfo *ot, asUINT width, asUINT height);
	static VMCGrid *Create(asITypeInfo *ot, asUINT width, asUINT height, void *defaultValue);
	static VMCGrid *Create(asITypeInfo *ot, void *listBuffer);
	void AddRef() const;
	void Release() const;
	asITypeInfo *GetGridObjectType() const;
	int GetGridTypeId() const;
	int GetElementTypeId() const;
	asUINT GetWidth() const;
	asUINT GetHeight() const;
	void Resize(asUINT width, asUINT height);
	void *At(asUINT x, asUINT y);
	const void *At(asUINT x, asUINT y) const;
	void  SetValue(asUINT x, asUINT y, void *value);
	int GetRefCount();
	void SetFlag();
	bool GetFlag();
	void EnumReferences(asIScriptEngine *engine);
	void ReleaseAllHandles(asIScriptEngine *engine);

protected:
	mutable int refCount;
	mutable bool gcFlag;
	asITypeInfo* objType;
	SGridBuffer* buffer;
	int elementSize;
	int subTypeId;

	VMCGrid(asITypeInfo *ot, void *initBuf);
	VMCGrid(asUINT w, asUINT h, asITypeInfo *ot);
	VMCGrid(asUINT w, asUINT h, void *defVal, asITypeInfo *ot);
	virtual ~VMCGrid();
	bool CheckMaxSize(asUINT x, asUINT y);
	void CreateBuffer(SGridBuffer **buf, asUINT w, asUINT h);
	void DeleteBuffer(SGridBuffer *buf);
	void Construct(SGridBuffer *buf);
	void Destruct(SGridBuffer *buf);
	void SetValue(SGridBuffer *buf, asUINT x, asUINT y, void *value);
	void* At(SGridBuffer *buf, asUINT x, asUINT y);
};

void VM_RegisterGrid(asIScriptEngine *engine);
#endif
