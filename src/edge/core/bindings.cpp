#include "bindings.h"
#include <new>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sstream>
#include <inttypes.h>
#include <angelscript.h>
#ifndef __psp2__
#include <locale.h>
#endif
#ifdef __BORLANDC__
#include <cmath>
#if __BORLANDC__ < 0x580
inline float cosf(float Value)
{
	return std::cos(Value);
}
inline float sinf(float Value)
{
	return std::sin(Value);
}
inline float tanf(float Value)
{
	return std::tan(Value);
}
inline float atan2f(float Y, float X)
{
	return std::atan2(Y, X);
}
inline float logf(float Value)
{
	return std::log(Value);
}
inline float powf(float X, float Y)
{
	return std::pow(X, Y);
}
#endif
inline float acosf(float Value)
{
	return std::acos(Value);
}
inline float asinf(float Value)
{
	return std::asin(Value);
}
inline float atanf(float Value)
{
	return std::atan(Value);
}
inline float coshf(float Value)
{
	return std::cosh(Value);
}
inline float sinhf(float Value)
{
	return std::sinh(Value);
}
inline float tanhf(float Value)
{
	return std::tanh(Value);
}
inline float log10f(float Value)
{
	return std::log10(Value);
}
inline float ceilf(float Value)
{
	return std::ceil(Value);
}
inline float fabsf(float Value)
{
	return std::fabs(Value);
}
inline float floorf(float Value)
{
	return std::floor(Value);
}
inline float modff(float X, float* Y)
{
	double D;
	float F = (float)modf((double)X, &D);
	*Y = (float)D;
	return F;
}
inline float sqrtf(float X)
{
	return sqrt(X);
}
#endif
#ifndef _WIN32_WCE
float fracf(float v)
{
	float Part;
	return modff(v, &Part);
}
#else
double frac(double Value)
{
	return Value;
}
#endif
#define ARRAY_CACHE 1000
#define MAP_CACHE 1003
#define TYPENAME_ARRAY "array"
#define TYPENAME_STRING "string"
#define TYPENAME_MAP "map"
#define TYPENAME_ANY "any"
#define TYPENAME_CLOSURE "closure"
#define TYPENAME_THREAD "thread"
#define TYPENAME_REF "ref"
#define TYPENAME_WEAKREF "weak_ref"
#define TYPENAME_CONSTWEAKREF "const_weak_ref"
#define TYPENAME_SCHEMA "schema"
#define TYPENAME_DECIMAL "decimal"
#define TYPENAME_VARIANT "variant"
#define TYPENAME_VERTEX "vertex"
#define TYPENAME_VECTOR3 "vector3"
#define TYPENAME_FILEENTRY "file_entry"
#define TYPENAME_REGEXMATCH "regex_match"
#define TYPENAME_ELEMENTNODE "ui_element"

namespace
{
	class StringFactory : public asIStringFactory
	{
	private:
		static StringFactory* Base;

	public:
		std::unordered_map<std::string, int> Cache;

	public:
		StringFactory()
		{
		}
		~StringFactory()
		{
			ED_ASSERT_V(Cache.size() == 0, "some strings are still in use");
		}
		const void* GetStringConstant(const char* Buffer, asUINT Length)
		{
			ED_ASSERT(Buffer != nullptr, nullptr, "buffer must not be null");

			asAcquireExclusiveLock();
			std::string Source(Buffer, Length);
			auto It = Cache.find(Source);

			if (It == Cache.end())
				It = Cache.insert(std::make_pair(std::move(Source), 1)).first;
			else
				It->second++;

			asReleaseExclusiveLock();
			return reinterpret_cast<const void*>(&It->first);
		}
		int ReleaseStringConstant(const void* Source)
		{
			ED_ASSERT(Source != nullptr, asERROR, "source must not be null");

			asAcquireExclusiveLock();
			auto It = Cache.find(*reinterpret_cast<const std::string*>(Source));
			if (It == Cache.end())
			{
				asReleaseExclusiveLock();
				return asERROR;
			}

			It->second--;
			if (It->second <= 0)
				Cache.erase(It);

			asReleaseExclusiveLock();
			return asSUCCESS;
		}
		int GetRawStringData(const void* Source, char* Buffer, asUINT* Length) const
		{
			ED_ASSERT(Source != nullptr, asERROR, "source must not be null");
			if (Length != nullptr)
				*Length = (asUINT)reinterpret_cast<const std::string*>(Source)->length();

			if (Buffer != nullptr)
				memcpy(Buffer, reinterpret_cast<const std::string*>(Source)->c_str(), reinterpret_cast<const std::string*>(Source)->length());

			return asSUCCESS;
		}

	public:
		static StringFactory* Get()
		{
			if (!Base)
				Base = ED_NEW(StringFactory);

			return Base;
		}
		static void Free()
		{
			if (Base != nullptr && Base->Cache.empty())
				ED_DELETE(StringFactory, Base);
		}
	};
	StringFactory* StringFactory::Base = nullptr;
}

namespace Edge
{
	namespace Script
	{
		namespace Bindings
		{
			template <typename T, typename V>
			void PopulateComponent(V& Base)
			{
				Base.SetMethod("uptr@ get_name() const", &T::GetName);
				Base.SetMethod("uint64 get_id() const", &T::GetId);
			}

			Core::Mapping<std::unordered_map<uint64_t, std::pair<std::string, int>>>* Names = nullptr;
			uint64_t TypeCache::Set(uint64_t Id, const std::string& Name)
			{
				ED_ASSERT(Id > 0 && !Name.empty(), 0, "id should be greater than zero and name should not be empty");

				using Map = Core::Mapping<std::unordered_map<uint64_t, std::pair<std::string, int>>>;
				if (!Names)
					Names = ED_NEW(Map);

				Names->Map[Id] = std::make_pair(Name, (int)-1);
				return Id;
			}
			int TypeCache::GetTypeId(uint64_t Id)
			{
				auto It = Names->Map.find(Id);
				if (It == Names->Map.end())
					return -1;

				if (It->second.second < 0)
				{
					VMManager* Engine = VMManager::Get();
					ED_ASSERT(Engine != nullptr, -1, "engine should be set");
					It->second.second = Engine->Global().GetTypeIdByDecl(It->second.first.c_str());
				}

				return It->second.second;
			}

			void String::Construct(std::string* Current)
			{
				ED_ASSERT_V(Current != nullptr, "Current should be set");
				new(Current) std::string();
			}
			void String::CopyConstruct(const std::string& Other, std::string* Current)
			{
				ED_ASSERT_V(Current != nullptr, "Current should be set");
				new(Current) std::string(Other);
			}
			void String::Destruct(std::string* Current)
			{
				ED_ASSERT_V(Current != nullptr, "Current should be set");
				Current->~basic_string();
			}
			std::string& String::AddAssignTo(const std::string& Current, std::string& Dest)
			{
				Dest += Current;
				return Dest;
			}
			bool String::IsEmpty(const std::string& Current)
			{
				return Current.empty();
			}
			void* String::ToPtr(const std::string& Value)
			{
				return (void*)Value.c_str();
			}
			std::string String::Reverse(const std::string& Value)
			{
				Core::Parser Result(Value);
				Result.Reverse();
				return Result.R();
			}
			std::string& String::AssignUInt64To(as_uint64_t Value, std::string& Dest)
			{
				std::ostringstream Stream;
				Stream << Value;
				Dest = Stream.str();
				return Dest;
			}
			std::string& String::AddAssignUInt64To(as_uint64_t Value, std::string& Dest)
			{
				std::ostringstream Stream;
				Stream << Value;
				Dest += Stream.str();
				return Dest;
			}
			std::string String::AddUInt641(const std::string& Current, as_uint64_t Value)
			{
				std::ostringstream Stream;
				Stream << Value;
				Stream << Current;
				return Stream.str();
			}
			std::string String::AddInt641(as_int64_t Value, const std::string& Current)
			{
				std::ostringstream Stream;
				Stream << Current;
				Stream << Value;
				return Stream.str();
			}
			std::string& String::AssignInt64To(as_int64_t Value, std::string& Dest)
			{
				std::ostringstream Stream;
				Stream << Value;
				Dest = Stream.str();
				return Dest;
			}
			std::string& String::AddAssignInt64To(as_int64_t Value, std::string& Dest)
			{
				std::ostringstream Stream;
				Stream << Value;
				Dest += Stream.str();
				return Dest;
			}
			std::string String::AddInt642(const std::string& Current, as_int64_t Value)
			{
				std::ostringstream Stream;
				Stream << Value;
				return Current + Stream.str();
			}
			std::string String::AddUInt642(as_uint64_t Value, const std::string& Current)
			{
				std::ostringstream Stream;
				Stream << Value;
				return Stream.str() + Current;
			}
			std::string& String::AssignDoubleTo(double Value, std::string& Dest)
			{
				std::ostringstream Stream;
				Stream << Value;
				Dest = Stream.str();
				return Dest;
			}
			std::string& String::AddAssignDoubleTo(double Value, std::string& Dest)
			{
				std::ostringstream Stream;
				Stream << Value;
				Dest += Stream.str();
				return Dest;
			}
			std::string& String::AssignFloatTo(float Value, std::string& Dest)
			{
				std::ostringstream Stream;
				Stream << Value;
				Dest = Stream.str();
				return Dest;
			}
			std::string& String::AddAssignFloatTo(float Value, std::string& Dest)
			{
				std::ostringstream Stream;
				Stream << Value;
				Dest += Stream.str();
				return Dest;
			}
			std::string& String::AssignBoolTo(bool Value, std::string& Dest)
			{
				std::ostringstream Stream;
				Stream << (Value ? "true" : "false");
				Dest = Stream.str();
				return Dest;
			}
			std::string& String::AddAssignBoolTo(bool Value, std::string& Dest)
			{
				std::ostringstream Stream;
				Stream << (Value ? "true" : "false");
				Dest += Stream.str();
				return Dest;
			}
			std::string String::AddDouble1(const std::string& Current, double Value)
			{
				std::ostringstream Stream;
				Stream << Value;
				return Current + Stream.str();
			}
			std::string String::AddDouble2(double Value, const std::string& Current)
			{
				std::ostringstream Stream;
				Stream << Value;
				return Stream.str() + Current;
			}
			std::string String::AddFloat1(const std::string& Current, float Value)
			{
				std::ostringstream Stream;
				Stream << Value;
				return Current + Stream.str();
			}
			std::string String::AddFloat2(float Value, const std::string& Current)
			{
				std::ostringstream Stream;
				Stream << Value;
				return Stream.str() + Current;
			}
			std::string String::AddBool1(const std::string& Current, bool Value)
			{
				std::ostringstream Stream;
				Stream << (Value ? "true" : "false");
				return Current + Stream.str();
			}
			std::string String::AddBool2(bool Value, const std::string& Current)
			{
				std::ostringstream Stream;
				Stream << (Value ? "true" : "false");
				return Stream.str() + Current;
			}
			char* String::CharAt(size_t Value, std::string& Current)
			{
				if (Value >= Current.size())
				{
					VMCContext* Context = asGetActiveContext();
					if (Context != nullptr)
						Context->SetException("Out of range");

					return 0;
				}

				return &Current[Value];
			}
			int String::Cmp(const std::string& A, const std::string& B)
			{
				int Result = 0;
				if (A < B)
					Result = -1;
				else if (A > B)
					Result = 1;

				return Result;
			}
			int String::FindFirst(const std::string& Needle, size_t Start, const std::string& Current)
			{
				return (int)Current.find(Needle, (size_t)Start);
			}
			int String::FindFirstOf(const std::string& Needle, size_t Start, const std::string& Current)
			{
				return (int)Current.find_first_of(Needle, (size_t)Start);
			}
			int String::FindLastOf(const std::string& Needle, size_t Start, const std::string& Current)
			{
				return (int)Current.find_last_of(Needle, (size_t)Start);
			}
			int String::FindFirstNotOf(const std::string& Needle, size_t Start, const std::string& Current)
			{
				return (int)Current.find_first_not_of(Needle, (size_t)Start);
			}
			int String::FindLastNotOf(const std::string& Needle, size_t Start, const std::string& Current)
			{
				return (int)Current.find_last_not_of(Needle, (size_t)Start);
			}
			int String::FindLast(const std::string& Needle, int Start, const std::string& Current)
			{
				return (int)Current.rfind(Needle, (size_t)Start);
			}
			void String::Insert(size_t Offset, const std::string& Other, std::string& Current)
			{
				Current.insert(Offset, Other);
			}
			void String::Erase(size_t Offset, int Count, std::string& Current)
			{
				Current.erase(Offset, (size_t)(Count < 0 ? std::string::npos : Count));
			}
			size_t String::Length(const std::string& Current)
			{
				return (size_t)Current.length();
			}
			void String::Resize(size_t Size, std::string& Current)
			{
				Current.resize(Size);
			}
			std::string String::Replace(const std::string& A, const std::string& B, uint64_t Offset, const std::string& Base)
			{
				return Edge::Core::Parser(Base).Replace(A, B, (size_t)Offset).R();
			}
			as_int64_t String::IntStore(const std::string& Value, size_t Base, size_t* ByteCount)
			{
				if (Base != 10 && Base != 16)
				{
					if (ByteCount)
						*ByteCount = 0;

					return 0;
				}

				const char* End = &Value[0];
				bool Sign = false;
				if (*End == '-')
				{
					Sign = true;
					End++;
				}
				else if (*End == '+')
					End++;

				as_int64_t Result = 0;
				if (Base == 10)
				{
					while (*End >= '0' && *End <= '9')
					{
						Result *= 10;
						Result += *End++ - '0';
					}
				}
				else
				{
					while ((*End >= '0' && *End <= '9') || (*End >= 'a' && *End <= 'f') || (*End >= 'A' && *End <= 'F'))
					{
						Result *= 16;
						if (*End >= '0' && *End <= '9')
							Result += *End++ - '0';
						else if (*End >= 'a' && *End <= 'f')
							Result += *End++ - 'a' + 10;
						else if (*End >= 'A' && *End <= 'F')
							Result += *End++ - 'A' + 10;
					}
				}

				if (ByteCount)
					*ByteCount = size_t(size_t(End - Value.c_str()));

				if (Sign)
					Result = -Result;

				return Result;
			}
			as_uint64_t String::UIntStore(const std::string& Value, size_t Base, size_t* ByteCount)
			{
				if (Base != 10 && Base != 16)
				{
					if (ByteCount)
						*ByteCount = 0;
					return 0;
				}

				const char* End = &Value[0];
				as_uint64_t Result = 0;

				if (Base == 10)
				{
					while (*End >= '0' && *End <= '9')
					{
						Result *= 10;
						Result += *End++ - '0';
					}
				}
				else
				{
					while ((*End >= '0' && *End <= '9') || (*End >= 'a' && *End <= 'f') || (*End >= 'A' && *End <= 'F'))
					{
						Result *= 16;
						if (*End >= '0' && *End <= '9')
							Result += *End++ - '0';
						else if (*End >= 'a' && *End <= 'f')
							Result += *End++ - 'a' + 10;
						else if (*End >= 'A' && *End <= 'F')
							Result += *End++ - 'A' + 10;
					}
				}

				if (ByteCount)
					*ByteCount = size_t(size_t(End - Value.c_str()));

				return Result;
			}
			double String::FloatStore(const std::string& Value, size_t* ByteCount)
			{
				char* End;
	#if !defined(_WIN32_WCE) && !defined(ANDROID) && !defined(__psp2__)
				char* Temp = setlocale(LC_NUMERIC, 0);
				std::string Base = Temp ? Temp : "C";
				setlocale(LC_NUMERIC, "C");
	#endif

				double res = strtod(Value.c_str(), &End);
	#if !defined(_WIN32_WCE) && !defined(ANDROID) && !defined(__psp2__)
				setlocale(LC_NUMERIC, Base.c_str());
	#endif
				if (ByteCount)
					*ByteCount = size_t(size_t(End - Value.c_str()));

				return res;
			}
			std::string String::Sub(size_t Start, int Count, const std::string& Current)
			{
				std::string Result;
				if (Start < Current.length() && Count != 0)
					Result = Current.substr(Start, (size_t)(Count < 0 ? std::string::npos : Count));

				return Result;
			}
			bool String::Equals(const std::string& Left, const std::string& Right)
			{
				return Left == Right;
			}
			std::string String::ToLower(const std::string& Symbol)
			{
				return Edge::Core::Parser(Symbol).ToLower().R();
			}
			std::string String::ToUpper(const std::string& Symbol)
			{
				return Edge::Core::Parser(Symbol).ToUpper().R();
			}
			std::string String::ToInt8(char Value)
			{
				return std::to_string(Value);
			}
			std::string String::ToInt16(short Value)
			{
				return std::to_string(Value);
			}
			std::string String::ToInt(int Value)
			{
				return std::to_string(Value);
			}
			std::string String::ToInt64(int64_t Value)
			{
				return std::to_string(Value);
			}
			std::string String::ToUInt8(unsigned char Value)
			{
				return std::to_string(Value);
			}
			std::string String::ToUInt16(unsigned short Value)
			{
				return std::to_string(Value);
			}
			std::string String::ToUInt(unsigned int Value)
			{
				return std::to_string(Value);
			}
			std::string String::ToUInt64(uint64_t Value)
			{
				return std::to_string(Value);
			}
			std::string String::ToFloat(float Value)
			{
				return std::to_string(Value);
			}
			std::string String::ToDouble(double Value)
			{
				return std::to_string(Value);
			}
			std::string String::ToPointer(void* Value)
			{
				char* Buffer = (char*)Value;
				if (!Buffer)
					return std::string();

				size_t Size = strlen(Buffer);
				return Size > 1024 * 1024 * 1024 ? std::string() : std::string(Buffer, Size);
			}
			Array* String::Split(const std::string& Splitter, const std::string& Current)
			{
				VMCContext* Context = asGetActiveContext();
				VMCManager* Engine = Context->GetEngine();
				asITypeInfo* ArrayType = Engine->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				Array* Array = Array::Create(ArrayType);

				int Offset = 0, Prev = 0, Count = 0;
				while ((Offset = (int)Current.find(Splitter, Prev)) != (int)std::string::npos)
				{
					Array->Resize(Array->GetSize() + 1);
					((std::string*)Array->At(Count))->assign(&Current[Prev], Offset - Prev);
					Prev = Offset + (int)Splitter.length();
					Count++;
				}

				Array->Resize(Array->GetSize() + 1);
				((std::string*)Array->At(Count))->assign(&Current[Prev]);
				return Array;
			}
			std::string String::Join(const Array& Array, const std::string& Splitter)
			{
				std::string Current = "";
				if (!Array.GetSize())
					return Current;

				int i;
				for (i = 0; i < (int)Array.GetSize() - 1; i++)
				{
					Current += *(std::string*)Array.At(i);
					Current += Splitter;
				}

				Current += *(std::string*)Array.At(i);
				return Current;
			}
			char String::ToChar(const std::string& Symbol)
			{
				return Symbol.empty() ? '\0' : Symbol[0];
			}

			Mutex::Mutex() noexcept : Ref(1)
			{
			}
			void Mutex::Release()
			{
				if (asAtomicDec(Ref) <= 0)
				{
					this->~Mutex();
					asFreeMem((void*)this);
				}
			}
			void Mutex::AddRef()
			{
				asAtomicInc(Ref);
			}
			bool Mutex::TryLock()
			{
				return Base.try_lock();
			}
			void Mutex::Lock()
			{
				Base.lock();
			}
			void Mutex::Unlock()
			{
				Base.unlock();
			}
			Mutex* Mutex::Factory()
			{
				void* Data = asAllocMem(sizeof(Mutex));
				if (!Data)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context != nullptr)
						Context->SetException("Out of memory");

					return nullptr;
				}

				return new(Data) Mutex();
			}

			float Math::FpFromIEEE(uint32_t Raw)
			{
				return *reinterpret_cast<float*>(&Raw);
			}
			uint32_t Math::FpToIEEE(float Value)
			{
				return *reinterpret_cast<uint32_t*>(&Value);
			}
			double Math::FpFromIEEE(as_uint64_t Raw)
			{
				return *reinterpret_cast<double*>(&Raw);
			}
			as_uint64_t Math::FpToIEEE(double Value)
			{
				return *reinterpret_cast<as_uint64_t*>(&Value);
			}
			bool Math::CloseTo(float A, float B, float Epsilon)
			{
				if (A == B)
					return true;

				float diff = fabsf(A - B);
				if ((A == 0 || B == 0) && (diff < Epsilon))
					return true;

				return diff / (fabs(A) + fabs(B)) < Epsilon;
			}
			bool Math::CloseTo(double A, double B, double Epsilon)
			{
				if (A == B)
					return true;

				double diff = fabs(A - B);
				if ((A == 0 || B == 0) && (diff < Epsilon))
					return true;

				return diff / (fabs(A) + fabs(B)) < Epsilon;
			}

			void Exception::Throw(const std::string& In)
			{
				VMCContext* Context = asGetActiveContext();
				if (Context != nullptr)
					Context->SetException(In.empty() ? "runtime exception" : In.c_str());
			}
			std::string Exception::GetException()
			{
				VMCContext* Context = asGetActiveContext();
				if (!Context)
					return "";

				const char* Message = Context->GetExceptionString();
				if (!Message)
					return "";

				return Message;
			}

			Any::Any(VMCManager* _Engine) noexcept
			{
				Engine = _Engine;
				RefCount = 1;
				GCFlag = false;
				Value.TypeId = 0;
				Value.ValueInt = 0;

				Engine->NotifyGarbageCollectorOfNewObject(this, Engine->GetTypeInfoByName(TYPENAME_ANY));
			}
			Any::Any(void* Ref, int RefTypeId, VMCManager* _Engine) noexcept
			{
				Engine = _Engine;
				RefCount = 1;
				GCFlag = false;
				Value.TypeId = 0;
				Value.ValueInt = 0;

				Engine->NotifyGarbageCollectorOfNewObject(this, Engine->GetTypeInfoByName(TYPENAME_ANY));
				Store(Ref, RefTypeId);
			}
			Any::Any(const Any& Other) noexcept
			{
				Engine = Other.Engine;
				RefCount = 1;
				GCFlag = false;
				Value.TypeId = 0;
				Value.ValueInt = 0;

				Engine->NotifyGarbageCollectorOfNewObject(this, Engine->GetTypeInfoByName(TYPENAME_ANY));
				if ((Other.Value.TypeId & asTYPEID_MASK_OBJECT))
				{
					VMCTypeInfo* Info = Engine->GetTypeInfoById(Other.Value.TypeId);
					if (Info != nullptr)
						Info->AddRef();
				}

				Value.TypeId = Other.Value.TypeId;
				if (Value.TypeId & asTYPEID_OBJHANDLE)
				{
					Value.ValueObj = Other.Value.ValueObj;
					Engine->AddRefScriptObject(Value.ValueObj, Engine->GetTypeInfoById(Value.TypeId));
				}
				else if (Value.TypeId & asTYPEID_MASK_OBJECT)
					Value.ValueObj = Engine->CreateScriptObjectCopy(Other.Value.ValueObj, Engine->GetTypeInfoById(Value.TypeId));
				else
					Value.ValueInt = Other.Value.ValueInt;
			}
			Any::~Any() noexcept
			{
				FreeObject();
			}
			Any& Any::operator=(const Any& Other) noexcept
			{
				if ((Other.Value.TypeId & asTYPEID_MASK_OBJECT))
				{
					VMCTypeInfo* Info = Engine->GetTypeInfoById(Other.Value.TypeId);
					if (Info != nullptr)
						Info->AddRef();
				}

				FreeObject();

				Value.TypeId = Other.Value.TypeId;
				if (Value.TypeId & asTYPEID_OBJHANDLE)
				{
					Value.ValueObj = Other.Value.ValueObj;
					Engine->AddRefScriptObject(Value.ValueObj, Engine->GetTypeInfoById(Value.TypeId));
				}
				else if (Value.TypeId & asTYPEID_MASK_OBJECT)
					Value.ValueObj = Engine->CreateScriptObjectCopy(Other.Value.ValueObj, Engine->GetTypeInfoById(Value.TypeId));
				else
					Value.ValueInt = Other.Value.ValueInt;

				return *this;
			}
			int Any::CopyFrom(const Any* Other)
			{
				if (Other == 0)
					return asINVALID_ARG;

				*this = *(Any*)Other;
				return 0;
			}
			void Any::Store(void* Ref, int RefTypeId)
			{
				if ((RefTypeId & asTYPEID_MASK_OBJECT))
				{
					VMCTypeInfo* T = Engine->GetTypeInfoById(RefTypeId);
					if (T)
						T->AddRef();
				}

				FreeObject();

				Value.TypeId = RefTypeId;
				if (Value.TypeId & asTYPEID_OBJHANDLE)
				{
					Value.ValueObj = *(void**)Ref;
					Engine->AddRefScriptObject(Value.ValueObj, Engine->GetTypeInfoById(Value.TypeId));
				}
				else if (Value.TypeId & asTYPEID_MASK_OBJECT)
				{
					Value.ValueObj = Engine->CreateScriptObjectCopy(Ref, Engine->GetTypeInfoById(Value.TypeId));
				}
				else
				{
					Value.ValueInt = 0;

					int Size = Engine->GetSizeOfPrimitiveType(Value.TypeId);
					memcpy(&Value.ValueInt, Ref, Size);
				}
			}
			bool Any::Retrieve(void* Ref, int RefTypeId) const
			{
				if (RefTypeId & asTYPEID_OBJHANDLE)
				{
					if ((Value.TypeId & asTYPEID_MASK_OBJECT))
					{
						if ((Value.TypeId & asTYPEID_HANDLETOCONST) && !(RefTypeId & asTYPEID_HANDLETOCONST))
							return false;

						Engine->RefCastObject(Value.ValueObj, Engine->GetTypeInfoById(Value.TypeId), Engine->GetTypeInfoById(RefTypeId), reinterpret_cast<void**>(Ref));
						if (*(asPWORD*)Ref == 0)
							return false;

						return true;
					}
				}
				else if (RefTypeId & asTYPEID_MASK_OBJECT)
				{
					if (Value.TypeId == RefTypeId)
					{
						Engine->AssignScriptObject(Ref, Value.ValueObj, Engine->GetTypeInfoById(Value.TypeId));
						return true;
					}
				}
				else
				{
					int Size1 = Engine->GetSizeOfPrimitiveType(Value.TypeId);
					int Size2 = Engine->GetSizeOfPrimitiveType(RefTypeId);

					if (Size1 == Size2)
					{
						memcpy(Ref, &Value.ValueInt, Size1);
						return true;
					}
				}

				return false;
			}
			int Any::GetTypeId() const
			{
				return Value.TypeId;
			}
			void Any::FreeObject()
			{
				if (Value.TypeId & asTYPEID_MASK_OBJECT)
				{
					VMCTypeInfo* T = Engine->GetTypeInfoById(Value.TypeId);
					Engine->ReleaseScriptObject(Value.ValueObj, T);

					if (T)
						T->Release();

					Value.ValueObj = 0;
					Value.TypeId = 0;
				}
			}
			void Any::EnumReferences(VMCManager* InEngine)
			{
				if (Value.ValueObj && (Value.TypeId & asTYPEID_MASK_OBJECT))
				{
					VMCTypeInfo* SubType = Engine->GetTypeInfoById(Value.TypeId);
					if ((SubType->GetFlags() & asOBJ_REF))
						InEngine->GCEnumCallback(Value.ValueObj);
					else if ((SubType->GetFlags() & asOBJ_VALUE) && (SubType->GetFlags() & asOBJ_GC))
						Engine->ForwardGCEnumReferences(Value.ValueObj, SubType);

					VMCTypeInfo* T = InEngine->GetTypeInfoById(Value.TypeId);
					if (T)
						InEngine->GCEnumCallback(T);
				}
			}
			void Any::ReleaseAllHandles(VMCManager*)
			{
				FreeObject();
			}
			int Any::AddRef() const
			{
				GCFlag = false;
				return asAtomicInc(RefCount);
			}
			int Any::Release() const
			{
				GCFlag = false;
				if (asAtomicDec(RefCount) == 0)
				{
					this->~Any();
					asFreeMem((void*)this);
					return 0;
				}

				return RefCount;
			}
			int Any::GetRefCount()
			{
				return RefCount;
			}
			void Any::SetFlag()
			{
				GCFlag = true;
			}
			bool Any::GetFlag()
			{
				return GCFlag;
			}
			Core::Unique<Any> Any::Create()
			{
				VMCContext* Context = asGetActiveContext();
				if (!Context)
					return nullptr;

				void* Data = asAllocMem(sizeof(Any));
				return new(Data) Any(Context->GetEngine());
			}
			Any* Any::Create(int TypeId, void* Ref)
			{
				VMCContext* Context = asGetActiveContext();
				if (!Context)
					return nullptr;

				void* Data = asAllocMem(sizeof(Any));
				return new(Data) Any(Ref, TypeId, Context->GetEngine());
			}
			Any* Any::Create(const char* Decl, void* Ref)
			{
				VMCContext* Context = asGetActiveContext();
				if (!Context)
					return nullptr;

				VMCManager* Engine = Context->GetEngine();
				void* Data = asAllocMem(sizeof(Any));
				return new(Data) Any(Ref, Engine->GetTypeIdByDecl(Decl), Engine);
			}
			void Any::Factory1(VMCGeneric* G)
			{
				VMCManager* Engine = G->GetEngine();
				void* Mem = asAllocMem(sizeof(Any));
				*(Any**)G->GetAddressOfReturnLocation() = new(Mem) Any(Engine);
			}
			void Any::Factory2(VMCGeneric* G)
			{
				VMCManager* Engine = G->GetEngine();
				void* Ref = (void*)G->GetArgAddress(0);
				void* Mem = asAllocMem(sizeof(Any));
				int RefType = G->GetArgTypeId(0);

				*(Any**)G->GetAddressOfReturnLocation() = new(Mem) Any(Ref, RefType, Engine);
			}
			Any& Any::Assignment(Any* Other, Any* Self)
			{
				return *Self = *Other;
			}

			Array& Array::operator=(const Array& Other) noexcept
			{
				if (&Other != this && Other.GetArrayObjectType() == GetArrayObjectType())
				{
					Resize(Other.Buffer->NumElements);
					CopyBuffer(Buffer, Other.Buffer);
				}

				return *this;
			}
			Array::Array(VMCTypeInfo* Info, void* BufferPtr) noexcept
			{
				ED_ASSERT_V(Info && std::string(Info->GetName()) == TYPENAME_ARRAY, "array type is invalid");
				RefCount = 1;
				GCFlag = false;
				ObjType = Info;
				ObjType->AddRef();
				Buffer = 0;
				Precache();

				VMCManager* Engine = Info->GetEngine();
				if (SubTypeId & asTYPEID_MASK_OBJECT)
					ElementSize = sizeof(asPWORD);
				else
					ElementSize = Engine->GetSizeOfPrimitiveType(SubTypeId);

				size_t length = *(size_t*)BufferPtr;
				if (!CheckMaxSize(length))
					return;

				if ((Info->GetSubTypeId() & asTYPEID_MASK_OBJECT) == 0)
				{
					CreateBuffer(&Buffer, length);
					if (length > 0)
						memcpy(At(0), (((size_t*)BufferPtr) + 1), (size_t)length * (size_t)ElementSize);
				}
				else if (Info->GetSubTypeId() & asTYPEID_OBJHANDLE)
				{
					CreateBuffer(&Buffer, length);
					if (length > 0)
						memcpy(At(0), (((size_t*)BufferPtr) + 1), (size_t)length * (size_t)ElementSize);

					memset((((size_t*)BufferPtr) + 1), 0, (size_t)length * (size_t)ElementSize);
				}
				else if (Info->GetSubType()->GetFlags() & asOBJ_REF)
				{
					SubTypeId |= asTYPEID_OBJHANDLE;
					CreateBuffer(&Buffer, length);
					SubTypeId &= ~asTYPEID_OBJHANDLE;

					if (length > 0)
						memcpy(Buffer->Data, (((size_t*)BufferPtr) + 1), (size_t)length * (size_t)ElementSize);

					memset((((size_t*)BufferPtr) + 1), 0, (size_t)length * (size_t)ElementSize);
				}
				else
				{
					CreateBuffer(&Buffer, length);
					for (size_t n = 0; n < length; n++)
					{
						void* Obj = At(n);
						unsigned char* srcObj = (unsigned char*)BufferPtr;
						srcObj += 4 + n * Info->GetSubType()->GetSize();
						Engine->AssignScriptObject(Obj, srcObj, Info->GetSubType());
					}
				}

				if (ObjType->GetFlags() & asOBJ_GC)
					ObjType->GetEngine()->NotifyGarbageCollectorOfNewObject(this, ObjType);
			}
			Array::Array(size_t length, VMCTypeInfo* Info) noexcept
			{
				ED_ASSERT_V(Info && std::string(Info->GetName()) == TYPENAME_ARRAY, "array type is invalid");
				RefCount = 1;
				GCFlag = false;
				ObjType = Info;
				ObjType->AddRef();
				Buffer = 0;
				Precache();

				if (SubTypeId & asTYPEID_MASK_OBJECT)
					ElementSize = sizeof(asPWORD);
				else
					ElementSize = ObjType->GetEngine()->GetSizeOfPrimitiveType(SubTypeId);

				if (!CheckMaxSize(length))
					return;

				CreateBuffer(&Buffer, length);
				if (ObjType->GetFlags() & asOBJ_GC)
					ObjType->GetEngine()->NotifyGarbageCollectorOfNewObject(this, ObjType);
			}
			Array::Array(const Array& Other) noexcept
			{
				RefCount = 1;
				GCFlag = false;
				ObjType = Other.ObjType;
				ObjType->AddRef();
				Buffer = 0;
				Precache();

				ElementSize = Other.ElementSize;
				if (ObjType->GetFlags() & asOBJ_GC)
					ObjType->GetEngine()->NotifyGarbageCollectorOfNewObject(this, ObjType);

				CreateBuffer(&Buffer, 0);
				*this = Other;
			}
			Array::Array(size_t Length, void* DefaultValue, VMCTypeInfo* Info) noexcept
			{
				ED_ASSERT_V(Info && std::string(Info->GetName()) == TYPENAME_ARRAY, "array type is invalid");
				RefCount = 1;
				GCFlag = false;
				ObjType = Info;
				ObjType->AddRef();
				Buffer = 0;
				Precache();

				if (SubTypeId & asTYPEID_MASK_OBJECT)
					ElementSize = sizeof(asPWORD);
				else
					ElementSize = ObjType->GetEngine()->GetSizeOfPrimitiveType(SubTypeId);

				if (!CheckMaxSize(Length))
					return;

				CreateBuffer(&Buffer, Length);
				if (ObjType->GetFlags() & asOBJ_GC)
					ObjType->GetEngine()->NotifyGarbageCollectorOfNewObject(this, ObjType);

				for (size_t i = 0; i < GetSize(); i++)
					SetValue(i, DefaultValue);
			}
			void Array::SetValue(size_t Index, void* Value)
			{
				void* Ptr = At(Index);
				if (Ptr == 0)
					return;

				if ((SubTypeId & ~asTYPEID_MASK_SEQNBR) && !(SubTypeId & asTYPEID_OBJHANDLE))
					ObjType->GetEngine()->AssignScriptObject(Ptr, Value, ObjType->GetSubType());
				else if (SubTypeId & asTYPEID_OBJHANDLE)
				{
					void* Swap = *(void**)Ptr;
					*(void**)Ptr = *(void**)Value;
					ObjType->GetEngine()->AddRefScriptObject(*(void**)Value, ObjType->GetSubType());
					if (Swap)
						ObjType->GetEngine()->ReleaseScriptObject(Swap, ObjType->GetSubType());
				}
				else if (SubTypeId == asTYPEID_BOOL ||
					SubTypeId == asTYPEID_INT8 ||
					SubTypeId == asTYPEID_UINT8)
					*(char*)Ptr = *(char*)Value;
				else if (SubTypeId == asTYPEID_INT16 ||
					SubTypeId == asTYPEID_UINT16)
					*(short*)Ptr = *(short*)Value;
				else if (SubTypeId == asTYPEID_INT32 ||
					SubTypeId == asTYPEID_UINT32 ||
					SubTypeId == asTYPEID_FLOAT ||
					SubTypeId > asTYPEID_DOUBLE)
					*(int*)Ptr = *(int*)Value;
				else if (SubTypeId == asTYPEID_INT64 ||
					SubTypeId == asTYPEID_UINT64 ||
					SubTypeId == asTYPEID_DOUBLE)
					*(double*)Ptr = *(double*)Value;
			}
			Array::~Array()
			{
				if (Buffer)
				{
					DeleteBuffer(Buffer);
					Buffer = 0;
				}
				if (ObjType)
					ObjType->Release();
			}
			size_t Array::GetSize() const
			{
				return Buffer->NumElements;
			}
			bool Array::IsEmpty() const
			{
				return Buffer->NumElements == 0;
			}
			void Array::Reserve(size_t MaxElements)
			{
				if (MaxElements <= Buffer->MaxElements)
					return;

				if (!CheckMaxSize(MaxElements))
					return;

				SBuffer* NewBuffer = reinterpret_cast<SBuffer*>(asAllocMem(sizeof(SBuffer) - 1 + (size_t)ElementSize * (size_t)MaxElements));
				if (NewBuffer)
				{
					NewBuffer->NumElements = Buffer->NumElements;
					NewBuffer->MaxElements = MaxElements;
				}
				else
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Out of memory");
					return;
				}

				memcpy(NewBuffer->Data, Buffer->Data, (size_t)Buffer->NumElements * (size_t)ElementSize);
				asFreeMem(Buffer);
				Buffer = NewBuffer;
			}
			void Array::Resize(size_t NumElements)
			{
				if (!CheckMaxSize(NumElements))
					return;

				Resize((int)NumElements - (int)Buffer->NumElements, (size_t)-1);
			}
			void Array::RemoveRange(size_t Start, size_t Count)
			{
				if (Count == 0)
					return;

				if (Buffer == 0 || Start > Buffer->NumElements)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Index out of bounds");
					return;
				}

				if (Start + Count > Buffer->NumElements)
					Count = Buffer->NumElements - Start;

				Destruct(Buffer, Start, Start + Count);
				memmove(Buffer->Data + Start * (size_t)ElementSize, Buffer->Data + (Start + Count) * (size_t)ElementSize, (size_t)(Buffer->NumElements - Start - Count) * (size_t)ElementSize);
				Buffer->NumElements -= Count;
			}
			void Array::Resize(int Delta, size_t Where)
			{
				if (Delta < 0)
				{
					if (-Delta > (int)Buffer->NumElements)
						Delta = -(int)Buffer->NumElements;
					if (Where > Buffer->NumElements + Delta)
						Where = Buffer->NumElements + Delta;
				}
				else if (Delta > 0)
				{
					if (!CheckMaxSize(Buffer->NumElements + Delta))
						return;

					if (Where > Buffer->NumElements)
						Where = Buffer->NumElements;
				}

				if (Delta == 0)
					return;

				if (Buffer->MaxElements < Buffer->NumElements + Delta)
				{
					size_t Count = (size_t)Buffer->NumElements + (size_t)Delta, Size = (size_t)ElementSize;
					SBuffer* NewBuffer = reinterpret_cast<SBuffer*>(asAllocMem(sizeof(SBuffer) - 1 + Size * Count));
					if (NewBuffer)
					{
						NewBuffer->NumElements = Buffer->NumElements + Delta;
						NewBuffer->MaxElements = NewBuffer->NumElements;
					}
					else
					{
						VMCContext* Context = asGetActiveContext();
						if (Context)
							Context->SetException("Out of memory");
						return;
					}

					memcpy(NewBuffer->Data, Buffer->Data, (size_t)Where * (size_t)ElementSize);
					if (Where < Buffer->NumElements)
						memcpy(NewBuffer->Data + (Where + Delta) * (size_t)ElementSize, Buffer->Data + Where * (size_t)ElementSize, (size_t)(Buffer->NumElements - Where) * (size_t)ElementSize);

					Construct(NewBuffer, Where, Where + Delta);
					asFreeMem(Buffer);
					Buffer = NewBuffer;
				}
				else if (Delta < 0)
				{
					Destruct(Buffer, Where, Where - Delta);
					memmove(Buffer->Data + Where * (size_t)ElementSize, Buffer->Data + (Where - Delta) * (size_t)ElementSize, (size_t)(Buffer->NumElements - (Where - Delta)) * (size_t)ElementSize);
					Buffer->NumElements += Delta;
				}
				else
				{
					memmove(Buffer->Data + (Where + Delta) * (size_t)ElementSize, Buffer->Data + Where * (size_t)ElementSize, (size_t)(Buffer->NumElements - Where) * (size_t)ElementSize);
					Construct(Buffer, Where, Where + Delta);
					Buffer->NumElements += Delta;
				}
			}
			bool Array::CheckMaxSize(size_t NumElements)
			{
				size_t MaxSize = 0xFFFFFFFFul - sizeof(SBuffer) + 1;
				if (ElementSize > 0)
					MaxSize /= (size_t)ElementSize;

				if (NumElements > MaxSize)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Too large array size");

					return false;
				}

				return true;
			}
			VMCTypeInfo* Array::GetArrayObjectType() const
			{
				return ObjType;
			}
			int Array::GetArrayTypeId() const
			{
				return ObjType->GetTypeId();
			}
			int Array::GetElementTypeId() const
			{
				return SubTypeId;
			}
			void Array::InsertAt(size_t Index, void* Value)
			{
				if (Index > Buffer->NumElements)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Index out of bounds");
					return;
				}

				Resize(1, Index);
				SetValue(Index, Value);
			}
			void Array::InsertAt(size_t Index, const Array& Array)
			{
				if (Index > Buffer->NumElements)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Index out of bounds");
					return;
				}

				if (ObjType != Array.ObjType)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Mismatching array types");
					return;
				}

				size_t elements = Array.GetSize();
				Resize((int)elements, Index);

				if (&Array != this)
				{
					for (size_t i = 0; i < Array.GetSize(); i++)
					{
						void* Value = const_cast<void*>(Array.At(i));
						SetValue(Index + i, Value);
					}
				}
				else
				{
					for (size_t i = 0; i < Index; i++)
					{
						void* Value = const_cast<void*>(Array.At(i));
						SetValue(Index + i, Value);
					}

					for (size_t i = Index + elements, k = 0; i < Array.GetSize(); i++, k++)
					{
						void* Value = const_cast<void*>(Array.At(i));
						SetValue(Index + Index + k, Value);
					}
				}
			}
			void Array::InsertLast(void* Value)
			{
				InsertAt(Buffer->NumElements, Value);
			}
			void Array::RemoveAt(size_t Index)
			{
				if (Index >= Buffer->NumElements)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Index out of bounds");
					return;
				}

				Resize(-1, Index);
			}
			void Array::RemoveLast()
			{
				RemoveAt(Buffer->NumElements - 1);
			}
			const void* Array::At(size_t Index) const
			{
				if (Buffer == 0 || Index >= Buffer->NumElements)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Index out of bounds");
					return 0;
				}

				if ((SubTypeId & asTYPEID_MASK_OBJECT) && !(SubTypeId & asTYPEID_OBJHANDLE))
					return *(void**)(Buffer->Data + (size_t)ElementSize * Index);
				else
					return Buffer->Data + (size_t)ElementSize * Index;
			}
			void* Array::At(size_t Index)
			{
				return const_cast<void*>(const_cast<const Array*>(this)->At(Index));
			}
			void* Array::GetBuffer()
			{
				return Buffer->Data;
			}
			void Array::CreateBuffer(SBuffer** BufferPtr, size_t NumElements)
			{
				*BufferPtr = reinterpret_cast<SBuffer*>(asAllocMem(sizeof(SBuffer) - 1 + (size_t)ElementSize * (size_t)NumElements));
				if (*BufferPtr)
				{
					(*BufferPtr)->NumElements = NumElements;
					(*BufferPtr)->MaxElements = NumElements;
					Construct(*BufferPtr, 0, NumElements);
				}
				else
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Out of memory");
				}
			}
			void Array::DeleteBuffer(SBuffer* BufferPtr)
			{
				Destruct(BufferPtr, 0, BufferPtr->NumElements);
				asFreeMem(BufferPtr);
			}
			void Array::Construct(SBuffer* BufferPtr, size_t Start, size_t End)
			{
				if ((SubTypeId & asTYPEID_MASK_OBJECT) && !(SubTypeId & asTYPEID_OBJHANDLE))
				{
					void** Max = (void**)(BufferPtr->Data + End * sizeof(void*));
					void** D = (void**)(BufferPtr->Data + Start * sizeof(void*));

					VMCManager* Engine = ObjType->GetEngine();
					VMCTypeInfo* SubType = ObjType->GetSubType();

					for (; D < Max; D++)
					{
						*D = (void*)Engine->CreateScriptObject(SubType);
						if (*D == 0)
						{
							memset(D, 0, sizeof(void*) * (Max - D));
							return;
						}
					}
				}
				else
				{
					void* D = (void*)(BufferPtr->Data + Start * (size_t)ElementSize);
					memset(D, 0, (size_t)(End - Start) * (size_t)ElementSize);
				}
			}
			void Array::Destruct(SBuffer* BufferPtr, size_t Start, size_t End)
			{
				if (SubTypeId & asTYPEID_MASK_OBJECT)
				{
					VMCManager* Engine = ObjType->GetEngine();
					void** Max = (void**)(BufferPtr->Data + End * sizeof(void*));
					void** D = (void**)(BufferPtr->Data + Start * sizeof(void*));

					for (; D < Max; D++)
					{
						if (*D)
							Engine->ReleaseScriptObject(*D, ObjType->GetSubType());
					}
				}
			}
			bool Array::Less(const void* A, const void* B, bool Asc, VMCContext* Context, SCache* Cache)
			{
				if (!Asc)
				{
					const void* Temp = A;
					A = B;
					B = Temp;
				}

				if (!(SubTypeId & ~asTYPEID_MASK_SEQNBR))
				{
					switch (SubTypeId)
					{
	#define COMPARE(T) *((T*)A) < *((T*)B)
						case asTYPEID_BOOL: return COMPARE(bool);
						case asTYPEID_INT8: return COMPARE(signed char);
						case asTYPEID_UINT8: return COMPARE(unsigned char);
						case asTYPEID_INT16: return COMPARE(signed short);
						case asTYPEID_UINT16: return COMPARE(unsigned short);
						case asTYPEID_INT32: return COMPARE(signed int);
						case asTYPEID_UINT32: return COMPARE(unsigned int);
						case asTYPEID_FLOAT: return COMPARE(float);
						case asTYPEID_DOUBLE: return COMPARE(double);
						default: return COMPARE(signed int);
	#undef COMPARE
					}
				}
				else
				{
					if (SubTypeId & asTYPEID_OBJHANDLE)
					{
						if (*(void**)A == 0)
							return true;

						if (*(void**)B == 0)
							return false;
					}

					if (Cache && Cache->CmpFunc)
					{
						Context->Prepare(Cache->CmpFunc);
						if (SubTypeId & asTYPEID_OBJHANDLE)
						{
							Context->SetObject(*((void**)A));
							Context->SetArgObject(0, *((void**)B));
						}
						else
						{
							Context->SetObject((void*)A);
							Context->SetArgObject(0, (void*)B);
						}

						if (Context->Execute() == asEXECUTION_FINISHED)
							return (int)Context->GetReturnDWord() < 0;
					}
				}

				return false;
			}
			void Array::Reverse()
			{
				size_t Size = GetSize();
				if (Size >= 2)
				{
					unsigned char Temp[16];
					for (size_t i = 0; i < Size / 2; i++)
					{
						Copy(Temp, GetArrayItemPointer((int)i));
						Copy(GetArrayItemPointer((int)i), GetArrayItemPointer((int)(Size - i - 1)));
						Copy(GetArrayItemPointer((int)(Size - i - 1)), Temp);
					}
				}
			}
			bool Array::operator==(const Array& Other) const
			{
				if (ObjType != Other.ObjType)
					return false;

				if (GetSize() != Other.GetSize())
					return false;

				VMCContext* CmpContext = 0;
				bool IsNested = false;

				if (SubTypeId & ~asTYPEID_MASK_SEQNBR)
				{
					CmpContext = asGetActiveContext();
					if (CmpContext)
					{
						if (CmpContext->GetEngine() == ObjType->GetEngine() && CmpContext->PushState() >= 0)
							IsNested = true;
						else
							CmpContext = 0;
					}

					if (CmpContext == 0)
						CmpContext = ObjType->GetEngine()->CreateContext();
				}

				bool IsEqual = true;
				SCache* Cache = reinterpret_cast<SCache*>(ObjType->GetUserData(ARRAY_CACHE));
				for (size_t n = 0; n < GetSize(); n++)
				{
					if (!Equals(At(n), Other.At(n), CmpContext, Cache))
					{
						IsEqual = false;
						break;
					}
				}

				if (CmpContext)
				{
					if (IsNested)
					{
						asEContextState State = CmpContext->GetState();
						CmpContext->PopState();
						if (State == asEXECUTION_ABORTED)
							CmpContext->Abort();
					}
					else
						CmpContext->Release();
				}

				return IsEqual;
			}
			bool Array::Equals(const void* A, const void* B, VMCContext* Context, SCache* Cache) const
			{
				if (!(SubTypeId & ~asTYPEID_MASK_SEQNBR))
				{
					switch (SubTypeId)
					{
	#define COMPARE(T) *((T*)A) == *((T*)B)
						case asTYPEID_BOOL: return COMPARE(bool);
						case asTYPEID_INT8: return COMPARE(signed char);
						case asTYPEID_UINT8: return COMPARE(unsigned char);
						case asTYPEID_INT16: return COMPARE(signed short);
						case asTYPEID_UINT16: return COMPARE(unsigned short);
						case asTYPEID_INT32: return COMPARE(signed int);
						case asTYPEID_UINT32: return COMPARE(unsigned int);
						case asTYPEID_FLOAT: return COMPARE(float);
						case asTYPEID_DOUBLE: return COMPARE(double);
						default: return COMPARE(signed int);
	#undef COMPARE
					}
				}
				else
				{
					if (SubTypeId & asTYPEID_OBJHANDLE)
					{
						if (*(void**)A == *(void**)B)
							return true;
					}

					if (Cache && Cache->EqFunc)
					{
						Context->Prepare(Cache->EqFunc);
						if (SubTypeId & asTYPEID_OBJHANDLE)
						{
							Context->SetObject(*((void**)A));
							Context->SetArgObject(0, *((void**)B));
						}
						else
						{
							Context->SetObject((void*)A);
							Context->SetArgObject(0, (void*)B);
						}

						if (Context->Execute() == asEXECUTION_FINISHED)
							return Context->GetReturnByte() != 0;

						return false;
					}

					if (Cache && Cache->CmpFunc)
					{
						Context->Prepare(Cache->CmpFunc);
						if (SubTypeId & asTYPEID_OBJHANDLE)
						{
							Context->SetObject(*((void**)A));
							Context->SetArgObject(0, *((void**)B));
						}
						else
						{
							Context->SetObject((void*)A);
							Context->SetArgObject(0, (void*)B);
						}

						if (Context->Execute() == asEXECUTION_FINISHED)
							return (int)Context->GetReturnDWord() == 0;

						return false;
					}
				}

				return false;
			}
			int Array::FindByRef(void* RefPtr) const
			{
				return FindByRef(0, RefPtr);
			}
			int Array::FindByRef(size_t StartAt, void* RefPtr) const
			{
				size_t Size = GetSize();
				if (SubTypeId & asTYPEID_OBJHANDLE)
				{
					RefPtr = *(void**)RefPtr;
					for (size_t i = StartAt; i < Size; i++)
					{
						if (*(void**)At(i) == RefPtr)
							return (int)i;
					}
				}
				else
				{
					for (size_t i = StartAt; i < Size; i++)
					{
						if (At(i) == RefPtr)
							return (int)i;
					}
				}

				return -1;
			}
			int Array::Find(void* Value) const
			{
				return Find(0, Value);
			}
			int Array::Find(size_t StartAt, void* Value) const
			{
				SCache* Cache = 0;
				if (SubTypeId & ~asTYPEID_MASK_SEQNBR)
				{
					Cache = reinterpret_cast<SCache*>(ObjType->GetUserData(ARRAY_CACHE));
					if (!Cache || (Cache->CmpFunc == 0 && Cache->EqFunc == 0))
					{
						VMCContext* Context = asGetActiveContext();
						VMCTypeInfo* SubType = ObjType->GetEngine()->GetTypeInfoById(SubTypeId);
						if (Context)
						{
							char Swap[512];
							if (Cache && Cache->EqFuncReturnCode == asMULTIPLE_FUNCTIONS)
								snprintf(Swap, 512, "Type '%s' has multiple matching opEquals or opCmp methods", SubType->GetName());
							else
								snprintf(Swap, 512, "Type '%s' does not have a matching opEquals or opCmp method", SubType->GetName());
							Context->SetException(Swap);
						}

						return -1;
					}
				}

				VMCContext* CmpContext = 0;
				bool IsNested = false;

				if (SubTypeId & ~asTYPEID_MASK_SEQNBR)
				{
					CmpContext = asGetActiveContext();
					if (CmpContext)
					{
						if (CmpContext->GetEngine() == ObjType->GetEngine() && CmpContext->PushState() >= 0)
							IsNested = true;
						else
							CmpContext = 0;
					}

					if (CmpContext == 0)
						CmpContext = ObjType->GetEngine()->CreateContext();
				}

				int Result = -1;
				size_t Size = GetSize();
				for (size_t i = StartAt; i < Size; i++)
				{
					if (Equals(At(i), Value, CmpContext, Cache))
					{
						Result = (int)i;
						break;
					}
				}

				if (CmpContext)
				{
					if (IsNested)
					{
						asEContextState State = CmpContext->GetState();
						CmpContext->PopState();
						if (State == asEXECUTION_ABORTED)
							CmpContext->Abort();
					}
					else
						CmpContext->Release();
				}

				return Result;
			}
			void Array::Copy(void* Dest, void* Src)
			{
				memcpy(Dest, Src, ElementSize);
			}
			void* Array::GetArrayItemPointer(int Index)
			{
				return Buffer->Data + Index * ElementSize;
			}
			void* Array::GetDataPointer(void* BufferPtr)
			{
				if ((SubTypeId & asTYPEID_MASK_OBJECT) && !(SubTypeId & asTYPEID_OBJHANDLE))
					return reinterpret_cast<void*>(*(size_t*)BufferPtr);
				else
					return BufferPtr;
			}
			void Array::SortAsc()
			{
				Sort(0, GetSize(), true);
			}
			void Array::SortAsc(size_t StartAt, size_t Count)
			{
				Sort(StartAt, Count, true);
			}
			void Array::SortDesc()
			{
				Sort(0, GetSize(), false);
			}
			void Array::SortDesc(size_t StartAt, size_t Count)
			{
				Sort(StartAt, Count, false);
			}
			void Array::Sort(size_t StartAt, size_t Count, bool Asc)
			{
				SCache* Cache = reinterpret_cast<SCache*>(ObjType->GetUserData(ARRAY_CACHE));
				if (SubTypeId & ~asTYPEID_MASK_SEQNBR)
				{
					if (!Cache || Cache->CmpFunc == 0)
					{
						VMCContext* Context = asGetActiveContext();
						VMCTypeInfo* SubType = ObjType->GetEngine()->GetTypeInfoById(SubTypeId);
						if (Context)
						{
							char Swap[512];
							if (Cache && Cache->CmpFuncReturnCode == asMULTIPLE_FUNCTIONS)
								snprintf(Swap, 512, "Type '%s' has multiple matching opCmp methods", SubType->GetName());
							else
								snprintf(Swap, 512, "Type '%s' does not have a matching opCmp method", SubType->GetName());
							Context->SetException(Swap);
						}

						return;
					}
				}

				if (Count < 2)
					return;

				int Start = (int)StartAt;
				int End = (int)StartAt + (int)Count;

				if (Start >= (int)Buffer->NumElements || End > (int)Buffer->NumElements)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Index out of bounds");

					return;
				}

				unsigned char Swap[16];
				VMCContext* CmpContext = 0;
				bool IsNested = false;

				if (SubTypeId & ~asTYPEID_MASK_SEQNBR)
				{
					CmpContext = asGetActiveContext();
					if (CmpContext)
					{
						if (CmpContext->GetEngine() == ObjType->GetEngine() && CmpContext->PushState() >= 0)
							IsNested = true;
						else
							CmpContext = 0;
					}

					if (CmpContext == 0)
						CmpContext = ObjType->GetEngine()->RequestContext();
				}

				for (int i = Start + 1; i < End; i++)
				{
					Copy(Swap, GetArrayItemPointer(i));
					int j = i - 1;

					while (j >= Start && Less(GetDataPointer(Swap), At(j), Asc, CmpContext, Cache))
					{
						Copy(GetArrayItemPointer(j + 1), GetArrayItemPointer(j));
						j--;
					}

					Copy(GetArrayItemPointer(j + 1), Swap);
				}

				if (CmpContext)
				{
					if (IsNested)
					{
						asEContextState State = CmpContext->GetState();
						CmpContext->PopState();
						if (State == asEXECUTION_ABORTED)
							CmpContext->Abort();
					}
					else
						ObjType->GetEngine()->ReturnContext(CmpContext);
				}
			}
			void Array::Sort(asIScriptFunction* Function, size_t StartAt, size_t Count)
			{
				if (Count < 2)
					return;

				size_t Start = StartAt;
				size_t End = as_uint64_t(StartAt) + as_uint64_t(Count) >= (as_uint64_t(1) << 32) ? 0xFFFFFFFF : StartAt + Count;
				if (End > Buffer->NumElements)
					End = Buffer->NumElements;

				if (Start >= Buffer->NumElements)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Index out of bounds");

					return;
				}

				unsigned char Swap[16];
				VMCContext* CmpContext = 0;
				bool IsNested = false;

				CmpContext = asGetActiveContext();
				if (CmpContext)
				{
					if (CmpContext->GetEngine() == ObjType->GetEngine() && CmpContext->PushState() >= 0)
						IsNested = true;
					else
						CmpContext = 0;
				}

				if (CmpContext == 0)
					CmpContext = ObjType->GetEngine()->RequestContext();

				if (!CmpContext)
					return;

				for (size_t i = Start + 1; i < End; i++)
				{
					Copy(Swap, GetArrayItemPointer((int)i));
					size_t j = i - 1;

					while (j != 0xFFFFFFFF && j >= Start)
					{
						CmpContext->Prepare(Function);
						CmpContext->SetArgAddress(0, GetDataPointer(Swap));
						CmpContext->SetArgAddress(1, At(j));
						int Result = CmpContext->Execute();
						if (Result != asEXECUTION_FINISHED)
							break;

						if (*(bool*)(CmpContext->GetAddressOfReturnValue()))
						{
							Copy(GetArrayItemPointer((int)j + 1), GetArrayItemPointer((int)j));
							j--;
						}
						else
							break;
					}

					Copy(GetArrayItemPointer((int)j + 1), Swap);
				}

				if (IsNested)
				{
					asEContextState State = CmpContext->GetState();
					CmpContext->PopState();
					if (State == asEXECUTION_ABORTED)
						CmpContext->Abort();
				}
				else
					ObjType->GetEngine()->ReturnContext(CmpContext);
			}
			void Array::CopyBuffer(SBuffer* Dest, SBuffer* Src)
			{
				VMCManager* Engine = ObjType->GetEngine();
				if (SubTypeId & asTYPEID_OBJHANDLE)
				{
					if (Dest->NumElements > 0 && Src->NumElements > 0)
					{
						int Count = (int)(Dest->NumElements > Src->NumElements ? Src->NumElements : Dest->NumElements);
						void** Max = (void**)(Dest->Data + Count * sizeof(void*));
						void** D = (void**)Dest->Data;
						void** S = (void**)Src->Data;

						for (; D < Max; D++, S++)
						{
							void* Swap = *D;
							*D = *S;

							if (*D)
								Engine->AddRefScriptObject(*D, ObjType->GetSubType());

							if (Swap)
								Engine->ReleaseScriptObject(Swap, ObjType->GetSubType());
						}
					}
				}
				else
				{
					if (Dest->NumElements > 0 && Src->NumElements > 0)
					{
						int Count = (int)(Dest->NumElements > Src->NumElements ? Src->NumElements : Dest->NumElements);
						if (SubTypeId & asTYPEID_MASK_OBJECT)
						{
							void** Max = (void**)(Dest->Data + Count * sizeof(void*));
							void** D = (void**)Dest->Data;
							void** s = (void**)Src->Data;

							VMCTypeInfo* SubType = ObjType->GetSubType();
							for (; D < Max; D++, s++)
								Engine->AssignScriptObject(*D, *s, SubType);
						}
						else
							memcpy(Dest->Data, Src->Data, (size_t)Count * (size_t)ElementSize);
					}
				}
			}
			void Array::Precache()
			{
				SubTypeId = ObjType->GetSubTypeId();
				if (!(SubTypeId & ~asTYPEID_MASK_SEQNBR))
					return;

				SCache* Cache = reinterpret_cast<SCache*>(ObjType->GetUserData(ARRAY_CACHE));
				if (Cache)
					return;

				asAcquireExclusiveLock();
				Cache = reinterpret_cast<SCache*>(ObjType->GetUserData(ARRAY_CACHE));
				if (Cache)
				{
					asReleaseExclusiveLock();
					return;
				}

				Cache = reinterpret_cast<SCache*>(asAllocMem(sizeof(SCache)));
				if (!Cache)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Out of memory");

					asReleaseExclusiveLock();
					return;
				}

				memset(Cache, 0, sizeof(SCache));
				bool MustBeConst = (SubTypeId & asTYPEID_HANDLETOCONST) ? true : false;

				VMCTypeInfo* SubType = ObjType->GetEngine()->GetTypeInfoById(SubTypeId);
				if (SubType)
				{
					for (size_t i = 0; i < SubType->GetMethodCount(); i++)
					{
						asIScriptFunction* Function = SubType->GetMethodByIndex((int)i);
						if (Function->GetParamCount() == 1 && (!MustBeConst || Function->IsReadOnly()))
						{
							asDWORD Flags = 0;
							int ReturnTypeId = Function->GetReturnTypeId(&Flags);
							if (Flags != asTM_NONE)
								continue;

							bool IsCmp = false, IsEquals = false;
							if (ReturnTypeId == asTYPEID_INT32 && strcmp(Function->GetName(), "opCmp") == 0)
								IsCmp = true;
							if (ReturnTypeId == asTYPEID_BOOL && strcmp(Function->GetName(), "opEquals") == 0)
								IsEquals = true;

							if (!IsCmp && !IsEquals)
								continue;

							int ParamTypeId;
							Function->GetParam(0, &ParamTypeId, &Flags);

							if ((ParamTypeId & ~(asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST)) != (SubTypeId & ~(asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST)))
								continue;

							if ((Flags & asTM_INREF))
							{
								if ((ParamTypeId & asTYPEID_OBJHANDLE) || (MustBeConst && !(Flags & asTM_CONST)))
									continue;
							}
							else if (ParamTypeId & asTYPEID_OBJHANDLE)
							{
								if (MustBeConst && !(ParamTypeId & asTYPEID_HANDLETOCONST))
									continue;
							}
							else
								continue;

							if (IsCmp)
							{
								if (Cache->CmpFunc || Cache->CmpFuncReturnCode)
								{
									Cache->CmpFunc = 0;
									Cache->CmpFuncReturnCode = asMULTIPLE_FUNCTIONS;
								}
								else
									Cache->CmpFunc = Function;
							}
							else if (IsEquals)
							{
								if (Cache->EqFunc || Cache->EqFuncReturnCode)
								{
									Cache->EqFunc = 0;
									Cache->EqFuncReturnCode = asMULTIPLE_FUNCTIONS;
								}
								else
									Cache->EqFunc = Function;
							}
						}
					}
				}

				if (Cache->EqFunc == 0 && Cache->EqFuncReturnCode == 0)
					Cache->EqFuncReturnCode = asNO_FUNCTION;
				if (Cache->CmpFunc == 0 && Cache->CmpFuncReturnCode == 0)
					Cache->CmpFuncReturnCode = asNO_FUNCTION;

				ObjType->SetUserData(Cache, ARRAY_CACHE);
				asReleaseExclusiveLock();
			}
			void Array::EnumReferences(VMCManager* Engine)
			{
				if (SubTypeId & asTYPEID_MASK_OBJECT)
				{
					void** D = (void**)Buffer->Data;
					VMCTypeInfo* SubType = Engine->GetTypeInfoById(SubTypeId);
					if ((SubType->GetFlags() & asOBJ_REF))
					{
						for (size_t n = 0; n < Buffer->NumElements; n++)
						{
							if (D[n])
								Engine->GCEnumCallback(D[n]);
						}
					}
					else if ((SubType->GetFlags() & asOBJ_VALUE) && (SubType->GetFlags() & asOBJ_GC))
					{
						for (size_t n = 0; n < Buffer->NumElements; n++)
						{
							if (D[n])
								Engine->ForwardGCEnumReferences(D[n], SubType);
						}
					}
				}
			}
			void Array::ReleaseAllHandles(VMCManager*)
			{
				Resize(0);
			}
			void Array::AddRef() const
			{
				GCFlag = false;
				asAtomicInc(RefCount);
			}
			void Array::Release() const
			{
				GCFlag = false;
				if (asAtomicDec(RefCount) == 0)
				{
					this->~Array();
					asFreeMem(const_cast<Array*>(this));
				}
			}
			int Array::GetRefCount()
			{
				return RefCount;
			}
			void Array::SetFlag()
			{
				GCFlag = true;
			}
			bool Array::GetFlag()
			{
				return GCFlag;
			}
			Array* Array::Create(VMCTypeInfo* Info, size_t Length)
			{
				void* Memory = asAllocMem(sizeof(Array));
				if (Memory == 0)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Out of memory");

					return 0;
				}

				Array* a = new(Memory) Array(Length, Info);
				return a;
			}
			Array* Array::Create(VMCTypeInfo* Info, void* InitList)
			{
				void* Memory = asAllocMem(sizeof(Array));
				if (Memory == 0)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Out of memory");

					return 0;
				}

				Array* a = new(Memory) Array(Info, InitList);
				return a;
			}
			Array* Array::Create(VMCTypeInfo* Info, size_t length, void* DefaultValue)
			{
				void* Memory = asAllocMem(sizeof(Array));
				if (Memory == 0)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Out of memory");

					return 0;
				}

				Array* a = new(Memory) Array(length, DefaultValue, Info);
				return a;
			}
			Array* Array::Create(VMCTypeInfo* Info)
			{
				return Array::Create(Info, size_t(0));
			}
			void Array::CleanupTypeInfoCache(VMCTypeInfo* Type)
			{
				Array::SCache* Cache = reinterpret_cast<Array::SCache*>(Type->GetUserData(ARRAY_CACHE));
				if (Cache != nullptr)
				{
					Cache->~SCache();
					asFreeMem(Cache);
				}
			}
			bool Array::TemplateCallback(VMCTypeInfo* Info, bool& DontGarbageCollect)
			{
				int TypeId = Info->GetSubTypeId();
				if (TypeId == asTYPEID_VOID)
					return false;

				if ((TypeId & asTYPEID_MASK_OBJECT) && !(TypeId & asTYPEID_OBJHANDLE))
				{
					VMCManager* Engine = Info->GetEngine();
					VMCTypeInfo* SubType = Engine->GetTypeInfoById(TypeId);
					asDWORD Flags = SubType->GetFlags();

					if ((Flags & asOBJ_VALUE) && !(Flags & asOBJ_POD))
					{
						bool Found = false;
						for (size_t n = 0; n < SubType->GetBehaviourCount(); n++)
						{
							asEBehaviours Beh;
							asIScriptFunction* Func = SubType->GetBehaviourByIndex((int)n, &Beh);
							if (Beh != asBEHAVE_CONSTRUCT)
								continue;

							if (Func->GetParamCount() == 0)
							{
								Found = true;
								break;
							}
						}

						if (!Found)
						{
							Engine->WriteMessage(TYPENAME_ARRAY, 0, 0, asMSGTYPE_ERROR, "The subtype has no default constructor");
							return false;
						}
					}
					else if ((Flags & asOBJ_REF))
					{
						bool Found = false;
						if (!Engine->GetEngineProperty(asEP_DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE))
						{
							for (size_t n = 0; n < SubType->GetFactoryCount(); n++)
							{
								asIScriptFunction* Func = SubType->GetFactoryByIndex((int)n);
								if (Func->GetParamCount() == 0)
								{
									Found = true;
									break;
								}
							}
						}

						if (!Found)
						{
							Engine->WriteMessage(TYPENAME_ARRAY, 0, 0, asMSGTYPE_ERROR, "The subtype has no default factory");
							return false;
						}
					}

					if (!(Flags & asOBJ_GC))
						DontGarbageCollect = true;
				}
				else if (!(TypeId & asTYPEID_OBJHANDLE))
				{
					DontGarbageCollect = true;
				}
				else
				{
					VMCTypeInfo* SubType = Info->GetEngine()->GetTypeInfoById(TypeId);
					asDWORD Flags = SubType->GetFlags();

					if (!(Flags & asOBJ_GC))
					{
						if ((Flags & asOBJ_SCRIPT_OBJECT))
						{
							if ((Flags & asOBJ_NOINHERIT))
								DontGarbageCollect = true;
						}
						else
							DontGarbageCollect = true;
					}
				}

				return true;
			}

			MapKey::MapKey() noexcept
			{
				ValueObj = 0;
				TypeId = 0;
			}
			MapKey::MapKey(VMCManager* Engine, void* Value, int _TypeId) noexcept
			{
				ValueObj = 0;
				TypeId = 0;
				Set(Engine, Value, _TypeId);
			}
			MapKey::~MapKey() noexcept
			{
				if (ValueObj && TypeId)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context != nullptr)
						FreeValue(Context->GetEngine());
				}
			}
			void MapKey::FreeValue(VMCManager* Engine)
			{
				if (TypeId & asTYPEID_MASK_OBJECT)
				{
					Engine->ReleaseScriptObject(ValueObj, Engine->GetTypeInfoById(TypeId));
					ValueObj = 0;
					TypeId = 0;
				}
			}
			void MapKey::EnumReferences(VMCManager* _Engine)
			{
				if (ValueObj)
					_Engine->GCEnumCallback(ValueObj);

				if (TypeId)
					_Engine->GCEnumCallback(_Engine->GetTypeInfoById(TypeId));
			}
			void MapKey::Set(VMCManager* Engine, void* Value, int _TypeId)
			{
				FreeValue(Engine);
				TypeId = _TypeId;

				if (TypeId & asTYPEID_OBJHANDLE)
				{
					ValueObj = *(void**)Value;
					Engine->AddRefScriptObject(ValueObj, Engine->GetTypeInfoById(TypeId));
				}
				else if (TypeId & asTYPEID_MASK_OBJECT)
				{
					ValueObj = Engine->CreateScriptObjectCopy(Value, Engine->GetTypeInfoById(TypeId));
					if (ValueObj == 0)
					{
						VMCContext* Context = asGetActiveContext();
						if (Context)
							Context->SetException("Cannot create copy of object");
					}
				}
				else
				{
					int Size = Engine->GetSizeOfPrimitiveType(TypeId);
					memcpy(&ValueInt, Value, Size);
				}
			}
			void MapKey::Set(VMCManager* Engine, MapKey& Value)
			{
				if (Value.TypeId & asTYPEID_OBJHANDLE)
					Set(Engine, (void*)&Value.ValueObj, Value.TypeId);
				else if (Value.TypeId & asTYPEID_MASK_OBJECT)
					Set(Engine, (void*)Value.ValueObj, Value.TypeId);
				else
					Set(Engine, (void*)&Value.ValueInt, Value.TypeId);
			}
			bool MapKey::Get(VMCManager* Engine, void* Value, int _TypeId) const
			{
				if (_TypeId & asTYPEID_OBJHANDLE)
				{
					if ((_TypeId & asTYPEID_MASK_OBJECT))
					{
						if ((TypeId & asTYPEID_HANDLETOCONST) && !(_TypeId & asTYPEID_HANDLETOCONST))
							return false;

						Engine->RefCastObject(ValueObj, Engine->GetTypeInfoById(TypeId), Engine->GetTypeInfoById(_TypeId), reinterpret_cast<void**>(Value));
						return true;
					}
				}
				else if (_TypeId & asTYPEID_MASK_OBJECT)
				{
					bool isCompatible = false;
					if ((TypeId & ~(asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST)) == _TypeId && ValueObj != 0)
						isCompatible = true;

					if (isCompatible)
					{
						Engine->AssignScriptObject(Value, ValueObj, Engine->GetTypeInfoById(TypeId));
						return true;
					}
				}
				else
				{
					if (TypeId == _TypeId)
					{
						int Size = Engine->GetSizeOfPrimitiveType(_TypeId);
						memcpy(Value, &ValueInt, Size);
						return true;
					}

					if (_TypeId == asTYPEID_DOUBLE)
					{
						if (TypeId == asTYPEID_INT64)
						{
							*(double*)Value = double(ValueInt);
						}
						else if (TypeId == asTYPEID_BOOL)
						{
							char localValue;
							memcpy(&localValue, &ValueInt, sizeof(char));
							*(double*)Value = localValue ? 1.0 : 0.0;
						}
						else if (TypeId > asTYPEID_DOUBLE && (TypeId & asTYPEID_MASK_OBJECT) == 0)
						{
							int localValue;
							memcpy(&localValue, &ValueInt, sizeof(int));
							*(double*)Value = double(localValue);
						}
						else
						{
							*(double*)Value = 0;
							return false;
						}

						return true;
					}
					else if (_TypeId == asTYPEID_INT64)
					{
						if (TypeId == asTYPEID_DOUBLE)
						{
							*(as_int64_t*)Value = as_int64_t(ValueFlt);
						}
						else if (TypeId == asTYPEID_BOOL)
						{
							char localValue;
							memcpy(&localValue, &ValueInt, sizeof(char));
							*(as_int64_t*)Value = localValue ? 1 : 0;
						}
						else if (TypeId > asTYPEID_DOUBLE && (TypeId & asTYPEID_MASK_OBJECT) == 0)
						{
							int localValue;
							memcpy(&localValue, &ValueInt, sizeof(int));
							*(as_int64_t*)Value = localValue;
						}
						else
						{
							*(as_int64_t*)Value = 0;
							return false;
						}

						return true;
					}
					else if (_TypeId > asTYPEID_DOUBLE && (TypeId & asTYPEID_MASK_OBJECT) == 0)
					{
						if (TypeId == asTYPEID_DOUBLE)
						{
							*(int*)Value = int(ValueFlt);
						}
						else if (TypeId == asTYPEID_INT64)
						{
							*(int*)Value = int(ValueInt);
						}
						else if (TypeId == asTYPEID_BOOL)
						{
							char localValue;
							memcpy(&localValue, &ValueInt, sizeof(char));
							*(int*)Value = localValue ? 1 : 0;
						}
						else if (TypeId > asTYPEID_DOUBLE && (TypeId & asTYPEID_MASK_OBJECT) == 0)
						{
							int localValue;
							memcpy(&localValue, &ValueInt, sizeof(int));
							*(int*)Value = localValue;
						}
						else
						{
							*(int*)Value = 0;
							return false;
						}

						return true;
					}
					else if (_TypeId == asTYPEID_BOOL)
					{
						if (TypeId & asTYPEID_OBJHANDLE)
						{
							*(bool*)Value = ValueObj ? true : false;
						}
						else if (TypeId & asTYPEID_MASK_OBJECT)
						{
							*(bool*)Value = true;
						}
						else
						{
							as_uint64_t Zero = 0;
							int Size = Engine->GetSizeOfPrimitiveType(TypeId);
							*(bool*)Value = memcmp(&ValueInt, &Zero, Size) == 0 ? false : true;
						}

						return true;
					}
				}

				return false;
			}
			const void* MapKey::GetAddressOfValue() const
			{
				if ((TypeId & asTYPEID_MASK_OBJECT) && !(TypeId & asTYPEID_OBJHANDLE))
					return ValueObj;

				return reinterpret_cast<const void*>(&ValueObj);
			}
			int MapKey::GetTypeId() const
			{
				return TypeId;
			}

			Map::LocalIterator::LocalIterator(const Map& Dict, InternalMap::const_iterator It) noexcept : It(It), Dict(Dict)
			{
			}
			void Map::LocalIterator::operator++()
			{
				++It;
			}
			void Map::LocalIterator::operator++(int)
			{
				++It;
			}
			Map::LocalIterator& Map::LocalIterator::operator*()
			{
				return *this;
			}
			bool Map::LocalIterator::operator==(const LocalIterator& Other) const
			{
				return It == Other.It;
			}
			bool Map::LocalIterator::operator!=(const LocalIterator& Other) const
			{
				return It != Other.It;
			}
			const std::string& Map::LocalIterator::GetKey() const
			{
				return It->first;
			}
			int Map::LocalIterator::GetTypeId() const
			{
				return It->second.TypeId;
			}
			bool Map::LocalIterator::GetValue(void* Value, int TypeId) const
			{
				return It->second.Get(Dict.Engine, Value, TypeId);
			}
			const void* Map::LocalIterator::GetAddressOfValue() const
			{
				return It->second.GetAddressOfValue();
			}

			Map::Map(VMCManager* _Engine) noexcept
			{
				Init(_Engine);
			}
			Map::Map(unsigned char* buffer) noexcept
			{
				VMCContext* Context = asGetActiveContext();
				Init(Context->GetEngine());

				Map::SCache& Cache = *reinterpret_cast<Map::SCache*>(Engine->GetUserData(MAP_CACHE));
				bool keyAsRef = Cache.KeyType->GetFlags() & asOBJ_REF ? true : false;

				size_t length = *(size_t*)buffer;
				buffer += 4;

				while (length--)
				{
					if (asPWORD(buffer) & 0x3)
						buffer += 4 - (asPWORD(buffer) & 0x3);

					std::string name;
					if (keyAsRef)
					{
						name = **(std::string**)buffer;
						buffer += sizeof(std::string*);
					}
					else
					{
						name = *(std::string*)buffer;
						buffer += sizeof(std::string);
					}

					int TypeId = *(int*)buffer;
					buffer += sizeof(int);

					void* RefPtr = (void*)buffer;
					if (TypeId >= asTYPEID_INT8 && TypeId <= asTYPEID_DOUBLE)
					{
						as_int64_t i64;
						double D;

						switch (TypeId)
						{
							case asTYPEID_INT8:
								i64 = *(char*)RefPtr;
								break;
							case asTYPEID_INT16:
								i64 = *(short*)RefPtr;
								break;
							case asTYPEID_INT32:
								i64 = *(int*)RefPtr;
								break;
							case asTYPEID_UINT8:
								i64 = *(unsigned char*)RefPtr;
								break;
							case asTYPEID_UINT16:
								i64 = *(unsigned short*)RefPtr;
								break;
							case asTYPEID_UINT32:
								i64 = *(unsigned int*)RefPtr;
								break;
							case asTYPEID_INT64:
							case asTYPEID_UINT64:
								i64 = *(as_int64_t*)RefPtr;
								break;
							case asTYPEID_FLOAT:
								D = *(float*)RefPtr;
								break;
							case asTYPEID_DOUBLE:
								D = *(double*)RefPtr;
								break;
						}

						if (TypeId >= asTYPEID_FLOAT)
							Set(name, &D, asTYPEID_DOUBLE);
						else
							Set(name, &i64, asTYPEID_INT64);
					}
					else
					{
						if ((TypeId & asTYPEID_MASK_OBJECT) && !(TypeId & asTYPEID_OBJHANDLE) && (Engine->GetTypeInfoById(TypeId)->GetFlags() & asOBJ_REF))
							RefPtr = *(void**)RefPtr;

						Set(name, RefPtr, VMManager::Get(Engine)->IsNullable(TypeId) ? 0 : TypeId);
					}

					if (TypeId & asTYPEID_MASK_OBJECT)
					{
						VMCTypeInfo* Info = Engine->GetTypeInfoById(TypeId);
						if (Info->GetFlags() & asOBJ_VALUE)
							buffer += Info->GetSize();
						else
							buffer += sizeof(void*);
					}
					else if (TypeId == 0)
						buffer += sizeof(void*);
					else
						buffer += Engine->GetSizeOfPrimitiveType(TypeId);
				}
			}
			Map::Map(const Map& Other) noexcept
			{
				Init(Other.Engine);
				for (auto It = Other.Dict.begin(); It != Other.Dict.end(); ++It)
				{
					auto& Key = It->second;
					if (Key.TypeId & asTYPEID_OBJHANDLE)
						Set(It->first, (void*)&Key.ValueObj, Key.TypeId);
					else if (Key.TypeId & asTYPEID_MASK_OBJECT)
						Set(It->first, (void*)Key.ValueObj, Key.TypeId);
					else
						Set(It->first, (void*)&Key.ValueInt, Key.TypeId);
				}
			}
			Map::~Map() noexcept
			{
				DeleteAll();
			}
			void Map::AddRef() const
			{
				GCFlag = false;
				asAtomicInc(RefCount);
			}
			void Map::Release() const
			{
				GCFlag = false;
				if (asAtomicDec(RefCount) == 0)
				{
					this->~Map();
					asFreeMem(const_cast<Map*>(this));
				}
			}
			int Map::GetRefCount()
			{
				return RefCount;
			}
			void Map::SetGCFlag()
			{
				GCFlag = true;
			}
			bool Map::GetGCFlag()
			{
				return GCFlag;
			}
			void Map::EnumReferences(VMCManager* _Engine)
			{
				for (auto It = Dict.begin(); It != Dict.end(); ++It)
				{
					auto& Key = It->second;
					if (Key.TypeId & asTYPEID_MASK_OBJECT)
					{
						VMCTypeInfo* SubType = Engine->GetTypeInfoById(Key.TypeId);
						if ((SubType->GetFlags() & asOBJ_VALUE) && (SubType->GetFlags() & asOBJ_GC))
							Engine->ForwardGCEnumReferences(Key.ValueObj, SubType);
						else
							_Engine->GCEnumCallback(Key.ValueObj);
					}
				}
			}
			void Map::ReleaseAllReferences(VMCManager*)
			{
				DeleteAll();
			}
			Map& Map::operator =(const Map& Other) noexcept
			{
				DeleteAll();
				for (auto It = Other.Dict.begin(); It != Other.Dict.end(); ++It)
				{
					auto& Key = It->second;
					if (Key.TypeId & asTYPEID_OBJHANDLE)
						Set(It->first, (void*)&Key.ValueObj, Key.TypeId);
					else if (Key.TypeId & asTYPEID_MASK_OBJECT)
						Set(It->first, (void*)Key.ValueObj, Key.TypeId);
					else
						Set(It->first, (void*)&Key.ValueInt, Key.TypeId);
				}

				return *this;
			}
			MapKey* Map::operator[](const std::string& Key)
			{
				return &Dict[Key];
			}
			const MapKey* Map::operator[](const std::string& Key) const
			{
				auto It = Dict.find(Key);
				if (It != Dict.end())
					return &It->second;

				VMCContext* Context = asGetActiveContext();
				if (Context)
					Context->SetException("Invalid access to non-existing Value");

				return 0;
			}
			void Map::Set(const std::string& Key, void* Value, int TypeId)
			{
				auto It = Dict.find(Key);
				if (It == Dict.end())
					It = Dict.insert(InternalMap::value_type(Key, MapKey())).first;

				It->second.Set(Engine, Value, TypeId);
			}
			bool Map::Get(const std::string& Key, void* Value, int TypeId) const
			{
				auto It = Dict.find(Key);
				if (It != Dict.end())
					return It->second.Get(Engine, Value, TypeId);

				return false;
			}
			bool Map::GetIndex(size_t Index, std::string* Key, void** Value, int* TypeId) const
			{
				if (Index >= Dict.size())
					return false;

				auto It = Begin();
				size_t Offset = 0;

				while (Offset != Index)
				{
					++Offset;
					++It;
				}

				if (Key != nullptr)
					*Key = It.GetKey();

				if (Value != nullptr)
					*Value = (void*)It.GetAddressOfValue();

				if (TypeId != nullptr)
					*TypeId = It.GetTypeId();

				return true;
			}
			int Map::GetTypeId(const std::string& Key) const
			{
				auto It = Dict.find(Key);
				if (It != Dict.end())
					return It->second.TypeId;

				return -1;
			}
			bool Map::Exists(const std::string& Key) const
			{
				auto It = Dict.find(Key);
				if (It != Dict.end())
					return true;

				return false;
			}
			bool Map::IsEmpty() const
			{
				if (Dict.size() == 0)
					return true;

				return false;
			}
			size_t Map::GetSize() const
			{
				return size_t(Dict.size());
			}
			bool Map::Delete(const std::string& Key)
			{
				auto It = Dict.find(Key);
				if (It != Dict.end())
				{
					It->second.FreeValue(Engine);
					Dict.erase(It);
					return true;
				}

				return false;
			}
			void Map::DeleteAll()
			{
				for (auto It = Dict.begin(); It != Dict.end(); ++It)
					It->second.FreeValue(Engine);

				Dict.clear();
			}
			Array* Map::GetKeys() const
			{
				Map::SCache* Cache = reinterpret_cast<Map::SCache*>(Engine->GetUserData(MAP_CACHE));
				VMCTypeInfo* Info = Cache->ArrayType;

				Array* Array = Array::Create(Info, size_t(Dict.size()));
				long Current = -1;

				for (auto It = Dict.begin(); It != Dict.end(); ++It)
				{
					Current++;
					*(std::string*)Array->At((unsigned int)Current) = It->first;
				}

				return Array;
			}
			Map::LocalIterator Map::Begin() const
			{
				return LocalIterator(*this, Dict.begin());
			}
			Map::LocalIterator Map::End() const
			{
				return LocalIterator(*this, Dict.end());
			}
			Map::LocalIterator Map::Find(const std::string& Key) const
			{
				return LocalIterator(*this, Dict.find(Key));
			}
			Map* Map::Create(VMCManager* Engine)
			{
				Map* Obj = (Map*)asAllocMem(sizeof(Map));
				new(Obj) Map(Engine);
				return Obj;
			}
			Map* Map::Create(unsigned char* buffer)
			{
				Map* Obj = (Map*)asAllocMem(sizeof(Map));
				new(Obj) Map(buffer);
				return Obj;
			}
			void Map::Init(VMCManager* e)
			{
				RefCount = 1;
				GCFlag = false;
				Engine = e;

				Map::SCache* Cache = reinterpret_cast<Map::SCache*>(Engine->GetUserData(MAP_CACHE));
				Engine->NotifyGarbageCollectorOfNewObject(this, Cache->DictType);
			}
			void Map::Cleanup(VMCManager* Engine)
			{
				Map::SCache* Cache = reinterpret_cast<Map::SCache*>(Engine->GetUserData(MAP_CACHE));
				ED_DELETE(SCache, Cache);
			}
			void Map::Setup(VMCManager* Engine)
			{
				Map::SCache* Cache = reinterpret_cast<Map::SCache*>(Engine->GetUserData(MAP_CACHE));
				if (Cache == 0)
				{
					Cache = ED_NEW(Map::SCache);
					Engine->SetUserData(Cache, MAP_CACHE);
					Engine->SetEngineUserDataCleanupCallback(Cleanup, MAP_CACHE);

					Cache->DictType = Engine->GetTypeInfoByName(TYPENAME_MAP);
					Cache->ArrayType = Engine->GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">");
					Cache->KeyType = Engine->GetTypeInfoByDecl(TYPENAME_STRING);
				}
			}
			void Map::Factory(VMCGeneric* gen)
			{
				*(Map**)gen->GetAddressOfReturnLocation() = Map::Create(gen->GetEngine());
			}
			void Map::ListFactory(VMCGeneric* gen)
			{
				unsigned char* buffer = (unsigned char*)gen->GetArgAddress(0);
				*(Map**)gen->GetAddressOfReturnLocation() = Map::Create(buffer);
			}
			void Map::KeyConstruct(void* Memory)
			{
				new(Memory) MapKey();
			}
			void Map::KeyDestruct(MapKey* Obj)
			{
				VMCContext* Context = asGetActiveContext();
				if (Context)
				{
					VMCManager* Engine = Context->GetEngine();
					Obj->FreeValue(Engine);
				}
				Obj->~MapKey();
			}
			MapKey& Map::KeyopAssign(void* RefPtr, int TypeId, MapKey* Obj)
			{
				VMCContext* Context = asGetActiveContext();
				if (Context)
				{
					VMCManager* Engine = Context->GetEngine();
					Obj->Set(Engine, RefPtr, TypeId);
				}
				return *Obj;
			}
			MapKey& Map::KeyopAssign(const MapKey& Other, MapKey* Obj)
			{
				VMCContext* Context = asGetActiveContext();
				if (Context)
				{
					VMCManager* Engine = Context->GetEngine();
					Obj->Set(Engine, const_cast<MapKey&>(Other));
				}

				return *Obj;
			}
			MapKey& Map::KeyopAssign(double Value, MapKey* Obj)
			{
				return KeyopAssign(&Value, asTYPEID_DOUBLE, Obj);
			}
			MapKey& Map::KeyopAssign(as_int64_t Value, MapKey* Obj)
			{
				return Map::KeyopAssign(&Value, asTYPEID_INT64, Obj);
			}
			void Map::KeyopCast(void* RefPtr, int TypeId, MapKey* Obj)
			{
				VMCContext* Context = asGetActiveContext();
				if (Context)
				{
					VMCManager* Engine = Context->GetEngine();
					Obj->Get(Engine, RefPtr, TypeId);
				}
			}
			as_int64_t Map::KeyopConvInt(MapKey* Obj)
			{
				as_int64_t Value;
				KeyopCast(&Value, asTYPEID_INT64, Obj);
				return Value;
			}
			double Map::KeyopConvDouble(MapKey* Obj)
			{
				double Value;
				KeyopCast(&Value, asTYPEID_DOUBLE, Obj);
				return Value;
			}

			Grid::Grid(VMCTypeInfo* Info, void* BufferPtr) noexcept
			{
				RefCount = 1;
				GCFlag = false;
				ObjType = Info;
				ObjType->AddRef();
				Buffer = 0;
				SubTypeId = ObjType->GetSubTypeId();

				VMCManager* Engine = Info->GetEngine();
				if (SubTypeId & asTYPEID_MASK_OBJECT)
					ElementSize = sizeof(asPWORD);
				else
					ElementSize = Engine->GetSizeOfPrimitiveType(SubTypeId);

				size_t Height = *(size_t*)BufferPtr;
				size_t Width = Height ? *(size_t*)((char*)(BufferPtr)+4) : 0;

				if (!CheckMaxSize(Width, Height))
					return;

				BufferPtr = (size_t*)(BufferPtr)+1;
				if ((Info->GetSubTypeId() & asTYPEID_MASK_OBJECT) == 0)
				{
					CreateBuffer(&Buffer, Width, Height);
					for (size_t y = 0; y < Height; y++)
					{
						BufferPtr = (size_t*)(BufferPtr)+1;
						if (Width > 0)
							memcpy(At(0, y), BufferPtr, (size_t)Width * (size_t)ElementSize);

						BufferPtr = (char*)(BufferPtr)+Width * (size_t)ElementSize;
						if (asPWORD(BufferPtr) & 0x3)
							BufferPtr = (char*)(BufferPtr)+4 - (asPWORD(BufferPtr) & 0x3);
					}
				}
				else if (Info->GetSubTypeId() & asTYPEID_OBJHANDLE)
				{
					CreateBuffer(&Buffer, Width, Height);
					for (size_t y = 0; y < Height; y++)
					{
						BufferPtr = (size_t*)(BufferPtr)+1;
						if (Width > 0)
							memcpy(At(0, y), BufferPtr, (size_t)Width * (size_t)ElementSize);

						memset(BufferPtr, 0, (size_t)Width * (size_t)ElementSize);
						BufferPtr = (char*)(BufferPtr)+Width * (size_t)ElementSize;

						if (asPWORD(BufferPtr) & 0x3)
							BufferPtr = (char*)(BufferPtr)+4 - (asPWORD(BufferPtr) & 0x3);
					}
				}
				else if (Info->GetSubType()->GetFlags() & asOBJ_REF)
				{
					SubTypeId |= asTYPEID_OBJHANDLE;
					CreateBuffer(&Buffer, Width, Height);
					SubTypeId &= ~asTYPEID_OBJHANDLE;

					for (size_t y = 0; y < Height; y++)
					{
						BufferPtr = (size_t*)(BufferPtr)+1;
						if (Width > 0)
							memcpy(At(0, y), BufferPtr, (size_t)Width * (size_t)ElementSize);

						memset(BufferPtr, 0, (size_t)Width * (size_t)ElementSize);
						BufferPtr = (char*)(BufferPtr)+Width * (size_t)ElementSize;

						if (asPWORD(BufferPtr) & 0x3)
							BufferPtr = (char*)(BufferPtr)+4 - (asPWORD(BufferPtr) & 0x3);
					}
				}
				else
				{
					CreateBuffer(&Buffer, Width, Height);

					VMCTypeInfo* SubType = Info->GetSubType();
					size_t SubTypeSize = SubType->GetSize();
					for (size_t y = 0; y < Height; y++)
					{
						BufferPtr = (size_t*)(BufferPtr)+1;
						for (size_t x = 0; x < Width; x++)
						{
							void* Obj = At(x, y);
							unsigned char* srcObj = (unsigned char*)(BufferPtr)+x * SubTypeSize;
							Engine->AssignScriptObject(Obj, srcObj, SubType);
						}

						BufferPtr = (char*)(BufferPtr)+Width * SubTypeSize;
						if (asPWORD(BufferPtr) & 0x3)
							BufferPtr = (char*)(BufferPtr)+4 - (asPWORD(BufferPtr) & 0x3);
					}
				}

				if (ObjType->GetFlags() & asOBJ_GC)
					ObjType->GetEngine()->NotifyGarbageCollectorOfNewObject(this, ObjType);
			}
			Grid::Grid(size_t Width, size_t Height, VMCTypeInfo* Info) noexcept
			{
				RefCount = 1;
				GCFlag = false;
				ObjType = Info;
				ObjType->AddRef();
				Buffer = 0;
				SubTypeId = ObjType->GetSubTypeId();

				if (SubTypeId & asTYPEID_MASK_OBJECT)
					ElementSize = sizeof(asPWORD);
				else
					ElementSize = ObjType->GetEngine()->GetSizeOfPrimitiveType(SubTypeId);

				if (!CheckMaxSize(Width, Height))
					return;

				CreateBuffer(&Buffer, Width, Height);
				if (ObjType->GetFlags() & asOBJ_GC)
					ObjType->GetEngine()->NotifyGarbageCollectorOfNewObject(this, ObjType);
			}
			void Grid::Resize(size_t Width, size_t Height)
			{
				if (!CheckMaxSize(Width, Height))
					return;

				SBuffer* TempBuffer = 0;
				CreateBuffer(&TempBuffer, Width, Height);
				if (TempBuffer == 0)
					return;

				if (Buffer)
				{
					size_t W = Width > Buffer->Width ? Buffer->Width : Width;
					size_t H = Height > Buffer->Height ? Buffer->Height : Height;
					for (size_t y = 0; y < H; y++)
					{
						for (size_t x = 0; x < W; x++)
							SetValue(TempBuffer, x, y, At(Buffer, x, y));
					}

					DeleteBuffer(Buffer);
				}

				Buffer = TempBuffer;
			}
			Grid::Grid(size_t Width, size_t Height, void* DefaultValue, VMCTypeInfo* Info) noexcept
			{
				RefCount = 1;
				GCFlag = false;
				ObjType = Info;
				ObjType->AddRef();
				Buffer = 0;
				SubTypeId = ObjType->GetSubTypeId();

				if (SubTypeId & asTYPEID_MASK_OBJECT)
					ElementSize = sizeof(asPWORD);
				else
					ElementSize = ObjType->GetEngine()->GetSizeOfPrimitiveType(SubTypeId);

				if (!CheckMaxSize(Width, Height))
					return;

				CreateBuffer(&Buffer, Width, Height);
				if (ObjType->GetFlags() & asOBJ_GC)
					ObjType->GetEngine()->NotifyGarbageCollectorOfNewObject(this, ObjType);

				for (size_t y = 0; y < GetHeight(); y++)
				{
					for (size_t x = 0; x < GetWidth(); x++)
						SetValue(x, y, DefaultValue);
				}
			}
			void Grid::SetValue(size_t x, size_t y, void* Value)
			{
				SetValue(Buffer, x, y, Value);
			}
			void Grid::SetValue(SBuffer* BufferPtr, size_t x, size_t y, void* Value)
			{
				void* Ptr = At(BufferPtr, x, y);
				if (Ptr == 0)
					return;

				if ((SubTypeId & ~asTYPEID_MASK_SEQNBR) && !(SubTypeId & asTYPEID_OBJHANDLE))
					ObjType->GetEngine()->AssignScriptObject(Ptr, Value, ObjType->GetSubType());
				else if (SubTypeId & asTYPEID_OBJHANDLE)
				{
					void* Swap = *(void**)Ptr;
					*(void**)Ptr = *(void**)Value;
					ObjType->GetEngine()->AddRefScriptObject(*(void**)Value, ObjType->GetSubType());
					if (Swap)
						ObjType->GetEngine()->ReleaseScriptObject(Swap, ObjType->GetSubType());
				}
				else if (SubTypeId == asTYPEID_BOOL ||
					SubTypeId == asTYPEID_INT8 ||
					SubTypeId == asTYPEID_UINT8)
					*(char*)Ptr = *(char*)Value;
				else if (SubTypeId == asTYPEID_INT16 ||
					SubTypeId == asTYPEID_UINT16)
					*(short*)Ptr = *(short*)Value;
				else if (SubTypeId == asTYPEID_INT32 ||
					SubTypeId == asTYPEID_UINT32 ||
					SubTypeId == asTYPEID_FLOAT ||
					SubTypeId > asTYPEID_DOUBLE)
					*(int*)Ptr = *(int*)Value;
				else if (SubTypeId == asTYPEID_INT64 ||
					SubTypeId == asTYPEID_UINT64 ||
					SubTypeId == asTYPEID_DOUBLE)
					*(double*)Ptr = *(double*)Value;
			}
			Grid::~Grid()
			{
				if (Buffer)
				{
					DeleteBuffer(Buffer);
					Buffer = 0;
				}

				if (ObjType)
					ObjType->Release();
			}
			size_t Grid::GetWidth() const
			{
				if (Buffer)
					return Buffer->Width;

				return 0;
			}
			size_t Grid::GetHeight() const
			{
				if (Buffer)
					return Buffer->Height;

				return 0;
			}
			bool Grid::CheckMaxSize(size_t Width, size_t Height)
			{
				size_t MaxSize = 0xFFFFFFFFul - sizeof(SBuffer) + 1;
				if (ElementSize > 0)
					MaxSize /= (size_t)ElementSize;

				as_uint64_t NumElements = (as_uint64_t)Width * (as_uint64_t)Height;
				if ((NumElements >> 32) || NumElements > (as_uint64_t)MaxSize)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Too large grid Size");

					return false;
				}
				return true;
			}
			VMCTypeInfo* Grid::GetGridObjectType() const
			{
				return ObjType;
			}
			int Grid::GetGridTypeId() const
			{
				return ObjType->GetTypeId();
			}
			int Grid::GetElementTypeId() const
			{
				return SubTypeId;
			}
			void* Grid::At(size_t x, size_t y)
			{
				return At(Buffer, x, y);
			}
			void* Grid::At(SBuffer* BufferPtr, size_t x, size_t y)
			{
				if (BufferPtr == 0 || x >= BufferPtr->Width || y >= BufferPtr->Height)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Index out of bounds");
					return 0;
				}

				size_t Index = x + y * BufferPtr->Width;
				if ((SubTypeId & asTYPEID_MASK_OBJECT) && !(SubTypeId & asTYPEID_OBJHANDLE))
					return *(void**)(BufferPtr->Data + (size_t)ElementSize * Index);

				return BufferPtr->Data + (size_t)ElementSize * Index;
			}
			const void* Grid::At(size_t x, size_t y) const
			{
				return const_cast<Grid*>(this)->At(const_cast<SBuffer*>(Buffer), x, y);
			}
			void Grid::CreateBuffer(SBuffer** BufferPtr, size_t W, size_t H)
			{
				size_t NumElements = W * H;
				*BufferPtr = reinterpret_cast<SBuffer*>(asAllocMem(sizeof(SBuffer) - 1 + (size_t)ElementSize * (size_t)NumElements));

				if (*BufferPtr)
				{
					(*BufferPtr)->Width = W;
					(*BufferPtr)->Height = H;
					Construct(*BufferPtr);
				}
				else
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Out of memory");
				}
			}
			void Grid::DeleteBuffer(SBuffer* BufferPtr)
			{
				ED_ASSERT_V(BufferPtr, "buffer should be set");
				Destruct(BufferPtr);
				asFreeMem(BufferPtr);
			}
			void Grid::Construct(SBuffer* BufferPtr)
			{
				ED_ASSERT_V(BufferPtr, "buffer should be set");
				if (SubTypeId & asTYPEID_OBJHANDLE)
				{
					void* D = (void*)(BufferPtr->Data);
					memset(D, 0, (BufferPtr->Width * BufferPtr->Height) * sizeof(void*));
				}
				else if (SubTypeId & asTYPEID_MASK_OBJECT)
				{
					void** Max = (void**)(BufferPtr->Data + (BufferPtr->Width * BufferPtr->Height) * sizeof(void*));
					void** D = (void**)(BufferPtr->Data);

					VMCManager* Engine = ObjType->GetEngine();
					VMCTypeInfo* SubType = ObjType->GetSubType();

					for (; D < Max; D++)
					{
						*D = (void*)Engine->CreateScriptObject(SubType);
						if (*D == 0)
						{
							memset(D, 0, sizeof(void*) * (Max - D));
							return;
						}
					}
				}
			}
			void Grid::Destruct(SBuffer* BufferPtr)
			{
				ED_ASSERT_V(BufferPtr, "buffer should be set");
				if (SubTypeId & asTYPEID_MASK_OBJECT)
				{
					VMCManager* Engine = ObjType->GetEngine();
					void** Max = (void**)(BufferPtr->Data + (BufferPtr->Width * BufferPtr->Height) * sizeof(void*));
					void** D = (void**)(BufferPtr->Data);

					for (; D < Max; D++)
					{
						if (*D)
							Engine->ReleaseScriptObject(*D, ObjType->GetSubType());
					}
				}
			}
			void Grid::EnumReferences(VMCManager* Engine)
			{
				if (Buffer == 0)
					return;

				if (SubTypeId & asTYPEID_MASK_OBJECT)
				{
					size_t NumElements = Buffer->Width * Buffer->Height;
					void** D = (void**)Buffer->Data;
					VMCTypeInfo* SubType = Engine->GetTypeInfoById(SubTypeId);

					if ((SubType->GetFlags() & asOBJ_REF))
					{
						for (size_t n = 0; n < NumElements; n++)
						{
							if (D[n])
								Engine->GCEnumCallback(D[n]);
						}
					}
					else if ((SubType->GetFlags() & asOBJ_VALUE) && (SubType->GetFlags() & asOBJ_GC))
					{
						for (size_t n = 0; n < NumElements; n++)
						{
							if (D[n])
								Engine->ForwardGCEnumReferences(D[n], SubType);
						}
					}
				}
			}
			void Grid::ReleaseAllHandles(VMCManager*)
			{
				if (Buffer == 0)
					return;

				DeleteBuffer(Buffer);
				Buffer = 0;
			}
			void Grid::AddRef() const
			{
				GCFlag = false;
				asAtomicInc(RefCount);
			}
			void Grid::Release() const
			{
				GCFlag = false;
				if (asAtomicDec(RefCount) == 0)
				{
					this->~Grid();
					asFreeMem(const_cast<Grid*>(this));
				}
			}
			int Grid::GetRefCount()
			{
				return RefCount;
			}
			void Grid::SetFlag()
			{
				GCFlag = true;
			}
			bool Grid::GetFlag()
			{
				return GCFlag;
			}
			Grid* Grid::Create(VMCTypeInfo* Info)
			{
				return Grid::Create(Info, 0, 0);
			}
			Grid* Grid::Create(VMCTypeInfo* Info, size_t W, size_t H)
			{
				void* Memory = asAllocMem(sizeof(Grid));
				if (Memory == 0)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Out of memory");

					return 0;
				}

				Grid* a = new(Memory) Grid(W, H, Info);
				return a;
			}
			Grid* Grid::Create(VMCTypeInfo* Info, void* InitList)
			{
				void* Memory = asAllocMem(sizeof(Grid));
				if (Memory == 0)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Out of memory");

					return 0;
				}

				Grid* a = new(Memory) Grid(Info, InitList);
				return a;
			}
			Grid* Grid::Create(VMCTypeInfo* Info, size_t W, size_t H, void* DefaultValue)
			{
				void* Memory = asAllocMem(sizeof(Grid));
				if (Memory == 0)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Out of memory");

					return 0;
				}

				Grid* a = new(Memory) Grid(W, H, DefaultValue, Info);
				return a;
			}
			bool Grid::TemplateCallback(VMCTypeInfo* Info, bool& DontGarbageCollect)
			{
				int TypeId = Info->GetSubTypeId();
				if (TypeId == asTYPEID_VOID)
					return false;

				VMCManager* Engine = Info->GetEngine();
				if ((TypeId & asTYPEID_MASK_OBJECT) && !(TypeId & asTYPEID_OBJHANDLE))
				{
					VMCTypeInfo* SubType = Engine->GetTypeInfoById(TypeId);
					asDWORD Flags = SubType->GetFlags();

					if ((Flags & asOBJ_VALUE) && !(Flags & asOBJ_POD))
					{
						bool Found = false;
						for (size_t n = 0; n < SubType->GetBehaviourCount(); n++)
						{
							asEBehaviours Beh;
							asIScriptFunction* Function = SubType->GetBehaviourByIndex((int)n, &Beh);
							if (Beh != asBEHAVE_CONSTRUCT)
								continue;

							if (Function->GetParamCount() == 0)
							{
								Found = true;
								break;
							}
						}

						if (!Found)
						{
							Engine->WriteMessage(TYPENAME_ARRAY, 0, 0, asMSGTYPE_ERROR, "The subtype has no default constructor");
							return false;
						}
					}
					else if ((Flags & asOBJ_REF))
					{
						bool Found = false;
						if (!Engine->GetEngineProperty(asEP_DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE))
						{
							for (size_t n = 0; n < SubType->GetFactoryCount(); n++)
							{
								asIScriptFunction* Function = SubType->GetFactoryByIndex((int)n);
								if (Function->GetParamCount() == 0)
								{
									Found = true;
									break;
								}
							}
						}

						if (!Found)
						{
							Engine->WriteMessage(TYPENAME_ARRAY, 0, 0, asMSGTYPE_ERROR, "The subtype has no default factory");
							return false;
						}
					}

					if (!(Flags & asOBJ_GC))
						DontGarbageCollect = true;
				}
				else if (!(TypeId & asTYPEID_OBJHANDLE))
				{
					DontGarbageCollect = true;
				}
				else
				{
					VMCTypeInfo* SubType = Engine->GetTypeInfoById(TypeId);
					asDWORD Flags = SubType->GetFlags();

					if (!(Flags & asOBJ_GC))
					{
						if ((Flags & asOBJ_SCRIPT_OBJECT))
						{
							if ((Flags & asOBJ_NOINHERIT))
								DontGarbageCollect = true;
						}
						else
							DontGarbageCollect = true;
					}
				}

				return true;
			}

			Ref::Ref() noexcept : Type(0), Pointer(nullptr)
			{
			}
			Ref::Ref(const Ref& Other) noexcept : Type(Other.Type), Pointer(Other.Pointer)
			{
				AddRefHandle();
			}
			Ref::Ref(void* RefPtr, VMCTypeInfo* _Type) noexcept : Type(_Type), Pointer(RefPtr)
			{
				AddRefHandle();
			}
			Ref::Ref(void* RefPtr, int TypeId) noexcept : Type(0), Pointer(nullptr)
			{
				Assign(RefPtr, TypeId);
			}
			Ref::~Ref() noexcept
			{
				ReleaseHandle();
			}
			void Ref::ReleaseHandle()
			{
				if (Pointer && Type)
				{
					VMCManager* Engine = Type->GetEngine();
					Engine->ReleaseScriptObject(Pointer, Type);
					Engine->Release();
					Pointer = nullptr;
					Type = 0;
				}
			}
			void Ref::AddRefHandle()
			{
				if (Pointer && Type)
				{
					VMCManager* Engine = Type->GetEngine();
					Engine->AddRefScriptObject(Pointer, Type);
					Engine->AddRef();
				}
			}
			Ref& Ref::operator =(const Ref& Other) noexcept
			{
				Set(Other.Pointer, Other.Type);
				return *this;
			}
			void Ref::Set(void* RefPtr, VMCTypeInfo* _Type)
			{
				if (Pointer == RefPtr)
					return;

				ReleaseHandle();
				Pointer = RefPtr;
				Type = _Type;
				AddRefHandle();
			}
			void* Ref::GetRef()
			{
				return Pointer;
			}
			VMCTypeInfo* Ref::GetType() const
			{
				return Type;
			}
			int Ref::GetTypeId() const
			{
				if (Type == 0) return 0;

				return Type->GetTypeId() | asTYPEID_OBJHANDLE;
			}
			Ref& Ref::Assign(void* RefPtr, int TypeId)
			{
				if (TypeId == 0)
				{
					Set(0, 0);
					return *this;
				}

				if (TypeId & asTYPEID_OBJHANDLE)
				{
					RefPtr = *(void**)RefPtr;
					TypeId &= ~asTYPEID_OBJHANDLE;
				}

				VMCContext* Context = asGetActiveContext();
				VMCManager* Engine = Context->GetEngine();
				VMCTypeInfo* Info = Engine->GetTypeInfoById(TypeId);

				if (Info && strcmp(Info->GetName(), TYPENAME_REF) == 0)
				{
					Ref* Result = (Ref*)RefPtr;
					RefPtr = Result->Pointer;
					Info = Result->Type;
				}

				Set(RefPtr, Info);
				return *this;
			}
			bool Ref::operator==(const Ref& Other) const
			{
				if (Pointer == Other.Pointer && Type == Other.Type)
					return true;

				return false;
			}
			bool Ref::operator!=(const Ref& Other) const
			{
				return !(*this == Other);
			}
			bool Ref::Equals(void* RefPtr, int TypeId) const
			{
				if (TypeId == 0)
					RefPtr = 0;

				if (TypeId & asTYPEID_OBJHANDLE)
				{
					void** Sub = (void**)RefPtr;
					if (!Sub)
						return false;

					RefPtr = *Sub;
					TypeId &= ~asTYPEID_OBJHANDLE;
				}

				if (RefPtr == Pointer)
					return true;

				return false;
			}
			void Ref::Cast(void** OutRef, int TypeId)
			{
				if (Type == 0)
				{
					*OutRef = 0;
					return;
				}

				ED_ASSERT_V(TypeId & asTYPEID_OBJHANDLE, "type should be object handle");
				TypeId &= ~asTYPEID_OBJHANDLE;

				VMCManager* Engine = Type->GetEngine();
				VMCTypeInfo* Info = Engine->GetTypeInfoById(TypeId);
				*OutRef = 0;

				Engine->RefCastObject(Pointer, Type, Info, OutRef);
			}
			void Ref::EnumReferences(VMCManager* _Engine)
			{
				if (Pointer)
					_Engine->GCEnumCallback(Pointer);

				if (Type)
					_Engine->GCEnumCallback(Type);
			}
			void Ref::ReleaseReferences(VMCManager* _Engine)
			{
				Set(0, 0);
			}
			void Ref::Construct(Ref* Base)
			{
				new(Base) Ref();
			}
			void Ref::Construct(Ref* Base, const Ref& Other)
			{
				new(Base) Ref(Other);
			}
			void Ref::Construct(Ref* Base, void* RefPtr, int TypeId)
			{
				new(Base) Ref(RefPtr, TypeId);
			}
			void Ref::Destruct(Ref* Base)
			{
				Base->~Ref();
			}

			WeakRef::WeakRef(VMCTypeInfo* _Type) noexcept
			{
				Ref = 0;
				Type = _Type;
				Type->AddRef();
				WeakRefFlag = 0;
			}
			WeakRef::WeakRef(const WeakRef& Other) noexcept
			{
				Ref = Other.Ref;
				Type = Other.Type;
				Type->AddRef();
				WeakRefFlag = Other.WeakRefFlag;
				if (WeakRefFlag)
					WeakRefFlag->AddRef();
			}
			WeakRef::WeakRef(void* RefPtr, VMCTypeInfo* _Type) noexcept
			{
				if (!_Type || !(strcmp(_Type->GetName(), TYPENAME_WEAKREF) == 0 || strcmp(_Type->GetName(), TYPENAME_CONSTWEAKREF) == 0))
					return;

				Ref = RefPtr;
				Type = _Type;
				Type->AddRef();

				WeakRefFlag = Type->GetEngine()->GetWeakRefFlagOfScriptObject(Ref, Type->GetSubType());
				if (WeakRefFlag)
					WeakRefFlag->AddRef();
			}
			WeakRef::~WeakRef() noexcept
			{
				if (Type)
					Type->Release();

				if (WeakRefFlag)
					WeakRefFlag->Release();
			}
			WeakRef& WeakRef::operator =(const WeakRef& Other) noexcept
			{
				if (Ref == Other.Ref && WeakRefFlag == Other.WeakRefFlag)
					return *this;

				if (Type != Other.Type)
				{
					if (!(strcmp(Type->GetName(), TYPENAME_CONSTWEAKREF) == 0 && strcmp(Other.Type->GetName(), TYPENAME_WEAKREF) == 0 && Type->GetSubType() == Other.Type->GetSubType()))
						return *this;
				}

				Ref = Other.Ref;
				if (WeakRefFlag)
					WeakRefFlag->Release();

				WeakRefFlag = Other.WeakRefFlag;
				if (WeakRefFlag)
					WeakRefFlag->AddRef();

				return *this;
			}
			WeakRef& WeakRef::Set(void* newRef)
			{
				if (WeakRefFlag)
					WeakRefFlag->Release();

				Ref = newRef;
				if (newRef)
				{
					WeakRefFlag = Type->GetEngine()->GetWeakRefFlagOfScriptObject(newRef, Type->GetSubType());
					WeakRefFlag->AddRef();
				}
				else
					WeakRefFlag = 0;

				Type->GetEngine()->ReleaseScriptObject(newRef, Type->GetSubType());
				return *this;
			}
			VMCTypeInfo* WeakRef::GetRefType() const
			{
				return Type->GetSubType();
			}
			bool WeakRef::operator==(const WeakRef& Other) const
			{
				if (Ref == Other.Ref &&
					WeakRefFlag == Other.WeakRefFlag &&
					Type == Other.Type)
					return true;

				return false;
			}
			bool WeakRef::operator!=(const WeakRef& Other) const
			{
				return !(*this == Other);
			}
			void* WeakRef::Get() const
			{
				if (Ref == 0 || WeakRefFlag == 0)
					return 0;

				WeakRefFlag->Lock();
				if (!WeakRefFlag->Get())
				{
					Type->GetEngine()->AddRefScriptObject(Ref, Type->GetSubType());
					WeakRefFlag->Unlock();
					return Ref;
				}
				WeakRefFlag->Unlock();

				return 0;
			}
			bool WeakRef::Equals(void* RefPtr) const
			{
				if (Ref != RefPtr)
					return false;

				asILockableSharedBool* flag = Type->GetEngine()->GetWeakRefFlagOfScriptObject(RefPtr, Type->GetSubType());
				if (WeakRefFlag != flag)
					return false;

				return true;
			}
			void WeakRef::Construct(VMCTypeInfo* Type, void* Memory)
			{
				new(Memory) WeakRef(Type);
			}
			void WeakRef::Construct2(VMCTypeInfo* Type, void* RefPtr, void* Memory)
			{
				new(Memory) WeakRef(RefPtr, Type);
				VMCContext* Context = asGetActiveContext();
				if (Context && Context->GetState() == asEXECUTION_EXCEPTION)
					reinterpret_cast<WeakRef*>(Memory)->~WeakRef();
			}
			void WeakRef::Destruct(WeakRef* Obj)
			{
				Obj->~WeakRef();
			}
			bool WeakRef::TemplateCallback(VMCTypeInfo* Info, bool&)
			{
				VMCTypeInfo* SubType = Info->GetSubType();
				if (SubType == 0)
					return false;

				if (!(SubType->GetFlags() & asOBJ_REF))
					return false;

				if (Info->GetSubTypeId() & asTYPEID_OBJHANDLE)
					return false;

				size_t cnt = SubType->GetBehaviourCount();
				for (size_t n = 0; n < cnt; n++)
				{
					asEBehaviours Beh;
					SubType->GetBehaviourByIndex((int)n, &Beh);
					if (Beh == asBEHAVE_GET_WEAKREF_FLAG)
						return true;
				}

				Info->GetEngine()->WriteMessage(TYPENAME_WEAKREF, 0, 0, asMSGTYPE_ERROR, "The subtype doesn't support weak references");
				return false;
			}

			Complex::Complex() noexcept
			{
				R = 0;
				I = 0;
			}
			Complex::Complex(const Complex& Other) noexcept
			{
				R = Other.R;
				I = Other.I;
			}
			Complex::Complex(float _R, float _I) noexcept
			{
				R = _R;
				I = _I;
			}
			bool Complex::operator==(const Complex& Other) const
			{
				return (R == Other.R) && (I == Other.I);
			}
			bool Complex::operator!=(const Complex& Other) const
			{
				return !(*this == Other);
			}
			Complex& Complex::operator=(const Complex& Other) noexcept
			{
				R = Other.R;
				I = Other.I;
				return *this;
			}
			Complex& Complex::operator+=(const Complex& Other)
			{
				R += Other.R;
				I += Other.I;
				return *this;
			}
			Complex& Complex::operator-=(const Complex& Other)
			{
				R -= Other.R;
				I -= Other.I;
				return *this;
			}
			Complex& Complex::operator*=(const Complex& Other)
			{
				*this = *this * Other;
				return *this;
			}
			Complex& Complex::operator/=(const Complex& Other)
			{
				*this = *this / Other;
				return *this;
			}
			float Complex::SquaredLength() const
			{
				return R * R + I * I;
			}
			float Complex::Length() const
			{
				return sqrtf(SquaredLength());
			}
			Complex Complex::operator+(const Complex& Other) const
			{
				return Complex(R + Other.R, I + Other.I);
			}
			Complex Complex::operator-(const Complex& Other) const
			{
				return Complex(R - Other.R, I + Other.I);
			}
			Complex Complex::operator*(const Complex& Other) const
			{
				return Complex(R * Other.R - I * Other.I, R * Other.I + I * Other.R);
			}
			Complex Complex::operator/(const Complex& Other) const
			{
				float Sq = Other.SquaredLength();
				if (Sq == 0)
					return Complex(0, 0);

				return Complex((R * Other.R + I * Other.I) / Sq, (I * Other.R - R * Other.I) / Sq);
			}
			Complex Complex::GetRI() const
			{
				return *this;
			}
			Complex Complex::GetIR() const
			{
				return Complex(R, I);
			}
			void Complex::SetRI(const Complex& Other)
			{
				*this = Other;
			}
			void Complex::SetIR(const Complex& Other)
			{
				R = Other.I;
				I = Other.R;
			}
			void Complex::DefaultConstructor(Complex* Base)
			{
				new(Base) Complex();
			}
			void Complex::CopyConstructor(const Complex& Other, Complex* Base)
			{
				new(Base) Complex(Other);
			}
			void Complex::ConvConstructor(float Result, Complex* Base)
			{
				new(Base) Complex(Result);
			}
			void Complex::InitConstructor(float Result, float _I, Complex* Base)
			{
				new(Base) Complex(Result, _I);
			}
			void Complex::ListConstructor(float* InitList, Complex* Base)
			{
				new(Base) Complex(InitList[0], InitList[1]);
			}

			std::string Random::Getb(uint64_t Size)
			{
				return Compute::Codec::HexEncode(Compute::Crypto::RandomBytes((size_t)Size)).substr(0, (size_t)Size);
			}
			double Random::Betweend(double Min, double Max)
			{
				return Compute::Mathd::Random(Min, Max);
			}
			double Random::Magd()
			{
				return Compute::Mathd::RandomMag();
			}
			double Random::Getd()
			{
				return Compute::Mathd::Random();
			}
			float Random::Betweenf(float Min, float Max)
			{
				return Compute::Mathf::Random(Min, Max);
			}
			float Random::Magf()
			{
				return Compute::Mathf::RandomMag();
			}
			float Random::Getf()
			{
				return Compute::Mathf::Random();
			}
			uint64_t Random::Betweeni(uint64_t Min, uint64_t Max)
			{
				return Compute::Math<uint64_t>::Random(Min, Max);
			}

			Thread::Thread(VMCManager* Engine, VMCFunction* Func) noexcept : Function(Func), Manager(VMManager::Get(Engine)), Context(nullptr), GCFlag(false), Ref(1)
			{
				Engine->NotifyGarbageCollectorOfNewObject(this, Engine->GetTypeInfoByName(TYPENAME_THREAD));
			}
			void Thread::Routine()
			{
				Mutex.lock();
				if (!Function)
				{
					Release();
					return Mutex.unlock();
				}

				if (Context == nullptr)
					Context = Manager->CreateContext();

				if (Context == nullptr)
				{
					Manager->GetEngine()->WriteMessage(TYPENAME_THREAD, 0, 0, asMSGTYPE_ERROR, "failed to start a thread: no available context");
					Release();

					return Mutex.unlock();
				}

				Mutex.unlock();
				Context->TryExecute(false, Function, [this](Script::VMContext* Context)
				{
					Context->SetArgObject(0, this);
					Context->SetUserData(this, ContextUD);
				}).Await([this](int&&)
				{
					Context->SetUserData(nullptr, ContextUD);
					this->Mutex.lock();

					if (!Context->IsSuspended())
						ED_CLEAR(Context);

					CV.notify_all();
					this->Mutex.unlock();
					Release();
				});
			}
			void Thread::AddRef()
			{
				GCFlag = false;
				asAtomicInc(Ref);
			}
			void Thread::Suspend()
			{
				Mutex.lock();
				if (Context && Context->GetState() != VMRuntime::SUSPENDED)
					Context->Suspend();
				Mutex.unlock();
			}
			void Thread::Resume()
			{
				Mutex.lock();
				if (Context && Context->GetState() == VMRuntime::SUSPENDED)
					Context->Execute();
				Mutex.unlock();
			}
			void Thread::Release()
			{
				GCFlag = false;
				if (asAtomicDec(Ref) <= 0)
				{
					if (Procedure.joinable())
					{
						ED_DEBUG("[vm] join thread %s", Core::OS::Process::GetThreadId(Procedure.get_id()).c_str());
						Procedure.join();
					}

					ReleaseReferences(nullptr);
					this->~Thread();
					asFreeMem((void*)this);
				}
			}
			void Thread::SetGCFlag()
			{
				GCFlag = true;
			}
			bool Thread::GetGCFlag()
			{
				return GCFlag;
			}
			int Thread::GetRefCount()
			{
				return Ref;
			}
			void Thread::EnumReferences(VMCManager* Engine)
			{
				for (int i = 0; i < 2; i++)
				{
					for (auto Any : Pipe[i].Queue)
					{
						if (Any != nullptr)
							Engine->GCEnumCallback(Any);
					}
				}

				Engine->GCEnumCallback(Engine);
				if (Context != nullptr)
					Engine->GCEnumCallback(Context);

				if (Function != nullptr)
					Engine->GCEnumCallback(Function);
			}
			int Thread::Join(uint64_t Timeout)
			{
				if (std::this_thread::get_id() == Procedure.get_id())
					return -1;

				Mutex.lock();
				if (!Procedure.joinable())
				{
					Mutex.unlock();
					return -1;
				}
				Mutex.unlock();

				std::unique_lock<std::mutex> Guard(Mutex);
				if (CV.wait_for(Guard, std::chrono::milliseconds(Timeout), [&]
				{
					return !((Context && Context->GetState() != VMRuntime::SUSPENDED));
				}))
				{
					ED_DEBUG("[vm] join thread %s", Core::OS::Process::GetThreadId(Procedure.get_id()).c_str());
					Procedure.join();
					return 1;
				}

				return 0;
			}
			int Thread::Join()
			{
				if (std::this_thread::get_id() == Procedure.get_id())
					return -1;

				while (true)
				{
					int R = Join(1000);
					if (R == -1 || R == 1)
						return R;
				}

				return 0;
			}
			std::string Thread::GetId() const
			{
				return Core::OS::Process::GetThreadId(Procedure.get_id());
			}
			void Thread::Push(void* _Ref, int TypeId)
			{
				auto* _Thread = GetThread();
				int Id = (_Thread == this ? 1 : 0);

				void* Data = asAllocMem(sizeof(Any));
				Any* Next = new(Data) Any(_Ref, TypeId, VMManager::Get()->GetEngine());
				Pipe[Id].Mutex.lock();
				Pipe[Id].Queue.push_back(Next);
				Pipe[Id].Mutex.unlock();
				Pipe[Id].CV.notify_one();
			}
			bool Thread::Pop(void* _Ref, int TypeId)
			{
				bool Resolved = false;
				while (!Resolved)
					Resolved = Pop(_Ref, TypeId, 1000);

				return true;
			}
			bool Thread::Pop(void* _Ref, int TypeId, uint64_t Timeout)
			{
				auto* _Thread = GetThread();
				int Id = (_Thread == this ? 0 : 1);

				std::unique_lock<std::mutex> Guard(Pipe[Id].Mutex);
				if (!CV.wait_for(Guard, std::chrono::milliseconds(Timeout), [&]
				{
					return Pipe[Id].Queue.size() != 0;
				}))
					return false;

				Any* Result = Pipe[Id].Queue.front();
				if (!Result->Retrieve(_Ref, TypeId))
					return false;

				Pipe[Id].Queue.erase(Pipe[Id].Queue.begin());
				Result->Release();

				return true;
			}
			bool Thread::IsActive()
			{
				Mutex.lock();
				bool State = (Context && Context->GetState() != VMRuntime::SUSPENDED);
				Mutex.unlock();

				return State;
			}
			bool Thread::Start()
			{
				Mutex.lock();
				if (!Function)
				{
					Mutex.unlock();
					return false;
				}

				if (Context != nullptr)
				{
					if (Context->GetState() != VMRuntime::SUSPENDED)
					{
						Mutex.unlock();
						return false;
					}
					else
					{
						Mutex.unlock();
						Join();
						Mutex.lock();
					}
				}
				else if (Procedure.joinable())
				{
					if (std::this_thread::get_id() == Procedure.get_id())
					{
						Mutex.unlock();
						return false;
					}

					ED_DEBUG("[vm] join thread %s", Core::OS::Process::GetThreadId(Procedure.get_id()).c_str());
					Procedure.join();
				}

				AddRef();
				Procedure = std::thread(&Thread::Routine, this);
				ED_DEBUG("[vm] spawn thread %s", Core::OS::Process::GetThreadId(Procedure.get_id()).c_str());
				Mutex.unlock();

				return true;
			}
			void Thread::ReleaseReferences(VMCManager*)
			{
				if (Join() >= 0)
					ED_ERR("[memerr] thread was forced to join");

				for (int i = 0; i < 2; i++)
				{
					Pipe[i].Mutex.lock();
					for (auto Any : Pipe[i].Queue)
					{
						if (Any != nullptr)
							Any->Release();
					}
					Pipe[i].Queue.clear();
					Pipe[i].Mutex.unlock();
				}

				Mutex.lock();
				if (Function)
					Function->Release();

				ED_CLEAR(Context);
				Manager = nullptr;
				Function = nullptr;
				Mutex.unlock();
			}
			void Thread::Create(VMCGeneric* Generic)
			{
				VMCManager* Engine = Generic->GetEngine();
				VMCFunction* Function = *(VMCFunction**)Generic->GetAddressOfArg(0);
				void* Data = asAllocMem(sizeof(Thread));
				*(Thread**)Generic->GetAddressOfReturnLocation() = new(Data) Thread(Engine, Function);
			}
			Thread* Thread::GetThread()
			{
				VMCContext* Context = asGetActiveContext();
				if (!Context)
					return nullptr;

				return static_cast<Thread*>(Context->GetUserData(ContextUD));
			}
			void Thread::ThreadSleep(uint64_t Timeout)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(Timeout));
			}
			void Thread::ThreadSuspend()
			{
				VMCContext* Context = asGetActiveContext();
				if (Context && Context->GetState() != asEXECUTION_SUSPENDED)
					Context->Suspend();
			}
			std::string Thread::GetThreadId()
			{
				return Core::OS::Process::GetThreadId(std::this_thread::get_id());
			}
			int Thread::ContextUD = 550;
			int Thread::EngineListUD = 551;

			Promise::Promise(VMCContext* _Base, bool IsRef) noexcept : Context(VMContext::Get(_Base)), Future(nullptr), Ref(1), Flag(false), Pending(false)
			{
				if (!Context)
					return;

				Context->AddRef();
				Engine = Context->GetManager()->GetEngine();
				Engine->NotifyGarbageCollectorOfNewObject(this, Engine->GetTypeInfoByName(IsRef ? "ref_promise" : "promise"));
			}
			void Promise::Release()
			{
				Flag = false;
				if (asAtomicDec(Ref) <= 0)
				{
					ReleaseReferences(nullptr);
					this->~Promise();
					asFreeMem((void*)this);
				}
			}
			void Promise::AddRef()
			{
				Flag = false;
				asAtomicInc(Ref);
			}
			void Promise::EnumReferences(VMCManager* Engine)
			{
				if (Future != nullptr)
					Engine->GCEnumCallback(Future);
			}
			void Promise::ReleaseReferences(VMCManager*)
			{
				if (Future != nullptr)
				{
					Future->Release();
					Future = nullptr;
				}

				if (Context != nullptr)
				{
					Context->Release();
					Context = nullptr;
				}
			}
			void Promise::SetGCFlag()
			{
				Flag = true;
			}
			bool Promise::GetGCFlag()
			{
				return Flag;
			}
			int Promise::GetRefCount()
			{
				return Ref;
			}
			int Promise::Notify()
			{
				Update.lock();
				if (Context->GetState() == VMRuntime::ACTIVE)
				{
					Update.unlock();
					return Core::Schedule::Get()->SetTask(std::bind(&Promise::Notify, this), Core::Difficulty::Light);
				}

				Update.unlock();
				Context->Execute();
				Release();

				return 0;
			}
			int Promise::Set(void* _Ref, int TypeId)
			{
				Update.lock();
				if (Future != nullptr)
					Future->Release();

				Future = new(asAllocMem(sizeof(Any))) Any(_Ref, TypeId, Engine);
				if (TypeId & asTYPEID_OBJHANDLE)
					Engine->ReleaseScriptObject(*(void**)_Ref, Engine->GetTypeInfoById(TypeId));

				if (!Pending)
				{
					Update.unlock();
					return 0;
				}

				Pending = false;
				if (Context->GetUserData(PromiseUD) == (void*)this)
					Context->SetUserData(nullptr, PromiseUD);

				if (Context->GetState() == VMRuntime::ACTIVE)
				{
					Update.unlock();
					return Core::Schedule::Get()->SetTask(std::bind(&Promise::Notify, this), Core::Difficulty::Light);
				}

				Update.unlock();
				Context->Execute();
				Release();

				return 0;
			}
			int Promise::Set(void* _Ref, const char* TypeName)
			{
				return Set(_Ref, Engine->GetTypeIdByDecl(TypeName));
			}
			bool Promise::To(void* _Ref, int TypeId)
			{
				if (!Future)
					return false;

				return Future->Retrieve(_Ref, TypeId);
			}
			void* Promise::GetHandle()
			{
				if (!Future)
					return nullptr;

				int TypeId = Future->GetTypeId();
				if (TypeId & asTYPEID_OBJHANDLE)
					return Future->Value.ValueObj;

				Context->SetException("object cannot be safely retrieved by reference type promise");
				return nullptr;
			}
			void* Promise::GetValue()
			{
				if (!Future)
					return nullptr;

				int TypeId = Future->GetTypeId();
				if (TypeId & asTYPEID_MASK_OBJECT)
					return Future->Value.ValueObj;
				else if (TypeId <= asTYPEID_DOUBLE || TypeId & asTYPEID_MASK_SEQNBR)
					return &Future->Value.ValueInt;

				if (TypeId & asTYPEID_OBJHANDLE)
					Context->SetException("object cannot be safely retrieved by value type promise");
				else
					Context->SetException("retrieve this object explicitly");

				return nullptr;
			}
			Promise* Promise::Create()
			{
				return new(asAllocMem(sizeof(Promise))) Promise(asGetActiveContext(), false);
			}
			Promise* Promise::CreateRef()
			{
				return new(asAllocMem(sizeof(Promise))) Promise(asGetActiveContext(), true);
			}
			Promise* Promise::JumpIf(Promise* Base)
			{
				Base->Update.lock();
				if (!Base->Future && Base->Context != nullptr)
				{
					Base->AddRef();
					Base->Pending = true;
					Base->Context->SetUserData(Base, PromiseUD);
					Base->Context->Suspend();
				}
				Base->Update.unlock();
				return Base;
			}
			std::string Promise::GetStatus(VMContext* Context)
			{
				ED_ASSERT(Context != nullptr, std::string(), "context should be set");

				std::string Result;
				switch (Context->GetState())
				{
					case Edge::Script::VMRuntime::FINISHED:
						Result = "FIN";
						break;
					case Edge::Script::VMRuntime::SUSPENDED:
						Result = "SUSP";
						break;
					case Edge::Script::VMRuntime::ABORTED:
						Result = "ABRT";
						break;
					case Edge::Script::VMRuntime::EXCEPTION:
						Result = "EXCE";
						break;
					case Edge::Script::VMRuntime::PREPARED:
						Result = "PREP";
						break;
					case Edge::Script::VMRuntime::ACTIVE:
						Result = "ACTV";
						break;
					case Edge::Script::VMRuntime::ERR:
						Result = "ERR";
						break;
					default:
						Result = "INIT";
						break;
				}

				Promise* Base = (Promise*)Context->GetUserData(PromiseUD);
				if (Base != nullptr)
				{
					const char* Format = " in pending promise on 0x%" PRIXPTR " %s";
					if (Base->Future != nullptr)
						Result += Core::Form(Format, (uintptr_t)Base, "that was fulfilled").R();
					else
						Result += Core::Form(Format, (uintptr_t)Base, "that was not fulfilled").R();
				}

				return Result;
			}
			int Promise::PromiseUD = 559;

			Core::Decimal DecimalNegate(Core::Decimal& Base)
			{
				return -Base;
			}
			Core::Decimal& DecimalMulEq(Core::Decimal& Base, const Core::Decimal& V)
			{
				Base *= V;
				return Base;
			}
			Core::Decimal& DecimalDivEq(Core::Decimal& Base, const Core::Decimal& V)
			{
				Base /= V;
				return Base;
			}
			Core::Decimal& DecimalAddEq(Core::Decimal& Base, const Core::Decimal& V)
			{
				Base += V;
				return Base;
			}
			Core::Decimal& DecimalSubEq(Core::Decimal& Base, const Core::Decimal& V)
			{
				Base -= V;
				return Base;
			}
			Core::Decimal& DecimalPP(Core::Decimal& Base)
			{
				Base++;
				return Base;
			}
			Core::Decimal& DecimalMM(Core::Decimal& Base)
			{
				Base--;
				return Base;
			}
			bool DecimalEq(Core::Decimal& Base, const Core::Decimal& Right)
			{
				return Base == Right;
			}
			int DecimalCmp(Core::Decimal& Base, const Core::Decimal& Right)
			{
				if (Base == Right)
					return 0;

				return Base > Right ? 1 : -1;
			}
			Core::Decimal DecimalAdd(const Core::Decimal& Left, const Core::Decimal& Right)
			{
				return Left + Right;
			}
			Core::Decimal DecimalSub(const Core::Decimal& Left, const Core::Decimal& Right)
			{
				return Left - Right;
			}
			Core::Decimal DecimalMul(const Core::Decimal& Left, const Core::Decimal& Right)
			{
				return Left * Right;
			}
			Core::Decimal DecimalDiv(const Core::Decimal& Left, const Core::Decimal& Right)
			{
				return Left / Right;
			}
			Core::Decimal DecimalPer(const Core::Decimal& Left, const Core::Decimal& Right)
			{
				return Left % Right;
			}

			Core::DateTime& DateTimeAddEq(Core::DateTime& Base, const Core::DateTime& V)
			{
				Base += V;
				return Base;
			}
			Core::DateTime& DateTimeSubEq(Core::DateTime& Base, const Core::DateTime& V)
			{
				Base -= V;
				return Base;
			}
			Core::DateTime& DateTimeSet(Core::DateTime& Base, const Core::DateTime& V)
			{
				Base = V;
				return Base;
			}
			bool DateTimeEq(Core::DateTime& Base, const Core::DateTime& Right)
			{
				return Base == Right;
			}
			int DateTimeCmp(Core::DateTime& Base, const Core::DateTime& Right)
			{
				if (Base == Right)
					return 0;

				return Base > Right ? 1 : -1;
			}
			Core::DateTime DateTimeAdd(const Core::DateTime& Left, const Core::DateTime& Right)
			{
				return Left + Right;
			}
			Core::DateTime DateTimeSub(const Core::DateTime& Left, const Core::DateTime& Right)
			{
				return Left - Right;
			}

			void ConsoleTrace(Core::Console* Base, uint32_t Frames)
			{
				VMContext* Context = VMContext::Get();
				if (Context != nullptr)
					return Base->WriteLine(Context->GetStackTrace(3, (size_t)Frames));

				ED_ERR("[vm] no active context for stack tracing");
			}
			void ConsoleGetSize(Core::Console* Base, uint32_t& X, uint32_t& Y)
			{
				Base->GetSize(&X, &Y);
			}

			size_t VariantGetSize(Core::Variant& Base)
			{
				return Base.GetSize();
			}
			bool VariantEquals(Core::Variant& Base, const Core::Variant& Other)
			{
				return Base == Other;
			}
			bool VariantImplCast(Core::Variant& Base)
			{
				return Base;
			}

			Core::Schema* SchemaInit(Core::Schema* Base)
			{
				return Base;
			}
			Core::Schema* SchemaConstructBuffer(unsigned char* Buffer)
			{
				if (!Buffer)
					return nullptr;

				Core::Schema* Result = Core::Var::Set::Object();
				VMCContext* Context = asGetActiveContext();
				VMCManager* Manager = Context->GetEngine();
				asUINT Length = *(asUINT*)Buffer;
				Buffer += 4;

				while (Length--)
				{
					if (asPWORD(Buffer) & 0x3)
						Buffer += 4 - (asPWORD(Buffer) & 0x3);

					std::string Name = *(std::string*)Buffer;
					Buffer += sizeof(std::string);

					int TypeId = *(int*)Buffer;
					Buffer += sizeof(int);

					void* Ref = (void*)Buffer;
					if (TypeId >= asTYPEID_BOOL && TypeId <= asTYPEID_DOUBLE)
					{
						switch (TypeId)
						{
							case asTYPEID_BOOL:
								Result->Set(Name, Core::Var::Boolean(*(bool*)Ref));
								break;
							case asTYPEID_INT8:
								Result->Set(Name, Core::Var::Integer(*(char*)Ref));
								break;
							case asTYPEID_INT16:
								Result->Set(Name, Core::Var::Integer(*(short*)Ref));
								break;
							case asTYPEID_INT32:
								Result->Set(Name, Core::Var::Integer(*(int*)Ref));
								break;
							case asTYPEID_UINT8:
								Result->Set(Name, Core::Var::Integer(*(unsigned char*)Ref));
								break;
							case asTYPEID_UINT16:
								Result->Set(Name, Core::Var::Integer(*(unsigned short*)Ref));
								break;
							case asTYPEID_UINT32:
								Result->Set(Name, Core::Var::Integer(*(unsigned int*)Ref));
								break;
							case asTYPEID_INT64:
							case asTYPEID_UINT64:
								Result->Set(Name, Core::Var::Integer(*(asINT64*)Ref));
								break;
							case asTYPEID_FLOAT:
								Result->Set(Name, Core::Var::Number(*(float*)Ref));
								break;
							case asTYPEID_DOUBLE:
								Result->Set(Name, Core::Var::Number(*(double*)Ref));
								break;
						}
					}
					else
					{
						VMCTypeInfo* Type = Manager->GetTypeInfoById(TypeId);
						if ((TypeId & asTYPEID_MASK_OBJECT) && !(TypeId & asTYPEID_OBJHANDLE) && (Type && Type->GetFlags() & asOBJ_REF))
							Ref = *(void**)Ref;

						if (TypeId & asTYPEID_OBJHANDLE)
							Ref = *(void**)Ref;

						if (VMManager::Get(Manager)->IsNullable(TypeId) || !Ref)
						{
							Result->Set(Name, Core::Var::Null());
						}
						else if (Type && !strcmp(TYPENAME_SCHEMA, Type->GetName()))
						{
							Core::Schema* Base = (Core::Schema*)Ref;
							if (Base->GetParent() != Result)
								Base->AddRef();

							Result->Set(Name, Base);
						}
						else if (Type && !strcmp(TYPENAME_STRING, Type->GetName()))
							Result->Set(Name, Core::Var::String(*(std::string*)Ref));
						else if (Type && !strcmp(TYPENAME_DECIMAL, Type->GetName()))
							Result->Set(Name, Core::Var::Decimal(*(Core::Decimal*)Ref));
					}

					if (TypeId & asTYPEID_MASK_OBJECT)
					{
						asITypeInfo* ti = Manager->GetTypeInfoById(TypeId);
						if (ti->GetFlags() & asOBJ_VALUE)
							Buffer += ti->GetSize();
						else
							Buffer += sizeof(void*);
					}
					else if (TypeId == 0)
					{
						ED_WARN("[memerr] use nullptr instead of null for initialization lists");
						Buffer += sizeof(void*);
					}
					else
						Buffer += Manager->GetSizeOfPrimitiveType(TypeId);
				}

				return Result;
			}
			void SchemaConstruct(VMCGeneric* Generic)
			{
				unsigned char* Buffer = (unsigned char*)Generic->GetArgAddress(0);
				*(Core::Schema**)Generic->GetAddressOfReturnLocation() = SchemaConstructBuffer(Buffer);
			}
			Core::Schema* SchemaGetIndex(Core::Schema* Base, const std::string& Name)
			{
				Core::Schema* Result = Base->Fetch(Name);
				if (Result != nullptr)
					return Result;

				return Base->Set(Name, Core::Var::Undefined());
			}
			Core::Schema* SchemaGetIndexOffset(Core::Schema* Base, size_t Offset)
			{
				return Base->Get(Offset);
			}
			Core::Schema* SchemaSet(Core::Schema* Base, const std::string& Name, Core::Schema* Value)
			{
				if (Value != nullptr && Value->GetParent() != Base)
					Value->AddRef();

				return Base->Set(Name, Value);
			}
			Core::Schema* SchemaPush(Core::Schema* Base, Core::Schema* Value)
			{
				if (Value != nullptr)
					Value->AddRef();

				return Base->Push(Value);
			}
			Core::Schema* SchemaFirst(Core::Schema* Base)
			{
				auto& Childs = Base->GetChilds();
				return Childs.empty() ? nullptr : Childs.front();
			}
			Core::Schema* SchemaLast(Core::Schema* Base)
			{
				auto& Childs = Base->GetChilds();
				return Childs.empty() ? nullptr : Childs.back();
			}
			Core::Variant SchemaFirstVar(Core::Schema* Base)
			{
				auto& Childs = Base->GetChilds();
				return Childs.empty() ? Core::Var::Undefined() : Childs.front()->Value;
			}
			Core::Variant SchemaLastVar(Core::Schema* Base)
			{
				auto& Childs = Base->GetChilds();
				return Childs.empty() ? Core::Var::Undefined() : Childs.back()->Value;
			}
			Array* SchemaGetCollection(Core::Schema* Base, const std::string& Name, bool Deep)
			{
				VMContext* Context = VMContext::Get();
				if (!Context)
					return nullptr;

				VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				std::vector<Core::Schema*> Nodes = Base->FetchCollection(Name, Deep);
				for (auto& Node : Nodes)
					Node->AddRef();

				VMTypeInfo Type = Manager->Global().GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_SCHEMA "@>@");
				return Array::Compose(Type.GetTypeInfo(), Nodes);
			}
			Array* SchemaGetChilds(Core::Schema* Base)
			{
				VMContext* Context = VMContext::Get();
				if (!Context)
					return nullptr;

				VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				VMTypeInfo Type = Manager->Global().GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_SCHEMA "@>@");
				return Array::Compose(Type.GetTypeInfo(), Base->GetChilds());
			}
			Array* SchemaGetAttributes(Core::Schema* Base)
			{
				VMContext* Context = VMContext::Get();
				if (!Context)
					return nullptr;

				VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				VMTypeInfo Type = Manager->Global().GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_SCHEMA "@>@");
				return Array::Compose(Type.GetTypeInfo(), Base->GetAttributes());
			}
			Map* SchemaGetNames(Core::Schema* Base)
			{
				VMContext* Context = VMContext::Get();
				if (!Context)
					return nullptr;

				VMManager* Manager = Context->GetManager();
				if (!Manager)
					return nullptr;

				std::unordered_map<std::string, size_t> Mapping = Base->GetNames();
				Map* Map = Map::Create(Manager->GetEngine());

				for (auto& Item : Mapping)
				{
					int64_t Index = (int64_t)Item.second;
					Map->Set(Item.first, &Index, (int)VMTypeId::INT64);
				}

				return Map;
			}
			size_t SchemaGetSize(Core::Schema* Base)
			{
				return Base->Size();
			}
			std::string SchemaToJSON(Core::Schema* Base)
			{
				std::string Stream;
				Core::Schema::ConvertToJSON(Base, [&Stream](Core::VarForm, const char* Buffer, size_t Length)
				{
					if (Buffer != nullptr && Length > 0)
					Stream.append(Buffer, Length);
				});

				return Stream;
			}
			std::string SchemaToXML(Core::Schema* Base)
			{
				std::string Stream;
				Core::Schema::ConvertToXML(Base, [&Stream](Core::VarForm, const char* Buffer, size_t Length)
				{
					if (Buffer != nullptr && Length > 0)
					Stream.append(Buffer, Length);
				});

				return Stream;
			}
			std::string SchemaToString(Core::Schema* Base)
			{
				switch (Base->Value.GetType())
				{
					case Core::VarType::Null:
					case Core::VarType::Undefined:
					case Core::VarType::Object:
					case Core::VarType::Array:
					case Core::VarType::Pointer:
						break;
					case Core::VarType::String:
					case Core::VarType::Binary:
						return Base->Value.GetBlob();
					case Core::VarType::Integer:
						return std::to_string(Base->Value.GetInteger());
					case Core::VarType::Number:
						return std::to_string(Base->Value.GetNumber());
					case Core::VarType::Decimal:
						return Base->Value.GetDecimal().ToString();
					case Core::VarType::Boolean:
						return Base->Value.GetBoolean() ? "1" : "0";
				}

				return "";
			}
			std::string SchemaToBinary(Core::Schema* Base)
			{
				return Base->Value.GetBlob();
			}
			int64_t SchemaToInteger(Core::Schema* Base)
			{
				return Base->Value.GetInteger();
			}
			double SchemaToNumber(Core::Schema* Base)
			{
				return Base->Value.GetNumber();
			}
			Core::Decimal SchemaToDecimal(Core::Schema* Base)
			{
				return Base->Value.GetDecimal();
			}
			bool SchemaToBoolean(Core::Schema* Base)
			{
				return Base->Value.GetBoolean();
			}
			Core::Schema* SchemaFromJSON(const std::string& Value)
			{
				return Core::Schema::ConvertFromJSON(Value.c_str(), Value.size());
			}
			Core::Schema* SchemaFromXML(const std::string& Value)
			{
				return Core::Schema::ConvertFromXML(Value.c_str());
			}
			Core::Schema* SchemaImport(const std::string& Value)
			{
				VMManager* Manager = VMManager::Get();
				if (!Manager)
					return nullptr;

				return Manager->ImportJSON(Value);
			}
			Core::Schema* SchemaCopyAssign(Core::Schema* Base, const Core::Variant& Other)
			{
				Base->Value = Other;
				return Base;
			}
			bool SchemaEquals(Core::Schema* Base, Core::Schema* Other)
			{
				if (Other != nullptr)
					return Base->Value == Other->Value;

				Core::VarType Type = Base->Value.GetType();
				return (Type == Core::VarType::Null || Type == Core::VarType::Undefined);
			}

			Format::Format() noexcept
			{
			}
			Format::Format(unsigned char* Buffer) noexcept
			{
				VMContext* Context = VMContext::Get();
				if (!Context || !Buffer)
					return;

				VMGlobal& Global = Context->GetManager()->Global();
				unsigned int Length = *(unsigned int*)Buffer;
				Buffer += 4;

				while (Length--)
				{
					if (uintptr_t(Buffer) & 0x3)
						Buffer += 4 - (uintptr_t(Buffer) & 0x3);

					int TypeId = *(int*)Buffer;
					Buffer += sizeof(int);

					Core::Parser Result; std::string Offset;
					FormatBuffer(Global, Result, Offset, (void*)Buffer, TypeId);
					Args.push_back(Result.R()[0] == '\n' ? Result.Substring(1).R() : Result.R());

					if (TypeId & (int)VMTypeId::MASK_OBJECT)
					{
						VMTypeInfo Type = Global.GetTypeInfoById(TypeId);
						if (Type.IsValid() && Type.GetFlags() & (size_t)VMObjType::VALUE)
							Buffer += Type.GetSize();
						else
							Buffer += sizeof(void*);
					}
					else if (TypeId == 0)
						Buffer += sizeof(void*);
					else
						Buffer += Global.GetSizeOfPrimitiveType(TypeId);
				}
			}
			std::string Format::JSON(void* Ref, int TypeId)
			{
				VMContext* Context = VMContext::Get();
				if (!Context)
					return "{}";

				VMGlobal& Global = Context->GetManager()->Global();
				Core::Parser Result;

				FormatJSON(Global, Result, Ref, TypeId);
				return Result.R();
			}
			std::string Format::Form(const std::string& F, const Format& Form)
			{
				Core::Parser Buffer = F;
				size_t Offset = 0;

				for (auto& Item : Form.Args)
				{
					auto R = Buffer.FindUnescaped('?', Offset);
					if (!R.Found)
						break;

					Buffer.ReplacePart(R.Start, R.End, Item);
					Offset = R.End;
				}

				return Buffer.R();
			}
			void Format::WriteLine(Core::Console* Base, const std::string& F, Format* Form)
			{
				Core::Parser Buffer = F;
				size_t Offset = 0;

				if (Form != nullptr)
				{
					for (auto& Item : Form->Args)
					{
						auto R = Buffer.FindUnescaped('?', Offset);
						if (!R.Found)
							break;

						Buffer.ReplacePart(R.Start, R.End, Item);
						Offset = R.End;
					}
				}

				Base->sWriteLine(Buffer.R());
			}
			void Format::Write(Core::Console* Base, const std::string& F, Format* Form)
			{
				Core::Parser Buffer = F;
				size_t Offset = 0;

				if (Form != nullptr)
				{
					for (auto& Item : Form->Args)
					{
						auto R = Buffer.FindUnescaped('?', Offset);
						if (!R.Found)
							break;

						Buffer.ReplacePart(R.Start, R.End, Item);
						Offset = R.End;
					}
				}

				Base->sWrite(Buffer.R());
			}
			void Format::FormatBuffer(VMGlobal& Global, Core::Parser& Result, std::string& Offset, void* Ref, int TypeId)
			{
				if (TypeId < (int)VMTypeId::BOOL || TypeId >(int)VMTypeId::DOUBLE)
				{
					VMTypeInfo Type = Global.GetTypeInfoById(TypeId);
					if (!Ref)
					{
						Result.Append("null");
						return;
					}

					if (Global.GetManager()->IsNullable(TypeId))
					{
						if (TypeId == 0)
							ED_WARN("[memerr] use nullptr instead of null for initialization lists");

						Result.Append("null");
					}
					else if (VMTypeInfo::IsScriptObject(TypeId))
					{
						VMObject VObject = *(VMCObject**)Ref;
						Core::Parser Decl;

						Offset += '\t';
						for (unsigned int i = 0; i < VObject.GetPropertiesCount(); i++)
						{
							const char* Name = VObject.GetPropertyName(i);
							Decl.Append(Offset).fAppend("%s: ", Name ? Name : "");
							FormatBuffer(Global, Decl, Offset, VObject.GetAddressOfProperty(i), VObject.GetPropertyTypeId(i));
							Decl.Append(",\n");
						}

						Offset = Offset.substr(0, Offset.size() - 1);
						if (!Decl.Empty())
							Result.fAppend("\n%s{\n%s\n%s}", Offset.c_str(), Decl.Clip(Decl.Size() - 2).Get(), Offset.c_str());
						else
							Result.Append("{}");
					}
					else if (strcmp(Type.GetName(), TYPENAME_MAP) == 0)
					{
						Map* Base = *(Map**)Ref;
						Core::Parser Decl; std::string Name;

						Offset += '\t';
						for (unsigned int i = 0; i < Base->GetSize(); i++)
						{
							void* ElementRef; int ElementTypeId;
							if (!Base->GetIndex(i, &Name, &ElementRef, &ElementTypeId))
								continue;

							Decl.Append(Offset).fAppend("%s: ", Name.c_str());
							FormatBuffer(Global, Decl, Offset, ElementRef, ElementTypeId);
							Decl.Append(",\n");
						}

						Offset = Offset.substr(0, Offset.size() - 1);
						if (!Decl.Empty())
							Result.fAppend("\n%s{\n%s\n%s}", Offset.c_str(), Decl.Clip(Decl.Size() - 2).Get(), Offset.c_str());
						else
							Result.Append("{}");
					}
					else if (strcmp(Type.GetName(), TYPENAME_ARRAY) == 0)
					{
						Array* Base = *(Array**)Ref;
						int ArrayTypeId = Base->GetElementTypeId();
						Core::Parser Decl;

						Offset += '\t';
						for (unsigned int i = 0; i < Base->GetSize(); i++)
						{
							Decl.Append(Offset);
							FormatBuffer(Global, Decl, Offset, Base->At(i), ArrayTypeId);
							Decl.Append(", ");
						}

						Offset = Offset.substr(0, Offset.size() - 1);
						if (!Decl.Empty())
							Result.fAppend("\n%s[\n%s\n%s]", Offset.c_str(), Decl.Clip(Decl.Size() - 2).Get(), Offset.c_str());
						else
							Result.Append("[]");
					}
					else if (strcmp(Type.GetName(), TYPENAME_STRING) != 0)
					{
						Core::Parser Decl;
						Offset += '\t';

						Type.ForEachProperty([&Decl, &Global, &Offset, Ref, TypeId](VMTypeInfo* Base, VMFuncProperty* Prop)
						{
							Decl.Append(Offset).fAppend("%s: ", Prop->Name ? Prop->Name : "");
						FormatBuffer(Global, Decl, Offset, Base->GetProperty<void>(Ref, Prop->Offset, TypeId), Prop->TypeId);
						Decl.Append(",\n");
						});

						Offset = Offset.substr(0, Offset.size() - 1);
						if (!Decl.Empty())
							Result.fAppend("\n%s{\n%s\n%s}", Offset.c_str(), Decl.Clip(Decl.Size() - 2).Get(), Offset.c_str());
						else
							Result.fAppend("{}\n", Type.GetName());
					}
					else
						Result.Append(*(std::string*)Ref);
				}
				else
				{
					switch (TypeId)
					{
						case (int)VMTypeId::BOOL:
							Result.fAppend("%s", *(bool*)Ref ? "true" : "false");
							break;
						case (int)VMTypeId::INT8:
							Result.fAppend("%i", *(char*)Ref);
							break;
						case (int)VMTypeId::INT16:
							Result.fAppend("%i", *(short*)Ref);
							break;
						case (int)VMTypeId::INT32:
							Result.fAppend("%i", *(int*)Ref);
							break;
						case (int)VMTypeId::INT64:
							Result.fAppend("%ll", *(int64_t*)Ref);
							break;
						case (int)VMTypeId::UINT8:
							Result.fAppend("%u", *(unsigned char*)Ref);
							break;
						case (int)VMTypeId::UINT16:
							Result.fAppend("%u", *(unsigned short*)Ref);
							break;
						case (int)VMTypeId::UINT32:
							Result.fAppend("%u", *(unsigned int*)Ref);
							break;
						case (int)VMTypeId::UINT64:
							Result.fAppend("%llu", *(uint64_t*)Ref);
							break;
						case (int)VMTypeId::FLOAT:
							Result.fAppend("%f", *(float*)Ref);
							break;
						case (int)VMTypeId::DOUBLE:
							Result.fAppend("%f", *(double*)Ref);
							break;
						default:
							Result.Append("null");
							break;
					}
				}
			}
			void Format::FormatJSON(VMGlobal& Global, Core::Parser& Result, void* Ref, int TypeId)
			{
				if (TypeId < (int)VMTypeId::BOOL || TypeId >(int)VMTypeId::DOUBLE)
				{
					VMTypeInfo Type = Global.GetTypeInfoById(TypeId);
					void* Object = Type.GetInstance<void>(Ref, TypeId);

					if (!Object)
					{
						Result.Append("null");
						return;
					}

					if (VMTypeInfo::IsScriptObject(TypeId))
					{
						VMObject VObject = (VMCObject*)Object;
						Core::Parser Decl;

						for (unsigned int i = 0; i < VObject.GetPropertiesCount(); i++)
						{
							const char* Name = VObject.GetPropertyName(i);
							Decl.fAppend("\"%s\":", Name ? Name : "");
							FormatJSON(Global, Decl, VObject.GetAddressOfProperty(i), VObject.GetPropertyTypeId(i));
							Decl.Append(",");
						}

						if (!Decl.Empty())
							Result.fAppend("{%s}", Decl.Clip(Decl.Size() - 1).Get());
						else
							Result.Append("{}");
					}
					else if (strcmp(Type.GetName(), TYPENAME_MAP) == 0)
					{
						Map* Base = (Map*)Object;
						Core::Parser Decl; std::string Name;

						for (unsigned int i = 0; i < Base->GetSize(); i++)
						{
							void* ElementRef; int ElementTypeId;
							if (!Base->GetIndex(i, &Name, &ElementRef, &ElementTypeId))
								continue;

							Decl.fAppend("\"%s\":", Name.c_str());
							FormatJSON(Global, Decl, ElementRef, ElementTypeId);
							Decl.Append(",");
						}

						if (!Decl.Empty())
							Result.fAppend("{%s}", Decl.Clip(Decl.Size() - 1).Get());
						else
							Result.Append("{}");
					}
					else if (strcmp(Type.GetName(), TYPENAME_ARRAY) == 0)
					{
						Array* Base = (Array*)Object;
						int ArrayTypeId = Base->GetElementTypeId();
						Core::Parser Decl;

						for (unsigned int i = 0; i < Base->GetSize(); i++)
						{
							FormatJSON(Global, Decl, Base->At(i), ArrayTypeId);
							Decl.Append(",");
						}

						if (!Decl.Empty())
							Result.fAppend("[%s]", Decl.Clip(Decl.Size() - 1).Get());
						else
							Result.Append("[]");
					}
					else if (strcmp(Type.GetName(), TYPENAME_STRING) != 0)
					{
						Core::Parser Decl;
						Type.ForEachProperty([&Decl, &Global, Ref, TypeId](VMTypeInfo* Base, VMFuncProperty* Prop)
						{
							Decl.fAppend("\"%s\":", Prop->Name ? Prop->Name : "");
						FormatJSON(Global, Decl, Base->GetProperty<void>(Ref, Prop->Offset, TypeId), Prop->TypeId);
						Decl.Append(",");
						});

						if (!Decl.Empty())
							Result.fAppend("{%s}", Decl.Clip(Decl.Size() - 1).Get());
						else
							Result.fAppend("{}", Type.GetName());
					}
					else
						Result.fAppend("\"%s\"", ((std::string*)Object)->c_str());
				}
				else
				{
					switch (TypeId)
					{
						case (int)VMTypeId::BOOL:
							Result.fAppend("%s", *(bool*)Ref ? "true" : "false");
							break;
						case (int)VMTypeId::INT8:
							Result.fAppend("%i", *(char*)Ref);
							break;
						case (int)VMTypeId::INT16:
							Result.fAppend("%i", *(short*)Ref);
							break;
						case (int)VMTypeId::INT32:
							Result.fAppend("%i", *(int*)Ref);
							break;
						case (int)VMTypeId::INT64:
							Result.fAppend("%ll", *(int64_t*)Ref);
							break;
						case (int)VMTypeId::UINT8:
							Result.fAppend("%u", *(unsigned char*)Ref);
							break;
						case (int)VMTypeId::UINT16:
							Result.fAppend("%u", *(unsigned short*)Ref);
							break;
						case (int)VMTypeId::UINT32:
							Result.fAppend("%u", *(unsigned int*)Ref);
							break;
						case (int)VMTypeId::UINT64:
							Result.fAppend("%llu", *(uint64_t*)Ref);
							break;
						case (int)VMTypeId::FLOAT:
							Result.fAppend("%f", *(float*)Ref);
							break;
						case (int)VMTypeId::DOUBLE:
							Result.fAppend("%f", *(double*)Ref);
							break;
					}
				}
			}

			bool StreamOpen(Core::Stream* Base, const std::string& Path, Core::FileMode Mode)
			{
				return Base->Open(Path.c_str(), Mode);
			}
			std::string StreamRead(Core::Stream* Base, size_t Size)
			{
				std::string Result;
				Result.resize(Size);

				size_t Count = Base->Read(Result.data(), Size);
				if (Count < Size)
					Result.resize(Count);

				return Result;
			}
			size_t StreamWrite(Core::Stream* Base, const std::string& Data)
			{
				return Base->Write(Data.data(), Data.size());
			}
			Core::FileStream* StreamToFileStream(Core::Stream* Base)
			{
				return dynamic_cast<Core::FileStream*>(Base);
			}
			Core::GzStream* StreamToGzStream(Core::Stream* Base)
			{
				return dynamic_cast<Core::GzStream*>(Base);
			}
			Core::WebStream* StreamToWebStream(Core::Stream* Base)
			{
				return dynamic_cast<Core::WebStream*>(Base);
			}

			bool FileStreamOpen(Core::FileStream* Base, const std::string& Path, Core::FileMode Mode)
			{
				return Base->Open(Path.c_str(), Mode);
			}
			std::string FileStreamRead(Core::FileStream* Base, size_t Size)
			{
				std::string Result;
				Result.resize(Size);

				size_t Count = Base->Read(Result.data(), Size);
				if (Count < Size)
					Result.resize(Count);

				return Result;
			}
			size_t FileStreamWrite(Core::FileStream* Base, const std::string& Data)
			{
				return Base->Write(Data.data(), Data.size());
			}
			Core::Stream* FileStreamToStream(Core::FileStream* Base)
			{
				return dynamic_cast<Core::Stream*>(Base);
			}

			bool GzStreamOpen(Core::GzStream* Base, const std::string& Path, Core::FileMode Mode)
			{
				return Base->Open(Path.c_str(), Mode);
			}
			std::string GzStreamRead(Core::GzStream* Base, size_t Size)
			{
				std::string Result;
				Result.resize(Size);

				size_t Count = Base->Read(Result.data(), Size);
				if (Count < Size)
					Result.resize(Count);

				return Result;
			}
			size_t GzStreamWrite(Core::GzStream* Base, const std::string& Data)
			{
				return Base->Write(Data.data(), Data.size());
			}
			Core::Stream* GzStreamToStream(Core::GzStream* Base)
			{
				return dynamic_cast<Core::Stream*>(Base);
			}

			bool WebStreamOpen(Core::WebStream* Base, const std::string& Path, Core::FileMode Mode)
			{
				return Base->Open(Path.c_str(), Mode);
			}
			std::string WebStreamRead(Core::WebStream* Base, size_t Size)
			{
				std::string Result;
				Result.resize(Size);

				size_t Count = Base->Read(Result.data(), Size);
				if (Count < Size)
					Result.resize(Count);

				return Result;
			}
			size_t WebStreamWrite(Core::WebStream* Base, const std::string& Data)
			{
				return Base->Write(Data.data(), Data.size());
			}
			Core::Stream* WebStreamToStream(Core::WebStream* Base)
			{
				return dynamic_cast<Core::Stream*>(Base);
			}
			
			Core::TaskId ScheduleSetInterval(Core::Schedule* Base, uint64_t Mills, VMCFunction* Callback, Core::Difficulty Type)
			{
				if (!Callback)
					return ED_INVALID_TASK_ID;

				VMContext* Context = VMContext::Get();
				if (!Context)
					return ED_INVALID_TASK_ID;

				Callback->AddRef();
				Context->AddRef();
				
				Core::TaskId Task = Base->SetSeqInterval(Mills, [Context, Callback](size_t InvocationId) mutable
				{
					Callback->AddRef();
					Context->AddRef();

					if (InvocationId == 1)
					{
						Callback->Release();
						Context->Release();
					}

					Context->TryExecute(false, Callback, nullptr).Await([Context, Callback](int&&)
					{
						Callback->Release();
						Context->Release();
					});
				}, Type);

				if (Task != ED_INVALID_TASK_ID)
					return Task;

				Callback->Release();
				Context->Release();
				return ED_INVALID_TASK_ID;
			}
			Core::TaskId ScheduleSetTimeout(Core::Schedule* Base, uint64_t Mills, VMCFunction* Callback, Core::Difficulty Type)
			{
				if (!Callback)
					return ED_INVALID_TASK_ID;

				VMContext* Context = VMContext::Get();
				if (!Context)
					return ED_INVALID_TASK_ID;

				Callback->AddRef();
				Context->AddRef();

				Core::TaskId Task = Base->SetTimeout(Mills, [Context, Callback]() mutable
				{
					Context->TryExecute(false, Callback, nullptr).Await([Context, Callback](int&&)
					{
						Callback->Release();
						Context->Release();
					});
				}, Type);

				if (Task != ED_INVALID_TASK_ID)
					return Task;

				Callback->Release();
				Context->Release();
				return ED_INVALID_TASK_ID;
			}
			bool ScheduleSetImmediate(Core::Schedule* Base, VMCFunction* Callback, Core::Difficulty Type)
			{
				if (!Callback)
					return false;

				VMContext* Context = VMContext::Get();
				if (!Context)
					return false;

				Callback->AddRef();
				Context->AddRef();

				bool Queued = Base->SetTask([Context, Callback]() mutable
				{
					Context->TryExecute(false, Callback, nullptr).Await([Context, Callback](int&&)
					{
						Callback->Release();
						Context->Release();
					});
				}, Type);

				if (Queued)
					return true;

				Callback->Release();
				Context->Release();
				return false;
			}

			Array* OSDirectoryScan(const std::string& Path)
			{
				std::vector<Core::FileEntry> Entries;
				Core::OS::Directory::Scan(Path, &Entries);

				VMTypeInfo Type = VMManager::Get()->Global().GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_FILEENTRY ">@");
				return Array::Compose<Core::FileEntry>(Type.GetTypeInfo(), Entries);
			}
			Array* OSDirectoryGetMounts(const std::string& Path)
			{
				VMTypeInfo Type = VMManager::Get()->Global().GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose<std::string>(Type.GetTypeInfo(), Core::OS::Directory::GetMounts());
			}
			bool OSDirectoryCreate(const std::string& Path)
			{
				return Core::OS::Directory::Create(Path.c_str());
			}
			bool OSDirectoryRemove(const std::string& Path)
			{
				return Core::OS::Directory::Remove(Path.c_str());
			}
			bool OSDirectoryIsExists(const std::string& Path)
			{
				return Core::OS::Directory::IsExists(Path.c_str());
			}
			void OSDirectorySet(const std::string& Path)
			{
				return Core::OS::Directory::Set(Path.c_str());
			}

			bool OSFileState(const std::string& Path, Core::FileEntry& Data)
			{
				return Core::OS::File::State(Path.c_str(), &Data);
			}
			bool OSFileMove(const std::string& From, const std::string& To)
			{
				return Core::OS::File::Move(From.c_str(), To.c_str());
			}
			bool OSFileRemove(const std::string& Path)
			{
				return Core::OS::File::Remove(Path.c_str());
			}
			bool OSFileIsExists(const std::string& Path)
			{
				return Core::OS::File::IsExists(Path.c_str());
			}
			Core::FileState OSFileGetProperties(const std::string& Path)
			{
				return Core::OS::File::GetProperties(Path.c_str());
			}
			Array* OSFileReadAsArray(const std::string& Path)
			{
				VMTypeInfo Type = VMManager::Get()->Global().GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose<std::string>(Type.GetTypeInfo(), Core::OS::File::ReadAsArray(Path));
			}

			bool OSPathIsRemote(const std::string& Path)
			{
				return Core::OS::Path::IsRemote(Path.c_str());
			}
			std::string OSPathResolve(const std::string& Path)
			{
				return Core::OS::Path::Resolve(Path.c_str());
			}
			std::string OSPathResolveDirectory(const std::string& Path)
			{
				return Core::OS::Path::ResolveDirectory(Path.c_str());
			}
			std::string OSPathGetDirectory(const std::string& Path, size_t Level)
			{
				return Core::OS::Path::GetDirectory(Path.c_str(), Level);
			}
			std::string OSPathGetFilename(const std::string& Path)
			{
				return Core::OS::Path::GetFilename(Path.c_str());
			}
			std::string OSPathGetExtension(const std::string& Path)
			{
				return Core::OS::Path::GetExtension(Path.c_str());
			}

			int OSProcessExecute(const std::string& Command)
			{
				return Core::OS::Process::Execute("%s", Command.c_str());
			}
			int OSProcessAwait(void* ProcessPtr)
			{
				int ExitCode = -1;
				if (!Core::OS::Process::Await((Core::ChildProcess*)ProcessPtr, &ExitCode))
					return -1;

				return ExitCode;
			}
			bool OSProcessFree(void* ProcessPtr)
			{
				auto* Result = (Core::ChildProcess*)ProcessPtr;
				bool Success = Core::OS::Process::Free(Result);
				ED_DELETE(ChildProcess, Result);
				return Success;
			}
			void* OSProcessSpawn(const std::string& Path, Array* Args)
			{
				Core::ChildProcess* Result = ED_NEW(Core::ChildProcess);
				if (!Core::OS::Process::Spawn(Path, Array::Decompose<std::string>(Args), Result))
				{
					ED_DELETE(ChildProcess, Result);
					return nullptr;
				}

				return (void*)Result;
			}
			
			Compute::Vector2& Vector2MulEq1(Compute::Vector2& A, const Compute::Vector2& B)
			{
				return A *= B;
			}
			Compute::Vector2& Vector2MulEq2(Compute::Vector2& A, float B)
			{
				return A *= B;
			}
			Compute::Vector2& Vector2DivEq1(Compute::Vector2& A, const Compute::Vector2& B)
			{
				return A /= B;
			}
			Compute::Vector2& Vector2DivEq2(Compute::Vector2& A, float B)
			{
				return A /= B;
			}
			Compute::Vector2& Vector2AddEq1(Compute::Vector2& A, const Compute::Vector2& B)
			{
				return A += B;
			}
			Compute::Vector2& Vector2AddEq2(Compute::Vector2& A, float B)
			{
				return A += B;
			}
			Compute::Vector2& Vector2SubEq1(Compute::Vector2& A, const Compute::Vector2& B)
			{
				return A -= B;
			}
			Compute::Vector2& Vector2SubEq2(Compute::Vector2& A, float B)
			{
				return A -= B;
			}
			Compute::Vector2 Vector2Mul1(Compute::Vector2& A, const Compute::Vector2& B)
			{
				return A * B;
			}
			Compute::Vector2 Vector2Mul2(Compute::Vector2& A, float B)
			{
				return A * B;
			}
			Compute::Vector2 Vector2Div1(Compute::Vector2& A, const Compute::Vector2& B)
			{
				return A / B;
			}
			Compute::Vector2 Vector2Div2(Compute::Vector2& A, float B)
			{
				return A / B;
			}
			Compute::Vector2 Vector2Add1(Compute::Vector2& A, const Compute::Vector2& B)
			{
				return A + B;
			}
			Compute::Vector2 Vector2Add2(Compute::Vector2& A, float B)
			{
				return A + B;
			}
			Compute::Vector2 Vector2Sub1(Compute::Vector2& A, const Compute::Vector2& B)
			{
				return A - B;
			}
			Compute::Vector2 Vector2Sub2(Compute::Vector2& A, float B)
			{
				return A - B;
			}
			Compute::Vector2 Vector2Neg(Compute::Vector2& A)
			{
				return -A;
			}
			bool Vector2Eq(Compute::Vector2& A, const Compute::Vector2& B)
			{
				return A == B;
			}
			int Vector2Cmp(Compute::Vector2& A, const Compute::Vector2& B)
			{
				if (A == B)
					return 0;

				return A > B ? 1 : -1;
			}

			Compute::Vector3& Vector3MulEq1(Compute::Vector3& A, const Compute::Vector3& B)
			{
				return A *= B;
			}
			Compute::Vector3& Vector3MulEq2(Compute::Vector3& A, float B)
			{
				return A *= B;
			}
			Compute::Vector3& Vector3DivEq1(Compute::Vector3& A, const Compute::Vector3& B)
			{
				return A /= B;
			}
			Compute::Vector3& Vector3DivEq2(Compute::Vector3& A, float B)
			{
				return A /= B;
			}
			Compute::Vector3& Vector3AddEq1(Compute::Vector3& A, const Compute::Vector3& B)
			{
				return A += B;
			}
			Compute::Vector3& Vector3AddEq2(Compute::Vector3& A, float B)
			{
				return A += B;
			}
			Compute::Vector3& Vector3SubEq1(Compute::Vector3& A, const Compute::Vector3& B)
			{
				return A -= B;
			}
			Compute::Vector3& Vector3SubEq2(Compute::Vector3& A, float B)
			{
				return A -= B;
			}
			Compute::Vector3 Vector3Mul1(Compute::Vector3& A, const Compute::Vector3& B)
			{
				return A * B;
			}
			Compute::Vector3 Vector3Mul2(Compute::Vector3& A, float B)
			{
				return A * B;
			}
			Compute::Vector3 Vector3Div1(Compute::Vector3& A, const Compute::Vector3& B)
			{
				return A / B;
			}
			Compute::Vector3 Vector3Div2(Compute::Vector3& A, float B)
			{
				return A / B;
			}
			Compute::Vector3 Vector3Add1(Compute::Vector3& A, const Compute::Vector3& B)
			{
				return A + B;
			}
			Compute::Vector3 Vector3Add2(Compute::Vector3& A, float B)
			{
				return A + B;
			}
			Compute::Vector3 Vector3Sub1(Compute::Vector3& A, const Compute::Vector3& B)
			{
				return A - B;
			}
			Compute::Vector3 Vector3Sub2(Compute::Vector3& A, float B)
			{
				return A - B;
			}
			Compute::Vector3 Vector3Neg(Compute::Vector3& A)
			{
				return -A;
			}
			bool Vector3Eq(Compute::Vector3& A, const Compute::Vector3& B)
			{
				return A == B;
			}
			int Vector3Cmp(Compute::Vector3& A, const Compute::Vector3& B)
			{
				if (A == B)
					return 0;

				return A > B ? 1 : -1;
			}

			Compute::Vector4& Vector4MulEq1(Compute::Vector4& A, const Compute::Vector4& B)
			{
				return A *= B;
			}
			Compute::Vector4& Vector4MulEq2(Compute::Vector4& A, float B)
			{
				return A *= B;
			}
			Compute::Vector4& Vector4DivEq1(Compute::Vector4& A, const Compute::Vector4& B)
			{
				return A /= B;
			}
			Compute::Vector4& Vector4DivEq2(Compute::Vector4& A, float B)
			{
				return A /= B;
			}
			Compute::Vector4& Vector4AddEq1(Compute::Vector4& A, const Compute::Vector4& B)
			{
				return A += B;
			}
			Compute::Vector4& Vector4AddEq2(Compute::Vector4& A, float B)
			{
				return A += B;
			}
			Compute::Vector4& Vector4SubEq1(Compute::Vector4& A, const Compute::Vector4& B)
			{
				return A -= B;
			}
			Compute::Vector4& Vector4SubEq2(Compute::Vector4& A, float B)
			{
				return A -= B;
			}
			Compute::Vector4 Vector4Mul1(Compute::Vector4& A, const Compute::Vector4& B)
			{
				return A * B;
			}
			Compute::Vector4 Vector4Mul2(Compute::Vector4& A, float B)
			{
				return A * B;
			}
			Compute::Vector4 Vector4Div1(Compute::Vector4& A, const Compute::Vector4& B)
			{
				return A / B;
			}
			Compute::Vector4 Vector4Div2(Compute::Vector4& A, float B)
			{
				return A / B;
			}
			Compute::Vector4 Vector4Add1(Compute::Vector4& A, const Compute::Vector4& B)
			{
				return A + B;
			}
			Compute::Vector4 Vector4Add2(Compute::Vector4& A, float B)
			{
				return A + B;
			}
			Compute::Vector4 Vector4Sub1(Compute::Vector4& A, const Compute::Vector4& B)
			{
				return A - B;
			}
			Compute::Vector4 Vector4Sub2(Compute::Vector4& A, float B)
			{
				return A - B;
			}
			Compute::Vector4 Vector4Neg(Compute::Vector4& A)
			{
				return -A;
			}
			bool Vector4Eq(Compute::Vector4& A, const Compute::Vector4& B)
			{
				return A == B;
			}
			int Vector4Cmp(Compute::Vector4& A, const Compute::Vector4& B)
			{
				if (A == B)
					return 0;

				return A > B ? 1 : -1;
			}

			Compute::Matrix4x4 Matrix4x4Mul1(Compute::Matrix4x4& A, const Compute::Matrix4x4& B)
			{
				return A * B;
			}
			Compute::Vector4 Matrix4x4Mul2(Compute::Matrix4x4& A, const Compute::Vector4& B)
			{
				return A * B;
			}
			bool Matrix4x4Eq(Compute::Matrix4x4& A, const Compute::Matrix4x4& B)
			{
				return A == B;
			}
			float& Matrix4x4GetRow(Compute::Matrix4x4& Base, size_t Index)
			{
				return Base.Row[Index % 16];
			}

			Compute::Vector3 QuaternionMul1(Compute::Quaternion& A, const Compute::Vector3& B)
			{
				return A * B;
			}
			Compute::Quaternion QuaternionMul2(Compute::Quaternion& A, const Compute::Quaternion& B)
			{
				return A * B;
			}
			Compute::Quaternion QuaternionMul3(Compute::Quaternion& A, float B)
			{
				return A * B;
			}
			Compute::Quaternion QuaternionAdd(Compute::Quaternion& A, const Compute::Quaternion& B)
			{
				return A + B;
			}
			Compute::Quaternion QuaternionSub(Compute::Quaternion& A, const Compute::Quaternion& B)
			{
				return A - B;
			}

			Compute::Vector4& Frustum8CGetCorners(Compute::Frustum8C& Base, size_t Index)
			{
				return Base.Corners[Index % 8];
			}

			Compute::Vector4& Frustum6PGetCorners(Compute::Frustum6P& Base, size_t Index)
			{
				return Base.Planes[Index % 6];
			}

			size_t JointSize(Compute::Joint& Base)
			{
				return Base.Childs.size();
			}
			Compute::Joint& JointGetChilds(Compute::Joint& Base, size_t Index)
			{
				if (Base.Childs.empty())
					return Base;

				return Base.Childs[Index % Base.Childs.size()];
			}

			size_t SkinAnimatorKeySize(Compute::SkinAnimatorKey& Base)
			{
				return Base.Pose.size();
			}
			Compute::AnimatorKey& SkinAnimatorKeyGetPose(Compute::SkinAnimatorKey& Base, size_t Index)
			{
				if (Base.Pose.empty())
				{
					Base.Pose.push_back(Compute::AnimatorKey());
					return Base.Pose.front();
				}

				return Base.Pose[Index % Base.Pose.size()];
			}

			size_t SkinAnimatorClipSize(Compute::SkinAnimatorClip& Base)
			{
				return Base.Keys.size();
			}
			Compute::SkinAnimatorKey& SkinAnimatorClipGetKeys(Compute::SkinAnimatorClip& Base, size_t Index)
			{
				if (Base.Keys.empty())
				{
					Base.Keys.push_back(Compute::SkinAnimatorKey());
					return Base.Keys.front();
				}

				return Base.Keys[Index % Base.Keys.size()];
			}

			size_t KeyAnimatorClipSize(Compute::KeyAnimatorClip& Base)
			{
				return Base.Keys.size();
			}
			Compute::AnimatorKey& KeyAnimatorClipGetKeys(Compute::KeyAnimatorClip& Base, size_t Index)
			{
				if (Base.Keys.empty())
				{
					Base.Keys.push_back(Compute::AnimatorKey());
					return Base.Keys.front();
				}

				return Base.Keys[Index % Base.Keys.size()];
			}

			std::string RegexMatchTarget(Compute::RegexMatch& Base)
			{
				return Base.Pointer ? Base.Pointer : std::string();
			}

			Array* RegexResultGet(Compute::RegexResult& Base)
			{
				VMTypeInfo Type = VMManager::Get()->Global().GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_REGEXMATCH ">@");
				return Array::Compose<Compute::RegexMatch>(Type.GetTypeInfo(), Base.Get());
			}
			Array* RegexResultToArray(Compute::RegexResult& Base)
			{
				VMTypeInfo Type = VMManager::Get()->Global().GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return Array::Compose<std::string>(Type.GetTypeInfo(), Base.ToArray());
			}

			Compute::RegexResult RegexSourceMatch(Compute::RegexSource& Base, const std::string& Text)
			{
				Compute::RegexResult Result;
				Compute::Regex::Match(&Base, Result, Text);
				return Result;
			}
			std::string RegexSourceReplace(Compute::RegexSource& Base, const std::string& Text, const std::string& To)
			{
				std::string Copy = Text;
				Compute::RegexResult Result;
				Compute::Regex::Replace(&Base, To, Copy);
				return Copy;
			}

			void WebTokenSetAudience(Compute::WebToken* Base, Array* Data)
			{
				Base->SetAudience(Array::Decompose<std::string>(Data));
			}

			void CosmosQueryIndex(Compute::Cosmos* Base, VMCFunction* Overlaps, VMCFunction* Match)
			{
				VMContext* Context = VMContext::Get();
				if (!Context || !Overlaps || !Match)
					return;

				Compute::Cosmos::Iterator Iterator;
				Base->QueryIndex<void>(Iterator, [Context, Overlaps](const Compute::Bounding& Box)
				{
					Context->TryExecute(false, Overlaps, [&Box](VMContext* Context)
					{
						Context->SetArgObject(0, (void*)&Box);
					}).Wait();

					return (bool)Context->GetReturnWord();
				}, [Context, Match](void* Item)
				{
					Context->TryExecute(true, Match, [Item](VMContext* Context)
					{
						Context->SetArgAddress(0, Item);
					}).Wait();
				});
			}

			void IncludeDescAddExt(Compute::IncludeDesc& Base, const std::string& Value)
			{
				Base.Exts.push_back(Value);
			}
			void IncludeDescRemoveExt(Compute::IncludeDesc& Base, const std::string& Value)
			{
				auto It = std::find(Base.Exts.begin(), Base.Exts.end(), Value);
				if (It != Base.Exts.end())
					Base.Exts.erase(It);
			}

			void PreprocessorSetIncludeCallback(Compute::Preprocessor* Base, VMCFunction* Callback)
			{
				VMContext* Context = VMContext::Get();
				if (!Context || !Callback)
					return Base->SetIncludeCallback(nullptr);

				Base->SetIncludeCallback([Context, Callback](Compute::Preprocessor* Base, const Compute::IncludeResult& File, std::string* Out)
				{
					Context->TryExecute(true, Callback, [Base, &File, &Out](VMContext* Context)
					{
						Context->SetArgObject(0, Base);
						Context->SetArgObject(1, (void*)&File);
						Context->SetArgObject(2, Out);
					}).Wait();

					return (bool)Context->GetReturnWord();
				});
			}
			void PreprocessorSetPragmaCallback(Compute::Preprocessor* Base, VMCFunction* Callback)
			{
				VMContext* Context = VMContext::Get();
				if (!Context || !Callback)
					return Base->SetPragmaCallback(nullptr);

				VMTypeInfo Type = VMManager::Get()->Global().GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				Base->SetPragmaCallback([Type, Context, Callback](Compute::Preprocessor* Base, const std::string& Name, const std::vector<std::string>& Args)
				{
					Array* Params = Array::Compose<std::string>(Type.GetTypeInfo(), Args);
					Context->TryExecute(true, Callback, [Base, &Name, &Params](VMContext* Context)
					{
						Context->SetArgObject(0, Base);
						Context->SetArgObject(1, (void*)&Name);
						Context->SetArgObject(2, Params);
					}).Wait();

					if (Params != nullptr)
						Params->Release();

					return (bool)Context->GetReturnWord();
				});
			}
			bool PreprocessorIsDefined(Compute::Preprocessor* Base, const std::string& Name)
			{
				return Base->IsDefined(Name.c_str());
			}

			void HullShapeSetVertices(Compute::HullShape* Base, Array* Data)
			{
				Base->Vertices = Array::Decompose<Compute::Vertex>(Data);
			}
			void HullShapeSetIndices(Compute::HullShape* Base, Array* Data)
			{
				Base->Indices = Array::Decompose<int>(Data);
			}
			Array* HullShapeGetVertices(Compute::HullShape* Base)
			{
				VMTypeInfo Type = VMManager::Get()->Global().GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_VERTEX ">@");
				return Array::Compose(Type.GetTypeInfo(), Base->Vertices);
			}
			Array* HullShapeGetIndices(Compute::HullShape* Base)
			{
				VMTypeInfo Type = VMManager::Get()->Global().GetTypeInfoByDecl(TYPENAME_ARRAY "<int>@");
				return Array::Compose(Type.GetTypeInfo(), Base->Indices);
			}

			Array* SoftBodyGetIndices(Compute::SoftBody* Base)
			{
				std::vector<int> Result;
				Base->GetIndices(&Result);

				VMTypeInfo Type = VMManager::Get()->Global().GetTypeInfoByDecl(TYPENAME_ARRAY "<int>@");
				return Array::Compose(Type.GetTypeInfo(), Result);
			}
			Array* SoftBodyGetVertices(Compute::SoftBody* Base)
			{
				std::vector<Compute::Vertex> Result;
				Base->GetVertices(&Result);

				VMTypeInfo Type = VMManager::Get()->Global().GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_VERTEX ">@");
				return Array::Compose(Type.GetTypeInfo(), Result);
			}

			Compute::PConstraint* ConstraintToPConstraint(Compute::Constraint* Base)
			{
				return dynamic_cast<Compute::PConstraint*>(Base);
			}
			Compute::HConstraint* ConstraintToHConstraint(Compute::Constraint* Base)
			{
				return dynamic_cast<Compute::HConstraint*>(Base);
			}
			Compute::SConstraint* ConstraintToSConstraint(Compute::Constraint* Base)
			{
				return dynamic_cast<Compute::SConstraint*>(Base);
			}
			Compute::CTConstraint* ConstraintToCTConstraint(Compute::Constraint* Base)
			{
				return dynamic_cast<Compute::CTConstraint*>(Base);
			}
			Compute::DF6Constraint* ConstraintToDF6Constraint(Compute::Constraint* Base)
			{
				return dynamic_cast<Compute::DF6Constraint*>(Base);
			}
			Compute::Constraint* PConstraintToConstraint(Compute::PConstraint* Base)
			{
				return dynamic_cast<Compute::Constraint*>(Base);
			}
			Compute::Constraint* HConstraintToConstraint(Compute::HConstraint* Base)
			{
				return dynamic_cast<Compute::Constraint*>(Base);
			}
			Compute::Constraint* SConstraintToConstraint(Compute::SConstraint* Base)
			{
				return dynamic_cast<Compute::Constraint*>(Base);
			}
			Compute::Constraint* CTConstraintToConstraint(Compute::CTConstraint* Base)
			{
				return dynamic_cast<Compute::Constraint*>(Base);
			}
			Compute::Constraint* DF6ConstraintToConstraint(Compute::DF6Constraint* Base)
			{
				return dynamic_cast<Compute::Constraint*>(Base);
			}

			btCollisionShape* SimulatorCreateConvexHullSkinVertex(Compute::Simulator* Base, Array* Data)
			{
				auto Value = Array::Decompose<Compute::SkinVertex>(Data);
				return Base->CreateConvexHull(Value);
			}
			btCollisionShape* SimulatorCreateConvexHullVertex(Compute::Simulator* Base, Array* Data)
			{
				auto Value = Array::Decompose<Compute::Vertex>(Data);
				return Base->CreateConvexHull(Value);
			}
			btCollisionShape* SimulatorCreateConvexHullVector2(Compute::Simulator* Base, Array* Data)
			{
				auto Value = Array::Decompose<Compute::Vector2>(Data);
				return Base->CreateConvexHull(Value);
			}
			btCollisionShape* SimulatorCreateConvexHullVector3(Compute::Simulator* Base, Array* Data)
			{
				auto Value = Array::Decompose<Compute::Vector3>(Data);
				return Base->CreateConvexHull(Value);
			}
			btCollisionShape* SimulatorCreateConvexHullVector4(Compute::Simulator* Base, Array* Data)
			{
				auto Value = Array::Decompose<Compute::Vector4>(Data);
				return Base->CreateConvexHull(Value);
			}
			Array* SimulatorGetShapeVertices(Compute::Simulator* Base, btCollisionShape* Shape)
			{
				VMTypeInfo Type = VMManager::Get()->Global().GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_VECTOR3 ">@");
				return Array::Compose(Type.GetTypeInfo(), Base->GetShapeVertices(Shape));
			}

			bool AudioEffectSetFilter(Audio::AudioEffect* Base, Audio::AudioFilter* New)
			{
				Audio::AudioFilter* Copy = New;
				return Base->SetFilter(&Copy);
			}

			bool IElementDispatchEvent(Engine::GUI::IElement& Base, const std::string& Name, Core::Schema* Args)
			{
				Core::VariantArgs Data;
				if (Args != nullptr)
				{
					Data.reserve(Args->Size());
					for (auto Item : Args->GetChilds())
						Data[Item->Key] = Item->Value;
				}

				return Base.DispatchEvent(Name, Data);
			}
			Array* IElementQuerySelectorAll(Engine::GUI::IElement& Base, const std::string& Value)
			{
				VMTypeInfo Type = VMManager::Get()->Global().GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_ELEMENTNODE ">@");
				return Array::Compose(Type.GetTypeInfo(), Base.QuerySelectorAll(Value));
			}

			bool IElementDocumentDispatchEvent(Engine::GUI::IElementDocument& Base, const std::string& Name, Core::Schema* Args)
			{
				Core::VariantArgs Data;
				if (Args != nullptr)
				{
					Data.reserve(Args->Size());
					for (auto Item : Args->GetChilds())
						Data[Item->Key] = Item->Value;
				}

				return Base.DispatchEvent(Name, Data);
			}
			Array* IElementDocumentQuerySelectorAll(Engine::GUI::IElementDocument& Base, const std::string& Value)
			{
				VMTypeInfo Type = VMManager::Get()->Global().GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_ELEMENTNODE ">@");
				return Array::Compose(Type.GetTypeInfo(), Base.QuerySelectorAll(Value));
			}

			bool DataModelSetRecursive(Engine::GUI::DataNode* Node, Core::Schema* Data, size_t Depth)
			{
				size_t Index = 0;
				for (auto& Item : Data->GetChilds())
				{
					auto& Child = Node->Add(Item->Value);
					Child.SetTop((void*)(uintptr_t)Index++, Depth);
					if (Item->Value.IsObject())
						DataModelSetRecursive(&Child, Item, Depth + 1);
				}

				Node->SortTree();
				return true;
			}
			bool DataModelGetRecursive(Engine::GUI::DataNode* Node, Core::Schema* Data)
			{
				size_t Size = Node->GetSize();
				for (size_t i = 0; i < Size; i++)
				{
					auto& Child = Node->At(i);
					DataModelGetRecursive(&Child, Data->Push(Child.Get()));
				}

				return true;
			}
			bool DataModelSet(Engine::GUI::DataModel* Base, const std::string& Name, Core::Schema* Data)
			{
				if (!Data->Value.IsObject())
					return Base->SetProperty(Name, Data->Value) != nullptr;

				Engine::GUI::DataNode* Node = Base->SetArray(Name);
				if (!Node)
					return false;

				return DataModelSetRecursive(Node, Data, 0);
			}
			bool DataModelSetVar(Engine::GUI::DataModel* Base, const std::string& Name, const Core::Variant& Data)
			{
				return Base->SetProperty(Name, Data) != nullptr;
			}
			bool DataModelSetString(Engine::GUI::DataModel* Base, const std::string& Name, const std::string& Value)
			{
				return Base->SetString(Name, Value) != nullptr;
			}
			bool DataModelSetInteger(Engine::GUI::DataModel* Base, const std::string& Name, int64_t Value)
			{
				return Base->SetInteger(Name, Value) != nullptr;
			}
			bool DataModelSetFloat(Engine::GUI::DataModel* Base, const std::string& Name, float Value)
			{
				return Base->SetFloat(Name, Value) != nullptr;
			}
			bool DataModelSetDouble(Engine::GUI::DataModel* Base, const std::string& Name, double Value)
			{
				return Base->SetDouble(Name, Value) != nullptr;
			}
			bool DataModelSetBoolean(Engine::GUI::DataModel* Base, const std::string& Name, bool Value)
			{
				return Base->SetBoolean(Name, Value) != nullptr;
			}
			bool DataModelSetPointer(Engine::GUI::DataModel* Base, const std::string& Name, void* Value)
			{
				return Base->SetPointer(Name, Value) != nullptr;
			}
			bool DataModelSetCallback(Engine::GUI::DataModel* Base, const std::string& Name, VMCFunction* Callback)
			{
				VMContext* Context = VMContext::Get();
				if (!Context)
					return false;

				if (!Callback)
					return false;

				Callback->AddRef();
				Context->AddRef();

				VMTypeInfo Type = VMManager::Get()->Global().GetTypeInfoByDecl(TYPENAME_ARRAY "<" TYPENAME_VARIANT ">@");
				Base->SetDetachCallback([Callback, Context]()
				{
					Callback->Release();
					Context->Release();
				});

				return Base->SetCallback(Name, [Type, Context, Callback](Engine::GUI::IEvent& Wrapper, const Core::VariantList& Args)
				{
					Engine::GUI::IEvent Event = Wrapper.Copy();
					Array* Data = Array::Compose(Type.GetTypeInfo(), Args);
					Context->TryExecute(false, Callback, [Event, &Data](VMContext* Context)
					{
						Engine::GUI::IEvent Copy = Event;
						Context->SetArgObject(0, &Copy);
						Context->SetArgObject(1, &Data);
					}).Await([Event](int&&)
					{
						Engine::GUI::IEvent Copy = Event;
						Copy.Release();
					});
				});
			}
			Core::Schema* DataModelGet(Engine::GUI::DataModel* Base, const std::string& Name)
			{
				Engine::GUI::DataNode* Node = Base->GetProperty(Name);
				if (!Node)
					return nullptr;

				Core::Schema* Result = new Core::Schema(Node->Get());
				if (Result->Value.IsObject())
					DataModelGetRecursive(Node, Result);

				return Result;
			}

			Engine::GUI::IElement ContextGetFocusElement(Engine::GUI::Context* Base, const Compute::Vector2& Value)
			{
				return Base->GetElementAtPoint(Value);
			}

			ModelListener::ModelListener(VMCFunction* NewCallback) noexcept : Engine::GUI::Listener(Bind(NewCallback)), Source(NewCallback), Context(nullptr)
			{
			}
			ModelListener::ModelListener(const std::string& FunctionName) noexcept : Engine::GUI::Listener(FunctionName), Source(nullptr), Context(nullptr)
			{
			}
			ModelListener::~ModelListener() noexcept
			{
				if (Source != nullptr)
					Source->Release();

				if (Context != nullptr)
					Context->Release();
			}
			Engine::GUI::EventCallback ModelListener::Bind(VMCFunction* Callback)
			{
				if (Context != nullptr)
					Context->Release();
				Context = VMContext::Get();

				if (Source != nullptr)
					Source->Release();
				Source = Callback;

				if (!Context || !Source)
					return nullptr;

				Source->AddRef();
				Context->AddRef();

				return [this](Engine::GUI::IEvent& Wrapper)
				{
					Engine::GUI::IEvent Event = Wrapper.Copy();
					Context->TryExecute(false, Source, [Event](VMContext* Context)
					{
						Engine::GUI::IEvent Copy = Event;
						Context->SetArgObject(0, &Copy);
					}).Await([Event](int&&)
					{
						Engine::GUI::IEvent Copy = Event;
						Copy.Release();
					});
				};
			}

			bool Registry::LoadCTypes(VMManager* Manager)
			{
				ED_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
				VMCManager* Engine = Manager->GetEngine();
#ifdef ED_64
				Engine->RegisterTypedef("usize", "uint64");
#else
				Engine->RegisterTypedef("usize", "uint32");
#endif
				return Engine->RegisterObjectType("uptr", 0, asOBJ_REF | asOBJ_NOCOUNT) >= 0;
			}
			bool Registry::LoadAny(VMManager* Manager)
			{
				ED_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
				VMCManager* Engine = Manager->GetEngine();
				Engine->RegisterObjectType("any", sizeof(Any), asOBJ_REF | asOBJ_GC);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_FACTORY, "any@ f()", asFUNCTION(Any::Factory1), asCALL_GENERIC);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_FACTORY, "any@ f(?&in) explicit", asFUNCTION(Any::Factory2), asCALL_GENERIC);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_FACTORY, "any@ f(const int64&in) explicit", asFUNCTION(Any::Factory2), asCALL_GENERIC);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_FACTORY, "any@ f(const double&in) explicit", asFUNCTION(Any::Factory2), asCALL_GENERIC);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_ADDREF, "void f()", asMETHOD(Any, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_RELEASE, "void f()", asMETHOD(Any, Release), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(Any, GetRefCount), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(Any, SetFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(Any, GetFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(Any, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("any", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(Any, ReleaseAllHandles), asCALL_THISCALL);
				Engine->RegisterObjectMethod("any", "any &opAssign(any&in)", asFUNCTION(Any::Assignment), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("any", "void store(?&in)", asMETHODPR(Any, Store, (void*, int), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("any", "bool retrieve(?&out)", asMETHODPR(Any, Retrieve, (void*, int) const, bool), asCALL_THISCALL);
				return true;
			}
			bool Registry::LoadArray(VMManager* Manager)
			{
				ED_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
				VMCManager* Engine = Manager->GetEngine();
				Engine->SetTypeInfoUserDataCleanupCallback(Array::CleanupTypeInfoCache, ARRAY_CACHE);
				Engine->RegisterObjectType("array<class T>", 0, asOBJ_REF | asOBJ_GC | asOBJ_TEMPLATE);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(Array::TemplateCallback), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_FACTORY, "array<T>@ f(int&in)", asFUNCTIONPR(Array::Create, (VMCTypeInfo*), Array*), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_FACTORY, "array<T>@ f(int&in, usize length) explicit", asFUNCTIONPR(Array::Create, (VMCTypeInfo*, size_t), Array*), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_FACTORY, "array<T>@ f(int&in, usize length, const T &in Value)", asFUNCTIONPR(Array::Create, (VMCTypeInfo*, size_t, void*), Array*), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_LIST_FACTORY, "array<T>@ f(int&in Type, int&in InitList) {repeat T}", asFUNCTIONPR(Array::Create, (VMCTypeInfo*, void*), Array*), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(Array, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(Array, Release), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "T &opIndex(usize Index)", asMETHODPR(Array, At, (size_t), void*), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "const T &opIndex(usize Index) const", asMETHODPR(Array, At, (size_t) const, const void*), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "array<T> &opAssign(const array<T>&in)", asMETHOD(Array, operator=), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void insert_at(usize Index, const T&in Value)", asMETHODPR(Array, InsertAt, (size_t, void*), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void insert_at(usize Index, const array<T>& Array)", asMETHODPR(Array, InsertAt, (size_t, const Array&), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void insert_last(const T&in Value)", asMETHOD(Array, InsertLast), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void push(const T&in Value)", asMETHOD(Array, InsertLast), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void pop()", asMETHOD(Array, RemoveLast), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void remove_at(usize Index)", asMETHOD(Array, RemoveAt), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void remove_last()", asMETHOD(Array, RemoveLast), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void remove_range(usize Start, usize Count)", asMETHOD(Array, RemoveRange), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "usize size() const", asMETHOD(Array, GetSize), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void reserve(usize length)", asMETHOD(Array, Reserve), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void resize(usize length)", asMETHODPR(Array, Resize, (size_t), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void sort_asc()", asMETHODPR(Array, SortAsc, (), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void sort_asc(usize StartAt, usize Count)", asMETHODPR(Array, SortAsc, (size_t, size_t), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void sort_desc()", asMETHODPR(Array, SortDesc, (), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void sort_desc(usize StartAt, usize Count)", asMETHODPR(Array, SortDesc, (size_t, size_t), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void reverse()", asMETHOD(Array, Reverse), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "usize find(const T&in if_handle_then_const Value) const", asMETHODPR(Array, Find, (void*) const, int), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "usize find(usize StartAt, const T&in if_handle_then_const Value) const", asMETHODPR(Array, Find, (size_t, void*) const, int), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "usize find_by_ref(const T&in if_handle_then_const Value) const", asMETHODPR(Array, FindByRef, (void*) const, int), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "usize find_by_ref(usize StartAt, const T&in if_handle_then_const Value) const", asMETHODPR(Array, FindByRef, (size_t, void*) const, int), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "bool opEquals(const array<T>&in) const", asMETHOD(Array, operator==), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "bool empty() const", asMETHOD(Array, IsEmpty), asCALL_THISCALL);
				Engine->RegisterFuncdef("bool array<T>::less(const T&in if_handle_then_const a, const T&in if_handle_then_const b)");
				Engine->RegisterObjectMethod("array<T>", "void sort(const less &in, usize StartAt = 0, usize Count = usize(-1))", asMETHODPR(Array, Sort, (asIScriptFunction*, size_t, size_t), void), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(Array, GetRefCount), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(Array, SetFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(Array, GetFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(Array, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(Array, ReleaseAllHandles), asCALL_THISCALL);
				Engine->RegisterDefaultArrayType("array<T>");
				return true;
			}
			bool Registry::LoadComplex(VMManager* Manager)
			{
				ED_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
				VMCManager* Engine = Manager->GetEngine();
				Engine->RegisterObjectType("complex", sizeof(Complex), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Complex>() | asOBJ_APP_CLASS_ALLFLOATS);
				Engine->RegisterObjectProperty("complex", "float R", asOFFSET(Complex, R));
				Engine->RegisterObjectProperty("complex", "float I", asOFFSET(Complex, I));
				Engine->RegisterObjectBehaviour("complex", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Complex::DefaultConstructor), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("complex", asBEHAVE_CONSTRUCT, "void f(const complex &in)", asFUNCTION(Complex::CopyConstructor), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("complex", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTION(Complex::ConvConstructor), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("complex", asBEHAVE_CONSTRUCT, "void f(float, float)", asFUNCTION(Complex::InitConstructor), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("complex", asBEHAVE_LIST_CONSTRUCT, "void f(const int &in) {float, float}", asFUNCTION(Complex::ListConstructor), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("complex", "complex &opAddAssign(const complex &in)", asMETHODPR(Complex, operator+=, (const Complex&), Complex&), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "complex &opSubAssign(const complex &in)", asMETHODPR(Complex, operator-=, (const Complex&), Complex&), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "complex &opMulAssign(const complex &in)", asMETHODPR(Complex, operator*=, (const Complex&), Complex&), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "complex &opDivAssign(const complex &in)", asMETHODPR(Complex, operator/=, (const Complex&), Complex&), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "bool opEquals(const complex &in) const", asMETHODPR(Complex, operator==, (const Complex&) const, bool), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "complex opAdd(const complex &in) const", asMETHODPR(Complex, operator+, (const Complex&) const, Complex), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "complex opSub(const complex &in) const", asMETHODPR(Complex, operator-, (const Complex&) const, Complex), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "complex opMul(const complex &in) const", asMETHODPR(Complex, operator*, (const Complex&) const, Complex), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "complex opDiv(const complex &in) const", asMETHODPR(Complex, operator/, (const Complex&) const, Complex), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "float abs() const", asMETHOD(Complex, Length), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "complex get_ri() const property", asMETHOD(Complex, GetRI), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "complex get_ir() const property", asMETHOD(Complex, GetIR), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "void set_ri(const complex &in) property", asMETHOD(Complex, SetRI), asCALL_THISCALL);
				Engine->RegisterObjectMethod("complex", "void set_ir(const complex &in) property", asMETHOD(Complex, SetIR), asCALL_THISCALL);
				return true;
			}
			bool Registry::LoadMap(VMManager* Manager)
			{
				ED_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
				VMCManager* Engine = Manager->GetEngine();
				Engine->RegisterObjectType("map_key", sizeof(MapKey), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_GC | asGetTypeTraits<MapKey>());
				Engine->RegisterObjectBehaviour("map_key", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Map::KeyConstruct), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("map_key", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Map::KeyDestruct), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("map_key", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(MapKey, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("map_key", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(MapKey, FreeValue), asCALL_THISCALL);
				Engine->RegisterObjectMethod("map_key", "map_key &opAssign(const map_key &in)", asFUNCTIONPR(Map::KeyopAssign, (const MapKey&, MapKey*), MapKey&), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("map_key", "map_key &opHndlAssign(const ?&in)", asFUNCTIONPR(Map::KeyopAssign, (void*, int, MapKey*), MapKey&), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("map_key", "map_key &opHndlAssign(const map_key &in)", asFUNCTIONPR(Map::KeyopAssign, (const MapKey&, MapKey*), MapKey&), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("map_key", "map_key &opAssign(const ?&in)", asFUNCTIONPR(Map::KeyopAssign, (void*, int, MapKey*), MapKey&), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("map_key", "map_key &opAssign(double)", asFUNCTIONPR(Map::KeyopAssign, (double, MapKey*), MapKey&), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("map_key", "map_key &opAssign(int64)", asFUNCTIONPR(Map::KeyopAssign, (as_int64_t, MapKey*), MapKey&), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("map_key", "void opCast(?&out)", asFUNCTIONPR(Map::KeyopCast, (void*, int, MapKey*), void), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("map_key", "void opConv(?&out)", asFUNCTIONPR(Map::KeyopCast, (void*, int, MapKey*), void), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("map_key", "int64 opConv()", asFUNCTIONPR(Map::KeyopConvInt, (MapKey*), as_int64_t), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("map_key", "double opConv()", asFUNCTIONPR(Map::KeyopConvDouble, (MapKey*), double), asCALL_CDECL_OBJLAST);

				Engine->RegisterObjectType("map", sizeof(Map), asOBJ_REF | asOBJ_GC);
				Engine->RegisterObjectBehaviour("map", asBEHAVE_FACTORY, "map@ f()", asFUNCTION(Map::Factory), asCALL_GENERIC);
				Engine->RegisterObjectBehaviour("map", asBEHAVE_LIST_FACTORY, "map @f(int &in) {repeat {string, ?}}", asFUNCTION(Map::ListFactory), asCALL_GENERIC);
				Engine->RegisterObjectBehaviour("map", asBEHAVE_ADDREF, "void f()", asMETHOD(Map, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("map", asBEHAVE_RELEASE, "void f()", asMETHOD(Map, Release), asCALL_THISCALL);
				Engine->RegisterObjectMethod("map", "map &opAssign(const map &in)", asMETHODPR(Map, operator=, (const Map&), Map&), asCALL_THISCALL);
				Engine->RegisterObjectMethod("map", "void set(const string &in, const ?&in)", asMETHODPR(Map, Set, (const std::string&, void*, int), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("map", "bool get(const string &in, ?&out) const", asMETHODPR(Map, Get, (const std::string&, void*, int) const, bool), asCALL_THISCALL);
				Engine->RegisterObjectMethod("map", "bool exists(const string &in) const", asMETHOD(Map, Exists), asCALL_THISCALL);
				Engine->RegisterObjectMethod("map", "bool empty() const", asMETHOD(Map, IsEmpty), asCALL_THISCALL);
				Engine->RegisterObjectMethod("map", "usize size() const", asMETHOD(Map, GetSize), asCALL_THISCALL);
				Engine->RegisterObjectMethod("map", "bool delete(const string &in)", asMETHOD(Map, Delete), asCALL_THISCALL);
				Engine->RegisterObjectMethod("map", "void delete_all()", asMETHOD(Map, DeleteAll), asCALL_THISCALL);
				Engine->RegisterObjectMethod("map", "array<string>@ get_keys() const", asMETHOD(Map, GetKeys), asCALL_THISCALL);
				Engine->RegisterObjectMethod("map", "map_key &opIndex(const string &in)", asMETHODPR(Map, operator[], (const std::string&), MapKey*), asCALL_THISCALL);
				Engine->RegisterObjectMethod("map", "const map_key &opIndex(const string &in) const", asMETHODPR(Map, operator[], (const std::string&) const, const MapKey*), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("map", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(Map, GetRefCount), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("map", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(Map, SetGCFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("map", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(Map, GetGCFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("map", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(Map, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("map", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(Map, ReleaseAllReferences), asCALL_THISCALL);

				Map::Setup(Engine);
				return true;
			}
			bool Registry::LoadGrid(VMManager* Manager)
			{
				VMCManager* Engine = Manager->GetEngine();
				if (!Engine)
					return false;

				Engine->RegisterObjectType("grid<class T>", 0, asOBJ_REF | asOBJ_GC | asOBJ_TEMPLATE);
				Engine->RegisterObjectBehaviour("grid<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(Grid::TemplateCallback), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("grid<T>", asBEHAVE_FACTORY, "grid<T>@ f(int&in)", asFUNCTIONPR(Grid::Create, (VMCTypeInfo*), Grid*), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("grid<T>", asBEHAVE_FACTORY, "grid<T>@ f(int&in, usize, usize)", asFUNCTIONPR(Grid::Create, (VMCTypeInfo*, size_t, size_t), Grid*), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("grid<T>", asBEHAVE_FACTORY, "grid<T>@ f(int&in, usize, usize, const T &in)", asFUNCTIONPR(Grid::Create, (VMCTypeInfo*, size_t, size_t, void*), Grid*), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("grid<T>", asBEHAVE_LIST_FACTORY, "grid<T>@ f(int&in Type, int&in InitList) {repeat {repeat_same T}}", asFUNCTIONPR(Grid::Create, (VMCTypeInfo*, void*), Grid*), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("grid<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(Grid, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("grid<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(Grid, Release), asCALL_THISCALL);
				Engine->RegisterObjectMethod("grid<T>", "T &opIndex(usize, usize)", asMETHODPR(Grid, At, (size_t, size_t), void*), asCALL_THISCALL);
				Engine->RegisterObjectMethod("grid<T>", "const T &opIndex(usize, usize) const", asMETHODPR(Grid, At, (size_t, size_t) const, const void*), asCALL_THISCALL);
				Engine->RegisterObjectMethod("grid<T>", "void resize(usize, usize)", asMETHODPR(Grid, Resize, (size_t, size_t), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("grid<T>", "usize width() const", asMETHOD(Grid, GetWidth), asCALL_THISCALL);
				Engine->RegisterObjectMethod("grid<T>", "usize height() const", asMETHOD(Grid, GetHeight), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("grid<T>", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(Grid, GetRefCount), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("grid<T>", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(Grid, SetFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("grid<T>", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(Grid, GetFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("grid<T>", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(Grid, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("grid<T>", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(Grid, ReleaseAllHandles), asCALL_THISCALL);

				return true;
			}
			bool Registry::LoadRef(VMManager* Manager)
			{
				VMCManager* Engine = Manager->GetEngine();
				if (!Engine)
					return false;

				Engine->RegisterObjectType("ref", sizeof(Ref), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_GC | asGetTypeTraits<Ref>());
				Engine->RegisterObjectBehaviour("ref", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR(Ref::Construct, (Ref*), void), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectBehaviour("ref", asBEHAVE_CONSTRUCT, "void f(const ref &in)", asFUNCTIONPR(Ref::Construct, (Ref*, const Ref&), void), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectBehaviour("ref", asBEHAVE_CONSTRUCT, "void f(const ?&in)", asFUNCTIONPR(Ref::Construct, (Ref*, void*, int), void), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectBehaviour("ref", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR(Ref::Destruct, (Ref*), void), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectBehaviour("ref", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(Ref, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("ref", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(Ref, ReleaseReferences), asCALL_THISCALL);
				Engine->RegisterObjectMethod("ref", "void opCast(?&out)", asMETHODPR(Ref, Cast, (void**, int), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("ref", "ref &opHndlAssign(const ref &in)", asMETHOD(Ref, operator=), asCALL_THISCALL);
				Engine->RegisterObjectMethod("ref", "ref &opHndlAssign(const ?&in)", asMETHOD(Ref, Assign), asCALL_THISCALL);
				Engine->RegisterObjectMethod("ref", "bool opEquals(const ref &in) const", asMETHODPR(Ref, operator==, (const Ref&) const, bool), asCALL_THISCALL);
				Engine->RegisterObjectMethod("ref", "bool opEquals(const ?&in) const", asMETHODPR(Ref, Equals, (void*, int) const, bool), asCALL_THISCALL);
				return true;
			}
			bool Registry::LoadWeakRef(VMManager* Manager)
			{
				VMCManager* Engine = Manager->GetEngine();
				if (!Engine)
					return false;

				Engine->RegisterObjectType("weak_ref<class T>", sizeof(WeakRef), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_TEMPLATE | asOBJ_APP_CLASS_DAK);
				Engine->RegisterObjectBehaviour("weak_ref<T>", asBEHAVE_CONSTRUCT, "void f(int&in)", asFUNCTION(WeakRef::Construct), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("weak_ref<T>", asBEHAVE_CONSTRUCT, "void f(int&in, T@+)", asFUNCTION(WeakRef::Construct2), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("weak_ref<T>", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(WeakRef::Destruct), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("weak_ref<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(WeakRef::TemplateCallback), asCALL_CDECL);
				Engine->RegisterObjectMethod("weak_ref<T>", "T@ opImplCast()", asMETHOD(WeakRef, Get), asCALL_THISCALL);
				Engine->RegisterObjectMethod("weak_ref<T>", "T@ get() const", asMETHODPR(WeakRef, Get, () const, void*), asCALL_THISCALL);
				Engine->RegisterObjectMethod("weak_ref<T>", "weak_ref<T> &opHndlAssign(const weak_ref<T> &in)", asMETHOD(WeakRef, operator=), asCALL_THISCALL);
				Engine->RegisterObjectMethod("weak_ref<T>", "weak_ref<T> &opAssign(const weak_ref<T> &in)", asMETHOD(WeakRef, operator=), asCALL_THISCALL);
				Engine->RegisterObjectMethod("weak_ref<T>", "bool opEquals(const weak_ref<T> &in) const", asMETHODPR(WeakRef, operator==, (const WeakRef&) const, bool), asCALL_THISCALL);
				Engine->RegisterObjectMethod("weak_ref<T>", "weak_ref<T> &opHndlAssign(T@)", asMETHOD(WeakRef, Set), asCALL_THISCALL);
				Engine->RegisterObjectMethod("weak_ref<T>", "bool opEquals(const T@+) const", asMETHOD(WeakRef, Equals), asCALL_THISCALL);
				Engine->RegisterObjectType("const_weak_ref<class T>", sizeof(WeakRef), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_TEMPLATE | asOBJ_APP_CLASS_DAK);
				Engine->RegisterObjectBehaviour("const_weak_ref<T>", asBEHAVE_CONSTRUCT, "void f(int&in)", asFUNCTION(WeakRef::Construct), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("const_weak_ref<T>", asBEHAVE_CONSTRUCT, "void f(int&in, const T@+)", asFUNCTION(WeakRef::Construct2), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("const_weak_ref<T>", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(WeakRef::Destruct), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("const_weak_ref<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(WeakRef::TemplateCallback), asCALL_CDECL);
				Engine->RegisterObjectMethod("const_weak_ref<T>", "const T@ opImplCast() const", asMETHOD(WeakRef, Get), asCALL_THISCALL);
				Engine->RegisterObjectMethod("const_weak_ref<T>", "const T@ get() const", asMETHODPR(WeakRef, Get, () const, void*), asCALL_THISCALL);
				Engine->RegisterObjectMethod("const_weak_ref<T>", "const_weak_ref<T> &opHndlAssign(const const_weak_ref<T> &in)", asMETHOD(WeakRef, operator=), asCALL_THISCALL);
				Engine->RegisterObjectMethod("const_weak_ref<T>", "const_weak_ref<T> &opAssign(const const_weak_ref<T> &in)", asMETHOD(WeakRef, operator=), asCALL_THISCALL);
				Engine->RegisterObjectMethod("const_weak_ref<T>", "bool opEquals(const const_weak_ref<T> &in) const", asMETHODPR(WeakRef, operator==, (const WeakRef&) const, bool), asCALL_THISCALL);
				Engine->RegisterObjectMethod("const_weak_ref<T>", "const_weak_ref<T> &opHndlAssign(const T@)", asMETHOD(WeakRef, Set), asCALL_THISCALL);
				Engine->RegisterObjectMethod("const_weak_ref<T>", "bool opEquals(const T@+) const", asMETHOD(WeakRef, Equals), asCALL_THISCALL);
				Engine->RegisterObjectMethod("const_weak_ref<T>", "const_weak_ref<T> &opHndlAssign(const weak_ref<T> &in)", asMETHOD(WeakRef, operator=), asCALL_THISCALL);
				Engine->RegisterObjectMethod("const_weak_ref<T>", "bool opEquals(const weak_ref<T> &in) const", asMETHODPR(WeakRef, operator==, (const WeakRef&) const, bool), asCALL_THISCALL);
				return true;
			}
			bool Registry::LoadMath(VMManager* Manager)
			{
				VMCManager* Engine = Manager->GetEngine();
				if (!Engine)
					return false;

				Engine->RegisterGlobalFunction("float fp_from_ieee(uint)", asFUNCTIONPR(Math::FpFromIEEE, (uint32_t), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("uint fp_to_ieee(float)", asFUNCTIONPR(Math::FpToIEEE, (float), uint32_t), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double fp_from_ieee(uint64)", asFUNCTIONPR(Math::FpFromIEEE, (as_uint64_t), double), asCALL_CDECL);
				Engine->RegisterGlobalFunction("uint64 fp_to_ieee(double)", asFUNCTIONPR(Math::FpToIEEE, (double), as_uint64_t), asCALL_CDECL);
				Engine->RegisterGlobalFunction("bool close_to(float, float, float = 0.00001f)", asFUNCTIONPR(Math::CloseTo, (float, float, float), bool), asCALL_CDECL);
				Engine->RegisterGlobalFunction("bool close_to(double, double, double = 0.0000000001)", asFUNCTIONPR(Math::CloseTo, (double, double, double), bool), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float rad2deg()", asFUNCTIONPR(Compute::Mathf::Rad2Deg, (), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float deg2rad()", asFUNCTIONPR(Compute::Mathf::Deg2Rad, (), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float pi_value()", asFUNCTIONPR(Compute::Mathf::Pi, (), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float clamp(float, float, float)", asFUNCTIONPR(Compute::Mathf::Clamp, (float, float, float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float acotan(float)", asFUNCTIONPR(Compute::Mathf::Acotan, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float cotan(float)", asFUNCTIONPR(Compute::Mathf::Cotan, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float minf(float, float)", asFUNCTIONPR(Compute::Mathf::Min, (float, float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float maxf(float, float)", asFUNCTIONPR(Compute::Mathf::Max, (float, float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float saturate(float)", asFUNCTIONPR(Compute::Mathf::Saturate, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float lerp(float, float, float)", asFUNCTIONPR(Compute::Mathf::Lerp, (float, float, float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float strong_lerp(float, float, float)", asFUNCTIONPR(Compute::Mathf::StrongLerp, (float, float, float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float angle_saturate(float)", asFUNCTIONPR(Compute::Mathf::SaturateAngle, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float angle_distance(float, float)", asFUNCTIONPR(Compute::Mathf::AngleDistance, (float, float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float angle_lerp(float, float, float)", asFUNCTIONPR(Compute::Mathf::AngluarLerp, (float, float, float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float randomf()", asFUNCTIONPR(Compute::Mathf::Random, (), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float randomf_mag()", asFUNCTIONPR(Compute::Mathf::RandomMag, (), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float randomf_range()", asFUNCTIONPR(Compute::Mathf::Random, (float, float), float), asCALL_CDECL);
#if !defined(_WIN32_WCE)
				Engine->RegisterGlobalFunction("float cos(float)", asFUNCTIONPR(cosf, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float sin(float)", asFUNCTIONPR(sinf, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float tan(float)", asFUNCTIONPR(tanf, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float acos(float)", asFUNCTIONPR(acosf, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float asin(float)", asFUNCTIONPR(asinf, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float atan(float)", asFUNCTIONPR(atanf, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float atan2(float, float)", asFUNCTIONPR(atan2f, (float, float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float cosh(float)", asFUNCTIONPR(coshf, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float sinh(float)", asFUNCTIONPR(sinhf, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float tanh(float)", asFUNCTIONPR(tanhf, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float log(float)", asFUNCTIONPR(logf, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float log10(float)", asFUNCTIONPR(log10f, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float pow(float, float)", asFUNCTIONPR(powf, (float, float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float sqrt(float)", asFUNCTIONPR(sqrtf, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float ceil(float)", asFUNCTIONPR(ceilf, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float abs(float)", asFUNCTIONPR(fabsf, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float floor(float)", asFUNCTIONPR(floorf, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float fraction(float)", asFUNCTIONPR(fracf, (float), float), asCALL_CDECL);
#else
				Engine->RegisterGlobalFunction("double cos(double)", asFUNCTIONPR(cos, (double), double), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double sin(double)", asFUNCTIONPR(sin, (double), double), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double tan(double)", asFUNCTIONPR(tan, (double), double), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double acos(double)", asFUNCTIONPR(acos, (double), double), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double asin(double)", asFUNCTIONPR(asin, (double), double), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double atan(double)", asFUNCTIONPR(atan, (double), double), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double atan2(double, double)", asFUNCTIONPR(atan2, (double, double), double), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double cosh(double)", asFUNCTIONPR(cosh, (double), double), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double sinh(double)", asFUNCTIONPR(sinh, (double), double), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double tanh(double)", asFUNCTIONPR(tanh, (double), double), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double log(double)", asFUNCTIONPR(log, (double), double), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double log10(double)", asFUNCTIONPR(log10, (double), double), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double pow(double, double)", asFUNCTIONPR(pow, (double, double), double), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double sqrt(double)", asFUNCTIONPR(sqrt, (double), double), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double ceil(double)", asFUNCTIONPR(ceil, (double), double), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double abs(double)", asFUNCTIONPR(fabs, (double), double), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double floor(double)", asFUNCTIONPR(floor, (double), double), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double fraction(double)", asFUNCTIONPR(frac, (double), double), asCALL_CDECL);
#endif
				return true;
			}
			bool Registry::LoadString(VMManager* Manager)
			{
				VMCManager* Engine = Manager->GetEngine();
				if (!Engine)
					return false;

				Engine->RegisterObjectType("string", sizeof(std::string), asOBJ_VALUE | asGetTypeTraits<std::string>());
				Engine->RegisterStringFactory("string", StringFactory::Get());
				Engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(String::Construct), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f(const string &in)", asFUNCTION(String::CopyConstruct), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectBehaviour("string", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(String::Destruct), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAssign(const string &in)", asMETHODPR(std::string, operator =, (const std::string&), std::string&), asCALL_THISCALL);
				Engine->RegisterObjectMethod("string", "string &opAddAssign(const string &in)", asFUNCTION(String::AddAssignTo), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "bool opEquals(const string &in) const", asFUNCTIONPR(String::Equals, (const std::string&, const std::string&), bool), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "int opCmp(const string &in) const", asFUNCTION(String::Cmp), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "string opAdd(const string &in) const", asFUNCTIONPR(std::operator +, (const std::string&, const std::string&), std::string), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "usize size() const", asFUNCTION(String::Length), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "void resize(usize)", asFUNCTION(String::Resize), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "bool empty() const", asFUNCTION(String::IsEmpty), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "uint8 &opIndex(usize)", asFUNCTION(String::CharAt), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "const uint8 &opIndex(usize) const", asFUNCTION(String::CharAt), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAssign(double)", asFUNCTION(String::AssignDoubleTo), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAddAssign(double)", asFUNCTION(String::AddAssignDoubleTo), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string opAdd(double) const", asFUNCTION(String::AddDouble1), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "string opAdd_r(double) const", asFUNCTION(String::AddDouble2), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAssign(float)", asFUNCTION(String::AssignFloatTo), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAddAssign(float)", asFUNCTION(String::AddAssignFloatTo), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string opAdd(float) const", asFUNCTION(String::AddFloat1), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "string opAdd_r(float) const", asFUNCTION(String::AddFloat2), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAssign(int64)", asFUNCTION(String::AssignInt64To), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAddAssign(int64)", asFUNCTION(String::AddAssignInt64To), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string opAdd(int64) const", asFUNCTION(String::AddInt641), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string opAdd_r(int64) const", asFUNCTION(String::AddInt642), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "string &opAssign(uint64)", asFUNCTION(String::AssignUInt64To), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAddAssign(uint64)", asFUNCTION(String::AddAssignUInt64To), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string opAdd(uint64) const", asFUNCTION(String::AddUInt641), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "string opAdd_r(uint64) const", asFUNCTION(String::AddUInt642), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAssign(bool)", asFUNCTION(String::AssignBoolTo), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAddAssign(bool)", asFUNCTION(String::AddAssignBoolTo), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string opAdd(bool) const", asFUNCTION(String::AddBool1), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "string opAdd_r(bool) const", asFUNCTION(String::AddBool2), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string needle(usize = 0, int = -1) const", asFUNCTION(String::Sub), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "int find_first(const string &in, usize = 0) const", asFUNCTION(String::FindFirst), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "int find_first_of(const string &in, usize = 0) const", asFUNCTION(String::FindFirstOf), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "int find_first_not_of(const string &in, usize = 0) const", asFUNCTION(String::FindFirstNotOf), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "int find_last(const string &in, int = -1) const", asFUNCTION(String::FindLast), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "int find_last_of(const string &in, int = -1) const", asFUNCTION(String::FindLastOf), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "int find_last_not_of(const string &in, int = -1) const", asFUNCTION(String::FindLastNotOf), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "void insert(usize, const string &in)", asFUNCTION(String::Insert), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "void erase(usize, int Count = -1)", asFUNCTION(String::Erase), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string replace(const string &in, const string &in, usize = 0)", asFUNCTION(String::Replace), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "array<string>@ split(const string &in) const", asFUNCTION(String::Split), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string to_lower() const", asFUNCTION(String::ToLower), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string to_upper() const", asFUNCTION(String::ToUpper), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string reverse() const", asFUNCTION(String::Reverse), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "uptr@ get_pointer() const", asFUNCTION(String::ToPtr), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "uptr@ opImplCast()", asFUNCTION(String::ToPtr), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "uptr@ opImplCast() const", asFUNCTION(String::ToPtr), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "uptr@ opImplConv() const", asFUNCTION(String::ToPtr), asCALL_CDECL_OBJFIRST);
				Engine->RegisterGlobalFunction("int64 to_int(const string &in, usize = 10, usize &out = 0)", asFUNCTION(String::IntStore), asCALL_CDECL);
				Engine->RegisterGlobalFunction("uint64 to_uint(const string &in, usize = 10, usize &out = 0)", asFUNCTION(String::UIntStore), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double to_float(const string &in, usize &out = 0)", asFUNCTION(String::FloatStore), asCALL_CDECL);
				Engine->RegisterGlobalFunction("uint8 to_char(const string &in)", asFUNCTION(String::ToChar), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(const array<string> &in, const string &in)", asFUNCTION(String::Join), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(int8)", asFUNCTION(String::ToInt8), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(int16)", asFUNCTION(String::ToInt16), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(int)", asFUNCTION(String::ToInt), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(int64)", asFUNCTION(String::ToInt64), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(uint8)", asFUNCTION(String::ToUInt8), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(uint16)", asFUNCTION(String::ToUInt16), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(uint)", asFUNCTION(String::ToUInt), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(uint64)", asFUNCTION(String::ToUInt64), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(float)", asFUNCTION(String::ToFloat), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(double)", asFUNCTION(String::ToDouble), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string to_string(uptr@)", asFUNCTION(String::ToPointer), asCALL_CDECL);

				return true;
			}
			bool Registry::LoadException(VMManager* Manager)
			{
				ED_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
				VMCManager* Engine = Manager->GetEngine();
				Manager->BeginNamespace("exception");
				Engine->RegisterGlobalFunction("void throw(const string &in = \"\")", asFUNCTION(Exception::Throw), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string unwrap()", asFUNCTION(Exception::GetException), asCALL_CDECL);
				Manager->EndNamespace();
				return true;
			}
			bool Registry::LoadMutex(VMManager* Manager)
			{
				ED_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
				VMCManager* Engine = Manager->GetEngine();
				Engine->RegisterObjectType("mutex", sizeof(Mutex), asOBJ_REF);
				Engine->RegisterObjectBehaviour("mutex", asBEHAVE_FACTORY, "mutex@ f()", asFUNCTION(Mutex::Factory), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("mutex", asBEHAVE_ADDREF, "void f()", asMETHOD(Mutex, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("mutex", asBEHAVE_RELEASE, "void f()", asMETHOD(Mutex, Release), asCALL_THISCALL);
				Engine->RegisterObjectMethod("mutex", "bool try_lock()", asMETHOD(Mutex, TryLock), asCALL_THISCALL);
				Engine->RegisterObjectMethod("mutex", "void lock()", asMETHOD(Mutex, Lock), asCALL_THISCALL);
				Engine->RegisterObjectMethod("mutex", "void unlock()", asMETHOD(Mutex, Unlock), asCALL_THISCALL);
				return true;
			}
			bool Registry::LoadThread(VMManager* Manager)
			{
				ED_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
				VMCManager* Engine = Manager->GetEngine();
				Engine->RegisterObjectType("thread", 0, asOBJ_REF | asOBJ_GC);
				Engine->RegisterFuncdef("void thread_event(thread@+)");
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_FACTORY, "thread@ f(thread_event@)", asFUNCTION(Thread::Create), asCALL_GENERIC);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_ADDREF, "void f()", asMETHOD(Thread, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_RELEASE, "void f()", asMETHOD(Thread, Release), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(Thread, SetGCFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(Thread, GetGCFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(Thread, GetRefCount), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(Thread, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("thread", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(Thread, ReleaseReferences), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "bool is_active()", asMETHOD(Thread, IsActive), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "bool invoke()", asMETHOD(Thread, Start), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "void suspend()", asMETHOD(Thread, Suspend), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "void resume()", asMETHOD(Thread, Resume), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "void push(const ?&in)", asMETHOD(Thread, Push), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "bool pop(?&out)", asMETHODPR(Thread, Pop, (void*, int), bool), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "bool pop(?&out, uint64)", asMETHODPR(Thread, Pop, (void*, int, uint64_t), bool), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "int join(uint64)", asMETHODPR(Thread, Join, (uint64_t), int), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "int join()", asMETHODPR(Thread, Join, (), int), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "string get_id() const", asMETHODPR(Thread, GetId, () const, std::string), asCALL_THISCALL);

				Manager->BeginNamespace("this_thread");
				Engine->RegisterGlobalFunction("thread@+ get_routine()", asFUNCTION(Thread::GetThread), asCALL_CDECL);
				Engine->RegisterGlobalFunction("uint64 get_id()", asFUNCTION(Thread::GetThreadId), asCALL_CDECL);
				Engine->RegisterGlobalFunction("void sleep(uint64)", asFUNCTION(Thread::ThreadSleep), asCALL_CDECL);
				Engine->RegisterGlobalFunction("void suspend()", asFUNCTION(Thread::ThreadSuspend), asCALL_CDECL);
				Manager->EndNamespace();
				return true;
			}
			bool Registry::LoadRandom(VMManager* Manager)
			{
				ED_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
				VMGlobal& Global = Manager->Global();

				VMRefClass VRandom = Global.SetClassUnmanaged<Random>("random");
				VRandom.SetMethodStatic("string bytes(uint64)", &Random::Getb);
				VRandom.SetMethodStatic("double betweend(double, double)", &Random::Betweend);
				VRandom.SetMethodStatic("double magd()", &Random::Magd);
				VRandom.SetMethodStatic("double getd()", &Random::Getd);
				VRandom.SetMethodStatic("float betweenf(float, float)", &Random::Betweenf);
				VRandom.SetMethodStatic("float magf()", &Random::Magf);
				VRandom.SetMethodStatic("float getf()", &Random::Getf);
				VRandom.SetMethodStatic("uint64 betweeni(uint64, uint64)", &Random::Betweeni);

				return true;
			}
			bool Registry::LoadPromise(VMManager* Manager)
			{
				VMCManager* Engine = Manager->GetEngine();
				if (!Engine)
					return false;

				Engine->RegisterObjectType("promise<class T>", 0, asOBJ_REF | asOBJ_GC | asOBJ_TEMPLATE);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(Array::TemplateCallback), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(Promise, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(Promise, Release), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(Promise, SetGCFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(Promise, GetGCFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(Promise, GetRefCount), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(Promise, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("promise<T>", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(Promise, ReleaseReferences), asCALL_THISCALL);
				Engine->RegisterObjectMethod("promise<T>", "bool retrieve(?&out)", asMETHODPR(Promise, To, (void*, int), bool), asCALL_THISCALL);
				Engine->RegisterObjectMethod("promise<T>", "T& await()", asMETHOD(Promise, GetValue), asCALL_THISCALL);

				Engine->RegisterObjectType("ref_promise<class T>", 0, asOBJ_REF | asOBJ_GC | asOBJ_TEMPLATE);
				Engine->RegisterObjectBehaviour("ref_promise<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(Array::TemplateCallback), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("ref_promise<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(Promise, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("ref_promise<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(Promise, Release), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("ref_promise<T>", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(Promise, SetGCFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("ref_promise<T>", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(Promise, GetGCFlag), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("ref_promise<T>", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(Promise, GetRefCount), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("ref_promise<T>", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(Promise, EnumReferences), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("ref_promise<T>", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(Promise, ReleaseReferences), asCALL_THISCALL);
				Engine->RegisterObjectMethod("ref_promise<T>", "T@+ await()", asMETHOD(Promise, GetHandle), asCALL_THISCALL);

				return true;
			}
			bool Registry::LoadFormat(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();
				VMRefClass VFormat = Register.SetClassUnmanaged<Format>("format");
				VFormat.SetUnmanagedConstructor<Format>("format@ f()");
				VFormat.SetUnmanagedConstructorList<Format>("format@ f(int &in) {repeat ?}");
				VFormat.SetMethodStatic("string to_json(const ? &in)", &Format::JSON);

				return true;
			}
			bool Registry::LoadDecimal(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();
				VMTypeClass VDecimal = Register.SetStructUnmanaged<Core::Decimal>("decimal");
				VDecimal.SetConstructor<Core::Decimal>("void f()");
				VDecimal.SetConstructor<Core::Decimal, int32_t>("void f(int)");
				VDecimal.SetConstructor<Core::Decimal, int32_t>("void f(uint)");
				VDecimal.SetConstructor<Core::Decimal, int64_t>("void f(int64)");
				VDecimal.SetConstructor<Core::Decimal, uint64_t>("void f(uint64)");
				VDecimal.SetConstructor<Core::Decimal, float>("void f(float)");
				VDecimal.SetConstructor<Core::Decimal, double>("void f(double)");
				VDecimal.SetConstructor<Core::Decimal, const std::string&>("void f(const string &in)");
				VDecimal.SetConstructor<Core::Decimal, const Core::Decimal&>("void f(const decimal &in)");
				VDecimal.SetMethod("decimal& truncate(int)", &Core::Decimal::Truncate);
				VDecimal.SetMethod("decimal& round(int)", &Core::Decimal::Round);
				VDecimal.SetMethod("decimal& trim()", &Core::Decimal::Trim);
				VDecimal.SetMethod("decimal& unlead()", &Core::Decimal::Unlead);
				VDecimal.SetMethod("decimal& untrail()", &Core::Decimal::Untrail);
				VDecimal.SetMethod("bool is_nan() const", &Core::Decimal::IsNaN);
				VDecimal.SetMethod("double to_double() const", &Core::Decimal::ToDouble);
				VDecimal.SetMethod("int64 to_int64() const", &Core::Decimal::ToInt64);
				VDecimal.SetMethod("string to_string() const", &Core::Decimal::ToString);
				VDecimal.SetMethod("string exp() const", &Core::Decimal::Exp);
				VDecimal.SetMethod("string decimals() const", &Core::Decimal::Decimals);
				VDecimal.SetMethod("string ints() const", &Core::Decimal::Ints);
				VDecimal.SetMethod("string size() const", &Core::Decimal::Size);
				VDecimal.SetOperatorEx(VMOpFunc::Neg, (uint32_t)VMOp::Const, "decimal", "", &DecimalNegate);
				VDecimal.SetOperatorEx(VMOpFunc::MulAssign, (uint32_t)VMOp::Left, "decimal&", "const decimal &in", &DecimalMulEq);
				VDecimal.SetOperatorEx(VMOpFunc::DivAssign, (uint32_t)VMOp::Left, "decimal&", "const decimal &in", &DecimalDivEq);
				VDecimal.SetOperatorEx(VMOpFunc::AddAssign, (uint32_t)VMOp::Left, "decimal&", "const decimal &in", &DecimalAddEq);
				VDecimal.SetOperatorEx(VMOpFunc::SubAssign, (uint32_t)VMOp::Left, "decimal&", "const decimal &in", &DecimalSubEq);
				VDecimal.SetOperatorEx(VMOpFunc::PostInc, (uint32_t)VMOp::Left, "decimal&", "", &DecimalPP);
				VDecimal.SetOperatorEx(VMOpFunc::PostDec, (uint32_t)VMOp::Left, "decimal&", "", &DecimalMM);
				VDecimal.SetOperatorEx(VMOpFunc::Equals, (uint32_t)VMOp::Const, "bool", "const decimal &in", &DecimalEq);
				VDecimal.SetOperatorEx(VMOpFunc::Cmp, (uint32_t)VMOp::Const, "int", "const decimal &in", &DecimalCmp);
				VDecimal.SetOperatorEx(VMOpFunc::Add, (uint32_t)VMOp::Const, "decimal", "const decimal &in", &DecimalAdd);
				VDecimal.SetOperatorEx(VMOpFunc::Sub, (uint32_t)VMOp::Const, "decimal", "const decimal &in", &DecimalSub);
				VDecimal.SetOperatorEx(VMOpFunc::Mul, (uint32_t)VMOp::Const, "decimal", "const decimal &in", &DecimalMul);
				VDecimal.SetOperatorEx(VMOpFunc::Div, (uint32_t)VMOp::Const, "decimal", "const decimal &in", &DecimalDiv);
				VDecimal.SetOperatorEx(VMOpFunc::Mod, (uint32_t)VMOp::Const, "decimal", "const decimal &in", &DecimalPer);
				VDecimal.SetMethodStatic("decimal nan()", &Core::Decimal::NaN);

				return true;
			}
			bool Registry::LoadVariant(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();
				VMEnum VVarType = Register.SetEnum("var_type");
				VVarType.SetValue("null_t", (int)Core::VarType::Null);
				VVarType.SetValue("undefined_t", (int)Core::VarType::Undefined);
				VVarType.SetValue("object_t", (int)Core::VarType::Object);
				VVarType.SetValue("array_t", (int)Core::VarType::Array);
				VVarType.SetValue("string_t", (int)Core::VarType::String);
				VVarType.SetValue("binary_t", (int)Core::VarType::Binary);
				VVarType.SetValue("integer_t", (int)Core::VarType::Integer);
				VVarType.SetValue("number_t", (int)Core::VarType::Number);
				VVarType.SetValue("decimal_t", (int)Core::VarType::Decimal);
				VVarType.SetValue("boolean_t", (int)Core::VarType::Boolean);

				VMTypeClass VVariant = Register.SetStructUnmanaged<Core::Variant>("variant");
				VVariant.SetConstructor<Core::Variant>("void f()");
				VVariant.SetConstructor<Core::Variant, const Core::Variant&>("void f(const variant &in)");
				VVariant.SetMethod("bool deserialize(const string &in, bool = false)", &Core::Variant::Deserialize);
				VVariant.SetMethod("string serialize() const", &Core::Variant::Serialize);
				VVariant.SetMethod("decimal to_decimal() const", &Core::Variant::GetDecimal);
				VVariant.SetMethod("string to_string() const", &Core::Variant::GetBlob);
				VVariant.SetMethod("uptr@ to_pointer() const", &Core::Variant::GetPointer);
				VVariant.SetMethod("int64 to_integer() const", &Core::Variant::GetInteger);
				VVariant.SetMethod("double to_number() const", &Core::Variant::GetNumber);
				VVariant.SetMethod("bool to_boolean() const", &Core::Variant::GetBoolean);
				VVariant.SetMethod("var_type get_type() const", &Core::Variant::GetType);
				VVariant.SetMethod("bool is_object() const", &Core::Variant::IsObject);
				VVariant.SetMethod("bool empty() const", &Core::Variant::IsEmpty);
				VVariant.SetMethodEx("usize size() const", &VariantGetSize);
				VVariant.SetOperatorEx(VMOpFunc::Equals, (uint32_t)VMOp::Const, "bool", "const variant &in", &VariantEquals);
				VVariant.SetOperatorEx(VMOpFunc::ImplCast, (uint32_t)VMOp::Const, "bool", "", &VariantImplCast);

				Engine->BeginNamespace("var");
				Register.SetFunction("variant auto_t(const string &in, bool = false)", &Core::Var::Auto);
				Register.SetFunction("variant null_t()", &Core::Var::Null);
				Register.SetFunction("variant undefined_t()", &Core::Var::Undefined);
				Register.SetFunction("variant object_t()", &Core::Var::Object);
				Register.SetFunction("variant array_t()", &Core::Var::Array);
				Register.SetFunction("variant pointer_t(uptr@)", &Core::Var::Pointer);
				Register.SetFunction("variant integer_t(int64)", &Core::Var::Integer);
				Register.SetFunction("variant number_t(double)", &Core::Var::Number);
				Register.SetFunction("variant boolean_t(bool)", &Core::Var::Boolean);
				Register.SetFunction<Core::Variant(const std::string&)>("variant string_t(const string &in)", &Core::Var::String);
				Register.SetFunction<Core::Variant(const std::string&)>("variant binary_t(const string &in)", &Core::Var::Binary);
				Register.SetFunction<Core::Variant(const std::string&)>("variant decimal_t(const string &in)", &Core::Var::DecimalString);
				Register.SetFunction<Core::Variant(const Core::Decimal&)>("variant decimal_t(const decimal &in)", &Core::Var::Decimal);
				Engine->EndNamespace();

				return true;
			}
			bool Registry::LoadTimestamp(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();
				VMTypeClass VDateTime = Register.SetStructUnmanaged<Core::DateTime>("timestamp");
				VDateTime.SetConstructor<Core::DateTime>("void f()");
				VDateTime.SetConstructor<Core::DateTime, uint64_t>("void f(uint64)");
				VDateTime.SetConstructor<Core::DateTime, const Core::DateTime&>("void f(const timestamp &in)");
				VDateTime.SetMethod("string format(const string &in)", &Core::DateTime::Format);
				VDateTime.SetMethod("string date(const string &in)", &Core::DateTime::Date);
				VDateTime.SetMethod("string to_iso8601()", &Core::DateTime::Iso8601);
				VDateTime.SetMethod("timestamp now()", &Core::DateTime::Now);
				VDateTime.SetMethod("timestamp from_nanoseconds(uint64)", &Core::DateTime::FromNanoseconds);
				VDateTime.SetMethod("timestamp from_microseconds(uint64)", &Core::DateTime::FromMicroseconds);
				VDateTime.SetMethod("timestamp from_milliseconds(uint64)", &Core::DateTime::FromMilliseconds);
				VDateTime.SetMethod("timestamp from_seconds(uint64)", &Core::DateTime::FromSeconds);
				VDateTime.SetMethod("timestamp from_minutes(uint64)", &Core::DateTime::FromMinutes);
				VDateTime.SetMethod("timestamp from_hours(uint64)", &Core::DateTime::FromHours);
				VDateTime.SetMethod("timestamp from_days(uint64)", &Core::DateTime::FromDays);
				VDateTime.SetMethod("timestamp from_weeks(uint64)", &Core::DateTime::FromWeeks);
				VDateTime.SetMethod("timestamp from_months(uint64)", &Core::DateTime::FromMonths);
				VDateTime.SetMethod("timestamp from_years(uint64)", &Core::DateTime::FromYears);
				VDateTime.SetMethod("timestamp& set_date_seconds(uint64, bool = true)", &Core::DateTime::SetDateSeconds);
				VDateTime.SetMethod("timestamp& set_date_minutes(uint64, bool = true)", &Core::DateTime::SetDateMinutes);
				VDateTime.SetMethod("timestamp& set_date_hours(uint64, bool = true)", &Core::DateTime::SetDateHours);
				VDateTime.SetMethod("timestamp& set_date_day(uint64, bool = true)", &Core::DateTime::SetDateDay);
				VDateTime.SetMethod("timestamp& set_date_week(uint64, bool = true)", &Core::DateTime::SetDateWeek);
				VDateTime.SetMethod("timestamp& set_date_month(uint64, bool = true)", &Core::DateTime::SetDateMonth);
				VDateTime.SetMethod("timestamp& set_date_year(uint64, bool = true)", &Core::DateTime::SetDateYear);
				VDateTime.SetMethod("uint64 date_second()", &Core::DateTime::DateSecond);
				VDateTime.SetMethod("uint64 date_minute()", &Core::DateTime::DateMinute);
				VDateTime.SetMethod("uint64 date_hour()", &Core::DateTime::DateHour);
				VDateTime.SetMethod("uint64 date_day()", &Core::DateTime::DateDay);
				VDateTime.SetMethod("uint64 date_week()", &Core::DateTime::DateWeek);
				VDateTime.SetMethod("uint64 date_month()", &Core::DateTime::DateMonth);
				VDateTime.SetMethod("uint64 date_year()", &Core::DateTime::DateYear);
				VDateTime.SetMethod("uint64 nanoseconds()", &Core::DateTime::Nanoseconds);
				VDateTime.SetMethod("uint64 microseconds()", &Core::DateTime::Microseconds);
				VDateTime.SetMethod("uint64 milliseconds()", &Core::DateTime::Milliseconds);
				VDateTime.SetMethod("uint64 seconds()", &Core::DateTime::Seconds);
				VDateTime.SetMethod("uint64 minutes()", &Core::DateTime::Minutes);
				VDateTime.SetMethod("uint64 hours()", &Core::DateTime::Hours);
				VDateTime.SetMethod("uint64 days()", &Core::DateTime::Days);
				VDateTime.SetMethod("uint64 weeks()", &Core::DateTime::Weeks);
				VDateTime.SetMethod("uint64 months()", &Core::DateTime::Months);
				VDateTime.SetMethod("uint64 years()", &Core::DateTime::Years);
				VDateTime.SetOperatorEx(VMOpFunc::AddAssign, (uint32_t)VMOp::Left, "timestamp&", "const timestamp &in", &DateTimeAddEq);
				VDateTime.SetOperatorEx(VMOpFunc::SubAssign, (uint32_t)VMOp::Left, "timestamp&", "const timestamp &in", &DateTimeSubEq);
				VDateTime.SetOperatorEx(VMOpFunc::Equals, (uint32_t)VMOp::Const, "bool", "const timestamp &in", &DateTimeEq);
				VDateTime.SetOperatorEx(VMOpFunc::Cmp, (uint32_t)VMOp::Const, "int", "const timestamp &in", &DateTimeCmp);
				VDateTime.SetOperatorEx(VMOpFunc::Add, (uint32_t)VMOp::Const, "timestamp", "const timestamp &in", &DateTimeAdd);
				VDateTime.SetOperatorEx(VMOpFunc::Sub, (uint32_t)VMOp::Const, "timestamp", "const timestamp &in", &DateTimeSub);
				VDateTime.SetMethodStatic<std::string, int64_t>("string get_gmt(int64)", &Core::DateTime::FetchWebDateGMT);
				VDateTime.SetMethodStatic<std::string, int64_t>("string get_time(int64)", &Core::DateTime::FetchWebDateTime);

				return true;
			}
			bool Registry::LoadConsole(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();
				VMEnum VStdColor = Register.SetEnum("std_color");
				VStdColor.SetValue("black", (int)Core::StdColor::Black);
				VStdColor.SetValue("dark_blue", (int)Core::StdColor::DarkBlue);
				VStdColor.SetValue("dark_green", (int)Core::StdColor::DarkGreen);
				VStdColor.SetValue("light_blue", (int)Core::StdColor::LightBlue);
				VStdColor.SetValue("dark_red", (int)Core::StdColor::DarkRed);
				VStdColor.SetValue("magenta", (int)Core::StdColor::Magenta);
				VStdColor.SetValue("orange", (int)Core::StdColor::Orange);
				VStdColor.SetValue("light_gray", (int)Core::StdColor::LightGray);
				VStdColor.SetValue("gray", (int)Core::StdColor::Gray);
				VStdColor.SetValue("blue", (int)Core::StdColor::Blue);
				VStdColor.SetValue("green", (int)Core::StdColor::Green);
				VStdColor.SetValue("cyan", (int)Core::StdColor::Cyan);
				VStdColor.SetValue("red", (int)Core::StdColor::Red);
				VStdColor.SetValue("pink", (int)Core::StdColor::Pink);
				VStdColor.SetValue("yellow", (int)Core::StdColor::Yellow);
				VStdColor.SetValue("white", (int)Core::StdColor::White);

				VMRefClass VConsole = Register.SetClassUnmanaged<Core::Console>("console");
				VConsole.SetMethod("void begin()", &Core::Console::Begin);
				VConsole.SetMethod("void end()", &Core::Console::End);
				VConsole.SetMethod("void hide()", &Core::Console::Hide);
				VConsole.SetMethod("void show()", &Core::Console::Show);
				VConsole.SetMethod("void clear()", &Core::Console::Clear);
				VConsole.SetMethod("void flush()", &Core::Console::Flush);
				VConsole.SetMethod("void flush_write()", &Core::Console::FlushWrite);
				VConsole.SetMethod("void set_cursor(uint32, uint32)", &Core::Console::SetCursor);
				VConsole.SetMethod("void set_coloring(bool)", &Core::Console::SetColoring);
				VConsole.SetMethod("void color_begin(std_color, std_color)", &Core::Console::ColorBegin);
				VConsole.SetMethod("void color_end()", &Core::Console::ColorEnd);
				VConsole.SetMethod("void capture_time()", &Core::Console::CaptureTime);
				VConsole.SetMethod("void write_line(const string &in)", &Core::Console::sWriteLine);
				VConsole.SetMethod("void write_char(uint8)", &Core::Console::WriteChar);
				VConsole.SetMethod("void write(const string &in)", &Core::Console::sWrite);
				VConsole.SetMethod("double get_captured_time()", &Core::Console::GetCapturedTime);
				VConsole.SetMethod("string read(usize)", &Core::Console::Read);
				VConsole.SetMethodStatic("console@+ get()", &Core::Console::Get);
				VConsole.SetMethodEx("void trace(uint32 = 32)", &ConsoleTrace);
				VConsole.SetMethodEx("void get_size(uint32 &out, uint32 &out)", &ConsoleGetSize);
				VConsole.SetMethodEx("void write_line(const string &in, format@+)", &Format::WriteLine);
				VConsole.SetMethodEx("void write(const string &in, format@+)", &Format::Write);

				return true;
			}
			bool Registry::LoadSchema(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMRefClass VSchema = Engine->Global().SetClassUnmanaged<Core::Schema>("schema");
				VSchema.SetProperty<Core::Schema>("string key", &Core::Schema::Key);
				VSchema.SetProperty<Core::Schema>("variant value", &Core::Schema::Value);
				VSchema.SetUnmanagedConstructor<Core::Schema, const Core::Variant&>("schema@ f(const variant &in)");
				VSchema.SetUnmanagedConstructorListEx<Core::Schema>("schema@ f(int &in) { repeat { string, ? } }", &SchemaConstruct);
				VSchema.SetMethod<Core::Schema, Core::Variant, size_t>("variant getVar(usize) const", &Core::Schema::GetVar);
				VSchema.SetMethod<Core::Schema, Core::Variant, const std::string&>("variant get_var(const string &in) const", &Core::Schema::GetVar);
				VSchema.SetMethod("schema@+ get_parent() const", &Core::Schema::GetParent);
				VSchema.SetMethod("schema@+ get_attribute(const string &in) const", &Core::Schema::GetAttribute);
				VSchema.SetMethod<Core::Schema, Core::Schema*, size_t>("schema@+ get(usize) const", &Core::Schema::Get);
				VSchema.SetMethod<Core::Schema, Core::Schema*, const std::string&, bool>("schema@+ get(const string &in, bool = false) const", &Core::Schema::Fetch);
				VSchema.SetMethod<Core::Schema, Core::Schema*, const std::string&>("schema@+ get(const string &in)", &Core::Schema::Set);
				VSchema.SetMethod<Core::Schema, Core::Schema*, const std::string&, const Core::Variant&>("schema@+ set(const string &in, const variant &in)", &Core::Schema::Set);
				VSchema.SetMethod<Core::Schema, Core::Schema*, const std::string&, const Core::Variant&>("schema@+ set_attribute(const string& in, const variant &in)", &Core::Schema::SetAttribute);
				VSchema.SetMethod<Core::Schema, Core::Schema*, const Core::Variant&>("schema@+ push(const variant &in)", &Core::Schema::Push);
				VSchema.SetMethod<Core::Schema, Core::Schema*, size_t>("schema@+ pop(usize)", &Core::Schema::Pop);
				VSchema.SetMethod<Core::Schema, Core::Schema*, const std::string&>("schema@+ pop(const string &in)", &Core::Schema::Pop);
				VSchema.SetMethod("schema@ copy() const", &Core::Schema::Copy);
				VSchema.SetMethod("bool has(const string &in) const", &Core::Schema::Has);
				VSchema.SetMethod("bool empty() const", &Core::Schema::IsEmpty);
				VSchema.SetMethod("bool is_attribute() const", &Core::Schema::IsAttribute);
				VSchema.SetMethod("bool is_saved() const", &Core::Schema::IsAttribute);
				VSchema.SetMethod("string get_name() const", &Core::Schema::GetName);
				VSchema.SetMethod("void join(schema@+)", &Core::Schema::Join);
				VSchema.SetMethod("void clear()", &Core::Schema::Clear);
				VSchema.SetMethod("void save()", &Core::Schema::Save);
				VSchema.SetMethodEx("variant first_var() const", &SchemaFirstVar);
				VSchema.SetMethodEx("variant last_var() const", &SchemaLastVar);
				VSchema.SetMethodEx("schema@+ first() const", &SchemaFirst);
				VSchema.SetMethodEx("schema@+ last() const", &SchemaLast);
				VSchema.SetMethodEx("schema@+ set(const string &in, schema@+)", &SchemaSet);
				VSchema.SetMethodEx("schema@+ push(schema@+)", &SchemaPush);
				VSchema.SetMethodEx("array<schema@>@ get_collection(const string &in, bool = false) const", &SchemaGetCollection);
				VSchema.SetMethodEx("array<schema@>@ get_attributes() const", &SchemaGetAttributes);
				VSchema.SetMethodEx("array<schema@>@ get_childs() const", &SchemaGetChilds);
				VSchema.SetMethodEx("map@ get_names() const", &SchemaGetNames);
				VSchema.SetMethodEx("usize size() const", &SchemaGetSize);
				VSchema.SetMethodEx("string to_json() const", &SchemaToJSON);
				VSchema.SetMethodEx("string to_xml() const", &SchemaToXML);
				VSchema.SetMethodEx("string to_string() const", &SchemaToString);
				VSchema.SetMethodEx("string to_binary() const", &SchemaToBinary);
				VSchema.SetMethodEx("int64 to_integer() const", &SchemaToInteger);
				VSchema.SetMethodEx("double to_number() const", &SchemaToNumber);
				VSchema.SetMethodEx("decimal to_decimal() const", &SchemaToDecimal);
				VSchema.SetMethodEx("bool to_boolean() const", &SchemaToBoolean);
				VSchema.SetMethodStatic("schema@ from_json(const string &in)", &SchemaFromJSON);
				VSchema.SetMethodStatic("schema@ from_xml(const string &in)", &SchemaFromXML);
				VSchema.SetMethodStatic("schema@ import_json(const string &in)", &SchemaImport);
				VSchema.SetOperatorEx(VMOpFunc::Assign, (uint32_t)VMOp::Left, "schema@+", "const variant &in", &SchemaCopyAssign);
				VSchema.SetOperatorEx(VMOpFunc::Equals, (uint32_t)(VMOp::Left | VMOp::Const), "bool", "schema@+", &SchemaEquals);
				VSchema.SetOperatorEx(VMOpFunc::Index, (uint32_t)VMOp::Left, "schema@+", "const string &in", &SchemaGetIndex);
				VSchema.SetOperatorEx(VMOpFunc::Index, (uint32_t)VMOp::Left, "schema@+", "usize", &SchemaGetIndexOffset);

				VMGlobal& Register = Engine->Global();
				Engine->BeginNamespace("var::set");
				Register.SetFunction("schema@ auto_t(const string &in, bool = false)", &Core::Var::Auto);
				Register.SetFunction("schema@ null_t()", &Core::Var::Set::Null);
				Register.SetFunction("schema@ undefined_t()", &Core::Var::Set::Undefined);
				Register.SetFunction("schema@ object_t()", &Core::Var::Set::Object);
				Register.SetFunction("schema@ array_t()", &Core::Var::Set::Array);
				Register.SetFunction("schema@ pointer_t(uptr@)", &Core::Var::Set::Pointer);
				Register.SetFunction("schema@ integer_t(int64)", &Core::Var::Set::Integer);
				Register.SetFunction("schema@ number_t(double)", &Core::Var::Set::Number);
				Register.SetFunction("schema@ boolean_t(bool)", &Core::Var::Set::Boolean);
				Register.SetFunction<Core::Schema* (const std::string&)>("schema@ string_t(const string &in)", &Core::Var::Set::String);
				Register.SetFunction<Core::Schema* (const std::string&)>("schema@ binary_t(const string &in)", &Core::Var::Set::Binary);
				Register.SetFunction<Core::Schema* (const std::string&)>("schema@ decimal_t(const string &in)", &Core::Var::Set::DecimalString);
				Register.SetFunction<Core::Schema* (const Core::Decimal&)>("schema@ decimal_t(const decimal &in)", &Core::Var::Set::Decimal);
				Register.SetFunction("schema@+ as(schema@+)", &SchemaInit);
				Engine->EndNamespace();

				return true;
			}
			bool Registry::LoadTickClock(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");

				VMRefClass VTimer = Engine->Global().SetClassUnmanaged<Core::Timer>("tick_clock");
				VTimer.SetUnmanagedConstructor<Core::Timer>("tick_clock@ f()");
				VTimer.SetMethod("void set_fixed_frames(float)", &Core::Timer::SetFixedFrames);
				VTimer.SetMethod("void begin()", &Core::Timer::Begin);
				VTimer.SetMethod("void finish()", &Core::Timer::Finish);
				VTimer.SetMethod("float get_max_frames() const", &Core::Timer::GetMaxFrames);
				VTimer.SetMethod("float get_min_step() const", &Core::Timer::GetMinStep);
				VTimer.SetMethod("float get_frames() const", &Core::Timer::GetFrames);
				VTimer.SetMethod("float get_elapsed() const", &Core::Timer::GetElapsed);
				VTimer.SetMethod("float get_elapsed_mills() const", &Core::Timer::GetElapsedMills);
				VTimer.SetMethod("float get_step() const", &Core::Timer::GetStep);
				VTimer.SetMethod("float get_fixed_step() const", &Core::Timer::GetFixedStep);
				VTimer.SetMethod("bool is_fixed() const", &Core::Timer::IsFixed);

				return true;
			}
			bool Registry::LoadFileSystem(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMEnum VFileMode = Engine->Global().SetEnum("file_mode");
				VFileMode.SetValue("read_only", (int)Core::FileMode::Read_Only);
				VFileMode.SetValue("write_only", (int)Core::FileMode::Write_Only);
				VFileMode.SetValue("append_only", (int)Core::FileMode::Append_Only);
				VFileMode.SetValue("read_write", (int)Core::FileMode::Read_Write);
				VFileMode.SetValue("write_read", (int)Core::FileMode::Write_Read);
				VFileMode.SetValue("read_append_write", (int)Core::FileMode::Read_Append_Write);
				VFileMode.SetValue("binary_read_only", (int)Core::FileMode::Binary_Read_Only);
				VFileMode.SetValue("binary_write_only", (int)Core::FileMode::Binary_Write_Only);
				VFileMode.SetValue("binary_append_only", (int)Core::FileMode::Binary_Append_Only);
				VFileMode.SetValue("binary_read_write", (int)Core::FileMode::Binary_Read_Write);
				VFileMode.SetValue("binary_write_read", (int)Core::FileMode::Binary_Write_Read);
				VFileMode.SetValue("binary_read_append_write", (int)Core::FileMode::Binary_Read_Append_Write);

				VMEnum VFileSeek = Engine->Global().SetEnum("file_seek");
				VFileSeek.SetValue("begin", (int)Core::FileSeek::Begin);
				VFileSeek.SetValue("current", (int)Core::FileSeek::Current);
				VFileSeek.SetValue("end", (int)Core::FileSeek::End);

				VMTypeClass VFileState = Engine->Global().SetPod<Core::FileState>("file_state");
				VFileState.SetProperty("usize size", &Core::FileState::Size);
				VFileState.SetProperty("usize links", &Core::FileState::Links);
				VFileState.SetProperty("usize permissions", &Core::FileState::Permissions);
				VFileState.SetProperty("usize document", &Core::FileState::Document);
				VFileState.SetProperty("usize device", &Core::FileState::Device);
				VFileState.SetProperty("usize userId", &Core::FileState::UserId);
				VFileState.SetProperty("usize groupId", &Core::FileState::GroupId);
				VFileState.SetProperty("int64 last_access", &Core::FileState::LastAccess);
				VFileState.SetProperty("int64 last_permission_change", &Core::FileState::LastPermissionChange);
				VFileState.SetProperty("int64 last_modified", &Core::FileState::LastModified);
				VFileState.SetProperty("bool exists", &Core::FileState::Exists);
				VFileState.SetConstructor<Core::FileState>("void f()");

				VMTypeClass VFileEntry = Engine->Global().SetStructUnmanaged<Core::FileEntry>("file_entry");
				VFileEntry.SetConstructor<Core::FileEntry>("void f()");
				VFileEntry.SetProperty("string path", &Core::FileEntry::Path);
				VFileEntry.SetProperty("usize size", &Core::FileEntry::Size);
				VFileEntry.SetProperty("int64 last_modified", &Core::FileEntry::LastModified);
				VFileEntry.SetProperty("int64 creation_time", &Core::FileEntry::CreationTime);
				VFileEntry.SetProperty("bool is_referenced", &Core::FileEntry::IsReferenced);
				VFileEntry.SetProperty("bool is_directory", &Core::FileEntry::IsDirectory);
				VFileEntry.SetProperty("bool is_exists", &Core::FileEntry::IsExists);

				VMRefClass VStream = Engine->Global().SetClassUnmanaged<Core::Stream>("base_stream");
				VStream.SetMethod("void clear()", &Core::Stream::Clear);
				VStream.SetMethod("bool close()", &Core::Stream::Close);
				VStream.SetMethod("bool seek(file_seek, int64)", &Core::Stream::Seek);
				VStream.SetMethod("bool move(int64)", &Core::Stream::Move);
				VStream.SetMethod("int32 flush()", &Core::Stream::Flush);
				VStream.SetMethod("int32 get_fd()", &Core::Stream::GetFd);
				VStream.SetMethod("usize get_size()", &Core::Stream::GetSize);
				VStream.SetMethod("usize tell()", &Core::Stream::Tell);
				VStream.SetMethodEx("bool open(const string &in, file_mode)", &StreamOpen);
				VStream.SetMethodEx("usize write(const string &in)", &StreamWrite);
				VStream.SetMethodEx("string read(usize)", &StreamRead);

				VMRefClass VFileStream = Engine->Global().SetClassUnmanaged<Core::FileStream>("file_stream");
				VFileStream.SetUnmanagedConstructor<Core::FileStream>("file_stream@ f()");
				VFileStream.SetMethod("void clear()", &Core::FileStream::Clear);
				VFileStream.SetMethod("bool close()", &Core::FileStream::Close);
				VFileStream.SetMethod("bool seek(file_seek, int64)", &Core::FileStream::Seek);
				VFileStream.SetMethod("bool move(int64)", &Core::FileStream::Move);
				VFileStream.SetMethod("int32 flush()", &Core::FileStream::Flush);
				VFileStream.SetMethod("int32 get_fd()", &Core::FileStream::GetFd);
				VFileStream.SetMethod("usize get_size()", &Core::FileStream::GetSize);
				VFileStream.SetMethod("usize tell()", &Core::FileStream::Tell);
				VFileStream.SetMethodEx("bool open(const string &in, file_mode)", &FileStreamOpen);
				VFileStream.SetMethodEx("usize write(const string &in)", &FileStreamWrite);
				VFileStream.SetMethodEx("string read(usize)", &FileStreamRead);

				VMRefClass VGzStream = Engine->Global().SetClassUnmanaged<Core::GzStream>("gz_stream");
				VGzStream.SetUnmanagedConstructor<Core::GzStream>("gz_stream@ f()");
				VGzStream.SetMethod("void clear()", &Core::GzStream::Clear);
				VGzStream.SetMethod("bool close()", &Core::GzStream::Close);
				VGzStream.SetMethod("bool seek(file_seek, int64)", &Core::GzStream::Seek);
				VGzStream.SetMethod("bool move(int64)", &Core::GzStream::Move);
				VGzStream.SetMethod("int32 flush()", &Core::GzStream::Flush);
				VGzStream.SetMethod("int32 get_fd()", &Core::GzStream::GetFd);
				VGzStream.SetMethod("usize get_size()", &Core::GzStream::GetSize);
				VGzStream.SetMethod("usize tell()", &Core::GzStream::Tell);
				VGzStream.SetMethodEx("bool open(const string &in, file_mode)", &GzStreamOpen);
				VGzStream.SetMethodEx("usize write(const string &in)", &GzStreamWrite);
				VGzStream.SetMethodEx("string read(usize)", &GzStreamRead);

				VMRefClass VWebStream = Engine->Global().SetClassUnmanaged<Core::WebStream>("web_stream");
				VWebStream.SetUnmanagedConstructor<Core::WebStream, bool>("web_stream@ f(bool)");
				VWebStream.SetMethod("void clear()", &Core::WebStream::Clear);
				VWebStream.SetMethod("bool close()", &Core::WebStream::Close);
				VWebStream.SetMethod("bool seek(file_seek, int64)", &Core::WebStream::Seek);
				VWebStream.SetMethod("bool move(int64)", &Core::WebStream::Move);
				VWebStream.SetMethod("int32 flush()", &Core::WebStream::Flush);
				VWebStream.SetMethod("int32 get_fd()", &Core::WebStream::GetFd);
				VWebStream.SetMethod("usize get_size()", &Core::WebStream::GetSize);
				VWebStream.SetMethod("usize tell()", &Core::WebStream::Tell);
				VWebStream.SetMethodEx("bool open(const string &in, file_mode)", &WebStreamOpen);
				VWebStream.SetMethodEx("usize write(const string &in)", &WebStreamWrite);
				VWebStream.SetMethodEx("string read(usize)", &WebStreamRead);

				VStream.SetOperatorEx(VMOpFunc::Cast, 0, "file_stream@+", "", &StreamToFileStream);
				VStream.SetOperatorEx(VMOpFunc::Cast, 0, "web_stream@+", "", &StreamToWebStream);
				VStream.SetOperatorEx(VMOpFunc::Cast, 0, "gz_stream@+", "", &StreamToGzStream);
				VStream.SetOperatorEx(VMOpFunc::Cast, (uint32_t)VMOp::Const, "file_stream@+", "", &StreamToFileStream);
				VStream.SetOperatorEx(VMOpFunc::Cast, (uint32_t)VMOp::Const, "web_stream@+", "", &StreamToWebStream);
				VStream.SetOperatorEx(VMOpFunc::Cast, (uint32_t)VMOp::Const, "gz_stream@+", "", &StreamToGzStream);
				VFileStream.SetOperatorEx(VMOpFunc::ImplCast, 0, "base_stream@+", "", &FileStreamToStream);
				VFileStream.SetOperatorEx(VMOpFunc::ImplCast, (uint32_t)VMOp::Const, "base_stream@+", "", &FileStreamToStream);
				VGzStream.SetOperatorEx(VMOpFunc::ImplCast, 0, "base_stream@+", "", &GzStreamToStream);
				VGzStream.SetOperatorEx(VMOpFunc::ImplCast, (uint32_t)VMOp::Const, "base_stream@+", "", &GzStreamToStream);
				VWebStream.SetOperatorEx(VMOpFunc::ImplCast, 0, "base_stream@+", "", &WebStreamToStream);
				VWebStream.SetOperatorEx(VMOpFunc::ImplCast, (uint32_t)VMOp::Const, "base_stream@+", "", &WebStreamToStream);

				return true;
			}
			bool Registry::LoadOS(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();

				Engine->BeginNamespace("os::cpu");
				VMEnum VArch = Engine->Global().SetEnum("arch");
				VArch.SetValue("x64", (int)Core::OS::CPU::Arch::X64);
				VArch.SetValue("arm", (int)Core::OS::CPU::Arch::ARM);
				VArch.SetValue("itanium", (int)Core::OS::CPU::Arch::Itanium);
				VArch.SetValue("x86", (int)Core::OS::CPU::Arch::X86);
				VArch.SetValue("unknown", (int)Core::OS::CPU::Arch::Unknown);

				VMEnum VEndian = Engine->Global().SetEnum("endian");
				VEndian.SetValue("little", (int)Core::OS::CPU::Endian::Little);
				VEndian.SetValue("big", (int)Core::OS::CPU::Endian::Big);

				VMEnum VCache = Engine->Global().SetEnum("cache");
				VCache.SetValue("unified", (int)Core::OS::CPU::Cache::Unified);
				VCache.SetValue("instruction", (int)Core::OS::CPU::Cache::Instruction);
				VCache.SetValue("data", (int)Core::OS::CPU::Cache::Data);
				VCache.SetValue("trace", (int)Core::OS::CPU::Cache::Trace);

				VMTypeClass VQuantityInfo = Engine->Global().SetPod<Core::OS::CPU::QuantityInfo>("quantity_info");
				VQuantityInfo.SetProperty("uint32 logical", &Core::OS::CPU::QuantityInfo::Logical);
				VQuantityInfo.SetProperty("uint32 physical", &Core::OS::CPU::QuantityInfo::Physical);
				VQuantityInfo.SetProperty("uint32 packages", &Core::OS::CPU::QuantityInfo::Packages);
				VQuantityInfo.SetConstructor<Core::OS::CPU::QuantityInfo>("void f()");

				VMTypeClass VCacheInfo = Engine->Global().SetPod<Core::OS::CPU::CacheInfo>("cache_info");
				VCacheInfo.SetProperty("usize size", &Core::OS::CPU::CacheInfo::Size);
				VCacheInfo.SetProperty("usize line_size", &Core::OS::CPU::CacheInfo::LineSize);
				VCacheInfo.SetProperty("uint8 associativity", &Core::OS::CPU::CacheInfo::Associativity);
				VCacheInfo.SetProperty("cache type", &Core::OS::CPU::CacheInfo::Type);
				VCacheInfo.SetConstructor<Core::OS::CPU::CacheInfo>("void f()");

				Register.SetFunction("quantity_info get_quantity_info()", &Core::OS::CPU::GetQuantityInfo);
				Register.SetFunction("cache_info get_cache_info(uint32)", &Core::OS::CPU::GetCacheInfo);
				Register.SetFunction("arch get_arch()", &Core::OS::CPU::GetArch);
				Register.SetFunction("endian get_endianness()", &Core::OS::CPU::GetEndianness);
				Register.SetFunction("usize get_frequency()", &Core::OS::CPU::GetFrequency);
				Engine->EndNamespace();

				Engine->BeginNamespace("os::directory");
				Register.SetFunction("array<file_entry>@ scan(const string &in)", &OSDirectoryScan);
				Register.SetFunction("array<string>@ get_mounts(const string &in)", &OSDirectoryGetMounts);
				Register.SetFunction("bool create(const string &in)", &OSDirectoryCreate);
				Register.SetFunction("bool remove(const string &in)", &OSDirectoryRemove);
				Register.SetFunction("bool is_exists(const string &in)", &OSDirectoryIsExists);
				Register.SetFunction("string get()", &Core::OS::Directory::Get);
				Register.SetFunction("void set(const string &in)", &OSDirectorySet);
				Register.SetFunction("void patch(const string &in)", &Core::OS::Directory::Patch);
				Engine->EndNamespace();

				Engine->BeginNamespace("os::file");
				Register.SetFunction<bool(const std::string&, const std::string&)>("bool write(const string &in, const string &in)", &Core::OS::File::Write);
				Register.SetFunction("bool state(const string &in, file_entry &out)", &OSFileState);
				Register.SetFunction("bool move(const string &in, const string &in)", &OSFileMove);
				Register.SetFunction("bool remove(const string &in)", &OSFileRemove);
				Register.SetFunction("bool is_exists(const string &in)", &OSFileIsExists);
				Register.SetFunction("file_state get_properties(const string &in)", &OSFileGetProperties);
				Register.SetFunction("string read_as_string(const string &in)", &Core::OS::File::ReadAsString);
				Register.SetFunction("array<string>@ read_as_array(const string &in)", &OSFileReadAsArray);
				Register.SetFunction("int32 compare(const string &in, const string &in)", &Core::OS::File::Compare);
				Register.SetFunction("int64 get_check_sum(const string &in)", &Core::OS::File::GetCheckSum);
				Register.SetFunction<Core::Stream*(const std::string&, Core::FileMode, bool)>("base_stream@ open(const string &in, file_mode, bool = false)", &Core::OS::File::Open);
				Engine->EndNamespace();

				Engine->BeginNamespace("os::path");
				Register.SetFunction("bool is_remote(const string &in)", &OSPathIsRemote);
				Register.SetFunction("string resolve(const string &in)", &OSPathResolve);
				Register.SetFunction("string resolve_directory(const string &in)", &OSPathResolveDirectory);
				Register.SetFunction("string get_directory(const string &in, usize = 0)", &OSPathGetDirectory);
				Register.SetFunction("string get_filename(const string &in)", &OSPathGetFilename);
				Register.SetFunction("string get_extension(const string &in)", &OSPathGetExtension);
				Register.SetFunction<std::string(const std::string&, const std::string&)>("string resolve(const string &in, const string &in)", &Core::OS::Path::Resolve);
				Register.SetFunction<std::string(const std::string&, const std::string&)>("string resolve_directory(const string &in, const string &in)", &Core::OS::Path::ResolveDirectory);
				Register.SetFunction<std::string(const std::string&)>("string resolve_resource(const string &in)", &Core::OS::Path::ResolveResource);
				Register.SetFunction<std::string(const std::string&, const std::string&)>("string resolve_resource(const string &in, const string &in)", &Core::OS::Path::ResolveResource);
				Engine->EndNamespace();

				Engine->BeginNamespace("os::process");
				Register.SetFunction("void interrupt()", &Core::OS::Process::Interrupt);
				Register.SetFunction("int execute(const string &in)", &OSProcessExecute);
				Register.SetFunction("int await(uptr@)", &OSProcessAwait);
				Register.SetFunction("bool free(uptr@)", &OSProcessFree);
				Register.SetFunction("uptr@ spawn(const string &in, array<string>@+)", &OSProcessSpawn);
				Engine->EndNamespace();

				Engine->BeginNamespace("os::symbol");
				Register.SetFunction("uptr@ load(const string &in)", &Core::OS::Symbol::Load);
				Register.SetFunction("uptr@ load_function(uptr@, const string &in)", &Core::OS::Symbol::LoadFunction);
				Register.SetFunction("bool unload(uptr@)", &Core::OS::Symbol::Unload);
				Engine->EndNamespace();

				Engine->BeginNamespace("os::input");
				Register.SetFunction("bool text(const string &in, const string &in, const string &in, string &out)", &Core::OS::Input::Text);
				Register.SetFunction("bool password(const string &in, const string &in, string &out)", &Core::OS::Input::Password);
				Register.SetFunction("bool save(const string &in, const string &in, const string &in, const string &in, string &out)", &Core::OS::Input::Save);
				Register.SetFunction("bool open(const string &in, const string &in, const string &in, const string &in, bool, string &out)", &Core::OS::Input::Open);
				Register.SetFunction("bool folder(const string &in, const string &in, string &out)", &Core::OS::Input::Folder);
				Register.SetFunction("bool color(const string &in, const string &in, string &out)", &Core::OS::Input::Color);
				Engine->EndNamespace();

				Engine->BeginNamespace("os::error");
				Register.SetFunction("int32 get()", &Core::OS::Error::Get);
				Register.SetFunction("string get_name(int32)", &Core::OS::Error::GetName);
				Register.SetFunction("bool is_error(int32)", &Core::OS::Error::IsError);
				Engine->EndNamespace();

				return true;
			}
			bool Registry::LoadSchedule(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				Engine->GetEngine()->RegisterTypedef("task_id", "uint64");

				VMEnum VDifficulty = Engine->Global().SetEnum("difficulty");
				VDifficulty.SetValue("coroutine", (int)Core::Difficulty::Coroutine);
				VDifficulty.SetValue("light", (int)Core::Difficulty::Light);
				VDifficulty.SetValue("heavy", (int)Core::Difficulty::Heavy);
				VDifficulty.SetValue("clock", (int)Core::Difficulty::Clock);

				VMTypeClass VDesc = Engine->Global().SetStructUnmanaged<Core::Schedule::Desc>("schedule_policy");
				VDesc.SetProperty("usize memory", &Core::Schedule::Desc::Memory);
				VDesc.SetProperty("usize coroutines", &Core::Schedule::Desc::Coroutines);
				VDesc.SetProperty("bool parallel", &Core::Schedule::Desc::Parallel);
				VDesc.SetConstructor<Core::Schedule::Desc>("void f()");
				VDesc.SetMethod("void set_threads(usize)", &Core::Schedule::Desc::SetThreads);

				VMRefClass VSchedule = Engine->Global().SetClassUnmanaged<Core::Schedule>("schedule");
				VSchedule.SetFunctionDef("void task_event()");
				VSchedule.SetMethodEx("task_id set_interval(uint64, task_event@+, difficulty = difficulty::light)", &ScheduleSetInterval);
				VSchedule.SetMethodEx("task_id set_timeout(uint64, task_event@+, difficulty = difficulty::light)", &ScheduleSetTimeout);
				VSchedule.SetMethodEx("bool set_immediate(task_event@+, difficulty = difficulty::heavy)", &ScheduleSetImmediate);
				VSchedule.SetMethod("bool clear_timeout(task_id)", &Core::Schedule::ClearTimeout);
				VSchedule.SetMethod("bool dispatch()", &Core::Schedule::Dispatch);
				VSchedule.SetMethod("bool start(const schedule_policy &in)", &Core::Schedule::Start);
				VSchedule.SetMethod("bool stop()", &Core::Schedule::Stop);
				VSchedule.SetMethod("bool wakeup()", &Core::Schedule::Wakeup);
				VSchedule.SetMethod("bool is_active() const", &Core::Schedule::IsActive);
				VSchedule.SetMethod("bool can_enqueue() const", &Core::Schedule::CanEnqueue);
				VSchedule.SetMethod("bool has_tasks(difficulty) const", &Core::Schedule::HasTasks);
				VSchedule.SetMethod("bool has_any_tasks() const", &Core::Schedule::HasAnyTasks);
				VSchedule.SetMethod("usize get_total_threads() const", &Core::Schedule::GetTotalThreads);
				VSchedule.SetMethod("usize get_threads(difficulty) const", &Core::Schedule::GetThreads);
				VSchedule.SetMethod("const schedule_policy& get_policy() const", &Core::Schedule::GetPolicy);
				VSchedule.SetMethodStatic("schedule@+ get()", &Core::Schedule::Get);

				return true;
			}
			bool Registry::LoadVertices(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();
				VMTypeClass VVertex = Register.SetPod<Compute::Vertex>("vertex");
				VVertex.SetProperty<Compute::Vertex>("float position_x", &Compute::Vertex::PositionX);
				VVertex.SetProperty<Compute::Vertex>("float position_y", &Compute::Vertex::PositionY);
				VVertex.SetProperty<Compute::Vertex>("float position_z", &Compute::Vertex::PositionZ);
				VVertex.SetProperty<Compute::Vertex>("float texcoord_x", &Compute::Vertex::TexCoordX);
				VVertex.SetProperty<Compute::Vertex>("float texcoord_y", &Compute::Vertex::TexCoordY);
				VVertex.SetProperty<Compute::Vertex>("float normal_x", &Compute::Vertex::NormalX);
				VVertex.SetProperty<Compute::Vertex>("float normal_y", &Compute::Vertex::NormalY);
				VVertex.SetProperty<Compute::Vertex>("float normal_z", &Compute::Vertex::NormalZ);
				VVertex.SetProperty<Compute::Vertex>("float tangent_x", &Compute::Vertex::TangentX);
				VVertex.SetProperty<Compute::Vertex>("float tangent_y", &Compute::Vertex::TangentY);
				VVertex.SetProperty<Compute::Vertex>("float tangent_z", &Compute::Vertex::TangentZ);
				VVertex.SetProperty<Compute::Vertex>("float bitangent_x", &Compute::Vertex::BitangentX);
				VVertex.SetProperty<Compute::Vertex>("float bitangent_y", &Compute::Vertex::BitangentY);
				VVertex.SetProperty<Compute::Vertex>("float bitangent_z", &Compute::Vertex::BitangentZ);
				VVertex.SetConstructor<Compute::Vertex>("void f()");

				VMTypeClass VSkinVertex = Register.SetPod<Compute::SkinVertex>("skin_vertex");
				VSkinVertex.SetProperty<Compute::SkinVertex>("float position_x", &Compute::SkinVertex::PositionX);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float position_y", &Compute::SkinVertex::PositionY);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float position_z", &Compute::SkinVertex::PositionZ);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float texcoord_x", &Compute::SkinVertex::TexCoordX);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float texcoord_y", &Compute::SkinVertex::TexCoordY);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float normal_x", &Compute::SkinVertex::NormalX);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float normal_y", &Compute::SkinVertex::NormalY);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float normal_z", &Compute::SkinVertex::NormalZ);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float tangent_x", &Compute::SkinVertex::TangentX);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float tangent_y", &Compute::SkinVertex::TangentY);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float tangent_z", &Compute::SkinVertex::TangentZ);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float bitangent_x", &Compute::SkinVertex::BitangentX);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float bitangent_y", &Compute::SkinVertex::BitangentY);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float bitangent_z", &Compute::SkinVertex::BitangentZ);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float joint_index0", &Compute::SkinVertex::JointIndex0);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float joint_index1", &Compute::SkinVertex::JointIndex1);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float joint_index2", &Compute::SkinVertex::JointIndex2);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float joint_index3", &Compute::SkinVertex::JointIndex3);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float joint_bias0", &Compute::SkinVertex::JointBias0);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float joint_bias1", &Compute::SkinVertex::JointBias1);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float joint_bias2", &Compute::SkinVertex::JointBias2);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float joint_bias3", &Compute::SkinVertex::JointBias3);
				VSkinVertex.SetConstructor<Compute::SkinVertex>("void f()");

				VMTypeClass VShapeVertex = Register.SetPod<Compute::ShapeVertex>("shape_vertex");
				VShapeVertex.SetProperty<Compute::ShapeVertex>("float position_x", &Compute::ShapeVertex::PositionX);
				VShapeVertex.SetProperty<Compute::ShapeVertex>("float position_y", &Compute::ShapeVertex::PositionY);
				VShapeVertex.SetProperty<Compute::ShapeVertex>("float position_z", &Compute::ShapeVertex::PositionZ);
				VShapeVertex.SetProperty<Compute::ShapeVertex>("float texcoord_x", &Compute::ShapeVertex::TexCoordX);
				VShapeVertex.SetProperty<Compute::ShapeVertex>("float texcoord_y", &Compute::ShapeVertex::TexCoordY);
				VShapeVertex.SetConstructor<Compute::ShapeVertex>("void f()");

				VMTypeClass VElementVertex = Register.SetPod<Compute::ElementVertex>("element_vertex");
				VElementVertex.SetProperty<Compute::ElementVertex>("float position_x", &Compute::ElementVertex::PositionX);
				VElementVertex.SetProperty<Compute::ElementVertex>("float position_y", &Compute::ElementVertex::PositionY);
				VElementVertex.SetProperty<Compute::ElementVertex>("float position_z", &Compute::ElementVertex::PositionZ);
				VElementVertex.SetProperty<Compute::ElementVertex>("float velocity_x", &Compute::ElementVertex::VelocityX);
				VElementVertex.SetProperty<Compute::ElementVertex>("float velocity_y", &Compute::ElementVertex::VelocityY);
				VElementVertex.SetProperty<Compute::ElementVertex>("float velocity_z", &Compute::ElementVertex::VelocityZ);
				VElementVertex.SetProperty<Compute::ElementVertex>("float color_x", &Compute::ElementVertex::ColorX);
				VElementVertex.SetProperty<Compute::ElementVertex>("float color_y", &Compute::ElementVertex::ColorY);
				VElementVertex.SetProperty<Compute::ElementVertex>("float color_z", &Compute::ElementVertex::ColorZ);
				VElementVertex.SetProperty<Compute::ElementVertex>("float color_w", &Compute::ElementVertex::ColorW);
				VElementVertex.SetProperty<Compute::ElementVertex>("float scale", &Compute::ElementVertex::Scale);
				VElementVertex.SetProperty<Compute::ElementVertex>("float rotation", &Compute::ElementVertex::Rotation);
				VElementVertex.SetProperty<Compute::ElementVertex>("float angular", &Compute::ElementVertex::Angular);
				VElementVertex.SetConstructor<Compute::ElementVertex>("void f()");

				return true;
			}
			bool Registry::LoadVectors(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();

				VMEnum VCubeFace = Register.SetEnum("cube_face");
				VCubeFace.SetValue("positive_x", (int)Compute::CubeFace::PositiveX);
				VCubeFace.SetValue("negative_x", (int)Compute::CubeFace::NegativeX);
				VCubeFace.SetValue("positive_y", (int)Compute::CubeFace::PositiveY);
				VCubeFace.SetValue("negative_y", (int)Compute::CubeFace::NegativeY);
				VCubeFace.SetValue("positive_z", (int)Compute::CubeFace::PositiveZ);
				VCubeFace.SetValue("negative_z", (int)Compute::CubeFace::NegativeZ);

				VMTypeClass VVector2 = Register.SetPod<Compute::Vector2>("vector2");
				VVector2.SetConstructor<Compute::Vector2>("void f()");
				VVector2.SetConstructor<Compute::Vector2, float, float>("void f(float, float)");
				VVector2.SetConstructor<Compute::Vector2, float>("void f(float)");
				VVector2.SetProperty<Compute::Vector2>("float x", &Compute::Vector2::X);
				VVector2.SetProperty<Compute::Vector2>("float y", &Compute::Vector2::Y);
				VVector2.SetMethod("float length() const", &Compute::Vector2::Length);
				VVector2.SetMethod("float sum() const", &Compute::Vector2::Sum);
				VVector2.SetMethod("float dot(const vector2 &in) const", &Compute::Vector2::Dot);
				VVector2.SetMethod("float distance(const vector2 &in) const", &Compute::Vector2::Distance);
				VVector2.SetMethod("float hypotenuse() const", &Compute::Vector2::Hypotenuse);
				VVector2.SetMethod("float look_at(const vector2 &in) const", &Compute::Vector2::LookAt);
				VVector2.SetMethod("float cross(const vector2 &in) const", &Compute::Vector2::Cross);
				VVector2.SetMethod("vector2 direction(float) const", &Compute::Vector2::Direction);
				VVector2.SetMethod("vector2 inv() const", &Compute::Vector2::Inv);
				VVector2.SetMethod("vector2 inv_x() const", &Compute::Vector2::InvX);
				VVector2.SetMethod("vector2 inv_y() const", &Compute::Vector2::InvY);
				VVector2.SetMethod("vector2 normalize() const", &Compute::Vector2::Normalize);
				VVector2.SetMethod("vector2 snormalize() const", &Compute::Vector2::sNormalize);
				VVector2.SetMethod("vector2 lerp(const vector2 &in, float) const", &Compute::Vector2::Lerp);
				VVector2.SetMethod("vector2 slerp(const vector2 &in, float) const", &Compute::Vector2::sLerp);
				VVector2.SetMethod("vector2 alerp(const vector2 &in, float) const", &Compute::Vector2::aLerp);
				VVector2.SetMethod("vector2 rlerp() const", &Compute::Vector2::rLerp);
				VVector2.SetMethod("vector2 abs() const", &Compute::Vector2::Abs);
				VVector2.SetMethod("vector2 radians() const", &Compute::Vector2::Radians);
				VVector2.SetMethod("vector2 degrees() const", &Compute::Vector2::Degrees);
				VVector2.SetMethod("vector2 xy() const", &Compute::Vector2::XY);
				VVector2.SetMethod<Compute::Vector2, Compute::Vector2, float>("vector2 mul(float) const", &Compute::Vector2::Mul);
				VVector2.SetMethod<Compute::Vector2, Compute::Vector2, float, float>("vector2 mul(float, float) const", &Compute::Vector2::Mul);
				VVector2.SetMethod<Compute::Vector2, Compute::Vector2, const Compute::Vector2&>("vector2 mul(const vector2 &in) const", &Compute::Vector2::Mul);
				VVector2.SetMethod("vector2 div(const vector2 &in) const", &Compute::Vector2::Div);
				VVector2.SetMethod("vector2 set_x(float) const", &Compute::Vector2::SetX);
				VVector2.SetMethod("vector2 set_y(float) const", &Compute::Vector2::SetY);
				VVector2.SetMethod("void set(const vector2 &in) const", &Compute::Vector2::Set);
				VVector2.SetOperatorEx(VMOpFunc::MulAssign, (uint32_t)VMOp::Left, "vector2&", "const vector2 &in", &Vector2MulEq1);
				VVector2.SetOperatorEx(VMOpFunc::MulAssign, (uint32_t)VMOp::Left, "vector2&", "float", &Vector2MulEq2);
				VVector2.SetOperatorEx(VMOpFunc::DivAssign, (uint32_t)VMOp::Left, "vector2&", "const vector2 &in", &Vector2DivEq1);
				VVector2.SetOperatorEx(VMOpFunc::DivAssign, (uint32_t)VMOp::Left, "vector2&", "float", &Vector2DivEq2);
				VVector2.SetOperatorEx(VMOpFunc::AddAssign, (uint32_t)VMOp::Left, "vector2&", "const vector2 &in", &Vector2AddEq1);
				VVector2.SetOperatorEx(VMOpFunc::AddAssign, (uint32_t)VMOp::Left, "vector2&", "float", &Vector2AddEq2);
				VVector2.SetOperatorEx(VMOpFunc::SubAssign, (uint32_t)VMOp::Left, "vector2&", "const vector2 &in", &Vector2SubEq1);
				VVector2.SetOperatorEx(VMOpFunc::SubAssign, (uint32_t)VMOp::Left, "vector2&", "float", &Vector2SubEq2);
				VVector2.SetOperatorEx(VMOpFunc::Mul, (uint32_t)VMOp::Const, "vector2", "const vector2 &in", &Vector2Mul1);
				VVector2.SetOperatorEx(VMOpFunc::Mul, (uint32_t)VMOp::Const, "vector2", "float", &Vector2Mul2);
				VVector2.SetOperatorEx(VMOpFunc::Div, (uint32_t)VMOp::Const, "vector2", "const vector2 &in", &Vector2Div1);
				VVector2.SetOperatorEx(VMOpFunc::Div, (uint32_t)VMOp::Const, "vector2", "float", &Vector2Div2);
				VVector2.SetOperatorEx(VMOpFunc::Add, (uint32_t)VMOp::Const, "vector2", "const vector2 &in", &Vector2Add1);
				VVector2.SetOperatorEx(VMOpFunc::Add, (uint32_t)VMOp::Const, "vector2", "float", &Vector2Add2);
				VVector2.SetOperatorEx(VMOpFunc::Sub, (uint32_t)VMOp::Const, "vector2", "const vector2 &in", &Vector2Sub1);
				VVector2.SetOperatorEx(VMOpFunc::Sub, (uint32_t)VMOp::Const, "vector2", "float", &Vector2Sub2);
				VVector2.SetOperatorEx(VMOpFunc::Neg, (uint32_t)VMOp::Const, "vector2", "", &Vector2Neg);
				VVector2.SetOperatorEx(VMOpFunc::Equals, (uint32_t)VMOp::Const, "bool", "const vector2 &in", &Vector2Eq);
				VVector2.SetOperatorEx(VMOpFunc::Cmp, (uint32_t)VMOp::Const, "int", "const vector2 &in", &Vector2Cmp);

				Engine->BeginNamespace("vector2");
				Register.SetFunction("vector2 random()", &Compute::Vector2::Random);
				Register.SetFunction("vector2 random_abs()", &Compute::Vector2::RandomAbs);
				Register.SetFunction("vector2 one()", &Compute::Vector2::One);
				Register.SetFunction("vector2 zero()", &Compute::Vector2::Zero);
				Register.SetFunction("vector2 up()", &Compute::Vector2::Up);
				Register.SetFunction("vector2 down()", &Compute::Vector2::Down);
				Register.SetFunction("vector2 left()", &Compute::Vector2::Left);
				Register.SetFunction("vector2 right()", &Compute::Vector2::Right);
				Engine->EndNamespace();

				VMTypeClass VVector3 = Register.SetPod<Compute::Vector3>("vector3");
				VVector3.SetConstructor<Compute::Vector3>("void f()");
				VVector3.SetConstructor<Compute::Vector3, float, float>("void f(float, float)");
				VVector3.SetConstructor<Compute::Vector3, float, float, float>("void f(float, float, float)");
				VVector3.SetConstructor<Compute::Vector3, float>("void f(float)");
				VVector3.SetConstructor<Compute::Vector3, const Compute::Vector2&>("void f(const vector2 &in)");
				VVector3.SetProperty<Compute::Vector3>("float x", &Compute::Vector3::X);
				VVector3.SetProperty<Compute::Vector3>("float y", &Compute::Vector3::Y);
				VVector3.SetProperty<Compute::Vector3>("float z", &Compute::Vector3::Z);
				VVector3.SetMethod("float length() const", &Compute::Vector3::Length);
				VVector3.SetMethod("float sum() const", &Compute::Vector3::Sum);
				VVector3.SetMethod("float dot(const vector3 &in) const", &Compute::Vector3::Dot);
				VVector3.SetMethod("float distance(const vector3 &in) const", &Compute::Vector3::Distance);
				VVector3.SetMethod("float hypotenuse() const", &Compute::Vector3::Hypotenuse);
				VVector3.SetMethod("vector3 look_at(const vector3 &in) const", &Compute::Vector3::LookAt);
				VVector3.SetMethod("vector3 cross(const vector3 &in) const", &Compute::Vector3::Cross);
				VVector3.SetMethod("vector3 direction_horizontal() const", &Compute::Vector3::hDirection);
				VVector3.SetMethod("vector3 direction_depth() const", &Compute::Vector3::dDirection);
				VVector3.SetMethod("vector3 inv() const", &Compute::Vector3::Inv);
				VVector3.SetMethod("vector3 inv_x() const", &Compute::Vector3::InvX);
				VVector3.SetMethod("vector3 inv_y() const", &Compute::Vector3::InvY);
				VVector3.SetMethod("vector3 inv_z() const", &Compute::Vector3::InvZ);
				VVector3.SetMethod("vector3 normalize() const", &Compute::Vector3::Normalize);
				VVector3.SetMethod("vector3 snormalize() const", &Compute::Vector3::sNormalize);
				VVector3.SetMethod("vector3 lerp(const vector3 &in, float) const", &Compute::Vector3::Lerp);
				VVector3.SetMethod("vector3 slerp(const vector3 &in, float) const", &Compute::Vector3::sLerp);
				VVector3.SetMethod("vector3 alerp(const vector3 &in, float) const", &Compute::Vector3::aLerp);
				VVector3.SetMethod("vector3 rlerp() const", &Compute::Vector3::rLerp);
				VVector3.SetMethod("vector3 abs() const", &Compute::Vector3::Abs);
				VVector3.SetMethod("vector3 radians() const", &Compute::Vector3::Radians);
				VVector3.SetMethod("vector3 degrees() const", &Compute::Vector3::Degrees);
				VVector3.SetMethod("vector2 xy() const", &Compute::Vector3::XY);
				VVector3.SetMethod("vector3 xyz() const", &Compute::Vector3::XYZ);
				VVector3.SetMethod<Compute::Vector3, Compute::Vector3, float>("vector3 mul(float) const", &Compute::Vector3::Mul);
				VVector3.SetMethod<Compute::Vector3, Compute::Vector3, const Compute::Vector2&, float>("vector3 mul(const vector2 &in, float) const", &Compute::Vector3::Mul);
				VVector3.SetMethod<Compute::Vector3, Compute::Vector3, const Compute::Vector3&>("vector3 mul(const vector3 &in) const", &Compute::Vector3::Mul);
				VVector3.SetMethod("vector3 div(const vector3 &in) const", &Compute::Vector3::Div);
				VVector3.SetMethod("vector3 set_x(float) const", &Compute::Vector3::SetX);
				VVector3.SetMethod("vector3 set_y(float) const", &Compute::Vector3::SetY);
				VVector3.SetMethod("vector3 set_z(float) const", &Compute::Vector3::SetZ);
				VVector3.SetMethod("void set(const vector3 &in) const", &Compute::Vector3::Set);
				VVector3.SetOperatorEx(VMOpFunc::MulAssign, (uint32_t)VMOp::Left, "vector3&", "const vector3 &in", &Vector3MulEq1);
				VVector3.SetOperatorEx(VMOpFunc::MulAssign, (uint32_t)VMOp::Left, "vector3&", "float", &Vector3MulEq2);
				VVector3.SetOperatorEx(VMOpFunc::DivAssign, (uint32_t)VMOp::Left, "vector3&", "const vector3 &in", &Vector3DivEq1);
				VVector3.SetOperatorEx(VMOpFunc::DivAssign, (uint32_t)VMOp::Left, "vector3&", "float", &Vector3DivEq2);
				VVector3.SetOperatorEx(VMOpFunc::AddAssign, (uint32_t)VMOp::Left, "vector3&", "const vector3 &in", &Vector3AddEq1);
				VVector3.SetOperatorEx(VMOpFunc::AddAssign, (uint32_t)VMOp::Left, "vector3&", "float", &Vector3AddEq2);
				VVector3.SetOperatorEx(VMOpFunc::SubAssign, (uint32_t)VMOp::Left, "vector3&", "const vector3 &in", &Vector3SubEq1);
				VVector3.SetOperatorEx(VMOpFunc::SubAssign, (uint32_t)VMOp::Left, "vector3&", "float", &Vector3SubEq2);
				VVector3.SetOperatorEx(VMOpFunc::Mul, (uint32_t)VMOp::Const, "vector3", "const vector3 &in", &Vector3Mul1);
				VVector3.SetOperatorEx(VMOpFunc::Mul, (uint32_t)VMOp::Const, "vector3", "float", &Vector3Mul2);
				VVector3.SetOperatorEx(VMOpFunc::Div, (uint32_t)VMOp::Const, "vector3", "const vector3 &in", &Vector3Div1);
				VVector3.SetOperatorEx(VMOpFunc::Div, (uint32_t)VMOp::Const, "vector3", "float", &Vector3Div2);
				VVector3.SetOperatorEx(VMOpFunc::Add, (uint32_t)VMOp::Const, "vector3", "const vector3 &in", &Vector3Add1);
				VVector3.SetOperatorEx(VMOpFunc::Add, (uint32_t)VMOp::Const, "vector3", "float", &Vector3Add2);
				VVector3.SetOperatorEx(VMOpFunc::Sub, (uint32_t)VMOp::Const, "vector3", "const vector3 &in", &Vector3Sub1);
				VVector3.SetOperatorEx(VMOpFunc::Sub, (uint32_t)VMOp::Const, "vector3", "float", &Vector3Sub2);
				VVector3.SetOperatorEx(VMOpFunc::Neg, (uint32_t)VMOp::Const, "vector3", "", &Vector3Neg);
				VVector3.SetOperatorEx(VMOpFunc::Equals, (uint32_t)VMOp::Const, "bool", "const vector3 &in", &Vector3Eq);
				VVector3.SetOperatorEx(VMOpFunc::Cmp, (uint32_t)VMOp::Const, "int", "const vector3 &in", &Vector3Cmp);

				Engine->BeginNamespace("vector3");
				Register.SetFunction("vector3 random()", &Compute::Vector3::Random);
				Register.SetFunction("vector3 random_abs()", &Compute::Vector3::RandomAbs);
				Register.SetFunction("vector3 one()", &Compute::Vector3::One);
				Register.SetFunction("vector3 zero()", &Compute::Vector3::Zero);
				Register.SetFunction("vector3 up()", &Compute::Vector3::Up);
				Register.SetFunction("vector3 down()", &Compute::Vector3::Down);
				Register.SetFunction("vector3 left()", &Compute::Vector3::Left);
				Register.SetFunction("vector3 right()", &Compute::Vector3::Right);
				Register.SetFunction("vector3 forward()", &Compute::Vector3::Forward);
				Register.SetFunction("vector3 backward()", &Compute::Vector3::Backward);
				Engine->EndNamespace();

				VMTypeClass VVector4 = Register.SetPod<Compute::Vector4>("vector4");
				VVector4.SetConstructor<Compute::Vector4>("void f()");
				VVector4.SetConstructor<Compute::Vector4, float, float>("void f(float, float)");
				VVector4.SetConstructor<Compute::Vector4, float, float, float>("void f(float, float, float)");
				VVector4.SetConstructor<Compute::Vector4, float, float, float, float>("void f(float, float, float, float)");
				VVector4.SetConstructor<Compute::Vector4, float>("void f(float)");
				VVector4.SetConstructor<Compute::Vector4, const Compute::Vector2&>("void f(const vector2 &in)");
				VVector4.SetConstructor<Compute::Vector4, const Compute::Vector3&>("void f(const vector3 &in)");
				VVector4.SetProperty<Compute::Vector4>("float x", &Compute::Vector4::X);
				VVector4.SetProperty<Compute::Vector4>("float y", &Compute::Vector4::Y);
				VVector4.SetProperty<Compute::Vector4>("float z", &Compute::Vector4::Z);
				VVector4.SetProperty<Compute::Vector4>("float w", &Compute::Vector4::W);
				VVector4.SetMethod("float length() const", &Compute::Vector4::Length);
				VVector4.SetMethod("float sum() const", &Compute::Vector4::Sum);
				VVector4.SetMethod("float dot(const vector4 &in) const", &Compute::Vector4::Dot);
				VVector4.SetMethod("float distance(const vector4 &in) const", &Compute::Vector4::Distance);
				VVector4.SetMethod("float cross(const vector4 &in) const", &Compute::Vector4::Cross);
				VVector4.SetMethod("vector4 inv() const", &Compute::Vector4::Inv);
				VVector4.SetMethod("vector4 inv_x() const", &Compute::Vector4::InvX);
				VVector4.SetMethod("vector4 inv_y() const", &Compute::Vector4::InvY);
				VVector4.SetMethod("vector4 inv_z() const", &Compute::Vector4::InvZ);
				VVector4.SetMethod("vector4 inv_w() const", &Compute::Vector4::InvW);
				VVector4.SetMethod("vector4 normalize() const", &Compute::Vector4::Normalize);
				VVector4.SetMethod("vector4 snormalize() const", &Compute::Vector4::sNormalize);
				VVector4.SetMethod("vector4 lerp(const vector4 &in, float) const", &Compute::Vector4::Lerp);
				VVector4.SetMethod("vector4 slerp(const vector4 &in, float) const", &Compute::Vector4::sLerp);
				VVector4.SetMethod("vector4 alerp(const vector4 &in, float) const", &Compute::Vector4::aLerp);
				VVector4.SetMethod("vector4 rlerp() const", &Compute::Vector4::rLerp);
				VVector4.SetMethod("vector4 abs() const", &Compute::Vector4::Abs);
				VVector4.SetMethod("vector4 radians() const", &Compute::Vector4::Radians);
				VVector4.SetMethod("vector4 degrees() const", &Compute::Vector4::Degrees);
				VVector4.SetMethod("vector2 xy() const", &Compute::Vector4::XY);
				VVector4.SetMethod("vector3 xyz() const", &Compute::Vector4::XYZ);
				VVector4.SetMethod("vector4 xyzw() const", &Compute::Vector4::XYZW);
				VVector4.SetMethod<Compute::Vector4, Compute::Vector4, float>("vector4 mul(float) const", &Compute::Vector4::Mul);
				VVector4.SetMethod<Compute::Vector4, Compute::Vector4, const Compute::Vector2&, float, float>("vector4 mul(const vector2 &in, float, float) const", &Compute::Vector4::Mul);
				VVector4.SetMethod<Compute::Vector4, Compute::Vector4, const Compute::Vector3&, float>("vector4 mul(const vector3 &in, float) const", &Compute::Vector4::Mul);
				VVector4.SetMethod<Compute::Vector4, Compute::Vector4, const Compute::Vector4&>("vector4 mul(const vector4 &in) const", &Compute::Vector4::Mul);
				VVector4.SetMethod("vector4 div(const vector4 &in) const", &Compute::Vector4::Div);
				VVector4.SetMethod("vector4 set_x(float) const", &Compute::Vector4::SetX);
				VVector4.SetMethod("vector4 set_y(float) const", &Compute::Vector4::SetY);
				VVector4.SetMethod("vector4 set_z(float) const", &Compute::Vector4::SetZ);
				VVector4.SetMethod("vector4 set_w(float) const", &Compute::Vector4::SetW);
				VVector4.SetMethod("void set(const vector4 &in) const", &Compute::Vector4::Set);
				VVector4.SetOperatorEx(VMOpFunc::MulAssign, (uint32_t)VMOp::Left, "vector4&", "const vector4 &in", &Vector4MulEq1);
				VVector4.SetOperatorEx(VMOpFunc::MulAssign, (uint32_t)VMOp::Left, "vector4&", "float", &Vector4MulEq2);
				VVector4.SetOperatorEx(VMOpFunc::DivAssign, (uint32_t)VMOp::Left, "vector4&", "const vector4 &in", &Vector4DivEq1);
				VVector4.SetOperatorEx(VMOpFunc::DivAssign, (uint32_t)VMOp::Left, "vector4&", "float", &Vector4DivEq2);
				VVector4.SetOperatorEx(VMOpFunc::AddAssign, (uint32_t)VMOp::Left, "vector4&", "const vector4 &in", &Vector4AddEq1);
				VVector4.SetOperatorEx(VMOpFunc::AddAssign, (uint32_t)VMOp::Left, "vector4&", "float", &Vector4AddEq2);
				VVector4.SetOperatorEx(VMOpFunc::SubAssign, (uint32_t)VMOp::Left, "vector4&", "const vector4 &in", &Vector4SubEq1);
				VVector4.SetOperatorEx(VMOpFunc::SubAssign, (uint32_t)VMOp::Left, "vector4&", "float", &Vector4SubEq2);
				VVector4.SetOperatorEx(VMOpFunc::Mul, (uint32_t)VMOp::Const, "vector4", "const vector4 &in", &Vector4Mul1);
				VVector4.SetOperatorEx(VMOpFunc::Mul, (uint32_t)VMOp::Const, "vector4", "float", &Vector4Mul2);
				VVector4.SetOperatorEx(VMOpFunc::Div, (uint32_t)VMOp::Const, "vector4", "const vector4 &in", &Vector4Div1);
				VVector4.SetOperatorEx(VMOpFunc::Div, (uint32_t)VMOp::Const, "vector4", "float", &Vector4Div2);
				VVector4.SetOperatorEx(VMOpFunc::Add, (uint32_t)VMOp::Const, "vector4", "const vector4 &in", &Vector4Add1);
				VVector4.SetOperatorEx(VMOpFunc::Add, (uint32_t)VMOp::Const, "vector4", "float", &Vector4Add2);
				VVector4.SetOperatorEx(VMOpFunc::Sub, (uint32_t)VMOp::Const, "vector4", "const vector4 &in", &Vector4Sub1);
				VVector4.SetOperatorEx(VMOpFunc::Sub, (uint32_t)VMOp::Const, "vector4", "float", &Vector4Sub2);
				VVector4.SetOperatorEx(VMOpFunc::Neg, (uint32_t)VMOp::Const, "vector4", "", &Vector4Neg);
				VVector4.SetOperatorEx(VMOpFunc::Equals, (uint32_t)VMOp::Const, "bool", "const vector4 &in", &Vector4Eq);
				VVector4.SetOperatorEx(VMOpFunc::Cmp, (uint32_t)VMOp::Const, "int", "const vector4 &in", &Vector4Cmp);

				Engine->BeginNamespace("vector4");
				Register.SetFunction("vector4 random()", &Compute::Vector4::Random);
				Register.SetFunction("vector4 random_abs()", &Compute::Vector4::RandomAbs);
				Register.SetFunction("vector4 one()", &Compute::Vector4::One);
				Register.SetFunction("vector4 zero()", &Compute::Vector4::Zero);
				Register.SetFunction("vector4 up()", &Compute::Vector4::Up);
				Register.SetFunction("vector4 down()", &Compute::Vector4::Down);
				Register.SetFunction("vector4 left()", &Compute::Vector4::Left);
				Register.SetFunction("vector4 right()", &Compute::Vector4::Right);
				Register.SetFunction("vector4 forward()", &Compute::Vector4::Forward);
				Register.SetFunction("vector4 backward()", &Compute::Vector4::Backward);
				Register.SetFunction("vector4 unitW()", &Compute::Vector4::UnitW);
				Engine->EndNamespace();

				VMTypeClass VMatrix4x4 = Register.SetPod<Compute::Matrix4x4>("matrix4x4");
				VMatrix4x4.SetConstructor<Compute::Matrix4x4>("void f()");
				VMatrix4x4.SetConstructor<Compute::Matrix4x4, const Compute::Vector4&, const Compute::Vector4&, const Compute::Vector4&, const Compute::Vector4&>("void f(const vector4 &in, const vector4 &in, const vector4 &in, const vector4 &in)");
				VMatrix4x4.SetConstructor<Compute::Matrix4x4, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float>("void f(float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float)");
				VMatrix4x4.SetMethod("matrix4x4 inv() const", &Compute::Matrix4x4::Inv);
				VMatrix4x4.SetMethod("matrix4x4 transpose() const", &Compute::Matrix4x4::Transpose);
				VMatrix4x4.SetMethod("matrix4x4 set_scale(const vector3 &in) const", &Compute::Matrix4x4::SetScale);
				VMatrix4x4.SetMethod("vector4 row11() const", &Compute::Matrix4x4::Row11);
				VMatrix4x4.SetMethod("vector4 row22() const", &Compute::Matrix4x4::Row22);
				VMatrix4x4.SetMethod("vector4 row33() const", &Compute::Matrix4x4::Row33);
				VMatrix4x4.SetMethod("vector4 row44() const", &Compute::Matrix4x4::Row44);
				VMatrix4x4.SetMethod("vector3 up(bool) const", &Compute::Matrix4x4::Up);
				VMatrix4x4.SetMethod("vector3 right(bool) const", &Compute::Matrix4x4::Right);
				VMatrix4x4.SetMethod("vector3 forward(bool) const", &Compute::Matrix4x4::Forward);
				VMatrix4x4.SetMethod("vector3 rotation() const", &Compute::Matrix4x4::Rotation);
				VMatrix4x4.SetMethod("vector3 position() const", &Compute::Matrix4x4::Position);
				VMatrix4x4.SetMethod("vector3 scale() const", &Compute::Matrix4x4::Scale);
				VMatrix4x4.SetMethod("vector3 xy() const", &Compute::Matrix4x4::XY);
				VMatrix4x4.SetMethod("vector3 xyz() const", &Compute::Matrix4x4::XYZ);
				VMatrix4x4.SetMethod("vector3 xyzw() const", &Compute::Matrix4x4::XYZW);
				VMatrix4x4.SetMethod("void identify()", &Compute::Matrix4x4::Identify);
				VMatrix4x4.SetMethod("void set(const matrix4x4 &in)", &Compute::Matrix4x4::Set);
				VMatrix4x4.SetMethod<Compute::Matrix4x4, Compute::Matrix4x4, const Compute::Matrix4x4&>("matrix4x4 mul(const matrix4x4 &in) const", &Compute::Matrix4x4::Mul);
				VMatrix4x4.SetMethod<Compute::Matrix4x4, Compute::Matrix4x4, const Compute::Vector4&>("matrix4x4 mul(const vector4 &in) const", &Compute::Matrix4x4::Mul);
				VMatrix4x4.SetOperatorEx(VMOpFunc::Index, (uint32_t)VMOp::Left, "float&", "usize", &Matrix4x4GetRow);
				VMatrix4x4.SetOperatorEx(VMOpFunc::Index, (uint32_t)VMOp::Const, "const float&", "usize", &Matrix4x4GetRow);
				VMatrix4x4.SetOperatorEx(VMOpFunc::Equals, (uint32_t)VMOp::Const, "bool", "const matrix4x4 &in", &Matrix4x4Eq);
				VMatrix4x4.SetOperatorEx(VMOpFunc::Mul, (uint32_t)VMOp::Const, "matrix4x4", "const matrix4x4 &in", &Matrix4x4Mul1);
				VMatrix4x4.SetOperatorEx(VMOpFunc::Mul, (uint32_t)VMOp::Const, "vector4", "const vector4 &in", &Matrix4x4Mul2);

				Engine->BeginNamespace("matrix4x4");
				Register.SetFunction("matrix4x4 identity()", &Compute::Matrix4x4::Identity);
				Register.SetFunction("matrix4x4 create_rotation_x(float)", &Compute::Matrix4x4::CreateRotationX);
				Register.SetFunction("matrix4x4 create_rotation_y(float)", &Compute::Matrix4x4::CreateRotationY);
				Register.SetFunction("matrix4x4 create_rotation_z(float)", &Compute::Matrix4x4::CreateRotationZ);
				Register.SetFunction("matrix4x4 create_origin(const vector3 &in, const vector3 &in)", &Compute::Matrix4x4::CreateOrigin);
				Register.SetFunction("matrix4x4 create_scale(const vector3 &in)", &Compute::Matrix4x4::CreateScale);
				Register.SetFunction("matrix4x4 create_translated_scale(const vector3& in, const vector3 &in)", &Compute::Matrix4x4::CreateTranslatedScale);
				Register.SetFunction("matrix4x4 create_translation(const vector3& in)", &Compute::Matrix4x4::CreateTranslation);
				Register.SetFunction("matrix4x4 create_perspective(float, float, float, float)", &Compute::Matrix4x4::CreatePerspective);
				Register.SetFunction("matrix4x4 create_perspective_rad(float, float, float, float)", &Compute::Matrix4x4::CreatePerspectiveRad);
				Register.SetFunction("matrix4x4 create_orthographic(float, float, float, float)", &Compute::Matrix4x4::CreateOrthographic);
				Register.SetFunction("matrix4x4 create_orthographic_off_center(float, float, float, float, float, float)", &Compute::Matrix4x4::CreateOrthographicOffCenter);
				Register.SetFunction<Compute::Matrix4x4(const Compute::Vector3&)>("matrix4x4 createRotation(const vector3 &in)", &Compute::Matrix4x4::CreateRotation);
				Register.SetFunction<Compute::Matrix4x4(const Compute::Vector3&, const Compute::Vector3&, const Compute::Vector3&)>("matrix4x4 create_rotation(const vector3 &in, const vector3 &in, const vector3 &in)", &Compute::Matrix4x4::CreateRotation);
				Register.SetFunction<Compute::Matrix4x4(const Compute::Vector3&, const Compute::Vector3&, const Compute::Vector3&)>("matrix4x4 create_look_at(const vector3 &in, const vector3 &in, const vector3 &in)", &Compute::Matrix4x4::CreateLookAt);
				Register.SetFunction<Compute::Matrix4x4(Compute::CubeFace, const Compute::Vector3&)>("matrix4x4 create_look_at(cube_face, const vector3 &in)", &Compute::Matrix4x4::CreateLookAt);
				Register.SetFunction<Compute::Matrix4x4(const Compute::Vector3&, const Compute::Vector3&, const Compute::Vector3&)>("matrix4x4 create(const vector3 &in, const vector3 &in, const vector3 &in)", &Compute::Matrix4x4::Create);
				Register.SetFunction<Compute::Matrix4x4(const Compute::Vector3&, const Compute::Vector3&)>("matrix4x4 create(const vector3 &in, const vector3 &in)", &Compute::Matrix4x4::Create);
				Engine->EndNamespace();

				VMTypeClass VQuaternion = Register.SetPod<Compute::Vector4>("quaternion");
				VQuaternion.SetConstructor<Compute::Quaternion>("void f()");
				VQuaternion.SetConstructor<Compute::Quaternion, float, float, float, float>("void f(float, float, float, float)");
				VQuaternion.SetConstructor<Compute::Quaternion, const Compute::Vector3&, float>("void f(const vector3 &in, float)");
				VQuaternion.SetConstructor<Compute::Quaternion, const Compute::Vector3&>("void f(const vector3 &in)");
				VQuaternion.SetConstructor<Compute::Quaternion, const Compute::Matrix4x4&>("void f(const matrix4x4 &in)");
				VQuaternion.SetProperty<Compute::Quaternion>("float x", &Compute::Quaternion::X);
				VQuaternion.SetProperty<Compute::Quaternion>("float y", &Compute::Quaternion::Y);
				VQuaternion.SetProperty<Compute::Quaternion>("float z", &Compute::Quaternion::Z);
				VQuaternion.SetProperty<Compute::Quaternion>("float w", &Compute::Quaternion::W);
				VQuaternion.SetMethod("void set_axis(const vector3 &in, float)", &Compute::Quaternion::SetAxis);
				VQuaternion.SetMethod("void set_euler(const vector3 &in)", &Compute::Quaternion::SetEuler);
				VQuaternion.SetMethod("void set_matrix(const matrix4x4 &in)", &Compute::Quaternion::SetMatrix);
				VQuaternion.SetMethod("void set(const quaternion &in)", &Compute::Quaternion::Set);
				VQuaternion.SetMethod("quaternion normalize() const", &Compute::Quaternion::Normalize);
				VQuaternion.SetMethod("quaternion snormalize() const", &Compute::Quaternion::sNormalize);
				VQuaternion.SetMethod("quaternion conjugate() const", &Compute::Quaternion::Conjugate);
				VQuaternion.SetMethod("quaternion sub(const quaternion &in) const", &Compute::Quaternion::Sub);
				VQuaternion.SetMethod("quaternion add(const quaternion &in) const", &Compute::Quaternion::Add);
				VQuaternion.SetMethod("quaternion lerp(const quaternion &in, float) const", &Compute::Quaternion::Lerp);
				VQuaternion.SetMethod("quaternion slerp(const quaternion &in, float) const", &Compute::Quaternion::sLerp);
				VQuaternion.SetMethod("vector3 forward() const", &Compute::Quaternion::Forward);
				VQuaternion.SetMethod("vector3 up() const", &Compute::Quaternion::Up);
				VQuaternion.SetMethod("vector3 right() const", &Compute::Quaternion::Right);
				VQuaternion.SetMethod("matrix4x4 get_matrix() const", &Compute::Quaternion::GetMatrix);
				VQuaternion.SetMethod("vector3 get_euler() const", &Compute::Quaternion::GetEuler);
				VQuaternion.SetMethod("float dot(const quaternion &in) const", &Compute::Quaternion::Dot);
				VQuaternion.SetMethod("float length() const", &Compute::Quaternion::Length);
				VQuaternion.SetMethod<Compute::Quaternion, Compute::Quaternion, float>("quaternion mul(float) const", &Compute::Quaternion::Mul);
				VQuaternion.SetMethod<Compute::Quaternion, Compute::Quaternion, const Compute::Quaternion&>("quaternion mul(const quaternion &in) const", &Compute::Quaternion::Mul);
				VQuaternion.SetMethod<Compute::Quaternion, Compute::Vector3, const Compute::Vector3&>("vector3 mul(const vector3 &in) const", &Compute::Quaternion::Mul);
				VQuaternion.SetOperatorEx(VMOpFunc::Mul, (uint32_t)VMOp::Const, "vector3", "const vector3 &in", &QuaternionMul1);
				VQuaternion.SetOperatorEx(VMOpFunc::Mul, (uint32_t)VMOp::Const, "quaternion", "const quaternion &in", &QuaternionMul2);
				VQuaternion.SetOperatorEx(VMOpFunc::Mul, (uint32_t)VMOp::Const, "quaternion", "float", &QuaternionMul3);
				VQuaternion.SetOperatorEx(VMOpFunc::Add, (uint32_t)VMOp::Const, "quaternion", "const quaternion &in", &QuaternionAdd);
				VQuaternion.SetOperatorEx(VMOpFunc::Sub, (uint32_t)VMOp::Const, "quaternion", "const quaternion &in", &QuaternionSub);

				Engine->BeginNamespace("quaternion");
				Register.SetFunction("quaternion create_euler_rotation(const vector3 &in)", &Compute::Quaternion::CreateEulerRotation);
				Register.SetFunction("quaternion create_rotation(const matrix4x4 &in)", &Compute::Quaternion::CreateRotation);
				Engine->EndNamespace();

				return true;
			}
			bool Registry::LoadShapes(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();

				VMTypeClass VRectangle = Register.SetPod<Compute::Rectangle>("rectangle");
				VRectangle.SetProperty<Compute::Rectangle>("int64 left", &Compute::Rectangle::Left);
				VRectangle.SetProperty<Compute::Rectangle>("int64 top", &Compute::Rectangle::Top);
				VRectangle.SetProperty<Compute::Rectangle>("int64 right", &Compute::Rectangle::Right);
				VRectangle.SetProperty<Compute::Rectangle>("int64 bottom", &Compute::Rectangle::Bottom);
				VRectangle.SetConstructor<Compute::Rectangle>("void f()");

				VMTypeClass VBounding = Register.SetPod<Compute::Bounding>("bounding");
				VBounding.SetProperty<Compute::Bounding>("vector3 lower", &Compute::Bounding::Lower);
				VBounding.SetProperty<Compute::Bounding>("vector3 upper", &Compute::Bounding::Upper);
				VBounding.SetProperty<Compute::Bounding>("vector3 center", &Compute::Bounding::Center);
				VBounding.SetProperty<Compute::Bounding>("float radius", &Compute::Bounding::Radius);
				VBounding.SetProperty<Compute::Bounding>("float volume", &Compute::Bounding::Volume);
				VBounding.SetConstructor<Compute::Bounding>("void f()");
				VBounding.SetConstructor<Compute::Bounding, const Compute::Vector3&, const Compute::Vector3&>("void f(const vector3 &in, const vector3 &in)");
				VBounding.SetMethod("void merge(const bounding &in, const bounding &in)", &Compute::Bounding::Merge);
				VBounding.SetMethod("bool contains(const bounding &in) const", &Compute::Bounding::Contains);
				VBounding.SetMethod("bool overlaps(const bounding &in) const", &Compute::Bounding::Overlaps);

				VMTypeClass VRay = Register.SetPod<Compute::Ray>("ray");
				VBounding.SetProperty<Compute::Ray>("vector3 origin", &Compute::Ray::Origin);
				VBounding.SetProperty<Compute::Ray>("vector3 direction", &Compute::Ray::Direction);
				VBounding.SetConstructor<Compute::Ray>("void f()");
				VBounding.SetConstructor<Compute::Ray, const Compute::Vector3&, const Compute::Vector3&>("void f(const vector3 &in, const vector3 &in)");
				VBounding.SetMethod("vector3 get_point(float) const", &Compute::Ray::GetPoint);
				VBounding.SetMethod("bool intersects_plane(const vector3 &in, float) const", &Compute::Ray::IntersectsPlane);
				VBounding.SetMethod("bool intersects_sphere(const vector3 &in, float, bool = true) const", &Compute::Ray::IntersectsSphere);
				VBounding.SetMethod("bool intersects_aabb_at(const vector3 &in, const vector3 &in, vector3 &out) const", &Compute::Ray::IntersectsAABBAt);
				VBounding.SetMethod("bool intersects_aabb(const vector3 &in, const vector3 &in, vector3 &out) const", &Compute::Ray::IntersectsAABB);
				VBounding.SetMethod("bool intersects_obb(const matrix4x4 &in, vector3 &out) const", &Compute::Ray::IntersectsOBB);

				VMTypeClass VFrustum8C = Register.SetPod<Compute::Frustum8C>("frustum_8c");
				VFrustum8C.SetConstructor<Compute::Frustum8C>("void f()");
				VFrustum8C.SetConstructor<Compute::Frustum8C, float, float, float, float>("void f(float, float, float, float)");
				VFrustum8C.SetMethod("void transform(const matrix4x4 &in) const", &Compute::Frustum8C::Transform);
				VFrustum8C.SetMethod("void get_bounding_box(vector2 &out, vector2 &out, vector2 &out) const", &Compute::Frustum8C::GetBoundingBox);
				VFrustum8C.SetOperatorEx(VMOpFunc::Index, (uint32_t)VMOp::Left, "vector4&", "usize", &Frustum8CGetCorners);
				VFrustum8C.SetOperatorEx(VMOpFunc::Index, (uint32_t)VMOp::Const, "const vector4&", "usize", &Frustum8CGetCorners);

				VMTypeClass VFrustum6P = Register.SetPod<Compute::Frustum6P>("frustum_6p");
				VFrustum6P.SetConstructor<Compute::Frustum6P>("void f()");
				VFrustum6P.SetConstructor<Compute::Frustum6P, const Compute::Matrix4x4&>("void f(const matrix4x4 &in)");
				VFrustum6P.SetMethod("bool overlaps_aabb(const bounding &in) const", &Compute::Frustum6P::OverlapsAABB);
				VFrustum6P.SetMethod("bool overlaps_sphere(const vector3 &in, float) const", &Compute::Frustum6P::OverlapsSphere);
				VFrustum6P.SetOperatorEx(VMOpFunc::Index, (uint32_t)VMOp::Left, "vector4&", "usize", &Frustum6PGetCorners);
				VFrustum6P.SetOperatorEx(VMOpFunc::Index, (uint32_t)VMOp::Const, "const vector4&", "usize", &Frustum6PGetCorners);

				return true;
			}
			bool Registry::LoadKeyFrames(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();

				VMTypeClass VJoint = Register.SetStructUnmanaged<Compute::Joint>("joint");
				VJoint.SetProperty<Compute::Joint>("string name", &Compute::Joint::Name);
				VJoint.SetProperty<Compute::Joint>("matrix4x4 transform", &Compute::Joint::Transform);
				VJoint.SetProperty<Compute::Joint>("matrix4x4 bind_shape", &Compute::Joint::BindShape);
				VJoint.SetProperty<Compute::Joint>("int64 index", &Compute::Joint::Index);
				VJoint.SetConstructor<Compute::Joint>("void f()");
				VJoint.SetMethodEx("usize size() const", &JointSize);
				VJoint.SetOperatorEx(VMOpFunc::Index, (uint32_t)VMOp::Left, "joint&", "usize", &JointGetChilds);
				VJoint.SetOperatorEx(VMOpFunc::Index, (uint32_t)VMOp::Const, "const joint&", "usize", &JointGetChilds);

				VMTypeClass VAnimatorKey = Register.SetPod<Compute::AnimatorKey>("animator_key");
				VAnimatorKey.SetProperty<Compute::AnimatorKey>("vector3 position", &Compute::AnimatorKey::Position);
				VAnimatorKey.SetProperty<Compute::AnimatorKey>("vector3 rotation", &Compute::AnimatorKey::Rotation);
				VAnimatorKey.SetProperty<Compute::AnimatorKey>("vector3 scale", &Compute::AnimatorKey::Scale);
				VAnimatorKey.SetProperty<Compute::AnimatorKey>("float time", &Compute::AnimatorKey::Time);
				VAnimatorKey.SetConstructor<Compute::AnimatorKey>("void f()");

				VMTypeClass VSkinAnimatorKey = Register.SetStructUnmanaged<Compute::SkinAnimatorKey>("skin_animator_key");
				VSkinAnimatorKey.SetProperty<Compute::SkinAnimatorKey>("float time", &Compute::SkinAnimatorKey::Time);
				VSkinAnimatorKey.SetConstructor<Compute::AnimatorKey>("void f()");
				VSkinAnimatorKey.SetMethodEx("usize size() const", &SkinAnimatorKeySize);
				VSkinAnimatorKey.SetOperatorEx(VMOpFunc::Index, (uint32_t)VMOp::Left, "animator_key&", "usize", &SkinAnimatorKeyGetPose);
				VSkinAnimatorKey.SetOperatorEx(VMOpFunc::Index, (uint32_t)VMOp::Const, "const animator_key&", "usize", &SkinAnimatorKeyGetPose);

				VMTypeClass VSkinAnimatorClip = Register.SetStructUnmanaged<Compute::SkinAnimatorClip>("skin_animator_clip");
				VSkinAnimatorClip.SetProperty<Compute::SkinAnimatorClip>("string name", &Compute::SkinAnimatorClip::Name);
				VSkinAnimatorClip.SetProperty<Compute::SkinAnimatorClip>("float duration", &Compute::SkinAnimatorClip::Duration);
				VSkinAnimatorClip.SetProperty<Compute::SkinAnimatorClip>("float rate", &Compute::SkinAnimatorClip::Rate);
				VSkinAnimatorClip.SetConstructor<Compute::SkinAnimatorClip>("void f()");
				VSkinAnimatorClip.SetMethodEx("usize size() const", &SkinAnimatorClipSize);
				VSkinAnimatorClip.SetOperatorEx(VMOpFunc::Index, (uint32_t)VMOp::Left, "skin_animator_key&", "usize", &SkinAnimatorClipGetKeys);
				VSkinAnimatorClip.SetOperatorEx(VMOpFunc::Index, (uint32_t)VMOp::Const, "const skin_animator_key&", "usize", &SkinAnimatorClipGetKeys);

				VMTypeClass VKeyAnimatorClip = Register.SetStructUnmanaged<Compute::KeyAnimatorClip>("key_animator_clip");
				VKeyAnimatorClip.SetProperty<Compute::KeyAnimatorClip>("string name", &Compute::KeyAnimatorClip::Name);
				VKeyAnimatorClip.SetProperty<Compute::KeyAnimatorClip>("float duration", &Compute::KeyAnimatorClip::Duration);
				VKeyAnimatorClip.SetProperty<Compute::KeyAnimatorClip>("float rate", &Compute::KeyAnimatorClip::Rate);
				VKeyAnimatorClip.SetConstructor<Compute::KeyAnimatorClip>("void f()");
				VKeyAnimatorClip.SetMethodEx("usize size() const", &KeyAnimatorClipSize);
				VKeyAnimatorClip.SetOperatorEx(VMOpFunc::Index, (uint32_t)VMOp::Left, "animator_key&", "usize", &KeyAnimatorClipGetKeys);
				VKeyAnimatorClip.SetOperatorEx(VMOpFunc::Index, (uint32_t)VMOp::Const, "const animator_key&", "usize", &KeyAnimatorClipGetKeys);

				VMTypeClass VRandomVector2 = Register.SetPod<Compute::RandomVector2>("random_vector2");
				VRandomVector2.SetProperty<Compute::RandomVector2>("vector2 min", &Compute::RandomVector2::Min);
				VRandomVector2.SetProperty<Compute::RandomVector2>("vector2 max", &Compute::RandomVector2::Max);
				VRandomVector2.SetProperty<Compute::RandomVector2>("bool intensity", &Compute::RandomVector2::Intensity);
				VRandomVector2.SetProperty<Compute::RandomVector2>("float accuracy", &Compute::RandomVector2::Accuracy);
				VRandomVector2.SetConstructor<Compute::RandomVector2>("void f()");
				VRandomVector2.SetConstructor<Compute::RandomVector2, const Compute::Vector2&, const Compute::Vector2&, bool, float>("void f(const vector2 &in, const vector2 &in, bool, float)");
				VRandomVector2.SetMethod("vector2 generate()", &Compute::RandomVector2::Generate);

				VMTypeClass VRandomVector3 = Register.SetPod<Compute::RandomVector3>("random_vector3");
				VRandomVector3.SetProperty<Compute::RandomVector3>("vector3 min", &Compute::RandomVector3::Min);
				VRandomVector3.SetProperty<Compute::RandomVector3>("vector3 max", &Compute::RandomVector3::Max);
				VRandomVector3.SetProperty<Compute::RandomVector3>("bool intensity", &Compute::RandomVector3::Intensity);
				VRandomVector3.SetProperty<Compute::RandomVector3>("float accuracy", &Compute::RandomVector3::Accuracy);
				VRandomVector3.SetConstructor<Compute::RandomVector3>("void f()");
				VRandomVector3.SetConstructor<Compute::RandomVector3, const Compute::Vector3&, const Compute::Vector3&, bool, float>("void f(const vector3 &in, const vector3 &in, bool, float)");
				VRandomVector3.SetMethod("vector3 generate()", &Compute::RandomVector3::Generate);

				VMTypeClass VRandomVector4 = Register.SetPod<Compute::RandomVector4>("random_vector4");
				VRandomVector4.SetProperty<Compute::RandomVector4>("vector4 min", &Compute::RandomVector4::Min);
				VRandomVector4.SetProperty<Compute::RandomVector4>("vector4 max", &Compute::RandomVector4::Max);
				VRandomVector4.SetProperty<Compute::RandomVector4>("bool intensity", &Compute::RandomVector4::Intensity);
				VRandomVector4.SetProperty<Compute::RandomVector4>("float accuracy", &Compute::RandomVector4::Accuracy);
				VRandomVector4.SetConstructor<Compute::RandomVector4>("void f()");
				VRandomVector4.SetConstructor<Compute::RandomVector4, const Compute::Vector4&, const Compute::Vector4&, bool, float>("void f(const vector4 &in, const vector4 &in, bool, float)");
				VRandomVector4.SetMethod("vector4 generate()", &Compute::RandomVector4::Generate);

				VMTypeClass VRandomFloat = Register.SetPod<Compute::RandomFloat>("random_float");
				VRandomFloat.SetProperty<Compute::RandomFloat>("float min", &Compute::RandomFloat::Min);
				VRandomFloat.SetProperty<Compute::RandomFloat>("float max", &Compute::RandomFloat::Max);
				VRandomFloat.SetProperty<Compute::RandomFloat>("bool intensity", &Compute::RandomFloat::Intensity);
				VRandomFloat.SetProperty<Compute::RandomFloat>("float accuracy", &Compute::RandomFloat::Accuracy);
				VRandomFloat.SetConstructor<Compute::RandomFloat>("void f()");
				VRandomFloat.SetConstructor<Compute::RandomFloat, float, float, bool, float>("void f(float, float, bool, float)");
				VRandomFloat.SetMethod("float generate()", &Compute::RandomFloat::Generate);

				return true;
			}
			bool Registry::LoadRegex(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();

				VMEnum VRegexState = Register.SetEnum("regex_state");
				VRegexState.SetValue("preprocessed", (int)Compute::RegexState::Preprocessed);
				VRegexState.SetValue("match_found", (int)Compute::RegexState::Match_Found);
				VRegexState.SetValue("no_match", (int)Compute::RegexState::No_Match);
				VRegexState.SetValue("unexpected_quantifier", (int)Compute::RegexState::Unexpected_Quantifier);
				VRegexState.SetValue("unbalanced_brackets", (int)Compute::RegexState::Unbalanced_Brackets);
				VRegexState.SetValue("internal_error", (int)Compute::RegexState::Internal_Error);
				VRegexState.SetValue("invalid_character_set", (int)Compute::RegexState::Invalid_Character_Set);
				VRegexState.SetValue("invalid_metacharacter", (int)Compute::RegexState::Invalid_Metacharacter);
				VRegexState.SetValue("sumatch_array_too_small", (int)Compute::RegexState::Sumatch_Array_Too_Small);
				VRegexState.SetValue("too_many_branches", (int)Compute::RegexState::Too_Many_Branches);
				VRegexState.SetValue("too_many_brackets", (int)Compute::RegexState::Too_Many_Brackets);

				VMTypeClass VRegexMatch = Register.SetPod<Compute::RegexMatch>("regex_match");
				VRegexMatch.SetProperty<Compute::RegexMatch>("int64 start", &Compute::RegexMatch::Start);
				VRegexMatch.SetProperty<Compute::RegexMatch>("int64 end", &Compute::RegexMatch::End);
				VRegexMatch.SetProperty<Compute::RegexMatch>("int64 length", &Compute::RegexMatch::Length);
				VRegexMatch.SetProperty<Compute::RegexMatch>("int64 steps", &Compute::RegexMatch::Steps);
				VRegexMatch.SetConstructor<Compute::RegexMatch>("void f()");
				VRegexMatch.SetMethodEx("string target() const", &RegexMatchTarget);

				VMTypeClass VRegexResult = Register.SetStructUnmanaged<Compute::RegexResult>("regex_result");
				VRegexResult.SetConstructor<Compute::RegexResult>("void f()");
				VRegexResult.SetMethod("bool empty() const", &Compute::RegexResult::Empty);
				VRegexResult.SetMethod("int64 get_steps() const", &Compute::RegexResult::GetSteps);
				VRegexResult.SetMethod("regex_state get_state() const", &Compute::RegexResult::GetState);
				VRegexResult.SetMethodEx("array<regex_match>@ get() const", &RegexResultGet);
				VRegexResult.SetMethodEx("array<string>@ to_array() const", &RegexResultToArray);

				VMTypeClass VRegexSource = Register.SetStructUnmanaged<Compute::RegexSource>("regex_source");
				VRegexSource.SetProperty<Compute::RegexSource>("bool ignoreCase", &Compute::RegexSource::IgnoreCase);
				VRegexSource.SetConstructor<Compute::RegexSource>("void f()");
				VRegexSource.SetConstructor<Compute::RegexSource, const std::string&, bool, int64_t, int64_t, int64_t>("void f(const string &in, bool = false, int64 = -1, int64 = -1, int64 = -1)");
				VRegexSource.SetMethod("const string& get_regex() const", &Compute::RegexSource::GetRegex);
				VRegexSource.SetMethod("int64 get_max_branches() const", &Compute::RegexSource::GetMaxBranches);
				VRegexSource.SetMethod("int64 get_max_brackets() const", &Compute::RegexSource::GetMaxBrackets);
				VRegexSource.SetMethod("int64 get_max_matches() const", &Compute::RegexSource::GetMaxMatches);
				VRegexSource.SetMethod("int64 get_complexity() const", &Compute::RegexSource::GetComplexity);
				VRegexSource.SetMethod("regex_state getState() const", &Compute::RegexSource::GetState);
				VRegexSource.SetMethod("bool is_simple() const", &Compute::RegexSource::IsSimple);
				VRegexSource.SetMethodEx("regex_result match(const string &in) const", &RegexSourceMatch);
				VRegexSource.SetMethodEx("string replace(const string &in, const string &in) const", &RegexSourceReplace);

				return true;
			}
			bool Registry::LoadCrypto(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();

				VMTypeClass VPrivateKey = Register.SetStructUnmanaged<Compute::PrivateKey>("private_key");
				VPrivateKey.SetConstructor<Compute::PrivateKey>("void f()");
				VPrivateKey.SetConstructor<Compute::PrivateKey, const std::string&>("void f(const string &in)");
				VPrivateKey.SetMethod("void clear()", &Compute::PrivateKey::Clear);
				VPrivateKey.SetMethod<Compute::PrivateKey, void, const std::string&>("void secure(const string &in)", &Compute::PrivateKey::Secure);
				VPrivateKey.SetMethod("string expose_to_heap() const", &Compute::PrivateKey::ExposeToHeap);
				VPrivateKey.SetMethod("usize size() const", &Compute::PrivateKey::Clear);
				VPrivateKey.SetMethodStatic<Compute::PrivateKey, const std::string&>("private_key get_plain(const string &in)", &Compute::PrivateKey::GetPlain);

				VMRefClass VWebToken = Engine->Global().SetClassUnmanaged<Compute::WebToken>("web_token");
				VWebToken.SetProperty<Compute::WebToken>("schema@ header", &Compute::WebToken::Header);
				VWebToken.SetProperty<Compute::WebToken>("schema@ payload", &Compute::WebToken::Payload);
				VWebToken.SetProperty<Compute::WebToken>("schema@ token", &Compute::WebToken::Token);
				VWebToken.SetProperty<Compute::WebToken>("string refresher", &Compute::WebToken::Refresher);
				VWebToken.SetProperty<Compute::WebToken>("string signature", &Compute::WebToken::Signature);
				VWebToken.SetProperty<Compute::WebToken>("string data", &Compute::WebToken::Data);
				VWebToken.SetUnmanagedConstructor<Compute::WebToken>("web_token@ f()");
				VWebToken.SetUnmanagedConstructor<Compute::WebToken, const std::string&, const std::string&, int64_t>("web_token@ f(const string &in, const string &in, int64)");
				VWebToken.SetMethod("void unsign()", &Compute::WebToken::Unsign);
				VWebToken.SetMethod("void set_algorithm(const string &in)", &Compute::WebToken::SetAlgorithm);
				VWebToken.SetMethod("void set_type(const string &in)", &Compute::WebToken::SetType);
				VWebToken.SetMethod("void set_content_type(const string &in)", &Compute::WebToken::SetContentType);
				VWebToken.SetMethod("void set_issuer(const string &in)", &Compute::WebToken::SetIssuer);
				VWebToken.SetMethod("void set_subject(const string &in)", &Compute::WebToken::SetSubject);
				VWebToken.SetMethod("void set_id(const string &in)", &Compute::WebToken::SetId);
				VWebToken.SetMethod("void set_expiration(int64)", &Compute::WebToken::SetExpiration);
				VWebToken.SetMethod("void set_not_before(int64)", &Compute::WebToken::SetNotBefore);
				VWebToken.SetMethod("void set_created(int64)", &Compute::WebToken::SetCreated);
				VWebToken.SetMethod("void set_refresh_token(const string &in, const private_key &in, const private_key &in)", &Compute::WebToken::SetRefreshToken);
				VWebToken.SetMethod("bool sign(const private_key &in)", &Compute::WebToken::Sign);
				VWebToken.SetMethod("string get_refresh_token(const private_key &in, const private_key &in)", &Compute::WebToken::GetRefreshToken);
				VWebToken.SetMethod("bool is_valid()", &Compute::WebToken::IsValid);
				VWebToken.SetMethodEx("void set_audience(array<string>@+)", &WebTokenSetAudience);

				Engine->BeginNamespace("ciphers");
				Register.SetFunction("uptr@ des_ecb()", &Compute::Ciphers::DES_ECB);
				Register.SetFunction("uptr@ des_ede()", &Compute::Ciphers::DES_EDE);
				Register.SetFunction("uptr@ des_ede3()", &Compute::Ciphers::DES_EDE3);
				Register.SetFunction("uptr@ des_ede_ecb()", &Compute::Ciphers::DES_EDE_ECB);
				Register.SetFunction("uptr@ des_ede3_ecb()", &Compute::Ciphers::DES_EDE3_ECB);
				Register.SetFunction("uptr@ des_cfb64()", &Compute::Ciphers::DES_CFB64);
				Register.SetFunction("uptr@ des_cfb1()", &Compute::Ciphers::DES_CFB1);
				Register.SetFunction("uptr@ des_cfb8()", &Compute::Ciphers::DES_CFB8);
				Register.SetFunction("uptr@ des_ede_cfb64()", &Compute::Ciphers::DES_EDE_CFB64);
				Register.SetFunction("uptr@ des_ede3_cfb64()", &Compute::Ciphers::DES_EDE3_CFB64);
				Register.SetFunction("uptr@ des_ede3_cfb1()", &Compute::Ciphers::DES_EDE3_CFB1);
				Register.SetFunction("uptr@ des_ede3_cfb8()", &Compute::Ciphers::DES_EDE3_CFB8);
				Register.SetFunction("uptr@ des_ofb()", &Compute::Ciphers::DES_OFB);
				Register.SetFunction("uptr@ des_ede_ofb()", &Compute::Ciphers::DES_EDE_OFB);
				Register.SetFunction("uptr@ des_ede3_ofb()", &Compute::Ciphers::DES_EDE3_OFB);
				Register.SetFunction("uptr@ des_cbc()", &Compute::Ciphers::DES_CBC);
				Register.SetFunction("uptr@ des_ede_cbc()", &Compute::Ciphers::DES_EDE_CBC);
				Register.SetFunction("uptr@ des_ede3_cbc()", &Compute::Ciphers::DES_EDE3_CBC);
				Register.SetFunction("uptr@ des_ede3_wrap()", &Compute::Ciphers::DES_EDE3_Wrap);
				Register.SetFunction("uptr@ desx_cbc()", &Compute::Ciphers::DESX_CBC);
				Register.SetFunction("uptr@ rc4()", &Compute::Ciphers::RC4);
				Register.SetFunction("uptr@ rc4_40()", &Compute::Ciphers::RC4_40);
				Register.SetFunction("uptr@ rc4_hmac_md5()", &Compute::Ciphers::RC4_HMAC_MD5);
				Register.SetFunction("uptr@ idea_ecb()", &Compute::Ciphers::IDEA_ECB);
				Register.SetFunction("uptr@ idea_cfb64()", &Compute::Ciphers::IDEA_CFB64);
				Register.SetFunction("uptr@ idea_ofb()", &Compute::Ciphers::IDEA_OFB);
				Register.SetFunction("uptr@ idea_cbc()", &Compute::Ciphers::IDEA_CBC);
				Register.SetFunction("uptr@ rc2_ecb()", &Compute::Ciphers::RC2_ECB);
				Register.SetFunction("uptr@ rc2_cbc()", &Compute::Ciphers::RC2_CBC);
				Register.SetFunction("uptr@ rc2_40_cbc()", &Compute::Ciphers::RC2_40_CBC);
				Register.SetFunction("uptr@ rc2_64_cbc()", &Compute::Ciphers::RC2_64_CBC);
				Register.SetFunction("uptr@ rc2_cfb64()", &Compute::Ciphers::RC2_CFB64);
				Register.SetFunction("uptr@ rc2_ofb()", &Compute::Ciphers::RC2_OFB);
				Register.SetFunction("uptr@ bf_ecb()", &Compute::Ciphers::BF_ECB);
				Register.SetFunction("uptr@ bf_cbc()", &Compute::Ciphers::BF_CBC);
				Register.SetFunction("uptr@ bf_cfb64()", &Compute::Ciphers::BF_CFB64);
				Register.SetFunction("uptr@ bf_ofb()", &Compute::Ciphers::BF_OFB);
				Register.SetFunction("uptr@ cast5_ecb()", &Compute::Ciphers::CAST5_ECB);
				Register.SetFunction("uptr@ cast5_cbc()", &Compute::Ciphers::CAST5_CBC);
				Register.SetFunction("uptr@ cast5_cfb64()", &Compute::Ciphers::CAST5_CFB64);
				Register.SetFunction("uptr@ cast5_ofb()", &Compute::Ciphers::CAST5_OFB);
				Register.SetFunction("uptr@ rc5_32_12_16_cbc()", &Compute::Ciphers::RC5_32_12_16_CBC);
				Register.SetFunction("uptr@ rc5_32_12_16_ecb()", &Compute::Ciphers::RC5_32_12_16_ECB);
				Register.SetFunction("uptr@ rc5_32_12_16_cfb64()", &Compute::Ciphers::RC5_32_12_16_CFB64);
				Register.SetFunction("uptr@ rc5_32_12_16_ofb()", &Compute::Ciphers::RC5_32_12_16_OFB);
				Register.SetFunction("uptr@ aes_128_ecb()", &Compute::Ciphers::AES_128_ECB);
				Register.SetFunction("uptr@ aes_128_cbc()", &Compute::Ciphers::AES_128_CBC);
				Register.SetFunction("uptr@ aes_128_cfb1()", &Compute::Ciphers::AES_128_CFB1);
				Register.SetFunction("uptr@ aes_128_cfb8()", &Compute::Ciphers::AES_128_CFB8);
				Register.SetFunction("uptr@ aes_128_cfb128()", &Compute::Ciphers::AES_128_CFB128);
				Register.SetFunction("uptr@ aes_128_ofb()", &Compute::Ciphers::AES_128_OFB);
				Register.SetFunction("uptr@ aes_128_ctr()", &Compute::Ciphers::AES_128_CTR);
				Register.SetFunction("uptr@ aes_128_ccm()", &Compute::Ciphers::AES_128_CCM);
				Register.SetFunction("uptr@ aes_128_gcm()", &Compute::Ciphers::AES_128_GCM);
				Register.SetFunction("uptr@ aes_128_xts()", &Compute::Ciphers::AES_128_XTS);
				Register.SetFunction("uptr@ aes_128_wrap()", &Compute::Ciphers::AES_128_Wrap);
				Register.SetFunction("uptr@ aes_128_wrap_pad()", &Compute::Ciphers::AES_128_WrapPad);
				Register.SetFunction("uptr@ aes_128_ocb()", &Compute::Ciphers::AES_128_OCB);
				Register.SetFunction("uptr@ aes_192_ecb()", &Compute::Ciphers::AES_192_ECB);
				Register.SetFunction("uptr@ aes_192_cbc()", &Compute::Ciphers::AES_192_CBC);
				Register.SetFunction("uptr@ aes_192_cfb1()", &Compute::Ciphers::AES_192_CFB1);
				Register.SetFunction("uptr@ aes_192_cfb8()", &Compute::Ciphers::AES_192_CFB8);
				Register.SetFunction("uptr@ aes_192_cfb128()", &Compute::Ciphers::AES_192_CFB128);
				Register.SetFunction("uptr@ aes_192_ofb()", &Compute::Ciphers::AES_192_OFB);
				Register.SetFunction("uptr@ aes_192_ctr()", &Compute::Ciphers::AES_192_CTR);
				Register.SetFunction("uptr@ aes_192_ccm()", &Compute::Ciphers::AES_192_CCM);
				Register.SetFunction("uptr@ aes_192_gcm()", &Compute::Ciphers::AES_192_GCM);
				Register.SetFunction("uptr@ aes_192_wrap()", &Compute::Ciphers::AES_192_Wrap);
				Register.SetFunction("uptr@ aes_192_wrap_pad()", &Compute::Ciphers::AES_192_WrapPad);
				Register.SetFunction("uptr@ aes_192_ocb()", &Compute::Ciphers::AES_192_OCB);
				Register.SetFunction("uptr@ aes_256_ecb()", &Compute::Ciphers::AES_256_ECB);
				Register.SetFunction("uptr@ aes_256_cbc()", &Compute::Ciphers::AES_256_CBC);
				Register.SetFunction("uptr@ aes_256_cfb1()", &Compute::Ciphers::AES_256_CFB1);
				Register.SetFunction("uptr@ aes_256_cfb8()", &Compute::Ciphers::AES_256_CFB8);
				Register.SetFunction("uptr@ aes_256_cfb128()", &Compute::Ciphers::AES_256_CFB128);
				Register.SetFunction("uptr@ aes_256_ofb()", &Compute::Ciphers::AES_256_OFB);
				Register.SetFunction("uptr@ aes_256_ctr()", &Compute::Ciphers::AES_256_CTR);
				Register.SetFunction("uptr@ aes_256_ccm()", &Compute::Ciphers::AES_256_CCM);
				Register.SetFunction("uptr@ aes_256_gcm()", &Compute::Ciphers::AES_256_GCM);
				Register.SetFunction("uptr@ aes_256_xts()", &Compute::Ciphers::AES_256_XTS);
				Register.SetFunction("uptr@ aes_256_wrap()", &Compute::Ciphers::AES_256_Wrap);
				Register.SetFunction("uptr@ aes_256_wrap_pad()", &Compute::Ciphers::AES_256_WrapPad);
				Register.SetFunction("uptr@ aes_256_ocb()", &Compute::Ciphers::AES_256_OCB);
				Register.SetFunction("uptr@ aes_128_cbc_hmac_SHA1()", &Compute::Ciphers::AES_128_CBC_HMAC_SHA1);
				Register.SetFunction("uptr@ aes_256_cbc_hmac_SHA1()", &Compute::Ciphers::AES_256_CBC_HMAC_SHA1);
				Register.SetFunction("uptr@ aes_128_cbc_hmac_SHA256()", &Compute::Ciphers::AES_128_CBC_HMAC_SHA256);
				Register.SetFunction("uptr@ aes_256_cbc_hmac_SHA256()", &Compute::Ciphers::AES_256_CBC_HMAC_SHA256);
				Register.SetFunction("uptr@ aria_128_ecb()", &Compute::Ciphers::ARIA_128_ECB);
				Register.SetFunction("uptr@ aria_128_cbc()", &Compute::Ciphers::ARIA_128_CBC);
				Register.SetFunction("uptr@ aria_128_cfb1()", &Compute::Ciphers::ARIA_128_CFB1);
				Register.SetFunction("uptr@ aria_128_cfb8()", &Compute::Ciphers::ARIA_128_CFB8);
				Register.SetFunction("uptr@ aria_128_cfb128()", &Compute::Ciphers::ARIA_128_CFB128);
				Register.SetFunction("uptr@ aria_128_ctr()", &Compute::Ciphers::ARIA_128_CTR);
				Register.SetFunction("uptr@ aria_128_ofb()", &Compute::Ciphers::ARIA_128_OFB);
				Register.SetFunction("uptr@ aria_128_gcm()", &Compute::Ciphers::ARIA_128_GCM);
				Register.SetFunction("uptr@ aria_128_ccm()", &Compute::Ciphers::ARIA_128_CCM);
				Register.SetFunction("uptr@ aria_192_ecb()", &Compute::Ciphers::ARIA_192_ECB);
				Register.SetFunction("uptr@ aria_192_cbc()", &Compute::Ciphers::ARIA_192_CBC);
				Register.SetFunction("uptr@ aria_192_cfb1()", &Compute::Ciphers::ARIA_192_CFB1);
				Register.SetFunction("uptr@ aria_192_cfb8()", &Compute::Ciphers::ARIA_192_CFB8);
				Register.SetFunction("uptr@ aria_192_cfb128()", &Compute::Ciphers::ARIA_192_CFB128);
				Register.SetFunction("uptr@ aria_192_ctr()", &Compute::Ciphers::ARIA_192_CTR);
				Register.SetFunction("uptr@ aria_192_ofb()", &Compute::Ciphers::ARIA_192_OFB);
				Register.SetFunction("uptr@ aria_192_gcm()", &Compute::Ciphers::ARIA_192_GCM);
				Register.SetFunction("uptr@ aria_192_ccm()", &Compute::Ciphers::ARIA_192_CCM);
				Register.SetFunction("uptr@ aria_256_ecb()", &Compute::Ciphers::ARIA_256_ECB);
				Register.SetFunction("uptr@ aria_256_cbc()", &Compute::Ciphers::ARIA_256_CBC);
				Register.SetFunction("uptr@ aria_256_cfb1()", &Compute::Ciphers::ARIA_256_CFB1);
				Register.SetFunction("uptr@ aria_256_cfb8()", &Compute::Ciphers::ARIA_256_CFB8);
				Register.SetFunction("uptr@ aria_256_cfb128()", &Compute::Ciphers::ARIA_256_CFB128);
				Register.SetFunction("uptr@ aria_256_ctr()", &Compute::Ciphers::ARIA_256_CTR);
				Register.SetFunction("uptr@ aria_256_ofb()", &Compute::Ciphers::ARIA_256_OFB);
				Register.SetFunction("uptr@ aria_256_gcm()", &Compute::Ciphers::ARIA_256_GCM);
				Register.SetFunction("uptr@ aria_256_ccm()", &Compute::Ciphers::ARIA_256_CCM);
				Register.SetFunction("uptr@ camellia_128_ecb()", &Compute::Ciphers::Camellia_128_ECB);
				Register.SetFunction("uptr@ camellia_128_cbc()", &Compute::Ciphers::Camellia_128_CBC);
				Register.SetFunction("uptr@ camellia_128_cfb1()", &Compute::Ciphers::Camellia_128_CFB1);
				Register.SetFunction("uptr@ camellia_128_cfb8()", &Compute::Ciphers::Camellia_128_CFB8);
				Register.SetFunction("uptr@ camellia_128_cfb128()", &Compute::Ciphers::Camellia_128_CFB128);
				Register.SetFunction("uptr@ camellia_128_ofb()", &Compute::Ciphers::Camellia_128_OFB);
				Register.SetFunction("uptr@ camellia_128_ctr()", &Compute::Ciphers::Camellia_128_CTR);
				Register.SetFunction("uptr@ camellia_192_ecb()", &Compute::Ciphers::Camellia_192_ECB);
				Register.SetFunction("uptr@ camellia_192_cbc()", &Compute::Ciphers::Camellia_192_CBC);
				Register.SetFunction("uptr@ camellia_192_cfb1()", &Compute::Ciphers::Camellia_192_CFB1);
				Register.SetFunction("uptr@ camellia_192_cfb8()", &Compute::Ciphers::Camellia_192_CFB8);
				Register.SetFunction("uptr@ camellia_192_cfb128()", &Compute::Ciphers::Camellia_192_CFB128);
				Register.SetFunction("uptr@ camellia_192_ofb()", &Compute::Ciphers::Camellia_192_OFB);
				Register.SetFunction("uptr@ camellia_192_ctr()", &Compute::Ciphers::Camellia_192_CTR);
				Register.SetFunction("uptr@ camellia_256_ecb()", &Compute::Ciphers::Camellia_256_ECB);
				Register.SetFunction("uptr@ camellia_256_cbc()", &Compute::Ciphers::Camellia_256_CBC);
				Register.SetFunction("uptr@ camellia_256_cfb1()", &Compute::Ciphers::Camellia_256_CFB1);
				Register.SetFunction("uptr@ camellia_256_cfb8()", &Compute::Ciphers::Camellia_256_CFB8);
				Register.SetFunction("uptr@ camellia_256_cfb128()", &Compute::Ciphers::Camellia_256_CFB128);
				Register.SetFunction("uptr@ camellia_256_ofb()", &Compute::Ciphers::Camellia_256_OFB);
				Register.SetFunction("uptr@ camellia_256_ctr()", &Compute::Ciphers::Camellia_256_CTR);
				Register.SetFunction("uptr@ chacha20()", &Compute::Ciphers::Chacha20);
				Register.SetFunction("uptr@ chacha20_poly1305()", &Compute::Ciphers::Chacha20_Poly1305);
				Register.SetFunction("uptr@ seed_ecb()", &Compute::Ciphers::Seed_ECB);
				Register.SetFunction("uptr@ seed_cbc()", &Compute::Ciphers::Seed_CBC);
				Register.SetFunction("uptr@ seed_cfb128()", &Compute::Ciphers::Seed_CFB128);
				Register.SetFunction("uptr@ seed_ofb()", &Compute::Ciphers::Seed_OFB);
				Register.SetFunction("uptr@ sm4_ecb()", &Compute::Ciphers::SM4_ECB);
				Register.SetFunction("uptr@ sm4_cbc()", &Compute::Ciphers::SM4_CBC);
				Register.SetFunction("uptr@ sm4_cfb128()", &Compute::Ciphers::SM4_CFB128);
				Register.SetFunction("uptr@ sm4_ofb()", &Compute::Ciphers::SM4_OFB);
				Register.SetFunction("uptr@ sm4_ctr()", &Compute::Ciphers::SM4_CTR);
				Engine->EndNamespace();

				Engine->BeginNamespace("digests");
				Register.SetFunction("uptr@ md2()", &Compute::Digests::MD2);
				Register.SetFunction("uptr@ md4()", &Compute::Digests::MD4);
				Register.SetFunction("uptr@ md5()", &Compute::Digests::MD5);
				Register.SetFunction("uptr@ md5_sha1()", &Compute::Digests::MD5_SHA1);
				Register.SetFunction("uptr@ blake2b512()", &Compute::Digests::Blake2B512);
				Register.SetFunction("uptr@ blake2s256()", &Compute::Digests::Blake2S256);
				Register.SetFunction("uptr@ sha1()", &Compute::Digests::SHA1);
				Register.SetFunction("uptr@ sha224()", &Compute::Digests::SHA224);
				Register.SetFunction("uptr@ sha256()", &Compute::Digests::SHA256);
				Register.SetFunction("uptr@ sha384()", &Compute::Digests::SHA384);
				Register.SetFunction("uptr@ sha512()", &Compute::Digests::SHA512);
				Register.SetFunction("uptr@ sha512_224()", &Compute::Digests::SHA512_224);
				Register.SetFunction("uptr@ sha512_256()", &Compute::Digests::SHA512_256);
				Register.SetFunction("uptr@ sha3_224()", &Compute::Digests::SHA3_224);
				Register.SetFunction("uptr@ sha3_256()", &Compute::Digests::SHA3_256);
				Register.SetFunction("uptr@ sha3_384()", &Compute::Digests::SHA3_384);
				Register.SetFunction("uptr@ sha3_512()", &Compute::Digests::SHA3_512);
				Register.SetFunction("uptr@ shake128()", &Compute::Digests::Shake128);
				Register.SetFunction("uptr@ shake256()", &Compute::Digests::Shake256);
				Register.SetFunction("uptr@ mdc2()", &Compute::Digests::MDC2);
				Register.SetFunction("uptr@ ripemd160()", &Compute::Digests::RipeMD160);
				Register.SetFunction("uptr@ whirlpool()", &Compute::Digests::Whirlpool);
				Register.SetFunction("uptr@ sm3()", &Compute::Digests::SM3);
				Engine->EndNamespace();

				Engine->BeginNamespace("crypto");
				Register.SetFunction("string random_bytes(usize)", &Compute::Crypto::RandomBytes);
				Register.SetFunction("string hash(uptr@, const string &in)", &Compute::Crypto::Hash);
				Register.SetFunction("string hash_binary(uptr@, const string &in)", &Compute::Crypto::HashBinary);
				Register.SetFunction<std::string(Compute::Digest, const std::string&, const Compute::PrivateKey&)>("string sign(uptr@, const string &in, const private_key &in)", &Compute::Crypto::Sign);
				Register.SetFunction<std::string(Compute::Digest, const std::string&, const Compute::PrivateKey&)>("string hmac(uptr@, const string &in, const private_key &in)", &Compute::Crypto::HMAC);
				Register.SetFunction<std::string(Compute::Digest, const std::string&, const Compute::PrivateKey&, const Compute::PrivateKey&, int)>("string encrypt(uptr@, const string &in, const private_key &in, const private_key &in, int = -1)", &Compute::Crypto::Encrypt);
				Register.SetFunction<std::string(Compute::Digest, const std::string&, const Compute::PrivateKey&, const Compute::PrivateKey&, int)>("string decrypt(uptr@, const string &in, const private_key &in, const private_key &in, int = -1)", &Compute::Crypto::Decrypt);
				Register.SetFunction("string jwt_sign(const string &in, const string &in, const private_key &in)", &Compute::Crypto::JWTSign);
				Register.SetFunction("string jwt_encode(web_token@+, const private_key &in)", &Compute::Crypto::JWTEncode);
				Register.SetFunction("web_token@ jwt_decode(const string &in, const private_key &in)", &Compute::Crypto::JWTDecode);
				Register.SetFunction("string doc_encrypt(schema@+, const private_key &in, const private_key &in)", &Compute::Crypto::DocEncrypt);
				Register.SetFunction("schema@ doc_decrypt(const string &in, const private_key &in, const private_key &in)", &Compute::Crypto::DocDecrypt);
				Register.SetFunction("uint8 random_uc()", &Compute::Crypto::RandomUC);
				Register.SetFunction<uint64_t()>("uint64 random()", &Compute::Crypto::Random);
				Register.SetFunction<uint64_t(uint64_t, uint64_t)>("uint64 random(uint64, uint64)", &Compute::Crypto::Random);
				Register.SetFunction("uint64 crc32(const string &in)", &Compute::Crypto::CRC32);
				Register.SetFunction("void display_crypto_log()", &Compute::Crypto::DisplayCryptoLog);
				Engine->EndNamespace();

				Engine->BeginNamespace("codec");
				Register.SetFunction("string move(const string &in, int)", &Compute::Codec::Move);
				Register.SetFunction("string bep45_encode(const string &in)", &Compute::Codec::Bep45Encode);
				Register.SetFunction("string bep45_decode(const string &in)", &Compute::Codec::Bep45Decode);
				Register.SetFunction("string base45_encode(const string &in)", &Compute::Codec::Base45Encode);
				Register.SetFunction("string base45_decode(const string &in)", &Compute::Codec::Base45Decode);
				Register.SetFunction<std::string(const std::string&)>("string base65_encode(const string &in)", &Compute::Codec::Base64Encode);
				Register.SetFunction<std::string(const std::string&)>("string base65_decode(const string &in)", &Compute::Codec::Base64Decode);
				Register.SetFunction<std::string(const std::string&)>("string base64_url_encode(const string &in)", &Compute::Codec::Base64URLEncode);
				Register.SetFunction<std::string(const std::string&)>("string base64_url_decode(const string &in)", &Compute::Codec::Base64URLDecode);
				Register.SetFunction<std::string(const std::string&)>("string hex_dncode(const string &in)", &Compute::Codec::HexEncode);
				Register.SetFunction<std::string(const std::string&)>("string hex_decode(const string &in)", &Compute::Codec::HexDecode);
				Register.SetFunction<std::string(const std::string&)>("string uri_encode(const string &in)", &Compute::Codec::URIEncode);
				Register.SetFunction<std::string(const std::string&)>("string uri_decode(const string &in)", &Compute::Codec::URIDecode);
				Register.SetFunction("string decimal_to_hex(uint64)", &Compute::Codec::DecimalToHex);
				Register.SetFunction("string base10_to_base_n(uint64, uint8)", &Compute::Codec::Base10ToBaseN);
				Engine->EndNamespace();

				return true;
			}
			bool Registry::LoadGeometric(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();

				VMEnum VPositioning = Register.SetEnum("positioning");
				VPositioning.SetValue("local", (int)Compute::Positioning::Local);
				VPositioning.SetValue("global", (int)Compute::Positioning::Global);

				VMTypeClass VSpacing = Engine->Global().SetPod<Compute::Transform::Spacing>("transform_spacing");
				VSpacing.SetProperty<Compute::Transform::Spacing>("matrix4x4 offset", &Compute::Transform::Spacing::Offset);
				VSpacing.SetProperty<Compute::Transform::Spacing>("vector3 position", &Compute::Transform::Spacing::Position);
				VSpacing.SetProperty<Compute::Transform::Spacing>("vector3 rotation", &Compute::Transform::Spacing::Rotation);
				VSpacing.SetProperty<Compute::Transform::Spacing>("vector3 scale", &Compute::Transform::Spacing::Scale);
				VSpacing.SetConstructor<Compute::Transform::Spacing>("void f()");

				VMRefClass VTransform = Engine->Global().SetClassUnmanaged<Compute::Transform>("transform");
				VTransform.SetProperty<Compute::Transform>("uptr@ user_data", &Compute::Transform::UserData);
				VTransform.SetUnmanagedConstructor<Compute::Transform, void*>("transform@ f(uptr@)");
				VTransform.SetMethod("void synchronize()", &Compute::Transform::Synchronize);
				VTransform.SetMethod("void move(const vector3 &in)", &Compute::Transform::Move);
				VTransform.SetMethod("void rotate(const vector3 &in)", &Compute::Transform::Rotate);
				VTransform.SetMethod("void rescale(const vector3 &in)", &Compute::Transform::Rescale);
				VTransform.SetMethod("void localize(transform_spacing &in)", &Compute::Transform::Localize);
				VTransform.SetMethod("void globalize(transform_spacing &in)", &Compute::Transform::Globalize);
				VTransform.SetMethod("void specialize(transform_spacing &in)", &Compute::Transform::Specialize);
				VTransform.SetMethod("void copy(transform@+) const", &Compute::Transform::Copy);
				VTransform.SetMethod("void add_child(transform@+)", &Compute::Transform::AddChild);
				VTransform.SetMethod("void remove_child(transform@+)", &Compute::Transform::RemoveChild);
				VTransform.SetMethod("void remove_childs()", &Compute::Transform::RemoveChilds);
				VTransform.SetMethod("void make_dirty()", &Compute::Transform::MakeDirty);
				VTransform.SetMethod("void set_scaling(bool)", &Compute::Transform::SetScaling);
				VTransform.SetMethod("void set_position(const vector3 &in)", &Compute::Transform::SetPosition);
				VTransform.SetMethod("void set_rotation(const vector3 &in)", &Compute::Transform::SetRotation);
				VTransform.SetMethod("void set_scale(const vector3 &in)", &Compute::Transform::SetScale);
				VTransform.SetMethod("void set_spacing(positioning, transform_spacing &in)", &Compute::Transform::SetSpacing);
				VTransform.SetMethod("void set_pivot(transform@+, transform_spacing &in)", &Compute::Transform::SetPivot);
				VTransform.SetMethod("void set_root(transform@+)", &Compute::Transform::SetRoot);
				VTransform.SetMethod("void get_bounds(matrix4x4 &in, vector3 &in, vector3 &in)", &Compute::Transform::GetBounds);
				VTransform.SetMethod("bool has_root(transform@+) const", &Compute::Transform::HasRoot);
				VTransform.SetMethod("bool has_child(transform@+) const", &Compute::Transform::HasChild);
				VTransform.SetMethod("bool has_scaling() const", &Compute::Transform::HasScaling);
				VTransform.SetMethod("bool is_dirty() const", &Compute::Transform::IsDirty);
				VTransform.SetMethod("const matrix4x4& get_bias() const", &Compute::Transform::GetBias);
				VTransform.SetMethod("const matrix4x4& get_bias_unscaled() const", &Compute::Transform::GetBiasUnscaled);
				VTransform.SetMethod("const vector3& get_position() const", &Compute::Transform::GetPosition);
				VTransform.SetMethod("const vector3& get_rotation() const", &Compute::Transform::GetRotation);
				VTransform.SetMethod("const vector3& get_scale() const", &Compute::Transform::GetScale);
				VTransform.SetMethod("vector3 forward(bool = false) const", &Compute::Transform::Forward);
				VTransform.SetMethod("vector3 right(bool = false) const", &Compute::Transform::Right);
				VTransform.SetMethod("vector3 up(bool = false) const", &Compute::Transform::Up);
				VTransform.SetMethod<Compute::Transform, Compute::Transform::Spacing&>("transform_spacing& get_spacing()", &Compute::Transform::GetSpacing);
				VTransform.SetMethod<Compute::Transform, Compute::Transform::Spacing&, Compute::Positioning>("transform_spacing& get_spacing(positioning)", &Compute::Transform::GetSpacing);
				VTransform.SetMethod("transform@+ get_root() const", &Compute::Transform::GetRoot);
				VTransform.SetMethod("transform@+ get_upper_root() const", &Compute::Transform::GetUpperRoot);
				VTransform.SetMethod("transform@+ get_child(usize) const", &Compute::Transform::GetChild);
				VTransform.SetMethod("usize get_childs_count() const", &Compute::Transform::GetChildsCount);

				VMTypeClass VNode = Engine->Global().SetPod<Compute::Cosmos::Node>("cosmos_node");
				VNode.SetProperty<Compute::Cosmos::Node>("bounding bounds", &Compute::Cosmos::Node::Bounds);
				VNode.SetProperty<Compute::Cosmos::Node>("usize parent", &Compute::Cosmos::Node::Parent);
				VNode.SetProperty<Compute::Cosmos::Node>("usize next", &Compute::Cosmos::Node::Next);
				VNode.SetProperty<Compute::Cosmos::Node>("usize left", &Compute::Cosmos::Node::Left);
				VNode.SetProperty<Compute::Cosmos::Node>("usize right", &Compute::Cosmos::Node::Right);
				VNode.SetProperty<Compute::Cosmos::Node>("uptr@ item", &Compute::Cosmos::Node::Item);
				VNode.SetProperty<Compute::Cosmos::Node>("int32 height", &Compute::Cosmos::Node::Height);
				VNode.SetConstructor<Compute::Cosmos::Node>("void f()");
				VNode.SetMethod("bool is_leaf() const", &Compute::Cosmos::Node::IsLeaf);

				VMTypeClass VCosmos = Engine->Global().SetStructUnmanaged<Compute::Cosmos>("cosmos");
				VCosmos.SetFunctionDef("bool cosmos_query_overlaps(const bounding &in)");
				VCosmos.SetFunctionDef("void cosmos_query_match(uptr@)");
				VCosmos.SetConstructor<Compute::Cosmos>("void f(usize = 16)");
				VCosmos.SetMethod("void reserve(usize)", &Compute::Cosmos::Reserve);
				VCosmos.SetMethod("void clear()", &Compute::Cosmos::Clear);
				VCosmos.SetMethod("void remove_item(uptr@)", &Compute::Cosmos::RemoveItem);
				VCosmos.SetMethod("void insert_item(uptr@, const vector3 &in, const vector3 &in)", &Compute::Cosmos::InsertItem);
				VCosmos.SetMethod("void update_item(uptr@, const vector3 &in, const vector3 &in, bool = false)", &Compute::Cosmos::UpdateItem);
				VCosmos.SetMethod("const bounding& get_area(uptr@)", &Compute::Cosmos::GetArea);
				VCosmos.SetMethod("usize get_nodes_count() const", &Compute::Cosmos::GetNodesCount);
				VCosmos.SetMethod("usize get_height() const", &Compute::Cosmos::GetHeight);
				VCosmos.SetMethod("usize get_max_balance() const", &Compute::Cosmos::GetMaxBalance);
				VCosmos.SetMethod("usize get_root() const", &Compute::Cosmos::GetRoot);
				VCosmos.SetMethod("const cosmos_node& get_root_node() const", &Compute::Cosmos::GetRootNode);
				VCosmos.SetMethod("const cosmos_node& get_node(usize) const", &Compute::Cosmos::GetNode);
				VCosmos.SetMethod("float get_volume_ratio() const", &Compute::Cosmos::GetVolumeRatio);
				VCosmos.SetMethod("bool is_null(usize) const", &Compute::Cosmos::IsNull);
				VCosmos.SetMethod("bool is_empty() const", &Compute::Cosmos::IsEmpty);
				VCosmos.SetMethodEx("void query_index()", &CosmosQueryIndex);

				Engine->BeginNamespace("geometric");
				Register.SetFunction("bool is_cube_in_frustum(const matrix4x4 &in, float)", &Compute::Geometric::IsCubeInFrustum);
				Register.SetFunction("bool is_left_handed()", &Compute::Geometric::IsLeftHanded);
				Register.SetFunction("bool has_sphere_intersected(const vector3 &in, float, const vector3 &in, float)", &Compute::Geometric::HasSphereIntersected);
				Register.SetFunction("bool has_line_intersected(float, float, const vector3 &in, const vector3 &in, vector3 &out)", &Compute::Geometric::HasLineIntersected);
				Register.SetFunction("bool has_line_intersected_cube(const vector3 &in, const vector3 &in, const vector3 &in, const vector3 &in)", &Compute::Geometric::HasLineIntersectedCube);
				Register.SetFunction<bool(const Compute::Vector3&, const Compute::Vector3&, const Compute::Vector3&, int)>("bool has_point_intersected_cube(const vector3 &in, const vector3 &in, const vector3 &in, int32)", &Compute::Geometric::HasPointIntersectedCube);
				Register.SetFunction("bool has_point_intersected_rectangle(const vector3 &in, const vector3 &in, const vector3 &in)", &Compute::Geometric::HasPointIntersectedRectangle);
				Register.SetFunction<bool(const Compute::Vector3&, const Compute::Vector3&, const Compute::Vector3&)>("bool has_point_intersected_cube(const vector3 &in, const vector3 &in, const vector3 &in)", &Compute::Geometric::HasPointIntersectedCube);
				Register.SetFunction("bool has_sb_intersected(transform@+, transform@+)", &Compute::Geometric::HasSBIntersected);
				Register.SetFunction("bool has_obb_intersected(transform@+, transform@+)", &Compute::Geometric::HasOBBIntersected);
				Register.SetFunction("bool has_aabb_intersected(transform@+, transform@+)", &Compute::Geometric::HasAABBIntersected);
				Register.SetFunction<void(const Compute::SkinVertex&, const Compute::SkinVertex&, const Compute::SkinVertex&, Compute::Vector3&, Compute::Vector3&)>("void compute_influence_tangent_bitangent(const skin_vertex &in, const skin_vertex &in, const skin_vertex &in, vector3 &in, vector3 &in)", &Compute::Geometric::ComputeInfluenceTangentBitangent);
				Register.SetFunction<void(const Compute::SkinVertex&, const Compute::SkinVertex&, const Compute::SkinVertex&, Compute::Vector3&, Compute::Vector3&, Compute::Vector3&)>("void compute_influence_tangent_bitangent(const skin_vertex &in, const skin_vertex &in, const skin_vertex &in, vector3 &in, vector3 &in, vector3 &in)", &Compute::Geometric::ComputeInfluenceTangentBitangent);
				Register.SetFunction("void matrix_rh_to_lh(matrix4x4 &out)", &Compute::Geometric::MatrixRhToLh);
				Register.SetFunction("void set_left_handed(bool)", &Compute::Geometric::SetLeftHanded);
				Register.SetFunction("ray create_cursor_ray(const vector3 &in, const vector2 &in, const vector2 &in, const matrix4x4 &in, const matrix4x4 &in)", &Compute::Geometric::CreateCursorRay);
				Register.SetFunction<bool(const Compute::Ray&, const Compute::Vector3&, const Compute::Vector3&, Compute::Vector3*)>("bool cursor_ray_test(const ray &in, const vector3 &in, const vector3 &in, vector3 &out)", &Compute::Geometric::CursorRayTest);
				Register.SetFunction<bool(const Compute::Ray&, const Compute::Matrix4x4&, Compute::Vector3*)>("bool cursor_ray_test(const ray &in, const matrix4x4 &in, vector3 &out)", &Compute::Geometric::CursorRayTest);
				Register.SetFunction("float fast_inv_sqrt(float)", &Compute::Geometric::FastInvSqrt);
				Register.SetFunction("float fast_sqrt(float)", &Compute::Geometric::FastSqrt);
				Register.SetFunction("float aabb_volume(const vector3 &in, const vector3 &in)", &Compute::Geometric::AabbVolume);
				Engine->EndNamespace();

				return true;
			}
			bool Registry::LoadPreprocessor(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();

				VMTypeClass VIncludeDesc = Engine->Global().SetStructUnmanaged<Compute::IncludeDesc>("include_desc");
				VIncludeDesc.SetProperty<Compute::IncludeDesc>("string from", &Compute::IncludeDesc::From);
				VIncludeDesc.SetProperty<Compute::IncludeDesc>("string path", &Compute::IncludeDesc::Path);
				VIncludeDesc.SetProperty<Compute::IncludeDesc>("string root", &Compute::IncludeDesc::Root);
				VIncludeDesc.SetConstructor<Compute::IncludeDesc>("void f()");
				VIncludeDesc.SetMethodEx("void add_ext(const string &in)", &IncludeDescAddExt);
				VIncludeDesc.SetMethodEx("void remove_ext(const string &in)", &IncludeDescRemoveExt);

				VMTypeClass VIncludeResult = Engine->Global().SetStructUnmanaged<Compute::IncludeResult>("include_result");
				VIncludeResult.SetProperty<Compute::IncludeResult>("string module", &Compute::IncludeResult::Module);
				VIncludeResult.SetProperty<Compute::IncludeResult>("bool is_system", &Compute::IncludeResult::IsSystem);
				VIncludeResult.SetProperty<Compute::IncludeResult>("bool is_file", &Compute::IncludeResult::IsFile);
				VIncludeResult.SetConstructor<Compute::IncludeResult>("void f()");

				VMTypeClass VDesc = Engine->Global().SetPod<Compute::Preprocessor::Desc>("preprocessor_desc");
				VDesc.SetProperty<Compute::Preprocessor::Desc>("bool Pragmas", &Compute::Preprocessor::Desc::Pragmas);
				VDesc.SetProperty<Compute::Preprocessor::Desc>("bool Includes", &Compute::Preprocessor::Desc::Includes);
				VDesc.SetProperty<Compute::Preprocessor::Desc>("bool Defines", &Compute::Preprocessor::Desc::Defines);
				VDesc.SetProperty<Compute::Preprocessor::Desc>("bool Conditions", &Compute::Preprocessor::Desc::Conditions);
				VDesc.SetConstructor<Compute::Preprocessor::Desc>("void f()");

				VMRefClass VPreprocessor = Engine->Global().SetClassUnmanaged<Compute::Preprocessor>("preprocessor");
				VPreprocessor.SetFunctionDef("bool proc_include_event(preprocessor@+, const include_result &in, string &out)");
				VPreprocessor.SetFunctionDef("bool proc_pragma_event(preprocessor@+, const string &in, array<string>@+)");
				VPreprocessor.SetUnmanagedConstructor<Compute::Preprocessor>("preprocessor@ f(uptr@)");
				VPreprocessor.SetMethod("void set_include_options(const include_desc &in)", &Compute::Preprocessor::SetIncludeOptions);
				VPreprocessor.SetMethod("void set_features(const preprocessor_desc &in)", &Compute::Preprocessor::SetFeatures);
				VPreprocessor.SetMethod("void define(const string &in)", &Compute::Preprocessor::Define);
				VPreprocessor.SetMethod("void undefine(const string &in)", &Compute::Preprocessor::Undefine);
				VPreprocessor.SetMethod("void clear()", &Compute::Preprocessor::Clear);
				VPreprocessor.SetMethod("bool process(const string &in, string &out)", &Compute::Preprocessor::Process);
				VPreprocessor.SetMethod("const string& get_current_file_path() const", &Compute::Preprocessor::GetCurrentFilePath);
				VPreprocessor.SetMethodEx("void set_include_callback(proc_include_event@+)", &PreprocessorSetIncludeCallback);
				VPreprocessor.SetMethodEx("void set_pragma_callback(proc_pragma_event@+)", &PreprocessorSetPragmaCallback);
				VPreprocessor.SetMethodEx("bool is_defined(const string &in) const", &PreprocessorIsDefined);

				return true;
			}
			bool Registry::LoadPhysics(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();

				VMRefClass VSimulator = Engine->Global().SetClassUnmanaged<Compute::Simulator>("physics_simulator");
				
				VMEnum VShape = Register.SetEnum("physics_shape");
				VShape.SetValue("box", (int)Compute::Shape::Box);
				VShape.SetValue("triangle", (int)Compute::Shape::Triangle);
				VShape.SetValue("tetrahedral", (int)Compute::Shape::Tetrahedral);
				VShape.SetValue("convex_triangle_mesh", (int)Compute::Shape::Convex_Triangle_Mesh);
				VShape.SetValue("convex_hull", (int)Compute::Shape::Convex_Hull);
				VShape.SetValue("convex_point_cloud", (int)Compute::Shape::Convex_Point_Cloud);
				VShape.SetValue("convex_polyhedral", (int)Compute::Shape::Convex_Polyhedral);
				VShape.SetValue("implicit_convex_start", (int)Compute::Shape::Implicit_Convex_Start);
				VShape.SetValue("sphere", (int)Compute::Shape::Sphere);
				VShape.SetValue("multi_sphere", (int)Compute::Shape::Multi_Sphere);
				VShape.SetValue("capsule", (int)Compute::Shape::Capsule);
				VShape.SetValue("cone", (int)Compute::Shape::Cone);
				VShape.SetValue("convex", (int)Compute::Shape::Convex);
				VShape.SetValue("cylinder", (int)Compute::Shape::Cylinder);
				VShape.SetValue("uniform_scaling", (int)Compute::Shape::Uniform_Scaling);
				VShape.SetValue("minkowski_sum", (int)Compute::Shape::Minkowski_Sum);
				VShape.SetValue("minkowski_difference", (int)Compute::Shape::Minkowski_Difference);
				VShape.SetValue("box_2d", (int)Compute::Shape::Box_2D);
				VShape.SetValue("convex_2d", (int)Compute::Shape::Convex_2D);
				VShape.SetValue("custom_convex", (int)Compute::Shape::Custom_Convex);
				VShape.SetValue("concaves_start", (int)Compute::Shape::Concaves_Start);
				VShape.SetValue("triangle_mesh", (int)Compute::Shape::Triangle_Mesh);
				VShape.SetValue("triangle_mesh_scaled", (int)Compute::Shape::Triangle_Mesh_Scaled);
				VShape.SetValue("fast_concave_mesh", (int)Compute::Shape::Fast_Concave_Mesh);
				VShape.SetValue("terrain", (int)Compute::Shape::Terrain);
				VShape.SetValue("triangle_mesh_multimaterial", (int)Compute::Shape::Triangle_Mesh_Multimaterial);
				VShape.SetValue("empty", (int)Compute::Shape::Empty);
				VShape.SetValue("static_plane", (int)Compute::Shape::Static_Plane);
				VShape.SetValue("custom_concave", (int)Compute::Shape::Custom_Concave);
				VShape.SetValue("concaves_end", (int)Compute::Shape::Concaves_End);
				VShape.SetValue("compound", (int)Compute::Shape::Compound);
				VShape.SetValue("softbody", (int)Compute::Shape::Softbody);
				VShape.SetValue("hf_fluid", (int)Compute::Shape::HF_Fluid);
				VShape.SetValue("hf_fluid_bouyant_convex", (int)Compute::Shape::HF_Fluid_Bouyant_Convex);
				VShape.SetValue("invalid", (int)Compute::Shape::Invalid);

				VMEnum VMotionState = Register.SetEnum("physics_motion_state");
				VMotionState.SetValue("active", (int)Compute::MotionState::Active);
				VMotionState.SetValue("island_sleeping", (int)Compute::MotionState::Island_Sleeping);
				VMotionState.SetValue("deactivation_needed", (int)Compute::MotionState::Deactivation_Needed);
				VMotionState.SetValue("disable_deactivation", (int)Compute::MotionState::Disable_Deactivation);
				VMotionState.SetValue("disable_simulation", (int)Compute::MotionState::Disable_Simulation);

				VMEnum VSoftFeature = Register.SetEnum("physics_soft_feature");
				VSoftFeature.SetValue("none", (int)Compute::SoftFeature::None);
				VSoftFeature.SetValue("node", (int)Compute::SoftFeature::Node);
				VSoftFeature.SetValue("link", (int)Compute::SoftFeature::Link);
				VSoftFeature.SetValue("face", (int)Compute::SoftFeature::Face);
				VSoftFeature.SetValue("tetra", (int)Compute::SoftFeature::Tetra);

				VMEnum VSoftAeroModel = Register.SetEnum("physics_soft_aero_model");
				VSoftAeroModel.SetValue("vpoint", (int)Compute::SoftAeroModel::VPoint);
				VSoftAeroModel.SetValue("vtwo_sided", (int)Compute::SoftAeroModel::VTwoSided);
				VSoftAeroModel.SetValue("vtwo_sided_lift_drag", (int)Compute::SoftAeroModel::VTwoSidedLiftDrag);
				VSoftAeroModel.SetValue("vone_sided", (int)Compute::SoftAeroModel::VOneSided);
				VSoftAeroModel.SetValue("ftwo_sided", (int)Compute::SoftAeroModel::FTwoSided);
				VSoftAeroModel.SetValue("ftwo_sided_lift_drag", (int)Compute::SoftAeroModel::FTwoSidedLiftDrag);
				VSoftAeroModel.SetValue("fone_sided", (int)Compute::SoftAeroModel::FOneSided);

				VMEnum VSoftCollision = Register.SetEnum("physics_soft_collision");
				VSoftCollision.SetValue("rvs_mask", (int)Compute::SoftCollision::RVS_Mask);
				VSoftCollision.SetValue("sdf_rs", (int)Compute::SoftCollision::SDF_RS);
				VSoftCollision.SetValue("cl_rs", (int)Compute::SoftCollision::CL_RS);
				VSoftCollision.SetValue("sdf_rd", (int)Compute::SoftCollision::SDF_RD);
				VSoftCollision.SetValue("sdf_rdf", (int)Compute::SoftCollision::SDF_RDF);
				VSoftCollision.SetValue("svs_mask", (int)Compute::SoftCollision::SVS_Mask);
				VSoftCollision.SetValue("vf_ss", (int)Compute::SoftCollision::VF_SS);
				VSoftCollision.SetValue("cl_ss", (int)Compute::SoftCollision::CL_SS);
				VSoftCollision.SetValue("cl_self", (int)Compute::SoftCollision::CL_Self);
				VSoftCollision.SetValue("vf_dd", (int)Compute::SoftCollision::VF_DD);
				VSoftCollision.SetValue("default_t", (int)Compute::SoftCollision::Default);

				VMEnum VRotator = Register.SetEnum("physics_rotator");
				VRotator.SetValue("xyz", (int)Compute::Rotator::XYZ);
				VRotator.SetValue("xzy", (int)Compute::Rotator::XZY);
				VRotator.SetValue("yxz", (int)Compute::Rotator::YXZ);
				VRotator.SetValue("yzx", (int)Compute::Rotator::YZX);
				VRotator.SetValue("zxy", (int)Compute::Rotator::ZXY);
				VRotator.SetValue("zyx", (int)Compute::Rotator::ZYX);

				VMRefClass VHullShape = Engine->Global().SetClassUnmanaged<Compute::HullShape>("physics_hull_shape");
				VHullShape.SetProperty<Compute::HullShape>("uptr@ shape", &Compute::HullShape::Shape);
				VHullShape.SetUnmanagedConstructor<Compute::HullShape>("physics_hull_shape@ f()");
				VHullShape.SetMethodEx("void set_vertices(array<vertex>@+)", &HullShapeSetVertices);
				VHullShape.SetMethodEx("void set_indices(array<int>@+)", &HullShapeSetIndices);
				VHullShape.SetMethodEx("array<vertex>@ get_vertices()", &HullShapeGetVertices);
				VHullShape.SetMethodEx("array<int>@ get_indices()", &HullShapeGetIndices);

				VMTypeClass VRigidBodyDesc = Engine->Global().SetPod<Compute::RigidBody::Desc>("physics_rigidbody_desc");
				VRigidBodyDesc.SetProperty<Compute::RigidBody::Desc>("uptr@ shape", &Compute::RigidBody::Desc::Shape);
				VRigidBodyDesc.SetProperty<Compute::RigidBody::Desc>("float anticipation", &Compute::RigidBody::Desc::Anticipation);
				VRigidBodyDesc.SetProperty<Compute::RigidBody::Desc>("float mass", &Compute::RigidBody::Desc::Mass);
				VRigidBodyDesc.SetProperty<Compute::RigidBody::Desc>("vector3 position", &Compute::RigidBody::Desc::Position);
				VRigidBodyDesc.SetProperty<Compute::RigidBody::Desc>("vector3 rotation", &Compute::RigidBody::Desc::Rotation);
				VRigidBodyDesc.SetProperty<Compute::RigidBody::Desc>("vector3 scale", &Compute::RigidBody::Desc::Scale);
				VRigidBodyDesc.SetConstructor<Compute::RigidBody::Desc>("void f()");

				VMRefClass VRigidBody = Engine->Global().SetClassUnmanaged<Compute::RigidBody>("physics_rigidbody");
				VRigidBody.SetMethod("physics_rigidbody@ copy()", &Compute::RigidBody::Copy);
				VRigidBody.SetMethod<Compute::RigidBody, void, const Compute::Vector3&>("void push(const vector3 &in)", &Compute::RigidBody::Push);
				VRigidBody.SetMethod<Compute::RigidBody, void, const Compute::Vector3&, const Compute::Vector3&>("void push(const vector3 &in, const vector3 &in)", &Compute::RigidBody::Push);
				VRigidBody.SetMethod<Compute::RigidBody, void, const Compute::Vector3&, const Compute::Vector3&, const Compute::Vector3&>("void push(const vector3 &in, const vector3 &in, const vector3 &in)", &Compute::RigidBody::Push);
				VRigidBody.SetMethod<Compute::RigidBody, void, const Compute::Vector3&>("void push_kinematic(const vector3 &in)", &Compute::RigidBody::PushKinematic);
				VRigidBody.SetMethod<Compute::RigidBody, void, const Compute::Vector3&, const Compute::Vector3&>("void push_kinematic(const vector3 &in, const vector3 &in)", &Compute::RigidBody::PushKinematic);
				VRigidBody.SetMethod("void synchronize(transform@+, bool)", &Compute::RigidBody::Synchronize);
				VRigidBody.SetMethod("void set_collision_flags(usize)", &Compute::RigidBody::SetCollisionFlags);
				VRigidBody.SetMethod("void set_activity(bool)", &Compute::RigidBody::SetActivity);
				VRigidBody.SetMethod("void set_as_ghost()", &Compute::RigidBody::SetAsGhost);
				VRigidBody.SetMethod("void set_as_normal()", &Compute::RigidBody::SetAsNormal);
				VRigidBody.SetMethod("void set_self_pointer()", &Compute::RigidBody::SetSelfPointer);
				VRigidBody.SetMethod("void set_world_transform(uptr@)", &Compute::RigidBody::SetWorldTransform);
				VRigidBody.SetMethod("void set_collision_shape(uptr@, transform@+)", &Compute::RigidBody::SetCollisionShape);
				VRigidBody.SetMethod("void set_mass(float)", &Compute::RigidBody::SetMass);
				VRigidBody.SetMethod("void set_activation_state(physics_motion_state)", &Compute::RigidBody::SetActivationState);
				VRigidBody.SetMethod("void set_angular_damping(float)", &Compute::RigidBody::SetAngularDamping);
				VRigidBody.SetMethod("void set_angular_sleeping_threshold(float)", &Compute::RigidBody::SetAngularSleepingThreshold);
				VRigidBody.SetMethod("void set_spinning_friction(float)", &Compute::RigidBody::SetSpinningFriction);
				VRigidBody.SetMethod("void set_contact_stiffness(float)", &Compute::RigidBody::SetContactStiffness);
				VRigidBody.SetMethod("void set_contact_damping(float)", &Compute::RigidBody::SetContactDamping);
				VRigidBody.SetMethod("void set_friction(float)", &Compute::RigidBody::SetFriction);
				VRigidBody.SetMethod("void set_restitution(float)", &Compute::RigidBody::SetRestitution);
				VRigidBody.SetMethod("void set_hit_fraction(float)", &Compute::RigidBody::SetHitFraction);
				VRigidBody.SetMethod("void set_linear_damping(float)", &Compute::RigidBody::SetLinearDamping);
				VRigidBody.SetMethod("void set_linear_sleeping_threshold(float)", &Compute::RigidBody::SetLinearSleepingThreshold);
				VRigidBody.SetMethod("void set_ccd_motion_threshold(float)", &Compute::RigidBody::SetCcdMotionThreshold);
				VRigidBody.SetMethod("void set_ccd_swept_sphere_radius(float)", &Compute::RigidBody::SetCcdSweptSphereRadius);
				VRigidBody.SetMethod("void set_contact_processing_threshold(float)", &Compute::RigidBody::SetContactProcessingThreshold);
				VRigidBody.SetMethod("void set_deactivation_time(float)", &Compute::RigidBody::SetDeactivationTime);
				VRigidBody.SetMethod("void set_rolling_friction(float)", &Compute::RigidBody::SetRollingFriction);
				VRigidBody.SetMethod("void set_angular_factor(const vector3 &in)", &Compute::RigidBody::SetAngularFactor);
				VRigidBody.SetMethod("void set_anisotropic_friction(const vector3 &in)", &Compute::RigidBody::SetAnisotropicFriction);
				VRigidBody.SetMethod("void set_gravity(const vector3 &in)", &Compute::RigidBody::SetGravity);
				VRigidBody.SetMethod("void set_linear_factor(const vector3 &in)", &Compute::RigidBody::SetLinearFactor);
				VRigidBody.SetMethod("void set_linear_velocity(const vector3 &in)", &Compute::RigidBody::SetLinearVelocity);
				VRigidBody.SetMethod("void set_angular_velocity(const vector3 &in)", &Compute::RigidBody::SetAngularVelocity);
				VRigidBody.SetMethod("physics_motion_state get_activation_state() const", &Compute::RigidBody::GetActivationState);
				VRigidBody.SetMethod("physics_shape get_collision_shape_type() const", &Compute::RigidBody::GetCollisionShapeType);
				VRigidBody.SetMethod("vector3 get_angular_factor() const", &Compute::RigidBody::GetAngularFactor);
				VRigidBody.SetMethod("vector3 get_anisotropic_friction() const", &Compute::RigidBody::GetAnisotropicFriction);
				VRigidBody.SetMethod("vector3 get_Gravity() const", &Compute::RigidBody::GetGravity);
				VRigidBody.SetMethod("vector3 get_linear_factor() const", &Compute::RigidBody::GetLinearFactor);
				VRigidBody.SetMethod("vector3 get_linear_velocity() const", &Compute::RigidBody::GetLinearVelocity);
				VRigidBody.SetMethod("vector3 get_angular_velocity() const", &Compute::RigidBody::GetAngularVelocity);
				VRigidBody.SetMethod("vector3 get_scale() const", &Compute::RigidBody::GetScale);
				VRigidBody.SetMethod("vector3 get_position() const", &Compute::RigidBody::GetPosition);
				VRigidBody.SetMethod("vector3 get_rotation() const", &Compute::RigidBody::GetRotation);
				VRigidBody.SetMethod("uptr@ get_world_transform() const", &Compute::RigidBody::GetWorldTransform);
				VRigidBody.SetMethod("uptr@ get_collision_shape() const", &Compute::RigidBody::GetCollisionShape);
				VRigidBody.SetMethod("uptr@ get() const", &Compute::RigidBody::Get);
				VRigidBody.SetMethod("bool is_active() const", &Compute::RigidBody::IsActive);
				VRigidBody.SetMethod("bool is_static() const", &Compute::RigidBody::IsStatic);
				VRigidBody.SetMethod("bool is_ghost() const", &Compute::RigidBody::IsGhost);
				VRigidBody.SetMethod("bool Is_colliding() const", &Compute::RigidBody::IsColliding);
				VRigidBody.SetMethod("float get_spinning_friction() const", &Compute::RigidBody::GetSpinningFriction);
				VRigidBody.SetMethod("float get_contact_stiffness() const", &Compute::RigidBody::GetContactStiffness);
				VRigidBody.SetMethod("float get_contact_damping() const", &Compute::RigidBody::GetContactDamping);
				VRigidBody.SetMethod("float get_angular_damping() const", &Compute::RigidBody::GetAngularDamping);
				VRigidBody.SetMethod("float get_angular_sleeping_threshold() const", &Compute::RigidBody::GetAngularSleepingThreshold);
				VRigidBody.SetMethod("float get_friction() const", &Compute::RigidBody::GetFriction);
				VRigidBody.SetMethod("float get_restitution() const", &Compute::RigidBody::GetRestitution);
				VRigidBody.SetMethod("float get_hit_fraction() const", &Compute::RigidBody::GetHitFraction);
				VRigidBody.SetMethod("float get_linear_damping() const", &Compute::RigidBody::GetLinearDamping);
				VRigidBody.SetMethod("float get_linear_sleeping_threshold() const", &Compute::RigidBody::GetLinearSleepingThreshold);
				VRigidBody.SetMethod("float get_ccd_motion_threshold() const", &Compute::RigidBody::GetCcdMotionThreshold);
				VRigidBody.SetMethod("float get_ccd_swept_sphere_radius() const", &Compute::RigidBody::GetCcdSweptSphereRadius);
				VRigidBody.SetMethod("float get_contact_processing_threshold() const", &Compute::RigidBody::GetContactProcessingThreshold);
				VRigidBody.SetMethod("float get_deactivation_time() const", &Compute::RigidBody::GetDeactivationTime);
				VRigidBody.SetMethod("float get_rolling_friction() const", &Compute::RigidBody::GetRollingFriction);
				VRigidBody.SetMethod("float get_mass() const", &Compute::RigidBody::GetMass);
				VRigidBody.SetMethod("usize get_collision_flags() const", &Compute::RigidBody::GetCollisionFlags);
				VRigidBody.SetMethod("physics_rigidbody_desc& get_initial_state()", &Compute::RigidBody::GetInitialState);
				VRigidBody.SetMethod("physics_simulator@+ get_simulator() const", &Compute::RigidBody::GetSimulator);

				VMTypeClass VSoftBodySConvex = Engine->Global().SetPod<Compute::SoftBody::Desc::CV::SConvex>("physics_softbody_desc_cv_sconvex");
				VSoftBodySConvex.SetProperty<Compute::SoftBody::Desc::CV::SConvex>("physics_hull_shape@ hull", &Compute::SoftBody::Desc::CV::SConvex::Hull);
				VSoftBodySConvex.SetProperty<Compute::SoftBody::Desc::CV::SConvex>("bool enabled", &Compute::SoftBody::Desc::CV::SConvex::Enabled);
				VSoftBodySConvex.SetConstructor<Compute::SoftBody::Desc::CV::SConvex>("void f()");

				VMTypeClass VSoftBodySRope = Engine->Global().SetPod<Compute::SoftBody::Desc::CV::SRope>("physics_softbody_desc_cv_srope");
				VSoftBodySRope.SetProperty<Compute::SoftBody::Desc::CV::SRope>("bool start_fixed", &Compute::SoftBody::Desc::CV::SRope::StartFixed);
				VSoftBodySRope.SetProperty<Compute::SoftBody::Desc::CV::SRope>("bool end_fixed", &Compute::SoftBody::Desc::CV::SRope::EndFixed);
				VSoftBodySRope.SetProperty<Compute::SoftBody::Desc::CV::SRope>("bool enabled", &Compute::SoftBody::Desc::CV::SRope::Enabled);
				VSoftBodySRope.SetProperty<Compute::SoftBody::Desc::CV::SRope>("int count", &Compute::SoftBody::Desc::CV::SRope::Count);
				VSoftBodySRope.SetProperty<Compute::SoftBody::Desc::CV::SRope>("vector3 start", &Compute::SoftBody::Desc::CV::SRope::Start);
				VSoftBodySRope.SetProperty<Compute::SoftBody::Desc::CV::SRope>("vector3 end", &Compute::SoftBody::Desc::CV::SRope::End);
				VSoftBodySRope.SetConstructor<Compute::SoftBody::Desc::CV::SRope>("void f()");

				VMTypeClass VSoftBodySPatch = Engine->Global().SetPod<Compute::SoftBody::Desc::CV::SPatch>("physics_softbody_desc_cv_spatch");
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("bool generate_diagonals", &Compute::SoftBody::Desc::CV::SPatch::GenerateDiagonals);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("bool corner00_fixed", &Compute::SoftBody::Desc::CV::SPatch::Corner00Fixed);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("bool corner10_fixed", &Compute::SoftBody::Desc::CV::SPatch::Corner10Fixed);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("bool corner01_fixed", &Compute::SoftBody::Desc::CV::SPatch::Corner01Fixed);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("bool corner11_fixed", &Compute::SoftBody::Desc::CV::SPatch::Corner11Fixed);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("bool enabled", &Compute::SoftBody::Desc::CV::SPatch::Enabled);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("int count_x", &Compute::SoftBody::Desc::CV::SPatch::CountX);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("int count_y", &Compute::SoftBody::Desc::CV::SPatch::CountY);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("vector3 corner00", &Compute::SoftBody::Desc::CV::SPatch::Corner00);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("vector3 corner10", &Compute::SoftBody::Desc::CV::SPatch::Corner10);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("vector3 corner01", &Compute::SoftBody::Desc::CV::SPatch::Corner01);
				VSoftBodySPatch.SetProperty<Compute::SoftBody::Desc::CV::SPatch>("vector3 corner11", &Compute::SoftBody::Desc::CV::SPatch::Corner11);
				VSoftBodySPatch.SetConstructor<Compute::SoftBody::Desc::CV::SPatch>("void f()");

				VMTypeClass VSoftBodySEllipsoid = Engine->Global().SetPod<Compute::SoftBody::Desc::CV::SEllipsoid>("physics_softbody_desc_cv_sellipsoid");
				VSoftBodySEllipsoid.SetProperty<Compute::SoftBody::Desc::CV::SEllipsoid>("vector3 center", &Compute::SoftBody::Desc::CV::SEllipsoid::Center);
				VSoftBodySEllipsoid.SetProperty<Compute::SoftBody::Desc::CV::SEllipsoid>("vector3 radius", &Compute::SoftBody::Desc::CV::SEllipsoid::Radius);
				VSoftBodySEllipsoid.SetProperty<Compute::SoftBody::Desc::CV::SEllipsoid>("int count", &Compute::SoftBody::Desc::CV::SEllipsoid::Count);
				VSoftBodySEllipsoid.SetProperty<Compute::SoftBody::Desc::CV::SEllipsoid>("bool enabled", &Compute::SoftBody::Desc::CV::SEllipsoid::Enabled);
				VSoftBodySEllipsoid.SetConstructor<Compute::SoftBody::Desc::CV::SEllipsoid>("void f()");

				VMTypeClass VSoftBodyCV = Engine->Global().SetPod<Compute::SoftBody::Desc::CV>("physics_softbody_desc_cv");
				VSoftBodyCV.SetProperty<Compute::SoftBody::Desc::CV>("physics_softbody_desc_cv_sconvex convex", &Compute::SoftBody::Desc::CV::Convex);
				VSoftBodyCV.SetProperty<Compute::SoftBody::Desc::CV>("physics_softbody_desc_cv_srope rope", &Compute::SoftBody::Desc::CV::Rope);
				VSoftBodyCV.SetProperty<Compute::SoftBody::Desc::CV>("physics_softbody_desc_cv_spatch patch", &Compute::SoftBody::Desc::CV::Patch);
				VSoftBodyCV.SetProperty<Compute::SoftBody::Desc::CV>("physics_softbody_desc_cv_sellipsoid ellipsoid", &Compute::SoftBody::Desc::CV::Ellipsoid);
				VSoftBodyCV.SetConstructor<Compute::SoftBody::Desc::CV>("void f()");

				VMTypeClass VSoftBodySConfig = Engine->Global().SetPod<Compute::SoftBody::Desc::SConfig>("physics_softbody_desc_config");
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("physics_soft_aero_model aero_model", &Compute::SoftBody::Desc::SConfig::AeroModel);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float vcf", &Compute::SoftBody::Desc::SConfig::VCF);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float dp", &Compute::SoftBody::Desc::SConfig::DP);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float dg", &Compute::SoftBody::Desc::SConfig::DG);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float lf", &Compute::SoftBody::Desc::SConfig::LF);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float pr", &Compute::SoftBody::Desc::SConfig::PR);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float vc", &Compute::SoftBody::Desc::SConfig::VC);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float df", &Compute::SoftBody::Desc::SConfig::DF);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float mt", &Compute::SoftBody::Desc::SConfig::MT);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float chr", &Compute::SoftBody::Desc::SConfig::CHR);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float khr", &Compute::SoftBody::Desc::SConfig::KHR);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float shr", &Compute::SoftBody::Desc::SConfig::SHR);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float ahr", &Compute::SoftBody::Desc::SConfig::AHR);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float srhr_cl", &Compute::SoftBody::Desc::SConfig::SRHR_CL);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float skhr_cl", &Compute::SoftBody::Desc::SConfig::SKHR_CL);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float sshr_cl", &Compute::SoftBody::Desc::SConfig::SSHR_CL);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float sr_splt_cl", &Compute::SoftBody::Desc::SConfig::SR_SPLT_CL);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float sk_splt_cl", &Compute::SoftBody::Desc::SConfig::SK_SPLT_CL);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float ss_splt_cl", &Compute::SoftBody::Desc::SConfig::SS_SPLT_CL);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float max_volume", &Compute::SoftBody::Desc::SConfig::MaxVolume);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float time_scale", &Compute::SoftBody::Desc::SConfig::TimeScale);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float drag", &Compute::SoftBody::Desc::SConfig::Drag);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("float max_stress", &Compute::SoftBody::Desc::SConfig::MaxStress);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("int clusters", &Compute::SoftBody::Desc::SConfig::Clusters);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("int constraints", &Compute::SoftBody::Desc::SConfig::Constraints);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("int viterations", &Compute::SoftBody::Desc::SConfig::VIterations);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("int Piterations", &Compute::SoftBody::Desc::SConfig::PIterations);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("int diterations", &Compute::SoftBody::Desc::SConfig::DIterations);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("int citerations", &Compute::SoftBody::Desc::SConfig::CIterations);
				VSoftBodySConfig.SetProperty<Compute::SoftBody::Desc::SConfig>("int collisions", &Compute::SoftBody::Desc::SConfig::Collisions);
				VSoftBodySConfig.SetConstructor<Compute::SoftBody::Desc::SConfig>("void f()");

				VMTypeClass VSoftBodyDesc = Engine->Global().SetPod<Compute::SoftBody::Desc>("physics_softbody_desc");
				VSoftBodyDesc.SetProperty<Compute::SoftBody::Desc>("physics_softbody_desc_cv shape", &Compute::SoftBody::Desc::Shape);
				VSoftBodyDesc.SetProperty<Compute::SoftBody::Desc>("physics_softbody_desc_config feature", &Compute::SoftBody::Desc::Config);
				VSoftBodyDesc.SetProperty<Compute::SoftBody::Desc>("float anticipation", &Compute::SoftBody::SoftBody::Desc::Anticipation);
				VSoftBodyDesc.SetProperty<Compute::SoftBody::Desc>("vector3 position", &Compute::SoftBody::SoftBody::Desc::Position);
				VSoftBodyDesc.SetProperty<Compute::SoftBody::Desc>("vector3 rotation", &Compute::SoftBody::SoftBody::Desc::Rotation);
				VSoftBodyDesc.SetProperty<Compute::SoftBody::Desc>("vector3 scale", &Compute::SoftBody::SoftBody::Desc::Scale);
				VSoftBodyDesc.SetConstructor<Compute::SoftBody::Desc>("void f()");

				VMRefClass VSoftBody = Engine->Global().SetClassUnmanaged<Compute::SoftBody>("physics_softbody");
				VMTypeClass VSoftBodyRayCast = Engine->Global().SetPod<Compute::SoftBody::RayCast>("physics_softbody_raycast");
				VSoftBodyRayCast.SetProperty<Compute::SoftBody::RayCast>("physics_softbody@ body", &Compute::SoftBody::RayCast::Body);
				VSoftBodyRayCast.SetProperty<Compute::SoftBody::RayCast>("physics_soft_feature feature", &Compute::SoftBody::RayCast::Feature);
				VSoftBodyRayCast.SetProperty<Compute::SoftBody::RayCast>("float fraction", &Compute::SoftBody::RayCast::Fraction);
				VSoftBodyRayCast.SetProperty<Compute::SoftBody::RayCast>("int32 index", &Compute::SoftBody::RayCast::Index);
				VSoftBodyRayCast.SetConstructor<Compute::SoftBody::RayCast>("void f()");

				VSoftBody.SetMethod("physics_softbody@ copy()", &Compute::SoftBody::Copy);
				VSoftBody.SetMethod("void activate(bool)", &Compute::SoftBody::Activate);
				VSoftBody.SetMethod("void synchronize(transform@+, bool)", &Compute::SoftBody::Synchronize);
				VSoftBody.SetMethodEx("array<int32>@ GetIndices() const", &SoftBodyGetIndices);
				VSoftBody.SetMethodEx("array<vertex>@ GetVertices() const", &SoftBodyGetVertices);
				VSoftBody.SetMethod("void get_bounding_box(vector3 &out, vector3 &out) const", &Compute::SoftBody::GetBoundingBox);
				VSoftBody.SetMethod("void set_contact_stiffness_and_damping(float, float)", &Compute::SoftBody::SetContactStiffnessAndDamping);
				VSoftBody.SetMethod<Compute::SoftBody, void, int, Compute::RigidBody*, bool, float>("void add_anchor(int32, physics_rigidbody@+, bool = false, float = 1)", &Compute::SoftBody::AddAnchor);
				VSoftBody.SetMethod<Compute::SoftBody, void, int, Compute::RigidBody*, const Compute::Vector3&, bool, float>("void add_anchor(int32, physics_rigidbody@+, const vector3 &in, bool = false, float = 1)", &Compute::SoftBody::AddAnchor);
				VSoftBody.SetMethod<Compute::SoftBody, void, const Compute::Vector3&>("void add_force(const vector3 &in)", &Compute::SoftBody::AddForce);
				VSoftBody.SetMethod<Compute::SoftBody, void, const Compute::Vector3&, int>("void add_force(const vector3 &in, int)", &Compute::SoftBody::AddForce);
				VSoftBody.SetMethod("void add_aero_force_to_node(const vector3 &in, int)", &Compute::SoftBody::AddAeroForceToNode);
				VSoftBody.SetMethod("void add_aero_force_to_face(const vector3 &in, int)", &Compute::SoftBody::AddAeroForceToFace);
				VSoftBody.SetMethod<Compute::SoftBody, void, const Compute::Vector3&>("void add_velocity(const vector3 &in)", &Compute::SoftBody::AddVelocity);
				VSoftBody.SetMethod<Compute::SoftBody, void, const Compute::Vector3&, int>("void add_velocity(const vector3 &in, int)", &Compute::SoftBody::AddVelocity);
				VSoftBody.SetMethod("void set_selocity(const vector3 &in)", &Compute::SoftBody::SetVelocity);
				VSoftBody.SetMethod("void set_mass(int, float)", &Compute::SoftBody::SetMass);
				VSoftBody.SetMethod("void set_total_mass(float, bool = false)", &Compute::SoftBody::SetTotalMass);
				VSoftBody.SetMethod("void set_total_density(float)", &Compute::SoftBody::SetTotalDensity);
				VSoftBody.SetMethod("void set_volume_mass(float)", &Compute::SoftBody::SetVolumeMass);
				VSoftBody.SetMethod("void set_volume_density(float)", &Compute::SoftBody::SetVolumeDensity);
				VSoftBody.SetMethod("void translate(const vector3 &in)", &Compute::SoftBody::Translate);
				VSoftBody.SetMethod("void rotate(const vector3 &in)", &Compute::SoftBody::Rotate);
				VSoftBody.SetMethod("void scale(const vector3 &in)", &Compute::SoftBody::Scale);
				VSoftBody.SetMethod("void set_rest_length_scale(float)", &Compute::SoftBody::SetRestLengthScale);
				VSoftBody.SetMethod("void set_pose(bool, bool)", &Compute::SoftBody::SetPose);
				VSoftBody.SetMethod("float get_mass(int) const", &Compute::SoftBody::GetMass);
				VSoftBody.SetMethod("float get_total_mass() const", &Compute::SoftBody::GetTotalMass);
				VSoftBody.SetMethod("float get_rest_length_scale() const", &Compute::SoftBody::GetRestLengthScale);
				VSoftBody.SetMethod("float get_volume() const", &Compute::SoftBody::GetVolume);
				VSoftBody.SetMethod("int generate_bending_constraints(int)", &Compute::SoftBody::GenerateBendingConstraints);
				VSoftBody.SetMethod("void randomize_constraints()", &Compute::SoftBody::RandomizeConstraints);
				VSoftBody.SetMethod("bool cut_link(int, int, float)", &Compute::SoftBody::CutLink);
				VSoftBody.SetMethod("bool ray_test(const vector3 &in, const vector3 &in, physics_softbody_raycast &out)", &Compute::SoftBody::RayTest);
				VSoftBody.SetMethod("void set_wind_velocity(const vector3 &in)", &Compute::SoftBody::SetWindVelocity);
				VSoftBody.SetMethod("vector3 get_wind_velocity() const", &Compute::SoftBody::GetWindVelocity);
				VSoftBody.SetMethod("void get_aabb(vector3 &out, vector3 &out) const", &Compute::SoftBody::GetAabb);
				VSoftBody.SetMethod("void set_spinning_friction(float)", &Compute::SoftBody::SetSpinningFriction);
				VSoftBody.SetMethod("vector3 get_linear_velocity() const", &Compute::SoftBody::GetLinearVelocity);
				VSoftBody.SetMethod("vector3 get_angular_velocity() const", &Compute::SoftBody::GetAngularVelocity);
				VSoftBody.SetMethod("vector3 get_center_position() const", &Compute::SoftBody::GetCenterPosition);
				VSoftBody.SetMethod("void set_activity(bool)", &Compute::SoftBody::SetActivity);
				VSoftBody.SetMethod("void set_as_ghost()", &Compute::SoftBody::SetAsGhost);
				VSoftBody.SetMethod("void set_as_normal()", &Compute::SoftBody::SetAsNormal);
				VSoftBody.SetMethod("void set_self_pointer()", &Compute::SoftBody::SetSelfPointer);
				VSoftBody.SetMethod("void set_world_transform(uptr@)", &Compute::SoftBody::SetWorldTransform);
				VSoftBody.SetMethod("void set_activation_state(physics_motion_state)", &Compute::SoftBody::SetActivationState);
				VSoftBody.SetMethod("void set_contact_stiffness(float)", &Compute::SoftBody::SetContactStiffness);
				VSoftBody.SetMethod("void set_contact_damping(float)", &Compute::SoftBody::SetContactDamping);
				VSoftBody.SetMethod("void set_friction(float)", &Compute::SoftBody::SetFriction);
				VSoftBody.SetMethod("void set_restitution(float)", &Compute::SoftBody::SetRestitution);
				VSoftBody.SetMethod("void set_hit_fraction(float)", &Compute::SoftBody::SetHitFraction);
				VSoftBody.SetMethod("void set_ccd_motion_threshold(float)", &Compute::SoftBody::SetCcdMotionThreshold);
				VSoftBody.SetMethod("void set_ccd_swept_sphere_radius(float)", &Compute::SoftBody::SetCcdSweptSphereRadius);
				VSoftBody.SetMethod("void set_contact_processing_threshold(float)", &Compute::SoftBody::SetContactProcessingThreshold);
				VSoftBody.SetMethod("void set_reactivation_time(float)", &Compute::SoftBody::SetDeactivationTime);
				VSoftBody.SetMethod("void set_rolling_friction(float)", &Compute::SoftBody::SetRollingFriction);
				VSoftBody.SetMethod("void set_anisotropic_friction(const vector3 &in)", &Compute::SoftBody::SetAnisotropicFriction);
				VSoftBody.SetMethod("void set_config(const physics_softbody_desc_config &in)", &Compute::SoftBody::SetConfig);
				VSoftBody.SetMethod("physics_shape get_collision_shape_type() const", &Compute::SoftBody::GetCollisionShapeType);
				VSoftBody.SetMethod("physics_motion_state get_activation_state() const", &Compute::SoftBody::GetActivationState);
				VSoftBody.SetMethod("vector3 get_anisotropic_friction() const", &Compute::SoftBody::GetAnisotropicFriction);
				VSoftBody.SetMethod("vector3 get_scale() const", &Compute::SoftBody::GetScale);
				VSoftBody.SetMethod("vector3 get_position() const", &Compute::SoftBody::GetPosition);
				VSoftBody.SetMethod("vector3 get_rotation() const", &Compute::SoftBody::GetRotation);
				VSoftBody.SetMethod("uptr@ get_world_transform() const", &Compute::SoftBody::GetWorldTransform);
				VSoftBody.SetMethod("uptr@ get() const", &Compute::SoftBody::Get);
				VSoftBody.SetMethod("bool is_active() const", &Compute::SoftBody::IsActive);
				VSoftBody.SetMethod("bool is_static() const", &Compute::SoftBody::IsStatic);
				VSoftBody.SetMethod("bool is_ghost() const", &Compute::SoftBody::IsGhost);
				VSoftBody.SetMethod("bool is_colliding() const", &Compute::SoftBody::IsColliding);
				VSoftBody.SetMethod("float get_spinning_friction() const", &Compute::SoftBody::GetSpinningFriction);
				VSoftBody.SetMethod("float get_contact_stiffness() const", &Compute::SoftBody::GetContactStiffness);
				VSoftBody.SetMethod("float get_contact_damping() const", &Compute::SoftBody::GetContactDamping);
				VSoftBody.SetMethod("float get_friction() const", &Compute::SoftBody::GetFriction);
				VSoftBody.SetMethod("float get_restitution() const", &Compute::SoftBody::GetRestitution);
				VSoftBody.SetMethod("float get_hit_fraction() const", &Compute::SoftBody::GetHitFraction);
				VSoftBody.SetMethod("float get_ccd_motion_threshold() const", &Compute::SoftBody::GetCcdMotionThreshold);
				VSoftBody.SetMethod("float get_ccd_swept_sphere_radius() const", &Compute::SoftBody::GetCcdSweptSphereRadius);
				VSoftBody.SetMethod("float get_contact_processing_threshold() const", &Compute::SoftBody::GetContactProcessingThreshold);
				VSoftBody.SetMethod("float get_deactivation_time() const", &Compute::SoftBody::GetDeactivationTime);
				VSoftBody.SetMethod("float get_rolling_friction() const", &Compute::SoftBody::GetRollingFriction);
				VSoftBody.SetMethod("usize get_collision_flags() const", &Compute::SoftBody::GetCollisionFlags);
				VSoftBody.SetMethod("usize get_vertices_count() const", &Compute::SoftBody::GetVerticesCount);
				VSoftBody.SetMethod("physics_softbody_desc& get_initial_state()", &Compute::SoftBody::GetInitialState);
				VSoftBody.SetMethod("physics_simulator@+ get_simulator() const", &Compute::SoftBody::GetSimulator);

				VMRefClass VConstraint = Engine->Global().SetClassUnmanaged<Compute::Constraint>("physics_constraint");
				VConstraint.SetMethod("physics_constraint@ copy() const", &Compute::Constraint::Copy);
				VConstraint.SetMethod("physics_simulator@+ get_simulator() const", &Compute::Constraint::GetSimulator);
				VConstraint.SetMethod("uptr@ get() const", &Compute::Constraint::Get);
				VConstraint.SetMethod("uptr@ get_first() const", &Compute::Constraint::GetFirst);
				VConstraint.SetMethod("uptr@ get_second() const", &Compute::Constraint::GetSecond);
				VConstraint.SetMethod("bool has_collisions() const", &Compute::Constraint::HasCollisions);
				VConstraint.SetMethod("bool is_enabled() const", &Compute::Constraint::IsEnabled);
				VConstraint.SetMethod("bool is_active() const", &Compute::Constraint::IsActive);
				VConstraint.SetMethod("void set_breaking_impulse_threshold(float)", &Compute::Constraint::SetBreakingImpulseThreshold);
				VConstraint.SetMethod("void set_enabled(bool)", &Compute::Constraint::SetEnabled);
				VConstraint.SetMethod("float get_breaking_impulse_threshold() const", &Compute::Constraint::GetBreakingImpulseThreshold);

				VMTypeClass VPConstraintDesc = Engine->Global().SetPod<Compute::PConstraint::Desc>("physics_pconstraint_desc");
				VPConstraintDesc.SetProperty<Compute::PConstraint::Desc>("physics_rigidbody@ target_a", &Compute::PConstraint::Desc::TargetA);
				VPConstraintDesc.SetProperty<Compute::PConstraint::Desc>("physics_rigidbody@ target_b", &Compute::PConstraint::Desc::TargetB);
				VPConstraintDesc.SetProperty<Compute::PConstraint::Desc>("vector3 pivot_a", &Compute::PConstraint::Desc::PivotA);
				VPConstraintDesc.SetProperty<Compute::PConstraint::Desc>("vector3 pivot_b", &Compute::PConstraint::Desc::PivotB);
				VPConstraintDesc.SetProperty<Compute::PConstraint::Desc>("bool collisions", &Compute::PConstraint::Desc::Collisions);
				VPConstraintDesc.SetConstructor<Compute::PConstraint::Desc>("void f()");

				VMRefClass VPConstraint = Engine->Global().SetClassUnmanaged<Compute::PConstraint>("physics_pconstraint");
				VPConstraint.SetMethod("physics_pconstraint@ copy() const", &Compute::PConstraint::Copy);
				VPConstraint.SetMethod("physics_simulator@+ get_simulator() const", &Compute::PConstraint::GetSimulator);
				VPConstraint.SetMethod("uptr@ get() const", &Compute::PConstraint::Get);
				VPConstraint.SetMethod("uptr@ get_first() const", &Compute::PConstraint::GetFirst);
				VPConstraint.SetMethod("uptr@ get_second() const", &Compute::PConstraint::GetSecond);
				VPConstraint.SetMethod("bool has_collisions() const", &Compute::PConstraint::HasCollisions);
				VPConstraint.SetMethod("bool is_enabled() const", &Compute::PConstraint::IsEnabled);
				VPConstraint.SetMethod("bool is_active() const", &Compute::PConstraint::IsActive);
				VPConstraint.SetMethod("void set_breaking_impulse_threshold(float)", &Compute::PConstraint::SetBreakingImpulseThreshold);
				VPConstraint.SetMethod("void set_enabled(bool)", &Compute::PConstraint::SetEnabled);
				VPConstraint.SetMethod("float get_breaking_impulse_threshold() const", &Compute::PConstraint::GetBreakingImpulseThreshold);
				VPConstraint.SetMethod("void set_pivot_a(const vector3 &in)", &Compute::PConstraint::SetPivotA);
				VPConstraint.SetMethod("void set_pivot_b(const vector3 &in)", &Compute::PConstraint::SetPivotB);
				VPConstraint.SetMethod("vector3 get_pivot_a() const", &Compute::PConstraint::GetPivotA);
				VPConstraint.SetMethod("vector3 get_pivot_b() const", &Compute::PConstraint::GetPivotB);
				VPConstraint.SetMethod("physics_pconstraint_desc get_state() const", &Compute::PConstraint::GetState);

				VMTypeClass VHConstraintDesc = Engine->Global().SetPod<Compute::HConstraint::Desc>("physics_hconstraint_desc");
				VHConstraintDesc.SetProperty<Compute::HConstraint::Desc>("physics_rigidbody@ target_a", &Compute::HConstraint::Desc::TargetA);
				VHConstraintDesc.SetProperty<Compute::HConstraint::Desc>("physics_rigidbody@ target_b", &Compute::HConstraint::Desc::TargetB);
				VHConstraintDesc.SetProperty<Compute::HConstraint::Desc>("bool references", &Compute::HConstraint::Desc::References);
				VHConstraintDesc.SetProperty<Compute::HConstraint::Desc>("bool collisions", &Compute::HConstraint::Desc::Collisions);
				VPConstraintDesc.SetConstructor<Compute::HConstraint::Desc>("void f()");

				VMRefClass VHConstraint = Engine->Global().SetClassUnmanaged<Compute::HConstraint>("physics_hconstraint");
				VHConstraint.SetMethod("physics_hconstraint@ copy() const", &Compute::HConstraint::Copy);
				VHConstraint.SetMethod("physics_simulator@+ get_simulator() const", &Compute::HConstraint::GetSimulator);
				VHConstraint.SetMethod("uptr@ get() const", &Compute::HConstraint::Get);
				VHConstraint.SetMethod("uptr@ get_first() const", &Compute::HConstraint::GetFirst);
				VHConstraint.SetMethod("uptr@ get_second() const", &Compute::HConstraint::GetSecond);
				VHConstraint.SetMethod("bool has_collisions() const", &Compute::HConstraint::HasCollisions);
				VHConstraint.SetMethod("bool is_enabled() const", &Compute::HConstraint::IsEnabled);
				VHConstraint.SetMethod("bool is_active() const", &Compute::HConstraint::IsActive);
				VHConstraint.SetMethod("void set_breaking_impulse_threshold(float)", &Compute::HConstraint::SetBreakingImpulseThreshold);
				VHConstraint.SetMethod("void set_enabled(bool)", &Compute::HConstraint::SetEnabled);
				VHConstraint.SetMethod("float get_breaking_impulse_threshold() const", &Compute::HConstraint::GetBreakingImpulseThreshold);
				VHConstraint.SetMethod("void enable_angular_motor(bool, float, float)", &Compute::HConstraint::EnableAngularMotor);
				VHConstraint.SetMethod("void enable_motor(bool)", &Compute::HConstraint::EnableMotor);
				VHConstraint.SetMethod("void test_limit(const matrix4x4 &in, const matrix4x4 &in)", &Compute::HConstraint::TestLimit);
				VHConstraint.SetMethod("void set_frames(const matrix4x4 &in, const matrix4x4 &in)", &Compute::HConstraint::SetFrames);
				VHConstraint.SetMethod("void set_angular_only(bool)", &Compute::HConstraint::SetAngularOnly);
				VHConstraint.SetMethod("void set_max_motor_impulse(float)", &Compute::HConstraint::SetMaxMotorImpulse);
				VHConstraint.SetMethod("void set_motor_target_velocity(float)", &Compute::HConstraint::SetMotorTargetVelocity);
				VHConstraint.SetMethod("void set_motor_target(float, float)", &Compute::HConstraint::SetMotorTarget);
				VHConstraint.SetMethod("void set_limit(float Low, float High, float Softness = 0.9f, float BiasFactor = 0.3f, float RelaxationFactor = 1.0f)", &Compute::HConstraint::SetLimit);
				VHConstraint.SetMethod("void set_offset(bool Value)", &Compute::HConstraint::SetOffset);
				VHConstraint.SetMethod("void set_reference_to_a(bool Value)", &Compute::HConstraint::SetReferenceToA);
				VHConstraint.SetMethod("void set_axis(const vector3 &in)", &Compute::HConstraint::SetAxis);
				VHConstraint.SetMethod("int get_solve_limit() const", &Compute::HConstraint::GetSolveLimit);
				VHConstraint.SetMethod("float get_motor_target_velocity() const", &Compute::HConstraint::GetMotorTargetVelocity);
				VHConstraint.SetMethod("float get_max_motor_impulse() const", &Compute::HConstraint::GetMaxMotorImpulse);
				VHConstraint.SetMethod("float get_limit_sign() const", &Compute::HConstraint::GetLimitSign);
				VHConstraint.SetMethod<Compute::HConstraint, float>("float get_hinge_angle() const", &Compute::HConstraint::GetHingeAngle);
				VHConstraint.SetMethod<Compute::HConstraint, float, const Compute::Matrix4x4&, const Compute::Matrix4x4&>("float get_hinge_angle(const matrix4x4 &in, const matrix4x4 &in) const", &Compute::HConstraint::GetHingeAngle);
				VHConstraint.SetMethod("float get_lower_limit() const", &Compute::HConstraint::GetLowerLimit);
				VHConstraint.SetMethod("float get_upper_limit() const", &Compute::HConstraint::GetUpperLimit);
				VHConstraint.SetMethod("float get_limit_softness() const", &Compute::HConstraint::GetLimitSoftness);
				VHConstraint.SetMethod("float get_limit_bias_factor() const", &Compute::HConstraint::GetLimitBiasFactor);
				VHConstraint.SetMethod("float get_limit_relaxation_factor() const", &Compute::HConstraint::GetLimitRelaxationFactor);
				VHConstraint.SetMethod("bool has_limit() const", &Compute::HConstraint::HasLimit);
				VHConstraint.SetMethod("bool is_offset() const", &Compute::HConstraint::IsOffset);
				VHConstraint.SetMethod("bool is_reference_to_a() const", &Compute::HConstraint::IsReferenceToA);
				VHConstraint.SetMethod("bool is_angular_only() const", &Compute::HConstraint::IsAngularOnly);
				VHConstraint.SetMethod("bool is_angular_motor_enabled() const", &Compute::HConstraint::IsAngularMotorEnabled);
				VHConstraint.SetMethod("physics_hconstraint_desc& get_state()", &Compute::HConstraint::GetState);

				VMTypeClass VSConstraintDesc = Engine->Global().SetPod<Compute::SConstraint::Desc>("physics_sconstraint_desc");
				VSConstraintDesc.SetProperty<Compute::SConstraint::Desc>("physics_rigidbody@ target_a", &Compute::SConstraint::Desc::TargetA);
				VSConstraintDesc.SetProperty<Compute::SConstraint::Desc>("physics_rigidbody@ target_b", &Compute::SConstraint::Desc::TargetB);
				VSConstraintDesc.SetProperty<Compute::SConstraint::Desc>("bool linear", &Compute::SConstraint::Desc::Linear);
				VSConstraintDesc.SetProperty<Compute::SConstraint::Desc>("bool collisions", &Compute::SConstraint::Desc::Collisions);
				VSConstraintDesc.SetConstructor<Compute::SConstraint::Desc>("void f()");

				VMRefClass VSConstraint = Engine->Global().SetClassUnmanaged<Compute::SConstraint>("physics_sconstraint");
				VSConstraint.SetMethod("physics_sconstraint@ copy() const", &Compute::SConstraint::Copy);
				VSConstraint.SetMethod("physics_simulator@+ get_simulator() const", &Compute::SConstraint::GetSimulator);
				VSConstraint.SetMethod("uptr@ get() const", &Compute::SConstraint::Get);
				VSConstraint.SetMethod("uptr@ get_first() const", &Compute::SConstraint::GetFirst);
				VSConstraint.SetMethod("uptr@ get_second() const", &Compute::SConstraint::GetSecond);
				VSConstraint.SetMethod("bool has_collisions() const", &Compute::SConstraint::HasCollisions);
				VSConstraint.SetMethod("bool is_enabled() const", &Compute::SConstraint::IsEnabled);
				VSConstraint.SetMethod("bool is_active() const", &Compute::SConstraint::IsActive);
				VSConstraint.SetMethod("void set_breaking_impulse_threshold(float)", &Compute::SConstraint::SetBreakingImpulseThreshold);
				VSConstraint.SetMethod("void set_enabled(bool)", &Compute::SConstraint::SetEnabled);
				VSConstraint.SetMethod("float get_breaking_impulse_threshold() const", &Compute::SConstraint::GetBreakingImpulseThreshold);
				VSConstraint.SetMethod("void set_angular_motor_velocity(float)", &Compute::SConstraint::SetAngularMotorVelocity);
				VSConstraint.SetMethod("void set_linear_motor_velocity(float)", &Compute::SConstraint::SetLinearMotorVelocity);
				VSConstraint.SetMethod("void set_upper_linear_limit(float)", &Compute::SConstraint::SetUpperLinearLimit);
				VSConstraint.SetMethod("void set_lower_linear_limit(float)", &Compute::SConstraint::SetLowerLinearLimit);
				VSConstraint.SetMethod("void set_angular_damping_direction(float)", &Compute::SConstraint::SetAngularDampingDirection);
				VSConstraint.SetMethod("void set_linear_damping_direction(float)", &Compute::SConstraint::SetLinearDampingDirection);
				VSConstraint.SetMethod("void set_angular_damping_limit(float)", &Compute::SConstraint::SetAngularDampingLimit);
				VSConstraint.SetMethod("void set_linear_damping_limit(float)", &Compute::SConstraint::SetLinearDampingLimit);
				VSConstraint.SetMethod("void set_angular_damping_ortho(float)", &Compute::SConstraint::SetAngularDampingOrtho);
				VSConstraint.SetMethod("void set_linear_damping_ortho(float)", &Compute::SConstraint::SetLinearDampingOrtho);
				VSConstraint.SetMethod("void set_upper_angular_limit(float)", &Compute::SConstraint::SetUpperAngularLimit);
				VSConstraint.SetMethod("void set_lower_angular_limit(float)", &Compute::SConstraint::SetLowerAngularLimit);
				VSConstraint.SetMethod("void set_max_angular_motor_force(float)", &Compute::SConstraint::SetMaxAngularMotorForce);
				VSConstraint.SetMethod("void set_max_linear_motor_force(float)", &Compute::SConstraint::SetMaxLinearMotorForce);
				VSConstraint.SetMethod("void set_angular_restitution_direction(float)", &Compute::SConstraint::SetAngularRestitutionDirection);
				VSConstraint.SetMethod("void set_linear_restitution_direction(float)", &Compute::SConstraint::SetLinearRestitutionDirection);
				VSConstraint.SetMethod("void set_angular_restitution_limit(float)", &Compute::SConstraint::SetAngularRestitutionLimit);
				VSConstraint.SetMethod("void set_linear_restitution_limit(float)", &Compute::SConstraint::SetLinearRestitutionLimit);
				VSConstraint.SetMethod("void set_angular_restitution_ortho(float)", &Compute::SConstraint::SetAngularRestitutionOrtho);
				VSConstraint.SetMethod("void set_linear_restitution_ortho(float)", &Compute::SConstraint::SetLinearRestitutionOrtho);
				VSConstraint.SetMethod("void set_angular_softness_direction(float)", &Compute::SConstraint::SetAngularSoftnessDirection);
				VSConstraint.SetMethod("void SetLinearSoftness_direction(float)", &Compute::SConstraint::SetLinearSoftnessDirection);
				VSConstraint.SetMethod("void Set_angular_softness_limit(float)", &Compute::SConstraint::SetAngularSoftnessLimit);
				VSConstraint.SetMethod("void Set_linear_softness_limit(float)", &Compute::SConstraint::SetLinearSoftnessLimit);
				VSConstraint.SetMethod("void Set_angular_softness_ortho(float)", &Compute::SConstraint::SetAngularSoftnessOrtho);
				VSConstraint.SetMethod("void set_linear_softness_ortho(float)", &Compute::SConstraint::SetLinearSoftnessOrtho);
				VSConstraint.SetMethod("void set_powered_angular_motor(bool)", &Compute::SConstraint::SetPoweredAngularMotor);
				VSConstraint.SetMethod("void set_powered_linear_motor(bool)", &Compute::SConstraint::SetPoweredLinearMotor);
				VSConstraint.SetMethod("float getAngularMotorVelocity() const", &Compute::SConstraint::GetAngularMotorVelocity);
				VSConstraint.SetMethod("float get_linear_motor_velocity() const", &Compute::SConstraint::GetLinearMotorVelocity);
				VSConstraint.SetMethod("float get_upper_linear_limit() const", &Compute::SConstraint::GetUpperLinearLimit);
				VSConstraint.SetMethod("float get_lower_linear_limit() const", &Compute::SConstraint::GetLowerLinearLimit);
				VSConstraint.SetMethod("float get_angular_damping_direction() const", &Compute::SConstraint::GetAngularDampingDirection);
				VSConstraint.SetMethod("float get_linear_damping_direction() const", &Compute::SConstraint::GetLinearDampingDirection);
				VSConstraint.SetMethod("float get_angular_damping_limit() const", &Compute::SConstraint::GetAngularDampingLimit);
				VSConstraint.SetMethod("float get_linear_damping_limit() const", &Compute::SConstraint::GetLinearDampingLimit);
				VSConstraint.SetMethod("float get_angular_damping_ortho() const", &Compute::SConstraint::GetAngularDampingOrtho);
				VSConstraint.SetMethod("float get_linear_damping_ortho() const", &Compute::SConstraint::GetLinearDampingOrtho);
				VSConstraint.SetMethod("float get_upper_angular_limit() const", &Compute::SConstraint::GetUpperAngularLimit);
				VSConstraint.SetMethod("float get_lower_angular_limit() const", &Compute::SConstraint::GetLowerAngularLimit);
				VSConstraint.SetMethod("float get_max_angular_motor_force() const", &Compute::SConstraint::GetMaxAngularMotorForce);
				VSConstraint.SetMethod("float get_max_linear_motor_force() const", &Compute::SConstraint::GetMaxLinearMotorForce);
				VSConstraint.SetMethod("float get_angular_restitution_direction() const", &Compute::SConstraint::GetAngularRestitutionDirection);
				VSConstraint.SetMethod("float get_linear_restitution_direction() const", &Compute::SConstraint::GetLinearRestitutionDirection);
				VSConstraint.SetMethod("float get_angular_restitution_limit() const", &Compute::SConstraint::GetAngularRestitutionLimit);
				VSConstraint.SetMethod("float get_linear_restitution_limit() const", &Compute::SConstraint::GetLinearRestitutionLimit);
				VSConstraint.SetMethod("float get_angular_restitution_ortho() const", &Compute::SConstraint::GetAngularRestitutionOrtho);
				VSConstraint.SetMethod("float get_linearRestitution_ortho() const", &Compute::SConstraint::GetLinearRestitutionOrtho);
				VSConstraint.SetMethod("float get_angular_softness_direction() const", &Compute::SConstraint::GetAngularSoftnessDirection);
				VSConstraint.SetMethod("float get_linear_softness_direction() const", &Compute::SConstraint::GetLinearSoftnessDirection);
				VSConstraint.SetMethod("float get_angular_softness_limit() const", &Compute::SConstraint::GetAngularSoftnessLimit);
				VSConstraint.SetMethod("float get_linear_softness_limit() const", &Compute::SConstraint::GetLinearSoftnessLimit);
				VSConstraint.SetMethod("float get_angular_softness_ortho() const", &Compute::SConstraint::GetAngularSoftnessOrtho);
				VSConstraint.SetMethod("float get_linear_softness_ortho() const", &Compute::SConstraint::GetLinearSoftnessOrtho);
				VSConstraint.SetMethod("bool get_powered_angular_motor() const", &Compute::SConstraint::GetPoweredAngularMotor);
				VSConstraint.SetMethod("bool get_powered_linear_motor() const", &Compute::SConstraint::GetPoweredLinearMotor);
				VSConstraint.SetMethod("physics_sconstraint_desc& get_state()", &Compute::SConstraint::GetState);

				VMTypeClass VCTConstraintDesc = Engine->Global().SetPod<Compute::CTConstraint::Desc>("physics_ctconstraint_desc");
				VCTConstraintDesc.SetProperty<Compute::CTConstraint::Desc>("physics_rigidbody@ target_a", &Compute::CTConstraint::Desc::TargetA);
				VCTConstraintDesc.SetProperty<Compute::CTConstraint::Desc>("physics_rigidbody@ target_b", &Compute::CTConstraint::Desc::TargetB);
				VCTConstraintDesc.SetProperty<Compute::CTConstraint::Desc>("bool collisions", &Compute::CTConstraint::Desc::Collisions);
				VCTConstraintDesc.SetConstructor<Compute::CTConstraint::Desc>("void f()");

				VMRefClass VCTConstraint = Engine->Global().SetClassUnmanaged<Compute::CTConstraint>("physics_ctconstraint");
				VCTConstraint.SetMethod("physics_ctconstraint@ copy() const", &Compute::CTConstraint::Copy);
				VCTConstraint.SetMethod("physics_simulator@+ get_simulator() const", &Compute::CTConstraint::GetSimulator);
				VCTConstraint.SetMethod("uptr@ get() const", &Compute::CTConstraint::Get);
				VCTConstraint.SetMethod("uptr@ get_first() const", &Compute::CTConstraint::GetFirst);
				VCTConstraint.SetMethod("uptr@ get_second() const", &Compute::CTConstraint::GetSecond);
				VCTConstraint.SetMethod("bool has_collisions() const", &Compute::CTConstraint::HasCollisions);
				VCTConstraint.SetMethod("bool is_enabled() const", &Compute::CTConstraint::IsEnabled);
				VCTConstraint.SetMethod("bool is_active() const", &Compute::CTConstraint::IsActive);
				VCTConstraint.SetMethod("void set_breaking_impulse_threshold(float)", &Compute::CTConstraint::SetBreakingImpulseThreshold);
				VCTConstraint.SetMethod("void set_enabled(bool)", &Compute::CTConstraint::SetEnabled);
				VCTConstraint.SetMethod("float get_breaking_impulse_threshold() const", &Compute::CTConstraint::GetBreakingImpulseThreshold);
				VCTConstraint.SetMethod("void enable_motor(bool)", &Compute::CTConstraint::EnableMotor);
				VCTConstraint.SetMethod("void set_frames(const matrix4x4 &in, const matrix4x4 &in)", &Compute::CTConstraint::SetFrames);
				VCTConstraint.SetMethod("void set_angular_only(bool)", &Compute::CTConstraint::SetAngularOnly);
				VCTConstraint.SetMethod<Compute::CTConstraint, void, int, float>("void set_limit(int, float)", &Compute::CTConstraint::SetLimit);
				VCTConstraint.SetMethod<Compute::CTConstraint, void, float, float, float, float, float, float>("void set_limit(float, float, float, float = 1.f, float = 0.3f, float = 1.0f)", &Compute::CTConstraint::SetLimit);
				VCTConstraint.SetMethod("void set_damping(float)", &Compute::CTConstraint::SetDamping);
				VCTConstraint.SetMethod("void set_max_motor_impulse(float)", &Compute::CTConstraint::SetMaxMotorImpulse);
				VCTConstraint.SetMethod("void set_max_motor_impulse_normalized(float)", &Compute::CTConstraint::SetMaxMotorImpulseNormalized);
				VCTConstraint.SetMethod("void set_fix_thresh(float)", &Compute::CTConstraint::SetFixThresh);
				VCTConstraint.SetMethod("void set_motor_target(const quaternion &in)", &Compute::CTConstraint::SetMotorTarget);
				VCTConstraint.SetMethod("void set_motor_target_in_constraint_space(const quaternion &in)", &Compute::CTConstraint::SetMotorTargetInConstraintSpace);
				VCTConstraint.SetMethod("vector3 get_point_for_angle(float, float) const", &Compute::CTConstraint::GetPointForAngle);
				VCTConstraint.SetMethod("quaternion get_motor_target() const", &Compute::CTConstraint::GetMotorTarget);
				VCTConstraint.SetMethod("int get_solve_twist_limit() const", &Compute::CTConstraint::GetSolveTwistLimit);
				VCTConstraint.SetMethod("int get_solve_swing_limit() const", &Compute::CTConstraint::GetSolveSwingLimit);
				VCTConstraint.SetMethod("float get_twist_limit_sign() const", &Compute::CTConstraint::GetTwistLimitSign);
				VCTConstraint.SetMethod("float get_swing_span1() const", &Compute::CTConstraint::GetSwingSpan1);
				VCTConstraint.SetMethod("float get_swing_span2() const", &Compute::CTConstraint::GetSwingSpan2);
				VCTConstraint.SetMethod("float get_twist_span() const", &Compute::CTConstraint::GetTwistSpan);
				VCTConstraint.SetMethod("float get_limit_softness() const", &Compute::CTConstraint::GetLimitSoftness);
				VCTConstraint.SetMethod("float get_bias_factor() const", &Compute::CTConstraint::GetBiasFactor);
				VCTConstraint.SetMethod("float get_relaxation_factor() const", &Compute::CTConstraint::GetRelaxationFactor);
				VCTConstraint.SetMethod("float get_twist_angle() const", &Compute::CTConstraint::GetTwistAngle);
				VCTConstraint.SetMethod("float get_limit(int) const", &Compute::CTConstraint::GetLimit);
				VCTConstraint.SetMethod("float get_damping() const", &Compute::CTConstraint::GetDamping);
				VCTConstraint.SetMethod("float get_max_motor_impulse() const", &Compute::CTConstraint::GetMaxMotorImpulse);
				VCTConstraint.SetMethod("float get_fix_thresh() const", &Compute::CTConstraint::GetFixThresh);
				VCTConstraint.SetMethod("bool is_motor_enabled() const", &Compute::CTConstraint::IsMotorEnabled);
				VCTConstraint.SetMethod("bool is_max_motor_impulse_normalized() const", &Compute::CTConstraint::IsMaxMotorImpulseNormalized);
				VCTConstraint.SetMethod("bool is_past_swing_limit() const", &Compute::CTConstraint::IsPastSwingLimit);
				VCTConstraint.SetMethod("bool is_angular_only() const", &Compute::CTConstraint::IsAngularOnly);
				VCTConstraint.SetMethod("physics_ctconstraint_desc& get_state()", &Compute::CTConstraint::GetState);

				VMTypeClass VDF6ConstraintDesc = Engine->Global().SetPod<Compute::DF6Constraint::Desc>("physics_df6constraint_desc");
				VDF6ConstraintDesc.SetProperty<Compute::DF6Constraint::Desc>("physics_rigidbody@ target_a", &Compute::DF6Constraint::Desc::TargetA);
				VDF6ConstraintDesc.SetProperty<Compute::DF6Constraint::Desc>("physics_rigidbody@ target_b", &Compute::DF6Constraint::Desc::TargetB);
				VDF6ConstraintDesc.SetProperty<Compute::DF6Constraint::Desc>("bool collisions", &Compute::DF6Constraint::Desc::Collisions);
				VDF6ConstraintDesc.SetConstructor<Compute::DF6Constraint::Desc>("void f()");

				VMRefClass VDF6Constraint = Engine->Global().SetClassUnmanaged<Compute::DF6Constraint>("physics_df6constraint");
				VDF6Constraint.SetMethod("physics_df6constraint@ copy() const", &Compute::DF6Constraint::Copy);
				VDF6Constraint.SetMethod("physics_simulator@+ get_simulator() const", &Compute::DF6Constraint::GetSimulator);
				VDF6Constraint.SetMethod("uptr@ get() const", &Compute::DF6Constraint::Get);
				VDF6Constraint.SetMethod("uptr@ get_first() const", &Compute::DF6Constraint::GetFirst);
				VDF6Constraint.SetMethod("uptr@ get_second() const", &Compute::DF6Constraint::GetSecond);
				VDF6Constraint.SetMethod("bool has_collisions() const", &Compute::DF6Constraint::HasCollisions);
				VDF6Constraint.SetMethod("bool is_enabled() const", &Compute::DF6Constraint::IsEnabled);
				VDF6Constraint.SetMethod("bool is_active() const", &Compute::DF6Constraint::IsActive);
				VDF6Constraint.SetMethod("void set_breaking_impulse_threshold(float)", &Compute::DF6Constraint::SetBreakingImpulseThreshold);
				VDF6Constraint.SetMethod("void set_enabled(bool)", &Compute::DF6Constraint::SetEnabled);
				VDF6Constraint.SetMethod("float get_breaking_impulse_threshold() const", &Compute::DF6Constraint::GetBreakingImpulseThreshold);		
				VDF6Constraint.SetMethod("void enable_motor(int, bool)", &Compute::DF6Constraint::EnableMotor);
				VDF6Constraint.SetMethod("void enable_spring(int, bool)", &Compute::DF6Constraint::EnableSpring);
				VDF6Constraint.SetMethod("void set_frames(const matrix4x4 &in, const matrix4x4 &in)", &Compute::DF6Constraint::SetFrames);
				VDF6Constraint.SetMethod("void set_linear_lower_limit(const vector3 &in)", &Compute::DF6Constraint::SetLinearLowerLimit);
				VDF6Constraint.SetMethod("void set_linear_upper_limit(const vector3 &in)", &Compute::DF6Constraint::SetLinearUpperLimit);
				VDF6Constraint.SetMethod("void set_angular_lower_limit(const vector3 &in)", &Compute::DF6Constraint::SetAngularLowerLimit);
				VDF6Constraint.SetMethod("void set_angular_lower_limit_reversed(const vector3 &in)", &Compute::DF6Constraint::SetAngularLowerLimitReversed);
				VDF6Constraint.SetMethod("void set_angular_upper_limit(const vector3 &in)", &Compute::DF6Constraint::SetAngularUpperLimit);
				VDF6Constraint.SetMethod("void set_angular_upper_limit_reversed(const vector3 &in)", &Compute::DF6Constraint::SetAngularUpperLimitReversed);
				VDF6Constraint.SetMethod("void set_limit(int, float, float)", &Compute::DF6Constraint::SetLimit);
				VDF6Constraint.SetMethod("void set_limit_reversed(int, float, float)", &Compute::DF6Constraint::SetLimitReversed);
				VDF6Constraint.SetMethod("void set_rotation_order(physics_rotator)", &Compute::DF6Constraint::SetRotationOrder);
				VDF6Constraint.SetMethod("void set_axis(const vector3 &in, const vector3 &in)", &Compute::DF6Constraint::SetAxis);
				VDF6Constraint.SetMethod("void set_bounce(int, float)", &Compute::DF6Constraint::SetBounce);
				VDF6Constraint.SetMethod("void set_servo(int, bool)", &Compute::DF6Constraint::SetServo);
				VDF6Constraint.SetMethod("void set_target_velocity(int, float)", &Compute::DF6Constraint::SetTargetVelocity);
				VDF6Constraint.SetMethod("void set_servo_target(int, float)", &Compute::DF6Constraint::SetServoTarget);
				VDF6Constraint.SetMethod("void set_max_motor_force(int, float)", &Compute::DF6Constraint::SetMaxMotorForce);
				VDF6Constraint.SetMethod("void set_stiffness(int, float, bool = true)", &Compute::DF6Constraint::SetStiffness);
				VDF6Constraint.SetMethod<Compute::DF6Constraint, void>("void set_equilibrium_point()", &Compute::DF6Constraint::SetEquilibriumPoint);
				VDF6Constraint.SetMethod<Compute::DF6Constraint, void, int>("void set_equilibrium_point(int)", &Compute::DF6Constraint::SetEquilibriumPoint);
				VDF6Constraint.SetMethod<Compute::DF6Constraint, void, int, float>("void set_equilibrium_point(int, float)", &Compute::DF6Constraint::SetEquilibriumPoint);
				VDF6Constraint.SetMethod("vector3 get_angular_upper_limit() const", &Compute::DF6Constraint::GetAngularUpperLimit);
				VDF6Constraint.SetMethod("vector3 get_angular_upper_limit_reversed() const", &Compute::DF6Constraint::GetAngularUpperLimitReversed);
				VDF6Constraint.SetMethod("vector3 get_angular_lower_limit() const", &Compute::DF6Constraint::GetAngularLowerLimit);
				VDF6Constraint.SetMethod("vector3 get_angular_lower_limit_reversed() const", &Compute::DF6Constraint::GetAngularLowerLimitReversed);
				VDF6Constraint.SetMethod("vector3 get_linear_upper_limit() const", &Compute::DF6Constraint::GetLinearUpperLimit);
				VDF6Constraint.SetMethod("vector3 get_linear_lower_limit() const", &Compute::DF6Constraint::GetLinearLowerLimit);
				VDF6Constraint.SetMethod("vector3 get_axis(int) const", &Compute::DF6Constraint::GetAxis);
				VDF6Constraint.SetMethod("physics_rotator get_rotation_order() const", &Compute::DF6Constraint::GetRotationOrder);
				VDF6Constraint.SetMethod("float get_angle(int) const", &Compute::DF6Constraint::GetAngle);
				VDF6Constraint.SetMethod("float get_relative_pivot_position(int) const", &Compute::DF6Constraint::GetRelativePivotPosition);
				VDF6Constraint.SetMethod("bool is_limited(int) const", &Compute::DF6Constraint::IsLimited);
				VDF6Constraint.SetMethod("physics_df6constraint_desc& get_state()", &Compute::DF6Constraint::GetState);

				VMTypeClass VSimulatorDesc = Engine->Global().SetPod<Compute::Simulator::Desc>("physics_simulator_desc");
				VSimulatorDesc.SetProperty<Compute::Simulator::Desc>("vector3 water_normal", &Compute::Simulator::Desc::WaterNormal);
				VSimulatorDesc.SetProperty<Compute::Simulator::Desc>("vector3 gravity", &Compute::Simulator::Desc::Gravity);
				VSimulatorDesc.SetProperty<Compute::Simulator::Desc>("float air_density", &Compute::Simulator::Desc::AirDensity);
				VSimulatorDesc.SetProperty<Compute::Simulator::Desc>("float water_density", &Compute::Simulator::Desc::WaterDensity);
				VSimulatorDesc.SetProperty<Compute::Simulator::Desc>("float water_offset", &Compute::Simulator::Desc::WaterOffset);
				VSimulatorDesc.SetProperty<Compute::Simulator::Desc>("float max_displacement", &Compute::Simulator::Desc::MaxDisplacement);
				VSimulatorDesc.SetProperty<Compute::Simulator::Desc>("bool enable_soft_body", &Compute::Simulator::Desc::EnableSoftBody);
				VSimulatorDesc.SetConstructor<Compute::Simulator::Desc>("void f()");

				VSimulator.SetProperty<Compute::Simulator>("float time_speed", &Compute::Simulator::TimeSpeed);
				VSimulator.SetProperty<Compute::Simulator>("int interpolate", &Compute::Simulator::Interpolate);
				VSimulator.SetProperty<Compute::Simulator>("bool active", &Compute::Simulator::Active);
				VSimulator.SetUnmanagedConstructor<Compute::Simulator, const Compute::Simulator::Desc&>("physics_simulator@ f(const physics_simulator_desc &in)");
				VSimulator.SetMethod("void set_gravity(const vector3 &in)", &Compute::Simulator::SetGravity);
				VSimulator.SetMethod<Compute::Simulator, void, const Compute::Vector3&, bool>("void set_linear_impulse(const vector3 &in, bool = false)", &Compute::Simulator::SetLinearImpulse);
				VSimulator.SetMethod<Compute::Simulator, void, const Compute::Vector3&, int, int, bool>("void set_linear_impulse(const vector3 &in, int, int, bool = false)", &Compute::Simulator::SetLinearImpulse);
				VSimulator.SetMethod<Compute::Simulator, void, const Compute::Vector3&, bool>("void set_angular_impulse(const vector3 &in, bool = false)", &Compute::Simulator::SetAngularImpulse);
				VSimulator.SetMethod<Compute::Simulator, void, const Compute::Vector3&, int, int, bool>("void set_angular_impulse(const vector3 &in, int, int, bool = false)", &Compute::Simulator::SetAngularImpulse);
				VSimulator.SetMethod<Compute::Simulator, void, const Compute::Vector3&, bool>("void create_linear_impulse(const vector3 &in, bool = false)", &Compute::Simulator::CreateLinearImpulse);
				VSimulator.SetMethod<Compute::Simulator, void, const Compute::Vector3&, int, int, bool>("void create_linear_impulse(const vector3 &in, int, int, bool = false)", &Compute::Simulator::CreateLinearImpulse);
				VSimulator.SetMethod<Compute::Simulator, void, const Compute::Vector3&, bool>("void create_angular_impulse(const vector3 &in, bool = false)", &Compute::Simulator::CreateAngularImpulse);
				VSimulator.SetMethod<Compute::Simulator, void, const Compute::Vector3&, int, int, bool>("void create_angular_impulse(const vector3 &in, int, int, bool = false)", &Compute::Simulator::CreateAngularImpulse);
				VSimulator.SetMethod("void add_softbody(physics_softbody@+)", &Compute::Simulator::AddSoftBody);
				VSimulator.SetMethod("void remove_softbody(physics_softbody@+)", &Compute::Simulator::RemoveSoftBody);
				VSimulator.SetMethod("void add_rigidbody(physics_rigidbody@+)", &Compute::Simulator::AddRigidBody);
				VSimulator.SetMethod("void remove_rigidbody(physics_rigidbody@+)", &Compute::Simulator::RemoveRigidBody);
				VSimulator.SetMethod("void add_constraint(physics_constraint@+)", &Compute::Simulator::AddConstraint);
				VSimulator.SetMethod("void remove_constraint(physics_constraint@+)", &Compute::Simulator::RemoveConstraint);
				VSimulator.SetMethod("void remove_all()", &Compute::Simulator::RemoveAll);
				VSimulator.SetMethod("void simulate(int, float, float)", &Compute::Simulator::Simulate);
				VSimulator.SetMethod<Compute::Simulator, Compute::RigidBody*, const Compute::RigidBody::Desc&>("physics_rigidbody@ create_rigidbody(const physics_rigidbody_desc &in)", &Compute::Simulator::CreateRigidBody);
				VSimulator.SetMethod<Compute::Simulator, Compute::RigidBody*, const Compute::RigidBody::Desc&, Compute::Transform*>("physics_rigidbody@ create_rigidbody(const physics_rigidbody_desc &in, transform@+)", &Compute::Simulator::CreateRigidBody);
				VSimulator.SetMethod<Compute::Simulator, Compute::SoftBody*, const Compute::SoftBody::Desc&>("physics_softbody@ create_softbody(const physics_softbody_desc &in)", &Compute::Simulator::CreateSoftBody);
				VSimulator.SetMethod<Compute::Simulator, Compute::SoftBody*, const Compute::SoftBody::Desc&, Compute::Transform*>("physics_softbody@ create_softbody(const physics_softbody_desc &in, transform@+)", &Compute::Simulator::CreateSoftBody);
				VSimulator.SetMethod("physics_pconstraint@ create_pconstraint(const physics_pconstraint_desc &in)", &Compute::Simulator::CreatePoint2PointConstraint);
				VSimulator.SetMethod("physics_hconstraint@ create_hconstraint(const physics_hconstraint_desc &in)", &Compute::Simulator::CreateHingeConstraint);
				VSimulator.SetMethod("physics_sconstraint@ create_sconstraint(const physics_sconstraint_desc &in)", &Compute::Simulator::CreateSliderConstraint);
				VSimulator.SetMethod("physics_ctconstraint@ create_ctconstraint(const physics_ctconstraint_desc &in)", &Compute::Simulator::CreateConeTwistConstraint);
				VSimulator.SetMethod("physics_df6constraint@ create_df6constraint(const physics_df6constraint_desc &in)", &Compute::Simulator::Create6DoFConstraint);
				VSimulator.SetMethod("uptr@ create_shape(physics_shape)", &Compute::Simulator::CreateShape);
				VSimulator.SetMethod("uptr@ create_cube(const vector3 &in = vector3(1, 1, 1))", &Compute::Simulator::CreateCube);
				VSimulator.SetMethod("uptr@ create_sphere(float = 1)", &Compute::Simulator::CreateSphere);
				VSimulator.SetMethod("uptr@ create_capsule(float = 1, float = 1)", &Compute::Simulator::CreateCapsule);
				VSimulator.SetMethod("uptr@ create_cone(float = 1, float = 1)", &Compute::Simulator::CreateCone);
				VSimulator.SetMethod("uptr@ create_cylinder(const vector3 &in = vector3(1, 1, 1))", &Compute::Simulator::CreateCylinder);
				VSimulator.SetMethodEx("uptr@ create_convex_hull(array<skin_vertex>@+)", &SimulatorCreateConvexHullSkinVertex);
				VSimulator.SetMethodEx("uptr@ create_convex_hull(array<vertex>@+)", &SimulatorCreateConvexHullVertex);
				VSimulator.SetMethodEx("uptr@ create_convex_hull(array<vector2>@+)", &SimulatorCreateConvexHullVector2);
				VSimulator.SetMethodEx("uptr@ create_convex_hull(array<vector3>@+)", &SimulatorCreateConvexHullVector3);
				VSimulator.SetMethodEx("uptr@ create_convex_hull(array<vector4>@+)", &SimulatorCreateConvexHullVector4);
				VSimulator.SetMethod<Compute::Simulator, btCollisionShape*, btCollisionShape*>("uptr@ create_convex_hull(uptr@)", &Compute::Simulator::CreateConvexHull);
				VSimulator.SetMethod("uptr@ try_clone_shape(uptr@)", &Compute::Simulator::TryCloneShape);
				VSimulator.SetMethod("uptr@ reuse_shape(uptr@)", &Compute::Simulator::ReuseShape);
				VSimulator.SetMethod("void free_shape(uptr@)", &Compute::Simulator::FreeShape);
				VSimulator.SetMethodEx("array<vector3>@ get_shape_vertices(uptr@) const", &SimulatorGetShapeVertices);
				VSimulator.SetMethod("usize get_shape_vertices_count(uptr@) const", &Compute::Simulator::GetShapeVerticesCount);
				VSimulator.SetMethod("float get_max_displacement() const", &Compute::Simulator::GetMaxDisplacement);
				VSimulator.SetMethod("float get_air_density() const", &Compute::Simulator::GetAirDensity);
				VSimulator.SetMethod("float get_water_offset() const", &Compute::Simulator::GetWaterOffset);
				VSimulator.SetMethod("float get_water_density() const", &Compute::Simulator::GetWaterDensity);
				VSimulator.SetMethod("vector3 get_water_normal() const", &Compute::Simulator::GetWaterNormal);
				VSimulator.SetMethod("vector3 get_gravity() const", &Compute::Simulator::GetGravity);
				VSimulator.SetMethod("bool has_softbody_support() const", &Compute::Simulator::HasSoftBodySupport);
				VSimulator.SetMethod("int get_contact_manifold_count() const", &Compute::Simulator::GetContactManifoldCount);

				VConstraint.SetOperatorEx(VMOpFunc::Cast, 0, "physics_pconstraint@+", "", &ConstraintToPConstraint);
				VConstraint.SetOperatorEx(VMOpFunc::Cast, 0, "physics_hconstraint@+", "", &ConstraintToHConstraint);
				VConstraint.SetOperatorEx(VMOpFunc::Cast, 0, "physics_sconstraint@+", "", &ConstraintToSConstraint);
				VConstraint.SetOperatorEx(VMOpFunc::Cast, 0, "physics_ctconstraint@+", "", &ConstraintToCTConstraint);
				VConstraint.SetOperatorEx(VMOpFunc::Cast, 0, "physics_df6constraint@+", "", &ConstraintToDF6Constraint);
				VPConstraint.SetOperatorEx(VMOpFunc::ImplCast, 0, "physics_constraint@+", "", &PConstraintToConstraint);
				VPConstraint.SetOperatorEx(VMOpFunc::ImplCast, (uint32_t)VMOp::Const, "physics_constraint@+", "", &PConstraintToConstraint);
				VHConstraint.SetOperatorEx(VMOpFunc::ImplCast, 0, "physics_constraint@+", "", &HConstraintToConstraint);
				VHConstraint.SetOperatorEx(VMOpFunc::ImplCast, (uint32_t)VMOp::Const, "physics_constraint@+", "", &HConstraintToConstraint);
				VSConstraint.SetOperatorEx(VMOpFunc::ImplCast, 0, "physics_constraint@+", "", &SConstraintToConstraint);
				VSConstraint.SetOperatorEx(VMOpFunc::ImplCast, (uint32_t)VMOp::Const, "physics_constraint@+", "", &SConstraintToConstraint);
				VCTConstraint.SetOperatorEx(VMOpFunc::ImplCast, 0, "physics_constraint@+", "", &CTConstraintToConstraint);
				VCTConstraint.SetOperatorEx(VMOpFunc::ImplCast, (uint32_t)VMOp::Const, "physics_constraint@+", "", &CTConstraintToConstraint);
				VDF6Constraint.SetOperatorEx(VMOpFunc::ImplCast, 0, "physics_constraint@+", "", &DF6ConstraintToConstraint);
				VDF6Constraint.SetOperatorEx(VMOpFunc::ImplCast, (uint32_t)VMOp::Const, "physics_constraint@+", "", &DF6ConstraintToConstraint);

				return true;
			}
			bool Registry::LoadAudio(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();

				VMEnum VSoundDistanceModel = Register.SetEnum("sound_distance_model");
				VSoundDistanceModel.SetValue("invalid", (int)Audio::SoundDistanceModel::Invalid);
				VSoundDistanceModel.SetValue("invert", (int)Audio::SoundDistanceModel::Invert);
				VSoundDistanceModel.SetValue("invert_clamp", (int)Audio::SoundDistanceModel::Invert_Clamp);
				VSoundDistanceModel.SetValue("linear", (int)Audio::SoundDistanceModel::Linear);
				VSoundDistanceModel.SetValue("linear_clamp", (int)Audio::SoundDistanceModel::Linear_Clamp);
				VSoundDistanceModel.SetValue("exponent", (int)Audio::SoundDistanceModel::Exponent);
				VSoundDistanceModel.SetValue("exponent_clamp", (int)Audio::SoundDistanceModel::Exponent_Clamp);

				VMEnum VSoundEx = Register.SetEnum("sound_ex");
				VSoundEx.SetValue("source_relative", (int)Audio::SoundEx::Source_Relative);
				VSoundEx.SetValue("cone_inner_angle", (int)Audio::SoundEx::Cone_Inner_Angle);
				VSoundEx.SetValue("cone_outer_angle", (int)Audio::SoundEx::Cone_Outer_Angle);
				VSoundEx.SetValue("pitch", (int)Audio::SoundEx::Pitch);
				VSoundEx.SetValue("position", (int)Audio::SoundEx::Position);
				VSoundEx.SetValue("direction", (int)Audio::SoundEx::Direction);
				VSoundEx.SetValue("velocity", (int)Audio::SoundEx::Velocity);
				VSoundEx.SetValue("looping", (int)Audio::SoundEx::Looping);
				VSoundEx.SetValue("buffer", (int)Audio::SoundEx::Buffer);
				VSoundEx.SetValue("gain", (int)Audio::SoundEx::Gain);
				VSoundEx.SetValue("min_gain", (int)Audio::SoundEx::Min_Gain);
				VSoundEx.SetValue("max_gain", (int)Audio::SoundEx::Max_Gain);
				VSoundEx.SetValue("orientation", (int)Audio::SoundEx::Orientation);
				VSoundEx.SetValue("channel_mask", (int)Audio::SoundEx::Channel_Mask);
				VSoundEx.SetValue("source_state", (int)Audio::SoundEx::Source_State);
				VSoundEx.SetValue("initial", (int)Audio::SoundEx::Initial);
				VSoundEx.SetValue("playing", (int)Audio::SoundEx::Playing);
				VSoundEx.SetValue("paused", (int)Audio::SoundEx::Paused);
				VSoundEx.SetValue("stopped", (int)Audio::SoundEx::Stopped);
				VSoundEx.SetValue("buffers_queued", (int)Audio::SoundEx::Buffers_Queued);
				VSoundEx.SetValue("buffers_processed", (int)Audio::SoundEx::Buffers_Processed);
				VSoundEx.SetValue("seconds_offset", (int)Audio::SoundEx::Seconds_Offset);
				VSoundEx.SetValue("sample_offset", (int)Audio::SoundEx::Sample_Offset);
				VSoundEx.SetValue("byte_offset", (int)Audio::SoundEx::Byte_Offset);
				VSoundEx.SetValue("source_type", (int)Audio::SoundEx::Source_Type);
				VSoundEx.SetValue("static", (int)Audio::SoundEx::Static);
				VSoundEx.SetValue("streaming", (int)Audio::SoundEx::Streaming);
				VSoundEx.SetValue("undetermined", (int)Audio::SoundEx::Undetermined);
				VSoundEx.SetValue("format_mono8", (int)Audio::SoundEx::Format_Mono8);
				VSoundEx.SetValue("format_mono16", (int)Audio::SoundEx::Format_Mono16);
				VSoundEx.SetValue("format_stereo8", (int)Audio::SoundEx::Format_Stereo8);
				VSoundEx.SetValue("format_stereo16", (int)Audio::SoundEx::Format_Stereo16);
				VSoundEx.SetValue("reference_distance", (int)Audio::SoundEx::Reference_Distance);
				VSoundEx.SetValue("rolloff_gactor", (int)Audio::SoundEx::Rolloff_Factor);
				VSoundEx.SetValue("cone_outer_gain", (int)Audio::SoundEx::Cone_Outer_Gain);
				VSoundEx.SetValue("max_distance", (int)Audio::SoundEx::Max_Distance);
				VSoundEx.SetValue("frequency", (int)Audio::SoundEx::Frequency);
				VSoundEx.SetValue("bits", (int)Audio::SoundEx::Bits);
				VSoundEx.SetValue("channels", (int)Audio::SoundEx::Channels);
				VSoundEx.SetValue("size", (int)Audio::SoundEx::Size);
				VSoundEx.SetValue("unused", (int)Audio::SoundEx::Unused);
				VSoundEx.SetValue("pending", (int)Audio::SoundEx::Pending);
				VSoundEx.SetValue("processed", (int)Audio::SoundEx::Processed);
				VSoundEx.SetValue("invalid_name", (int)Audio::SoundEx::Invalid_Name);
				VSoundEx.SetValue("illegal_enum", (int)Audio::SoundEx::Illegal_Enum);
				VSoundEx.SetValue("invalid_enum", (int)Audio::SoundEx::Invalid_Enum);
				VSoundEx.SetValue("invalid_value", (int)Audio::SoundEx::Invalid_Value);
				VSoundEx.SetValue("illegal_command", (int)Audio::SoundEx::Illegal_Command);
				VSoundEx.SetValue("invalid_operation", (int)Audio::SoundEx::Invalid_Operation);
				VSoundEx.SetValue("out_of_memory", (int)Audio::SoundEx::Out_Of_Memory);
				VSoundEx.SetValue("vendor", (int)Audio::SoundEx::Vendor);
				VSoundEx.SetValue("version", (int)Audio::SoundEx::Version);
				VSoundEx.SetValue("renderer", (int)Audio::SoundEx::Renderer);
				VSoundEx.SetValue("extentions", (int)Audio::SoundEx::Extentions);
				VSoundEx.SetValue("doppler_factor", (int)Audio::SoundEx::Doppler_Factor);
				VSoundEx.SetValue("doppler_velocity", (int)Audio::SoundEx::Doppler_Velocity);
				VSoundEx.SetValue("speed_of_sound", (int)Audio::SoundEx::Speed_Of_Sound);

				VMTypeClass VAudioSync = Engine->Global().SetPod<Audio::AudioSync>("audio_sync");
				VAudioSync.SetProperty<Audio::AudioSync>("vector3 direction", &Audio::AudioSync::Direction);
				VAudioSync.SetProperty<Audio::AudioSync>("vector3 velocity", &Audio::AudioSync::Velocity);
				VAudioSync.SetProperty<Audio::AudioSync>("float cone_inner_angle", &Audio::AudioSync::ConeInnerAngle);
				VAudioSync.SetProperty<Audio::AudioSync>("float cone_outer_angle", &Audio::AudioSync::ConeOuterAngle);
				VAudioSync.SetProperty<Audio::AudioSync>("float cone_outer_gain", &Audio::AudioSync::ConeOuterGain);
				VAudioSync.SetProperty<Audio::AudioSync>("float pitch", &Audio::AudioSync::Pitch);
				VAudioSync.SetProperty<Audio::AudioSync>("float gain", &Audio::AudioSync::Gain);
				VAudioSync.SetProperty<Audio::AudioSync>("float ref_distance", &Audio::AudioSync::RefDistance);
				VAudioSync.SetProperty<Audio::AudioSync>("float distance", &Audio::AudioSync::Distance);
				VAudioSync.SetProperty<Audio::AudioSync>("float rolloff", &Audio::AudioSync::Rolloff);
				VAudioSync.SetProperty<Audio::AudioSync>("float position", &Audio::AudioSync::Position);
				VAudioSync.SetProperty<Audio::AudioSync>("float air_absorption", &Audio::AudioSync::AirAbsorption);
				VAudioSync.SetProperty<Audio::AudioSync>("float room_roll_off", &Audio::AudioSync::RoomRollOff);
				VAudioSync.SetProperty<Audio::AudioSync>("bool is_relative", &Audio::AudioSync::IsRelative);
				VAudioSync.SetProperty<Audio::AudioSync>("bool is_looped", &Audio::AudioSync::IsLooped);
				VAudioSync.SetConstructor<Audio::AudioSync>("void f()");

				Engine->BeginNamespace("audio_context");
				Register.SetFunction("void create()", Audio::AudioContext::Create);
				Register.SetFunction("void release()", Audio::AudioContext::Release);
				Register.SetFunction("void lock()", Audio::AudioContext::Lock);
				Register.SetFunction("void unlock()", Audio::AudioContext::Unlock);
				Register.SetFunction("void generate_buffers(int32, uint32 &out)", Audio::AudioContext::GenerateBuffers);
				Register.SetFunction("void set_source_data_3f(uint32, sound_ex, float, float, float)", Audio::AudioContext::SetSourceData3F);
				Register.SetFunction("void get_source_data_3f(uint32, sound_ex, float &out, float &out, float &out)", Audio::AudioContext::GetSourceData3F);
				Register.SetFunction("void set_source_data_1f(uint32, sound_ex, float)", Audio::AudioContext::SetSourceData1F);
				Register.SetFunction("void get_source_data_1f(uint32, sound_ex, float &out)", Audio::AudioContext::GetSourceData1F);
				Register.SetFunction("void set_source_data_3i(uint32, sound_ex, int32, int32, int32)", Audio::AudioContext::SetSourceData3I);
				Register.SetFunction("void get_source_data_3i(uint32, sound_ex, int32 &out, int32 &out, int32 &out)", Audio::AudioContext::GetSourceData3I);
				Register.SetFunction("void set_source_data_1i(uint32, sound_ex, int32)", Audio::AudioContext::SetSourceData1I);
				Register.SetFunction("void get_source_data_1i(uint32, sound_ex, int32 &out)", Audio::AudioContext::GetSourceData1I);
				Register.SetFunction("void set_listener_data_3f(sound_ex, float, float, float)", Audio::AudioContext::SetListenerData3F);
				Register.SetFunction("void get_listener_data_3f(sound_ex, float &out, float &out, float &out)", Audio::AudioContext::GetListenerData3F);
				Register.SetFunction("void set_listener_data_1f(sound_ex, float)", Audio::AudioContext::SetListenerData1F);
				Register.SetFunction("void get_listener_data_1f(sound_ex, float &out)", Audio::AudioContext::GetListenerData1F);
				Register.SetFunction("void set_listener_data_3i(sound_ex, int32, int32, int32)", Audio::AudioContext::SetListenerData3I);
				Register.SetFunction("void get_listener_data_3i(sound_ex, int32 &out, int32 &out, int32 &out)", Audio::AudioContext::GetListenerData3I);
				Register.SetFunction("void set_listener_data_1i(sound_ex, int32)", Audio::AudioContext::SetListenerData1I);
				Register.SetFunction("void get_listener_data_1i(sound_ex, int32 &out)", Audio::AudioContext::GetListenerData1I);
				Engine->EndNamespace();

				VMRefClass VAudioSource = Engine->Global().SetClassUnmanaged<Audio::AudioSource>("audio_source");
				VMRefClass VAudioFilter = Engine->Global().SetClassUnmanaged<Audio::AudioFilter>("audio_filter");
				VAudioFilter.SetMethod("void synchronize()", &Audio::AudioFilter::Synchronize);
				VAudioFilter.SetMethod("void deserialize(schema@+)", &Audio::AudioFilter::Deserialize);
				VAudioFilter.SetMethod("void serialize(schema@+)", &Audio::AudioFilter::Serialize);
				VAudioFilter.SetMethod("audio_filter@ copy()", &Audio::AudioFilter::Copy);
				VAudioFilter.SetMethod("audio_source@+ get_source()", &Audio::AudioFilter::GetSource);
				PopulateComponent<Audio::AudioFilter>(VAudioFilter);

				VMRefClass VAudioEffect = Engine->Global().SetClassUnmanaged<Audio::AudioEffect>("audio_effect");
				VAudioEffect.SetMethod("void synchronize()", &Audio::AudioEffect::Synchronize);
				VAudioEffect.SetMethod("void deserialize(schema@+)", &Audio::AudioEffect::Deserialize);
				VAudioEffect.SetMethod("void serialize(schema@+)", &Audio::AudioEffect::Serialize);
				VAudioEffect.SetMethodEx("bool set_filter(audio_filter@+)", &AudioEffectSetFilter);
				VAudioEffect.SetMethod("audio_effect@ copy()", &Audio::AudioEffect::Copy);
				VAudioEffect.SetMethod("audio_source@+ get_filter()", &Audio::AudioEffect::GetFilter);
				VAudioEffect.SetMethod("audio_source@+ get_source()", &Audio::AudioEffect::GetSource);
				PopulateComponent<Audio::AudioEffect>(VAudioEffect);

				VMRefClass VAudioClip = Engine->Global().SetClassUnmanaged<Audio::AudioClip>("audio_clip");
				VAudioClip.SetUnmanagedConstructor<Audio::AudioClip, int, int>("audio_clip@ f(int, int)");
				VAudioClip.SetMethod("float length() const", &Audio::AudioClip::Length);
				VAudioClip.SetMethod("bool is_mono() const", &Audio::AudioClip::IsMono);
				VAudioClip.SetMethod("uint32 get_buffer() const", &Audio::AudioClip::GetBuffer);
				VAudioClip.SetMethod("int32 get_format() const", &Audio::AudioClip::GetFormat);

				VAudioSource.SetUnmanagedConstructor<Audio::AudioSource>("audio_source@ f()");
				VAudioSource.SetMethod("int64 add_effect(audio_effect@+)", &Audio::AudioSource::AddEffect);
				VAudioSource.SetMethod("bool remove_effect(usize)", &Audio::AudioSource::RemoveEffect);
				VAudioSource.SetMethod("bool remove_effect_by_id(uint64)", &Audio::AudioSource::RemoveEffectById);
				VAudioSource.SetMethod("void set_clip(audio_clip@+)", &Audio::AudioSource::SetClip);
				VAudioSource.SetMethod("void synchronize(audio_sync &in, const vector3 &in)", &Audio::AudioSource::Synchronize);
				VAudioSource.SetMethod("void reset()", &Audio::AudioSource::Reset);
				VAudioSource.SetMethod("void pause()", &Audio::AudioSource::Pause);
				VAudioSource.SetMethod("void play()", &Audio::AudioSource::Play);
				VAudioSource.SetMethod("void stop()", &Audio::AudioSource::Stop);
				VAudioSource.SetMethod("bool is_playing() const", &Audio::AudioSource::IsPlaying);
				VAudioSource.SetMethod("usize get_effects_count() const", &Audio::AudioSource::GetEffectsCount);
				VAudioSource.SetMethod("uint32 get_instance() const", &Audio::AudioSource::GetInstance);
				VAudioSource.SetMethod("audio_clip@+ get_clip() const", &Audio::AudioSource::GetClip);
				VAudioSource.SetMethod<Audio::AudioSource, Audio::AudioEffect*, uint64_t>("audio_effect@+ get_effect(uint64) const", &Audio::AudioSource::GetEffect);

				VMRefClass VAudioDevice = Engine->Global().SetClassUnmanaged<Audio::AudioDevice>("audio_device");
				VAudioDevice.SetUnmanagedConstructor<Audio::AudioDevice>("audio_device@ f()");
				VAudioDevice.SetMethod("void offset(audio_source@+, float &out, bool)", &Audio::AudioDevice::Offset);
				VAudioDevice.SetMethod("void velocity(audio_source@+, vector3 &out, bool)", &Audio::AudioDevice::Velocity);
				VAudioDevice.SetMethod("void position(audio_source@+, vector3 &out, bool)", &Audio::AudioDevice::Position);
				VAudioDevice.SetMethod("void direction(audio_source@+, vector3 &out, bool)", &Audio::AudioDevice::Direction);
				VAudioDevice.SetMethod("void relative(audio_source@+, int &out, bool)", &Audio::AudioDevice::Relative);
				VAudioDevice.SetMethod("void pitch(audio_source@+, float &out, bool)", &Audio::AudioDevice::Pitch);
				VAudioDevice.SetMethod("void gain(audio_source@+, float &out, bool)", &Audio::AudioDevice::Gain);
				VAudioDevice.SetMethod("void loop(audio_source@+, int &out, bool)", &Audio::AudioDevice::Loop);
				VAudioDevice.SetMethod("void cone_inner_angle(audio_source@+, float &out, bool)", &Audio::AudioDevice::ConeInnerAngle);
				VAudioDevice.SetMethod("void cone_outer_angle(audio_source@+, float &out, bool)", &Audio::AudioDevice::ConeOuterAngle);
				VAudioDevice.SetMethod("void cone_outer_gain(audio_source@+, float &out, bool)", &Audio::AudioDevice::ConeOuterGain);
				VAudioDevice.SetMethod("void distance(audio_source@+, float &out, bool)", &Audio::AudioDevice::Distance);
				VAudioDevice.SetMethod("void ref_distance(audio_source@+, float &out, bool)", &Audio::AudioDevice::RefDistance);
				VAudioDevice.SetMethod("void set_distance_model(sound_distance_model)", &Audio::AudioDevice::SetDistanceModel);
				VAudioDevice.SetMethod("void set_exception_codes(int32 &out, int32 &out) const", &Audio::AudioDevice::GetExceptionCodes);
				VAudioDevice.SetMethod("bool is_valid() const", &Audio::AudioDevice::IsValid);

				return true;
			}
			bool Registry::LoadUiControl(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();

				VMEnum VEventPhase = Register.SetEnum("ui_event_phase");
				VMEnum VArea = Register.SetEnum("ui_area");
				VMEnum VDisplay = Register.SetEnum("ui_display");
				VMEnum VPosition = Register.SetEnum("ui_position");
				VMEnum VFloat = Register.SetEnum("ui_float");
				VMEnum VTimingFunc = Register.SetEnum("ui_timing_func");
				VMEnum VTimingDir = Register.SetEnum("ui_timing_dir");
				VMEnum VFocusFlag = Register.SetEnum("ui_focus_flag");
				VMEnum VModalFlag = Register.SetEnum("ui_modal_flag");
				VMTypeClass VElement = Register.SetStructUnmanaged<Engine::GUI::IElement>("ui_element");
				VMTypeClass VDocument = Register.SetStructUnmanaged<Engine::GUI::IElementDocument>("ui_document");
				VMTypeClass VEvent = Register.SetStructUnmanaged<Engine::GUI::IEvent>("ui_event");
				VMRefClass VListener = Register.SetClassUnmanaged<ModelListener>("ui_listener");
				Register.SetFunctionDef("void model_listener_event(ui_event &in)");

				VModalFlag.SetValue("none", (int)Engine::GUI::ModalFlag::None);
				VModalFlag.SetValue("modal", (int)Engine::GUI::ModalFlag::Modal);
				VModalFlag.SetValue("keep", (int)Engine::GUI::ModalFlag::Keep);

				VFocusFlag.SetValue("none", (int)Engine::GUI::FocusFlag::None);
				VFocusFlag.SetValue("document", (int)Engine::GUI::FocusFlag::Document);
				VFocusFlag.SetValue("keep", (int)Engine::GUI::FocusFlag::Keep);
				VFocusFlag.SetValue("automatic", (int)Engine::GUI::FocusFlag::Auto);

				VEventPhase.SetValue("none", (int)Engine::GUI::EventPhase::None);
				VEventPhase.SetValue("capture", (int)Engine::GUI::EventPhase::Capture);
				VEventPhase.SetValue("target", (int)Engine::GUI::EventPhase::Target);
				VEventPhase.SetValue("bubble", (int)Engine::GUI::EventPhase::Bubble);

				VEvent.SetConstructor<Engine::GUI::IEvent>("void f()");
				VEvent.SetConstructor<Engine::GUI::IEvent, Rml::Event*>("void f(uptr@)");
				VEvent.SetMethod("ui_event_phase get_phase() const", &Engine::GUI::IEvent::GetPhase);
				VEvent.SetMethod("void set_phase(ui_event_phase Phase)", &Engine::GUI::IEvent::SetPhase);
				VEvent.SetMethod("void set_current_element(const ui_element &in)", &Engine::GUI::IEvent::SetCurrentElement);
				VEvent.SetMethod("ui_element get_current_element() const", &Engine::GUI::IEvent::GetCurrentElement);
				VEvent.SetMethod("ui_element get_target_element() const", &Engine::GUI::IEvent::GetTargetElement);
				VEvent.SetMethod("string get_type() const", &Engine::GUI::IEvent::GetType);
				VEvent.SetMethod("void stop_propagation()", &Engine::GUI::IEvent::StopPropagation);
				VEvent.SetMethod("void stop_immediate_propagation()", &Engine::GUI::IEvent::StopImmediatePropagation);
				VEvent.SetMethod("bool is_interruptible() const", &Engine::GUI::IEvent::IsInterruptible);
				VEvent.SetMethod("bool is_propagating() const", &Engine::GUI::IEvent::IsPropagating);
				VEvent.SetMethod("bool is_immediate_propagating() const", &Engine::GUI::IEvent::IsImmediatePropagating);
				VEvent.SetMethod("bool get_boolean(const string &in) const", &Engine::GUI::IEvent::GetBoolean);
				VEvent.SetMethod("int64 get_integer(const string &in) const", &Engine::GUI::IEvent::GetInteger);
				VEvent.SetMethod("double get_number(const string &in) const", &Engine::GUI::IEvent::GetNumber);
				VEvent.SetMethod("string get_string(const string &in) const", &Engine::GUI::IEvent::GetString);
				VEvent.SetMethod("vector2 get_vector2(const string &in) const", &Engine::GUI::IEvent::GetVector2);
				VEvent.SetMethod("vector3 get_vector3(const string &in) const", &Engine::GUI::IEvent::GetVector3);
				VEvent.SetMethod("vector4 get_vector4(const string &in) const", &Engine::GUI::IEvent::GetVector4);
				VEvent.SetMethod("uptr@ get_pointer(const string &in) const", &Engine::GUI::IEvent::GetPointer);
				VEvent.SetMethod("uptr@ get_event() const", &Engine::GUI::IEvent::GetEvent);
				VEvent.SetMethod("bool is_valid() const", &Engine::GUI::IEvent::IsValid);

				VListener.SetUnmanagedConstructor<ModelListener, VMCFunction*>("ui_listener@ f(model_listener_event@)");
				VListener.SetUnmanagedConstructor<ModelListener, const std::string&>("ui_listener@ f(const string &in)");

				VArea.SetValue("margin", (int)Engine::GUI::Area::Margin);
				VArea.SetValue("border", (int)Engine::GUI::Area::Border);
				VArea.SetValue("padding", (int)Engine::GUI::Area::Padding);
				VArea.SetValue("content", (int)Engine::GUI::Area::Content);

				VDisplay.SetValue("none", (int)Engine::GUI::Display::None);
				VDisplay.SetValue("block", (int)Engine::GUI::Display::Block);
				VDisplay.SetValue("inline", (int)Engine::GUI::Display::Inline);
				VDisplay.SetValue("inline_block", (int)Engine::GUI::Display::InlineBlock);
				VDisplay.SetValue("flex", (int)Engine::GUI::Display::Flex);
				VDisplay.SetValue("table", (int)Engine::GUI::Display::Table);
				VDisplay.SetValue("table_row", (int)Engine::GUI::Display::TableRow);
				VDisplay.SetValue("table_row_group", (int)Engine::GUI::Display::TableRowGroup);
				VDisplay.SetValue("table_column", (int)Engine::GUI::Display::TableColumn);
				VDisplay.SetValue("table_column_group", (int)Engine::GUI::Display::TableColumnGroup);
				VDisplay.SetValue("table_cell", (int)Engine::GUI::Display::TableCell);

				VPosition.SetValue("static", (int)Engine::GUI::Position::Static);
				VPosition.SetValue("relative", (int)Engine::GUI::Position::Relative);
				VPosition.SetValue("absolute", (int)Engine::GUI::Position::Absolute);
				VPosition.SetValue("fixed", (int)Engine::GUI::Position::Fixed);

				VFloat.SetValue("none", (int)Engine::GUI::Float::None);
				VFloat.SetValue("left", (int)Engine::GUI::Float::Left);
				VFloat.SetValue("right", (int)Engine::GUI::Float::Right);

				VTimingFunc.SetValue("none", (int)Engine::GUI::TimingFunc::None);
				VTimingFunc.SetValue("back", (int)Engine::GUI::TimingFunc::Back);
				VTimingFunc.SetValue("bounce", (int)Engine::GUI::TimingFunc::Bounce);
				VTimingFunc.SetValue("circular", (int)Engine::GUI::TimingFunc::Circular);
				VTimingFunc.SetValue("cubic", (int)Engine::GUI::TimingFunc::Cubic);
				VTimingFunc.SetValue("elastic", (int)Engine::GUI::TimingFunc::Elastic);
				VTimingFunc.SetValue("exponential", (int)Engine::GUI::TimingFunc::Exponential);
				VTimingFunc.SetValue("linear", (int)Engine::GUI::TimingFunc::Linear);
				VTimingFunc.SetValue("quadratic", (int)Engine::GUI::TimingFunc::Quadratic);
				VTimingFunc.SetValue("quartic", (int)Engine::GUI::TimingFunc::Quartic);
				VTimingFunc.SetValue("sine", (int)Engine::GUI::TimingFunc::Sine);
				VTimingFunc.SetValue("callback", (int)Engine::GUI::TimingFunc::Callback);

				VTimingDir.SetValue("use_in", (int)Engine::GUI::TimingDir::In);
				VTimingDir.SetValue("use_out", (int)Engine::GUI::TimingDir::Out);
				VTimingDir.SetValue("use_in_out", (int)Engine::GUI::TimingDir::InOut);
				
				VElement.SetConstructor<Engine::GUI::IElement>("void f()");
				VElement.SetConstructor<Engine::GUI::IElement, Rml::Element*>("void f(uptr@)");
				VElement.SetMethod("ui_element clone() const", &Engine::GUI::IElement::Clone);
				VElement.SetMethod("void set_class(const string &in, bool)", &Engine::GUI::IElement::SetClass);
				VElement.SetMethod("bool is_class_set(const string &in) const", &Engine::GUI::IElement::IsClassSet);
				VElement.SetMethod("void set_class_names(const string &in)", &Engine::GUI::IElement::SetClassNames);
				VElement.SetMethod("string get_class_names() const", &Engine::GUI::IElement::GetClassNames);
				VElement.SetMethod("string get_address(bool = false, bool = true) const", &Engine::GUI::IElement::GetAddress);
				VElement.SetMethod("void set_offset(const vector2 &in, const ui_element &in, bool = false)", &Engine::GUI::IElement::SetOffset);
				VElement.SetMethod("vector2 get_relative_offset(ui_area = ui_area::Content) const", &Engine::GUI::IElement::GetRelativeOffset);
				VElement.SetMethod("vector2 get_absolute_offset(ui_area = ui_area::Content) const", &Engine::GUI::IElement::GetAbsoluteOffset);
				VElement.SetMethod("void set_client_area(ui_area)", &Engine::GUI::IElement::SetClientArea);
				VElement.SetMethod("ui_area get_client_area() const", &Engine::GUI::IElement::GetClientArea);
				VElement.SetMethod("void set_bontent_box(const vector2 &in, const vector2 &in)", &Engine::GUI::IElement::SetContentBox);
				VElement.SetMethod("float get_baseline() const", &Engine::GUI::IElement::GetBaseline);
				VElement.SetMethod("bool get_intrinsic_dimensions(vector2 &out, float &out)", &Engine::GUI::IElement::GetIntrinsicDimensions);
				VElement.SetMethod("bool is_point_within_element(const vector2 &in)", &Engine::GUI::IElement::IsPointWithinElement);
				VElement.SetMethod("bool is_visible() const", &Engine::GUI::IElement::IsVisible);
				VElement.SetMethod("float get_zindex() const", &Engine::GUI::IElement::GetZIndex);
				VElement.SetMethod("bool set_property(const string &in, const string &in)", &Engine::GUI::IElement::SetProperty);
				VElement.SetMethod("void remove_property(const string &in)", &Engine::GUI::IElement::RemoveProperty);
				VElement.SetMethod("string get_property(const string &in) const", &Engine::GUI::IElement::GetProperty);
				VElement.SetMethod("string get_local_property(const string &in) const", &Engine::GUI::IElement::GetLocalProperty);
				VElement.SetMethod("float resolve_numeric_property(const string &in) const", &Engine::GUI::IElement::ResolveNumericProperty);
				VElement.SetMethod("vector2 get_containing_block() const", &Engine::GUI::IElement::GetContainingBlock);
				VElement.SetMethod("ui_position get_position() const", &Engine::GUI::IElement::GetPosition);
				VElement.SetMethod("ui_float get_float() const", &Engine::GUI::IElement::GetFloat);
				VElement.SetMethod("ui_display get_display() const", &Engine::GUI::IElement::GetDisplay);
				VElement.SetMethod("float get_line_height() const", &Engine::GUI::IElement::GetLineHeight);
				VElement.SetMethod("bool project(vector2 &out) const", &Engine::GUI::IElement::Project);
				VElement.SetMethod("bool animate(const string &in, const string &in, float, ui_timing_func, ui_timing_dir, int = -1, bool = true, float = 0)", &Engine::GUI::IElement::Animate);
				VElement.SetMethod("bool add_animation_key(const string &in, const string &in, float, ui_timing_func, ui_timing_dir)", &Engine::GUI::IElement::AddAnimationKey);
				VElement.SetMethod("void set_pseudo_Class(const string &in, bool)", &Engine::GUI::IElement::SetPseudoClass);
				VElement.SetMethod("bool is_pseudo_class_set(const string &in) const", &Engine::GUI::IElement::IsPseudoClassSet);
				VElement.SetMethod("void set_attribute(const string &in, const string &in)", &Engine::GUI::IElement::SetAttribute);
				VElement.SetMethod("string get_attribute(const string &in) const", &Engine::GUI::IElement::GetAttribute);
				VElement.SetMethod("bool has_attribute(const string &in) const", &Engine::GUI::IElement::HasAttribute);
				VElement.SetMethod("void remove_attribute(const string &in)", &Engine::GUI::IElement::RemoveAttribute);
				VElement.SetMethod("ui_element get_focus_leaf_node()", &Engine::GUI::IElement::GetFocusLeafNode);
				VElement.SetMethod("string get_tag_name() const", &Engine::GUI::IElement::GetTagName);
				VElement.SetMethod("string get_id() const", &Engine::GUI::IElement::GetId);
				VElement.SetMethod("float get_absolute_left()", &Engine::GUI::IElement::GetAbsoluteLeft);
				VElement.SetMethod("float get_absolute_top()", &Engine::GUI::IElement::GetAbsoluteTop);
				VElement.SetMethod("float get_client_left()", &Engine::GUI::IElement::GetClientLeft);
				VElement.SetMethod("float get_client_top()", &Engine::GUI::IElement::GetClientTop);
				VElement.SetMethod("float get_client_width()", &Engine::GUI::IElement::GetClientWidth);
				VElement.SetMethod("float get_client_height()", &Engine::GUI::IElement::GetClientHeight);
				VElement.SetMethod("ui_element get_offset_parent()", &Engine::GUI::IElement::GetOffsetParent);
				VElement.SetMethod("float get_offset_left()", &Engine::GUI::IElement::GetOffsetLeft);
				VElement.SetMethod("float get_offset_top()", &Engine::GUI::IElement::GetOffsetTop);
				VElement.SetMethod("float get_offset_width()", &Engine::GUI::IElement::GetOffsetWidth);
				VElement.SetMethod("float get_offset_height()", &Engine::GUI::IElement::GetOffsetHeight);
				VElement.SetMethod("float get_scroll_left()", &Engine::GUI::IElement::GetScrollLeft);
				VElement.SetMethod("void set_scroll_left(float)", &Engine::GUI::IElement::SetScrollLeft);
				VElement.SetMethod("float get_scroll_top()", &Engine::GUI::IElement::GetScrollTop);
				VElement.SetMethod("void set_scroll_top(float)", &Engine::GUI::IElement::SetScrollTop);
				VElement.SetMethod("float get_scroll_width()", &Engine::GUI::IElement::GetScrollWidth);
				VElement.SetMethod("float get_scroll_height()", &Engine::GUI::IElement::GetScrollHeight);
				VElement.SetMethod("ui_document get_owner_document() const", &Engine::GUI::IElement::GetOwnerDocument);
				VElement.SetMethod("ui_element get_parent_node() const", &Engine::GUI::IElement::GetParentNode);
				VElement.SetMethod("ui_element get_next_sibling() const", &Engine::GUI::IElement::GetNextSibling);
				VElement.SetMethod("ui_element get_previous_sibling() const", &Engine::GUI::IElement::GetPreviousSibling);
				VElement.SetMethod("ui_element get_first_child() const", &Engine::GUI::IElement::GetFirstChild);
				VElement.SetMethod("ui_element get_last_child() const", &Engine::GUI::IElement::GetLastChild);
				VElement.SetMethod("ui_element get_child(int) const", &Engine::GUI::IElement::GetChild);
				VElement.SetMethod("int get_num_children(bool = false) const", &Engine::GUI::IElement::GetNumChildren);
				VElement.SetMethod<Engine::GUI::IElement, void, std::string&>("void get_inner_html(string &out) const", &Engine::GUI::IElement::GetInnerHTML);
				VElement.SetMethod<Engine::GUI::IElement, std::string>("string get_inner_html() const", &Engine::GUI::IElement::GetInnerHTML);
				VElement.SetMethod("void set_inner_html(const string &in)", &Engine::GUI::IElement::SetInnerHTML);
				VElement.SetMethod("bool is_focused()", &Engine::GUI::IElement::IsFocused);
				VElement.SetMethod("bool is_hovered()", &Engine::GUI::IElement::IsHovered);
				VElement.SetMethod("bool is_active()", &Engine::GUI::IElement::IsActive);
				VElement.SetMethod("bool is_checked()", &Engine::GUI::IElement::IsChecked);
				VElement.SetMethod("bool focus()", &Engine::GUI::IElement::Focus);
				VElement.SetMethod("void blur()", &Engine::GUI::IElement::Blur);
				VElement.SetMethod("void click()", &Engine::GUI::IElement::Click);
				VElement.SetMethod("void add_event_listener(const string &in, ui_listener@+, bool = false)", &Engine::GUI::IElement::AddEventListener);
				VElement.SetMethod("void remove_event_listener(const string &in, ui_listener@+, bool = false)", &Engine::GUI::IElement::RemoveEventListener);
				VElement.SetMethodEx("bool dispatch_event(const string &in, schema@+)", &IElementDocumentDispatchEvent);
				VElement.SetMethod("void scroll_into_view(bool = true)", &Engine::GUI::IElement::ScrollIntoView);
				VElement.SetMethod("ui_element append_child(const ui_element &in, bool = true)", &Engine::GUI::IElement::AppendChild);
				VElement.SetMethod("ui_element insert_before(const ui_element &in, const ui_element &in)", &Engine::GUI::IElement::InsertBefore);
				VElement.SetMethod("ui_element replace_child(const ui_element &in, const ui_element &in)", &Engine::GUI::IElement::ReplaceChild);
				VElement.SetMethod("ui_element remove_child(const ui_element &in)", &Engine::GUI::IElement::RemoveChild);
				VElement.SetMethod("bool has_child_nodes() const", &Engine::GUI::IElement::HasChildNodes);
				VElement.SetMethod("ui_element get_element_by_id(const string &in)", &Engine::GUI::IElement::GetElementById);
				VElement.SetMethodEx("array<ui_element>@ query_selector_all(const string &in)", &IElementDocumentQuerySelectorAll);
				VElement.SetMethod("bool cast_form_color(vector4 &out, bool)", &Engine::GUI::IElement::CastFormColor);
				VElement.SetMethod("bool cast_form_string(string &out)", &Engine::GUI::IElement::CastFormString);
				VElement.SetMethod("bool cast_form_pointer(uptr@ &out)", &Engine::GUI::IElement::CastFormPointer);
				VElement.SetMethod("bool cast_form_int32(int &out)", &Engine::GUI::IElement::CastFormInt32);
				VElement.SetMethod("bool cast_form_uint32(uint &out)", &Engine::GUI::IElement::CastFormUInt32);
				VElement.SetMethod("bool cast_form_flag32(uint &out, uint)", &Engine::GUI::IElement::CastFormFlag32);
				VElement.SetMethod("bool cast_form_int64(int64 &out)", &Engine::GUI::IElement::CastFormInt64);
				VElement.SetMethod("bool cast_form_uint64(uint64 &out)", &Engine::GUI::IElement::CastFormUInt64);
				VElement.SetMethod("bool cast_form_flag64(uint64 &out, uint64)", &Engine::GUI::IElement::CastFormFlag64);
				VElement.SetMethod<Engine::GUI::IElement, bool, float*>("bool cast_form_float(float &out)", &Engine::GUI::IElement::CastFormFloat);
				VElement.SetMethod<Engine::GUI::IElement, bool, float*, float>("bool cast_form_float(float &out, float)", &Engine::GUI::IElement::CastFormFloat);
				VElement.SetMethod("bool cast_form_double(double &out)", &Engine::GUI::IElement::CastFormDouble);
				VElement.SetMethod("bool cast_form_boolean(bool &out)", &Engine::GUI::IElement::CastFormBoolean);
				VElement.SetMethod("string get_form_name() const", &Engine::GUI::IElement::GetFormName);
				VElement.SetMethod("void set_form_name(const string &in)", &Engine::GUI::IElement::SetFormName);
				VElement.SetMethod("string get_form_value() const", &Engine::GUI::IElement::GetFormValue);
				VElement.SetMethod("void set_form_value(const string &in)", &Engine::GUI::IElement::SetFormValue);
				VElement.SetMethod("bool is_form_disabled() const", &Engine::GUI::IElement::IsFormDisabled);
				VElement.SetMethod("void set_form_disabled(bool)", &Engine::GUI::IElement::SetFormDisabled);
				VElement.SetMethod("uptr@ get_element() const", &Engine::GUI::IElement::GetElement);
				VElement.SetMethod("bool is_valid() const", &Engine::GUI::IElement::IsValid);

				VDocument.SetConstructor<Engine::GUI::IElementDocument>("void f()");
				VDocument.SetConstructor<Engine::GUI::IElementDocument, Rml::ElementDocument*>("void f(uptr@)");
				VDocument.SetMethod("ui_element clone() const", &Engine::GUI::IElementDocument::Clone);
				VDocument.SetMethod("void set_class(const string &in, bool)", &Engine::GUI::IElementDocument::SetClass);
				VDocument.SetMethod("bool is_class_set(const string &in) const", &Engine::GUI::IElementDocument::IsClassSet);
				VDocument.SetMethod("void set_class_names(const string &in)", &Engine::GUI::IElementDocument::SetClassNames);
				VDocument.SetMethod("string get_class_names() const", &Engine::GUI::IElementDocument::GetClassNames);
				VDocument.SetMethod("string get_address(bool = false, bool = true) const", &Engine::GUI::IElementDocument::GetAddress);
				VDocument.SetMethod("void set_offset(const vector2 &in, const ui_element &in, bool = false)", &Engine::GUI::IElementDocument::SetOffset);
				VDocument.SetMethod("vector2 get_relative_offset(ui_area = ui_area::Content) const", &Engine::GUI::IElementDocument::GetRelativeOffset);
				VDocument.SetMethod("vector2 get_absolute_offset(ui_area = ui_area::Content) const", &Engine::GUI::IElementDocument::GetAbsoluteOffset);
				VDocument.SetMethod("void set_client_area(ui_area)", &Engine::GUI::IElementDocument::SetClientArea);
				VDocument.SetMethod("ui_area get_client_area() const", &Engine::GUI::IElementDocument::GetClientArea);
				VDocument.SetMethod("void set_bontent_box(const vector2 &in, const vector2 &in)", &Engine::GUI::IElementDocument::SetContentBox);
				VDocument.SetMethod("float get_baseline() const", &Engine::GUI::IElementDocument::GetBaseline);
				VDocument.SetMethod("bool get_intrinsic_dimensions(vector2 &out, float &out)", &Engine::GUI::IElementDocument::GetIntrinsicDimensions);
				VDocument.SetMethod("bool is_point_within_element(const vector2 &in)", &Engine::GUI::IElementDocument::IsPointWithinElement);
				VDocument.SetMethod("bool is_visible() const", &Engine::GUI::IElementDocument::IsVisible);
				VDocument.SetMethod("float get_zindex() const", &Engine::GUI::IElementDocument::GetZIndex);
				VDocument.SetMethod("bool set_property(const string &in, const string &in)", &Engine::GUI::IElementDocument::SetProperty);
				VDocument.SetMethod("void remove_property(const string &in)", &Engine::GUI::IElementDocument::RemoveProperty);
				VDocument.SetMethod("string get_property(const string &in) const", &Engine::GUI::IElementDocument::GetProperty);
				VDocument.SetMethod("string get_local_property(const string &in) const", &Engine::GUI::IElementDocument::GetLocalProperty);
				VDocument.SetMethod("float resolve_numeric_property(const string &in) const", &Engine::GUI::IElementDocument::ResolveNumericProperty);
				VDocument.SetMethod("vector2 get_containing_block() const", &Engine::GUI::IElementDocument::GetContainingBlock);
				VDocument.SetMethod("ui_position get_position() const", &Engine::GUI::IElementDocument::GetPosition);
				VDocument.SetMethod("ui_float get_float() const", &Engine::GUI::IElementDocument::GetFloat);
				VDocument.SetMethod("ui_display get_display() const", &Engine::GUI::IElementDocument::GetDisplay);
				VDocument.SetMethod("float get_line_height() const", &Engine::GUI::IElementDocument::GetLineHeight);
				VDocument.SetMethod("bool project(vector2 &out) const", &Engine::GUI::IElementDocument::Project);
				VDocument.SetMethod("bool animate(const string &in, const string &in, float, ui_timing_func, ui_timing_dir, int = -1, bool = true, float = 0)", &Engine::GUI::IElementDocument::Animate);
				VDocument.SetMethod("bool add_animation_key(const string &in, const string &in, float, ui_timing_func, ui_timing_dir)", &Engine::GUI::IElementDocument::AddAnimationKey);
				VDocument.SetMethod("void set_pseudo_Class(const string &in, bool)", &Engine::GUI::IElementDocument::SetPseudoClass);
				VDocument.SetMethod("bool is_pseudo_class_set(const string &in) const", &Engine::GUI::IElementDocument::IsPseudoClassSet);
				VDocument.SetMethod("void set_attribute(const string &in, const string &in)", &Engine::GUI::IElementDocument::SetAttribute);
				VDocument.SetMethod("string get_attribute(const string &in) const", &Engine::GUI::IElementDocument::GetAttribute);
				VDocument.SetMethod("bool has_attribute(const string &in) const", &Engine::GUI::IElementDocument::HasAttribute);
				VDocument.SetMethod("void remove_attribute(const string &in)", &Engine::GUI::IElementDocument::RemoveAttribute);
				VDocument.SetMethod("ui_element get_focus_leaf_node()", &Engine::GUI::IElementDocument::GetFocusLeafNode);
				VDocument.SetMethod("string get_tag_name() const", &Engine::GUI::IElementDocument::GetTagName);
				VDocument.SetMethod("string get_id() const", &Engine::GUI::IElementDocument::GetId);
				VDocument.SetMethod("float get_absolute_left()", &Engine::GUI::IElementDocument::GetAbsoluteLeft);
				VDocument.SetMethod("float get_absolute_top()", &Engine::GUI::IElementDocument::GetAbsoluteTop);
				VDocument.SetMethod("float get_client_left()", &Engine::GUI::IElementDocument::GetClientLeft);
				VDocument.SetMethod("float get_client_top()", &Engine::GUI::IElementDocument::GetClientTop);
				VDocument.SetMethod("float get_client_width()", &Engine::GUI::IElementDocument::GetClientWidth);
				VDocument.SetMethod("float get_client_height()", &Engine::GUI::IElementDocument::GetClientHeight);
				VDocument.SetMethod("ui_element get_offset_parent()", &Engine::GUI::IElementDocument::GetOffsetParent);
				VDocument.SetMethod("float get_offset_left()", &Engine::GUI::IElementDocument::GetOffsetLeft);
				VDocument.SetMethod("float get_offset_top()", &Engine::GUI::IElementDocument::GetOffsetTop);
				VDocument.SetMethod("float get_offset_width()", &Engine::GUI::IElementDocument::GetOffsetWidth);
				VDocument.SetMethod("float get_offset_height()", &Engine::GUI::IElementDocument::GetOffsetHeight);
				VDocument.SetMethod("float get_scroll_left()", &Engine::GUI::IElementDocument::GetScrollLeft);
				VDocument.SetMethod("void set_scroll_left(float)", &Engine::GUI::IElementDocument::SetScrollLeft);
				VDocument.SetMethod("float get_scroll_top()", &Engine::GUI::IElementDocument::GetScrollTop);
				VDocument.SetMethod("void set_scroll_top(float)", &Engine::GUI::IElementDocument::SetScrollTop);
				VDocument.SetMethod("float get_scroll_width()", &Engine::GUI::IElementDocument::GetScrollWidth);
				VDocument.SetMethod("float get_scroll_height()", &Engine::GUI::IElementDocument::GetScrollHeight);
				VDocument.SetMethod("ui_document get_owner_document() const", &Engine::GUI::IElementDocument::GetOwnerDocument);
				VDocument.SetMethod("ui_element get_parent_node() const", &Engine::GUI::IElementDocument::GetParentNode);
				VDocument.SetMethod("ui_element get_next_sibling() const", &Engine::GUI::IElementDocument::GetNextSibling);
				VDocument.SetMethod("ui_element get_previous_sibling() const", &Engine::GUI::IElementDocument::GetPreviousSibling);
				VDocument.SetMethod("ui_element get_first_child() const", &Engine::GUI::IElementDocument::GetFirstChild);
				VDocument.SetMethod("ui_element get_last_child() const", &Engine::GUI::IElementDocument::GetLastChild);
				VDocument.SetMethod("ui_element get_child(int) const", &Engine::GUI::IElementDocument::GetChild);
				VDocument.SetMethod("int get_num_children(bool = false) const", &Engine::GUI::IElementDocument::GetNumChildren);
				VDocument.SetMethod<Engine::GUI::IElement, void, std::string&>("void get_inner_html(string &out) const", &Engine::GUI::IElementDocument::GetInnerHTML);
				VDocument.SetMethod<Engine::GUI::IElement, std::string>("string get_inner_html() const", &Engine::GUI::IElementDocument::GetInnerHTML);
				VDocument.SetMethod("void set_inner_html(const string &in)", &Engine::GUI::IElementDocument::SetInnerHTML);
				VDocument.SetMethod("bool is_focused()", &Engine::GUI::IElementDocument::IsFocused);
				VDocument.SetMethod("bool is_hovered()", &Engine::GUI::IElementDocument::IsHovered);
				VDocument.SetMethod("bool is_active()", &Engine::GUI::IElementDocument::IsActive);
				VDocument.SetMethod("bool is_checked()", &Engine::GUI::IElementDocument::IsChecked);
				VDocument.SetMethod("bool focus()", &Engine::GUI::IElementDocument::Focus);
				VDocument.SetMethod("void blur()", &Engine::GUI::IElementDocument::Blur);
				VDocument.SetMethod("void click()", &Engine::GUI::IElementDocument::Click);
				VDocument.SetMethod("void add_event_listener(const string &in, ui_listener@+, bool = false)", &Engine::GUI::IElementDocument::AddEventListener);
				VDocument.SetMethod("void remove_event_listener(const string &in, ui_listener@+, bool = false)", &Engine::GUI::IElementDocument::RemoveEventListener);
				VDocument.SetMethodEx("bool dispatch_event(const string &in, schema@+)", &IElementDocumentDispatchEvent);
				VDocument.SetMethod("void scroll_into_view(bool = true)", &Engine::GUI::IElementDocument::ScrollIntoView);
				VDocument.SetMethod("ui_element append_child(const ui_element &in, bool = true)", &Engine::GUI::IElementDocument::AppendChild);
				VDocument.SetMethod("ui_element insert_before(const ui_element &in, const ui_element &in)", &Engine::GUI::IElementDocument::InsertBefore);
				VDocument.SetMethod("ui_element replace_child(const ui_element &in, const ui_element &in)", &Engine::GUI::IElementDocument::ReplaceChild);
				VDocument.SetMethod("ui_element remove_child(const ui_element &in)", &Engine::GUI::IElementDocument::RemoveChild);
				VDocument.SetMethod("bool has_child_nodes() const", &Engine::GUI::IElementDocument::HasChildNodes);
				VDocument.SetMethod("ui_element get_element_by_id(const string &in)", &Engine::GUI::IElementDocument::GetElementById);
				VDocument.SetMethodEx("array<ui_element>@ query_selector_all(const string &in)", &IElementDocumentQuerySelectorAll);
				VDocument.SetMethod("bool cast_form_color(vector4 &out, bool)", &Engine::GUI::IElementDocument::CastFormColor);
				VDocument.SetMethod("bool cast_form_string(string &out)", &Engine::GUI::IElementDocument::CastFormString);
				VDocument.SetMethod("bool cast_form_pointer(uptr@ &out)", &Engine::GUI::IElementDocument::CastFormPointer);
				VDocument.SetMethod("bool cast_form_int32(int &out)", &Engine::GUI::IElementDocument::CastFormInt32);
				VDocument.SetMethod("bool cast_form_uint32(uint &out)", &Engine::GUI::IElementDocument::CastFormUInt32);
				VDocument.SetMethod("bool cast_form_flag32(uint &out, uint)", &Engine::GUI::IElementDocument::CastFormFlag32);
				VDocument.SetMethod("bool cast_form_int64(int64 &out)", &Engine::GUI::IElementDocument::CastFormInt64);
				VDocument.SetMethod("bool cast_form_uint64(uint64 &out)", &Engine::GUI::IElementDocument::CastFormUInt64);
				VDocument.SetMethod("bool cast_form_flag64(uint64 &out, uint64)", &Engine::GUI::IElementDocument::CastFormFlag64);
				VDocument.SetMethod<Engine::GUI::IElement, bool, float*>("bool cast_form_float(float &out)", &Engine::GUI::IElementDocument::CastFormFloat);
				VDocument.SetMethod<Engine::GUI::IElement, bool, float*, float>("bool cast_form_float(float &out, float)", &Engine::GUI::IElementDocument::CastFormFloat);
				VDocument.SetMethod("bool cast_form_double(double &out)", &Engine::GUI::IElementDocument::CastFormDouble);
				VDocument.SetMethod("bool cast_form_boolean(bool &out)", &Engine::GUI::IElementDocument::CastFormBoolean);
				VDocument.SetMethod("string get_form_name() const", &Engine::GUI::IElementDocument::GetFormName);
				VDocument.SetMethod("void set_form_name(const string &in)", &Engine::GUI::IElementDocument::SetFormName);
				VDocument.SetMethod("string get_form_value() const", &Engine::GUI::IElementDocument::GetFormValue);
				VDocument.SetMethod("void set_form_value(const string &in)", &Engine::GUI::IElementDocument::SetFormValue);
				VDocument.SetMethod("bool is_form_disabled() const", &Engine::GUI::IElementDocument::IsFormDisabled);
				VDocument.SetMethod("void set_form_disabled(bool)", &Engine::GUI::IElementDocument::SetFormDisabled);
				VDocument.SetMethod("uptr@ get_element() const", &Engine::GUI::IElementDocument::GetElement);
				VDocument.SetMethod("bool is_valid() const", &Engine::GUI::IElementDocument::IsValid);
				VDocument.SetMethod("void set_title(const string &in)", &Engine::GUI::IElementDocument::SetTitle);
				VDocument.SetMethod("void pull_to_front()", &Engine::GUI::IElementDocument::PullToFront);
				VDocument.SetMethod("void push_to_back()", &Engine::GUI::IElementDocument::PushToBack);
				VDocument.SetMethod("void show(ui_modal_flag = ui_modal_flag::None, ui_focus_flag = ui_focus_flag::Auto)", &Engine::GUI::IElementDocument::Show);
				VDocument.SetMethod("void hide()", &Engine::GUI::IElementDocument::Hide);
				VDocument.SetMethod("void close()", &Engine::GUI::IElementDocument::Close);
				VDocument.SetMethod("string get_title() const", &Engine::GUI::IElementDocument::GetTitle);
				VDocument.SetMethod("string get_source_url() const", &Engine::GUI::IElementDocument::GetSourceURL);
				VDocument.SetMethod("ui_element create_element(const string &in)", &Engine::GUI::IElementDocument::CreateElement);
				VDocument.SetMethod("bool is_modal() const", &Engine::GUI::IElementDocument::IsModal);;
				VDocument.SetMethod("uptr@ get_element_document() const", &Engine::GUI::IElementDocument::GetElementDocument);

				return true;
			}
			bool Registry::LoadUiModel(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();
				Register.SetFunctionDef("void ui_data_event(ui_event &in, array<variant>@+)");

				VMRefClass VModel = Register.SetClassUnmanaged<Engine::GUI::DataModel>("ui_model");
				VModel.SetMethodEx("bool set(const string &in, schema@+)", &DataModelSet);
				VModel.SetMethodEx("bool set_var(const string &in, const variant &in)", &DataModelSetVar);
				VModel.SetMethodEx("bool set_string(const string &in, const string &in)", &DataModelSetString);
				VModel.SetMethodEx("bool set_integer(const string &in, int64)", &DataModelSetInteger);
				VModel.SetMethodEx("bool set_float(const string &in, float)", &DataModelSetFloat);
				VModel.SetMethodEx("bool set_double(const string &in, double)", &DataModelSetDouble);
				VModel.SetMethodEx("bool set_boolean(const string &in, bool)", &DataModelSetBoolean);
				VModel.SetMethodEx("bool set_pointer(const string &in, uptr@)", &DataModelSetPointer);
				VModel.SetMethodEx("bool set_callback(const string &in, ui_data_event@)", &DataModelSetCallback);
				VModel.SetMethodEx("schema@+ get(const string &in)", &DataModelGet);
				VModel.SetMethod("string get_string(const string &in)", &Engine::GUI::DataModel::GetString);
				VModel.SetMethod("int64 get_integer(const string &in)", &Engine::GUI::DataModel::GetInteger);
				VModel.SetMethod("float get_float(const string &in)", &Engine::GUI::DataModel::GetFloat);
				VModel.SetMethod("double get_double(const string &in)", &Engine::GUI::DataModel::GetDouble);
				VModel.SetMethod("bool get_boolean(const string &in)", &Engine::GUI::DataModel::GetBoolean);
				VModel.SetMethod("uptr@ get_pointer(const string &in)", &Engine::GUI::DataModel::GetPointer);
				VModel.SetMethod("bool has_changed(const string &in)", &Engine::GUI::DataModel::HasChanged);
				VModel.SetMethod("void change(const string &in)", &Engine::GUI::DataModel::Change);
				VModel.SetMethod("bool isValid() const", &Engine::GUI::DataModel::IsValid);
				VModel.SetMethodStatic("vector4 to_color4(const string &in)", &Engine::GUI::IVariant::ToColor4);
				VModel.SetMethodStatic("string from_color4(const vector4 &in, bool)", &Engine::GUI::IVariant::FromColor4);
				VModel.SetMethodStatic("vector4 to_color3(const string &in)", &Engine::GUI::IVariant::ToColor3);
				VModel.SetMethodStatic("string from_color3(const vector4 &in, bool)", &Engine::GUI::IVariant::ToColor3);
				VModel.SetMethodStatic("int get_vector_type(const string &in)", &Engine::GUI::IVariant::GetVectorType);
				VModel.SetMethodStatic("vector4 to_vector4(const string &in)", &Engine::GUI::IVariant::ToVector4);
				VModel.SetMethodStatic("string from_vector4(const vector4 &in)", &Engine::GUI::IVariant::FromVector4);
				VModel.SetMethodStatic("vector3 to_vector3(const string &in)", &Engine::GUI::IVariant::ToVector3);
				VModel.SetMethodStatic("string from_vector3(const vector3 &in)", &Engine::GUI::IVariant::FromVector3);
				VModel.SetMethodStatic("vector2 to_vector2(const string &in)", &Engine::GUI::IVariant::ToVector2);
				VModel.SetMethodStatic("string from_vector2(const vector2 &in)", &Engine::GUI::IVariant::FromVector2);

				return true;
			}
			bool Registry::LoadUiContext(VMManager* Engine)
			{
				ED_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();
				VMRefClass VContext = Register.SetClassUnmanaged<Engine::GUI::Context>("gui_context");
				VContext.SetMethod("void set_documents_base_tag(const string &in)", &Engine::GUI::Context::SetDocumentsBaseTag);
				VContext.SetMethod("void clear_styles()", &Engine::GUI::Context::ClearStyles);
				VContext.SetMethod("bool clear_documents()", &Engine::GUI::Context::ClearDocuments);
				VContext.SetMethod<Engine::GUI::Context, bool, const std::string&>("bool initialize(const string &in)", &Engine::GUI::Context::Initialize);
				VContext.SetMethod("bool is_input_focused()", &Engine::GUI::Context::IsInputFocused);
				VContext.SetMethod("bool is_loading()", &Engine::GUI::Context::IsLoading);
				VContext.SetMethod("bool replace_html(const string &in, const string &in, int = 0)", &Engine::GUI::Context::ReplaceHTML);
				VContext.SetMethod("bool remove_data_model(const string &in)", &Engine::GUI::Context::RemoveDataModel);
				VContext.SetMethod("bool remove_data_models()", &Engine::GUI::Context::RemoveDataModels);
				VContext.SetMethod("uptr@ get_context()", &Engine::GUI::Context::GetContext);
				VContext.SetMethod("vector2 get_dimensions() const", &Engine::GUI::Context::GetDimensions);
				VContext.SetMethod("string get_documents_base_tag() const", &Engine::GUI::Context::GetDocumentsBaseTag);
				VContext.SetMethod("void set_density_independent_pixel_ratio(float)", &Engine::GUI::Context::GetDensityIndependentPixelRatio);
				VContext.SetMethod("float get_density_independent_pixel_ratio() const", &Engine::GUI::Context::GetDensityIndependentPixelRatio);
				VContext.SetMethod("void enable_mouse_cursor(bool)", &Engine::GUI::Context::EnableMouseCursor);
				VContext.SetMethod("ui_document eval_html(const string &in, int = 0)", &Engine::GUI::Context::EvalHTML);
				VContext.SetMethod("ui_document add_css(const string &in, int = 0)", &Engine::GUI::Context::AddCSS);
				VContext.SetMethod("ui_document load_css(const string &in, int = 0)", &Engine::GUI::Context::LoadCSS);
				VContext.SetMethod("ui_document load_document(const string &in)", &Engine::GUI::Context::LoadDocument);
				VContext.SetMethod("ui_document add_document(const string &in)", &Engine::GUI::Context::AddDocument);
				VContext.SetMethod("ui_document add_document_empty(const string &in = \"body\")", &Engine::GUI::Context::AddDocumentEmpty);
				VContext.SetMethod<Engine::GUI::Context, Engine::GUI::IElementDocument, const std::string&>("ui_document get_document(const string &in)", &Engine::GUI::Context::GetDocument);
				VContext.SetMethod<Engine::GUI::Context, Engine::GUI::IElementDocument, int>("ui_document get_document(int)", &Engine::GUI::Context::GetDocument);
				VContext.SetMethod("int get_num_documents() const", &Engine::GUI::Context::GetNumDocuments);
				VContext.SetMethod("ui_element get_element_by_id(const string &in, int = 0)", &Engine::GUI::Context::GetElementById);
				VContext.SetMethod("ui_element get_hover_element()", &Engine::GUI::Context::GetHoverElement);
				VContext.SetMethod("ui_element get_focus_element()", &Engine::GUI::Context::GetFocusElement);
				VContext.SetMethod("ui_element get_root_element()", &Engine::GUI::Context::GetRootElement);
				VContext.SetMethodEx("ui_element get_element_at_point(const vector2 &in)", &ContextGetFocusElement);
				VContext.SetMethod("void pull_Document_to_front(const ui_document &in)", &Engine::GUI::Context::PullDocumentToFront);
				VContext.SetMethod("void push_document_to_back(const ui_document &in)", &Engine::GUI::Context::PushDocumentToBack);
				VContext.SetMethod("void unfocus_document(const ui_document &in)", &Engine::GUI::Context::UnfocusDocument);
				VContext.SetMethod("void add_event_listener(const string &in, ui_listener@+, bool = false)", &Engine::GUI::Context::AddEventListener);
				VContext.SetMethod("void remove_event_listener(const string &in, ui_listener@+, bool = false)", &Engine::GUI::Context::RemoveEventListener);
				VContext.SetMethod("bool is_mouse_interacting()", &Engine::GUI::Context::IsMouseInteracting);
				VContext.SetMethod("bool get_active_clip_region(vector2 &out, vector2 &out)", &Engine::GUI::Context::GetActiveClipRegion);
				VContext.SetMethod("void set_active_clip_region(const vector2 &in, const vector2 &in)", &Engine::GUI::Context::SetActiveClipRegion);
				VContext.SetMethod("ui_model@ set_model(const string &in)", &Engine::GUI::Context::SetDataModel);
				VContext.SetMethod("ui_model@ get_model(const string &in)", &Engine::GUI::Context::GetDataModel);

				return true;
			}
			bool Registry::Release()
			{
				StringFactory::Free();
				ED_DELETE(Mapping, Names);
				Names = nullptr;
				return false;
			}
		}
	}
}