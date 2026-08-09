#ifndef JOPP2_CONSUMABLE_RANGE_HPP
#define JOPP2_CONSUMABLE_RANGE_HPP

#include <cstddef>
#include <iterator>
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

	template<class IterType>
	class consumable_range: private range_size<consumable_range<IterType>>
	{
		using base = range_size<consumable_range<IterType>>;
	public:
		using base::size;

		using iterator = IterType;
		using value_type = std::iter_value_t<IterType>;

		template<std::ranges::forward_range Range>
		requires(std::ranges::borrowed_range<Range> || std::is_lvalue_reference_v<Range>)
		constexpr explicit consumable_range(Range&& range) noexcept:
			base{range},
			m_begin{std::ranges::begin(range)},
			m_end{std::ranges::end(range)}
		{}

		template<std::ranges::forward_range Range>
		requires (!std::ranges::borrowed_range<Range> && !std::is_lvalue_reference_v<Range>)
		consumable_range(Range&&) = delete;

		constexpr auto begin() const
		{ return m_begin; }

		constexpr auto end() const
		{ return m_end; }

		constexpr auto& pop_element()
		{
			if constexpr(std::random_access_iterator<IterType>)
			{
				++m_begin;
				return *this;
			}
			else
			{
				++m_begin;
				--base::m_size;
				return *this;
			}
		}

		constexpr auto& pop_elements(size_t count)
		{
			auto const num_elems_to_pop = std::min(count, size());
			if constexpr(std::random_access_iterator<IterType>)
			{
				m_begin += num_elems_to_pop;
				return *this;
			}
			else
			{
				std::ranges::advance(m_begin, num_elems_to_pop);
				base::m_size -= num_elems_to_pop;
				return *this;
			}
		}

	private:
		IterType m_begin;
		IterType m_end;
	};
};

#endif
