//@	{"target":{"name": "generic_value.test"}}

#include "./generic_value.hpp"

#include <flat_map>
#include <testfwk/testfwk.hpp>
#include <format>

namespace
{
	struct my_value_traits_with_variant
	{
		using key_type = std::string;
		using leaf_value_type = std::variant<int, double, std::string, char>;
	};

	struct my_value_traits_with_no_variant
	{
		using key_type = std::string;
		using leaf_value_type = std::string;
	};

	enum class bool_wrapper:bool{
		disabled = false,
		enabled = true
	};

	struct json_value_traits
	{
		using key_type = std::string;
		using leaf_value_type = std::variant<double, std::string, bool_wrapper, std::nullptr_t>;
	};

	struct test_node_visitor
	{
		void do_indent()
		{
			if(skip_indent)
			{
				skip_indent = false;
				return;
			}

			for(size_t k = 0; k != indentation; ++k)
			{
				fputs("    ", stdout);
			}
		}

		void handle_leaf_value(std::string const& str, jopp2::value_visitation_context context)
		{
			do_indent();
			if(!context.is_last_node())
			{ printf("(%zu of %zu) %s,\n", context.node_index + 1, context.parent_container_size, str.c_str()); }
			else
			{ printf("(%zu of %zu) %s\n", context.node_index + 1, context.parent_container_size, str.c_str()); }
		}

		void handle_leaf_value(double value, jopp2::value_visitation_context context)
		{
			do_indent();
			if(!context.is_last_node())
			{ puts(std::format("({} of {}) {},", context.node_index + 1, context.parent_container_size, value).c_str()); }
			else
			{ puts(std::format("({} of {}) {}", context.node_index + 1, context.parent_container_size, value).c_str()); }
		}

		void handle_leaf_value(std::nullptr_t, jopp2::value_visitation_context context)
		{
			do_indent();
			if(!context.is_last_node())
			{ printf("(%zu of %zu) null,\n", context.node_index + 1, context.parent_container_size); }
			else
			{ printf("(%zu of %zu) null\n", context.node_index + 1, context.parent_container_size); }
		}

		void handle_leaf_value(bool_wrapper value, jopp2::value_visitation_context context)
		{
			do_indent();
			if(!context.is_last_node())
			{ printf("(%zu of %zu) %s,\n", context.node_index + 1, context.parent_container_size, value == bool_wrapper::enabled? "true": "false"); }
			else
			{ printf("(%zu of %zu) %s\n", context.node_index + 1, context.parent_container_size, value == bool_wrapper::enabled? "true": "false"); }
		}

		void handle_property_name(std::string const& name, jopp2::value_visitation_context context)
		{
			do_indent();
			printf("%s (%zu of %zu): ", name.c_str(), context.node_index + 1, context.parent_container_size);
			skip_indent = true;
		}

		void handle_begin_of_object(jopp2::value_visitation_context context)
		{
			do_indent();
			printf("(%zu of %zu) {\n", context.node_index + 1, context.parent_container_size);
			++indentation;
		}

		void handle_end_of_object(jopp2::value_visitation_context context)
		{
			--indentation;
			do_indent();
			if(!context.is_last_node())
			{ puts("},"); }
			else
			{ puts("}"); }
		}

		template<class T>
		void handle_begin_of_array(std::type_identity<T> /*unused*/, jopp2::value_visitation_context context)
		{
			assert(context.node_index < context.parent_container_size);
			do_indent();
			printf("(%zu of %zu) [\n", context.node_index + 1, context.parent_container_size);
			++indentation;
		}

		template<class T>
		void handle_end_of_array(std::type_identity<T> /*unused*/, jopp2::value_visitation_context context)
		{
			--indentation;
			do_indent();
			if(!context.is_last_node())
			{ puts("],"); }
			else
			{ puts("]"); }
		}

		size_t indentation = 0;
		bool skip_indent = false;
	};
}

