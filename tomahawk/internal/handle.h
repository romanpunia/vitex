#ifndef SCRIPT_HANDLE_H
#define SCRIPT_HANDLE_H
#ifndef ANGELSCRIPT_H 
#include <angelscript.h>
#endif

class VMCRef 
{
public:
	VMCRef();
	VMCRef(const VMCRef &other);
	VMCRef(void *ref, asITypeInfo *type);
	~VMCRef();
	VMCRef &operator=(const VMCRef &other);
	void Set(void *ref, asITypeInfo *type);
	bool operator==(const VMCRef &o) const;
	bool operator!=(const VMCRef &o) const;
	bool Equals(void *ref, int typeId) const;
	void Cast(void **outRef, int typeId);
	asITypeInfo *GetType() const;
	int GetTypeId() const;
	void *GetRef();
	void EnumReferences(asIScriptEngine *engine);
	void ReleaseReferences(asIScriptEngine *engine);

protected:
	friend void Construct(VMCRef *self, void *ref, int typeId);
	friend void RegisterScriptHandle_Native(asIScriptEngine *engine);
	friend void CScriptHandle_AssignVar_Generic(asIScriptGeneric *gen);
	void ReleaseHandle();
	void AddRefHandle();
	VMCRef(void *ref, int typeId);
	VMCRef &Assign(void *ref, int typeId);
	void *m_ref;
	asITypeInfo *m_type;
};

void VM_RegisterHandle(asIScriptEngine *engine);
#endif
