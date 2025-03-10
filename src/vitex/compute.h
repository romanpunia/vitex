#ifndef VI_COMPUTE_H
#define VI_COMPUTE_H
#include "core.h"
#include <cmath>
#include <map>
#include <stack>
#include <limits>

namespace vitex
{
	namespace compute
	{
		class web_token;

		enum class regex_state
		{
			preprocessed = 0,
			match_found = -1,
			no_match = -2,
			unexpected_quantifier = -3,
			unbalanced_brackets = -4,
			internal_error = -5,
			invalid_character_set = -6,
			invalid_metacharacter = -7,
			sumatch_array_too_small = -8,
			too_many_branches = -9,
			too_many_brackets = -10,
		};

		enum class compression
		{
			none = 0,
			best_speed = 1,
			best_compression = 9,
			placeholder = -1
		};

		enum class include_type
		{
			error,
			preprocess,
			unchanged,
			computed
		};

		enum class preprocessor_error
		{
			macro_definition_empty,
			macro_name_empty,
			macro_parenthesis_double_closed,
			macro_parenthesis_not_closed,
			macro_definition_error,
			macro_expansion_parenthesis_double_closed,
			macro_expansion_parenthesis_not_closed,
			macro_expansion_arguments_error,
			macro_expansion_execution_error,
			macro_expansion_error,
			condition_not_opened,
			condition_not_closed,
			condition_error,
			directive_not_found,
			directive_expansion_error,
			include_denied,
			include_error,
			include_not_found,
			pragma_not_found,
			pragma_error,
			extension_error
		};

		typedef void* cipher;
		typedef void* digest;
		typedef int32_t sign_alg;

		struct include_desc
		{
			core::vector<core::string> exts;
			core::string from;
			core::string path;
			core::string root;
		};

		struct include_result
		{
			core::string library;
			bool is_abstract = false;
			bool is_remote = false;
			bool is_file = false;
		};

		struct regex_bracket
		{
			const char* pointer = nullptr;
			int64_t length = 0;
			int64_t branches = 0;
			int64_t branches_count = 0;
		};

		struct regex_branch
		{
			int64_t bracket_index;
			const char* pointer;
		};

		struct regex_match
		{
			const char* pointer;
			int64_t start;
			int64_t end;
			int64_t length;
			int64_t steps;
		};

		struct regex_source
		{
			friend class regex;

		private:
			core::string expression;
			core::vector<regex_bracket> brackets;
			core::vector<regex_branch> branches;
			int64_t max_branches;
			int64_t max_brackets;
			int64_t max_matches;
			regex_state state;

		public:
			bool ignore_case;

		public:
			regex_source() noexcept;
			regex_source(const std::string_view& regexp, bool fIgnoreCase = false, int64_t fMaxMatches = -1, int64_t fMaxBranches = -1, int64_t fMaxBrackets = -1) noexcept;
			regex_source(const regex_source& other) noexcept;
			regex_source(regex_source&& other) noexcept;
			regex_source& operator =(const regex_source& v) noexcept;
			regex_source& operator =(regex_source&& v) noexcept;
			const core::string& get_regex() const;
			int64_t get_max_branches() const;
			int64_t get_max_brackets() const;
			int64_t get_max_matches() const;
			int64_t get_complexity() const;
			regex_state get_state() const;
			bool is_simple() const;

		private:
			void compile();
		};

		struct regex_result
		{
			friend class regex;

		private:
			core::vector<regex_match> matches;
			regex_state state;
			int64_t steps;

		public:
			regex_source* src;

		public:
			regex_result() noexcept;
			regex_result(const regex_result& other) noexcept;
			regex_result(regex_result&& other) noexcept;
			regex_result& operator =(const regex_result& v) noexcept;
			regex_result& operator =(regex_result&& v) noexcept;
			bool empty() const;
			int64_t get_steps() const;
			regex_state get_state() const;
			const core::vector<regex_match>& get() const;
			core::vector<core::string> to_array() const;
		};

		struct secret_box
		{
		public:
			template <size_t max_size>
			struct exposable
			{
				uint8_t buffer[max_size];
				std::string_view view;

				~exposable() noexcept
				{
					secret_box::randomize_buffer((char*)buffer, view.size());
				}
			};

		private:
			union box_data
			{
				core::vector<void*> set;
				core::string sequence;
				std::string_view view;

				box_data()
				{
					memset((void*)this, 0, sizeof(*this));
				}
				~box_data()
				{
				}
			} data;
			enum class box_type : uint8_t
			{
				secure,
				insecure,
				view
			} type;

		private:
			secret_box(box_type new_type) noexcept;

		public:
			secret_box() noexcept;
			secret_box(const secret_box& other) noexcept;
			secret_box(secret_box&& other) noexcept;
			~secret_box() noexcept;
			secret_box& operator =(const secret_box& v) noexcept;
			secret_box& operator =(secret_box&& v) noexcept;
			void clear();
			void stack(char* buffer, size_t max_size, size_t* out_size = nullptr) const;
			core::string heap() const;
			size_t size() const;
			bool empty() const;

		public:
			template <size_t max_size>
			exposable<max_size> expose() const
			{
				size_t size = 0;
				exposable<max_size> result;
				stack((char*)result.buffer, max_size, &size);
				result.view = std::string_view((char*)result.buffer, size);
				return result;
			}

		public:
			static secret_box secure(const std::string_view& value);
			static secret_box insecure(core::string&& value);
			static secret_box insecure(const std::string_view& value);
			static secret_box view(const std::string_view& value);

		private:
			static void randomize_buffer(char* data, size_t size);
			static char load_partition(size_t* dest, size_t size, size_t index);
			static void roll_partition(size_t* dest, size_t size, size_t index);
			static void fill_partition(size_t* dest, size_t size, size_t index, char source);
			void copy_distribution(const secret_box& other);
			void move_distribution(secret_box&& other);
		};

		struct proc_directive
		{
			core::string name;
			core::string value;
			size_t start = 0;
			size_t end = 0;
			bool found = false;
			bool as_global = false;
			bool as_scope = false;
		};

