#include "core.h"
#include "../network/http.h"
#include <cctype>
#include <ctime>
#include <thread>
#include <functional>
#include <iostream>
#include <csignal>
#include <sstream>
#include <sys/stat.h>
#include <rapidxml.hpp>
#include <json/document.h>
#include <tinyfiledialogs.h>
#include <concurrentqueue.h>
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
#include <ucontext.h>
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

		for (unsigned int i = 0; Buffer1[i] != '\0'; i++)
		{
			if (!i || (Buffer1[i] != '/' && Buffer1[i] != '\\'))
				continue;

			while (Buffer1[i + 1] == '\\' || Buffer1[i + 1] == '/')
				memmove(Buffer1 + i + 1, Buffer1 + i + 2, strlen(Buffer1 + i + 1));
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
#ifndef TH_64
    int Pack1_32(void* Value)
    {
        return (int)(uintptr_t)Value;
    }
    void* Unpack1_32(int Value)
    {
        return (void*)(uintptr_t)Value;
    }
#else
    void Pack2_64(void* Value, int* X, int* Y)
    {
        uint64_t Subvalue = (uint64_t)Value;
        *X = (int)(uint32_t)((Subvalue & 0xFFFFFFFF00000000LL) >> 32);
        *Y = (int)(uint32_t)(Subvalue & 0xFFFFFFFFLL);
    }
    void* Unpack2_64(int X, int Y)
    {
        uint64_t Subvalue = ((uint64_t)(uint32_t)X) << 32 | (uint32_t)Y;
        return (void*)Subvalue;
        }
#endif
    bool LocalTime(time_t const* const A, struct tm* const B)
    {
        return localtime_r(A, B) != nullptr;
    }
	const char* GetColorId(Tomahawk::Core::StdColor Color, bool Background)
	{
		switch (Color)
		{
			case Tomahawk::Core::StdColor::Black:
				return "40";
			case Tomahawk::Core::StdColor::DarkBlue:
				return "44";
			case Tomahawk::Core::StdColor::DarkGreen:
				return "42";
			case Tomahawk::Core::StdColor::LightBlue:
				return "46";
			case Tomahawk::Core::StdColor::Dark_red:
				return "41";
			case Tomahawk::Core::StdColor::Magenta:
				return "45";
			case Tomahawk::Core::StdColor::Orange:
				return "43";
			case Tomahawk::Core::StdColor::LightGray:
				return "47";
			case Tomahawk::Core::StdColor::Gray:
				return "100";
			case Tomahawk::Core::StdColor::Blue:
				return "104";
			case Tomahawk::Core::StdColor::Green:
				return "102";
			case Tomahawk::Core::StdColor::Cyan:
				return "106";
			case Tomahawk::Core::StdColor::Red:
				return "101";
			case Tomahawk::Core::StdColor::Pink:
				return "105";
			case Tomahawk::Core::StdColor::Yellow:
				return "103";
			case Tomahawk::Core::StdColor::White:
				return "107";
			default:
				return Background ? "40" : "107";
		}
	}
}
#endif

namespace Tomahawk
{
	namespace Core
	{
		typedef moodycamel::ConcurrentQueue<EventBase*> EQueue;
		typedef moodycamel::ConcurrentQueue<TaskCallback*> TQueue;
		typedef moodycamel::ConsumerToken CToken;

		struct Cocontext
		{
#ifndef TH_MICROSOFT
			ucontext_t Context;
			char* Stack = nullptr;
#else
			LPVOID Context = nullptr;
#endif
			bool Swapchain;

			Cocontext(bool Swap) :
#ifdef TH_MICROSOFT
				Context(nullptr), Swapchain(Swap)
#else
				Stack(nullptr), Swapchain(Swap)
#endif
			{
#ifdef TH_MICROSOFT
				if (Swapchain)
					Context = ConvertThreadToFiber(nullptr);
#endif
			}
			~Cocontext()
			{
#ifdef TH_MICROSOFT
				if (Swapchain)
				{
					ConvertFiberToThread();
					return;
				}

				if (Context != nullptr)
				{
					DeleteFiber(Context);
					Context = nullptr;
				}
#else
				if (Swapchain)
					return;

				TH_FREE(Stack);
				Stack = nullptr;
#endif
			}
		};

		Coroutine::Coroutine(Costate* Base, const TaskCallback& Procedure) : State(Coactive::Active), Callback(Procedure), Switch(TH_NEW(Cocontext, false)), Master(Base), Dead(false)
		{
		}
		Coroutine::Coroutine(Costate* Base, TaskCallback&& Procedure) : State(Coactive::Active), Callback(std::move(Procedure)), Switch(TH_NEW(Cocontext, false)), Master(Base), Dead(false)
		{
		}
		Coroutine::~Coroutine()
		{
			TH_DELETE(Cocontext, Switch);
		}

		Decimal::Decimal() : Length(0), Sign('\0'), Invalid(true)
		{
		}
		Decimal::Decimal(const char* Value) : Length(0), Sign('\0'), Invalid(false)
		{
            int Count = 0;
            if (Value[Count] == '+')
            {
                Sign = '+';
                Count++;
            }
            else if (Value[Count] == '-')
            {
                Sign = '-';
                Count++;
            }
            else if (isdigit(Value[Count]))
            {
                Sign = '+';
            }
            else
            {
                Invalid = 1;
                return;
            }

            bool Points = false;
            while (Value[Count] != '\0')
            {
                if (!Points && Value[Count] == '.')
                {
                    if (Source.empty())
                    {
                        Sign = '\0';
                        Invalid = 1;
                        return;
                    }

                    Points = true;
                    Count++;
                }

                if (isdigit(Value[Count]))
                {
                    Source.push_front(Value[Count]);
                    Count++;

                    if (Points)
                        Length++;
                }
                else
                {
                    Sign = '\0';
                    Source.clear();
                    Length = 0;
                    Invalid = 1;
                    return;
                }
            }

            Unlead();
		}
        Decimal::Decimal(const std::string& Value) : Decimal(Value.c_str())
		{
		}
		Decimal::Decimal(int32_t Value) : Decimal(std::to_string(Value))
		{
		}
        Decimal::Decimal(uint32_t Value) : Decimal(std::to_string(Value))
        {
        }
        Decimal::Decimal(int64_t Value) : Decimal(std::to_string(Value))
        {
        }
        Decimal::Decimal(uint64_t Value) : Decimal(std::to_string(Value))
        {
        }
        Decimal::Decimal(float Value) : Decimal(std::to_string(Value))
        {
        }
		Decimal::Decimal(double Value) : Decimal(std::to_string(Value))
		{
		}
		Decimal::Decimal(const Decimal& Value) : Source(Value.Source), Length(Value.Length), Sign(Value.Sign), Invalid(Value.Invalid)
		{
		}
		Decimal::Decimal(Decimal&& Value) : Source(std::move(Value.Source)), Length(Value.Length), Sign(Value.Sign), Invalid(Value.Invalid)
		{
		}
		Decimal& Decimal::Precise(int prec)
		{
			if (Invalid || prec < 0)
				return *this;

			if (this->Length < prec)
			{
				while (this->Length < prec)
				{
					this->Length++;
					this->Source.push_front('0');
				}
			}
			else if (this->Length > prec)
			{
				char last;
				while (this->Length > prec)
				{
					last = Source[0];
					this->Length--;
					this->Source.pop_front();
				}

				if (CharToInt(last) >= 5)
				{
					if (prec != 0)
					{
						std::string Result = "0.";
						for (int i = 1; i < prec; i++)
							Result += '0';
						Result += '1';

						Decimal Temp(Result);
						*this = *this + Temp;
					}
					else
						++(*this);
				}
			}

			return *this;
		}
		Decimal& Decimal::Trim()
		{
			return Unlead().Untrail();
		}
		Decimal& Decimal::Unlead()
		{
			for (int i = Source.size() - 1; i > Length; --i)
			{
				if (Source[i] != '0')
					break;

				Source.pop_back();
			}

			return *this;
		}
		Decimal& Decimal::Untrail()
		{
			if (Invalid || Source.empty())
				return *this;

			while ((Source[0] == '0') && (Length > 0))
			{
				Source.pop_front();
				Length--;
			}

			return *this;
		}
		bool Decimal::IsNaN() const
		{
			return Invalid;
		}
		double Decimal::ToDouble() const
		{
			if (Invalid)
				return std::nan("");

			double dec = 1;
			if (Length > 0)
			{
				int aus = Length;
				while (aus != 0)
				{
					dec /= 10;
					aus--;
				}
			}

			double var = 0;
			for (int i = 0; i < Source.size(); i++)
			{
				var += CharToInt(Source[i]) * dec;
				dec *= 10;
			}

			if (Sign == '-')
				var *= -1;

			return var;
		}
		int64_t Decimal::ToInt64() const
		{
            return (int64_t)ToDouble();
		}
		std::string Decimal::ToString() const
		{
			if (Invalid || Source.empty())
				return "NaN";

			std::string Result;
			if (Sign == '-')
				Result += Sign;

			int Offset = 0, Size = Length;
			while ((Source[Offset] == '0') && (Size > 0))
			{
				Offset++;
				Size--;
			}

			for (int i = Source.size() - 1; i >= Offset; i--)
			{
				Result += Source[i];
				if ((i == Length) && (i != 0) && Offset != Length)
					Result += '.';
			}

			return Result;
		}
		std::string Decimal::Exp() const
		{
			if (Invalid)
				return "NaN";

			std::string Result;
			int Compare = Decimal::CompareNum(*this, Decimal(1));
			if (Compare == 0)
			{
				Result += Sign;
				Result += "1e+0";
			}
			else if (Compare == 1)
			{
				Result += Sign;
				int i = Source.size() - 1;
				Result += Source[i];
				i--;

				if (i > 0)
				{
					Result += '.';
					for (; (i >= (int)Source.size() - 6) && (i >= 0); --i)
						Result += Source[i];
				}
				Result += "e+";
				Result += std::to_string(Ints() - 1);
			}
			else if (Compare == 2)
			{
				int exp = 0, count = Source.size() - 1;
				while (count > 0 && Source[count] == '0')
				{
					count--;
					exp++;
				}

				if (count == 0)
				{
					if (Source[count] != '0')
					{
						Result += Sign;
						Result += Source[count];
						Result += "e-";
						Result += std::to_string(exp);
					}
					else
						Result += "+0";
				}
				else
				{
					Result += Sign;
					Result += Source[count];
					Result += '.';

					for (int i = count - 1; (i >= (int)count - 5) && (i >= 0); --i)
						Result += Source[i];

					Result += "e-";
					Result += std::to_string(exp);
				}
			}

			return Result;
		}
		int Decimal::Decimals() const
		{
			return Length;
		}
		int Decimal::Ints() const
		{
			return Source.size() - Length;
		}
		int Decimal::Size() const
		{
			return sizeof(*this) + Source.size() * sizeof(char);
		}
        Decimal Decimal::operator -() const
        {
            Decimal Result = *this;
            if (Result.Sign == '+')
                Result.Sign = '-';
            else if (Result.Sign == '-')
                Result.Sign = '+';
            
            return Result;
        }
        Decimal& Decimal::operator *=(const Decimal& V)
        {
            *this = *this * V;
            return *this;
        }
        Decimal& Decimal::operator /=(const Decimal& V)
        {
            *this = *this / V;
            return *this;
        }
        Decimal& Decimal::operator +=(const Decimal& V)
        {
            *this = *this + V;
            return *this;
        }
        Decimal& Decimal::operator -=(const Decimal& V)
        {
            *this = *this - V;
            return *this;
        }
		Decimal& Decimal::operator=(const Decimal& Value)
		{
			Source = Value.Source;
			Length = Value.Length;
			Sign = Value.Sign;
			Invalid = Value.Invalid;

			return *this;
		}
		Decimal& Decimal::operator=(Decimal&& Value)
		{
			Source = std::move(Value.Source);
			Length = Value.Length;
			Sign = Value.Sign;
			Invalid = Value.Invalid;

			return *this;
		}
		Decimal& Decimal::operator++(int)
		{
			*this = *this + 1;
			return *this;
		}
		Decimal& Decimal::operator++()
		{
			*this = *this + 1;
			return *this;
		}
		Decimal& Decimal::operator--(int)
		{
			*this = *this - 1;
			return *this;
		}
		Decimal& Decimal::operator--()
		{
			*this = *this - 1;
			return *this;
		}
		bool Decimal::operator==(const Decimal& right) const
		{
			if (Invalid || right.Invalid)
				return false;

			int check = CompareNum(*this, right);
			if ((check == 0) && (Sign == right.Sign))
				return true;

			return false;
		}
		bool Decimal::operator!=(const Decimal& right) const
		{
			if (Invalid || right.Invalid)
				return false;

			return !(*this == right);
		}
		bool Decimal::operator>(const Decimal& right) const
		{
			if (Invalid || right.Invalid)
				return false;

			if (((Sign == '+') && (right.Sign == '+')))
			{
				int check = CompareNum(*this, right);
				return check == 1;
			}

			if (((Sign == '-') && (right.Sign == '-')))
			{
				int check = CompareNum(*this, right);
				return check == 2;
			}

			if (((Sign == '-') && (right.Sign == '+')))
				return false;

			if (((Sign == '+') && (right.Sign == '-')))
				return true;

			return false;
		}
		bool Decimal::operator>=(const Decimal& right) const
		{
			if (Invalid || right.Invalid)
				return false;

			return !(*this < right);
		}
		bool Decimal::operator<(const Decimal& right) const
		{
			if (Invalid || right.Invalid)
				return false;

			if (((Sign == '+') && (right.Sign == '+')))
			{
				int check = CompareNum(*this, right);
				return check == 2;
			}

			if (((Sign == '-') && (right.Sign == '-')))
			{
				int check = CompareNum(*this, right);
				return check == 1;
			}

			if (((Sign == '-') && (right.Sign == '+')))
				return true;

			if (((Sign == '+') && (right.Sign == '-')))
				return false;

			return false;
		}
		bool Decimal::operator<=(const Decimal& right) const
		{
			if (Invalid || right.Invalid)
				return false;

			return !(*this > right);
		}
		Decimal operator+(const Decimal& left_, const Decimal& right_)
		{
			Decimal tmp;
			if (left_.Invalid || right_.Invalid)
				return tmp;

			Decimal left, right;
			left = left_;
			right = right_;

			if (left.Length > right.Length)
			{
				while (left.Length > right.Length)
				{
					right.Length++;
					right.Source.push_front('0');
				}
			}
			else if (left.Length < right.Length)
			{
				while (left.Length < right.Length)
				{
					left.Length++;
					left.Source.push_front('0');
				}
			}

			if ((left.Sign == '+') && (right.Sign == '-'))
			{
				int check = Decimal::CompareNum(left, right);
				if (check == 0)
				{
					tmp = 0;
					return tmp;
				}

				if (check == 1)
				{
					tmp = Decimal::Subtract(left, right);
					tmp.Sign = '+';
					tmp.Length = left.Length;
					tmp.Unlead();
					tmp.Invalid = 0;
					return tmp;
				}

				if (check == 2)
				{
					tmp = Decimal::Subtract(right, left);
					tmp.Sign = '-';
					tmp.Length = left.Length;
					tmp.Unlead();
					tmp.Invalid = 0;
					return tmp;
				}
			}

			if ((left.Sign == '-') && (right.Sign == '+'))
			{
				int check = Decimal::CompareNum(left, right);
				if (check == 0)
				{
					tmp = 0;
					return tmp;
				}

				if (check == 1)
				{
					tmp = Decimal::Subtract(left, right);
					tmp.Sign = '-';
					tmp.Length = left.Length;
					tmp.Unlead();
					tmp.Invalid = 0;
					return tmp;
				}

				if (check == 2)
				{
					tmp = Decimal::Subtract(right, left);
					tmp.Sign = '+';
					tmp.Length = left.Length;
					tmp.Unlead();
					tmp.Invalid = 0;
					return tmp;
				}
			}

			if ((left.Sign == '+') && (right.Sign == '+'))
			{
				tmp = Decimal::Sum(left, right);
				tmp.Sign = '+';
				tmp.Length = left.Length;
				tmp.Invalid = 0;
				return tmp;
			}

			if ((left.Sign == '-') && (right.Sign == '-'))
			{
				tmp = Decimal::Sum(left, right);
				tmp.Sign = '-';
				tmp.Length = left.Length;
				tmp.Invalid = 0;
				return tmp;
			}

			return tmp;
		}
		Decimal operator+(const Decimal& left, const int& int_right)
		{
			Decimal right;
			right = int_right;
			return left + right;
		}
		Decimal operator+(const Decimal& left, const double& double_right)
		{
			Decimal right;
			right = double_right;
			return left + right;
		}
		Decimal operator-(const Decimal& left_, const Decimal& right_)
		{
			Decimal tmp;
			if (left_.Invalid || right_.Invalid)
				return tmp;

			Decimal left, right;
			left = left_;
			right = right_;

			if (left.Length > right.Length)
			{
				while (left.Length > right.Length)
				{
					right.Length++;
					right.Source.push_front('0');
				}
			}
			else if (left.Length < right.Length)
			{
				while (left.Length < right.Length)
				{
					left.Length++;
					left.Source.push_front('0');
				}
			}

			if ((left.Sign == '+') && (right.Sign == '-'))
			{
				tmp = Decimal::Sum(left, right);
				tmp.Sign = '+';
				tmp.Length = left.Length;
				tmp.Invalid = 0;
				return tmp;
			}
			if ((left.Sign == '-') && (right.Sign == '+'))
			{
				tmp = Decimal::Sum(left, right);
				tmp.Sign = '-';
				tmp.Length = left.Length;
				tmp.Invalid = 0;
				return tmp;
			}

			if ((left.Sign == '+') && (right.Sign == '+'))
			{
				int check = Decimal::CompareNum(left, right);
				if (check == 0)
				{
					tmp = 0;
					return tmp;
				}

				if (check == 1)
				{
					tmp = Decimal::Subtract(left, right);
					tmp.Sign = '+';
					tmp.Length = left.Length;
					tmp.Unlead();
					tmp.Invalid = 0;
					return tmp;
				}

				if (check == 2)
				{
					tmp = Decimal::Subtract(right, left);
					tmp.Sign = '-';
					tmp.Length = left.Length;
					tmp.Unlead();
					tmp.Invalid = 0;
					return tmp;
				}
			}

			if ((left.Sign == '-') && (right.Sign == '-'))
			{
				int check = Decimal::CompareNum(left, right);
				if (check == 0)
				{
					tmp = 0;
					return tmp;
				}

				if (check == 1)
				{
					tmp = Decimal::Subtract(left, right);
					tmp.Sign = '-';
					tmp.Length = left.Length;
					tmp.Unlead();
					tmp.Invalid = 0;
					return tmp;
				}

				if (check == 2)
				{
					tmp = Decimal::Subtract(right, left);
					tmp.Sign = '+';
					tmp.Length = left.Length;
					tmp.Unlead();
					tmp.Invalid = 0;
					return tmp;
				}
			}

			return tmp;
		}
		Decimal operator-(const Decimal& left, const int& int_right)
		{
			Decimal right;
			right = int_right;
			return left - right;
		}
		Decimal operator-(const Decimal& left, const double& double_right)
		{
			Decimal right;
			right = double_right;
			return left - right;
		}
		Decimal operator*(const Decimal& left, const Decimal& right)
		{
			Decimal tmp;
			if (left.Invalid || right.Invalid)
				return tmp;

			tmp = Decimal::Multiply(left, right);
			if (((left.Sign == '-') && (right.Sign == '-')) || ((left.Sign == '+') && (right.Sign == '+')))
				tmp.Sign = '+';
			else
				tmp.Sign = '-';

			tmp.Length = left.Length + right.Length;
			tmp.Invalid = 0;
			tmp.Unlead();

			return tmp;
		}
		Decimal operator*(const Decimal& left, const int& int_right)
		{
			Decimal right;
			right = int_right;
			return left * right;
		}
		Decimal operator*(const Decimal& left, const double& double_right)
		{
			Decimal right;
			right = double_right;
			return left * right;
		}
		Decimal operator/(const Decimal& left, const Decimal& right)
		{
			Decimal tmp;
			if (left.Invalid || right.Invalid)
				return tmp;

			Decimal Q, R, D, N, zero;
			zero = 0;

			if (right == zero)
				return tmp;

			N = (left > zero) ? (left) : (left * (-1));
			D = (right > zero) ? (right) : (right * (-1));
			R.Sign = '+';
			R.Invalid = 0;

			while ((N.Length != 0) || (D.Length != 0))
			{
				if (N.Length == 0)
					N.Source.push_front('0');
				else
					N.Length--;

				if (D.Length == 0)
					D.Source.push_front('0');
				else
					D.Length--;
			}

			N.Unlead();
			D.Unlead();

			int div_precision = (left.Length > right.Length) ? (left.Length) : (right.Length);
			for (int i = 0; i < div_precision; i++)
				N.Source.push_front('0');

			int check = Decimal::CompareNum(N, D);
			if (check == 0)
				tmp.Source.push_front('1');

			if (check == 2)
				return zero;

			while (!N.Source.empty())
			{
				R.Source.push_front(*(N.Source.rbegin()));
				N.Source.pop_back();

				bool is_zero = true;
				std::deque<char>::const_iterator zero_iter = R.Source.begin();
				for (; zero_iter != R.Source.end(); ++zero_iter)
				{
					if (*zero_iter != '0')
						is_zero = false;
				}

				if ((R >= D) && (!is_zero))
				{
					int Q_sub = 0;
					int min = 0;
					int max = 9;

					while (R >= D)
					{
						int avg = max - min;
						int mod_avg = avg / 2;
						avg = (avg - mod_avg * 2) ? (mod_avg + 1) : (mod_avg);

						int div_check = Decimal::CompareNum(R, D * avg);

						if (div_check != 2)
						{
							Q_sub = Q_sub + avg;
							R = R - D * avg;

							max = 9;
						}
						else
							max = avg;
					}

					Q.Source.push_front(Decimal::IntToChar(Q_sub));

					bool is_zero = true;
					std::deque<char>::const_iterator zero_iter = R.Source.begin();
					for (; zero_iter != R.Source.end(); ++zero_iter)
					{
						if (*zero_iter != '0')
							is_zero = false;
					}

					if (is_zero)
						R.Source.clear();
				}
				else
					Q.Source.push_front('0');
			}

			tmp = Q;
			if (((left.Sign == '-') && (right.Sign == '-')) || ((left.Sign == '+') && (right.Sign == '+')))
				tmp.Sign = '+';
			else
				tmp.Sign = '-';

			tmp.Length = div_precision;
			tmp.Invalid = 0;
			tmp.Unlead();

			return tmp;
		}
		Decimal operator/(const Decimal& left, const int& int_right)
		{
			Decimal right;
			right = int_right;
			return left / right;
		}
		Decimal operator/(const Decimal& left, const double& double_right)
		{
			Decimal right;
			right = double_right;
			return left / right;
		}
		Decimal operator%(const Decimal& left, const Decimal& right)
		{
			Decimal tmp;
			if (left.Invalid || right.Invalid)
				return tmp;

			if ((left.Length != 0) || (right.Length != 0))
				return tmp;

			Decimal Q, R, D, N, zero, ret;
			zero = 0;

			if (right == zero)
				return tmp;

			N = (left > zero) ? (left) : (left * (-1));
			D = (right > zero) ? (right) : (right * (-1));
			R.Sign = '+';
			R.Invalid = 0;

			int check = Decimal::CompareNum(N, D);

			if (check == 0)
				return zero;

			if (check == 2)
				return left;

			while (!N.Source.empty())
			{
				R.Source.push_front(*(N.Source.rbegin()));
				N.Source.pop_back();

				bool is_zero = true;
				std::deque<char>::const_iterator zero_iter = R.Source.begin();
				for (; zero_iter != R.Source.end(); ++zero_iter)
				{
					if (*zero_iter != '0')
						is_zero = false;
				}

				if ((R >= D) && (!is_zero))
				{
					int Q_sub = 0;
					int min = 0;
					int max = 9;

					while (R >= D)
					{
						int avg = max - min;
						int mod_avg = avg / 2;
						avg = (avg - mod_avg * 2) ? (mod_avg + 1) : (mod_avg);

						int div_check = Decimal::CompareNum(R, D * avg);
						if (div_check != 2)
						{
							Q_sub = Q_sub + avg;
							R = R - D * avg;

							max = 9;
						}
						else
							max = avg;
					}

					Q.Source.push_front(Decimal::IntToChar(Q_sub));
					ret = R;

					bool is_zero = true;
					std::deque<char>::const_iterator zero_iter = R.Source.begin();
					for (; zero_iter != R.Source.end(); ++zero_iter)
					{
						if (*zero_iter != '0')
							is_zero = false;
					}

					if (is_zero)
						R.Source.clear();
				}
				else
				{
					ret = R;
					Q.Source.push_front('0');
				}
			}

			Q.Unlead();
			ret.Unlead();
			tmp = ret;

			if (((left.Sign == '-') && (right.Sign == '-')) || ((left.Sign == '+') && (right.Sign == '+')))
				tmp.Sign = '+';
			else
				tmp.Sign = '-';

			if (!Decimal::CompareNum(tmp, zero))
				tmp.Sign = '+';

			tmp.Invalid = 0;
			return tmp;
		}
		Decimal operator%(const Decimal& left, const int& int_right)
		{
			Decimal right;
			right = int_right;
			return left % right;
		}
		Decimal Decimal::Divide(const Decimal& left, const Decimal& right, int div_precision)
		{
			Decimal tmp;
			Decimal Q, R, D, N, zero;
			zero = 0;

			if (right == zero)
				return tmp;

			N = (left > zero) ? (left) : (left * (-1));
			D = (right > zero) ? (right) : (right * (-1));
			R.Sign = '+';
			R.Invalid = 0;

			while ((N.Length != 0) || (D.Length != 0))
			{
				if (N.Length == 0)
					N.Source.push_front('0');
				else
					N.Length--;

				if (D.Length == 0)
					D.Source.push_front('0');
				else
					D.Length--;
			}

			N.Unlead();
			D.Unlead();

			for (int i = 0; i < div_precision; i++)
				N.Source.push_front('0');

			int check = Decimal::CompareNum(N, D);

			if (check == 0)
				tmp.Source.push_front('1');

			if (check == 2)
				return zero;

			while (!N.Source.empty())
			{
				R.Source.push_front(*(N.Source.rbegin()));
				N.Source.pop_back();

				bool is_zero = true;
				std::deque<char>::const_iterator zero_iter = R.Source.begin();
				for (; zero_iter != R.Source.end(); ++zero_iter)
				{
					if (*zero_iter != '0')
						is_zero = false;
				}

				if ((R >= D) && (!is_zero))
				{
					int Q_sub = 0;
					int min = 0;
					int max = 9;

					while (R >= D)
					{
						int avg = max - min;
						int mod_avg = avg / 2;
						avg = (avg - mod_avg * 2) ? (mod_avg + 1) : (mod_avg);

						int div_check = Decimal::CompareNum(R, D * avg);

						if (div_check != 2)
						{
							Q_sub = Q_sub + avg;
							R = R - D * avg;

							max = 9;
						}
						else
							max = avg;
					}

					Q.Source.push_front(Decimal::IntToChar(Q_sub));

					bool is_zero = true;
					std::deque<char>::const_iterator zero_iter = R.Source.begin();
					for (; zero_iter != R.Source.end(); ++zero_iter)
					{
						if (*zero_iter != '0')
							is_zero = false;
					}

					if (is_zero)
						R.Source.clear();
				}
				else
					Q.Source.push_front('0');
			}

			tmp = Q;
			if (((left.Sign == '-') && (right.Sign == '-')) || ((left.Sign == '+') && (right.Sign == '+')))
				tmp.Sign = '+';
			else
				tmp.Sign = '-';

			tmp.Length = div_precision;
			tmp.Invalid = 0;
			tmp.Unlead();

			return tmp;
		}
		Decimal Decimal::NaN()
		{
			Decimal Result;
			Result.Invalid = true;

			return Result;
		}
		Decimal Decimal::Sum(const Decimal& left, const Decimal& right)
		{
			Decimal tmp;
			int carry = 0;
			int loopsize = (left.Source.size() > right.Source.size()) ? left.Source.size() : right.Source.size();

			for (int i = 0; i < loopsize; ++i)
			{
				int val1, val2;
				val1 = (i > left.Source.size() - 1) ? 0 : CharToInt(left.Source[i]);
				val2 = (i > right.Source.size() - 1) ? 0 : CharToInt(right.Source[i]);

				int aus = val1 + val2 + carry;
				carry = 0;

				if (aus > 9)
				{
					carry = 1;
					aus = aus - 10;
				}

				tmp.Source.push_back(IntToChar(aus));
			}

			if (carry != 0)
				tmp.Source.push_back(IntToChar(carry));

			return tmp;
		}
		Decimal Decimal::Subtract(const Decimal& left, const Decimal& right)
		{
			Decimal tmp;
			int carry = 0;
			int aus;

			for (int i = 0; i < left.Source.size(); ++i)
			{
				int val1, val2;
				val1 = CharToInt(left.Source[i]);
				val2 = (i > right.Source.size() - 1) ? 0 : CharToInt(right.Source[i]);
				val1 -= carry;

				if (val1 < val2)
				{
					aus = 10 + val1 - val2;
					carry = 1;
				}
				else
				{
					aus = val1 - val2;
					carry = 0;
				}

				tmp.Source.push_back(IntToChar(aus));
			}

			return tmp;
		}
		Decimal Decimal::Multiply(const Decimal& left, const Decimal& right)
		{
			Decimal ris;
			Decimal tmp;
			ris.Source.push_back('0');
			int carry = 0;

			for (int i = 0; i < right.Source.size(); ++i)
			{
				for (int k = 0; k < i; ++k)
					tmp.Source.push_front('0');

				for (int j = 0; j < left.Source.size(); ++j)
				{
					int aus = CharToInt(right.Source[i]) * CharToInt(left.Source[j]) + carry;
					carry = 0;
					if (aus > 9)
					{
						while (aus > 9)
						{
							carry++;
							aus -= 10;
						}
					}

					tmp.Source.push_back(IntToChar(aus));
				}

				if (carry != 0)
					tmp.Source.push_back(IntToChar(carry));

				carry = 0;
				ris = Sum(ris, tmp);
				tmp.Source.clear();
			}

			return ris;
		}
		int Decimal::CompareNum(const Decimal& left, const Decimal& right)
		{
			if ((left.Source.size() - left.Length) > (right.Source.size() - right.Length))
				return 1;

			if ((left.Source.size() - left.Length) < (right.Source.size() - right.Length))
				return 2;

			if (left.Length > right.Length)
			{
				Decimal tmp;
				tmp = right;
				while (left.Length > tmp.Length)
				{
					tmp.Length++;
					tmp.Source.push_front('0');
				}

				for (int i = left.Source.size() - 1; i >= 0; i--)
				{
					if (left.Source[i] > tmp.Source[i])
						return 1;

					if (left.Source[i] < tmp.Source[i])
						return 2;
				}

				return 0;
			}
			else if (left.Length < right.Length)
			{
				Decimal tmp;
				tmp = left;
				while (tmp.Length < right.Length)
				{
					tmp.Length++;
					tmp.Source.push_front('0');
				}

				for (int i = tmp.Source.size() - 1; i >= 0; i--)
				{
					if (tmp.Source[i] > right.Source[i])
						return 1;

					if (tmp.Source[i] < right.Source[i])
						return 2;
				}

				return 0;
			}
			else
			{
				for (int i = left.Source.size() - 1; i >= 0; i--)
				{
					if (left.Source[i] > right.Source[i])
						return 1;
					else if (left.Source[i] < right.Source[i])
						return 2;
				}

				return 0;
			}
		}
		int Decimal::CharToInt(const char& Value)
		{
			return Value - '0';
		}
		char Decimal::IntToChar(const int& Value)
		{
			return Value + '0';
		}

