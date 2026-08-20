//@	{"target":{"name":"tree_walker.test"}}

#include "./tree_walker.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	struct test_generic_value
	{};

	struct test_node_visitor
	{};
}

TESTCASE(jopp2_tree_walker_instantiate_with_dummy)
{
	test_generic_value value;
	test_node_visitor visitor;
	jopp2::tree_walker walker{value, visitor};
}
