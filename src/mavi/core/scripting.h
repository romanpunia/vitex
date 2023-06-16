#ifndef VI_SCRIPTING_H
#define VI_SCRIPTING_H
#include "compute.h"
#include <type_traits>
#include <random>
#include <queue>

class asIScriptEngine;
class asIScriptContext;
class asIScriptModule;
class asIScriptTranslator;
class asITypeInfo;
class asIScriptFunction;
class asIScriptGeneric;
class asIScriptObject;
class asILockableSharedBool;
struct asSFuncPtr;
struct asSMessageInfo;

namespace Mavi
{
	namespace Scripting
	{
		struct Module;

		struct Function;

		struct FunctionDelegate;

		class VirtualMachine;

		class ImmediateContext;

		class DummyPtr
		{
		};

		enum class DebugType
		{
			Suspended,
			Attach,
			Detach
		};

		enum class TranslationOptions
		{
			No_Suspend = 0x01,
			Syscall_FPU_No_Reset = 0x02,
			Syscall_No_Errors = 0x04,
			Alloc_Simple = 0x08,
			No_Switches = 0x10,
			No_Script_Calls = 0x20,
			Fast_Ref_Counter = 0x40,
			Optimal = Alloc_Simple | Fast_Ref_Counter,
			Disabled = -1
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
			MAX_CALL_STACK_SIZE = 31,
			IGNORE_DUPLICATE_SHARED_INTF = 32,
			NO_DEBUG_OUTPUT = 33,
		};

		enum class LibraryFeatures
		{
			PromiseNoCallbacks = 0,
			PromiseNoConstructor = 1
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
			Finished = 0,
			Suspended = 1,
			Aborted = 2,
			Exception = 3,
			Prepared = 4,
			Uninitialized = 5,
			Active = 6,
			Error = 7,
			Deserialization = 8
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
			CFunctions = 2,
			Addons = 4,
			Files = 8,
			Remotes = 8,
			All = (CLibraries | CFunctions | Addons | Files | Remotes)
		};

		enum class GarbageCollector
		{
			FULL_CYCLE = 1,
			ONE_STEP = 2,
			DESTROY_GARBAGE = 4,
			DETECT_GARBAGE = 8
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
		inline GarbageCollector operator |(GarbageCollector A, GarbageCollector B)
		{
			return static_cast<GarbageCollector>(static_cast<size_t>(A) | static_cast<size_t>(B));
		}

		typedef void(DummyPtr::* DummyMethodPtr)();
		typedef void(*FunctionPtr)();
		typedef std::function<void(struct TypeInfo*, struct FunctionInfo*)> PropertyCallback;
		typedef std::function<void(struct TypeInfo*, struct Function*)> MethodCallback;
		typedef std::function<void(class VirtualMachine*)> AddonCallback;
		typedef std::function<void(class ImmediateContext*)> ArgsCallback;

		template <typename T>
		using ExpectedReturn = Core::Expects<T, Errors>;

		template <typename T>
		using ExpectedFuture = Core::Promise<ExpectedReturn<T>>;

		class VI_OUT TypeCache : public Core::Singletonish
		{
		private:
			static Core::Mapping<Core::UnorderedMap<uint64_t, std::pair<Core::String, int>>>* Names;

		public:
			static uint64_t Set(uint64_t Id, const Core::String& Name);
			static int GetTypeId(uint64_t Id);
			static void Cleanup();
		};

		class VI_OUT_TS FunctionFactory
		{
		public:
			static Core::Unique<asSFuncPtr> CreateFunctionBase(void(*Base)(), int Type);
			static Core::Unique<asSFuncPtr> CreateMethodBase(const void* Base, size_t Size, int Type);
			static Core::Unique<asSFuncPtr> CreateDummyBase();
			static ExpectedReturn<void> AtomicNotifyGC(const char* TypeName, void* Object);
			static ExpectedReturn<void> AtomicNotifyGCById(int TypeId, void* Object);
			static void ReplacePreconditions(const Core::String& Keyword, Core::String& Data, const std::function<Core::String(const Core::String& Expression)>& Replacer);
			static void ReleaseFunctor(Core::Unique<asSFuncPtr>* Ptr);
			static void GCEnumCallback(asIScriptEngine* Engine, void* Reference);
			static void GCEnumCallback(asIScriptEngine* Engine, asIScriptFunction* Reference);
			static void GCEnumCallback(asIScriptEngine* Engine, FunctionDelegate* Reference);

		public:
			template <typename T>
			static ExpectedReturn<T> ToReturn(int Code, T&& Value)
			{
				if (Code < 0)
					return (Errors)Code;

				return Value;
			}
			static ExpectedReturn<void> ToReturn(int Code)
			{
				if (Code < 0)
					return (Errors)Code;

				return Core::Optional::OK;
			}
		};

		template <int N>
		struct FunctionBinding
		{
			template <class M>
			static Core::Unique<asSFuncPtr> Bind(M Value)
			{
				return FunctionFactory::CreateDummyBase();
			}
		};

		template <>
		struct FunctionBinding<sizeof(DummyMethodPtr)>
		{
			template <class M>
			static Core::Unique<asSFuncPtr> Bind(M Value)
			{
				return FunctionFactory::CreateMethodBase(&Value, sizeof(DummyMethodPtr), 3);
			}
		};
#if defined(_MSC_VER) && !defined(__MWERKS__)
		template <>
		struct FunctionBinding<sizeof(DummyMethodPtr) + 1 * sizeof(int)>
		{
			template <class M>
			static Core::Unique<asSFuncPtr> Bind(M Value)
			{
				return FunctionFactory::CreateMethodBase(&Value, sizeof(DummyMethodPtr) + sizeof(int), 3);
			}
		};

		template <>
		struct FunctionBinding<sizeof(DummyMethodPtr) + 2 * sizeof(int)>
		{
			template <class M>
			static Core::Unique<asSFuncPtr> Bind(M Value)
			{
				asSFuncPtr* Ptr = FunctionFactory::CreateMethodBase(&Value, sizeof(DummyMethodPtr) + 2 * sizeof(int), 3);
#if defined(_MSC_VER) && !defined(VI_64)
				* (reinterpret_cast<unsigned long*>(Ptr) + 3) = *(reinterpret_cast<unsigned long*>(Ptr) + 2);
#endif
				return Ptr;
			}
		};

		template <>
		struct FunctionBinding<sizeof(DummyMethodPtr) + 3 * sizeof(int)>
		{
			template <class M>
			static Core::Unique<asSFuncPtr> Bind(M Value)
			{
				return FunctionFactory::CreateMethodBase(&Value, sizeof(DummyMethodPtr) + 3 * sizeof(int), 3);
			}
		};

		template <>
		struct FunctionBinding<sizeof(DummyMethodPtr) + 4 * sizeof(int)>
		{
			template <class M>
			static Core::Unique<asSFuncPtr> Bind(M Value)
			{
				return FunctionFactory::CreateMethodBase(&Value, sizeof(DummyMethodPtr) + 4 * sizeof(int), 3);
			}
		};
#endif
		class VI_OUT GenericContext
		{
		private:
			VirtualMachine* VM;
			asIScriptGeneric* Generic;

		public:
			GenericContext(asIScriptGeneric* Base) noexcept;
			void* GetObjectAddress();
			int GetObjectTypeId() const;
			size_t GetArgsCount() const;
			ExpectedReturn<void> GetArgTypeId(size_t Argument, size_t* Flags = 0) const;
			unsigned char GetArgByte(size_t Argument);
			unsigned short GetArgWord(size_t Argument);
			size_t GetArgDWord(size_t Argument);
			uint64_t GetArgQWord(size_t Argument);
			float GetArgFloat(size_t Argument);
			double GetArgDouble(size_t Argument);
			void* GetArgAddress(size_t Argument);
			void* GetArgObjectAddress(size_t Argument);
			void* GetAddressOfArg(size_t Argument);
			int GetReturnTypeId(size_t* Flags = 0) const;
			ExpectedReturn<void> SetReturnByte(unsigned char Value);
			ExpectedReturn<void> SetReturnWord(unsigned short Value);
			ExpectedReturn<void> SetReturnDWord(size_t Value);
			ExpectedReturn<void> SetReturnQWord(uint64_t Value);
			ExpectedReturn<void> SetReturnFloat(float Value);
			ExpectedReturn<void> SetReturnDouble(double Value);
			ExpectedReturn<void> SetReturnAddress(void* Address);
			ExpectedReturn<void> SetReturnObjectAddress(void* Object);
			void* GetAddressOfReturnLocation();
			bool IsValid() const;
			asIScriptGeneric* GetGeneric() const;
			VirtualMachine* GetVM() const;

