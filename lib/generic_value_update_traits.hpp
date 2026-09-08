#ifndef JOPP2_GENERIC_VALUE_UPDATE_TRAITS_HPP
#define JOPP2_GENERIC_VALUE_UPDATE_TRAITS_HPP

#include "./exception.hpp"

namespace jopp2
{
	template<class GenericValueOut>
	struct generic_value_update_traits
	{
		template<class Rhs>
		requires(std::is_constructible_v<GenericValueOut, Rhs>)
		[[gnu::always_inline]] static auto update(GenericValueOut& lhs, Rhs&& rhs)
		{ lhs = GenericValueOut(std::forward<Rhs>(rhs)); }

		template<class Rhs>
		[[gnu::always_inline]] static auto update(
			GenericValueOut& /*unused*/,
			Rhs&& /*unsed*/
		)
		{ raise_internal_error("Cannot store the given value in a GenericValue"); }
	};
}

#endif
