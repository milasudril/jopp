#ifndef JOPP2_TEMPLATE_PARAM_PACK_HPP
#define JOPP2_TEMPLATE_PARAM_PACK_HPP

#include <type_traits>
#include <cstddef>
#include <utility>

namespace jopp2
{
	template<class... Types>
	struct template_param_pack
	{ static constexpr auto size = sizeof...(Types); };



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
			{ throw "Type not found"; }
			else
			{ return get_index_of_type_impl<Current + 1, Which, Types...>(); }
		}
	}

	template<class Which, class Head, class... Types>
	inline size_t consteval get_index_of_type()
	{ return get_index_of_type_impl<0, Which, Head, Types...>(); }



	template<template<class...> class, class ... Types>
	struct map_template_param_pack_to_type
	{};

	template<template<class...> class TargetType, class... Types>
	struct map_template_param_pack_to_type<TargetType, template_param_pack<Types...>>
	{ using type = TargetType<Types...>; };

	template<template<class...> class TargetType, class... Types>
	using map_template_param_pack_to_type_t = map_template_param_pack_to_type<TargetType, Types...>::type;



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

	template<class T>
	using wrap_in_template_param_pack_t = wrap_in_template_param_pack<T>::type;



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

	template<class PackType, class... TypesToAppend>
	using append_to_template_param_pack_t = append_to_template_param_pack<PackType,TypesToAppend...>::type;



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

	template<class PackA, class PackB, class... Tail>
	using concatenate_template_param_packs_t = concatenate_template_param_packs<PackA, PackB, Tail...>::type;



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

	template<class PackType, template<class, class...> class Wrapper, class... OtherArgs>
	using wrap_template_param_pack_elements_t =
		wrap_template_param_pack_elements<PackType, Wrapper, OtherArgs...>::type;
}

#endif