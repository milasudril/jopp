#ifndef JOPP2_TEMPLATE_PARAM_PACK_HPP
#define JOPP2_TEMPLATE_PARAM_PACK_HPP

#include <type_traits>
#include <cstddef>
#include <utility>
#include <limits>

/**
 * \file template_param_pack.hpp
 * \brief Contains helpers for managing template parameter packs
 */

namespace jopp2
{
	/**
	 * \brief A class for holding a pack
	 */
	template<class... Types>
	struct template_param_pack
	{ static constexpr auto size = sizeof...(Types); };


	/**
	 * \brief Queries TemplateParamPack which type is stored at Index
	 */
	template<size_t Index, class TemplateParamPack>
	struct template_param_pack_type_at_index
	{};

	template<size_t Index, class Head, class... Tail>
	struct template_param_pack_type_at_index<Index, template_param_pack<Head, Tail...>>
	{
		using type = typename template_param_pack_type_at_index<Index - 1, template_param_pack<Tail...>>::type;
	};

	template<class Head, class... Tail>
	struct template_param_pack_type_at_index<0, template_param_pack<Head, Tail...>>
	{ using type = Head; };

	/**
	 * \brief Helper type for using template_param_pack_type_at_index
	 */
	template<size_t Index, class TemplateParamPack>
	using template_param_pack_type_at_index_t = template_param_pack_type_at_index<Index, TemplateParamPack>::type;



	template<size_t Current, class Which, class Head, class... Types>
	inline size_t consteval get_index_of_type_impl()
	{
		if constexpr(std::is_same_v<Which, Head>)
		{ return Current; }
		else
		{
			if constexpr(sizeof...(Types) == 0)
			{ return std::numeric_limits<size_t>::max(); }
			else
			{ return get_index_of_type_impl<Current + 1, Which, Types...>(); }
		}
	}

	/**
	 * \brief Queries the first index that matches the Which
	 */
	template<class Which, class Head, class... Types>
	inline size_t consteval get_index_of_type()
	{ return get_index_of_type_impl<0, Which, Head, Types...>(); }

	template<class Which, class ... Types>
	struct index_of_type
	{};

	template<class Which, class ... Types>
	struct index_of_type<Which, template_param_pack<Types...>>
	{
		static constexpr auto value = get_index_of_type<Which, Types...>();
	};

	template<class Which, class ... Types>
	inline constexpr auto index_of_type_v = index_of_type<Which, Types...>::value;

	template<class Which, class... Types>
	inline constexpr auto type_is_present_v =
		index_of_type_v<Which, Types...> != std::numeric_limits<size_t>::max();



	template<template<class...> class, class ... Types>
	struct map_template_param_pack_to_type
	{};

	/**
	 * \brief Maps a template parameter pack to a type that accepts multiple class template arguments
	 *
	 * map_template_param_pack_to_type can be used like so:
	 *
	 * ```
	 * using my_pack = template_param_pack<int, double>;
	 * using my_variant = map_template_param_pack_to_type_t<std::varaint, my_pack>;
	 * ```
	 *
	 * In this case, `my_variant` becomes `std::variant<int, double>`
	 */
	template<template<class...> class TargetType, class... Types>
	struct map_template_param_pack_to_type<TargetType, template_param_pack<Types...>>
	{ using type = TargetType<Types...>; };

	/**
	 * \brief Helper type for using map_template_param_pack_to_type
	 */
	template<template<class...> class TargetType, class... Types>
	using map_template_param_pack_to_type_t = map_template_param_pack_to_type<TargetType, Types...>::type;


	/**
	 * \brief Conditionally wraps T inside a template_param_pack
	 * \returns template_param_pack<T> if T is not already a a template_param_pack, otherwise it is
	 *          the identity function
	 */
	template<class T>
	struct wrap_in_template_param_pack
	{
		using type = template_param_pack<T>;
	};

	template<class ... Args>
	struct wrap_in_template_param_pack<template_param_pack<Args...>>
	{
		using type = template_param_pack<Args...>;
	};

	/**
	 * \brief Helper type for using wrap_in_template_param_pack
	 */
	template<class T>
	using wrap_in_template_param_pack_t = wrap_in_template_param_pack<T>::type;


	/**
	 * \brief Appends TypesToAppend to a template_param_pack
	 */
	template<class PackType, class... TypesToAppend>
	struct append_to_template_param_pack
	{
	private:
		template<size_t... I>
		static consteval auto resolve_type(std::index_sequence<I...>)
		{
			return std::type_identity<
				template_param_pack<
					template_param_pack_type_at_index_t<I, PackType>...,
					TypesToAppend...
				>
			>{};
		}
	public:
		using type = decltype(
			resolve_type(std::make_index_sequence<PackType::size>{})
		)::type;
	};

	/**
	 * \brief Helper type for using append_to_template_param_pack
	 */
	template<class PackType, class... TypesToAppend>
	using append_to_template_param_pack_t = append_to_template_param_pack<PackType,TypesToAppend...>::type;



	/**
	 * \brief Concatenates PackA and PackB
	 */
	template<class PackA, class PackB, class... Tail>
	struct concatenate_template_param_packs
	{
	private:
		template<size_t... I>
		static consteval auto resolve_type(std::index_sequence<I...>)
		{
			using next_type = append_to_template_param_pack_t<
				PackA,
				template_param_pack_type_at_index_t<I, PackB>...
			>;

			if constexpr(sizeof...(Tail) == 0)
			{ return std::type_identity<next_type>{}; }
			else
			{ return std::type_identity<typename concatenate_template_param_packs<next_type, Tail...>::type>{}; }
		}
	public:
		using type = decltype(
			resolve_type(std::make_index_sequence<PackB::size>{})
		)::type;
	};

	/**
	 * \brief Helper type for using concatenate_template_param_packs
	 */
	template<class PackA, class PackB, class... Tail>
	using concatenate_template_param_packs_t = concatenate_template_param_packs<PackA, PackB, Tail...>::type;


	/**
	 * \brief Wraps each element of a template_param_pack, and wraps it inside wrapper
	 * \tparam PackType the template_param_pack instantiation to use as source for the types
	 * \tparam Wrapper type class template to instantiate with the types
	 * \tparam OtherArgs other arguments to pass to wrapper
	 *
	 * wrap_template_param_pack_elements can be used like so
	 *
	 * ```
	 * using my_pack = template_param_pack<int, double>;
	 * using my_other_pack = wrap_template_param_pack_elements_t<my_pack, std::pair, int>;
	 * ```
	 *
	 * Now my_other_pack is `template_param_pack<std::pair<int, int>, std::pair<double, int>>`
	 */
	template<class PackType, template<class, class...> class Wrapper, class... OtherArgs>
	struct wrap_template_param_pack_elements
	{
	private:
		template<size_t... I>
		static consteval auto resolve_type(std::index_sequence<I...>)
		{
			return std::type_identity<
				template_param_pack<
					Wrapper<template_param_pack_type_at_index_t<I, PackType>, OtherArgs...>...
				>
			>{};
		}

	public:
		using type = decltype(
			resolve_type(std::make_index_sequence<PackType::size>{})
		)::type;
	};

	/**
	 * \brief Helper type for using wrap_template_param_pack_elements
	 */
	template<class PackType, template<class, class...> class Wrapper, class... OtherArgs>
	using wrap_template_param_pack_elements_t =
		wrap_template_param_pack_elements<PackType, Wrapper, OtherArgs...>::type;
}

#endif
