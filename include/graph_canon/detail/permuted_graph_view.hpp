#ifndef GRAPH_CANON_PERMUTED_GRAPH_VIEW_HPP
#define GRAPH_CANON_PERMUTED_GRAPH_VIEW_HPP

#include <graph_canon/detail/partition.hpp>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/iteration_macros.hpp>

namespace graph_canon {
namespace detail {

template<typename Config, typename TreeNode>
struct permuted_graph_view {
	using Graph = typename Config::Graph;
	using IndexMap = typename Config::IndexMap;
	using SizeType = typename Config::SizeType;
	using Vertex = typename boost::graph_traits<Graph>::vertex_descriptor;
	using Edge = typename boost::graph_traits<Graph>::edge_descriptor;

	struct Adj {
		Vertex v;
		std::size_t out_degree;
		Edge *out_edges;
	};

	template<typename State>
	struct edge_less {

		edge_less(const State &state, const partition<SizeType> &pi) : state(state), pi(pi) { }

		bool operator()(const Edge &lhs, const Edge &rhs) const {
			const Graph &g = state.g;
			const IndexMap &idx = state.idx;
			if(pi.get_inverse(idx[target(lhs, g)])
					!= pi.get_inverse(idx[target(rhs, g)]))
				return pi.get_inverse(idx[target(lhs, g)]) < pi.get_inverse(idx[target(rhs, g)]);
			return state.edge_handler.compare(state, lhs, rhs) < 0;
		}
	private:
		const State &state;
		const partition<SizeType> &pi;
	};

	template<typename State>
	permuted_graph_view(const State &state, const typename TreeNode::OwnerPtr &leaf_node)
	: leaf_node(leaf_node), n(state.n), repr(new Adj[n]), extra_repr(nullptr) {
		const auto &pi = leaf_node->pi;
		const Graph &g = state.g;
		const IndexMap &idx = state.idx;
		assert(pi.get_num_cells() == state.n);

		edge_less<State> less(state, pi);

		const auto vs = vertices(g);
		for(auto v_iter = vs.first; v_iter != vs.second; ++v_iter) {
			const auto v = *v_iter;
			const auto v_idx = pi.get_inverse(idx[v]);

			repr[v_idx].v = v;
			const auto d = repr[v_idx].out_degree = out_degree(v, g);
			Edge *edges = repr[v_idx].out_edges = new Edge[d];
			std::size_t i = 0;

			const auto oes = out_edges(v, g);
			for(auto e_iter = oes.first; e_iter != oes.second; ++e_iter) {
				const auto e_out = *e_iter;
				edges[i] = e_out;
				i++;
			}
			std::sort(edges, edges + d, less);
		}
	}

	template<typename State>
	void repermute(const State &state, const typename TreeNode::OwnerPtr &leaf_node_new) {
		const auto &pi = leaf_node->pi;
		const auto &pi_new = leaf_node_new->pi;
		if(!extra_repr) extra_repr = new Adj[n];
		edge_less<State> less(state, pi_new);
		// copy each element of repr to extra_repr using the new index
		// also sort all the out_edges again
		for(std::size_t v_id_old = 0; v_id_old < n; v_id_old++) {
			const auto v_id = pi.get(v_id_old);
			const auto v_id_new = pi_new.get_inverse(v_id);
			extra_repr[v_id_new] = repr[v_id_old];

			Edge *edges = extra_repr[v_id_new].out_edges;
			std::sort(edges, edges + extra_repr[v_id_new].out_degree, less);
		}
		std::swap(repr, extra_repr);
		leaf_node = leaf_node_new;
	}

	~permuted_graph_view() {
		if(repr) {
			for(std::size_t i = 0; i < n; i++)
				delete [] repr[i].out_edges;
		}
		delete [] repr;
		delete [] extra_repr;
	}

	template<typename State>
	static long long compare(const State &state,
			const permuted_graph_view<Config, typename State::TreeNode> &g1,
			const permuted_graph_view<Config, typename State::TreeNode> &g2) {
		const Graph &g = state.g;
		const IndexMap &idx = state.idx;
		assert(g1.n == g2.n);
		for(std::size_t v_idx = 0; v_idx < g1.n; v_idx++) {
			const Adj &adj1 = g1.repr[v_idx];
			const Adj &adj2 = g2.repr[v_idx];
			if(adj1.out_degree != adj2.out_degree) {
				return (adj1.out_degree) - (adj2.out_degree);
			}
			const Edge *edges1 = adj1.out_edges;
			const Edge *edges2 = adj2.out_edges;
			for(std::size_t e_id = 0; e_id < adj1.out_degree; e_id++) {
				const Edge e1 = edges1[e_id];
				const Edge e2 = edges2[e_id];
				const Vertex v1 = target(e1, g);
				const Vertex v2 = target(e2, g);
				const auto v_tar_idx1 = g1.leaf_node->pi.get_inverse(idx[v1]);
				const auto v_tar_idx2 = g2.leaf_node->pi.get_inverse(idx[v2]);
				if(v_tar_idx1 != v_tar_idx2)
					return v_tar_idx1 - v_tar_idx2;
				auto eDiff = state.edge_handler.compare(state, e1, e2);
				if(eDiff != 0) return eDiff;
			}
		}
		return 0;
	}
private:
	typename TreeNode::OwnerPtr leaf_node;
	std::size_t n;
	Adj *repr;
	Adj *extra_repr;
};

} // namespace detail
} // namespace graph_canon

#endif /* GRAPH_CANON_PERMUTED_GRAPH_VIEW_HPP */