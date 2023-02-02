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
#define TYPENAME_THREAD "thread"
#define TYPENAME_REF "ref"
#define TYPENAME_WEAKREF "weak_ref"
#define TYPENAME_CONSTWEAKREF "const_weak_ref"
#define TYPENAME_SCHEMA "schema"
#define TYPENAME_DECIMAL "decimal"
#define TYPENAME_VARIANT "variant"
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
			TH_ASSERT_V(Cache.size() == 0, "some strings are still in use");
		}
		const void* GetStringConstant(const char* Buffer, asUINT Length)
		{
			TH_ASSERT(Buffer != nullptr, nullptr, "buffer must not be null");

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
			TH_ASSERT(Source != nullptr, asERROR, "source must not be null");

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
			TH_ASSERT(Source != nullptr, asERROR, "source must not be null");
			if (Length != nullptr)
				*Length = (as_size_t)reinterpret_cast<const std::string*>(Source)->length();

			if (Buffer != nullptr)
				memcpy(Buffer, reinterpret_cast<const std::string*>(Source)->c_str(), reinterpret_cast<const std::string*>(Source)->length());

			return asSUCCESS;
		}

	public:
		static StringFactory* Get()
		{
			if (!Base)
				Base = TH_NEW(StringFactory);

			return Base;
		}
		static void Free()
		{
			if (Base != nullptr && Base->Cache.empty())
				TH_DELETE(StringFactory, Base);
		}
	};
	StringFactory* StringFactory::Base = nullptr;
}

namespace Tomahawk
{
	namespace Script
	{
		namespace Bindings
		{
			Core::Mapping<std::unordered_map<uint64_t, std::pair<std::string, int>>>* Names = nullptr;
			uint64_t TypeCache::Set(uint64_t Id, const std::string& Name)
			{
				TH_ASSERT(Id > 0 && !Name.empty(), 0, "id should be greater than zero and name should not be empty");

				using Map = Core::Mapping<std::unordered_map<uint64_t, std::pair<std::string, int>>>;
				if (!Names)
					Names = TH_NEW(Map);

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
					TH_ASSERT(Engine != nullptr, -1, "engine should be set");
					It->second.second = Engine->Global().GetTypeIdByDecl(It->second.first.c_str());
				}

				return It->second.second;
			}

			void String::Construct(std::string* Current)
			{
				TH_ASSERT_V(Current != nullptr, "Current should be set");
				new(Current) std::string();
			}
			void String::CopyConstruct(const std::string& Other, std::string* Current)
			{
				TH_ASSERT_V(Current != nullptr, "Current should be set");
				new(Current) std::string(Other);
			}
			void String::Destruct(std::string* Current)
			{
				TH_ASSERT_V(Current != nullptr, "Current should be set");
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
				return Current + Stream.str();
			}
			std::string String::AddInt641(as_int64_t Value, const std::string& Current)
			{
				std::ostringstream Stream;
				Stream << Value;
				return Stream.str() + Current;
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
			char* String::CharAt(unsigned int Value, std::string& Current)
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
			int String::FindFirst(const std::string& Needle, as_size_t Start, const std::string& Current)
			{
				return (int)Current.find(Needle, (size_t)Start);
			}
			int String::FindFirstOf(const std::string& Needle, as_size_t Start, const std::string& Current)
			{
				return (int)Current.find_first_of(Needle, (size_t)Start);
			}
			int String::FindLastOf(const std::string& Needle, as_size_t Start, const std::string& Current)
			{
				return (int)Current.find_last_of(Needle, (size_t)Start);
			}
			int String::FindFirstNotOf(const std::string& Needle, as_size_t Start, const std::string& Current)
			{
				return (int)Current.find_first_not_of(Needle, (size_t)Start);
			}
			int String::FindLastNotOf(const std::string& Needle, as_size_t Start, const std::string& Current)
			{
				return (int)Current.find_last_not_of(Needle, (size_t)Start);
			}
			int String::FindLast(const std::string& Needle, int Start, const std::string& Current)
			{
				return (int)Current.rfind(Needle, (size_t)Start);
			}
			void String::Insert(unsigned int Offset, const std::string& Other, std::string& Current)
			{
				Current.insert(Offset, Other);
			}
			void String::Erase(unsigned int Offset, int Count, std::string& Current)
			{
				Current.erase(Offset, (size_t)(Count < 0 ? std::string::npos : Count));
			}
			as_size_t String::Length(const std::string& Current)
			{
				return (as_size_t)Current.length();
			}
			void String::Resize(as_size_t Size, std::string& Current)
			{
				Current.resize(Size);
			}
			std::string String::Replace(const std::string& A, const std::string& B, uint64_t Offset, const std::string& Base)
			{
				return Tomahawk::Core::Parser(Base).Replace(A, B, (size_t)Offset).R();
			}
			as_int64_t String::IntStore(const std::string& Value, as_size_t Base, as_size_t* ByteCount)
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
					*ByteCount = as_size_t(size_t(End - Value.c_str()));

				if (Sign)
					Result = -Result;

				return Result;
			}
			as_uint64_t String::UIntStore(const std::string& Value, as_size_t Base, as_size_t* ByteCount)
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
					*ByteCount = as_size_t(size_t(End - Value.c_str()));

				return Result;
			}
			double String::FloatStore(const std::string& Value, as_size_t* ByteCount)
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
					*ByteCount = as_size_t(size_t(End - Value.c_str()));

