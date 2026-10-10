//@	{"target":{"name":"value_sink_ref.test"}}

#include "./value_sink_ref.hpp"

#include <testfwk/testfwk.hpp>
#include <testfwk/mock_util.hpp>

namespace
{
	template<class T>
	struct source_type
	{
		using type = T;
	};

	template<class T>
	requires requires{typename T::value_type;}
	struct source_type<T>
	{
		using type = T::value_type;
	};

	struct test_sink_traits
	{
		template<class T>
		using source_value_type = source_type<T>::type;

		using supported_sink_types = jopp2::template_param_pack<
			int,
			std::unique_ptr<int>,
			std::vector<int>
		>;

		inline static TestFwk::mock_entry_overload_set<
			int&(int&, int),
			std::unique_ptr<int>&(std::unique_ptr<int>&, std::unique_ptr<int>),
			int&(std::vector<int>&, int)
		> store_value;

		template<class SinkType>
		inline static TestFwk::mock_entry_overload_set<
			void(jopp2::source_value_type_tag<int>, jopp2::sink_type_tag<SinkType>),
			void(jopp2::source_value_type_tag<std::unique_ptr<int>>, jopp2::sink_type_tag<SinkType>)
		> value_sink_type_mismatch_impl;

		template<class SrcTypeTag, class SinkTypeTag>
		static void value_sink_type_mismatch(SrcTypeTag /*unused*/, SinkTypeTag /*unused*/)
		{
			value_sink_type_mismatch_impl<typename SinkTypeTag::type>(
				SrcTypeTag{},
				SinkTypeTag{}
			);
		}
	};
}

TESTCASE(jopp2_value_sink_ref_store_value_unbound_object)
{
	jopp2::value_sink_ref<test_sink_traits> sink{};
	test_sink_traits::value_sink_type_mismatch_impl<
		jopp2::value_sink_ref_unset_tag
	>.expect_call_with_action(
		[](jopp2::source_value_type_tag<int>, jopp2::sink_type_tag<jopp2::value_sink_ref_unset_tag>){
			throw std::runtime_error{"Foo"};
		}
	);
	try
	{
		sink.store_value(0);
		abort();
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Foo"}); };
}

TESTCASE(jopp2_value_sink_ref_store_value)
{
	int value = 0;
	jopp2::value_sink_ref<test_sink_traits> sink{value};
	test_sink_traits::store_value.expect_call_with_action(
		[&value](int& sink, int val) -> int& {
			EXPECT_EQ(val, 34);
			EXPECT_EQ(&value, &sink);
			sink = val;
			return sink;
		}
	);
	auto& result = sink.store_value(34);
	EXPECT_EQ(&result, &value);
	EXPECT_EQ(value, 34);
}

TESTCASE(jopp2_value_sink_ref_store_value_wrong_type)
{
	int value = 0;
	jopp2::value_sink_ref<test_sink_traits> sink{value};
	test_sink_traits::value_sink_type_mismatch_impl<int>.expect_call_with_action(
		[](
			jopp2::source_value_type_tag<std::unique_ptr<int>>,
			jopp2::sink_type_tag<int>
		){
			throw std::runtime_error{"Foo"};
		}
	);
	try
	{
		sink.store_value(std::make_unique<int>(352));
		abort();
	}
	catch(std::exception const& err)
	{ EXPECT_EQ(err.what(), std::string_view{"Foo"}); };
}

TESTCASE(jopp2_value_sink_ref_store_move_only_type)
{
	std::unique_ptr<int> value;
	jopp2::value_sink_ref<test_sink_traits> sink{value};
	test_sink_traits::store_value.expect_call_with_action(
		[&value](std::unique_ptr<int>& sink, std::unique_ptr<int> val) -> std::unique_ptr<int>& {
			EXPECT_EQ(*val, 34);
			EXPECT_EQ(&value, &sink);
			sink = std::move(val);
			return sink;
		}
	);
	auto& result = sink.store_value(std::make_unique<int>(34));
	EXPECT_EQ(&result, &value);
	EXPECT_EQ(*value, 34);
}

TESTCASE(jopp2_value_sink_ref_store_value_to_sequence)
{
	std::vector<int> value;
	jopp2::value_sink_ref<test_sink_traits> sink{value};
	test_sink_traits::store_value.expect_call_with_action(
		[&value](std::vector<int>& sink, int val) -> int& {
			EXPECT_EQ(val, 34);
			EXPECT_EQ(&value, &sink);
			sink.push_back(val);
			return sink.back();
		}
	);
	auto& result = sink.store_value(34);
	EXPECT_EQ(&result, &value.back());
	EXPECT_EQ(value.back(), 34);
}