		struct uint128
		{
		private:
#ifdef VI_ENDIAN_BIG
			uint64_t upper, lower;
#else
			uint64_t lower, upper;
#endif
		public:
			uint128() = default;
			uint128(const uint128& right) = default;
			uint128(uint128&& right) = default;
			uint128(const std::string_view& text);
			uint128(const std::string_view& text, uint8_t base);
			uint128& operator=(const uint128& right) = default;
			uint128& operator=(uint128&& right) = default;
			operator bool() const;
			operator uint8_t() const;
			operator uint16_t() const;
			operator uint32_t() const;
			operator uint64_t() const;
			uint128 operator&(const uint128& right) const;
			uint128& operator&=(const uint128& right);
			uint128 operator|(const uint128& right) const;
			uint128& operator|=(const uint128& right);
			uint128 operator^(const uint128& right) const;
			uint128& operator^=(const uint128& right);
			uint128 operator~() const;
			uint128 operator<<(const uint128& right) const;
			uint128& operator<<=(const uint128& right);
			uint128 operator>>(const uint128& right) const;
			uint128& operator>>=(const uint128& right);
			bool operator!() const;
			bool operator&&(const uint128& right) const;
			bool operator||(const uint128& right) const;
			bool operator==(const uint128& right) const;
			bool operator!=(const uint128& right) const;
			bool operator>(const uint128& right) const;
			bool operator<(const uint128& right) const;
			bool operator>=(const uint128& right) const;
			bool operator<=(const uint128& right) const;
			uint128 operator+(const uint128& right) const;
			uint128& operator+=(const uint128& right);
			uint128 operator-(const uint128& right) const;
			uint128& operator-=(const uint128& right);
			uint128 operator*(const uint128& right) const;
			uint128& operator*=(const uint128& right);
			uint128 operator/(const uint128& right) const;
			uint128& operator/=(const uint128& right);
			uint128 operator%(const uint128& right) const;
			uint128& operator%=(const uint128& right);
			uint128& operator++();
			uint128 operator++(int);
			uint128& operator--();
			uint128 operator--(int);
			uint128 operator+() const;
			uint128 operator-() const;
			const uint64_t& high() const;
			const uint64_t& low() const;
			uint8_t bits() const;
			uint8_t bytes() const;
			uint64_t& high();
			uint64_t& low();
			core::decimal to_decimal() const;
			core::string to_string(uint8_t base = 10, uint32_t length = 0) const;
			friend std::ostream& operator<<(std::ostream& stream, const uint128& right);
			friend uint128 operator<<(const uint8_t& left, const uint128& right);
			friend uint128 operator<<(const uint16_t& left, const uint128& right);
			friend uint128 operator<<(const uint32_t& left, const uint128& right);
			friend uint128 operator<<(const uint64_t& left, const uint128& right);
			friend uint128 operator<<(const int8_t& left, const uint128& right);
			friend uint128 operator<<(const int16_t& left, const uint128& right);
			friend uint128 operator<<(const int32_t& left, const uint128& right);
			friend uint128 operator<<(const int64_t& left, const uint128& right);
			friend uint128 operator>>(const uint8_t& left, const uint128& right);
			friend uint128 operator>>(const uint16_t& left, const uint128& right);
			friend uint128 operator>>(const uint32_t& left, const uint128& right);
			friend uint128 operator>>(const uint64_t& left, const uint128& right);
			friend uint128 operator>>(const int8_t& left, const uint128& right);
			friend uint128 operator>>(const int16_t& left, const uint128& right);
			friend uint128 operator>>(const int32_t& left, const uint128& right);
			friend uint128 operator>>(const int64_t& left, const uint128& right);

		public:
			static uint128 min();
			static uint128 max();

		private:
			std::pair<uint128, uint128> divide(const uint128& left, const uint128& right) const;