TESTCASE(jopp2_generic_value_static_properties)
{
	using type_with_variant = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_variant>;
	EXPECT_EQ(
		(std::is_same_v<
			type_with_variant::variant_type,
			std::variant<
				int,
				double,
				std::string,
				char,
				type_with_variant::object,
				std::vector<int>,
				std::vector<double>,
				std::vector<std::string>,
				std::vector<char>,
				std::vector<type_with_variant::object>,
				std::vector<type_with_variant>
			>
		>),
		true
	);
	EXPECT_EQ(std::is_constructible_v<type_with_variant>, true);
	EXPECT_EQ(std::is_copy_constructible_v<type_with_variant>, false);
	EXPECT_EQ(std::is_copy_assignable_v<type_with_variant>, false);
	EXPECT_EQ(std::is_move_constructible_v<type_with_variant>, true);
	EXPECT_EQ(std::is_move_assignable_v<type_with_variant>, true);
	EXPECT_EQ((std::is_constructible_v<type_with_variant, int>), true);
	EXPECT_EQ((std::is_constructible_v<type_with_variant, type_with_variant::variant_type>), true);

	using type_with_no_variant = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_no_variant>;
		EXPECT_EQ(
		(std::is_same_v<
			type_with_no_variant::variant_type,
			std::variant<
				std::string,
				type_with_no_variant::object,
				std::vector<std::string>,
				std::vector<type_with_no_variant::object>,
				std::vector<type_with_no_variant>
			>
		>),
		true
	);
	EXPECT_EQ(std::is_constructible_v<type_with_no_variant>, true);
	EXPECT_EQ(std::is_copy_constructible_v<type_with_no_variant>, false);
	EXPECT_EQ(std::is_copy_assignable_v<type_with_no_variant>, false);
	EXPECT_EQ(std::is_move_constructible_v<type_with_no_variant>, true);
	EXPECT_EQ(std::is_move_assignable_v<type_with_no_variant>, true);
	EXPECT_EQ((std::is_constructible_v<type_with_no_variant, int>), true);
	EXPECT_EQ((std::is_constructible_v<type_with_no_variant, type_with_variant::variant_type>), true);
}

TESTCASE(jopp2_generic_value_set_field_and_get_value)
{
	using type_with_variant = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_variant>;
	type_with_variant foo{type_with_variant::object{}};

	{
		auto& obj = foo.get<type_with_variant::object&>();
		obj.insert(std::pair{"the_key", 2345});
		for(size_t k = 0; k != 2; ++k)
		{
			auto item = std::as_const(foo).get_by_name<int const&>("the_key");
			EXPECT_EQ(item, 2345);
			item = 1;
			EXPECT_EQ(item, 1);
		}
	}

	{
		auto& obj = foo.get<type_with_variant::object&>();
		obj.insert(std::pair{"other_key", 0});
		for(size_t k = 0; k != 2; ++k)
		{
			auto item = foo.get_by_name<int>("other_key");
			EXPECT_EQ(item, 0);
			item = 1;
			EXPECT_EQ(item, 1);
		}
	}

	{
		auto& obj = foo.get<type_with_variant::object&>();
		obj.insert(std::pair{"third_key", 0});
		for(int k = 0; k != 2; ++k)
		{
			auto& item = foo.get_by_name<int&>("third_key");
			EXPECT_EQ(item, k);
			++item;
			EXPECT_EQ(item, k + 1);
		}
	}
}

TESTCASE(jopp2_generic_value_store_value_as)
{
	using type_with_variant = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_variant>;
	type_with_variant foo{type_with_variant::object{}};

	auto const result_1 = foo.store_value_as(42, "The answer to the question of life the universe and everything");
	EXPECT_EQ(result_1.first, "The answer to the question of life the universe and everything");
	EXPECT_EQ(result_1.second, 42);

	auto const result_2 = foo.try_store_value_as(
		43,
		"The answer to the question of life the universe and everything"
	);
	EXPECT_EQ(result_2.first, nullptr);

	foo = type_with_variant{};
	auto const result_3 = foo.try_store_value_as(
		42,
		"The answer to the question of life the universe and everything"
	);
	EXPECT_EQ(result_3.first, nullptr);
}

