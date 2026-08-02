//@	{"target":{"name": "generic_value.test"}}

#include "./generic_value.hpp"
#include "testfwk/death_test.hpp"
#include "testfwk/validation.hpp"

#include <vector>
#include <flat_map>
#include <testfwk/testfwk.hpp>
#include <format>
#include <print>

namespace
{
	struct my_value_traits_with_pack
	{
		using key_type = std::string;
		using leaf_value_type = jopp2::template_param_pack<int, double, std::string, char>;
	};

	struct my_value_traits_with_no_pack
	{
		using key_type = std::string;
		using leaf_value_type = std::string;
	};

	enum class bool_wrapper:bool{
		disabled = false,
		enabled = true
	};

	struct json_value_traits
	{
		using key_type = std::string;
		using leaf_value_type = jopp2::template_param_pack<std::monostate, bool_wrapper, double, std::string>;
	};

	template<class T>
	struct map_type_name
	{};

	template<>
	struct map_type_name<jopp2::src_object>
	{ static constexpr const char* name = "obj"; };

	template<>
	struct map_type_name<jopp2::src_value>
	{ static constexpr const char* name = ""; };

	template<>
	struct map_type_name<std::string>
	{ static constexpr const char* name = "str"; };

	template<>
	struct map_type_name<double>
	{ static constexpr const char* name = "fd"; };

	template<>
	struct map_type_name<bool_wrapper>
	{ static constexpr const char* name = "bool"; };

	template<>
	struct map_type_name<std::monostate>
	{ static constexpr const char* name = "null"; };

	struct test_node_visitor
	{
		void do_indent()
		{
			if(skip_indent)
			{
				skip_indent = false;
				return;
			}

			for(size_t k = 0; k != indentation; ++k)
			{ output.get() +="    "; }
		}

		static constexpr auto internal_to_string = jopp2::overload{
			[](std::monostate) {
				return "null";
			},
			[](bool_wrapper value) {
				return value == bool_wrapper::enabled? "true" : "false";
			},
			[](std::string const& str) {
				return str;
			},
			[](double x) {
				return std::format("{}", x);
			}
		};

		template<class T>
		void print_without_tag(T const& item, size_t index, size_t parent_container_size)
		{
			do_indent();
			if(index + 1== parent_container_size) [[unlikely]]
			{
				output.get() += std::format(
					"({} of {}) {}\n", index + 1, parent_container_size, internal_to_string(item)
				);
			}
			else
			{
				output.get() += std::format(
					"({} of {}) {},\n", index + 1, parent_container_size, internal_to_string(item)
				);
			}
		}

		template<class T>
		void handle_leaf_value_array(std::vector<T> const& src, jopp2::value_visitation_context context)
		{
			handle_begin_of_array(std::type_identity<T>{}, context);
			auto const size = std::size(src);
			for(auto&& [index, item]: std::ranges::enumerate_view{src})
			{ print_without_tag(item, static_cast<size_t>(index), size); }
			handle_end_of_array(std::type_identity<T>{}, context);
		}

		void handle_leaf_value(std::string const& str, jopp2::value_visitation_context context)
		{
			do_indent();
			auto const index = context.node_index ;
			auto const parent_container_size = context.parent_container_size;
			if(index + 1 == parent_container_size) [[unlikely]]
			{
				output.get() += std::format("({} of {}) str({})\n", index + 1, parent_container_size, str);
			}
			else
			{
				output.get() += std::format("({} of {}) str({}),\n", index + 1, parent_container_size, str);
			}
		}

		void handle_leaf_value(double value, jopp2::value_visitation_context context)
		{
			do_indent();
			auto const index = context.node_index ;
			auto const parent_container_size = context.parent_container_size;
			if(index + 1 == parent_container_size) [[unlikely]]
			{
				output.get() += std::format( "({} of {}) sd{}\n", index + 1, parent_container_size, value);
			}
			else
			{
				output.get() += std::format( "({} of {}) sd{},\n", index + 1, parent_container_size, value);
			}
		}

