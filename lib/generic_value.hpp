#ifndef JOPP_GENERIC_VALUE_HPP
#define JOPP_GENERIC_VALUE_HPP

#include "./variant_utils.hpp"
#include "./utils.hpp"
#include "./template_param_pack.hpp"
#include "./value_storage.hpp"
#include "./exception.hpp"
#include "./container_proxy.hpp"

#include <ranges>
#include <stack>
#include <algorithm>
#include <type_traits>
#include <list>

namespace jopp2
{
	using jopp::overload;
	using jopp::instance_of;

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
			m_value{nullptr},
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
		using key_type = typename ValueTraits::key_type;
		using object = AssociativeContainerType<key_type, generic_value>;
		using map_value_type = object::value_type;
		template<class T>
		using sequence_container_type = SequenceContainerType<T>;
		static_assert(sequence_container<sequence_container_type<leaf_value_type>>);

		template<class T>
		static constexpr auto is_leaf_value = type_is_present_v<T, leaf_value_template_param_pack>;

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
		~generic_value() = default;

		generic_value(generic_value const&) = delete;
		generic_value& operator=(generic_value const&) = delete;
		generic_value(value_type const&) = delete;
		generic_value& operator=(value_type const&) = delete;

		generic_value(generic_value&&) = default;
		generic_value& operator=(generic_value&&) = default;

