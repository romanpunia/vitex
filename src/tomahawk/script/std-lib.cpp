#include "std-lib.h"
#include <new>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sstream>
#ifndef __psp2__
#include <locale.h>
#endif
#ifndef ANGELSCRIPT_H 
#include <angelscript.h>
#endif
#ifdef __BORLANDC__
#include <cmath>
#if __BORLANDC__ < 0x580
inline float cosf(float arg)
{
	return std::cos(arg);
}
inline float sinf(float arg)
{
	return std::sin(arg);
}
inline float tanf(float arg)
{
	return std::tan(arg);
}
inline float atan2f(float y, float x)
{
	return std::atan2(y, x);
}
inline float logf(float arg)
{
	return std::log(arg);
}
inline float powf(float x, float y)
{
	return std::pow(x, y);
}
#endif
inline float acosf(float arg)
{
	return std::acos(arg);
}
inline float asinf(float arg)
{
	return std::asin(arg);
}
inline float atanf(float arg)
{
	return std::atan(arg);
}
inline float coshf(float arg)
{
	return std::cosh(arg);
}
inline float sinhf(float arg)
{
	return std::sinh(arg);
}
inline float tanhf(float arg)
{
	return std::tanh(arg);
}
inline float log10f(float arg)
{
	return std::log10(arg);
}
inline float ceilf(float arg)
{
	return std::ceil(arg);
}
inline float fabsf(float arg)
{
	return std::fabs(arg);
}
inline float floorf(float arg)
{
	return std::floor(arg);
}
inline float modff(float x, float* y)
{
	double d;
	float f = (float)modf((double)x, &d);
	*y = (float)d;
	return f;
}
inline float sqrtf(float x)
{
	return sqrt(x);
}
#endif
#ifndef _WIN32_WCE
float fracf(float v)
{
	float intPart;
	return modff(v, &intPart);
}
#else
double frac(double v)
{
	return v;
}
#endif
#define ARRAY_CACHE 1000
#define MAP_CACHE 1003

namespace Tomahawk
{
	namespace Script
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

		void STDString::Construct(std::string* Current)
		{
			TH_ASSERT_V(Current != nullptr, "current should be set");
			new(Current) std::string();
		}
		void STDString::CopyConstruct(const std::string& Other, std::string* Current)
		{
			TH_ASSERT_V(Current != nullptr, "current should be set");
			new(Current) std::string(Other);
		}
		void STDString::Destruct(std::string* Current)
		{
			TH_ASSERT_V(Current != nullptr, "current should be set");
			Current->~basic_string();
		}
		std::string& STDString::AddAssignTo(const std::string& Current, std::string& Dest)
		{
			Dest += Current;
			return Dest;
		}
		bool STDString::IsEmpty(const std::string& Current)
		{
			return Current.empty();
		}
		void* STDString::ToPtr(const std::string& Value)
		{
			return (void*)Value.c_str();
		}
		std::string STDString::Reverse(const std::string& Value)
		{
			Core::Parser Result(Value);
			Result.Reverse();
			return Result.R();
		}
		std::string& STDString::AssignUInt64To(as_uint64_t i, std::string& Dest)
		{
			std::ostringstream stream;
			stream << i;
			Dest = stream.str();
			return Dest;
		}
		std::string& STDString::AddAssignUInt64To(as_uint64_t i, std::string& Dest)
		{
			std::ostringstream stream;
			stream << i;
			Dest += stream.str();
			return Dest;
		}
		std::string STDString::AddUInt641(const std::string& Current, as_uint64_t i)
		{
			std::ostringstream stream;
			stream << i;
			return Current + stream.str();
		}
		std::string STDString::AddInt641(as_int64_t i, const std::string& Current)
		{
			std::ostringstream stream;
			stream << i;
			return stream.str() + Current;
		}
		std::string& STDString::AssignInt64To(as_int64_t i, std::string& Dest)
		{
			std::ostringstream stream;
			stream << i;
			Dest = stream.str();
			return Dest;
		}
		std::string& STDString::AddAssignInt64To(as_int64_t i, std::string& Dest)
		{
			std::ostringstream stream;
			stream << i;
			Dest += stream.str();
			return Dest;
		}
		std::string STDString::AddInt642(const std::string& Current, as_int64_t i)
		{
			std::ostringstream stream;
			stream << i;
			return Current + stream.str();
		}
		std::string STDString::AddUInt642(as_uint64_t i, const std::string& Current)
		{
			std::ostringstream stream;
			stream << i;
			return stream.str() + Current;
		}
		std::string& STDString::AssignDoubleTo(double f, std::string& Dest)
		{
			std::ostringstream stream;
			stream << f;
			Dest = stream.str();
			return Dest;
		}
		std::string& STDString::AddAssignDoubleTo(double f, std::string& Dest)
		{
			std::ostringstream stream;
			stream << f;
			Dest += stream.str();
			return Dest;
		}
		std::string& STDString::AssignFloatTo(float f, std::string& Dest)
		{
			std::ostringstream stream;
			stream << f;
			Dest = stream.str();
			return Dest;
		}
		std::string& STDString::AddAssignFloatTo(float f, std::string& Dest)
		{
			std::ostringstream stream;
			stream << f;
			Dest += stream.str();
			return Dest;
		}
		std::string& STDString::AssignBoolTo(bool b, std::string& Dest)
		{
			std::ostringstream stream;
			stream << (b ? "true" : "false");
			Dest = stream.str();
			return Dest;
		}
		std::string& STDString::AddAssignBoolTo(bool b, std::string& Dest)
		{
			std::ostringstream stream;
			stream << (b ? "true" : "false");
			Dest += stream.str();
			return Dest;
		}
		std::string STDString::AddDouble1(const std::string& Current, double f)
		{
			std::ostringstream stream;
			stream << f;
			return Current + stream.str();
		}
		std::string STDString::AddDouble2(double f, const std::string& Current)
		{
			std::ostringstream stream;
			stream << f;
			return stream.str() + Current;
		}
		std::string STDString::AddFloat1(const std::string& Current, float f)
		{
			std::ostringstream stream;
			stream << f;
			return Current + stream.str();
		}
		std::string STDString::AddFloat2(float f, const std::string& Current)
		{
			std::ostringstream stream;
			stream << f;
			return stream.str() + Current;
		}
		std::string STDString::AddBool1(const std::string& Current, bool b)
		{
			std::ostringstream stream;
			stream << (b ? "true" : "false");
			return Current + stream.str();
		}
		std::string STDString::AddBool2(bool b, const std::string& Current)
		{
			std::ostringstream stream;
			stream << (b ? "true" : "false");
			return stream.str() + Current;
		}
		char* STDString::CharAt(unsigned int i, std::string& Current)
		{
			if (i >= Current.size())
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx != nullptr)
					ctx->SetException("Out of range");

