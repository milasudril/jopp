#ifndef JOPP_GENERIC_VALUE_HPP
#define JOPP_GENERIC_VALUE_HPP

#include "./variant_utils.hpp"
#include "./utils.hpp"
#include "./template_param_pack.hpp"
#include "./updater.hpp"

#include <stdexcept>
#include <stack>

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

	struct value_visitation_context
	{
		size_t node_index;
		size_t parent_container_size;

		constexpr bool is_last_node() const
		{ return node_index == parent_container_size - 1; }

		constexpr bool is_first_node () const
		{ return node_index == 0; }
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
		using key_type = typename ValueTraits::key_type;
		using object = AssociativeContainerType<key_type, generic_value>;
		using map_value_type = object::value_type;
		static_assert(sequence_container<SequenceContainerType<leaf_value_type>>);

		template<class T>
		static constexpr auto is_leaf_value = requires(T&& x){
			{ leaf_value_type{std::forward<T>(x)} };
		};

		using value_template_param_pack_type = concatenate_template_param_packs_t<
			leaf_value_template_param_pack,
			template_param_pack<object>,
			wrap_template_param_pack_elements_t<
				leaf_value_template_param_pack, SequenceContainerType
			>,
			template_param_pack<SequenceContainerType<object>>,
			template_param_pack<SequenceContainerType<generic_value>>
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
		generic_value& operator=(generic_value&&) = default;

		template<class ... Args>
		explicit generic_value(Args&&... args):
			m_value{std::forward<Args>(args)...}
		{}

		template<class Self>
		auto&& get_value(this Self&& self)
		{ return std::forward_like<Self>(self.m_value); }

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
			using ret_type = std::pair<key_type const*, std::remove_cvref_t<T>*>;

			auto i = self.template get_if<object>();
			if(i == nullptr)
			{ return ret_type{}; }

			auto const insert_result = i->emplace(std::forward<KeyLike>(key), std::forward<T>(value));
			if(!insert_result.second)
			{ return ret_type{}; }

			if constexpr(std::is_same_v<std::remove_cvref_t<T>, generic_value>)
			{ return ret_type{&insert_result.first->first, &insert_result.first->second}; }
			else
			{ return ret_type{&insert_result.first->first, insert_result.first->second.template get_if<T>()}; }
		}

		template<class Self, class T, class KeyLike>
		auto store_value_as(this Self& self, T&& value, KeyLike&& key)
		{
			auto res = self.try_store_value_as(std::forward<T>(value), std::forward<KeyLike>(key));
			if(res.first == nullptr)
			{
				throw std::runtime_error{
					"This generic value is not an object, or the property has already been set"
				};
			}
			return std::pair<key_type const&, std::remove_cvref_t<T>&>{*res.first, *res.second};
		}

		template<class Self, class T>
		std::remove_cvref_t<T>* try_store_at_end(this Self& self, T&& value)
		{
			return visit_with_args(
				self.m_value,
				overload{
					[](SequenceContainerType<std::remove_cvref_t<T>>& seq, T&& value) -> std::remove_cvref_t<T>* {
						seq.emplace_back(std::forward<T>(value));
						return &seq.back();
					},
					[&self]<sequence_container Seq>(Seq& seq, T&& value)  -> std::remove_cvref_t<T>* {
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
					[](auto const&...)  -> std::remove_cvref_t<T>* {
						return static_cast<std::remove_cvref_t<T>*>(nullptr);
					}
				},
				std::forward<T>(value)
			);
		}

	private:
		value_type m_value;
	};

	struct begin_of_object{};

	struct end_of_object{};

	template<class ValueType>
	struct begin_of_array{};

	template<class ValueType>
	struct end_of_array{};

	template<class GenericValue, class Visitor>
	void visit_nodes(GenericValue&& root, Visitor&& visitor)
	{
		using generic_value = std::remove_cvref_t<GenericValue>;
		using value_type = typename generic_value::value_type;
		using key_type = typename generic_value::key_type;
		using object = typename generic_value::object;

		using node_value = concatenate_variants_t<
			wrap_variant_element_t<
				std::conditional_t<
					std::is_const_v<std::remove_reference_t<value_type>>,
					wrap_variant_element_t<std::remove_cvref_t<value_type>, std::add_const_t>,
					std::remove_cvref_t<value_type>
				>,
				std::add_pointer_t
			>,
			std::variant<
				std::pair<key_type const*, std::remove_reference_t<value_type>*>,
				begin_of_object,
				end_of_object,
				begin_of_array<generic_value>,
				end_of_array<generic_value>,
				begin_of_array<object>,
				end_of_array<object>
			>
		>;


		struct node
		{
			node_value value;
			value_visitation_context context;
		};

		struct visitation_state
		{
			Visitor visitor;
			std::stack<node> nodes_to_visit;
		};

		visitation_state current_state{
			.visitor = std::forward<Visitor>(visitor),
			.nodes_to_visit = {}
		};
		static constexpr auto make_node_value = [](value_type& item) static {
			return std::visit(
				[](auto& item){return node_value{&item};},
				item
			);
		};

		current_state.nodes_to_visit.push(
			node{
				.value = make_node_value(root.get_value()),
				.context = value_visitation_context{
					.node_index = 0,
					.parent_container_size = 1
				}
			}
		);

		while(!current_state.nodes_to_visit.empty())
		{
			auto current_node = current_state.nodes_to_visit.top();
			current_state.nodes_to_visit.pop();

			using obj_ptr = std::conditional_t<
				std::is_const_v<std::remove_reference_t<value_type>>,
				object const*,
				object*
			>;

			static constexpr auto handle_object = [](
				obj_ptr obj,
				visitation_state& state,
				value_visitation_context context
			) static {
				state.nodes_to_visit.push(
					node{
						.value = end_of_object{},
						.context = context
					}
				);
				auto const container_size = std::size(*obj);
				if constexpr(std::ranges::bidirectional_range<object>)
				{
					for(auto&& [index, item]: std::ranges::reverse_view{std::ranges::enumerate_view{*obj}})
					{
						state.nodes_to_visit.push(
							node{
								.value = std::pair{&item.first, &item.second.get_value()},
								.context = value_visitation_context{
									.node_index = static_cast<size_t>(index),
									.parent_container_size = container_size
								}
							}
						);
					}
				}
				else
				{
					for(auto&& [index, item]: std::ranges::enumerate_view{*obj})
					{
						state.nodes_to_visit.push(
							node{
								.value = std::pair{&item.first, &item.second.get_value()},
								.context = value_visitation_context{
									.node_index = container_size - static_cast<size_t>(index) - 1,
									.parent_container_size = container_size
								}
							}
						);
					}
				}
				state.nodes_to_visit.push(
					node{
						.value = begin_of_object{},
						.context = context
					}
				);
			};

			visit_with_args(
				current_node.value,
				overload{
					handle_object,
					[]<class LeafValue> requires(generic_value::template is_leaf_value<LeafValue>)
					(LeafValue* value, visitation_state& state, value_visitation_context context){
						state.visitor.handle_leaf_value(*value, context);
					},
					[]<class Seq>
					requires sequence_container<std::remove_cvref_t<Seq>>
					(Seq* seq, visitation_state& state, value_visitation_context context) {
						using seq_type = std::remove_cvref_t<Seq>;
						auto const container_size = std::size(*seq);
						using value_type = typename seq_type::value_type;
						if constexpr(std::is_same_v<value_type, generic_value>)
						{
							state.nodes_to_visit.push(
								node{
									.value = end_of_array<value_type>{},
									.context = context
								}
							);
							for(auto&& [index, item]: std::ranges::reverse_view{std::ranges::enumerate_view{*seq}})
							{
								state.nodes_to_visit.push(
									node{
										.value = make_node_value(item.get_value()),
										.context = value_visitation_context{
											.node_index = static_cast<size_t>(index),
											.parent_container_size = container_size,
										}
									}
								);
							}
							state.nodes_to_visit.push(
								node{
									.value = begin_of_array<value_type>{},
									.context = context
								}
							);
						}
						else
						if constexpr(std::is_same_v<value_type, object>)
						{
							state.nodes_to_visit.push(
								node{
									.value = end_of_array<value_type>{},
									.context = context
								}
							);
							for(auto&& [index, item]: std::ranges::reverse_view{std::ranges::enumerate_view{*seq}})
							{
								handle_object(
									&item, state,
									value_visitation_context{
										.node_index = static_cast<size_t>(index),
										.parent_container_size = container_size
									}
								);
							}
							state.nodes_to_visit.push(
								node{
									.value = begin_of_array<value_type>{},
									.context = context
								}
							);
						}
						else
						{
							state.visitor.handle_begin_of_array(std::type_identity<value_type>{}, context);
							for(auto&& [index, item]: std::ranges::enumerate_view{*seq})
							{
								state.visitor.handle_leaf_value(
									item,
									value_visitation_context{
										.node_index = static_cast<size_t>(index),
										.parent_container_size = container_size
									}
								);
							}
							state.visitor.handle_end_of_array(std::type_identity<value_type>{}, context);
						}
					},
					[](
						std::pair<key_type const*, std::remove_reference_t<value_type>*> kv_ptr,
						visitation_state& state,
						value_visitation_context context
					) {
						state.visitor.handle_property_name(*kv_ptr.first, context);
						state.nodes_to_visit.push(
							node{
								.value = make_node_value(*kv_ptr.second),
								.context = context
							}
						);
					},
					[](begin_of_object, visitation_state& state, value_visitation_context context) {
						state.visitor.handle_begin_of_object(context);
					},
					[](end_of_object, visitation_state& state, value_visitation_context context) {
						state.visitor.handle_end_of_object(context);
					},
					[]<class T>(begin_of_array<T>, visitation_state& state, value_visitation_context context) {
						state.visitor.handle_begin_of_array(std::type_identity<T>{}, context);
					},
					[]<class T>(end_of_array<T>, visitation_state& state, value_visitation_context context) {
						state.visitor.handle_end_of_array(std::type_identity<T>{}, context);
					}
				},
				current_state,
				current_node.context
			);
		}
	}

	template<class Lhs, class Rhs>
	struct clone_visitor_value_update_traits_impl
	{
		THISCALL static int update(Lhs& lhs, update_param_t<Rhs> rhs)
		{
			lhs = Lhs{maybe_move(rhs)};
			return 0;
		}
	};

	template<class T>
	struct clone_visitor_update_result
	{ using type = int; };

	template<class SrcValueTemplateParamPack, class GenericValueOut>
	class clone_visitor
	{
	public:
		using kv_item = std::pair<typename GenericValueOut::key_type, GenericValueOut>;

		template<class... SrcValueTypes>
		struct clone_visitor_value_update_traits:
			clone_visitor_value_update_traits_impl<GenericValueOut, SrcValueTypes>...
		{
			using clone_visitor_value_update_traits_impl<GenericValueOut, SrcValueTypes>::update...;

			THISCALL static int update(GenericValueOut&, update_param_t<kv_item>)
			{ return 0; }
		};

		using pack_with_kv_item_and_object = concatenate_template_param_packs_t<
			SrcValueTemplateParamPack,
			template_param_pack<kv_item, typename GenericValueOut::object>
		>;

		using output_value_update_traits = map_template_param_pack_to_type_t<
			clone_visitor_value_update_traits,
			pack_with_kv_item_and_object
		>;

		template<class T>
		using update_result_t = typename clone_visitor_update_result<T>::type;

		template<class ... Args>
		using updater_with_result = updater<update_result_t, Args...>;

		using value_updater = map_template_param_pack_to_type_t<
			updater_with_result,
			pack_with_kv_item_and_object
		>;

		explicit clone_visitor(GenericValueOut& output_value)
		{
			m_contexts.push(
				context{
					.parent_node = {},
					.output_value = value_updater{
						output_value,
						std::type_identity<output_value_update_traits>{}
					}
				}
			);
		}

		template<class T>
		void handle_leaf_value(T&&, value_visitation_context)
		{
#if 0
			if(m_contexts.top().output_value == nullptr)
			{
				if(m_contexts.top().parent_node != nullptr)
				{ m_contexts.top().parent_node->try_store_at_end(std::forward<T>(value)); }
			}
			else
			{ *m_contexts.top().output_value = GenericValueOut{std::forward<T>(value)}; }
#endif
		}

		template<class T>
		void handle_property_name(T&&, value_visitation_context)
		{
#if 0
			if(m_contexts.top().parent_node != nullptr)
			{
				auto const res = m_contexts.top().parent_node->try_store_value_as(
					GenericValueOut{},
					std::forward<T>(prop_name)
				);
				m_contexts.top().output_value = res.second;
			}
#endif
		}

		void handle_begin_of_object(value_visitation_context)
		{
			if(m_contexts.top().output_value)
			{
				std::ignore = m_contexts.top().output_value.update_with(typename GenericValueOut::object{});

			}

			m_contexts.push(
				context{
					.parent_node = m_contexts.top().output_value,
					.output_value = {}
				}
			);
		}

		void handle_end_of_object(value_visitation_context)
		{
			m_contexts.pop();
		}

		template<class T>
		void handle_begin_of_array(std::type_identity<T> /*unused*/, value_visitation_context)
		{
#if 0
			if(m_contexts.top().output_value != nullptr)
			{
				*m_contexts.top().output_value = GenericValueOut{
					typename GenericValueOut::generic_sequence_container{}
				};
			}

			m_contexts.push(
				context{
					.parent_node = m_contexts.top().output_value,
					.output_value = nullptr
				}
			);
#endif
		}

		template<class T>
		void handle_end_of_array(std::type_identity<T> /*unused*/, value_visitation_context)
		{
#if 0
			m_contexts.pop();
#endif
		}

		struct context
		{
			value_updater parent_node;
			value_updater output_value;
		};

	private:
		std::stack<context> m_contexts;
	};

	template<class SrcValueTemplateParamPack, class GenericValueOut>
	auto make_clone_visitor(GenericValueOut& ret)
	{
		return clone_visitor<SrcValueTemplateParamPack, GenericValueOut>(ret);
	}

	template<class GenericValueOut, class GenericValueIn>
	auto clone(GenericValueIn&& src)
	{
		using src_value_template_param_pack = typename std::remove_cvref_t<GenericValueIn>::leaf_value_template_param_pack;
		GenericValueOut ret;
		visit_nodes(
			std::forward<GenericValueIn>(src),
			make_clone_visitor<src_value_template_param_pack>(ret)
		);
		return ret;
	}
}

#endif