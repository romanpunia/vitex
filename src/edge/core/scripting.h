#ifndef ED_SCRIPTING_H
#define ED_SCRIPTING_H
#include "compute.h"
#include <type_traits>
#include <random>
#include <queue>

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

namespace Edge
{
	namespace Scripting
	{
		struct Module;

		struct Function;

		class Compiler;

		class VirtualMachine;

		class ImmediateContext;

		class DummyPtr
		{
		};

		enum class Optimizer
		{
			No_Suspend = 0x01,
			Syscall_FPU_No_Reset = 0x02,
			Syscall_No_Errors = 0x04,
			Alloc_Simple = 0x08,
			No_Switches = 0x10,
			No_Script_Calls = 0x20,
			Fast_Ref_Counter = 0x40,
			Optimal = Alloc_Simple | Fast_Ref_Counter
		};

		enum class LogCategory
		{
			ERR = 0,
			WARNING = 1,
			INFORMATION = 2
		};

		enum class Features
		{
			ALLOW_UNSAFE_REFERENCES = 1,
			OPTIMIZE_BYTECODE = 2,
			COPY_SCRIPT_SECTIONS = 3,
			MAX_STACK_SIZE = 4,
			USE_CHARACTER_LITERALS = 5,
			ALLOW_MULTILINE_STRINGS = 6,
			ALLOW_IMPLICIT_HANDLE_TYPES = 7,
			BUILD_WITHOUT_LINE_CUES = 8,
			INIT_GLOBAL_VARS_AFTER_BUILD = 9,
			REQUIRE_ENUM_SCOPE = 10,
			SCRIPT_SCANNER = 11,
			INCLUDE_JIT_INSTRUCTIONS = 12,
			STRING_ENCODING = 13,
			PROPERTY_ACCESSOR_MODE = 14,
			EXPAND_DEF_ARRAY_TO_TMPL = 15,
			AUTO_GARBAGE_COLLECT = 16,
			DISALLOW_GLOBAL_VARS = 17,
			ALWAYS_IMPL_DEFAULT_CONSTRUCT = 18,
			COMPILER_WARNINGS = 19,
			DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE = 20,
			ALTER_SYNTAX_NAMED_ARGS = 21,
			DISABLE_INTEGER_DIVISION = 22,
			DISALLOW_EMPTY_LIST_ELEMENTS = 23,
			PRIVATE_PROP_AS_PROTECTED = 24,
			ALLOW_UNICODE_IDENTIFIERS = 25,
			HEREDOC_TRIM_MODE = 26,
			MAX_NESTED_CALLS = 27,
			GENERIC_CALL_MODE = 28,
			INIT_STACK_SIZE = 29,
			INIT_CALL_STACK_SIZE = 30,
			MAX_CALL_STACK_SIZE = 31
		};

		enum class Modifiers
		{
			NONE = 0,
			INREF = 1,
			OUTREF = 2,
			INOUTREF = 3,
			CONSTF = 4
		};

		enum class CompileFlags
		{
			ADD_TO_MODULE = 1
		};

		enum class FunctionType
		{
			DUMMY = -1,
			SYSTEM = 0,
			SCRIPT = 1,
			INTERFACE = 2,
			VIRTUAL = 3,
			FUNCDEF = 4,
			IMPORTED = 5,
			DELEGATE = 6
		};

		enum class Behaviours
		{
			CONSTRUCT,
			LIST_CONSTRUCT,
			DESTRUCT,
			FACTORY,
			LIST_FACTORY,
			ADDREF,
			RELEASE,
			GET_WEAKREF_FLAG,
			TEMPLATE_CALLBACK,
			FIRST_GC,
			GETREFCOUNT = FIRST_GC,
			SETGCFLAG,
			GETGCFLAG,
			ENUMREFS,
			RELEASEREFS,
			LAST_GC = RELEASEREFS,
			MAX
		};

		enum class Activation
		{
			FINISHED = 0,
			SUSPENDED = 1,
			ABORTED = 2,
			EXCEPTION = 3,
			PREPARED = 4,
			UNINITIALIZED = 5,
			ACTIVE = 6,
			ERR = 7
		};

		enum class FunctionCall
		{
			CDECLF = 0,
			STDCALL = 1,
			THISCALL_ASGLOBAL = 2,
			THISCALL = 3,
			CDECL_OBJLAST = 4,
			CDECL_OBJFIRST = 5,
			GENERIC = 6,
			THISCALL_OBJLAST = 7,
			THISCALL_OBJFIRST = 8
		};

		enum class Errors
		{
			SUCCESS = 0,
			ERR = -1,
			CONTEXT_ACTIVE = -2,
			CONTEXT_NOT_FINISHED = -3,
			CONTEXT_NOT_PREPARED = -4,
			INVALID_ARG = -5,
			NO_FUNCTION = -6,
			NOT_SUPPORTED = -7,
			INVALID_NAME = -8,
			NAME_TAKEN = -9,
			INVALID_DECLARATION = -10,
			INVALID_OBJECT = -11,
			INVALID_TYPE = -12,
			ALREADY_REGISTERED = -13,
			MULTIPLE_FUNCTIONS = -14,
			NO_MODULE = -15,
			NO_GLOBAL_VAR = -16,
			INVALID_CONFIGURATION = -17,
			INVALID_INTERFACE = -18,
			CANT_BIND_ALL_FUNCTIONS = -19,
			LOWER_ARRAY_DIMENSION_NOT_REGISTERED = -20,
			WRONG_CONFIG_GROUP = -21,
			CONFIG_GROUP_IS_IN_USE = -22,
			ILLEGAL_BEHAVIOUR_FOR_TYPE = -23,
			WRONG_CALLING_CONV = -24,
			BUILD_IN_PROGRESS = -25,
			INIT_GLOBAL_VARS_FAILED = -26,
			OUT_OF_MEMORY = -27,
			MODULE_IS_IN_USE = -28
		};

		enum class TypeId
		{
			VOIDF = 0,
			BOOL = 1,
			INT8 = 2,
			INT16 = 3,
			INT32 = 4,
			INT64 = 5,
			UINT8 = 6,
			UINT16 = 7,
			UINT32 = 8,
			UINT64 = 9,
			FLOAT = 10,
			DOUBLE = 11,
			OBJHANDLE = 0x40000000,
			HANDLETOCONST = 0x20000000,
			MASK_OBJECT = 0x1C000000,
			APPOBJECT = 0x04000000,
			SCRIPTOBJECT = 0x08000000,
			TEMPLATE = 0x10000000,
			MASK_SEQNBR = 0x03FFFFFF
		};

		enum class ObjectBehaviours
		{
			REF = (1 << 0),
			VALUE = (1 << 1),
			GC = (1 << 2),
			POD = (1 << 3),
			NOHANDLE = (1 << 4),
			SCOPED = (1 << 5),
			TEMPLATE = (1 << 6),
			ASHANDLE = (1 << 7),
			APP_CLASS = (1 << 8),
			APP_CLASS_CONSTRUCTOR = (1 << 9),
			APP_CLASS_DESTRUCTOR = (1 << 10),
			APP_CLASS_ASSIGNMENT = (1 << 11),
			APP_CLASS_COPY_CONSTRUCTOR = (1 << 12),
			APP_CLASS_C = (APP_CLASS + APP_CLASS_CONSTRUCTOR),
			APP_CLASS_CD = (APP_CLASS + APP_CLASS_CONSTRUCTOR + APP_CLASS_DESTRUCTOR),
			APP_CLASS_CA = (APP_CLASS + APP_CLASS_CONSTRUCTOR + APP_CLASS_ASSIGNMENT),
			APP_CLASS_CK = (APP_CLASS + APP_CLASS_CONSTRUCTOR + APP_CLASS_COPY_CONSTRUCTOR),
			APP_CLASS_CDA = (APP_CLASS + APP_CLASS_CONSTRUCTOR + APP_CLASS_DESTRUCTOR + APP_CLASS_ASSIGNMENT),
			APP_CLASS_CDK = (APP_CLASS + APP_CLASS_CONSTRUCTOR + APP_CLASS_DESTRUCTOR + APP_CLASS_COPY_CONSTRUCTOR),
			APP_CLASS_CAK = (APP_CLASS + APP_CLASS_CONSTRUCTOR + APP_CLASS_ASSIGNMENT + APP_CLASS_COPY_CONSTRUCTOR),
			APP_CLASS_CDAK = (APP_CLASS + APP_CLASS_CONSTRUCTOR + APP_CLASS_DESTRUCTOR + APP_CLASS_ASSIGNMENT + APP_CLASS_COPY_CONSTRUCTOR),
			APP_CLASS_D = (APP_CLASS + APP_CLASS_DESTRUCTOR),
			APP_CLASS_DA = (APP_CLASS + APP_CLASS_DESTRUCTOR + APP_CLASS_ASSIGNMENT),
			APP_CLASS_DK = (APP_CLASS + APP_CLASS_DESTRUCTOR + APP_CLASS_COPY_CONSTRUCTOR),
			APP_CLASS_DAK = (APP_CLASS + APP_CLASS_DESTRUCTOR + APP_CLASS_ASSIGNMENT + APP_CLASS_COPY_CONSTRUCTOR),
			APP_CLASS_A = (APP_CLASS + APP_CLASS_ASSIGNMENT),
			APP_CLASS_AK = (APP_CLASS + APP_CLASS_ASSIGNMENT + APP_CLASS_COPY_CONSTRUCTOR),
			APP_CLASS_K = (APP_CLASS + APP_CLASS_COPY_CONSTRUCTOR),
			APP_PRIMITIVE = (1 << 13),
			APP_FLOAT = (1 << 14),
			APP_ARRAY = (1 << 15),
			APP_CLASS_ALLINTS = (1 << 16),
			APP_CLASS_ALLFLOATS = (1 << 17),
			NOCOUNT = (1 << 18),
			APP_CLASS_ALIGN8 = (1 << 19),
			IMPLICIT_HANDLE = (1 << 20),
			MASK_VALID_FLAGS = 0x1FFFFF
		};

