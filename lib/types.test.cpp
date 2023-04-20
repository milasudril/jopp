//@	{"target":{"name":"types.test"}}

#include "./types.hpp"

#include <testfwk/testfwk.hpp>

/*
TODO:
	std::unique_ptr<object>,
	std::unique_ptr<array>,
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

TESTCASE(jopp_value_store_bool)
{
	jopp::value a{true};
	REQUIRE_NE(a.get_if<jopp::boolean>(), nullptr);
	EXPECT_EQ(*a.get_if<jopp::boolean>(), true);
	EXPECT_EQ(*std::as_const(a).get_if<jopp::boolean>(), true);

	auto x = *a.get_if<jopp::boolean>();
	EXPECT_EQ((std::is_same_v<decltype(x), jopp::boolean>), true);
	EXPECT_EQ(x, true);

	auto y = *std::as_const(a).get_if<jopp::boolean>();
 	EXPECT_EQ((std::is_same_v<decltype(y), jopp::boolean>), true);
	EXPECT_EQ(y, true);

	a.visit([]<class T>(T const&){
		EXPECT_EQ((std::is_same_v<T, jopp::boolean>), true);
	});

	std::as_const(a).visit([]<class T>(T const&){
		EXPECT_EQ((std::is_same_v<T, jopp::boolean>), true);
	});
}

TESTCASE(jopp_value_store_null)
{
	jopp::value a{nullptr};
	REQUIRE_NE(a.get_if<jopp::null>(), nullptr);
	EXPECT_EQ(*a.get_if<jopp::null>(), nullptr);
	EXPECT_EQ(*std::as_const(a).get_if<jopp::null>(), nullptr);

	auto x = *a.get_if<jopp::null>();
	EXPECT_EQ((std::is_same_v<decltype(x), jopp::null>), true);
	EXPECT_EQ(x, nullptr);

	auto y = *std::as_const(a).get_if<jopp::null>();
 	EXPECT_EQ((std::is_same_v<decltype(y), jopp::null>), true);
	EXPECT_EQ(y, nullptr);

	a.visit([]<class T>(T const&){
		EXPECT_EQ((std::is_same_v<T, jopp::null>), true);
	});

	std::as_const(a).visit([]<class T>(T const&){
		EXPECT_EQ((std::is_same_v<T, jopp::null>), true);
	});
}