//@	{"target":{"name":"value_sink_ref.test"}}

#include "./value_sink_ref.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(array_union_set_and_get_values)
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
