//@	{"target":{"name":"template_param_pack.test"}}

#include "./template_param_pack.hpp"

#include <variant>
#include <testfwk/testfwk.hpp>

TESTCASE(jopp2_map_template_param_pack_properties)
{
	using my_pack = jopp2::template_param_pack<int, double>;

	static_assert(my_pack::size == 2);
	static_assert(std::is_same_v<jopp2::template_param_pack_type_at_index_t<0, my_pack>, int>);
	static_assert(std::is_same_v<jopp2::template_param_pack_type_at_index_t<1, my_pack>, double>);

	static_assert(
		std::is_same_v<
			jopp2::map_template_param_pack_to_type_t<std::variant, my_pack>,
			std::variant<int, double>
		>
	);

	static_assert(jopp2::get_index_of_type<int, int, double>() == 0);
	static_assert(jopp2::get_index_of_type<double, int, double>() == 1);

	static_assert(jopp2::index_of_type_v<int, my_pack> == 0);
	static_assert(jopp2::index_of_type_v<double, my_pack> == 1);
}

TESTCASE(jopp2_map_template_param_pack_to_type)
{
	using my_pack = jopp2::template_param_pack<int, double>;

	static_assert(
		std::is_same_v<
			jopp2::map_template_param_pack_to_type_t<std::variant, my_pack>,
			std::variant<int, double>
		>
	);
}

TESTCASE(jopp2_wrap_in_template_param_pack)
{
	using my_pack = jopp2::template_param_pack<int, double>;

	static_assert(std::is_same_v<jopp2::wrap_in_template_param_pack_t<my_pack>, my_pack>);
	static_assert(std::is_same_v<jopp2::wrap_in_template_param_pack_t<int>, jopp2::template_param_pack<int>>);
}

TESTCASE(jopp2_append_to_template_param_pack)
{
	using my_pack = jopp2::template_param_pack<int, double>;

	using appended_pack = jopp2::append_to_template_param_pack_t<my_pack, char>;

	static_assert(
		std::is_same_v<
			appended_pack,
			jopp2::template_param_pack<int, double, char>
		>
	);
}

TESTCASE(jopp2_concatenate_template_param_pack)
{
	using pack_a = jopp2::template_param_pack<float, double>;
	using pack_b = jopp2::template_param_pack<int, long long>;

	using total_pack = jopp2::concatenate_template_param_packs_t<pack_a, pack_b>;

	static_assert(
		std::is_same_v<
			total_pack,
			jopp2::template_param_pack<float, double, int ,long long>
		>
	);
}

namespace
{
	template<class Foo, class Bar>
	struct type_with_extra_arg
	{};

	struct test_type
	{};
}

TESTCASE(jopp2_wrap_template_param_pack_elements)
{
	using my_pack = jopp2::template_param_pack<float, double>;
	using my_new_pack = jopp2::wrap_template_param_pack_elements_t<
		my_pack,
		type_with_extra_arg,
		test_type
	>;

	static_assert(
		std::is_same_v<
			my_new_pack,
			jopp2::template_param_pack<
				type_with_extra_arg<float, test_type>,
				type_with_extra_arg<double, test_type>
			>
		>
	);
}
