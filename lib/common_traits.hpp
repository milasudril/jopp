#ifndef JOPP2_COMMON_TRAITS_HPP
#define JOPP2_COMMON_TRAITS_HPP

#include <type_traits>

/**
 * \defgroup typetraits (Common type traits)
 */

namespace jopp2
{
	/**
	 * \brief Tests whether or not T is an instantiations of C
	 * \ingroup typetraits
	 */
	template <typename T, template <typename...> class C>
	struct is_instantiation_of : std::false_type {};

	template <template <typename...> class C, typename... Args>
	struct is_instantiation_of<C<Args...>, C> : std::true_type {};

	/**
	 * \brief Helper alias for is_instantiation_of
	 */
	template <typename T, template <typename...> class C>
	inline constexpr bool is_instantiation_of_v = is_instantiation_of<T, C>::value;
}

#endif
