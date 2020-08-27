#ifndef THAWK_SCRIPT_H
#define THAWK_SCRIPT_H
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

class VMCArray;
class VMCAny;
class VMCDictionary;
class VMCGrid;
class VMCWeakRef;
class VMCRef;
class VMCJITCompiler;
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
		struct VMWModule;

		struct VMWFunction;

		class VMCompiler;

		class VMManager;

		class VMContext;

		class VMDummy
		{
		};

		enum VMOptJIT
		{
			VMOptJIT_No_Suspend = 0x01,
			VMOptJIT_Syscall_FPU_No_Reset = 0x02,
			VMOptJIT_Syscall_No_Errors = 0x04,
			VMOptJIT_Alloc_Simple = 0x08,
			VMOptJIT_No_Switches = 0x10,
			VMOptJIT_No_Script_Calls = 0x20,
			VMOptJIT_Fast_Ref_Counter = 0x40,
			VMOptJIT_Optimal = VMOptJIT_Alloc_Simple | VMOptJIT_Fast_Ref_Counter
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

		enum VMFeature
		{
			VMFeature_All = 0,
			VMFeature_String = (1 << 0),
			VMFeature_Array = (1 << 1),
			VMFeature_Any = (1 << 2),
			VMFeature_Dictionary = (1 << 3),
			VMFeature_Grid = (1 << 4),
			VMFeature_Math = (1 << 5),
			VMFeature_DateTime = (1 << 6),
			VMFeature_Exception = (1 << 7),
			VMFeature_Reference = (1 << 8),
			VMFeature_WeakReference = (1 << 9),
			VMFeature_Random = (1 << 10),
			VMFeature_Thread = (1 << 11),
			VMFeature_Async = (1 << 12),
			VMFeature_Complex = (1 << 13),
			VMFeature_Rest = (1 << 14)
		};

		typedef asIScriptEngine VMCManager;
		typedef asIScriptContext VMCContext;
		typedef asIScriptModule VMCModule;
		typedef asITypeInfo VMCTypeInfo;
		typedef asIScriptFunction VMCFunction;
		typedef asIScriptGeneric VMCGeneric;
		typedef asIScriptObject VMCObject;
		typedef asILockableSharedBool VMCLockableSharedBool;
		typedef void (VMDummy::*VMMethodPtr)();
		typedef std::function<void(class VMCAsync*)> AsyncWorkCallback;
		typedef std::function<void(enum VMExecState)> AsyncDoneCallback;
		typedef std::function<void(struct VMWTypeInfo*, struct VMFuncProperty*)> PropertyCallback;
		typedef std::function<void(struct VMWTypeInfo*, struct VMWFunction*)> MethodCallback;

		class THAWK_OUT VMWGeneric
		{
		private:
			VMManager* Manager;
			VMCGeneric* Generic;

		public:
			VMWGeneric(VMCGeneric* Base);
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

        class THAWK_OUT VMCReceiver
        {
            friend class VMCThread;

        private:
            std::vector<VMCAny*> Queue;
            std::condition_variable CV;
            std::mutex Mutex;
            int Debug;

        public:
            VMCReceiver();
            void Send(VMCAny* Any);
            void Release();
            int GetQueueSize();
            VMCAny* ReceiveWait();
            VMCAny* Receive(uint64_t Timeout);

        private:
            void EnumReferences(VMCManager* Engine);
        };

        class THAWK_OUT VMCThread
        {
        private:
            static int ContextUD;
            static int EngineListUD;

        private:
            VMCFunction* Function;
            VMManager* Manager;
            VMContext* Context;
            std::condition_variable CV;
            std::thread Thread;
            std::mutex Mutex;
            VMCReceiver Incoming;
            VMCReceiver Outgoing;
            bool Active;
            int Ref;
            bool GCFlag;

        public:
            VMCThread(VMCManager* Engine, VMCFunction* Function);
            void AddRef();
            void Release();
            void Suspend();
            void Send(VMCAny* Any);
            int Wait(uint64_t Timeout);
            int Join();
            bool IsActive();
            bool Start();
            VMCAny* ReceiveWait();
            VMCAny* Receive(uint64_t Timeout);
            void EnumReferences(VMCManager* Engine);
            void SetGCFlag();
            void ReleaseReferences(VMCManager* Engine);
            void StartRoutineThread();
            bool GetGCFlag();
            int GetRefCount();

        public:
            static int AtomicNotifyGC(const char* TypeName, void* Object);
            static void StartRoutine(VMCThread* Thread);
            static void SendInThread(VMCAny* Any);
            static void SleepInThread(uint64_t Timeout);
            static VMCThread* GetThisThread();
            static VMCThread* GetThisThreadSafe();
            static VMCAny* ReceiveWaitInThread();
            static VMCAny* ReceiveInThread(uint64_t Timeout);
            static VMCThread* StartThread(VMCFunction* Func);
            static uint64_t GetIdInThread();
        };

		class THAWK_OUT VMBind
		{
		public:
			template <int N>
			struct THAWK_OUT Method
			{
				template <class M>
				static asSFuncPtr* Bind(M Value)
				{
					int ERROR_UnsupportedMethodPtr[N - 100];
					return VMBind::CreateDummyBase();
				}
			};

			template <>
			struct THAWK_OUT Method<sizeof(VMMethodPtr)>
			{
				template <class M>
				static asSFuncPtr* Bind(M Value)
				{
					return VMBind::CreateMethodBase(&Value, sizeof(VMMethodPtr), 3);
				}
			};

#if defined(_MSC_VER) && !defined(__MWERKS__)

			template <>
			struct THAWK_OUT Method<sizeof(VMMethodPtr) + 1 * sizeof(int)>
			{
				template <class M>
				static asSFuncPtr* Bind(M Value)
				{
					return VMBind::CreateMethodBase(&Value, sizeof(VMMethodPtr) + sizeof(int), 3);
				}
			};

			template <>
			struct THAWK_OUT Method<sizeof(VMMethodPtr) + 2 * sizeof(int)>
			{
				template <class M>
				static asSFuncPtr* Bind(M Value)
				{
					asSFuncPtr* Ptr = VMBind::CreateMethodBase(&Value, sizeof(VMMethodPtr) + 2 * sizeof(int), 3);
#if defined(_MSC_VER) && !defined(THAWK_64)
					*(reinterpret_cast<unsigned long*>(Ptr) + 3) = *(reinterpret_cast<unsigned long*>(Ptr) + 2);
#endif
					return Ptr;
				}
			};

			template <>
			struct THAWK_OUT Method<sizeof(VMMethodPtr) + 3 * sizeof(int)>
			{
				template <class M>
				static asSFuncPtr* Bind(M Value)
				{
					return VMBind::CreateMethodBase(&Value, sizeof(VMMethodPtr) + 3 * sizeof(int), 3);
				}
			};

			template <>
			struct THAWK_OUT Method<sizeof(VMMethodPtr) + 4 * sizeof(int)>
			{
				template <class M>
				static asSFuncPtr* Bind(M Value)
				{
					return VMBind::CreateMethodBase(&Value, sizeof(VMMethodPtr) + 4 * sizeof(int), 3);
				}
			};

#endif

		public:
			static asSFuncPtr* CreateFunctionBase(void(* Base)(), int Type);
			static asSFuncPtr* CreateMethodBase(const void* Base, size_t Size, int Type);
			static asSFuncPtr* CreateDummyBase();
			static void ReleaseFunctor(asSFuncPtr** Ptr);

		public:
			template <typename T>
			static size_t GetTypeTraits()
			{
#if defined(_MSC_VER) || defined(_LIBCPP_TYPE_TRAITS) || (__GNUC__ >= 5) || defined(__clang__)
				bool hasConstructor = std::is_default_constructible<T>::value && !std::is_trivially_default_constructible<T>::value;
				bool hasDestructor = std::is_destructible<T>::value && !std::is_trivially_destructible<T>::value;
				bool hasAssignmentOperator = std::is_copy_assignable<T>::value && !std::is_trivially_copy_assignable<T>::value;
				bool hasCopyConstructor = std::is_copy_constructible<T>::value && !std::is_trivially_copy_constructible<T>::value;
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
				bool hasConstructor = std::is_default_constructible<T>::value && !std::has_trivial_default_constructor<T>::value;
				bool hasDestructor = std::is_destructible<T>::value && !std::is_trivially_destructible<T>::value;
				bool hasAssignmentOperator = std::is_copy_assignable<T>::value && !std::has_trivial_copy_assign<T>::value;
				bool hasCopyConstructor = std::is_copy_constructible<T>::value && !std::has_trivial_copy_constructor<T>::value;
#else
				bool hasConstructor = std::is_default_constructible<T>::value && !std::has_trivial_default_constructor<T>::value;
				bool hasDestructor = std::is_destructible<T>::value && !std::has_trivial_destructor<T>::value;
				bool hasAssignmentOperator = std::is_copy_assignable<T>::value && !std::has_trivial_copy_assign<T>::value;
				bool hasCopyConstructor = std::is_copy_constructible<T>::value && !std::has_trivial_copy_constructor<T>::value;
#endif
				bool isFloat = std::is_floating_point<T>::value;
				bool isPrimitive = std::is_integral<T>::value || std::is_pointer<T>::value || std::is_enum<T>::value;
				bool isClass = std::is_class<T>::value;
				bool isArray = std::is_array<T>::value;

				if (isFloat)
					return VMObjType_APP_FLOAT;

				if (isPrimitive)
					return VMObjType_APP_PRIMITIVE;

				if (isClass)
				{
					size_t flags = VMObjType_APP_CLASS;
					if (hasConstructor)
						flags |= VMObjType_APP_CLASS_CONSTRUCTOR;

					if (hasDestructor)
						flags |= VMObjType_APP_CLASS_DESTRUCTOR;

					if (hasAssignmentOperator)
						flags |= VMObjType_APP_CLASS_ASSIGNMENT;

					if (hasCopyConstructor)
						flags |= VMObjType_APP_CLASS_COPY_CONSTRUCTOR;

					return flags;
				}

				if (isArray)
					return VMObjType_APP_ARRAY;

				return 0;
			}
			template <typename T>
			static asSFuncPtr* BindFunction(T Value)
			{
#ifdef THAWK_64
				void(*Address)() = reinterpret_cast<void(*)()>(size_t(Value));
#else
				void (* Address)() = reinterpret_cast<void (*)()>(Value);
#endif
				return VMBind::CreateFunctionBase(Address, 2);
			}
			template <typename T>
			static asSFuncPtr* BindFunctionGeneric(T Value)
			{
#ifdef THAWK_64
				void(*Address)() = reinterpret_cast<void(*)()>(size_t(Value));
#else
				void(*Address)() = reinterpret_cast<void(*)()>(Value);
#endif
				return VMBind::CreateFunctionBase(Address, 1);
			}
			template <typename T, typename R, typename... Args>
			static asSFuncPtr* BindMethod(R(T::*Value)(Args...))
			{
				return Method<sizeof(void (T::*)())>::Bind((void (T::*)())(Value));
			}
			template <typename T, typename R, typename... Args>
			static asSFuncPtr* BindMethod(R(T::*Value)(Args...) const)
			{
				return Method<sizeof(void (T::*)())>::Bind((void (T::*)())(Value));
			}
			template <typename T, typename R, typename... Args>
			static asSFuncPtr* BindMethodOp(R(T::*Value)(Args...))
			{
				return Method<sizeof(void (T::*)())>::Bind(static_cast<R(T::*)(Args...)>(Value));
			}
			template <typename T, typename R, typename... Args>
			static asSFuncPtr* BindMethodOp(R(T::*Value)(Args...) const)
			{
				return Method<sizeof(void (T::*)())>::Bind(static_cast<R(T::*)(Args...)>(Value));
			}
			template <typename T, typename... Args>
			static void BindConstructor(void* Memory, Args... Data)
			{
				new(Memory) T(Data...);
			}
			template <typename T>
			static void BindConstructorList(VMCGeneric* Generic)
			{
				VMWGeneric Args(Generic);
				*reinterpret_cast<T**>(Args.GetAddressOfReturnLocation()) = new T((unsigned char*)Args.GetArgAddress(0));
			}
			template <typename T>
			static void BindDestructor(void* Memory)
			{
				((T*)Memory)->~T();
			}
			template <typename T, const char* TypeName, typename... Args>
			static T* BindManaged(Args... Data)
			{
				auto* Result = new T(Data...);
				VMCThread::AtomicNotifyGC(TypeName, (void*)Result);

				return Result;
			}
			template <typename T, const char* TypeName>
			static void BindManagedList(VMCGeneric* Generic)
			{
				VMWGeneric Args(Generic);
				T* Result = new T((unsigned char*)Args.GetArgAddress(0));
				*reinterpret_cast<T**>(Args.GetAddressOfReturnLocation()) = Result;
				VMCThread::AtomicNotifyGC(TypeName, (void*)Result);
			}
			template <typename T, typename... Args>
			static T* BindUnmanaged(Args... Data)
			{
				return new T(Data...);
			}
			template <typename T>
			static void BindUnmanagedList(VMCGeneric* Generic)
			{
				VMWGeneric Args(Generic);
				*reinterpret_cast<T**>(Args.GetAddressOfReturnLocation()) = new T((unsigned char*)Args.GetArgAddress(0));
			}
		};

		class THAWK_OUT VMCRandom
		{
		private:
			std::mt19937 Twister;
			std::uniform_real_distribution<double> DDist = std::uniform_real_distribution<double>(0.0, 1.0);
			int Ref = 1;

		public:
			VMCRandom();
			void AddRef();
			void Release();
			void SeedFromTime();
			uint32_t GetU();
			int32_t GetI();
			double GetD();
			void Seed(uint32_t Seed);
			void Seed(VMCArray* Array);
			void Assign(VMCRandom* From);

		public:
			static VMCRandom* Create();
		};

		class THAWK_OUT VMCAsync
		{
		private:
			AsyncDoneCallback Done;
			VMCContext* Context;
			VMCAny* Any;
			std::mutex Mutex;
			bool Rejected;
			bool GCFlag;
			int Ref;

		private:
			VMCAsync(VMCContext* Base);

		public:
			void EnumReferences(VMCManager* Engine);
			void ReleaseReferences(VMCManager* Engine);
			void AddRef();
			void Release();
			void SetGCFlag();
			bool GetGCFlag();
			int GetRefCount();
			int Set(VMCAny* Value);
			int Set(const struct VMWAny& Value);
			int Set(void* Ref, int TypeId);
			int Set(void* Ref, const char* TypeName);
			bool GetAny(void* Ref, int TypeId) const;
			VMCAny* Get() const;
			VMCAsync* Await();

		public:
			static VMCAsync* Create(const AsyncWorkCallback& WorkCallback, const AsyncDoneCallback& DoneCallback);
			static VMCAsync* CreateFilled(void* Ref, int TypeId);
			static VMCAsync* CreateFilled(void* Ref, const char* TypeName);
			static VMCAsync* CreateFilled(bool Value);
			static VMCAsync* CreateFilled(int64_t Value);
			static VMCAsync* CreateFilled(double Value);
			static VMCAsync* CreateEmpty();
		};

		struct THAWK_OUT VMByteCode
		{
			std::vector<unsigned char> Data;
			std::string Name;
			bool Valid = false;
			bool Debug = true;
		};

		struct THAWK_OUT VMProperty
		{
			const char* Name;
			const char* NameSpace;
			int TypeId;
			bool IsConst;
			const char* ConfigGroup;
			void* Pointer;
			size_t AccessMask;
		};

		struct THAWK_OUT VMFuncProperty
		{
			const char* Name;
			size_t AccessMask;
			int TypeId;
			int Offset;
			bool IsPrivate;
			bool IsProtected;
			bool IsReference;
		};

		struct THAWK_OUT VMWMessage
		{
		private:
			asSMessageInfo* Info;

		public:
			VMWMessage(asSMessageInfo* Info);
			const char* GetSection() const;
			const char* GetMessage() const;
			VMLogType GetType() const;
			int GetRow() const;
			int GetColumn() const;
			asSMessageInfo* GetMessageInfo() const;
			bool IsValid() const;
		};

		struct THAWK_OUT VMWTypeInfo
		{
		private:
			VMManager* Manager;
			VMCTypeInfo* Info;

		public:
			VMWTypeInfo(VMCTypeInfo* TypeInfo);
			void ForEachProperty(const PropertyCallback& Callback);
			void ForEachMethod(const MethodCallback& Callback);
			const char* GetGroup() const;
			size_t GetAccessMask() const;
			VMWModule GetModule() const;
			int AddRef() const;
			int Release();
			const char* GetName() const;
			const char* GetNamespace() const;
			VMWTypeInfo GetBaseType() const;
			bool DerivesFrom(const VMWTypeInfo& Type) const;
			size_t GetFlags() const;
			unsigned int GetSize() const;
			int GetTypeId() const;
			int GetSubTypeId(unsigned int SubTypeIndex = 0) const;
			VMWTypeInfo GetSubType(unsigned int SubTypeIndex = 0) const;
			unsigned int GetSubTypeCount() const;
			unsigned int GetInterfaceCount() const;
			VMWTypeInfo GetInterface(unsigned int Index) const;
			bool Implements(const VMWTypeInfo& Type) const;
			unsigned int GetFactoriesCount() const;
			VMWFunction GetFactoryByIndex(unsigned int Index) const;
			VMWFunction GetFactoryByDecl(const char* Decl) const;
			unsigned int GetMethodsCount() const;
			VMWFunction GetMethodByIndex(unsigned int Index, bool GetVirtual = true) const;
			VMWFunction GetMethodByName(const char* Name, bool GetVirtual = true) const;
			VMWFunction GetMethodByDecl(const char* Decl, bool GetVirtual = true) const;
			unsigned int GetPropertiesCount() const;
			int GetProperty(unsigned int Index, VMFuncProperty* Out) const;
			const char* GetPropertyDeclaration(unsigned int Index, bool IncludeNamespace = false) const;
			unsigned int GetBehaviourCount() const;
			VMWFunction GetBehaviourByIndex(unsigned int Index, VMBehave* OutBehaviour) const;
			unsigned int GetChildFunctionDefCount() const;
			VMWTypeInfo GetChildFunctionDef(unsigned int Index) const;
			VMWTypeInfo GetParentType() const;
			unsigned int GetEnumValueCount() const;
			const char* GetEnumValueByIndex(unsigned int Index, int* OutValue) const;
			VMWFunction GetFunctionDefSignature() const;
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

		struct THAWK_OUT VMWFunction
		{
		private:
			VMManager* Manager;
			VMCFunction* Function;

		public:
			VMWFunction(VMCFunction* Base);
			int AddRef() const;
			int Release();
			int GetId() const;
			VMFuncType GetType() const;
			const char* GetModuleName() const;
			VMWModule GetModule() const;
			const char* GetSectionName() const;
			const char* GetGroup() const;
			size_t GetAccessMask() const;
			VMWTypeInfo GetObjectType() const;
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
			VMWTypeInfo GetDelegateObjectType() const;
			VMWFunction GetDelegateFunction() const;
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

		struct THAWK_OUT VMWArray
		{
		private:
			VMCArray* Array;

		public:
			VMWArray(VMCArray* Base);
			void AddRef() const;
			void Release();
			VMWTypeInfo GetArrayObjectType() const;
			int GetArrayTypeId() const;
			int GetElementTypeId() const;
			unsigned int GetSize() const;
			bool IsEmpty() const;
			void Reserve(unsigned int NumElements);
			void Resize(unsigned int NumElements);
			void* At(unsigned int Index);
			const void* At(unsigned int Index) const;
			void SetValue(unsigned int Index, void* Value);
			void InsertAt(unsigned int Index, void* Value);
			void RemoveAt(unsigned int Index);
			void InsertLast(void* Value);
			void RemoveLast();
			void SortAsc();
			void SortAsc(unsigned int StartAt, unsigned int Count);
			void SortDesc();
			void SortDesc(unsigned int StartAt, unsigned int Count);
			void Sort(unsigned int StartAt, unsigned int Count, bool Asc);
			void Sort(const VMWFunction& Less, unsigned int StartAt, unsigned int Count);
			void Reverse();
			int Find(void* Value) const;
			int Find(unsigned int StartAt, void* Value) const;
			int FindByRef(void* Ref) const;
			int FindByRef(unsigned int StartAt, void* Ref) const;
			void* GetBuffer();
			bool IsValid() const;
			VMCArray* GetArray() const;

		public:
			static VMWArray Create(const VMWTypeInfo& ArrayType);
			static VMWArray Create(const VMWTypeInfo& ArrayType, unsigned int Length);
			static VMWArray Create(const VMWTypeInfo& ArrayType, unsigned int Length, void* DefaultValue);
			static VMWArray Create(const VMWTypeInfo& ArrayType, void* ListBuffer);

		public:
			template <typename T>
			static VMWArray ComposeFromObjects(const VMWTypeInfo& ArrayType, const std::vector<T>& Objects)
			{
				VMWArray Array = Create(ArrayType, (unsigned int)Objects.size());
				for (size_t i = 0; i < Objects.size(); i++)
					Array.SetValue(i, (void*)&Objects[i]);

				return Array;
			}
			template <typename T>
			static VMWArray ComposeFromPointers(const VMWTypeInfo& ArrayType, const std::vector<T*>& Objects)
			{
				VMWArray Array = Create(ArrayType, (unsigned int)Objects.size());
				for (size_t i = 0; i < Objects.size(); i++)
					Array.SetValue(i, (void*)Objects[i]);

				return Array;
			}
			template <typename T>
			static void DecomposeToObjects(const VMWArray& Array, std::vector<T>* Objects)
			{
				if (!Objects)
					return;

				unsigned int Size = Array.GetSize();
				Objects->reserve(Size);

				for (unsigned int i = 0; i < Size; i++)
				{
					T* Object = (T*)Array.At(i);
					Objects->push_back(*Object);
				}
			}
			template <typename T>
			static void DecomposeToPointers(const VMWArray& Array, std::vector<T*>* Objects)
			{
				if (!Objects)
					return;

				unsigned int Size = Array.GetSize();
				Objects->reserve(Size);

				for (unsigned int i = 0; i < Size; i++)
				{
					T* Object = (T*)Array.At(i);
					Objects->push_back(Object);
				}
			}
		};

		struct THAWK_OUT VMWAny
		{
		private:
			VMCAny* Any;

		public:
			VMWAny(VMCAny* Base);
			int AddRef() const;
			int Release();
			int CopyFrom(const VMWAny& Other);
			void Store(void* Ref, int RefTypeId);
			void Store(as_int64_t& Value);
			void Store(double& Value);
			bool RetrieveAny(void** Ref, int* RefTypeId) const;
			bool Retrieve(void* Ref, int RefTypeId) const;
			bool Retrieve(as_int64_t& Value) const;
			bool Retrieve(double& Value) const;
			int GetTypeId() const;
			bool IsValid() const;
			VMCAny* GetAny() const;

		public:
			static VMWAny Create(VMManager* Manager, void* Ref, int TypeId);
		};

		struct THAWK_OUT VMWObject
		{
		private:
			VMCObject* Object;

		public:
			VMWObject(VMCObject* Base);
			int AddRef() const;
			int Release();
			VMCLockableSharedBool* GetWeakRefFlag();
			VMWTypeInfo GetObjectType();
			int GetTypeId();
			int GetPropertyTypeId(unsigned int Id) const;
			unsigned int GetPropertiesCount() const;
			const char* GetPropertyName(unsigned int Id) const;
			void* GetAddressOfProperty(unsigned int Id);
			VMManager* GetManager() const;
			int CopyFrom(const VMWObject& Other);
			void* SetUserData(void* Data, uint64_t Type = 0);
			void* GetUserData(uint64_t Type = 0) const;
			bool IsValid() const;
			VMCObject* GetObject() const;
		};

		struct THAWK_OUT VMWDictionary
		{
		private:
			VMCDictionary* Dictionary;

		public:
			VMWDictionary(VMCDictionary* Base);
			void AddRef() const;
			void Release();
			void Set(const std::string& Key, void* Value, int TypeId);
			void Set(const std::string& Key, as_int64_t& Value);
			void Set(const std::string& Key, double& Value);
			bool GetIndex(size_t Index, std::string* Key, void** Value, int* TypeId) const;
			bool Get(const std::string& Key, void** Value, int* TypeId) const;
			bool Get(const std::string& Key, as_int64_t& Value) const;
			bool Get(const std::string& Key, double& Value) const;
			int GetTypeId(const std::string& Key) const;
			bool Exists(const std::string& Key) const;
			bool IsEmpty() const;
			unsigned int GetSize() const;
			bool Delete(const std::string& Key);
			void DeleteAll();
			VMWArray GetKeys() const;
			bool IsValid() const;
			VMCDictionary* GetDictionary() const;

		public:
			static VMWDictionary Create(VMManager* Engine);
		};

		struct THAWK_OUT VMWGrid
		{
		private:
			VMCGrid* Grid;

		public:
			VMWGrid(VMCGrid* Base);
			void AddRef() const;
			void Release();
			VMWTypeInfo GetGridObjectType() const;
			int GetGridTypeId() const;
			int GetElementTypeId() const;
			unsigned int GetWidth() const;
			unsigned int GetHeight() const;
			void Resize(unsigned int Width, unsigned int Height);
			void* At(unsigned int X, unsigned int Y);
			const void* At(unsigned int X, unsigned int Y) const;
			void SetValue(unsigned int X, unsigned int Y, void* Value);
			bool IsValid() const;
			VMCGrid* GetGrid() const;

		public:
			static VMWGrid Create(const VMWTypeInfo& GridType);
			static VMWGrid Create(const VMWTypeInfo& GridType, unsigned int Width, unsigned int Height);
			static VMWGrid Create(const VMWTypeInfo& GridType, unsigned int Width, unsigned int Height, void* DefaultValue);
			static VMWGrid Create(const VMWTypeInfo& GridType, void* ListBuffer);
		};

		struct THAWK_OUT VMWWeakRef
		{
		private:
			VMCWeakRef* WeakRef;

		public:
			VMWWeakRef(VMCWeakRef* Base);
			VMWWeakRef& Set(void* NewRef);
			void* Get() const;
			bool Equals(void* Ref) const;
			VMWTypeInfo GetRefType() const;
			bool IsValid() const;
			VMCWeakRef* GetWeakRef() const;

		public:
			static VMWWeakRef Create(void* Ref, const VMWTypeInfo& TypeInfo);
		};

		struct THAWK_OUT VMWRef
		{
		private:
			VMCRef* Ref;

		public:
			VMWRef(VMCRef* Base);
			void Set(void* Ref, const VMWTypeInfo& Type);
			bool Equals(void* Ref, int TypeId) const;
			void Cast(void** OutRef, int TypeId);
			VMWTypeInfo GetType() const;
			int GetTypeId() const;
			void* GetHandle();
			bool IsValid() const;
			VMCRef* GetRef() const;

		public:
			static VMWRef Create(void* Ref, const VMWTypeInfo& TypeInfo);
		};

		struct THAWK_OUT VMWLockableSharedBool
		{
		private:
			VMCLockableSharedBool* Bool;

		public:
			VMWLockableSharedBool(VMCLockableSharedBool* Base);
			int AddRef() const;
			int Release();
			bool Get() const;
			void Set(bool Value);
			void Lock() const;
			void Unlock() const;
			bool IsValid() const;
			VMCLockableSharedBool* GetSharedBool() const;
		};

		struct THAWK_OUT VMWClass
		{
		protected:
			VMManager* Manager;
			std::string Object;
			int TypeId;

		public:
			VMWClass(VMManager* Engine, const std::string& Name, int Type);
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
			static Rest::Stroke GetOperator(VMOpFunc Op, const char* Out, const char* Args, bool Const, bool Right);

		public:
			template <typename T>
			int SetEnumRefs(void(T::*Value)(VMCManager*))
			{
				asSFuncPtr* EnumRefs = VMBind::BindMethod<T>(Value);
				int Result = SetBehaviourAddress("void f(int &in)", VMBehave_ENUMREFS, EnumRefs, VMCall_THISCALL);
				VMBind::ReleaseFunctor(&EnumRefs);

				return Result;
			}
			template <typename T>
			int SetReleaseRefs(void(T::*Value)(VMCManager*))
			{
				asSFuncPtr* ReleaseRefs = VMBind::BindMethod<T>(Value);
				int Result = SetBehaviourAddress("void f(int &in)", VMBehave_RELEASEREFS, ReleaseRefs, VMCall_THISCALL);
				VMBind::ReleaseFunctor(&ReleaseRefs);

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
				asSFuncPtr* Ptr = VMBind::BindMethod<T, R>(Value);
				int Result = SetMethodAddress(Rest::Form("%s get_%s()", Type, Name).Get(), Ptr, VMCall_THISCALL);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetGetterEx(const char* Type, const char* Name, R(*Value)(T*))
			{
				asSFuncPtr* Ptr = VMBind::BindFunction(Value);
				int Result = SetMethodAddress(Rest::Form("%s get_%s()", Type, Name).Get(), Ptr, VMCall_CDECL_OBJFIRST);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetSetter(const char* Type, const char* Name, void(T::*Value)(R))
			{
				asSFuncPtr* Ptr = VMBind::BindMethod<T, void, R>(Value);
				int Result = SetMethodAddress(Rest::Form("void set_%s(%s)", Name, Type).Get(), Ptr, VMCall_THISCALL);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetSetterEx(const char* Type, const char* Name, void(*Value)(T*, R))
			{
				asSFuncPtr* Ptr = VMBind::BindFunction(Value);
				int Result = SetMethodAddress(Rest::Form("void set_%s(%s)", Name, Type).Get(), Ptr, VMCall_CDECL_OBJFIRST);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetArrayGetter(const char* Type, const char* Name, R(T::*Value)(unsigned int))
			{
				asSFuncPtr* Ptr = VMBind::BindMethod<T, R, unsigned int>(Value);
				int Result = SetMethodAddress(Rest::Form("%s get_%s(uint)", Type, Name).Get(), Ptr, VMCall_THISCALL);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetArrayGetterEx(const char* Type, const char* Name, R(*Value)(T*, unsigned int))
			{
				asSFuncPtr* Ptr = VMBind::BindFunction(Value);
				int Result = SetMethodAddress(Rest::Form("%s get_%s(uint)", Type, Name).Get(), Ptr, VMCall_CDECL_OBJFIRST);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetArraySetter(const char* Type, const char* Name, void(T::*Value)(unsigned int, R))
			{
				asSFuncPtr* Ptr = VMBind::BindMethod<T, void, unsigned int, R>(Value);
				int Result = SetMethodAddress(Rest::Form("void set_%s(uint, %s)", Name, Type).Get(), Ptr, VMCall_THISCALL);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetArraySetterEx(const char* Type, const char* Name, void(*Value)(T*, unsigned int, R))
			{
				asSFuncPtr* Ptr = VMBind::BindFunction(Value);
				int Result = SetMethodAddress(Rest::Form("void set_%s(uint, %s)", Name, Type).Get(), Ptr, VMCall_CDECL_OBJFIRST);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R, typename... A>
			int SetLOperator(VMOpFunc Op, const char* Out, const char* Args, R(T::*Value)(A...))
			{
				if (!Out)
					return -1;

				Rest::Stroke Operator = GetOperator(Op, Out, Args, false, false);
				if (Operator.Empty())
					return -1;

				asSFuncPtr* Ptr = VMBind::BindMethod<T, R, A...>(Value);
				int Result = SetOperatorAddress(Operator.Get(), Ptr, VMCall_THISCALL);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename R, typename... A>
			int SetLOperatorEx(VMOpFunc Op, const char* Out, const char* Args, R(*Value)(A...))
			{
				if (!Out)
					return -1;

				Rest::Stroke Operator = GetOperator(Op, Out, Args, false, false);
				if (Operator.Empty())
					return -1;

				asSFuncPtr* Ptr = VMBind::BindFunction(Value);
				int Result = SetOperatorAddress(Operator.Get(), Ptr, VMCall_CDECL_OBJFIRST);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R, typename... A>
			int SetLCOperator(VMOpFunc Op, const char* Out, const char* Args, R(T::*Value)(A...))
			{
				if (!Out)
					return -1;

				Rest::Stroke Operator = GetOperator(Op, Out, Args, true, false);
				if (Operator.Empty())
					return -1;

				asSFuncPtr* Ptr = VMBind::BindMethod<T, R, A...>(Value);
				int Result = SetOperatorAddress(Operator.Get(), Ptr, VMCall_THISCALL);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename R, typename... A>
			int SetLCOperatorEx(VMOpFunc Op, const char* Out, const char* Args, R(*Value)(A...))
			{
				if (!Out)
					return -1;

				Rest::Stroke Operator = GetOperator(Op, Out, Args, true, false);
				if (Operator.Empty())
					return -1;

				asSFuncPtr* Ptr = VMBind::BindFunction(Value);
				int Result = SetOperatorAddress(Operator.Get(), Ptr, VMCall_CDECL_OBJFIRST);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R, typename... A>
			int SetROperator(VMOpFunc Op, const char* Out, const char* Args, R(T::*Value)(A...))
			{
				if (!Out)
					return -1;

				Rest::Stroke Operator = GetOperator(Op, Out, Args, false, true);
				if (Operator.Empty())
					return -1;

				asSFuncPtr* Ptr = VMBind::BindMethod<T, R, A...>(Value);
				int Result = SetOperatorAddress(Operator.Get(), Ptr, VMCall_THISCALL);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename R, typename... A>
			int SetROperatorEx(VMOpFunc Op, const char* Out, const char* Args, R(*Value)(A...))
			{
				if (!Out)
					return -1;

				Rest::Stroke Operator = GetOperator(Op, Out, Args, false, true);
				if (Operator.Empty())
					return -1;

				asSFuncPtr* Ptr = VMBind::BindFunction(Value);
				int Result = SetOperatorAddress(Operator.Get(), Ptr, VMCall_CDECL_OBJFIRST);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R, typename... A>
			int SetRCOperator(VMOpFunc Op, const char* Out, const char* Args, R(T::*Value)(A...))
			{
				if (!Out)
					return -1;

				Rest::Stroke Operator = GetOperator(Op, Out, Args, true, true);
				if (Operator.Empty())
					return -1;

				asSFuncPtr* Ptr = VMBind::BindMethod<T, R, A...>(Value);
				int Result = SetOperatorAddress(Operator.Get(), Ptr, VMCall_THISCALL);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename R, typename... A>
			int SetRCOperatorEx(VMOpFunc Op, const char* Out, const char* Args, R(*Value)(A...))
			{
				if (!Out)
					return -1;

				Rest::Stroke Operator = GetOperator(Op, Out, Args, true, true);
				if (Operator.Empty())
					return -1;

				asSFuncPtr* Ptr = VMBind::BindFunction(Value);
				int Result = SetOperatorAddress(Operator.Get(), Ptr, VMCall_CDECL_OBJFIRST);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T>
			int SetOperatorCopy()
			{
				asSFuncPtr* Ptr = VMBind::BindMethodOp<T, T&, const T&>(&T::operator =);
				int Result = SetOperatorCopyAddress(Ptr);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R, typename... Args>
			int SetMethod(const char* Decl, R(T::*Value)(Args...))
			{
				asSFuncPtr* Ptr = VMBind::BindMethod<T, R, Args...>(Value);
				int Result = SetMethodAddress(Decl, Ptr, VMCall_THISCALL);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R, typename... Args>
			int SetMethod(const char* Decl, R(T::*Value)(Args...) const)
			{
				asSFuncPtr* Ptr = VMBind::BindMethod<T, R, Args...>(Value);
				int Result = SetMethodAddress(Decl, Ptr, VMCall_THISCALL);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R, typename... Args>
			int SetMethodEx(const char* Decl, R(* Value)(T*, Args...))
			{
				asSFuncPtr* Ptr = VMBind::BindFunction<R(*)(T*, Args...)>(Value);
				int Result = SetMethodAddress(Decl, Ptr, VMCall_CDECL_OBJFIRST);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename R, typename... Args>
			int SetMethodStatic(const char* Decl, R(*Value)(Args...), VMCall Type = VMCall_CDECL)
			{
				asSFuncPtr* Ptr = (Type == VMCall_GENERIC ? VMBind::BindFunctionGeneric<R(*)(Args...)>(Value) : VMBind::BindFunction<R(*)(Args...)>(Value));
				int Result = SetMethodStaticAddress(Decl, Ptr, Type);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
		};

		struct THAWK_OUT VMWRefClass : public VMWClass
		{
		public:
			VMWRefClass(VMManager* Engine, const std::string& Name, int Type) : VMWClass(Engine, Name, Type)
			{
			}

		public:
			template <typename T, const char* TypeName, typename... Args>
			int SetManagedConstructor(const char* Decl)
			{
				if (!Manager)
					return -1;

				asSFuncPtr* Functor = VMBind::BindFunction(&VMBind::BindManaged<T, TypeName, Args...>);
				int Result = SetBehaviourAddress(Decl, VMBehave_FACTORY, Functor, VMCall_CDECL);
				VMBind::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T, const char* TypeName, VMCGeneric*>
			int SetManagedConstructor(const char* Decl)
			{
				if (!Manager)
					return -1;

				asSFuncPtr* Functor = VMBind::BindFunctionGeneric(&VMBind::BindManaged<T, TypeName, VMCGeneric*>);
				int Result = SetBehaviourAddress(Decl, VMBehave_FACTORY, Functor, VMCall_GENERIC);
				VMBind::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T, const char* TypeName>
			int SetManagedConstructorList(const char* Decl)
			{
				asSFuncPtr* Functor = VMBind::BindFunctionGeneric(&VMBind::BindManagedList<T, TypeName>);
				int Result = SetBehaviourAddress(Decl, VMBehave_LIST_FACTORY, Functor, VMCall_GENERIC);
				VMBind::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T, typename... Args>
			int SetUnmanagedConstructor(const char* Decl)
			{
				if (!Manager)
					return -1;

				asSFuncPtr* Functor = VMBind::BindFunction(&VMBind::BindUnmanaged<T, Args...>);
				int Result = SetBehaviourAddress(Decl, VMBehave_FACTORY, Functor, VMCall_CDECL);
				VMBind::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T, VMCGeneric*>
			int SetUnmanagedConstructor(const char* Decl)
			{
				if (!Manager)
					return -1;

				asSFuncPtr* Functor = VMBind::BindFunctionGeneric(&VMBind::BindUnmanaged<T, VMCGeneric*>);
				int Result = SetBehaviourAddress(Decl, VMBehave_FACTORY, Functor, VMCall_GENERIC);
				VMBind::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T>
			int SetUnmanagedConstructorList(const char* Decl)
			{
				asSFuncPtr* Functor = VMBind::BindFunctionGeneric(&VMBind::BindUnmanagedList<T>);
				int Result = SetBehaviourAddress(Decl, VMBehave_LIST_FACTORY, Functor, VMCall_GENERIC);
				VMBind::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T>
			int SetUnmanagedConstructorListEx(const char* Decl, void(*Value)(VMCGeneric*))
			{
				asSFuncPtr* Functor = VMBind::BindFunctionGeneric(Value);
				int Result = SetBehaviourAddress(Decl, VMBehave_LIST_FACTORY, Functor, VMCall_GENERIC);
				VMBind::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T>
			int SetAddRef()
			{
				asSFuncPtr* AddRef = VMBind::BindFunction(&Rest::LT::AddRef);
				int Result = SetBehaviourAddress("void f()", VMBehave_ADDREF, AddRef, VMCall_CDECL_OBJFIRST);
				VMBind::ReleaseFunctor(&AddRef);

				return Result;
			}
			template <typename T>
			int SetRelease()
			{
				asSFuncPtr* Release = VMBind::BindFunction(&Rest::LT::Release);
				int Result = SetBehaviourAddress("void f()", VMBehave_RELEASE, Release, VMCall_CDECL_OBJFIRST);
				VMBind::ReleaseFunctor(&Release);

				return Result;
			}
			template <typename T>
			int SetGetRefCount()
			{
				asSFuncPtr* GetRefCount = VMBind::BindFunction(&Rest::LT::GetRefCount);
				int Result = SetBehaviourAddress("int f()", VMBehave_GETREFCOUNT, GetRefCount, VMCall_CDECL_OBJFIRST);
				VMBind::ReleaseFunctor(&GetRefCount);

				return Result;
			}
			template <typename T>
			int SetSetGCFlag()
			{
				asSFuncPtr* SetGCFlag = VMBind::BindFunction(&Rest::LT::SetFlag);
				int Result = SetBehaviourAddress("void f()", VMBehave_SETGCFLAG, SetGCFlag, VMCall_CDECL_OBJFIRST);
				VMBind::ReleaseFunctor(&SetGCFlag);

				return Result;
			}
			template <typename T>
			int SetGetGCFlag()
			{
				asSFuncPtr* GetGCFlag = VMBind::BindFunction(&Rest::LT::GetFlag);
				int Result = SetBehaviourAddress("bool f()", VMBehave_GETGCFLAG, GetGCFlag, VMCall_CDECL_OBJFIRST);
				VMBind::ReleaseFunctor(&GetGCFlag);

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

		struct THAWK_OUT VMWTypeClass : public VMWClass
		{
		public:
			VMWTypeClass(VMManager* Engine, const std::string& Name, int Type) : VMWClass(Engine, Name, Type)
			{
			}

		public:
			template <typename T, typename... Args>
			int SetConstructor(const char* Decl)
			{
				asSFuncPtr* Ptr = VMBind::BindFunction(&VMBind::BindConstructor<T, Args...>);
				int Result = SetConstructorAddress(Decl, Ptr, VMCall_CDECL_OBJFIRST);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, VMCGeneric*>
			int SetConstructor(const char* Decl)
			{
				asSFuncPtr* Ptr = VMBind::BindFunctionGeneric(&VMBind::BindConstructor<T, VMCGeneric*>);
				int Result = SetConstructorAddress(Decl, Ptr, VMCall_GENERIC);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T>
			int SetConstructorList(const char* Decl)
			{
				asSFuncPtr* Ptr = VMBind::BindFunctionGeneric(&VMBind::BindConstructorList<T>);
				int Result = SetConstructorListAddress(Decl, Ptr, VMCall_GENERIC);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T>
			int SetDestructor(const char* Decl)
			{
				asSFuncPtr* Ptr = VMBind::BindFunction(&VMBind::BindDestructor<T>);
				int Result = SetDestructorAddress(Decl, Ptr);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename R, typename... Args>
			int SetDestructorStatic(const char* Decl, R(*Value)(Args...))
			{
				asSFuncPtr* Ptr = VMBind::BindFunction<R(*)(Args...)>(Value);
				int Result = SetDestructorAddress(Decl, Ptr);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
		};

		struct THAWK_OUT VMWInterface
		{
		private:
			VMManager* Manager;
			std::string Object;
			int TypeId;

		public:
			VMWInterface(VMManager* Engine, const std::string& Name, int Type);
			int SetMethod(const char* Decl);
			int GetTypeId() const;
			bool IsValid() const;
			std::string GetName() const;
			VMManager* GetManager() const;
		};

		struct THAWK_OUT VMWEnum
		{
		private:
			VMManager* Manager;
			std::string Object;
			int TypeId;

		public:
			VMWEnum(VMManager* Engine, const std::string& Name, int Type);
			int SetValue(const char* Name, int Value);
			int GetTypeId() const;
			bool IsValid() const;
			std::string GetName() const;
			VMManager* GetManager() const;
		};

		struct THAWK_OUT VMWModule
		{
		private:
			VMManager* Manager;
			VMCModule* Mod;

		public:
			VMWModule(VMCModule* Type);
			void SetName(const char* Name);
			int AddSection(const char* Name, const char* Code, size_t CodeLength = 0, int LineOffset = 0);
			int RemoveFunction(const VMWFunction& Function);
			int ResetProperties(VMCContext* Context = nullptr);
			int Build();
			int LoadByteCode(VMByteCode* Info);
			int Discard();
			int BindImportedFunction(size_t ImportIndex, const VMWFunction& Function);
			int UnbindImportedFunction(size_t ImportIndex);
			int BindAllImportedFunctions();
			int UnbindAllImportedFunctions();
			int CompileFunction(const char* SectionName, const char* Code, int LineOffset, size_t CompileFlags, VMWFunction* OutFunction);
			int CompileProperty(const char* SectionName, const char* Code, int LineOffset);
			int SetDefaultNamespace(const char* NameSpace);
			void* GetAddressOfProperty(size_t Index);
			int RemoveProperty(size_t Index);
			size_t SetAccessMask(size_t AccessMask);
			size_t GetFunctionCount() const;
			VMWFunction GetFunctionByIndex(size_t Index) const;
			VMWFunction GetFunctionByDecl(const char* Decl) const;
			VMWFunction GetFunctionByName(const char* Name) const;
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
			VMWTypeInfo GetObjectByIndex(size_t Index) const;
			VMWTypeInfo GetTypeInfoByName(const char* Name) const;
			VMWTypeInfo GetTypeInfoByDecl(const char* Decl) const;
			VMWTypeInfo GetEnumByIndex(size_t Index) const;
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

		struct THAWK_OUT VMGlobal
		{
		private:
			VMManager* Manager;

		public:
			VMGlobal(VMManager* Engine);
			int SetFunctionDef(const char* Decl);
			int SetFunctionAddress(const char* Decl, asSFuncPtr* Value, VMCall Type = VMCall_CDECL);
			int SetPropertyAddress(const char* Decl, void* Value);
			VMWTypeClass SetStructAddress(const char* Name, size_t Size, uint64_t Flags = VMObjType_VALUE);
			VMWTypeClass SetPodAddress(const char* Name, size_t Size, uint64_t Flags = VMObjType_VALUE | VMObjType_POD);
			VMWRefClass SetClassAddress(const char* Name, uint64_t Flags = VMObjType_REF);
			VMWInterface SetInterface(const char* Name);
			VMWEnum SetEnum(const char* Name);
			size_t GetFunctionsCount() const;
			VMWFunction GetFunctionById(int Id) const;
			VMWFunction GetFunctionByIndex(int Index) const;
			VMWFunction GetFunctionByDecl(const char* Decl) const;
			size_t GetPropertiesCount() const;
			int GetPropertyByIndex(int Index, VMProperty* Info) const;
			int GetPropertyIndexByName(const char* Name) const;
			int GetPropertyIndexByDecl(const char* Decl) const;
			size_t GetObjectsCount() const;
			VMWTypeInfo GetObjectByTypeId(int TypeId) const;
			size_t GetEnumCount() const;
			VMWTypeInfo GetEnumByTypeId(int TypeId) const;
			size_t GetFunctionDefsCount() const;
			VMWTypeInfo GetFunctionDefByIndex(int Index) const;
			size_t GetModulesCount() const;
			VMCModule* GetModuleById(int Id) const;
			int GetTypeIdByDecl(const char* Decl) const;
			const char* GetTypeIdDecl(int TypeId, bool IncludeNamespace = false) const;
			int GetSizeOfPrimitiveType(int TypeId) const;
			std::string GetObjectView(void* Object, int TypeId);
			std::string GetObjectView(const VMWAny& Any);
			VMWTypeInfo GetTypeInfoById(int TypeId) const;
			VMWTypeInfo GetTypeInfoByName(const char* Name) const;
			VMWTypeInfo GetTypeInfoByDecl(const char* Decl) const;
			VMManager* GetManager() const;

		public:
			template <typename T>
			int SetFunction(const char* Decl, T Value)
			{
				asSFuncPtr* Ptr = VMBind::BindFunction<T>(Value);
				int Result = SetFunctionAddress(Decl, Ptr, VMCall_CDECL);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <>
			int SetFunction(const char* Decl, void(* Value)(VMCGeneric*))
			{
				asSFuncPtr* Ptr = VMBind::BindFunction<void (*)(VMCGeneric*)>(Value);
				int Result = SetFunctionAddress(Decl, Ptr, VMCall_GENERIC);
				VMBind::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T>
			int SetProperty(const char* Decl, T* Value)
			{
				return SetPropertyAddress(Decl, (void*)Value);
			}
			template <typename T>
			VMWRefClass SetClassManaged(const char* Name, void(T::*EnumRefs)(VMCManager*), void(T::*ReleaseRefs)(VMCManager*))
			{
				VMWRefClass Class = SetClassAddress(Name, VMObjType_REF | VMObjType_GC);
				Class.SetManaged<T>(EnumRefs, ReleaseRefs);

				return Class;
			}
			template <typename T>
			VMWRefClass SetClassUnmanaged(const char* Name)
			{
				VMWRefClass Class = SetClassAddress(Name, VMObjType_REF);
				Class.SetUnmanaged<T>();

				return Class;
			}
			template <typename T>
			VMWTypeClass SetStructManaged(const char* Name, void(T::*EnumRefs)(VMCManager*), void(T::* ReleaseRefs)(VMCManager*))
			{
				VMWTypeClass Struct = SetStructAddress(Name, sizeof(T), VMObjType_VALUE | VMObjType_GC | VMBind::GetTypeTraits<T>());
				Struct.SetEnumRefs(EnumRefs);
				Struct.SetReleaseRefs(ReleaseRefs);
				Struct.SetDestructor<T>("void f()");

				return Struct;
			}
			template <typename T>
			VMWTypeClass SetStructUnmanaged(const char* Name)
			{
				VMWTypeClass Struct = SetStructAddress(Name, sizeof(T), VMObjType_VALUE | VMBind::GetTypeTraits<T>());
				Struct.SetOperatorCopy<T>();
				Struct.SetDestructor<T>("void f()");

				return Struct;
			}
			template <typename T>
			VMWTypeClass SetPod(const char* Name)
			{
				return SetPodAddress(Name, sizeof(T), VMObjType_VALUE | VMObjType_POD | VMBind::GetTypeTraits<T>());
			}
		};

		class THAWK_OUT VMCompiler : public Rest::Object
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
			uint64_t Features;
			VMByteCode VCache;
			bool BuiltOK;

		public:
			VMCompiler(VMManager* Engine);
			~VMCompiler();
			void SetIncludeCallback(const Compute::ProcIncludeCallback& Callback);
			void SetPragmaCallback(const Compute::ProcPragmaCallback& Callback);
			void SetAllowedFeatures(uint64_t Features);
			void Define(const std::string& Word);
			void Undefine(const std::string& Word);
			void Clear();
			bool IsDefined(const std::string& Word);
			bool IsBuilt();
			int Prepare(const std::string& ModuleName);
			int Prepare(const std::string& ModuleName, const std::string& Cache, bool Debug = true);
			int PrepareScope(const std::string& ModuleName);
			int PrepareScope(const std::string& ModuleName, const std::string& Cache, bool Debug = true);
			int Compile(bool Await);
			int SaveByteCode(VMByteCode* Info);
			int LoadByteCode(VMByteCode* Info);
			int LoadFile(const std::string& Path);
			int LoadCode(const std::string& Name, const std::string& Buffer);
			int LoadCode(const std::string& Name, const char* Buffer, uint64_t Length);
			int ExecuteFile(const char* Name, const char* ModuleName, const char* EntryName, void* Return = nullptr, int ReturnTypeId = VMTypeId_VOID);
			int ExecuteMemory(const std::string& Buffer, const char* ModuleName, const char* EntryName, void* Return = nullptr, int ReturnTypeId = VMTypeId_VOID);
			int ExecuteEntry(const char* Name, void* Return = nullptr, int ReturnTypeId = VMTypeId_VOID);
			int ExecuteScoped(const std::string& Code, void* Return = nullptr, int ReturnTypeId = VMTypeId_VOID);
			int ExecuteScoped(const char* Buffer, uint64_t Length, void* Return = nullptr, int ReturnTypeId = VMTypeId_VOID);
			VMWModule GetModule() const;
			VMManager* GetManager() const;
			VMContext* GetContext() const;
			Compute::Preprocessor* GetProcessor() const;

		private:
			static VMCompiler* Get(VMContext* Context);
		};

		class THAWK_OUT VMContext : public Rest::Object
		{
		private:
			static int ContextUD;

		private:
			VMCContext* Context;
			VMManager* Manager;

		public:
			VMContext(VMCContext* Base);
			~VMContext();
			int SetExceptionCallback(void(* Callback)(VMCContext* Context, void* Object), void* Object);
			int AddRefVM() const;
			int ReleaseVM();
			int Prepare(const VMWFunction& Function);
			int Unprepare();
			int Execute();
			int Resume();
			int Abort();
			int Suspend();
			VMExecState GetState() const;
			std::string GetStackTrace() const;
			int PushState();
			int PopState();
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
			VMWFunction GetExceptionFunction();
			const char* GetExceptionString();
			bool WillExceptionBeCaught();
			void ClearExceptionCallback();
			int SetLineCallback(void(* Callback)(VMCContext* Context, void* Object), void* Object);
			void ClearLineCallback();
			unsigned int GetCallstackSize() const;
			VMWFunction GetFunction(unsigned int StackLevel = 0);
			int GetLineNumber(unsigned int StackLevel = 0, int* Column = 0, const char** SectionName = 0);
			int GetPropertiesCount(unsigned int StackLevel = 0);
			const char* GetPropertyName(unsigned int Index, unsigned int StackLevel = 0);
			const char* GetPropertyDecl(unsigned int Index, unsigned int StackLevel = 0, bool IncludeNamespace = false);
			int GetPropertyTypeId(unsigned int Index, unsigned int StackLevel = 0);
			void* GetAddressOfProperty(unsigned int Index, unsigned int StackLevel = 0);
			bool IsPropertyInScope(unsigned int Index, unsigned int StackLevel = 0);
			int GetThisTypeId(unsigned int StackLevel = 0);
			void* GetThisPointer(unsigned int StackLevel = 0);
			VMWFunction GetSystemFunction();
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

		class THAWK_OUT VMManager : public Rest::Object
		{
		private:
			static int ManagerUD;

		private:
			std::unordered_map<std::string, std::string> Files;
			std::unordered_map<std::string, Rest::Document*> Datas;
			std::unordered_map<std::string, VMByteCode> Opcodes;
			std::vector<VMCContext*> Contexts;
			Compute::Preprocessor::Desc Proc;
			Compute::IncludeDesc Include;
			std::mutex Safe;
			uint64_t Features;
			uint64_t Scope;
#ifdef HAS_AS_JIT
			VMCJITCompiler* JIT;
#endif
			VMCManager* Engine;
			VMGlobal Globals;
			bool Cached;

		public:
			VMManager();
			~VMManager();
			void Setup(uint64_t NewFeatures);
			void SetupJIT(unsigned int JITOpts);
			void SetCache(bool Enabled);
			void ClearCache();
			void Lock();
			void Unlock();
			void SetCompilerIncludeOptions(const Compute::IncludeDesc& NewDesc);
			void SetCompilerFeatures(const Compute::Preprocessor::Desc& NewDesc);
			void SetProcessorOptions(Compute::Preprocessor* Processor);
			int Collect(size_t NumIterations = 1);
			void GetStatistics(unsigned int* CurrentSize, unsigned int* TotalDestroyed, unsigned int* TotalDetected, unsigned int* NewObjects, unsigned int* TotalNewDestroyed) const;
			int NotifyOfNewObject(void* Object, const VMWTypeInfo& Type);
			int GetObjectAddress(size_t Index, size_t* SequenceNumber = nullptr, void** Object = nullptr, VMWTypeInfo* Type = nullptr);
			void ForwardEnumReferences(void* Reference, const VMWTypeInfo& Type);
			void ForwardReleaseReferences(void* Reference, const VMWTypeInfo& Type);
			void GCEnumCallback(void* Reference);
			bool GetByteCodeCache(VMByteCode* Info);
			void SetByteCodeCache(VMByteCode* Info);
			VMContext* CreateContext();
			VMCompiler* CreateCompiler();
			void* CreateObject(const VMWTypeInfo& Type);
			void* CreateObjectCopy(void* Object, const VMWTypeInfo& Type);
			void* CreateEmptyObject(const VMWTypeInfo& Type);
			VMWFunction CreateDelegate(const VMWFunction& Function, void* Object);
			int AssignObject(void* DestObject, void* SrcObject, const VMWTypeInfo& Type);
			void ReleaseObject(void* Object, const VMWTypeInfo& Type);
			void AddRefObject(void* Object, const VMWTypeInfo& Type);
			int RefCastObject(void* Object, const VMWTypeInfo& FromType, const VMWTypeInfo& ToType, void** NewPtr, bool UseOnlyImplicitCast = false);
			VMWLockableSharedBool GetWeakRefFlagOfObject(void* Object, const VMWTypeInfo& Type) const;
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
			VMWModule Module(const char* Name);
			int SetLogCallback(void(* Callback)(const asSMessageInfo* Message, void* Object), void* Object);
			int Log(const char* Section, int Row, int Column, VMLogType Type, const char* Message);
			int AddRefVM() const;
			int ReleaseVM();
			int SetProperty(VMProp Property, size_t Value);
			void SetDocumentRoot(const std::string& Root);
			size_t GetProperty(VMProp Property) const;
			VMCManager* GetEngine() const;
			std::string GetDocumentRoot() const;
			std::string GetScopedName(const std::string& Name);
			Rest::Document* ImportJSON(const std::string& Path);
			bool ImportFile(const std::string& Path, std::string* Out);

		public:
			static VMManager* Get(VMCManager* Engine);
			static VMManager* Get();
			static size_t GetDefaultAccessMask();
			static void FreeProxy();

		private:
			static VMCContext* RequestContext(VMCManager* Engine, void* Data);
			static void ReturnContext(VMCManager* Engine, VMCContext* Context, void* Data);
			static void CompileLogger(asSMessageInfo* Info, void* Object);
		};

		class THAWK_OUT VMDebugger : public Rest::Object
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
			void RegisterToStringCallback(const VMWTypeInfo& Type, ToStringCallback Callback);
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
	}
}
#endif
