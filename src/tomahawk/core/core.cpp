#include "core.h"
#include "../network/http.h"
#include <cctype>
#include <ctime>
#include <thread>
#include <functional>
#include <iostream>
#include <csignal>
#include <sys/stat.h>
#include <rapidxml.hpp>
#include <json/document.h>
#include <tinyfiledialogs.h>
#ifdef TH_MICROSOFT
#include <Windows.h>
#include <io.h>
#elif defined TH_UNIX
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifndef TH_APPLE
#include <sys/sendfile.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#endif
#include <stdio.h>
#include <fcntl.h>
#include <dlfcn.h>
#endif
#ifdef TH_HAS_SDL2
#include <SDL2/SDL.h>
#endif
#ifdef TH_HAS_ZLIB
extern "C"
{
#include <zlib.h>
}
#endif
#define MAKEUQUAD(L, H) ((uint64_t)(((uint32_t)(L)) | ((uint64_t)((uint32_t)(H))) << 32))
#define RATE_DIFF (10000000)
#define EPOCH_DIFF (MAKEUQUAD(0xd53e8000, 0x019db1de))
#define SYS2UNIX_TIME(L, H) ((int64_t)((MAKEUQUAD((L), (H)) - EPOCH_DIFF) / RATE_DIFF))
#define LEAP_YEAR(X) (((X) % 4 == 0) && (((X) % 100) != 0 || ((X) % 400) == 0))
#ifdef TH_MICROSOFT
namespace
{
	BOOL WINAPI ConsoleEventHandler(DWORD Event)
	{
		switch (Event)
		{
			case CTRL_C_EVENT:
			case CTRL_BREAK_EVENT:
				raise(SIGINT);
				break;
			case CTRL_CLOSE_EVENT:
			case CTRL_SHUTDOWN_EVENT:
			case CTRL_LOGOFF_EVENT:
				raise(SIGTERM);
				break;
			default:
				break;
		}

		return TRUE;
	}
	int UnicodeCompare(const wchar_t* Value1, const wchar_t* Value2)
	{
		int Difference;
		do
		{
			Difference = tolower(*Value1) - tolower(*Value2);
			Value1++;
			Value2++;
		} while (Difference == 0 && Value1[-1] != '\0');

		return Difference;
	}
	void UnicodePath(const char* Path, wchar_t* Input, size_t InputSize)
	{
		char Buffer1[1024];
		strncpy(Buffer1, Path, sizeof(Buffer1));

		for (int i = 0; Buffer1[i] != '\0'; i++)
		{
			if (Buffer1[i] == '/')
				Buffer1[i] = '\\';

			if (Buffer1[i] == '\\' && i > 0)
			{
				while (Buffer1[i + 1] == '\\' || Buffer1[i + 1] == '/')
					memmove(Buffer1 + i + 1, Buffer1 + i + 2, strlen(Buffer1 + i + 1));
			}
		}

		memset(Input, 0, InputSize * sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, Buffer1, -1, Input, (int)InputSize);

		char Buffer2[1024];
		WideCharToMultiByte(CP_UTF8, 0, Input, (int)InputSize, Buffer2, sizeof(Buffer2), nullptr, nullptr);
		if (strcmp(Buffer1, Buffer2) != 0)
			Input[0] = L'\0';

		wchar_t WBuffer[1024 + 1];
		memset(WBuffer, 0, (sizeof(WBuffer) / sizeof(WBuffer[0])) * sizeof(wchar_t));

		DWORD Length = GetLongPathNameW(Input, WBuffer, (sizeof(WBuffer) / sizeof(WBuffer[0])) - 1);
		if (Length == 0)
		{
			if (GetLastError() == ERROR_FILE_NOT_FOUND)
				return;
		}

		if (Length >= (sizeof(WBuffer) / sizeof(WBuffer[0])) || UnicodeCompare(Input, WBuffer) != 0)
			Input[0] = L'\0';
	}
	bool LocalTime(time_t const* const A, struct tm* const B)
	{
		return localtime_s(B, A) == 0;
	}
}
#else
namespace
{
	bool LocalTime(time_t const* const A, struct tm* const B)
	{
		return localtime_r(A, B) != nullptr;
	}
}
#endif

namespace Tomahawk
{
	namespace Core
	{
		Variant::Variant() : Type(VarType_Undefined), Data(nullptr)
		{
		}
		Variant::Variant(VarType NewType, char* NewData) : Type(NewType), Data(NewData)
		{
		}
		Variant::Variant(const Variant& Other)
		{
			Copy(Other);
		}
		Variant::Variant(Variant&& Other) : Type(Other.Type), Data(Other.Data)
		{
			Other.Data = nullptr;
			Other.Type = VarType_Undefined;
		}
		Variant::~Variant()
		{
			Free();
		}
		bool Variant::Deserialize(const std::string& Value, bool Strict)
		{
			Free();
			if (!Strict)
			{
				if (Value == TH_PREFIX_STR "null")
				{
					Type = VarType_Null;
					return true;
				}

				if (Value == TH_PREFIX_STR "undefined")
				{
					Type = VarType_Undefined;
					return true;
				}

				if (Value == TH_PREFIX_STR "object")
				{
					Type = VarType_Object;
					return true;
				}

				if (Value == TH_PREFIX_STR "array")
				{
					Type = VarType_Array;
					return true;
				}

				if (Value == TH_PREFIX_STR "pointer")
				{
					Type = VarType_Pointer;
					return true;
				}

				if (Value == "true")
				{
					Copy(std::move(Var::Boolean(true)));
					return true;
				}

				if (Value == "false")
				{
					Copy(std::move(Var::Boolean(false)));
					return true;
				}

				Parser Buffer(&Value);
				if (Buffer.HasNumber())
				{
					if (Buffer.HasDecimal())
						Copy(std::move(Var::Decimal(Buffer.R())));
					else if (Buffer.HasInteger())
						Copy(std::move(Var::Integer(Buffer.ToInt64())));
					else
						Copy(std::move(Var::Number(Buffer.ToDouble())));

					return true;
				}
			}

			if (Value.size() > 2 && Value.front() == TH_PREFIX_CHAR && Value.back() == TH_PREFIX_CHAR)
			{
				Copy(std::move(Var::Base64(Compute::Common::Base64Decode(std::string(Value.substr(1).c_str(), Value.size() - 2)))));
				return true;
			}

			Copy(std::move(Var::String(Value)));
			return true;
		}
		std::string Variant::Serialize() const
		{
			switch (Type)
			{
				case VarType_Null:
					return TH_PREFIX_STR "null";
				case VarType_Undefined:
					return TH_PREFIX_STR "undefined";
				case VarType_Object:
					return TH_PREFIX_STR "object";
				case VarType_Array:
					return TH_PREFIX_STR "array";
				case VarType_Pointer:
					return TH_PREFIX_STR "pointer";
				case VarType_String:
				case VarType_Decimal:
					return std::string(GetString(), GetSize());
				case VarType_Base64:
					return TH_PREFIX_STR + Compute::Common::Base64Encode(GetBase64(), GetSize()) + TH_PREFIX_STR;
				case VarType_Integer:
					return std::to_string(GetInteger());
				case VarType_Number:
					return std::to_string(GetNumber());
				case VarType_Boolean:
					return GetBoolean() ? "true" : "false";
				default:
					return "";
			}
		}
		std::string Variant::GetDecimal() const
		{
			if (Type != VarType_Decimal)
				return "0.0";

			return std::string(((String*)Data)->Buffer, ((String*)Data)->Size);
		}
		std::string Variant::GetBlob() const
		{
			if (Type != VarType_String && Type != VarType_Base64)
				return "";

			return std::string(((String*)Data)->Buffer, ((String*)Data)->Size);
		}
		void* Variant::GetPointer() const
		{
			if (Type == VarType_Pointer)
				return (void*)Data;

			return nullptr;
		}
		const char* Variant::GetString() const
		{
			if (Type != VarType_String && Type != VarType_Base64)
				return nullptr;

			return (const char*)((String*)Data)->Buffer;
		}
		unsigned char* Variant::GetBase64() const
		{
			if (Type != VarType_String && Type != VarType_Base64)
				return nullptr;

			return (unsigned char*)((String*)Data)->Buffer;
		}
		int64_t Variant::GetInteger() const
		{
			if (Type == VarType_Integer)
				return *(int64_t*)Data;

			if (Type == VarType_Number)
				return (int64_t)*(double*)Data;

			return 0;
		}
		double Variant::GetNumber() const
		{
			if (Type == VarType_Integer)
				return (double)*(int64_t*)Data;

			if (Type == VarType_Number)
				return *(double*)Data;

			return 0.0;
		}
		bool Variant::GetBoolean() const
		{
			if (Type == VarType_Boolean)
				return (Data != nullptr);

			return false;
		}
		VarType Variant::GetType() const
		{
			return Type;
		}
		size_t Variant::GetSize() const
		{
			switch (Type)
			{
				case VarType_Null:
				case VarType_Undefined:
				case VarType_Object:
				case VarType_Array:
				case VarType_Boolean:
					return 0;
				case VarType_Pointer:
					return sizeof(char*);
				case VarType_String:
				case VarType_Base64:
				case VarType_Decimal:
					return ((String*)Data)->Size;
				case VarType_Integer:
					return sizeof(int64_t);
				case VarType_Number:
					return sizeof(double);
			}

			return 0;
		}
		bool Variant::operator== (const Variant& Other) const
		{
			return Is(Other);
		}
		bool Variant::operator!= (const Variant& Other) const
		{
			return !Is(Other);
		}
		Variant& Variant::operator= (const Variant& Other)
		{
			Free();
			Copy(Other);

			return *this;
		}
		Variant& Variant::operator= (Variant&& Other)
		{
			Free();
			Copy(std::move(Other));

			return *this;
		}
		Variant::operator bool() const
		{
			switch (Type)
			{
				case VarType_Object:
				case VarType_Array:
					return true;
				case VarType_String:
				case VarType_Base64:
				case VarType_Decimal:
					return GetSize() > 0;
				case VarType_Integer:
					return GetInteger() > 0;
				case VarType_Number:
					return GetNumber() > 0.0;
				case VarType_Boolean:
					return GetBoolean();
			}

			return Data != nullptr;
		}
		bool Variant::IsObject() const
		{
			return Type == VarType_Object || Type == VarType_Array;
		}
		bool Variant::IsEmpty() const
		{
			return Data == nullptr;
		}
		bool Variant::Is(const Variant& Value) const
		{
			if (Type != Value.Type)
				return false;

			switch (Type)
			{
				case VarType_Null:
				case VarType_Undefined:
					return true;
				case VarType_Pointer:
					return GetPointer() == Value.GetPointer();
				case VarType_String:
				case VarType_Base64:
				case VarType_Decimal:
					if (GetSize() != Value.GetSize())
						return false;

					return strncmp(GetString(), Value.GetString(), sizeof(char) * GetSize()) == 0;
				case VarType_Integer:
					return GetInteger() == Value.GetInteger();
				case VarType_Number:
					return GetNumber() == Value.GetNumber();
				case VarType_Boolean:
					return GetBoolean() == Value.GetBoolean();
			}

			return false;
		}
		void Variant::Copy(const Variant& Other)
		{
			Type = Other.Type;
			switch (Type)
			{
				case VarType_Pointer:
				case VarType_Boolean:
					Data = (char*)Other.Data;
					break;
				case VarType_String:
				case VarType_Base64:
				case VarType_Decimal:
				{
					String* From = (String*)Other.Data;
					String* Buffer = (String*)TH_MALLOC(sizeof(String));
					Buffer->Buffer = (char*)TH_MALLOC(sizeof(char) * (From->Size + 1));
					Buffer->Size = From->Size;

					memcpy(Buffer->Buffer, From->Buffer, sizeof(char) * From->Size);
					Buffer->Buffer[Buffer->Size] = '\0';
					Data = (char*)Buffer;
					break;
				}
				case VarType_Integer:
					Data = (char*)TH_MALLOC(sizeof(int64_t));
					memcpy(Data, Other.Data, sizeof(int64_t));
					break;
				case VarType_Number:
					Data = (char*)TH_MALLOC(sizeof(double));
					memcpy(Data, Other.Data, sizeof(double));
					break;
				default:
					Data = nullptr;
					break;
			}
		}
		void Variant::Copy(Variant&& Other)
		{
			Type = Other.Type;
			Other.Type = VarType_Undefined;
			Data = Other.Data;
			Other.Data = nullptr;
		}
		void Variant::Free()
		{
			if (!Data)
				return;

			if (Type == VarType_String || Type == VarType_Base64)
			{
				String* Buffer = (String*)Data;
				TH_FREE(Buffer->Buffer);
				TH_FREE(Data);
			}
			else if (Type != VarType_Undefined && Type != VarType_Null && Type != VarType_Pointer && Type != VarType_Boolean)
				TH_FREE(Data);

			Data = nullptr;
		}

		EventBase::EventBase(const std::string& NewName) : Name(NewName)
		{
		}
		EventBase::EventBase(const std::string& NewName, const VariantArgs& NewArgs) : Name(NewName), Args(NewArgs)
		{
		}
		EventBase::EventBase(const std::string& NewName, VariantArgs&& NewArgs) : Name(NewName), Args(std::move(NewArgs))
		{
		}
		EventBase::EventBase(const EventBase& Other) : Name(Other.Name), Args(Other.Args)
		{
		}
		EventBase::EventBase(EventBase&& Other) : Name(std::move(Other.Name)), Args(std::move(Other.Args))
		{
		}
		EventBase& EventBase::operator= (const EventBase& Other)
		{
			Name = Other.Name;
			Args = Other.Args;
			return *this;
		}
		EventBase& EventBase::operator= (EventBase&& Other)
		{
			Name = std::move(Other.Name);
			Args = std::move(Other.Args);
			return *this;
		}

		EventTimer::EventTimer(const TimerCallback& NewCallback, uint64_t NewTimeout, int64_t NewTime, EventId NewId, bool NewAlive) : Callback(NewCallback), Timeout(NewTimeout), Time(NewTime), Id(NewId), Alive(NewAlive)
		{
		}
		EventTimer::EventTimer(TimerCallback&& NewCallback, uint64_t NewTimeout, int64_t NewTime, EventId NewId, bool NewAlive) : Callback(std::move(NewCallback)), Timeout(NewTimeout), Time(NewTime), Id(NewId), Alive(NewAlive)
		{
		}
		EventTimer::EventTimer(const EventTimer& Other) : Callback(Other.Callback), Timeout(Other.Timeout), Time(Other.Time), Id(Other.Id), Alive(Other.Alive)
		{
		}
		EventTimer::EventTimer(EventTimer&& Other) : Callback(std::move(Other.Callback)), Timeout(Other.Timeout), Time(Other.Time), Id(Other.Id), Alive(Other.Alive)
		{
		}
		EventTimer& EventTimer::operator= (const EventTimer& Other)
		{
			Callback = Other.Callback;
			Timeout = Other.Timeout;
			Time = Other.Time;
			Id = Other.Id;
			Alive = Other.Alive;
			return *this;
		}
		EventTimer& EventTimer::operator= (EventTimer&& Other)
		{
			Callback = std::move(Other.Callback);
			Timeout = Other.Timeout;
			Time = Other.Time;
			Id = Other.Id;
			Alive = Other.Alive;
			return *this;
		}

		DateTime::DateTime() : Time(std::chrono::system_clock::now().time_since_epoch())
		{
		}
		DateTime::DateTime(const DateTime& Value) : Time(Value.Time)
		{
		}
		void DateTime::Rebuild()
		{
			if (!DateRebuild)
				return;

			Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(mktime(&DateValue)));
			DateRebuild = false;
		}
		void DateTime::operator +=(const DateTime& Right)
		{
			Time += Right.Time;
		}
		void DateTime::operator -=(const DateTime& Right)
		{
			Time -= Right.Time;
		}
		bool DateTime::operator >=(const DateTime& Right)
		{
			return Time >= Right.Time;
		}
		bool DateTime::operator <=(const DateTime& Right)
		{
			return Time <= Right.Time;
		}
		bool DateTime::operator >(const DateTime& Right)
		{
			return Time > Right.Time;
		}
		bool DateTime::operator <(const DateTime& Right)
		{
			return Time < Right.Time;
		}
		std::string DateTime::Format(const std::string& Value)
		{
			return Parser(Value).Replace("{ns}", std::to_string(Nanoseconds())).Replace("{us}", std::to_string(Microseconds())).Replace("{ms}", std::to_string(Milliseconds())).Replace("{s}", std::to_string(Seconds())).Replace("{m}", std::to_string(Minutes())).Replace("{h}", std::to_string(Hours())).Replace("{D}", std::to_string(Days())).Replace("{W}", std::to_string(Weeks())).Replace("{M}", std::to_string(Months())).Replace("{Y}", std::to_string(Years())).R();
		}
		std::string DateTime::Date(const std::string& Value)
		{
			auto Offset = std::chrono::system_clock::to_time_t(std::chrono::system_clock::time_point(Time));
			if (DateRebuild)
				Rebuild();

			struct tm T;
			if (!LocalTime(&Offset, &T))
				return Value;

			T.tm_mon++;
			T.tm_year += 1900;

			return Parser(Value).Replace("{s}", T.tm_sec < 10 ? Form("0%i", T.tm_sec).R() : std::to_string(T.tm_sec)).Replace("{m}", T.tm_min < 10 ? Form("0%i", T.tm_min).R() : std::to_string(T.tm_min)).Replace("{h}", std::to_string(T.tm_hour)).Replace("{D}", std::to_string(T.tm_yday)).Replace("{MD}", T.tm_mday < 10 ? Form("0%i", T.tm_mday).R() : std::to_string(T.tm_mday)).Replace("{WD}", std::to_string(T.tm_wday + 1)).Replace("{M}", T.tm_mon < 10 ? Form("0%i", T.tm_mon).R() : std::to_string(T.tm_mon)).Replace("{Y}", std::to_string(T.tm_year)).R();
		}
		DateTime DateTime::Now()
		{
			DateTime New;
			New.Time = std::chrono::system_clock::now().time_since_epoch();

			return New;
		}
		DateTime DateTime::FromNanoseconds(uint64_t Value)
		{
			DateTime New;
			New.Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::nanoseconds(Value));

