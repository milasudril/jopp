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
}

#endif
