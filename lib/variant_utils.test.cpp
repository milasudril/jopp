//@	{"target":{"name":"./variant_utils.test"}}

#include "./variant_utils.hpp"

#include <utility>
#include <testfwk/testfwk.hpp>

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