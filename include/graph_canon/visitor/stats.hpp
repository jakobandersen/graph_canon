#ifndef GRAPH_CANON_CANONICALIZATION_VISITOR_STATS_HPP
#define GRAPH_CANON_CANONICALIZATION_VISITOR_STATS_HPP

#include <graph_canon/visitor/visitor.hpp>
#include <graph_canon/detail/io.hpp>

#include <perm_group/group/io.hpp>

#include <iostream>
#include <string>

namespace graph_canon {

// rst: .. class:: stats_visitor
// rst:
// rst:		A visitor for counting stats and extracting visualizations.
// rst:

struct stats_visitor : null_visitor {
	// rst:		.. class:: result_t
	// rst:
	// rst:			The tag type used for returning data.

	struct result_t {
	};

	struct Node {
		std::size_t level;
		std::size_t id, parentId;
		std::string data, parentData;
		bool has_been_canon = false, refine_abort = false, pruned = false, worse = false, canon_pruned = false;
	};

	// rst:		.. class:: ResultData
	// rst:
	// rst:			The results returned.
	// rst:
	// rst:			.. todo:: document all members.
	// rst:

	struct ResultData {
		std::vector<Node> nodes;
		std::vector<std::tuple<std::size_t, std::size_t, std::string> > automorphismEdges;
		std::size_t num_refine = 0, num_refine_abort = 0, num_tree_nodes = 0, num_terminals = 0, num_pruned = 0;
		std::size_t num_new_best_leaf = 0, num_canon_pruned = 0, num_explicit_automorphisms = 0, num_implicit_automorphisms = 0;
		std::size_t max_root_distance = 0;
		std::size_t vIdCanon;
		std::size_t cur_num_tree_nodes = 0;
		std::size_t max_num_tree_nodes = 0;
	public:

		// rst:			.. function:: friend std::ostream &operator<<(std::ostream &s, const ResultData &d)

		friend std::ostream &operator<<(std::ostream &s, const ResultData &d) {
			s << "num_refine:                 " << d.num_refine << '\n';
			s << "num_refine_abort:           " << d.num_refine_abort << '\n';
			s << "num_tree_nodes:             " << d.num_tree_nodes << '\n';
			s << "max_num_tree_nodes:         " << d.max_num_tree_nodes << '\n';
			s << "num_terminals:              " << d.num_terminals << '\n';
			s << "num_pruned:                 " << d.num_pruned << '\n';
			s << "num_new_best_leaf:          " << d.num_new_best_leaf << '\n';
			s << "num_canon_pruned:           " << d.num_canon_pruned << '\n';
			s << "num_explicit_automorphisms: " << d.num_explicit_automorphisms << '\n';
			s << "num_implicit_automorphisms: " << d.num_implicit_automorphisms << '\n';
			s << "max_root_distance:          " << d.max_root_distance << '\n';
			return s;
		}
	};

	struct tree_node_index_t {
	};

	template<typename Config, typename TreeNode>
	struct TreeNodeData {
		using type = tagged_element<tree_node_index_t, std::size_t>;
	};

	template<typename Config, typename TreeNode>
	struct InstanceData {
		using type = tagged_element<result_t, ResultData>;
	};
private:

	template<typename State>
	ResultData &data(State &state) {
		return get(result_t(), state.data);
	}

	template<typename State>
	const ResultData &data(const State &state) {
		return get(result_t(), state.data);
	}
public:

	// rst:		.. function:: stats_visitor(std::ostream *tree_dump = nullptr)
	// rst:
	// rst:			:param tree_dump: print a Grahpviz depiction of the search tree to this stream.

	stats_visitor(std::ostream *tree_dump = nullptr) : tree_dump(tree_dump) { }

