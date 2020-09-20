#include "rest.h"
#include "../network/bson.h"
#include <cctype>
#include <ctime>
#include <functional>
#include <iostream>
#include <csignal>
#include <sys/stat.h>
#include <rapidxml.hpp>
#include <tinyfiledialogs.h>
#ifdef THAWK_MICROSOFT
#include <Windows.h>
#include <io.h>
#elif defined THAWK_UNIX
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifndef THAWK_APPLE
#include <sys/sendfile.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#endif
#include <stdio.h>
#include <fcntl.h>
#endif
#ifdef THAWK_HAS_SDL2
#include <SDL2/SDL.h>
#endif
#ifdef THAWK_HAS_ZLIB
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

namespace Tomahawk
{
	namespace Rest
	{
#ifdef THAWK_MICROSOFT
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
#endif
		bool EventArgs::Blockable()
		{
			if (!Worker || !Worker->Queue)
				return false;

			if (Worker->Queue->State != EventState_Working)
				return false;

			if (Worker->Extended)
				return !Worker->Queue->Async.Workers[0].empty();

			return !Worker->Queue->Async.Workers[1].empty();
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
			return Stroke(Value).Replace("{ns}", std::to_string(Nanoseconds())).Replace("{us}", std::to_string(Microseconds())).Replace("{ms}", std::to_string(Milliseconds())).Replace("{s}", std::to_string(Seconds())).Replace("{m}", std::to_string(Minutes())).Replace("{h}", std::to_string(Hours())).Replace("{D}", std::to_string(Days())).Replace("{W}", std::to_string(Weeks())).Replace("{M}", std::to_string(Months())).Replace("{Y}", std::to_string(Years())).R();
		}
		std::string DateTime::Date(const std::string& Value)
		{
			if (DateRebuild)
				Rebuild();

			auto Offset = std::chrono::system_clock::to_time_t(std::chrono::system_clock::time_point(Time));
			tm* T = std::localtime(&Offset);
			T->tm_mon++;
			T->tm_year += 1900;

			return Stroke(Value).Replace("{s}", T->tm_sec < 10 ? Form("0%i", T->tm_sec).R() : std::to_string(T->tm_sec)).Replace("{m}", T->tm_min < 10 ? Form("0%i", T->tm_min).R() : std::to_string(T->tm_min)).Replace("{h}", std::to_string(T->tm_hour)).Replace("{D}", std::to_string(T->tm_yday)).Replace("{MD}", T->tm_mday < 10 ? Form("0%i", T->tm_mday).R() : std::to_string(T->tm_mday)).Replace("{WD}", std::to_string(T->tm_wday + 1)).Replace("{M}", T->tm_mon < 10 ? Form("0%i", T->tm_mon).R() : std::to_string(T->tm_mon)).Replace("{Y}", std::to_string(T->tm_year)).R();
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
					DateValue = *localtime(&TimeNow);
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
					DateValue = *localtime(&TimeNow);
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
					DateValue = *localtime(&TimeNow);
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
					DateValue = *localtime(&TimeNow);
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
					DateValue = *localtime(&TimeNow);
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
					DateValue = *localtime(&TimeNow);
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
					DateValue = *localtime(&TimeNow);
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
			struct tm GTMTimeStamp{ };

#ifdef THAWK_MICROSOFT
			if (gmtime_s(&GTMTimeStamp, &Time) != 0)
#elif defined(THAWK_UNIX)
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
			struct tm Date{ };

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
#elif defined(THAWK_MICROSOFT)
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
			auto TimeStamp = (time_t)Time;
			struct tm Date{ };

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
			if (localtime_s(&Date, &TimeStamp) != 0)
				strncpy(Buffer, "01-Jan-1970 00:00", (size_t)Length);
			else
				strftime(Buffer, (size_t)Length, "%d-%b-%Y %H:%M", &Date);
#else
																																	if (localtime_r(&TimeStamp, &Date) == nullptr)
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

				struct tm Time{ };
				Time.tm_year = Year - 1900;
				Time.tm_mon = (int)i;
				Time.tm_mday = Day;
				Time.tm_hour = Hour;
				Time.tm_min = Minute;
				Time.tm_sec = Second;

#ifdef THAWK_MICROSOFT
				return _mkgmtime(&Time);
#else
				return mktime(&Time);
#endif
			}

			return 0;
		}

