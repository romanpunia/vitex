#include "handle.h"
#include <new>
#include <assert.h>
#include <string.h>

static void Construct(VMCRef *self) {
	new(self) VMCRef();
}
static void Construct(VMCRef *self, const VMCRef &o) {
	new(self) VMCRef(o);
}
void Construct(VMCRef *self, void *ref, int typeId) {
	new(self) VMCRef(ref, typeId);
}
static void Destruct(VMCRef *self) {
	self->~VMCRef();
}

VMCRef::VMCRef()
{
	m_ref = 0;
	m_type = 0;
}
VMCRef::VMCRef(const VMCRef &other)
{
	m_ref = other.m_ref;
	m_type = other.m_type;

	AddRefHandle();
}
VMCRef::VMCRef(void *ref, asITypeInfo *type)
{
	m_ref = ref;
	m_type = type;

	AddRefHandle();
}
VMCRef::VMCRef(void *ref, int typeId)
{
	m_ref = 0;
	m_type = 0;

	Assign(ref, typeId);
}
VMCRef::~VMCRef()
{
	ReleaseHandle();
}
void VMCRef::ReleaseHandle()
{
	if (m_ref && m_type)
	{
		asIScriptEngine *engine = m_type->GetEngine();
		engine->ReleaseScriptObject(m_ref, m_type);

		engine->Release();

		m_ref = 0;
		m_type = 0;
	}
}
void VMCRef::AddRefHandle()
{
	if (m_ref && m_type)
	{
		asIScriptEngine *engine = m_type->GetEngine();
		engine->AddRefScriptObject(m_ref, m_type);

		// Hold on to the engine so it isn't destroyed while
		// a reference to a script object is still held
		engine->AddRef();
	}
}
VMCRef &VMCRef::operator =(const VMCRef &other)
{
	Set(other.m_ref, other.m_type);

	return *this;
}
void VMCRef::Set(void *ref, asITypeInfo *type)
{
	if (m_ref == ref) return;

	ReleaseHandle();

	m_ref = ref;
	m_type = type;

	AddRefHandle();
}
void *VMCRef::GetRef()
{
	return m_ref;
}
asITypeInfo *VMCRef::GetType() const
{
	return m_type;
}
int VMCRef::GetTypeId() const
{
	if (m_type == 0) return 0;

	return m_type->GetTypeId() | asTYPEID_OBJHANDLE;
}
VMCRef &VMCRef::Assign(void *ref, int typeId)
{
	// When receiving a null handle we just clear our memory
	if (typeId == 0)
	{
		Set(0, 0);
		return *this;
	}

	// Dereference received handles to get the object
	if (typeId & asTYPEID_OBJHANDLE)
	{
		// Store the actual reference
		ref = *(void**)ref;
		typeId &= ~asTYPEID_OBJHANDLE;
	}

	// Get the object type
	asIScriptContext *ctx = asGetActiveContext();
	asIScriptEngine  *engine = ctx->GetEngine();
	asITypeInfo      *type = engine->GetTypeInfoById(typeId);

	// If the argument is another VMCRef, we should copy the content instead
	if (type && strcmp(type->GetName(), "ref") == 0)
	{
		VMCRef *r = (VMCRef*)ref;
		ref = r->m_ref;
		type = r->m_type;
	}

	Set(ref, type);

	return *this;
}
bool VMCRef::operator==(const VMCRef &o) const
{
	if (m_ref == o.m_ref &&
		m_type == o.m_type)
		return true;

	// TODO: If type is not the same, we should attempt to do a dynamic cast,
	//       which may change the pointer for application registered classes

	return false;
}
bool VMCRef::operator!=(const VMCRef &o) const
{
	return !(*this == o);
}
bool VMCRef::Equals(void *ref, int typeId) const
{
	// Null handles are received as reference to a null handle
	if (typeId == 0)
		ref = 0;

	// Dereference handles to get the object
	if (typeId & asTYPEID_OBJHANDLE)
	{
		// Compare the actual reference
		ref = *(void**)ref;
		typeId &= ~asTYPEID_OBJHANDLE;
	}

	// TODO: If typeId is not the same, we should attempt to do a dynamic cast, 
	//       which may change the pointer for application registered classes

	if (ref == m_ref) return true;

	return false;
}
void VMCRef::Cast(void **outRef, int typeId)
{
	// If we hold a null handle, then just return null
	if (m_type == 0)
	{
		*outRef = 0;
		return;
	}

	// It is expected that the outRef is always a handle
	assert(typeId & asTYPEID_OBJHANDLE);

	// Compare the type id of the actual object
	typeId &= ~asTYPEID_OBJHANDLE;
	asIScriptEngine  *engine = m_type->GetEngine();
	asITypeInfo      *type = engine->GetTypeInfoById(typeId);

	*outRef = 0;

	// RefCastObject will increment the refCount of the returned object if successful
	engine->RefCastObject(m_ref, m_type, type, outRef);
}
void VMCRef::EnumReferences(asIScriptEngine *inEngine)
{
	// If we're holding a reference, we'll notify the garbage collector of it
	if (m_ref)
		inEngine->GCEnumCallback(m_ref);

	// The object type itself is also garbage collected
	if (m_type)
		inEngine->GCEnumCallback(m_type);
}
void VMCRef::ReleaseReferences(asIScriptEngine *inEngine)
{
	// Simply clear the content to release the references
	Set(0, 0);
}

