#ifndef GRAPH_CANON_CANONICALIZATION_VISITOR_STATS_HPP
#define GRAPH_CANON_CANONICALIZATION_VISITOR_STATS_HPP

#include <graph_canon/visitor/compound.hpp>
#include <graph_canon/detail/io.hpp>

#include <perm_group/group/io.hpp>

#include <iostream>
#include <string>

namespace graph_canon {

struct stats_visitor : null_visitor {
	using can_select_target_cell = std::false_type;

	struct tree_node_index_t {
	};

	template<typename Config, typename TreeNode>
	struct TreeNodeData {
		using type = tagged_list<tree_node_index_t, std::size_t>;
	};

	stats_visitor(std::ostream *tree_dump = nullptr) : tree_dump(tree_dump) { }

	~stats_visitor() {
		if(tree_dump) {
			std::ostream &s = *tree_dump;
			s << "digraph g {\nrankdir=LR\n";
			for(const Node &node : nodes) {
				s << node.id << " [ " << node.data;
				if(node.id == vIdCanon) s << "color=\"darkgreen\"";
				else if(node.has_been_canon) s << " color=\"green\"";
				else if(node.refine_abort) s << " color=\"red\"";
				else if(node.pruned) s << " color=\"purple\"";
				else if(node.worse) s << " color=\"brown\"";
				s << " ];\n";
				if(node.id > 0) {
					s << node.parentId << " -> " << node.id << " [ " << node.parentData << " ];\n";
				}
			}
			std::size_t iUnique = 0;
			for(auto e : automorphismEdges) {
				const std::size_t src = get<0>(e);
				const std::size_t tar = get<1>(e);
				s << "aut_" << src << "_" << iUnique << " [ color=\"gray\" label=\"aut = " << get<2>(e) << "\" ];\n";
				s << src << " -> aut_" << src << "_" << iUnique << ";\n";
				// tar == -1 means it was implicit
				if(tar != -1) s << "aut_" << src << "_" << iUnique << " -> " << tar << " [ constraint=false ];\n";
				++iUnique;
			}
			s << "}\n";
		}
	}

	template<typename State, typename TreeNode>
	bool tree_create_node_begin(const State &state, TreeNode &t) {
		using SizeType = typename State::SizeType;
		get(tree_node_index_t(), t.data) = num_tree_nodes;
		if(tree_dump) {
			Node node;
			node.level = t.level;
			node.id = num_tree_nodes;
			nodes.push_back(std::move(node));
		}
		++num_tree_nodes;
		max_root_distance = std::max(static_cast<SizeType> (max_root_distance), t.level);
		++cur_num_tree_nodes;
		max_num_tree_nodes = std::max(max_num_tree_nodes, cur_num_tree_nodes);
		return true;
	}

	template<typename State, typename TreeNode>
	void tree_destroy_node(State &state, TreeNode &t) {
		--cur_num_tree_nodes;
	}

	template<typename State, typename TreeNode>
	bool tree_create_node_end(const State &state, const TreeNode &t) {
		if(tree_dump) {
			Node &node = nodes.back();
			std::ostringstream s;
			s << "label=\"id=" << node.id << ", ";
			printPartition(s, state, t.pi) << "\"";
			node.data = s.str();
			if(t.get_parent()) {
				std::ostringstream s;
				s << "taillabel=\"" << t.pi.get(t.get_parent()->child_refiner_cell) << "\"";
				s << " headlabel=\"" << t.pi.get(t.get_parent()->child_refiner_cell) << "\"";
				s << " label=\"" << t.pi.get(t.get_parent()->child_refiner_cell) << "\"";
				node.parentId = get(tree_node_index_t(), t.get_parent()->data);
				node.parentData = s.str();
			}
		}
		return true;
	}

	template<typename State, typename TreeNode>
	void tree_leaf(const State &state, const TreeNode &t) {
		num_terminals++;
	}

	template<typename State, typename TreeNode>
	void tree_prune_node(const State &state, const TreeNode &t) {
		num_pruned++;
		if(tree_dump) {
			nodes[get(tree_node_index_t(), t.data)].pruned = true;
		}
	}

	template<typename State>
	void canon_new_best(const State &state) {
		if(tree_dump) {
			vIdCanon = get(tree_node_index_t(), state.get_canon_leaf()->data);
			nodes[vIdCanon].has_been_canon = true;
		}
		num_new_best_leaf++;
	}