		void handle_leaf_value(std::monostate /*unused*/, jopp2::value_visitation_context context)
		{
			do_indent();
			auto const index = context.node_index ;
			auto const parent_container_size = context.parent_container_size;
			if(index + 1 == parent_container_size) [[unlikely]]
			{
				output.get() += std::format("({} of {}) null\n", index + 1, parent_container_size);
			}
			else
			{
				output.get() += std::format("({} of {}) null,\n", index + 1, parent_container_size);
			}
		}

		void handle_leaf_value(bool_wrapper value, jopp2::value_visitation_context context)
		{
			do_indent();
			auto const index = context.node_index ;
			auto const parent_container_size = context.parent_container_size;
			if(index + 1 == parent_container_size) [[unlikely]]
			{
				output.get() += std::format(
					"({} of {}) {}\n", index + 1, parent_container_size, internal_to_string(value)
				);
			}
			else
			{
				output.get() += std::format(
					"({} of {}) {},\n", index + 1, parent_container_size, internal_to_string(value)
				);
			}
		}

		void handle_property_name(std::string const& name, jopp2::value_visitation_context /*unused*/)
		{
			do_indent();
			output.get() += std::format("{}: ", name);
			skip_indent = true;
		}

		void handle_begin_of_object(jopp2::value_visitation_context context)
		{
			do_indent();
			output.get() += std::format(
				"({} of {}) {{\n",
				context.node_index + 1,
				context.parent_container_size
			);
			++indentation;
		}

		void handle_end_of_object(jopp2::value_visitation_context context)
		{
			--indentation;
			do_indent();
			if(!context.is_last_node())
			{ output.get() += "},\n"; }
			else
			{ output.get() += "}\n"; }
		}

		template<class T>
		void handle_begin_of_array(std::type_identity<T> /*unused*/, jopp2::value_visitation_context context)
		{
			assert(context.node_index < context.parent_container_size);
			do_indent();
			output.get() += std::format(
				"({} of {}) {}[\n",
				context.node_index + 1,
				context.parent_container_size,
				map_type_name<T>::name
			);
			++indentation;
		}

		template<class T>
		void handle_end_of_array(std::type_identity<T> /*unused*/, jopp2::value_visitation_context context)
		{
			--indentation;
			do_indent();
			if(!context.is_last_node())
			{ output.get() += "],\n"; }
			else
			{ output.get() += "]\n"; }
		}

		std::reference_wrapper<std::string> output;
		size_t indentation = 0;
		bool skip_indent = false;
	};
}

TESTCASE(jopp2_explain_lookup_error_code)
{
	EXPECT_EQ(
		explain(jopp2::lookup_error_code::unexpected_type),
		"Item exists but has a different type"
	);

	EXPECT_EQ(
		explain(jopp2::lookup_error_code::key_not_found),
		"Key not found"
	);

	EXPECT_EQ(
		explain(jopp2::lookup_error_code::value_not_an_object),
		"Value is not an object"
	);

	TestFwk::expect_death(
		[](){
			// NOLINTNEXTLINE
			explain(static_cast<jopp2::lookup_error_code>(234));
		},
		"jopp internal error: lib/./generic_value.hpp:57: Invalid lookup error code\n",
		SIGABRT
	);
}

TESTCASE(jopp2_lookup_result_from_error_code)
{
	jopp2::lookup_result<int> foo{jopp2::lookup_error_code::key_not_found};
	EXPECT_EQ(foo.error_code(), jopp2::lookup_error_code::key_not_found);
	TestFwk::expect_death([foo](){*foo = 123;},"" , SIGSEGV);
	try
	{
		foo.value("Foobar");
		EXPECT_EQ(false, true);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(
			err.what(),
			std::string_view{"Could not get `Foobar` from the current value: Key not found"}
		);
	}
}