TESTCASE(jopp2_generic_value_try_store_at_end_not_a_sequence)
{
	using type_with_variant = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_variant>;
	type_with_variant foo{};
	auto const res = foo.try_store_at_end(134);
	EXPECT_EQ(res, nullptr);
}

TESTCASE(jopp2_generic_value_try_store_at_value_is_a_string)
{
	using type_with_variant = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_variant>;
	type_with_variant foo{std::string{"Hej"}};
	auto const res = foo.try_store_at_end('a');
	EXPECT_EQ(res, nullptr);
}

TESTCASE(jopp2_generic_value_try_store_at_end_sequence_empty_wrong_type)
{
	using type_with_variant = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_variant>;
	type_with_variant foo{std::vector<int>{}};
	auto const res = foo.try_store_at_end(std::string{"foobar"});
	REQUIRE_NE(res, nullptr);
	EXPECT_EQ(*res, "foobar");
	auto container = foo.get_if<std::vector<std::string>>();
	REQUIRE_NE(container, nullptr);
	EXPECT_EQ(res, &container->back());
}

TESTCASE(jopp2_generic_value_try_store_at_end_nonempty_generic_sequence)
{
	using type_with_variant = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_variant>;
	std::vector<type_with_variant> initial_value{};
	initial_value.emplace_back("foobar");
	initial_value.emplace_back(123);
	type_with_variant foo{std::move(initial_value)};

	auto const res = foo.try_store_at_end(2.5);
	REQUIRE_NE(res, nullptr);
	EXPECT_EQ(*res, 2.5);
	auto container = foo.get_if<std::vector<type_with_variant>>();
	REQUIRE_NE(container, nullptr);
	auto const stored_value_ptr = container->back().get_if<double>();
	EXPECT_EQ(res, stored_value_ptr);
}

TESTCASE(jopp2_generic_value_try_store_at_end_different_type_from_typed_sequence)
{
	using type_with_variant = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_variant>;
	type_with_variant foo{std::vector{1,2,3,4}};
	auto const res = foo.try_store_at_end(std::string{"Foobar"});
	REQUIRE_NE(res, nullptr);
	EXPECT_EQ(*res, "Foobar");
	auto container = foo.get_if<std::vector<type_with_variant>>();
	REQUIRE_NE(container, nullptr);
	auto const stored_value_ptr = container->back().get_if<std::string>();
	EXPECT_EQ(res, stored_value_ptr);
}

TESTCASE(jopp2_generic_value_try_store_at_end_of_typed_container)
{
	using type_with_variant = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_variant>;
	type_with_variant foo{std::vector{1,2,3,4}};
	auto const res = foo.try_store_at_end(5);
	REQUIRE_NE(res, nullptr);
	EXPECT_EQ(*res, 5);
	auto container = foo.get_if<std::vector<int>>();
	REQUIRE_NE(container, nullptr);
	EXPECT_EQ(res, &container->back());
}

