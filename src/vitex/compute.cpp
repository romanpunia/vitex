#include "compute.h"
#include "vitex.h"
#include <cctype>
#include <random>
#include <sstream>
#ifdef VI_ZLIB
extern "C"
{
#include <zlib.h>
}
#endif
#ifdef VI_MICROSOFT
#include <Windows.h>
#else
#include <time.h>
#endif
#ifdef VI_OPENSSL
extern "C"
{
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/err.h>
#include <openssl/ec.h>
#include <openssl/dsa.h>
#include <openssl/rsa.h>
}
#endif
#define REGEX_FAIL(A, B) if (A) return (B)
#define REGEX_FAIL_IN(A, B) if (A) { State = B; return; }
#define PRIVATE_KEY_SIZE (sizeof(size_t) * 8)

namespace
{
	size_t OffsetOf64(const char* Source, char Dest)
	{
		VI_ASSERT(Source != nullptr, "source should be set");
		for (size_t i = 0; i < 64; i++)
		{
			if (Source[i] == Dest)
				return i;
		}

		return 63;
	}
	Vitex::Core::String EscapeText(const Vitex::Core::String& Data)
	{
		Vitex::Core::String Result = "\"";
		Result.append(Data).append("\"");
		return Result;
	}
}

namespace Vitex
{
	namespace Compute
	{
		RegexSource::RegexSource() noexcept :
			MaxBranches(128), MaxBrackets(128), MaxMatches(128),
			State(RegexState::No_Match), IgnoreCase(false)
		{
		}
		RegexSource::RegexSource(const std::string_view& Regexp, bool fIgnoreCase, int64_t fMaxMatches, int64_t fMaxBranches, int64_t fMaxBrackets) noexcept :
			Expression(Regexp),
			MaxBranches(fMaxBranches >= 1 ? fMaxBranches : 128),
			MaxBrackets(fMaxBrackets >= 1 ? fMaxBrackets : 128),
			MaxMatches(fMaxMatches >= 1 ? fMaxMatches : 128),
			State(RegexState::Preprocessed), IgnoreCase(fIgnoreCase)
		{
			Compile();
		}
		RegexSource::RegexSource(const RegexSource& Other) noexcept :
			Expression(Other.Expression),
			MaxBranches(Other.MaxBranches),
			MaxBrackets(Other.MaxBrackets),
			MaxMatches(Other.MaxMatches),
			State(Other.State), IgnoreCase(Other.IgnoreCase)
		{
			Compile();
		}
		RegexSource::RegexSource(RegexSource&& Other) noexcept :
			Expression(std::move(Other.Expression)),
			MaxBranches(Other.MaxBranches),
			MaxBrackets(Other.MaxBrackets),
			MaxMatches(Other.MaxMatches),
			State(Other.State), IgnoreCase(Other.IgnoreCase)
		{
			Brackets.reserve(Other.Brackets.capacity());
			Branches.reserve(Other.Branches.capacity());
			Compile();
		}
		RegexSource& RegexSource::operator=(const RegexSource& V) noexcept
		{
			VI_ASSERT(this != &V, "cannot set to self");
			Brackets.clear();
			Brackets.reserve(V.Brackets.capacity());
			Branches.clear();
			Branches.reserve(V.Branches.capacity());
			Expression = V.Expression;
			IgnoreCase = V.IgnoreCase;
			State = V.State;
			MaxBrackets = V.MaxBrackets;
			MaxBranches = V.MaxBranches;
			MaxMatches = V.MaxMatches;
			Compile();

			return *this;
		}
		RegexSource& RegexSource::operator=(RegexSource&& V) noexcept
		{
			VI_ASSERT(this != &V, "cannot set to self");
			Brackets.clear();
			Brackets.reserve(V.Brackets.capacity());
			Branches.clear();
			Branches.reserve(V.Branches.capacity());
			Expression = std::move(V.Expression);
			IgnoreCase = V.IgnoreCase;
			State = V.State;
			MaxBrackets = V.MaxBrackets;
			MaxBranches = V.MaxBranches;
			MaxMatches = V.MaxMatches;
			Compile();

			return *this;
		}
		const Core::String& RegexSource::GetRegex() const
		{
			return Expression;
		}
		int64_t RegexSource::GetMaxBranches() const
		{
			return MaxBranches;
		}
		int64_t RegexSource::GetMaxBrackets() const
		{
			return MaxBrackets;
		}
		int64_t RegexSource::GetMaxMatches() const
		{
			return MaxMatches;
		}
		int64_t RegexSource::GetComplexity() const
		{
			int64_t A = (int64_t)Branches.size() + 1;
			int64_t B = (int64_t)Brackets.size();
			int64_t C = 0;

			for (size_t i = 0; i < (size_t)B; i++)
				C += Brackets[i].Length;

			return (int64_t)Expression.size() + A * B * C;
		}
		RegexState RegexSource::GetState() const
		{
			return State;
		}
		bool RegexSource::IsSimple() const
		{
			return !Core::Stringify::FindOf(Expression, "\\+*?|[]").Found;
		}
		void RegexSource::Compile()
		{
			const char* vPtr = Expression.c_str();
			int64_t vSize = (int64_t)Expression.size();
			Brackets.reserve(8);
			Branches.reserve(8);

			RegexBracket Bracket;
			Bracket.Pointer = vPtr;
			Bracket.Length = vSize;
			Brackets.push_back(Bracket);

			int64_t Step = 0, Depth = 0;
			for (int64_t i = 0; i < vSize; i += Step)
			{
				Step = Regex::GetOpLength(vPtr + i, vSize - i);
				if (vPtr[i] == '|')
				{
					RegexBranch Branch;
					Branch.BracketIndex = (Brackets.back().Length == -1 ? Brackets.size() - 1 : Depth);
					Branch.Pointer = &vPtr[i];
					Branches.push_back(Branch);
				}
				else if (vPtr[i] == '\\')
				{
					REGEX_FAIL_IN(i >= vSize - 1, RegexState::Invalid_Metacharacter);
					if (vPtr[i + 1] == 'x')
					{
						REGEX_FAIL_IN(i >= vSize - 3, RegexState::Invalid_Metacharacter);
						REGEX_FAIL_IN(!(isxdigit(vPtr[i + 2]) && isxdigit(vPtr[i + 3])), RegexState::Invalid_Metacharacter);
					}
					else
					{
						REGEX_FAIL_IN(!Regex::Meta((const uint8_t*)vPtr + i + 1), RegexState::Invalid_Metacharacter);
					}
				}
				else if (vPtr[i] == '(')
				{
					Depth++;
					Bracket.Pointer = vPtr + i + 1;
					Bracket.Length = -1;
					Brackets.push_back(Bracket);
				}
				else if (vPtr[i] == ')')
				{
					int64_t Idx = (Brackets[Brackets.size() - 1].Length == -1 ? Brackets.size() - 1 : Depth);
					Brackets[(size_t)Idx].Length = (int64_t)(&vPtr[i] - Brackets[(size_t)Idx].Pointer); Depth--;
					REGEX_FAIL_IN(Depth < 0, RegexState::Unbalanced_Brackets);
					REGEX_FAIL_IN(i > 0 && vPtr[i - 1] == '(', RegexState::No_Match);
				}
			}

			REGEX_FAIL_IN(Depth != 0, RegexState::Unbalanced_Brackets);

			RegexBranch Branch; size_t i, j;
			for (i = 0; i < Branches.size(); i++)
			{
				for (j = i + 1; j < Branches.size(); j++)
				{
					if (Branches[i].BracketIndex > Branches[j].BracketIndex)
					{
						Branch = Branches[i];
						Branches[i] = Branches[j];
						Branches[j] = Branch;
					}
				}
			}

			for (i = j = 0; i < Brackets.size(); i++)
			{
				auto& Bracket = Brackets[i];
				Bracket.BranchesCount = 0;
				Bracket.Branches = j;

				while (j < Branches.size() && Branches[j].BracketIndex == i)
				{
					Bracket.BranchesCount++;
					j++;
				}
			}
		}

		RegexResult::RegexResult() noexcept : State(RegexState::No_Match), Steps(0), Src(nullptr)
		{
		}
		RegexResult::RegexResult(const RegexResult& Other) noexcept : Matches(Other.Matches), State(Other.State), Steps(Other.Steps), Src(Other.Src)
		{
		}
		RegexResult::RegexResult(RegexResult&& Other) noexcept : Matches(std::move(Other.Matches)), State(Other.State), Steps(Other.Steps), Src(Other.Src)
		{
		}
		RegexResult& RegexResult::operator =(const RegexResult& V) noexcept
		{
			Matches = V.Matches;
			Src = V.Src;
			Steps = V.Steps;
			State = V.State;

			return *this;
		}
		RegexResult& RegexResult::operator =(RegexResult&& V) noexcept
		{
			Matches.swap(V.Matches);
			Src = V.Src;
			Steps = V.Steps;
			State = V.State;

			return *this;
		}
		bool RegexResult::Empty() const
		{
			return Matches.empty();
		}
		int64_t RegexResult::GetSteps() const
		{
			return Steps;
		}
		RegexState RegexResult::GetState() const
		{
			return State;
		}
		const Core::Vector<RegexMatch>& RegexResult::Get() const
		{
			return Matches;
		}
		Core::Vector<Core::String> RegexResult::ToArray() const
		{
			Core::Vector<Core::String> Array;
			Array.reserve(Matches.size());

			for (auto& Item : Matches)
				Array.emplace_back(Item.Pointer, (size_t)Item.Length);

			return Array;
		}

		bool Regex::Match(RegexSource* Value, RegexResult& Result, const std::string_view& Buffer)
		{
			VI_ASSERT(Value != nullptr && Value->State == RegexState::Preprocessed, "invalid regex source");
			VI_ASSERT(!Buffer.empty(), "invalid buffer");

			Result.Src = Value;
			Result.State = RegexState::Preprocessed;
			Result.Matches.clear();
			Result.Matches.reserve(8);

			int64_t Code = Parse(Buffer.data(), Buffer.size(), &Result);
			if (Code <= 0)
			{
				Result.State = (RegexState)Code;
				Result.Matches.clear();
				return false;
			}

			for (auto It = Result.Matches.begin(); It != Result.Matches.end(); ++It)
			{
				It->Start = It->Pointer - Buffer.data();
				It->End = It->Start + It->Length;
			}

			Result.State = RegexState::Match_Found;
			return true;
		}
		bool Regex::Replace(RegexSource* Value, const std::string_view& To, Core::String& Buffer)
		{
			Core::String Emplace;
			RegexResult Result;
			size_t Matches = 0;

			bool Expression = (!To.empty() && To.find('$') != Core::String::npos);
			if (!Expression)
				Emplace.assign(To);

			size_t Start = 0;
			while (Match(Value, Result, std::string_view(Buffer.c_str() + Start, Buffer.size() - Start)))
			{
				Matches++;
				if (Result.Matches.empty())
					continue;

				if (Expression)
				{
					Emplace.assign(To);
					for (size_t i = 1; i < Result.Matches.size(); i++)
					{
						auto& Item = Result.Matches[i];
						Core::Stringify::Replace(Emplace, "$" + Core::ToString(i), Core::String(Item.Pointer, (size_t)Item.Length));
					}
				}

				auto& Where = Result.Matches.front();
				Core::Stringify::ReplacePart(Buffer, (size_t)Where.Start + Start, (size_t)Where.End + Start, Emplace);
				Start += (size_t)Where.Start + (size_t)Emplace.size() - (Emplace.empty() ? 0 : 1);
			}

			return Matches > 0;
		}
		int64_t Regex::Meta(const uint8_t* Buffer)
		{
			VI_ASSERT(Buffer != nullptr, "invalid buffer");
			static const char* Chars = "^$().[]*+?|\\Ssdbfnrtv";
			return strchr(Chars, *Buffer) != nullptr;
		}
		int64_t Regex::OpLength(const char* Value)
		{
			VI_ASSERT(Value != nullptr, "invalid value");
			return Value[0] == '\\' && Value[1] == 'x' ? 4 : Value[0] == '\\' ? 2 : 1;
		}
		int64_t Regex::SetLength(const char* Value, int64_t ValueLength)
		{
			VI_ASSERT(Value != nullptr, "invalid value");
			int64_t Length = 0;
			while (Length < ValueLength && Value[Length] != ']')
				Length += OpLength(Value + Length);

			return Length <= ValueLength ? Length + 1 : -1;
		}
		int64_t Regex::GetOpLength(const char* Value, int64_t ValueLength)
		{
			VI_ASSERT(Value != nullptr, "invalid value");
			return Value[0] == '[' ? SetLength(Value + 1, ValueLength - 1) + 1 : OpLength(Value);
		}
		int64_t Regex::Quantifier(const char* Value)
		{
			VI_ASSERT(Value != nullptr, "invalid value");
			return Value[0] == '*' || Value[0] == '+' || Value[0] == '?';
		}
		int64_t Regex::ToInt(int64_t x)
		{
			return (int64_t)(isdigit((int)x) ? (int)x - '0' : (int)x - 'W');
		}
		int64_t Regex::HexToInt(const uint8_t* Buffer)
		{
			VI_ASSERT(Buffer != nullptr, "invalid buffer");
			return (ToInt(tolower(Buffer[0])) << 4) | ToInt(tolower(Buffer[1]));
		}
		int64_t Regex::MatchOp(const uint8_t* Value, const uint8_t* Buffer, RegexResult* Info)
		{
			VI_ASSERT(Buffer != nullptr, "invalid buffer");
			VI_ASSERT(Value != nullptr, "invalid value");
			VI_ASSERT(Info != nullptr, "invalid regex result");

			int64_t Result = 0;
			switch (*Value)
			{
				case '\\':
					switch (Value[1])
					{
						case 'S':
							REGEX_FAIL(isspace(*Buffer), (int64_t)RegexState::No_Match);
							Result++;
							break;
						case 's':
							REGEX_FAIL(!isspace(*Buffer), (int64_t)RegexState::No_Match);
							Result++;
							break;
						case 'd':
							REGEX_FAIL(!isdigit(*Buffer), (int64_t)RegexState::No_Match);
							Result++;
							break;
						case 'b':
							REGEX_FAIL(*Buffer != '\b', (int64_t)RegexState::No_Match);
							Result++;
							break;
						case 'f':
							REGEX_FAIL(*Buffer != '\f', (int64_t)RegexState::No_Match);
							Result++;
							break;
						case 'n':
							REGEX_FAIL(*Buffer != '\n', (int64_t)RegexState::No_Match);
							Result++;
							break;
						case 'r':
							REGEX_FAIL(*Buffer != '\r', (int64_t)RegexState::No_Match);
							Result++;
							break;
						case 't':
							REGEX_FAIL(*Buffer != '\t', (int64_t)RegexState::No_Match);
							Result++;
							break;
						case 'v':
							REGEX_FAIL(*Buffer != '\v', (int64_t)RegexState::No_Match);
							Result++;
							break;
						case 'x':
							REGEX_FAIL((uint8_t)HexToInt(Value + 2) != *Buffer, (int64_t)RegexState::No_Match);
							Result++;
							break;
						default:
							REGEX_FAIL(Value[1] != Buffer[0], (int64_t)RegexState::No_Match);
							Result++;
							break;
					}
					break;
				case '|':
					REGEX_FAIL(1, (int64_t)RegexState::Internal_Error);
					break;
				case '$':
					REGEX_FAIL(1, (int64_t)RegexState::No_Match);
					break;
				case '.':
					Result++;
					break;
				default:
					if (Info->Src->IgnoreCase)
					{
						REGEX_FAIL(tolower(*Value) != tolower(*Buffer), (int64_t)RegexState::No_Match);
					}
					else
					{
						REGEX_FAIL(*Value != *Buffer, (int64_t)RegexState::No_Match);
					}
					Result++;
					break;
			}

			return Result;
		}
		int64_t Regex::MatchSet(const char* Value, int64_t ValueLength, const char* Buffer, RegexResult* Info)
		{
			VI_ASSERT(Buffer != nullptr, "invalid buffer");
			VI_ASSERT(Value != nullptr, "invalid value");
			VI_ASSERT(Info != nullptr, "invalid regex result");

			int64_t Length = 0, Result = -1, Invert = Value[0] == '^';
			if (Invert)
				Value++, ValueLength--;

			while (Length <= ValueLength && Value[Length] != ']' && Result <= 0)
			{
				if (Value[Length] != '-' && Value[Length + 1] == '-' && Value[Length + 2] != ']' && Value[Length + 2] != '\0')
				{
					Result = (Info->Src->IgnoreCase) ? tolower(*Buffer) >= tolower(Value[Length]) && tolower(*Buffer) <= tolower(Value[Length + 2]) : *Buffer >= Value[Length] && *Buffer <= Value[Length + 2];
					Length += 3;
				}
				else
				{
					Result = MatchOp((const uint8_t*)Value + Length, (const uint8_t*)Buffer, Info);
					Length += OpLength(Value + Length);
				}
			}

			return (!Invert && Result > 0) || (Invert && Result <= 0) ? 1 : -1;
		}
		int64_t Regex::ParseInner(const char* Value, int64_t ValueLength, const char* Buffer, int64_t BufferLength, RegexResult* Info, int64_t Case)
		{
			VI_ASSERT(Buffer != nullptr, "invalid buffer");
			VI_ASSERT(Value != nullptr, "invalid value");
			VI_ASSERT(Info != nullptr, "invalid regex result");

			int64_t i, j, n, Step;
			for (i = j = 0; i < ValueLength && j <= BufferLength; i += Step)
			{
				Step = Value[i] == '(' ? Info->Src->Brackets[(size_t)Case + 1].Length + 2 : GetOpLength(Value + i, ValueLength - i);
				REGEX_FAIL(Quantifier(&Value[i]), (int64_t)RegexState::Unexpected_Quantifier);
				REGEX_FAIL(Step <= 0, (int64_t)RegexState::Invalid_Character_Set);
				Info->Steps++;

				if (i + Step < ValueLength && Quantifier(Value + i + Step))
				{
					if (Value[i + Step] == '?')
					{
						int64_t result = ParseInner(Value + i, Step, Buffer + j, BufferLength - j, Info, Case);
						j += result > 0 ? result : 0;
						i++;
					}
					else if (Value[i + Step] == '+' || Value[i + Step] == '*')
					{
						int64_t j2 = j, nj = j, n1, n2 = -1, ni, non_greedy = 0;
						ni = i + Step + 1;
						if (ni < ValueLength && Value[ni] == '?')
						{
							non_greedy = 1;
							ni++;
						}

						do
						{
							if ((n1 = ParseInner(Value + i, Step, Buffer + j2, BufferLength - j2, Info, Case)) > 0)
								j2 += n1;

							if (Value[i + Step] == '+' && n1 < 0)
								break;

							if (ni >= ValueLength)
								nj = j2;
							else if ((n2 = ParseInner(Value + ni, ValueLength - ni, Buffer + j2, BufferLength - j2, Info, Case)) >= 0)
								nj = j2 + n2;

							if (nj > j && non_greedy)
								break;
						} while (n1 > 0);

						if (n1 < 0 && n2 < 0 && Value[i + Step] == '*' && (n2 = ParseInner(Value + ni, ValueLength - ni, Buffer + j, BufferLength - j, Info, Case)) > 0)
							nj = j + n2;

						REGEX_FAIL(Value[i + Step] == '+' && nj == j, (int64_t)RegexState::No_Match);
						REGEX_FAIL(nj == j && ni < ValueLength&& n2 < 0, (int64_t)RegexState::No_Match);
						return nj;
					}

					continue;
				}

				if (Value[i] == '[')
				{
					n = MatchSet(Value + i + 1, ValueLength - (i + 2), Buffer + j, Info);
					REGEX_FAIL(n <= 0, (int64_t)RegexState::No_Match);
					j += n;
				}
				else if (Value[i] == '(')
				{
					n = (int64_t)RegexState::No_Match;
					Case++;

					REGEX_FAIL(Case >= (int64_t)Info->Src->Brackets.size(), (int64_t)RegexState::Internal_Error);
					if (ValueLength - (i + Step) > 0)
					{
						int64_t j2;
						for (j2 = 0; j2 <= BufferLength - j; j2++)
						{
							if ((n = ParseDOH(Buffer + j, BufferLength - (j + j2), Info, Case)) >= 0 && ParseInner(Value + i + Step, ValueLength - (i + Step), Buffer + j + n, BufferLength - (j + n), Info, Case) >= 0)
								break;
						}
					}
					else
						n = ParseDOH(Buffer + j, BufferLength - j, Info, Case);

					REGEX_FAIL(n < 0, n);
					if (n > 0)
					{
						RegexMatch* Match;
						if (Case - 1 >= (int64_t)Info->Matches.size())
						{
							Info->Matches.emplace_back();
							Match = &Info->Matches[Info->Matches.size() - 1];
						}
						else
							Match = &Info->Matches.at((size_t)Case - 1);

						Match->Pointer = Buffer + j;
						Match->Length = n;
						Match->Steps++;
					}
					j += n;
				}
				else if (Value[i] == '^')
				{
					REGEX_FAIL(j != 0, (int64_t)RegexState::No_Match);
				}
				else if (Value[i] == '$')
				{
					REGEX_FAIL(j != BufferLength, (int64_t)RegexState::No_Match);
				}
				else
				{
					REGEX_FAIL(j >= BufferLength, (int64_t)RegexState::No_Match);
					n = MatchOp((const uint8_t*)(Value + i), (const uint8_t*)(Buffer + j), Info);
					REGEX_FAIL(n <= 0, n);
					j += n;
				}
			}

			return j;
		}
		int64_t Regex::ParseDOH(const char* Buffer, int64_t BufferLength, RegexResult* Info, int64_t Case)
		{
			VI_ASSERT(Buffer != nullptr, "invalid buffer");
			VI_ASSERT(Info != nullptr, "invalid regex result");

			const RegexBracket* Bk = &Info->Src->Brackets[(size_t)Case];
			int64_t i = 0, Length, Result;
			const char* Ptr;

			do
			{
				Ptr = i == 0 ? Bk->Pointer : Info->Src->Branches[(size_t)(Bk->Branches + i - 1)].Pointer + 1;
				Length = Bk->BranchesCount == 0 ? Bk->Length : i == Bk->BranchesCount ? (int64_t)(Bk->Pointer + Bk->Length - Ptr) : (int64_t)(Info->Src->Branches[(size_t)(Bk->Branches + i)].Pointer - Ptr);
				Result = ParseInner(Ptr, Length, Buffer, BufferLength, Info, Case);
				VI_MEASURE_LOOP();
			} while (Result <= 0 && i++ < Bk->BranchesCount);

			return Result;
		}
		int64_t Regex::Parse(const char* Buffer, int64_t BufferLength, RegexResult* Info)
		{
			VI_ASSERT(Buffer != nullptr, "invalid buffer");
			VI_ASSERT(Info != nullptr, "invalid regex result");
			VI_MEASURE(Core::Timings::Frame);

			int64_t is_anchored = Info->Src->Brackets[0].Pointer[0] == '^', i, result = -1;
			for (i = 0; i <= BufferLength; i++)
			{
				result = ParseDOH(Buffer + i, BufferLength - i, Info, 0);
				if (result >= 0)
				{
					result += i;
					break;
				}

				if (is_anchored)
					break;
			}

			return result;
		}
		const char* Regex::Syntax()
		{
			return
				"\"^\" - Match beginning of a buffer\n"
				"\"$\" - Match end of a buffer\n"
				"\"()\" - Grouping and substring capturing\n"
				"\"\\s\" - Match whitespace\n"
				"\"\\S\" - Match non - whitespace\n"
				"\"\\d\" - Match decimal digit\n"
				"\"\\n\" - Match new line character\n"
				"\"\\r\" - Match line feed character\n"
				"\"\\f\" - Match form feed character\n"
				"\"\\v\" - Match vertical tab character\n"
				"\"\\t\" - Match horizontal tab character\n"
				"\"\\b\" - Match backspace character\n"
				"\"+\" - Match one or more times (greedy)\n"
				"\"+?\" - Match one or more times (non - greedy)\n"
				"\"*\" - Match zero or more times (greedy)\n"
				"\"*?\" - Match zero or more times(non - greedy)\n"
				"\"?\" - Match zero or once(non - greedy)\n"
				"\"x|y\" - Match x or y(alternation operator)\n"
				"\"\\meta\" - Match one of the meta character: ^$().[]*+?|\\\n"
				"\"\\xHH\" - Match byte with hex value 0xHH, e.g. \\x4a\n"
				"\"[...]\" - Match any character from set. Ranges like[a-z] are supported\n"
				"\"[^...]\" - Match any character but ones from set\n";
		}

