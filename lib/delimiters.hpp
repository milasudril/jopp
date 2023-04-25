#ifndef JOPP_DELIMITERS_HPP
#define JOPP_DELIMITERS_HPP

#include <optional>

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
	}

	inline constexpr auto is_whitespace(char ch)
	{ return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'; }

	inline constexpr auto char_should_be_escaped(char ch)
	{ return (ch >= '\0' && ch <= '\x1f') || ch == '"'; }

	inline constexpr auto begin_esc_seq = '\\';

	namespace esc_chars
	{
		inline constexpr auto quotation_mark = '"';
		inline constexpr auto rev_sollidus = '\\';
		inline constexpr auto linefeed = 'n';
		inline constexpr auto tab = 't';
	}

	inline constexpr std::optional<char> unescape(char esc_char)
	{
		switch(esc_char)
		{
			case esc_chars::quotation_mark:
				return '"';
			case esc_chars::rev_sollidus:
				return '\\';
			case esc_chars::linefeed:
				return '\n';
			case esc_chars::tab:
				return '\t';
			default:
				return std::nullopt;
		}
	}
}
#endif