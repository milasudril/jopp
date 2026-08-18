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
			m_nodes.reserve(1024);
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
			m_nodes.reserve(1024);
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
			printf("(scalar) %s\n", typeid(T).name());
			return 0;
		}

		template<class T>
		requires instance_of<std::remove_cvref_t<T>, container_proxy>
		size_t dispatch(T& /*TODO*/)
		{
			printf("(array) %s\n", typeid(T).name());
			return 0;
		}

		size_t dispatch(container_proxy<sequence_container_type<generic_value_t>>& obj)
		{
			if(obj.at_begin())
			{
				puts("[");
			}

			if(obj.at_end())
			{
				puts("]");
				return 0;
			}

			auto& next_item = obj.active_range().begin()->get_value();
			obj.pop_active_element();

			m_nodes.push_back(
				node{
					.value = make_node_value(next_item)
				}
			);

			return 1;
		}

		template<class T>
		requires instance_of<std::remove_cvref_t<T>, container_proxy>
		&& std::ranges::range<typename std::remove_cvref_t<T>::value_type>
		size_t dispatch(T& obj)
		{
			using next_level = typename std::remove_cvref_t<T>::value_type;
			if(obj.at_begin())
			{
				puts("[");
			}

			if(obj.at_end())
			{
				puts("]");
				return 0;
			}

			auto& next_item = *obj.active_range().begin();
			obj.pop_active_element();

			m_nodes.push_back(
				node{
					.value = node_value{node_item<next_level, src_is_const>::create(next_item)}
				}
			);
			return 1;

		}

		size_t dispatch(container_proxy<objcontainer>& obj)
		{
			if(obj.at_begin())
			{ puts("{"); }

			if(obj.at_end())
			{
				puts("}");
				return 0;
			}

			printf("%s: ", obj.active_range().begin()->first.c_str());

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

		[[gnu::always_inline]] size_t operator()(container_proxy<objcontainer>& item)
		{ return dispatch(item); }

	private:
		Visitor m_visitor;
		std::vector<node> m_nodes;
	};
}
#endif
