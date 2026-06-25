//@	{"target":{"name": "generic_value.test"}}

#include "./generic_value.hpp"

#include <flat_map>
#include <testfwk/testfwk.hpp>

namespace
{
	struct my_value_traits_with_variant
	{
		using key_type = std::string;
		using leaf_value_type = std::variant<int, double>;
	};

	struct my_value_traits_with_no_variant
	{
		using key_type = std::string;
		using leaf_value_type = std::string;
	};
}

TESTCASE(jopp2_create_empty_value)
{
	using type_with_variant = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_variant>;
	EXPECT_EQ(
		(std::is_same_v<
			type_with_variant::variant_type,
			std::variant<
				int,
				double,
				type_with_variant::object,
				std::vector<int>,
				std::vector<double>,
				std::vector<type_with_variant::object>,
				std::vector<type_with_variant>
			>
		>),
		true
	);
	EXPECT_EQ(std::is_constructible_v<type_with_variant>, true);
	EXPECT_EQ(type_with_variant::first_sequence_type_index(), 3);

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

}