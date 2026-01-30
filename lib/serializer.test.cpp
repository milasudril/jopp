//@	{"target":{"name":"serializer.test"}}

#include "./serializer.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(jopp_serializer_serialize)
{
	jopp::object root;
	root.insert("empty object", jopp::object{});
	root.insert("empty array", jopp::array{});
	jopp::object had;
	jopp::array tightly;
	jopp::array inner_array;
	inner_array.push_back(4.0);
	inner_array.push_back(2.0);
	inner_array.push_back(3.0);
	inner_array.push_back(1.0);
	tightly.push_back(std::move(inner_array));
	tightly.push_back("feet");
	tightly.push_back(true);
	tightly.push_back(2145719840.4312375);
	tightly.push_back(-286229488.0);
	tightly.push_back(true);
	tightly.push_back(true);
	jopp::object object_in_array;
	object_in_array.insert("object in array", "bar");
	tightly.push_back(std::move(object_in_array));
	jopp::object object_with_literal_last;
	object_with_literal_last.insert("object with literal last", jopp::null{});
	tightly.push_back(std::move(object_with_literal_last));
	had.insert("tightly", std::move(tightly));
	had.insert("sound", false);
	had.insert("eaten", false);
	had.insert("pull", 1285774482.782745);
	had.insert("long", -1437168945.8634152);
	had.insert("independent", -1451031326.0);
	jopp::object repeated_end;
	repeated_end.insert("value", 46.0);
	had.insert("repeated end", std::move(repeated_end));
	root.insert("had", std::move(had));
	root.insert("fireplace", 720535269.0),
	root.insert("refused", "better"),
	root.insert("wood", "involved"),
	root.insert("without", true),
	root.insert("it", false),
	root.insert("testing null", jopp::null{}),
	root.insert(R"(a key with esc seq
	\foo")", R"(A value with esc seq
	\foo")");

	jopp::serializer serializer{root, true};
	std::array<char, 1024> buffer{};
	auto res = serializer.serialize(buffer);
	EXPECT_EQ(res.ec, jopp::serializer_error_code::completed);
	REQUIRE_NE(res.ptr, std::data(buffer) + 1024);
	EXPECT_EQ(*res.ptr, '\0');
	EXPECT_EQ(*(res.ptr - 2), '}');
	EXPECT_EQ(*(res.ptr - 1), '\n');

	std::string_view expected_result{R"({
	"a key with esc seq\n\t\\foo\"": "A value with esc seq\n\t\\foo\"",
	"empty array": [

	],
	"empty object": {

	},
	"fireplace": 720535269,
	"had": {
		"eaten": false,
		"independent": -1451031326,
		"long": -1437168945.8634152,
		"pull": 1285774482.782745,
		"repeated end": {
			"value": 46
		},
		"sound": false,
		"tightly": [
			[
				4,
				2,
				3,
				1
			],
			"feet",
			true,
			2145719840.4312375,
			-286229488,
			true,
			true,
			{
				"object in array": "bar"
			},
			{
				"object with literal last": null
			}
		]
	},
	"it": false,
	"refused": "better",
	"testing null": null,
	"without": true,
	"wood": "involved"
}
)"};
	EXPECT_EQ(std::data(buffer), expected_result);
}

