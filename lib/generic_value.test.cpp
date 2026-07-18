//@	{"target":{"name": "generic_value.test"}}

#include "./generic_value.hpp"

#include <flat_map>
#include <testfwk/testfwk.hpp>
#include <format>

namespace
{
	struct my_value_traits_with_pack
	{
		using key_type = std::string;
		using leaf_value_type = jopp2::template_param_pack<int, double, std::string, char>;
	};

	struct my_value_traits_with_no_pack
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
		using leaf_value_type = jopp2::template_param_pack<std::monostate, bool_wrapper, double, std::string>;
	};

	template<class T>
	struct map_type_name
	{};

	template<>
	struct map_type_name<jopp2::src_object>
	{ static constexpr const char* name = "obj"; };

	template<>
	struct map_type_name<jopp2::src_value>
	{ static constexpr const char* name = ""; };

	template<>
	struct map_type_name<std::string>
	{ static constexpr const char* name = "str"; };

	template<>
	struct map_type_name<double>
	{ static constexpr const char* name = "fd"; };

	template<>
	struct map_type_name<bool_wrapper>
	{ static constexpr const char* name = "bool"; };

	template<>
	struct map_type_name<std::monostate>
	{ static constexpr const char* name = "null"; };

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
			{ printf("(%zu of %zu) str(%s),\n", context.node_index + 1, context.parent_container_size, str.c_str()); }
			else
			{ printf("(%zu of %zu) str(%s)\n", context.node_index + 1, context.parent_container_size, str.c_str()); }
		}

		void handle_leaf_value(double value, jopp2::value_visitation_context context)
		{
			do_indent();
			if(!context.is_last_node())
			{ puts(std::format("({} of {}) fd{},", context.node_index + 1, context.parent_container_size, value).c_str()); }
			else
			{ puts(std::format("({} of {}) fd{}", context.node_index + 1, context.parent_container_size, value).c_str()); }
		}

		void handle_leaf_value(std::monostate /*unused*/, jopp2::value_visitation_context context)
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

		void handle_property_name(std::string const& name, jopp2::value_visitation_context /*unused*/)
		{
			do_indent();
			printf("%s: ", name.c_str());
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
			printf("(%zu of %zu) %s[\n", context.node_index + 1, context.parent_container_size, map_type_name<T>::name);
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
	using type_with_pack = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_pack>;
	EXPECT_EQ(
		(std::is_same_v<
			type_with_pack::value_type,
			std::variant<
				int,
				double,
				std::string,
				char,
				type_with_pack::object,
				std::vector<int>,
				std::vector<double>,
				std::vector<std::string>,
				std::vector<char>,
				std::vector<type_with_pack::object>,
				std::vector<type_with_pack>
			>
		>),
		true
	);
	EXPECT_EQ(std::is_constructible_v<type_with_pack>, true);
	EXPECT_EQ(std::is_copy_constructible_v<type_with_pack>, false);
	EXPECT_EQ(std::is_copy_assignable_v<type_with_pack>, false);
	EXPECT_EQ(std::is_move_constructible_v<type_with_pack>, true);
	EXPECT_EQ(std::is_move_assignable_v<type_with_pack>, true);
	EXPECT_EQ((std::is_constructible_v<type_with_pack, int>), true);
	EXPECT_EQ((std::is_constructible_v<type_with_pack, type_with_pack::value_type>), true);

	using type_with_no_pack = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_no_pack>;
		EXPECT_EQ(
		(std::is_same_v<
			type_with_no_pack::value_type,
			std::variant<
				std::string,
				type_with_no_pack::object,
				std::vector<std::string>,
				std::vector<type_with_no_pack::object>,
				std::vector<type_with_no_pack>
			>
		>),
		true
	);
	EXPECT_EQ(std::is_constructible_v<type_with_no_pack>, true);
	EXPECT_EQ(std::is_copy_constructible_v<type_with_no_pack>, false);
	EXPECT_EQ(std::is_copy_assignable_v<type_with_no_pack>, false);
	EXPECT_EQ(std::is_move_constructible_v<type_with_no_pack>, true);
	EXPECT_EQ(std::is_move_assignable_v<type_with_no_pack>, true);
	EXPECT_EQ((std::is_constructible_v<type_with_no_pack, std::string>), true);
	EXPECT_EQ((std::is_constructible_v<type_with_no_pack, type_with_pack::value_type>), false);
	EXPECT_EQ((std::is_constructible_v<type_with_no_pack, int>), false);
}

