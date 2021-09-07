#ifndef SCRIPT_COMPILER_H
#define SCRIPT_COMPILER_H
#include "interpreter.h"
#ifndef ANGELSCRIPT_H
#include <angelscript.h>
#endif
#ifdef HAS_AS_JIT
#include <vector>
#include <map>

namespace assembler
{
	struct CodePage;
	struct CriticalSection;
};

enum JITSettings
{
	JIT_NO_SUSPEND = 0x01,
	JIT_SYSCALL_FPU_NORESET = 0x02,
	JIT_SYSCALL_NO_ERRORS = 0x04,
	JIT_ALLOC_SIMPLE = 0x08,
	JIT_NO_SWITCHES = 0x10,
	JIT_NO_SCRIPT_CALLS = 0x20,
	JIT_FAST_REFCOUNT = 0x40,
};

struct DeferredCodePointer
{
	void** jitFunction;
	void** jitEntry;
};

class VMCJITCompiler : public asIJITCompiler
{
	std::multimap<asJITFunction, assembler::CodePage*> pages;
	std::multimap<asJITFunction, unsigned char**> jumpTables;
	std::multimap<asIScriptFunction*, DeferredCodePointer> deferredPointers;
	assembler::CriticalSection* lock;
	assembler::CodePage* activePage;
	unsigned char** activeJumpTable;
	unsigned currentTableSize;
	unsigned flags;

public:
	VMCJITCompiler(unsigned Flags = 0);
	~VMCJITCompiler();
	int CompileFunction(asIScriptFunction* function, asJITFunction* output);
	void ReleaseJITFunction(asJITFunction func);
	void finalizePages();
};
#endif
#endif