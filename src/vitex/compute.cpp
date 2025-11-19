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
#include <windows.h>
#else
#include <time.h>
#endif
#ifdef VI_OPENSSL
#define OSSL_DEPRECATED(x)
extern "C"
{
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/err.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/dsa.h>
#include <openssl/rsa.h>
}
#endif
#define REGEX_FAIL(a, b) if (a) return (b)
#define REGEX_FAIL_IN(a, b) if (a) { state = b; return; }
#define PARTITION_SIZE (sizeof(size_t) * 2)

namespace
{
	size_t offset_of64(const char* source, char dest)
	{
		VI_ASSERT(source != nullptr, "source should be set");
		for (size_t i = 0; i < 64; i++)
		{
			if (source[i] == dest)
				return i;
		}

		return 63;
	}
	vitex::core::string escape_text(const vitex::core::string& data)
	{
		vitex::core::string result = "\"";
		result.append(data).append("\"");
		return result;
	}
#ifdef VI_OPENSSL
	point_conversion_form_t to_point_conversion(vitex::compute::proofs::format format)
	{
		switch (format)
		{
			case vitex::compute::proofs::format::compressed:
				return POINT_CONVERSION_COMPRESSED;
			case vitex::compute::proofs::format::hybrid:
				return POINT_CONVERSION_HYBRID;
			case vitex::compute::proofs::format::uncompressed:
			default:
				return POINT_CONVERSION_UNCOMPRESSED;
		}
	}
#endif
}

namespace vitex
{
	namespace compute
	{
		regex_source::regex_source() noexcept :
			max_branches(128), max_brackets(128), max_matches(128),
			state(regex_state::no_match), ignore_case(false)
		{
		}
		regex_source::regex_source(const std::string_view& regexp, bool fIgnoreCase, int64_t fMaxMatches, int64_t fMaxBranches, int64_t fMaxBrackets) noexcept :
			expression(regexp),
			max_branches(fMaxBranches >= 1 ? fMaxBranches : 128),
			max_brackets(fMaxBrackets >= 1 ? fMaxBrackets : 128),
			max_matches(fMaxMatches >= 1 ? fMaxMatches : 128),
			state(regex_state::preprocessed), ignore_case(fIgnoreCase)
		{
			compile();
		}
		regex_source::regex_source(const regex_source& other) noexcept :
			expression(other.expression),
			max_branches(other.max_branches),
			max_brackets(other.max_brackets),
			max_matches(other.max_matches),
			state(other.state), ignore_case(other.ignore_case)
		{
			compile();
		}
		regex_source::regex_source(regex_source&& other) noexcept :
			expression(std::move(other.expression)),
			max_branches(other.max_branches),
			max_brackets(other.max_brackets),
			max_matches(other.max_matches),
			state(other.state), ignore_case(other.ignore_case)
		{
			brackets.reserve(other.brackets.capacity());
			branches.reserve(other.branches.capacity());
			compile();
		}
		regex_source& regex_source::operator=(const regex_source& v) noexcept
		{
			VI_ASSERT(this != &v, "cannot set to self");
			brackets.clear();
			brackets.reserve(v.brackets.capacity());
			branches.clear();
			branches.reserve(v.branches.capacity());
			expression = v.expression;
			ignore_case = v.ignore_case;
			state = v.state;
			max_brackets = v.max_brackets;
			max_branches = v.max_branches;
			max_matches = v.max_matches;
			compile();

			return *this;
		}
		regex_source& regex_source::operator=(regex_source&& v) noexcept
		{
			VI_ASSERT(this != &v, "cannot set to self");
			brackets.clear();
			brackets.reserve(v.brackets.capacity());
			branches.clear();
			branches.reserve(v.branches.capacity());
			expression = std::move(v.expression);
			ignore_case = v.ignore_case;
			state = v.state;
			max_brackets = v.max_brackets;
			max_branches = v.max_branches;
			max_matches = v.max_matches;
			compile();

			return *this;
		}
		const core::string& regex_source::get_regex() const
		{
			return expression;
		}
		int64_t regex_source::get_max_branches() const
		{
			return max_branches;
		}
		int64_t regex_source::get_max_brackets() const
		{
			return max_brackets;
		}
		int64_t regex_source::get_max_matches() const
		{
			return max_matches;
		}
		int64_t regex_source::get_complexity() const
		{
			int64_t a = (int64_t)branches.size() + 1;
			int64_t b = (int64_t)brackets.size();
			int64_t c = 0;

			for (size_t i = 0; i < (size_t)b; i++)
				c += brackets[i].length;

			return (int64_t)expression.size() + a * b * c;
		}
		regex_state regex_source::get_state() const
		{
			return state;
		}
		bool regex_source::is_simple() const
		{
			return !core::stringify::find_of(expression, "\\+*?|[]").found;
		}
		void regex_source::compile()
		{
			const char* vPtr = expression.c_str();
			int64_t vSize = (int64_t)expression.size();
			brackets.reserve(8);
			branches.reserve(8);

			regex_bracket bracket;
			bracket.pointer = vPtr;
			bracket.length = vSize;
			brackets.push_back(bracket);

			int64_t step = 0, depth = 0;
			for (int64_t i = 0; i < vSize; i += step)
			{
				step = regex::get_op_length(vPtr + i, vSize - i);
				if (vPtr[i] == '|')
				{
					regex_branch branch;
					branch.bracket_index = (brackets.back().length == -1 ? brackets.size() - 1 : depth);
					branch.pointer = &vPtr[i];
					branches.push_back(branch);
				}
				else if (vPtr[i] == '\\')
				{
					REGEX_FAIL_IN(i >= vSize - 1, regex_state::invalid_metacharacter);
					if (vPtr[i + 1] == 'x')
					{
						REGEX_FAIL_IN(i >= vSize - 3, regex_state::invalid_metacharacter);
						REGEX_FAIL_IN(!(isxdigit(vPtr[i + 2]) && isxdigit(vPtr[i + 3])), regex_state::invalid_metacharacter);
					}
					else
					{
						REGEX_FAIL_IN(!regex::meta((const uint8_t*)vPtr + i + 1), regex_state::invalid_metacharacter);
					}
				}
				else if (vPtr[i] == '(')
				{
					depth++;
					bracket.pointer = vPtr + i + 1;
					bracket.length = -1;
					brackets.push_back(bracket);
				}
				else if (vPtr[i] == ')')
				{
					int64_t idx = (brackets[brackets.size() - 1].length == -1 ? brackets.size() - 1 : depth);
					brackets[(size_t)idx].length = (int64_t)(&vPtr[i] - brackets[(size_t)idx].pointer); depth--;
					REGEX_FAIL_IN(depth < 0, regex_state::unbalanced_brackets);
					REGEX_FAIL_IN(i > 0 && vPtr[i - 1] == '(', regex_state::no_match);
				}
			}

			REGEX_FAIL_IN(depth != 0, regex_state::unbalanced_brackets);

			regex_branch branch; size_t i, j;
			for (i = 0; i < branches.size(); i++)
			{
				for (j = i + 1; j < branches.size(); j++)
				{
					if (branches[i].bracket_index > branches[j].bracket_index)
					{
						branch = branches[i];
						branches[i] = branches[j];
						branches[j] = branch;
					}
				}
			}

			for (i = j = 0; i < brackets.size(); i++)
			{
				auto& bracket = brackets[i];
				bracket.branches_count = 0;
				bracket.branches = j;

				while (j < branches.size() && branches[j].bracket_index == i)
				{
					bracket.branches_count++;
					j++;
				}
			}
		}

		regex_result::regex_result() noexcept : state(regex_state::no_match), steps(0), src(nullptr)
		{
		}
		regex_result::regex_result(const regex_result& other) noexcept : matches(other.matches), state(other.state), steps(other.steps), src(other.src)
		{
		}
		regex_result::regex_result(regex_result&& other) noexcept : matches(std::move(other.matches)), state(other.state), steps(other.steps), src(other.src)
		{
		}
		regex_result& regex_result::operator =(const regex_result& v) noexcept
		{
			matches = v.matches;
			src = v.src;
			steps = v.steps;
			state = v.state;

			return *this;
		}
		regex_result& regex_result::operator =(regex_result&& v) noexcept
		{
			matches.swap(v.matches);
			src = v.src;
			steps = v.steps;
			state = v.state;

			return *this;
		}
		bool regex_result::empty() const
		{
			return matches.empty();
		}
		int64_t regex_result::get_steps() const
		{
			return steps;
		}
		regex_state regex_result::get_state() const
		{
			return state;
		}
		const core::vector<regex_match>& regex_result::get() const
		{
			return matches;
		}
		core::vector<core::string> regex_result::to_array() const
		{
			core::vector<core::string> array;
			array.reserve(matches.size());

			for (auto& item : matches)
				array.emplace_back(item.pointer, (size_t)item.length);

			return array;
		}

