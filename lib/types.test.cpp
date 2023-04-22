//@	{"target":{"name":"types.test"}}

#include "./types.hpp"

#include <testfwk/testfwk.hpp>

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
	jopp::value a{jopp::null{}};
	REQUIRE_NE(a.get_if<jopp::null>(), nullptr);
	EXPECT_EQ(*a.get_if<jopp::null>(), jopp::null{});
	EXPECT_EQ(*std::as_const(a).get_if<jopp::null>(), jopp::null{});

	auto x = *a.get_if<jopp::null>();
	EXPECT_EQ((std::is_same_v<decltype(x), jopp::null>), true);
	EXPECT_EQ(x, jopp::null{});

	auto y = *std::as_const(a).get_if<jopp::null>();
 	EXPECT_EQ((std::is_same_v<decltype(y), jopp::null>), true);
	EXPECT_EQ(y, jopp::null{});

	a.visit([]<class T>(T const&){
		EXPECT_EQ((std::is_same_v<T, jopp::null>), true);
	});

	std::as_const(a).visit([]<class T>(T const&){
		EXPECT_EQ((std::is_same_v<T, jopp::null>), true);
	});
}

TESTCASE(jopp_value_store_object)
{
	jopp::object obj;
	obj.insert("Foo", 1.25);
	jopp::value a{std::move(obj)};

	auto obj_retrieved = a.get_if<jopp::object>();
	REQUIRE_NE(obj_retrieved, nullptr);
	auto i = obj_retrieved->find("Foo");
	REQUIRE_NE(i, std::end(*obj_retrieved));
	EXPECT_EQ(i->first, "Foo");
	REQUIRE_NE(i->second.get_if<jopp::number>(), nullptr);
	EXPECT_EQ(*i->second.get_if<jopp::number>(), 1.25);

	a.visit([]<class T>(T const&){
		EXPECT_EQ((std::is_same_v<T, jopp::object>), true);
	});

	std::as_const(a).visit([]<class T>(T const&){
		EXPECT_EQ((std::is_same_v<T, jopp::object>), true);
	});
}

TESTCASE(jopp_value_store_array)
{
	jopp::array array;
	array.push_back(1.25);
	jopp::value a{std::move(array)};

	auto array_retrieved = a.get_if<jopp::array>();
	REQUIRE_NE(array_retrieved, nullptr);
	auto i = array_retrieved->begin();
	REQUIRE_NE(i, std::end(*array_retrieved));
	REQUIRE_NE(i->get_if<jopp::number>(), nullptr);
	EXPECT_EQ(*i->get_if<jopp::number>(), 1.25);

	a.visit([]<class T>(T const&){
		EXPECT_EQ((std::is_same_v<T, jopp::array>), true);
	});

	std::as_const(a).visit([]<class T>(T const&){
		EXPECT_EQ((std::is_same_v<T, jopp::array>), true);
	});
}

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

TESTCASE(jopp_value_store_string)
{
	jopp::value a{"Hello, Wolrd"};
	REQUIRE_NE(a.get_if<jopp::string>(), nullptr);
	EXPECT_EQ(*a.get_if<jopp::string>(), "Hello, Wolrd");
	EXPECT_EQ(*std::as_const(a).get_if<jopp::string>(), "Hello, Wolrd");

	auto x = *a.get_if<jopp::string>();
	EXPECT_EQ((std::is_same_v<decltype(x), jopp::string>), true);
	EXPECT_EQ(x, "Hello, Wolrd");

	auto y = *std::as_const(a).get_if<jopp::string>();
 	EXPECT_EQ((std::is_same_v<decltype(y), jopp::string>), true);
	EXPECT_EQ(y, "Hello, Wolrd");

	a.visit([]<class T>(T const&){
		EXPECT_EQ((std::is_same_v<T, jopp::string>), true);
	});

	std::as_const(a).visit([]<class T>(T const&){
		EXPECT_EQ((std::is_same_v<T, jopp::string>), true);
	});
}

TESTCASE(jopp_value_default_is_null)
{
	jopp::value a{};
	REQUIRE_NE(a.get_if<jopp::null>(), nullptr);
	EXPECT_EQ(*a.get_if<jopp::null>(), jopp::null{});
}


