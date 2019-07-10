#ifndef GRAPH_CANON_EDGE_HANDLER_EDGE_HANDLER_HPP
#define GRAPH_CANON_EDGE_HANDLER_EDGE_HANDLER_HPP

namespace graph_canon {

// rst: .. concept:: template<typename T> EdgeHandlerCreator
// rst:
// rst:		An edge handler creator is an object that from a given integer type instantiates the actual edge handler object.
// rst:
// rst:		.. notation::
// rst:
// rst:		.. type:: SizeType
// rst:
// rst:			An integer type.
// rst:
// rst:		.. assoc_types::
// rst:
// rst:		.. type:: EdgeHandlerT = typename T::template type<SizeType>
// rst:
// rst:			A type satisfying the `EdgeHandler` concept.
// rst:
// rst:		.. valid_expr::
// rst:
// rst:		`T::make<SizeType>()`: return an object of type `EdgeHandlerT`.
// rst:

// rst: .. concept:: template<typename T> EdgeHandler
// rst:
// rst:		An edge handler is an object that implements procedures related to labels on edges.
// rst:		If no labels are present, the handler creator `edge_handler_all_equal` can be used.
// rst:
// rst:		.. notation::
// rst:
// rst:		.. var:: T edge_handler
// rst:		.. type:: State
// rst:
// rst:			A specialization of `canon_state`.
// rst:
// rst:		.. type:: Edge = typename boost::graph_traits<typename State::Graph>::edge_descriptor
// rst:		.. var:: State state
// rst:		.. var:: Edge e_left
// rst:		         Edge e_right
// rst:
// rst:		.. valid_expr::
// rst:
// rst: 		- `edge_handler.initialize(state)`. Is executed in the beginning of canonicalization.
// rst:		- `edge_handler.compare(state, e_left, e_right)` must compare the two edges
// rst:		  (e.g., their labels) and return an integer representing their order:
// rst:
// rst:		  - A negative number: `e_left` is ordered before `e_right`.
// rst:		  - Zero: `e_left` and `e_right` are considered equal.
// rst:		  - A positive number: `e_left` is ordered after `e_right`.
// rst:
// rst: 		  The function is at least used when creating ordered graphs in the leaves of the search tree,
// rst: 		  but may also be used by visitors.
// rst:		  Note that the function should not compare the end-points, only auxiliary data.
// rst:
// rst:		.. todo:: List requirements from the WL-1 refiner.
// rst:

} // namespace graph_canon

#endif /* GRAPH_CANON_EDGE_HANDLER_EDGE_HANDLER_HPP */