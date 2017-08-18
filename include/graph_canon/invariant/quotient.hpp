#ifndef GRAPH_CANON_INVARIANT_QUOTIENT_HPP
#define GRAPH_CANON_INVARIANT_QUOTIENT_HPP

#include <graph_canon/invariant/support.hpp>

#include <cassert>
#include <vector>

namespace graph_canon {

struct invariant_quotient : null_visitor {

	struct tree_data_t {
	};

	struct tree_data {
		std::size_t end_index = 0;
	};

	template<typename Config, typename TreeNode>
	struct TreeNodeData {
		using type = tagged_list<tree_data_t, tree_data>;
	};

	struct element {

		friend bool operator<(const element &a, const element &b) {
			return std::tie(a.refiner, a.refinee, a.count)
					< std::tie(b.refiner, b.refinee, b.count);
		}

		friend bool operator==(const element &a, const element &b) {
			return std::tie(a.refiner, a.refinee, a.count)
					== std::tie(b.refiner, b.refinee, b.count);
		}
	public:
		std::size_t refiner, refinee;
		std::size_t count;
	};

	struct instance_data_t {
	};

	struct instance_data {
		std::vector<std::vector<element> > trace; // a trace for each level
		std::size_t max_level = 0;
		std::size_t visitor_type;
	};

	template<typename Config, typename TreeNode>
	struct InstanceData {
		using type = tagged_list<instance_data_t, instance_data>;
	};
private:

	instance_data &get_data(auto &state) const {
		return get(instance_data_t(), state.data);
	}
public:

	void initialize(auto &state) {
		auto &i_data = get(instance_data_t(), state.data);
		i_data.trace.resize(state.n);
		i_data.visitor_type = invariant_support::init_visitor(state);
	}

	bool tree_create_node_begin(auto &state, auto &t) {
		auto &i_data = get_data(state);
		if(t.get_parent()) {
			if(t.level > i_data.max_level) {
				assert(t.level == i_data.max_level + 1);
			}
		}
		return true;
	}

	bool tree_create_node_end(auto &state, auto &t) {
		auto &i_data = get_data(state);
		if(!t.get_parent())
			return true;
		// did we extend beyond the previous max_level?
		assert(i_data.max_level + 1 >= t.level);
		if(i_data.max_level < t.level) {
			assert(i_data.max_level + 1 == t.level);
			++i_data.max_level;
		}
		return true;
	}

	template<typename State, typename TreeNode>
	bool refine_quotient_edge(State &state, TreeNode &t, std::size_t refiner, std::size_t refinee, std::size_t count) {
		if(t.get_is_pruned()) return false;
		auto &i_data = get_data(state);
		auto &t_data = get(tree_data_t(), t.data);
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
		std::cout << "TraceQ    add, e_end=" << t_data.end_index << ", i_end=" << i_data.trace[t.level].size() << ", elem=(" << refiner << ", " << refinee << ", " << count << ")" << std::endl;
#endif
		const auto continue_ = invariant_support::add_trace_element(state, t, i_data.visitor_type);
		if(!continue_) return false;
		const auto elem = element{refiner, refinee, count};
		assert(i_data.trace[t.level].size() >= t_data.end_index);
		if(i_data.trace[t.level].size() == t_data.end_index) {
			extend_trace(state, t, elem);
			return true;
		}
		const auto old_elem = i_data.trace[t.level][t_data.end_index];
		if(elem < old_elem) {
			// we are better
			invariant_support::better_trace(state, t);
			extend_trace(state, t, elem);
			return true;
		} else if(elem == old_elem) {
			// same
			++t_data.end_index;
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
			std::cout << "TraceQ    add(inc), end_index=" << t_data.end_index << ", i_end=" << i_data.trace[t.level].size() << std::endl;
#endif
			return true;
		} else {
			// we are worse
			invariant_support::worse_trace(state, t);
			return false;
		}
	}

	void trace_better(auto &state, auto &t) {
		auto &i_data = get_data(state);
		auto &t_data = get(tree_data_t(), t.data);
		assert(t.level != 0);
		assert(i_data.max_level >= t.level);
		// clear all those above t.level
		for(auto i = t.level + 1; i <= i_data.max_level; ++i)
			i_data.trace[i].clear();
		// shorten the trace
		i_data.max_level = t.level - 1;
		assert(i_data.trace[t.level].size() >= t_data.end_index);
		i_data.trace[t.level].resize(t_data.end_index);
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
		std::cout << "TraceQ    better, max_level=" << t.level << std::endl;
#endif
	}
private:

	template<typename State, typename TreeNode>
	void extend_trace(State &state, TreeNode &t, const element elem) {
		auto &i_data = get_data(state);
		auto &t_data = get(tree_data_t(), t.data);
		assert(i_data.trace[t.level].size() == t_data.end_index);
		i_data.trace[t.level].push_back(elem);
		++t_data.end_index;
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
		std::cout << "TraceQ    better, max_level=" << t_data.end_index << std::endl;
#endif
	}
};

} // namespace graph_canon

#endif /* GRAPH_CANON_INVARIANT_QUOTIENT_HPP */