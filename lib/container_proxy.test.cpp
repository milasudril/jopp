//@	{"target":{"name":"./container_proxy.test"}}

#include "./container_proxy.hpp"

#include <list>
#include <deque>
#include <vector>

#include <testfwk/testfwk.hpp>

TESTCASE(jopp2_selected_iterator_type)
{
	static_assert(!std::random_access_iterator<std::list<int>::iterator>);
	static_assert(
		std::is_same_v<
			jopp2::selected_iterator<std::list<int>>::type,
			std::list<int>::iterator
		>
	);
	static_assert(
		std::is_same_v<
			jopp2::selected_iterator<std::list<int> const>::type,
			std::list<int>::const_iterator
		>
	);

	static_assert(std::random_access_iterator<std::deque<int>::iterator>);
	static_assert(!std::contiguous_iterator<std::deque<int>::iterator>);
	static_assert(
		std::is_same_v<
			jopp2::selected_iterator<std::deque<int> const>::type,
			std::deque<int>::const_iterator
		>
	);
	static_assert(
		std::is_same_v<
			jopp2::selected_iterator<std::deque<int>>::type,
			std::deque<int>::iterator
		>
	);

	static_assert(std::random_access_iterator<std::vector<int>::iterator>);
	static_assert(std::contiguous_iterator<std::vector<int>::iterator>);
	static_assert(
		std::is_same_v<
			jopp2::selected_iterator<std::vector<int> const>::type,
			int const*
		>
	);
	static_assert(
		std::is_same_v<
			jopp2::selected_iterator<std::vector<int>>::type,
			int*
		>
	);
}

TESTCASE(jopp2_selected_iterator_get_begin)
{
	std::list<int> a_list{1, 2, 3};
	EXPECT_EQ(
		jopp2::selected_iterator<std::list<int>>::get_begin(a_list),
		std::ranges::begin(a_list)
	);

	std::deque<int> a_deque{1, 2, 3};
	EXPECT_EQ(
		jopp2::selected_iterator<std::deque<int>>::get_begin(a_deque),
		std::ranges::begin(a_deque)
	);

	std::vector<int> a_vector{1, 2, 3};
	EXPECT_EQ(
		jopp2::selected_iterator<std::vector<int>>::get_begin(a_vector),
		std::ranges::data(a_vector)
	);
}

TESTCASE(jopp2_selected_iterator_get_end)
{
	std::list<int> a_list{1, 2, 3};
	EXPECT_EQ(
		jopp2::selected_iterator<std::list<int>>::get_end(a_list),
		std::ranges::end(a_list)
	);

	std::deque<int> a_deque{1, 2, 3};
	EXPECT_EQ(
		jopp2::selected_iterator<std::deque<int>>::get_end(a_deque),
		std::ranges::end(a_deque)
	);

	std::vector<int> a_vector{1, 2, 3};
	EXPECT_EQ(
		jopp2::selected_iterator<std::vector<int>>::get_end(a_vector),
		std::ranges::data(a_vector) + std::ranges::size(a_vector)
	);
}

#if 0
TESTCASE(jopp2_container_proxy_mutable_non_random_access_container)
{
	std::list<int> backing_store{1, 2, 3, 5};

	jopp2::container_proxy proxy{std::ref(backing_store)};

	{
		auto const active_range = proxy.active_range();
		EXPECT_EQ(active_range.front(), 1);
		EXPECT_EQ(active_range.back(), 5);
		EXPECT_EQ(proxy.total_size(), 4);
	}

	{
		proxy.pop_active_element();
		auto const active_range = proxy.active_range();
		EXPECT_EQ(active_range.front(), 2);
		EXPECT_EQ(active_range.back(), 5);
		EXPECT_EQ(proxy.total_size(), 4);
	}

	{
		proxy.pop_active_elements(2);
		auto const active_range = proxy.active_range();
		EXPECT_EQ(active_range.front(), 5);
		EXPECT_EQ(active_range.back(), 5);
		EXPECT_EQ(proxy.total_size(), 4);
	}

	EXPECT_EQ(backing_store.front(), 1);
	EXPECT_EQ(backing_store.back(), 5);

	proxy.replace_backing_store(std::list<int>{3, 6, 7, 1, 8});
	EXPECT_EQ(proxy.total_size(), 5);
	EXPECT_EQ(std::size(backing_store), 5);

	{
		auto const active_range = proxy.active_range();
		EXPECT_EQ(active_range.front(), 3);
		EXPECT_EQ(active_range.back(), 8);
	}

	EXPECT_EQ(std::size(backing_store), 5);
	EXPECT_EQ(backing_store.front(), 3);
	EXPECT_EQ(backing_store.back(), 8);

	proxy.clear_backing_store();
	EXPECT_EQ(std::size(backing_store), 0);
	EXPECT_EQ(proxy.total_size(), 0);
}

TESTCASE(jopp2_container_proxy_const_non_random_access_container)
{
	std::list<int> backing_store{1, 2, 3, 5};

	jopp2::container_proxy proxy{std::cref(backing_store)};

	{
		auto const active_range = proxy.active_range();
		EXPECT_EQ(active_range.front(), 1);
		EXPECT_EQ(active_range.back(), 5);
	}

	{
		proxy.pop_active_element();
		auto const active_range = proxy.active_range();
		EXPECT_EQ(active_range.front(), 2);
		EXPECT_EQ(active_range.back(), 5);
	}

	{
		proxy.pop_active_elements(2);
		auto const active_range = proxy.active_range();
		EXPECT_EQ(active_range.front(), 5);
		EXPECT_EQ(active_range.back(), 5);
	}

	EXPECT_EQ(backing_store.front(), 1);
	EXPECT_EQ(backing_store.back(), 5);
}
#endif
