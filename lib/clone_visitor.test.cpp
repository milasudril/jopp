//@	{"target":{"name":"clone_visitor.test"}}

#include "./clone_visitor.hpp"
#include "lib/node_visitor_adaptor.hpp"
#include "lib/template_param_pack.hpp"

#include <map>
#include <testfwk/testfwk.hpp>

namespace
{
	struct test_generic_value_in
	{
		using leaf_value_template_param_pack = jopp2::template_param_pack<
			int,
			std::string
		>;

		using object = std::map<std::variant<int, std::string>, test_generic_value_in>;

#if 0
		using value_type = std::variant<
			int,
			std::string,
			std::vector<int>,
			std::vector<std::string>,
			std::vector<test_generic_value_in>,
			object
		>;

		template<class T>
		using sequence_container_type = std::vector<T>;

		template<class T>
		static constexpr auto is_leaf_value = std::is_same_v<T, int> || std::is_same_v<T, std::string>;

		template<class Self>
		auto&& get_value(this Self&& self)
		{ return std::forward_like<Self>(std::forward<Self>(self).value); }

		value_type value;
#endif
	};

	struct test_generic_value_out
	{
		using object = std::map<std::variant<int, std::string>, test_generic_value_in>;
		using value_type = std::variant<
			int,
			std::string,
			std::vector<int>,
			std::vector<std::string>,
			std::vector<test_generic_value_in>,
			object
		>;

		template<class ... Args>
		requires(std::is_constructible_v<value_type, Args...>)
		explicit test_generic_value_out(Args&&... args):
			value{std::forward<Args>(args)...}
		{}
#if 0

		template<class T>
		using sequence_container_type = std::vector<T>;

		template<class T>
		static constexpr auto is_leaf_value = std::is_same_v<T, int> || std::is_same_v<T, std::string>;
#endif

		template<class Self>
		auto&& get_value(this Self&& self)
		{ return std::forward_like<Self>(std::forward<Self>(self).value); }

		value_type value;
		template<class T, class Self>
		auto get_if(this Self&& self)
		{ return std::get_if<std::remove_cvref_t<T>>(&std::forward<Self>(self).value); }
	};
}


TESTCASE(jopp2_clone_visitor_handle_leaf_value_currently_no_key)
{
	test_generic_value_out output;
	jopp2::clone_visitor_2<test_generic_value_in, test_generic_value_out> visitor{output};
	auto const res = visitor.handle_leaf_value(1234, jopp2::value_visitation_context{});
	EXPECT_EQ(res, jopp2::node_visitor_status::ready);
}
