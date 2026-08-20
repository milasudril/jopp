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
	{
		int test_value;
	};
}

TESTCASE(jopp2_tree_walker_create_with_ref_to_visitor)
{
	test_generic_value value;
	test_node_visitor visitor;
	jopp2::tree_walker walker{value, visitor};
	EXPECT_EQ(&walker.visitor(), &visitor);
	EXPECT_EQ(walker.current_depth(), 1);
	static_assert(
		std::is_same_v<
			decltype(walker),
			jopp2::tree_walker<test_generic_value&, test_node_visitor&>
		>
	);
}

TESTCASE(jopp2_tree_walker_create_with_visitor_by_value)
{
	test_generic_value value;
	jopp2::tree_walker walker{value, test_node_visitor{123}};
	EXPECT_EQ(walker.visitor().test_value, 123);
	EXPECT_EQ(walker.current_depth(), 1);
	static_assert(
		std::is_same_v<
			decltype(walker),
			jopp2::tree_walker<test_generic_value&, test_node_visitor>
		>
	);
}

TESTCASE(jopp2_tree_walker_create_with_visitor_by_value_in_place)
{
	test_generic_value value;
	jopp2::tree_walker walker{value, std::in_place_type_t<test_node_visitor>{}, 56};
	EXPECT_EQ(walker.visitor().test_value, 56);
	EXPECT_EQ(walker.current_depth(), 1);
	static_assert(
		std::is_same_v<
			decltype(walker),
			jopp2::tree_walker<test_generic_value&, test_node_visitor>
		>
	);
}

TESTCASE(jopp2_tree_walker_create_with_ref_to_const_value)
{
	test_generic_value value;
	test_node_visitor visitor;
	jopp2::tree_walker walker{std::as_const(value), visitor};
	EXPECT_EQ(&walker.visitor(), &visitor);
	EXPECT_EQ(walker.current_depth(), 1);
	static_assert(
		std::is_same_v<
			decltype(walker),
			jopp2::tree_walker<test_generic_value const&, test_node_visitor&>
		>
	);
}

TESTCASE(jopp2_tree_walker_create_with_value)
{
	static_assert(!
		std::is_constructible_v<
			jopp2::tree_walker<test_generic_value const, test_node_visitor&>,
			test_generic_value,
			test_node_visitor
		>
	);
}
