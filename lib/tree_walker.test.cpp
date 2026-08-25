//@	{"target":{"name":"tree_walker.test"}}

#include "./tree_walker.hpp"
#include "lib/container_proxy.hpp"
#include <testfwk/death_test.hpp>
#include <testfwk/mock_util.hpp>

#include <testfwk/testfwk.hpp>

namespace
{
	struct test_generic_value
	{
		using value_type = std::variant<int>;
		using object = int;

		template<class T>
		using sequence_container_type = std::vector<T>;

		template<class T>
		static constexpr auto is_leaf_value = std::is_same_v<T, int>;

		template<class Self>
		auto&& get_value(this Self&& self)
		{ return std::forward_like<Self>(self.value); }

		value_type value;
	};

	struct test_node_visitor
	{
		int test_value{};

		test_node_visitor() = default;

		explicit test_node_visitor(int val):
			test_value{val}
		{}

		TestFwk::mock_entry<jopp2::node_visitor_status(int, jopp2::value_visitation_context)>
			handle_leaf_value;

		TestFwk::mock_entry<
			jopp2::node_visitor_status(
				jopp2::container_proxy<std::vector<int>>&,
				jopp2::value_visitation_context const& ctxt
			)
		> handle_leaf_value_array;

		TestFwk::mock_entry<
			jopp2::node_visitor_status(
				jopp2::container_proxy<std::vector<test_generic_value> const>&,
				jopp2::value_visitation_context const& ctxt
			)
		> handle_begin_of_container_generic_value;

		jopp2::node_visitor_status handle_begin_of_container(
			jopp2::container_proxy<std::vector<test_generic_value> const>& val,
			jopp2::value_visitation_context const& ctxt
		)
		{
			return handle_begin_of_container_generic_value(val, ctxt);
		}

		TestFwk::mock_entry<
			jopp2::node_visitor_status(
				jopp2::container_proxy<std::vector<test_generic_value> const>&,
				jopp2::value_visitation_context const& ctxt
			)
		> handle_end_of_container_generic_value;

		jopp2::node_visitor_status handle_end_of_container(
			jopp2::container_proxy<std::vector<test_generic_value> const>& val,
			jopp2::value_visitation_context const& ctxt
		)
		{
			return handle_end_of_container_generic_value(val, ctxt);
		}
	};
}

TESTCASE(jopp2_value_visitation_context_default_state)
{
	jopp2::value_visitation_context ctxt;
	EXPECT_EQ(ctxt.is_first_node(), true);
	EXPECT_EQ(ctxt.depth(), 0);
	EXPECT_EQ(ctxt.parent_container_size(), 0);
	EXPECT_EQ(ctxt.node_index(), 0);
}

TESTCASE(jopp2_value_visitation_context_enter_next_level)
{
	jopp2::value_visitation_context const ctxt;
	EXPECT_EQ(ctxt.is_first_node(), true);
	EXPECT_EQ(ctxt.depth(), 0);
	EXPECT_EQ(ctxt.parent_container_size(), 0);
	EXPECT_EQ(ctxt.node_index(), 0);

	auto next_level = ctxt.enter_next_level(23);
	EXPECT_EQ(next_level.is_first_node(), true);
	EXPECT_EQ(next_level.depth(), 1);
	EXPECT_EQ(next_level.is_last_node(), false);
	EXPECT_EQ(next_level.parent_container_size(), 23);
	EXPECT_EQ(next_level.node_index(), 0);

	for(size_t k = 0; k != 22; ++k)
	{
		EXPECT_EQ(next_level.is_last_node(), false);
		EXPECT_EQ(next_level.node_index(), k);
		next_level.step_node_index();
	}
	EXPECT_EQ(next_level.node_index(), 22);
	EXPECT_EQ(next_level.is_last_node(), true);

	auto const third_level = next_level.enter_next_level(3);;
	EXPECT_EQ(third_level.node_index(), 0);
	EXPECT_EQ(third_level.depth(), 2);
}

