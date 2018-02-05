#ifndef GRAPH_CANON_COMPARE_HPP
#define GRAPH_CANON_COMPARE_HPP

#include <graph_canon/ordered_graph.hpp>

namespace graph_canon {

// rst: .. function:: template<typename GraphL, typename IndexMapL, bool WithInEdgesL, \
// rst:                  typename GraphR, typename IndexMapR, bool WithInEdgesR, \
// rst:                  typename VertexEqual, typename EdgeEqual, \
// rst:                  typename Visitor> \
// rst:               bool ordered_graph_equal( \
// rst:                  const ordered_graph<GraphL, IndexMapL, WithInEdgesL> &g_left, \
// rst:                  const ordered_graph<GraphR, IndexMapR, WithInEdgesR> &g_right, \
// rst:                  VertexEqual vertex_equal, EdgeEqual edge_equal, Visitor &&visitor)
// rst:
// rst:		Compare two ordered graphs for equality.
// rst:		For labelled graphs the two binary predicates `vertex_equal` and `edge_equal`,
// rst:		can be used to compare labels.
// rst:		If the two graphs are not equal the reason is indicated by calling a corresponding
// rst:		member function on `visitor`.
// rst:		See `graph_compare_null_visitor` for the list of member functions that must exist.
// rst:

template<typename GraphL, typename IndexMapL, bool WithInEdgesL,
/**/ typename GraphR, typename IndexMapR, bool WithInEdgesR,
/**/ typename VertexEqual, typename EdgeEqual,
/**/ typename Visitor
>
bool ordered_graph_equal(
		const ordered_graph<GraphL, IndexMapL, WithInEdgesL> &g_left,
		const ordered_graph<GraphR, IndexMapR, WithInEdgesR> &g_right,
		VertexEqual vertex_equal, EdgeEqual edge_equal, Visitor &&visitor) {
	using TraitsL = boost::graph_traits<ordered_graph<GraphL, IndexMapL, WithInEdgesL> >;
	using TraitsR = boost::graph_traits<ordered_graph<GraphR, IndexMapR, WithInEdgesR> >;
	const auto n_left = num_vertices(g_left);
	const auto n_right = num_vertices(g_right);
	if(n_left != n_right) {
		visitor.at_num_vertices();
		return false;
	}
	typename TraitsL::vertex_iterator v_iter_left, v_iter_left_end;
	typename TraitsR::vertex_iterator v_iter_right = vertices(g_right).first;
	for(boost::tie(v_iter_left, v_iter_left_end) = vertices(g_left);
			v_iter_left != v_iter_left_end;
			++v_iter_left, ++v_iter_right) {
		if(!vertex_equal(*v_iter_left, *v_iter_right)) {
			visitor.at_vertex_compare(*v_iter_left, *v_iter_right);
			return false;
		}
		const auto d_left = out_degree(*v_iter_left, g_left);
		const auto d_right = out_degree(*v_iter_right, g_right);
		if(d_left != d_right) {
			visitor.at_out_degree(*v_iter_left, *v_iter_right);
			return false;
		}
		typename TraitsL::out_edge_iterator e_iter_left, e_iter_left_end;
		typename TraitsR::out_edge_iterator e_iter_right = out_edges(*v_iter_right, g_right).first;
		for(boost::tie(e_iter_left, e_iter_left_end) = out_edges(*v_iter_left, g_left);
				e_iter_left != e_iter_left_end;
				++e_iter_left, ++e_iter_right) {
			const auto v_idx_left = get(g_left.data.idx, target(*e_iter_left, g_left));
			const auto v_idx_right = get(g_right.data.idx, target(*e_iter_right, g_right));
			if(v_idx_left != v_idx_right) {
				visitor.at_out_edge(*e_iter_left, *e_iter_right);
				return false;
			}
			if(!edge_equal(*e_iter_left, *e_iter_right)) {
				visitor.at_edge_compare(*e_iter_left, *e_iter_right);
				return false;
			}
		}
	}
	return true;
}

// rst: .. function:: template<typename GraphL, typename IndexMapL, bool WithInEdgesL, \
// rst:                  typename GraphR, typename IndexMapR, bool WithInEdgesR, \
// rst:                  typename VertexLess, typename EdgeLess, \
// rst:                  typename VertexEqual, typename EdgeEqual> \
// rst:               bool ordered_graph_less( \
// rst:                  const ordered_graph<GraphL, IndexMapL, WithInEdgesL> &g_left, \
// rst:                  const ordered_graph<GraphR, IndexMapR, WithInEdgesR> &g_right, \
// rst:                  VertexLess vertex_less, EdgeLess edge_less, \
// rst:                  VertexEqual vertex_equal, EdgeEqual edge_equal)
// rst:
// rst:		Compare two ordered graphs and return `true` when `g_left` is considered smaller than `g_right`.
// rst:		For labelled graphs the binary predicates `vertex_less`, `edge_less`, `vertex_equal`, and `edge_equal`,
// rst:		can be used to compare labels. The ``_equal`` and ``_less`` versions must be consistent with each other.
// rst:

template<typename GraphL, typename IndexMapL, bool WithInEdgesL,
/**/ typename GraphR, typename IndexMapR, bool WithInEdgesR,
/**/ typename VertexLess, typename EdgeLess,
/**/ typename VertexEqual, typename EdgeEqual
>
bool ordered_graph_less(
		const ordered_graph<GraphL, IndexMapL, WithInEdgesL> &g_left,
		const ordered_graph<GraphR, IndexMapR, WithInEdgesR> &g_right,
		VertexLess vertex_less, EdgeLess edge_less,
		VertexEqual vertex_equal, EdgeEqual edge_equal) {
	using TraitsL = boost::graph_traits<ordered_graph<GraphL, IndexMapL, WithInEdgesL> >;
	using TraitsR = boost::graph_traits<ordered_graph<GraphR, IndexMapR, WithInEdgesR> >;
	const auto n_left = num_vertices(g_left);
	const auto n_right = num_vertices(g_right);
	if(n_left != n_right) return n_left < n_right;
	typename TraitsL::vertex_iterator v_iter_left, v_iter_left_end;
	typename TraitsR::vertex_iterator v_iter_right;
	boost::tie(v_iter_left, v_iter_left_end) = vertices(g_left);
	v_iter_right = vertices(g_right).first;
	for(; v_iter_left != v_iter_left_end; ++v_iter_left, ++v_iter_right) {
		if(!vertex_equal(*v_iter_left, *v_iter_right)) return vertex_less(*v_iter_left, *v_iter_right);
		const auto d_left = out_degree(*v_iter_left, g_left);
		const auto d_right = out_degree(*v_iter_right, g_right);
		if(d_left != d_right) return d_left < d_right;
		typename TraitsL::out_edge_iterator e_iter_left, e_iter_left_end;
		typename TraitsR::out_edge_iterator e_iter_right;
		boost::tie(e_iter_left, e_iter_left_end) = out_edges(*v_iter_left, g_left);
		e_iter_right = out_edges(*v_iter_right, g_right).first;
		for(; e_iter_left != e_iter_left_end; ++e_iter_left, ++e_iter_right) {
			const auto v_idx_left = get(g_left.data.idx, target(*e_iter_left, g_left));
			const auto v_idx_right = get(g_right.data.idx, target(*e_iter_right, g_right));
			if(v_idx_left != v_idx_right) return v_idx_left < v_idx_right;
			if(!edge_equal(*e_iter_left, *e_iter_right)) return edge_less(*e_iter_left, *e_iter_right);
		}
	}
	return false;
}

// rst: .. class:: graph_compare_null_visitor
// rst:
// rst:		A visitor class that can be used for `ordered_graph_equal`.
// rst:

struct graph_compare_null_visitor {
	// rst:		.. function:: void at_num_vertices() const
	// rst:
	// rst:			Called when the two graphs have different number of vertices.

