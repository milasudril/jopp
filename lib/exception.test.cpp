//@	{"target":{"name":"exception.test"}}

#include "./exception.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(jopp2_exception_what)
{
	jopp2::exception foo{"This is a test {}", 123};
	EXPECT_EQ(foo.what(), std::string_view{"This is a test 123"});
	EXPECT_EQ((std::is_base_of_v<std::exception, jopp2::exception>), true);
}