TESTCASE(jopp2_lookup_result_from_pointer)
{
	std::pair res_value{235, 35};
	jopp2::lookup_result result{&res_value};
	EXPECT_EQ(*result, res_value);
	EXPECT_EQ(result->first, 235);
	EXPECT_EQ(result->second, 35);
	EXPECT_EQ(result.value("Foobar"), res_value);
	TestFwk::expect_death(
		[result](){
			auto const _ = result.error_code();
		},
		"jopp internal error: lib/./generic_value.hpp:93: Error code not set in a non-error condition\n",
		SIGABRT
	);
}

TESTCASE(jopp2_generic_value_static_properties)
{
	using type_with_pack = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_pack>;
	EXPECT_EQ(
		(std::is_same_v<
			type_with_pack::value_type,
			std::variant<
				int,
				double,
				std::string,
				char,
				type_with_pack::object,
				std::vector<int>,
				std::vector<double>,
				std::vector<std::string>,
				std::vector<char>,
				std::vector<type_with_pack::object>,
				std::vector<type_with_pack>
			>
		>),
		true
	);
	EXPECT_EQ(std::is_constructible_v<type_with_pack>, true);
	EXPECT_EQ(std::is_copy_constructible_v<type_with_pack>, false);
	EXPECT_EQ(std::is_copy_assignable_v<type_with_pack>, false);
	EXPECT_EQ(std::is_move_constructible_v<type_with_pack>, true);
	EXPECT_EQ(std::is_move_assignable_v<type_with_pack>, true);
	EXPECT_EQ((std::is_constructible_v<type_with_pack, int>), true);
	EXPECT_EQ((std::is_constructible_v<type_with_pack, type_with_pack::value_type>), true);

	using type_with_no_pack = jopp2::generic_value<std::flat_map, std::vector, my_value_traits_with_no_pack>;
		EXPECT_EQ(
		(std::is_same_v<
			type_with_no_pack::value_type,
			std::variant<
				std::string,
				type_with_no_pack::object,
				std::vector<std::string>,
				std::vector<type_with_no_pack::object>,
				std::vector<type_with_no_pack>
			>
		>),
		true
	);
	EXPECT_EQ(std::is_constructible_v<type_with_no_pack>, true);
	EXPECT_EQ(std::is_copy_constructible_v<type_with_no_pack>, false);
	EXPECT_EQ(std::is_copy_assignable_v<type_with_no_pack>, false);
	EXPECT_EQ(std::is_move_constructible_v<type_with_no_pack>, true);
	EXPECT_EQ(std::is_move_assignable_v<type_with_no_pack>, true);
	EXPECT_EQ((std::is_constructible_v<type_with_no_pack, std::string>), true);
	EXPECT_EQ((std::is_constructible_v<type_with_no_pack, type_with_pack::value_type>), false);
	EXPECT_EQ((std::is_constructible_v<type_with_no_pack, int>), false);
}

TESTCASE(jopp2_generic_value_default_constructed)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val;
	EXPECT_EQ(val.get<std::monostate>(), std::monostate{});
}

TESTCASE(jopp2_generic_value_construct_from_value)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{12.5};
	EXPECT_EQ(val.get<double>(), 12.5);
}

TESTCASE(jopp2_generic_value_get_wrong_type)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{12.5};

	{
		auto const stored_val_ptr = val.get_if<std::string>();
		EXPECT_EQ(stored_val_ptr, nullptr);
	}

	try
	{
		auto const _ = val.get<std::string>();
		EXPECT_EQ(true, false);
	}
	catch(jopp2::exception const& e)
	{
		EXPECT_EQ(e.what(), std::string_view{"Current value has an unexpected type"});
	}
}

