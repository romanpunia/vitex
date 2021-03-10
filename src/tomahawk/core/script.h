#ifndef TH_SCRIPT_H
#define TH_SCRIPT_H
#include "compute.h"
#include <type_traits>
#include <random>
#ifdef __LP64__
typedef unsigned int as_uint32_t;
typedef unsigned long as_uint64_t;
typedef long as_int64_t;
#else
typedef unsigned long as_uint32_t;
#if !defined(_MSC_VER) && (defined(__GNUC__) || defined(__MWERKS__) || defined(__SUNPRO_CC) || defined(__psp2__))
typedef uint64_t as_uint64_t;
typedef int64_t as_int64_t;
#else
typedef unsigned __int64 as_uint64_t;
typedef __int64 as_int64_t;
#endif
#endif
typedef unsigned int as_size_t;

class asIScriptEngine;
class asIScriptContext;
class asIScriptModule;
class asITypeInfo;
class asIScriptFunction;
class asIScriptGeneric;
class asIScriptObject;
class asILockableSharedBool;
struct asSFuncPtr;
struct asSMessageInfo;

namespace Tomahawk
{
	namespace Script
	{
		struct VMModule;

		struct VMFunction;

		class VMCompiler;

		class VMManager;

		class VMContext;

		class VMDummy
		{
		};

		enum VMJIT
		{
			VMJIT_No_Suspend = 0x01,
			VMJIT_Syscall_FPU_No_Reset = 0x02,
			VMJIT_Syscall_No_Errors = 0x04,
			VMJIT_Alloc_Simple = 0x08,
			VMJIT_No_Switches = 0x10,
			VMJIT_No_Script_Calls = 0x20,
			VMJIT_Fast_Ref_Counter = 0x40,
			VMJIT_Optimal = VMJIT_Alloc_Simple | VMJIT_Fast_Ref_Counter
		};

		enum VMLogType
		{
			VMLogType_ERROR = 0,
			VMLogType_WARNING = 1,
			VMLogType_INFORMATION = 2
		};

		enum VMProp
		{
			VMProp_ALLOW_UNSAFE_REFERENCES = 1,
			VMProp_OPTIMIZE_BYTECODE = 2,
			VMProp_COPY_SCRIPT_SECTIONS = 3,
			VMProp_MAX_STACK_SIZE = 4,
			VMProp_USE_CHARACTER_LITERALS = 5,
			VMProp_ALLOW_MULTILINE_STRINGS = 6,
			VMProp_ALLOW_IMPLICIT_HANDLE_TYPES = 7,
			VMProp_BUILD_WITHOUT_LINE_CUES = 8,
			VMProp_INIT_GLOBAL_VARS_AFTER_BUILD = 9,
			VMProp_REQUIRE_ENUM_SCOPE = 10,
			VMProp_SCRIPT_SCANNER = 11,
			VMProp_INCLUDE_JIT_INSTRUCTIONS = 12,
			VMProp_STRING_ENCODING = 13,
			VMProp_PROPERTY_ACCESSOR_MODE = 14,
			VMProp_EXPAND_DEF_ARRAY_TO_TMPL = 15,
			VMProp_AUTO_GARBAGE_COLLECT = 16,
			VMProp_DISALLOW_GLOBAL_VARS = 17,
			VMProp_ALWAYS_IMPL_DEFAULT_CONSTRUCT = 18,
			VMProp_COMPILER_WARNINGS = 19,
			VMProp_DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE = 20,
			VMProp_ALTER_SYNTAX_NAMED_ARGS = 21,
			VMProp_DISABLE_INTEGER_DIVISION = 22,
			VMProp_DISALLOW_EMPTY_LIST_ELEMENTS = 23,
			VMProp_PRIVATE_PROP_AS_PROTECTED = 24,
			VMProp_ALLOW_UNICODE_IDENTIFIERS = 25,
			VMProp_HEREDOC_TRIM_MODE = 26,
			VMProp_MAX_NESTED_CALLS = 27,
			VMProp_GENERIC_CALL_MODE = 28,
			VMProp_INIT_STACK_SIZE = 29,
			VMProp_INIT_CALL_STACK_SIZE = 30,
			VMProp_MAX_CALL_STACK_SIZE = 31
		};

		enum VMTypeMod
		{
			VMTypeMod_NONE = 0,
			VMTypeMod_INREF = 1,
			VMTypeMod_OUTREF = 2,
			VMTypeMod_INOUTREF = 3,
			VMTypeMod_CONST = 4
		};

		enum VMCompileFlags
		{
			VMCompileFlags_ADD_TO_MODULE = 1
		};

		enum VMFuncType
		{
			VMFuncType_DUMMY = -1,
			VMFuncType_SYSTEM = 0,
			VMFuncType_SCRIPT = 1,
			VMFuncType_INTERFACE = 2,
			VMFuncType_VIRTUAL = 3,
			VMFuncType_FUNCDEF = 4,
			VMFuncType_IMPORTED = 5,
			VMFuncType_DELEGATE = 6
		};

		enum VMBehave
		{
			VMBehave_CONSTRUCT,
			VMBehave_LIST_CONSTRUCT,
			VMBehave_DESTRUCT,
			VMBehave_FACTORY,
			VMBehave_LIST_FACTORY,
			VMBehave_ADDREF,
			VMBehave_RELEASE,
			VMBehave_GET_WEAKREF_FLAG,
			VMBehave_TEMPLATE_CALLBACK,
			VMBehave_FIRST_GC,
			VMBehave_GETREFCOUNT = VMBehave_FIRST_GC,
			VMBehave_SETGCFLAG,
			VMBehave_GETGCFLAG,
			VMBehave_ENUMREFS,
			VMBehave_RELEASEREFS,
			VMBehave_LAST_GC = VMBehave_RELEASEREFS,
			VMBehave_MAX
		};

		enum VMExecState
		{
			VMExecState_FINISHED = 0,
			VMExecState_SUSPENDED = 1,
			VMExecState_ABORTED = 2,
			VMExecState_EXCEPTION = 3,
			VMExecState_PREPARED = 4,
			VMExecState_UNINITIALIZED = 5,
			VMExecState_ACTIVE = 6,
			VMExecState_ERROR = 7
		};

		enum VMCall
		{
			VMCall_CDECL = 0,
			VMCall_STDCALL = 1,
			VMCall_THISCALL_ASGLOBAL = 2,
			VMCall_THISCALL = 3,
			VMCall_CDECL_OBJLAST = 4,
			VMCall_CDECL_OBJFIRST = 5,
			VMCall_GENERIC = 6,
			VMCall_THISCALL_OBJLAST = 7,
			VMCall_THISCALL_OBJFIRST = 8
		};

		enum VMResult
		{
			VMResult_SUCCESS = 0,
			VMResult_ERROR = -1,
			VMResult_CONTEXT_ACTIVE = -2,
			VMResult_CONTEXT_NOT_FINISHED = -3,
			VMResult_CONTEXT_NOT_PREPARED = -4,
			VMResult_INVALID_ARG = -5,
			VMResult_NO_FUNCTION = -6,
			VMResult_NOT_SUPPORTED = -7,
			VMResult_INVALID_NAME = -8,
			VMResult_NAME_TAKEN = -9,
			VMResult_INVALID_DECLARATION = -10,
			VMResult_INVALID_OBJECT = -11,
			VMResult_INVALID_TYPE = -12,
			VMResult_ALREADY_REGISTERED = -13,
			VMResult_MULTIPLE_FUNCTIONS = -14,
			VMResult_NO_MODULE = -15,
			VMResult_NO_GLOBAL_VAR = -16,
			VMResult_INVALID_CONFIGURATION = -17,
			VMResult_INVALID_INTERFACE = -18,
			VMResult_CANT_BIND_ALL_FUNCTIONS = -19,
			VMResult_LOWER_ARRAY_DIMENSION_NOT_REGISTERED = -20,
			VMResult_WRONG_CONFIG_GROUP = -21,
			VMResult_CONFIG_GROUP_IS_IN_USE = -22,
			VMResult_ILLEGAL_BEHAVIOUR_FOR_TYPE = -23,
			VMResult_WRONG_CALLING_CONV = -24,
			VMResult_BUILD_IN_PROGRESS = -25,
			VMResult_INIT_GLOBAL_VARS_FAILED = -26,
			VMResult_OUT_OF_MEMORY = -27,
			VMResult_MODULE_IS_IN_USE = -28
		};

		enum VMTypeId
		{
			VMTypeId_VOID = 0,
			VMTypeId_BOOL = 1,
			VMTypeId_INT8 = 2,
			VMTypeId_INT16 = 3,
			VMTypeId_INT32 = 4,
			VMTypeId_INT64 = 5,
			VMTypeId_UINT8 = 6,
			VMTypeId_UINT16 = 7,
			VMTypeId_UINT32 = 8,
			VMTypeId_UINT64 = 9,
			VMTypeId_FLOAT = 10,
			VMTypeId_DOUBLE = 11,
			VMTypeId_OBJHANDLE = 0x40000000,
			VMTypeId_HANDLETOCONST = 0x20000000,
			VMTypeId_MASK_OBJECT = 0x1C000000,
			VMTypeId_APPOBJECT = 0x04000000,
			VMTypeId_SCRIPTOBJECT = 0x08000000,
			VMTypeId_TEMPLATE = 0x10000000,
			VMTypeId_MASK_SEQNBR = 0x03FFFFFF
		};

