#ifndef JOPP_PROPERTY_TREE_HPP
#define JOPP_PROPERTY_TREE_HPP


#include "./variant_utils.hpp"

#include <memory>

namespace jopp2
{
	template<
		template<class KeyType, class MappedType> class AssociativeContainerType,
		template<class ValueType> class SequenceContainerType,
		class ValueTraits
	>
	class property_tree;

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
				std::unique_ptr<
					property_tree<AssociativeContainerType, SequenceContainerType, ValueTraits>
				>,
				SequenceContainerType<generic_value>,
				SequenceContainerType<property_tree<AssociativeContainerType, SequenceContainerType, ValueTraits>>
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

	template<
		template<class KeyType, class MappedType> class AssociativeContainerType,
		template<class ValueType> class SequenceContainerType,
		class ValueTraits
	>
	class property_tree
	{
	public:
		using key_type = typename ValueTraits::key_type;
		using mapped_type = generic_value<AssociativeContainerType, SequenceContainerType, ValueTraits>;
		using container_type = AssociativeContainerType<key_type, mapped_type>;
		using value_type = typename container_type::value_type;

	private:
		container_type m_container;
	};
}



#endif