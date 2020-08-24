#ifndef SCRIPT_ANY_H
#define SCRIPT_ANY_H
#ifndef ANGELSCRIPT_H 
#include <angelscript.h>
#endif

class VMCAny 
{
public:
	VMCAny(asIScriptEngine *engine);
	VMCAny(void *ref, int refTypeId, asIScriptEngine *engine);
	int AddRef() const;
	int Release() const;
	VMCAny &operator=(const VMCAny&);
	int CopyFrom(const VMCAny *other);
	void Store(void *ref, int refTypeId);
	void Store(asINT64 &value);
	void Store(double &value);
	bool Retrieve(void *ref, int refTypeId) const;
	bool Retrieve(asINT64 &value) const;
	bool Retrieve(double &value) const;
	int GetTypeId() const;
	int GetRefCount();
	void SetFlag();
	bool GetFlag();
	void EnumReferences(asIScriptEngine *engine);
	void ReleaseAllHandles(asIScriptEngine *engine);

protected:
	virtual ~VMCAny();
	void FreeObject();

	mutable int refCount;
	mutable bool gcFlag;
	asIScriptEngine *engine;

    struct valueStruct
    {
        union
        {
            asINT64 valueInt;
            double  valueFlt;
            void   *valueObj;
        };
        int   typeId;
    };

	valueStruct value;
};

void VM_RegisterAny(asIScriptEngine *engine);
#endif
