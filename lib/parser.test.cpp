//@	{"target":{"name":"parser.test"}}

#include "./parser.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(jopp_is_whitespace)
{
	for(int k = 0; k != 255; ++k)
	{
		if(k == ' ' || k == '\t' || k == '\n' || k == '\r')
		{ EXPECT_EQ(jopp::is_whitespace(static_cast<char>(k)), true); }
		else
		{ EXPECT_EQ(jopp::is_whitespace(static_cast<char>(k)), false); }
	}
}

TESTCASE(jopp_char_should_be_escaped)
{
	for(int k = 0; k != 255; ++k)
	{
		if((k >= 0 && k <= 31) || k == '"')
		{ EXPECT_EQ(jopp::char_should_be_escaped(static_cast<char>(k)), true); }
		else
		{ EXPECT_EQ(jopp::char_should_be_escaped(static_cast<char>(k)), false); }
	}
}

TESTCASE(jopp_unescape)
{
	for(int k = 0; k != 255; ++k)
	{
		switch(k)
		{
			case '"':
				EXPECT_EQ(*jopp::unescape(static_cast<char>(k)), '"');
				break;

			case '\\':
				EXPECT_EQ(*jopp::unescape(static_cast<char>(k)), '\\');
				break;

			case 'n':
				EXPECT_EQ(*jopp::unescape(static_cast<char>(k)), '\n');
				break;

			case 't':
				EXPECT_EQ(*jopp::unescape(static_cast<char>(k)), '\t');
				break;

			default:
				EXPECT_EQ(jopp::unescape(static_cast<char>(k)).has_value(), false);
		}
	}
}

TESTCASE(jopp_error_code_to_string)
{
	EXPECT_EQ(to_string(jopp::error_code::completed), std::string_view{"Completed"});
	EXPECT_EQ(to_string(jopp::error_code::more_data_needed), std::string_view{"More data needed"});
	EXPECT_EQ(to_string(jopp::error_code::key_already_exists), std::string_view{"Key already exists"});
	EXPECT_EQ(to_string(jopp::error_code::character_must_be_escaped),
				 std::string_view{"Character must be escaped"});
	EXPECT_EQ(to_string(jopp::error_code::unsupported_escape_sequence),
				 std::string_view{"Unsupported escape sequence"});
	EXPECT_EQ(to_string(jopp::error_code::illegal_delimiter), std::string_view{"Illegal delimiter"});
	EXPECT_EQ(to_string(jopp::error_code::invalid_value), std::string_view{"Invalid value"});
	EXPECT_EQ(to_string(jopp::error_code::no_top_level_node), std::string_view{"No top level node"});

}

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

TESTCASE(jopp_parser_store_value_in_object)
{
	jopp::value val{jopp::object{}};

	{
		auto const res = store_value(val, "key", jopp::value{2.5});
		EXPECT_EQ(res.next_state, jopp::parser_state::after_value_object);
		EXPECT_EQ(res.err, jopp::error_code::more_data_needed);
		auto& obj = *val.get_if<jopp::object>();
		EXPECT_EQ(obj.find("key")->second, jopp::value{2.5});
	}

	{
		auto const res = store_value(val, "key", jopp::value{2.0});
		EXPECT_EQ(res.err, jopp::error_code::key_already_exists);
		auto& obj = *val.get_if<jopp::object>();
		EXPECT_EQ(obj.find("key")->second, jopp::value{2.5});
	}
}

TESTCASE(jopp_parser_store_value_in_array)
{
	jopp::value val{jopp::array{}};

	{
		auto const res = store_value(val, "key", jopp::value{2.5});
		EXPECT_EQ(res.next_state, jopp::parser_state::after_value_array);
		EXPECT_EQ(res.err, jopp::error_code::more_data_needed);
	}

	{
		auto const res = store_value(val, "key", jopp::value{1.0});
		EXPECT_EQ(res.next_state, jopp::parser_state::after_value_array);
		EXPECT_EQ(res.err, jopp::error_code::more_data_needed);
	}

	{
		auto& array = *val.get_if<jopp::array>();
		EXPECT_EQ(std::size(array), 2);
		EXPECT_EQ(array[0], jopp::value{2.5});
		EXPECT_EQ(array[1], jopp::value{1.0});
	}
}