TESTCASE(jopp2_generic_value_get_by_name_not_an_object)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{12.5};

	{
		auto const stored_val_ptr = val.get_if_by_name<double>("Foobar");
		EXPECT_EQ(stored_val_ptr, nullptr);
		EXPECT_EQ(stored_val_ptr.error_code(), jopp2::lookup_error_code::value_not_an_object);
	}

	try
	{
		auto const _ = val.get_by_name<double>("Foobar");
		EXPECT_EQ(true, false);
	}
	catch(jopp2::exception const& e)
	{
		EXPECT_EQ(
			e.what(),
			std::string_view{"Could not get `Foobar` from the current value: Value is not an object"}
		);
	}
}

TESTCASE(jopp2_generic_value_get_by_name_key_not_found)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{json_value::object{}};

	{
		auto const stored_val_ptr = val.get_if_by_name<double>("Foobar");
		EXPECT_EQ(stored_val_ptr, nullptr);
		EXPECT_EQ(stored_val_ptr.error_code(), jopp2::lookup_error_code::key_not_found);
	}

	try
	{
		auto const _ = val.get_by_name<double>("Foobar");
		EXPECT_EQ(true, false);
	}
	catch(jopp2::exception const& e)
	{
		EXPECT_EQ(
			e.what(),
			std::string_view{"Could not get `Foobar` from the current value: Key not found"}
		);
	}
}

TESTCASE(jopp2_generic_value_get_by_name_wrong_type)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{json_value::object{}};
	val.store_value_as(std::string{"This is a string"}, "Foobar");

	{
		auto const stored_val_ptr = val.get_if_by_name<double>("Foobar");
		EXPECT_EQ(stored_val_ptr, nullptr);
		EXPECT_EQ(stored_val_ptr.error_code(), jopp2::lookup_error_code::unexpected_type);
	}

	try
	{
		auto const _ = val.get_by_name<double>("Foobar");
		EXPECT_EQ(true, false);
	}
	catch(jopp2::exception const& e)
	{
		EXPECT_EQ(
			e.what(),
			std::string_view{"Could not get `Foobar` from the current value: Item exists but has a different type"}
		);
	}
}

TESTCASE(jopp2_generic_value_get_by_name_succesful)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{json_value::object{}};
	val.store_value_as(12.5, "Foobar");

	{
		auto const stored_val_ptr = val.get_if_by_name<double>("Foobar");
		EXPECT_EQ(*stored_val_ptr, 12.5);
	}

	EXPECT_EQ(val.get_by_name<double>("Foobar"), 12.5);
}

TESTCASE(jopp2_generic_value_try_store_value_as_value_is_not_an_object)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{};
	auto const res = val.try_store_value_as(12.5, "Foobar");
	EXPECT_EQ(res.value, nullptr);
	EXPECT_EQ(res.key, nullptr);
	EXPECT_EQ(res.was_inserted, false);
}

TESTCASE(jopp2_generic_value_try_store_value_value_is_an_object)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{json_value::object{}};

	{
		auto const res = val.try_store_value_as(12.5, "Foobar");
		EXPECT_EQ(*res.value, 12.5);
		EXPECT_EQ(*res.key, "Foobar");
		EXPECT_EQ(res.was_inserted, true);
	}

	{
		auto const res = val.try_store_value_as(25.0, "Foobar");
		EXPECT_EQ(*res.value, 12.5);
		EXPECT_EQ(*res.key, "Foobar");
		EXPECT_EQ(res.was_inserted, false);
	}
}

TESTCASE(jopp2_generic_value_store_value_as_value_is_not_an_object)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{};
	try
	{
		auto const _ = val.store_value_as(12.5, std::string{"This is a longer key"});
		EXPECT_EQ(false, true);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Failed to insert `This is a longer key` into a non-object"});
	}
}

TESTCASE(jopp2_generic_value_store_value_as_value_is_an_object)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{json_value::object{}};
	{
		auto const res = val.store_value_as(12.5, std::string{"This is a longer key"});
		EXPECT_EQ(res.first, "This is a longer key");
		EXPECT_EQ(res.second, 12.5);
	}

	try
	{
		auto const _ = val.store_value_as(50.0, std::string{"This is a longer key"});
		EXPECT_EQ(false, true);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"`This is a longer key` has already been set"});
	}
}

