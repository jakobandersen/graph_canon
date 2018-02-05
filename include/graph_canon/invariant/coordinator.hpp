#ifndef GRAPH_CANON_INVARIANT_SUPPORT_HPP
#define GRAPH_CANON_INVARIANT_SUPPORT_HPP

#include <graph_canon/visitor/visitor.hpp>

#include <cassert>
#include <vector>

//#define GRAPH_CANON_TRACE_SUPPORT_DEBUG
//#define GRAPH_CANON_TRACE_SUPPORT_DEBUG_ADD_TRACE

namespace graph_canon {

// rst: .. class:: invariant_coordinator
// rst:
// rst:		A visitor for properly coordinating multiple node invariants.
// rst:		That is, ensuring a total order among different providers.
// rst:

struct invariant_coordinator : null_visitor {

	struct tree_data_t {
	};

	struct tree_data {
		// Set in the beginning, updated throughout.
		std::size_t generation;
		// Set in the beginning, updated throughout.
		std::size_t end_index;
	};

	template<typename Config, typename TreeNode>
	struct TreeNodeData {
		using type = tagged_element<tree_data_t, tree_data>;
	};

	struct instance_data_t {
	};

	template<typename SizeType>
	struct instance_data {
		// Set when extending the trace or improving it.
		// When extending, it will always be one lower than t.level.
		// (except in the root)
		std::size_t max_level = 0;
		// Increased when improving the trace.
		std::size_t generation = 0;
		std::vector<std::vector<SizeType> > trace; // trace of visitors
		// Set in node_end if the trace.
		std::vector<SizeType> generations; // for versioning when jumping around in the tree
		// Type counter for different trace visitors
		std::size_t visitor = 0;
	};

	template<typename Config, typename TreeNode>
	struct InstanceData {
		using type = tagged_element<instance_data_t, instance_data<typename Config::SizeType> >;
	};
public:

	void initialize(auto &state) {
		auto &i_data = get(instance_data_t(), state.data);
		i_data.trace.resize(state.n);
		i_data.generations.resize(state.n);
	}

	template<typename State, typename TreeNode>
	bool tree_create_node_begin(State &state, TreeNode &t) {
		auto &i_data = get(instance_data_t(), state.data);
		auto &t_data = get(tree_data_t(), t.data);
		t_data.end_index = 0;
		if(t.get_parent()) {
			assert(i_data.max_level >= t.get_parent()->level);
			assert(i_data.generations[t.get_parent()->level] == get(tree_data_t(), t.get_parent()->data).generation);
			if(t.level > i_data.max_level) {
				assert(t.level == i_data.max_level + 1);
				t_data.generation = i_data.generation;
			} else {
				t_data.generation = i_data.generations[t.level];
			}
		} else {
			t_data.generation = 0;
		}
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
		std::cout << "Trace     ctree, gen=" << i_data.generation << ", index=" << t_data.end_index << std::endl;
#endif
		return true;
	}

	template<typename State, typename TreeNode>
	bool tree_create_node_end(State &state, TreeNode &t) {
		if(t.get_is_pruned()) {
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
			std::cout << "Trace     dtree, already pruned" << std::endl;
#endif
			return false;
		}
		auto &i_data = get(instance_data_t(), state.data);
		auto &t_data = get(tree_data_t(), t.data);
		if(!t.get_parent()) {
			assert(t_data.generation == i_data.generation);
			i_data.generations[t.level] = i_data.generation;
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
			std::cout << "Trace     dtree, root" << std::endl;
#endif
			return true;
		}
		// did we extend beyond the previous max_level?
		assert(i_data.max_level + 1 >= t.level);
		if(i_data.max_level < t.level) {
			assert(i_data.max_level + 1 == t.level);
			++i_data.max_level;
			assert(t_data.generation == i_data.generation);
			i_data.generations[t.level] = i_data.generation;
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
			std::cout << "Trace     dtree, is_extending, max_level=" << i_data.max_level << ", gen=" << i_data.generation << std::endl;
#endif
			return true;
		}
		// we are not extending, so check if we fall short of the previous trace
		// Note: if i_end has not been "initialized",
		// then we have not ended at this level before,
		// but then max_level has never been this value,
		// but then i_data.max_level < t.level
		// i.e., i_end is initialized.
		const auto t_end = t_data.end_index;
		const auto i_end = i_data.trace[t.level].size();
		if(t_end < i_end) {
			// there was a trace before, and we are worse
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
			std::cout << "Trace     earlier, gen=" << i_data.generation << ", max_level=" << i_data.max_level << ", i_end=" << i_end << ", t_end=" << t_end << std::endl;
#endif
			worse_invariant(state, t);
			return false;
		} else if(t_end > i_end) {
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
			std::cout << "Trace     later, gen=" << i_data.generation << ", max_level=" << i_data.max_level << ", i_end=" << i_end << ", t_end=" << t_end << std::endl;
#endif
			// TODO: how can this happen ? it should be handled in add_trace
			assert(false);
			better_invariant(state, t);
			i_data.generations[t.level] = t_data.generation;
			return true;
		} else { // same end
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
			std::cout << "Trace     same, gen=" << i_data.generation << ", max_level=" << i_data.max_level << ", i_end=" << i_end << ", t_end=" << t_end << std::endl;
#endif
			i_data.generations[t.level] = t_data.generation;
			return true;
		}
	}

