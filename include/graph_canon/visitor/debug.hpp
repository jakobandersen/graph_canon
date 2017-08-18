#ifndef GRAPH_CANON_CANONICALIZATION_VISITOR_DEBUG_HPP
#define GRAPH_CANON_CANONICALIZATION_VISITOR_DEBUG_HPP

#include <graph_canon/canonicalization.hpp>
#include <graph_canon/detail/io.hpp>

#include <perm_group/group/io.hpp>

#include <iomanip>
#include <iostream>
#include <stack>

namespace graph_canon {

struct debug_visitor : null_visitor {
	using can_select_target_cell = std::false_type;

	struct instance_data_t {
	};

	template<typename SizeType>
	struct instance_data {
		// for asserting correct usage of the cell splitting callbacks
		bool cell_splitting_in_progress = false;
		std::size_t num_nodes = 0;
		std::size_t cur_num_tree_nodes = 0;
	};

	template<typename Config, typename TreeNode>
	struct InstanceData {
		using type = tagged_list<instance_data_t, instance_data<typename Config::SizeType> >;
	};

	struct tree_node_index_t {
	};

	template<typename Config, typename TreeNode>
	struct TreeNodeData {
		using type = tagged_list<tree_node_index_t, std::size_t>;
	};
public:
	static constexpr std::size_t prefixWidth = 10;

	debug_visitor(bool tree, bool canon, bool aut, bool refine, bool compressed)
	: tree(tree), canon(canon), aut(aut), refine_(refine), compressed(compressed) { }

	template<typename State, typename TreeNode>
	bool tree_create_node_begin(State &state, TreeNode &t) {
		get(tree_node_index_t(), t.data) = get(instance_data_t(), state.data).num_nodes++;
		auto &i_data = get(instance_data_t(), state.data);
		++i_data.cur_num_tree_nodes;
		if(tree) {
			std::cout << std::setw(prefixWidth) << std::left << "Tree";
			std::cout << "(Before initialisation) ";
			std::cout << "id=" << get(tree_node_index_t(), t.data) << " ";
			detail::printTreeNode(std::cout, state, t, !compressed) << std::endl;
			std::cout << std::setw(prefixWidth) << std::left << "Tree";
			std::cout << "Current number of nodes = " << i_data.cur_num_tree_nodes << std::endl;
		}
		return true;
	}

	template<typename State, typename TreeNode>
	bool tree_create_node_end(const State &state, const TreeNode &t) {
		if(tree) {
			std::cout << std::setw(prefixWidth) << std::left << "Tree";
			std::cout << "id=" << get(tree_node_index_t(), t.data) << " ";
			detail::printTreeNode(std::cout, state, t, !compressed) << std::endl;
		}
		return true;
	}

	template<typename State, typename TreeNode>
	void tree_destroy_node(State &state, const TreeNode &t) {
		if(tree) {
			std::cout << std::setw(prefixWidth) << std::left << "Tree";
			std::cout << "Destroy, id=" << get(tree_node_index_t(), t.data) << " ";
			detail::printTreeNode(std::cout, state, t, !compressed) << std::endl;
		}
		--get(instance_data_t(), state.data).cur_num_tree_nodes;
	}

	template<typename State, typename TreeNode>
	void tree_before_descend(const State &state, const TreeNode &t) {
		if(tree) {
			std::cout << std::setw(prefixWidth) << std::left << "Tree";
			std::cout << "Descend, ";
			std::cout << "id=" << get(tree_node_index_t(), t.data) << " ";
			detail::printTreeNode(std::cout, state, t, !compressed) << std::endl;
		}
	}

	template<typename State, typename TreeNode>
	void tree_create_child(const State &state, const TreeNode &t, std::size_t element_idx_to_individualise) {
		if(tree) {
			auto child_idx = element_idx_to_individualise - t.child_refiner_cell;
			std::cout << std::setw(prefixWidth) << std::left << "Tree";
			std::cout << "Individualize idx=" << element_idx_to_individualise << ", v=" << t.pi.get(element_idx_to_individualise)
					<< " (child idx=" << child_idx << ") of ";
			std::cout << "id=" << get(tree_node_index_t(), t.data) << " ";
			detail::printTreeNode(std::cout, state, t, !compressed) << std::endl;
		}
	}

	template<typename State, typename TreeNode>
	void tree_leaf(const State &state, const TreeNode &t) {
		if(tree) {
			std::cout << std::setw(prefixWidth) << std::left << "Tree";
			if(!state.get_canon_leaf()) std::cout << "First leaf" << std::endl;
			std::cout << "Leaf: ";
			std::cout << "id=" << get(tree_node_index_t(), t.data) << " ";
			detail::printTreeNode(std::cout, state, t, !compressed) << std::endl;
		}
	}

	template<typename State, typename TreeNode>
	void tree_prune_node(const State &state, const TreeNode &t) {
		if(tree) {
			std::cout << std::setw(prefixWidth) << std::left << "Tree";
			std::cout << "Prune ";
			std::cout << "id=" << get(tree_node_index_t(), t.data) << " ";
			detail::printTreeNode(std::cout, state, t, !compressed) << std::endl;
		}
	}

