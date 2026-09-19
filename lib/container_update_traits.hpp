#ifndef JOPP_CONTAINER_UPDATE_TRAITS_HPP
#define JOPP_CONTAINER_UPDATE_TRAITS_HPP

#include "./exception.hpp"
#include "./node_visitor_adaptor.hpp"

namespace jopp2
{
	template<class Container>
	class container_update_traits
	{
	public:
		using output_value_type = Container::value_type;

		template<class Rhs>
		static auto update(Container&, Rhs&&) = delete;

		template<class Rhs>
		requires(std::is_constructible_v<output_value_type, Rhs>)
		[[gnu::always_inline]] static output_value_type* update(Container& lhs, Rhs&& rhs)
		{
			lhs.emplace_back(std::forward<Rhs>(rhs));
			return &lhs.back();
		}

		template<class Rhs>
		requires instance_of<std::remove_cvref_t<Rhs> ,key_to_clone>
		[[gnu::always_inline]] [[noreturn]] static output_value_type* update(
			Container& /*unused*/,
			Rhs&& /*unsed*/
		)
		{
			raise_internal_error("Cannot store a key in a sequence container");
		}
	};
}

#endif
