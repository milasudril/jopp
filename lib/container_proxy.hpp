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

	template<class Range>
	requires(std::ranges::contiguous_range<Range>)
	struct selected_iterator<Range>
	{
		using type = std::conditional_t<
			std::is_const_v<Range>,
			std::ranges::range_value_t<Range> const*,
			std::ranges::range_value_t<Range>*
		>;

		static constexpr auto get_begin(Range& container)
		{ return std::ranges::data(container); }

		static constexpr auto get_end(Range& container)
		{ return std::ranges::data(container) + std::ranges::size(container); }
	};

	template<class Container>
	class container_wrapper
	{
	public:
		constexpr explicit container_wrapper(std::reference_wrapper<Container> container):
			m_begin{selected_iterator<Container>::get_begin(container.get())},
			m_end{selected_iterator<Container>::get_end(container.get())},
			m_size(std::ranges::size(container.get()))
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
	container_wrapper(std::reference_wrapper<Container> container) -> container_wrapper<Container>;

	template<class Container>
	requires(!std::is_const_v<Container>)
	class container_wrapper<Container>
	{
	public:
		explicit container_wrapper(std::reference_wrapper<Container> container):
			m_container{container}
		{}

		constexpr auto begin() const
		{ return std::ranges::begin(m_container.get()); }

		constexpr auto end() const
		{ return std::ranges::end(m_container.get()); }

		constexpr auto size() const
		{ return std::ranges::size(m_container.get()); }

		constexpr auto& get() const
		{ return m_container.get(); }

	private:
		std::reference_wrapper<Container> m_container;
	};

	template<class Container>
	class container_proxy
	{
	public:
		using iterator = selected_iterator<Container>::type;
		using active_range_type = std::ranges::subrange<iterator, iterator>;

		explicit container_proxy(std::reference_wrapper<Container> container):
			m_current_iterator{selected_iterator<Container>::get_begin(container.get())},
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
			using diff_t = std::ranges::range_difference_t<Container>;
			auto const num_elems_to_pop = std::min(
				static_cast<diff_t>(count),
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

	template<class Container>
	container_proxy(std::reference_wrapper<Container> container) -> container_proxy<Container>;
};

#endif
