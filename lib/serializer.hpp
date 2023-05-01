#ifndef JOPP_SERIALIZER_HPP
#define JOPP_SERIALIZER_HPP

#include "./types.hpp"
#include "./delimiters.hpp"
#include "./utils.hpp"

#include <string_view>
#include <stack>
#include <span>
#include <functional>
#include <algorithm>
#include <ranges>

namespace jopp
{
	enum class serializer_error_code
	{
		completed,
		buffer_is_full,
		illegal_char_in_string
	};

	struct serialize_result
	{
		value const* last_value;
		serializer_error_code ec;
	};


	struct serializer_context
	{
		range_processor<item_pointer> range;
		char block_starter;
		char block_terminator;
	};
	
	template<class Input, class Output>
	requires(std::ranges::range<Input> && std::ranges::range<Output>)
	struct write_buffer_result
	{
		Input in;
		Output out;
	};
	
	template<class Input, class Output>
	requires(std::ranges::random_access_range<Input> && std::ranges::random_access_range<Output>)
	inline auto write_buffer(Input src, Output dest)
	{
		auto const n = std::min(std::size(src), std::size(dest));
		std::copy_n(std::begin(src), n, std::begin(dest));
		return write_buffer_result{
			.in =Input{std::begin(src) + n, std::end(src)},
			.out = Output{std::begin(dest) + n, std::end(dest)}
		};
	}

	inline auto make_serializer_context(std::reference_wrapper<object const> val)
	{
		return serializer_context{
			.range = range_processor<item_pointer>{std::begin(val.get()), std::end(val.get())},
			.block_starter = delimiters::begin_object,
			.block_terminator = delimiters::end_object
		};
	}
	
	inline auto make_serializer_context(std::reference_wrapper<array const> val)
	{
		return serializer_context{
			.range = range_processor<item_pointer>{std::begin(val.get()), std::end(val.get())},
			.block_starter = delimiters::begin_array,
			.block_terminator = delimiters::end_array
		};
	}
	
	template<class T>
	requires(!std::is_same_v<T, jopp::object> && !std::is_same_v<T, jopp::array>)
	inline auto make_serializer_context(T const&)
	{
		assert(false);
		return serializer_context{};
	}
	
	auto make_serializer_context(std::reference_wrapper<value const> val)
	{
		return val.get().visit([](auto const& val) { return make_serializer_context(val); } );
	}
	
	enum class serializer_state{write_value, write_key, fetch_item};

	class serializer
	{
	public:
		explicit serializer(std::reference_wrapper<value const> root):
			m_current_state{serializer_state::fetch_item}
		{ m_contexts.push(make_serializer_context(root)); }

		serialize_result serialize(std::span<char> output_buffer);

	private:
		std::stack<serializer_context> m_contexts;
		std::string_view m_current_key;
		std::string m_current_value;
		serializer_context m_next_context;
		serializer_state m_current_state;

		std::span<char const> m_range_to_write;
	};
}

inline jopp::serialize_result jopp::serializer::serialize(std::span<char> output_buffer)
{
	while(true)
	{
		if(std::size(output_buffer) == 0)
		{
			// FIXME: Fill in result
			return serialize_result{};
		}

		switch(m_current_state)
		{
			case serializer_state::write_key:
			{
				auto res = write_buffer(m_current_key, output_buffer);
				m_current_key = res.in;
				output_buffer = res.out;
				if(std::size(m_current_key) == 0)
				{ m_current_state = serializer_state::write_value; }
				break;
			}

			case serializer_state::write_value:
			{
				if(std::size(m_range_to_write) == 0)
				{ 
					putchar(m_next_context.block_starter);
					fflush(stdout);
					m_contexts.push(std::move(m_next_context));
					m_current_state = serializer_state::fetch_item;
				}
				else
				{
					auto res = write_buffer(m_range_to_write, output_buffer);
					m_range_to_write = res.in;
					output_buffer = res.out;
					if(std::size(m_range_to_write) == 0)
					{ m_current_state = serializer_state::fetch_item; }
				}
				break;
			}

			case serializer_state::fetch_item:
			{
				auto& current_context = m_contexts.top();
				auto const res = current_context.range.pop_element();
				if(!res.has_value())
				{
					putchar(current_context.block_terminator);
					fflush(stdout);
					m_contexts.pop();
					if(m_contexts.empty())
					{ return serialize_result{}; }
				}
				else
				{
					m_current_key = res.get_key();
					m_current_state = (m_current_key != std::string_view{}) ?
						serializer_state::write_key : serializer_state::write_value;

					res.get_value().visit(overload{
						[this](auto const& val) {
							m_current_value = to_string(val);
							m_range_to_write = std::span{std::begin(m_current_value), std::end(m_current_value)};
						},
						[this](jopp::string const& val){
							m_range_to_write = std::span{std::begin(val), std::end(val)};
						},
						[this](jopp::object const& val){
							m_range_to_write = std::span<char const>{};
							m_next_context = make_serializer_context(val);
						},
						[this](jopp::array const& val){
							m_range_to_write = std::span<char const>{};
							m_next_context = make_serializer_context(val);
						}
					});
				}
				break;
			}
		}
	}
}

#endif
 
