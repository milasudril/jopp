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
		char* ptr;
		serializer_error_code ec;
	};


	struct serializer_context
	{
		range_processor<item_pointer> range;
		char block_starter;
		char block_terminator;
		bool first_item{true};
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

	inline auto make_serializer_context(std::reference_wrapper<container const> val)
	{
		return val.get().visit([](auto const& val) { return make_serializer_context(val); } );
	}

	class serializer
	{
	public:
		explicit serializer(std::reference_wrapper<container const> root):
			m_string_to_write{std::make_unique<std::string>()}
		{
			m_contexts.push(make_serializer_context(root));
			(*m_string_to_write) += m_contexts.top().block_starter;
			(*m_string_to_write) += '\n';
			m_range_to_write = std::span{std::begin((*m_string_to_write)), std::end((*m_string_to_write))};
		}

		explicit serializer(std::reference_wrapper<object const> root):
			m_string_to_write{std::make_unique<std::string>()}
		{
			m_contexts.push(make_serializer_context(root));
			(*m_string_to_write) += m_contexts.top().block_starter;
			(*m_string_to_write) += '\n';
			m_range_to_write = std::span{std::begin((*m_string_to_write)), std::end((*m_string_to_write))};
		}

		explicit serializer(std::reference_wrapper<array const> root):
			m_string_to_write{std::make_unique<std::string>()}
		{
			m_contexts.push(make_serializer_context(root));
			(*m_string_to_write) += m_contexts.top().block_starter;
			(*m_string_to_write) += '\n';
			m_range_to_write = std::span{std::begin((*m_string_to_write)), std::end((*m_string_to_write))};
		}

		serialize_result serialize(std::span<char> output_buffer);

	private:
		std::unique_ptr<std::string> m_string_to_write;
		std::span<char const> m_range_to_write;
		std::stack<serializer_context> m_contexts;

	};
}

inline jopp::serialize_result jopp::serializer::serialize(std::span<char> output_buffer)
{
	while(true)
	{
		if(std::begin(output_buffer) == std::end(output_buffer))
		{ return serialize_result{std::data(output_buffer), serializer_error_code::buffer_is_full}; }

		auto res = write_buffer(m_range_to_write, output_buffer);
		m_range_to_write = res.in;
		output_buffer = res.out;
		if(m_contexts.empty())
		{ return serialize_result{std::data(output_buffer), serializer_error_code::completed}; }

		if(std::size(m_range_to_write) == 0)
		{
			(*m_string_to_write).clear();
			auto& current_context = m_contexts.top();

			auto const res = current_context.range.pop_element();
			if(!res.has_value())
			{
				(*m_string_to_write) += '\n';
				for(size_t k = 0; k != m_contexts.size() - 1; ++k)
				{ (*m_string_to_write) += '\t'; }
				(*m_string_to_write) += std::string{current_context.block_terminator};
				m_range_to_write = std::span{std::begin((*m_string_to_write)), std::end((*m_string_to_write))};
				m_contexts.pop();
				continue;
			}

			if(!current_context.first_item)
			{
				(*m_string_to_write) += delimiters::value_separator;
				(*m_string_to_write) += '\n';
			}
			current_context.first_item = false;
			for(size_t k = 0; k != m_contexts.size(); ++k)
			{ (*m_string_to_write) += '\t'; }

			if(auto key = res.get_key(); key != std::string_view{})
			{
				auto wrapped_string = wrap_string(key);
				if(!wrapped_string.has_value())
				{
					return serialize_result{
						std::data(output_buffer), serializer_error_code::illegal_char_in_string
					};
				}
				(*m_string_to_write) += *wrapped_string;
				(*m_string_to_write) += delimiters::name_separator;
				(*m_string_to_write) += ' ';
			}

			auto const result = res.get_value().visit(overload{
				[this](auto const& val) {
					(*m_string_to_write) += to_string(val);
					return true;
				},
				[this](jopp::string const& val){
					auto str = wrap_string(val);
					if(!str.has_value())
					{ return false; }

					(*m_string_to_write) += *str;
					return true;
				},
				[this](jopp::object const& val){
					m_contexts.push(make_serializer_context(val));
					(*m_string_to_write) += m_contexts.top().block_starter;
					(*m_string_to_write) += '\n';
					return true;
				},
				[this](jopp::array const& val){
					m_contexts.push(make_serializer_context(val));
					(*m_string_to_write) += m_contexts.top().block_starter;
					(*m_string_to_write) += '\n';
					return true;
				}
			});
			if(!result)
			{
				return serialize_result{
					std::data(output_buffer), serializer_error_code::illegal_char_in_string
				};
			}

			m_range_to_write = std::span{std::begin((*m_string_to_write)), std::end((*m_string_to_write))};
		}
	}
}

namespace jopp
{
	template<class T>
 	requires std::is_same_v<std::remove_cvref_t<T>, container>
		|| std::is_same_v<std::remove_cvref_t<T>, array>
		|| std::is_same_v<std::remove_cvref_t<T>, object>
	inline auto to_string(T const& root)
	{
		std::string ret;
		jopp::serializer serializer{root};
		std::array<char, 65536> buffer{};
		while(true)
		{
			auto const res = serializer.serialize(buffer);
			ret.insert(std::end(ret), std::data(buffer), res.ptr);

			if(res.ec == jopp::serializer_error_code::completed)
			{ return ret; }

			if(res.ec != jopp::serializer_error_code::buffer_is_full)
			{ throw std::runtime_error{"jopp bad data"}; }
		}
	}

	class json_buffer;

	class json_buffer_view:private std::string_view
	{
	public:
		using std::string_view::begin;
		using std::string_view::end;
		using std::string_view::data;
		using std::string_view::size;

		inline json_buffer_view(std::reference_wrapper<json_buffer const>);
	};

	class json_buffer
	{
	public:
		template<class T>
		requires std::is_same_v<std::remove_cvref_t<T>, container>
			|| std::is_same_v<std::remove_cvref_t<T>, array>
			|| std::is_same_v<std::remove_cvref_t<T>, object>
		explicit json_buffer(T const& root):
			m_content{to_string(root)}
		{}

		auto get_view() const
		{ return std::string_view{m_content}; }

	private:
		std::string m_content;
	};

	json_buffer_view::json_buffer_view(std::reference_wrapper<json_buffer const> buffer):
		std::string_view{buffer.get().get_view()}
	{}
}

#endif