				return 0;
			}

			return &Current[i];
		}
		int STDString::Cmp(const std::string& a, const std::string& b)
		{
			int cmp = 0;
			if (a < b)
				cmp = -1;
			else if (a > b)
				cmp = 1;

			return cmp;
		}
		int STDString::FindFirst(const std::string& Substr, as_size_t Start, const std::string& Current)
		{
			return (int)Current.find(Substr, (size_t)Start);
		}
		int STDString::FindFirstOf(const std::string& Substr, as_size_t Start, const std::string& Current)
		{
			return (int)Current.find_first_of(Substr, (size_t)Start);
		}
		int STDString::FindLastOf(const std::string& Substr, as_size_t Start, const std::string& Current)
		{
			return (int)Current.find_last_of(Substr, (size_t)Start);
		}
		int STDString::FindFirstNotOf(const std::string& Substr, as_size_t Start, const std::string& Current)
		{
			return (int)Current.find_first_not_of(Substr, (size_t)Start);
		}
		int STDString::FindLastNotOf(const std::string& Substr, as_size_t Start, const std::string& Current)
		{
			return (int)Current.find_last_not_of(Substr, (size_t)Start);
		}
		int STDString::FindLast(const std::string& Substr, int Start, const std::string& Current)
		{
			return (int)Current.rfind(Substr, (size_t)Start);
		}
		void STDString::Insert(unsigned int Offset, const std::string& Other, std::string& Current)
		{
			Current.insert(Offset, Other);
		}
		void STDString::Erase(unsigned int Offset, int count, std::string& Current)
		{
			Current.erase(Offset, (size_t)(count < 0 ? std::string::npos : count));
		}
		as_size_t STDString::Length(const std::string& Current)
		{
			return (as_size_t)Current.length();
		}
		void STDString::Resize(as_size_t l, std::string& Current)
		{
			Current.resize(l);
		}
		std::string STDString::Replace(const std::string& a, const std::string& b, uint64_t o, const std::string& Base)
		{
			return Tomahawk::Core::Parser(Base).Replace(a, b, o).R();
		}
		as_int64_t STDString::IntStore(const std::string& Value, as_size_t Base, as_size_t* ByteCount)
		{
			if (Base != 10 && Base != 16)
			{
				if (ByteCount)
					*ByteCount = 0;

				return 0;
			}

			const char* end = &Value[0];
			bool sign = false;
			if (*end == '-')
			{
				sign = true;
				end++;
			}
			else if (*end == '+')
				end++;

			as_int64_t res = 0;
			if (Base == 10)
			{
				while (*end >= '0' && *end <= '9')
				{
					res *= 10;
					res += *end++ - '0';
				}
			}
			else
			{
				while ((*end >= '0' && *end <= '9') || (*end >= 'a' && *end <= 'f') || (*end >= 'A' && *end <= 'F'))
				{
					res *= 16;
					if (*end >= '0' && *end <= '9')
						res += *end++ - '0';
					else if (*end >= 'a' && *end <= 'f')
						res += *end++ - 'a' + 10;
					else if (*end >= 'A' && *end <= 'F')
						res += *end++ - 'A' + 10;
				}
			}

			if (ByteCount)
				*ByteCount = as_size_t(size_t(end - Value.c_str()));

			if (sign)
				res = -res;

			return res;
		}
		as_uint64_t STDString::UIntStore(const std::string& Value, as_size_t Base, as_size_t* ByteCount)
		{
			if (Base != 10 && Base != 16)
			{
				if (ByteCount)
					*ByteCount = 0;
				return 0;
			}

			const char* end = &Value[0];
			as_uint64_t res = 0;

			if (Base == 10)
			{
				while (*end >= '0' && *end <= '9')
				{
					res *= 10;
					res += *end++ - '0';
				}
			}
			else
			{
				while ((*end >= '0' && *end <= '9') || (*end >= 'a' && *end <= 'f') || (*end >= 'A' && *end <= 'F'))
				{
					res *= 16;
					if (*end >= '0' && *end <= '9')
						res += *end++ - '0';
					else if (*end >= 'a' && *end <= 'f')
						res += *end++ - 'a' + 10;
					else if (*end >= 'A' && *end <= 'F')
						res += *end++ - 'A' + 10;
				}
			}

			if (ByteCount)
				*ByteCount = as_size_t(size_t(end - Value.c_str()));

			return res;
		}
		double STDString::FloatStore(const std::string& Value, as_size_t* ByteCount)
		{
			char* end;
#if !defined(_WIN32_WCE) && !defined(ANDROID) && !defined(__psp2__)
			char* tmp = setlocale(LC_NUMERIC, 0);
			std::string orig = tmp ? tmp : "C";
			setlocale(LC_NUMERIC, "C");
#endif

			double res = strtod(Value.c_str(), &end);
#if !defined(_WIN32_WCE) && !defined(ANDROID) && !defined(__psp2__)
			setlocale(LC_NUMERIC, orig.c_str());
#endif
			if (ByteCount)
				*ByteCount = as_size_t(size_t(end - Value.c_str()));

			return res;
		}
		std::string STDString::Sub(as_size_t Start, int count, const std::string& Current)
		{
			std::string ret;
			if (Start < Current.length() && count != 0)
				ret = Current.substr(Start, (size_t)(count < 0 ? std::string::npos : count));

			return ret;
		}
		bool STDString::Equals(const std::string& lhs, const std::string& rhs)
		{
			return lhs == rhs;
		}
		std::string STDString::ToLower(const std::string& Symbol)
		{
			return Tomahawk::Core::Parser(Symbol).ToLower().R();
		}
		std::string STDString::ToUpper(const std::string& Symbol)
		{
			return Tomahawk::Core::Parser(Symbol).ToUpper().R();
		}
		std::string STDString::ToInt8(char Value)
		{
			return std::to_string(Value);
		}
		std::string STDString::ToInt16(short Value)
		{
			return std::to_string(Value);
		}
		std::string STDString::ToInt(int Value)
		{
			return std::to_string(Value);
		}
		std::string STDString::ToInt64(int64_t Value)
		{
			return std::to_string(Value);
		}
		std::string STDString::ToUInt8(unsigned char Value)
		{
			return std::to_string(Value);
		}
		std::string STDString::ToUInt16(unsigned short Value)
		{
			return std::to_string(Value);
		}
		std::string STDString::ToUInt(unsigned int Value)
		{
			return std::to_string(Value);
		}
		std::string STDString::ToUInt64(uint64_t Value)
		{
			return std::to_string(Value);
		}
		std::string STDString::ToFloat(float Value)
		{
			return std::to_string(Value);
		}
		std::string STDString::ToDouble(double Value)
		{
			return std::to_string(Value);
		}
		STDArray* STDString::Split(const std::string& delim, const std::string& Current)
		{
			VMCContext* ctx = asGetActiveContext();
			VMCManager* engine = ctx->GetEngine();
			asITypeInfo* arrayType = engine->GetTypeInfoByDecl("Array<String>@");
			STDArray* array = STDArray::Create(arrayType);

			int Offset = 0, prev = 0, count = 0;
			while ((Offset = (int)Current.find(delim, prev)) != (int)std::string::npos)
			{
				array->Resize(array->GetSize() + 1);
				((std::string*)array->At(count))->assign(&Current[prev], Offset - prev);
				count++;
				prev = Offset + (int)delim.length();
			}

			array->Resize(array->GetSize() + 1);
			((std::string*)array->At(count))->assign(&Current[prev]);
			return array;
		}
		std::string STDString::Join(const STDArray& array, const std::string& delim)
		{
			std::string Current = "";
			if (!array.GetSize())
				return Current;

			int n;
			for (n = 0; n < (int)array.GetSize() - 1; n++)
			{
				Current += *(std::string*)array.At(n);
				Current += delim;
			}

			Current += *(std::string*)array.At(n);
			return Current;
		}
		char STDString::ToChar(const std::string& Symbol)
		{
			return Symbol.empty() ? '\0' : Symbol[0];
		}

		STDMutex::STDMutex() : Ref(1)
		{
		}
		void STDMutex::Release()
		{
			if (asAtomicDec(Ref) <= 0)
			{
				this->~STDMutex();
				asFreeMem((void*)this);
			}
		}
		void STDMutex::AddRef()
		{
			asAtomicInc(Ref);
		}
		bool STDMutex::TryLock()
		{
			return Base.try_lock();
		}
		void STDMutex::Lock()
		{
			Base.lock();
		}
		void STDMutex::Unlock()
		{
			Base.unlock();
		}
		STDMutex* STDMutex::Factory()
		{
			void* Data = asAllocMem(sizeof(STDMutex));
			if (!Data)
			{
				VMCContext* Context = asGetActiveContext();
				if (Context != nullptr)
					Context->SetException("Out of memory");

				return nullptr;
			}

			return new(Data) STDMutex();
		}

		float STDMath::FpFromIEEE(as_size_t raw)
		{
			return *reinterpret_cast<float*>(&raw);
		}
		as_size_t STDMath::FpToIEEE(float fp)
		{
			return *reinterpret_cast<as_size_t*>(&fp);
		}
		double STDMath::FpFromIEEE(as_uint64_t raw)
		{
			return *reinterpret_cast<double*>(&raw);
		}
		as_uint64_t STDMath::FpToIEEE(double fp)
		{
			return *reinterpret_cast<as_uint64_t*>(&fp);
		}
		bool STDMath::CloseTo(float a, float b, float epsilon)
		{
			if (a == b)
				return true;

			float diff = fabsf(a - b);
			if ((a == 0 || b == 0) && (diff < epsilon))
				return true;

			return diff / (fabs(a) + fabs(b)) < epsilon;
		}
		bool STDMath::CloseTo(double a, double b, double epsilon)
		{
			if (a == b)
				return true;

			double diff = fabs(a - b);
			if ((a == 0 || b == 0) && (diff < epsilon))
				return true;

			return diff / (fabs(a) + fabs(b)) < epsilon;
		}

		void STDException::Throw(const std::string& In)
		{
			VMCContext* Context = asGetActiveContext();
			if (Context != nullptr)
				Context->SetException(In.empty() ? "runtime exception" : In.c_str());
		}
		std::string STDException::GetException()
		{
			VMCContext* Context = asGetActiveContext();
			if (!Context)
				return "";

			const char* Message = Context->GetExceptionString();
			if (!Message)
				return "";

			return Message;
		}

		STDAny::STDAny(VMCManager* fEngine)
		{
			Engine = fEngine;
			RefCount = 1;
			GCFlag = false;
			Value.TypeId = 0;
			Value.ValueInt = 0;

			Engine->NotifyGarbageCollectorOfNewObject(this, Engine->GetTypeInfoByName("Any"));
		}
		STDAny::STDAny(void* Ref, int RefTypeId, VMCManager* fEngine)
		{
			Engine = fEngine;
			RefCount = 1;
			GCFlag = false;
			Value.TypeId = 0;
			Value.ValueInt = 0;

			Engine->NotifyGarbageCollectorOfNewObject(this, Engine->GetTypeInfoByName("Any"));
			Store(Ref, RefTypeId);
		}
		STDAny::STDAny(const STDAny& Other)
		{
			Engine = Other.Engine;
			RefCount = 1;
			GCFlag = false;
			Value.TypeId = 0;
			Value.ValueInt = 0;

			Engine->NotifyGarbageCollectorOfNewObject(this, Engine->GetTypeInfoByName("Any"));
			if ((Other.Value.TypeId & asTYPEID_MASK_OBJECT))
			{
				VMCTypeInfo* T = Engine->GetTypeInfoById(Other.Value.TypeId);
				if (T)
					T->AddRef();
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
		STDAny::~STDAny()
		{
			FreeObject();
		}
		STDAny& STDAny::operator=(const STDAny& Other)
		{
			if ((Other.Value.TypeId & asTYPEID_MASK_OBJECT))
			{
				VMCTypeInfo* T = Engine->GetTypeInfoById(Other.Value.TypeId);
				if (T)
					T->AddRef();
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
		int STDAny::CopyFrom(const STDAny* Other)
		{
			if (Other == 0)
				return asINVALID_ARG;

			*this = *(STDAny*)Other;
			return 0;
		}
		void STDAny::Store(void* Ref, int RefTypeId)
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
		bool STDAny::Retrieve(void* Ref, int RefTypeId) const
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
		int STDAny::GetTypeId() const
		{
			return Value.TypeId;
		}
		void STDAny::FreeObject()
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
		void STDAny::EnumReferences(VMCManager* InEngine)
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
		void STDAny::ReleaseAllHandles(VMCManager*)
		{
			FreeObject();
		}
		int STDAny::AddRef() const
		{
			GCFlag = false;
			return asAtomicInc(RefCount);
		}
		int STDAny::Release() const
		{
			GCFlag = false;
			if (asAtomicDec(RefCount) == 0)
			{
				this->~STDAny();
				asFreeMem((void*)this);
				return 0;
			}

			return RefCount;
		}
		int STDAny::GetRefCount()
		{
			return RefCount;
		}
		void STDAny::SetFlag()
		{
			GCFlag = true;
		}
		bool STDAny::GetFlag()
		{
			return GCFlag;
		}
		STDAny* STDAny::Create(int TypeId, void* Ref)
		{
			VMCContext* Context = asGetActiveContext();
			if (!Context)
				return nullptr;

			void* Data = asAllocMem(sizeof(STDAny));
			return new(Data) STDAny(Ref, TypeId, Context->GetEngine());
		}
		STDAny* STDAny::Create(const char* Decl, void* Ref)
		{
			VMCContext* Context = asGetActiveContext();
			if (!Context)
				return nullptr;

			VMCManager* Engine = Context->GetEngine();
			void* Data = asAllocMem(sizeof(STDAny));
			return new(Data) STDAny(Ref, Engine->GetTypeIdByDecl(Decl), Engine);
		}
		void STDAny::Factory1(VMCGeneric* G)
		{
			VMCManager* Engine = G->GetEngine();
			void* Mem = asAllocMem(sizeof(STDAny));
			*(STDAny**)G->GetAddressOfReturnLocation() = new(Mem) STDAny(Engine);
		}
		void STDAny::Factory2(VMCGeneric* G)
		{
			VMCManager* Engine = G->GetEngine();
			void* Ref = (void*)G->GetArgAddress(0);
			void* Mem = asAllocMem(sizeof(STDAny));
			int RefType = G->GetArgTypeId(0);

			*(STDAny**)G->GetAddressOfReturnLocation() = new(Mem) STDAny(Ref, RefType, Engine);
		}
		STDAny& STDAny::Assignment(STDAny* Other, STDAny* Self)
		{
			return *Self = *Other;
		}

		STDArray& STDArray::operator=(const STDArray& Other)
		{
			if (&Other != this && Other.GetArrayObjectType() == GetArrayObjectType())
			{
				Resize(Other.Buffer->NumElements);
				CopyBuffer(Buffer, Other.Buffer);
			}

			return *this;
		}
		STDArray::STDArray(VMCTypeInfo* TI, void* buf)
		{
			assert(TI && std::string(TI->GetName()) == "Array");
			RefCount = 1;
			GCFlag = false;
			ObjType = TI;
			ObjType->AddRef();
			Buffer = 0;
			Precache();

			VMCManager* engine = TI->GetEngine();
			if (SubTypeId & asTYPEID_MASK_OBJECT)
				ElementSize = sizeof(asPWORD);
			else
				ElementSize = engine->GetSizeOfPrimitiveType(SubTypeId);

			as_size_t length = *(as_size_t*)buf;
			if (!CheckMaxSize(length))
				return;

			if ((TI->GetSubTypeId() & asTYPEID_MASK_OBJECT) == 0)
			{
				CreateBuffer(&Buffer, length);
				if (length > 0)
					memcpy(At(0), (((as_size_t*)buf) + 1), (size_t)length * (size_t)ElementSize);
			}
			else if (TI->GetSubTypeId() & asTYPEID_OBJHANDLE)
			{
				CreateBuffer(&Buffer, length);
				if (length > 0)
					memcpy(At(0), (((as_size_t*)buf) + 1), (size_t)length * (size_t)ElementSize);

				memset((((as_size_t*)buf) + 1), 0, (size_t)length * (size_t)ElementSize);
			}
			else if (TI->GetSubType()->GetFlags() & asOBJ_REF)
			{
				SubTypeId |= asTYPEID_OBJHANDLE;
				CreateBuffer(&Buffer, length);
				SubTypeId &= ~asTYPEID_OBJHANDLE;

				if (length > 0)
					memcpy(Buffer->Data, (((as_size_t*)buf) + 1), (size_t)length * (size_t)ElementSize);

				memset((((as_size_t*)buf) + 1), 0, (size_t)length * (size_t)ElementSize);
			}
			else
			{
				CreateBuffer(&Buffer, length);
				for (as_size_t n = 0; n < length; n++)
				{
					void* obj = At(n);
					unsigned char* srcObj = (unsigned char*)buf;
					srcObj += 4 + n * TI->GetSubType()->GetSize();
					engine->AssignScriptObject(obj, srcObj, TI->GetSubType());
				}
			}

			if (ObjType->GetFlags() & asOBJ_GC)
				ObjType->GetEngine()->NotifyGarbageCollectorOfNewObject(this, ObjType);
		}
		STDArray::STDArray(as_size_t length, VMCTypeInfo* TI)
		{
			assert(TI && std::string(TI->GetName()) == "Array");
			RefCount = 1;
			GCFlag = false;
			ObjType = TI;
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
		STDArray::STDArray(const STDArray& Other)
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
		STDArray::STDArray(as_size_t length, void* defVal, VMCTypeInfo* TI)
		{
			assert(TI && std::string(TI->GetName()) == "Array");
			RefCount = 1;
			GCFlag = false;
			ObjType = TI;
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

			for (as_size_t n = 0; n < GetSize(); n++)
				SetValue(n, defVal);
		}
		void STDArray::SetValue(as_size_t index, void* value)
		{
			void* ptr = At(index);
			if (ptr == 0)
				return;

			if ((SubTypeId & ~asTYPEID_MASK_SEQNBR) && !(SubTypeId & asTYPEID_OBJHANDLE))
				ObjType->GetEngine()->AssignScriptObject(ptr, value, ObjType->GetSubType());
			else if (SubTypeId & asTYPEID_OBJHANDLE)
			{
				void* tmp = *(void**)ptr;
				*(void**)ptr = *(void**)value;
				ObjType->GetEngine()->AddRefScriptObject(*(void**)value, ObjType->GetSubType());
				if (tmp)
					ObjType->GetEngine()->ReleaseScriptObject(tmp, ObjType->GetSubType());
			}
			else if (SubTypeId == asTYPEID_BOOL ||
				SubTypeId == asTYPEID_INT8 ||
				SubTypeId == asTYPEID_UINT8)
				*(char*)ptr = *(char*)value;
			else if (SubTypeId == asTYPEID_INT16 ||
				SubTypeId == asTYPEID_UINT16)
				*(short*)ptr = *(short*)value;
			else if (SubTypeId == asTYPEID_INT32 ||
				SubTypeId == asTYPEID_UINT32 ||
				SubTypeId == asTYPEID_FLOAT ||
				SubTypeId > asTYPEID_DOUBLE)
				*(int*)ptr = *(int*)value;
			else if (SubTypeId == asTYPEID_INT64 ||
				SubTypeId == asTYPEID_UINT64 ||
				SubTypeId == asTYPEID_DOUBLE)
				*(double*)ptr = *(double*)value;
		}
		STDArray::~STDArray()
		{
			if (Buffer)
			{
				DeleteBuffer(Buffer);
				Buffer = 0;
			}
			if (ObjType)
				ObjType->Release();
		}
		as_size_t STDArray::GetSize() const
		{
			return Buffer->NumElements;
		}
		bool STDArray::IsEmpty() const
		{
			return Buffer->NumElements == 0;
		}
		void STDArray::Reserve(as_size_t MaxElements)
		{
			if (MaxElements <= Buffer->MaxElements)
				return;

			if (!CheckMaxSize(MaxElements))
				return;

			SBuffer* newBuffer = reinterpret_cast<SBuffer*>(asAllocMem(sizeof(SBuffer) - 1 + (size_t)ElementSize * (size_t)MaxElements));
			if (newBuffer)
			{
				newBuffer->NumElements = Buffer->NumElements;
				newBuffer->MaxElements = MaxElements;
			}
			else
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Out of memory");
				return;
			}

			memcpy(newBuffer->Data, Buffer->Data, (size_t)Buffer->NumElements * (size_t)ElementSize);
			asFreeMem(Buffer);
			Buffer = newBuffer;
		}
		void STDArray::Resize(as_size_t NumElements)
		{
			if (!CheckMaxSize(NumElements))
				return;

			Resize((int)NumElements - (int)Buffer->NumElements, (as_size_t)-1);
		}
		void STDArray::RemoveRange(as_size_t Start, as_size_t count)
		{
			if (count == 0)
				return;

			if (Buffer == 0 || Start > Buffer->NumElements)
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Index out of bounds");
				return;
			}

			if (Start + count > Buffer->NumElements)
				count = Buffer->NumElements - Start;

			Destruct(Buffer, Start, Start + count);
			memmove(Buffer->Data + Start * (as_size_t)ElementSize, Buffer->Data + (Start + count) * (as_size_t)ElementSize, (size_t)(Buffer->NumElements - Start - count) * (size_t)ElementSize);
			Buffer->NumElements -= count;
		}
		void STDArray::Resize(int delta, as_size_t at)
		{
			if (delta < 0)
			{
				if (-delta > (int)Buffer->NumElements)
					delta = -(int)Buffer->NumElements;
				if (at > Buffer->NumElements + delta)
					at = Buffer->NumElements + delta;
			}
			else if (delta > 0)
			{
				if (!CheckMaxSize(Buffer->NumElements + delta))
					return;

				if (at > Buffer->NumElements)
					at = Buffer->NumElements;
			}

			if (delta == 0)
				return;

			if (Buffer->MaxElements < Buffer->NumElements + delta)
			{
				size_t Count = (size_t)Buffer->NumElements + (size_t)delta, Size = (size_t)ElementSize;
				SBuffer* newBuffer = reinterpret_cast<SBuffer*>(asAllocMem(sizeof(SBuffer) - 1 + Size * Count));
				if (newBuffer)
				{
					newBuffer->NumElements = Buffer->NumElements + delta;
					newBuffer->MaxElements = newBuffer->NumElements;
				}
				else
				{
					VMCContext* ctx = asGetActiveContext();
					if (ctx)
						ctx->SetException("Out of memory");
					return;
				}

				memcpy(newBuffer->Data, Buffer->Data, (size_t)at * (size_t)ElementSize);
				if (at < Buffer->NumElements)
					memcpy(newBuffer->Data + (at + delta) * (as_size_t)ElementSize, Buffer->Data + at * (as_size_t)ElementSize, (size_t)(Buffer->NumElements - at) * (size_t)ElementSize);

				Construct(newBuffer, at, at + delta);
				asFreeMem(Buffer);
				Buffer = newBuffer;
			}
			else if (delta < 0)
			{
				Destruct(Buffer, at, at - delta);
				memmove(Buffer->Data + at * (as_size_t)ElementSize, Buffer->Data + (at - delta) * (as_size_t)ElementSize, (size_t)(Buffer->NumElements - (at - delta)) * (size_t)ElementSize);
				Buffer->NumElements += delta;
			}
			else
			{
				memmove(Buffer->Data + (at + delta) * (as_size_t)ElementSize, Buffer->Data + at * (as_size_t)ElementSize, (size_t)(Buffer->NumElements - at) * (size_t)ElementSize);
				Construct(Buffer, at, at + delta);
				Buffer->NumElements += delta;
			}
		}
		bool STDArray::CheckMaxSize(as_size_t NumElements)
		{
			as_size_t maxSize = 0xFFFFFFFFul - sizeof(SBuffer) + 1;
			if (ElementSize > 0)
				maxSize /= (as_size_t)ElementSize;

			if (NumElements > maxSize)
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Too large Array size");

				return false;
			}

			return true;
		}
		VMCTypeInfo* STDArray::GetArrayObjectType() const
		{
			return ObjType;
		}
		int STDArray::GetArrayTypeId() const
		{
			return ObjType->GetTypeId();
		}
		int STDArray::GetElementTypeId() const
		{
			return SubTypeId;
		}
		void STDArray::InsertAt(as_size_t index, void* value)
		{
			if (index > Buffer->NumElements)
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Index out of bounds");
				return;
			}

			Resize(1, index);
			SetValue(index, value);
		}
		void STDArray::InsertAt(as_size_t index, const STDArray& arr)
		{
			if (index > Buffer->NumElements)
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Index out of bounds");
				return;
			}

			if (ObjType != arr.ObjType)
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Mismatching Array types");
				return;
			}

			as_size_t elements = arr.GetSize();
			Resize(elements, index);
			if (&arr != this)
			{
				for (as_size_t n = 0; n < arr.GetSize(); n++)
				{
					void* value = const_cast<void*>(arr.At(n));
					SetValue(index + n, value);
				}
			}
			else
			{
				for (as_size_t n = 0; n < index; n++)
				{
					void* value = const_cast<void*>(arr.At(n));
					SetValue(index + n, value);
				}

				for (as_size_t n = index + elements, m = 0; n < arr.GetSize(); n++, m++)
				{
					void* value = const_cast<void*>(arr.At(n));
					SetValue(index + index + m, value);
				}
			}
		}
		void STDArray::InsertLast(void* value)
		{
			InsertAt(Buffer->NumElements, value);
		}
		void STDArray::RemoveAt(as_size_t index)
		{
			if (index >= Buffer->NumElements)
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Index out of bounds");
				return;
			}

			Resize(-1, index);
		}
		void STDArray::RemoveLast()
		{
			RemoveAt(Buffer->NumElements - 1);
		}
		const void* STDArray::At(as_size_t index) const
		{
			if (Buffer == 0 || index >= Buffer->NumElements)
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Index out of bounds");
				return 0;
			}

			if ((SubTypeId & asTYPEID_MASK_OBJECT) && !(SubTypeId & asTYPEID_OBJHANDLE))
				return *(void**)(Buffer->Data + (as_size_t)ElementSize * index);
			else
				return Buffer->Data + (as_size_t)ElementSize * index;
		}
		void* STDArray::At(as_size_t index)
		{
			return const_cast<void*>(const_cast<const STDArray*>(this)->At(index));
		}
		void* STDArray::GetBuffer()
		{
			return Buffer->Data;
		}
		void STDArray::CreateBuffer(SBuffer** buf, as_size_t NumElements)
		{
			*buf = reinterpret_cast<SBuffer*>(asAllocMem(sizeof(SBuffer) - 1 + (size_t)ElementSize * (size_t)NumElements));
			if (*buf)
			{
				(*buf)->NumElements = NumElements;
				(*buf)->MaxElements = NumElements;
				Construct(*buf, 0, NumElements);
			}
			else
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Out of memory");
			}
		}
		void STDArray::DeleteBuffer(SBuffer* buf)
		{
			Destruct(buf, 0, buf->NumElements);
			asFreeMem(buf);
		}
		void STDArray::Construct(SBuffer* buf, as_size_t Start, as_size_t end)
		{
			if ((SubTypeId & asTYPEID_MASK_OBJECT) && !(SubTypeId & asTYPEID_OBJHANDLE))
			{
				void** max = (void**)(buf->Data + end * sizeof(void*));
				void** d = (void**)(buf->Data + Start * sizeof(void*));

				VMCManager* engine = ObjType->GetEngine();
				VMCTypeInfo* subType = ObjType->GetSubType();

				for (; d < max; d++)
				{
					*d = (void*)engine->CreateScriptObject(subType);
					if (*d == 0)
					{
						memset(d, 0, sizeof(void*) * (max - d));
						return;
					}
				}
			}
			else
			{
				void* d = (void*)(buf->Data + Start * (as_size_t)ElementSize);
				memset(d, 0, (size_t)(end - Start) * (size_t)ElementSize);
			}
		}
		void STDArray::Destruct(SBuffer* buf, as_size_t Start, as_size_t end)
		{
			if (SubTypeId & asTYPEID_MASK_OBJECT)
			{
				VMCManager* engine = ObjType->GetEngine();
				void** max = (void**)(buf->Data + end * sizeof(void*));
				void** d = (void**)(buf->Data + Start * sizeof(void*));

				for (; d < max; d++)
				{
					if (*d)
						engine->ReleaseScriptObject(*d, ObjType->GetSubType());
				}
			}
		}
		bool STDArray::Less(const void* a, const void* b, bool asc, VMCContext* ctx, SCache* cache)
		{
			if (!asc)
			{
				const void* TEMP = a;
				a = b;
				b = TEMP;
			}

			if (!(SubTypeId & ~asTYPEID_MASK_SEQNBR))
			{
				switch (SubTypeId)
				{
#define COMPARE(T) *((T*)a) < *((T*)b)
					case asTYPEID_BOOL: return COMPARE(bool);
					case asTYPEID_INT8: return COMPARE(signed char);
					case asTYPEID_UINT8: return COMPARE(unsigned char);
					case asTYPEID_INT16: return COMPARE(signed short);
					case asTYPEID_UINT16: return COMPARE(unsigned short);
					case asTYPEID_INT32: return COMPARE(signed int);
					case asTYPEID_UINT32: return COMPARE(unsigned int);
					case asTYPEID_FLOAT: return COMPARE(float);
					case asTYPEID_DOUBLE: return COMPARE(double);
					default: return COMPARE(signed int); // All enums fall in this case
#undef COMPARE
				}
			}
			else
			{
				int r = 0;
				if (SubTypeId & asTYPEID_OBJHANDLE)
				{
					if (*(void**)a == 0)
						return true;

					if (*(void**)b == 0)
						return false;
				}

				if (cache && cache->CmpFunc)
				{
					r = ctx->Prepare(cache->CmpFunc); assert(r >= 0);
					if (SubTypeId & asTYPEID_OBJHANDLE)
					{
						r = ctx->SetObject(*((void**)a)); assert(r >= 0);
						r = ctx->SetArgObject(0, *((void**)b)); assert(r >= 0);
					}
					else
					{
						r = ctx->SetObject((void*)a); assert(r >= 0);
						r = ctx->SetArgObject(0, (void*)b); assert(r >= 0);
					}

					r = ctx->Execute();
					if (r == asEXECUTION_FINISHED)
						return (int)ctx->GetReturnDWord() < 0;
				}
			}

			return false;
		}
		void STDArray::Reverse()
		{
			as_size_t size = GetSize();
			if (size >= 2)
			{
				unsigned char TEMP[16];
				for (as_size_t i = 0; i < size / 2; i++)
				{
					Copy(TEMP, GetArrayItemPointer(i));
					Copy(GetArrayItemPointer(i), GetArrayItemPointer(size - i - 1));
					Copy(GetArrayItemPointer(size - i - 1), TEMP);
				}
			}
		}
		bool STDArray::operator==(const STDArray& Other) const
		{
			if (ObjType != Other.ObjType)
				return false;

			if (GetSize() != Other.GetSize())
				return false;

			VMCContext* cmpContext = 0;
			bool isNested = false;

			if (SubTypeId & ~asTYPEID_MASK_SEQNBR)
			{
				cmpContext = asGetActiveContext();
				if (cmpContext)
				{
					if (cmpContext->GetEngine() == ObjType->GetEngine() && cmpContext->PushState() >= 0)
						isNested = true;
					else
						cmpContext = 0;
				}

				if (cmpContext == 0)
					cmpContext = ObjType->GetEngine()->CreateContext();
			}

			bool isEqual = true;
			SCache* cache = reinterpret_cast<SCache*>(ObjType->GetUserData(ARRAY_CACHE));
			for (as_size_t n = 0; n < GetSize(); n++)
			{
				if (!Equals(At(n), Other.At(n), cmpContext, cache))
				{
					isEqual = false;
					break;
				}
			}

			if (cmpContext)
			{
				if (isNested)
				{
					asEContextState state = cmpContext->GetState();
					cmpContext->PopState();
					if (state == asEXECUTION_ABORTED)
						cmpContext->Abort();
				}
				else
					cmpContext->Release();
			}

			return isEqual;
		}
		bool STDArray::Equals(const void* a, const void* b, VMCContext* ctx, SCache* cache) const
		{
			if (!(SubTypeId & ~asTYPEID_MASK_SEQNBR))
			{
				switch (SubTypeId)
				{
#define COMPARE(T) *((T*)a) == *((T*)b)
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
				int r = 0;
				if (SubTypeId & asTYPEID_OBJHANDLE)
				{
					if (*(void**)a == *(void**)b)
						return true;
				}

				if (cache && cache->EqFunc)
				{
					r = ctx->Prepare(cache->EqFunc); assert(r >= 0);

					if (SubTypeId & asTYPEID_OBJHANDLE)
					{
						r = ctx->SetObject(*((void**)a)); assert(r >= 0);
						r = ctx->SetArgObject(0, *((void**)b)); assert(r >= 0);
					}
					else
					{
						r = ctx->SetObject((void*)a); assert(r >= 0);
						r = ctx->SetArgObject(0, (void*)b); assert(r >= 0);
					}

					r = ctx->Execute();
					if (r == asEXECUTION_FINISHED)
						return ctx->GetReturnByte() != 0;

					return false;
				}

				if (cache && cache->CmpFunc)
				{
					r = ctx->Prepare(cache->CmpFunc); assert(r >= 0);
					if (SubTypeId & asTYPEID_OBJHANDLE)
					{
						r = ctx->SetObject(*((void**)a)); assert(r >= 0);
						r = ctx->SetArgObject(0, *((void**)b)); assert(r >= 0);
					}
					else
					{
						r = ctx->SetObject((void*)a); assert(r >= 0);
						r = ctx->SetArgObject(0, (void*)b); assert(r >= 0);
					}

					r = ctx->Execute();
					if (r == asEXECUTION_FINISHED)
						return (int)ctx->GetReturnDWord() == 0;

					return false;
				}
			}

			return false;
		}
		int STDArray::FindByRef(void* ref) const
		{
			return FindByRef(0, ref);
		}
		int STDArray::FindByRef(as_size_t startAt, void* ref) const
		{
			as_size_t size = GetSize();
			if (SubTypeId & asTYPEID_OBJHANDLE)
			{
				ref = *(void**)ref;
				for (as_size_t i = startAt; i < size; i++)
				{
					if (*(void**)At(i) == ref)
						return i;
				}
			}
			else
			{
				for (as_size_t i = startAt; i < size; i++)
				{
					if (At(i) == ref)
						return i;
				}
			}

			return -1;
		}
		int STDArray::Find(void* value) const
		{
			return Find(0, value);
		}
		int STDArray::Find(as_size_t startAt, void* value) const
		{
			SCache* cache = 0;
			if (SubTypeId & ~asTYPEID_MASK_SEQNBR)
			{
				cache = reinterpret_cast<SCache*>(ObjType->GetUserData(ARRAY_CACHE));
				if (!cache || (cache->CmpFunc == 0 && cache->EqFunc == 0))
				{
					VMCContext* ctx = asGetActiveContext();
					VMCTypeInfo* subType = ObjType->GetEngine()->GetTypeInfoById(SubTypeId);
					if (ctx)
					{
						char tmp[512];
						if (cache && cache->EqFuncReturnCode == asMULTIPLE_FUNCTIONS)
#if defined(_MSC_VER) && _MSC_VER >= 1500 && !defined(__S3E__)
							sprintf_s(tmp, 512, "Type '%s' has multiple matching opEquals or opCmp methods", subType->GetName());
#else
							sprintf(tmp, "Type '%s' has multiple matching opEquals or opCmp methods", subType->GetName());
#endif
						else
#if defined(_MSC_VER) && _MSC_VER >= 1500 && !defined(__S3E__)
							sprintf_s(tmp, 512, "Type '%s' does not have a matching opEquals or opCmp method", subType->GetName());
#else
							sprintf(tmp, "Type '%s' does not have a matching opEquals or opCmp method", subType->GetName());
#endif
						ctx->SetException(tmp);
					}

					return -1;
				}
			}

			VMCContext* cmpContext = 0;
			bool isNested = false;

			if (SubTypeId & ~asTYPEID_MASK_SEQNBR)
			{
				cmpContext = asGetActiveContext();
				if (cmpContext)
				{
					if (cmpContext->GetEngine() == ObjType->GetEngine() && cmpContext->PushState() >= 0)
						isNested = true;
					else
						cmpContext = 0;
				}

				if (cmpContext == 0)
					cmpContext = ObjType->GetEngine()->CreateContext();
			}

			int ret = -1;
			as_size_t size = GetSize();
			for (as_size_t i = startAt; i < size; i++)
			{
				if (Equals(At(i), value, cmpContext, cache))
				{
					ret = (int)i;
					break;
				}
			}

			if (cmpContext)
			{
				if (isNested)
				{
					asEContextState state = cmpContext->GetState();
					cmpContext->PopState();
					if (state == asEXECUTION_ABORTED)
						cmpContext->Abort();
				}
				else
					cmpContext->Release();
			}

			return ret;
		}
		void STDArray::Copy(void* dst, void* src)
		{
			memcpy(dst, src, ElementSize);
		}
		void* STDArray::GetArrayItemPointer(int index)
		{
			return Buffer->Data + index * ElementSize;
		}
		void* STDArray::GetDataPointer(void* buf)
		{
			if ((SubTypeId & asTYPEID_MASK_OBJECT) && !(SubTypeId & asTYPEID_OBJHANDLE))
				return reinterpret_cast<void*>(*(size_t*)buf);
			else
				return buf;
		}
		void STDArray::SortAsc()
		{
			Sort(0, GetSize(), true);
		}
		void STDArray::SortAsc(as_size_t startAt, as_size_t count)
		{
			Sort(startAt, count, true);
		}
		void STDArray::SortDesc()
		{
			Sort(0, GetSize(), false);
		}
		void STDArray::SortDesc(as_size_t startAt, as_size_t count)
		{
			Sort(startAt, count, false);
		}
		void STDArray::Sort(as_size_t startAt, as_size_t count, bool asc)
		{
			SCache* cache = reinterpret_cast<SCache*>(ObjType->GetUserData(ARRAY_CACHE));
			if (SubTypeId & ~asTYPEID_MASK_SEQNBR)
			{
				if (!cache || cache->CmpFunc == 0)
				{
					VMCContext* ctx = asGetActiveContext();
					VMCTypeInfo* subType = ObjType->GetEngine()->GetTypeInfoById(SubTypeId);
					if (ctx)
					{
						char tmp[512];
						if (cache && cache->CmpFuncReturnCode == asMULTIPLE_FUNCTIONS)
#if defined(_MSC_VER) && _MSC_VER >= 1500 && !defined(__S3E__)
							sprintf_s(tmp, 512, "Type '%s' has multiple matching opCmp methods", subType->GetName());
#else
							sprintf(tmp, "Type '%s' has multiple matching opCmp methods", subType->GetName());
#endif
						else
#if defined(_MSC_VER) && _MSC_VER >= 1500 && !defined(__S3E__)
							sprintf_s(tmp, 512, "Type '%s' does not have a matching opCmp method", subType->GetName());
#else
							sprintf(tmp, "Type '%s' does not have a matching opCmp method", subType->GetName());
#endif
						ctx->SetException(tmp);
					}

					return;
				}
			}

			if (count < 2)
				return;

			int Start = startAt;
			int end = startAt + count;

			if (Start >= (int)Buffer->NumElements || end > (int)Buffer->NumElements)
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Index out of bounds");

				return;
			}

			unsigned char tmp[16];
			VMCContext* cmpContext = 0;
			bool isNested = false;

			if (SubTypeId & ~asTYPEID_MASK_SEQNBR)
			{
				cmpContext = asGetActiveContext();
				if (cmpContext)
				{
					if (cmpContext->GetEngine() == ObjType->GetEngine() && cmpContext->PushState() >= 0)
						isNested = true;
					else
						cmpContext = 0;
				}

				if (cmpContext == 0)
					cmpContext = ObjType->GetEngine()->RequestContext();
			}

			for (int i = Start + 1; i < end; i++)
			{
				Copy(tmp, GetArrayItemPointer(i));
				int j = i - 1;

				while (j >= Start && Less(GetDataPointer(tmp), At(j), asc, cmpContext, cache))
				{
					Copy(GetArrayItemPointer(j + 1), GetArrayItemPointer(j));
					j--;
				}

				Copy(GetArrayItemPointer(j + 1), tmp);
			}

			if (cmpContext)
			{
				if (isNested)
				{
					asEContextState state = cmpContext->GetState();
					cmpContext->PopState();
					if (state == asEXECUTION_ABORTED)
						cmpContext->Abort();
				}
				else
					ObjType->GetEngine()->ReturnContext(cmpContext);
			}
		}
		void STDArray::Sort(asIScriptFunction* func, as_size_t startAt, as_size_t count)
		{
			if (count < 2)
				return;

			as_size_t Start = startAt;
			as_size_t end = as_uint64_t(startAt) + as_uint64_t(count) >= (as_uint64_t(1) << 32) ? 0xFFFFFFFF : startAt + count;
			if (end > Buffer->NumElements)
				end = Buffer->NumElements;

			if (Start >= Buffer->NumElements)
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Index out of bounds");

				return;
			}

			unsigned char tmp[16];
			VMCContext* cmpContext = 0;
			bool isNested = false;

			cmpContext = asGetActiveContext();
			if (cmpContext)
			{
				if (cmpContext->GetEngine() == ObjType->GetEngine() && cmpContext->PushState() >= 0)
					isNested = true;
				else
					cmpContext = 0;
			}

			if (cmpContext == 0)
				cmpContext = ObjType->GetEngine()->RequestContext();

			if (!cmpContext)
				return;

			for (as_size_t i = Start + 1; i < end; i++)
			{
				Copy(tmp, GetArrayItemPointer(i));
				as_size_t j = i - 1;

				while (j != 0xFFFFFFFF && j >= Start)
				{
					cmpContext->Prepare(func);
					cmpContext->SetArgAddress(0, GetDataPointer(tmp));
					cmpContext->SetArgAddress(1, At(j));
					int r = cmpContext->Execute();
					if (r != asEXECUTION_FINISHED)
						break;

					if (*(bool*)(cmpContext->GetAddressOfReturnValue()))
					{
						Copy(GetArrayItemPointer(j + 1), GetArrayItemPointer(j));
						j--;
					}
					else
						break;
				}

				Copy(GetArrayItemPointer(j + 1), tmp);
			}

			if (isNested)
			{
				asEContextState state = cmpContext->GetState();
				cmpContext->PopState();
				if (state == asEXECUTION_ABORTED)
					cmpContext->Abort();
			}
			else
				ObjType->GetEngine()->ReturnContext(cmpContext);
		}
		void STDArray::CopyBuffer(SBuffer* dst, SBuffer* src)
		{
			VMCManager* engine = ObjType->GetEngine();
			if (SubTypeId & asTYPEID_OBJHANDLE)
			{
				if (dst->NumElements > 0 && src->NumElements > 0)
				{
					int count = dst->NumElements > src->NumElements ? src->NumElements : dst->NumElements;
					void** max = (void**)(dst->Data + count * sizeof(void*));
					void** d = (void**)dst->Data;
					void** s = (void**)src->Data;

					for (; d < max; d++, s++)
					{
						void* tmp = *d;
						*d = *s;

						if (*d)
							engine->AddRefScriptObject(*d, ObjType->GetSubType());

						if (tmp)
							engine->ReleaseScriptObject(tmp, ObjType->GetSubType());
					}
				}
			}
			else
			{
				if (dst->NumElements > 0 && src->NumElements > 0)
				{
					int count = dst->NumElements > src->NumElements ? src->NumElements : dst->NumElements;
					if (SubTypeId & asTYPEID_MASK_OBJECT)
					{
						void** max = (void**)(dst->Data + count * sizeof(void*));
						void** d = (void**)dst->Data;
						void** s = (void**)src->Data;

						VMCTypeInfo* subType = ObjType->GetSubType();
						for (; d < max; d++, s++)
							engine->AssignScriptObject(*d, *s, subType);
					}
					else
						memcpy(dst->Data, src->Data, (size_t)count * (size_t)ElementSize);
				}
			}
		}
		void STDArray::Precache()
		{
			SubTypeId = ObjType->GetSubTypeId();
			if (!(SubTypeId & ~asTYPEID_MASK_SEQNBR))
				return;

			SCache* cache = reinterpret_cast<SCache*>(ObjType->GetUserData(ARRAY_CACHE));
			if (cache)
				return;

			asAcquireExclusiveLock();
			cache = reinterpret_cast<SCache*>(ObjType->GetUserData(ARRAY_CACHE));
			if (cache)
			{
				asReleaseExclusiveLock();
				return;
			}

			cache = reinterpret_cast<SCache*>(asAllocMem(sizeof(SCache)));
			if (!cache)
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Out of memory");

				asReleaseExclusiveLock();
				return;
			}

			memset(cache, 0, sizeof(SCache));
			bool mustBeConst = (SubTypeId & asTYPEID_HANDLETOCONST) ? true : false;

			VMCTypeInfo* subType = ObjType->GetEngine()->GetTypeInfoById(SubTypeId);
			if (subType)
			{
				for (as_size_t i = 0; i < subType->GetMethodCount(); i++)
				{
					asIScriptFunction* func = subType->GetMethodByIndex(i);
					if (func->GetParamCount() == 1 && (!mustBeConst || func->IsReadOnly()))
					{
						asDWORD flags = 0;
						int returnTypeId = func->GetReturnTypeId(&flags);
						if (flags != asTM_NONE)
							continue;

						bool isCmp = false, isEq = false;
						if (returnTypeId == asTYPEID_INT32 && strcmp(func->GetName(), "opCmp") == 0)
							isCmp = true;
						if (returnTypeId == asTYPEID_BOOL && strcmp(func->GetName(), "opEquals") == 0)
							isEq = true;

						if (!isCmp && !isEq)
							continue;

						int paramTypeId;
						func->GetParam(0, &paramTypeId, &flags);

						if ((paramTypeId & ~(asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST)) != (SubTypeId & ~(asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST)))
							continue;

						if ((flags & asTM_INREF))
						{
							if ((paramTypeId & asTYPEID_OBJHANDLE) || (mustBeConst && !(flags & asTM_CONST)))
								continue;
						}
						else if (paramTypeId & asTYPEID_OBJHANDLE)
						{
							if (mustBeConst && !(paramTypeId & asTYPEID_HANDLETOCONST))
								continue;
						}
						else
							continue;

						if (isCmp)
						{
							if (cache->CmpFunc || cache->CmpFuncReturnCode)
							{
								cache->CmpFunc = 0;
								cache->CmpFuncReturnCode = asMULTIPLE_FUNCTIONS;
							}
							else
								cache->CmpFunc = func;
						}
						else if (isEq)
						{
							if (cache->EqFunc || cache->EqFuncReturnCode)
							{
								cache->EqFunc = 0;
								cache->EqFuncReturnCode = asMULTIPLE_FUNCTIONS;
							}
							else
								cache->EqFunc = func;
						}
					}
				}
			}

			if (cache->EqFunc == 0 && cache->EqFuncReturnCode == 0)
				cache->EqFuncReturnCode = asNO_FUNCTION;
			if (cache->CmpFunc == 0 && cache->CmpFuncReturnCode == 0)
				cache->CmpFuncReturnCode = asNO_FUNCTION;

			ObjType->SetUserData(cache, ARRAY_CACHE);
			asReleaseExclusiveLock();
		}
		void STDArray::EnumReferences(VMCManager* engine)
		{
			if (SubTypeId & asTYPEID_MASK_OBJECT)
			{
				void** d = (void**)Buffer->Data;
				VMCTypeInfo* subType = engine->GetTypeInfoById(SubTypeId);
				if ((subType->GetFlags() & asOBJ_REF))
				{
					for (as_size_t n = 0; n < Buffer->NumElements; n++)
					{
						if (d[n])
							engine->GCEnumCallback(d[n]);
					}
				}
				else if ((subType->GetFlags() & asOBJ_VALUE) && (subType->GetFlags() & asOBJ_GC))
				{
					for (as_size_t n = 0; n < Buffer->NumElements; n++)
					{
						if (d[n])
							engine->ForwardGCEnumReferences(d[n], subType);
					}
				}
			}
		}
		void STDArray::ReleaseAllHandles(VMCManager*)
		{
			Resize(0);
		}
		void STDArray::AddRef() const
		{
			GCFlag = false;
			asAtomicInc(RefCount);
		}
		void STDArray::Release() const
		{
			GCFlag = false;
			if (asAtomicDec(RefCount) == 0)
			{
				this->~STDArray();
				asFreeMem(const_cast<STDArray*>(this));
			}
		}
		int STDArray::GetRefCount()
		{
			return RefCount;
		}
		void STDArray::SetFlag()
		{
			GCFlag = true;
		}
		bool STDArray::GetFlag()
		{
			return GCFlag;
		}
		STDArray* STDArray::Create(VMCTypeInfo* TI, as_size_t length)
		{
			void* mem = asAllocMem(sizeof(STDArray));
			if (mem == 0)
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Out of memory");

				return 0;
			}

			STDArray* a = new(mem) STDArray(length, TI);
			return a;
		}
		STDArray* STDArray::Create(VMCTypeInfo* TI, void* initList)
		{
			void* mem = asAllocMem(sizeof(STDArray));
			if (mem == 0)
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Out of memory");

				return 0;
			}

			STDArray* a = new(mem) STDArray(TI, initList);
			return a;
		}
		STDArray* STDArray::Create(VMCTypeInfo* TI, as_size_t length, void* defVal)
		{
			void* mem = asAllocMem(sizeof(STDArray));
			if (mem == 0)
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Out of memory");

				return 0;
			}

			STDArray* a = new(mem) STDArray(length, defVal, TI);
			return a;
		}
		STDArray* STDArray::Create(VMCTypeInfo* TI)
		{
			return STDArray::Create(TI, as_size_t(0));
		}
		void STDArray::CleanupTypeInfoCache(VMCTypeInfo* Type)
		{
			STDArray::SCache* Cache = reinterpret_cast<STDArray::SCache*>(Type->GetUserData(ARRAY_CACHE));
			if (Cache != nullptr)
			{
				Cache->~SCache();
				asFreeMem(Cache);
			}
		}
		bool STDArray::TemplateCallback(VMCTypeInfo* T, bool& DontGarbageCollect)
		{
			int TypeId = T->GetSubTypeId();
			if (TypeId == asTYPEID_VOID)
				return false;

			if ((TypeId & asTYPEID_MASK_OBJECT) && !(TypeId & asTYPEID_OBJHANDLE))
			{
				VMCManager* Engine = T->GetEngine();
				VMCTypeInfo* Subtype = Engine->GetTypeInfoById(TypeId);
				asDWORD Flags = Subtype->GetFlags();

				if ((Flags & asOBJ_VALUE) && !(Flags & asOBJ_POD))
				{
					bool Found = false;
					for (as_size_t n = 0; n < Subtype->GetBehaviourCount(); n++)
					{
						asEBehaviours Beh;
						asIScriptFunction* Func = Subtype->GetBehaviourByIndex(n, &Beh);
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
						Engine->WriteMessage("Array", 0, 0, asMSGTYPE_ERROR, "The subtype has no default constructor");
						return false;
					}
				}
				else if ((Flags & asOBJ_REF))
				{
					bool Found = false;
					if (!Engine->GetEngineProperty(asEP_DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE))
					{
						for (as_size_t n = 0; n < Subtype->GetFactoryCount(); n++)
						{
							asIScriptFunction* Func = Subtype->GetFactoryByIndex(n);
							if (Func->GetParamCount() == 0)
							{
								Found = true;
								break;
							}
						}
					}

					if (!Found)
					{
						Engine->WriteMessage("Array", 0, 0, asMSGTYPE_ERROR, "The subtype has no default factory");
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
				assert(TypeId & asTYPEID_OBJHANDLE);
				VMCTypeInfo* Subtype = T->GetEngine()->GetTypeInfoById(TypeId);
				asDWORD Flags = Subtype->GetFlags();

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

		STDMapKey::STDMapKey()
		{
			ValueObj = 0;
			TypeId = 0;
		}
		STDMapKey::STDMapKey(VMCManager* engine, void* value, int typeId)
		{
			ValueObj = 0;
			TypeId = 0;
			Set(engine, value, typeId);
		}
		STDMapKey::~STDMapKey()
		{
			if (ValueObj && TypeId)
			{
				VMCContext* ctx = asGetActiveContext();
				if (!ctx)
				{
					assert((TypeId & asTYPEID_MASK_OBJECT) == 0);
				}
				else
					FreeValue(ctx->GetEngine());
			}
		}
		void STDMapKey::FreeValue(VMCManager* engine)
		{
			if (TypeId & asTYPEID_MASK_OBJECT)
			{
				engine->ReleaseScriptObject(ValueObj, engine->GetTypeInfoById(TypeId));
				ValueObj = 0;
				TypeId = 0;
			}
		}
		void STDMapKey::EnumReferences(VMCManager* inEngine)
		{
			if (ValueObj)
				inEngine->GCEnumCallback(ValueObj);

			if (TypeId)
				inEngine->GCEnumCallback(inEngine->GetTypeInfoById(TypeId));
		}
		void STDMapKey::Set(VMCManager* engine, void* value, int typeId)
		{
			FreeValue(engine);
			TypeId = typeId;

			if (typeId & asTYPEID_OBJHANDLE)
			{
				ValueObj = *(void**)value;
				engine->AddRefScriptObject(ValueObj, engine->GetTypeInfoById(typeId));
			}
			else if (typeId & asTYPEID_MASK_OBJECT)
			{
				ValueObj = engine->CreateScriptObjectCopy(value, engine->GetTypeInfoById(typeId));
				if (ValueObj == 0)
				{
					VMCContext* ctx = asGetActiveContext();
					if (ctx)
						ctx->SetException("Cannot create copy of object");
				}
			}
			else
			{
				int size = engine->GetSizeOfPrimitiveType(typeId);
				memcpy(&ValueInt, value, size);
			}
		}
		void STDMapKey::Set(VMCManager* engine, STDMapKey& value)
		{
			if (value.TypeId & asTYPEID_OBJHANDLE)
				Set(engine, (void*)&value.ValueObj, value.TypeId);
			else if (value.TypeId & asTYPEID_MASK_OBJECT)
				Set(engine, (void*)value.ValueObj, value.TypeId);
			else
				Set(engine, (void*)&value.ValueInt, value.TypeId);
		}
		bool STDMapKey::Get(VMCManager* engine, void* value, int typeId) const
		{
			if (typeId & asTYPEID_OBJHANDLE)
			{
				if ((TypeId & asTYPEID_MASK_OBJECT))
				{
					if ((TypeId & asTYPEID_HANDLETOCONST) && !(typeId & asTYPEID_HANDLETOCONST))
						return false;

					engine->RefCastObject(ValueObj, engine->GetTypeInfoById(TypeId), engine->GetTypeInfoById(typeId), reinterpret_cast<void**>(value));
					return true;
				}
			}
			else if (typeId & asTYPEID_MASK_OBJECT)
			{
				bool isCompatible = false;
				if ((TypeId & ~(asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST)) == typeId && ValueObj != 0)
					isCompatible = true;

				if (isCompatible)
				{
					engine->AssignScriptObject(value, ValueObj, engine->GetTypeInfoById(typeId));
					return true;
				}
			}
			else
			{
				if (TypeId == typeId)
				{
					int size = engine->GetSizeOfPrimitiveType(typeId);
					memcpy(value, &ValueInt, size);
					return true;
				}

				if (typeId == asTYPEID_DOUBLE)
				{
					if (TypeId == asTYPEID_INT64)
					{
						*(double*)value = double(ValueInt);
					}
					else if (TypeId == asTYPEID_BOOL)
					{
						char localValue;
						memcpy(&localValue, &ValueInt, sizeof(char));
						*(double*)value = localValue ? 1.0 : 0.0;
					}
					else if (TypeId > asTYPEID_DOUBLE && (TypeId & asTYPEID_MASK_OBJECT) == 0)
					{
						int localValue;
						memcpy(&localValue, &ValueInt, sizeof(int));
						*(double*)value = double(localValue);
					}
					else
					{
						*(double*)value = 0;
						return false;
					}

					return true;
				}
				else if (typeId == asTYPEID_INT64)
				{
					if (TypeId == asTYPEID_DOUBLE)
					{
						*(as_int64_t*)value = as_int64_t(ValueFlt);
					}
					else if (TypeId == asTYPEID_BOOL)
					{
						char localValue;
						memcpy(&localValue, &ValueInt, sizeof(char));
						*(as_int64_t*)value = localValue ? 1 : 0;
					}
					else if (TypeId > asTYPEID_DOUBLE && (TypeId & asTYPEID_MASK_OBJECT) == 0)
					{
						int localValue;
						memcpy(&localValue, &ValueInt, sizeof(int));
						*(as_int64_t*)value = localValue;
					}
					else
					{
						*(as_int64_t*)value = 0;
						return false;
					}

					return true;
				}
				else if (typeId > asTYPEID_DOUBLE && (TypeId & asTYPEID_MASK_OBJECT) == 0)
				{
					if (TypeId == asTYPEID_DOUBLE)
					{
						*(int*)value = int(ValueFlt);
					}
					else if (TypeId == asTYPEID_INT64)
					{
						*(int*)value = int(ValueInt);
					}
					else if (TypeId == asTYPEID_BOOL)
					{
						char localValue;
						memcpy(&localValue, &ValueInt, sizeof(char));
						*(int*)value = localValue ? 1 : 0;
					}
					else if (TypeId > asTYPEID_DOUBLE && (TypeId & asTYPEID_MASK_OBJECT) == 0)
					{
						int localValue;
						memcpy(&localValue, &ValueInt, sizeof(int));
						*(int*)value = localValue;
					}
					else
					{
						*(int*)value = 0;
						return false;
					}

					return true;
				}
				else if (typeId == asTYPEID_BOOL)
				{
					if (TypeId & asTYPEID_OBJHANDLE)
					{
						*(bool*)value = ValueObj ? true : false;
					}
					else if (TypeId & asTYPEID_MASK_OBJECT)
					{
						*(bool*)value = true;
					}
					else
					{
						as_uint64_t zero = 0;
						int size = engine->GetSizeOfPrimitiveType(TypeId);
						*(bool*)value = memcmp(&ValueInt, &zero, size) == 0 ? false : true;
					}

					return true;
				}
			}

			return false;
		}
		const void* STDMapKey::GetAddressOfValue() const
		{
			if ((TypeId & asTYPEID_MASK_OBJECT) && !(TypeId & asTYPEID_OBJHANDLE))
				return ValueObj;

			return reinterpret_cast<const void*>(&ValueObj);
		}
		int STDMapKey::GetTypeId() const
		{
			return TypeId;
		}

		STDMap::Iterator::Iterator(const STDMap& Dict, Map::const_iterator it) : It(it), Dict(Dict)
		{
		}
		void STDMap::Iterator::operator++()
		{
			++It;
		}
		void STDMap::Iterator::operator++(int)
		{
			++It;
		}
		STDMap::Iterator& STDMap::Iterator::operator*()
		{
			return *this;
		}
		bool STDMap::Iterator::operator==(const Iterator& Other) const
		{
			return It == Other.It;
		}
		bool STDMap::Iterator::operator!=(const Iterator& Other) const
		{
			return It != Other.It;
		}
		const std::string& STDMap::Iterator::GetKey() const
		{
			return It->first;
		}
		int STDMap::Iterator::GetTypeId() const
		{
			return It->second.TypeId;
		}
		bool STDMap::Iterator::GetValue(void* value, int typeId) const
		{
			return It->second.Get(Dict.Engine, value, typeId);
		}
		const void* STDMap::Iterator::GetAddressOfValue() const
		{
			return It->second.GetAddressOfValue();
		}

		STDMap::STDMap(VMCManager* engine)
		{
			Init(engine);
		}
		STDMap::STDMap(unsigned char* buffer)
		{
			VMCContext* ctx = asGetActiveContext();
			Init(ctx->GetEngine());

			STDMap::SCache& cache = *reinterpret_cast<STDMap::SCache*>(Engine->GetUserData(MAP_CACHE));
			bool keyAsRef = cache.KeyType->GetFlags() & asOBJ_REF ? true : false;

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

				int typeId = *(int*)buffer;
				buffer += sizeof(int);

				void* ref = (void*)buffer;
				if (typeId >= asTYPEID_INT8 && typeId <= asTYPEID_DOUBLE)
				{
					as_int64_t i64;
					double d;

					switch (typeId)
					{
						case asTYPEID_INT8:
							i64 = *(char*)ref;
							break;
						case asTYPEID_INT16:
							i64 = *(short*)ref;
							break;
						case asTYPEID_INT32:
							i64 = *(int*)ref;
							break;
						case asTYPEID_UINT8:
							i64 = *(unsigned char*)ref;
							break;
						case asTYPEID_UINT16:
							i64 = *(unsigned short*)ref;
							break;
						case asTYPEID_UINT32:
							i64 = *(unsigned int*)ref;
							break;
						case asTYPEID_INT64:
						case asTYPEID_UINT64:
							i64 = *(as_int64_t*)ref;
							break;
						case asTYPEID_FLOAT:
							d = *(float*)ref;
							break;
						case asTYPEID_DOUBLE:
							d = *(double*)ref;
							break;
					}

					if (typeId >= asTYPEID_FLOAT)
						Set(name, &d, asTYPEID_DOUBLE);
					else
						Set(name, &i64, asTYPEID_INT64);
				}
				else
				{
					if ((typeId & asTYPEID_MASK_OBJECT) && !(typeId & asTYPEID_OBJHANDLE) && (Engine->GetTypeInfoById(typeId)->GetFlags() & asOBJ_REF))
						ref = *(void**)ref;

					Set(name, ref, VMManager::Get(Engine)->IsNullable(typeId) ? 0 : typeId);
				}

				if (typeId & asTYPEID_MASK_OBJECT)
				{
					VMCTypeInfo* TI = Engine->GetTypeInfoById(typeId);
					if (TI->GetFlags() & asOBJ_VALUE)
						buffer += TI->GetSize();
					else
						buffer += sizeof(void*);
				}
				else if (typeId == 0)
				{
					TH_WARN("[memerr] use nullptr instead of null for initialization lists");
					buffer += sizeof(void*);
				}
				else
					buffer += Engine->GetSizeOfPrimitiveType(typeId);
			}
		}
		STDMap::STDMap(const STDMap& Other)
		{
			Init(Other.Engine);

			Map::const_iterator it;
			for (it = Other.Dict.begin(); it != Other.Dict.end(); ++it)
			{
				auto& Key = it->second;
				if (Key.TypeId & asTYPEID_OBJHANDLE)
					Set(it->first, (void*)&Key.ValueObj, Key.TypeId);
				else if (Key.TypeId & asTYPEID_MASK_OBJECT)
					Set(it->first, (void*)Key.ValueObj, Key.TypeId);
				else
					Set(it->first, (void*)&Key.ValueInt, Key.TypeId);
			}
		}
		STDMap::~STDMap()
		{
			DeleteAll();
		}
		void STDMap::AddRef() const
		{
			GCFlag = false;
			asAtomicInc(RefCount);
		}
		void STDMap::Release() const
		{
			GCFlag = false;
			if (asAtomicDec(RefCount) == 0)
			{
				this->~STDMap();
				asFreeMem(const_cast<STDMap*>(this));
			}
		}
		int STDMap::GetRefCount()
		{
			return RefCount;
		}
		void STDMap::SetGCFlag()
		{
			GCFlag = true;
		}
		bool STDMap::GetGCFlag()
		{
			return GCFlag;
		}
		void STDMap::EnumReferences(VMCManager* inEngine)
		{
			Map::iterator it;
			for (it = Dict.begin(); it != Dict.end(); ++it)
			{
				auto& Key = it->second;
				if (Key.TypeId & asTYPEID_MASK_OBJECT)
				{
					VMCTypeInfo* subType = Engine->GetTypeInfoById(Key.TypeId);
					if ((subType->GetFlags() & asOBJ_VALUE) && (subType->GetFlags() & asOBJ_GC))
						Engine->ForwardGCEnumReferences(Key.ValueObj, subType);
					else
						inEngine->GCEnumCallback(Key.ValueObj);
				}
			}
		}
		void STDMap::ReleaseAllReferences(VMCManager*)
		{
			DeleteAll();
		}
		STDMap& STDMap::operator =(const STDMap& Other)
		{
			DeleteAll();

			Map::const_iterator it;
			for (it = Other.Dict.begin(); it != Other.Dict.end(); ++it)
			{
				auto& Key = it->second;
				if (Key.TypeId & asTYPEID_OBJHANDLE)
					Set(it->first, (void*)&Key.ValueObj, Key.TypeId);
				else if (Key.TypeId & asTYPEID_MASK_OBJECT)
					Set(it->first, (void*)Key.ValueObj, Key.TypeId);
				else
					Set(it->first, (void*)&Key.ValueInt, Key.TypeId);
			}

			return *this;
		}
		STDMapKey* STDMap::operator[](const std::string& key)
		{
			return &Dict[key];
		}
		const STDMapKey* STDMap::operator[](const std::string& key) const
		{
			Map::const_iterator it;
			it = Dict.find(key);
			if (it != Dict.end())
				return &it->second;

			VMCContext* ctx = asGetActiveContext();
			if (ctx)
				ctx->SetException("Invalid access to non-existing value");

			return 0;
		}
		void STDMap::Set(const std::string& key, void* value, int typeId)
		{
			Map::iterator it;
			it = Dict.find(key);
			if (it == Dict.end())
				it = Dict.insert(Map::value_type(key, STDMapKey())).first;

			it->second.Set(Engine, value, typeId);
		}
		bool STDMap::Get(const std::string& key, void* value, int typeId) const
		{
			Map::const_iterator it;
			it = Dict.find(key);
			if (it != Dict.end())
				return it->second.Get(Engine, value, typeId);

			return false;
		}
		bool STDMap::GetIndex(size_t Index, std::string* Key, void** Value, int* TypeId) const
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
		int STDMap::GetTypeId(const std::string& key) const
		{
			Map::const_iterator it;
			it = Dict.find(key);
			if (it != Dict.end())
				return it->second.TypeId;

			return -1;
		}
		bool STDMap::Exists(const std::string& key) const
		{
			Map::const_iterator it;
			it = Dict.find(key);
			if (it != Dict.end())
				return true;

			return false;
		}
		bool STDMap::IsEmpty() const
		{
			if (Dict.size() == 0)
				return true;

			return false;
		}
		as_size_t STDMap::GetSize() const
		{
			return as_size_t(Dict.size());
		}
		bool STDMap::Delete(const std::string& key)
		{
			Map::iterator it;
			it = Dict.find(key);
			if (it != Dict.end())
			{
				it->second.FreeValue(Engine);
				Dict.erase(it);
				return true;
			}

			return false;
		}
		void STDMap::DeleteAll()
		{
			Map::iterator it;
			for (it = Dict.begin(); it != Dict.end(); ++it)
				it->second.FreeValue(Engine);

			Dict.clear();
		}
		STDArray* STDMap::GetKeys() const
		{
			STDMap::SCache* cache = reinterpret_cast<STDMap::SCache*>(Engine->GetUserData(MAP_CACHE));
			VMCTypeInfo* TI = cache->ArrayType;

			STDArray* Array = STDArray::Create(TI, as_size_t(Dict.size()));
			Map::const_iterator it;
			long current = -1;

			for (it = Dict.begin(); it != Dict.end(); ++it)
			{
				current++;
				*(std::string*)Array->At(current) = it->first;
			}

			return Array;
		}
		STDMap::Iterator STDMap::Begin() const
		{
			return Iterator(*this, Dict.begin());
		}
		STDMap::Iterator STDMap::End() const
		{
			return Iterator(*this, Dict.end());
		}
		STDMap::Iterator STDMap::Find(const std::string& key) const
		{
			return Iterator(*this, Dict.find(key));
		}
		STDMap* STDMap::Create(VMCManager* engine)
		{
			STDMap* obj = (STDMap*)asAllocMem(sizeof(STDMap));
			new(obj) STDMap(engine);
			return obj;
		}
		STDMap* STDMap::Create(unsigned char* buffer)
		{
			STDMap* obj = (STDMap*)asAllocMem(sizeof(STDMap));
			new(obj) STDMap(buffer);
			return obj;
		}
		void STDMap::Init(VMCManager* e)
		{
			RefCount = 1;
			GCFlag = false;
			Engine = e;

			STDMap::SCache* Cache = reinterpret_cast<STDMap::SCache*>(Engine->GetUserData(MAP_CACHE));
			Engine->NotifyGarbageCollectorOfNewObject(this, Cache->DictType);
		}
		void STDMap::Cleanup(VMCManager* engine)
		{
			STDMap::SCache* Cache = reinterpret_cast<STDMap::SCache*>(engine->GetUserData(MAP_CACHE));
			TH_DELETE(SCache, Cache);
		}
		void STDMap::Setup(VMCManager* engine)
		{
			STDMap::SCache* cache = reinterpret_cast<STDMap::SCache*>(engine->GetUserData(MAP_CACHE));
			if (cache == 0)
			{
				cache = TH_NEW(STDMap::SCache);
				engine->SetUserData(cache, MAP_CACHE);
				engine->SetEngineUserDataCleanupCallback(Cleanup, MAP_CACHE);

				cache->DictType = engine->GetTypeInfoByName("Map");
				cache->ArrayType = engine->GetTypeInfoByDecl("Array<String>");
				cache->KeyType = engine->GetTypeInfoByDecl("String");
			}
		}
		void STDMap::Factory(VMCGeneric* gen)
		{
			*(STDMap**)gen->GetAddressOfReturnLocation() = STDMap::Create(gen->GetEngine());
		}
		void STDMap::ListFactory(VMCGeneric* gen)
		{
			unsigned char* buffer = (unsigned char*)gen->GetArgAddress(0);
			*(STDMap**)gen->GetAddressOfReturnLocation() = STDMap::Create(buffer);
		}
		void STDMap::KeyConstruct(void* mem)
		{
			new(mem) STDMapKey();
		}
		void STDMap::KeyDestruct(STDMapKey* obj)
		{
			VMCContext* ctx = asGetActiveContext();
			if (ctx)
			{
				VMCManager* engine = ctx->GetEngine();
				obj->FreeValue(engine);
			}
			obj->~STDMapKey();
		}
		STDMapKey& STDMap::KeyopAssign(void* ref, int typeId, STDMapKey* obj)
		{
			VMCContext* ctx = asGetActiveContext();
			if (ctx)
			{
				VMCManager* engine = ctx->GetEngine();
				obj->Set(engine, ref, typeId);
			}
			return *obj;
		}
		STDMapKey& STDMap::KeyopAssign(const STDMapKey& Other, STDMapKey* obj)
		{
			VMCContext* ctx = asGetActiveContext();
			if (ctx)
			{
				VMCManager* engine = ctx->GetEngine();
				obj->Set(engine, const_cast<STDMapKey&>(Other));
			}

			return *obj;
		}
		STDMapKey& STDMap::KeyopAssign(double Value, STDMapKey* obj)
		{
			return KeyopAssign(&Value, asTYPEID_DOUBLE, obj);
		}
		STDMapKey& STDMap::KeyopAssign(as_int64_t Value, STDMapKey* obj)
		{
			return STDMap::KeyopAssign(&Value, asTYPEID_INT64, obj);
		}
		void STDMap::KeyopCast(void* ref, int typeId, STDMapKey* obj)
		{
			VMCContext* ctx = asGetActiveContext();
			if (ctx)
			{
				VMCManager* engine = ctx->GetEngine();
				obj->Get(engine, ref, typeId);
			}
		}
		as_int64_t STDMap::KeyopConvInt(STDMapKey* obj)
		{
			as_int64_t value;
			KeyopCast(&value, asTYPEID_INT64, obj);
			return value;
		}
		double STDMap::KeyopConvDouble(STDMapKey* obj)
		{
			double value;
			KeyopCast(&value, asTYPEID_DOUBLE, obj);
			return value;
		}

		STDGrid::STDGrid(VMCTypeInfo* TI, void* buf)
		{
			RefCount = 1;
			GCFlag = false;
			ObjType = TI;
			ObjType->AddRef();
			Buffer = 0;
			SubTypeId = ObjType->GetSubTypeId();

			VMCManager* engine = TI->GetEngine();
			if (SubTypeId & asTYPEID_MASK_OBJECT)
				ElementSize = sizeof(asPWORD);
			else
				ElementSize = engine->GetSizeOfPrimitiveType(SubTypeId);

			as_size_t Height = *(as_size_t*)buf;
			as_size_t Width = Height ? *(as_size_t*)((char*)(buf)+4) : 0;

			if (!CheckMaxSize(Width, Height))
				return;

			buf = (as_size_t*)(buf)+1;
			if ((TI->GetSubTypeId() & asTYPEID_MASK_OBJECT) == 0)
			{
				CreateBuffer(&Buffer, Width, Height);
				for (as_size_t y = 0; y < Height; y++)
				{
					buf = (as_size_t*)(buf)+1;
					if (Width > 0)
						memcpy(At(0, y), buf, (size_t)Width * (size_t)ElementSize);

					buf = (char*)(buf)+Width * (as_size_t)ElementSize;
					if (asPWORD(buf) & 0x3)
						buf = (char*)(buf)+4 - (asPWORD(buf) & 0x3);
				}
			}
			else if (TI->GetSubTypeId() & asTYPEID_OBJHANDLE)
			{
				CreateBuffer(&Buffer, Width, Height);
				for (as_size_t y = 0; y < Height; y++)
				{
					buf = (as_size_t*)(buf)+1;
					if (Width > 0)
						memcpy(At(0, y), buf, (size_t)Width * (size_t)ElementSize);

					memset(buf, 0, (size_t)Width * (size_t)ElementSize);
					buf = (char*)(buf)+Width * (as_size_t)ElementSize;

					if (asPWORD(buf) & 0x3)
						buf = (char*)(buf)+4 - (asPWORD(buf) & 0x3);
				}
			}
			else if (TI->GetSubType()->GetFlags() & asOBJ_REF)
			{
				SubTypeId |= asTYPEID_OBJHANDLE;
				CreateBuffer(&Buffer, Width, Height);
				SubTypeId &= ~asTYPEID_OBJHANDLE;

				for (as_size_t y = 0; y < Height; y++)
				{
					buf = (as_size_t*)(buf)+1;
					if (Width > 0)
						memcpy(At(0, y), buf, (size_t)Width * (size_t)ElementSize);

					memset(buf, 0, (size_t)Width * (size_t)ElementSize);
					buf = (char*)(buf)+Width * (as_size_t)ElementSize;

					if (asPWORD(buf) & 0x3)
						buf = (char*)(buf)+4 - (asPWORD(buf) & 0x3);
				}
			}
			else
			{
				CreateBuffer(&Buffer, Width, Height);

				VMCTypeInfo* subType = TI->GetSubType();
				as_size_t subTypeSize = subType->GetSize();
				for (as_size_t y = 0; y < Height; y++)
				{
					buf = (as_size_t*)(buf)+1;
					for (as_size_t x = 0; x < Width; x++)
					{
						void* obj = At(x, y);
						unsigned char* srcObj = (unsigned char*)(buf)+x * subTypeSize;
						engine->AssignScriptObject(obj, srcObj, subType);
					}

					buf = (char*)(buf)+Width * subTypeSize;
					if (asPWORD(buf) & 0x3)
						buf = (char*)(buf)+4 - (asPWORD(buf) & 0x3);
				}
			}

			if (ObjType->GetFlags() & asOBJ_GC)
				ObjType->GetEngine()->NotifyGarbageCollectorOfNewObject(this, ObjType);
		}
		STDGrid::STDGrid(as_size_t Width, as_size_t Height, VMCTypeInfo* TI)
		{
			RefCount = 1;
			GCFlag = false;
			ObjType = TI;
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
		void STDGrid::Resize(as_size_t Width, as_size_t Height)
		{
			if (!CheckMaxSize(Width, Height))
				return;

			SBuffer* tmpBuffer = 0;
			CreateBuffer(&tmpBuffer, Width, Height);
			if (tmpBuffer == 0)
				return;

			if (Buffer)
			{
				as_size_t w = Width > Buffer->Width ? Buffer->Width : Width;
				as_size_t h = Height > Buffer->Height ? Buffer->Height : Height;
				for (as_size_t y = 0; y < h; y++)
				{
					for (as_size_t x = 0; x < w; x++)
						SetValue(tmpBuffer, x, y, At(Buffer, x, y));
				}

				DeleteBuffer(Buffer);
			}

			Buffer = tmpBuffer;
		}
		STDGrid::STDGrid(as_size_t Width, as_size_t Height, void* defVal, VMCTypeInfo* TI)
		{
			RefCount = 1;
			GCFlag = false;
			ObjType = TI;
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
					SetValue(x, y, defVal);
			}
		}
		void STDGrid::SetValue(as_size_t x, as_size_t y, void* value)
		{
			SetValue(Buffer, x, y, value);
		}
		void STDGrid::SetValue(SBuffer* buf, as_size_t x, as_size_t y, void* value)
		{
			void* ptr = At(buf, x, y);
			if (ptr == 0)
				return;

			if ((SubTypeId & ~asTYPEID_MASK_SEQNBR) && !(SubTypeId & asTYPEID_OBJHANDLE))
				ObjType->GetEngine()->AssignScriptObject(ptr, value, ObjType->GetSubType());
			else if (SubTypeId & asTYPEID_OBJHANDLE)
			{
				void* tmp = *(void**)ptr;
				*(void**)ptr = *(void**)value;
				ObjType->GetEngine()->AddRefScriptObject(*(void**)value, ObjType->GetSubType());
				if (tmp)
					ObjType->GetEngine()->ReleaseScriptObject(tmp, ObjType->GetSubType());
			}
			else if (SubTypeId == asTYPEID_BOOL ||
				SubTypeId == asTYPEID_INT8 ||
				SubTypeId == asTYPEID_UINT8)
				*(char*)ptr = *(char*)value;
			else if (SubTypeId == asTYPEID_INT16 ||
				SubTypeId == asTYPEID_UINT16)
				*(short*)ptr = *(short*)value;
			else if (SubTypeId == asTYPEID_INT32 ||
				SubTypeId == asTYPEID_UINT32 ||
				SubTypeId == asTYPEID_FLOAT ||
				SubTypeId > asTYPEID_DOUBLE)
				*(int*)ptr = *(int*)value;
			else if (SubTypeId == asTYPEID_INT64 ||
				SubTypeId == asTYPEID_UINT64 ||
				SubTypeId == asTYPEID_DOUBLE)
				*(double*)ptr = *(double*)value;
		}
		STDGrid::~STDGrid()
		{
			if (Buffer)
			{
				DeleteBuffer(Buffer);
				Buffer = 0;
			}

			if (ObjType)
				ObjType->Release();
		}
		as_size_t STDGrid::GetWidth() const
		{
			if (Buffer)
				return Buffer->Width;

			return 0;
		}
		as_size_t STDGrid::GetHeight() const
		{
			if (Buffer)
				return Buffer->Height;

			return 0;
		}
		bool STDGrid::CheckMaxSize(as_size_t Width, as_size_t Height)
		{
			as_size_t maxSize = 0xFFFFFFFFul - sizeof(SBuffer) + 1;
			if (ElementSize > 0)
				maxSize /= (as_size_t)ElementSize;

			as_uint64_t numElements = (as_uint64_t)Width * (as_uint64_t)Height;
			if ((numElements >> 32) || numElements > (as_uint64_t)maxSize)
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Too large grid size");

				return false;
			}
			return true;
		}
		VMCTypeInfo* STDGrid::GetGridObjectType() const
		{
			return ObjType;
		}
		int STDGrid::GetGridTypeId() const
		{
			return ObjType->GetTypeId();
		}
		int STDGrid::GetElementTypeId() const
		{
			return SubTypeId;
		}
		void* STDGrid::At(as_size_t x, as_size_t y)
		{
			return At(Buffer, x, y);
		}
		void* STDGrid::At(SBuffer* buf, as_size_t x, as_size_t y)
		{
			if (buf == 0 || x >= buf->Width || y >= buf->Height)
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Index out of bounds");
				return 0;
			}

			as_size_t index = x + y * buf->Width;
			if ((SubTypeId & asTYPEID_MASK_OBJECT) && !(SubTypeId & asTYPEID_OBJHANDLE))
				return *(void**)(buf->Data + (as_size_t)ElementSize * index);
			else
				return buf->Data + (as_size_t)ElementSize * index;
		}
		const void* STDGrid::At(as_size_t x, as_size_t y) const
		{
			return const_cast<STDGrid*>(this)->At(const_cast<SBuffer*>(Buffer), x, y);
		}
		void STDGrid::CreateBuffer(SBuffer** buf, as_size_t w, as_size_t h)
		{
			as_size_t numElements = w * h;
			*buf = reinterpret_cast<SBuffer*>(asAllocMem(sizeof(SBuffer) - 1 + (size_t)ElementSize * (size_t)numElements));

			if (*buf)
			{
				(*buf)->Width = w;
				(*buf)->Height = h;
				Construct(*buf);
			}
			else
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Out of memory");
			}
		}
		void STDGrid::DeleteBuffer(SBuffer* buf)
		{
			assert(buf);
			Destruct(buf);
			asFreeMem(buf);
		}
		void STDGrid::Construct(SBuffer* buf)
		{
			assert(buf);
			if (SubTypeId & asTYPEID_OBJHANDLE)
			{
				void* d = (void*)(buf->Data);
				memset(d, 0, (buf->Width * buf->Height) * sizeof(void*));
			}
			else if (SubTypeId & asTYPEID_MASK_OBJECT)
			{
				void** max = (void**)(buf->Data + (buf->Width * buf->Height) * sizeof(void*));
				void** d = (void**)(buf->Data);

				VMCManager* engine = ObjType->GetEngine();
				VMCTypeInfo* subType = ObjType->GetSubType();

				for (; d < max; d++)
				{
					*d = (void*)engine->CreateScriptObject(subType);
					if (*d == 0)
					{
						memset(d, 0, sizeof(void*) * (max - d));
						return;
					}
				}
			}
		}
		void STDGrid::Destruct(SBuffer* buf)
		{
			assert(buf);
			if (SubTypeId & asTYPEID_MASK_OBJECT)
			{
				VMCManager* engine = ObjType->GetEngine();
				void** max = (void**)(buf->Data + (buf->Width * buf->Height) * sizeof(void*));
				void** d = (void**)(buf->Data);

				for (; d < max; d++)
				{
					if (*d)
						engine->ReleaseScriptObject(*d, ObjType->GetSubType());
				}
			}
		}
		void STDGrid::EnumReferences(VMCManager* engine)
		{
			if (Buffer == 0)
				return;

			if (SubTypeId & asTYPEID_MASK_OBJECT)
			{
				as_size_t numElements = Buffer->Width * Buffer->Height;
				void** d = (void**)Buffer->Data;
				VMCTypeInfo* subType = engine->GetTypeInfoById(SubTypeId);

				if ((subType->GetFlags() & asOBJ_REF))
				{
					for (as_size_t n = 0; n < numElements; n++)
					{
						if (d[n])
							engine->GCEnumCallback(d[n]);
					}
				}
				else if ((subType->GetFlags() & asOBJ_VALUE) && (subType->GetFlags() & asOBJ_GC))
				{
					for (as_size_t n = 0; n < numElements; n++)
					{
						if (d[n])
							engine->ForwardGCEnumReferences(d[n], subType);
					}
				}
			}
		}
		void STDGrid::ReleaseAllHandles(VMCManager*)
		{
			if (Buffer == 0)
				return;

			DeleteBuffer(Buffer);
			Buffer = 0;
		}
		void STDGrid::AddRef() const
		{
			GCFlag = false;
			asAtomicInc(RefCount);
		}
		void STDGrid::Release() const
		{
			GCFlag = false;
			if (asAtomicDec(RefCount) == 0)
			{
				this->~STDGrid();
				asFreeMem(const_cast<STDGrid*>(this));
			}
		}
		int STDGrid::GetRefCount()
		{
			return RefCount;
		}
		void STDGrid::SetFlag()
		{
			GCFlag = true;
		}
		bool STDGrid::GetFlag()
		{
			return GCFlag;
		}
		STDGrid* STDGrid::Create(VMCTypeInfo* TI)
		{
			return STDGrid::Create(TI, 0, 0);
		}
		STDGrid* STDGrid::Create(VMCTypeInfo* TI, as_size_t w, as_size_t h)
		{
			void* mem = asAllocMem(sizeof(STDGrid));
			if (mem == 0)
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Out of memory");

				return 0;
			}

			STDGrid* a = new(mem) STDGrid(w, h, TI);
			return a;
		}
		STDGrid* STDGrid::Create(VMCTypeInfo* TI, void* initList)
		{
			void* mem = asAllocMem(sizeof(STDGrid));
			if (mem == 0)
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Out of memory");

				return 0;
			}

			STDGrid* a = new(mem) STDGrid(TI, initList);
			return a;
		}
		STDGrid* STDGrid::Create(VMCTypeInfo* TI, as_size_t w, as_size_t h, void* defVal)
		{
			void* mem = asAllocMem(sizeof(STDGrid));
			if (mem == 0)
			{
				VMCContext* ctx = asGetActiveContext();
				if (ctx)
					ctx->SetException("Out of memory");

				return 0;
			}

			STDGrid* a = new(mem) STDGrid(w, h, defVal, TI);
			return a;
		}
		bool STDGrid::TemplateCallback(VMCTypeInfo* TI, bool& DontGarbageCollect)
		{
			int typeId = TI->GetSubTypeId();
			if (typeId == asTYPEID_VOID)
				return false;

			VMCManager* Engine = TI->GetEngine();
			if ((typeId & asTYPEID_MASK_OBJECT) && !(typeId & asTYPEID_OBJHANDLE))
			{
				VMCTypeInfo* subtype = Engine->GetTypeInfoById(typeId);
				asDWORD flags = subtype->GetFlags();

				if ((flags & asOBJ_VALUE) && !(flags & asOBJ_POD))
				{
					bool found = false;
					for (as_size_t n = 0; n < subtype->GetBehaviourCount(); n++)
					{
						asEBehaviours beh;
						asIScriptFunction* func = subtype->GetBehaviourByIndex(n, &beh);
						if (beh != asBEHAVE_CONSTRUCT)
							continue;

						if (func->GetParamCount() == 0)
						{
							found = true;
							break;
						}
					}

					if (!found)
					{
						Engine->WriteMessage("Array", 0, 0, asMSGTYPE_ERROR, "The subtype has no default constructor");
						return false;
					}
				}
				else if ((flags & asOBJ_REF))
				{
					bool found = false;
					if (!Engine->GetEngineProperty(asEP_DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE))
					{
						for (as_size_t n = 0; n < subtype->GetFactoryCount(); n++)
						{
							asIScriptFunction* func = subtype->GetFactoryByIndex(n);
							if (func->GetParamCount() == 0)
							{
								found = true;
								break;
							}
						}
					}

					if (!found)
					{
						Engine->WriteMessage("Array", 0, 0, asMSGTYPE_ERROR, "The subtype has no default factory");
						return false;
					}
				}

				if (!(flags & asOBJ_GC))
					DontGarbageCollect = true;
			}
			else if (!(typeId & asTYPEID_OBJHANDLE))
			{
				DontGarbageCollect = true;
			}
			else
			{
				assert(typeId & asTYPEID_OBJHANDLE);
				VMCTypeInfo* subtype = Engine->GetTypeInfoById(typeId);
				asDWORD flags = subtype->GetFlags();

				if (!(flags & asOBJ_GC))
				{
					if ((flags & asOBJ_SCRIPT_OBJECT))
					{
						if ((flags & asOBJ_NOINHERIT))
							DontGarbageCollect = true;
					}
					else
						DontGarbageCollect = true;
				}
			}

			return true;
		}

		STDRef::STDRef()
		{
			Ref = 0;
			Type = 0;
		}
		STDRef::STDRef(const STDRef& Other)
		{
			Ref = Other.Ref;
			Type = Other.Type;

			AddRefHandle();
		}
		STDRef::STDRef(void* ref, VMCTypeInfo* type)
		{
			Ref = ref;
			Type = type;

			AddRefHandle();
		}
		STDRef::STDRef(void* ref, int typeId)
		{
			Ref = 0;
			Type = 0;

			Assign(ref, typeId);
		}
		STDRef::~STDRef()
		{
			ReleaseHandle();
		}
		void STDRef::ReleaseHandle()
		{
			if (Ref && Type)
			{
				VMCManager* engine = Type->GetEngine();
				engine->ReleaseScriptObject(Ref, Type);
				engine->Release();

				Ref = 0;
				Type = 0;
			}
		}
		void STDRef::AddRefHandle()
		{
			if (Ref && Type)
			{
				VMCManager* engine = Type->GetEngine();
				engine->AddRefScriptObject(Ref, Type);
				engine->AddRef();
			}
		}
		STDRef& STDRef::operator =(const STDRef& Other)
		{
			Set(Other.Ref, Other.Type);
			return *this;
		}
		void STDRef::Set(void* ref, VMCTypeInfo* type)
		{
			if (Ref == ref)
				return;

			ReleaseHandle();
			Ref = ref;
			Type = type;
			AddRefHandle();
		}
		void* STDRef::GetRef()
		{
			return Ref;
		}
		VMCTypeInfo* STDRef::GetType() const
		{
			return Type;
		}
		int STDRef::GetTypeId() const
		{
			if (Type == 0) return 0;

			return Type->GetTypeId() | asTYPEID_OBJHANDLE;
		}
		STDRef& STDRef::Assign(void* ref, int typeId)
		{
			if (typeId == 0)
			{
				Set(0, 0);
				return *this;
			}

			if (typeId & asTYPEID_OBJHANDLE)
			{
				ref = *(void**)ref;
				typeId &= ~asTYPEID_OBJHANDLE;
			}

			VMCContext* ctx = asGetActiveContext();
			VMCManager* engine = ctx->GetEngine();
			VMCTypeInfo* type = engine->GetTypeInfoById(typeId);

			if (type && strcmp(type->GetName(), "ref") == 0)
			{
				STDRef* r = (STDRef*)ref;
				ref = r->Ref;
				type = r->Type;
			}

			Set(ref, type);
			return *this;
		}
		bool STDRef::operator==(const STDRef& o) const
		{
			if (Ref == o.Ref && Type == o.Type)
				return true;

			return false;
		}
		bool STDRef::operator!=(const STDRef& o) const
		{
			return !(*this == o);
		}
		bool STDRef::Equals(void* ref, int typeId) const
		{
			if (typeId == 0)
				ref = 0;

			if (typeId & asTYPEID_OBJHANDLE)
			{
				void** Sub = (void**)ref;
				if (!Sub)
					return false;

				ref = *Sub;
				typeId &= ~asTYPEID_OBJHANDLE;
			}

			if (ref == Ref)
				return true;

			return false;
		}
		void STDRef::Cast(void** outRef, int typeId)
		{
			if (Type == 0)
			{
				*outRef = 0;
				return;
			}

			assert(typeId & asTYPEID_OBJHANDLE);
			typeId &= ~asTYPEID_OBJHANDLE;

			VMCManager* engine = Type->GetEngine();
			VMCTypeInfo* type = engine->GetTypeInfoById(typeId);
			*outRef = 0;

			engine->RefCastObject(Ref, Type, type, outRef);
		}
		void STDRef::EnumReferences(VMCManager* inEngine)
		{
			if (Ref)
				inEngine->GCEnumCallback(Ref);

			if (Type)
				inEngine->GCEnumCallback(Type);
		}
		void STDRef::ReleaseReferences(VMCManager* inEngine)
		{
			Set(0, 0);
		}
		void STDRef::Construct(STDRef* self)
		{
			new(self) STDRef();
		}
		void STDRef::Construct(STDRef* self, const STDRef& o)
		{
			new(self) STDRef(o);
		}
		void STDRef::Construct(STDRef* self, void* ref, int typeId)
		{
			new(self) STDRef(ref, typeId);
		}
		void STDRef::Destruct(STDRef* self)
		{
			self->~STDRef();
		}

		STDWeakRef::STDWeakRef(VMCTypeInfo* type)
		{
			Ref = 0;
			Type = type;
			Type->AddRef();
			WeakRefFlag = 0;
		}
		STDWeakRef::STDWeakRef(const STDWeakRef& Other)
		{
			Ref = Other.Ref;
			Type = Other.Type;
			Type->AddRef();
			WeakRefFlag = Other.WeakRefFlag;
			if (WeakRefFlag)
				WeakRefFlag->AddRef();
		}
		STDWeakRef::STDWeakRef(void* ref, VMCTypeInfo* type)
		{
			Ref = ref;
			Type = type;
			Type->AddRef();

			assert(strcmp(type->GetName(), "WeakRef") == 0 ||
				strcmp(type->GetName(), "ConstWeakRef") == 0);

			WeakRefFlag = Type->GetEngine()->GetWeakRefFlagOfScriptObject(Ref, Type->GetSubType());
			if (WeakRefFlag)
				WeakRefFlag->AddRef();
		}
		STDWeakRef::~STDWeakRef()
		{
			if (Type)
				Type->Release();

			if (WeakRefFlag)
				WeakRefFlag->Release();
		}
		STDWeakRef& STDWeakRef::operator =(const STDWeakRef& Other)
		{
			if (Ref == Other.Ref &&
				WeakRefFlag == Other.WeakRefFlag)
				return *this;

			if (Type != Other.Type)
			{
				if (!(strcmp(Type->GetName(), "ConstWeakRef") == 0 &&
					strcmp(Other.Type->GetName(), "WeakRef") == 0 &&
					Type->GetSubType() == Other.Type->GetSubType()))
				{
					assert(false);
					return *this;
				}
			}

			Ref = Other.Ref;
			if (WeakRefFlag)
				WeakRefFlag->Release();

			WeakRefFlag = Other.WeakRefFlag;
			if (WeakRefFlag)
				WeakRefFlag->AddRef();

			return *this;
		}
		STDWeakRef& STDWeakRef::Set(void* newRef)
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
		VMCTypeInfo* STDWeakRef::GetRefType() const
		{
			return Type->GetSubType();
		}
		bool STDWeakRef::operator==(const STDWeakRef& o) const
		{
			if (Ref == o.Ref &&
				WeakRefFlag == o.WeakRefFlag &&
				Type == o.Type)
				return true;

			return false;
		}
		bool STDWeakRef::operator!=(const STDWeakRef& o) const
		{
			return !(*this == o);
		}
		void* STDWeakRef::Get() const
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
		bool STDWeakRef::Equals(void* ref) const
		{
			if (Ref != ref)
				return false;

			asILockableSharedBool* flag = Type->GetEngine()->GetWeakRefFlagOfScriptObject(ref, Type->GetSubType());
			if (WeakRefFlag != flag)
				return false;

			return true;
		}
		void STDWeakRef::Construct(VMCTypeInfo* type, void* mem)
		{
			new(mem) STDWeakRef(type);
		}
		void STDWeakRef::Construct2(VMCTypeInfo* type, void* ref, void* mem)
		{
			new(mem) STDWeakRef(ref, type);
			VMCContext* ctx = asGetActiveContext();
			if (ctx && ctx->GetState() == asEXECUTION_EXCEPTION)
				reinterpret_cast<STDWeakRef*>(mem)->~STDWeakRef();
		}
		void STDWeakRef::Destruct(STDWeakRef* obj)
		{
			obj->~STDWeakRef();
		}
		bool STDWeakRef::TemplateCallback(VMCTypeInfo* TI, bool&)
		{
			VMCTypeInfo* subType = TI->GetSubType();
			if (subType == 0)
				return false;

			if (!(subType->GetFlags() & asOBJ_REF))
				return false;

			if (TI->GetSubTypeId() & asTYPEID_OBJHANDLE)
				return false;

			as_size_t cnt = subType->GetBehaviourCount();
			for (as_size_t n = 0; n < cnt; n++)
			{
				asEBehaviours beh;
				subType->GetBehaviourByIndex(n, &beh);
				if (beh == asBEHAVE_GET_WEAKREF_FLAG)
					return true;
			}

			TI->GetEngine()->WriteMessage("WeakRef", 0, 0, asMSGTYPE_ERROR, "The subtype doesn't support weak references");
			return false;
		}

		STDComplex::STDComplex()
		{
			R = 0;
			I = 0;
		}
		STDComplex::STDComplex(const STDComplex& Other)
		{
			R = Other.R;
			I = Other.I;
		}
		STDComplex::STDComplex(float _r, float _i)
		{
			R = _r;
			I = _i;
		}
		bool STDComplex::operator==(const STDComplex& o) const
		{
			return (R == o.R) && (I == o.I);
		}
		bool STDComplex::operator!=(const STDComplex& o) const
		{
			return !(*this == o);
		}
		STDComplex& STDComplex::operator=(const STDComplex& Other)
		{
			R = Other.R;
			I = Other.I;
			return *this;
		}
		STDComplex& STDComplex::operator+=(const STDComplex& Other)
		{
			R += Other.R;
			I += Other.I;
			return *this;
		}
		STDComplex& STDComplex::operator-=(const STDComplex& Other)
		{
			R -= Other.R;
			I -= Other.I;
			return *this;
		}
		STDComplex& STDComplex::operator*=(const STDComplex& Other)
		{
			*this = *this * Other;
			return *this;
		}
		STDComplex& STDComplex::operator/=(const STDComplex& Other)
		{
			*this = *this / Other;
			return *this;
		}
		float STDComplex::SquaredLength() const
		{
			return R * R + I * I;
		}
		float STDComplex::Length() const
		{
			return sqrtf(SquaredLength());
		}
		STDComplex STDComplex::operator+(const STDComplex& Other) const
		{
			return STDComplex(R + Other.R, I + Other.I);
		}
		STDComplex STDComplex::operator-(const STDComplex& Other) const
		{
			return STDComplex(R - Other.R, I + Other.I);
		}
		STDComplex STDComplex::operator*(const STDComplex& Other) const
		{
			return STDComplex(R * Other.R - I * Other.I, R * Other.I + I * Other.R);
		}
		STDComplex STDComplex::operator/(const STDComplex& Other) const
		{
			float squaredLen = Other.SquaredLength();
			if (squaredLen == 0)
				return STDComplex(0, 0);

			return STDComplex((R * Other.R + I * Other.I) / squaredLen, (I * Other.R - R * Other.I) / squaredLen);
		}
		STDComplex STDComplex::GetRI() const
		{
			return *this;
		}
		STDComplex STDComplex::GetIR() const
		{
			return STDComplex(R, I);
		}
		void STDComplex::SetRI(const STDComplex& o)
		{
			*this = o;
		}
		void STDComplex::SetIR(const STDComplex& o)
		{
			R = o.I;
			I = o.R;
		}
		void STDComplex::DefaultConstructor(STDComplex* self)
		{
			new(self) STDComplex();
		}
		void STDComplex::CopyConstructor(const STDComplex& Other, STDComplex* self)
		{
			new(self) STDComplex(Other);
		}
		void STDComplex::ConvConstructor(float r, STDComplex* self)
		{
			new(self) STDComplex(r);
		}
		void STDComplex::InitConstructor(float r, float i, STDComplex* self)
		{
			new(self) STDComplex(r, i);
		}
		void STDComplex::ListConstructor(float* list, STDComplex* self)
		{
			new(self) STDComplex(list[0], list[1]);
		}

		STDRandom::STDRandom()
		{
			SeedFromTime();
		}
		void STDRandom::AddRef()
		{
			asAtomicInc(Ref);
		}
		void STDRandom::Release()
		{
			if (asAtomicDec(Ref) <= 0)
			{
				this->~STDRandom();
				asFreeMem((void*)this);
			}
		}
		void STDRandom::SeedFromTime()
		{
			Seed(static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
		}
		uint32_t STDRandom::GetU()
		{
			return Twister();
		}
		int32_t STDRandom::GetI()
		{
			return Twister();
		}
		double STDRandom::GetD()
		{
			return DDist(Twister);
		}
		void STDRandom::Seed(uint32_t Seed)
		{
			Twister.seed(Seed);
		}
		void STDRandom::Seed(STDArray* Array)
		{
			if (!Array || Array->GetElementTypeId() != asTYPEID_UINT32)
			{
				VMCContext* Context = asGetActiveContext();
				if (Context != nullptr)
					Context->SetException("random::seed Array element type not uint32");

				return;
			}

			std::vector<uint32_t> Vector;
			Vector.reserve(Array->GetSize());

			for (unsigned int i = 0; i < Array->GetSize(); i++)
				Vector.push_back(static_cast<uint32_t*>(Array->GetBuffer())[i]);

			std::seed_seq Sq(Vector.begin(), Vector.end());
			Twister.seed(Sq);
		}
		void STDRandom::Assign(STDRandom* From)
		{
			if (From != nullptr)
				Twister = From->Twister;
		}
		STDRandom* STDRandom::Create()
		{
			void* Data = asAllocMem(sizeof(STDRandom));
			return new(Data) STDRandom();
		}

		STDThread::STDThread(VMCManager* Engine, VMCFunction* Func) : Function(Func), Manager(VMManager::Get(Engine)), Context(nullptr), GCFlag(false), Ref(1)
		{
			Engine->NotifyGarbageCollectorOfNewObject(this, Engine->GetTypeInfoByName("Thread"));
		}
		void STDThread::Routine()
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
				Manager->GetEngine()->WriteMessage("", 0, 0, asMSGTYPE_ERROR, "failed to Start a thread: no available context");
				Release();

				return Mutex.unlock();
			}

			Mutex.unlock();
			Context->Execute(Function, [this](Script::VMContext* Ctx)
			{
				Ctx->SetArgObject(0, this);
				Ctx->SetUserData(this, ContextUD);
			});
			Context->SetUserData(nullptr, ContextUD);
			Mutex.lock();

			if (!Context->IsSuspended())
				TH_CLEAR(Context);

			CV.notify_all();
			Mutex.unlock();
			Release();
			asThreadCleanup();
		}
		void STDThread::AddRef()
		{
			GCFlag = false;
			asAtomicInc(Ref);
		}
		void STDThread::Suspend()
		{
			Mutex.lock();
			if (Context && Context->GetState() != VMExecState::SUSPENDED)
				Context->Suspend();
			Mutex.unlock();
		}
		void STDThread::Resume()
		{
			Mutex.lock();
			if (Context && Context->GetState() == VMExecState::SUSPENDED)
				Context->Execute();
			Mutex.unlock();
		}
		void STDThread::Release()
		{
			GCFlag = false;
			if (asAtomicDec(Ref) <= 0)
			{
				if (Thread.joinable())
					Thread.join();

				ReleaseReferences(nullptr);
				this->~STDThread();
				asFreeMem((void*)this);
			}
		}
		void STDThread::SetGCFlag()
		{
			GCFlag = true;
		}
		bool STDThread::GetGCFlag()
		{
			return GCFlag;
		}
		int STDThread::GetRefCount()
		{
			return Ref;
		}
		void STDThread::EnumReferences(VMCManager* Engine)
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
		int STDThread::Join(uint64_t Timeout)
		{
			{
				std::lock_guard<std::mutex> Guard(Mutex);
				if (std::this_thread::get_id() == Thread.get_id())
					return -1;

				if (!Thread.joinable())
					return -1;
			}
			{
				std::unique_lock<std::mutex> Guard(Mutex);
				if (CV.wait_for(Guard, std::chrono::milliseconds(Timeout), [&]
				{
					return !((Context && Context->GetState() != VMExecState::SUSPENDED));
				}))
				{
					Thread.join();
					return 1;
				}
			}

			return 0;
		}
		int STDThread::Join()
		{
			if (std::this_thread::get_id() == Thread.get_id())
				return -1;

			while (true)
			{
				int R = Join(1000);
				if (R == -1 || R == 1)
					return R;
			}

			return 0;
		}
		void STDThread::Push(void* fRef, int TypeId)
		{
			auto* fThread = GetThread();
			int Id = (fThread == this ? 1 : 0);

			void* Data = asAllocMem(sizeof(STDAny));
			STDAny* Any = new(Data) STDAny(fRef, TypeId, VMManager::Get()->GetEngine());
			Pipe[Id].Mutex.lock();
			Pipe[Id].Queue.push_back(Any);
			Pipe[Id].Mutex.unlock();
			Pipe[Id].CV.notify_one();
		}
		bool STDThread::Pop(void* fRef, int TypeId)
		{
			bool Resolved = false;
			while (!Resolved)
				Resolved = Pop(fRef, TypeId, 1000);

			return true;
		}
		bool STDThread::Pop(void* fRef, int TypeId, uint64_t Timeout)
		{
			auto* fThread = GetThread();
			int Id = (fThread == this ? 0 : 1);

			std::unique_lock<std::mutex> Guard(Pipe[Id].Mutex);
			if (!CV.wait_for(Guard, std::chrono::milliseconds(Timeout), [&]
			{
				return Pipe[Id].Queue.size() != 0;
			}))
				return false;

			STDAny* Result = Pipe[Id].Queue.front();
			if (!Result->Retrieve(fRef, TypeId))
				return false;

			Pipe[Id].Queue.erase(Pipe[Id].Queue.begin());
			Result->Release();

			return true;
		}
		bool STDThread::IsActive()
		{
			Mutex.lock();
			bool State = (Context && Context->GetState() != VMExecState::SUSPENDED);
			Mutex.unlock();

			return State;
		}
		bool STDThread::Start()
		{
			Mutex.lock();
			if (!Function)
			{
				Mutex.unlock();
				return false;
			}

			if (Context != nullptr)
			{
				if (Context->GetState() != VMExecState::SUSPENDED)
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
			else if (Thread.joinable())
			{
				if (std::this_thread::get_id() == Thread.get_id())
				{
					Mutex.unlock();
					return false;
				}

				Thread.join();
			}

			AddRef();
			Thread = std::thread(&STDThread::Routine, this);
			Mutex.unlock();

			return true;
		}
		void STDThread::ReleaseReferences(VMCManager*)
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
		void STDThread::Create(VMCGeneric* Generic)
		{
			VMCManager* Engine = Generic->GetEngine();
			VMCFunction* Function = *(VMCFunction**)Generic->GetAddressOfArg(0);
			void* Data = asAllocMem(sizeof(STDThread));
			*(STDThread**)Generic->GetAddressOfReturnLocation() = new(Data) STDThread(Engine, Function);
		}
		STDThread* STDThread::GetThread()
		{
			VMCContext* Context = asGetActiveContext();
			if (!Context)
				return nullptr;

			return static_cast<STDThread*>(Context->GetUserData(ContextUD));
		}
		void STDThread::ThreadSleep(uint64_t Timeout)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(Timeout));
		}
		uint64_t STDThread::GetThreadId()
		{
			return (uint64_t)std::hash<std::thread::id>()(std::this_thread::get_id());
		}
		int STDThread::ContextUD = 550;
		int STDThread::EngineListUD = 551;

		STDPromise::STDPromise(VMCContext* Base) : Context(Base), Future(nullptr), Ref(2), Flag(false)
		{
			if (!Context)
				return;

			VMContext* Next = VMContext::Get(Context);
			if (Next != nullptr)
				++Next->Promises;

			VMCManager* Engine = Context->GetEngine();
			Context->AddRef();
			Engine->NotifyGarbageCollectorOfNewObject(this, Engine->GetTypeInfoByName("Promise"));
		}
		void STDPromise::Release()
		{
			Flag = false;
			if (asAtomicDec(Ref) <= 0)
			{
				ReleaseReferences(nullptr);
				this->~STDPromise();
				asFreeMem((void*)this);
			}
		}
		void STDPromise::AddRef()
		{
			Flag = false;
			asAtomicInc(Ref);
		}
		void STDPromise::EnumReferences(VMCManager* Engine)
		{
			if (Context != nullptr)
				Engine->GCEnumCallback(Context);

			if (Future != nullptr)
				Engine->GCEnumCallback(Future);
		}
		void STDPromise::ReleaseReferences(VMCManager*)
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
		void STDPromise::SetGCFlag()
		{
			Flag = true;
		}
		bool STDPromise::GetGCFlag()
		{
			return Flag;
		}
		int STDPromise::GetRefCount()
		{
			return Ref;
		}
		int STDPromise::Set(void* fRef, int TypeId)
		{
			VMContext* Base = VMContext::Get(Context);
			if (!Base || Future != nullptr)
				return -1;

			void* Data = asAllocMem(sizeof(STDAny));
			STDAny* Result = new(Data) STDAny(fRef, TypeId, Context->GetEngine());
			Result->Release();

			if (TypeId & asTYPEID_OBJHANDLE)
			{
				VMCManager* Manager = Context->GetEngine();
				Manager->ReleaseScriptObject(*(void**)fRef, Manager->GetTypeInfoById(TypeId));
			}

			Result->AddRef();
			if (Future != nullptr)
				Future->Release();
			Future = Result;
			Release();

			return Core::Schedule::Get()->SetTask([this, Base]()
			{
				int State = Base->Execute(false);
				Base->Promises--;
				Base->ExecuteNotify(State);
			}) ? 0 : -1;
		}
		int STDPromise::Set(void* fRef, const char* TypeName)
		{
			if (!Context)
				return -1;

			return Set(fRef, Context->GetEngine()->GetTypeIdByDecl(TypeName));
		}
		bool STDPromise::To(void* fRef, int TypeId)
		{
			if (!Future)
				return false;

			return Future->Retrieve(fRef, TypeId);
		}
		void* STDPromise::Get()
		{
			if (!Future)
				return nullptr;

			int TypeId = Future->GetTypeId();
			if (TypeId & asTYPEID_OBJHANDLE)
				return &Future->Value.ValueObj;
			else if (TypeId & asTYPEID_MASK_OBJECT)
				return Future->Value.ValueObj;
			else if (TypeId <= asTYPEID_DOUBLE || TypeId & asTYPEID_MASK_SEQNBR)
				return &Future->Value.ValueInt;

			Context->SetException("retrieve this object explicitly with To(T& out)");
			return nullptr;
		}
		STDPromise* STDPromise::Create()
		{
			VMCContext* Context = asGetActiveContext();
			if (!Context)
				return nullptr;

			VMCManager* Engine = Context->GetEngine();
			if (!Engine)
				return nullptr;

			return new(asAllocMem(sizeof(STDPromise))) STDPromise(Context);
		}
		STDPromise* STDPromise::Jump(STDPromise* Value)
		{
			VMContext* Context = VMContext::Get();
			if (!Context || !Value)
				return Value;

			if (Value->Future != nullptr)
				--Context->Promises;
			else
				Context->Suspend();

			return Value;
		}
		
		bool STDRegisterAny(VMManager* Manager)
		{
			TH_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
			VMCManager* Engine = Manager->GetEngine();
			Engine->RegisterObjectType("Any", sizeof(STDAny), asOBJ_REF | asOBJ_GC);
			Engine->RegisterObjectBehaviour("Any", asBEHAVE_FACTORY, "Any@ f()", asFUNCTION(STDAny::Factory1), asCALL_GENERIC);
			Engine->RegisterObjectBehaviour("Any", asBEHAVE_FACTORY, "Any@ f(?&in) explicit", asFUNCTION(STDAny::Factory2), asCALL_GENERIC);
			Engine->RegisterObjectBehaviour("Any", asBEHAVE_FACTORY, "Any@ f(const int64&in) explicit", asFUNCTION(STDAny::Factory2), asCALL_GENERIC);
			Engine->RegisterObjectBehaviour("Any", asBEHAVE_FACTORY, "Any@ f(const double&in) explicit", asFUNCTION(STDAny::Factory2), asCALL_GENERIC);
			Engine->RegisterObjectBehaviour("Any", asBEHAVE_ADDREF, "void f()", asMETHOD(STDAny, AddRef), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Any", asBEHAVE_RELEASE, "void f()", asMETHOD(STDAny, Release), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Any", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(STDAny, GetRefCount), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Any", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(STDAny, SetFlag), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Any", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(STDAny, GetFlag), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Any", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(STDAny, EnumReferences), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Any", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(STDAny, ReleaseAllHandles), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Any", "Any &opAssign(Any&in)", asFUNCTION(STDAny::Assignment), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("Any", "void Store(?&in)", asMETHODPR(STDAny, Store, (void*, int), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Any", "bool Retrieve(?&out)", asMETHODPR(STDAny, Retrieve, (void*, int) const, bool), asCALL_THISCALL);
			return true;
		}
		bool STDRegisterArray(VMManager* Manager)
		{
			TH_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
			VMCManager* Engine = Manager->GetEngine();
			Engine->SetTypeInfoUserDataCleanupCallback(STDArray::CleanupTypeInfoCache, ARRAY_CACHE);
			Engine->RegisterObjectType("Array<class T>", 0, asOBJ_REF | asOBJ_GC | asOBJ_TEMPLATE);
			Engine->RegisterObjectBehaviour("Array<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(STDArray::TemplateCallback), asCALL_CDECL);
			Engine->RegisterObjectBehaviour("Array<T>", asBEHAVE_FACTORY, "Array<T>@ f(int&in)", asFUNCTIONPR(STDArray::Create, (VMCTypeInfo*), STDArray*), asCALL_CDECL);
			Engine->RegisterObjectBehaviour("Array<T>", asBEHAVE_FACTORY, "Array<T>@ f(int&in, uint length) explicit", asFUNCTIONPR(STDArray::Create, (VMCTypeInfo*, as_size_t), STDArray*), asCALL_CDECL);
			Engine->RegisterObjectBehaviour("Array<T>", asBEHAVE_FACTORY, "Array<T>@ f(int&in, uint length, const T &in value)", asFUNCTIONPR(STDArray::Create, (VMCTypeInfo*, as_size_t, void*), STDArray*), asCALL_CDECL);
			Engine->RegisterObjectBehaviour("Array<T>", asBEHAVE_LIST_FACTORY, "Array<T>@ f(int&in type, int&in list) {repeat T}", asFUNCTIONPR(STDArray::Create, (VMCTypeInfo*, void*), STDArray*), asCALL_CDECL);
			Engine->RegisterObjectBehaviour("Array<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(STDArray, AddRef), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Array<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(STDArray, Release), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "T &opIndex(uint index)", asMETHODPR(STDArray, At, (as_size_t), void*), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "const T &opIndex(uint index) const", asMETHODPR(STDArray, At, (as_size_t) const, const void*), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "Array<T> &opAssign(const Array<T>&in)", asMETHOD(STDArray, operator=), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void InsertAt(uint index, const T&in value)", asMETHODPR(STDArray, InsertAt, (as_size_t, void*), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void InsertAt(uint index, const Array<T>& arr)", asMETHODPR(STDArray, InsertAt, (as_size_t, const STDArray&), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void InsertLast(const T&in value)", asMETHOD(STDArray, InsertLast), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void RemoveAt(uint index)", asMETHOD(STDArray, RemoveAt), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void RemoveLast()", asMETHOD(STDArray, RemoveLast), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void RemoveRange(uint Start, uint count)", asMETHOD(STDArray, RemoveRange), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "uint Size() const", asMETHOD(STDArray, GetSize), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void Reserve(uint length)", asMETHOD(STDArray, Reserve), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void Resize(uint length)", asMETHODPR(STDArray, Resize, (as_size_t), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void SortAsc()", asMETHODPR(STDArray, SortAsc, (), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void SortAsc(uint startAt, uint count)", asMETHODPR(STDArray, SortAsc, (as_size_t, as_size_t), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void SortDesc()", asMETHODPR(STDArray, SortDesc, (), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void SortDesc(uint startAt, uint count)", asMETHODPR(STDArray, SortDesc, (as_size_t, as_size_t), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void Reverse()", asMETHOD(STDArray, Reverse), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "int Find(const T&in if_handle_then_const value) const", asMETHODPR(STDArray, Find, (void*) const, int), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "int Find(uint startAt, const T&in if_handle_then_const value) const", asMETHODPR(STDArray, Find, (as_size_t, void*) const, int), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "int FindByRef(const T&in if_handle_then_const value) const", asMETHODPR(STDArray, FindByRef, (void*) const, int), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "int FindByRef(uint startAt, const T&in if_handle_then_const value) const", asMETHODPR(STDArray, FindByRef, (as_size_t, void*) const, int), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "bool opEquals(const Array<T>&in) const", asMETHOD(STDArray, operator==), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "bool IsEmpty() const", asMETHOD(STDArray, IsEmpty), asCALL_THISCALL);
			Engine->RegisterFuncdef("bool Array<T>::Less(const T&in if_handle_then_const a, const T&in if_handle_then_const b)");
			Engine->RegisterObjectMethod("Array<T>", "void Sort(const Less &in, uint startAt = 0, uint count = uint(-1))", asMETHODPR(STDArray, Sort, (asIScriptFunction*, as_size_t, as_size_t), void), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Array<T>", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(STDArray, GetRefCount), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Array<T>", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(STDArray, SetFlag), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Array<T>", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(STDArray, GetFlag), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Array<T>", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(STDArray, EnumReferences), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Array<T>", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(STDArray, ReleaseAllHandles), asCALL_THISCALL);
			Engine->RegisterDefaultArrayType("Array<T>");
			return true;
		}
		bool STDRegisterComplex(VMManager* Manager)
		{
			TH_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
			VMCManager* Engine = Manager->GetEngine();
			Engine->RegisterObjectType("Complex", sizeof(STDComplex), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<STDComplex>() | asOBJ_APP_CLASS_ALLFLOATS);
			Engine->RegisterObjectProperty("Complex", "float R", asOFFSET(STDComplex, R));
			Engine->RegisterObjectProperty("Complex", "float I", asOFFSET(STDComplex, I));
			Engine->RegisterObjectBehaviour("Complex", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(STDComplex::DefaultConstructor), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectBehaviour("Complex", asBEHAVE_CONSTRUCT, "void f(const Complex &in)", asFUNCTION(STDComplex::CopyConstructor), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectBehaviour("Complex", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTION(STDComplex::ConvConstructor), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectBehaviour("Complex", asBEHAVE_CONSTRUCT, "void f(float, float)", asFUNCTION(STDComplex::InitConstructor), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectBehaviour("Complex", asBEHAVE_LIST_CONSTRUCT, "void f(const int &in) {float, float}", asFUNCTION(STDComplex::ListConstructor), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("Complex", "Complex &opAddAssign(const Complex &in)", asMETHODPR(STDComplex, operator+=, (const STDComplex&), STDComplex&), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Complex", "Complex &opSubAssign(const Complex &in)", asMETHODPR(STDComplex, operator-=, (const STDComplex&), STDComplex&), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Complex", "Complex &opMulAssign(const Complex &in)", asMETHODPR(STDComplex, operator*=, (const STDComplex&), STDComplex&), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Complex", "Complex &opDivAssign(const Complex &in)", asMETHODPR(STDComplex, operator/=, (const STDComplex&), STDComplex&), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Complex", "bool opEquals(const Complex &in) const", asMETHODPR(STDComplex, operator==, (const STDComplex&) const, bool), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Complex", "Complex opAdd(const Complex &in) const", asMETHODPR(STDComplex, operator+, (const STDComplex&) const, STDComplex), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Complex", "Complex opSub(const Complex &in) const", asMETHODPR(STDComplex, operator-, (const STDComplex&) const, STDComplex), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Complex", "Complex opMul(const Complex &in) const", asMETHODPR(STDComplex, operator*, (const STDComplex&) const, STDComplex), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Complex", "Complex opDiv(const Complex &in) const", asMETHODPR(STDComplex, operator/, (const STDComplex&) const, STDComplex), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Complex", "float Abs() const", asMETHOD(STDComplex, Length), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Complex", "Complex get_RI() const property", asMETHOD(STDComplex, GetRI), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Complex", "Complex get_IR() const property", asMETHOD(STDComplex, GetIR), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Complex", "void set_RI(const Complex &in) property", asMETHOD(STDComplex, SetRI), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Complex", "void set_IR(const Complex &in) property", asMETHOD(STDComplex, SetIR), asCALL_THISCALL);
			return true;
		}
		bool STDRegisterMap(VMManager* Manager)
		{
			TH_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
			VMCManager* Engine = Manager->GetEngine();
			Engine->RegisterObjectType("MapKey", sizeof(STDMapKey), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_GC | asGetTypeTraits<STDMapKey>());
			Engine->RegisterObjectBehaviour("MapKey", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(STDMap::KeyConstruct), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectBehaviour("MapKey", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(STDMap::KeyDestruct), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectBehaviour("MapKey", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(STDMapKey, EnumReferences), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("MapKey", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(STDMapKey, FreeValue), asCALL_THISCALL);
			Engine->RegisterObjectMethod("MapKey", "MapKey &opAssign(const MapKey &in)", asFUNCTIONPR(STDMap::KeyopAssign, (const STDMapKey&, STDMapKey*), STDMapKey&), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("MapKey", "MapKey &opHndlAssign(const ?&in)", asFUNCTIONPR(STDMap::KeyopAssign, (void*, int, STDMapKey*), STDMapKey&), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("MapKey", "MapKey &opHndlAssign(const MapKey &in)", asFUNCTIONPR(STDMap::KeyopAssign, (const STDMapKey&, STDMapKey*), STDMapKey&), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("MapKey", "MapKey &opAssign(const ?&in)", asFUNCTIONPR(STDMap::KeyopAssign, (void*, int, STDMapKey*), STDMapKey&), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("MapKey", "MapKey &opAssign(double)", asFUNCTIONPR(STDMap::KeyopAssign, (double, STDMapKey*), STDMapKey&), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("MapKey", "MapKey &opAssign(int64)", asFUNCTIONPR(STDMap::KeyopAssign, (as_int64_t, STDMapKey*), STDMapKey&), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("MapKey", "void opCast(?&out)", asFUNCTIONPR(STDMap::KeyopCast, (void*, int, STDMapKey*), void), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("MapKey", "void opConv(?&out)", asFUNCTIONPR(STDMap::KeyopCast, (void*, int, STDMapKey*), void), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("MapKey", "int64 opConv()", asFUNCTIONPR(STDMap::KeyopConvInt, (STDMapKey*), as_int64_t), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("MapKey", "double opConv()", asFUNCTIONPR(STDMap::KeyopConvDouble, (STDMapKey*), double), asCALL_CDECL_OBJLAST);

			Engine->RegisterObjectType("Map", sizeof(STDMap), asOBJ_REF | asOBJ_GC);
			Engine->RegisterObjectBehaviour("Map", asBEHAVE_FACTORY, "Map@ f()", asFUNCTION(STDMap::Factory), asCALL_GENERIC);
			Engine->RegisterObjectBehaviour("Map", asBEHAVE_LIST_FACTORY, "Map @f(int &in) {repeat {String, ?}}", asFUNCTION(STDMap::ListFactory), asCALL_GENERIC);
			Engine->RegisterObjectBehaviour("Map", asBEHAVE_ADDREF, "void f()", asMETHOD(STDMap, AddRef), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Map", asBEHAVE_RELEASE, "void f()", asMETHOD(STDMap, Release), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Map", "Map &opAssign(const Map &in)", asMETHODPR(STDMap, operator=, (const STDMap&), STDMap&), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Map", "void Set(const String &in, const ?&in)", asMETHODPR(STDMap, Set, (const std::string&, void*, int), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Map", "bool Get(const String &in, ?&out) const", asMETHODPR(STDMap, Get, (const std::string&, void*, int) const, bool), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Map", "bool Exists(const String &in) const", asMETHOD(STDMap, Exists), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Map", "bool IsEmpty() const", asMETHOD(STDMap, IsEmpty), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Map", "uint GetSize() const", asMETHOD(STDMap, GetSize), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Map", "bool Delete(const String &in)", asMETHOD(STDMap, Delete), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Map", "void DeleteAll()", asMETHOD(STDMap, DeleteAll), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Map", "Array<String>@ GetKeys() const", asMETHOD(STDMap, GetKeys), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Map", "MapKey &opIndex(const String &in)", asMETHODPR(STDMap, operator[], (const std::string&), STDMapKey*), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Map", "const MapKey &opIndex(const String &in) const", asMETHODPR(STDMap, operator[], (const std::string&) const, const STDMapKey*), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Map", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(STDMap, GetRefCount), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Map", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(STDMap, SetGCFlag), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Map", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(STDMap, GetGCFlag), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Map", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(STDMap, EnumReferences), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Map", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(STDMap, ReleaseAllReferences), asCALL_THISCALL);

			STDMap::Setup(Engine);
			return true;
		}
		bool STDRegisterGrid(VMManager* Manager)
		{
			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return false;

			Engine->RegisterObjectType("Grid<class T>", 0, asOBJ_REF | asOBJ_GC | asOBJ_TEMPLATE);
			Engine->RegisterObjectBehaviour("Grid<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(STDGrid::TemplateCallback), asCALL_CDECL);
			Engine->RegisterObjectBehaviour("Grid<T>", asBEHAVE_FACTORY, "Grid<T>@ f(int&in)", asFUNCTIONPR(STDGrid::Create, (VMCTypeInfo*), STDGrid*), asCALL_CDECL);
			Engine->RegisterObjectBehaviour("Grid<T>", asBEHAVE_FACTORY, "Grid<T>@ f(int&in, uint, uint)", asFUNCTIONPR(STDGrid::Create, (VMCTypeInfo*, as_size_t, as_size_t), STDGrid*), asCALL_CDECL);
			Engine->RegisterObjectBehaviour("Grid<T>", asBEHAVE_FACTORY, "Grid<T>@ f(int&in, uint, uint, const T &in)", asFUNCTIONPR(STDGrid::Create, (VMCTypeInfo*, as_size_t, as_size_t, void*), STDGrid*), asCALL_CDECL);
			Engine->RegisterObjectBehaviour("Grid<T>", asBEHAVE_LIST_FACTORY, "Grid<T>@ f(int&in type, int&in list) {repeat {repeat_same T}}", asFUNCTIONPR(STDGrid::Create, (VMCTypeInfo*, void*), STDGrid*), asCALL_CDECL);
			Engine->RegisterObjectBehaviour("Grid<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(STDGrid, AddRef), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Grid<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(STDGrid, Release), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Grid<T>", "T &opIndex(uint, uint)", asMETHODPR(STDGrid, At, (as_size_t, as_size_t), void*), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Grid<T>", "const T &opIndex(uint, uint) const", asMETHODPR(STDGrid, At, (as_size_t, as_size_t) const, const void*), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Grid<T>", "void Resize(uint, uint)", asMETHODPR(STDGrid, Resize, (as_size_t, as_size_t), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Grid<T>", "uint Width() const", asMETHOD(STDGrid, GetWidth), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Grid<T>", "uint Height() const", asMETHOD(STDGrid, GetHeight), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Grid<T>", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(STDGrid, GetRefCount), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Grid<T>", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(STDGrid, SetFlag), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Grid<T>", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(STDGrid, GetFlag), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Grid<T>", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(STDGrid, EnumReferences), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Grid<T>", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(STDGrid, ReleaseAllHandles), asCALL_THISCALL);

			return true;
		}
		bool STDRegisterRef(VMManager* Manager)
		{
			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return false;

			Engine->RegisterObjectType("Ref", sizeof(STDRef), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_GC | asGetTypeTraits<STDRef>());
			Engine->RegisterObjectBehaviour("Ref", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR(STDRef::Construct, (STDRef*), void), asCALL_CDECL_OBJFIRST);
			Engine->RegisterObjectBehaviour("Ref", asBEHAVE_CONSTRUCT, "void f(const Ref &in)", asFUNCTIONPR(STDRef::Construct, (STDRef*, const STDRef&), void), asCALL_CDECL_OBJFIRST);
			Engine->RegisterObjectBehaviour("Ref", asBEHAVE_CONSTRUCT, "void f(const ?&in)", asFUNCTIONPR(STDRef::Construct, (STDRef*, void*, int), void), asCALL_CDECL_OBJFIRST);
			Engine->RegisterObjectBehaviour("Ref", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR(STDRef::Destruct, (STDRef*), void), asCALL_CDECL_OBJFIRST);
			Engine->RegisterObjectBehaviour("Ref", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(STDRef, EnumReferences), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Ref", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(STDRef, ReleaseReferences), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Ref", "void opCast(?&out)", asMETHODPR(STDRef, Cast, (void**, int), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Ref", "Ref &opHndlAssign(const Ref &in)", asMETHOD(STDRef, operator=), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Ref", "Ref &opHndlAssign(const ?&in)", asMETHOD(STDRef, Assign), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Ref", "bool opEquals(const Ref &in) const", asMETHODPR(STDRef, operator==, (const STDRef&) const, bool), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Ref", "bool opEquals(const ?&in) const", asMETHODPR(STDRef, Equals, (void*, int) const, bool), asCALL_THISCALL);
			return true;
		}
		bool STDRegisterWeakRef(VMManager* Manager)
		{
			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return false;

			Engine->RegisterObjectType("WeakRef<class T>", sizeof(STDWeakRef), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_TEMPLATE | asOBJ_APP_CLASS_DAK);
			Engine->RegisterObjectBehaviour("WeakRef<T>", asBEHAVE_CONSTRUCT, "void f(int&in)", asFUNCTION(STDWeakRef::Construct), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectBehaviour("WeakRef<T>", asBEHAVE_CONSTRUCT, "void f(int&in, T@+)", asFUNCTION(STDWeakRef::Construct2), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectBehaviour("WeakRef<T>", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(STDWeakRef::Destruct), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectBehaviour("WeakRef<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(STDWeakRef::TemplateCallback), asCALL_CDECL);
			Engine->RegisterObjectMethod("WeakRef<T>", "T@ opImplCast()", asMETHOD(STDWeakRef, Get), asCALL_THISCALL);
			Engine->RegisterObjectMethod("WeakRef<T>", "T@ Get() const", asMETHODPR(STDWeakRef, Get, () const, void*), asCALL_THISCALL);
			Engine->RegisterObjectMethod("WeakRef<T>", "WeakRef<T> &opHndlAssign(const WeakRef<T> &in)", asMETHOD(STDWeakRef, operator=), asCALL_THISCALL);
			Engine->RegisterObjectMethod("WeakRef<T>", "WeakRef<T> &opAssign(const WeakRef<T> &in)", asMETHOD(STDWeakRef, operator=), asCALL_THISCALL);
			Engine->RegisterObjectMethod("WeakRef<T>", "bool opEquals(const WeakRef<T> &in) const", asMETHODPR(STDWeakRef, operator==, (const STDWeakRef&) const, bool), asCALL_THISCALL);
			Engine->RegisterObjectMethod("WeakRef<T>", "WeakRef<T> &opHndlAssign(T@)", asMETHOD(STDWeakRef, Set), asCALL_THISCALL);
			Engine->RegisterObjectMethod("WeakRef<T>", "bool opEquals(const T@+) const", asMETHOD(STDWeakRef, Equals), asCALL_THISCALL);
			Engine->RegisterObjectType("ConstWeakRef<class T>", sizeof(STDWeakRef), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_TEMPLATE | asOBJ_APP_CLASS_DAK);
			Engine->RegisterObjectBehaviour("ConstWeakRef<T>", asBEHAVE_CONSTRUCT, "void f(int&in)", asFUNCTION(STDWeakRef::Construct), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectBehaviour("ConstWeakRef<T>", asBEHAVE_CONSTRUCT, "void f(int&in, const T@+)", asFUNCTION(STDWeakRef::Construct2), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectBehaviour("ConstWeakRef<T>", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(STDWeakRef::Destruct), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectBehaviour("ConstWeakRef<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(STDWeakRef::TemplateCallback), asCALL_CDECL);
			Engine->RegisterObjectMethod("ConstWeakRef<T>", "const T@ opImplCast() const", asMETHOD(STDWeakRef, Get), asCALL_THISCALL);
			Engine->RegisterObjectMethod("ConstWeakRef<T>", "const T@ Get() const", asMETHODPR(STDWeakRef, Get, () const, void*), asCALL_THISCALL);
			Engine->RegisterObjectMethod("ConstWeakRef<T>", "ConstWeakRef<T> &opHndlAssign(const ConstWeakRef<T> &in)", asMETHOD(STDWeakRef, operator=), asCALL_THISCALL);
			Engine->RegisterObjectMethod("ConstWeakRef<T>", "ConstWeakRef<T> &opAssign(const ConstWeakRef<T> &in)", asMETHOD(STDWeakRef, operator=), asCALL_THISCALL);
			Engine->RegisterObjectMethod("ConstWeakRef<T>", "bool opEquals(const ConstWeakRef<T> &in) const", asMETHODPR(STDWeakRef, operator==, (const STDWeakRef&) const, bool), asCALL_THISCALL);
			Engine->RegisterObjectMethod("ConstWeakRef<T>", "ConstWeakRef<T> &opHndlAssign(const T@)", asMETHOD(STDWeakRef, Set), asCALL_THISCALL);
			Engine->RegisterObjectMethod("ConstWeakRef<T>", "bool opEquals(const T@+) const", asMETHOD(STDWeakRef, Equals), asCALL_THISCALL);
			Engine->RegisterObjectMethod("ConstWeakRef<T>", "ConstWeakRef<T> &opHndlAssign(const WeakRef<T> &in)", asMETHOD(STDWeakRef, operator=), asCALL_THISCALL);
			Engine->RegisterObjectMethod("ConstWeakRef<T>", "bool opEquals(const WeakRef<T> &in) const", asMETHODPR(STDWeakRef, operator==, (const STDWeakRef&) const, bool), asCALL_THISCALL);
			return true;
		}
		bool STDRegisterMath(VMManager* Manager)
		{
			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return false;

			Engine->RegisterGlobalFunction("float FpFromIEEE(uint)", asFUNCTIONPR(STDMath::FpFromIEEE, (as_size_t), float), asCALL_CDECL);
			Engine->RegisterGlobalFunction("uint FpToIEEE(float)", asFUNCTIONPR(STDMath::FpToIEEE, (float), as_size_t), asCALL_CDECL);
			Engine->RegisterGlobalFunction("double FpFromIEEE(uint64)", asFUNCTIONPR(STDMath::FpFromIEEE, (as_uint64_t), double), asCALL_CDECL);
			Engine->RegisterGlobalFunction("uint64 FpToIEEE(double)", asFUNCTIONPR(STDMath::FpToIEEE, (double), as_uint64_t), asCALL_CDECL);
			Engine->RegisterGlobalFunction("bool CloseTo(float, float, float = 0.00001f)", asFUNCTIONPR(STDMath::CloseTo, (float, float, float), bool), asCALL_CDECL);
			Engine->RegisterGlobalFunction("bool CloseTo(double, double, double = 0.0000000001)", asFUNCTIONPR(STDMath::CloseTo, (double, double, double), bool), asCALL_CDECL);
#if !defined(_WIN32_WCE)
			Engine->RegisterGlobalFunction("float Cos(float)", asFUNCTIONPR(cosf, (float), float), asCALL_CDECL);
			Engine->RegisterGlobalFunction("float Sin(float)", asFUNCTIONPR(sinf, (float), float), asCALL_CDECL);
			Engine->RegisterGlobalFunction("float Tan(float)", asFUNCTIONPR(tanf, (float), float), asCALL_CDECL);
			Engine->RegisterGlobalFunction("float Acos(float)", asFUNCTIONPR(acosf, (float), float), asCALL_CDECL);
			Engine->RegisterGlobalFunction("float Asin(float)", asFUNCTIONPR(asinf, (float), float), asCALL_CDECL);
			Engine->RegisterGlobalFunction("float Atan(float)", asFUNCTIONPR(atanf, (float), float), asCALL_CDECL);
			Engine->RegisterGlobalFunction("float Atan2(float,float)", asFUNCTIONPR(atan2f, (float, float), float), asCALL_CDECL);
			Engine->RegisterGlobalFunction("float Cosh(float)", asFUNCTIONPR(coshf, (float), float), asCALL_CDECL);
			Engine->RegisterGlobalFunction("float Sinh(float)", asFUNCTIONPR(sinhf, (float), float), asCALL_CDECL);
			Engine->RegisterGlobalFunction("float Tanh(float)", asFUNCTIONPR(tanhf, (float), float), asCALL_CDECL);
			Engine->RegisterGlobalFunction("float Log(float)", asFUNCTIONPR(logf, (float), float), asCALL_CDECL);
			Engine->RegisterGlobalFunction("float Log10(float)", asFUNCTIONPR(log10f, (float), float), asCALL_CDECL);
			Engine->RegisterGlobalFunction("float Pow(float, float)", asFUNCTIONPR(powf, (float, float), float), asCALL_CDECL);
			Engine->RegisterGlobalFunction("float Sqrt(float)", asFUNCTIONPR(sqrtf, (float), float), asCALL_CDECL);
			Engine->RegisterGlobalFunction("float Ceil(float)", asFUNCTIONPR(ceilf, (float), float), asCALL_CDECL);
			Engine->RegisterGlobalFunction("float Abs(float)", asFUNCTIONPR(fabsf, (float), float), asCALL_CDECL);
			Engine->RegisterGlobalFunction("float Floor(float)", asFUNCTIONPR(floorf, (float), float), asCALL_CDECL);
			Engine->RegisterGlobalFunction("float Fraction(float)", asFUNCTIONPR(fracf, (float), float), asCALL_CDECL);
#else
			Engine->RegisterGlobalFunction("double Cos(double)", asFUNCTIONPR(cos, (double), double), asCALL_CDECL);
			Engine->RegisterGlobalFunction("double Sin(double)", asFUNCTIONPR(sin, (double), double), asCALL_CDECL);
			Engine->RegisterGlobalFunction("double Tan(double)", asFUNCTIONPR(tan, (double), double), asCALL_CDECL);
			Engine->RegisterGlobalFunction("double Acos(double)", asFUNCTIONPR(acos, (double), double), asCALL_CDECL);
			Engine->RegisterGlobalFunction("double Asin(double)", asFUNCTIONPR(asin, (double), double), asCALL_CDECL);
			Engine->RegisterGlobalFunction("double Atan(double)", asFUNCTIONPR(atan, (double), double), asCALL_CDECL);
			Engine->RegisterGlobalFunction("double Atan2(double,double)", asFUNCTIONPR(atan2, (double, double), double), asCALL_CDECL);
			Engine->RegisterGlobalFunction("double Cosh(double)", asFUNCTIONPR(cosh, (double), double), asCALL_CDECL);
			Engine->RegisterGlobalFunction("double Sinh(double)", asFUNCTIONPR(sinh, (double), double), asCALL_CDECL);
			Engine->RegisterGlobalFunction("double Tanh(double)", asFUNCTIONPR(tanh, (double), double), asCALL_CDECL);
			Engine->RegisterGlobalFunction("double Log(double)", asFUNCTIONPR(log, (double), double), asCALL_CDECL);
			Engine->RegisterGlobalFunction("double Log10(double)", asFUNCTIONPR(log10, (double), double), asCALL_CDECL);
			Engine->RegisterGlobalFunction("double Pow(double, double)", asFUNCTIONPR(pow, (double, double), double), asCALL_CDECL);
			Engine->RegisterGlobalFunction("double Sqrt(double)", asFUNCTIONPR(sqrt, (double), double), asCALL_CDECL);
			Engine->RegisterGlobalFunction("double Ceil(double)", asFUNCTIONPR(ceil, (double), double), asCALL_CDECL);
			Engine->RegisterGlobalFunction("double Abs(double)", asFUNCTIONPR(fabs, (double), double), asCALL_CDECL);
			Engine->RegisterGlobalFunction("double Floor(double)", asFUNCTIONPR(floor, (double), double), asCALL_CDECL);
			Engine->RegisterGlobalFunction("double Fraction(double)", asFUNCTIONPR(frac, (double), double), asCALL_CDECL);
#endif
			return true;
		}
		bool STDRegisterString(VMManager* Manager)
		{
			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return false;

			Engine->RegisterObjectType("String", sizeof(std::string), asOBJ_VALUE | asGetTypeTraits<std::string>());
			Engine->RegisterStringFactory("String", StringFactory::Get());
			Engine->RegisterObjectBehaviour("String", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(STDString::Construct), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectBehaviour("String", asBEHAVE_CONSTRUCT, "void f(const String &in)", asFUNCTION(STDString::CopyConstruct), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectBehaviour("String", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(STDString::Destruct), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String &opAssign(const String &in)", asMETHODPR(std::string, operator =, (const std::string&), std::string&), asCALL_THISCALL);
			Engine->RegisterObjectMethod("String", "String &opAddAssign(const String &in)", asFUNCTION(STDString::AddAssignTo), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "bool opEquals(const String &in) const", asFUNCTIONPR(STDString::Equals, (const std::string&, const std::string&), bool), asCALL_CDECL_OBJFIRST);
			Engine->RegisterObjectMethod("String", "int opCmp(const String &in) const", asFUNCTION(STDString::Cmp), asCALL_CDECL_OBJFIRST);
			Engine->RegisterObjectMethod("String", "String opAdd(const String &in) const", asFUNCTIONPR(std::operator +, (const std::string&, const std::string&), std::string), asCALL_CDECL_OBJFIRST);
			Engine->RegisterObjectMethod("String", "uint Size() const", asFUNCTION(STDString::Length), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "void Resize(uint)", asFUNCTION(STDString::Resize), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "bool IsEmpty() const", asFUNCTION(STDString::IsEmpty), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "uint8 &opIndex(uint)", asFUNCTION(STDString::CharAt), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "const uint8 &opIndex(uint) const", asFUNCTION(STDString::CharAt), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String &opAssign(double)", asFUNCTION(STDString::AssignDoubleTo), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String &opAddAssign(double)", asFUNCTION(STDString::AddAssignDoubleTo), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String opAdd(double) const", asFUNCTION(STDString::AddDouble1), asCALL_CDECL_OBJFIRST);
			Engine->RegisterObjectMethod("String", "String opAdd_r(double) const", asFUNCTION(STDString::AddDouble2), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String &opAssign(float)", asFUNCTION(STDString::AssignFloatTo), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String &opAddAssign(float)", asFUNCTION(STDString::AddAssignFloatTo), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String opAdd(float) const", asFUNCTION(STDString::AddFloat1), asCALL_CDECL_OBJFIRST);
			Engine->RegisterObjectMethod("String", "String opAdd_r(float) const", asFUNCTION(STDString::AddFloat2), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String &opAssign(int64)", asFUNCTION(STDString::AssignInt64To), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String &opAddAssign(int64)", asFUNCTION(STDString::AddAssignInt64To), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String opAdd(int64) const", asFUNCTION(STDString::AddInt641), asCALL_CDECL_OBJFIRST);
			Engine->RegisterObjectMethod("String", "String opAdd_r(int64) const", asFUNCTION(STDString::AddInt642), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String &opAssign(uint64)", asFUNCTION(STDString::AssignUInt64To), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String &opAddAssign(uint64)", asFUNCTION(STDString::AddAssignUInt64To), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String opAdd(uint64) const", asFUNCTION(STDString::AddUInt641), asCALL_CDECL_OBJFIRST);
			Engine->RegisterObjectMethod("String", "String opAdd_r(uint64) const", asFUNCTION(STDString::AddUInt642), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String &opAssign(bool)", asFUNCTION(STDString::AssignBoolTo), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String &opAddAssign(bool)", asFUNCTION(STDString::AddAssignBoolTo), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String opAdd(bool) const", asFUNCTION(STDString::AddBool1), asCALL_CDECL_OBJFIRST);
			Engine->RegisterObjectMethod("String", "String opAdd_r(bool) const", asFUNCTION(STDString::AddBool2), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String Substr(uint Start = 0, int count = -1) const", asFUNCTION(STDString::Sub), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "int FindFirst(const String &in, uint Start = 0) const", asFUNCTION(STDString::FindFirst), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "int FindFirstOf(const String &in, uint Start = 0) const", asFUNCTION(STDString::FindFirstOf), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "int FindFirstNotOf(const String &in, uint Start = 0) const", asFUNCTION(STDString::FindFirstNotOf), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "int FindLast(const String &in, int Start = -1) const", asFUNCTION(STDString::FindLast), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "int FindLastOf(const String &in, int Start = -1) const", asFUNCTION(STDString::FindLastOf), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "int FindLastNotOf(const String &in, int Start = -1) const", asFUNCTION(STDString::FindLastNotOf), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "void Insert(uint Offset, const String &in Other)", asFUNCTION(STDString::Insert), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "void Erase(uint Offset, int count = -1)", asFUNCTION(STDString::Erase), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String Replace(const String &in, const String &in, uint64 o = 0)", asFUNCTION(STDString::Replace), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "Array<String>@ Split(const String &in) const", asFUNCTION(STDString::Split), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String ToLower() const", asFUNCTION(STDString::ToLower), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String ToUpper() const", asFUNCTION(STDString::ToUpper), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String Reverse() const", asFUNCTION(STDString::Reverse), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "Address@ GetAddress() const", asFUNCTION(STDString::ToPtr), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "Address@ opImplCast()", asFUNCTION(STDString::ToPtr), asCALL_CDECL_OBJFIRST);
			Engine->RegisterObjectMethod("String", "Address@ opImplConv() const", asFUNCTION(STDString::ToPtr), asCALL_CDECL_OBJFIRST);
			Engine->RegisterObjectMethod("String", "Address@ opImplCast() const", asFUNCTION(STDString::ToPtr), asCALL_CDECL_OBJFIRST);
			Engine->RegisterGlobalFunction("int64 ToInt(const String &in, uint Base = 10, uint &out ByteCount = 0)", asFUNCTION(STDString::IntStore), asCALL_CDECL);
			Engine->RegisterGlobalFunction("uint64 ToUInt(const String &in, uint Base = 10, uint &out ByteCount = 0)", asFUNCTION(STDString::UIntStore), asCALL_CDECL);
			Engine->RegisterGlobalFunction("double ToFloat(const String &in, uint &out ByteCount = 0)", asFUNCTION(STDString::FloatStore), asCALL_CDECL);
			Engine->RegisterGlobalFunction("uint8 ToChar(const String &in)", asFUNCTION(STDString::ToChar), asCALL_CDECL);
			Engine->RegisterGlobalFunction("String ToString(const Array<String> &in, const String &in)", asFUNCTION(STDString::Join), asCALL_CDECL);
			Engine->RegisterGlobalFunction("String ToString(int8)", asFUNCTION(STDString::ToInt8), asCALL_CDECL);
			Engine->RegisterGlobalFunction("String ToString(int16)", asFUNCTION(STDString::ToInt16), asCALL_CDECL);
			Engine->RegisterGlobalFunction("String ToString(int)", asFUNCTION(STDString::ToInt), asCALL_CDECL);
			Engine->RegisterGlobalFunction("String ToString(int64)", asFUNCTION(STDString::ToInt64), asCALL_CDECL);
			Engine->RegisterGlobalFunction("String ToString(uint8)", asFUNCTION(STDString::ToUInt8), asCALL_CDECL);
			Engine->RegisterGlobalFunction("String ToString(uint16)", asFUNCTION(STDString::ToUInt16), asCALL_CDECL);
			Engine->RegisterGlobalFunction("String ToString(uint)", asFUNCTION(STDString::ToUInt), asCALL_CDECL);
			Engine->RegisterGlobalFunction("String ToString(uint64)", asFUNCTION(STDString::ToUInt64), asCALL_CDECL);
			Engine->RegisterGlobalFunction("String ToString(float)", asFUNCTION(STDString::ToFloat), asCALL_CDECL);
			Engine->RegisterGlobalFunction("String ToString(double)", asFUNCTION(STDString::ToDouble), asCALL_CDECL);

			return true;
		}
		bool STDRegisterException(VMManager* Manager)
		{
			TH_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
			VMCManager* Engine = Manager->GetEngine();
			Engine->RegisterGlobalFunction("void Throw(const String &in = \"\")", asFUNCTION(STDException::Throw), asCALL_CDECL);
			Engine->RegisterGlobalFunction("String GetException()", asFUNCTION(STDException::GetException), asCALL_CDECL);
			return true;
		}
		bool STDRegisterMutex(VMManager* Manager)
		{
			TH_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
			VMCManager* Engine = Manager->GetEngine();
			Engine->RegisterObjectType("Mutex", sizeof(STDMutex), asOBJ_REF);
			Engine->RegisterObjectBehaviour("Mutex", asBEHAVE_FACTORY, "Mutex@ f()", asFUNCTION(STDMutex::Factory), asCALL_CDECL);
			Engine->RegisterObjectBehaviour("Mutex", asBEHAVE_ADDREF, "void f()", asMETHOD(STDMutex, AddRef), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Mutex", asBEHAVE_RELEASE, "void f()", asMETHOD(STDMutex, Release), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Mutex", "bool TryLock()", asMETHOD(STDMutex, TryLock), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Mutex", "void Lock()", asMETHOD(STDMutex, Lock), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Mutex", "void Unlock()", asMETHOD(STDMutex, Unlock), asCALL_THISCALL);
			return true;
		}
		bool STDRegisterThread(VMManager* Manager)
		{
			TH_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
			VMCManager* Engine = Manager->GetEngine();
			Engine->RegisterObjectType("Thread", 0, asOBJ_REF | asOBJ_GC);
			Engine->RegisterFuncdef("void ThreadEvent(Thread@+)");
			Engine->RegisterObjectBehaviour("Thread", asBEHAVE_FACTORY, "Thread@ f(ThreadEvent@)", asFUNCTION(STDThread::Create), asCALL_GENERIC);
			Engine->RegisterObjectBehaviour("Thread", asBEHAVE_ADDREF, "void f()", asMETHOD(STDThread, AddRef), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Thread", asBEHAVE_RELEASE, "void f()", asMETHOD(STDThread, Release), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Thread", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(STDThread, SetGCFlag), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Thread", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(STDThread, GetGCFlag), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Thread", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(STDThread, GetRefCount), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Thread", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(STDThread, EnumReferences), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Thread", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(STDThread, ReleaseReferences), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Thread", "bool IsActive()", asMETHOD(STDThread, IsActive), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Thread", "bool Invoke()", asMETHOD(STDThread, Start), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Thread", "void Suspend()", asMETHOD(STDThread, Suspend), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Thread", "void Resume()", asMETHOD(STDThread, Resume), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Thread", "void Push(const ?&in)", asMETHOD(STDThread, Push), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Thread", "bool Pop(?&out)", asMETHODPR(STDThread, Pop, (void*, int), bool), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Thread", "bool Pop(?&out, uint64)", asMETHODPR(STDThread, Pop, (void*, int, uint64_t), bool), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Thread", "int Join(uint64)", asMETHODPR(STDThread, Join, (uint64_t), int), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Thread", "int Join()", asMETHODPR(STDThread, Join, (), int), asCALL_THISCALL);
			Engine->RegisterGlobalFunction("Thread@+ GetThread()", asFUNCTION(STDThread::GetThread), asCALL_CDECL);
			Engine->RegisterGlobalFunction("uint64 GetThreadId()", asFUNCTION(STDThread::GetThreadId), asCALL_CDECL);
			Engine->RegisterGlobalFunction("void Sleep(uint64)", asFUNCTION(STDThread::ThreadSleep), asCALL_CDECL);
			return true;
		}
		bool STDRegisterRandom(VMManager* Manager)
		{
			TH_ASSERT(Manager != nullptr && Manager->GetEngine() != nullptr, false, "manager should be set");
			VMCManager* Engine = Manager->GetEngine();
			Engine->RegisterObjectType("Random", 0, asOBJ_REF);
			Engine->RegisterObjectBehaviour("Random", asBEHAVE_FACTORY, "Random@ f()", asFUNCTION(STDRandom::Create), asCALL_CDECL);
			Engine->RegisterObjectBehaviour("Random", asBEHAVE_ADDREF, "void f()", asMETHOD(STDRandom, AddRef), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Random", asBEHAVE_RELEASE, "void f()", asMETHOD(STDRandom, Release), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Random", "void opAssign(const Random&)", asMETHODPR(STDRandom, Assign, (STDRandom*), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Random", "void Seed(uint)", asMETHODPR(STDRandom, Seed, (uint32_t), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Random", "void Seed(uint[]&)", asMETHODPR(STDRandom, Seed, (STDArray*), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Random", "int GetI()", asMETHOD(STDRandom, GetI), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Random", "uint GetU()", asMETHOD(STDRandom, GetU), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Random", "double GetD()", asMETHOD(STDRandom, GetD), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Random", "void SeedFromTime()", asMETHOD(STDRandom, SeedFromTime), asCALL_THISCALL);
			return true;
		}
		bool STDRegisterPromise(VMManager* Manager)
		{
			VMCManager* Engine = Manager->GetEngine();
			if (!Engine)
				return false;

			Engine->RegisterObjectType("Promise<class T>", 0, asOBJ_REF | asOBJ_GC | asOBJ_TEMPLATE);
			Engine->RegisterObjectBehaviour("Promise<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(STDArray::TemplateCallback), asCALL_CDECL);
			Engine->RegisterObjectBehaviour("Promise<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(STDPromise, AddRef), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Promise<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(STDPromise, Release), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Promise<T>", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(STDPromise, SetGCFlag), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Promise<T>", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(STDPromise, GetGCFlag), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Promise<T>", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(STDPromise, GetRefCount), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Promise<T>", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(STDPromise, EnumReferences), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Promise<T>", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(STDPromise, ReleaseReferences), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Promise<T>", "bool To(?&out)", asMETHODPR(STDPromise, To, (void*, int), bool), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Promise<T>", "T& Get()", asMETHOD(STDPromise, Get), asCALL_THISCALL);
			return true;
		}
		bool STDFreeCore()
		{
			StringFactory::Free();
			return false;
		}
	}
}
