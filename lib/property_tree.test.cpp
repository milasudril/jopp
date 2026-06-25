//@	{"target":{"name": "property_tree.test"}}

#include "./property_tree.hpp"

#include <flat_map>
#include <testfwk/testfwk.hpp>

namespace
{
	struct my_value_traits
	{
		using key_type = std::string;
	};
}

TESTCASE(jopp2_create_empty_value)
{
	jopp2::generic_value<std::flat_map, std::vector, my_value_traits> val;
}