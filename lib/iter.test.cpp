//@	{"target":{"name":"iter.test"}}

#include "./iter.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(jopp2_iter_from_range)
{
	std::vector vals{35, 5, 3, 2, 4};

	jopp2::iter i{vals};

	size_t count = 0;
	while(!i.at_end())
	{
		EXPECT_EQ(i.next(), vals[count]);
		++count;
	}

	static_assert(std::is_same_v<decltype(i), jopp2::make_iter_t<decltype(vals)>>);
}

TESTCASE(jopp2_iter_from_value)
{
	auto val = 35;
	jopp2::iter i{val};
	EXPECT_EQ(i.at_end(), false);
	EXPECT_EQ(i.next(), val);
	EXPECT_EQ(i.at_end(), true);
	static_assert(std::is_same_v<decltype(i), jopp2::make_iter_t<int>>);
	static_assert(std::is_same_v<jopp2::make_iter_t<int>::iter_type, int*>);
	static_assert(std::is_same_v<jopp2::make_iter_t<int const>::iter_type, int const*>);
}
