//@	{"target":{"name":"serialization_config.test"}}

#include "./serialization_config.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	struct json_tree_traits
	{
		static constexpr auto associative_container_item_separator = ',';
		static constexpr auto associative_container_end_marker = '}';
		static constexpr auto sequence_container_end_marker = ']';
		static constexpr auto sequence_container_item_separator = ',';
		static constexpr auto key_begin_marker = '"';
		static constexpr auto key_escape_sequence_begin_marker = '\\';
		static constexpr auto key_escape_sequence_end_marker = jopp2::serialization_config::not_configured{};
		static constexpr auto key_end_marker = '"';
		static constexpr auto associative_container_begin_marker = '{';
		static constexpr auto sequence_container_begin_marker = '[';
		static constexpr auto key_mapped_item_separator = ':';
		static constexpr auto discard_root_marker = jopp2::serialization_config::not_configured{};
		static constexpr auto flush_root_marker = jopp2::serialization_config::not_configured{};
	};

};

static_assert(jopp2::serialization_config::serialization_traits<json_tree_traits>);
