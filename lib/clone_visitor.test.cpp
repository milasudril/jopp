//@	{"target":{"name":"clone_visitor.test"}}

#include "./clone_visitor.hpp"
#include "lib/container_proxy.hpp"
#include "lib/node_visitor_adaptor.hpp"
#include "lib/template_param_pack.hpp"

#include <map>
#include <testfwk/testfwk.hpp>

namespace
{
	struct test_generic_value_in
	{
		using leaf_value_template_param_pack = jopp2::template_param_pack<
			int,
			std::string
		>;

		using key_type = std::variant<int, std::string>;

		using object = std::map<key_type, test_generic_value_in>;
		using value_type = std::variant<
			int,
			std::string,
			std::vector<int>,
			std::vector<std::string>,
			std::vector<test_generic_value_in>,
			object
		>;

		template<class T>
		using sequence_container_type = std::vector<T>;

		template<class T>
		static constexpr auto is_leaf_value = std::is_same_v<T, int> || std::is_same_v<T, std::string>;

		template<class Self>
		auto&& get_value(this Self&& self)
		{ return std::forward_like<Self>(std::forward<Self>(self).value); }

		value_type value;
	};

	struct test_generic_value_out
	{
		using object = std::map<std::variant<int, std::string>, test_generic_value_out>;
		using value_type = std::variant<
			int,
			std::string,
			std::vector<int>,
			std::vector<std::string>,
			std::vector<test_generic_value_out>,
			object
		>;

		template<class ... Args>
		requires(std::is_constructible_v<value_type, Args...>)
		explicit test_generic_value_out(Args&&... args):
			value{std::forward<Args>(args)...}
		{}

		template<class T>
		using sequence_container_type = std::vector<T>;
#if 0

		template<class T>
		static constexpr auto is_leaf_value = std::is_same_v<T, int> || std::is_same_v<T, std::string>;
#endif

		template<class Self>
		auto&& get_value(this Self&& self)
		{ return std::forward_like<Self>(std::forward<Self>(self).value); }

		value_type value;
		template<class T, class Self>
		auto get_if(this Self&& self)
		{ return std::get_if<std::remove_cvref_t<T>>(&std::forward<Self>(self).value); }

		struct emplace_ret_val
		{
			test_generic_value_out* value;
		};

		template<class Self, class T, class KeyLike>
		auto emplace(this Self& self, KeyLike&& key, T&& value)
		{
			using ret_type = emplace_ret_val;

			auto i = self.template get_if<object>();
			if(i == nullptr)
			{ return ret_type{}; }

			auto const insert_result = i->emplace(std::forward<KeyLike>(key), std::forward<T>(value));
			return ret_type{
				.value = &insert_result.first->second
			};
		}
	};
}

TESTCASE(jopp2_clone_visitor_initial_state)
{
	test_generic_value_out output{"This is a test"};
	using visitor_type = jopp2::clone_visitor_2<test_generic_value_in, test_generic_value_out>;
	visitor_type visitor{output};

	EXPECT_EQ(*output.get_if<int>(), 0);
	EXPECT_EQ(visitor.value_after_key(), nullptr);
	auto const& contexts = visitor.contexts();
	EXPECT_EQ(contexts.size(), 1);
	auto const& current_ctxt = contexts.back();
	EXPECT_EQ(current_ctxt.parent_node, false);
	EXPECT_EQ(
		current_ctxt.output_value.is_bound_to(
			output,
			std::type_identity<jopp2::generic_value_update_traits<test_generic_value_out>>{}
		),
		true
	);
}

TESTCASE(jopp2_clone_visitor_handle_leaf_value_no_current_key)
{
	test_generic_value_out output{"Hello, World"};
	using visitor_type = jopp2::clone_visitor_2<test_generic_value_in, test_generic_value_out>;
	visitor_type visitor{output};

	auto const& context_before = visitor.contexts().back();
	EXPECT_EQ(*output.get_if<int>(), 0);
	auto const res = visitor.handle_leaf_value(1234, jopp2::value_visitation_context{});
	EXPECT_EQ(res, jopp2::node_visitor_status::ready);
	EXPECT_EQ(*output.get_if<int>(), 1234);
	auto const& context_after = visitor.contexts().back();
	EXPECT_EQ(&context_before, &context_after);
	EXPECT_EQ(visitor.value_after_key(), nullptr);
}

