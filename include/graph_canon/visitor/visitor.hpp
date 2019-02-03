#ifndef GRAPH_CANON_VISITOR_VISITOR_HPP
#define GRAPH_CANON_VISITOR_VISITOR_HPP

#include <graph_canon/tagged_list.hpp>
#include <graph_canon/refine/refine.hpp>

namespace graph_canon {

// rst: .. class:: no_result
// rst:
// rst:		A helper-class to derive visitors from that do not want to return data.
// rst:

struct no_result {
	// rst:		.. function:: template<typename State> \
	// rst:		              tagged_list<> extract_result(const State&)
	// rst:
	// rst:			:returns: `tagged_list<>()`

	template<typename State>
	tagged_list<> extract_result(const State&) {
		return {};
	}
};


// rst: .. class:: no_instance_data
// rst:
// rst:		A helper-class to derive visitors from that do not need extra instance-wide data.
// rst:

struct no_instance_data {
	// rst:		.. class:: template<typename Config, typename TreeNode> \
	// rst:		           InstanceData
	// rst:

	template<typename Config, typename TreeNode>
	struct InstanceData {
		// rst:			.. type:: type = tagged_list<>
		using type = tagged_list<>;
	};
};



// rst: .. class:: no_tree_node_data
// rst:
// rst:		A helper-class to derive visitors from that do not need extra data in each tree node.
// rst:

struct no_tree_node_data {
	// rst:		.. class:: template<typename Config, typename TreeNode> \
	// rst:		           TreeNodeData
	// rst:

	template<typename Config, typename TreeNode>
	struct TreeNodeData {
		// rst:			.. type:: type = tagged_list<>
		using type = tagged_list<>;
	};
};

// rst: .. class:: null_visitor : no_instance_data, no_tree_node_data, no_result
// rst:
// rst:		A visitor that does nothing.
// rst:		That is, it provides a default implementation for each event.
// rst:

// rst: .. concept:: template<typename Vis> \
// rst:              Visitor
// rst:
// rst:		The central concept for plugins.
// rst:		When writing new plugins it may be useful to derive from `null_visitor`
// rst:		to start with default-implementations of all elements.
// rst:
// rst:		.. notation::
// rst:
// rst:		.. var:: Vis vis
// rst:		.. var:: State state
// rst:
// rst:			A specialization of `canon_state`.
// rst:
// rst:		.. var:: TreeNode t
// rst:
// rst:			A specialization of `tree_node`.
// rst:
// rst:		.. var:: Perm aut
// rst:
// rst:			A `perm_group::Permutation`.
// rst:

struct null_visitor : no_instance_data, no_tree_node_data, no_result {
	// rst:		.. assoc_types::
	// rst:
	// rst:		.. class:: template<typename Config, typename TreeNode> \
	// rst:		           Vis::InstanceData
	// rst:
	// rst:			:tparam Config: a specialization of `config`.
	// rst:			:tparam TreeNode: a specialization of `tree_node`.
	// rst:
	// rst:			.. type:: type
	// rst:
	// rst:				An alias for either a `tagged_list` or a `tagged_element`.
	// rst:				An object of the this type will be instantiated in each `canon_state`.
	// rst:				If non is needed, you can derive from `no_instance_data`.
	// rst:
	// rst:		.. class:: template<typename Config, typename TreeNode> \
	// rst:		           Vis::TreeNodeData
	// rst:
	// rst:			.. type:: type
	// rst:
	// rst:				An alias for either a `tagged_list` or a `tagged_element`.
	// rst:				An object of the this type will be instantiated in each `tree_node`.
	// rst:				If non is needed, you can derive from `no_tree_node_data`.
	// rst:
	// rst:		.. type:: Vis::can_select_target_cell
	// rst:
	// rst:			An alias for either `std::true_type` or `std::false_type`
	// rst:			to denote whether the visitor want to be the target cell selector.
	// rst:			If so, the expression `vis.select_target_cell(state, t)` must be valid,
	// rst:			and return a non-negative integer strictly less than :math:`n`.
	// rst:
	using can_select_target_cell = std::false_type;
	// rst:		.. type:: Vis::can_explore_tree
	// rst:
	// rst:			An alias for either `std::true_type` or `std::false_type`
	// rst:			to denote whether the visitor want to provide the tree traversal algorithm.
	// rst:			If so, the expression `vis.explore_tree(state)` must be valid.
	// rst:
	using can_explore_tree = std::false_type;
public:

	// rst:		.. valid_expr::
	// rst:
	// rst:		- | Expression: `vis.initialize(state)`
	// rst:		  | Return type: `void`
	// rst:		  | Called: before the root node is constructed.

	template<typename State>
	void initialize(State &state) { }

	// rst:
	// rst:		**Tree Node Methods**
	// rst:
	// rst:		- | Expression: `vis.extract_result(state)`
	// rst:		  | Return type: a specialization of either `tagged_list` or `tagged_element`.
	// rst:		  | Called: when the complete tree has been explored.

