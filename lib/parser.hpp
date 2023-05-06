#ifndef JOPP_PARSER_HPP
#define JOPP_PARSER_HPP

#include "./types.hpp"
#include "./delimiters.hpp"
#include "./utils.hpp"

#include <string_view>
#include <stack>
#include <span>
#include <iterator>

namespace jopp
{
	enum class parser_error_code
	{
		completed,
		more_data_needed,
		key_already_exists,
		character_must_be_escaped,
		unsupported_escape_sequence,
		illegal_delimiter,
		invalid_value,
		no_top_level_node,
		nesting_level_too_deep
	};

	inline constexpr char const* to_string(parser_error_code ec)
	{
		switch(ec)
		{
			case parser_error_code::completed:
				return "Completed";
			case parser_error_code::more_data_needed:
				return "More data needed";
			case parser_error_code::key_already_exists:
				return "Key already exists";
			case parser_error_code::character_must_be_escaped:
				return "Character must be escaped";
			case parser_error_code::unsupported_escape_sequence:
				return "Unsupported escape sequence";
			case parser_error_code::illegal_delimiter:
				return "Illegal delimiter";
			case parser_error_code::invalid_value:
				return "Invalid value";
			case parser_error_code::no_top_level_node:
				return "No top level node";
			case parser_error_code::nesting_level_too_deep:
				return "Nesting too level deep";
		}
		__builtin_unreachable();
	}

	template<class InputSeqIterator>
	struct parse_result
	{
		InputSeqIterator ptr;
		parser_error_code ec;
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
		container value;
	};

	struct store_value_result
	{
		parser_state next_state;
		parser_error_code err;
	};

	inline auto store_value(container& parent, string&& key, class value&& value)
	{
		return parent.visit(
			overload{
				[](object& item, string&& key, class value&& val) {
					if(!item.insert(std::move(key), std::move(val)).second)
					{
						return store_value_result{
							parser_state::after_value_object,
							parser_error_code::key_already_exists
						};
					}

					return store_value_result{
						parser_state::after_value_object,
						parser_error_code::more_data_needed
					};
				},
				[](array& item, string&&, class value&& val) {
					item.push_back(std::move(val));
					return store_value_result{
						parser_state::after_value_array,
						parser_error_code::more_data_needed
					};
				}
			},
			std::move(key),
			std::move(value)
		);
	}

	struct literal_view
	{ std::string_view value; };

	inline auto store_value(container& parent, string&& key, literal_view buffer)
	{
		auto val = make_value(buffer.value);
		if(!val.has_value())
		{
			return store_value_result{
				parser_state::value,
				parser_error_code::invalid_value
			};
		}

		return store_value(parent, std::move(key), std::move(*val));
	}

	inline auto store_value(container& parent, string&& key, container&& child)
	{
		return store_value(parent, std::move(key), to_value(std::move(child)));
	}

	template<class T>
	concept parser_input_range = requires(T x)
	{
		{ std::begin(x) } -> std::bidirectional_iterator;
		{ std::end(x) } -> std::bidirectional_iterator;
		{ *std::begin(x) } -> std::convertible_to<char>;
	};

	class parser
	{
	public:
		explicit parser(container& root, std::optional<size_t> max_levels = 1024):
			m_line{1},
			m_col{1},
			m_current_state{parser_state::value},
			m_max_levels{max_levels},
			m_root{root}
		{}

		template<parser_input_range InputSeq>
		auto parse(InputSeq input_seq);

	private:
		size_t m_line;
		size_t m_col;
		parser_state m_current_state;
		std::optional<size_t> m_max_levels;
		std::stack<parser_context> m_contexts;
		string m_buffer;
		std::reference_wrapper<container> m_root;
	};
}

