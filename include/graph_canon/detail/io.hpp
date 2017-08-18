#ifndef GRAPH_CANON_DETAIL_IO_HPP
#define GRAPH_CANON_DETAIL_IO_HPP

#include <graph_canon/canonicalization.hpp>

#include <iomanip>
#include <iostream>
#include <stack>

namespace graph_canon {
namespace detail {

template<typename State>
std::ostream &printPartition(std::ostream &s, const State &state, const typename State::Partition &pi) {
	s << "(" << pi.get(0);
	for(std::size_t i = 1; i < pi.get_cell_end(0); i++)
		s << " " << pi.get(i);
	for(std::size_t cell = pi.get_cell_end(0); cell < state.n; cell = pi.get_cell_end(cell)) {
		s << " | " << pi.get(cell);
		for(std::size_t i = cell + 1; i < pi.get_cell_end(cell); i++)
			s << " " << pi.get(i);
	}
	return s << ")";
}

template<typename State>
std::ostream &printPartitionCompressed(std::ostream &s, const State &state, const typename State::Partition &pi) {
	std::size_t num_non_trivial = 0;
	for(std::size_t c = 0; c < state.n; c = pi.get_cell_end(c)) {
		if(c + 1 < pi.get_cell_end(c)) num_non_trivial++;
	}
	s << "#nonTrivialCells = " << num_non_trivial;
	for(std::size_t i = 0; i < state.n; i = pi.get_cell_end(i)) {
		if(pi.get_cell_size(i) > 1) {
			s << " " << i << ":" << pi.get_cell_size(i);
		}
	}
	return s;
}

template<typename Config, typename TreeNode>
std::ostream &print(std::ostream &s, 
		const permuted_graph_view<Config, TreeNode> &pg, 
		const typename Config::Graph &g, typename Config::IndexMap idx) {
	typedef typename boost::graph_traits<typename Config::Graph>::vertex_descriptor Vertex;
	typedef typename boost::graph_traits<typename Config::Graph>::edge_descriptor Edge;

	for(std::size_t v_idx = 0; v_idx < pg.n; v_idx++) {
		s << v_idx << "(" << idx[pg.repr[v_idx].first] << "):";
		const std::vector<Edge> &edges = pg.repr[v_idx].second;
		for(std::size_t i_edge = 0; i_edge < edges.size(); i_edge++) {
			Vertex v_tar = target(edges[i_edge], g);
			s << " " << pg.pi->get_inverse(idx[v_tar]) << "(" << idx[v_tar] << ")";
		}
		s << '\n';
	}
	return s;
}

template<typename State>
std::ostream &printTreeNode(std::ostream &s, const State &state, const typename State::TreeNode &node, bool explicitPartition) {
	typedef typename State::TreeNode TreeNode;
	s << "T<";
	std::stack<const TreeNode*> to_write;
	for(const TreeNode *n = &node; n->get_parent(); n = n->get_parent()) to_write.push(n);
	if(!to_write.empty()) {
		to_write.top();
		bool first = true;
		while(!to_write.empty()) {
			const TreeNode *n = to_write.top();
			to_write.pop();
			const TreeNode *parent = n->get_parent();
			std::size_t i;
			for(i = 0; i < parent->children.size(); i++) {
				if(parent->children[i] == n) break;
			}
			if(!first) s << " ";
			else first = false;
			if(i >= parent->children.size()) {
				s << "x"; // if the parent has not had its children initialised yet
			} else {
				s << parent->pi.get(i + parent->child_refiner_cell);
			}
			parent = n;
		}
	}
	s << ">";
	s << " pi cells: ";
	printPartitionCompressed(s, state, node.pi);
	if(explicitPartition) {
		printPartition(s << " pi = ", state, node.pi);
	}
	if(node.pi.get_num_cells() != state.n && node.child_refiner_cell < state.n) {
		s << ", tar = " << node.child_refiner_cell;
		s << ", children =";
		for(std::size_t i = node.child_refiner_cell; i < node.pi.get_cell_end(node.child_refiner_cell); i++) {
			if(node.child_pruned[i - node.child_refiner_cell]) s << " x";
			else s << " " << node.pi.get(i);
		}
	}
	return s;
}

} // namespace detail
} // namespace graph_canon

#endif /* GRAPH_CANON_DETAIL_IO_HPP */
