//@	{"target":{"name":"tree_walker.test"}}

#include "./tree_walker.hpp"
#include "testfwk/death_test.hpp"

#include <functional>
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
		test_node_visitor(test_node_visitor&&) = default;
		test_node_visitor(test_node_visitor const&) = default;
		test_node_visitor& operator=(test_node_visitor&&) = default;
		test_node_visitor& operator=(test_node_visitor const&) = default;

		explicit test_node_visitor(int val):
			test_value{val}
		{}

		struct handle_leaf_value_int_expectation
		{
			int expected_value{};
			jopp2::value_visitation_context ctxt;
			jopp2::node_visitor_status retval;
		};

		std::optional<handle_leaf_value_int_expectation> handle_leaf_value_int_call_expected;
		auto handle_leaf_value(int recv_value, jopp2::value_visitation_context const& ctxt)
		{
			REQUIRE_EQ(handle_leaf_value_int_call_expected.has_value(), true);
			EXPECT_EQ(recv_value, handle_leaf_value_int_call_expected->expected_value);
			EXPECT_EQ(ctxt, handle_leaf_value_int_call_expected->ctxt);

			auto retval = handle_leaf_value_int_call_expected->retval;
			handle_leaf_value_int_call_expected.reset();
			return retval;
		}

		std::move_only_function<
			jopp2::node_visitor_status(
				jopp2::container_proxy<std::vector<int>>&,
				jopp2::value_visitation_context const&
			)
		> handle_leaf_value_action;

		auto handle_leaf_value_array(
			jopp2::container_proxy<std::vector<int>>& container,
			jopp2::value_visitation_context const& ctxt
		)
		{
			REQUIRE_EQ(static_cast<bool>(handle_leaf_value_action), true);
			return handle_leaf_value_action(container, ctxt);
		}

		~test_node_visitor()
		{
			EXPECT_EQ(static_cast<bool>(handle_leaf_value_action), false);
			EXPECT_EQ(handle_leaf_value_int_call_expected.has_value(), false);
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
	visitor.handle_leaf_value_int_call_expected =
		test_node_visitor::handle_leaf_value_int_expectation{
			.expected_value = 123,
			.ctxt = jopp2::value_visitation_context{}.enter_next_level(42),
			.retval = jopp2::node_visitor_status::ready
		};;
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
	visitor.handle_leaf_value_int_call_expected =
		test_node_visitor::handle_leaf_value_int_expectation{
			.expected_value = 123,
			.ctxt = jopp2::value_visitation_context{}.enter_next_level(42),
			.retval = jopp2::node_visitor_status::suspended
		};;
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
	visitor.handle_leaf_value_int_call_expected =
		test_node_visitor::handle_leaf_value_int_expectation{
			.expected_value = 123,
			.ctxt = jopp2::value_visitation_context{}.enter_next_level(42),
			.retval = static_cast<jopp2::node_visitor_status>(23),
		};
	TestFwk::expect_death(
		[&walker](){
			std::ignore = walker.dispatch<int>(123,jopp2::value_visitation_context{}.enter_next_level(42));
		},
		"jopp internal error: lib/./tree_walker.hpp:244: Invalid return value from node visitor\n",
		SIGABRT
	);
	visitor.handle_leaf_value_int_call_expected.reset();
}

TESTCASE(jopp2_tree_walker_dispatch_leaf_value_array_return_ready)
{
	std::vector<int> vals{1, 2, 3};
	jopp2::container_proxy vals_proxy{std::ref(vals)};
	test_generic_value value;
	test_node_visitor visitor{};
	jopp2::tree_walker walker{std::as_const(value), visitor};

	visitor.handle_leaf_value_action = [callcount = 0](auto& vals, auto const& ctxt) mutable {
		REQUIRE_LT(callcount, 3);
		EXPECT_EQ(ctxt, jopp2::value_visitation_context{}.enter_next_level(42));
		EXPECT_EQ(*vals.active_range().begin(), callcount + 1);
		vals.pop_active_element();
		++callcount;
		return jopp2::node_visitor_status::ready;
	};

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
	visitor.handle_leaf_value_action = {};
}

TESTCASE(jopp2_tree_walker_dispatch_leaf_value_array_return_suspended)
{
	std::vector<int> vals{1, 2, 3};
	jopp2::container_proxy vals_proxy{std::ref(vals)};
	test_generic_value value;
	test_node_visitor visitor{};
	jopp2::tree_walker walker{std::as_const(value), visitor};

	visitor.handle_leaf_value_action = [callcount = 0](auto& vals, auto const& ctxt) mutable {
		REQUIRE_EQ(callcount, 0);
		EXPECT_EQ(ctxt, jopp2::value_visitation_context{}.enter_next_level(42));
		EXPECT_EQ(*vals.active_range().begin(), 1);
		++callcount;
		return jopp2::node_visitor_status::suspended;
	};

	{
		auto const res = walker.dispatch(
			vals_proxy,
			jopp2::value_visitation_context{}.enter_next_level(42)
		);
		EXPECT_EQ(res, jopp2::visit_node_result::node_visitor_suspended);
	}
	visitor.handle_leaf_value_action = {};
}

TESTCASE(jopp2_tree_walker_dispatch_leaf_value_array_return_junk)
{
	std::vector<int> vals{1, 2, 3};
	jopp2::container_proxy vals_proxy{std::ref(vals)};
	test_generic_value value;
	test_node_visitor visitor{};
	jopp2::tree_walker walker{std::as_const(value), visitor};

	visitor.handle_leaf_value_action = [callcount = 0](auto& vals, auto const& ctxt) mutable {
		REQUIRE_EQ(callcount, 0);
		EXPECT_EQ(ctxt, jopp2::value_visitation_context{}.enter_next_level(42));
		EXPECT_EQ(*vals.active_range().begin(), 1);
		++callcount;
		return static_cast<jopp2::node_visitor_status>(23);
	};

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
	visitor.handle_leaf_value_action = {};
}
