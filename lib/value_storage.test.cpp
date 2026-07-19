//@	{"target":{"name":"value_storage.test.cpp"}}

#include "./value_storage.hpp"

#include <array>
#include <variant>
#include <testfwk/testfwk.hpp>

namespace
{
	using long_array = std::array<char, 235>;

	template<class T>
	struct test_update_result;

	template<>
	struct test_update_result<int>
	{ using type = int*; };

	template<>
	struct test_update_result<std::string>
	{ using type = std::string*; };

	template<>
	struct test_update_result<long_array>
	{ using type = long_array*; };

	template<class T>
	using test_update_result_t = test_update_result<T>::type;

	struct test_value_storage_traits
	{
		UPDATE_CALLBACK static auto update(
			std::variant<std::string, int, long_array>& sink,
			jopp2::update_param_t<int> value
		)
		{
			sink = jopp2::maybe_move(value);
			return std::get_if<int>(&sink);
		}

		UPDATE_CALLBACK static auto update(
			std::variant<std::string, int, long_array>& sink,
			jopp2::update_param_t<std::string> value
		)
		{
			sink = jopp2::maybe_move(value);
			return std::get_if<std::string>(&sink);
		}

		UPDATE_CALLBACK static auto update(
			std::variant<std::string, int, long_array>& sink,
			jopp2::update_param_t<long_array> value
		)
		{
			sink = jopp2::maybe_move(value);
			return std::get_if<long_array>(&sink);
		}
	};
}

TESTCASE(jopp2_value_storage_update_value)
{
	std::variant<std::string, int, long_array> value;
	jopp2::value_storage<test_update_result_t, std::string, int, long_array> value_storage{
		value,
		std::type_identity<test_value_storage_traits>{}
	};

	auto const res1 = value_storage.update_with(124);
	EXPECT_EQ(std::get<int>(value), 124);
	EXPECT_EQ(std::get_if<int>(&value), res1);

	auto const res2 = value_storage.update_with(std::string{"Foobar"});
	EXPECT_EQ(std::get<std::string>(value), "Foobar");
	EXPECT_EQ(std::get_if<std::string>(&value), res2);

	std::string moved_from_value{"Foobar 2"};
	auto const res3 = value_storage.update_with(std::move(moved_from_value));
	EXPECT_EQ(std::get<std::string>(value), "Foobar 2");
	EXPECT_EQ(std::get_if<std::string>(&value), res3);
	// NOLINTNEXTLINE
	EXPECT_EQ(moved_from_value.empty(), true);

	int some_l_value = 123;
	auto const res4 = value_storage.update_with(some_l_value);
	EXPECT_EQ(std::get_if<int>(&value), res4);
	EXPECT_NE(res4, &some_l_value);
	EXPECT_EQ(std::get<int>(value), 123);

	std::string some_other_l_value{"Kaka"};
	auto const res5 = value_storage.update_with(some_other_l_value);
	EXPECT_EQ(std::get<std::string>(value), "Kaka");
	EXPECT_EQ(std::get_if<std::string>(&value), res5);
	EXPECT_EQ(some_other_l_value, "Kaka");

	std::string const yet_another_l_value{"Kaka 12"};
	auto const res6 = value_storage.update_with(yet_another_l_value);
	EXPECT_EQ(std::get<std::string>(value), "Kaka 12");
	EXPECT_EQ(std::get_if<std::string>(&value), res6);
	EXPECT_EQ(yet_another_l_value, "Kaka 12");

	auto const res7 = value_storage.update_with(long_array{'X'});
	EXPECT_EQ(std::get<long_array>(value)[0], 'X');
	EXPECT_EQ(std::get_if<long_array>(&value), res7);

}
