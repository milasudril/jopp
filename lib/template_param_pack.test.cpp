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

TESTCASE(jopp2_cocatenate_template_param_pack)
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