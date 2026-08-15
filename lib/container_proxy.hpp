#ifndef JOPP2_CONTAINER_PROXY_HPP
#define JOPP2_CONTAINER_PROXY_HPP

#include <cstddef>
#include <ranges>
#include <type_traits>

namespace jopp2
{
	template<class Range>
	struct selected_iterator
	{
		using type = std::ranges::iterator_t<Range>;

		static constexpr auto get_begin(Range& container)
		{ return std::ranges::begin(container); }

		static constexpr auto get_end(Range& container)
		{ return std::ranges::end(container); }
	};

	template<class Container>
	class container_wrapper
	{
	public:
		constexpr explicit container_wrapper(Container& container):
			m_begin{selected_iterator<Container>::get_begin(container)},
			m_end{selected_iterator<Container>::get_end(container)},
			m_size(std::size(container))
		{}

		constexpr auto begin() const
		{ return m_begin; }

		constexpr auto end() const
		{ return m_end; }

		constexpr auto size() const
		{ return m_size; }

	private:
		selected_iterator<Container>::type m_begin;
		selected_iterator<Container>::type m_end;
		size_t m_size;
	};

	template<class Container>
	requires(!std::is_const_v<Container>)
	struct container_wrapper<Container>:std::reference_wrapper<Container>
	{
		using std::reference_wrapper<Container>::reference_wrapper;

		constexpr auto begin() const
		{ return std::ranges::begin(std::reference_wrapper<Container>::get()); }

		constexpr auto end() const
		{ return std::ranges::end(std::reference_wrapper<Container>::get()); }

		constexpr auto size() const
		{ return std::ranges::size(std::reference_wrapper<Container>::get()); }
	};

	template<class Container>
	class container_proxy
	{
	public:
		using iterator = selected_iterator<Container>::type;
		using active_range_type = std::ranges::subrange<iterator, iterator>;

		explicit container_proxy(Container& container):
			m_current_iterator{selected_iterator<Container>::get_begin(container)},
			m_backing_store{container}
		{}

		constexpr auto active_range() const
		{ return active_range_type{m_current_iterator, m_backing_store.end()}; }

		constexpr auto total_size() const
		{ return m_backing_store.size(); }

		constexpr auto& pop_active_element()
		{ return pop_active_elements(1); }

		constexpr auto& pop_active_elements(size_t count)
		{
			auto const num_elems_to_pop = std::min(
				static_cast<ssize_t>(count),
				std::ranges::distance(m_current_iterator, m_backing_store.end())
			);
			std::ranges::advance(m_current_iterator, num_elems_to_pop);
			return *this;
		}

		constexpr void clear_backing_store() requires(!std::is_const_v<Container>)
		{
			m_backing_store.get().clear();
			m_current_iterator = m_backing_store.begin();
		}

		template<class Other>
		requires(!std::is_const_v<Container>)
		constexpr void replace_backing_store(Other&& container)
		{
			m_backing_store.get() = std::forward<Other>(container);
			m_current_iterator = m_backing_store.begin();
		}

	private:
		iterator m_current_iterator;
		container_wrapper<Container> m_backing_store;
	};
};

#endif
