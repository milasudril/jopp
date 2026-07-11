//@	{"target":{"name":"updater.test.cpp"}}

#include "./updater.hpp"

#include <array>
#include <variant>
#include <testfwk/testfwk.hpp>

namespace
{
	using long_array = std::array<char, 235>;

	struct test_updater_traits
	{
		THISCALL static void update(
			std::variant<std::string, int, long_array>& sink,
			jopp2::update_param_t<int> value
		)
		{ sink = jopp2::maybe_move(value); }

		THISCALL static void update(
			std::variant<std::string, int, long_array>& sink,
			jopp2::update_param_t<std::string> value
		)
		{ sink = jopp2::maybe_move(value); }

		THISCALL static void update(
			std::variant<std::string, int, long_array>& sink,
			jopp2::update_param_t<long_array> value
		)
		{ sink = jopp2::maybe_move(value); }
	};
}

TESTCASE(jopp2_updater_update_value)
{
	std::variant<std::string, int, long_array> value;
	jopp2::updater<std::string, int, long_array> updater{
		value,
		std::type_identity<test_updater_traits>{}
	};

	updater.update_with(124);
	EXPECT_EQ(std::get<int>(value), 124);

	updater.update_with(std::string{"Foobar"});
	EXPECT_EQ(std::get<std::string>(value), "Foobar");

	std::string moved_from_value{"Foobar 2"};
	updater.update_with(std::move(moved_from_value));
	EXPECT_EQ(std::get<std::string>(value), "Foobar 2");
	// NOLINTNEXTLINE
	EXPECT_EQ(moved_from_value.empty(), true);

	int some_l_value = 123;
	updater.update_with(some_l_value);
	EXPECT_EQ(std::get<int>(value), 123);

	std::string some_other_l_value{"Kaka"};
	updater.update_with(some_other_l_value);
	EXPECT_EQ(std::get<std::string>(value), "Kaka");
	EXPECT_EQ(some_other_l_value, "Kaka");

	std::string const yet_another_l_value{"Kaka 12"};
	updater.update_with(yet_another_l_value);
	EXPECT_EQ(std::get<std::string>(value), "Kaka 12");
	EXPECT_EQ(yet_another_l_value, "Kaka 12");

	updater.update_with(long_array{'X'});
	EXPECT_EQ(std::get<long_array>(value)[0], 'X');

}
