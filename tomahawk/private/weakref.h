#ifndef SCRIPT_WEAKREF_H
#define SCRIPT_WEAKREF_H
#ifndef ANGELSCRIPT_H 
#include <angelscript.h>
#endif

class VMCWeakRef
{
public:
	VMCWeakRef(asITypeInfo *type);
	VMCWeakRef(const VMCWeakRef &other);
	VMCWeakRef(void *ref, asITypeInfo *type);
	~VMCWeakRef();
	VMCWeakRef &operator=(const VMCWeakRef &other);
	bool operator==(const VMCWeakRef &o) const;
	bool operator!=(const VMCWeakRef &o) const;
	VMCWeakRef &Set(void *newRef);
	void *Get() const;
	bool Equals(void *ref) const;
	asITypeInfo *GetRefType() const;

protected:
	friend void RegisterScriptWeakRef_Native(asIScriptEngine *engine);
	void* m_ref;
	asITypeInfo* m_type;
	asILockableSharedBool* m_weakRefFlag;
};

void VM_RegisterWeakRef(asIScriptEngine *engine);
#endif