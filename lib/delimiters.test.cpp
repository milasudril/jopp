//@	{"target":{"name":"delimiters.test"}}

#include "./delimiters.hpp"

#include "testfwk/testfwk.hpp"

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
		if((k >= 0 && k <= 31) || k == '"' || k == '\\')
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
				EXPECT_EQ(jopp::unescape(static_cast<char>(k)), '"');
				break;

			case '\\':
				EXPECT_EQ(jopp::unescape(static_cast<char>(k)), '\\');
				break;

			case 'n':
				EXPECT_EQ(jopp::unescape(static_cast<char>(k)), '\n');
				break;

			case 't':
				EXPECT_EQ(jopp::unescape(static_cast<char>(k)), '\t');
				break;

			default:
				EXPECT_EQ(jopp::unescape(static_cast<char>(k)).has_value(), false);
		}
	}
}

TESTCASE(jopp_get_esc_char)
{
	for(int k = 0; k != 255; ++k)
	{
		switch(k)
		{
			case '"':
				EXPECT_EQ(jopp::get_escape_char(static_cast<char>(k)), '"');
				break;

			case '\\':
				EXPECT_EQ(jopp::get_escape_char(static_cast<char>(k)), '\\');
				break;

			case '\n':
				EXPECT_EQ(jopp::get_escape_char(static_cast<char>(k)), 'n');
				break;

			case '\t':
				EXPECT_EQ(jopp::get_escape_char(static_cast<char>(k)), 't');
				break;

			default:
				EXPECT_EQ(jopp::get_escape_char(static_cast<char>(k)).has_value(), false);
		}
	}
}

TESTCASE(jopp_escape_string)
{
	auto res = jopp::escape(
		std::string_view{"This is a test string that contains chars to escape \" \n \t \\"});

	EXPECT_EQ(res,
		std::string_view{"This is a test string that contains chars to escape \\\" \\n \\t \\\\"});
}

TESTCASE(jopp_escape_string_with_bad_char)
{
	auto res = jopp::escape(
		std::string_view{"This is a test string that contains bad chars \r"});

	EXPECT_EQ(res.has_value(), false);
}