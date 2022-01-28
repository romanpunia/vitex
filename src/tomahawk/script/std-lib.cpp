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

		std::unordered_map<uint64_t, std::pair<std::string, int>>* Names = nullptr;
		uint64_t STDRegistry::Set(uint64_t Id, const std::string& Name)
		{
			TH_ASSERT(Id > 0 && !Name.empty(), 0, "id should be greater than zero and name should not be empty");
			if (!Names)
				Names = new std::unordered_map<uint64_t, std::pair<std::string, int>>();

			(*Names)[Id] = std::make_pair(Name, (int)-1);
			return Id;
		}
		int STDRegistry::GetTypeId(uint64_t Id)
		{
			auto It = Names->find(Id);
			if (It == Names->end())
				return -1;

			if (It->second.second < 0)
			{
				VMManager* Engine = VMManager::Get();
				TH_ASSERT(Engine != nullptr, -1, "engine should be set");
				It->second.second = Engine->Global().GetTypeIdByDecl(It->second.first.c_str());
			}

			return It->second.second;
		}

		void STDString::Construct(std::string* Current)
		{
			TH_ASSERT_V(Current != nullptr, "Current should be set");
			new(Current) std::string();
		}
		void STDString::CopyConstruct(const std::string& Other, std::string* Current)
		{
			TH_ASSERT_V(Current != nullptr, "Current should be set");
			new(Current) std::string(Other);
		}
		void STDString::Destruct(std::string* Current)
		{
			TH_ASSERT_V(Current != nullptr, "Current should be set");
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
		std::string& STDString::AssignUInt64To(as_uint64_t Value, std::string& Dest)
		{
			std::ostringstream Stream;
			Stream << Value;
			Dest = Stream.str();
			return Dest;
		}
		std::string& STDString::AddAssignUInt64To(as_uint64_t Value, std::string& Dest)
		{
			std::ostringstream Stream;
			Stream << Value;
			Dest += Stream.str();
			return Dest;
		}
		std::string STDString::AddUInt641(const std::string& Current, as_uint64_t Value)
		{
			std::ostringstream Stream;
			Stream << Value;
			return Current + Stream.str();
		}
		std::string STDString::AddInt641(as_int64_t Value, const std::string& Current)
		{
			std::ostringstream Stream;
			Stream << Value;
			return Stream.str() + Current;
		}
		std::string& STDString::AssignInt64To(as_int64_t Value, std::string& Dest)
		{
			std::ostringstream Stream;
			Stream << Value;
			Dest = Stream.str();
			return Dest;
		}
		std::string& STDString::AddAssignInt64To(as_int64_t Value, std::string& Dest)
		{
			std::ostringstream Stream;
			Stream << Value;
			Dest += Stream.str();
			return Dest;
		}
		std::string STDString::AddInt642(const std::string& Current, as_int64_t Value)
		{
			std::ostringstream Stream;
			Stream << Value;
			return Current + Stream.str();
		}
		std::string STDString::AddUInt642(as_uint64_t Value, const std::string& Current)
		{
			std::ostringstream Stream;
			Stream << Value;
			return Stream.str() + Current;
		}
		std::string& STDString::AssignDoubleTo(double Value, std::string& Dest)
		{
			std::ostringstream Stream;
			Stream << Value;
			Dest = Stream.str();
			return Dest;
		}
		std::string& STDString::AddAssignDoubleTo(double Value, std::string& Dest)
		{
			std::ostringstream Stream;
			Stream << Value;
			Dest += Stream.str();
			return Dest;
		}
		std::string& STDString::AssignFloatTo(float Value, std::string& Dest)
		{
			std::ostringstream Stream;
			Stream << Value;
			Dest = Stream.str();
			return Dest;
		}
		std::string& STDString::AddAssignFloatTo(float Value, std::string& Dest)
		{
			std::ostringstream Stream;
			Stream << Value;
			Dest += Stream.str();
			return Dest;
		}
		std::string& STDString::AssignBoolTo(bool Value, std::string& Dest)
		{
			std::ostringstream Stream;
			Stream << (Value ? "true" : "false");
			Dest = Stream.str();
			return Dest;
		}
		std::string& STDString::AddAssignBoolTo(bool Value, std::string& Dest)
		{
			std::ostringstream Stream;
			Stream << (Value ? "true" : "false");
			Dest += Stream.str();
			return Dest;
		}
		std::string STDString::AddDouble1(const std::string& Current, double Value)
		{
			std::ostringstream Stream;
			Stream << Value;
			return Current + Stream.str();
		}
		std::string STDString::AddDouble2(double Value, const std::string& Current)
		{
			std::ostringstream Stream;
			Stream << Value;
			return Stream.str() + Current;
		}
		std::string STDString::AddFloat1(const std::string& Current, float Value)
		{
			std::ostringstream Stream;
			Stream << Value;
			return Current + Stream.str();
		}
		std::string STDString::AddFloat2(float Value, const std::string& Current)
		{
			std::ostringstream Stream;
			Stream << Value;
			return Stream.str() + Current;
		}
		std::string STDString::AddBool1(const std::string& Current, bool Value)
		{
			std::ostringstream Stream;
			Stream << (Value ? "true" : "false");
			return Current + Stream.str();
		}
		std::string STDString::AddBool2(bool Value, const std::string& Current)
		{
			std::ostringstream Stream;
			Stream << (Value ? "true" : "false");
			return Stream.str() + Current;
		}
		char* STDString::CharAt(unsigned int Value, std::string& Current)
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
		int STDString::Cmp(const std::string& A, const std::string& B)
		{
			int Result = 0;
			if (A < B)
				Result = -1;
			else if (A > B)
				Result = 1;

			return Result;
		}
		int STDString::FindFirst(const std::string& Needle, as_size_t Start, const std::string& Current)
		{
			return (int)Current.find(Needle, (size_t)Start);
		}
		int STDString::FindFirstOf(const std::string& Needle, as_size_t Start, const std::string& Current)
		{
			return (int)Current.find_first_of(Needle, (size_t)Start);
		}
		int STDString::FindLastOf(const std::string& Needle, as_size_t Start, const std::string& Current)
		{
			return (int)Current.find_last_of(Needle, (size_t)Start);
		}
		int STDString::FindFirstNotOf(const std::string& Needle, as_size_t Start, const std::string& Current)
		{
			return (int)Current.find_first_not_of(Needle, (size_t)Start);
		}
		int STDString::FindLastNotOf(const std::string& Needle, as_size_t Start, const std::string& Current)
		{
			return (int)Current.find_last_not_of(Needle, (size_t)Start);
		}
		int STDString::FindLast(const std::string& Needle, int Start, const std::string& Current)
		{
			return (int)Current.rfind(Needle, (size_t)Start);
		}
		void STDString::Insert(unsigned int Offset, const std::string& Other, std::string& Current)
		{
			Current.insert(Offset, Other);
		}
		void STDString::Erase(unsigned int Offset, int Count, std::string& Current)
		{
			Current.erase(Offset, (size_t)(Count < 0 ? std::string::npos : Count));
		}
		as_size_t STDString::Length(const std::string& Current)
		{
			return (as_size_t)Current.length();
		}
		void STDString::Resize(as_size_t Size, std::string& Current)
		{
			Current.resize(Size);
		}
		std::string STDString::Replace(const std::string& A, const std::string& B, uint64_t Offset, const std::string& Base)
		{
			return Tomahawk::Core::Parser(Base).Replace(A, B, Offset).R();
		}
		as_int64_t STDString::IntStore(const std::string& Value, as_size_t Base, as_size_t* ByteCount)
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
		as_uint64_t STDString::UIntStore(const std::string& Value, as_size_t Base, as_size_t* ByteCount)
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
		double STDString::FloatStore(const std::string& Value, as_size_t* ByteCount)
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
		std::string STDString::Sub(as_size_t Start, int Count, const std::string& Current)
		{
			std::string Result;
			if (Start < Current.length() && Count != 0)
				Result = Current.substr(Start, (size_t)(Count < 0 ? std::string::npos : Count));

			return Result;
		}
		bool STDString::Equals(const std::string& Left, const std::string& Right)
		{
			return Left == Right;
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
		STDArray* STDString::Split(const std::string& Splitter, const std::string& Current)
		{
			VMCContext* Context = asGetActiveContext();
			VMCManager* Engine = Context->GetEngine();
			asITypeInfo* ArrayType = Engine->GetTypeInfoByDecl("Array<String>@");
			STDArray* Array = STDArray::Create(ArrayType);

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
		std::string STDString::Join(const STDArray& Array, const std::string& Splitter)
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

		float STDMath::FpFromIEEE(as_size_t Raw)
		{
			return *reinterpret_cast<float*>(&Raw);
		}
		as_size_t STDMath::FpToIEEE(float Value)
		{
			return *reinterpret_cast<as_size_t*>(&Value);
		}
		double STDMath::FpFromIEEE(as_uint64_t Raw)
		{
			return *reinterpret_cast<double*>(&Raw);
		}
		as_uint64_t STDMath::FpToIEEE(double Value)
		{
			return *reinterpret_cast<as_uint64_t*>(&Value);
		}
		bool STDMath::CloseTo(float A, float B, float Epsilon)
		{
			if (A == B)
				return true;

			float diff = fabsf(A - B);
			if ((A == 0 || B == 0) && (diff < Epsilon))
				return true;

			return diff / (fabs(A) + fabs(B)) < Epsilon;
		}
		bool STDMath::CloseTo(double A, double B, double Epsilon)
		{
			if (A == B)
				return true;

			double diff = fabs(A - B);
			if ((A == 0 || B == 0) && (diff < Epsilon))
				return true;

			return diff / (fabs(A) + fabs(B)) < Epsilon;
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

		STDAny::STDAny(VMCManager* _Engine)
		{
			Engine = _Engine;
			RefCount = 1;
			GCFlag = false;
			Value.TypeId = 0;
			Value.ValueInt = 0;

			Engine->NotifyGarbageCollectorOfNewObject(this, Engine->GetTypeInfoByName("Any"));
		}
		STDAny::STDAny(void* Ref, int RefTypeId, VMCManager* _Engine)
		{
			Engine = _Engine;
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
		STDAny::~STDAny()
		{
			FreeObject();
		}
		STDAny& STDAny::operator=(const STDAny& Other)
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
		STDArray::STDArray(VMCTypeInfo* Info, void* BufferPtr)
		{
			TH_ASSERT_V(Info && std::string(Info->GetName()) == "Array", "array type is invalid");
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
		STDArray::STDArray(as_size_t length, VMCTypeInfo* Info)
		{
			TH_ASSERT_V(Info && std::string(Info->GetName()) == "Array", "array type is invalid");
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
		STDArray::STDArray(as_size_t Length, void* DefaultValue, VMCTypeInfo* Info)
		{
			TH_ASSERT_V(Info && std::string(Info->GetName()) == "Array", "array type is invalid");
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
		void STDArray::SetValue(as_size_t Index, void* Value)
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
		void STDArray::Resize(as_size_t NumElements)
		{
			if (!CheckMaxSize(NumElements))
				return;

			Resize((int)NumElements - (int)Buffer->NumElements, (as_size_t)-1);
		}
		void STDArray::RemoveRange(as_size_t Start, as_size_t Count)
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
		void STDArray::Resize(int Delta, as_size_t Where)
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
		bool STDArray::CheckMaxSize(as_size_t NumElements)
		{
			as_size_t MaxSize = 0xFFFFFFFFul - sizeof(SBuffer) + 1;
			if (ElementSize > 0)
				MaxSize /= (as_size_t)ElementSize;

			if (NumElements > MaxSize)
			{
				VMCContext* Context = asGetActiveContext();
				if (Context)
					Context->SetException("Too large Array Size");

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
		void STDArray::InsertAt(as_size_t Index, void* Value)
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
		void STDArray::InsertAt(as_size_t Index, const STDArray& Array)
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
					Context->SetException("Mismatching Array types");
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
		void STDArray::InsertLast(void* Value)
		{
			InsertAt(Buffer->NumElements, Value);
		}
		void STDArray::RemoveAt(as_size_t Index)
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
		void STDArray::RemoveLast()
		{
			RemoveAt(Buffer->NumElements - 1);
		}
		const void* STDArray::At(as_size_t Index) const
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
		void* STDArray::At(as_size_t Index)
		{
			return const_cast<void*>(const_cast<const STDArray*>(this)->At(Index));
		}
		void* STDArray::GetBuffer()
		{
			return Buffer->Data;
		}
		void STDArray::CreateBuffer(SBuffer** BufferPtr, as_size_t NumElements)
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
		void STDArray::DeleteBuffer(SBuffer* BufferPtr)
		{
			Destruct(BufferPtr, 0, BufferPtr->NumElements);
			asFreeMem(BufferPtr);
		}
		void STDArray::Construct(SBuffer* BufferPtr, as_size_t Start, as_size_t End)
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
		void STDArray::Destruct(SBuffer* BufferPtr, as_size_t Start, as_size_t End)
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
		bool STDArray::Less(const void* A, const void* B, bool Asc, VMCContext* Context, SCache* Cache)
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
		void STDArray::Reverse()
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
		bool STDArray::operator==(const STDArray& Other) const
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
		bool STDArray::Equals(const void* A, const void* B, VMCContext* Context, SCache* Cache) const
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
		int STDArray::FindByRef(void* RefPtr) const
		{
			return FindByRef(0, RefPtr);
		}
		int STDArray::FindByRef(as_size_t StartAt, void* RefPtr) const
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
		int STDArray::Find(void* Value) const
		{
			return Find(0, Value);
		}
		int STDArray::Find(as_size_t StartAt, void* Value) const
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
#if defined(_MSC_VER) && _MSC_VER >= 1500 && !defined(__S3E__)
							sprintf_s(Swap, 512, "Type '%s' has multiple matching opEquals or opCmp methods", SubType->GetName());
#else
							sprintf(Swap, "Type '%s' has multiple matching opEquals or opCmp methods", SubType->GetName());
#endif
						else
#if defined(_MSC_VER) && _MSC_VER >= 1500 && !defined(__S3E__)
							sprintf_s(Swap, 512, "Type '%s' does not have a matching opEquals or opCmp method", SubType->GetName());
#else
							sprintf(Swap, "Type '%s' does not have a matching opEquals or opCmp method", SubType->GetName());
#endif
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
		void STDArray::Copy(void* Dest, void* Src)
		{
			memcpy(Dest, Src, ElementSize);
		}
		void* STDArray::GetArrayItemPointer(int Index)
		{
			return Buffer->Data + Index * ElementSize;
		}
		void* STDArray::GetDataPointer(void* BufferPtr)
		{
			if ((SubTypeId & asTYPEID_MASK_OBJECT) && !(SubTypeId & asTYPEID_OBJHANDLE))
				return reinterpret_cast<void*>(*(size_t*)BufferPtr);
			else
				return BufferPtr;
		}
		void STDArray::SortAsc()
		{
			Sort(0, GetSize(), true);
		}
		void STDArray::SortAsc(as_size_t StartAt, as_size_t Count)
		{
			Sort(StartAt, Count, true);
		}
		void STDArray::SortDesc()
		{
			Sort(0, GetSize(), false);
		}
		void STDArray::SortDesc(as_size_t StartAt, as_size_t Count)
		{
			Sort(StartAt, Count, false);
		}
		void STDArray::Sort(as_size_t StartAt, as_size_t Count, bool Asc)
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
#if defined(_MSC_VER) && _MSC_VER >= 1500 && !defined(__S3E__)
							sprintf_s(Swap, 512, "Type '%s' has multiple matching opCmp methods", SubType->GetName());
#else
							sprintf(Swap, "Type '%s' has multiple matching opCmp methods", SubType->GetName());
#endif
						else
#if defined(_MSC_VER) && _MSC_VER >= 1500 && !defined(__S3E__)
							sprintf_s(Swap, 512, "Type '%s' does not have a matching opCmp method", SubType->GetName());
#else
							sprintf(Swap, "Type '%s' does not have a matching opCmp method", SubType->GetName());
#endif
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
		void STDArray::Sort(asIScriptFunction* Function, as_size_t StartAt, as_size_t Count)
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
		void STDArray::CopyBuffer(SBuffer* Dest, SBuffer* Src)
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
		void STDArray::Precache()
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
		void STDArray::EnumReferences(VMCManager* Engine)
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
		STDArray* STDArray::Create(VMCTypeInfo* Info, as_size_t Length)
		{
			void* Memory = asAllocMem(sizeof(STDArray));
			if (Memory == 0)
			{
				VMCContext* Context = asGetActiveContext();
				if (Context)
					Context->SetException("Out of memory");

				return 0;
			}

			STDArray* a = new(Memory) STDArray(Length, Info);
			return a;
		}
		STDArray* STDArray::Create(VMCTypeInfo* Info, void* InitList)
		{
			void* Memory = asAllocMem(sizeof(STDArray));
			if (Memory == 0)
			{
				VMCContext* Context = asGetActiveContext();
				if (Context)
					Context->SetException("Out of memory");

				return 0;
			}

			STDArray* a = new(Memory) STDArray(Info, InitList);
			return a;
		}
		STDArray* STDArray::Create(VMCTypeInfo* Info, as_size_t length, void* DefaultValue)
		{
			void* Memory = asAllocMem(sizeof(STDArray));
			if (Memory == 0)
			{
				VMCContext* Context = asGetActiveContext();
				if (Context)
					Context->SetException("Out of memory");

				return 0;
			}

			STDArray* a = new(Memory) STDArray(length, DefaultValue, Info);
			return a;
		}
		STDArray* STDArray::Create(VMCTypeInfo* Info)
		{
			return STDArray::Create(Info, as_size_t(0));
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
		bool STDArray::TemplateCallback(VMCTypeInfo* Info, bool& DontGarbageCollect)
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
						Engine->WriteMessage("Array", 0, 0, asMSGTYPE_ERROR, "The subtype has no default constructor");
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

		STDIterator::STDIterator()
		{
			ValueObj = 0;
			TypeId = 0;
		}
		STDIterator::STDIterator(VMCManager* Engine, void* Value, int _TypeId)
		{
			ValueObj = 0;
			TypeId = 0;
			Set(Engine, Value, _TypeId);
		}
		STDIterator::~STDIterator()
		{
			if (ValueObj && TypeId)
			{
				VMCContext* Context = asGetActiveContext();
				if (Context != nullptr)
					FreeValue(Context->GetEngine());
			}
		}
		void STDIterator::FreeValue(VMCManager* Engine)
		{
			if (TypeId & asTYPEID_MASK_OBJECT)
			{
				Engine->ReleaseScriptObject(ValueObj, Engine->GetTypeInfoById(TypeId));
				ValueObj = 0;
				TypeId = 0;
			}
		}
		void STDIterator::EnumReferences(VMCManager* _Engine)
		{
			if (ValueObj)
				_Engine->GCEnumCallback(ValueObj);

			if (TypeId)
				_Engine->GCEnumCallback(_Engine->GetTypeInfoById(TypeId));
		}
		void STDIterator::Set(VMCManager* Engine, void* Value, int _TypeId)
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
		void STDIterator::Set(VMCManager* Engine, STDIterator& Value)
		{
			if (Value.TypeId & asTYPEID_OBJHANDLE)
				Set(Engine, (void*)&Value.ValueObj, Value.TypeId);
			else if (Value.TypeId & asTYPEID_MASK_OBJECT)
				Set(Engine, (void*)Value.ValueObj, Value.TypeId);
			else
				Set(Engine, (void*)&Value.ValueInt, Value.TypeId);
		}
		bool STDIterator::Get(VMCManager* Engine, void* Value, int _TypeId) const
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
		const void* STDIterator::GetAddressOfValue() const
		{
			if ((TypeId & asTYPEID_MASK_OBJECT) && !(TypeId & asTYPEID_OBJHANDLE))
				return ValueObj;

			return reinterpret_cast<const void*>(&ValueObj);
		}
		int STDIterator::GetTypeId() const
		{
			return TypeId;
		}

		STDMap::Iterator::Iterator(const STDMap& Dict, Map::const_iterator It) : It(It), Dict(Dict)
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
		bool STDMap::Iterator::GetValue(void* Value, int TypeId) const
		{
			return It->second.Get(Dict.Engine, Value, TypeId);
		}
		const void* STDMap::Iterator::GetAddressOfValue() const
		{
			return It->second.GetAddressOfValue();
		}

		STDMap::STDMap(VMCManager* _Engine)
		{
			Init(_Engine);
		}
		STDMap::STDMap(unsigned char* buffer)
		{
			VMCContext* Context = asGetActiveContext();
			Init(Context->GetEngine());

			STDMap::SCache& Cache = *reinterpret_cast<STDMap::SCache*>(Engine->GetUserData(MAP_CACHE));
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
				{
					TH_WARN("[memerr] use nullptr instead of null for initialization lists");
					buffer += sizeof(void*);
				}
				else
					buffer += Engine->GetSizeOfPrimitiveType(TypeId);
			}
		}
		STDMap::STDMap(const STDMap& Other)
		{
			Init(Other.Engine);

			Map::const_iterator It;
			for (It = Other.Dict.begin(); It != Other.Dict.end(); ++It)
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
		void STDMap::EnumReferences(VMCManager* _Engine)
		{
			Map::iterator It;
			for (It = Dict.begin(); It != Dict.end(); ++It)
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
		void STDMap::ReleaseAllReferences(VMCManager*)
		{
			DeleteAll();
		}
		STDMap& STDMap::operator =(const STDMap& Other)
		{
			DeleteAll();

			Map::const_iterator It;
			for (It = Other.Dict.begin(); It != Other.Dict.end(); ++It)
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
		STDIterator* STDMap::operator[](const std::string& Key)
		{
			return &Dict[Key];
		}
		const STDIterator* STDMap::operator[](const std::string& Key) const
		{
			Map::const_iterator It;
			It = Dict.find(Key);
			if (It != Dict.end())
				return &It->second;

			VMCContext* Context = asGetActiveContext();
			if (Context)
				Context->SetException("Invalid access to non-existing Value");

			return 0;
		}
		void STDMap::Set(const std::string& Key, void* Value, int TypeId)
		{
			Map::iterator It;
			It = Dict.find(Key);
			if (It == Dict.end())
				It = Dict.insert(Map::value_type(Key, STDIterator())).first;

			It->second.Set(Engine, Value, TypeId);
		}
		bool STDMap::Get(const std::string& Key, void* Value, int TypeId) const
		{
			Map::const_iterator It;
			It = Dict.find(Key);
			if (It != Dict.end())
				return It->second.Get(Engine, Value, TypeId);

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
		int STDMap::GetTypeId(const std::string& Key) const
		{
			Map::const_iterator It;
			It = Dict.find(Key);
			if (It != Dict.end())
				return It->second.TypeId;

			return -1;
		}
		bool STDMap::Exists(const std::string& Key) const
		{
			Map::const_iterator It;
			It = Dict.find(Key);
			if (It != Dict.end())
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
		bool STDMap::Delete(const std::string& Key)
		{
			Map::iterator It;
			It = Dict.find(Key);
			if (It != Dict.end())
			{
				It->second.FreeValue(Engine);
				Dict.erase(It);
				return true;
			}

			return false;
		}
		void STDMap::DeleteAll()
		{
			Map::iterator It;
			for (It = Dict.begin(); It != Dict.end(); ++It)
				It->second.FreeValue(Engine);

			Dict.clear();
		}
		STDArray* STDMap::GetKeys() const
		{
			STDMap::SCache* Cache = reinterpret_cast<STDMap::SCache*>(Engine->GetUserData(MAP_CACHE));
			VMCTypeInfo* Info = Cache->ArrayType;

			STDArray* Array = STDArray::Create(Info, as_size_t(Dict.size()));
			Map::const_iterator It;
			long Current = -1;

			for (It = Dict.begin(); It != Dict.end(); ++It)
			{
				Current++;
				*(std::string*)Array->At(Current) = It->first;
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
		STDMap::Iterator STDMap::Find(const std::string& Key) const
		{
			return Iterator(*this, Dict.find(Key));
		}
		STDMap* STDMap::Create(VMCManager* Engine)
		{
			STDMap* Obj = (STDMap*)asAllocMem(sizeof(STDMap));
			new(Obj) STDMap(Engine);
			return Obj;
		}
		STDMap* STDMap::Create(unsigned char* buffer)
		{
			STDMap* Obj = (STDMap*)asAllocMem(sizeof(STDMap));
			new(Obj) STDMap(buffer);
			return Obj;
		}
		void STDMap::Init(VMCManager* e)
		{
			RefCount = 1;
			GCFlag = false;
			Engine = e;

			STDMap::SCache* Cache = reinterpret_cast<STDMap::SCache*>(Engine->GetUserData(MAP_CACHE));
			Engine->NotifyGarbageCollectorOfNewObject(this, Cache->DictType);
		}
		void STDMap::Cleanup(VMCManager* Engine)
		{
			STDMap::SCache* Cache = reinterpret_cast<STDMap::SCache*>(Engine->GetUserData(MAP_CACHE));
			TH_DELETE(SCache, Cache);
		}
		void STDMap::Setup(VMCManager* Engine)
		{
			STDMap::SCache* Cache = reinterpret_cast<STDMap::SCache*>(Engine->GetUserData(MAP_CACHE));
			if (Cache == 0)
			{
				Cache = TH_NEW(STDMap::SCache);
				Engine->SetUserData(Cache, MAP_CACHE);
				Engine->SetEngineUserDataCleanupCallback(Cleanup, MAP_CACHE);

				Cache->DictType = Engine->GetTypeInfoByName("Map");
				Cache->ArrayType = Engine->GetTypeInfoByDecl("Array<String>");
				Cache->KeyType = Engine->GetTypeInfoByDecl("String");
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
		void STDMap::KeyConstruct(void* Memory)
		{
			new(Memory) STDIterator();
		}
		void STDMap::KeyDestruct(STDIterator* Obj)
		{
			VMCContext* Context = asGetActiveContext();
			if (Context)
			{
				VMCManager* Engine = Context->GetEngine();
				Obj->FreeValue(Engine);
			}
			Obj->~STDIterator();
		}
		STDIterator& STDMap::KeyopAssign(void* RefPtr, int TypeId, STDIterator* Obj)
		{
			VMCContext* Context = asGetActiveContext();
			if (Context)
			{
				VMCManager* Engine = Context->GetEngine();
				Obj->Set(Engine, RefPtr, TypeId);
			}
			return *Obj;
		}
		STDIterator& STDMap::KeyopAssign(const STDIterator& Other, STDIterator* Obj)
		{
			VMCContext* Context = asGetActiveContext();
			if (Context)
			{
				VMCManager* Engine = Context->GetEngine();
				Obj->Set(Engine, const_cast<STDIterator&>(Other));
			}

			return *Obj;
		}
		STDIterator& STDMap::KeyopAssign(double Value, STDIterator* Obj)
		{
			return KeyopAssign(&Value, asTYPEID_DOUBLE, Obj);
		}
		STDIterator& STDMap::KeyopAssign(as_int64_t Value, STDIterator* Obj)
		{
			return STDMap::KeyopAssign(&Value, asTYPEID_INT64, Obj);
		}
		void STDMap::KeyopCast(void* RefPtr, int TypeId, STDIterator* Obj)
		{
			VMCContext* Context = asGetActiveContext();
			if (Context)
			{
				VMCManager* Engine = Context->GetEngine();
				Obj->Get(Engine, RefPtr, TypeId);
			}
		}
		as_int64_t STDMap::KeyopConvInt(STDIterator* Obj)
		{
			as_int64_t Value;
			KeyopCast(&Value, asTYPEID_INT64, Obj);
			return Value;
		}
		double STDMap::KeyopConvDouble(STDIterator* Obj)
		{
			double Value;
			KeyopCast(&Value, asTYPEID_DOUBLE, Obj);
			return Value;
		}

		STDGrid::STDGrid(VMCTypeInfo* Info, void* BufferPtr)
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
		STDGrid::STDGrid(as_size_t Width, as_size_t Height, VMCTypeInfo* Info)
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
		void STDGrid::Resize(as_size_t Width, as_size_t Height)
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
		STDGrid::STDGrid(as_size_t Width, as_size_t Height, void* DefaultValue, VMCTypeInfo* Info)
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
		void STDGrid::SetValue(as_size_t x, as_size_t y, void* Value)
		{
			SetValue(Buffer, x, y, Value);
		}
		void STDGrid::SetValue(SBuffer* BufferPtr, as_size_t x, as_size_t y, void* Value)
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
		void* STDGrid::At(SBuffer* BufferPtr, as_size_t x, as_size_t y)
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
		const void* STDGrid::At(as_size_t x, as_size_t y) const
		{
			return const_cast<STDGrid*>(this)->At(const_cast<SBuffer*>(Buffer), x, y);
		}
		void STDGrid::CreateBuffer(SBuffer** BufferPtr, as_size_t W, as_size_t H)
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
		void STDGrid::DeleteBuffer(SBuffer* BufferPtr)
		{
			TH_ASSERT_V(BufferPtr, "buffer should be set");
			Destruct(BufferPtr);
			asFreeMem(BufferPtr);
		}
		void STDGrid::Construct(SBuffer* BufferPtr)
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
		void STDGrid::Destruct(SBuffer* BufferPtr)
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
		void STDGrid::EnumReferences(VMCManager* Engine)
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
		STDGrid* STDGrid::Create(VMCTypeInfo* Info)
		{
			return STDGrid::Create(Info, 0, 0);
		}
		STDGrid* STDGrid::Create(VMCTypeInfo* Info, as_size_t W, as_size_t H)
		{
			void* Memory = asAllocMem(sizeof(STDGrid));
			if (Memory == 0)
			{
				VMCContext* Context = asGetActiveContext();
				if (Context)
					Context->SetException("Out of memory");

				return 0;
			}

			STDGrid* a = new(Memory) STDGrid(W, H, Info);
			return a;
		}
		STDGrid* STDGrid::Create(VMCTypeInfo* Info, void* InitList)
		{
			void* Memory = asAllocMem(sizeof(STDGrid));
			if (Memory == 0)
			{
				VMCContext* Context = asGetActiveContext();
				if (Context)
					Context->SetException("Out of memory");

				return 0;
			}

			STDGrid* a = new(Memory) STDGrid(Info, InitList);
			return a;
		}
		STDGrid* STDGrid::Create(VMCTypeInfo* Info, as_size_t W, as_size_t H, void* DefaultValue)
		{
			void* Memory = asAllocMem(sizeof(STDGrid));
			if (Memory == 0)
			{
				VMCContext* Context = asGetActiveContext();
				if (Context)
					Context->SetException("Out of memory");

				return 0;
			}

			STDGrid* a = new(Memory) STDGrid(W, H, DefaultValue, Info);
			return a;
		}
		bool STDGrid::TemplateCallback(VMCTypeInfo* Info, bool& DontGarbageCollect)
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
						Engine->WriteMessage("Array", 0, 0, asMSGTYPE_ERROR, "The subtype has no default constructor");
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
		STDRef::STDRef(void* RefPtr, VMCTypeInfo* _Type)
		{
			Ref = RefPtr;
			Type = _Type;

			AddRefHandle();
		}
		STDRef::STDRef(void* RefPtr, int TypeId)
		{
			Ref = 0;
			Type = 0;

			Assign(RefPtr, TypeId);
		}
		STDRef::~STDRef()
		{
			ReleaseHandle();
		}
		void STDRef::ReleaseHandle()
		{
			if (Ref && Type)
			{
				VMCManager* Engine = Type->GetEngine();
				Engine->ReleaseScriptObject(Ref, Type);
				Engine->Release();

				Ref = 0;
				Type = 0;
			}
		}
		void STDRef::AddRefHandle()
		{
			if (Ref && Type)
			{
				VMCManager* Engine = Type->GetEngine();
				Engine->AddRefScriptObject(Ref, Type);
				Engine->AddRef();
			}
		}
		STDRef& STDRef::operator =(const STDRef& Other)
		{
			Set(Other.Ref, Other.Type);
			return *this;
		}
		void STDRef::Set(void* RefPtr, VMCTypeInfo* _Type)
		{
			if (Ref == RefPtr)
				return;

			ReleaseHandle();
			Ref = RefPtr;
			Type = _Type;
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
		STDRef& STDRef::Assign(void* RefPtr, int TypeId)
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

			if (Info && strcmp(Info->GetName(), "RefPtr") == 0)
			{
				STDRef* Result = (STDRef*)RefPtr;
				RefPtr = Result->Ref;
				Info = Result->Type;
			}

			Set(RefPtr, Info);
			return *this;
		}
		bool STDRef::operator==(const STDRef& Other) const
		{
			if (Ref == Other.Ref && Type == Other.Type)
				return true;

			return false;
		}
		bool STDRef::operator!=(const STDRef& Other) const
		{
			return !(*this == Other);
		}
		bool STDRef::Equals(void* RefPtr, int TypeId) const
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

			if (RefPtr == Ref)
				return true;

			return false;
		}
		void STDRef::Cast(void** OutRef, int TypeId)
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

			Engine->RefCastObject(Ref, Type, Info, OutRef);
		}
		void STDRef::EnumReferences(VMCManager* _Engine)
		{
			if (Ref)
				_Engine->GCEnumCallback(Ref);

			if (Type)
				_Engine->GCEnumCallback(Type);
		}
		void STDRef::ReleaseReferences(VMCManager* _Engine)
		{
			Set(0, 0);
		}
		void STDRef::Construct(STDRef* Base)
		{
			new(Base) STDRef();
		}
		void STDRef::Construct(STDRef* Base, const STDRef& Other)
		{
			new(Base) STDRef(Other);
		}
		void STDRef::Construct(STDRef* Base, void* RefPtr, int TypeId)
		{
			new(Base) STDRef(RefPtr, TypeId);
		}
		void STDRef::Destruct(STDRef* Base)
		{
			Base->~STDRef();
		}

		STDWeakRef::STDWeakRef(VMCTypeInfo* _Type)
		{
			Ref = 0;
			Type = _Type;
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
		STDWeakRef::STDWeakRef(void* RefPtr, VMCTypeInfo* _Type)
		{
			if (!_Type || !(strcmp(_Type->GetName(), "WeakRef") == 0 || strcmp(_Type->GetName(), "ConstWeakRef") == 0))
				return;

			Ref = RefPtr;
			Type = _Type;
			Type->AddRef();

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
			if (Ref == Other.Ref && WeakRefFlag == Other.WeakRefFlag)
				return *this;

			if (Type != Other.Type)
			{
				if (!(strcmp(Type->GetName(), "ConstWeakRef") == 0 && strcmp(Other.Type->GetName(), "WeakRef") == 0 && Type->GetSubType() == Other.Type->GetSubType()))
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
		bool STDWeakRef::operator==(const STDWeakRef& Other) const
		{
			if (Ref == Other.Ref &&
				WeakRefFlag == Other.WeakRefFlag &&
				Type == Other.Type)
				return true;

			return false;
		}
		bool STDWeakRef::operator!=(const STDWeakRef& Other) const
		{
			return !(*this == Other);
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
		bool STDWeakRef::Equals(void* RefPtr) const
		{
			if (Ref != RefPtr)
				return false;

			asILockableSharedBool* flag = Type->GetEngine()->GetWeakRefFlagOfScriptObject(RefPtr, Type->GetSubType());
			if (WeakRefFlag != flag)
				return false;

			return true;
		}
		void STDWeakRef::Construct(VMCTypeInfo* Type, void* Memory)
		{
			new(Memory) STDWeakRef(Type);
		}
		void STDWeakRef::Construct2(VMCTypeInfo* Type, void* RefPtr, void* Memory)
		{
			new(Memory) STDWeakRef(RefPtr, Type);
			VMCContext* Context = asGetActiveContext();
			if (Context && Context->GetState() == asEXECUTION_EXCEPTION)
				reinterpret_cast<STDWeakRef*>(Memory)->~STDWeakRef();
		}
		void STDWeakRef::Destruct(STDWeakRef* Obj)
		{
			Obj->~STDWeakRef();
		}
		bool STDWeakRef::TemplateCallback(VMCTypeInfo* Info, bool&)
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

			Info->GetEngine()->WriteMessage("WeakRef", 0, 0, asMSGTYPE_ERROR, "The subtype doesn't support weak references");
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
		STDComplex::STDComplex(float _R, float _I)
		{
			R = _R;
			I = _I;
		}
		bool STDComplex::operator==(const STDComplex& Other) const
		{
			return (R == Other.R) && (I == Other.I);
		}
		bool STDComplex::operator!=(const STDComplex& Other) const
		{
			return !(*this == Other);
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
			float Sq = Other.SquaredLength();
			if (Sq == 0)
				return STDComplex(0, 0);

			return STDComplex((R * Other.R + I * Other.I) / Sq, (I * Other.R - R * Other.I) / Sq);
		}
		STDComplex STDComplex::GetRI() const
		{
			return *this;
		}
		STDComplex STDComplex::GetIR() const
		{
			return STDComplex(R, I);
		}
		void STDComplex::SetRI(const STDComplex& Other)
		{
			*this = Other;
		}
		void STDComplex::SetIR(const STDComplex& Other)
		{
			R = Other.I;
			I = Other.R;
		}
		void STDComplex::DefaultConstructor(STDComplex* Base)
		{
			new(Base) STDComplex();
		}
		void STDComplex::CopyConstructor(const STDComplex& Other, STDComplex* Base)
		{
			new(Base) STDComplex(Other);
		}
		void STDComplex::ConvConstructor(float Result, STDComplex* Base)
		{
			new(Base) STDComplex(Result);
		}
		void STDComplex::InitConstructor(float Result, float _I, STDComplex* Base)
		{
			new(Base) STDComplex(Result, _I);
		}
		void STDComplex::ListConstructor(float* InitList, STDComplex* Base)
		{
			new(Base) STDComplex(InitList[0], InitList[1]);
		}

		std::string STDRandom::Getb(uint64_t Size)
		{
			return Compute::Common::HexEncode(Compute::Common::RandomBytes(Size)).substr(0, Size);
		}
		double STDRandom::Betweend(double Min, double Max)
		{
			return Compute::Mathd::Random(Min, Max);
		}
		double STDRandom::Magd()
		{
			return Compute::Mathd::RandomMag();
		}
		double STDRandom::Getd()
		{
			return Compute::Mathd::Random();
		}
		float STDRandom::Betweenf(float Min, float Max)
		{
			return Compute::Mathf::Random(Min, Max);
		}
		float STDRandom::Magf()
		{
			return Compute::Mathf::RandomMag();
		}
		float STDRandom::Getf()
		{
			return Compute::Mathf::Random();
		}
		uint64_t STDRandom::Betweeni(uint64_t Min, uint64_t Max)
		{
			return Compute::Math<uint64_t>::Random(Min, Max);
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
				Manager->GetEngine()->WriteMessage("", 0, 0, asMSGTYPE_ERROR, "failed to start a thread: no available context");
				Release();

				return Mutex.unlock();
			}

			Mutex.unlock();
			Context->TryExecuteAsync(Function, [this](Script::VMContext* Context)
			{
				Context->SetArgObject(0, this);
				Context->SetUserData(this, ContextUD);
			}, nullptr).Await([this](int&&)
			{
				Context->SetUserData(nullptr, ContextUD);
				Mutex.lock();

				if (!Context->IsSuspended())
					TH_CLEAR(Context);

				CV.notify_all();
				Mutex.unlock();
				Release();
			});
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
				{
					TH_TRACE("join thread %s", Core::OS::Process::GetThreadId(Thread.get_id()).c_str());
					Thread.join();
				}

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
			if (std::this_thread::get_id() == Thread.get_id())
				return -1;

			Mutex.lock();
			if (!Thread.joinable())
			{
				Mutex.unlock();
				return -1;
			}
			Mutex.unlock();

			std::unique_lock<std::mutex> Guard(Mutex);
			if (CV.wait_for(Guard, std::chrono::milliseconds(Timeout), [&]
			{
				return !((Context && Context->GetState() != VMExecState::SUSPENDED));
			}))
			{
				TH_TRACE("join thread %s", Core::OS::Process::GetThreadId(Thread.get_id()).c_str());
				Thread.join();
				return 1;
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
		void STDThread::Push(void* _Ref, int TypeId)
		{
			auto* _Thread = GetThread();
			int Id = (_Thread == this ? 1 : 0);

			void* Data = asAllocMem(sizeof(STDAny));
			STDAny* Any = new(Data) STDAny(_Ref, TypeId, VMManager::Get()->GetEngine());
			Pipe[Id].Mutex.lock();
			Pipe[Id].Queue.push_back(Any);
			Pipe[Id].Mutex.unlock();
			Pipe[Id].CV.notify_one();
		}
		bool STDThread::Pop(void* _Ref, int TypeId)
		{
			bool Resolved = false;
			while (!Resolved)
				Resolved = Pop(_Ref, TypeId, 1000);

			return true;
		}
		bool STDThread::Pop(void* _Ref, int TypeId, uint64_t Timeout)
		{
			auto* _Thread = GetThread();
			int Id = (_Thread == this ? 0 : 1);

			std::unique_lock<std::mutex> Guard(Pipe[Id].Mutex);
			if (!CV.wait_for(Guard, std::chrono::milliseconds(Timeout), [&]
			{
				return Pipe[Id].Queue.size() != 0;
			}))
				return false;

			STDAny* Result = Pipe[Id].Queue.front();
			if (!Result->Retrieve(_Ref, TypeId))
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

				TH_TRACE("join thread %s", Core::OS::Process::GetThreadId(Thread.get_id()).c_str());
				Thread.join();
			}

			AddRef();
			Thread = std::thread(&STDThread::Routine, this);
			TH_TRACE("spawn thread %s", Core::OS::Process::GetThreadId(Thread.get_id()).c_str());
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

		STDPromise::STDPromise(VMCContext* _Base) : Context(VMContext::Get(_Base)), Future(nullptr), Ref(2), Flag(false)
		{
			if (!Context)
				return;

			Engine = Context->GetManager()->GetEngine();
			Engine->NotifyGarbageCollectorOfNewObject(this, Engine->GetTypeInfoByName("Promise"));
			Context->PromiseAwake(this);
			Context->AddRef();
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
		int STDPromise::Set(void* _Ref, int TypeId)
		{
			if (Future != nullptr)
				return -1;

			AddRef();
			Context->AddRef();
			if (Future != nullptr)
				Future->Release();
			Future = new(asAllocMem(sizeof(STDAny))) STDAny(_Ref, TypeId, Engine);
			Release();

			if (TypeId & asTYPEID_OBJHANDLE)
				Engine->ReleaseScriptObject(*(void**)_Ref, Engine->GetTypeInfoById(TypeId));

			return Core::Schedule::Get()->SetTask([this]()
			{
				Context->PromiseResume(this);
				Context->Release();
				this->Release();
			}) ? 0 : -1;
		}
		int STDPromise::Set(void* _Ref, const char* TypeName)
		{
			return Set(_Ref, Engine->GetTypeIdByDecl(TypeName));
		}
		bool STDPromise::To(void* _Ref, int TypeId)
		{
			if (!Future)
				return false;

			return Future->Retrieve(_Ref, TypeId);
		}
		void* STDPromise::GetHandle()
		{
			if (!Future)
				return nullptr;

			int TypeId = Future->GetTypeId();
			if (TypeId & asTYPEID_OBJHANDLE)
				return Future->Value.ValueObj;

			Context->SetException("object cannot be safely retrieved by reference type promise");
			return nullptr;
		}
		void* STDPromise::GetValue()
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
			if (Context != nullptr)
				Context->PromiseSuspend(Value);

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
			Engine->RegisterObjectBehaviour("Array<T>", asBEHAVE_FACTORY, "Array<T>@ f(int&in, uint length, const T &in Value)", asFUNCTIONPR(STDArray::Create, (VMCTypeInfo*, as_size_t, void*), STDArray*), asCALL_CDECL);
			Engine->RegisterObjectBehaviour("Array<T>", asBEHAVE_LIST_FACTORY, "Array<T>@ f(int&in Type, int&in InitList) {repeat T}", asFUNCTIONPR(STDArray::Create, (VMCTypeInfo*, void*), STDArray*), asCALL_CDECL);
			Engine->RegisterObjectBehaviour("Array<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(STDArray, AddRef), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("Array<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(STDArray, Release), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "T &opIndex(uint Index)", asMETHODPR(STDArray, At, (as_size_t), void*), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "const T &opIndex(uint Index) const", asMETHODPR(STDArray, At, (as_size_t) const, const void*), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "Array<T> &opAssign(const Array<T>&in)", asMETHOD(STDArray, operator=), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void InsertAt(uint Index, const T&in Value)", asMETHODPR(STDArray, InsertAt, (as_size_t, void*), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void InsertAt(uint Index, const Array<T>& Array)", asMETHODPR(STDArray, InsertAt, (as_size_t, const STDArray&), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void InsertLast(const T&in Value)", asMETHOD(STDArray, InsertLast), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void Push(const T&in Value)", asMETHOD(STDArray, InsertLast), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void Pop()", asMETHOD(STDArray, RemoveLast), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void RemoveAt(uint Index)", asMETHOD(STDArray, RemoveAt), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void RemoveLast()", asMETHOD(STDArray, RemoveLast), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void RemoveRange(uint Start, uint Count)", asMETHOD(STDArray, RemoveRange), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "uint Size() const", asMETHOD(STDArray, GetSize), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void Reserve(uint length)", asMETHOD(STDArray, Reserve), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void Resize(uint length)", asMETHODPR(STDArray, Resize, (as_size_t), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void SortAsc()", asMETHODPR(STDArray, SortAsc, (), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void SortAsc(uint StartAt, uint Count)", asMETHODPR(STDArray, SortAsc, (as_size_t, as_size_t), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void SortDesc()", asMETHODPR(STDArray, SortDesc, (), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void SortDesc(uint StartAt, uint Count)", asMETHODPR(STDArray, SortDesc, (as_size_t, as_size_t), void), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "void Reverse()", asMETHOD(STDArray, Reverse), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "int Find(const T&in if_handle_then_const Value) const", asMETHODPR(STDArray, Find, (void*) const, int), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "int Find(uint StartAt, const T&in if_handle_then_const Value) const", asMETHODPR(STDArray, Find, (as_size_t, void*) const, int), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "int FindByRef(const T&in if_handle_then_const Value) const", asMETHODPR(STDArray, FindByRef, (void*) const, int), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "int FindByRef(uint StartAt, const T&in if_handle_then_const Value) const", asMETHODPR(STDArray, FindByRef, (as_size_t, void*) const, int), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "bool opEquals(const Array<T>&in) const", asMETHOD(STDArray, operator==), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Array<T>", "bool IsEmpty() const", asMETHOD(STDArray, IsEmpty), asCALL_THISCALL);
			Engine->RegisterFuncdef("bool Array<T>::Less(const T&in if_handle_then_const a, const T&in if_handle_then_const b)");
			Engine->RegisterObjectMethod("Array<T>", "void Sort(const Less &in, uint StartAt = 0, uint Count = uint(-1))", asMETHODPR(STDArray, Sort, (asIScriptFunction*, as_size_t, as_size_t), void), asCALL_THISCALL);
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
			Engine->RegisterObjectType("MapKey", sizeof(STDIterator), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_GC | asGetTypeTraits<STDIterator>());
			Engine->RegisterObjectBehaviour("MapKey", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(STDMap::KeyConstruct), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectBehaviour("MapKey", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(STDMap::KeyDestruct), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectBehaviour("MapKey", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(STDIterator, EnumReferences), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("MapKey", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(STDIterator, FreeValue), asCALL_THISCALL);
			Engine->RegisterObjectMethod("MapKey", "MapKey &opAssign(const MapKey &in)", asFUNCTIONPR(STDMap::KeyopAssign, (const STDIterator&, STDIterator*), STDIterator&), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("MapKey", "MapKey &opHndlAssign(const ?&in)", asFUNCTIONPR(STDMap::KeyopAssign, (void*, int, STDIterator*), STDIterator&), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("MapKey", "MapKey &opHndlAssign(const MapKey &in)", asFUNCTIONPR(STDMap::KeyopAssign, (const STDIterator&, STDIterator*), STDIterator&), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("MapKey", "MapKey &opAssign(const ?&in)", asFUNCTIONPR(STDMap::KeyopAssign, (void*, int, STDIterator*), STDIterator&), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("MapKey", "MapKey &opAssign(double)", asFUNCTIONPR(STDMap::KeyopAssign, (double, STDIterator*), STDIterator&), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("MapKey", "MapKey &opAssign(int64)", asFUNCTIONPR(STDMap::KeyopAssign, (as_int64_t, STDIterator*), STDIterator&), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("MapKey", "void opCast(?&out)", asFUNCTIONPR(STDMap::KeyopCast, (void*, int, STDIterator*), void), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("MapKey", "void opConv(?&out)", asFUNCTIONPR(STDMap::KeyopCast, (void*, int, STDIterator*), void), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("MapKey", "int64 opConv()", asFUNCTIONPR(STDMap::KeyopConvInt, (STDIterator*), as_int64_t), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("MapKey", "double opConv()", asFUNCTIONPR(STDMap::KeyopConvDouble, (STDIterator*), double), asCALL_CDECL_OBJLAST);

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
			Engine->RegisterObjectMethod("Map", "MapKey &opIndex(const String &in)", asMETHODPR(STDMap, operator[], (const std::string&), STDIterator*), asCALL_THISCALL);
			Engine->RegisterObjectMethod("Map", "const MapKey &opIndex(const String &in) const", asMETHODPR(STDMap, operator[], (const std::string&) const, const STDIterator*), asCALL_THISCALL);
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
			Engine->RegisterObjectBehaviour("Grid<T>", asBEHAVE_LIST_FACTORY, "Grid<T>@ f(int&in Type, int&in InitList) {repeat {repeat_same T}}", asFUNCTIONPR(STDGrid::Create, (VMCTypeInfo*, void*), STDGrid*), asCALL_CDECL);
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
			Engine->RegisterObjectMethod("String", "String Needle(uint Start = 0, int Count = -1) const", asFUNCTION(STDString::Sub), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "int FindFirst(const String &in, uint Start = 0) const", asFUNCTION(STDString::FindFirst), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "int FindFirstOf(const String &in, uint Start = 0) const", asFUNCTION(STDString::FindFirstOf), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "int FindFirstNotOf(const String &in, uint Start = 0) const", asFUNCTION(STDString::FindFirstNotOf), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "int FindLast(const String &in, int Start = -1) const", asFUNCTION(STDString::FindLast), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "int FindLastOf(const String &in, int Start = -1) const", asFUNCTION(STDString::FindLastOf), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "int FindLastNotOf(const String &in, int Start = -1) const", asFUNCTION(STDString::FindLastNotOf), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "void Insert(uint Offset, const String &in Other)", asFUNCTION(STDString::Insert), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "void Erase(uint Offset, int Count = -1)", asFUNCTION(STDString::Erase), asCALL_CDECL_OBJLAST);
			Engine->RegisterObjectMethod("String", "String Replace(const String &in, const String &in, uint64 Other = 0)", asFUNCTION(STDString::Replace), asCALL_CDECL_OBJLAST);
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
			VMGlobal& Register = Manager->Global();

			VMRefClass VRandom = Register.SetClassUnmanaged<STDRandom>("Random");
			VRandom.SetMethodStatic("String Getb(uint64)", &STDRandom::Getb);
			VRandom.SetMethodStatic("double Betweend(double, double)", &STDRandom::Betweend);
			VRandom.SetMethodStatic("double Magd()", &STDRandom::Magd);
			VRandom.SetMethodStatic("double Getd()", &STDRandom::Getd);
			VRandom.SetMethodStatic("float Betweenf(float, float)", &STDRandom::Betweenf);
			VRandom.SetMethodStatic("float Magf()", &STDRandom::Magf);
			VRandom.SetMethodStatic("float Getf()", &STDRandom::Getf);
			VRandom.SetMethodStatic("uint64 Betweeni(uint64, uint64)", &STDRandom::Betweeni);

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
			Engine->RegisterObjectMethod("Promise<T>", "T& Get()", asMETHOD(STDPromise, GetValue), asCALL_THISCALL);

			Engine->RegisterObjectType("RefPromise<class T>", 0, asOBJ_REF | asOBJ_GC | asOBJ_TEMPLATE);
			Engine->RegisterObjectBehaviour("RefPromise<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(STDArray::TemplateCallback), asCALL_CDECL);
			Engine->RegisterObjectBehaviour("RefPromise<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(STDPromise, AddRef), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("RefPromise<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(STDPromise, Release), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("RefPromise<T>", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(STDPromise, SetGCFlag), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("RefPromise<T>", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(STDPromise, GetGCFlag), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("RefPromise<T>", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(STDPromise, GetRefCount), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("RefPromise<T>", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(STDPromise, EnumReferences), asCALL_THISCALL);
			Engine->RegisterObjectBehaviour("RefPromise<T>", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(STDPromise, ReleaseReferences), asCALL_THISCALL);
			Engine->RegisterObjectMethod("RefPromise<T>", "T@+ Get()", asMETHOD(STDPromise, GetHandle), asCALL_THISCALL);

			return true;
		}
		bool STDFreeCore()
		{
			StringFactory::Free();
			delete Names;
			Names = nullptr;
			return false;
		}
	}
}