		Stroke::Stroke() : Safe(true)
		{
			L = new std::string();
		}
		Stroke::Stroke(int Value) : Safe(true)
		{
			L = new std::string(std::to_string(Value));
		}
		Stroke::Stroke(unsigned int Value) : Safe(true)
		{
			L = new std::string(std::to_string(Value));
		}
		Stroke::Stroke(int64_t Value) : Safe(true)
		{
			L = new std::string(std::to_string(Value));
		}
		Stroke::Stroke(uint64_t Value) : Safe(true)
		{
			L = new std::string(std::to_string(Value));
		}
		Stroke::Stroke(float Value) : Safe(true)
		{
			L = new std::string(std::to_string(Value));
		}
		Stroke::Stroke(double Value) : Safe(true)
		{
			L = new std::string(std::to_string(Value));
		}
		Stroke::Stroke(long double Value) : Safe(true)
		{
			L = new std::string(std::to_string(Value));
		}
		Stroke::Stroke(const std::string& Buffer) : Safe(true)
		{
			L = new std::string(Buffer);
		}
		Stroke::Stroke(std::string* Buffer)
		{
			Safe = (!Buffer);
			L = (Safe ? new std::string() : Buffer);
		}
		Stroke::Stroke(const std::string* Buffer)
		{
			Safe = (!Buffer);
			L = (Safe ? new std::string() : (std::string*)Buffer);
		}
		Stroke::Stroke(const char* Buffer) : Safe(true)
		{
			if (Buffer != nullptr)
				L = new std::string(Buffer);
			else
				L = new std::string();
		}
		Stroke::Stroke(const char* Buffer, int64_t Length) : Safe(true)
		{
			if (Buffer != nullptr)
				L = new std::string(Buffer, Length);
			else
				L = new std::string();
		}
		Stroke::Stroke(const Stroke& Value) : Safe(true)
		{
			if (Value.L != nullptr)
				L = new std::string(*Value.L);
			else
				L = new std::string();
		}
		Stroke::~Stroke()
		{
			if (Safe)
				delete L;
		}
		Stroke& Stroke::EscapePrint()
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
		Stroke& Stroke::Escape()
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
		Stroke& Stroke::Unescape()
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
		Stroke& Stroke::Reserve(uint64_t Count)
		{
			L->reserve(L->capacity() + Count);
			return *this;
		}
		Stroke& Stroke::Resize(uint64_t Count)
		{
			L->resize(Count);
			return *this;
		}
		Stroke& Stroke::Resize(uint64_t Count, char Char)
		{
			L->resize(Count, Char);
			return *this;
		}
		Stroke& Stroke::Clear()
		{
			L->clear();
			return *this;
		}
		Stroke& Stroke::ToUtf8()
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
		Stroke& Stroke::ToUpper()
		{
			std::transform(L->begin(), L->end(), L->begin(), ::toupper);
			return *this;
		}
		Stroke& Stroke::ToLower()
		{
			std::transform(L->begin(), L->end(), L->begin(), ::tolower);
			return *this;
		}
		Stroke& Stroke::Clip(uint64_t Length)
		{
			if (Length < L->size())
				L->erase(Length, L->size() - Length);

			return *this;
		}
		Stroke& Stroke::ReplaceOf(const char* Chars, const char* To, uint64_t Start)
		{
			if (!Chars || Chars[0] == '\0' || !To)
				return *this;

			Stroke::Settle Result{ };
			uint64_t Offset = Start, ToSize = (uint64_t)strlen(To);
			while ((Result = FindOf(Chars, Offset)).Found)
			{
				EraseOffsets(Result.Start, Result.End);
				Insert(To, Result.Start);
				Offset += ToSize;
			}

			return *this;
		}
		Stroke& Stroke::Replace(const std::string& From, const std::string& To, uint64_t Start)
		{
			uint64_t Offset = Start;
			Stroke::Settle Result{ };

			while ((Result = Find(From, Offset)).Found)
			{
				EraseOffsets(Result.Start, Result.End);
				Insert(To, Result.Start);
				Offset += To.size();
			}

			return *this;
		}
		Stroke& Stroke::Replace(const char* From, const char* To, uint64_t Start)
		{
			if (!From || !To)
				return *this;

			uint64_t Offset = Start;
			auto Size = (uint64_t)strlen(To);
			Stroke::Settle Result{ };

			while ((Result = Find(From, Offset)).Found)
			{
				EraseOffsets(Result.Start, Result.End);
				Insert(To, Result.Start);
				Offset += Size;
			}

			return *this;
		}
		Stroke& Stroke::Replace(const char& From, const char& To, uint64_t Position)
		{
			for (uint64_t i = Position; i < L->size(); i++)
			{
				if (L->at(i) == From)
					L->at(i) = To;
			}

			return *this;
		}
		Stroke& Stroke::Replace(const char& From, const char& To, uint64_t Position, uint64_t Count)
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
		Stroke& Stroke::ReplacePart(uint64_t Start, uint64_t End, const std::string& Value)
		{
			return ReplacePart(Start, End, Value.c_str());
		}
		Stroke& Stroke::ReplacePart(uint64_t Start, uint64_t End, const char* Value)
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
		Stroke& Stroke::RemovePart(uint64_t Start, uint64_t End)
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
		Stroke& Stroke::Reverse()
		{
			return Reverse(0, L->size() - 1);
		}
		Stroke& Stroke::Reverse(uint64_t Start, uint64_t End)
		{
			if (Start == End || L->size() < 2 || End > (L->size() - 1) || Start > (L->size() - 1))
				return *this;

			std::reverse(L->begin() + Start, L->begin() + End + 1);
			return *this;
		}
		Stroke& Stroke::Substring(uint64_t Start)
		{
			if (Start >= L->size())
			{
				L->clear();
				return *this;
			}

			L->assign(L->substr(Start));
			return *this;
		}
		Stroke& Stroke::Substring(uint64_t Start, uint64_t Count)
		{
			if (Start >= L->size() || !Count)
			{
				L->clear();
				return *this;
			}

			L->assign(L->substr(Start, Count));
			return *this;
		}
		Stroke& Stroke::Substring(const Stroke::Settle& Result)
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
		Stroke& Stroke::Splice(uint64_t Start, uint64_t End)
		{
			if (Start > (L->size() - 1))
				return (*this);

			if (End > L->size())
				End = (L->size() - Start);

			int64_t Offset = (int64_t)Start - (int64_t)End;
			L->assign(L->substr(Start, (uint64_t)(Offset < 0 ? -Offset : Offset)));
			return *this;
		}
		Stroke& Stroke::Trim()
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
		Stroke& Stroke::Fill(const char& Char)
		{
			if (L->empty())
				return (*this);

			for (char& i : *L)
				i = Char;

			return *this;
		}
		Stroke& Stroke::Fill(const char& Char, uint64_t Count)
		{
			if (L->empty())
				return (*this);

			L->assign(Count, Char);
			return *this;
		}
		Stroke& Stroke::Fill(const char& Char, uint64_t Start, uint64_t Count)
		{
			if (L->empty() || Start > L->size())
				return (*this);

			if (Start + Count > L->size())
				Count = L->size() - Start;

			for (uint64_t i = Start; i < (Start + Count); i++)
				L->at(i) = Char;

			return *this;
		}
		Stroke& Stroke::Assign(const char* Raw)
		{
			if (Raw != nullptr)
				L->assign(Raw);
			else
				L->clear();

			return *this;
		}
		Stroke& Stroke::Assign(const char* Raw, uint64_t Length)
		{
			if (Raw != nullptr)
				L->assign(Raw, Length);
			else
				L->clear();

			return *this;
		}
		Stroke& Stroke::Assign(const std::string& Raw)
		{
			L->assign(Raw);
			return *this;
		}
		Stroke& Stroke::Assign(const std::string& Raw, uint64_t Start, uint64_t Count)
		{
			L->assign(Raw.substr(Start, Count));
			return *this;
		}
		Stroke& Stroke::Assign(const char* Raw, uint64_t Start, uint64_t Count)
		{
			if (!Raw)
			{
				L->clear();
				return *this;
			}

			L->assign(Raw);
			return Substring(Start, Count);
		}
		Stroke& Stroke::Append(const char* Raw)
		{
			if (Raw != nullptr)
				L->append(Raw);

			return *this;
		}
		Stroke& Stroke::Append(const char& Char)
		{
			L->append(1, Char);
			return *this;
		}
		Stroke& Stroke::Append(const char& Char, uint64_t Count)
		{
			L->append(Count, Char);
			return *this;
		}
		Stroke& Stroke::Append(const std::string& Raw)
		{
			L->append(Raw);
			return *this;
		}
		Stroke& Stroke::Append(const char* Raw, uint64_t Count)
		{
			if (Raw != nullptr)
				L->append(Raw, Count);

			return *this;
		}
		Stroke& Stroke::Append(const char* Raw, uint64_t Start, uint64_t Count)
		{
			if (!Raw)
				return *this;

			std::string V(Raw);
			if (!Count || V.size() < Start + Count)
				return *this;

			L->append(V.substr(Start, Count));
			return *this;
		}
		Stroke& Stroke::Append(const std::string& Raw, uint64_t Start, uint64_t Count)
		{
			if (!Count || Raw.size() < Start + Count)
				return *this;

			L->append(Raw.substr(Start, Count));
			return *this;
		}
		Stroke& Stroke::fAppend(const char* Format, ...)
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
		Stroke& Stroke::Insert(const std::string& Raw, uint64_t Position)
		{
			if (Position >= L->size())
				Position = L->size();

			L->insert(Position, Raw);
			return *this;
		}
		Stroke& Stroke::Insert(const std::string& Raw, uint64_t Position, uint64_t Start, uint64_t Count)
		{
			if (Position >= L->size())
				Position = L->size();

			if (Raw.size() >= Start + Count)
				L->insert(Position, Raw.substr(Start, Count));

			return *this;
		}
		Stroke& Stroke::Insert(const std::string& Raw, uint64_t Position, uint64_t Count)
		{
			if (Position >= L->size())
				Position = L->size();

			if (Count >= Raw.size())
				Count = Raw.size();

			L->insert(Position, Raw.substr(0, Count));
			return *this;
		}
		Stroke& Stroke::Insert(const char& Char, uint64_t Position, uint64_t Count)
		{
			if (Position >= L->size())
				return *this;

			L->insert(Position, Count, Char);
			return *this;
		}
		Stroke& Stroke::Insert(const char& Char, uint64_t Position)
		{
			if (Position >= L->size())
				return *this;

			L->insert(L->begin() + Position, Char);
			return *this;
		}
		Stroke& Stroke::Erase(uint64_t Position)
		{
			if (Position >= L->size())
				return *this;

			L->erase(Position);
			return *this;
		}
		Stroke& Stroke::Erase(uint64_t Position, uint64_t Count)
		{
			if (Position >= L->size())
				return *this;

			L->erase(Position, Count);
			return *this;
		}
		Stroke& Stroke::EraseOffsets(uint64_t Start, uint64_t End)
		{
			return Erase(Start, End - Start);
		}
		Stroke& Stroke::Path(const std::string& Net, const std::string& Dir)
		{
			if (StartsOf("./\\"))
			{
				std::string Result = Rest::OS::Resolve(L->c_str(), Dir);
				if (!Result.empty())
					Assign(Result);
			}
			else
				Replace("[subnet]", Net);

			return *this;
		}
		Stroke::Settle Stroke::ReverseFind(const std::string& Needle, uint64_t Offset) const
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
		Stroke::Settle Stroke::ReverseFind(const char* Needle, uint64_t Offset) const
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
		Stroke::Settle Stroke::ReverseFind(const char& Needle, uint64_t Offset) const
		{
			if (L->empty())
				return { L->size() - 1, L->size(), false };

			for (uint64_t i = L->size() - 1 - Offset; i > 0; i--)
			{
				if (L->at(i) == Needle)
					return { i, i + 1, true };
			}

			return { L->size() - 1, L->size(), false };
		}
		Stroke::Settle Stroke::ReverseFindUnescaped(const char& Needle, uint64_t Offset) const
		{
			if (L->empty())
				return { L->size() - 1, L->size(), false };

			for (uint64_t i = L->size() - 1 - Offset; i > 0; i--)
			{
				if (L->at(i) == Needle && ((int64_t)i - 1 < 0 || L->at(i - 1) != '\\'))
					return { i, i + 1, true };
			}

			return { L->size() - 1, L->size(), false };
		}
		Stroke::Settle Stroke::ReverseFindOf(const std::string& Needle, uint64_t Offset) const
		{
			if (L->empty())
				return { L->size() - 1, L->size(), false };

			for (uint64_t i = L->size() - 1 - Offset; i > 0; i--)
			{
				for (char k : Needle)
				{
					if (L->at(i) == k)
						return { i, i + 1, true };
				}
			}

			return { L->size() - 1, L->size(), false };
		}
		Stroke::Settle Stroke::ReverseFindOf(const char* Needle, uint64_t Offset) const
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
			}

			return { L->size() - 1, L->size(), false };
		}
		Stroke::Settle Stroke::Find(const std::string& Needle, uint64_t Offset) const
		{
			const char* It = strstr(L->c_str() + Offset, Needle.c_str());
			if (It == nullptr)
				return { L->size() - 1, L->size(), false };

			return { (uint64_t)(It - L->c_str()), (uint64_t)(It - L->c_str() + Needle.size()), true };
		}
		Stroke::Settle Stroke::Find(const char* Needle, uint64_t Offset) const
		{
			if (!Needle)
				return { L->size() - 1, L->size(), false };

			const char* It = strstr(L->c_str() + Offset, Needle);
			if (It == nullptr)
				return { L->size() - 1, L->size(), false };

			return { (uint64_t)(It - L->c_str()), (uint64_t)(It - L->c_str() + strlen(Needle)), true };
		}
		Stroke::Settle Stroke::Find(const char& Needle, uint64_t Offset) const
		{
			for (uint64_t i = Offset; i < L->size(); i++)
			{
				if (L->at(i) == Needle)
					return { i, i + 1, true };
			}

			return { L->size() - 1, L->size(), false };
		}
		Stroke::Settle Stroke::FindUnescaped(const char& Needle, uint64_t Offset) const
		{
			for (uint64_t i = Offset; i < L->size(); i++)
			{
				if (L->at(i) == Needle && ((int64_t)i - 1 < 0 || L->at(i - 1) != '\\'))
					return { i, i + 1, true };
			}

			return { L->size() - 1, L->size(), false };
		}
		Stroke::Settle Stroke::FindOf(const std::string& Needle, uint64_t Offset) const
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
		Stroke::Settle Stroke::FindOf(const char* Needle, uint64_t Offset) const
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
		bool Stroke::StartsWith(const std::string& Value, uint64_t Offset) const
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
		bool Stroke::StartsWith(const char* Value, uint64_t Offset) const
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
		bool Stroke::StartsOf(const char* Value, uint64_t Offset) const
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
		bool Stroke::EndsWith(const std::string& Value) const
		{
			if (L->empty())
				return false;

			return strcmp(L->c_str() + L->size() - Value.size(), Value.c_str()) == 0;
		}
		bool Stroke::EndsWith(const char* Value) const
		{
			if (L->empty() || !Value)
				return false;

			return strcmp(L->c_str() + L->size() - strlen(Value), Value) == 0;
		}
		bool Stroke::EndsWith(const char& Value) const
		{
			return !L->empty() && L->back() == Value;
		}
		bool Stroke::EndsOf(const char* Value) const
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
		bool Stroke::Empty() const
		{
			return L->empty();
		}
		bool Stroke::HasInteger() const
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
		bool Stroke::HasNumber() const
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
		bool Stroke::HasDecimal() const
		{
			auto F = Find('.');
			if (F.Found)
			{
				auto D1 = Stroke(L->c_str(), F.End);
				if (D1.Empty() || !D1.HasInteger())
					return false;

				auto D2 = Stroke(L->c_str() + F.End + 1, L->size() - F.End);
				if (D2.Empty() || !D2.HasInteger())
					return false;

				return D1.Size() > 19 || D2.Size() > 15;
			}

			return HasInteger() && L->size() > 19;
		}
		bool Stroke::ToBoolean() const
		{
			return !strncmp(L->c_str(), "true", 4) || !strncmp(L->c_str(), "1", 1);
		}
		bool Stroke::IsDigit(char Char)
		{
			return Char == '0' || Char == '1' || Char == '2' || Char == '3' || Char == '4' || Char == '5' || Char == '6' || Char == '7' || Char == '8' || Char == '9';
		}
		int Stroke::CaseCompare(const char* Value1, const char* Value2)
		{
			if (!Value1 || !Value2)
				return 0;

			int Result;
			do
			{
				Result = tolower(*(const unsigned char*)(Value1++)) - tolower(*(const unsigned char*)(Value2++));
			}while (Result == 0 && Value1[-1] != '\0');

			return Result;
		}
		int Stroke::Match(const char* Pattern, const char* Text)
		{
			return Match(Pattern, strlen(Pattern), Text);
		}
		int Stroke::Match(const char* Pattern, uint64_t Length, const char* Text)
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
					}while (Result == -1 && Offset-- > 0);

					return (Result == -1) ? -1 : j + Result + Offset;
				}
				else if (tolower((const unsigned char)Pattern[i]) != tolower((const unsigned char)Text[j]))
					return -1;

				i++;
				j++;
			}

			return j;
		}
		int Stroke::ToInt() const
		{
			return (int)strtol(L->c_str(), nullptr, 10);
		}
		long Stroke::ToLong() const
		{
			return strtol(L->c_str(), nullptr, 10);
		}
		float Stroke::ToFloat() const
		{
			return strtof(L->c_str(), nullptr);
		}
		unsigned int Stroke::ToUInt() const
		{
			return (unsigned int)ToULong();
		}
		unsigned long Stroke::ToULong() const
		{
			return strtoul(L->c_str(), nullptr, 10);
		}
		int64_t Stroke::ToInt64() const
		{
			return strtoll(L->c_str(), nullptr, 10);
		}
		double Stroke::ToFloat64() const
		{
			return strtod(L->c_str(), nullptr);
		}
		long double Stroke::ToLFloat64() const
		{
			return strtold(L->c_str(), nullptr);
		}
		uint64_t Stroke::ToUInt64() const
		{
			return strtoull(L->c_str(), nullptr, 10);
		}
		uint64_t Stroke::Size() const
		{
			return L->size();
		}
		uint64_t Stroke::Capacity() const
		{
			return L->capacity();
		}
		char* Stroke::Value() const
		{
			return (char*)L->data();
		}
		const char* Stroke::Get() const
		{
			return L->c_str();
		}
		std::string& Stroke::R()
		{
			return *L;
		}
		std::basic_string<wchar_t> Stroke::ToUnicode() const
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
		std::vector<std::string> Stroke::Split(const std::string& With, uint64_t Start) const
		{
			Stroke::Settle Result = Find(With, Start);
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
		std::vector<std::string> Stroke::Split(char With, uint64_t Start) const
		{
			Stroke::Settle Result = Find(With, Start);
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
		std::vector<std::string> Stroke::SplitMax(char With, uint64_t Count, uint64_t Start) const
		{
			Stroke::Settle Result = Find(With, Start);
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
		std::vector<std::string> Stroke::SplitOf(const char* With, uint64_t Start) const
		{
			Stroke::Settle Result = FindOf(With, Start);
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
		Stroke& Stroke::operator= (const Stroke& Value)
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

		void LT::AddRef(Object* Value)
		{
			if (Value != nullptr)
				Value->AddRef();
		}
		void LT::SetFlag(Object* Value)
		{
			if (Value != nullptr)
				Value->SetFlag();
		}
		bool LT::GetFlag(Object* Value)
		{
			return Value ? Value->GetFlag() : false;
		}
		int LT::GetRefCount(Object* Value)
		{
			return Value ? Value->GetRefCount() : 1;
		}
		void LT::Release(Object* Value)
		{
			if (Value != nullptr)
				Value->Release();
		}
		void LT::AttachCallback(const std::function<void(const char*, int)>& _Callback)
		{
			Callback = _Callback;
		}
		void LT::AttachStream()
		{
			Enabled = true;
		}
		void LT::DetachCallback()
		{
			Callback = nullptr;
		}
		void LT::DetachStream()
		{
			Enabled = false;
		}
		void LT::Log(int Level, int Line, const char* Source, const char* Format, ...)
		{
			if (!Source || !Format || (!Enabled && !Callback))
				return;

			auto TimeStamp = (time_t)time(nullptr);
			tm DateTime{ };
			char Date[64];

			if (Line < 0)
				Line = 0;

#if defined(THAWK_MICROSOFT)
			if (gmtime_s(&DateTime, &TimeStamp) != 0)
#elif defined(THAWK_UNIX)
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
				int ErrorCode = OS::GetError();
#ifdef THAWK_MICROSOFT
				if (ErrorCode != ERROR_SUCCESS)
					snprintf(Buffer, sizeof(Buffer), "%s %s:%d [err] %s\n\tsystem: %s\n", Date, Source, Line, Format, OS::GetErrorName(ErrorCode).c_str());
				else
					snprintf(Buffer, sizeof(Buffer), "%s %s:%d [err] %s\n", Date, Source, Line, Format);
#else
																																		if (ErrorCode > 0)
                    snprintf(Buffer, sizeof(Buffer), "%s %s:%d [err] %s\n\tsystem: %s\n", Date, Source, Line, Format, OS::GetErrorName(ErrorCode).c_str());
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
#if defined(THAWK_MICROSOFT) && defined(_DEBUG)
				OutputDebugStringA(Storage);
#endif
				printf("%s", Storage);
			}

			va_end(Args);
		}
		void LT::Free(void* Ptr)
		{
#ifndef NDEBUG
			if (!Objects || !Safe)
			{
				THAWK_ERROR("[segfault] object at 0x%p is suspended", Ptr);
				return Interrupt();
			}

			Safe->lock();
			auto It = Objects->find((Object*)Ptr);
			if (It == Objects->end())
			{
				Safe->unlock();
				THAWK_ERROR("[segfault] object at 0x%p is suspended", Ptr);	
				return Interrupt();
			}

			Object* Ref = (Object*)Ptr;
			if (Ref != nullptr && --Ref->__vcnt > 0)
				return Safe->unlock();

			Memory -= It->second;
			Objects->erase(It);

			if (Objects->empty())
			{
				delete Objects;
				Objects = nullptr;
			}

			Safe->unlock();
			if (!Objects)
			{
				delete Safe;
				Safe = nullptr;
			}
#else
			Object* Ref = (Object*)Ptr;
			if (Ref != nullptr && --Ref->__vcnt > 0)
				return;
#endif
			free(Ptr);
		}
		void* LT::Alloc(uint64_t Size)
		{
#ifndef NDEBUG
			if (!Objects)
				Objects = new std::unordered_map<void*, uint64_t>();

			if (!Safe)
				Safe = new std::mutex();

			Safe->lock();
			void* Ptr = malloc((size_t)Size);
			Objects->insert({ Ptr, Size });
			Safe->unlock();

			return Ptr;
#else
			return malloc((size_t)Size);
#endif
		}
		void* LT::GetPtr(void* Ptr)
		{
#ifndef NDEBUG
			if (!Objects)
				return nullptr;

			auto It = Objects->find((Object*)Ptr);
			if (It == Objects->end())
				return nullptr;

			return It->first;
#else
			return (Object*)Ptr;
#endif
		}
		uint64_t LT::GetSize(void* Ptr)
		{
#ifndef NDEBUG
			if (!Objects)
				return 0;

			auto It = Objects->find((Object*)Ptr);
			if (It == Objects->end())
				return 0;

			return It->second;
#else
			return 0;
#endif
		}
		uint64_t LT::GetCount()
		{
#ifndef NDEBUG
			if (!Objects || !Safe)
				return 0;

			Safe->lock();
			uint64_t Count = (uint64_t)Objects->size();
			Safe->unlock();

			return Count;
#else
			return 0;
#endif
		}
		uint64_t LT::GetMemory()
		{
#ifndef NDEBUG
			return Memory;
#else
			return 0;
#endif
		}
		void LT::Report()
		{
#ifndef NDEBUG
			if (!Objects || !Safe || Objects->empty())
				return;

			Safe->lock();
			uint64_t Size = 0;
			for (auto& Item : *Objects)
			{
				THAWK_WARN("[memerr] object of size %llu at 0x%p", Item.second, Item.first);
				Size += Item.second;
			}
			THAWK_WARN("[memerr] at least %llu bytes of memory in %i blocks were not released", Size, (int)Objects->size());
			Safe->unlock();
			Interrupt();
#endif
		}
		void LT::Interrupt()
		{
#ifndef NDEBUG
#ifndef THAWK_MICROSOFT
#ifndef SIGTRAP
			__debugbreak();
#else
			raise(SIGTRAP);
#endif
#else
			if (!IsDebuggerPresent())
				THAWK_ERROR("[dbg] cannot interrupt");
			else
				DebugBreak();
#endif
			THAWK_INFO("[dbg] process interruption called");
#endif
		}
		std::function<void(const char*, int)> LT::Callback;
		bool LT::Enabled = false;
#ifndef NDEBUG
		std::unordered_map<void*, uint64_t>* LT::Objects = nullptr;
		std::mutex* LT::Safe = nullptr;
		uint64_t LT::Memory = 0;
#endif
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

			auto It = Factory->find(THAWK_COMPONENT_HASH(Hash));
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

		Object::Object() : __vcnt(1), __vflg(false)
		{
		}
        Object::~Object()
        {
        }
		void Object::operator delete(void* Data)
		{
			LT::Free(Data);
		}
		void* Object::operator new(size_t Size)
		{
			return LT::Alloc((uint64_t)Size);
		}
		void Object::SetFlag()
		{
			__vflg = true;
		}
		bool Object::GetFlag()
		{
			return __vflg.load();
		}
		int Object::GetRefCount()
		{
			return __vcnt.load();
		}
		Object* Object::AddRef()
		{
			__vcnt++;
			__vflg = false;
			return this;
		}
		Object* Object::Release()
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

#ifdef THAWK_MICROSOFT
			if (!Handle)
				return;

			::ShowWindow(::GetConsoleWindow(), SW_HIDE);
			FreeConsole();
#endif
		}
		void Console::Hide()
		{
#ifdef THAWK_MICROSOFT
			if (Handle)
				::ShowWindow(::GetConsoleWindow(), SW_HIDE);
#endif
		}
		void Console::Show()
		{
#ifdef THAWK_MICROSOFT
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
#ifdef THAWK_MICROSOFT
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
#elif defined THAWK_UNIX
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
#ifdef THAWK_MICROSOFT
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
#ifdef THAWK_MICROSOFT
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
#ifdef THAWK_MICROSOFT
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
#ifdef THAWK_MICROSOFT
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
			char Buffer[2048] = { '\0' };

			va_list Args;
			va_start(Args, Format);
#ifdef THAWK_MICROSOFT
			_vsnprintf(Buffer, sizeof(Buffer), Format, Args);
			va_end(Args);

			OutputDebugString(Buffer);
#elif defined THAWK_UNIX
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
#ifdef THAWK_MICROSOFT
			Frequency = new LARGE_INTEGER();
			QueryPerformanceFrequency((LARGE_INTEGER*)Frequency);

			TimeLimit = new LARGE_INTEGER();
			QueryPerformanceCounter((LARGE_INTEGER*)TimeLimit);

			PastTime = new LARGE_INTEGER();
			QueryPerformanceCounter((LARGE_INTEGER*)PastTime);
#elif defined THAWK_UNIX
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
#ifdef THAWK_MICROSOFT
			if (PastTime != nullptr)
				delete (LARGE_INTEGER*)PastTime;
			PastTime = nullptr;

			if (TimeLimit != nullptr)
				delete (LARGE_INTEGER*)TimeLimit;
			TimeLimit = nullptr;

			if (Frequency != nullptr)
				delete (LARGE_INTEGER*)Frequency;
			Frequency = nullptr;
#elif defined THAWK_UNIX
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
#ifdef THAWK_MICROSOFT
			QueryPerformanceCounter((LARGE_INTEGER*)PastTime);
			return (((LARGE_INTEGER*)PastTime)->QuadPart - ((LARGE_INTEGER*)TimeLimit)->QuadPart) * 1000.0 / ((LARGE_INTEGER*)Frequency)->QuadPart;
#elif defined THAWK_UNIX
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

		FileStream::FileStream()
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
				Buffer = (FILE*)OS::Open(Path.c_str(), "w");
		}
		bool FileStream::Open(const char* File, FileMode Mode)
		{
			if (!File || !Close())
				return false;

			Path = OS::Resolve(File).c_str();
			switch (Mode)
			{
				case FileMode_Read_Only:
					Buffer = (FILE*)OS::Open(Path.c_str(), "r");
					break;
				case FileMode_Write_Only:
					Buffer = (FILE*)OS::Open(Path.c_str(), "w");
					break;
				case FileMode_Append_Only:
					Buffer = (FILE*)OS::Open(Path.c_str(), "a");
					break;
				case FileMode_Read_Write:
					Buffer = (FILE*)OS::Open(Path.c_str(), "r+");
					break;
				case FileMode_Write_Read:
					Buffer = (FILE*)OS::Open(Path.c_str(), "w+");
					break;
				case FileMode_Read_Append_Write:
					Buffer = (FILE*)OS::Open(Path.c_str(), "a+");
					break;
				case FileMode_Binary_Read_Only:
					Buffer = (FILE*)OS::Open(Path.c_str(), "rb");
					break;
				case FileMode_Binary_Write_Only:
					Buffer = (FILE*)OS::Open(Path.c_str(), "wb");
					break;
				case FileMode_Binary_Append_Only:
					Buffer = (FILE*)OS::Open(Path.c_str(), "ab");
					break;
				case FileMode_Binary_Read_Write:
					Buffer = (FILE*)OS::Open(Path.c_str(), "rb+");
					break;
				case FileMode_Binary_Write_Read:
					Buffer = (FILE*)OS::Open(Path.c_str(), "wb+");
					break;
				case FileMode_Binary_Read_Append_Write:
					Buffer = (FILE*)OS::Open(Path.c_str(), "ab+");
					break;
			}

			return Buffer != nullptr;
		}
		bool FileStream::OpenZ(const char* File, FileMode Mode)
		{
			if (!File || !Close())
				return false;

#ifdef THAWK_HAS_ZLIB
			Path = OS::Resolve(File).c_str();
			switch (Mode)
			{
				case FileMode_Binary_Read_Only:
				case FileMode_Read_Only:
					Compress = gzopen(Path.c_str(), "rb");
					break;
				case FileMode_Binary_Write_Only:
				case FileMode_Write_Only:
					Compress = gzopen(Path.c_str(), "wb");
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

			return Compress;
#else
			return false;
#endif
		}
		bool FileStream::Close()
		{
#ifdef THAWK_HAS_ZLIB
			if (Compress != nullptr)
			{
				gzclose((gzFile)Compress);
				Compress = nullptr;
			}
#endif
			if (Buffer != nullptr)
			{
				fclose(Buffer);
				Buffer = nullptr;
			}

			return true;
		}
		bool FileStream::Seek(FileSeek Mode, int64_t Offset)
		{
			switch (Mode)
			{
				case FileSeek_Begin:
#ifdef THAWK_HAS_ZLIB
					if (Compress != nullptr)
						return gzseek((gzFile)Compress, (long)Offset, SEEK_SET) == 0;
#endif
					if (Buffer != nullptr)
						return fseek(Buffer, (long)Offset, SEEK_SET) == 0;
					break;
				case FileSeek_Current:
#ifdef THAWK_HAS_ZLIB
					if (Compress != nullptr)
						return gzseek((gzFile)Compress, (long)Offset, SEEK_CUR) == 0;
#endif
					if (Buffer != nullptr)
						return fseek(Buffer, (long)Offset, SEEK_CUR) == 0;
					break;
				case FileSeek_End:
#ifdef THAWK_HAS_ZLIB
					if (Compress != nullptr)
						return gzseek((gzFile)Compress, (long)Offset, SEEK_END) == 0;
#endif
					if (Buffer != nullptr)
						return fseek(Buffer, (long)Offset, SEEK_END) == 0;
					break;
			}

			return false;
		}
		bool FileStream::Move(int64_t Offset)
		{
#ifdef THAWK_HAS_ZLIB
			if (Compress != nullptr)
				return gzseek((gzFile)Compress, (long)Offset, SEEK_CUR) == 0;
#endif
			if (!Buffer)
				return false;

			return fseek(Buffer, (long)Offset, SEEK_CUR) == 0;
		}
		int FileStream::Error()
		{
#ifdef THAWK_HAS_ZLIB
			if (Compress != nullptr)
			{
				int Error;
				const char* M = gzerror((gzFile)Compress, &Error);
				if (M != nullptr && M[0] != '\0')
					THAWK_ERROR("gz stream error -> %s", M);

				return Error;
			}
#endif
			if (!Buffer)
				return 0;

			return ferror(Buffer);
		}
		int FileStream::Flush()
		{
			if (!Buffer)
				return 0;

			return fflush(Buffer);
		}
		int FileStream::Fd()
		{
			if (!Buffer)
				return -1;

#ifdef THAWK_MICROSOFT
			return _fileno(Buffer);
#else
			return fileno(Buffer);
#endif
		}
		unsigned char FileStream::Get()
		{
#ifdef THAWK_HAS_ZLIB
			if (Compress != nullptr)
				return (unsigned char)gzgetc((gzFile)Compress);
#endif
			if (!Buffer)
				return 0;

			return (unsigned char)getc(Buffer);
		}
		unsigned char FileStream::Put(unsigned char Value)
		{
#ifdef THAWK_HAS_ZLIB
			if (Compress != nullptr)
				return (unsigned char)gzputc((gzFile)Compress, Value);
#endif
			if (!Buffer)
				return 0;

			return (unsigned char)putc((int)Value, Buffer);
		}
		uint64_t FileStream::ReadAny(const char* Format, ...)
		{
#ifdef THAWK_HAS_ZLIB
			if (Compress != nullptr)
				return 0;
#endif
			va_list Args;
			uint64_t R = 0;
			va_start(Args, Format);

			if (Buffer != nullptr)
				R = (uint64_t)vfscanf(Buffer, Format, Args);

			va_end(Args);

			return R;
		}
		uint64_t FileStream::Read(char* Data, uint64_t Length)
		{
#ifdef THAWK_HAS_ZLIB
			if (Compress != nullptr)
				return gzread((gzFile)Compress, Data, Length);
#endif
			if (!Buffer)
				return 0;

			return fread(Data, 1, (size_t)Length, Buffer);
		}
		uint64_t FileStream::WriteAny(const char* Format, ...)
		{
			va_list Args;
			uint64_t R = 0;
			va_start(Args, Format);
#ifdef THAWK_HAS_ZLIB
			if (Compress != nullptr)
				R = (uint64_t)gzvprintf((gzFile)Compress, Format, Args);
			else if (Buffer != nullptr)
				R = (uint64_t)vfprintf(Buffer, Format, Args);
#else
			if (Buffer != nullptr)
				R = (uint64_t)vfprintf(Buffer, Format, Args);
#endif
			va_end(Args);

			return R;
		}
		uint64_t FileStream::Write(const char* Data, uint64_t Length)
		{
#ifdef THAWK_HAS_ZLIB
			if (Compress != nullptr)
				return gzwrite((gzFile)Compress, Data, Length);
#endif
			if (!Buffer)
				return 0;

			return fwrite(Data, 1, (size_t)Length, Buffer);
		}
		uint64_t FileStream::Tell()
		{
#ifdef THAWK_HAS_ZLIB
			if (Compress != nullptr)
				return gztell((gzFile)Compress);
#endif
			if (!Buffer)
				return 0;

			return ftell(Buffer);
		}
		uint64_t FileStream::Size()
		{
			if (!Buffer)
				return 0;

			uint64_t Position = Tell();
			Seek(FileSeek_End, 0);
			uint64_t Size = Tell();
			Seek(FileSeek_Begin, Position);

			return Size;
		}
		std::string& FileStream::Filename()
		{
			return Path;
		}
		FILE* FileStream::Stream()
		{
			return Buffer;
		}
		void* FileStream::StreamZ()
		{
			return Compress;
		}

		FileTree::FileTree(const std::string& Folder)
		{
			Path = OS::Resolve(Folder.c_str());
			if (!Path.empty())
			{
				OS::Iterate(Path.c_str(), [this](DirectoryEntry* Entry) -> bool
				{
					if (Entry->IsDirectory)
						Directories.push_back(new FileTree(Entry->Path));
					else
						Files.push_back(OS::Resolve(Entry->Path.c_str()));

					return true;
				});
			}
			else
			{
				std::vector<std::string> Drives = OS::GetDiskDrives();
				for (auto& Drive : Drives)
					Directories.push_back(new FileTree(Drive));
			}
		}
		FileTree::~FileTree()
		{
			for (auto& Directory : Directories)
				delete Directory;
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

		void OS::SetDirectory(const char* Path)
		{
			if (!Path)
				return;

#ifdef THAWK_MICROSOFT
			SetCurrentDirectoryA(Path);
#elif defined(THAWK_UNIX)
			chdir(Path);
#endif
		}
		void OS::SaveBitmap(const char* Path, int Width, int Height, unsigned char* Ptr)
		{
			unsigned char File[14] = { 'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, 40 + 14, 0, 0, 0 };

			unsigned char Header[40] = { 40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x13, 0x0B, 0, 0, 0x13, 0x0B, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

			int PadSize = (4 - (Width * 3) % 4) % 4;
			int DataSize = Width * Height * 3 + Height * PadSize;
			int Size = DataSize + (int)sizeof(File) + (int)sizeof(Header);

			File[2] = (unsigned char)(Size);
			File[3] = (unsigned char)(Size >> 8);
			File[4] = (unsigned char)(Size >> 16);
			File[5] = (unsigned char)(Size >> 24);
			Header[4] = (unsigned char)(Width);
			Header[5] = (unsigned char)(Width >> 8);
			Header[6] = (unsigned char)(Width >> 16);
			Header[7] = (unsigned char)(Width >> 24);
			Header[8] = (unsigned char)(Height);
			Header[9] = (unsigned char)(Height >> 8);
			Header[10] = (unsigned char)(Height >> 16);
			Header[11] = (unsigned char)(Height >> 24);
			Header[20] = (unsigned char)(DataSize);
			Header[21] = (unsigned char)(DataSize >> 8);
			Header[22] = (unsigned char)(DataSize >> 16);
			Header[23] = (unsigned char)(DataSize >> 24);

			FILE* Stream = (FILE*)Open(Path, "wb");
			if (!Stream)
				return;

			fwrite((char*)File, sizeof(File), 1, Stream);
			fwrite((char*)Header, sizeof(Header), 1, Stream);

			unsigned char Padding[3] = { 0, 0, 0 };
			uint64_t Offset = 0;

			for (int y = 0; y < Width; y++)
			{
				for (int x = 0; x < Height; x++)
				{
					unsigned char Pixel[3];
					Pixel[0] = Ptr[Offset];
					Pixel[1] = Ptr[Offset + 1];
					Pixel[2] = Ptr[Offset + 2];
					Offset += 3;

					fwrite((char*)Pixel, 3, 1, Stream);
				}
				fwrite((char*)Padding, PadSize, 1, Stream);
			}

			fclose(Stream);
		}
		bool OS::Iterate(const char* Path, const std::function<bool(DirectoryEntry*)>& Callback)
		{
			if (!Path)
				return false;

			std::vector<ResourceEntry> Entries;
			std::string Result = Resolve(Path);
			ScanDir(Result, &Entries);

			Stroke R(&Result);
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
		bool OS::FileExists(const char* Path)
		{
			struct stat Buffer{ };
			return (stat(Resolve(Path).c_str(), &Buffer) == 0);
		}
		bool OS::ExecExists(const char* Path)
		{
#ifdef THAWK_MICROSOFT
			Stroke File = Path;
			if (!File.EndsWith(".exe"))
				File.Append(".exe");

			return FileExists(File.Get());
#else
			return FileExists(Path);
#endif
		}
		bool OS::DirExists(const char* Path)
		{
			struct stat Buffer{ };
			if (stat(Resolve(Path).c_str(), &Buffer) != 0)
				return false;

			return Buffer.st_mode & S_IFDIR;
		}
		bool OS::Write(const char* Path, const char* Data, uint64_t Length)
		{
			FILE* Stream = (FILE*)Open(Path, "wb");
			if (!Stream)
				return false;

			fwrite((const void*)Data, (size_t)Length, 1, Stream);
			fclose(Stream);

			return true;
		}
		bool OS::Write(const char* Path, const std::string& Data)
		{
			FILE* Stream = (FILE*)Open(Path, "wb");
			if (!Stream)
				return false;

			fwrite((const void*)Data.c_str(), (size_t)Data.size(), 1, Stream);
			fclose(Stream);

			return true;
		}
		bool OS::RemoveFile(const char* Path)
		{
#ifdef THAWK_MICROSOFT
			SetFileAttributesA(Path, 0);
			return DeleteFileA(Path) != 0;
#elif defined THAWK_UNIX
			return unlink(Path) == 0;
#endif
		}
		bool OS::RemoveDir(const char* Path)
		{
#ifdef THAWK_MICROSOFT
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
					return RemoveDir(FilePath.c_str());

				if (::SetFileAttributes(FilePath.c_str(), FILE_ATTRIBUTE_NORMAL) == FALSE)
					return false;

				if (::DeleteFile(FilePath.c_str()) == FALSE)
					return false;
			}while (::FindNextFile(Handle, &FileInformation) == TRUE);
			::FindClose(Handle);

			if (::GetLastError() != ERROR_NO_MORE_FILES)
				return false;

			if (::SetFileAttributes(Path, FILE_ATTRIBUTE_NORMAL) == FALSE)
				return false;

			return ::RemoveDirectory(Path) != FALSE;
#elif defined THAWK_UNIX
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
                Buffer = (char*)malloc(Length);

                if (!Buffer)
                    continue;

                struct stat State;
                snprintf(Buffer, Length, "%s/%s", Path, It->d_name);

                if (!stat(Buffer, &State))
                {
                    if (S_ISDIR(State.st_mode))
                        Next = RemoveDir(Buffer);
                    else
                        Next = (unlink(Buffer) == 0);
                }

                free(Buffer);
                if (!Next)
                    break;
            }

            closedir(Root);
            return (rmdir(Path) == 0);
#endif
		}
		bool OS::CreateDir(const char* Path)
		{
			if (!Path || Path[0] == '\0')
				return false;

#ifdef THAWK_MICROSOFT
			wchar_t Buffer[1024];
			UnicodePath(Path, Buffer, 1024);
			if (::CreateDirectoryW(Buffer, nullptr) == TRUE || GetLastError() == ERROR_ALREADY_EXISTS)
				return true;

			size_t Index = wcslen(Buffer) - 1;
			while (Index > 0 && Buffer[Index] != '/' && Buffer[Index] != '\\')
				Index--;

			if (Index > 0 && !CreateDir(std::string(Path).substr(0, Index).c_str()))
				return false;

			return ::CreateDirectoryW(Buffer, nullptr) == TRUE || GetLastError() == ERROR_ALREADY_EXISTS;
#else
																																	if (mkdir(Path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1 || errno == EEXIST)
                return true;

            size_t Index = strlen(Path) - 1;
            while (Index > 0 && Path[Index] != '/' && Path[Index] != '\\')
                Index--;

            if (Index > 0 && !CreateDir(std::string(Path).substr(0, Index).c_str()))
                return false;

            return mkdir(Path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1 || errno == EEXIST;
#endif
		}
		bool OS::Move(const char* From, const char* To)
		{
#ifdef THAWK_MICROSOFT
			return MoveFileA(From, To) != 0;
#elif defined THAWK_UNIX
			return !rename(From, To);
#endif
		}
		bool OS::ScanDir(const std::string& Path, std::vector<ResourceEntry>* Entries)
		{
			if (!Entries)
				return false;

			ResourceEntry Entry;

#if defined(THAWK_MICROSOFT)
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

			auto* Value = (Directory*)malloc(sizeof(Directory));
			DWORD Attributes = GetFileAttributesW(WPath);
			if (Attributes != 0xFFFFFFFF && ((Attributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY))
			{
				wcscat(WPath, L"\\*");
				Value->Handle = FindFirstFileW(WPath, &Value->Info);
				Value->Result.Directory[0] = '\0';
			}
			else
			{
				free(Value);
				return false;
			}

			while (true)
			{
				Dirent* Next = &Value->Result;
				WideCharToMultiByte(CP_UTF8, 0, Value->Info.cFileName, -1, Next->Directory, sizeof(Next->Directory), nullptr, nullptr);
				if (strcmp(Next->Directory, ".") != 0 && strcmp(Next->Directory, "..") != 0 && StateResource(Path + '/' + Next->Directory, &Entry.Source))
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

			free(Value);
			return true;
#else
																																	DIR* Value = opendir(Path.c_str());
            if (!Value)
                return false;

            dirent* Dirent = nullptr;
            while ((Dirent = readdir(Value)) != nullptr)
            {
                if (strcmp(Dirent->d_name, ".") && strcmp(Dirent->d_name, "..") && StateResource(Path + '/' + Dirent->d_name, &Entry.Source))
                {
                    Entry.Path = Dirent->d_name;
                    Entries->push_back(Entry);
                }
            }

            closedir(Value);
#endif
			return true;
		}
		bool OS::StateResource(const std::string& Path, Resource* Resource)
		{
			if (!Resource)
				return false;

			memset(Resource, 0, sizeof(*Resource));
#if defined(THAWK_MICROSOFT)

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
																																	struct stat State{};
            if (stat(Path.c_str(), &State) != 0)
                return false;

            struct tm* Time = localtime(&State.st_ctime);
            Resource->CreationTime = mktime(Time);
            Resource->Size = (uint64_t)(State.st_size);
            Resource->LastModified = State.st_mtime;
            Resource->IsDirectory = S_ISDIR(State.st_mode);

            return true;
#endif
		}
		bool OS::ConstructETag(char* Buffer, uint64_t Length, Resource* Resource)
		{
			if (Resource != nullptr)
				ConstructETagManually(Buffer, Length, Resource->LastModified, Resource->Size);

			return true;
		}
		bool OS::ConstructETagManually(char* Buffer, uint64_t Length, int64_t LastModified, uint64_t ContentLength)
		{
			if (Buffer != nullptr && Length > 0)
				snprintf(Buffer, (const size_t)Length, "\"%lx.%llu\"", (unsigned long)LastModified, ContentLength);

			return true;
		}
		bool OS::UnloadObject(void* Handle)
		{
#ifdef THAWK_HAS_SDL2
			if (!Handle)
				return false;

			SDL_UnloadObject(Handle);
			return true;
#else
			return false;
#endif
		}
		bool OS::SpawnProcess(const std::string& Path, const std::vector<std::string>& Params, ChildProcess* Child)
		{
#ifdef THAWK_MICROSOFT
			HANDLE Job = CreateJobObject(nullptr, nullptr);
			if (Job == nullptr)
			{
				THAWK_ERROR("cannot create job object for process");
				return false;
			}

			JOBOBJECT_EXTENDED_LIMIT_INFORMATION Info = { 0 };
			Info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
			if (SetInformationJobObject(Job, JobObjectExtendedLimitInformation, &Info, sizeof(Info)) == 0)
			{
				THAWK_ERROR("cannot set job object for process");
				return false;
			}

			STARTUPINFO StartupInfo;
			ZeroMemory(&StartupInfo, sizeof(StartupInfo));
			StartupInfo.cb = sizeof(StartupInfo);
			StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
			StartupInfo.wShowWindow = SW_HIDE;

			PROCESS_INFORMATION Process;
			ZeroMemory(&Process, sizeof(Process));

			Stroke Exe = Resolve(Path.c_str());
			if (!Exe.EndsWith(".exe"))
				Exe.Append(".exe");

			Stroke Args = Form("\"%s\"", Exe.Get());
			for (const auto& Param : Params)
				Args.Append(' ').Append(Param);

			if (!CreateProcessA(Exe.Get(), Args.Value(), nullptr, nullptr, TRUE, CREATE_BREAKAWAY_FROM_JOB | HIGH_PRIORITY_CLASS, NULL, NULL, &StartupInfo, &Process))
			{
				THAWK_ERROR("cannot spawn process %s", Exe.Get());
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
																																	if (!FileExists(Path.c_str()))
            {
                THAWK_ERROR("cannot spawn process %s (file does not exists)", Path.c_str());
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
		bool OS::FreeProcess(ChildProcess* Child)
		{
			if (!Child || !Child->Valid)
				return false;

#ifdef THAWK_MICROSOFT
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
		bool OS::AwaitProcess(ChildProcess* Process, int* ExitCode)
		{
			if (!Process || !Process->Valid)
				return false;

#ifdef THAWK_MICROSOFT
			WaitForSingleObject(Process->Process, INFINITE);
			if (ExitCode != nullptr)
			{
				DWORD Result;
				if (!GetExitCodeProcess(Process->Process, &Result))
				{
					FreeProcess(Process);
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

			FreeProcess(Process);
			return true;
		}
		socket_t OS::GetFD(FILE* Stream)
		{
			if (!Stream)
				return -1;

#ifdef THAWK_MICROSOFT
			return _fileno(Stream);
#else
			return fileno(Stream);
#endif
		}
		int OS::GetError()
		{
#ifdef THAWK_MICROSOFT
			int ErrorCode = WSAGetLastError();
			WSASetLastError(0);

			return ErrorCode;
#else
																																	int ErrorCode = errno;
            errno = 0;

            return ErrorCode;
#endif
		}
		std::string OS::GetErrorName(int Code)
		{
#ifdef THAWK_MICROSOFT
			LPSTR Buffer = nullptr;
			size_t Size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, Code, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPSTR)&Buffer, 0, nullptr);
			std::string Result(Buffer, Size);
			LocalFree(Buffer);

			return Stroke(&Result).Replace("\r", "").Replace("\n", "").R();
#else
																																	char Buffer[1024];
            strerror_r(Code, Buffer, sizeof(Buffer));

            return Buffer;
#endif
		}
		void OS::Run(const char* Format, ...)
		{
			char Buffer[16384];

			va_list Args;
					va_start(Args, Format);
#ifdef THAWK_MICROSOFT
			_vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#else
			vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#endif
					va_end(Args);
			system(Buffer);
		}
		void* OS::LoadObject(const char* Path)
		{
#ifdef THAWK_HAS_SDL2
			if (!Path)
				return nullptr;

			return SDL_LoadObject(Path);
#else
			return nullptr;
#endif
		}
		void* OS::LoadObjectFunction(void* Handle, const char* Name)
		{
#ifdef THAWK_HAS_SDL2
			if (!Handle || !Name)
				return nullptr;

			return SDL_LoadFunction(Handle, Name);
#else
			return nullptr;
#endif
		}
		void* OS::Open(const char* Path, const char* Mode)
		{
			if (!Path || !Mode)
				return nullptr;

#ifdef THAWK_MICROSOFT
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
		bool OS::SendFile(FILE* Stream, socket_t Socket, int64_t Size)
		{
			if (!Stream || !Size)
				return false;

#ifdef THAWK_MICROSOFT
			return TransmitFile((SOCKET)Socket, (HANDLE)_get_osfhandle(_fileno(Stream)), (DWORD)Size, 16384, nullptr, nullptr, 0) > 0;
#elif defined(THAWK_APPLE)
			return sendfile(fileno(Stream), Socket, 0, (off_t*)&Size, nullptr, 0);
#elif defined(THAWK_UNIX)
            return sendfile(Socket, fileno(Stream), nullptr, (size_t)Size) > 0;
#else
            return false;
#endif
		}
		std::string OS::FileDirectory(const std::string& Path, int Level)
		{
			Stroke RV(Path);
			Stroke::Settle Result = RV.ReverseFindOf("/\\");
			if (!Result.Found)
				return Path;

			for (int i = 0; i < Level; i++)
			{
				Stroke::Settle Current = RV.ReverseFindOf("/\\", Path.size() - Result.Start);
				if (!Current.Found)
				{
					if (RV.Splice(0, Result.End).Empty())
						return "/";

					return RV.R();
				}

				Result = Current;
			}

			if (RV.Splice(0, Result.End).Empty())
				return "/";

			return RV.R();
		}
		std::string OS::GetFilename(const std::string& Path)
		{
			Stroke RV(Path);
			Stroke::Settle Result = RV.ReverseFindOf("/\\");
			if (!Result.Found)
				return Path;

			return RV.Substring(Result.End).R();
		}
		std::string OS::Read(const char* Path)
		{
			uint64_t Length = 0;
			char* Data = (char*)ReadAllBytes(Path, &Length);
			if (!Data)
				return std::string();

			std::string Output = std::string(Data, Length);
			delete[] Data;

			return Output;
		}
		std::string OS::Resolve(const char* Path)
		{
			if (!Path || Path[0] == '\0')
				return "";

#ifdef THAWK_MICROSOFT
			char Buffer[2048] = { 0 };
			GetFullPathNameA(Path, sizeof(Buffer), Buffer, nullptr);
#elif defined THAWK_UNIX
			char* Data = realpath(Path, nullptr);
            if (!Data)
                return Path;

            std::string Buffer = Data;
            free(Data);
#endif
			return Buffer;
		}
		std::string OS::Resolve(const std::string& Path, const std::string& Directory)
		{
			if (Path.empty() || Directory.empty())
				return "";

			if (Stroke(&Path).StartsOf("/\\"))
				return Resolve(("." + Path).c_str());

			if (Stroke(&Directory).EndsOf("/\\"))
				return Resolve((Directory + Path).c_str());

#ifdef THAWK_MICROSOFT
			return Resolve((Directory + "\\" + Path).c_str());
#else
			return Resolve((Directory + "/" + Path).c_str());
#endif
		}
		std::string OS::ResolveDir(const char* Path)
		{
			std::string Result = Resolve(Path);
			if (!Result.empty() && !Stroke(&Result).EndsOf("/\\"))
			{
#ifdef THAWK_MICROSOFT
				Result += '\\';
#else
				Result += '/';
#endif
			}

			return Result;
		}
		std::string OS::ResolveDir(const std::string& Path, const std::string& Directory)
		{
			std::string Result = Resolve(Path, Directory);
			if (!Result.empty() && !Stroke(&Result).EndsOf("/\\"))
				Result += '/';

			return Result;
		}
		std::string OS::GetDirectory()
		{
#ifndef THAWK_HAS_SDL2
			char Buffer[THAWK_MAX_PATH + 1] = { 0 };
#ifdef THAWK_MICROSOFT
            GetModuleFileNameA(nullptr, Buffer, THAWK_MAX_PATH);

			std::string Result = FileDirectory(Buffer);
			memcpy(Buffer, Result.c_str(), sizeof(char) * Result.size());

			if (Result.size() < THAWK_MAX_PATH)
				Buffer[Result.size()] = '\0';
#elif defined THAWK_UNIX
            getcwd(Buffer, THAWK_MAX_PATH);
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
		FileState OS::GetState(const char* Path)
		{
			FileState State{ };
			struct stat Buffer{ };

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
		std::vector<std::string> OS::ReadAllLines(const char* Path)
		{
			FILE* Stream = (FILE*)Open(Path, "rb");
			if (!Stream)
				return std::vector<std::string>();

			fseek(Stream, 0, SEEK_END);
			uint64_t Length = ftell(Stream);
			fseek(Stream, 0, SEEK_SET);

			char* Buffer = (char*)malloc(sizeof(char) * Length);
			if (fread(Buffer, sizeof(char), (size_t)Length, Stream) != (size_t)Length)
			{
				fclose(Stream);
				free(Buffer);
				return std::vector<std::string>();
			}

			std::string Result(Buffer, Length);
			fclose(Stream);
			free(Buffer);

			return Stroke(&Result).Split('\n');
		}
		std::vector<std::string> OS::GetDiskDrives()
		{
			std::vector<std::string> Output;
#ifdef THAWK_MICROSOFT
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
		const char* OS::FileExtention(const char* Path)
		{
			if (!Path)
				return nullptr;

			const char* Buffer = Path;
			while (*Buffer != '\0')
				Buffer++;

			while (*Buffer != '.' || Buffer != Path)
				Buffer--;

			if (Buffer == Path)
				return nullptr;

			return Buffer;
		}
		unsigned char* OS::ReadAllBytes(const char* Path, uint64_t* Length)
		{
			FILE* Stream = (FILE*)Open(Path, "rb");
			if (!Stream)
				return nullptr;

			fseek(Stream, 0, SEEK_END);
			uint64_t Size = ftell(Stream);
			fseek(Stream, 0, SEEK_SET);

			auto* Bytes = (unsigned char*)malloc(sizeof(unsigned char) * (size_t)(Size + 1));
			if (fread((char*)Bytes, sizeof(unsigned char), (size_t)Size, Stream) != (size_t)Size)
			{
				fclose(Stream);
				free(Bytes);

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
		unsigned char* OS::ReadAllBytes(FileStream* Stream, uint64_t* Length)
		{
			if (!Stream)
				return nullptr;

			uint64_t Size = Stream->Size();

			auto* Bytes = new unsigned char[(size_t)(Size + 1)];
			Stream->Read((char*)Bytes, Size * sizeof(unsigned char));
			Bytes[Size] = '\0';

			if (Length != nullptr)
				*Length = Size;

			return Bytes;
		}
		unsigned char* OS::ReadByteChunk(FileStream* Stream, uint64_t Length)
		{
			auto* Bytes = (unsigned char*)malloc((size_t)(Length + 1));
			Stream->Read((char*)Bytes, Length);

			Bytes[Length] = '\0';
			return Bytes;
		}
		bool OS::WantTextInput(const std::string& Title, const std::string& Message, const std::string& DefaultInput, std::string* Result)
		{
			const char* Data = tinyfd_inputBox(Title.c_str(), Message.c_str(), DefaultInput.c_str());
			if (!Data)
				return false;

			if (!Result)
				*Result = Data;

			return true;
		}
		bool OS::WantPasswordInput(const std::string& Title, const std::string& Message, std::string* Result)
		{
			const char* Data = tinyfd_inputBox(Title.c_str(), Message.c_str(), nullptr);
			if (!Data)
				return false;

			if (Result != nullptr)
				*Result = Data;

			return true;
		}
		bool OS::WantFileSave(const std::string& Title, const std::string& DefaultPath, const std::string& Filter, const std::string& FilterDescription, std::string* Result)
		{
			std::vector<char*> Patterns;
			for (auto& It : Stroke(&Filter).Split(','))
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
		bool OS::WantFileOpen(const std::string& Title, const std::string& DefaultPath, const std::string& Filter, const std::string& FilterDescription, bool Multiple, std::string* Result)
		{
			std::vector<char*> Patterns;
			for (auto& It : Stroke(&Filter).Split(','))
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
		bool OS::WantFolder(const std::string& Title, const std::string& DefaultPath, std::string* Result)
		{
			const char* Data = tinyfd_selectFolderDialog(Title.c_str(), DefaultPath.c_str());
			if (!Data)
				return false;

			if (Result != nullptr)
				*Result = Data;

			return true;
		}
		bool OS::WantColor(const std::string& Title, const std::string& DefaultHexRGB, std::string* Result)
		{
			unsigned char RGB[3] = { 0, 0, 0 };
			const char* Data = tinyfd_colorChooser(Title.c_str(), DefaultHexRGB.c_str(), RGB, RGB);
			if (!Data)
				return false;

			if (Result != nullptr)
				*Result = Data;

			return true;
		}
		uint64_t OS::CheckSum(const std::string& Data)
		{
			int64_t Result = 0xFFFFFFFF;
			int64_t Index = 0;
			int64_t Byte = 0;
			int64_t Mask = 0;
			int64_t It = 0;

			while (Data[Index] != 0)
			{
				Byte = Data[Index];
				Result = Result ^ Byte;

				for (It = 7; It >= 0; It--)
				{
					Mask = -(Result & 1);
					Result = (Result >> 1) ^ (0xEDB88320 & Mask);
				}

				Index++;
			}

			return (uint64_t)~Result;
		}

		FileLogger::FileLogger(const std::string& Root) : Path(Root), Offset(-1)
		{
			Stream = new FileStream();
			auto V = Stroke(&Path).Replace("/", "\\").Split('\\');
			if (!V.empty())
				Name = V.back();
		}
		FileLogger::~FileLogger()
		{
			delete Stream;
		}
		void FileLogger::Process(const std::function<bool(FileLogger*, const char*, int64_t)>& Callback)
		{
			Stream->Open(Path.c_str(), FileMode_Binary_Read_Only);

			uint64_t Length = Stream->Size();
			if (Length <= Offset || Offset <= 0)
			{
				Offset = Length;
				return;
			}

			int64_t Delta = Length - Offset;
			Stream->Seek(FileSeek_Begin, Length - Delta);

			char* Data = (char*)malloc(sizeof(char) * ((size_t)Delta + 1));
			Stream->Read(Data, sizeof(char) * Delta);

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

			auto V = Stroke(&Value).Substring(0, ValueLength).Replace("\t", "\n").Replace("\n\n", "\n").Replace("\r", "");
			delete[] Data;

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

		EventWorker::EventWorker(EventQueue* Value, bool IsTask) : Queue(Value), Extended(IsTask)
		{
			Thread = std::thread(Extended ? &EventWorker::QueueTask : &EventWorker::QueueEvent, this);
		}
		EventWorker::~EventWorker()
		{
			if (Queue != nullptr)
				Queue->Async.Condition[Extended ? 0 : 1].notify_all();

			if (Thread.get_id() != std::this_thread::get_id() && Thread.joinable())
				Thread.join();
		}
		bool EventWorker::QueueTask()
		{
			if (!Queue)
				return false;

			std::condition_variable* Condition = &Queue->Async.Condition[0];
			if (Queue->State == EventState_Idle)
			{
				std::unique_lock<std::mutex> Lock(Queue->Async.Safe[0]);
				Condition->wait(Lock);
			}

			do
			{
				if (!Queue->QueueTask(this))
				{
					std::unique_lock<std::mutex> Lock(Queue->Async.Safe[0]);
					Condition->wait(Lock);
				}
			}while (Queue->State == EventState_Working);

			return true;
		}
		bool EventWorker::QueueEvent()
		{
			if (!Queue)
				return false;

			std::condition_variable* Condition = &Queue->Async.Condition[1];
			if (Queue->State == EventState_Idle)
			{
				std::unique_lock<std::mutex> Lock(Queue->Async.Safe[1]);
				Condition->wait(Lock);
			}

			do
			{
				if (!Queue->QueueEvent(this))
				{
					std::unique_lock<std::mutex> Lock(Queue->Async.Safe[1]);
					Condition->wait(Lock);
				}
			}while (Queue->State == EventState_Working);

			return true;
		}

		EventQueue::EventQueue()
		{
			State = EventState_Terminated;
			Idle = 1;
		}
		EventQueue::~EventQueue()
		{
			if (State != EventState_Terminated)
			{
				State = EventState_Idle;
				Stop();
			}
			else if (Async.Thread[0].get_id() != std::this_thread::get_id() && Async.Thread[0].joinable())
			{
				Async.Thread[0].join();
				if (Async.Thread[1].get_id() != std::this_thread::get_id() && Async.Thread[1].joinable())
					Async.Thread[1].join();
			}
		}
		void EventQueue::SetIdleTime(uint64_t IdleTime)
		{
			Idle = IdleTime;
		}
		void EventQueue::SetState(EventState NewState)
		{
			State = NewState;
		}
		bool EventQueue::Tick()
		{
			if (!Tasks.empty() && Async.Workers[0].empty())
				QueueTask(nullptr);

			if (!Events.empty())
				QueueEvent(nullptr);

			if (!Timers.empty())
				QueueTimer(Clock());

			return true;
		}
		bool EventQueue::Start(EventWorkflow Workflow, uint64_t TaskWorkers, uint64_t EventWorkers)
		{
			Synchronize = (int)Workflow;
			if (State != EventState_Terminated || Async.Thread[0].joinable())
				return false;
			else
				State = EventState_Idle;

			Async.Workers[0].reserve(TaskWorkers);
			for (uint64_t i = 0; i < TaskWorkers; i++)
				Async.Workers[0].push_back(new EventWorker(this, true));

			Async.Workers[1].reserve(EventWorkers);
			for (uint64_t i = 0; i < EventWorkers; i++)
				Async.Workers[1].push_back(new EventWorker(this, false));

			State = EventState_Working;
			if (Workflow == EventWorkflow_Multithreaded)
			{
				Async.Thread[0] = std::thread(&EventQueue::DefaultLoop, this);
				Async.Thread[1] = std::thread(&EventQueue::TimerLoop, this);
			}
			else if (Workflow == EventWorkflow_Mixed)
				Async.Thread[0] = std::thread(&EventQueue::MixedLoop, this);
			else if (Workflow == EventWorkflow_Singlethreaded)
				return MixedLoop();

			return true;
		}
		bool EventQueue::Stop()
		{
			if (State != EventState_Idle && Synchronize != 1)
			{
				if (State == EventState_Terminated)
					return false;

				State = EventState_Idle;
				return true;
			}

			State = EventState_Terminated;
			if (Synchronize != 1)
			{
				if (Async.Thread[0].get_id() != std::this_thread::get_id() && Async.Thread[0].joinable())
					Async.Thread[0].join();

				if (Synchronize == 0)
				{
					if (Async.Thread[1].get_id() != std::this_thread::get_id() && Async.Thread[1].joinable())
						Async.Thread[1].join();
				}
			}

			State = EventState_Terminated;
			for (auto& Worker : Async.Workers)
			{
				for (auto It = Worker.begin(); It != Worker.end(); It++)
					delete *It;
				Worker.clear();
			}

			while (!Tasks.empty())
				RemoveAny(EventType_Tasks, 0, nullptr, false);

			while (!Events.empty())
				RemoveAny(EventType_Events, 0, nullptr, false);

			while (!Timers.empty())
				RemoveAny(EventType_Timers, 0, nullptr, false);

			for (auto& Task : Tasks)
				delete Task;
			Tasks.clear();

			for (auto& Event : Events)
				delete Event;
			Events.clear();

			for (auto& Event : Timers)
				delete Event;
			Timers.clear();

			for (auto& Listener : Listeners)
				delete Listener.second;
			Listeners.clear();

			return true;
		}
		bool EventQueue::Expire(uint64_t TimerId)
		{
			Sync.Timers.lock();
			for (auto It = Timers.begin(); It != Timers.end(); It++)
			{
				EventTimer* Value = *It;
				if (Value->Id != TimerId)
					continue;

				Value->Args.Alive = false;
				Timers.erase(Timers.begin());
				delete Value;

				Sync.Timers.unlock();
				return true;
			}

			Sync.Timers.unlock();
			return false;
		}
		bool EventQueue::MixedLoop()
		{
			if (!Async.Workers[0].empty())
				Async.Condition[0].notify_all();

			if (!Async.Workers[1].empty())
				Async.Condition[1].notify_all();

			while (State == EventState_Working)
			{
				bool Overhead = true;
				if (!Tasks.empty() && Async.Workers[0].empty())
					Overhead = (QueueTask(nullptr) ? false : Overhead);

				if (!Events.empty())
					Overhead = (QueueEvent(nullptr) ? false : Overhead);

				if (!Timers.empty())
					Overhead = (QueueTimer(Clock()) ? false : Overhead);

				if (Overhead && Idle > 0)
					std::this_thread::sleep_for(std::chrono::milliseconds(Idle));
			}

			return true;
		}
		bool EventQueue::DefaultLoop()
		{
			if (!Async.Workers[0].empty())
				Async.Condition[0].notify_all();

			if (!Async.Workers[1].empty())
				Async.Condition[1].notify_all();

			while (State == EventState_Working)
			{
				bool Overhead = true;
				if (!Tasks.empty() && Async.Workers[0].empty())
					Overhead = (QueueTask(nullptr) ? false : Overhead);

				if (!Events.empty())
					Overhead = (QueueEvent(nullptr) ? false : Overhead);

				if (Overhead && Idle > 0)
					std::this_thread::sleep_for(std::chrono::milliseconds(Idle));
			}

			return true;
		}
		bool EventQueue::TimerLoop()
		{
			while (State == EventState_Working)
			{
				bool Overhead = true;
				if (!Timers.empty())
					Overhead = !QueueTimer(Clock());

				if (Overhead && Idle > 0)
					std::this_thread::sleep_for(std::chrono::milliseconds(Idle));
			}

			return true;
		}
		bool EventQueue::QueueEvent(EventWorker* Worker)
		{
			if (Events.empty())
				return false;

			Sync.Events.lock();
			EventBase* Event = nullptr;
			if (!Events.empty())
			{
				Event = Events.front();
				Events.pop_front();
			}
			Sync.Events.unlock();

			if (!Event)
				return false;

			Event->Args.Worker = Worker;
			if (Event->Callback)
				Event->Callback(this, (EventArgs*)&Event->Args);

			if (Event->Args.Alive)
				return AddEvent(Event);

			delete Event;
			return true;
		}
		bool EventQueue::QueueTask(EventWorker* Worker)
		{
			if (Tasks.empty())
				return false;

			Sync.Tasks.lock();
			EventBase* Task = nullptr;
			if (!Tasks.empty())
			{
				Task = Tasks.front();
				Tasks.pop_front();
			}
			Sync.Tasks.unlock();

			if (!Task)
				return false;

			Task->Args.Worker = Worker;
			if (Task->Callback)
				Task->Callback(this, (EventArgs*)&Task->Args);

			if (Task->Args.Alive)
				return AddTask(Task);

			delete Task;
			return true;
		}
		bool EventQueue::QueueTimer(int64_t Time)
		{
			Sync.Timers.lock();
			for (auto It = Timers.begin(); It != Timers.end(); It++)
			{
				EventTimer* Element = *It;
				if (Time - Element->Time < (int64_t)Element->Timeout)
					continue;

				Element->Time = Time;
				if (!Element->Args.Alive)
					Timers.erase(It);

				Sync.Timers.unlock();
				if (Element->Callback)
					Callback(Element->Args.Data, Element->Callback);

				if (!Element->Args.Alive)
					delete Element;

				return true;
			}

			Sync.Timers.unlock();
			return false;
		}
		bool EventQueue::AddEvent(EventBase* Value)
		{
			if (State == EventState_Idle)
			{
				delete Value;
				return false;
			}

			Sync.Events.lock();
			Events.push_back(Value);
			Sync.Events.unlock();

			if (State != EventState_Working)
				return true;

			if (!Async.Workers[1].empty())
				Async.Condition[1].notify_one();

			return true;
		}
		bool EventQueue::AddTask(EventBase* Value)
		{
			if (State == EventState_Idle)
			{
				delete Value;
				return false;
			}

			Sync.Tasks.lock();
			Tasks.push_back(Value);
			Sync.Tasks.unlock();

			if (State != EventState_Working)
				return true;

			if (!Async.Workers[0].empty())
				Async.Condition[0].notify_one();

			return true;
		}
		bool EventQueue::AddTimer(EventTimer* Value)
		{
			if (State == EventState_Idle)
			{
				delete Value;
				return false;
			}

			Sync.Timers.lock();
			Timers.push_back(Value);
			Sync.Timers.unlock();

			return true;
		}
		bool EventQueue::AddListener(EventListener* Value)
		{
			if (!Value)
				return false;
			else
				Sync.Listeners.lock();

			auto It = Listeners.find(Value->Type);
			if (It != Listeners.end())
			{
				delete It->second;
				It->second = Value;
			}
			else
				Listeners[Value->Type] = Value;

			Sync.Listeners.unlock();
			return false;
		}
		bool EventQueue::RemoveListener(uint64_t Type)
		{
			Sync.Listeners.lock();
			auto It = Listeners.find(Type);
			if (It == Listeners.end())
			{
				Sync.Listeners.unlock();
				return false;
			}

			delete It->second;
			Listeners.erase(It);
			Sync.Listeners.unlock();
			return false;
		}
		bool EventQueue::RemoveCallback(EventBase* Value)
		{
			if (!Value)
				return false;

			Sync.Listeners.lock();
			auto It = Listeners.find(Value->Type);
			if (It == Listeners.end())
			{
				Sync.Listeners.unlock();
				return false;
			}

			Value->Callback = It->second->Callback;
			Sync.Listeners.unlock();

			return true;
		}
		bool EventQueue::RemoveAny(EventType Type, uint64_t Hash, const PullCallback& Callback, bool NoCall)
		{
			if ((Type & EventType_Tasks) && !Tasks.empty())
			{
				Sync.Tasks.lock();
				for (auto It = Tasks.begin(); It != Tasks.end(); It++)
				{
					EventBase* Value = *It;
					if (Value->Type != Hash && Hash > 0)
						continue;

					if (Callback && !Callback(this, &Value->Args))
						continue;

					Value->Args.Alive = false;
					Tasks.erase(It--);

					Sync.Tasks.unlock();
					if (!NoCall && Value->Callback)
						Value->Callback(this, (EventArgs*)&Value->Args);

					delete Value;
					return true;
				}
				Sync.Tasks.unlock();
			}

			if ((Type & EventType_Events) && !Events.empty())
			{
				Sync.Events.lock();
				for (auto It = Events.begin(); It != Events.end(); It++)
				{
					EventBase* Value = *It;
					if (Value->Type != Hash && Hash > 0)
						continue;

					if (Callback && !Callback(this, &Value->Args))
						continue;

					Value->Args.Alive = false;
					Events.erase(It--);

					Sync.Events.unlock();
					if (!NoCall && Value->Callback)
						Value->Callback(this, (EventArgs*)&Value->Args);

					delete Value;
					return true;
				}
				Sync.Events.unlock();
			}

			if ((Type & EventType_Timers) && !Timers.empty())
			{
				Sync.Timers.lock();
				for (auto It = Timers.begin(); It != Timers.end(); It++)
				{
					EventTimer* Value = *It;
					if ((Value->Args.Hash != Hash && Hash > 0))
						continue;

					if (Callback && !Callback(this, &Value->Args))
						continue;

					Value->Args.Alive = false;
					Timers.erase(Timers.begin());

					Sync.Timers.unlock();
					if (!NoCall && Value->Callback)
						Value->Callback(this, (EventArgs*)&Value->Args);

					delete Value;
					return true;
				}
				Sync.Timers.unlock();
			}

			return false;
		}
		int64_t EventQueue::Clock()
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		}
		EventState EventQueue::GetState()
		{
			return State;
		}

		Document::Document() : Parent(nullptr), Type(NodeType_Object), Low(0), Integer(0), Number(0.0), Boolean(false), Saved(true)
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

				if (Type == NodeType_Array)
				{
					Nodes.push_back(Copy);
					continue;
				}

				bool Exists = false;
				for (auto It = Nodes.begin(); It != Nodes.end(); It++)
				{
					if (!*It || (*It)->Name != Copy->Name)
						continue;

					(*It)->Parent = nullptr;
					delete *It;
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
					delete Document;
				}
			}

			Nodes.clear();
		}
		void Document::Save()
		{
			for (auto& It : Nodes)
			{
				if (It->Type == NodeType_Array || It->Type == NodeType_Object)
					It->Save();
				else
					It->Saved = true;
			}

			Saved = true;
		}
		Document* Document::GetIndex(int64_t Index)
		{
			if (Index < 0 || Index >= Nodes.size())
				return nullptr;

			return Nodes[Index];
		}
		Document* Document::Get(const std::string& Label)
		{
			if (Label.empty())
				return nullptr;

			for (auto Document : Nodes)
			{
				if (Document->Name == Label)
					return Document;
			}

			return nullptr;
		}
		Document* Document::SetCast(const std::string& Label, const std::string& Prop)
		{
			Document* Value = new Document();
			if (!Value->Deserialize(Prop))
			{
				delete Value;
				return SetNull(Label);
			}

			Value->Saved = false;
			Value->Parent = this;
			Value->Name.assign(Label);
			Saved = false;

			if (Type != NodeType_Array)
			{
				for (auto It = Nodes.begin(); It != Nodes.end(); It++)
				{
					if (!*It || (*It)->Name != Label)
						continue;

					(*It)->Parent = nullptr;
					delete *It;
					*It = Value;

					return Value;
				}
			}

			Nodes.push_back(Value);
			return Value;
		}
		Document* Document::SetUndefined(const std::string& Label)
		{
			for (auto It = Nodes.begin(); It != Nodes.end(); It++)
			{
				if (!*It || (*It)->Name != Label)
					continue;

				(*It)->Parent = nullptr;
				delete *It;
				Nodes.erase(It);
				break;
			}

			return nullptr;
		}
		Document* Document::SetNull(const std::string& Label)
		{
			Document* Duplicate = Get(Label);
			Saved = false;

			if (Duplicate != nullptr)
			{
				Duplicate->Type = NodeType_Null;
				Duplicate->Saved = false;
				Duplicate->Name.assign(Label);

				return Duplicate;
			}

			Duplicate = new Document();
			Duplicate->Type = NodeType_Null;
			Duplicate->Saved = false;
			Duplicate->Parent = this;
			Duplicate->Name.assign(Label);

			Nodes.push_back(Duplicate);
			return Duplicate;
		}
		Document* Document::SetId(const std::string& Label, unsigned char Value[12])
		{
			Document* Duplicate = Get(Label);
			Saved = false;

			if (Duplicate != nullptr)
			{
				Duplicate->Type = NodeType_Id;
				Duplicate->String.assign((const char*)Value, 12);
				Duplicate->Saved = false;
				Duplicate->Name.assign(Label);

				return Duplicate;
			}

			Duplicate = new Document();
			Duplicate->Type = NodeType_Id;
			Duplicate->String.assign((const char*)Value, 12);
			Duplicate->Saved = false;
			Duplicate->Parent = this;
			Duplicate->Name.assign(Label);

			Nodes.push_back(Duplicate);
			return Duplicate;
		}
		Document* Document::SetDocument(const std::string& Label, Document* Value)
		{
			if (!Value)
				return SetNull(Label);

			Value->Type = NodeType_Object;
			Value->Saved = false;
			Value->Parent = this;
			Value->Name.assign(Label);
			Saved = false;

			if (Type != NodeType_Array)
			{
				for (auto It = Nodes.begin(); It != Nodes.end(); It++)
				{
					if (!*It || (*It)->Name != Label)
						continue;

					(*It)->Parent = nullptr;
					delete *It;
					*It = Value;

					return Value;
				}
			}

			Nodes.push_back(Value);
			return Value;
		}
		Document* Document::SetDocument(const std::string& Label)
		{
			return SetDocument(Label, new Document());
		}
		Document* Document::SetArray(const std::string& Label, Document* Value)
		{
			if (!Value)
				return SetNull(Label);

			Value->Type = NodeType_Array;
			Value->Saved = false;
			Value->Parent = this;
			Value->Name.assign(Label);
			Saved = false;

			if (Type != NodeType_Array)
			{
				for (auto It = Nodes.begin(); It != Nodes.end(); It++)
				{
					if (!*It || (*It)->Name != Label)
						continue;

					(*It)->Parent = nullptr;
					delete *It;
					*It = Value;

					return Value;
				}
			}

			Nodes.push_back(Value);
			return Value;
		}
		Document* Document::SetArray(const std::string& Label)
		{
			return SetArray(Label, new Document());
		}
		Document* Document::SetAttribute(const std::string& Label, const std::string& Value)
		{
			return SetCast("[" + Label + "]", Value);
		}
		Document* Document::SetString(const std::string& Label, const char* Value, int64_t Size)
		{
			if (!Value)
				return SetNull(Label);

			Document* Duplicate = Get(Label);
			Saved = false;

			if (Duplicate != nullptr)
			{
				Duplicate->Type = NodeType_String;
				Duplicate->String.assign(Value, (size_t)(Size < 0 ? strlen(Value) : Size));
				Duplicate->Saved = false;
				Duplicate->Name.assign(Label);

				return Duplicate;
			}

			Duplicate = new Document();
			Duplicate->Type = NodeType_String;
			Duplicate->String.assign(Value, (size_t)(Size < 0 ? strlen(Value) : Size));
			Duplicate->Saved = false;
			Duplicate->Parent = this;
			Duplicate->Name.assign(Label);

			Nodes.push_back(Duplicate);
			return Duplicate;
		}
		Document* Document::SetString(const std::string& Label, const std::string& Value)
		{
			Document* Duplicate = Get(Label);
			Saved = false;

			if (Duplicate != nullptr)
			{
				Duplicate->Type = NodeType_String;
				Duplicate->String.assign(Value);
				Duplicate->Saved = false;
				Duplicate->Name.assign(Label);

				return Duplicate;
			}

			Duplicate = new Document();
			Duplicate->Type = NodeType_String;
			Duplicate->String.assign(Value);
			Duplicate->Saved = false;
			Duplicate->Parent = this;
			Duplicate->Name.assign(Label);

			Nodes.push_back(Duplicate);
			return Duplicate;
		}
		Document* Document::SetInteger(const std::string& Label, int64_t Value)
		{
			Document* Duplicate = Get(Label);
			Saved = false;

			if (Duplicate != nullptr)
			{
				Duplicate->Type = NodeType_Integer;
				Duplicate->Integer = Value;
				Duplicate->Number = (double)Value;
				Duplicate->Saved = false;
				Duplicate->Name.assign(Label);

				return Duplicate;
			}

			Duplicate = new Document();
			Duplicate->Type = NodeType_Integer;
			Duplicate->Integer = Value;
			Duplicate->Number = (double)Value;
			Duplicate->Saved = false;
			Duplicate->Parent = this;
			Duplicate->Name.assign(Label);

			Nodes.push_back(Duplicate);
			return Duplicate;
		}
		Document* Document::SetNumber(const std::string& Label, double Value)
		{
			Document* Duplicate = Get(Label);
			Saved = false;

			if (Duplicate != nullptr)
			{
				Duplicate->Type = NodeType_Number;
				Duplicate->Integer = (int64_t)Value;
				Duplicate->Number = Value;
				Duplicate->Saved = false;
				Duplicate->Name.assign(Label);

				return Duplicate;
			}

			Duplicate = new Document();
			Duplicate->Type = NodeType_Number;
			Duplicate->Integer = (int64_t)Value;
			Duplicate->Number = Value;
			Duplicate->Saved = false;
			Duplicate->Parent = this;
			Duplicate->Name.assign(Label);

			Nodes.push_back(Duplicate);
			return Duplicate;
		}
		Document* Document::SetDecimal(const std::string& Label, int64_t fHigh, int64_t fLow)
		{
			Document* Duplicate = Get(Label);
			Saved = false;

			if (Duplicate != nullptr)
			{
				Duplicate->Type = NodeType_Decimal;
				Duplicate->Integer = fHigh;
				Duplicate->Low = fLow;
				Duplicate->Saved = false;
				Duplicate->Name.assign(Label);

				return Duplicate;
			}

			Duplicate = new Document();
			Duplicate->Type = NodeType_Decimal;
			Duplicate->Integer = fHigh;
			Duplicate->Low = fLow;
			Duplicate->Saved = false;
			Duplicate->Parent = this;
			Duplicate->Name.assign(Label);

			Nodes.push_back(Duplicate);
			return Duplicate;
		}
		Document* Document::SetDecimal(const std::string& Label, const std::string& Value)
		{
#ifdef THAWK_HAS_MONGOC
			int64_t fHigh, fLow;
			if (!Network::BSON::Document::ParseDecimal(Value.c_str(), &fHigh, &fLow))
				return nullptr;

			return SetDecimal(Label, fHigh, fLow);
#else
			return nullptr;
#endif
		}
		Document* Document::SetBoolean(const std::string& Label, bool Value)
		{
			Document* Duplicate = Get(Label);
			Saved = false;

			if (Duplicate != nullptr)
			{
				Duplicate->Type = NodeType_Boolean;
				Duplicate->Boolean = Value;
				Duplicate->Saved = false;
				Duplicate->Name.assign(Label);

				return Duplicate;
			}

			Duplicate = new Document();
			Duplicate->Type = NodeType_Boolean;
			Duplicate->Boolean = Value;
			Duplicate->Saved = false;
			Duplicate->Parent = this;
			Duplicate->Name.assign(Label);

			Nodes.push_back(Duplicate);
			return Duplicate;
		}
		Document* Document::Copy()
		{
			Document* New = new Document();
			New->Parent = nullptr;
			New->Name.assign(Name);
			New->String.assign(String);
			New->Type = Type;
			New->Low = Low;
			New->Integer = Integer;
			New->Number = Number;
			New->Boolean = Boolean;
			New->Saved = Saved;
			New->Nodes = Nodes;

			for (auto It = New->Nodes.begin(); It != New->Nodes.end(); It++)
			{
				if (*It != nullptr)
					*It = (*It)->Copy();
			}

			return New;
		}
		Document* Document::GetParent()
		{
			return Parent;
		}
		Document* Document::GetAttribute(const std::string& Label)
		{
			return Get("[" + Label + "]");
		}
		bool Document::IsAttribute()
		{
			if (Name.empty())
				return false;

			return (Name.front() == '[' && Name.back() == ']');
		}
		bool Document::IsObject()
		{
			return Type == NodeType_Object || Type == NodeType_Array;
		}
		bool Document::GetBoolean(const std::string& Label)
		{
			Document* Value = Get(Label);
			if (!Value || Value->Type != NodeType_Boolean)
				return false;

			return Value->Boolean;
		}
		bool Document::GetNull(const std::string& Label)
		{
			Document* Value = Get(Label);
			return !Value || Value->Type == NodeType_Null;
		}
		bool Document::GetUndefined(const std::string& Label)
		{
			Document* Value = Get(Label);
			return !Value || Value->Type == NodeType_Undefined;
		}
		int64_t Document::Size()
		{
			return (int64_t)Nodes.size();
		}
		int64_t Document::GetDecimal(const std::string& Label, int64_t* fLow)
		{
			Document* Value = Get(Label);
			if (!Value || Value->Type != NodeType_Decimal)
				return 0;

			if (fLow != nullptr)
				*fLow = Value->Low;

			return Value->Integer;
		}
		int64_t Document::GetInteger(const std::string& Label)
		{
			Document* Value = Get(Label);
			if (!Value)
				return 0;

			if (Value->Type == NodeType_Integer)
				return Value->Integer;

			if (Value->Type == NodeType_Number)
				return (int64_t)Value->Number;

			return 0;
		}
		double Document::GetNumber(const std::string& Label)
		{
			Document* Value = Get(Label);
			if (!Value)
				return 0.0;

			if (Value->Type == NodeType_Integer)
				return (double)Value->Integer;

			if (Value->Type == NodeType_Number)
				return Value->Number;

			return 0.0;
		}
		unsigned char* Document::GetId(const std::string& Label)
		{
			Document* Value = Get(Label);
			if (!Value || Value->Type != NodeType_Id || Value->String.size() != 12)
				return nullptr;

			return (unsigned char*)Value->String.c_str();
		}
		const char* Document::GetString(const std::string& Label)
		{
			Document* Value = Get(Label);
			if (!Value || Value->Type != NodeType_String)
				return nullptr;

			return Value->String.c_str();
		}
		std::string& Document::GetStringBlob(const std::string& Label)
		{
			Document* Value = Get(Label);
			if (!Value || Value->Type != NodeType_String)
				return String;

			return Value->String;
		}
		std::string Document::GetName()
		{
			return IsAttribute() ? Name.substr(1, Name.size() - 2) : Name;
		}
		Document* Document::Find(const std::string& Label, bool Here)
		{
			if (Type == NodeType_Array)
			{
				Rest::Stroke Number(&Label);
				if (Number.HasInteger())
				{
					int64_t Index = Number.ToInt64();
					if (Index >= 0 && Index < Nodes.size())
						return Nodes[Index];
				}
			}

			for (auto K : Nodes)
			{
				if (K->Name == Label)
					return K;

				if (Here)
					continue;

				Document* V = K->Find(Label);
				if (V != nullptr)
					return V;
			}

			return nullptr;
		}
		Document* Document::FindPath(const std::string& Notation, bool Here)
		{
			std::vector<std::string> Names = Stroke(Notation).Split('.');
			if (Names.empty())
				return nullptr;

			Document* Current = Find(*Names.begin(), Here);
			if (!Current)
				return nullptr;

			for (auto It = Names.begin() + 1; It != Names.end(); It++)
			{
				Current = Current->Find(*It, Here);
				if (!Current)
					return nullptr;
			}

			return Current;
		}
		std::vector<Document*> Document::FindCollection(const std::string& Label, bool Here)
		{
			std::vector<Document*> Result;
			for (auto Value : Nodes)
			{
				if (Value->Name == Label)
					Result.push_back(Value);

				if (Here)
					continue;

				std::vector<Document*> New = Value->FindCollection(Label);
				for (auto& Ji : New)
					Result.push_back(Ji);
			}

			return Result;
		}
		std::vector<Document*> Document::FindCollectionPath(const std::string& Notation, bool Here)
		{
			std::vector<std::string> Names = Stroke(Notation).Split('.');
			if (Names.empty())
				return std::vector<Document*>();

			if (Names.size() == 1)
				return FindCollection(*Names.begin());

			Document* Current = Find(*Names.begin(), Here);
			if (!Current)
				return std::vector<Document*>();

			for (auto It = Names.begin() + 1; It != Names.end() - 1; It++)
			{
				Current = Current->Find(*It, Here);
				if (!Current)
					return std::vector<Document*>();
			}

			return Current->FindCollection(*(Names.end() - 1), Here);
		}
		std::vector<Document*> Document::GetAttributes()
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
		std::unordered_map<std::string, uint64_t> Document::CreateMapping()
		{
			std::unordered_map<std::string, uint64_t> Mapping;
			uint64_t Index = 0;

			ProcessMAPRead(this, &Mapping, Index);
			return Mapping;
		}
		std::string Document::Serialize()
		{
			switch (Type)
			{
				case NodeType_Object:
				case NodeType_Array:
				case NodeType_String:
					return String;
				case NodeType_Integer:
					return std::to_string(Integer);
				case NodeType_Number:
					return std::to_string(Number);
				case NodeType_Boolean:
					return Boolean ? "true" : "false";
				case NodeType_Decimal:
				{
#ifdef THAWK_HAS_MONGOC
					Network::BSON::KeyPair Pair;
					Pair.Mod = Network::BSON::Type_Decimal;
					Pair.High = Integer;
					Pair.Low = Low;

					return Pair.ToString();
#else
					break;
#endif
				}
				case NodeType_Id:
#ifdef THAWK_HAS_MONGOC
					return THAWK_PREFIX_STR + Network::BSON::Document::OIdToString((unsigned char*)String.c_str());
#else
					return THAWK_PREFIX_STR + Compute::MathCommon::Base64Encode(String);
#endif
				case NodeType_Null:
					return THAWK_PREFIX_STR "null";
				case NodeType_Undefined:
					return THAWK_PREFIX_STR "undefined";
				default:
					break;
			}

			return "";
		}
		bool Document::Deserialize(const std::string& Value)
		{
			if (Value == THAWK_PREFIX_STR "undefined")
			{
				Type = NodeType_Undefined;
				return true;
			}

			if (Value == THAWK_PREFIX_STR "object")
			{
				Type = NodeType_Object;
				return true;
			}

			if (Value == THAWK_PREFIX_STR "array")
			{
				Type = NodeType_Array;
				return true;
			}

			if (Value == THAWK_PREFIX_STR "null")
			{
				Type = NodeType_Null;
				return true;
			}

			if (Value == "true")
			{
				Type = NodeType_Boolean;
				Boolean = true;
				return true;
			}

			if (Value == "false")
			{
				Type = NodeType_Boolean;
				Boolean = false;
				return true;
			}

			if (Value.size() == 25 && Value.front() == THAWK_PREFIX_CHAR)
			{
#ifdef THAWK_HAS_MONGOC
				std::string OId = Network::BSON::Document::StringToOId(Value.substr(1));
				if (OId.size() == 12)
				{
					Type = NodeType_Id;
					String = OId;
					return true;
				}
#else
				Type = NodeType_Id;
				String = Compute::MathCommon::Base64Decode(Value.substr(1));
				return true;
#endif
			}

			Stroke Man(&Value);
			if (Man.HasNumber())
			{
				if (Man.HasDecimal() && Network::BSON::Document::ParseDecimal(Value.c_str(), &Integer, &Low))
				{
					Type = NodeType_Decimal;
					return true;
				}

				if (Man.HasInteger())
				{
					Type = NodeType_Integer;
					Integer = Man.ToInt64();
					Number = (double)Integer;
				}
				else
				{
					Type = NodeType_Number;
					Integer = (int64_t)Number;
					Number = Man.ToFloat64();
				}

				return true;
			}

			Type = NodeType_String;
			String = Value;

			return true;
		}
		bool Document::WriteBIN(Document* Value, const NWriteCallback& Callback)
		{
			if (!Value || !Callback)
				return false;

			std::unordered_map<std::string, uint64_t> Mapping = Value->CreateMapping();
			uint64_t Set = (uint64_t)Mapping.size();

			Callback(DocumentPretty_Dummy, "\0b\0i\0n\0h\0e\0a\0d\r\n", sizeof(char) * 16);
			Callback(DocumentPretty_Dummy, (const char*)&Set, sizeof(uint64_t));

			for (auto It = Mapping.begin(); It != Mapping.end(); It++)
			{
				uint64_t Size = (uint64_t)It->first.size();
				Callback(DocumentPretty_Dummy, (const char*)&It->second, sizeof(uint64_t));
				Callback(DocumentPretty_Dummy, (const char*)&Size, sizeof(uint64_t));

				if (Size > 0)
					Callback(DocumentPretty_Dummy, It->first.c_str(), sizeof(char) * Size);
			}

			ProcessBINWrite(Value, &Mapping, Callback);
			return true;
		}
		bool Document::WriteXML(Document* Value, const NWriteCallback& Callback)
		{
			if (!Value || !Callback)
				return false;

			std::vector<Document*> Attributes = Value->GetAttributes();
			std::string Text = Value->String;

			bool Scalable = (!Text.empty() || ((int64_t)Value->Nodes.size() - (int64_t)Attributes.size()) > 0);
			Callback(DocumentPretty_Write_Tab, "", 0);
			Callback(DocumentPretty_Dummy, "<", 1);
			Callback(DocumentPretty_Dummy, Value->Name.c_str(), (int64_t)Value->Name.size());

			if (Attributes.empty())
			{
				if (Scalable)
					Callback(DocumentPretty_Dummy, ">", 1);
				else
					Callback(DocumentPretty_Dummy, " />", 3);
			}
			else
				Callback(DocumentPretty_Dummy, " ", 1);

			for (auto It = Attributes.begin(); It != Attributes.end(); It++)
			{
				std::string Name = (*It)->Name;
				std::string Blob = (*It)->Serialize();

				Callback(DocumentPretty_Dummy, Name.c_str() + 1, (int64_t)Name.size() - 2);
				Callback(DocumentPretty_Dummy, "=\"", 2);
				Callback(DocumentPretty_Dummy, Blob.c_str(), (int64_t)Blob.size());
				It++;

				if (It == Attributes.end())
				{
					if (!Scalable)
					{
						Callback(DocumentPretty_Write_Space, "\"", 1);
						Callback(DocumentPretty_Dummy, "/>", 2);
					}
					else
						Callback(DocumentPretty_Dummy, "\">", 2);
				}
				else
					Callback(DocumentPretty_Write_Space, "\"", 1);

				It--;
			}

			Callback(DocumentPretty_Tab_Increase, "", 0);
			if (!Text.empty())
			{
				if (!Value->Nodes.empty())
				{
					Callback(DocumentPretty_Write_Line, "", 0);
					Callback(DocumentPretty_Write_Tab, "", 0);
					Callback(DocumentPretty_Dummy, Text.c_str(), Text.size());
					Callback(DocumentPretty_Write_Line, "", 0);
				}
				else
					Callback(DocumentPretty_Dummy, Text.c_str(), Text.size());
			}
			else
				Callback(DocumentPretty_Write_Line, "", 0);

			for (auto&& It : Value->Nodes)
			{
				if (!It->IsAttribute())
					WriteXML(It, Callback);
			}

			Callback(DocumentPretty_Tab_Decrease, "", 0);
			if (!Scalable)
				return true;

			if (!Value->Nodes.empty())
				Callback(DocumentPretty_Write_Tab, "", 0);

			Callback(DocumentPretty_Dummy, "</", 2);
			Callback(DocumentPretty_Dummy, Value->Name.c_str(), (int64_t)Value->Name.size());
			Callback(Value->Parent ? DocumentPretty_Write_Line : DocumentPretty_Dummy, ">", 1);

			return true;
		}
		bool Document::WriteJSON(Document* Value, const NWriteCallback& Callback)
		{
			if (!Value || !Callback)
				return false;

			auto Size = Value->Nodes.size();
			size_t Offset = 0;
			bool Array = (Value->Type == NodeType_Array);

			if (Array)
			{
				for (auto&& Document : Value->Nodes)
				{
					if (Document->Name.empty())
						continue;

					Array = false;
					break;
				}
			}

			if (Value->Parent != nullptr)
				Callback(DocumentPretty_Write_Line, "", 0);

			Callback(DocumentPretty_Write_Tab, "", 0);
			Callback(DocumentPretty_Dummy, Array ? "[" : "{", 1);
			Callback(DocumentPretty_Tab_Increase, "", 0);

			if (!Array && !Value->String.empty())
			{
				Rest::Stroke Safe(Value->String);
				Safe.Escape();

				Callback(DocumentPretty_Write_Line, "", 0);
				Callback(DocumentPretty_Write_Tab, "", 0);
				Callback(DocumentPretty_Write_Space, "\"@text@\":", 9);
				Callback(DocumentPretty_Dummy, "\"", 1);
				Callback(DocumentPretty_Dummy, Safe.Get(), (int64_t)Safe.Size());
				Callback(DocumentPretty_Dummy, "\"", 1);
			}

			std::unordered_map<std::string, uint64_t> Mapping = Value->CreateMapping();
			for (auto&& It : Mapping)
				It.second = 0;

			for (auto&& Document : Value->Nodes)
			{
				if (!Array)
				{
					std::string Name = Document->Name;

					auto It = Mapping.find(Name);
					if (It != Mapping.end())
					{
						if (It->second > 0)
							Name.append(1, '@').append(std::to_string(It->second - 1));
						It->second++;
					}

					Callback(DocumentPretty_Write_Line, "", 0);
					Callback(DocumentPretty_Write_Tab, "", 0);
					Callback(DocumentPretty_Dummy, "\"", 1);
					Callback(DocumentPretty_Dummy, Name.c_str(), (int64_t)Name.size());
					Callback(DocumentPretty_Write_Space, "\":", 2);
				}

				bool Blob = Document->IsObject();
				if (Blob && Document->Type == NodeType_Array && !Document->String.empty() && Document->Nodes.empty())
					Blob = false;

				if (!Blob)
				{
					std::string Key = Document->Serialize();
					Rest::Stroke Safe(&Key);
					Safe.Escape();

					if (Array)
					{
						Callback(DocumentPretty_Write_Line, "", 0);
						Callback(DocumentPretty_Write_Tab, "", 0);
					}

					if (!Document->IsObject() && Document->Type != NodeType_String && Document->Type != NodeType_Id)
					{
						if (!Key.empty() && Key.front() == THAWK_PREFIX_CHAR)
							Callback(DocumentPretty_Dummy, Key.c_str() + 1, (int64_t)Key.size() - 1);
						else
							Callback(DocumentPretty_Dummy, Key.c_str(), (int64_t)Key.size());
					}
					else
					{
						Callback(DocumentPretty_Dummy, "\"", 1);
						Callback(DocumentPretty_Dummy, Key.c_str(), (int64_t)Key.size());
						Callback(DocumentPretty_Dummy, "\"", 1);
					}
				}
				else
					WriteJSON(Document, Callback);

				Offset++;
				if (Offset < Size)
					Callback(DocumentPretty_Dummy, ",", 1);
			}

			Callback(DocumentPretty_Tab_Decrease, "", 0);
			Callback(DocumentPretty_Write_Line, "", 0);

			if (Value->Parent != nullptr)
				Callback(DocumentPretty_Write_Tab, "", 0);

			Callback(DocumentPretty_Dummy, Array ? "]" : "}", 1);
			return true;
		}
		Document* Document::ReadBIN(const NReadCallback& Callback)
		{
			if (!Callback)
				return nullptr;

			char Hello[18];
			if (!Callback((char*)Hello, sizeof(char) * 16))
			{
				THAWK_ERROR("form cannot be defined");
				return nullptr;
			}

			if (memcmp((void*)Hello, (void*)"\0b\0i\0n\0h\0e\0a\0d\r\n", sizeof(char) * 16) != 0)
			{
				THAWK_ERROR("version is undefined");
				return nullptr;
			}

			uint64_t Set = 0;
			if (!Callback((char*)&Set, sizeof(uint64_t)))
			{
				THAWK_ERROR("name map is undefined");
				return nullptr;
			}

			std::unordered_map<uint64_t, std::string> Map;
			for (uint64_t i = 0; i < Set; i++)
			{
				uint64_t Index = 0;
				if (!Callback((char*)&Index, sizeof(uint64_t)))
				{
					THAWK_ERROR("name index is undefined");
					return nullptr;
				}

				uint64_t Size = 0;
				if (!Callback((char*)&Size, sizeof(uint64_t)))
				{
					THAWK_ERROR("name size is undefined");
					return nullptr;
				}

				if (Size <= 0)
					continue;

				std::string Name;
				Name.resize(Size);
				if (!Callback((char*)Name.c_str(), sizeof(char) * Size))
				{
					THAWK_ERROR("name data is undefined");
					return nullptr;
				}

				Map.insert({ Index, Name });
			}

			Document* Current = new Document();
			if (!ProcessBINRead(Current, &Map, Callback))
			{
				delete Current;
				return nullptr;
			}

			return Current;
		}
		Document* Document::ReadXML(int64_t Size, const NReadCallback& Callback)
		{
			if (!Callback || !Size)
				return nullptr;

			std::string Buffer;
			Buffer.resize(Size);
			if (!Callback((char*)Buffer.c_str(), sizeof(char) * Size))
			{
				THAWK_ERROR("cannot read xml document");
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
				THAWK_ERROR("xml runtime error caused because %s", e.what());
				return nullptr;
			}
			catch (const rapidxml::parse_error& e)
			{
				delete iDocument;
				THAWK_ERROR("xml parse error caused because %s", e.what());
				return nullptr;
			}
			catch (const std::exception& e)
			{
				delete iDocument;
				THAWK_ERROR("xml parse exception caused because %s", e.what());
				return nullptr;
			}
			catch (...)
			{
				delete iDocument;
				THAWK_ERROR("undefined xml parse error");
				return nullptr;
			}

			rapidxml::xml_node<>* Base = iDocument->first_node();
			if (!Base)
			{
				iDocument->clear();
				delete iDocument;

				return nullptr;
			}

			Document* Result = new Document();
			Result->Name = Base->name();
			Result->String = Base->value();
			Result->Type = NodeType_Array;

			if (!ProcessXMLRead((void*)Base, Result))
			{
				delete Result;
				Result = nullptr;
			}

			iDocument->clear();
			delete iDocument;

			return Result;
		}
		Document* Document::ReadJSON(int64_t Size, const NReadCallback& Callback)
		{
			if (!Callback || !Size)
				return nullptr;

#ifdef THAWK_HAS_MONGOC
			std::string Buffer;
			Buffer.resize(Size);
			if (!Callback((char*)Buffer.c_str(), sizeof(char) * Size))
			{
				THAWK_ERROR("cannot read json document");
				return nullptr;
			}

			Network::BSON::TDocument* Document = Network::BSON::Document::Create(Buffer);
			if (!Document)
			{
				THAWK_ERROR("cannot parse json document");
				return nullptr;
			}

			Rest::Document* Result = Network::BSON::Document::ToDocument(Document);
			Network::BSON::Document::Release(&Document);
			ProcessJSONRead(Result);

			if (Result != nullptr && Result->Name.empty())
				Result->Name.assign("document");

			return Result;
#else
			THAWK_ERROR("json parse is unsupported for this build");
            return false;
#endif
		}
		Document* Document::NewArray()
		{
			Document* Result = new Document();
			Result->Type = NodeType_Array;

			return Result;
		}
		Document* Document::NewUndefined()
		{
			Document* Result = new Document();
			Result->Type = NodeType_Undefined;

			return Result;
		}
		Document* Document::NewNull()
		{
			Document* Result = new Document();
			Result->Type = NodeType_Null;

			return Result;
		}
		Document* Document::NewId(unsigned char Value[12])
		{
			if (!Value)
				return nullptr;

			Document* Result = new Document();
			Result->Type = NodeType_Id;
			Result->String.assign((const char*)Value, 12);

			return Result;
		}
		Document* Document::NewString(const char* Value, int64_t Size)
		{
			if (!Value)
				return nullptr;

			Document* Result = new Document();
			Result->Type = NodeType_String;
			Result->String.assign(Value, Size);

			return Result;
		}
		Document* Document::NewString(const std::string& Value)
		{
			Document* Result = new Document();
			Result->Type = NodeType_String;
			Result->String.assign(Value);

			return Result;
		}
		Document* Document::NewInteger(int64_t Value)
		{
			Document* Result = new Document();
			Result->Type = NodeType_Integer;
			Result->Integer = Value;
			Result->Number = (double)Value;

			return Result;
		}
		Document* Document::NewNumber(double Value)
		{
			Document* Result = new Document();
			Result->Type = NodeType_Number;
			Result->Integer = (int64_t)Value;
			Result->Number = Value;

			return Result;
		}
		Document* Document::NewDecimal(int64_t High, int64_t Low)
		{
			Document* Result = new Document();
			Result->Type = NodeType_Decimal;
			Result->Integer = High;
			Result->Low = Low;

			return Result;
		}
		Document* Document::NewDecimal(const std::string& Value)
		{
#ifdef THAWK_HAS_MONGOC
			int64_t fHigh, fLow;
			if (!Network::BSON::Document::ParseDecimal(Value.c_str(), &fHigh, &fLow))
				return nullptr;

			Document* Result = new Document();
			Result->Type = NodeType_Decimal;
			Result->Integer = fHigh;
			Result->Low = fLow;

			return Result;
#else
			return nullptr;
#endif
		}
		Document* Document::NewBoolean(bool Value)
		{
			Document* Result = new Document();
			Result->Type = NodeType_Boolean;
			Result->Boolean = Value;

			return Result;
		}
		void Document::ProcessBINWrite(Document* Current, std::unordered_map<std::string, uint64_t>* Map, const NWriteCallback& Callback)
		{
			uint64_t Id = Map->at(Current->Name), Count = 0;
			Callback(DocumentPretty_Dummy, (const char*)&Id, sizeof(uint64_t));
			Callback(DocumentPretty_Dummy, (const char*)&Current->Type, sizeof(NodeType));
			Callback(DocumentPretty_Dummy, (const char*)&(Count = Current->String.size()), sizeof(uint64_t));

			if (Count > 0)
				Callback(DocumentPretty_Dummy, (const char*)Current->String.c_str(), (size_t)Count);

			switch (Current->Type)
			{
				case NodeType_Integer:
					Callback(DocumentPretty_Dummy, (const char*)&Current->Integer, sizeof(int64_t));
					break;
				case NodeType_Number:
					Callback(DocumentPretty_Dummy, (const char*)&Current->Number, sizeof(double));
					break;
				case NodeType_Decimal:
					Callback(DocumentPretty_Dummy, (const char*)&Current->Integer, sizeof(int64_t));
					Callback(DocumentPretty_Dummy, (const char*)&Current->Low, sizeof(int64_t));
					break;
				case NodeType_Boolean:
					Callback(DocumentPretty_Dummy, (const char*)&Current->Boolean, sizeof(bool));
					break;
				case NodeType_Array:
				case NodeType_Object:
					Callback(DocumentPretty_Dummy, (const char*)&(Count = Current->Nodes.size()), sizeof(uint64_t));
					for (auto& Document : Current->Nodes)
						ProcessBINWrite(Document, Map, Callback);
					break;
				default:
					break;
			}
		}
		bool Document::ProcessBINRead(Document* Current, std::unordered_map<uint64_t, std::string>* Map, const NReadCallback& Callback)
		{
			uint64_t Id = 0;
			if (!Callback((char*)&Id, sizeof(uint64_t)))
			{
				THAWK_ERROR("key name index is undefined");
				return false;
			}

			auto It = Map->find(Id);
			if (It != Map->end())
				Current->Name = It->second;

			if (!Callback((char*)&Current->Type, sizeof(NodeType)))
			{
				THAWK_ERROR("key type is undefined");
				return false;
			}

			uint64_t Count = 0;
			if (!Callback((char*)&Count, sizeof(uint64_t)))
			{
				THAWK_ERROR("key value size is undefined");
				return false;
			}

			if (Count > 0)
			{
				Current->String.resize(Count);
				if (!Callback((char*)Current->String.c_str(), sizeof(char) * Count))
				{
					THAWK_ERROR("key value data is undefined");
					return false;
				}
			}

			switch (Current->Type)
			{
				case NodeType_Integer:
					if (!Callback((char*)&Current->Integer, sizeof(int64_t)))
					{
						THAWK_ERROR("key value is undefined");
						return false;
					}

					Current->Number = (double)Current->Integer;
					break;
				case NodeType_Number:
					if (!Callback((char*)&Current->Number, sizeof(double)))
					{
						THAWK_ERROR("key value is undefined");
						return false;
					}

					Current->Integer = (int64_t)Current->Number;
					break;
				case NodeType_Decimal:
					if (!Callback((char*)&Current->Integer, sizeof(int64_t)))
					{
						THAWK_ERROR("key value is undefined");
						return false;
					}

					if (!Callback((char*)&Current->Low, sizeof(int64_t)))
					{
						THAWK_ERROR("key value is undefined");
						return false;
					}
					break;
				case NodeType_Boolean:
					if (!Callback((char*)&Current->Boolean, sizeof(bool)))
					{
						THAWK_ERROR("key value is undefined");
						return false;
					}
					break;
				case NodeType_Array:
				case NodeType_Object:
					if (!Callback((char*)&Count, sizeof(uint64_t)))
					{
						THAWK_ERROR("key value size is undefined");
						return false;
					}

					if (!Count)
						break;

					Current->Nodes.resize(Count);
					for (auto K = Current->Nodes.begin(); K != Current->Nodes.end(); K++)
					{
						*K = new Document();
						(*K)->Parent = Current;
						(*K)->Saved = true;

						ProcessBINRead(*K, Map, Callback);
					}
					break;
				default:
					break;
			}

			return true;
		}
		bool Document::ProcessMAPRead(Document* Current, std::unordered_map<std::string, uint64_t>* Map, uint64_t& Index)
		{
			auto M = Map->find(Current->Name);
			if (M == Map->end())
				Map->insert({ Current->Name, Index++ });

			for (auto Document : Current->Nodes)
				ProcessMAPRead(Document, Map, Index);

			return true;
		}
		bool Document::ProcessXMLRead(void* Base, Document* Current)
		{
			auto Ref = (rapidxml::xml_node<>*)Base;
			if (!Ref || !Current)
				return false;

			for (rapidxml::xml_attribute<>* It = Ref->first_attribute(); It; It = It->next_attribute())
			{
				if (strcmp(It->name(), "") != 0)
					Current->SetAttribute(It->name(), It->value());
			}

			for (rapidxml::xml_node<>* It = Ref->first_node(); It; It = It->next_sibling())
			{
				if (strcmp(It->name(), "") == 0)
					continue;

				Document* V = Current->SetArray(It->name());
				V->String = It->value();

				ProcessXMLRead((void*)It, V);
				if (V->Nodes.empty() && !V->String.empty())
					V->Type = NodeType_String;
			}

			return true;
		}
		bool Document::ProcessJSONRead(Document* Current)
		{
			Document* Text = Current->Get("@text@");
			if (Text != nullptr)
			{
				NodeType Base = Current->Type;
				Current->Deserialize(Text->Serialize());
				Current->SetUndefined("@text@");
				Current->Type = Base;
			}

			for (auto&& Document : Current->Nodes)
			{
				Stroke V(&Document->Name);
				Stroke::Settle F = V.Find('@');

				if (F.Found)
				{
					const char* Buffer = V.Get() + F.End;
					while (*Buffer != '\0' && Stroke::IsDigit(*Buffer))
						Buffer++;

					if (*Buffer == '\0')
						V.Clip(F.End - 1);
				}

				ProcessJSONRead(Document);
			}

			return true;
		}

		Stroke Form(const char* Format, ...)
		{
			char Buffer[16384];
			if (!Format)
				return Stroke();

			va_list Args;
			va_start(Args, Format);
			int Size = vsnprintf(Buffer, sizeof(Buffer), Format, Args);
			va_end(Args);

			return Stroke(Buffer, Size > 16384 ? 16384 : (size_t)Size);
		}
	}
}
