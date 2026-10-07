//@	{"target":{"name":"value_sink_ref.test"}}

#include "./value_sink_ref.hpp"
#include "testfwk/validation.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(jopp2_array_union_set_and_get_values)
{
	{
		constexpr jopp2::array_union<4> data{
			std::array<char, 1>{'A'}
		};

		constexpr auto stored_value = get<1>(data);
		static_assert(stored_value.size() == 1);
		static_assert(stored_value.at(0) == 'A');
	}

	{
		constexpr jopp2::array_union<4> data{
			std::array<char, 2>{'A', 'B'}
		};

		constexpr auto stored_value = get<2>(data);
		static_assert(stored_value.size() == 2);
		static_assert(stored_value.at(0) == 'A');
		static_assert(stored_value.at(1) == 'B');
	}

	{
		constexpr jopp2::array_union<4> data{
			std::array<char, 3>{'A', 'B', 'C'}
		};

		constexpr auto stored_value = get<3>(data);
		static_assert(stored_value.size() == 3);
		static_assert(stored_value.at(0) == 'A');
		static_assert(stored_value.at(1) == 'B');
		static_assert(stored_value.at(2) == 'C');
	}

	{
		constexpr jopp2::array_union<4> data{
			std::array<char, 4>{'A', 'B', 'C', 'D'}
		};

		constexpr auto stored_value = get<4>(data);
		static_assert(stored_value.size() == 4);
		static_assert(stored_value.at(0) == 'A');
		static_assert(stored_value.at(1) == 'B');
		static_assert(stored_value.at(2) == 'C');
		static_assert(stored_value.at(3) == 'D');
	}
}

namespace
{
	struct sink_ref_traits
	{
		template<class T>
		using source_value_type = T;

		template<class T>
		static constexpr auto source_type_id = 0;

		static constexpr int& store_value(int& sink, int source)
		{
			sink = source;
			return sink;
		}
	};
#if __cpp_constexpr >= 202306L
	consteval int set_value_throug_sink_ref(int new_val)
	{
		int ret{};
		jopp2::value_sink_ref<sink_ref_traits> sink_ref{std::ref(ret)};
		sink_ref.store_value(new_val);
		return ret;
	}
#endif
}

TESTCASE(jopp2_value_sink_ref_store_int)
{
#if __cpp_constexpr >= 202306L
	static_assert(set_value_throug_sink_ref(243) == 243);
#endif

	int value{};
	jopp2::value_sink_ref<sink_ref_traits> sink_ref{std::ref(value)};
	auto ret = sink_ref.store_value(243);
	EXPECT_EQ(value, 243);
	EXPECT_EQ(ret.is_bound(), true);
	EXPECT_EQ(static_cast<bool>(ret), true);
	EXPECT_EQ(ret.accepts_type<int>(), true);
	EXPECT_EQ(ret.is_bound_to(value), true);
}
