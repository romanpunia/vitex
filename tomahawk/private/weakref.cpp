#include "weakref.h"
#include <new>
#include <assert.h>
#include <string.h>

static void ScriptWeakRefConstruct(asITypeInfo *type, void *mem)
{
	new(mem) VMCWeakRef(type);
}
static void ScriptWeakRefConstruct2(asITypeInfo *type, void *ref, void *mem)
{
	new(mem) VMCWeakRef(ref, type);
	asIScriptContext *ctx = asGetActiveContext();
	if (ctx && ctx->GetState() == asEXECUTION_EXCEPTION)
		reinterpret_cast<VMCWeakRef*>(mem)->~VMCWeakRef();
}
static void ScriptWeakRefDestruct(VMCWeakRef *obj)
{
	obj->~VMCWeakRef();
}
static bool ScriptWeakRefTemplateCallback(asITypeInfo *ti, bool&)
{
	asITypeInfo *subType = ti->GetSubType();
	if (subType == 0) return false;
	if (!(subType->GetFlags() & asOBJ_REF)) return false;

	if (ti->GetSubTypeId() & asTYPEID_OBJHANDLE)
		return false;

	asUINT cnt = subType->GetBehaviourCount();
	for (asUINT n = 0; n < cnt; n++)
	{
		asEBehaviours beh;
		subType->GetBehaviourByIndex(n, &beh);
		if (beh == asBEHAVE_GET_WEAKREF_FLAG)
			return true;
	}

	ti->GetEngine()->WriteMessage("weakref", 0, 0, asMSGTYPE_ERROR, "The subtype doesn't support weak references");
	return false;
}

VMCWeakRef::VMCWeakRef(asITypeInfo *type)
{
	m_ref = 0;
	m_type = type;
	m_type->AddRef();
	m_weakRefFlag = 0;
}
VMCWeakRef::VMCWeakRef(const VMCWeakRef &other)
{
	m_ref = other.m_ref;
	m_type = other.m_type;
	m_type->AddRef();
	m_weakRefFlag = other.m_weakRefFlag;
	if (m_weakRefFlag)
		m_weakRefFlag->AddRef();
}
VMCWeakRef::VMCWeakRef(void *ref, asITypeInfo *type)
{
	m_ref = ref;
	m_type = type;
	m_type->AddRef();

	assert(strcmp(type->GetName(), "weakref") == 0 ||
		strcmp(type->GetName(), "const_weakref") == 0);

	m_weakRefFlag = m_type->GetEngine()->GetWeakRefFlagOfScriptObject(m_ref, m_type->GetSubType());
	if (m_weakRefFlag)
		m_weakRefFlag->AddRef();
}
VMCWeakRef::~VMCWeakRef()
{
	if (m_type)
		m_type->Release();
	if (m_weakRefFlag)
		m_weakRefFlag->Release();
}
VMCWeakRef &VMCWeakRef::operator =(const VMCWeakRef &other)
{
	if (m_ref == other.m_ref &&
		m_weakRefFlag == other.m_weakRefFlag)
		return *this;

	if (m_type != other.m_type)
	{
		if (!(strcmp(m_type->GetName(), "const_weakref") == 0 &&
			strcmp(other.m_type->GetName(), "weakref") == 0 &&
			m_type->GetSubType() == other.m_type->GetSubType()))
		{
			assert(false);
			return *this;
		}
	}

	m_ref = other.m_ref;
	if (m_weakRefFlag)
		m_weakRefFlag->Release();

	m_weakRefFlag = other.m_weakRefFlag;
	if (m_weakRefFlag)
		m_weakRefFlag->AddRef();

	return *this;
}
VMCWeakRef &VMCWeakRef::Set(void *newRef)
{
	if (m_weakRefFlag)
		m_weakRefFlag->Release();

	m_ref = newRef;
	if (newRef)
	{
		m_weakRefFlag = m_type->GetEngine()->GetWeakRefFlagOfScriptObject(newRef, m_type->GetSubType());
		m_weakRefFlag->AddRef();
	}
	else
		m_weakRefFlag = 0;

	m_type->GetEngine()->ReleaseScriptObject(newRef, m_type->GetSubType());
	return *this;
}
asITypeInfo *VMCWeakRef::GetRefType() const
{
	return m_type->GetSubType();
}
bool VMCWeakRef::operator==(const VMCWeakRef &o) const
{
	if (m_ref == o.m_ref &&
		m_weakRefFlag == o.m_weakRefFlag &&
		m_type == o.m_type)
		return true;

	return false;
}
bool VMCWeakRef::operator!=(const VMCWeakRef &o) const
{
	return !(*this == o);
}
void *VMCWeakRef::Get() const
{
	if (m_ref == 0 || m_weakRefFlag == 0)
		return 0;

	m_weakRefFlag->Lock();
	if (!m_weakRefFlag->Get())
	{
		m_type->GetEngine()->AddRefScriptObject(m_ref, m_type->GetSubType());
		m_weakRefFlag->Unlock();
		return m_ref;
	}
	m_weakRefFlag->Unlock();

	return 0;
}
bool VMCWeakRef::Equals(void *ref) const
{
	if (m_ref != ref)
		return false;

	asILockableSharedBool *flag = m_type->GetEngine()->GetWeakRefFlagOfScriptObject(ref, m_type->GetSubType());
	if (m_weakRefFlag != flag)
		return false;

	return true;
}

