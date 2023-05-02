		#ifndef JOPP_DELIMITERS_HPP
#define JOPP_DELIMITERS_HPP

#include <optional>
#include <string>
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
	}

	inline constexpr auto is_whitespace(char ch)
	{ return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'; }

	inline constexpr auto char_should_be_escaped(char ch)
	{ return (ch >= '\0' && ch <= '\x1f') || ch == '"' || ch == '\\'; }

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

	inline constexpr std::optional<char> get_escape_char(char ch)
	{
		switch(ch)
		{
			case '"':
				return esc_chars::quotation_mark;
			case '\\':
				return esc_chars::rev_sollidus;
			case '\n':
				return esc_chars::linefeed;
			case '\t':
				return esc_chars::tab;
			default:
				return std::nullopt;
		}
	}

	inline std::optional<std::string> escape(std::string_view val)
	{
		std::string ret;
		ret.reserve(std::size(val));
		for(size_t k = 0; k != std::size(val); ++k)
		{
			if(char_should_be_escaped(val[k]))
			{
				auto const esc_char = get_escape_char(val[k]);
				if(!esc_char.has_value())
				{ return std::nullopt; }

				ret.push_back(begin_esc_seq);
				ret.push_back(*esc_char);
			}
			else
			{ ret.push_back(val[k]); }
		}

		return ret;
	}

	inline std::optional<std::string> wrap_string(std::string_view val)
	{
		auto escaped_string = escape(val);
		if(!escaped_string.has_value())
		{ return std::nullopt; }

		std::string ret{delimiters::string_begin_end};
		ret.append(*escaped_string);
		ret += delimiters::string_begin_end;

		return ret;
	}
}
#endif