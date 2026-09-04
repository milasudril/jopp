//@	{"target":{"name":"clear_nodes_visitor.test"}}

#include "./clear_nodes_visitor.hpp"
#include "lib/node_visitor_adaptor.hpp"

#include <testfwk/testfwk.hpp>
#include <testfwk/mock_util.hpp>

namespace
{
	struct thing_with_clear
	{
		TestFwk::mock_entry<void()> clear;
	};
}

TESTCASE(jopp2_clear_nodes_visitor_handle_leaf_value)
{
	jopp2::clear_nodes_visitor visitor;
	EXPECT_EQ(
		visitor.handle_leaf_value(123, jopp2::value_visitation_context{}),
		jopp2::node_visitor_status::ready
	)
}

TESTCASE(jopp2_clear_nodes_visitor_handle_simple_array)
{
	jopp2::clear_nodes_visitor visitor;
	thing_with_clear testobj;
	testobj.clear.expect_call_with_action([]{});
	EXPECT_EQ(
		visitor.handle_simple_array(testobj, jopp2::value_visitation_context{}),
		jopp2::node_visitor_status::ready
	)
}

TESTCASE(jopp2_clear_nodes_visitor_handle_begin_of_container)
{
	jopp2::clear_nodes_visitor visitor;
	thing_with_clear testobj;;
	EXPECT_EQ(
		visitor.handle_begin_of_container(testobj, jopp2::value_visitation_context{}),
		jopp2::node_visitor_status::ready
	)
}

TESTCASE(jopp2_clear_nodes_visitor_handle_end_of_container)
{
	jopp2::clear_nodes_visitor visitor;
	thing_with_clear testobj;
	testobj.clear.expect_call_with_action([]{});
	EXPECT_EQ(
		visitor.handle_end_of_container(testobj, jopp2::value_visitation_context{}),
		jopp2::node_visitor_status::ready
	)
}