	template<typename State>
	void canon_new_best(const State &state) {
		if(canon) {
			std::cout << std::setw(prefixWidth) << std::left << "Canon";
			std::cout << "New best leaf!" << std::endl;
		}
	}

	template<typename State, typename TreeNode>
	void canon_worse(const State &state, const TreeNode &t) {
		if(canon) {
			std::cout << std::setw(prefixWidth) << std::left << "Canon";
			std::cout << "Worse ";
			std::cout << "id=" << get(tree_node_index_t(), t.data) << " ";
			detail::printTreeNode(std::cout, state, t, !compressed) << std::endl;
		}
	}

	template<typename State>
	void canon_prune(State &state) {
		if(canon) {
			std::cout << std::setw(prefixWidth) << std::left << "Canon";
			std::cout << "Pruned" << std::endl;
		}
	}

	template<typename State, typename TreeNode, typename Perm>
	void automorphism_leaf(const State &state, const TreeNode &t, const Perm &aut) {
		if(this->aut) {
			std::cout << std::setw(prefixWidth) << std::left << "Aut";
			std::cout << "explicit:      ";
			perm_group::write_permutation_cycles(std::cout, aut);
			std::cout << std::endl;
		}
	}

	template<typename State, typename TreeNode, typename Perm>
	void automorphism_implicit(const State &state, const TreeNode &t, const Perm &aut, std::size_t tag) {
		if(this->aut) {
			std::cout << std::setw(prefixWidth) << std::left << "Aut";
			std::cout << "implicit(" << std::setw(3) << tag;
			std::cout << "): ";
			perm_group::write_permutation_cycles(std::cout, aut);
			std::cout << std::endl;
		}
	}

	template<typename State, typename TreeNode>
	void refine_cell_split_begin(State &state, const TreeNode &t, std::size_t refiner_begin,
			std::size_t refinee_begin, std::size_t refinee_end) {
		auto &data = get(instance_data_t(), state.data);
		assert(!data.cell_splitting_in_progress);
		data.cell_splitting_in_progress = true;

		if(refine_) {
			const std::size_t max_display_size = 50;
			if(refiner_begin == state.n) {
				std::cout << "\t\trefiner: n/a" << std::endl;
			} else {
				std::size_t refiner_end = t.pi.get_cell_end(refiner_begin);
				std::size_t refiner_size = refiner_end - refiner_begin;
				std::cout << "\t\trefiner: " << refiner_begin << ":" << refiner_size;
				if(refiner_size <= max_display_size) {
					std::cout << ",";
					for(std::size_t i_refiner = refiner_begin; i_refiner < refiner_end; i_refiner++)
						std::cout << " " << t.pi.get(i_refiner);
				}
				std::cout << std::endl;
			}
			std::size_t refinee_size = refinee_end - refinee_begin;
			std::cout << "\t\trefinee: " << refinee_begin << ":" << refinee_size;
			if(refinee_size <= max_display_size) {
				std::cout << ",";
				for(std::size_t i_refinee = refinee_begin; i_refinee < refinee_end; i_refinee++)
					std::cout << " " << t.pi.get(i_refinee);
			}
			std::cout << std::endl;
			// TODO: can we make this work generically?
			//      const auto &vertex_invariants = state.vertex_invariants;
			//      std::cout << "\t\tinvariant:";
			//      for(std::size_t i_refinee = refinee_begin; i_refinee < refinee_end; i_refinee++)
			//        std::cout << " " << vertex_invariants[i_refinee - refinee_begin].second;
			//      for(std::size_t i_refinee = refinee_begin; i_refinee < refinee_end; i_refinee++)
			//        std::cout << " " << vertex_invariants[i_refinee - refinee_begin].first; // << "(" << vertex_invariants[i_refinee - refinee_begin].second << ")";
			//      std::cout << std::endl;
			if(compressed)
				detail::printPartitionCompressed(std::cout << "\t\tcurrent cells: ", state, t.pi) << std::endl;
			else
				detail::printPartition(std::cout << "\t\tcurrent: ", state, t.pi) << std::endl;
		}
	}

	template<typename State, typename TreeNode>
	bool refine_new_cell(const State &state, const TreeNode &t, std::size_t new_cell, std::size_t type) {
		auto &data = get(instance_data_t(), state.data);
		if(refine_) {
			std::cout << "\t\trefine_new_cell: " << new_cell << std::endl;
		}
		assert(data.cell_splitting_in_progress);
		return true;
	}

	template<typename State, typename TreeNode>
	void refine_cell_split_end(State &state, const TreeNode &t, std::size_t refiner_begin,
			std::size_t refinee_begin, std::size_t refinee_end) {
		auto &data = get(instance_data_t(), state.data);
		assert(data.cell_splitting_in_progress);
		data.cell_splitting_in_progress = false;
	}

	template<typename State, typename TreeNode>
	void refine_abort(State &state, const TreeNode &t) {
		auto &data = get(instance_data_t(), state.data);
		data.cell_splitting_in_progress = false;
	}
public:
	const bool tree, canon, aut, refine_, compressed;
};

} // namespace graph_canon

#endif /* GRAPH_CANON_CANONICALIZATION_VISITOR_DEBUG_HPP */
