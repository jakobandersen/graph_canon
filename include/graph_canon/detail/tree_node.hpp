#ifndef GRAPH_CANON_TREE_NODE_HPP
#define GRAPH_CANON_TREE_NODE_HPP

#include <graph_canon/detail/partition.hpp>

#include <boost/intrusive_ptr.hpp>

#include <cassert>

namespace graph_canon {
namespace detail {

//------------------------------------------------------------------------------
// tree_node
//------------------------------------------------------------------------------

template<typename SizeType, typename DataGenerator>
struct tree_node {
	using Self = tree_node<SizeType, DataGenerator>;
	using Data = typename DataGenerator::template apply<Self>::type;
	using Partition = partition<SizeType>;
	using OwnerPtr = boost::intrusive_ptr<Self>;
private:

	tree_node(Self *parent, Partition &&pi_not_equitable, auto &state)
	: ref_count(0), parent(parent), level(parent ? parent->level + 1 : 0),
	pi(std::move(pi_not_equitable)), child_refiner_cell(state.n), is_pruned(false), data(state) {
		bool isCandidate = state.visitor.tree_create_node_begin(state, *this);
		if(isCandidate) {
			isCandidate = state.make_equitable(*this);
			if(isCandidate && pi.get_num_cells() != state.n) {
				child_refiner_cell = state.select_target_cell(*this);
				assert(child_refiner_cell != state.n);
				children.resize(pi.get_cell_size(child_refiner_cell), nullptr);
				child_pruned.resize(children.size(), false);
			}
		} else {
			state.visitor.refine_abort(state, *this);
		}
		is_pruned = !isCandidate;
		// don't do cleanup here, the caller will obtain ownership
		const bool continue_ = state.visitor.tree_create_node_end(state, *this);
		if(!continue_) {
			is_pruned = true;
			state.visitor.refine_abort(state, *this);
		}
		if(is_pruned) assert(get_parent());
	}

	~tree_node() {
		data.state.visitor.tree_destroy_node(data.state, *this);
		for(SizeType i = 0; i < children.size(); i++) assert(!children[i]);
		if(!parent) return;
		// first find our index into our parent's child list
		const SizeType ind_idx = parent->child_refiner_cell;
		const SizeType ind_element = pi.get(ind_idx);
		const SizeType ind_element_idx = parent->pi.get_inverse(ind_element);
		assert(ind_element_idx >= ind_idx);
		const SizeType child_offset = ind_element_idx - ind_idx;
		// mark it pruned, and set it null
		parent->child_pruned[child_offset] = true;
		parent->children[child_offset] = nullptr;
	}
public:

	static OwnerPtr make(Self *parent, Partition &&pi_not_equitable, auto &state) {
		return OwnerPtr(new Self(parent, std::move(pi_not_equitable), state));
	}

	static OwnerPtr make(Partition &&pi_not_equitable, auto &state) {
		return OwnerPtr(new Self(nullptr, std::move(pi_not_equitable), state));
	}

	Self *get_parent() {
		return parent.get();
	}

	const Self *get_parent() const {
		return parent.get();
	}

	bool get_is_pruned() const {
		return is_pruned;
	}

	template<typename State>
	OwnerPtr create_child(std::size_t element_idx_to_individualise, State &state) {
		SizeType child_idx = element_idx_to_individualise - child_refiner_cell;
		assert(child_idx < children.size());
		assert(!children[child_idx]);
		assert(!child_pruned[child_idx]);
		state.visitor.tree_create_child(state, *this, element_idx_to_individualise);

		assert(element_idx_to_individualise < state.n);
		assert(child_refiner_cell < state.n);
		assert(element_idx_to_individualise >= child_refiner_cell);
		assert(element_idx_to_individualise < pi.get_cell_end(child_refiner_cell));

		Partition pi_child(pi);
		// swap the vertex to be individualised down to the beginning
		pi_child.put_element_on_index(pi.get(element_idx_to_individualise), child_refiner_cell);
		pi_child.put_element_on_index(pi.get(child_refiner_cell), element_idx_to_individualise);
		// and split the cell
		{
			std::size_t cell_end = pi_child.get_cell_end(child_refiner_cell);
			pi_child.define_cell(child_refiner_cell, child_refiner_cell + 1);
			pi_child.define_cell(child_refiner_cell + 1, cell_end);
			pi_child.set_element_to_cell(child_refiner_cell + 1);
		}

		OwnerPtr childPtr = make(this, std::move(pi_child), state);
		assert(childPtr);
		if(childPtr->get_is_pruned()) {
			child_pruned[child_idx] = true;
			return nullptr;
		} else {
			children[child_idx] = childPtr.get();
			return childPtr;
		}
	}

	void prune_subtree(auto &state, const bool allow_canon_leaf_pruning = false) {
		if(is_pruned) return;
		assert(get_parent());
		OwnerPtr self(this); // make sure no one kills us before we are done
		(void) self; // stop compiler warnings
		if(allow_canon_leaf_pruning) {
			for(auto *t = state.get_canon_leaf(); t; t = t->get_parent()) {
				if(t == this) {
					state.prune_canon_leaf();
					break;
				}
			}
		}
		state.visitor.tree_prune_node(state, *this);
		std::fill(child_pruned.begin(), child_pruned.end(), true);
		for(Self *child : children) {
			if(child) child->prune_subtree(state);
		}
		// someone should have reset that (maybe us, earlier)
		assert(this != state.get_canon_leaf());
		is_pruned = true;
	}
public:

	friend void intrusive_ptr_add_ref(Self *t) {
		++t->ref_count;
	}

	friend void intrusive_ptr_release(Self *t) {
		assert(t->ref_count > 0);
		--t->ref_count;
		if(t->ref_count == 0) delete t;
	}
public: // for debugging

	std::size_t get_ref_count() const {
		return ref_count;
	}
private:
	std::size_t ref_count;
	OwnerPtr parent;
public:
	const SizeType level; // distance to root
	Partition pi;
	SizeType child_refiner_cell; // the cell in the partition which defines the children of the node
	// these two arrays have the same size
	// they are empty if the node is pruned OR is a leaf
	std::vector<Self*> children; // a nullptr child just means it hasn't been explored yet
	std::vector<bool> child_pruned;
private:
	bool is_pruned;
public:
	Data data;
};

} // namespace detail
} // namespace graph_canon

#endif /* GRAPH_CANON_TREE_NODE_HPP */
