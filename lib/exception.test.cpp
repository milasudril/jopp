//@	{"target":{"name":"exception.test"}}

#include "./exception.hpp"

#include <testfwk/testfwk.hpp>
#include <unistd.h>
#include <sys/wait.h>

TESTCASE(jopp2_exception_what)
{
	jopp2::exception foo{"This is a test {}", 123};
	EXPECT_EQ(foo.what(), std::string_view{"This is a test 123"});
	EXPECT_EQ((std::is_base_of_v<std::exception, jopp2::exception>), true);
}

TESTCASE(jopp2_raise_internal_error)
{
	TestFwk::expect_death(
		[](){
			jopp2::raise_internal_error("Something went wrong {} {}", jopp2::make_fmt_args(1, "Foobar"));
		},
		"jopp internal error: lib/exception.test.cpp:20: Something went wrong 1 Foobar\n",
		SIGABRT
	);
}