		public:
			template <typename T>
			ExpectedReturn<void> SetReturnObject(T* Object)
			{
				return SetReturnObjectAddress((void*)Object);
			}
			template <typename T>
			T* GetArgObject(size_t Arg)
			{
				return (T*)GetArgObjectAddress(Arg);
			}
		};

		class VI_OUT Bridge
		{
		public:
			template <typename T>
			static Core::Unique<asSFuncPtr> Function(T Value)
			{
#ifdef VI_64
				void(*Address)() = reinterpret_cast<void(*)()>(size_t(Value));
#else
				void (*Address)() = reinterpret_cast<void (*)()>(Value);
#endif
				return FunctionFactory::CreateFunctionBase(Address, 2);
			}
			template <typename T>
			static Core::Unique<asSFuncPtr> FunctionGeneric(T Value)
			{
#ifdef VI_64
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

		struct VI_OUT ByteCodeInfo
		{
			Core::Vector<unsigned char> Data;
			Core::String Name;
			bool Valid = false;
			bool Debug = true;
		};

		struct VI_OUT ByteCodeLabel
		{
			const char* Name;
			uint8_t Id;
			uint8_t Args;
		};

		struct VI_OUT PropertyInfo
		{
			const char* Name;
			const char* Namespace;
			int TypeId;
			bool IsConst;
			const char* ConfigGroup;
			void* Pointer;
			size_t AccessMask;
		};

		struct VI_OUT FunctionInfo
		{
			const char* Name;
			size_t AccessMask;
			int TypeId;
			int Offset;
			bool IsPrivate;
			bool IsProtected;
			bool IsReference;
		};

		struct VI_OUT MessageInfo
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

		struct VI_OUT TypeInfo
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
			void AddRef() const;
			void Release();
			const char* GetName() const;
			const char* GetNamespace() const;
			TypeInfo GetBaseType() const;
			bool DerivesFrom(const TypeInfo& Type) const;
			size_t GetFlags() const;
			size_t GetSize() const;
			int GetTypeId() const;
			int GetSubTypeId(size_t SubTypeIndex = 0) const;
			TypeInfo GetSubType(size_t SubTypeIndex = 0) const;
			size_t GetSubTypeCount() const;
			size_t GetInterfaceCount() const;
			TypeInfo GetInterface(size_t Index) const;
			bool Implements(const TypeInfo& Type) const;
			size_t GetFactoriesCount() const;
			Function GetFactoryByIndex(size_t Index) const;
			Function GetFactoryByDecl(const char* Decl) const;
			size_t GetMethodsCount() const;
			Function GetMethodByIndex(size_t Index, bool GetVirtual = true) const;
			Function GetMethodByName(const char* Name, bool GetVirtual = true) const;
			Function GetMethodByDecl(const char* Decl, bool GetVirtual = true) const;
			size_t GetPropertiesCount() const;
			ExpectedReturn<void> GetProperty(size_t Index, FunctionInfo* Out) const;
			const char* GetPropertyDeclaration(size_t Index, bool IncludeNamespace = false) const;
			size_t GetBehaviourCount() const;
			Function GetBehaviourByIndex(size_t Index, Behaviours* OutBehaviour) const;
			size_t GetChildFunctionDefCount() const;
			TypeInfo GetChildFunctionDef(size_t Index) const;
			TypeInfo GetParentType() const;
			size_t GetEnumValueCount() const;
			const char* GetEnumValueByIndex(size_t Index, int* OutValue) const;
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
				VI_ASSERT(Object != nullptr, "object should be set");
				return IsHandle() ? *(T**)Object : (T*)Object;
			}
			template <typename T>
			T* GetProperty(void* Object, int Offset)
			{
				VI_ASSERT(Object != nullptr, "object should be set");
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
				VI_ASSERT(Object != nullptr, "object should be set");
				return IsHandle(TypeId) ? *(T**)Object : (T*)Object;
			}
			template <typename T>
			static T* GetProperty(void* Object, int Offset, int TypeId)
			{
				VI_ASSERT(Object != nullptr, "object should be set");
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

		struct VI_OUT Function
		{
		private:
			VirtualMachine* VM;
			asIScriptFunction* Ptr;

		public:
			Function(asIScriptFunction* Base) noexcept;
			Function(const Function& Base) noexcept;
			void AddRef() const;
			void Release();
			int GetId() const;
			FunctionType GetType() const;
			uint32_t* GetByteCode(size_t* Size = nullptr) const;
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
			size_t GetArgsCount() const;
			ExpectedReturn<void> GetArg(size_t Index, int* TypeId, size_t* Flags = nullptr, const char** Name = nullptr, const char** DefaultArg = nullptr) const;
			int GetReturnTypeId(size_t* Flags = nullptr) const;
			int GetTypeId() const;
			bool IsCompatibleWithTypeId(int TypeId) const;
			void* GetDelegateObject() const;
			TypeInfo GetDelegateObjectType() const;
			Function GetDelegateFunction() const;
			size_t GetPropertiesCount() const;
			ExpectedReturn<void> GetProperty(size_t Index, const char** Name, int* TypeId = nullptr) const;
			const char* GetPropertyDecl(size_t Index, bool IncludeNamespace = false) const;
			int FindNextLineWithCode(int Line) const;
			void* SetUserData(void* UserData, size_t Type = 0);
			void* GetUserData(size_t Type = 0) const;
			bool IsValid() const;
			asIScriptFunction* GetFunction() const;
			VirtualMachine* GetVM() const;
		};

		struct VI_OUT ScriptObject
		{
		private:
			asIScriptObject* Object;

		public:
			ScriptObject(asIScriptObject* Base) noexcept;
			void AddRef() const;
			void Release();
			TypeInfo GetObjectType();
			int GetTypeId();
			int GetPropertyTypeId(size_t Id) const;
			size_t GetPropertiesCount() const;
			const char* GetPropertyName(size_t Id) const;
			void* GetAddressOfProperty(size_t Id);
			VirtualMachine* GetVM() const;
			int CopyFrom(const ScriptObject& Other);
			void* SetUserData(void* Data, size_t Type = 0);
			void* GetUserData(size_t Type = 0) const;
			bool IsValid() const;
			asIScriptObject* GetObject() const;
		};

		struct VI_OUT BaseClass
		{
		protected:
			VirtualMachine* VM;
			Core::String Object;
			int TypeId;

		public:
			BaseClass(VirtualMachine* Engine, const Core::String& Name, int Type) noexcept;
			ExpectedReturn<void> SetFunctionDef(const char* Decl);
			ExpectedReturn<void> SetOperatorCopyAddress(asSFuncPtr* Value, FunctionCall = FunctionCall::THISCALL);
			ExpectedReturn<void> SetBehaviourAddress(const char* Decl, Behaviours Behave, asSFuncPtr* Value, FunctionCall = FunctionCall::THISCALL);
			ExpectedReturn<void> SetPropertyAddress(const char* Decl, int Offset);
			ExpectedReturn<void> SetPropertyStaticAddress(const char* Decl, void* Value);
			ExpectedReturn<void> SetOperatorAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type = FunctionCall::THISCALL);
			ExpectedReturn<void> SetMethodAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type = FunctionCall::THISCALL);
			ExpectedReturn<void> SetMethodStaticAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type = FunctionCall::CDECLF);
			ExpectedReturn<void> SetConstructorAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type = FunctionCall::CDECL_OBJFIRST);
			ExpectedReturn<void> SetConstructorListAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type = FunctionCall::CDECL_OBJFIRST);
			ExpectedReturn<void> SetDestructorAddress(const char* Decl, asSFuncPtr* Value);
			int GetTypeId() const;
			bool IsValid() const;
			Core::String GetName() const;
			VirtualMachine* GetVM() const;

		private:
			static Core::String GetOperator(Operators Op, const char* Out, const char* Args, bool Const, bool Right);

