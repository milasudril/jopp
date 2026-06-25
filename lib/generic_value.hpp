#ifndef JOPP_GENERIC_VALUE_HPP
#define JOPP_GENERIC_VALUE_HPP

#include "./variant_utils.hpp"

#include <memory>

namespace jopp2
{
	template<
		template<class KeyType, class MappedType> class AssociativeContainerType,
		template<class ValueType> class SequenceContainerType,
		class ValueTraits
	>
	class generic_value
	{
	public:
		using leaf_value_type = typename ValueTraits::leaf_value_type;
		using key_type = typename ValueTraits::key_type;
		using object = AssociativeContainerType<key_type, generic_value>;
		using variant_type = concatenate_variants_t<
			wrap_in_variant_t<leaf_value_type>,
			wrap_in_variant_t<object>,
			wrap_variant_element_t<
				wrap_in_variant_t<leaf_value_type>, SequenceContainerType
			>,
			wrap_in_variant_t<SequenceContainerType<object>>,
			wrap_in_variant_t<SequenceContainerType<generic_value>>
		>;

		auto const& get() const
		{ return m_value; }

		static constexpr size_t first_sequence_type_index()
		{ return std::variant_size_v<wrap_in_variant_t<leaf_value_type>> + 1; }

	private:
		variant_type m_value;
	};
}

#endif