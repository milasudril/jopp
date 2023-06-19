#ifndef JOPP_JSON_TYPES_HPP
#define JOPP_JSON_TYPES_HPP

#include "./utils.hpp"

#include <variant>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <type_traits>
#include <optional>
#include <charconv>

namespace jopp
{
	template<class T>
	struct get_type_name;


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

	template<>
	struct get_type_name<object>
	{ static constexpr char const* value = "object"; };

	template<>
	struct get_type_name<array>
	{ static constexpr char const* value = "array"; };

	template<>
	struct get_type_name<number>
	{ static constexpr char const* value = "number"; };

	template<>
	struct get_type_name<string>
	{ static constexpr char const* value = "string"; };

	template<>
	struct get_type_name<bool>
	{ static constexpr char const* value = "boolean"; };

	template<>
	struct get_type_name<null>
	{ static constexpr char const* value = "null"; };

	inline constexpr char const* false_literal{"false"};
	inline constexpr char const* null_literal{"null"};
	inline constexpr char const* true_literal{"true"};

	inline constexpr char const* to_string(null)
	{ return null_literal; }

	inline constexpr char const* to_string(boolean x)
	{ return x == true ? true_literal : false_literal; }

	template<class T>
	requires(std::is_same_v<std::remove_cvref_t<T>, string>)
	inline decltype(auto) to_string(T&& val)
	{ return std::forward<T>(val); }

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

	inline std::string to_string(jopp::number x)
	{
		std::array<char, 32> buffer{};
		auto const res = std::to_chars(std::begin(buffer), std::end(buffer), x);
		return std::string{std::data(buffer), res.ptr};
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

		template<class T>
		auto get_if()
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

	inline bool is_null(value const& val)
	{ return val.get_if<null>() != nullptr; }

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

	class missing_field_error:public std::runtime_error
	{
	public:
		explicit missing_field_error(std::string_view key):
			runtime_error{std::string{"Mandatory field `"}.append(key).append("` is missing")}
		{}
	};

	class field_type_mismatch_error:public std::runtime_error
	{
	public:
		template<class T>
		explicit field_type_mismatch_error(std::string_view key, std::type_identity<T>):
			runtime_error{std::string{"Field `"}.append(key).append("` should be a ").append(get_type_name<T>::value)}
		{}
	};

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
		auto& get_field_as(std::string_view key) const
		{
			auto const i = find(key);
			if(i == std::end(m_values))
			{ throw missing_field_error{key}; }

			auto const value = i->second.get_if<T>();
			if(value == nullptr)
			{ throw field_type_mismatch_error{key, std::type_identity<T>{}}; }

			return *value;
		}

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

	class item_pointer
	{
	public:
		explicit item_pointer(std::pair<object::key_type const, object::mapped_type> const* kv):
			m_key{safe_deref(kv).first},
			m_value{&safe_deref(kv).second}
		{}

		explicit item_pointer(value const* val):
			m_key{},
			m_value{val}
		{}

		explicit item_pointer(nullptr_t):m_key{}, m_value{nullptr} {}

		bool has_value() const { return m_value != nullptr; }

		std::string_view get_key() const
		{ return m_key; }

		value const& get_value() const
		{ return *m_value; }

	private:
		std::string_view m_key;
		value const* m_value;
	};

	class container
	{
	public:
		container() = default;

		template<class T>
		requires(is_object_or_array_v<T>)
		explicit container(T&& val):m_value{std::forward<T>(val)}
		{}

		template<class T>
		auto get_if() const
		{ return std::get_if<T>(&m_value); }

		template<class T>
		auto get_if()
		{ return std::get_if<T>(&m_value); }

		template<class Visitor, class ... Args>
		decltype(auto) visit(Visitor&& v, Args&& ... args) const
		{
			return std::visit([v = std::forward<Visitor>(v),
				...args = std::forward<Args>(args)]<class T>(T&& x) mutable {
					return v(std::forward<T>(x), std::forward<Args>(args)...);

			}, m_value);
		}

		template<class Visitor, class ... Args>
		decltype(auto) visit(Visitor&& v, Args&& ... args)
		{
			return std::visit([v = std::forward<Visitor>(v),
				...args = std::forward<Args>(args)]<class T>(T&& x) mutable {
					return v(std::forward<T>(x), std::forward<Args>(args)...);

			}, m_value);
		}

		bool operator==(container const&) const = default;
		bool operator!=(container const&) const = default;

	private:
		std::variant<object, array> m_value;
	};

	template<class T>
	requires(std::is_same_v<std::decay_t<T>, container>)
	decltype(auto) to_value(T&& obj)
	{
		return obj.visit([]<class U>(U&& item){
			return value{std::forward<std::decay_t<U>>(item)};
		});
	}
}

#endif