		PrivateKey::PrivateKey() noexcept
		{
			VI_TRACE("[crypto] create empty private key");
		}
		PrivateKey::PrivateKey(Core::String&& Text, bool) noexcept : Plain(std::move(Text))
		{
			VI_TRACE("[crypto] create plain private key on %" PRIu64 " bytes", (uint64_t)Plain.size());
		}
		PrivateKey::PrivateKey(const std::string_view& Text, bool) noexcept : Plain(Text)
		{
			VI_TRACE("[crypto] create plain private key on %" PRIu64 " bytes", (uint64_t)Plain.size());
		}
		PrivateKey::PrivateKey(const std::string_view& Key) noexcept
		{
			Secure(Key);
		}
		PrivateKey::PrivateKey(const PrivateKey& Other) noexcept
		{
			CopyDistribution(Other);
		}
		PrivateKey::PrivateKey(PrivateKey&& Other) noexcept : Blocks(std::move(Other.Blocks)), Plain(std::move(Other.Plain))
		{
		}
		PrivateKey::~PrivateKey() noexcept
		{
			Clear();
		}
		PrivateKey& PrivateKey::operator =(const PrivateKey& V) noexcept
		{
			CopyDistribution(V);
			return *this;
		}
		PrivateKey& PrivateKey::operator =(PrivateKey&& V) noexcept
		{
			Clear();
			Blocks = std::move(V.Blocks);
			Plain = std::move(V.Plain);
			return *this;
		}
		void PrivateKey::Clear()
		{
			size_t Size = Blocks.size();
			for (size_t i = 0; i < Size; i++)
			{
				size_t* Partition = (size_t*)Blocks[i];
				RollPartition(Partition, Size, i);
				Core::Memory::Deallocate(Partition);
			}
			Blocks.clear();
			Plain.clear();
		}
		void PrivateKey::Secure(const std::string_view& Key)
		{
			VI_TRACE("[crypto] secure private key on %" PRIu64 " bytes", (uint64_t)Key.size());
			Blocks.reserve(Key.size());
			Clear();

			for (size_t i = 0; i < Key.size(); i++)
			{
				size_t* Partition = Core::Memory::Allocate<size_t>(PRIVATE_KEY_SIZE);
				FillPartition(Partition, Key.size(), i, Key[i]);
				Blocks.emplace_back(Partition);
			}
		}
		void PrivateKey::ExposeToStack(char* Buffer, size_t MaxSize, size_t* OutSize) const
		{
			VI_TRACE("[crypto] stack expose private key to 0x%" PRIXPTR, (void*)Buffer);
			size_t Size;
			if (Plain.empty())
			{
				Size = (--MaxSize > Blocks.size() ? Blocks.size() : MaxSize);
				for (size_t i = 0; i < Size; i++)
					Buffer[i] = LoadPartition((size_t*)Blocks[i], Size, i);
			}
			else
			{
				Size = (--MaxSize > Plain.size() ? Plain.size() : MaxSize);
				memcpy(Buffer, Plain.data(), sizeof(char) * Size);
			}

			Buffer[Size] = '\0';
			if (OutSize != nullptr)
				*OutSize = Size;
		}
		Core::String PrivateKey::ExposeToHeap() const
		{
			VI_TRACE("[crypto] heap expose private key from 0x%" PRIXPTR, (void*)this);
			Core::String Result = Plain;
			if (Result.empty())
			{
				Result.resize(Blocks.size());
				ExposeToStack((char*)Result.data(), Result.size() + 1);
			}
			return Result;
		}
		void PrivateKey::CopyDistribution(const PrivateKey& Other)
		{
			VI_TRACE("[crypto] copy private key from 0x%" PRIXPTR, (void*)&Other);
			Clear();
			if (Other.Plain.empty())
			{
				Blocks.reserve(Other.Blocks.size());
				for (auto* Partition : Other.Blocks)
				{
					void* CopiedPartition = Core::Memory::Allocate<void>(PRIVATE_KEY_SIZE);
					memcpy(CopiedPartition, Partition, PRIVATE_KEY_SIZE);
					Blocks.emplace_back(CopiedPartition);
				}
			}
			else
				Plain = Other.Plain;
		}
		size_t PrivateKey::Size() const
		{
			return Blocks.size();
		}
		char PrivateKey::LoadPartition(size_t* Dest, size_t Size, size_t Index) const
		{
			char* Buffer = (char*)Dest + sizeof(size_t);
			return Buffer[Dest[0]];
		}
		void PrivateKey::RollPartition(size_t* Dest, size_t Size, size_t Index) const
		{
			char* Buffer = (char*)Dest + sizeof(size_t);
			Buffer[Dest[0]] = (char)(Crypto::Random() % std::numeric_limits<uint8_t>::max());
		}
		void PrivateKey::FillPartition(size_t* Dest, size_t Size, size_t Index, char Source) const
		{
			Dest[0] = ((size_t(Source) << 16) + 17) % (PRIVATE_KEY_SIZE - sizeof(size_t));
			char* Buffer = (char*)Dest + sizeof(size_t);
			for (size_t i = 0; i < PRIVATE_KEY_SIZE - sizeof(size_t); i++)
				Buffer[i] = (char)(Crypto::Random() % std::numeric_limits<uint8_t>::max());
			Buffer[Dest[0]] = Source;
		}
		void PrivateKey::RandomizeBuffer(char* Buffer, size_t Size)
		{
			for (size_t i = 0; i < Size; i++)
				Buffer[i] = Crypto::Random() % std::numeric_limits<char>::max();
		}
		PrivateKey PrivateKey::GetPlain(Core::String&& Value)
		{
			PrivateKey Key = PrivateKey(std::move(Value), true);
			return Key;
		}
		PrivateKey PrivateKey::GetPlain(const std::string_view& Value)
		{
			PrivateKey Key = PrivateKey(Value, true);
			return Key;
		}

		UInt128::UInt128(const std::string_view& Text) : UInt128(Text, 10)
		{
		}
		UInt128::UInt128(const std::string_view& Text, uint8_t Base)
		{
			if (Text.empty())
			{
				Lower = Upper = 0;
				return;
			}

			size_t Size = Text.size();
			char* Data = (char*)Text.data();
			while (Size > 0 && Core::Stringify::IsWhitespace(*Data))
			{
				++Data;
				Size--;
			}

			switch (Base)
			{
				case 16:
				{
					static const size_t MAX_LEN = 32;
					const size_t max_len = std::min(Size, MAX_LEN);
					const size_t starting_index = (MAX_LEN < Size) ? (Size - MAX_LEN) : 0;
					const size_t double_lower = sizeof(Lower) * 2;
					const size_t lower_len = (max_len >= double_lower) ? double_lower : max_len;
					const size_t upper_len = (max_len >= double_lower) ? (max_len - double_lower) : 0;

					std::stringstream lower_s, upper_s;
					upper_s << std::hex << Core::String(Data + starting_index, upper_len);
					lower_s << std::hex << Core::String(Data + starting_index + upper_len, lower_len);
					upper_s >> Upper;
					lower_s >> Lower;
					break;
				}
				case 10:
				{
					static const size_t MAX_LEN = 39;
					const size_t max_len = std::min(Size, MAX_LEN);
					const size_t starting_index = (MAX_LEN < Size) ? (Size - MAX_LEN) : 0;
					Data += starting_index;

					for (size_t i = 0; *Data && ('0' <= *Data) && (*Data <= '9') && (i < max_len); ++Data, ++i)
					{
						*this *= 10;
						*this += *Data - '0';
					}
					break;
				}
				case 8:
				{
					static const size_t MAX_LEN = 43;
					const size_t max_len = std::min(Size, MAX_LEN);
					const size_t starting_index = (MAX_LEN < Size) ? (Size - MAX_LEN) : 0;
					Data += starting_index;

					for (size_t i = 0; *Data && ('0' <= *Data) && (*Data <= '7') && (i < max_len); ++Data, ++i)
					{
						*this *= 8;
						*this += *Data - '0';
					}
					break;
				}
				case 2:
				{
					static const size_t MAX_LEN = 128;
					const size_t max_len = std::min(Size, MAX_LEN);
					const size_t starting_index = (MAX_LEN < Size) ? (Size - MAX_LEN) : 0;
					const size_t eight_lower = sizeof(Lower) * 8;
					const size_t lower_len = (max_len >= eight_lower) ? eight_lower : max_len;
					const size_t upper_len = (max_len >= eight_lower) ? (max_len - eight_lower) : 0;
					Data += starting_index;

					for (size_t i = 0; *Data && ('0' <= *Data) && (*Data <= '1') && (i < upper_len); ++Data, ++i)
					{
						Upper <<= 1;
						Upper |= *Data - '0';
					}

					for (size_t i = 0; *Data && ('0' <= *Data) && (*Data <= '1') && (i < lower_len); ++Data, ++i)
					{
						Lower <<= 1;
						Lower |= *Data - '0';
					}
					break;
				}
				default:
					VI_ASSERT(false, "invalid from string base: %i", (int)Base);
					break;
			}
		}
		UInt128::operator bool() const
		{
			return (bool)(Upper || Lower);
		}
		UInt128::operator uint8_t() const
		{
			return (uint8_t)Lower;
		}
		UInt128::operator uint16_t() const
		{
			return (uint16_t)Lower;
		}
		UInt128::operator uint32_t() const
		{
			return (uint32_t)Lower;
		}
		UInt128::operator uint64_t() const
		{
			return (uint64_t)Lower;
		}
		UInt128 UInt128::operator&(const UInt128& Right) const
		{
			return UInt128(Upper & Right.Upper, Lower & Right.Lower);
		}
		UInt128& UInt128::operator&=(const UInt128& Right)
		{
			Upper &= Right.Upper;
			Lower &= Right.Lower;
			return *this;
		}
		UInt128 UInt128::operator|(const UInt128& Right) const
		{
			return UInt128(Upper | Right.Upper, Lower | Right.Lower);
		}
		UInt128& UInt128::operator|=(const UInt128& Right)
		{
			Upper |= Right.Upper;
			Lower |= Right.Lower;
			return *this;
		}
		UInt128 UInt128::operator^(const UInt128& Right) const
		{
			return UInt128(Upper ^ Right.Upper, Lower ^ Right.Lower);
		}
		UInt128& UInt128::operator^=(const UInt128& Right)
		{
			Upper ^= Right.Upper;
			Lower ^= Right.Lower;
			return *this;
		}
		UInt128 UInt128::operator~() const
		{
			return UInt128(~Upper, ~Lower);
		}
		UInt128 UInt128::operator<<(const UInt128& Right) const
		{
			const uint64_t shift = Right.Lower;
			if (((bool)Right.Upper) || (shift >= 128))
				return UInt128(0);
			else if (shift == 64)
				return UInt128(Lower, 0);
			else if (shift == 0)
				return *this;
			else if (shift < 64)
				return UInt128((Upper << shift) + (Lower >> (64 - shift)), Lower << shift);
			else if ((128 > shift) && (shift > 64))
				return UInt128(Lower << (shift - 64), 0);

			return UInt128(0);
		}
		UInt128& UInt128::operator<<=(const UInt128& Right)
		{
			*this = *this << Right;
			return *this;
		}
		UInt128 UInt128::operator>>(const UInt128& Right) const
		{
			const uint64_t shift = Right.Lower;
			if (((bool)Right.Upper) || (shift >= 128))
				return UInt128(0);
			else if (shift == 64)
				return UInt128(0, Upper);
			else if (shift == 0)
				return *this;
			else if (shift < 64)
				return UInt128(Upper >> shift, (Upper << (64 - shift)) + (Lower >> shift));
			else if ((128 > shift) && (shift > 64))
				return UInt128(0, (Upper >> (shift - 64)));

			return UInt128(0);
		}
		UInt128& UInt128::operator>>=(const UInt128& Right)
		{
			*this = *this >> Right;
			return *this;
		}
		bool UInt128::operator!() const
		{
			return !(bool)(Upper || Lower);
		}
		bool UInt128::operator&&(const UInt128& Right) const
		{
			return ((bool)*this && Right);
		}
		bool UInt128::operator||(const UInt128& Right) const
		{
			return ((bool)*this || Right);
		}
		bool UInt128::operator==(const UInt128& Right) const
		{
			return ((Upper == Right.Upper) && (Lower == Right.Lower));
		}
		bool UInt128::operator!=(const UInt128& Right) const
		{
			return ((Upper != Right.Upper) || (Lower != Right.Lower));
		}
		bool UInt128::operator>(const UInt128& Right) const
		{
			if (Upper == Right.Upper)
				return (Lower > Right.Lower);

			return (Upper > Right.Upper);
		}
		bool UInt128::operator<(const UInt128& Right) const
		{
			if (Upper == Right.Upper)
				return (Lower < Right.Lower);

			return (Upper < Right.Upper);
		}
		bool UInt128::operator>=(const UInt128& Right) const
		{
			return ((*this > Right) || (*this == Right));
		}
		bool UInt128::operator<=(const UInt128& Right) const
		{
			return ((*this < Right) || (*this == Right));
		}
		UInt128 UInt128::operator+(const UInt128& Right) const
		{
			return UInt128(Upper + Right.Upper + ((Lower + Right.Lower) < Lower), Lower + Right.Lower);
		}
		UInt128& UInt128::operator+=(const UInt128& Right)
		{
			Upper += Right.Upper + ((Lower + Right.Lower) < Lower);
			Lower += Right.Lower;
			return *this;
		}
		UInt128 UInt128::operator-(const UInt128& Right) const
		{
			return UInt128(Upper - Right.Upper - ((Lower - Right.Lower) > Lower), Lower - Right.Lower);
		}
		UInt128& UInt128::operator-=(const UInt128& Right)
		{
			*this = *this - Right;
			return *this;
		}
		UInt128 UInt128::operator*(const UInt128& Right) const
		{
			uint64_t top[4] = { Upper >> 32, Upper & 0xffffffff, Lower >> 32, Lower & 0xffffffff };
			uint64_t bottom[4] = { Right.Upper >> 32, Right.Upper & 0xffffffff, Right.Lower >> 32, Right.Lower & 0xffffffff };
			uint64_t products[4][4];

			for (int y = 3; y > -1; y--)
			{
				for (int x = 3; x > -1; x--)
					products[3 - x][y] = top[x] * bottom[y];
			}

			uint64_t fourth32 = (products[0][3] & 0xffffffff);
			uint64_t third32 = (products[0][2] & 0xffffffff) + (products[0][3] >> 32);
			uint64_t second32 = (products[0][1] & 0xffffffff) + (products[0][2] >> 32);
			uint64_t first32 = (products[0][0] & 0xffffffff) + (products[0][1] >> 32);
			third32 += (products[1][3] & 0xffffffff);
			second32 += (products[1][2] & 0xffffffff) + (products[1][3] >> 32);
			first32 += (products[1][1] & 0xffffffff) + (products[1][2] >> 32);
			second32 += (products[2][3] & 0xffffffff);
			first32 += (products[2][2] & 0xffffffff) + (products[2][3] >> 32);
			first32 += (products[3][3] & 0xffffffff);
			third32 += fourth32 >> 32;
			second32 += third32 >> 32;
			first32 += second32 >> 32;
			fourth32 &= 0xffffffff;
			third32 &= 0xffffffff;
			second32 &= 0xffffffff;
			first32 &= 0xffffffff;
			return UInt128((first32 << 32) | second32, (third32 << 32) | fourth32);
		}
		UInt128& UInt128::operator*=(const UInt128& Right)
		{
			*this = *this * Right;
			return *this;
		}
		UInt128 UInt128::Min()
		{
			static UInt128 Value = UInt128(0);
			return Value;
		}
		UInt128 UInt128::Max()
		{
			static UInt128 Value = UInt128(-1, -1);
			return Value;
		}
		std::pair<UInt128, UInt128> UInt128::Divide(const UInt128& Left, const UInt128& Right) const
		{
			UInt128 Zero(0), One(1);
			if (Right == Zero)
			{
				VI_ASSERT(false, "division or modulus by zero");
				return std::pair<UInt128, UInt128>(Zero, Zero);
			}
			else if (Right == One)
				return std::pair <UInt128, UInt128>(Left, Zero);
			else if (Left == Right)
				return std::pair <UInt128, UInt128>(One, Zero);
			else if ((Left == Zero) || (Left < Right))
				return std::pair <UInt128, UInt128>(Zero, Left);

			std::pair <UInt128, UInt128> qr(Zero, Zero);
			for (uint8_t x = Left.Bits(); x > 0; x--)
			{
				qr.first <<= One;
				qr.second <<= One;
				if ((Left >> (x - 1U)) & 1)
					++qr.second;

				if (qr.second >= Right)
				{
					qr.second -= Right;
					++qr.first;
				}
			}
			return qr;
		}
		UInt128 UInt128::operator/(const UInt128& Right) const
		{
			return Divide(*this, Right).first;
		}
		UInt128& UInt128::operator/=(const UInt128& Right)
		{
			*this = *this / Right;
			return *this;
		}
		UInt128 UInt128::operator%(const UInt128& Right) const
		{
			return Divide(*this, Right).second;
		}
		UInt128& UInt128::operator%=(const UInt128& Right)
		{
			*this = *this % Right;
			return *this;
		}
		UInt128& UInt128::operator++()
		{
			return *this += UInt128(1);
		}
		UInt128 UInt128::operator++(int)
		{
			UInt128 temp(*this);
			++*this;
			return temp;
		}
		UInt128& UInt128::operator--()
		{
			return *this -= UInt128(1);
		}
		UInt128 UInt128::operator--(int)
		{
			UInt128 temp(*this);
			--*this;
			return temp;
		}
		UInt128 UInt128::operator+() const
		{
			return *this;
		}
		UInt128 UInt128::operator-() const
		{
			return ~*this + UInt128(1);
		}
		const uint64_t& UInt128::High() const
		{
			return Upper;
		}
		const uint64_t& UInt128::Low() const
		{
			return Lower;
		}
		uint64_t& UInt128::High()
		{
			return Upper;
		}
		uint64_t& UInt128::Low()
		{
			return Lower;
		}
		uint8_t UInt128::Bits() const
		{
			uint8_t out = 0;
			if (Upper)
			{
				out = 64;
				uint64_t up = Upper;
				while (up)
				{
					up >>= 1;
					out++;
				}
			}
			else
			{
				uint64_t low = Lower;
				while (low)
				{
					low >>= 1;
					out++;
				}
			}
			return out;
		}
		uint8_t UInt128::Bytes() const
		{
			if (!*this)
				return 0;

			uint8_t Length = Bits();
			return std::max<uint8_t>(1, std::min(16, (Length - Length % 8 + 8) / 8));
		}
		Core::Decimal UInt128::ToDecimal() const
		{
			return Core::Decimal::From(ToString(16), 16);
		}
		Core::String UInt128::ToString(uint8_t Base, uint32_t Length) const
		{
			VI_ASSERT(Base >= 2 && Base <= 16, "base must be in the range [2, 16]");
			static const char* Alphabet = "0123456789abcdef";
			Core::String Output;
			if (!(*this))
			{
				if (!Length)
					Output.push_back('0');
				else if (Output.size() < Length)
					Output.append(Length - Output.size(), '0');
				return Output;
			}

			switch (Base)
			{
				case 16:
				{
					uint64_t Array[2]; size_t Size = Bytes();
					if (Size > sizeof(uint64_t) * 0)
					{
						Array[1] = Core::OS::CPU::ToEndianness(Core::OS::CPU::Endian::Big, Lower);
						if (Size > sizeof(uint64_t) * 1)
							Array[0] = Core::OS::CPU::ToEndianness(Core::OS::CPU::Endian::Big, Upper);
					}

					Output = Codec::HexEncodeOdd(std::string_view(((char*)Array) + (sizeof(Array) - Size), Size));
					if (Output.size() < Length)
						Output.append(Length - Output.size(), '0');
					break;
				}
				default:
				{
					std::pair<UInt256, UInt256> Remainder(*this, Min());
					do
					{
						Remainder = Divide(Remainder.first, Base);
						Output.push_back(Alphabet[(uint8_t)Remainder.second]);
					} while (Remainder.first);
					if (Output.size() < Length)
						Output.append(Length - Output.size(), '0');

					std::reverse(Output.begin(), Output.end());
					break;
				}
			}
			return Output;
		}
		UInt128 operator<<(const uint8_t& Left, const UInt128& Right)
		{
			return UInt128(Left) << Right;
		}
		UInt128 operator<<(const uint16_t& Left, const UInt128& Right)
		{
			return UInt128(Left) << Right;
		}
		UInt128 operator<<(const uint32_t& Left, const UInt128& Right)
		{
			return UInt128(Left) << Right;
		}
		UInt128 operator<<(const uint64_t& Left, const UInt128& Right)
		{
			return UInt128(Left) << Right;
		}
		UInt128 operator<<(const int8_t& Left, const UInt128& Right)
		{
			return UInt128(Left) << Right;
		}
		UInt128 operator<<(const int16_t& Left, const UInt128& Right)
		{
			return UInt128(Left) << Right;
		}
		UInt128 operator<<(const int32_t& Left, const UInt128& Right)
		{
			return UInt128(Left) << Right;
		}
		UInt128 operator<<(const int64_t& Left, const UInt128& Right)
		{
			return UInt128(Left) << Right;
		}
		UInt128 operator>>(const uint8_t& Left, const UInt128& Right)
		{
			return UInt128(Left) >> Right;
		}
		UInt128 operator>>(const uint16_t& Left, const UInt128& Right)
		{
			return UInt128(Left) >> Right;
		}
		UInt128 operator>>(const uint32_t& Left, const UInt128& Right)
		{
			return UInt128(Left) >> Right;
		}
		UInt128 operator>>(const uint64_t& Left, const UInt128& Right)
		{
			return UInt128(Left) >> Right;
		}
		UInt128 operator>>(const int8_t& Left, const UInt128& Right)
		{
			return UInt128(Left) >> Right;
		}
		UInt128 operator>>(const int16_t& Left, const UInt128& Right)
		{
			return UInt128(Left) >> Right;
		}
		UInt128 operator>>(const int32_t& Left, const UInt128& Right)
		{
			return UInt128(Left) >> Right;
		}
		UInt128 operator>>(const int64_t& Left, const UInt128& Right)
		{
			return UInt128(Left) >> Right;
		}
		std::ostream& operator<<(std::ostream& Stream, const UInt128& Right)
		{
			if (Stream.flags() & Stream.oct)
				Stream << Right.ToString(8);
			else if (Stream.flags() & Stream.dec)
				Stream << Right.ToString(10);
			else if (Stream.flags() & Stream.hex)
				Stream << Right.ToString(16);
			return Stream;
		}

