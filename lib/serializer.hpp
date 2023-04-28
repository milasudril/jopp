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
	};

	auto make_range_processor(std::reference_wrapper<value const> val)
	{
		return val.get().visit(overload{
			[](array const& item){
				return range_processor<item_pointer>{std::begin(item), std::end(item)};
			},
			[](object const& item){
				return range_processor<item_pointer>{std::begin(item), std::end(item)};
			},
			[](auto const&) {
				assert(false);
				return range_processor<item_pointer>{};
			}
		});
	}

	class serializer
	{
	public:
		explicit serializer(std::reference_wrapper<value const> root)
		{
			m_contexts.push(serializer_context{make_range_processor(root)});
		}

		serialize_result serialize(std::span<char> output_buffer);

	private:
		std::stack<serializer_context> m_contexts;
	};

	inline serialize_result serializer::serialize(std::span<char>)
	{
		auto& current_context = m_contexts.top();
		while(true)
		{
			auto res = current_context.range.pop_element();
			if(!res.has_value())
			{
				m_contexts.pop();
				if(m_contexts.empty())
				{ return serialize_result{}; }
			}

			res.visit(overload{
				[](value const&){},
				[](char const*, value const&){}
			});
		}
	}

}

#endif