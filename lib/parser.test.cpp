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

TESTCASE(jopp_parser_unescape_char)
{
	auto const quotation_mark = jopp::unescape(jopp::esc_chars::quotation_mark);
	EXPECT_EQ(quotation_mark, '"');

	auto const rev_sollidus = jopp::unescape(jopp::esc_chars::rev_sollidus);
	EXPECT_EQ(rev_sollidus, '\\');

	auto const linefeed = jopp::unescape(jopp::esc_chars::linefeed);
	EXPECT_EQ(linefeed, '\n');

	auto const tab = jopp::unescape(jopp::esc_chars::tab);
	EXPECT_EQ(tab, '\t');

	auto const other = jopp::unescape('0');
	EXPECT_EQ(other.has_value(), false);
}

namespace
{
#if 0
	constexpr std::string_view json_test_data{R"({
	"had": {
		"tightly": [
			"feet",
			true,
			2145719840.4312375,
			-286229488,
			true,
			true
		],
		"sound": false,
		"eaten": false,
		"pull": 1285774482.782745,
		"long": -1437168945.8634152,
		"independent": -1451031326
	},
	"fireplace": 720535269,
	"refused": "better",
	"wood": "involved",
	"without": true,
	"it": false
})"};
#else
	constexpr std::string_view json_test_data{R"({
	"empty object": { },
	"had": {
		"tightly": [
			"feet",
			true,
			2145719840.4312375,
			-286229488,
			true,
			true,
			{"foo": "bar"}
		],
		"sound": false,
		"eaten": false,
		"pull": 1285774482.782745,
		"long": -1437168945.8634152,
		"independent": -1451031326,
		"repeated end": {
			"value": 46
		}
	},
	"fireplace": 720535269,
	"refused": "better",
	"wood": "involved",
	"without": true,
	"it": false,
	"testing null": null
})"};
#endif
}

TESTCASE(jopp_parser_parse_data)
{
	jopp::parser parser{};

	jopp::value val;
	auto res = parser.parse(
		std::span{std::data(json_test_data), std::size(json_test_data)}, val);
	EXPECT_EQ(res.ec, jopp::error_code::completed);
	EXPECT_EQ(res.ptr, std::end(json_test_data));

	debug_print(val);
}