		enum VMObjType
		{
			VMObjType_REF = (1 << 0),
			VMObjType_VALUE = (1 << 1),
			VMObjType_GC = (1 << 2),
			VMObjType_POD = (1 << 3),
			VMObjType_NOHANDLE = (1 << 4),
			VMObjType_SCOPED = (1 << 5),
			VMObjType_TEMPLATE = (1 << 6),
			VMObjType_ASHANDLE = (1 << 7),
			VMObjType_APP_CLASS = (1 << 8),
			VMObjType_APP_CLASS_CONSTRUCTOR = (1 << 9),
			VMObjType_APP_CLASS_DESTRUCTOR = (1 << 10),
			VMObjType_APP_CLASS_ASSIGNMENT = (1 << 11),
			VMObjType_APP_CLASS_COPY_CONSTRUCTOR = (1 << 12),
			VMObjType_APP_CLASS_C = (VMObjType_APP_CLASS + VMObjType_APP_CLASS_CONSTRUCTOR),
			VMObjType_APP_CLASS_CD = (VMObjType_APP_CLASS + VMObjType_APP_CLASS_CONSTRUCTOR + VMObjType_APP_CLASS_DESTRUCTOR),
			VMObjType_APP_CLASS_CA = (VMObjType_APP_CLASS + VMObjType_APP_CLASS_CONSTRUCTOR + VMObjType_APP_CLASS_ASSIGNMENT),
			VMObjType_APP_CLASS_CK = (VMObjType_APP_CLASS + VMObjType_APP_CLASS_CONSTRUCTOR + VMObjType_APP_CLASS_COPY_CONSTRUCTOR),
			VMObjType_APP_CLASS_CDA = (VMObjType_APP_CLASS + VMObjType_APP_CLASS_CONSTRUCTOR + VMObjType_APP_CLASS_DESTRUCTOR + VMObjType_APP_CLASS_ASSIGNMENT),
			VMObjType_APP_CLASS_CDK = (VMObjType_APP_CLASS + VMObjType_APP_CLASS_CONSTRUCTOR + VMObjType_APP_CLASS_DESTRUCTOR + VMObjType_APP_CLASS_COPY_CONSTRUCTOR),
			VMObjType_APP_CLASS_CAK = (VMObjType_APP_CLASS + VMObjType_APP_CLASS_CONSTRUCTOR + VMObjType_APP_CLASS_ASSIGNMENT + VMObjType_APP_CLASS_COPY_CONSTRUCTOR),
			VMObjType_APP_CLASS_CDAK = (VMObjType_APP_CLASS + VMObjType_APP_CLASS_CONSTRUCTOR + VMObjType_APP_CLASS_DESTRUCTOR + VMObjType_APP_CLASS_ASSIGNMENT + VMObjType_APP_CLASS_COPY_CONSTRUCTOR),
			VMObjType_APP_CLASS_D = (VMObjType_APP_CLASS + VMObjType_APP_CLASS_DESTRUCTOR),
			VMObjType_APP_CLASS_DA = (VMObjType_APP_CLASS + VMObjType_APP_CLASS_DESTRUCTOR + VMObjType_APP_CLASS_ASSIGNMENT),
			VMObjType_APP_CLASS_DK = (VMObjType_APP_CLASS + VMObjType_APP_CLASS_DESTRUCTOR + VMObjType_APP_CLASS_COPY_CONSTRUCTOR),
			VMObjType_APP_CLASS_DAK = (VMObjType_APP_CLASS + VMObjType_APP_CLASS_DESTRUCTOR + VMObjType_APP_CLASS_ASSIGNMENT + VMObjType_APP_CLASS_COPY_CONSTRUCTOR),
			VMObjType_APP_CLASS_A = (VMObjType_APP_CLASS + VMObjType_APP_CLASS_ASSIGNMENT),
			VMObjType_APP_CLASS_AK = (VMObjType_APP_CLASS + VMObjType_APP_CLASS_ASSIGNMENT + VMObjType_APP_CLASS_COPY_CONSTRUCTOR),
			VMObjType_APP_CLASS_K = (VMObjType_APP_CLASS + VMObjType_APP_CLASS_COPY_CONSTRUCTOR),
			VMObjType_APP_PRIMITIVE = (1 << 13),
			VMObjType_APP_FLOAT = (1 << 14),
			VMObjType_APP_ARRAY = (1 << 15),
			VMObjType_APP_CLASS_ALLINTS = (1 << 16),
			VMObjType_APP_CLASS_ALLFLOATS = (1 << 17),
			VMObjType_NOCOUNT = (1 << 18),
			VMObjType_APP_CLASS_ALIGN8 = (1 << 19),
			VMObjType_IMPLICIT_HANDLE = (1 << 20),
			VMObjType_MASK_VALID_FLAGS = 0x1FFFFF
		};

		enum VMOpFunc
		{
			VMOpFunc_Neg,
			VMOpFunc_Com,
			VMOpFunc_PreInc,
			VMOpFunc_PreDec,
			VMOpFunc_PostInc,
			VMOpFunc_PostDec,
			VMOpFunc_Equals,
			VMOpFunc_Cmp,
			VMOpFunc_Assign,
			VMOpFunc_AddAssign,
			VMOpFunc_SubAssign,
			VMOpFunc_MulAssign,
			VMOpFunc_DivAssign,
			VMOpFunc_ModAssign,
			VMOpFunc_PowAssign,
			VMOpFunc_AndAssign,
			VMOpFunc_OrAssign,
			VMOpFunc_XOrAssign,
			VMOpFunc_ShlAssign,
			VMOpFunc_ShrAssign,
			VMOpFunc_UshrAssign,
			VMOpFunc_Add,
			VMOpFunc_Sub,
			VMOpFunc_Mul,
			VMOpFunc_Div,
			VMOpFunc_Mod,
			VMOpFunc_Pow,
			VMOpFunc_And,
			VMOpFunc_Or,
			VMOpFunc_XOr,
			VMOpFunc_Shl,
			VMOpFunc_Shr,
			VMOpFunc_Ushr,
			VMOpFunc_Index,
			VMOpFunc_Call,
			VMOpFunc_Cast,
			VMOpFunc_ImplCast
		};

		enum VMOp
		{
			VMOp_Left = 0,
			VMOp_Right = 1,
			VMOp_Const = 2
		};

		enum VMImport
		{
			VMImport_CLibraries = 1,
			VMImport_CSymbols = 2,
			VMImport_Submodules = 4,
			VMImport_Files = 8,
			VMImport_JSON = 8,
			VMImport_All = (VMImport_CLibraries | VMImport_CSymbols | VMImport_Submodules | VMImport_Files | VMImport_JSON)
		};

		typedef asIScriptEngine VMCManager;
		typedef asIScriptContext VMCContext;
		typedef asIScriptModule VMCModule;
		typedef asITypeInfo VMCTypeInfo;
		typedef asIScriptFunction VMCFunction;
		typedef asIScriptGeneric VMCGeneric;
		typedef asIScriptObject VMCObject;
		typedef asILockableSharedBool VMCLockableSharedBool;
		typedef void(VMDummy::*VMMethodPtr)();
		typedef void(*VMObjectFunction)();
		typedef std::function<void(struct VMTypeInfo*, struct VMFuncProperty*)> PropertyCallback;
		typedef std::function<void(struct VMTypeInfo*, struct VMFunction*)> MethodCallback;
		typedef std::function<void(class VMManager*)> SubmoduleCallback;

        class TH_OUT VMFuncStore
        {
        public:
            static asSFuncPtr* CreateFunctionBase(void(*Base)(), int Type);
            static asSFuncPtr* CreateMethodBase(const void *Base, size_t Size, int Type);
            static asSFuncPtr* CreateDummyBase();
            static void ReleaseFunctor(asSFuncPtr **Ptr);
            static int AtomicNotifyGC(const char* TypeName, void* Object);
        };
        
        template <int N>
        struct TH_OUT VMFuncCall
        {
            template <class M>
            static asSFuncPtr* Bind(M Value)
            {
                return VMFuncStore::CreateDummyBase();
            }
        };

        template <>
        struct TH_OUT VMFuncCall<sizeof(VMMethodPtr)>
        {
            template <class M>
            static asSFuncPtr* Bind(M Value)
            {
                return VMFuncStore::CreateMethodBase(&Value, sizeof(VMMethodPtr), 3);
            }
        };
#if defined(_MSC_VER) && !defined(__MWERKS__)
        template <>
        struct TH_OUT VMFuncCall<sizeof(VMMethodPtr) + 1 * sizeof(int)>
        {
            template <class M>
            static asSFuncPtr* Bind(M Value)
            {
                return VMFuncStore::CreateMethodBase(&Value, sizeof(VMMethodPtr) + sizeof(int), 3);
            }
        };

        template <>
        struct TH_OUT VMFuncCall<sizeof(VMMethodPtr) + 2 * sizeof(int)>
        {
            template <class M>
            static asSFuncPtr* Bind(M Value)
            {
                asSFuncPtr* Ptr = VMFuncStore::CreateMethodBase(&Value, sizeof(VMMethodPtr) + 2 * sizeof(int), 3);
#if defined(_MSC_VER) && !defined(TH_64)
                *(reinterpret_cast<unsigned long*>(Ptr) + 3) = *(reinterpret_cast<unsigned long*>(Ptr) + 2);
#endif
                return Ptr;
            }
        };

