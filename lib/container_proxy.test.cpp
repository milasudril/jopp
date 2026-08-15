//@	{"target":{"name":"./container_proxy.test"}}

#include "./container_proxy.hpp"

#include <list>
#include <deque>
#include <vector>

#include <testfwk/testfwk.hpp>


TESTCASE(jopp2_container_proxy_mutable_non_random_access_container)
{
	std::list<int> backing_store{1, 2, 3, 5};

	jopp2::container_proxy proxy{backing_store};

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

	proxy.replace_backing_store(std::list<int>{3, 6, 7, 1, 8});

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
}

TESTCASE(jopp2_container_proxy_const_non_random_access_container)
{
	std::list<int> const backing_store{1, 2, 3, 5};

	jopp2::container_proxy proxy{backing_store};

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