		enum class Operators
		{
			Neg,
			Com,
			PreInc,
			PreDec,
			PostInc,
			PostDec,
			Equals,
			Cmp,
			Assign,
			AddAssign,
			SubAssign,
			MulAssign,
			DivAssign,
			ModAssign,
			PowAssign,
			AndAssign,
			OrAssign,
			XOrAssign,
			ShlAssign,
			ShrAssign,
			UshrAssign,
			Add,
			Sub,
			Mul,
			Div,
			Mod,
			Pow,
			And,
			Or,
			XOr,
			Shl,
			Shr,
			Ushr,
			Index,
			Call,
			Cast,
			ImplCast
		};

		enum class Position
		{
			Left = 0,
			Right = 1,
			Const = 2
		};

		enum class Imports
		{
			CLibraries = 1,
			CSymbols = 2,
			Submodules = 4,
			Files = 8,
			JSON = 8,
			All = (CLibraries | CSymbols | Submodules | Files | JSON)
		};

		inline ObjectBehaviours operator |(ObjectBehaviours A, ObjectBehaviours B)
		{
			return static_cast<ObjectBehaviours>(static_cast<size_t>(A) | static_cast<size_t>(B));
		}
		inline Position operator |(Position A, Position B)
		{
			return static_cast<Position>(static_cast<size_t>(A) | static_cast<size_t>(B));
		}
		inline Imports operator |(Imports A, Imports B)
		{
			return static_cast<Imports>(static_cast<size_t>(A) | static_cast<size_t>(B));
		}

		typedef void(DummyPtr::* DummyMethodPtr)();
		typedef void(*FunctionPtr)();
		typedef std::function<void(struct TypeInfo*, struct FunctionInfo*)> PropertyCallback;
		typedef std::function<void(struct TypeInfo*, struct Function*)> MethodCallback;
		typedef std::function<void(class VirtualMachine*)> SubmoduleCallback;
		typedef std::function<void(class ImmediateContext*)> ArgsCallback;

		class ED_OUT TypeCache
		{
		private:
			static Core::Mapping<Core::UnorderedMap<uint64_t, std::pair<Core::String, int>>>* Names;

		public:
			static uint64_t Set(uint64_t Id, const Core::String& Name);
			static int GetTypeId(uint64_t Id);
			static void FreeProxy();
		};

		class ED_OUT_TS FunctionFactory
		{
		public:
			static Core::Unique<asSFuncPtr> CreateFunctionBase(void(*Base)(), int Type);
			static Core::Unique<asSFuncPtr> CreateMethodBase(const void* Base, size_t Size, int Type);
			static Core::Unique<asSFuncPtr> CreateDummyBase();
			static void ReleaseFunctor(Core::Unique<asSFuncPtr>* Ptr);
			static int AtomicNotifyGC(const char* TypeName, void* Object);
			static int AtomicNotifyGCById(int TypeId, void* Object);
		};

		template <int N>
		struct ED_OUT_TS FunctionBinding
		{
			template <class M>
			static Core::Unique<asSFuncPtr> Bind(M Value)
			{
				return FunctionFactory::CreateDummyBase();
			}
		};

		template <>
		struct ED_OUT_TS FunctionBinding<sizeof(DummyMethodPtr)>
		{
			template <class M>
			static Core::Unique<asSFuncPtr> Bind(M Value)
			{
				return FunctionFactory::CreateMethodBase(&Value, sizeof(DummyMethodPtr), 3);
			}
		};
#if defined(_MSC_VER) && !defined(__MWERKS__)
		template <>
		struct ED_OUT_TS FunctionBinding<sizeof(DummyMethodPtr) + 1 * sizeof(int)>
		{
			template <class M>
			static Core::Unique<asSFuncPtr> Bind(M Value)
			{
				return FunctionFactory::CreateMethodBase(&Value, sizeof(DummyMethodPtr) + sizeof(int), 3);
			}
		};

		template <>
		struct ED_OUT_TS FunctionBinding<sizeof(DummyMethodPtr) + 2 * sizeof(int)>
		{
			template <class M>
			static Core::Unique<asSFuncPtr> Bind(M Value)
			{
				asSFuncPtr* Ptr = FunctionFactory::CreateMethodBase(&Value, sizeof(DummyMethodPtr) + 2 * sizeof(int), 3);
#if defined(_MSC_VER) && !defined(ED_64)
				*(reinterpret_cast<unsigned long*>(Ptr) + 3) = *(reinterpret_cast<unsigned long*>(Ptr) + 2);
#endif
				return Ptr;
			}
		};

		template <>
		struct ED_OUT_TS FunctionBinding<sizeof(DummyMethodPtr) + 3 * sizeof(int)>
		{
			template <class M>
			static Core::Unique<asSFuncPtr> Bind(M Value)
			{
				return FunctionFactory::CreateMethodBase(&Value, sizeof(DummyMethodPtr) + 3 * sizeof(int), 3);
			}
		};

		template <>
		struct ED_OUT_TS FunctionBinding<sizeof(DummyMethodPtr) + 4 * sizeof(int)>
		{
			template <class M>
			static Core::Unique<asSFuncPtr> Bind(M Value)
			{
				return FunctionFactory::CreateMethodBase(&Value, sizeof(DummyMethodPtr) + 4 * sizeof(int), 3);
			}
		};
#endif
		class ED_OUT GenericContext
		{
		private:
			VirtualMachine* VM;
			asIScriptGeneric* Generic;

		public:
			GenericContext(asIScriptGeneric* Base) noexcept;
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
			asIScriptGeneric* GetGeneric() const;
			VirtualMachine* GetVM() const;

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

