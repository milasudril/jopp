#ifndef JOPP2_COMMON_TRAITS_HPP
#define JOPP2_COMMON_TRAITS_HPP

#include <type_traits>

/**
 * \defgroup typetraits Common type traits
 */

namespace jopp2
{
	/**
	 * \brief Tests whether or not T is an instantiations of C
	 * \ingroup typetraits
	 */
	template <class T, template<class...> class C>
	consteval bool is_instantiation_of()
	{
		return requires {
			[]<class... Args>(const C<Args...>&) {}(std::declval<T>());
		};
	}

	/**
	 * \brief Adds the constness of Source to Target
	 * \ingroup typetraits
	 */
	template<class Source, class Target>
	struct add_const_from
	{ using type = std::conditional_t<std::is_const_v<Source>, Target const, Target>; };

	/**
	 * \brief Helper alias for add_const_from
	 * \ingroup typetraits
	 */
	template<class Source, class Target>
	using add_const_from_t = add_const_from<Source, Target>::type;

	/**
	 * \brief Adds the lref property of Source to Target
	 * \ingroup typetraits
	 */
	template<class Source, class Target>
	struct add_lref_from
	{ using type = std::conditional_t<std::is_lvalue_reference_v<Source>, Target&, Target>; };

	/**
	 * \brief Helper alias for add_lref_from
	 * \ingroup typetraits
	 */
	template<class Source, class Target>
	using add_lref_from_t = add_lref_from<Source, Target>::type;

	/**
	 * \brief Adds the rref property of Source to Target
	 * \ingroup typetraits
	 */
	template<class Source, class Target>
	struct add_rref_from
	{ using type = std::conditional_t<std::is_reference_v<Source>, Target&&, Target>; };

	/**
	 * \brief Helper alias for add_rref_from
	 * \ingroup typetraits
	 */
	template<class Source, class Target>
	using add_rref_from_t = add_rref_from<Source, Target>::type;

	/**
	 * \brief Adds the constness and ref property of Source to Target
	 * \ingroup typetraits
	 */
	template<class Source, class Target>
	struct add_cref_from
	{
		using type = add_rref_from_t<
			Source,
			add_lref_from_t<
				Source,
				add_const_from_t<std::remove_reference_t<Source>, Target>>
			>;
	};

	/**
	 * \brief Helper alias for add_cref_from
	 * \ingroup typetraits
	 */
	template <class Source, class Target>
	using add_cref_from_t = add_cref_from<Source, Target>::type;

	static_assert(std::is_same_v<add_cref_from_t<int, int>, int>);
	static_assert(std::is_same_v<add_cref_from_t<int const, int>, int const>);
	static_assert(std::is_same_v<add_cref_from_t<int const&, int>, int const&>);
	static_assert(std::is_same_v<add_cref_from_t<int const&&, int>, int const&&>);
	static_assert(std::is_same_v<add_cref_from_t<int&&, int>, int&&>);


	/**
	 * \brief Tests whether or not calling Callable with Args returns result
	 * \return True if it will return Result, otherwise false, including the case when Callable cannot
	 * be invoked with Args at all.
	 * \ingroup typetraits
	 */
	template <class Result, class Callable, class... Args>
	consteval bool is_invokable_with_result()
	{
		if constexpr (std::is_invocable_v<Callable, Args...>)
		{ return std::is_same_v<std::invoke_result_t<Callable, Args...>, Result>; }
		else
		{ return false;}
	}
}

#endif
