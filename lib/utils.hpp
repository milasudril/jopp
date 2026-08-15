#ifndef JOPP_UTILS_HPP
#define JOPP_UTILS_HPP

#include <type_traits>
#include <memory>
#include <cassert>
#include <array>

namespace jopp
{
	template<class ... Ts>
	struct overload : Ts ...
	{ using Ts::operator() ...; };

	template<class ValueReference>
	class enumerator
	{
	public:
		virtual ValueReference pop_element() = 0;
		virtual ~enumerator() = default;
	};

	template<class InputIterator,
		class ValueReference = decltype(&*std::declval<InputIterator>())>
	class iterator_enumerator : public enumerator<ValueReference>
	{
	public:
		explicit iterator_enumerator(InputIterator begin, InputIterator end):
			m_ptr{begin},
			m_end{end}
		{}

		ValueReference pop_element() override
		{
			if(m_ptr == m_end)
			{ return ValueReference{nullptr}; }

			auto ret = ValueReference{&*m_ptr};
			++m_ptr;
			return ret;
		}

	private:
		InputIterator m_ptr;
		InputIterator m_end;
	};

	template<class ValueReference>
	class range_processor
	{
	public:
		range_processor() = default;

		template<class InputIterator>
		explicit range_processor(InputIterator begin, InputIterator end):
			m_impl{std::make_unique<iterator_enumerator<InputIterator, ValueReference>>(begin, end)}
		{}

		ValueReference pop_element()
		{ return m_impl->pop_element(); }


	private:
		std::unique_ptr<enumerator<ValueReference>> m_impl;
	};

	template<class T>
	requires(std::is_pointer_v<T>)
	constexpr decltype(auto) safe_deref(T val)
	{
		assert(val != nullptr);
		return *val;
	}

	// 1. Primary template: defaults to false
	template <typename T, template <typename...> class C>
	struct is_instantiation_of : std::false_type {};

	// 2. Partial specialization: matches any instantiation of C<Args...>
	template <template <typename...> class C, typename... Args>
	struct is_instantiation_of<C<Args...>, C> : std::true_type {};

	// 3. Variable template helper
	template <typename T, template <typename...> class C>
	inline constexpr bool is_instantiation_of_v = is_instantiation_of<T, C>::value;

	// 4. C++20 Concept
	template <typename T, template <typename...> class C>
	concept instance_of = is_instantiation_of_v<T, C>;
}

#endif
