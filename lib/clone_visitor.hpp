#ifndef JOPP2_CLONE_VISITOR_HPP
#define JOPP2_CLONE_VISITOR_HPP

#include "./node_visitor_adaptor.hpp"
#include "./value_storage.hpp"
#include "./template_param_pack.hpp"
#include "lib/exception.hpp"
#include <ranges>

namespace jopp2
{
	template<class GenericValueOut, class Other>
	struct clone_visitor_update_result
	{ using type = GenericValueOut*; };

	template<class T>
	struct key_to_clone
	{
		using captured_type = T;
		T value;
	};

	template<class T>
	requires std::ranges::range<T>
	struct key_to_clone<T>
	{
		using captured_type = T;
		container_proxy<T const>::active_range_type value;
	};

	template<class GenericValueIn, class GenericValueOut>
	class clone_visitor_2
	{
	public:
		using src_value_param_pack = GenericValueIn::leaf_value_template_param_pack;
		using src_kv_item = GenericValueIn::object::value_type;
		using dest_kv_item = GenericValueOut::object::value_type;
		using objcontainer = std::conditional_t<
			std::is_const_v<GenericValueIn>,
			typename GenericValueOut::object const,
			typename GenericValueOut::object
		>;

		template<class T>
		using container_proxy_range = container_proxy<
			typename GenericValueIn::template sequence_container_type<T> const
		>::active_range_type;

		using complete_pack= concatenate_template_param_packs_t<
			src_value_param_pack,
			wrap_template_param_pack_elements_t<
				make_template_param_pack_t<typename GenericValueIn::object::key_type>,
				key_to_clone
			>,
			wrap_template_param_pack_elements_t<
				src_value_param_pack,
				container_proxy_range
			>
		>;

		template<class T>
		using sequence_container_out = GenericValueOut::template sequence_container_type<T>;

		template<class T>
		using update_result_t = clone_visitor_update_result<GenericValueOut, T>::type;

		template<class ... Args>
		using value_storage_with_result = value_storage<update_result_t, Args...>;

		using value_storage_out = map_template_param_pack_to_type_t<
			value_storage_with_result,
			complete_pack
		>;

		struct generic_value_update_traits
		{
			template<class Rhs>
			requires(std::is_constructible_v<GenericValueOut, Rhs> && !instance_of<Rhs, key_to_clone>)
			[[gnu::always_inline]] static auto update(GenericValueOut& lhs, Rhs&& rhs)
			{
				lhs = GenericValueOut(std::forward<Rhs>(rhs));
				return &lhs;
			}

			template<class Rhs>
			requires instance_of<std::remove_cvref_t<Rhs>, key_to_clone>
			&& (!std::ranges::range<typename std::remove_cvref_t<Rhs>::captured_type>)
			[[gnu::always_inline]] static auto update(GenericValueOut& lhs, Rhs&& rhs)
			{
				auto const result = lhs.emplace(std::forward<Rhs>(rhs).value, GenericValueOut{});
				if(result.value == nullptr)
				{ raise_internal_error("lhs is not an obejct"); }
				return result.value;
			}

			template<class Rhs>
			requires instance_of<std::remove_cvref_t<Rhs>, key_to_clone>
			&& (std::ranges::range<typename std::remove_cvref_t<Rhs>::captured_type>)
			[[gnu::always_inline]] static auto update(GenericValueOut& lhs, Rhs&& rhs)
			{
				using output_type = std::remove_cvref_t<Rhs>::captured_type;
				auto const result = lhs.emplace(
					output_type{std::from_range_t{}, std::forward<Rhs>(rhs).value}, GenericValueOut{}
				);
				if(result.value == nullptr)
				{ raise_internal_error("lhs is not an obejct"); }
				return result.value;
			}

			template<class Rhs>
			requires std::ranges::range<Rhs>
			&& (!std::is_constructible_v<GenericValueOut, Rhs> && !instance_of<Rhs, key_to_clone>)
			[[gnu::always_inline]] static auto update(GenericValueOut& lhs, Rhs&& rhs)
			{
				using output_type = sequence_container_out<
					std::remove_cvref_t<std::ranges::range_value_t<Rhs>>
				>;

				lhs = GenericValueOut{output_type{std::from_range_t{}, std::forward<Rhs>(rhs)}};
				return &lhs;
			}
		};