		public:
			template <typename T>
			ExpectedReturn<void> SetEnumRefs(void(T::* Value)(asIScriptEngine*))
			{
				asSFuncPtr* EnumRefs = Bridge::Method<T>(Value);
				auto Result = SetBehaviourAddress("void f(int &in)", Behaviours::ENUMREFS, EnumRefs, FunctionCall::THISCALL);
				FunctionFactory::ReleaseFunctor(&EnumRefs);
				return Result;
			}
			template <typename T>
			ExpectedReturn<void> SetReleaseRefs(void(T::* Value)(asIScriptEngine*))
			{
				asSFuncPtr* ReleaseRefs = Bridge::Method<T>(Value);
				auto Result = SetBehaviourAddress("void f(int &in)", Behaviours::RELEASEREFS, ReleaseRefs, FunctionCall::THISCALL);
				FunctionFactory::ReleaseFunctor(&ReleaseRefs);
				return Result;
			}
			template <typename T>
			ExpectedReturn<void> SetEnumRefsEx(void(*Value)(T*, asIScriptEngine*))
			{
				asSFuncPtr* EnumRefs = Bridge::Function<void(*)(T*, asIScriptEngine*)>(Value);
				auto Result = SetBehaviourAddress("void f(int &in)", Behaviours::ENUMREFS, EnumRefs, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&EnumRefs);
				return Result;
			}
			template <typename T>
			ExpectedReturn<void> SetReleaseRefsEx(void(*Value)(T*, asIScriptEngine*))
			{
				asSFuncPtr* ReleaseRefs = Bridge::Function<void(*)(T*, asIScriptEngine*)>(Value);
				auto Result = SetBehaviourAddress("void f(int &in)", Behaviours::RELEASEREFS, ReleaseRefs, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&ReleaseRefs);
				return Result;
			}
			template <typename T, typename R>
			ExpectedReturn<void> SetProperty(const char* Decl, R T::* Value)
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				return SetPropertyAddress(Decl, (int)reinterpret_cast<size_t>(&(((T*)0)->*Value)));
			}
			template <typename T, typename R>
			ExpectedReturn<void> SetPropertyArray(const char* Decl, R T::* Value, size_t ElementsCount)
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				for (size_t i = 0; i < ElementsCount; i++)
				{
					Core::String ElementDecl = Decl + Core::ToString(i);
					auto Result = SetPropertyAddress(ElementDecl.c_str(), (int)reinterpret_cast<size_t>(&(((T*)0)->*Value)) + (int)(sizeof(R) * i));
					if (!Result)
						return Result;
				}
				return Core::Optional::OK;
			}
			template <typename T>
			ExpectedReturn<void> SetPropertyStatic(const char* Decl, T* Value)
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				return SetPropertyStaticAddress(Decl, (void*)Value);
			}
			template <typename T, typename R>
			ExpectedReturn<void> SetGetter(const char* Type, const char* Name, R(T::* Value)())
			{
				VI_ASSERT(Type != nullptr, "type should be set");
				VI_ASSERT(Name != nullptr, "name should be set");
				asSFuncPtr* Ptr = Bridge::Method<T, R>(Value);
				auto Result = SetMethodAddress(Core::Stringify::Text("%s get_%s()", Type, Name).c_str(), Ptr, FunctionCall::THISCALL);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename T, typename R>
			ExpectedReturn<void> SetGetterEx(const char* Type, const char* Name, R(*Value)(T*))
			{
				VI_ASSERT(Type != nullptr, "type should be set");
				VI_ASSERT(Name != nullptr, "name should be set");
				asSFuncPtr* Ptr = Bridge::Function(Value);
				auto Result = SetMethodAddress(Core::Stringify::Text("%s get_%s()", Type, Name).c_str(), Ptr, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename T, typename R>
			ExpectedReturn<void> SetSetter(const char* Type, const char* Name, void(T::* Value)(R))
			{
				VI_ASSERT(Type != nullptr, "type should be set");
				VI_ASSERT(Name != nullptr, "name should be set");
				asSFuncPtr* Ptr = Bridge::Method<T, void, R>(Value);
				auto Result = SetMethodAddress(Core::Stringify::Text("void set_%s(%s)", Name, Type).c_str(), Ptr, FunctionCall::THISCALL);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename T, typename R>
			ExpectedReturn<void> SetSetterEx(const char* Type, const char* Name, void(*Value)(T*, R))
			{
				VI_ASSERT(Type != nullptr, "type should be set");
				VI_ASSERT(Name != nullptr, "name should be set");
				asSFuncPtr* Ptr = Bridge::Function(Value);
				auto Result = SetMethodAddress(Core::Stringify::Text("void set_%s(%s)", Name, Type).c_str(), Ptr, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename T, typename R>
			ExpectedReturn<void> SetArrayGetter(const char* Type, const char* Name, R(T::* Value)(unsigned int))
			{
				VI_ASSERT(Type != nullptr, "type should be set");
				VI_ASSERT(Name != nullptr, "name should be set");
				asSFuncPtr* Ptr = Bridge::Method<T, R, unsigned int>(Value);
				auto Result = SetMethodAddress(Core::Stringify::Text("%s get_%s(uint)", Type, Name).c_str(), Ptr, FunctionCall::THISCALL);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename T, typename R>
			ExpectedReturn<void> SetArrayGetterEx(const char* Type, const char* Name, R(*Value)(T*, unsigned int))
			{
				VI_ASSERT(Type != nullptr, "type should be set");
				VI_ASSERT(Name != nullptr, "name should be set");
				asSFuncPtr* Ptr = Bridge::Function(Value);
				auto Result = SetMethodAddress(Core::Stringify::Text("%s get_%s(uint)", Type, Name).c_str(), Ptr, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename T, typename R>
			ExpectedReturn<void> SetArraySetter(const char* Type, const char* Name, void(T::* Value)(unsigned int, R))
			{
				VI_ASSERT(Type != nullptr, "type should be set");
				VI_ASSERT(Name != nullptr, "name should be set");
				asSFuncPtr* Ptr = Bridge::Method<T, void, unsigned int, R>(Value);
				auto Result = SetMethodAddress(Core::Stringify::Text("void set_%s(uint, %s)", Name, Type).c_str(), Ptr, FunctionCall::THISCALL);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename T, typename R>
			ExpectedReturn<void> SetArraySetterEx(const char* Type, const char* Name, void(*Value)(T*, unsigned int, R))
			{
				VI_ASSERT(Type != nullptr, "type should be set");
				VI_ASSERT(Name != nullptr, "name should be set");
				asSFuncPtr* Ptr = Bridge::Function(Value);
				auto Result = SetMethodAddress(Core::Stringify::Text("void set_%s(uint, %s)", Name, Type).c_str(), Ptr, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename T, typename R, typename... A>
			ExpectedReturn<void> SetOperator(Operators Type, uint32_t Opts, const char* Out, const char* Args, R(T::* Value)(A...))
			{
				VI_ASSERT(Out != nullptr, "output should be set");
				Core::String Operator = GetOperator(Type, Out, Args, Opts & (uint32_t)Position::Const, Opts & (uint32_t)Position::Right);
				VI_ASSERT(!Operator.empty(), "resulting operator should not be empty");
				asSFuncPtr* Ptr = Bridge::Method<T, R, A...>(Value);
				auto Result = SetOperatorAddress(Operator.c_str(), Ptr, FunctionCall::THISCALL);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename R, typename... A>
			ExpectedReturn<void> SetOperatorEx(Operators Type, uint32_t Opts, const char* Out, const char* Args, R(*Value)(A...))
			{
				VI_ASSERT(Out != nullptr, "output should be set");
				Core::String Operator = GetOperator(Type, Out, Args, Opts & (uint32_t)Position::Const, Opts & (uint32_t)Position::Right);
				VI_ASSERT(!Operator.empty(), "resulting operator should not be empty");
				asSFuncPtr* Ptr = Bridge::Function(Value);
				auto Result = SetOperatorAddress(Operator.c_str(), Ptr, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename T>
			ExpectedReturn<void> SetOperatorCopy()
			{
				asSFuncPtr* Ptr = Bridge::MethodOp<T, T&, const T&>(&T::operator =);
				auto Result = SetOperatorCopyAddress(Ptr, FunctionCall::THISCALL);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename R, typename... Args>
			ExpectedReturn<void> SetOperatorCopyStatic(R(*Value)(Args...), FunctionCall Type = FunctionCall::CDECLF)
			{
				asSFuncPtr* Ptr = (Type == FunctionCall::GENERIC ? Bridge::FunctionGeneric<R(*)(Args...)>(Value) : Bridge::Function<R(*)(Args...)>(Value));
				auto Result = SetOperatorCopyAddress(Ptr, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename T, typename R, typename... Args>
			ExpectedReturn<void> SetMethod(const char* Decl, R(T::* Value)(Args...))
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				asSFuncPtr* Ptr = Bridge::Method<T, R, Args...>(Value);
				auto Result = SetMethodAddress(Decl, Ptr, FunctionCall::THISCALL);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename T, typename R, typename... Args>
			ExpectedReturn<void> SetMethod(const char* Decl, R(T::* Value)(Args...) const)
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				asSFuncPtr* Ptr = Bridge::Method<T, R, Args...>(Value);
				auto Result = SetMethodAddress(Decl, Ptr, FunctionCall::THISCALL);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename R, typename... Args>
			ExpectedReturn<void> SetMethodEx(const char* Decl, R(*Value)(Args...))
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				asSFuncPtr* Ptr = Bridge::Function<R(*)(Args...)>(Value);
				auto Result = SetMethodAddress(Decl, Ptr, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename R, typename... Args>
			ExpectedReturn<void> SetMethodStatic(const char* Decl, R(*Value)(Args...), FunctionCall Type = FunctionCall::CDECLF)
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				asSFuncPtr* Ptr = (Type == FunctionCall::GENERIC ? Bridge::FunctionGeneric<R(*)(Args...)>(Value) : Bridge::Function<R(*)(Args...)>(Value));
				auto Result = SetMethodStaticAddress(Decl, Ptr, Type);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename T, typename To>
			ExpectedReturn<void> SetDynamicCast(const char* ToDecl, bool Implicit = false)
			{
				auto Type = Implicit ? Operators::ImplCast : Operators::Cast;
				auto Result1 = SetOperatorEx(Type, 0, ToDecl, "", &BaseClass::DynamicCastFunction<T, To>);
				if (!Result1)
					return Result1;

				auto Result2 = SetOperatorEx(Type, (uint32_t)Position::Const, ToDecl, "", &BaseClass::DynamicCastFunction<T, To>);
				return Result2;
			}

		private:
			template <typename T, typename To>
			static To* DynamicCastFunction(T* Base)
			{
				return dynamic_cast<To*>(Base);
			}
		};

		struct VI_OUT RefClass : public BaseClass
		{
		public:
			RefClass(VirtualMachine* Engine, const Core::String& Name, int Type) noexcept : BaseClass(Engine, Name, Type)
			{
			}

		public:
			template <typename T, typename... Args>
			ExpectedReturn<void> SetConstructor(const char* Decl)
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				asSFuncPtr* Functor = Bridge::Function(&Bridge::GetUnmanagedCall<T, Args...>);
				auto Result = SetBehaviourAddress(Decl, Behaviours::FACTORY, Functor, FunctionCall::CDECLF);
				FunctionFactory::ReleaseFunctor(&Functor);
				return Result;
			}
			template <typename T, asIScriptGeneric*>
			ExpectedReturn<void> SetConstructor(const char* Decl)
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				asSFuncPtr* Functor = Bridge::FunctionGeneric(&Bridge::GetUnmanagedCall<T, asIScriptGeneric*>);
				auto Result = SetBehaviourAddress(Decl, Behaviours::FACTORY, Functor, FunctionCall::GENERIC);
				FunctionFactory::ReleaseFunctor(&Functor);
				return Result;
			}
			template <typename T>
			ExpectedReturn<void> SetConstructorList(const char* Decl)
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				asSFuncPtr* Functor = Bridge::FunctionGeneric(&Bridge::GetUnmanagedListCall<T>);
				auto Result = SetBehaviourAddress(Decl, Behaviours::LIST_FACTORY, Functor, FunctionCall::GENERIC);
				FunctionFactory::ReleaseFunctor(&Functor);
				return Result;
			}
			template <typename T>
			ExpectedReturn<void> SetConstructorListEx(const char* Decl, void(*Value)(asIScriptGeneric*))
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				asSFuncPtr* Functor = Bridge::FunctionGeneric(Value);
				auto Result = SetBehaviourAddress(Decl, Behaviours::LIST_FACTORY, Functor, FunctionCall::GENERIC);
				FunctionFactory::ReleaseFunctor(&Functor);
				return Result;
			}
			template <typename T, uint64_t TypeName, typename... Args>
			ExpectedReturn<void> SetGcConstructor(const char* Decl)
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				asSFuncPtr* Functor = Bridge::Function(&Bridge::GetManagedCall<T, TypeName, Args...>);
				auto Result = SetBehaviourAddress(Decl, Behaviours::FACTORY, Functor, FunctionCall::CDECLF);
				FunctionFactory::ReleaseFunctor(&Functor);
				return Result;
			}
			template <typename T, uint64_t TypeName, asIScriptGeneric*>
			ExpectedReturn<void> SetGcConstructor(const char* Decl)
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				asSFuncPtr* Functor = Bridge::FunctionGeneric(&Bridge::GetManagedCall<T, TypeName, asIScriptGeneric*>);
				auto Result = SetBehaviourAddress(Decl, Behaviours::FACTORY, Functor, FunctionCall::GENERIC);
				FunctionFactory::ReleaseFunctor(&Functor);
				return Result;
			}
			template <typename T, uint64_t TypeName>
			ExpectedReturn<void> SetGcConstructorList(const char* Decl)
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				asSFuncPtr* Functor = Bridge::FunctionGeneric(&Bridge::GetManagedListCall<T, TypeName>);
				auto Result = SetBehaviourAddress(Decl, Behaviours::LIST_FACTORY, Functor, FunctionCall::GENERIC);
				FunctionFactory::ReleaseFunctor(&Functor);
				return Result;
			}
			template <typename T>
			ExpectedReturn<void> SetGcConstructorListEx(const char* Decl, void(*Value)(asIScriptGeneric*))
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				asSFuncPtr* Functor = Bridge::FunctionGeneric(Value);
				auto Result = SetBehaviourAddress(Decl, Behaviours::LIST_FACTORY, Functor, FunctionCall::GENERIC);
				FunctionFactory::ReleaseFunctor(&Functor);
				return Result;
			}
			template <typename F>
			ExpectedReturn<void> SetAddRef()
			{
				auto FactoryPtr = &RefClass::GcAddRef<F>;
				asSFuncPtr* AddRef = Bridge::Function<decltype(FactoryPtr)>(FactoryPtr);
				auto Result = SetBehaviourAddress("void f()", Behaviours::ADDREF, AddRef, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&AddRef);
				return Result;
			}
			template <typename F>
			ExpectedReturn<void> SetRelease()
			{
				auto FactoryPtr = &RefClass::GcRelease<F>;
				asSFuncPtr* Release = Bridge::Function<decltype(FactoryPtr)>(FactoryPtr);
				auto Result = SetBehaviourAddress("void f()", Behaviours::RELEASE, Release, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Release);
				return Result;
			}
			template <typename F>
			ExpectedReturn<void> SetMarkRef()
			{
				auto FactoryPtr = &RefClass::GcMarkRef<F>;
				asSFuncPtr* Release = Bridge::Function<decltype(FactoryPtr)>(FactoryPtr);
				auto Result = SetBehaviourAddress("void f()", Behaviours::SETGCFLAG, Release, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Release);
				return Result;
			}
			template <typename F>
			ExpectedReturn<void> SetIsMarkedRef()
			{
				auto FactoryPtr = &RefClass::GcIsMarkedRef<F>;
				asSFuncPtr* Release = Bridge::Function<decltype(FactoryPtr)>(FactoryPtr);
				auto Result = SetBehaviourAddress("bool f()", Behaviours::GETGCFLAG, Release, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Release);
				return Result;
			}
			template <typename F>
			ExpectedReturn<void> SetRefCount()
			{
				auto FactoryPtr = &RefClass::GcGetRefCount<F>;
				asSFuncPtr* Release = Bridge::Function<decltype(FactoryPtr)>(FactoryPtr);
				auto Result = SetBehaviourAddress("int f()", Behaviours::GETREFCOUNT, Release, FunctionCall::CDECL_OBJFIRST);
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

		struct VI_OUT TypeClass : public BaseClass
		{
		public:
			TypeClass(VirtualMachine* Engine, const Core::String& Name, int Type) noexcept : BaseClass(Engine, Name, Type)
			{
			}

		public:
			template <typename T, typename... Args>
			ExpectedReturn<void> SetConstructor(const char* Decl)
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				asSFuncPtr* Ptr = Bridge::Function(&Bridge::GetConstructorCall<T, Args...>);
				auto Result = SetConstructorAddress(Decl, Ptr, FunctionCall::CDECL_OBJFIRST);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename T, asIScriptGeneric*>
			ExpectedReturn<void> SetConstructor(const char* Decl)
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				asSFuncPtr* Ptr = Bridge::FunctionGeneric(&Bridge::GetConstructorCall<T, asIScriptGeneric*>);
				auto Result = SetConstructorAddress(Decl, Ptr, FunctionCall::GENERIC);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename T>
			ExpectedReturn<void> SetConstructorList(const char* Decl)
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				asSFuncPtr* Ptr = Bridge::FunctionGeneric(&Bridge::GetConstructorListCall<T>);
				auto Result = SetConstructorListAddress(Decl, Ptr, FunctionCall::GENERIC);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename T>
			ExpectedReturn<void> SetDestructor(const char* Decl)
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				asSFuncPtr* Ptr = Bridge::Function(&Bridge::GetDestructorCall<T>);
				auto Result = SetDestructorAddress(Decl, Ptr);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename R, typename... Args>
			ExpectedReturn<void> SetDestructorStatic(const char* Decl, R(*Value)(Args...))
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				asSFuncPtr* Ptr = Bridge::Function<R(*)(Args...)>(Value);
				auto Result = SetDestructorAddress(Decl, Ptr);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
		};

		struct VI_OUT TypeInterface
		{
		private:
			VirtualMachine* VM;
			Core::String Object;
			int TypeId;

		public:
			TypeInterface(VirtualMachine* Engine, const Core::String& Name, int Type) noexcept;
			ExpectedReturn<void> SetMethod(const char* Decl);
			int GetTypeId() const;
			bool IsValid() const;
			Core::String GetName() const;
			VirtualMachine* GetVM() const;
		};

		struct VI_OUT Enumeration
		{
		private:
			VirtualMachine* VM;
			Core::String Object;
			int TypeId;

		public:
			Enumeration(VirtualMachine* Engine, const Core::String& Name, int Type) noexcept;
			ExpectedReturn<void> SetValue(const char* Name, int Value);
			int GetTypeId() const;
			bool IsValid() const;
			Core::String GetName() const;
			VirtualMachine* GetVM() const;
		};

		struct VI_OUT Module
		{
		private:
			VirtualMachine* VM;
			asIScriptModule* Mod;

		public:
			Module(asIScriptModule* Type) noexcept;
			void SetName(const char* Name);
			ExpectedReturn<void> AddSection(const char* Name, const char* Code, size_t CodeLength = 0, int LineOffset = 0);
			ExpectedReturn<void> RemoveFunction(const Function& Function);
			ExpectedReturn<void> ResetProperties(asIScriptContext* Context = nullptr);
			ExpectedReturn<void> Build();
			ExpectedReturn<void> LoadByteCode(ByteCodeInfo* Info);
			ExpectedReturn<void> BindImportedFunction(size_t ImportIndex, const Function& Function);
			ExpectedReturn<void> UnbindImportedFunction(size_t ImportIndex);
			ExpectedReturn<void> BindAllImportedFunctions();
			ExpectedReturn<void> UnbindAllImportedFunctions();
			ExpectedReturn<void> CompileFunction(const char* SectionName, const char* Code, int LineOffset, size_t CompileFlags, Function* OutFunction);
			ExpectedReturn<void> CompileProperty(const char* SectionName, const char* Code, int LineOffset);
			ExpectedReturn<void> SetDefaultNamespace(const char* Namespace);
			ExpectedReturn<void> RemoveProperty(size_t Index);
			void Discard();
			void* GetAddressOfProperty(size_t Index);
			size_t SetAccessMask(size_t AccessMask);
			size_t GetFunctionCount() const;
			Function GetFunctionByIndex(size_t Index) const;
			Function GetFunctionByDecl(const char* Decl) const;
			Function GetFunctionByName(const char* Name) const;
			int GetTypeIdByDecl(const char* Decl) const;
			ExpectedReturn<size_t> GetImportedFunctionIndexByDecl(const char* Decl) const;
			ExpectedReturn<void> SaveByteCode(ByteCodeInfo* Info) const;
			ExpectedReturn<size_t> GetPropertyIndexByName(const char* Name) const;
			ExpectedReturn<size_t> GetPropertyIndexByDecl(const char* Decl) const;
			ExpectedReturn<void> GetProperty(size_t Index, PropertyInfo* Out) const;
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
			ExpectedReturn<void> SetTypeProperty(const char* Name, T* Value)
			{
				VI_ASSERT(Name != nullptr, "name should be set");
				auto Index = GetPropertyIndexByName(Name);
				if (!Index)
					return Index.Error();

				T** Address = (T**)GetAddressOfProperty(*Index);
				if (!Address)
					return Errors::INVALID_OBJECT;

				*Address = Value;
				return Core::Optional::OK;
			}
			template <typename T>
			ExpectedReturn<void> SetTypeProperty(const char* Name, const T& Value)
			{
				VI_ASSERT(Name != nullptr, "name should be set");
				auto Index = GetPropertyIndexByName(Name);
				if (!Index)
					return Index.Error();

				T* Address = (T*)GetAddressOfProperty(*Index);
				if (!Address)
					return Errors::INVALID_OBJECT;

				*Address = Value;
				return Core::Optional::OK;
			}
			template <typename T>
			ExpectedReturn<void> SetRefProperty(const char* Name, T* Value)
			{
				VI_ASSERT(Name != nullptr, "name should be set");
				auto Index = GetPropertyIndexByName(Name);
				if (!Index)
					return Index.Error();

				T** Address = (T**)GetAddressOfProperty(*Index);
				if (!Address)
					return Errors::INVALID_OBJECT;

				VI_RELEASE(*Address);
				*Address = (T*)Value;
				if (*Address != nullptr)
					(*Address)->AddRef();
				return Core::Optional::OK;
			}
		};

		struct VI_OUT FunctionDelegate
		{
			ImmediateContext* Context;
			asIScriptFunction* Callback;
			asITypeInfo* DelegateType;
			void* DelegateObject;

			FunctionDelegate();
			FunctionDelegate(const Function& Function);
			FunctionDelegate(const Function& Function, ImmediateContext* WantedContext);
			FunctionDelegate(const FunctionDelegate& Other);
			FunctionDelegate(FunctionDelegate&& Other);
			~FunctionDelegate();
			FunctionDelegate& operator= (const FunctionDelegate& Other);
			FunctionDelegate& operator= (FunctionDelegate&& Other);
			ExpectedFuture<Activation> operator()(ArgsCallback&& OnArgs, ArgsCallback&& OnReturn = nullptr);
			Function Callable() const;
			bool IsValid() const;
			void AddRef();
			void Release();

		private:
			ExpectedFuture<Activation> ExecuteOnNewContext(ArgsCallback&& OnArgs, ArgsCallback&& OnReturn);
			void AddRefAndInitialize(bool IsFirstTime);
		};

		class VI_OUT Compiler final : public Core::Reference<Compiler>
		{
		private:
			static int CompilerUD;

		private:
			Compute::ProcIncludeCallback Include;
			Compute::ProcPragmaCallback Pragma;
			Compute::Preprocessor* Processor;
			asIScriptModule* Scope;
			VirtualMachine* VM;
			ByteCodeInfo VCache;
			size_t Counter;
			bool Built;

		public:
			Compiler(VirtualMachine* Engine) noexcept;
			~Compiler() noexcept;
			void SetIncludeCallback(const Compute::ProcIncludeCallback& Callback);
			void SetPragmaCallback(const Compute::ProcPragmaCallback& Callback);
			void Define(const Core::String& Word);
			void Undefine(const Core::String& Word);
			Module UnlinkModule();
			bool Clear();
			bool IsDefined(const Core::String& Word) const;
			bool IsBuilt() const;
			bool IsCached() const;
			ExpectedReturn<void> Prepare(ByteCodeInfo* Info);
			ExpectedReturn<void> Prepare(const Core::String& ModuleName, bool Scoped = false);
			ExpectedReturn<void> Prepare(const Core::String& ModuleName, const Core::String& Cache, bool Debug = true, bool Scoped = false);
			ExpectedReturn<void> SaveByteCode(ByteCodeInfo* Info);
			ExpectedReturn<void> LoadFile(const Core::String& Path);
			ExpectedReturn<void> LoadCode(const Core::String& Name, const Core::String& Buffer);
			ExpectedReturn<void> LoadCode(const Core::String& Name, const char* Buffer, size_t Length);
			ExpectedFuture<void> LoadByteCode(ByteCodeInfo* Info);
			ExpectedFuture<void> Compile();
			ExpectedFuture<void> CompileFile(const char* Name, const char* ModuleName, const char* EntryName);
			ExpectedFuture<void> CompileMemory(const Core::String& Buffer, const char* ModuleName, const char* EntryName);
			ExpectedFuture<Function> CompileFunction(const Core::String& Code, const char* Returns = nullptr, const char* Args = nullptr);
			Module GetModule() const;
			VirtualMachine* GetVM() const;
			Compute::Preprocessor* GetProcessor() const;

		private:
			static Compiler* Get(ImmediateContext* Context);
		};

		class VI_OUT DebuggerContext final : public Core::Reference<DebuggerContext>
		{
		public:
			typedef std::function<Core::String(Core::String& Indent, int Depth, void* Object)> ToStringCallback;
			typedef std::function<Core::String(Core::String& Indent, int Depth, void* Object, int TypeId)> ToStringTypeCallback;
			typedef std::function<bool(ImmediateContext*, const Core::Vector<Core::String>&)> CommandCallback;
			typedef std::function<void(const Core::String&)> OutputCallback;
			typedef std::function<bool(Core::String&)> InputCallback;
			typedef std::function<void()> ExitCallback;

		protected:
			enum class DebugAction
			{
				Trigger,
				Interrupt,
				Switch,
				Continue,
				StepInto,
				StepOver,
				StepOut
			};

			enum class ArgsType
			{
				NoArgs,
				Expression,
				Array
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

			struct CommandData
			{
				CommandCallback Callback;
				Core::String Description;
				ArgsType Arguments;
			};

			struct ThreadData
			{
				ImmediateContext* Context;
				std::thread::id Id;
			};

		protected:
			Core::UnorderedMap<const asITypeInfo*, ToStringCallback> FastToStringCallbacks;
			Core::UnorderedMap<Core::String, ToStringTypeCallback> SlowToStringCallbacks;
			Core::UnorderedMap<Core::String, CommandData> Commands;
			Core::UnorderedMap<Core::String, Core::String> Descriptions;
			Core::Vector<ThreadData> Threads;
			Core::Vector<BreakPoint> BreakPoints;
			std::recursive_mutex ThreadBarrier;
			std::recursive_mutex Mutex;
			ImmediateContext* LastContext;
			unsigned int LastCommandAtStackLevel;
			std::atomic<int32_t> ForceSwitchThreads;
			asIScriptFunction* LastFunction;
			VirtualMachine* VM;
			OutputCallback OnOutput;
			InputCallback OnInput;
			ExitCallback OnExit;
			DebugAction Action;
			bool InputError;
			bool Attachable;

		public:
			DebuggerContext(DebugType Type = DebugType::Suspended) noexcept;
			~DebuggerContext() noexcept;
			void SetOutputCallback(OutputCallback&& Callback);
			void SetInputCallback(InputCallback&& Callback);
			void SetExitCallback(ExitCallback&& Callback);
			void AddToStringCallback(const TypeInfo& Type, ToStringCallback&& Callback);
			void AddToStringCallback(const Core::String& Type, const ToStringTypeCallback& Callback);
			void AllowInputAfterFailure();
			void Input(ImmediateContext* Context);
			void Output(const Core::String& Data);
			void LineCallback(asIScriptContext* Context);
			void ExceptionCallback(asIScriptContext* Context);
			void AddFileBreakPoint(const Core::String& File, int LineNumber);
			void AddFuncBreakPoint(const Core::String& Function);
			void ShowException(ImmediateContext* Context);
			void ListBreakPoints();
			void ListThreads();
			void ListAddons();
			void ListStackRegisters(ImmediateContext* Context, uint32_t Level);
			void ListLocalVariables(ImmediateContext* Context);
			void ListGlobalVariables(ImmediateContext* Context);
			void ListMemberProperties(ImmediateContext* Context);
			void ListSourceCode(ImmediateContext* Context);
			void ListInterfaces(ImmediateContext* Context);
			void ListStatistics(ImmediateContext* Context);
			void PrintCallstack(ImmediateContext* Context);
			void PrintValue(const Core::String& Expression, ImmediateContext* Context);
			void PrintByteCode(const Core::String& FunctionDecl, ImmediateContext* Context);
			void SetEngine(VirtualMachine* Engine);
			bool InterpretCommand(const Core::String& Command, ImmediateContext* Context);
			bool CheckBreakPoint(ImmediateContext* Context);
			bool Interrupt();
			bool IsInputIgnored();
			bool IsAttached();
			Core::String ToString(int MaxDepth, void* Value, unsigned int TypeId);
			Core::String ToString(Core::String& Indent, int MaxDepth, void* Value, unsigned int TypeId);
			VirtualMachine* GetEngine();

		private:
			void AddCommand(const Core::String& Name, const Core::String& Description, ArgsType Type, const CommandCallback& Callback);
			void AddDefaultCommands();
			void AddDefaultStringifiers();
			void ClearThread(ImmediateContext* Context);
			ExpectedReturn<void> ExecuteExpression(ImmediateContext* Context, const Core::String& Code);
			ThreadData GetThread(ImmediateContext* Context);
		};

		class VI_OUT ImmediateContext final : public Core::Reference<ImmediateContext>
		{
			friend FunctionDelegate;
			friend VirtualMachine;

		private:
			static int ContextUD;

		private:
			struct Events
			{
				std::function<void(ImmediateContext*)> Exception;
				std::function<void(ImmediateContext*)> Line;
			} Callbacks;

			struct Frame
			{
				ExpectedFuture<Activation> Future = ExpectedFuture<Activation>::Null();
				Core::String Stacktrace;
				size_t DenySuspends = 0;
			} Executor;

		private:
			asIScriptContext* Context;
			VirtualMachine* VM;
			std::recursive_mutex Exchange;

		public:
			~ImmediateContext() noexcept;
			ExpectedFuture<Activation> ExecuteCall(const Function& Function, ArgsCallback&& OnArgs);
			ExpectedReturn<Activation> ExecuteCallSync(const Function& Function, ArgsCallback&& OnArgs);
			ExpectedReturn<Activation> ExecuteSubcall(const Function& Function, ArgsCallback&& OnArgs);
			ExpectedReturn<Activation> ExecuteNext();
			ExpectedReturn<Activation> Resume();
			ExpectedReturn<void> Prepare(const Function& Function);
			ExpectedReturn<void> Unprepare();
			ExpectedReturn<void> Abort();
			ExpectedReturn<void> Suspend();
			ExpectedReturn<void> PushState();
			ExpectedReturn<void> PopState();
			ExpectedReturn<void> SetObject(void* Object);
			ExpectedReturn<void> SetArg8(size_t Arg, unsigned char Value);
			ExpectedReturn<void> SetArg16(size_t Arg, unsigned short Value);
			ExpectedReturn<void> SetArg32(size_t Arg, int Value);
			ExpectedReturn<void> SetArg64(size_t Arg, int64_t Value);
			ExpectedReturn<void> SetArgFloat(size_t Arg, float Value);
			ExpectedReturn<void> SetArgDouble(size_t Arg, double Value);
			ExpectedReturn<void> SetArgAddress(size_t Arg, void* Address);
			ExpectedReturn<void> SetArgObject(size_t Arg, void* Object);
			ExpectedReturn<void> SetArgAny(size_t Arg, void* Ptr, int TypeId);
			ExpectedReturn<void> GetReturnableByType(void* Return, asITypeInfo* ReturnTypeId);
			ExpectedReturn<void> GetReturnableByDecl(void* Return, const char* ReturnTypeDecl);
			ExpectedReturn<void> GetReturnableById(void* Return, int ReturnTypeId);
			ExpectedReturn<void> SetException(const char* Info, bool AllowCatch = true);
			ExpectedReturn<void> SetExceptionCallback(void(*Callback)(asIScriptContext* Context, void* Object), void* Object);
			ExpectedReturn<void> SetLineCallback(void(*Callback)(asIScriptContext* Context, void* Object), void* Object);
			ExpectedReturn<void> StartDeserialization();
			ExpectedReturn<void> FinishDeserialization();
			ExpectedReturn<void> PushFunction(const Function& Func, void* Object);
			ExpectedReturn<void> GetStateRegisters(size_t StackLevel, Function* CallingSystemFunction, Function* InitialFunction, uint32_t* OrigStackPointer, uint32_t* ArgumentsSize, uint64_t* ValueRegister, void** ObjectRegister, TypeInfo* ObjectTypeRegister);
			ExpectedReturn<void> GetCallStateRegisters(size_t StackLevel, uint32_t* StackFramePointer, Function* CurrentFunction, uint32_t* ProgramPointer, uint32_t* StackPointer, uint32_t* StackIndex);
			ExpectedReturn<void> SetStateRegisters(size_t StackLevel, Function CallingSystemFunction, const Function& InitialFunction, uint32_t OrigStackPointer, uint32_t ArgumentsSize, uint64_t ValueRegister, void* ObjectRegister, const TypeInfo& ObjectTypeRegister);
			ExpectedReturn<void> SetCallStateRegisters(size_t StackLevel, uint32_t StackFramePointer, const Function& CurrentFunction, uint32_t ProgramPointer, uint32_t StackPointer, uint32_t StackIndex);
			ExpectedReturn<size_t> GetArgsOnStackCount(size_t StackLevel);
			ExpectedReturn<void> GetArgOnStack(size_t StackLevel, size_t Argument, int* TypeId, size_t* Flags, void** Address);
			ExpectedReturn<size_t> GetPropertiesCount(size_t StackLevel = 0);
			ExpectedReturn<void> GetProperty(size_t Index, size_t StackLevel, const char** Name, int* TypeId = 0, Modifiers* TypeModifiers = 0, bool* IsVarOnHeap = 0, int* StackOffset = 0);
			Function GetFunction(size_t StackLevel = 0);
			void SetLineCallback(const std::function<void(ImmediateContext*)>& Callback);
			void SetExceptionCallback(const std::function<void(ImmediateContext*)>& Callback);
			void DisableSuspends();
			void EnableSuspends();
			bool IsNested(size_t* NestCount = 0) const;
			bool IsThrown() const;
			bool IsPending();
			Activation GetState() const;
			void* GetAddressOfArg(size_t Arg);
			unsigned char GetReturnByte();
			unsigned short GetReturnWord();
			size_t GetReturnDWord();
			uint64_t GetReturnQWord();
			float GetReturnFloat();
			double GetReturnDouble();
			void* GetReturnAddress();
			void* GetReturnObjectAddress();
			void* GetAddressOfReturnValue();
			int GetExceptionLineNumber(int* Column = 0, const char** SectionName = 0);
			int GetLineNumber(size_t StackLevel = 0, int* Column = 0, const char** SectionName = 0);
			Function GetExceptionFunction();
			const char* GetExceptionString();
			bool WillExceptionBeCaught();
			void ClearExceptionCallback();
			void ClearLineCallback();
			size_t GetCallstackSize() const;
			Core::Option<Core::String> GetExceptionStackTrace();
			const char* GetPropertyName(size_t Index, size_t StackLevel = 0);
			const char* GetPropertyDecl(size_t Index, size_t StackLevel = 0, bool IncludeNamespace = false);
			int GetPropertyTypeId(size_t Index, size_t StackLevel = 0);
			void* GetAddressOfProperty(size_t Index, size_t StackLevel = 0, bool DontDereference = false, bool ReturnAddressOfUnitializedObjects = false);
			bool IsPropertyInScope(size_t Index, size_t StackLevel = 0);
			int GetThisTypeId(size_t StackLevel = 0);
			void* GetThisPointer(size_t StackLevel = 0);
			Function GetSystemFunction();
			bool IsSuspended() const;
			bool IsSuspendable() const;
			bool IsSyncLocked() const;
			bool CanExecuteCall() const;
			bool CanExecuteSubcall() const;
			void* SetUserData(void* Data, size_t Type = 0);
			void* GetUserData(size_t Type = 0) const;
			asIScriptContext* GetContext();
			VirtualMachine* GetVM();

		private:
			ImmediateContext(asIScriptContext* Base) noexcept;

		public:
			template <typename T>
			T* GetReturnObject()
			{
				return (T*)GetReturnObjectAddress();
			}

		public:
			static ImmediateContext* Get(asIScriptContext* Context);
			static ImmediateContext* Get();
		};

		class VI_OUT VirtualMachine final : public Core::Reference<VirtualMachine>
		{
		public:
			typedef std::function<bool(const Core::String& Path, Core::String& Buffer)> GeneratorCallback;
			typedef std::function<void(const Core::String&)> CompileCallback;
			typedef std::function<void()> WhenErrorCallback;

		public:
			struct CFunction
			{
				Core::String Declaration;
				void* Handle;
			};

			struct CLibrary
			{
				Core::UnorderedMap<Core::String, CFunction> Functions;
				void* Handle;
				bool IsAddon;
			};

			struct Addon
			{
				Core::Vector<Core::String> Dependencies;
				AddonCallback Callback;
				bool Exposed = false;
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
			Core::UnorderedMap<Core::String, Core::String> Sections;
			Core::UnorderedMap<Core::String, Core::Schema*> Datas;
			Core::UnorderedMap<Core::String, ByteCodeInfo> Opcodes;
			Core::UnorderedMap<Core::String, CLibrary> CLibraries;
			Core::UnorderedMap<Core::String, Addon> Addons;
			Core::UnorderedMap<Core::String, CompileCallback> Callbacks;
			Core::UnorderedMap<Core::String, GeneratorCallback> Generators;
			Core::UnorderedMap<LibraryFeatures, size_t> LibrarySettings;
			Core::Vector<ImmediateContext*> Threads;
			Core::Vector<asIScriptContext*> Stacks;
			Core::String DefaultNamespace;
			Compute::Preprocessor::Desc Proc;
			Compute::IncludeDesc Include;
			std::function<void(ImmediateContext*)> GlobalException;
			WhenErrorCallback WhenError;
			uint64_t Scope;
			DebuggerContext* Debugger;
			asIScriptEngine* Engine;
			asIScriptTranslator* Translator;
			unsigned int Imports;
			bool SaveSources;
			bool Cached;

		public:
			VirtualMachine() noexcept;
			~VirtualMachine() noexcept;
			ExpectedReturn<void> GarbageCollect(GarbageCollector Flags, size_t NumIterations = 1);
			ExpectedReturn<void> PerformFullGarbageCollection();
			ExpectedReturn<void> NotifyOfNewObject(void* Object, const TypeInfo& Type);
			ExpectedReturn<void> GetObjectAddress(size_t Index, size_t* SequenceNumber = nullptr, void** Object = nullptr, TypeInfo* Type = nullptr);
			ExpectedReturn<void> AssignObject(void* DestObject, void* SrcObject, const TypeInfo& Type);
			ExpectedReturn<void> RefCastObject(void* Object, const TypeInfo& FromType, const TypeInfo& ToType, void** NewPtr, bool UseOnlyImplicitCast = false);
			ExpectedReturn<void> BeginGroup(const char* GroupName);
			ExpectedReturn<void> EndGroup();
			ExpectedReturn<void> RemoveGroup(const char* GroupName);
			ExpectedReturn<void> BeginNamespace(const char* Namespace);
			ExpectedReturn<void> BeginNamespaceIsolated(const char* Namespace, size_t DefaultMask);
			ExpectedReturn<void> EndNamespace();
			ExpectedReturn<void> EndNamespaceIsolated();
			ExpectedReturn<void> AddScriptSection(asIScriptModule* Module, const char* Name, const char* Code, size_t CodeLength = 0, int LineOffset = 0);
			ExpectedReturn<void> GetTypeNameScope(const char** TypeName, const char** Namespace, size_t* NamespaceSize) const;
			ExpectedReturn<void> SetFunctionDef(const char* Decl);
			ExpectedReturn<void> SetFunctionAddress(const char* Decl, asSFuncPtr* Value, FunctionCall Type = FunctionCall::CDECLF);
			ExpectedReturn<void> SetPropertyAddress(const char* Decl, void* Value);
			ExpectedReturn<void> SetLogCallback(void(*Callback)(const asSMessageInfo* Message, void* Object), void* Object);
			ExpectedReturn<void> Log(const char* Section, int Row, int Column, LogCategory Type, const char* Message);
			ExpectedReturn<void> SetProperty(Features Property, size_t Value);
			ExpectedReturn<void> GetPropertyByIndex(size_t Index, PropertyInfo* Info) const;
			ExpectedReturn<size_t> GetPropertyIndexByName(const char* Name) const;
			ExpectedReturn<size_t> GetPropertyIndexByDecl(const char* Decl) const;
			ExpectedReturn<size_t> GetSizeOfPrimitiveType(int TypeId) const;
			ExpectedReturn<TypeClass> SetStructAddress(const char* Name, size_t Size, uint64_t Flags = (uint64_t)ObjectBehaviours::VALUE);
			ExpectedReturn<TypeClass> SetPodAddress(const char* Name, size_t Size, uint64_t Flags = (uint64_t)(ObjectBehaviours::VALUE | ObjectBehaviours::POD));
			ExpectedReturn<RefClass> SetClassAddress(const char* Name, uint64_t Flags = (uint64_t)ObjectBehaviours::REF);
			ExpectedReturn<TypeInterface> SetInterface(const char* Name);
			ExpectedReturn<Enumeration> SetEnum(const char* Name);
			bool SetByteCodeTranslator(unsigned int Options);
			void SetCodeGenerator(const Core::String& Name, GeneratorCallback&& Callback);
			void SetPreserveSourceCode(bool Enabled);
			void SetImports(unsigned int Opts);
			void SetCache(bool Enabled);
			void SetExceptionCallback(const std::function<void(ImmediateContext*)>& Callback);
			void SetDebugger(Core::Unique<DebuggerContext> Context);
			void SetCompilerErrorCallback(const WhenErrorCallback& Callback);
			void SetCompilerIncludeOptions(const Compute::IncludeDesc& NewDesc);
			void SetCompilerFeatures(const Compute::Preprocessor::Desc& NewDesc);
			void SetProcessorOptions(Compute::Preprocessor* Processor);
			void SetCompileCallback(const Core::String& Section, CompileCallback&& Callback);
			void ClearCache();
			void ClearSections();
			void AttachDebuggerToContext(asIScriptContext* Context);
			void DetachDebuggerFromContext(asIScriptContext* Context);
			void GetStatistics(unsigned int* CurrentSize, unsigned int* TotalDestroyed, unsigned int* TotalDetected, unsigned int* NewObjects, unsigned int* TotalNewDestroyed) const;
			void ForwardEnumReferences(void* Reference, const TypeInfo& Type);
			void ForwardReleaseReferences(void* Reference, const TypeInfo& Type);
			void GCEnumCallback(void* Reference);
			void GCEnumCallback(asIScriptFunction* Reference);
			void GCEnumCallback(FunctionDelegate* Reference);
			bool TriggerDebugger(uint64_t TimeoutMs = 0);
			bool GenerateCode(Compute::Preprocessor* Processor, const Core::String& Path, Core::String& InoutBuffer);
			Core::UnorderedMap<Core::String, Core::String> DumpRegisteredInterfaces(ImmediateContext* Context);
			Core::Unique<Compiler> CreateCompiler();
			Core::Unique<asIScriptModule> CreateScopedModule(const Core::String& Name);
			Core::Unique<asIScriptModule> CreateModule(const Core::String& Name);
			Core::Unique<ImmediateContext> RequestContext();
			void ReturnContext(ImmediateContext* Context);
			bool GetByteCodeCache(ByteCodeInfo* Info);
			void SetByteCodeCache(ByteCodeInfo* Info);
			void* CreateObject(const TypeInfo& Type);
			void* CreateObjectCopy(void* Object, const TypeInfo& Type);
			void* CreateEmptyObject(const TypeInfo& Type);
			Function CreateDelegate(const Function& Function, void* Object);
			void ReleaseObject(void* Object, const TypeInfo& Type);
			void AddRefObject(void* Object, const TypeInfo& Type);
			size_t BeginAccessMask(size_t DefaultMask);
			size_t EndAccessMask();
			const char* GetNamespace() const;
			Module GetModule(const char* Name);
			void SetLibraryProperty(LibraryFeatures Property, size_t Value);
			size_t GetLibraryProperty(LibraryFeatures Property);
			size_t GetProperty(Features Property);
			void SetModuleDirectory(const Core::String& Root);
			size_t GetProperty(Features Property) const;
			asIScriptEngine* GetEngine() const;
			DebuggerContext* GetDebugger() const;
			const Core::String& GetModuleDirectory() const;
			Core::Vector<Core::String> GetExposedAddons();
			const Core::UnorderedMap<Core::String, Addon>& GetSystemAddons() const;
			const Core::UnorderedMap<Core::String, CLibrary>& GetCLibraries() const;
			const Compute::IncludeDesc& GetCompileIncludeOptions() const;
			bool HasLibrary(const Core::String& Name, bool IsAddon = false);
			bool HasSystemAddon(const Core::String& Name);
			bool HasAddon(const Core::String& Name);
			bool IsNullable(int TypeId);
			bool IsTranslatorSupported();
			bool AddSystemAddon(const Core::String& Name, const Core::Vector<Core::String>& Dependencies, const AddonCallback& Callback);
			bool ImportFile(const Core::String& Path, bool IsRemote, Core::String& Output);
			bool ImportCFunction(const Core::Vector<Core::String>& Sources, const Core::String& Name, const Core::String& Decl);
			bool ImportCLibrary(const Core::String& Path, bool IAddon = false);
			bool ImportAddon(const Core::String& Path);
			bool ImportSystemAddon(const Core::String& Name);
			Core::Option<Core::String> GetSourceCodeAppendix(const char* Label, const Core::String& Code, uint32_t LineNumber, uint32_t ColumnNumber, size_t MaxLines);
			Core::Option<Core::String> GetSourceCodeAppendixByPath(const char* Label, const Core::String& Path, uint32_t LineNumber, uint32_t ColumnNumber, size_t MaxLines);
			Core::Option<Core::String> GetScriptSection(const Core::String& SectionName);
			Core::String GetObjectView(void* Object, int TypeId);
			size_t GetFunctionsCount() const;
			Function GetFunctionById(int Id) const;
			Function GetFunctionByIndex(size_t Index) const;
			Function GetFunctionByDecl(const char* Decl) const;
			size_t GetPropertiesCount() const;
			size_t GetObjectsCount() const;
			TypeInfo GetObjectByIndex(size_t Index) const;
			size_t GetEnumCount() const;
			TypeInfo GetEnumByIndex(size_t Index) const;
			size_t GetFunctionDefsCount() const;
			TypeInfo GetFunctionDefByIndex(size_t Index) const;
			size_t GetModulesCount() const;
			asIScriptModule* GetModuleById(int Id) const;
			int GetTypeIdByDecl(const char* Decl) const;
			const char* GetTypeIdDecl(int TypeId, bool IncludeNamespace = false) const;
			TypeInfo GetTypeInfoById(int TypeId) const;
			TypeInfo GetTypeInfoByName(const char* Name);
			TypeInfo GetTypeInfoByDecl(const char* Decl) const;

		private:
			Core::Unique<ImmediateContext> CreateContext();
			bool InitializeAddon(const Core::String& Name, CLibrary& Library);
			void UninitializeAddon(const Core::String& Name, CLibrary& Library);

		public:
			static void LineHandler(asIScriptContext* Context, void* Object);
			static void ExceptionHandler(asIScriptContext* Context, void* Object);
			static void SetMemoryFunctions(void* (*Alloc)(size_t), void(*Free)(void*));
			static void CleanupThisThread();
			static ByteCodeLabel GetByteCodeInfo(uint8_t Code);
			static VirtualMachine* Get(asIScriptEngine* Engine);
			static VirtualMachine* Get();
			static size_t GetDefaultAccessMask();
			static void Cleanup();

		private:
			static Core::String GetLibraryName(const Core::String& Path);
			static asIScriptContext* RequestRawContext(asIScriptEngine* Engine, void* Data);
			static void ReturnRawContext(asIScriptEngine* Engine, asIScriptContext* Context, void* Data);
			static void MessageLogger(asSMessageInfo* Info, void* Object);
			static void RegisterAddons(VirtualMachine* Engine);
			static void* GetNullable();

		public:
			template <typename T>
			ExpectedReturn<void> SetFunction(const char* Decl, T Value)
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				asSFuncPtr* Ptr = Bridge::Function<T>(Value);
				auto Result = SetFunctionAddress(Decl, Ptr, FunctionCall::CDECLF);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <void(*)(asIScriptGeneric*)>
			ExpectedReturn<void> SetFunction(const char* Decl, void(*Value)(asIScriptGeneric*))
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				asSFuncPtr* Ptr = Bridge::Function<void (*)(asIScriptGeneric*)>(Value);
				auto Result = SetFunctionAddress(Decl, Ptr, FunctionCall::GENERIC);
				FunctionFactory::ReleaseFunctor(&Ptr);
				return Result;
			}
			template <typename T>
			ExpectedReturn<void> SetProperty(const char* Decl, T* Value)
			{
				VI_ASSERT(Decl != nullptr, "declaration should be set");
				return SetPropertyAddress(Decl, (void*)Value);
			}
			template <typename T>
			ExpectedReturn<RefClass> SetClass(const char* Name, bool GC)
			{
				VI_ASSERT(Name != nullptr, "name should be set");
				auto Class = SetClassAddress(Name, GC ? (size_t)ObjectBehaviours::REF | (size_t)ObjectBehaviours::GC : (size_t)ObjectBehaviours::REF);
				if (!Class)
					return Class;

				auto Status = Class->SetAddRef<T>();
				if (!Status)
					return Status.Error();

				Status = Class->SetRelease<T>();
				if (!Status)
					return Status.Error();

				if (!GC)
					return Class;

				Status = Class->SetMarkRef<T>();
				if (!Status)
					return Status.Error();

				Status = Class->SetIsMarkedRef<T>();
				if (!Status)
					return Status.Error();

				Status = Class->SetRefCount<T>();
				if (!Status)
					return Status.Error();

				return Class;
			}
			template <typename T>
			ExpectedReturn<TypeClass> SetStructTrivial(const char* Name)
			{
				VI_ASSERT(Name != nullptr, "name should be set");
				auto Struct = SetStructAddress(Name, sizeof(T), (size_t)ObjectBehaviours::VALUE | Bridge::GetTypeTraits<T>());
				if (!Struct)
					return Struct;

				auto Status = Struct->SetOperatorCopy<T>();
				if (!Status)
					return Status.Error();

				Status = Struct->SetDestructor<T>("void f()");
				if (!Status)
					return Status.Error();

				return Struct;
			}
			template <typename T>
			ExpectedReturn<TypeClass> SetStruct(const char* Name)
			{
				VI_ASSERT(Name != nullptr, "name should be set");
				return SetStructAddress(Name, sizeof(T), (size_t)ObjectBehaviours::VALUE | Bridge::GetTypeTraits<T>());
			}
			template <typename T>
			ExpectedReturn<TypeClass> SetPod(const char* Name)
			{
				VI_ASSERT(Name != nullptr, "name should be set");
				return SetPodAddress(Name, sizeof(T), (size_t)ObjectBehaviours::VALUE | (size_t)ObjectBehaviours::POD | Bridge::GetTypeTraits<T>());
			}
		};
	}
}
#endif