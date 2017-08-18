#ifndef GRAPH_CANON_INVARIANT_PARTIAL_LEAF_HPP
#define GRAPH_CANON_INVARIANT_PARTIAL_LEAF_HPP

#include <graph_canon/invariant/support.hpp>

#include <cassert>
#include <vector>

//#define GRAPH_CANON_PARTIAL_LEAF_DEBUG_PRINT

#ifdef GRAPH_CANON_PARTIAL_LEAF_DEBUG_PRINT
#include <graph_canon/detail/io.hpp>
#endif

namespace graph_canon {

struct invariant_partial_leaf : null_visitor {

	struct tree_data_t {
	};

	struct tree_data {
		std::size_t next = 0;
	};

	template<typename Config, typename TreeNode>
	struct TreeNodeData {
		using type = tagged_list<tree_data_t, tree_data>;
	};

	struct element {
		std::size_t canon_idx;
		std::vector<std::size_t> edges;
	};

	struct instance_data_t {
	};

	struct instance_data {
		std::vector<element> trace;
		std::size_t next = 0;
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
		auto &t_data = get(tree_data_t(), t.data);
		// fix book keeping
		if(t.get_parent()) {
			t_data.next = get(tree_data_t(), t.get_parent()->data).next;
		} else {
			t_data.next = 0;
		}
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
		std::cout << "TracePL   begin, t_next=" << t_data.next << ", i_next=" << get(instance_data_t(), state.data).next << std::endl;
#endif
		return true;
	}

	bool tree_create_node_end(auto &state, auto &t) {
		auto &t_data = get(tree_data_t(), t.data);
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
		std::cout << "TracePL   end, t_next=" << t_data.next << ", i_next=" << get(instance_data_t(), state.data).next << std::endl;
#endif
		if(t.get_parent() && !t.get_is_pruned()) {
			const auto parent = get(tree_data_t(), t.get_parent()->data).next;
			assert(t_data.next > parent);
		}
		return true;
	}

	template<typename State, typename TreeNode>
	bool refine_refiner_done(State &state, TreeNode &t, const std::size_t refiner, const std::size_t refiner_end) {
		if(t.get_is_pruned()) return false;
		// we are only concerned with singleton cells
		if(refiner + 1 != refiner_end)
			return true;
		auto &i_data = get_data(state);
		auto &t_data = get(tree_data_t(), t.data);
		const auto continue_ = invariant_support::add_trace_element(state, t, i_data.visitor_type);
		if(!continue_) return false;
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
		std::cout << "TracePL   singleton, refiner=" << refiner << std::endl;
#endif
		if(t_data.next == i_data.next) {
			extend_trace(state, t, refiner);
			return true;
		}
		assert(t_data.next < i_data.next);
		auto &elem = i_data.trace[t_data.next];
		if(refiner > elem.canon_idx) {
			invariant_support::better_trace(state, t);
			extend_trace(state, t, refiner);
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
			std::cout << "TracePL   better(refiner), refiner=" << refiner << std::endl;
#endif
			return true;
		} else if(refiner < elem.canon_idx) {
			// we are worse
			invariant_support::worse_trace(state, t);
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
			std::cout << "TracePL   worse(refiner), refiner=" << refiner << std::endl;
#endif
			return false;
		}

		assert(refiner == elem.canon_idx);
		std::vector<std::size_t> edges;
		add_edges(state, t, refiner, edges);
		if(edges.size() > elem.edges.size()) {
			invariant_support::better_trace(state, t);
			extend_trace(state, t, refiner, std::move(edges));
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
			std::cout << "TracePL   better(num_edges), refiner=" << refiner << std::endl;
#endif
			return true;
		} else if(edges.size() < elem.edges.size()) {
			invariant_support::worse_trace(state, t);
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
			std::cout << "TracePL   worse(num_edges), refiner=" << refiner << std::endl;
#endif
			return false;
		}
		assert(edges.size() == elem.edges.size());
		for(int i = 0; i < edges.size(); ++i) {
			const auto trace = elem.edges[i];
			const auto cand = edges[i];
			if(cand < trace) {
				invariant_support::better_trace(state, t);
				extend_trace(state, t, refiner, std::move(edges));
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
				std::cout << "TracePL   better(edge), refiner=" << refiner << std::endl;
#endif
				return true;
			} else if(cand == trace) {
				// fine
			} else {
				// we are worse
				invariant_support::worse_trace(state, t);
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
				std::cout << "TracePL   worse(edge), refiner=" << refiner << std::endl;
#endif
				return false;
			}
		}
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
		std::cout << "TracePL   same, " << refiner << ":";
		for(const auto e : elem.edges) std::cout << " " << e;
		std::cout << std::endl;
#endif
		++t_data.next;
		return true;
	}