	template<typename State>
	auto extract_result(State &state) {
		if(tree_dump) {
			auto &d = data(state);
			std::ostream &s = *tree_dump;
			s << "digraph g {\nrankdir=LR\n";
			for(const Node &node : d.nodes) {
				s << node.id << " [ " << node.data;
				if(node.id == d.vIdCanon) s << "color=\"darkgreen\"";
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
			for(auto e : d.automorphismEdges) {
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
		return tagged_element<result_t, ResultData>{std::move(data(state))};
	}

	template<typename State, typename TreeNode>
	bool tree_create_node_begin(State &state, TreeNode &t) {
		using SizeType = typename State::SizeType;
		auto &d = data(state);
		get(tree_node_index_t(), t.data) = d.num_tree_nodes;
		if(tree_dump) {
			Node node;
			node.level = t.level;
			node.id = d.num_tree_nodes;
			d.nodes.push_back(std::move(node));
		}
		++d.num_tree_nodes;
		d.max_root_distance = std::max(static_cast<SizeType> (d.max_root_distance), t.level);
		++d.cur_num_tree_nodes;
		d.max_num_tree_nodes = std::max(d.max_num_tree_nodes, d.cur_num_tree_nodes);
		return true;
	}

	template<typename State, typename TreeNode>
	void tree_destroy_node(State &state, TreeNode &t) {
		--data(state).cur_num_tree_nodes;
	}

	template<typename State, typename TreeNode>
	bool tree_create_node_end(State &state, const TreeNode &t) {
		if(tree_dump) {
			auto &d = data(state);
			Node &node = d.nodes.back();
			std::ostringstream s;
			s << "label=\"id=" << node.id << ", ";
			printPartition(s, state, t.pi) << "\"";
			node.data = s.str();
			if(t.get_parent()) {
				std::ostringstream s;
				s << "taillabel=\"" << t.pi.get(t.get_parent()->get_child_refiner_cell()) << "\"";
				s << " headlabel=\"" << t.pi.get(t.get_parent()->get_child_refiner_cell()) << "\"";
				s << " label=\"" << t.pi.get(t.get_parent()->get_child_refiner_cell()) << "\"";
				node.parentId = get(tree_node_index_t(), t.get_parent()->data);
				node.parentData = s.str();
			}
		}
		return true;
	}

	template<typename State, typename TreeNode>
	void tree_leaf(State &state, const TreeNode &t) {
		++data(state).num_terminals;
	}

	template<typename State, typename TreeNode>
	void tree_prune_node(State &state, const TreeNode &t) {
		++data(state).num_pruned;
		if(tree_dump) {
			data(state).nodes[get(tree_node_index_t(), t.data)].pruned = true;
		}
	}

	template<typename State, typename TreeNode>
	void canon_new_best(State &state, const TreeNode *previous) {
		auto &d = data(state);
		if(tree_dump) {
			d.vIdCanon = get(tree_node_index_t(), state.get_canon_leaf()->data);
			d.nodes[d.vIdCanon].has_been_canon = true;
		}
		++d.num_new_best_leaf;
	}

	template<typename State, typename TreeNode>
	void canon_worse(State &state, const TreeNode &t) {
		if(tree_dump) {
			auto id = get(tree_node_index_t(), t.data);
			data(state).nodes[id].worse = true;
		}
	}

	template<typename State>
	void canon_prune(State &state) {
		auto &d = data(state);
		if(tree_dump) {
			auto id = get(tree_node_index_t(), state.get_canon_leaf()->data);
			d.nodes[id].canon_pruned = true;
		}
		++d.num_canon_pruned;
	}

	template<typename State, typename TreeNode, typename Perm>
	void automorphism_leaf(State &state, const TreeNode &t, const Perm &aut) {
		auto &d = data(state);
		++d.num_explicit_automorphisms;
		if(tree_dump) {
			std::ostringstream s;
			perm_group::write_permutation_cycles(s, aut);
			s << ", fixed={";
			for(std::size_t i = 0; i < perm_group::degree(aut); ++i) {
				if(perm_group::get(aut, i) == i) s << " " << i;
			}
			s << " }";
			d.automorphismEdges.emplace_back(
					get(tree_node_index_t(), t.data),
					get(tree_node_index_t(), state.get_canon_leaf()->data),
					s.str());
		}
	}

	template<typename State, typename TreeNode, typename Perm>
	void automorphism_implicit(State &state, const TreeNode &t, const Perm &aut, std::size_t tag) {
		auto &d = data(state);
		++d.num_implicit_automorphisms;
		if(tree_dump) {
			std::ostringstream s;
			perm_group::write_permutation_cycles(s, aut);
			s << ", fixed={";
			for(std::size_t i = 0; i < perm_group::degree(aut); ++i) {
				if(perm_group::get(aut, i) == i) s << " " << i;
			}
			s << " }, tag=" << tag;
			d.automorphismEdges.emplace_back(
					get(tree_node_index_t(), t.data),
					-1,
					s.str());
		}
	}

	template<typename State, typename TreeNode>
	RefinementResult refine(State &state, const TreeNode &node) {
		++data(state).num_refine;
		return RefinementResult::Never;
	}

	template<typename State, typename TreeNode>
	void refine_abort(State &state, const TreeNode &t) {
		++data(state).num_refine_abort;
		if(tree_dump) {
			data(state).nodes[get(tree_node_index_t(), t.data)].refine_abort = true;
		}
	}
public:
	std::ostream *tree_dump;
};

} // namespace graph_canon


#endif /* GRAPH_CANON_CANONICALIZATION_VISITOR_STATS_HPP */
