//@	{"target":{"name":"tree_walker.test"}}

#include "./tree_walker.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(jopp2_value_visitation_context_default_state)
{
	jopp2::value_visitation_context ctxt;
	EXPECT_EQ(ctxt.is_first_node(), true);
	EXPECT_EQ(ctxt.depth(), 0);
}
