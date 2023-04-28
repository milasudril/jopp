#ifndef JOPP_SERIALIZER_HPP
#define JOPP_SERIALIZER_HPP

#include "./types.hpp"
#include "./delimiters.hpp"
#include "./utils.hpp"

#include <string_view>
#include <stack>
#include <span>
#include <reference_wrapper>

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


	struct serailzer_context
	{
		range_processor<item_pointer> current_item;
	};

	class serializer
	{
	public:
		explicit serializer(std::reference_wrapper<value const> root):m_root{root}{}

		serialize_result serialize(std::span<char> output_buffer, value const& val);

	private:
		std::reference_wrapper<value const> m_root;
		std::stack<serailzer_context> m_contexts;
	};
}

#endif