	void tree_before_descend(auto &state, auto &t) {
		if(!t.get_parent()) return; // the root is always ok
		if(t.get_is_pruned()) return;

		auto &i_data = get(instance_data_t(), state.data);
		auto &t_data = get(tree_data_t(), t.data);

		// the trace may have been shortened
		if(t.level > i_data.max_level) {
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
			std::cout << "Trace     prune(level), t_level=" << t.level << ", i_level=" << i_data.max_level << std::endl;
#endif
			t.prune_subtree(state, true);
			return;
		}
		// we have be outdated
		const auto i_gen = i_data.generations[t.level];
		const auto t_gen = t_data.generation;
		assert(t_gen <= i_gen);
		if(t_gen < i_gen) {
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
			std::cout << "Trace     prune(gen), t_gen=" << t_gen << ", i_gen=" << i_gen << ", index=" << t_data.end_index << std::endl;
#endif
			t.prune_subtree(state, true);
			return;
		}
	}
public:

	// rst:		.. function:: template<typename State> \
	// rst:		              static std::size_t init_visitor(State &state)
	// rst:
	// rst:			Must be called by each node invariant visitor, probably in its `initialize` method.
	// rst:
	// rst:			:returns: an ID that must be provided in subsequent calls to the coordinator.

	template<typename State>
	static std::size_t init_visitor(State &state) {
		return get(instance_data_t(), state.data).visitor++;
	}

	// rst:		.. function:: template<typename State, typename TreeNode> \
	// rst:		              static bool add_invariant_element(State &state, TreeNode &t, const std::size_t visitor)
	// rst:
	// rst:			Tell the coordinator that the calling visitor wants to add another trace element.
	// rst:			This may fail if another visitor has the current invariant position.
	// rst:
	// rst:			:returns: `true` if the calling visitor may add a value, `false` if not.

	template<typename State, typename TreeNode>
	static bool add_invariant_element(State &state, TreeNode &t, const std::size_t visitor) {
		auto &i_data = get(instance_data_t(), state.data);
		auto &t_data = get(tree_data_t(), t.data);
		auto &trace = i_data.trace[t.level];
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG_ADD_TRACE
		std::cout << "Trace     add, index=" << t_data.end_index << ", t_level=" << t.level << ", i_level=" << i_data.max_level << ", visitor=" << visitor << ", t_end=" << t_data.end_index << ", i_end=" << trace.size() << std::endl;
#endif
		if(!t.get_parent()) {
			// always extending
			trace.push_back(visitor);
			return true;
		}
		assert(i_data.max_level + 1 >= t.level);
		if(t.level > i_data.max_level) {
			// also always extending
			trace.push_back(visitor);
			return true;
		}
		if(t_data.end_index == trace.size()) {
			// extending beyond the previous trace
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
			std::cout << "Trace     add(better by length), index=" << t_data.end_index << ", t_level=" << t.level << ", i_level=" << i_data.max_level << std::endl;
#endif
			better_invariant(state, t);
			trace.push_back(visitor);
			return true;
		}
		// compare race
		if(visitor < trace[t_data.end_index]) {
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
			std::cout << "Trace     add(better by visitor), index=" << t_data.end_index << ", t_level=" << t.level << ", i_level=" << i_data.max_level << ", t_visitor=" << visitor << ", i_visitor=" << trace[t_data.end_index] << std::endl;
#endif
			better_invariant(state, t);
			trace.push_back(visitor);
			return true;
		} else if(visitor > trace[t_data.end_index]) {
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
			std::cout << "Trace     add(worse by visitor), index=" << t_data.end_index << ", t_level=" << t.level << ", i_level=" << i_data.max_level << ", t_visitor=" << visitor << ", i_visitor=" << trace[t_data.end_index] << std::endl;
#endif
			worse_invariant(state, t);
			return false;
		} else {
			++t_data.end_index;
			return true;
		}
	}

	// rst:		.. function:: template<typename State, typename TreeNode> \
	// rst:		              static void better_invariant(State &state, TreeNode &t)
	// rst:
	// rst:			Must be called by a visitor when it has been allowed to add a value,
	// rst:			and that value is better than the value the visitor previously had in this position.
	// rst:
	// rst:			Will invoke the `invariant_better` event and prune the current best leaf.

	template<typename State, typename TreeNode>
	static void better_invariant(State &state, TreeNode &t) {
		auto &i_data = get(instance_data_t(), state.data);
		auto &t_data = get(tree_data_t(), t.data);
		// shrink the trace and increase the generation
		assert(t.level != 0);
		assert(i_data.max_level >= t.level);
		// clear all those above t.level
		for(auto i = t.level + 1; i <= i_data.max_level; ++i)
			i_data.trace[i].clear();
		// shorten the trace
		i_data.max_level = t.level - 1;
		assert(i_data.trace[t.level].size() >= t_data.end_index);
		i_data.trace[t.level].resize(t_data.end_index);
		++i_data.generation;
		t_data.generation = i_data.generation;
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
		std::cout << "Trace     better, gen=" << i_data.generation << ", index=" << t_data.end_index << ", t_level=" << t.level << ", i_level=" << i_data.max_level << std::endl;
#endif
		state.visitor.invariant_better(state, t);
		// if there is a canon leaf, prune it
		state.prune_canon_leaf();
	}

	// rst:		.. function:: template<typename State, typename TreeNode> \
	// rst:		              static void worse_invariant(State &state, TreeNode &t)
	// rst:
	// rst:			Must be called by a visitor when it has been allowed to add a value,
	// rst:			but that value is worse than the value the visitor previously had in this position.
	// rst:
	// rst:			Current this method does nothing.

	template<typename State, typename TreeNode>
	static void worse_invariant(State &state, TreeNode &t) {
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
		auto &i_data = get(instance_data_t(), state.data);
		auto &t_data = get(tree_data_t(), t.data);
		std::cout << "Trace     worse, gen=" << i_data.generation << ", index=" << t_data.end_index << ", t_level=" << t.level << ", i_level=" << i_data.max_level << std::endl;
#endif
	}
};

} // namespace graph_canon

#endif /* GRAPH_CANON_INVARIANT_SUPPORT_HPP */