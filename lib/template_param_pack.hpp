#ifndef JOPP2_TEMPLATE_PARAM_PACK_HPP
#define JOPP2_TEMPLATE_PARAM_PACK_HPP

namespace jopp2
{
	template<class ... Types>
	struct template_param_pack
	{};

	template<template<class...> class, class ... Types>
	struct map_template_param_pack_to_type
	{};

	template<template<class...> class TargetType, class... Types>
	struct map_template_param_pack_to_type<TargetType, template_param_pack<Types...>>
	{
		using type = TargetType<Types...>;
	};

	template<template<class...> class TargetType, class... Types>
	using map_template_param_pack_to_type_t = map_template_param_pack_to_type<TargetType, Types...>::type;

}

#endif