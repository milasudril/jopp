#ifndef JOPP_GENERIC_VALUE_HPP
#define JOPP_GENERIC_VALUE_HPP

#include "./variant_utils.hpp"
#include "./utils.hpp"
#include "./template_param_pack.hpp"
#include "./clear_nodes_visitor.hpp"
#include "./exception.hpp"
#include "./sequence_container.hpp"
#include "./node_visitor_adaptor.hpp"
#include "./with_subtype_id.hpp"

#include <algorithm>
#include <type_traits>

namespace jopp2
{
	using jopp::overload;

	struct src_object{};

	struct src_value{};

	enum class lookup_error_code{value_not_an_object, key_not_found, unexpected_type};

	constexpr std::string_view explain(lookup_error_code ec)
	{
		switch(ec)
		{
			using enum lookup_error_code;
			case value_not_an_object:
				return "Value is not an object";
			case key_not_found:
				return "Key not found";
			case unexpected_type:
				return "Item exists but has a different type";
		}
		raise_internal_error("Invalid lookup error code");
	}

	template<class RetType>
	class lookup_result
	{
	public:
		constexpr explicit lookup_result(lookup_error_code err_code):
			m_err_code{err_code}
		{}

		constexpr explicit lookup_result(RetType* value):
			m_value{value},
			m_err_code{}
		{}

		constexpr operator RetType*() const
		{ return m_value;}

		constexpr RetType* operator->() const
		{ return m_value; }

		template<class KeyLike>
		constexpr RetType& value(KeyLike const& key) const
		{
			if(m_value == nullptr)
			{
				throw exception{"Could not get `{}` from the current value: {}", key, explain(m_err_code) };
			}
			return *m_value;
		}

		constexpr auto error_code() const
		{
			if(m_value != nullptr)
			{ raise_internal_error("Error code not set in a non-error condition"); }
			return m_err_code;
		}

	private:
		RetType* m_value{};
		lookup_error_code m_err_code;
	};

	template<
		template<class KeyType, class MappedType, class...> class AssociativeContainerType,
		template<class ValueType, class...> class SequenceContainerType,
		class ValueTraits
	>
	class generic_value
	{
	public:
		using leaf_value_template_param_pack = wrap_in_template_param_pack_t<
			typename ValueTraits::leaf_value_type
		>;

		using leaf_value_type = map_template_param_pack_to_type_t<
			std::variant,
			leaf_value_template_param_pack
		>;
		using key_type = ValueTraits::key_type;

		// TODO: Subtype id type should be fetched from ValueTraits
		using object = with_subtype_id<std::string, AssociativeContainerType<key_type, generic_value>>;
		using map_value_type = object::value_type;
		template<class T>
		using sequence_container_type = SequenceContainerType<T>;
		static_assert(sequence_container<sequence_container_type<leaf_value_type>>);

		template<class T>
		static constexpr auto is_leaf_value = requires(T&& x){
			{ leaf_value_type{std::forward<T>(x)} };
		};

		using array_value_template_param_pack = concatenate_template_param_packs_t<
			wrap_template_param_pack_elements_t<
				leaf_value_template_param_pack, sequence_container_type
			>,
			template_param_pack<sequence_container_type<object>>,
			template_param_pack<sequence_container_type<generic_value>>
		>;

		using value_template_param_pack_type = concatenate_template_param_packs_t<
			leaf_value_template_param_pack,
			template_param_pack<object>,
			array_value_template_param_pack
		>;

		using value_type = map_template_param_pack_to_type_t<
			std::variant,
			value_template_param_pack_type
		>;

		using generic_sequence_container = SequenceContainerType<generic_value>;

		generic_value() = default;

		generic_value(generic_value const&) = delete;
		generic_value& operator=(generic_value const&) = delete;
		generic_value(value_type const&) = delete;
		generic_value& operator=(value_type const&) = delete;

		generic_value(generic_value&&) = default;

		generic_value& operator=(generic_value&& other) noexcept
		{
			clear();
			m_value = std::move(other.m_value);
			return *this;
		}

		~generic_value()
		{ clear(); }

		void clear()
		{
			node_visitor visitor{*this, clear_nodes_visitor{}};
			visitor.visit_nodes();
			m_value = value_type{};
		}

		template<class ... Args>
		requires(std::is_constructible_v<value_type, Args...>)
		explicit generic_value(Args&&... args):
			m_value{std::forward<Args>(args)...}
		{}

		template<class Self>
		auto&& get_value(this Self&& self)
		{ return std::forward_like<Self>(std::forward<Self>(self).m_value); }

		template<class T, class Self>
		auto get_if(this Self&& self)
		{ return std::get_if<std::remove_cvref_t<T>>(&std::forward<Self>(self).m_value); }

		template<class T, class Self>
		auto&& get(this Self&& self)
		{
			auto retval = std::forward<Self>(self).template get_if<T>();
			if(retval == nullptr)
			{ throw exception{"Current value has an unexpected type"}; }
			return std::forward_like<Self>(*retval);
		}

		template<class T, class Self, class KeyLike>
		auto get_if_by_name(this Self&& self, KeyLike const& key)
		{
			using ret_type = std::conditional_t<
				std::is_const_v<std::remove_reference_t<Self>>,
				lookup_result<std::remove_cvref_t<T> const>,
				lookup_result<std::remove_cvref_t<T>>
			>;
			auto item = std::forward<Self>(self).template get_if<object>();
			if(item == nullptr)
			{ return ret_type{lookup_error_code::value_not_an_object}; }

			auto const i = item->find(key);
			if(i == std::end(*item))
			{ return ret_type{lookup_error_code::key_not_found}; }

			auto const val_ptr = i->second.template get_if<T>();
			if(val_ptr == nullptr)
			{ return ret_type{lookup_error_code::unexpected_type}; }

			return ret_type{val_ptr};
		}

