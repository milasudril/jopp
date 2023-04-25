//@	{"target":{"name":"parser.test"}}

#include "./parser.hpp"

#include <testfwk/testfwk.hpp>

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
			[4, 2, 3, 1],
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

	auto const& root = *val.get_if<jopp::object>();
	EXPECT_EQ(std::size(root), 10);
	{
		auto const& obj = *root.find("empty object")->second.get_if<jopp::object>();
		EXPECT_EQ(std::size(obj), 0);
	}
	{
		auto const& array = *root.find("empty array")->second.get_if<jopp::array>();
		EXPECT_EQ(std::size(array), 0);
	}
	{
		auto const& obj = *root.find("had")->second.get_if<jopp::object>();
		EXPECT_EQ(std::size(obj), 7);
		{
			auto const& array = *obj.find("tightly")->second.get_if<jopp::array>();
			EXPECT_EQ(std::size(array), 9);
			{
				auto const& inner_array = *array[0].get_if<jopp::array>();
				EXPECT_EQ(std::size(inner_array), 4);
				EXPECT_EQ(inner_array[0], jopp::value{4.0});
				EXPECT_EQ(inner_array[1], jopp::value{2.0});
				EXPECT_EQ(inner_array[2], jopp::value{3.0});
				EXPECT_EQ(inner_array[3], jopp::value{1.0});
			}
			{
				EXPECT_EQ(array[1], jopp::value{"feet"});
				EXPECT_EQ(array[2], jopp::value{true});
				EXPECT_EQ(array[3], jopp::value{2145719840.4312375});
				EXPECT_EQ(array[4], jopp::value{-286229488.0});
				EXPECT_EQ(array[5], jopp::value{true});
				EXPECT_EQ(array[6], jopp::value{true});
			}
			{
				auto const& obj = *array[7].get_if<jopp::object>();
				EXPECT_EQ(std::size(obj), 1);
				EXPECT_EQ(obj.find("object in array")->second, jopp::value{jopp::string{"bar"}});
			}
			{
				auto const& obj = *array[8].get_if<jopp::object>();
				EXPECT_EQ(std::size(obj), 1);
				EXPECT_EQ(obj.find("object with literal last")->second, jopp::value{jopp::null{}});
			}
		}
		{
			EXPECT_EQ(obj.find("sound")->second, jopp::value{false});
			EXPECT_EQ(obj.find("eaten")->second, jopp::value{false});
			EXPECT_EQ(obj.find("pull")->second, jopp::value{1285774482.782745});
			EXPECT_EQ(obj.find("long")->second, jopp::value{-1437168945.8634152});
			EXPECT_EQ(obj.find("independent")->second, jopp::value{-1451031326.0});
		}
		{
			auto const& inner_obj = *obj.find("repeated end")->second.get_if<jopp::object>();
			EXPECT_EQ(std::size(inner_obj), 1);
			EXPECT_EQ(inner_obj.find("value")->second, jopp::value{46.0});
		}
	}
	EXPECT_EQ(root.find("fireplace")->second, jopp::value{720535269.0});
	EXPECT_EQ(root.find("refused")->second, jopp::value{jopp::string{"better"}});
	EXPECT_EQ(root.find("wood")->second, jopp::value{jopp::string{"involved"}});
	EXPECT_EQ(root.find("without")->second, jopp::value{true});
	EXPECT_EQ(root.find("it")->second, jopp::value{false});
	EXPECT_EQ(root.find("testing null")->second, jopp::value{jopp::null{}});
	EXPECT_EQ(root.find("a key with esc seq\n\t\\foo\"")->second,
		jopp::value{jopp::string{"A value with esc seq\n\t\\foo\""}});
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

	auto const& root = *val.get_if<jopp::object>();
	EXPECT_EQ(std::size(root), 10);
	{
		auto const& obj = *root.find("empty object")->second.get_if<jopp::object>();
		EXPECT_EQ(std::size(obj), 0);
	}
	{
		auto const& array = *root.find("empty array")->second.get_if<jopp::array>();
		EXPECT_EQ(std::size(array), 0);
	}
	{
		auto const& obj = *root.find("had")->second.get_if<jopp::object>();
		EXPECT_EQ(std::size(obj), 7);
		{
			auto const& array = *obj.find("tightly")->second.get_if<jopp::array>();
			EXPECT_EQ(std::size(array), 9);
			{
				auto const& inner_array = *array[0].get_if<jopp::array>();
				EXPECT_EQ(std::size(inner_array), 4);
				EXPECT_EQ(inner_array[0], jopp::value{4.0});
				EXPECT_EQ(inner_array[1], jopp::value{2.0});
				EXPECT_EQ(inner_array[2], jopp::value{3.0});
				EXPECT_EQ(inner_array[3], jopp::value{1.0});
			}
			{
				EXPECT_EQ(array[1], jopp::value{"feet"});
				EXPECT_EQ(array[2], jopp::value{true});
				EXPECT_EQ(array[3], jopp::value{2145719840.4312375});
				EXPECT_EQ(array[4], jopp::value{-286229488.0});
				EXPECT_EQ(array[5], jopp::value{true});
				EXPECT_EQ(array[6], jopp::value{true});
			}
			{
				auto const& obj = *array[7].get_if<jopp::object>();
				EXPECT_EQ(std::size(obj), 1);
				EXPECT_EQ(obj.find("object in array")->second, jopp::value{jopp::string{"bar"}});
			}
			{
				auto const& obj = *array[8].get_if<jopp::object>();
				EXPECT_EQ(std::size(obj), 1);
				EXPECT_EQ(obj.find("object with literal last")->second, jopp::value{jopp::null{}});
			}
		}
		{
			EXPECT_EQ(obj.find("sound")->second, jopp::value{false});
			EXPECT_EQ(obj.find("eaten")->second, jopp::value{false});
			EXPECT_EQ(obj.find("pull")->second, jopp::value{1285774482.782745});
			EXPECT_EQ(obj.find("long")->second, jopp::value{-1437168945.8634152});
			EXPECT_EQ(obj.find("independent")->second, jopp::value{-1451031326.0});
		}
		{
			auto const& inner_obj = *obj.find("repeated end")->second.get_if<jopp::object>();
			EXPECT_EQ(std::size(inner_obj), 1);
			EXPECT_EQ(inner_obj.find("value")->second, jopp::value{46.0});
		}
	}
	EXPECT_EQ(root.find("fireplace")->second, jopp::value{720535269.0});
	EXPECT_EQ(root.find("refused")->second, jopp::value{jopp::string{"better"}});
	EXPECT_EQ(root.find("wood")->second, jopp::value{jopp::string{"involved"}});
	EXPECT_EQ(root.find("without")->second, jopp::value{true});
	EXPECT_EQ(root.find("it")->second, jopp::value{false});
	EXPECT_EQ(root.find("testing null")->second, jopp::value{jopp::null{}});
	EXPECT_EQ(root.find("a key with esc seq\n\t\\foo\"")->second,
		jopp::value{jopp::string{"A value with esc seq\n\t\\foo\""}});
}

