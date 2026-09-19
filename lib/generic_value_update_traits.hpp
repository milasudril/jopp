#ifndef JOPP2_GENERIC_VALUE_UPDATE_TRAITS_HPP
#define JOPP2_GENERIC_VALUE_UPDATE_TRAITS_HPP

#include "./exception.hpp"
#include "./container_proxy.hpp"
#include "lib/node_visitor_adaptor.hpp"
#include "lib/utils.hpp"

namespace jopp2
{
	using jopp::instance_of;

	template<class T>
	struct key_to_clone
	{
		using captured_type = T;
		T value;
	};

	template<class T>
	requires std::ranges::range<T>
	struct key_to_clone<T>
	{
		using captured_type = T;
		container_proxy<T const>::active_range_type value;
	};

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

		template<class Rhs>
		requires instance_of<std::remove_cvref_t<Rhs> ,key_to_clone>
		[[gnu::always_inline]] static GenericValueOut* update(
			GenericValueOut& /*unused*/,
			Rhs&& /*unsed*/
		)
		{
			raise_internal_error("Cannot store the given value in a GenericValue");
			return nullptr;
		}
	};
}

#endif