		template<class T, class Self, class KeyLike>
		auto&& get_by_name(this Self&& self, KeyLike const& key)
		{ return std::forward_like<Self>(std::forward<Self>(self).template get_if_by_name<T>(key).value(key)); }

		template<class Value>
		struct insert_result
		{
			key_type const* key = nullptr;
			Value* value = nullptr;
			bool was_inserted = false;
		};

		template<class TargetType, class SrcType>
		requires(std::is_same_v<std::remove_cvref_t<SrcType>, generic_value>)
		[[gnu::always_inline]] static auto get_value_pointer(SrcType* ptr)
		{
			if constexpr(std::is_same_v<std::remove_cvref_t<TargetType>, generic_value>)
			{ return ptr; }
			else
			{ return ptr->template get_if<TargetType>(); }
		}

		template<class Self, class T, class KeyLike>
		auto emplace(this Self& self, KeyLike&& key, T&& value)
		{
			using ret_type = insert_result<generic_value>;

			auto i = self.template get_if<object>();
			if(i == nullptr)
			{ return ret_type{}; }

			auto const insert_result = i->emplace(std::forward<KeyLike>(key), std::forward<T>(value));
			return ret_type{
				.key = &insert_result.first->first,
				.value = &insert_result.first->second,
				.was_inserted = insert_result.second
			};
		}

		template<class Self, class T, class KeyLike>
		auto try_store_value_as(this Self& self, T&& value, KeyLike&& key)
		{
			using ret_type = insert_result<std::remove_cvref_t<T>>;

			auto i = self.template get_if<object>();
			if(i == nullptr)
			{ return ret_type{}; }

			auto const insert_result = i->emplace(std::forward<KeyLike>(key), std::forward<T>(value));
			return ret_type{
				.key = &insert_result.first->first,
				.value = get_value_pointer<std::remove_cvref_t<T>>(&insert_result.first->second),
				.was_inserted = insert_result.second
			};
		}

		template<class Self, class T, class KeyLike>
		auto store_value_as(this Self& self, T&& value, KeyLike&& key)
		{
			auto res = self.try_store_value_as(std::forward<T>(value), std::forward<KeyLike>(key));
			if(res.key == nullptr)
			{ throw exception{"Failed to insert `{}` into a non-object", std::forward<KeyLike>(key)}; }

			if(!res.was_inserted)
			{ throw exception{"`{}` has already been set", *res.key}; }

			return std::pair<key_type const&, std::remove_cvref_t<T>&>{*res.key, *res.value};
		}

		template<class Self, class Item>
		requires(std::is_same_v<std::remove_cvref_t<Item>, map_value_type>)
		auto try_store_key_value(this Self& self, Item&& item)
		{
			using ret_type = insert_result<generic_value>;

			auto i = self.template get_if<object>();
			if(i == nullptr)
			{ return ret_type{}; }

			auto const insert_result = i->insert(std::forward<Item>(item));
			return ret_type{
				.key = &insert_result.first->first,
				.value = &insert_result.first->second,
				.was_inserted=insert_result.second
			};
		}

		template<class Self, class Item>
		requires(std::is_same_v<std::remove_cvref_t<Item>, map_value_type>)
		auto store_key_value(this Self& self, Item&& item)
		{
			auto const res = self.try_store_key_value(std::forward<Item>(item));
			if(res.key == nullptr)
			{ throw exception{"Failed to insert `{}` into a non-object", item.first}; }

			if(!res.was_inserted)
			{ throw exception{"`{}` has already been set", *res.key}; }

			return std::pair<key_type const&, generic_value&>{
				*res.key,
				*res.value
			};
		}

		template<class Self, class T>
		std::remove_cvref_t<T>* try_store_at_end(this Self& self, T&& value)
		{
			return visit_with_args(
				self.m_value,
				overload{
					[](SequenceContainerType<std::remove_cvref_t<T>>& seq, T&& value) -> std::remove_cvref_t<T>* {
						seq.emplace_back(std::move(value));
						return &seq.back();
					},
					[&self]<sequence_container Seq>(Seq& seq, T&& value) -> std::remove_cvref_t<T>* {
						if(seq.empty())
						{
							SequenceContainerType<std::remove_cvref_t<T>> new_container{};
							new_container.emplace_back(std::move(value));
							auto ret = &new_container.back();
							self.m_value = std::move(new_container);
							return ret;
						}

						if constexpr(std::is_same_v<typename std::remove_cvref_t<Seq>::value_type, generic_value>)
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

							auto& ret_ref = new_container.back();
							self.m_value = std::move(new_container);
							return get_value_pointer<T>(&ret_ref);
						}
					},
					[](auto const&...)  -> std::remove_cvref_t<T>* {
						return static_cast<std::remove_cvref_t<T>*>(nullptr);
					}
				},
				std::forward<T>(value)
			);
		}

		template<class Self, class T>
		std::remove_cvref_t<T>& store_at_end(this Self& self, T&& value)
		{
			auto ret = self.try_store_at_end(std::forward<T>(value));
			if(ret == nullptr)
			{ throw exception{"Cannot append `{}` to a non-array", std::forward<T>(value)}; }
			return *ret;
		}

	private:
		value_type m_value;
	};
}

#endif
