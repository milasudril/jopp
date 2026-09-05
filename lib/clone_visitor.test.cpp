//@	{"target":{"name":"clone_visitor.test"}}

#include "./clone_visitor.hpp"
#include "lib/node_visitor_adaptor.hpp"
#include "lib/template_param_pack.hpp"
#include "testfwk/death_test.hpp"

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

		using key_type = std::variant<int, std::string>;

		using object = std::map<key_type, test_generic_value_in>;

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
		using object = std::map<std::variant<int, std::string>, test_generic_value_out>;
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

		struct emplace_ret_val
		{
			test_generic_value_out* value;
		};

		template<class Self, class T, class KeyLike>
		auto emplace(this Self& self, KeyLike&& key, T&& value)
		{
			using ret_type = emplace_ret_val;

			auto i = self.template get_if<object>();
			if(i == nullptr)
			{ return ret_type{}; }

			auto const insert_result = i->emplace(std::forward<KeyLike>(key), std::forward<T>(value));
			return ret_type{
				.value = &insert_result.first->second
			};
		}
	};
}

TESTCASE(jopp2_clone_visitor_handle_leaf_value_currently_no_key)
{
	test_generic_value_out output;
	jopp2::clone_visitor_2<test_generic_value_in, test_generic_value_out> visitor{output};
	auto const res = visitor.handle_leaf_value(1234, jopp2::value_visitation_context{});
	EXPECT_EQ(res, jopp2::node_visitor_status::ready);
}

TESTCASE(jopp2_clone_visitor_handle_int_key_current_value_is_not_an_object)
{
	test_generic_value_out output;
	jopp2::clone_visitor_2<test_generic_value_in, test_generic_value_out> visitor{output};
	TestFwk::expect_death(
		[&visitor]{
			std::ignore = visitor.handle_key(1234, jopp2::value_visitation_context{});
		},
		"jopp internal error: lib/./clone_visitor.hpp:64: lhs is not an obejct\n",
		SIGABRT
	);
}

TESTCASE(jopp2_clone_visitor_handle_int_key_current_value_is_an_object)
{
	test_generic_value_out output;
	jopp2::clone_visitor_2<test_generic_value_in, test_generic_value_out> visitor{output};
	output.value = test_generic_value_out::object{};

	auto& object = *output.get_if<test_generic_value_out::object>();

	{
		auto const res = visitor.handle_key(1234, jopp2::value_visitation_context{});
		EXPECT_EQ(res, jopp2::node_visitor_status::ready);
		EXPECT_EQ(object.contains(test_generic_value_out::object::key_type{1234}), true );
	}

	{
		auto const res = visitor.handle_leaf_value(66, jopp2::value_visitation_context{});
		EXPECT_EQ(res, jopp2::node_visitor_status::ready);
		EXPECT_EQ(*object.at(test_generic_value_out::object::key_type{1234}).get_if<int>(), 66);
	}
}