TESTCASE(jopp_parser_top_level_is_array)
{
	jopp::parser parser;
	std::string_view data{R"([3, 1, 2])"};

	jopp::value val;
	auto const res = parser.parse(data, val);
	EXPECT_EQ(res.line, 1);
	EXPECT_EQ(res.col, 9);
	EXPECT_EQ(res.ec, jopp::error_code::completed);
	EXPECT_EQ(*res.ptr, '\0');

	auto& array = *val.get_if<jopp::array>();
	EXPECT_EQ(std::size(array), 3);
	EXPECT_EQ(array[0], jopp::value{3.0});
	EXPECT_EQ(array[1], jopp::value{1.0});
	EXPECT_EQ(array[2], jopp::value{2.0});
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

TESTCASE(jopp_parser_duplicate_key)
{
	jopp::parser parser;
	std::string_view data{R"({
	"the key": "123",
	"the key": "124"
})"};

	jopp::value val;
	auto const res = parser.parse(data, val);
	EXPECT_EQ(res.line, 3);
	EXPECT_EQ(res.col, 17);
	EXPECT_EQ(res.ec, jopp::error_code::key_already_exists);
	EXPECT_EQ(*res.ptr, '\n');
}

TESTCASE(jopp_parser_missing_esc_char_in_value)
{
	jopp::parser parser;
	std::string_view data{R"({
	"the key": "123
"
})"};

	jopp::value val;
	auto const res = parser.parse(data, val);
	EXPECT_EQ(res.line, 2);
	EXPECT_EQ(res.col, 17);
	EXPECT_EQ(res.ec, jopp::error_code::character_must_be_escaped);
	EXPECT_EQ(*res.ptr, '"');
}

TESTCASE(jopp_parser_unsupp_esc_seq_in_value)
{
	jopp::parser parser;
	std::string_view data{R"({"the key": "123\u0000"})"};

	jopp::value val;
	auto const res = parser.parse(data, val);
	EXPECT_EQ(res.line, 1);
	EXPECT_EQ(res.col, 18);
	EXPECT_EQ(res.ec, jopp::error_code::unsupported_escape_sequence);
	EXPECT_EQ(*res.ptr, '0');
}

TESTCASE(jopp_parser_junk_before_key)
{
	jopp::parser parser;
	std::string_view data{R"({junk"the key": "123"})"};

	jopp::value val;
	auto const res = parser.parse(data, val);
	EXPECT_EQ(res.line, 1);
	EXPECT_EQ(res.col, 2);
	EXPECT_EQ(res.ec, jopp::error_code::illegal_delimiter);
	EXPECT_EQ(*res.ptr, 'u');
}

