//@	{"target":{"name":"./variant_utils.test"}}

#include "./variant_utils.hpp"

#include <utility>
#include <testfwk/testfwk.hpp>
#include <testfwk/mock_util.hpp>

TESTCASE(jopp2_variant_utils_make_variant_of_pointers)
{
	{
		std::variant<int, double> input{243};
		auto output = jopp2::make_variant_of_pointers(input);
		static_assert(std::is_same_v<decltype(output), std::variant<int*, double*>>);
		EXPECT_EQ(*std::get<int*>(output), 243);
	}

	{
		std::variant<int, double> input{243};
		auto output = jopp2::make_variant_of_pointers(std::as_const(input));
		static_assert(
			std::is_same_v<
				decltype(output),
				std::variant<int const*, double const*>
			>
		);
		EXPECT_EQ(*std::get<int const*>(output), 243);
	}
}

TESTCASE(jopp2_variant_utils_visit_variant_element)
{
	using variant_type = std::variant<int, std::string>;
	enum class which{int_visited, string_visisted};
	struct arg_type{
		int value;
	};
	TestFwk::mock_entry_overload_set<
		which(jopp2::variant_element_tag<int>, arg_type),
		which(jopp2::variant_element_tag<std::string>, arg_type)
	> visitor;

	visitor.expect_call_with_action(
		[](jopp2::variant_element_tag<int>, arg_type arg) {
			EXPECT_EQ(arg.value, 2);
			return which::int_visited;
		}
	);
	auto const result1 = jopp2::visit_variant_element<variant_type>(
		0,
		visitor,
		arg_type{
			.value = 2
		}
	);
	EXPECT_EQ(result1, which::int_visited);

	visitor.expect_call_with_action(
		[](jopp2::variant_element_tag<std::string>, arg_type arg) {
			EXPECT_EQ(arg.value, 1);
			return which::string_visisted;
		}
	);
	auto const result2 = jopp2::visit_variant_element<variant_type>(
		1,
		visitor,
		arg_type{
			.value = 1
		}
	);
	EXPECT_EQ(result2, which::string_visisted);
}