	// rst:		- | Expression: `tree_create_node_begin(state, t)`
	// rst:		  | Return type: `bool`
	// rst:		  | Called: in the beginning of the `tree_node` constructor.
	// rst:		  | Returning `false` means the tree node should be pruned.

	template<typename State, typename TreeNode>
	bool tree_create_node_begin(State &state, TreeNode &t) {
		return true;
	}

	// rst:		- | Expression: `vis.tree_create_node_end(state, t)`
	// rst:		  | Return type: `bool`
	// rst:		  | Called: in the end of the `tree_node` constructor.
	// rst:		  | Returning `false` means the tree node should be pruned.

	template<typename State, typename TreeNode>
	bool tree_create_node_end(State &state, TreeNode &t) {
		return true;
	}

	// rst:		- | Expression: `vis.tree_destroy_node(state, t)`
	// rst:		  | Return type: `void`
	// rst:		  | Called: in the beginning of the `tree_node` destructor.

	template<typename State, typename TreeNode>
	void tree_destroy_node(State &state, TreeNode &t) { }

	// rst:		- | Expression: `vis.tree_before_descend(state, t)`
	// rst:		  | Return type: `void`
	// rst:		  | Called: by tree traversal algorithms before inspecting the list of children.
	// rst:		  | The method facilitates pruning of children, and may be called at any time
	// rst:		  | the tree traversal algorithm thinks there may be new pruning information available.

	template<typename State, typename TreeNode>
	void tree_before_descend(State &state, TreeNode &t) { }

	// rst:		- | Expression: `vis.tree_create_child(state, t, element_idx_to_individualise)`
	// rst:		  | Return type: `void`
	// rst:		  | Called: by `tree_node.create_child` before a new tree node is created.
	// rst:		  | See `tree_node.create_child` for the meaning of `element_idx_to_individualise`.

	template<typename State, typename TreeNode>
	void tree_create_child(State &state, TreeNode &t, std::size_t element_idx_to_individualise) { }

	// rst:		- | Expression: `vis.tree_leaf(state, t)`
	// rst:		  | Return type: `void`
	// rst:		  | Called: in the beginning of `canon_state::report_leaf`.

	template<typename State, typename TreeNode>
	void tree_leaf(State &state, TreeNode &t) { }

	// rst:		- | Expression: `vis.tree_prune_node(state, t)`
	// rst:		  | Return type: `void`
	// rst:		  | Called: in `tree_node.prune_subtree` before the children are pruned recursively.

	template<typename State, typename TreeNode>
	void tree_prune_node(State &state, TreeNode &t) { }

	// rst:
	// rst:		**Canonical Form Methods**
	// rst:

	// rst:		- | Expression: `vis.canon_new_best(state, t_ptr)`
	// rst:		  | Return type: `void`
	// rst:		  | Called: in `canon_state::report_leaf` after a new best leaf has been assigned. A pointer to a previous leaf, if any, is given.

	template<typename State, typename TreeNode>
	void canon_new_best(State &state, TreeNode *previous) { }

	// rst:		- | Expression: `vis.canon_worse(state, t)`
	// rst:		  | Return type: `void`
	// rst:		  | Called: in `canon_state::report_leaf` when the candidate node `t` is worse than the current best leaf.

	template<typename State, typename TreeNode>
	void canon_worse(State &state, TreeNode &t) { }

	// rst:		- | Expression: `vis.canon_prune(state)`
	// rst:		  | Return type: `void`
	// rst:		  | Called: in `canon_state::prune_canon_leaf` if there is a best leaf, before it is unassigned.

	template<typename State>
	void canon_prune(State &state) { }

	// rst:
	// rst:		**Automorphism Methods**
	// rst:

	// rst:		- | Expression: `vis.automorphism_leaf(state, t, aut)`
	// rst:		  | Return type: `void`
	// rst:		  | Called: in `canon_state::prune_canon_leaf` when the candidate leaf `t` is as good as the current best leaf.
	// rst:		  | The given permutation `aut` is the automorphism that maps the current best leaf to `t`.

	template<typename State, typename TreeNode, typename Perm>
	void automorphism_leaf(State &state, TreeNode &t, const Perm &aut) { }

	// rst:		- | Expression: `vis.automorphism_implicit(state, t, aut, tag)`
	// rst:		  | Return type: `void`
	// rst:		  | Called: by visitors when they have a permutation `aut` which is an automorphism.
	// rst:		  | The `tag`, of type `std::size_t`, is a visitor-defined number.

	template<typename State, typename TreeNode, typename Perm>
	void automorphism_implicit(State &state, TreeNode &t, const Perm &aut, std::size_t tag) { }

	// rst:
	// rst:		**Refinement Methods**
	// rst:

	// rst:		- | Expression: `vis.refine(state, t)`
	// rst:		  | Return type: `RefinementResult`
	// rst:		  | Called: by the `canon_state` through the `tree_node` constructor.

	template<typename State, typename TreeNode>
	RefinementResult refine(State &state, TreeNode &t) {
		return RefinementResult::Never;
	}