		class ED_OUT Bridge
		{
		public:
			template <typename T>
			static Core::Unique<asSFuncPtr> Function(T Value)
			{
#ifdef ED_64
				void(*Address)() = reinterpret_cast<void(*)()>(size_t(Value));
#else
				void (*Address)() = reinterpret_cast<void (*)()>(Value);
#endif
				return FunctionFactory::CreateFunctionBase(Address, 2);
			}
			template <typename T>
			static Core::Unique<asSFuncPtr> FunctionGeneric(T Value)
			{
#ifdef ED_64
				void(*Address)() = reinterpret_cast<void(*)()>(size_t(Value));
#else
				void(*Address)() = reinterpret_cast<void(*)()>(Value);
#endif
				return FunctionFactory::CreateFunctionBase(Address, 1);
			}
			template <typename T, typename R, typename... Args>
			static Core::Unique<asSFuncPtr> Method(R(T::* Value)(Args...))
			{
				return FunctionBinding<sizeof(void (T::*)())>::Bind((void (T::*)())(Value));
			}
			template <typename T, typename R, typename... Args>
			static Core::Unique<asSFuncPtr> Method(R(T::* Value)(Args...) const)
			{
				return FunctionBinding<sizeof(void (T::*)())>::Bind((void (T::*)())(Value));
			}
			template <typename T, typename R, typename... Args>
			static Core::Unique<asSFuncPtr> MethodOp(R(T::* Value)(Args...))
			{
				return FunctionBinding<sizeof(void (T::*)())>::Bind(static_cast<R(T::*)(Args...)>(Value));
			}
			template <typename T, typename R, typename... Args>
			static Core::Unique<asSFuncPtr> MethodOp(R(T::* Value)(Args...) const)
			{
				return FunctionBinding<sizeof(void (T::*)())>::Bind(static_cast<R(T::*)(Args...)>(Value));
			}
			template <typename T, typename... Args>
			static void GetConstructorCall(void* Memory, Args... Data)
			{
				new(Memory) T(Data...);
			}
			template <typename T>
			static void GetConstructorListCall(asIScriptGeneric* Generic)
			{
				GenericContext Args(Generic);
				*reinterpret_cast<T**>(Args.GetAddressOfReturnLocation()) = new T((unsigned char*)Args.GetArgAddress(0));
			}
			template <typename T>
			static void GetDestructorCall(void* Memory)
			{
				((T*)Memory)->~T();
			}
			template <typename T, uint64_t TypeName, typename... Args>
			static T* GetManagedCall(Args... Data)
			{
				auto* Result = new T(Data...);
				FunctionFactory::AtomicNotifyGCById(TypeCache::GetTypeId(TypeName), (void*)Result);

				return Result;
			}
			template <typename T, uint64_t TypeName>
			static void GetManagedListCall(asIScriptGeneric* Generic)
			{
				GenericContext Args(Generic);
				T* Result = new T((unsigned char*)Args.GetArgAddress(0));
				*reinterpret_cast<T**>(Args.GetAddressOfReturnLocation()) = Result;
				FunctionFactory::AtomicNotifyGCById(TypeCache::GetTypeId(TypeName), (void*)Result);
			}
			template <typename T, typename... Args>
			static Core::Unique<T> GetUnmanagedCall(Args... Data)
			{
				return new T(Data...);
			}
			template <typename T>
			static void GetUnmanagedListCall(asIScriptGeneric* Generic)
			{
				GenericContext Args(Generic);
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
					return (size_t)ObjectBehaviours::APP_FLOAT;

				if (IsPrimitive)
					return (size_t)ObjectBehaviours::APP_PRIMITIVE;

				if (IsClass)
				{
					size_t Flags = (size_t)ObjectBehaviours::APP_CLASS;
					if (HasConstructor)
						Flags |= (size_t)ObjectBehaviours::APP_CLASS_CONSTRUCTOR;

					if (HasDestructor)
						Flags |= (size_t)ObjectBehaviours::APP_CLASS_DESTRUCTOR;

					if (HasAssignmentOperator)
						Flags |= (size_t)ObjectBehaviours::APP_CLASS_ASSIGNMENT;

					if (HasCopyConstructor)
						Flags |= (size_t)ObjectBehaviours::APP_CLASS_COPY_CONSTRUCTOR;

					return Flags;
				}

				if (IsArray)
					return (size_t)ObjectBehaviours::APP_ARRAY;

				return 0;
			}
		};

		struct ED_OUT ByteCodeInfo
		{
			Core::Vector<unsigned char> Data;
			Core::String Name;
			bool Valid = false;
			bool Debug = true;
		};

		struct ED_OUT PropertyInfo
		{
			const char* Name;
			const char* Namespace;
			int TypeId;
			bool IsConst;
			const char* ConfigGroup;
			void* Pointer;
			size_t AccessMask;
		};

		struct ED_OUT FunctionInfo
		{
			const char* Name;
			size_t AccessMask;
			int TypeId;
			int Offset;
			bool IsPrivate;
			bool IsProtected;
			bool IsReference;
		};

		struct ED_OUT MessageInfo
		{
		private:
			asSMessageInfo* Info;

		public:
			MessageInfo(asSMessageInfo* Info) noexcept;
			const char* GetSection() const;
			const char* GetText() const;
			LogCategory GetType() const;
			int GetRow() const;
			int GetColumn() const;
			asSMessageInfo* GetMessageInfo() const;
			bool IsValid() const;
		};

		struct ED_OUT TypeInfo
		{
		private:
			VirtualMachine* VM;
			asITypeInfo* Info;

		public:
			TypeInfo(asITypeInfo* TypeInfo) noexcept;
			void ForEachProperty(const PropertyCallback& Callback);
			void ForEachMethod(const MethodCallback& Callback);
			const char* GetGroup() const;
			size_t GetAccessMask() const;
			Module GetModule() const;
			int AddRef() const;
			int Release();
			const char* GetName() const;
			const char* GetNamespace() const;
			TypeInfo GetBaseType() const;
			bool DerivesFrom(const TypeInfo& Type) const;
			size_t GetFlags() const;
			unsigned int GetSize() const;
			int GetTypeId() const;
			int GetSubTypeId(unsigned int SubTypeIndex = 0) const;
			TypeInfo GetSubType(unsigned int SubTypeIndex = 0) const;
			unsigned int GetSubTypeCount() const;
			unsigned int GetInterfaceCount() const;
			TypeInfo GetInterface(unsigned int Index) const;
			bool Implements(const TypeInfo& Type) const;
			unsigned int GetFactoriesCount() const;
			Function GetFactoryByIndex(unsigned int Index) const;
			Function GetFactoryByDecl(const char* Decl) const;
			unsigned int GetMethodsCount() const;
			Function GetMethodByIndex(unsigned int Index, bool GetVirtual = true) const;
			Function GetMethodByName(const char* Name, bool GetVirtual = true) const;
			Function GetMethodByDecl(const char* Decl, bool GetVirtual = true) const;
			unsigned int GetPropertiesCount() const;
			int GetProperty(unsigned int Index, FunctionInfo* Out) const;
			const char* GetPropertyDeclaration(unsigned int Index, bool IncludeNamespace = false) const;
			unsigned int GetBehaviourCount() const;
			Function GetBehaviourByIndex(unsigned int Index, Behaviours* OutBehaviour) const;
			unsigned int GetChildFunctionDefCount() const;
			TypeInfo GetChildFunctionDef(unsigned int Index) const;
			TypeInfo GetParentType() const;
			unsigned int GetEnumValueCount() const;
			const char* GetEnumValueByIndex(unsigned int Index, int* OutValue) const;
			Function GetFunctionDefSignature() const;
			void* SetUserData(void* Data, size_t Type = 0);
			void* GetUserData(size_t Type = 0) const;
			bool IsHandle() const;
			bool IsValid() const;
			asITypeInfo* GetTypeInfo() const;
			VirtualMachine* GetVM() const;