TESTCASE(jopp2_generic_value_try_store_key_value_value_is_not_an_object)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{};
	auto const res = val.try_store_key_value(json_value::map_value_type{"Foobar", 12.5});
	EXPECT_EQ(res.was_inserted, false);
	EXPECT_EQ(res.key, nullptr);
	EXPECT_EQ(res.value, nullptr);
}

TESTCASE(jopp2_generic_value_try_store_key_value_value_is_an_object)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{json_value::object{}};
	{
		auto const res = val.try_store_key_value(json_value::map_value_type{"Foobar", 12.5});
		EXPECT_EQ(res.value->get<double>(), 12.5);
		EXPECT_EQ(*res.key, "Foobar");
		EXPECT_EQ(res.was_inserted, true);
	}

	{
		auto const res = val.try_store_key_value(json_value::map_value_type{"Foobar", 25.0});
		EXPECT_EQ(res.value->get<double>(), 12.5);
		EXPECT_EQ(*res.key, "Foobar");
		EXPECT_EQ(res.was_inserted, false);
	}
}

TESTCASE(jopp2_generic_value_store_key_value_value_is_not_an_object)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{};
	try
	{
		auto const _ = val.store_key_value(json_value::map_value_type{"This is a longer key", 12.5});
		EXPECT_EQ(false, true);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(
			err.what(),
			std::string_view{"Failed to insert `This is a longer key` into a non-object"}
		);
	}
}

TESTCASE(jopp2_generic_value_store_key_value_value_is_an_object)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{json_value::object{}};

	{
		auto const res = val.store_key_value(json_value::map_value_type{"This is a longer key", 12.5});
		EXPECT_EQ(res.first, "This is a longer key");
		EXPECT_EQ(res.second.get<double>(), 12.5);
	}

	try
	{
		auto const _ = val.store_key_value(json_value::map_value_type{"This is a longer key", 50.0});
		EXPECT_EQ(false, true);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(
			err.what(),
			std::string_view{"`This is a longer key` has already been set"}
		);
	}
}

TESTCASE(jopp2_generic_value_try_store_at_end_not_a_sequence)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{json_value::object{}};
	auto const res = val.try_store_at_end(134.0);
	EXPECT_EQ(res, nullptr);
}

TESTCASE(jopp2_generic_value_try_store_at_end_value_is_a_string)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{std::string{"Hej hopp"}};
	auto const res = val.try_store_at_end(134.0);
	EXPECT_EQ(res, nullptr);
}

TESTCASE(jopp2_generic_value_try_store_at_end_typed_container_with_matching_type)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	{
		json_value val{std::vector{1.0, 2.0, 3.0}};
		auto const res = val.try_store_at_end(5.0);
		EXPECT_EQ(*res, 5.0);
		EXPECT_EQ(std::size(val.get<std::vector<double>>()), 4);
		EXPECT_EQ(val.get<std::vector<double>>().back(), 5.0);
	}

	{
		std::vector<json_value> vals{};
		vals.emplace_back(2.0);
		vals.emplace_back("Foobar");

		json_value val{std::move(vals)};
		auto const res = val.try_store_at_end(json_value{"Kaka"});
		EXPECT_EQ(res->get<std::string>(), "Kaka");
		EXPECT_EQ(std::size(val.get<std::vector<json_value>>()), 3);
		EXPECT_EQ(val.get<std::vector<json_value>>().back().get<std::string>(), "Kaka");
	}
}

TESTCASE(jopp2_generic_value_try_store_at_end_leaf_value_in_empty_generic_container)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{std::vector<json_value>{}};
	auto const res = val.try_store_at_end(5.0);
	EXPECT_EQ(*res, 5.0);
	EXPECT_EQ(std::size(val.get<std::vector<double>>()), 1);
	EXPECT_EQ(val.get<std::vector<double>>().back(), 5.0);
}

