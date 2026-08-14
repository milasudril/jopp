#ifndef JOPP2_CONTAINER_PROXY_HPP
#define JOPP2_CONTAINER_PROXY_HPP

#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>

namespace jopp2
{
	template<class RangeType>
	class range_size
	{
	public:
		friend RangeType;

		template<std::ranges::sized_range R>
		explicit range_size(R const& r):
			m_size{std::ranges::size(r)}
		{}

		constexpr auto size() const
		{ return m_size; }

	private:
		size_t m_size;
	};

	template<class RangeType>
	requires(std::random_access_iterator<typename RangeType::iterator>)
	class range_size<RangeType>
	{
	public:
		template<class R>
		explicit range_size(R const&/*unused*/){}

		template<class Self>
		constexpr auto size(this Self const& self)
		{ return self.end() - self.begin(); }
	};

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

	template<class Container>
	class container_proxy:
		private range_size<container_proxy<Container>>
	{
		using base = range_size<container_proxy<std::ranges::iterator_t<Container>>>;
	public:
		using base::size;
		using iterator = std::conditional_t<
			std::ranges::contiguous_range<Container>,
			decltype(std::ranges::data(std::declval<Container&>())),
			std::ranges::iterator_t<Container>
		>;

		explicit container_proxy(Container& container):
			base{container},
			m_active_range{container},
			m_backing_store{container}
		{}

		constexpr auto active_range() const
		{ return m_active_range; }

		constexpr auto& pop_active_element()
		{ return pop_active_elements(); }

		constexpr auto& pop_active_elements(size_t count)
		{
			auto const num_elems_to_pop = std::min(count, size());
			if constexpr(std::random_access_iterator<iterator>)
			{
				m_active_range.advance(num_elems_to_pop);
				return *this;
			}
			else
			{
				m_active_range.advance(num_elems_to_pop);
				base::m_size -= num_elems_to_pop;
				return *this;
			}
		}

		void clear_backing_store()
		requires(!std::is_const_v<Container>)
		{
			m_backing_store.get().clear();
			m_active_range = std::ranges::subrange<iterator>{};
		}

		void replace_backing_store(Container&& container)
		requires(!std::is_const_v<Container>)
		{
			m_backing_store.get() = std::move(container);
			m_active_range = std::ranges::subrange<iterator>{container};
		}

	private:
		std::ranges::subrange<iterator> m_active_range;
		[[no_unique_address]] container_wrapper<Container> m_backing_store;
	};
};

#endif