TESTCASE(jopp2_generic_value_set_field_and_get_value)
{
	using type_with_pack = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_pack>;
	type_with_pack foo{type_with_pack::object{}};

	{
		auto& obj = foo.get<type_with_pack::object&>();
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
		auto& obj = foo.get<type_with_pack::object&>();
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
		auto& obj = foo.get<type_with_pack::object&>();
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
	using type_with_pack = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_pack>;
	type_with_pack foo{type_with_pack::object{}};

	auto const result_1 = foo.store_value_as(42, "The answer to the question of life the universe and everything");
	EXPECT_EQ(result_1.first, "The answer to the question of life the universe and everything");
	EXPECT_EQ(result_1.second, 42);

	auto const result_2 = foo.try_store_value_as(
		43,
		"The answer to the question of life the universe and everything"
	);
	EXPECT_EQ(result_2.first, nullptr);

	foo = type_with_pack{};
	auto const result_3 = foo.try_store_value_as(
		42,
		"The answer to the question of life the universe and everything"
	);
	EXPECT_EQ(result_3.first, nullptr);
}

TESTCASE(jopp2_generic_value_try_store_at_end_not_a_sequence)
{
	using type_with_pack = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_pack>;
	type_with_pack foo{};
	auto const res = foo.try_store_at_end(134);
	EXPECT_EQ(res, nullptr);
}

TESTCASE(jopp2_generic_value_try_store_at_value_is_a_string)
{
	using type_with_pack = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_pack>;
	type_with_pack foo{std::string{"Hej"}};
	auto const res = foo.try_store_at_end('a');
	EXPECT_EQ(res, nullptr);
}

TESTCASE(jopp2_generic_value_try_store_at_end_sequence_empty_wrong_type)
{
	using type_with_pack = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_pack>;
	type_with_pack foo{std::vector<int>{}};
	auto const res = foo.try_store_at_end(std::string{"foobar"});
	REQUIRE_NE(res, nullptr);
	EXPECT_EQ(*res, "foobar");
	auto container = foo.get_if<std::vector<std::string>>();
	REQUIRE_NE(container, nullptr);
	EXPECT_EQ(res, &container->back());
}

TESTCASE(jopp2_generic_value_try_store_at_end_nonempty_generic_sequence)
{
	using type_with_pack = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_pack>;
	std::vector<type_with_pack> initial_value{};
	initial_value.emplace_back("foobar");
	initial_value.emplace_back(123);
	type_with_pack foo{std::move(initial_value)};

	auto const res = foo.try_store_at_end(2.5);
	REQUIRE_NE(res, nullptr);
	EXPECT_EQ(*res, 2.5);
	auto container = foo.get_if<std::vector<type_with_pack>>();
	REQUIRE_NE(container, nullptr);
	auto const stored_value_ptr = container->back().get_if<double>();
	EXPECT_EQ(res, stored_value_ptr);
}

TESTCASE(jopp2_generic_value_try_store_at_end_different_type_from_typed_sequence)
{
	using type_with_pack = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_pack>;
	type_with_pack foo{std::vector{1,2,3,4}};
	auto const res = foo.try_store_at_end(std::string{"Foobar"});
	REQUIRE_NE(res, nullptr);
	EXPECT_EQ(*res, "Foobar");
	auto container = foo.get_if<std::vector<type_with_pack>>();
	REQUIRE_NE(container, nullptr);
	auto const stored_value_ptr = container->back().get_if<std::string>();
	EXPECT_EQ(res, stored_value_ptr);
}

TESTCASE(jopp2_generic_value_try_store_at_end_of_typed_container)
{
	using type_with_pack = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_pack>;
	type_with_pack foo{std::vector{1,2,3,4}};
	auto const res = foo.try_store_at_end(5);
	REQUIRE_NE(res, nullptr);
	EXPECT_EQ(*res, 5);
	auto container = foo.get_if<std::vector<int>>();
	REQUIRE_NE(container, nullptr);
	EXPECT_EQ(res, &container->back());
}

TESTCASE(jopp2_generic_value_visit_nodes)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;

	static_assert(json_value::is_leaf_value<double>);
	static_assert(!json_value::is_leaf_value<int>);

	json_value value{json_value::object{}};
	{
		auto const res = value.store_value_as(std::vector<json_value>{}, "array_of_arrays_of_bools");
		res.second.emplace_back(
			json_value{std::vector{bool_wrapper::enabled, bool_wrapper::disabled}}
		);

		res.second.emplace_back(
			std::vector<bool_wrapper>{
				bool_wrapper::disabled,
				bool_wrapper::enabled,
				bool_wrapper::enabled
			}
		);
	}

	{
		auto const res = value.store_value_as(std::vector<json_value>{}, "array_of_arrays_of_nulls");
		res.second.emplace_back(
			json_value{std::vector{std::monostate{}, std::monostate{}}}
		);

		res.second.emplace_back(
			json_value{std::vector{std::monostate{}}}
		);
	}

	{
		auto const res = value.store_value_as(std::vector<json_value>{}, "array_of_arrays_of_numbers");
		res.second.emplace_back(
			json_value{std::vector{1.0, 2.0, 3.0}}
		);

		res.second.emplace_back(
			json_value{std::vector{4.5, 5.5}}
		);
	}

	{
		auto const res = value.store_value_as(
			std::vector<json_value>{}, "array_of_arrays_of_objects"
		);

		{
			std::vector<json_value::object> to_append;

			{
				json_value::object obj;
				obj.emplace("id", json_value{1.0});
				obj.emplace("status", "active");
				to_append.emplace_back(std::move(obj));
			}

			{
				json_value::object obj;
				obj.emplace("id", json_value{2.0});
				obj.emplace("status", "pending");
				to_append.emplace_back(std::move(obj));
			}

			res.second.emplace_back(std::move(to_append));
		}

		{
			std::vector<json_value::object> to_append;

			{
				json_value::object obj;
				obj.emplace("id", json_value{3.0});
				obj.emplace("status", "completed");
				to_append.emplace_back(std::move(obj));
			}

			res.second.emplace_back(std::move(to_append));
		}
	}

	{
		auto const res = value.store_value_as(std::vector<json_value>{}, "array_of_arrays_of_strings");
		res.second.emplace_back(
			json_value{std::vector<std::string>{"apple", "banana"}}
		);

		res.second.emplace_back(
			json_value{std::vector<std::string>{"cherry" ,"date"}}
		);
	}

	value.store_value_as(
		std::vector{
			bool_wrapper::enabled,
			bool_wrapper::disabled,
			bool_wrapper::enabled
		},
		"array_of_bools"
	);

	{
		auto const res = value.store_value_as(
			std::vector<json_value>{}, "array_of_heterogenous_arrays"
		);
		{
			std::vector<json_value> to_append;
			to_append.emplace_back(1.0);
			to_append.emplace_back("two");
			to_append.emplace_back(bool_wrapper::enabled);
			to_append.emplace_back(json_value{});
			res.second.emplace_back(std::move(to_append));
		}

		{
			std::vector<json_value> to_append;
			to_append.emplace_back(bool_wrapper::disabled);
			to_append.emplace_back(3.14);
			{
				json_value::object obj;
				obj.emplace("key", "value");
				to_append.emplace_back(std::move(obj));
			}

			res.second.emplace_back(std::move(to_append));
		}
	}

	value.store_value_as(
		std::vector{
			std::monostate{},
			std::monostate{},
			std::monostate{}
		},
		"array_of_nulls"
	);

	{
		auto const res = value.store_value_as(std::vector<json_value::object>{}, "array_of_objects");
		{
			json_value::object to_append;
			to_append.emplace("item", "A");
			to_append.emplace("value", 100.0);
			res.second.emplace_back(std::move(to_append));
		}

		{
			json_value::object to_append;
			to_append.emplace("item", "B");
			to_append.emplace("value", 200.0);
			res.second.emplace_back(std::move(to_append));
		}
	}

	{
		value.store_value_as(
			std::vector<std::string>{
				"test 1",
				"test 2",
				"test 3"
			},
			"array_of_strings"
		);
	}

	value.store_value_as(bool_wrapper::enabled, "bool");

	{
		auto const res = value.store_value_as(std::vector<json_value>{}, "heterogenous_arrays");
		res.second.emplace_back("text");
		res.second.emplace_back(123.0);
		res.second.emplace_back(bool_wrapper::disabled);
		res.second.emplace_back(std::monostate{});

		{
			json_value::object to_append;
			to_append.emplace("nested", "object");
			res.second.emplace_back(std::move(to_append));
		}
	}

	value.store_value_as(42.0, "number");

	{
		auto const res = value.store_value_as(json_value::object{}, "object");
		res.second.emplace("sample_key", "sample_value");
		res.second.emplace("is_dummy", bool_wrapper::enabled);
	}

	value.store_value_as(std::string{"lorem ipsum"}, "string_value");

	using json_value_sorted = jopp2::generic_value<std::flat_map, std::vector, json_value_traits>;

	auto result = clone<json_value_sorted>(value);
	visit_nodes(result, test_node_visitor{});
}