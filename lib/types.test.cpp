//@	{"target":{"name":"types.test"}}

#include "./types.hpp"

#include <testfwk/testfwk.hpp>

/*
TODO:
	std::unique_ptr<object>,
	std::unique_ptr<array>,
	bool,
	std::nullptr_t
*/

TESTCASE(jopp_value_store_number)
{
	jopp::value a{1.25};
	REQUIRE_NE(a.get_if<jopp::number>(), nullptr);
	EXPECT_EQ(*a.get_if<jopp::number>(), 1.25);
	EXPECT_EQ(*std::as_const(a).get_if<jopp::number>(), 1.25);

	auto x = *a.get_if<jopp::number>();
	EXPECT_EQ((std::is_same_v<decltype(x), jopp::number>), true);
	EXPECT_EQ(x, 1.25);

	auto y = *std::as_const(a).get_if<jopp::number>();
 	EXPECT_EQ((std::is_same_v<decltype(y), jopp::number>), true);
	EXPECT_EQ(y, 1.25);

	a.visit([]<class T>(T const&){
		EXPECT_EQ((std::is_same_v<T, jopp::number>), true);
	});

	std::as_const(a).visit([]<class T>(T const&){
		EXPECT_EQ((std::is_same_v<T, jopp::number>), true);
	});
}