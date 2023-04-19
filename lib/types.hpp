#ifndef JOPP_JSON_TYPES_HPP
#define JOPP_JSON_TYPES_HPP

#include <variant>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <type_traits>

namespace jopp
{
	template<class A, class B>
	inline constexpr auto is_same_without_cvref_v = std::is_same_v<std::remove_cvref_t<A>,
		std::remove_cvref_t<B>>;

	class array;
	class object;
	using number = double;
	using value = std::variant<
		std::unique_ptr<object>,
		std::unique_ptr<array>,
		number,
		bool,
		std::monostate>;

	template<class T>
	inline constexpr auto is_object_or_array_v = is_same_without_cvref_v<T, array>
		|| is_same_without_cvref_v<T, object>;

	class array
	{
	public:
		template<class T>
		requires(!is_object_or_array_v<T>)
		void push_back(T&& val)
		{ m_values.push_back(std::forward<T>(val)); }

		template<class T>
		requires(is_object_or_array_v<T>)
		void push_back(T&& val)
		{ m_values.push_back(std::make_unique<T>(std::forward<T>(val))); }

		auto begin() const
		{ return std::begin(m_values); }

		auto end() const
		{ return std::end(m_values); }

		auto begin()
		{ return std::begin(m_values); }

		auto end()
		{ return std::end(m_values); }

		auto size() const
		{ return std::size(m_values); }

	private:
		std::vector<value> m_values;
	};
}

#endif