		Variant::Variant() : Type(VarType::Undefined)
		{
		}
		Variant::Variant(VarType NewType) : Type(NewType)
		{
		}
		Variant::Variant(const Variant& Other)
		{
			Copy(Other);
		}
		Variant::Variant(Variant&& Other)
		{
			Copy(std::move(Other));
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
					Type = VarType::Null;
					return true;
				}

				if (Value == TH_PREFIX_STR "undefined")
				{
					Type = VarType::Undefined;
					return true;
				}

				if (Value == TH_PREFIX_STR "object")
				{
					Type = VarType::Object;
					return true;
				}

				if (Value == TH_PREFIX_STR "array")
				{
					Type = VarType::Array;
					return true;
				}

				if (Value == TH_PREFIX_STR "pointer")
				{
					Type = VarType::Pointer;
					return true;
				}

				if (Value == "true")
				{
					Copy(Var::Boolean(true));
					return true;
				}

				if (Value == "false")
				{
					Copy(Var::Boolean(false));
					return true;
				}

				Parser Buffer(&Value);
				if (Buffer.HasNumber())
				{
					if (Buffer.HasDecimal())
						Copy(Var::DecimalString(Buffer.R()));
					else if (Buffer.HasInteger())
						Copy(Var::Integer(Buffer.ToInt64()));
					else
						Copy(Var::Number(Buffer.ToDouble()));

					return true;
				}
			}

			if (Value.size() > 2 && Value.front() == TH_PREFIX_CHAR && Value.back() == TH_PREFIX_CHAR)
			{
				Copy(Var::Base64(Compute::Common::Base64Decode(std::string(Value.substr(1).c_str(), Value.size() - 2))));
				return true;
			}

