#ifndef GRAPH_CANON_TREE_TRAVERSAL_DFS_HPP
#define GRAPH_CANON_TREE_TRAVERSAL_DFS_HPP

#include <graph_canon/visitor/compound.hpp>

namespace graph_canon {

struct traversal_dfs : null_visitor {
	using can_explore_tree = std::true_type;

	template<typename SizeType, typename TreeNode>
	struct elem {

		elem(const typename TreeNode::OwnerPtr &node, SizeType next_child)
		: node(node), next_child(next_child) { }
	public:
		typename TreeNode::OwnerPtr node;
		SizeType next_child;
	};

	template<typename State>
	static void explore_tree(State &state) {
		using SizeType = typename State::SizeType;
		using TreeNode = typename State::TreeNode;
		using Elem = elem<SizeType, TreeNode>;
		std::vector<Elem> work_stack;
		work_stack.reserve(state.n);
		traverse(state, work_stack, state.root);
	}

	template<typename State>
	static void traverse(State &state, auto &work_stack, auto t_ptr) {
		using TreeNode = typename State::TreeNode;
		work_stack.emplace_back(t_ptr, 0);
		while(!work_stack.empty()) {
			typename TreeNode::OwnerPtr parent = work_stack.back().node;
			std::size_t next_child_index = work_stack.back().next_child;
			work_stack.pop_back();
			// update the node
			state.visitor.tree_before_descend(state, *parent);
			// skip if pruned
			if(parent->get_is_pruned()) continue;
			// skip pruned children
			while(next_child_index < parent->children.size() && parent->child_pruned[next_child_index])
				next_child_index++;
			if(next_child_index < parent->children.size()) {
				// we still have children to explore
				work_stack.emplace_back(parent, next_child_index + 1);
				typename TreeNode::OwnerPtr child = parent->create_child(next_child_index + parent->child_refiner_cell, state);
				// if the child got pruned already, don't add it
				if(child) work_stack.emplace_back(child, 0);
			} else if(next_child_index == 0) {
				// this is actually a leaf node
				assert(parent->pi.get_num_cells() == state.n);
				state.add_terminal(parent);
			}
		}
	}
};

} // namespace graph_canon

#endif /* GRAPH_CANON_TREE_TRAVERSAL_DFS_HPP */