	void trace_better(auto &state, auto &t) {
		auto &i_data = get_data(state);
		auto &t_data = get(tree_data_t(), t.data);
		i_data.next = t_data.next;
	}
private:

	template<typename State, typename TreeNode>
	void extend_trace(State &state, TreeNode &t, const std::size_t refiner) {
		auto &i_data = get_data(state);
		auto &t_data = get(tree_data_t(), t.data);
		assert(i_data.next == t_data.next);
		auto &elem = i_data.trace[i_data.next];
		elem.canon_idx = refiner;
		add_edges(state, t, refiner, elem.edges);
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
		std::cout << "TracePL   extend, " << refiner << ":";
		for(const auto e : elem.edges) std::cout << " " << e;
		std::cout << std::endl;
#endif
		++i_data.next;
		++t_data.next;
	}

	template<typename State, typename TreeNode>
	void extend_trace(State &state, TreeNode &t, const std::size_t refiner, std::vector<std::size_t> &&edges) {
		auto &i_data = get_data(state);
		auto &t_data = get(tree_data_t(), t.data);
		assert(i_data.next == t_data.next);
		auto &elem = i_data.trace[i_data.next];
		elem.canon_idx = refiner;
		std::swap(elem.edges, edges);
#ifdef GRAPH_CANON_TRACE_SUPPORT_DEBUG
		std::cout << "TracePL   extend, " << refiner << ":";
		for(const auto e : elem.edges) std::cout << " " << e;
		std::cout << std::endl;
#endif
		++i_data.next;
		++t_data.next;
	}

	template<typename State, typename TreeNode>
	void add_edges(State &state, TreeNode &t, const std::size_t refiner, std::vector<std::size_t> &edges) {
		const auto v = vertex(t.pi.get(refiner), state.g);
		edges.clear();
		edges.reserve(out_degree(v, state.g));
		const auto oes = out_edges(v, state.g);
		for(auto e_iter = oes.first; e_iter != oes.second; ++e_iter) {
			const auto v_adj = target(*e_iter, state.g);
			const auto v_adj_idx = state.idx[v_adj];
			const auto v_adj_canon_idx = t.pi.get_inverse(v_adj_idx);
			edges.push_back(v_adj_canon_idx);
		}
		std::sort(edges.begin(), edges.end());
		//		{ // checks
		//			for(int i = 0; i < edges.size();) {
		//				const auto pos = edges[i];
		//				const auto cell_end = t.pi.get_cell_end(pos);
		//				if(cell_end == 0) {
		//#ifdef GRAPH_CANON_PARTIAL_LEAF_DEBUG_PRINT
		//					std::cout << "Edges: " << refiner << ":";
		//					for(auto e : edges) {
		//						std::cout << " " << e;
		//						const auto b = t.pi.get_cell_end(e);
		//						if(b != 0) std::cout << ":" << b;
		//					}
		//					std::cout << std::endl;
		//					std::cout << "Idx=" << i << ", pos=" << pos << ", cell_end=" << cell_end << std::endl;
		//					detail::printTreeNode(std::cout, state, t, true) << std::endl;
		//#endif
		//					assert(false);
		//				}
		//				auto prev = pos;
		//				for(++i; i != edges.size() && edges[i] < cell_end; ++i) {
		//					const auto next = edges[i];
		//					if(prev + 1 != next) {
		//#ifdef GRAPH_CANON_PARTIAL_LEAF_DEBUG_PRINT
		//						std::cout << "Edges: " << refiner << ":";
		//						for(auto e : edges) {
		//							std::cout << " " << e;
		//							const auto b = t.pi.get_cell_end(e);
		//							if(b != 0) std::cout << ":" << b;
		//						}
		//						std::cout << std::endl;
		//						std::cout << "Idx=" << i << ", prev=" << prev << ", next=" << next << ", cell_end=" << cell_end << std::endl;
		//#endif
		//						assert(false);
		//					}
		//					prev = next;
		//				}
		//			}
		//		}
	}
};

} // namespace graph_canon

#endif /* GRAPH_CANON_INVARIANT_PARTIAL_LEAF_HPP */