TESTCASE(jopp2_clone_visitor_handle_simple_array_no_current_key)
{
	test_generic_value_out output;
	using visitor_type = jopp2::clone_visitor_2<test_generic_value_in, test_generic_value_out>;
	visitor_type visitor{output};

	std::vector vals{1, 2, 3};
	jopp2::container_proxy val_proxy{std::cref(vals)};
	auto const& context_before = visitor.contexts().back();
	auto const res = visitor.handle_simple_array(val_proxy, jopp2::value_visitation_context{});
	EXPECT_EQ(res, jopp2::node_visitor_status::ready);
	EXPECT_EQ(val_proxy.at_end(), true);
	auto const& saved_v = *output.get_if<std::vector<int>>();
	EXPECT_EQ(saved_v, vals);
	EXPECT_NE(std::data(saved_v), std::data(vals));
	EXPECT_EQ(visitor.contexts().size(), 1);
	auto const& context_after = visitor.contexts().back();
	EXPECT_EQ(&context_before, &context_after);
	EXPECT_EQ(visitor.value_after_key(), nullptr);
}

TESTCASE(jopp2_clone_visitor_handle_begin_of_container_sequence_no_current_key)
{
	test_generic_value_out output;
	using visitor_type = jopp2::clone_visitor_2<test_generic_value_in, test_generic_value_out>;
	visitor_type visitor{output};

	std::vector vals{
		test_generic_value_in{1},
		test_generic_value_in{2},
		test_generic_value_in{3}
	};

	jopp2::container_proxy container{std::cref(vals)};
	auto const& context_before = visitor.contexts().back();
	auto const res = visitor.handle_begin_of_container(container, jopp2::value_visitation_context{});
	EXPECT_EQ(res, jopp2::node_visitor_status::ready);
	EXPECT_EQ(container.at_begin(), true);
	auto const& saved_v = *output.get_if<std::vector<test_generic_value_out>>();
	// TODO: capacity should equal vals.capacity
	EXPECT_EQ(saved_v.empty(), true);
	EXPECT_EQ(visitor.contexts().size(), 2);
	auto const& context_after = visitor.contexts().back();
	EXPECT_NE(&context_before, &context_after);
	EXPECT_EQ(visitor.value_after_key(), nullptr);
	EXPECT_EQ(context_after.parent_node.is_bound_to(context_before.output_value), true);
	EXPECT_EQ(
		context_after.output_value.is_bound_to(
			*output.get_if<std::vector<test_generic_value_out>>(),
			std::type_identity<jopp2::container_update_traits<std::vector<test_generic_value_out>>>{}
		),
		true
	);
}

TESTCASE(jopp2_clone_visitor_handle_begin_of_container_object_no_current_key)
{
	test_generic_value_out output;
	using visitor_type = jopp2::clone_visitor_2<test_generic_value_in, test_generic_value_out>;
	visitor_type visitor{output};

	test_generic_value_in::object obj{
		{"first",test_generic_value_in{1}},
		{"second",test_generic_value_in{2}},
		{"third",test_generic_value_in{3}}
	};

	jopp2::container_proxy container{std::cref(obj)};
	auto const& context_before = visitor.contexts().back();
	auto const res = visitor.handle_begin_of_container(container, jopp2::value_visitation_context{});
	EXPECT_EQ(res, jopp2::node_visitor_status::ready);
	EXPECT_EQ(container.at_begin(), true);
	auto const& saved_v = *output.get_if<test_generic_value_out::object>();
	EXPECT_EQ(saved_v.empty(), true);
	EXPECT_EQ(visitor.contexts().size(), 2);
	auto const& context_after = visitor.contexts().back();
	EXPECT_NE(&context_before, &context_after);
	EXPECT_EQ(visitor.value_after_key(), nullptr);
	EXPECT_EQ(context_after.parent_node.is_bound_to(context_before.output_value), true);
	EXPECT_EQ(
		context_after.output_value.is_bound_to(
			*output.get_if<test_generic_value_out::object>(),
			std::type_identity<jopp2::container_update_traits<test_generic_value_out::object>>{}
		),
		true
	);
}

namespace
{
	template<class Visitor, class Key>
	void set_current_key(Visitor& visitor, Key&& key)
	{
		test_generic_value_in::object obj;
		jopp2::container_proxy container{std::cref(obj)};
		visitor.handle_begin_of_container(container, jopp2::value_visitation_context{});
		visitor.handle_key(
			jopp2::key_to_clone{std::forward<Key>(key)},
			jopp2::value_visitation_context{}
		);
	}
}

TESTCASE(jopp2_clone_visitor_handle_leaf_value_with_current_key)
{
	test_generic_value_out value_out;
	using visitor_type = jopp2::clone_visitor_2<test_generic_value_in, test_generic_value_out>;
	visitor_type visitor{value_out};
	set_current_key(visitor, std::string{"Hello, world"});

	auto const res = visitor.handle_leaf_value(132, jopp2::value_visitation_context{});
	EXPECT_EQ(res, jopp2::node_visitor_status::ready);

	auto const& context_before = visitor.contexts().back();
	EXPECT_EQ(visitor.contexts().size(), 2);
	auto const object = value_out.get_if<test_generic_value_out::object>();
	REQUIRE_NE(object, nullptr);
	EXPECT_EQ(*object->at("Hello, world").get_if<int>(), 132);
	EXPECT_EQ(visitor.contexts().size(), 2);
	auto const& context_after = visitor.contexts().back();
	EXPECT_EQ(&context_before, &context_after);
	EXPECT_EQ(visitor.value_after_key(), nullptr);
}


