#ifndef JOPP_NODE_VISITOR_ADAPTOR_HPP
#define JOPP_NODE_VISITOR_ADAPTOR_HPP

#include "./variant_utils.hpp"
#include "./value_storage.hpp"
#include "./container_proxy.hpp"
#include "./utils.hpp"
#include "./exception.hpp"

#include <type_traits>
#include <ranges>
#include <vector>
#include <string>
#include <format>

namespace jopp2
{
	struct value_visitation_context
	{
		constexpr bool operator==(value_visitation_context const&) const = default;
		constexpr bool operator!=(value_visitation_context const&) const = default;

		size_t node_index{};
		size_t parent_container_size{};
		size_t depth{};
	};

	enum class node_visitor_status {
		ready,
		suspended
	};

	enum class visit_node_result{
		node_visitor_ready = static_cast<int>(node_visitor_status::ready),
		node_visitor_suspended = static_cast<int>(node_visitor_status::suspended),
		completed
	};

	using jopp::instance_of;

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

	template<class GenericValue, class NodeVisitor>
	class node_visitor_adaptor
	{
	public:
		static_assert(std::is_reference_v<GenericValue>);

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

		template<class Other, class... Args>
		node_visitor_adaptor(Other&&, Args&&...) = delete;

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
		requires(!std::is_same_v<std::remove_cvref_t<NodeVisitorType>, node_visitor_adaptor>)
		explicit node_visitor_adaptor(NodeVisitorType&& visitor):
			m_visitor{std::forward<NodeVisitorType>(visitor)}
		{ }

		template<class NodeVisitorType, class ... NodeVisitorArgs>
		explicit node_visitor_adaptor(
			std::in_place_type_t<NodeVisitorType> /*unused*/,
			NodeVisitorArgs&&... args
		):
			m_visitor{std::forward<NodeVisitorArgs>(args)...}
		{ }

		template<class T>
		requires(generic_value_t::template is_leaf_value<std::remove_cvref_t<T>>)
		visit_node_result dispatch(
			callback_param_t<std::remove_cvref_t<T>> item,
			value_visitation_context const& current_context
		)
		{
			switch(m_visitor.handle_leaf_value(item, current_context))
			{
				case node_visitor_status::ready:
					return visit_node_result::completed;
				case node_visitor_status::suspended:
					return visit_node_result::node_visitor_suspended;
				default:
					raise_internal_error("Invalid return value from node visitor");
			}
		}

		template<class T>
		requires instance_of<std::remove_cvref_t<T>, container_proxy>
		visit_node_result dispatch(T& item, value_visitation_context const& current_context)
		{
			switch(m_visitor.handle_leaf_value_array(item, current_context))
			{
				case node_visitor_status::ready:
					if(item.at_end())
					{ return visit_node_result::completed; }
					else
					{ return visit_node_result::node_visitor_ready; }

				case node_visitor_status::suspended:
					return visit_node_result::node_visitor_suspended;

				default:
					raise_internal_error("Invalid return value from node visitor");
			}
		}

		visit_node_result dispatch(
			container_proxy<sequence_container_type<generic_value_t>>& obj,
			value_visitation_context const& current_context,
			std::vector<node>& nodes
		)
		{
			if(obj.at_begin())
			{
				if(m_visitor.handle_begin_of_container(obj, std::as_const(current_context)) == node_visitor_status::suspended)
				{ return visit_node_result::node_visitor_suspended; }

				if(obj.empty())
				{ obj.pop_active_element(); }
			}

			if(obj.at_end())
			{
				if(m_visitor.handle_end_of_container(obj, std::as_const(current_context)) == node_visitor_status::suspended)
				{ return visit_node_result::node_visitor_suspended; }
				return visit_node_result::completed;
			}

			auto& next_item = obj.active_range().begin()->get_value();
			value_visitation_context const next_context{
				.node_index = obj.cursor_offest(),
				.parent_container_size = obj.total_size(),
				.depth = current_context.depth + 1
			};
			obj.pop_active_element();

			nodes.push_back(
				node{
					.value = make_node_value(next_item),
					.context = next_context
				}
			);

			return visit_node_result::node_visitor_ready;
		}

		template<class T>
		requires instance_of<std::remove_cvref_t<T>, container_proxy>
		&& std::ranges::range<typename std::remove_cvref_t<T>::value_type>
		visit_node_result dispatch(
			T& obj,
			value_visitation_context const& current_context,
			std::vector<node>& nodes
		)
		{
			using next_level = typename std::remove_cvref_t<T>::value_type;
			if(obj.at_begin())
			{
				if(m_visitor.handle_begin_of_container(obj, std::as_const(current_context)) == node_visitor_status::suspended)
				{ return visit_node_result::node_visitor_suspended; }

				if(obj.empty())
				{ obj.pop_active_element(); }
			}

			if(obj.at_end())
			{
				if(m_visitor.handle_end_of_container(obj, std::as_const(current_context)) == node_visitor_status::suspended)
				{ return visit_node_result::node_visitor_suspended; }
				return visit_node_result::completed;
			}

			auto& next_item = *obj.active_range().begin();
			value_visitation_context const next_context{
				.node_index = obj.cursor_offest(),
				.parent_container_size = obj.total_size(),
				.depth = current_context.depth + 1
			};
			obj.pop_active_element();

			nodes.push_back(
				node{
					.value = node_value{node_item<next_level, src_is_const>::create(next_item)},
					.context = next_context
				}
			);

			return visit_node_result::node_visitor_ready;
		}

		visit_node_result dispatch(
			container_proxy<objcontainer>& obj,
			value_visitation_context const& current_context,
			std::vector<node>& nodes
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
/*
			TODO: Need to push key as well...
			if(m_visitor.handle_key(obj.active_range().begin()->first, std::as_const(current_context)) == node_visitor_status::suspended)
			{ return visit_node_result::node_visitor_suspended; }
*/

			auto& next_item = obj.active_range().begin()->second.get_value();
			value_visitation_context const next_context{
				.node_index = obj.cursor_offest(),
				.parent_container_size = obj.total_size(),
				.depth = current_context.depth + 1
			};
			obj.pop_active_element();

			nodes.push_back(
				node{
					.value = make_node_value(next_item),
					.context = next_context
				}
			);

			return visit_node_result::node_visitor_ready;
		}

		auto const& visitor() const
		{ return m_visitor; }

	private:
		NodeVisitor m_visitor;
	};

	template<class GenericValue, class NodeVisitorType, class ... Args>
	auto make_node_visitor_adaptor(Args&&... args)
	{
		return node_visitor_adaptor<GenericValue, NodeVisitorType>(
			std::in_place_type_t<NodeVisitorType>{},
			std::forward<Args>(args)...
		);
	}

	template<class GenericValue, class NodeVisitorType>
	auto make_node_visitor_adaptor(NodeVisitorType&& node_visitor)
	{
		return node_visitor_adaptor<GenericValue, NodeVisitorType>(
			std::forward<NodeVisitorType>(node_visitor)
		);
	}
}
#endif