TESTCASE(jopp_parser_missing_esc_char_in_key)
{
	jopp::parser parser;
	std::string_view data{R"({
	"the
key": "123"})"};

	jopp::value val;
	auto const res = parser.parse(data, val);
	EXPECT_EQ(res.line, 2);
	EXPECT_EQ(res.col, 6);
	EXPECT_EQ(res.ec, jopp::error_code::character_must_be_escaped);
	EXPECT_EQ(*res.ptr, 'k');
}

TESTCASE(jopp_parser_unsupp_esc_seq_in_key)
{
	jopp::parser parser;
	std::string_view data{R"({"the key\u0000": "123"})"};

	jopp::value val;
	auto const res = parser.parse(data, val);
	EXPECT_EQ(res.line, 1);
	EXPECT_EQ(res.col, 11);
	EXPECT_EQ(res.ec, jopp::error_code::unsupported_escape_sequence);
	EXPECT_EQ(*res.ptr, '0');
}

TESTCASE(jopp_parser_junk_before_value)
{
	jopp::parser parser;
	std::string_view data{R"({"the key" junk: "123"})"};

	jopp::value val;
	auto const res = parser.parse(data, val);
	EXPECT_EQ(res.line, 1);
	EXPECT_EQ(res.col, 12);
	EXPECT_EQ(res.ec, jopp::error_code::illegal_delimiter);
	EXPECT_EQ(*res.ptr, 'u');
}

TESTCASE(jopp_parser_junk_after_value_object_string)
{
	jopp::parser parser;
	std::string_view data{R"({"the key": "123" junk})"};

	jopp::value val;
	auto const res = parser.parse(data, val);
	EXPECT_EQ(res.line, 1);
	EXPECT_EQ(res.col, 19);
	EXPECT_EQ(res.ec, jopp::error_code::illegal_delimiter);
	EXPECT_EQ(*res.ptr, 'u');
}

TESTCASE(jopp_parser_junk_after_value_object_other_1)
{
	jopp::parser parser;
	std::string_view data{R"({"the key": 123 junk})"};

	jopp::value val;
	auto const res = parser.parse(data, val);
	EXPECT_EQ(res.line, 1);
	EXPECT_EQ(res.col, 17);
	EXPECT_EQ(res.ec, jopp::error_code::illegal_delimiter);
	EXPECT_EQ(*res.ptr, 'u');
}

TESTCASE(jopp_parser_junk_after_value_object_other_2)
{
	jopp::parser parser;
	std::string_view data{R"({"the key": 123 , junk})"};

	jopp::value val;
	auto const res = parser.parse(data, val);
	EXPECT_EQ(res.line, 1);
	EXPECT_EQ(res.col, 19);
	EXPECT_EQ(res.ec, jopp::error_code::illegal_delimiter);
	EXPECT_EQ(*res.ptr, 'u');
}

TESTCASE(jopp_parser_junk_after_value_array_string)
{
	jopp::parser parser;
	std::string_view data{R"(["A string" 123])"};

	jopp::value val;
	auto const res = parser.parse(data, val);
	EXPECT_EQ(res.line, 1);
	EXPECT_EQ(res.col, 13);
	EXPECT_EQ(res.ec, jopp::error_code::illegal_delimiter);
	EXPECT_EQ(*res.ptr, '2');
}

TESTCASE(jopp_parser_junk_after_value_array_other)
{
	jopp::parser parser;
	std::string_view data{R"([123 456])"};

	jopp::value val;
	auto const res = parser.parse(data, val);
	EXPECT_EQ(res.line, 1);
	EXPECT_EQ(res.col, 6);
	EXPECT_EQ(res.ec, jopp::error_code::illegal_delimiter);
	EXPECT_EQ(*res.ptr, '5');
}

TESTCASE(jopp_parser_store_object_duplicate_key)
{
	jopp::parser parser;
	std::string_view data{R"({"key a": 456,
	"key a": {}
})"};

	jopp::value val;
	auto const res = parser.parse(data, val);
	EXPECT_EQ(res.line, 2);
	EXPECT_EQ(res.col, 12);
	EXPECT_EQ(*res.ptr, '\n');
	EXPECT_EQ(res.ec, jopp::error_code::key_already_exists);
}

TESTCASE(jopp_parser_terminate_object_as_array)
{
	jopp::parser parser;
	std::string_view data{R"({"key a": 456,
	"key b": 5687
])"};

	jopp::value val;
	auto const res = parser.parse(data, val);
	EXPECT_EQ(res.ec, jopp::error_code::illegal_delimiter);
	EXPECT_EQ(res.line, 3);
	EXPECT_EQ(res.col, 1);
	EXPECT_EQ(*res.ptr, '\0');
}

TESTCASE(jopp_parser_terminate_array_as_object)
{
	jopp::parser parser;
	std::string_view data{R"([456,
	5687
})"};

	jopp::value val;
	auto const res = parser.parse(data, val);
	EXPECT_EQ(res.ec, jopp::error_code::illegal_delimiter);
	EXPECT_EQ(res.line, 3);
	EXPECT_EQ(res.col, 1);
	EXPECT_EQ(*res.ptr, '\0');
}