void RegisterScriptHandle_Native(asIScriptEngine *engine)
{
	int r;

#if AS_CAN_USE_CPP11
	// With C++11 it is possible to use asGetTypeTraits to automatically determine the flags that represent the C++ class
	r = engine->RegisterObjectType("ref", sizeof(VMCRef), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_GC | asGetTypeTraits<VMCRef>()); assert(r >= 0);
#else
	r = engine->RegisterObjectType("ref", sizeof(VMCRef), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_GC | asOBJ_APP_CLASS_CDAK); assert(r >= 0);
#endif
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR(Construct, (VMCRef *), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_CONSTRUCT, "void f(const ref &in)", asFUNCTIONPR(Construct, (VMCRef *, const VMCRef &), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_CONSTRUCT, "void f(const ?&in)", asFUNCTIONPR(Construct, (VMCRef *, void *, int), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR(Destruct, (VMCRef *), void), asCALL_CDECL_OBJFIRST); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(VMCRef, EnumReferences), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(VMCRef, ReleaseReferences), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("ref", "void opCast(?&out)", asMETHODPR(VMCRef, Cast, (void **, int), void), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("ref", "ref &opHndlAssign(const ref &in)", asMETHOD(VMCRef, operator=), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("ref", "ref &opHndlAssign(const ?&in)", asMETHOD(VMCRef, Assign), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("ref", "bool opEquals(const ref &in) const", asMETHODPR(VMCRef, operator==, (const VMCRef &) const, bool), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("ref", "bool opEquals(const ?&in) const", asMETHODPR(VMCRef, Equals, (void*, int) const, bool), asCALL_THISCALL); assert(r >= 0);
}
void CScriptHandle_Construct_Generic(asIScriptGeneric *gen)
{
	VMCRef *self = reinterpret_cast<VMCRef*>(gen->GetObject());
	new(self) VMCRef();
}
void CScriptHandle_ConstructCopy_Generic(asIScriptGeneric *gen)
{
	VMCRef *other = reinterpret_cast<VMCRef*>(gen->GetArgAddress(0));
	VMCRef *self = reinterpret_cast<VMCRef*>(gen->GetObject());
	new(self) VMCRef(*other);
}
void CScriptHandle_ConstructVar_Generic(asIScriptGeneric *gen)
{
	void *ref = gen->GetArgAddress(0);
	int typeId = gen->GetArgTypeId(0);
	VMCRef *self = reinterpret_cast<VMCRef*>(gen->GetObject());
	Construct(self, ref, typeId);
}
void CScriptHandle_Destruct_Generic(asIScriptGeneric *gen)
{
	VMCRef *self = reinterpret_cast<VMCRef*>(gen->GetObject());
	self->~VMCRef();
}
void CScriptHandle_Cast_Generic(asIScriptGeneric *gen)
{
	void **ref = reinterpret_cast<void**>(gen->GetArgAddress(0));
	int typeId = gen->GetArgTypeId(0);
	VMCRef *self = reinterpret_cast<VMCRef*>(gen->GetObject());
	self->Cast(ref, typeId);
}
void CScriptHandle_Assign_Generic(asIScriptGeneric *gen)
{
	VMCRef *other = reinterpret_cast<VMCRef*>(gen->GetArgAddress(0));
	VMCRef *self = reinterpret_cast<VMCRef*>(gen->GetObject());
	*self = *other;
	gen->SetReturnAddress(self);
}
void CScriptHandle_AssignVar_Generic(asIScriptGeneric *gen)
{
	void *ref = gen->GetArgAddress(0);
	int typeId = gen->GetArgTypeId(0);
	VMCRef *self = reinterpret_cast<VMCRef*>(gen->GetObject());
	self->Assign(ref, typeId);
	gen->SetReturnAddress(self);
}
void CScriptHandle_Equals_Generic(asIScriptGeneric *gen)
{
	VMCRef *other = reinterpret_cast<VMCRef*>(gen->GetArgAddress(0));
	VMCRef *self = reinterpret_cast<VMCRef*>(gen->GetObject());
	gen->SetReturnByte(*self == *other);
}
void CScriptHandle_EqualsVar_Generic(asIScriptGeneric *gen)
{
	void *ref = gen->GetArgAddress(0);
	int typeId = gen->GetArgTypeId(0);
	VMCRef *self = reinterpret_cast<VMCRef*>(gen->GetObject());
	gen->SetReturnByte(self->Equals(ref, typeId));
}
void CScriptHandle_EnumReferences_Generic(asIScriptGeneric *gen)
{
	VMCRef *self = reinterpret_cast<VMCRef*>(gen->GetObject());
	self->EnumReferences(gen->GetEngine());
}
void CScriptHandle_ReleaseReferences_Generic(asIScriptGeneric *gen)
{
	VMCRef *self = reinterpret_cast<VMCRef*>(gen->GetObject());
	self->ReleaseReferences(gen->GetEngine());
}
void RegisterScriptHandle_Generic(asIScriptEngine *engine)
{
	int r;

	r = engine->RegisterObjectType("ref", sizeof(VMCRef), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_GC | asOBJ_APP_CLASS_CDAK); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(CScriptHandle_Construct_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_CONSTRUCT, "void f(const ref &in)", asFUNCTION(CScriptHandle_ConstructCopy_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_CONSTRUCT, "void f(const ?&in)", asFUNCTION(CScriptHandle_ConstructVar_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(CScriptHandle_Destruct_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_ENUMREFS, "void f(int&in)", asFUNCTION(CScriptHandle_EnumReferences_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_RELEASEREFS, "void f(int&in)", asFUNCTION(CScriptHandle_ReleaseReferences_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("ref", "void opCast(?&out)", asFUNCTION(CScriptHandle_Cast_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("ref", "ref &opHndlAssign(const ref &in)", asFUNCTION(CScriptHandle_Assign_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("ref", "ref &opHndlAssign(const ?&in)", asFUNCTION(CScriptHandle_AssignVar_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("ref", "bool opEquals(const ref &in) const", asFUNCTION(CScriptHandle_Equals_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterObjectMethod("ref", "bool opEquals(const ?&in) const", asFUNCTION(CScriptHandle_EqualsVar_Generic), asCALL_GENERIC); assert(r >= 0);
}
void VM_RegisterHandle(asIScriptEngine *engine)
{
	if (strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY"))
		RegisterScriptHandle_Generic(engine);
	else
		RegisterScriptHandle_Native(engine);
}