			return New;
		}
		DateTime DateTime::FromMicroseconds(uint64_t Value)
		{
			DateTime New;
			New.Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::microseconds(Value));

			return New;
		}
		DateTime DateTime::FromMilliseconds(uint64_t Value)
		{
			DateTime New;
			New.Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::milliseconds(Value));

			return New;
		}
		DateTime DateTime::FromSeconds(uint64_t Value)
		{
			DateTime New;
			New.Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(Value));

			return New;
		}
		DateTime DateTime::FromMinutes(uint64_t Value)
		{
			DateTime New;
			New.Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::minutes(Value));

			return New;
		}
		DateTime DateTime::FromHours(uint64_t Value)
		{
			DateTime New;
			New.Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::hours(Value));

			return New;
		}
		DateTime DateTime::FromDays(uint64_t Value)
		{
			using _Days = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;

			DateTime New;
			New.Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(_Days(Value));

			return New;
		}
		DateTime DateTime::FromWeeks(uint64_t Value)
		{
			using _Days = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;

			using _Weeks = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<7>, _Days::period>>;

			DateTime New;
			New.Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(_Weeks(Value));

			return New;
		}
		DateTime DateTime::FromMonths(uint64_t Value)
		{
			using _Days = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;

			using _Years = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<146097, 400>, _Days::period>>;

			using _Months = std::chrono::duration<uint64_t, std::ratio_divide<_Years::period, std::ratio<12>>>;

			DateTime New;
			New.Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(_Months(Value));

			return New;
		}
		DateTime DateTime::FromYears(uint64_t Value)
		{
			using _Days = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;

			using _Years = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<146097, 400>, _Days::period>>;

			DateTime New;
			New.Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(_Years(Value));

			return New;
		}
		DateTime DateTime::operator +(const DateTime& Right)
		{
			DateTime New;
			New.Time = Time + Right.Time;

			return New;
		}
		DateTime DateTime::operator -(const DateTime& Right)
		{
			DateTime New;
			New.Time = Time - Right.Time;

			return New;
		}
		DateTime& DateTime::SetDateSeconds(uint64_t Value, bool NoFlush)
		{
			if (!DateRebuild)
			{
				if (!NoFlush)
				{
					time_t TimeNow;
					time(&TimeNow);
					LocalTime(&TimeNow, &DateValue);
				}
				DateRebuild = true;
			}

			if (Value > 60)
				Value = 60;

			DateValue.tm_sec = (int)Value;
			return *this;
		}
		DateTime& DateTime::SetDateMinutes(uint64_t Value, bool NoFlush)
		{
			if (!DateRebuild)
			{
				if (!NoFlush)
				{
					time_t TimeNow;
					time(&TimeNow);
					LocalTime(&TimeNow, &DateValue);
				}
				DateRebuild = true;
			}

			if (Value > 60)
				Value = 60;
			else if (Value < 1)
				Value = 1;

			DateValue.tm_min = (int)Value - 1;
			return *this;
		}
		DateTime& DateTime::SetDateHours(uint64_t Value, bool NoFlush)
		{
			if (!DateRebuild)
			{
				if (!NoFlush)
				{
					time_t TimeNow;
					time(&TimeNow);
					LocalTime(&TimeNow, &DateValue);
				}
				DateRebuild = true;
			}

			if (Value > 24)
				Value = 24;
			else if (Value < 1)
				Value = 1;

			DateValue.tm_hour = (int)Value - 1;
			return *this;
		}
		DateTime& DateTime::SetDateDay(uint64_t Value, bool NoFlush)
		{
			if (!DateRebuild)
			{
				if (!NoFlush)
				{
					time_t TimeNow;
					time(&TimeNow);
					LocalTime(&TimeNow, &DateValue);
				}
				DateRebuild = true;
			}

			if (Value > 31)
				Value = 31;
			else if (Value < 1)
				Value = 1;

			if (DateValue.tm_mday > (int)Value)
				DateValue.tm_yday = DateValue.tm_yday - DateValue.tm_mday + (int)Value;
			else
				DateValue.tm_yday = DateValue.tm_yday - (int)Value + DateValue.tm_mday;

			if (Value <= 7)
				DateValue.tm_wday = (int)Value - 1;
			else if (Value > 7 && Value <= 14)
				DateValue.tm_wday = (int)Value - 8;
			else if (Value > 14 && Value <= 21)
				DateValue.tm_wday = (int)Value - 15;
			else if (Value > 21 && Value <= 31)
				DateValue.tm_wday = (int)Value - 22;

			DateValue.tm_mday = (int)Value;
			return *this;
		}
		DateTime& DateTime::SetDateWeek(uint64_t Value, bool NoFlush)
		{
			if (!DateRebuild)
			{
				if (!NoFlush)
				{
					time_t TimeNow;
					time(&TimeNow);
					LocalTime(&TimeNow, &DateValue);
				}
				DateRebuild = true;
			}

			if (Value > 7)
				Value = 7;
			else if (Value < 1)
				Value = 1;

			DateValue.tm_wday = (int)Value - 1;
			return *this;
		}
		DateTime& DateTime::SetDateMonth(uint64_t Value, bool NoFlush)
		{
			if (!DateRebuild)
			{
				if (!NoFlush)
				{
					time_t TimeNow;
					time(&TimeNow);
					LocalTime(&TimeNow, &DateValue);
				}
				DateRebuild = true;
			}

			if (Value < 1)
				Value = 1;
			else if (Value > 12)
				Value = 12;

			DateValue.tm_mday = (int)Value - 1;
			return *this;
		}
		DateTime& DateTime::SetDateYear(uint64_t Value, bool NoFlush)
		{
			if (!DateRebuild)
			{
				if (!NoFlush)
				{
					time_t TimeNow;
					time(&TimeNow);
					LocalTime(&TimeNow, &DateValue);
				}
				DateRebuild = true;
			}

			if (Value < 1900)
				Value = 1900;

			DateValue.tm_year = (int)Value - 1900;
			return *this;
		}
		uint64_t DateTime::Nanoseconds()
		{
			if (DateRebuild)
				Rebuild();

			return std::chrono::duration_cast<std::chrono::nanoseconds>(Time).count();
		}
		uint64_t DateTime::Microseconds()
		{
			if (DateRebuild)
				Rebuild();

			return std::chrono::duration_cast<std::chrono::microseconds>(Time).count();
		}
		uint64_t DateTime::Milliseconds()
		{
			if (DateRebuild)
				Rebuild();

			return std::chrono::duration_cast<std::chrono::milliseconds>(Time).count();
		}
		uint64_t DateTime::Seconds()
		{
			if (DateRebuild)
				Rebuild();

			return std::chrono::duration_cast<std::chrono::seconds>(Time).count();
		}
		uint64_t DateTime::Minutes()
		{
			if (DateRebuild)
				Rebuild();

			return (uint64_t)std::chrono::duration_cast<std::chrono::minutes>(Time).count();
		}
		uint64_t DateTime::Hours()
		{
			if (DateRebuild)
				Rebuild();

			return (uint64_t)std::chrono::duration_cast<std::chrono::hours>(Time).count();
		}
		uint64_t DateTime::Days()
		{
			if (DateRebuild)
				Rebuild();

			using _Days = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;

			return std::chrono::duration_cast<_Days>(Time).count();
		}
		uint64_t DateTime::Weeks()
		{
			if (DateRebuild)
				Rebuild();

			using _Days = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;

			using _Weeks = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<7>, _Days::period>>;

			return std::chrono::duration_cast<_Weeks>(Time).count();
		}
		uint64_t DateTime::Months()
		{
			if (DateRebuild)
				Rebuild();

			using _Days = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;

			using _Years = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<146097, 400>, _Days::period>>;

			using _Months = std::chrono::duration<uint64_t, std::ratio_divide<_Years::period, std::ratio<12>>>;

			return std::chrono::duration_cast<_Months>(Time).count();
		}
		uint64_t DateTime::Years()
		{
			if (DateRebuild)
				Rebuild();

			using _Days = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;

			using _Years = std::chrono::duration<int, std::ratio_multiply<std::ratio<146097, 400>, _Days::period>>;

			return std::chrono::duration_cast<_Years>(Time).count();
		}
		std::string DateTime::GetGMTBasedString(int64_t TimeStamp)
		{
			auto Time = (time_t)TimeStamp;
			struct tm GTMTimeStamp
			{
			};

#ifdef TH_MICROSOFT
			if (gmtime_s(&GTMTimeStamp, &Time) != 0)
#elif defined(TH_UNIX)
			if (gmtime_r(&Time, &GTMTimeStamp) == nullptr)
#endif
				return "Thu, 01 Jan 1970 00:00:00 GMT";

			char Buffer[64];
			strftime(Buffer, sizeof(Buffer), "%a, %d %b %Y %H:%M:%S GMT", &GTMTimeStamp);
			return Buffer;
		}
		bool DateTime::TimeFormatGMT(char* Buffer, uint64_t Length, int64_t Time)
		{
			if (!Buffer || !Length)
				return false;

			auto TimeStamp = (time_t)Time;
			struct tm Date
			{
			};

#if defined(_WIN32_CE)
			static const int DaysPerMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

			FILETIME FileTime;
			*(int64_t)&FileTime = ((int64_t)*clk) * RATE_DIFF * EPOCH_DIFF;

			SYSTEMTIME SystemTime;
			FileTimeToSystemTime(&FileTime, &SystemTime);

			Date.tm_year = SystemTime.wYear - 1900;
			Date.tm_mon = SystemTime.wMonth - 1;
			Date.tm_wday = SystemTime.wDayOfWeek;
			Date.tm_mday = SystemTime.wDay;
			Date.tm_hour = SystemTime.wHour;
			Date.tm_min = SystemTime.wMinute;
			Date.tm_sec = SystemTime.wSecond;
			Date.tm_isdst = false;

			int Day = Date.tm_mday;
			for (int i = 0; i < Date.tm_mon; i++)
				Day += DaysPerMonth[i];

			if (Date.tm_mon >= 2 && LEAP_YEAR(Date.tm_year + 1900))
				Day++;

			Date.tm_yday = Day;
			strftime(Buffer, Length, "%a, %d %b %Y %H:%M:%S GMT", &Date);
#elif defined(TH_MICROSOFT)
			if (gmtime_s(&Date, &TimeStamp) != 0)
				strncpy(Buffer, "Thu, 01 Jan 1970 00:00:00 GMT", (size_t)Length);
			else
				strftime(Buffer, (size_t)Length, "%a, %d %b %Y %H:%M:%S GMT", &Date);
#else
			if (gmtime_r(&TimeStamp, &Date) == nullptr)
				strncpy(Buffer, "Thu, 01 Jan 1970 00:00:00 GMT", Length);
			else
				strftime(Buffer, Length, "%a, %d %b %Y %H:%M:%S GMT", &Date);
#endif
			return true;
		}
		bool DateTime::TimeFormatLCL(char* Buffer, uint64_t Length, int64_t Time)
		{
			time_t TimeStamp = (time_t)Time;
			struct tm Date
			{
			};

#if defined(_WIN32_WCE)
			static const int DaysPerMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

			FILETIME FileTime, LocalFileTime;
			*(int64_t)&FileTime = ((int64_t)*clk) * RATE_DIFF * EPOCH_DIFF;
			FileTimeToLocalFileTime(&FileTime, &LocalFileTime);

			SYSTEMTIME SystemTime;
			FileTimeToSystemTime(&LocalFileTime, &SystemTime);

			TIME_ZONE_INFORMATION TimeZone;
			Date.tm_year = st.wYear - 1900;
			Date.tm_mon = st.wMonth - 1;
			Date.tm_wday = st.wDayOfWeek;
			Date.tm_mday = st.wDay;
			Date.tm_hour = st.wHour;
			Date.tm_min = st.wMinute;
			Date.tm_sec = st.wSecond;
			Date.tm_isdst = (GetTimeZoneInformation(&TimeZone) == TIME_ZONE_ID_DAYLIGHT) ? 1 : 0;

			int Day = Date.tm_mday;
			for (int i = 0; i < Date.tm_mon; i++)
				Day += DaysPerMonth[i];

			if (Date.tm_mon >= 2 && LEAP_YEAR(Date.tm_year + 1900))
				Day++;

			Date.tm_yday = doy;
			strftime(Buffer, Length, "%d-%b-%Y %H:%M", &Date);
#elif defined(_WIN32)
			if (!LocalTime(&TimeStamp, &Date))
				strncpy(Buffer, "01-Jan-1970 00:00", (size_t)Length);
			else
				strftime(Buffer, (size_t)Length, "%d-%b-%Y %H:%M", &Date);
#else
			if (!LocalTime(&TimeStamp, &Date))
				strncpy(Buffer, "01-Jan-1970 00:00", Length);
			else
				strftime(Buffer, Length, "%d-%b-%Y %H:%M", &Date);
#endif
			return true;
		}
		int64_t DateTime::ReadGMTBasedString(const char* Date)
		{
			static const char* MonthNames[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

			char Name[32] = { 0 };
			int Second, Minute, Hour, Day, Year;
			if (sscanf(Date, "%d/%3s/%d %d:%d:%d", &Day, Name, &Year, &Hour, &Minute, &Second) != 6 && sscanf(Date, "%d %3s %d %d:%d:%d", &Day, Name, &Year, &Hour, &Minute, &Second) != 6 && sscanf(Date, "%*3s, %d %3s %d %d:%d:%d", &Day, Name, &Year, &Hour, &Minute, &Second) != 6 && sscanf(Date, "%d-%3s-%d %d:%d:%d", &Day, Name, &Year, &Hour, &Minute, &Second) != 6)
				return 0;

			if (Year <= 1970)
				return 0;

			for (uint64_t i = 0; i < 12; i++)
			{
				if (strcmp(Name, MonthNames[i]) != 0)
					continue;

				struct tm Time
				{
				};
				Time.tm_year = Year - 1900;
				Time.tm_mon = (int)i;
				Time.tm_mday = Day;
				Time.tm_hour = Hour;
				Time.tm_min = Minute;
				Time.tm_sec = Second;

#ifdef TH_MICROSOFT
				return _mkgmtime(&Time);
#else
				return mktime(&Time);
#endif
			}

			return 0;
		}

		Parser::Parser() : Safe(true)
		{
			L = new std::string();
		}
		Parser::Parser(int Value) : Safe(true)
		{
			L = new std::string(std::to_string(Value));
		}
		Parser::Parser(unsigned int Value) : Safe(true)
		{
			L = new std::string(std::to_string(Value));
		}
		Parser::Parser(int64_t Value) : Safe(true)
		{
			L = new std::string(std::to_string(Value));
		}
		Parser::Parser(uint64_t Value) : Safe(true)
		{
			L = new std::string(std::to_string(Value));
		}
		Parser::Parser(float Value) : Safe(true)
		{
			L = new std::string(std::to_string(Value));
		}
		Parser::Parser(double Value) : Safe(true)
		{
			L = new std::string(std::to_string(Value));
		}
		Parser::Parser(long double Value) : Safe(true)
		{
			L = new std::string(std::to_string(Value));
		}
		Parser::Parser(const std::string& Buffer) : Safe(true)
		{
			L = new std::string(Buffer);
		}
		Parser::Parser(std::string* Buffer)
		{
			Safe = (!Buffer);
			L = (Safe ? new std::string() : Buffer);
		}
		Parser::Parser(const std::string* Buffer)
		{
			Safe = (!Buffer);
			L = (Safe ? new std::string() : (std::string*)Buffer);
		}
		Parser::Parser(const char* Buffer) : Safe(true)
		{
			if (Buffer != nullptr)
				L = new std::string(Buffer);
			else
				L = new std::string();
		}
		Parser::Parser(const char* Buffer, int64_t Length) : Safe(true)
		{
			if (Buffer != nullptr)
				L = new std::string(Buffer, Length);
			else
				L = new std::string();
		}
		Parser::Parser(const Parser& Value) : Safe(true)
		{
			if (Value.L != nullptr)
				L = new std::string(*Value.L);
			else
				L = new std::string();
		}
		Parser::~Parser()
		{
			if (Safe)
				delete L;
		}
		Parser& Parser::EscapePrint()
		{
			for (size_t i = 0; i < L->size(); i++)
			{
				if (L->at(i) != '%')
					continue;

				if (i + 1 < L->size())
				{
					if (L->at(i + 1) != '%')
					{
						L->insert(L->begin() + i, '%');
						i++;
					}
				}
				else
				{
					L->append(1, '%');
					i++;
				}
			}

			return *this;
		}
		Parser& Parser::Escape()
		{
			for (size_t i = 0; i < L->size(); i++)
			{
				char& V = L->at(i);
				if (V == '\n')
					V = 'n';
				else if (V == '\t')
					V = 't';
				else if (V == '\v')
					V = 'v';
				else if (V == '\b')
					V = 'b';
				else if (V == '\r')
					V = 'r';
				else if (V == '\f')
					V = 'f';
				else if (V == '\a')
					V = 'a';
				else
					continue;

				L->insert(L->begin() + i, '\\');
				i++;
			}

			return *this;
		}
		Parser& Parser::Unescape()
		{
			for (size_t i = 0; i < L->size(); i++)
			{
				if (L->at(i) != '\\' || i + 1 >= L->size())
					continue;

				char& V = L->at(i + 1);
				if (V == 'n')
					V = '\n';
				else if (V == 't')
					V = '\t';
				else if (V == 'v')
					V = '\v';
				else if (V == 'b')
					V = '\b';
				else if (V == 'r')
					V = '\r';
				else if (V == 'f')
					V = '\f';
				else if (V == 'a')
					V = '\a';
				else
					continue;

				L->erase(L->begin() + i);
			}

			return *this;
		}
		Parser& Parser::Reserve(uint64_t Count)
		{
			L->reserve(L->capacity() + Count);
			return *this;
		}
		Parser& Parser::Resize(uint64_t Count)
		{
			L->resize(Count);
			return *this;
		}
		Parser& Parser::Resize(uint64_t Count, char Char)
		{
			L->resize(Count, Char);
			return *this;
		}
		Parser& Parser::Clear()
		{
			L->clear();
			return *this;
		}
		Parser& Parser::ToUtf8()
		{
#pragma warning(push)
#pragma warning(disable: 4333)
			std::string Output;
			for (char i : *L)
			{
				auto V = (wchar_t)i;
				if (0 <= V && V <= 0x7f)
				{
					Output += V;
				}
				else if (0x80 <= V && V <= 0x7ff)
				{
					Output += (wchar_t)((0xc0 | (V >> 6)));
					Output += (wchar_t)((0x80 | (V & 0x3f)));
				}
				else if (0x800 <= V && V <= 0xffff)
				{
					Output += (wchar_t)((0xe0 | (V >> 12)));
					Output += (wchar_t)((0x80 | ((V >> 6) & 0x3f)));
					Output += (wchar_t)((0x80 | (V & 0x3f)));
				}
				else if (0x10000 <= V && V <= 0x1fffff)
				{
					Output += (wchar_t)((0xf0 | (V >> 18)));
					Output += (wchar_t)((0x80 | ((V >> 12) & 0x3f)));
					Output += (wchar_t)((0x80 | ((V >> 6) & 0x3f)));
					Output += (wchar_t)((0x80 | (V & 0x3f)));
				}
				else if (0x200000 <= V && V <= 0x3ffffff)
				{
					Output += (wchar_t)((0xf8 | (V >> 24)));
					Output += (wchar_t)((0x80 | ((V >> 18) & 0x3f)));
					Output += (wchar_t)((0x80 | ((V >> 12) & 0x3f)));
					Output += (wchar_t)((0x80 | ((V >> 6) & 0x3f)));
					Output += (wchar_t)((0x80 | (V & 0x3f)));
				}
				else if (0x4000000 <= V && V <= 0x7fffffff)
				{
					Output += (wchar_t)((0xfc | (V >> 30)));
					Output += (wchar_t)((0x80 | ((V >> 24) & 0x3f)));
					Output += (wchar_t)((0x80 | ((V >> 18) & 0x3f)));
					Output += (wchar_t)((0x80 | ((V >> 12) & 0x3f)));
					Output += (wchar_t)(0x80 | ((V >> 6) & 0x3f));
					Output += (wchar_t)((0x80 | (V & 0x3f)));
				}
			}
#pragma warning(pop)

			L->assign(Output);
			return *this;
		}
		Parser& Parser::ToUpper()
		{
			std::transform(L->begin(), L->end(), L->begin(), ::toupper);
			return *this;
		}
		Parser& Parser::ToLower()
		{
			std::transform(L->begin(), L->end(), L->begin(), ::tolower);
			return *this;
		}
		Parser& Parser::Clip(uint64_t Length)
		{
			if (Length < L->size())
				L->erase(Length, L->size() - Length);

			return *this;
		}
		Parser& Parser::ReplaceOf(const char* Chars, const char* To, uint64_t Start)
		{
			if (!Chars || Chars[0] == '\0' || !To)
				return *this;

			Parser::Settle Result{ };
			uint64_t Offset = Start, ToSize = (uint64_t)strlen(To);
			while ((Result = FindOf(Chars, Offset)).Found)
			{
				EraseOffsets(Result.Start, Result.End);
				Insert(To, Result.Start);
				Offset = Result.Start + ToSize;
			}

			return *this;
		}
		Parser& Parser::ReplaceNotOf(const char* Chars, const char* To, uint64_t Start)
		{
			if (!Chars || Chars[0] == '\0' || !To)
				return *this;

			Parser::Settle Result{};
			uint64_t Offset = Start, ToSize = (uint64_t)strlen(To);
			while ((Result = FindNotOf(Chars, Offset)).Found)
			{
				EraseOffsets(Result.Start, Result.End);
				Insert(To, Result.Start);
				Offset = Result.Start + ToSize;
			}

			return *this;
		}
		Parser& Parser::Replace(const std::string& From, const std::string& To, uint64_t Start)
		{
			uint64_t Offset = Start;
			Parser::Settle Result{ };

			while ((Result = Find(From, Offset)).Found)
			{
				EraseOffsets(Result.Start, Result.End);
				Insert(To, Result.Start);
				Offset = Result.Start + To.size();
			}

			return *this;
		}
		Parser& Parser::Replace(const char* From, const char* To, uint64_t Start)
		{
			if (!From || !To)
				return *this;

			uint64_t Offset = Start;
			auto Size = (uint64_t)strlen(To);
			Parser::Settle Result{ };

			while ((Result = Find(From, Offset)).Found)
			{
				EraseOffsets(Result.Start, Result.End);
				Insert(To, Result.Start);
				Offset = Result.Start + Size;
			}

			return *this;
		}
		Parser& Parser::Replace(const char& From, const char& To, uint64_t Position)
		{
			for (uint64_t i = Position; i < L->size(); i++)
			{
				if (L->at(i) == From)
					L->at(i) = To;
			}

			return *this;
		}
		Parser& Parser::Replace(const char& From, const char& To, uint64_t Position, uint64_t Count)
		{
			if (L->size() < (Position + Count))
				return *this;

			for (uint64_t i = Position; i < (Position + Count); i++)
			{
				if (L->at(i) == From)
					L->at(i) = To;
			}

			return *this;
		}
		Parser& Parser::ReplacePart(uint64_t Start, uint64_t End, const std::string& Value)
		{
			return ReplacePart(Start, End, Value.c_str());
		}
		Parser& Parser::ReplacePart(uint64_t Start, uint64_t End, const char* Value)
		{
			if (Start >= L->size() || End > L->size() || Start >= End || !Value)
				return *this;

			if (Start == 0 && L->size() == End)
				L->assign(Value);
			else if (Start == 0)
				L->assign(Value + L->substr(End, L->size() - End));
			else if (L->size() == End)
				L->assign(L->substr(0, Start) + Value);
			else
				L->assign(L->substr(0, Start) + Value + L->substr(End, L->size() - End));

			return *this;
		}
		Parser& Parser::RemovePart(uint64_t Start, uint64_t End)
		{
			if (Start >= L->size() || End > L->size() || Start >= End)
				return *this;

			if (Start == 0 && L->size() == End)
				L->clear();
			else if (Start == 0)
				L->assign(L->substr(End, L->size() - End));
			else if (L->size() == End)
				L->assign(L->substr(0, Start));
			else
				L->assign(L->substr(0, Start) + L->substr(End, L->size() - End));

			return *this;
		}
		Parser& Parser::Reverse()
		{
			return Reverse(0, L->size() - 1);
		}
		Parser& Parser::Reverse(uint64_t Start, uint64_t End)
		{
			if (Start == End || L->size() < 2 || End > (L->size() - 1) || Start > (L->size() - 1))
				return *this;

			std::reverse(L->begin() + Start, L->begin() + End + 1);
			return *this;
		}
		Parser& Parser::Substring(uint64_t Start)
		{
			if (Start >= L->size())
			{
				L->clear();
				return *this;
			}

			L->assign(L->substr(Start));
			return *this;
		}
		Parser& Parser::Substring(uint64_t Start, uint64_t Count)
		{
			if (Start >= L->size() || !Count)
			{
				L->clear();
				return *this;
			}

			L->assign(L->substr(Start, Count));
			return *this;
		}
		Parser& Parser::Substring(const Parser::Settle& Result)
		{
			if (!Result.Found)
			{
				L->clear();
				return *this;
			}

			if (Result.Start > (L->size() - 1))
				return *this;

			auto Offset = (int64_t)Result.End;
			if (Result.End > L->size())
				Offset = (int64_t)(L->size() - Result.Start);

			Offset = (int64_t)Result.Start - Offset;
			L->assign(L->substr(Result.Start, (uint64_t)(Offset < 0 ? -Offset : Offset)));
			return *this;
		}
		Parser& Parser::Splice(uint64_t Start, uint64_t End)
		{
			if (Start > (L->size() - 1))
				return (*this);

			if (End > L->size())
				End = (L->size() - Start);

			int64_t Offset = (int64_t)Start - (int64_t)End;
			L->assign(L->substr(Start, (uint64_t)(Offset < 0 ? -Offset : Offset)));
			return *this;
		}
		Parser& Parser::Trim()
		{
			L->erase(L->begin(), std::find_if(L->begin(), L->end(), [](int C) -> int
			{
				if (C < -1 || C > 255)
					return true;

				return !std::isspace(C);
			}));
			L->erase(std::find_if(L->rbegin(), L->rend(), [](int C) -> int
			{
				if (C < -1 || C > 255)
					return true;

				return !std::isspace(C);
			}).base(), L->end());

			return *this;
		}
		Parser& Parser::Fill(const char& Char)
		{
			if (L->empty())
				return (*this);

			for (char& i : *L)
				i = Char;

			return *this;
		}
		Parser& Parser::Fill(const char& Char, uint64_t Count)
		{
			if (L->empty())
				return (*this);

			L->assign(Count, Char);
			return *this;
		}
		Parser& Parser::Fill(const char& Char, uint64_t Start, uint64_t Count)
		{
			if (L->empty() || Start > L->size())
				return (*this);

			if (Start + Count > L->size())
				Count = L->size() - Start;

			for (uint64_t i = Start; i < (Start + Count); i++)
				L->at(i) = Char;

			return *this;
		}
		Parser& Parser::Assign(const char* Raw)
		{
			if (Raw != nullptr)
				L->assign(Raw);
			else
				L->clear();

			return *this;
		}
		Parser& Parser::Assign(const char* Raw, uint64_t Length)
		{
			if (Raw != nullptr)
				L->assign(Raw, Length);
			else
				L->clear();

			return *this;
		}
		Parser& Parser::Assign(const std::string& Raw)
		{
			L->assign(Raw);
			return *this;
		}
		Parser& Parser::Assign(const std::string& Raw, uint64_t Start, uint64_t Count)
		{
			L->assign(Raw.substr(Start, Count));
			return *this;
		}
		Parser& Parser::Assign(const char* Raw, uint64_t Start, uint64_t Count)
		{
			if (!Raw)
			{
				L->clear();
				return *this;
			}

			L->assign(Raw);
			return Substring(Start, Count);
		}
		Parser& Parser::Append(const char* Raw)
		{
			if (Raw != nullptr)
				L->append(Raw);

			return *this;
		}
		Parser& Parser::Append(const char& Char)
		{
			L->append(1, Char);
			return *this;
		}
		Parser& Parser::Append(const char& Char, uint64_t Count)
		{
			L->append(Count, Char);
			return *this;
		}
		Parser& Parser::Append(const std::string& Raw)
		{
			L->append(Raw);
			return *this;
		}
		Parser& Parser::Append(const char* Raw, uint64_t Count)
		{
			if (Raw != nullptr)
				L->append(Raw, Count);

			return *this;
		}
		Parser& Parser::Append(const char* Raw, uint64_t Start, uint64_t Count)
		{
			if (!Raw)
				return *this;

			std::string V(Raw);
			if (!Count || V.size() < Start + Count)
				return *this;

			L->append(V.substr(Start, Count));
			return *this;
		}
		Parser& Parser::Append(const std::string& Raw, uint64_t Start, uint64_t Count)
		{
			if (!Count || Raw.size() < Start + Count)
				return *this;

			L->append(Raw.substr(Start, Count));
			return *this;
		}
		Parser& Parser::fAppend(const char* Format, ...)
		{
			if (!Format)
				return *this;

			char Buffer[16384];
			va_list Args;
			va_start(Args, Format);
			int Count = vsnprintf(Buffer, sizeof(Buffer), Format, Args);
			va_end(Args);

			return Append(Buffer, Count);
		}
		Parser& Parser::Insert(const std::string& Raw, uint64_t Position)
		{
			if (Position >= L->size())
				Position = L->size();

			L->insert(Position, Raw);
			return *this;
		}
		Parser& Parser::Insert(const std::string& Raw, uint64_t Position, uint64_t Start, uint64_t Count)
		{
			if (Position >= L->size())
				Position = L->size();

			if (Raw.size() >= Start + Count)
				L->insert(Position, Raw.substr(Start, Count));

			return *this;
		}
		Parser& Parser::Insert(const std::string& Raw, uint64_t Position, uint64_t Count)
		{
			if (Position >= L->size())
				Position = L->size();

			if (Count >= Raw.size())
				Count = Raw.size();

			L->insert(Position, Raw.substr(0, Count));
			return *this;
		}
		Parser& Parser::Insert(const char& Char, uint64_t Position, uint64_t Count)
		{
			if (Position >= L->size())
				return *this;

			L->insert(Position, Count, Char);
			return *this;
		}
		Parser& Parser::Insert(const char& Char, uint64_t Position)
		{
			if (Position >= L->size())
				return *this;

			L->insert(L->begin() + Position, Char);
			return *this;
		}
		Parser& Parser::Erase(uint64_t Position)
		{
			if (Position >= L->size())
				return *this;

			L->erase(Position);
			return *this;
		}
		Parser& Parser::Erase(uint64_t Position, uint64_t Count)
		{
			if (Position >= L->size())
				return *this;

			L->erase(Position, Count);
			return *this;
		}
		Parser& Parser::EraseOffsets(uint64_t Start, uint64_t End)
		{
			return Erase(Start, End - Start);
		}
		Parser& Parser::Path(const std::string& Net, const std::string& Dir)
		{
			if (StartsOf("./\\"))
			{
				std::string Result = Core::OS::Path::Resolve(L->c_str(), Dir);
				if (!Result.empty())
					Assign(Result);
			}
			else
				Replace("[subnet]", Net);

			return *this;
		}
		Parser::Settle Parser::ReverseFind(const std::string& Needle, uint64_t Offset) const
		{
			if (L->empty())
				return { L->size() - 1, L->size(), false };

			const char* Ptr = L->c_str() - Offset;
			if (Needle.c_str() > Ptr)
				return { L->size() - 1, L->size(), false };

			const char* It = nullptr;
			for (It = Ptr + L->size() - Needle.size(); It > Ptr; --It)
			{
				if (strncmp(Ptr, Needle.c_str(), (size_t)Needle.size()) == 0)
					return { (uint64_t)(It - Ptr), (uint64_t)(It - Ptr + Needle.size()), true };
			}

			return { L->size() - 1, L->size(), false };
		}
		Parser::Settle Parser::ReverseFind(const char* Needle, uint64_t Offset) const
		{
			if (L->empty())
				return { L->size() - 1, L->size(), false };

			if (!Needle)
				return { L->size() - 1, L->size(), false };

			const char* Ptr = L->c_str() - Offset;
			if (Needle > Ptr)
				return { L->size() - 1, L->size(), false };

			const char* It = nullptr;
			auto Length = (uint64_t)strlen(Needle);
			for (It = Ptr + L->size() - Length; It > Ptr; --It)
			{
				if (strncmp(Ptr, Needle, (size_t)Length) == 0)
					return { (uint64_t)(It - Ptr), (uint64_t)(It - Ptr + Length), true };
			}

			return { L->size() - 1, L->size(), false };
		}
		Parser::Settle Parser::ReverseFind(const char& Needle, uint64_t Offset) const
		{
			if (L->empty())
				return { L->size() - 1, L->size(), false };

			for (uint64_t i = L->size() - 1 - Offset; i >= 0; i--)
			{
				if (L->at(i) == Needle)
					return { i, i + 1, true };

				if (i == 0)
					break;
			}

			return { L->size() - 1, L->size(), false };
		}
		Parser::Settle Parser::ReverseFindUnescaped(const char& Needle, uint64_t Offset) const
		{
			if (L->empty())
				return { L->size() - 1, L->size(), false };

			for (uint64_t i = L->size() - 1 - Offset; i >= 0; i--)
			{
				if (L->at(i) == Needle && ((int64_t)i - 1 < 0 || L->at(i - 1) != '\\'))
					return { i, i + 1, true };

				if (i == 0)
					break;
			}

			return { L->size() - 1, L->size(), false };
		}
		Parser::Settle Parser::ReverseFindOf(const std::string& Needle, uint64_t Offset) const
		{
			if (L->empty())
				return { L->size() - 1, L->size(), false };

			for (uint64_t i = L->size() - 1 - Offset; i >= 0; i--)
			{
				for (char k : Needle)
				{
					if (L->at(i) == k)
						return { i, i + 1, true };
				}

				if (i == 0)
					break;
			}

			return { L->size() - 1, L->size(), false };
		}
		Parser::Settle Parser::ReverseFindOf(const char* Needle, uint64_t Offset) const
		{
			if (L->empty())
				return { L->size() - 1, L->size(), false };

			if (!Needle)
				return { L->size() - 1, L->size(), false };

			uint64_t Length = strlen(Needle);
			for (uint64_t i = L->size() - 1 - Offset; i >= 0; i--)
			{
				for (uint64_t k = 0; k < Length; k++)
				{
					if (L->at(i) == Needle[k])
						return { i, i + 1, true };
				}

				if (i == 0)
					break;
			}

			return { L->size() - 1, L->size(), false };
		}
		Parser::Settle Parser::Find(const std::string& Needle, uint64_t Offset) const
		{
			const char* It = strstr(L->c_str() + Offset, Needle.c_str());
			if (It == nullptr)
				return { L->size() - 1, L->size(), false };

			return { (uint64_t)(It - L->c_str()), (uint64_t)(It - L->c_str() + Needle.size()), true };
		}
		Parser::Settle Parser::Find(const char* Needle, uint64_t Offset) const
		{
			if (!Needle)
				return { L->size() - 1, L->size(), false };

			const char* It = strstr(L->c_str() + Offset, Needle);
			if (It == nullptr)
				return { L->size() - 1, L->size(), false };

			return { (uint64_t)(It - L->c_str()), (uint64_t)(It - L->c_str() + strlen(Needle)), true };
		}
		Parser::Settle Parser::Find(const char& Needle, uint64_t Offset) const
		{
			for (uint64_t i = Offset; i < L->size(); i++)
			{
				if (L->at(i) == Needle)
					return { i, i + 1, true };
			}

			return { L->size() - 1, L->size(), false };
		}
		Parser::Settle Parser::FindUnescaped(const char& Needle, uint64_t Offset) const
		{
			for (uint64_t i = Offset; i < L->size(); i++)
			{
				if (L->at(i) == Needle && ((int64_t)i - 1 < 0 || L->at(i - 1) != '\\'))
					return { i, i + 1, true };
			}

			return { L->size() - 1, L->size(), false };
		}
		Parser::Settle Parser::FindOf(const std::string& Needle, uint64_t Offset) const
		{
			for (uint64_t i = Offset; i < L->size(); i++)
			{
				for (char k : Needle)
				{
					if (L->at(i) == k)
						return { i, i + 1, true };
				}
			}

			return { L->size() - 1, L->size(), false };
		}
		Parser::Settle Parser::FindOf(const char* Needle, uint64_t Offset) const
		{
			if (!Needle)
				return { L->size() - 1, L->size(), false };

			auto Length = (uint64_t)strlen(Needle);
			for (uint64_t i = Offset; i < L->size(); i++)
			{
				for (uint64_t k = 0; k < Length; k++)
				{
					if (L->at(i) == Needle[k])
						return { i, i + 1, true };
				}
			}

			return { L->size() - 1, L->size(), false };
		}
		Parser::Settle Parser::FindNotOf(const std::string& Needle, uint64_t Offset) const
		{
			for (uint64_t i = Offset; i < L->size(); i++)
			{
				bool Result = false;
				for (char k : Needle)
				{
					if (L->at(i) == k)
					{
						Result = true;
						break;
					}
				}

				if (!Result)
					return { i, i + 1, true };
			}

			return { L->size() - 1, L->size(), false };
		}
		Parser::Settle Parser::FindNotOf(const char* Needle, uint64_t Offset) const
		{
			if (!Needle)
				return { L->size() - 1, L->size(), false };

			auto Length = (uint64_t)strlen(Needle);
			for (uint64_t i = Offset; i < L->size(); i++)
			{
				bool Result = false;
				for (uint64_t k = 0; k < Length; k++)
				{
					if (L->at(i) == Needle[k])
					{
						Result = true;
						break;
					}
				}

				if (!Result)
					return { i, i + 1, true };
			}

			return { L->size() - 1, L->size(), false };
		}
		bool Parser::StartsWith(const std::string& Value, uint64_t Offset) const
		{
			if (L->size() < Value.size())
				return false;

			for (uint64_t i = 0; i < Value.size(); i++)
			{
				if (Value[i] != L->at(i + Offset))
					return false;
			}

			return true;
		}
		bool Parser::StartsWith(const char* Value, uint64_t Offset) const
		{
			if (!Value)
				return false;

			auto Length = (uint64_t)strlen(Value);
			if (L->size() < Length)
				return false;

			for (uint64_t i = 0; i < Length; i++)
			{
				if (Value[i] != L->at(i + Offset))
					return false;
			}

			return true;
		}
		bool Parser::StartsOf(const char* Value, uint64_t Offset) const
		{
			if (!Value)
				return false;

			auto Length = (uint64_t)strlen(Value);
			if (Offset >= L->size())
				return false;

			for (uint64_t j = 0; j < Length; j++)
			{
				if (L->at(Offset) == Value[j])
					return true;
			}

			return false;
		}
		bool Parser::StartsNotOf(const char* Value, uint64_t Offset) const
		{
			if (!Value)
				return false;

			auto Length = (uint64_t)strlen(Value);
			if (Offset >= L->size())
				return false;

			bool Result = true;
			for (uint64_t j = 0; j < Length; j++)
			{
				if (L->at(Offset) == Value[j])
				{
					Result = false;
					break;
				}
			}

			return Result;
		}
		bool Parser::EndsWith(const std::string& Value) const
		{
			if (L->empty())
				return false;

			return strcmp(L->c_str() + L->size() - Value.size(), Value.c_str()) == 0;
		}
		bool Parser::EndsWith(const char* Value) const
		{
			if (L->empty() || !Value)
				return false;

			return strcmp(L->c_str() + L->size() - strlen(Value), Value) == 0;
		}
		bool Parser::EndsWith(const char& Value) const
		{
			return !L->empty() && L->back() == Value;
		}
		bool Parser::EndsOf(const char* Value) const
		{
			if (!Value)
				return false;

			auto Length = (uint64_t)strlen(Value);
			for (uint64_t j = 0; j < Length; j++)
			{
				if (L->back() == Value[j])
					return true;
			}

			return false;
		}
		bool Parser::EndsNotOf(const char* Value) const
		{
			if (!Value)
				return false;

			auto Length = (uint64_t)strlen(Value);
			bool Result = true;

			for (uint64_t j = 0; j < Length; j++)
			{
				if (L->back() == Value[j])
				{
					Result = false;
					break;
				}
			}

			return Result;
		}
		bool Parser::Empty() const
		{
			return L->empty();
		}
		bool Parser::HasInteger() const
		{
			if (L->empty())
				return false;

			bool HadSign = false;
			for (char i : *L)
			{
				if (IsDigit(i))
					continue;

				if (i != '-' || HadSign)
					return false;

				HadSign = true;
			}

			return true;
		}
		bool Parser::HasNumber() const
		{
			if (L->empty() || (L->size() == 1 && L->front() == '.'))
				return false;

			bool HadPoint = false, HadSign = false;
			for (char i : *L)
			{
				if (IsDigit(i))
					continue;

				if (i != '.' || HadPoint)
				{
					if (i != '-' || HadSign)
						return false;

					HadSign = true;
				}
				else
					HadPoint = true;
			}

			return true;
		}
		bool Parser::HasDecimal() const
		{
			auto F = Find('.');
			if (F.Found)
			{
				auto D1 = Parser(L->c_str(), F.End);
				if (D1.Empty() || !D1.HasInteger())
					return false;

				auto D2 = Parser(L->c_str() + F.End + 1, L->size() - F.End);
				if (D2.Empty() || !D2.HasInteger())
					return false;

				return D1.Size() > 19 || D2.Size() > 15;
			}

			return HasInteger() && L->size() > 19;
		}
		bool Parser::ToBoolean() const
		{
			return !strncmp(L->c_str(), "true", 4) || !strncmp(L->c_str(), "1", 1);
		}
		bool Parser::IsDigit(char Char)
		{
			return Char == '0' || Char == '1' || Char == '2' || Char == '3' || Char == '4' || Char == '5' || Char == '6' || Char == '7' || Char == '8' || Char == '9';
		}
		int Parser::CaseCompare(const char* Value1, const char* Value2)
		{
			if (!Value1 || !Value2)
				return 0;

			int Result;
			do
			{
				Result = tolower(*(const unsigned char*)(Value1++)) - tolower(*(const unsigned char*)(Value2++));
			} while (Result == 0 && Value1[-1] != '\0');

			return Result;
		}
		int Parser::Match(const char* Pattern, const char* Text)
		{
			return Match(Pattern, strlen(Pattern), Text);
		}
		int Parser::Match(const char* Pattern, uint64_t Length, const char* Text)
		{
			if (!Pattern || !Text)
				return -1;

			const char* Token = (const char*)memchr(Pattern, '|', (size_t)Length);
			if (Token != nullptr)
			{
				int Output = Match(Pattern, (uint64_t)(Token - Pattern), Text);
				return (Output > 0) ? Output : Match(Token + 1, (uint64_t)((Pattern + Length) - (Token + 1)), Text);
			}

			int Offset = 0, Result = 0;
			uint64_t i = 0;
			int j = 0;
			while (i < Length)
			{
				if (Pattern[i] == '?' && Text[j] != '\0')
					continue;

				if (Pattern[i] == '$')
					return (Text[j] == '\0') ? j : -1;

				if (Pattern[i] == '*')
				{
					i++;
					if (Pattern[i] == '*')
					{
						Offset = (int)strlen(Text + j);
						i++;
					}
					else
						Offset = (int)strcspn(Text + j, "/");

					if (i == Length)
						return j + Offset;

					do
					{
						Result = Match(Pattern + i, Length - i, Text + j + Offset);
					} while (Result == -1 && Offset-- > 0);

					return (Result == -1) ? -1 : j + Result + Offset;
				}
				else if (tolower((const unsigned char)Pattern[i]) != tolower((const unsigned char)Text[j]))
					return -1;

				i++;
				j++;
			}

			return j;
		}
		int Parser::ToInt() const
		{
			return (int)strtol(L->c_str(), nullptr, 10);
		}
		long Parser::ToLong() const
		{
			return strtol(L->c_str(), nullptr, 10);
		}
		float Parser::ToFloat() const
		{
			return strtof(L->c_str(), nullptr);
		}
		unsigned int Parser::ToUInt() const
		{
			return (unsigned int)ToULong();
		}
		unsigned long Parser::ToULong() const
		{
			return strtoul(L->c_str(), nullptr, 10);
		}
		int64_t Parser::ToInt64() const
		{
			return strtoll(L->c_str(), nullptr, 10);
		}
		double Parser::ToDouble() const
		{
			return strtod(L->c_str(), nullptr);
		}
		long double Parser::ToLDouble() const
		{
			return strtold(L->c_str(), nullptr);
		}
		uint64_t Parser::ToUInt64() const
		{
			return strtoull(L->c_str(), nullptr, 10);
		}
		uint64_t Parser::Size() const
		{
			return L->size();
		}
		uint64_t Parser::Capacity() const
		{
			return L->capacity();
		}
		char* Parser::Value() const
		{
			return (char*)L->data();
		}
		const char* Parser::Get() const
		{
			return L->c_str();
		}
		std::string& Parser::R()
		{
			return *L;
		}
		std::basic_string<wchar_t> Parser::ToUnicode() const
		{
#pragma warning(push)
#pragma warning(disable: 4333)
			std::basic_string<wchar_t> Output;
			wchar_t W;
			for (uint64_t i = 0; i < L->size();)
			{
				char C = L->at(i);
				if ((C & 0x80) == 0)
				{
					W = C;
					i++;
				}
				else if ((C & 0xE0) == 0xC0)
				{
					W = (C & 0x1F) << 6;
					W |= (L->at(i + 1) & 0x3F);
					i += 2;
				}
				else if ((C & 0xF0) == 0xE0)
				{
					W = (C & 0xF) << 12;
					W |= (L->at(i + 1) & 0x3F) << 6;
					W |= (L->at(i + 2) & 0x3F);
					i += 3;
				}
				else if ((C & 0xF8) == 0xF0)
				{
					W = (C & 0x7) << 18;
					W |= (L->at(i + 1) & 0x3F) << 12;
					W |= (L->at(i + 2) & 0x3F) << 6;
					W |= (L->at(i + 3) & 0x3F);
					i += 4;
				}
				else if ((C & 0xFC) == 0xF8)
				{
					W = (C & 0x3) << 24;
					W |= (C & 0x3F) << 18;
					W |= (C & 0x3F) << 12;
					W |= (C & 0x3F) << 6;
					W |= (C & 0x3F);
					i += 5;
				}
				else if ((C & 0xFE) == 0xFC)
				{
					W = (C & 0x1) << 30;
					W |= (C & 0x3F) << 24;
					W |= (C & 0x3F) << 18;
					W |= (C & 0x3F) << 12;
					W |= (C & 0x3F) << 6;
					W |= (C & 0x3F);
					i += 6;
				}
				else
					W = C;

				Output += W;
			}
#pragma warning(pop)
			return Output;
		}
		std::vector<std::string> Parser::Split(const std::string& With, uint64_t Start) const
		{
			Parser::Settle Result = Find(With, Start);
			uint64_t Offset = Start;

			std::vector<std::string> Output;
			while (Result.Found)
			{
				Output.push_back(L->substr(Offset, Result.Start - Offset));
				Result = Find(With, Offset = Result.End);
			}

			if (Offset < L->size())
				Output.push_back(L->substr(Offset));

			return Output;
		}
		std::vector<std::string> Parser::Split(char With, uint64_t Start) const
		{
			Parser::Settle Result = Find(With, Start);
			uint64_t Offset = Start;

			std::vector<std::string> Output;
			while (Result.Found)
			{
				Output.push_back(L->substr(Offset, Result.Start - Offset));
				Result = Find(With, Offset = Result.End);
			}

			if (Offset < L->size())
				Output.push_back(L->substr(Offset));

			return Output;
		}
		std::vector<std::string> Parser::SplitMax(char With, uint64_t Count, uint64_t Start) const
		{
			Parser::Settle Result = Find(With, Start);
			uint64_t Offset = Start;

			std::vector<std::string> Output;
			while (Result.Found && Output.size() < Count)
			{
				Output.push_back(L->substr(Offset, Result.Start - Offset));
				Result = Find(With, Offset = Result.End);
			}

			if (Offset < L->size() && Output.size() < Count)
				Output.push_back(L->substr(Offset));

			return Output;
		}
		std::vector<std::string> Parser::SplitOf(const char* With, uint64_t Start) const
		{
			Parser::Settle Result = FindOf(With, Start);
			uint64_t Offset = Start;

			std::vector<std::string> Output;
			while (Result.Found)
			{
				Output.push_back(L->substr(Offset, Result.Start - Offset));
				Result = FindOf(With, Offset = Result.End);
			}

			if (Offset < L->size())
				Output.push_back(L->substr(Offset));

			return Output;
		}
		std::vector<std::string> Parser::SplitNotOf(const char* With, uint64_t Start) const
		{
			Parser::Settle Result = FindNotOf(With, Start);
			uint64_t Offset = Start;

			std::vector<std::string> Output;
			while (Result.Found)
			{
				Output.push_back(L->substr(Offset, Result.Start - Offset));
				Result = FindNotOf(With, Offset = Result.End);
			}

			if (Offset < L->size())
				Output.push_back(L->substr(Offset));

			return Output;
		}
		Parser& Parser::operator= (const Parser& Value)
		{
			if (&Value == this)
				return *this;

			if (Safe)
				delete L;

			Safe = true;
			if (Value.L != nullptr)
				L = new std::string(*Value.L);
			else
				L = new std::string();

			return *this;
		}
		std::string Parser::ToStringAutoPrec(float Number)
		{
			std::string Result(std::to_string(Number));
			Result.erase(Result.find_last_not_of('0') + 1, std::string::npos);
			if (!Result.empty() && Result.back() == '.')
				Result.erase(Result.end() - 1);

			return Result;
		}
		std::string Parser::ToStringAutoPrec(double Number)
		{
			std::string Result(std::to_string(Number));
			Result.erase(Result.find_last_not_of('0') + 1, std::string::npos);
			if (!Result.empty() && Result.back() == '.')
				Result.erase(Result.end() - 1);

			return Result;
		}

		SpinLock::SpinLock()
		{
		}
		void SpinLock::Acquire()
		{
			while (Atom.test_and_set(std::memory_order_acquire))
				std::this_thread::yield();
		}
		void SpinLock::Release()
		{
			Atom.clear(std::memory_order_release);
		}

		Variant Var::Auto(const std::string& Value, bool Strict)
		{
			Variant Result;
			Result.Deserialize(Value, Strict);

			return Result;
		}
		Variant Var::Null()
		{
			return Variant(VarType_Null, nullptr);
		}
		Variant Var::Undefined()
		{
			return Variant(VarType_Undefined, nullptr);
		}
		Variant Var::Object()
		{
			return Variant(VarType_Object, nullptr);
		}
		Variant Var::Array()
		{
			return Variant(VarType_Array, nullptr);
		}
		Variant Var::Pointer(void* Value)
		{
			if (!Value)
				return Null();

			return Variant(VarType_Pointer, (char*)Value);
		}
		Variant Var::String(const std::string& Value)
		{
			Variant::String* Buffer = (Variant::String*)TH_MALLOC(sizeof(Variant::String));
			Buffer->Size = Value.size();
			Buffer->Buffer = (char*)TH_MALLOC(sizeof(char) * (Buffer->Size + 1));

			memcpy(Buffer->Buffer, Value.c_str(), sizeof(char) * Buffer->Size);
			Buffer->Buffer[Buffer->Size] = '\0';

			return Variant(VarType_String, (char*)Buffer);
		}
		Variant Var::String(const char* Value, size_t Size)
		{
			if (!Value)
				return Null();

			Variant::String* Buffer = (Variant::String*)TH_MALLOC(sizeof(Variant::String));
			Buffer->Size = Size;
			Buffer->Buffer = (char*)TH_MALLOC(sizeof(char) * (Buffer->Size + 1));

			memcpy(Buffer->Buffer, Value, sizeof(char) * Buffer->Size);
			Buffer->Buffer[Buffer->Size] = '\0';

			return Variant(VarType_String, (char*)Buffer);
		}
		Variant Var::Base64(const std::string& Value)
		{
			Variant::String* Buffer = (Variant::String*)TH_MALLOC(sizeof(Variant::String));
			Buffer->Size = Value.size();
			Buffer->Buffer = (char*)TH_MALLOC(sizeof(char) * (Buffer->Size + 1));

			memcpy(Buffer->Buffer, Value.c_str(), sizeof(char) * Buffer->Size);
			Buffer->Buffer[Buffer->Size] = '\0';

			return Variant(VarType_Base64, (char*)Buffer);
		}
		Variant Var::Base64(const unsigned char* Value, size_t Size)
		{
			return Base64((const char*)Value, Size);
		}
		Variant Var::Base64(const char* Value, size_t Size)
		{
			if (!Value)
				return Null();

			Variant::String* Buffer = (Variant::String*)TH_MALLOC(sizeof(Variant::String));
			Buffer->Size = Size;
			Buffer->Buffer = (char*)TH_MALLOC(sizeof(char) * (Buffer->Size + 1));

			memcpy(Buffer->Buffer, Value, sizeof(char) * Buffer->Size);
			Buffer->Buffer[Buffer->Size] = '\0';

			return Variant(VarType_Base64, (char*)Buffer);
		}
		Variant Var::Integer(int64_t Value)
		{
			char* Data = (char*)TH_MALLOC(sizeof(int64_t));
			memcpy(Data, (void*)&Value, sizeof(int64_t));

			return Variant(VarType_Integer, Data);
		}
		Variant Var::Number(double Value)
		{
			char* Data = (char*)TH_MALLOC(sizeof(double));
			memcpy(Data, (void*)&Value, sizeof(double));

			return Variant(VarType_Number, Data);
		}
		Variant Var::Decimal(const std::string& Value)
		{
			Variant::String* Buffer = (Variant::String*)TH_MALLOC(sizeof(Variant::String));
			Buffer->Size = Value.size();
			Buffer->Buffer = (char*)TH_MALLOC(sizeof(char) * (Buffer->Size + 1));

			memcpy(Buffer->Buffer, Value.c_str(), sizeof(char) * Buffer->Size);
			Buffer->Buffer[Buffer->Size] = '\0';

			return Variant(VarType_Decimal, (char*)Buffer);
		}
		Variant Var::Decimal(const char* Value, size_t Size)
		{
			Variant::String* Buffer = (Variant::String*)TH_MALLOC(sizeof(Variant::String));
			Buffer->Size = Size;
			Buffer->Buffer = (char*)TH_MALLOC(sizeof(char) * (Buffer->Size + 1));

			memcpy(Buffer->Buffer, Value, sizeof(char) * Buffer->Size);
			Buffer->Buffer[Buffer->Size] = '\0';

			return Variant(VarType_Decimal, (char*)Buffer);
		}
		Variant Var::Boolean(bool Value)
		{
			void* Ptr = (Value ? (void*)&Value : nullptr);
			return Variant(VarType_Boolean, (char*)Ptr);
		}

		void Mem::Create(size_t InitialSize)
		{
			if (!InitialSize)
				return;

			if (!Mutex)
				Mutex = new std::mutex();

			Heap = (MemoryPage*)malloc(InitialSize);
			if (Heap != nullptr)
			{
				Heap->Size = InitialSize - HeadSize;
				Heap->Allocated = false;
				HeapSize = InitialSize;
			}
			else
				TH_ERROR("[memerr] couldn't allocate page of size %llu", (uint64_t)InitialSize);
		}
		void Mem::Release()
		{
			if (Mutex != nullptr)
				Mutex->lock();

			HeapSize = 0;
			if (Heap != nullptr)
			{
				free(Heap);
				Heap = nullptr;
			}

			if (Mutex != nullptr)
			{
				Mutex->unlock();
				delete Mutex;
				Mutex = nullptr;
			}
		}
		void* Mem::Malloc(size_t Size)
		{
			if (!Heap)
				return malloc(Size);

			Atom.Acquire();
			MemoryPage* Result = FindFirstPage(Size);
			if (!Result)
			{
				Atom.Release();
				TH_ERROR("[memerr] out of memory");
				return nullptr;
			}

			SplitPage(Result, Size);
			Result->Allocated = true;
			Atom.Release();

			return (void*)&(Result->Data);
		}
		void* Mem::Realloc(void* Ptr, size_t Size)
		{
			if (!Ptr)
				return Malloc(Size);

			if (!Heap)
				return realloc(Ptr, Size);

			Atom.Acquire();
			MemoryPage* Block = (MemoryPage*)(static_cast<char*>(Ptr) - HeadSize);
			uint64_t BlockSize = Block->Size;

			Block->Allocated = false;
			ConcatSequentialPages(Block, Block->Allocated);
			Block->Allocated = true;

			if (Block->Size >= Size)
			{
				SplitPage(Block, Size);
				Block->Allocated = true;
				Atom.Release();
				return (void*)&(Block->Data);
			}

			SplitPage(Block, BlockSize);
			Block->Allocated = true;

			MemoryPage* NewBlock = FindFirstPage(Size);
			if (!NewBlock)
			{
				Atom.Release();
				TH_ERROR("[memerr] out of memory");
				return nullptr;
			}

			SplitPage(NewBlock, Size);
			memcpy(&(NewBlock->Data), &(Block->Data), Block->Size);
			NewBlock->Allocated = true;
			Block->Allocated = false;
			Atom.Release();

			return (void*)&(NewBlock->Data);
		}
		void Mem::Free(void* Ptr)
		{
			if (!Ptr)
				return;

			if (!Heap)
				return free(Ptr);

			Atom.Acquire();
			MemoryPage* Block = (MemoryPage*)(static_cast<char*>(Ptr) - HeadSize);
			Block->Allocated = false;
			Atom.Release();
		}
		void Mem::Interrupt()
		{
#ifndef NDEBUG
#ifndef TH_MICROSOFT
#ifndef SIGTRAP
			__debugbreak();
#else
			raise(SIGTRAP);
#endif
#else
			if (!IsDebuggerPresent())
				TH_ERROR("[dbg] cannot interrupt");
			else
				DebugBreak();
#endif
			TH_INFO("[dbg] process interruption called");
#endif
		}
		void Mem::ConcatSequentialPages(MemoryPage* Block, bool IsAllocated)
		{
			MemoryPage* Next = nullptr;
			while ((Next = (MemoryPage*)((char*)Block + Block->Size + HeadSize)))
			{
				if (!((char*)Next + HeadSize < (char*)Heap + HeapSize && (char*)Next + HeadSize >= (char*)Heap) || Next->Allocated != IsAllocated)
					break;

				Block->Size += Next->Size + HeadSize;
			}
		}
		Mem::MemoryPage* Mem::FindFirstPage(uint64_t MinSize)
		{
			static MemoryPage* Offset = nullptr;
			bool Repeated = false;

		Each:
			if (!Offset)
				Offset = Heap;

			while ((char*)Offset + sizeof(MemoryPage) < (char*)Heap + HeapSize && (char*)Offset + sizeof(MemoryPage) >= (char*)Heap)
			{
				if (!Offset->Allocated)
				{
					ConcatSequentialPages(Offset, false);
					if (Offset->Size >= MinSize)
						return Offset;
				}

				Offset = (MemoryPage*)((char*)Offset + Offset->Size + HeadSize);
			}

			Offset = nullptr;
			if (Repeated)
				return nullptr;

			Repeated = true;
			goto Each;
		}
		void Mem::SplitPage(MemoryPage* Block, uint64_t Size)
		{
			MemoryPage* Next = (MemoryPage*)((char*)Block + Size + HeadSize);
			MemoryPage* Base = (MemoryPage*)((char*)Block + Block->Size + HeadSize);
			uint64_t BlockSize = Block->Size;

			Block->Allocated = false;
			if (!((char*)Next + HeadSize < (char*)Base && (char*)Next + HeadSize >= (char*)nullptr))
				return;

			if (!((char*)Next + sizeof(MemoryPage) < (char*)Heap + HeapSize && (char*)Next + sizeof(MemoryPage) >= (char*)Heap))
				return;

			Block->Size = Size;
			Next->Size = BlockSize - (Size + HeadSize);
			Next->Allocated = false;
		}
		Mem::MemoryPage* Mem::Heap = nullptr;
		SpinLock Mem::Atom;
		uint64_t Mem::HeadSize = offsetof(Mem::MemoryPage, Data);
		uint64_t Mem::HeapSize = 0;
		std::mutex* Mem::Mutex = nullptr;

		void Debug::AttachCallback(const std::function<void(const char*, int)>& _Callback)
		{
			Callback = _Callback;
		}
		void Debug::AttachStream()
		{
			Enabled = true;
		}
		void Debug::DetachCallback()
		{
			Callback = nullptr;
		}
		void Debug::DetachStream()
		{
			Enabled = false;
		}
		void Debug::Log(int Level, int Line, const char* Source, const char* Format, ...)
		{
			if (!Source || !Format || (!Enabled && !Callback))
				return;

			auto TimeStamp = (time_t)time(nullptr);
			tm DateTime{ };
			char Date[64];

			if (Line < 0)
				Line = 0;

#if defined(TH_MICROSOFT)
			if (gmtime_s(&DateTime, &TimeStamp) != 0)
#elif defined(TH_UNIX)
			if (gmtime_r(&TimeStamp, &DateTime) == 0)
#else
			if (true)
#endif
				strncpy(Date, "01-01-1970 00:00:00", sizeof(Date));
			else
				strftime(Date, sizeof(Date), "%Y-%m-%d %H:%M:%S", &DateTime);

			char Buffer[8192];
			if (Level == 1)
			{
				int ErrorCode = OS::Error::Get();
#ifdef TH_MICROSOFT
				if (ErrorCode != ERROR_SUCCESS)
					snprintf(Buffer, sizeof(Buffer), "%s %s:%d [err] %s\n\tsystem: %s\n", Date, Source, Line, Format, OS::Error::GetName(ErrorCode).c_str());
				else
					snprintf(Buffer, sizeof(Buffer), "%s %s:%d [err] %s\n", Date, Source, Line, Format);
#else
				if (ErrorCode > 0)
					snprintf(Buffer, sizeof(Buffer), "%s %s:%d [err] %s\n\tsystem: %s\n", Date, Source, Line, Format, OS::Error::GetName(ErrorCode).c_str());
				else
					snprintf(Buffer, sizeof(Buffer), "%s %s:%d [err] %s\n", Date, Source, Line, Format);
#endif
			}
			else if (Level == 2)
				snprintf(Buffer, sizeof(Buffer), "%s %s:%d [warn] %s\n", Date, Source, Line, Format);
			else
				snprintf(Buffer, sizeof(Buffer), "%s %s:%d [info] %s\n", Date, Source, Line, Format);

			va_list Args;
			va_start(Args, Format);

			char Storage[8192];
			vsnprintf(Storage, sizeof(Storage), Buffer, Args);
			if (Callback)
				Callback(Storage, Level);

			if (Enabled)
			{
#if defined(TH_MICROSOFT) && defined(_DEBUG)
				OutputDebugStringA(Storage);
#endif
				printf("%s", Storage);
			}

			va_end(Args);
		}
		std::function<void(const char*, int)> Debug::Callback;
		bool Debug::Enabled = false;

		void Composer::AddRef(Object* Value)
		{
			if (Value != nullptr)
				Value->AddRef();
		}
		void Composer::SetFlag(Object* Value)
		{
			if (Value != nullptr)
				Value->SetFlag();
		}
		bool Composer::GetFlag(Object* Value)
		{
			return Value ? Value->GetFlag() : false;
		}
		int Composer::GetRefCount(Object* Value)
		{
			return Value ? Value->GetRefCount() : 1;
		}
		void Composer::Release(Object* Value)
		{
			if (Value != nullptr)
				Value->Release();
		}
		bool Composer::Clear()
		{
			if (!Factory)
				return false;

			delete Factory;
			Factory = nullptr;
			return true;
		}
		bool Composer::Pop(const std::string& Hash)
		{
			if (!Factory)
				return false;

			auto It = Factory->find(TH_COMPONENT_HASH(Hash));
			if (It == Factory->end())
				return false;

			Factory->erase(It);
			if (!Factory->empty())
				return true;

			delete Factory;
			Factory = nullptr;
			return true;
		}
		std::unordered_map<uint64_t, void*>* Composer::Factory = nullptr;

		Object::Object() noexcept : __vcnt(1), __vflg(false)
		{
		}
		Object::~Object() noexcept
		{
		}
		void Object::operator delete(void* Data) noexcept
		{
			Object* Ref = (Object*)Data;
			if (Ref != nullptr && --Ref->__vcnt <= 0)
				Mem::Free(Data);
		}
		void* Object::operator new(size_t Size) noexcept
		{
			return Mem::Malloc((size_t)Size);
		}
		void Object::SetFlag() noexcept
		{
			__vflg = true;
		}
		bool Object::GetFlag() noexcept
		{
			return __vflg.load();
		}
		int Object::GetRefCount() noexcept
		{
			return __vcnt.load();
		}
		Object* Object::AddRef() noexcept
		{
			__vcnt++;
			__vflg = false;
			return this;
		}
		Object* Object::Release() noexcept
		{
			__vflg = false;
			if (--__vcnt)
				return this;

			delete this;
			return nullptr;
		}

		Console::Console() : Handle(false), Time(0)
		{
		}
		Console::~Console()
		{
			if (Singleton == this)
				Singleton = nullptr;

#ifdef TH_MICROSOFT
			if (!Handle)
				return;

			::ShowWindow(::GetConsoleWindow(), SW_HIDE);
			FreeConsole();
#endif
		}
		void Console::Hide()
		{
#ifdef TH_MICROSOFT
			if (Handle)
				::ShowWindow(::GetConsoleWindow(), SW_HIDE);
#endif
		}
		void Console::Show()
		{
#ifdef TH_MICROSOFT
			if (Handle)
			{
				::ShowWindow(::GetConsoleWindow(), SW_SHOW);
				return;
			}

			if (AllocConsole() == 0)
				return;

			freopen("conin$", "r", stdin);
			freopen("conout$", "w", stdout);
			freopen("conout$", "w", stderr);
			SetConsoleCtrlHandler(ConsoleEventHandler, true);
#else
			if (Handle)
				return;
#endif
			Handle = true;
		}
		void Console::Clear()
		{
#ifdef TH_MICROSOFT
			if (!Handle)
				return;

			HANDLE Wnd = GetStdHandle(STD_OUTPUT_HANDLE);

			CONSOLE_SCREEN_BUFFER_INFO Info;
			GetConsoleScreenBufferInfo((HANDLE)Wnd, &Info);

			COORD TopLeft = { 0, 0 };
			DWORD Written;
			FillConsoleOutputCharacterA((HANDLE)Wnd, ' ', Info.dwSize.X * Info.dwSize.Y, TopLeft, &Written);
			FillConsoleOutputAttribute((HANDLE)Wnd, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE, Info.dwSize.X * Info.dwSize.Y, TopLeft, &Written);
			SetConsoleCursorPosition((HANDLE)Wnd, TopLeft);
#elif defined TH_UNIX
			std::cout << "\033[2J";
#endif
		}
		void Console::Flush()
		{
			if (Handle)
				std::cout.flush();
		}
		void Console::FlushWrite()
		{
			if (Handle)
				std::cout << std::flush;
		}
		void Console::CaptureTime()
		{
			Time = (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000.0;
		}
		void Console::WriteLine(const std::string& Line)
		{
			if (Handle)
				std::cout << Line << '\n';
		}
		void Console::Write(const std::string& Line)
		{
			if (Handle)
				std::cout << Line;
		}
		void Console::fWriteLine(const char* Format, ...)
		{
			if (!Handle)
				return;

			char Buffer[8192] = { '\0' };

			va_list Args;
			va_start(Args, Format);
#ifdef TH_MICROSOFT
			_vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#else
			vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#endif
			va_end(Args);

			std::cout << Buffer << '\n';
		}
		void Console::fWrite(const char* Format, ...)
		{
			if (!Handle)
				return;

			char Buffer[8192] = { '\0' };

			va_list Args;
			va_start(Args, Format);
#ifdef TH_MICROSOFT
			_vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#else
			vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#endif
			va_end(Args);

			std::cout << Buffer;
		}
		void Console::sWriteLine(const std::string& Line)
		{
			if (!Handle)
				return;

			Lock.lock();
			std::cout << Line << '\n';
			Lock.unlock();
		}
		void Console::sWrite(const std::string& Line)
		{
			if (!Handle)
				return;

			Lock.lock();
			std::cout << Line;
			Lock.unlock();
		}
		void Console::sfWriteLine(const char* Format, ...)
		{
			if (!Handle)
				return;

			char Buffer[8192] = { '\0' };

			va_list Args;
			va_start(Args, Format);
#ifdef TH_MICROSOFT
			_vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#else
			vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#endif
			va_end(Args);

			Lock.lock();
			std::cout << Buffer << '\n';
			Lock.unlock();
		}
		void Console::sfWrite(const char* Format, ...)
		{
			if (!Handle)
				return;

			char Buffer[8192] = { '\0' };

			va_list Args;
			va_start(Args, Format);
#ifdef TH_MICROSOFT
			_vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#else
			vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#endif
			va_end(Args);

			Lock.lock();
			std::cout << Buffer;
			Lock.unlock();
		}
		void Console::Trace(const char* Format, ...)
		{
			char Buffer[2048];
			va_list Args;
			va_start(Args, Format);
#ifdef TH_MICROSOFT
			_vsnprintf(Buffer, sizeof(Buffer), Format, Args);
			va_end(Args);

			OutputDebugString(Buffer);
#elif defined TH_UNIX
			vsnprintf(Buffer, sizeof(Buffer), Format, Args);
			va_end(Args);

			std::cout << Buffer;
#endif
		}
		double Console::GetCapturedTime()
		{
			return (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000.0 - Time;
		}
		std::string Console::Read(uint64_t Size)
		{
			if (!Handle || !Size)
				return "";

			if (Size < 2)
				Size = 2;

			char* Value = new char[(size_t)(Size + 1)];
			memset(Value, 0, (size_t)Size * sizeof(char));
			Value[Size] = '\0';

			std::cin.getline(Value, Size);
			std::string Output = Value;
			delete[] Value;

			return Output;
		}
		Console* Console::Get()
		{
			if (Singleton == nullptr)
				Singleton = new Console();

			return Singleton;
		}
		Console* Console::Singleton = nullptr;

		TickTimer::TickTimer()
		{
			Time = 0.0;
			Delay = 16.0;
		}
		bool TickTimer::TickEvent(double ElapsedTime)
		{
			if (ElapsedTime - Time > Delay)
			{
				Time = ElapsedTime;
				return true;
			}

			return false;
		}
		double TickTimer::GetTime()
		{
			return Time;
		}

		Timer::Timer() : FrameLimit(0), TickCounter(16), TimeIncrement(0.0), CapturedTime(0.0)
		{
#ifdef TH_MICROSOFT
			Frequency = new LARGE_INTEGER();
			QueryPerformanceFrequency((LARGE_INTEGER*)Frequency);

			TimeLimit = new LARGE_INTEGER();
			QueryPerformanceCounter((LARGE_INTEGER*)TimeLimit);

			PastTime = new LARGE_INTEGER();
			QueryPerformanceCounter((LARGE_INTEGER*)PastTime);
#elif defined TH_UNIX
			Frequency = new timespec();
			clock_gettime(CLOCK_REALTIME, (timespec*)Frequency);

			TimeLimit = new timespec();
			clock_gettime(CLOCK_REALTIME, (timespec*)TimeLimit);

			PastTime = new timespec();
			clock_gettime(CLOCK_REALTIME, (timespec*)PastTime);
#endif
			SetStepLimitation(60.0f, 10.0f);
		}
		Timer::~Timer()
		{
#ifdef TH_MICROSOFT
			if (PastTime != nullptr)
				delete (LARGE_INTEGER*)PastTime;
			PastTime = nullptr;

			if (TimeLimit != nullptr)
				delete (LARGE_INTEGER*)TimeLimit;
			TimeLimit = nullptr;

			if (Frequency != nullptr)
				delete (LARGE_INTEGER*)Frequency;
			Frequency = nullptr;
#elif defined TH_UNIX
			if (PastTime != nullptr)
				delete (timespec*)PastTime;
			PastTime = nullptr;

			if (TimeLimit != nullptr)
				delete (timespec*)TimeLimit;
			TimeLimit = nullptr;

			if (Frequency != nullptr)
				delete (timespec*)Frequency;
#endif
		}
		double Timer::GetTimeIncrement()
		{
			return TimeIncrement;
		}
		double Timer::GetTickCounter()
		{
			return TickCounter;
		}
		double Timer::GetFrameCount()
		{
			return FrameCount;
		}
		double Timer::GetElapsedTime()
		{
#ifdef TH_MICROSOFT
			QueryPerformanceCounter((LARGE_INTEGER*)PastTime);
			return (((LARGE_INTEGER*)PastTime)->QuadPart - ((LARGE_INTEGER*)TimeLimit)->QuadPart) * 1000.0 / ((LARGE_INTEGER*)Frequency)->QuadPart;
#elif defined TH_UNIX
			clock_gettime(CLOCK_REALTIME, (timespec*)PastTime);
			return (((timespec*)PastTime)->tv_nsec - ((timespec*)TimeLimit)->tv_nsec) * 1000.0 / ((timespec*)Frequency)->tv_nsec;
#endif
		}
		double Timer::GetCapturedTime()
		{
			return GetElapsedTime() - CapturedTime;
		}
		double Timer::GetTimeStep()
		{
			double TimeStep = 1.0 / FrameCount;
			if (TimeStep > TimeStepLimit)
				return TimeStepLimit;

			return TimeStep;
		}
		double Timer::GetDeltaTime()
		{
			double DeltaTime = FrameRelation / FrameCount;
			if (DeltaTime > DeltaTimeLimit)
				return DeltaTimeLimit;

			return DeltaTime;
		}
		void Timer::SetStepLimitation(double MaxFrames, double MinFrames)
		{
			MinFrames = MinFrames >= 0.1 ? MinFrames : 0.1;
			FrameRelation = MaxFrames;

			TimeStepLimit = 1.0f / MinFrames;
			DeltaTimeLimit = FrameRelation / MinFrames;
		}
		void Timer::Synchronize()
		{
			double ElapsedTime = GetElapsedTime();
			double Tick = ElapsedTime - TickCounter;

			FrameCount = 1000.0 / (Tick >= 0.000001 ? Tick : 0.000001);
			TickCounter = ElapsedTime;

			if (FrameLimit <= 0)
				return;

			if (ElapsedTime - TimeIncrement < 1000.0 / FrameLimit)
				std::this_thread::sleep_for(std::chrono::milliseconds((uint64_t)(1000.0f / FrameLimit - ElapsedTime + TimeIncrement)));
			TimeIncrement = GetElapsedTime();
		}
		void Timer::CaptureTime()
		{
			CapturedTime = GetElapsedTime();
		}
		void Timer::Sleep(uint64_t Ms)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(Ms));
		}

		Stream::Stream()
		{
		}
		std::string& Stream::GetSource()
		{
			return Path;
		}
		uint64_t Stream::GetSize()
		{
			uint64_t Position = Tell();
			Seek(FileSeek_End, 0);
			uint64_t Size = Tell();
			Seek(FileSeek_Begin, Position);

			return Size;
		}

		FileStream::FileStream() : Resource(nullptr)
		{
		}
		FileStream::~FileStream()
		{
			Close();
		}
		void FileStream::Clear()
		{
			Close();
			if (!Path.empty())
				Resource = (FILE*)OS::File::Open(Path.c_str(), "w");
		}
		bool FileStream::Open(const char* File, FileMode Mode)
		{
			if (!File || !Close())
				return false;

			Path = OS::Path::Resolve(File).c_str();
			switch (Mode)
			{
				case FileMode_Read_Only:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "r");
					break;
				case FileMode_Write_Only:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "w");
					break;
				case FileMode_Append_Only:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "a");
					break;
				case FileMode_Read_Write:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "r+");
					break;
				case FileMode_Write_Read:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "w+");
					break;
				case FileMode_Read_Append_Write:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "a+");
					break;
				case FileMode_Binary_Read_Only:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "rb");
					break;
				case FileMode_Binary_Write_Only:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "wb");
					break;
				case FileMode_Binary_Append_Only:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "ab");
					break;
				case FileMode_Binary_Read_Write:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "rb+");
					break;
				case FileMode_Binary_Write_Read:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "wb+");
					break;
				case FileMode_Binary_Read_Append_Write:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "ab+");
					break;
			}

			return Resource != nullptr;
		}
		bool FileStream::Close()
		{
			if (Resource != nullptr)
			{
				fclose(Resource);
				Resource = nullptr;
			}

			return true;
		}
		bool FileStream::Seek(FileSeek Mode, int64_t Offset)
		{
			switch (Mode)
			{
				case FileSeek_Begin:
					if (Resource != nullptr)
						return fseek(Resource, (long)Offset, SEEK_SET) == 0;
					break;
				case FileSeek_Current:
					if (Resource != nullptr)
						return fseek(Resource, (long)Offset, SEEK_CUR) == 0;
					break;
				case FileSeek_End:
					if (Resource != nullptr)
						return fseek(Resource, (long)Offset, SEEK_END) == 0;
					break;
			}

			return false;
		}
		bool FileStream::Move(int64_t Offset)
		{
			if (!Resource)
				return false;

			return fseek(Resource, (long)Offset, SEEK_CUR) == 0;
		}
		int FileStream::Flush()
		{
			if (!Resource)
				return 0;

			return fflush(Resource);
		}
		uint64_t FileStream::ReadAny(const char* Format, ...)
		{
			va_list Args;
			uint64_t R = 0;
			va_start(Args, Format);

			if (Resource != nullptr)
				R = (uint64_t)vfscanf(Resource, Format, Args);

			va_end(Args);

			return R;
		}
		uint64_t FileStream::Read(char* Data, uint64_t Length)
		{
			if (!Resource)
				return 0;

			return fread(Data, 1, (size_t)Length, Resource);
		}
		uint64_t FileStream::WriteAny(const char* Format, ...)
		{
			va_list Args;
			uint64_t R = 0;
			va_start(Args, Format);
			if (Resource != nullptr)
				R = (uint64_t)vfprintf(Resource, Format, Args);
			va_end(Args);

			return R;
		}
		uint64_t FileStream::Write(const char* Data, uint64_t Length)
		{
			if (!Resource)
				return 0;

			return fwrite(Data, 1, (size_t)Length, Resource);
		}
		uint64_t FileStream::Tell()
		{
			if (!Resource)
				return 0;

			return ftell(Resource);
		}
		int FileStream::GetFd()
		{
			if (!Resource)
				return -1;

#ifdef TH_MICROSOFT
			return _fileno(Resource);
#else
			return fileno(Resource);
#endif
		}
		void* FileStream::GetBuffer()
		{
			return (void*)Resource;
		}

		GzStream::GzStream() : Resource(nullptr)
		{
		}
		GzStream::~GzStream()
		{
			Close();
		}
		void GzStream::Clear()
		{
			Close();
			if (!Path.empty())
			{
				fclose((FILE*)OS::File::Open(Path.c_str(), "w"));
				Open(Path.c_str(), FileMode_Binary_Write_Only);
			}
		}
		bool GzStream::Open(const char* File, FileMode Mode)
		{
			if (!File || !Close())
				return false;

#ifdef TH_HAS_ZLIB
			Path = OS::Path::Resolve(File).c_str();
			switch (Mode)
			{
				case FileMode_Binary_Read_Only:
				case FileMode_Read_Only:
					Resource = gzopen(Path.c_str(), "rb");
					break;
				case FileMode_Binary_Write_Only:
				case FileMode_Write_Only:
					Resource = gzopen(Path.c_str(), "wb");
					break;
				case FileMode_Read_Write:
				case FileMode_Write_Read:
				case FileMode_Append_Only:
				case FileMode_Read_Append_Write:
				case FileMode_Binary_Append_Only:
				case FileMode_Binary_Read_Write:
				case FileMode_Binary_Write_Read:
				case FileMode_Binary_Read_Append_Write:
					Close();
					break;
			}

			return Resource != nullptr;
#else
			return false;
#endif
		}
		bool GzStream::Close()
		{
#ifdef TH_HAS_ZLIB
			if (Resource != nullptr)
			{
				gzclose((gzFile)Resource);
				Resource = nullptr;
			}
#endif
			return true;
		}
		bool GzStream::Seek(FileSeek Mode, int64_t Offset)
		{
			switch (Mode)
			{
				case FileSeek_Begin:
#ifdef TH_HAS_ZLIB
					if (Resource != nullptr)
						return gzseek((gzFile)Resource, (long)Offset, SEEK_SET) == 0;
#endif
					break;
				case FileSeek_Current:
#ifdef TH_HAS_ZLIB
					if (Resource != nullptr)
						return gzseek((gzFile)Resource, (long)Offset, SEEK_CUR) == 0;
#endif
					break;
				case FileSeek_End:
#ifdef TH_HAS_ZLIB
					if (Resource != nullptr)
						return gzseek((gzFile)Resource, (long)Offset, SEEK_END) == 0;
#endif
					break;
			}

			return false;
		}
		bool GzStream::Move(int64_t Offset)
		{
#ifdef TH_HAS_ZLIB
			if (Resource != nullptr)
				return gzseek((gzFile)Resource, (long)Offset, SEEK_CUR) == 0;
#endif
			return false;
		}
		int GzStream::Flush()
		{
			if (!Resource)
				return 0;
#ifdef TH_HAS_ZLIB
			return gzflush((gzFile)Resource, Z_SYNC_FLUSH);
#else
			return 0;
#endif
		}
		uint64_t GzStream::ReadAny(const char* Format, ...)
		{
			return 0;
		}
		uint64_t GzStream::Read(char* Data, uint64_t Length)
		{
#ifdef TH_HAS_ZLIB
			if (Resource != nullptr)
				return gzread((gzFile)Resource, Data, Length);
#endif
			return 0;
		}
		uint64_t GzStream::WriteAny(const char* Format, ...)
		{
			va_list Args;
			uint64_t R = 0;
			va_start(Args, Format);
#ifdef TH_HAS_ZLIB
			if (Resource != nullptr)
				R = (uint64_t)gzvprintf((gzFile)Resource, Format, Args);
#endif
			va_end(Args);

			return R;
		}
		uint64_t GzStream::Write(const char* Data, uint64_t Length)
		{
#ifdef TH_HAS_ZLIB
			if (Resource != nullptr)
				return gzwrite((gzFile)Resource, Data, Length);
#endif
			return 0;
		}
		uint64_t GzStream::Tell()
		{
#ifdef TH_HAS_ZLIB
			if (Resource != nullptr)
				return gztell((gzFile)Resource);
#endif
			return 0;
		}
		int GzStream::GetFd()
		{
			return -1;
		}
		void* GzStream::GetBuffer()
		{
			return (void*)Resource;
		}

		WebStream::WebStream() : Resource(nullptr), Offset(0), Size(0)
		{
		}
		WebStream::~WebStream()
		{
			Close();
		}
		void WebStream::Clear()
		{
		}
		bool WebStream::Open(const char* File, FileMode Mode)
		{
			if (!File || !Close())
				return false;

			Network::SourceURL URL(File);
			if (URL.Protocol != "http" && URL.Protocol != "https")
				return false;

			Network::Host Address;
			Address.Hostname = URL.Host;
			Address.Secure = (URL.Protocol == "https");
			Address.Port = (URL.Port < 0 ? (Address.Secure ? 443 : 80) : URL.Port);

			bool Chunked = false;
			switch (Mode)
			{
				case FileMode_Binary_Read_Only:
				case FileMode_Read_Only:
				{
					auto* Client = new Network::HTTP::Client(30000);
					Client->Connect(&Address, false).Sync().Then<Core::Async<Network::HTTP::ResponseFrame*>>([Client, URL](int Code)
					{
						if (Code < 0)
							return Core::Async<Network::HTTP::ResponseFrame*>::Store(nullptr);

						Network::HTTP::RequestFrame Request;
						strcpy(Request.Version, "HTTP/1.1");
						strcpy(Request.Method, "GET");
						Request.URI.assign('/' + URL.Path);

						for (auto& Item : URL.Query)
							Request.Query += Item.first + "=" + Item.second;

						return Client->Send(&Request);
					}).Then<Core::Async<Network::HTTP::ResponseFrame*>>([this, Client, &Chunked](Network::HTTP::ResponseFrame* Response)
					{
						if (!Response || Response->StatusCode < 0)
							return Core::Async<Network::HTTP::ResponseFrame*>::Store(nullptr);

						const char* ContentLength = Response->GetHeader("Content-Length");
						if (ContentLength != nullptr)
						{
							this->Size = Parser(ContentLength).ToUInt64();
							return Core::Async<Network::HTTP::ResponseFrame*>::Store(Response);
						}

						Chunked = true;
						return Client->Consume(1024 * 1024 * 16);
					}).Await([this, Client, &Chunked](Network::HTTP::ResponseFrame* Response)
					{
						if (Response != nullptr && Response->StatusCode >= 0)
						{
							this->Resource = Client;
							if (!Chunked)
								return;

							this->Buffer.assign(Response->Buffer.begin(), Response->Buffer.end());
							this->Size = Response->Buffer.size();
						}
						else
							TH_RELEASE(Client);
					});
					break;
				}
				case FileMode_Binary_Write_Only:
				case FileMode_Write_Only:
				case FileMode_Read_Write:
				case FileMode_Write_Read:
				case FileMode_Append_Only:
				case FileMode_Read_Append_Write:
				case FileMode_Binary_Append_Only:
				case FileMode_Binary_Read_Write:
				case FileMode_Binary_Write_Read:
				case FileMode_Binary_Read_Append_Write:
					Close();
					break;
			}

			return Resource != nullptr;
		}
		bool WebStream::Close()
		{
			auto* Client = (Network::HTTP::Client*)Resource;
			TH_RELEASE(Client);
			Resource = nullptr;
			Offset = Size = 0;
			std::string().swap(Buffer);

			return true;
		}
		bool WebStream::Seek(FileSeek Mode, int64_t NewOffset)
		{
			switch (Mode)
			{
				case FileSeek_Begin:
					Offset = NewOffset;
					return true;
				case FileSeek_Current:
					Offset += NewOffset;
					return true;
				case FileSeek_End:
					Offset = Size - Offset;
					return true;
			}

			return false;
		}
		bool WebStream::Move(int64_t Offset)
		{
			return false;
		}
		int WebStream::Flush()
		{
			return 0;
		}
		uint64_t WebStream::ReadAny(const char* Format, ...)
		{
			return 0;
		}
		uint64_t WebStream::Read(char* Data, uint64_t Length)
		{
			if (!Resource || !Length)
				return 0;

			uint64_t Result = 0;
			if (!Buffer.empty())
			{
				Result = std::min(Length, (uint64_t)Buffer.size() - Offset);
				memcpy(Data, Buffer.c_str() + Offset, Result);
				Offset += Result;

				return Result;
			}

			((Network::HTTP::Client*)Resource)->Consume(Length).Sync().Await([this, &Data, &Length, &Result](Network::HTTP::ResponseFrame* Response)
			{
				Result = std::min(Length, (uint64_t)Response->Buffer.size());
				memcpy(Data, Response->Buffer.data(), Result);
			});

			Offset += Result;
			return Result;
		}
		uint64_t WebStream::WriteAny(const char* Format, ...)
		{
			return 0;
		}
		uint64_t WebStream::Write(const char* Data, uint64_t Length)
		{
			return 0;
		}
		uint64_t WebStream::Tell()
		{
			return Offset;
		}
		int WebStream::GetFd()
		{
			return (int)((Network::HTTP::Client*)Resource)->GetStream()->GetFd();
		}
		void* WebStream::GetBuffer()
		{
			return (void*)Resource;
		}

		FileTree::FileTree(const std::string& Folder)
		{
			Path = OS::Path::Resolve(Folder.c_str());
			if (!Path.empty())
			{
				OS::Directory::Each(Path.c_str(), [this](DirectoryEntry* Entry) -> bool
				{
					if (Entry->IsDirectory)
						Directories.push_back(new FileTree(Entry->Path));
					else
						Files.push_back(OS::Path::Resolve(Entry->Path.c_str()));

					return true;
				});
			}
			else
			{
				std::vector<std::string> Drives = OS::Directory::GetMounts();
				for (auto& Drive : Drives)
					Directories.push_back(new FileTree(Drive));
			}
		}
		FileTree::~FileTree()
		{
			for (auto& Directory : Directories)
				TH_RELEASE(Directory);
		}
		void FileTree::Loop(const std::function<bool(FileTree*)>& Callback)
		{
			if (!Callback || !Callback(this))
				return;

			for (auto& Directory : Directories)
				Directory->Loop(Callback);
		}
		FileTree* FileTree::Find(const std::string& V)
		{
			if (Path == V)
				return this;

			for (auto& Directory : Directories)
			{
				FileTree* Ref = Directory->Find(V);
				if (Ref != nullptr)
					return Ref;
			}

			return nullptr;
		}
		uint64_t FileTree::GetFiles()
		{
			uint64_t Count = Files.size();
			for (auto& Directory : Directories)
				Count += Directory->GetFiles();

			return Count;
		}

		void OS::Directory::Set(const char* Path)
		{
			if (!Path)
				return;

#ifdef TH_MICROSOFT
			if (!SetCurrentDirectoryA(Path))
				TH_ERROR("[dir] couldn't set current directory");
#elif defined(TH_UNIX)
			if (chdir(Path) != 0)
				TH_ERROR("[dir] couldn't set current directory");
#endif
		}
		void OS::Directory::Patch(const std::string& Path)
		{
			if (!IsExists(Path.c_str()))
				Create(Path.c_str());
		}
		bool OS::Directory::Scan(const std::string& Path, std::vector<ResourceEntry>* Entries)
		{
			if (!Entries)
				return false;

			ResourceEntry Entry;

#if defined(TH_MICROSOFT)
			struct Dirent
			{
				char Directory[1024];
			};

			struct Directory
			{
				HANDLE Handle;
				WIN32_FIND_DATAW Info;
				Dirent Result;
			};

			wchar_t WPath[1024];
			UnicodePath(Path.c_str(), WPath, sizeof(WPath) / sizeof(WPath[0]));

			auto* Value = (Directory*)TH_MALLOC(sizeof(Directory));
			DWORD Attributes = GetFileAttributesW(WPath);
			if (Attributes != 0xFFFFFFFF && ((Attributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY))
			{
				wcscat(WPath, L"\\*");
				Value->Handle = FindFirstFileW(WPath, &Value->Info);
				Value->Result.Directory[0] = '\0';
			}
			else
			{
				TH_FREE(Value);
				return false;
			}

			while (true)
			{
				Dirent* Next = &Value->Result;
				WideCharToMultiByte(CP_UTF8, 0, Value->Info.cFileName, -1, Next->Directory, sizeof(Next->Directory), nullptr, nullptr);
				if (strcmp(Next->Directory, ".") != 0 && strcmp(Next->Directory, "..") != 0 && File::State(Path + '/' + Next->Directory, &Entry.Source))
				{
					Entry.Path = Next->Directory;
					Entries->push_back(Entry);
				}

				if (!FindNextFileW(Value->Handle, &Value->Info))
				{
					FindClose(Value->Handle);
					Value->Handle = INVALID_HANDLE_VALUE;
					break;
				}
			}

			if (Value->Handle != INVALID_HANDLE_VALUE)
				FindClose(Value->Handle);

			TH_FREE(Value);
			return true;
#else
			DIR* Value = opendir(Path.c_str());
			if (!Value)
				return false;

			dirent* Dirent = nullptr;
			while ((Dirent = readdir(Value)) != nullptr)
			{
				if (strcmp(Dirent->d_name, ".") && strcmp(Dirent->d_name, "..") && File::State(Path + '/' + Dirent->d_name, &Entry.Source))
				{
					Entry.Path = Dirent->d_name;
					Entries->push_back(Entry);
				}
			}

			closedir(Value);
#endif
			return true;
		}
		bool OS::Directory::Each(const char* Path, const std::function<bool(DirectoryEntry*)>& Callback)
		{
			if (!Path)
				return false;

			std::vector<ResourceEntry> Entries;
			std::string Result = Path::Resolve(Path);
			Scan(Result, &Entries);

			Parser R(&Result);
			if (!R.EndsWith('/') && !R.EndsWith('\\'))
				Result += '/';

			for (auto& Entrie : Entries)
			{
				DirectoryEntry Entry;
				Entry.Path = Result + Entrie.Path;
				Entry.IsGood = true;
				Entry.IsDirectory = Entrie.Source.IsDirectory;
				if (!Entry.IsDirectory)
					Entry.Length = Entrie.Source.Size;

				if (!Callback(&Entry))
					break;
			}

			return true;
		}
		bool OS::Directory::Create(const char* Path)
		{
			if (!Path || Path[0] == '\0')
				return false;

#ifdef TH_MICROSOFT
			wchar_t Buffer[1024];
			UnicodePath(Path, Buffer, 1024);
			if (::CreateDirectoryW(Buffer, nullptr) == TRUE || GetLastError() == ERROR_ALREADY_EXISTS)
				return true;

			size_t Index = wcslen(Buffer) - 1;
			while (Index > 0 && Buffer[Index] != '/' && Buffer[Index] != '\\')
				Index--;

			if (Index > 0 && !Create(std::string(Path).substr(0, Index).c_str()))
				return false;

			return ::CreateDirectoryW(Buffer, nullptr) == TRUE || GetLastError() == ERROR_ALREADY_EXISTS;
#else
			if (mkdir(Path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1 || errno == EEXIST)
				return true;

			size_t Index = strlen(Path) - 1;
			while (Index > 0 && Path[Index] != '/' && Path[Index] != '\\')
				Index--;

			if (Index > 0 && !Create(std::string(Path).substr(0, Index).c_str()))
				return false;

			return mkdir(Path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1 || errno == EEXIST;
#endif
		}
		bool OS::Directory::Remove(const char* Path)
		{
#ifdef TH_MICROSOFT
			WIN32_FIND_DATA FileInformation;
			std::string FilePath, Pattern = std::string(Path) + "\\*.*";
			HANDLE Handle = ::FindFirstFile(Pattern.c_str(), &FileInformation);

			if (Handle == INVALID_HANDLE_VALUE)
				return false;

			do
			{
				if (FileInformation.cFileName[0] == '.')
					continue;

				FilePath = std::string(Path) + "\\" + FileInformation.cFileName;
				if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					return Remove(FilePath.c_str());

				if (::SetFileAttributes(FilePath.c_str(), FILE_ATTRIBUTE_NORMAL) == FALSE)
					return false;

				if (::DeleteFile(FilePath.c_str()) == FALSE)
					return false;
			} while (::FindNextFile(Handle, &FileInformation) == TRUE);
			::FindClose(Handle);

			if (::GetLastError() != ERROR_NO_MORE_FILES)
				return false;

			if (::SetFileAttributes(Path, FILE_ATTRIBUTE_NORMAL) == FALSE)
				return false;

			return ::RemoveDirectory(Path) != FALSE;
#elif defined TH_UNIX
			DIR* Root = opendir(Path);
			size_t Size = strlen(Path);

			if (!Root)
				return (rmdir(Path) == 0);

			struct dirent* It;
			while ((It = readdir(Root)))
			{
				char* Buffer; bool Next = false; size_t Length;
				if (!strcmp(It->d_name, ".") || !strcmp(It->d_name, ".."))
					continue;

				Length = Size + strlen(It->d_name) + 2;
				Buffer = (char*)TH_MALLOC(Length);

				if (!Buffer)
					continue;

				struct stat State;
				snprintf(Buffer, Length, "%s/%s", Path, It->d_name);

				if (!stat(Buffer, &State))
				{
					if (S_ISDIR(State.st_mode))
						Next = Remove(Buffer);
					else
						Next = (unlink(Buffer) == 0);
				}

				TH_FREE(Buffer);
				if (!Next)
					break;
			}

			closedir(Root);
			return (rmdir(Path) == 0);
#endif
		}
		bool OS::Directory::IsExists(const char* Path)
		{
			struct stat Buffer;
			if (stat(Path::Resolve(Path).c_str(), &Buffer) != 0)
				return false;

			return Buffer.st_mode & S_IFDIR;
		}
		std::string OS::Directory::Get()
		{
#ifndef TH_HAS_SDL2
			char Buffer[TH_MAX_PATH + 1] = { 0 };
#ifdef TH_MICROSOFT
			GetModuleFileNameA(nullptr, Buffer, TH_MAX_PATH);

			std::string Result = Path::GetDirectory(Buffer);
			memcpy(Buffer, Result.c_str(), sizeof(char) * Result.size());

			if (Result.size() < TH_MAX_PATH)
				Buffer[Result.size()] = '\0';
#elif defined TH_UNIX
			if (!getcwd(Buffer, TH_MAX_PATH))
				return "";
#endif
			int64_t Length = strlen(Buffer);
			if (Length > 0 && Buffer[Length - 1] != '/' && Buffer[Length - 1] != '\\')
			{
				Buffer[Length] = '/';
				Length++;
			}

			return std::string(Buffer, Length);
#else
			char* Base = SDL_GetBasePath();
			std::string Result = Base;
			SDL_free(Base);

			return Result;
#endif
		}
		std::vector<std::string> OS::Directory::GetMounts()
		{
			std::vector<std::string> Output;
#ifdef TH_MICROSOFT
			DWORD DriveMask = GetLogicalDrives();
			int Offset = (int)'A';

			while (DriveMask)
			{
				if (DriveMask & 1)
					Output.push_back(std::string(1, (char)Offset) + '\\');
				DriveMask >>= 1;
				Offset++;
			}

			return Output;
#else
			Output.push_back("/");
			return Output;
#endif
		}

		bool OS::File::State(const std::string& Path, Resource* Resource)
		{
			if (!Resource)
				return false;

			memset(Resource, 0, sizeof(*Resource));
#if defined(TH_MICROSOFT)
			wchar_t WBuffer[1024];
			UnicodePath(Path.c_str(), WBuffer, sizeof(WBuffer) / sizeof(WBuffer[0]));

			WIN32_FILE_ATTRIBUTE_DATA Info;
			if (GetFileAttributesExW(WBuffer, GetFileExInfoStandard, &Info) == 0)
				return false;

			Resource->Size = MAKEUQUAD(Info.nFileSizeLow, Info.nFileSizeHigh);
			Resource->LastModified = SYS2UNIX_TIME(Info.ftLastWriteTime.dwLowDateTime, Info.ftLastWriteTime.dwHighDateTime);
			Resource->CreationTime = SYS2UNIX_TIME(Info.ftCreationTime.dwLowDateTime, Info.ftCreationTime.dwHighDateTime);
			if (Resource->CreationTime > Resource->LastModified)
				Resource->LastModified = Resource->CreationTime;

			Resource->IsDirectory = Info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
			if (Resource->IsDirectory)
				return true;

			if (Path.empty())
				return false;

			int End = Path.back();
			if (isalnum(End) || strchr("_-", End) != nullptr)
				return true;

			memset(Resource, 0, sizeof(*Resource));
			return false;
#else
			struct stat State;
			if (stat(Path.c_str(), &State) != 0)
				return false;

			struct tm Time;
			LocalTime(&State.st_ctime, &Time);
			Resource->CreationTime = mktime(&Time);
			Resource->Size = (uint64_t)(State.st_size);
			Resource->LastModified = State.st_mtime;
			Resource->IsDirectory = S_ISDIR(State.st_mode);

			return true;
#endif
		}
		bool OS::File::Write(const char* Path, const char* Data, uint64_t Length)
		{
			FILE* Stream = (FILE*)Open(Path, "wb");
			if (!Stream)
				return false;

			fwrite((const void*)Data, (size_t)Length, 1, Stream);
			fclose(Stream);

			return true;
		}
		bool OS::File::Write(const char* Path, const std::string& Data)
		{
			FILE* Stream = (FILE*)Open(Path, "wb");
			if (!Stream)
				return false;

			fwrite((const void*)Data.c_str(), (size_t)Data.size(), 1, Stream);
			fclose(Stream);

			return true;
		}
		bool OS::File::Move(const char* From, const char* To)
		{
#ifdef TH_MICROSOFT
			return MoveFileA(From, To) != 0;
#elif defined TH_UNIX
			return !rename(From, To);
#endif
		}
		bool OS::File::Remove(const char* Path)
		{
#ifdef TH_MICROSOFT
			SetFileAttributesA(Path, 0);
			return DeleteFileA(Path) != 0;
#elif defined TH_UNIX
			return unlink(Path) == 0;
#endif
		}
		bool OS::File::IsExists(const char* Path)
		{
			struct stat Buffer;
			return (stat(Path::Resolve(Path).c_str(), &Buffer) == 0);
		}
		uint64_t OS::File::GetCheckSum(const std::string& Data)
		{
			return Compute::Common::CRC32(Data);
		}
		FileState OS::File::GetState(const char* Path)
		{
			FileState State;
			struct stat Buffer;

			if (stat(Path, &Buffer) != 0)
				return State;

			State.Exists = true;
			State.Size = Buffer.st_size;
			State.Links = Buffer.st_nlink;
			State.Permissions = Buffer.st_mode;
			State.Device = Buffer.st_dev;
			State.GroupId = Buffer.st_gid;
			State.UserId = Buffer.st_uid;
			State.IDocument = Buffer.st_ino;
			State.LastAccess = Buffer.st_atime;
			State.LastPermissionChange = Buffer.st_ctime;
			State.LastModified = Buffer.st_mtime;

			return State;
		}
		void* OS::File::Open(const char* Path, const char* Mode)
		{
			if (!Path || !Mode)
				return nullptr;

#ifdef TH_MICROSOFT
			wchar_t WBuffer[1024], WMode[20];
			UnicodePath(Path, WBuffer, sizeof(WBuffer) / sizeof(WBuffer[0]));
			MultiByteToWideChar(CP_UTF8, 0, Mode, -1, WMode, sizeof(WMode) / sizeof(WMode[0]));

			return (void*)_wfopen(WBuffer, WMode);
#else
			FILE* Stream = fopen(Path, Mode);
			if (Stream != nullptr)
				fcntl(fileno(Stream), F_SETFD, FD_CLOEXEC);

			return (void*)Stream;
#endif
		}
		Stream* OS::File::Open(const std::string& Path, FileMode Mode)
		{
			Network::SourceURL URL(Path);
			if (URL.Protocol == "file")
			{
				Stream* Result = nullptr;
				if (Parser(&Path).EndsWith(".gz"))
					Result = new GzStream();
				else
					Result = new FileStream();

				if (Result->Open(Path.c_str(), Mode))
					return Result;

				TH_RELEASE(Result);
			}
			else if (URL.Protocol == "http" || URL.Protocol == "https")
			{
				Stream* Result = new WebStream();
				if (Result->Open(Path.c_str(), Mode))
					return Result;

				TH_RELEASE(Result);
			}

			return nullptr;
		}
		unsigned char* OS::File::ReadAll(const char* Path, uint64_t* Length)
		{
			FILE* Stream = (FILE*)Open(Path, "rb");
			if (!Stream)
				return nullptr;

			fseek(Stream, 0, SEEK_END);
			uint64_t Size = ftell(Stream);
			fseek(Stream, 0, SEEK_SET);

			auto* Bytes = (unsigned char*)TH_MALLOC(sizeof(unsigned char) * (size_t)(Size + 1));
			if (fread((char*)Bytes, sizeof(unsigned char), (size_t)Size, Stream) != (size_t)Size)
			{
				fclose(Stream);
				TH_FREE(Bytes);

				if (Length != nullptr)
					*Length = 0;

				return nullptr;
			}

			Bytes[Size] = '\0';
			if (Length != nullptr)
				*Length = Size;

			fclose(Stream);
			return Bytes;
		}
		unsigned char* OS::File::ReadAll(Stream* Stream, uint64_t* Length)
		{
			if (!Stream)
				return nullptr;

			uint64_t Size = Stream->GetSize();
			auto* Bytes = new unsigned char[(size_t)(Size + 1)];
			Stream->Read((char*)Bytes, Size * sizeof(unsigned char));
			Bytes[Size] = '\0';

			if (Length != nullptr)
				*Length = Size;

			return Bytes;
		}
		unsigned char* OS::File::ReadChunk(Stream* Stream, uint64_t Length)
		{
			auto* Bytes = (unsigned char*)TH_MALLOC((size_t)(Length + 1));
			Stream->Read((char*)Bytes, Length);
			Bytes[Length] = '\0';

			return Bytes;
		}
		std::string OS::File::ReadAsString(const char* Path)
		{
			uint64_t Length = 0;
			char* Data = (char*)ReadAll(Path, &Length);
			if (!Data)
				return std::string();

			std::string Output = std::string(Data, Length);
			TH_FREE(Data);

			return Output;
		}
		std::vector<std::string> OS::File::ReadAsArray(const char* Path)
		{
			FILE* Stream = (FILE*)Open(Path, "rb");
			if (!Stream)
				return std::vector<std::string>();

			fseek(Stream, 0, SEEK_END);
			uint64_t Length = ftell(Stream);
			fseek(Stream, 0, SEEK_SET);

			char* Buffer = (char*)TH_MALLOC(sizeof(char) * Length);
			if (fread(Buffer, sizeof(char), (size_t)Length, Stream) != (size_t)Length)
			{
				fclose(Stream);
				TH_FREE(Buffer);
				return std::vector<std::string>();
			}

			std::string Result(Buffer, Length);
			fclose(Stream);
			TH_FREE(Buffer);

			return Parser(&Result).Split('\n');
		}

		bool OS::Path::IsRemote(const char* Path)
		{
			return Path && Network::SourceURL(Path).Protocol != "file";
		}
		std::string OS::Path::Resolve(const char* Path)
		{
			if (!Path || Path[0] == '\0')
				return "";

#ifdef TH_MICROSOFT
			char Buffer[2048] = { 0 };
			GetFullPathNameA(Path, sizeof(Buffer), Buffer, nullptr);
#elif defined TH_UNIX
			char* Data = realpath(Path, nullptr);
			if (!Data)
				return Path;

			std::string Buffer = Data;
			TH_FREE(Data);
#endif
			return Buffer;
		}
		std::string OS::Path::Resolve(const std::string& Path, const std::string& Directory)
		{
			if (Path.empty() || Directory.empty())
				return "";

			if (Parser(&Path).StartsOf("/\\"))
				return Resolve(("." + Path).c_str());

			if (Parser(&Directory).EndsOf("/\\"))
				return Resolve((Directory + Path).c_str());
#ifdef TH_MICROSOFT
			return Resolve((Directory + "\\" + Path).c_str());
#else
			return Resolve((Directory + "/" + Path).c_str());
#endif
		}
		std::string OS::Path::ResolveDirectory(const char* Path)
		{
			std::string Result = Resolve(Path);
			if (!Result.empty() && !Parser(&Result).EndsOf("/\\"))
				Result += '/';

			return Result;
		}
		std::string OS::Path::ResolveDirectory(const std::string& Path, const std::string& Directory)
		{
			std::string Result = Resolve(Path, Directory);
			if (!Result.empty() && !Parser(&Result).EndsOf("/\\"))
				Result += '/';

			return Result;
		}
		std::string OS::Path::GetDirectory(const char* Path, uint32_t Level)
		{
			Parser Buffer(Path);
			Parser::Settle Result = Buffer.ReverseFindOf("/\\");
			if (!Result.Found)
				return Path;

			uint64_t Size = Buffer.Size();
			for (uint32_t i = 0; i < Level; i++)
			{
				Parser::Settle Current = Buffer.ReverseFindOf("/\\", Size - Result.Start);
				if (!Current.Found)
				{
					if (Buffer.Splice(0, Result.End).Empty())
						return "/";

					return Buffer.R();
				}

				Result = Current;
			}

			if (Buffer.Splice(0, Result.End).Empty())
				return "/";

			return Buffer.R();
		}
		const char* OS::Path::GetFilename(const char* Path)
		{
			if (!Path)
				return nullptr;

			int64_t Size = (int64_t)strlen(Path);
			for (int64_t i = Size; i-- > 0;)
			{
				if (Path[i] == '/' || Path[i] == '\\')
					return Path + i + 1;
			}

			return Path;
		}
		const char* OS::Path::GetExtension(const char* Path)
		{
			if (!Path)
				return nullptr;

			const char* Buffer = Path;
			while (*Buffer != '\0')
				Buffer++;

			while (*Buffer != '.' && Buffer != Path)
				Buffer--;

			if (Buffer == Path)
				return nullptr;

			return Buffer;
		}

		bool OS::Net::SendFile(FILE* Stream, socket_t Socket, int64_t Size)
		{
			if (!Stream || !Size)
				return false;

#ifdef TH_MICROSOFT
			return TransmitFile((SOCKET)Socket, (HANDLE)_get_osfhandle(_fileno(Stream)), (DWORD)Size, 16384, nullptr, nullptr, 0) > 0;
#elif defined(TH_APPLE)
			return sendfile(fileno(Stream), Socket, 0, (off_t*)&Size, nullptr, 0);
#elif defined(TH_UNIX)
			return sendfile(Socket, fileno(Stream), nullptr, (size_t)Size) > 0;
#else
			return false;
#endif
		}
		bool OS::Net::GetETag(char* Buffer, uint64_t Length, Resource* Resource)
		{
			if (!Resource)
				return false;

			return GetETag(Buffer, Length, Resource->LastModified, Resource->Size);
		}
		bool OS::Net::GetETag(char* Buffer, uint64_t Length, int64_t LastModified, uint64_t ContentLength)
		{
			if (!Buffer || !Length)
				return false;

			snprintf(Buffer, (const size_t)Length, "\"%lx.%llu\"", (unsigned long)LastModified, ContentLength);
			return true;
		}
		socket_t OS::Net::GetFd(FILE* Stream)
		{
			if (!Stream)
				return -1;

#ifdef TH_MICROSOFT
			return _fileno(Stream);
#else
			return fileno(Stream);
#endif
		}

		void OS::Process::Execute(const char* Format, ...)
		{
			char Buffer[16384];

			va_list Args;
			va_start(Args, Format);
#ifdef TH_MICROSOFT
			_vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#else
			vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#endif
			va_end(Args);
			if (system(Buffer) == 0)
				TH_ERROR("[sys] couldn't execute command");
		}
		bool OS::Process::Spawn(const std::string& Path, const std::vector<std::string>& Params, ChildProcess* Child)
		{
#ifdef TH_MICROSOFT
			HANDLE Job = CreateJobObject(nullptr, nullptr);
			if (Job == nullptr)
			{
				TH_ERROR("cannot create job object for process");
				return false;
			}

			JOBOBJECT_EXTENDED_LIMIT_INFORMATION Info = { 0 };
			Info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
			if (SetInformationJobObject(Job, JobObjectExtendedLimitInformation, &Info, sizeof(Info)) == 0)
			{
				TH_ERROR("cannot set job object for process");
				return false;
			}

			STARTUPINFO StartupInfo;
			ZeroMemory(&StartupInfo, sizeof(StartupInfo));
			StartupInfo.cb = sizeof(StartupInfo);
			StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
			StartupInfo.wShowWindow = SW_HIDE;

			PROCESS_INFORMATION Process;
			ZeroMemory(&Process, sizeof(Process));

			Parser Exe = Path::Resolve(Path.c_str());
			if (!Exe.EndsWith(".exe"))
				Exe.Append(".exe");

			Parser Args = Form("\"%s\"", Exe.Get());
			for (const auto& Param : Params)
				Args.Append(' ').Append(Param);

			if (!CreateProcessA(Exe.Get(), Args.Value(), nullptr, nullptr, TRUE, CREATE_BREAKAWAY_FROM_JOB | HIGH_PRIORITY_CLASS, nullptr, nullptr, &StartupInfo, &Process))
			{
				TH_ERROR("cannot spawn process %s", Exe.Get());
				return false;
			}

			AssignProcessToJobObject(Job, Process.hProcess);
			if (Child != nullptr && !Child->Valid)
			{
				Child->Process = (void*)Process.hProcess;
				Child->Thread = (void*)Process.hThread;
				Child->Job = (void*)Job;
				Child->Valid = true;
			}

			return true;
#else
			if (!File::IsExists(Path.c_str()))
			{
				TH_ERROR("cannot spawn process %s (file does not exists)", Path.c_str());
				return false;
			}

			pid_t ProcessId = fork();
			if (ProcessId == 0)
			{
				std::vector<char*> Args;
				for (auto It = Params.begin(); It != Params.end(); It++)
					Args.push_back((char*)It->c_str());
				Args.push_back(nullptr);

				execve(Path.c_str(), Args.data(), nullptr);
				exit(0);
			}
			else if (Child != nullptr)
			{
				Child->Process = ProcessId;
				Child->Valid = (ProcessId > 0);
			}

			return (ProcessId > 0);
#endif
		}
		bool OS::Process::Await(ChildProcess* Process, int* ExitCode)
		{
			if (!Process || !Process->Valid)
				return false;

#ifdef TH_MICROSOFT
			WaitForSingleObject(Process->Process, INFINITE);
			if (ExitCode != nullptr)
			{
				DWORD Result;
				if (!GetExitCodeProcess(Process->Process, &Result))
				{
					Free(Process);
					return false;
				}

				*ExitCode = (int)Result;
			}
#else
			int Status;
			waitpid(Process->Process, &Status, 0);

			if (ExitCode != nullptr)
				*ExitCode = WEXITSTATUS(Status);
#endif

			Free(Process);
			return true;
		}
		bool OS::Process::Free(ChildProcess* Child)
		{
			if (!Child || !Child->Valid)
				return false;

#ifdef TH_MICROSOFT
			if (Child->Process != nullptr)
			{
				CloseHandle((HANDLE)Child->Process);
				Child->Process = nullptr;
			}

			if (Child->Thread != nullptr)
			{
				CloseHandle((HANDLE)Child->Thread);
				Child->Thread = nullptr;
			}

			if (Child->Job != nullptr)
			{
				CloseHandle((HANDLE)Child->Job);
				Child->Job = nullptr;
			}
#endif
			Child->Valid = false;
			return true;
		}

		void* OS::Symbol::Load(const std::string& Path)
		{
			Parser Name(Path);
#ifdef TH_MICROSOFT
			if (Path.empty())
				return GetModuleHandle(nullptr);

			if (!Name.EndsWith(".dll"))
				Name.Append(".dll");

			return (void*)LoadLibrary(Name.Get());
#elif defined(TH_APPLE)
			if (Path.empty())
				return (void*)dlopen(nullptr, RTLD_LAZY);

			if (!Name.EndsWith(".dylib"))
				Name.Append(".dylib");

			return (void*)dlopen(Name.Get(), RTLD_LAZY);
#elif defined(TH_UNIX)
			if (Path.empty())
				return (void*)dlopen(nullptr, RTLD_LAZY);

			if (!Name.EndsWith(".so"))
				Name.Append(".so");

			return (void*)dlopen(Name.Get(), RTLD_LAZY);
#else
			return nullptr;
#endif
		}
		void* OS::Symbol::LoadFunction(void* Handle, const std::string& Name)
		{
			if (!Handle || Name.empty())
				return nullptr;

#ifdef TH_MICROSOFT
			void* Result = (void*)GetProcAddress((HMODULE)Handle, Name.c_str());
			if (!Result)
			{
				LPVOID Buffer;
				FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&Buffer, 0, nullptr);
				LPVOID Display = (LPVOID)LocalAlloc(LMEM_ZEROINIT, (::lstrlen((LPCTSTR)Buffer) + 40) * sizeof(TCHAR));
				std::string Text((LPCTSTR)Display);
				LocalFree(Buffer);
				LocalFree(Display);

				if (!Text.empty())
					TH_ERROR("dll symload error: %s", Text.c_str());
			}

			return Result;
#elif defined(TH_UNIX)
			void* Result = (void*)dlsym(Handle, Name.c_str());
			if (!Result)
			{
				const char* Text = dlerror();
				if (Text != nullptr)
					TH_ERROR("so symload error: %s", Text);
			}
#else
			return nullptr;
#endif
		}
		bool OS::Symbol::Unload(void* Handle)
		{
			if (!Handle)
				return false;

#ifdef TH_MICROSOFT
			return (FreeLibrary((HMODULE)Handle) != 0);
#elif defined(TH_UNIX)
			return (dlclose(Handle) == 0);
#else
			return false;
#endif
		}

		bool OS::Input::Text(const std::string& Title, const std::string& Message, const std::string& DefaultInput, std::string* Result)
		{
			const char* Data = tinyfd_inputBox(Title.c_str(), Message.c_str(), DefaultInput.c_str());
			if (!Data)
				return false;

			if (!Result)
				*Result = Data;

			return true;
		}
		bool OS::Input::Password(const std::string& Title, const std::string& Message, std::string* Result)
		{
			const char* Data = tinyfd_inputBox(Title.c_str(), Message.c_str(), nullptr);
			if (!Data)
				return false;

			if (Result != nullptr)
				*Result = Data;

			return true;
		}
		bool OS::Input::Save(const std::string& Title, const std::string& DefaultPath, const std::string& Filter, const std::string& FilterDescription, std::string* Result)
		{
			std::vector<char*> Patterns;
			for (auto& It : Parser(&Filter).Split(','))
				Patterns.push_back(strdup(It.c_str()));

			const char* Data = tinyfd_saveFileDialog(Title.c_str(), DefaultPath.c_str(), Patterns.size(),
				Patterns.empty() ? nullptr : Patterns.data(), FilterDescription.empty() ? nullptr : FilterDescription.c_str());

			for (auto& It : Patterns)
				free(It);

			if (!Data)
				return false;

			if (Result != nullptr)
				*Result = Data;

			return true;
		}
		bool OS::Input::Open(const std::string& Title, const std::string& DefaultPath, const std::string& Filter, const std::string& FilterDescription, bool Multiple, std::string* Result)
		{
			std::vector<char*> Patterns;
			for (auto& It : Parser(&Filter).Split(','))
				Patterns.push_back(strdup(It.c_str()));

			const char* Data = tinyfd_openFileDialog(Title.c_str(), DefaultPath.c_str(), Patterns.size(),
				Patterns.empty() ? nullptr : Patterns.data(), FilterDescription.empty() ? nullptr : FilterDescription.c_str(), Multiple);

			for (auto& It : Patterns)
				free(It);

			if (!Data)
				return false;

			if (Result != nullptr)
				*Result = Data;

			return true;
		}
		bool OS::Input::Folder(const std::string& Title, const std::string& DefaultPath, std::string* Result)
		{
			const char* Data = tinyfd_selectFolderDialog(Title.c_str(), DefaultPath.c_str());
			if (!Data)
				return false;

			if (Result != nullptr)
				*Result = Data;

			return true;
		}
		bool OS::Input::Color(const std::string& Title, const std::string& DefaultHexRGB, std::string* Result)
		{
			unsigned char RGB[3] = { 0, 0, 0 };
			const char* Data = tinyfd_colorChooser(Title.c_str(), DefaultHexRGB.c_str(), RGB, RGB);
			if (!Data)
				return false;

			if (Result != nullptr)
				*Result = Data;

			return true;
		}

		int OS::Error::Get()
		{
#ifdef TH_MICROSOFT
			int ErrorCode = WSAGetLastError();
			WSASetLastError(0);

			return ErrorCode;
#else
			int ErrorCode = errno;
			errno = 0;

			return ErrorCode;
#endif
		}
		std::string OS::Error::GetName(int Code)
		{
#ifdef TH_MICROSOFT
			LPSTR Buffer = nullptr;
			size_t Size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, Code, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPSTR)&Buffer, 0, nullptr);
			std::string Result(Buffer, Size);
			LocalFree(Buffer);

			return Parser(&Result).Replace("\r", "").Replace("\n", "").R();
#else
			char* Buffer = strerror(Code);
			return Buffer ? Buffer : "";
#endif
		}

		ChangeLog::ChangeLog(const std::string& Root) : Path(Root), Offset(-1)
		{
			Source = new FileStream();
			auto V = Parser(&Path).Replace("/", "\\").Split('\\');
			if (!V.empty())
				Name = V.back();
		}
		ChangeLog::~ChangeLog()
		{
			TH_RELEASE(Source);
		}
		void ChangeLog::Process(const std::function<bool(ChangeLog*, const char*, int64_t)>& Callback)
		{
			Source->Open(Path.c_str(), FileMode_Binary_Read_Only);

			uint64_t Length = Source->GetSize();
			if (Length <= Offset || Offset <= 0)
			{
				Offset = Length;
				return;
			}

			int64_t Delta = Length - Offset;
			Source->Seek(FileSeek_Begin, Length - Delta);

			char* Data = (char*)TH_MALLOC(sizeof(char) * ((size_t)Delta + 1));
			Source->Read(Data, sizeof(char) * Delta);

			std::string Value = Data;
			int64_t ValueLength = -1;
			for (int64_t i = Value.size(); i > 0; i--)
			{
				if (Value[i] == '\n' && ValueLength == -1)
					ValueLength = i;

				if ((int)Value[i] < 0)
					Value[i] = ' ';
			}

			if (ValueLength == -1)
				ValueLength = Value.size();

			auto V = Parser(&Value).Substring(0, ValueLength).Replace("\t", "\n").Replace("\n\n", "\n").Replace("\r", "");
			TH_FREE(Data);

			if (Value == LastValue)
				return;

			LastValue = Value;
			if (V.Find("\n").Found)
			{
				std::vector<std::string> Lines = V.Split('\n');
				for (auto& Line : Lines)
				{
					if (Line.empty())
						continue;

					if (Callback && !Callback(this, Line.c_str(), (int64_t)Line.size()))
					{
						Offset = Length;
						return;
					}
				}
			}
			else if (!Value.empty() && Callback)
				Callback(this, Value.c_str(), (int64_t)Value.size());

			Offset = Length;
		}

		Schedule::Schedule() : Active(false), Terminate(false), Timer(0)
		{
		}
		Schedule::~Schedule()
		{
			Stop();
			if (Singleton == this)
				Singleton = nullptr;
		}
		EventId Schedule::SetInterval(uint64_t Milliseconds, const TimerCallback& Callback)
		{
			if (!Callback)
				return -1;

			int64_t Clock = GetClock();
			Sync.Timers.lock();

			EventId Id = Timer++;
			Timers.emplace_back(Callback, Milliseconds, Clock, Id, true);
			Sync.Timers.unlock();

			return Id;
		}
		EventId Schedule::SetInterval(uint64_t Milliseconds, TimerCallback&& Callback)
		{
			if (!Callback)
				return -1;

			int64_t Clock = GetClock();
			Sync.Timers.lock();

			EventId Id = Timer++;
			Timers.emplace_back(std::move(Callback), Milliseconds, Clock, Id, true);
			Sync.Timers.unlock();

			return Id;
		}
		EventId Schedule::SetTimeout(uint64_t Milliseconds, const TimerCallback& Callback)
		{
			if (!Callback)
				return -1;

			int64_t Clock = GetClock();
			Sync.Timers.lock();

			EventId Id = Timer++;
			Timers.emplace_back(Callback, Milliseconds, Clock, Id, false);
			Sync.Timers.unlock();

			return Id;
		}
		EventId Schedule::SetTimeout(uint64_t Milliseconds, TimerCallback&& Callback)
		{
			if (!Callback)
				return -1;

			int64_t Clock = GetClock();
			Sync.Timers.lock();

			EventId Id = Timer++;
			Timers.emplace_back(std::move(Callback), Milliseconds, Clock, Id, false);
			Sync.Timers.unlock();

			return Id;
		}
		EventId Schedule::SetListener(const std::string& Name, const EventCallback& Callback)
		{
			if (!Callback)
				return std::numeric_limits<uint64_t>::max();

			Sync.Listeners.lock();
			auto It = Listeners.find(Name);
			if (It != Listeners.end())
			{
				uint64_t Id = It->second.Counter++;
				It->second.Callbacks[Id] = Callback;
				Sync.Listeners.unlock();

				return Id;
			}

			EventListener& Src = Listeners[Name];
			Src.Callbacks[Src.Counter++] = Callback;
			Sync.Listeners.unlock();

			return 0;
		}
		EventId Schedule::SetListener(const std::string& Name, EventCallback&& Callback)
		{
			if (!Callback)
				return std::numeric_limits<uint64_t>::max();

			Sync.Listeners.lock();
			auto It = Listeners.find(Name);
			if (It != Listeners.end())
			{
				uint64_t Id = It->second.Counter++;
				It->second.Callbacks[Id] = std::move(Callback);
				Sync.Listeners.unlock();

				return Id;
			}

			EventListener& Src = Listeners[Name];
			Src.Callbacks[Src.Counter++] = std::move(Callback);
			Sync.Listeners.unlock();

			return 0;
		}
		bool Schedule::SetTask(const TaskCallback& Callback)
		{
			if (!Callback)
				return false;

			Sync.Tasks.lock();
			Tasks.emplace(Callback);
			Sync.Tasks.unlock();

			if (!Async.Childs.empty())
				Async.Condition.notify_one();

			return true;
		}
		bool Schedule::SetTask(TaskCallback&& Callback)
		{
			if (!Callback)
				return false;

			Sync.Tasks.lock();
			Tasks.emplace(std::move(Callback));
			Sync.Tasks.unlock();

			if (!Async.Childs.empty())
				Async.Condition.notify_one();

			return true;
		}
		bool Schedule::SetEvent(const std::string& Name, const VariantArgs& Args)
		{
			Sync.Events.lock();
			Events.emplace(Name, Args);
			Sync.Events.unlock();

			return true;
		}
		bool Schedule::SetEvent(const std::string& Name, VariantArgs&& Args)
		{
			Sync.Events.lock();
			Events.emplace(Name, std::move(Args));
			Sync.Events.unlock();

			return true;
		}
		bool Schedule::SetEvent(const std::string& Name)
		{
			Sync.Events.lock();
			Events.emplace(Name);
			Sync.Events.unlock();

			return true;
		}
		bool Schedule::ClearListener(const std::string& Name, EventId ListenerId)
		{
			Sync.Listeners.lock();
			auto It = Listeners.find(Name);
			if (It != Listeners.end())
			{
				auto Callback = It->second.Callbacks.find(ListenerId);
				if (Callback != It->second.Callbacks.end())
				{
					It->second.Callbacks.erase(Callback);
					Sync.Listeners.unlock();

					return true;
				}
			}

			Sync.Listeners.unlock();
			return false;
		}
		bool Schedule::ClearTimeout(EventId TimerId)
		{
			Sync.Timers.lock();
			for (auto It = Timers.begin(); It != Timers.end(); It++)
			{
				EventTimer& Value = *It;
				if (Value.Id == TimerId)
				{
					Timers.erase(It);
					Sync.Timers.unlock();
					return true;
				}
			}

			Sync.Timers.unlock();
			return false;
		}
		bool Schedule::Clear(EventType Type, bool NoCall)
		{
			if ((Type & EventType_Tasks) && !Tasks.empty())
			{
				Sync.Tasks.lock();
				while (!Tasks.empty())
				{
					EventTask Value(std::move(Tasks.front()));
					Tasks.pop();

					Sync.Tasks.unlock();
					if (!NoCall && Value)
						Value();

					return true;
				}
				Sync.Tasks.unlock();
			}

			if ((Type & EventType_Events) && !Events.empty())
			{
				Sync.Events.lock();
				while (!Events.empty())
				{
					EventBase Value(std::move(Events.front()));
					Events.pop();

					Sync.Events.unlock();
					if (NoCall)
						return true;

					Sync.Listeners.lock();
					auto Base = Listeners.find(Value.Name);
					if (Base == Listeners.end())
					{
						Sync.Listeners.unlock();
						return true;
					}

					auto Array = Base->second.Callbacks;
					Sync.Listeners.unlock();

					for (auto& Callback : Array)
					{
						if (Callback.second)
							Callback.second(Value.Args);
					}

					return true;
				}
				Sync.Events.unlock();
			}

			if ((Type & EventType_Timers) && !Timers.empty())
			{
				Sync.Timers.lock();
				for (auto It = Timers.begin(); It != Timers.end(); It++)
				{
					EventTimer Value(std::move(*It));
					Timers.erase(It);
					Sync.Timers.unlock();

					if (!NoCall && Value.Callback)
						Value.Callback();

					return true;
				}
				Sync.Timers.unlock();
			}

			return false;
		}
		bool Schedule::Start(bool IsAsync, uint64_t WorkersCount)
		{
			Async.Manage.lock();
			if (Active)
			{
				Async.Manage.unlock();
				return false;
			}

			Workers = WorkersCount;
			Async.Childs.reserve(Workers + (IsAsync ? 1 : 0));
			for (uint64_t i = 0; i < Workers; i++)
				Async.Childs.push_back(std::move(std::thread(&Schedule::LoopCycle, this)));

			Active = true;
			if (IsAsync)
				Async.Childs.push_back(std::move(std::thread(&Schedule::LoopIncome, this)));

			Async.Manage.unlock();
			return true;
		}
		bool Schedule::Dispatch()
		{
			if (!Tasks.empty())
				DispatchTask();

			if (!Events.empty())
				DispatchEvent();

			if (!Timers.empty())
				DispatchTimer(GetClock());

			return Active;
		}
		bool Schedule::Stop()
		{
			Async.Manage.lock();
			if (!Active && !Terminate)
			{
				Async.Manage.unlock();
				return false;
			}

			Active = false;
			Async.Condition.notify_all();
			for (auto& Thread : Async.Childs)
			{
				if (Thread.get_id() == std::this_thread::get_id())
				{
					Terminate = true;
					Async.Manage.unlock();
					return false;
				}

				if (Thread.joinable())
					Thread.join();
			}

			Async.Childs.clear();
			while (!Tasks.empty() || !Events.empty() || !Timers.empty())
				Clear(EventType_Tasks | EventType_Events | EventType_Timers, false);

			Sync.Tasks.lock();
			std::queue<EventTask>().swap(Tasks);
			Sync.Tasks.unlock();

			Sync.Events.lock();
			std::queue<EventBase>().swap(Events);
			Sync.Events.unlock();

			Sync.Listeners.lock();
			Listeners.clear();
			Sync.Listeners.unlock();

			Sync.Timers.lock();
			Timers.clear();
			Sync.Timers.unlock();

			Terminate = false;
			Workers = Timer = 0;
			Async.Manage.unlock();

			return true;
		}
		bool Schedule::LoopIncome()
		{
			if (!Async.Childs.empty())
				Async.Condition.notify_all();

			while (Active)
			{
				bool Overhead = true;
				if (!Workers && !Tasks.empty())
					Overhead = (DispatchTask() ? false : Overhead);

				if (!Events.empty())
					Overhead = (DispatchEvent() ? false : Overhead);

				if (!Timers.empty())
					Overhead = (DispatchTimer(GetClock()) ? false : Overhead);

				if (Overhead)
					std::this_thread::sleep_for(std::chrono::microseconds(100));
			}

			return true;
		}
		bool Schedule::LoopCycle()
		{
			if (!Active)
				goto Wait;

			do
			{
				if (!DispatchTask())
				{
				Wait:
					std::unique_lock<std::mutex> Lock(Async.Child);
					Async.Condition.wait(Lock);
				}
			} while (Active);

			return true;
		}
		bool Schedule::DispatchEvent()
		{
			if (Events.empty())
				return false;

			Sync.Events.lock();
			if (Events.empty())
			{
				Sync.Events.unlock();
				return false;
			}

			EventBase Src(std::move(Events.front()));
			Events.pop();
			Sync.Events.unlock();

			Sync.Listeners.lock();
			auto Base = Listeners.find(Src.Name);
			if (Base == Listeners.end())
			{
				Sync.Listeners.unlock();
				return false;
			}

			auto Array = Base->second.Callbacks;
			Sync.Listeners.unlock();

			return SetTask([Src = std::move(Src), Array]() mutable
			{
				for (auto& Callback : Array)
				{
					if (Callback.second)
						Callback.second(Src.Args);
				}
			});
		}
		bool Schedule::DispatchTask()
		{
			if (Tasks.empty())
				return !Active;

			Sync.Tasks.lock();
			if (Tasks.empty())
			{
				Sync.Tasks.unlock();
				return !Active;
			}

			EventTask Src(std::move(Tasks.front()));
			Tasks.pop();

			Sync.Tasks.unlock();
			if (Src)
				Src();

			return true;
		}
		bool Schedule::DispatchTimer(int64_t Time)
		{
			Sync.Timers.lock();
			for (auto It = Timers.begin(); It != Timers.end(); It++)
			{
				EventTimer& Element = *It;
				if (Time - Element.Time < (int64_t)Element.Timeout)
					continue;

				TimerCallback Callback = Element.Callback;
				if (Element.Alive)
					Element.Time = Time;
				else
					Timers.erase(It);

				Sync.Timers.unlock();
				if (Callback)
					SetTask(std::move(Callback));

				return true;
			}

			Sync.Timers.unlock();
			return false;
		}
		bool Schedule::IsBlockable()
		{
			return Active && Workers > 1;
		}
		bool Schedule::IsActive()
		{
			return Active;
		}
		int64_t Schedule::GetClock()
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		}
		Schedule* Schedule::Get()
		{
			if (Singleton == nullptr)
				Singleton = new Schedule();

			return Singleton;
		}
		Schedule* Schedule::Singleton = nullptr;

		Document::Document(const Variant& Base) : Parent(nullptr), Saved(true), Value(Base)
		{
		}
		Document::Document(Variant&& Base) : Parent(nullptr), Saved(true), Value(std::move(Base))
		{
		}
		Document::~Document()
		{
			if (Parent != nullptr)
			{
				for (auto It = Parent->Nodes.begin(); It != Parent->Nodes.end(); It++)
				{
					if (*It != this)
						continue;

					Parent->Nodes.erase(It);
					break;
				}

				Parent = nullptr;
			}

			Clear();
		}
		std::unordered_map<std::string, uint64_t> Document::GetNames() const
		{
			std::unordered_map<std::string, uint64_t> Mapping;
			uint64_t Index = 0;

			ProcessNames(this, &Mapping, Index);
			return Mapping;
		}
		std::vector<Document*> Document::FindCollection(const std::string& Name, bool Deep) const
		{
			std::vector<Document*> Result;
			for (auto Value : Nodes)
			{
				if (Value->Key == Name)
					Result.push_back(Value);

				if (!Deep)
					continue;

				std::vector<Document*> New = Value->FindCollection(Name);
				for (auto& Subvalue : New)
					Result.push_back(Subvalue);
			}

			return Result;
		}
		std::vector<Document*> Document::FetchCollection(const std::string& Notation, bool Deep) const
		{
			std::vector<std::string> Names = Parser(Notation).Split('.');
			if (Names.empty())
				return std::vector<Document*>();

			if (Names.size() == 1)
				return FindCollection(*Names.begin());

			Document* Current = Find(*Names.begin(), Deep);
			if (!Current)
				return std::vector<Document*>();

			for (auto It = Names.begin() + 1; It != Names.end() - 1; It++)
			{
				Current = Current->Find(*It, Deep);
				if (!Current)
					return std::vector<Document*>();
			}

			return Current->FindCollection(*(Names.end() - 1), Deep);
		}
		std::vector<Document*> Document::GetAttributes() const
		{
			std::vector<Document*> Attributes;
			for (auto It : Nodes)
			{
				if (It->IsAttribute())
					Attributes.push_back(It);
			}

			return Attributes;
		}
		std::vector<Document*>* Document::GetNodes()
		{
			return &Nodes;
		}
		Document* Document::Find(const std::string& Name, bool Deep) const
		{
			if (Value.Type == VarType_Array)
			{
				Core::Parser Number(&Name);
				if (Number.HasInteger())
				{
					int64_t Index = Number.ToInt64();
					if (Index >= 0 && Index < Nodes.size())
						return Nodes[Index];
				}
			}

			for (auto K : Nodes)
			{
				if (K->Key == Name)
					return K;

				if (!Deep)
					continue;

				Document* V = K->Find(Name);
				if (V != nullptr)
					return V;
			}

			return nullptr;
		}
		Document* Document::Fetch(const std::string& Notation, bool Deep) const
		{
			std::vector<std::string> Names = Parser(Notation).Split('.');
			if (Names.empty())
				return nullptr;

			Document* Current = Find(*Names.begin(), Deep);
			if (!Current)
				return nullptr;

			for (auto It = Names.begin() + 1; It != Names.end(); It++)
			{
				Current = Current->Find(*It, Deep);
				if (!Current)
					return nullptr;
			}

			return Current;
		}
		Document* Document::GetParent() const
		{
			return Parent;
		}
		Document* Document::GetAttribute(const std::string& Name) const
		{
			return Get("[" + Name + "]");
		}
		Variant Document::GetVar(size_t Index) const
		{
			Document* Result = Get(Index);
			if (!Result)
				return Var::Undefined();

			return Result->Value;
		}
		Variant Document::GetVar(const std::string& Key) const
		{
			Document* Result = Get(Key);
			if (!Result)
				return Var::Undefined();

			return Result->Value;
		}
		Document* Document::Get(size_t Index) const
		{
			if (Index < 0 || Index >= Nodes.size())
				return nullptr;

			return Nodes[Index];
		}
		Document* Document::Get(const std::string& Name) const
		{
			if (Name.empty())
				return nullptr;

			for (auto Document : Nodes)
			{
				if (Document->Key == Name)
					return Document;
			}

			return nullptr;
		}
		Document* Document::Set(const std::string& Name)
		{
			return Set(Name, std::move(Var::Object()));
		}
		Document* Document::Set(const std::string& Name, const Variant& Base)
		{
			Saved = false;
			if (Value.Type == VarType_Object)
			{
				for (auto Node : Nodes)
				{
					if (Node->Key == Name)
					{
						Node->Value = Base;
						Node->Saved = false;
						Node->Nodes.clear();

						return Node;
					}
				}
			}

			Document* Result = new Document(Base);
			Result->Key.assign(Name);
			Result->Saved = false;
			Result->Parent = this;

			Nodes.push_back(Result);
			return Result;
		}
		Document* Document::Set(const std::string& Name, Variant&& Base)
		{
			Saved = false;
			if (Value.Type == VarType_Object)
			{
				for (auto Node : Nodes)
				{
					if (Node->Key == Name)
					{
						Node->Value = std::move(Base);
						Node->Saved = false;
						Node->Nodes.clear();

						return Node;
					}
				}
			}

			Document* Result = new Document(std::move(Base));
			Result->Key.assign(Name);
			Result->Saved = false;
			Result->Parent = this;

			Nodes.push_back(Result);
			return Result;
		}
		Document* Document::Set(const std::string& Name, Document* Base)
		{
			if (!Base)
				return Set(Name, std::move(Var::Null()));

			Base->Key.assign(Name);
			Base->Saved = false;
			Base->Parent = this;
			Saved = false;

			if (Value.Type == VarType_Object)
			{
				for (auto It = Nodes.begin(); It != Nodes.end(); It++)
				{
					if ((*It)->Key != Name)
						continue;

					if (*It == Base)
						return Base;

					(*It)->Parent = nullptr;
					TH_RELEASE(*It);
					*It = Base;

					return Base;
				}
			}

			Nodes.push_back(Base);
			return Base;
		}
		Document* Document::SetAttribute(const std::string& Name, const Variant& Value)
		{
			return Set("[" + Name + "]", Value);
		}
		Document* Document::SetAttribute(const std::string& Name, Variant&& Value)
		{
			return Set("[" + Name + "]", std::move(Value));
		}
		Document* Document::Push(const Variant& Base)
		{
			Document* Result = new Document(Base);
			Result->Saved = false;
			Result->Parent = this;
			Saved = false;

			Nodes.push_back(Result);
			return Result;
		}
		Document* Document::Push(Variant&& Base)
		{
			Document* Result = new Document(std::move(Base));
			Result->Saved = false;
			Result->Parent = this;
			Saved = false;

			Nodes.push_back(Result);
			return Result;
		}
		Document* Document::Push(Document* Base)
		{
			if (!Base)
				return Push(std::move(Var::Null()));

			Base->Saved = false;
			Base->Parent = this;
			Saved = false;

			Nodes.push_back(Base);
			return Base;
		}
		Document* Document::Pop(size_t Index)
		{
			if (Index < 0 || Index >= Nodes.size())
				return nullptr;

			Document* Base = Nodes[Index];
			Base->Parent = nullptr;
			TH_RELEASE(Base);
			Nodes.erase(Nodes.begin() + Index);

			return this;
		}
		Document* Document::Pop(const std::string& Name)
		{
			for (auto It = Nodes.begin(); It != Nodes.end(); It++)
			{
				if (!*It || (*It)->Key != Name)
					continue;

				(*It)->Parent = nullptr;
				TH_RELEASE(*It);
				Nodes.erase(It);
				break;
			}

			return this;
		}
		Document* Document::Copy() const
		{
			Document* New = new Document(Value);
			New->Parent = nullptr;
			New->Key.assign(Key);
			New->Saved = Saved;
			New->Nodes = Nodes;

			for (auto It = New->Nodes.begin(); It != New->Nodes.end(); It++)
			{
				if (*It != nullptr)
					*It = (*It)->Copy();
			}

			return New;
		}
		bool Document::Has(const std::string& Name) const
		{
			return Fetch(Name) != nullptr;
		}
		bool Document::Has64(const std::string& Name, size_t Size) const
		{
			Document* Base = Fetch(Name);
			if (!Base || Base->Value.GetType() != VarType_Base64)
				return false;

			return Base->Value.GetSize() == Size;
		}
		bool Document::IsAttribute() const
		{
			if (Key.size() < 2)
				return false;

			return (Key.front() == '[' && Key.back() == ']');
		}
		bool Document::IsSaved() const
		{
			return Saved;
		}
		int64_t Document::Size() const
		{
			return (int64_t)Nodes.size();
		}
		std::string Document::GetName() const
		{
			return IsAttribute() ? Key.substr(1, Key.size() - 2) : Key;
		}
		void Document::Join(Document* Other)
		{
			if (!Other)
				return;

			for (auto& Node : Other->Nodes)
			{
				Document* Copy = Node->Copy();
				Copy->Saved = false;
				Copy->Parent = this;
				Saved = false;

				if (Value.Type == VarType_Array)
				{
					Nodes.push_back(Copy);
					continue;
				}

				bool Exists = false;
				for (auto It = Nodes.begin(); It != Nodes.end(); It++)
				{
					if (!*It || (*It)->Key != Copy->Key)
						continue;

					(*It)->Parent = nullptr;
					TH_RELEASE(*It);
					*It = Copy;
					Exists = true;
					break;
				}

				if (!Exists)
					Nodes.push_back(Copy);
			}
		}
		void Document::Clear()
		{
			for (auto& Document : Nodes)
			{
				if (Document != nullptr)
				{
					Document->Parent = nullptr;
					TH_RELEASE(Document);
				}
			}

			Nodes.clear();
		}
		void Document::Save()
		{
			for (auto& It : Nodes)
			{
				if (It->Value.IsObject())
					It->Save();
				else
					It->Saved = true;
			}

			Saved = true;
		}
		Document* Document::Object()
		{
			return new Document(std::move(Var::Object()));
		}
		Document* Document::Array()
		{
			return new Document(std::move(Var::Array()));
		}
		bool Document::WriteXML(Document* Base, const NWriteCallback& Callback)
		{
			if (!Base || !Callback)
				return false;

			std::vector<Document*> Attributes = Base->GetAttributes();
			bool Scalable = (Base->Value.GetSize() || ((int64_t)Base->Nodes.size() - (int64_t)Attributes.size()) > 0);
			Callback(VarForm_Write_Tab, "", 0);
			Callback(VarForm_Dummy, "<", 1);
			Callback(VarForm_Dummy, Base->Key.c_str(), (int64_t)Base->Key.size());

			if (Attributes.empty())
			{
				if (Scalable)
					Callback(VarForm_Dummy, ">", 1);
				else
					Callback(VarForm_Dummy, " />", 3);
			}
			else
				Callback(VarForm_Dummy, " ", 1);

			for (auto It = Attributes.begin(); It != Attributes.end(); It++)
			{
				std::string Key = (*It)->GetName();
				std::string Value = (*It)->Value.Serialize();

				Callback(VarForm_Dummy, Key.c_str(), (int64_t)Key.size());
				Callback(VarForm_Dummy, "=\"", 2);
				Callback(VarForm_Dummy, Value.c_str(), (int64_t)Value.size());
				It++;

				if (It == Attributes.end())
				{
					if (!Scalable)
					{
						Callback(VarForm_Write_Space, "\"", 1);
						Callback(VarForm_Dummy, "/>", 2);
					}
					else
						Callback(VarForm_Dummy, "\">", 2);
				}
				else
					Callback(VarForm_Write_Space, "\"", 1);

				It--;
			}

			Callback(VarForm_Tab_Increase, "", 0);
			if (Base->Value.GetSize() > 0)
			{
				std::string Text = Base->Value.Serialize();
				if (!Base->Nodes.empty())
				{
					Callback(VarForm_Write_Line, "", 0);
					Callback(VarForm_Write_Tab, "", 0);
					Callback(VarForm_Dummy, Text.c_str(), Text.size());
					Callback(VarForm_Write_Line, "", 0);
				}
				else
					Callback(VarForm_Dummy, Text.c_str(), Text.size());
			}
			else
				Callback(VarForm_Write_Line, "", 0);

			for (auto&& It : Base->Nodes)
			{
				if (!It->IsAttribute())
					WriteXML(It, Callback);
			}

			Callback(VarForm_Tab_Decrease, "", 0);
			if (!Scalable)
				return true;

			if (!Base->Nodes.empty())
				Callback(VarForm_Write_Tab, "", 0);

			Callback(VarForm_Dummy, "</", 2);
			Callback(VarForm_Dummy, Base->Key.c_str(), (int64_t)Base->Key.size());
			Callback(Base->Parent ? VarForm_Write_Line : VarForm_Dummy, ">", 1);

			return true;
		}
		bool Document::WriteJSON(Document* Base, const NWriteCallback& Callback)
		{
			if (!Base || !Callback)
				return false;

			if (!Base->Parent && !Base->Value.IsObject())
			{
				std::string Value = Base->Value.Serialize();
				Core::Parser Safe(&Value);
				Safe.Escape();

				if (Base->Value.Type != VarType_String && Base->Value.Type != VarType_Base64)
				{
					if (!Value.empty() && Value.front() == TH_PREFIX_CHAR)
						Callback(VarForm_Dummy, Value.c_str() + 1, (int64_t)Value.size() - 1);
					else
						Callback(VarForm_Dummy, Value.c_str(), (int64_t)Value.size());
				}
				else
				{
					Callback(VarForm_Dummy, "\"", 1);
					Callback(VarForm_Dummy, Value.c_str(), (int64_t)Value.size());
					Callback(VarForm_Dummy, "\"", 1);
				}

				return true;
			}

			size_t Size = Base->Nodes.size(), Offset = 0;
			bool Array = (Base->Value.Type == VarType_Array);

			if (Base->Parent != nullptr)
				Callback(VarForm_Write_Line, "", 0);

			Callback(VarForm_Write_Tab, "", 0);
			Callback(VarForm_Dummy, Array ? "[" : "{", 1);
			Callback(VarForm_Tab_Increase, "", 0);

			for (auto&& Document : Base->Nodes)
			{
				if (!Array)
				{
					Callback(VarForm_Write_Line, "", 0);
					Callback(VarForm_Write_Tab, "", 0);
					Callback(VarForm_Dummy, "\"", 1);
					Callback(VarForm_Dummy, Document->Key.c_str(), (int64_t)Document->Key.size());
					Callback(VarForm_Write_Space, "\":", 2);
				}

				if (!Document->Value.IsObject())
				{
					std::string Value = Document->Value.Serialize();
					Core::Parser Safe(&Value);
					Safe.Escape();

					if (Array)
					{
						Callback(VarForm_Write_Line, "", 0);
						Callback(VarForm_Write_Tab, "", 0);
					}

					if (!Document->Value.IsObject() && Document->Value.Type != VarType_String && Document->Value.Type != VarType_Base64)
					{
						if (!Value.empty() && Value.front() == TH_PREFIX_CHAR)
							Callback(VarForm_Dummy, Value.c_str() + 1, (int64_t)Value.size() - 1);
						else
							Callback(VarForm_Dummy, Value.c_str(), (int64_t)Value.size());
					}
					else
					{
						Callback(VarForm_Dummy, "\"", 1);
						Callback(VarForm_Dummy, Value.c_str(), (int64_t)Value.size());
						Callback(VarForm_Dummy, "\"", 1);
					}
				}
				else
					WriteJSON(Document, Callback);

				Offset++;
				if (Offset < Size)
					Callback(VarForm_Dummy, ",", 1);
			}

			Callback(VarForm_Tab_Decrease, "", 0);
			Callback(VarForm_Write_Line, "", 0);

			if (Base->Parent != nullptr)
				Callback(VarForm_Write_Tab, "", 0);

			Callback(VarForm_Dummy, Array ? "]" : "}", 1);
			return true;
		}
		bool Document::WriteJSONB(Document* Base, const NWriteCallback& Callback)
		{
			if (!Base || !Callback)
				return false;

			std::unordered_map<std::string, uint64_t> Mapping = Base->GetNames();
			uint64_t Set = (uint64_t)Mapping.size();

			Callback(VarForm_Dummy, "\0b\0i\0n\0h\0e\0a\0d\r\n", sizeof(char) * 16);
			Callback(VarForm_Dummy, (const char*)&Set, sizeof(uint64_t));

			for (auto It = Mapping.begin(); It != Mapping.end(); It++)
			{
				uint64_t Size = (uint64_t)It->first.size();
				Callback(VarForm_Dummy, (const char*)&It->second, sizeof(uint64_t));
				Callback(VarForm_Dummy, (const char*)&Size, sizeof(uint64_t));

				if (Size > 0)
					Callback(VarForm_Dummy, It->first.c_str(), sizeof(char) * Size);
			}

			ProcessJSONBWrite(Base, &Mapping, Callback);
			return true;
		}
		Document* Document::ReadXML(int64_t Size, const NReadCallback& Callback, bool Assert)
		{
			if (!Callback || !Size)
				return nullptr;

			std::string Buffer;
			Buffer.resize(Size);
			if (!Callback((char*)Buffer.c_str(), sizeof(char) * Size))
			{
				if (Assert)
					TH_ERROR("cannot read xml document");

				return nullptr;
			}

			auto iDocument = new rapidxml::xml_document<>();
			try
			{
				iDocument->parse<rapidxml::parse_trim_whitespace>((char*)Buffer.c_str());
			}
			catch (const std::runtime_error& e)
			{
				delete iDocument;
				if (Assert)
					TH_ERROR("[xml] %s", e.what());

				return nullptr;
			}
			catch (const rapidxml::parse_error& e)
			{
				delete iDocument;
				if (Assert)
					TH_ERROR("[xml] %s", e.what());

				return nullptr;
			}
			catch (const std::exception& e)
			{
				delete iDocument;
				if (Assert)
					TH_ERROR("[xml] %s", e.what());

				return nullptr;
			}
			catch (...)
			{
				delete iDocument;
				if (Assert)
					TH_ERROR("[xml] parse error");

				return nullptr;
			}

			rapidxml::xml_node<>* Base = iDocument->first_node();
			if (!Base)
			{
				iDocument->clear();
				delete iDocument;

				return nullptr;
			}

			Document* Result = Document::Array();
			Result->Key = Base->name();

			if (!ProcessXMLRead((void*)Base, Result))
				TH_CLEAR(Result);

			iDocument->clear();
			delete iDocument;

			return Result;
		}
		Document* Document::ReadJSON(int64_t Size, const NReadCallback& Callback, bool Assert)
		{
			if (!Callback || !Size)
				return nullptr;

			std::string Buffer;
			Buffer.resize(Size);
			if (!Callback((char*)Buffer.c_str(), sizeof(char) * Size))
			{
				if (Assert)
					TH_ERROR("cannot read json document");

				return nullptr;
			}

			rapidjson::Document Base;
			Base.Parse(Buffer.c_str(), Buffer.size());

			Core::Document* Result = nullptr;
			if (Base.HasParseError())
			{
				if (!Assert)
					return nullptr;

				int Offset = (int)Base.GetErrorOffset();
				switch (Base.GetParseError())
				{
					case rapidjson::kParseErrorDocumentEmpty:
						TH_ERROR("[json:%i] the document is empty", Offset);
						break;
					case rapidjson::kParseErrorDocumentRootNotSingular:
						TH_ERROR("[json:%i] the document root must not follow by other values", Offset);
						break;
					case rapidjson::kParseErrorValueInvalid:
						TH_ERROR("[json:%i] invalid value", Offset);
						break;
					case rapidjson::kParseErrorObjectMissName:
						TH_ERROR("[json:%i] missing a name for object member", Offset);
						break;
					case rapidjson::kParseErrorObjectMissColon:
						TH_ERROR("[json:%i] missing a colon after a name of object member", Offset);
						break;
					case rapidjson::kParseErrorObjectMissCommaOrCurlyBracket:
						TH_ERROR("[json:%i] missing a comma or '}' after an object member", Offset);
						break;
					case rapidjson::kParseErrorArrayMissCommaOrSquareBracket:
						TH_ERROR("[json:%i] missing a comma or ']' after an array element", Offset);
						break;
					case rapidjson::kParseErrorStringUnicodeEscapeInvalidHex:
						TH_ERROR("[json:%i] incorrect hex digit after \\u escape in string", Offset);
						break;
					case rapidjson::kParseErrorStringUnicodeSurrogateInvalid:
						TH_ERROR("[json:%i] the surrogate pair in string is invalid", Offset);
						break;
					case rapidjson::kParseErrorStringEscapeInvalid:
						TH_ERROR("[json:%i] invalid escape character in string", Offset);
						break;
					case rapidjson::kParseErrorStringMissQuotationMark:
						TH_ERROR("[json:%i] missing a closing quotation mark in string", Offset);
						break;
					case rapidjson::kParseErrorStringInvalidEncoding:
						TH_ERROR("[json:%i] invalid encoding in string", Offset);
						break;
					case rapidjson::kParseErrorNumberTooBig:
						TH_ERROR("[json:%i] number too big to be stored in double", Offset);
						break;
					case rapidjson::kParseErrorNumberMissFraction:
						TH_ERROR("[json:%i] miss fraction part in number", Offset);
						break;
					case rapidjson::kParseErrorNumberMissExponent:
						TH_ERROR("[json:%i] miss exponent in number", Offset);
						break;
					case rapidjson::kParseErrorTermination:
						TH_ERROR("[json:%i] parsing was terminated", Offset);
						break;
					case rapidjson::kParseErrorUnspecificSyntaxError:
						TH_ERROR("[json:%i] unspecific syntax error", Offset);
						break;
					default:
						break;
				}

				return nullptr;
			}

			rapidjson::Type Type = Base.GetType();
			switch (Type)
			{
				case rapidjson::kNullType:
					Result = new Document(std::move(Var::Null()));
					break;
				case rapidjson::kFalseType:
					Result = new Document(std::move(Var::Boolean(false)));
					break;
				case rapidjson::kTrueType:
					Result = new Document(std::move(Var::Boolean(true)));
					break;
				case rapidjson::kObjectType:
					Result = Document::Object();
					if (!ProcessJSONRead((void*)&Base, Result))
						TH_CLEAR(Result);
					break;
				case rapidjson::kArrayType:
					Result = Document::Array();
					if (!ProcessJSONRead((void*)&Base, Result))
						TH_CLEAR(Result);
					break;
				case rapidjson::kStringType:
					Result = new Document(std::move(Var::Auto(std::string(Base.GetString(), Base.GetStringLength()), true)));
					break;
				case rapidjson::kNumberType:
					if (Base.IsInt())
						Result = new Document(std::move(Var::Integer(Base.GetInt64())));
					else
						Result = new Document(std::move(Var::Number(Base.GetDouble())));
					break;
				default:
					Result = new Document(std::move(Var::Undefined()));
					break;
			}

			if (Result != nullptr && Result->Key.empty())
				Result->Key.assign("document");

			return Result;
		}
		Document* Document::ReadJSONB(const NReadCallback& Callback, bool Assert)
		{
			if (!Callback)
				return nullptr;

			char Hello[18];
			if (!Callback((char*)Hello, sizeof(char) * 16))
			{
				if (Assert)
					TH_ERROR("form cannot be defined");

				return nullptr;
			}

			if (memcmp((void*)Hello, (void*)"\0b\0i\0n\0h\0e\0a\0d\r\n", sizeof(char) * 16) != 0)
			{
				if (Assert)
					TH_ERROR("version is undefined");

				return nullptr;
			}

			uint64_t Set = 0;
			if (!Callback((char*)&Set, sizeof(uint64_t)))
			{
				if (Assert)
					TH_ERROR("name map is undefined");

				return nullptr;
			}

			std::unordered_map<uint64_t, std::string> Map;
			for (uint64_t i = 0; i < Set; i++)
			{
				uint64_t Index = 0;
				if (!Callback((char*)&Index, sizeof(uint64_t)))
				{
					if (Assert)
						TH_ERROR("name index is undefined");

					return nullptr;
				}

				uint64_t Size = 0;
				if (!Callback((char*)&Size, sizeof(uint64_t)))
				{
					if (Assert)
						TH_ERROR("name size is undefined");

					return nullptr;
				}

				if (Size <= 0)
					continue;

				std::string Name;
				Name.resize(Size);
				if (!Callback((char*)Name.c_str(), sizeof(char) * Size))
				{
					if (Assert)
						TH_ERROR("name data is undefined");

					return nullptr;
				}

				Map.insert({ Index, Name });
			}

			Document* Current = Document::Object();
			if (!ProcessJSONBRead(Current, &Map, Callback))
			{
				TH_RELEASE(Current);
				return nullptr;
			}

			return Current;
		}
		bool Document::ProcessXMLRead(void* Base, Document* Current)
		{
			auto Ref = (rapidxml::xml_node<>*)Base;
			if (!Ref || !Current)
				return false;

			for (rapidxml::xml_attribute<>* It = Ref->first_attribute(); It; It = It->next_attribute())
			{
				if (It->name()[0] != '\0')
					Current->SetAttribute(It->name(), std::move(Var::Auto(It->value())));
			}

			for (rapidxml::xml_node<>* It = Ref->first_node(); It; It = It->next_sibling())
			{
				if (It->name()[0] == '\0')
					continue;

				Document* Subresult = Current->Set(It->name(), Document::Array());
				ProcessXMLRead((void*)It, Subresult);

				if (Subresult->Nodes.empty() && It->value_size() > 0)
					Subresult->Value.Deserialize(std::string(It->value(), It->value_size()));
			}

			return true;
		}
		bool Document::ProcessJSONRead(void* Base, Document* Current)
		{
			auto Ref = (rapidjson::Value*)Base;
			if (!Ref || !Current)
				return false;

			std::string Value;
			if (!Ref->IsArray())
			{
				VarType Type = Current->Value.Type;
				Current->Value.Type = VarType_Array;

				std::string Name;
				for (auto It = Ref->MemberBegin(); It != Ref->MemberEnd(); It++)
				{
					if (!It->name.IsString())
						continue;

					Name.assign(It->name.GetString(), (size_t)It->name.GetStringLength());
					switch (It->value.GetType())
					{
						case rapidjson::kNullType:
							Current->Set(Name, std::move(Var::Null()));
							break;
						case rapidjson::kFalseType:
							Current->Set(Name, std::move(Var::Boolean(false)));
							break;
						case rapidjson::kTrueType:
							Current->Set(Name, std::move(Var::Boolean(true)));
							break;
						case rapidjson::kObjectType:
							ProcessJSONRead((void*)&It->value, Current->Set(Name));
							break;
						case rapidjson::kArrayType:
							ProcessJSONRead((void*)&It->value, Current->Set(Name, std::move(Var::Array())));
							break;
						case rapidjson::kStringType:
							Value.assign(It->value.GetString(), It->value.GetStringLength());
							Current->Set(Name, std::move(Var::Auto(Value, true)));
							break;
						case rapidjson::kNumberType:
							if (It->value.IsInt())
								Current->Set(Name, std::move(Var::Integer(It->value.GetInt64())));
							else
								Current->Set(Name, std::move(Var::Number(It->value.GetDouble())));
							break;
						default:
							break;
					}
				}

				Current->Value.Type = Type;
			}
			else
			{
				for (auto It = Ref->Begin(); It != Ref->End(); It++)
				{
					switch (It->GetType())
					{
						case rapidjson::kNullType:
							Current->Push(std::move(Var::Null()));
							break;
						case rapidjson::kFalseType:
							Current->Push(std::move(Var::Boolean(false)));
							break;
						case rapidjson::kTrueType:
							Current->Push(std::move(Var::Boolean(true)));
							break;
						case rapidjson::kObjectType:
							ProcessJSONRead((void*)It, Current->Push(std::move(Var::Object())));
							break;
						case rapidjson::kArrayType:
							ProcessJSONRead((void*)It, Current->Push(std::move(Var::Array())));
							break;
						case rapidjson::kStringType:
							Value.assign(It->GetString(), It->GetStringLength());
							Current->Push(std::move(Var::Auto(Value, true)));
							break;
						case rapidjson::kNumberType:
							if (It->IsInt())
								Current->Push(std::move(Var::Integer(It->GetInt64())));
							else
								Current->Push(std::move(Var::Number(It->GetDouble())));
							break;
						default:
							break;
					}
				}
			}

			return true;
		}
		bool Document::ProcessJSONBWrite(Document* Current, std::unordered_map<std::string, uint64_t>* Map, const NWriteCallback& Callback)
		{
			uint64_t Id = Map->at(Current->Key);
			Callback(VarForm_Dummy, (const char*)&Id, sizeof(uint64_t));
			Callback(VarForm_Dummy, (const char*)&Current->Value.Type, sizeof(VarType));

			switch (Current->Value.Type)
			{
				case VarType_Object:
				case VarType_Array:
				{
					uint64_t Count = Current->Nodes.size();
					Callback(VarForm_Dummy, (const char*)&Count, sizeof(uint64_t));
					for (auto& Document : Current->Nodes)
						ProcessJSONBWrite(Document, Map, Callback);
					break;
				}
				case VarType_String:
				case VarType_Base64:
				case VarType_Decimal:
				{
					uint64_t Size = Current->Value.GetSize();
					Callback(VarForm_Dummy, (const char*)&Size, sizeof(uint64_t));
					Callback(VarForm_Dummy, Current->Value.GetString(), Size * sizeof(char));
					break;
				}
				case VarType_Integer:
				{
					int64_t Copy = Current->Value.GetInteger();
					Callback(VarForm_Dummy, (const char*)&Copy, sizeof(int64_t));
					break;
				}
				case VarType_Number:
				{
					double Copy = Current->Value.GetNumber();
					Callback(VarForm_Dummy, (const char*)&Copy, sizeof(double));
					break;
				}
				case VarType_Boolean:
				{
					bool Copy = Current->Value.GetBoolean();
					Callback(VarForm_Dummy, (const char*)&Copy, sizeof(bool));
					break;
				}
			}

			return true;
		}
		bool Document::ProcessJSONBRead(Document* Current, std::unordered_map<uint64_t, std::string>* Map, const NReadCallback& Callback)
		{
			uint64_t Id = 0;
			if (!Callback((char*)&Id, sizeof(uint64_t)))
			{
				TH_ERROR("key name index is undefined");
				return false;
			}

			auto It = Map->find(Id);
			if (It != Map->end())
				Current->Key = It->second;

			if (!Callback((char*)&Current->Value.Type, sizeof(VarType)))
			{
				TH_ERROR("key type is undefined");
				return false;
			}

			switch (Current->Value.Type)
			{
				case VarType_Object:
				case VarType_Array:
				{
					uint64_t Count = 0;
					if (!Callback((char*)&Count, sizeof(uint64_t)))
					{
						TH_ERROR("key value size is undefined");
						return false;
					}

					if (!Count)
						break;

					Current->Nodes.resize(Count);
					for (auto K = Current->Nodes.begin(); K != Current->Nodes.end(); K++)
					{
						*K = Document::Object();
						(*K)->Parent = Current;
						(*K)->Saved = true;

						ProcessJSONBRead(*K, Map, Callback);
					}
					break;
				}
				case VarType_String:
				{
					uint64_t Size = 0;
					if (!Callback((char*)&Size, sizeof(uint64_t)))
					{
						TH_ERROR("key value size is undefined");
						return false;
					}

					std::string Buffer;
					Buffer.resize(Size);

					if (!Callback((char*)Buffer.c_str(), Size * sizeof(char)))
					{
						TH_ERROR("key value data is undefined");
						return false;
					}

					Current->Value = std::move(Var::String(Buffer));
					break;
				}
				case VarType_Base64:
				{
					uint64_t Size = 0;
					if (!Callback((char*)&Size, sizeof(uint64_t)))
					{
						TH_ERROR("key value size is undefined");
						return false;
					}

					std::string Buffer;
					Buffer.resize(Size);

					if (!Callback((char*)Buffer.c_str(), Size * sizeof(char)))
					{
						TH_ERROR("key value data is undefined");
						return false;
					}

					Current->Value = std::move(Var::Base64(Buffer));
					break;
				}
				case VarType_Integer:
				{
					int64_t Integer = 0;
					if (!Callback((char*)&Integer, sizeof(int64_t)))
					{
						TH_ERROR("key value is undefined");
						return false;
					}

					Current->Value = std::move(Var::Integer(Integer));
					break;
				}
				case VarType_Number:
				{
					double Number = 0.0;
					if (!Callback((char*)&Number, sizeof(double)))
					{
						TH_ERROR("key value is undefined");
						return false;
					}

					Current->Value = std::move(Var::Number(Number));
					break;
				}
				case VarType_Decimal:
				{
					uint64_t Size = 0;
					if (!Callback((char*)&Size, sizeof(uint64_t)))
					{
						TH_ERROR("key value size is undefined");
						return false;
					}

					std::string Buffer;
					Buffer.resize(Size);

					if (!Callback((char*)Buffer.c_str(), Size * sizeof(char)))
					{
						TH_ERROR("key value data is undefined");
						return false;
					}

					Current->Value = std::move(Var::Decimal(Buffer));
					break;
				}
				case VarType_Boolean:
				{
					bool Boolean = false;
					if (!Callback((char*)&Boolean, sizeof(bool)))
					{
						TH_ERROR("key value is undefined");
						return false;
					}

					Current->Value = std::move(Var::Boolean(Boolean));
					break;
				}
			}

			return true;
		}
		bool Document::ProcessNames(const Document* Current, std::unordered_map<std::string, uint64_t>* Map, uint64_t& Index)
		{
			auto M = Map->find(Current->Key);
			if (M == Map->end())
				Map->insert({ Current->Key, Index++ });

			for (auto Document : Current->Nodes)
				ProcessNames(Document, Map, Index);

			return true;
		}

		Parser Form(const char* Format, ...)
		{
			char Buffer[16384];
			if (!Format)
				return Parser();

			va_list Args;
			va_start(Args, Format);
			int Size = vsnprintf(Buffer, sizeof(Buffer), Format, Args);
			va_end(Args);

			return Parser(Buffer, Size > 16384 ? 16384 : (size_t)Size);
		}
	}
}
