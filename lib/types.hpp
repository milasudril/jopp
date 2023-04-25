#ifndef JOPP_JSON_TYPES_HPP
#define JOPP_JSON_TYPES_HPP

#include <variant>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <type_traits>
#include <optional>
#include <charconv>

#include <cstdio>

namespace jopp
{
	template<class A, class B>
	inline constexpr auto is_same_without_cvref_v = std::is_same_v<std::remove_cvref_t<A>,
		std::remove_cvref_t<B>>;

	using boolean = bool;
	struct null
	{
		constexpr bool operator==(null) const
		{ return true; }

		constexpr bool operator!=(null) const
		{ return false; }
	};

	class object;
	class array;
	using number = double;
	using string = std::string;

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
		explicit value():m_value{}{}

		template<class T>
		requires(!is_object_or_array_v<T>)
		explicit value(T&& val):m_value{std::forward<T>(val)}
		{}

		template<class T>
		requires(is_object_or_array_v<T>)
		explicit value(T&& val):m_value{std::make_unique<T>(std::forward<T>(val))}
		{}

		template<class T>
		auto get_if() const
		{
			if constexpr(is_object_or_array_v<T>)
			{
				auto ptr = std::get_if<std::unique_ptr<T>>(&m_value);
				return ptr != nullptr ? ptr->get() : nullptr;
			}
			else
			{ return std::get_if<T>(&m_value); }
		}

		template<class Visitor, class ... Args>
		decltype(auto) visit(Visitor&& v, Args&& ... args) const
		{
			return std::visit([v = std::forward<Visitor>(v),
				...args = std::forward<Args>(args)]<class T>(T&& x) mutable {
				if constexpr(dereferenceable<T>)
				{
					using referenced_type = decltype(*x);
					return v(std::forward<referenced_type>(*x), std::forward<Args>(args)...);
				}
				else
				{ return v(std::forward<T>(x), std::forward<Args>(args)...); }
			}, m_value);
		}

		template<class Visitor, class ... Args>
		decltype(auto) visit(Visitor&& v, Args&& ... args)
		{
			return std::visit([v = std::forward<Visitor>(v),
				...args = std::forward<Args>(args)]<class T>(T&& x) mutable {
				if constexpr(dereferenceable<T>)
				{
					using referenced_type = decltype(*x);
					return v(std::forward<referenced_type>(*x), std::forward<Args>(args)...);
				}
				else
				{ return v(std::forward<T>(x), std::forward<Args>(args)...); }
			}, m_value);
		}

		bool operator==(value const&) const = default;
		bool operator!=(value const&) const = default;

	private:
		std::variant<
			null,
			boolean,
			std::unique_ptr<object>,
			std::unique_ptr<array>,
			number,
			string
			> m_value;
	};

	template<class ... Ts>
	struct overload : Ts ...
	{ using Ts::operator() ...; };

	inline bool is_null(value const& val)
	{ return val.get_if<null>() != nullptr; }

	inline constexpr std::string_view false_literal{"false"};
	inline constexpr std::string_view null_literal{"null"};
	inline constexpr std::string_view true_literal{"true"};

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

	class object
	{
	public:
		using key_type = std::string;
		using mapped_type = value;
		using value_type = std::pair<key_type const, mapped_type>;

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

		auto find(std::string_view key) const
		{ return m_values.find(key); }

		auto find(std::string_view key)
		{ return m_values.find(key); }

		template<class T>
		auto insert(key_type&& key, T&& value)
		{ return m_values.insert(std::pair{std::move(key), mapped_type{std::forward<T>(value)}}); }

		template<class T>
		auto insert_or_assign(key_type&& key, T&& value)
		{ return m_values.insert_or_assign(std::move(key), mapped_type{std::forward<T>(value)}); }

		auto constains(std::string_view key) const
		{ return m_values.contains(key); }

	private:
		std::map<key_type, mapped_type, std::less<>> m_values;
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

		decltype(auto) operator[](size_t k)
		{ return m_values[k]; }

		decltype(auto) operator[](size_t k) const
		{ return m_values[k]; }

	private:
		std::vector<value> m_values;
	};

	void debug_print(jopp::object const& obj, size_t level = 0);
	void debug_print(jopp::array const&, size_t level = 0);
	void debug_print(jopp::number x, size_t level = 0);
	void debug_print(jopp::string const& x, size_t level = 0);
	void debug_print(jopp::boolean x, size_t level = 0);
	void debug_print(jopp::null, size_t level = 0);
	void debug_print(jopp::value const& x, size_t level = 0);

	void debug_indent(size_t level)
	{
		for(size_t k = 0; k != level; ++k)
		{ printf("  "); }
	}

	void debug_print(jopp::value const& x, size_t level)
	{ x.visit([level](auto const& item){ debug_print(item, level); }); }

	void debug_print(jopp::object const& obj, size_t level)
	{
		debug_indent(level);
		puts("«object»");
		for(auto& item : obj)
		{
			debug_indent(level);
			printf("%s:\n", item.first.c_str());
			debug_print(item.second, level + 1);
		}
	}

	void debug_print(jopp::array const& obj, size_t level)
	{
		debug_indent(level);
		puts("«array»");
		for(auto& item : obj)
		{
			debug_print(item, level + 1);
		}
	}

	void debug_print(jopp::number x, size_t level)
	{
		debug_indent(level);
		printf("%.16g\n", x);
	}

	void debug_print(jopp::string const& x, size_t level)
	{
		debug_indent(level);
		puts(x.c_str());
	}

	void debug_print(jopp::boolean x, size_t level)
	{
		debug_indent(level);
		printf("%s\n", x?"true":"false");
	}

	void debug_print(jopp::null, size_t level)
	{
		debug_indent(level);
		puts("null");
	}
}

#endif