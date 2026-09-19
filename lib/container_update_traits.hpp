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
		template<class Rhs>
		static auto update(Container&, Rhs&&) = delete;

#if 0
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
#endif
	};
}

#endif
