#ifndef GRAPH_CANON_REFINE_DEGREE_1_HPP
#define GRAPH_CANON_REFINE_DEGREE_1_HPP

#include <graph_canon/visitor/compound.hpp>

#include <cassert>
#include <utility>
#include <vector>

namespace graph_canon {

// This visitor relies on
// - the vertex_less predicate to be correct,
// - the edge_compare function to be correct, and
// - the vertices to be ordered by degree in ascending order.

// Cells consisting entirely of degree 1 vertices, where all vertices have the
// same neighbour, with the same type of edge, will be split into trivial cells.
// Either Never or Unchanged will be returned.

struct refine_degree_1 : null_visitor {
	static const std::size_t split_type_cell = 2; // these are good, right? so set it low
	static const std::size_t aut_tag_swap = 100;
	static const std::size_t aut_tag_cycle = 101;

	template<typename State, typename TreeNode>
	RefinementResult refine(State &state, TreeNode &node) {
		using SizeType = typename State::SizeType;
		using Graph = typename State::Graph;
		using Vertex = typename State::Vertex;
		using Edge = typename State::Edge;
		using Partition = typename State::Partition;

		const auto n = state.n;
		const Graph &g = state.g;
		auto &pi = node.pi;

		RefinementResult result = RefinementResult::Never;
		// Cache the cell end both because of efficiency,
		// but also because if we split the cell we should skip the new ones.
		SizeType cell_end = 0;
		std::vector<SizeType> aut; // for reporting
		for(SizeType cell_begin = 0; cell_begin != n; cell_begin = cell_end) {
			cell_end = pi.get_cell_end(cell_begin);
			const Vertex v_first = vertex(pi.get(cell_begin), g);
			const std::size_t d = degree(v_first, g);
			if(d > 1) continue;
			if(d == 0) continue;
			if(cell_end == cell_begin + 1) continue;
			result = RefinementResult::Unchanged;
			bool fits = true;
			const Edge e_first = *out_edges(v_first, g).first;
			const Vertex v_target_first = target(e_first, g);
			for(SizeType c = cell_begin + 1; c < cell_end; ++c) {
				const Vertex v = vertex(pi.get(c), g);
				assert(degree(v, g) == 1);
				const Edge e = *out_edges(v, g).first;
				const Vertex v_target = target(e, g);
				if(v_target_first != v_target) {
					fits = false;
					break;
				}
				const auto cmp = state.edge_handler.compare(state, e_first, e);
				if(cmp != 0) {
					fits = false;
					break;
				}
			}
			if(!fits) continue;
			state.visitor.refine_cell_split_begin(state, node, state.n, cell_begin, cell_end);
			{ // first do the splits
				auto raii_splitter = pi.split_cell(cell_begin);
				for(SizeType c = cell_begin + 1; c < cell_end; ++c)
					raii_splitter.add_split(c);
			} // and then report them
			for(std::size_t c = cell_begin + 1; c < cell_end; ++c) {
				bool abort = state.visitor.refine_new_cell(state, node, c, split_type_cell);
				if(!abort) return RefinementResult::Abort;
			}
			state.visitor.refine_cell_split_end(state, node, state.n, cell_begin, cell_end);

			// now report generating permutations for the symmetric group of the cell
			aut.resize(n);
			for(std::size_t i = 0; i < perm_group::size(aut); ++i)
				perm_group::put(aut, i, i);
			const std::size_t first = pi.get(cell_begin);
			{ // first the swap in the beginning
				const std::size_t second = pi.get(cell_begin + 1);
				perm_group::put(aut, first, second);
				perm_group::put(aut, second, first);
				state.visitor.automorphism_implicit(state, node, aut, aut_tag_swap);
			}
			// then the rotation of all elements, if needed
			if(cell_end - cell_begin > 2) {
				std::size_t prev = first;
				for(std::size_t c = cell_begin + 1; c < cell_end; ++c) {
					std::size_t cur = pi.get(c);
					perm_group::put(aut, prev, cur);
					prev = cur;
				}
				// and close the cycle:
				perm_group::put(aut, prev, first);
				state.visitor.automorphism_implicit(state, node, aut, aut_tag_cycle);
			}
		}
		// even though we may change the partition we do not report it, as it will not help any other refinement function
		return result;
	}
};

} // namespace graph_canon

#endif /* GRAPH_CANON_REFINE_DEGREE_1_HPP */