		bool regex::match(regex_source* value, regex_result& result, const std::string_view& buffer)
		{
			VI_ASSERT(value != nullptr && value->state == regex_state::preprocessed, "invalid regex source");
			VI_ASSERT(!buffer.empty(), "invalid buffer");

			result.src = value;
			result.state = regex_state::preprocessed;
			result.matches.clear();
			result.matches.reserve(8);

			int64_t code = parse(buffer.data(), buffer.size(), &result);
			if (code <= 0)
			{
				result.state = (regex_state)code;
				result.matches.clear();
				return false;
			}

			for (auto it = result.matches.begin(); it != result.matches.end(); ++it)
			{
				it->start = it->pointer - buffer.data();
				it->end = it->start + it->length;
			}

			result.state = regex_state::match_found;
			return true;
		}
		bool regex::replace(regex_source* value, const std::string_view& to, core::string& buffer)
		{
			core::string emplace;
			regex_result result;
			size_t matches = 0;

			bool expression = (!to.empty() && to.find('$') != core::string::npos);
			if (!expression)
				emplace.assign(to);

			size_t start = 0;
			while (match(value, result, std::string_view(buffer.c_str() + start, buffer.size() - start)))
			{
				matches++;
				if (result.matches.empty())
					continue;

				if (expression)
				{
					emplace.assign(to);
					for (size_t i = 1; i < result.matches.size(); i++)
					{
						auto& item = result.matches[i];
						core::stringify::replace(emplace, "$" + core::to_string(i), core::string(item.pointer, (size_t)item.length));
					}
				}

				auto& where = result.matches.front();
				core::stringify::replace_part(buffer, (size_t)where.start + start, (size_t)where.end + start, emplace);
				start += (size_t)where.start + (size_t)emplace.size() - (emplace.empty() ? 0 : 1);
			}

			return matches > 0;
		}
		int64_t regex::meta(const uint8_t* buffer)
		{
			VI_ASSERT(buffer != nullptr, "invalid buffer");
			static const char* chars = "^$().[]*+?|\\Ssdbfnrtv";
			return strchr(chars, *buffer) != nullptr;
		}
		int64_t regex::op_length(const char* value)
		{
			VI_ASSERT(value != nullptr, "invalid value");
			return value[0] == '\\' && value[1] == 'x' ? 4 : value[0] == '\\' ? 2 : 1;
		}
		int64_t regex::set_length(const char* value, int64_t value_length)
		{
			VI_ASSERT(value != nullptr, "invalid value");
			int64_t length = 0;
			while (length < value_length && value[length] != ']')
				length += op_length(value + length);

			return length <= value_length ? length + 1 : -1;
		}
		int64_t regex::get_op_length(const char* value, int64_t value_length)
		{
			VI_ASSERT(value != nullptr, "invalid value");
			return value[0] == '[' ? set_length(value + 1, value_length - 1) + 1 : op_length(value);
		}
		int64_t regex::quantifier(const char* value)
		{
			VI_ASSERT(value != nullptr, "invalid value");
			return value[0] == '*' || value[0] == '+' || value[0] == '?';
		}
		int64_t regex::to_int(int64_t x)
		{
			return (int64_t)(isdigit((int)x) ? (int)x - '0' : (int)x - 'W');
		}
		int64_t regex::hex_to_int(const uint8_t* buffer)
		{
			VI_ASSERT(buffer != nullptr, "invalid buffer");
			return (to_int(tolower(buffer[0])) << 4) | to_int(tolower(buffer[1]));
		}
		int64_t regex::match_op(const uint8_t* value, const uint8_t* buffer, regex_result* info)
		{
			VI_ASSERT(buffer != nullptr, "invalid buffer");
			VI_ASSERT(value != nullptr, "invalid value");
			VI_ASSERT(info != nullptr, "invalid regex result");

			int64_t result = 0;
			switch (*value)
			{
				case '\\':
					switch (value[1])
					{
						case 'S':
							REGEX_FAIL(isspace(*buffer), (int64_t)regex_state::no_match);
							result++;
							break;
						case 's':
							REGEX_FAIL(!isspace(*buffer), (int64_t)regex_state::no_match);
							result++;
							break;
						case 'd':
							REGEX_FAIL(!isdigit(*buffer), (int64_t)regex_state::no_match);
							result++;
							break;
						case 'b':
							REGEX_FAIL(*buffer != '\b', (int64_t)regex_state::no_match);
							result++;
							break;
						case 'f':
							REGEX_FAIL(*buffer != '\f', (int64_t)regex_state::no_match);
							result++;
							break;
						case 'n':
							REGEX_FAIL(*buffer != '\n', (int64_t)regex_state::no_match);
							result++;
							break;
						case 'r':
							REGEX_FAIL(*buffer != '\r', (int64_t)regex_state::no_match);
							result++;
							break;
						case 't':
							REGEX_FAIL(*buffer != '\t', (int64_t)regex_state::no_match);
							result++;
							break;
						case 'v':
							REGEX_FAIL(*buffer != '\v', (int64_t)regex_state::no_match);
							result++;
							break;
						case 'x':
							REGEX_FAIL((uint8_t)hex_to_int(value + 2) != *buffer, (int64_t)regex_state::no_match);
							result++;
							break;
						default:
							REGEX_FAIL(value[1] != buffer[0], (int64_t)regex_state::no_match);
							result++;
							break;
					}
					break;
				case '|':
					REGEX_FAIL(1, (int64_t)regex_state::internal_error);
					break;
				case '$':
					REGEX_FAIL(1, (int64_t)regex_state::no_match);
					break;
				case '.':
					result++;
					break;
				default:
					if (info->src->ignore_case)
					{
						REGEX_FAIL(tolower(*value) != tolower(*buffer), (int64_t)regex_state::no_match);
					}
					else
					{
						REGEX_FAIL(*value != *buffer, (int64_t)regex_state::no_match);
					}
					result++;
					break;
			}

			return result;
		}
		int64_t regex::match_set(const char* value, int64_t value_length, const char* buffer, regex_result* info)
		{
			VI_ASSERT(buffer != nullptr, "invalid buffer");
			VI_ASSERT(value != nullptr, "invalid value");
			VI_ASSERT(info != nullptr, "invalid regex result");

			int64_t length = 0, result = -1, invert = value[0] == '^';
			if (invert)
				value++, value_length--;

			while (length <= value_length && value[length] != ']' && result <= 0)
			{
				if (value[length] != '-' && value[length + 1] == '-' && value[length + 2] != ']' && value[length + 2] != '\0')
				{
					result = (info->src->ignore_case) ? tolower(*buffer) >= tolower(value[length]) && tolower(*buffer) <= tolower(value[length + 2]) : *buffer >= value[length] && *buffer <= value[length + 2];
					length += 3;
				}
				else
				{
					result = match_op((const uint8_t*)value + length, (const uint8_t*)buffer, info);
					length += op_length(value + length);
				}
			}

			return (!invert && result > 0) || (invert && result <= 0) ? 1 : -1;
		}
		int64_t regex::parse_inner(const char* value, int64_t value_length, const char* buffer, int64_t buffer_length, regex_result* info, int64_t condition)
		{
			VI_ASSERT(buffer != nullptr, "invalid buffer");
			VI_ASSERT(value != nullptr, "invalid value");
			VI_ASSERT(info != nullptr, "invalid regex result");

			int64_t i, j, n, step;
			for (i = j = 0; i < value_length && j <= buffer_length; i += step)
			{
				step = value[i] == '(' ? info->src->brackets[(size_t)condition + 1].length + 2 : get_op_length(value + i, value_length - i);
				REGEX_FAIL(quantifier(&value[i]), (int64_t)regex_state::unexpected_quantifier);
				REGEX_FAIL(step <= 0, (int64_t)regex_state::invalid_character_set);
				info->steps++;

				if (i + step < value_length && quantifier(value + i + step))
				{
					if (value[i + step] == '?')
					{
						int64_t result = parse_inner(value + i, step, buffer + j, buffer_length - j, info, condition);
						j += result > 0 ? result : 0;
						i++;
					}
					else if (value[i + step] == '+' || value[i + step] == '*')
					{
						int64_t j2 = j, nj = j, n1, n2 = -1, ni, non_greedy = 0;
						ni = i + step + 1;
						if (ni < value_length && value[ni] == '?')
						{
							non_greedy = 1;
							ni++;
						}

						do
						{
							if ((n1 = parse_inner(value + i, step, buffer + j2, buffer_length - j2, info, condition)) > 0)
								j2 += n1;

							if (value[i + step] == '+' && n1 < 0)
								break;

							if (ni >= value_length)
								nj = j2;
							else if ((n2 = parse_inner(value + ni, value_length - ni, buffer + j2, buffer_length - j2, info, condition)) >= 0)
								nj = j2 + n2;

							if (nj > j && non_greedy)
								break;
						} while (n1 > 0);

						if (n1 < 0 && n2 < 0 && value[i + step] == '*' && (n2 = parse_inner(value + ni, value_length - ni, buffer + j, buffer_length - j, info, condition)) > 0)
							nj = j + n2;

						REGEX_FAIL(value[i + step] == '+' && nj == j, (int64_t)regex_state::no_match);
						REGEX_FAIL(nj == j && ni < value_length && n2 < 0, (int64_t)regex_state::no_match);
						return nj;
					}

					continue;
				}

				if (value[i] == '[')
				{
					n = match_set(value + i + 1, value_length - (i + 2), buffer + j, info);
					REGEX_FAIL(n <= 0, (int64_t)regex_state::no_match);
					j += n;
				}
				else if (value[i] == '(')
				{
					n = (int64_t)regex_state::no_match;
					condition++;

					REGEX_FAIL(condition >= (int64_t)info->src->brackets.size(), (int64_t)regex_state::internal_error);
					if (value_length - (i + step) > 0)
					{
						int64_t j2;
						for (j2 = 0; j2 <= buffer_length - j; j2++)
						{
							if ((n = parse_doh(buffer + j, buffer_length - (j + j2), info, condition)) >= 0 && parse_inner(value + i + step, value_length - (i + step), buffer + j + n, buffer_length - (j + n), info, condition) >= 0)
								break;
						}
					}
					else
						n = parse_doh(buffer + j, buffer_length - j, info, condition);

					REGEX_FAIL(n < 0, n);
					if (n > 0)
					{
						regex_match* match;
						if (condition - 1 >= (int64_t)info->matches.size())
						{
							info->matches.emplace_back();
							match = &info->matches[info->matches.size() - 1];
						}
						else
							match = &info->matches.at((size_t)condition - 1);

						match->pointer = buffer + j;
						match->length = n;
						match->steps++;
					}
					j += n;
				}
				else if (value[i] == '^')
				{
					REGEX_FAIL(j != 0, (int64_t)regex_state::no_match);
				}
				else if (value[i] == '$')
				{
					REGEX_FAIL(j != buffer_length, (int64_t)regex_state::no_match);
				}
				else
				{
					REGEX_FAIL(j >= buffer_length, (int64_t)regex_state::no_match);
					n = match_op((const uint8_t*)(value + i), (const uint8_t*)(buffer + j), info);
					REGEX_FAIL(n <= 0, n);
					j += n;
				}
			}

			return j;
		}
		int64_t regex::parse_doh(const char* buffer, int64_t buffer_length, regex_result* info, int64_t condition)
		{
			VI_ASSERT(buffer != nullptr, "invalid buffer");
			VI_ASSERT(info != nullptr, "invalid regex result");

			const regex_bracket* bk = &info->src->brackets[(size_t)condition];
			int64_t i = 0, length, result;
			const char* ptr;

			do
			{
				ptr = i == 0 ? bk->pointer : info->src->branches[(size_t)(bk->branches + i - 1)].pointer + 1;
				length = bk->branches_count == 0 ? bk->length : i == bk->branches_count ? (int64_t)(bk->pointer + bk->length - ptr) : (int64_t)(info->src->branches[(size_t)(bk->branches + i)].pointer - ptr);
				result = parse_inner(ptr, length, buffer, buffer_length, info, condition);
				VI_MEASURE_LOOP();
			} while (result <= 0 && i++ < bk->branches_count);

			return result;
		}
		int64_t regex::parse(const char* buffer, int64_t buffer_length, regex_result* info)
		{
			VI_ASSERT(buffer != nullptr, "invalid buffer");
			VI_ASSERT(info != nullptr, "invalid regex result");
			VI_MEASURE(core::timings::frame);

			int64_t is_anchored = info->src->brackets[0].pointer[0] == '^', i, result = -1;
			for (i = 0; i <= buffer_length; i++)
			{
				result = parse_doh(buffer + i, buffer_length - i, info, 0);
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
		const char* regex::syntax()
		{
			return
				"\"^\" - match beginning of a buffer\n"
				"\"$\" - match end of a buffer\n"
				"\"()\" - grouping and substring capturing\n"
				"\"\\s\" - match whitespace\n"
				"\"\\s\" - match non - whitespace\n"
				"\"\\d\" - match decimal digit\n"
				"\"\\n\" - match new line character\n"
				"\"\\r\" - match line feed character\n"
				"\"\\f\" - match form feed character\n"
				"\"\\v\" - match vertical tab character\n"
				"\"\\t\" - match horizontal tab character\n"
				"\"\\b\" - match backspace character\n"
				"\"+\" - match one or more times (greedy)\n"
				"\"+?\" - match one or more times (non - greedy)\n"
				"\"*\" - match zero or more times (greedy)\n"
				"\"*?\" - match zero or more times(non - greedy)\n"
				"\"?\" - match zero or once(non - greedy)\n"
				"\"x|y\" - match x or y(alternation operator)\n"
				"\"\\meta\" - match one of the meta character: ^$().[]*+?|\\\n"
				"\"\\xHH\" - match byte with hex value 0xHH, e.g. \\x4a\n"
				"\"[...]\" - match any character from set. ranges like[a-z] are supported\n"
				"\"[^...]\" - match any character but ones from set\n";
		}

		secret_box::secret_box() noexcept : type((box_type)std::numeric_limits<uint8_t>::max())
		{
		}
		secret_box::secret_box(box_type new_type) noexcept : type(new_type)
		{
		}
		secret_box::secret_box(const secret_box& other) noexcept : type((box_type)std::numeric_limits<uint8_t>::max())
		{
			copy_distribution(other);
		}
		secret_box::secret_box(secret_box&& other) noexcept : type((box_type)std::numeric_limits<uint8_t>::max())
		{
			move_distribution(std::move(other));
		}
		secret_box::~secret_box() noexcept
		{
			clear();
		}
		secret_box& secret_box::operator =(const secret_box& v) noexcept
		{
			copy_distribution(v);
			return *this;
		}
		secret_box& secret_box::operator =(secret_box&& v) noexcept
		{
			move_distribution(std::move(v));
			return *this;
		}
		void secret_box::clear()
		{
			switch (type)
			{
				case box_type::secure:
				{
					size_t size = data.set.size();
					for (size_t i = 0; i < size; i++)
					{
						size_t* partition = (size_t*)data.set[i];
						roll_partition(partition, size, i);
						core::memory::deallocate(partition);
					}
					data.set.~vector();
					break;
				}
				case box_type::insecure:
					randomize_buffer(data.sequence.data(), data.sequence.size());
					data.sequence.~basic_string();
					break;
				case box_type::view:
					data.view.~basic_string_view();
					break;
				default:
					break;
			}
			type = (box_type)std::numeric_limits<uint8_t>::max();
		}
		void secret_box::stack(char* buffer, size_t max_size, size_t* out_size) const
		{
			VI_TRACE("crypto stack expose secret box to 0x%" PRIXPTR, (void*)buffer);
			switch (type)
			{
				case box_type::secure:
				{
					size_t size = std::min(max_size - 1, data.set.size());
					for (size_t i = 0; i < size; i++)
						buffer[i] = load_partition((size_t*)data.set[i], size, i);
					buffer[size] = '\0';
					if (out_size != nullptr)
						*out_size = size;
					break;
				}
				case box_type::insecure:
				{
					size_t size = std::min(max_size - 1, data.sequence.size());
					memcpy(buffer, data.sequence.data(), sizeof(char) * size);
					buffer[size] = '\0';
					if (out_size != nullptr)
						*out_size = size;
					break;
				}
				case box_type::view:
				{
					size_t size = std::min(max_size - 1, data.view.size());
					memcpy(buffer, data.view.data(), sizeof(char) * size);
					buffer[size] = '\0';
					if (out_size != nullptr)
						*out_size = size;
					break;
				}
				default:
					break;
			}
		}
		core::string secret_box::heap() const
		{
			VI_TRACE("crypto heap expose secret box from 0x%" PRIXPTR, (void*)this);
			core::string result = core::string(size(), '\0');
			stack(result.data(), result.size() + 1);
			return result;
		}
		void secret_box::copy_distribution(const secret_box& other)
		{
			VI_TRACE("crypto copy secret box from 0x%" PRIXPTR, (void*)&other);
			clear();
			type = other.type;
			switch (type)
			{
				case box_type::secure:
				{
					new (&data.set) core::vector<void*>();
					data.set.reserve(other.data.set.size());
					for (auto* partition : other.data.set)
					{
						void* copied_partition = core::memory::allocate<void>(PARTITION_SIZE);
						memcpy(copied_partition, partition, PARTITION_SIZE);
						data.set.emplace_back(copied_partition);
					}
					break;
				}
				case box_type::insecure:
					new (&data.sequence) core::string(other.data.sequence);
					break;
				case box_type::view:
					new (&data.view) std::string_view(other.data.view);
					break;
				default:
					break;
			}
		}
		void secret_box::move_distribution(secret_box&& other)
		{
			clear();
			type = other.type;
			switch (type)
			{
				case box_type::secure:
					new (&data.set) core::vector<void*>(std::move(other.data.set));
					break;
				case box_type::insecure:
					new (&data.sequence) core::string(std::move(other.data.sequence));
					break;
				case box_type::view:
					new (&data.view) std::string_view(std::move(other.data.view));
					break;
				default:
					break;
			}
		}
		size_t secret_box::size() const
		{
			switch (type)
			{
				case box_type::secure:
					return data.set.size();
				case box_type::insecure:
					return data.sequence.size();
				case box_type::view:
					return data.view.size();
				default:
					return 0;
			}
		}
		bool secret_box::empty() const
		{
			switch (type)
			{
				case box_type::secure:
					return data.set.empty();
				case box_type::insecure:
					return data.sequence.empty();
				case box_type::view:
					return data.view.empty();
				default:
					return true;
			}
		}
		char secret_box::load_partition(size_t* dest, size_t size, size_t index)
		{
			char* buffer = (char*)dest + sizeof(size_t);
			return buffer[dest[0]];
		}
		void secret_box::roll_partition(size_t* dest, size_t size, size_t index)
		{
			char* buffer = (char*)dest + sizeof(size_t);
			buffer[dest[0]] = (char)(crypto::random() % std::numeric_limits<uint8_t>::max());
		}
		void secret_box::fill_partition(size_t* dest, size_t size, size_t index, char source)
		{
			dest[0] = ((size_t(source) << 16) + 17) % (PARTITION_SIZE - sizeof(size_t));
			char* buffer = (char*)dest + sizeof(size_t);
			for (size_t i = 0; i < PARTITION_SIZE - sizeof(size_t); i++)
				buffer[i] = (char)(crypto::random() % std::numeric_limits<uint8_t>::max());
			buffer[dest[0]] = source;
		}
		void secret_box::randomize_buffer(char* buffer, size_t size)
		{
			for (size_t i = 0; i < size; i++)
				buffer[i] = crypto::random() % std::numeric_limits<char>::max();
		}
		secret_box secret_box::secure(const std::string_view& value)
		{
			VI_TRACE("crypto secure secret box on %" PRIu64 " bytes", (uint64_t)value.size());
			secret_box result = secret_box(box_type::secure);
			new (&result.data.set) core::vector<void*>();
			result.data.set.reserve(value.size());
			for (size_t i = 0; i < value.size(); i++)
			{
				size_t* partition = core::memory::allocate<size_t>(PARTITION_SIZE);
				fill_partition(partition, value.size(), i, value[i]);
				result.data.set.emplace_back(partition);
			}
			return result;
		}
		secret_box secret_box::insecure(core::string&& value)
		{
			VI_TRACE("crypto insecure secret box on %" PRIu64 " bytes", (uint64_t)value.size());
			secret_box result = secret_box(box_type::insecure);
			new (&result.data.sequence) core::string(std::move(value));
			return result;
		}
		secret_box secret_box::insecure(const std::string_view& value)
		{
			VI_TRACE("crypto insecure secret box on %" PRIu64 " bytes", (uint64_t)value.size());
			secret_box result = secret_box(box_type::insecure);
			new (&result.data.sequence) core::string(value);
			return result;
		}
		secret_box secret_box::view(const std::string_view& value)
		{
			VI_TRACE("crypto view secret box on %" PRIu64 " bytes", (uint64_t)value.size());
			secret_box result = secret_box(box_type::view);
			new (&result.data.view) std::string_view(value);
			return result;
		}

		uint128::uint128(const std::string_view& text) : uint128(text, 10)
		{
		}
		uint128::uint128(const std::string_view& text, uint8_t base)
		{
			if (text.empty())
			{
				lower = upper = 0;
				return;
			}

			size_t size = text.size();
			char* data = (char*)text.data();
			while (size > 0 && core::stringify::is_whitespace(*data))
			{
				++data;
				size--;
			}

			switch (base)
			{
				case 16:
				{
					static const size_t MAX_LEN = 32;
					const size_t max_len = std::min(size, MAX_LEN);
					const size_t starting_index = (MAX_LEN < size) ? (size - MAX_LEN) : 0;
					const size_t double_lower = sizeof(lower) * 2;
					const size_t lower_len = (max_len >= double_lower) ? double_lower : max_len;
					const size_t upper_len = (max_len >= double_lower) ? (max_len - double_lower) : 0;

					std::stringstream lower_s, upper_s;
					upper_s << std::hex << core::string(data + starting_index, upper_len);
					lower_s << std::hex << core::string(data + starting_index + upper_len, lower_len);
					upper_s >> upper;
					lower_s >> lower;
					break;
				}
				case 10:
				{
					static const size_t MAX_LEN = 39;
					const size_t max_len = std::min(size, MAX_LEN);
					const size_t starting_index = (MAX_LEN < size) ? (size - MAX_LEN) : 0;
					data += starting_index;

					for (size_t i = 0; *data && ('0' <= *data) && (*data <= '9') && (i < max_len); ++data, ++i)
					{
						*this *= 10;
						*this += *data - '0';
					}
					break;
				}
				case 8:
				{
					static const size_t MAX_LEN = 43;
					const size_t max_len = std::min(size, MAX_LEN);
					const size_t starting_index = (MAX_LEN < size) ? (size - MAX_LEN) : 0;
					data += starting_index;

					for (size_t i = 0; *data && ('0' <= *data) && (*data <= '7') && (i < max_len); ++data, ++i)
					{
						*this *= 8;
						*this += *data - '0';
					}
					break;
				}
				case 2:
				{
					static const size_t MAX_LEN = 128;
					const size_t max_len = std::min(size, MAX_LEN);
					const size_t starting_index = (MAX_LEN < size) ? (size - MAX_LEN) : 0;
					const size_t eight_lower = sizeof(lower) * 8;
					const size_t lower_len = (max_len >= eight_lower) ? eight_lower : max_len;
					const size_t upper_len = (max_len >= eight_lower) ? (max_len - eight_lower) : 0;
					data += starting_index;

					for (size_t i = 0; *data && ('0' <= *data) && (*data <= '1') && (i < upper_len); ++data, ++i)
					{
						upper <<= 1;
						upper |= *data - '0';
					}

					for (size_t i = 0; *data && ('0' <= *data) && (*data <= '1') && (i < lower_len); ++data, ++i)
					{
						lower <<= 1;
						lower |= *data - '0';
					}
					break;
				}
				default:
					VI_ASSERT(false, "invalid from string base: %i", (int)base);
					break;
			}
		}
		uint128::operator bool() const
		{
			return (bool)(upper || lower);
		}
		uint128::operator uint8_t() const
		{
			return (uint8_t)lower;
		}
		uint128::operator uint16_t() const
		{
			return (uint16_t)lower;
		}
		uint128::operator uint32_t() const
		{
			return (uint32_t)lower;
		}
		uint128::operator uint64_t() const
		{
			return (uint64_t)lower;
		}
		uint128 uint128::operator&(const uint128& right) const
		{
			return uint128(upper & right.upper, lower & right.lower);
		}
		uint128& uint128::operator&=(const uint128& right)
		{
			upper &= right.upper;
			lower &= right.lower;
			return *this;
		}
		uint128 uint128::operator|(const uint128& right) const
		{
			return uint128(upper | right.upper, lower | right.lower);
		}
		uint128& uint128::operator|=(const uint128& right)
		{
			upper |= right.upper;
			lower |= right.lower;
			return *this;
		}
		uint128 uint128::operator^(const uint128& right) const
		{
			return uint128(upper ^ right.upper, lower ^ right.lower);
		}
		uint128& uint128::operator^=(const uint128& right)
		{
			upper ^= right.upper;
			lower ^= right.lower;
			return *this;
		}
		uint128 uint128::operator~() const
		{
			return uint128(~upper, ~lower);
		}
		uint128 uint128::operator<<(const uint128& right) const
		{
			const uint64_t shift = right.lower;
			if (((bool)right.upper) || (shift >= 128))
				return uint128(0);
			else if (shift == 64)
				return uint128(lower, 0);
			else if (shift == 0)
				return *this;
			else if (shift < 64)
				return uint128((upper << shift) + (lower >> (64 - shift)), lower << shift);
			else if ((128 > shift) && (shift > 64))
				return uint128(lower << (shift - 64), 0);

			return uint128(0);
		}
		uint128& uint128::operator<<=(const uint128& right)
		{
			*this = *this << right;
			return *this;
		}
		uint128 uint128::operator>>(const uint128& right) const
		{
			const uint64_t shift = right.lower;
			if (((bool)right.upper) || (shift >= 128))
				return uint128(0);
			else if (shift == 64)
				return uint128(0, upper);
			else if (shift == 0)
				return *this;
			else if (shift < 64)
				return uint128(upper >> shift, (upper << (64 - shift)) + (lower >> shift));
			else if ((128 > shift) && (shift > 64))
				return uint128(0, (upper >> (shift - 64)));

			return uint128(0);
		}
		uint128& uint128::operator>>=(const uint128& right)
		{
			*this = *this >> right;
			return *this;
		}
		bool uint128::operator!() const
		{
			return !(bool)(upper || lower);
		}
		bool uint128::operator&&(const uint128& right) const
		{
			return ((bool)*this && right);
		}
		bool uint128::operator||(const uint128& right) const
		{
			return ((bool)*this || right);
		}
		bool uint128::operator==(const uint128& right) const
		{
			return ((upper == right.upper) && (lower == right.lower));
		}
		bool uint128::operator!=(const uint128& right) const
		{
			return ((upper != right.upper) || (lower != right.lower));
		}
		bool uint128::operator>(const uint128& right) const
		{
			if (upper == right.upper)
				return (lower > right.lower);

			return (upper > right.upper);
		}
		bool uint128::operator<(const uint128& right) const
		{
			if (upper == right.upper)
				return (lower < right.lower);

			return (upper < right.upper);
		}
		bool uint128::operator>=(const uint128& right) const
		{
			return ((*this > right) || (*this == right));
		}
		bool uint128::operator<=(const uint128& right) const
		{
			return ((*this < right) || (*this == right));
		}
		uint128 uint128::operator+(const uint128& right) const
		{
			return uint128(upper + right.upper + ((lower + right.lower) < lower), lower + right.lower);
		}
		uint128& uint128::operator+=(const uint128& right)
		{
			upper += right.upper + ((lower + right.lower) < lower);
			lower += right.lower;
			return *this;
		}
		uint128 uint128::operator-(const uint128& right) const
		{
			return uint128(upper - right.upper - ((lower - right.lower) > lower), lower - right.lower);
		}
		uint128& uint128::operator-=(const uint128& right)
		{
			*this = *this - right;
			return *this;
		}
		uint128 uint128::operator*(const uint128& right) const
		{
			uint64_t top[4] = { upper >> 32, upper & 0xffffffff, lower >> 32, lower & 0xffffffff };
			uint64_t bottom[4] = { right.upper >> 32, right.upper & 0xffffffff, right.lower >> 32, right.lower & 0xffffffff };
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
			return uint128((first32 << 32) | second32, (third32 << 32) | fourth32);
		}
		uint128& uint128::operator*=(const uint128& right)
		{
			*this = *this * right;
			return *this;
		}
		uint128 uint128::min()
		{
			static uint128 value = uint128(0);
			return value;
		}
		uint128 uint128::max()
		{
			static uint128 value = uint128(-1, -1);
			return value;
		}
		std::pair<uint128, uint128> uint128::divide(const uint128& left, const uint128& right) const
		{
			uint128 zero(0), one(1);
			if (right == zero)
			{
				VI_ASSERT(false, "division or modulus by zero");
				return std::pair<uint128, uint128>(zero, zero);
			}
			else if (right == one)
				return std::pair <uint128, uint128>(left, zero);
			else if (left == right)
				return std::pair <uint128, uint128>(one, zero);
			else if ((left == zero) || (left < right))
				return std::pair <uint128, uint128>(zero, left);

			std::pair <uint128, uint128> qr(zero, zero);
			for (uint8_t x = left.bits(); x > 0; x--)
			{
				qr.first <<= one;
				qr.second <<= one;
				if ((left >> (x - 1U)) & 1)
					++qr.second;

				if (qr.second >= right)
				{
					qr.second -= right;
					++qr.first;
				}
			}
			return qr;
		}
		uint128 uint128::operator/(const uint128& right) const
		{
			return divide(*this, right).first;
		}
		uint128& uint128::operator/=(const uint128& right)
		{
			*this = *this / right;
			return *this;
		}
		uint128 uint128::operator%(const uint128& right) const
		{
			return divide(*this, right).second;
		}
		uint128& uint128::operator%=(const uint128& right)
		{
			*this = *this % right;
			return *this;
		}
		uint128& uint128::operator++()
		{
			return *this += uint128(1);
		}
		uint128 uint128::operator++(int)
		{
			uint128 temp(*this);
			++*this;
			return temp;
		}
		uint128& uint128::operator--()
		{
			return *this -= uint128(1);
		}
		uint128 uint128::operator--(int)
		{
			uint128 temp(*this);
			--*this;
			return temp;
		}
		uint128 uint128::operator+() const
		{
			return *this;
		}
		uint128 uint128::operator-() const
		{
			return ~*this + uint128(1);
		}
		const uint64_t& uint128::high() const
		{
			return upper;
		}
		const uint64_t& uint128::low() const
		{
			return lower;
		}
		uint64_t& uint128::high()
		{
			return upper;
		}
		uint64_t& uint128::low()
		{
			return lower;
		}
		void uint128::encode(uint8_t data[16]) const
		{
			VI_ASSERT(data != nullptr, "data should be set");
			uint64_t array[2] =
			{
#if VI_ENDIAN_BIG
				core::os::hw::to_endianness(core::os::hw::endian::big, lower),
				core::os::hw::to_endianness(core::os::hw::endian::big, upper)
#else
				core::os::hw::to_endianness(core::os::hw::endian::big, upper),
				core::os::hw::to_endianness(core::os::hw::endian::big, lower)
#endif
			};
			memcpy((char*)data, array, sizeof(array));
		}
		void uint128::encode_compact(uint8_t data[16], size_t* data_size) const
		{
			VI_ASSERT(data != nullptr, "data should be set");
			uint8_t intermediate[sizeof(uint128)];
			encode(intermediate);

			size_t size = bytes();
			memcpy(data, intermediate + (sizeof(uint128) - size), size);
			if (data_size != nullptr)
				*data_size = size;
		}
		void uint128::decode(const uint8_t data[16])
		{
			VI_ASSERT(data != nullptr, "data should be set");
			uint64_t array[2] = { 0 };
			memcpy(array, data, sizeof(array));
			array[0] = core::os::hw::to_endianness(core::os::hw::endian::big, array[0]);
			array[1] = core::os::hw::to_endianness(core::os::hw::endian::big, array[1]);
#if VI_ENDIAN_BIG
			memcpy((uint64_t*)&lower, &array[0], sizeof(uint64_t));
			memcpy((uint64_t*)&upper, &array[1], sizeof(uint64_t));
#else
			memcpy((uint64_t*)&upper, &array[0], sizeof(uint64_t));
			memcpy((uint64_t*)&lower, &array[1], sizeof(uint64_t));
#endif
		}
		void uint128::decode_compact(const uint8_t* data, size_t data_size)
		{
			VI_ASSERT(data != nullptr, "data should be set");
			uint8_t intermediate[sizeof(uint128)] = { 0 };
			memcpy(intermediate, data, std::min(data_size, sizeof(intermediate)));
			decode(intermediate);
		}
		uint8_t uint128::bits() const
		{
			uint8_t out = 0;
			if (upper)
			{
				out = 64;
				uint64_t up = upper;
				while (up)
				{
					up >>= 1;
					out++;
				}
			}
			else
			{
				uint64_t low = lower;
				while (low)
				{
					low >>= 1;
					out++;
				}
			}
			return out;
		}
		uint8_t uint128::bytes() const
		{
			if (!*this)
				return 0;

			uint8_t length = bits();
			return std::max<uint8_t>(1, std::min(16, (length - length % 8 + 8) / 8));
		}
		core::decimal uint128::to_decimal() const
		{
			return core::decimal::from(to_string(16), 16);
		}
		core::string uint128::to_string(uint8_t base, uint32_t length) const
		{
			VI_ASSERT(base >= 2 && base <= 16, "base must be in the range [2, 16]");
			static const char* alphabet = "0123456789abcdef";
			core::string output;
			if (!(*this))
			{
				if (!length)
					output.push_back('0');
				else if (output.size() < length)
					output.append(length - output.size(), '0');
				return output;
			}

			switch (base)
			{
				case 16:
				{
					uint64_t array[2]; size_t size = bytes();
					if (size > sizeof(uint64_t) * 0)
					{
						array[1] = core::os::hw::to_endianness(core::os::hw::endian::big, lower);
						if (size > sizeof(uint64_t) * 1)
							array[0] = core::os::hw::to_endianness(core::os::hw::endian::big, upper);
					}

					length = std::max(length, (uint32_t)size * 2);
					output = codec::hex_encode(std::string_view(((char*)array) + (sizeof(array) - size), size));
					if (output.size() < length)
						output.append(length - output.size(), '0');
					while (output.size() > 1 && output.front() == '0')
						output.erase(0, 1);
					break;
				}
				default:
				{
					std::pair<uint256, uint256> remainder(*this, min());
					do
					{
						remainder = divide(remainder.first, base);
						output.push_back(alphabet[(uint8_t)remainder.second]);
					} while (remainder.first);
					if (output.size() < length)
						output.append(length - output.size(), '0');

					std::reverse(output.begin(), output.end());
					break;
				}
			}
			return output;
		}
		uint128 operator<<(const uint8_t& left, const uint128& right)
		{
			return uint128(left) << right;
		}
		uint128 operator<<(const uint16_t& left, const uint128& right)
		{
			return uint128(left) << right;
		}
		uint128 operator<<(const uint32_t& left, const uint128& right)
		{
			return uint128(left) << right;
		}
		uint128 operator<<(const uint64_t& left, const uint128& right)
		{
			return uint128(left) << right;
		}
		uint128 operator<<(const int8_t& left, const uint128& right)
		{
			return uint128(left) << right;
		}
		uint128 operator<<(const int16_t& left, const uint128& right)
		{
			return uint128(left) << right;
		}
		uint128 operator<<(const int32_t& left, const uint128& right)
		{
			return uint128(left) << right;
		}
		uint128 operator<<(const int64_t& left, const uint128& right)
		{
			return uint128(left) << right;
		}
		uint128 operator>>(const uint8_t& left, const uint128& right)
		{
			return uint128(left) >> right;
		}
		uint128 operator>>(const uint16_t& left, const uint128& right)
		{
			return uint128(left) >> right;
		}
		uint128 operator>>(const uint32_t& left, const uint128& right)
		{
			return uint128(left) >> right;
		}
		uint128 operator>>(const uint64_t& left, const uint128& right)
		{
			return uint128(left) >> right;
		}
		uint128 operator>>(const int8_t& left, const uint128& right)
		{
			return uint128(left) >> right;
		}
		uint128 operator>>(const int16_t& left, const uint128& right)
		{
			return uint128(left) >> right;
		}
		uint128 operator>>(const int32_t& left, const uint128& right)
		{
			return uint128(left) >> right;
		}
		uint128 operator>>(const int64_t& left, const uint128& right)
		{
			return uint128(left) >> right;
		}
		std::ostream& operator<<(std::ostream& stream, const uint128& right)
		{
			if (stream.flags() & stream.oct)
				stream << right.to_string(8);
			else if (stream.flags() & stream.dec)
				stream << right.to_string(10);
			else if (stream.flags() & stream.hex)
				stream << right.to_string(16);
			return stream;
		}

		uint256::uint256(const std::string_view& text) : uint256(text, 10)
		{
		}
		uint256::uint256(const std::string_view& text, uint8_t base)
		{
			*this = 0;
			uint256 power(1);
			uint8_t digit;
			int64_t pos = (int64_t)text.size() - 1;
			while (pos >= 0)
			{
				digit = 0;
				if ('0' <= text[pos] && text[pos] <= '9')
					digit = text[pos] - '0';
				else if ('a' <= text[pos] && text[pos] <= 'z')
					digit = text[pos] - 'a' + 10;
				*this += digit * power;
				power *= base;
				pos--;
			}
		}
		uint256 uint256::min()
		{
			static uint256 value = uint256(0);
			return value;
		}
		uint256 uint256::max()
		{
			static uint256 value = uint256(uint128((uint64_t)-1, (uint64_t)-1), uint128((uint64_t)-1, (uint64_t)-1));
			return value;
		}
		uint256::operator bool() const
		{
			return (bool)(upper || lower);
		}
		uint256::operator uint8_t() const
		{
			return (uint8_t)lower;
		}
		uint256::operator uint16_t() const
		{
			return (uint16_t)lower;
		}
		uint256::operator uint32_t() const
		{
			return (uint32_t)lower;
		}
		uint256::operator uint64_t() const
		{
			return (uint64_t)lower;
		}
		uint256::operator uint128() const
		{
			return lower;
		}
		uint256 uint256::operator&(const uint128& right) const
		{
			return uint256(uint128::min(), lower & right);
		}
		uint256 uint256::operator&(const uint256& right) const
		{
			return uint256(upper & right.upper, lower & right.lower);
		}
		uint256& uint256::operator&=(const uint128& right)
		{
			upper = uint128::min();
			lower &= right;
			return *this;
		}
		uint256& uint256::operator&=(const uint256& right)
		{
			upper &= right.upper;
			lower &= right.lower;
			return *this;
		}
		uint256 uint256::operator|(const uint128& right) const
		{
			return uint256(upper, lower | right);
		}
		uint256 uint256::operator|(const uint256& right) const
		{
			return uint256(upper | right.upper, lower | right.lower);
		}
		uint256& uint256::operator|=(const uint128& right)
		{
			lower |= right;
			return *this;
		}
		uint256& uint256::operator|=(const uint256& right)
		{
			upper |= right.upper;
			lower |= right.lower;
			return *this;
		}
		uint256 uint256::operator^(const uint128& right) const
		{
			return uint256(upper, lower ^ right);
		}
		uint256 uint256::operator^(const uint256& right) const
		{
			return uint256(upper ^ right.upper, lower ^ right.lower);
		}
		uint256& uint256::operator^=(const uint128& right)
		{
			lower ^= right;
			return *this;
		}
		uint256& uint256::operator^=(const uint256& right)
		{
			upper ^= right.upper;
			lower ^= right.lower;
			return *this;
		}
		uint256 uint256::operator~() const
		{
			return uint256(~upper, ~lower);
		}
		uint256 uint256::operator<<(const uint128& right) const
		{
			return *this << uint256(right);
		}
		uint256 uint256::operator<<(const uint256& right) const
		{
			const uint128 shift = right.lower;
			if (((bool)right.upper) || (shift >= uint128(256)))
				return min();
			else if (shift == uint128(128))
				return uint256(lower, uint128::min());
			else if (shift == uint128::min())
				return *this;
			else if (shift < uint128(128))
				return uint256((upper << shift) + (lower >> (uint128(128) - shift)), lower << shift);
			else if ((uint128(256) > shift) && (shift > uint128(128)))
				return uint256(lower << (shift - uint128(128)), uint128::min());

			return min();
		}
		uint256& uint256::operator<<=(const uint128& shift)
		{
			return *this <<= uint256(shift);
		}
		uint256& uint256::operator<<=(const uint256& shift)
		{
			*this = *this << shift;
			return *this;
		}
		uint256 uint256::operator>>(const uint128& right) const
		{
			return *this >> uint256(right);
		}
		uint256 uint256::operator>>(const uint256& right) const
		{
			const uint128 shift = right.lower;
			if (((bool)right.upper) || (shift >= uint128(128)))
				return min();
			else if (shift == uint128(128))
				return uint256(upper);
			else if (shift == uint128::min())
				return *this;
			else if (shift < uint128(128))
				return uint256(upper >> shift, (upper << (uint128(128) - shift)) + (lower >> shift));
			else if ((uint128(256) > shift) && (shift > uint128(128)))
				return uint256(upper >> (shift - uint128(128)));

			return min();
		}
		uint256& uint256::operator>>=(const uint128& shift)
		{
			return *this >>= uint256(shift);
		}
		uint256& uint256::operator>>=(const uint256& shift)
		{
			*this = *this >> shift;
			return *this;
		}
		bool uint256::operator!() const
		{
			return !(bool)*this;
		}
		bool uint256::operator&&(const uint128& right) const
		{
			return (*this && uint256(right));
		}
		bool uint256::operator&&(const uint256& right) const
		{
			return ((bool)*this && (bool)right);
		}
		bool uint256::operator||(const uint128& right) const
		{
			return (*this || uint256(right));
		}
		bool uint256::operator||(const uint256& right) const
		{
			return ((bool)*this || (bool)right);
		}
		bool uint256::operator==(const uint128& right) const
		{
			return (*this == uint256(right));
		}
		bool uint256::operator==(const uint256& right) const
		{
			return ((upper == right.upper) && (lower == right.lower));
		}
		bool uint256::operator!=(const uint128& right) const
		{
			return (*this != uint256(right));
		}
		bool uint256::operator!=(const uint256& right) const
		{
			return ((upper != right.upper) || (lower != right.lower));
		}
		bool uint256::operator>(const uint128& right) const
		{
			return (*this > uint256(right));
		}
		bool uint256::operator>(const uint256& right) const
		{
			if (upper == right.upper)
				return (lower > right.lower);
			if (upper > right.upper)
				return true;
			return false;
		}
		bool uint256::operator<(const uint128& right) const
		{
			return (*this < uint256(right));
		}
		bool uint256::operator<(const uint256& right) const
		{
			if (upper == right.upper)
				return (lower < right.lower);
			if (upper < right.upper)
				return true;
			return false;
		}
		bool uint256::operator>=(const uint128& right) const
		{
			return (*this >= uint256(right));
		}
		bool uint256::operator>=(const uint256& right) const
		{
			return ((*this > right) || (*this == right));
		}
		bool uint256::operator<=(const uint128& right) const
		{
			return (*this <= uint256(right));
		}
		bool uint256::operator<=(const uint256& right) const
		{
			return ((*this < right) || (*this == right));
		}
		uint256 uint256::operator+(const uint128& right) const
		{
			return *this + uint256(right);
		}
		uint256 uint256::operator+(const uint256& right) const
		{
			return uint256(upper + right.upper + (((lower + right.lower) < lower) ? uint128(1) : uint128::min()), lower + right.lower);
		}
		uint256& uint256::operator+=(const uint128& right)
		{
			return *this += uint256(right);
		}
		uint256& uint256::operator+=(const uint256& right)
		{
			upper = right.upper + upper + ((lower + right.lower) < lower);
			lower = lower + right.lower;
			return *this;
		}
		uint256 uint256::operator-(const uint128& right) const
		{
			return *this - uint256(right);
		}
		uint256 uint256::operator-(const uint256& right) const
		{
			return uint256(upper - right.upper - ((lower - right.lower) > lower), lower - right.lower);
		}
		uint256& uint256::operator-=(const uint128& right)
		{
			return *this -= uint256(right);
		}
		uint256& uint256::operator-=(const uint256& right)
		{
			*this = *this - right;
			return *this;
		}
		uint256 uint256::operator*(const uint128& right) const
		{
			return *this * uint256(right);
		}
		uint256 uint256::operator*(const uint256& right) const
		{
			uint128 top[4] = { upper.high(), upper.low(), lower.high(), lower.low() };
			uint128 bottom[4] = { right.high().high(), right.high().low(), right.low().high(), right.low().low() };
			uint128 products[4][4];

			for (int y = 3; y > -1; y--)
			{
				for (int x = 3; x > -1; x--)
					products[3 - y][x] = top[x] * bottom[y];
			}

			uint128 fourth64 = uint128(products[0][3].low());
			uint128 third64 = uint128(products[0][2].low()) + uint128(products[0][3].high());
			uint128 second64 = uint128(products[0][1].low()) + uint128(products[0][2].high());
			uint128 first64 = uint128(products[0][0].low()) + uint128(products[0][1].high());
			third64 += uint128(products[1][3].low());
			second64 += uint128(products[1][2].low()) + uint128(products[1][3].high());
			first64 += uint128(products[1][1].low()) + uint128(products[1][2].high());
			second64 += uint128(products[2][3].low());
			first64 += uint128(products[2][2].low()) + uint128(products[2][3].high());
			first64 += uint128(products[3][3].low());

			return uint256(first64 << uint128(64), uint128::min()) +
				uint256(third64.high(), third64 << uint128(64)) +
				uint256(second64, uint128::min()) +
				uint256(fourth64);
		}
		uint256& uint256::operator*=(const uint128& right)
		{
			return *this *= uint256(right);
		}
		uint256& uint256::operator*=(const uint256& right)
		{
			*this = *this * right;
			return *this;
		}
		std::pair<uint256, uint256> uint256::divide(const uint256& left, const uint256& right) const
		{
			if (right == min())
			{
				VI_ASSERT(false, " division or modulus by zero");
				return std::pair <uint256, uint256>(min(), min());
			}
			else if (right == uint256(1))
				return std::pair <uint256, uint256>(left, min());
			else if (left == right)
				return std::pair <uint256, uint256>(uint256(1), min());
			else if ((left == min()) || (left < right))
				return std::pair <uint256, uint256>(min(), left);

			std::pair <uint256, uint256> qr(min(), left);
			uint256 copyd = right << (left.bits() - right.bits());
			uint256 adder = uint256(1) << (left.bits() - right.bits());
			if (copyd > qr.second)
			{
				copyd >>= uint256(1);
				adder >>= uint256(1);
			}
			while (qr.second >= right)
			{
				if (qr.second >= copyd)
				{
					qr.second -= copyd;
					qr.first |= adder;
				}
				copyd >>= uint256(1);
				adder >>= uint256(1);
			}
			return qr;
		}
		uint256 uint256::operator/(const uint128& right) const
		{
			return *this / uint256(right);
		}
		uint256 uint256::operator/(const uint256& right) const
		{
			return divide(*this, right).first;
		}
		uint256& uint256::operator/=(const uint128& right)
		{
			return *this /= uint256(right);
		}
		uint256& uint256::operator/=(const uint256& right)
		{
			*this = *this / right;
			return *this;
		}
		uint256 uint256::operator%(const uint128& right) const
		{
			return *this % uint256(right);
		}
		uint256 uint256::operator%(const uint256& right) const
		{
			return *this - (right * (*this / right));
		}
		uint256& uint256::operator%=(const uint128& right)
		{
			return *this %= uint256(right);
		}
		uint256& uint256::operator%=(const uint256& right)
		{
			*this = *this % right;
			return *this;
		}
		uint256& uint256::operator++()
		{
			*this += uint256(1);
			return *this;
		}
		uint256 uint256::operator++(int)
		{
			uint256 temp(*this);
			++*this;
			return temp;
		}
		uint256& uint256::operator--()
		{
			*this -= uint256(1);
			return *this;
		}
		uint256 uint256::operator--(int)
		{
			uint256 temp(*this);
			--*this;
			return temp;
		}
		uint256 uint256::operator+() const
		{
			return *this;
		}
		uint256 uint256::operator-() const
		{
			return ~*this + uint256(1);
		}
		const uint128& uint256::high() const
		{
			return upper;
		}
		const uint128& uint256::low() const
		{
			return lower;
		}
		uint128& uint256::high()
		{
			return upper;
		}
		uint128& uint256::low()
		{
			return lower;
		}
		void uint256::encode(uint8_t data[16]) const
		{
			VI_ASSERT(data != nullptr, "data should be set");
			uint64_t array[4] =
			{
#if VI_ENDIAN_BIG
				core::os::hw::to_endianness(core::os::hw::endian::big, lower.lower),
				core::os::hw::to_endianness(core::os::hw::endian::big, lower.upper),
				core::os::hw::to_endianness(core::os::hw::endian::big, upper.lower),
				core::os::hw::to_endianness(core::os::hw::endian::big, upper.upper)
#else
				core::os::hw::to_endianness(core::os::hw::endian::big, upper.upper),
				core::os::hw::to_endianness(core::os::hw::endian::big, upper.lower),
				core::os::hw::to_endianness(core::os::hw::endian::big, lower.upper),
				core::os::hw::to_endianness(core::os::hw::endian::big, lower.lower)
#endif
			};
			memcpy((char*)data, array, sizeof(array));
		}
		void uint256::encode_compact(uint8_t data[16], size_t* data_size) const
		{
			VI_ASSERT(data != nullptr, "data should be set");
			uint8_t intermediate[sizeof(uint256)];
			encode(intermediate);

			size_t size = bytes();
			memcpy(data, intermediate + (sizeof(uint256) - size), size);
			if (data_size != nullptr)
				*data_size = size;
		}
		void uint256::decode(const uint8_t data[16])
		{
			VI_ASSERT(data != nullptr, "value should be set");
			uint64_t array[4] = { 0 };
			memcpy(array, data, sizeof(array));
			array[0] = core::os::hw::to_endianness(core::os::hw::endian::big, array[0]);
			array[1] = core::os::hw::to_endianness(core::os::hw::endian::big, array[1]);
			array[2] = core::os::hw::to_endianness(core::os::hw::endian::big, array[2]);
			array[3] = core::os::hw::to_endianness(core::os::hw::endian::big, array[3]);
#if VI_ENDIAN_BIG
			memcpy((uint64_t*)&lower.lower, &array[0], sizeof(uint64_t));
			memcpy((uint64_t*)&lower.upper, &array[1], sizeof(uint64_t));
			memcpy((uint64_t*)&upper.lower, &array[2], sizeof(uint64_t));
			memcpy((uint64_t*)&upper.upper, &array[3], sizeof(uint64_t));
#else
			memcpy((uint64_t*)&upper.upper, &array[0], sizeof(uint64_t));
			memcpy((uint64_t*)&upper.lower, &array[1], sizeof(uint64_t));
			memcpy((uint64_t*)&lower.upper, &array[2], sizeof(uint64_t));
			memcpy((uint64_t*)&lower.lower, &array[3], sizeof(uint64_t));
#endif
		}
		void uint256::decode_compact(const uint8_t* data, size_t data_size)
		{
			VI_ASSERT(data != nullptr, "data should be set");
			uint8_t intermediate[sizeof(uint256)] = { 0 };
			memcpy(intermediate, data, std::min(data_size, sizeof(intermediate)));
			decode(intermediate);
		}
		uint16_t uint256::bits() const
		{
			uint16_t out = 0;
			if (upper)
			{
				out = 128;
				uint128 up = upper;
				while (up)
				{
					up >>= uint128(1);
					out++;
				}
			}
			else
			{
				uint128 low = lower;
				while (low)
				{
					low >>= uint128(1);
					out++;
				}
			}
			return out;
		}
		uint16_t uint256::bytes() const
		{
			if (!*this)
				return 0;

			uint16_t length = bits();
			if (!length--)
				return 0;

			return std::max<uint16_t>(1, std::min(32, (length - length % 8 + 8) / 8));
		}
		core::decimal uint256::to_decimal() const
		{
			return core::decimal::from(to_string(16), 16);
		}
		core::string uint256::to_string(uint8_t base, uint32_t length) const
		{
			VI_ASSERT(base >= 2 && base <= 36, "base must be in the range [2, 36]");
			static const char* alphabet = "0123456789abcdefghijklmnopqrstuvwxyz";
			core::string output;
			if (!(*this))
			{
				if (!length)
					output.push_back('0');
				else if (output.size() < length)
					output.append(length - output.size(), '0');
				return output;
			}

			switch (base)
			{
				case 16:
				{
					uint64_t array[4]; size_t size = bytes();
					if (size > sizeof(uint64_t) * 0)
					{
						array[3] = core::os::hw::to_endianness(core::os::hw::endian::big, lower.low());
						if (size > sizeof(uint64_t) * 1)
						{
							array[2] = core::os::hw::to_endianness(core::os::hw::endian::big, lower.high());
							if (size > sizeof(uint64_t) * 2)
							{
								array[1] = core::os::hw::to_endianness(core::os::hw::endian::big, upper.low());
								if (size > sizeof(uint64_t) * 3)
									array[0] = core::os::hw::to_endianness(core::os::hw::endian::big, upper.high());
							}
						}
					}

					length = std::max(length, (uint32_t)size * 2);
					output = codec::hex_encode(std::string_view(((char*)array) + (sizeof(array) - size), size));
					if (output.size() < length)
						output.append(length - output.size(), '0');
					while (output.size() > 1 && output.front() == '0')
						output.erase(0, 1);
					break;
				}
				default:
				{
					std::pair<uint256, uint256> remainder(*this, min());
					do
					{
						remainder = divide(remainder.first, base);
						output.push_back(alphabet[(uint8_t)remainder.second]);
					} while (remainder.first);
					if (output.size() < length)
						output.append(length - output.size(), '0');

					std::reverse(output.begin(), output.end());
					break;
				}
			}
			return output;
		}
		uint256 operator&(const uint128& left, const uint256& right)
		{
			return right & left;
		}
		uint128& operator&=(uint128& left, const uint256& right)
		{
			left = (right & left).low();
			return left;
		}
		uint256 operator|(const uint128& left, const uint256& right)
		{
			return right | left;
		}
		uint128& operator|=(uint128& left, const uint256& right)
		{
			left = (right | left).low();
			return left;
		}
		uint256 operator^(const uint128& left, const uint256& right)
		{
			return right ^ left;
		}
		uint128& operator^=(uint128& left, const uint256& right)
		{
			left = (right ^ left).low();
			return left;
		}
		uint256 operator<<(const uint8_t& left, const uint256& right)
		{
			return uint256(left) << right;
		}
		uint256 operator<<(const uint16_t& left, const uint256& right)
		{
			return uint256(left) << right;
		}
		uint256 operator<<(const uint32_t& left, const uint256& right)
		{
			return uint256(left) << right;
		}
		uint256 operator<<(const uint64_t& left, const uint256& right)
		{
			return uint256(left) << right;
		}
		uint256 operator<<(const uint128& left, const uint256& right)
		{
			return uint256(left) << right;
		}
		uint256 operator<<(const int8_t& left, const uint256& right)
		{
			return uint256(left) << right;
		}
		uint256 operator<<(const int16_t& left, const uint256& right)
		{
			return uint256(left) << right;
		}
		uint256 operator<<(const int32_t& left, const uint256& right)
		{
			return uint256(left) << right;
		}
		uint256 operator<<(const int64_t& left, const uint256& right)
		{
			return uint256(left) << right;
		}
		uint128& operator<<=(uint128& left, const uint256& right)
		{
			left = (uint256(left) << right).low();
			return left;
		}
		uint256 operator>>(const uint8_t& left, const uint256& right)
		{
			return uint256(left) >> right;
		}
		uint256 operator>>(const uint16_t& left, const uint256& right)
		{
			return uint256(left) >> right;
		}
		uint256 operator>>(const uint32_t& left, const uint256& right)
		{
			return uint256(left) >> right;
		}
		uint256 operator>>(const uint64_t& left, const uint256& right)
		{
			return uint256(left) >> right;
		}
		uint256 operator>>(const uint128& left, const uint256& right)
		{
			return uint256(left) >> right;
		}
		uint256 operator>>(const int8_t& left, const uint256& right)
		{
			return uint256(left) >> right;
		}
		uint256 operator>>(const int16_t& left, const uint256& right)
		{
			return uint256(left) >> right;
		}
		uint256 operator>>(const int32_t& left, const uint256& right)
		{
			return uint256(left) >> right;
		}
		uint256 operator>>(const int64_t& left, const uint256& right)
		{
			return uint256(left) >> right;
		}
		uint128& operator>>=(uint128& left, const uint256& right)
		{
			left = (uint256(left) >> right).low();
			return left;
		}
		bool operator==(const uint128& left, const uint256& right)
		{
			return right == left;
		}
		bool operator!=(const uint128& left, const uint256& right)
		{
			return right != left;
		}
		bool operator>(const uint128& left, const uint256& right)
		{
			return right < left;
		}
		bool operator<(const uint128& left, const uint256& right)
		{
			return right > left;
		}
		bool operator>=(const uint128& left, const uint256& right)
		{
			return right <= left;
		}
		bool operator<=(const uint128& left, const uint256& right)
		{
			return right >= left;
		}
		uint256 operator+(const uint128& left, const uint256& right)
		{
			return right + left;
		}
		uint128& operator+=(uint128& left, const uint256& right)
		{
			left = (right + left).low();
			return left;
		}
		uint256 operator-(const uint128& left, const uint256& right)
		{
			return -(right - left);
		}
		uint128& operator-=(uint128& left, const uint256& right)
		{
			left = (-(right - left)).low();
			return left;
		}
		uint256 operator*(const uint128& left, const uint256& right)
		{
			return right * left;
		}
		uint128& operator*=(uint128& left, const uint256& right)
		{
			left = (right * left).low();
			return left;
		}
		uint256 operator/(const uint128& left, const uint256& right)
		{
			return uint256(left) / right;
		}
		uint128& operator/=(uint128& left, const uint256& right)
		{
			left = (uint256(left) / right).low();
			return left;
		}
		uint256 operator%(const uint128& left, const uint256& right)
		{
			return uint256(left) % right;
		}
		uint128& operator%=(uint128& left, const uint256& right)
		{
			left = (uint256(left) % right).low();
			return left;
		}
		std::ostream& operator<<(std::ostream& stream, const uint256& right)
		{
			if (stream.flags() & stream.oct)
				stream << right.to_string(8);
			else if (stream.flags() & stream.dec)
				stream << right.to_string(10);
			else if (stream.flags() & stream.hex)
				stream << right.to_string(16);
			return stream;
		}
		preprocessor_exception::preprocessor_exception(preprocessor_error new_type) : preprocessor_exception(new_type, 0)
		{
		}
		preprocessor_exception::preprocessor_exception(preprocessor_error new_type, size_t new_offset) : preprocessor_exception(new_type, new_offset, "")
		{
		}
		preprocessor_exception::preprocessor_exception(preprocessor_error new_type, size_t new_offset, const std::string_view& new_message) : error_type(new_type), error_offset(new_offset)
		{
			switch (error_type)
			{
				case preprocessor_error::macro_definition_empty:
					error_message = "empty macro directive definition";
					break;
				case preprocessor_error::macro_name_empty:
					error_message = "empty macro directive name";
					break;
				case preprocessor_error::macro_parenthesis_double_closed:
					error_message = "macro directive parenthesis is closed twice";
					break;
				case preprocessor_error::macro_parenthesis_not_closed:
					error_message = "macro directive parenthesis is not closed";
					break;
				case preprocessor_error::macro_definition_error:
					error_message = "macro directive definition parsing error";
					break;
				case preprocessor_error::macro_expansion_parenthesis_double_closed:
					error_message = "macro expansion directive parenthesis is closed twice";
					break;
				case preprocessor_error::macro_expansion_parenthesis_not_closed:
					error_message = "macro expansion directive parenthesis is not closed";
					break;
				case preprocessor_error::macro_expansion_arguments_error:
					error_message = "macro expansion directive uses incorrect number of arguments";
					break;
				case preprocessor_error::macro_expansion_execution_error:
					error_message = "macro expansion directive execution error";
					break;
				case preprocessor_error::macro_expansion_error:
					error_message = "macro expansion directive parsing error";
					break;
				case preprocessor_error::condition_not_opened:
					error_message = "conditional directive has no opened parenthesis";
					break;
				case preprocessor_error::condition_not_closed:
					error_message = "conditional directive parenthesis is not closed";
					break;
				case preprocessor_error::condition_error:
					error_message = "conditional directive parsing error";
					break;
				case preprocessor_error::directive_not_found:
					error_message = "directive is not found";
					break;
				case preprocessor_error::directive_expansion_error:
					error_message = "directive expansion error";
					break;
				case preprocessor_error::include_denied:
					error_message = "inclusion directive is denied";
					break;
				case preprocessor_error::include_error:
					error_message = "inclusion directive parsing error";
					break;
				case preprocessor_error::include_not_found:
					error_message = "inclusion directive is not found";
					break;
				case preprocessor_error::pragma_not_found:
					error_message = "pragma directive is not found";
					break;
				case preprocessor_error::pragma_error:
					error_message = "pragma directive parsing error";
					break;
				case preprocessor_error::extension_error:
					error_message = "extension error";
					break;
				default:
					break;
			}

			if (error_offset > 0)
			{
				error_message += " at offset ";
				error_message += core::to_string(error_offset);
			}

			if (!new_message.empty())
			{
				error_message += " on ";
				error_message += new_message;
			}
		}
		const char* preprocessor_exception::type() const noexcept
		{
			return "preprocessor_error";
		}
		preprocessor_error preprocessor_exception::status() const noexcept
		{
			return error_type;
		}
		size_t preprocessor_exception::offset() const noexcept
		{
			return error_offset;
		}

		crypto_exception::crypto_exception() : error_code(0)
		{
#ifdef VI_OPENSSL
			error_code = (size_t)ERR_get_error();
			ERR_print_errors_cb([](const char* message, size_t size, void* exception_ptr)
			{
				auto* exception = (crypto_exception*)exception_ptr;
				exception->error_message.append(message, size).append(1, '\n');
				return 0;
			}, this);
			if (!error_message.empty())
				core::stringify::trim(error_message);
			else
				error_message = "error:internal";
#else
			error_message = "error:unsupported";
#endif
		}
		crypto_exception::crypto_exception(size_t new_error_code, const std::string_view& new_message) : error_code(new_error_code)
		{
			error_message = "error:" + core::to_string(error_code) + ":";
			if (!new_message.empty())
				error_message += new_message;
			else
				error_message += "internal";
		}
		const char* crypto_exception::type() const noexcept
		{
			return "crypto_error";
		}
		size_t crypto_exception::code() const noexcept
		{
			return error_code;
		}

		compression_exception::compression_exception(int new_error_code, const std::string_view& new_message) : error_code(new_error_code)
		{
			if (!new_message.empty())
				error_message += new_message;
			else
				error_message += "internal error";
			error_message += " (error = " + core::to_string(error_code) + ")";
		}
		const char* compression_exception::type() const noexcept
		{
			return "compression_error";
		}
		int compression_exception::code() const noexcept
		{
			return error_code;
		}

		md5_hasher::md5_hasher() noexcept
		{
			VI_TRACE("crypto create md5 hasher");
			memset(buffer, 0, sizeof(buffer));
			memset(digest, 0, sizeof(digest));
			finalized = false;
			count[0] = 0;
			count[1] = 0;
			state[0] = 0x67452301;
			state[1] = 0xefcdab89;
			state[2] = 0x98badcfe;
			state[3] = 0x10325476;
		}
		void md5_hasher::decode(uint4* output, const uint1* input, uint32_t length)
		{
			VI_ASSERT(output != nullptr && input != nullptr, "output and input should be set");
			for (uint32_t i = 0, j = 0; j < length; i++, j += 4)
				output[i] = ((uint4)input[j]) | (((uint4)input[j + 1]) << 8) | (((uint4)input[j + 2]) << 16) | (((uint4)input[j + 3]) << 24);
			VI_TRACE("crypto md5 hasher decode to 0x%" PRIXPTR, (void*)output);
		}
		void md5_hasher::encode(uint1* output, const uint4* input, uint32_t length)
		{
			VI_ASSERT(output != nullptr && input != nullptr, "output and input should be set");
			for (uint32_t i = 0, j = 0; j < length; i++, j += 4)
			{
				output[j] = input[i] & 0xff;
				output[j + 1] = (input[i] >> 8) & 0xff;
				output[j + 2] = (input[i] >> 16) & 0xff;
				output[j + 3] = (input[i] >> 24) & 0xff;
			}
			VI_TRACE("crypto md5 hasher encode to 0x%" PRIXPTR, (void*)output);
		}
		void md5_hasher::transform(const uint1* block, uint32_t length)
		{
			VI_ASSERT(block != nullptr, "block should be set");
			VI_TRACE("crypto md5 hasher transform from 0x%" PRIXPTR, (void*)block);
			uint4 a = state[0], b = state[1], c = state[2], d = state[3], x[16];
			decode(x, block, length);

			ff(a, b, c, d, x[0], s11, 0xd76aa478);
			ff(d, a, b, c, x[1], s12, 0xe8c7b756);
			ff(c, d, a, b, x[2], s13, 0x242070db);
			ff(b, c, d, a, x[3], s14, 0xc1bdceee);
			ff(a, b, c, d, x[4], s11, 0xf57c0faf);
			ff(d, a, b, c, x[5], s12, 0x4787c62a);
			ff(c, d, a, b, x[6], s13, 0xa8304613);
			ff(b, c, d, a, x[7], s14, 0xfd469501);
			ff(a, b, c, d, x[8], s11, 0x698098d8);
			ff(d, a, b, c, x[9], s12, 0x8b44f7af);
			ff(c, d, a, b, x[10], s13, 0xffff5bb1);
			ff(b, c, d, a, x[11], s14, 0x895cd7be);
			ff(a, b, c, d, x[12], s11, 0x6b901122);
			ff(d, a, b, c, x[13], s12, 0xfd987193);
			ff(c, d, a, b, x[14], s13, 0xa679438e);
			ff(b, c, d, a, x[15], s14, 0x49b40821);
			gg(a, b, c, d, x[1], s21, 0xf61e2562);
			gg(d, a, b, c, x[6], s22, 0xc040b340);
			gg(c, d, a, b, x[11], s23, 0x265e5a51);
			gg(b, c, d, a, x[0], s24, 0xe9b6c7aa);
			gg(a, b, c, d, x[5], s21, 0xd62f105d);
			gg(d, a, b, c, x[10], s22, 0x2441453);
			gg(c, d, a, b, x[15], s23, 0xd8a1e681);
			gg(b, c, d, a, x[4], s24, 0xe7d3fbc8);
			gg(a, b, c, d, x[9], s21, 0x21e1cde6);
			gg(d, a, b, c, x[14], s22, 0xc33707d6);
			gg(c, d, a, b, x[3], s23, 0xf4d50d87);
			gg(b, c, d, a, x[8], s24, 0x455a14ed);
			gg(a, b, c, d, x[13], s21, 0xa9e3e905);
			gg(d, a, b, c, x[2], s22, 0xfcefa3f8);
			gg(c, d, a, b, x[7], s23, 0x676f02d9);
			gg(b, c, d, a, x[12], s24, 0x8d2a4c8a);
			hh(a, b, c, d, x[5], s31, 0xfffa3942);
			hh(d, a, b, c, x[8], s32, 0x8771f681);
			hh(c, d, a, b, x[11], s33, 0x6d9d6122);
			hh(b, c, d, a, x[14], s34, 0xfde5380c);
			hh(a, b, c, d, x[1], s31, 0xa4beea44);
			hh(d, a, b, c, x[4], s32, 0x4bdecfa9);
			hh(c, d, a, b, x[7], s33, 0xf6bb4b60);
			hh(b, c, d, a, x[10], s34, 0xbebfbc70);
			hh(a, b, c, d, x[13], s31, 0x289b7ec6);
			hh(d, a, b, c, x[0], s32, 0xeaa127fa);
			hh(c, d, a, b, x[3], s33, 0xd4ef3085);
			hh(b, c, d, a, x[6], s34, 0x4881d05);
			hh(a, b, c, d, x[9], s31, 0xd9d4d039);
			hh(d, a, b, c, x[12], s32, 0xe6db99e5);
			hh(c, d, a, b, x[15], s33, 0x1fa27cf8);
			hh(b, c, d, a, x[2], s34, 0xc4ac5665);
			ii(a, b, c, d, x[0], s41, 0xf4292244);
			ii(d, a, b, c, x[7], s42, 0x432aff97);
			ii(c, d, a, b, x[14], s43, 0xab9423a7);
			ii(b, c, d, a, x[5], s44, 0xfc93a039);
			ii(a, b, c, d, x[12], s41, 0x655b59c3);
			ii(d, a, b, c, x[3], s42, 0x8f0ccc92);
			ii(c, d, a, b, x[10], s43, 0xffeff47d);
			ii(b, c, d, a, x[1], s44, 0x85845dd1);
			ii(a, b, c, d, x[8], s41, 0x6fa87e4f);
			ii(d, a, b, c, x[15], s42, 0xfe2ce6e0);
			ii(c, d, a, b, x[6], s43, 0xa3014314);
			ii(b, c, d, a, x[13], s44, 0x4e0811a1);
			ii(a, b, c, d, x[4], s41, 0xf7537e82);
			ii(d, a, b, c, x[11], s42, 0xbd3af235);
			ii(c, d, a, b, x[2], s43, 0x2ad7d2bb);
			ii(b, c, d, a, x[9], s44, 0xeb86d391);

			state[0] += a;
			state[1] += b;
			state[2] += c;
			state[3] += d;
#ifdef VI_MICROSOFT
			RtlSecureZeroMemory(x, sizeof(x));
#else
			memset(x, 0, sizeof(x));
#endif
		}
		void md5_hasher::update(const std::string_view& input, uint32_t block_size)
		{
			update(input.data(), (uint32_t)input.size(), block_size);
		}
		void md5_hasher::update(const uint8_t* input, uint32_t length, uint32_t block_size)
		{
			VI_ASSERT(input != nullptr, "input should be set");
			VI_TRACE("crypto md5 hasher update from 0x%" PRIXPTR, (void*)input);
			uint32_t index = count[0] / 8 % block_size;
			count[0] += (length << 3);
			if (count[0] < length << 3)
				count[1]++;

			count[1] += (length >> 29);
			uint32_t chunk = 64 - index, i = 0;
			if (length >= chunk)
			{
				memcpy(&buffer[index], input, chunk);
				transform(buffer, block_size);

				for (i = chunk; i + block_size <= length; i += block_size)
					transform(&input[i]);
				index = 0;
			}
			else
				i = 0;

			memcpy(&buffer[index], &input[i], length - i);
		}
		void md5_hasher::update(const char* input, uint32_t length, uint32_t block_size)
		{
			update((const uint8_t*)input, length, block_size);
		}
		void md5_hasher::finalize()
		{
			VI_TRACE("crypto md5 hasher finalize at 0x%" PRIXPTR, (void*)this);
			static uint8_t padding[64] = { 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
			if (finalized)
				return;

			uint8_t bits[8];
			encode(bits, count, 8);

			uint32_t index = count[0] / 8 % 64;
			uint32_t size = (index < 56) ? (56 - index) : (120 - index);
			update(padding, size);
			update(bits, 8);
			encode(digest, state, 16);

			memset(buffer, 0, sizeof(buffer));
			memset(count, 0, sizeof(count));
			finalized = true;
		}
		char* md5_hasher::hex_digest() const
		{
			VI_ASSERT(finalized, "md5 hash should be finalized");
			char* data = core::memory::allocate<char>(sizeof(char) * 48);
			memset(data, 0, sizeof(char) * 48);

			for (size_t i = 0; i < 16; i++)
				snprintf(data + i * 2, 4, "%02x", (uint32_t)digest[i]);

			return data;
		}
		uint8_t* md5_hasher::raw_digest() const
		{
			VI_ASSERT(finalized, "md5 hash should be finalized");
			uint1* output = core::memory::allocate<uint1>(sizeof(uint1) * 17);
			memcpy(output, digest, 16);
			output[16] = '\0';

			return output;
		}
		core::string md5_hasher::to_hex() const
		{
			VI_ASSERT(finalized, "md5 hash should be finalized");
			char data[48];
			memset(data, 0, sizeof(data));

			for (size_t i = 0; i < 16; i++)
				snprintf(data + i * 2, 4, "%02x", (uint32_t)digest[i]);

			return data;
		}
		core::string md5_hasher::to_raw() const
		{
			VI_ASSERT(finalized, "md5 hash should be finalized");
			uint1 data[17];
			memcpy(data, digest, 16);
			data[16] = '\0';

			return (const char*)data;
		}
		md5_hasher::uint4 md5_hasher::f(uint4 x, uint4 y, uint4 z)
		{
			return (x & y) | (~x & z);
		}
		md5_hasher::uint4 md5_hasher::g(uint4 x, uint4 y, uint4 z)
		{
			return (x & z) | (y & ~z);
		}
		md5_hasher::uint4 md5_hasher::h(uint4 x, uint4 y, uint4 z)
		{
			return x ^ y ^ z;
		}
		md5_hasher::uint4 md5_hasher::i(uint4 x, uint4 y, uint4 z)
		{
			return y ^ (x | ~z);
		}
		md5_hasher::uint4 md5_hasher::l(uint4 x, int n)
		{
			return (x << n) | (x >> (32 - n));
		}
		void md5_hasher::ff(uint4& a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac)
		{
			a = l(a + f(b, c, d) + x + ac, s) + b;
		}
		void md5_hasher::gg(uint4& a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac)
		{
			a = l(a + g(b, c, d) + x + ac, s) + b;
		}
		void md5_hasher::hh(uint4& a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac)
		{
			a = l(a + h(b, c, d) + x + ac, s) + b;
		}
		void md5_hasher::ii(uint4& a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac)
		{
			a = l(a + i(b, c, d) + x + ac, s) + b;
		}

		cipher ciphers::des_ecb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (cipher)EVP_des_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::des_ede()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (cipher)EVP_des_ede();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::des_ede3()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (cipher)EVP_des_ede3();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::des_ede_ecb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (cipher)EVP_des_ede_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::des_ede3_ecb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (cipher)EVP_des_ede3_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::des_cfb64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (cipher)EVP_des_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::des_cfb1()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (cipher)EVP_des_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::des_cfb8()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (cipher)EVP_des_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::des_ede_cfb64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (cipher)EVP_des_ede_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::des_ede3_cfb64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (cipher)EVP_des_ede3_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::des_ede3_cfb1()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (cipher)EVP_des_ede3_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::des_ede3_cfb8()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (cipher)EVP_des_ede3_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::des_ofb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (cipher)EVP_des_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::des_ede_ofb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (cipher)EVP_des_ede_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::des_ede3_ofb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (cipher)EVP_des_ede3_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::des_cbc()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (cipher)EVP_des_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::des_ede_cbc()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (cipher)EVP_des_ede_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::des_ede3_cbc()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (cipher)EVP_des_ede3_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::desede3_wrap()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_DES
			return (cipher)EVP_des_ede3_wrap();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::desx_cbc()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_DES
			return (cipher)EVP_desx_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::rc4()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC4
			return (cipher)EVP_rc4();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::rc4_40()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC4
			return (cipher)EVP_rc4_40();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::rc4_hmac_md5()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC4
#ifndef OPENSSL_NO_MD5
			return (cipher)EVP_rc4_hmac_md5();
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
		cipher ciphers::idea_ecb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_IDEA
			return (cipher)EVP_idea_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::idea_cfb64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_IDEA
			return (cipher)EVP_idea_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::idea_ofb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_IDEA
			return (cipher)EVP_idea_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::idea_cbc()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_IDEA
			return (cipher)EVP_idea_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::rc2_ecb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC2
			return (cipher)EVP_rc2_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::rc2_cbc()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC2
			return (cipher)EVP_rc2_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::rc2_40_cbc()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC2
			return (cipher)EVP_rc2_40_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::rc2_64_cbc()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC2
			return (cipher)EVP_rc2_64_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::rc2_cfb64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC2
			return (cipher)EVP_rc2_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::rc2_ofb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC2
			return (cipher)EVP_rc2_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::bf_ecb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_BF
			return (cipher)EVP_bf_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::bf_cbc()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_BF
			return (cipher)EVP_bf_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::bf_cfb64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_BF
			return (cipher)EVP_bf_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::bf_ofb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_BF
			return (cipher)EVP_bf_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::cast5_ecb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAST
			return (cipher)EVP_cast5_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::cast5_cbc()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAST
			return (cipher)EVP_cast5_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::cast5_cfb64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAST
			return (cipher)EVP_cast5_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::cast5_ofb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAST
			return (cipher)EVP_cast5_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::rc5_32_12_16_cbc()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC5
			return (cipher)EVP_rc5_32_12_16_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::rc5_32_12_16_ecb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC5
			return (cipher)EVP_rc5_32_12_16_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::rc5_32_12_16_cfb64()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC5
			return (cipher)EVP_rc5_32_12_16_cfb64();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::rc5_32_12_16_ofb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RC5
			return (cipher)EVP_rc5_32_12_16_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_128_ecb()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_128_ecb();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_128_cbc()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_128_cbc();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_128_cfb1()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_128_cfb1();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_128_cfb8()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_128_cfb8();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_128_cfb128()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_128_cfb128();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_128_ofb()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_128_ofb();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_128_ctr()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_128_ctr();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_128_ccm()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_128_ccm();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_128_gcm()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_128_gcm();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_128_xts()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_128_xts();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes128_wrap()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (cipher)EVP_aes_128_wrap();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes128_wrap_pad()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (cipher)EVP_aes_128_wrap_pad();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_128_ocb()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_OCB
			return (cipher)EVP_aes_128_ocb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_192_ecb()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_192_ecb();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_192_cbc()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_192_cbc();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_192_cfb1()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_192_cfb1();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_192_cfb8()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_192_cfb8();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_192_cfb128()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_192_cfb128();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_192_ofb()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_192_ofb();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_192_ctr()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_192_ctr();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_192_ccm()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_192_ccm();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_192_gcm()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_192_gcm();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes192_wrap()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (cipher)EVP_aes_192_wrap();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes192_wrap_pad()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (cipher)EVP_aes_192_wrap_pad();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_192_ocb()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_OCB
			return (cipher)EVP_aes_192_ocb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_256_ecb()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_256_ecb();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_256_cbc()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_256_cbc();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_256_cfb1()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_256_cfb1();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_256_cfb8()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_256_cfb8();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_256_cfb128()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_256_cfb128();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_256_ofb()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_256_ofb();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_256_ctr()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_256_ctr();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_256_ccm()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_256_ccm();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_256_gcm()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_256_gcm();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_256_xts()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_256_xts();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes256_wrap()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (cipher)EVP_aes_256_wrap();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes256_wrap_pad()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (cipher)EVP_aes_256_wrap_pad();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_256_ocb()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_OCB
			return (cipher)EVP_aes_256_ocb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_128_cbc_hmac_sha1()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_128_cbc_hmac_sha1();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_256_cbc_hmac_sha1()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_aes_256_cbc_hmac_sha1();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_128_cbc_hmac_sha256()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (cipher)EVP_aes_128_cbc_hmac_sha256();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aes_256_cbc_hmac_sha256()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (cipher)EVP_aes_256_cbc_hmac_sha256();
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_128_ecb()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_128_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_128_cbc()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_128_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_128_cfb1()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_128_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_128_cfb8()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_128_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_128_cfb128()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_128_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_128_ctr()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_128_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_128_ofb()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_128_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_128_gcm()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_128_gcm();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_128_ccm()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_128_ccm();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_192_ecb()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_192_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_192_cbc()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_192_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_192_cfb1()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_192_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_192_cfb8()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_192_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_192_cfb128()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_192_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_192_ctr()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_192_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_192_ofb()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_192_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_192_gcm()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_192_gcm();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_192_ccm()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_192_ccm();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_256_ecb()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_256_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_256_cbc()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_256_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_256_cfb1()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_256_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_256_cfb8()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_256_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_256_cfb128()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_256_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_256_ctr()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_256_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_256_ofb()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_256_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_256_gcm()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_256_gcm();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::aria_256_ccm()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_ARIA
			return (cipher)EVP_aria_256_ccm();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia128_ecb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_128_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia128_cbc()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_128_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia128_cfb1()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_128_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia128_cfb8()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_128_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia128_cfb128()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_128_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia128_ofb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_128_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia128_ctr()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_128_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia192_ecb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_192_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia192_cbc()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_192_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia192_cfb1()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_192_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia192_cfb8()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_192_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia192_cfb128()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_192_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia192_ofb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_192_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia192_ctr()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_192_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia256_ecb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_256_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia256_cbc()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_256_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia256_cfb1()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_256_cfb1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia256_cfb8()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_256_cfb8();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia256_cfb128()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_256_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia256_ofb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_256_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::camellia256_ctr()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_CAMELLIA
			return (cipher)EVP_camellia_256_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::chacha20()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_CHACHA
			return (cipher)EVP_chacha20();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::chacha20_poly1305()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_CHACHA
#ifndef OPENSSL_NO_POLY1305
			return (cipher)EVP_chacha20_poly1305();
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
		cipher ciphers::seed_ecb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_SEED
			return (cipher)EVP_seed_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::seed_cbc()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_SEED
			return (cipher)EVP_seed_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::seed_cfb128()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_SEED
			return (cipher)EVP_seed_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::seed_ofb()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_SEED
			return (cipher)EVP_seed_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::sm4_ecb()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_SM4
			return (cipher)EVP_sm4_ecb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::sm4_cbc()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_SM4
			return (cipher)EVP_sm4_cbc();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::sm4_cfb128()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_SM4
			return (cipher)EVP_sm4_cfb128();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::sm4_ofb()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_SM4
			return (cipher)EVP_sm4_ofb();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		cipher ciphers::sm4_ctr()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_SM4
			return (cipher)EVP_sm4_ctr();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}

		digest digests::md2()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_MD2
			return (cipher)EVP_md2();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		digest digests::md4()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_MD4
			return (cipher)EVP_md4();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		digest digests::md5()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_MD5
			return (cipher)EVP_md5();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		digest digests::md5_sha1()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_MD5
			return (cipher)EVP_md5_sha1();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		digest digests::blake2_b512()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_BLAKE2
			return (cipher)EVP_blake2b512();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		digest digests::blake2_s256()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_BLAKE2
			return (cipher)EVP_blake2s256();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		digest digests::sha1()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_sha1();
#else
			return nullptr;
#endif
		}
		digest digests::sha224()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_sha224();
#else
			return nullptr;
#endif
		}
		digest digests::sha256()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_sha256();
#else
			return nullptr;
#endif
		}
		digest digests::sha384()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_sha384();
#else
			return nullptr;
#endif
		}
		digest digests::sha512()
		{
#ifdef VI_OPENSSL
			return (cipher)EVP_sha512();
#else
			return nullptr;
#endif
		}
		digest digests::sha512_224()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (cipher)EVP_sha512_224();
