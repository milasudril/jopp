#ifndef JOPP_GENERIC_VALUE_HPP
#define JOPP_GENERIC_VALUE_HPP

#include "./variant_utils.hpp"
#include "./utils.hpp"

#include <optional>
#include <stdexcept>

namespace jopp2
{
	using jopp::overload;

	template<class T>
	concept sequence_container = requires(T& obj){
		{obj.back()};
		{obj.push_back(std::declval<typename T::value_type>())};
		{obj.emplace_back(std::declval<typename T::value_type>())};
		{obj.empty()} -> std::same_as<bool>;
	};

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
		using map_value_type = object::value_type;
		static_assert(sequence_container<SequenceContainerType<leaf_value_type>>);

		using variant_type = concatenate_variants_t<
			wrap_in_variant_t<leaf_value_type>,
			wrap_in_variant_t<object>,
			wrap_variant_element_t<
				wrap_in_variant_t<leaf_value_type>, SequenceContainerType
			>,
			wrap_in_variant_t<SequenceContainerType<object>>,
			wrap_in_variant_t<SequenceContainerType<generic_value>>
		>;

		static constexpr size_t first_sequence_type_index()
		{ return std::variant_size_v<wrap_in_variant_t<leaf_value_type>> + 1; }

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
		{ return std::get_if<std::remove_cvref_t<T>>(&self.m_value); }

		template<class T, class Self>
		auto&& get(this Self&& self)
		{
			auto retval = self.template get_if<T>();
			if(retval == nullptr)
			{ throw std::runtime_error{"Item has an unexpected type"}; }
			return std::forward_like<Self>(*retval);
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

			return i->second.template get_if<T>();
		}

		template<class T, class Self, class KeyLike>
		auto&& get_by_name(this Self&& self, KeyLike&& key)
		{
			auto retval = self.template get_if_by_name<T>(std::forward<KeyLike>(key));
			if(retval == nullptr)
			{ throw std::runtime_error{"Item has an unexpected type or does not exist"}; }
			return std::forward_like<Self>(*retval);
		}

		template<class Self, class T, class KeyLike>
		auto try_store_value_as(this Self& self, T&& value, KeyLike&& key)
		{
			auto i = self.template get_if<object>();
			using objref = object::reference;
			using ret_type = std::conditional_t<
				std::is_same_v<objref, std::pair<key_type const&, generic_value&>>,
				std::optional<objref>,
				std::pair<key_type const, generic_value>*
			>;

			if(i == nullptr)
			{ return ret_type{}; }

			auto const insert_result = i->emplace(std::forward<KeyLike>(key), std::forward<T>(value));

			if constexpr(std::is_same_v<objref, std::pair<key_type const&, generic_value&>>)
			{
				if(!insert_result.second)
				{ return ret_type{}; }
				return ret_type{*insert_result.first};
			}
			else
			{ return &*insert_result.first; }
		}

		template<class Self, class T, class KeyLike>
		std::conditional_t<
			std::is_same_v<typename object::reference, std::pair<key_type const&, generic_value&>>,
			typename object::reference,
			std::pair<key_type const, generic_value>&
		> store_value_as(this Self& self, T&& value, KeyLike&& key)
		{
			auto res = self.try_store_value_as(std::forward<T>(value), std::forward<KeyLike>(key));
			if(!res)
			{
				throw std::runtime_error{
					"This generic value is not an object, or the property has already been set"
				};
			}
			return *res;
		}

		template<class Self, class T>
		T* try_store_at_end(this Self& self, T&& value)
		{
			auto const is_sequence_container = std::visit(
				overload{
					[]<sequence_container Seq>(Seq const&) static {
						return true;
					},
					[](auto const&) static {
						return false;
					}
				},
				self.m_value
			);
			if(!is_sequence_container)
			{ return nullptr; }

			return visit_with_args(
				self.m_value,
				overload{
					[](SequenceContainerType<std::remove_cvref_t<T>>& seq, T&& value) {
						seq.emplace_back(std::forward<T>(value));
						return &seq.back();
					},
					[&self]<sequence_container Seq>(Seq& seq, T&& value) {
						if(seq.empty())
						{
							SequenceContainerType<std::remove_cvref_t<T>> new_container{};
							new_container.emplace_back(std::forward<T>(value));
							auto ret = &new_container.back();
							self.m_value = std::move(new_container);
							return ret;
						}

						if constexpr(std::is_same_v<typename Seq::value_type, generic_value>)
						{
							seq.emplace_back(std::forward<T>(value));
							return seq.back().template get_if<T>();
						}
						else
						{
							SequenceContainerType<generic_value> new_container;

							if constexpr(
								requires{{new_container.reserve(size_t{})};} &&
								requires{{std::size(seq)};}
							)
							{ new_container.reserve(std::size(seq) + 1); }

							for(auto& item : seq)
							{ new_container.emplace_back(std::move(item)); }

							new_container.emplace_back(std::forward<T>(value));

							auto ret = new_container.back().template get_if<T>();
							self.m_value = std::move(new_container);
							return ret;
						}
					},
					[](auto const&...) {
						return static_cast<T*>(nullptr);
					}
				},
				std::forward<T>(value)
			);
		}

	private:
		variant_type m_value;
	};
}

#endif