		UInt256::UInt256(const std::string_view& Text) : UInt256(Text, 10)
		{
		}
		UInt256::UInt256(const std::string_view& Text, uint8_t Base)
		{
			*this = 0;
			UInt256 power(1);
			uint8_t digit;
			int64_t pos = (int64_t)Text.size() - 1;
			while (pos >= 0)
			{
				digit = 0;
				if ('0' <= Text[pos] && Text[pos] <= '9')
					digit = Text[pos] - '0';
				else if ('a' <= Text[pos] && Text[pos] <= 'z')
					digit = Text[pos] - 'a' + 10;
				*this += digit * power;
				power *= Base;
				pos--;
			}
		}
		UInt256 UInt256::Min()
		{
			static UInt256 Value = UInt256(0);
			return Value;
		}
		UInt256 UInt256::Max()
		{
			static UInt256 Value = UInt256(UInt128((uint64_t)-1, (uint64_t)-1), UInt128((uint64_t)-1, (uint64_t)-1));
			return Value;
		}
		UInt256::operator bool() const
		{
			return (bool)(Upper || Lower);
		}
		UInt256::operator uint8_t() const
		{
			return (uint8_t)Lower;
		}
		UInt256::operator uint16_t() const
		{
			return (uint16_t)Lower;
		}
		UInt256::operator uint32_t() const
		{
			return (uint32_t)Lower;
		}
		UInt256::operator uint64_t() const
		{
			return (uint64_t)Lower;
		}
		UInt256::operator UInt128() const
		{
			return Lower;
		}
		UInt256 UInt256::operator&(const UInt128& Right) const
		{
			return UInt256(UInt128::Min(), Lower & Right);
		}
		UInt256 UInt256::operator&(const UInt256& Right) const
		{
			return UInt256(Upper & Right.Upper, Lower & Right.Lower);
		}
		UInt256& UInt256::operator&=(const UInt128& Right)
		{
			Upper = UInt128::Min();
			Lower &= Right;
			return *this;
		}
		UInt256& UInt256::operator&=(const UInt256& Right)
		{
			Upper &= Right.Upper;
			Lower &= Right.Lower;
			return *this;
		}
		UInt256 UInt256::operator|(const UInt128& Right) const
		{
			return UInt256(Upper, Lower | Right);
		}
		UInt256 UInt256::operator|(const UInt256& Right) const
		{
			return UInt256(Upper | Right.Upper, Lower | Right.Lower);
		}
		UInt256& UInt256::operator|=(const UInt128& Right)
		{
			Lower |= Right;
			return *this;
		}
		UInt256& UInt256::operator|=(const UInt256& Right)
		{
			Upper |= Right.Upper;
			Lower |= Right.Lower;
			return *this;
		}
		UInt256 UInt256::operator^(const UInt128& Right) const
		{
			return UInt256(Upper, Lower ^ Right);
		}
		UInt256 UInt256::operator^(const UInt256& Right) const
		{
			return UInt256(Upper ^ Right.Upper, Lower ^ Right.Lower);
		}
		UInt256& UInt256::operator^=(const UInt128& Right)
		{
			Lower ^= Right;
			return *this;
		}
		UInt256& UInt256::operator^=(const UInt256& Right)
		{
			Upper ^= Right.Upper;
			Lower ^= Right.Lower;
			return *this;
		}
		UInt256 UInt256::operator~() const
		{
			return UInt256(~Upper, ~Lower);
		}
		UInt256 UInt256::operator<<(const UInt128& Right) const
		{
			return *this << UInt256(Right);
		}
		UInt256 UInt256::operator<<(const UInt256& Right) const
		{
			const UInt128 shift = Right.Lower;
			if (((bool)Right.Upper) || (shift >= UInt128(256)))
				return Min();
			else if (shift == UInt128(128))
				return UInt256(Lower, UInt128::Min());
			else if (shift == UInt128::Min())
				return *this;
			else if (shift < UInt128(128))
				return UInt256((Upper << shift) + (Lower >> (UInt128(128) - shift)), Lower << shift);
			else if ((UInt128(256) > shift) && (shift > UInt128(128)))
				return UInt256(Lower << (shift - UInt128(128)), UInt128::Min());

			return Min();
		}
		UInt256& UInt256::operator<<=(const UInt128& Shift)
		{
			return *this <<= UInt256(Shift);
		}
		UInt256& UInt256::operator<<=(const UInt256& Shift)
		{
			*this = *this << Shift;
			return *this;
		}
		UInt256 UInt256::operator>>(const UInt128& Right) const
		{
			return *this >> UInt256(Right);
		}
		UInt256 UInt256::operator>>(const UInt256& Right) const
		{
			const UInt128 Shift = Right.Lower;
			if (((bool)Right.Upper) || (Shift >= UInt128(128)))
				return Min();
			else if (Shift == UInt128(128))
				return UInt256(Upper);
			else if (Shift == UInt128::Min())
				return *this;
			else if (Shift < UInt128(128))
				return UInt256(Upper >> Shift, (Upper << (UInt128(128) - Shift)) + (Lower >> Shift));
			else if ((UInt128(256) > Shift) && (Shift > UInt128(128)))
				return UInt256(Upper >> (Shift - UInt128(128)));

			return Min();
		}
		UInt256& UInt256::operator>>=(const UInt128& Shift)
		{
			return *this >>= UInt256(Shift);
		}
		UInt256& UInt256::operator>>=(const UInt256& Shift)
		{
			*this = *this >> Shift;
			return *this;
		}
		bool UInt256::operator!() const
		{
			return !(bool)*this;
		}
		bool UInt256::operator&&(const UInt128& Right) const
		{
			return (*this && UInt256(Right));
		}
		bool UInt256::operator&&(const UInt256& Right) const
		{
			return ((bool)*this && (bool)Right);
		}
		bool UInt256::operator||(const UInt128& Right) const
		{
			return (*this || UInt256(Right));
		}
		bool UInt256::operator||(const UInt256& Right) const
		{
			return ((bool)*this || (bool)Right);
		}
		bool UInt256::operator==(const UInt128& Right) const
		{
			return (*this == UInt256(Right));
		}
		bool UInt256::operator==(const UInt256& Right) const
		{
			return ((Upper == Right.Upper) && (Lower == Right.Lower));
		}
		bool UInt256::operator!=(const UInt128& Right) const
		{
			return (*this != UInt256(Right));
		}
		bool UInt256::operator!=(const UInt256& Right) const
		{
			return ((Upper != Right.Upper) || (Lower != Right.Lower));
		}
		bool UInt256::operator>(const UInt128& Right) const
		{
			return (*this > UInt256(Right));
		}
		bool UInt256::operator>(const UInt256& Right) const
		{
			if (Upper == Right.Upper)
				return (Lower > Right.Lower);
			if (Upper > Right.Upper)
				return true;
			return false;
		}
		bool UInt256::operator<(const UInt128& Right) const
		{
			return (*this < UInt256(Right));
		}
		bool UInt256::operator<(const UInt256& Right) const
		{
			if (Upper == Right.Upper)
				return (Lower < Right.Lower);
			if (Upper < Right.Upper)
				return true;
			return false;
		}
		bool UInt256::operator>=(const UInt128& Right) const
		{
			return (*this >= UInt256(Right));
		}
		bool UInt256::operator>=(const UInt256& Right) const
		{
			return ((*this > Right) || (*this == Right));
		}
		bool UInt256::operator<=(const UInt128& Right) const
		{
			return (*this <= UInt256(Right));
		}
		bool UInt256::operator<=(const UInt256& Right) const
		{
			return ((*this < Right) || (*this == Right));
		}
		UInt256 UInt256::operator+(const UInt128& Right) const
		{
			return *this + UInt256(Right);
		}
		UInt256 UInt256::operator+(const UInt256& Right) const
		{
			return UInt256(Upper + Right.Upper + (((Lower + Right.Lower) < Lower) ? UInt128(1) : UInt128::Min()), Lower + Right.Lower);
		}
		UInt256& UInt256::operator+=(const UInt128& Right)
		{
			return *this += UInt256(Right);
		}
		UInt256& UInt256::operator+=(const UInt256& Right)
		{
			Upper = Right.Upper + Upper + ((Lower + Right.Lower) < Lower);
			Lower = Lower + Right.Lower;
			return *this;
		}
		UInt256 UInt256::operator-(const UInt128& Right) const
		{
			return *this - UInt256(Right);
		}
		UInt256 UInt256::operator-(const UInt256& Right) const
		{
			return UInt256(Upper - Right.Upper - ((Lower - Right.Lower) > Lower), Lower - Right.Lower);
		}
		UInt256& UInt256::operator-=(const UInt128& Right)
		{
			return *this -= UInt256(Right);
		}
		UInt256& UInt256::operator-=(const UInt256& Right)
		{
			*this = *this - Right;
			return *this;
		}
		UInt256 UInt256::operator*(const UInt128& Right) const
		{
			return *this * UInt256(Right);
		}
		UInt256 UInt256::operator*(const UInt256& Right) const
		{
			UInt128 top[4] = { Upper.High(), Upper.Low(), Lower.High(), Lower.Low() };
			UInt128 bottom[4] = { Right.High().High(), Right.High().Low(), Right.Low().High(), Right.Low().Low() };
			UInt128 products[4][4];

			for (int y = 3; y > -1; y--)
			{
				for (int x = 3; x > -1; x--)
					products[3 - y][x] = top[x] * bottom[y];
			}

			UInt128 fourth64 = UInt128(products[0][3].Low());
			UInt128 third64 = UInt128(products[0][2].Low()) + UInt128(products[0][3].High());
			UInt128 second64 = UInt128(products[0][1].Low()) + UInt128(products[0][2].High());
			UInt128 first64 = UInt128(products[0][0].Low()) + UInt128(products[0][1].High());
			third64 += UInt128(products[1][3].Low());
			second64 += UInt128(products[1][2].Low()) + UInt128(products[1][3].High());
			first64 += UInt128(products[1][1].Low()) + UInt128(products[1][2].High());
			second64 += UInt128(products[2][3].Low());
			first64 += UInt128(products[2][2].Low()) + UInt128(products[2][3].High());
			first64 += UInt128(products[3][3].Low());

			return UInt256(first64 << UInt128(64), UInt128::Min()) +
				UInt256(third64.High(), third64 << UInt128(64)) +
				UInt256(second64, UInt128::Min()) +
				UInt256(fourth64);
		}
		UInt256& UInt256::operator*=(const UInt128& Right)
		{
			return *this *= UInt256(Right);
		}
		UInt256& UInt256::operator*=(const UInt256& Right)
		{
			*this = *this * Right;
			return *this;
		}
		std::pair<UInt256, UInt256> UInt256::Divide(const UInt256& Left, const UInt256& Right) const
		{
			if (Right == Min())
			{
				VI_ASSERT(false, " division or modulus by zero");
				return std::pair <UInt256, UInt256>(Min(), Min());
			}
			else if (Right == UInt256(1))
				return std::pair <UInt256, UInt256>(Left, Min());
			else if (Left == Right)
				return std::pair <UInt256, UInt256>(UInt256(1), Min());
			else if ((Left == Min()) || (Left < Right))
				return std::pair <UInt256, UInt256>(Min(), Left);

			std::pair <UInt256, UInt256> qr(Min(), Left);
			UInt256 copyd = Right << (Left.Bits() - Right.Bits());
			UInt256 adder = UInt256(1) << (Left.Bits() - Right.Bits());
			if (copyd > qr.second)
			{
				copyd >>= UInt256(1);
				adder >>= UInt256(1);
			}
			while (qr.second >= Right)
			{
				if (qr.second >= copyd)
				{
					qr.second -= copyd;
					qr.first |= adder;
				}
				copyd >>= UInt256(1);
				adder >>= UInt256(1);
			}
			return qr;
		}
		UInt256 UInt256::operator/(const UInt128& Right) const
		{
			return *this / UInt256(Right);
		}
		UInt256 UInt256::operator/(const UInt256& Right) const
		{
			return Divide(*this, Right).first;
		}
		UInt256& UInt256::operator/=(const UInt128& Right)
		{
			return *this /= UInt256(Right);
		}
		UInt256& UInt256::operator/=(const UInt256& Right)
		{
			*this = *this / Right;
			return *this;
		}
		UInt256 UInt256::operator%(const UInt128& Right) const
		{
			return *this % UInt256(Right);
		}
		UInt256 UInt256::operator%(const UInt256& Right) const
		{
			return *this - (Right * (*this / Right));
		}
		UInt256& UInt256::operator%=(const UInt128& Right)
		{
			return *this %= UInt256(Right);
		}
		UInt256& UInt256::operator%=(const UInt256& Right)
		{
			*this = *this % Right;
			return *this;
		}
		UInt256& UInt256::operator++()
		{
			*this += UInt256(1);
			return *this;
		}
		UInt256 UInt256::operator++(int)
		{
			UInt256 temp(*this);
			++*this;
			return temp;
		}
		UInt256& UInt256::operator--()
		{
			*this -= UInt256(1);
			return *this;
		}
		UInt256 UInt256::operator--(int)
		{
			UInt256 temp(*this);
			--*this;
			return temp;
		}
		UInt256 UInt256::operator+() const
		{
			return *this;
		}
		UInt256 UInt256::operator-() const
		{
			return ~*this + UInt256(1);
		}
		const UInt128& UInt256::High() const
		{
			return Upper;
		}
		const UInt128& UInt256::Low() const
		{
			return Lower;
		}
		UInt128& UInt256::High()
		{
			return Upper;
		}
		UInt128& UInt256::Low()
		{
			return Lower;
		}
		uint16_t UInt256::Bits() const
		{
			uint16_t out = 0;
			if (Upper)
			{
				out = 128;
				UInt128 up = Upper;
				while (up)
				{
					up >>= UInt128(1);
					out++;
				}
			}
			else
			{
				UInt128 low = Lower;
				while (low)
				{
					low >>= UInt128(1);
					out++;
				}
			}
			return out;
		}
		uint16_t UInt256::Bytes() const
		{
			if (!*this)
				return 0;

			uint16_t Length = Bits();
			if (!Length--)
				return 0;

			return std::max<uint16_t>(1, std::min(32, (Length - Length % 8 + 8) / 8));
		}
		Core::Decimal UInt256::ToDecimal() const
		{
			return Core::Decimal::From(ToString(16), 16);
		}
		Core::String UInt256::ToString(uint8_t Base, uint32_t Length) const
		{
			VI_ASSERT(Base >= 2 && Base <= 36, "base must be in the range [2, 36]");
			static const char* Alphabet = "0123456789abcdefghijklmnopqrstuvwxyz";
			Core::String Output;
			if (!(*this))
			{
				if (!Length)
					Output.push_back('0');
				else if (Output.size() < Length)
					Output.append(Length - Output.size(), '0');
				return Output;
			}

			switch (Base)
			{
				case 16:
				{
					uint64_t Array[4]; size_t Size = Bytes();
					if (Size > sizeof(uint64_t) * 0)
					{
						Array[3] = Core::OS::CPU::ToEndianness(Core::OS::CPU::Endian::Big, Lower.Low());
						if (Size > sizeof(uint64_t) * 1)
						{
							Array[2] = Core::OS::CPU::ToEndianness(Core::OS::CPU::Endian::Big, Lower.High());
							if (Size > sizeof(uint64_t) * 2)
							{
								Array[1] = Core::OS::CPU::ToEndianness(Core::OS::CPU::Endian::Big, Upper.Low());
								if (Size > sizeof(uint64_t) * 3)
									Array[0] = Core::OS::CPU::ToEndianness(Core::OS::CPU::Endian::Big, Upper.High());
							}
						}
					}
					
					Output = Codec::HexEncodeOdd(std::string_view(((char*)Array) + (sizeof(Array) - Size), Size));
					if (Output.size() < Length)
						Output.append(Length - Output.size(), '0');
					break;
				}
				default:
				{
					std::pair<UInt256, UInt256> Remainder(*this, Min());
					do
					{
						Remainder = Divide(Remainder.first, Base);
						Output.push_back(Alphabet[(uint8_t)Remainder.second]);
					} while (Remainder.first);
					if (Output.size() < Length)
						Output.append(Length - Output.size(), '0');

					std::reverse(Output.begin(), Output.end());
					break;
				}
			}
			return Output;
		}
		UInt256 operator&(const UInt128& Left, const UInt256& Right)
		{
			return Right & Left;
		}
		UInt128& operator&=(UInt128& Left, const UInt256& Right)
		{
			Left = (Right & Left).Low();
			return Left;
		}
		UInt256 operator|(const UInt128& Left, const UInt256& Right)
		{
			return Right | Left;
		}
		UInt128& operator|=(UInt128& Left, const UInt256& Right)
		{
			Left = (Right | Left).Low();
			return Left;
		}
		UInt256 operator^(const UInt128& Left, const UInt256& Right)
		{
			return Right ^ Left;
		}
		UInt128& operator^=(UInt128& Left, const UInt256& Right)
		{
			Left = (Right ^ Left).Low();
			return Left;
		}
		UInt256 operator<<(const uint8_t& Left, const UInt256& Right)
		{
			return UInt256(Left) << Right;
		}
		UInt256 operator<<(const uint16_t& Left, const UInt256& Right)
		{
			return UInt256(Left) << Right;
		}
		UInt256 operator<<(const uint32_t& Left, const UInt256& Right)
		{
			return UInt256(Left) << Right;
		}
		UInt256 operator<<(const uint64_t& Left, const UInt256& Right)
		{
			return UInt256(Left) << Right;
		}
		UInt256 operator<<(const UInt128& Left, const UInt256& Right)
		{
			return UInt256(Left) << Right;
		}
		UInt256 operator<<(const int8_t& Left, const UInt256& Right)
		{
			return UInt256(Left) << Right;
		}
		UInt256 operator<<(const int16_t& Left, const UInt256& Right)
		{
			return UInt256(Left) << Right;
		}
		UInt256 operator<<(const int32_t& Left, const UInt256& Right)
		{
			return UInt256(Left) << Right;
		}
		UInt256 operator<<(const int64_t& Left, const UInt256& Right)
		{
			return UInt256(Left) << Right;
		}
		UInt128& operator<<=(UInt128& Left, const UInt256& Right)
		{
			Left = (UInt256(Left) << Right).Low();
			return Left;
		}
		UInt256 operator>>(const uint8_t& Left, const UInt256& Right)
		{
			return UInt256(Left) >> Right;
		}
		UInt256 operator>>(const uint16_t& Left, const UInt256& Right)
		{
			return UInt256(Left) >> Right;
		}
		UInt256 operator>>(const uint32_t& Left, const UInt256& Right)
		{
			return UInt256(Left) >> Right;
		}
		UInt256 operator>>(const uint64_t& Left, const UInt256& Right)
		{
			return UInt256(Left) >> Right;
		}
		UInt256 operator>>(const UInt128& Left, const UInt256& Right)
		{
			return UInt256(Left) >> Right;
		}
		UInt256 operator>>(const int8_t& Left, const UInt256& Right)
		{
			return UInt256(Left) >> Right;
		}
		UInt256 operator>>(const int16_t& Left, const UInt256& Right)
		{
			return UInt256(Left) >> Right;
		}
		UInt256 operator>>(const int32_t& Left, const UInt256& Right)
		{
			return UInt256(Left) >> Right;
		}
		UInt256 operator>>(const int64_t& Left, const UInt256& Right)
		{
			return UInt256(Left) >> Right;
		}
		UInt128& operator>>=(UInt128& Left, const UInt256& Right)
		{
			Left = (UInt256(Left) >> Right).Low();
			return Left;
		}
		bool operator==(const UInt128& Left, const UInt256& Right)
		{
			return Right == Left;
		}
		bool operator!=(const UInt128& Left, const UInt256& Right)
		{
			return Right != Left;
		}
		bool operator>(const UInt128& Left, const UInt256& Right)
		{
			return Right < Left;
		}
		bool operator<(const UInt128& Left, const UInt256& Right)
		{
			return Right > Left;
		}
		bool operator>=(const UInt128& Left, const UInt256& Right)
		{
			return Right <= Left;
		}
		bool operator<=(const UInt128& Left, const UInt256& Right)
		{
			return Right >= Left;
		}
		UInt256 operator+(const UInt128& Left, const UInt256& Right)
		{
			return Right + Left;
		}
		UInt128& operator+=(UInt128& Left, const UInt256& Right)
		{
			Left = (Right + Left).Low();
			return Left;
		}
		UInt256 operator-(const UInt128& Left, const UInt256& Right)
		{
			return -(Right - Left);
		}
		UInt128& operator-=(UInt128& Left, const UInt256& Right)
		{
			Left = (-(Right - Left)).Low();
			return Left;
		}
		UInt256 operator*(const UInt128& Left, const UInt256& Right)
		{
			return Right * Left;
		}
		UInt128& operator*=(UInt128& Left, const UInt256& Right)
		{
			Left = (Right * Left).Low();
			return Left;
		}
		UInt256 operator/(const UInt128& Left, const UInt256& Right)
		{
			return UInt256(Left) / Right;
		}
		UInt128& operator/=(UInt128& Left, const UInt256& Right)
		{
			Left = (UInt256(Left) / Right).Low();
			return Left;
		}
		UInt256 operator%(const UInt128& Left, const UInt256& Right)
		{
			return UInt256(Left) % Right;
		}
		UInt128& operator%=(UInt128& Left, const UInt256& Right)
		{
			Left = (UInt256(Left) % Right).Low();
			return Left;
		}
		std::ostream& operator<<(std::ostream& Stream, const UInt256& Right)
		{
			if (Stream.flags() & Stream.oct)
				Stream << Right.ToString(8);
			else if (Stream.flags() & Stream.dec)
				Stream << Right.ToString(10);
			else if (Stream.flags() & Stream.hex)
				Stream << Right.ToString(16);
			return Stream;
		}

		PreprocessorException::PreprocessorException(PreprocessorError NewType) : PreprocessorException(NewType, 0)
		{
		}
		PreprocessorException::PreprocessorException(PreprocessorError NewType, size_t NewOffset) : PreprocessorException(NewType, NewOffset, "")
		{
		}
		PreprocessorException::PreprocessorException(PreprocessorError NewType, size_t NewOffset, const std::string_view& NewMessage) : Type(NewType), Offset(NewOffset)
		{
			switch (Type)
			{
				case PreprocessorError::MacroDefinitionEmpty:
					Message = "empty macro directive definition";
					break;
				case PreprocessorError::MacroNameEmpty:
					Message = "empty macro directive name";
					break;
				case PreprocessorError::MacroParenthesisDoubleClosed:
					Message = "macro directive parenthesis is closed twice";
					break;
				case PreprocessorError::MacroParenthesisNotClosed:
					Message = "macro directive parenthesis is not closed";
					break;
				case PreprocessorError::MacroDefinitionError:
					Message = "macro directive definition parsing error";
					break;
				case PreprocessorError::MacroExpansionParenthesisDoubleClosed:
					Message = "macro expansion directive parenthesis is closed twice";
					break;
				case PreprocessorError::MacroExpansionParenthesisNotClosed:
					Message = "macro expansion directive parenthesis is not closed";
					break;
				case PreprocessorError::MacroExpansionArgumentsError:
					Message = "macro expansion directive uses incorrect number of arguments";
					break;
				case PreprocessorError::MacroExpansionExecutionError:
					Message = "macro expansion directive execution error";
					break;
				case PreprocessorError::MacroExpansionError:
					Message = "macro expansion directive parsing error";
					break;
				case PreprocessorError::ConditionNotOpened:
					Message = "conditional directive has no opened parenthesis";
					break;
				case PreprocessorError::ConditionNotClosed:
					Message = "conditional directive parenthesis is not closed";
					break;
				case PreprocessorError::ConditionError:
					Message = "conditional directive parsing error";
					break;
				case PreprocessorError::DirectiveNotFound:
					Message = "directive is not found";
					break;
				case PreprocessorError::DirectiveExpansionError:
					Message = "directive expansion error";
					break;
				case PreprocessorError::IncludeDenied:
					Message = "inclusion directive is denied";
					break;
				case PreprocessorError::IncludeError:
					Message = "inclusion directive parsing error";
					break;
				case PreprocessorError::IncludeNotFound:
					Message = "inclusion directive is not found";
					break;
				case PreprocessorError::PragmaNotFound:
					Message = "pragma directive is not found";
					break;
				case PreprocessorError::PragmaError:
					Message = "pragma directive parsing error";
					break;
				case PreprocessorError::ExtensionError:
					Message = "extension error";
					break;
				default:
					break;
			}

			if (Offset > 0)
			{
				Message += " at offset ";
				Message += Core::ToString(Offset);
			}

			if (!NewMessage.empty())
			{
				Message += " on ";
				Message += NewMessage;
			}
		}
		const char* PreprocessorException::type() const noexcept
		{
			return "preprocessor_error";
		}
		PreprocessorError PreprocessorException::status() const noexcept
		{
			return Type;
		}
		size_t PreprocessorException::offset() const noexcept
		{
			return Offset;
		}

		CryptoException::CryptoException() : ErrorCode(0)
		{
#ifdef VI_OPENSSL
			unsigned long Error = ERR_get_error();
			if (Error > 0)
			{
				ErrorCode = (size_t)Error;
				char* ErrorText = ERR_error_string(Error, nullptr);
				if (ErrorText != nullptr)
					Message = ErrorText;
				else
					Message = "error:" + Core::ToString(ErrorCode);
			}
			else
				Message = "error:internal";
#else
			Message = "error:unsupported";
#endif
		}
		CryptoException::CryptoException(size_t NewErrorCode, const std::string_view& NewMessage) : ErrorCode(NewErrorCode)
		{
			Message = "error:" + Core::ToString(ErrorCode) + ":";
			if (!NewMessage.empty())
				Message += NewMessage;
			else
				Message += "internal";
		}
		const char* CryptoException::type() const noexcept
		{
			return "crypto_error";
		}
		size_t CryptoException::error_code() const noexcept
		{
			return ErrorCode;
		}

		CompressionException::CompressionException(int NewErrorCode, const std::string_view& NewMessage) : ErrorCode(NewErrorCode)
		{
			if (!NewMessage.empty())
				Message += NewMessage;
			else
				Message += "internal error";
			Message += " (error = " + Core::ToString(ErrorCode) + ")";
		}
		const char* CompressionException::type() const noexcept
		{
			return "compression_error";
		}
		int CompressionException::error_code() const noexcept
		{
			return ErrorCode;
		}