		public:
			template <typename T>
			T* GetInstance(void* Object)
			{
				ED_ASSERT(Object != nullptr, nullptr, "object should be set");
				return IsHandle() ? *(T**)Object : (T*)Object;
			}
			template <typename T>
			T* GetProperty(void* Object, int Offset)
			{
				ED_ASSERT(Object != nullptr, nullptr, "object should be set");
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
				ED_ASSERT(Object != nullptr, nullptr, "object should be set");
				return IsHandle(TypeId) ? *(T**)Object : (T*)Object;
			}
			template <typename T>
			static T* GetProperty(void* Object, int Offset, int TypeId)
			{
				ED_ASSERT(Object != nullptr, nullptr, "object should be set");
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

		struct ED_OUT Function
		{
		private:
			VirtualMachine* VM;
			asIScriptFunction* Ptr;

		public:
			Function(asIScriptFunction* Base) noexcept;
			Function(const Function& Base) noexcept;
			int AddRef() const;
			int Release();
			int GetId() const;
			FunctionType GetType() const;
			const char* GetModuleName() const;
			Module GetModule() const;
			const char* GetSectionName() const;
			const char* GetGroup() const;
			size_t GetAccessMask() const;
			TypeInfo GetObjectType() const;
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
			TypeInfo GetDelegateObjectType() const;
			Function GetDelegateFunction() const;
			unsigned int GetPropertiesCount() const;
			int GetProperty(unsigned int Index, const char** Name, int* TypeId = nullptr) const;
			const char* GetPropertyDecl(unsigned int Index, bool IncludeNamespace = false) const;
			int FindNextLineWithCode(int Line) const;
			void* SetUserData(void* UserData, size_t Type = 0);
			void* GetUserData(size_t Type = 0) const;
			bool IsValid() const;
			asIScriptFunction* GetFunction() const;
			VirtualMachine* GetVM() const;
		};

		struct ED_OUT ScriptObject
		{
		private:
			asIScriptObject* Object;

		public:
			ScriptObject(asIScriptObject* Base) noexcept;
			int AddRef() const;
			int Release();
			TypeInfo GetObjectType();
			int GetTypeId();
			int GetPropertyTypeId(unsigned int Id) const;
			unsigned int GetPropertiesCount() const;
			const char* GetPropertyName(unsigned int Id) const;
			void* GetAddressOfProperty(unsigned int Id);
			VirtualMachine* GetVM() const;
			int CopyFrom(const ScriptObject& Other);
			void* SetUserData(void* Data, size_t Type = 0);
			void* GetUserData(size_t Type = 0) const;
			bool IsValid() const;
			asIScriptObject* GetObject() const;
		};

		struct ED_OUT BaseClass
		{
		protected:
			VirtualMachine* VM;
			Core::String Object;
			int TypeId;

		public:
			BaseClass(VirtualMachine* Engine, const Core::String& Name, int Type) noexcept;
			int SetFunctionDef(const char* Decl);
			int SetOperatorCopyAddress(asSFuncPtr* Value, FunctionCall = FunctionCall::THISCALL);
			int SetBehaviourAddress(const char* Decl, Behaviours Behave, asSFuncPtr* Value, FunctionCall = FunctionCall::THISCALL);
			int SetPropertyAddress(const char* Decl, int Offset);
			int SetPropertyStaticAddress(const char* Decl, void* Value);
			int SetOperatorAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type = FunctionCall::THISCALL);
			int SetMethodAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type = FunctionCall::THISCALL);
			int SetMethodStaticAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type = FunctionCall::CDECLF);
			int SetConstructorAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type = FunctionCall::CDECL_OBJFIRST);
			int SetConstructorListAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type = FunctionCall::CDECL_OBJFIRST);
			int SetDestructorAddress(const char* Decl, asSFuncPtr* Value);
			int GetTypeId() const;
			bool IsValid() const;
			Core::String GetName() const;
			VirtualMachine* GetVM() const;

		private:
			static Core::Stringify GetOperator(Operators Op, const char* Out, const char* Args, bool Const, bool Right);