	// rst:		- | Expression: `vis.refine_cell_split_begin(state, t, refiner_begin, refinee_begin, refinee_end)`
	// rst:		  | Return type: `void`
	// rst:		  | Called: by visitors before the cell starting at `refinee_begin` and ending at `refinee_end` is being split.
	// rst:		  | The partition of `t` must be in a valid state when this method is called.
	// rst:		  | The `refiner_begin` may be `canon_state::n` or the cell which the calling visitor defines as being responsible for the splits.
	// rst:		  | After this method call the `vis.refine_new_cell` method may be invoked with numbers in the range `refinee_begin` to `refinee_end`.
	// rst:		  | A matching call to `vis.refine_cell_split_end` with the same arguments must be made before a new cell can be split.
	// rst:		  | This method will not be called before an individualization operation.

	template<typename State, typename TreeNode>
	void refine_cell_split_begin(State &state, TreeNode &t, std::size_t refiner_begin,
			std::size_t refinee_begin, std::size_t refinee_end) { }

	// rst:		- | Expression: `vis.refine_new_cell(state, t, new_cell, type)`
	// rst:		  | Return type: `void`
	// rst:		  | Called: by visitors for each new cell-beginning being created. E.g., for a cell 0 to 10, split into 0 to 2 and 2 to 10, only a call with 2 will be made.
	// rst:		  | The method may only be called between a matching pair of `vis.refine_cell_split_begin` and `vis.refine_cell_split_end`, and only with cell beginnings in the range specified by those calls.
	// rst:		  | The partition of `t` must be in a valid state when this method is called, and the cell must have been created at this time.
	// rst:		    That is, after `vis.refine_cell_split_begin` the partition is fully modified (all splits are made), and then all calls to `vis.refine_new_cell` is made.
	// rst:		  | The `type` variable of type `std::size_t` is a visitor-defined number to communicate which kind of split it made.
	// rst:		  | This method will not be called before an individualization operation.

	template<typename State, typename TreeNode>
	bool refine_new_cell(State &state, TreeNode &t, std::size_t new_cell, std::size_t type) {
		return true;
	}

	// rst:		- | Expression: `vis.refine_cell_split_end(state, t, refiner_begin, refinee_begin, refinee_end)`
	// rst:		  | Return type: `bool`
	// rst:		  | Called: by visitors after the cell starting at `refinee_begin` and ending at `refinee_end` was split.
	// rst:		  | The partition of `t` must be in a valid state when this method is called.
	// rst:		  | The call must be after a matching `vis.refine_cell_begin` and must be given the same arguments.
	// rst:		  | Returning `false` means refinement should be aborted and the tree node pruned.
	// rst:		  | This method will not be called before an individualization operation.

	template<typename State, typename TreeNode>
	void refine_cell_split_end(State &state, TreeNode &t, std::size_t refiner_begin,
			std::size_t refinee_begin, std::size_t refinee_end) { }

	// rst:		- | Expression: `vis.refine_quotient_edge(state, t, refiner, refinee, count)`
	// rst:		  | Return type: `bool`
	// rst:		  | Called: by visitors, probably mostly Weisfeiler-Leman-style visitors.
	// rst:		  | The partition of `t` must be in a valid state when this method is called.
	// rst:
	// rst:		  .. deprecated:: 0.2
	// rst:		     This method will be refactored not too far in the future.

	template<typename State, typename TreeNode>
	bool refine_quotient_edge(State &state, TreeNode &t, std::size_t refiner, std::size_t refinee, std::size_t count) {
		return true;
	}

	// rst:		- | Expression: `vis.refine_refiner_done(state, t, refiner, refiner_end)`
	// rst:		  | Return type: `bool`
	// rst:		  | Called: by visitors, probably mostly Weisfeiler-Leman-style visitors.
	// rst:		  | The partition of `t` must be in a valid state when this method is called.
	// rst:
	// rst:		  .. deprecated:: 0.2
	// rst:		     This method will be refactored not too far in the future.

	template<typename State, typename TreeNode>
	bool refine_refiner_done(State &state, TreeNode &t, const std::size_t refiner, const std::size_t refiner_end) {
		return true;
	}

	// rst:		- | Expression: `vis.refine_abort(state, t)`
	// rst:		  | Return type: `void`
	// rst:		  | Called: by visitors and by the `tree_node` constructor when refinement is being aborted.
	// rst:		  | The partition of `t` must be in a valid state when this method is called.
	// rst:
	// rst:		  .. deprecated:: 0.2
	// rst:		     This method will be refactored not too far in the future.

	template<typename State, typename TreeNode>
	void refine_abort(State &state, TreeNode &t) { }

	// rst:
	// rst:		**Node Invariant Methods**
	// rst:

	// rst:		- | Expression: `vis.invariant_better(state, t)`
	// rst:		  | Return type: `void`
	// rst:		  | Called: by `invariant_coordinator::better_invariant` when a visitor reports that it has a better invariant value.

	template<typename State, typename TreeNode>
	void invariant_better(State &state, TreeNode &t) { }
};

} // namespace graph_canon

#endif /* GRAPH_CANON_VISITOR_VISITOR_HPP */