TESTCASE(jopp_parser_store_value_litral_in_array)
{
	jopp::value val{jopp::array{}};

	{
		auto const res = store_value(val, "key", jopp::literal_view{"null"});
		EXPECT_EQ(res.next_state, jopp::parser_state::after_value_array);
		EXPECT_EQ(res.err, jopp::error_code::more_data_needed);
	}

	{
		auto& array = *val.get_if<jopp::array>();
		EXPECT_EQ(std::size(array), 1);
		EXPECT_EQ(array[0], jopp::value{jopp::null{}});
	}
}

TESTCASE(jopp_parser_store_value_unknown_litral_in_array)
{
	jopp::value val{jopp::array{}};

	{
		auto const res = store_value(val, "key", jopp::literal_view{"foobar"});
		EXPECT_EQ(res.err, jopp::error_code::invalid_value);
	}

	{
		auto& array = *val.get_if<jopp::array>();
		EXPECT_EQ(std::size(array), 0);
	}
}

namespace
{
	constexpr std::string_view json_test_data{R"({
	"empty object": { },
	"empty array": [ ],
	"had": {
		"tightly": [
			[1, 2, 3, 4],
			"feet",
			true,
			2145719840.4312375,
			-286229488,
			true,
			true,
			{"object in array": "bar"},
			{"object with literal last" : null}
		],
		"sound": false,
		"eaten": false,
		"pull" : 1285774482.782745,
		"long" : -1437168945.8634152,
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
	"testing null": null,
	"a key with esc seq\n\t\\foo\"": "A value with esc seq\n\t\\foo\""
}Some data after blob)"};
}

TESTCASE(jopp_parser_parse_data_one_block)
{
	jopp::parser parser{};

	jopp::value val;
	auto res = parser.parse(
		std::span{std::data(json_test_data), std::size(json_test_data)}, val);
	EXPECT_EQ(res.ec, jopp::error_code::completed);
	EXPECT_EQ(*res.ptr, 'S');
	EXPECT_EQ(res.line, 32);
	EXPECT_EQ(res.col, 1);

	debug_print(val);
}

TESTCASE(jopp_parser_parse_data_multiple_blocks)
{
	jopp::parser parser{};

	auto ptr = std::data(json_test_data);
	auto const n = std::size(json_test_data);
	auto bytes_left = n;
	jopp::value val;

	while(bytes_left != 0)
	{
		auto const bytes_to_process = std::min(bytes_left, static_cast<size_t>(13));
		auto res = parser.parse(std::span{ptr, bytes_to_process}, val);
		bytes_left -= bytes_to_process;
		ptr += bytes_to_process;
		if(res.ec == jopp::error_code::completed)
		{
			EXPECT_EQ(*res.ptr, 'S');
			break;
		}
		else
		{
			EXPECT_EQ(ptr, res.ptr);
			EXPECT_EQ(res.ec, jopp::error_code::more_data_needed);
		}
	}

	debug_print(val);
}

TESTCASE(jopp_parser_leaf_at_top_level)
{
	{
		jopp::parser parser{};
		std::string_view data{"\"lorem ipsum\""};
		jopp::value val;
		auto const res = parser.parse(data, val);
		EXPECT_EQ(res.line, 1);
		EXPECT_EQ(res.col, 1);
		EXPECT_EQ(res.ec, jopp::error_code::no_top_level_node);
		EXPECT_EQ(res.ptr, std::data(data) + 1);
	}

	{
		jopp::parser parser{};
		std::string_view data{"false "};
		jopp::value val;
		auto const res = parser.parse(data, val);
		EXPECT_EQ(res.line, 1);
		EXPECT_EQ(res.col, 1);
		EXPECT_EQ(res.ec, jopp::error_code::no_top_level_node);
		EXPECT_EQ(res.ptr, std::data(data) + 1);
	}
}

TESTCASE(jopp_parser_invalid_literal)
{
	jopp::parser parser;
	std::string_view data{"[foobar]"};

	jopp::value val;
	auto const res = parser.parse(data, val);
	EXPECT_EQ(res.line, 1);
	EXPECT_EQ(res.col, 8);
	EXPECT_EQ(res.ptr, std::data(data) + 8);
}