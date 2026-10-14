#ifndef JOPP2_CLONE_VISITOR_HPP
#define JOPP2_CLONE_VISITOR_HPP

#include "./value_sink_ref.hpp"
#include "./template_param_pack.hpp"
#include "./exception.hpp"
#include "./sequence_container.hpp"
#include "./node_visitor_adaptor.hpp"

namespace jopp2
{
	template<class GenericValueIn, class GenericValueOut>
	class clone_visitor_2
	{
	public:
		using objcontainer_in = std::conditional_t<
			std::is_const_v<GenericValueIn>,
			typename GenericValueIn::object const,
			typename GenericValueIn::object
		>;

		using objcontainer_out = GenericValueOut::object;

		template<class T>
		using sequence_container_out = GenericValueOut::template sequence_container_type<T>;

		using supported_sink_types =
			append_to_template_param_pack_t<
				filter_template_param_pack_t<
					[]<class T>(std::type_identity<T>){
						if constexpr(sequence_container<T>)
						{
							return !GenericValueOut::template is_leaf_value<T>
								|| sequence_container<typename T::value_type>
								|| std::is_same_v<typename T::value_type, objcontainer_out>;
						}
						else
						{ return std::is_same_v<T, objcontainer_out>; }
					},
					typename GenericValueOut::value_template_param_pack_type
				>,
				GenericValueOut
			>;

		using value_storage_out = value_sink_ref<clone_visitor_2>;

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
						output_value
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
			{
				if(m_contexts.empty())
				{ raise_internal_error("No contexts"); }

				m_contexts.back().output_value.store_value(std::forward<T>(value));
			}

			return node_visitor_status::ready;
		}

		template<class T>
		node_visitor_status handle_key(jopp2::container_proxy<T>& key, value_visitation_context const& /*unused*/)
		{
			if(m_contexts.empty())
			{ raise_internal_error("No contexts"); }

			auto& old_out = m_contexts.back().output_value;
			using key_type = std::remove_const_t<T>;
			m_value_after_key = &old_out.store_value(
				typename objcontainer_out::value_type{
					typename objcontainer_out::key_type{key_type{std::from_range_t{}, key.active_range()}},
					GenericValueOut{}
				}
			).second;
			key.pop_active_elements();
			return node_visitor_status::ready;
		}

		template<class T>
		node_visitor_status handle_key(T&& key, value_visitation_context const& /*unused*/)
		{
			auto& old_out = m_contexts.back().output_value;
			m_value_after_key = &old_out.store_value(
				typename objcontainer_out::value_type{
					typename objcontainer_out::key_type{std::forward<T>(key)},
					GenericValueOut{}
				}
			).second;
			return node_visitor_status::ready;
		}

		/**
		 * \todo Add handling of the case when subtype id is of bounded size (not wrapped in
		 *       container_proxy)
		 */
		template<class T>
		node_visitor_status handle_subtype_id(
			jopp2::container_proxy<T>& id,
			jopp2::value_visitation_context const& /*unused*/
		)
		{
			m_current_subtype_id = typename objcontainer_out::subtype_id_type{
				std::remove_const_t<T>(std::from_range_t{}, id.active_range())
			};
			id.pop_active_elements();
			return node_visitor_status::ready;
		}

		template<class T>
		node_visitor_status handle_simple_array(T& value, value_visitation_context const& /*unused*/)
		{
			using src_type = std::remove_cvref_t<T>;
			using src_value_type = src_type::value_type;
			using output_array = std::conditional_t<
				GenericValueOut::template is_leaf_value<typename T::container_type>,
				std::remove_const_t<typename T::container_type>,
				sequence_container_out<src_value_type>
			>;
			if(m_value_after_key != nullptr)
			{
				auto const val_ptr = m_value_after_key;
				m_value_after_key = nullptr;
				*val_ptr = GenericValueOut{output_array{std::from_range_t{}, value.active_range()}};
			}
			else
			{
				if(m_contexts.empty())
				{ raise_internal_error("No contexts"); }

				auto const old_out = m_contexts.back().output_value;
				old_out.store_value(output_array{std::from_range_t{}, value.active_range()});
			}
			value.pop_active_elements();
			return node_visitor_status::ready;
		}

		template<class Container>
		Container create_container()
		{
			if constexpr(std::is_same_v<Container, objcontainer_out>)
			{ return Container{std::move(m_current_subtype_id)}; }
			else
			{ return Container{};}
		}