#else
			return nullptr;
#endif
		}
		digest digests::sha512_256()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (cipher)EVP_sha512_256();
#else
			return nullptr;
#endif
		}
		digest digests::sha3_224()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (cipher)EVP_sha3_224();
#else
			return nullptr;
#endif
		}
		digest digests::sha3_256()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (cipher)EVP_sha3_256();
#else
			return nullptr;
#endif
		}
		digest digests::sha3_384()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (cipher)EVP_sha3_384();
#else
			return nullptr;
#endif
		}
		digest digests::sha3_512()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (cipher)EVP_sha3_512();
#else
			return nullptr;
#endif
		}
		digest digests::shake128()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (cipher)EVP_shake128();
#else
			return nullptr;
#endif
		}
		digest digests::shake256()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
			return (cipher)EVP_shake256();
#else
			return nullptr;
#endif
		}
		digest digests::mdc2()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_MDC2
			return (cipher)EVP_mdc2();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		digest digests::ripe_md160()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_RMD160
			return (cipher)EVP_ripemd160();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		digest digests::whirlpool()
		{
#ifdef VI_OPENSSL
#ifndef OPENSSL_NO_WHIRLPOOL
			return (cipher)EVP_whirlpool();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}
		digest digests::sm3()
		{
#if VI_OPENSSL && OPENSSL_VERSION_NUMBER >= 0x1010000fL
#ifndef OPENSSL_NO_SM3
			return (cipher)EVP_sm3();
#else
			return nullptr;
#endif
#else
			return nullptr;
#endif
		}

		proofs::curve proofs::curves::c2pnb163v1()
		{
#ifdef NID_X9_62_c2pnb163v1
			return NID_X9_62_c2pnb163v1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::c2pnb163v2()
		{
#ifdef NID_X9_62_c2pnb163v2
			return NID_X9_62_c2pnb163v2;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::c2pnb163v3()
		{
#ifdef NID_X9_62_c2pnb163v3
			return NID_X9_62_c2pnb163v3;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::c2pnb176v1()
		{
#ifdef NID_X9_62_c2pnb176v1
			return NID_X9_62_c2pnb176v1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::c2tnb191v1()
		{
#ifdef NID_X9_62_c2tnb191v1
			return NID_X9_62_c2tnb191v1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::c2tnb191v2()
		{
#ifdef NID_X9_62_c2tnb191v2
			return NID_X9_62_c2tnb191v2;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::c2tnb191v3()
		{
#ifdef NID_X9_62_c2tnb191v3
			return NID_X9_62_c2tnb191v3;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::c2onb191v4()
		{
#ifdef NID_X9_62_c2onb191v4
			return NID_X9_62_c2onb191v4;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::c2onb191v5()
		{
#ifdef NID_X9_62_c2onb191v5
			return NID_X9_62_c2onb191v5;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::c2pnb208w1()
		{
#ifdef NID_X9_62_c2pnb208w1
			return NID_X9_62_c2pnb208w1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::c2tnb239v1()
		{
#ifdef NID_X9_62_c2tnb239v1
			return NID_X9_62_c2tnb239v1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::c2tnb239v2()
		{
#ifdef NID_X9_62_c2tnb239v2
			return NID_X9_62_c2tnb239v2;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::c2tnb239v3()
		{
#ifdef NID_X9_62_c2tnb239v3
			return NID_X9_62_c2tnb239v3;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::c2onb239v4()
		{
#ifdef NID_X9_62_c2onb239v4
			return NID_X9_62_c2onb239v4;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::c2onb239v5()
		{
#ifdef NID_X9_62_c2onb239v5
			return NID_X9_62_c2onb239v5;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::c2pnb272w1()
		{
#ifdef NID_X9_62_c2pnb272w1
			return NID_X9_62_c2pnb272w1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::c2pnb304w1()
		{
#ifdef NID_X9_62_c2pnb304w1
			return NID_X9_62_c2pnb304w1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::c2tnb359v1()
		{
#ifdef NID_X9_62_c2tnb359v1
			return NID_X9_62_c2tnb359v1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::c2pnb368w1()
		{
#ifdef NID_X9_62_c2pnb368w1
			return NID_X9_62_c2pnb368w1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::c2tnb431r1()
		{
#ifdef NID_X9_62_c2tnb431r1
			return NID_X9_62_c2tnb431r1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::prime192v1()
		{
#ifdef NID_X9_62_prime192v1
			return NID_X9_62_prime192v1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::prime192v2()
		{
#ifdef NID_X9_62_prime192v2
			return NID_X9_62_prime192v2;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::prime192v3()
		{
#ifdef NID_X9_62_prime192v3
			return NID_X9_62_prime192v3;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::prime239v1()
		{
#ifdef NID_X9_62_prime239v1
			return NID_X9_62_prime239v1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::prime239v2()
		{
#ifdef NID_X9_62_prime239v2
			return NID_X9_62_prime239v2;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::prime239v3()
		{
#ifdef NID_X9_62_prime239v3
			return NID_X9_62_prime239v3;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::prime256v1()
		{
#ifdef NID_X9_62_prime256v1
			return NID_X9_62_prime256v1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::ecdsa_sha1()
		{
#ifdef NID_ecdsa_with_SHA1
			return NID_ecdsa_with_SHA1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::ecdsa_sha224()
		{
#ifdef NID_ecdsa_with_SHA224
			return NID_ecdsa_with_SHA224;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::ecdsa_sha256()
		{
#ifdef NID_ecdsa_with_SHA256
			return NID_ecdsa_with_SHA256;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::ecdsa_sha384()
		{
#ifdef NID_ecdsa_with_SHA384
			return NID_ecdsa_with_SHA384;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::ecdsa_sha512()
		{
#ifdef NID_ecdsa_with_SHA512
			return NID_ecdsa_with_SHA512;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::secp112r1()
		{
#ifdef NID_secp112r1
			return NID_secp112r1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::secp112r2()
		{
#ifdef NID_secp112r2
			return NID_secp112r2;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::secp128r1()
		{
#ifdef NID_secp128r1
			return NID_secp128r1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::secp128r2()
		{
#ifdef NID_secp128r2
			return NID_secp128r2;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::secp160k1()
		{
#ifdef NID_secp160k1
			return NID_secp160k1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::secp160r1()
		{
#ifdef NID_secp160r1
			return NID_secp160r1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::secp160r2()
		{
#ifdef NID_secp160r2
			return NID_secp160r2;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::secp192k1()
		{
#ifdef NID_secp192k1
			return NID_secp192k1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::secp224k1()
		{
#ifdef NID_secp224k1
			return NID_secp224k1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::secp224r1()
		{
#ifdef NID_secp224r1
			return NID_secp224r1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::secp256k1()
		{
#ifdef NID_secp256k1
			return NID_secp256k1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::secp384r1()
		{
#ifdef NID_secp384r1
			return NID_secp384r1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::secp521r1()
		{
#ifdef NID_secp521r1
			return NID_secp521r1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::sect113r1()
		{
#ifdef NID_sect113r1
			return NID_sect113r1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::sect113r2()
		{
#ifdef NID_sect113r2
			return NID_sect113r2;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::sect131r1()
		{
#ifdef NID_sect131r1
			return NID_sect131r1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::sect131r2()
		{
#ifdef NID_sect131r2
			return NID_sect131r2;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::sect163k1()
		{
#ifdef NID_sect163k1
			return NID_sect163k1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::sect163r1()
		{
#ifdef NID_sect163r1
			return NID_sect163r1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::sect163r2()
		{
#ifdef NID_sect163r2
			return NID_sect163r2;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::sect193r1()
		{
#ifdef NID_sect193r1
			return NID_sect193r1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::sect193r2()
		{
#ifdef NID_sect193r2
			return NID_sect193r2;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::sect233k1()
		{
#ifdef NID_sect233k1
			return NID_sect233k1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::sect233r1()
		{
#ifdef NID_sect233r1
			return NID_sect233r1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::sect239k1()
		{
#ifdef NID_sect239k1
			return NID_sect239k1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::sect283k1()
		{
#ifdef NID_sect283k1
			return NID_sect283k1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::sect283r1()
		{
#ifdef NID_sect283r1
			return NID_sect283r1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::sect409k1()
		{
#ifdef NID_sect409k1
			return NID_sect409k1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::sect409r1()
		{
#ifdef NID_sect409r1
			return NID_sect409r1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::sect571k1()
		{
#ifdef NID_sect571k1
			return NID_sect571k1;
#else
			return 0;
#endif
		}
		proofs::curve proofs::curves::sect571r1()
		{
#ifdef NID_sect571r1
			return NID_sect571r1;
#else
			return 0;
#endif
		}

		proof proofs::pk_rsa()
		{
#ifdef EVP_PK_RSA
			return (proof)EVP_PK_RSA;
#else
			return 0;
#endif
		}
		proof proofs::pk_dsa()
		{
#ifdef EVP_PK_DSA
			return (proof)EVP_PK_DSA;
#else
			return 0;
#endif
		}
		proof proofs::pk_dh()
		{
#ifdef EVP_PK_DH
			return (proof)EVP_PK_DH;
#else
			return 0;
#endif
		}
		proof proofs::pk_ec()
		{
#ifdef EVP_PK_EC
			return (proof)EVP_PK_EC;
#else
			return 0;
#endif
		}
		proof proofs::pkt_sign()
		{
#ifdef EVP_PKT_SIGN
			return (proof)EVP_PKT_SIGN;
#else
			return 0;
#endif
		}
		proof proofs::pkt_enc()
		{
#ifdef EVP_PKT_ENC
			return (proof)EVP_PKT_ENC;
#else
			return 0;
#endif
		}
		proof proofs::pkt_exch()
		{
#ifdef EVP_PKT_EXCH
			return (proof)EVP_PKT_EXCH;
#else
			return 0;
#endif
		}
		proof proofs::pks_rsa()
		{
#ifdef EVP_PKS_RSA
			return (proof)EVP_PKS_RSA;
#else
			return 0;
#endif
		}
		proof proofs::pks_dsa()
		{
#ifdef EVP_PKS_DSA
			return (proof)EVP_PKS_DSA;
#else
			return 0;
#endif
		}
		proof proofs::pks_ec()
		{
#ifdef EVP_PKS_EC
			return (proof)EVP_PKS_EC;
#else
			return 0;
#endif
		}
		proof proofs::rsa()
		{
#ifdef EVP_PKEY_RSA
			return (proof)EVP_PKEY_RSA;
#else
			return 0;
#endif
		}
		proof proofs::rsa2()
		{
#ifdef EVP_PKEY_RSA2
			return (proof)EVP_PKEY_RSA2;
#else
			return 0;
#endif
		}
		proof proofs::rsa_pss()
		{
#ifdef EVP_PKEY_RSA_PSS
			return (proof)EVP_PKEY_RSA_PSS;
#else
			return 0;
#endif
		}
		proof proofs::dsa()
		{
#ifdef EVP_PKEY_DSA
			return (proof)EVP_PKEY_DSA;
#else
			return 0;
#endif
		}
		proof proofs::dsa1()
		{
#ifdef EVP_PKEY_DSA1
			return (proof)EVP_PKEY_DSA1;
#else
			return 0;
#endif
		}
		proof proofs::dsa2()
		{
#ifdef EVP_PKEY_DSA2
			return (proof)EVP_PKEY_DSA2;
#else
			return 0;
#endif
		}
		proof proofs::dsa3()
		{
#ifdef EVP_PKEY_DSA3
			return (proof)EVP_PKEY_DSA3;
#else
			return 0;
#endif
		}
		proof proofs::dsa4()
		{
#ifdef EVP_PKEY_DSA4
			return (proof)EVP_PKEY_DSA4;
#else
			return 0;
#endif
		}
		proof proofs::dh()
		{
#ifdef EVP_PKEY_DH
			return (proof)EVP_PKEY_DH;
#else
			return 0;
#endif
		}
		proof proofs::dhx()
		{
#ifdef EVP_PKEY_DHX
			return (proof)EVP_PKEY_DHX;
#else
			return 0;
#endif
		}
		proof proofs::ec()
		{
#ifdef EVP_PKEY_EC
			return (proof)EVP_PKEY_EC;
#else
			return 0;
#endif
		}
		proof proofs::sm2()
		{
#ifdef EVP_PKEY_SM2
			return (proof)EVP_PKEY_SM2;
#else
			return 0;
#endif
		}
		proof proofs::hmac()
		{
#ifdef EVP_PKEY_HMAC
			return (proof)EVP_PKEY_HMAC;
#else
			return 0;
#endif
		}
		proof proofs::cmac()
		{
#ifdef EVP_PKEY_CMAC
			return (proof)EVP_PKEY_CMAC;
#else
			return 0;
#endif
		}
		proof proofs::scrypt()
		{
#ifdef EVP_PKEY_SCRYPT
			return (proof)EVP_PKEY_SCRYPT;
#else
			return 0;
#endif
		}
		proof proofs::tls1_prf()
		{
#ifdef EVP_PKEY_TLS1_PRF
			return (proof)EVP_PKEY_TLS1_PRF;
#else
			return 0;
#endif
		}
		proof proofs::hkdf()
		{
#ifdef EVP_PKEY_HKDF
			return (proof)EVP_PKEY_HKDF;
#else
			return 0;
#endif
		}
		proof proofs::poly1305()
		{
#ifdef EVP_PKEY_POLY1305
			return (proof)EVP_PKEY_POLY1305;
#else
			return 0;
#endif
		}
		proof proofs::siphash()
		{
#ifdef EVP_PKEY_SIPHASH
			return (proof)EVP_PKEY_SIPHASH;
#else
			return 0;
#endif
		}
		proof proofs::x25519()
		{
#ifdef EVP_PKEY_X25519
			return (proof)EVP_PKEY_X25519;
#else
			return 0;
#endif
		}
		proof proofs::ed25519()
		{
#ifdef EVP_PKEY_ED25519
			return (proof)EVP_PKEY_ED25519;
#else
			return 0;
#endif
		}
		proof proofs::x448()
		{
#ifdef EVP_PKEY_X448
			return (proof)EVP_PKEY_X448;
#else
			return 0;
#endif
		}
		proof proofs::ed448()
		{
#ifdef EVP_PKEY_ED448
			return (proof)EVP_PKEY_ED448;
#else
			return 0;
#endif
		}

		digest crypto::get_digest_by_name(const std::string_view& name)
		{
			VI_ASSERT(core::stringify::is_cstring(name), "digest name should not be empty");
#ifdef EVP_MD_name
			return (digest)EVP_get_digestbyname(name.data());
#else
			return nullptr;
#endif
		}
		cipher crypto::get_cipher_by_name(const std::string_view& name)
		{
			VI_ASSERT(core::stringify::is_cstring(name), "cipher name should not be empty");
#ifdef EVP_MD_name
			return (cipher)EVP_get_cipherbyname(name.data());
#else
			return nullptr;
#endif
		}
		proof crypto::get_proof_by_name(const std::string_view& name)
		{
#ifdef EVP_MD_name
			auto* method = EVP_PKEY_asn1_find_str(nullptr, name.data(), (int)name.size());
			if (!method)
				return -1;

			int key_type = -1;
			if (EVP_PKEY_asn1_get0_info(&key_type, nullptr, nullptr, nullptr, nullptr, method) != 1)
				return -1;

			return (proof)key_type;
#else
			return -1;
#endif
		}
		proofs::curve crypto::get_curve_by_name(const std::string_view& name)
		{
			return get_proof_by_name(name);
		}
		std::string_view crypto::get_digest_name(digest type)
		{
			if (!type)
				return "none";
#ifdef EVP_MD_name
			const char* name = EVP_MD_name((EVP_MD*)type);
			return name ? name : "";
#else
			return "";
#endif
		}
		std::string_view crypto::get_cipher_name(cipher type)
		{
			VI_ASSERT(type != nullptr, "cipher should be set");
#ifdef EVP_CIPHER_name
			const char* name = EVP_CIPHER_name((EVP_CIPHER*)type);
			return name ? name : "";
#else
			return "";
#endif
		}
		std::string_view crypto::get_proof_name(proof type)
		{
#ifdef VI_OPENSSL
			if (type <= 0)
				return "none";

			const char* name = OBJ_nid2sn(type);
			return name ? name : "";
#else
			return "";
#endif
		}
		std::string_view crypto::get_curve_name(proofs::curve type)
		{
			return get_proof_name(type);
		}
		expects_crypto<void> crypto::fill_random_bytes(uint8_t* buffer, size_t length)
		{
#ifdef VI_OPENSSL
			VI_TRACE("crypto fill random %" PRIu64 " bytes", (uint64_t)length);
			if (RAND_bytes(buffer, (int)length) == 1)
				return core::expectation::met;
#endif
			return crypto_exception();
		}
		expects_crypto<core::string> crypto::random_bytes(size_t length)
		{
#ifdef VI_OPENSSL
			core::string buffer(length, '\0');
			auto status = fill_random_bytes((uint8_t*)buffer.data(), buffer.size());
			if (!status)
				return status.error();

			return buffer;
#else
			return crypto_exception();
#endif
		}
		expects_crypto<core::string> crypto::checksum_hex(digest type, core::stream* stream)
		{
			auto data = crypto::checksum(type, stream);
			if (!data)
				return data;

			return codec::hex_encode(*data);
		}
		expects_crypto<core::string> crypto::checksum(digest type, core::stream* stream)
		{
			VI_ASSERT(type != nullptr, "type should be set");
			VI_ASSERT(stream != nullptr, "stream should be set");
#ifdef VI_OPENSSL
			VI_TRACE("crypto %s stream-hash fd %i", get_digest_name(type).data(), (int)stream->get_readable_fd());
			EVP_MD* method = (EVP_MD*)type;
			EVP_MD_CTX* context = EVP_MD_CTX_create();
			if (!context)
				return crypto_exception();

			core::string result;
			result.resize(EVP_MD_size(method));

			uint32_t size = 0; bool OK = true;
			OK = EVP_DigestInit_ex(context, method, nullptr) == 1 ? OK : false;
			{
				uint8_t buffer[core::BLOB_SIZE]; size_t size = 0;
				while ((size = stream->read(buffer, sizeof(buffer)).or_else(0)) > 0)
					OK = EVP_DigestUpdate(context, buffer, size) == 1 ? OK : false;
			}
			if (OK)
			{
				OK = EVP_DigestFinal_ex(context, (uint8_t*)result.data(), &size) == 1 ? OK : false;
				if (!OK)
				{
					size = (uint32_t)result.size();
					OK = EVP_DigestFinalXOF(context, (uint8_t*)result.data(), size) == 1;
				}
			}
			EVP_MD_CTX_destroy(context);

			if (!OK)
				return crypto_exception();

			result.resize((size_t)size);
			return result;
#else
			return crypto_exception();
#endif
		}
		expects_crypto<core::string> crypto::hash_hex(digest type, const std::string_view& value)
		{
			auto data = crypto::hash(type, value);
			if (!data)
				return data;

			return codec::hex_encode(*data);
		}
		expects_crypto<core::string> crypto::hash(digest type, const std::string_view& value)
		{
#ifdef VI_OPENSSL
			VI_TRACE("crypto %s hash %" PRIu64 " bytes", get_digest_name(type).data(), (uint64_t)value.size());
			if (!type || value.empty())
				return core::string(value);

			EVP_MD* method = (EVP_MD*)type;
			EVP_MD_CTX* context = EVP_MD_CTX_create();
			if (!context)
				return crypto_exception();

			core::string result;
			result.resize(EVP_MD_size(method));

			uint32_t size = 0; bool OK = true;
			OK = EVP_DigestInit_ex(context, method, nullptr) == 1 ? OK : false;
			OK = EVP_DigestUpdate(context, value.data(), value.size()) == 1 ? OK : false;
			if (OK)
			{
				OK = EVP_DigestFinal_ex(context, (uint8_t*)result.data(), &size) == 1 ? OK : false;
				if (!OK)
				{
					size = (uint32_t)result.size();
					OK = EVP_DigestFinalXOF(context, (uint8_t*)result.data(), size) == 1;
				}
			}
			EVP_MD_CTX_destroy(context);
			if (!OK)
				return crypto_exception();

			result.resize((size_t)size);
			return result;
#else
			return crypto_exception();
#endif
		}
		expects_crypto<secret_box> crypto::private_key_gen(proof type, size_t target_bits)
		{
#ifdef VI_OPENSSL
			VI_TRACE("crypto generate %s private key (bits: %i)", get_proof_name(type).data(), (int)target_bits);
			uint8_t data[8192]; int data_size = -1;
			if (target_bits > sizeof(data) * 8)
				return crypto_exception(-1, "bad:length");

			EVP_PKEY_CTX* context = EVP_PKEY_CTX_new_id((int)type, nullptr);
			if (!context)
				return crypto_exception();

			if (EVP_PKEY_keygen_init(context) != 1)
			{
				EVP_PKEY_CTX_free(context);
				return crypto_exception();
			}

			switch (type)
			{
				case EVP_PKEY_RSA:
				case EVP_PKEY_RSA2:
				case EVP_PKEY_RSA_PSS:
				{
					if (EVP_PKEY_CTX_set_rsa_keygen_bits(context, (int)target_bits) != 1)
						break;

					EVP_PKEY* key = nullptr;
					if (EVP_PKEY_keygen(context, &key) != 1)
						break;

					RSA* rsa_key = EVP_PKEY_get1_RSA(key);
					if (!rsa_key)
					{
						EVP_PKEY_free(key);
						break;
					}

					uint8_t* result = nullptr;
					data_size = i2d_RSAPrivateKey(rsa_key, &result);
					if (data_size > 0)
						memcpy(data, result, (size_t)data_size);
					RSA_free(rsa_key);
					EVP_PKEY_free(key);
					OPENSSL_free(result);
					break;
				}
				case EVP_PKEY_DSA:
				case EVP_PKEY_DSA1:
				case EVP_PKEY_DSA2:
				case EVP_PKEY_DSA3:
				case EVP_PKEY_DSA4:
				{
					DSA* dsa_key = DSA_new();
					if (!dsa_key || !DSA_generate_parameters_ex(dsa_key, (int)target_bits, nullptr, 0, nullptr, nullptr, nullptr) || !DSA_generate_key(dsa_key))
					{
						if (dsa_key != nullptr)
							DSA_free(dsa_key);
						break;
					}

					uint8_t* result = nullptr;
					data_size = i2d_DSAPrivateKey(dsa_key, &result);
					if (data_size > 0)
						memcpy(data, result, (size_t)data_size);
					DSA_free(dsa_key);
					OPENSSL_free(result);
					break;
				}
				case EVP_PKEY_DH:
				case EVP_PKEY_DHX:
					return crypto_exception(-1, "bad:proof");
				default:
				{
					EVP_PKEY* key = nullptr;
					if (EVP_PKEY_keygen(context, &key) != 1)
						break;

					size_t length = sizeof(data);
					if (EVP_PKEY_get_raw_private_key(key, data, &length) != 1)
					{
						EVP_PKEY_free(key);
						break;
					}

					data_size = (int)length;
					EVP_PKEY_free(key);
					break;
				}
			}

			EVP_PKEY_CTX_free(context);
			if (data_size <= 0)
				return crypto_exception();

			return secret_box::secure(std::string_view((char*)data, (size_t)data_size));
#else
			return crypto_exception();
#endif
		}
		expects_crypto<core::string> crypto::to_public_key(proof type, const secret_box& secret_key)
		{
#ifdef VI_OPENSSL
			VI_TRACE("crypto generate %s public key from %i bytes", get_proof_name(type).data(), (int)secret_key.size());
			auto local_key = secret_key.expose<core::CHUNK_SIZE>();
			uint8_t buffer[8192];
			switch (type)
			{
				case EVP_PKEY_RSA:
				case EVP_PKEY_RSA2:
				case EVP_PKEY_RSA_PSS:
				{
					RSA* rsa_key = nullptr;
					const uint8_t* data = local_key.buffer;
					if (!d2i_RSAPrivateKey(&rsa_key, &data, (long)local_key.view.size()))
						return crypto_exception();

					uint8_t* result = nullptr;
					int length = i2d_RSAPublicKey(rsa_key, &result);
					RSA_free(rsa_key);
					if (length <= 0)
					{
						OPENSSL_free(result);
						return crypto_exception();
					}

					memcpy(buffer, result, (size_t)length);
					OPENSSL_free(result);
					return core::string((char*)buffer, (size_t)length);
				}
				case EVP_PKEY_DSA:
				case EVP_PKEY_DSA1:
				case EVP_PKEY_DSA2:
				case EVP_PKEY_DSA3:
				case EVP_PKEY_DSA4:
				{
					DSA* dsa_key = nullptr;
					const uint8_t* data = local_key.buffer;
					if (!d2i_DSAPrivateKey(&dsa_key, &data, (long)local_key.view.size()))
						return crypto_exception();

					uint8_t* result = nullptr;
					int length = i2d_DSAPublicKey(dsa_key, &result);
					DSA_free(dsa_key);
					if (length <= 0)
					{
						OPENSSL_free(result);
						return crypto_exception();
					}

					memcpy(buffer, result, (size_t)length);
					OPENSSL_free(result);
					return core::string((char*)buffer, (size_t)length);
				}
				case EVP_PKEY_DH:
				case EVP_PKEY_DHX:
					return crypto_exception(-1, "bad:proof");
				default:
				{
					EVP_PKEY* key = EVP_PKEY_new_raw_private_key((int)type, nullptr, (uint8_t*)local_key.view.data(), local_key.view.size());
					if (!key)
						return crypto_exception();

					size_t length;
					if (EVP_PKEY_get_raw_public_key(key, nullptr, &length) != 1)
					{
						EVP_PKEY_free(key);
						return crypto_exception();
					}

					core::string public_key;
					public_key.resize(length);

					if (EVP_PKEY_get_raw_public_key(key, (uint8_t*)public_key.data(), &length) != 1)
					{
						EVP_PKEY_free(key);
						return crypto_exception();
					}

					EVP_PKEY_free(key);
					return public_key;
				}
			}
#else
			return crypto_exception();
#endif
		}
		expects_crypto<core::string> crypto::sign(digest type, proof key_type, const std::string_view& value, const secret_box& secret_key)
		{
#ifdef VI_OPENSSL
			VI_TRACE("crypto %s:%s sign %" PRIu64 " bytes", get_digest_name(type).data(), get_proof_name(key_type).data(), (uint64_t)value.size());
			if (value.empty())
				return crypto_exception(-1, "sign:empty");

			auto local_key = secret_key.expose<core::CHUNK_SIZE>();
			EVP_PKEY* key;
			switch (key_type)
			{
				case EVP_PKEY_RSA:
				case EVP_PKEY_RSA2:
				case EVP_PKEY_RSA_PSS:
				{
					key = EVP_PKEY_new();
					if (key != nullptr)
					{
						RSA* rsa_key = nullptr;
						const uint8_t* data = local_key.buffer;
						if (!d2i_RSAPrivateKey(&rsa_key, &data, (long)local_key.view.size()))
						{
							EVP_PKEY_free(key);
							return crypto_exception();
						}
						EVP_PKEY_set1_RSA(key, rsa_key);
						RSA_free(rsa_key);
					}
					break;
				}
				case EVP_PKEY_DSA:
				case EVP_PKEY_DSA1:
				case EVP_PKEY_DSA2:
				case EVP_PKEY_DSA3:
				case EVP_PKEY_DSA4:
				{
					key = EVP_PKEY_new();
					if (key != nullptr)
					{
						DSA* dsa_key = nullptr;
						const uint8_t* data = local_key.buffer;
						if (!d2i_DSAPrivateKey(&dsa_key, &data, (long)local_key.view.size()))
						{
							EVP_PKEY_free(key);
							return crypto_exception();
						}
						EVP_PKEY_set1_DSA(key, dsa_key);
						DSA_free(dsa_key);
					}
					break;
				}
				case EVP_PKEY_DH:
				case EVP_PKEY_DHX:
					return crypto_exception(-1, "bad:proof");
				default:
				{
					key = EVP_PKEY_new_raw_private_key((int)key_type, nullptr, (uint8_t*)local_key.view.data(), local_key.view.size());
					break;
				}
			}

			if (!key)
				return crypto_exception();

			EVP_MD_CTX* context = EVP_MD_CTX_create();
			if (EVP_DigestSignInit(context, nullptr, (EVP_MD*)type, nullptr, key) != 1)
			{
				EVP_MD_CTX_free(context);
				EVP_PKEY_free(key);
				return crypto_exception();
			}

			size_t length;
			if (EVP_DigestSign(context, nullptr, &length, (uint8_t*)value.data(), value.size()) != 1)
			{
				EVP_MD_CTX_free(context);
				EVP_PKEY_free(key);
				return crypto_exception();
			}

			core::string signature;
			signature.resize(length);

			if (EVP_DigestSign(context, (uint8_t*)signature.data(), &length, (uint8_t*)value.data(), value.size()) != 1)
			{
				EVP_MD_CTX_free(context);
				EVP_PKEY_free(key);
				return crypto_exception();
			}

			EVP_MD_CTX_free(context);
			EVP_PKEY_free(key);
			return signature;
#else
			return crypto_exception();
#endif
		}
		expects_crypto<bool> crypto::verify(digest type, proof key_type, const std::string_view& value, const std::string_view& signature, const std::string_view& public_key)
		{
#ifdef VI_OPENSSL
			VI_TRACE("crypto %s:%s verify %" PRIu64 " bytes", get_digest_name(type).data(), get_proof_name(key_type).data(), (uint64_t)(value.size() + signature.size()));
			if (value.empty())
				return crypto_exception(-1, "verify:empty");

			EVP_PKEY* key = nullptr;
			switch (key_type)
			{
				case EVP_PKEY_RSA:
				case EVP_PKEY_RSA2:
				case EVP_PKEY_RSA_PSS:
				{
					key = EVP_PKEY_new();
					if (key != nullptr)
					{
						RSA* rsa_key = nullptr;
						const uint8_t* data = (uint8_t*)public_key.data();
						if (!d2i_RSAPublicKey(&rsa_key, &data, (long)public_key.size()))
						{
							EVP_PKEY_free(key);
							return crypto_exception();
						}
						EVP_PKEY_set1_RSA(key, rsa_key);
						RSA_free(rsa_key);
					}
					break;
				}
				case EVP_PKEY_DSA:
				case EVP_PKEY_DSA1:
				case EVP_PKEY_DSA2:
				case EVP_PKEY_DSA3:
				case EVP_PKEY_DSA4:
				{
					key = EVP_PKEY_new();
					if (key != nullptr)
					{
						DSA* dsa_key = nullptr;
						const uint8_t* data = (uint8_t*)public_key.data();
						if (!d2i_DSAPublicKey(&dsa_key, &data, (long)public_key.size()))
						{
							EVP_PKEY_free(key);
							return crypto_exception();
						}
						EVP_PKEY_set1_DSA(key, dsa_key);
						DSA_free(dsa_key);
					}
					break;
				}
				case EVP_PKEY_DH:
				case EVP_PKEY_DHX:
					return crypto_exception(-1, "bad:proof");
				default:
				{
					key = EVP_PKEY_new_raw_public_key((int)key_type, nullptr, (uint8_t*)public_key.data(), public_key.size());
					break;
				}
			}

			if (!key)
				return crypto_exception();

			EVP_MD_CTX* context = EVP_MD_CTX_create();
			if (EVP_DigestVerifyInit(context, nullptr, (EVP_MD*)type, nullptr, key) != 1)
			{
				EVP_MD_CTX_free(context);
				EVP_PKEY_free(key);
				return crypto_exception();
			}

			if (EVP_DigestVerify(context, (uint8_t*)signature.data(), signature.size(), (uint8_t*)value.data(), value.size()) != 1)
			{
				EVP_MD_CTX_free(context);
				EVP_PKEY_free(key);
				return crypto_exception();
			}

			EVP_MD_CTX_free(context);
			EVP_PKEY_free(key);
			return true;
#else
			return crypto_exception();
#endif
		}
		expects_crypto<secret_box> crypto::ec_private_key_gen(proofs::curve curve)
		{
#ifdef VI_OPENSSL
			VI_TRACE("crypto generate %s private key", get_curve_name(curve).data());
			EC_KEY* key = EC_KEY_new_by_curve_name(curve);
			if (!key)
				return crypto_exception();

			if (EC_KEY_generate_key(key) != 1)
			{
				EC_KEY_free(key);
				return crypto_exception();
			}

			uint8_t buffer[256];
			int buffer_size = BN_bn2binpad(EC_KEY_get0_private_key(key), buffer, (int)BN_num_bytes(EC_GROUP_get0_order(EC_KEY_get0_group(key))));
			EC_KEY_free(key);
			if (buffer_size <= 0)
				return crypto_exception();

			return secret_box::secure(std::string_view((char*)buffer, (size_t)buffer_size));
#else
			return crypto_exception();
#endif
		}
		expects_crypto<core::string> crypto::ec_to_public_key(proofs::curve curve, proofs::format format, const secret_box& secret_key)
		{
#ifdef VI_OPENSSL
			VI_TRACE("crypto generate %s private key from %i bytes", get_curve_name(curve).data(), (int)secret_key.size());
			auto local_key = secret_key.expose<core::CHUNK_SIZE>();
			BIGNUM* scalar = BN_bin2bn(local_key.buffer, (int)local_key.view.size(), nullptr);
			if (!scalar)
				return crypto_exception();

			EC_GROUP* group = EC_GROUP_new_by_curve_name(curve);
			if (!group)
			{
				BN_clear_free(scalar);
				return crypto_exception();
			}

			EC_POINT* point = EC_POINT_new(group);
			bool success = EC_POINT_mul(group, point, scalar, nullptr, nullptr, nullptr) == 1;
			BN_clear_free(scalar);
			if (!success)
			{
				EC_POINT_free(point);
				EC_GROUP_free(group);
				return crypto_exception();
			}

			uint8_t buffer[1024] = { 0 };
			size_t buffer_size = EC_POINT_point2oct(group, point, to_point_conversion(format), buffer, sizeof(buffer), nullptr);
			EC_POINT_free(point);
			EC_GROUP_free(group);
			if (!buffer_size)
				return crypto_exception();

			return core::string((char*)buffer, buffer_size);
#else
			return crypto_exception();
#endif
		}
		expects_crypto<secret_box> crypto::ec_scalar_add(proofs::curve curve, const secret_box& a, const secret_box& b)
		{
#ifdef VI_OPENSSL
			VI_TRACE("crypto scalar sub %s", get_curve_name(curve).data());
			auto local = a.expose<core::CHUNK_SIZE>();
			BIGNUM* scalar1 = BN_bin2bn(local.buffer, (int)local.view.size(), nullptr);
			if (!scalar1)
				return crypto_exception();

			local = b.expose<core::CHUNK_SIZE>();
			BIGNUM* scalar2 = BN_bin2bn(local.buffer, (int)local.view.size(), nullptr);
			if (!scalar2)
			{
				BN_clear_free(scalar1);
				return crypto_exception();
			}

			EC_GROUP* group = EC_GROUP_new_by_curve_name(curve);
			if (!group)
			{
				BN_clear_free(scalar1);
				BN_clear_free(scalar2);
				return crypto_exception();
			}

			BIGNUM* result = BN_new();
			const BIGNUM* order = EC_GROUP_get0_order(group);
			size_t order_size = BN_num_bytes(order);
			bool success = result != nullptr && BN_mod_sub(result, scalar1, scalar2, order, nullptr) == 1;
			EC_GROUP_free(group);
			BN_clear_free(scalar1);
			BN_clear_free(scalar2);
			if (!success)
			{
				if (result != nullptr)
					BN_clear_free(result);
				return crypto_exception();
			}

			uint8_t buffer[256];
			int buffer_size = BN_bn2binpad(result, buffer, (int)order_size);
			BN_clear_free(result);
			if (buffer_size <= 0)
				return crypto_exception();

			return secret_box::secure(std::string_view((char*)buffer, (size_t)buffer_size));
#else
			return crypto_exception();
#endif
		}
		expects_crypto<secret_box> crypto::ec_scalar_sub(proofs::curve curve, const secret_box& a, const secret_box& b)
		{
#ifdef VI_OPENSSL
			VI_TRACE("crypto scalar add %s", get_curve_name(curve).data());
			auto local = a.expose<core::CHUNK_SIZE>();
			BIGNUM* scalar1 = BN_bin2bn(local.buffer, (int)local.view.size(), nullptr);
			if (!scalar1)
				return crypto_exception();

			local = b.expose<core::CHUNK_SIZE>();
			BIGNUM* scalar2 = BN_bin2bn(local.buffer, (int)local.view.size(), nullptr);
			if (!scalar2)
			{
				BN_clear_free(scalar1);
				return crypto_exception();
			}

			EC_GROUP* group = EC_GROUP_new_by_curve_name(curve);
			if (!group)
			{
				BN_clear_free(scalar1);
				BN_clear_free(scalar2);
				return crypto_exception();
			}

			BIGNUM* result = BN_new();
			const BIGNUM* order = EC_GROUP_get0_order(group);
			size_t order_size = BN_num_bytes(order);
			bool success = result != nullptr && BN_mod_add(result, scalar1, scalar2, order, nullptr) == 1;
			EC_GROUP_free(group);
			BN_clear_free(scalar1);
			BN_clear_free(scalar2);
			if (!success)
			{
				if (result != nullptr)
					BN_clear_free(result);
				return crypto_exception();
			}

			uint8_t buffer[256];
			int buffer_size = BN_bn2binpad(result, buffer, (int)order_size);
			BN_clear_free(result);
			if (buffer_size <= 0)
				return crypto_exception();

			return secret_box::secure(std::string_view((char*)buffer, (size_t)buffer_size));
#else
			return crypto_exception();
#endif
		}
		expects_crypto<secret_box> crypto::ec_scalar_mul(proofs::curve curve, const secret_box& a, const secret_box& b)
		{
#ifdef VI_OPENSSL
			VI_TRACE("crypto scalar mul %s", get_curve_name(curve).data());
			auto local = a.expose<core::CHUNK_SIZE>();
			BIGNUM* scalar1 = BN_bin2bn(local.buffer, (int)local.view.size(), nullptr);
			if (!scalar1)
				return crypto_exception();

			local = b.expose<core::CHUNK_SIZE>();
			BIGNUM* scalar2 = BN_bin2bn(local.buffer, (int)local.view.size(), nullptr);
			if (!scalar2)
			{
				BN_clear_free(scalar1);
				return crypto_exception();
			}

			EC_GROUP* group = EC_GROUP_new_by_curve_name(curve);
			if (!group)
			{
				BN_clear_free(scalar1);
				BN_clear_free(scalar2);
				return crypto_exception();
			}

			BIGNUM* result = BN_new();
			const BIGNUM* order = EC_GROUP_get0_order(group);
			size_t order_size = BN_num_bytes(order);
			bool success = result != nullptr && BN_mod_mul(result, scalar1, scalar2, order, nullptr) == 1;
			EC_GROUP_free(group);
			BN_clear_free(scalar1);
			BN_clear_free(scalar2);
			if (!success)
			{
				if (result != nullptr)
					BN_clear_free(result);
				return crypto_exception();
			}

			uint8_t buffer[256];
			int buffer_size = BN_bn2binpad(result, buffer, (int)order_size);
			BN_clear_free(result);
			if (buffer_size <= 0)
				return crypto_exception();

			return secret_box::secure(std::string_view((char*)buffer, (size_t)buffer_size));
#else
			return crypto_exception();
#endif
		}
		expects_crypto<secret_box> crypto::ec_scalar_inv(proofs::curve curve, const secret_box& a)
		{
#ifdef VI_OPENSSL
			VI_TRACE("crypto scalar inv %s", get_curve_name(curve).data());
			auto local = a.expose<core::CHUNK_SIZE>();
			BIGNUM* scalar = BN_bin2bn(local.buffer, (int)local.view.size(), nullptr);
			if (!scalar)
				return crypto_exception();

			EC_GROUP* group = EC_GROUP_new_by_curve_name(curve);
			if (!group)
			{
				BN_clear_free(scalar);
				return crypto_exception();
			}

			BIGNUM* result = BN_new();
			const BIGNUM* order = EC_GROUP_get0_order(group);
			size_t order_size = BN_num_bytes(order);
			bool success = result != nullptr && BN_mod_inverse(result, scalar, order, nullptr) != nullptr;
			EC_GROUP_free(group);
			BN_clear_free(scalar);
			if (!success)
			{
				if (result != nullptr)
					BN_clear_free(result);
				return crypto_exception();
			}

			uint8_t buffer[256];
			int buffer_size = BN_bn2binpad(result, buffer, (int)order_size);
			BN_clear_free(result);
			if (buffer_size <= 0)
				return crypto_exception();

			return secret_box::secure(std::string_view((char*)buffer, (size_t)buffer_size));
#else
			return crypto_exception();
#endif
		}
		expects_crypto<bool> crypto::ec_scalar_is_on_curve(proofs::curve curve, const secret_box& a)
		{
#ifdef VI_OPENSSL
			VI_TRACE("crypto scalar check %s", get_curve_name(curve).data());
			auto local = a.expose<core::CHUNK_SIZE>();
			BIGNUM* scalar = BN_bin2bn(local.buffer, (int)local.view.size(), nullptr);
			if (!scalar)
				return crypto_exception();

			EC_GROUP* group = EC_GROUP_new_by_curve_name(curve);
			if (!group)
			{
				BN_clear_free(scalar);
				return crypto_exception();
			}

			bool is_on_curve = BN_cmp(scalar, EC_GROUP_get0_order(group)) < 0;
			BN_clear_free(scalar);
			EC_GROUP_free(group);
			return is_on_curve;
#else
			return crypto_exception();
#endif
		}
		expects_crypto<core::string> crypto::ec_point_mul(proofs::curve curve, proofs::format format, const std::string_view& a, const secret_box& b)
		{
#ifdef VI_OPENSSL
			VI_TRACE("crypto point mul %s", get_curve_name(curve).data());
			EC_GROUP* group = EC_GROUP_new_by_curve_name(curve);
			if (!group)
				return crypto_exception();

			EC_POINT* point1 = EC_POINT_new(group);
			if (EC_POINT_oct2point(group, point1, (uint8_t*)a.data(), a.size(), nullptr) != 1)
			{
				EC_POINT_free(point1);
				EC_GROUP_free(group);
				return crypto_exception();
			}

			auto local = b.expose<core::CHUNK_SIZE>();
			BIGNUM* scalar2 = BN_bin2bn(local.buffer, (int)local.view.size(), nullptr);
			if (!scalar2)
			{
				EC_POINT_free(point1);
				EC_GROUP_free(group);
				return crypto_exception();
			}

			bool success = EC_POINT_mul(group, point1, scalar2, nullptr, nullptr, nullptr) != 1;
			BN_clear_free(scalar2);
			if (!success)
			{
				EC_POINT_free(point1);
				EC_GROUP_free(group);
				return crypto_exception();
			}

			uint8_t buffer[1024] = { 0 };
			size_t buffer_size = EC_POINT_point2oct(group, point1, to_point_conversion(format), buffer, sizeof(buffer), nullptr);
			EC_POINT_free(point1);
			EC_GROUP_free(group);
			if (!buffer_size)
				return crypto_exception();

			return core::string((char*)buffer, buffer_size);
#else
			return crypto_exception();
#endif
		}
		expects_crypto<core::string> crypto::ec_point_add(proofs::curve curve, proofs::format format, const std::string_view& a, const std::string_view& b)
		{
#ifdef VI_OPENSSL
			VI_TRACE("crypto point add %s", get_curve_name(curve).data());
			EC_GROUP* group = EC_GROUP_new_by_curve_name(curve);
			if (!group)
				return crypto_exception();

			EC_POINT* point1 = EC_POINT_new(group);
			if (EC_POINT_oct2point(group, point1, (uint8_t*)a.data(), a.size(), nullptr) != 1)
			{
				EC_POINT_free(point1);
				EC_GROUP_free(group);
				return crypto_exception();
			}

			EC_POINT* point2 = EC_POINT_new(group);
			if (EC_POINT_oct2point(group, point2, (uint8_t*)b.data(), b.size(), nullptr) != 1)
			{
				EC_POINT_free(point1);
				EC_POINT_free(point2);
				EC_GROUP_free(group);
				return crypto_exception();
			}

			EC_POINT* result = EC_POINT_new(group);
			bool success = EC_POINT_add(group, result, point1, point2, nullptr) != 1;
			EC_POINT_free(point1);
			EC_POINT_free(point2);
			if (!success)
			{
				EC_POINT_free(result);
				EC_GROUP_free(group);
				return crypto_exception();
			}

			uint8_t buffer[1024] = { 0 };
			size_t buffer_size = EC_POINT_point2oct(group, result, to_point_conversion(format), buffer, sizeof(buffer), nullptr);
			EC_POINT_free(result);
			EC_GROUP_free(group);
			if (!buffer_size)
				return crypto_exception();

			return core::string((char*)buffer, buffer_size);
#else
			return crypto_exception();
#endif
		}
		expects_crypto<core::string> crypto::ec_point_inv(proofs::curve curve, proofs::format format, const std::string_view& a)
		{
#ifdef VI_OPENSSL
			VI_TRACE("crypto point inv %s", get_curve_name(curve).data());
			EC_GROUP* group = EC_GROUP_new_by_curve_name(curve);
			if (!group)
				return crypto_exception();

			EC_POINT* point = EC_POINT_new(group);
			if (EC_POINT_oct2point(group, point, (uint8_t*)a.data(), a.size(), nullptr) != 1)
			{
				EC_POINT_free(point);
				EC_GROUP_free(group);
				return crypto_exception();
			}

			if (EC_POINT_invert(group, point, nullptr) != 1)
			{
				EC_POINT_free(point);
				EC_GROUP_free(group);
				return crypto_exception();
			}

			uint8_t buffer[1024] = { 0 };
			size_t buffer_size = EC_POINT_point2oct(group, point, to_point_conversion(format), buffer, sizeof(buffer), nullptr);
			EC_POINT_free(point);
			EC_GROUP_free(group);
			if (!buffer_size)
				return crypto_exception();

			return core::string((char*)buffer, buffer_size);
#else
			return crypto_exception();
#endif
		}
		expects_crypto<bool> crypto::ec_point_is_on_curve(proofs::curve curve, const std::string_view& a)
		{
#ifdef VI_OPENSSL
			VI_TRACE("crypto point check %s", get_curve_name(curve).data());
			EC_GROUP* group = EC_GROUP_new_by_curve_name(curve);
			if (!group)
				return crypto_exception();

			EC_POINT* point = EC_POINT_new(group);
			if (EC_POINT_oct2point(group, point, (uint8_t*)a.data(), a.size(), nullptr) != 1)
			{
				EC_POINT_free(point);
				EC_GROUP_free(group);
				return crypto_exception();
			}

			int result = EC_POINT_is_on_curve(group, point, nullptr);
			EC_POINT_free(point);
			EC_GROUP_free(group);
			if (result == -1)
				return crypto_exception();

			bool is_on_curve = result == 1;
			return is_on_curve;
#else
			return crypto_exception();
#endif
		}
		expects_crypto<core::string> crypto::ec_sign(digest type, proofs::curve curve, const std::string_view& value, const secret_box& secret_key)
		{
#ifdef VI_OPENSSL
			VI_TRACE("crypto %s:%s sign %" PRIu64 " bytes", get_digest_name(type).data(), get_curve_name(curve).data(), (uint64_t)value.size());
			if (value.empty())
				return crypto_exception(-1, "sign:empty");

			auto digest = crypto::hash(type, value);
			if (!digest)
				return digest.error();

			auto local_key = secret_key.expose<core::CHUNK_SIZE>();
			BIGNUM* data = BN_bin2bn(local_key.buffer, (int)local_key.view.size(), nullptr);
			if (!data)
				return crypto_exception();

			EC_KEY* ec_key = EC_KEY_new_by_curve_name(curve);
			if (!ec_key)
			{
				BN_free(data);
				return crypto_exception();
			}

			bool success = EC_KEY_set_private_key(ec_key, data) == 1;
			BN_free(data);
			if (!success)
			{
				EC_KEY_free(ec_key);
				return crypto_exception();
			}

			uint8_t signature[1024]; uint32_t signature_size = (uint32_t)sizeof(signature);
			if (ECDSA_sign(0, (uint8_t*)digest->data(), (int)digest->size(), signature, &signature_size, ec_key) != 1)
			{
				EC_KEY_free(ec_key);
				return crypto_exception();
			}

			EC_KEY_free(ec_key);
			return core::string((char*)signature, (size_t)signature_size);
#else
			return crypto_exception();
#endif
		}
		expects_crypto<bool> crypto::ec_verify(digest type, proofs::curve curve, const std::string_view& value, const std::string_view& signature, const std::string_view& public_key)
		{
#ifdef VI_OPENSSL
			VI_TRACE("crypto %s:%s verify %" PRIu64 " bytes", get_digest_name(type).data(), get_curve_name(curve).data(), (uint64_t)(value.size() + signature.size()));
			if (value.empty())
				return crypto_exception(-1, "verify:empty");

			auto digest = crypto::hash(type, value);
			if (!digest)
				return digest.error();

			EC_KEY* ec_key = EC_KEY_new_by_curve_name(curve);
			if (!ec_key)
				return crypto_exception();

			EC_POINT* ec_point = EC_POINT_new(EC_KEY_get0_group(ec_key));
			if (EC_POINT_oct2point(EC_KEY_get0_group(ec_key), ec_point, (uint8_t*)public_key.data(), public_key.size(), nullptr) != 1)
			{
				EC_POINT_free(ec_point);
				EC_KEY_free(ec_key);
				return crypto_exception();
			}

			bool success = EC_KEY_set_public_key(ec_key, ec_point) == 1;
			EC_POINT_free(ec_point);
			if (!success)
			{
				EC_KEY_free(ec_key);
				return crypto_exception();
			}

			int result = ECDSA_verify(0, (uint8_t*)digest->data(), (int)digest->size(), (uint8_t*)signature.data(), (int)signature.size(), ec_key);
			EC_KEY_free(ec_key);
			if (result == -1)
				return crypto_exception();

			return result == 1;
#else
			return crypto_exception();
#endif
		}
		expects_crypto<core::string> crypto::ec_der_to_rs(const std::string_view& signature)
		{
#ifdef VI_OPENSSL
			if (signature.empty())
				return crypto_exception(-1, "signature:empty");

			ECDSA_SIG* ec_signature = nullptr;
			const uint8_t* data = (uint8_t*)signature.data();
			if (!d2i_ECDSA_SIG(&ec_signature, &data, (long)signature.size()))
				return crypto_exception();

			const BIGNUM* r = ECDSA_SIG_get0_r(ec_signature);
			const BIGNUM* s = ECDSA_SIG_get0_s(ec_signature);
			size_t order_size = std::max(BN_num_bytes(r), BN_num_bytes(s));

			uint8_t r_buffer[1024], s_buffer[1024];
			int r_buffer_size = BN_bn2binpad(r, r_buffer, (int)order_size);
			int s_buffer_size = BN_bn2binpad(s, s_buffer, (int)order_size);
			ECDSA_SIG_free(ec_signature);
			if (r_buffer_size <= 0 || s_buffer_size <= 0)
				return crypto_exception();

			core::string result;
			result.reserve((size_t)(r_buffer_size + s_buffer_size));
			result.append((char*)r_buffer, (size_t)r_buffer_size);
			result.append((char*)s_buffer, (size_t)s_buffer_size);
			return expects_crypto<core::string>(std::move(result));
#else
			return crypto_exception();
#endif
		}
		expects_crypto<core::string> crypto::ec_rs_to_der(const std::string_view& signature)
		{
#ifdef VI_OPENSSL
			if (signature.empty() || signature.size() % 2 != 0)
				return crypto_exception(-1, "bad:signature");

			size_t size = signature.size();
			BIGNUM* r = BN_bin2bn((uint8_t*)signature.data() + size * 0, (int)size, nullptr);
			BIGNUM* s = BN_bin2bn((uint8_t*)signature.data() + size * 1, (int)size, nullptr);
			if (!r || !s)
			{
				if (r != nullptr)
					BN_free(r);
				if (s != nullptr)
					BN_free(s);
				return crypto_exception();
			}

			ECDSA_SIG* ec_signature = ECDSA_SIG_new();
			if (!ec_signature)
			{
				BN_free(r);
				BN_free(s);
				return crypto_exception();
			}

			bool success = ECDSA_SIG_set0(ec_signature, r, s) == 1;
			BN_free(r);
			BN_free(s);
			if (!success)
			{
				ECDSA_SIG_free(ec_signature);
				return crypto_exception();
			}

			uint8_t* data = nullptr;
			int data_size = i2d_ECDSA_SIG(ec_signature, &data);
			ECDSA_SIG_free(ec_signature);
			if (data_size <= 0)
			{
				OPENSSL_free(data);
				return crypto_exception();
			}

			core::string result = core::string((char*)data, (size_t)data_size);
			OPENSSL_free(data);
			return expects_crypto<core::string>(std::move(result));
#else
			return crypto_exception();
#endif
		}
		expects_crypto<core::string> crypto::hmac(digest type, const std::string_view& value, const secret_box& key)
		{
			VI_ASSERT(type != nullptr, "type should be set");
#ifdef VI_OPENSSL
			VI_TRACE("crypto hmac-%s sign %" PRIu64 " bytes", get_digest_name(type).data(), (uint64_t)value.size());
			if (value.empty())
				return crypto_exception(-1, "hmac:empty");

			auto local_key = key.expose<core::CHUNK_SIZE>();
#if OPENSSL_VERSION_MAJOR >= 3
			uint8_t result[EVP_MAX_MD_SIZE];
			uint32_t size = sizeof(result);
			uint8_t* pointer = ::HMAC((const EVP_MD*)type, local_key.view.data(), (int)local_key.view.size(), (const uint8_t*)value.data(), value.size(), result, &size);

			if (!pointer)
				return crypto_exception();

			return core::string((const char*)result, size);
#elif OPENSSL_VERSION_NUMBER >= 0x1010000fL
			VI_TRACE("crypto hmac-%s sign %" PRIu64 " bytes", get_digest_name(type).data(), (uint64_t)value.size());
			HMAC_CTX* context = HMAC_CTX_new();
			if (!context)
				return crypto_exception();

			uint8_t result[EVP_MAX_MD_SIZE];
			if (1 != HMAC_Init_ex(context, local_key.view.data(), (int)local_key.view.size(), (const EVP_MD*)type, nullptr))
			{
				HMAC_CTX_free(context);
				return crypto_exception();
			}

			if (1 != HMAC_Update(context, (const uint8_t*)value.data(), (int)value.size()))
			{
				HMAC_CTX_free(context);
				return crypto_exception();
			}

			uint32_t size = sizeof(result);
			if (1 != HMAC_Final(context, result, &size))
			{
				HMAC_CTX_free(context);
				return crypto_exception();
			}

			core::string output((const char*)result, size);
			HMAC_CTX_free(context);

			return output;
#else
			VI_TRACE("crypto hmac-%s sign %" PRIu64 " bytes", get_digest_name(type).data(), (uint64_t)value.size());
			HMAC_CTX context;
			HMAC_CTX_init(&context);

			uint8_t result[EVP_MAX_MD_SIZE];
			if (1 != HMAC_Init_ex(&context, local_key.view.data(), (int)local_key.view.size(), (const EVP_MD*)type, nullptr))
			{
				HMAC_CTX_cleanup(&context);
				return crypto_exception();
			}

			if (1 != HMAC_Update(&context, (const uint8_t*)value.data(), (int)value.size()))
			{
				HMAC_CTX_cleanup(&context);
				return crypto_exception();
			}

			uint32_t size = sizeof(result);
			if (1 != HMAC_Final(&context, result, &size))
			{
				HMAC_CTX_cleanup(&context);
				return crypto_exception();
			}

			core::string output((const char*)result, size);
			HMAC_CTX_cleanup(&context);

			return output;
#endif
#else
			return crypto_exception();
#endif
		}
		expects_crypto<core::string> crypto::encrypt(cipher type, const std::string_view& value, const secret_box& key, const secret_box& salt, int complexity_bytes)
		{
			VI_ASSERT(complexity_bytes < 0 || (complexity_bytes > 0 && complexity_bytes % 2 == 0), "compexity should be valid 64, 128, 256, etc.");
			VI_ASSERT(type != nullptr, "type should be set");
			VI_TRACE("crypto %s encrypt-%i %" PRIu64 " bytes", get_cipher_name(type).data(), complexity_bytes, (uint64_t)value.size());
			if (value.empty())
				return crypto_exception(-1, "encrypt:empty");
#ifdef VI_OPENSSL
			EVP_CIPHER_CTX* context = EVP_CIPHER_CTX_new();
			if (!context)
				return crypto_exception();

			auto local_key = key.expose<core::CHUNK_SIZE>();
			if (complexity_bytes > 0)
			{
				if (1 != EVP_EncryptInit_ex(context, (const EVP_CIPHER*)type, nullptr, nullptr, nullptr) || 1 != EVP_CIPHER_CTX_set_key_length(context, complexity_bytes))
				{
					EVP_CIPHER_CTX_free(context);
					return crypto_exception();
				}
			}

			auto local_salt = salt.expose<core::CHUNK_SIZE>();
			if (1 != EVP_EncryptInit_ex(context, (const EVP_CIPHER*)type, nullptr, (const uint8_t*)local_key.view.data(), (const uint8_t*)local_salt.view.data()))
			{
				EVP_CIPHER_CTX_free(context);
				return crypto_exception();
			}

			core::string output;
			output.reserve(value.size());

			size_t offset = 0; bool is_finalized = false;
			uint8_t out_buffer[core::BLOB_SIZE + 2048];
			const uint8_t* in_buffer = (const uint8_t*)value.data();
			while (offset < value.size())
			{
				int in_size = std::min<int>(core::BLOB_SIZE, (int)(value.size() - offset)), out_size = 0;
				if (1 != EVP_EncryptUpdate(context, out_buffer, &out_size, in_buffer + offset, in_size))
				{
					EVP_CIPHER_CTX_free(context);
					return crypto_exception();
				}

			finalize:
				size_t output_offset = output.size();
				size_t out_buffer_size = (size_t)out_size;
				output.resize(output.size() + out_buffer_size);
				memcpy((char*)output.data() + output_offset, out_buffer, out_buffer_size);
				offset += (size_t)in_size;
				if (offset < value.size())
					continue;
				else if (is_finalized)
					break;

				if (1 != EVP_EncryptFinal_ex(context, out_buffer, &out_size))
				{
					EVP_CIPHER_CTX_free(context);
					return crypto_exception();
				}

				is_finalized = true;
				goto finalize;
			}

			uint8_t tag[16] = { 0 }, null[16] = { 0 };
			if (1 == EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_AEAD_GET_TAG, (int)sizeof(tag), tag) || 1 == EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_GET_TAG, (int)sizeof(tag), tag) || 1 == EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_CCM_GET_TAG, (int)sizeof(tag), tag))
			{
				if (memcmp(tag, null, sizeof(null)) != 0)
				{
					size_t output_size = output.size();
					output.resize(output_size + sizeof(tag));
					memcpy(output.data() + output_size, tag, sizeof(tag));
				}
			}

			EVP_CIPHER_CTX_free(context);
			return output;
#else
			return crypto_exception();
#endif
		}
		expects_crypto<core::string> crypto::decrypt(cipher type, const std::string_view& value, const secret_box& key, const secret_box& salt, int complexity_bytes)
		{
			VI_ASSERT(complexity_bytes < 0 || (complexity_bytes > 0 && complexity_bytes % 2 == 0), "compexity should be valid 64, 128, 256, etc.");
			VI_ASSERT(type != nullptr, "type should be set");
			VI_TRACE("crypto %s decrypt-%i %" PRIu64 " bytes", get_cipher_name(type).data(), complexity_bytes, (uint64_t)value.size());
			if (value.empty())
				return crypto_exception(-1, "decrypt:empty");
#ifdef VI_OPENSSL
			EVP_CIPHER_CTX* context = EVP_CIPHER_CTX_new();
			if (!context)
				return crypto_exception();

			auto local_key = key.expose<core::CHUNK_SIZE>();
			if (complexity_bytes > 0)
			{
				if (1 != EVP_EncryptInit_ex(context, (const EVP_CIPHER*)type, nullptr, nullptr, nullptr) || 1 != EVP_CIPHER_CTX_set_key_length(context, complexity_bytes))
				{
					EVP_CIPHER_CTX_free(context);
					return crypto_exception();
				}
			}

			auto local_salt = salt.expose<core::CHUNK_SIZE>();
			if (1 != EVP_DecryptInit_ex(context, (const EVP_CIPHER*)type, nullptr, (const uint8_t*)local_key.view.data(), (const uint8_t*)local_salt.view.data()))
			{
				EVP_CIPHER_CTX_free(context);
				return crypto_exception();
			}

			uint8_t tag[16]; auto untagged_value = value;
			if (untagged_value.size() > sizeof(tag))
			{
				switch (EVP_CIPHER_nid((const EVP_CIPHER*)type))
				{
					case NID_aes_128_gcm:
					case NID_aes_192_gcm:
					case NID_aes_256_gcm:
					case NID_camellia_128_gcm:
					case NID_camellia_192_gcm:
					case NID_camellia_256_gcm:
					case NID_aria_128_gcm:
					case NID_aria_192_gcm:
					case NID_aria_256_gcm:
					case NID_aes_128_ccm:
					case NID_aes_192_ccm:
					case NID_aes_256_ccm:
					case NID_camellia_128_ccm:
					case NID_camellia_192_ccm:
					case NID_camellia_256_ccm:
					case NID_aria_128_ccm:
					case NID_aria_192_ccm:
					case NID_aria_256_ccm:
					case NID_sm4_ccm:
					case NID_chacha20_poly1305:
					{
						size_t tag_offset = untagged_value.size() - sizeof(tag);
						memcpy(tag, untagged_value.data() + tag_offset, sizeof(tag));
						if (1 == EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_AEAD_SET_TAG, (int)sizeof(tag), tag) || 1 == EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_TAG, (int)sizeof(tag), tag) || 1 == EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_CCM_SET_TAG, (int)sizeof(tag), tag))
							untagged_value = untagged_value.substr(0, tag_offset);
						break;
					}
					default:
						break;
				}
			}

			core::string output;
			output.reserve(untagged_value.size());

			size_t offset = 0; bool is_finalized = false;
			uint8_t out_buffer[core::BLOB_SIZE + 2048];
			const uint8_t* in_buffer = (const uint8_t*)untagged_value.data();
			while (offset < untagged_value.size())
			{
				int in_size = std::min<int>(core::BLOB_SIZE, (int)(untagged_value.size() - offset)), out_size = 0;
				if (1 != EVP_DecryptUpdate(context, out_buffer, &out_size, in_buffer + offset, in_size))
				{
					EVP_CIPHER_CTX_free(context);
					return crypto_exception();
				}

			finalize:
				size_t output_offset = output.size();
				size_t out_buffer_size = (size_t)out_size;
				output.resize(output.size() + out_buffer_size);
				memcpy((char*)output.data() + output_offset, out_buffer, out_buffer_size);
				offset += (size_t)in_size;
				if (offset < untagged_value.size())
					continue;
				else if (is_finalized)
					break;

				if (1 != EVP_DecryptFinal_ex(context, out_buffer, &out_size))
				{
					EVP_CIPHER_CTX_free(context);
					return crypto_exception();
				}

				is_finalized = true;
				goto finalize;
			}

			EVP_CIPHER_CTX_free(context);
			return output;
#else
			return crypto_exception();
#endif
		}
		expects_crypto<core::string> crypto::jwt_sign(const std::string_view& alg, const std::string_view& payload, const secret_box& key)
		{
			digest hash = nullptr;
			if (alg == "HS256")
				hash = digests::sha256();
			else if (alg == "HS384")
				hash = digests::sha384();
			else if (alg == "HS512")
				hash = digests::sha512();

			return crypto::hmac(hash, payload, key);
		}
		expects_crypto<core::string> crypto::jwt_encode(web_token* src, const secret_box& key)
		{
			VI_ASSERT(src != nullptr, "web token should be set");
			VI_ASSERT(src->header != nullptr, "web token header should be set");
			VI_ASSERT(src->payload != nullptr, "web token payload should be set");
			core::string alg = src->header->get_var("alg").get_blob();
			if (alg.empty())
				return crypto_exception(-1, "jwt:algorithm_error");

			core::string header;
			core::schema::convert_to_json(src->header, [&header](core::var_form, const std::string_view& buffer) { header.append(buffer); });

			core::string payload;
			core::schema::convert_to_json(src->payload, [&payload](core::var_form, const std::string_view& buffer) { payload.append(buffer); });

			core::string data = codec::base64_url_encode(header) + '.' + codec::base64_url_encode(payload);
			auto signature = jwt_sign(alg, data, key);
			if (!signature)
				return signature;

			src->signature = *signature;
			return data + '.' + codec::base64_url_encode(src->signature);
		}
		expects_crypto<web_token*> crypto::jwt_decode(const std::string_view& value, const secret_box& key)
		{
			core::vector<core::string> source = core::stringify::split(value, '.');
			if (source.size() != 3)
				return crypto_exception(-1, "jwt:format_error");

			size_t offset = source[0].size() + source[1].size() + 1;
			source[0] = codec::base64_url_decode(source[0]);
			core::uptr<core::schema> header = core::schema::convert_from_json(source[0]).or_else(nullptr);
			if (!header)
				return crypto_exception(-1, "jwt:header_parser_error");

			source[1] = codec::base64_url_decode(source[1]);
			core::uptr<core::schema> payload = core::schema::convert_from_json(source[1]).or_else(nullptr);
			if (!payload)
				return crypto_exception(-1, "jwt:payload_parser_error");

			source[0] = header->get_var("alg").get_blob();
			auto signature = jwt_sign(source[0], value.substr(0, offset), key);
			if (!signature || codec::base64_url_encode(*signature) != source[2])
				return crypto_exception(-1, "jwt:signature_error");

			web_token* result = new web_token();
			result->signature = codec::base64_url_decode(source[2]);
			result->header = header.reset();
			result->payload = payload.reset();
			return result;
		}
		expects_crypto<core::string> crypto::schema_encrypt(core::schema* src, const secret_box& key, const secret_box& salt)
		{
			VI_ASSERT(src != nullptr, "schema should be set");
			core::string result;
			core::schema::convert_to_json(src, [&result](core::var_form, const std::string_view& buffer) { result.append(buffer); });

			auto data = encrypt(ciphers::aes_256_cbc(), result, key, salt);
			if (!data)
				return data;

			result = codec::bep45_encode(*data);
			return result;
		}
		expects_crypto<core::schema*> crypto::schema_decrypt(const std::string_view& value, const secret_box& key, const secret_box& salt)
		{
			VI_ASSERT(!value.empty(), "value should not be empty");
			if (value.empty())
				return crypto_exception(-1, "doc:payload_empty");

			auto source = decrypt(ciphers::aes_256_cbc(), codec::bep45_decode(value), key, salt);
			if (!source)
				return source.error();

			auto result = core::schema::convert_from_json(*source);
			if (!result)
				return crypto_exception(-1, "doc:payload_parser_error");

			return *result;
		}
		expects_crypto<size_t> crypto::file_encrypt(cipher type, core::stream* from, core::stream* to, const secret_box& key, const secret_box& salt, block_callback&& callback, size_t read_interval, int complexity_bytes)
		{
			VI_ASSERT(complexity_bytes < 0 || (complexity_bytes > 0 && complexity_bytes % 2 == 0), "compexity should be valid 64, 128, 256, etc.");
			VI_ASSERT(read_interval > 0, "read interval should be greater than zero.");
			VI_ASSERT(from != nullptr, "from stream should be set");
			VI_ASSERT(to != nullptr, "to stream should be set");
			VI_ASSERT(type != nullptr, "type should be set");
			VI_TRACE("crypto %s stream-encrypt-%i from fd %i to fd %i", get_cipher_name(type).data(), complexity_bytes, (int)from->get_readable_fd(), (int)to->get_writeable_fd());
#ifdef VI_OPENSSL
			EVP_CIPHER_CTX* context = EVP_CIPHER_CTX_new();
			if (!context)
				return crypto_exception();

			auto local_key = key.expose<core::CHUNK_SIZE>();
			if (complexity_bytes > 0)
			{
				if (1 != EVP_EncryptInit_ex(context, (const EVP_CIPHER*)type, nullptr, nullptr, nullptr) || 1 != EVP_CIPHER_CTX_set_key_length(context, complexity_bytes))
				{
					EVP_CIPHER_CTX_free(context);
					return crypto_exception();
				}
			}

			auto local_salt = salt.expose<core::CHUNK_SIZE>();
			if (1 != EVP_EncryptInit_ex(context, (const EVP_CIPHER*)type, nullptr, (const uint8_t*)local_key.view.data(), (const uint8_t*)local_salt.view.data()))
			{
				EVP_CIPHER_CTX_free(context);
				return crypto_exception();
			}

			size_t size = 0, in_buffer_size = 0, trailing_buffer_size = 0; bool is_finalized = false;
			uint8_t in_buffer[core::CHUNK_SIZE], out_buffer[core::CHUNK_SIZE + 1024], trailing_buffer[core::CHUNK_SIZE];
			while ((in_buffer_size = from->read(in_buffer, sizeof(in_buffer)).or_else(0)) > 0)
			{
				int out_size = 0;
				if (1 != EVP_EncryptUpdate(context, out_buffer + trailing_buffer_size, &out_size, in_buffer, (int)in_buffer_size))
				{
					EVP_CIPHER_CTX_free(context);
					return crypto_exception();
				}

			finalize:
				uint8_t* write_buffer = out_buffer;
				size_t write_buffer_size = (size_t)out_size + trailing_buffer_size;
				size_t trailing_offset = write_buffer_size % read_interval;
				memcpy(write_buffer, trailing_buffer, trailing_buffer_size);
				if (trailing_offset > 0 && !is_finalized)
				{
					size_t offset = write_buffer_size - trailing_offset;
					memcpy(trailing_buffer, write_buffer + offset, trailing_offset);
					trailing_buffer_size = trailing_offset;
					write_buffer_size -= trailing_offset;
				}
				else
					trailing_buffer_size = 0;

				if (callback && write_buffer_size > 0)
					callback(&write_buffer, &write_buffer_size);

				if (to->write(write_buffer, write_buffer_size).or_else(0) != write_buffer_size)
				{
					EVP_CIPHER_CTX_free(context);
					return crypto_exception();
				}

				size += write_buffer_size;
				if (in_buffer_size >= sizeof(in_buffer))
					continue;
				else if (is_finalized)
					break;

				if (1 != EVP_EncryptFinal_ex(context, out_buffer + trailing_buffer_size, &out_size))
				{
					EVP_CIPHER_CTX_free(context);
					return crypto_exception();
				}

				is_finalized = true;
				goto finalize;
			}

			EVP_CIPHER_CTX_free(context);
			return size;
#else
			return crypto_exception();
#endif
		}
		expects_crypto<size_t> crypto::file_decrypt(cipher type, core::stream* from, core::stream* to, const secret_box& key, const secret_box& salt, block_callback&& callback, size_t read_interval, int complexity_bytes)
		{
			VI_ASSERT(complexity_bytes < 0 || (complexity_bytes > 0 && complexity_bytes % 2 == 0), "compexity should be valid 64, 128, 256, etc.");
			VI_ASSERT(read_interval > 0, "read interval should be greater than zero.");
			VI_ASSERT(from != nullptr, "from stream should be set");
			VI_ASSERT(to != nullptr, "to stream should be set");
			VI_ASSERT(type != nullptr, "type should be set");
			VI_TRACE("crypto %s stream-decrypt-%i from fd %i to fd %i", get_cipher_name(type).data(), complexity_bytes, (int)from->get_readable_fd(), (int)to->get_writeable_fd());
#ifdef VI_OPENSSL
			EVP_CIPHER_CTX* context = EVP_CIPHER_CTX_new();
			if (!context)
				return crypto_exception();

			auto local_key = key.expose<core::CHUNK_SIZE>();
			if (complexity_bytes > 0)
			{
				if (1 != EVP_EncryptInit_ex(context, (const EVP_CIPHER*)type, nullptr, nullptr, nullptr) || 1 != EVP_CIPHER_CTX_set_key_length(context, complexity_bytes))
				{
					EVP_CIPHER_CTX_free(context);
					return crypto_exception();
				}
			}

			auto local_salt = salt.expose<core::CHUNK_SIZE>();
			if (1 != EVP_DecryptInit_ex(context, (const EVP_CIPHER*)type, nullptr, (const uint8_t*)local_key.view.data(), (const uint8_t*)local_salt.view.data()))
			{
				EVP_CIPHER_CTX_free(context);
				return crypto_exception();
			}

			size_t size = 0, in_buffer_size = 0, trailing_buffer_size = 0; bool is_finalized = false;
			uint8_t in_buffer[core::CHUNK_SIZE + 1024], out_buffer[core::CHUNK_SIZE + 1024], trailing_buffer[core::CHUNK_SIZE];
			while ((in_buffer_size = from->read(in_buffer + trailing_buffer_size, sizeof(trailing_buffer)).or_else(0)) > 0)
			{
				uint8_t* read_buffer = in_buffer;
				size_t read_buffer_size = (size_t)in_buffer_size + trailing_buffer_size;
				size_t trailing_offset = read_buffer_size % read_interval;
				memcpy(read_buffer, trailing_buffer, trailing_buffer_size);
				if (trailing_offset > 0 && !is_finalized)
				{
					size_t offset = read_buffer_size - trailing_offset;
					memcpy(trailing_buffer, read_buffer + offset, trailing_offset);
					trailing_buffer_size = trailing_offset;
					read_buffer_size -= trailing_offset;
				}
				else
					trailing_buffer_size = 0;

				if (callback && read_buffer_size > 0)
					callback(&read_buffer, &read_buffer_size);

				int out_size = 0;
				if (1 != EVP_DecryptUpdate(context, out_buffer, &out_size, (uint8_t*)read_buffer, (int)read_buffer_size))
				{
					EVP_CIPHER_CTX_free(context);
					return crypto_exception();
				}

			finalize:
				size_t out_buffer_size = (size_t)out_size;
				if (to->write(out_buffer, out_buffer_size).or_else(0) != out_buffer_size)
				{
					EVP_CIPHER_CTX_free(context);
					return crypto_exception();
				}

				size += out_buffer_size;
				if (in_buffer_size >= sizeof(trailing_buffer))
					continue;
				else if (is_finalized)
					break;

				if (1 != EVP_DecryptFinal_ex(context, out_buffer, &out_size))
				{
					display_crypto_log();
					EVP_CIPHER_CTX_free(context);
					return crypto_exception();
				}

				is_finalized = true;
				goto finalize;
			}

			EVP_CIPHER_CTX_free(context);
			return size;
#else
			return crypto_exception();
#endif
		}
		uint8_t crypto::random_uc()
		{
			static const char alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
			return alphabet[(size_t)(random() % (sizeof(alphabet) - 1))];
		}
		uint32_t crypto::crc32(const std::string_view& data)
		{
			VI_TRACE("crypto crc32 %" PRIu64 " bytes", (uint64_t)data.size());
			uint32_t crc = ~0;
			for (auto byte : data)
			{
				crc ^= static_cast<uint32_t>(byte);
				for (int i = 0; i < 8; i++)
				{
					uint32_t t = ~((crc & 1) - 1);
					crc = (crc >> 1) ^ (0xEDB88320 & t);
				}
			}
			return ~crc;
		}
		uint64_t crypto::crc64(const std::string_view& data)
		{
			VI_TRACE("crypto crc64 %" PRIu64 " bytes", (uint64_t)data.size());
			uint64_t crc = ~0;
			for (auto byte : data)
			{
				crc ^= static_cast<uint64_t>(byte);
				for (int i = 0; i < 8; i++)
				{
					uint64_t t = ~((crc & 1) - 1);
					crc = (crc >> 1) ^ (0xC96C5795D7870F42ULL & t);
				}
			}
			return ~crc;
		}
		uint64_t crypto::random(uint64_t min, uint64_t max)
		{
			uint64_t raw = 0;
			if (min > max)
				return raw;
#ifdef VI_OPENSSL
			if (RAND_bytes((uint8_t*)&raw, sizeof(uint64_t)) != 1)
			{
				ERR_clear_error();
				raw = random();
			}
#else
			raw = random();
#endif
			return raw % (max - min + 1) + min;
		}
		uint64_t crypto::random()
		{
			static std::random_device device;
			static std::mt19937 engine(device());
			static std::uniform_int_distribution<uint64_t> range;

			return range(engine);
		}
		void crypto::sha1_collapse_buffer_block(uint32_t* buffer)
		{
			VI_ASSERT(buffer != nullptr, "buffer should be set");
			for (int i = 16; --i >= 0;)
				buffer[i] = 0;
		}
		void crypto::sha1_compute_hash_block(uint32_t* result, uint32_t* w)
		{
			VI_ASSERT(result != nullptr, "result should be set");
			VI_ASSERT(w != nullptr, "salt should be set");
			uint32_t a = result[0];
			uint32_t b = result[1];
			uint32_t c = result[2];
			uint32_t d = result[3];
			uint32_t e = result[4];
			int r = 0;

#define sha1_roll(A1, A2) (((A1) << (A2)) | ((A1) >> (32 - (A2))))
#define sha1_make(f, v) {const uint32_t t = sha1_roll(a, 5) + (f) + e + v + w[r]; e = d; d = c; c = sha1_roll(b, 30); b = a; a = t; r++;}

			while (r < 16)
				sha1_make((b & c) | (~b & d), 0x5a827999);

			while (r < 20)
			{
				w[r] = sha1_roll((w[r - 3] ^ w[r - 8] ^ w[r - 14] ^ w[r - 16]), 1);
				sha1_make((b & c) | (~b & d), 0x5a827999);
			}

			while (r < 40)
			{
				w[r] = sha1_roll((w[r - 3] ^ w[r - 8] ^ w[r - 14] ^ w[r - 16]), 1);
				sha1_make(b ^ c ^ d, 0x6ed9eba1);
			}

			while (r < 60)
			{
				w[r] = sha1_roll((w[r - 3] ^ w[r - 8] ^ w[r - 14] ^ w[r - 16]), 1);
				sha1_make((b & c) | (b & d) | (c & d), 0x8f1bbcdc);
			}

			while (r < 80)
			{
				w[r] = sha1_roll((w[r - 3] ^ w[r - 8] ^ w[r - 14] ^ w[r - 16]), 1);
				sha1_make(b ^ c ^ d, 0xca62c1d6);
			}

#undef sha1_roll
#undef sha1_make

			result[0] += a;
			result[1] += b;
			result[2] += c;
			result[3] += d;
			result[4] += e;
		}
		void crypto::sha1_compute(const void* value, const int length, char* hash20)
		{
			VI_ASSERT(value != nullptr, "value should be set");
			VI_ASSERT(hash20 != nullptr, "hash of size 20 should be set");

			uint32_t result[5] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0 };
			const uint8_t* value_cuc = (const uint8_t*)value;
			uint32_t w[80];

			const int end_of_full_blocks = length - 64;
			int end_current_block, current_block = 0;

			while (current_block <= end_of_full_blocks)
			{
				end_current_block = current_block + 64;
				for (int i = 0; current_block < end_current_block; current_block += 4)
					w[i++] = (uint32_t)value_cuc[current_block + 3] | (((uint32_t)value_cuc[current_block + 2]) << 8) | (((uint32_t)value_cuc[current_block + 1]) << 16) | (((uint32_t)value_cuc[current_block]) << 24);
				sha1_compute_hash_block(result, w);
			}

			end_current_block = length - current_block;
			sha1_collapse_buffer_block(w);

			int last_block_bytes = 0;
			for (; last_block_bytes < end_current_block; last_block_bytes++)
				w[last_block_bytes >> 2] |= (uint32_t)value_cuc[last_block_bytes + current_block] << ((3 - (last_block_bytes & 3)) << 3);

			w[last_block_bytes >> 2] |= 0x80 << ((3 - (last_block_bytes & 3)) << 3);
			if (end_current_block >= 56)
			{
				sha1_compute_hash_block(result, w);
				sha1_collapse_buffer_block(w);
			}

			w[15] = length << 3;
			sha1_compute_hash_block(result, w);

			for (int i = 20; --i >= 0;)
				hash20[i] = (result[i >> 2] >> (((3 - i) & 0x3) << 3)) & 0xff;
		}
		void crypto::sha1_hash20_to_hex(const char* hash20, char* hex_string)
		{
			VI_ASSERT(hash20 != nullptr, "hash of size 20 should be set");
			VI_ASSERT(hex_string != nullptr, "result hex should be set");

			const char hex[] = { "0123456789abcdef" };
			for (int i = 20; --i >= 0;)
			{
				hex_string[i << 1] = hex[(hash20[i] >> 4) & 0xf];
				hex_string[(i << 1) + 1] = hex[hash20[i] & 0xf];
			}

			hex_string[40] = 0;
		}
		void crypto::display_crypto_log()
		{
#ifdef VI_OPENSSL
			ERR_print_errors_cb([](const char* message, size_t size, void*)
			{
				while (size > 0 && core::stringify::is_whitespace(message[size - 1]))
					--size;
				VI_ERR("openssl %.*s", (int)size, message);
				return 0;
			}, nullptr);
#endif
		}

		void codec::rotate_buffer(uint8_t* buffer, size_t buffer_size, uint64_t hash, int8_t direction)
		{
			core::string partition;
			core::key_hasher<core::string> hasher;
			partition.reserve(buffer_size);

			constexpr uint8_t limit = std::numeric_limits<uint8_t>::max() - 1;
			if (direction < 0)
			{
				core::vector<uint8_t> rotated_buffer(buffer, buffer + buffer_size);
				for (size_t i = 0; i < buffer_size; i++)
				{
					partition.assign((char*)rotated_buffer.data(), i + 1).back() = (char)(++hash % limit);
					uint8_t rotation = (uint8_t)(hasher(partition) % limit);
					buffer[i] -= rotation;
				}
			}
			else
			{
				for (size_t i = 0; i < buffer_size; i++)
				{
					partition.assign((char*)buffer, i + 1).back() = (char)(++hash % limit);
					uint8_t rotation = (uint8_t)(hasher(partition) % limit);
					buffer[i] += rotation;
				}
			}
		}
		core::string codec::rotate(const std::string_view& value, uint64_t hash, int8_t direction)
		{
			core::string result = core::string(value);
			rotate_buffer((uint8_t*)result.data(), result.size(), hash, direction);
			return result;
		}
		core::string codec::encode64(const char alphabet[65], const uint8_t* value, size_t length, bool padding)
		{
			VI_ASSERT(value != nullptr, "value should be set");
			VI_ASSERT(length > 0, "length should be greater than zero");
			VI_TRACE("codec %s encode-64 %" PRIu64 " bytes", padding ? "padded" : "unpadded", (uint64_t)length);

			core::string result;
			uint8_t row3[3];
			uint8_t row4[4];
			uint32_t offset = 0, step = 0;

			while (length--)
			{
				row3[offset++] = *(value++);
				if (offset != 3)
					continue;

				row4[0] = (row3[0] & 0xfc) >> 2;
				row4[1] = ((row3[0] & 0x03) << 4) + ((row3[1] & 0xf0) >> 4);
				row4[2] = ((row3[1] & 0x0f) << 2) + ((row3[2] & 0xc0) >> 6);
				row4[3] = row3[2] & 0x3f;

				for (offset = 0; offset < 4; offset++)
					result += alphabet[row4[offset]];

				offset = 0;
			}

			if (!offset)
				return result;

			for (step = offset; step < 3; step++)
				row3[step] = '\0';

			row4[0] = (row3[0] & 0xfc) >> 2;
			row4[1] = ((row3[0] & 0x03) << 4) + ((row3[1] & 0xf0) >> 4);
			row4[2] = ((row3[1] & 0x0f) << 2) + ((row3[2] & 0xc0) >> 6);
			row4[3] = row3[2] & 0x3f;

			for (step = 0; (step < offset + 1); step++)
				result += alphabet[row4[step]];

			if (!padding)
				return result;

			while (offset++ < 3)
				result += '=';

			return result;
		}
		core::string codec::decode64(const char alphabet[65], const uint8_t* value, size_t length, bool(*is_alphabetic)(uint8_t))
		{
			VI_ASSERT(value != nullptr, "value should be set");
			VI_ASSERT(is_alphabetic != nullptr, "callback should be set");
			VI_ASSERT(length > 0, "length should be greater than zero");
			VI_TRACE("codec decode-64 %" PRIu64 " bytes", (uint64_t)length);

			core::string result;
			uint8_t row4[4];
			uint8_t row3[3];
			uint32_t offset = 0, step = 0;
			uint32_t focus = 0;

			while (length-- && (value[focus] != '=') && is_alphabetic(value[focus]))
			{
				row4[offset++] = value[focus]; focus++;
				if (offset != 4)
					continue;

				for (offset = 0; offset < 4; offset++)
					row4[offset] = (uint8_t)offset_of64(alphabet, row4[offset]);

				row3[0] = (row4[0] << 2) + ((row4[1] & 0x30) >> 4);
				row3[1] = ((row4[1] & 0xf) << 4) + ((row4[2] & 0x3c) >> 2);
				row3[2] = ((row4[2] & 0x3) << 6) + row4[3];

				for (offset = 0; (offset < 3); offset++)
					result += row3[offset];

				offset = 0;
			}

			if (!offset)
				return result;

			for (step = offset; step < 4; step++)
				row4[step] = 0;

			for (step = 0; step < 4; step++)
				row4[step] = (uint8_t)offset_of64(alphabet, row4[step]);

			row3[0] = (row4[0] << 2) + ((row4[1] & 0x30) >> 4);
			row3[1] = ((row4[1] & 0xf) << 4) + ((row4[2] & 0x3c) >> 2);
			row3[2] = ((row4[2] & 0x3) << 6) + row4[3];

			for (step = 0; (step < offset - 1); step++)
				result += row3[step];

			return result;
		}
		core::string codec::bep45_encode(const std::string_view& data)
		{
			static const char from[] = " $%*+-./:";
			static const char to[] = "abcdefghi";

			core::string result(base45_encode(data));
			for (size_t i = 0; i < sizeof(from) - 1; i++)
				core::stringify::replace(result, from[i], to[i]);

			return result;
		}
		core::string codec::bep45_decode(const std::string_view& data)
		{
			static const char from[] = "abcdefghi";
			static const char to[] = " $%*+-./:";

			core::string result(data);
			for (size_t i = 0; i < sizeof(from) - 1; i++)
				core::stringify::replace(result, from[i], to[i]);

			return base45_decode(result);
		}
		core::string codec::base32_encode(const std::string_view& value)
		{
			static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
			core::string result;
			if (value.empty())
				return result;

			size_t next = (size_t)value[0], offset = 1, remainder = 8;
			result.reserve((size_t)((double)value.size() * 1.6));

			while (remainder > 0 || offset < value.size())
			{
				if (remainder < 5)
				{
					if (offset < value.size())
					{
						next <<= 8;
						next |= (size_t)value[offset++] & 0xFF;
						remainder += 8;
					}
					else
					{
						size_t padding = 5 - remainder;
						next <<= padding;
						remainder += padding;
					}
				}

				remainder -= 5;
				size_t index = 0x1F & (next >> remainder);
				result.push_back(alphabet[index % (sizeof(alphabet) - 1)]);
			}

			return result;
		}
		core::string codec::base32_decode(const std::string_view& value)
		{
			core::string result;
			result.reserve(value.size());

			size_t prev = 0, bits_left = 0;
			for (char next : value)
			{
				if (next == ' ' || next == '\t' || next == '\r' || next == '\n' || next == '-')
					continue;

				prev <<= 5;
				if (next == '0')
					next = 'O';
				else if (next == '1')
					next = 'L';
				else if (next == '8')
					next = 'B';

				if ((next >= 'A' && next <= 'Z') || (next >= 'a' && next <= 'z'))
					next = (next & 0x1F) - 1;
				else if (next >= '2' && next <= '7')
					next -= '2' - 26;
				else
					break;

				prev |= next;
				bits_left += 5;
				if (bits_left < 8)
					continue;

				result.push_back((char)(prev >> (bits_left - 8)));
				bits_left -= 8;
			}

			return result;
		}
		core::string codec::base45_encode(const std::string_view& data)
		{
			VI_TRACE("codec base45 encode %" PRIu64 " bytes", (uint64_t)data.size());
			static const char alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";
			core::string result;
			size_t size = data.size();
			result.reserve(size);

			for (size_t i = 0; i < size; i += 2)
			{
				if (size - i > 1)
				{
					int x = ((uint8_t)(data[i]) << 8) + (uint8_t)data[i + 1];
					uint8_t e = x / (45 * 45);
					x %= 45 * 45;

					uint8_t d = x / 45;
					uint8_t c = x % 45;
					result += alphabet[c];
					result += alphabet[d];
					result += alphabet[e];
				}
				else
				{
					int x = (uint8_t)data[i];
					uint8_t d = x / 45;
					uint8_t c = x % 45;

					result += alphabet[c];
					result += alphabet[d];
				}
			}

			return result;
		}
		core::string codec::base45_decode(const std::string_view& data)
		{
			VI_TRACE("codec base45 decode %" PRIu64 " bytes", (uint64_t)data.size());
			static uint8_t char_to_int[256] =
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

			size_t size = data.size();
			core::string result(size, ' ');
			size_t offset = 0;

			for (size_t i = 0; i < size; i += 3)
			{
				int x, a, b;
				if (size - i < 2)
					break;

				if ((255 == (a = (char)char_to_int[(uint8_t)data[i]])) || (255 == (b = (char)char_to_int[(uint8_t)data[i + 1]])))
					break;

				x = a + 45 * b;
				if (size - i >= 3)
				{
					if (255 == (a = (char)char_to_int[(uint8_t)data[i + 2]]))
						break;

					x += a * 45 * 45;
					result[offset++] = x / 256;
					x %= 256;
				}

				result[offset++] = x;
			}

			return core::string(result.c_str(), offset);
		}
		core::string codec::base64_encode(const std::string_view& value)
		{
			static const char set[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			return encode64(set, (uint8_t*)value.data(), value.size(), true);
		}
		core::string codec::base64_decode(const std::string_view& value)
		{
			static const char set[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			return decode64(set, (uint8_t*)value.data(), value.size(), is_base64);
		}
		core::string codec::base64_url_encode(const std::string_view& value)
		{
			static const char set[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
			return encode64(set, (uint8_t*)value.data(), value.size(), false);
		}
		core::string codec::base64_url_decode(const std::string_view& value)
		{
			static const char set[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
			size_t padding = value.size() % 4;
			if (padding == 0)
				return decode64(set, (uint8_t*)value.data(), value.size(), is_base64_url);

			core::string padded = core::string(value);
			padded.append(4 - padding, '=');
			return decode64(set, (uint8_t*)padded.c_str(), padded.size(), is_base64_url);
		}
		core::string codec::shuffle(const char* value, size_t size, uint64_t mask)
		{
			VI_ASSERT(value != nullptr, "value should be set");
			VI_TRACE("codec shuffle %" PRIu64 " bytes", (uint64_t)size);

			core::string result;
			result.resize(size);

			int64_t hash = mask;
			for (size_t i = 0; i < size; i++)
			{
				hash = ((hash << 5) - hash) + value[i];
				hash = hash & hash;
				result[i] = (char)(hash % 255);
			}

			return result;
		}
		expects_compression<core::string> codec::compress(const std::string_view& data, compression type)
		{
#ifdef VI_ZLIB
			VI_TRACE("codec compress %" PRIu64 " bytes", (uint64_t)data.size());
			if (data.empty())
				return compression_exception(Z_DATA_ERROR, "empty input buffer");

			uLongf size = compressBound((uLong)data.size());
			Bytef* buffer = core::memory::allocate<Bytef>(size);
			int code = compress2(buffer, &size, (const Bytef*)data.data(), (uLong)data.size(), (int)type);
			if (code != Z_OK)
			{
				core::memory::deallocate(buffer);
				return compression_exception(code, "buffer compression using compress2 error");
			}

			core::string output((char*)buffer, (size_t)size);
			core::memory::deallocate(buffer);
			return output;
#else
			return compression_exception(-1, "unsupported");
#endif
		}
		expects_compression<core::string> codec::decompress(const std::string_view& data)
		{
#ifdef VI_ZLIB
			VI_TRACE("codec decompress %" PRIu64 " bytes", (uint64_t)data.size());
			if (data.empty())
				return compression_exception(Z_DATA_ERROR, "empty input buffer");

			uLongf source_size = (uLong)data.size();
			uLongf total_size = source_size * 2;
			while (true)
			{
				uLongf size = total_size, final_size = source_size;
				Bytef* buffer = core::memory::allocate<Bytef>(size);
				int code = uncompress2(buffer, &size, (const Bytef*)data.data(), &final_size);
				if (code == Z_MEM_ERROR || code == Z_BUF_ERROR)
				{
					core::memory::deallocate(buffer);
					total_size += source_size;
					continue;
				}
				else if (code != Z_OK)
				{
					core::memory::deallocate(buffer);
					return compression_exception(code, "buffer decompression using uncompress2 error");
				}

				core::string output((char*)buffer, (size_t)size);
				core::memory::deallocate(buffer);
				return output;
			}

			return compression_exception(Z_DATA_ERROR, "buffer decompression error");
#else
			return compression_exception(-1, "unsupported");
#endif
		}
		core::string codec::hex_encode_odd(const std::string_view& value, bool upper_case)
		{
			VI_TRACE("codec hex encode odd %" PRIu64 " bytes", (uint64_t)value.size());
			static const char hex_lower_case[17] = "0123456789abcdef";
			static const char hex_upper_case[17] = "0123456789ABCDEF";

			core::string output;
			output.reserve(value.size() * 2);

			const char* hex = upper_case ? hex_upper_case : hex_lower_case;
			for (size_t i = 0; i < value.size(); i++)
			{
				uint8_t c = static_cast<uint8_t>(value[i]);
				if (c >= 16)
					output += hex[c >> 4];
				output += hex[c & 0xf];
			}

			return output;
		}
		core::string codec::hex_encode(const std::string_view& value, bool upper_case)
		{
			VI_TRACE("codec hex encode %" PRIu64 " bytes", (uint64_t)value.size());
			static const char hex_lower_case[17] = "0123456789abcdef";
			static const char hex_upper_case[17] = "0123456789ABCDEF";
			const char* hex = upper_case ? hex_upper_case : hex_lower_case;

			core::string output;
			output.reserve(value.size() * 2);

			for (size_t i = 0; i < value.size(); i++)
			{
				uint8_t c = static_cast<uint8_t>(value[i]);
				output += hex[c >> 4];
				output += hex[c & 0xf];
			}

			return output;
		}
		core::string codec::hex_decode(const std::string_view& value)
		{
			VI_TRACE("codec hex decode %" PRIu64 " bytes", (uint64_t)value.size());
			size_t offset = 0;
			if (value.size() >= 2 && value[0] == '0' && value[1] == 'x')
				offset = 2;

			core::string output;
			output.reserve(value.size() / 2);

			char hex[3] = { 0, 0, 0 };
			for (size_t i = offset; i < value.size(); i += 2)
			{
				size_t size = std::min<size_t>(2, value.size() - i);
				memcpy(hex, value.data() + i, sizeof(char) * size);
				output.push_back((char)(int)strtol(hex, nullptr, 16));
				hex[1] = 0;
			}

			return output;
		}
		core::string codec::url_encode(const std::string_view& text)
		{
			VI_TRACE("codec url encode %" PRIu64 " bytes", (uint64_t)text.size());
			static const char* unescape = "._-$,;~()";
			static const char* hex = "0123456789abcdef";

			core::string value;
			value.reserve(text.size());

			size_t offset = 0;
			while (offset < text.size())
			{
				uint8_t v = text[offset++];
				if (!isalnum(v) && !strchr(unescape, v))
				{
					value += '%';
					value += (hex[v >> 4]);
					value += (hex[v & 0xf]);
				}
				else
					value += v;
			}

			return value;
		}
		core::string codec::url_decode(const std::string_view& text)
		{
			VI_TRACE("codec url encode %" PRIu64 " bytes", (uint64_t)text.size());
			core::string value;
			uint8_t* data = (uint8_t*)text.data();
			size_t size = text.size();
			size_t i = 0;

			while (i < size)
			{
				if (size >= 2 && i < size - 2 && text[i] == '%' && isxdigit(*(data + i + 1)) && isxdigit(*(data + i + 2)))
				{
					int a = tolower(*(data + i + 1)), b = tolower(*(data + i + 2));
					value += (char)(((isdigit(a) ? (a - '0') : (a - 'W')) << 4) | (isdigit(b) ? (b - '0') : (b - 'W')));
					i += 2;
				}
				else if (text[i] != '+')
					value += text[i];
				else
					value += ' ';

				i++;
			}

			return value;
		}
		core::string codec::base10_to_base_n(uint64_t value, uint32_t base_less_than65)
		{
			VI_ASSERT(base_less_than65 >= 1 && base_less_than65 <= 64, "base should be between 1 and 64");
			static const char* base62 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/";
			static const char* base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			if (value == 0)
				return "0";

			const char* base = (base_less_than65 == 64 ? base64 : base62);
			core::string output;

			while (value > 0)
			{
				output.insert(0, 1, base[value % base_less_than65]);
				value /= base_less_than65;
			}

			return output;
		}
		core::string codec::decimal_to_hex(uint64_t n)
		{
			const char* set = "0123456789abcdef";
			core::string result;

			do
			{
				result = set[n & 15] + result;
				n >>= 4;
			} while (n > 0);

			return result;
		}
		size_t codec::utf8(int code, char* buffer)
		{
			VI_ASSERT(buffer != nullptr, "buffer should be set");
			if (code < 0x0080)
			{
				buffer[0] = (code & 0x7F);
				return 1;
			}
			else if (code < 0x0800)
			{
				buffer[0] = (0xC0 | ((code >> 6) & 0x1F));
				buffer[1] = (0x80 | (code & 0x3F));
				return 2;
			}
			else if (code < 0xD800)
			{
				buffer[0] = (0xE0 | ((code >> 12) & 0xF));
				buffer[1] = (0x80 | ((code >> 6) & 0x3F));
				buffer[2] = (0x80 | (code & 0x3F));
				return 3;
			}
			else if (code < 0xE000)
			{
				return 0;
			}
			else if (code < 0x10000)
			{
				buffer[0] = (0xE0 | ((code >> 12) & 0xF));
				buffer[1] = (0x80 | ((code >> 6) & 0x3F));
				buffer[2] = (0x80 | (code & 0x3F));
				return 3;
			}
			else if (code < 0x110000)
			{
				buffer[0] = (0xF0 | ((code >> 18) & 0x7));
				buffer[1] = (0x80 | ((code >> 12) & 0x3F));
				buffer[2] = (0x80 | ((code >> 6) & 0x3F));
				buffer[3] = (0x80 | (code & 0x3F));
				return 4;
			}

			return 0;
		}
		bool codec::hex(char code, int& value)
		{
			if (0x20 <= code && isdigit(code))
			{
				value = code - '0';
				return true;
			}
			else if ('A' <= code && code <= 'F')
			{
				value = code - 'A' + 10;
				return true;
			}
			else if ('a' <= code && code <= 'f')
			{
				value = code - 'a' + 10;
				return true;
			}

			return false;
		}
		bool codec::hex_to_string(const std::string_view& data, char* buffer, size_t buffer_length)
		{
			VI_ASSERT(buffer != nullptr && buffer_length > 0, "buffer should be set");
			VI_ASSERT(buffer_length >= (3 * data.size()), "buffer is too small");
			if (data.empty())
				return false;

			static const char HEX[] = "0123456789abcdef";
			uint8_t* value = (uint8_t*)data.data();
			for (size_t i = 0; i < data.size(); i++)
			{
				if (i > 0)
					buffer[3 * i - 1] = ' ';
				buffer[3 * i] = HEX[(value[i] >> 4) & 0xF];
				buffer[3 * i + 1] = HEX[value[i] & 0xF];
			}

			buffer[3 * data.size() - 1] = 0;
			return true;
		}
		bool codec::hex_to_decimal(const std::string_view& text, size_t index, size_t count, int& value)
		{
			VI_ASSERT(index < text.size(), "index outside of range");

			value = 0;
			for (; count; index++, count--)
			{
				if (!text[index])
					return false;

				int v = 0;
				if (!hex(text[index], v))
					return false;

				value = value * 16 + v;
			}

			return true;
		}
		bool codec::is_base64(uint8_t value)
		{
			return (isalnum(value) || (value == '+') || (value == '/'));
		}
		bool codec::is_base64_url(uint8_t value)
		{
			return (isalnum(value) || (value == '-') || (value == '_'));
		}

		web_token::web_token() noexcept : header(nullptr), payload(nullptr), token(nullptr)
		{
		}
		web_token::web_token(const std::string_view& issuer, const std::string_view& subject, int64_t expiration) noexcept : header(core::var::set::object()), payload(core::var::set::object()), token(nullptr)
		{
			header->set("alg", core::var::string("HS256"));
			header->set("typ", core::var::string("JWT"));
			payload->set("iss", core::var::string(issuer));
			payload->set("sub", core::var::string(subject));
			payload->set("exp", core::var::integer(expiration));
		}
		web_token::~web_token() noexcept
		{
			core::memory::release(header);
			core::memory::release(payload);
			core::memory::release(token);
		}
		void web_token::unsign()
		{
			signature.clear();
			data.clear();
		}
		void web_token::set_algorithm(const std::string_view& value)
		{
			if (!header)
				header = core::var::set::object();

			header->set("alg", core::var::string(value));
			unsign();
		}
		void web_token::set_type(const std::string_view& value)
		{
			if (!header)
				header = core::var::set::object();

			header->set("typ", core::var::string(value));
			unsign();
		}
		void web_token::set_content_type(const std::string_view& value)
		{
			if (!header)
				header = core::var::set::object();

			header->set("cty", core::var::string(value));
			unsign();
		}
		void web_token::set_issuer(const std::string_view& value)
		{
			if (!payload)
				payload = core::var::set::object();

			payload->set("iss", core::var::string(value));
			unsign();
		}
		void web_token::set_subject(const std::string_view& value)
		{
			if (!payload)
				payload = core::var::set::object();

			payload->set("sub", core::var::string(value));
			unsign();
		}
		void web_token::set_id(const std::string_view& value)
		{
			if (!payload)
				payload = core::var::set::object();

			payload->set("jti", core::var::string(value));
			unsign();
		}
		void web_token::set_audience(const core::vector<core::string>& value)
		{
			core::schema* array = core::var::set::array();
			for (auto& item : value)
				array->push(core::var::string(item));

			if (!payload)
				payload = core::var::set::object();

			payload->set("aud", array);
			unsign();
		}
		void web_token::set_expiration(int64_t value)
		{
			if (!payload)
				payload = core::var::set::object();

			payload->set("exp", core::var::integer(value));
			unsign();
		}
		void web_token::set_not_before(int64_t value)
		{
			if (!payload)
				payload = core::var::set::object();

			payload->set("nbf", core::var::integer(value));
			unsign();
		}
		void web_token::set_created(int64_t value)
		{
			if (!payload)
				payload = core::var::set::object();

			payload->set("iat", core::var::integer(value));
			unsign();
		}
		void web_token::set_refresh_token(const std::string_view& value, const secret_box& key, const secret_box& salt)
		{
			core::memory::release(token);
			auto new_token = crypto::schema_decrypt(value, key, salt);
			if (new_token)
			{
				token = *new_token;
				refresher.assign(value);
			}
			else
			{
				token = nullptr;
				refresher.clear();
			}
		}
		bool web_token::sign(const secret_box& key)
		{
			VI_ASSERT(header != nullptr, "header should be set");
			VI_ASSERT(payload != nullptr, "payload should be set");
			if (!data.empty())
				return true;

			auto new_data = crypto::jwt_encode(this, key);
			if (!new_data)
				return false;

			data = *new_data;
			return !data.empty();
		}
		expects_crypto<core::string> web_token::get_refresh_token(const secret_box& key, const secret_box& salt)
		{
			auto new_token = crypto::schema_encrypt(token, key, salt);
			if (!new_token)
				return new_token;

			refresher = *new_token;
			return refresher;
		}
		bool web_token::is_valid() const
		{
			if (!header || !payload || signature.empty())
				return false;

			int64_t expires = payload->get_var("exp").get_integer();
			return time(nullptr) < expires;
		}

		preprocessor::preprocessor() noexcept : nested(false)
		{
			core::vector<std::pair<core::string, condition>> controls =
			{
				std::make_pair(core::string("ifdef"), condition::exists),
				std::make_pair(core::string("ifndef"), condition::not_exists),
				std::make_pair(core::string("ifeq"), condition::equals),
				std::make_pair(core::string("ifneq"), condition::not_equals),
				std::make_pair(core::string("ifgt"), condition::greater),
				std::make_pair(core::string("ifngt"), condition::not_greater),
				std::make_pair(core::string("ifgte"), condition::greater_equals),
				std::make_pair(core::string("ifngte"), condition::not_greater_equals),
				std::make_pair(core::string("iflt"), condition::less),
				std::make_pair(core::string("ifnlt"), condition::not_less),
				std::make_pair(core::string("iflte"), condition::less_equals),
				std::make_pair(core::string("ifnlte"), condition::not_less_equals),
				std::make_pair(core::string("iflte"), condition::less_equals),
				std::make_pair(core::string("ifnlte"), condition::not_less_equals)
			};

			control_flow.reserve(controls.size() * 2 + 2);
			control_flow["else"] = std::make_pair(condition::exists, controller::else_case);
			control_flow["endif"] = std::make_pair(condition::not_exists, controller::end_if);

			for (auto& item : controls)
			{
				control_flow[item.first] = std::make_pair(item.second, controller::start_if);
				control_flow["el" + item.first] = std::make_pair(item.second, controller::else_if);
			}
		}
		void preprocessor::set_include_options(const include_desc& new_desc)
		{
			file_desc = new_desc;
		}
		void preprocessor::set_include_callback(proc_include_callback&& callback)
		{
			include = std::move(callback);
		}
		void preprocessor::set_pragma_callback(proc_pragma_callback&& callback)
		{
			pragma = std::move(callback);
		}
		void preprocessor::set_directive_callback(const std::string_view& name, proc_directive_callback&& callback)
		{
			auto it = directives.find(core::key_lookup_cast(name));
			if (it != directives.end())
			{
				if (callback)
					it->second = std::move(callback);
				else
					directives.erase(it);
			}
			else if (callback)
				directives[core::string(name)] = std::move(callback);
			else
				directives.erase(core::string(name));
		}
		void preprocessor::set_features(const desc& f)
		{
			features = f;
		}
		void preprocessor::add_default_definitions()
		{
			define_dynamic("__VERSION__", [](preprocessor*, const core::vector<core::string>& args) -> expects_preprocessor<core::string>
			{
				return core::to_string<size_t>(vitex::version);
			});
			define_dynamic("__DATE__", [](preprocessor*, const core::vector<core::string>& args) -> expects_preprocessor<core::string>
			{
				return escape_text(core::date_time::serialize_local(core::date_time::now(), "%b %d %Y"));
			});
			define_dynamic("__FILE__", [](preprocessor* base, const core::vector<core::string>& args) -> expects_preprocessor<core::string>
			{
				core::string path = base->get_current_file_path();
				return escape_text(core::stringify::replace(path, "\\", "\\\\"));
			});
			define_dynamic("__LINE__", [](preprocessor* base, const core::vector<core::string>& args) -> expects_preprocessor<core::string>
			{
				return core::to_string<size_t>(base->get_current_line_number());
			});
			define_dynamic("__DIRECTORY__", [](preprocessor* base, const core::vector<core::string>& args) -> expects_preprocessor<core::string>
			{
				core::string path = core::os::path::get_directory(base->get_current_file_path().c_str());
				if (!path.empty() && (path.back() == '\\' || path.back() == '/'))
					path.erase(path.size() - 1, 1);
				return escape_text(core::stringify::replace(path, "\\", "\\\\"));
			});
		}
		expects_preprocessor<void> preprocessor::define(const std::string_view& expression)
		{
			return define_dynamic(expression, nullptr);
		}
		expects_preprocessor<void> preprocessor::define_dynamic(const std::string_view& expression, proc_expansion_callback&& callback)
		{
			if (expression.empty())
				return preprocessor_exception(preprocessor_error::macro_definition_empty, 0, this_file.path);

			VI_TRACE("proc define %.*s on 0x%" PRIXPTR, (int)expression.size(), expression.data(), (void*)this);
			core::string name; size_t name_offset = 0;
			while (name_offset < expression.size())
			{
				char v = expression[name_offset++];
				if (v != '_' && !core::stringify::is_alphanum(v))
				{
					name = expression.substr(0, --name_offset);
					break;
				}
				else if (name_offset >= expression.size())
				{
					name = expression.substr(0, name_offset);
					break;
				}
			}

			bool empty_parenthesis = false;
			if (name.empty())
				return preprocessor_exception(preprocessor_error::macro_name_empty, 0, this_file.path);

			definition data; size_t template_begin = name_offset, template_end = name_offset + 1;
			if (template_begin < expression.size() && expression[template_begin] == '(')
			{
				int32_t pose = 1;
				while (template_end < expression.size() && pose > 0)
				{
					char v = expression[template_end++];
					if (v == '(')
						++pose;
					else if (v == ')')
						--pose;
				}

				if (pose < 0)
					return preprocessor_exception(preprocessor_error::macro_parenthesis_double_closed, 0, expression);
				else if (pose > 1)
					return preprocessor_exception(preprocessor_error::macro_parenthesis_not_closed, 0, expression);

				core::string pattern = core::string(expression.substr(0, template_end));
				core::stringify::trim(pattern);

				if (!parse_arguments(pattern, data.tokens, false) || data.tokens.empty())
					return preprocessor_exception(preprocessor_error::macro_definition_error, 0, pattern);

				data.tokens.erase(data.tokens.begin());
				empty_parenthesis = data.tokens.empty();
			}

			core::stringify::trim(name);
			if (template_end < expression.size())
			{
				data.expansion = expression.substr(template_end);
				core::stringify::trim(data.expansion);
			}

			if (empty_parenthesis)
				name += "()";

			size_t size = data.expansion.size();
			data.callback = std::move(callback);

			if (size > 0)
				expand_definitions(data.expansion, size);

			defines[name] = std::move(data);
			return core::expectation::met;
		}
		void preprocessor::undefine(const std::string_view& name)
		{
			VI_TRACE("proc undefine %.*s on 0x%" PRIXPTR, (int)name.size(), name.data(), (void*)this);
			auto it = defines.find(core::key_lookup_cast(name));
			if (it != defines.end())
				defines.erase(it);
		}
		void preprocessor::clear()
		{
			defines.clear();
			sets.clear();
			this_file.path.clear();
			this_file.line = 0;
		}
		bool preprocessor::is_defined(const std::string_view& name) const
		{
			bool exists = defines.count(core::key_lookup_cast(name)) > 0;
			VI_TRACE("proc ifdef %.*s on 0x%: %s" PRIXPTR, (int)name.size(), name.data(), (void*)this, exists ? "yes" : "no");
			return exists;
		}
		bool preprocessor::is_defined(const std::string_view& name, const std::string_view& value) const
		{
			auto it = defines.find(core::key_lookup_cast(name));
			if (it != defines.end())
				return it->second.expansion == value;

			return value.empty();
		}
		expects_preprocessor<void> preprocessor::process(const std::string_view& path, core::string& data)
		{
			bool nesting = save_result();
			if (data.empty() || (!path.empty() && has_result(path)))
			{
				apply_result(nesting);
				return core::expectation::met;
			}

			file_context last_file = this_file;
			this_file.path = path;
			this_file.line = 0;

			if (!path.empty())
				sets.insert(core::string(path));

			auto tokens_status = consume_tokens(path, data);
			if (!tokens_status)
			{
				this_file = last_file;
				apply_result(nesting);
				return tokens_status;
			}

			size_t size = data.size();
			auto expansion_status = expand_definitions(data, size);
			if (!expansion_status)
			{
				this_file = last_file;
				apply_result(nesting);
				return expansion_status;
			}

			this_file = last_file;
			apply_result(nesting);
			return core::expectation::met;
		}
		void preprocessor::apply_result(bool was_nested)
		{
			nested = was_nested;
			if (!nested)
			{
				sets.clear();
				this_file.path.clear();
				this_file.line = 0;
			}
		}
		bool preprocessor::has_result(const std::string_view& path)
		{
			return path != this_file.path && sets.count(core::key_lookup_cast(path)) > 0;
		}
		bool preprocessor::save_result()
		{
			bool nesting = nested;
			nested = true;

			return nesting;
		}
		proc_directive preprocessor::find_next_token(core::string& buffer, size_t& offset)
		{
			bool has_multiline_comments = !features.multiline_comment_begin.empty() && !features.multiline_comment_end.empty();
			bool has_comments = !features.comment_begin.empty();
			bool has_string_literals = !features.string_literals.empty();
			proc_directive result;

			while (offset < buffer.size())
			{
				char v = buffer[offset];
				if (v == '#' && offset + 1 < buffer.size() && !core::stringify::is_whitespace(buffer[offset + 1]))
				{
					result.start = offset;
					while (offset < buffer.size())
					{
						if (core::stringify::is_whitespace(buffer[++offset]))
						{
							result.name = buffer.substr(result.start + 1, offset - result.start - 1);
							break;
						}
					}

					result.end = result.start;
					while (result.end < buffer.size())
					{
						char n = buffer[++result.end];
						if (n == '\r' || n == '\n' || result.end == buffer.size())
						{
							result.value = buffer.substr(offset, result.end - offset);
							break;
						}
					}

					core::stringify::trim(result.name);
					core::stringify::trim(result.value);
					if (result.value.size() >= 2)
					{
						if (has_string_literals && result.value.front() == result.value.back() && features.string_literals.find(result.value.front()) != core::string::npos)
						{
							result.value = result.value.substr(1, result.value.size() - 2);
							result.as_scope = true;
						}
						else if (result.value.front() == '<' && result.value.back() == '>')
						{
							result.value = result.value.substr(1, result.value.size() - 2);
							result.as_global = true;
						}
					}

					result.found = true;
					break;
				}
				else if (has_multiline_comments && v == features.multiline_comment_begin.front() && offset + features.multiline_comment_begin.size() - 1 < buffer.size())
				{
					if (memcmp(buffer.c_str() + offset, features.multiline_comment_begin.c_str(), sizeof(char) * features.multiline_comment_begin.size()) != 0)
						goto parse_comments_or_literals;

					offset += features.multiline_comment_begin.size();
					offset = buffer.find(features.multiline_comment_end, offset);
					if (offset == core::string::npos)
					{
						offset = buffer.size();
						break;
					}
					else
						offset += features.multiline_comment_begin.size();
					continue;
				}

			parse_comments_or_literals:
				if (has_comments && v == features.comment_begin.front() && offset + features.comment_begin.size() - 1 < buffer.size())
				{
					if (memcmp(buffer.c_str() + offset, features.comment_begin.c_str(), sizeof(char) * features.comment_begin.size()) != 0)
						goto parse_literals;

					while (offset < buffer.size())
					{
						char n = buffer[++offset];
						if (n == '\r' || n == '\n')
							break;
					}
					continue;
				}

			parse_literals:
				if (has_string_literals && features.string_literals.find(v) != core::string::npos)
				{
					while (offset < buffer.size())
					{
						if (buffer[++offset] == v)
							break;
					}
				}
				++offset;
			}

			return result;
		}
		proc_directive preprocessor::find_next_conditional_token(core::string& buffer, size_t& offset)
		{
		retry:
			proc_directive next = find_next_token(buffer, offset);
			if (!next.found)
				return next;

			if (control_flow.find(next.name) == control_flow.end())
				goto retry;

			return next;
		}
		size_t preprocessor::replace_token(proc_directive& where, core::string& buffer, const std::string_view& to)
		{
			buffer.replace(where.start, where.end - where.start, to);
			return where.start;
		}
		expects_preprocessor<core::vector<preprocessor::conditional>> preprocessor::prepare_conditions(core::string& buffer, proc_directive& next, size_t& offset, bool top)
		{
			core::vector<conditional> conditions;
			size_t child_ending = 0;

			do
			{
				auto control = control_flow.find(next.name);
				if (!conditions.empty())
				{
					auto& last = conditions.back();
					if (!last.text_end)
						last.text_end = next.start;
				}

				conditional block;
				block.type = control->second.first;
				block.chaining = control->second.second != controller::start_if;
				block.expression = next.value;
				block.token_start = next.start;
				block.token_end = next.end;
				block.text_start = next.end;

				if (child_ending > 0)
				{
					core::string text = buffer.substr(child_ending, block.token_start - child_ending);
					if (!text.empty())
					{
						conditional subblock;
						subblock.expression = text;
						conditions.back().childs.emplace_back(std::move(subblock));
					}
					child_ending = 0;
				}

				if (control->second.second == controller::start_if)
				{
					if (conditions.empty())
					{
						conditions.emplace_back(std::move(block));
						continue;
					}

					auto& base = conditions.back();
					auto listing = prepare_conditions(buffer, next, offset, false);
					if (!listing)
						return listing;

					size_t last_size = base.childs.size();
					base.childs.reserve(last_size + listing->size());
					for (auto& item : *listing)
						base.childs.push_back(item);

					child_ending = next.end;
					if (last_size > 0)
						continue;

					core::string text = buffer.substr(base.token_end, block.token_start - base.token_end);
					if (!text.empty())
					{
						conditional subblock;
						subblock.expression = text;
						base.childs.insert(base.childs.begin() + last_size, std::move(subblock));
					}
					continue;
				}
				else if (control->second.second == controller::else_case)
				{
					block.type = (conditions.empty() ? condition::equals : (condition)(-(int32_t)conditions.back().type));
					if (conditions.empty())
						return preprocessor_exception(preprocessor_error::condition_not_opened, 0, next.name);

					conditions.emplace_back(std::move(block));
					continue;
				}
				else if (control->second.second == controller::else_if)
				{
					if (conditions.empty())
						return preprocessor_exception(preprocessor_error::condition_not_opened, 0, next.name);

					conditions.emplace_back(std::move(block));
					continue;
				}
				else if (control->second.second == controller::end_if)
					break;

				return preprocessor_exception(preprocessor_error::condition_error, 0, next.name);
			} while ((next = find_next_conditional_token(buffer, offset)).found);

			return conditions;
		}
		core::string preprocessor::evaluate(core::string& buffer, const core::vector<conditional>& conditions)
		{
			core::string result;
			for (size_t i = 0; i < conditions.size(); i++)
			{
				auto& next = conditions[i];
				int condition = switch_case(next);
				if (condition == 1)
				{
					if (next.childs.empty())
						result += buffer.substr(next.text_start, next.text_end - next.text_start);
					else
						result += evaluate(buffer, next.childs);

					while (i + 1 < conditions.size() && conditions[i + 1].chaining)
						++i;
				}
				else if (condition == -1)
					result += next.expression;
			}

			return result;
		}
		std::pair<core::string, core::string> preprocessor::get_expression_parts(const std::string_view& value)
		{
			size_t offset = 0;
			while (offset < value.size())
			{
				char v = value[offset];
				if (!core::stringify::is_whitespace(v))
				{
					++offset;
					continue;
				}

				size_t count = offset;
				while (offset < value.size() && core::stringify::is_whitespace(value[++offset]));

				core::string right = core::string(value.substr(offset));
				core::string left = core::string(value.substr(0, count));
				core::stringify::trim(right);
				core::stringify::trim(left);

				if (!features.string_literals.empty() && right.front() == right.back() && features.string_literals.find(right.front()) != core::string::npos)
				{
					right = right.substr(1, right.size() - 2);
					core::stringify::trim(right);
				}

				return std::make_pair(left, right);
			}

			core::string expression = core::string(value);
			core::stringify::trim(expression);
			return std::make_pair(expression, core::string());
		}
		std::pair<core::string, core::string> preprocessor::unpack_expression(const std::pair<core::string, core::string>& expression)
		{
			auto left = defines.find(expression.first);
			auto right = defines.find(expression.second);
			return std::make_pair(left == defines.end() ? expression.first : left->second.expansion, right == defines.end() ? expression.second : right->second.expansion);
		}
		int preprocessor::switch_case(const conditional& value)
		{
			if (value.expression.empty())
				return 1;

			switch (value.type)
			{
				case condition::exists:
					return is_defined(value.expression) ? 1 : 0;
				case condition::equals:
				{
					auto parts = unpack_expression(get_expression_parts(value.expression));
					return is_defined(parts.first, parts.second) ? 1 : 0;
				}
				case condition::greater:
				{
					auto parts = unpack_expression(get_expression_parts(value.expression));
					auto left = core::from_string<double>(parts.first), right = core::from_string<double>(parts.second);
					return (left ? *left : 0.0) > (right ? *right : 0.0);
				}
				case condition::greater_equals:
				{
					auto parts = unpack_expression(get_expression_parts(value.expression));
					auto left = core::from_string<double>(parts.first), right = core::from_string<double>(parts.second);
					return (left ? *left : 0.0) >= (right ? *right : 0.0);
				}
				case condition::less:
				{
					auto parts = unpack_expression(get_expression_parts(value.expression));
					auto left = core::from_string<double>(parts.first), right = core::from_string<double>(parts.second);
					return (left ? *left : 0.0) < (right ? *right : 0.0);
				}
				case condition::less_equals:
				{
					auto parts = unpack_expression(get_expression_parts(value.expression));
					auto left = core::from_string<double>(parts.first), right = core::from_string<double>(parts.second);
					return (left ? *left : 0.0) <= (right ? *right : 0.0);
				}
				case condition::not_exists:
					return !is_defined(value.expression) ? 1 : 0;
				case condition::not_equals:
				{
					auto parts = get_expression_parts(value.expression);
					return !is_defined(parts.first, parts.second) ? 1 : 0;
				}
				case condition::not_greater:
				{
					auto parts = unpack_expression(get_expression_parts(value.expression));
					auto left = core::from_string<double>(parts.first), right = core::from_string<double>(parts.second);
					return (left ? *left : 0.0) <= (right ? *right : 0.0);
				}
				case condition::not_greater_equals:
				{
					auto parts = unpack_expression(get_expression_parts(value.expression));
					auto left = core::from_string<double>(parts.first), right = core::from_string<double>(parts.second);
					return (left ? *left : 0.0) < (right ? *right : 0.0);
				}
				case condition::not_less:
				{
					auto parts = unpack_expression(get_expression_parts(value.expression));
					auto left = core::from_string<double>(parts.first), right = core::from_string<double>(parts.second);
					return (left ? *left : 0.0) >= (right ? *right : 0.0);
				}
				case condition::not_less_equals:
				{
					auto parts = unpack_expression(get_expression_parts(value.expression));
					auto left = core::from_string<double>(parts.first), right = core::from_string<double>(parts.second);
					return (left ? *left : 0.0) > (right ? *right : 0.0);
				}
				case condition::text:
				default:
					return -1;
			}
		}
		size_t preprocessor::get_lines_count(core::string& buffer, size_t end)
		{
			const char* target = buffer.c_str();
			size_t offset = 0, lines = 0;
			while (offset < end)
			{
				if (target[offset++] == '\n')
					++lines;
			}

			return lines;
		}
		expects_preprocessor<void> preprocessor::expand_definitions(core::string& buffer, size_t& size)
		{
			if (!size || defines.empty())
				return core::expectation::met;

			core::vector<core::string> tokens;
			core::string formatter = buffer.substr(0, size);
			buffer.erase(buffer.begin(), buffer.begin() + size);

			for (auto& item : defines)
			{
				if (size < item.first.size())
					continue;

				if (item.second.tokens.empty())
				{
					tokens.clear();
					if (item.second.callback != nullptr)
					{
						size_t found_offset = formatter.find(item.first);
						size_t template_size = item.first.size();
						while (found_offset != core::string::npos)
						{
							store_current_line = [this, &formatter, found_offset, template_size]() { return get_lines_count(formatter, found_offset + template_size); };
							auto status = item.second.callback(this, tokens);
							if (!status)
								return status.error();

							formatter.replace(found_offset, template_size, *status);
							found_offset = formatter.find(item.first, found_offset);
						}
					}
					else
						core::stringify::replace(formatter, item.first, item.second.expansion);
					continue;
				}
				else if (size < item.first.size() + 1)
					continue;

				bool stringify = item.second.expansion.find('#') != core::string::npos;
				size_t template_start, offset = 0; core::string needle = item.first + '(';
				while ((template_start = formatter.find(needle, offset)) != core::string::npos)
				{
					int32_t pose = 1; size_t template_end = template_start + needle.size();
					while (template_end < formatter.size() && pose > 0)
					{
						char v = formatter[template_end++];
						if (v == '(')
							++pose;
						else if (v == ')')
							--pose;
					}

					if (pose < 0)
						return preprocessor_exception(preprocessor_error::macro_expansion_parenthesis_double_closed, template_start, this_file.path);
					else if (pose > 1)
						return preprocessor_exception(preprocessor_error::macro_expansion_parenthesis_not_closed, template_start, this_file.path);

					core::string pattern = formatter.substr(template_start, template_end - template_start);
					tokens.reserve(item.second.tokens.size() + 1);
					tokens.clear();

					if (!parse_arguments(pattern, tokens, false) || tokens.empty())
						return preprocessor_exception(preprocessor_error::macro_expansion_error, template_start, this_file.path);

					if (tokens.size() - 1 != item.second.tokens.size())
						return preprocessor_exception(preprocessor_error::macro_expansion_arguments_error, template_start, core::stringify::text("%i out of %i", (int)tokens.size() - 1, (int)item.second.tokens.size()));

					core::string body;
					if (item.second.callback != nullptr)
					{
						auto status = item.second.callback(this, tokens);
						if (!status)
							return status.error();
						body = std::move(*status);
					}
					else
						body = item.second.expansion;

					for (size_t i = 0; i < item.second.tokens.size(); i++)
					{
						auto& from = item.second.tokens[i];
						auto& to = tokens[i + 1];
						core::stringify::replace(body, from, to);
						if (stringify)
							core::stringify::replace(body, "#" + from, '\"' + to + '\"');
					}

					store_current_line = [this, &formatter, template_end]() { return get_lines_count(formatter, template_end); };
					core::stringify::replace_part(formatter, template_start, template_end, body);
					offset = template_start + body.size();
				}
			}

			size = formatter.size();
			buffer.insert(0, formatter);
			return core::expectation::met;
		}
		expects_preprocessor<void> preprocessor::parse_arguments(const std::string_view& value, core::vector<core::string>& tokens, bool unpack_literals)
		{
			size_t where = value.find('(');
			if (where == core::string::npos || value.back() != ')')
				return preprocessor_exception(preprocessor_error::macro_definition_error, 0, this_file.path);

			std::string_view data = value.substr(where + 1, value.size() - where - 2);
			tokens.emplace_back(value.substr(0, where));
			where = 0;

			size_t last = 0;
			while (where < data.size())
			{
				char v = data[where];
				if (v == '\"' || v == '\'')
				{
					while (where < data.size())
					{
						char n = data[++where];
						if (n == v)
							break;
					}

					if (where + 1 >= data.size())
					{
						++where;
						goto add_value;
					}
				}
				else if (v == ',' || where + 1 >= data.size())
				{
				add_value:
					core::string subvalue = core::string(data.substr(last, where + 1 >= data.size() ? core::string::npos : where - last));
					core::stringify::trim(subvalue);

					if (unpack_literals && subvalue.size() >= 2)
					{
						if (!features.string_literals.empty() && subvalue.front() == subvalue.back() && features.string_literals.find(subvalue.front()) != core::string::npos)
							tokens.emplace_back(subvalue.substr(1, subvalue.size() - 2));
						else
							tokens.emplace_back(std::move(subvalue));
					}
					else
						tokens.emplace_back(std::move(subvalue));
					last = where + 1;
				}
				++where;
			}

			return core::expectation::met;
		}
		expects_preprocessor<void> preprocessor::consume_tokens(const std::string_view& path, core::string& buffer)
		{
			size_t offset = 0;
			while (true)
			{
				auto next = find_next_token(buffer, offset);
				if (!next.found)
					break;

				if (next.name == "include")
				{
					if (!features.includes)
						return preprocessor_exception(preprocessor_error::include_denied, offset, core::string(path) + " << " + next.value);

					core::string subbuffer;
					file_desc.path = next.value;
					file_desc.from = path;

					include_result file = resolve_include(file_desc, next.as_global);
					if (has_result(file.library))
					{
					successful_include:
						offset = replace_token(next, buffer, subbuffer);
						continue;
					}

					if (!include)
						return preprocessor_exception(preprocessor_error::include_denied, offset, core::string(path) + " << " + next.value);

					auto status = include(this, file, subbuffer);
					if (!status)
						return status.error();

					switch (*status)
					{
						case include_type::preprocess:
						{
							VI_TRACE("proc %sinclude preprocess %s%s%s on 0x%" PRIXPTR, file.is_remote ? "remote " : "", file.is_abstract ? "abstract " : "", file.is_file ? "file " : "", file.library.c_str(), (void*)this);
							if (subbuffer.empty())
								goto successful_include;

							auto process_status = process(file.library, subbuffer);
							if (process_status)
								goto successful_include;

							return process_status;
						}
						case include_type::unchanged:
							VI_TRACE("proc %sinclude as-is %s%s%s on 0x%" PRIXPTR, file.is_remote ? "remote " : "", file.is_abstract ? "abstract " : "", file.is_file ? "file " : "", file.library.c_str(), (void*)this);
							goto successful_include;
						case include_type::computed:
							VI_TRACE("proc %sinclude virtual %s%s%s on 0x%" PRIXPTR, file.is_remote ? "remote " : "", file.is_abstract ? "abstract " : "", file.is_file ? "file " : "", file.library.c_str(), (void*)this);
							subbuffer.clear();
							goto successful_include;
						case include_type::error:
						default:
							return preprocessor_exception(preprocessor_error::include_not_found, offset, core::string(path) + " << " + next.value);
					}
				}
				else if (next.name == "pragma")
				{
					core::vector<core::string> tokens;
					if (!parse_arguments(next.value, tokens, true) || tokens.empty())
						return preprocessor_exception(preprocessor_error::pragma_not_found, offset, next.value);

					core::string name = tokens.front();
					tokens.erase(tokens.begin());
					if (!pragma)
						continue;

					auto status = pragma(this, name, tokens);
					if (!status)
						return status;

					VI_TRACE("proc apply pragma %s on 0x%" PRIXPTR, buffer.substr(next.start, next.end - next.start).c_str(), (void*)this);
					offset = replace_token(next, buffer, core::string());
				}
				else if (next.name == "define")
				{
					define(next.value);
					offset = replace_token(next, buffer, "");
				}
				else if (next.name == "undef")
				{
					undefine(next.value);
					offset = replace_token(next, buffer, "");
					if (!expand_definitions(buffer, offset))
						return preprocessor_exception(preprocessor_error::directive_expansion_error, offset, next.name);
				}
				else if (next.name.size() >= 2 && next.name[0] == 'i' && next.name[1] == 'f' && control_flow.find(next.name) != control_flow.end())
				{
					size_t start = next.start;
					auto conditions = prepare_conditions(buffer, next, offset, true);
					if (!conditions)
						return preprocessor_exception(preprocessor_error::condition_not_closed, offset, next.name);

					core::string result = evaluate(buffer, *conditions);
					next.start = start; next.end = offset;
					offset = replace_token(next, buffer, result);
				}
				else
				{
					auto it = directives.find(next.name);
					if (it == directives.end())
						continue;

					core::string result;
					auto status = it->second(this, next, result);
					if (!status)
						return status.error();

					offset = replace_token(next, buffer, result);
				}
			}

			return core::expectation::met;
		}
		expects_preprocessor<core::string> preprocessor::resolve_file(const std::string_view& path, const std::string_view& include_path)
		{
			if (!features.includes)
				return preprocessor_exception(preprocessor_error::include_denied, 0, core::string(path) + " << " + core::string(include_path));

			file_context last_file = this_file;
			this_file.path = path;
			this_file.line = 0;

			core::string subbuffer;
			file_desc.path = include_path;
			file_desc.from = path;

			include_result file = resolve_include(file_desc, !core::stringify::find_of(include_path, ":/\\").found && !core::stringify::find(include_path, "./").found);
			if (has_result(file.library))
			{
			include_result:
				this_file = last_file;
				return subbuffer;
			}

			if (!include)
			{
				this_file = last_file;
				return preprocessor_exception(preprocessor_error::include_denied, 0, core::string(path) + " << " + core::string(include_path));
			}

			auto status = include(this, file, subbuffer);
			if (!status)
			{
				this_file = last_file;
				return status.error();
			}

			switch (*status)
			{
				case include_type::preprocess:
				{
					VI_TRACE("proc %sinclude preprocess %s%s%s on 0x%" PRIXPTR, file.is_remote ? "remote " : "", file.is_abstract ? "abstract " : "", file.is_file ? "file " : "", file.library.c_str(), (void*)this);
					if (subbuffer.empty())
						goto include_result;

					auto process_status = process(file.library, subbuffer);
					if (process_status)
						goto include_result;

					this_file = last_file;
					return process_status.error();
				}
				case include_type::unchanged:
					VI_TRACE("proc %sinclude as-is %s%s%s on 0x%" PRIXPTR, file.is_remote ? "remote " : "", file.is_abstract ? "abstract " : "", file.is_file ? "file " : "", file.library.c_str(), (void*)this);
					goto include_result;
				case include_type::computed:
					VI_TRACE("proc %sinclude virtual %s%s%s on 0x%" PRIXPTR, file.is_remote ? "remote " : "", file.is_abstract ? "abstract " : "", file.is_file ? "file " : "", file.library.c_str(), (void*)this);
					subbuffer.clear();
					goto include_result;
				case include_type::error:
				default:
					this_file = last_file;
					return preprocessor_exception(preprocessor_error::include_not_found, 0, core::string(path) + " << " + core::string(include_path));
			}
		}
		const core::string& preprocessor::get_current_file_path() const
		{
			return this_file.path;
		}
		size_t preprocessor::get_current_line_number()
		{
			if (store_current_line != nullptr)
			{
				this_file.line = store_current_line();
				store_current_line = nullptr;
			}

			return this_file.line + 1;
		}
		include_result preprocessor::resolve_include(const include_desc& desc, bool as_global)
		{
			include_result result;
			if (!as_global)
			{
				core::string base;
				if (desc.from.empty())
				{
					auto directory = core::os::directory::get_working();
					if (directory)
						base = *directory;
				}
				else
					base = core::os::path::get_directory(desc.from.c_str());

				bool is_origin_remote = (desc.from.empty() ? false : core::os::path::is_remote(base.c_str()));
				bool is_path_remote = (desc.path.empty() ? false : core::os::path::is_remote(desc.path.c_str()));
				if (is_origin_remote || is_path_remote)
				{
					result.library = desc.path;
					result.is_remote = true;
					result.is_file = true;
					if (!is_origin_remote)
						return result;

					core::stringify::replace(result.library, "./", "");
					result.library.insert(0, base);
					return result;
				}
			}

			if (!core::stringify::starts_of(desc.path, "/."))
			{
				if (as_global || desc.path.empty() || desc.root.empty())
				{
					result.library = desc.path;
					result.is_abstract = true;
					core::stringify::replace(result.library, '\\', '/');
					return result;
				}

				auto library = core::os::path::resolve(desc.path, desc.root, false);
				if (library)
				{
					result.library = *library;
					if (core::os::file::is_exists(result.library.c_str()))
					{
						result.is_abstract = true;
						result.is_file = true;
						return result;
					}
				}

				for (auto it : desc.exts)
				{
					core::string file(result.library);
					if (result.library.empty())
					{
						auto target = core::os::path::resolve(desc.path + it, desc.root, false);
						if (!target)
							continue;

						file.assign(*target);
					}
					else
						file.append(it);

					if (!core::os::file::is_exists(file.c_str()))
						continue;

					result.library = std::move(file);
					result.is_abstract = true;
					result.is_file = true;
					return result;
				}

				result.library = desc.path;
				result.is_abstract = true;
				core::stringify::replace(result.library, '\\', '/');
				return result;
			}
			else if (as_global)
				return result;

			core::string base;
			if (desc.from.empty())
			{
				auto directory = core::os::directory::get_working();
				if (directory)
					base = *directory;
			}
			else
				base = core::os::path::get_directory(desc.from.c_str());

			auto library = core::os::path::resolve(desc.path, base, true);
			if (library)
			{
				result.library = *library;
				if (core::os::file::is_exists(result.library.c_str()))
				{
					result.is_file = true;
					return result;
				}
			}

			for (auto it : desc.exts)
			{
				core::string file(result.library);
				if (result.library.empty())
				{
					auto target = core::os::path::resolve(desc.path + it, desc.root, true);
					if (!target)
						continue;

					file.assign(*target);
				}
				else
					file.append(it);

				if (!core::os::file::is_exists(file.c_str()))
					continue;

				result.library = std::move(file);
				result.is_file = true;
				return result;
			}

			result.library.clear();
			return result;
		}
	}
}