				return res;
			}
			std::string String::Sub(as_size_t Start, int Count, const std::string& Current)
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
				return Tomahawk::Core::Parser(Symbol).ToLower().R();
			}
			std::string String::ToUpper(const std::string& Symbol)
			{
				return Tomahawk::Core::Parser(Symbol).ToUpper().R();
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

			Mutex::Mutex() : Ref(1)
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

			float Math::FpFromIEEE(as_size_t Raw)
			{
				return *reinterpret_cast<float*>(&Raw);
			}
			as_size_t Math::FpToIEEE(float Value)
			{
				return *reinterpret_cast<as_size_t*>(&Value);
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

			Any::Any(VMCManager* _Engine)
			{
				Engine = _Engine;
				RefCount = 1;
				GCFlag = false;
				Value.TypeId = 0;
				Value.ValueInt = 0;

				Engine->NotifyGarbageCollectorOfNewObject(this, Engine->GetTypeInfoByName(TYPENAME_ANY));
			}
			Any::Any(void* Ref, int RefTypeId, VMCManager* _Engine)
			{
				Engine = _Engine;
				RefCount = 1;
				GCFlag = false;
				Value.TypeId = 0;
				Value.ValueInt = 0;

				Engine->NotifyGarbageCollectorOfNewObject(this, Engine->GetTypeInfoByName(TYPENAME_ANY));
				Store(Ref, RefTypeId);
			}
			Any::Any(const Any& Other)
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
			Any::~Any()
			{
				FreeObject();
			}
			Any& Any::operator=(const Any& Other)
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

			Array& Array::operator=(const Array& Other)
			{
				if (&Other != this && Other.GetArrayObjectType() == GetArrayObjectType())
				{
					Resize(Other.Buffer->NumElements);
					CopyBuffer(Buffer, Other.Buffer);
				}

				return *this;
			}
			Array::Array(VMCTypeInfo* Info, void* BufferPtr)
			{
				TH_ASSERT_V(Info && std::string(Info->GetName()) == TYPENAME_ARRAY, "array type is invalid");
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

				as_size_t length = *(as_size_t*)BufferPtr;
				if (!CheckMaxSize(length))
					return;

				if ((Info->GetSubTypeId() & asTYPEID_MASK_OBJECT) == 0)
				{
					CreateBuffer(&Buffer, length);
					if (length > 0)
						memcpy(At(0), (((as_size_t*)BufferPtr) + 1), (size_t)length * (size_t)ElementSize);
				}
				else if (Info->GetSubTypeId() & asTYPEID_OBJHANDLE)
				{
					CreateBuffer(&Buffer, length);
					if (length > 0)
						memcpy(At(0), (((as_size_t*)BufferPtr) + 1), (size_t)length * (size_t)ElementSize);

					memset((((as_size_t*)BufferPtr) + 1), 0, (size_t)length * (size_t)ElementSize);
				}
				else if (Info->GetSubType()->GetFlags() & asOBJ_REF)
				{
					SubTypeId |= asTYPEID_OBJHANDLE;
					CreateBuffer(&Buffer, length);
					SubTypeId &= ~asTYPEID_OBJHANDLE;

					if (length > 0)
						memcpy(Buffer->Data, (((as_size_t*)BufferPtr) + 1), (size_t)length * (size_t)ElementSize);

					memset((((as_size_t*)BufferPtr) + 1), 0, (size_t)length * (size_t)ElementSize);
				}
				else
				{
					CreateBuffer(&Buffer, length);
					for (as_size_t n = 0; n < length; n++)
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
			Array::Array(as_size_t length, VMCTypeInfo* Info)
			{
				TH_ASSERT_V(Info && std::string(Info->GetName()) == TYPENAME_ARRAY, "array type is invalid");
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
			Array::Array(const Array& Other)
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
			Array::Array(as_size_t Length, void* DefaultValue, VMCTypeInfo* Info)
			{
				TH_ASSERT_V(Info && std::string(Info->GetName()) == TYPENAME_ARRAY, "array type is invalid");
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

				for (as_size_t i = 0; i < GetSize(); i++)
					SetValue(i, DefaultValue);
			}
			void Array::SetValue(as_size_t Index, void* Value)
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
			as_size_t Array::GetSize() const
			{
				return Buffer->NumElements;
			}
			bool Array::IsEmpty() const
			{
				return Buffer->NumElements == 0;
			}
			void Array::Reserve(as_size_t MaxElements)
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
			void Array::Resize(as_size_t NumElements)
			{
				if (!CheckMaxSize(NumElements))
					return;

				Resize((int)NumElements - (int)Buffer->NumElements, (as_size_t)-1);
			}
			void Array::RemoveRange(as_size_t Start, as_size_t Count)
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
				memmove(Buffer->Data + Start * (as_size_t)ElementSize, Buffer->Data + (Start + Count) * (as_size_t)ElementSize, (size_t)(Buffer->NumElements - Start - Count) * (size_t)ElementSize);
				Buffer->NumElements -= Count;
			}
			void Array::Resize(int Delta, as_size_t Where)
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
						memcpy(NewBuffer->Data + (Where + Delta) * (as_size_t)ElementSize, Buffer->Data + Where * (as_size_t)ElementSize, (size_t)(Buffer->NumElements - Where) * (size_t)ElementSize);

					Construct(NewBuffer, Where, Where + Delta);
					asFreeMem(Buffer);
					Buffer = NewBuffer;
				}
				else if (Delta < 0)
				{
					Destruct(Buffer, Where, Where - Delta);
					memmove(Buffer->Data + Where * (as_size_t)ElementSize, Buffer->Data + (Where - Delta) * (as_size_t)ElementSize, (size_t)(Buffer->NumElements - (Where - Delta)) * (size_t)ElementSize);
					Buffer->NumElements += Delta;
				}
				else
				{
					memmove(Buffer->Data + (Where + Delta) * (as_size_t)ElementSize, Buffer->Data + Where * (as_size_t)ElementSize, (size_t)(Buffer->NumElements - Where) * (size_t)ElementSize);
					Construct(Buffer, Where, Where + Delta);
					Buffer->NumElements += Delta;
				}
			}
			bool Array::CheckMaxSize(as_size_t NumElements)
			{
				as_size_t MaxSize = 0xFFFFFFFFul - sizeof(SBuffer) + 1;
				if (ElementSize > 0)
					MaxSize /= (as_size_t)ElementSize;

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
			void Array::InsertAt(as_size_t Index, void* Value)
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
			void Array::InsertAt(as_size_t Index, const Array& Array)
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

				as_size_t elements = Array.GetSize();
				Resize(elements, Index);
				if (&Array != this)
				{
					for (as_size_t i = 0; i < Array.GetSize(); i++)
					{
						void* Value = const_cast<void*>(Array.At(i));
						SetValue(Index + i, Value);
					}
				}
				else
				{
					for (as_size_t i = 0; i < Index; i++)
					{
						void* Value = const_cast<void*>(Array.At(i));
						SetValue(Index + i, Value);
					}

					for (as_size_t i = Index + elements, k = 0; i < Array.GetSize(); i++, k++)
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
			void Array::RemoveAt(as_size_t Index)
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
			const void* Array::At(as_size_t Index) const
			{
				if (Buffer == 0 || Index >= Buffer->NumElements)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Index out of bounds");
					return 0;
				}

				if ((SubTypeId & asTYPEID_MASK_OBJECT) && !(SubTypeId & asTYPEID_OBJHANDLE))
					return *(void**)(Buffer->Data + (as_size_t)ElementSize * Index);
				else
					return Buffer->Data + (as_size_t)ElementSize * Index;
			}
			void* Array::At(as_size_t Index)
			{
				return const_cast<void*>(const_cast<const Array*>(this)->At(Index));
			}
			void* Array::GetBuffer()
			{
				return Buffer->Data;
			}
			void Array::CreateBuffer(SBuffer** BufferPtr, as_size_t NumElements)
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
			void Array::Construct(SBuffer* BufferPtr, as_size_t Start, as_size_t End)
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
					void* D = (void*)(BufferPtr->Data + Start * (as_size_t)ElementSize);
					memset(D, 0, (size_t)(End - Start) * (size_t)ElementSize);
				}
			}
			void Array::Destruct(SBuffer* BufferPtr, as_size_t Start, as_size_t End)
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
				as_size_t Size = GetSize();
				if (Size >= 2)
				{
					unsigned char Temp[16];
					for (as_size_t i = 0; i < Size / 2; i++)
					{
						Copy(Temp, GetArrayItemPointer(i));
						Copy(GetArrayItemPointer(i), GetArrayItemPointer(Size - i - 1));
						Copy(GetArrayItemPointer(Size - i - 1), Temp);
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
				for (as_size_t n = 0; n < GetSize(); n++)
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
			int Array::FindByRef(as_size_t StartAt, void* RefPtr) const
			{
				as_size_t Size = GetSize();
				if (SubTypeId & asTYPEID_OBJHANDLE)
				{
					RefPtr = *(void**)RefPtr;
					for (as_size_t i = StartAt; i < Size; i++)
					{
						if (*(void**)At(i) == RefPtr)
							return i;
					}
				}
				else
				{
					for (as_size_t i = StartAt; i < Size; i++)
					{
						if (At(i) == RefPtr)
							return i;
					}
				}

				return -1;
			}
			int Array::Find(void* Value) const
			{
				return Find(0, Value);
			}
			int Array::Find(as_size_t StartAt, void* Value) const
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
				as_size_t Size = GetSize();
				for (as_size_t i = StartAt; i < Size; i++)
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
			void Array::SortAsc(as_size_t StartAt, as_size_t Count)
			{
				Sort(StartAt, Count, true);
			}
			void Array::SortDesc()
			{
				Sort(0, GetSize(), false);
			}
			void Array::SortDesc(as_size_t StartAt, as_size_t Count)
			{
				Sort(StartAt, Count, false);
			}
			void Array::Sort(as_size_t StartAt, as_size_t Count, bool Asc)
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

				int Start = StartAt;
				int End = StartAt + Count;

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
			void Array::Sort(asIScriptFunction* Function, as_size_t StartAt, as_size_t Count)
			{
				if (Count < 2)
					return;

				as_size_t Start = StartAt;
				as_size_t End = as_uint64_t(StartAt) + as_uint64_t(Count) >= (as_uint64_t(1) << 32) ? 0xFFFFFFFF : StartAt + Count;
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

				for (as_size_t i = Start + 1; i < End; i++)
				{
					Copy(Swap, GetArrayItemPointer(i));
					as_size_t j = i - 1;

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
							Copy(GetArrayItemPointer(j + 1), GetArrayItemPointer(j));
							j--;
						}
						else
							break;
					}

					Copy(GetArrayItemPointer(j + 1), Swap);
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
						int Count = Dest->NumElements > Src->NumElements ? Src->NumElements : Dest->NumElements;
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
						int Count = Dest->NumElements > Src->NumElements ? Src->NumElements : Dest->NumElements;
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
					for (as_size_t i = 0; i < SubType->GetMethodCount(); i++)
					{
						asIScriptFunction* Function = SubType->GetMethodByIndex(i);
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
						for (as_size_t n = 0; n < Buffer->NumElements; n++)
						{
							if (D[n])
								Engine->GCEnumCallback(D[n]);
						}
					}
					else if ((SubType->GetFlags() & asOBJ_VALUE) && (SubType->GetFlags() & asOBJ_GC))
					{
						for (as_size_t n = 0; n < Buffer->NumElements; n++)
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
			Array* Array::Create(VMCTypeInfo* Info, as_size_t Length)
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
			Array* Array::Create(VMCTypeInfo* Info, as_size_t length, void* DefaultValue)
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
				return Array::Create(Info, as_size_t(0));
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
						for (as_size_t n = 0; n < SubType->GetBehaviourCount(); n++)
						{
							asEBehaviours Beh;
							asIScriptFunction* Func = SubType->GetBehaviourByIndex(n, &Beh);
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
							for (as_size_t n = 0; n < SubType->GetFactoryCount(); n++)
							{
								asIScriptFunction* Func = SubType->GetFactoryByIndex(n);
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

			MapKey::MapKey()
			{
				ValueObj = 0;
				TypeId = 0;
			}
			MapKey::MapKey(VMCManager* Engine, void* Value, int _TypeId)
			{
				ValueObj = 0;
				TypeId = 0;
				Set(Engine, Value, _TypeId);
			}
			MapKey::~MapKey()
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

			Map::LocalIterator::LocalIterator(const Map& Dict, InternalMap::const_iterator It) : It(It), Dict(Dict)
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

			Map::Map(VMCManager* _Engine)
			{
				Init(_Engine);
			}
			Map::Map(unsigned char* buffer)
			{
				VMCContext* Context = asGetActiveContext();
				Init(Context->GetEngine());

				Map::SCache& Cache = *reinterpret_cast<Map::SCache*>(Engine->GetUserData(MAP_CACHE));
				bool keyAsRef = Cache.KeyType->GetFlags() & asOBJ_REF ? true : false;

				as_size_t length = *(as_size_t*)buffer;
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
			Map::Map(const Map& Other)
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
			Map::~Map()
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
			Map& Map::operator =(const Map& Other)
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
			as_size_t Map::GetSize() const
			{
				return as_size_t(Dict.size());
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

				Array* Array = Array::Create(Info, as_size_t(Dict.size()));
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
				TH_DELETE(SCache, Cache);
			}
			void Map::Setup(VMCManager* Engine)
			{
				Map::SCache* Cache = reinterpret_cast<Map::SCache*>(Engine->GetUserData(MAP_CACHE));
				if (Cache == 0)
				{
					Cache = TH_NEW(Map::SCache);
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

			Grid::Grid(VMCTypeInfo* Info, void* BufferPtr)
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

				as_size_t Height = *(as_size_t*)BufferPtr;
				as_size_t Width = Height ? *(as_size_t*)((char*)(BufferPtr)+4) : 0;

				if (!CheckMaxSize(Width, Height))
					return;

				BufferPtr = (as_size_t*)(BufferPtr)+1;
				if ((Info->GetSubTypeId() & asTYPEID_MASK_OBJECT) == 0)
				{
					CreateBuffer(&Buffer, Width, Height);
					for (as_size_t y = 0; y < Height; y++)
					{
						BufferPtr = (as_size_t*)(BufferPtr)+1;
						if (Width > 0)
							memcpy(At(0, y), BufferPtr, (size_t)Width * (size_t)ElementSize);

						BufferPtr = (char*)(BufferPtr)+Width * (as_size_t)ElementSize;
						if (asPWORD(BufferPtr) & 0x3)
							BufferPtr = (char*)(BufferPtr)+4 - (asPWORD(BufferPtr) & 0x3);
					}
				}
				else if (Info->GetSubTypeId() & asTYPEID_OBJHANDLE)
				{
					CreateBuffer(&Buffer, Width, Height);
					for (as_size_t y = 0; y < Height; y++)
					{
						BufferPtr = (as_size_t*)(BufferPtr)+1;
						if (Width > 0)
							memcpy(At(0, y), BufferPtr, (size_t)Width * (size_t)ElementSize);

						memset(BufferPtr, 0, (size_t)Width * (size_t)ElementSize);
						BufferPtr = (char*)(BufferPtr)+Width * (as_size_t)ElementSize;

						if (asPWORD(BufferPtr) & 0x3)
							BufferPtr = (char*)(BufferPtr)+4 - (asPWORD(BufferPtr) & 0x3);
					}
				}
				else if (Info->GetSubType()->GetFlags() & asOBJ_REF)
				{
					SubTypeId |= asTYPEID_OBJHANDLE;
					CreateBuffer(&Buffer, Width, Height);
					SubTypeId &= ~asTYPEID_OBJHANDLE;

					for (as_size_t y = 0; y < Height; y++)
					{
						BufferPtr = (as_size_t*)(BufferPtr)+1;
						if (Width > 0)
							memcpy(At(0, y), BufferPtr, (size_t)Width * (size_t)ElementSize);

						memset(BufferPtr, 0, (size_t)Width * (size_t)ElementSize);
						BufferPtr = (char*)(BufferPtr)+Width * (as_size_t)ElementSize;

						if (asPWORD(BufferPtr) & 0x3)
							BufferPtr = (char*)(BufferPtr)+4 - (asPWORD(BufferPtr) & 0x3);
					}
				}
				else
				{
					CreateBuffer(&Buffer, Width, Height);

					VMCTypeInfo* SubType = Info->GetSubType();
					as_size_t SubTypeSize = SubType->GetSize();
					for (as_size_t y = 0; y < Height; y++)
					{
						BufferPtr = (as_size_t*)(BufferPtr)+1;
						for (as_size_t x = 0; x < Width; x++)
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
			Grid::Grid(as_size_t Width, as_size_t Height, VMCTypeInfo* Info)
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
			void Grid::Resize(as_size_t Width, as_size_t Height)
			{
				if (!CheckMaxSize(Width, Height))
					return;

				SBuffer* TempBuffer = 0;
				CreateBuffer(&TempBuffer, Width, Height);
				if (TempBuffer == 0)
					return;

				if (Buffer)
				{
					as_size_t W = Width > Buffer->Width ? Buffer->Width : Width;
					as_size_t H = Height > Buffer->Height ? Buffer->Height : Height;
					for (as_size_t y = 0; y < H; y++)
					{
						for (as_size_t x = 0; x < W; x++)
							SetValue(TempBuffer, x, y, At(Buffer, x, y));
					}

					DeleteBuffer(Buffer);
				}

				Buffer = TempBuffer;
			}
			Grid::Grid(as_size_t Width, as_size_t Height, void* DefaultValue, VMCTypeInfo* Info)
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

				for (as_size_t y = 0; y < GetHeight(); y++)
				{
					for (as_size_t x = 0; x < GetWidth(); x++)
						SetValue(x, y, DefaultValue);
				}
			}
			void Grid::SetValue(as_size_t x, as_size_t y, void* Value)
			{
				SetValue(Buffer, x, y, Value);
			}
			void Grid::SetValue(SBuffer* BufferPtr, as_size_t x, as_size_t y, void* Value)
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
			as_size_t Grid::GetWidth() const
			{
				if (Buffer)
					return Buffer->Width;

				return 0;
			}
			as_size_t Grid::GetHeight() const
			{
				if (Buffer)
					return Buffer->Height;

				return 0;
			}
			bool Grid::CheckMaxSize(as_size_t Width, as_size_t Height)
			{
				as_size_t MaxSize = 0xFFFFFFFFul - sizeof(SBuffer) + 1;
				if (ElementSize > 0)
					MaxSize /= (as_size_t)ElementSize;

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
			void* Grid::At(as_size_t x, as_size_t y)
			{
				return At(Buffer, x, y);
			}
			void* Grid::At(SBuffer* BufferPtr, as_size_t x, as_size_t y)
			{
				if (BufferPtr == 0 || x >= BufferPtr->Width || y >= BufferPtr->Height)
				{
					VMCContext* Context = asGetActiveContext();
					if (Context)
						Context->SetException("Index out of bounds");
					return 0;
				}

				as_size_t Index = x + y * BufferPtr->Width;
				if ((SubTypeId & asTYPEID_MASK_OBJECT) && !(SubTypeId & asTYPEID_OBJHANDLE))
					return *(void**)(BufferPtr->Data + (as_size_t)ElementSize * Index);

				return BufferPtr->Data + (as_size_t)ElementSize * Index;
			}
			const void* Grid::At(as_size_t x, as_size_t y) const
			{
				return const_cast<Grid*>(this)->At(const_cast<SBuffer*>(Buffer), x, y);
			}
			void Grid::CreateBuffer(SBuffer** BufferPtr, as_size_t W, as_size_t H)
			{
				as_size_t NumElements = W * H;
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
				TH_ASSERT_V(BufferPtr, "buffer should be set");
				Destruct(BufferPtr);
				asFreeMem(BufferPtr);
			}
			void Grid::Construct(SBuffer* BufferPtr)
			{
				TH_ASSERT_V(BufferPtr, "buffer should be set");
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
				TH_ASSERT_V(BufferPtr, "buffer should be set");
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
					as_size_t NumElements = Buffer->Width * Buffer->Height;
					void** D = (void**)Buffer->Data;
					VMCTypeInfo* SubType = Engine->GetTypeInfoById(SubTypeId);

					if ((SubType->GetFlags() & asOBJ_REF))
					{
						for (as_size_t n = 0; n < NumElements; n++)
						{
							if (D[n])
								Engine->GCEnumCallback(D[n]);
						}
					}
					else if ((SubType->GetFlags() & asOBJ_VALUE) && (SubType->GetFlags() & asOBJ_GC))
					{
						for (as_size_t n = 0; n < NumElements; n++)
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
			Grid* Grid::Create(VMCTypeInfo* Info, as_size_t W, as_size_t H)
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
			Grid* Grid::Create(VMCTypeInfo* Info, as_size_t W, as_size_t H, void* DefaultValue)
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
						for (as_size_t n = 0; n < SubType->GetBehaviourCount(); n++)
						{
							asEBehaviours Beh;
							asIScriptFunction* Function = SubType->GetBehaviourByIndex(n, &Beh);
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
							for (as_size_t n = 0; n < SubType->GetFactoryCount(); n++)
							{
								asIScriptFunction* Function = SubType->GetFactoryByIndex(n);
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

			Ref::Ref() : Type(0), Pointer(nullptr)
			{
			}
			Ref::Ref(const Ref& Other) : Type(Other.Type), Pointer(Other.Pointer)
			{
				AddRefHandle();
			}
			Ref::Ref(void* RefPtr, VMCTypeInfo* _Type) : Type(_Type), Pointer(RefPtr)
			{
				AddRefHandle();
			}
			Ref::Ref(void* RefPtr, int TypeId) : Type(0), Pointer(nullptr)
			{
				Assign(RefPtr, TypeId);
			}
			Ref::~Ref()
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
			Ref& Ref::operator =(const Ref& Other)
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

				TH_ASSERT_V(TypeId & asTYPEID_OBJHANDLE, "type should be object handle");
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

			WeakRef::WeakRef(VMCTypeInfo* _Type)
			{
				Ref = 0;
				Type = _Type;
				Type->AddRef();
				WeakRefFlag = 0;
			}
			WeakRef::WeakRef(const WeakRef& Other)
			{
				Ref = Other.Ref;
				Type = Other.Type;
				Type->AddRef();
				WeakRefFlag = Other.WeakRefFlag;
				if (WeakRefFlag)
					WeakRefFlag->AddRef();
			}
			WeakRef::WeakRef(void* RefPtr, VMCTypeInfo* _Type)
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
			WeakRef::~WeakRef()
			{
				if (Type)
					Type->Release();

				if (WeakRefFlag)
					WeakRefFlag->Release();
			}
			WeakRef& WeakRef::operator =(const WeakRef& Other)
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

				as_size_t cnt = SubType->GetBehaviourCount();
				for (as_size_t n = 0; n < cnt; n++)
				{
					asEBehaviours Beh;
					SubType->GetBehaviourByIndex(n, &Beh);
					if (Beh == asBEHAVE_GET_WEAKREF_FLAG)
						return true;
				}

				Info->GetEngine()->WriteMessage(TYPENAME_WEAKREF, 0, 0, asMSGTYPE_ERROR, "The subtype doesn't support weak references");
				return false;
			}

			Complex::Complex()
			{
				R = 0;
				I = 0;
			}
			Complex::Complex(const Complex& Other)
			{
				R = Other.R;
				I = Other.I;
			}
			Complex::Complex(float _R, float _I)
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
			Complex& Complex::operator=(const Complex& Other)
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

			Thread::Thread(VMCManager* Engine, VMCFunction* Func) : Function(Func), Manager(VMManager::Get(Engine)), Context(nullptr), GCFlag(false), Ref(1)
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
				Context->TryExecute(Function, [this](Script::VMContext* Context)
				{
					Context->SetArgObject(0, this);
					Context->SetUserData(this, ContextUD);
				}).Await([this](int&&)
				{
					Context->SetUserData(nullptr, ContextUD);
					this->Mutex.lock();

					if (!Context->IsSuspended())
						TH_CLEAR(Context);

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
						TH_DEBUG("[vm] join thread %s", Core::OS::Process::GetThreadId(Procedure.get_id()).c_str());
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
					TH_DEBUG("[vm] join thread %s", Core::OS::Process::GetThreadId(Procedure.get_id()).c_str());
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

					TH_DEBUG("[vm] join thread %s", Core::OS::Process::GetThreadId(Procedure.get_id()).c_str());
					Procedure.join();
				}

				AddRef();
				Procedure = std::thread(&Thread::Routine, this);
				TH_DEBUG("[vm] spawn thread %s", Core::OS::Process::GetThreadId(Procedure.get_id()).c_str());
				Mutex.unlock();

				return true;
			}
			void Thread::ReleaseReferences(VMCManager*)
			{
				if (Join() >= 0)
					TH_ERR("[memerr] thread was forced to join");

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

				TH_CLEAR(Context);
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
			uint64_t Thread::GetThreadId()
			{
				return (uint64_t)std::hash<std::thread::id>()(std::this_thread::get_id());
			}
			int Thread::ContextUD = 550;
			int Thread::EngineListUD = 551;

			Promise::Promise(VMCContext* _Base, bool IsRef) : Context(VMContext::Get(_Base)), Future(nullptr), Ref(1), Flag(false), Pending(false)
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
				TH_ASSERT(Context != nullptr, std::string(), "context should be set");

				std::string Result;
				switch (Context->GetState())
				{
					case Tomahawk::Script::VMRuntime::FINISHED:
						Result = "FIN";
						break;
					case Tomahawk::Script::VMRuntime::SUSPENDED:
						Result = "SUSP";
						break;
					case Tomahawk::Script::VMRuntime::ABORTED:
						Result = "ABRT";
						break;
					case Tomahawk::Script::VMRuntime::EXCEPTION:
						Result = "EXCE";
						break;
					case Tomahawk::Script::VMRuntime::PREPARED:
						Result = "PREP";
						break;
					case Tomahawk::Script::VMRuntime::ACTIVE:
						Result = "ACTV";
						break;
					case Tomahawk::Script::VMRuntime::ERR:
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

				TH_ERR("[vm] no active context for stack tracing");
			}

			uint64_t VariantGetSize(Core::Variant& Base)
			{
				return (uint64_t)Base.GetSize();
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
						TH_WARN("[memerr] use nullptr instead of null for initialization lists");
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
			Core::Schema* SchemaGetIndexOffset(Core::Schema* Base, uint64_t Offset)
			{
				return Base->Get((size_t)Offset);
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
			uint64_t SchemaGetSize(Core::Schema* Base)
			{
				return (uint64_t)Base->Size();
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

			Format::Format()
			{
			}
			Format::Format(unsigned char* Buffer)
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
							TH_WARN("[memerr] use nullptr instead of null for initialization lists");

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
					Context->TryExecute(Callback, [Event, &Data](VMContext* Context)
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

			ModelListener::ModelListener(VMCFunction* NewCallback) : Engine::GUI::Listener(Bind(NewCallback)), Source(NewCallback), Context(nullptr)
			{
			}
			ModelListener::ModelListener(const std::string& FunctionName) : Engine::GUI::Listener(FunctionName), Source(nullptr), Context(nullptr)
			{
			}
			ModelListener::~ModelListener()
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
					Context->TryExecute(Source, [Event](VMContext* Context)
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

			bool Registry::LoadPointer(VMManager* Manager)
			{
				TH_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
				VMCManager* Engine = Manager->GetEngine();
				return Engine->RegisterObjectType("pointer", 0, asOBJ_REF | asOBJ_NOCOUNT) >= 0;
			}
			bool Registry::LoadAny(VMManager* Manager)
			{
				TH_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
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
				TH_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
				VMCManager* Engine = Manager->GetEngine();
				Engine->SetTypeInfoUserDataCleanupCallback(Array::CleanupTypeInfoCache, ARRAY_CACHE);
				Engine->RegisterObjectType("array<class T>", 0, asOBJ_REF | asOBJ_GC | asOBJ_TEMPLATE);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(Array::TemplateCallback), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_FACTORY, "array<T>@ f(int&in)", asFUNCTIONPR(Array::Create, (VMCTypeInfo*), Array*), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_FACTORY, "array<T>@ f(int&in, uint length) explicit", asFUNCTIONPR(Array::Create, (VMCTypeInfo*, as_size_t), Array*), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_FACTORY, "array<T>@ f(int&in, uint length, const T &in Value)", asFUNCTIONPR(Array::Create, (VMCTypeInfo*, as_size_t, void*), Array*), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_LIST_FACTORY, "array<T>@ f(int&in Type, int&in InitList) {repeat T}", asFUNCTIONPR(Array::Create, (VMCTypeInfo*, void*), Array*), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(Array, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("array<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(Array, Release), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "T &opIndex(uint Index)", asMETHODPR(Array, At, (as_size_t), void*), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "const T &opIndex(uint Index) const", asMETHODPR(Array, At, (as_size_t) const, const void*), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "array<T> &opAssign(const array<T>&in)", asMETHOD(Array, operator=), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void insertAt(uint Index, const T&in Value)", asMETHODPR(Array, InsertAt, (as_size_t, void*), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void insertAt(uint Index, const array<T>& Array)", asMETHODPR(Array, InsertAt, (as_size_t, const Array&), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void insertLast(const T&in Value)", asMETHOD(Array, InsertLast), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void push(const T&in Value)", asMETHOD(Array, InsertLast), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void pop()", asMETHOD(Array, RemoveLast), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void removeAt(uint Index)", asMETHOD(Array, RemoveAt), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void removeLast()", asMETHOD(Array, RemoveLast), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void removeRange(uint Start, uint Count)", asMETHOD(Array, RemoveRange), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "uint size() const", asMETHOD(Array, GetSize), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void reserve(uint length)", asMETHOD(Array, Reserve), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void resize(uint length)", asMETHODPR(Array, Resize, (as_size_t), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void sortAsc()", asMETHODPR(Array, SortAsc, (), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void sortAsc(uint StartAt, uint Count)", asMETHODPR(Array, SortAsc, (as_size_t, as_size_t), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void sortDesc()", asMETHODPR(Array, SortDesc, (), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void sortDesc(uint StartAt, uint Count)", asMETHODPR(Array, SortDesc, (as_size_t, as_size_t), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "void reverse()", asMETHOD(Array, Reverse), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "int find(const T&in if_handle_then_const Value) const", asMETHODPR(Array, Find, (void*) const, int), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "int find(uint StartAt, const T&in if_handle_then_const Value) const", asMETHODPR(Array, Find, (as_size_t, void*) const, int), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "int findByRef(const T&in if_handle_then_const Value) const", asMETHODPR(Array, FindByRef, (void*) const, int), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "int findByRef(uint StartAt, const T&in if_handle_then_const Value) const", asMETHODPR(Array, FindByRef, (as_size_t, void*) const, int), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "bool opEquals(const array<T>&in) const", asMETHOD(Array, operator==), asCALL_THISCALL);
				Engine->RegisterObjectMethod("array<T>", "bool empty() const", asMETHOD(Array, IsEmpty), asCALL_THISCALL);
				Engine->RegisterFuncdef("bool array<T>::less(const T&in if_handle_then_const a, const T&in if_handle_then_const b)");
				Engine->RegisterObjectMethod("array<T>", "void sort(const less &in, uint StartAt = 0, uint Count = uint(-1))", asMETHODPR(Array, Sort, (asIScriptFunction*, as_size_t, as_size_t), void), asCALL_THISCALL);
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
				TH_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
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
				TH_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
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
				Engine->RegisterObjectMethod("map", "uint size() const", asMETHOD(Map, GetSize), asCALL_THISCALL);
				Engine->RegisterObjectMethod("map", "bool delete(const string &in)", asMETHOD(Map, Delete), asCALL_THISCALL);
				Engine->RegisterObjectMethod("map", "void deleteAll()", asMETHOD(Map, DeleteAll), asCALL_THISCALL);
				Engine->RegisterObjectMethod("map", "array<string>@ getKeys() const", asMETHOD(Map, GetKeys), asCALL_THISCALL);
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
				Engine->RegisterObjectBehaviour("grid<T>", asBEHAVE_FACTORY, "grid<T>@ f(int&in, uint, uint)", asFUNCTIONPR(Grid::Create, (VMCTypeInfo*, as_size_t, as_size_t), Grid*), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("grid<T>", asBEHAVE_FACTORY, "grid<T>@ f(int&in, uint, uint, const T &in)", asFUNCTIONPR(Grid::Create, (VMCTypeInfo*, as_size_t, as_size_t, void*), Grid*), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("grid<T>", asBEHAVE_LIST_FACTORY, "grid<T>@ f(int&in Type, int&in InitList) {repeat {repeat_same T}}", asFUNCTIONPR(Grid::Create, (VMCTypeInfo*, void*), Grid*), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("grid<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(Grid, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("grid<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(Grid, Release), asCALL_THISCALL);
				Engine->RegisterObjectMethod("grid<T>", "T &opIndex(uint, uint)", asMETHODPR(Grid, At, (as_size_t, as_size_t), void*), asCALL_THISCALL);
				Engine->RegisterObjectMethod("grid<T>", "const T &opIndex(uint, uint) const", asMETHODPR(Grid, At, (as_size_t, as_size_t) const, const void*), asCALL_THISCALL);
				Engine->RegisterObjectMethod("grid<T>", "void resize(uint, uint)", asMETHODPR(Grid, Resize, (as_size_t, as_size_t), void), asCALL_THISCALL);
				Engine->RegisterObjectMethod("grid<T>", "uint width() const", asMETHOD(Grid, GetWidth), asCALL_THISCALL);
				Engine->RegisterObjectMethod("grid<T>", "uint height() const", asMETHOD(Grid, GetHeight), asCALL_THISCALL);
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

				Engine->RegisterGlobalFunction("float fpFromIEEE(uint)", asFUNCTIONPR(Math::FpFromIEEE, (as_size_t), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("uint fpToIEEE(float)", asFUNCTIONPR(Math::FpToIEEE, (float), as_size_t), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double fpFromIEEE(uint64)", asFUNCTIONPR(Math::FpFromIEEE, (as_uint64_t), double), asCALL_CDECL);
				Engine->RegisterGlobalFunction("uint64 fpToIEEE(double)", asFUNCTIONPR(Math::FpToIEEE, (double), as_uint64_t), asCALL_CDECL);
				Engine->RegisterGlobalFunction("bool closeTo(float, float, float = 0.00001f)", asFUNCTIONPR(Math::CloseTo, (float, float, float), bool), asCALL_CDECL);
				Engine->RegisterGlobalFunction("bool closeTo(double, double, double = 0.0000000001)", asFUNCTIONPR(Math::CloseTo, (double, double, double), bool), asCALL_CDECL);
	#if !defined(_WIN32_WCE)
				Engine->RegisterGlobalFunction("float cos(float)", asFUNCTIONPR(cosf, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float sin(float)", asFUNCTIONPR(sinf, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float tan(float)", asFUNCTIONPR(tanf, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float acos(float)", asFUNCTIONPR(acosf, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float asin(float)", asFUNCTIONPR(asinf, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float atan(float)", asFUNCTIONPR(atanf, (float), float), asCALL_CDECL);
				Engine->RegisterGlobalFunction("float atan2(float,float)", asFUNCTIONPR(atan2f, (float, float), float), asCALL_CDECL);
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
				Engine->RegisterGlobalFunction("double atan2(double,double)", asFUNCTIONPR(atan2, (double, double), double), asCALL_CDECL);
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
				Engine->RegisterObjectMethod("string", "uint size() const", asFUNCTION(String::Length), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "void resize(uint)", asFUNCTION(String::Resize), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "bool empty() const", asFUNCTION(String::IsEmpty), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "uint8 &opIndex(uint)", asFUNCTION(String::CharAt), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "const uint8 &opIndex(uint) const", asFUNCTION(String::CharAt), asCALL_CDECL_OBJLAST);
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
				Engine->RegisterObjectMethod("string", "string opAdd(int64) const", asFUNCTION(String::AddInt641), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "string opAdd_r(int64) const", asFUNCTION(String::AddInt642), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAssign(uint64)", asFUNCTION(String::AssignUInt64To), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAddAssign(uint64)", asFUNCTION(String::AddAssignUInt64To), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string opAdd(uint64) const", asFUNCTION(String::AddUInt641), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "string opAdd_r(uint64) const", asFUNCTION(String::AddUInt642), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAssign(bool)", asFUNCTION(String::AssignBoolTo), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string &opAddAssign(bool)", asFUNCTION(String::AddAssignBoolTo), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string opAdd(bool) const", asFUNCTION(String::AddBool1), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "string opAdd_r(bool) const", asFUNCTION(String::AddBool2), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string needle(uint = 0, int = -1) const", asFUNCTION(String::Sub), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "int findFirst(const string &in, uint = 0) const", asFUNCTION(String::FindFirst), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "int findFirstOf(const string &in, uint = 0) const", asFUNCTION(String::FindFirstOf), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "int findFirstNotOf(const string &in, uint = 0) const", asFUNCTION(String::FindFirstNotOf), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "int findLast(const string &in, int = -1) const", asFUNCTION(String::FindLast), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "int findLastOf(const string &in, int = -1) const", asFUNCTION(String::FindLastOf), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "int findLastNotOf(const string &in, int = -1) const", asFUNCTION(String::FindLastNotOf), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "void insert(uint, const string &in)", asFUNCTION(String::Insert), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "void erase(uint, int Count = -1)", asFUNCTION(String::Erase), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string replace(const string &in, const string &in, uint64 = 0)", asFUNCTION(String::Replace), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "array<string>@ split(const string &in) const", asFUNCTION(String::Split), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string toLower() const", asFUNCTION(String::ToLower), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string toUpper() const", asFUNCTION(String::ToUpper), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "string reverse() const", asFUNCTION(String::Reverse), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "pointer@ getPointer() const", asFUNCTION(String::ToPtr), asCALL_CDECL_OBJLAST);
				Engine->RegisterObjectMethod("string", "pointer@ opImplCast()", asFUNCTION(String::ToPtr), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "pointer@ opImplCast() const", asFUNCTION(String::ToPtr), asCALL_CDECL_OBJFIRST);
				Engine->RegisterObjectMethod("string", "pointer@ opImplConv() const", asFUNCTION(String::ToPtr), asCALL_CDECL_OBJFIRST);
				Engine->RegisterGlobalFunction("int64 toInt(const string &in, uint = 10, uint &out = 0)", asFUNCTION(String::IntStore), asCALL_CDECL);
				Engine->RegisterGlobalFunction("uint64 toUInt(const string &in, uint = 10, uint &out = 0)", asFUNCTION(String::UIntStore), asCALL_CDECL);
				Engine->RegisterGlobalFunction("double toFloat(const string &in, uint &out = 0)", asFUNCTION(String::FloatStore), asCALL_CDECL);
				Engine->RegisterGlobalFunction("uint8 toChar(const string &in)", asFUNCTION(String::ToChar), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string toString(const array<string> &in, const string &in)", asFUNCTION(String::Join), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string toString(int8)", asFUNCTION(String::ToInt8), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string toString(int16)", asFUNCTION(String::ToInt16), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string toString(int)", asFUNCTION(String::ToInt), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string toString(int64)", asFUNCTION(String::ToInt64), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string toString(uint8)", asFUNCTION(String::ToUInt8), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string toString(uint16)", asFUNCTION(String::ToUInt16), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string toString(uint)", asFUNCTION(String::ToUInt), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string toString(uint64)", asFUNCTION(String::ToUInt64), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string toString(float)", asFUNCTION(String::ToFloat), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string toString(double)", asFUNCTION(String::ToDouble), asCALL_CDECL);

				return true;
			}
			bool Registry::LoadException(VMManager* Manager)
			{
				TH_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
				VMCManager* Engine = Manager->GetEngine();
				Engine->RegisterGlobalFunction("void throw(const string &in = \"\")", asFUNCTION(Exception::Throw), asCALL_CDECL);
				Engine->RegisterGlobalFunction("string getException()", asFUNCTION(Exception::GetException), asCALL_CDECL);
				return true;
			}
			bool Registry::LoadMutex(VMManager* Manager)
			{
				TH_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
				VMCManager* Engine = Manager->GetEngine();
				Engine->RegisterObjectType("mutex", sizeof(Mutex), asOBJ_REF);
				Engine->RegisterObjectBehaviour("mutex", asBEHAVE_FACTORY, "mutex@ f()", asFUNCTION(Mutex::Factory), asCALL_CDECL);
				Engine->RegisterObjectBehaviour("mutex", asBEHAVE_ADDREF, "void f()", asMETHOD(Mutex, AddRef), asCALL_THISCALL);
				Engine->RegisterObjectBehaviour("mutex", asBEHAVE_RELEASE, "void f()", asMETHOD(Mutex, Release), asCALL_THISCALL);
				Engine->RegisterObjectMethod("mutex", "bool tryLock()", asMETHOD(Mutex, TryLock), asCALL_THISCALL);
				Engine->RegisterObjectMethod("mutex", "void lock()", asMETHOD(Mutex, Lock), asCALL_THISCALL);
				Engine->RegisterObjectMethod("mutex", "void unlock()", asMETHOD(Mutex, Unlock), asCALL_THISCALL);
				return true;
			}
			bool Registry::LoadThread(VMManager* Manager)
			{
				TH_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
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
				Engine->RegisterObjectMethod("thread", "bool isActive()", asMETHOD(Thread, IsActive), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "bool invoke()", asMETHOD(Thread, Start), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "void suspend()", asMETHOD(Thread, Suspend), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "void resume()", asMETHOD(Thread, Resume), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "void push(const ?&in)", asMETHOD(Thread, Push), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "bool pop(?&out)", asMETHODPR(Thread, Pop, (void*, int), bool), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "bool pop(?&out, uint64)", asMETHODPR(Thread, Pop, (void*, int, uint64_t), bool), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "int join(uint64)", asMETHODPR(Thread, Join, (uint64_t), int), asCALL_THISCALL);
				Engine->RegisterObjectMethod("thread", "int join()", asMETHODPR(Thread, Join, (), int), asCALL_THISCALL);
				Engine->RegisterGlobalFunction("thread@+ getThread()", asFUNCTION(Thread::GetThread), asCALL_CDECL);
				Engine->RegisterGlobalFunction("uint64 getThreadId()", asFUNCTION(Thread::GetThreadId), asCALL_CDECL);
				Engine->RegisterGlobalFunction("void sleep(uint64)", asFUNCTION(Thread::ThreadSleep), asCALL_CDECL);
				return true;
			}
			bool Registry::LoadRandom(VMManager* Manager)
			{
				TH_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
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
				TH_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();
				VMRefClass VFormat = Register.SetClassUnmanaged<Format>("format");
				VFormat.SetUnmanagedConstructor<Format>("format@ f()");
				VFormat.SetUnmanagedConstructorList<Format>("format@ f(int &in) {repeat ?}");
				VFormat.SetMethodStatic("string toJSON(const ? &in)", &Format::JSON);

				return true;
			}
			bool Registry::LoadDecimal(VMManager* Engine)
			{
				TH_ASSERT(Engine != nullptr, false, "manager should be set");
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
				VDecimal.SetMethod("bool isNaN() const", &Core::Decimal::IsNaN);
				VDecimal.SetMethod("double toDouble() const", &Core::Decimal::ToDouble);
				VDecimal.SetMethod("int64 toInt64() const", &Core::Decimal::ToInt64);
				VDecimal.SetMethod("string toString() const", &Core::Decimal::ToString);
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
				VDecimal.SetMethodStatic("decimal NaN()", &Core::Decimal::NaN);

				return true;
			}
			bool Registry::LoadVariant(VMManager* Engine)
			{
				TH_ASSERT(Engine != nullptr, false, "manager should be set");
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
				VVariant.SetMethod("decimal toDecimal() const", &Core::Variant::GetDecimal);
				VVariant.SetMethod("string toString() const", &Core::Variant::GetBlob);
				VVariant.SetMethod("pointer@ toPointer() const", &Core::Variant::GetPointer);
				VVariant.SetMethod("int64 toInteger() const", &Core::Variant::GetInteger);
				VVariant.SetMethod("double toNumber() const", &Core::Variant::GetNumber);
				VVariant.SetMethod("bool toBoolean() const", &Core::Variant::GetBoolean);
				VVariant.SetMethod("var_type getType() const", &Core::Variant::GetType);
				VVariant.SetMethod("bool isObject() const", &Core::Variant::IsObject);
				VVariant.SetMethod("bool empty() const", &Core::Variant::IsEmpty);
				VVariant.SetMethodEx("uint64 size() const", &VariantGetSize);
				VVariant.SetOperatorEx(VMOpFunc::Equals, (uint32_t)VMOp::Const, "bool", "const variant &in", &VariantEquals);
				VVariant.SetOperatorEx(VMOpFunc::ImplCast, (uint32_t)VMOp::Const, "bool", "", &VariantImplCast);

				Engine->BeginNamespace("var");
				Register.SetFunction("variant auto_t(const string &in, bool = false)", &Core::Var::Auto);
				Register.SetFunction("variant null_t()", &Core::Var::Null);
				Register.SetFunction("variant undefined_t()", &Core::Var::Undefined);
				Register.SetFunction("variant object_t()", &Core::Var::Object);
				Register.SetFunction("variant array_t()", &Core::Var::Array);
				Register.SetFunction("variant pointer_t(pointer@)", &Core::Var::Pointer);
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
			bool Registry::LoadDateTime(VMManager* Engine)
			{
				TH_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();
				VMTypeClass VDateTime = Register.SetStructUnmanaged<Core::DateTime>("timestamp");
				VDateTime.SetConstructor<Core::DateTime>("void f()");
				VDateTime.SetConstructor<Core::DateTime, uint64_t>("void f(uint64)");
				VDateTime.SetConstructor<Core::DateTime, const Core::DateTime&>("void f(const timestamp &in)");
				VDateTime.SetMethod("string format(const string &in)", &Core::DateTime::Format);
				VDateTime.SetMethod("string date(const string &in)", &Core::DateTime::Date);
				VDateTime.SetMethod("string toISO8601()", &Core::DateTime::Iso8601);
				VDateTime.SetMethod("timestamp now()", &Core::DateTime::Now);
				VDateTime.SetMethod("timestamp fromNanoseconds(uint64)", &Core::DateTime::FromNanoseconds);
				VDateTime.SetMethod("timestamp fromMicroseconds(uint64)", &Core::DateTime::FromMicroseconds);
				VDateTime.SetMethod("timestamp fromMilliseconds(uint64)", &Core::DateTime::FromMilliseconds);
				VDateTime.SetMethod("timestamp fromSeconds(uint64)", &Core::DateTime::FromSeconds);
				VDateTime.SetMethod("timestamp fromMinutes(uint64)", &Core::DateTime::FromMinutes);
				VDateTime.SetMethod("timestamp fromHours(uint64)", &Core::DateTime::FromHours);
				VDateTime.SetMethod("timestamp fromDays(uint64)", &Core::DateTime::FromDays);
				VDateTime.SetMethod("timestamp fromWeeks(uint64)", &Core::DateTime::FromWeeks);
				VDateTime.SetMethod("timestamp fromMonths(uint64)", &Core::DateTime::FromMonths);
				VDateTime.SetMethod("timestamp fromYears(uint64)", &Core::DateTime::FromYears);
				VDateTime.SetMethod("timestamp& setDateSeconds(uint64, bool = true)", &Core::DateTime::SetDateSeconds);
				VDateTime.SetMethod("timestamp& setDateMinutes(uint64, bool = true)", &Core::DateTime::SetDateMinutes);
				VDateTime.SetMethod("timestamp& setDateHours(uint64, bool = true)", &Core::DateTime::SetDateHours);
				VDateTime.SetMethod("timestamp& setDateDay(uint64, bool = true)", &Core::DateTime::SetDateDay);
				VDateTime.SetMethod("timestamp& setDateWeek(uint64, bool = true)", &Core::DateTime::SetDateWeek);
				VDateTime.SetMethod("timestamp& setDateMonth(uint64, bool = true)", &Core::DateTime::SetDateMonth);
				VDateTime.SetMethod("timestamp& setDateYear(uint64, bool = true)", &Core::DateTime::SetDateYear);
				VDateTime.SetMethod("uint64 dateSecond()", &Core::DateTime::DateSecond);
				VDateTime.SetMethod("uint64 dateMinute()", &Core::DateTime::DateMinute);
				VDateTime.SetMethod("uint64 dateHour()", &Core::DateTime::DateHour);
				VDateTime.SetMethod("uint64 dateDay()", &Core::DateTime::DateDay);
				VDateTime.SetMethod("uint64 dateWeek()", &Core::DateTime::DateWeek);
				VDateTime.SetMethod("uint64 dateMonth()", &Core::DateTime::DateMonth);
				VDateTime.SetMethod("uint64 dateYear()", &Core::DateTime::DateYear);
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
				VDateTime.SetMethodStatic<std::string, int64_t>("string getGMT(int64)", &Core::DateTime::FetchWebDateGMT);
				VDateTime.SetMethodStatic<std::string, int64_t>("string getTime(int64)", &Core::DateTime::FetchWebDateTime);

				return true;
			}
			bool Registry::LoadConsole(VMManager* Engine)
			{
				TH_ASSERT(Engine != nullptr, false, "manager should be set");
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
				VConsole.SetMethod("void hide()", &Core::Console::Hide);
				VConsole.SetMethod("void show()", &Core::Console::Show);
				VConsole.SetMethod("void clear()", &Core::Console::Clear);
				VConsole.SetMethod("void flush()", &Core::Console::Flush);
				VConsole.SetMethod("void flushWrite()", &Core::Console::FlushWrite);
				VConsole.SetMethod("void setColoring(bool)", &Core::Console::SetColoring);
				VConsole.SetMethod("void colorBegin(std_color, std_color)", &Core::Console::ColorBegin);
				VConsole.SetMethod("void colorEnd()", &Core::Console::ColorEnd);
				VConsole.SetMethod("void captureTime()", &Core::Console::CaptureTime);
				VConsole.SetMethod("void writeLine(const string &in)", &Core::Console::sWriteLine);
				VConsole.SetMethod("void write(const string &in)", &Core::Console::sWrite);
				VConsole.SetMethod("double getCapturedTime()", &Core::Console::GetCapturedTime);
				VConsole.SetMethod("string read(uint64)", &Core::Console::Read);
				VConsole.SetMethodStatic("console@+ get()", &Core::Console::Get);
				VConsole.SetMethodEx("void trace(uint32 = 32)", &ConsoleTrace);
				VConsole.SetMethodEx("void writeLine(const string &in, format@+)", &Format::WriteLine);
				VConsole.SetMethodEx("void write(const string &in, format@+)", &Format::Write);

				return true;
			}
			bool Registry::LoadSchema(VMManager* Engine)
			{
				TH_ASSERT(Engine != nullptr, false, "manager should be set");
				VMRefClass VSchema = Engine->Global().SetClassUnmanaged<Core::Schema>("schema");
				VSchema.SetProperty<Core::Schema>("string key", &Core::Schema::Key);
				VSchema.SetProperty<Core::Schema>("variant value", &Core::Schema::Value);
				VSchema.SetUnmanagedConstructor<Core::Schema, const Core::Variant&>("schema@ f(const variant &in)");
				VSchema.SetUnmanagedConstructorListEx<Core::Schema>("schema@ f(int &in) { repeat { string, ? } }", &SchemaConstruct);
				VSchema.SetMethod<Core::Schema, Core::Variant, size_t>("variant getVar(uint) const", &Core::Schema::GetVar);
				VSchema.SetMethod<Core::Schema, Core::Variant, const std::string&>("variant getVar(const string &in) const", &Core::Schema::GetVar);
				VSchema.SetMethod("schema@+ getParent() const", &Core::Schema::GetParent);
				VSchema.SetMethod("schema@+ getAttribute(const string &in) const", &Core::Schema::GetAttribute);
				VSchema.SetMethod<Core::Schema, Core::Schema*, size_t>("schema@+ get(uint) const", &Core::Schema::Get);
				VSchema.SetMethod<Core::Schema, Core::Schema*, const std::string&, bool>("schema@+ get(const string &in, bool = false) const", &Core::Schema::Fetch);
				VSchema.SetMethod<Core::Schema, Core::Schema*, const std::string&>("schema@+ get(const string &in)", &Core::Schema::Set);
				VSchema.SetMethod<Core::Schema, Core::Schema*, const std::string&, const Core::Variant&>("schema@+ set(const string &in, const variant &in)", &Core::Schema::Set);
				VSchema.SetMethod<Core::Schema, Core::Schema*, const std::string&, const Core::Variant&>("schema@+ setAttribute(const string& in, const variant &in)", &Core::Schema::SetAttribute);
				VSchema.SetMethod<Core::Schema, Core::Schema*, const Core::Variant&>("schema@+ push(const variant &in)", &Core::Schema::Push);
				VSchema.SetMethod<Core::Schema, Core::Schema*, size_t>("schema@+ pop(uint)", &Core::Schema::Pop);
				VSchema.SetMethod<Core::Schema, Core::Schema*, const std::string&>("schema@+ pop(const string &in)", &Core::Schema::Pop);
				VSchema.SetMethod("schema@ copy() const", &Core::Schema::Copy);
				VSchema.SetMethod("bool has(const string &in) const", &Core::Schema::Has);
				VSchema.SetMethod("bool has64(const string &in, uint = 12) const", &Core::Schema::Has64);
				VSchema.SetMethod("bool empty() const", &Core::Schema::IsEmpty);
				VSchema.SetMethod("bool isAttribute() const", &Core::Schema::IsAttribute);
				VSchema.SetMethod("bool isSaved() const", &Core::Schema::IsAttribute);
				VSchema.SetMethod("string getName() const", &Core::Schema::GetName);
				VSchema.SetMethod("void join(schema@+)", &Core::Schema::Join);
				VSchema.SetMethod("void clear()", &Core::Schema::Clear);
				VSchema.SetMethod("void save()", &Core::Schema::Save);
				VSchema.SetMethodEx("variant firstVar() const", &SchemaFirstVar);
				VSchema.SetMethodEx("variant lastVar() const", &SchemaLastVar);
				VSchema.SetMethodEx("schema@+ first() const", &SchemaFirst);
				VSchema.SetMethodEx("schema@+ last() const", &SchemaLast);
				VSchema.SetMethodEx("schema@+ set(const string &in, schema@+)", &SchemaSet);
				VSchema.SetMethodEx("schema@+ push(schema@+)", &SchemaPush);
				VSchema.SetMethodEx("array<schema@>@ getCollection(const string &in, bool = false) const", &SchemaGetCollection);
				VSchema.SetMethodEx("array<schema@>@ getAttributes() const", &SchemaGetAttributes);
				VSchema.SetMethodEx("array<schema@>@ getChilds() const", &SchemaGetChilds);
				VSchema.SetMethodEx("map@ getNames() const", &SchemaGetNames);
				VSchema.SetMethodEx("uint64 size() const", &SchemaGetSize);
				VSchema.SetMethodEx("string toJSON() const", &SchemaToJSON);
				VSchema.SetMethodEx("string toXML() const", &SchemaToXML);
				VSchema.SetMethodEx("string toString() const", &SchemaToString);
				VSchema.SetMethodEx("string toBinary() const", &SchemaToBinary);
				VSchema.SetMethodEx("int64 toInteger() const", &SchemaToInteger);
				VSchema.SetMethodEx("double toNumber() const", &SchemaToNumber);
				VSchema.SetMethodEx("decimal toDecimal() const", &SchemaToDecimal);
				VSchema.SetMethodEx("bool toBoolean() const", &SchemaToBoolean);
				VSchema.SetMethodStatic("schema@ fromJSON(const string &in)", &SchemaFromJSON);
				VSchema.SetMethodStatic("schema@ fromXML(const string &in)", &SchemaFromXML);
				VSchema.SetMethodStatic("schema@ importJSON(const string &in)", &SchemaImport);
				VSchema.SetOperatorEx(VMOpFunc::Assign, (uint32_t)VMOp::Left, "schema@+", "const variant &in", &SchemaCopyAssign);
				VSchema.SetOperatorEx(VMOpFunc::Equals, (uint32_t)(VMOp::Left | VMOp::Const), "bool", "schema@+", &SchemaEquals);
				VSchema.SetOperatorEx(VMOpFunc::Index, (uint32_t)VMOp::Left, "schema@+", "const string &in", &SchemaGetIndex);
				VSchema.SetOperatorEx(VMOpFunc::Index, (uint32_t)VMOp::Left, "schema@+", "uint64", &SchemaGetIndexOffset);

				VMGlobal& Register = Engine->Global();
				Engine->BeginNamespace("var::set");
				Register.SetFunction("schema@ auto_t(const string &in, bool = false)", &Core::Var::Auto);
				Register.SetFunction("schema@ null_t()", &Core::Var::Set::Null);
				Register.SetFunction("schema@ undefined_t()", &Core::Var::Set::Undefined);
				Register.SetFunction("schema@ object_t()", &Core::Var::Set::Object);
				Register.SetFunction("schema@ array_t()", &Core::Var::Set::Array);
				Register.SetFunction("schema@ pointer_t(pointer@)", &Core::Var::Set::Pointer);
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
			bool Registry::LoadVertices(VMManager* Engine)
			{
				TH_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();
				VMTypeClass VVertex = Register.SetPod<Compute::Vertex>("vertex");
				VVertex.SetProperty<Compute::Vertex>("float positionX", &Compute::Vertex::PositionX);
				VVertex.SetProperty<Compute::Vertex>("float positionY", &Compute::Vertex::PositionY);
				VVertex.SetProperty<Compute::Vertex>("float positionZ", &Compute::Vertex::PositionZ);
				VVertex.SetProperty<Compute::Vertex>("float texCoordX", &Compute::Vertex::TexCoordX);
				VVertex.SetProperty<Compute::Vertex>("float texCoordY", &Compute::Vertex::TexCoordY);
				VVertex.SetProperty<Compute::Vertex>("float normalX", &Compute::Vertex::NormalX);
				VVertex.SetProperty<Compute::Vertex>("float normalY", &Compute::Vertex::NormalY);
				VVertex.SetProperty<Compute::Vertex>("float normalZ", &Compute::Vertex::NormalZ);
				VVertex.SetProperty<Compute::Vertex>("float tangentX", &Compute::Vertex::TangentX);
				VVertex.SetProperty<Compute::Vertex>("float tangentY", &Compute::Vertex::TangentY);
				VVertex.SetProperty<Compute::Vertex>("float tangentZ", &Compute::Vertex::TangentZ);
				VVertex.SetProperty<Compute::Vertex>("float bitangentX", &Compute::Vertex::BitangentX);
				VVertex.SetProperty<Compute::Vertex>("float bitangentY", &Compute::Vertex::BitangentY);
				VVertex.SetProperty<Compute::Vertex>("float bitangentZ", &Compute::Vertex::BitangentZ);

				VMTypeClass VSkinVertex = Register.SetPod<Compute::SkinVertex>("skin_vertex");
				VSkinVertex.SetProperty<Compute::SkinVertex>("float positionX", &Compute::SkinVertex::PositionX);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float positionY", &Compute::SkinVertex::PositionY);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float positionZ", &Compute::SkinVertex::PositionZ);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float texCoordX", &Compute::SkinVertex::TexCoordX);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float texCoordY", &Compute::SkinVertex::TexCoordY);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float normalX", &Compute::SkinVertex::NormalX);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float normalY", &Compute::SkinVertex::NormalY);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float normalZ", &Compute::SkinVertex::NormalZ);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float tangentX", &Compute::SkinVertex::TangentX);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float tangentY", &Compute::SkinVertex::TangentY);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float tangentZ", &Compute::SkinVertex::TangentZ);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float bitangentX", &Compute::SkinVertex::BitangentX);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float bitangentY", &Compute::SkinVertex::BitangentY);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float bitangentZ", &Compute::SkinVertex::BitangentZ);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float jointIndex0", &Compute::SkinVertex::JointIndex0);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float jointIndex1", &Compute::SkinVertex::JointIndex1);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float jointIndex2", &Compute::SkinVertex::JointIndex2);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float jointIndex3", &Compute::SkinVertex::JointIndex3);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float jointBias0", &Compute::SkinVertex::JointBias0);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float jointBias1", &Compute::SkinVertex::JointBias1);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float jointBias2", &Compute::SkinVertex::JointBias2);
				VSkinVertex.SetProperty<Compute::SkinVertex>("float jointBias3", &Compute::SkinVertex::JointBias3);

				VMTypeClass VShapeVertex = Register.SetPod<Compute::ShapeVertex>("shape_vertex");
				VShapeVertex.SetProperty<Compute::ShapeVertex>("float positionX", &Compute::ShapeVertex::PositionX);
				VShapeVertex.SetProperty<Compute::ShapeVertex>("float positionY", &Compute::ShapeVertex::PositionY);
				VShapeVertex.SetProperty<Compute::ShapeVertex>("float positionZ", &Compute::ShapeVertex::PositionZ);
				VShapeVertex.SetProperty<Compute::ShapeVertex>("float texCoordX", &Compute::ShapeVertex::TexCoordX);
				VShapeVertex.SetProperty<Compute::ShapeVertex>("float texCoordY", &Compute::ShapeVertex::TexCoordY);

				VMTypeClass VElementVertex = Register.SetPod<Compute::ElementVertex>("element_vertex");
				VElementVertex.SetProperty<Compute::ElementVertex>("float positionX", &Compute::ElementVertex::PositionX);
				VElementVertex.SetProperty<Compute::ElementVertex>("float positionY", &Compute::ElementVertex::PositionY);
				VElementVertex.SetProperty<Compute::ElementVertex>("float positionZ", &Compute::ElementVertex::PositionZ);
				VElementVertex.SetProperty<Compute::ElementVertex>("float velocityX", &Compute::ElementVertex::VelocityX);
				VElementVertex.SetProperty<Compute::ElementVertex>("float velocityY", &Compute::ElementVertex::VelocityY);
				VElementVertex.SetProperty<Compute::ElementVertex>("float velocityZ", &Compute::ElementVertex::VelocityZ);
				VElementVertex.SetProperty<Compute::ElementVertex>("float colorX", &Compute::ElementVertex::ColorX);
				VElementVertex.SetProperty<Compute::ElementVertex>("float colorY", &Compute::ElementVertex::ColorY);
				VElementVertex.SetProperty<Compute::ElementVertex>("float colorZ", &Compute::ElementVertex::ColorZ);
				VElementVertex.SetProperty<Compute::ElementVertex>("float colorW", &Compute::ElementVertex::ColorW);
				VElementVertex.SetProperty<Compute::ElementVertex>("float scale", &Compute::ElementVertex::Scale);
				VElementVertex.SetProperty<Compute::ElementVertex>("float rotation", &Compute::ElementVertex::Rotation);
				VElementVertex.SetProperty<Compute::ElementVertex>("float angular", &Compute::ElementVertex::Angular);

				return true;
			}
			bool Registry::LoadRectangle(VMManager* Engine)
			{
				TH_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();
				VMTypeClass VRectangle = Register.SetPod<Compute::Rectangle>("rectangle");
				VRectangle.SetProperty<Compute::Rectangle>("int64 left", &Compute::Rectangle::Left);
				VRectangle.SetProperty<Compute::Rectangle>("int64 top", &Compute::Rectangle::Top);
				VRectangle.SetProperty<Compute::Rectangle>("int64 right", &Compute::Rectangle::Right);
				VRectangle.SetProperty<Compute::Rectangle>("int64 bottom", &Compute::Rectangle::Bottom);

				return true;
			}
			bool Registry::LoadVector2(VMManager* Engine)
			{
				TH_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();
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
				VVector2.SetMethod("float lookAt(const vector2 &in) const", &Compute::Vector2::LookAt);
				VVector2.SetMethod("float cross(const vector2 &in) const", &Compute::Vector2::Cross);
				VVector2.SetMethod("vector2 direction(float) const", &Compute::Vector2::Direction);
				VVector2.SetMethod("vector2 inv() const", &Compute::Vector2::Inv);
				VVector2.SetMethod("vector2 invX() const", &Compute::Vector2::InvX);
				VVector2.SetMethod("vector2 invY() const", &Compute::Vector2::InvY);
				VVector2.SetMethod("vector2 normalize() const", &Compute::Vector2::Normalize);
				VVector2.SetMethod("vector2 normalizeSafe() const", &Compute::Vector2::sNormalize);
				VVector2.SetMethod("vector2 lerp(const vector2 &in, float) const", &Compute::Vector2::Lerp);
				VVector2.SetMethod("vector2 lerpStable(const vector2 &in, float) const", &Compute::Vector2::sLerp);
				VVector2.SetMethod("vector2 lerpAngular(const vector2 &in, float) const", &Compute::Vector2::aLerp);
				VVector2.SetMethod("vector2 lerpRound() const", &Compute::Vector2::rLerp);
				VVector2.SetMethod("vector2 abs() const", &Compute::Vector2::Abs);
				VVector2.SetMethod("vector2 radians() const", &Compute::Vector2::Radians);
				VVector2.SetMethod("vector2 degrees() const", &Compute::Vector2::Degrees);
				VVector2.SetMethod("vector2 xy() const", &Compute::Vector2::XY);
				VVector2.SetMethod<Compute::Vector2, Compute::Vector2, float>("vector2 mul(float) const", &Compute::Vector2::Mul);
				VVector2.SetMethod<Compute::Vector2, Compute::Vector2, float, float>("vector2 mul(float, float) const", &Compute::Vector2::Mul);
				VVector2.SetMethod<Compute::Vector2, Compute::Vector2, const Compute::Vector2&>("vector2 mul(const vector2 &in) const", &Compute::Vector2::Mul);
				VVector2.SetMethod("vector2 div(const vector2 &in) const", &Compute::Vector2::Div);
				VVector2.SetMethod("vector2 setX(float) const", &Compute::Vector2::SetX);
				VVector2.SetMethod("vector2 setY(float) const", &Compute::Vector2::SetY);
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
				Register.SetFunction("vector2 randomAbs()", &Compute::Vector2::RandomAbs);
				Register.SetFunction("vector2 one()", &Compute::Vector2::One);
				Register.SetFunction("vector2 zero()", &Compute::Vector2::Zero);
				Register.SetFunction("vector2 up()", &Compute::Vector2::Up);
				Register.SetFunction("vector2 down()", &Compute::Vector2::Down);
				Register.SetFunction("vector2 left()", &Compute::Vector2::Left);
				Register.SetFunction("vector2 right()", &Compute::Vector2::Right);
				Engine->EndNamespace();

				return true;
			}
			bool Registry::LoadVector3(VMManager* Engine)
			{
				TH_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();
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
				VVector3.SetMethod("vector3 lookAt(const vector3 &in) const", &Compute::Vector3::LookAt);
				VVector3.SetMethod("vector3 cross(const vector3 &in) const", &Compute::Vector3::Cross);
				VVector3.SetMethod("vector3 DirectionHorizontal() const", &Compute::Vector3::hDirection);
				VVector3.SetMethod("vector3 DirectionDepth() const", &Compute::Vector3::dDirection);
				VVector3.SetMethod("vector3 inv() const", &Compute::Vector3::Inv);
				VVector3.SetMethod("vector3 invX() const", &Compute::Vector3::InvX);
				VVector3.SetMethod("vector3 invY() const", &Compute::Vector3::InvY);
				VVector3.SetMethod("vector3 invZ() const", &Compute::Vector3::InvZ);
				VVector3.SetMethod("vector3 normalize() const", &Compute::Vector3::Normalize);
				VVector3.SetMethod("vector3 normalizeSafe() const", &Compute::Vector3::sNormalize);
				VVector3.SetMethod("vector3 lerp(const vector3 &in, float) const", &Compute::Vector3::Lerp);
				VVector3.SetMethod("vector3 lerpStable(const vector3 &in, float) const", &Compute::Vector3::sLerp);
				VVector3.SetMethod("vector3 lerpAngular(const vector3 &in, float) const", &Compute::Vector3::aLerp);
				VVector3.SetMethod("vector3 lerpRound() const", &Compute::Vector3::rLerp);
				VVector3.SetMethod("vector3 abs() const", &Compute::Vector3::Abs);
				VVector3.SetMethod("vector3 radians() const", &Compute::Vector3::Radians);
				VVector3.SetMethod("vector3 degrees() const", &Compute::Vector3::Degrees);
				VVector3.SetMethod("vector2 xy() const", &Compute::Vector3::XY);
				VVector3.SetMethod("vector3 xyz() const", &Compute::Vector3::XYZ);
				VVector3.SetMethod<Compute::Vector3, Compute::Vector3, float>("vector3 mul(float) const", &Compute::Vector3::Mul);
				VVector3.SetMethod<Compute::Vector3, Compute::Vector3, const Compute::Vector2&, float>("vector3 mul(const vector2 &in, float) const", &Compute::Vector3::Mul);
				VVector3.SetMethod<Compute::Vector3, Compute::Vector3, const Compute::Vector3&>("vector3 mul(const vector3 &in) const", &Compute::Vector3::Mul);
				VVector3.SetMethod("vector3 div(const vector3 &in) const", &Compute::Vector3::Div);
				VVector3.SetMethod("vector3 setX(float) const", &Compute::Vector3::SetX);
				VVector3.SetMethod("vector3 setY(float) const", &Compute::Vector3::SetY);
				VVector3.SetMethod("vector3 setZ(float) const", &Compute::Vector3::SetZ);
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
				Register.SetFunction("vector3 randomAbs()", &Compute::Vector3::RandomAbs);
				Register.SetFunction("vector3 one()", &Compute::Vector3::One);
				Register.SetFunction("vector3 zero()", &Compute::Vector3::Zero);
				Register.SetFunction("vector3 up()", &Compute::Vector3::Up);
				Register.SetFunction("vector3 down()", &Compute::Vector3::Down);
				Register.SetFunction("vector3 left()", &Compute::Vector3::Left);
				Register.SetFunction("vector3 right()", &Compute::Vector3::Right);
				Register.SetFunction("vector3 forward()", &Compute::Vector3::Forward);
				Register.SetFunction("vector3 backward()", &Compute::Vector3::Backward);
				Engine->EndNamespace();

				return true;
			}
			bool Registry::LoadVector4(VMManager* Engine)
			{
				TH_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();
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
				VVector4.SetMethod("vector4 invX() const", &Compute::Vector4::InvX);
				VVector4.SetMethod("vector4 invY() const", &Compute::Vector4::InvY);
				VVector4.SetMethod("vector4 invZ() const", &Compute::Vector4::InvZ);
				VVector4.SetMethod("vector4 invW() const", &Compute::Vector4::InvW);
				VVector4.SetMethod("vector4 normalize() const", &Compute::Vector4::Normalize);
				VVector4.SetMethod("vector4 normalizeSafe() const", &Compute::Vector4::sNormalize);
				VVector4.SetMethod("vector4 lerp(const vector4 &in, float) const", &Compute::Vector4::Lerp);
				VVector4.SetMethod("vector4 lerpStable(const vector4 &in, float) const", &Compute::Vector4::sLerp);
				VVector4.SetMethod("vector4 lerpAngular(const vector4 &in, float) const", &Compute::Vector4::aLerp);
				VVector4.SetMethod("vector4 lerpRound() const", &Compute::Vector4::rLerp);
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
				VVector4.SetMethod("vector4 setX(float) const", &Compute::Vector4::SetX);
				VVector4.SetMethod("vector4 setY(float) const", &Compute::Vector4::SetY);
				VVector4.SetMethod("vector4 setZ(float) const", &Compute::Vector4::SetZ);
				VVector4.SetMethod("vector4 setW(float) const", &Compute::Vector4::SetW);
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
				Register.SetFunction("vector4 randomAbs()", &Compute::Vector4::RandomAbs);
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

				return true;
			}
			bool Registry::LoadUiControl(VMManager* Engine)
			{
				TH_ASSERT(Engine != nullptr, false, "manager should be set");
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
				VEvent.SetConstructor<Engine::GUI::IEvent, Rml::Event*>("void f(pointer@)");
				VEvent.SetMethod("ui_event_phase getPhase() const", &Engine::GUI::IEvent::GetPhase);
				VEvent.SetMethod("void setPhase(ui_event_phase Phase)", &Engine::GUI::IEvent::SetPhase);
				VEvent.SetMethod("void setCurrentElement(const ui_element &in)", &Engine::GUI::IEvent::SetCurrentElement);
				VEvent.SetMethod("ui_element getCurrentElement() const", &Engine::GUI::IEvent::GetCurrentElement);
				VEvent.SetMethod("ui_element getTargetElement() const", &Engine::GUI::IEvent::GetTargetElement);
				VEvent.SetMethod("string getType() const", &Engine::GUI::IEvent::GetType);
				VEvent.SetMethod("void stopPropagation()", &Engine::GUI::IEvent::StopPropagation);
				VEvent.SetMethod("void stopImmediatePropagation()", &Engine::GUI::IEvent::StopImmediatePropagation);
				VEvent.SetMethod("bool isInterruptible() const", &Engine::GUI::IEvent::IsInterruptible);
				VEvent.SetMethod("bool isPropagating() const", &Engine::GUI::IEvent::IsPropagating);
				VEvent.SetMethod("bool isImmediatePropagating() const", &Engine::GUI::IEvent::IsImmediatePropagating);
				VEvent.SetMethod("bool getBoolean(const string &in) const", &Engine::GUI::IEvent::GetBoolean);
				VEvent.SetMethod("int64 getInteger(const string &in) const", &Engine::GUI::IEvent::GetInteger);
				VEvent.SetMethod("double getNumber(const string &in) const", &Engine::GUI::IEvent::GetNumber);
				VEvent.SetMethod("string getString(const string &in) const", &Engine::GUI::IEvent::GetString);
				VEvent.SetMethod("vector2 getVector2(const string &in) const", &Engine::GUI::IEvent::GetVector2);
				VEvent.SetMethod("vector3 getVector3(const string &in) const", &Engine::GUI::IEvent::GetVector3);
				VEvent.SetMethod("vector4 getVector4(const string &in) const", &Engine::GUI::IEvent::GetVector4);
				VEvent.SetMethod("pointer@ getPointer(const string &in) const", &Engine::GUI::IEvent::GetPointer);
				VEvent.SetMethod("pointer@ getEvent() const", &Engine::GUI::IEvent::GetEvent);
				VEvent.SetMethod("bool isValid() const", &Engine::GUI::IEvent::IsValid);

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
				VElement.SetConstructor<Engine::GUI::IElement, Rml::Element*>("void f(pointer@)");
				VElement.SetMethod("ui_element Clone() const", &Engine::GUI::IElement::Clone);
				VElement.SetMethod("void setClass(const string &in, bool)", &Engine::GUI::IElement::SetClass);
				VElement.SetMethod("bool isClassSet(const string &in) const", &Engine::GUI::IElement::IsClassSet);
				VElement.SetMethod("void setClassNames(const string &in)", &Engine::GUI::IElement::SetClassNames);
				VElement.SetMethod("string getClassNames() const", &Engine::GUI::IElement::GetClassNames);
				VElement.SetMethod("string getAddress(bool = false, bool = true) const", &Engine::GUI::IElement::GetAddress);
				VElement.SetMethod("void setOffset(const vector2 &in, const ui_element &in, bool = false)", &Engine::GUI::IElement::SetOffset);
				VElement.SetMethod("vector2 getRelativeOffset(ui_area = ui_area::Content) const", &Engine::GUI::IElement::GetRelativeOffset);
				VElement.SetMethod("vector2 getAbsoluteOffset(ui_area = ui_area::Content) const", &Engine::GUI::IElement::GetAbsoluteOffset);
				VElement.SetMethod("void setClientArea(ui_area)", &Engine::GUI::IElement::SetClientArea);
				VElement.SetMethod("ui_area getClientArea() const", &Engine::GUI::IElement::GetClientArea);
				VElement.SetMethod("void setContentBox(const vector2 &in, const vector2 &in)", &Engine::GUI::IElement::SetContentBox);
				VElement.SetMethod("float getBaseline() const", &Engine::GUI::IElement::GetBaseline);
				VElement.SetMethod("bool getIntrinsicDimensions(vector2 &out, float &out)", &Engine::GUI::IElement::GetIntrinsicDimensions);
				VElement.SetMethod("bool isPointWithinElement(const vector2 &in)", &Engine::GUI::IElement::IsPointWithinElement);
				VElement.SetMethod("bool isVisible() const", &Engine::GUI::IElement::IsVisible);
				VElement.SetMethod("float getZIndex() const", &Engine::GUI::IElement::GetZIndex);
				VElement.SetMethod("bool setProperty(const string &in, const string &in)", &Engine::GUI::IElement::SetProperty);
				VElement.SetMethod("void removeProperty(const string &in)", &Engine::GUI::IElement::RemoveProperty);
				VElement.SetMethod("string getProperty(const string &in) const", &Engine::GUI::IElement::GetProperty);
				VElement.SetMethod("string getLocalProperty(const string &in) const", &Engine::GUI::IElement::GetLocalProperty);
				VElement.SetMethod("float resolveNumericProperty(const string &in) const", &Engine::GUI::IElement::ResolveNumericProperty);
				VElement.SetMethod("vector2 getContainingBlock() const", &Engine::GUI::IElement::GetContainingBlock);
				VElement.SetMethod("ui_position getPosition() const", &Engine::GUI::IElement::GetPosition);
				VElement.SetMethod("ui_float getFloat() const", &Engine::GUI::IElement::GetFloat);
				VElement.SetMethod("ui_display getDisplay() const", &Engine::GUI::IElement::GetDisplay);
				VElement.SetMethod("float getLineHeight() const", &Engine::GUI::IElement::GetLineHeight);
				VElement.SetMethod("bool project(vector2 &out) const", &Engine::GUI::IElement::Project);
				VElement.SetMethod("bool animate(const string &in, const string &in, float, ui_timing_func, ui_timing_dir, int = -1, bool = true, float = 0)", &Engine::GUI::IElement::Animate);
				VElement.SetMethod("bool addAnimationKey(const string &in, const string &in, float, ui_timing_func, ui_timing_dir)", &Engine::GUI::IElement::AddAnimationKey);
				VElement.SetMethod("void setPseudoClass(const string &in, bool)", &Engine::GUI::IElement::SetPseudoClass);
				VElement.SetMethod("bool isPseudoClassSet(const string &in) const", &Engine::GUI::IElement::IsPseudoClassSet);
				VElement.SetMethod("void setAttribute(const string &in, const string &in)", &Engine::GUI::IElement::SetAttribute);
				VElement.SetMethod("string getAttribute(const string &in) const", &Engine::GUI::IElement::GetAttribute);
				VElement.SetMethod("bool hasAttribute(const string &in) const", &Engine::GUI::IElement::HasAttribute);
				VElement.SetMethod("void removeAttribute(const string &in)", &Engine::GUI::IElement::RemoveAttribute);
				VElement.SetMethod("ui_element getFocusLeafNode()", &Engine::GUI::IElement::GetFocusLeafNode);
				VElement.SetMethod("string getTagName() const", &Engine::GUI::IElement::GetTagName);
				VElement.SetMethod("string getId() const", &Engine::GUI::IElement::GetId);
				VElement.SetMethod("float getAbsoluteLeft()", &Engine::GUI::IElement::GetAbsoluteLeft);
				VElement.SetMethod("float getAbsoluteTop()", &Engine::GUI::IElement::GetAbsoluteTop);
				VElement.SetMethod("float getClientLeft()", &Engine::GUI::IElement::GetClientLeft);
				VElement.SetMethod("float getClientTop()", &Engine::GUI::IElement::GetClientTop);
				VElement.SetMethod("float getClientWidth()", &Engine::GUI::IElement::GetClientWidth);
				VElement.SetMethod("float getClientHeight()", &Engine::GUI::IElement::GetClientHeight);
				VElement.SetMethod("ui_element getOffsetParent()", &Engine::GUI::IElement::GetOffsetParent);
				VElement.SetMethod("float getOffsetLeft()", &Engine::GUI::IElement::GetOffsetLeft);
				VElement.SetMethod("float getOffsetTop()", &Engine::GUI::IElement::GetOffsetTop);
				VElement.SetMethod("float getOffsetWidth()", &Engine::GUI::IElement::GetOffsetWidth);
				VElement.SetMethod("float getOffsetHeight()", &Engine::GUI::IElement::GetOffsetHeight);
				VElement.SetMethod("float getScrollLeft()", &Engine::GUI::IElement::GetScrollLeft);
				VElement.SetMethod("void setScrollLeft(float)", &Engine::GUI::IElement::SetScrollLeft);
				VElement.SetMethod("float getScrollTop()", &Engine::GUI::IElement::GetScrollTop);
				VElement.SetMethod("void setScrollTop(float)", &Engine::GUI::IElement::SetScrollTop);
				VElement.SetMethod("float getScrollWidth()", &Engine::GUI::IElement::GetScrollWidth);
				VElement.SetMethod("float getScrollHeight()", &Engine::GUI::IElement::GetScrollHeight);
				VElement.SetMethod("ui_document getOwnerDocument() const", &Engine::GUI::IElement::GetOwnerDocument);
				VElement.SetMethod("ui_element getParentNode() const", &Engine::GUI::IElement::GetParentNode);
				VElement.SetMethod("ui_element getNextSibling() const", &Engine::GUI::IElement::GetNextSibling);
				VElement.SetMethod("ui_element getPreviousSibling() const", &Engine::GUI::IElement::GetPreviousSibling);
				VElement.SetMethod("ui_element getFirstChild() const", &Engine::GUI::IElement::GetFirstChild);
				VElement.SetMethod("ui_element getLastChild() const", &Engine::GUI::IElement::GetLastChild);
				VElement.SetMethod("ui_element getChild(int) const", &Engine::GUI::IElement::GetChild);
				VElement.SetMethod("int getNumChildren(bool = false) const", &Engine::GUI::IElement::GetNumChildren);
				VElement.SetMethod<Engine::GUI::IElement, void, std::string&>("void getInnerHTML(string &out) const", &Engine::GUI::IElement::GetInnerHTML);
				VElement.SetMethod<Engine::GUI::IElement, std::string>("string getInnerHTML() const", &Engine::GUI::IElement::GetInnerHTML);
				VElement.SetMethod("void setInnerHTML(const string &in)", &Engine::GUI::IElement::SetInnerHTML);
				VElement.SetMethod("bool isFocused()", &Engine::GUI::IElement::IsFocused);
				VElement.SetMethod("bool isHovered()", &Engine::GUI::IElement::IsHovered);
				VElement.SetMethod("bool isActive()", &Engine::GUI::IElement::IsActive);
				VElement.SetMethod("bool isChecked()", &Engine::GUI::IElement::IsChecked);
				VElement.SetMethod("bool focus()", &Engine::GUI::IElement::Focus);
				VElement.SetMethod("void blur()", &Engine::GUI::IElement::Blur);
				VElement.SetMethod("void click()", &Engine::GUI::IElement::Click);
				VElement.SetMethod("void addEventListener(const string &in, ui_listener@+, bool = false)", &Engine::GUI::IElement::AddEventListener);
				VElement.SetMethod("void removeEventListener(const string &in, ui_listener@+, bool = false)", &Engine::GUI::IElement::RemoveEventListener);
				VElement.SetMethodEx("bool dispatchEvent(const string &in, schema@+)", &IElementDispatchEvent);
				VElement.SetMethod("void scrollIntoView(bool = true)", &Engine::GUI::IElement::ScrollIntoView);
				VElement.SetMethod("ui_element appendChild(const ui_element &in, bool = true)", &Engine::GUI::IElement::AppendChild);
				VElement.SetMethod("ui_element insertBefore(const ui_element &in, const ui_element &in)", &Engine::GUI::IElement::InsertBefore);
				VElement.SetMethod("ui_element replaceChild(const ui_element &in, const ui_element &in)", &Engine::GUI::IElement::ReplaceChild);
				VElement.SetMethod("ui_element removeChild(const ui_element &in)", &Engine::GUI::IElement::RemoveChild);
				VElement.SetMethod("bool hasChildNodes() const", &Engine::GUI::IElement::HasChildNodes);
				VElement.SetMethod("ui_element getElementById(const string &in)", &Engine::GUI::IElement::GetElementById);
				VElement.SetMethodEx("array<ui_element>@ querySelectorAll(const string &in)", &IElementQuerySelectorAll);
				VElement.SetMethod("bool castFormColor(vector4 &out, bool)", &Engine::GUI::IElement::CastFormColor);
				VElement.SetMethod("bool castFormString(string &out)", &Engine::GUI::IElement::CastFormString);
				VElement.SetMethod("bool castFormPointer(pointer@ &out)", &Engine::GUI::IElement::CastFormPointer);
				VElement.SetMethod("bool castFormInt32(int &out)", &Engine::GUI::IElement::CastFormInt32);
				VElement.SetMethod("bool castFormUInt32(uint &out)", &Engine::GUI::IElement::CastFormUInt32);
				VElement.SetMethod("bool castFormFlag32(uint &out, uint)", &Engine::GUI::IElement::CastFormFlag32);
				VElement.SetMethod("bool castFormInt64(int64 &out)", &Engine::GUI::IElement::CastFormInt64);
				VElement.SetMethod("bool castFormUInt64(uint64 &out)", &Engine::GUI::IElement::CastFormUInt64);
				VElement.SetMethod("bool castFormFlag64(uint64 &out, uint64)", &Engine::GUI::IElement::CastFormFlag64);
				VElement.SetMethod<Engine::GUI::IElement, bool, float*>("bool castFormFloat(float &out)", &Engine::GUI::IElement::CastFormFloat);
				VElement.SetMethod<Engine::GUI::IElement, bool, float*, float>("bool castFormFloat(float &out, float)", &Engine::GUI::IElement::CastFormFloat);
				VElement.SetMethod("bool castFormDouble(double &out)", &Engine::GUI::IElement::CastFormDouble);
				VElement.SetMethod("bool castFormBoolean(bool &out)", &Engine::GUI::IElement::CastFormBoolean);
				VElement.SetMethod("string getFormName() const", &Engine::GUI::IElement::GetFormName);
				VElement.SetMethod("void setFormName(const string &in)", &Engine::GUI::IElement::SetFormName);
				VElement.SetMethod("string getFormValue() const", &Engine::GUI::IElement::GetFormValue);
				VElement.SetMethod("void setFormValue(const string &in)", &Engine::GUI::IElement::SetFormValue);
				VElement.SetMethod("bool isFormDisabled() const", &Engine::GUI::IElement::IsFormDisabled);
				VElement.SetMethod("void setFormDisabled(bool)", &Engine::GUI::IElement::SetFormDisabled);
				VElement.SetMethod("pointer@ getElement() const", &Engine::GUI::IElement::GetElement);
				VElement.SetMethod("bool isValid() const", &Engine::GUI::IElement::GetElement);

				VDocument.SetConstructor<Engine::GUI::IElementDocument>("void f()");
				VDocument.SetConstructor<Engine::GUI::IElementDocument, Rml::ElementDocument*>("void f(pointer@)");
				VDocument.SetMethod("ui_element clone() const", &Engine::GUI::IElementDocument::Clone);
				VDocument.SetMethod("void setClass(const string &in, bool)", &Engine::GUI::IElementDocument::SetClass);
				VDocument.SetMethod("bool isClassSet(const string &in) const", &Engine::GUI::IElementDocument::IsClassSet);
				VDocument.SetMethod("void setClassNames(const string &in)", &Engine::GUI::IElementDocument::SetClassNames);
				VDocument.SetMethod("string getClassNames() const", &Engine::GUI::IElementDocument::GetClassNames);
				VDocument.SetMethod("string getAddress(bool = false, bool = true) const", &Engine::GUI::IElementDocument::GetAddress);
				VDocument.SetMethod("void setOffset(const vector2 &in, const ui_element &in, bool = false)", &Engine::GUI::IElementDocument::SetOffset);
				VDocument.SetMethod("vector2 getRelativeOffset(ui_area = ui_area::Content) const", &Engine::GUI::IElementDocument::GetRelativeOffset);
				VDocument.SetMethod("vector2 getAbsoluteOffset(ui_area = ui_area::Content) const", &Engine::GUI::IElementDocument::GetAbsoluteOffset);
				VDocument.SetMethod("void setClientArea(ui_area)", &Engine::GUI::IElementDocument::SetClientArea);
				VDocument.SetMethod("ui_area getClientArea() const", &Engine::GUI::IElementDocument::GetClientArea);
				VDocument.SetMethod("void setContentBox(const vector2 &in, const vector2 &in)", &Engine::GUI::IElementDocument::SetContentBox);
				VDocument.SetMethod("float getBaseline() const", &Engine::GUI::IElementDocument::GetBaseline);
				VDocument.SetMethod("bool getIntrinsicDimensions(vector2 &out, float &out)", &Engine::GUI::IElementDocument::GetIntrinsicDimensions);
				VDocument.SetMethod("bool isPointWithinElement(const vector2 &in)", &Engine::GUI::IElementDocument::IsPointWithinElement);
				VDocument.SetMethod("bool isVisible() const", &Engine::GUI::IElementDocument::IsVisible);
				VDocument.SetMethod("float getZIndex() const", &Engine::GUI::IElementDocument::GetZIndex);
				VDocument.SetMethod("bool setProperty(const string &in, const string &in)", &Engine::GUI::IElementDocument::SetProperty);
				VDocument.SetMethod("void removeProperty(const string &in)", &Engine::GUI::IElementDocument::RemoveProperty);
				VDocument.SetMethod("string getProperty(const string &in) const", &Engine::GUI::IElementDocument::GetProperty);
				VDocument.SetMethod("string getLocalProperty(const string &in) const", &Engine::GUI::IElementDocument::GetLocalProperty);
				VDocument.SetMethod("float resolveNumericProperty(const string &in) const", &Engine::GUI::IElementDocument::ResolveNumericProperty);
				VDocument.SetMethod("vector2 getContainingBlock() const", &Engine::GUI::IElementDocument::GetContainingBlock);
				VDocument.SetMethod("ui_position getPosition() const", &Engine::GUI::IElementDocument::GetPosition);
				VDocument.SetMethod("ui_float getFloat() const", &Engine::GUI::IElementDocument::GetFloat);
				VDocument.SetMethod("ui_display getDisplay() const", &Engine::GUI::IElementDocument::GetDisplay);
				VDocument.SetMethod("float getLineHeight() const", &Engine::GUI::IElementDocument::GetLineHeight);
				VDocument.SetMethod("bool project(vector2 &out) const", &Engine::GUI::IElementDocument::Project);
				VDocument.SetMethod("bool animate(const string &in, const string &in, float, ui_timing_func, ui_timing_dir, int = -1, bool = true, float = 0)", &Engine::GUI::IElementDocument::Animate);
				VDocument.SetMethod("bool addAnimationKey(const string &in, const string &in, float, ui_timing_func, ui_timing_dir)", &Engine::GUI::IElementDocument::AddAnimationKey);
				VDocument.SetMethod("void setPseudoClass(const string &in, bool)", &Engine::GUI::IElementDocument::SetPseudoClass);
				VDocument.SetMethod("bool isPseudoClassSet(const string &in) const", &Engine::GUI::IElementDocument::IsPseudoClassSet);
				VDocument.SetMethod("void setAttribute(const string &in, const string &in)", &Engine::GUI::IElementDocument::SetAttribute);
				VDocument.SetMethod("string getAttribute(const string &in) const", &Engine::GUI::IElementDocument::GetAttribute);
				VDocument.SetMethod("bool hasAttribute(const string &in) const", &Engine::GUI::IElementDocument::HasAttribute);
				VDocument.SetMethod("void removeAttribute(const string &in)", &Engine::GUI::IElementDocument::RemoveAttribute);
				VDocument.SetMethod("ui_element getFocusLeafNode()", &Engine::GUI::IElementDocument::GetFocusLeafNode);
				VDocument.SetMethod("string getTagName() const", &Engine::GUI::IElementDocument::GetTagName);
				VDocument.SetMethod("string getId() const", &Engine::GUI::IElementDocument::GetId);
				VDocument.SetMethod("float getAbsoluteLeft()", &Engine::GUI::IElementDocument::GetAbsoluteLeft);
				VDocument.SetMethod("float getAbsoluteTop()", &Engine::GUI::IElementDocument::GetAbsoluteTop);
				VDocument.SetMethod("float getClientLeft()", &Engine::GUI::IElementDocument::GetClientLeft);
				VDocument.SetMethod("float getClientTop()", &Engine::GUI::IElementDocument::GetClientTop);
				VDocument.SetMethod("float getClientWidth()", &Engine::GUI::IElementDocument::GetClientWidth);
				VDocument.SetMethod("float getClientHeight()", &Engine::GUI::IElementDocument::GetClientHeight);
				VDocument.SetMethod("ui_element getOffsetParent()", &Engine::GUI::IElementDocument::GetOffsetParent);
				VDocument.SetMethod("float getOffsetLeft()", &Engine::GUI::IElementDocument::GetOffsetLeft);
				VDocument.SetMethod("float getOffsetTop()", &Engine::GUI::IElementDocument::GetOffsetTop);
				VDocument.SetMethod("float getOffsetWidth()", &Engine::GUI::IElementDocument::GetOffsetWidth);
				VDocument.SetMethod("float getOffsetHeight()", &Engine::GUI::IElementDocument::GetOffsetHeight);
				VDocument.SetMethod("float getScrollLeft()", &Engine::GUI::IElementDocument::GetScrollLeft);
				VDocument.SetMethod("void setScrollLeft(float)", &Engine::GUI::IElementDocument::SetScrollLeft);
				VDocument.SetMethod("float getScrollTop()", &Engine::GUI::IElementDocument::GetScrollTop);
				VDocument.SetMethod("void setScrollTop(float)", &Engine::GUI::IElementDocument::SetScrollTop);
				VDocument.SetMethod("float getScrollWidth()", &Engine::GUI::IElementDocument::GetScrollWidth);
				VDocument.SetMethod("float getScrollHeight()", &Engine::GUI::IElementDocument::GetScrollHeight);
				VDocument.SetMethod("ui_document getOwnerDocument() const", &Engine::GUI::IElementDocument::GetOwnerDocument);
				VDocument.SetMethod("ui_element getParentNode() const", &Engine::GUI::IElementDocument::GetParentNode);
				VDocument.SetMethod("ui_element getNextSibling() const", &Engine::GUI::IElementDocument::GetNextSibling);
				VDocument.SetMethod("ui_element getPreviousSibling() const", &Engine::GUI::IElementDocument::GetPreviousSibling);
				VDocument.SetMethod("ui_element getFirstChild() const", &Engine::GUI::IElementDocument::GetFirstChild);
				VDocument.SetMethod("ui_element getLastChild() const", &Engine::GUI::IElementDocument::GetLastChild);
				VDocument.SetMethod("ui_element getChild(int) const", &Engine::GUI::IElementDocument::GetChild);
				VDocument.SetMethod("int getNumChildren(bool = false) const", &Engine::GUI::IElementDocument::GetNumChildren);
				VDocument.SetMethod<Engine::GUI::IElement, void, std::string&>("void getInnerHTML(string &out) const", &Engine::GUI::IElementDocument::GetInnerHTML);
				VDocument.SetMethod<Engine::GUI::IElement, std::string>("string getInnerHTML() const", &Engine::GUI::IElementDocument::GetInnerHTML);
				VDocument.SetMethod("void setInnerHTML(const string &in)", &Engine::GUI::IElementDocument::SetInnerHTML);
				VDocument.SetMethod("bool isFocused()", &Engine::GUI::IElementDocument::IsFocused);
				VDocument.SetMethod("bool isHovered()", &Engine::GUI::IElementDocument::IsHovered);
				VDocument.SetMethod("bool isActive()", &Engine::GUI::IElementDocument::IsActive);
				VDocument.SetMethod("bool isChecked()", &Engine::GUI::IElementDocument::IsChecked);
				VDocument.SetMethod("bool focus()", &Engine::GUI::IElementDocument::Focus);
				VDocument.SetMethod("void blur()", &Engine::GUI::IElementDocument::Blur);
				VDocument.SetMethod("void click()", &Engine::GUI::IElementDocument::Click);
				VDocument.SetMethod("void addEventListener(const string &in, ui_listener@+, bool = false)", &Engine::GUI::IElementDocument::AddEventListener);
				VDocument.SetMethod("void removeEventListener(const string &in, ui_listener@+, bool = false)", &Engine::GUI::IElementDocument::RemoveEventListener);
				VDocument.SetMethodEx("bool dispatchEvent(const string &in, schema@+)", &IElementDocumentDispatchEvent);
				VDocument.SetMethod("void scrollIntoView(bool = true)", &Engine::GUI::IElementDocument::ScrollIntoView);
				VDocument.SetMethod("ui_element appendChild(const ui_element &in, bool = true)", &Engine::GUI::IElementDocument::AppendChild);
				VDocument.SetMethod("ui_element insertBefore(const ui_element &in, const ui_element &in)", &Engine::GUI::IElementDocument::InsertBefore);
				VDocument.SetMethod("ui_element replaceChild(const ui_element &in, const ui_element &in)", &Engine::GUI::IElementDocument::ReplaceChild);
				VDocument.SetMethod("ui_element removeChild(const ui_element &in)", &Engine::GUI::IElementDocument::RemoveChild);
				VDocument.SetMethod("bool hasChildNodes() const", &Engine::GUI::IElementDocument::HasChildNodes);
				VDocument.SetMethod("ui_element getElementById(const string &in)", &Engine::GUI::IElementDocument::GetElementById);
				VDocument.SetMethodEx("array<ui_element>@ querySelectorAll(const string &in)", &IElementDocumentQuerySelectorAll);
				VDocument.SetMethod("bool castFormColor(vector4 &out, bool)", &Engine::GUI::IElementDocument::CastFormColor);
				VDocument.SetMethod("bool castFormString(string &out)", &Engine::GUI::IElementDocument::CastFormString);
				VDocument.SetMethod("bool castFormPointer(pointer@ &out)", &Engine::GUI::IElementDocument::CastFormPointer);
				VDocument.SetMethod("bool castFormInt32(int &out)", &Engine::GUI::IElementDocument::CastFormInt32);
				VDocument.SetMethod("bool castFormUInt32(uint &out)", &Engine::GUI::IElementDocument::CastFormUInt32);
				VDocument.SetMethod("bool castFormFlag32(uint &out, uint)", &Engine::GUI::IElementDocument::CastFormFlag32);
				VDocument.SetMethod("bool castFormInt64(int64 &out)", &Engine::GUI::IElementDocument::CastFormInt64);
				VDocument.SetMethod("bool castFormUInt64(uint64 &out)", &Engine::GUI::IElementDocument::CastFormUInt64);
				VDocument.SetMethod("bool castFormFlag64(uint64 &out, uint64)", &Engine::GUI::IElementDocument::CastFormFlag64);
				VDocument.SetMethod<Engine::GUI::IElement, bool, float*>("bool castFormFloat(float &out)", &Engine::GUI::IElementDocument::CastFormFloat);
				VDocument.SetMethod<Engine::GUI::IElement, bool, float*, float>("bool castFormFloat(float &out, float)", &Engine::GUI::IElementDocument::CastFormFloat);
				VDocument.SetMethod("bool castFormDouble(double &out)", &Engine::GUI::IElementDocument::CastFormDouble);
				VDocument.SetMethod("bool castFormBoolean(bool &out)", &Engine::GUI::IElementDocument::CastFormBoolean);
				VDocument.SetMethod("string getFormName() const", &Engine::GUI::IElementDocument::GetFormName);
				VDocument.SetMethod("void setFormName(const string &in)", &Engine::GUI::IElementDocument::SetFormName);
				VDocument.SetMethod("string getFormValue() const", &Engine::GUI::IElementDocument::GetFormValue);
				VDocument.SetMethod("void setFormValue(const string &in)", &Engine::GUI::IElementDocument::SetFormValue);
				VDocument.SetMethod("bool isFormDisabled() const", &Engine::GUI::IElementDocument::IsFormDisabled);
				VDocument.SetMethod("void setFormDisabled(bool)", &Engine::GUI::IElementDocument::SetFormDisabled);
				VDocument.SetMethod("pointer@ getElement() const", &Engine::GUI::IElementDocument::GetElement);
				VDocument.SetMethod("bool isValid() const", &Engine::GUI::IElementDocument::GetElement);
				VDocument.SetMethod("void setTitle(const string &in)", &Engine::GUI::IElementDocument::SetTitle);
				VDocument.SetMethod("void pullToFront()", &Engine::GUI::IElementDocument::PullToFront);
				VDocument.SetMethod("void pushToBack()", &Engine::GUI::IElementDocument::PushToBack);
				VDocument.SetMethod("void show(ui_modal_flag = ui_modal_flag::None, ui_focus_flag = ui_focus_flag::Auto)", &Engine::GUI::IElementDocument::Show);
				VDocument.SetMethod("void hide()", &Engine::GUI::IElementDocument::Hide);
				VDocument.SetMethod("void close()", &Engine::GUI::IElementDocument::Close);
				VDocument.SetMethod("string getTitle() const", &Engine::GUI::IElementDocument::GetTitle);
				VDocument.SetMethod("string getSourceURL() const", &Engine::GUI::IElementDocument::GetSourceURL);
				VDocument.SetMethod("ui_element createElement(const string &in)", &Engine::GUI::IElementDocument::CreateElement);
				VDocument.SetMethod("bool isModal() const", &Engine::GUI::IElementDocument::IsModal);;
				VDocument.SetMethod("pointer@ getElementDocument() const", &Engine::GUI::IElementDocument::GetElementDocument);

				return true;
			}
			bool Registry::LoadUiModel(VMManager* Engine)
			{
				TH_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();
				Register.SetFunctionDef("void ui_data_event(ui_event &in, array<variant>@+)");

				VMRefClass VModel = Register.SetClassUnmanaged<Engine::GUI::DataModel>("ui_model");
				VModel.SetMethodEx("bool set(const string &in, schema@+)", &DataModelSet);
				VModel.SetMethodEx("bool setVar(const string &in, const variant &in)", &DataModelSetVar);
				VModel.SetMethodEx("bool setString(const string &in, const string &in)", &DataModelSetString);
				VModel.SetMethodEx("bool setInteger(const string &in, int64)", &DataModelSetInteger);
				VModel.SetMethodEx("bool setFloat(const string &in, float)", &DataModelSetFloat);
				VModel.SetMethodEx("bool setDouble(const string &in, double)", &DataModelSetDouble);
				VModel.SetMethodEx("bool setBoolean(const string &in, bool)", &DataModelSetBoolean);
				VModel.SetMethodEx("bool setPointer(const string &in, pointer@)", &DataModelSetPointer);
				VModel.SetMethodEx("bool setCallback(const string &in, ui_data_event@)", &DataModelSetCallback);
				VModel.SetMethodEx("schema@+ get(const string &in)", &DataModelGet);
				VModel.SetMethod("string getString(const string &in)", &Engine::GUI::DataModel::GetString);
				VModel.SetMethod("int64 getInteger(const string &in)", &Engine::GUI::DataModel::GetInteger);
				VModel.SetMethod("float getFloat(const string &in)", &Engine::GUI::DataModel::GetFloat);
				VModel.SetMethod("double getDouble(const string &in)", &Engine::GUI::DataModel::GetDouble);
				VModel.SetMethod("bool getBoolean(const string &in)", &Engine::GUI::DataModel::GetBoolean);
				VModel.SetMethod("pointer@ getPointer(const string &in)", &Engine::GUI::DataModel::GetPointer);
				VModel.SetMethod("bool hasChanged(const string &in)", &Engine::GUI::DataModel::HasChanged);
				VModel.SetMethod("void change(const string &in)", &Engine::GUI::DataModel::Change);
				VModel.SetMethod("bool isValid() const", &Engine::GUI::DataModel::IsValid);
				VModel.SetMethodStatic("vector4 toColor4(const string &in)", &Engine::GUI::IVariant::ToColor4);
				VModel.SetMethodStatic("string fromColor4(const vector4 &in, bool)", &Engine::GUI::IVariant::FromColor4);
				VModel.SetMethodStatic("vector4 toColor3(const string &in)", &Engine::GUI::IVariant::ToColor3);
				VModel.SetMethodStatic("string fromColor3(const vector4 &in, bool)", &Engine::GUI::IVariant::ToColor3);
				VModel.SetMethodStatic("int getVectorType(const string &in)", &Engine::GUI::IVariant::GetVectorType);
				VModel.SetMethodStatic("vector4 toVector4(const string &in)", &Engine::GUI::IVariant::ToVector4);
				VModel.SetMethodStatic("string fromVector4(const vector4 &in)", &Engine::GUI::IVariant::FromVector4);
				VModel.SetMethodStatic("vector3 toVector3(const string &in)", &Engine::GUI::IVariant::ToVector3);
				VModel.SetMethodStatic("string fromVector3(const vector3 &in)", &Engine::GUI::IVariant::FromVector3);
				VModel.SetMethodStatic("vector2 toVector2(const string &in)", &Engine::GUI::IVariant::ToVector2);
				VModel.SetMethodStatic("string fromVector2(const vector2 &in)", &Engine::GUI::IVariant::FromVector2);

				return true;
			}
			bool Registry::LoadUiContext(VMManager* Engine)
			{
				TH_ASSERT(Engine != nullptr, false, "manager should be set");
				VMGlobal& Register = Engine->Global();
				VMRefClass VContext = Register.SetClassUnmanaged<Engine::GUI::Context>("gui_context");
				VContext.SetMethod("void setDocumentsBaseTag(const string &in)", &Engine::GUI::Context::SetDocumentsBaseTag);
				VContext.SetMethod("void clearStyles()", &Engine::GUI::Context::ClearStyles);
				VContext.SetMethod("bool clearDocuments()", &Engine::GUI::Context::ClearDocuments);
				VContext.SetMethod<Engine::GUI::Context, bool, const std::string&>("bool initialize(const string &in)", &Engine::GUI::Context::Initialize);
				VContext.SetMethod("bool isInputFocused()", &Engine::GUI::Context::IsInputFocused);
				VContext.SetMethod("bool isLoading()", &Engine::GUI::Context::IsLoading);
				VContext.SetMethod("bool replaceHTML(const string &in, const string &in, int = 0)", &Engine::GUI::Context::ReplaceHTML);
				VContext.SetMethod("bool removeDataModel(const string &in)", &Engine::GUI::Context::RemoveDataModel);
				VContext.SetMethod("bool removeDataModels()", &Engine::GUI::Context::RemoveDataModels);
				VContext.SetMethod("pointer@ getContext()", &Engine::GUI::Context::GetContext);
				VContext.SetMethod("vector2 getDimensions() const", &Engine::GUI::Context::GetDimensions);
				VContext.SetMethod("string getDocumentsBaseTag() const", &Engine::GUI::Context::GetDocumentsBaseTag);
				VContext.SetMethod("void setDensityIndependentPixelRatio(float)", &Engine::GUI::Context::GetDensityIndependentPixelRatio);
				VContext.SetMethod("float getDensityIndependentPixelRatio() const", &Engine::GUI::Context::GetDensityIndependentPixelRatio);
				VContext.SetMethod("void enableMouseCursor(bool)", &Engine::GUI::Context::EnableMouseCursor);
				VContext.SetMethod("ui_document evalHTML(const string &in, int = 0)", &Engine::GUI::Context::EvalHTML);
				VContext.SetMethod("ui_document addCSS(const string &in, int = 0)", &Engine::GUI::Context::AddCSS);
				VContext.SetMethod("ui_document loadCSS(const string &in, int = 0)", &Engine::GUI::Context::LoadCSS);
				VContext.SetMethod("ui_document loadDocument(const string &in)", &Engine::GUI::Context::LoadDocument);
				VContext.SetMethod("ui_document addDocument(const string &in)", &Engine::GUI::Context::AddDocument);
				VContext.SetMethod("ui_document addDocumentEmpty(const string &in = \"body\")", &Engine::GUI::Context::AddDocumentEmpty);
				VContext.SetMethod<Engine::GUI::Context, Engine::GUI::IElementDocument, const std::string&>("ui_document getDocument(const string &in)", &Engine::GUI::Context::GetDocument);
				VContext.SetMethod<Engine::GUI::Context, Engine::GUI::IElementDocument, int>("ui_document getDocument(int)", &Engine::GUI::Context::GetDocument);
				VContext.SetMethod("int getNumDocuments() const", &Engine::GUI::Context::GetNumDocuments);
				VContext.SetMethod("ui_element getElementById(const string &in, int = 0)", &Engine::GUI::Context::GetElementById);
				VContext.SetMethod("ui_element getHoverElement()", &Engine::GUI::Context::GetHoverElement);
				VContext.SetMethod("ui_element getFocusElement()", &Engine::GUI::Context::GetFocusElement);
				VContext.SetMethod("ui_element getRootElement()", &Engine::GUI::Context::GetRootElement);
				VContext.SetMethodEx("ui_element getElementAtPoint(const vector2 &in)", &ContextGetFocusElement);
				VContext.SetMethod("void pullDocumentToFront(const ui_document &in)", &Engine::GUI::Context::PullDocumentToFront);
				VContext.SetMethod("void pushDocumentToBack(const ui_document &in)", &Engine::GUI::Context::PushDocumentToBack);
				VContext.SetMethod("void unfocusDocument(const ui_document &in)", &Engine::GUI::Context::UnfocusDocument);
				VContext.SetMethod("void addEventListener(const string &in, ui_listener@+, bool = false)", &Engine::GUI::Context::AddEventListener);
				VContext.SetMethod("void removeEventListener(const string &in, ui_listener@+, bool = false)", &Engine::GUI::Context::RemoveEventListener);
				VContext.SetMethod("bool isMouseInteracting()", &Engine::GUI::Context::IsMouseInteracting);
				VContext.SetMethod("bool getActiveClipRegion(vector2 &out, vector2 &out)", &Engine::GUI::Context::GetActiveClipRegion);
				VContext.SetMethod("void setActiveClipRegion(const vector2 &in, const vector2 &in)", &Engine::GUI::Context::SetActiveClipRegion);
				VContext.SetMethod("ui_model@ setModel(const string &in)", &Engine::GUI::Context::SetDataModel);
				VContext.SetMethod("ui_model@ getModel(const string &in)", &Engine::GUI::Context::GetDataModel);

				return true;
			}
			bool Registry::Release()
			{
				StringFactory::Free();
				TH_DELETE(Mapping, Names);
				Names = nullptr;
				return false;
			}
		}
	}
}