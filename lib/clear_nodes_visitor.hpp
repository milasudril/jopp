#ifndef JOPP2_CLEAR_NODES_VISITOR_HPP
#define JOPP2_CLEAR_NODES_VISITOR_HPP

#include "./node_visitor_adaptor.hpp"

namespace jopp2
{
	struct clear_nodes_visitor
	{
		template<class T>
		node_visitor_status handle_leaf_value(
			T&& /*unused*/,
			jopp2::value_visitation_context const& /*unused*/
		)
		{ return node_visitor_status::ready; }

		template<class T>
		node_visitor_status handle_simple_array(
			T& obj,
			jopp2::value_visitation_context const& /*unused*/
		)
		{
			obj.clear();
			return node_visitor_status::ready;
		}

		template<class T>
		node_visitor_status handle_begin_of_container(
			T&& /*unused*/,
			jopp2::value_visitation_context const& /*unused*/
		)
		{ return node_visitor_status::ready; }

		template<class T>
		node_visitor_status handle_end_of_container(
			T& obj,
			jopp2::value_visitation_context const& /*unused*/
		)
		{
			obj.clear();
			return node_visitor_status::ready;
		}

		template<class T>
		node_visitor_status handle_key(
			T&& /*unused*/,
			jopp2::value_visitation_context const& /*unused*/
		)
		{ return node_visitor_status::ready; }
	};
}

#endif
