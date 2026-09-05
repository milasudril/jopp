#ifndef JOPP2_CLONE_VISITOR_HPP
#define JOPP2_CLONE_VISITOR_HPP

#include "./node_visitor_adaptor.hpp"
#include "./value_storage.hpp"
#include "./template_param_pack.hpp"

namespace jopp2
{
	template<class T>
	struct clone_visitor_update_result
	{ using type = T*; };

	template<class GenericValueOut>
	struct clone_visitor_update_result<std::pair<typename GenericValueOut::key_type, GenericValueOut>>
	{ using type = GenericValueOut*; };

	template<class GenericValueIn, class GenericValueOut>
	class clone_visitor_2
	{
	public:
		using src_value_param_pack = GenericValueIn::leaf_value_template_param_pack;
		using src_kv_item = GenericValueIn::object::value_type;
		using complete_pack_with_kv_item = concatenate_template_param_packs_t<
			src_value_param_pack,
			wrap_in_template_param_pack_t<src_kv_item>
		>;

		template<class T>
		using update_result_t = clone_visitor_update_result<T>::type;

		template<class ... Args>
		using value_storage_with_result = value_storage<update_result_t, Args...>;

		using value_storage_out = map_template_param_pack_to_type_t<
			value_storage_with_result,
			complete_pack_with_kv_item
		>;


		explicit clone_visitor_2(GenericValueOut& output_value)
		{
			m_contexts.push(
				context{
					.parent_node = {},
					.output_value = value_storage_out{
						output_value,
						std::type_identity<output_value_update_traits>{}
					}
				}
			);
		}

		template<class T>
		void handle_leaf_value(T&& value, value_visitation_context const& /*unused*/)
		{
			if(m_value_after_key != nullptr)
			{
				auto const val_ptr = m_value_after_key;
				m_value_after_key = nullptr;
				using convert_to = std::remove_cvref_t<std::remove_pointer_t<decltype(val_ptr)>>;
				*val_ptr = convert_to{std::forward<T>(value)};
			}
			else
			{
				auto _ = m_contexts.top().output_value.update_with(std::forward<T>(value));
			}
		}

		template<class T>
		void handle_key(T&& prop_name, value_visitation_context const& /*unused*/)
		{
			auto& old_out = m_contexts.top().output_value;
			if(old_out)
			{
				m_value_after_key = old_out.update_with(
					std::pair{
						std::forward<T>(prop_name),
						GenericValueOut{}
					}
				);
			}
		}

		template<class T>
		void handle_begin_of_container(T& container, value_visitation_context const& /*unused*/)
		{
			auto const old_out = m_contexts.top().output_value;
			if(m_value_after_key != nullptr)
			{
				auto const val_ptr = m_value_after_key;
				m_value_after_key = nullptr;
				*val_ptr = GenericValueOut{typename GenericValueOut::object{}};
				m_contexts.push(
					context{
						.parent_node = old_out,
						.output_value = value_storage_out{
							*val_ptr,
							std::type_identity<output_value_update_traits>{}
						}
					}
				);
			}
			else
			{
				auto const ret = old_out.update_with(typename GenericValueOut::object{});
				m_contexts.push(
					context{
						.parent_node = old_out,
						.output_value = value_storage_out{
							*ret,
							std::type_identity<output_object_update_traits>{}
						}
					}
				);
			}
		}


		void handle_end_of_object(value_visitation_context /*unused*/)
		{ m_contexts.pop(); }

		void handle_begin_of_array(std::type_identity<src_value> /*unused*/, value_visitation_context /*unused*/)
		{
			auto const old_out = m_contexts.top().output_value;
			using output_array = sequence_container_out<GenericValueOut>;
			if(m_value_after_key != nullptr)
			{
				auto const val_ptr = m_value_after_key;
				m_value_after_key = nullptr;
				*val_ptr = GenericValueOut{output_array{}};
				m_contexts.push(
					context{
						.parent_node = old_out,
						.output_value = value_storage_out{
							*val_ptr->template get_if<output_array>(),
							std::type_identity<output_array_update_traits<output_array>>{}
						}
					}
				);
			}
			else
			{
				auto const ret = old_out.update_with(output_array{});
				m_contexts.push(
					context{
						.parent_node = old_out,
						.output_value = value_storage_out{
							*ret,
							std::type_identity<output_array_update_traits<output_array>>{}
						}
					}
				);
			}
		}

		void handle_begin_of_array(std::type_identity<src_object> /*unused*/, value_visitation_context /*unused*/)
		{
			auto const old_out = m_contexts.top().output_value;
			using output_array = sequence_container_out<typename GenericValueOut::object>;
			assert(old_out);
			if(m_value_after_key != nullptr)
			{
				auto const val_ptr = m_value_after_key;
				m_value_after_key = nullptr;
				*val_ptr = GenericValueOut{output_array{}};
				m_contexts.push(
					context{
						.parent_node = old_out,
						.output_value = value_storage_out{
							*val_ptr->template get_if<output_array>(),
							std::type_identity<output_array_update_traits<output_array>>{}
						}
					}
				);
			}
			else
			{
				auto const ret = old_out.update_with(output_array{});
				m_contexts.push(
					context{
						.parent_node = old_out,
						.output_value = value_storage_out{
							*ret,
							std::type_identity<output_array_update_traits<output_array>>{}
						}
					}
				);
			}
		}

		template<class T>
		void handle_end_of_array(std::type_identity<T> /*unused*/, value_visitation_context /*unused*/)
		{ m_contexts.pop(); }

		template<class T>
		void handle_leaf_value_array(T const& src, value_visitation_context /*unused*/)
		{
			using src_type = std::remove_cvref_t<T>;
			using src_value_type = typename src_type::value_type;
			using output_array = sequence_container_out<src_value_type>;
			output_array* out_ptr{nullptr};
			if(m_value_after_key != nullptr)
			{
				auto const val_ptr = m_value_after_key;
				m_value_after_key = nullptr;
				*val_ptr = GenericValueOut{output_array{}};
				out_ptr = val_ptr->template get_if<output_array>();
			}
			else
			{
				auto const old_out = m_contexts.top().output_value;
				out_ptr = old_out.update_with(output_array{});
			}
			assert(out_ptr != nullptr);
			if constexpr(pass_by_value_v<src_value_type> && std::is_default_constructible_v<src_value_type>)
			{
				out_ptr->resize(std::size(src));
				std::copy(std::begin(src), std::end(src), std::begin(*out_ptr));
			}
			else
			{
				out_ptr->reserve(std::size(src));
				std::copy(std::begin(src), std::end(src), std::back_inserter(*out_ptr));
			}
		}

		struct context
		{
			value_storage_out parent_node;
			value_storage_out output_value;
		};

	private:
		std::stack<context> m_contexts;
		GenericValueOut* m_value_after_key{nullptr};
	};

	template<class SrcValueTemplateParamPack, class GenericValueOut>
	auto make_clone_visitor(GenericValueOut& ret)
	{
		return clone_visitor<SrcValueTemplateParamPack, GenericValueOut>(ret);
	}
}

#endif
