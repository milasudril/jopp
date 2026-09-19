#ifndef JOPP2_GENERIC_VALUE_UPDATE_TRAITS_HPP
#define JOPP2_GENERIC_VALUE_UPDATE_TRAITS_HPP

#include "./exception.hpp"
#include "./node_visitor_adaptor.hpp"

namespace jopp2
{
	template<class GenericValueOut>
	struct generic_value_update_traits
	{
		template<class Rhs>
		requires(std::is_constructible_v<GenericValueOut, Rhs>)
		[[gnu::always_inline]] static GenericValueOut* update(GenericValueOut& lhs, Rhs&& rhs)
		{
			lhs = GenericValueOut(std::forward<Rhs>(rhs));
			return &lhs;
		}

		template<class Rhs>
		static auto update(GenericValueOut&, Rhs&&) = delete;

		template<class Rhs>
		requires instance_of<std::remove_cvref_t<Rhs> ,key_to_clone>
		[[gnu::always_inline]] [[noreturn]] static GenericValueOut* update(
			GenericValueOut& /*unused*/,
			Rhs&& /*unsed*/
		)
		{ raise_internal_error("Cannot store a key in a GenericValue"); }
	};
}

#endif
