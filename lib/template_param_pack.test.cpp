//@	{"target":{"name":"template_param_pack.test"}}

#include "./template_param_pack.hpp"

#include <variant>
#include <testfwk/testfwk.hpp>

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