		explicit clone_visitor_2(GenericValueOut& output_value)
		{
			m_contexts.reserve(1024);
			m_contexts.push_back(
				context{
					.parent_node = {},
					.output_value = value_storage_out{
						output_value,
						std::type_identity<generic_value_update_traits>{}
					}
				}
			);
		}

		template<class T>
		node_visitor_status handle_leaf_value(T&& value, value_visitation_context const& /*unused*/)
		{
			if(m_value_after_key != nullptr)
			{
				auto const val_ptr = m_value_after_key;
				m_value_after_key = nullptr;
				using convert_to = std::remove_cvref_t<std::remove_pointer_t<decltype(val_ptr)>>;
				*val_ptr = convert_to{std::forward<T>(value)};
			}
			else
			{ std::ignore = m_contexts.back().output_value.update_with(std::forward<T>(value)); }

			return node_visitor_status::ready;
		}

		template<class T>
		node_visitor_status handle_key(jopp2::container_proxy<T>& key, value_visitation_context const& /*unused*/)
		{
			auto& old_out = m_contexts.back().output_value;
			if(!old_out)
			{ jopp2::raise_internal_error("No output object present"); }

			m_value_after_key = old_out.update_with(
				key_to_clone<std::remove_const_t<T>>{key.active_range()}
			);
			key.pop_active_elements();
			return node_visitor_status::ready;
		}

		template<class T>
		node_visitor_status handle_key(T&& key, value_visitation_context const& /*unused*/)
		{
			auto& old_out = m_contexts.back().output_value;
			if(!old_out)
			{ jopp2::raise_internal_error("No output object present"); }

			m_value_after_key = old_out.update_with(key_to_clone{std::forward<T>(key)});
			return node_visitor_status::ready;
		}

		template<class T>
		node_visitor_status handle_simple_array(T& value, value_visitation_context const& /*unused*/)
		{
			using src_type = std::remove_cvref_t<T>;
			using src_value_type = src_type::value_type;
			using output_array = sequence_container_out<src_value_type>;
			if(m_value_after_key != nullptr)
			{
				auto const val_ptr = m_value_after_key;
				m_value_after_key = nullptr;
				*val_ptr = GenericValueOut{output_array{std::from_range_t{}, value.active_range()}};
			}
			else
			{
				auto const old_out = m_contexts.back().output_value;
				std::ignore =  old_out.update_with(value.active_range());
			}
			value.pop_active_elements();
			return node_visitor_status::ready;
		}

		template<class T>
		node_visitor_status handle_begin_of_container(
			container_proxy<T>& value,
			value_visitation_context const& /*unused*/
		)
		{
			auto const old_out = m_contexts.top().output_value;
			using container = std::conditional_t<
				std::is_same_v<T, objcontainer>,
				typename GenericValueOut::object,
				sequence_container_out<typename T::value_type>
			>;

			if(m_value_after_key != nullptr)
			{
				auto const val_ptr = m_value_after_key;
				m_value_after_key = nullptr;
				*val_ptr = GenericValueOut{container{}};
				m_contexts.push(
					context{
						.parent_node = old_out,
						.output_value = value_storage_out{
							*val_ptr,
							std::type_identity<generic_value_update_traits>{}
						}
					}
				);
			}
			else
			{
				auto const ret = old_out.update_with(container{});
				m_contexts.push(
					context{
						.parent_node = old_out,
						.output_value = value_storage_out{
							*ret,
							std::type_identity<generic_value_update_traits>{}
						}
					}
				);
			}
			return node_visitor_status::ready;
		}

		template<class T>
		node_visitor_status handle_end_of_container(
			T& /*unused*/,
			value_visitation_context const& /*unused*/
		)
		{
			m_contexts.pop_back();
			return node_visitor_status::ready;
		}

		struct context
		{
			value_storage_out parent_node;
			value_storage_out output_value;
		};

	private:
		std::vector<context> m_contexts;
		GenericValueOut* m_value_after_key{nullptr};
	};

	template<class SrcValueTemplateParamPack, class GenericValueOut>
	auto make_clone_visitor(GenericValueOut& ret)
	{
		return clone_visitor<SrcValueTemplateParamPack, GenericValueOut>(ret);
	}
}

#endif
