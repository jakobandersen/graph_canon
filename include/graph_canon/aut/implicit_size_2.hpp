#ifndef GRAPH_CANON_AUT_IMPLICIT_SIZE_2_HPP
#define GRAPH_CANON_AUT_IMPLICIT_SIZE_2_HPP

#include <graph_canon/visitor/compound.hpp>

#include <cassert>

//#define GRAPH_CANON_IMPLICIT_AUT_DEBUG

#ifdef GRAPH_CANON_IMPLICIT_AUT_DEBUG
#include <graph_canon/detail/io.hpp>
#include <perm_group/permutation/io.hpp>
#include <iostream>
#endif

namespace graph_canon {

struct aut_implicit_size_2 : null_visitor {
	static const std::size_t tag = 200;

	struct instance_data_t {
	};

	template<typename SizeType>
	struct instance_data {
		std::vector<SizeType> aut;
	};

	template<typename Config, typename TreeNode>
	struct InstanceData {
		using type = tagged_list<instance_data_t, instance_data<typename Config::SizeType> >;
	};

	struct tree_data_t {
	};

	struct tree_data {
		bool fits = false;
	};

	template<typename Config, typename TreeNode>
	struct TreeNodeData {
		using type = tagged_list<tree_data_t, tree_data>;
	};

	template<typename State>
	void initialize(State &state) {
		using SizeType = typename State::SizeType;
#ifdef GRAPH_CANON_IMPLICIT_AUT_DEBUG
		detail::printTreeNode(std::cout << "Implicit: initial resize, ", state, t, false) << std::endl;
#endif
		auto &instance = get(instance_data_t(), state.data);
		instance.aut.resize(state.n);
		for(SizeType i = 0; i < perm_group::size(instance.aut); ++i)
			perm_group::put(instance.aut, i, i);
	}

	template<typename State, typename TreeNode>
	bool tree_create_node_begin(State &state, TreeNode &t) {
		auto &t_data = get(tree_data_t(), t.data);
		const TreeNode * const parent = t.get_parent();
		if(!parent) return true;
		const auto &t_parent_data = get(tree_data_t(), t.get_parent()->data);
		t_data.fits = t_parent_data.fits;
		if(!t_data.fits) return true;
		const std::size_t parentTargetCell = parent->child_refiner_cell;
		assert(parent->pi.get_cell_end(parentTargetCell) == parentTargetCell + 2);
		assert(t.pi.get_cell_end(parentTargetCell) == parentTargetCell + 1);
		const std::size_t first = t.pi.get(parentTargetCell);
		const std::size_t second = t.pi.get(parentTargetCell + 1);
		auto &i_data = get(instance_data_t(), state.data);
		perm_group::put(i_data.aut, first, second);
		perm_group::put(i_data.aut, second, first);
#ifdef GRAPH_CANON_IMPLICIT_AUT_DEBUG
		std::cout << "Implicit: node_begin, parentCell=" << parentTargetCell << ", perm=";
		perm_group::write_permutation_cycles(std::cout, i_data.aut) << std::endl;
#endif
		return true;
	}

	template<typename State, typename TreeNode>
	bool refine_new_cell(State &state, TreeNode &t, std::size_t new_cell, std::size_t type) {
		const auto &t_data = get(tree_data_t(), t.data);
		if(!t_data.fits) return true;
		const TreeNode * const parent = t.get_parent();
		assert(parent);
		assert(new_cell > 0);
		const std::size_t parentCell = new_cell - 1;
		assert(parent->pi.get_cell_end(parentCell) == parentCell + 2);
		assert(t.pi.get_cell_end(parentCell) == parentCell + 1);
		assert(t.pi.get_cell_end(new_cell) == new_cell + 1);
		const std::size_t first = t.pi.get(new_cell - 1);
		const std::size_t second = t.pi.get(new_cell);
		auto &i_data = get(instance_data_t(), state.data);
#ifdef GRAPH_CANON_IMPLICIT_AUT_DEBUG
		std::cout << "Implicit: refine_new_cell before, parentCell=" << parentCell << ", "
				<< "(" << first << " " << second << "), perm=";
		perm_group::write_permutation_cycles(std::cout, i_data.aut) << std::endl;
#endif
		assert(perm_group::size(i_data.aut) == state.n);
		assert(perm_group::get(i_data.aut, first) == first);
		assert(perm_group::get(i_data.aut, second) == second);
		perm_group::put(i_data.aut, first, second);
		perm_group::put(i_data.aut, second, first);
		return true;
	}

	template<typename State, typename TreeNode>
	bool tree_create_node_end(State &state, TreeNode &t) {
		if(t.get_is_pruned()) {
			const auto &t_data = get(tree_data_t(), t.data);
			if(t_data.fits) {
#ifdef GRAPH_CANON_IMPLICIT_AUT_DEBUG
				instance_data<SizeType> &instance = get(instance_data_t(), state.data);
				std::cout << "Implicit: node_end pruned, should reset, perm=";
				perm_group::write_permutation_cycles(std::cout, instance.aut) << std::endl;
#endif
				auto &i_data = get(instance_data_t(), state.data);
				for(std::size_t i = 0; i < perm_group::size(i_data.aut); ++i)
					perm_group::put(i_data.aut, i, i);
			}
			return false;
		}
		auto &t_data = get(tree_data_t(), t.data);
		if(t_data.fits) {
			// report the automorphism
			auto &i_data = get(instance_data_t(), state.data);
			state.visitor.automorphism_implicit(state, t, i_data.aut, tag);
#ifdef GRAPH_CANON_IMPLICIT_AUT_DEBUG
			std::cout << "Implicit: node_end, perm reported, perm=";
			perm_group::write_permutation_cycles(std::cout, i_data.aut) << std::endl;
#endif
			for(std::size_t i = 0; i < perm_group::size(i_data.aut); ++i)
				perm_group::put(i_data.aut, i, i);
		}
		// we may have gotten it from the parent
		if(!t_data.fits && t.pi.get_num_cells() < state.n) {
			t_data.fits = true;
			std::size_t cell_end = 0; // init for the compiler to shut up
			for(std::size_t cell = 0; cell < state.n; cell = cell_end) {
				cell_end = t.pi.get_cell_end(cell);
				if(cell_end - cell > 2) {
					t_data.fits = false;
					break;
				}
			}
		}
		if(t_data.fits && t.pi.get_num_cells() < state.n) {
			// yay, prune all but one child
			assert(t.children.size() > 1);
			// first find an unpruned child, then prune the rest
			std::size_t child = 0;
			for(; child < t.children.size(); ++child) {
				if(!t.child_pruned[child]) break;
			}
			for(++child; child < t.children.size(); ++child) {
				t.child_pruned[child] = true;
				t.children[child] = nullptr;
			}
		}
		return true;
	}
};

} // namespace graph_canon

#endif /* GRAPH_CANON_AUT_IMPLICIT_SIZE_2_HPP */

