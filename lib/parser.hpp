#ifndef JOPP_PARSER_HPP
#define JOPP_PARSER_HPP

#include <string_view>

namespace jopp
{
	namespace delimiters
	{
		inline constexpr auto begin_array = '[';
		inline constexpr auto begin_object = '{';
		inline constexpr auto end_array = ']';
		inline constexpr auto end_object = '}';
		inline constexpr auto name_separator = ':';
		inline constexpr auto value_separator = ',';
		inline constexpr auto string_begin_end = '"';
		inline constexpr auto begin_esc_seq = '\\';
	}

	inline constexpr bool is_whitespace(char ch)
	{ return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'; }

	inline constexpr bool char_should_be_escaped(char ch)
	{ return (ch >= '\0' && ch <= '\x1f') || ch == '"'; }

	inline constexpr std::string_view false_literal{"false"};
	inline constexpr std::string_view null_literal{"null"};
	inline constexpr std::string_view true_literal{"true"};

	namespace esc_chars
	{
		inline constexpr auto quotation_mark = '"';
		inline constexpr auto rev_sollidus = '\\';
		inline constexpr auto linefeed = 'n';
		inline constexpr auto tab = 't';
	}
};

#endif