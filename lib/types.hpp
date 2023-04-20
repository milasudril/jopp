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

	template<class T>
	inline constexpr auto is_object_or_array_v = is_same_without_cvref_v<T, array>
		|| is_same_without_cvref_v<T, object>;

	template<class T>
	concept dereferenceable = requires(T x)
	{
		{*x};
	};

	class value
	{
	public:
		template<class T>
		requires(!is_object_or_array_v<T>)
		explicit value(T&& val):m_value{std::forward<T>(val)}
		{}

		template<class T>
		requires(is_object_or_array_v<T>)
		explicit value(T&& val):m_value{std::make_unique<T>(std::forward<T>(val))}
		{}

		template<class T>
		decltype(auto) get_if() const
		{
			if constexpr(is_object_or_array_v<T>)
			{ return std::get_if<T>(&m_value).get(); }
			else
			{ return std::get_if<T>(&m_value); }
		}

		template<class T>
		decltype(auto) get_if()
		{
			if constexpr(is_object_or_array_v<T>)
			{ return std::get_if<T>(&m_value).get(); }
			else
			{ return std::get_if<T>(&m_value); }
		}

		template<class Visitor>
		decltype(auto) visit(Visitor&& v) const
		{
			return std::visit([v = std::forward<Visitor>(v)]<class T>(T&& x) {
				if constexpr(dereferenceable<T>)
				{
					using referenced_type = std::decay_t<decltype(*x)>;
					return v(std::forward<referenced_type>(*x));
				}
				else
				{ return v(std::forward<T>(x)); }
			}, m_value);
		}

	private:
		std::variant<
			std::unique_ptr<object>,
			std::unique_ptr<array>,
			number,
			bool,
			std::nullptr_t> m_value;
	};

	class array
	{
	public:
		template<class T>
		void push_back(T&& val)
		{ m_values.push_back(value{std::forward<T>(val)}); }

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

	class object
	{
	public:
		using key_type = std::string;
		using mapped_type = value;
		using value_type = std::pair<key_type const, mapped_type>;

	private:
		std::map<key_type, mapped_type> properties;
	};
}

#endif