//@	{"target":{"name": "generic_value.test"}}

#include "./generic_value.hpp"

#include <flat_map>
#include <testfwk/testfwk.hpp>

namespace
{
	struct my_value_traits_with_variant
	{
		using key_type = std::string;
		using leaf_value_type = std::variant<std::string, double>;
	};

	struct my_value_traits_with_no_variant
	{
		using key_type = std::string;
		using leaf_value_type = std::string;
	};
}

TESTCASE(jopp2_create_empty_value)
{
	jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_variant> val_1;
	jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_no_variant> val_2;
}