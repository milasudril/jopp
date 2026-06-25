#ifndef JOPP_PROPERTY_TREE_HPP
#define JOPP_PROPERTY_TREE_HPP

#include <variant>
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
		std::variant<
			std::unique_ptr<
				property_tree<
					AssociativeContainerType,
					SequenceContainerType,
					ValueTraits
				>
			>,
			SequenceContainerType<generic_value>
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