			Copy(Var::String(Value));
			return true;
		}
		std::string Variant::Serialize() const
		{
			switch (Type)
			{
				case VarType::Null:
					return TH_PREFIX_STR "null";
				case VarType::Undefined:
					return TH_PREFIX_STR "undefined";
				case VarType::Object:
					return TH_PREFIX_STR "object";
				case VarType::Array:
					return TH_PREFIX_STR "array";
				case VarType::Pointer:
					return TH_PREFIX_STR "pointer";
				case VarType::String:
					return std::string(GetString(), GetSize());
				case VarType::Base64:
					return TH_PREFIX_STR + Compute::Common::Base64Encode(GetBase64(), GetSize()) + TH_PREFIX_STR;
				case VarType::Decimal:
					return ((Decimal*)Value.Data)->ToString();
				case VarType::Integer:
					return std::to_string(Value.Integer);
				case VarType::Number:
					return std::to_string(Value.Number);
				case VarType::Boolean:
					return Value.Boolean ? "true" : "false";
				default:
					return "";
			}
		}
		std::string Variant::GetBlob() const
		{
			if (Type == VarType::String || Type == VarType::Base64)
				return std::string(((String*)Value.Data)->Buffer, ((String*)Value.Data)->Size);

			if (Type == VarType::Decimal)
				return ((Decimal*)Value.Data)->ToString();

			if (Type == VarType::Integer)
				return std::to_string(GetInteger());

			if (Type == VarType::Number)
				return std::to_string(GetNumber());

			if (Type == VarType::Boolean)
				return Value.Boolean ? "1" : "0";

			return "";
		}
		Decimal Variant::GetDecimal() const
		{
			if (Type == VarType::Decimal)
				return *(Decimal*)Value.Data;

			if (Type == VarType::Integer)
				return Decimal(std::to_string(Value.Integer));

			if (Type == VarType::Number)
				return Decimal(std::to_string(Value.Number));

			if (Type == VarType::Boolean)
				return Decimal(Value.Boolean ? "1" : "0");

			if (Type == VarType::String)
				return Decimal(GetString());

			return Decimal::NaN();
		}
		void* Variant::GetPointer() const
		{
			if (Type == VarType::Pointer)
				return (void*)Value.Data;

			return nullptr;
		}
		const char* Variant::GetString() const
		{
			if (Type != VarType::String && Type != VarType::Base64)
				return nullptr;

			return (const char*)((String*)Value.Data)->Buffer;
		}
		unsigned char* Variant::GetBase64() const
		{
			if (Type != VarType::String && Type != VarType::Base64)
				return nullptr;

			return (unsigned char*)((String*)Value.Data)->Buffer;
		}
		int64_t Variant::GetInteger() const
		{
			if (Type == VarType::Integer)
				return Value.Integer;

			if (Type == VarType::Number)
				return (int64_t)Value.Number;

			if (Type == VarType::Decimal)
				return (int64_t)((Decimal*)Value.Data)->ToDouble();

			if (Type == VarType::Boolean)
				return Value.Boolean ? 1 : 0;

			return 0;
		}
		double Variant::GetNumber() const
		{
			if (Type == VarType::Number)
				return Value.Number;

			if (Type == VarType::Integer)
				return (double)Value.Integer;

			if (Type == VarType::Decimal)
				return ((Decimal*)Value.Data)->ToDouble();

			if (Type == VarType::Boolean)
				return Value.Boolean ? 1.0 : 0.0;

			return 0.0;
		}
		bool Variant::GetBoolean() const
		{
			if (Type == VarType::Boolean)
				return Value.Boolean;

			if (Type == VarType::Number)
				return Value.Number > 0.0;

			if (Type == VarType::Integer)
				return Value.Integer > 0;

			if (Type == VarType::Decimal)
                return ((Decimal*)Value.Data)->ToDouble() > 0.0;

			return GetSize() > 0;
		}
		VarType Variant::GetType() const
		{
			return Type;
		}
		size_t Variant::GetSize() const
		{
			switch (Type)
			{
				case VarType::Null:
				case VarType::Undefined:
				case VarType::Object:
				case VarType::Array:
				case VarType::Pointer:
					return sizeof(void*);
				case VarType::String:
				case VarType::Base64:
					return ((String*)Value.Data)->Size;
				case VarType::Decimal:
					return ((Decimal*)Value.Data)->Size();
				case VarType::Integer:
					return sizeof(int64_t);
				case VarType::Number:
					return sizeof(double);
				case VarType::Boolean:
					return sizeof(bool);
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
			return !IsEmpty();
		}
		bool Variant::IsObject() const
		{
			return Type == VarType::Object || Type == VarType::Array;
		}
		bool Variant::IsEmpty() const
		{
			switch (Type)
			{
				case VarType::Null:
				case VarType::Undefined:
					return true;
				case VarType::Object:
				case VarType::Array:
					return false;
				case VarType::Pointer:
					return Value.Data == nullptr;
				case VarType::String:
				case VarType::Base64:
					return ((String*)Value.Data)->Size == 0;
				case VarType::Decimal:
					return ((Decimal*)Value.Data)->ToDouble() == 0.0;
				case VarType::Integer:
					return Value.Integer == 0;
				case VarType::Number:
					return Value.Number == 0.0;
				case VarType::Boolean:
					return Value.Boolean == false;
				default:
					return true;
			}
		}
		bool Variant::Is(const Variant& Other) const
		{
			if (Type != Other.Type)
				return false;

			switch (Type)
			{
				case VarType::Null:
				case VarType::Undefined:
					return true;
				case VarType::Pointer:
					return GetPointer() == Other.GetPointer();
				case VarType::String:
				case VarType::Base64:
				{
					if (GetSize() != Other.GetSize())
						return false;

					const char* Src1 = GetString();
					const char* Src2 = Other.GetString();
					if (!Src1 || !Src2)
						return false;

					return strncmp(Src1, Src2, sizeof(char) * GetSize()) == 0;
				}
				case VarType::Decimal:
					return (*(Decimal*)Value.Data) == (*(Decimal*)Other.Value.Data);
				case VarType::Integer:
					return GetInteger() == Other.GetInteger();
				case VarType::Number:
					return abs(GetNumber() - Other.GetNumber()) < std::numeric_limits<double>::epsilon();
				case VarType::Boolean:
					return GetBoolean() == Other.GetBoolean();
				default:
					return false;
			}
		}
		void Variant::Copy(const Variant& Other)
		{
			Type = Other.Type;
			switch (Type)
			{
				case VarType::Null:
				case VarType::Undefined:
				case VarType::Object:
				case VarType::Array:
					Value.Data = nullptr;
					break;
				case VarType::Pointer:
					Value.Data = Other.Value.Data;
					break;
				case VarType::String:
				case VarType::Base64:
				{
					String* From = (String*)Other.Value.Data;
					String* Buffer = (String*)TH_MALLOC(sizeof(String));
					Buffer->Buffer = (char*)TH_MALLOC(sizeof(char) * (From->Size + 1));
					Buffer->Size = From->Size;

					memcpy(Buffer->Buffer, From->Buffer, sizeof(char) * From->Size);
					Buffer->Buffer[Buffer->Size] = '\0';
					Value.Data = (char*)Buffer;
					break;
				}
				case VarType::Decimal:
				{
					Decimal* From = (Decimal*)Other.Value.Data;
					Value.Data = (char*)TH_NEW(Decimal, *From);
					break;
				}
				case VarType::Integer:
					Value.Integer = Other.Value.Integer;
					break;
				case VarType::Number:
					Value.Number = Other.Value.Number;
					break;
				case VarType::Boolean:
					Value.Boolean = Other.Value.Boolean;
					break;
				default:
					Value.Data = nullptr;
					break;
			}
		}
		void Variant::Copy(Variant&& Other)
		{
			Type = Other.Type;
			Other.Type = VarType::Undefined;

			switch (Type)
			{
				case VarType::Null:
				case VarType::Undefined:
				case VarType::Object:
				case VarType::Array:
				case VarType::Pointer:
				case VarType::String:
				case VarType::Base64:
				case VarType::Decimal:
					Value.Data = Other.Value.Data;
					Other.Value.Data = nullptr;
					break;
				case VarType::Integer:
					Value.Integer = Other.Value.Integer;
					break;
				case VarType::Number:
					Value.Number = Other.Value.Number;
					break;
				case VarType::Boolean:
					Value.Boolean = Other.Value.Boolean;
					break;
				default:
					break;
			}
		}
		void Variant::Free()
		{
			switch (Type)
			{
				case VarType::Pointer:
					Value.Data = nullptr;
					break;
				case VarType::String:
				case VarType::Base64:
				{
					if (!Value.Data)
						break;

					String* Buffer = (String*)Value.Data;
					TH_FREE(Buffer->Buffer);
					TH_FREE(Value.Data);
					Value.Data = nullptr;
					break;
				}
				case VarType::Decimal:
				{
					if (!Value.Data)
						break;

					Decimal* Buffer = (Decimal*)Value.Data;
					TH_DELETE(Decimal, Buffer);
					Value.Data = nullptr;
					break;
				}
				default:
					break;
			}
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

		EventTimer::EventTimer(const TaskCallback& NewCallback, uint64_t NewTimeout, EventId NewId, bool NewAlive) : Callback(NewCallback), Timeout(NewTimeout), Id(NewId), Alive(NewAlive)
		{
		}
		EventTimer::EventTimer(TaskCallback&& NewCallback, uint64_t NewTimeout, EventId NewId, bool NewAlive) : Callback(std::move(NewCallback)), Timeout(NewTimeout), Id(NewId), Alive(NewAlive)
		{
		}
		EventTimer::EventTimer(const EventTimer& Other) : Callback(Other.Callback), Timeout(Other.Timeout), Id(Other.Id), Alive(Other.Alive)
		{
		}
		EventTimer::EventTimer(EventTimer&& Other) : Callback(std::move(Other.Callback)), Timeout(Other.Timeout), Id(Other.Id), Alive(Other.Alive)
		{
		}
		EventTimer& EventTimer::operator= (const EventTimer& Other)
		{
			Callback = Other.Callback;
			Timeout = Other.Timeout;
			Id = Other.Id;
			Alive = Other.Alive;
			return *this;
		}
		EventTimer& EventTimer::operator= (EventTimer&& Other)
		{
			Callback = std::move(Other.Callback);
			Timeout = Other.Timeout;
			Id = Other.Id;
			Alive = Other.Alive;
			return *this;
		}

		DateTime::DateTime() : Time(std::chrono::system_clock::now().time_since_epoch()), DateRebuild(false)
		{
#ifdef TH_MICROSOFT
			RtlSecureZeroMemory(&DateValue, sizeof(DateValue));
#else
			memset(&DateValue, 0, sizeof(DateValue));
#endif
		}
		DateTime::DateTime(const DateTime& Value) : Time(Value.Time), DateRebuild(false)
		{
#ifdef TH_MICROSOFT
			RtlSecureZeroMemory(&DateValue, sizeof(DateValue));
#else
			memset(&DateValue, 0, sizeof(DateValue));
#endif
		}
		void DateTime::Rebuild()
		{
			if (!DateRebuild)
				return;

			Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(mktime(&DateValue)));
			DateRebuild = false;
		}
		DateTime& DateTime::operator= (const DateTime& Other)
		{
			Time = Other.Time;
			DateRebuild = false;
#ifdef TH_MICROSOFT
			RtlSecureZeroMemory(&DateValue, sizeof(DateValue));
#else
			memset(&DateValue, 0, sizeof(DateValue));
#endif
			return *this;
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
			else if (Value <= 14)
				DateValue.tm_wday = (int)Value - 8;
			else if (Value <= 21)
				DateValue.tm_wday = (int)Value - 15;
			else
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
			L = TH_NEW(std::string);
		}
		Parser::Parser(int Value) : Safe(true)
		{
			L = TH_NEW(std::string, std::to_string(Value));
		}
		Parser::Parser(unsigned int Value) : Safe(true)
		{
			L = TH_NEW(std::string, std::to_string(Value));
		}
		Parser::Parser(int64_t Value) : Safe(true)
		{
			L = TH_NEW(std::string, std::to_string(Value));
		}
		Parser::Parser(uint64_t Value) : Safe(true)
		{
			L = TH_NEW(std::string, std::to_string(Value));
		}
		Parser::Parser(float Value) : Safe(true)
		{
			L = TH_NEW(std::string, std::to_string(Value));
		}
		Parser::Parser(double Value) : Safe(true)
		{
			L = TH_NEW(std::string, std::to_string(Value));
		}
		Parser::Parser(long double Value) : Safe(true)
		{
			L = TH_NEW(std::string, std::to_string(Value));
		}
		Parser::Parser(const std::string& Buffer) : Safe(true)
		{
			L = TH_NEW(std::string, Buffer);
		}
		Parser::Parser(std::string* Buffer)
		{
			Safe = (!Buffer);
			L = (Safe ? TH_NEW(std::string) : Buffer);
		}
		Parser::Parser(const std::string* Buffer)
		{
			Safe = (!Buffer);
			L = (Safe ? TH_NEW(std::string) : (std::string*)Buffer);
		}
		Parser::Parser(const char* Buffer) : Safe(true)
		{
			if (Buffer != nullptr)
				L = TH_NEW(std::string, Buffer);
			else
				L = TH_NEW(std::string);
		}
		Parser::Parser(const char* Buffer, int64_t Length) : Safe(true)
		{
			if (Buffer != nullptr)
				L = TH_NEW(std::string, Buffer, Length);
			else
				L = TH_NEW(std::string);
		}
		Parser::Parser(const Parser& Value) : Safe(true)
		{
			if (Value.L != nullptr)
				L = TH_NEW(std::string, *Value.L);
			else
				L = TH_NEW(std::string);
		}
		Parser::~Parser()
		{
			if (Safe)
				TH_DELETE(basic_string, L);
		}
		Parser& Parser::EscapePrint()
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
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
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
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
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
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
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			L->reserve(L->capacity() + Count);
			return *this;
		}
		Parser& Parser::Resize(uint64_t Count)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			L->resize(Count);
			return *this;
		}
		Parser& Parser::Resize(uint64_t Count, char Char)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			L->resize(Count, Char);
			return *this;
		}
		Parser& Parser::Clear()
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			L->clear();
			return *this;
		}
		Parser& Parser::ToUpper()
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			std::transform(L->begin(), L->end(), L->begin(), ::toupper);
			return *this;
		}
		Parser& Parser::ToLower()
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			std::transform(L->begin(), L->end(), L->begin(), ::tolower);
			return *this;
		}
		Parser& Parser::Clip(uint64_t Length)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			if (Length < L->size())
				L->erase(Length, L->size() - Length);

			return *this;
		}
		Parser& Parser::ReplaceOf(const char* Chars, const char* To, uint64_t Start)
		{
			TH_ASSERT(Chars != nullptr && Chars[0] != '\0' && To != nullptr, *this, "match list and replacer should not be empty");

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
			TH_ASSERT(Chars != nullptr && Chars[0] != '\0' && To != nullptr, *this, "match list and replacer should not be empty");

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
			TH_ASSERT(!From.empty(), *this, "match should not be empty");

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
			TH_ASSERT(From != nullptr && To != nullptr, *this, "from and to should not be empty");

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
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			for (uint64_t i = Position; i < L->size(); i++)
			{
				if (L->at(i) == From)
					L->at(i) = To;
			}

			return *this;
		}
		Parser& Parser::Replace(const char& From, const char& To, uint64_t Position, uint64_t Count)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(L->size() >= (Position + Count), *this, "invalid offset");

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
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(Start < L->size(), *this, "invalid start");
			TH_ASSERT(End <= L->size(), *this, "invalid end");
			TH_ASSERT(Start < End, *this, "start should be less than end");
			TH_ASSERT(Value != nullptr, *this, "replacer should not be empty");

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
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(Start < L->size(), *this, "invalid start");
			TH_ASSERT(End <= L->size(), *this, "invalid end");
			TH_ASSERT(Start < End, *this, "start should be less than end");

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
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			return Reverse(0, L->size() - 1);
		}
		Parser& Parser::Reverse(uint64_t Start, uint64_t End)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(L->size() >= 2, *this, "length should be at least 2 chars");
			TH_ASSERT(End <= (L->size() - 1), *this, "end should be less than length - 1");
			TH_ASSERT(Start <= (L->size() - 1), *this, "start should be less than length - 1");
			TH_ASSERT(Start < End, *this, "start should be less than end");

			std::reverse(L->begin() + Start, L->begin() + End + 1);
			return *this;
		}
		Parser& Parser::Substring(uint64_t Start)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(Start < L->size(), *this, "start should not be greater than length");

			L->assign(L->substr(Start));
			return *this;
		}
		Parser& Parser::Substring(uint64_t Start, uint64_t Count)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(Start < L->size(), *this, "start should not be greater than length");
			TH_ASSERT(Count > 0, *this, "count should be greater than zero");

			L->assign(L->substr(Start, Count));
			return *this;
		}
		Parser& Parser::Substring(const Parser::Settle& Result)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(Result.Found, *this, "result should be found");
			TH_ASSERT(Result.Start <= (L->size() - 1), *this, "result start should be less or equal than length - 1");

			auto Offset = (int64_t)Result.End;
			if (Result.End > L->size())
				Offset = (int64_t)(L->size() - Result.Start);

			Offset = (int64_t)Result.Start - Offset;
			L->assign(L->substr(Result.Start, (uint64_t)(Offset < 0 ? -Offset : Offset)));
			return *this;
		}
		Parser& Parser::Splice(uint64_t Start, uint64_t End)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(Start <= (L->size() - 1), *this, "result start should be less or equal than length - 1");

			if (End > L->size())
				End = (L->size() - Start);

			int64_t Offset = (int64_t)Start - (int64_t)End;
			L->assign(L->substr(Start, (uint64_t)(Offset < 0 ? -Offset : Offset)));
			return *this;
		}
		Parser& Parser::Trim()
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			L->erase(L->begin(), std::find_if(L->begin(), L->end(), [](int C) -> int
			{
				if (C < -1 || C > 255)
					return 1;

				return !std::isspace(C);
			}));
			L->erase(std::find_if(L->rbegin(), L->rend(), [](int C) -> int
			{
				if (C < -1 || C > 255)
					return 1;

				return !std::isspace(C);
			}).base(), L->end());

			return *this;
		}
		Parser& Parser::Fill(const char& Char)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(!L->empty(), *this, "length should be greater than zero");

			for (char& i : *L)
				i = Char;

			return *this;
		}
		Parser& Parser::Fill(const char& Char, uint64_t Count)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(!L->empty(), *this, "length should be greater than zero");

			L->assign(Count, Char);
			return *this;
		}
		Parser& Parser::Fill(const char& Char, uint64_t Start, uint64_t Count)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(!L->empty(), *this, "length should be greater than zero");
			TH_ASSERT(Start <= L->size(), *this, "start should be less or equal than length");

			if (Start + Count > L->size())
				Count = L->size() - Start;

			for (uint64_t i = Start; i < (Start + Count); i++)
				L->at(i) = Char;

			return *this;
		}
		Parser& Parser::Assign(const char* Raw)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			if (Raw != nullptr)
				L->assign(Raw);
			else
				L->clear();

			return *this;
		}
		Parser& Parser::Assign(const char* Raw, uint64_t Length)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			if (Raw != nullptr)
				L->assign(Raw, Length);
			else
				L->clear();

			return *this;
		}
		Parser& Parser::Assign(const std::string& Raw)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			L->assign(Raw);
			return *this;
		}
		Parser& Parser::Assign(const std::string& Raw, uint64_t Start, uint64_t Count)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			L->assign(Raw.substr(Start, Count));
			return *this;
		}
		Parser& Parser::Assign(const char* Raw, uint64_t Start, uint64_t Count)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(Raw != nullptr, *this, "assign string should be set");

			L->assign(Raw);
			return Substring(Start, Count);
		}
		Parser& Parser::Append(const char* Raw)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(Raw != nullptr, *this, "append string should be set");

			L->append(Raw);
			return *this;
		}
		Parser& Parser::Append(const char& Char)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			L->append(1, Char);
			return *this;
		}
		Parser& Parser::Append(const char& Char, uint64_t Count)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			L->append(Count, Char);
			return *this;
		}
		Parser& Parser::Append(const std::string& Raw)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			L->append(Raw);
			return *this;
		}
		Parser& Parser::Append(const char* Raw, uint64_t Count)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(Raw != nullptr, *this, "append string should be set");
			
			L->append(Raw, Count);
			return *this;
		}
		Parser& Parser::Append(const char* Raw, uint64_t Start, uint64_t Count)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(Raw != nullptr, *this, "append string should be set");
			TH_ASSERT(Count > 0, *this, "count should be greater than zero");

			std::string V(Raw);
			TH_ASSERT(V.size() >= Start + Count, *this, "offset should be less than append string length");

			L->append(V.substr(Start, Count));
			return *this;
		}
		Parser& Parser::Append(const std::string& Raw, uint64_t Start, uint64_t Count)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(Count > 0, *this, "count should be greater than zero");
			TH_ASSERT(Raw.size() >= Start + Count, *this, "offset should be less than append string length");

			L->append(Raw.substr(Start, Count));
			return *this;
		}
		Parser& Parser::fAppend(const char* Format, ...)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(Format != nullptr, *this, "format should be set");

			char Buffer[16384];
			va_list Args;
			va_start(Args, Format);
			int Count = vsnprintf(Buffer, sizeof(Buffer), Format, Args);
			va_end(Args);

			return Append(Buffer, Count);
		}
		Parser& Parser::Insert(const std::string& Raw, uint64_t Position)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			if (Position >= L->size())
				Position = L->size();

			L->insert(Position, Raw);
			return *this;
		}
		Parser& Parser::Insert(const std::string& Raw, uint64_t Position, uint64_t Start, uint64_t Count)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			if (Position >= L->size())
				Position = L->size();

			if (Raw.size() >= Start + Count)
				L->insert(Position, Raw.substr(Start, Count));

			return *this;
		}
		Parser& Parser::Insert(const std::string& Raw, uint64_t Position, uint64_t Count)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			if (Position >= L->size())
				Position = L->size();

			if (Count >= Raw.size())
				Count = Raw.size();

			L->insert(Position, Raw.substr(0, Count));
			return *this;
		}
		Parser& Parser::Insert(const char& Char, uint64_t Position, uint64_t Count)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(Position < L->size(), *this, "position should be less than length");
			L->insert(Position, Count, Char);
			return *this;
		}
		Parser& Parser::Insert(const char& Char, uint64_t Position)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(Position < L->size(), *this, "position should be less than length");
			L->insert(L->begin() + Position, Char);
			return *this;
		}
		Parser& Parser::Erase(uint64_t Position)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(Position < L->size(), *this, "position should be less than length");
			L->erase(Position);
			return *this;
		}
		Parser& Parser::Erase(uint64_t Position, uint64_t Count)
		{
			TH_ASSERT(L != nullptr, *this, "cannot parse without context");
			TH_ASSERT(Position < L->size(), *this, "position should be less than length");
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
			TH_ASSERT(L != nullptr, Parser::Settle(), "cannot parse without context");
			if (L->empty())
				return { L->size() - 1, L->size(), false };

			const char* Ptr = L->c_str() - Offset;
			if (Needle.c_str() > Ptr)
				return { L->size() - 1, L->size(), false };

			const char* It = nullptr;
			for (It = Ptr + L->size() - Needle.size(); It > Ptr; --It)
			{
				if (strncmp(Ptr, Needle.c_str(), (size_t)Needle.size()) == 0)
				{
					uint64_t Set = (uint64_t)(It - Ptr);
					return { Set, Set + (uint64_t)Needle.size(), true };
				}
			}

			return { L->size() - 1, L->size(), false };
		}
		Parser::Settle Parser::ReverseFind(const char* Needle, uint64_t Offset) const
		{
			TH_ASSERT(L != nullptr, Parser::Settle(), "cannot parse without context");
			if (L->empty())
				return { L->size() - 1, L->size(), false };

			if (!Needle)
				return { L->size() - 1, L->size(), false };

			const char* Ptr = L->c_str() - Offset;
			if (Needle > Ptr)
				return { L->size() - 1, L->size(), false };

			const char* It = nullptr;
			uint64_t Length = (uint64_t)strlen(Needle);
			for (It = Ptr + L->size() - Length; It > Ptr; --It)
			{
				if (strncmp(Ptr, Needle, (size_t)Length) == 0)
					return { (uint64_t)(It - Ptr), (uint64_t)(It - Ptr + Length), true };
			}

			return { L->size() - 1, L->size(), false };
		}
		Parser::Settle Parser::ReverseFind(const char& Needle, uint64_t Offset) const
		{
			TH_ASSERT(L != nullptr, Parser::Settle(), "cannot parse without context");
			if (L->empty())
				return { L->size() - 1, L->size(), false };

			uint64_t Size = L->size() - 1 - Offset;
			for (int64_t i = Size; i > -1; i--)
			{
				if (L->at(i) == Needle)
					return { (uint64_t)i, (uint64_t)i + 1, true };
			}

			return { L->size() - 1, L->size(), false };
		}
		Parser::Settle Parser::ReverseFindUnescaped(const char& Needle, uint64_t Offset) const
		{
			TH_ASSERT(L != nullptr, Parser::Settle(), "cannot parse without context");
			if (L->empty())
				return { L->size() - 1, L->size(), false };

			uint64_t Size = L->size() - 1 - Offset;
			for (int64_t i = Size; i > -1; i--)
			{
				if (L->at(i) == Needle && ((int64_t)i - 1 < 0 || L->at(i - 1) != '\\'))
					return { (uint64_t)i, (uint64_t)i + 1, true };
			}

			return { L->size() - 1, L->size(), false };
		}
		Parser::Settle Parser::ReverseFindOf(const std::string& Needle, uint64_t Offset) const
		{
			TH_ASSERT(L != nullptr, Parser::Settle(), "cannot parse without context");
			if (L->empty())
				return { L->size() - 1, L->size(), false };

			uint64_t Size = L->size() - 1 - Offset;
			for (int64_t i = Size; i > -1; i--)
			{
				for (char k : Needle)
				{
					if (L->at(i) == k)
						return { (uint64_t)i, (uint64_t)i + 1, true };
				}
			}

			return { L->size() - 1, L->size(), false };
		}
		Parser::Settle Parser::ReverseFindOf(const char* Needle, uint64_t Offset) const
		{
			TH_ASSERT(L != nullptr, Parser::Settle(), "cannot parse without context");
			if (L->empty())
				return { L->size() - 1, L->size(), false };

			if (!Needle)
				return { L->size() - 1, L->size(), false };

			uint64_t Length = strlen(Needle);
			uint64_t Size = L->size() - 1 - Offset;
			for (int64_t i = Size; i > -1; i--)
			{
				for (uint64_t k = 0; k < Length; k++)
				{
					if (L->at(i) == Needle[k])
						return { (uint64_t)i, (uint64_t)i + 1, true };
				}
			}

			return { L->size() - 1, L->size(), false };
		}
		Parser::Settle Parser::Find(const std::string& Needle, uint64_t Offset) const
		{
			TH_ASSERT(L != nullptr, Parser::Settle(), "cannot parse without context");
			const char* It = strstr(L->c_str() + Offset, Needle.c_str());
			if (It == nullptr)
				return { L->size() - 1, L->size(), false };

			uint64_t Set = (uint64_t)(It - L->c_str());
			return { Set, Set + (uint64_t)Needle.size(), true };
		}
		Parser::Settle Parser::Find(const char* Needle, uint64_t Offset) const
		{
			TH_ASSERT(L != nullptr, Parser::Settle(), "cannot parse without context");
			TH_ASSERT(Needle != nullptr, Parser::Settle(), "needle should be set");

			const char* It = strstr(L->c_str() + Offset, Needle);
			if (It == nullptr)
				return { L->size() - 1, L->size(), false };

			uint64_t Set = (uint64_t)(It - L->c_str());
			return { Set, Set + (uint64_t)strlen(Needle), true };
		}
		Parser::Settle Parser::Find(const char& Needle, uint64_t Offset) const
		{
			TH_ASSERT(L != nullptr, Parser::Settle(), "cannot parse without context");
			for (uint64_t i = Offset; i < L->size(); i++)
			{
				if (L->at(i) == Needle)
					return { i, i + 1, true };
			}

			return { L->size() - 1, L->size(), false };
		}
		Parser::Settle Parser::FindUnescaped(const char& Needle, uint64_t Offset) const
		{
			TH_ASSERT(L != nullptr, Parser::Settle(), "cannot parse without context");
			for (uint64_t i = Offset; i < L->size(); i++)
			{
				if (L->at(i) == Needle && ((int64_t)i - 1 < 0 || L->at(i - 1) != '\\'))
					return { i, i + 1, true };
			}

			return { L->size() - 1, L->size(), false };
		}
		Parser::Settle Parser::FindOf(const std::string& Needle, uint64_t Offset) const
		{
			TH_ASSERT(L != nullptr, Parser::Settle(), "cannot parse without context");
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
			TH_ASSERT(L != nullptr, Parser::Settle(), "cannot parse without context");
			TH_ASSERT(Needle != nullptr, Parser::Settle(), "needle should be set");

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
			TH_ASSERT(L != nullptr, Parser::Settle(), "cannot parse without context");
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
			TH_ASSERT(L != nullptr, Parser::Settle(), "cannot parse without context");
			TH_ASSERT(Needle != nullptr, Parser::Settle(), "needle should be set");

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
			TH_ASSERT(L != nullptr, false, "cannot parse without context");
			TH_ASSERT(Value != nullptr, false, "value should be set");

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
			TH_ASSERT(L != nullptr, false, "cannot parse without context");
			TH_ASSERT(Value != nullptr, false, "value should be set");

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
			TH_ASSERT(L != nullptr, false, "cannot parse without context");
			TH_ASSERT(Value != nullptr, false, "value should be set");

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
			TH_ASSERT(L != nullptr, false, "cannot parse without context");
			if (L->empty())
				return false;

			return strcmp(L->c_str() + L->size() - Value.size(), Value.c_str()) == 0;
		}
		bool Parser::EndsWith(const char* Value) const
		{
			TH_ASSERT(L != nullptr, false, "cannot parse without context");
			TH_ASSERT(Value != nullptr, false, "value should be set");

			if (L->empty() || !Value)
				return false;

			return strcmp(L->c_str() + L->size() - strlen(Value), Value) == 0;
		}
		bool Parser::EndsWith(const char& Value) const
		{
			TH_ASSERT(L != nullptr, false, "cannot parse without context");
			return !L->empty() && L->back() == Value;
		}
		bool Parser::EndsOf(const char* Value) const
		{
			TH_ASSERT(L != nullptr, false, "cannot parse without context");
			TH_ASSERT(Value != nullptr, false, "value should be set");

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
			TH_ASSERT(L != nullptr, false, "cannot parse without context");
			TH_ASSERT(Value != nullptr, false, "value should be set");

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
			TH_ASSERT(L != nullptr, false, "cannot parse without context");
			return L->empty();
		}
		bool Parser::HasInteger() const
		{
			TH_ASSERT(L != nullptr, false, "cannot parse without context");
			if (L->empty())
				return false;

			bool HadSign = false;
			for (size_t i = 0; i < L->size(); i++)
			{
				char& V = (*L)[i];
				if (IsDigit(V))
					continue;

				if ((V == '+' || V == '-') && i == 0 && !HadSign)
				{
					HadSign = true;
					continue;
				}

				return false;
			}

			return true;
		}
		bool Parser::HasNumber() const
		{
			TH_ASSERT(L != nullptr, false, "cannot parse without context");
			if (L->empty() || (L->size() == 1 && L->front() == '.'))
				return false;

			bool HadPoint = false, HadSign = false;
			for (size_t i = 0; i < L->size(); i++)
			{
				char& V = (*L)[i];
				if (IsDigit(V))
					continue;

				if ((V == '+' || V == '-') && i == 0 && !HadSign)
				{
					HadSign = true;
					continue;
				}

				if (V == '.' && !HadPoint)
				{
					HadPoint = true;
					continue;
				}

				return false;
			}

			return true;
		}
		bool Parser::HasDecimal() const
		{
			TH_ASSERT(L != nullptr, false, "cannot parse without context");

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
			TH_ASSERT(L != nullptr, false, "cannot parse without context");
			return !strncmp(L->c_str(), "true", 4) || !strncmp(L->c_str(), "1", 1);
		}
		bool Parser::IsDigit(char Char)
		{
			return Char == '0' || Char == '1' || Char == '2' || Char == '3' || Char == '4' || Char == '5' || Char == '6' || Char == '7' || Char == '8' || Char == '9';
		}
		int Parser::CaseCompare(const char* Value1, const char* Value2)
		{
			TH_ASSERT(Value1 != nullptr && Value2 != nullptr, 0, "both values should be set");

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
			TH_ASSERT(Pattern != nullptr && Text != nullptr, -1, "pattern and text should be set");

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
			TH_ASSERT(L != nullptr, 0, "cannot parse without context");
			return (int)strtol(L->c_str(), nullptr, 10);
		}
		long Parser::ToLong() const
		{
			TH_ASSERT(L != nullptr, 0, "cannot parse without context");
			return strtol(L->c_str(), nullptr, 10);
		}
		float Parser::ToFloat() const
		{
			TH_ASSERT(L != nullptr, 0.0f, "cannot parse without context");
			return strtof(L->c_str(), nullptr);
		}
		unsigned int Parser::ToUInt() const
		{
			TH_ASSERT(L != nullptr, 0, "cannot parse without context");
			return (unsigned int)ToULong();
		}
		unsigned long Parser::ToULong() const
		{
			TH_ASSERT(L != nullptr, 0, "cannot parse without context");
			return strtoul(L->c_str(), nullptr, 10);
		}
		int64_t Parser::ToInt64() const
		{
			TH_ASSERT(L != nullptr, 0, "cannot parse without context");
			return strtoll(L->c_str(), nullptr, 10);
		}
		double Parser::ToDouble() const
		{
			TH_ASSERT(L != nullptr, 0.0, "cannot parse without context");
			return strtod(L->c_str(), nullptr);
		}
		long double Parser::ToLDouble() const
		{
			TH_ASSERT(L != nullptr, 0.0, "cannot parse without context");
			return strtold(L->c_str(), nullptr);
		}
		uint64_t Parser::ToUInt64() const
		{
			TH_ASSERT(L != nullptr, 0, "cannot parse without context");
			return strtoull(L->c_str(), nullptr, 10);
		}
		uint64_t Parser::Size() const
		{
			TH_ASSERT(L != nullptr, 0, "cannot parse without context");
			return L->size();
		}
		uint64_t Parser::Capacity() const
		{
			TH_ASSERT(L != nullptr, 0, "cannot parse without context");
			return L->capacity();
		}
		char* Parser::Value() const
		{
			TH_ASSERT(L != nullptr, nullptr, "cannot parse without context");
			return (char*)L->data();
		}
		const char* Parser::Get() const
		{
			TH_ASSERT(L != nullptr, nullptr, "cannot parse without context");
			return L->c_str();
		}
		std::string& Parser::R()
		{
			TH_ASSERT(L != nullptr, *L, "cannot parse without context");
			return *L;
		}
		std::wstring Parser::ToWide() const
		{
			TH_ASSERT(L != nullptr, std::wstring(), "cannot parse without context");
#pragma warning(push)
#pragma warning(disable: 4333)
			std::wstring Output;
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
			TH_ASSERT(L != nullptr, std::vector<std::string>(), "cannot parse without context");
			Parser::Settle Result = Find(With, Start);
			uint64_t Offset = Start;

			std::vector<std::string> Output;
			while (Result.Found)
			{
				Output.emplace_back(L->substr(Offset, Result.Start - Offset));
				Result = Find(With, Offset = Result.End);
			}

			if (Offset < L->size())
				Output.emplace_back(L->substr(Offset));

			return Output;
		}
		std::vector<std::string> Parser::Split(char With, uint64_t Start) const
		{
			TH_ASSERT(L != nullptr, std::vector<std::string>(), "cannot parse without context");
			Parser::Settle Result = Find(With, Start);
			uint64_t Offset = Start;

			std::vector<std::string> Output;
			while (Result.Found)
			{
				Output.emplace_back(L->substr(Offset, Result.Start - Offset));
				Result = Find(With, Offset = Result.End);
			}

			if (Offset < L->size())
				Output.emplace_back(L->substr(Offset));

			return Output;
		}
		std::vector<std::string> Parser::SplitMax(char With, uint64_t Count, uint64_t Start) const
		{
			TH_ASSERT(L != nullptr, std::vector<std::string>(), "cannot parse without context");
			Parser::Settle Result = Find(With, Start);
			uint64_t Offset = Start;

			std::vector<std::string> Output;
			while (Result.Found && Output.size() < Count)
			{
				Output.emplace_back(L->substr(Offset, Result.Start - Offset));
				Result = Find(With, Offset = Result.End);
			}

			if (Offset < L->size() && Output.size() < Count)
				Output.emplace_back(L->substr(Offset));

			return Output;
		}
		std::vector<std::string> Parser::SplitOf(const char* With, uint64_t Start) const
		{
			TH_ASSERT(L != nullptr, std::vector<std::string>(), "cannot parse without context");
			Parser::Settle Result = FindOf(With, Start);
			uint64_t Offset = Start;

			std::vector<std::string> Output;
			while (Result.Found)
			{
				Output.emplace_back(L->substr(Offset, Result.Start - Offset));
				Result = FindOf(With, Offset = Result.End);
			}

			if (Offset < L->size())
				Output.emplace_back(L->substr(Offset));

			return Output;
		}
		std::vector<std::string> Parser::SplitNotOf(const char* With, uint64_t Start) const
		{
			TH_ASSERT(L != nullptr, std::vector<std::string>(), "cannot parse without context");
			Parser::Settle Result = FindNotOf(With, Start);
			uint64_t Offset = Start;

			std::vector<std::string> Output;
			while (Result.Found)
			{
				Output.emplace_back(L->substr(Offset, Result.Start - Offset));
				Result = FindNotOf(With, Offset = Result.End);
			}

			if (Offset < L->size())
				Output.emplace_back(L->substr(Offset));

			return Output;
		}
		Parser& Parser::operator= (const Parser& Value)
		{
			TH_ASSERT(&Value != this, *this, "cannot set to self");
			if (Safe)
				TH_DELETE(basic_string, L);

			Safe = true;
			if (Value.L != nullptr)
				L = TH_NEW(std::string, *Value.L);
			else
				L = TH_NEW(std::string);

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

		Spin::Spin()
		{
		}
		void Spin::Lock()
		{
			while (Atom.test_and_set(std::memory_order_acquire))
				std::this_thread::yield();
		}
		void Spin::Unlock()
		{
			Atom.clear(std::memory_order_release);
		}

		Document* Var::Set::Auto(Variant&& Value)
		{
			return new Document(std::move(Value));
		}
		Document* Var::Set::Auto(const Variant& Value)
		{
			return new Document(Value);
		}
		Document* Var::Set::Auto(const std::string& Value, bool Strict)
		{
			return new Document(Var::Auto(Value, Strict));
		}
		Document* Var::Set::Null()
		{
			return new Document(Var::Null());
		}
		Document* Var::Set::Undefined()
		{
			return new Document(Var::Undefined());
		}
		Document* Var::Set::Object()
		{
			return new Document(Var::Object());
		}
		Document* Var::Set::Array()
		{
			return new Document(Var::Array());
		}
		Document* Var::Set::Pointer(void* Value)
		{
			return new Document(Var::Pointer(Value));
		}
		Document* Var::Set::String(const std::string& Value)
		{
			return new Document(Var::String(Value));
		}
		Document* Var::Set::String(const char* Value, size_t Size)
		{
			return new Document(Var::String(Value, Size));
		}
		Document* Var::Set::Base64(const std::string& Value)
		{
			return new Document(Var::Base64(Value));
		}
		Document* Var::Set::Base64(const unsigned char* Value, size_t Size)
		{
			return new Document(Var::Base64(Value, Size));
		}
		Document* Var::Set::Base64(const char* Value, size_t Size)
		{
			return new Document(Var::Base64(Value, Size));
		}
		Document* Var::Set::Integer(int64_t Value)
		{
			return new Document(Var::Integer(Value));
		}
		Document* Var::Set::Number(double Value)
		{
			return new Document(Var::Number(Value));
		}
		Document* Var::Set::Decimal(const BigNumber& Value)
		{
			return new Document(Var::Decimal(Value));
		}
		Document* Var::Set::Decimal(BigNumber&& Value)
		{
			return new Document(Var::Decimal(std::move(Value)));
		}
		Document* Var::Set::DecimalString(const std::string& Value)
		{
			return new Document(Var::DecimalString(Value));
		}
		Document* Var::Set::Boolean(bool Value)
		{
			return new Document(Var::Boolean(Value));
		}

		Variant Var::Auto(const std::string& Value, bool Strict)
		{
			Variant Result;
			Result.Deserialize(Value, Strict);

			return Result;
		}
		Variant Var::Null()
		{
			return Variant(VarType::Null);
		}
		Variant Var::Undefined()
		{
			return Variant(VarType::Undefined);
		}
		Variant Var::Object()
		{
			return Variant(VarType::Object);
		}
		Variant Var::Array()
		{
			return Variant(VarType::Array);
		}
		Variant Var::Pointer(void* Value)
		{
			if (!Value)
				return Null();

			Variant Result(VarType::Pointer);
			Result.Value.Data = (char*)Value;
			return Result;
		}
		Variant Var::String(const std::string& Value)
		{
			Variant::String* Buffer = (Variant::String*)TH_MALLOC(sizeof(Variant::String));
			Buffer->Size = Value.size();
			Buffer->Buffer = (char*)TH_MALLOC(sizeof(char) * (Buffer->Size + 1));

			memcpy(Buffer->Buffer, Value.c_str(), sizeof(char) * Buffer->Size);
			Buffer->Buffer[Buffer->Size] = '\0';

			Variant Result(VarType::String);
			Result.Value.Data = (char*)Buffer;
			return Result;
		}
		Variant Var::String(const char* Value, size_t Size)
		{
			TH_ASSERT(Value != nullptr, Null(), "value should be set");
			Variant::String* Buffer = (Variant::String*)TH_MALLOC(sizeof(Variant::String));
			Buffer->Size = Size;
			Buffer->Buffer = (char*)TH_MALLOC(sizeof(char) * (Buffer->Size + 1));

			memcpy(Buffer->Buffer, Value, sizeof(char) * Buffer->Size);
			Buffer->Buffer[Buffer->Size] = '\0';

			Variant Result(VarType::String);
			Result.Value.Data = (char*)Buffer;
			return Result;
		}
		Variant Var::Base64(const std::string& Value)
		{
			Variant::String* Buffer = (Variant::String*)TH_MALLOC(sizeof(Variant::String));
			Buffer->Size = Value.size();
			Buffer->Buffer = (char*)TH_MALLOC(sizeof(char) * (Buffer->Size + 1));

			memcpy(Buffer->Buffer, Value.c_str(), sizeof(char) * Buffer->Size);
			Buffer->Buffer[Buffer->Size] = '\0';

			Variant Result(VarType::Base64);
			Result.Value.Data = (char*)Buffer;
			return Result;
		}
		Variant Var::Base64(const unsigned char* Value, size_t Size)
		{
			return Base64((const char*)Value, Size);
		}
		Variant Var::Base64(const char* Value, size_t Size)
		{
			TH_ASSERT(Value != nullptr, Null(), "value should be set");
			Variant::String* Buffer = (Variant::String*)TH_MALLOC(sizeof(Variant::String));
			Buffer->Size = Size;
			Buffer->Buffer = (char*)TH_MALLOC(sizeof(char) * (Buffer->Size + 1));

			memcpy(Buffer->Buffer, Value, sizeof(char) * Buffer->Size);
			Buffer->Buffer[Buffer->Size] = '\0';

			Variant Result(VarType::Base64);
			Result.Value.Data = (char*)Buffer;
			return Result;
		}
		Variant Var::Integer(int64_t Value)
		{
			Variant Result(VarType::Integer);
			Result.Value.Integer = Value;
			return Result;
		}
		Variant Var::Number(double Value)
		{
			Variant Result(VarType::Number);
			Result.Value.Number = Value;
			return Result;
		}
		Variant Var::Decimal(const BigNumber& Value)
		{
			BigNumber* Buffer = TH_NEW(BigNumber, Value);
			Variant Result(VarType::Decimal);
			Result.Value.Data = (char*)Buffer;
			return Result;
		}
		Variant Var::Decimal(BigNumber&& Value)
		{
			BigNumber* Buffer = TH_NEW(BigNumber, std::move(Value));
			Variant Result(VarType::Decimal);
			Result.Value.Data = (char*)Buffer;
			return Result;
		}
		Variant Var::DecimalString(const std::string& Value)
		{
			BigNumber* Buffer = TH_NEW(BigNumber, Value);
			Variant Result(VarType::Decimal);
			Result.Value.Data = (char*)Buffer;
			return Result;
		}
		Variant Var::Boolean(bool Value)
		{
			Variant Result(VarType::Boolean);
			Result.Value.Boolean = Value;
			return Result;
		}

		void Mem::SetAlloc(const AllocCallback& Callback)
		{
			OnAlloc = Callback;
		}
		void Mem::SetRealloc(const ReallocCallback& Callback)
		{
			OnRealloc = Callback;
		}
		void Mem::SetFree(const FreeCallback& Callback)
		{
			OnFree = Callback;
		}
		void* Mem::Malloc(size_t Size)
		{
			void* Result = (OnAlloc ? OnAlloc(Size) : malloc(Size));
			TH_ASSERT(Result != nullptr, nullptr, "not enough memory to malloc %llu bytes", (uint64_t)Size);
			return Result;
		}
		void* Mem::Realloc(void* Ptr, size_t Size)
		{
			if (!Ptr)
				return Malloc(Size);

			void* Result = (OnRealloc ? OnRealloc(Ptr, Size) : realloc(Ptr, Size));
			TH_ASSERT(Result != nullptr, nullptr, "not enough memory to realloc %llu bytes", (uint64_t)Size);
			return Result;
		}
		void Mem::Free(void* Ptr)
		{
			if (!Ptr)
				return;

			if (!OnFree)
				return free(Ptr);

			return OnFree(Ptr);
		}
		AllocCallback Mem::OnAlloc;
		ReallocCallback Mem::OnRealloc;
		FreeCallback Mem::OnFree;

		static thread_local Debug::Context DbgPerf;
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
		void Debug::OpStart(const char* File, const char* Section, const char* Function, int Line, uint64_t ThresholdMS, void* Id)
		{
			TH_ASSERT_V(File != nullptr, "file should be set");
			TH_ASSERT_V(Section != nullptr, "section should be set");
			TH_ASSERT_V(Function != nullptr, "function should be set");
			TH_ASSERT_V(ThresholdMS > 0, "threshold time should be greater than zero");

			Context Ctx;
			Ctx.File = File;
			Ctx.Section = Section;
			Ctx.Function = Function;
			Ctx.Id = Id;
			Ctx.Threshold = ThresholdMS * 1000;
			Ctx.Time = DateTime().Microseconds();
			Ctx.Line = Line;

			Safe.lock();
			Contexts.emplace_back(std::move(Ctx));
			Safe.unlock();
		}
		void Debug::OpSignal()
		{
			if (Contexts.empty())
				return;

			Safe.lock();
			for (auto& Ctx : Contexts)
			{
				uint64_t Time = DateTime().Microseconds();
				uint64_t Diff = Time - Ctx.Time;
				if (Diff > Ctx.Threshold)
				{
					TH_WARN("[perf] task @%s takes %llu ms (%llu us)\n\tfunction: %s()\n\tfile: %s:%i\n\tcontext: 0x%p\n\texpected: %llu ms at most", Ctx.Section, Diff / 1000, Diff, Ctx.Function, Ctx.File, Ctx.Line, Ctx.Id, Ctx.Threshold / 1000);
					Ctx.Time = Time;
				}
			}
			Safe.unlock();
		}
		void Debug::OpEnd(void* Id)
		{
			Safe.lock();
			for (auto It = Contexts.begin(); It != Contexts.end(); It++)
			{
				Context& Ctx = *It;
				if (Ctx.Id != Id)
					continue;

				uint64_t Diff = DateTime().Microseconds() - Ctx.Time;
				if (Diff > Ctx.Threshold)
					TH_WARN("[perf] task @%s took %llu ms (%llu us)\n\tfunction: %s()\n\tfile: %s:%i\n\tcontext: 0x%p\n\texpected: %llu ms at most", Ctx.Section, Diff / 1000, Diff, Ctx.Function, Ctx.File, Ctx.Line, Ctx.Id, Ctx.Threshold / 1000);
				
				Contexts.erase(It);
				break;
			}
			Safe.unlock();
		}
		void Debug::TimeStart(const char* File, const char* Section, const char* Function, int Line, uint64_t ThresholdMS)
		{
			TH_ASSERT_V(File != nullptr, "file should be set");
			TH_ASSERT_V(Section != nullptr, "section should be set");
			TH_ASSERT_V(Function != nullptr, "function should be set");
			TH_ASSERT_V(ThresholdMS > 0, "threshold time should be greater than zero");

			if (DbgPerf.Ignore)
				return;

			DbgPerf.File = File;
			DbgPerf.Section = Section;
			DbgPerf.Function = Function;
			DbgPerf.Threshold = ThresholdMS * 1000;
			DbgPerf.Time = DateTime().Microseconds();
			DbgPerf.Line = Line;
		}
		void Debug::TimeSignal()
		{
			TH_ASSERT_V(DbgPerf.File != nullptr, "file should be set");
			TH_ASSERT_V(DbgPerf.Section != nullptr, "section should be set");
			TH_ASSERT_V(DbgPerf.Function != nullptr, "function should be set");

			if (DbgPerf.Ignore)
				return;

			uint64_t Time = DateTime().Microseconds();
			uint64_t Diff = Time - DbgPerf.Time;
			if (Diff > DbgPerf.Threshold)
			{
				TH_WARN("[perf] @%s takes %llu ms (%llu us)\n\tfunction: %s()\n\tfile: %s:%i\n\texpected: %llu ms at most", DbgPerf.Section, Diff / 1000, Diff, DbgPerf.Function, DbgPerf.File, DbgPerf.Line, DbgPerf.Threshold / 1000);
				DbgPerf.Time = Time;
			}
		}
		void Debug::TimeEnd()
		{
			TH_ASSERT_V(DbgPerf.File != nullptr, "file should be set");
			TH_ASSERT_V(DbgPerf.Section != nullptr, "section should be set");
			TH_ASSERT_V(DbgPerf.Function != nullptr, "function should be set");

			if (DbgPerf.Ignore)
				return;

			uint64_t Diff = DateTime().Microseconds() - DbgPerf.Time;
			if (Diff > DbgPerf.Threshold)
				TH_WARN("[perf] @%s took %llu ms (%llu us)\n\tfunction: %s()\n\tfile: %s:%i\n\texpected: %llu ms at most", DbgPerf.Section, Diff / 1000, Diff, DbgPerf.Function, DbgPerf.File, DbgPerf.Line, DbgPerf.Threshold / 1000);
		}
		void Debug::Log(int Level, int Line, const char* Source, const char* Format, ...)
		{
			if (!Format || (!Enabled && !Callback))
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
#ifndef _DEBUG
			if (Level == 1)
			{
				int ErrorCode = OS::Error::Get();
#ifdef TH_MICROSOFT
				if (ErrorCode != ERROR_SUCCESS)
					snprintf(Buffer, sizeof(Buffer), "%s [err] %s\n\tsystem: %s\n", Date, Format, OS::Error::GetName(ErrorCode).c_str());
				else
					snprintf(Buffer, sizeof(Buffer), "%s [err] %s\n", Date, Format);
#else
				if (ErrorCode > 0)
					snprintf(Buffer, sizeof(Buffer), "%s [err] %s\n\tsystem: %s\n", Date, Format, OS::Error::GetName(ErrorCode).c_str());
				else
					snprintf(Buffer, sizeof(Buffer), "%s [err] %s\n", Date, Format);
#endif
			}
			else if (Level == 2)
				snprintf(Buffer, sizeof(Buffer), "%s [warn] %s\n", Date, Format);
			else if (Level == 4)
				snprintf(Buffer, sizeof(Buffer), "%s [sys] %s\n", Date, Format);
			else
				snprintf(Buffer, sizeof(Buffer), "%s [info] %s\n", Date, Format);
#else
			if (Level == 1)
			{
				int ErrorCode = OS::Error::Get();
#ifdef TH_MICROSOFT
				if (ErrorCode != ERROR_SUCCESS)
					snprintf(Buffer, sizeof(Buffer), "%s %s:%d [err] %s\n\tsystem: %s\n", Date, Source ? Source : "log", Line, Format, OS::Error::GetName(ErrorCode).c_str());
				else
					snprintf(Buffer, sizeof(Buffer), "%s %s:%d [err] %s\n", Date, Source ? Source : "log", Line, Format);
#else
				if (ErrorCode > 0)
					snprintf(Buffer, sizeof(Buffer), "%s %s:%d [err] %s\n\tsystem: %s\n", Date, Source ? Source : "log", Line, Format, OS::Error::GetName(ErrorCode).c_str());
				else
					snprintf(Buffer, sizeof(Buffer), "%s %s:%d [err] %s\n", Date, Source ? Source : "log", Line, Format);
#endif
			}
			else if (Level == 2)
				snprintf(Buffer, sizeof(Buffer), "%s %s:%d [warn] %s\n", Date, Source ? Source : "log", Line, Format);
			else if (Level == 4)
				snprintf(Buffer, sizeof(Buffer), "%s %s:%d [sys] %s\n", Date, Source ? Source : "log", Line, Format);
			else
				snprintf(Buffer, sizeof(Buffer), "%s %s:%d [info] %s\n", Date, Source ? Source : "log", Line, Format);
#endif
			char Storage[8192];
			va_list Args;
			va_start(Args, Format);
			vsnprintf(Storage, sizeof(Storage), Buffer, Args);
			va_end(Args);

			if (Callback && !DbgPerf.Ignore)
			{
				DbgPerf.Ignore = true;
				Callback(Storage, Level);
				DbgPerf.Ignore = false;
			}

			if (Enabled)
			{
#if defined(TH_MICROSOFT) && defined(_DEBUG)
				OutputDebugStringA(Storage);
#endif
				if (Console::IsPresent() && (Level == 1 || Level == 2 || Level == 4))
				{
					Console* Dbg = Console::Get();
					if (Level == 1)
						Dbg->ColorBegin(StdColor::DarkRed, StdColor::Black);
					else if (Level == 2)
						Dbg->ColorBegin(StdColor::Yellow, StdColor::Black);
					else if (Level == 4)
						Dbg->ColorBegin(StdColor::Gray, StdColor::Black);
					Dbg->WriteBuffer(Storage);
					Dbg->ColorEnd();
				}
				else
					printf("%s", Storage);
			}
		}
		void Debug::Assert(bool Fatal, int Line, const char* Source, const char* Function, const char* Condition, const char* Format, ...)
		{
			if (!Function || !Condition || (!Enabled && !Callback))
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
#ifndef _DEBUG
			snprintf(Buffer, sizeof(Buffer), "%s [err] %s%s():\n\tassertion: %s\n\texception: %s\n", Date, Fatal ? "[fatal] " : "", Function, Condition, Format ? Format : "none");
#else
			snprintf(Buffer, sizeof(Buffer), "%s %s:%d [err] %s%s():\n\tassertion: %s\n\texception: %s\n", Date, Source ? Source : "log", Line, Fatal ? "[fatal] " : "", Function, Condition, Format ? Format : "none");
#endif
			char Storage[8192];
			if (Format != nullptr)
			{
				va_list Args;
				va_start(Args, Format);
				vsnprintf(Storage, sizeof(Storage), Buffer, Args);
				va_end(Args);
			}
			else
				memcpy(Storage, Buffer, sizeof(Buffer));

			if (Callback && !DbgPerf.Ignore)
			{
				DbgPerf.Ignore = true;
				Callback(Storage, 1);
				DbgPerf.Ignore = false;
			}

			if (Enabled)
			{
#if defined(TH_MICROSOFT) && defined(_DEBUG)
				OutputDebugStringA(Storage);
#endif
				if (Console::IsPresent())
				{
					Console* Dbg = Console::Get();
					Dbg->ColorBegin(StdColor::DarkRed, StdColor::Black);
					Dbg->WriteBuffer(Storage);
					Dbg->ColorEnd();
				}
				else
					printf("%s", Storage);
			}

			if (Fatal)
				Pause();
		}
		void Debug::Pause()
		{
			OS::Process::Interrupt();
		}
		std::vector<Debug::Context> Debug::Contexts;
		std::function<void(const char*, int)> Debug::Callback;
		std::mutex Debug::Safe;
		bool Debug::Enabled = false;

		void Composer::AddRef(Object* Value)
		{
			TH_ASSERT_V(Value != nullptr, "object should be set");
			Value->AddRef();
		}
		void Composer::SetFlag(Object* Value)
		{
			TH_ASSERT_V(Value != nullptr, "object should be set");
			Value->SetFlag();
		}
		bool Composer::GetFlag(Object* Value)
		{
			TH_ASSERT(Value != nullptr, false, "object should be set");
			return Value->GetFlag();
		}
		int Composer::GetRefCount(Object* Value)
		{
			TH_ASSERT(Value != nullptr, 1, "object should be set");
			return Value->GetRefCount();
		}
		void Composer::Release(Object* Value)
		{
			TH_ASSERT_V(Value != nullptr, "object should be set");
			Value->Release();
		}
		bool Composer::Clear()
		{
			TH_ASSERT(Factory != nullptr, false, "composer should be initialized");

			delete Factory;
			Factory = nullptr;
			return true;
		}
		bool Composer::Pop(const std::string& Hash)
		{
			TH_ASSERT(Factory != nullptr, false, "composer should be initialized");

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

		Console::Console() : Coloring(true), Handle(false), Time(0)
#ifdef TH_MICROSOFT
			, Conin(nullptr), Conout(nullptr), Conerr(nullptr), Attributes(0)
#endif
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
			TH_ASSERT_V(Handle, "console should be shown at least once to be hidden");
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

			Conin = freopen("conin$", "r", stdin);
			Conout = freopen("conout$", "w", stdout);
			Conerr = freopen("conout$", "w", stderr);
			SetConsoleCtrlHandler(ConsoleEventHandler, true);

			CONSOLE_SCREEN_BUFFER_INFO ScreenBuffer;
			if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ScreenBuffer))
				Attributes = ScreenBuffer.wAttributes;