TESTCASE(jopp2_generic_value_try_store_at_end_generic_value_in_empty_leaf_container)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{std::vector<double>{}};
	auto const res = val.try_store_at_end(json_value{"Foo"});
	EXPECT_EQ(res->get<std::string>(), "Foo");
	EXPECT_EQ(std::size(val.get<std::vector<json_value>>()), 1);
	EXPECT_EQ(val.get<std::vector<json_value>>().back().get<std::string>(), "Foo");
}

TESTCASE(jopp2_generic_value_try_store_at_end_leaf_value_in_non_empty_generic_container)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{std::vector<json_value>{}};
	{
		auto const res = val.try_store_at_end(json_value{"Foo"});
		EXPECT_EQ(res->get<std::string>(), "Foo");
		EXPECT_EQ(std::size(val.get<std::vector<json_value>>()), 1);
		EXPECT_EQ(val.get<std::vector<json_value>>().back().get<std::string>(), "Foo");
	}

	{
		auto const res = val.try_store_at_end(2.0);
		EXPECT_EQ(*res, 2.0);
		EXPECT_EQ(std::size(val.get<std::vector<json_value>>()), 2);
		EXPECT_EQ(val.get<std::vector<json_value>>().back().get<double>(), 2.0);
	}
}

TESTCASE(jopp2_generic_value_try_store_at_end_leaf_value_in_non_empty_container_of_different_type)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{std::vector{1.0, 2.0, 4.0}};
	auto const res = val.try_store_at_end(std::string{"This is a string"});
	EXPECT_EQ(*res, "This is a string");
	EXPECT_EQ((val.get<std::vector<json_value>>().end() -2)->get<double>(), 4.0);
	EXPECT_EQ(val.get<std::vector<json_value>>().back().get<std::string>(), "This is a string");
	EXPECT_EQ(std::size(val.get<std::vector<json_value>>()), 4);
}

TESTCASE(jopp2_generic_value_try_store_at_end_generic_value_in_non_empty_container_of_different_type)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{std::vector{1.0, 2.0, 4.0}};
	auto const res = val.try_store_at_end(json_value{std::string{"This is a string"}});
	EXPECT_EQ(res->get<std::string>(), "This is a string");
	EXPECT_EQ((val.get<std::vector<json_value>>().end() -2)->get<double>(), 4.0);
	EXPECT_EQ(val.get<std::vector<json_value>>().back().get<std::string>(), "This is a string");
	EXPECT_EQ(std::size(val.get<std::vector<json_value>>()), 4);
}

TESTCASE(jopp2_generic_value_store_at_end_value_is_not_a_sequence)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{std::string{"foo"}};
	try
	{
		auto const& _ = val.store_at_end(std::string{"bar"});
		EXPECT_EQ(false, true);
	}
	catch(std::exception const& err)
	{
		EXPECT_EQ(err.what(), std::string_view{"Cannot append `bar` to a non-array"});
	}
}

TESTCASE(jopp2_generic_value_store_at_end_value_is_a_sequence)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;
	json_value val{std::vector<std::string>{"foo"}};
	auto const& res = val.store_at_end(std::string{"bar"});
	EXPECT_EQ(res, "bar");
}