template<jopp::parser_input_range InputSeq>
auto jopp::parser::parse(InputSeq input_seq)
{
	auto ptr = std::begin(input_seq);
	while(true)
	{
		if(ptr == std::end(input_seq))
		{ return parse_result{ptr, parser_error_code::more_data_needed, m_line, m_col}; }

		auto ch_in = *ptr;
		auto const old_pos = ptr;
		++ptr;

		switch(m_current_state)
		{
			case parser_state::value:
				switch(ch_in)
				{
					case delimiters::begin_array:
						if(m_max_levels.has_value() && std::size(m_contexts) == *m_max_levels)
						{ return parse_result{ptr, parser_error_code::nesting_level_too_deep, m_line, m_col }; }
						m_contexts.push(parser_context{});
						m_contexts.top().value = container{array{}};
						m_current_state = parser_state::value;
						break;

					case delimiters::begin_object:
						if(m_max_levels.has_value() && std::size(m_contexts) == *m_max_levels)
						{ return parse_result{ptr, parser_error_code::nesting_level_too_deep, m_line, m_col }; }
						m_contexts.push(parser_context{});
						m_contexts.top().value = container{object{}};
						m_current_state = parser_state::before_key;
						break;

					case delimiters::end_array:
						m_current_state = parser_state::after_value_array;
						--ptr;
						break;

					case delimiters::string_begin_end:
						if(std::size(m_contexts) == 0)
						{ return parse_result{ptr, parser_error_code::no_top_level_node, m_line, m_col}; }
						m_current_state = parser_state::string_value;
						break;

					default:
						if(std::size(m_contexts) == 0)
						{ return parse_result{ptr, parser_error_code::no_top_level_node, m_line, m_col}; }
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
					|| ch_in == delimiters::end_object
					|| is_whitespace(ch_in))
				{
					auto res = store_value(m_contexts.top().value,
						std::move(m_contexts.top().key),
						literal_view{m_buffer});
					if(res.err != parser_error_code::more_data_needed)
					{ return parse_result{ptr, res.err, m_line, m_col}; }

					if(!is_whitespace(ch_in))
					{ --ptr; }

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
							value{m_buffer});
						if(res.err != parser_error_code::more_data_needed)
						{ return parse_result{ptr, res.err, m_line, m_col}; }

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
						{ return parse_result{ptr, parser_error_code::character_must_be_escaped, m_line, m_col}; }
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
				{ return parse_result{ptr, parser_error_code::unsupported_escape_sequence, m_line, m_col}; }
				break;

			case parser_state::before_key:
				switch(ch_in)
				{
					case delimiters::string_begin_end:
						m_current_state = parser_state::key;
						break;

					case delimiters::end_object:
						m_current_state = parser_state::after_value_object;
						--ptr;
						break;

					default:
						if(!is_whitespace(ch_in))
						{ return parse_result{ptr, parser_error_code::illegal_delimiter, m_line, m_col}; }
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
						{ return parse_result{ptr, parser_error_code::character_must_be_escaped, m_line, m_col}; }
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
				{ return parse_result{ptr, parser_error_code::unsupported_escape_sequence, m_line, m_col}; }
				break;

			case parser_state::before_value:
				switch(ch_in)
				{
					case delimiters::name_separator:
						m_current_state = parser_state::value;
						break;

					default:
						if(!is_whitespace(ch_in))
						{ return parse_result{ptr, parser_error_code::illegal_delimiter, m_line, m_col}; }
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
							m_root.get() = std::move(current_context.value);
							return parse_result{ptr, parser_error_code::completed, m_line, m_col};
						}

						auto const res = store_value(m_contexts.top().value,
							std::move(m_contexts.top().key),
							std::move(current_context.value));
						if(res.err != parser_error_code::more_data_needed)
						{ return parse_result{ptr, res.err, m_line, m_col}; }
						m_current_state = res.next_state;
						break;
					}

					case delimiters::value_separator:
						m_current_state = parser_state::before_key;
						break;

					default:
						if(!is_whitespace(ch_in))
						{ return parse_result{ptr, parser_error_code::illegal_delimiter, m_line, m_col}; }
				}
				break;

			case parser_state::after_value_array:
				switch(ch_in)
				{
					case delimiters::end_array:
					{
						auto current_context = std::move(m_contexts.top());
						m_contexts.pop();
						if(m_contexts.empty())
						{
							m_root.get() = std::move(current_context.value);
							return parse_result{ptr, parser_error_code::completed, m_line, m_col};
						}

						m_current_state = store_value(m_contexts.top().value,
							std::move(m_contexts.top().key),
							std::move(current_context.value)).next_state;
						break;
					}

					case delimiters::value_separator:
						m_current_state = parser_state::value;
						break;

					default:
						if(!is_whitespace(ch_in))
						{ return parse_result{ptr, parser_error_code::illegal_delimiter, m_line, m_col}; }
				}
				break;
		}

		if(ptr != old_pos)
		{
			if(ch_in == '\n')
			{
				m_col = 0;
				++m_line;
			}
			++m_col;
		}
	}
}

#endif