TESTCASE(jopp_serializer_serialize_blocks)
{
	jopp::object root;
	root.insert("empty object", jopp::object{});
	root.insert("empty array", jopp::array{});
	jopp::object had;
	jopp::array tightly;
	jopp::array inner_array;
	inner_array.push_back(4.0);
	inner_array.push_back(2.0);
	inner_array.push_back(3.0);
	inner_array.push_back(1.0);
	tightly.push_back(std::move(inner_array));
	tightly.push_back("feet");
	tightly.push_back(true);
	tightly.push_back(2145719840.4312375);
	tightly.push_back(-286229488.0);
	tightly.push_back(true);
	tightly.push_back(true);
	jopp::object object_in_array;
	object_in_array.insert("object in array", "bar");
	tightly.push_back(std::move(object_in_array));
	jopp::object object_with_literal_last;
	object_with_literal_last.insert("object with literal last", jopp::null{});
	tightly.push_back(std::move(object_with_literal_last));
	had.insert("tightly", std::move(tightly));
	had.insert("sound", false);
	had.insert("eaten", false);
	had.insert("pull", 1285774482.782745);
	had.insert("long", -1437168945.8634152);
	had.insert("independent", -1451031326.0);
	jopp::object repeated_end;
	repeated_end.insert("value", 46.0);
	had.insert("repeated end", std::move(repeated_end));
	root.insert("had", std::move(had));
	root.insert("fireplace", 720535269.0),
	root.insert("refused", "better"),
	root.insert("wood", "involved"),
	root.insert("without", true),
	root.insert("it", false),
	root.insert("testing null", jopp::null{}),
	root.insert(R"(a key with esc seq
	\foo")", R"(A value with esc seq
	\foo")");

	jopp::serializer serializer{root, true};
	std::array<char, 1024> buffer{};
	auto ptr = std::begin(buffer);
	while(true)
	{
		auto res = serializer.serialize(std::span{ptr, ptr + 13});
		if(res.ec == jopp::serializer_error_code::completed)
		{ break; }
		ptr = res.ptr;
	}

	std::string_view expected_result{R"({
	"a key with esc seq\n\t\\foo\"": "A value with esc seq\n\t\\foo\"",
	"empty array": [

	],
	"empty object": {

	},
	"fireplace": 720535269,
	"had": {
		"eaten": false,
		"independent": -1451031326,
		"long": -1437168945.8634152,
		"pull": 1285774482.782745,
		"repeated end": {
			"value": 46
		},
		"sound": false,
		"tightly": [
			[
				4,
				2,
				3,
				1
			],
			"feet",
			true,
			2145719840.4312375,
			-286229488,
			true,
			true,
			{
				"object in array": "bar"
			},
			{
				"object with literal last": null
			}
		]
	},
	"it": false,
	"refused": "better",
	"testing null": null,
	"without": true,
	"wood": "involved"
}
)"};
	EXPECT_EQ(std::data(buffer), expected_result);
}

TESTCASE(jopp_serializer_bad_char_in_key)
{
	jopp::object obj;
	std::string key{"this is_illegal"};
	key[7] = '\0';
	obj.insert(std::move(key), 42.0);
	std::array<char, 1024> buffer{};
	jopp::serializer serializer{obj, true};
	auto res = serializer.serialize(buffer);
	EXPECT_EQ(res.ec, jopp::serializer_error_code::illegal_char_in_string);
	EXPECT_EQ(res.ptr, std::data(buffer) + 2);
}

TESTCASE(jopp_serializer_bad_char_in_string_value)
{
	jopp::object obj;
	std::string value{"this is_illegal"};
	value[7] = '\0';
	obj.insert("hello, world", std::move(value));
	std::array<char, 1024> buffer{};
	jopp::serializer serializer{obj, true};
	auto res = serializer.serialize(buffer);
	EXPECT_EQ(res.ec, jopp::serializer_error_code::illegal_char_in_string);
	EXPECT_EQ(res.ptr, std::data(buffer) + 2);
}

TESTCASE(jopp_serializer_serialize_array)
{
	jopp::array obj;
	obj.push_back(1.0);
	obj.push_back(4.0);
	obj.push_back(2.0);
	obj.push_back(3.0);

	std::array<char, 1024> buffer{};
	jopp::serializer serializer{obj, true};
	auto res = serializer.serialize(buffer);
	EXPECT_EQ(res.ec, jopp::serializer_error_code::completed);
	EXPECT_EQ(std::data(buffer), std::string_view{R"([
	1,
	4,
	2,
	3
]
)"});
}