void RegisterWeakRef_Native(asIScriptEngine *engine)
{
	int r;
	r = engine->RegisterObjectType("weakref<class T>", sizeof(VMCWeakRef), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_TEMPLATE | asOBJ_APP_CLASS_DAK); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("weakref<T>", asBEHAVE_CONSTRUCT, "void f(int&in)", asFUNCTION(ScriptWeakRefConstruct), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("weakref<T>", asBEHAVE_CONSTRUCT, "void f(int&in, T@+)", asFUNCTION(ScriptWeakRefConstruct2), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("weakref<T>", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(ScriptWeakRefDestruct), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("weakref<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(ScriptWeakRefTemplateCallback), asCALL_CDECL); assert(r >= 0);
	r = engine->RegisterObjectMethod("weakref<T>", "T@ opImplCast()", asMETHOD(VMCWeakRef, Get), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("weakref<T>", "T@ get() const", asMETHODPR(VMCWeakRef, Get, () const, void*), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("weakref<T>", "weakref<T> &opHndlAssign(const weakref<T> &in)", asMETHOD(VMCWeakRef, operator=), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("weakref<T>", "weakref<T> &opAssign(const weakref<T> &in)", asMETHOD(VMCWeakRef, operator=), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("weakref<T>", "bool opEquals(const weakref<T> &in) const", asMETHODPR(VMCWeakRef, operator==, (const VMCWeakRef &) const, bool), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("weakref<T>", "weakref<T> &opHndlAssign(T@)", asMETHOD(VMCWeakRef, Set), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("weakref<T>", "bool opEquals(const T@+) const", asMETHOD(VMCWeakRef, Equals), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectType("const_weakref<class T>", sizeof(VMCWeakRef), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_TEMPLATE | asOBJ_APP_CLASS_DAK); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("const_weakref<T>", asBEHAVE_CONSTRUCT, "void f(int&in)", asFUNCTION(ScriptWeakRefConstruct), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("const_weakref<T>", asBEHAVE_CONSTRUCT, "void f(int&in, const T@+)", asFUNCTION(ScriptWeakRefConstruct2), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("const_weakref<T>", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(ScriptWeakRefDestruct), asCALL_CDECL_OBJLAST); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("const_weakref<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(ScriptWeakRefTemplateCallback), asCALL_CDECL); assert(r >= 0);
	r = engine->RegisterObjectMethod("const_weakref<T>", "const T@ opImplCast() const", asMETHOD(VMCWeakRef, Get), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("const_weakref<T>", "const T@ get() const", asMETHODPR(VMCWeakRef, Get, () const, void*), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("const_weakref<T>", "const_weakref<T> &opHndlAssign(const const_weakref<T> &in)", asMETHOD(VMCWeakRef, operator=), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("const_weakref<T>", "const_weakref<T> &opAssign(const const_weakref<T> &in)", asMETHOD(VMCWeakRef, operator=), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("const_weakref<T>", "bool opEquals(const const_weakref<T> &in) const", asMETHODPR(VMCWeakRef, operator==, (const VMCWeakRef &) const, bool), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("const_weakref<T>", "const_weakref<T> &opHndlAssign(const T@)", asMETHOD(VMCWeakRef, Set), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("const_weakref<T>", "bool opEquals(const T@+) const", asMETHOD(VMCWeakRef, Equals), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("const_weakref<T>", "const_weakref<T> &opHndlAssign(const weakref<T> &in)", asMETHOD(VMCWeakRef, operator=), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("const_weakref<T>", "bool opEquals(const weakref<T> &in) const", asMETHODPR(VMCWeakRef, operator==, (const VMCWeakRef &) const, bool), asCALL_THISCALL); assert(r >= 0);
}
static void ScriptWeakRefConstruct_Generic(asIScriptGeneric *gen)
{
	asITypeInfo *ti = *reinterpret_cast<asITypeInfo**>(gen->GetAddressOfArg(0));

	ScriptWeakRefConstruct(ti, gen->GetObject());
}
static void ScriptWeakRefConstruct2_Generic(asIScriptGeneric *gen)
{
	asITypeInfo *ti = *reinterpret_cast<asITypeInfo**>(gen->GetAddressOfArg(0));
	void *ref = gen->GetArgAddress(1);

	ScriptWeakRefConstruct2(ti, ref, gen->GetObject());
}
static void ScriptWeakRefDestruct_Generic(asIScriptGeneric *gen)
{
	VMCWeakRef *self = reinterpret_cast<VMCWeakRef*>(gen->GetObject());
	self->~VMCWeakRef();
}
void CScriptWeakRef_Get_Generic(asIScriptGeneric *gen)
{
	VMCWeakRef *self = reinterpret_cast<VMCWeakRef*>(gen->GetObject());
	gen->SetReturnAddress(self->Get());
}
void CScriptWeakRef_Assign_Generic(asIScriptGeneric *gen)
{
	VMCWeakRef *other = reinterpret_cast<VMCWeakRef*>(gen->GetArgAddress(0));
	VMCWeakRef *self = reinterpret_cast<VMCWeakRef*>(gen->GetObject());
	*self = *other;
	gen->SetReturnAddress(self);
}
void CScriptWeakRef_Assign2_Generic(asIScriptGeneric *gen)
{
	void *other = gen->GetArgAddress(0);
	VMCWeakRef *self = reinterpret_cast<VMCWeakRef*>(gen->GetObject());
	self->Set(other);
	gen->SetReturnAddress(self);
}
void CScriptWeakRef_Equals_Generic(asIScriptGeneric *gen)
{
	VMCWeakRef *other = reinterpret_cast<VMCWeakRef*>(gen->GetArgAddress(0));
	VMCWeakRef *self = reinterpret_cast<VMCWeakRef*>(gen->GetObject());
	gen->SetReturnByte(*self == *other);
}
void CScriptWeakRef_Equals2_Generic(asIScriptGeneric *gen)
{
	void *other = gen->GetArgAddress(0);
	VMCWeakRef *self = reinterpret_cast<VMCWeakRef*>(gen->GetObject());

	gen->SetReturnByte(self->Equals(other));
}
static void ScriptWeakRefTemplateCallback_Generic(asIScriptGeneric *gen)
{
	asITypeInfo *ti = *reinterpret_cast<asITypeInfo**>(gen->GetAddressOfArg(0));
	bool *dontGarbageCollect = *reinterpret_cast<bool**>(gen->GetAddressOfArg(1));
	*reinterpret_cast<bool*>(gen->GetAddressOfReturnLocation()) = ScriptWeakRefTemplateCallback(ti, *dontGarbageCollect);
}
void RegisterWeakRef_Generic(asIScriptEngine *engine)
{
	int r;
	r = engine->RegisterObjectType("weakref<class T>", sizeof(VMCWeakRef), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_TEMPLATE | asOBJ_APP_CLASS_DAK); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("weakref<T>", asBEHAVE_CONSTRUCT, "void f(int&in)", asFUNCTION(ScriptWeakRefConstruct_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("weakref<T>", asBEHAVE_CONSTRUCT, "void f(int&in, T@+)", asFUNCTION(ScriptWeakRefConstruct2_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("weakref<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(ScriptWeakRefTemplateCallback_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("weakref<T>", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(ScriptWeakRefDestruct_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("weakref<T>", "T@ opImplCast()", asFUNCTION(CScriptWeakRef_Get_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("weakref<T>", "T@ get() const", asFUNCTION(CScriptWeakRef_Get_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("weakref<T>", "weakref<T> &opHndlAssign(const weakref<T> &in)", asFUNCTION(CScriptWeakRef_Assign_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("weakref<T>", "weakref<T> &opAssign(const weakref<T> &in)", asFUNCTION(CScriptWeakRef_Assign_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("weakref<T>", "bool opEquals(const weakref<T> &in) const", asFUNCTION(CScriptWeakRef_Equals_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("weakref<T>", "weakref<T> &opHndlAssign(T@)", asFUNCTION(CScriptWeakRef_Assign2_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("weakref<T>", "bool opEquals(const T@+) const", asFUNCTION(CScriptWeakRef_Equals2_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectType("const_weakref<class T>", sizeof(VMCWeakRef), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_TEMPLATE | asOBJ_APP_CLASS_DAK); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("const_weakref<T>", asBEHAVE_CONSTRUCT, "void f(int&in)", asFUNCTION(ScriptWeakRefConstruct_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("const_weakref<T>", asBEHAVE_CONSTRUCT, "void f(int&in, const T@+)", asFUNCTION(ScriptWeakRefConstruct2_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("const_weakref<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(ScriptWeakRefTemplateCallback_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("const_weakref<T>", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(ScriptWeakRefDestruct_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("const_weakref<T>", "const T@ opImplCast() const", asFUNCTION(CScriptWeakRef_Get_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("const_weakref<T>", "const T@ get() const", asFUNCTION(CScriptWeakRef_Get_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("const_weakref<T>", "const_weakref<T> &opHndlAssign(const const_weakref<T> &in)", asFUNCTION(CScriptWeakRef_Assign_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("const_weakref<T>", "const_weakref<T> &opAssign(const const_weakref<T> &in)", asFUNCTION(CScriptWeakRef_Assign_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("const_weakref<T>", "bool opEquals(const const_weakref<T> &in) const", asFUNCTION(CScriptWeakRef_Equals_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("const_weakref<T>", "const_weakref<T> &opHndlAssign(const T@)", asFUNCTION(CScriptWeakRef_Assign2_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("const_weakref<T>", "bool opEquals(const T@+) const", asFUNCTION(CScriptWeakRef_Equals2_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("const_weakref<T>", "const_weakref<T> &opHndlAssign(const weakref<T> &in)", asFUNCTION(CScriptWeakRef_Assign_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("const_weakref<T>", "bool opEquals(const weakref<T> &in) const", asFUNCTION(CScriptWeakRef_Equals_Generic), asCALL_GENERIC); assert(r >= 0);
}
void VM_RegisterWeakRef(asIScriptEngine *engine)
{
	if (strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY"))
		RegisterWeakRef_Generic(engine);
	else
		RegisterWeakRef_Native(engine);
}