		template<class ... Args>
		requires(std::is_constructible_v<value_type, Args...>)
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
			auto item = self.template get_if<object>();
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
		{ return std::forward_like<Self>(self.template get_if_by_name<T>(key).value(key)); }

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
				.was_inserted=insert_result.second
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
						seq.emplace_back(std::forward<T>(value));
						return &seq.back();
					},
					[&self]<sequence_container Seq>(Seq& seq, T&& value) -> std::remove_cvref_t<T>* {
						if(seq.empty())
						{
							SequenceContainerType<std::remove_cvref_t<T>> new_container{};
							new_container.emplace_back(std::forward<T>(value));
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

	struct begin_of_object{};

	struct end_of_object{};

	template<class ValueType>
	struct begin_of_array{};

	template<class ValueType>
	struct end_of_array{};

	template<class SrcRange>
	decltype(auto) make_range_to_push(SrcRange&& range)
	{
		if constexpr(std::ranges::bidirectional_range<SrcRange>)
		{ return std::ranges::reverse_view{std::forward<SrcRange>(range)};}
		else
		{ return std::forward<SrcRange>(range); }
	}

	enum class visitor_status{suspend, keep_going};

	template<class Type, bool IsConst>
	struct node_item
	{
		using input_type = std::remove_cvref_t<Type>;

		using type = std::conditional_t<
			IsConst,
			std::conditional_t<
				pass_by_value_v<input_type>,
				input_type,
				std::reference_wrapper<input_type const>
			>,
			std::reference_wrapper<input_type>
		>;

		using param_type = std::conditional_t<
			IsConst,
			std::conditional_t<
				pass_by_value_v<input_type>,
				input_type,
				Type const&
			>,
			Type&
		>;

		static constexpr decltype(auto) create(type val)
		{ return val; }
	};

	template<class Type, bool IsConst>
	requires(std::ranges::range<Type>)
	struct node_item<Type, IsConst>
	{
		using input_type =  std::conditional_t<
			IsConst,
			std::remove_cvref_t<Type> const,
			std::remove_cvref_t<Type>
		>;

		using type = container_proxy<input_type>;

		using param_type = type&;

		static constexpr auto create(input_type& val)
		{ return type{val}; }
	};
	template<class GenericValue, class Visitor>
	class node_visitor_2
	{
	public:
		using generic_value_t = std::remove_cvref_t<GenericValue>;
		static constexpr auto src_is_const = std::is_const_v<std::remove_reference_t<GenericValue>>;
		using value_type = typename generic_value_t::value_type;
		using object = typename generic_value_t::object;
		using objcontainer = std::conditional_t<
			src_is_const,
			object const,
			object
		>;

		template<class T>
		using sequence_container_type =
			std::conditional_t<
				src_is_const,
				typename generic_value_t::template sequence_container_type<T> const,
				typename generic_value_t::template sequence_container_type<T>
			>;

		template <class T>
		using callback_param_t = typename node_item<T, src_is_const>::param_type;

		template<class T>
		using node_item_t = typename node_item<T, src_is_const>::type;

		using node_value = wrap_variant_element_t<
			std::conditional_t<
				src_is_const,
				wrap_variant_element_t<value_type, std::add_const_t>,
				value_type
			>,
			node_item_t
		>;

		struct node
		{
			node_value value;
		};

		[[gnu::always_inline]] static auto make_node_value(auto& item)
		{
			return std::visit(
				[]<class T>(T& item){
					return node_value{node_item<T, src_is_const>::create(item)};
				},
				item
			);
		};

		template<class VisitorType>
		explicit node_visitor_2(GenericValue& root, VisitorType&& visitor):
			m_visitor{std::forward<VisitorType>(visitor)}
		{
		//	m_nodes.reserve(1024);
			m_nodes.push_back(
				node{
					.value = make_node_value(root.get_value())
				}
			);
		}

		template<class ... VisitorArgs>
		explicit node_visitor_2(GenericValue& root, std::in_place_t /*unused*/, VisitorArgs&&... args):
			m_visitor{std::forward<VisitorArgs>(args)...}
		{
		//	m_nodes.reserve(1024);
			m_nodes.push_back(
				node{
					.value = make_node_value(root.get_value())
				}
			);
		}

		[[nodiscard]] auto visit_nodes()
		{
			while(!m_nodes.empty())
			{
				auto& current_node = m_nodes.back();
				if(visit_with_args(current_node.value, *this) == 0)
				{ m_nodes.pop_back(); }
			}
			return 0;
		}

		template<class T>
		requires(generic_value_t::template is_leaf_value<std::remove_cvref_t<T>>)
		size_t dispatch(callback_param_t<std::remove_cvref_t<T>> /*TODO*/)
		{
		//	printf("leaf value %s\n", typeid(T).name());
			return 0;
		}

		template<class T>
		requires instance_of<std::remove_cvref_t<T>, container_proxy>
			&& (!std::is_same_v<std::remove_cvref_t<T>, container_proxy<objcontainer>>)
		size_t dispatch(T& /*TODO*/)
		{
		//	puts("leaf value container");
			return 0;

		}

		size_t dispatch(container_proxy<sequence_container_type<generic_value_t>>& /*TODO*/)
		{
		//	puts("array of values");
			return 0;

		}

		size_t dispatch(container_proxy<sequence_container_type<object>>& /*TODO*/)
		{
		//	puts("array of objects");
			return 0;
		}

		size_t dispatch(container_proxy<objcontainer>& obj)
		{
			if(obj.at_end())
			{
				puts("}");
				m_visitor.handle_end_of_object();
				return 0;
			}

			if(obj.at_begin())
			{
				puts("{");
				m_visitor.handle_begin_of_object(obj.total_size());
			}

			printf("key: %s\n", obj.active_range().begin()->first.c_str());
			fflush(stdout);

			auto& next_item = obj.active_range().begin()->second.get_value();
			obj.pop_active_element();

			m_nodes.push_back(
				node{
					.value = make_node_value(next_item)
				}
			);

			return 1;
		}


		template<class T>
		requires(generic_value_t::template is_leaf_value<std::remove_cvref_t<T>>)
		[[gnu::always_inline]] size_t operator()(std::reference_wrapper<T> item)
		{ return dispatch<T>(item.get()); }

		template<class T>
		[[gnu::always_inline]] size_t operator()(T&& item)
		{ return dispatch<std::remove_cvref_t<T>>(std::forward<T>(item)); }

		[[gnu::always_inline]]
		size_t operator()(container_proxy<sequence_container_type<generic_value_t>>& item)
		{ return dispatch(item); }

		[[gnu::always_inline]] size_t operator()(container_proxy<sequence_container_type<object>>& item)
		{ return dispatch(item); }

		[[gnu::always_inline]] size_t operator()(container_proxy<objcontainer>& item)
		{ return dispatch(item); }

	private:
		Visitor m_visitor;
		std::list<node> m_nodes;
	};

	template<class GenericValue, class Visitor>
	class node_visitor
	{
	public:
		using generic_value = std::remove_cvref_t<GenericValue>;
		using value_type = typename generic_value::value_type;
		using key_type = typename generic_value::key_type;
		using object = typename generic_value::object;
		using objptr =  std::conditional_t<
			std::is_const_v<std::remove_reference_t<GenericValue>>,
			object const*,
			object*
		>;

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
				begin_of_array<src_value>,
				end_of_array<src_value>,
				begin_of_array<src_object>,
				end_of_array<src_object>
			>
		>;

		struct node
		{
			node_value value;
			value_visitation_context context;
		};

		template<class VisitorType>
		explicit node_visitor(GenericValue& root, VisitorType&& visitor):
			m_visitor{std::forward<VisitorType>(visitor)}
		{
			m_nodes_to_visit.push(
				node{
					.value = make_node_value(root.get_value()),
					.context = value_visitation_context{
						.node_index = 0,
						.parent_container_size = 1
					}
				}
			);
		}

		template<class ... VisitorArgs>
		explicit node_visitor(GenericValue& root, std::in_place_t /*unused*/, VisitorArgs&&... args):
			m_visitor{std::forward<VisitorArgs>(args)...}
		{
			m_nodes_to_visit.push(
				node{
					.value = make_node_value(root.get_value()),
					.context = value_visitation_context{
						.node_index = 0,
						.parent_container_size = 1
					}
				}
			);
		}

		[[gnu::always_inline]] static auto make_node_value(value_type& item)
		{
			return std::visit(
				[](auto& item){return node_value{&item};},
				item
			);
		};

		[[nodiscard]] auto visit_nodes()
		{
			if constexpr(Visitor::is_suspendable)
			{
				auto const result = m_visitor.flush();
				if(result == visitor_status::suspend)
				{ return result; }
			}

			while(!m_nodes_to_visit.empty())
			{
				auto current_node = m_nodes_to_visit.top();
				m_nodes_to_visit.pop();
				if constexpr(Visitor::is_suspendable)
				{
					if(visit_with_args(current_node.value, *this, current_node.context) == visitor_status::suspend)
					{ return visitor_status::suspend; }
				}
				else
				{ visit_with_args(current_node.value, *this, current_node.context); }
			}

			if constexpr(Visitor::is_suspendable)
			{ return visitor_status::keep_going; }
		}

		template<class LeafValue>
		requires(generic_value::template is_leaf_value<LeafValue>)
		auto operator()(LeafValue* value, value_visitation_context context)
		{ return m_visitor.handle_leaf_value(*value, context); }

		auto operator()(
			std::pair<key_type const*, std::remove_reference_t<value_type>*> kv_ptr,
			value_visitation_context context
		)
		{
			m_nodes_to_visit.push(
				node{
					.value = make_node_value(*kv_ptr.second),
					.context = context
				}
			);
			return m_visitor.handle_property_name(*kv_ptr.first, context);
		}

		auto operator()(objptr obj, value_visitation_context context)
		{
			m_nodes_to_visit.push(
				node{
					.value = end_of_object{},
					.context = context
				}
			);
			auto const container_size = std::size(*obj);
			for(auto&& [index, item]: make_range_to_push(std::ranges::enumerate_view{*obj}))
			{
				m_nodes_to_visit.push(
					node{
						.value = std::pair{&item.first, &item.second.get_value()},
						.context = value_visitation_context{
							.node_index = static_cast<size_t>(index),
							.parent_container_size = container_size
						}
					}
				);
			}

			m_nodes_to_visit.push(
				node{
					.value = begin_of_object{},
					.context = context
				}
			);

			if constexpr(Visitor::is_suspendable)
			{ return visitor_status::keep_going; }
		}

		template<class Seq>
		requires(
			sequence_container<std::remove_cvref_t<Seq>> &&
			generic_value::template is_leaf_value <std::ranges::range_value_t<std::remove_cvref_t<Seq>>>
		)
		auto operator()(Seq* seq, value_visitation_context context)
		{ return m_visitor.handle_leaf_value_array(*seq, context); }

		template<class Seq>
		requires(
			sequence_container<std::remove_cvref_t<Seq>> &&
			std::is_same_v<std::ranges::range_value_t<std::remove_cvref_t<Seq>>, generic_value>
		)
		auto operator()(Seq* seq, value_visitation_context context)
		{
			auto const container_size = std::size(*seq);

			m_nodes_to_visit.push(
					node{
							.value = end_of_array<src_value>{},
							.context = context
					}
			);

			for (auto&& [index, item] : std::ranges::reverse_view{std::ranges::enumerate_view{*seq}})
			{
				m_nodes_to_visit.push(
					node{
						.value = make_node_value(item.get_value()),
						.context = value_visitation_context{
							.node_index = static_cast<size_t>(index),
							.parent_container_size = container_size
						}
					}
				);
			}

			m_nodes_to_visit.push(
				node{
					.value = begin_of_array<src_value>{},
					.context = context
				}
			);

			if constexpr(Visitor::is_suspendable)
			{ return visitor_status::keep_going; }
		}

		template<class Seq>
		requires(
			sequence_container<std::remove_cvref_t<Seq>> &&
			std::is_same_v<std::ranges::range_value_t<std::remove_cvref_t<Seq>>, object>
		)
		auto operator()(Seq* seq, value_visitation_context context)
		{
			auto const container_size = std::size(*seq);

			m_nodes_to_visit.push(
				node{
						.value = end_of_array<src_object>{},
						.context = context
				}
			);

			for (auto&& [index, item] : std::ranges::reverse_view{std::ranges::enumerate_view{*seq}})
			{
				operator()(
					&item,
					value_visitation_context{
							.node_index = static_cast<size_t>(index),
							.parent_container_size = container_size
					}
				);
			}

			m_nodes_to_visit.push(
				node{
					.value = begin_of_array<src_object>{},
					.context = context
				}
			);

			if constexpr(Visitor::is_suspendable)
			{ return visitor_status::keep_going; }
		}

		auto operator()(begin_of_object /*unused*/, value_visitation_context context)
		{ return m_visitor.handle_begin_of_object(context); }

		auto operator()(end_of_object /*unused*/, value_visitation_context context)
		{ return m_visitor.handle_end_of_object(context); }

		template<class T>
		auto operator()(begin_of_array<T> /*unused*/, value_visitation_context context)
		{ return m_visitor.handle_begin_of_array(std::type_identity<T>{}, context); }

		template<class T>
		auto operator()(end_of_array<T> /*unused*/, value_visitation_context context)
		{ return m_visitor.handle_end_of_array(std::type_identity<T>{}, context); }

	private:
		Visitor m_visitor;
		std::stack<node> m_nodes_to_visit;
	};

	template<class GenericValue, class VisitorType>
	explicit node_visitor(GenericValue&, VisitorType&&)->node_visitor<GenericValue, VisitorType>;

	template<class GenericValue, class VisitorType>
	void visit_nodes(GenericValue&& root, VisitorType&& visitor)
	{
		node_visitor node_visitor{root, std::forward<VisitorType>(visitor)};
		if constexpr(std::remove_cvref_t<VisitorType>::is_suspendable)
		{ while(node_visitor.visit_nodes() == visitor_status::suspend){} }
		else
		{ node_visitor.visit_nodes(); }
	}

	template<class GenericValue, class VisitorType, class... VisitorArgs>
	auto visit_nodes(
		GenericValue&& root,
		std::in_place_type_t<VisitorType> /*unused*/,
		VisitorArgs&&... visitor_args
	)
	{
		node_visitor<GenericValue,VisitorType> node_visitor{
			root,
			std::in_place_t{},
			std::forward<VisitorArgs>(visitor_args)...
		};
		if constexpr(std::remove_cvref_t<VisitorType>::is_suspendable)
		{ while(node_visitor.visit_nodes() == visitor_status::suspend){} }
		else
		{ node_visitor.visit_nodes(); }
	}

	template<class Lhs, class Rhs>
	struct clone_visitor_value_update_traits_impl
	{
		UPDATE_CALLBACK static Rhs* update(Lhs& lhs, update_param_t<Rhs> rhs)
		{
			static_assert(std::is_constructible_v<Lhs, Rhs>);
			lhs = Lhs{maybe_move(rhs)};
			return lhs.template get_if<Rhs>();
		}
	};

	template<class Lhs, class Rhs>
	struct clone_visitor_object_update_traits_impl
	{
		[[noreturn]] UPDATE_CALLBACK static Rhs* update(Lhs& /*unused*/, update_param_t<Rhs> /*unused*/)
		{ raise_internal_error("Cannot assign a value to an object"); }
	};

	template<class OutputArray, class TypeToStore>
	struct clone_visitor_array_update_traits_impl
	{
		UPDATE_CALLBACK static TypeToStore* update(OutputArray& out, update_param_t<TypeToStore> val)
		{
			using output_value_type = typename OutputArray::value_type;
			if constexpr(
				   std::is_constructible_v<output_value_type, TypeToStore>
				|| std::is_same_v<output_value_type, TypeToStore>
			)
			{
				out.emplace_back(maybe_move(val));
				if constexpr(requires{{out.back().template get_if<TypeToStore>()};})
				{ return out.back().template get_if<TypeToStore>(); }
				else
				{ return &out.back(); }
			}
			else
			{ raise_internal_error("Type mismatch at array emplace_back"); }
		}
	};

	template<class T>
	struct clone_visitor_update_result
	{ using type = T*; };

	template<class GenericValueOut>
	struct clone_visitor_update_result<std::pair<typename GenericValueOut::key_type, GenericValueOut>>
	{
		using type = GenericValueOut*;
	};

	template<class SrcValueTemplateParamPack, class GenericValueOut>
	class clone_visitor
	{
	public:
		static constexpr bool is_suspendable = false;

		using kv_item = std::pair<typename GenericValueOut::key_type, GenericValueOut>;
		using object_out = typename GenericValueOut::object;

		template<class... SrcValueTypes>
		struct clone_visitor_value_update_traits:
			clone_visitor_value_update_traits_impl<GenericValueOut, SrcValueTypes>...
		{
			using clone_visitor_value_update_traits_impl<GenericValueOut, SrcValueTypes>::update...;

			UPDATE_CALLBACK static auto update(GenericValueOut& lhs, update_param_t<kv_item> item)
			{
				auto retval = lhs.try_store_key_value(maybe_move(item)).value;
				assert(retval != nullptr);
				return retval;
			}
		};

		template<class... SrcValueTypes>
		struct clone_visitor_object_update_traits:
			clone_visitor_object_update_traits_impl<object_out, SrcValueTypes>...
		{
			using clone_visitor_object_update_traits_impl<object_out, SrcValueTypes>::update...;

			UPDATE_CALLBACK static auto update(object_out& lhs, update_param_t<kv_item> item)
			{
				auto const result = lhs.insert(maybe_move(item));
				return &result.first->second;
			}
		};

		template<class OutputArray, class... SrcValueTypes>
		struct clone_visitor_array_update_traits:
			clone_visitor_array_update_traits_impl<OutputArray, SrcValueTypes>...
		{
			using clone_visitor_array_update_traits_impl<OutputArray, SrcValueTypes>::update...;

			[[noreturn]] UPDATE_CALLBACK static GenericValueOut* update(OutputArray& /*unused*/, update_param_t<kv_item> item)
			{ raise_internal_error("Unexpected prop name {}", make_fmt_args(maybe_move(item.first))); }
		};

		template<class T>
		using sequence_container_out = typename GenericValueOut::template sequence_container_type<T>;

		using leaf_value_template_param_pack = SrcValueTemplateParamPack;

		using array_value_template_param_pack = concatenate_template_param_packs_t<
			wrap_template_param_pack_elements_t<
				leaf_value_template_param_pack, sequence_container_out
			>,
			template_param_pack<sequence_container_out<object_out>>,
			template_param_pack<sequence_container_out<GenericValueOut>>
		>;

		using complete_pack = concatenate_template_param_packs_t<
			SrcValueTemplateParamPack,
			template_param_pack<object_out>,
			array_value_template_param_pack
		>;

		using complete_pack_with_kv_item = append_to_template_param_pack_t<
			complete_pack,
			kv_item
		>;

		using output_value_update_traits = map_template_param_pack_to_type_t<
			clone_visitor_value_update_traits,
			complete_pack
		>;

		using output_object_update_traits = map_template_param_pack_to_type_t<
			clone_visitor_object_update_traits,
			complete_pack
		>;

		template<class OutputArray>
		using output_array_update_traits = map_template_param_pack_to_type_t<
			clone_visitor_array_update_traits,
			concatenate_template_param_packs_t<
				wrap_in_template_param_pack_t<OutputArray>,
				complete_pack
			>
		>;

		template<class T>
		using update_result_t = typename clone_visitor_update_result<T>::type;

		template<class ... Args>
		using value_storage_with_result = value_storage<update_result_t, Args...>;

		using value_value_storage = map_template_param_pack_to_type_t<
			value_storage_with_result,
			complete_pack_with_kv_item
		>;

		explicit clone_visitor(GenericValueOut& output_value)
		{
			m_contexts.push(
				context{
					.parent_node = {},
					.output_value = value_value_storage{
						output_value,
						std::type_identity<output_value_update_traits>{}
					}
				}
			);
		}

		template<class T>
		void handle_leaf_value(T&& value, value_visitation_context /*unused*/)
		{
			if(m_value_after_key != nullptr)
			{
				auto const val_ptr = m_value_after_key;
				m_value_after_key = nullptr;
				using convert_to = typename std::remove_cvref_t<std::remove_pointer_t<decltype(val_ptr)>>;
				*val_ptr = convert_to{std::forward<T>(value)};
			}
			else
			{
				auto _ = m_contexts.top().output_value.update_with(std::forward<T>(value));
			}
		}

		template<class T>
		void handle_property_name(T&& prop_name, value_visitation_context /*unused*/)
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

		void handle_begin_of_object(value_visitation_context /*unused*/)
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
						.output_value = value_value_storage{
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
						.output_value = value_value_storage{
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
						.output_value = value_value_storage{
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
						.output_value = value_value_storage{
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
						.output_value = value_value_storage{
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
						.output_value = value_value_storage{
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
			value_value_storage parent_node;
			value_value_storage output_value;
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