        template <>
        struct TH_OUT VMFuncCall<sizeof(VMMethodPtr) + 3 * sizeof(int)>
        {
            template <class M>
            static asSFuncPtr* Bind(M Value)
            {
                return VMFuncStore::CreateMethodBase(&Value, sizeof(VMMethodPtr) + 3 * sizeof(int), 3);
            }
        };

        template <>
        struct TH_OUT VMFuncCall<sizeof(VMMethodPtr) + 4 * sizeof(int)>
        {
            template <class M>
            static asSFuncPtr* Bind(M Value)
            {
                return VMFuncStore::CreateMethodBase(&Value, sizeof(VMMethodPtr) + 4 * sizeof(int), 3);
            }
        };
#endif
        class TH_OUT VMGeneric
		{
		private:
			VMManager* Manager;
			VMCGeneric* Generic;

		public:
			VMGeneric(VMCGeneric* Base);
			void* GetObjectAddress();
			int GetObjectTypeId() const;
			int GetArgsCount() const;
			int GetArgTypeId(unsigned int Argument, size_t* Flags = 0) const;
			unsigned char GetArgByte(unsigned int Argument);
			unsigned short GetArgWord(unsigned int Argument);
			size_t GetArgDWord(unsigned int Argument);
			uint64_t GetArgQWord(unsigned int Argument);
			float GetArgFloat(unsigned int Argument);
			double GetArgDouble(unsigned int Argument);
			void* GetArgAddress(unsigned int Argument);
			void* GetArgObjectAddress(unsigned int Argument);
			void* GetAddressOfArg(unsigned int Argument);
			int GetReturnTypeId(size_t* Flags = 0) const;
			int SetReturnByte(unsigned char Value);
			int SetReturnWord(unsigned short Value);
			int SetReturnDWord(size_t Value);
			int SetReturnQWord(uint64_t Value);
			int SetReturnFloat(float Value);
			int SetReturnDouble(double Value);
			int SetReturnAddress(void* Address);
			int SetReturnObjectAddress(void* Object);
			void* GetAddressOfReturnLocation();
			bool IsValid() const;
			VMCGeneric* GetGeneric() const;
			VMManager* GetManager() const;

		public:
			template <typename T>
			int SetReturnObject(T* Object)
			{
				return SetReturnObjectAddress((void*)Object);
			}
			template <typename T>
			T* GetArgObject(unsigned int Arg)
			{
				return (T*)GetArgObjectAddress(Arg);
			}
		};

		class TH_OUT VMBridge
		{
		public:
			template <typename T>
			static asSFuncPtr* Function(T Value)
			{
#ifdef TH_64
				void(*Address)() = reinterpret_cast<void(*)()>(size_t(Value));
#else
				void (* Address)() = reinterpret_cast<void (*)()>(Value);
#endif
				return VMFuncStore::CreateFunctionBase(Address, 2);
			}
			template <typename T>
			static asSFuncPtr* FunctionGeneric(T Value)
			{
#ifdef TH_64
				void(*Address)() = reinterpret_cast<void(*)()>(size_t(Value));
#else
				void(*Address)() = reinterpret_cast<void(*)()>(Value);
#endif
				return VMFuncStore::CreateFunctionBase(Address, 1);
			}
			template <typename T, typename R, typename... Args>
			static asSFuncPtr* Method(R(T::*Value)(Args...))
			{
				return VMFuncCall<sizeof(void (T::*)())>::Bind((void (T::*)())(Value));
			}
			template <typename T, typename R, typename... Args>
			static asSFuncPtr* Method(R(T::*Value)(Args...) const)
			{
				return VMFuncCall<sizeof(void (T::*)())>::Bind((void (T::*)())(Value));
			}
			template <typename T, typename R, typename... Args>
			static asSFuncPtr* MethodOp(R(T::*Value)(Args...))
			{
				return VMFuncCall<sizeof(void (T::*)())>::Bind(static_cast<R(T::*)(Args...)>(Value));
			}
			template <typename T, typename R, typename... Args>
			static asSFuncPtr* MethodOp(R(T::*Value)(Args...) const)
			{
				return VMFuncCall<sizeof(void (T::*)())>::Bind(static_cast<R(T::*)(Args...)>(Value));
			}
			template <typename T, typename... Args>
			static void GetConstructorCall(void* Memory, Args... Data)
			{
				new(Memory) T(Data...);
			}
			template <typename T>
			static void GetConstructorListCall(VMCGeneric* Generic)
			{
				VMGeneric Args(Generic);
				*reinterpret_cast<T**>(Args.GetAddressOfReturnLocation()) = new T((unsigned char*)Args.GetArgAddress(0));
			}
			template <typename T>
			static void GetDestructorCall(void* Memory)
			{
				((T*)Memory)->~T();
			}
			template <typename T, const char* TypeName, typename... Args>
			static T* GetManagedCall(Args... Data)
			{
				auto* Result = new T(Data...);
				VMFuncStore::AtomicNotifyGC(TypeName, (void*)Result);

				return Result;
			}
			template <typename T, const char* TypeName>
			static void GetManagedListCall(VMCGeneric* Generic)
			{
				VMGeneric Args(Generic);
				T* Result = new T((unsigned char*)Args.GetArgAddress(0));
				*reinterpret_cast<T**>(Args.GetAddressOfReturnLocation()) = Result;
                VMFuncStore::AtomicNotifyGC(TypeName, (void*)Result);
			}
			template <typename T, typename... Args>
			static T* GetUnmanagedCall(Args... Data)
			{
				return new T(Data...);
			}
			template <typename T>
			static void GetUnmanagedListCall(VMCGeneric* Generic)
			{
				VMGeneric Args(Generic);
				*reinterpret_cast<T**>(Args.GetAddressOfReturnLocation()) = new T((unsigned char*)Args.GetArgAddress(0));
			}
            template <typename T>
            static size_t GetTypeTraits()
            {
#if defined(_MSC_VER) || defined(_LIBCPP_TYPE_TRAITS) || (__GNUC__ >= 5) || defined(__clang__)
                bool HasConstructor = std::is_default_constructible<T>::value && !std::is_trivially_default_constructible<T>::value;
                bool HasDestructor = std::is_destructible<T>::value && !std::is_trivially_destructible<T>::value;
                bool HasAssignmentOperator = std::is_copy_assignable<T>::value && !std::is_trivially_copy_assignable<T>::value;
                bool HasCopyConstructor = std::is_copy_constructible<T>::value && !std::is_trivially_copy_constructible<T>::value;
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
                bool HasConstructor = std::is_default_constructible<T>::value && !std::has_trivial_default_constructor<T>::value;
				bool HasDestructor = std::is_destructible<T>::value && !std::is_trivially_destructible<T>::value;
				bool HasAssignmentOperator = std::is_copy_assignable<T>::value && !std::has_trivial_copy_assign<T>::value;
				bool HasCopyConstructor = std::is_copy_constructible<T>::value && !std::has_trivial_copy_constructor<T>::value;
#else
				bool HasConstructor = std::is_default_constructible<T>::value && !std::has_trivial_default_constructor<T>::value;
				bool HasDestructor = std::is_destructible<T>::value && !std::has_trivial_destructor<T>::value;
				bool HasAssignmentOperator = std::is_copy_assignable<T>::value && !std::has_trivial_copy_assign<T>::value;
				bool HasCopyConstructor = std::is_copy_constructible<T>::value && !std::has_trivial_copy_constructor<T>::value;
#endif
                bool IsFloat = std::is_floating_point<T>::value;
                bool IsPrimitive = std::is_integral<T>::value || std::is_pointer<T>::value || std::is_enum<T>::value;
                bool IsClass = std::is_class<T>::value;
                bool IsArray = std::is_array<T>::value;

                if (IsFloat)
                    return VMObjType_APP_FLOAT;

                if (IsPrimitive)
                    return VMObjType_APP_PRIMITIVE;

                if (IsClass)
                {
                    size_t Flags = VMObjType_APP_CLASS;
                    if (HasConstructor)
                        Flags |= VMObjType_APP_CLASS_CONSTRUCTOR;

                    if (HasDestructor)
                        Flags |= VMObjType_APP_CLASS_DESTRUCTOR;

                    if (HasAssignmentOperator)
                        Flags |= VMObjType_APP_CLASS_ASSIGNMENT;

                    if (HasCopyConstructor)
                        Flags |= VMObjType_APP_CLASS_COPY_CONSTRUCTOR;

                    return Flags;
                }

                if (IsArray)
                    return VMObjType_APP_ARRAY;

                return 0;
            }
		};

		struct TH_OUT VMByteCode
		{
			std::vector<unsigned char> Data;
			std::string Name;
			bool Valid = false;
			bool Debug = true;
		};

		struct TH_OUT VMProperty
		{
			const char* Name;
			const char* NameSpace;
			int TypeId;
			bool IsConst;
			const char* ConfigGroup;
			void* Pointer;
			size_t AccessMask;
		};

