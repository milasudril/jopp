//@	{"target":{"name":"tree_walker.test"}}

#include "./tree_walker.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	struct test_generic_value
	{
		using value_type = std::variant<int>;
		using object = int;

		template<class T>
		using sequence_container_type = std::vector<T>;

		template<class Self>
		auto&& get_value(this Self&& self)
		{ return std::forward_like<Self>(self.value); }

		value_type value;
	};

	struct test_node_visitor
	{};
}

TESTCASE(jopp2_tree_walker_create_with_ref_to_visitor)
{
	test_generic_value value;
	test_node_visitor visitor;
	jopp2::tree_walker walker{value, visitor};
	EXPECT_EQ(&walker.visitor(), &visitor);
	EXPECT_EQ(walker.current_depth(), 1);
}
