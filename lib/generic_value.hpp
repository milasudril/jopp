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

		generic_value() = default;

		generic_value(generic_value const&) = delete;
		generic_value& operator=(generic_value const&) = delete;
		generic_value(variant_type const&) = delete;
		generic_value& operator=(variant_type const&) = delete;

		generic_value(generic_value&&) = default;
		generic_value& operator=(generic_value&&) = default;

		template<class ... Args>
		explicit generic_value(Args&&... args):
			m_value{std::forward<Args>(args)...}
		{}

		auto const& get() const
		{ return m_value; }

		template<class T, class Self>
		auto get_if(this Self&& self)
		{ return std::get_if<std::remove_cvref_t<T>>(&std::forward<Self>(self).m_value); }

		template<class T, class Self>
		auto&& get(this Self&& self)
		{
			auto retval = std::forward<Self>(self).template get_if<T>();
			if(retval == nullptr)
			{ throw std::runtime_error{"Item has an unexpected type"}; }
			return std::forward<T>(*retval);
		}

		template<class T, class Self, class KeyLike>
		std::conditional_t<
			std::is_const_v<std::remove_reference_t<Self>>,
			std::remove_cvref_t<T> const*,
			std::remove_cvref_t<T>*
		> get_if_by_name(this Self&& self, KeyLike&& key)
		{
			auto item = self.template get_if<object>();
			if(item == nullptr)
			{ return nullptr; }

			auto const i = item->find(std::forward<KeyLike>(key));
			if(i == std::end(*item))
			{ return nullptr; }

			return i->second.template get_if<std::remove_cvref_t<T>>();
		}

		template<class T, class Self, class KeyLike>
		auto&& get_by_name(this Self&& self, KeyLike&& key)
		{
			auto retval = std::forward<Self>(self).template get_if_by_name<T>(std::forward<KeyLike>(key));
			if(retval == nullptr)
			{ throw std::runtime_error{"Item has an unexpected type or does not exist"}; }
			return std::forward<
				std::conditional_t<
					std::is_const_v<std::remove_reference_t<Self>>,
					T const,
					T
				>
			>(*retval);
		}

		static constexpr size_t first_sequence_type_index()
		{ return std::variant_size_v<wrap_in_variant_t<leaf_value_type>> + 1; }

	private:
		variant_type m_value;
	};
}

#endif