#ifndef GRAPH_CANON_TREE_TRAVERSAL_BFS_EXP_M_HPP
#define GRAPH_CANON_TREE_TRAVERSAL_BFS_EXP_M_HPP

#include <graph_canon/tagged_list.hpp>

#include <graph_canon/tree_traversal/dfs.hpp>
#include <graph_canon/detail/visitor_utils.hpp>

#include <cassert>
#include <cstddef>
#include <utility>

namespace graph_canon {

struct traversal_bfs_exp_m : null_visitor {
	using can_explore_tree = std::true_type;

	struct instance_data_t {
	};

	template<typename SizeType, typename TreeNode>
	struct instance_data {
		std::size_t num_nodes = 0;
		std::size_t max_num_nodes;
		bool use_dfs = false;
		std::vector<traversal_dfs::elem<SizeType, TreeNode> > dfs_stack;
	};

	template<typename Config, typename TreeNode>
	struct InstanceData {
		using type = tagged_list<instance_data_t, instance_data<typename Config::SizeType, TreeNode> >;
	};

	struct tree_data_t {
	};

	template<typename TreeNode>
	struct tree_data {
		boost::intrusive_ptr<TreeNode> level_next; // next in the same layer during BFS
		boost::intrusive_ptr<TreeNode> keep_alive; // self pointer when need to be kept alive
		bool is_dfs_root = false;
	};

	template<typename Config, typename TreeNode>
	struct TreeNodeData {
		using type = tagged_list<tree_data_t, tree_data<TreeNode> >;
	};

public:

	traversal_bfs_exp_m(const std::size_t max_mem_mb) : max_mem_mb(max_mem_mb) { }
private:

	template<typename OwnerPtr>
	static void keep_alive_begin(OwnerPtr node) {
		get(tree_data_t(), node->data).keep_alive = node;
	}

	template<typename TreeNode>
	static void keep_alive_end(TreeNode &t) {
		get(tree_data_t(), t.data).keep_alive = nullptr;
	}
public:

	template<typename State>
	void initialize(State &state) {
		auto &i_data = get(instance_data_t(), state.data);
		const auto mem_per_node = state.n * 4 * sizeof (typename State::SizeType);
		i_data.max_num_nodes = max_mem_mb * 1024 * 1024 / mem_per_node;
		if(i_data.max_num_nodes == 0) i_data.max_num_nodes = 1;
	}

	template<typename State, typename TreeNode>
	bool tree_create_node_begin(State &state, TreeNode &t) {
		auto &i_data = get(instance_data_t(), state.data);
		++i_data.num_nodes;
		if(i_data.num_nodes == i_data.max_num_nodes) {
			i_data.use_dfs = true;
		}
		return true;
	}

	template<typename State, typename TreeNode>
	void tree_destroy_node(State &state, TreeNode &t) {
		auto &i_data = get(instance_data_t(), state.data);
		if(i_data.num_nodes == i_data.max_num_nodes) {
			i_data.use_dfs = false;
		}
		--i_data.num_nodes;
	}

	template<typename State, typename TreeNode>
	void tree_prune_node(const State &state, TreeNode &t) {
		keep_alive_end(t);
	}

	template<typename State>
	static void explore_tree(State &state) {
		using TreeNode = typename State::TreeNode;
		using OwnerPtr = typename TreeNode::OwnerPtr;
		const auto makeExperimentalPath = [&state](OwnerPtr node) {
			while(true) {
				assert(!node->get_is_pruned());
				auto &i_data = get(instance_data_t(), state.data);
				if(i_data.use_dfs) {
					get(tree_data_t(), node->data).is_dfs_root = true;
					i_data.dfs_stack.reserve(state.n);
					traversal_dfs::traverse(state, i_data.dfs_stack, node);
					break;
				}
				state.visitor.tree_before_descend(state, *node);
				// search for a child: not pruned, not existing, and isn't immediately pruned in the construction
				if(node->children.empty()) {
					// this is actually a leaf node
					assert(node->pi.get_num_cells() == state.n);
					state.add_terminal(node);
					return;
				}
				std::size_t next_child_index = 0;
				OwnerPtr child;
				for(; next_child_index < node->children.size(); ++next_child_index) {
					if(node->child_pruned[next_child_index]) continue;
					if(node->children[next_child_index]) continue;
					child = node->create_child(next_child_index + node->child_refiner_cell, state);
					if(child) break; // yay
				}
				if(child) {
					// extend the path
					keep_alive_begin(node);
					node = child;
				} else {
					// We could have children, but they were all pruned.
					// No longer interesting, so don't keep alive.
					return; // TODO: maybe do a dfs for leaf instead, although that kind of is against the idea of an experimental path. The whole subtree might be explored
				}
			}
		};

		makeExperimentalPath(state.root);
		OwnerPtr head = state.root;
		OwnerPtr headChildren;
		while(head) {
			OwnerPtr *prevChildNext = &headChildren;
			OwnerPtr nodeNext = nullptr;
			for(OwnerPtr node = head; node; node = nodeNext) {
				auto &t_data = get(tree_data_t(), node->data);
				nodeNext = t_data.level_next;
				// release from the level list
				t_data.level_next = nullptr;
				// and from it self, if it were kept alive
				keep_alive_end(*node);
				if(node->get_is_pruned()) continue;
				state.visitor.tree_before_descend(state, *node);
				if(node->get_is_pruned()) continue;
				if(node->children.empty()) {
					continue; // just skip it, it was processed in makeExperimentalPath
				}
				for(std::size_t next_child_index = 0; next_child_index < node->children.size(); ++next_child_index) {
					// the experimental paths we create might, for example, discover automorphisms
					state.visitor.tree_before_descend(state, *node);
					if(node->get_is_pruned()) break;
					if(node->child_pruned[next_child_index]) continue;
					OwnerPtr child = node->children[next_child_index];
					// if it doesn't exist, try to create the child
					if(!child) {
						child = node->create_child(next_child_index + node->child_refiner_cell, state);
						// the child could already be pruned
						if(child) {
							makeExperimentalPath(child);
							if(node->get_is_pruned()) break;
						}
					}
					// it may have not been created due to pruning during construction
					if(child) {
						if(!get(tree_data_t(), child->data).is_dfs_root) {
							*prevChildNext = child;
							prevChildNext = &get(tree_data_t(), child->data).level_next;
						}
					}
				}
			}
			*prevChildNext = nullptr;
			std::swap(head, headChildren);
		}
	}
private:
	const std::size_t max_mem_mb;
};

} // namespace graph_canon

#endif /* GRAPH_CANON_TREE_TRAVERSAL_BFS_EXP_M_HPP */