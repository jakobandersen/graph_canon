#ifndef GRAPH_CANON_SHORTHANDS_HPP
#define GRAPH_CANON_SHORTHANDS_HPP

#include <graph_canon/canonicalization.hpp>
#include <graph_canon/aut/implicit_size_2.hpp>
#include <graph_canon/aut/pruner_basic.hpp>
#include <graph_canon/invariant/cell_split.hpp>
#include <graph_canon/invariant/coordinator.hpp>
#include <graph_canon/invariant/partial_leaf.hpp>
#include <graph_canon/invariant/quotient.hpp>
#include <graph_canon/refine/WL_1.hpp>
#include <graph_canon/target_cell/flm.hpp>
#include <graph_canon/tree_traversal/bfs-exp.hpp>


#include <graph_canon/visitor/debug.hpp>


// rst: This header defines several overloads of the `canonicalize` function template
// rst: where various arguments have been defaulted.
// rst:

namespace graph_canon {

// rst: .. function:: auto make_default_visitor()
// rst:
// rst:		:returns: a compound visitor with the following visitors:
// rst:

auto make_default_visitor() {
	return make_visitor(
			// rst:		- `target_cell_flm`
			target_cell_flm(),
			// rst:		- `traversal_bfs_exp`
			traversal_bfs_exp(),
			// rst: 		- `refine_WL_1`
			refine_WL_1(),
			// rst:		- `aut_pruner_basic`
			aut_pruner_basic(),
			// rst:		- `aut_implicit_size_2`
			aut_implicit_size_2(),
			// rst:		- `invariant_cell_split`
			invariant_cell_split(),
			// rst:		- `invariant_partial_leaf`
			invariant_partial_leaf(),
			// rst:		- `invariant_quotient`
			invariant_quotient()
			);
}

// rst: .. function:: template<bool ParallelEdges, bool Loops, typename Graph, typename Visitor> \
// rst:               auto canonicalize(const Graph &graph, Visitor visitor)
// rst:
// rst:		A shorthand function for canonicalization, where only `ParallelEdges` and `Loops` must be specified.
// rst:
// rst:		- The size type used is `boost::graph_traits<Graph>::vertices_size_type`.
// rst:		- The index map used is `get(boost::vertex_index_t(), graph)`.
// rst:		- The vertices and edges are considered unlabelled, i.e., `always_false` is used as vertex-less predicate,
// rst:		  and `edge_handler_all_equal` is used as `EdgeHandlerCreator`.
// rst:
// rst:		See :expr:`canonicalizer::operator()`.

template<bool ParallelEdges, bool Loops, typename Graph, typename Visitor>
auto canonicalize(const Graph &graph, Visitor visitor) {
	return canonicalize<ParallelEdges, Loops>(graph, always_false(), visitor);
}

// rst: .. function:: template<bool ParallelEdges, bool Loops, typename Graph, typename VertexLess, typename Visitor> \
// rst:               auto canonicalize(const Graph &graph, VertexLess vertex_less, Visitor visitor)
// rst:
// rst:		A shorthand function for canonicalization, where only `ParallelEdges`, `Loops`, and `VertexLess` must be specified.
// rst:
// rst:		- The size type used is `boost::graph_traits<Graph>::vertices_size_type`.
// rst:		- The index map used is `get(boost::vertex_index_t(), graph)`.
// rst:		- The edges are considered unlabelled, i.e., `edge_handler_all_equal` is used as `EdgeHandlerCreator`.
// rst:
// rst:		See :expr:`canonicalizer::operator()`.

template<bool ParallelEdges, bool Loops, typename Graph, typename VertexLess, typename Visitor>
auto canonicalize(const Graph &graph, VertexLess vertex_less, Visitor visitor) {
	using SizeType = typename boost::graph_traits<Graph>::vertices_size_type;
	return canonicalize<SizeType, ParallelEdges, Loops>(graph, get(boost::vertex_index_t(), graph), vertex_less, edge_handler_all_equal(), visitor);
}

} // namespace graph_canon

#endif /* GRAPH_CANON_SHORTHANDS_HPP */