		struct TH_OUT VMFuncProperty
		{
			const char* Name;
			size_t AccessMask;
			int TypeId;
			int Offset;
			bool IsPrivate;
			bool IsProtected;
			bool IsReference;
		};

		struct TH_OUT VMMessage
		{
		private:
			asSMessageInfo* Info;

		public:
			VMMessage(asSMessageInfo* Info);
			const char* GetSection() const;
			const char* GetText() const;
			VMLogType GetType() const;
			int GetRow() const;
			int GetColumn() const;
			asSMessageInfo* GetMessageInfo() const;
			bool IsValid() const;
		};

		struct TH_OUT VMTypeInfo
		{
		private:
			VMManager* Manager;
			VMCTypeInfo* Info;

		public:
			VMTypeInfo(VMCTypeInfo* TypeInfo);
			void ForEachProperty(const PropertyCallback& Callback);
			void ForEachMethod(const MethodCallback& Callback);
			const char* GetGroup() const;
			size_t GetAccessMask() const;
			VMModule GetModule() const;
			int AddRef() const;
			int Release();
			const char* GetName() const;
			const char* GetNamespace() const;
			VMTypeInfo GetBaseType() const;
			bool DerivesFrom(const VMTypeInfo& Type) const;
			size_t GetFlags() const;
			unsigned int GetSize() const;
			int GetTypeId() const;
			int GetSubTypeId(unsigned int SubTypeIndex = 0) const;
			VMTypeInfo GetSubType(unsigned int SubTypeIndex = 0) const;
			unsigned int GetSubTypeCount() const;
			unsigned int GetInterfaceCount() const;
			VMTypeInfo GetInterface(unsigned int Index) const;
			bool Implements(const VMTypeInfo& Type) const;
			unsigned int GetFactoriesCount() const;
			VMFunction GetFactoryByIndex(unsigned int Index) const;
			VMFunction GetFactoryByDecl(const char* Decl) const;
			unsigned int GetMethodsCount() const;
			VMFunction GetMethodByIndex(unsigned int Index, bool GetVirtual = true) const;
			VMFunction GetMethodByName(const char* Name, bool GetVirtual = true) const;
			VMFunction GetMethodByDecl(const char* Decl, bool GetVirtual = true) const;
			unsigned int GetPropertiesCount() const;
			int GetProperty(unsigned int Index, VMFuncProperty* Out) const;
			const char* GetPropertyDeclaration(unsigned int Index, bool IncludeNamespace = false) const;
			unsigned int GetBehaviourCount() const;
			VMFunction GetBehaviourByIndex(unsigned int Index, VMBehave* OutBehaviour) const;
			unsigned int GetChildFunctionDefCount() const;
			VMTypeInfo GetChildFunctionDef(unsigned int Index) const;
			VMTypeInfo GetParentType() const;
			unsigned int GetEnumValueCount() const;
			const char* GetEnumValueByIndex(unsigned int Index, int* OutValue) const;
			VMFunction GetFunctionDefSignature() const;
			void* SetUserData(void* Data, uint64_t Type = 0);
			void* GetUserData(uint64_t Type = 0) const;
			bool IsHandle() const;
			bool IsValid() const;
			VMCTypeInfo* GetTypeInfo() const;
			VMManager* GetManager() const;

		public:
			template <typename T>
			T* GetInstance(void* Object)
			{
				if (!Object)
					return nullptr;

				return IsHandle() ? *(T**)Object : (T*)Object;
			}
			template <typename T>
			T* GetProperty(void* Object, int Offset)
			{
				if (!Object)
					return nullptr;

				if (!IsHandle())
					return reinterpret_cast<T*>(reinterpret_cast<char*>(Object) + Offset);

				if (!(*(void**)Object))
					return nullptr;

				return reinterpret_cast<T*>(reinterpret_cast<char*>(*(void**)Object) + Offset);
			}

		public:
			template <typename T>
			static T* GetInstance(void* Object, int TypeId)
			{
				if (!Object)
					return nullptr;

				return IsHandle(TypeId) ? *(T**)Object : (T*)Object;
			}
			template <typename T>
			static T* GetProperty(void* Object, int Offset, int TypeId)
			{
				if (!Object)
					return nullptr;

				if (!IsHandle(TypeId))
					return reinterpret_cast<T*>(reinterpret_cast<char*>(Object) + Offset);

				if (!(*(void**)Object))
					return nullptr;

				return reinterpret_cast<T*>(reinterpret_cast<char*>(*(void**)Object) + Offset);
			}

		public:
			static bool IsHandle(int TypeId);
			static bool IsScriptObject(int TypeId);
		};

		struct TH_OUT VMFunction
		{
		private:
			VMManager* Manager;
			VMCFunction* Function;

		public:
			VMFunction(VMCFunction* Base);
			int AddRef() const;
			int Release();
			int GetId() const;
			VMFuncType GetType() const;
			const char* GetModuleName() const;
			VMModule GetModule() const;
			const char* GetSectionName() const;
			const char* GetGroup() const;
			size_t GetAccessMask() const;
			VMTypeInfo GetObjectType() const;
			const char* GetObjectName() const;
			const char* GetName() const;
			const char* GetNamespace() const;
			const char* GetDecl(bool IncludeObjectName = true, bool IncludeNamespace = false, bool IncludeArgNames = false) const;
			bool IsReadOnly() const;
			bool IsPrivate() const;
			bool IsProtected() const;
			bool IsFinal() const;
			bool IsOverride() const;
			bool IsShared() const;
			bool IsExplicit() const;
			bool IsProperty() const;
			unsigned int GetArgsCount() const;
			int GetArg(unsigned int Index, int* TypeId, size_t* Flags = nullptr, const char** Name = nullptr, const char** DefaultArg = nullptr) const;
			int GetReturnTypeId(size_t* Flags = nullptr) const;
			int GetTypeId() const;
			bool IsCompatibleWithTypeId(int TypeId) const;
			void* GetDelegateObject() const;
			VMTypeInfo GetDelegateObjectType() const;
			VMFunction GetDelegateFunction() const;
			unsigned int GetPropertiesCount() const;
			int GetProperty(unsigned int Index, const char** Name, int* TypeId = nullptr) const;
			const char* GetPropertyDecl(unsigned int Index, bool IncludeNamespace = false) const;
			int FindNextLineWithCode(int Line) const;
			void* SetUserData(void* UserData, uint64_t Type = 0);
			void* GetUserData(uint64_t Type = 0) const;
			bool IsValid() const;
			VMCFunction* GetFunction() const;
			VMManager* GetManager() const;
		};

		struct TH_OUT VMObject
		{
		private:
			VMCObject* Object;

		public:
			VMObject(VMCObject* Base);
			int AddRef() const;
			int Release();
			VMTypeInfo GetObjectType();
			int GetTypeId();
			int GetPropertyTypeId(unsigned int Id) const;
			unsigned int GetPropertiesCount() const;
			const char* GetPropertyName(unsigned int Id) const;
			void* GetAddressOfProperty(unsigned int Id);
			VMManager* GetManager() const;
			int CopyFrom(const VMObject& Other);
			void* SetUserData(void* Data, uint64_t Type = 0);
			void* GetUserData(uint64_t Type = 0) const;
			bool IsValid() const;
			VMCObject* GetObject() const;
		};

		struct TH_OUT VMClass
		{
		protected:
			VMManager* Manager;
			std::string Object;
			int TypeId;

		public:
			VMClass(VMManager* Engine, const std::string& Name, int Type);
			int SetFunctionDef(const char* Decl);
			int SetOperatorCopyAddress(asSFuncPtr* Value);
			int SetBehaviourAddress(const char* Decl, VMBehave Behave, asSFuncPtr* Value, VMCall = VMCall_THISCALL);
			int SetPropertyAddress(const char* Decl, int Offset);
			int SetPropertyStaticAddress(const char* Decl, void* Value);
			int SetOperatorAddress(const char* Decl, asSFuncPtr* Value, VMCall Type = VMCall_THISCALL);
			int SetMethodAddress(const char* Decl, asSFuncPtr* Value, VMCall Type = VMCall_THISCALL);
			int SetMethodStaticAddress(const char* Decl, asSFuncPtr* Value, VMCall Type = VMCall_CDECL);
			int SetConstructorAddress(const char* Decl, asSFuncPtr* Value, VMCall Type = VMCall_CDECL_OBJFIRST);
			int SetConstructorListAddress(const char* Decl, asSFuncPtr* Value, VMCall Type = VMCall_CDECL_OBJFIRST);
			int SetDestructorAddress(const char* Decl, asSFuncPtr* Value);
			int GetTypeId() const;
			bool IsValid() const;
			std::string GetName() const;
			VMManager* GetManager() const;

		private:
			static Core::Parser GetOperator(VMOpFunc Op, const char* Out, const char* Args, bool Const, bool Right);

