//@	{"target":{"name":"updater.test.cpp"}}

#include "./updater.hpp"

#include <variant>
#include <testfwk/testfwk.hpp>

namespace
{
	struct test_updater_traits
	{
		static void update(std::variant<std::string, int>& sink, int value)
		{ sink = value; }

		static void update(std::variant<std::string, int>& sink, std::string&& value)
		{ sink = std::move(value); }
	};
}

TESTCASE(jopp2_updater_update_value)
{
	std::variant<std::string, int> value;
	jopp2::updater<std::string, int> updater{
		value,
		std::type_identity<test_updater_traits>{}
	};

	updater.update(124);
	EXPECT_EQ(std::get<int>(value), 124);

	updater.update(std::string{"Foobar"});
	EXPECT_EQ(std::get<std::string>(value), "Foobar");

	std::string moved_from_value{"Foobar 2"};
	updater.update(std::move(moved_from_value));
	EXPECT_EQ(std::get<std::string>(value), "Foobar 2");
	// NOLINTNEXTLINE
	EXPECT_EQ(moved_from_value.empty(), true);

	int some_l_value = 123;
	updater.update(some_l_value);
	EXPECT_EQ(std::get<int>(value), 123);

	std::string some_other_l_value{"Kaka"};
	updater.update(some_other_l_value);
	EXPECT_EQ(std::get<std::string>(value), "Kaka");
	EXPECT_EQ(some_other_l_value, "Kaka");

	std::string const yet_another_l_value{"Kaka 12"};
	updater.update(yet_another_l_value);
	EXPECT_EQ(std::get<std::string>(value), "Kaka 12");
	EXPECT_EQ(yet_another_l_value, "Kaka 12");
}
