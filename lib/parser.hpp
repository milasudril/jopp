#ifndef JOPP_PARSER_HPP
#define JOPP_PARSER_HPP

#include "./types.hpp"

#include <string_view>
#include <stack>
#include <span>
#include <optional>
#include <charconv>

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

	inline constexpr std::string_view false_literal{"false"};
	inline constexpr std::string_view null_literal{"null"};
	inline constexpr std::string_view true_literal{"true"};

	inline std::optional<number> to_number(std::string_view val)
	{
		number ret{};
		auto const res = std::from_chars(std::begin(val), std::end(val), ret);

		if(res.ptr != std::end(val) ||
			res.ec == std::errc::result_out_of_range ||
			res.ec == std::errc::invalid_argument)
		{
			return std::nullopt;
		}

		return ret;
	}

	inline std::optional<value> make_value(std::string_view literal)
	{
		if(literal == false_literal)
		{ return value{false}; }

		if(literal == true_literal)
		{ return value{true}; }

		if(literal == null_literal)
		{ return value{null{}}; }

		if(auto val = to_number(literal); val.has_value())
		{ return value{*val}; }

		return std::nullopt;
	}

	enum class error_code
	{
		completed,
		more_data_needed,
		duplicate_key_value,
		character_must_be_escaped,
		unsupported_escape_sequence,
		illegal_delimiter,
		invalid_value
	};

	struct parse_result
	{
		char const* ptr;
		error_code ec;
		size_t line;
		size_t col;
	};

	class parser
	{
	public:
		parser(): m_state{state::value}{}
		parse_result parse(std::span<char const> input_seq, value& root);

	private:
		enum class state
		{
			value,
			literal,
			string_value,
			string_value_esc_seq
		};

		using value_factory = value (*)(std::string&& buffer);

		struct node
		{
			state current_state;
			std::variant<array, object> container;
		};

		std::stack<node> m_nodes;

		std::string m_buffer;
		state m_state;

		value_factory m_value_factory;
	};

	parse_result parser::parse(std::span<char const> input_seq, value& root)
	{
		auto current_state = m_state;
		auto buffer = std::move(m_buffer);

		auto ptr = std::data(input_seq);
		while(true)
		{
			if(ptr == std::data(input_seq) + std::size(input_seq))
			{
				return parse_result{
					.ptr = ptr,
					.ec = std::size(m_nodes) == 0 ?
						error_code::completed : error_code::more_data_needed,
					.line = 0,  // TODO: count lines
					.col = 0  // TODO: count cols
				};
			}

			auto ch_in = *ptr;
			++ptr;

			switch(current_state)
			{
				case state::value:
					switch(ch_in)
					{
						case delimiters::begin_array:
							// Start reading array
							break;
						case delimiters::begin_object:
							// Start reading object
							break;
						case delimiters::string_begin_end:
							current_state = state::string_value;
							break;
						default:
							if(!is_whitespace(ch_in))
							{
								buffer += ch_in;
								current_state = state::literal;
							}
					}
					break;

				case state::literal:
					if(is_whitespace(ch_in))
					{
						auto val = make_value(buffer);
						buffer.clear();
						if(!val.has_value())
						{
							return parse_result{
								.ptr = ptr - 1,
								.ec = error_code::invalid_value,
								.line = 0, // TODO: count lines
								.col = 0  // TODO: count lines
							};
						}

						if(std::size(m_nodes) == 0)
						{
							root = std::move(*val);
							return parse_result{
								.ptr = ptr - 1,
								.ec = error_code::completed,
								.line = 0,  // TODO: count lines
								.col = 0  // TODO: count cols
							};
						}

						// If reading object, go to state::after_value_object
						// If reading array, go to state::after_value_array
					}
					else
					{ buffer += ch_in; }
					break;

				case state::string_value:
					switch(ch_in)
					{
						case delimiters::string_begin_end:
							if(std::size(m_nodes) == 0)
							{
								root = value{std::move(buffer)};
								return parse_result{
									.ptr = ptr - 1,
									.ec = error_code::completed,
									.line = 0,  // TODO: count lines
									.col = 0  // TODO: count cols
								};
							}

							// If reading object, go to state::after_value_object
							// If reading array, go to state::after_value_array
							break;

						case begin_esc_seq:
							current_state = state::string_value_esc_seq;
							break;

						default:
							if(char_should_be_escaped(ch_in))
							{
								return parse_result{
									.ptr = ptr - 1,
									.ec = error_code::character_must_be_escaped,
									.line = 0,
									.col = 0
								};
							}
							else
							{ buffer += ch_in; }
					}
					break;

				case state::string_value_esc_seq:
					if(auto val = unescape(ch_in); val.has_value())
					{
						buffer += *val;
						current_state = state::string_value;
					}
					else
					{
						return parse_result{
							.ptr = ptr - 1,
							.ec = error_code::unsupported_escape_sequence,
							.line = 0,
							.col = 0
						};
					}
					break;
			}
		}
	}
}

#endif