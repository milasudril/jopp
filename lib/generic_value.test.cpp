//@	{"target":{"name": "generic_value.test"}}

#include "./generic_value.hpp"

#include <flat_map>
#include <testfwk/testfwk.hpp>

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
	EXPECT_EQ(type_with_variant::first_sequence_type_index(), 5);
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
	EXPECT_EQ(type_with_no_variant::first_sequence_type_index(), 2);
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
	EXPECT_EQ(result_1.second.get<int>(), 42);

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
	using type_with_variant = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_variant>;

	type_with_variant foo{type_with_variant::object{}};

	struct visitor
	{

	};

	std::as_const(foo).visit_nodes(visitor{});
}