TESTCASE(jopp2_generic_value_visit_nodes)
{
	using json_value = jopp2::generic_value<std::unordered_map, std::vector, json_value_traits>;

	static_assert(json_value::is_leaf_value<double>);
	static_assert(!json_value::is_leaf_value<int>);

	json_value value{json_value::object{}};
	{
		auto const res = value.store_value_as(std::vector<json_value>{}, "array_of_arrays_of_bools");
		res.second.emplace_back(
			json_value{std::vector{bool_wrapper::enabled, bool_wrapper::disabled}}
		);

		res.second.emplace_back(
			std::vector<bool_wrapper>{
				bool_wrapper::disabled,
				bool_wrapper::enabled,
				bool_wrapper::enabled
			}
		);
	}

	{
		auto const res = value.store_value_as(std::vector<json_value>{}, "array_of_arrays_of_nulls");
		res.second.emplace_back(
			json_value{std::vector{std::monostate{}, std::monostate{}}}
		);

		res.second.emplace_back(
			json_value{std::vector{std::monostate{}}}
		);
	}

	{
		auto const res = value.store_value_as(std::vector<json_value>{}, "array_of_arrays_of_numbers");
		res.second.emplace_back(
			json_value{std::vector{1.0, 2.0, 3.0}}
		);

		res.second.emplace_back(
			json_value{std::vector{4.5, 5.5}}
		);
	}

	{
		auto const res = value.store_value_as(
			std::vector<json_value>{}, "array_of_arrays_of_objects"
		);

		{
			std::vector<json_value::object> to_append;

			{
				json_value::object obj;
				obj.emplace("id", json_value{1.0});
				obj.emplace("status", "active");
				to_append.emplace_back(std::move(obj));
			}

			{
				json_value::object obj;
				obj.emplace("id", json_value{2.0});
				obj.emplace("status", "pending");
				to_append.emplace_back(std::move(obj));
			}

			res.second.emplace_back(std::move(to_append));
		}

		{
			std::vector<json_value::object> to_append;

			{
				json_value::object obj;
				obj.emplace("id", json_value{3.0});
				obj.emplace("status", "completed");
				to_append.emplace_back(std::move(obj));
			}

			res.second.emplace_back(std::move(to_append));
		}
	}

	{
		auto const res = value.store_value_as(std::vector<json_value>{}, "array_of_arrays_of_strings");
		res.second.emplace_back(
			json_value{std::vector<std::string>{"apple", "banana"}}
		);

		res.second.emplace_back(
			json_value{std::vector<std::string>{"cherry" ,"date"}}
		);
	}

	value.store_value_as(
		std::vector{
			bool_wrapper::enabled,
			bool_wrapper::disabled,
			bool_wrapper::enabled
		},
		"array_of_bools"
	);

	{
		auto const res = value.store_value_as(
			std::vector<json_value>{}, "array_of_heterogenous_arrays"
		);
		{
			std::vector<json_value> to_append;
			to_append.emplace_back(1.0);
			to_append.emplace_back("two");
			to_append.emplace_back(bool_wrapper::enabled);
			to_append.emplace_back(json_value{});
			res.second.emplace_back(std::move(to_append));
		}

		{
			std::vector<json_value> to_append;
			to_append.emplace_back(bool_wrapper::disabled);
			to_append.emplace_back(3.14);
			{
				json_value::object obj;
				obj.emplace("key", "value");
				to_append.emplace_back(std::move(obj));
			}

			res.second.emplace_back(std::move(to_append));
		}
	}

	value.store_value_as(
		std::vector{
			std::monostate{},
			std::monostate{},
			std::monostate{}
		},
		"array_of_nulls"
	);

	{
		auto const res = value.store_value_as(std::vector<json_value::object>{}, "array_of_objects");
		{
			json_value::object to_append;
			to_append.emplace("item", "A");
			to_append.emplace("value", 100.0);
			res.second.emplace_back(std::move(to_append));
		}

		{
			json_value::object to_append;
			to_append.emplace("item", "B");
			to_append.emplace("value", 200.0);
			res.second.emplace_back(std::move(to_append));
		}
	}

	{
		value.store_value_as(
			std::vector<std::string>{
				"test 1",
				"test 2",
				"test 3"
			},
			"array_of_strings"
		);
	}

	value.store_value_as(bool_wrapper::enabled, "bool");

	{
		auto const res = value.store_value_as(std::vector<json_value>{}, "heterogenous_arrays");
		res.second.emplace_back("text");
		res.second.emplace_back(123.0);
		res.second.emplace_back(bool_wrapper::disabled);
		res.second.emplace_back(std::monostate{});

		{
			json_value::object to_append;
			to_append.emplace("nested", "object");
			res.second.emplace_back(std::move(to_append));
		}
	}

	value.store_value_as(42.0, "number");

	{
		auto const res = value.store_value_as(json_value::object{}, "object");
		res.second.emplace("sample_key", "sample_value");
		res.second.emplace("is_dummy", bool_wrapper::enabled);
	}

	value.store_value_as(std::string{"lorem ipsum"}, "string_value");

	using json_value_sorted = jopp2::generic_value<std::flat_map, std::vector, json_value_traits>;

	auto result = clone<json_value_sorted>(value);
	std::string output;
	visit_nodes(result, test_node_visitor{output});
	static constexpr auto expected_output = R"((1 of 1) {
    array_of_arrays_of_bools: (1 of 15) [
        (1 of 2) bool[
            (1 of 2) true,
            (2 of 2) false
        ],
        (2 of 2) bool[
            (1 of 3) false,
            (2 of 3) true,
            (3 of 3) true
        ]
    ],
    array_of_arrays_of_nulls: (2 of 15) [
        (1 of 2) null[
            (1 of 2) null,
            (2 of 2) null
        ],
        (2 of 2) null[
            (1 of 1) null
        ]
    ],
    array_of_arrays_of_numbers: (3 of 15) [
        (1 of 2) fd[
            (1 of 3) 1,
            (2 of 3) 2,
            (3 of 3) 3
        ],
        (2 of 2) fd[
            (1 of 2) 4.5,
            (2 of 2) 5.5
        ]
    ],
    array_of_arrays_of_objects: (4 of 15) [
        (1 of 2) obj[
            (1 of 2) {
                id: (1 of 2) sd1,
                status: (2 of 2) str(active)
            },
            (2 of 2) {
                id: (1 of 2) sd2,
                status: (2 of 2) str(pending)
            }
        ],
        (2 of 2) obj[
            (1 of 1) {
                id: (1 of 2) sd3,
                status: (2 of 2) str(completed)
            }
        ]
    ],
    array_of_arrays_of_strings: (5 of 15) [
        (1 of 2) str[
            (1 of 2) apple,
            (2 of 2) banana
        ],
        (2 of 2) str[
            (1 of 2) cherry,
            (2 of 2) date
        ]
    ],
    array_of_bools: (6 of 15) bool[
        (1 of 3) true,
        (2 of 3) false,
        (3 of 3) true
    ],
    array_of_heterogenous_arrays: (7 of 15) [
        (1 of 2) [
            (1 of 4) sd1,
            (2 of 4) str(two),
            (3 of 4) true,
            (4 of 4) null
        ],
        (2 of 2) [
            (1 of 3) false,
            (2 of 3) sd3.14,
            (3 of 3) {
                key: (1 of 1) str(value)
            }
        ]
    ],
    array_of_nulls: (8 of 15) null[
        (1 of 3) null,
        (2 of 3) null,
        (3 of 3) null
    ],
    array_of_objects: (9 of 15) obj[
        (1 of 2) {
            item: (1 of 2) str(A),
            value: (2 of 2) sd100
        },
        (2 of 2) {
            item: (1 of 2) str(B),
            value: (2 of 2) sd200
        }
    ],
    array_of_strings: (10 of 15) str[
        (1 of 3) test 1,
        (2 of 3) test 2,
        (3 of 3) test 3
    ],
    bool: (11 of 15) true,
    heterogenous_arrays: (12 of 15) [
        (1 of 5) str(text),
        (2 of 5) sd123,
        (3 of 5) false,
        (4 of 5) null,
        (5 of 5) {
            nested: (1 of 1) str(object)
        }
    ],
    number: (13 of 15) sd42,
    object: (14 of 15) {
        is_dummy: (1 of 2) true,
        sample_key: (2 of 2) str(sample_value)
    },
    string_value: (15 of 15) str(lorem ipsum)
}
)";

	EXPECT_EQ(output, expected_output);
}