		public:
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128(const t& right)
#ifdef VI_ENDIAN_BIG
				: upper(0), lower(right)
#else
				: lower(right), upper(0)
#endif
			{
				if (std::is_signed<t>::value && right < 0)
					upper = -1;
			}
			template <typename s, typename t, typename = typename std::enable_if<std::is_integral<s>::value&& std::is_integral<t>::value, void>::type>
			uint128(const s& upper_right, const t& lower_right)
#ifdef VI_ENDIAN_BIG
				: upper(upper_right), lower(lower_right)
#else
				: lower(lower_right), upper(upper_right)
#endif
			{
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128& operator=(const t& right)
			{
				upper = 0;
				if (std::is_signed<t>::value && right < 0)
					upper = -1;

				lower = right;
				return *this;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128 operator&(const t& right) const
			{
				return uint128(0, lower & (uint64_t)right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128& operator&=(const t& right)
			{
				upper = 0;
				lower &= right;
				return *this;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128 operator|(const t& right) const
			{
				return uint128(upper, lower | (uint64_t)right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128& operator|=(const t& right)
			{
				lower |= (uint64_t)right;
				return *this;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128 operator^(const t& right) const
			{
				return uint128(upper, lower ^ (uint64_t)right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128& operator^=(const t& right)
			{
				lower ^= (uint64_t)right;
				return *this;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128 operator<<(const t& right) const
			{
				return *this << uint128(right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128& operator<<=(const t& right)
			{
				*this = *this << uint128(right);
				return *this;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128 operator>>(const t& right) const
			{
				return *this >> uint128(right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128& operator>>=(const t& right)
			{
				*this = *this >> uint128(right);
				return *this;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			bool operator&&(const t& right) const
			{
				return static_cast <bool> (*this && right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			bool operator||(const t& right) const
			{
				return static_cast <bool> (*this || right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			bool operator==(const t& right) const
			{
				return (!upper && (lower == (uint64_t)right));
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			bool operator!=(const t& right) const
			{
				return (upper || (lower != (uint64_t)right));
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			bool operator>(const t& right) const
			{
				return (upper || (lower > (uint64_t)right));
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			bool operator<(const t& right) const
			{
				return (!upper) ? (lower < (uint64_t)right) : false;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			bool operator>=(const t& right) const
			{
				return ((*this > right) || (*this == right));
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			bool operator<=(const t& right) const
			{
				return ((*this < right) || (*this == right));
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128 operator+(const t& right) const
			{
				return uint128(upper + ((lower + (uint64_t)right) < lower), lower + (uint64_t)right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128& operator+=(const t& right)
			{
				return *this += uint128(right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128 operator-(const t& right) const
			{
				return uint128((uint64_t)(upper - ((lower - right) > lower)), (uint64_t)(lower - right));
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128& operator-=(const t& right)
			{
				return *this = *this - uint128(right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128 operator*(const t& right) const
			{
				return *this * uint128(right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128& operator*=(const t& right)
			{
				return *this = *this * uint128(right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128 operator/(const t& right) const
			{
				return *this / uint128(right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128& operator/=(const t& right)
			{
				return *this = *this / uint128(right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128 operator%(const t& right) const
			{
				return *this % uint128(right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint128& operator%=(const t& right)
			{
				return *this = *this % uint128(right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend uint128 operator&(const t& left, const uint128& right)
			{
				return right & left;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend t& operator&=(t& left, const uint128& right)
			{
				return left = static_cast <t> (right & left);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend uint128 operator|(const t& left, const uint128& right)
			{
				return right | left;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend t& operator|=(t& left, const uint128& right)
			{
				return left = static_cast <t> (right | left);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend uint128 operator^(const t& left, const uint128& right)
			{
				return right ^ left;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend t& operator^=(t& left, const uint128& right)
			{
				return left = static_cast <t> (right ^ left);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend t& operator<<=(t& left, const uint128& right)
			{
				return left = static_cast <t> (uint128(left) << right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend t& operator>>=(t& left, const uint128& right)
			{
				return left = static_cast <t> (uint128(left) >> right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend bool operator==(const t& left, const uint128& right)
			{
				return (!right.high() && ((uint64_t)left == right.low()));
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend bool operator!=(const t& left, const uint128& right)
			{
				return (right.high() || ((uint64_t)left != right.low()));
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend bool operator>(const t& left, const uint128& right)
			{
				return (!right.high()) && ((uint64_t)left > right.low());
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend bool operator<(const t& left, const uint128& right)
			{
				if (right.high())
					return true;
				return ((uint64_t)left < right.low());
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend bool operator>=(const t& left, const uint128& right)
			{
				if (right.high())
					return false;
				return ((uint64_t)left >= right.low());
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend bool operator<=(const t& left, const uint128& right)
			{
				if (right.high())
					return true;
				return ((uint64_t)left <= right.low());
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend uint128 operator+(const t& left, const uint128& right)
			{
				return right + left;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend t& operator+=(t& left, const uint128& right)
			{
				return left = static_cast <t> (right + left);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend uint128 operator-(const t& left, const uint128& right)
			{
				return -(right - left);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend t& operator-=(t& left, const uint128& right)
			{
				return left = static_cast <t> (-(right - left));
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend uint128 operator*(const t& left, const uint128& right)
			{
				return right * left;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend t& operator*=(t& left, const uint128& right)
			{
				return left = static_cast <t> (right * left);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend uint128 operator/(const t& left, const uint128& right)
			{
				return uint128(left) / right;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend t& operator/=(t& left, const uint128& right)
			{
				return left = static_cast <t> (uint128(left) / right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend uint128 operator%(const t& left, const uint128& right)
			{
				return uint128(left) % right;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend t& operator%=(t& left, const uint128& right)
			{
				return left = static_cast <t> (uint128(left) % right);
			}
		};

		struct uint256
		{
		private:
#ifdef VI_ENDIAN_BIG
			uint128 upper, lower;
#else
			uint128 lower, upper;
#endif

		public:
			uint256() = default;
			uint256(const uint256& right) = default;
			uint256(uint256&& right) = default;
			uint256(const std::string_view& text);
			uint256(const std::string_view& text, uint8_t base);
			uint256(const uint128& upper_right, const uint128& lower_right)
#ifdef VI_ENDIAN_BIG
				: upper(upper_right), lower(lower_right)
#else
				: lower(lower_right), upper(upper_right)
#endif
			{
			}
			uint256(const uint128& lower_right)
#ifdef VI_ENDIAN_BIG
				: upper(uint128::min()), lower(lower_right)
#else
				: lower(lower_right), upper(uint128::min())
#endif
			{
			}
			uint256& operator=(const uint256& right) = default;
			uint256& operator=(uint256&& right) = default;
			operator bool() const;
			operator uint8_t() const;
			operator uint16_t() const;
			operator uint32_t() const;
			operator uint64_t() const;
			operator uint128() const;
			uint256 operator&(const uint128& right) const;
			uint256 operator&(const uint256& right) const;
			uint256& operator&=(const uint128& right);
			uint256& operator&=(const uint256& right);
			uint256 operator|(const uint128& right) const;
			uint256 operator|(const uint256& right) const;
			uint256& operator|=(const uint128& right);
			uint256& operator|=(const uint256& right);
			uint256 operator^(const uint128& right) const;
			uint256 operator^(const uint256& right) const;
			uint256& operator^=(const uint128& right);
			uint256& operator^=(const uint256& right);
			uint256 operator~() const;
			uint256 operator<<(const uint128& shift) const;
			uint256 operator<<(const uint256& shift) const;
			uint256& operator<<=(const uint128& shift);
			uint256& operator<<=(const uint256& shift);
			uint256 operator>>(const uint128& shift) const;
			uint256 operator>>(const uint256& shift) const;
			uint256& operator>>=(const uint128& shift);
			uint256& operator>>=(const uint256& shift);
			bool operator!() const;
			bool operator&&(const uint128& right) const;
			bool operator&&(const uint256& right) const;
			bool operator||(const uint128& right) const;
			bool operator||(const uint256& right) const;
			bool operator==(const uint128& right) const;
			bool operator==(const uint256& right) const;
			bool operator!=(const uint128& right) const;
			bool operator!=(const uint256& right) const;
			bool operator>(const uint128& right) const;
			bool operator>(const uint256& right) const;
			bool operator<(const uint128& right) const;
			bool operator<(const uint256& right) const;
			bool operator>=(const uint128& right) const;
			bool operator>=(const uint256& right) const;
			bool operator<=(const uint128& right) const;
			bool operator<=(const uint256& right) const;
			uint256 operator+(const uint128& right) const;
			uint256 operator+(const uint256& right) const;
			uint256& operator+=(const uint128& right);
			uint256& operator+=(const uint256& right);
			uint256 operator-(const uint128& right) const;
			uint256 operator-(const uint256& right) const;
			uint256& operator-=(const uint128& right);
			uint256& operator-=(const uint256& right);
			uint256 operator*(const uint128& right) const;
			uint256 operator*(const uint256& right) const;
			uint256& operator*=(const uint128& right);
			uint256& operator*=(const uint256& right);
			uint256 operator/(const uint128& right) const;
			uint256 operator/(const uint256& right) const;
			uint256& operator/=(const uint128& right);
			uint256& operator/=(const uint256& right);
			uint256 operator%(const uint128& right) const;
			uint256 operator%(const uint256& right) const;
			uint256& operator%=(const uint128& right);
			uint256& operator%=(const uint256& right);
			uint256& operator++();
			uint256 operator++(int);
			uint256& operator--();
			uint256 operator--(int);
			uint256 operator+() const;
			uint256 operator-() const;
			const uint128& high() const;
			const uint128& low() const;
			uint16_t bits() const;
			uint16_t bytes() const;
			uint128& high();
			uint128& low();
			core::decimal to_decimal() const;
			core::string to_string(uint8_t base = 10, uint32_t length = 0) const;
			friend uint256 operator&(const uint128& left, const uint256& right);
			friend uint128& operator&=(uint128& left, const uint256& right);
			friend uint256 operator|(const uint128& left, const uint256& right);
			friend uint128& operator|=(uint128& left, const uint256& right);
			friend uint256 operator^(const uint128& left, const uint256& right);
			friend uint128& operator^=(uint128& left, const uint256& right);
			friend uint256 operator<<(const uint8_t& left, const uint256& right);
			friend uint256 operator<<(const uint16_t& left, const uint256& right);
			friend uint256 operator<<(const uint32_t& left, const uint256& right);
			friend uint256 operator<<(const uint64_t& left, const uint256& right);
			friend uint256 operator<<(const uint128& left, const uint256& right);
			friend uint256 operator<<(const int8_t& left, const uint256& right);
			friend uint256 operator<<(const int16_t& left, const uint256& right);
			friend uint256 operator<<(const int32_t& left, const uint256& right);
			friend uint256 operator<<(const int64_t& left, const uint256& right);
			friend uint128& operator<<=(uint128& left, const uint256& right);
			friend uint256 operator>>(const uint8_t& left, const uint256& right);
			friend uint256 operator>>(const uint16_t& left, const uint256& right);
			friend uint256 operator>>(const uint32_t& left, const uint256& right);
			friend uint256 operator>>(const uint64_t& left, const uint256& right);
			friend uint256 operator>>(const uint128& left, const uint256& right);
			friend uint256 operator>>(const int8_t& left, const uint256& right);
			friend uint256 operator>>(const int16_t& left, const uint256& right);
			friend uint256 operator>>(const int32_t& left, const uint256& right);
			friend uint256 operator>>(const int64_t& left, const uint256& right);
			friend uint128& operator>>=(uint128& left, const uint256& right);
			friend bool operator==(const uint128& left, const uint256& right);
			friend bool operator!=(const uint128& left, const uint256& right);
			friend bool operator>(const uint128& left, const uint256& right);
			friend bool operator<(const uint128& left, const uint256& right);
			friend bool operator>=(const uint128& left, const uint256& right);
			friend bool operator<=(const uint128& left, const uint256& right);
			friend uint256 operator+(const uint128& left, const uint256& right);
			friend uint128& operator+=(uint128& left, const uint256& right);
			friend uint256 operator-(const uint128& left, const uint256& right);
			friend uint128& operator-=(uint128& left, const uint256& right);
			friend uint256 operator*(const uint128& left, const uint256& right);
			friend uint128& operator*=(uint128& left, const uint256& right);
			friend uint256 operator/(const uint128& left, const uint256& right);
			friend uint128& operator/=(uint128& left, const uint256& right);
			friend uint256 operator%(const uint128& left, const uint256& right);
			friend uint128& operator%=(uint128& left, const uint256& right);
			friend std::ostream& operator<<(std::ostream& stream, const uint256& right);

		public:
			static uint256 min();
			static uint256 max();

		private:
			std::pair<uint256, uint256> divide(const uint256& left, const uint256& right) const;

		public:
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type>
			uint256(const t& right)
#ifdef VI_ENDIAN_BIG
				: upper(uint128::min()), lower(right)
#else
				: lower(right), upper(uint128::min())
#endif
			{
				if (std::is_signed<t>::value && right < 0)
					upper = uint128(-1, -1);
			}
			template <typename s, typename t, typename = typename std::enable_if <std::is_integral<s>::value&& std::is_integral<t>::value, void>::type>
			uint256(const s& upper_right, const t& lower_right)
#ifdef VI_ENDIAN_BIG
				: upper(upper_right), lower(lower_right)
#else
				: lower(lower_right), upper(upper_right)
#endif
			{
			}
			template <typename r, typename s, typename t, typename u, typename = typename std::enable_if<std::is_integral<r>::value&& std::is_integral<s>::value&& std::is_integral<t>::value&& std::is_integral<u>::value, void>::type>
			uint256(const r& upper_lhs, const s& lower_lhs, const t& upper_right, const u& lower_right)
#ifdef VI_ENDIAN_BIG
				: upper(upper_lhs, lower_lhs), lower(upper_right, lower_right)
#else
				: lower(upper_right, lower_right), upper(upper_lhs, lower_lhs)
#endif
			{
			}
			template <typename t, typename = typename std::enable_if <std::is_integral<t>::value, t>::type>
			uint256& operator=(const t& right)
			{
				upper = uint128::min();
				if (std::is_signed<t>::value && right < 0)
					upper = uint128(-1, -1);

				lower = right;
				return *this;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint256 operator&(const t& right) const
			{
				return uint256(uint128::min(), lower & (uint128)right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint256& operator&=(const t& right)
			{
				upper = uint128::min();
				lower &= right;
				return *this;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint256 operator|(const t& right) const
			{
				return uint256(upper, lower | uint128(right));
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint256& operator|=(const t& right)
			{
				lower |= (uint128)right;
				return *this;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint256 operator^(const t& right) const
			{
				return uint256(upper, lower ^ (uint128)right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint256& operator^=(const t& right)
			{
				lower ^= (uint128)right;
				return *this;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint256 operator<<(const t& right) const
			{
				return *this << uint256(right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint256& operator<<=(const t& right)
			{
				*this = *this << uint256(right);
				return *this;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint256 operator>>(const t& right) const
			{
				return *this >> uint256(right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint256& operator>>=(const t& right)
			{
				*this = *this >> uint256(right);
				return *this;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			bool operator&&(const t& right) const
			{
				return ((bool)*this && right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			bool operator||(const t& right) const
			{
				return ((bool)*this || right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			bool operator==(const t& right) const
			{
				return (!upper && (lower == uint128(right)));
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			bool operator!=(const t& right) const
			{
				return ((bool)upper || (lower != uint128(right)));
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			bool operator>(const t& right) const
			{
				return ((bool)upper || (lower > uint128(right)));
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			bool operator<(const t& right) const
			{
				return (!upper) ? (lower < uint128(right)) : false;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			bool operator>=(const t& right) const
			{
				return ((*this > right) || (*this == right));
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			bool operator<=(const t& right) const
			{
				return ((*this < right) || (*this == right));
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint256 operator+(const t& right) const
			{
				return uint256(upper + ((lower + (uint128)right) < lower), lower + (uint128)right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint256& operator+=(const t& right)
			{
				return *this += uint256(right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint256 operator-(const t& right) const
			{
				return uint256(upper - ((lower - right) > lower), lower - right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint256& operator-=(const t& right)
			{
				return *this = *this - uint256(right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint256 operator*(const t& right) const
			{
				return *this * uint256(right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint256& operator*=(const t& right)
			{
				return *this = *this * uint256(right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint256 operator/(const t& right) const
			{
				return *this / uint256(right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint256& operator/=(const t& right)
			{
				return *this = *this / uint256(right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint256 operator%(const t& right) const
			{
				return *this % uint256(right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			uint256& operator%=(const t& right)
			{
				return *this = *this % uint256(right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend uint256 operator&(const t& left, const uint256& right)
			{
				return right & left;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend t& operator&=(t& left, const uint256& right)
			{
				return left = static_cast <t> (right & left);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend uint256 operator|(const t& left, const uint256& right)
			{
				return right | left;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend t& operator|=(t& left, const uint256& right)
			{
				return left = static_cast <t> (right | left);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend uint256 operator^(const t& left, const uint256& right)
			{
				return right ^ left;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend t& operator^=(t& left, const uint256& right)
			{
				return left = static_cast <t> (right ^ left);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend t& operator<<=(t& left, const uint256& right)
			{
				left = static_cast <t> (uint256(left) << right);
				return left;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend t& operator>>=(t& left, const uint256& right)
			{
				return left = static_cast <t> (uint256(left) >> right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend bool operator==(const t& left, const uint256& right)
			{
				return (!right.high() && ((uint64_t)left == right.low()));
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend bool operator!=(const t& left, const uint256& right)
			{
				return (right.high() || ((uint64_t)left != right.low()));
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend bool operator>(const t& left, const uint256& right)
			{
				return right.high() ? false : ((uint128)left > right.low());
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend bool operator<(const t& left, const uint256& right)
			{
				return right.high() ? true : ((uint128)left < right.low());
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend bool operator>=(const t& left, const uint256& right)
			{
				return right.high() ? false : ((uint128)left >= right.low());
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend bool operator<=(const t& left, const uint256& right)
			{
				return right.high() ? true : ((uint128)left <= right.low());
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend uint256 operator+(const t& left, const uint256& right)
			{
				return right + left;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend t& operator+=(t& left, const uint256& right)
			{
				left = static_cast <t> (right + left);
				return left;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend uint256 operator-(const t& left, const uint256& right)
			{
				return -(right - left);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend t& operator-=(t& left, const uint256& right)
			{
				return left = static_cast <t> (-(right - left));
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend uint256 operator*(const t& left, const uint256& right)
			{
				return right * left;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend t& operator*=(t& left, const uint256& right)
			{
				return left = static_cast <t> (right * left);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend uint256 operator/(const t& left, const uint256& right)
			{
				return uint256(left) / right;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend t& operator/=(t& left, const uint256& right)
			{
				return left = static_cast <t> (uint256(left) / right);
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend uint256 operator%(const t& left, const uint256& right)
			{
				return uint256(left) % right;
			}
			template <typename t, typename = typename std::enable_if<std::is_integral<t>::value, t>::type >
			friend t& operator%=(t& left, const uint256& right)
			{
				return left = static_cast <t> (uint256(left) % right);
			}
		};

		class preprocessor_exception final : public core::basic_exception
		{
		private:
			preprocessor_error error_type;
			size_t error_offset;

		public:
			preprocessor_exception(preprocessor_error new_type);
			preprocessor_exception(preprocessor_error new_type, size_t new_offset);
			preprocessor_exception(preprocessor_error new_type, size_t new_offset, const std::string_view& message);
			const char* type() const noexcept override;
			preprocessor_error status() const noexcept;
			size_t offset() const noexcept;
		};

		class crypto_exception final : public core::basic_exception
		{
		private:
			size_t error_code;

		public:
			crypto_exception();
			crypto_exception(size_t error_code, const std::string_view& message);
			const char* type() const noexcept override;
			size_t code() const noexcept;
		};

		class compression_exception final : public core::basic_exception
		{
		private:
			int error_code;

		public:
			compression_exception(int error_code, const std::string_view& message);
			const char* type() const noexcept override;
			int code() const noexcept;
		};

		template <typename v>
		using expects_preprocessor = core::expects<v, preprocessor_exception>;

		template <typename v>
		using expects_crypto = core::expects<v, crypto_exception>;

		template <typename v>
		using expects_compression = core::expects<v, compression_exception>;

		typedef std::function<expects_preprocessor<include_type>(class preprocessor*, const struct include_result& file, core::string& output)> proc_include_callback;
		typedef std::function<expects_preprocessor<void>(class preprocessor*, const std::string_view& name, const core::vector<core::string>& args)> proc_pragma_callback;
		typedef std::function<expects_preprocessor<void>(class preprocessor*, const struct proc_directive&, core::string& output)> proc_directive_callback;
		typedef std::function<expects_preprocessor<core::string>(class preprocessor*, const core::vector<core::string>& args)> proc_expansion_callback;

		class md5_hasher
		{
		private:
			typedef uint8_t uint1;
			typedef uint32_t uint4;

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
			bool finalized;
			uint1 buffer[64];
			uint4 count[2];
			uint4 state[4];
			uint1 digest[16];

		public:
			md5_hasher() noexcept;
			void transform(const uint1* buffer, uint32_t length = 64);
			void update(const std::string_view& buffer, uint32_t block_size = 64);
			void update(const uint8_t* buffer, uint32_t length, uint32_t block_size = 64);
			void update(const char* buffer, uint32_t length, uint32_t block_size = 64);
			void finalize();
			core::unique<char> hex_digest() const;
			core::unique<uint8_t> raw_digest() const;
			core::string to_hex() const;
			core::string to_raw() const;

		private:
			static void decode(uint4* output, const uint1* input, uint32_t length);
			static void encode(uint1* output, const uint4* input, uint32_t length);
			static void FF(uint4& a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 AC);
			static void GG(uint4& a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 AC);
			static void HH(uint4& a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 AC);
			static void II(uint4& a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 AC);
			static uint4 f(uint4 x, uint4 y, uint4 z);
			static uint4 g(uint4 x, uint4 y, uint4 z);
			static uint4 h(uint4 x, uint4 y, uint4 z);
			static uint4 i(uint4 x, uint4 y, uint4 z);
			static uint4 l(uint4 x, int n);
		};

		class s8_hasher
		{
		public:
			s8_hasher() noexcept = default;
			s8_hasher(const s8_hasher&) noexcept = default;
			s8_hasher(s8_hasher&&) noexcept = default;
			~s8_hasher() noexcept = default;
			s8_hasher& operator=(const s8_hasher&) = default;
			s8_hasher& operator=(s8_hasher&&) noexcept = default;
			inline size_t operator()(uint64_t value) const noexcept
			{
				value ^= (size_t)(value >> 33);
				value *= 0xFF51AFD7ED558CCD;
				value ^= (size_t)(value >> 33);
				value *= 0xC4CEB9FE1A85EC53;
				value ^= (size_t)(value >> 33);
				return (size_t)value;
			}
			static inline uint64_t parse(const char data[8]) noexcept
			{
				uint64_t result = 0;
				result |= ((uint64_t)data[0]);
				result |= ((uint64_t)data[1]) << 8;
				result |= ((uint64_t)data[2]) << 16;
				result |= ((uint64_t)data[3]) << 24;
				result |= ((uint64_t)data[4]) << 32;
				result |= ((uint64_t)data[5]) << 40;
				result |= ((uint64_t)data[6]) << 48;
				result |= ((uint64_t)data[7]) << 56;
				return result;
			}
		};

		class ciphers
		{
		public:
			static cipher DES_ECB();
			static cipher DES_EDE();
			static cipher DES_EDE3();
			static cipher DES_EDE_ECB();
			static cipher DES_EDE3_ECB();
			static cipher DES_CFB64();
			static cipher DES_CFB1();
			static cipher DES_CFB8();
			static cipher DES_EDE_CFB64();
			static cipher DES_EDE3_CFB64();
			static cipher DES_EDE3_CFB1();
			static cipher DES_EDE3_CFB8();
			static cipher DES_OFB();
			static cipher DES_EDE_OFB();
			static cipher DES_EDE3_OFB();
			static cipher DES_CBC();
			static cipher DES_EDE_CBC();
			static cipher DES_EDE3_CBC();
			static cipher desede3_wrap();
			static cipher DESX_CBC();
			static cipher RC4();
			static cipher RC4_40();
			static cipher RC4_HMAC_MD5();
			static cipher IDEA_ECB();
			static cipher IDEA_CFB64();
			static cipher IDEA_OFB();
			static cipher IDEA_CBC();
			static cipher RC2_ECB();
			static cipher RC2_CBC();
			static cipher RC2_40_CBC();
			static cipher RC2_64_CBC();
			static cipher RC2_CFB64();
			static cipher RC2_OFB();
			static cipher BF_ECB();
			static cipher BF_CBC();
			static cipher BF_CFB64();
			static cipher BF_OFB();
			static cipher CAST5_ECB();
			static cipher CAST5_CBC();
			static cipher CAST5_CFB64();
			static cipher CAST5_OFB();
			static cipher RC5_32_12_16_CBC();
			static cipher RC5_32_12_16_ECB();
			static cipher RC5_32_12_16_CFB64();
			static cipher RC5_32_12_16_OFB();
			static cipher AES_128_ECB();
			static cipher AES_128_CBC();
			static cipher AES_128_CFB1();
			static cipher AES_128_CFB8();
			static cipher AES_128_CFB128();
			static cipher AES_128_OFB();
			static cipher AES_128_CTR();
			static cipher AES_128_CCM();
			static cipher AES_128_GCM();
			static cipher AES_128_XTS();
			static cipher aes128_wrap();
			static cipher aes128_wrap_pad();
			static cipher AES_128_OCB();
			static cipher AES_192_ECB();
			static cipher AES_192_CBC();
			static cipher AES_192_CFB1();
			static cipher AES_192_CFB8();
			static cipher AES_192_CFB128();
			static cipher AES_192_OFB();
			static cipher AES_192_CTR();
			static cipher AES_192_CCM();
			static cipher AES_192_GCM();
			static cipher aes192_wrap();
			static cipher aes192_wrap_pad();
			static cipher AES_192_OCB();
			static cipher AES_256_ECB();
			static cipher AES_256_CBC();
			static cipher AES_256_CFB1();
			static cipher AES_256_CFB8();
			static cipher AES_256_CFB128();
			static cipher AES_256_OFB();
			static cipher AES_256_CTR();
			static cipher AES_256_CCM();
			static cipher AES_256_GCM();
			static cipher AES_256_XTS();
			static cipher aes256_wrap();
			static cipher aes256_wrap_pad();
			static cipher AES_256_OCB();
			static cipher AES_128_CBC_HMAC_SHA1();
			static cipher AES_256_CBC_HMAC_SHA1();
			static cipher AES_128_CBC_HMAC_SHA256();
			static cipher AES_256_CBC_HMAC_SHA256();
			static cipher ARIA_128_ECB();
			static cipher ARIA_128_CBC();
			static cipher ARIA_128_CFB1();
			static cipher ARIA_128_CFB8();
			static cipher ARIA_128_CFB128();
			static cipher ARIA_128_CTR();
			static cipher ARIA_128_OFB();
			static cipher ARIA_128_GCM();
			static cipher ARIA_128_CCM();
			static cipher ARIA_192_ECB();
			static cipher ARIA_192_CBC();
			static cipher ARIA_192_CFB1();
			static cipher ARIA_192_CFB8();
			static cipher ARIA_192_CFB128();
			static cipher ARIA_192_CTR();
			static cipher ARIA_192_OFB();
			static cipher ARIA_192_GCM();
			static cipher ARIA_192_CCM();
			static cipher ARIA_256_ECB();
			static cipher ARIA_256_CBC();
			static cipher ARIA_256_CFB1();
			static cipher ARIA_256_CFB8();
			static cipher ARIA_256_CFB128();
			static cipher ARIA_256_CTR();
			static cipher ARIA_256_OFB();
			static cipher ARIA_256_GCM();
			static cipher ARIA_256_CCM();
			static cipher camellia128_ecb();
			static cipher camellia128_cbc();
			static cipher camellia128_cfb1();
			static cipher camellia128_cfb8();
			static cipher camellia128_cfb128();
			static cipher camellia128_ofb();
			static cipher camellia128_ctr();
			static cipher camellia192_ecb();
			static cipher camellia192_cbc();
			static cipher camellia192_cfb1();
			static cipher camellia192_cfb8();
			static cipher camellia192_cfb128();
			static cipher camellia192_ofb();
			static cipher camellia192_ctr();
			static cipher camellia256_ecb();
			static cipher camellia256_cbc();
			static cipher camellia256_cfb1();
			static cipher camellia256_cfb8();
			static cipher camellia256_cfb128();
			static cipher camellia256_ofb();
			static cipher camellia256_ctr();
			static cipher chacha20();
			static cipher chacha20_poly1305();
			static cipher seed_ecb();
			static cipher seed_cbc();
			static cipher seed_cfb128();
			static cipher seed_ofb();
			static cipher SM4_ECB();
			static cipher SM4_CBC();
			static cipher SM4_CFB128();
			static cipher SM4_OFB();
			static cipher SM4_CTR();
		};

		class digests
		{
		public:
			static digest MD2();
			static digest MD4();
			static digest MD5();
			static digest MD5_SHA1();
			static digest blake2_b512();
			static digest blake2_s256();
			static digest SHA1();
			static digest SHA224();
			static digest SHA256();
			static digest SHA384();
			static digest SHA512();
			static digest SHA512_224();
			static digest SHA512_256();
			static digest SHA3_224();
			static digest SHA3_256();
			static digest SHA3_384();
			static digest SHA3_512();
			static digest shake128();
			static digest shake256();
			static digest MDC2();
			static digest ripe_md160();
			static digest whirlpool();
			static digest SM3();
		};

		class signers
		{
		public:
			static sign_alg pk_rsa();
			static sign_alg pk_dsa();
			static sign_alg pk_dh();
			static sign_alg pk_ec();
			static sign_alg pkt_sign();
			static sign_alg pkt_enc();
			static sign_alg pkt_exch();
			static sign_alg pks_rsa();
			static sign_alg pks_dsa();
			static sign_alg pks_ec();
			static sign_alg RSA();
			static sign_alg RSA2();
			static sign_alg RSA_PSS();
			static sign_alg DSA();
			static sign_alg DSA1();
			static sign_alg DSA2();
			static sign_alg DSA3();
			static sign_alg DSA4();
			static sign_alg DH();
			static sign_alg DHX();
			static sign_alg EC();
			static sign_alg SM2();
			static sign_alg HMAC();
			static sign_alg CMAC();
			static sign_alg SCRYPT();
			static sign_alg TLS1_PRF();
			static sign_alg HKDF();
			static sign_alg POLY1305();
			static sign_alg SIPHASH();
			static sign_alg X25519();
			static sign_alg ED25519();
			static sign_alg X448();
			static sign_alg ED448();
		};

		class crypto
		{
		public:
			typedef std::function<void(uint8_t**, size_t*)> block_callback;

		public:
			static digest get_digest_by_name(const std::string_view& name);
			static cipher get_cipher_by_name(const std::string_view& name);
			static sign_alg get_signer_by_name(const std::string_view& name);
			static std::string_view get_digest_name(digest type);
			static std::string_view get_cipher_name(cipher type);
			static std::string_view get_signer_name(sign_alg type);
			static expects_crypto<void> fill_random_bytes(uint8_t* buffer, size_t length);
			static expects_crypto<core::string> random_bytes(size_t length);
			static expects_crypto<core::string> generate_private_key(sign_alg type, size_t target_bits = 2048, const std::string_view& curve = std::string_view());
			static expects_crypto<core::string> generate_public_key(sign_alg type, const secret_box& secret_key);
			static expects_crypto<core::string> checksum_hex(digest type, core::stream* stream);
			static expects_crypto<core::string> checksum_raw(digest type, core::stream* stream);
			static expects_crypto<core::string> hash_hex(digest type, const std::string_view& value);
			static expects_crypto<core::string> hash_raw(digest type, const std::string_view& value);
			static expects_crypto<core::string> sign(digest type, sign_alg key_type, const std::string_view& value, const secret_box& secret_key);
			static expects_crypto<void> verify(digest type, sign_alg key_type, const std::string_view& value, const std::string_view& signature, const secret_box& public_key);
			static expects_crypto<core::string> HMAC(digest type, const std::string_view& value, const secret_box& key);
			static expects_crypto<core::string> encrypt(cipher type, const std::string_view& value, const secret_box& key, const secret_box& salt, int complexity_bytes = -1);
			static expects_crypto<core::string> decrypt(cipher type, const std::string_view& value, const secret_box& key, const secret_box& salt, int complexity_bytes = -1);
			static expects_crypto<core::string> jwt_sign(const std::string_view& algo, const std::string_view& payload, const secret_box& key);
			static expects_crypto<core::string> jwt_encode(web_token* src, const secret_box& key);
			static expects_crypto<core::unique<web_token>> jwt_decode(const std::string_view& value, const secret_box& key);
			static expects_crypto<core::string> doc_encrypt(core::schema* src, const secret_box& key, const secret_box& salt);
			static expects_crypto<core::unique<core::schema>> doc_decrypt(const std::string_view& value, const secret_box& key, const secret_box& salt);
			static expects_crypto<size_t> encrypt(cipher type, core::stream* from, core::stream* to, const secret_box& key, const secret_box& salt, block_callback&& callback = nullptr, size_t read_interval = 1, int complexity_bytes = -1);
			static expects_crypto<size_t> decrypt(cipher type, core::stream* from, core::stream* to, const secret_box& key, const secret_box& salt, block_callback&& callback = nullptr, size_t read_interval = 1, int complexity_bytes = -1);
			static uint8_t random_uc();
			static uint64_t CRC32(const std::string_view& data);
			static uint64_t random(uint64_t min, uint64_t max);
			static uint64_t random();
			static void sha1_collapse_buffer_block(uint32_t* buffer);
			static void sha1_compute_hash_block(uint32_t* result, uint32_t* w);
			static void sha1_compute(const void* value, int length, char* hash20);
			static void sha1_hash20_to_hex(const char* hash20, char* hex_string);
			static void display_crypto_log();
		};

		class codec
		{
		public:
			static void rotate_buffer(uint8_t* buffer, size_t buffer_size, uint64_t hash, int8_t direction);
			static core::string rotate(const std::string_view& value, uint64_t hash, int8_t direction);
			static core::string encode64(const char alphabet[65], const uint8_t* value, size_t length, bool padding);
			static core::string decode64(const char alphabet[65], const uint8_t* value, size_t length, bool(*is_alphabetic)(uint8_t));
			static core::string bep45_encode(const std::string_view& value);
			static core::string bep45_decode(const std::string_view& value);
			static core::string base32_encode(const std::string_view& value);
			static core::string base32_decode(const std::string_view& value);
			static core::string base45_encode(const std::string_view& value);
			static core::string base45_decode(const std::string_view& value);
			static core::string base64_encode(const std::string_view& value);
			static core::string base64_decode(const std::string_view& value);
			static core::string base64_url_encode(const std::string_view& value);
			static core::string base64_url_decode(const std::string_view& value);
			static core::string shuffle(const char* value, size_t size, uint64_t mask);
			static expects_compression<core::string> compress(const std::string_view& data, compression type = compression::placeholder);
			static expects_compression<core::string> decompress(const std::string_view& data);
			static core::string hex_encode_odd(const std::string_view& value, bool upper_case = false);
			static core::string hex_encode(const std::string_view& value, bool upper_case = false);
			static core::string hex_decode(const std::string_view& value);
			static core::string url_encode(const std::string_view& text);
			static core::string url_decode(const std::string_view& text);
			static core::string decimal_to_hex(uint64_t v);
			static core::string base10_to_base_n(uint64_t value, uint32_t base_less_than65);
			static size_t utf8(int code, char* buffer);
			static bool hex(char code, int& value);
			static bool hex_to_string(const std::string_view& data, char* buffer, size_t buffer_length);
			static bool hex_to_decimal(const std::string_view& text, size_t index, size_t size, int& value);
			static bool is_base64_url(uint8_t value);
			static bool is_base64(uint8_t value);
		};

		class regex
		{
			friend regex_source;

		private:
			static int64_t meta(const uint8_t* buffer);
			static int64_t op_length(const char* value);
			static int64_t set_length(const char* value, int64_t value_length);
			static int64_t get_op_length(const char* value, int64_t value_length);
			static int64_t quantifier(const char* value);
			static int64_t to_int(int64_t x);
			static int64_t hex_to_int(const uint8_t* buffer);
			static int64_t match_op(const uint8_t* value, const uint8_t* buffer, regex_result* info);
			static int64_t match_set(const char* value, int64_t value_length, const char* buffer, regex_result* info);
			static int64_t parse_doh(const char* buffer, int64_t buffer_length, regex_result* info, int64_t condition);
			static int64_t parse_inner(const char* value, int64_t value_length, const char* buffer, int64_t buffer_length, regex_result* info, int64_t condition);
			static int64_t parse(const char* buffer, int64_t buffer_length, regex_result* info);

		public:
			static bool match(regex_source* value, regex_result& result, const std::string_view& buffer);
			static bool replace(regex_source* value, const std::string_view& to_expression, core::string& buffer);
			static const char* syntax();
		};

		class web_token final : public core::reference<web_token>
		{
		public:
			core::schema* header;
			core::schema* payload;
			core::schema* token;
			core::string refresher;
			core::string signature;
			core::string data;

		public:
			web_token() noexcept;
			web_token(const std::string_view& issuer, const std::string_view& subject, int64_t expiration) noexcept;
			virtual ~web_token() noexcept;
			void unsign();
			void set_algorithm(const std::string_view& value);
			void set_type(const std::string_view& value);
			void set_content_type(const std::string_view& value);
			void set_issuer(const std::string_view& value);
			void set_subject(const std::string_view& value);
			void set_id(const std::string_view& value);
			void set_audience(const core::vector<core::string>& value);
			void set_expiration(int64_t value);
			void set_not_before(int64_t value);
			void set_created(int64_t value);
			void set_refresh_token(const std::string_view& value, const secret_box& key, const secret_box& salt);
			bool sign(const secret_box& key);
			expects_crypto<core::string> get_refresh_token(const secret_box& key, const secret_box& salt);
			bool is_valid() const;
		};

		class preprocessor final : public core::reference<preprocessor>
		{
		public:
			struct desc
			{
				core::string multiline_comment_begin = "/*";
				core::string multiline_comment_end = "*/";
				core::string comment_begin = "//";
				core::string string_literals = "\"'`";
				bool pragmas = true;
				bool includes = true;
				bool defines = true;
				bool conditions = true;
			};

		private:
			enum class controller
			{
				start_if = 0,
				else_if = 1,
				else_case = 2,
				end_if = 3
			};

			enum class condition
			{
				exists = 1,
				equals = 2,
				greater = 3,
				greater_equals = 4,
				less = 5,
				less_equals = 6,
				text = 0,
				not_exists = -1,
				not_equals = -2,
				not_greater = -3,
				not_greater_equals = -4,
				not_less = -5,
				not_less_equals = -6,
			};

			struct conditional
			{
				core::vector<conditional> childs;
				core::string expression;
				bool chaining = false;
				condition type = condition::text;
				size_t token_start = 0;
				size_t token_end = 0;
				size_t text_start = 0;
				size_t text_end = 0;
			};

			struct definition
			{
				core::vector<core::string> tokens;
				core::string expansion;
				proc_expansion_callback callback;
			};

		private:
			struct file_context
			{
				core::string path;
				size_t line = 0;
			} this_file;

		private:
			core::unordered_map<core::string, std::pair<condition, controller>> control_flow;
			core::unordered_map<core::string, proc_directive_callback> directives;
			core::unordered_map<core::string, definition> defines;
			core::unordered_set<core::string> sets;
			std::function<size_t()> store_current_line;
			proc_include_callback include;
			proc_pragma_callback pragma;
			include_desc file_desc;
			desc features;
			bool nested;

		public:
			preprocessor() noexcept;
			~preprocessor() noexcept = default;
			void set_include_options(const include_desc& new_desc);
			void set_include_callback(proc_include_callback&& callback);
			void set_pragma_callback(proc_pragma_callback&& callback);
			void set_directive_callback(const std::string_view& name, proc_directive_callback&& callback);
			void set_features(const desc& value);
			void add_default_definitions();
			expects_preprocessor<void> define(const std::string_view& expression);
			expects_preprocessor<void> define_dynamic(const std::string_view& expression, proc_expansion_callback&& callback);
			void undefine(const std::string_view& name);
			void clear();
			bool is_defined(const std::string_view& name) const;
			bool is_defined(const std::string_view& name, const std::string_view& value) const;
			expects_preprocessor<void> process(const std::string_view& path, core::string& buffer);
			expects_preprocessor<core::string> resolve_file(const std::string_view& path, const std::string_view& include);
			const core::string& get_current_file_path() const;
			size_t get_current_line_number();

		private:
			proc_directive find_next_token(core::string& buffer, size_t& offset);
			proc_directive find_next_conditional_token(core::string& buffer, size_t& offset);
			size_t replace_token(proc_directive& where, core::string& buffer, const std::string_view& to);
			expects_preprocessor<core::vector<conditional>> prepare_conditions(core::string& buffer, proc_directive& next, size_t& offset, bool top);
			core::string evaluate(core::string& buffer, const core::vector<conditional>& conditions);
			std::pair<core::string, core::string> get_expression_parts(const std::string_view& value);
			std::pair<core::string, core::string> unpack_expression(const std::pair<core::string, core::string>& expression);
			int switch_case(const conditional& value);
			size_t get_lines_count(core::string& buffer, size_t end);
			expects_preprocessor<void> expand_definitions(core::string& buffer, size_t& size);
			expects_preprocessor<void> parse_arguments(const std::string_view& value, core::vector<core::string>& tokens, bool unpack_literals);
			expects_preprocessor<void> consume_tokens(const std::string_view& path, core::string& buffer);
			void apply_result(bool was_nested);
			bool has_result(const std::string_view& path);
			bool save_result();

		public:
			static include_result resolve_include(const include_desc& desc, bool as_global);
		};

		template <typename t>
		class math
		{
		public:
			template <typename f, bool = std::is_fundamental<f>::value>
			struct type_traits
			{
			};

			template <typename f>
			struct type_traits<f, true>
			{
				typedef f type;
			};

			template <typename f>
			struct type_traits<f, false>
			{
				typedef const f& type;
			};

		public:
			typedef typename type_traits<t>::type i;

		public:
			static t rad2deg()
			{
				return t(57.2957795130);
			}
			static t deg2rad()
			{
				return t(0.01745329251);
			}
			static t pi()
			{
				return t(3.14159265359);
			}
			static t sqrt(i value)
			{
				return t(std::sqrt((double)value));
			}
			static t abs(i value)
			{
				return value < 0 ? -value : value;
			}
			static t atan(i angle)
			{
				return t(std::atan((double)angle));
			}
			static t atan2(i angle0, i angle1)
			{
				return t(std::atan2((double)angle0, (double)angle1));
			}
			static t acos(i angle)
			{
				return t(std::acos((double)angle));
			}
			static t asin(i angle)
			{
				return t(std::asin((double)angle));
			}
			static t cos(i angle)
			{
				return t(std::cos((double)angle));
			}
			static t sin(i angle)
			{
				return t(std::sin((double)angle));
			}
			static t tan(i angle)
			{
				return t(std::tan((double)angle));
			}
			static t acotan(i angle)
			{
				return t(std::atan(1.0 / (double)angle));
			}
			static t max(i value1, i value2)
			{
				return value1 > value2 ? value1 : value2;
			}
			static t min(i value1, i value2)
			{
				return value1 < value2 ? value1 : value2;
			}
			static t log(i value)
			{
				return t(std::log((double)value));
			}
			static t log2(i value)
			{
				return t(std::log2((double)value));
			}
			static t log10(i value)
			{
				return t(std::log10((double)value));
			}
			static t exp(i value)
			{
				return t(std::exp((double)value));
			}
			static t ceil(i value)
			{
				return t(std::ceil((double)value));
			}
			static t floor(i value)
			{
				return t(std::floor((double)value));
			}
			static t lerp(i a, i b, i delta_time)
			{
				return a + delta_time * (b - a);
			}
			static t strong_lerp(i a, i b, i time)
			{
				return (t(1.0) - time) * a + time * b;
			}
			static t saturate_angle(i angle)
			{
				return t(std::atan2(std::sin((double)angle), std::cos((double)angle)));
			}
			static t saturate(i value)
			{
				return min(max(value, t(0.0)), t(1.0));
			}
			static t random(i number0, i number1)
			{
				if (number0 == number1)
					return number0;

				return t((double)number0 + ((double)number1 - (double)number0) / (double)std::numeric_limits<uint64_t>::max() * (double)crypto::random());
			}
			static t round(i value)
			{
				return t(std::round((double)value));
			}
			static t random()
			{
				return t((double)crypto::random() / (double)std::numeric_limits<uint64_t>::max());
			}
			static t random_mag()
			{
				return t(2.0) * random() - t(1.0);
			}
			static t pow(i value0, i value1)
			{
				return t(std::pow((double)value0, (double)value1));
			}
			static t pow2(i value0)
			{
				return value0 * value0;
			}
			static t pow3(i value0)
			{
				return value0 * value0 * value0;
			}
			static t pow4(i value0)
			{
				t value = value0 * value0;
				return value * value;
			}
			static t clamp(i value, i pMin, i pMax)
			{
				return min(max(value, pMin), pMax);
			}
			static t select(i a, i b)
			{
				return crypto::random() < std::numeric_limits<uint64_t>::max() / 2 ? b : a;
			}
			static t cotan(i value)
			{
				return t(1.0 / std::tan((double)value));
			}
			static t map(i value, i from_min, i from_max, i to_min, i to_max)
			{
				return to_min + (value - from_min) * (to_max - to_min) / (from_max - from_min);
			}
			static void swap(t& value0, t& value1)
			{
				t value2 = value0;
				value0 = value1;
				value1 = value2;
			}
		};

		typedef math<core::decimal> math0;
		typedef math<uint128> math128u;
		typedef math<uint256> math256u;
		typedef math<int32_t> math32;
		typedef math<uint32_t> math32u;
		typedef math<int64_t> math64;
		typedef math<uint64_t> math64u;
		typedef math<float> mathf;
		typedef math<double> mathd;
	}
}

using uint128_t = vitex::compute::uint128;
using uint256_t = vitex::compute::uint256;
#endif