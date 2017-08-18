#ifndef GRAPH_CANON_AUT_PRUNER_BASE_HPP
#define GRAPH_CANON_AUT_PRUNER_BASE_HPP

#include <graph_canon/visitor/compound.hpp>

#include <cassert>

//#define GRAPH_CANON_AUT_BASE_DEBUG

#ifdef GRAPH_CANON_AUT_BASE_DEBUG
#include <graph_canon/detail/io.hpp>
#include <perm_group/permutation/io.hpp>
#include <iostream>
#endif

namespace graph_canon {

// aut_range
// -----------------------------------------------------------------------------

template<typename Iter>
struct aut_range {

	aut_range(Iter first, Iter last) : first(first), last(last) { }

	Iter begin() const {
		return first;
	}

	Iter end() const {
		return last;
	}
private:
	Iter first, last;
};

template<typename Iter>
auto make_aut_range(Iter first, Iter last) {
	return aut_range<Iter>(first, last);
}

// wrapped perm
// -----------------------------------------------------------------------------

template<typename Perm>
struct wrapped_perm {
	using value_type = typename Perm::value_type;
public:

	wrapped_perm(value_type n) : perm(n) { }

	explicit wrapped_perm(Perm p) : perm(std::move(p)) {
		value_type num_moved = 0;
		for(value_type i = 0; i != perm_group::size(perm); ++i)
			if(perm_group::get(perm, i) != i)
				++num_moved;
		if(num_moved <= perm_group::size(perm) / 2) {
			moved_points.reserve(num_moved);
			for(value_type i = 0; i != perm_group::size(perm); ++i)
				if(perm_group::get(perm, i) != i)
					moved_points.push_back(i);
		}
	}
public:

	value_type get_(value_type i) const {
		return perm_group::get(perm, i);
	}

	void put_(value_type i, value_type image) {
		perm_group::put(perm, i, image);
	}

	value_type size_() const {
		return perm_group::size(perm);
	}
public:

	const Perm &get_perm() const {
		return perm;
	}

	const std::vector<value_type> &get_moved_points() const {
		return moved_points;
	}
private:
	Perm perm;
	std::vector<value_type> moved_points;
};

// the actual pruner
// -----------------------------------------------------------------------------

template<typename Derived>
struct aut_pruner_base : null_visitor {

	struct instance_data_t {
	};

	template<typename TreeNode>
	struct instance_data {
		std::vector<TreeNode*> t_path, c_path;
	};

	template<typename Config, typename TreeNode>
	struct InstanceData {
		using type = tagged_list<instance_data_t, instance_data<TreeNode> >;
	};

	struct tree_data_t {
	};

	template<typename SizeType>
	struct tree_data {
		std::vector<SizeType> parent;
		SizeType num_roots; // number of roots minus one (so we can compare with 0 instead of 1)
	};

	template<typename Config, typename TreeNode>
	struct TreeNodeData {
		using type = tagged_list<tree_data_t, tree_data<typename Config::SizeType> >;
	};
private:

	Derived &get_derived() {
		return static_cast<Derived&> (*this);
	}
public:

	template<typename State>
	void initialize(State &state) {
		auto &i_data = get(instance_data_t(), state.data);
		auto &t_path = i_data.t_path;
		auto &c_path = i_data.c_path;
		t_path.reserve(state.n);
		c_path.reserve(state.n);
	}

	template<typename State, typename TreeNode, typename Perm>
	void automorphism_leaf(State &state, TreeNode &t, const Perm &aut) {
		get_derived().add_automorphism(state, aut);
		// prune the new tree from the child of the lowest common ancestor
		TreeNode *tCanon = state.get_canon_leaf();
		TreeNode *tAut = &t;
		// tCanon can not be the root, otherwise tAut would not exist
		assert(tCanon->get_parent());
		assert(tAut->get_parent());
		assert(tCanon != tAut);
		while(tCanon->level > tAut->level) tCanon = tCanon->get_parent();
		while(tAut->level > tCanon->level) tAut = tAut->get_parent();
		assert(tCanon != tAut);
		// tCanon and tAut can not be the root, otherwise at least one of them were the root before
		assert(tCanon->get_parent());
		assert(tAut->get_parent());
		while(tCanon->get_parent() != tAut->get_parent()) {
			tCanon = tCanon->get_parent();
			tAut = tAut->get_parent();
		}
		assert(tCanon != tAut);
		tAut->prune_subtree(state);
	}

