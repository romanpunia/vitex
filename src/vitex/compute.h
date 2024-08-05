#ifndef VI_COMPUTE_H
#define VI_COMPUTE_H
#include "core.h"
#include <cmath>
#include <map>
#include <stack>
#include <limits>

namespace Vitex
{
	namespace Compute
	{
		class WebToken;

		enum class RegexState
		{
			Preprocessed = 0,
			Match_Found = -1,
			No_Match = -2,
			Unexpected_Quantifier = -3,
			Unbalanced_Brackets = -4,
			Internal_Error = -5,
			Invalid_Character_Set = -6,
			Invalid_Metacharacter = -7,
			Sumatch_Array_Too_Small = -8,
			Too_Many_Branches = -9,
			Too_Many_Brackets = -10,
		};

		enum class Compression
		{
			None = 0,
			BestSpeed = 1,
			BestCompression = 9,
			Default = -1
		};

		enum class IncludeType
		{
			Error,
			Preprocess,
			Unchanged,
			Virtual
		};

		enum class PreprocessorError
		{
			MacroDefinitionEmpty,
			MacroNameEmpty,
			MacroParenthesisDoubleClosed,
			MacroParenthesisNotClosed,
			MacroDefinitionError,
			MacroExpansionParenthesisDoubleClosed,
			MacroExpansionParenthesisNotClosed,
			MacroExpansionArgumentsError,
			MacroExpansionExecutionError,
			MacroExpansionError,
			ConditionNotOpened,
			ConditionNotClosed,
			ConditionError,
			DirectiveNotFound,
			DirectiveExpansionError,
			IncludeDenied,
			IncludeError,
			IncludeNotFound,
			PragmaNotFound,
			PragmaError,
			ExtensionError
		};

		typedef void* Cipher;
		typedef void* Digest;
		typedef int32_t SignAlg;

		struct VI_OUT IncludeDesc
		{
			Core::Vector<Core::String> Exts;
			Core::String From;
			Core::String Path;
			Core::String Root;
		};

		struct VI_OUT IncludeResult
		{
			Core::String Module;
			bool IsAbstract = false;
			bool IsRemote = false;
			bool IsFile = false;
		};

		struct VI_OUT RegexBracket
		{
			const char* Pointer = nullptr;
			int64_t Length = 0;
			int64_t Branches = 0;
			int64_t BranchesCount = 0;
		};

		struct VI_OUT RegexBranch
		{
			int64_t BracketIndex;
			const char* Pointer;
		};

		struct VI_OUT RegexMatch
		{
			const char* Pointer;
			int64_t Start;
			int64_t End;
			int64_t Length;
			int64_t Steps;
		};

		struct VI_OUT RegexSource
		{
			friend class Regex;

		private:
			Core::String Expression;
			Core::Vector<RegexBracket> Brackets;
			Core::Vector<RegexBranch> Branches;
			int64_t MaxBranches;
			int64_t MaxBrackets;
			int64_t MaxMatches;
			RegexState State;

		public:
			bool IgnoreCase;

		public:
			RegexSource() noexcept;
			RegexSource(const std::string_view& Regexp, bool fIgnoreCase = false, int64_t fMaxMatches = -1, int64_t fMaxBranches = -1, int64_t fMaxBrackets = -1) noexcept;
			RegexSource(const RegexSource& Other) noexcept;
			RegexSource(RegexSource&& Other) noexcept;
			RegexSource& operator =(const RegexSource& V) noexcept;
			RegexSource& operator =(RegexSource&& V) noexcept;
			const Core::String& GetRegex() const;
			int64_t GetMaxBranches() const;
			int64_t GetMaxBrackets() const;
			int64_t GetMaxMatches() const;
			int64_t GetComplexity() const;
			RegexState GetState() const;
			bool IsSimple() const;

		private:
			void Compile();
		};

		struct VI_OUT RegexResult
		{
			friend class Regex;

		private:
			Core::Vector<RegexMatch> Matches;
			RegexState State;
			int64_t Steps;

		public:
			RegexSource* Src;

		public:
			RegexResult() noexcept;
			RegexResult(const RegexResult& Other) noexcept;
			RegexResult(RegexResult&& Other) noexcept;
			RegexResult& operator =(const RegexResult& V) noexcept;
			RegexResult& operator =(RegexResult&& V) noexcept;
			bool Empty() const;
			int64_t GetSteps() const;
			RegexState GetState() const;
			const Core::Vector<RegexMatch>& Get() const;
			Core::Vector<Core::String> ToArray() const;
		};

		struct VI_OUT PrivateKey
		{
		public:
			template <size_t MaxSize>
			struct Exposable
			{
				char Key[MaxSize];
				size_t Size;

				~Exposable() noexcept
				{
					PrivateKey::RandomizeBuffer(Key, Size);
				}
			};

		private:
			Core::Vector<void*> Blocks;
			Core::String Plain;

		private:
			PrivateKey(const std::string_view& Text, bool) noexcept;
			PrivateKey(Core::String&& Text, bool) noexcept;

		public:
			PrivateKey() noexcept;
			PrivateKey(const PrivateKey& Other) noexcept;
			PrivateKey(PrivateKey&& Other) noexcept;
			explicit PrivateKey(const std::string_view& Key) noexcept;
			~PrivateKey() noexcept;
			PrivateKey& operator =(const PrivateKey& V) noexcept;
			PrivateKey& operator =(PrivateKey&& V) noexcept;
			void Clear();
			void Secure(const std::string_view& Key);
			void ExposeToStack(char* Buffer, size_t MaxSize, size_t* OutSize = nullptr) const;
			Core::String ExposeToHeap() const;
			size_t Size() const;

		public:
			template <size_t MaxSize>
			Exposable<MaxSize> Expose() const
			{
				Exposable<MaxSize> Result;
				ExposeToStack(Result.Key, MaxSize, &Result.Size);
				return Result;
			}

		public:
			static PrivateKey GetPlain(Core::String&& Value);
			static PrivateKey GetPlain(const std::string_view& Value);
			static void RandomizeBuffer(char* Data, size_t Size);

		private:
			char LoadPartition(size_t* Dest, size_t Size, size_t Index) const;
			void RollPartition(size_t* Dest, size_t Size, size_t Index) const;
			void FillPartition(size_t* Dest, size_t Size, size_t Index, char Source) const;
			void CopyDistribution(const PrivateKey& Other);
		};

		struct VI_OUT ProcDirective
		{
			Core::String Name;
			Core::String Value;
			size_t Start = 0;
			size_t End = 0;
			bool Found = false;
			bool AsGlobal = false;
			bool AsScope = false;
		};