TESTCASE(jopp_serializer_serialize_blocks_compact)
{
	jopp::object root;
	root.insert("empty object", jopp::object{});
	root.insert("empty array", jopp::array{});
	jopp::object had;
	jopp::array tightly;
	jopp::array inner_array;
	inner_array.push_back(4.0);
	inner_array.push_back(2.0);
	inner_array.push_back(3.0);
	inner_array.push_back(1.0);
	tightly.push_back(std::move(inner_array));
	tightly.push_back("feet");
	tightly.push_back(true);
	tightly.push_back(2145719840.4312375);
	tightly.push_back(-286229488.0);
	tightly.push_back(true);
	tightly.push_back(true);
	jopp::object object_in_array;
	object_in_array.insert("object in array", "bar");
	tightly.push_back(std::move(object_in_array));
	jopp::object object_with_literal_last;
	object_with_literal_last.insert("object with literal last", jopp::null{});
	tightly.push_back(std::move(object_with_literal_last));
	had.insert("tightly", std::move(tightly));
	had.insert("sound", false);
	had.insert("eaten", false);
	had.insert("pull", 1285774482.782745);
	had.insert("long", -1437168945.8634152);
	had.insert("independent", -1451031326.0);
	jopp::object repeated_end;
	repeated_end.insert("value", 46.0);
	had.insert("repeated end", std::move(repeated_end));
	root.insert("had", std::move(had));
	root.insert("fireplace", 720535269.0),
	root.insert("refused", "better"),
	root.insert("wood", "involved"),
	root.insert("without", true),
	root.insert("it", false),
	root.insert("testing null", jopp::null{}),
	root.insert(R"(a key with esc seq
	\foo")", R"(A value with esc seq
	\foo")");

	jopp::serializer serializer{root, false};
	std::array<char, 1024> buffer{};
	auto ptr = std::begin(buffer);
	while(true)
	{
		auto res = serializer.serialize(std::span{ptr, ptr + 13});
		if(res.ec == jopp::serializer_error_code::completed)
		{ break; }
		ptr = res.ptr;
	}
	std::string_view expected_result{R"({"a key with esc seq\n\t\\foo\"":"A value with esc seq\n\t\\foo\"","empty array":[],"empty object":{},"fireplace":720535269,"had":{"eaten":false,"independent":-1451031326,"long":-1437168945.8634152,"pull":1285774482.782745,"repeated end":{"value":46},"sound":false,"tightly":[[4,2,3,1],"feet",true,2145719840.4312375,-286229488,true,true,{"object in array":"bar"},{"object with literal last":null}]},"it":false,"refused":"better","testing null":null,"without":true,"wood":"involved"}
)"};
	EXPECT_EQ(std::data(buffer), expected_result);
}


TESTCASE(jopp_serializer_to_string)
{
	jopp::object root;
	root.insert("empty object", jopp::object{});
	root.insert("empty array", jopp::array{});
	jopp::object had;
	jopp::array tightly;
	jopp::array inner_array;
	inner_array.push_back(4.0);
	inner_array.push_back(2.0);
	inner_array.push_back(3.0);
	inner_array.push_back(1.0);
	tightly.push_back(std::move(inner_array));
	tightly.push_back("feet");
	tightly.push_back(true);
	tightly.push_back(2145719840.4312375);
	tightly.push_back(-286229488.0);
	tightly.push_back(true);
	tightly.push_back(true);
	jopp::object object_in_array;
	object_in_array.insert("object in array", "bar");
	tightly.push_back(std::move(object_in_array));
	jopp::object object_with_literal_last;
	object_with_literal_last.insert("object with literal last", jopp::null{});
	tightly.push_back(std::move(object_with_literal_last));
	had.insert("tightly", std::move(tightly));
	had.insert("sound", false);
	had.insert("eaten", false);
	had.insert("pull", 1285774482.782745);
	had.insert("long", -1437168945.8634152);
	had.insert("independent", -1451031326.0);
	jopp::object repeated_end;
	repeated_end.insert("value", 46.0);
	had.insert("repeated end", std::move(repeated_end));
	root.insert("had", std::move(had));
	root.insert("fireplace", 720535269.0),
	root.insert("refused", "better"),
	root.insert("wood", "involved"),
	root.insert("without", true),
	root.insert("it", false),
	root.insert("testing null", jopp::null{}),
	root.insert(R"(a key with esc seq
	\foo")", R"(A value with esc seq
	\foo")");

	auto res = to_string(root);
	std::string_view expected_result{R"({"a key with esc seq\n\t\\foo\"":"A value with esc seq\n\t\\foo\"","empty array":[],"empty object":{},"fireplace":720535269,"had":{"eaten":false,"independent":-1451031326,"long":-1437168945.8634152,"pull":1285774482.782745,"repeated end":{"value":46},"sound":false,"tightly":[[4,2,3,1],"feet",true,2145719840.4312375,-286229488,true,true,{"object in array":"bar"},{"object with literal last":null}]},"it":false,"refused":"better","testing null":null,"without":true,"wood":"involved"}
)"};
	EXPECT_EQ(res, expected_result);
}