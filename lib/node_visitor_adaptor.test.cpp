//@	{"target":{"name":"node_visitor_adaptor.test"}}

#include "./node_visitor_adaptor.hpp"
#include "lib/container_proxy.hpp"
#include <ranges>
#include <testfwk/death_test.hpp>
#include <testfwk/mock_util.hpp>

#include <testfwk/testfwk.hpp>

namespace
{
	struct test_generic_value
	{
		using value_type = std::variant<
			int,
			std::vector<test_generic_value>
		>;
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

		TestFwk::mock_entry_overload_set<
			jopp2::node_visitor_status(
				jopp2::container_proxy<std::vector<test_generic_value> const>&,
				jopp2::value_visitation_context const& ctxt
			),
			jopp2::node_visitor_status(
				jopp2::container_proxy<std::vector<std::vector<test_generic_value>> const>&,
				jopp2::value_visitation_context const& ctxt
			)
		> handle_begin_of_container;

		TestFwk::mock_entry_overload_set<
			jopp2::node_visitor_status(
				jopp2::container_proxy<std::vector<test_generic_value> const>&,
				jopp2::value_visitation_context const& ctxt
			),
			jopp2::node_visitor_status(
				jopp2::container_proxy<std::vector<std::vector<test_generic_value>> const>&,
				jopp2::value_visitation_context const& ctxt
			)
		> handle_end_of_container;
	};
}

TESTCASE(jopp2_node_visitor_adaptor_create_with_ref_to_visitor)
{
	test_node_visitor visitor{};
	auto adaptor = jopp2::make_node_visitor_adaptor<test_generic_value const&>(visitor);
	EXPECT_EQ(&adaptor.visitor(), &visitor);
	static_assert(
		std::is_same_v<
			decltype(adaptor),
			jopp2::node_visitor_adaptor<test_generic_value const&, test_node_visitor&>
		>
	);
}

TESTCASE(jopp2_node_visitor_adaptor_create_with_visitor_by_value)
{
	auto adaptor = jopp2::make_node_visitor_adaptor<test_generic_value&>(test_node_visitor{123});
	EXPECT_EQ(adaptor.visitor().test_value, 123);
	static_assert(
		std::is_same_v<
			decltype(adaptor),
			jopp2::node_visitor_adaptor<test_generic_value&, test_node_visitor>
		>
	);
}

TESTCASE(jopp2_node_visitor_adaptor_create_with_visitor_by_value_in_place)
{
	auto adaptor = jopp2::make_node_visitor_adaptor<test_generic_value&, test_node_visitor>(
		56
	);
	EXPECT_EQ(adaptor.visitor().test_value, 56);
	static_assert(
		std::is_same_v<
			decltype(adaptor),
			jopp2::node_visitor_adaptor<test_generic_value&, test_node_visitor>
		>
	);
}

TESTCASE(jopp2_node_visitor_adaptor_dispatch_leaf_value_return_ready)
{
	test_node_visitor visitor{};
	auto adaptor = jopp2::make_node_visitor_adaptor<test_generic_value const&>(visitor);
	static constexpr jopp2::value_visitation_context expected_context{
		.node_index = 1,
		.parent_container_size = 2,
		.depth = 3
	};

	visitor.handle_leaf_value.expect_call_with_action(
		[](int value, jopp2::value_visitation_context const& ctxt){
			EXPECT_EQ(value, 123);
			EXPECT_EQ(ctxt, expected_context);
			return jopp2::node_visitor_status::ready;
		}
	);
	auto const res = adaptor.dispatch<int>(123, expected_context);
	EXPECT_EQ(res, jopp2::visit_node_result::completed);
}

TESTCASE(jopp2_node_visitor_adaptor_dispatch_leaf_value_return_suspended)
{
	test_node_visitor visitor{};
	auto adaptor = jopp2::make_node_visitor_adaptor<test_generic_value const&>(visitor);
	static constexpr jopp2::value_visitation_context expected_context{
		.node_index = 1,
		.parent_container_size = 2,
		.depth = 3
	};

	visitor.handle_leaf_value.expect_call_with_action(
		[](int value, jopp2::value_visitation_context const& ctxt){
			EXPECT_EQ(value, 123);
			EXPECT_EQ(ctxt, expected_context);
			return jopp2::node_visitor_status::suspended;
		}
	);

	auto const res = adaptor.dispatch<int>(123, expected_context);
	EXPECT_EQ(res, jopp2::visit_node_result::node_visitor_suspended);
}

