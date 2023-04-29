#ifndef JOPP_SERIALIZER_HPP
#define JOPP_SERIALIZER_HPP

#include "./types.hpp"
#include "./delimiters.hpp"
#include "./utils.hpp"

#include <string_view>
#include <stack>
#include <span>
#include <functional>

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

	auto make_serializer_context(std::reference_wrapper<value const> val)
	{
		return val.get().visit(overload{
			[](array const& item){
				return serializer_context{
					.range = range_processor<item_pointer>{std::begin(item), std::end(item)},
					.block_starter = delimiters::begin_array,
					.block_terminator = delimiters::end_array
				};
			},
			[](object const& item){
				return serializer_context{
					.range = range_processor<item_pointer>{std::begin(item), std::end(item)},
					.block_starter = delimiters::begin_object,
					.block_terminator = delimiters::end_object
				};
			},
			[](auto const&) {
				assert(false);
				return serializer_context{};
			}
		});
	}

	class serializer
	{
	public:
		explicit serializer(std::reference_wrapper<value const> root)
		{
			m_contexts.push(make_serializer_context(root));
		}

		serialize_result serialize(std::span<char> output_buffer);

	private:
		std::stack<serializer_context> m_contexts;
		std::string_view m_current_key;
		std::string m_current_value;

		std::span<char const> m_range_to_write;
	};
}

inline jopp::serialize_result jopp::serializer::serialize(std::span<char>)
{
	auto& current_context = m_contexts.top();
	while(true)
	{
		auto const res = current_context.range.pop_element();
		if(!res.has_value())
		{
			m_contexts.pop();
			if(m_contexts.empty())
			{ return serialize_result{}; }
		}

		m_current_key = res.get_key();
		res.get_value().visit(overload{
			[this](auto const& val) {
				m_current_value = to_string(val);
				m_range_to_write = std::span{std::begin(m_current_value), std::end(m_current_value)};
			},
			[this](jopp::string const& val){
				m_range_to_write = std::span{std::begin(val), std::end(val)};
			},
			[](jopp::object const&){puts("Object");},
			[](jopp::array const&){puts("Array");}
		});

		printf("%s: %s\n", m_current_key.data(), m_range_to_write.data());
	}
}

#endif
