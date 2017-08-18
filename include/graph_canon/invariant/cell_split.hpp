#ifndef GRAPH_CANON_INVARIANT_CELL_SPLIT_HPP
#define GRAPH_CANON_INVARIANT_CELL_SPLIT_HPP

#include <graph_canon/invariant/support.hpp>

#include <cassert>
#include <vector>

namespace graph_canon {

struct invariant_cell_split : null_visitor {

	struct tree_data_t {
	};

	struct tree_data {
		std::size_t end_index;
	};

	template<typename Config, typename TreeNode>
	struct TreeNodeData {
		using type = tagged_list<tree_data_t, tree_data>;
	};

	struct instance_data_t {
	};

	template<typename SizeType>
	struct instance_data {
		std::vector<SizeType> trace;
		std::size_t trace_end = 0;
		std::size_t visitor_type;
	};

	template<typename Config, typename TreeNode>
	struct InstanceData {
		using type = tagged_list<instance_data_t, instance_data<typename Config::SizeType> >;
	};
private:

	template<typename State>
	instance_data<typename State::SizeType> &get_data(State &state) const {
		return get(instance_data_t(), state.data);
	}
public:

	void initialize(auto &state) {
		auto &i_data = get(instance_data_t(), state.data);
		i_data.trace.resize(state.n);
		i_data.visitor_type = invariant_support::init_visitor(state);
	}

	bool tree_create_node_begin(auto &state, auto &t) {
		auto &t_data = get(tree_data_t(), t.data);
		// add the individualization as a cell split
		// and fix book keeping
		if(t.get_parent()) {
			t_data.end_index = get(tree_data_t(), t.get_parent()->data).end_index;
			const auto ind_cell = t.get_parent()->child_refiner_cell + 1;
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
			std::cout << "TraceCS   ctree, end_index=" << t_data.end_index << ", ind_cell=" << ind_cell << std::endl;
#endif
			const bool continue_ = add_trace(state, t, ind_cell);
			return continue_;
		} else {
			t_data.end_index = 0;
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
			std::cout << "TraceCS   ctree, end_index=" << t_data.end_index << std::endl;
#endif
			return true;
		}
	}

	bool refine_new_cell(auto &state, auto &t, std::size_t cell, std::size_t type) {
		return add_trace(state, t, cell);
	}

	void trace_better(auto &state, auto &t) {
		auto &i_data = get_data(state);
		auto &t_data = get(tree_data_t(), t.data);
		// shorten the trace
		i_data.trace_end = t_data.end_index;
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
		std::cout << "TraceCS   better, trace_end=" << i_data.trace_end << std::endl;
#endif
	}
private:

	template<typename State, typename TreeNode>
	void extend_trace(State &state, TreeNode &t, const auto elem) {
		auto &i_data = get_data(state);
		auto &t_data = get(tree_data_t(), t.data);
		assert(t_data.end_index == i_data.trace_end);
		i_data.trace[i_data.trace_end] = elem;
		++i_data.trace_end;
		++t_data.end_index;
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
		std::cout << "TraceCS   better, trace_end=" << i_data.trace_end << std::endl;
#endif
	}

	template<typename State, typename TreeNode>
	bool add_trace(State &state, TreeNode &t, std::size_t cell) {
		if(t.get_is_pruned()) return false;
		auto &i_data = get_data(state);
		auto &t_data = get(tree_data_t(), t.data);
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG_ADD_TRACE
		std::cout << "TraceCS   add, end_index=" << t_data.end_index << ", i_end=" << i_data.trace_end << ", cell=" << cell << std::endl;
#endif
		assert(i_data.trace_end >= t_data.end_index);
		const auto continue_ = invariant_support::add_trace_element(state, t, i_data.visitor_type);
		if(!continue_) return false;
		const auto elem = cell;
		if(i_data.trace_end == t_data.end_index) {
			extend_trace(state, t, elem);
			return true;
		}
		const auto old_elem = i_data.trace[t_data.end_index];
		if(elem < old_elem) {
			// we are better
			invariant_support::better_trace(state, t);
			extend_trace(state, t, elem);
			return true;
		} else if(elem == old_elem) {
			// same
			++t_data.end_index;
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG_ADD_TRACE
			std::cout << "TraceCS   add(inc), end_index=" << t_data.end_index << ", i_end=" << i_data.trace_end << std::endl;
#endif
			return true;
		} else {
			// we are worse
			invariant_support::worse_trace(state, t);
			return false;
		}
	}
};

} // namespace graph_canon

#endif /* GRAPH_CANON_INVARIANT_CELL_SPLIT_HPP */