		public:
			template <typename T>
			int SetEnumRefs(void(T::* Value)(asIScriptEngine*))
			{
				asSFuncPtr* EnumRefs = Bridge::Method<T>(Value);
				int Result = SetBehaviourAddress("void f(int &in)", Behaviours::ENUMREFS, EnumRefs, FunctionCall::THISCALL);
				FunctionFactory::ReleaseFunctor(&EnumRefs);

				return Result;
			}
			template <typename T>
			int SetReleaseRefs(void(T::* Value)(asIScriptEngine*))
			{
				asSFuncPtr* ReleaseRefs = Bridge::Method<T>(Value);
				int Result = SetBehaviourAddress("void f(int &in)", Behaviours::RELEASEREFS, ReleaseRefs, FunctionCall::THISCALL);
				FunctionFactory::ReleaseFunctor(&ReleaseRefs);

				return Result;
			}
			template <typename T>
			int SetEnumRefsEx(void(*Value)(T*, asIScriptEngine*))
			{
				asSFuncPtr* EnumRefs = Bridge::Function<void(*)(T*, asIScriptEngine*)>(Value);
				int Result = SetBehaviourAddress("void f(int &in)", Behaviours::ENUMREFS, EnumRefs, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&EnumRefs);

				return Result;
			}
			template <typename T>
			int SetReleaseRefsEx(void(*Value)(T*, asIScriptEngine*))
			{
				asSFuncPtr* ReleaseRefs = Bridge::Function<void(*)(T*, asIScriptEngine*)>(Value);
				int Result = SetBehaviourAddress("void f(int &in)", Behaviours::RELEASEREFS, ReleaseRefs, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&ReleaseRefs);

				return Result;
			}
			template <typename T, typename R>
			int SetProperty(const char* Decl, R T::* Value)
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				return SetPropertyAddress(Decl, (int)reinterpret_cast<size_t>(&(((T*)0)->*Value)));
			}
			template <typename T, typename R>
			int SetPropertyArray(const char* Decl, R T::* Value, size_t ElementsCount)
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				for (size_t i = 0; i < ElementsCount; i++)
				{
					Core::String ElementDecl = Decl + Core::ToString(i);
					int RE = SetPropertyAddress(ElementDecl.c_str(), (int)reinterpret_cast<size_t>(&(((T*)0)->*Value)) + (int)(sizeof(R) * i));
					if (RE < 0)
						return RE;
				}

				return 0;
			}
			template <typename T>
			int SetPropertyStatic(const char* Decl, T* Value)
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				return SetPropertyStaticAddress(Decl, (void*)Value);
			}
			template <typename T, typename R>
			int SetGetter(const char* Type, const char* Name, R(T::* Value)())
			{
				ED_ASSERT(Type != nullptr, -1, "type should be set");
				ED_ASSERT(Name != nullptr, -1, "name should be set");

				asSFuncPtr* Ptr = Bridge::Method<T, R>(Value);
				int Result = SetMethodAddress(Core::Form("%s get_%s()", Type, Name).Get(), Ptr, FunctionCall::THISCALL);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetGetterEx(const char* Type, const char* Name, R(*Value)(T*))
			{
				ED_ASSERT(Type != nullptr, -1, "type should be set");
				ED_ASSERT(Name != nullptr, -1, "name should be set");

				asSFuncPtr* Ptr = Bridge::Function(Value);
				int Result = SetMethodAddress(Core::Form("%s get_%s()", Type, Name).Get(), Ptr, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetSetter(const char* Type, const char* Name, void(T::* Value)(R))
			{
				ED_ASSERT(Type != nullptr, -1, "type should be set");
				ED_ASSERT(Name != nullptr, -1, "name should be set");

				asSFuncPtr* Ptr = Bridge::Method<T, void, R>(Value);
				int Result = SetMethodAddress(Core::Form("void set_%s(%s)", Name, Type).Get(), Ptr, FunctionCall::THISCALL);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetSetterEx(const char* Type, const char* Name, void(*Value)(T*, R))
			{
				ED_ASSERT(Type != nullptr, -1, "type should be set");
				ED_ASSERT(Name != nullptr, -1, "name should be set");

				asSFuncPtr* Ptr = Bridge::Function(Value);
				int Result = SetMethodAddress(Core::Form("void set_%s(%s)", Name, Type).Get(), Ptr, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetArrayGetter(const char* Type, const char* Name, R(T::* Value)(unsigned int))
			{
				ED_ASSERT(Type != nullptr, -1, "type should be set");
				ED_ASSERT(Name != nullptr, -1, "name should be set");

				asSFuncPtr* Ptr = Bridge::Method<T, R, unsigned int>(Value);
				int Result = SetMethodAddress(Core::Form("%s get_%s(uint)", Type, Name).Get(), Ptr, FunctionCall::THISCALL);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetArrayGetterEx(const char* Type, const char* Name, R(*Value)(T*, unsigned int))
			{
				ED_ASSERT(Type != nullptr, -1, "type should be set");
				ED_ASSERT(Name != nullptr, -1, "name should be set");

				asSFuncPtr* Ptr = Bridge::Function(Value);
				int Result = SetMethodAddress(Core::Form("%s get_%s(uint)", Type, Name).Get(), Ptr, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetArraySetter(const char* Type, const char* Name, void(T::* Value)(unsigned int, R))
			{
				ED_ASSERT(Type != nullptr, -1, "type should be set");
				ED_ASSERT(Name != nullptr, -1, "name should be set");

				asSFuncPtr* Ptr = Bridge::Method<T, void, unsigned int, R>(Value);
				int Result = SetMethodAddress(Core::Form("void set_%s(uint, %s)", Name, Type).Get(), Ptr, FunctionCall::THISCALL);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R>
			int SetArraySetterEx(const char* Type, const char* Name, void(*Value)(T*, unsigned int, R))
			{
				ED_ASSERT(Type != nullptr, -1, "type should be set");
				ED_ASSERT(Name != nullptr, -1, "name should be set");

				asSFuncPtr* Ptr = Bridge::Function(Value);
				int Result = SetMethodAddress(Core::Form("void set_%s(uint, %s)", Name, Type).Get(), Ptr, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R, typename... A>
			int SetOperator(Operators Type, uint32_t Opts, const char* Out, const char* Args, R(T::* Value)(A...))
			{
				ED_ASSERT(Out != nullptr, -1, "output should be set");
				Core::Stringify Operator = GetOperator(Type, Out, Args, Opts & (uint32_t)Position::Const, Opts & (uint32_t)Position::Right);

				ED_ASSERT(!Operator.Empty(), -1, "resulting operator should not be empty");
				asSFuncPtr* Ptr = Bridge::Method<T, R, A...>(Value);
				int Result = SetOperatorAddress(Operator.Get(), Ptr, FunctionCall::THISCALL);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename R, typename... A>
			int SetOperatorEx(Operators Type, uint32_t Opts, const char* Out, const char* Args, R(*Value)(A...))
			{
				ED_ASSERT(Out != nullptr, -1, "output should be set");
				Core::Stringify Operator = GetOperator(Type, Out, Args, Opts & (uint32_t)Position::Const, Opts & (uint32_t)Position::Right);

				ED_ASSERT(!Operator.Empty(), -1, "resulting operator should not be empty");
				asSFuncPtr* Ptr = Bridge::Function(Value);
				int Result = SetOperatorAddress(Operator.Get(), Ptr, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T>
			int SetOperatorCopy()
			{
				asSFuncPtr* Ptr = Bridge::MethodOp<T, T&, const T&>(&T::operator =);
				int Result = SetOperatorCopyAddress(Ptr, FunctionCall::THISCALL);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename R, typename... Args>
			int SetOperatorCopyStatic(R(*Value)(Args...), FunctionCall Type = FunctionCall::CDECLF)
			{
				asSFuncPtr* Ptr = (Type == FunctionCall::GENERIC ? Bridge::FunctionGeneric<R(*)(Args...)>(Value) : Bridge::Function<R(*)(Args...)>(Value));
				int Result = SetOperatorCopyAddress(Ptr, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R, typename... Args>
			int SetMethod(const char* Decl, R(T::* Value)(Args...))
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				asSFuncPtr* Ptr = Bridge::Method<T, R, Args...>(Value);
				int Result = SetMethodAddress(Decl, Ptr, FunctionCall::THISCALL);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename R, typename... Args>
			int SetMethod(const char* Decl, R(T::* Value)(Args...) const)
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				asSFuncPtr* Ptr = Bridge::Method<T, R, Args...>(Value);
				int Result = SetMethodAddress(Decl, Ptr, FunctionCall::THISCALL);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename R, typename... Args>
			int SetMethodEx(const char* Decl, R(*Value)(Args...))
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				asSFuncPtr* Ptr = Bridge::Function<R(*)(Args...)>(Value);
				int Result = SetMethodAddress(Decl, Ptr, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename R, typename... Args>
			int SetMethodStatic(const char* Decl, R(*Value)(Args...), FunctionCall Type = FunctionCall::CDECLF)
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				asSFuncPtr* Ptr = (Type == FunctionCall::GENERIC ? Bridge::FunctionGeneric<R(*)(Args...)>(Value) : Bridge::Function<R(*)(Args...)>(Value));
				int Result = SetMethodStaticAddress(Decl, Ptr, Type);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, typename To>
			void SetDynamicCast(const char* ToDecl, bool Implicit = false)
			{
				auto Type = Implicit ? Operators::ImplCast : Operators::Cast;
				SetOperatorEx(Type, 0, ToDecl, "", &BaseClass::DynamicCastFunction<T, To>);
				SetOperatorEx(Type, (uint32_t)Position::Const, ToDecl, "", &BaseClass::DynamicCastFunction<T, To>);
			}

		private:
			template <typename T, typename To>
			static To* DynamicCastFunction(T* Base)
			{
				return dynamic_cast<To*>(Base);
			}
		};

		struct ED_OUT RefClass : public BaseClass
		{
		public:
			RefClass(VirtualMachine* Engine, const Core::String& Name, int Type) noexcept : BaseClass(Engine, Name, Type)
			{
			}

		public:
			template <typename T, typename... Args>
			int SetConstructor(const char* Decl)
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				asSFuncPtr* Functor = Bridge::Function(&Bridge::GetUnmanagedCall<T, Args...>);
				int Result = SetBehaviourAddress(Decl, Behaviours::FACTORY, Functor, FunctionCall::CDECLF);
				FunctionFactory::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T, asIScriptGeneric*>
			int SetConstructor(const char* Decl)
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				asSFuncPtr* Functor = Bridge::FunctionGeneric(&Bridge::GetUnmanagedCall<T, asIScriptGeneric*>);
				int Result = SetBehaviourAddress(Decl, Behaviours::FACTORY, Functor, FunctionCall::GENERIC);
				FunctionFactory::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T>
			int SetConstructorList(const char* Decl)
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				asSFuncPtr* Functor = Bridge::FunctionGeneric(&Bridge::GetUnmanagedListCall<T>);
				int Result = SetBehaviourAddress(Decl, Behaviours::LIST_FACTORY, Functor, FunctionCall::GENERIC);
				FunctionFactory::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T>
			int SetConstructorListEx(const char* Decl, void(*Value)(asIScriptGeneric*))
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				asSFuncPtr* Functor = Bridge::FunctionGeneric(Value);
				int Result = SetBehaviourAddress(Decl, Behaviours::LIST_FACTORY, Functor, FunctionCall::GENERIC);
				FunctionFactory::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T, uint64_t TypeName, typename... Args>
			int SetGcConstructor(const char* Decl)
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				asSFuncPtr* Functor = Bridge::Function(&Bridge::GetManagedCall<T, TypeName, Args...>);
				int Result = SetBehaviourAddress(Decl, Behaviours::FACTORY, Functor, FunctionCall::CDECLF);
				FunctionFactory::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T, uint64_t TypeName, asIScriptGeneric*>
			int SetGcConstructor(const char* Decl)
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				asSFuncPtr* Functor = Bridge::FunctionGeneric(&Bridge::GetManagedCall<T, TypeName, asIScriptGeneric*>);
				int Result = SetBehaviourAddress(Decl, Behaviours::FACTORY, Functor, FunctionCall::GENERIC);
				FunctionFactory::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T, uint64_t TypeName>
			int SetGcConstructorList(const char* Decl)
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				asSFuncPtr* Functor = Bridge::FunctionGeneric(&Bridge::GetManagedListCall<T, TypeName>);
				int Result = SetBehaviourAddress(Decl, Behaviours::LIST_FACTORY, Functor, FunctionCall::GENERIC);
				FunctionFactory::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename T>
			int SetGcConstructorListEx(const char* Decl, void(*Value)(asIScriptGeneric*))
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				asSFuncPtr* Functor = Bridge::FunctionGeneric(Value);
				int Result = SetBehaviourAddress(Decl, Behaviours::LIST_FACTORY, Functor, FunctionCall::GENERIC);
				FunctionFactory::ReleaseFunctor(&Functor);

				return Result;
			}
			template <typename F>
			int SetAddRef()
			{
				auto FactoryPtr = &RefClass::GcAddRef<F>;
				asSFuncPtr* AddRef = Bridge::Function<decltype(FactoryPtr)>(FactoryPtr);
				int Result = SetBehaviourAddress("void f()", Behaviours::ADDREF, AddRef, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&AddRef);

				return Result;
			}
			template <typename F>
			int SetRelease()
			{
				auto FactoryPtr = &RefClass::GcRelease<F>;
				asSFuncPtr* Release = Bridge::Function<decltype(FactoryPtr)>(FactoryPtr);
				int Result = SetBehaviourAddress("void f()", Behaviours::RELEASE, Release, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Release);

				return Result;
			}
			template <typename F>
			int SetMarkRef()
			{
				auto FactoryPtr = &RefClass::GcMarkRef<F>;
				asSFuncPtr* Release = Bridge::Function<decltype(FactoryPtr)>(FactoryPtr);
				int Result = SetBehaviourAddress("void f()", Behaviours::SETGCFLAG, Release, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Release);

				return Result;
			}
			template <typename F>
			int SetIsMarkedRef()
			{
				auto FactoryPtr = &RefClass::GcIsMarkedRef<F>;
				asSFuncPtr* Release = Bridge::Function<decltype(FactoryPtr)>(FactoryPtr);
				int Result = SetBehaviourAddress("bool f()", Behaviours::GETGCFLAG, Release, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Release);

				return Result;
			}
			template <typename F>
			int SetRefCount()
			{
				auto FactoryPtr = &RefClass::GcGetRefCount<F>;
				asSFuncPtr* Release = Bridge::Function<decltype(FactoryPtr)>(FactoryPtr);
				int Result = SetBehaviourAddress("int f()", Behaviours::GETREFCOUNT, Release, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Release);

				return Result;
			}

		private:
			template <typename U>
			static void GcAddRef(U* Base)
			{
				Base->AddRef();
			}
			template <typename U>
			static void GcRelease(U* Base)
			{
				Base->Release();
			}
			template <typename U>
			static void GcMarkRef(U* Base)
			{
				Base->MarkRef();
			}
			template <typename U>
			static bool GcIsMarkedRef(U* Base)
			{
				return Base->IsMarkedRef();
			}
			template <typename U>
			static int GcGetRefCount(U* Base)
			{
				return (int)Base->GetRefCount();
			}
		};

		struct ED_OUT TypeClass : public BaseClass
		{
		public:
			TypeClass(VirtualMachine* Engine, const Core::String& Name, int Type) noexcept : BaseClass(Engine, Name, Type)
			{
			}

		public:
			template <typename T, typename... Args>
			int SetConstructor(const char* Decl)
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				asSFuncPtr* Ptr = Bridge::Function(&Bridge::GetConstructorCall<T, Args...>);
				int Result = SetConstructorAddress(Decl, Ptr, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T, asIScriptGeneric*>
			int SetConstructor(const char* Decl)
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				asSFuncPtr* Ptr = Bridge::FunctionGeneric(&Bridge::GetConstructorCall<T, asIScriptGeneric*>);
				int Result = SetConstructorAddress(Decl, Ptr, FunctionCall::GENERIC);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T>
			int SetConstructorList(const char* Decl)
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				asSFuncPtr* Ptr = Bridge::FunctionGeneric(&Bridge::GetConstructorListCall<T>);
				int Result = SetConstructorListAddress(Decl, Ptr, FunctionCall::GENERIC);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T>
			int SetDestructor(const char* Decl)
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				asSFuncPtr* Ptr = Bridge::Function(&Bridge::GetDestructorCall<T>);
				int Result = SetDestructorAddress(Decl, Ptr);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename R, typename... Args>
			int SetDestructorStatic(const char* Decl, R(*Value)(Args...))
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				asSFuncPtr* Ptr = Bridge::Function<R(*)(Args...)>(Value);
				int Result = SetDestructorAddress(Decl, Ptr);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
		};

		struct ED_OUT TypeInterface
		{
		private:
			VirtualMachine* VM;
			Core::String Object;
			int TypeId;

		public:
			TypeInterface(VirtualMachine* Engine, const Core::String& Name, int Type) noexcept;
			int SetMethod(const char* Decl);
			int GetTypeId() const;
			bool IsValid() const;
			Core::String GetName() const;
			VirtualMachine* GetVM() const;
		};

		struct ED_OUT Enumeration
		{
		private:
			VirtualMachine* VM;
			Core::String Object;
			int TypeId;

		public:
			Enumeration(VirtualMachine* Engine, const Core::String& Name, int Type) noexcept;
			int SetValue(const char* Name, int Value);
			int GetTypeId() const;
			bool IsValid() const;
			Core::String GetName() const;
			VirtualMachine* GetVM() const;
		};

		struct ED_OUT Module
		{
		private:
			VirtualMachine* VM;
			asIScriptModule* Mod;

		public:
			Module(asIScriptModule* Type) noexcept;
			void SetName(const char* Name);
			int AddSection(const char* Name, const char* Code, size_t CodeLength = 0, int LineOffset = 0);
			int RemoveFunction(const Function& Function);
			int ResetProperties(asIScriptContext* Context = nullptr);
			int Build();
			int LoadByteCode(ByteCodeInfo* Info);
			int Discard();
			int BindImportedFunction(size_t ImportIndex, const Function& Function);
			int UnbindImportedFunction(size_t ImportIndex);
			int BindAllImportedFunctions();
			int UnbindAllImportedFunctions();
			int CompileFunction(const char* SectionName, const char* Code, int LineOffset, size_t CompileFlags, Function* OutFunction);
			int CompileProperty(const char* SectionName, const char* Code, int LineOffset);
			int SetDefaultNamespace(const char* Namespace);
			void* GetAddressOfProperty(size_t Index);
			int RemoveProperty(size_t Index);
			size_t SetAccessMask(size_t AccessMask);
			size_t GetFunctionCount() const;
			Function GetFunctionByIndex(size_t Index) const;
			Function GetFunctionByDecl(const char* Decl) const;
			Function GetFunctionByName(const char* Name) const;
			int GetTypeIdByDecl(const char* Decl) const;
			int GetImportedFunctionIndexByDecl(const char* Decl) const;
			int SaveByteCode(ByteCodeInfo* Info) const;
			int GetPropertyIndexByName(const char* Name) const;
			int GetPropertyIndexByDecl(const char* Decl) const;
			int GetProperty(size_t Index, PropertyInfo* Out) const;
			size_t GetAccessMask() const;
			size_t GetObjectsCount() const;
			size_t GetPropertiesCount() const;
			size_t GetEnumsCount() const;
			size_t GetImportedFunctionCount() const;
			TypeInfo GetObjectByIndex(size_t Index) const;
			TypeInfo GetTypeInfoByName(const char* Name) const;
			TypeInfo GetTypeInfoByDecl(const char* Decl) const;
			TypeInfo GetEnumByIndex(size_t Index) const;
			const char* GetPropertyDecl(size_t Index, bool IncludeNamespace = false) const;
			const char* GetDefaultNamespace() const;
			const char* GetImportedFunctionDecl(size_t ImportIndex) const;
			const char* GetImportedFunctionModule(size_t ImportIndex) const;
			const char* GetName() const;
			bool IsValid() const;
			asIScriptModule* GetModule() const;
			VirtualMachine* GetVM() const;

		public:
			template <typename T>
			int SetTypeProperty(const char* Name, T* Value)
			{
				ED_ASSERT(Name != nullptr, -1, "name should be set");
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
				ED_ASSERT(Name != nullptr, -1, "name should be set");
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
				ED_ASSERT(Name != nullptr, -1, "name should be set");
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

		class ED_OUT Compiler final : public Core::Reference<Compiler>
		{
		private:
			static int CompilerUD;

		private:
			Compute::ProcIncludeCallback Include;
			Compute::ProcPragmaCallback Pragma;
			Compute::Preprocessor* Processor;
			asIScriptModule* Scope;
			VirtualMachine* VM;
			ImmediateContext* Context;
			ByteCodeInfo VCache;
			bool BuiltOK;

		public:
			Compiler(VirtualMachine* Engine) noexcept;
			~Compiler() noexcept;
			void SetIncludeCallback(const Compute::ProcIncludeCallback& Callback);
			void SetPragmaCallback(const Compute::ProcPragmaCallback& Callback);
			void Define(const Core::String& Word);
			void Undefine(const Core::String& Word);
			bool Clear();
			bool IsDefined(const Core::String& Word) const;
			bool IsBuilt() const;
			bool IsCached() const;
			int Prepare(ByteCodeInfo* Info);
			int Prepare(const Core::String& ModuleName, bool Scoped = false);
			int Prepare(const Core::String& ModuleName, const Core::String& Cache, bool Debug = true, bool Scoped = false);
			int SaveByteCode(ByteCodeInfo* Info);
			int LoadFile(const Core::String& Path);
			int LoadCode(const Core::String& Name, const Core::String& Buffer);
			int LoadCode(const Core::String& Name, const char* Buffer, size_t Length);
			Core::Promise<int> Compile();
			Core::Promise<int> LoadByteCode(ByteCodeInfo* Info);
			Core::Promise<int> ExecuteFile(const char* Name, const char* ModuleName, const char* EntryName, ArgsCallback&& OnArgs = nullptr);
			Core::Promise<int> ExecuteMemory(const Core::String& Buffer, const char* ModuleName, const char* EntryName, ArgsCallback&& OnArgs = nullptr);
			Core::Promise<int> ExecuteEntry(const char* Name, ArgsCallback&& OnArgs = nullptr);
			Core::Promise<int> ExecuteScoped(const Core::String& Code, const char* Args = nullptr, ArgsCallback&& OnArgs = nullptr);
			Core::Promise<int> ExecuteScoped(const char* Buffer, size_t Length, const char* Args = nullptr, ArgsCallback&& OnArgs = nullptr);
			Module GetModule() const;
			VirtualMachine* GetVM() const;
			ImmediateContext* GetContext() const;
			Compute::Preprocessor* GetProcessor() const;

		private:
			static Compiler* Get(ImmediateContext* Context);
		};

		class ED_OUT ImmediateContext final : public Core::Reference<ImmediateContext>
		{
		private:
			static int ContextUD;

		private:
			struct Task
			{
				Core::Promise<int> Future;
				Function Callback = nullptr;
				ArgsCallback Args;
			};

		private:
			std::function<void(ImmediateContext*)> LineCallback;
			std::function<void(ImmediateContext*)> ExceptionCallback;
			Core::SingleQueue<Task> Tasks;
			std::mutex Exchange;
			Core::String Stacktrace;
			asIScriptContext* Context;
			VirtualMachine* VM;

		public:
			ImmediateContext(asIScriptContext* Base) noexcept;
			~ImmediateContext() noexcept;
			Core::Promise<int> TryExecute(bool IsNested, const Function& Function, ArgsCallback&& OnArgs);
			int SetOnException(void(*Callback)(asIScriptContext* Context, void* Object), void* Object);
			int Prepare(const Function& Function);
			int Unprepare();
			int Execute();
			int Abort();
			int Suspend();
			Activation GetState() const;
			Core::String GetStackTrace(size_t Skips, size_t MaxFrames) const;
			int PushState();
			int PopState();
			bool IsNested(unsigned int* NestCount = 0) const;
			bool IsThrown() const;
			bool IsPending();
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
			int GetReturnableByType(void* Return, asITypeInfo* ReturnTypeId);
			int GetReturnableByDecl(void* Return, const char* ReturnTypeDecl);
			int GetReturnableById(void* Return, int ReturnTypeId);
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
			Function GetExceptionFunction();
			const char* GetExceptionString();
			bool WillExceptionBeCaught();
			void ClearExceptionCallback();
			int SetLineCallback(void(*Callback)(asIScriptContext* Context, void* Object), void* Object);
			int SetLineCallback(const std::function<void(ImmediateContext*)>& Callback);
			int SetExceptionCallback(const std::function<void(ImmediateContext*)>& Callback);
			void ClearLineCallback();
			unsigned int GetCallstackSize() const;
			Core::String GetErrorStackTrace();
			Function GetFunction(unsigned int StackLevel = 0);
			int GetLineNumber(unsigned int StackLevel = 0, int* Column = 0, const char** SectionName = 0);
			int GetPropertiesCount(unsigned int StackLevel = 0);
			const char* GetPropertyName(unsigned int Index, unsigned int StackLevel = 0);
			const char* GetPropertyDecl(unsigned int Index, unsigned int StackLevel = 0, bool IncludeNamespace = false);
			int GetPropertyTypeId(unsigned int Index, unsigned int StackLevel = 0);
			void* GetAddressOfProperty(unsigned int Index, unsigned int StackLevel = 0);
			bool IsPropertyInScope(unsigned int Index, unsigned int StackLevel = 0);
			int GetThisTypeId(unsigned int StackLevel = 0);
			void* GetThisPointer(unsigned int StackLevel = 0);
			Function GetSystemFunction();
			bool IsSuspended() const;
			void* SetUserData(void* Data, size_t Type = 0);
			void* GetUserData(size_t Type = 0) const;
			asIScriptContext* GetContext();
			VirtualMachine* GetVM();

		public:
			template <typename T>
			T* GetReturnObject()
			{
				return (T*)GetReturnObjectAddress();
			}

		public:
			static ImmediateContext* Get(asIScriptContext* Context);
			static ImmediateContext* Get();

		private:
			static void LineLogger(asIScriptContext* Context, void* Object);
			static void ExceptionLogger(asIScriptContext* Context, void* Object);
		};

		class ED_OUT VirtualMachine final : public Core::Reference<VirtualMachine>
		{
		public:
			typedef std::function<void(const Core::String&)> CompileCallback;
			typedef std::function<void()> WhenErrorCallback;

		private:
			struct Kernel
			{
				Core::UnorderedMap<Core::String, void*> Functions;
				void* Handle;
			};

			struct Submodule
			{
				Core::Vector<Core::String> Dependencies;
				SubmoduleCallback Callback;
				bool Registered = false;
			};

		private:
			struct
			{
				std::mutex General;
				std::mutex Pool;
			} Sync;

		private:
			static int ManagerUD;

		private:
			Core::UnorderedMap<Core::String, Core::String> Files;
			Core::UnorderedMap<Core::String, Core::Schema*> Datas;
			Core::UnorderedMap<Core::String, ByteCodeInfo> Opcodes;
			Core::UnorderedMap<Core::String, Kernel> Kernels;
			Core::UnorderedMap<Core::String, Submodule> Modules;
			Core::UnorderedMap<Core::String, CompileCallback> Callbacks;
			Core::Vector<asIScriptContext*> Contexts;
			Core::String DefaultNamespace;
			Compute::Preprocessor::Desc Proc;
			Compute::IncludeDesc Include;
			WhenErrorCallback WherError;
			uint64_t Scope;
			asIScriptEngine* Engine;
			unsigned int Imports;
			bool Cached;

		public:
			VirtualMachine() noexcept;
			~VirtualMachine() noexcept;
			void SetImports(unsigned int Opts);
			void SetCache(bool Enabled);
			void ClearCache();
			void SetCompilerErrorCallback(const WhenErrorCallback& Callback);
			void SetCompilerIncludeOptions(const Compute::IncludeDesc& NewDesc);
			void SetCompilerFeatures(const Compute::Preprocessor::Desc& NewDesc);
			void SetProcessorOptions(Compute::Preprocessor* Processor);
			void SetCompileCallback(const Core::String& Section, CompileCallback&& Callback);
			int Collect(size_t NumIterations = 1);
			void GetStatistics(unsigned int* CurrentSize, unsigned int* TotalDestroyed, unsigned int* TotalDetected, unsigned int* NewObjects, unsigned int* TotalNewDestroyed) const;
			int NotifyOfNewObject(void* Object, const TypeInfo& Type);
			int GetObjectAddress(size_t Index, size_t* SequenceNumber = nullptr, void** Object = nullptr, TypeInfo* Type = nullptr);
			void ForwardEnumReferences(void* Reference, const TypeInfo& Type);
			void ForwardReleaseReferences(void* Reference, const TypeInfo& Type);
			void GCEnumCallback(void* Reference);
			bool DumpRegisteredInterfaces(const Core::String& Path);
			bool DumpAllInterfaces(const Core::String& Path);
			bool GetByteCodeCache(ByteCodeInfo* Info);
			void SetByteCodeCache(ByteCodeInfo* Info);
			Core::Unique<ImmediateContext> CreateContext();
			Core::Unique<Compiler> CreateCompiler();
			asIScriptModule* CreateScopedModule(const Core::String& Name);
			asIScriptModule* CreateModule(const Core::String& Name);
			void* CreateObject(const TypeInfo& Type);
			void* CreateObjectCopy(void* Object, const TypeInfo& Type);
			void* CreateEmptyObject(const TypeInfo& Type);
			Function CreateDelegate(const Function& Function, void* Object);
			int AssignObject(void* DestObject, void* SrcObject, const TypeInfo& Type);
			void ReleaseObject(void* Object, const TypeInfo& Type);
			void AddRefObject(void* Object, const TypeInfo& Type);
			int RefCastObject(void* Object, const TypeInfo& FromType, const TypeInfo& ToType, void** NewPtr, bool UseOnlyImplicitCast = false);
			int BeginGroup(const char* GroupName);
			int EndGroup();
			int RemoveGroup(const char* GroupName);
			int BeginNamespace(const char* Namespace);
			int BeginNamespaceIsolated(const char* Namespace, size_t DefaultMask);
			int EndNamespace();
			int EndNamespaceIsolated();
			int Namespace(const char* Namespace, const std::function<int(VirtualMachine*)>& Callback);
			int NamespaceIsolated(const char* Namespace, size_t DefaultMask, const std::function<int(VirtualMachine*)>& Callback);
			int GetTypeNameScope(const char** TypeName, const char** Namespace, size_t* NamespaceSize) const;
			size_t BeginAccessMask(size_t DefaultMask);
			size_t EndAccessMask();
			const char* GetNamespace() const;
			Module GetModule(const char* Name);
			int SetLogCallback(void(*Callback)(const asSMessageInfo* Message, void* Object), void* Object);
			int Log(const char* Section, int Row, int Column, LogCategory Type, const char* Message);
			int SetProperty(Features Property, size_t Value);
			void SetDocumentRoot(const Core::String& Root);
			size_t GetProperty(Features Property) const;
			asIScriptEngine* GetEngine() const;
			Core::String GetDocumentRoot() const;
			Core::Vector<Core::String> GetSubmodules() const;
			Core::Vector<Core::String> VerifyModules(const Core::String& Directory, const Compute::RegexSource& Exp);
			bool VerifyModule(const Core::String& Path);
			bool IsNullable(int TypeId);
			bool AddSubmodule(const Core::String& Name, const Core::Vector<Core::String>& Dependencies, const SubmoduleCallback& Callback);
			bool ImportFile(const Core::String& Path, Core::String* Out);
			bool ImportSymbol(const Core::Vector<Core::String>& Sources, const Core::String& Name, const Core::String& Decl);
			bool ImportLibrary(const Core::String& Path);
			bool ImportSubmodule(const Core::String& Name);
			Core::Schema* ImportJSON(const Core::String& Path);
			int SetFunctionDef(const char* Decl);
			int SetFunctionAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type = FunctionCall::CDECLF);
			int SetPropertyAddress(const char* Decl, void* Value);
			TypeClass SetStructAddress(const char* Name, size_t Size, uint64_t Flags = (uint64_t)ObjectBehaviours::VALUE);
			TypeClass SetPodAddress(const char* Name, size_t Size, uint64_t Flags = (uint64_t)(ObjectBehaviours::VALUE | ObjectBehaviours::POD));
			RefClass SetClassAddress(const char* Name, uint64_t Flags = (uint64_t)ObjectBehaviours::REF);
			TypeInterface SetInterface(const char* Name);
			Enumeration SetEnum(const char* Name);
			size_t GetFunctionsCount() const;
			Function GetFunctionById(int Id) const;
			Function GetFunctionByIndex(int Index) const;
			Function GetFunctionByDecl(const char* Decl) const;
			size_t GetPropertiesCount() const;
			int GetPropertyByIndex(int Index, PropertyInfo* Info) const;
			int GetPropertyIndexByName(const char* Name) const;
			int GetPropertyIndexByDecl(const char* Decl) const;
			size_t GetObjectsCount() const;
			TypeInfo GetObjectByIndex(size_t Index) const;
			size_t GetEnumCount() const;
			TypeInfo GetEnumByIndex(size_t Index) const;
			size_t GetFunctionDefsCount() const;
			TypeInfo GetFunctionDefByIndex(int Index) const;
			size_t GetModulesCount() const;
			asIScriptModule* GetModuleById(int Id) const;
			int GetTypeIdByDecl(const char* Decl) const;
			const char* GetTypeIdDecl(int TypeId, bool IncludeNamespace = false) const;
			int GetSizeOfPrimitiveType(int TypeId) const;
			Core::String GetObjectView(void* Object, int TypeId);
			TypeInfo GetTypeInfoById(int TypeId) const;
			TypeInfo GetTypeInfoByName(const char* Name);
			TypeInfo GetTypeInfoByDecl(const char* Decl) const;

		public:
			static void SetMemoryFunctions(void* (*Alloc)(size_t), void(*Free)(void*));
			static void CleanupThisThread();
			static VirtualMachine* Get(asIScriptEngine* Engine);
			static VirtualMachine* Get();
			static size_t GetDefaultAccessMask();
			static void FreeProxy();

		private:
			static Core::String GetLibraryName(const Core::String& Path);
			static asIScriptContext* RequestContext(asIScriptEngine* Engine, void* Data);
			static void ReturnContext(asIScriptEngine* Engine, asIScriptContext* Context, void* Data);
			static void CompileLogger(asSMessageInfo* Info, void* Object);
			static void RegisterSubmodules(VirtualMachine* Engine);
			static void* GetNullable();

		public:
			template <typename T>
			int SetFunction(const char* Decl, T Value)
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				asSFuncPtr* Ptr = Bridge::Function<T>(Value);
				int Result = SetFunctionAddress(Decl, Ptr, FunctionCall::CDECLF);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <void(*)(asIScriptGeneric*)>
			int SetFunction(const char* Decl, void(*Value)(asIScriptGeneric*))
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				asSFuncPtr* Ptr = Bridge::Function<void (*)(asIScriptGeneric*)>(Value);
				int Result = SetFunctionAddress(Decl, Ptr, FunctionCall::GENERIC);
				FunctionFactory::ReleaseFunctor(&Ptr);

				return Result;
			}
			template <typename T>
			int SetProperty(const char* Decl, T* Value)
			{
				ED_ASSERT(Decl != nullptr, -1, "declaration should be set");
				return SetPropertyAddress(Decl, (void*)Value);
			}
			template <typename T>
			RefClass SetClass(const char* Name, bool GC)
			{
				ED_ASSERT(Name != nullptr, RefClass(nullptr, "", -1), "name should be set");
				RefClass Class = SetClassAddress(Name, GC ? (size_t)ObjectBehaviours::REF | (size_t)ObjectBehaviours::GC : (size_t)ObjectBehaviours::REF);
				Class.SetAddRef<T>();
				Class.SetRelease<T>();

				if (GC)
				{
					Class.SetMarkRef<T>();
					Class.SetIsMarkedRef<T>();
					Class.SetRefCount<T>();
				}

				return Class;
			}
			template <typename T>
			TypeClass SetStructTrivial(const char* Name)
			{
				ED_ASSERT(Name != nullptr, TypeClass(nullptr, "", -1), "name should be set");
				TypeClass Struct = SetStructAddress(Name, sizeof(T), (size_t)ObjectBehaviours::VALUE | Bridge::GetTypeTraits<T>());
				Struct.SetOperatorCopy<T>();
				Struct.SetDestructor<T>("void f()");

				return Struct;
			}
			template <typename T>
			TypeClass SetStruct(const char* Name)
			{
				ED_ASSERT(Name != nullptr, TypeClass(nullptr, "", -1), "name should be set");
				return SetStructAddress(Name, sizeof(T), (size_t)ObjectBehaviours::VALUE | Bridge::GetTypeTraits<T>());
			}
			template <typename T>
			TypeClass SetPod(const char* Name)
			{
				ED_ASSERT(Name != nullptr, TypeClass(nullptr, "", -1), "name should be set");
				return SetPodAddress(Name, sizeof(T), (size_t)ObjectBehaviours::VALUE | (size_t)ObjectBehaviours::POD | Bridge::GetTypeTraits<T>());
			}
		};

		class ED_OUT Debugger final : public Core::Reference<Debugger>
		{
		public:
			typedef Core::String(*ToStringCallback)(void* Object, int ExpandLevel, Debugger* Dbg);

		protected:
			enum class DebugAction
			{
				CONTINUE,
				STEP_INTO,
				STEP_OVER,
				STEP_OUT
			};

			struct BreakPoint
			{
				Core::String Name;
				bool NeedsAdjusting;
				bool Function;
				int Line;

				BreakPoint(const Core::String& N, int L, bool F) noexcept : Name(N), NeedsAdjusting(true), Function(F), Line(L)
				{
				}
			};

		protected:
			Core::UnorderedMap<const asITypeInfo*, ToStringCallback> ToStringCallbacks;
			Core::Vector<BreakPoint> BreakPoints;
			unsigned int LastCommandAtStackLevel;
			asIScriptFunction* LastFunction;
			VirtualMachine* VM;
			DebugAction Action;

		public:
			Debugger() noexcept;
			~Debugger() noexcept;
			void RegisterToStringCallback(const TypeInfo& Type, ToStringCallback Callback);
			void TakeCommands(ImmediateContext* Context);
			void Output(const Core::String& Data);
			void LineCallback(ImmediateContext* Context);
			void PrintHelp();
			void AddFileBreakPoint(const Core::String& File, int LineNumber);
			void AddFuncBreakPoint(const Core::String& Function);
			void ListBreakPoints();
			void ListLocalVariables(ImmediateContext* Context);
			void ListGlobalVariables(ImmediateContext* Context);
			void ListMemberProperties(ImmediateContext* Context);
			void ListStatistics(ImmediateContext* Context);
			void PrintCallstack(ImmediateContext* Context);
			void PrintValue(const Core::String& Expression, ImmediateContext* Context);
			void SetEngine(VirtualMachine* Engine);
			bool InterpretCommand(const Core::String& Command, ImmediateContext* Context);
			bool CheckBreakPoint(ImmediateContext* Context);
			Core::String ToString(void* Value, unsigned int TypeId, int ExpandMembersLevel, VirtualMachine* Engine);
			VirtualMachine* GetEngine();
		};
	}
}
#endif
