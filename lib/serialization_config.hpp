#ifndef JOPP_SERIALIZATION_CONFIG_HPP
#define JOPP_SERIALIZATION_CONFIG_HPP

#include <variant>
#include <type_traits>
#include <array>

namespace jopp2::serialization_config
{
	template<class T, class A, class B>
	concept one_of = std::is_same_v<T, A> || std::is_same_v<T, B>;

	struct not_configured
	{
		constexpr bool operator==(not_configured const&) const = default;

		template<class T>
		requires(!std::is_same_v<std::remove_cvref_t<T>, not_configured>)
		constexpr bool operator==(T const&)
		{ return false;}
	};

	template<class T>
	requires(!std::is_same_v<std::remove_cvref_t<T>, not_configured>)
	constexpr bool operator==(T const&, not_configured)
	{ return false; }

	using trait_marker_field = std::variant<char8_t, not_configured>;

	template<class T>
	concept serialization_traits = requires {
		// Markers for value ranges
		{T::key_begin_marker} -> one_of<char8_t, not_configured>;
		{T::key_escape_sequence_begin_marker} -> std::same_as<char8_t>;
		{T::key_escape_sequence_end_marker} -> one_of<char8_t, not_configured>;
		{T::key_end_marker} -> one_of<char8_t, not_configured>;
		{T::string_begin_marker} -> one_of<char8_t, not_configured>;
		{T::string_escape_sequence_begin} -> std::same_as<char8_t>;
		{T::string_escape_sequence_end} -> one_of<char8_t, not_configured>;
		{T::string_end_marker} -> one_of<char8_t, not_configured>;
		{T::number_begin_marker} -> one_of<char8_t, not_configured>;
		{T::number_end_marker} -> one_of<char8_t, not_configured>;
		{T::special_value_begin_marker} -> one_of<char8_t, not_configured>;
		{T::special_value_end_marker} -> one_of<char8_t, not_configured>;
		{T::associative_container_begin_marker} -> one_of<char8_t, not_configured>;
		{T::associative_container_end_marker} -> one_of<char8_t, not_configured>;
		{T::sequence_container_begin_marker} -> one_of<char8_t, not_configured>;
		{T::sequence_container_end_marker} -> one_of<char8_t, not_configured>;

		// Markers for separators between values
		{T::key_mapped_item_separator} -> one_of<char8_t, not_configured>;
		{T::associative_container_item_separator} -> one_of<char8_t, not_configured>;
		{T::sequence_container_item_separator} -> one_of<char8_t, not_configured>;

		// Stream control markers
		{T::discard_root_marker} -> one_of<char8_t, not_configured>;
		{T::flush_root_marker} -> one_of<char8_t, not_configured>;

		// Behavioural control
		{T::allow_multiline_strings} -> std::same_as<bool>;
		{T::allow_multiline_keys} -> std::same_as<bool>;
		{T::allow_inf} -> std::same_as<bool>;
		{T::allow_nan} -> std::same_as<bool>;
	} &&
	// Escape sequence begin markers cannot be the same as the corresponding end marker
	T::key_escape_sequence_begin_marker != T::key_end_marker &&
	T::string_escape_sequence_begin != T::string_end_marker &&

	// All begin value begin markers has to be different
	all_different(
		std::array<trait_marker_field>{
			T::string_begin_marker,
			T::number_begin_marker,
			T::special_value_begin_marker,
			T::associative_container_begin_marker,
			T::sequence_container_begin_marker
		}
	) &&

	// Containers are recursive and cannot use the same marker for begin and end
	T::associative_container_begin_marker != T::associative_container_end_marker &&
	T::sequence_container_begin_marker != T::sequence_container_end_marker &&

	// When parsing the content of an associative_container, a mapped item is followed by a key_name.
	// associative_container_item_separator must be different from item end markers
	(
		// associative_container_item_separator comes before key_begin_marker. If
		// associative_container_item_separator is not configured, key_begin_marker
		// follows directly
		T::associative_container_item_separator == not_configured{} &&
		not_included_in(
			T::key_begin_marker,
			std::array<trait_marker_field>{
				T::string_end_marker,
				T::number_end_marker,
				T::special_value_end_marker,
				T::associative_container_end_marker,
				T::sequence_container_end_marker
			}
		)
	) || (
		// If associative_container_item_separator has been set, it should be different from
		// the end markers
		T::associative_container_item_separator != not_configured{} &&
		not_included_in(
			T::associative_container_item_separator,
			std::array<trait_marker_field>{
				T::string_end_marker,
				T::number_end_marker,
				T::special_value_end_marker,
				T::associative_container_end_marker,
				T::sequence_container_end_marker
			}
		)
	) &&

	// Similar for sequence containers, but only includes has the item separator
	not_included_in(
		T::sequence_container_item_separator,
		std::array<std::variant<char8_t, not_configured>>{
			T::string_end_marker,
			T::number_end_marker,
			T::special_value_end_marker,
			T::associative_container_end_marker,
			T::sequence_container_end_marker
		}
	)

	template<serialization_traits T>
	constexpr bool requires_escape_of_string_begin_marker()
	{ return T::string_begin_marker == T::string_end_marker; }

	template<serialization_traits T>
	constexpr bool requires_escape_of_key_begin_marker()
	{ return T::key_begin_marker == T::key_end_marker; }
}

#endif