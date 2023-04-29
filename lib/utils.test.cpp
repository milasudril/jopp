//@	{"target":{"name":"utils.test"}}

#include "./utils.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(jopp_utils_iterator_enumerator_default_valref)
{
	std::array<int, 16> vals{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16};

	jopp::iterator_enumerator rangeproc{std::begin(vals), std::end(vals)};

	size_t count = 0;

	while(true)
	{
		auto const ptr = rangeproc.pop_element();
		if(ptr == nullptr)
		{ break; }
		EXPECT_EQ(*ptr, vals[count]);
		++count;
	}

	EXPECT_EQ(count, std::size(vals));
}

namespace
{
	struct my_valref
	{
		int* ptr;
	};
}

TESTCASE(jopp_utils_iterator_enumerator_custom_valref)
{
	std::array<int, 16> vals{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16};
	using iterator = std::array<int, 16>::iterator;

	jopp::iterator_enumerator<iterator, my_valref> rangeproc{std::begin(vals), std::end(vals)};

	size_t count = 0;

	while(true)
	{
		auto const ref = rangeproc.pop_element();
		if(ref.ptr == nullptr)
		{ break; }
		EXPECT_EQ(*ref.ptr, vals[count]);
		++count;
	}

	EXPECT_EQ(count, std::size(vals));
}

TESTCASE(jopp_utils_range_processor_custom_valref)
{
	std::array<int, 16> vals{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16};
	jopp::range_processor<my_valref> rangeproc{std::begin(vals), std::end(vals)};

	size_t count = 0;

	while(true)
	{
		auto const ref = rangeproc.pop_element();
		if(ref.ptr == nullptr)
		{ break; }
		EXPECT_EQ(*ref.ptr, vals[count]);
		++count;
	}

	EXPECT_EQ(count, std::size(vals));
}
