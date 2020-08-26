#ifndef SCRIPT_LIBRARY_H
#define SCRIPT_LIBRARY_H
#include <sstream>
#include <string>
#ifndef ANGELSCRIPT_H
#include <angelscript.h>
#endif

int VM_CompareRelation(asIScriptEngine *engine, void *lobj, void *robj, int typeId, int &result);
int VM_CompareEquality(asIScriptEngine *engine, void *lobj, void *robj, int typeId, bool &result);
int VM_ExecuteString(asIScriptEngine *engine, const char *code, asIScriptModule *mod = 0, asIScriptContext *ctx = 0);
int VM_ExecuteString(asIScriptEngine *engine, const char *code, void *ret, int retTypeId, asIScriptModule *mod = 0, asIScriptContext *ctx = 0);
int VM_WriteConfigToFile(asIScriptEngine *engine, const char *filename);
int VM_WriteConfigToStream(asIScriptEngine *engine, std::ostream &strm);
int VM_ConfigEngineFromStream(asIScriptEngine *engine, std::istream &strm, const char *nameOfStream = "config", asIStringFactory *stringFactory = 0);
std::string VM_GetExceptionInfo(asIScriptContext *ctx, bool showStack = false);
void VM_RegisterExceptionRoutines(asIScriptEngine *engine);
#endif