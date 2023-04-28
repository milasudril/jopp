//@	{"target":{"name":"types.test"}}

#include "./types.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(jopp_value_store_bool)
{
	jopp::value a{true};
	REQUIRE_NE(a.get_if<jopp::boolean>(), nullptr);
	EXPECT_EQ(*a.get_if<jopp::boolean>(), true);
	EXPECT_EQ(*std::as_const(a).get_if<jopp::boolean>(), true);
	EXPECT_EQ(is_null(a), false);

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
	EXPECT_EQ(is_null(a), true);

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
	EXPECT_EQ(is_null(a), false);

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
	EXPECT_EQ(is_null(a), false);

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
	EXPECT_EQ(is_null(a), false);

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
	EXPECT_EQ(is_null(a), false);

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
	EXPECT_EQ(is_null(a), true);
}

TESTCASE(jopp_make_value)
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

TESTCASE(jopp_to_number)
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

char const* (*test_to_string_null)(jopp::null) = jopp::to_string;

TESTCASE(jopp_to_string_null)
{
	auto const str = test_to_string_null(jopp::null{});
	EXPECT_EQ(str, std::string_view{"null"});
}

char const* (*test_to_string_bool)(jopp::boolean) = jopp::to_string;

TESTCASE(jopp_to_string_boolean)
{
	{
		auto const str = test_to_string_bool(true);
		EXPECT_EQ(str, std::string_view{"true"});
	}

	{
		auto const str = test_to_string_bool(false);
		EXPECT_EQ(str, std::string_view{"false"});
	}
}

TESTCASE(jopp_to_string_string_temporary)
{
	auto const str = jopp::to_string(jopp::string{"A long text that does not fit in sbo"});
	EXPECT_EQ(str, "A long text that does not fit in sbo");
}

TESTCASE(jopp_to_string_string_non_temporary)
{
	jopp::string input{"A long text that does not fit in sbo"};
	auto const str = jopp::to_string(input);
	EXPECT_EQ(str, "A long text that does not fit in sbo");
}

TESTCASE(jopp_to_string_number)
{
	auto val = jopp::to_string(1.25);
	EXPECT_EQ(val, "1.25");
}

TESTCASE(jopp_item_pointer_empty)
{
	jopp::item_pointer ptr{nullptr};
	EXPECT_EQ(ptr.has_value(), false);
}

TESTCASE(jopp_item_pointer_key_value)
{
	std::pair<std::string const, jopp::value> pair{"foo", jopp::value{124.0}};
	jopp::item_pointer ptr{&pair};

	EXPECT_EQ(ptr.has_value(), true);
	auto ret = ptr.visit(jopp::overload{
		[](jopp::value const&, std::pair<std::string const, jopp::value> const&) {
			return 1;
		},
		[](char const* key, jopp::value const& val, std::pair<std::string const, jopp::value> const& expected) {
			EXPECT_EQ(key, expected.first);
			EXPECT_EQ(val, expected.second);
			return 2;
		}
	}, pair);

	EXPECT_EQ(ret, 2);
}

TESTCASE(jopp_item_pointer_value)
{
	jopp::value val{124.0};
	jopp::item_pointer ptr{&val};

	EXPECT_EQ(ptr.has_value(), true);
	auto ret = ptr.visit(jopp::overload{
		[](jopp::value const& val, jopp::value const& expected) {
			EXPECT_EQ(val, expected);
			return 1;
		},
		[](char const*, jopp::value const&, jopp::value const&) {
			return 2;
		}
	}, val);

	EXPECT_EQ(ret, 1);
}