	template<typename State, typename TreeNode>
	void canon_worse(const State &state, const TreeNode &t) {
		if(tree_dump) {
			auto id = get(tree_node_index_t(), t.data);
			nodes[id].worse = true;
		}
	}

	template<typename State>
	void canon_prune(State &state) {
		if(tree_dump) {
			auto id = get(tree_node_index_t(), state.get_canon_leaf()->data);
			nodes[id].canon_pruned = true;
		}
		num_canon_pruned++;
	}

	template<typename State, typename TreeNode, typename Perm>
	void automorphism_leaf(const State &state, const TreeNode &t, const Perm &aut) {
		num_explicit_automorphisms++;
		if(tree_dump) {
			std::ostringstream s;
			perm_group::write_permutation_cycles(s, aut);
			s << ", fixed={";
			for(std::size_t i = 0; i < perm_group::size(aut); ++i) {
				if(perm_group::get(aut, i) == i) s << " " << i;
			}
			s << " }";
			automorphismEdges.emplace_back(
					get(tree_node_index_t(), t.data),
					get(tree_node_index_t(), state.get_canon_leaf()->data),
					s.str());
		}
	}

	template<typename State, typename TreeNode, typename Perm>
	void automorphism_implicit(const State &state, const TreeNode &t, const Perm &aut, std::size_t tag) {
		num_implicit_automorphisms++;
		if(tree_dump) {
			std::ostringstream s;
			perm_group::write_permutation_cycles(s, aut);
			s << ", fixed={";
			for(std::size_t i = 0; i < perm_group::size(aut); ++i) {
				if(perm_group::get(aut, i) == i) s << " " << i;
			}
			s << " }, tag=" << tag;
			automorphismEdges.emplace_back(
					get(tree_node_index_t(), t.data),
					-1,
					s.str());
		}
	}

	template<typename State, typename TreeNode>
	RefinementResult refine(const State &state, const TreeNode &node) {
		num_refine++;
		return RefinementResult::Never;
	}

	template<typename State, typename TreeNode>
	void refine_abort(const State &state, const TreeNode &t) {
		num_refine_abort++;
		if(tree_dump) {
			nodes[get(tree_node_index_t(), t.data)].refine_abort = true;
		}
	}

	template<typename State, typename TreeNode>
	void trace_better(State &state, TreeNode &t) { }

	friend std::ostream &operator<<(std::ostream &s, const stats_visitor &vis) {
		s << "num_refine:                 " << vis.num_refine << '\n';
		s << "num_refine_abort:           " << vis.num_refine_abort << '\n';
		s << "num_tree_nodes:             " << vis.num_tree_nodes << '\n';
		s << "max_num_tree_nodes:         " << vis.max_num_tree_nodes << '\n';
		s << "num_terminals:              " << vis.num_terminals << '\n';
		s << "num_pruned:                 " << vis.num_pruned << '\n';
		s << "num_new_best_leaf:          " << vis.num_new_best_leaf << '\n';
		s << "num_canon_pruned:           " << vis.num_canon_pruned << '\n';
		s << "num_explicit_automorphisms: " << vis.num_explicit_automorphisms << '\n';
		s << "num_implicit_automorphisms: " << vis.num_implicit_automorphisms << '\n';
		s << "max_root_distance:          " << vis.max_root_distance << '\n';
		return s;
	}
public:

	struct Node {
		std::size_t level;
		std::size_t id, parentId;
		std::string data, parentData;
		bool has_been_canon = false, refine_abort = false, pruned = false, worse = false, canon_pruned = false;
	};
	std::vector<Node> nodes;
	std::vector<std::tuple<std::size_t, std::size_t, std::string> > automorphismEdges;
	std::ostream *tree_dump;
	std::size_t num_refine = 0, num_refine_abort = 0, num_tree_nodes = 0, num_terminals = 0, num_pruned = 0;
	std::size_t num_new_best_leaf = 0, num_canon_pruned = 0, num_explicit_automorphisms = 0, num_implicit_automorphisms = 0;
	std::size_t max_root_distance = 0;
	std::size_t vIdCanon;
	std::size_t cur_num_tree_nodes = 0;
	std::size_t max_num_tree_nodes = 0;
};

} // namespace graph_canon


#endif /* GRAPH_CANON_CANONICALIZATION_VISITOR_STATS_HPP */