		MD5Hasher::MD5Hasher() noexcept
		{
			VI_TRACE("[crypto] create md5 hasher");
			memset(Buffer, 0, sizeof(Buffer));
			memset(Digest, 0, sizeof(Digest));
			Finalized = false;
			Count[0] = 0;
			Count[1] = 0;
			State[0] = 0x67452301;
			State[1] = 0xefcdab89;
			State[2] = 0x98badcfe;
			State[3] = 0x10325476;
		}
		void MD5Hasher::Decode(UInt4* Output, const UInt1* Input, uint32_t Length)
		{
			VI_ASSERT(Output != nullptr && Input != nullptr, "output and input should be set");
			for (uint32_t i = 0, j = 0; j < Length; i++, j += 4)
				Output[i] = ((UInt4)Input[j]) | (((UInt4)Input[j + 1]) << 8) | (((UInt4)Input[j + 2]) << 16) | (((UInt4)Input[j + 3]) << 24);
			VI_TRACE("[crypto] md5 hasher decode to 0x%" PRIXPTR, (void*)Output);
		}
		void MD5Hasher::Encode(UInt1* Output, const UInt4* Input, uint32_t Length)
		{
			VI_ASSERT(Output != nullptr && Input != nullptr, "output and input should be set");
			for (uint32_t i = 0, j = 0; j < Length; i++, j += 4)
			{
				Output[j] = Input[i] & 0xff;
				Output[j + 1] = (Input[i] >> 8) & 0xff;
				Output[j + 2] = (Input[i] >> 16) & 0xff;
				Output[j + 3] = (Input[i] >> 24) & 0xff;
			}
			VI_TRACE("[crypto] md5 hasher encode to 0x%" PRIXPTR, (void*)Output);
		}
		void MD5Hasher::Transform(const UInt1* Block, uint32_t Length)
		{
			VI_ASSERT(Block != nullptr, "block should be set");
			VI_TRACE("[crypto] md5 hasher transform from 0x%" PRIXPTR, (void*)Block);
			UInt4 A = State[0], B = State[1], C = State[2], D = State[3], X[16];
			Decode(X, Block, Length);

			FF(A, B, C, D, X[0], S11, 0xd76aa478);
			FF(D, A, B, C, X[1], S12, 0xe8c7b756);
			FF(C, D, A, B, X[2], S13, 0x242070db);
			FF(B, C, D, A, X[3], S14, 0xc1bdceee);
			FF(A, B, C, D, X[4], S11, 0xf57c0faf);
			FF(D, A, B, C, X[5], S12, 0x4787c62a);
			FF(C, D, A, B, X[6], S13, 0xa8304613);
			FF(B, C, D, A, X[7], S14, 0xfd469501);
			FF(A, B, C, D, X[8], S11, 0x698098d8);
			FF(D, A, B, C, X[9], S12, 0x8b44f7af);
			FF(C, D, A, B, X[10], S13, 0xffff5bb1);
			FF(B, C, D, A, X[11], S14, 0x895cd7be);
			FF(A, B, C, D, X[12], S11, 0x6b901122);
			FF(D, A, B, C, X[13], S12, 0xfd987193);
			FF(C, D, A, B, X[14], S13, 0xa679438e);
			FF(B, C, D, A, X[15], S14, 0x49b40821);
			GG(A, B, C, D, X[1], S21, 0xf61e2562);
			GG(D, A, B, C, X[6], S22, 0xc040b340);
			GG(C, D, A, B, X[11], S23, 0x265e5a51);
			GG(B, C, D, A, X[0], S24, 0xe9b6c7aa);
			GG(A, B, C, D, X[5], S21, 0xd62f105d);
			GG(D, A, B, C, X[10], S22, 0x2441453);
			GG(C, D, A, B, X[15], S23, 0xd8a1e681);
			GG(B, C, D, A, X[4], S24, 0xe7d3fbc8);
			GG(A, B, C, D, X[9], S21, 0x21e1cde6);
			GG(D, A, B, C, X[14], S22, 0xc33707d6);
			GG(C, D, A, B, X[3], S23, 0xf4d50d87);
			GG(B, C, D, A, X[8], S24, 0x455a14ed);
			GG(A, B, C, D, X[13], S21, 0xa9e3e905);
			GG(D, A, B, C, X[2], S22, 0xfcefa3f8);
			GG(C, D, A, B, X[7], S23, 0x676f02d9);
			GG(B, C, D, A, X[12], S24, 0x8d2a4c8a);
			HH(A, B, C, D, X[5], S31, 0xfffa3942);
			HH(D, A, B, C, X[8], S32, 0x8771f681);
			HH(C, D, A, B, X[11], S33, 0x6d9d6122);
			HH(B, C, D, A, X[14], S34, 0xfde5380c);
			HH(A, B, C, D, X[1], S31, 0xa4beea44);
			HH(D, A, B, C, X[4], S32, 0x4bdecfa9);
			HH(C, D, A, B, X[7], S33, 0xf6bb4b60);
			HH(B, C, D, A, X[10], S34, 0xbebfbc70);
			HH(A, B, C, D, X[13], S31, 0x289b7ec6);
			HH(D, A, B, C, X[0], S32, 0xeaa127fa);
			HH(C, D, A, B, X[3], S33, 0xd4ef3085);
			HH(B, C, D, A, X[6], S34, 0x4881d05);
			HH(A, B, C, D, X[9], S31, 0xd9d4d039);
			HH(D, A, B, C, X[12], S32, 0xe6db99e5);
			HH(C, D, A, B, X[15], S33, 0x1fa27cf8);
			HH(B, C, D, A, X[2], S34, 0xc4ac5665);
			II(A, B, C, D, X[0], S41, 0xf4292244);
			II(D, A, B, C, X[7], S42, 0x432aff97);
			II(C, D, A, B, X[14], S43, 0xab9423a7);
			II(B, C, D, A, X[5], S44, 0xfc93a039);
			II(A, B, C, D, X[12], S41, 0x655b59c3);
			II(D, A, B, C, X[3], S42, 0x8f0ccc92);
			II(C, D, A, B, X[10], S43, 0xffeff47d);
			II(B, C, D, A, X[1], S44, 0x85845dd1);
			II(A, B, C, D, X[8], S41, 0x6fa87e4f);
			II(D, A, B, C, X[15], S42, 0xfe2ce6e0);
			II(C, D, A, B, X[6], S43, 0xa3014314);
			II(B, C, D, A, X[13], S44, 0x4e0811a1);
			II(A, B, C, D, X[4], S41, 0xf7537e82);
			II(D, A, B, C, X[11], S42, 0xbd3af235);
			II(C, D, A, B, X[2], S43, 0x2ad7d2bb);
			II(B, C, D, A, X[9], S44, 0xeb86d391);

			State[0] += A;
			State[1] += B;
			State[2] += C;
			State[3] += D;
#ifdef VI_MICROSOFT
			RtlSecureZeroMemory(X, sizeof(X));
#else
			memset(X, 0, sizeof(X));
#endif
		}
		void MD5Hasher::Update(const std::string_view& Input, uint32_t BlockSize)
		{
			Update(Input.data(), (uint32_t)Input.size(), BlockSize);
		}
		void MD5Hasher::Update(const uint8_t* Input, uint32_t Length, uint32_t BlockSize)
		{
			VI_ASSERT(Input != nullptr, "input should be set");
			VI_TRACE("[crypto] md5 hasher update from 0x%" PRIXPTR, (void*)Input);
			uint32_t Index = Count[0] / 8 % BlockSize;
			Count[0] += (Length << 3);
			if (Count[0] < Length << 3)
				Count[1]++;

			Count[1] += (Length >> 29);
			uint32_t Chunk = 64 - Index, i = 0;
			if (Length >= Chunk)
			{
				memcpy(&Buffer[Index], Input, Chunk);
				Transform(Buffer, BlockSize);

				for (i = Chunk; i + BlockSize <= Length; i += BlockSize)
					Transform(&Input[i]);
				Index = 0;
			}
			else
				i = 0;

			memcpy(&Buffer[Index], &Input[i], Length - i);
		}
		void MD5Hasher::Update(const char* Input, uint32_t Length, uint32_t BlockSize)
		{
			Update((const uint8_t*)Input, Length, BlockSize);
		}
		void MD5Hasher::Finalize()
		{
			VI_TRACE("[crypto] md5 hasher finalize at 0x%" PRIXPTR, (void*)this);
			static uint8_t Padding[64] = { 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
			if (Finalized)
				return;

			uint8_t Bits[8];
			Encode(Bits, Count, 8);

			uint32_t Index = Count[0] / 8 % 64;
			uint32_t Size = (Index < 56) ? (56 - Index) : (120 - Index);
			Update(Padding, Size);
			Update(Bits, 8);
			Encode(Digest, State, 16);

			memset(Buffer, 0, sizeof(Buffer));
			memset(Count, 0, sizeof(Count));
			Finalized = true;
		}
		char* MD5Hasher::HexDigest() const
		{
			VI_ASSERT(Finalized, "md5 hash should be finalized");
			char* Data = Core::Memory::Allocate<char>(sizeof(char) * 48);
			memset(Data, 0, sizeof(char) * 48);

			for (size_t i = 0; i < 16; i++)
				snprintf(Data + i * 2, 4, "%02x", (uint32_t)Digest[i]);

			return Data;
		}
		uint8_t* MD5Hasher::RawDigest() const
		{
			VI_ASSERT(Finalized, "md5 hash should be finalized");
			UInt1* Output = Core::Memory::Allocate<UInt1>(sizeof(UInt1) * 17);
			memcpy(Output, Digest, 16);
			Output[16] = '\0';

			return Output;
		}
		Core::String MD5Hasher::ToHex() const
		{
			VI_ASSERT(Finalized, "md5 hash should be finalized");
			char Data[48];
			memset(Data, 0, sizeof(Data));

			for (size_t i = 0; i < 16; i++)
				snprintf(Data + i * 2, 4, "%02x", (uint32_t)Digest[i]);

			return Data;
		}
		Core::String MD5Hasher::ToRaw() const
		{
			VI_ASSERT(Finalized, "md5 hash should be finalized");
			UInt1 Data[17];
			memcpy(Data, Digest, 16);
			Data[16] = '\0';

			return (const char*)Data;
		}
		MD5Hasher::UInt4 MD5Hasher::F(UInt4 X, UInt4 Y, UInt4 Z)
		{
			return (X & Y) | (~X & Z);
		}
		MD5Hasher::UInt4 MD5Hasher::G(UInt4 X, UInt4 Y, UInt4 Z)
		{
			return (X & Z) | (Y & ~Z);
		}
		MD5Hasher::UInt4 MD5Hasher::H(UInt4 X, UInt4 Y, UInt4 Z)
		{
			return X ^ Y ^ Z;
		}
		MD5Hasher::UInt4 MD5Hasher::I(UInt4 X, UInt4 Y, UInt4 Z)
		{
			return Y ^ (X | ~Z);
		}
		MD5Hasher::UInt4 MD5Hasher::L(UInt4 X, int n)
		{
			return (X << n) | (X >> (32 - n));
		}
		void MD5Hasher::FF(UInt4& A, UInt4 B, UInt4 C, UInt4 D, UInt4 X, UInt4 S, UInt4 AC)
		{
			A = L(A + F(B, C, D) + X + AC, S) + B;
		}
		void MD5Hasher::GG(UInt4& A, UInt4 B, UInt4 C, UInt4 D, UInt4 X, UInt4 S, UInt4 AC)
		{
			A = L(A + G(B, C, D) + X + AC, S) + B;
		}
		void MD5Hasher::HH(UInt4& A, UInt4 B, UInt4 C, UInt4 D, UInt4 X, UInt4 S, UInt4 AC)
		{
			A = L(A + H(B, C, D) + X + AC, S) + B;
		}
		void MD5Hasher::II(UInt4& A, UInt4 B, UInt4 C, UInt4 D, UInt4 X, UInt4 S, UInt4 AC)
		{
			A = L(A + I(B, C, D) + X + AC, S) + B;
		}

		Cipher Ciphers::DES_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE3()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede3();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE3_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede3_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_CFB64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_CFB1()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_CFB8()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE_CFB64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE3_CFB64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede3_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE3_CFB1()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede3_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE3_CFB8()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede3_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE3_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede3_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE3_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede3_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DES_EDE3_Wrap()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_des_ede3_wrap();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::DESX_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (Cipher)EVP_desx_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC4()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC4
			return (Cipher)EVP_rc4();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC4_40()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC4
			return (Cipher)EVP_rc4_40();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC4_HMAC_MD5()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC4
#ifndef OPENSSL_NO_MD5
			return (Cipher)EVP_rc4_hmac_md5();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::IDEA_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_IDEA
			return (Cipher)EVP_idea_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::IDEA_CFB64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_IDEA
			return (Cipher)EVP_idea_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::IDEA_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_IDEA
			return (Cipher)EVP_idea_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::IDEA_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_IDEA
			return (Cipher)EVP_idea_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC2_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC2
			return (Cipher)EVP_rc2_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC2_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC2
			return (Cipher)EVP_rc2_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC2_40_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC2
			return (Cipher)EVP_rc2_40_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC2_64_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC2
			return (Cipher)EVP_rc2_64_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC2_CFB64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC2
			return (Cipher)EVP_rc2_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC2_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC2
			return (Cipher)EVP_rc2_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::BF_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_BF
			return (Cipher)EVP_bf_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::BF_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_BF
			return (Cipher)EVP_bf_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::BF_CFB64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_BF
			return (Cipher)EVP_bf_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::BF_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_BF
			return (Cipher)EVP_bf_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::CAST5_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAST
			return (Cipher)EVP_cast5_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::CAST5_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAST
			return (Cipher)EVP_cast5_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::CAST5_CFB64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAST
			return (Cipher)EVP_cast5_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::CAST5_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAST
			return (Cipher)EVP_cast5_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC5_32_12_16_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC5
			return (Cipher)EVP_rc5_32_12_16_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC5_32_12_16_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC5
			return (Cipher)EVP_rc5_32_12_16_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC5_32_12_16_CFB64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC5
			return (Cipher)EVP_rc5_32_12_16_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::RC5_32_12_16_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC5
			return (Cipher)EVP_rc5_32_12_16_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_ECB()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_ecb();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CBC()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_cbc();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CFB1()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_cfb1();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CFB8()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_cfb8();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CFB128()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_cfb128();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_OFB()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_ofb();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CTR()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_ctr();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CCM()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_ccm();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_GCM()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_gcm();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_XTS()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_xts();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_Wrap()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_128_wrap();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_WrapPad()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_128_wrap_pad();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_OCB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_OCB
			return (Cipher)EVP_aes_128_ocb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_ECB()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_192_ecb();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_CBC()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_192_cbc();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_CFB1()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_192_cfb1();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_CFB8()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_192_cfb8();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_CFB128()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_192_cfb128();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_OFB()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_192_ofb();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_CTR()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_192_ctr();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_CCM()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_192_ccm();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_GCM()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_192_gcm();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_Wrap()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_192_wrap();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_WrapPad()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_192_wrap_pad();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_192_OCB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_OCB
			return (Cipher)EVP_aes_192_ocb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_ECB()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_ecb();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CBC()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_cbc();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CFB1()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_cfb1();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CFB8()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_cfb8();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CFB128()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_cfb128();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_OFB()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_ofb();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CTR()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_ctr();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CCM()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_ccm();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_GCM()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_gcm();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_XTS()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_xts();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_Wrap()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_256_wrap();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_WrapPad()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_256_wrap_pad();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_OCB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_OCB
			return (Cipher)EVP_aes_256_ocb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CBC_HMAC_SHA1()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_128_cbc_hmac_sha1();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CBC_HMAC_SHA1()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_aes_256_cbc_hmac_sha1();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_128_CBC_HMAC_SHA256()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_128_cbc_hmac_sha256();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::AES_256_CBC_HMAC_SHA256()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_aes_256_cbc_hmac_sha256();
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_128_ECB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_128_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_128_CBC()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_128_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_128_CFB1()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_128_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_128_CFB8()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_128_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_128_CFB128()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_128_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_128_CTR()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_128_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_128_OFB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_128_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_128_GCM()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_128_gcm();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_128_CCM()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_128_ccm();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_192_ECB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_192_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_192_CBC()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_192_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_192_CFB1()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_192_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_192_CFB8()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_192_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_192_CFB128()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_192_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_192_CTR()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_192_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_192_OFB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_192_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_192_GCM()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_192_gcm();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_192_CCM()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_192_ccm();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_256_ECB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_256_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_256_CBC()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_256_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_256_CFB1()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_256_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_256_CFB8()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_256_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_256_CFB128()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_256_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_256_CTR()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_256_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_256_OFB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_256_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_256_GCM()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_256_gcm();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::ARIA_256_CCM()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (Cipher)EVP_aria_256_ccm();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_128_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_128_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_128_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_128_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_128_CFB1()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_128_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_128_CFB8()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_128_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_128_CFB128()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_128_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_128_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_128_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_128_CTR()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_128_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_192_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_192_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_192_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_192_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_192_CFB1()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_192_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_192_CFB8()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_192_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_192_CFB128()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_192_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_192_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_192_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_192_CTR()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_192_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_256_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_256_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_256_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_256_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_256_CFB1()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_256_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_256_CFB8()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_256_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_256_CFB128()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_256_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_256_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_256_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Camellia_256_CTR()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_CAMELLIA
			return (Cipher)EVP_camellia_256_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Chacha20()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_CHACHA
			return (Cipher)EVP_chacha20();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Chacha20_Poly1305()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_CHACHA
#ifndef OPENSSL_NO_POLY1305
			return (Cipher)EVP_chacha20_poly1305();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Seed_ECB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_SEED
			return (Cipher)EVP_seed_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Seed_CBC()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_SEED
			return (Cipher)EVP_seed_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Seed_CFB128()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_SEED
			return (Cipher)EVP_seed_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::Seed_OFB()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_SEED
			return (Cipher)EVP_seed_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::SM4_ECB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_SM4
			return (Cipher)EVP_sm4_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::SM4_CBC()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_SM4
			return (Cipher)EVP_sm4_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::SM4_CFB128()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_SM4
			return (Cipher)EVP_sm4_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::SM4_OFB()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_SM4
			return (Cipher)EVP_sm4_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Cipher Ciphers::SM4_CTR()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_SM4
			return (Cipher)EVP_sm4_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}

		Digest Digests::MD2()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_MD2
			return (Cipher)EVP_md2();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Digest Digests::MD4()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_MD4
			return (Cipher)EVP_md4();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Digest Digests::MD5()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_MD5
			return (Cipher)EVP_md5();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Digest Digests::MD5_SHA1()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_MD5
			return (Cipher)EVP_md5_sha1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Digest Digests::Blake2B512()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_BLAKE2
			return (Cipher)EVP_blake2b512();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Digest Digests::Blake2S256()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_BLAKE2
			return (Cipher)EVP_blake2s256();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA1()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_sha1();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA224()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_sha224();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA256()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_sha256();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA384()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_sha384();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA512()
		{
#ifdef VI_OPENSSL
			return (Cipher)EVP_sha512();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA512_224()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_sha512_224();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA512_256()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_sha512_256();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA3_224()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_sha3_224();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA3_256()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_sha3_256();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA3_384()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_sha3_384();
#else
			return nullptr;
#endif
		}
		Digest Digests::SHA3_512()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_sha3_512();
#else
			return nullptr;
#endif
		}
		Digest Digests::Shake128()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_shake128();
#else
			return nullptr;
#endif
		}
		Digest Digests::Shake256()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (Cipher)EVP_shake256();
#else
			return nullptr;