		template<class Container>
		static Container&& reserve_space_if_supported(Container&& c, size_t elem_count)
		{
			if constexpr (requires { c.reserve(elem_count); })
			{ c.reserve(elem_count); }
			return std::forward<Container>(c);
		}

		template<class T>
		node_visitor_status handle_begin_of_container(
			container_proxy<T>& value,
			value_visitation_context const& /*unused*/
		)
		{
			if(m_contexts.empty())
			{ raise_internal_error("No contexts"); }

			auto const old_out = m_contexts.back().output_value;
			using container = std::conditional_t<
				std::is_same_v<std::remove_const_t<T>, std::remove_const_t<objcontainer_in>>,
				typename GenericValueOut::object,
				std::conditional_t<
					std::is_same_v<typename T::value_type, GenericValueIn>,
					sequence_container_out<GenericValueOut>,
					std::conditional_t<
						std::is_same_v<std::remove_const_t<typename T::value_type>, std::remove_const_t<objcontainer_in>>,
						sequence_container_out<objcontainer_out>,
						sequence_container_out<typename T::value_type>
					>
				>
			>;

			if(m_value_after_key != nullptr)
			{
				auto const val_ptr = m_value_after_key;
				m_value_after_key = nullptr;
				*val_ptr = GenericValueOut{
					reserve_space_if_supported(create_container<container>(), value.total_size())
				};
				m_contexts.push_back(
					context{
						.parent_node = old_out,
						.output_value = value_storage_out{
							*val_ptr->template get_if<container>()
						}
					}
				);
			}
			else
			{
				auto& ret = old_out.store_value(
					reserve_space_if_supported(create_container<container>(), value.total_size())
				);
				m_contexts.push_back(
					context{
						.parent_node = old_out,
						.output_value = value_storage_out{ret}
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
			if(m_contexts.empty())
			{ raise_internal_error("No contexts"); }

			m_contexts.pop_back();
			return node_visitor_status::ready;
		}

		auto const& contexts() const
		{ return m_contexts; }

		auto value_after_key() const
		{ return static_cast<GenericValueOut const*>(m_value_after_key); }

		template<class Source, class Sink>
		[[noreturn]] static void value_sink_type_mismatch(
			source_value_type_tag<Source> /*unused*/,
			sink_type_tag<Sink> /*unused*/
		)
		{
			raise_internal_error(
				"Value sink type mismatch Source = {}, Sink = {}",
				make_fmt_args(
					typeid(Source).name(),
					typeid(Sink).name()
				)
			);
		}

		template<class Source>
		requires(std::is_constructible_v<GenericValueOut, Source>)
		static decltype(auto) store_value(GenericValueOut& sink, Source&& source)
		{
			sink = GenericValueOut{std::forward<Source>(source)};
			return *sink.template get_if<std::remove_cvref_t<Source>>();
		}

		template<class Source>
		requires(std::is_same_v<std::remove_cvref_t<Source>, typename objcontainer_out::value_type>)
		static decltype(auto) store_value(objcontainer_out& sink, Source&& src)
		{ return *sink.insert(std::forward<Source>(src)).first; }

		template<class Source>
		requires(std::is_constructible_v<GenericValueOut, Source>)
		static decltype(auto) store_value(sequence_container_out<GenericValueOut>& seq_out, Source&& src)
		{
			seq_out.push_back(GenericValueOut{std::forward<Source>(src)});
			return *seq_out.back().template get_if<std::remove_const_t<Source>>();
		}

		template<class Dest, class Source>
		requires(!std::is_same_v<Dest, GenericValueOut> && std::is_constructible_v<Dest, Source>)
		static decltype(auto) store_value(sequence_container_out<Dest>& seq_out, Source&& src)
		{
			seq_out.push_back(Dest{std::forward<Source>(src)});
			return seq_out.back();
		}

	private:
		std::vector<context> m_contexts;
		GenericValueOut* m_value_after_key{nullptr};
		objcontainer_out::subtype_id_type m_current_subtype_id;
	};

	template<class SrcValueTemplateParamPack, class GenericValueOut>
	auto make_clone_visitor(GenericValueOut& ret)
	{
		return clone_visitor<SrcValueTemplateParamPack, GenericValueOut>(ret);
	}
}

#endif