	template<typename State, typename TreeNode, typename Perm>
	void automorphism_implicit(State &state, TreeNode &t, const Perm &aut, std::size_t tag) {
		get_derived().add_automorphism(state, aut);
	}

	template<typename State, typename TreeNode>
	void tree_before_descend(State &state, TreeNode &t) {
		using SizeType = typename State::SizeType;
		if(t.get_is_pruned()) return;
		if(t.children.empty()) return; // don't do leaves
		// early bail-out
		const bool trivial_group = get_derived().is_trivial(state, *state.root);
		if(trivial_group) return;
		if(!get_derived().need_update(state, t)) return;

		// We need to find the lowest ancestor that have needs updating.
		// But we have to preserve canon_leaf, so find the path from canon_leaf as well.
		auto &i_data = get(instance_data_t(), state.data);
		auto &t_path = i_data.t_path;
		auto &c_path = i_data.c_path;
		t_path.clear();
		c_path.clear();
		for(TreeNode *a = &t; a != nullptr; a = a->get_parent()) {
			if(!get_derived().need_update(state, *a)) break;
			t_path.push_back(a);
		}
		for(TreeNode *a = state.get_canon_leaf(); a != nullptr; a = a->get_parent()) {
			if(!get_derived().need_update(state, *a)) break;
			c_path.push_back(a);
		}
		assert(!t_path.empty());
		// if the two paths are independent just clear c_path
		if(!c_path.empty() && t_path.back() != c_path.back()) {
#ifdef GRAPH_CANON_AUT_BASE_DEBUG
			std::cout << "Aut: c independent" << std::endl;
#endif
			c_path.clear();
		}

		const auto next_iteration = [&]() {
			// Clean up for next iteration
			if(!c_path.empty()) c_path.pop_back();
			t_path.pop_back();
		};
		for(; !t_path.empty(); next_iteration()) {
			TreeNode *a_t = t_path.back();
#ifdef GRAPH_CANON_AUT_BASE_DEBUG
			std::cout << "Aut: a_t: ";
			printTreeNode(std::cout, state, *a_t, true) << std::endl;
#endif
			if(a_t->get_is_pruned()) {
#ifdef GRAPH_CANON_AUT_BASE_DEBUG
				std::cout << "Aut: pruned" << std::endl;
#endif
				break;
			}

			const auto new_auts = get_derived().update(state, *a_t);
#ifdef GRAPH_CANON_AUT_BASE_DEBUG
			std::cout << "Aut: num new auts = " << std::distance(begin(new_auts), end(new_auts)) << std::endl;
#endif
			if(new_auts.begin() == new_auts.end()) {
				if(a_t->get_parent()) break;
				else continue; // the root always needs updating
			}

			const SizeType num_children = a_t->children.size();
			auto &t_data = get(tree_data_t(), a_t->data);
			auto &parent = t_data.parent;
			if(parent.empty()) {
				parent.resize(num_children);
				for(int i = 0; i < parent.size(); ++i)
					parent[i] = i;
				t_data.num_roots = parent.size() - 1;
			} else {
				if(t_data.num_roots == 0)
					continue;
			}
			// we may never prune the canon child, use -1 if it's not there
			const auto canon_child_local_idx = [&]() -> SizeType {
				if(c_path.empty()) return -1;
				if(c_path.back() != a_t) {
					// Nice, we no longer have the canon_leaf in our subtree. Just forget about the rest of c_path.
					c_path.clear();
					return -1;
				} else {
					// Still have to be careful about pruning.
					assert(c_path.size() > 1); // we can not be a leaf
					const auto canon_child_v_idx = c_path[c_path.size() - 2]->pi.get(c_path.back()->child_refiner_cell);
					const auto canon_child_idx = c_path.back()->pi.get_inverse(canon_child_v_idx);
					const auto canon_child_local_idx = canon_child_idx - c_path.back()->child_refiner_cell;
#ifdef GRAPH_CANON_AUT_BASE_DEBUG
					std::cout << "Aut: canon_child: v=" << canon_child_v_idx << ", idx=" << canon_child_idx << ", local_idx=" << canon_child_local_idx << std::endl;
#endif
					return canon_child_local_idx;
				}
			}();
			const auto find_root = [&](SizeType self) -> SizeType {
				while(true) {
					const SizeType p = parent[self];
					if(p == self) return self;
					const SizeType pp = parent[p];
					if(pp == p) return p;
					parent[self] = pp;
					self = p;
				}
			};

			const auto cell_begin = a_t->child_refiner_cell;
			const SizeType * const pi_begin = a_t->pi.begin();
			const SizeType * const pi_inv_begin = a_t->pi.begin_inverse();
			for(auto iter = begin(new_auts); iter != end(new_auts); ++iter) {
				const auto &aut = *iter;
#ifdef GRAPH_CANON_AUT_BASE_DEBUG
				std::cout << "Aut: prune with = ";
				perm_group::write_permutation_cycles(std::cout, aut);
				std::cout << std::endl;
				std::cout << "Aut:";
				for(SizeType idx_local = 0; idx_local < a_t->children.size(); ++idx_local)
					std::cout << " " << idx_local;
				std::cout << std::endl << "Aut:";
				for(SizeType idx_local = 0; idx_local < a_t->children.size(); ++idx_local)
					std::cout << " " << a_t->pi.get(idx_local + a_t->child_refiner_cell);
				std::cout << std::endl << "Aut:";
				for(SizeType idx_local = 0; idx_local < a_t->children.size(); ++idx_local)
					std::cout << " " << parent[idx_local];
				std::cout << std::endl;
#endif
				const auto per_point = [&](const auto idx_local, const auto v_idx, const auto v_image_idx) {
					const auto image_idx = pi_inv_begin[v_image_idx];
					assert(image_idx >= cell_begin);
					assert(image_idx < a_t->pi.get_cell_end(cell_begin));
					const auto image_idx_local = image_idx - cell_begin;
					const auto root = find_root(idx_local);
					const auto root_image = find_root(image_idx_local);
					if(root == root_image) return;
#ifdef GRAPH_CANON_AUT_BASE_DEBUG
					std::cout << "Aut: elm, i=" << idx_local << ", v=" << v_idx << ", root=" << root << std::endl;
					std::cout << "Aut: img, i=" << image_idx_local << ", v=" << v_image_idx << ", root=" << root_image << std::endl;
#endif
					// merge the trees
					auto new_root = root;
					auto other = root_image;
					if(root == canon_child_local_idx) {
					} else if(root_image == canon_child_local_idx) {
						std::swap(new_root, other);
					} else if(a_t->child_pruned[root]) {
						std::swap(new_root, other);
					} else {
						if(other < new_root)
							std::swap(new_root, other);
					}
#ifdef GRAPH_CANON_AUT_BASE_DEBUG
					std::cout << "Aut: other=" << other << ", new_root=" << new_root << std::endl;
#endif
					parent[other] = new_root;
					--t_data.num_roots;
					a_t->child_pruned[other] = true;
					if(auto *child = a_t->children[other]) {
						child->prune_subtree(state);
					}
				}; // end of per_point
				const auto &moved_points = aut.get_moved_points();
				if(!moved_points.empty() && moved_points.size() < num_children) {
					const auto cell_end = cell_begin + num_children;
					for(const SizeType v_idx : moved_points) {
						const auto idx = pi_inv_begin[v_idx];
						if(idx < cell_begin || idx >= cell_end) continue;
						const auto v_image_idx = perm_group::get(aut, v_idx);
						assert(v_idx != v_image_idx);
						per_point(idx - cell_begin, v_idx, v_image_idx);
						if(t_data.num_roots == 0)
							break;
					}
				} else {
					for(SizeType idx_local = 0; idx_local != num_children; ++idx_local) {
						const auto idx = idx_local + cell_begin;
						const auto v_idx = pi_begin[idx];
						const auto v_image_idx = perm_group::get(aut, v_idx);
						if(v_idx == v_image_idx) continue;
						per_point(idx_local, v_idx, v_image_idx);
						if(t_data.num_roots == 0)
							break;
					}
				}
				if(t_data.num_roots == 0)
					break;
			}
#ifdef GRAPH_CANON_AUT_BASE_DEBUG
			std::cout << "Aut: after pruning" << std::endl;
			std::cout << "Aut:";
			for(SizeType idx_local = 0; idx_local < a_t->children.size(); ++idx_local)
				std::cout << " " << idx_local;
			std::cout << std::endl << "Aut:";
			for(SizeType idx_local = 0; idx_local < a_t->children.size(); ++idx_local)
				std::cout << " " << a_t->pi.get(idx_local + a_t->child_refiner_cell);
			std::cout << std::endl << "Aut:";
			for(SizeType idx_local = 0; idx_local < a_t->children.size(); ++idx_local)
				std::cout << " " << parent[idx_local];
			std::cout << std::endl;
#endif
		}
	}
};

} // namespace graph_canon

#endif /* GRAPH_CANON_AUT_PRUNER_BASE_HPP */
