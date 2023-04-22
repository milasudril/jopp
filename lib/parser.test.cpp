//@	{"target":{"name":"parser.test"}}

#include "./parser.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(jopp_parser_to_number)
{
	auto const too_big = jopp::to_number("1e400");
	EXPECT_EQ(too_big.has_value(), false);

	auto const inf = jopp::to_number("inf");
	EXPECT_EQ(inf, std::numeric_limits<jopp::number>::infinity());

	auto const negative_inf = jopp::to_number("-inf");
	EXPECT_EQ(negative_inf, -std::numeric_limits<jopp::number>::infinity());

	auto const nan = jopp::to_number("nan");
	REQUIRE_EQ(nan.has_value(), true);
	EXPECT_EQ(std::isnan(*nan), true);

	auto const finite_number = jopp::to_number("4.0");
	EXPECT_EQ(finite_number, 4.0);

	auto const junk_after_number = jopp::to_number("4.0eiruseo");
	EXPECT_EQ(junk_after_number.has_value(), false);

	auto const junk_before_number = jopp::to_number("aseiorui4.0");
	EXPECT_EQ(junk_before_number.has_value(), false);

	auto const empty_string = jopp::to_number("");
	EXPECT_EQ(empty_string.has_value(), false);
}

TESTCASE(jopp_parser_make_value)
{
	auto const null = jopp::make_value("null");
	REQUIRE_EQ(null.has_value(), true);
	EXPECT_EQ(is_null(*null), true);

	auto const val_false = jopp::make_value("false");
	REQUIRE_EQ(val_false.has_value(), true);
	REQUIRE_NE(val_false->get_if<jopp::boolean>(), nullptr);
	EXPECT_EQ(*val_false->get_if<jopp::boolean>(), false);

	auto const val_true = jopp::make_value("true");
	REQUIRE_EQ(val_true.has_value(), true);
	REQUIRE_NE(val_true->get_if<jopp::boolean>(), nullptr);
	EXPECT_EQ(*val_true->get_if<jopp::boolean>(), true);

	auto const number = jopp::make_value("2.5e-1");
	REQUIRE_EQ(number.has_value(), true);
	REQUIRE_NE(number->get_if<jopp::number>(), nullptr);
	EXPECT_EQ(*number->get_if<jopp::number>(), 0.25);

	auto const empty = jopp::make_value("");
	EXPECT_EQ(empty.has_value(), false);
}