		public:
			template <typename T>
			int SetEnumRefs(void(T::*Value)(VMCManager*))
			{
				asSFuncPtr* EnumRefs = VMBridge::Method<T>(Value);
				int Result = SetBehaviourAddress("void f(int &in)", VMBehave_ENUMREFS, EnumRefs, VMCall_THISCALL);
                VMFuncStore::ReleaseFunctor(&EnumRefs);

				return Result;
			}
			template <typename T>
			int SetReleaseRefs(void(T::*Value)(VMCManager*))
			{
				asSFuncPtr* ReleaseRefs = VMBridge::Method<T>(Value);
				int Result = SetBehaviourAddress("void f(int &in)", VMBehave_RELEASEREFS, ReleaseRefs, VMCall_THISCALL);
                VMFuncStore::ReleaseFunctor(&ReleaseRefs);

				return Result;
			}
			template <typename T, typename R>
			int SetProperty(const char* Decl, R T::*Value)
			{
				return SetPropertyAddress(Decl, reinterpret_cast<std::size_t>(&(((T*)0)->*Value)));
			}
			template <typename T>
			int SetPropertyStatic(const char* Decl, T* Value)
			{
				return SetPropertyStaticAddress(Decl, (void*)Value);
			}
			template <typename T, typename R>
			int SetGetter(const char* Type, const char* Name, R(T::*Value)())
			{
				asSFuncPtr* Ptr = VMBridge::Method<T, R>(Value);
				int Result = SetMethodAddress(Core::Form("%s get_%s()", Type, Name).Get(), Ptr, VMCall_THISCALL);
                VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetGetterEx(const char* Type, const char* Name, R(*Value)(T*))
			{
				asSFuncPtr* Ptr = VMBridge::Function(Value);
				int Result = SetMethodAddress(Core::Form("%s get_%s()", Type, Name).Get(), Ptr, VMCall_CDECL_OBJFIRST);
                VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetSetter(const char* Type, const char* Name, void(T::*Value)(R))
			{
				asSFuncPtr* Ptr = VMBridge::Method<T, void, R>(Value);
				int Result = SetMethodAddress(Core::Form("void set_%s(%s)", Name, Type).Get(), Ptr, VMCall_THISCALL);
                VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetSetterEx(const char* Type, const char* Name, void(*Value)(T*, R))
			{
				asSFuncPtr* Ptr = VMBridge::Function(Value);
				int Result = SetMethodAddress(Core::Form("void set_%s(%s)", Name, Type).Get(), Ptr, VMCall_CDECL_OBJFIRST);
                VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetArrayGetter(const char* Type, const char* Name, R(T::*Value)(unsigned int))
			{
				asSFuncPtr* Ptr = VMBridge::Method<T, R, unsigned int>(Value);
				int Result = SetMethodAddress(Core::Form("%s get_%s(uint)", Type, Name).Get(), Ptr, VMCall_THISCALL);
                VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetArrayGetterEx(const char* Type, const char* Name, R(*Value)(T*, unsigned int))
			{
				asSFuncPtr* Ptr = VMBridge::Function(Value);
				int Result = SetMethodAddress(Core::Form("%s get_%s(uint)", Type, Name).Get(), Ptr, VMCall_CDECL_OBJFIRST);
                VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetArraySetter(const char* Type, const char* Name, void(T::*Value)(unsigned int, R))
			{
				asSFuncPtr* Ptr = VMBridge::Method<T, void, unsigned int, R>(Value);
				int Result = SetMethodAddress(Core::Form("void set_%s(uint, %s)", Name, Type).Get(), Ptr, VMCall_THISCALL);
                VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetArraySetterEx(const char* Type, const char* Name, void(*Value)(T*, unsigned int, R))
			{
				asSFuncPtr* Ptr = VMBridge::Function(Value);
				int Result = SetMethodAddress(Core::Form("void set_%s(uint, %s)", Name, Type).Get(), Ptr, VMCall_CDECL_OBJFIRST);
                VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R, typename... A>
			int SetOperator(VMOpFunc Type, VMOp Opts, const char* Out, const char* Args, R(T::*Value)(A...))
			{
				if (!Out)
					return -1;

				Core::Parser Operator = GetOperator(Type, Out, Args, Opts & VMOp_Const, Opts & VMOp_Right);
				if (Operator.Empty())
					return -1;

				asSFuncPtr* Ptr = VMBridge::Method<T, R, A...>(Value);
				int Result = SetOperatorAddress(Operator.Get(), Ptr, VMCall_THISCALL);
                VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename R, typename... A>
			int SetOperatorEx(VMOpFunc Type, VMOp Opts, const char* Out, const char* Args, R(*Value)(A...))
			{
				if (!Out)
					return -1;

				Core::Parser Operator = GetOperator(Type, Out, Args, Opts & VMOp_Const, Opts & VMOp_Right);
				if (Operator.Empty())
					return -1;

				asSFuncPtr* Ptr = VMBridge::Function(Value);
				int Result = SetOperatorAddress(Operator.Get(), Ptr, VMCall_CDECL_OBJFIRST);
                VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T>
			int SetOperatorCopy()
			{
				asSFuncPtr* Ptr = VMBridge::MethodOp<T, T&, const T&>(&T::operator =);
				int Result = SetOperatorCopyAddress(Ptr);
				VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R, typename... Args>
			int SetMethod(const char* Decl, R(T::*Value)(Args...))
			{
				asSFuncPtr* Ptr = VMBridge::Method<T, R, Args...>(Value);
				int Result = SetMethodAddress(Decl, Ptr, VMCall_THISCALL);
				VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R, typename... Args>
			int SetMethod(const char* Decl, R(T::*Value)(Args...) const)
			{
				asSFuncPtr* Ptr = VMBridge::Method<T, R, Args...>(Value);
				int Result = SetMethodAddress(Decl, Ptr, VMCall_THISCALL);
				VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename R, typename... Args>
			int SetMethodEx(const char* Decl, R(* Value)(Args...))
			{
				asSFuncPtr* Ptr = VMBridge::Function<R(*)(Args...)>(Value);
				int Result = SetMethodAddress(Decl, Ptr, VMCall_CDECL_OBJFIRST);
				VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename R, typename... Args>
			int SetMethodStatic(const char* Decl, R(*Value)(Args...), VMCall Type = VMCall_CDECL)
			{
				asSFuncPtr* Ptr = (Type == VMCall_GENERIC ? VMBridge::FunctionGeneric<R(*)(Args...)>(Value) : VMBridge::Function<R(*)(Args...)>(Value));
				int Result = SetMethodStaticAddress(Decl, Ptr, Type);
				VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
		};

		struct TH_OUT VMRefClass : public VMClass
		{
		public:
			VMRefClass(VMManager* Engine, const std::string& Name, int Type) : VMClass(Engine, Name, Type)
			{
			}

		public:
			template <typename T, const char* TypeName, typename... Args>
			int SetManagedConstructor(const char* Decl)
			{
				if (!Manager)
					return -1;

				asSFuncPtr* Functor = VMBridge::Function(&VMBridge::GetManagedCall<T, TypeName, Args...>);
				int Result = SetBehaviourAddress(Decl, VMBehave_FACTORY, Functor, VMCall_CDECL);
				VMFuncStore::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T, const char* TypeName, VMCGeneric*>
			int SetManagedConstructor(const char* Decl)
			{
				if (!Manager)
					return -1;

				asSFuncPtr* Functor = VMBridge::FunctionGeneric(&VMBridge::GetManagedCall<T, TypeName, VMCGeneric*>);
				int Result = SetBehaviourAddress(Decl, VMBehave_FACTORY, Functor, VMCall_GENERIC);
				VMFuncStore::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T, const char* TypeName>
			int SetManagedConstructorList(const char* Decl)
			{
				asSFuncPtr* Functor = VMBridge::FunctionGeneric(&VMBridge::GetManagedListCall<T, TypeName>);
				int Result = SetBehaviourAddress(Decl, VMBehave_LIST_FACTORY, Functor, VMCall_GENERIC);
				VMFuncStore::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T, typename... Args>
			int SetUnmanagedConstructor(const char* Decl)
			{
				if (!Manager)
					return -1;

				asSFuncPtr* Functor = VMBridge::Function(&VMBridge::GetUnmanagedCall<T, Args...>);
				int Result = SetBehaviourAddress(Decl, VMBehave_FACTORY, Functor, VMCall_CDECL);
				VMFuncStore::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T, VMCGeneric*>
			int SetUnmanagedConstructor(const char* Decl)
			{
				if (!Manager)
					return -1;

				asSFuncPtr* Functor = VMBridge::FunctionGeneric(&VMBridge::GetUnmanagedCall<T, VMCGeneric*>);
				int Result = SetBehaviourAddress(Decl, VMBehave_FACTORY, Functor, VMCall_GENERIC);
				VMFuncStore::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T>
			int SetUnmanagedConstructorList(const char* Decl)
			{
				asSFuncPtr* Functor = VMBridge::FunctionGeneric(&VMBridge::GetUnmanagedListCall<T>);
				int Result = SetBehaviourAddress(Decl, VMBehave_LIST_FACTORY, Functor, VMCall_GENERIC);
				VMFuncStore::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T>
			int SetUnmanagedConstructorListEx(const char* Decl, void(*Value)(VMCGeneric*))
			{
				asSFuncPtr* Functor = VMBridge::FunctionGeneric(Value);
				int Result = SetBehaviourAddress(Decl, VMBehave_LIST_FACTORY, Functor, VMCall_GENERIC);
				VMFuncStore::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T>
			int SetAddRef()
			{
				asSFuncPtr* AddRef = VMBridge::Function(&Core::Composer::AddRef);
				int Result = SetBehaviourAddress("void f()", VMBehave_ADDREF, AddRef, VMCall_CDECL_OBJFIRST);
				VMFuncStore::ReleaseFunctor(&AddRef);

				return Result;
			}
			template <typename T>
			int SetRelease()
			{
				asSFuncPtr* Release = VMBridge::Function(&Core::Composer::Release);
				int Result = SetBehaviourAddress("void f()", VMBehave_RELEASE, Release, VMCall_CDECL_OBJFIRST);
				VMFuncStore::ReleaseFunctor(&Release);

				return Result;
			}
			template <typename T>
			int SetGetRefCount()
			{
				asSFuncPtr* GetRefCount = VMBridge::Function(&Core::Composer::GetRefCount);
				int Result = SetBehaviourAddress("int f()", VMBehave_GETREFCOUNT, GetRefCount, VMCall_CDECL_OBJFIRST);
				VMFuncStore::ReleaseFunctor(&GetRefCount);

				return Result;
			}
			template <typename T>
			int SetSetGCFlag()
			{
				asSFuncPtr* SetGCFlag = VMBridge::Function(&Core::Composer::SetFlag);
				int Result = SetBehaviourAddress("void f()", VMBehave_SETGCFLAG, SetGCFlag, VMCall_CDECL_OBJFIRST);
				VMFuncStore::ReleaseFunctor(&SetGCFlag);

				return Result;
			}
			template <typename T>
			int SetGetGCFlag()
			{
				asSFuncPtr* GetGCFlag = VMBridge::Function(&Core::Composer::GetFlag);
				int Result = SetBehaviourAddress("bool f()", VMBehave_GETGCFLAG, GetGCFlag, VMCall_CDECL_OBJFIRST);
				VMFuncStore::ReleaseFunctor(&GetGCFlag);

				return Result;
			}
			template <typename T>
			int SetManaged(void(T::*EnumRefs)(VMCManager*), void(T::*ReleaseRefs)(VMCManager*))
			{
				int R = SetAddRef<T>();
				if (R < 0)
					return R;

				R = SetRelease<T>();
				if (R < 0)
					return R;

				R = SetGetRefCount<T>();
				if (R < 0)
					return R;

				R = SetSetGCFlag<T>();
				if (R < 0)
					return R;

				R = SetGetGCFlag<T>();
				if (R < 0)
					return R;

				R = SetEnumRefs<T>(EnumRefs);
				if (R < 0)
					return R;

				return SetReleaseRefs<T>(ReleaseRefs);
			}
			template <typename T>
			int SetUnmanaged()
			{
				int R = SetAddRef<T>();
				if (R < 0)
					return R;

				return SetRelease<T>();
			}
		};

		struct TH_OUT VMTypeClass : public VMClass
		{
		public:
			VMTypeClass(VMManager* Engine, const std::string& Name, int Type) : VMClass(Engine, Name, Type)
			{
			}

		public:
			template <typename T, typename... Args>
			int SetConstructor(const char* Decl)
			{
				asSFuncPtr* Ptr = VMBridge::Function(&VMBridge::GetConstructorCall<T, Args...>);
				int Result = SetConstructorAddress(Decl, Ptr, VMCall_CDECL_OBJFIRST);
				VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, VMCGeneric*>
			int SetConstructor(const char* Decl)
			{
				asSFuncPtr* Ptr = VMBridge::FunctionGeneric(&VMBridge::GetConstructorCall<T, VMCGeneric*>);
				int Result = SetConstructorAddress(Decl, Ptr, VMCall_GENERIC);
				VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T>
			int SetConstructorList(const char* Decl)
			{
				asSFuncPtr* Ptr = VMBridge::FunctionGeneric(&VMBridge::GetConstructorListCall<T>);
				int Result = SetConstructorListAddress(Decl, Ptr, VMCall_GENERIC);
				VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T>
			int SetDestructor(const char* Decl)
			{
				asSFuncPtr* Ptr = VMBridge::Function(&VMBridge::GetDestructorCall<T>);
				int Result = SetDestructorAddress(Decl, Ptr);
				VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename R, typename... Args>
			int SetDestructorStatic(const char* Decl, R(*Value)(Args...))
			{
				asSFuncPtr* Ptr = VMBridge::Function<R(*)(Args...)>(Value);
				int Result = SetDestructorAddress(Decl, Ptr);
				VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
		};

		struct TH_OUT VMInterface
		{
		private:
			VMManager* Manager;
			std::string Object;
			int TypeId;

		public:
			VMInterface(VMManager* Engine, const std::string& Name, int Type);
			int SetMethod(const char* Decl);
			int GetTypeId() const;
			bool IsValid() const;
			std::string GetName() const;
			VMManager* GetManager() const;
		};

		struct TH_OUT VMEnum
		{
		private:
			VMManager* Manager;
			std::string Object;
			int TypeId;

		public:
			VMEnum(VMManager* Engine, const std::string& Name, int Type);
			int SetValue(const char* Name, int Value);
			int GetTypeId() const;
			bool IsValid() const;
			std::string GetName() const;
			VMManager* GetManager() const;
		};

		struct TH_OUT VMModule
		{
		private:
			VMManager* Manager;
			VMCModule* Mod;

		public:
			VMModule(VMCModule* Type);
			void SetName(const char* Name);
			int AddSection(const char* Name, const char* Code, size_t CodeLength = 0, int LineOffset = 0);
			int RemoveFunction(const VMFunction& Function);
			int ResetProperties(VMCContext* Context = nullptr);
			int Build();
			int LoadByteCode(VMByteCode* Info);
			int Discard();
			int BindImportedFunction(size_t ImportIndex, const VMFunction& Function);
			int UnbindImportedFunction(size_t ImportIndex);
			int BindAllImportedFunctions();
			int UnbindAllImportedFunctions();
			int CompileFunction(const char* SectionName, const char* Code, int LineOffset, size_t CompileFlags, VMFunction* OutFunction);
			int CompileProperty(const char* SectionName, const char* Code, int LineOffset);
			int SetDefaultNamespace(const char* NameSpace);
			void* GetAddressOfProperty(size_t Index);
			int RemoveProperty(size_t Index);
			size_t SetAccessMask(size_t AccessMask);
			size_t GetFunctionCount() const;
			VMFunction GetFunctionByIndex(size_t Index) const;
			VMFunction GetFunctionByDecl(const char* Decl) const;
			VMFunction GetFunctionByName(const char* Name) const;
			int GetTypeIdByDecl(const char* Decl) const;
			int GetImportedFunctionIndexByDecl(const char* Decl) const;
			int SaveByteCode(VMByteCode* Info) const;
			int GetPropertyIndexByName(const char* Name) const;
			int GetPropertyIndexByDecl(const char* Decl) const;
			int GetProperty(size_t Index, VMProperty* Out) const;
			size_t GetAccessMask() const;
			size_t GetObjectsCount() const;
			size_t GetPropertiesCount() const;
			size_t GetEnumsCount() const;
			size_t GetImportedFunctionCount() const;
			VMTypeInfo GetObjectByIndex(size_t Index) const;
			VMTypeInfo GetTypeInfoByName(const char* Name) const;
			VMTypeInfo GetTypeInfoByDecl(const char* Decl) const;
			VMTypeInfo GetEnumByIndex(size_t Index) const;
			const char* GetPropertyDecl(size_t Index, bool IncludeNamespace = false) const;
			const char* GetDefaultNamespace() const;
			const char* GetImportedFunctionDecl(size_t ImportIndex) const;
			const char* GetImportedFunctionModule(size_t ImportIndex) const;
			const char* GetName() const;
			bool IsValid() const;
			VMCModule* GetModule() const;
			VMManager* GetManager() const;

		public:
			template <typename T>
			int SetTypeProperty(const char* Name, T* Value)
			{
				if (!Name)
					return -1;

				int Index = GetPropertyIndexByName(Name);
				if (Index < 0)
					return Index;

				T** Address = (T**)GetAddressOfProperty(Index);
				if (!Address)
					return -1;

				*Address = Value;
				return 0;
			}
			template <typename T>
			int SetTypeProperty(const char* Name, const T& Value)
			{
				if (!Name)
					return -1;

				int Index = GetPropertyIndexByName(Name);
				if (Index < 0)
					return Index;

				T* Address = (T*)GetAddressOfProperty(Index);
				if (!Address)
					return -1;

				*Address = Value;
				return 0;
			}
			template <typename T>
			int SetRefProperty(const char* Name, T* Value)
			{
				if (Name)
					return -1;

				int Index = GetPropertyIndexByName(Name);
				if (Index < 0)
					return Index;

				T** Address = (T**)GetAddressOfProperty(Index);
				if (!Address)
					return -1;

				if (*Address != nullptr)
					(*Address)->Release();

				*Address = (T*)Value;
				if (*Address != nullptr)
					(*Address)->AddRef();

				return 0;
			}
		};

		struct TH_OUT VMGlobal
		{
		private:
			VMManager* Manager;

		public:
			VMGlobal(VMManager* Engine);
			int SetFunctionDef(const char* Decl);
			int SetFunctionAddress(const char* Decl, asSFuncPtr* Value, VMCall Type = VMCall_CDECL);
			int SetPropertyAddress(const char* Decl, void* Value);
			VMTypeClass SetStructAddress(const char* Name, size_t Size, uint64_t Flags = VMObjType_VALUE);
			VMTypeClass SetPodAddress(const char* Name, size_t Size, uint64_t Flags = VMObjType_VALUE | VMObjType_POD);
			VMRefClass SetClassAddress(const char* Name, uint64_t Flags = VMObjType_REF);
			VMInterface SetInterface(const char* Name);
			VMEnum SetEnum(const char* Name);
			size_t GetFunctionsCount() const;
			VMFunction GetFunctionById(int Id) const;
			VMFunction GetFunctionByIndex(int Index) const;
			VMFunction GetFunctionByDecl(const char* Decl) const;
			size_t GetPropertiesCount() const;
			int GetPropertyByIndex(int Index, VMProperty* Info) const;
			int GetPropertyIndexByName(const char* Name) const;
			int GetPropertyIndexByDecl(const char* Decl) const;
			size_t GetObjectsCount() const;
			VMTypeInfo GetObjectByTypeId(int TypeId) const;
			size_t GetEnumCount() const;
			VMTypeInfo GetEnumByTypeId(int TypeId) const;
			size_t GetFunctionDefsCount() const;
			VMTypeInfo GetFunctionDefByIndex(int Index) const;
			size_t GetModulesCount() const;
			VMCModule* GetModuleById(int Id) const;
			int GetTypeIdByDecl(const char* Decl) const;
			const char* GetTypeIdDecl(int TypeId, bool IncludeNamespace = false) const;
			int GetSizeOfPrimitiveType(int TypeId) const;
			std::string GetObjectView(void* Object, int TypeId);
			VMTypeInfo GetTypeInfoById(int TypeId) const;
			VMTypeInfo GetTypeInfoByName(const char* Name) const;
			VMTypeInfo GetTypeInfoByDecl(const char* Decl) const;
			VMManager* GetManager() const;

		public:
			template <typename T>
			int SetFunction(const char* Decl, T Value)
			{
				asSFuncPtr* Ptr = VMBridge::Function<T>(Value);
				int Result = SetFunctionAddress(Decl, Ptr, VMCall_CDECL);
				VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <void(*)(VMCGeneric*)>
			int SetFunction(const char* Decl, void(* Value)(VMCGeneric*))
			{
				asSFuncPtr* Ptr = VMBridge::Function<void (*)(VMCGeneric*)>(Value);
				int Result = SetFunctionAddress(Decl, Ptr, VMCall_GENERIC);
				VMFuncStore::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T>
			int SetProperty(const char* Decl, T* Value)
			{
				return SetPropertyAddress(Decl, (void*)Value);
			}
			template <typename T>
			VMRefClass SetClassManaged(const char* Name, void(T::*EnumRefs)(VMCManager*), void(T::*ReleaseRefs)(VMCManager*))
			{
				VMRefClass Class = SetClassAddress(Name, VMObjType_REF | VMObjType_GC);
				Class.SetManaged<T>(EnumRefs, ReleaseRefs);

				return Class;
			}
			template <typename T>
			VMRefClass SetClassUnmanaged(const char* Name)
			{
				VMRefClass Class = SetClassAddress(Name, VMObjType_REF);
				Class.SetUnmanaged<T>();

				return Class;
			}
			template <typename T>
			VMTypeClass SetStructManaged(const char* Name, void(T::*EnumRefs)(VMCManager*), void(T::* ReleaseRefs)(VMCManager*))
			{
				VMTypeClass Struct = SetStructAddress(Name, sizeof(T), VMObjType_VALUE | VMObjType_GC | VMBridge::GetTypeTraits<T>());
				Struct.SetEnumRefs(EnumRefs);
				Struct.SetReleaseRefs(ReleaseRefs);
				Struct.SetDestructor<T>("void f()");

				return Struct;
			}
			template <typename T>
			VMTypeClass SetStructUnmanaged(const char* Name)
			{
				VMTypeClass Struct = SetStructAddress(Name, sizeof(T), VMObjType_VALUE | VMBridge::GetTypeTraits<T>());
				Struct.SetOperatorCopy<T>();
				Struct.SetDestructor<T>("void f()");

				return Struct;
			}
			template <typename T>
			VMTypeClass SetPod(const char* Name)
			{
				return SetPodAddress(Name, sizeof(T), VMObjType_VALUE | VMObjType_POD | VMBridge::GetTypeTraits<T>());
			}
		};

		class TH_OUT VMCompiler : public Core::Object
		{
		private:
			static int CompilerUD;

		private:
			Compute::ProcIncludeCallback Include;
			Compute::ProcPragmaCallback Pragma;
			Compute::Preprocessor* Processor;
			asIScriptModule* Module;
			VMManager* Manager;
			VMContext* Context;
			VMByteCode VCache;
			bool BuiltOK;

		public:
			VMCompiler(VMManager* Engine);
			~VMCompiler();
			void SetIncludeCallback(const Compute::ProcIncludeCallback& Callback);
			void SetPragmaCallback(const Compute::ProcPragmaCallback& Callback);
			void Define(const std::string& Word);
			void Undefine(const std::string& Word);
			bool Clear();
			bool IsPending();
			bool IsDefined(const std::string& Word);
			bool IsBuilt();
			bool IsCached();
			int Prepare(const std::string& ModuleName, bool Scoped = false);
			int Prepare(const std::string& ModuleName, const std::string& Cache, bool Debug = true, bool Scoped = false);
			int Compile(bool Await);
			int SaveByteCode(VMByteCode* Info);
			int LoadByteCode(VMByteCode* Info);
			int LoadFile(const std::string& Path);
			int LoadCode(const std::string& Name, const std::string& Buffer);
			int LoadCode(const std::string& Name, const char* Buffer, uint64_t Length);
			int ExecuteFile(const char* Name, const char* ModuleName, const char* EntryName, void* Return = nullptr, int ReturnTypeId = VMTypeId_VOID);
			int ExecuteMemory(const std::string& Buffer, const char* ModuleName, const char* EntryName, void* Return = nullptr, int ReturnTypeId = VMTypeId_VOID);
			int ExecuteEntry(const char* Name, void* Return = nullptr, int ReturnTypeId = VMTypeId_VOID);
			int ExecuteEntry(const char* Name, void* Return, int ReturnTypeId, const std::function<void(VMContext*)>& ArgsCallback);
			int ExecuteScoped(const std::string& Code, void* Return = nullptr, int ReturnTypeId = VMTypeId_VOID);
			int ExecuteScoped(const char* Buffer, uint64_t Length, void* Return = nullptr, int ReturnTypeId = VMTypeId_VOID);
			VMModule GetModule() const;
			VMManager* GetManager() const;
			VMContext* GetContext() const;
			Compute::Preprocessor* GetProcessor() const;

		private:
			static VMCompiler* Get(VMContext* Context);
		};

		class TH_OUT VMContext : public Core::Object
		{
		private:
			static int ContextUD;

		private:
			std::atomic<uint64_t> Async;
			VMCContext* Context;
			VMManager* Manager;

		public:
			VMContext(VMCContext* Base);
			~VMContext();
			int SetExceptionCallback(void(* Callback)(VMCContext* Context, void* Object), void* Object);
			int AddRefVM() const;
			int ReleaseVM();
			int Prepare(const VMFunction& Function);
			int Unprepare();
			int Execute();
			int Resume();
			int Abort();
			int Suspend();
			VMExecState GetState() const;
			std::string GetStackTrace() const;
			int PushState();
			int PopState();
			int PushAsync();
			int PopAsync();
			bool IsPending();
			bool IsNested(unsigned int* NestCount = 0) const;
			int SetObject(void* Object);
			int SetArg8(unsigned int Arg, unsigned char Value);
			int SetArg16(unsigned int Arg, unsigned short Value);
			int SetArg32(unsigned int Arg, int Value);
			int SetArg64(unsigned int Arg, int64_t Value);
			int SetArgFloat(unsigned int Arg, float Value);
			int SetArgDouble(unsigned int Arg, double Value);
			int SetArgAddress(unsigned int Arg, void* Address);
			int SetArgObject(unsigned int Arg, void* Object);
			int SetArgAny(unsigned int Arg, void* Ptr, int TypeId);
			void* GetAddressOfArg(unsigned int Arg);
			unsigned char GetReturnByte();
			unsigned short GetReturnWord();
			size_t GetReturnDWord();
			uint64_t GetReturnQWord();
			float GetReturnFloat();
			double GetReturnDouble();
			void* GetReturnAddress();
			void* GetReturnObjectAddress();
			void* GetAddressOfReturnValue();
			int SetException(const char* Info, bool AllowCatch = true);
			int GetExceptionLineNumber(int* Column = 0, const char** SectionName = 0);
			VMFunction GetExceptionFunction();
			const char* GetExceptionString();
			bool WillExceptionBeCaught();
			void ClearExceptionCallback();
			int SetLineCallback(void(* Callback)(VMCContext* Context, void* Object), void* Object);
			void ClearLineCallback();
			unsigned int GetCallstackSize() const;
			VMFunction GetFunction(unsigned int StackLevel = 0);
			int GetLineNumber(unsigned int StackLevel = 0, int* Column = 0, const char** SectionName = 0);
			int GetPropertiesCount(unsigned int StackLevel = 0);
			const char* GetPropertyName(unsigned int Index, unsigned int StackLevel = 0);
			const char* GetPropertyDecl(unsigned int Index, unsigned int StackLevel = 0, bool IncludeNamespace = false);
			int GetPropertyTypeId(unsigned int Index, unsigned int StackLevel = 0);
			void* GetAddressOfProperty(unsigned int Index, unsigned int StackLevel = 0);
			bool IsPropertyInScope(unsigned int Index, unsigned int StackLevel = 0);
			int GetThisTypeId(unsigned int StackLevel = 0);
			void* GetThisPointer(unsigned int StackLevel = 0);
			VMFunction GetSystemFunction();
			bool IsSuspended() const;
			void* SetUserData(void* Data, uint64_t Type = 0);
			void* GetUserData(uint64_t Type = 0) const;
			VMCContext* GetContext();
			VMManager* GetManager();

		public:
			template <typename T>
			T* GetReturnObject()
			{
				return (T*)GetReturnObjectAddress();
			}

		public:
			static VMContext* Get(VMCContext* Context);
			static VMContext* Get();

		private:
			static void ExceptionLogger(VMCContext* Context, void* Object);
		};

		class TH_OUT VMManager : public Core::Object
		{
		private:
			struct Kernel
			{
				std::unordered_map<std::string, void*> Functions;
				void* Handle;
			};

			struct Submodule
			{
				std::vector<std::string> Dependencies;
				SubmoduleCallback Callback;
				bool Registered;
			};

		private:
			static int ManagerUD;

		private:
			std::unordered_map<std::string, std::string> Files;
			std::unordered_map<std::string, Core::Document*> Datas;
			std::unordered_map<std::string, VMByteCode> Opcodes;
			std::unordered_map<std::string, Kernel> Kernels;
			std::unordered_map<std::string, Submodule> Modules;
			std::vector<VMCContext*> Contexts;
			std::string DefaultNamespace;
			Compute::Preprocessor::Desc Proc;
			Compute::IncludeDesc Include;
			std::mutex Safe;
			uint64_t Scope;
			VMCManager* Engine;
			VMGlobal Globals;
			unsigned int Imports;
			int Nullable;
			void* JIT;
			bool Cached;

		public:
			VMManager();
			~VMManager();
			void SetImports(unsigned int Opts);
			void SetJIT(unsigned int Opts);
			void SetCache(bool Enabled);
			void ClearCache();
			void Lock();
			void Unlock();
			void SetCompilerIncludeOptions(const Compute::IncludeDesc& NewDesc);
			void SetCompilerFeatures(const Compute::Preprocessor::Desc& NewDesc);
			void SetProcessorOptions(Compute::Preprocessor* Processor);
			int Collect(size_t NumIterations = 1);
			void GetStatistics(unsigned int* CurrentSize, unsigned int* TotalDestroyed, unsigned int* TotalDetected, unsigned int* NewObjects, unsigned int* TotalNewDestroyed) const;
			int NotifyOfNewObject(void* Object, const VMTypeInfo& Type);
			int GetObjectAddress(size_t Index, size_t* SequenceNumber = nullptr, void** Object = nullptr, VMTypeInfo* Type = nullptr);
			void ForwardEnumReferences(void* Reference, const VMTypeInfo& Type);
			void ForwardReleaseReferences(void* Reference, const VMTypeInfo& Type);
			void GCEnumCallback(void* Reference);
			bool GetByteCodeCache(VMByteCode* Info);
			void SetByteCodeCache(VMByteCode* Info);
			VMContext* CreateContext();
			VMCompiler* CreateCompiler();
			VMCModule* CreateScopedModule(const std::string& Name, bool AqLock = true);
			VMCModule* CreateModule(const std::string& Name, bool AqLock = true);
			void* CreateObject(const VMTypeInfo& Type);
			void* CreateObjectCopy(void* Object, const VMTypeInfo& Type);
			void* CreateEmptyObject(const VMTypeInfo& Type);
			VMFunction CreateDelegate(const VMFunction& Function, void* Object);
			int AssignObject(void* DestObject, void* SrcObject, const VMTypeInfo& Type);
			void ReleaseObject(void* Object, const VMTypeInfo& Type);
			void AddRefObject(void* Object, const VMTypeInfo& Type);
			int RefCastObject(void* Object, const VMTypeInfo& FromType, const VMTypeInfo& ToType, void** NewPtr, bool UseOnlyImplicitCast = false);
			int BeginGroup(const char* GroupName);
			int EndGroup();
			int RemoveGroup(const char* GroupName);
			int BeginNamespace(const char* Namespace);
			int BeginNamespaceIsolated(const char* Namespace, size_t DefaultMask);
			int EndNamespace();
			int EndNamespaceIsolated();
			int Namespace(const char* Namespace, const std::function<int(VMGlobal*)>& Callback);
			int NamespaceIsolated(const char* Namespace, size_t DefaultMask, const std::function<int(VMGlobal*)>& Callback);
			int GetTypeNameScope(const char** TypeName, const char** Namespace, size_t* NamespaceSize) const;
			size_t BeginAccessMask(size_t DefaultMask);
			size_t EndAccessMask();
			const char* GetNamespace() const;
			VMGlobal& Global();
			VMModule Module(const char* Name);
			int SetLogCallback(void(* Callback)(const asSMessageInfo* Message, void* Object), void* Object);
			int Log(const char* Section, int Row, int Column, VMLogType Type, const char* Message);
			int AddRefVM() const;
			int ReleaseVM();
			int SetProperty(VMProp Property, size_t Value);
			void SetDocumentRoot(const std::string& Root);
			size_t GetProperty(VMProp Property) const;
			VMCManager* GetEngine() const;
			std::string GetDocumentRoot() const;
			std::vector<std::string> GetSubmodules() const;
			std::vector<std::string> VerifyModules(const std::string& Directory, const Compute::RegExp& Exp);
			bool VerifyModule(const std::string& Path);
			bool IsNullable(int TypeId);
			bool AddSubmodule(const std::string& Name, const std::vector<std::string>& Dependencies, const SubmoduleCallback& Callback);
			bool ImportFile(const std::string& Path, std::string* Out);
			bool ImportSymbol(const std::vector<std::string>& Sources, const std::string& Name, const std::string& Decl);
			bool ImportLibrary(const std::string& Path);
			bool ImportSubmodule(const std::string& Name);
			Core::Document* ImportJSON(const std::string& Path);

		public:
			static void SetMemoryFunctions(void*(*Alloc)(size_t), void(*Free)(void*));
			static VMManager* Get(VMCManager* Engine);
			static VMManager* Get();
			static size_t GetDefaultAccessMask();
			static void FreeProxy();

		private:
			static std::string GetLibraryName(const std::string& Path);
			static VMCContext* RequestContext(VMCManager* Engine, void* Data);
			static void ReturnContext(VMCManager* Engine, VMCContext* Context, void* Data);
			static void CompileLogger(asSMessageInfo* Info, void* Object);
			static void RegisterSubmodules(VMManager* Engine);
			static void* GetNullable();
		};

		class TH_OUT VMDebugger : public Core::Object
		{
		public:
			typedef std::string(* ToStringCallback)(void* Object, int ExpandLevel, VMDebugger* Dbg);

		protected:
			enum DebugAction
			{
				CONTINUE,
				STEP_INTO,
				STEP_OVER,
				STEP_OUT
			};

			struct BreakPoint
			{
				std::string Name;
				bool NeedsAdjusting;
				bool Function;
				int Line;

				BreakPoint(const std::string& N, int L, bool F) : Name(N), NeedsAdjusting(true), Function(F), Line(L)
				{
				}
			};

		protected:
			std::unordered_map<const VMCTypeInfo*, ToStringCallback> ToStringCallbacks;
			std::vector<BreakPoint> BreakPoints;
			unsigned int LastCommandAtStackLevel;
			VMCFunction* LastFunction;
			VMManager* Manager;
			DebugAction Action;

		public:
			VMDebugger();
			~VMDebugger();
			void RegisterToStringCallback(const VMTypeInfo& Type, ToStringCallback Callback);
			void TakeCommands(VMContext* Context);
			void Output(const std::string& Data);
			void LineCallback(VMContext* Context);
			void PrintHelp();
			void AddFileBreakPoint(const std::string& File, int LineNumber);
			void AddFuncBreakPoint(const std::string& Function);
			void ListBreakPoints();
			void ListLocalVariables(VMContext* Context);
			void ListGlobalVariables(VMContext* Context);
			void ListMemberProperties(VMContext* Context);
			void ListStatistics(VMContext* Context);
			void PrintCallstack(VMContext* Context);
			void PrintValue(const std::string& Expression, VMContext* Context);
			void SetEngine(VMManager* Engine);
			bool InterpretCommand(const std::string& Command, VMContext* Context);
			bool CheckBreakPoint(VMContext* Context);
			std::string ToString(void* Value, unsigned int TypeId, int ExpandMembersLevel, VMManager* Engine);
			VMManager* GetEngine();
		};

		inline VMOp operator |(VMOp A, VMOp B)
		{
			return static_cast<VMOp>(static_cast<uint64_t>(A) | static_cast<uint64_t>(B));
		}
	}
}
#endif