		struct VI_OUT UInt128
		{
		private:
#ifdef VI_ENDIAN_BIG
			uint64_t Upper, Lower;
#else
			uint64_t Lower, Upper;
#endif
		public:
			UInt128() = default;
			UInt128(const UInt128& Right) = default;
			UInt128(UInt128&& Right) = default;
			UInt128(const std::string_view& Text);
			UInt128(const std::string_view& Text, uint8_t Base);
			UInt128& operator=(const UInt128& Right) = default;
			UInt128& operator=(UInt128&& Right) = default;
			operator bool() const;
			operator uint8_t() const;
			operator uint16_t() const;
			operator uint32_t() const;
			operator uint64_t() const;
			UInt128 operator&(const UInt128& Right) const;
			UInt128& operator&=(const UInt128& Right);
			UInt128 operator|(const UInt128& Right) const;
			UInt128& operator|=(const UInt128& Right);
			UInt128 operator^(const UInt128& Right) const;
			UInt128& operator^=(const UInt128& Right);
			UInt128 operator~() const;
			UInt128 operator<<(const UInt128& Right) const;
			UInt128& operator<<=(const UInt128& Right);
			UInt128 operator>>(const UInt128& Right) const;
			UInt128& operator>>=(const UInt128& Right);
			bool operator!() const;
			bool operator&&(const UInt128& Right) const;
			bool operator||(const UInt128& Right) const;
			bool operator==(const UInt128& Right) const;
			bool operator!=(const UInt128& Right) const;
			bool operator>(const UInt128& Right) const;
			bool operator<(const UInt128& Right) const;
			bool operator>=(const UInt128& Right) const;
			bool operator<=(const UInt128& Right) const;
			UInt128 operator+(const UInt128& Right) const;
			UInt128& operator+=(const UInt128& Right);
			UInt128 operator-(const UInt128& Right) const;
			UInt128& operator-=(const UInt128& Right);
			UInt128 operator*(const UInt128& Right) const;
			UInt128& operator*=(const UInt128& Right);
			UInt128 operator/(const UInt128& Right) const;
			UInt128& operator/=(const UInt128& Right);
			UInt128 operator%(const UInt128& Right) const;
			UInt128& operator%=(const UInt128& Right);
			UInt128& operator++();
			UInt128 operator++(int);
			UInt128& operator--();
			UInt128 operator--(int);
			UInt128 operator+() const;
			UInt128 operator-() const;
			const uint64_t& High() const;
			const uint64_t& Low() const;
			uint8_t Bits() const;
			uint64_t& High();
			uint64_t& Low();
			Core::Decimal ToDecimal() const;
			Core::String ToString(uint8_t Base = 10, uint32_t Length = 0) const;
			VI_OUT friend std::ostream& operator<<(std::ostream& Stream, const UInt128& Right);
			VI_OUT friend UInt128 operator<<(const uint8_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator<<(const uint16_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator<<(const uint32_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator<<(const uint64_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator<<(const int8_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator<<(const int16_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator<<(const int32_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator<<(const int64_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator>>(const uint8_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator>>(const uint16_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator>>(const uint32_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator>>(const uint64_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator>>(const int8_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator>>(const int16_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator>>(const int32_t& Left, const UInt128& Right);
			VI_OUT friend UInt128 operator>>(const int64_t& Left, const UInt128& Right);

		public:
			static UInt128 Min();
			static UInt128 Max();

		private:
			std::pair<UInt128, UInt128> Divide(const UInt128& Left, const UInt128& Right) const;

		public:
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128(const T& Right)
#ifdef VI_ENDIAN_BIG
				: Upper(0), Lower(Right)
#else
				: Lower(Right), Upper(0)
#endif
			{
				if (std::is_signed<T>::value && Right < 0)
					Upper = -1;
			}
			template <typename S, typename T, typename = typename std::enable_if<std::is_integral<S>::value&& std::is_integral<T>::value, void>::type>
			UInt128(const S& UpperRight, const T& LowerRight)
#ifdef VI_ENDIAN_BIG
				: Upper(UpperRight), Lower(LowerRight)
#else
				: Lower(LowerRight), Upper(UpperRight)
#endif
			{
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator=(const T& Right)
			{
				Upper = 0;
				if (std::is_signed<T>::value && Right < 0)
					Upper = -1;

				Lower = Right;
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128 operator&(const T& Right) const
			{
				return UInt128(0, Lower & (uint64_t)Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator&=(const T& Right)
			{
				Upper = 0;
				Lower &= Right;
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128 operator|(const T& Right) const
			{
				return UInt128(Upper, Lower | (uint64_t)Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator|=(const T& Right)
			{
				Lower |= (uint64_t)Right;
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128 operator^(const T& Right) const
			{
				return UInt128(Upper, Lower ^ (uint64_t)Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator^=(const T& Right)
			{
				Lower ^= (uint64_t)Right;
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128 operator<<(const T& Right) const
			{
				return *this << UInt128(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator<<=(const T& Right)
			{
				*this = *this << UInt128(Right);
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128 operator>>(const T& Right) const
			{
				return *this >> UInt128(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator>>=(const T& Right)
			{
				*this = *this >> UInt128(Right);
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator&&(const T& Right) const
			{
				return static_cast <bool> (*this && Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator||(const T& Right) const
			{
				return static_cast <bool> (*this || Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator==(const T& Right) const
			{
				return (!Upper && (Lower == (uint64_t)Right));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator!=(const T& Right) const
			{
				return (Upper || (Lower != (uint64_t)Right));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator>(const T& Right) const
			{
				return (Upper || (Lower > (uint64_t)Right));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator<(const T& Right) const
			{
				return (!Upper) ? (Lower < (uint64_t)Right) : false;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator>=(const T& Right) const
			{
				return ((*this > Right) || (*this == Right));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator<=(const T& Right) const
			{
				return ((*this < Right) || (*this == Right));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128 operator+(const T& Right) const
			{
				return UInt128(Upper + ((Lower + (uint64_t)Right) < Lower), Lower + (uint64_t)Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator+=(const T& Right)
			{
				return *this += UInt128(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128 operator-(const T& Right) const
			{
				return UInt128((uint64_t)(Upper - ((Lower - Right) > Lower)), (uint64_t)(Lower - Right));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator-=(const T& Right)
			{
				return *this = *this - UInt128(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128 operator*(const T& Right) const
			{
				return *this * UInt128(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator*=(const T& Right)
			{
				return *this = *this * UInt128(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128 operator/(const T& Right) const
			{
				return *this / UInt128(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator/=(const T& Right)
			{
				return *this = *this / UInt128(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128 operator%(const T& Right) const
			{
				return *this % UInt128(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt128& operator%=(const T& Right)
			{
				return *this = *this % UInt128(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt128 operator&(const T& Left, const UInt128& Right)
			{
				return Right & Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator&=(T& Left, const UInt128& Right)
			{
				return Left = static_cast <T> (Right & Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt128 operator|(const T& Left, const UInt128& Right)
			{
				return Right | Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator|=(T& Left, const UInt128& Right)
			{
				return Left = static_cast <T> (Right | Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt128 operator^(const T& Left, const UInt128& Right)
			{
				return Right ^ Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator^=(T& Left, const UInt128& Right)
			{
				return Left = static_cast <T> (Right ^ Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator<<=(T& Left, const UInt128& Right)
			{
				return Left = static_cast <T> (UInt128(Left) << Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator>>=(T& Left, const UInt128& Right)
			{
				return Left = static_cast <T> (UInt128(Left) >> Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator==(const T& Left, const UInt128& Right)
			{
				return (!Right.High() && ((uint64_t)Left == Right.Low()));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator!=(const T& Left, const UInt128& Right)
			{
				return (Right.High() || ((uint64_t)Left != Right.Low()));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator>(const T& Left, const UInt128& Right)
			{
				return (!Right.High()) && ((uint64_t)Left > Right.Low());
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator<(const T& Left, const UInt128& Right)
			{
				if (Right.High())
					return true;
				return ((uint64_t)Left < Right.Low());
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator>=(const T& Left, const UInt128& Right)
			{
				if (Right.High())
					return false;
				return ((uint64_t)Left >= Right.Low());
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator<=(const T& Left, const UInt128& Right)
			{
				if (Right.High())
					return true;
				return ((uint64_t)Left <= Right.Low());
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt128 operator+(const T& Left, const UInt128& Right)
			{
				return Right + Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator+=(T& Left, const UInt128& Right)
			{
				return Left = static_cast <T> (Right + Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt128 operator-(const T& Left, const UInt128& Right)
			{
				return -(Right - Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator-=(T& Left, const UInt128& Right)
			{
				return Left = static_cast <T> (-(Right - Left));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt128 operator*(const T& Left, const UInt128& Right)
			{
				return Right * Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator*=(T& Left, const UInt128& Right)
			{
				return Left = static_cast <T> (Right * Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt128 operator/(const T& Left, const UInt128& Right)
			{
				return UInt128(Left) / Right;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator/=(T& Left, const UInt128& Right)
			{
				return Left = static_cast <T> (UInt128(Left) / Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt128 operator%(const T& Left, const UInt128& Right)
			{
				return UInt128(Left) % Right;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator%=(T& Left, const UInt128& Right)
			{
				return Left = static_cast <T> (UInt128(Left) % Right);
			}
		};

		struct VI_OUT UInt256
		{
		private:
#ifdef VI_ENDIAN_BIG
			UInt128 Upper, Lower;
#else
			UInt128 Lower, Upper;
#endif

		public:
			UInt256() = default;
			UInt256(const UInt256& Right) = default;
			UInt256(UInt256&& Right) = default;
			UInt256(const std::string_view& Text);
			UInt256(const std::string_view& Text, uint8_t Base);
			UInt256(const UInt128& UpperRight, const UInt128& LowerRight)
#ifdef VI_ENDIAN_BIG
				: Upper(UpperRight), Lower(LowerRight)
#else
				: Lower(LowerRight), Upper(UpperRight)
#endif
			{
			}
			UInt256(const UInt128& LowerRight)
#ifdef VI_ENDIAN_BIG
				: Upper(UInt128::Min()), Lower(LowerRight)
#else
				: Lower(LowerRight), Upper(UInt128::Min())
#endif
			{
			}
			UInt256& operator=(const UInt256& Right) = default;
			UInt256& operator=(UInt256&& Right) = default;
			operator bool() const;
			operator uint8_t() const;
			operator uint16_t() const;
			operator uint32_t() const;
			operator uint64_t() const;
			operator UInt128() const;
			UInt256 operator&(const UInt128& Right) const;
			UInt256 operator&(const UInt256& Right) const;
			UInt256& operator&=(const UInt128& Right);
			UInt256& operator&=(const UInt256& Right);
			UInt256 operator|(const UInt128& Right) const;
			UInt256 operator|(const UInt256& Right) const;
			UInt256& operator|=(const UInt128& Right);
			UInt256& operator|=(const UInt256& Right);
			UInt256 operator^(const UInt128& Right) const;
			UInt256 operator^(const UInt256& Right) const;
			UInt256& operator^=(const UInt128& Right);
			UInt256& operator^=(const UInt256& Right);
			UInt256 operator~() const;
			UInt256 operator<<(const UInt128& Shift) const;
			UInt256 operator<<(const UInt256& Shift) const;
			UInt256& operator<<=(const UInt128& Shift);
			UInt256& operator<<=(const UInt256& Shift);
			UInt256 operator>>(const UInt128& Shift) const;
			UInt256 operator>>(const UInt256& Shift) const;
			UInt256& operator>>=(const UInt128& Shift);
			UInt256& operator>>=(const UInt256& Shift);
			bool operator!() const;
			bool operator&&(const UInt128& Right) const;
			bool operator&&(const UInt256& Right) const;
			bool operator||(const UInt128& Right) const;
			bool operator||(const UInt256& Right) const;
			bool operator==(const UInt128& Right) const;
			bool operator==(const UInt256& Right) const;
			bool operator!=(const UInt128& Right) const;
			bool operator!=(const UInt256& Right) const;
			bool operator>(const UInt128& Right) const;
			bool operator>(const UInt256& Right) const;
			bool operator<(const UInt128& Right) const;
			bool operator<(const UInt256& Right) const;
			bool operator>=(const UInt128& Right) const;
			bool operator>=(const UInt256& Right) const;
			bool operator<=(const UInt128& Right) const;
			bool operator<=(const UInt256& Right) const;
			UInt256 operator+(const UInt128& Right) const;
			UInt256 operator+(const UInt256& Right) const;
			UInt256& operator+=(const UInt128& Right);
			UInt256& operator+=(const UInt256& Right);
			UInt256 operator-(const UInt128& Right) const;
			UInt256 operator-(const UInt256& Right) const;
			UInt256& operator-=(const UInt128& Right);
			UInt256& operator-=(const UInt256& Right);
			UInt256 operator*(const UInt128& Right) const;
			UInt256 operator*(const UInt256& Right) const;
			UInt256& operator*=(const UInt128& Right);
			UInt256& operator*=(const UInt256& Right);
			UInt256 operator/(const UInt128& Right) const;
			UInt256 operator/(const UInt256& Right) const;
			UInt256& operator/=(const UInt128& Right);
			UInt256& operator/=(const UInt256& Right);
			UInt256 operator%(const UInt128& Right) const;
			UInt256 operator%(const UInt256& Right) const;
			UInt256& operator%=(const UInt128& Right);
			UInt256& operator%=(const UInt256& Right);
			UInt256& operator++();
			UInt256 operator++(int);
			UInt256& operator--();
			UInt256 operator--(int);
			UInt256 operator+() const;
			UInt256 operator-() const;
			const UInt128& High() const;
			const UInt128& Low() const;
			uint16_t Bits() const;
			UInt128& High();
			UInt128& Low();
			Core::Decimal ToDecimal() const;
			Core::String ToString(uint8_t Base = 10, uint32_t Length = 0) const;
			VI_OUT friend UInt256 operator&(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt128& operator&=(UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator|(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt128& operator|=(UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator^(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt128& operator^=(UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator<<(const uint8_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator<<(const uint16_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator<<(const uint32_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator<<(const uint64_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator<<(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator<<(const int8_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator<<(const int16_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator<<(const int32_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator<<(const int64_t& Left, const UInt256& Right);
			VI_OUT friend UInt128& operator<<=(UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator>>(const uint8_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator>>(const uint16_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator>>(const uint32_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator>>(const uint64_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator>>(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator>>(const int8_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator>>(const int16_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator>>(const int32_t& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator>>(const int64_t& Left, const UInt256& Right);
			VI_OUT friend UInt128& operator>>=(UInt128& Left, const UInt256& Right);
			VI_OUT friend bool operator==(const UInt128& Left, const UInt256& Right);
			VI_OUT friend bool operator!=(const UInt128& Left, const UInt256& Right);
			VI_OUT friend bool operator>(const UInt128& Left, const UInt256& Right);
			VI_OUT friend bool operator<(const UInt128& Left, const UInt256& Right);
			VI_OUT friend bool operator>=(const UInt128& Left, const UInt256& Right);
			VI_OUT friend bool operator<=(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator+(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt128& operator+=(UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator-(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt128& operator-=(UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator*(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt128& operator*=(UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator/(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt128& operator/=(UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt256 operator%(const UInt128& Left, const UInt256& Right);
			VI_OUT friend UInt128& operator%=(UInt128& Left, const UInt256& Right);
			VI_OUT friend std::ostream& operator<<(std::ostream& Stream, const UInt256& Right);

		public:
			static UInt256 Min();
			static UInt256 Max();

		private:
			std::pair<UInt256, UInt256> Divide(const UInt256& Left, const UInt256& Right) const;

		public:
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type>
			UInt256(const T& Right)
#ifdef VI_ENDIAN_BIG
				: Upper(UInt128::Min()), Lower(Right)
#else
				: Lower(Right), Upper(UInt128::Min())
#endif
			{
				if (std::is_signed<T>::value && Right < 0)
					Upper = UInt128(-1, -1);
			}
			template <typename S, typename T, typename = typename std::enable_if <std::is_integral<S>::value&& std::is_integral<T>::value, void>::type>
			UInt256(const S& UpperRight, const T& LowerRight)
#ifdef VI_ENDIAN_BIG
				: Upper(UpperRight), Lower(LowerRight)
#else
				: Lower(LowerRight), Upper(UpperRight)
#endif
			{
			}
			template <typename R, typename S, typename T, typename U, typename = typename std::enable_if<std::is_integral<R>::value&& std::is_integral<S>::value&& std::is_integral<T>::value&& std::is_integral<U>::value, void>::type>
			UInt256(const R& upper_lhs, const S& lower_lhs, const T& UpperRight, const U& LowerRight)
#ifdef VI_ENDIAN_BIG
				: Upper(upper_lhs, lower_lhs), Lower(UpperRight, LowerRight)
#else
				: Lower(UpperRight, LowerRight), Upper(upper_lhs, lower_lhs)
#endif
			{
			}
			template <typename T, typename = typename std::enable_if <std::is_integral<T>::value, T>::type>
			UInt256& operator=(const T& Right)
			{
				Upper = UInt128::Min();
				if (std::is_signed<T>::value && Right < 0)
					Upper = UInt128(-1, -1);

				Lower = Right;
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256 operator&(const T& Right) const
			{
				return UInt256(UInt128::Min(), Lower & (UInt128)Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256& operator&=(const T& Right)
			{
				Upper = UInt128::Min();
				Lower &= Right;
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256 operator|(const T& Right) const
			{
				return UInt256(Upper, Lower | UInt128(Right));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256& operator|=(const T& Right)
			{
				Lower |= (UInt128)Right;
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256 operator^(const T& Right) const
			{
				return UInt256(Upper, Lower ^ (UInt128)Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256& operator^=(const T& Right)
			{
				Lower ^= (UInt128)Right;
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256 operator<<(const T& Right) const
			{
				return *this << UInt256(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256& operator<<=(const T& Right)
			{
				*this = *this << UInt256(Right);
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256 operator>>(const T& Right) const
			{
				return *this >> UInt256(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256& operator>>=(const T& Right)
			{
				*this = *this >> UInt256(Right);
				return *this;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator&&(const T& Right) const
			{
				return ((bool)*this && Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator||(const T& Right) const
			{
				return ((bool)*this || Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator==(const T& Right) const
			{
				return (!Upper && (Lower == UInt128(Right)));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator!=(const T& Right) const
			{
				return ((bool)Upper || (Lower != UInt128(Right)));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator>(const T& Right) const
			{
				return ((bool)Upper || (Lower > UInt128(Right)));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator<(const T& Right) const
			{
				return (!Upper) ? (Lower < UInt128(Right)) : false;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator>=(const T& Right) const
			{
				return ((*this > Right) || (*this == Right));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			bool operator<=(const T& Right) const
			{
				return ((*this < Right) || (*this == Right));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256 operator+(const T& Right) const
			{
				return UInt256(Upper + ((Lower + (UInt128)Right) < Lower), Lower + (UInt128)Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256& operator+=(const T& Right)
			{
				return *this += UInt256(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256 operator-(const T& Right) const
			{
				return UInt256(Upper - ((Lower - Right) > Lower), Lower - Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256& operator-=(const T& Right)
			{
				return *this = *this - UInt256(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256 operator*(const T& Right) const
			{
				return *this * UInt256(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256& operator*=(const T& Right)
			{
				return *this = *this * UInt256(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256 operator/(const T& Right) const
			{
				return *this / UInt256(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256& operator/=(const T& Right)
			{
				return *this = *this / UInt256(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256 operator%(const T& Right) const
			{
				return *this % UInt256(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			UInt256& operator%=(const T& Right)
			{
				return *this = *this % UInt256(Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt256 operator&(const T& Left, const UInt256& Right)
			{
				return Right & Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator&=(T& Left, const UInt256& Right)
			{
				return Left = static_cast <T> (Right & Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt256 operator|(const T& Left, const UInt256& Right)
			{
				return Right | Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator|=(T& Left, const UInt256& Right)
			{
				return Left = static_cast <T> (Right | Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt256 operator^(const T& Left, const UInt256& Right)
			{
				return Right ^ Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator^=(T& Left, const UInt256& Right)
			{
				return Left = static_cast <T> (Right ^ Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator<<=(T& Left, const UInt256& Right)
			{
				Left = static_cast <T> (UInt256(Left) << Right);
				return Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator>>=(T& Left, const UInt256& Right)
			{
				return Left = static_cast <T> (UInt256(Left) >> Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator==(const T& Left, const UInt256& Right)
			{
				return (!Right.High() && ((uint64_t)Left == Right.Low()));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator!=(const T& Left, const UInt256& Right)
			{
				return (Right.High() || ((uint64_t)Left != Right.Low()));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator>(const T& Left, const UInt256& Right)
			{
				return Right.High() ? false : ((UInt128)Left > Right.Low());
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator<(const T& Left, const UInt256& Right)
			{
				return Right.High() ? true : ((UInt128)Left < Right.Low());
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator>=(const T& Left, const UInt256& Right)
			{
				return Right.High() ? false : ((UInt128)Left >= Right.Low());
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend bool operator<=(const T& Left, const UInt256& Right)
			{
				return Right.High() ? true : ((UInt128)Left <= Right.Low());
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt256 operator+(const T& Left, const UInt256& Right)
			{
				return Right + Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator+=(T& Left, const UInt256& Right)
			{
				Left = static_cast <T> (Right + Left);
				return Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt256 operator-(const T& Left, const UInt256& Right)
			{
				return -(Right - Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator-=(T& Left, const UInt256& Right)
			{
				return Left = static_cast <T> (-(Right - Left));
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt256 operator*(const T& Left, const UInt256& Right)
			{
				return Right * Left;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator*=(T& Left, const UInt256& Right)
			{
				return Left = static_cast <T> (Right * Left);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt256 operator/(const T& Left, const UInt256& Right)
			{
				return UInt256(Left) / Right;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator/=(T& Left, const UInt256& Right)
			{
				return Left = static_cast <T> (UInt256(Left) / Right);
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend UInt256 operator%(const T& Left, const UInt256& Right)
			{
				return UInt256(Left) % Right;
			}
			template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type >
			friend T& operator%=(T& Left, const UInt256& Right)
			{
				return Left = static_cast <T> (UInt256(Left) % Right);
			}
		};

		class PreprocessorException final : public Core::BasicException
		{
		private:
			PreprocessorError Type;
			size_t Offset;

		public:
			VI_OUT PreprocessorException(PreprocessorError NewType);
			VI_OUT PreprocessorException(PreprocessorError NewType, size_t NewOffset);
			VI_OUT PreprocessorException(PreprocessorError NewType, size_t NewOffset, const std::string_view& Message);
			VI_OUT const char* type() const noexcept override;
			VI_OUT PreprocessorError status() const noexcept;
			VI_OUT size_t offset() const noexcept;
		};

		class CryptoException final : public Core::BasicException
		{
		private:
			size_t ErrorCode;

		public:
			VI_OUT CryptoException();
			VI_OUT CryptoException(size_t ErrorCode, const std::string_view& Message);
			VI_OUT const char* type() const noexcept override;
			VI_OUT size_t error_code() const noexcept;
		};

		class CompressionException final : public Core::BasicException
		{
		private:
			int ErrorCode;

		public:
			VI_OUT CompressionException(int ErrorCode, const std::string_view& Message);
			VI_OUT const char* type() const noexcept override;
			VI_OUT int error_code() const noexcept;
		};

		template <typename V>
		using ExpectsPreprocessor = Core::Expects<V, PreprocessorException>;

		template <typename V>
		using ExpectsCrypto = Core::Expects<V, CryptoException>;

		template <typename V>
		using ExpectsCompression = Core::Expects<V, CompressionException>;

		typedef std::function<ExpectsPreprocessor<IncludeType>(class Preprocessor*, const struct IncludeResult& File, Core::String& Output)> ProcIncludeCallback;
		typedef std::function<ExpectsPreprocessor<void>(class Preprocessor*, const std::string_view& Name, const Core::Vector<Core::String>& Args)> ProcPragmaCallback;
		typedef std::function<ExpectsPreprocessor<void>(class Preprocessor*, const struct ProcDirective&, Core::String& Output)> ProcDirectiveCallback;
		typedef std::function<ExpectsPreprocessor<Core::String>(class Preprocessor*, const Core::Vector<Core::String>& Args)> ProcExpansionCallback;

		class VI_OUT MD5Hasher
		{
		private:
			typedef uint8_t UInt1;
			typedef uint32_t UInt4;

		private:
			uint32_t S11 = 7;
			uint32_t S12 = 12;
			uint32_t S13 = 17;
			uint32_t S14 = 22;
			uint32_t S21 = 5;
			uint32_t S22 = 9;
			uint32_t S23 = 14;
			uint32_t S24 = 20;
			uint32_t S31 = 4;
			uint32_t S32 = 11;
			uint32_t S33 = 16;
			uint32_t S34 = 23;
			uint32_t S41 = 6;
			uint32_t S42 = 10;
			uint32_t S43 = 15;
			uint32_t S44 = 21;

		private:
			bool Finalized;
			UInt1 Buffer[64];
			UInt4 Count[2];
			UInt4 State[4];
			UInt1 Digest[16];

		public:
			MD5Hasher() noexcept;
			void Transform(const UInt1* Buffer, uint32_t Length = 64);
			void Update(const std::string_view& Buffer, uint32_t BlockSize = 64);
			void Update(const uint8_t* Buffer, uint32_t Length, uint32_t BlockSize = 64);
			void Update(const char* Buffer, uint32_t Length, uint32_t BlockSize = 64);
			void Finalize();
			Core::Unique<char> HexDigest() const;
			Core::Unique<uint8_t> RawDigest() const;
			Core::String ToHex() const;
			Core::String ToRaw() const;

		private:
			static void Decode(UInt4* Output, const UInt1* Input, uint32_t Length);
			static void Encode(UInt1* Output, const UInt4* Input, uint32_t Length);
			static void FF(UInt4& A, UInt4 B, UInt4 C, UInt4 D, UInt4 X, UInt4 S, UInt4 AC);
			static void GG(UInt4& A, UInt4 B, UInt4 C, UInt4 D, UInt4 X, UInt4 S, UInt4 AC);
			static void HH(UInt4& A, UInt4 B, UInt4 C, UInt4 D, UInt4 X, UInt4 S, UInt4 AC);
			static void II(UInt4& A, UInt4 B, UInt4 C, UInt4 D, UInt4 X, UInt4 S, UInt4 AC);
			static UInt4 F(UInt4 X, UInt4 Y, UInt4 Z);
			static UInt4 G(UInt4 X, UInt4 Y, UInt4 Z);
			static UInt4 H(UInt4 X, UInt4 Y, UInt4 Z);
			static UInt4 I(UInt4 X, UInt4 Y, UInt4 Z);
			static UInt4 L(UInt4 X, int n);
		};

		class VI_OUT S8Hasher
		{
		public:
			S8Hasher() noexcept = default;
			S8Hasher(const S8Hasher&) noexcept = default;
			S8Hasher(S8Hasher&&) noexcept = default;
			~S8Hasher() noexcept = default;
			S8Hasher& operator=(const S8Hasher&) = default;
			S8Hasher& operator=(S8Hasher&&) noexcept = default;
			inline size_t operator()(uint64_t Value) const noexcept
			{
				Value ^= (size_t)(Value >> 33);
				Value *= 0xFF51AFD7ED558CCD;
				Value ^= (size_t)(Value >> 33);
				Value *= 0xC4CEB9FE1A85EC53;
				Value ^= (size_t)(Value >> 33);
				return (size_t)Value;
			}
			static inline uint64_t Parse(const char Data[8]) noexcept
			{
				uint64_t Result = 0;
				Result |= ((uint64_t)Data[0]);
				Result |= ((uint64_t)Data[1]) << 8;
				Result |= ((uint64_t)Data[2]) << 16;
				Result |= ((uint64_t)Data[3]) << 24;
				Result |= ((uint64_t)Data[4]) << 32;
				Result |= ((uint64_t)Data[5]) << 40;
				Result |= ((uint64_t)Data[6]) << 48;
				Result |= ((uint64_t)Data[7]) << 56;
				return Result;
			}
		};

		class VI_OUT_TS Ciphers
		{
		public:
			static Cipher DES_ECB();
			static Cipher DES_EDE();
			static Cipher DES_EDE3();
			static Cipher DES_EDE_ECB();
			static Cipher DES_EDE3_ECB();
			static Cipher DES_CFB64();
			static Cipher DES_CFB1();
			static Cipher DES_CFB8();
			static Cipher DES_EDE_CFB64();
			static Cipher DES_EDE3_CFB64();
			static Cipher DES_EDE3_CFB1();
			static Cipher DES_EDE3_CFB8();
			static Cipher DES_OFB();
			static Cipher DES_EDE_OFB();
			static Cipher DES_EDE3_OFB();
			static Cipher DES_CBC();
			static Cipher DES_EDE_CBC();
			static Cipher DES_EDE3_CBC();
			static Cipher DES_EDE3_Wrap();
			static Cipher DESX_CBC();
			static Cipher RC4();
			static Cipher RC4_40();
			static Cipher RC4_HMAC_MD5();
			static Cipher IDEA_ECB();
			static Cipher IDEA_CFB64();
			static Cipher IDEA_OFB();
			static Cipher IDEA_CBC();
			static Cipher RC2_ECB();
			static Cipher RC2_CBC();
			static Cipher RC2_40_CBC();
			static Cipher RC2_64_CBC();
			static Cipher RC2_CFB64();
			static Cipher RC2_OFB();
			static Cipher BF_ECB();
			static Cipher BF_CBC();
			static Cipher BF_CFB64();
			static Cipher BF_OFB();
			static Cipher CAST5_ECB();
			static Cipher CAST5_CBC();
			static Cipher CAST5_CFB64();
			static Cipher CAST5_OFB();
			static Cipher RC5_32_12_16_CBC();
			static Cipher RC5_32_12_16_ECB();
			static Cipher RC5_32_12_16_CFB64();
			static Cipher RC5_32_12_16_OFB();
			static Cipher AES_128_ECB();
			static Cipher AES_128_CBC();
			static Cipher AES_128_CFB1();
			static Cipher AES_128_CFB8();
			static Cipher AES_128_CFB128();
			static Cipher AES_128_OFB();
			static Cipher AES_128_CTR();
			static Cipher AES_128_CCM();
			static Cipher AES_128_GCM();
			static Cipher AES_128_XTS();
			static Cipher AES_128_Wrap();
			static Cipher AES_128_WrapPad();
			static Cipher AES_128_OCB();
			static Cipher AES_192_ECB();
			static Cipher AES_192_CBC();
			static Cipher AES_192_CFB1();
			static Cipher AES_192_CFB8();
			static Cipher AES_192_CFB128();
			static Cipher AES_192_OFB();
			static Cipher AES_192_CTR();
			static Cipher AES_192_CCM();
			static Cipher AES_192_GCM();
			static Cipher AES_192_Wrap();
			static Cipher AES_192_WrapPad();
			static Cipher AES_192_OCB();
			static Cipher AES_256_ECB();
			static Cipher AES_256_CBC();
			static Cipher AES_256_CFB1();
			static Cipher AES_256_CFB8();
			static Cipher AES_256_CFB128();
			static Cipher AES_256_OFB();
			static Cipher AES_256_CTR();
			static Cipher AES_256_CCM();
			static Cipher AES_256_GCM();
			static Cipher AES_256_XTS();
			static Cipher AES_256_Wrap();
			static Cipher AES_256_WrapPad();
			static Cipher AES_256_OCB();
			static Cipher AES_128_CBC_HMAC_SHA1();
			static Cipher AES_256_CBC_HMAC_SHA1();
			static Cipher AES_128_CBC_HMAC_SHA256();
			static Cipher AES_256_CBC_HMAC_SHA256();
			static Cipher ARIA_128_ECB();
			static Cipher ARIA_128_CBC();
			static Cipher ARIA_128_CFB1();
			static Cipher ARIA_128_CFB8();
			static Cipher ARIA_128_CFB128();
			static Cipher ARIA_128_CTR();
			static Cipher ARIA_128_OFB();
			static Cipher ARIA_128_GCM();
			static Cipher ARIA_128_CCM();
			static Cipher ARIA_192_ECB();
			static Cipher ARIA_192_CBC();
			static Cipher ARIA_192_CFB1();
			static Cipher ARIA_192_CFB8();
			static Cipher ARIA_192_CFB128();
			static Cipher ARIA_192_CTR();
			static Cipher ARIA_192_OFB();
			static Cipher ARIA_192_GCM();
			static Cipher ARIA_192_CCM();
			static Cipher ARIA_256_ECB();
			static Cipher ARIA_256_CBC();
			static Cipher ARIA_256_CFB1();
			static Cipher ARIA_256_CFB8();
			static Cipher ARIA_256_CFB128();
			static Cipher ARIA_256_CTR();
			static Cipher ARIA_256_OFB();
			static Cipher ARIA_256_GCM();
			static Cipher ARIA_256_CCM();
			static Cipher Camellia_128_ECB();
			static Cipher Camellia_128_CBC();
			static Cipher Camellia_128_CFB1();
			static Cipher Camellia_128_CFB8();
			static Cipher Camellia_128_CFB128();
			static Cipher Camellia_128_OFB();
			static Cipher Camellia_128_CTR();
			static Cipher Camellia_192_ECB();
			static Cipher Camellia_192_CBC();
			static Cipher Camellia_192_CFB1();
			static Cipher Camellia_192_CFB8();
			static Cipher Camellia_192_CFB128();
			static Cipher Camellia_192_OFB();
			static Cipher Camellia_192_CTR();
			static Cipher Camellia_256_ECB();
			static Cipher Camellia_256_CBC();
			static Cipher Camellia_256_CFB1();
			static Cipher Camellia_256_CFB8();
			static Cipher Camellia_256_CFB128();
			static Cipher Camellia_256_OFB();
			static Cipher Camellia_256_CTR();
			static Cipher Chacha20();
			static Cipher Chacha20_Poly1305();
			static Cipher Seed_ECB();
			static Cipher Seed_CBC();
			static Cipher Seed_CFB128();
			static Cipher Seed_OFB();
			static Cipher SM4_ECB();
			static Cipher SM4_CBC();
			static Cipher SM4_CFB128();
			static Cipher SM4_OFB();
			static Cipher SM4_CTR();
		};

		class VI_OUT_TS Digests
		{
		public:
			static Digest MD2();
			static Digest MD4();
			static Digest MD5();
			static Digest MD5_SHA1();
			static Digest Blake2B512();
			static Digest Blake2S256();
			static Digest SHA1();
			static Digest SHA224();
			static Digest SHA256();
			static Digest SHA384();
			static Digest SHA512();
			static Digest SHA512_224();
			static Digest SHA512_256();
			static Digest SHA3_224();
			static Digest SHA3_256();
			static Digest SHA3_384();
			static Digest SHA3_512();
			static Digest Shake128();
			static Digest Shake256();
			static Digest MDC2();
			static Digest RipeMD160();
			static Digest Whirlpool();
			static Digest SM3();
		};

		class VI_OUT_TS Signers
		{
		public:
			static SignAlg PkRSA();
			static SignAlg PkDSA();
			static SignAlg PkDH();
			static SignAlg PkEC();
			static SignAlg PktSIGN();
			static SignAlg PktENC();
			static SignAlg PktEXCH();
			static SignAlg PksRSA();
			static SignAlg PksDSA();
			static SignAlg PksEC();
			static SignAlg RSA();
			static SignAlg RSA2();
			static SignAlg RSA_PSS();
			static SignAlg DSA();
			static SignAlg DSA1();
			static SignAlg DSA2();
			static SignAlg DSA3();
			static SignAlg DSA4();
			static SignAlg DH();
			static SignAlg DHX();
			static SignAlg EC();
			static SignAlg SM2();
			static SignAlg HMAC();
			static SignAlg CMAC();
			static SignAlg SCRYPT();
			static SignAlg TLS1_PRF();
			static SignAlg HKDF();
			static SignAlg POLY1305();
			static SignAlg SIPHASH();
			static SignAlg X25519();
			static SignAlg ED25519();
			static SignAlg X448();
			static SignAlg ED448();
		};

		class VI_OUT_TS Crypto
		{
		public:
			typedef std::function<void(uint8_t**, size_t*)> BlockCallback;

		public:
			static Digest GetDigestByName(const std::string_view& Name);
			static Cipher GetCipherByName(const std::string_view& Name);
			static SignAlg GetSignerByName(const std::string_view& Name);
			static std::string_view GetDigestName(Digest Type);
			static std::string_view GetCipherName(Cipher Type);
			static std::string_view GetSignerName(SignAlg Type);
			static ExpectsCrypto<void> FillRandomBytes(uint8_t* Buffer, size_t Length);
			static ExpectsCrypto<Core::String> RandomBytes(size_t Length);
			static ExpectsCrypto<Core::String> GeneratePrivateKey(SignAlg Type, size_t TargetBits = 2048, const std::string_view& Curve = std::string_view());
			static ExpectsCrypto<Core::String> GeneratePublicKey(SignAlg Type, const PrivateKey& SecretKey);
			static ExpectsCrypto<Core::String> ChecksumHex(Digest Type, Core::Stream* Stream);
			static ExpectsCrypto<Core::String> ChecksumRaw(Digest Type, Core::Stream* Stream);
			static ExpectsCrypto<Core::String> HashHex(Digest Type, const std::string_view& Value);
			static ExpectsCrypto<Core::String> HashRaw(Digest Type, const std::string_view& Value);
			static ExpectsCrypto<Core::String> Sign(Digest Type, SignAlg KeyType, const std::string_view& Value, const PrivateKey& SecretKey);
			static ExpectsCrypto<void> Verify(Digest Type, SignAlg KeyType, const std::string_view& Value, const std::string_view& Signature, const PrivateKey& PublicKey);
			static ExpectsCrypto<Core::String> HMAC(Digest Type, const std::string_view& Value, const PrivateKey& Key);
			static ExpectsCrypto<Core::String> Encrypt(Cipher Type, const std::string_view& Value, const PrivateKey& Key, const PrivateKey& Salt, int ComplexityBytes = -1);
			static ExpectsCrypto<Core::String> Decrypt(Cipher Type, const std::string_view& Value, const PrivateKey& Key, const PrivateKey& Salt, int ComplexityBytes = -1);
			static ExpectsCrypto<Core::String> JWTSign(const std::string_view& Algo, const std::string_view& Payload, const PrivateKey& Key);
			static ExpectsCrypto<Core::String> JWTEncode(WebToken* Src, const PrivateKey& Key);
			static ExpectsCrypto<Core::Unique<WebToken>> JWTDecode(const std::string_view& Value, const PrivateKey& Key);
			static ExpectsCrypto<Core::String> DocEncrypt(Core::Schema* Src, const PrivateKey& Key, const PrivateKey& Salt);
			static ExpectsCrypto<Core::Unique<Core::Schema>> DocDecrypt(const std::string_view& Value, const PrivateKey& Key, const PrivateKey& Salt);
			static ExpectsCrypto<size_t> Encrypt(Cipher Type, Core::Stream* From, Core::Stream* To, const PrivateKey& Key, const PrivateKey& Salt, BlockCallback&& Callback = nullptr, size_t ReadInterval = 1, int ComplexityBytes = -1);
			static ExpectsCrypto<size_t> Decrypt(Cipher Type, Core::Stream* From, Core::Stream* To, const PrivateKey& Key, const PrivateKey& Salt, BlockCallback&& Callback = nullptr, size_t ReadInterval = 1, int ComplexityBytes = -1);
			static uint8_t RandomUC();
			static uint64_t CRC32(const std::string_view& Data);
			static uint64_t Random(uint64_t Min, uint64_t Max);
			static uint64_t Random();
			static void Sha1CollapseBufferBlock(uint32_t* Buffer);
			static void Sha1ComputeHashBlock(uint32_t* Result, uint32_t* W);
			static void Sha1Compute(const void* Value, int Length, char* Hash20);
			static void Sha1Hash20ToHex(const char* Hash20, char* HexString);
			static void DisplayCryptoLog();
		};

		class VI_OUT_TS Codec
		{
		public:
			static void RotateBuffer(uint8_t* Buffer, size_t BufferSize, uint64_t Hash, int8_t Direction);
			static Core::String Rotate(const std::string_view& Value, uint64_t Hash, int8_t Direction);
			static Core::String Encode64(const char Alphabet[65], const uint8_t* Value, size_t Length, bool Padding);
			static Core::String Decode64(const char Alphabet[65], const uint8_t* Value, size_t Length, bool(*IsAlphabetic)(uint8_t));
			static Core::String Bep45Encode(const std::string_view& Value);
			static Core::String Bep45Decode(const std::string_view& Value);
			static Core::String Base32Encode(const std::string_view& Value);
			static Core::String Base32Decode(const std::string_view& Value);
			static Core::String Base45Encode(const std::string_view& Value);
			static Core::String Base45Decode(const std::string_view& Value);
			static Core::String Base64Encode(const std::string_view& Value);
			static Core::String Base64Decode(const std::string_view& Value);
			static Core::String Base64URLEncode(const std::string_view& Value);
			static Core::String Base64URLDecode(const std::string_view& Value);
			static Core::String Shuffle(const char* Value, size_t Size, uint64_t Mask);
			static ExpectsCompression<Core::String> Compress(const std::string_view& Data, Compression Type = Compression::Default);
			static ExpectsCompression<Core::String> Decompress(const std::string_view& Data);
			static Core::String HexEncodeOdd(const std::string_view& Value, bool UpperCase = false);
			static Core::String HexEncode(const std::string_view& Value, bool UpperCase = false);
			static Core::String HexDecode(const std::string_view& Value);
			static Core::String URLEncode(const std::string_view& Text);
			static Core::String URLDecode(const std::string_view& Text);
			static Core::String DecimalToHex(uint64_t V);
			static Core::String Base10ToBaseN(uint64_t Value, uint32_t BaseLessThan65);
			static size_t Utf8(int Code, char* Buffer);
			static bool Hex(char Code, int& Value);
			static bool HexToString(const std::string_view& Data, char* Buffer, size_t BufferLength);
			static bool HexToDecimal(const std::string_view& Text, size_t Index, size_t Size, int& Value);
			static bool IsBase64URL(uint8_t Value);
			static bool IsBase64(uint8_t Value);
		};

		class VI_OUT_TS Regex
		{
			friend RegexSource;

		private:
			static int64_t Meta(const uint8_t* Buffer);
			static int64_t OpLength(const char* Value);
			static int64_t SetLength(const char* Value, int64_t ValueLength);
			static int64_t GetOpLength(const char* Value, int64_t ValueLength);
			static int64_t Quantifier(const char* Value);
			static int64_t ToInt(int64_t x);
			static int64_t HexToInt(const uint8_t* Buffer);
			static int64_t MatchOp(const uint8_t* Value, const uint8_t* Buffer, RegexResult* Info);
			static int64_t MatchSet(const char* Value, int64_t ValueLength, const char* Buffer, RegexResult* Info);
			static int64_t ParseDOH(const char* Buffer, int64_t BufferLength, RegexResult* Info, int64_t Case);
			static int64_t ParseInner(const char* Value, int64_t ValueLength, const char* Buffer, int64_t BufferLength, RegexResult* Info, int64_t Case);
			static int64_t Parse(const char* Buffer, int64_t BufferLength, RegexResult* Info);

		public:
			static bool Match(RegexSource* Value, RegexResult& Result, const std::string_view& Buffer);
			static bool Replace(RegexSource* Value, const std::string_view& ToExpression, Core::String& Buffer);
			static const char* Syntax();
		};

		class VI_OUT WebToken final : public Core::Reference<WebToken>
		{
		public:
			Core::Schema* Header;
			Core::Schema* Payload;
			Core::Schema* Token;
			Core::String Refresher;
			Core::String Signature;
			Core::String Data;

		public:
			WebToken() noexcept;
			WebToken(const std::string_view& Issuer, const std::string_view& Subject, int64_t Expiration) noexcept;
			virtual ~WebToken() noexcept;
			void Unsign();
			void SetAlgorithm(const std::string_view& Value);
			void SetType(const std::string_view& Value);
			void SetContentType(const std::string_view& Value);
			void SetIssuer(const std::string_view& Value);
			void SetSubject(const std::string_view& Value);
			void SetId(const std::string_view& Value);
			void SetAudience(const Core::Vector<Core::String>& Value);
			void SetExpiration(int64_t Value);
			void SetNotBefore(int64_t Value);
			void SetCreated(int64_t Value);
			void SetRefreshToken(const std::string_view& Value, const PrivateKey& Key, const PrivateKey& Salt);
			bool Sign(const PrivateKey& Key);
			ExpectsCrypto<Core::String> GetRefreshToken(const PrivateKey& Key, const PrivateKey& Salt);
			bool IsValid() const;
		};

		class VI_OUT Preprocessor final : public Core::Reference<Preprocessor>
		{
		public:
			struct Desc
			{
				Core::String MultilineCommentBegin = "/*";
				Core::String MultilineCommentEnd = "*/";
				Core::String CommentBegin = "//";
				Core::String StringLiterals = "\"'`";
				bool Pragmas = true;
				bool Includes = true;
				bool Defines = true;
				bool Conditions = true;
			};

		private:
			enum class Controller
			{
				StartIf = 0,
				ElseIf = 1,
				Else = 2,
				EndIf = 3
			};

			enum class Condition
			{
				Exists = 1,
				Equals = 2,
				Greater = 3,
				GreaterEquals = 4,
				Less = 5,
				LessEquals = 6,
				Text = 0,
				NotExists = -1,
				NotEquals = -2,
				NotGreater = -3,
				NotGreaterEquals = -4,
				NotLess = -5,
				NotLessEquals = -6,
			};

			struct Conditional
			{
				Core::Vector<Conditional> Childs;
				Core::String Expression;
				bool Chaining = false;
				Condition Type = Condition::Text;
				size_t TokenStart = 0;
				size_t TokenEnd = 0;
				size_t TextStart = 0;
				size_t TextEnd = 0;
			};

			struct Definition
			{
				Core::Vector<Core::String> Tokens;
				Core::String Expansion;
				ProcExpansionCallback Callback;
			};

		private:
			struct FileContext
			{
				Core::String Path;
				size_t Line = 0;
			} ThisFile;

		private:
			Core::UnorderedMap<Core::String, std::pair<Condition, Controller>> ControlFlow;
			Core::UnorderedMap<Core::String, ProcDirectiveCallback> Directives;
			Core::UnorderedMap<Core::String, Definition> Defines;
			Core::UnorderedSet<Core::String> Sets;
			std::function<size_t()> StoreCurrentLine;
			ProcIncludeCallback Include;
			ProcPragmaCallback Pragma;
			IncludeDesc FileDesc;
			Desc Features;
			bool Nested;

		public:
			Preprocessor() noexcept;
			~Preprocessor() noexcept = default;
			void SetIncludeOptions(const IncludeDesc& NewDesc);
			void SetIncludeCallback(ProcIncludeCallback&& Callback);
			void SetPragmaCallback(ProcPragmaCallback&& Callback);
			void SetDirectiveCallback(const std::string_view& Name, ProcDirectiveCallback&& Callback);
			void SetFeatures(const Desc& Value);
			void AddDefaultDefinitions();
			ExpectsPreprocessor<void> Define(const std::string_view& Expression);
			ExpectsPreprocessor<void> DefineDynamic(const std::string_view& Expression, ProcExpansionCallback&& Callback);
			void Undefine(const std::string_view& Name);
			void Clear();
			bool IsDefined(const std::string_view& Name) const;
			bool IsDefined(const std::string_view& Name, const std::string_view& Value) const;
			ExpectsPreprocessor<void> Process(const std::string_view& Path, Core::String& Buffer);
			ExpectsPreprocessor<Core::String> ResolveFile(const std::string_view& Path, const std::string_view& Include);
			const Core::String& GetCurrentFilePath() const;
			size_t GetCurrentLineNumber();

		private:
			ProcDirective FindNextToken(Core::String& Buffer, size_t& Offset);
			ProcDirective FindNextConditionalToken(Core::String& Buffer, size_t& Offset);
			size_t ReplaceToken(ProcDirective& Where, Core::String& Buffer, const std::string_view& To);
			ExpectsPreprocessor<Core::Vector<Conditional>> PrepareConditions(Core::String& Buffer, ProcDirective& Next, size_t& Offset, bool Top);
			Core::String Evaluate(Core::String& Buffer, const Core::Vector<Conditional>& Conditions);
			std::pair<Core::String, Core::String> GetExpressionParts(const std::string_view& Value);
			std::pair<Core::String, Core::String> UnpackExpression(const std::pair<Core::String, Core::String>& Expression);
			int SwitchCase(const Conditional& Value);
			size_t GetLinesCount(Core::String& Buffer, size_t End);
			ExpectsPreprocessor<void> ExpandDefinitions(Core::String& Buffer, size_t& Size);
			ExpectsPreprocessor<void> ParseArguments(const std::string_view& Value, Core::Vector<Core::String>& Tokens, bool UnpackLiterals);
			ExpectsPreprocessor<void> ConsumeTokens(const std::string_view& Path, Core::String& Buffer);
			void ApplyResult(bool WasNested);
			bool HasResult(const std::string_view& Path);
			bool SaveResult();

		public:
			static IncludeResult ResolveInclude(const IncludeDesc& Desc, bool AsGlobal);
		};

		template <typename T>
		class Math
		{
		public:
			template <typename F, bool = std::is_fundamental<F>::value>
			struct TypeTraits
			{
			};

			template <typename F>
			struct TypeTraits<F, true>
			{
				typedef F type;
			};

			template <typename F>
			struct TypeTraits<F, false>
			{
				typedef const F& type;
			};

		public:
			typedef typename TypeTraits<T>::type I;

		public:
			static T Rad2Deg()
			{
				return T(57.2957795130);
			}
			static T Deg2Rad()
			{
				return T(0.01745329251);
			}
			static T Pi()
			{
				return T(3.14159265359);
			}
			static T Sqrt(I Value)
			{
				return T(std::sqrt((double)Value));
			}
			static T Abs(I Value)
			{
				return Value < 0 ? -Value : Value;
			}
			static T Atan(I Angle)
			{
				return T(std::atan((double)Angle));
			}
			static T Atan2(I Angle0, I Angle1)
			{
				return T(std::atan2((double)Angle0, (double)Angle1));
			}
			static T Acos(I Angle)
			{
				return T(std::acos((double)Angle));
			}
			static T Asin(I Angle)
			{
				return T(std::asin((double)Angle));
			}
			static T Cos(I Angle)
			{
				return T(std::cos((double)Angle));
			}
			static T Sin(I Angle)
			{
				return T(std::sin((double)Angle));
			}
			static T Tan(I Angle)
			{
				return T(std::tan((double)Angle));
			}
			static T Acotan(I Angle)
			{
				return T(std::atan(1.0 / (double)Angle));
			}
			static T Max(I Value1, I Value2)
			{
				return Value1 > Value2 ? Value1 : Value2;
			}
			static T Min(I Value1, I Value2)
			{
				return Value1 < Value2 ? Value1 : Value2;
			}
			static T Log(I Value)
			{
				return T(std::log((double)Value));
			}
			static T Log2(I Value)
			{
				return T(std::log2((double)Value));
			}
			static T Log10(I Value)
			{
				return T(std::log10((double)Value));
			}
			static T Exp(I Value)
			{
				return T(std::exp((double)Value));
			}
			static T Ceil(I Value)
			{
				return T(std::ceil((double)Value));
			}
			static T Floor(I Value)
			{
				return T(std::floor((double)Value));
			}
			static T Lerp(I A, I B, I DeltaTime)
			{
				return A + DeltaTime * (B - A);
			}
			static T StrongLerp(I A, I B, I Time)
			{
				return (T(1.0) - Time) * A + Time * B;
			}
			static T SaturateAngle(I Angle)
			{
				return T(std::atan2(std::sin((double)Angle), std::cos((double)Angle)));
			}
			static T AngluarLerp(I A, I B, I DeltaTime)
			{
				if (A == B)
					return A;

				T AX = T(std::cos((double)A)), AY = T(std::sin((double)A));
				T BX = T(std::cos((double)B)), BY = T(std::sin((double)B));
				T CX = (BX - AX) * DeltaTime, CY = (BY - AY) * DeltaTime;
				return T(std::atan2((double)(AY + CY), (double)(AX + CX)));
			}
			static T AngleDistance(I A, I B)
			{
				T AX = T(std::cos((double)A)), AY = T(std::sin((double)A));
				T BX = T(std::cos((double)B)), BY = T(std::sin((double)B));
				T CX = AX - BX, CY = AY - BY;
				return T(std::sqrt((double)(CX * CX + CY * CY)));
			}
			static T Saturate(I Value)
			{
				return Min(Max(Value, T(0.0)), T(1.0));
			}
			static T Random(I Number0, I Number1)
			{
				if (Number0 == Number1)
					return Number0;

				return T((double)Number0 + ((double)Number1 - (double)Number0) / (double)std::numeric_limits<uint64_t>::max() * (double)Crypto::Random());
			}
			static T Round(I Value)
			{
				return T(std::round((double)Value));
			}
			static T Random()
			{
				return T((double)Crypto::Random() / (double)std::numeric_limits<uint64_t>::max());
			}
			static T RandomMag()
			{
				return T(2.0) * Random() - T(1.0);
			}
			static T Pow(I Value0, I Value1)
			{
				return T(std::pow((double)Value0, (double)Value1));
			}
			static T Pow2(I Value0)
			{
				return Value0 * Value0;
			}
			static T Pow3(I Value0)
			{
				return Value0 * Value0 * Value0;
			}
			static T Pow4(I Value0)
			{
				T Value = Value0 * Value0;
				return Value * Value;
			}
			static T Clamp(I Value, I pMin, I pMax)
			{
				return Min(Max(Value, pMin), pMax);
			}
			static T Select(I A, I B)
			{
				return Crypto::Random() < std::numeric_limits<uint64_t>::max() / 2 ? B : A;
			}
			static T Cotan(I Value)
			{
				return T(1.0 / std::tan((double)Value));
			}
			static T Map(I Value, I FromMin, I FromMax, I ToMin, I ToMax)
			{
				return ToMin + (Value - FromMin) * (ToMax - ToMin) / (FromMax - FromMin);
			}
			static void Swap(T& Value0, T& Value1)
			{
				T Value2 = Value0;
				Value0 = Value1;
				Value1 = Value2;
			}
		};

		typedef Math<Core::Decimal> Math0;
		typedef Math<UInt128> Math128u;
		typedef Math<UInt256> Math256u;
		typedef Math<int32_t> Math32;
		typedef Math<uint32_t> Math32u;
		typedef Math<int64_t> Math64;
		typedef Math<uint64_t> Math64u;
		typedef Math<float> Mathf;
		typedef Math<double> Mathd;
	}
}

using uint128_t = Vitex::Compute::UInt128;
using uint256_t = Vitex::Compute::UInt256;
#endif