//@	{"target":{"name":"generic_value_update_traits.test"}}

#include "./generic_value_update_traits.hpp"

#include <testfwk/testfwk.hpp>
#include <testfwk/death_test.hpp>

TESTCASE(jopp2_generic_value_update_traits_update_generic_value)
{
	int value = 0;
	jopp2::generic_value_update_traits<int>::update(value, 1234.0);
	EXPECT_EQ(value, 1234);
}

TESTCASE(jopp2_generic_value_update_traits_update_generic_value_source_is_key)
{
	TestFwk::expect_death(
		[]{
			int value = 0;
			jopp2::generic_value_update_traits<int>::update(value, jopp2::key_to_clone{24});
		},
		"jopp internal error: lib/./generic_value_update_traits.hpp:29: Cannot store a key in a GenericValue\n",
		SIGABRT
	);
}
