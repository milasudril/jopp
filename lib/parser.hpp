#ifndef JOPP_PARSER_HPP
#define JOPP_PARSER_HPP

#include "./types.hpp"

#include <string_view>
#include <stack>
#include <span>
#include <optional>
#include <charconv>
#include <cassert>

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
		key_already_exists,
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

	enum class parser_state
	{
		value,
		literal,
		string_value,
		string_value_esc_seq,
		before_key,
		key,
		key_esc_seq,
		before_value,
		after_value_object,
		after_value_array
	};

	struct parser_context
	{
		string key;
		class value value{null{}};
	};

	struct store_value_result
	{
		parser_state next_state;
		error_code err;
	};

	inline
	auto store_value(class value& parent_value, string&& key, class value&& value, class value& root)
	{
		return parent_value.visit(
			overload{
				[](object& item, string&& key, class value&& val) {
					if(!item.insert(std::move(key), std::move(val)).second)
					{
						return store_value_result{
							parser_state::after_value_object,
							error_code::key_already_exists
						};
					}

					return store_value_result{
						parser_state::after_value_object,
						error_code::more_data_needed
					};
				},
				[](array& item, string&&, class value&& val) {
					item.push_back(std::move(val));
					return store_value_result{
						parser_state::after_value_array,
						error_code::more_data_needed
					};
				},
				[&root](null&, string&&, class value&& val) {
					root = std::move(val);
					return store_value_result{
						parser_state::value,
						error_code::completed
					};
				},
				[](auto&, string&&, class value&&) {
					assert(false);
					return store_value_result{};
				}
			},
			std::move(key),
			std::move(value)
		);
	}

	inline
	auto store_value(class value& parent_value, string&& key, std::string_view buffer, value& root)
	{
		auto val = make_value(buffer);
		if(!val.has_value())
		{
			return store_value_result{
				parser_state::value,
				error_code::more_data_needed
			};
		}

		return store_value(parent_value, std::move(key), std::move(*val), root);
	}

	class parser
	{
	public:
		parser():
			m_current_state{parser_state::value}
		{}

		parse_result parse(std::span<char const> input_seq, value& root);

	private:
		using value_factory = value (*)(std::string&& buffer);

		parser_state m_current_state;
		std::stack<parser_context> m_contexts;
		string m_buffer;

		value_factory m_value_factory;
	};

	parse_result parser::parse(std::span<char const> input_seq, value& root)
	{
		auto ptr = std::data(input_seq);
		while(true)
		{
			if(ptr == std::data(input_seq) + std::size(input_seq))
			{
				return parse_result{
					.ptr = ptr,
					.ec = std::size(m_contexts) == 0 ?
						error_code::completed : error_code::more_data_needed,
					.line = 0,  // TODO: count lines
					.col = 0  // TODO: count cols
				};
			}

			auto ch_in = *ptr;
			++ptr;

			switch(m_current_state)
			{
				case parser_state::value:
					switch(ch_in)
					{
						case delimiters::begin_array:
							m_contexts.push(parser_context{});
							m_contexts.top().value = value{array{}};
							m_current_state = parser_state::value;
							break;

						case delimiters::begin_object:
							m_contexts.push(parser_context{});
							m_contexts.top().value = value{object{}};
							m_current_state = parser_state::before_key;
							break;

						case delimiters::string_begin_end:
							m_current_state = parser_state::string_value;
							break;

						default:
							if(!is_whitespace(ch_in))
							{
								m_buffer += ch_in;
								m_current_state = parser_state::literal;
							}
					}
					break;

				case parser_state::literal:
					if(ch_in == delimiters::value_separator
						|| ch_in == delimiters::end_array
						|| ch_in == delimiters::end_object)
					{
						printf("Key: (%s), Input buffer (%s)\n", m_contexts.top().key.c_str(), m_buffer.c_str());
						auto res = store_value(m_contexts.top().value,
							std::move(m_contexts.top().key),
							m_buffer,
							root);
						if(res.err != error_code::more_data_needed)
						{
							return parse_result{
									.ptr = ptr,
									.ec = res.err,
									.line = 0,  // TODO: count lines
									.col = 0  // TODO: count cols
								};
						}

						--ptr;  // Because next state wants value_separator
						m_current_state = res.next_state;
						m_buffer = string{};
						m_contexts.top().key = string{};
					}
					else
					{ m_buffer += ch_in; }
					break;

				case parser_state::string_value:
					switch(ch_in)
					{
						case delimiters::string_begin_end:
						{
							auto res = store_value(m_contexts.top().value,
								std::move(m_contexts.top().key),
								value{m_buffer},
								root);
							if(res.err != error_code::more_data_needed)
							{
								return parse_result{
										.ptr = ptr,
										.ec = res.err,
										.line = 0,  // TODO: count lines
										.col = 0  // TODO: count cols
									};
							}

							m_current_state = res.next_state;
							m_contexts.top().key = string{};
							m_buffer = string{};
							break;
						}

						case begin_esc_seq:
							m_current_state = parser_state::string_value_esc_seq;
							break;

						default:
							if(char_should_be_escaped(ch_in))
							{
								return parse_result{
									.ptr = ptr,
									.ec = error_code::character_must_be_escaped,
									.line = 0,
									.col = 0
								};
							}
							else
							{ m_buffer += ch_in; }
					}
					break;

				case parser_state::string_value_esc_seq:
					if(auto val = unescape(ch_in); val.has_value())
					{
						m_buffer += *val;
						m_current_state = parser_state::string_value;
					}
					else
					{
						return parse_result{
							.ptr = ptr,
							.ec = error_code::unsupported_escape_sequence,
							.line = 0,
							.col = 0
						};
					}
					break;

				case parser_state::before_key:
					switch(ch_in)
					{
						case delimiters::string_begin_end:
							m_current_state= parser_state::key;
							break;
						default:
							if(!is_whitespace(ch_in))
							{
								return parse_result{
									.ptr = ptr,
									.ec = error_code::illegal_delimiter,
									.line = 0,  // TODO: count lines
									.col = 0  // TODO: count cols
								};
							}
					}
					break;

				case parser_state::key:
					switch(ch_in)
					{
						case delimiters::string_begin_end:
							m_contexts.top().key = std::move(m_buffer);
							m_buffer = string{};
							m_current_state = parser_state::before_value;
							break;

						case begin_esc_seq:
							m_current_state = parser_state::key_esc_seq;
							break;

						default:
							if(char_should_be_escaped(ch_in))
							{
								return parse_result{
									.ptr = ptr,
									.ec = error_code::character_must_be_escaped,
									.line = 0,
									.col = 0
								};
							}
							else
							{ m_buffer += ch_in; }
					}
					break;

				case parser_state::key_esc_seq:
					if(auto val = unescape(ch_in); val.has_value())
					{
						m_buffer += *val;
						m_current_state = parser_state::key;
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

				case parser_state::before_value:
					switch(ch_in)
					{
						case delimiters::name_separator:
							m_current_state = parser_state::value;
							break;

						default:
							if(!is_whitespace(ch_in))
							{
								return parse_result{
									.ptr = ptr,
									.ec = error_code::illegal_delimiter,
									.line = 0,  // TODO: count lines
									.col = 0  // TODO: count cols
								};
							}
					}
					break;

				case parser_state::after_value_object:
					switch(ch_in)
					{
						case delimiters::end_object:
						{
							auto current_context = std::move(m_contexts.top());
							m_contexts.pop();
							if(std::size(m_contexts) == 0)
							{
								printf("Last object read\n");
								root = std::move(current_context.value);
								return parse_result{
									.ptr = ptr,
									.ec = error_code::completed,
									.line = 0,
									.col = 0
								};
							}

							(void)store_value(m_contexts.top().value,
								std::move(m_contexts.top().key),
								std::move(current_context.value),
								root);
							m_contexts.pop();
							break;
						}

						case delimiters::value_separator:
							m_current_state = parser_state::before_key;
							break;

						default:
							if(!is_whitespace(ch_in))
							{
								return parse_result{
									.ptr = ptr,
									.ec = error_code::illegal_delimiter,
									.line = 0,  // TODO: count lines
									.col = 0  // TODO: count cols
								};
							}
					}
					break;

				case parser_state::after_value_array:
					switch(ch_in)
					{
						case delimiters::end_array:
						{
							auto current_context = std::move(m_contexts.top());
							m_contexts.pop();
							if(std::size(m_contexts) == 0)
							{
								root = std::move(current_context.value);
								return parse_result{
									.ptr = ptr,
									.ec = error_code::completed,
									.line = 0,
									.col = 0
								};
							}

							(void)store_value(m_contexts.top().value,
								std::move(m_contexts.top().key),
								std::move(current_context.value),
								root);
							m_contexts.pop();
							break;
						}

						case delimiters::value_separator:
							m_current_state = parser_state::value;
							break;

						default:
							if(!is_whitespace(ch_in))
							{
								return parse_result{
									.ptr = ptr,
									.ec = error_code::illegal_delimiter,
									.line = 0,  // TODO: count lines
									.col = 0  // TODO: count cols
								};
							}
					}
					break;
			}
		}
	}
}

#endif