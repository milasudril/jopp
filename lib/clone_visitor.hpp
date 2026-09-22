#ifndef JOPP2_CLONE_VISITOR_HPP
#define JOPP2_CLONE_VISITOR_HPP

#include "./node_visitor_adaptor.hpp"
#include "./generic_value_update_traits.hpp"
#include "./container_update_traits.hpp"
#include "./value_storage.hpp"
#include "./template_param_pack.hpp"
#include "./exception.hpp"
#include <ranges>

namespace jopp2
{
	template<class GenericValueOut, class Other>
	struct clone_visitor_update_result
	{ using type = GenericValueOut*; };

	template<class GenericValueIn, class GenericValueOut>
	class clone_visitor_2
	{
	public:
		using src_value_param_pack = GenericValueIn::leaf_value_template_param_pack;
		using src_kv_item = GenericValueIn::object::value_type;
		using dest_kv_item = GenericValueOut::object::value_type;
		using objcontainer_in = std::conditional_t<
			std::is_const_v<GenericValueIn>,
			typename GenericValueIn::object const,
			typename GenericValueIn::object
		>;

		template<class T>
		using sequence_container_in = GenericValueIn::template sequence_container_type<T>;

		template<class T>
		using sequence_container_out = GenericValueIn::template sequence_container_type<T>;

		template<class T>
		using container_proxy_range = container_proxy<
			sequence_container_in<T> const
		>::active_range_type;

		using complete_pack = concatenate_template_param_packs_t<
			src_value_param_pack,
			wrap_template_param_pack_elements_t<
				make_template_param_pack_t<typename GenericValueIn::object::key_type>,
				key_to_clone
			>,
			wrap_template_param_pack_elements_t<
				src_value_param_pack,
				sequence_container_in
			>,
			template_param_pack<
				sequence_container_out<GenericValueOut>,
				typename GenericValueOut::object
			>
		>;

		template<class T>
		using update_result_t = clone_visitor_update_result<GenericValueOut, T>::type;

		template<class ... Args>
		using value_storage_with_result = value_storage<update_result_t, Args...>;

		using value_storage_out = map_template_param_pack_to_type_t<
			value_storage_with_result,
			complete_pack
		>;

		struct context
		{
			value_storage_out parent_node;
			value_storage_out output_value;
		};

		explicit clone_visitor_2(GenericValueOut& output_value)
		{
			output_value = GenericValueOut{};
			m_contexts.reserve(1024);
			m_contexts.push_back(
				context{
					.parent_node = {},
					.output_value = value_storage_out{
						output_value,
						std::type_identity<generic_value_update_traits<GenericValueOut>>{}
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
			{ m_contexts.back().output_value.update_with(std::forward<T>(value)); }

			return node_visitor_status::ready;
		}

		template<class T>
		node_visitor_status handle_key(jopp2::container_proxy<T>& key, value_visitation_context const& /*unused*/)
		{
			auto& old_out = m_contexts.back().output_value;
			if(!old_out)
			{ jopp2::raise_internal_error("No output object present"); }

			m_value_after_key = old_out.update_with(
				key_to_clone<std::remove_const_t<T>>{std::from_range_t{}, key.active_range()}
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
				old_out.update_with(output_array{std::from_range_t{}, value.active_range()});
			}
			value.pop_active_elements();
			return node_visitor_status::ready;
		}

		template<class T>
		node_visitor_status handle_begin_of_container(
			container_proxy<T>& /*value*/,
			value_visitation_context const& /*unused*/
		)
		{
			auto const old_out = m_contexts.back().output_value;
			using container = std::conditional_t<
				std::is_same_v<std::remove_const_t<T>, std::remove_const_t<objcontainer_in>>,
				typename GenericValueOut::object,
				std::conditional_t<
					std::is_same_v<typename T::value_type, GenericValueIn>,
					sequence_container_out<GenericValueOut>,
					sequence_container_out<typename T::value_type>
				>
			>;
			// TODO: use number of elements in value to reserve space if supported by container


			if(m_value_after_key != nullptr)
			{
				auto const val_ptr = m_value_after_key;
				m_value_after_key = nullptr;
				*val_ptr = GenericValueOut{container{}};
				m_contexts.push_back(
					context{
						.parent_node = old_out,
						.output_value = value_storage_out{
							*val_ptr->template get_if<container>(),
							std::type_identity<container_update_traits<container>>{}
						}
					}
				);
			}
			else
			{
				auto const ret = old_out.update_with(container{});
				m_contexts.push_back(
					context{
						.parent_node = old_out,
						.output_value = value_storage_out{
							*ret->template get_if<container>(),
							std::type_identity<container_update_traits<container>>{}
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

		auto const& contexts() const
		{ return m_contexts; }

		auto value_after_key() const
		{ return static_cast<GenericValueOut const*>(m_value_after_key); }

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
