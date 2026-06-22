#ifndef JOPP_PROPERTY_TREE_HPP
#define JOPP_PROPERTY_TREE_HPP

#include <variant>
#include <type_traits>
#include <array>
#include <algorithm>

namespace jopp2::property_tree
{
	template<class T, class A>
	concept same_as_no_ref = std::is_same_v<
		std::remove_cvref_t<T>,
		std::remove_cvref_t<A>
	>;

	template<class T, class A, class B>
	concept one_of = same_as_no_ref<T, A> || same_as_no_ref<T, B>;

	struct not_configured :std::monostate
	{
		template<class T>
		requires(!same_as_no_ref<T, not_configured>)
		constexpr bool operator==(T const&) const
		{ return false; }
	};

	using trait_marker_field = std::variant<not_configured, char>;

	template<class T, size_t N>
	constexpr bool all_different(std::array<T, N> items)
	{
		std::sort(std::begin(items), std::end(items));
		auto const i = std::unique(std::begin(items), std::end(items));
		return i == std::end(items);
	}

	template<class T, size_t N>
	constexpr bool not_included_in(T const& value, std::array<T, N> const& values)
	{
		auto const i = std::ranges::find(values, value);
		return i == std::end(values);
	}

	template<class T>
	concept serialization_traits = requires {
		// Markers for value ranges
		{T::key_begin_marker} -> one_of<char, not_configured>;
		{T::key_escape_sequence_begin_marker} -> same_as_no_ref<char>;
		{T::key_escape_sequence_end_marker} -> one_of<char, not_configured>;
		{T::key_end_marker} -> one_of<char, not_configured>;
		{T::associative_container_begin_marker} -> one_of<char, not_configured>;
		{T::associative_container_end_marker} -> one_of<char, not_configured>;
		{T::sequence_container_begin_marker} -> one_of<char, not_configured>;
		{T::sequence_container_end_marker} -> one_of<char, not_configured>;

		// Markers for separators between values
		{T::key_mapped_item_separator} -> one_of<char, not_configured>;
		{T::associative_container_item_separator} -> one_of<char, not_configured>;
		{T::sequence_container_item_separator} -> one_of<char, not_configured>;

		// Stream control markers
		{T::discard_root_marker} -> one_of<char, not_configured>;
		{T::flush_root_marker} -> one_of<char, not_configured>;
	} &&
	// Escape sequence begin markers cannot be the same as the corresponding end marker
	T::key_escape_sequence_begin_marker != T::key_end_marker &&

	// All value begin markers has to be different
	T::associative_container_begin_marker != T::sequence_container_begin_marker &&

	// Containers are recursive and cannot use the same marker for begin and end
	T::associative_container_begin_marker != T::associative_container_end_marker &&
	T::sequence_container_begin_marker != T::sequence_container_end_marker &&

	// When parsing the content of an associative_container, a mapped item is followed by a key_name.
	// associative_container_item_separator must be different from item end markers
	(
		(
			// associative_container_item_separator comes before key_begin_marker. If
			// associative_container_item_separator is not configured, key_begin_marker
			// follows directly
			T::associative_container_item_separator == not_configured{} &&
			not_included_in(
				trait_marker_field{T::key_begin_marker},
				std::array<trait_marker_field, 5>{
					T::associative_container_end_marker,
					T::sequence_container_end_marker
				}
			)
		) || (
			// If associative_container_item_separator has been set, it should be different from
			// the end markers
			T::associative_container_item_separator != not_configured{} &&
			not_included_in(
				trait_marker_field{T::associative_container_item_separator},
				std::array<trait_marker_field, 5>{
					T::associative_container_end_marker,
					T::sequence_container_end_marker
				}
			)
		)
	)&&

	// Similar for sequence containers, but only includes has the item separator
	not_included_in(
		trait_marker_field{T::sequence_container_item_separator},
		std::array<trait_marker_field, 5>{
			T::associative_container_end_marker,
			T::sequence_container_end_marker
		}
	) &&

	// Ensure it is possible to identify end of containers
	T::associative_container_item_separator != T::associative_container_end_marker &&
	T::sequence_container_item_separator != T::sequence_container_end_marker;
}

#endif