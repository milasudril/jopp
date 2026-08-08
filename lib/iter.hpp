#ifndef JOPP2_ITER_HPP
#define JOPP2_ITER_HPP

#include <ranges>

namespace jopp2
{
	template<class IterType>
	class iter
	{
	public:
		using iter_type = IterType;

		template<std::ranges::forward_range Range>
		constexpr explicit iter(Range&& range):
			m_current_position{std::begin(range)},
			m_end{std::end(range)}
		{}

		template<class T>
		requires(
				!std::ranges::range<std::remove_cvref_t<T>>
			&&!std::is_same_v<std::remove_cvref_t<T>, iter>
		)
		constexpr iter(T& obj):
			m_current_position{&obj},
			m_end{&obj + 1}
		{}

		constexpr bool at_end() const
		{ return m_current_position == m_end; }

		constexpr auto& next()
		{ return *m_current_position++; }

	private:
		IterType m_current_position;
		IterType m_end;
	};

	template<std::ranges::forward_range Range>
	iter(Range&& range)->iter<std::ranges::iterator_t<Range>>;

	template<class T>
	requires(
			!std::ranges::range<std::remove_cvref_t<T>>
		&&!std::is_same_v<std::remove_cvref_t<T>, iter<T*>>
	)
	iter(T& obj) -> iter<T*>;

	template<class T>
	using make_iter_t = decltype(iter(std::declval<T&>()));
}

#endif
