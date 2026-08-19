#ifndef JOPP_TREEWALKER_HPP
#define JOPP_TREEWALKER_HPP

#include "./variant_utils.hpp"
#include "./value_storage.hpp"
#include "./container_proxy.hpp"
#include "./utils.hpp"

#include <type_traits>
#include <ranges>
#include <vector>

namespace jopp2
{
	using jopp::instance_of;

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

	class value_visitation_context
	{
	public:
		value_visitation_context() = default;

		constexpr void step_node_index()
		{ ++m_node_index; }

		constexpr bool is_last_node() const
		{ return m_node_index == m_parent_container_size - 1; }

		constexpr bool is_first_node () const
		{ return m_node_index == 0; }

		constexpr size_t depth() const
		{ return m_depth; }

		constexpr value_visitation_context next_level(size_t parent_container_size) const
		{
			return value_visitation_context{
				parent_container_size,
				m_depth + 1
			};
		}

	private:
		constexpr explicit
		// NOLINTNEXTLINE
		value_visitation_context(size_t parent_container_size, size_t depth):
			m_node_index{0},
			m_parent_container_size{parent_container_size},
			m_depth{depth}
		{}

		size_t m_node_index{};
		size_t m_parent_container_size{};
		size_t m_depth{};
	};

	enum class node_visitor_status {
		ready,
		suspended
	};

	template<class GenericValue, class NodeVisitor>
	class tree_walker
	{
		enum class visit_node_result{
			node_visitor_ready = node_visitor_status::ready,
			node_visitor_suspended = node_visitor_status::suspended,
			completed
		};

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
			value_visitation_context context;
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

		template<class NodeVisitorType>
		explicit tree_walker(GenericValue& root, NodeVisitorType&& visitor):
			m_visitor{std::forward<NodeVisitorType>(visitor)}
		{
			m_nodes.reserve(1024);
			m_nodes.push_back(
				node{
					.value = make_node_value(root.get_value()),
					.context = {}
				}
			);
		}

		template<class ... NodeVisitorArgs>
		explicit tree_walker(GenericValue& root, std::in_place_t /*unused*/, NodeVisitorArgs&&... args):
			m_visitor{std::forward<NodeVisitorArgs>(args)...}
		{
			m_nodes.reserve(1024);
			m_nodes.push_back(
				node{
					.value = make_node_value(root.get_value()),
					.context = {}
				}
			);
		}

		[[nodiscard]] auto visit_nodes()
		{
			while(!m_nodes.empty())
			{
				auto& current_node = m_nodes.back();
				switch(visit_with_args(current_node.value, *this, current_node.context))
				{
					case visit_node_result::node_visitor_ready:
						break;
					case visit_node_result::node_visitor_suspended:
						return node_visitor_status::suspended;
					case visit_node_result::completed:
						m_nodes.pop_back();
				}
			}
			return node_visitor_status::ready;
		}

		template<class T>
		requires(generic_value_t::template is_leaf_value<std::remove_cvref_t<T>>)
		visit_node_result dispatch(
			callback_param_t<std::remove_cvref_t<T>> item,
			value_visitation_context const& current_context
		)
		{ return m_visitor.handle_leaf_value(item, current_context); }

		template<class T>
		requires instance_of<std::remove_cvref_t<T>, container_proxy>
		visit_node_result dispatch(T& item, value_visitation_context const& current_context)
		{
			return m_visitor.handle_leaf_value_array(item, current_context);
		}

		visit_node_result dispatch(
			container_proxy<sequence_container_type<generic_value_t>>& obj,
			value_visitation_context& current_context
		)
		{
			if(obj.at_begin())
			{
				if(m_visitor.handle_begin_of_container(obj, std::as_const(current_context)) == node_visitor_status::suspended)
				{ return visit_node_result::node_visitor_suspended; }
			}

			if(obj.at_end())
			{
				if(m_visitor.handle_end_of_container(obj, std::as_const(current_context)) == node_visitor_status::suspended)
				{ return visit_node_result::node_visitor_suspended; }
				return visit_node_result::completed;
			}

			auto& next_item = obj.active_range().begin()->get_value();
			obj.pop_active_element();
			current_context.step_node_index();

			m_nodes.push_back(
				node{
					.value = make_node_value(next_item),
					.context = current_context.next_level(obj.total_size())
				}
			);

			return visit_node_result::node_visitor_ready;
		}

		template<class T>
		requires instance_of<std::remove_cvref_t<T>, container_proxy>
		&& std::ranges::range<typename std::remove_cvref_t<T>::value_type>
		visit_node_result dispatch(
			T& obj,
			value_visitation_context& current_context
		)
		{
			using next_level = typename std::remove_cvref_t<T>::value_type;
			if(obj.at_begin())
			{
				if(m_visitor.handle_begin_of_container(obj, std::as_const(current_context)) == node_visitor_status::suspended)
				{ return visit_node_result::node_visitor_suspended; }
			}

			if(obj.at_end())
			{
				if(m_visitor.handle_end_of_container(obj, std::as_const(current_context)) == node_visitor_status::suspended)
				{ return visit_node_result::node_visitor_suspended; }
				return visit_node_result::completed;
			}

			auto& next_item = *obj.active_range().begin();
			obj.pop_active_element();

			m_nodes.push_back(
				node{
					.value = make_node_value(next_item),
					.context = current_context.next_level(obj.total_size())
				}
			);

			return visit_node_result::node_visitor_ready;
		}

		visit_node_result dispatch(
			container_proxy<objcontainer>& obj,
			value_visitation_context& current_context
		)
		{
			if(obj.at_begin())
			{
				if(m_visitor.handle_begin_of_container(obj, std::as_const(current_context)) == node_visitor_status::suspended)
				{ return visit_node_result::node_visitor_suspended; }
			}

			if(obj.at_end())
			{
				if(m_visitor.handle_end_of_container(obj, std::as_const(current_context)) == node_visitor_status::suspended)
				{ return visit_node_result::node_visitor_suspended; }
				return visit_node_result::completed;
			}

			if(m_visitor.handle_key(obj.active_range().begin()->first, current_context) == node_visitor_status::suspended)
			{ return visit_node_result::node_visitor_suspended; }

			auto& next_item = obj.active_range().begin()->second.get_value();
			obj.pop_active_element();

			m_nodes.push_back(
				node{
					.value = make_node_value(next_item),
					.context = current_context.next_level(obj.total_size())
				}
			);

			return visit_node_result::node_visitor_ready;
		}

		template<class T>
		requires(generic_value_t::template is_leaf_value<std::remove_cvref_t<T>>)
		[[gnu::always_inline]] visit_node_result operator()(
			std::reference_wrapper<T> item,
			value_visitation_context& context
		)
		{ return dispatch<T>(item.get(), context); }

		template<class T>
		[[gnu::always_inline]] visit_node_result operator()(T&& item, value_visitation_context& context)
		{ return dispatch<std::remove_cvref_t<T>>(std::forward<T>(item), context); }

		[[gnu::always_inline]]
		visit_node_result operator()(
			container_proxy<sequence_container_type<generic_value_t>>& item,
			value_visitation_context& context
		)
		{ return dispatch(item, context); }

		[[gnu::always_inline]] visit_node_result operator()(
			container_proxy<objcontainer>& item,
			value_visitation_context& context
		)
		{ return dispatch(item, context); }

		size_t current_depth() const
		{ return std::size(m_nodes); }

	private:
		NodeVisitor m_visitor;
		std::vector<node> m_nodes;
	};
}
#endif