TESTCASE(jopp2_generic_value_visit_nodes)
{
/* Test data
{
  "heterogeneous_array": [
    "string_element",
    42,
    { "key": "value" },
    [1, 2, 3],
    true,
    null
  ],
  "array_of_numbers": [
    1,
    2.5,
    -3,
    0,
    1000
  ],
  "array_of_strings": [
    "apple",
    "banana",
    "cherry",
    "date"
  ],
  "array_of_nulls": [
    null,
    null,
    null
  ],
  "array_of_booleans": [
    true,
    false,
    true,
    true
  ],
  "array_of_arrays": [
    [1, 2],
    ["a", "b"],
    [true, false]
  ],
  "array_of_objects": [
    {
      "id": 1,
      "name": "Alice"
    },
    {
      "id": 2,
      "name": "Bob"
    }
  ],
  "object_value": {
    "nested_key_1": "nested_value",
    "nested_key_2": 123
  },
  "string_value": "Hello, world!",
  "number_value": 3.14159,
  "null_value": null,
  "boolean_value": true
}
*/

	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;

	static_assert(json_value::is_leaf_value<double>);
	static_assert(!json_value::is_leaf_value<int>);

	json_value value{json_value::object{}};
	{
		auto const res = value.store_value_as(std::vector<json_value>{}, "heterogeneous_array");
		res.second.emplace_back("string_element");
		res.second.emplace_back(json_value{42.0});
		{
			res.second.emplace_back(json_value::object{});
			res.second.back().store_value_as(std::string{"value"}, "key");
		}

		{
			res.second.emplace_back(std::vector<json_value>{});
			res.second.back().try_store_at_end(1.0);
			res.second.back().try_store_at_end(2.0);
			res.second.back().try_store_at_end(3.0);
		}

		res.second.emplace_back(bool_wrapper::enabled);
		res.second.emplace_back(nullptr);
	}

	{
		auto const res = value.store_value_as(std::vector<double>{}, "array_of_numbers");
		res.second.emplace_back(1.0);
		res.second.emplace_back(2.5);
		res.second.emplace_back(-3.0);
		res.second.emplace_back(0.0);
		res.second.emplace_back(1000.0);
	}

	{
		auto const res = value.store_value_as(std::vector<std::string>{}, "array_of_strings");
		res.second.emplace_back("apple");
		res.second.emplace_back("banana");
		res.second.emplace_back("cherry");
		res.second.emplace_back("date");
	}

	{
		auto const res = value.store_value_as(std::vector<std::nullptr_t>{}, "array_of_nulls");
		res.second.emplace_back(nullptr);
		res.second.emplace_back(nullptr);
		res.second.emplace_back(nullptr);
		res.second.emplace_back(nullptr);
	}

	{
		auto const res = value.store_value_as(std::vector<bool_wrapper>{}, "array_of_booleans");
		res.second.emplace_back(bool_wrapper::enabled);
		res.second.emplace_back(bool_wrapper::disabled);
		res.second.emplace_back(bool_wrapper::enabled);
		res.second.emplace_back(bool_wrapper::enabled);
	}

	{
		auto const res = value.store_value_as(std::vector<json_value>{}, "array_of_arrays");
		{
			res.second.emplace_back(std::vector<json_value>{});
			auto& inner = res.second.back();
			inner.try_store_at_end(1.0);
			inner.try_store_at_end(2.0);
		}

		{
			res.second.emplace_back(std::vector<json_value>{});
			auto& inner = res.second.back();
			inner.try_store_at_end(std::string{"a"});
			inner.try_store_at_end(std::string{"b"});
		}

		{
			res.second.emplace_back(std::vector<json_value>{});
			auto& inner = res.second.back();
			inner.try_store_at_end(bool_wrapper::enabled);
			inner.try_store_at_end(bool_wrapper::disabled);
		}
	}

	{
		auto const res = value.store_value_as(std::vector<json_value::object>{}, "array_of_objects");
		{
			res.second.emplace_back(json_value::object{});
			auto& inner = res.second.back();
			inner.emplace("id", 1.0);
			inner.emplace("name", "Alice");
		}

		{
			res.second.emplace_back(json_value::object{});
			auto& inner = res.second.back();
			inner.emplace("id", 2.0);
			inner.emplace("name", "Bob");
		}
	}

	{
		auto const res = value.store_value_as(json_value::object{}, "object_value");
		res.second.emplace("nested_key_1", "nested_value");
		res.second.emplace("nested_key_2", 123.0);
	}

	value.store_value_as(std::string{"Hello, world!"}, "string_value");
	value.store_value_as(3.14159, "number_value");
	value.store_value_as(nullptr, "null_value");
	value.store_value_as(bool_wrapper::enabled, "boolean_value");

	using json_value_sorted = jopp2::generic_value<std::flat_map, std::vector, json_value_traits>;

	auto result = clone<json_value_sorted>(value);
	visit_nodes(result, test_node_visitor{});

//	value.visit_nodes(test_node_visitor{});
//	std::as_const(value).visit_nodes(test_node_visitor{});
}