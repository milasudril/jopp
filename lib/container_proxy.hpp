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

		[[gnu::always_inline]] static constexpr auto get_begin(Range& container)
		{ return std::ranges::begin(container); }

		[[gnu::always_inline]] static constexpr auto get_end(Range& container)
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

		[[gnu::always_inline]] static constexpr auto get_begin(Range& container)
		{ return std::ranges::data(container); }

		[[gnu::always_inline]] static constexpr auto get_end(Range& container)
		{ return std::ranges::data(container) + std::ranges::size(container); }
	};

	template<class Container>
	class container_range
	{
	public:
		using range_type = std::ranges::subrange<
			typename selected_iterator<Container>::type,
			typename selected_iterator<Container>::type,
			std::ranges::subrange_kind::sized
		>;

		[[gnu::always_inline]]
		constexpr explicit container_range(std::reference_wrapper<Container> container):
			m_range{
				selected_iterator<Container>::get_begin(container.get()),
				selected_iterator<Container>::get_end(container.get()),
				std::ranges::size(container.get())
			}
		{}

		[[gnu::always_inline]] constexpr auto begin() const
		{ return std::ranges::begin(m_range); }

		[[gnu::always_inline]] constexpr auto end() const
		{ return std::ranges::end(m_range); }

		[[gnu::always_inline]] constexpr auto size() const
		{ return std::ranges::size(m_range); }

	private:
		range_type m_range;
	};

	template<class Container>
	struct container_wrapper:public container_range<Container>
	{
		using base = container_range<Container>;
		using base::container_range;
	};

	template<class Container>
	container_wrapper(std::reference_wrapper<Container> container) -> container_wrapper<Container>;

	template<class Container>
	requires(!std::is_const_v<Container>)
	class container_wrapper<Container>:public container_range<Container>
	{
	public:
		using base = container_range<Container>;

		explicit container_wrapper(std::reference_wrapper<Container> container):
			base{container},
			m_container{container}
		{}

		void clear()
		{
			m_container.get().clear();
			static_cast<base&>(*this) = base{m_container};
		}

		template<class Other>
		void replace_with(Other&& other)
		{
			m_container.get() = std::forward<Other>(other);
			static_cast<base&>(*this) = base{m_container};
		}

	private:
		std::reference_wrapper<Container> m_container;
	};

	template<class Container>
	class container_proxy
	{
	public:
		using iterator = selected_iterator<Container>::type;
		using active_range_type = std::ranges::subrange<iterator, iterator>;
		using value_type = typename Container::value_type;

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
			m_backing_store.clear();
			m_current_iterator = m_backing_store.begin();
		}

		template<class Other>
		requires(!std::is_const_v<Container>)
		constexpr void replace_backing_store(Other&& container)
		{
			m_backing_store.replace_with(std::forward<Other>(container));
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