TESTCASE(jopp2_node_visitor_adaptor_dispatch_leaf_value_return_junk)
{
	test_node_visitor visitor{};
	auto adaptor = jopp2::make_node_visitor_adaptor<test_generic_value const&>(visitor);
	static constexpr jopp2::value_visitation_context expected_context{
		.node_index = 1,
		.parent_container_size = 2,
		.depth = 3
	};

	visitor.handle_leaf_value.expect_call_with_action(
		[](int value, jopp2::value_visitation_context const& ctxt){
			EXPECT_EQ(value, 123);
			EXPECT_EQ(ctxt, expected_context);
			return static_cast<jopp2::node_visitor_status>(34);
		},
		TestFwk::expectation_options{
			.cardinality = TestFwk::cardinality_constraint::exactly(1),
			.count_with_fd = true
		}
	);
	TestFwk::expect_death(
		[&adaptor](){
			std::ignore = adaptor.dispatch<int>(123, expected_context);
		},
		"jopp internal error: lib/./node_visitor_adaptor.hpp:194: Invalid return value from node visitor\n",
		SIGABRT
	);
}

TESTCASE(jopp2_node_visitor_adaptor_dispatch_leaf_value_array_return_ready)
{

	std::vector<int> vals{1, 2, 3};
	jopp2::container_proxy vals_proxy{std::ref(vals)};
	test_node_visitor visitor{};
	auto adaptor = jopp2::make_node_visitor_adaptor<test_generic_value&>(visitor);
	static constexpr jopp2::value_visitation_context expected_context{
		.node_index = 1,
		.parent_container_size = 2,
		.depth = 3
	};

	visitor.handle_leaf_value_array.expect_call_with_action(
		[callcount = static_cast<size_t>(0), &vals](
			jopp2::container_proxy<std::vector<int>>& obj,
			jopp2::value_visitation_context const& ctxt
		) mutable {
			EXPECT_EQ(ctxt, expected_context);
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
		auto const res = adaptor.dispatch(
			vals_proxy,
			expected_context
		);
		EXPECT_EQ(res, jopp2::visit_node_result::node_visitor_ready);
	}

	auto const res = adaptor.dispatch(vals_proxy, expected_context);
	EXPECT_EQ(res, jopp2::visit_node_result::completed);
}

TESTCASE(jopp2_node_visitor_adaptor_dispatch_leaf_value_array_return_suspended)
{
	std::vector<int> vals{1, 2, 3};
	jopp2::container_proxy vals_proxy{std::ref(vals)};
	test_node_visitor visitor{};
	auto adaptor = jopp2::make_node_visitor_adaptor<test_generic_value&>(visitor);
	static constexpr jopp2::value_visitation_context expected_context{
		.node_index = 1,
		.parent_container_size = 2,
		.depth = 3
	};

	visitor.handle_leaf_value_array.expect_call_with_action(
		[](
			jopp2::container_proxy<std::vector<int>>& ,
			jopp2::value_visitation_context const& ctxt
		) {
			EXPECT_EQ(ctxt, expected_context);
			return jopp2::node_visitor_status::suspended;
		}
	);
	auto const res = adaptor.dispatch(vals_proxy, expected_context);
	EXPECT_EQ(res, jopp2::visit_node_result::node_visitor_suspended);
}

TESTCASE(jopp2_node_visitor_adaptor_dispatch_leaf_value_array_return_junk)
{
	std::vector<int> vals{1, 2, 3};
	jopp2::container_proxy vals_proxy{std::ref(vals)};
	test_node_visitor visitor{};
	auto adaptor = jopp2::make_node_visitor_adaptor<test_generic_value&>(visitor);
	static constexpr jopp2::value_visitation_context expected_context{
		.node_index = 1,
		.parent_container_size = 2,
		.depth = 3
	};

	visitor.handle_leaf_value_array.expect_call_with_action(
		[](
			jopp2::container_proxy<std::vector<int>>& ,
			jopp2::value_visitation_context const& ctxt
		){
			EXPECT_EQ(ctxt, expected_context);
			return static_cast<jopp2::node_visitor_status>(34);
		},
		TestFwk::expectation_options{
			.cardinality = TestFwk::cardinality_constraint::exactly(1),
			.count_with_fd = true
		}
	);

	TestFwk::expect_death(
		[&adaptor, &vals_proxy](){
			std::ignore = adaptor.dispatch(
				vals_proxy,
				expected_context
			);
		},
		"jopp internal error: lib/./node_visitor_adaptor.hpp:214: Invalid return value from node visitor\n",
		SIGABRT
	);
}

namespace
{
	template<class ContainerType>
	void jopp2_node_visitor_adaptor_dispatch_array_cursor_at_begin_visitor_suspended(
		ContainerType const& vals
	)
	{
	jopp2::container_proxy vals_proxy{std::cref(vals)};
	test_node_visitor visitor{};
	auto adaptor = jopp2::make_node_visitor_adaptor<test_generic_value const&>(visitor);
	static constexpr jopp2::value_visitation_context expected_context{
		.node_index = 1,
		.parent_container_size = 2,
		.depth = 3
	};

	visitor.handle_begin_of_container.expect_call_with_action(
		[&vals] (
			jopp2::container_proxy<ContainerType const>& obj,
			jopp2::value_visitation_context const& ctxt
		)
		{
			EXPECT_EQ(obj.at_begin(), true);
			EXPECT_EQ(obj.active_range().begin(), std::data(vals));
			EXPECT_EQ(ctxt, expected_context);
			return jopp2::node_visitor_status::suspended;
		}
	);

	std::vector<decltype(adaptor)::node> nodes;
	auto const result = adaptor.dispatch(vals_proxy, expected_context, nodes);
	EXPECT_EQ(std::size(nodes), 0);
	EXPECT_EQ(result, jopp2::visit_node_result::node_visitor_suspended);
	}

	template<class ContainerType>
	void jopp2_node_visitor_adaptor_dispatch_array_cursor_at_end_visitor_suspended(
		ContainerType const& vals
	)
	{
		jopp2::container_proxy vals_proxy{std::cref(vals)};
		vals_proxy.pop_active_elements(std::size(vals));
		test_node_visitor visitor{};
		auto adaptor = jopp2::make_node_visitor_adaptor<test_generic_value const&>(visitor);
		static constexpr jopp2::value_visitation_context expected_context{
			.node_index = 1,
			.parent_container_size = 2,
			.depth = 3
		};

		visitor.handle_end_of_container.expect_call_with_action(
			[&vals] (
				jopp2::container_proxy<ContainerType const>& obj,
				jopp2::value_visitation_context const& ctxt
			)
			{
				EXPECT_EQ(obj.at_end(), true);
				EXPECT_EQ(obj.active_range().begin(), std::data(vals) + std::size(vals));
				EXPECT_EQ(ctxt, expected_context);
				return jopp2::node_visitor_status::suspended;
			}
		);

		std::vector<decltype(adaptor)::node> nodes;
		auto const result = adaptor.dispatch(vals_proxy, expected_context, nodes);
		EXPECT_EQ(std::size(nodes), 0);
		EXPECT_EQ(result, jopp2::visit_node_result::node_visitor_suspended);
	}

	template<class ContainerType>
	void jopp2_node_visitor_adaptor_dispatch_array_cursor_at_end_visitor_ready(
		ContainerType const& vals
	)
	{
		jopp2::container_proxy vals_proxy{std::cref(vals)};
		vals_proxy.pop_active_elements(std::size(vals));
		test_node_visitor visitor{};
		auto adaptor = jopp2::make_node_visitor_adaptor<test_generic_value const&>(visitor);
		static constexpr jopp2::value_visitation_context expected_context{
			.node_index = 1,
			.parent_container_size = 2,
			.depth = 3
		};

		visitor.handle_end_of_container.expect_call_with_action(
			[&vals] (
				jopp2::container_proxy<ContainerType const>& obj,
				jopp2::value_visitation_context const& ctxt
			)
			{
				EXPECT_EQ(obj.at_end(), true);
				EXPECT_EQ(obj.active_range().begin(), std::data(vals) + std::size(vals));
				EXPECT_EQ(ctxt, expected_context);
				return jopp2::node_visitor_status::ready;
			}
		);
		std::vector<decltype(adaptor)::node> nodes;
		auto const result = adaptor.dispatch(vals_proxy, expected_context, nodes);
		EXPECT_EQ(result, jopp2::visit_node_result::completed);
	}

	template<class ContainerType>
	void jopp2_node_visitor_adaptor_dispatch_array_empty_calls_begin_first()
	{
		ContainerType vals{};
		jopp2::container_proxy vals_proxy{std::cref(vals)};
		test_node_visitor visitor{};
		auto adaptor = jopp2::make_node_visitor_adaptor<test_generic_value const&>(visitor);
		static constexpr jopp2::value_visitation_context expected_context{
			.node_index = 1,
			.parent_container_size = 2,
			.depth = 3
		};

		EXPECT_EQ(vals_proxy.at_begin(), true);
		EXPECT_EQ(vals_proxy.at_end(), true);

		bool called = false;
		visitor.handle_begin_of_container.expect_call_with_action(
			[&vals, &called] (
				jopp2::container_proxy<ContainerType const>& obj,
				jopp2::value_visitation_context const& ctxt
			)
			{
				called = true;
				EXPECT_EQ(obj.at_begin(), true);
				EXPECT_EQ(obj.at_end(), true);
				EXPECT_EQ(obj.active_range().begin(), std::data(vals) + std::size(vals));
				EXPECT_EQ(ctxt, expected_context);
				return jopp2::node_visitor_status::ready;
			}
		);

		visitor.handle_end_of_container.expect_call_with_action(
			[&vals, &called] (
				jopp2::container_proxy<ContainerType const>& obj,
				jopp2::value_visitation_context const& ctxt
			)
			{
				EXPECT_EQ(called, true);
				EXPECT_EQ(obj.at_begin(), false);
				EXPECT_EQ(obj.at_end(), true);
				EXPECT_EQ(obj.active_range().begin(), std::data(vals) + std::size(vals));
				EXPECT_EQ(ctxt, expected_context);
				return jopp2::node_visitor_status::ready;
			}
		);
		std::vector<decltype(adaptor)::node> nodes;
		auto const result = adaptor.dispatch(vals_proxy, expected_context, nodes);
		EXPECT_EQ(result, jopp2::visit_node_result::completed);
	}

	template<class ContainerType, class ValCheck>
	void jopp2_node_visitor_adaptor_dispatch_array(ContainerType const& vals, ValCheck valcheck)
	{
		jopp2::container_proxy vals_proxy{std::cref(vals)};
		test_node_visitor visitor{};
		auto adaptor = jopp2::make_node_visitor_adaptor<test_generic_value const&>(visitor);
		static constexpr jopp2::value_visitation_context expected_context{
			.node_index = 1,
			.parent_container_size = 2,
			.depth = 3
		};
		std::vector<decltype(adaptor)::node> nodes;

		visitor.handle_begin_of_container.expect_call_with_action(
			[] (
				jopp2::container_proxy<ContainerType const>&,
				jopp2::value_visitation_context const& ctxt
			)
			{
				EXPECT_EQ(ctxt, expected_context);
				return jopp2::node_visitor_status::ready;
			}
		);

		for(size_t k = 0; k != std::size(vals); ++k)
		{
			auto const result = adaptor.dispatch(vals_proxy, expected_context, nodes);
			EXPECT_EQ(result, jopp2::visit_node_result::node_visitor_ready);
			REQUIRE_EQ(nodes.size(), k + 1);
			EXPECT_EQ(nodes.back().context.node_index, k);
			EXPECT_EQ(nodes.back().context.parent_container_size, 3);
			EXPECT_EQ(nodes.back().context.depth, expected_context.depth + 1);
			valcheck(nodes, k);
			EXPECT_EQ(vals_proxy.active_range().size(), (std::size(vals) - 1) - k);
		}

		visitor.handle_end_of_container.expect_call_with_action(
			[] (
				jopp2::container_proxy<ContainerType const>&,
				jopp2::value_visitation_context const& ctxt
			)
			{
				EXPECT_EQ(ctxt, expected_context);
				return jopp2::node_visitor_status::ready;
			}
		);
		{
			auto const result = adaptor.dispatch(vals_proxy, expected_context, nodes);
			EXPECT_EQ(result, jopp2::visit_node_result::completed);
			EXPECT_EQ(nodes.size(), 3);
			EXPECT_EQ(nodes.back().context.node_index, 2);
			EXPECT_EQ(nodes.back().context.parent_container_size, 3);
			EXPECT_EQ(nodes.back().context.depth, expected_context.depth + 1);
			valcheck(nodes, std::size(vals) - 1);
			EXPECT_EQ(vals_proxy.active_range().size(), 0);
		}
	}

	template<class ContainerType>
	void jopp2_node_visitor_adaptor_dispatch_array_empty_first_at_end_suspends()
	{

	ContainerType vals{};
	jopp2::container_proxy vals_proxy{std::cref(vals)};

	test_node_visitor visitor{};
	auto adaptor = jopp2::make_node_visitor_adaptor<test_generic_value const&>(visitor);
	static constexpr jopp2::value_visitation_context expected_context{
		.node_index = 1,
		.parent_container_size = 2,
		.depth = 3
	};
	std::vector<decltype(adaptor)::node> nodes;
	{
		visitor.handle_begin_of_container.expect_call_with_action(
			[] (
				jopp2::container_proxy<ContainerType const>&,
				jopp2::value_visitation_context const& ctxt
			)
			{
				EXPECT_EQ(ctxt, expected_context);
				return jopp2::node_visitor_status::ready;
			}
		);

		visitor.handle_end_of_container.expect_call_with_action(
			[] (
				jopp2::container_proxy<ContainerType const>&,
				jopp2::value_visitation_context const& ctxt
			)
			{
				EXPECT_EQ(ctxt, expected_context);
				return jopp2::node_visitor_status::suspended;
			}
		);

		auto const result = adaptor.dispatch(vals_proxy, expected_context, nodes);
		EXPECT_EQ(result, jopp2::visit_node_result::node_visitor_suspended);
	}
	EXPECT_EQ(std::size(nodes), 0);

	{
		visitor.handle_end_of_container.expect_call_with_action(
			[] (
				jopp2::container_proxy<ContainerType const>&,
				jopp2::value_visitation_context const& ctxt
			)
			{
				EXPECT_EQ(ctxt, expected_context);
				return jopp2::node_visitor_status::ready;
			}
		);
		auto const result = adaptor.dispatch(vals_proxy, expected_context, nodes);
		EXPECT_EQ(result, jopp2::visit_node_result::completed);
	}
	EXPECT_EQ(std::size(nodes), 0);
	}
}

TESTCASE(jopp2_node_visitor_adaptor_dispatch_generic_value_array_cursor_at_begin_visitor_suspended)
{
	jopp2_node_visitor_adaptor_dispatch_array_cursor_at_begin_visitor_suspended(
		std::vector{
			test_generic_value{1},
			test_generic_value{2},
			test_generic_value{3}
		}
	);
}

TESTCASE(jopp2_node_visitor_adaptor_dispatch_array_array_cursor_at_begin_visitor_suspended)
{
	jopp2_node_visitor_adaptor_dispatch_array_cursor_at_begin_visitor_suspended(
		std::vector{
			std::vector{test_generic_value{1}, test_generic_value{4}, test_generic_value{7}},
			std::vector{test_generic_value{2}, test_generic_value{5}, test_generic_value{8}},
			std::vector{test_generic_value{3}, test_generic_value{6}, test_generic_value{9}}
		}
	);
}

TESTCASE(jopp2_node_visitor_adaptor_dispatch_generic_value_array_cursor_at_end_visitor_suspended)
{
	jopp2_node_visitor_adaptor_dispatch_array_cursor_at_end_visitor_suspended(
		std::vector{
			test_generic_value{1},
			test_generic_value{2},
			test_generic_value{3}
		}
	);
}

TESTCASE(jopp2_node_visitor_adaptor_dispatch_array_array_cursor_at_end_visitor_suspended)
{
	jopp2_node_visitor_adaptor_dispatch_array_cursor_at_end_visitor_suspended(
		std::vector{
			std::vector{test_generic_value{1}, test_generic_value{4}, test_generic_value{7}},
			std::vector{test_generic_value{2}, test_generic_value{5}, test_generic_value{8}},
			std::vector{test_generic_value{3}, test_generic_value{6}, test_generic_value{9}}
		}
	);
}

TESTCASE(jopp2_node_visitor_adaptor_dispatch_generic_value_array_cursor_at_end_visitor_ready)
{
	jopp2_node_visitor_adaptor_dispatch_array_cursor_at_end_visitor_ready(
		std::vector{
			test_generic_value{1},
			test_generic_value{2},
			test_generic_value{3}
		}
	);
}

TESTCASE(jopp2_node_visitor_adaptor_dispatch_array_array_cursor_at_end_visitor_ready)
{
	jopp2_node_visitor_adaptor_dispatch_array_cursor_at_end_visitor_ready(
		std::vector{
			std::vector{test_generic_value{1}, test_generic_value{4}, test_generic_value{7}},
			std::vector{test_generic_value{2}, test_generic_value{5}, test_generic_value{8}},
			std::vector{test_generic_value{3}, test_generic_value{6}, test_generic_value{9}}
		}
	);
}

TESTCASE(jopp2_node_visitor_adaptor_dispatch_generic_value_array_empty_calls_begin_first)
{
	jopp2_node_visitor_adaptor_dispatch_array_empty_calls_begin_first<
		std::vector<test_generic_value>
	>();
}

TESTCASE(jopp2_node_visitor_adaptor_dispatch_array_array_cursor_empty_calls_begin_first)
{
	jopp2_node_visitor_adaptor_dispatch_array_empty_calls_begin_first<
		std::vector<std::vector<test_generic_value>>
	>();
}

TESTCASE(jopp2_node_visitor_adaptor_dispatch_generic_value_array)
{
	jopp2_node_visitor_adaptor_dispatch_array(
		std::vector{
			test_generic_value{1},
			test_generic_value{2},
			test_generic_value{3}
		},
		[](auto const& nodes, size_t k) {
			EXPECT_EQ(std::get<int>(nodes.back().value), static_cast<int>(k) + 1);
		}
	);
}

TESTCASE(jopp2_node_visitor_adaptor_dispatch_array_array)
{
	jopp2_node_visitor_adaptor_dispatch_array(
		std::vector{
			std::vector{test_generic_value{1}, test_generic_value{4}, test_generic_value{7}},
			std::vector{test_generic_value{2}, test_generic_value{5}, test_generic_value{8}},
			std::vector{test_generic_value{3}, test_generic_value{6}, test_generic_value{9}}
		},
		[](auto const& nodes, size_t k) {
			auto& val = std::get<
				jopp2::container_proxy<
					std::vector<test_generic_value> const
				>
			>(nodes.back().value);

			for(auto const& [index, item] : std::ranges::enumerate_view{val.active_range()})
			{
				EXPECT_EQ(std::get<int>(item.get_value()), 1 + 3*index + static_cast<int>(k));
			}
		}
	);
}

TESTCASE(jopp2_node_visitor_adaptor_dispatch_generic_value_array_empty_first_at_end_suspends)
{
	jopp2_node_visitor_adaptor_dispatch_array_empty_first_at_end_suspends<
		std::vector<test_generic_value>
	>();
}

TESTCASE(jopp2_node_visitor_adaptor_dispatch_array_array_empty_first_at_end_suspends)
{
	jopp2_node_visitor_adaptor_dispatch_array_empty_first_at_end_suspends<
		std::vector<std::vector<test_generic_value>>
	>();
}