TESTCASE(jopp2_clone_visitor_handle_simple_array_withcurrent_key)
{
	test_generic_value_out output;
	using visitor_type = jopp2::clone_visitor_2<test_generic_value_in, test_generic_value_out>;
	visitor_type visitor{output};
	set_current_key(visitor, std::string{"Hello, world"});

	std::vector vals{1, 2, 3};
	jopp2::container_proxy val_proxy{std::cref(vals)};
	auto const& context_before = visitor.contexts().back();
	auto const res = visitor.handle_simple_array(val_proxy, jopp2::value_visitation_context{});
	EXPECT_EQ(res, jopp2::node_visitor_status::ready);
	EXPECT_EQ(val_proxy.at_end(), true);

	auto const object = output.get_if<test_generic_value_out::object>();
	REQUIRE_NE(object, nullptr);
	auto const& saved_v =*object->at("Hello, world").get_if<std::vector<int>>();
	EXPECT_EQ(saved_v, vals);
	EXPECT_NE(std::data(saved_v), std::data(vals));
	EXPECT_EQ(visitor.contexts().size(), 2);
	auto const& context_after = visitor.contexts().back();
	EXPECT_EQ(&context_before, &context_after);
	EXPECT_EQ(visitor.value_after_key(), nullptr);
}

TESTCASE(jopp2_clone_visitor_handle_begin_of_container_sequence_with_current_key)
{
	test_generic_value_out output;
	using visitor_type = jopp2::clone_visitor_2<test_generic_value_in, test_generic_value_out>;
	visitor_type visitor{output};
	set_current_key(visitor, std::string{"Hello, world"});

	std::vector vals{
		test_generic_value_in{1},
		test_generic_value_in{2},
		test_generic_value_in{3}
	};

	jopp2::container_proxy container{std::cref(vals)};
	auto const& context_before = visitor.contexts().back();
	auto const res = visitor.handle_begin_of_container(container, jopp2::value_visitation_context{});
	EXPECT_EQ(res, jopp2::node_visitor_status::ready);
	EXPECT_EQ(container.at_begin(), true);

	auto const object = output.get_if<test_generic_value_out::object>();
	REQUIRE_NE(object, nullptr);
	auto const& saved_v = *object->at("Hello, world").get_if<std::vector<test_generic_value_out>>();
	// TODO: capacity should equal vals.capacity
	EXPECT_EQ(saved_v.empty(), true);
	EXPECT_EQ(visitor.contexts().size(), 3);
	auto const& context_after = visitor.contexts().back();
	EXPECT_NE(&context_before, &context_after);
	EXPECT_EQ(visitor.value_after_key(), nullptr);
	EXPECT_EQ(context_after.parent_node.is_bound_to(context_before.output_value), true);
	EXPECT_EQ(
		context_after.output_value.is_bound_to(
			saved_v,
			std::type_identity<jopp2::container_update_traits<std::vector<test_generic_value_out>>>{}
		),
		true
	);
}

TESTCASE(jopp2_clone_visitor_handle_begin_of_container_object_with_current_key)
{
	test_generic_value_out output;
	using visitor_type = jopp2::clone_visitor_2<test_generic_value_in, test_generic_value_out>;
	visitor_type visitor{output};
	set_current_key(visitor, std::string{"Hello, world"});

	test_generic_value_in::object obj{
		{"first",test_generic_value_in{1}},
		{"second",test_generic_value_in{2}},
		{"third",test_generic_value_in{3}}
	};

	jopp2::container_proxy container{std::cref(obj)};
	auto const& context_before = visitor.contexts().back();
	auto const res = visitor.handle_begin_of_container(container, jopp2::value_visitation_context{});
	EXPECT_EQ(res, jopp2::node_visitor_status::ready);
	EXPECT_EQ(container.at_begin(), true);

	auto const object = output.get_if<test_generic_value_out::object>();
	REQUIRE_NE(object, nullptr);
	auto const& saved_v = *object->at("Hello, world").get_if<test_generic_value_out::object>();
	EXPECT_EQ(saved_v.empty(), true);
	EXPECT_EQ(visitor.contexts().size(), 3);
	auto const& context_after = visitor.contexts().back();
	EXPECT_NE(&context_before, &context_after);
	EXPECT_EQ(visitor.value_after_key(), nullptr);
	EXPECT_EQ(context_after.parent_node.is_bound_to(context_before.output_value), true);
	EXPECT_EQ(
		context_after.output_value.is_bound_to(
			saved_v,
			std::type_identity<jopp2::container_update_traits<test_generic_value_out::object>>{}
		),
		true
	);
}