	void at_num_vertices() const { }

	// rst:		.. function:: template<typename VertexL, typename VertexR> \
	// rst:		              void at_vertex_compare(VertexL, VertexR) const 
	// rst:
	// rst:			Called when the user-defined vertex predicate returns `false` for two vertices with the same index.

	template<typename VertexL, typename VertexR>
	void at_vertex_compare(VertexL, VertexR) const { }

	// rst:		.. function:: template<typename VertexL, typename VertexR> \
	// rst:		              void at_out_degree(VertexL, VertexR) const 
	// rst:
	// rst:			Called when two verices with the same index have different out-degrees.

	template<typename VertexL, typename VertexR>
	void at_out_degree(VertexL, VertexR) const { }

	// rst:		.. function:: template<typename EdgeL, typename EdgeR> \
	// rst:		              void at_out_edge(EdgeL, EdgeR) const
	// rst:
	// rst:			Called when two edges :math:`(u_l, v_l)`, :math:`(u_r, v_r)` from the two graphs, with :math:`u_l` and :math:`u_r` have the same index,
	// rst:			the edges are in the same position in their respective out-edge lists, but :math:`v_l` and :math:`v_r` have different index.

	template<typename EdgeL, typename EdgeR>
	void at_out_edge(EdgeL, EdgeR) const { }

	// rst:		.. function:: template<typename EdgeL, typename EdgeR> \
	// rst:		              void at_edge_compare(EdgeL, EdgeR) const
	// rst:
	// rst:			Called when two edges :math:`(u_l, v_l)`, :math:`(u_r, v_r)` from the two graphs, with :math:`u_l` and :math:`u_r` have the same index,
	// rst:			the edges are in the same position in their respective out-edge lists, :math:`v_l` and :math:`v_r` have the same index,
	// rst:			but the user-defined predicate returns `false`.

	template<typename EdgeL, typename EdgeR>
	void at_edge_compare(EdgeL, EdgeR) const { }
};

} // namespace graph_canon

#endif /* GRAPH_CANON_COMPARE_HPP */