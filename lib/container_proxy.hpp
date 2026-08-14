#ifndef JOPP2_CONTAINER_PROXY_HPP
#define JOPP2_CONTAINER_PROXY_HPP

#include <cstddef>
#include <ranges>
#include <type_traits>

namespace jopp2
{
	template<class Container>
	struct container_wrapper
	{
		explicit container_wrapper(Container& /*unused*/){}
	};

	template<class Container>
	requires(!std::is_const_v<Container>)
	struct container_wrapper<Container>:std::reference_wrapper<Container>
	{
		using std::reference_wrapper<Container>::reference_wrapper;
	};

	template<class Range>
	struct selected_iterator
	{
		using type = std::ranges::iterator_t<Range>;
	};

	template<class Container>
	class container_proxy
	{
	public:
		using iterator = selected_iterator<Container>::type;
		using active_range_type = std::ranges::subrange<
			iterator,
			iterator,
			std::ranges::subrange_kind::sized
		>;

		explicit container_proxy(Container& container):
			m_active_range{container},
			m_backing_store{container}
		{}

		constexpr auto active_range() const
		{ return m_active_range; }

		constexpr auto& pop_active_element()
		{ return pop_active_elements(1); }

		constexpr auto& pop_active_elements(size_t count)
		{
			auto const num_elems_to_pop = static_cast<ssize_t>(std::min(count, m_active_range.size()));
			m_active_range.advance(num_elems_to_pop);
			return *this;
		}

		template<class C=void>
		requires(!std::is_const_v<Container>)
		void clear_backing_store()
		{
			m_backing_store.get().clear();
			m_active_range = active_range_type{};
		}

		template<class Other>
		requires(!std::is_const_v<Container>)
		void replace_backing_store(Other&& container)
		{
			m_backing_store.get() = std::forward<Other>(container);
			m_active_range = std::ranges::subrange{m_backing_store.get()};
		}

	private:
		active_range_type m_active_range;
		[[no_unique_address]] container_wrapper<Container> m_backing_store;
	};
};

#endif
