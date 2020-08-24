#ifndef SCRIPT_STRING_H
#define SCRIPT_STRING_H
#ifndef ANGELSCRIPT_H 
#include <angelscript.h>
#endif
#ifndef AS_USE_STLNAMES
#define AS_USE_STLNAMES 0
#endif
#ifndef AS_USE_ACCESSORS
#define AS_USE_ACCESSORS 0
#endif
#include <string>

void VM_RegisterString(asIScriptEngine *engine);
void VM_FreeStringProxy();
#endif