#endif
		}
		Digest Digests::MDC2()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_MDC2
			return (Cipher)EVP_mdc2();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Digest Digests::RipeMD160()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RMD160
			return (Cipher)EVP_ripemd160();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Digest Digests::Whirlpool()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_WHIRLPOOL
			return (Cipher)EVP_whirlpool();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		Digest Digests::SM3()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_SM3
			return (Cipher)EVP_sm3();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}

		SignAlg Signers::PkRSA()
		{
#ifdef EVP_PK_RSA
			return (SignAlg)EVP_PK_RSA;
#else
			return -1;
#endif
		}
		SignAlg Signers::PkDSA()
		{
#ifdef EVP_PK_DSA
			return (SignAlg)EVP_PK_DSA;
#else
			return -1;
#endif
		}
		SignAlg Signers::PkDH()
		{
#ifdef EVP_PK_DH
			return (SignAlg)EVP_PK_DH;
#else
			return -1;
#endif
		}
		SignAlg Signers::PkEC()
		{
#ifdef EVP_PK_EC
			return (SignAlg)EVP_PK_EC;
#else
			return -1;
#endif
		}
		SignAlg Signers::PktSIGN()
		{
#ifdef EVP_PKT_SIGN
			return (SignAlg)EVP_PKT_SIGN;
#else
			return -1;
#endif
		}
		SignAlg Signers::PktENC()
		{
#ifdef EVP_PKT_ENC
			return (SignAlg)EVP_PKT_ENC;
#else
			return -1;
#endif
		}
		SignAlg Signers::PktEXCH()
		{
#ifdef EVP_PKT_EXCH
			return (SignAlg)EVP_PKT_EXCH;
#else
			return -1;
#endif
		}
		SignAlg Signers::PksRSA()
		{
#ifdef EVP_PKS_RSA
			return (SignAlg)EVP_PKS_RSA;
#else
			return -1;
#endif
		}
		SignAlg Signers::PksDSA()
		{
#ifdef EVP_PKS_DSA
			return (SignAlg)EVP_PKS_DSA;
#else
			return -1;
#endif
		}
		SignAlg Signers::PksEC()
		{
#ifdef EVP_PKS_EC
			return (SignAlg)EVP_PKS_EC;
#else
			return -1;
#endif
		}
		SignAlg Signers::RSA()
		{
#ifdef EVP_PKEY_RSA
			return (SignAlg)EVP_PKEY_RSA;
#else
			return -1;
#endif
		}
		SignAlg Signers::RSA2()
		{
#ifdef EVP_PKEY_RSA2
			return (SignAlg)EVP_PKEY_RSA2;
#else
			return -1;
#endif
		}
		SignAlg Signers::RSA_PSS()
		{
#ifdef EVP_PKEY_RSA_PSS
			return (SignAlg)EVP_PKEY_RSA_PSS;
#else
			return -1;
#endif
		}
		SignAlg Signers::DSA()
		{
#ifdef EVP_PKEY_DSA
			return (SignAlg)EVP_PKEY_DSA;
#else
			return -1;
#endif
		}
		SignAlg Signers::DSA1()
		{
#ifdef EVP_PKEY_DSA1
			return (SignAlg)EVP_PKEY_DSA1;
#else
			return -1;
#endif
		}
		SignAlg Signers::DSA2()
		{
#ifdef EVP_PKEY_DSA2
			return (SignAlg)EVP_PKEY_DSA2;
#else
			return -1;
#endif
		}
		SignAlg Signers::DSA3()
		{
#ifdef EVP_PKEY_DSA3
			return (SignAlg)EVP_PKEY_DSA3;
#else
			return -1;
#endif
		}
		SignAlg Signers::DSA4()
		{
#ifdef EVP_PKEY_DSA4
			return (SignAlg)EVP_PKEY_DSA4;
#else
			return -1;
#endif
		}
		SignAlg Signers::DH()
		{
#ifdef EVP_PKEY_DH
			return (SignAlg)EVP_PKEY_DH;
#else
			return -1;
#endif
		}
		SignAlg Signers::DHX()
		{
#ifdef EVP_PKEY_DHX
			return (SignAlg)EVP_PKEY_DHX;
#else
			return -1;
#endif
		}
		SignAlg Signers::EC()
		{
#ifdef EVP_PKEY_EC
			return (SignAlg)EVP_PKEY_EC;
#else
			return -1;
#endif
		}
		SignAlg Signers::SM2()
		{
#ifdef EVP_PKEY_SM2
			return (SignAlg)EVP_PKEY_SM2;
#else
			return -1;
#endif
		}
		SignAlg Signers::HMAC()
		{
#ifdef EVP_PKEY_HMAC
			return (SignAlg)EVP_PKEY_HMAC;
#else
			return -1;
#endif
		}
		SignAlg Signers::CMAC()
		{
#ifdef EVP_PKEY_CMAC
			return (SignAlg)EVP_PKEY_CMAC;
#else
			return -1;
#endif
		}
		SignAlg Signers::SCRYPT()
		{
#ifdef EVP_PKEY_SCRYPT
			return (SignAlg)EVP_PKEY_SCRYPT;
#else
			return -1;
#endif
		}
		SignAlg Signers::TLS1_PRF()
		{
#ifdef EVP_PKEY_TLS1_PRF
			return (SignAlg)EVP_PKEY_TLS1_PRF;
#else
			return -1;
#endif
		}
		SignAlg Signers::HKDF()
		{
#ifdef EVP_PKEY_HKDF
			return (SignAlg)EVP_PKEY_HKDF;
#else
			return -1;
#endif
		}
		SignAlg Signers::POLY1305()
		{
#ifdef EVP_PKEY_POLY1305
			return (SignAlg)EVP_PKEY_POLY1305;
#else
			return -1;
#endif
		}
		SignAlg Signers::SIPHASH()
		{
#ifdef EVP_PKEY_SIPHASH
			return (SignAlg)EVP_PKEY_SIPHASH;
#else
			return -1;
#endif
		}
		SignAlg Signers::X25519()
		{
#ifdef EVP_PKEY_X25519
			return (SignAlg)EVP_PKEY_X25519;
#else
			return -1;
#endif
		}
		SignAlg Signers::ED25519()
		{
#ifdef EVP_PKEY_ED25519
			return (SignAlg)EVP_PKEY_ED25519;
#else
			return -1;
#endif
		}
		SignAlg Signers::X448()
		{
#ifdef EVP_PKEY_X448
			return (SignAlg)EVP_PKEY_X448;
#else
			return -1;
#endif
		}
		SignAlg Signers::ED448()
		{
#ifdef EVP_PKEY_ED448
			return (SignAlg)EVP_PKEY_ED448;
#else
			return -1;
#endif
		}

		Digest Crypto::GetDigestByName(const std::string_view& Name)
		{
			VI_ASSERT(Core::Stringify::IsCString(Name), "digest name should not be empty");
#ifdef EVP_MD_name
			return (Digest)EVP_get_digestbyname(Name.data());
#else
			return nullptr;
#endif
		}
		Cipher Crypto::GetCipherByName(const std::string_view& Name)
		{
			VI_ASSERT(Core::Stringify::IsCString(Name), "cipher name should not be empty");
#ifdef EVP_MD_name
			return (Cipher)EVP_get_cipherbyname(Name.data());
#else
			return nullptr;
#endif
		}
		SignAlg Crypto::GetSignerByName(const std::string_view& Name)
		{
#ifdef EVP_MD_name
			auto* Method = EVP_PKEY_asn1_find_str(nullptr, Name.data(), (int)Name.size());
			if (!Method)
				return -1;

			int KeyType = -1;
			if (EVP_PKEY_asn1_get0_info(&KeyType, nullptr, nullptr, nullptr, nullptr, Method) != 1)
				return -1;

			return (SignAlg)KeyType;
#else
			return -1;
#endif
		}
		std::string_view Crypto::GetDigestName(Digest Type)
		{
			VI_ASSERT(Type != nullptr, "digest should be set");
#ifdef EVP_MD_name
			const char* Name = EVP_MD_name((EVP_MD*)Type);
			return Name ? Name : "";
#else
			return "";
#endif
		}
		std::string_view Crypto::GetCipherName(Cipher Type)
		{
			VI_ASSERT(Type != nullptr, "cipher should be set");
#ifdef EVP_CIPHER_name
			const char* Name = EVP_CIPHER_name((EVP_CIPHER*)Type);
			return Name ? Name : "";
#else
			return "";
#endif
		}
		std::string_view Crypto::GetSignerName(SignAlg Type)
		{
#ifdef VI_OPENSSL
			if (Type == EVP_PKEY_NONE)
				return "";

			const char* Name = OBJ_nid2sn(Type);
			return Name ? Name : "";
#else
			return "";
#endif
		}
		ExpectsCrypto<void> Crypto::FillRandomBytes(uint8_t* Buffer, size_t Length)
		{
#ifdef VI_OPENSSL
			VI_TRACE("[crypto] fill random %" PRIu64 " bytes", (uint64_t)Length);
			if (RAND_bytes(Buffer, (int)Length) == 1)
				return Core::Expectation::Met;
#endif
			return CryptoException();
		}
		ExpectsCrypto<Core::String> Crypto::RandomBytes(size_t Length)
		{
#ifdef VI_OPENSSL
			Core::String Buffer(Length, '\0');
			auto Status = FillRandomBytes((uint8_t*)Buffer.data(), Buffer.size());
			if (!Status)
				return Status.Error();

			return Buffer;
#else
			return CryptoException();
#endif
		}
		ExpectsCrypto<Core::String> Crypto::GeneratePrivateKey(SignAlg Type, size_t Bits, const std::string_view& Curve)
		{
#ifdef VI_OPENSSL
			VI_ASSERT(Curve.empty() || Core::Stringify::IsCString(Curve), "curve should not be empty");
			VI_TRACE("[crypto] generate %s private key up to %i bits", GetSignerName(Type).data(), (int)Bits);
			EVP_PKEY_CTX* Context = EVP_PKEY_CTX_new_id((int)Type, NULL);
			if (!Context)
				return CryptoException();

			if (EVP_PKEY_keygen_init(Context) != 1)
			{
				EVP_PKEY_CTX_free(Context);
				return CryptoException();
			}

			bool Success = true;
			switch (Type)
			{
				case EVP_PKEY_RSA:
				case EVP_PKEY_RSA2:
				case EVP_PKEY_RSA_PSS:
					Success = (EVP_PKEY_CTX_set_rsa_keygen_bits(Context, (int)Bits) == 1);
					break;
				case EVP_PKEY_EC:
				case EVP_PKEY_SM2:
				case EVP_PKEY_SCRYPT:
				case EVP_PKEY_TLS1_PRF:
				case EVP_PKEY_HKDF:
				case EVP_PKEY_POLY1305:
				case EVP_PKEY_SIPHASH:
				case EVP_PKEY_X25519:
				case EVP_PKEY_ED25519:
				case EVP_PKEY_X448:
				case EVP_PKEY_ED448:
					Success = (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(Context, Curve.empty() ? NID_X9_62_prime256v1 : OBJ_sn2nid(Curve.data())) == 1);
					break;
				case EVP_PKEY_DSA:
				case EVP_PKEY_DSA1:
				case EVP_PKEY_DSA2:
				case EVP_PKEY_DSA3:
				case EVP_PKEY_DSA4:
					Success = (EVP_PKEY_CTX_set_dsa_paramgen_bits(Context, (int)Bits) == 1);
					break;
				case EVP_PKEY_DH:
				case EVP_PKEY_DHX:
					Success = (EVP_PKEY_CTX_set_dh_paramgen_prime_len(Context, (int)Bits) == 1);
					if (Success)
						Success = (EVP_PKEY_CTX_set_dh_nid(Context, Curve.empty() ? NID_ffdhe2048 : OBJ_sn2nid(Curve.data())) == 1);
					break;
				case EVP_PKEY_HMAC:
				case EVP_PKEY_CMAC:
					Success = false;
					break;
				default:
					break;
			}

			EVP_PKEY* Key = nullptr;
			if (!Success || EVP_PKEY_keygen(Context, &Key) != 1)
			{
				EVP_PKEY_CTX_free(Context);
				return CryptoException();
			}

			size_t Length;
			if (EVP_PKEY_get_raw_private_key(Key, nullptr, &Length) != 1)
			{
				EVP_PKEY_free(Key);
				EVP_PKEY_CTX_free(Context);
				return CryptoException();
			}

			Core::String SecretKey;
			SecretKey.resize(Length);

			if (EVP_PKEY_get_raw_private_key(Key, (uint8_t*)SecretKey.data(), &Length) != 1)
			{
				EVP_PKEY_free(Key);
				EVP_PKEY_CTX_free(Context);
				return CryptoException();
			}

			EVP_PKEY_free(Key);
			EVP_PKEY_CTX_free(Context);
			return SecretKey;
#else
			return CryptoException();
#endif
		}
		ExpectsCrypto<Core::String> Crypto::GeneratePublicKey(SignAlg Type, const PrivateKey& SecretKey)
		{
#ifdef VI_OPENSSL
			VI_TRACE("[crypto] generate %s public key from %i bytes", GetSignerName(Type).data(), (int)SecretKey.Size());
			auto LocalKey = SecretKey.Expose<Core::CHUNK_SIZE>();
			EVP_PKEY* Key = EVP_PKEY_new_raw_private_key((int)Type, nullptr, (uint8_t*)LocalKey.Key, LocalKey.Size);
			if (!Key)
				return CryptoException();

			size_t Length;
			if (EVP_PKEY_get_raw_public_key(Key, nullptr, &Length) != 1)
			{
				EVP_PKEY_free(Key);
				return CryptoException();
			}

			Core::String PublicKey;
			PublicKey.resize(Length);

			if (EVP_PKEY_get_raw_public_key(Key, (uint8_t*)PublicKey.data(), &Length) != 1)
			{
				EVP_PKEY_free(Key);
				return CryptoException();
			}

			EVP_PKEY_free(Key);
			return PublicKey;
#else
			return CryptoException();
#endif
		}
		ExpectsCrypto<Core::String> Crypto::ChecksumHex(Digest Type, Core::Stream* Stream)
		{
			auto Data = Crypto::ChecksumRaw(Type, Stream);
			if (!Data)
				return Data;

			return Codec::HexEncode(*Data);
		}
		ExpectsCrypto<Core::String> Crypto::ChecksumRaw(Digest Type, Core::Stream* Stream)
		{
			VI_ASSERT(Type != nullptr, "type should be set");
			VI_ASSERT(Stream != nullptr, "stream should be set");
#ifdef VI_OPENSSL
			VI_TRACE("[crypto] %s stream-hash fd %i", GetDigestName(Type).data(), (int)Stream->GetReadableFd());

			EVP_MD* Method = (EVP_MD*)Type;
			EVP_MD_CTX* Context = EVP_MD_CTX_create();
			if (!Context)
				return CryptoException();

			Core::String Result;
			Result.resize(EVP_MD_size(Method));

			uint32_t Size = 0; bool OK = true;
			OK = EVP_DigestInit_ex(Context, Method, nullptr) == 1 ? OK : false;
			{
				uint8_t Buffer[Core::BLOB_SIZE]; size_t Size = 0;
				while ((Size = Stream->Read(Buffer, sizeof(Buffer)).Or(0)) > 0)
					OK = EVP_DigestUpdate(Context, Buffer, Size) == 1 ? OK : false;
			}
			OK = EVP_DigestFinal_ex(Context, (uint8_t*)Result.data(), &Size) == 1 ? OK : false;
			EVP_MD_CTX_destroy(Context);

			if (!OK)
				return CryptoException();

			Result.resize((size_t)Size);
			return Result;
#else
			return CryptoException();
#endif
		}
		ExpectsCrypto<Core::String> Crypto::HashHex(Digest Type, const std::string_view& Value)
		{
			auto Data = Crypto::HashRaw(Type, Value);
			if (!Data)
				return Data;

			return Codec::HexEncode(*Data);
		}
		ExpectsCrypto<Core::String> Crypto::HashRaw(Digest Type, const std::string_view& Value)
		{
			VI_ASSERT(Type != nullptr, "type should be set");
#ifdef VI_OPENSSL
			VI_TRACE("[crypto] %s hash %" PRIu64 " bytes", GetDigestName(Type).data(), (uint64_t)Value.size());
			if (Value.empty())
				return Core::String(Value);

			EVP_MD* Method = (EVP_MD*)Type;
			EVP_MD_CTX* Context = EVP_MD_CTX_create();
			if (!Context)
				return CryptoException();

			Core::String Result;
			Result.resize(EVP_MD_size(Method));

			uint32_t Size = 0; bool OK = true;
			OK = EVP_DigestInit_ex(Context, Method, nullptr) == 1 ? OK : false;
			OK = EVP_DigestUpdate(Context, Value.data(), Value.size()) == 1 ? OK : false;
			OK = EVP_DigestFinal_ex(Context, (uint8_t*)Result.data(), &Size) == 1 ? OK : false;
			EVP_MD_CTX_destroy(Context);
			if (!OK)
				return CryptoException();

			Result.resize((size_t)Size);
			return Result;
#else
			return CryptoException();
#endif
		}
		ExpectsCrypto<Core::String> Crypto::Sign(Digest Type, SignAlg KeyType, const std::string_view& Value, const PrivateKey& SecretKey)
		{
#ifdef VI_OPENSSL
			VI_TRACE("[crypto] %s sign %" PRIu64 " bytes", GetDigestName(Type).data(), (uint64_t)Value.size());
			if (Value.empty())
				return Core::String();

			auto LocalKey = SecretKey.Expose<Core::CHUNK_SIZE>();
			EVP_PKEY* Key = EVP_PKEY_new_raw_private_key((int)KeyType, nullptr, (uint8_t*)LocalKey.Key, LocalKey.Size);
			if (!Key)
				return CryptoException();

			EVP_MD_CTX* Context = EVP_MD_CTX_create();
			if (EVP_DigestSignInit(Context, nullptr, (EVP_MD*)Type, nullptr, Key) != 1)
			{
				EVP_MD_CTX_free(Context);
				EVP_PKEY_free(Key);
				return CryptoException();
			}

			size_t Length;
			if (EVP_DigestSign(Context, nullptr, &Length, (uint8_t*)Value.data(), Value.size()) != 1)
			{
				EVP_MD_CTX_free(Context);
				EVP_PKEY_free(Key);
				return CryptoException();
			}

			Core::String Signature;
			Signature.resize(Length);

			if (EVP_DigestSign(Context, (uint8_t*)Signature.data(), &Length, (uint8_t*)Value.data(), Value.size()) != 1)
			{
				EVP_MD_CTX_free(Context);
				EVP_PKEY_free(Key);
				return CryptoException();
			}

			EVP_MD_CTX_free(Context);
			EVP_PKEY_free(Key);
			return Signature;
#else
			return CryptoException();
#endif
		}
		ExpectsCrypto<void> Crypto::Verify(Digest Type, SignAlg KeyType, const std::string_view& Value, const std::string_view& Signature, const PrivateKey& PublicKey)
		{
#ifdef VI_OPENSSL
			VI_TRACE("[crypto] %s verify %" PRIu64 " bytes", GetDigestName(Type).data(), (uint64_t)(Value.size() + Signature.size()));
			if (Value.empty())
				return CryptoException(-1, "verify:empty");

			auto LocalKey = PublicKey.Expose<Core::CHUNK_SIZE>();
			EVP_PKEY* Key = EVP_PKEY_new_raw_public_key((int)KeyType, nullptr, (uint8_t*)LocalKey.Key, LocalKey.Size);
			if (!Key)
				return CryptoException();

			EVP_MD_CTX* Context = EVP_MD_CTX_create();
			if (EVP_DigestVerifyInit(Context, nullptr, (EVP_MD*)Type, nullptr, Key) != 1)
			{
				EVP_MD_CTX_free(Context);
				EVP_PKEY_free(Key);
				return CryptoException();
			}

			if (EVP_DigestVerify(Context, (uint8_t*)Signature.data(), Signature.size(), (uint8_t*)Value.data(), Value.size()) != 1)
			{
				EVP_MD_CTX_free(Context);
				EVP_PKEY_free(Key);
				return CryptoException();
			}

			EVP_MD_CTX_free(Context);
			EVP_PKEY_free(Key);
			return Core::Expectation::Met;
#else
			return CryptoException();
#endif
		}
		ExpectsCrypto<Core::String> Crypto::HMAC(Digest Type, const std::string_view& Value, const PrivateKey& Key)
		{
			VI_ASSERT(Type != nullptr, "type should be set");
#ifdef VI_OPENSSL
			VI_TRACE("[crypto] hmac-%s sign %" PRIu64 " bytes", GetDigestName(Type).data(), (uint64_t)Value.size());
			if (Value.empty())
				return Core::String();

			auto LocalKey = Key.Expose<Core::CHUNK_SIZE>();
#if OPENSSL_VERSION_MAJOR >= 3
			uint8_t Result[EVP_MAX_MD_SIZE];
			uint32_t Size = sizeof(Result);
			uint8_t* Pointer = ::HMAC((const EVP_MD*)Type, LocalKey.Key, (int)LocalKey.Size, (const uint8_t*)Value.data(), Value.size(), Result, &Size);

			if (!Pointer)
				return CryptoException();

			return Core::String((const char*)Result, Size);
#elif OPENSSL_VERSION_NUMBER >= 0x1010000fL
			VI_TRACE("[crypto] hmac-%s sign %" PRIu64 " bytes", GetDigestName(Type).data(), (uint64_t)Value.size());
			HMAC_CTX* Context = HMAC_CTX_new();
			if (!Context)
				return CryptoException();

			uint8_t Result[EVP_MAX_MD_SIZE];
			if (1 != HMAC_Init_ex(Context, LocalKey.Key, (int)LocalKey.Size, (const EVP_MD*)Type, nullptr))
			{
				HMAC_CTX_free(Context);
				return CryptoException();
			}

			if (1 != HMAC_Update(Context, (const uint8_t*)Value.data(), (int)Value.size()))
			{
				HMAC_CTX_free(Context);
				return CryptoException();
			}

			uint32_t Size = sizeof(Result);
			if (1 != HMAC_Final(Context, Result, &Size))
			{
				HMAC_CTX_free(Context);
				return CryptoException();
			}

			Core::String Output((const char*)Result, Size);
			HMAC_CTX_free(Context);

			return Output;
#else
			VI_TRACE("[crypto] hmac-%s sign %" PRIu64 " bytes", GetDigestName(Type).data(), (uint64_t)Value.size());
			HMAC_CTX Context;
			HMAC_CTX_init(&Context);

			uint8_t Result[EVP_MAX_MD_SIZE];
			if (1 != HMAC_Init_ex(&Context, LocalKey.Key, (int)LocalKey.Size, (const EVP_MD*)Type, nullptr))
			{
				HMAC_CTX_cleanup(&Context);
				return CryptoException();
			}

			if (1 != HMAC_Update(&Context, (const uint8_t*)Value.data(), (int)Value.size()))
			{
				HMAC_CTX_cleanup(&Context);
				return CryptoException();
			}

			uint32_t Size = sizeof(Result);
			if (1 != HMAC_Final(&Context, Result, &Size))
			{
				HMAC_CTX_cleanup(&Context);
				return CryptoException();
			}

			Core::String Output((const char*)Result, Size);
			HMAC_CTX_cleanup(&Context);

			return Output;
#endif
#else
			return CryptoException();
#endif
		}
		ExpectsCrypto<Core::String> Crypto::Encrypt(Cipher Type, const std::string_view& Value, const PrivateKey& Key, const PrivateKey& Salt, int ComplexityBytes)
		{
			VI_ASSERT(ComplexityBytes < 0 || (ComplexityBytes > 0 && ComplexityBytes % 2 == 0), "compexity should be valid 64, 128, 256, etc.");
			VI_ASSERT(Type != nullptr, "type should be set");
			VI_TRACE("[crypto] %s encrypt-%i %" PRIu64 " bytes", GetCipherName(Type), ComplexityBytes, (uint64_t)Value.size());
			if (Value.empty())
				return Core::String();
#ifdef VI_OPENSSL
			EVP_CIPHER_CTX* Context = EVP_CIPHER_CTX_new();
			if (!Context)
				return CryptoException();

			auto LocalKey = Key.Expose<Core::CHUNK_SIZE>();
			if (ComplexityBytes > 0)
			{
				if (1 != EVP_EncryptInit_ex(Context, (const EVP_CIPHER*)Type, nullptr, nullptr, nullptr) || 1 != EVP_CIPHER_CTX_set_key_length(Context, ComplexityBytes))
				{
					EVP_CIPHER_CTX_free(Context);
					return CryptoException();
				}
			}

			auto LocalSalt = Salt.Expose<Core::CHUNK_SIZE>();
			if (1 != EVP_EncryptInit_ex(Context, (const EVP_CIPHER*)Type, nullptr, (const uint8_t*)LocalKey.Key, (const uint8_t*)LocalSalt.Key))
			{
				EVP_CIPHER_CTX_free(Context);
				return CryptoException();
			}

			Core::String Output;
			Output.reserve(Value.size());

			size_t Offset = 0; bool IsFinalized = false;
			uint8_t OutBuffer[Core::BLOB_SIZE + 2048];
			const uint8_t* InBuffer = (const uint8_t*)Value.data();
			while (Offset < Value.size())
			{
				int InSize = std::min<int>(Core::BLOB_SIZE, (int)(Value.size() - Offset)), OutSize = 0;
				if (1 != EVP_EncryptUpdate(Context, OutBuffer, &OutSize, InBuffer + Offset, InSize))
				{
					EVP_CIPHER_CTX_free(Context);
					return CryptoException();
				}

			Finalize:
				size_t OutputOffset = Output.size();
				size_t OutBufferSize = (size_t)OutSize;
				Output.resize(Output.size() + OutBufferSize);
				memcpy((char*)Output.data() + OutputOffset, OutBuffer, OutBufferSize);
				Offset += (size_t)InSize;
				if (Offset < Value.size())
					continue;
				else if (IsFinalized)
					break;
				
				if (1 != EVP_EncryptFinal_ex(Context, OutBuffer, &OutSize))
				{
					EVP_CIPHER_CTX_free(Context);
					return CryptoException();
				}

				IsFinalized = true;
				goto Finalize;
			}

			EVP_CIPHER_CTX_free(Context);
			return Output;
#else
			return CryptoException();
#endif
		}
		ExpectsCrypto<Core::String> Crypto::Decrypt(Cipher Type, const std::string_view& Value, const PrivateKey& Key, const PrivateKey& Salt, int ComplexityBytes)
		{
			VI_ASSERT(ComplexityBytes < 0 || (ComplexityBytes > 0 && ComplexityBytes % 2 == 0), "compexity should be valid 64, 128, 256, etc.");
			VI_ASSERT(Type != nullptr, "type should be set");
			VI_TRACE("[crypto] %s decrypt-%i %" PRIu64 " bytes", GetCipherName(Type), ComplexityBytes, (uint64_t)Value.size());
			if (Value.empty())
				return Core::String();
#ifdef VI_OPENSSL
			EVP_CIPHER_CTX* Context = EVP_CIPHER_CTX_new();
			if (!Context)
				return CryptoException();

			auto LocalKey = Key.Expose<Core::CHUNK_SIZE>();
			if (ComplexityBytes > 0)
			{
				if (1 != EVP_EncryptInit_ex(Context, (const EVP_CIPHER*)Type, nullptr, nullptr, nullptr) || 1 != EVP_CIPHER_CTX_set_key_length(Context, ComplexityBytes))
				{
					EVP_CIPHER_CTX_free(Context);
					return CryptoException();
				}
			}

			auto LocalSalt = Salt.Expose<Core::CHUNK_SIZE>();
			if (1 != EVP_DecryptInit_ex(Context, (const EVP_CIPHER*)Type, nullptr, (const uint8_t*)LocalKey.Key, (const uint8_t*)LocalSalt.Key))
			{
				EVP_CIPHER_CTX_free(Context);
				return CryptoException();
			}

			Core::String Output;
			Output.reserve(Value.size());

			size_t Offset = 0; bool IsFinalized = false;
			uint8_t OutBuffer[Core::BLOB_SIZE + 2048];
			const uint8_t* InBuffer = (const uint8_t*)Value.data();
			while (Offset < Value.size())
			{
				int InSize = std::min<int>(Core::BLOB_SIZE, (int)(Value.size() - Offset)), OutSize = 0;
				if (1 != EVP_DecryptUpdate(Context, OutBuffer, &OutSize, InBuffer + Offset, InSize))
				{
					EVP_CIPHER_CTX_free(Context);
					return CryptoException();
				}

			Finalize:
				size_t OutputOffset = Output.size();
				size_t OutBufferSize = (size_t)OutSize;
				Output.resize(Output.size() + OutBufferSize);
				memcpy((char*)Output.data() + OutputOffset, OutBuffer, OutBufferSize);
				Offset += (size_t)InSize;
				if (Offset < Value.size())
					continue;
				else if (IsFinalized)
					break;
				
				if (1 != EVP_DecryptFinal_ex(Context, OutBuffer, &OutSize))
				{
					EVP_CIPHER_CTX_free(Context);
					return CryptoException();
				}

				IsFinalized = true;
				goto Finalize;
			}

			EVP_CIPHER_CTX_free(Context);
			return Output;
#else
			return CryptoException();
#endif
		}
		ExpectsCrypto<Core::String> Crypto::JWTSign(const std::string_view& Alg, const std::string_view& Payload, const PrivateKey& Key)
		{
			Digest Hash = nullptr;
			if (Alg == "HS256")
				Hash = Digests::SHA256();
			else if (Alg == "HS384")
				Hash = Digests::SHA384();
			else if (Alg == "HS512")
				Hash = Digests::SHA512();

			return Crypto::HMAC(Hash, Payload, Key);
		}
		ExpectsCrypto<Core::String> Crypto::JWTEncode(WebToken* Src, const PrivateKey& Key)
		{
			VI_ASSERT(Src != nullptr, "web token should be set");
			VI_ASSERT(Src->Header != nullptr, "web token header should be set");
			VI_ASSERT(Src->Payload != nullptr, "web token payload should be set");
			Core::String Alg = Src->Header->GetVar("alg").GetBlob();
			if (Alg.empty())
				return CryptoException(-1, "jwt:algorithm_error");

			Core::String Header;
			Core::Schema::ConvertToJSON(Src->Header, [&Header](Core::VarForm, const std::string_view& Buffer) { Header.append(Buffer); });

			Core::String Payload;
			Core::Schema::ConvertToJSON(Src->Payload, [&Payload](Core::VarForm, const std::string_view& Buffer) { Payload.append(Buffer); });

			Core::String Data = Codec::Base64URLEncode(Header) + '.' + Codec::Base64URLEncode(Payload);
			auto Signature = JWTSign(Alg, Data, Key);
			if (!Signature)
				return Signature;

			Src->Signature = *Signature;
			return Data + '.' + Codec::Base64URLEncode(Src->Signature);
		}
		ExpectsCrypto<WebToken*> Crypto::JWTDecode(const std::string_view& Value, const PrivateKey& Key)
		{
			Core::Vector<Core::String> Source = Core::Stringify::Split(Value, '.');
			if (Source.size() != 3)
				return CryptoException(-1, "jwt:format_error");

			size_t Offset = Source[0].size() + Source[1].size() + 1;
			Source[0] = Codec::Base64URLDecode(Source[0]);
			Core::UPtr<Core::Schema> Header = Core::Schema::ConvertFromJSON(Source[0]).Or(nullptr);
			if (!Header)
				return CryptoException(-1, "jwt:header_parser_error");

			Source[1] = Codec::Base64URLDecode(Source[1]);
			Core::UPtr<Core::Schema> Payload = Core::Schema::ConvertFromJSON(Source[1]).Or(nullptr);
			if (!Payload)
				return CryptoException(-1, "jwt:payload_parser_error");

			Source[0] = Header->GetVar("alg").GetBlob();
			auto Signature = JWTSign(Source[0], Value.substr(0, Offset), Key);
			if (!Signature || Codec::Base64URLEncode(*Signature) != Source[2])
				return CryptoException(-1, "jwt:signature_error");

			WebToken* Result = new WebToken();
			Result->Signature = Codec::Base64URLDecode(Source[2]);
			Result->Header = Header.Reset();
			Result->Payload = Payload.Reset();
			return Result;
		}
		ExpectsCrypto<Core::String> Crypto::DocEncrypt(Core::Schema* Src, const PrivateKey& Key, const PrivateKey& Salt)
		{
			VI_ASSERT(Src != nullptr, "schema should be set");
			Core::String Result;
			Core::Schema::ConvertToJSON(Src, [&Result](Core::VarForm, const std::string_view& Buffer) { Result.append(Buffer); });

			auto Data = Encrypt(Ciphers::AES_256_CBC(), Result, Key, Salt);
			if (!Data)
				return Data;

			Result = Codec::Bep45Encode(*Data);
			return Result;
		}
		ExpectsCrypto<Core::Schema*> Crypto::DocDecrypt(const std::string_view& Value, const PrivateKey& Key, const PrivateKey& Salt)
		{
			VI_ASSERT(!Value.empty(), "value should not be empty");
			if (Value.empty())
				return CryptoException(-1, "doc:payload_empty");

			auto Source = Decrypt(Ciphers::AES_256_CBC(), Codec::Bep45Decode(Value), Key, Salt);
			if (!Source)
				return Source.Error();

			auto Result = Core::Schema::ConvertFromJSON(*Source);
			if (!Result)
				return CryptoException(-1, "doc:payload_parser_error");

			return *Result;
		}
		ExpectsCrypto<size_t> Crypto::Encrypt(Cipher Type, Core::Stream* From, Core::Stream* To, const PrivateKey& Key, const PrivateKey& Salt, BlockCallback&& Callback, size_t ReadInterval, int ComplexityBytes)
		{
			VI_ASSERT(ComplexityBytes < 0 || (ComplexityBytes > 0 && ComplexityBytes % 2 == 0), "compexity should be valid 64, 128, 256, etc.");
			VI_ASSERT(ReadInterval > 0, "read interval should be greater than zero.");
			VI_ASSERT(From != nullptr, "from stream should be set");
			VI_ASSERT(To != nullptr, "to stream should be set");
			VI_ASSERT(Type != nullptr, "type should be set");
			VI_TRACE("[crypto] %s stream-encrypt-%i from fd %i to fd %i", GetCipherName(Type), ComplexityBytes, (int)From->GetReadableFd(), (int)To->GetWriteableFd());
#ifdef VI_OPENSSL
			EVP_CIPHER_CTX* Context = EVP_CIPHER_CTX_new();
			if (!Context)
				return CryptoException();

			auto LocalKey = Key.Expose<Core::CHUNK_SIZE>();
			if (ComplexityBytes > 0)
			{
				if (1 != EVP_EncryptInit_ex(Context, (const EVP_CIPHER*)Type, nullptr, nullptr, nullptr) || 1 != EVP_CIPHER_CTX_set_key_length(Context, ComplexityBytes))
				{
					EVP_CIPHER_CTX_free(Context);
					return CryptoException();
				}
			}

			auto LocalSalt = Salt.Expose<Core::CHUNK_SIZE>();
			if (1 != EVP_EncryptInit_ex(Context, (const EVP_CIPHER*)Type, nullptr, (const uint8_t*)LocalKey.Key, (const uint8_t*)LocalSalt.Key))
			{
				EVP_CIPHER_CTX_free(Context);
				return CryptoException();
			}

			size_t Size = 0, InBufferSize = 0, TrailingBufferSize = 0; bool IsFinalized = false;
			uint8_t InBuffer[Core::CHUNK_SIZE], OutBuffer[Core::CHUNK_SIZE + 1024], TrailingBuffer[Core::CHUNK_SIZE];
			while ((InBufferSize = From->Read(InBuffer, sizeof(InBuffer)).Or(0)) > 0)
			{
				int OutSize = 0;
				if (1 != EVP_EncryptUpdate(Context, OutBuffer + TrailingBufferSize, &OutSize, InBuffer, (int)InBufferSize))
				{
					EVP_CIPHER_CTX_free(Context);
					return CryptoException();
				}

			Finalize:
				uint8_t* WriteBuffer = OutBuffer;
				size_t WriteBufferSize = (size_t)OutSize + TrailingBufferSize;
				size_t TrailingOffset = WriteBufferSize % ReadInterval;
				memcpy(WriteBuffer, TrailingBuffer, TrailingBufferSize);
				if (TrailingOffset > 0 && !IsFinalized)
				{
					size_t Offset = WriteBufferSize - TrailingOffset;
					memcpy(TrailingBuffer, WriteBuffer + Offset, TrailingOffset);
					TrailingBufferSize = TrailingOffset;
					WriteBufferSize -= TrailingOffset;
				}
				else
					TrailingBufferSize = 0;

				if (Callback && WriteBufferSize > 0)
					Callback(&WriteBuffer, &WriteBufferSize);

				if (To->Write(WriteBuffer, WriteBufferSize).Or(0) != WriteBufferSize)
				{
					EVP_CIPHER_CTX_free(Context);
					return CryptoException();
				}

				Size += WriteBufferSize;
				if (InBufferSize >= sizeof(InBuffer))
					continue;
				else if (IsFinalized)
					break;

				if (1 != EVP_EncryptFinal_ex(Context, OutBuffer + TrailingBufferSize, &OutSize))
				{
					EVP_CIPHER_CTX_free(Context);
					return CryptoException();
				}

				IsFinalized = true;
				goto Finalize;
			}

			EVP_CIPHER_CTX_free(Context);
			return Size;
#else
			return CryptoException();
#endif
		}
		ExpectsCrypto<size_t> Crypto::Decrypt(Cipher Type, Core::Stream* From, Core::Stream* To, const PrivateKey& Key, const PrivateKey& Salt, BlockCallback&& Callback, size_t ReadInterval, int ComplexityBytes)
		{
			VI_ASSERT(ComplexityBytes < 0 || (ComplexityBytes > 0 && ComplexityBytes % 2 == 0), "compexity should be valid 64, 128, 256, etc.");
			VI_ASSERT(ReadInterval > 0, "read interval should be greater than zero.");
			VI_ASSERT(From != nullptr, "from stream should be set");
			VI_ASSERT(To != nullptr, "to stream should be set");
			VI_ASSERT(Type != nullptr, "type should be set");
			VI_TRACE("[crypto] %s stream-decrypt-%i from fd %i to fd %i", GetCipherName(Type), ComplexityBytes, (int)From->GetReadableFd(), (int)To->GetWriteableFd());
#ifdef VI_OPENSSL
			EVP_CIPHER_CTX* Context = EVP_CIPHER_CTX_new();
			if (!Context)
				return CryptoException();

			auto LocalKey = Key.Expose<Core::CHUNK_SIZE>();
			if (ComplexityBytes > 0)
			{
				if (1 != EVP_EncryptInit_ex(Context, (const EVP_CIPHER*)Type, nullptr, nullptr, nullptr) || 1 != EVP_CIPHER_CTX_set_key_length(Context, ComplexityBytes))
				{
					EVP_CIPHER_CTX_free(Context);
					return CryptoException();
				}
			}

			auto LocalSalt = Salt.Expose<Core::CHUNK_SIZE>();
			if (1 != EVP_DecryptInit_ex(Context, (const EVP_CIPHER*)Type, nullptr, (const uint8_t*)LocalKey.Key, (const uint8_t*)LocalSalt.Key))
			{
				EVP_CIPHER_CTX_free(Context);
				return CryptoException();
			}

			size_t Size = 0, InBufferSize = 0, TrailingBufferSize = 0; bool IsFinalized = false;
			uint8_t InBuffer[Core::CHUNK_SIZE + 1024], OutBuffer[Core::CHUNK_SIZE + 1024], TrailingBuffer[Core::CHUNK_SIZE];
			while ((InBufferSize = From->Read(InBuffer + TrailingBufferSize, sizeof(TrailingBuffer)).Or(0)) > 0)
			{
				uint8_t* ReadBuffer = InBuffer;
				size_t ReadBufferSize = (size_t)InBufferSize + TrailingBufferSize;
				size_t TrailingOffset = ReadBufferSize % ReadInterval;
				memcpy(ReadBuffer, TrailingBuffer, TrailingBufferSize);
				if (TrailingOffset > 0 && !IsFinalized)
				{
					size_t Offset = ReadBufferSize - TrailingOffset;
					memcpy(TrailingBuffer, ReadBuffer + Offset, TrailingOffset);
					TrailingBufferSize = TrailingOffset;
					ReadBufferSize -= TrailingOffset;
				}
				else
					TrailingBufferSize = 0;

				if (Callback && ReadBufferSize > 0)
					Callback(&ReadBuffer, &ReadBufferSize);

				int OutSize = 0;
				if (1 != EVP_DecryptUpdate(Context, OutBuffer, &OutSize, (uint8_t*)ReadBuffer, (int)ReadBufferSize))
				{
					EVP_CIPHER_CTX_free(Context);
					return CryptoException();
				}

			Finalize:
				size_t OutBufferSize = (size_t)OutSize;
				if (To->Write(OutBuffer, OutBufferSize).Or(0) != OutBufferSize)
				{
					EVP_CIPHER_CTX_free(Context);
					return CryptoException();
				}

				Size += OutBufferSize;
				if (InBufferSize >= sizeof(TrailingBuffer))
					continue;
				else if (IsFinalized)
					break;
				
				if (1 != EVP_DecryptFinal_ex(Context, OutBuffer, &OutSize))
				{
					DisplayCryptoLog();
					EVP_CIPHER_CTX_free(Context);
					return CryptoException();
				}

				IsFinalized = true;
				goto Finalize;
			}

			EVP_CIPHER_CTX_free(Context);
			return Size;
#else
			return CryptoException();
#endif
		}
		uint8_t Crypto::RandomUC()
		{
			static const char Alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
			return Alphabet[(size_t)(Random() % (sizeof(Alphabet) - 1))];
		}
		uint64_t Crypto::CRC32(const std::string_view& Data)
		{
			VI_TRACE("[crypto] crc32 %" PRIu64 " bytes", (uint64_t)Data.size());
			int64_t Result = 0xFFFFFFFF;
			int64_t Byte = 0;
			int64_t Mask = 0;
			int64_t Offset = 0;
			size_t Index = 0;

			while (Index < Data.size())
			{
				Byte = Data[Index];
				Result = Result ^ Byte;

				for (Offset = 7; Offset >= 0; Offset--)
				{
					Mask = -(Result & 1);
					Result = (Result >> 1) ^ (0xEDB88320 & Mask);
				}

				Index++;
			}

			return (uint64_t)~Result;
		}
		uint64_t Crypto::Random(uint64_t Min, uint64_t Max)
		{
			uint64_t Raw = 0;
			if (Min > Max)
				return Raw;
#ifdef VI_OPENSSL
			if (RAND_bytes((uint8_t*)&Raw, sizeof(uint64_t)) != 1)
			{
				ERR_clear_error();
				Raw = Random();
			}
#else
			Raw = Random();
#endif
			return Raw % (Max - Min + 1) + Min;
		}
		uint64_t Crypto::Random()
		{
			static std::random_device Device;
			static std::mt19937 Engine(Device());
			static std::uniform_int_distribution<uint64_t> Range;

			return Range(Engine);
		}
		void Crypto::Sha1CollapseBufferBlock(uint32_t* Buffer)
		{
			VI_ASSERT(Buffer != nullptr, "buffer should be set");
			for (int i = 16; --i >= 0;)
				Buffer[i] = 0;
		}
		void Crypto::Sha1ComputeHashBlock(uint32_t* Result, uint32_t* W)
		{
			VI_ASSERT(Result != nullptr, "result should be set");
			VI_ASSERT(W != nullptr, "salt should be set");
			uint32_t A = Result[0];
			uint32_t B = Result[1];
			uint32_t C = Result[2];
			uint32_t D = Result[3];
			uint32_t E = Result[4];
			int R = 0;

#define Sha1Roll(A1, A2) (((A1) << (A2)) | ((A1) >> (32 - (A2))))
#define Sha1Make(F, V) {const uint32_t T = Sha1Roll(A, 5) + (F) + E + V + W[R]; E = D; D = C; C = Sha1Roll(B, 30); B = A; A = T; R++;}

			while (R < 16)
				Sha1Make((B & C) | (~B & D), 0x5a827999);

			while (R < 20)
			{
				W[R] = Sha1Roll((W[R - 3] ^ W[R - 8] ^ W[R - 14] ^ W[R - 16]), 1);
				Sha1Make((B & C) | (~B & D), 0x5a827999);
			}

			while (R < 40)
			{
				W[R] = Sha1Roll((W[R - 3] ^ W[R - 8] ^ W[R - 14] ^ W[R - 16]), 1);
				Sha1Make(B ^ C ^ D, 0x6ed9eba1);
			}

			while (R < 60)
			{
				W[R] = Sha1Roll((W[R - 3] ^ W[R - 8] ^ W[R - 14] ^ W[R - 16]), 1);
				Sha1Make((B & C) | (B & D) | (C & D), 0x8f1bbcdc);
			}

			while (R < 80)
			{
				W[R] = Sha1Roll((W[R - 3] ^ W[R - 8] ^ W[R - 14] ^ W[R - 16]), 1);
				Sha1Make(B ^ C ^ D, 0xca62c1d6);
			}

#undef Sha1Roll
#undef Sha1Make

			Result[0] += A;
			Result[1] += B;
			Result[2] += C;
			Result[3] += D;
			Result[4] += E;
		}
		void Crypto::Sha1Compute(const void* Value, const int Length, char* Hash20)
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_ASSERT(Hash20 != nullptr, "hash of size 20 should be set");

			uint32_t Result[5] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0 };
			const uint8_t* ValueCUC = (const uint8_t*)Value;
			uint32_t W[80];

			const int EndOfFullBlocks = Length - 64;
			int EndCurrentBlock, CurrentBlock = 0;

			while (CurrentBlock <= EndOfFullBlocks)
			{
				EndCurrentBlock = CurrentBlock + 64;
				for (int i = 0; CurrentBlock < EndCurrentBlock; CurrentBlock += 4)
					W[i++] = (uint32_t)ValueCUC[CurrentBlock + 3] | (((uint32_t)ValueCUC[CurrentBlock + 2]) << 8) | (((uint32_t)ValueCUC[CurrentBlock + 1]) << 16) | (((uint32_t)ValueCUC[CurrentBlock]) << 24);
				Sha1ComputeHashBlock(Result, W);
			}

			EndCurrentBlock = Length - CurrentBlock;
			Sha1CollapseBufferBlock(W);

			int LastBlockBytes = 0;
			for (; LastBlockBytes < EndCurrentBlock; LastBlockBytes++)
				W[LastBlockBytes >> 2] |= (uint32_t)ValueCUC[LastBlockBytes + CurrentBlock] << ((3 - (LastBlockBytes & 3)) << 3);

			W[LastBlockBytes >> 2] |= 0x80 << ((3 - (LastBlockBytes & 3)) << 3);
			if (EndCurrentBlock >= 56)
			{
				Sha1ComputeHashBlock(Result, W);
				Sha1CollapseBufferBlock(W);
			}

			W[15] = Length << 3;
			Sha1ComputeHashBlock(Result, W);

			for (int i = 20; --i >= 0;)
				Hash20[i] = (Result[i >> 2] >> (((3 - i) & 0x3) << 3)) & 0xff;
		}
		void Crypto::Sha1Hash20ToHex(const char* Hash20, char* HexString)
		{
			VI_ASSERT(Hash20 != nullptr, "hash of size 20 should be set");
			VI_ASSERT(HexString != nullptr, "result hex should be set");

			const char Hex[] = { "0123456789abcdef" };
			for (int i = 20; --i >= 0;)
			{
				HexString[i << 1] = Hex[(Hash20[i] >> 4) & 0xf];
				HexString[(i << 1) + 1] = Hex[Hash20[i] & 0xf];
			}

			HexString[40] = 0;
		}
		void Crypto::DisplayCryptoLog()
		{
#ifdef VI_OPENSSL
			ERR_print_errors_cb([](const char* Message, size_t Size, void*)
			{
				while (Size > 0 && Core::Stringify::IsWhitespace(Message[Size - 1]))
					--Size;
				VI_ERR("[openssl] %.*s", (int)Size, Message);
				return 0;
			}, nullptr);
#endif
		}

		void Codec::RotateBuffer(uint8_t* Buffer, size_t BufferSize, uint64_t Hash, int8_t Direction)
		{
			Core::String Partition;
			Core::KeyHasher<Core::String> Hasher;
			Partition.reserve(BufferSize);

			constexpr uint8_t Limit = std::numeric_limits<uint8_t>::max() - 1;
			if (Direction < 0)
			{
				Core::Vector<uint8_t> RotatedBuffer(Buffer, Buffer + BufferSize);
				for (size_t i = 0; i < BufferSize; i++)
				{
					Partition.assign((char*)RotatedBuffer.data(), i + 1).back() = (char)(++Hash % Limit);
					uint8_t Rotation = (uint8_t)(Hasher(Partition) % Limit);
					Buffer[i] -= Rotation;
				}
			}
			else
			{
				for (size_t i = 0; i < BufferSize; i++)
				{
					Partition.assign((char*)Buffer, i + 1).back() = (char)(++Hash % Limit);
					uint8_t Rotation = (uint8_t)(Hasher(Partition) % Limit);
					Buffer[i] += Rotation;
				}
			}
		}
		Core::String Codec::Rotate(const std::string_view& Value, uint64_t Hash, int8_t Direction)
		{
			Core::String Result = Core::String(Value);
			RotateBuffer((uint8_t*)Result.data(), Result.size(), Hash, Direction);
			return Result;
		}
		Core::String Codec::Encode64(const char Alphabet[65], const uint8_t* Value, size_t Length, bool Padding)
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_ASSERT(Length > 0, "length should be greater than zero");
			VI_TRACE("[codec] %s encode-64 %" PRIu64 " bytes", Padding ? "padded" : "unpadded", (uint64_t)Length);

			Core::String Result;
			uint8_t Row3[3];
			uint8_t Row4[4];
			uint32_t Offset = 0, Step = 0;

			while (Length--)
			{
				Row3[Offset++] = *(Value++);
				if (Offset != 3)
					continue;

				Row4[0] = (Row3[0] & 0xfc) >> 2;
				Row4[1] = ((Row3[0] & 0x03) << 4) + ((Row3[1] & 0xf0) >> 4);
				Row4[2] = ((Row3[1] & 0x0f) << 2) + ((Row3[2] & 0xc0) >> 6);
				Row4[3] = Row3[2] & 0x3f;

				for (Offset = 0; Offset < 4; Offset++)
					Result += Alphabet[Row4[Offset]];

				Offset = 0;
			}

			if (!Offset)
				return Result;

			for (Step = Offset; Step < 3; Step++)
				Row3[Step] = '\0';

			Row4[0] = (Row3[0] & 0xfc) >> 2;
			Row4[1] = ((Row3[0] & 0x03) << 4) + ((Row3[1] & 0xf0) >> 4);
			Row4[2] = ((Row3[1] & 0x0f) << 2) + ((Row3[2] & 0xc0) >> 6);
			Row4[3] = Row3[2] & 0x3f;

			for (Step = 0; (Step < Offset + 1); Step++)
				Result += Alphabet[Row4[Step]];

			if (!Padding)
				return Result;

			while (Offset++ < 3)
				Result += '=';

			return Result;
		}
		Core::String Codec::Decode64(const char Alphabet[65], const uint8_t* Value, size_t Length, bool(*IsAlphabetic)(uint8_t))
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_ASSERT(IsAlphabetic != nullptr, "callback should be set");
			VI_ASSERT(Length > 0, "length should be greater than zero");
			VI_TRACE("[codec] decode-64 %" PRIu64 " bytes", (uint64_t)Length);

			Core::String Result;
			uint8_t Row4[4];
			uint8_t Row3[3];
			uint32_t Offset = 0, Step = 0;
			uint32_t Focus = 0;

			while (Length-- && (Value[Focus] != '=') && IsAlphabetic(Value[Focus]))
			{
				Row4[Offset++] = Value[Focus]; Focus++;
				if (Offset != 4)
					continue;

				for (Offset = 0; Offset < 4; Offset++)
					Row4[Offset] = (uint8_t)OffsetOf64(Alphabet, Row4[Offset]);

				Row3[0] = (Row4[0] << 2) + ((Row4[1] & 0x30) >> 4);
				Row3[1] = ((Row4[1] & 0xf) << 4) + ((Row4[2] & 0x3c) >> 2);
				Row3[2] = ((Row4[2] & 0x3) << 6) + Row4[3];

				for (Offset = 0; (Offset < 3); Offset++)
					Result += Row3[Offset];

				Offset = 0;
			}

			if (!Offset)
				return Result;

			for (Step = Offset; Step < 4; Step++)
				Row4[Step] = 0;

			for (Step = 0; Step < 4; Step++)
				Row4[Step] = (uint8_t)OffsetOf64(Alphabet, Row4[Step]);

			Row3[0] = (Row4[0] << 2) + ((Row4[1] & 0x30) >> 4);
			Row3[1] = ((Row4[1] & 0xf) << 4) + ((Row4[2] & 0x3c) >> 2);
			Row3[2] = ((Row4[2] & 0x3) << 6) + Row4[3];

			for (Step = 0; (Step < Offset - 1); Step++)
				Result += Row3[Step];

			return Result;
		}
		Core::String Codec::Bep45Encode(const std::string_view& Data)
		{
			static const char From[] = " $%*+-./:";
			static const char To[] = "abcdefghi";

			Core::String Result(Base45Encode(Data));
			for (size_t i = 0; i < sizeof(From) - 1; i++)
				Core::Stringify::Replace(Result, From[i], To[i]);

			return Result;
		}
		Core::String Codec::Bep45Decode(const std::string_view& Data)
		{
			static const char From[] = "abcdefghi";
			static const char To[] = " $%*+-./:";

			Core::String Result(Data);
			for (size_t i = 0; i < sizeof(From) - 1; i++)
				Core::Stringify::Replace(Result, From[i], To[i]);

			return Base45Decode(Result);
		}
		Core::String Codec::Base32Encode(const std::string_view& Value)
		{
			static const char Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
			Core::String Result;
			if (Value.empty())
				return Result;

			size_t Next = (size_t)Value[0], Offset = 1, Remainder = 8;
			Result.reserve((size_t)((double)Value.size() * 1.6));

			while (Remainder > 0 || Offset < Value.size())
			{
				if (Remainder < 5)
				{
					if (Offset < Value.size())
					{
						Next <<= 8;
						Next |= (size_t)Value[Offset++] & 0xFF;
						Remainder += 8;
					}
					else
					{
						size_t Padding = 5 - Remainder;
						Next <<= Padding;
						Remainder += Padding;
					}
				}

				Remainder -= 5;
				size_t Index = 0x1F & (Next >> Remainder);
				Result.push_back(Alphabet[Index % (sizeof(Alphabet) - 1)]);
			}

			return Result;
		}
		Core::String Codec::Base32Decode(const std::string_view& Value)
		{
			Core::String Result;
			Result.reserve(Value.size());

			size_t Prev = 0, BitsLeft = 0;
			for (char Next : Value)
			{
				if (Next == ' ' || Next == '\t' || Next == '\r' || Next == '\n' || Next == '-')
					continue;

				Prev <<= 5;
				if (Next == '0')
					Next = 'O';
				else if (Next == '1')
					Next = 'L';
				else if (Next == '8')
					Next = 'B';

				if ((Next >= 'A' && Next <= 'Z') || (Next >= 'a' && Next <= 'z'))
					Next = (Next & 0x1F) - 1;
				else if (Next >= '2' && Next <= '7')
					Next -= '2' - 26;
				else
					break;

				Prev |= Next;
				BitsLeft += 5;
				if (BitsLeft < 8)
					continue;

				Result.push_back((char)(Prev >> (BitsLeft - 8)));
				BitsLeft -= 8;
			}

			return Result;
		}
		Core::String Codec::Base45Encode(const std::string_view& Data)
		{
			VI_TRACE("[codec] base45 encode %" PRIu64 " bytes", (uint64_t)Data.size());
			static const char Alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";
			Core::String Result;
			size_t Size = Data.size();
			Result.reserve(Size);

			for (size_t i = 0; i < Size; i += 2)
			{
				if (Size - i > 1)
				{
					int x = ((uint8_t)(Data[i]) << 8) + (uint8_t)Data[i + 1];
					uint8_t e = x / (45 * 45);
					x %= 45 * 45;

					uint8_t d = x / 45;
					uint8_t c = x % 45;
					Result += Alphabet[c];
					Result += Alphabet[d];
					Result += Alphabet[e];
				}
				else
				{
					int x = (uint8_t)Data[i];
					uint8_t d = x / 45;
					uint8_t c = x % 45;

					Result += Alphabet[c];
					Result += Alphabet[d];
				}
			}

			return Result;
		}
		Core::String Codec::Base45Decode(const std::string_view& Data)
		{
			VI_TRACE("[codec] base45 decode %" PRIu64 " bytes", (uint64_t)Data.size());
			static uint8_t CharToInt[256] =
			{
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				36, 255, 255, 255, 37, 38, 255, 255, 255, 255, 39, 40, 255, 41, 42, 43,
				0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 44, 255, 255, 255, 255, 255,
				255, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
				25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 35, 255, 255, 255, 255,
				255, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
				25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 35, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
			};

			size_t Size = Data.size();
			Core::String Result(Size, ' ');
			size_t Offset = 0;

			for (size_t i = 0; i < Size; i += 3)
			{
				int x, a, b;
				if (Size - i < 2)
					break;

				if ((255 == (a = (char)CharToInt[(uint8_t)Data[i]])) || (255 == (b = (char)CharToInt[(uint8_t)Data[i + 1]])))
					break;

				x = a + 45 * b;
				if (Size - i >= 3)
				{
					if (255 == (a = (char)CharToInt[(uint8_t)Data[i + 2]]))
						break;

					x += a * 45 * 45;
					Result[Offset++] = x / 256;
					x %= 256;
				}

				Result[Offset++] = x;
			}

			return Core::String(Result.c_str(), Offset);
		}
		Core::String Codec::Base64Encode(const std::string_view& Value)
		{
			static const char Set[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			return Encode64(Set, (uint8_t*)Value.data(), Value.size(), true);
		}
		Core::String Codec::Base64Decode(const std::string_view& Value)
		{
			static const char Set[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			return Decode64(Set, (uint8_t*)Value.data(), Value.size(), IsBase64);
		}
		Core::String Codec::Base64URLEncode(const std::string_view& Value)
		{
			static const char Set[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
			return Encode64(Set, (uint8_t*)Value.data(), Value.size(), false);
		}
		Core::String Codec::Base64URLDecode(const std::string_view& Value)
		{
			static const char Set[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
			size_t Padding = Value.size() % 4;
			if (Padding == 0)
				return Decode64(Set, (uint8_t*)Value.data(), Value.size(), IsBase64URL);

			Core::String Padded = Core::String(Value);
			Padded.append(4 - Padding, '=');
			return Decode64(Set, (uint8_t*)Padded.c_str(), Padded.size(), IsBase64URL);
		}
		Core::String Codec::Shuffle(const char* Value, size_t Size, uint64_t Mask)
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			VI_TRACE("[codec] shuffle %" PRIu64 " bytes", (uint64_t)Size);

			Core::String Result;
			Result.resize(Size);

			int64_t Hash = Mask;
			for (size_t i = 0; i < Size; i++)
			{
				Hash = ((Hash << 5) - Hash) + Value[i];
				Hash = Hash & Hash;
				Result[i] = (char)(Hash % 255);
			}

			return Result;
		}
		ExpectsCompression<Core::String> Codec::Compress(const std::string_view& Data, Compression Type)
		{
#ifdef VI_ZLIB
			VI_TRACE("[codec] compress %" PRIu64 " bytes", (uint64_t)Data.size());
			if (Data.empty())
				return CompressionException(Z_DATA_ERROR, "empty input buffer");

			uLongf Size = compressBound((uLong)Data.size());
			Bytef* Buffer = Core::Memory::Allocate<Bytef>(Size);
			int Code = compress2(Buffer, &Size, (const Bytef*)Data.data(), (uLong)Data.size(), (int)Type);
			if (Code != Z_OK)
			{
				Core::Memory::Deallocate(Buffer);
				return CompressionException(Code, "buffer compression using compress2 error");
			}

			Core::String Output((char*)Buffer, (size_t)Size);
			Core::Memory::Deallocate(Buffer);
			return Output;
#else
			return CompressionException(-1, "unsupported");
#endif
		}
		ExpectsCompression<Core::String> Codec::Decompress(const std::string_view& Data)
		{
#ifdef VI_ZLIB
			VI_TRACE("[codec] decompress %" PRIu64 " bytes", (uint64_t)Data.size());
			if (Data.empty())
				return CompressionException(Z_DATA_ERROR, "empty input buffer");
			
			uLongf SourceSize = (uLong)Data.size();
			uLongf TotalSize = SourceSize * 2;
			while (true)
			{
				uLongf Size = TotalSize, FinalSize = SourceSize;
				Bytef* Buffer = Core::Memory::Allocate<Bytef>(Size);
				int Code = uncompress2(Buffer, &Size, (const Bytef*)Data.data(), &FinalSize);
				if (Code == Z_MEM_ERROR || Code == Z_BUF_ERROR)
				{
					Core::Memory::Deallocate(Buffer);
					TotalSize += SourceSize;
					continue;
				}
				else if (Code != Z_OK)
				{
					Core::Memory::Deallocate(Buffer);
					return CompressionException(Code, "buffer decompression using uncompress2 error");
				}

				Core::String Output((char*)Buffer, (size_t)Size);
				Core::Memory::Deallocate(Buffer);
				return Output;
			}

			return CompressionException(Z_DATA_ERROR, "buffer decompression error");
#else
			return CompressionException(-1, "unsupported");
#endif
		}
		Core::String Codec::HexEncodeOdd(const std::string_view& Value, bool UpperCase)
		{
			VI_TRACE("[codec] hex encode odd %" PRIu64 " bytes", (uint64_t)Value.size());
			static const char HexLowerCase[17] = "0123456789abcdef";
			static const char HexUpperCase[17] = "0123456789ABCDEF";

			Core::String Output;
			Output.reserve(Value.size() * 2);

			const char* Hex = UpperCase ? HexUpperCase : HexLowerCase;
			for (size_t i = 0; i < Value.size(); i++)
			{
				uint8_t C = static_cast<uint8_t>(Value[i]);
				if (C >= 16)
					Output += Hex[C >> 4];
				Output += Hex[C & 0xf];
			}

			return Output;
		}
		Core::String Codec::HexEncode(const std::string_view& Value, bool UpperCase)
		{
			VI_TRACE("[codec] hex encode %" PRIu64 " bytes", (uint64_t)Value.size());
			static const char HexLowerCase[17] = "0123456789abcdef";
			static const char HexUpperCase[17] = "0123456789ABCDEF";
			const char* Hex = UpperCase ? HexUpperCase : HexLowerCase;

			Core::String Output;
			Output.reserve(Value.size() * 2);

			for (size_t i = 0; i < Value.size(); i++)
			{
				uint8_t C = static_cast<uint8_t>(Value[i]);
				Output += Hex[C >> 4];
				Output += Hex[C & 0xf];
			}

			return Output;
		}
		Core::String Codec::HexDecode(const std::string_view& Value)
		{
			VI_TRACE("[codec] hex decode %" PRIu64 " bytes", (uint64_t)Value.size());
			size_t Offset = 0;
			if (Value.size() >= 2 && Value[0] == '0' && Value[1] == 'x')
				Offset = 2;

			Core::String Output;
			Output.reserve(Value.size() / 2);

			char Hex[3] = { 0, 0, 0 };
			for (size_t i = Offset; i < Value.size(); i += 2)
			{
				size_t Size = std::min<size_t>(2, Value.size() - i);
				memcpy(Hex, Value.data() + i, sizeof(char) * Size);
				Output.push_back((char)(int)strtol(Hex, nullptr, 16));
				Hex[1] = 0;
			}

			return Output;
		}
		Core::String Codec::URLEncode(const std::string_view& Text)
		{
			VI_TRACE("[codec] url encode %" PRIu64 " bytes", (uint64_t)Text.size());
			static const char* Unescape = "._-$,;~()";
			static const char* Hex = "0123456789abcdef";

			Core::String Value;
			Value.reserve(Text.size());

			size_t Offset = 0;
			while (Offset < Text.size())
			{
				uint8_t V = Text[Offset++];
				if (!isalnum(V) && !strchr(Unescape, V))
				{
					Value += '%';
					Value += (Hex[V >> 4]);
					Value += (Hex[V & 0xf]);
				}
				else
					Value += V;
			}

			return Value;
		}
		Core::String Codec::URLDecode(const std::string_view& Text)
		{
			VI_TRACE("[codec] url encode %" PRIu64 " bytes", (uint64_t)Text.size());
			Core::String Value;
			uint8_t* Data = (uint8_t*)Text.data();
			size_t Size = Text.size();
			size_t i = 0;

			while (i < Size)
			{
				if (Size >= 2 && i < Size - 2 && Text[i] == '%' && isxdigit(*(Data + i + 1)) && isxdigit(*(Data + i + 2)))
				{
					int A = tolower(*(Data + i + 1)), B = tolower(*(Data + i + 2));
					Value += (char)(((isdigit(A) ? (A - '0') : (A - 'W')) << 4) | (isdigit(B) ? (B - '0') : (B - 'W')));
					i += 2;
				}
				else if (Text[i] != '+')
					Value += Text[i];
				else
					Value += ' ';

				i++;
			}

			return Value;
		}
		Core::String Codec::Base10ToBaseN(uint64_t Value, uint32_t BaseLessThan65)
		{
			VI_ASSERT(BaseLessThan65 >= 1 && BaseLessThan65 <= 64, "base should be between 1 and 64");
			static const char* Base62 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/";
			static const char* Base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			if (Value == 0)
				return "0";

			const char* Base = (BaseLessThan65 == 64 ? Base64 : Base62);
			Core::String Output;

			while (Value > 0)
			{
				Output.insert(0, 1, Base[Value % BaseLessThan65]);
				Value /= BaseLessThan65;
			}

			return Output;
		}
		Core::String Codec::DecimalToHex(uint64_t n)
		{
			const char* Set = "0123456789abcdef";
			Core::String Result;

			do
			{
				Result = Set[n & 15] + Result;
				n >>= 4;
			} while (n > 0);

			return Result;
		}
		size_t Codec::Utf8(int Code, char* Buffer)
		{
			VI_ASSERT(Buffer != nullptr, "buffer should be set");
			if (Code < 0x0080)
			{
				Buffer[0] = (Code & 0x7F);
				return 1;
			}
			else if (Code < 0x0800)
			{
				Buffer[0] = (0xC0 | ((Code >> 6) & 0x1F));
				Buffer[1] = (0x80 | (Code & 0x3F));
				return 2;
			}
			else if (Code < 0xD800)
			{
				Buffer[0] = (0xE0 | ((Code >> 12) & 0xF));
				Buffer[1] = (0x80 | ((Code >> 6) & 0x3F));
				Buffer[2] = (0x80 | (Code & 0x3F));
				return 3;
			}
			else if (Code < 0xE000)
			{
				return 0;
			}
			else if (Code < 0x10000)
			{
				Buffer[0] = (0xE0 | ((Code >> 12) & 0xF));
				Buffer[1] = (0x80 | ((Code >> 6) & 0x3F));
				Buffer[2] = (0x80 | (Code & 0x3F));
				return 3;
			}
			else if (Code < 0x110000)
			{
				Buffer[0] = (0xF0 | ((Code >> 18) & 0x7));
				Buffer[1] = (0x80 | ((Code >> 12) & 0x3F));
				Buffer[2] = (0x80 | ((Code >> 6) & 0x3F));
				Buffer[3] = (0x80 | (Code & 0x3F));
				return 4;
			}

			return 0;
		}
		bool Codec::Hex(char Code, int& Value)
		{
			if (0x20 <= Code && isdigit(Code))
			{
				Value = Code - '0';
				return true;
			}
			else if ('A' <= Code && Code <= 'F')
			{
				Value = Code - 'A' + 10;
				return true;
			}
			else if ('a' <= Code && Code <= 'f')
			{
				Value = Code - 'a' + 10;
				return true;
			}

			return false;
		}
		bool Codec::HexToString(const std::string_view& Data, char* Buffer, size_t BufferLength)
		{
			VI_ASSERT(Buffer != nullptr && BufferLength > 0, "buffer should be set");
			VI_ASSERT(BufferLength >= (3 * Data.size()), "buffer is too small");
			if (Data.empty())
				return false;

			static const char HEX[] = "0123456789abcdef";
			uint8_t* Value = (uint8_t*)Data.data();
			for (size_t i = 0; i < Data.size(); i++)
			{
				if (i > 0)
					Buffer[3 * i - 1] = ' ';
				Buffer[3 * i] = HEX[(Value[i] >> 4) & 0xF];
				Buffer[3 * i + 1] = HEX[Value[i] & 0xF];
			}

			Buffer[3 * Data.size() - 1] = 0;
			return true;
		}
		bool Codec::HexToDecimal(const std::string_view& Text, size_t Index, size_t Count, int& Value)
		{
			VI_ASSERT(Index < Text.size(), "index outside of range");

			Value = 0;
			for (; Count; Index++, Count--)
			{
				if (!Text[Index])
					return false;

				int v = 0;
				if (!Hex(Text[Index], v))
					return false;

				Value = Value * 16 + v;
			}

			return true;
		}
		bool Codec::IsBase64(uint8_t Value)
		{
			return (isalnum(Value) || (Value == '+') || (Value == '/'));
		}
		bool Codec::IsBase64URL(uint8_t Value)
		{
			return (isalnum(Value) || (Value == '-') || (Value == '_'));
		}

		WebToken::WebToken() noexcept : Header(nullptr), Payload(nullptr), Token(nullptr)
		{
		}
		WebToken::WebToken(const std::string_view& Issuer, const std::string_view& Subject, int64_t Expiration) noexcept : Header(Core::Var::Set::Object()), Payload(Core::Var::Set::Object()), Token(nullptr)
		{
			Header->Set("alg", Core::Var::String("HS256"));
			Header->Set("typ", Core::Var::String("JWT"));
			Payload->Set("iss", Core::Var::String(Issuer));
			Payload->Set("sub", Core::Var::String(Subject));
			Payload->Set("exp", Core::Var::Integer(Expiration));
		}
		WebToken::~WebToken() noexcept
		{
			Core::Memory::Release(Header);
			Core::Memory::Release(Payload);
			Core::Memory::Release(Token);
		}
		void WebToken::Unsign()
		{
			Signature.clear();
			Data.clear();
		}
		void WebToken::SetAlgorithm(const std::string_view& Value)
		{
			if (!Header)
				Header = Core::Var::Set::Object();

			Header->Set("alg", Core::Var::String(Value));
			Unsign();
		}
		void WebToken::SetType(const std::string_view& Value)
		{
			if (!Header)
				Header = Core::Var::Set::Object();

			Header->Set("typ", Core::Var::String(Value));
			Unsign();
		}
		void WebToken::SetContentType(const std::string_view& Value)
		{
			if (!Header)
				Header = Core::Var::Set::Object();

			Header->Set("cty", Core::Var::String(Value));
			Unsign();
		}
		void WebToken::SetIssuer(const std::string_view& Value)
		{
			if (!Payload)
				Payload = Core::Var::Set::Object();

			Payload->Set("iss", Core::Var::String(Value));
			Unsign();
		}
		void WebToken::SetSubject(const std::string_view& Value)
		{
			if (!Payload)
				Payload = Core::Var::Set::Object();

			Payload->Set("sub", Core::Var::String(Value));
			Unsign();
		}
		void WebToken::SetId(const std::string_view& Value)
		{
			if (!Payload)
				Payload = Core::Var::Set::Object();

			Payload->Set("jti", Core::Var::String(Value));
			Unsign();
		}
		void WebToken::SetAudience(const Core::Vector<Core::String>& Value)
		{
			Core::Schema* Array = Core::Var::Set::Array();
			for (auto& Item : Value)
				Array->Push(Core::Var::String(Item));

			if (!Payload)
				Payload = Core::Var::Set::Object();

			Payload->Set("aud", Array);
			Unsign();
		}
		void WebToken::SetExpiration(int64_t Value)
		{
			if (!Payload)
				Payload = Core::Var::Set::Object();

			Payload->Set("exp", Core::Var::Integer(Value));
			Unsign();
		}
		void WebToken::SetNotBefore(int64_t Value)
		{
			if (!Payload)
				Payload = Core::Var::Set::Object();

			Payload->Set("nbf", Core::Var::Integer(Value));
			Unsign();
		}
		void WebToken::SetCreated(int64_t Value)
		{
			if (!Payload)
				Payload = Core::Var::Set::Object();

			Payload->Set("iat", Core::Var::Integer(Value));
			Unsign();
		}
		void WebToken::SetRefreshToken(const std::string_view& Value, const PrivateKey& Key, const PrivateKey& Salt)
		{
			Core::Memory::Release(Token);
			auto NewToken = Crypto::DocDecrypt(Value, Key, Salt);
			if (NewToken)
			{
				Token = *NewToken;
				Refresher.assign(Value);
			}
			else
			{
				Token = nullptr;
				Refresher.clear();
			}
		}
		bool WebToken::Sign(const PrivateKey& Key)
		{
			VI_ASSERT(Header != nullptr, "header should be set");
			VI_ASSERT(Payload != nullptr, "payload should be set");
			if (!Data.empty())
				return true;

			auto NewData = Crypto::JWTEncode(this, Key);
			if (!NewData)
				return false;

			Data = *NewData;
			return !Data.empty();
		}
		ExpectsCrypto<Core::String> WebToken::GetRefreshToken(const PrivateKey& Key, const PrivateKey& Salt)
		{
			auto NewToken = Crypto::DocEncrypt(Token, Key, Salt);
			if (!NewToken)
				return NewToken;

			Refresher = *NewToken;
			return Refresher;
		}
		bool WebToken::IsValid() const
		{
			if (!Header || !Payload || Signature.empty())
				return false;

			int64_t Expires = Payload->GetVar("exp").GetInteger();
			return time(nullptr) < Expires;
		}

		Preprocessor::Preprocessor() noexcept : Nested(false)
		{
			Core::Vector<std::pair<Core::String, Condition>> Controls =
			{
				std::make_pair(Core::String("ifdef"), Condition::Exists),
				std::make_pair(Core::String("ifndef"), Condition::NotExists),
				std::make_pair(Core::String("ifeq"), Condition::Equals),
				std::make_pair(Core::String("ifneq"), Condition::NotEquals),
				std::make_pair(Core::String("ifgt"), Condition::Greater),
				std::make_pair(Core::String("ifngt"), Condition::NotGreater),
				std::make_pair(Core::String("ifgte"), Condition::GreaterEquals),
				std::make_pair(Core::String("ifngte"), Condition::NotGreaterEquals),
				std::make_pair(Core::String("iflt"), Condition::Less),
				std::make_pair(Core::String("ifnlt"), Condition::NotLess),
				std::make_pair(Core::String("iflte"), Condition::LessEquals),
				std::make_pair(Core::String("ifnlte"), Condition::NotLessEquals),
				std::make_pair(Core::String("iflte"), Condition::LessEquals),
				std::make_pair(Core::String("ifnlte"), Condition::NotLessEquals)
			};

			ControlFlow.reserve(Controls.size() * 2 + 2);
			ControlFlow["else"] = std::make_pair(Condition::Exists, Controller::Else);
			ControlFlow["endif"] = std::make_pair(Condition::NotExists, Controller::EndIf);

			for (auto& Item : Controls)
			{
				ControlFlow[Item.first] = std::make_pair(Item.second, Controller::StartIf);
				ControlFlow["el" + Item.first] = std::make_pair(Item.second, Controller::ElseIf);
			}
		}
		void Preprocessor::SetIncludeOptions(const IncludeDesc& NewDesc)
		{
			FileDesc = NewDesc;
		}
		void Preprocessor::SetIncludeCallback(ProcIncludeCallback&& Callback)
		{
			Include = std::move(Callback);
		}
		void Preprocessor::SetPragmaCallback(ProcPragmaCallback&& Callback)
		{
			Pragma = std::move(Callback);
		}
		void Preprocessor::SetDirectiveCallback(const std::string_view& Name, ProcDirectiveCallback&& Callback)
		{
			auto It = Directives.find(Core::KeyLookupCast(Name));
			if (It != Directives.end())
			{
				if (Callback)
					It->second = std::move(Callback);
				else
					Directives.erase(It);
			}
			else if (Callback)
				Directives[Core::String(Name)] = std::move(Callback);
			else
				Directives.erase(Core::String(Name));
		}
		void Preprocessor::SetFeatures(const Desc& F)
		{
			Features = F;
		}
		void Preprocessor::AddDefaultDefinitions()
		{
			DefineDynamic("__VERSION__", [](Preprocessor*, const Core::Vector<Core::String>& Args) -> ExpectsPreprocessor<Core::String>
			{
				return Core::ToString<size_t>(Vitex::VERSION);
			});
			DefineDynamic("__DATE__", [](Preprocessor*, const Core::Vector<Core::String>& Args) -> ExpectsPreprocessor<Core::String>
			{
				return EscapeText(Core::DateTime::SerializeLocal(Core::DateTime::Now(), "%b %d %Y"));
			});
			DefineDynamic("__FILE__", [](Preprocessor* Base, const Core::Vector<Core::String>& Args) -> ExpectsPreprocessor<Core::String>
			{
				Core::String Path = Base->GetCurrentFilePath();
				return EscapeText(Core::Stringify::Replace(Path, "\\", "\\\\"));
			});
			DefineDynamic("__LINE__", [](Preprocessor* Base, const Core::Vector<Core::String>& Args) -> ExpectsPreprocessor<Core::String>
			{
				return Core::ToString<size_t>(Base->GetCurrentLineNumber());
			});
			DefineDynamic("__DIRECTORY__", [](Preprocessor* Base, const Core::Vector<Core::String>& Args) -> ExpectsPreprocessor<Core::String>
			{
				Core::String Path = Core::OS::Path::GetDirectory(Base->GetCurrentFilePath().c_str());
				if (!Path.empty() && (Path.back() == '\\' || Path.back() == '/'))
					Path.erase(Path.size() - 1, 1);
				return EscapeText(Core::Stringify::Replace(Path, "\\", "\\\\"));
			});
		}
		ExpectsPreprocessor<void> Preprocessor::Define(const std::string_view& Expression)
		{
			return DefineDynamic(Expression, nullptr);
		}
		ExpectsPreprocessor<void> Preprocessor::DefineDynamic(const std::string_view& Expression, ProcExpansionCallback&& Callback)
		{
			if (Expression.empty())
				return PreprocessorException(PreprocessorError::MacroDefinitionEmpty, 0, ThisFile.Path);

			VI_TRACE("[proc] define %.*s on 0x%" PRIXPTR, (int)Expression.size(), Expression.data(), (void*)this);
			Core::String Name; size_t NameOffset = 0;
			while (NameOffset < Expression.size())
			{
				char V = Expression[NameOffset++];
				if (V != '_' && !Core::Stringify::IsAlphanum(V))
				{
					Name = Expression.substr(0, --NameOffset);
					break;
				}
				else if (NameOffset >= Expression.size())
				{
					Name = Expression.substr(0, NameOffset);
					break;
				}
			}

			bool EmptyParenthesis = false;
			if (Name.empty())
				return PreprocessorException(PreprocessorError::MacroNameEmpty, 0, ThisFile.Path);

			Definition Data; size_t TemplateBegin = NameOffset, TemplateEnd = NameOffset + 1;
			if (TemplateBegin < Expression.size() && Expression[TemplateBegin] == '(')
			{
				int32_t Pose = 1;
				while (TemplateEnd < Expression.size() && Pose > 0)
				{
					char V = Expression[TemplateEnd++];
					if (V == '(')
						++Pose;
					else if (V == ')')
						--Pose;
				}

				if (Pose < 0)
					return PreprocessorException(PreprocessorError::MacroParenthesisDoubleClosed, 0, Expression);
				else if (Pose > 1)
					return PreprocessorException(PreprocessorError::MacroParenthesisNotClosed, 0, Expression);

				Core::String Template = Core::String(Expression.substr(0, TemplateEnd));
				Core::Stringify::Trim(Template);

				if (!ParseArguments(Template, Data.Tokens, false) || Data.Tokens.empty())
					return PreprocessorException(PreprocessorError::MacroDefinitionError, 0, Template);

				Data.Tokens.erase(Data.Tokens.begin());
				EmptyParenthesis = Data.Tokens.empty();
			}

			Core::Stringify::Trim(Name);
			if (TemplateEnd < Expression.size())
			{
				Data.Expansion = Expression.substr(TemplateEnd);
				Core::Stringify::Trim(Data.Expansion);
			}

			if (EmptyParenthesis)
				Name += "()";

			size_t Size = Data.Expansion.size();
			Data.Callback = std::move(Callback);

			if (Size > 0)
				ExpandDefinitions(Data.Expansion, Size);

			Defines[Name] = std::move(Data);
			return Core::Expectation::Met;
		}
		void Preprocessor::Undefine(const std::string_view& Name)
		{
			VI_TRACE("[proc] undefine %.*s on 0x%" PRIXPTR, (int)Name.size(), Name.data(), (void*)this);
			auto It = Defines.find(Core::KeyLookupCast(Name));
			if (It != Defines.end())
				Defines.erase(It);
		}
		void Preprocessor::Clear()
		{
			Defines.clear();
			Sets.clear();
			ThisFile.Path.clear();
			ThisFile.Line = 0;
		}
		bool Preprocessor::IsDefined(const std::string_view& Name) const
		{
			bool Exists = Defines.count(Core::KeyLookupCast(Name)) > 0;
			VI_TRACE("[proc] ifdef %.*s on 0x%: %s" PRIXPTR, (int)Name.size(), Name.data(), (void*)this, Exists ? "yes" : "no");
			return Exists;
		}
		bool Preprocessor::IsDefined(const std::string_view& Name, const std::string_view& Value) const
		{
			auto It = Defines.find(Core::KeyLookupCast(Name));
			if (It != Defines.end())
				return It->second.Expansion == Value;

			return Value.empty();
		}
		ExpectsPreprocessor<void> Preprocessor::Process(const std::string_view& Path, Core::String& Data)
		{
			bool Nesting = SaveResult();
			if (Data.empty() || (!Path.empty() && HasResult(Path)))
			{
				ApplyResult(Nesting);
				return Core::Expectation::Met;
			}

			FileContext LastFile = ThisFile;
			ThisFile.Path = Path;
			ThisFile.Line = 0;

			if (!Path.empty())
				Sets.insert(Core::String(Path));

			auto TokensStatus = ConsumeTokens(Path, Data);
			if (!TokensStatus)
			{
				ThisFile = LastFile;
				ApplyResult(Nesting);
				return TokensStatus;
			}

			size_t Size = Data.size();
			auto ExpansionStatus = ExpandDefinitions(Data, Size);
			if (!ExpansionStatus)
			{
				ThisFile = LastFile;
				ApplyResult(Nesting);
				return ExpansionStatus;
			}

			ThisFile = LastFile;
			ApplyResult(Nesting);
			return Core::Expectation::Met;
		}
		void Preprocessor::ApplyResult(bool WasNested)
		{
			Nested = WasNested;
			if (!Nested)
			{
				Sets.clear();
				ThisFile.Path.clear();
				ThisFile.Line = 0;
			}
		}
		bool Preprocessor::HasResult(const std::string_view& Path)
		{
			return Path != ThisFile.Path && Sets.count(Core::KeyLookupCast(Path)) > 0;
		}
		bool Preprocessor::SaveResult()
		{
			bool Nesting = Nested;
			Nested = true;

			return Nesting;
		}
		ProcDirective Preprocessor::FindNextToken(Core::String& Buffer, size_t& Offset)
		{
			bool HasMultilineComments = !Features.MultilineCommentBegin.empty() && !Features.MultilineCommentEnd.empty();
			bool HasComments = !Features.CommentBegin.empty();
			bool HasStringLiterals = !Features.StringLiterals.empty();
			ProcDirective Result;

			while (Offset < Buffer.size())
			{
				char V = Buffer[Offset];
				if (V == '#' && Offset + 1 < Buffer.size() && !Core::Stringify::IsWhitespace(Buffer[Offset + 1]))
				{
					Result.Start = Offset;
					while (Offset < Buffer.size())
					{
						if (Core::Stringify::IsWhitespace(Buffer[++Offset]))
						{
							Result.Name = Buffer.substr(Result.Start + 1, Offset - Result.Start - 1);
							break;
						}
					}

					Result.End = Result.Start;
					while (Result.End < Buffer.size())
					{
						char N = Buffer[++Result.End];
						if (N == '\r' || N == '\n' || Result.End == Buffer.size())
						{
							Result.Value = Buffer.substr(Offset, Result.End - Offset);
							break;
						}
					}

					Core::Stringify::Trim(Result.Name);
					Core::Stringify::Trim(Result.Value);
					if (Result.Value.size() >= 2)
					{
						if (HasStringLiterals && Result.Value.front() == Result.Value.back() && Features.StringLiterals.find(Result.Value.front()) != Core::String::npos)
						{
							Result.Value = Result.Value.substr(1, Result.Value.size() - 2);
							Result.AsScope = true;
						}
						else if (Result.Value.front() == '<' && Result.Value.back() == '>')
						{
							Result.Value = Result.Value.substr(1, Result.Value.size() - 2);
							Result.AsGlobal = true;
						}
					}

					Result.Found = true;
					break;
				}
				else if (HasMultilineComments && V == Features.MultilineCommentBegin.front() && Offset + Features.MultilineCommentBegin.size() - 1 < Buffer.size())
				{
					if (memcmp(Buffer.c_str() + Offset, Features.MultilineCommentBegin.c_str(), sizeof(char) * Features.MultilineCommentBegin.size()) != 0)
						goto ParseCommentsOrLiterals;

					Offset += Features.MultilineCommentBegin.size();
					Offset = Buffer.find(Features.MultilineCommentEnd, Offset);
					if (Offset == Core::String::npos)
					{
						Offset = Buffer.size();
						break;
					}
					else
						Offset += Features.MultilineCommentBegin.size();
					continue;
				}
				
			ParseCommentsOrLiterals:
				if (HasComments && V == Features.CommentBegin.front() && Offset + Features.CommentBegin.size() - 1 < Buffer.size())
				{
					if (memcmp(Buffer.c_str() + Offset, Features.CommentBegin.c_str(), sizeof(char) * Features.CommentBegin.size()) != 0)
						goto ParseLiterals;

					while (Offset < Buffer.size())
					{
						char N = Buffer[++Offset];
						if (N == '\r' || N == '\n')
							break;
					}
					continue;
				}

			ParseLiterals:
				if (HasStringLiterals && Features.StringLiterals.find(V) != Core::String::npos)
				{
					while (Offset < Buffer.size())
					{
						if (Buffer[++Offset] == V)
							break;
					}
				}
				++Offset;
			}

			return Result;
		}
		ProcDirective Preprocessor::FindNextConditionalToken(Core::String& Buffer, size_t& Offset)
		{
		Retry:
			ProcDirective Next = FindNextToken(Buffer, Offset);
			if (!Next.Found)
				return Next;

			if (ControlFlow.find(Next.Name) == ControlFlow.end())
				goto Retry;

			return Next;
		}
		size_t Preprocessor::ReplaceToken(ProcDirective& Where, Core::String& Buffer, const std::string_view& To)
		{
			Buffer.replace(Where.Start, Where.End - Where.Start, To);
			return Where.Start;
		}
		ExpectsPreprocessor<Core::Vector<Preprocessor::Conditional>> Preprocessor::PrepareConditions(Core::String& Buffer, ProcDirective& Next, size_t& Offset, bool Top)
		{
			Core::Vector<Conditional> Conditions;
			size_t ChildEnding = 0;

			do
			{
				auto Control = ControlFlow.find(Next.Name);
				if (!Conditions.empty())
				{
					auto& Last = Conditions.back();
					if (!Last.TextEnd)
						Last.TextEnd = Next.Start;
				}

				Conditional Block;
				Block.Type = Control->second.first;
				Block.Chaining = Control->second.second != Controller::StartIf;
				Block.Expression = Next.Value;
				Block.TokenStart = Next.Start;
				Block.TokenEnd = Next.End;
				Block.TextStart = Next.End;

				if (ChildEnding > 0)
				{
					Core::String Text = Buffer.substr(ChildEnding, Block.TokenStart - ChildEnding);
					if (!Text.empty())
					{
						Conditional Subblock;
						Subblock.Expression = Text;
						Conditions.back().Childs.emplace_back(std::move(Subblock));
					}
					ChildEnding = 0;
				}

				if (Control->second.second == Controller::StartIf)
				{
					if (Conditions.empty())
					{
						Conditions.emplace_back(std::move(Block));
						continue;
					}

					auto& Base = Conditions.back();
					auto Listing = PrepareConditions(Buffer, Next, Offset, false);
					if (!Listing)
						return Listing;

					size_t LastSize = Base.Childs.size();
					Base.Childs.reserve(LastSize + Listing->size());
					for (auto& Item : *Listing)
						Base.Childs.push_back(Item);

					ChildEnding = Next.End;
					if (LastSize > 0)
						continue;

					Core::String Text = Buffer.substr(Base.TokenEnd, Block.TokenStart - Base.TokenEnd);
					if (!Text.empty())
					{
						Conditional Subblock;
						Subblock.Expression = Text;
						Base.Childs.insert(Base.Childs.begin() + LastSize, std::move(Subblock));
					}
					continue;
				}
				else if (Control->second.second == Controller::Else)
				{
					Block.Type = (Conditions.empty() ? Condition::Equals : (Condition)(-(int32_t)Conditions.back().Type));
					if (Conditions.empty())
						return PreprocessorException(PreprocessorError::ConditionNotOpened, 0, Next.Name);

					Conditions.emplace_back(std::move(Block));
					continue;
				}
				else if (Control->second.second == Controller::ElseIf)
				{
					if (Conditions.empty())
						return PreprocessorException(PreprocessorError::ConditionNotOpened, 0, Next.Name);

					Conditions.emplace_back(std::move(Block));
					continue;
				}
				else if (Control->second.second == Controller::EndIf)
					break;

				return PreprocessorException(PreprocessorError::ConditionError, 0, Next.Name);
			} while ((Next = FindNextConditionalToken(Buffer, Offset)).Found);

			return Conditions;
		}
		Core::String Preprocessor::Evaluate(Core::String& Buffer, const Core::Vector<Conditional>& Conditions)
		{
			Core::String Result;
			for (size_t i = 0; i < Conditions.size(); i++)
			{
				auto& Next = Conditions[i];
				int Case = SwitchCase(Next);
				if (Case == 1)
				{
					if (Next.Childs.empty())
						Result += Buffer.substr(Next.TextStart, Next.TextEnd - Next.TextStart);
					else
						Result += Evaluate(Buffer, Next.Childs);

					while (i + 1 < Conditions.size() && Conditions[i + 1].Chaining)
						++i;
				}
				else if (Case == -1)
					Result += Next.Expression;
			}

			return Result;
		}
		std::pair<Core::String, Core::String> Preprocessor::GetExpressionParts(const std::string_view& Value)
		{
			size_t Offset = 0;
			while (Offset < Value.size())
			{
				char V = Value[Offset];
				if (!Core::Stringify::IsWhitespace(V))
				{
					++Offset;
					continue;
				}

				size_t Count = Offset;
				while (Offset < Value.size() && Core::Stringify::IsWhitespace(Value[++Offset]));

				Core::String Right = Core::String(Value.substr(Offset));
				Core::String Left = Core::String(Value.substr(0, Count));
				Core::Stringify::Trim(Right);
				Core::Stringify::Trim(Left);

				if (!Features.StringLiterals.empty() && Right.front() == Right.back() && Features.StringLiterals.find(Right.front()) != Core::String::npos)
				{
					Right = Right.substr(1, Right.size() - 2);
					Core::Stringify::Trim(Right);
				}

				return std::make_pair(Left, Right);
			}

			Core::String Expression = Core::String(Value);
			Core::Stringify::Trim(Expression);
			return std::make_pair(Expression, Core::String());
		}
		std::pair<Core::String, Core::String> Preprocessor::UnpackExpression(const std::pair<Core::String, Core::String>& Expression)
		{
			auto Left = Defines.find(Expression.first);
			auto Right = Defines.find(Expression.second);
			return std::make_pair(Left == Defines.end() ? Expression.first : Left->second.Expansion, Right == Defines.end() ? Expression.second : Right->second.Expansion);
		}
		int Preprocessor::SwitchCase(const Conditional& Value)
		{
			if (Value.Expression.empty())
				return 1;

			switch (Value.Type)
			{
				case Condition::Exists:
					return IsDefined(Value.Expression) ? 1 : 0;
				case Condition::Equals:
				{
					auto Parts = UnpackExpression(GetExpressionParts(Value.Expression));
					return IsDefined(Parts.first, Parts.second) ? 1 : 0;
				}
				case Condition::Greater:
				{
					auto Parts = UnpackExpression(GetExpressionParts(Value.Expression));
					auto Left = Core::FromString<double>(Parts.first), Right = Core::FromString<double>(Parts.second);
					return (Left ? *Left : 0.0) > (Right ? *Right : 0.0);
				}
				case Condition::GreaterEquals:
				{
					auto Parts = UnpackExpression(GetExpressionParts(Value.Expression));
					auto Left = Core::FromString<double>(Parts.first), Right = Core::FromString<double>(Parts.second);
					return (Left ? *Left : 0.0) >= (Right ? *Right : 0.0);
				}
				case Condition::Less:
				{
					auto Parts = UnpackExpression(GetExpressionParts(Value.Expression));
					auto Left = Core::FromString<double>(Parts.first), Right = Core::FromString<double>(Parts.second);
					return (Left ? *Left : 0.0) < (Right ? *Right : 0.0);
				}
				case Condition::LessEquals:
				{
					auto Parts = UnpackExpression(GetExpressionParts(Value.Expression));
					auto Left = Core::FromString<double>(Parts.first), Right = Core::FromString<double>(Parts.second);
					return (Left ? *Left : 0.0) <= (Right ? *Right : 0.0);
				}
				case Condition::NotExists:
					return !IsDefined(Value.Expression) ? 1 : 0;
				case Condition::NotEquals:
				{
					auto Parts = GetExpressionParts(Value.Expression);
					return !IsDefined(Parts.first, Parts.second) ? 1 : 0;
				}
				case Condition::NotGreater:
				{
					auto Parts = UnpackExpression(GetExpressionParts(Value.Expression));
					auto Left = Core::FromString<double>(Parts.first), Right = Core::FromString<double>(Parts.second);
					return (Left ? *Left : 0.0) <= (Right ? *Right : 0.0);
				}
				case Condition::NotGreaterEquals:
				{
					auto Parts = UnpackExpression(GetExpressionParts(Value.Expression));
					auto Left = Core::FromString<double>(Parts.first), Right = Core::FromString<double>(Parts.second);
					return (Left ? *Left : 0.0) < (Right ? *Right : 0.0);
				}
				case Condition::NotLess:
				{
					auto Parts = UnpackExpression(GetExpressionParts(Value.Expression));
					auto Left = Core::FromString<double>(Parts.first), Right = Core::FromString<double>(Parts.second);
					return (Left ? *Left : 0.0) >= (Right ? *Right : 0.0);
				}
				case Condition::NotLessEquals:
				{
					auto Parts = UnpackExpression(GetExpressionParts(Value.Expression));
					auto Left = Core::FromString<double>(Parts.first), Right = Core::FromString<double>(Parts.second);
					return (Left ? *Left : 0.0) > (Right ? *Right : 0.0);
				}
				case Condition::Text:
				default:
					return -1;
			}
		}
		size_t Preprocessor::GetLinesCount(Core::String& Buffer, size_t End)
		{
			const char* Target = Buffer.c_str();
			size_t Offset = 0, Lines = 0;
			while (Offset < End)
			{
				if (Target[Offset++] == '\n')
					++Lines;
			}

			return Lines;
		}
		ExpectsPreprocessor<void> Preprocessor::ExpandDefinitions(Core::String& Buffer, size_t& Size)
		{
			if (!Size || Defines.empty())
				return Core::Expectation::Met;

			Core::Vector<Core::String> Tokens;
			Core::String Formatter = Buffer.substr(0, Size);
			Buffer.erase(Buffer.begin(), Buffer.begin() + Size);

			for (auto& Item : Defines)
			{
				if (Size < Item.first.size())
					continue;

				if (Item.second.Tokens.empty())
				{
					Tokens.clear();
					if (Item.second.Callback != nullptr)
					{
						size_t FoundOffset = Formatter.find(Item.first);
						size_t TemplateSize = Item.first.size();
						while (FoundOffset != Core::String::npos)
						{
							StoreCurrentLine = [this, &Formatter, FoundOffset, TemplateSize]() { return GetLinesCount(Formatter, FoundOffset + TemplateSize); };
							auto Status = Item.second.Callback(this, Tokens);
							if (!Status)
								return Status.Error();

							Formatter.replace(FoundOffset, TemplateSize, *Status);
							FoundOffset = Formatter.find(Item.first, FoundOffset);
						}
					}
					else
						Core::Stringify::Replace(Formatter, Item.first, Item.second.Expansion);
					continue;
				}
				else if (Size < Item.first.size() + 1)
					continue;

				bool Stringify = Item.second.Expansion.find('#') != Core::String::npos;
				size_t TemplateStart, Offset = 0; Core::String Needle = Item.first + '(';
				while ((TemplateStart = Formatter.find(Needle, Offset)) != Core::String::npos)
				{
					int32_t Pose = 1; size_t TemplateEnd = TemplateStart + Needle.size();
					while (TemplateEnd < Formatter.size() && Pose > 0)
					{
						char V = Formatter[TemplateEnd++];
						if (V == '(')
							++Pose;
						else if (V == ')')
							--Pose;
					}

					if (Pose < 0)
						return PreprocessorException(PreprocessorError::MacroExpansionParenthesisDoubleClosed, TemplateStart, ThisFile.Path);
					else if (Pose > 1)
						return PreprocessorException(PreprocessorError::MacroExpansionParenthesisNotClosed, TemplateStart, ThisFile.Path);

					Core::String Template = Formatter.substr(TemplateStart, TemplateEnd - TemplateStart);
					Tokens.reserve(Item.second.Tokens.size() + 1);
					Tokens.clear();

					if (!ParseArguments(Template, Tokens, false) || Tokens.empty())
						return PreprocessorException(PreprocessorError::MacroExpansionError, TemplateStart, ThisFile.Path);

					if (Tokens.size() - 1 != Item.second.Tokens.size())
						return PreprocessorException(PreprocessorError::MacroExpansionArgumentsError, TemplateStart, Core::Stringify::Text("%i out of %i", (int)Tokens.size() - 1, (int)Item.second.Tokens.size()));

					Core::String Body;
					if (Item.second.Callback != nullptr)
					{
						auto Status = Item.second.Callback(this, Tokens);
						if (!Status)
							return Status.Error();
						Body = std::move(*Status);
					}
					else
						Body = Item.second.Expansion;

					for (size_t i = 0; i < Item.second.Tokens.size(); i++)
					{
						auto& From = Item.second.Tokens[i];
						auto& To = Tokens[i + 1];
						Core::Stringify::Replace(Body, From, To);
						if (Stringify)
							Core::Stringify::Replace(Body, "#" + From, '\"' + To + '\"');
					}

					StoreCurrentLine = [this, &Formatter, TemplateEnd]() { return GetLinesCount(Formatter, TemplateEnd); };
					Core::Stringify::ReplacePart(Formatter, TemplateStart, TemplateEnd, Body);
					Offset = TemplateStart + Body.size();
				}
			}

			Size = Formatter.size();
			Buffer.insert(0, Formatter);
			return Core::Expectation::Met;
		}
		ExpectsPreprocessor<void> Preprocessor::ParseArguments(const std::string_view& Value, Core::Vector<Core::String>& Tokens, bool UnpackLiterals)
		{
			size_t Where = Value.find('(');
			if (Where == Core::String::npos || Value.back() != ')')
				return PreprocessorException(PreprocessorError::MacroDefinitionError, 0, ThisFile.Path);

			std::string_view Data = Value.substr(Where + 1, Value.size() - Where - 2);
			Tokens.emplace_back(Value.substr(0, Where));
			Where = 0;

			size_t Last = 0;
			while (Where < Data.size())
			{
				char V = Data[Where];
				if (V == '\"' || V == '\'')
				{
					while (Where < Data.size())
					{
						char N = Data[++Where];
						if (N == V)
							break;
					}

					if (Where + 1 >= Data.size())
					{
						++Where;
						goto AddValue;
					}
				}
				else if (V == ',' || Where + 1 >= Data.size())
				{
				AddValue:
					Core::String Subvalue = Core::String(Data.substr(Last, Where + 1 >= Data.size() ? Core::String::npos : Where - Last));
					Core::Stringify::Trim(Subvalue);

					if (UnpackLiterals && Subvalue.size() >= 2)
					{
						if (!Features.StringLiterals.empty() && Subvalue.front() == Subvalue.back() && Features.StringLiterals.find(Subvalue.front()) != Core::String::npos)
							Tokens.emplace_back(Subvalue.substr(1, Subvalue.size() - 2));
						else
							Tokens.emplace_back(std::move(Subvalue));
					}
					else
						Tokens.emplace_back(std::move(Subvalue));
					Last = Where + 1;
				}
				++Where;
			}

			return Core::Expectation::Met;
		}
		ExpectsPreprocessor<void> Preprocessor::ConsumeTokens(const std::string_view& Path, Core::String& Buffer)
		{
			size_t Offset = 0;
			while (true)
			{
				auto Next = FindNextToken(Buffer, Offset);
				if (!Next.Found)
					break;

				if (Next.Name == "include")
				{
					if (!Features.Includes)
						return PreprocessorException(PreprocessorError::IncludeDenied, Offset, Core::String(Path) + " << " + Next.Value);

					Core::String Subbuffer;
					FileDesc.Path = Next.Value;
					FileDesc.From = Path;

					IncludeResult File = ResolveInclude(FileDesc, Next.AsGlobal);
					if (HasResult(File.Module))
					{
					SuccessfulInclude:
						Offset = ReplaceToken(Next, Buffer, Subbuffer);
						continue;
					}

					if (!Include)
						return PreprocessorException(PreprocessorError::IncludeDenied, Offset, Core::String(Path) + " << " + Next.Value);

					auto Status = Include(this, File, Subbuffer);
					if (!Status)
						return Status.Error();

					switch (*Status)
					{
						case IncludeType::Preprocess:
						{
							VI_TRACE("[proc] %sinclude preprocess %s%s%s on 0x%" PRIXPTR, File.IsRemote ? "remote " : "", File.IsAbstract ? "abstract " : "", File.IsFile ? "file " : "", File.Module.c_str(), (void*)this);
							if (Subbuffer.empty())
								goto SuccessfulInclude;

							auto ProcessStatus = Process(File.Module, Subbuffer);
							if (ProcessStatus)
								goto SuccessfulInclude;

							return ProcessStatus;
						}
						case IncludeType::Unchanged:
							VI_TRACE("[proc] %sinclude as-is %s%s%s on 0x%" PRIXPTR, File.IsRemote ? "remote " : "", File.IsAbstract ? "abstract " : "", File.IsFile ? "file " : "", File.Module.c_str(), (void*)this);
							goto SuccessfulInclude;
						case IncludeType::Virtual:
							VI_TRACE("[proc] %sinclude virtual %s%s%s on 0x%" PRIXPTR, File.IsRemote ? "remote " : "", File.IsAbstract ? "abstract " : "", File.IsFile ? "file " : "", File.Module.c_str(), (void*)this);
							Subbuffer.clear();
							goto SuccessfulInclude;
						case IncludeType::Error:
						default:
							return PreprocessorException(PreprocessorError::IncludeNotFound, Offset, Core::String(Path) + " << " + Next.Value);
					}
				}
				else if (Next.Name == "pragma")
				{
					Core::Vector<Core::String> Tokens;
					if (!ParseArguments(Next.Value, Tokens, true) || Tokens.empty())
						return PreprocessorException(PreprocessorError::PragmaNotFound, Offset, Next.Value);

					Core::String Name = Tokens.front();
					Tokens.erase(Tokens.begin());
					if (!Pragma)
						continue;
					
					auto Status = Pragma(this, Name, Tokens);
					if (!Status)
						return Status;

					VI_TRACE("[proc] apply pragma %s on 0x%" PRIXPTR, Buffer.substr(Next.Start, Next.End - Next.Start).c_str(), (void*)this);
					Offset = ReplaceToken(Next, Buffer, Core::String());
				}
				else if (Next.Name == "define")
				{
					Define(Next.Value);
					Offset = ReplaceToken(Next, Buffer, "");
				}
				else if (Next.Name == "undef")
				{
					Undefine(Next.Value);
					Offset = ReplaceToken(Next, Buffer, "");
					if (!ExpandDefinitions(Buffer, Offset))
						return PreprocessorException(PreprocessorError::DirectiveExpansionError, Offset, Next.Name);
				}
				else if (Next.Name.size() >= 2 && Next.Name[0] == 'i' && Next.Name[1] == 'f' && ControlFlow.find(Next.Name) != ControlFlow.end())
				{
					size_t Start = Next.Start;
					auto Conditions = PrepareConditions(Buffer, Next, Offset, true);
					if (!Conditions)
						return PreprocessorException(PreprocessorError::ConditionNotClosed, Offset, Next.Name);

					Core::String Result = Evaluate(Buffer, *Conditions);
					Next.Start = Start; Next.End = Offset;
					Offset = ReplaceToken(Next, Buffer, Result);
				}
				else
				{
					auto It = Directives.find(Next.Name);
					if (It == Directives.end())
						continue;

					Core::String Result;
					auto Status = It->second(this, Next, Result);
					if (!Status)
						return Status.Error();

					Offset = ReplaceToken(Next, Buffer, Result);
				}
			}

			return Core::Expectation::Met;
		}
		ExpectsPreprocessor<Core::String> Preprocessor::ResolveFile(const std::string_view& Path, const std::string_view& IncludePath)
		{
			if (!Features.Includes)
				return PreprocessorException(PreprocessorError::IncludeDenied, 0, Core::String(Path) + " << " + Core::String(IncludePath));

			FileContext LastFile = ThisFile;
			ThisFile.Path = Path;
			ThisFile.Line = 0;

			Core::String Subbuffer;
			FileDesc.Path = IncludePath;
			FileDesc.From = Path;

			IncludeResult File = ResolveInclude(FileDesc, !Core::Stringify::FindOf(IncludePath, ":/\\").Found && !Core::Stringify::Find(IncludePath, "./").Found);
			if (HasResult(File.Module))
			{
			IncludeResult:
				ThisFile = LastFile;
				return Subbuffer;
			}

			if (!Include)
			{
				ThisFile = LastFile;
				return PreprocessorException(PreprocessorError::IncludeDenied, 0, Core::String(Path) + " << " + Core::String(IncludePath));
			}

			auto Status = Include(this, File, Subbuffer);
			if (!Status)
			{
				ThisFile = LastFile;
				return Status.Error();
			}

			switch (*Status)
			{
				case IncludeType::Preprocess:
				{
					VI_TRACE("[proc] %sinclude preprocess %s%s%s on 0x%" PRIXPTR, File.IsRemote ? "remote " : "", File.IsAbstract ? "abstract " : "", File.IsFile ? "file " : "", File.Module.c_str(), (void*)this);
					if (Subbuffer.empty())
						goto IncludeResult;

					auto ProcessStatus = Process(File.Module, Subbuffer);
					if (ProcessStatus)
						goto IncludeResult;

					ThisFile = LastFile;
					return ProcessStatus.Error();
				}
				case IncludeType::Unchanged:
					VI_TRACE("[proc] %sinclude as-is %s%s%s on 0x%" PRIXPTR, File.IsRemote ? "remote " : "", File.IsAbstract ? "abstract " : "", File.IsFile ? "file " : "", File.Module.c_str(), (void*)this);
					goto IncludeResult;
				case IncludeType::Virtual:
					VI_TRACE("[proc] %sinclude virtual %s%s%s on 0x%" PRIXPTR, File.IsRemote ? "remote " : "", File.IsAbstract ? "abstract " : "", File.IsFile ? "file " : "", File.Module.c_str(), (void*)this);
					Subbuffer.clear();
					goto IncludeResult;
				case IncludeType::Error:
				default:
					ThisFile = LastFile;
					return PreprocessorException(PreprocessorError::IncludeNotFound, 0, Core::String(Path) + " << " + Core::String(IncludePath));
			}
		}
		const Core::String& Preprocessor::GetCurrentFilePath() const
		{
			return ThisFile.Path;
		}
		size_t Preprocessor::GetCurrentLineNumber()
		{
			if (StoreCurrentLine != nullptr)
			{
				ThisFile.Line = StoreCurrentLine();
				StoreCurrentLine = nullptr;
			}

			return ThisFile.Line + 1;
		}
		IncludeResult Preprocessor::ResolveInclude(const IncludeDesc& Desc, bool AsGlobal)
		{
			IncludeResult Result;
			if (!AsGlobal)
			{
				Core::String Base;
				if (Desc.From.empty())
				{
					auto Directory = Core::OS::Directory::GetWorking();
					if (Directory)
						Base = *Directory;
				}
				else
					Base = Core::OS::Path::GetDirectory(Desc.From.c_str());

				bool IsOriginRemote = (Desc.From.empty() ? false : Core::OS::Path::IsRemote(Base.c_str()));
				bool IsPathRemote = (Desc.Path.empty() ? false : Core::OS::Path::IsRemote(Desc.Path.c_str()));
				if (IsOriginRemote || IsPathRemote)
				{
					Result.Module = Desc.Path;
					Result.IsRemote = true;
					Result.IsFile = true;
					if (!IsOriginRemote)
						return Result;

					Core::Stringify::Replace(Result.Module, "./", "");
					Result.Module.insert(0, Base);
					return Result;
				}
			}

			if (!Core::Stringify::StartsOf(Desc.Path, "/."))
			{
				if (AsGlobal || Desc.Path.empty() || Desc.Root.empty())
				{
					Result.Module = Desc.Path;
					Result.IsAbstract = true;
					Core::Stringify::Replace(Result.Module, '\\', '/');
					return Result;
				}

				auto Module = Core::OS::Path::Resolve(Desc.Path, Desc.Root, false);
				if (Module)
				{
					Result.Module = *Module;
					if (Core::OS::File::IsExists(Result.Module.c_str()))
					{
						Result.IsAbstract = true;
						Result.IsFile = true;
						return Result;
					}
				}

				for (auto It : Desc.Exts)
				{
					Core::String File(Result.Module);
					if (Result.Module.empty())
					{
						auto Target = Core::OS::Path::Resolve(Desc.Path + It, Desc.Root, false);
						if (!Target)
							continue;

						File.assign(*Target);
					}
					else
						File.append(It);

					if (!Core::OS::File::IsExists(File.c_str()))
						continue;

					Result.Module = std::move(File);
					Result.IsAbstract = true;
					Result.IsFile = true;
					return Result;
				}

				Result.Module = Desc.Path;
				Result.IsAbstract = true;
				Core::Stringify::Replace(Result.Module, '\\', '/');
				return Result;
			}
			else if (AsGlobal)
				return Result;

			Core::String Base;
			if (Desc.From.empty())
			{
				auto Directory = Core::OS::Directory::GetWorking();
				if (Directory)
					Base = *Directory;
			}
			else
				Base = Core::OS::Path::GetDirectory(Desc.From.c_str());

			auto Module = Core::OS::Path::Resolve(Desc.Path, Base, true);
			if (Module)
			{
				Result.Module = *Module;
				if (Core::OS::File::IsExists(Result.Module.c_str()))
				{
					Result.IsFile = true;
					return Result;
				}
			}

			for (auto It : Desc.Exts)
			{
				Core::String File(Result.Module);
				if (Result.Module.empty())
				{
					auto Target = Core::OS::Path::Resolve(Desc.Path + It, Desc.Root, true);
					if (!Target)
						continue;

					File.assign(*Target);
				}
				else
					File.append(It);

				if (!Core::OS::File::IsExists(File.c_str()))
					continue;

				Result.Module = std::move(File);
				Result.IsFile = true;
				return Result;
			}

			Result.Module.clear();
			return Result;
		}
	}
}
