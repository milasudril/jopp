#ifndef JOPP_PARSER_HPP
#define JOPP_PARSER_HPP

#include "./types.hpp"

#include <string_view>
#include <stack>
#include <span>

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

	inline constexpr std::string_view false_literal{"false"};
	inline constexpr std::string_view null_literal{"null"};
	inline constexpr std::string_view true_literal{"true"};

	inline constexpr auto begin_esc_seq = '\\';

	namespace esc_chars
	{
		inline constexpr auto quotation_mark = '"';
		inline constexpr auto rev_sollidus = '\\';
		inline constexpr auto linefeed = 'n';
		inline constexpr auto tab = 't';
	}

	enum class error_code
	{
		completed,
		more_data_needed,
		duplicate_key_value,
		character_must_be_escaped,
		unsupported_escape_sequence,
		illegal_delimiter
	};

	struct parse_result
	{
		char const* ptr;
		error_code ec;
	};

	class parser
	{
	public:
		parse_result parse(std::span<char const> input_seq);

	private:
		enum class state
		{
			init,
			array,
			object,
			before_key,
			key,
			value,
			string,
			read_other_value
		};

		using value_factory = value (*)(std::string&& buffer);

		struct node
		{
			state current_state;
			std::variant<array, object> container;
		};

		std::stack<node> m_nodes;

		value_factory m_value_factory;
	};
};

#endif