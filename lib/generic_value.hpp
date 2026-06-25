#ifndef JOPP_GENERIC_VALUE_HPP
#define JOPP_GENERIC_VALUE_HPP

#include "./variant_utils.hpp"

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
	private:
		build_variant_t<
			std::variant<
				AssociativeContainerType<typename ValueTraits::key_type, generic_value>,
				SequenceContainerType<generic_value>,
				SequenceContainerType<AssociativeContainerType<typename ValueTraits::key_type, generic_value>>
			>,
			build_variant_t<
				typename ValueTraits::leaf_value_type,
				wrap_variant_element_t<
					wrap_in_variant_t<typename ValueTraits::leaf_value_type>, SequenceContainerType
				>
			>
		>
		m_value;
	};
}



#endif