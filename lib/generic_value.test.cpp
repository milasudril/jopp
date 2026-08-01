//@	{"target":{"name": "generic_value.test"}}

#include "./generic_value.hpp"

#include <flat_map>
#include <testfwk/testfwk.hpp>
#include <format>
#include <print>

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
			{ output.get() +="    "; }
		}

		static constexpr auto internal_to_string = jopp2::overload{
			[](std::monostate) {
				return "null";
			},
			[](bool_wrapper value) {
				return value == bool_wrapper::enabled? "true" : "false";
			},
			[](std::string const& str) {
				return str;
			},
			[](double x) {
				return std::format("{}", x);
			}
		};

		template<class T>
		void print_without_tag(T const& item, size_t index, size_t parent_container_size)
		{
			do_indent();
			if(index + 1== parent_container_size) [[unlikely]]
			{
				output.get() += std::format(
					"({} of {}) {}\n", index + 1, parent_container_size, internal_to_string(item)
				);
			}
			else
			{
				output.get() += std::format(
					"({} of {}) {},\n", index + 1, parent_container_size, internal_to_string(item)
				);
			}
		}

		template<class T>
		void handle_leaf_value_array(std::vector<T> const& src, jopp2::value_visitation_context context)
		{
			handle_begin_of_array(std::type_identity<T>{}, context);
			auto const size = std::size(src);
			for(auto&& [index, item]: std::ranges::enumerate_view{src})
			{ print_without_tag(item, static_cast<size_t>(index), size); }
			handle_end_of_array(std::type_identity<T>{}, context);
		}

		void handle_leaf_value(std::string const& str, jopp2::value_visitation_context context)
		{
			do_indent();
			auto const index = context.node_index ;
			auto const parent_container_size = context.parent_container_size;
			if(index + 1 == parent_container_size) [[unlikely]]
			{
				output.get() += std::format("({} of {}) str({})\n", index + 1, parent_container_size, str);
			}
			else
			{
				output.get() += std::format("({} of {}) str({}),\n", index + 1, parent_container_size, str);
			}
		}

		void handle_leaf_value(double value, jopp2::value_visitation_context context)
		{
			do_indent();
			auto const index = context.node_index ;
			auto const parent_container_size = context.parent_container_size;
			if(index + 1 == parent_container_size) [[unlikely]]
			{
				output.get() += std::format( "({} of {}) sd{}\n", index + 1, parent_container_size, value);
			}
			else
			{
				output.get() += std::format( "({} of {}) sd{},\n", index + 1, parent_container_size, value);
			}
		}

		void handle_leaf_value(std::monostate /*unused*/, jopp2::value_visitation_context context)
		{
			do_indent();
			auto const index = context.node_index ;
			auto const parent_container_size = context.parent_container_size;
			if(index + 1 == parent_container_size) [[unlikely]]
			{
				output.get() += std::format("({} of {}) null\n", index + 1, parent_container_size);
			}
			else
			{
				output.get() += std::format("({} of {}) null,\n", index + 1, parent_container_size);
			}
		}

		void handle_leaf_value(bool_wrapper value, jopp2::value_visitation_context context)
		{
			do_indent();
			auto const index = context.node_index ;
			auto const parent_container_size = context.parent_container_size;
			if(index + 1 == parent_container_size) [[unlikely]]
			{
				output.get() += std::format(
					"({} of {}) {}\n", index + 1, parent_container_size, internal_to_string(value)
				);
			}
			else
			{
				output.get() += std::format(
					"({} of {}) {},\n", index + 1, parent_container_size, internal_to_string(value)
				);
			}
		}

		void handle_property_name(std::string const& name, jopp2::value_visitation_context /*unused*/)
		{
			do_indent();
			output.get() += std::format("{}: ", name);
			skip_indent = true;
		}

		void handle_begin_of_object(jopp2::value_visitation_context context)
		{
			do_indent();
			output.get() += std::format(
				"({} of {}) {{\n",
				context.node_index + 1,
				context.parent_container_size
			);
			++indentation;
		}

		void handle_end_of_object(jopp2::value_visitation_context context)
		{
			--indentation;
			do_indent();
			if(!context.is_last_node())
			{ output.get() += "},\n"; }
			else
			{ output.get() += "}\n"; }
		}

		template<class T>
		void handle_begin_of_array(std::type_identity<T> /*unused*/, jopp2::value_visitation_context context)
		{
			assert(context.node_index < context.parent_container_size);
			do_indent();
			output.get() += std::format(
				"({} of {}) {}[\n",
				context.node_index + 1,
				context.parent_container_size,
				map_type_name<T>::name
			);
			++indentation;
		}

		template<class T>
		void handle_end_of_array(std::type_identity<T> /*unused*/, jopp2::value_visitation_context context)
		{
			--indentation;
			do_indent();
			if(!context.is_last_node())
			{ output.get() += "],\n"; }
			else
			{ output.get() += "]\n"; }
		}

		std::reference_wrapper<std::string> output;
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
	EXPECT_EQ(result_2.key, nullptr);

	foo = type_with_pack{};
	auto const result_3 = foo.try_store_value_as(
		42,
		"The answer to the question of life the universe and everything"
	);
	EXPECT_EQ(result_3.key, nullptr);
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
	std::string output;
	visit_nodes(result, test_node_visitor{output});
	static constexpr auto expected_output = R"((1 of 1) {
    array_of_arrays_of_bools: (1 of 15) [
        (1 of 2) bool[
            (1 of 2) true,
            (2 of 2) false
        ],
        (2 of 2) bool[
            (1 of 3) false,
            (2 of 3) true,
            (3 of 3) true
        ]
    ],
    array_of_arrays_of_nulls: (2 of 15) [
        (1 of 2) null[
            (1 of 2) null,
            (2 of 2) null
        ],
        (2 of 2) null[
            (1 of 1) null
        ]
    ],
    array_of_arrays_of_numbers: (3 of 15) [
        (1 of 2) fd[
            (1 of 3) 1,
            (2 of 3) 2,
            (3 of 3) 3
        ],
        (2 of 2) fd[
            (1 of 2) 4.5,
            (2 of 2) 5.5
        ]
    ],
    array_of_arrays_of_objects: (4 of 15) [
        (1 of 2) obj[
            (1 of 2) {
                id: (1 of 2) sd1,
                status: (2 of 2) str(active)
            },
            (2 of 2) {
                id: (1 of 2) sd2,
                status: (2 of 2) str(pending)
            }
        ],
        (2 of 2) obj[
            (1 of 1) {
                id: (1 of 2) sd3,
                status: (2 of 2) str(completed)
            }
        ]
    ],
    array_of_arrays_of_strings: (5 of 15) [
        (1 of 2) str[
            (1 of 2) apple,
            (2 of 2) banana
        ],
        (2 of 2) str[
            (1 of 2) cherry,
            (2 of 2) date
        ]
    ],
    array_of_bools: (6 of 15) bool[
        (1 of 3) true,
        (2 of 3) false,
        (3 of 3) true
    ],
    array_of_heterogenous_arrays: (7 of 15) [
        (1 of 2) [
            (1 of 4) sd1,
            (2 of 4) str(two),
            (3 of 4) true,
            (4 of 4) null
        ],
        (2 of 2) [
            (1 of 3) false,
            (2 of 3) sd3.14,
            (3 of 3) {
                key: (1 of 1) str(value)
            }
        ]
    ],
    array_of_nulls: (8 of 15) null[
        (1 of 3) null,
        (2 of 3) null,
        (3 of 3) null
    ],
    array_of_objects: (9 of 15) obj[
        (1 of 2) {
            item: (1 of 2) str(A),
            value: (2 of 2) sd100
        },
        (2 of 2) {
            item: (1 of 2) str(B),
            value: (2 of 2) sd200
        }
    ],
    array_of_strings: (10 of 15) str[
        (1 of 3) test 1,
        (2 of 3) test 2,
        (3 of 3) test 3
    ],
    bool: (11 of 15) true,
    heterogenous_arrays: (12 of 15) [
        (1 of 5) str(text),
        (2 of 5) sd123,
        (3 of 5) false,
        (4 of 5) null,
        (5 of 5) {
            nested: (1 of 1) str(object)
        }
    ],
    number: (13 of 15) sd42,
    object: (14 of 15) {
        is_dummy: (1 of 2) true,
        sample_key: (2 of 2) str(sample_value)
    },
    string_value: (15 of 15) str(lorem ipsum)
}
)";

	EXPECT_EQ(output, expected_output);
}