TESTCASE(jopp2_tree_walker_create_with_ref_to_visitor)
{
	test_generic_value value;
	test_node_visitor visitor{};
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
	test_node_visitor visitor{};
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

TESTCASE(jopp2_tree_walker_dispatch_leaf_value_return_ready)
{
	test_generic_value value;
	test_node_visitor visitor{};
	jopp2::tree_walker walker{std::as_const(value), visitor};

	visitor.handle_leaf_value.expect_call_with_action(
		[](int value, jopp2::value_visitation_context const& ctxt){
			EXPECT_EQ(value, 123);
			EXPECT_EQ(ctxt, jopp2::value_visitation_context{}.enter_next_level(42));
			return jopp2::node_visitor_status::ready;
		}
	);
	auto const res = walker.dispatch<int>(
		123,
		jopp2::value_visitation_context{}.enter_next_level(42)
	);
	EXPECT_EQ(res, jopp2::visit_node_result::completed);
}

TESTCASE(jopp2_tree_walker_dispatch_leaf_value_return_suspended)
{
	test_generic_value value;
	test_node_visitor visitor{};
	jopp2::tree_walker walker{std::as_const(value), visitor};

	visitor.handle_leaf_value.expect_call_with_action(
		[](int value, jopp2::value_visitation_context const& ctxt){
			EXPECT_EQ(value, 123);
			EXPECT_EQ(ctxt, jopp2::value_visitation_context{}.enter_next_level(42));
			return jopp2::node_visitor_status::suspended;
		}
	);

	auto const res = walker.dispatch<int>(
		123,
		jopp2::value_visitation_context{}.enter_next_level(42)
	);
	EXPECT_EQ(res, jopp2::visit_node_result::node_visitor_suspended);
}

TESTCASE(jopp2_tree_walker_dispatch_leaf_value_return_junk)
{
	test_generic_value value;
	test_node_visitor visitor{};
	jopp2::tree_walker walker{std::as_const(value), visitor};

	visitor.handle_leaf_value.expect_call_with_action(
		[](int value, jopp2::value_visitation_context const& ctxt){
			EXPECT_EQ(value, 123);
			EXPECT_EQ(ctxt, jopp2::value_visitation_context{}.enter_next_level(42));
			return static_cast<jopp2::node_visitor_status>(34);
		},
		TestFwk::expectation_options{
			.cardinality = TestFwk::cardinality_constraint::exactly(1),
			.count_with_fd = true
		}
	);
	TestFwk::expect_death(
		[&walker](){
			std::ignore = walker.dispatch<int>(123,jopp2::value_visitation_context{}.enter_next_level(42));
		},
		"jopp internal error: lib/./tree_walker.hpp:244: Invalid return value from node visitor\n",
		SIGABRT
	);
}

TESTCASE(jopp2_tree_walker_dispatch_leaf_value_array_return_ready)
{
	std::vector<int> vals{1, 2, 3};
	jopp2::container_proxy vals_proxy{std::ref(vals)};
	test_generic_value value;
	test_node_visitor visitor{};
	jopp2::tree_walker walker{std::as_const(value), visitor};

	visitor.handle_leaf_value_array.expect_call_with_action(
		[callcount = static_cast<size_t>(0), &vals](
			jopp2::container_proxy<std::vector<int>>& obj,
			jopp2::value_visitation_context const& ctxt
		) mutable {
			EXPECT_EQ(ctxt, jopp2::value_visitation_context{}.enter_next_level(42));
			REQUIRE_LT(callcount, std::size(vals));
			EXPECT_EQ(obj.total_size(), std::size(vals));
			EXPECT_EQ(*obj.active_range().begin(), vals[callcount]);
			obj.pop_active_element();
			++callcount;
			return jopp2::node_visitor_status::ready;
		},
		TestFwk::expectation_options{
			.cardinality = TestFwk::cardinality_constraint::exactly(3)
		}
	);

	for(size_t k = 0; k != std::size(vals) - 1; ++k)
	{
		auto const res = walker.dispatch(
			vals_proxy,
			jopp2::value_visitation_context{}.enter_next_level(42)
		);
		EXPECT_EQ(res, jopp2::visit_node_result::node_visitor_ready);
	}

	auto const res = walker.dispatch(
		vals_proxy,
		jopp2::value_visitation_context{}.enter_next_level(42)
	);
	EXPECT_EQ(res, jopp2::visit_node_result::completed);
}

TESTCASE(jopp2_tree_walker_dispatch_leaf_value_array_return_suspended)
{
	std::vector<int> vals{1, 2, 3};
	jopp2::container_proxy vals_proxy{std::ref(vals)};
	test_generic_value value;
	test_node_visitor visitor{};
	jopp2::tree_walker walker{std::as_const(value), visitor};

	visitor.handle_leaf_value_array.expect_call_with_action(
		[](
			jopp2::container_proxy<std::vector<int>>& ,
			jopp2::value_visitation_context const& ctxt
		) {
			EXPECT_EQ(ctxt, jopp2::value_visitation_context{}.enter_next_level(42));
			return jopp2::node_visitor_status::suspended;
		}
	);
	auto const res = walker.dispatch(
		vals_proxy,
		jopp2::value_visitation_context{}.enter_next_level(42)
	);
	EXPECT_EQ(res, jopp2::visit_node_result::node_visitor_suspended);
}

TESTCASE(jopp2_tree_walker_dispatch_leaf_value_array_return_junk)
{
	std::vector<int> vals{1, 2, 3};
	jopp2::container_proxy vals_proxy{std::ref(vals)};
	test_generic_value value;
	test_node_visitor visitor{};
	jopp2::tree_walker walker{std::as_const(value), visitor};

	visitor.handle_leaf_value_array.expect_call_with_action(
		[](
			jopp2::container_proxy<std::vector<int>>& ,
			jopp2::value_visitation_context const& ctxt
		){
			EXPECT_EQ(ctxt, jopp2::value_visitation_context{}.enter_next_level(42));
			return static_cast<jopp2::node_visitor_status>(34);
		},
		TestFwk::expectation_options{
			.cardinality = TestFwk::cardinality_constraint::exactly(1),
			.count_with_fd = true
		}
	);

	TestFwk::expect_death(
		[&walker, &vals_proxy](){
			std::ignore = walker.dispatch(
				vals_proxy,
				jopp2::value_visitation_context{}.enter_next_level(42)
			);
		},
		"jopp internal error: lib/./tree_walker.hpp:264: Invalid return value from node visitor\n",
		SIGABRT
	);
}

TESTCASE(jopp2_tree_walker_dispatch_generic_value_array_cursor_at_begin_visitor_suspended)
{
	std::vector<test_generic_value> vals{
		test_generic_value{},
		test_generic_value{},
		test_generic_value{}
	};
	jopp2::container_proxy vals_proxy{std::cref(vals)};
	test_generic_value value;
	test_node_visitor visitor{};
	jopp2::tree_walker walker{std::as_const(value), visitor};
	auto visitation_ctxt = jopp2::value_visitation_context{}.enter_next_level(42);

	visitor.handle_begin_of_container_generic_value.expect_call_with_action(
		[visitation_ctxt, &vals] (
			jopp2::container_proxy<std::vector<test_generic_value> const>& obj,
			jopp2::value_visitation_context const& ctxt
		)
		{
			EXPECT_EQ(obj.at_begin(), true);
			EXPECT_EQ(obj.active_range().begin(), std::data(vals));
			EXPECT_EQ(ctxt, visitation_ctxt);
			return jopp2::node_visitor_status::suspended;
		}
	);
	auto const result = walker.dispatch(vals_proxy, visitation_ctxt);
	EXPECT_EQ(result, jopp2::visit_node_result::node_visitor_suspended);
}

TESTCASE(jopp2_tree_walker_dispatch_generic_value_array_cursor_at_end_visitor_suspended)
{
	std::vector<test_generic_value> vals{
		test_generic_value{},
		test_generic_value{},
		test_generic_value{}
	};
	jopp2::container_proxy vals_proxy{std::cref(vals)};
	vals_proxy.pop_active_elements(std::size(vals));
	test_generic_value value;
	test_node_visitor visitor{};
	jopp2::tree_walker walker{std::as_const(value), visitor};
	auto visitation_ctxt = jopp2::value_visitation_context{}.enter_next_level(42);

	visitor.handle_end_of_container_generic_value.expect_call_with_action(
		[visitation_ctxt, &vals] (
			jopp2::container_proxy<std::vector<test_generic_value> const>& obj,
			jopp2::value_visitation_context const& ctxt
		)
		{
			EXPECT_EQ(obj.at_end(), true);
			EXPECT_EQ(obj.active_range().begin(), std::data(vals) + std::size(vals));
			EXPECT_EQ(ctxt, visitation_ctxt);
			return jopp2::node_visitor_status::suspended;
		}
	);
	auto const result = walker.dispatch(vals_proxy, visitation_ctxt);
	EXPECT_EQ(result, jopp2::visit_node_result::node_visitor_suspended);
}

TESTCASE(jopp2_tree_walker_dispatch_generic_value_array_cursor_at_end_visitor_ready)
{
	std::vector<test_generic_value> vals{
		test_generic_value{},
		test_generic_value{},
		test_generic_value{}
	};
	jopp2::container_proxy vals_proxy{std::cref(vals)};
	vals_proxy.pop_active_elements(std::size(vals));
	test_generic_value value;
	test_node_visitor visitor{};
	jopp2::tree_walker walker{std::as_const(value), visitor};
	auto visitation_ctxt = jopp2::value_visitation_context{}.enter_next_level(42);

	visitor.handle_end_of_container_generic_value.expect_call_with_action(
		[visitation_ctxt, &vals] (
			jopp2::container_proxy<std::vector<test_generic_value> const>& obj,
			jopp2::value_visitation_context const& ctxt
		)
		{
			EXPECT_EQ(obj.at_end(), true);
			EXPECT_EQ(obj.active_range().begin(), std::data(vals) + std::size(vals));
			EXPECT_EQ(ctxt, visitation_ctxt);
			return jopp2::node_visitor_status::ready;
		}
	);
	auto const result = walker.dispatch(vals_proxy, visitation_ctxt);
	EXPECT_EQ(result, jopp2::visit_node_result::completed);
}

TESTCASE(jopp2_tree_walker_dispatch_generic_value_array_cursor_empty_calls_begin_first)
{
	std::vector<test_generic_value> vals{};
	jopp2::container_proxy vals_proxy{std::cref(vals)};
	test_generic_value value;
	test_node_visitor visitor{};
	jopp2::tree_walker walker{std::as_const(value), visitor};
	auto visitation_ctxt = jopp2::value_visitation_context{}.enter_next_level(42);

	EXPECT_EQ(vals_proxy.at_begin(), true);
	EXPECT_EQ(vals_proxy.at_end(), true);

	bool called = false;
	visitor.handle_begin_of_container_generic_value.expect_call_with_action(
		[visitation_ctxt, &vals, &called] (
			jopp2::container_proxy<std::vector<test_generic_value> const>& obj,
			jopp2::value_visitation_context const& ctxt
		)
		{
			called = true;
			EXPECT_EQ(obj.at_begin(), true);
			EXPECT_EQ(obj.at_end(), true);
			EXPECT_EQ(obj.active_range().begin(), std::data(vals) + std::size(vals));
			EXPECT_EQ(ctxt, visitation_ctxt);
			return jopp2::node_visitor_status::ready;
		}
	);

	visitor.handle_end_of_container_generic_value.expect_call_with_action(
		[visitation_ctxt, &vals, &called] (
			jopp2::container_proxy<std::vector<test_generic_value> const>& obj,
			jopp2::value_visitation_context const& ctxt
		)
		{
			EXPECT_EQ(called, true);
			EXPECT_EQ(obj.at_begin(), true);
			EXPECT_EQ(obj.at_end(), true);
			EXPECT_EQ(obj.active_range().begin(), std::data(vals) + std::size(vals));
			EXPECT_EQ(ctxt, visitation_ctxt);
			return jopp2::node_visitor_status::ready;
		}
	);
	auto const result = walker.dispatch(vals_proxy, visitation_ctxt);
	EXPECT_EQ(result, jopp2::visit_node_result::completed);
}

TESTCASE(jopp2_tree_walker_dispatch_generic_value_array_in_the_middle)
{
	// TODO: validate values
	std::vector vals{
		test_generic_value{1},
		test_generic_value{2},
		test_generic_value{3}
	};
	jopp2::container_proxy vals_proxy{std::cref(vals)};
	vals_proxy.pop_active_element();
	EXPECT_EQ(vals_proxy.active_range().size(), 2);

	test_generic_value value;
	test_node_visitor visitor{};
	jopp2::tree_walker walker{std::as_const(value), visitor};
	EXPECT_EQ(walker.current_depth(), 1);
	auto visitation_ctxt = jopp2::value_visitation_context{}.enter_next_level(42);
	EXPECT_EQ(visitation_ctxt.depth(), 1);
	EXPECT_EQ(visitation_ctxt.node_index(), 0);
	auto const result = walker.dispatch(vals_proxy, visitation_ctxt);

	EXPECT_EQ(result, jopp2::visit_node_result::node_visitor_ready);
	EXPECT_EQ(walker.current_depth(), 2);
	EXPECT_EQ(vals_proxy.active_range().size(), 1);
	EXPECT_EQ(visitation_ctxt.depth(), 1);
	EXPECT_EQ(visitation_ctxt.node_index(), 1);
}