#else
			if (Handle)
				return;
#endif
			Handle = true;
		}
		void Console::Clear()
		{
#ifdef TH_MICROSOFT
			TH_ASSERT_V(Handle, "console should be shown at least once");
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
			TH_ASSERT_V(Handle, "console should be shown at least once");
			std::cout.flush();
		}
		void Console::FlushWrite()
		{
			TH_ASSERT_V(Handle, "console should be shown at least once");
			std::cout << std::flush;
		}
		void Console::CaptureTime()
		{
			Time = (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000.0;
		}
		void Console::SetColoring(bool Enabled)
		{
			Coloring = Enabled;
		}
		void Console::ColorBegin(StdColor Text, StdColor Background)
		{
			TH_ASSERT_V(Handle, "console should be shown at least once");
			if (!Coloring)
				return;
#if defined(_WIN32)
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (int)Background << 4 | (int)Text);
#else
			std::cout << "\033[" << GetColorId(Text, false) << ";" << GetColorId(Background, true) << "m";
#endif
		}
		void Console::ColorEnd()
		{
			TH_ASSERT_V(Handle, "console should be shown at least once");
			if (!Coloring)
				return;
#if defined(_WIN32)
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), Attributes);
#else
			std::cout << "\033[0m";
#endif
		}
		void Console::WriteBuffer(const char* Buffer)
		{
			TH_ASSERT_V(Handle, "console should be shown at least once");
			TH_ASSERT_V(Buffer != nullptr, "buffer should be set");
			std::cout << Buffer;
		}
		void Console::WriteLine(const std::string& Line)
		{
			TH_ASSERT_V(Handle, "console should be shown at least once");
			std::cout << Line << '\n';
		}
		void Console::Write(const std::string& Line)
		{
			TH_ASSERT_V(Handle, "console should be shown at least once");
			std::cout << Line;
		}
		void Console::fWriteLine(const char* Format, ...)
		{
			TH_ASSERT_V(Handle, "console should be shown at least once");
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
			TH_ASSERT_V(Handle, "console should be shown at least once");
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
			TH_ASSERT_V(Handle, "console should be shown at least once");
			Lock.lock();
			std::cout << Line << '\n';
			Lock.unlock();
		}
		void Console::sWrite(const std::string& Line)
		{
			TH_ASSERT_V(Handle, "console should be shown at least once");
			Lock.lock();
			std::cout << Line;
			Lock.unlock();
		}
		void Console::sfWriteLine(const char* Format, ...)
		{
			TH_ASSERT_V(Handle, "console should be shown at least once");
			TH_ASSERT_V(Format != nullptr, "format should be set");

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
			TH_ASSERT_V(Handle, "console should be shown at least once");
			TH_ASSERT_V(Format != nullptr, "format should be set");

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
			TH_ASSERT_V(Format != nullptr, "format should be set");

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
			TH_ASSERT(Handle, std::string(), "console should be shown at least once");
			TH_ASSERT(Size > 0, std::string(), "read length should be greater than zero");

			char* Value = (char*)TH_MALLOC(sizeof(char) * (size_t)(Size + 1));
			memset(Value, 0, (size_t)Size * sizeof(char));
			Value[Size] = '\0';
#ifndef TH_MICROSOFT
            std::cout.flush();
#endif
			std::cin.getline(Value, Size);
			std::string Output = Value;
			TH_FREE(Value);

			return Output;
		}
		bool Console::IsPresent()
		{
			return Singleton != nullptr && Singleton->Handle;
		}
		bool Console::Reset()
		{
			if (!Singleton)
				return false;

			TH_RELEASE(Singleton);
			return true;
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

		Timer::Timer() : TimeIncrement(0.0), TickCounter(16), FrameCount(0.0), CapturedTime(0.0), FrameLimit(0)
		{
#ifdef TH_MICROSOFT
			Frequency = TH_NEW(LARGE_INTEGER);
			QueryPerformanceFrequency((LARGE_INTEGER*)Frequency);

			TimeLimit = TH_NEW(LARGE_INTEGER);
			QueryPerformanceCounter((LARGE_INTEGER*)TimeLimit);

			PastTime = TH_NEW(LARGE_INTEGER);
			QueryPerformanceCounter((LARGE_INTEGER*)PastTime);
#elif defined TH_UNIX
			Frequency = TH_NEW(timespec);
			clock_gettime(CLOCK_REALTIME, (timespec*)Frequency);

			TimeLimit = TH_NEW(timespec);
			clock_gettime(CLOCK_REALTIME, (timespec*)TimeLimit);

			PastTime = TH_NEW(timespec);
			clock_gettime(CLOCK_REALTIME, (timespec*)PastTime);
#endif
			SetStepLimitation(60.0f, 10.0f);
		}
		Timer::~Timer()
		{
#ifdef TH_MICROSOFT
			LARGE_INTEGER* sPastTime = (LARGE_INTEGER*)PastTime;
			TH_DELETE(LARGE_INTEGER, sPastTime);

			LARGE_INTEGER* sTimeLimit = (LARGE_INTEGER*)TimeLimit;
			TH_DELETE(LARGE_INTEGER, sTimeLimit);

			LARGE_INTEGER* sFrequency = (LARGE_INTEGER*)Frequency;
			TH_DELETE(LARGE_INTEGER, sFrequency);
#elif defined TH_UNIX
			timespec* sPastTime = (timespec*)PastTime;
			TH_DELETE(timespec, sPastTime);

			timespec* sTimeLimit = (timespec*)TimeLimit;
			TH_DELETE(timespec, sTimeLimit);

			timespec* sFrequency = (timespec*)Frequency;
			TH_DELETE(timespec, sFrequency);
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
			Seek(FileSeek::End, 0);
			uint64_t Size = Tell();
			Seek(FileSeek::Begin, Position);

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
			TH_ASSERT(File != nullptr, false, "filename should be set");

			Close();
			Path = OS::Path::Resolve(File);

			switch (Mode)
			{
				case FileMode::Read_Only:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "r");
					break;
				case FileMode::Write_Only:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "w");
					break;
				case FileMode::Append_Only:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "a");
					break;
				case FileMode::Read_Write:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "r+");
					break;
				case FileMode::Write_Read:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "w+");
					break;
				case FileMode::Read_Append_Write:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "a+");
					break;
				case FileMode::Binary_Read_Only:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "rb");
					break;
				case FileMode::Binary_Write_Only:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "wb");
					break;
				case FileMode::Binary_Append_Only:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "ab");
					break;
				case FileMode::Binary_Read_Write:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "rb+");
					break;
				case FileMode::Binary_Write_Read:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "wb+");
					break;
				case FileMode::Binary_Read_Append_Write:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "ab+");
					break;
			}

			return Resource != nullptr;
		}
		bool FileStream::Close()
		{
			if (Resource != nullptr)
			{
				TH_TRACE("close fs %i", (int)GetFd());
				fclose(Resource);
				Resource = nullptr;
			}

			return true;
		}
		bool FileStream::Seek(FileSeek Mode, int64_t Offset)
		{
			TH_ASSERT(Resource != nullptr, false, "file should be opened");
			TH_PSTART("file-stream-seek", TH_PERF_IO);
			switch (Mode)
			{
				case FileSeek::Begin:
					TH_PRET(fseek(Resource, (long)Offset, SEEK_SET) == 0);
				case FileSeek::Current:
					TH_PRET(fseek(Resource, (long)Offset, SEEK_CUR) == 0);
				case FileSeek::End:
					TH_PRET(fseek(Resource, (long)Offset, SEEK_END) == 0);
			}

			TH_PEND();
			return false;
		}
		bool FileStream::Move(int64_t Offset)
		{
			TH_ASSERT(Resource != nullptr, false, "file should be opened");
			TH_PSTART("file-stream-move", TH_PERF_IO);
			TH_PRET(fseek(Resource, (long)Offset, SEEK_CUR) == 0);
		}
		int FileStream::Flush()
		{
			TH_ASSERT(Resource != nullptr, 0, "file should be opened");
			TH_PSTART("file-stream-flush", TH_PERF_IO);
			TH_PRET(fflush(Resource));
		}
		uint64_t FileStream::ReadAny(const char* Format, ...)
		{
			TH_ASSERT(Resource != nullptr, 0, "file should be opened");
			TH_ASSERT(Format != nullptr, false, "format should be set");
			TH_PSTART("file-stream-rscan", TH_PERF_IO);

			va_list Args;
			va_start(Args, Format);
			uint64_t R = (uint64_t)vfscanf(Resource, Format, Args);
			va_end(Args);

			TH_PEND();
			return R;
		}
		uint64_t FileStream::Read(char* Data, uint64_t Length)
		{
			TH_ASSERT(Resource != nullptr, 0, "file should be opened");
			TH_ASSERT(Data != nullptr, false, "data should be set");
			TH_PSTART("file-stream-read", TH_PERF_IO);
			TH_PRET(fread(Data, 1, (size_t)Length, Resource));
		}
		uint64_t FileStream::WriteAny(const char* Format, ...)
		{
			TH_ASSERT(Resource != nullptr, 0, "file should be opened");
			TH_ASSERT(Format != nullptr, false, "format should be set");
			TH_PSTART("file-stream-wscan", TH_PERF_IO);

			va_list Args;
			va_start(Args, Format);
			uint64_t R = (uint64_t)vfprintf(Resource, Format, Args);
			va_end(Args);

			TH_PEND();
			return R;
		}
		uint64_t FileStream::Write(const char* Data, uint64_t Length)
		{
			TH_ASSERT(Resource != nullptr, 0, "file should be opened");
			TH_ASSERT(Data != nullptr, false, "data should be set");
			TH_PSTART("file-stream-read", TH_PERF_IO);
			TH_PRET(fwrite(Data, 1, (size_t)Length, Resource));
		}
		uint64_t FileStream::Tell()
		{
			TH_ASSERT(Resource != nullptr, 0, "file should be opened");
			TH_PSTART("file-stream-tell", TH_PERF_IO);
			TH_PRET(ftell(Resource));
		}
		int FileStream::GetFd()
		{
			TH_ASSERT(Resource != nullptr, -1, "file should be opened");
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
				Open(Path.c_str(), FileMode::Binary_Write_Only);
			}
		}
		bool GzStream::Open(const char* File, FileMode Mode)
		{
			TH_ASSERT(File != nullptr, 0, "filename should be set");
#ifdef TH_HAS_ZLIB
			Close();
			Path = OS::Path::Resolve(File);
			switch (Mode)
			{
				case FileMode::Binary_Read_Only:
				case FileMode::Read_Only:
					Resource = gzopen(Path.c_str(), "rb");
					TH_TRACE("[gz] open rb %s", Path.c_str());
					break;
				case FileMode::Binary_Write_Only:
				case FileMode::Write_Only:
					Resource = gzopen(Path.c_str(), "wb");
					TH_TRACE("[gz] open wb %s", Path.c_str());
					break;
				case FileMode::Read_Write:
				case FileMode::Write_Read:
				case FileMode::Append_Only:
				case FileMode::Read_Append_Write:
				case FileMode::Binary_Append_Only:
				case FileMode::Binary_Read_Write:
				case FileMode::Binary_Write_Read:
				case FileMode::Binary_Read_Append_Write:
					TH_ERR("[gz] compressed stream supports only rb and wb modes");
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
			TH_PSTART("gz-stream-close", TH_PERF_IO);
			if (Resource != nullptr)
			{
				TH_TRACE("[gz] close 0x%p", (void*)Resource);
				gzclose((gzFile)Resource);
				Resource = nullptr;
			}
			TH_PEND();
#endif
			return true;
		}
		bool GzStream::Seek(FileSeek Mode, int64_t Offset)
		{
			TH_ASSERT(Resource != nullptr, 0, "file should be opened");
#ifdef TH_HAS_ZLIB
			TH_PSTART("gz-stream-seek", TH_PERF_IO);
			switch (Mode)
			{
				case FileSeek::Begin:
					TH_PRET(gzseek((gzFile)Resource, (long)Offset, SEEK_SET) == 0);
				case FileSeek::Current:
					TH_PRET(gzseek((gzFile)Resource, (long)Offset, SEEK_CUR) == 0);
				case FileSeek::End:
					TH_PRET(gzseek((gzFile)Resource, (long)Offset, SEEK_END) == 0);
			}
			TH_PEND();
#endif
			return false;
		}
		bool GzStream::Move(int64_t Offset)
		{
			TH_ASSERT(Resource != nullptr, 0, "file should be opened");
#ifdef TH_HAS_ZLIB
			TH_PSTART("gz-stream-move", TH_PERF_IO);
			TH_PRET(gzseek((gzFile)Resource, (long)Offset, SEEK_CUR) == 0);
#else
			return false;
#endif
		}
		int GzStream::Flush()
		{
			TH_ASSERT(Resource != nullptr, 0, "file should be opened");
#ifdef TH_HAS_ZLIB
			TH_PSTART("gz-stream-flush", TH_PERF_IO);
			TH_PRET(gzflush((gzFile)Resource, Z_SYNC_FLUSH));
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
			TH_ASSERT(Resource != nullptr, 0, "file should be opened");
			TH_ASSERT(Data != nullptr, 0, "data should be set");
#ifdef TH_HAS_ZLIB
			TH_PSTART("gz-stream-read", TH_PERF_IO);
			TH_PRET(gzread((gzFile)Resource, Data, Length));
#else
			return 0;
#endif
		}
		uint64_t GzStream::WriteAny(const char* Format, ...)
		{
			TH_ASSERT(Resource != nullptr, 0, "file should be opened");
			TH_ASSERT(Format != nullptr, 0, "format should be set");
			TH_PSTART("gz-stream-wscan", TH_PERF_IO);

			va_list Args;
			va_start(Args, Format);
#ifdef TH_HAS_ZLIB
			uint64_t R = (uint64_t)gzvprintf((gzFile)Resource, Format, Args);
#else
			uint64_t R = 0;
#endif
			va_end(Args);

			TH_PEND();
			return R;
		}
		uint64_t GzStream::Write(const char* Data, uint64_t Length)
		{
			TH_ASSERT(Resource != nullptr, 0, "file should be opened");
			TH_ASSERT(Data != nullptr, 0, "data should be set");
#ifdef TH_HAS_ZLIB
			TH_PSTART("gz-stream-write", TH_PERF_IO);
			TH_PRET(gzwrite((gzFile)Resource, Data, Length));
#else
			return 0;
#endif
		}
		uint64_t GzStream::Tell()
		{
			TH_ASSERT(Resource != nullptr, 0, "file should be opened");
#ifdef TH_HAS_ZLIB
			TH_PSTART("gz-stream-tell", TH_PERF_IO);
			TH_PRET(gztell((gzFile)Resource));
#else
			return 0;
#endif
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
			TH_ASSERT(File != nullptr, 0, "filename should be set");
			Close();

			Network::SourceURL URL(File);
			if (URL.Protocol != "http" && URL.Protocol != "https")
				return false;

			Network::Host Address;
			Address.Hostname = URL.Host;
			Address.Secure = (URL.Protocol == "https");
			Address.Port = (URL.Port < 0 ? (Address.Secure ? 443 : 80) : URL.Port);

			switch (Mode)
			{
				case FileMode::Binary_Read_Only:
				case FileMode::Read_Only:
				{
					auto* Client = new Network::HTTP::Client(30000);
					if (TH_AWAIT(Client->Connect(&Address, false)) < 0)
					{
						TH_RELEASE(Client);
						break;
					}
					
					Network::HTTP::RequestFrame Request;
					Request.URI.assign('/' + URL.Path);
					for (auto& Item : URL.Query)
						Request.Query += Item.first + "=" + Item.second;

					Network::HTTP::ResponseFrame* Response = TH_AWAIT(Client->Send(&Request));
					if (!Response || Response->StatusCode < 0)
					{
						TH_RELEASE(Client);
						break;
					}

					const char* ContentLength = Response->GetHeader("Content-Length");
					if (!ContentLength)
					{
						if (!TH_AWAIT(Client->Consume(1024 * 1024 * 16)) || !Network::HTTP::Util::ContentOK(Client->GetRequest()->ContentState))
						{
							TH_RELEASE(Client);
							break;
						}

						Buffer.assign(Response->Buffer.begin(), Response->Buffer.end());
						Size = Response->Buffer.size();
						Resource = Client;
					}
					else
					{
						Size = Parser(ContentLength).ToUInt64();
						Resource = Client;
					}

					TH_TRACE("[http] open ws %s", File);
					break;
				}
				case FileMode::Binary_Write_Only:
				case FileMode::Write_Only:
				case FileMode::Read_Write:
				case FileMode::Write_Read:
				case FileMode::Append_Only:
				case FileMode::Read_Append_Write:
				case FileMode::Binary_Append_Only:
				case FileMode::Binary_Read_Write:
				case FileMode::Binary_Write_Read:
				case FileMode::Binary_Read_Append_Write:
					TH_TRACE("[http] web stream supports only rb and r modes");
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
				case FileSeek::Begin:
					Offset = NewOffset;
					return true;
				case FileSeek::Current:
					Offset += NewOffset;
					return true;
				case FileSeek::End:
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
			TH_ASSERT(Resource != nullptr, 0, "file should be opened");
			TH_ASSERT(Data != nullptr, 0, "data should be set");
			TH_ASSERT(Length > 0, 0, "length should be greater than zero");

			uint64_t Result = 0;
			if (!Buffer.empty())
			{
				Result = std::min(Length, (uint64_t)Buffer.size() - Offset);
				memcpy(Data, Buffer.c_str() + Offset, Result);
				Offset += Result;

				return Result;
			}

			auto* Client = (Network::HTTP::Client*)Resource;
			auto* Response = TH_AWAIT(Client->Consume(Length));
			Result = std::min(Length, (uint64_t)Response->Buffer.size());
			memcpy(Data, Response->Buffer.data(), Result);
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
			TH_ASSERT(Resource != nullptr, -1, "file should be opened");
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
						Files.emplace_back(OS::Path::Resolve(Entry->Path.c_str()));

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
			TH_ASSERT_V(Callback, "callback should not be empty");
			if (!Callback(this))
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
			TH_ASSERT_V(Path != nullptr, "path should be set");
#ifdef TH_MICROSOFT
			if (!SetCurrentDirectoryA(Path))
				TH_ERR("[dir] couldn't set current directory");
#elif defined(TH_UNIX)
			if (chdir(Path) != 0)
				TH_ERR("[dir] couldn't set current directory");
#endif
		}
		void OS::Directory::Patch(const std::string& Path)
		{
			if (!IsExists(Path.c_str()))
				Create(Path.c_str());
		}
		bool OS::Directory::Scan(const std::string& Path, std::vector<ResourceEntry>* Entries)
		{
			TH_ASSERT(Entries != nullptr, false, "entries should be set");
			TH_PSTART("os-dir-scan", TH_PERF_IO);

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
				TH_PEND();
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
#else
			DIR* Value = opendir(Path.c_str());
			if (!Value)
			{
				TH_PEND();
				return false;
			}

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
			TH_PEND();
			return true;
		}
		bool OS::Directory::Each(const char* Path, const std::function<bool(DirectoryEntry*)>& Callback)
		{
			TH_ASSERT(Path != nullptr, false, "path should be set");
			std::vector<ResourceEntry> Entries;
			std::string Result = Path::Resolve(Path);
			Scan(Result, &Entries);

			Parser R(&Result);
			if (!R.EndsWith('/') && !R.EndsWith('\\'))
				Result += '/';

			for (auto& Dir : Entries)
			{
				DirectoryEntry Entry;
				Entry.Path = Result + Dir.Path;
				Entry.IsGood = true;
				Entry.IsDirectory = Dir.Source.IsDirectory;
				if (!Entry.IsDirectory)
					Entry.Length = Dir.Source.Size;

				if (!Callback(&Entry))
					break;
			}

			return true;
		}
		bool OS::Directory::Create(const char* Path)
		{
			TH_ASSERT(Path != nullptr, false, "path should be set");
			TH_PSTART("os-dir-mk", TH_PERF_IO);
			TH_TRACE("[io] create dir %s", Path);
#ifdef TH_MICROSOFT
			wchar_t Buffer[1024];
			UnicodePath(Path, Buffer, 1024);
			size_t Length = wcslen(Buffer);
			if (!Length)
			{
				TH_PEND();
				return false;
			}

			if (::CreateDirectoryW(Buffer, nullptr) != FALSE || GetLastError() == ERROR_ALREADY_EXISTS)
			{
				TH_PEND();
				return true;
			}

			size_t Index = Length - 1;
			while (Index > 0 && Buffer[Index] != '/' && Buffer[Index] != '\\')
				Index--;

			if (Index > 0 && !Create(std::string(Path).substr(0, Index).c_str()))
			{
				TH_PEND();
				return false;
			}

			TH_PRET(::CreateDirectoryW(Buffer, nullptr) != FALSE || GetLastError() == ERROR_ALREADY_EXISTS);
#else
			if (mkdir(Path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1 || errno == EEXIST)
			{
				TH_PEND();
				return true;
			}

			size_t Index = strlen(Path) - 1;
			while (Index > 0 && Path[Index] != '/' && Path[Index] != '\\')
				Index--;

			if (Index > 0 && !Create(std::string(Path).substr(0, Index).c_str()))
			{
				TH_PEND();
				return false;
			}

			TH_PRET(mkdir(Path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1 || errno == EEXIST);
#endif
		}
		bool OS::Directory::Remove(const char* Path)
		{
			TH_ASSERT(Path != nullptr, false, "path should be set");
			TH_PSTART("os-dir-rm", TH_PERF_IO);
			TH_TRACE("[io] remove dir %s", Path);

#ifdef TH_MICROSOFT
			WIN32_FIND_DATA FileInformation;
			std::string FilePath, Pattern = std::string(Path) + "\\*.*";
			HANDLE Handle = ::FindFirstFile(Pattern.c_str(), &FileInformation);

			if (Handle == INVALID_HANDLE_VALUE)
			{
				::FindClose(Handle);
				TH_PEND();
				return false;
			}

			do
			{
				if (FileInformation.cFileName[0] == '.')
					continue;

				FilePath = std::string(Path) + "\\" + FileInformation.cFileName;
				if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					::FindClose(Handle);
					TH_PRET(Remove(FilePath.c_str()));
				}

				if (::SetFileAttributes(FilePath.c_str(), FILE_ATTRIBUTE_NORMAL) == FALSE)
				{
					::FindClose(Handle);
					TH_PEND();
					return false;
				}

				if (::DeleteFile(FilePath.c_str()) == FALSE)
				{
					::FindClose(Handle);
					TH_PEND();
					return false;
				}
			} while (::FindNextFile(Handle, &FileInformation) != FALSE);
			::FindClose(Handle);

			if (::GetLastError() != ERROR_NO_MORE_FILES)
			{
				TH_PEND();
				return false;
			}

			if (::SetFileAttributes(Path, FILE_ATTRIBUTE_NORMAL) == FALSE)
			{
				TH_PEND();
				return false;
			}

			TH_PRET(::RemoveDirectory(Path) != FALSE);
#elif defined TH_UNIX
			DIR* Root = opendir(Path);
			size_t Size = strlen(Path);

			if (!Root)
				TH_PRET(rmdir(Path) == 0);

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
			TH_PRET(rmdir(Path) == 0);
#endif
		}
		bool OS::Directory::IsExists(const char* Path)
		{
			TH_ASSERT(Path != nullptr, false, "path should be set");
			TH_PSTART("os-dir-exists", TH_PERF_IO);

			struct stat Buffer;
			if (stat(Path::Resolve(Path).c_str(), &Buffer) != 0)
			{
				TH_PEND();
				return false;
			}

			TH_PEND();
			return Buffer.st_mode & S_IFDIR;
		}
		std::string OS::Directory::Get()
		{
			TH_PSTART("os-dir-fetch", TH_PERF_IO);
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
			{
				TH_PEND();
				return std::string();
			}
#endif
			int64_t Length = strlen(Buffer);
			if (Length > 0 && Buffer[Length - 1] != '/' && Buffer[Length - 1] != '\\')
			{
				Buffer[Length] = '/';
				Length++;
			}

			TH_PEND();
			return std::string(Buffer, Length);
#else
			char* Base = SDL_GetBasePath();
			std::string Result = Base;
			SDL_free(Base);

			TH_PEND();
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
			TH_ASSERT(Resource != nullptr, false, "resource should be set");
			TH_PSTART("os-file-stat", TH_PERF_IO);

			memset(Resource, 0, sizeof(*Resource));
#if defined(TH_MICROSOFT)
			wchar_t WBuffer[1024];
			UnicodePath(Path.c_str(), WBuffer, sizeof(WBuffer) / sizeof(WBuffer[0]));

			WIN32_FILE_ATTRIBUTE_DATA Info;
			if (GetFileAttributesExW(WBuffer, GetFileExInfoStandard, &Info) == 0)
			{
				TH_PEND();
				return false;
			}

			Resource->Size = MAKEUQUAD(Info.nFileSizeLow, Info.nFileSizeHigh);
			Resource->LastModified = SYS2UNIX_TIME(Info.ftLastWriteTime.dwLowDateTime, Info.ftLastWriteTime.dwHighDateTime);
			Resource->CreationTime = SYS2UNIX_TIME(Info.ftCreationTime.dwLowDateTime, Info.ftCreationTime.dwHighDateTime);
			if (Resource->CreationTime > Resource->LastModified)
				Resource->LastModified = Resource->CreationTime;

			Resource->IsDirectory = Info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
			if (Resource->IsDirectory)
			{
				TH_PEND();
				return true;
			}

			if (Path.empty())
			{
				TH_PEND();
				return false;
			}

			int End = Path.back();
			if (isalnum(End) || strchr("_-", End) != nullptr)
			{
				TH_PEND();
				return true;
			}

			TH_PEND();
			memset(Resource, 0, sizeof(*Resource));
			return false;
#else
			struct stat State;
			if (stat(Path.c_str(), &State) != 0)
			{
				TH_PEND();
				return false;
			}

			struct tm Time;
			LocalTime(&State.st_ctime, &Time);
			Resource->CreationTime = mktime(&Time);
			Resource->Size = (uint64_t)(State.st_size);
			Resource->LastModified = State.st_mtime;
			Resource->IsDirectory = S_ISDIR(State.st_mode);

			TH_PEND();
			return true;
#endif
		}
		bool OS::File::Write(const char* Path, const char* Data, uint64_t Length)
		{
			TH_ASSERT(Path != nullptr, false, "path should be set");
			TH_ASSERT(Data != nullptr, false, "data should be set");

			FILE* Stream = (FILE*)Open(Path, "wb");
			if (!Stream)
				return false;

			TH_PSTART("os-file-cwbuf", TH_PERF_IO);
			fwrite((const void*)Data, (size_t)Length, 1, Stream);
			fclose(Stream);
			TH_PEND();

			return true;
		}
		bool OS::File::Write(const char* Path, const std::string& Data)
		{
			TH_ASSERT(Path != nullptr, false, "path should be set");
			FILE* Stream = (FILE*)Open(Path, "wb");
			if (!Stream)
				return false;

			TH_PSTART("os-file-cwbuf", TH_PERF_IO);
			fwrite((const void*)Data.c_str(), (size_t)Data.size(), 1, Stream);
			fclose(Stream);
			TH_PEND();

			return true;
		}
		bool OS::File::Move(const char* From, const char* To)
		{
			TH_ASSERT(From != nullptr && To != nullptr, false, "from and to should be set");
			TH_PSTART("os-file-mv", TH_PERF_IO);
			TH_TRACE("[io] move file from %s to %s", From, To);
#ifdef TH_MICROSOFT
			TH_PRET(MoveFileA(From, To) != 0);
#elif defined TH_UNIX
			TH_PRET(!rename(From, To));
#else
			TH_PEND();
			return false;
#endif
		}
		bool OS::File::Remove(const char* Path)
		{
			TH_ASSERT(Path != nullptr, false, "path should be set");
			TH_PSTART("os-file-rm", TH_PERF_IO);
			TH_TRACE("[io] remove file %s", Path);
#ifdef TH_MICROSOFT
			SetFileAttributesA(Path, 0);
			TH_PRET(DeleteFileA(Path) != 0);
#elif defined TH_UNIX
			TH_PRET(unlink(Path) == 0);
#else
			TH_PEND();
			return false;
#endif
		}
		bool OS::File::IsExists(const char* Path)
		{
			TH_ASSERT(Path != nullptr, false, "path should be set");
			TH_PSTART("os-file-exists", TH_PERF_IO);

			struct stat Buffer;
			TH_PRET(stat(Path::Resolve(Path).c_str(), &Buffer) == 0);
		}
		uint64_t OS::File::GetCheckSum(const std::string& Data)
		{
			return Compute::Common::CRC32(Data);
		}
		FileState OS::File::GetState(const char* Path)
		{
			FileState State;
			struct stat Buffer;

			TH_ASSERT(Path != nullptr, State, "path should be set");
			TH_PSTART("os-file-state", TH_PERF_IO);

			if (stat(Path, &Buffer) != 0)
			{
				TH_PEND();
				return State;
			}

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

			TH_PEND();
			return State;
		}
		void* OS::File::Open(const char* Path, const char* Mode)
		{
			TH_PSTART("os-file-open", TH_PERF_IO);
			TH_ASSERT(Path != nullptr && Mode != nullptr, false, "path and mode should be set");
#ifdef TH_MICROSOFT
			wchar_t WBuffer[1024], WMode[20];
			UnicodePath(Path, WBuffer, sizeof(WBuffer) / sizeof(WBuffer[0]));
			MultiByteToWideChar(CP_UTF8, 0, Mode, -1, WMode, sizeof(WMode) / sizeof(WMode[0]));

			FILE* Stream = _wfopen(WBuffer, WMode);
			TH_TRACE("[io] open fs %s %s", Mode, Path);
			TH_PRET((void*)Stream);
#else
			FILE* Stream = fopen(Path, Mode);
			if (Stream != nullptr)
				fcntl(fileno(Stream), F_SETFD, FD_CLOEXEC);
			TH_TRACE("[io] open fs %s %s", Mode, Path);
			TH_PRET((void*)Stream);
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
			TH_ASSERT(Path != nullptr, false, "path should be set");
			FILE* Stream = (FILE*)Open(Path, "rb");
			if (!Stream)
				return nullptr;

			TH_PSTART("os-file-read-all", TH_PERF_IO);
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

				TH_PEND();
				return nullptr;
			}

			Bytes[Size] = '\0';
			if (Length != nullptr)
				*Length = Size;

			fclose(Stream);
			TH_PEND();

			return Bytes;
		}
		unsigned char* OS::File::ReadAll(Stream* Stream, uint64_t* Length)
		{
			TH_ASSERT(Stream != nullptr, false, "stream should be set");
			uint64_t Size = Stream->GetSize();
			auto* Bytes = (unsigned char*)TH_MALLOC(sizeof(unsigned char) * (size_t)(Size + 1));
			Stream->Read((char*)Bytes, Size * sizeof(unsigned char));
			Bytes[Size] = '\0';

			if (Length != nullptr)
				*Length = Size;

			return Bytes;
		}
		unsigned char* OS::File::ReadChunk(Stream* Stream, uint64_t Length)
		{
			TH_ASSERT(Stream != nullptr, false, "stream should be set");
			auto* Bytes = (unsigned char*)TH_MALLOC((size_t)(Length + 1));
			Stream->Read((char*)Bytes, Length);
			Bytes[Length] = '\0';

			return Bytes;
		}
		std::string OS::File::ReadAsString(const char* Path)
		{
			TH_ASSERT(Path != nullptr, false, "path should be set");
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
			TH_ASSERT(Path != nullptr, std::vector<std::string>(), "path should be set");
			FILE* Stream = (FILE*)Open(Path, "rb");
			if (!Stream)
				return std::vector<std::string>();

			fseek(Stream, 0, SEEK_END);
			uint64_t Length = ftell(Stream);
			fseek(Stream, 0, SEEK_SET);

			char* Buffer = (char*)TH_MALLOC(sizeof(char) * Length);
			if (!Buffer)
			{
				fclose(Stream);
				return std::vector<std::string>();
			}

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
			TH_ASSERT(Path != nullptr, false, "path should be set");
			return Network::SourceURL(Path).Protocol != "file";
		}
		std::string OS::Path::Resolve(const char* Path)
		{
			TH_ASSERT(Path != nullptr, false, "path should be set");
			TH_PSTART("os-path-resolve", TH_PERF_IO);
#ifdef TH_MICROSOFT
			char Buffer[2048] = { 0 };
			if (GetFullPathNameA(Path, sizeof(Buffer), Buffer, nullptr) == 0)
			{
				TH_PEND();
				return Path;
			}
#elif defined TH_UNIX
			char* Data = realpath(Path, nullptr);
			if (!Data)
			{
				TH_PEND();
				return Path;
			}

			std::string Buffer = Data;
			TH_FREE(Data);
#endif
			TH_PEND();
			return Buffer;
		}
		std::string OS::Path::Resolve(const std::string& Path, const std::string& Directory)
		{
			TH_ASSERT(!Path.empty() && !Directory.empty(), "", "path and directory should not be empty");
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
		std::string OS::Path::ResolveResource(const std::string& Path)
		{
			if (Path.empty() || OS::File::IsExists(Path.c_str()))
				return Path;

			std::string fPath = Resolve(Path.c_str());
			if (!fPath.empty() && OS::File::IsExists(fPath.c_str()))
				return fPath;

			fPath.clear();
			return fPath;
		}
		std::string OS::Path::ResolveResource(const std::string& Path, const std::string& Directory)
		{
			if (Path.empty() || OS::File::IsExists(Path.c_str()))
				return Path;

			std::string fPath = Resolve(Path.c_str());
			if (!fPath.empty() && OS::File::IsExists(fPath.c_str()))
				return fPath;

			if (!Directory.empty())
			{
				fPath = Resolve(Path.c_str(), Directory);
				if (!fPath.empty() && OS::File::IsExists(fPath.c_str()))
					return fPath;
			}

			fPath.clear();
			return fPath;
		}
		std::string OS::Path::GetDirectory(const char* Path, uint32_t Level)
		{
			TH_ASSERT(Path != nullptr, false, "path should be set");

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
			TH_ASSERT(Path != nullptr, nullptr, "path should be set");
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
			TH_ASSERT(Path != nullptr, nullptr, "path should be set");
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
			TH_ASSERT(Stream != nullptr, false, "stream should be set");
			TH_ASSERT(Size > 0, false, "size should be greater than zero");
			TH_PSTART("os-net-send", TH_PERF_NET);
#ifdef TH_MICROSOFT
			TH_PRET(TransmitFile((SOCKET)Socket, (HANDLE)_get_osfhandle(_fileno(Stream)), (DWORD)Size, 16384, nullptr, nullptr, 0) > 0);
#elif defined(TH_APPLE)
			TH_PRET(sendfile(fileno(Stream), Socket, 0, (off_t*)&Size, nullptr, 0));
#elif defined(TH_UNIX)
			TH_PRET(sendfile(Socket, fileno(Stream), nullptr, (size_t)Size) > 0);
#else
			TH_PEND();
			return false;
#endif
		}
		bool OS::Net::GetETag(char* Buffer, uint64_t Length, Resource* Resource)
		{
			TH_ASSERT(Resource != nullptr, false, "resource should be set");
			return GetETag(Buffer, Length, Resource->LastModified, Resource->Size);
		}
		bool OS::Net::GetETag(char* Buffer, uint64_t Length, int64_t LastModified, uint64_t ContentLength)
		{
			TH_ASSERT(Buffer != nullptr && Length > 0, false, "buffer should be set and size should be greater than zero");
			snprintf(Buffer, (const size_t)Length, "\"%lx.%llu\"", (unsigned long)LastModified, ContentLength);
			return true;
		}
		socket_t OS::Net::GetFd(FILE* Stream)
		{
			TH_ASSERT(Stream != nullptr, -1, "stream should be set");
#ifdef TH_MICROSOFT
			return _fileno(Stream);
#else
			return fileno(Stream);
#endif
		}

		void OS::Process::Interrupt()
		{
#ifndef NDEBUG
#ifndef TH_MICROSOFT
#ifndef SIGTRAP
			__debugbreak();
#else
			raise(SIGTRAP);
#endif
#else
			if (IsDebuggerPresent())
				__debugbreak();
#endif
			TH_TRACE("[dbg] process paused");
#endif
		}
		int OS::Process::Execute(const char* Format, ...)
		{
			TH_ASSERT(Format != nullptr, -1, "format should be set");

			char Buffer[8192];
			va_list Args;
			va_start(Args, Format);
#ifdef TH_MICROSOFT
			_vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#else
			vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#endif
			va_end(Args);
			
			TH_TRACE("[io] execute command\n\t%s", Buffer);
			return system(Buffer);
		}
		bool OS::Process::Spawn(const std::string& Path, const std::vector<std::string>& Params, ChildProcess* Child)
		{
#ifdef TH_MICROSOFT
			HANDLE Job = CreateJobObject(nullptr, nullptr);
			if (Job == nullptr)
			{
				TH_ERR("cannot create job object for process");
				return false;
			}

			JOBOBJECT_EXTENDED_LIMIT_INFORMATION Info = { 0 };
			Info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
			if (SetInformationJobObject(Job, JobObjectExtendedLimitInformation, &Info, sizeof(Info)) == 0)
			{
				TH_ERR("cannot set job object for process");
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
				TH_ERR("cannot spawn process %s", Exe.Get());
				return false;
			}

			TH_TRACE("[io] spawn process %i on %s", (int)GetProcessId(Process.hProcess), Path.c_str());
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
				TH_ERR("cannot spawn process %s (file does not exists)", Path.c_str());
				return false;
			}

			pid_t ProcessId = fork();
			if (ProcessId == 0)
			{
				std::vector<char*> Args;
				for (auto It = Params.begin(); It != Params.end(); ++It)
					Args.push_back((char*)It->c_str());
				Args.push_back(nullptr);

				execve(Path.c_str(), Args.data(), nullptr);
				exit(0);
			}
			else if (Child != nullptr)
			{
				TH_TRACE("[io] spawn process %i on %s", (int)ProcessId, Path.c_str());
				Child->Process = ProcessId;
				Child->Valid = (ProcessId > 0);
			}

			return (ProcessId > 0);
#endif
		}
		bool OS::Process::Await(ChildProcess* Process, int* ExitCode)
		{
			TH_ASSERT(Process != nullptr && Process->Valid, false, "process should be set and be valid");
#ifdef TH_MICROSOFT
			WaitForSingleObject(Process->Process, INFINITE);
			TH_TRACE("[io] close process %s", (int)GetProcessId(Process->Process));

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

			TH_TRACE("[io] close process %s", (int)Process->Process);
			if (ExitCode != nullptr)
				*ExitCode = WEXITSTATUS(Status);
#endif
			Free(Process);
			return true;
		}
		bool OS::Process::Free(ChildProcess* Child)
		{
			TH_ASSERT(Child != nullptr && Child->Valid, false, "child should be set and be valid");
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

			TH_TRACE("[dl] load dll library %s", Name.Get());
			return (void*)LoadLibrary(Name.Get());
#elif defined(TH_APPLE)
			if (Path.empty())
				return (void*)dlopen(nullptr, RTLD_LAZY);

			if (!Name.EndsWith(".dylib"))
				Name.Append(".dylib");

			TH_TRACE("[dl] load dylib library %s", Name.Get());
			return (void*)dlopen(Name.Get(), RTLD_LAZY);
#elif defined(TH_UNIX)
			if (Path.empty())
				return (void*)dlopen(nullptr, RTLD_LAZY);

			if (!Name.EndsWith(".so"))
				Name.Append(".so");

			TH_TRACE("[dl] load so library %s", Name.Get());
			return (void*)dlopen(Name.Get(), RTLD_LAZY);
#else
			return nullptr;
#endif
		}
		void* OS::Symbol::LoadFunction(void* Handle, const std::string& Name)
		{
			TH_ASSERT(Handle != nullptr && !Name.empty(), nullptr, "handle should be set and name should not be empty");
			TH_TRACE("[dl] load function %s", Name.c_str());
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
					TH_ERR("[dl] symload error: %s", Text.c_str());
			}

			return Result;
#elif defined(TH_UNIX)
			void* Result = (void*)dlsym(Handle, Name.c_str());
			if (!Result)
			{
				const char* Text = dlerror();
				if (Text != nullptr)
					TH_ERR("[dl] symload error: %s", Text);
			}

			return Result;
#else
			return nullptr;
#endif
		}
		bool OS::Symbol::Unload(void* Handle)
		{
			TH_ASSERT(Handle != nullptr, nullptr, "handle should be set");
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

			if (Result != nullptr)
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

		ChangeLog::ChangeLog(const std::string& Root) : Offset(-1), Time(0), Path(Root)
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
			TH_ASSERT_V(Callback, "callback should not be empty");	
			Resource State;
			if (Source->GetBuffer() && (!OS::File::State(Path, &State) || State.LastModified == Time))
				return;

			Source->Open(Path.c_str(), FileMode::Binary_Read_Only);
			Time = State.LastModified;

			uint64_t Length = Source->GetSize();
			if (Length <= Offset || Offset <= 0)
			{
				Offset = Length;
				return;
			}

			int64_t Delta = Length - Offset;
			Source->Seek(FileSeek::Begin, Length - Delta);

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

					if (!Callback(this, Line.c_str(), (int64_t)Line.size()))
					{
						Offset = Length;
						return;
					}
				}
			}
			else if (!Value.empty())
				Callback(this, Value.c_str(), (int64_t)Value.size());

			Offset = Length;
		}

		static thread_local Costate* Cothread = nullptr;
		Costate::Costate(size_t StackSize) : Waitable(nullptr), Thread(std::this_thread::get_id()), Current(nullptr), Switch(TH_NEW(Cocontext, true)), Size(StackSize)
		{
		}
		Costate::~Costate()
		{
			if (Cothread == this)
				Cothread = nullptr;

			Safe.lock();
			for (auto& Routine : Cached)
				TH_DELETE(Coroutine, Routine);

			for (auto& Routine : Used)
				TH_DELETE(Coroutine, Routine);

			TH_DELETE(Cocontext, Switch);
			Safe.unlock();
		}
		Coroutine* Costate::Pop(const TaskCallback& Procedure)
		{
			Coroutine* Routine = nullptr;
			Safe.lock();
			if (!Cached.empty())
			{
				Routine = *Cached.begin();
				Routine->Callback = Procedure;
                Routine->State = Coactive::Active;
				Cached.erase(Cached.begin());
			}
			else
				Routine = TH_NEW(Coroutine, this, Procedure);

			Used.emplace(Routine);
			Safe.unlock();

			return Routine;
		}
		Coroutine* Costate::Pop(TaskCallback&& Procedure)
		{
			Coroutine* Routine = nullptr;
			Safe.lock();
			if (!Cached.empty())
			{
				Routine = *Cached.begin();
				Routine->Callback = std::move(Procedure);
                Routine->State = Coactive::Active;
				Cached.erase(Cached.begin());
			}
			else
				Routine = TH_NEW(Coroutine, this, std::move(Procedure));

			Used.emplace(Routine);
			Safe.unlock();

			return Routine;
		}
		int Costate::Reuse(Coroutine* Routine, const TaskCallback& Procedure)
		{
			TH_ASSERT(Routine != nullptr, -1, "coroutine should be set");
			TH_ASSERT(Routine->Master == this, -1, "coroutine should be created by this costate");
			TH_ASSERT(Routine->Dead, -1, "coroutine should be dead");

			Routine->Callback = Procedure;
            Routine->Dead = false;
            Routine->State = Coactive::Active;
			return 1;
		}
		int Costate::Reuse(Coroutine* Routine, TaskCallback&& Procedure)
		{
			TH_ASSERT(Routine != nullptr, -1, "coroutine should be set");
			TH_ASSERT(Routine->Master == this, -1, "coroutine should be created by this costate");
			TH_ASSERT(Routine->Dead, -1, "coroutine should be dead");

			Routine->Callback = std::move(Procedure);
            Routine->Dead = false;
            Routine->State = Coactive::Active;
			return 1;
		}
		int Costate::Reuse(Coroutine* Routine)
		{
			TH_ASSERT(Routine != nullptr, -1, "coroutine should be set");
			TH_ASSERT(Routine->Master == this, -1, "coroutine should be created by this costate");
			TH_ASSERT(Routine->Dead, -1, "coroutine should be dead");

			Routine->Callback = nullptr;
            Routine->Dead = false;
            Routine->State = Coactive::Active;
            
			Safe.lock();
			Used.erase(Routine);
			Cached.emplace(Routine);
			Safe.unlock();

			return 1;
		}
		int Costate::Swap(Coroutine* Routine)
		{
			TH_ASSERT(Routine != nullptr, -1, "coroutine should be set");
			TH_ASSERT(!Routine->Dead, -1, "coroutine should not be dead");
			
			if (Routine->State == Coactive::Inactive)
				return 0;
			
			if (Routine->State == Coactive::Resumable)
                Routine->State = Coactive::Active;
            
            Cocontext* Fiber = Routine->Switch;
            Current = Routine;
#ifdef TH_MICROSOFT
			if (Fiber->Context == nullptr)
#else
			if (Fiber->Stack == nullptr)
#endif
			{
#ifndef TH_MICROSOFT
				getcontext(&Fiber->Context);
				Fiber->Stack = (char*)TH_MALLOC(sizeof(char) * Size);
				Fiber->Context.uc_stack.ss_sp = Fiber->Stack;
				Fiber->Context.uc_stack.ss_size = Size;
                Fiber->Context.uc_stack.ss_flags = 0;
				Fiber->Context.uc_link = &Switch->Context;
#ifdef TH_64
				int X, Y;
				Pack2_64((void*)this, &X, &Y);
				makecontext(&Fiber->Context, (void(*)())Execute, 2, X, Y);
#else
				int X = Pack1_32((void*)this);
				makecontext(&Fiber->Context, (void(*)())Execute, 1, X);
#endif
				swapcontext(&Switch->Context, &Fiber->Context);
#else
				Fiber->Context = CreateFiber(Size, Execute, (LPVOID)this);
				SwitchToFiber(Fiber->Context);
#endif
			}
			else
			{
#ifndef TH_MICROSOFT
				swapcontext(&Switch->Context, &Fiber->Context);
#else
				SwitchToFiber(Fiber->Context);
#endif
			}

			return Routine->Dead ? -1 : 1;
		}
		int Costate::Push(Coroutine* Routine)
		{
			TH_ASSERT(Routine != nullptr, -1, "coroutine should be set");
			TH_ASSERT(Routine->Master == this, -1, "coroutine should be created by this costate");
			TH_ASSERT(Routine->Dead, -1, "coroutine should be dead");

			Safe.lock();
            Cached.erase(Routine);
			Used.erase(Routine);
			Safe.unlock();

			TH_DELETE(Coroutine, Routine);
			return 1;
		}
        int Costate::Activate(Coroutine* Routine)
        {
			TH_ASSERT(Routine != nullptr, -1, "coroutine should be set");
			TH_ASSERT(Routine->Master == this, -1, "coroutine should be created by this costate");
			TH_ASSERT(!Routine->Dead, -1, "coroutine should not be dead");

			if (Routine->State != Coactive::Inactive)
				return -1;
			
            Routine->State = Coactive::Resumable;
			if (Waitable != nullptr)
				Waitable->notify_all();

            return 1;
        }
        int Costate::Deactivate(Coroutine* Routine)
        {
			TH_ASSERT(Thread == std::this_thread::get_id(), -1, "cannot deactive coroutine outside costate thread");
			TH_ASSERT(Routine != nullptr, -1, "coroutine should be set");
			TH_ASSERT(Routine->Master == this, -1, "coroutine should be created by this costate");
			TH_ASSERT(!Routine->Dead, -1, "coroutine should not be dead");

			if (Current != Routine || Routine->State != Coactive::Active)
				return -1;

            Routine->State = Coactive::Inactive;
            return Suspend();
        }
		int Costate::Resume(Coroutine* Routine)
		{
			TH_ASSERT(Routine != nullptr, -1, "coroutine should be set");
			TH_ASSERT(Thread == std::this_thread::get_id(), -1, "cannot resume coroutine outside costate thread");
			TH_ASSERT(Routine->Master == this, -1, "coroutine should be created by this costate");

			if (Current == Routine)
				return -1;
			
			return Swap(Routine);
		}
		int Costate::Resume(bool Restore)
		{
			TH_ASSERT(Thread == std::this_thread::get_id(), -1, "cannot resume coroutine outside costate thread");
			if (Used.empty())
				return -1;

			Safe.lock();
			Coroutine* Routine = (Used.empty() ? nullptr : *Used.begin());
			Safe.unlock();

			if (!Routine)
				return -1;

            int Code = Swap(Routine);
			if (Code != -1)
				return Code;

			if (Restore)
				Reuse(Routine);
			else
				Push(Routine);

			return Used.empty() ? Code : 1;
		}
		int Costate::Dispatch(bool Restore)
		{
			TH_ASSERT(Thread == std::this_thread::get_id(), -1, "cannot dispatch coroutine outside costate thread");
			if (Used.empty())
				return -1;

			Safe.lock();
			auto Sources = Used;
			Safe.unlock();

			size_t Activities = 0;
			for (auto* Routine : Sources)
			{
				if (Swap(Routine) != -1)
					continue;

				Activities++;
				if (Restore)
					Reuse(Routine);
				else
					Push(Routine);
				break;
			}

			return Used.empty() ? -1 : (int)Activities;
		}
		int Costate::Suspend()
		{
			TH_ASSERT(Thread == std::this_thread::get_id(), -1, "cannot suspend coroutine outside costate thread");

			Coroutine* Routine = Current;
			if (!Routine || Routine->Master != this)
				return -1;
#ifndef TH_MICROSOFT
			char Bottom = 0;
			char* Top = Routine->Switch->Stack + Size;
			if (size_t(Top - &Bottom) > Size)
				return -1;

			Current = nullptr;
			swapcontext(&Routine->Switch->Context, &Switch->Context);
#else
			Current = nullptr;
			SwitchToFiber(Switch->Context);
#endif
			return 1;
		}
		void Costate::Clear()
		{
			Safe.lock();
			for (auto& Routine : Cached)
				TH_DELETE(Coroutine, Routine);
			Cached.clear();
			Safe.unlock();
		}
		void Costate::Notify(std::condition_variable* Var)
		{
			Safe.lock();
			Waitable = Var;
			Safe.unlock();
		}
		bool Costate::IsWaitable() const
		{
			return Waitable != nullptr;
		}
		std::condition_variable* Costate::GetWaitable()
		{
			return Waitable;
		}
		Coroutine* Costate::GetCurrent() const
		{
			return Current;
		}
		uint64_t Costate::GetCount() const
		{
			return Used.size();
		}
		Costate* Costate::Get()
		{
			return Cothread;
		}
		Coroutine* Costate::GetCoroutine()
		{
			return Cothread ? Cothread->Current : nullptr;
		}
		bool Costate::GetState(Costate** State, Coroutine** Routine)
		{
			TH_ASSERT(State != nullptr, nullptr, "state should be set");
			TH_ASSERT(Routine != nullptr, nullptr, "state should be set");
			*Routine = (Cothread ? Cothread->Current : nullptr);
			*State = Cothread;

			return *Routine != nullptr;
		}
		bool Costate::IsCoroutine()
		{
			return Cothread ? Cothread->Current != nullptr : false;
		}
		void TH_COCALL Costate::Execute(TH_CODATA)
		{
#ifndef TH_MICROSOFT
#ifdef TH_64
			Costate* State = (Costate*)Unpack2_64(X, Y);
#else
			Costate* State = (Costate*)Unpack1_32(X);
#endif
#else
			Costate* State = (Costate*)Context;
#endif
            Cothread = State;
			TH_ASSERT_V(State != nullptr, "costate should be set");
			Coroutine* Routine = State->Current;
            if (Routine != nullptr)
			{
			Reuse:
				if (Routine->Callback)
					Routine->Callback();
				Routine->Dead = true;
			}

			State->Current = nullptr;
#ifndef TH_MICROSOFT
			swapcontext(&Routine->Switch->Context, &State->Switch->Context);
#else
			SwitchToFiber(State->Switch->Context);
#endif
			if (Routine != nullptr && Routine->Callback)
				goto Reuse;
		}
		
		Schedule::Schedule() : Comain(nullptr), Coroutines(0), Threads(0), Stack(0), Timer(0), Terminate(false), Active(false), Enqueue(true)
		{
			Events = (ConcurrentQueue)TH_NEW(EQueue);
			Asyncs = (ConcurrentQueue)TH_NEW(TQueue);
			Tasks = (ConcurrentQueue)TH_NEW(TQueue);
		}
		Schedule::~Schedule()
		{
			Stop();

			EQueue* cEvents = (EQueue*)Events;
			TH_DELETE(ConcurrentQueue, cEvents);

			TQueue* cAsyncs = (TQueue*)Asyncs;
			TH_DELETE(ConcurrentQueue, cAsyncs);

			TQueue* cTasks = (TQueue*)Tasks;
			TH_DELETE(ConcurrentQueue, cTasks);

			if (Singleton == this)
				Singleton = nullptr;
		}
		EventId Schedule::SetListener(const std::string& Name, const EventCallback& Callback)
		{
			TH_ASSERT(Callback, std::numeric_limits<uint64_t>::max(), "callback should not be empty");
			if (!Enqueue)
				return std::numeric_limits<uint64_t>::max();

			TH_PSTART("schedule-listen", TH_PERF_ATOM);
			Race.Listeners.lock();
			auto It = Listeners.find(Name);
			if (It != Listeners.end())
			{
				uint64_t Id = It->second->Counter++;
				It->second->Callbacks[Id] = Callback;
				Race.Listeners.unlock();

				TH_PEND();
				return Id;
			}

			EventListener*& Src = Listeners[Name];
			Src = TH_NEW(EventListener);
			Src->Callbacks[Src->Counter++] = Callback;
			Race.Listeners.unlock();

			TH_PEND();
			return 0;
		}
		EventId Schedule::SetListener(const std::string& Name, EventCallback&& Callback)
		{
			TH_ASSERT(Callback, std::numeric_limits<uint64_t>::max(), "callback should not be empty");
			if (!Enqueue)
				return std::numeric_limits<uint64_t>::max();

			TH_PSTART("schedule-listen", TH_PERF_ATOM);
			Race.Listeners.lock();
			auto It = Listeners.find(Name);
			if (It != Listeners.end())
			{
				uint64_t Id = It->second->Counter++;
				It->second->Callbacks[Id] = std::move(Callback);
				Race.Listeners.unlock();

				TH_PEND();
				return Id;
			}

			EventListener*& Src = Listeners[Name];
			Src = TH_NEW(EventListener);
			Src->Callbacks[Src->Counter++] = std::move(Callback);
			Race.Listeners.unlock();

			TH_PEND();
			return 0;
		}
		EventId Schedule::SetInterval(uint64_t Milliseconds, const TaskCallback& Callback)
		{
			TH_ASSERT(Callback, std::numeric_limits<uint64_t>::max(), "callback should not be empty");
			if (!Enqueue)
				return std::numeric_limits<uint64_t>::max();

			TH_PSTART("schedule-interval", TH_PERF_ATOM);
			int64_t Clock = GetClock();
			Race.Timers.lock();

			EventId Id = Timer++; int64_t Time = GetTimeout(Clock + Milliseconds);
			Timers.emplace(std::make_pair(Time, EventTimer(Callback, Milliseconds, Id, true)));
			Race.Timers.unlock();

			if (!Childs.empty())
				Queue.Publish.notify_one();
			
			TH_PEND();
			return Id;
		}
		EventId Schedule::SetInterval(uint64_t Milliseconds, TaskCallback&& Callback)
		{
			TH_ASSERT(Callback, std::numeric_limits<uint64_t>::max(), "callback should not be empty");
			if (!Enqueue)
				return std::numeric_limits<uint64_t>::max();

			TH_PSTART("schedule-interval", TH_PERF_ATOM);
			int64_t Clock = GetClock();
			Race.Timers.lock();

			EventId Id = Timer++; int64_t Time = GetTimeout(Clock + Milliseconds);
			Timers.emplace(std::make_pair(Time, EventTimer(std::move(Callback), Milliseconds, Id, true)));
			Race.Timers.unlock();

			if (!Childs.empty())
				Queue.Publish.notify_one();

			TH_PEND();
			return Id;
		}
		EventId Schedule::SetTimeout(uint64_t Milliseconds, const TaskCallback& Callback)
		{
			TH_ASSERT(Callback, std::numeric_limits<uint64_t>::max(), "callback should not be empty");
			if (!Enqueue)
				return std::numeric_limits<uint64_t>::max();

			TH_PSTART("schedule-timeout", TH_PERF_ATOM);
			int64_t Clock = GetClock();
			Race.Timers.lock();

			EventId Id = Timer++; int64_t Time = GetTimeout(Clock + Milliseconds);
			Timers.emplace(std::make_pair(Time, EventTimer(Callback, Milliseconds, Id, false)));
			Race.Timers.unlock();

			if (!Childs.empty())
				Queue.Publish.notify_one();

			TH_PEND();
			return Id;
		}
		EventId Schedule::SetTimeout(uint64_t Milliseconds, TaskCallback&& Callback)
		{
			TH_ASSERT(Callback, std::numeric_limits<uint64_t>::max(), "callback should not be empty");
			if (!Enqueue)
				return std::numeric_limits<uint64_t>::max();

			TH_PSTART("schedule-timeout", TH_PERF_ATOM);
			int64_t Clock = GetClock();
			Race.Timers.lock();

			EventId Id = Timer++; int64_t Time = GetTimeout(Clock + Milliseconds);
			Timers.emplace(std::make_pair(Time, EventTimer(std::move(Callback), Milliseconds, Id, false)));
			Race.Timers.unlock();

			if (!Childs.empty())
				Queue.Publish.notify_one();

			TH_PEND();
			return Id;
		}
		bool Schedule::SetTask(const TaskCallback& Callback)
		{
			TH_ASSERT(Callback, false, "callback should not be empty");
			if (!Enqueue)
				return false;

			TH_PSTART("schedule-task", TH_PERF_ATOM);
			((TQueue*)Tasks)->enqueue(TH_NEW(TaskCallback, Callback));
			if (!Childs.empty())
				Queue.Consume.notify_one();

			TH_PEND();
			return true;
		}
		bool Schedule::SetTask(TaskCallback&& Callback)
		{
			TH_ASSERT(Callback, false, "callback should not be empty");
			if (!Enqueue)
				return false;

			TH_PSTART("schedule-task", TH_PERF_ATOM);
			((TQueue*)Tasks)->enqueue(TH_NEW(TaskCallback, std::move(Callback)));
			if (!Childs.empty())
				Queue.Consume.notify_one();

			TH_PEND();
			return true;
		}
		bool Schedule::SetAsync(const TaskCallback& Callback)
		{
			TH_ASSERT(Callback, false, "callback should not be empty");
			if (!Enqueue)
				return false;

			if (Costate::IsCoroutine())
			{
				Callback();
				return true;
			}

			TH_PSTART("schedule-async", TH_PERF_ATOM);
			((TQueue*)Asyncs)->enqueue(TH_NEW(TaskCallback, Callback));
			if (!Childs.empty())
				Queue.Consume.notify_one();
			
			TH_PEND();
			return true;
		}
		bool Schedule::SetAsync(TaskCallback&& Callback)
		{
			TH_ASSERT(Callback, false, "callback should not be empty");
			if (!Enqueue)
				return false;

			if (Costate::IsCoroutine())
			{
				Callback();
				return true;
			}

			TH_PSTART("schedule-async", TH_PERF_ATOM);
			((TQueue*)Asyncs)->enqueue(TH_NEW(TaskCallback, std::move(Callback)));
			if (!Childs.empty())
				Queue.Consume.notify_one();

			TH_PEND();
			return true;
		}
		bool Schedule::SetEvent(const std::string& Name, const VariantArgs& Args)
		{
			TH_ASSERT(!Name.empty(), false, "name should not be empty");
			if (!Enqueue)
				return false;

			TH_PSTART("schedule-event", TH_PERF_ATOM);
			((EQueue*)Events)->enqueue(TH_NEW(EventBase, Name, Args));
			if (!Childs.empty())
				Queue.Publish.notify_one();

			TH_PEND();
			return true;
		}
		bool Schedule::SetEvent(const std::string& Name, VariantArgs&& Args)
		{
			TH_ASSERT(!Name.empty(), false, "name should not be empty");
			if (!Enqueue)
				return false;

			TH_PSTART("schedule-event", TH_PERF_ATOM);
			((EQueue*)Events)->enqueue(TH_NEW(EventBase, Name, std::move(Args)));
			if (!Childs.empty())
				Queue.Publish.notify_one();

			TH_PEND();
			return true;
		}
		bool Schedule::SetEvent(const std::string& Name)
		{
			TH_ASSERT(!Name.empty(), false, "name should not be empty");
			if (!Enqueue)
				return false;

			TH_PSTART("schedule-event", TH_PERF_ATOM);
			((EQueue*)Events)->enqueue(TH_NEW(EventBase, Name));
			if (!Childs.empty())
				Queue.Publish.notify_one();

			TH_PEND();
			return true;
		}
		bool Schedule::ClearListener(const std::string& Name, EventId ListenerId)
		{
			TH_ASSERT(!Name.empty(), false, "name should not be empty");
			TH_PSTART("schedule-unlisten", TH_PERF_ATOM);

			Race.Listeners.lock();
			auto It = Listeners.find(Name);
			if (It != Listeners.end())
			{
				auto Callback = It->second->Callbacks.find(ListenerId);
				if (Callback != It->second->Callbacks.end())
				{
					It->second->Callbacks.erase(Callback);
					Race.Listeners.unlock();

					TH_PEND();
					return true;
				}
			}
			Race.Listeners.unlock();

			TH_PEND();
			return false;
		}
		bool Schedule::ClearTimeout(EventId TimerId)
		{
			TH_PSTART("schedule-cl-timeout", TH_PERF_ATOM);

			Race.Timers.lock();
			for (auto It = Timers.begin(); It != Timers.end(); ++It)
			{
				if (It->second.Id != TimerId)
					continue;

				Timers.erase(It);
				Race.Timers.unlock();

				TH_PEND();
				return true;
			}
			Race.Timers.unlock();

			TH_PEND();
			return false;
		}
		bool Schedule::Start(bool IsAsync, uint64_t ThreadsCount, uint64_t CoroutinesCount, uint64_t StackSize)
		{
			TH_ASSERT(!Active, false, "queue should be stopped");
			Threads = (IsAsync ? ThreadsCount : 0);
			Coroutines = CoroutinesCount;
			Stack = StackSize;

			if (!IsAsync)
			{
				Active = true;
				return true;
			}

			Childs.reserve(Threads + 1);
			Childs.emplace_back(std::thread(&Schedule::Publish, this));
			for (uint64_t i = 0; i < Threads; i++)
				Childs.emplace_back(std::thread(&Schedule::Consume, this));

			Active = true;

			Queue.Publish.notify_one();
			Queue.Consume.notify_one();
			return true;
		}
		bool Schedule::Stop()
		{
			Race.Basement.lock();
			if (!Active && !Terminate)
			{
				Race.Basement.unlock();
				return false;
			}

			Active = Enqueue = false;
			Queue.Publish.notify_all();
			Queue.Consume.notify_all();

			for (auto& Thread : Childs)
			{
				if (Thread.get_id() == std::this_thread::get_id())
				{
					Terminate = true;
					Race.Basement.unlock();
					return false;
				}

				if (Thread.joinable())
					Thread.join();
			}

			Childs.clear();
			while (Dispatch());

			EQueue* cEvents = (EQueue*)Events;
			TQueue* cAsyncs = (TQueue*)Asyncs;
			TQueue* cTasks = (TQueue*)Tasks;
			TaskCallback* Callback = nullptr;
			EventBase* Event = nullptr;

			while (cEvents->try_dequeue(Event) || cEvents->size_approx() > 0)
			{
				TH_DELETE(EventBase, Event);
				Event = nullptr;
			}

			while (cAsyncs->try_dequeue(Callback) || cAsyncs->size_approx() > 0)
			{
				TH_DELETE(function, Callback);
				Callback = nullptr;
			}

			while (cTasks->try_dequeue(Callback) || cTasks->size_approx() > 0)
			{
				TH_DELETE(function, Callback);
				Callback = nullptr;
			}

			Race.Listeners.lock();
			for (auto& Listener : Listeners)
				TH_DELETE(EventListener, Listener.second);
			Listeners.clear();
			Race.Listeners.unlock();

			Race.Timers.lock();
			Timers.clear();
			Race.Timers.unlock();

			TH_CLEAR(Comain);
			Terminate = false;
			Enqueue = true;
			Threads = Stack = Timer = 0;
			Race.Basement.unlock();

			return true;
		}
		bool Schedule::Dispatch()
		{
			if (!Comain && Stack > 0 && Coroutines > 0)
				Comain = new Costate(Stack);

			int fAsyncs = DispatchAsync(nullptr, Comain, true);
			int fEvents = DispatchEvent(nullptr);
			int fTimers = DispatchTimer(nullptr);
			int fTasks = DispatchTask(nullptr);

			return fAsyncs != -1 || fEvents != -1 || fTimers != -1 || fTasks != -1;
		}
		bool Schedule::Publish()
		{
			CToken tEvents(*((EQueue*)Events));
			ConcurrentToken* vEvents = (ConcurrentToken*)&tEvents;
            int fEvents = -1, fTimers = -1;
            int64_t When = -1;
            
			if (!Active)
				goto Wait;

			do
			{
                fEvents = DispatchEvent(vEvents);
				fTimers = DispatchTimer(&When);

				if (fEvents == -1 && fTimers == 0)
                {
                    std::unique_lock<std::mutex> Lock(Race.Threads);
                    Queue.Publish.wait_for(Lock, std::chrono::milliseconds(When));
                }
                
				if (fEvents != -1 || fTimers != -1)
					continue;
			Wait:
				std::unique_lock<std::mutex> Lock(Race.Threads);
				Queue.Publish.wait(Lock);
			} while (Active);

			return true;
		}
		bool Schedule::Consume()
		{
			Costate* State = nullptr;
			if (Stack > 0 && Coroutines > 0)
			{
				State = new Costate(Stack);
				State->Notify(&Queue.Consume);
			}

			CToken tAsyncs(*((TQueue*)Asyncs)), tTasks(*((TQueue*)Tasks));
			ConcurrentToken* vAsyncs = (ConcurrentToken*)&tAsyncs;
			ConcurrentToken* vTasks = (ConcurrentToken*)&tTasks;
            int fAsyncs = -1, fTasks = -1;

            if (!Active)
				goto Wait;

			do
			{
                fAsyncs = DispatchAsync(vAsyncs, State, false);
				fTasks = DispatchTask(vTasks);

				if (fAsyncs != -1 || fTasks != -1)
					continue;
			Wait:
				std::unique_lock<std::mutex> Lock(Race.Threads);
				Queue.Consume.wait(Lock);
			} while (Active);

			TH_RELEASE(State);
			return true;
		}
		int Schedule::DispatchAsync(ConcurrentToken* Token, Costate* State, bool Reconsume)
		{
			CToken* cToken = (CToken*)Token;
			TQueue* cAsyncs = (TQueue*)Asyncs;
			TaskCallback* Data = nullptr;

			if (State != nullptr)
			{
			ResolveStateful:
				while ((cToken ? cAsyncs->try_dequeue(*cToken, Data) : cAsyncs->try_dequeue(Data)) && State->GetCount() < Coroutines)
				{
					State->Pop(std::move(*Data));
					TH_DELETE(function, Data);
				}

				if (State->GetCount() < Coroutines && cAsyncs->size_approx() > 0)
					goto ResolveStateful;

				if (!State->GetCount())
					return Data != nullptr ? 1 : -1;

				while (true)
				{
					TH_PSTART("dispatch-async", TH_PERF_CORE);
					int Count = State->Dispatch();
					TH_PEND();
					if (Count == -1)
						break;

					if (Reconsume)
					{
						DispatchEvent(nullptr);
						DispatchTimer(nullptr);
					}

					int fTasks = DispatchTask(nullptr);
					if (fTasks == -1 && Count == 0 && State->IsWaitable())
					{
						std::unique_lock<std::mutex> Lock(Race.Threads);
						State->GetWaitable()->wait(Lock);
					}

				ResolveNext:
					while ((cToken ? cAsyncs->try_dequeue(*cToken, Data) : cAsyncs->try_dequeue(Data)) && State->GetCount() < Coroutines)
					{
						State->Pop(std::move(*Data));
						TH_DELETE(function, Data);
					}

					if (cAsyncs->size_approx() > 0)
						goto ResolveNext;
				}
			}
			else
			{
			ResolveStateless:
				while (cToken ? cAsyncs->try_dequeue(*cToken, Data) : cAsyncs->try_dequeue(Data))
				{
					(*Data)();
					TH_DELETE(function, Data);
					if (!Threads)
						break;
				}

				if (Threads > 0 && cAsyncs->size_approx() > 0)
					goto ResolveStateless;

				return Data != nullptr ? 1 : -1;
			}

			return Data != nullptr ? 1 : -1;
		}
		int Schedule::DispatchTask(ConcurrentToken* Token)
		{
			CToken* cToken = (CToken*)Token;
			TQueue* cTasks = (TQueue*)Tasks;
			TaskCallback* Data = nullptr;

		Resolve:
			TH_PSTART("dispatch-task", TH_PERF_MAX);
			while (cToken ? cTasks->try_dequeue(*cToken, Data) : cTasks->try_dequeue(Data))
			{
				(*Data)();
				TH_DELETE(function, Data);
				if (!Threads)
					break;
			}

			TH_PEND();
			if (Threads > 0 && cTasks->size_approx() > 0)
				goto Resolve;

			return Data != nullptr ? 1 : -1;
		}
		int Schedule::DispatchEvent(ConcurrentToken* Token)
		{
			CToken* cToken = (CToken*)Token;
			EQueue* cEvents = (EQueue*)Events;
			EventBase* Data = nullptr;

		Resolve:
			while (cToken ? cEvents->try_dequeue(*cToken, Data) : cEvents->try_dequeue(Data))
			{
				Race.Listeners.lock();
				auto Base = Listeners.find(Data->Name);
				if (Base == Listeners.end())
				{
					Race.Listeners.unlock();
					TH_DELETE(EventBase, Data);
					if (!Threads)
						break;

					continue;
				}

				auto Array = Base->second->Callbacks;
				Race.Listeners.unlock();

				SetTask([Data, Array]() mutable
				{
					for (auto& Callback : Array)
						Callback.second(Data->Args);
					TH_DELETE(EventBase, Data);
				});

				if (!Threads)
					break;
			}

			if (Threads > 0 && cEvents->size_approx() > 0)
				goto Resolve;

			return Data != nullptr ? 1 : -1;
		}
		int Schedule::DispatchTimer(int64_t* When)
		{
			if (Timers.empty())
				return -1;

			int64_t Clock = GetClock();
			Race.Timers.lock();

			auto It = Timers.begin();
			if (It == Timers.end())
			{
				Race.Timers.unlock();
				return -1;
			}

			if (Active && It->first >= Clock)
			{
                if (When != nullptr)
                    *When = It->first - Clock;
				Race.Timers.unlock();
				return 0;
			}

			if (!It->second.Alive || !Active)
			{
				SetTask(std::move(It->second.Callback));
				Timers.erase(It);
				Race.Timers.unlock();
				return 1;
			}

			EventTimer Next(std::move(It->second));
			Timers.erase(It);

			SetTask((const TaskCallback&)Next.Callback);

			int64_t Time = GetTimeout(Clock + Next.Timeout);
			Timers.emplace(std::make_pair(Time, std::move(Next)));
			Race.Timers.unlock();
			return 1;
		}
		bool Schedule::IsBlockable()
		{
			return Active && Threads > 0;
		}
		bool Schedule::IsActive()
		{
			return Active;
		}
		bool Schedule::IsProcessing()
		{
			EQueue* cEvents = (EQueue*)Events;
			TQueue* cAsyncs = (TQueue*)Asyncs;
			TQueue* cTasks = (TQueue*)Tasks;

			return cAsyncs->size_approx() > 0 || cTasks->size_approx() > 0 || cEvents->size_approx() || !Timers.empty();
		}
		int64_t Schedule::GetClock()
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		}
		int64_t Schedule::GetTimeout(int64_t Clock)
		{
			while (Timers.find(Clock) != Timers.end())
				Clock++;

			return Clock;
		}
		bool Schedule::Reset()
		{
			if (!Singleton)
				return false;

			TH_RELEASE(Singleton);
			return true;
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
				for (auto It = Parent->Nodes.begin(); It != Parent->Nodes.end(); ++It)
				{
					if (*It == this)
					{
						Parent->Nodes.erase(It);
						break;
					}
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

			for (auto It = Names.begin() + 1; It != Names.end() - 1; ++It)
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
		std::vector<Document*>& Document::GetChilds()
		{
			return Nodes;
		}
		Document* Document::Find(const std::string& Name, bool Deep) const
		{
			if (Value.Type == VarType::Array)
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

			for (auto It = Names.begin() + 1; It != Names.end(); ++It)
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
        Variant Document::FetchVar(const std::string& fKey, bool Deep) const
        {
            Document* Result = Fetch(fKey, Deep);
            if (!Result)
                return Var::Undefined();

            return Result->Value;
        }
		Variant Document::GetVar(size_t Index) const
		{
			Document* Result = Get(Index);
			if (!Result)
				return Var::Undefined();

			return Result->Value;
		}
		Variant Document::GetVar(const std::string& fKey) const
		{
			Document* Result = Get(fKey);
			if (!Result)
				return Var::Undefined();

			return Result->Value;
		}
		Document* Document::Get(size_t Index) const
		{
			TH_ASSERT(Index < Nodes.size(), nullptr, "index outside of range");
			return Nodes[Index];
		}
		Document* Document::Get(const std::string& Name) const
		{
			TH_ASSERT(!Name.empty(), nullptr, "name should not be empty");
			for (auto Document : Nodes)
			{
				if (Document->Key == Name)
					return Document;
			}

			return nullptr;
		}
		Document* Document::Set(const std::string& Name)
		{
			return Set(Name, Var::Object());
		}
		Document* Document::Set(const std::string& Name, const Variant& Base)
		{
			Saved = false;
			if (Value.Type == VarType::Object)
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
			if (Value.Type == VarType::Object)
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
				return Set(Name, Var::Null());

			Base->Key.assign(Name);
			Base->Saved = false;
			Base->Parent = this;
			Saved = false;

			if (Value.Type == VarType::Object)
			{
				for (auto It = Nodes.begin(); It != Nodes.end(); ++It)
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
		Document* Document::SetAttribute(const std::string& Name, const Variant& fValue)
		{
			return Set("[" + Name + "]", fValue);
		}
		Document* Document::SetAttribute(const std::string& Name, Variant&& fValue)
		{
			return Set("[" + Name + "]", std::move(fValue));
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
				return Push(Var::Null());

			Base->Saved = false;
			Base->Parent = this;
			Saved = false;

			Nodes.push_back(Base);
			return Base;
		}
		Document* Document::Pop(size_t Index)
		{
			TH_ASSERT(Index < Nodes.size(), nullptr, "index outside of range");

			Document* Base = Nodes[Index];
			Base->Parent = nullptr;
			TH_RELEASE(Base);
			Nodes.erase(Nodes.begin() + Index);

			return this;
		}
		Document* Document::Pop(const std::string& Name)
		{
			for (auto It = Nodes.begin(); It != Nodes.end(); ++It)
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

			for (auto It = New->Nodes.begin(); It != New->Nodes.end(); ++It)
			{
				if (*It != nullptr)
					*It = (*It)->Copy();
			}

			return New;
		}
		bool Document::Rename(const std::string& Name, const std::string& NewName)
		{
			TH_ASSERT(!Name.empty() && !NewName.empty(), false, "name and new name should not be empty");

			Document* Result = Get(Name);
			if (!Result)
				return false;

			Result->Key = NewName;
			return true;
		}
		bool Document::Has(const std::string& Name) const
		{
			return Fetch(Name) != nullptr;
		}
		bool Document::Has64(const std::string& Name, size_t Size) const
		{
			Document* Base = Fetch(Name);
			if (!Base || Base->Value.GetType() != VarType::Base64)
				return false;

			return Base->Value.GetSize() == Size;
		}
		bool Document::IsEmpty() const
		{
			return Nodes.empty();
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
		size_t Document::Size() const
		{
			return Nodes.size();
		}
		std::string Document::GetName() const
		{
			return IsAttribute() ? Key.substr(1, Key.size() - 2) : Key;
		}
		void Document::Join(Document* Other, bool Copy, bool Fast)
		{
			TH_ASSERT_V(Other != nullptr && Value.IsObject(), "other should be object and not empty");

			Saved = false;
			Nodes.reserve(Nodes.size() + Other->Nodes.size());
			if (Copy)
			{
				for (auto& Node : Other->Nodes)
				{
					Document* Result = Node->Copy();
					Result->Saved = false;
					Result->Parent = this;
					Saved = false;

					if (Value.Type == VarType::Array && !Fast)
					{
						for (auto It = Nodes.begin(); It != Nodes.end(); ++It)
						{
							if ((*It)->Key == Result->Key)
							{
								(*It)->Parent = nullptr;
								TH_RELEASE(*It);
								*It = Result;
								break;
							}
						}
					}
					else
						Nodes.push_back(Result);
				}
			}
			else
			{
				Nodes.insert(Nodes.end(), Other->Nodes.begin(), Other->Nodes.end());
				Other->Nodes.clear();

				for (auto& Node : Nodes)
				{
					Node->Saved = false;
					Node->Parent = this;
				}
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
		bool Document::Transform(Document* Value, const DocNameCallback& Callback)
		{
			TH_ASSERT(Value != nullptr && Callback, false, "value should be set and callback should not be empty");
			Value->Key = Callback(Value->Key);
			for (auto* Item : Value->Nodes)
				Transform(Item, Callback);

			return true;
		}
		bool Document::WriteXML(Document* Base, const DocWriteCallback& Callback)
		{
			TH_ASSERT(Base != nullptr && Callback, false, "base should be set and callback should not be empty");
			std::vector<Document*> Attributes = Base->GetAttributes();
			bool Scalable = (Base->Value.GetSize() || ((int64_t)Base->Nodes.size() - (int64_t)Attributes.size()) > 0);
			Callback(VarForm::Write_Tab, "", 0);
			Callback(VarForm::Dummy, "<", 1);
			Callback(VarForm::Dummy, Base->Key.c_str(), (int64_t)Base->Key.size());

			if (Attributes.empty())
			{
				if (Scalable)
					Callback(VarForm::Dummy, ">", 1);
				else
					Callback(VarForm::Dummy, " />", 3);
			}
			else
				Callback(VarForm::Dummy, " ", 1);

			for (auto It = Attributes.begin(); It != Attributes.end(); ++It)
			{
				std::string Key = (*It)->GetName();
				std::string Value = (*It)->Value.Serialize();

				Callback(VarForm::Dummy, Key.c_str(), (int64_t)Key.size());
				Callback(VarForm::Dummy, "=\"", 2);
				Callback(VarForm::Dummy, Value.c_str(), (int64_t)Value.size());
				++It;

				if (It == Attributes.end())
				{
					if (!Scalable)
					{
						Callback(VarForm::Write_Space, "\"", 1);
						Callback(VarForm::Dummy, "/>", 2);
					}
					else
						Callback(VarForm::Dummy, "\">", 2);
				}
				else
					Callback(VarForm::Write_Space, "\"", 1);

				--It;
			}

			Callback(VarForm::Tab_Increase, "", 0);
			if (Base->Value.GetSize() > 0)
			{
				std::string Text = Base->Value.Serialize();
				if (!Base->Nodes.empty())
				{
					Callback(VarForm::Write_Line, "", 0);
					Callback(VarForm::Write_Tab, "", 0);
					Callback(VarForm::Dummy, Text.c_str(), Text.size());
					Callback(VarForm::Write_Line, "", 0);
				}
				else
					Callback(VarForm::Dummy, Text.c_str(), Text.size());
			}
			else
				Callback(VarForm::Write_Line, "", 0);

			for (auto&& It : Base->Nodes)
			{
				if (!It->IsAttribute())
					WriteXML(It, Callback);
			}

			Callback(VarForm::Tab_Decrease, "", 0);
			if (!Scalable)
				return true;

			if (!Base->Nodes.empty())
				Callback(VarForm::Write_Tab, "", 0);

			Callback(VarForm::Dummy, "</", 2);
			Callback(VarForm::Dummy, Base->Key.c_str(), (int64_t)Base->Key.size());
			Callback(Base->Parent ? VarForm::Write_Line : VarForm::Dummy, ">", 1);

			return true;
		}
		bool Document::WriteJSON(Document* Base, const DocWriteCallback& Callback)
		{
			TH_ASSERT(Base != nullptr && Callback, false, "base should be set and callback should not be empty");
			if (!Base->Parent && !Base->Value.IsObject())
			{
				std::string Value = Base->Value.Serialize();
				Core::Parser Safe(&Value);
				Safe.Escape();

				if (Base->Value.Type != VarType::String && Base->Value.Type != VarType::Base64)
				{
					if (!Value.empty() && Value.front() == TH_PREFIX_CHAR)
						Callback(VarForm::Dummy, Value.c_str() + 1, (int64_t)Value.size() - 1);
					else
						Callback(VarForm::Dummy, Value.c_str(), (int64_t)Value.size());
				}
				else
				{
					Callback(VarForm::Dummy, "\"", 1);
					Callback(VarForm::Dummy, Value.c_str(), (int64_t)Value.size());
					Callback(VarForm::Dummy, "\"", 1);
				}

				return true;
			}

			size_t Size = Base->Nodes.size(), Offset = 0;
			bool Array = (Base->Value.Type == VarType::Array);

			if (Base->Parent != nullptr)
				Callback(VarForm::Write_Line, "", 0);

			Callback(VarForm::Write_Tab, "", 0);
			Callback(VarForm::Dummy, Array ? "[" : "{", 1);
			Callback(VarForm::Tab_Increase, "", 0);

			for (auto&& Document : Base->Nodes)
			{
				if (!Array)
				{
					Callback(VarForm::Write_Line, "", 0);
					Callback(VarForm::Write_Tab, "", 0);
					Callback(VarForm::Dummy, "\"", 1);
					Callback(VarForm::Dummy, Document->Key.c_str(), (int64_t)Document->Key.size());
					Callback(VarForm::Write_Space, "\":", 2);
				}

				if (!Document->Value.IsObject())
				{
					std::string Value = Document->Value.Serialize();
					Core::Parser Safe(&Value);
					Safe.Escape();

					if (Array)
					{
						Callback(VarForm::Write_Line, "", 0);
						Callback(VarForm::Write_Tab, "", 0);
					}

					if (!Document->Value.IsObject() && Document->Value.Type != VarType::String && Document->Value.Type != VarType::Base64)
					{
						if (!Value.empty() && Value.front() == TH_PREFIX_CHAR)
							Callback(VarForm::Dummy, Value.c_str() + 1, (int64_t)Value.size() - 1);
						else
							Callback(VarForm::Dummy, Value.c_str(), (int64_t)Value.size());
					}
					else
					{
						Callback(VarForm::Dummy, "\"", 1);
						Callback(VarForm::Dummy, Value.c_str(), (int64_t)Value.size());
						Callback(VarForm::Dummy, "\"", 1);
					}
				}
				else
					WriteJSON(Document, Callback);

				Offset++;
				if (Offset < Size)
					Callback(VarForm::Dummy, ",", 1);
			}

			Callback(VarForm::Tab_Decrease, "", 0);
			Callback(VarForm::Write_Line, "", 0);

			if (Base->Parent != nullptr)
				Callback(VarForm::Write_Tab, "", 0);

			Callback(VarForm::Dummy, Array ? "]" : "}", 1);
			return true;
		}
		bool Document::WriteJSONB(Document* Base, const DocWriteCallback& Callback)
		{
			TH_ASSERT(Base != nullptr && Callback, false, "base should be set and callback should not be empty");
			std::unordered_map<std::string, uint64_t> Mapping = Base->GetNames();
			uint64_t Set = (uint64_t)Mapping.size();

			Callback(VarForm::Dummy, "\0b\0i\0n\0h\0e\0a\0d\r\n", sizeof(char) * 16);
			Callback(VarForm::Dummy, (const char*)&Set, sizeof(uint64_t));

			for (auto It = Mapping.begin(); It != Mapping.end(); ++It)
			{
				uint64_t Size = (uint64_t)It->first.size();
				Callback(VarForm::Dummy, (const char*)&It->second, sizeof(uint64_t));
				Callback(VarForm::Dummy, (const char*)&Size, sizeof(uint64_t));

				if (Size > 0)
					Callback(VarForm::Dummy, It->first.c_str(), sizeof(char) * Size);
			}

			ProcessJSONBWrite(Base, &Mapping, Callback);
			return true;
		}
		Document* Document::ReadXML(int64_t Size, const DocReadCallback& Callback, bool Assert)
		{
			TH_ASSERT(Size > 0 && Callback, nullptr, "size should be greater than zero and callback should not be empty");
			std::string Buffer;
			Buffer.resize(Size);
			if (!Callback((char*)Buffer.c_str(), sizeof(char) * Size))
			{
				if (Assert)
					TH_ERR("cannot read xml document");

				return nullptr;
			}

			rapidxml::xml_document<>* iDocument = TH_NEW(rapidxml::xml_document<>);
			if (!iDocument)
				return nullptr;

			try
			{
				iDocument->parse<rapidxml::parse_trim_whitespace>((char*)Buffer.c_str());
			}
			catch (const std::runtime_error& e)
			{
				TH_DELETE(xml_document, iDocument);
				if (Assert)
					TH_ERR("[xml] %s", e.what());

				return nullptr;
			}
			catch (const rapidxml::parse_error& e)
			{
				TH_DELETE(xml_document, iDocument);
				if (Assert)
					TH_ERR("[xml] %s", e.what());

				return nullptr;
			}
			catch (const std::exception& e)
			{
				TH_DELETE(xml_document, iDocument);
				if (Assert)
					TH_ERR("[xml] %s", e.what());

				return nullptr;
			}
			catch (...)
			{
				TH_DELETE(xml_document, iDocument);
				if (Assert)
					TH_ERR("[xml] parse error");

				return nullptr;
			}

			rapidxml::xml_node<>* Base = iDocument->first_node();
			if (!Base)
			{
				iDocument->clear();
				TH_DELETE(xml_document, iDocument);

				return nullptr;
			}

			Document* Result = Var::Set::Array();
			Result->Key = Base->name();

			if (!ProcessXMLRead((void*)Base, Result))
				TH_CLEAR(Result);

			iDocument->clear();
			TH_DELETE(xml_document, iDocument);

			return Result;
		}
		Document* Document::ReadJSON(int64_t Size, const DocReadCallback& Callback, bool Assert)
		{
			TH_ASSERT(Size > 0 && Callback, nullptr, "size should be greater than zero and callback should not be empty");
			std::string Buffer;
			Buffer.resize(Size);
			if (!Callback((char*)Buffer.c_str(), sizeof(char) * Size))
			{
				if (Assert)
					TH_ERR("cannot read json document");

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
						TH_ERR("[json:%i] the document is empty", Offset);
						break;
					case rapidjson::kParseErrorDocumentRootNotSingular:
						TH_ERR("[json:%i] the document root must not follow by other values", Offset);
						break;
					case rapidjson::kParseErrorValueInvalid:
						TH_ERR("[json:%i] invalid value", Offset);
						break;
					case rapidjson::kParseErrorObjectMissName:
						TH_ERR("[json:%i] missing a name for object member", Offset);
						break;
					case rapidjson::kParseErrorObjectMissColon:
						TH_ERR("[json:%i] missing a colon after a name of object member", Offset);
						break;
					case rapidjson::kParseErrorObjectMissCommaOrCurlyBracket:
						TH_ERR("[json:%i] missing a comma or '}' after an object member", Offset);
						break;
					case rapidjson::kParseErrorArrayMissCommaOrSquareBracket:
						TH_ERR("[json:%i] missing a comma or ']' after an array element", Offset);
						break;
					case rapidjson::kParseErrorStringUnicodeEscapeInvalidHex:
						TH_ERR("[json:%i] incorrect hex digit after \\u escape in string", Offset);
						break;
					case rapidjson::kParseErrorStringUnicodeSurrogateInvalid:
						TH_ERR("[json:%i] the surrogate pair in string is invalid", Offset);
						break;
					case rapidjson::kParseErrorStringEscapeInvalid:
						TH_ERR("[json:%i] invalid escape character in string", Offset);
						break;
					case rapidjson::kParseErrorStringMissQuotationMark:
						TH_ERR("[json:%i] missing a closing quotation mark in string", Offset);
						break;
					case rapidjson::kParseErrorStringInvalidEncoding:
						TH_ERR("[json:%i] invalid encoding in string", Offset);
						break;
					case rapidjson::kParseErrorNumberTooBig:
						TH_ERR("[json:%i] number too big to be stored in double", Offset);
						break;
					case rapidjson::kParseErrorNumberMissFraction:
						TH_ERR("[json:%i] miss fraction part in number", Offset);
						break;
					case rapidjson::kParseErrorNumberMissExponent:
						TH_ERR("[json:%i] miss exponent in number", Offset);
						break;
					case rapidjson::kParseErrorTermination:
						TH_ERR("[json:%i] parsing was terminated", Offset);
						break;
					case rapidjson::kParseErrorUnspecificSyntaxError:
						TH_ERR("[json:%i] unspecific syntax error", Offset);
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
					Result = new Document(Var::Null());
					break;
				case rapidjson::kFalseType:
					Result = new Document(Var::Boolean(false));
					break;
				case rapidjson::kTrueType:
					Result = new Document(Var::Boolean(true));
					break;
				case rapidjson::kObjectType:
					Result = Var::Set::Object();
					if (!ProcessJSONRead((void*)&Base, Result))
						TH_CLEAR(Result);
					break;
				case rapidjson::kArrayType:
					Result = Var::Set::Array();
					if (!ProcessJSONRead((void*)&Base, Result))
						TH_CLEAR(Result);
					break;
				case rapidjson::kStringType:
					Result = new Document(Var::Auto(std::string(Base.GetString(), Base.GetStringLength()), true));
					break;
				case rapidjson::kNumberType:
					if (Base.IsInt())
						Result = new Document(Var::Integer(Base.GetInt64()));
					else
						Result = new Document(Var::Number(Base.GetDouble()));
					break;
				default:
					Result = new Document(Var::Undefined());
					break;
			}

			return Result;
		}
		Document* Document::ReadJSONB(const DocReadCallback& Callback, bool Assert)
		{
			TH_ASSERT(Callback, nullptr, "callback should not be empty");
			char Hello[18];
			if (!Callback((char*)Hello, sizeof(char) * 16))
			{
				if (Assert)
					TH_ERR("form cannot be defined");

				return nullptr;
			}

			if (memcmp((void*)Hello, (void*)"\0b\0i\0n\0h\0e\0a\0d\r\n", sizeof(char) * 16) != 0)
			{
				if (Assert)
					TH_ERR("version is undefined");

				return nullptr;
			}

			uint64_t Set = 0;
			if (!Callback((char*)&Set, sizeof(uint64_t)))
			{
				if (Assert)
					TH_ERR("name map is undefined");

				return nullptr;
			}

			std::unordered_map<uint64_t, std::string> Map;
			for (uint64_t i = 0; i < Set; i++)
			{
				uint64_t Index = 0;
				if (!Callback((char*)&Index, sizeof(uint64_t)))
				{
					if (Assert)
						TH_ERR("name index is undefined");

					return nullptr;
				}

				uint64_t Size = 0;
				if (!Callback((char*)&Size, sizeof(uint64_t)))
				{
					if (Assert)
						TH_ERR("name size is undefined");

					return nullptr;
				}

				if (Size <= 0)
					continue;

				std::string Name;
				Name.resize(Size);
				if (!Callback((char*)Name.c_str(), sizeof(char) * Size))
				{
					if (Assert)
						TH_ERR("name data is undefined");

					return nullptr;
				}

				Map.insert({ Index, Name });
			}

			Document* Current = Var::Set::Object();
			if (!ProcessJSONBRead(Current, &Map, Callback))
			{
				TH_RELEASE(Current);
				return nullptr;
			}

			return Current;
		}
		bool Document::ProcessXMLRead(void* Base, Document* Current)
		{
			TH_ASSERT(Base != nullptr && Current != nullptr, false, "base and current should be set");

			auto Ref = (rapidxml::xml_node<>*)Base;
			for (rapidxml::xml_attribute<>* It = Ref->first_attribute(); It; It = It->next_attribute())
			{
				if (It->name()[0] != '\0')
					Current->SetAttribute(It->name(), Var::Auto(It->value()));
			}

			for (rapidxml::xml_node<>* It = Ref->first_node(); It; It = It->next_sibling())
			{
				if (It->name()[0] == '\0')
					continue;

				Document* Subresult = Current->Set(It->name(), Var::Set::Array());
				ProcessXMLRead((void*)It, Subresult);

				if (Subresult->Nodes.empty() && It->value_size() > 0)
					Subresult->Value.Deserialize(std::string(It->value(), It->value_size()));
			}

			return true;
		}
		bool Document::ProcessJSONRead(void* Base, Document* Current)
		{
			TH_ASSERT(Base != nullptr && Current != nullptr, false, "base and current should be set");

			auto Ref = (rapidjson::Value*)Base;
			std::string Value;

			if (!Ref->IsArray())
			{
				VarType Type = Current->Value.Type;
				Current->Value.Type = VarType::Array;

				std::string Name;
				for (auto It = Ref->MemberBegin(); It != Ref->MemberEnd(); ++It)
				{
					if (!It->name.IsString())
						continue;

					Name.assign(It->name.GetString(), (size_t)It->name.GetStringLength());
					switch (It->value.GetType())
					{
						case rapidjson::kNullType:
							Current->Set(Name, Var::Null());
							break;
						case rapidjson::kFalseType:
							Current->Set(Name, Var::Boolean(false));
							break;
						case rapidjson::kTrueType:
							Current->Set(Name, Var::Boolean(true));
							break;
						case rapidjson::kObjectType:
							ProcessJSONRead((void*)&It->value, Current->Set(Name));
							break;
						case rapidjson::kArrayType:
							ProcessJSONRead((void*)&It->value, Current->Set(Name, Var::Array()));
							break;
						case rapidjson::kStringType:
							Value.assign(It->value.GetString(), It->value.GetStringLength());
							Current->Set(Name, Var::Auto(Value, true));
							break;
						case rapidjson::kNumberType:
							if (It->value.IsInt())
								Current->Set(Name, Var::Integer(It->value.GetInt64()));
							else
								Current->Set(Name, Var::Number(It->value.GetDouble()));
							break;
						default:
							break;
					}
				}

				Current->Value.Type = Type;
			}
			else
			{
				for (auto It = Ref->Begin(); It != Ref->End(); ++It)
				{
					switch (It->GetType())
					{
						case rapidjson::kNullType:
							Current->Push(Var::Null());
							break;
						case rapidjson::kFalseType:
							Current->Push(Var::Boolean(false));
							break;
						case rapidjson::kTrueType:
							Current->Push(Var::Boolean(true));
							break;
						case rapidjson::kObjectType:
							ProcessJSONRead((void*)It, Current->Push(Var::Object()));
							break;
						case rapidjson::kArrayType:
							ProcessJSONRead((void*)It, Current->Push(Var::Array()));
							break;
						case rapidjson::kStringType:
							Value.assign(It->GetString(), It->GetStringLength());
							Current->Push(Var::Auto(Value, true));
							break;
						case rapidjson::kNumberType:
							if (It->IsInt())
								Current->Push(Var::Integer(It->GetInt64()));
							else
								Current->Push(Var::Number(It->GetDouble()));
							break;
						default:
							break;
					}
				}
			}

			return true;
		}
		bool Document::ProcessJSONBWrite(Document* Current, std::unordered_map<std::string, uint64_t>* Map, const DocWriteCallback& Callback)
		{
			uint64_t Id = Map->at(Current->Key);
			Callback(VarForm::Dummy, (const char*)&Id, sizeof(uint64_t));
			Callback(VarForm::Dummy, (const char*)&Current->Value.Type, sizeof(VarType));

			switch (Current->Value.Type)
			{
				case VarType::Object:
				case VarType::Array:
				{
					uint64_t Count = Current->Nodes.size();
					Callback(VarForm::Dummy, (const char*)&Count, sizeof(uint64_t));
					for (auto& Document : Current->Nodes)
						ProcessJSONBWrite(Document, Map, Callback);
					break;
				}
				case VarType::String:
				case VarType::Base64:
				{
					uint64_t Size = Current->Value.GetSize();
					Callback(VarForm::Dummy, (const char*)&Size, sizeof(uint64_t));
					Callback(VarForm::Dummy, Current->Value.GetString(), Size * sizeof(char));
					break;
				}
				case VarType::Decimal:
				{
					std::string Number = ((Decimal*)Current->Value.Value.Data)->ToString();
					uint64_t Size = (uint64_t)Number.size();
					Callback(VarForm::Dummy, (const char*)&Size, sizeof(uint64_t));
					Callback(VarForm::Dummy, Number.c_str(), Size * sizeof(char));
					break;
				}
				case VarType::Integer:
				{
					int64_t Copy = Current->Value.GetInteger();
					Callback(VarForm::Dummy, (const char*)&Copy, sizeof(int64_t));
					break;
				}
				case VarType::Number:
				{
					double Copy = Current->Value.GetNumber();
					Callback(VarForm::Dummy, (const char*)&Copy, sizeof(double));
					break;
				}
				case VarType::Boolean:
				{
					bool Copy = Current->Value.GetBoolean();
					Callback(VarForm::Dummy, (const char*)&Copy, sizeof(bool));
					break;
				}
				default:
					break;
			}

			return true;
		}
		bool Document::ProcessJSONBRead(Document* Current, std::unordered_map<uint64_t, std::string>* Map, const DocReadCallback& Callback)
		{
			uint64_t Id = 0;
			if (!Callback((char*)&Id, sizeof(uint64_t)))
			{
				TH_ERR("key name index is undefined");
				return false;
			}

			auto It = Map->find(Id);
			if (It != Map->end())
				Current->Key = It->second;

			if (!Callback((char*)&Current->Value.Type, sizeof(VarType)))
			{
				TH_ERR("key type is undefined");
				return false;
			}

			switch (Current->Value.Type)
			{
				case VarType::Object:
				case VarType::Array:
				{
					uint64_t Count = 0;
					if (!Callback((char*)&Count, sizeof(uint64_t)))
					{
						TH_ERR("key value size is undefined");
						return false;
					}

					if (!Count)
						break;

					Current->Nodes.resize(Count);
					for (auto&& Item : Current->Nodes)
					{
						Item = Var::Set::Object();
						Item->Parent = Current;
						Item->Saved = true;

						ProcessJSONBRead(Item, Map, Callback);
					}
					break;
				}
				case VarType::String:
				{
					uint64_t Size = 0;
					if (!Callback((char*)&Size, sizeof(uint64_t)))
					{
						TH_ERR("key value size is undefined");
						return false;
					}

					std::string Buffer;
					Buffer.resize(Size);

					if (!Callback((char*)Buffer.c_str(), Size * sizeof(char)))
					{
						TH_ERR("key value data is undefined");
						return false;
					}

					Current->Value = Var::String(Buffer);
					break;
				}
				case VarType::Base64:
				{
					uint64_t Size = 0;
					if (!Callback((char*)&Size, sizeof(uint64_t)))
					{
						TH_ERR("key value size is undefined");
						return false;
					}

					std::string Buffer;
					Buffer.resize(Size);

					if (!Callback((char*)Buffer.c_str(), Size * sizeof(char)))
					{
						TH_ERR("key value data is undefined");
						return false;
					}

					Current->Value = Var::Base64(Buffer);
					break;
				}
				case VarType::Integer:
				{
					int64_t Integer = 0;
					if (!Callback((char*)&Integer, sizeof(int64_t)))
					{
						TH_ERR("key value is undefined");
						return false;
					}

					Current->Value = Var::Integer(Integer);
					break;
				}
				case VarType::Number:
				{
					double Number = 0.0;
					if (!Callback((char*)&Number, sizeof(double)))
					{
						TH_ERR("key value is undefined");
						return false;
					}

					Current->Value = Var::Number(Number);
					break;
				}
				case VarType::Decimal:
				{
					uint64_t Size = 0;
					if (!Callback((char*)&Size, sizeof(uint64_t)))
					{
						TH_ERR("key value size is undefined");
						return false;
					}

					std::string Buffer;
					Buffer.resize(Size);

					if (!Callback((char*)Buffer.c_str(), Size * sizeof(char)))
					{
						TH_ERR("key value data is undefined");
						return false;
					}

					Current->Value = Var::Decimal(Buffer);
					break;
				}
				case VarType::Boolean:
				{
					bool Boolean = false;
					if (!Callback((char*)&Boolean, sizeof(bool)))
					{
						TH_ERR("key value is undefined");
						return false;
					}

					Current->Value = Var::Boolean(Boolean);
					break;
				}
				default:
					break;
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
	}
}
