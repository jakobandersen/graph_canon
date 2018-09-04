#ifndef GRAPH_CANON_AUT_PRUNER_BASIC_HPP
#define GRAPH_CANON_AUT_PRUNER_BASIC_HPP

#include <graph_canon/aut/pruner_base.hpp>

#include <perm_group/allocator/pooled.hpp>
#include <perm_group/allocator/raw_ptr.hpp>
#include <perm_group/group/generated.hpp>
#include <perm_group/group/basic_stabilizer.hpp>
#include <perm_group/orbit.hpp>
#include <perm_group/permutation/built_in.hpp>

#include <cassert>
#include <vector>

namespace graph_canon {

// rst: .. class:: aut_pruner_basic : aut_pruner_base<aut_pruner_basic>
// rst:
// rst:		A `Visitor` for pruning the search tree based on automorphisms reported by other visitors.
// rst:		Stabilizers are computed conservatively by plain filtering of the generators.
// rst:

struct aut_pruner_basic : aut_pruner_base<aut_pruner_basic> {
	using Base = aut_pruner_base<aut_pruner_basic>;
public:
	template<typename SizeType>
	using Perm = std::vector<SizeType>;
	template<typename SizeType>
	using AllocInner = perm_group::raw_ptr_allocator<Perm<SizeType> >;
	template<typename SizeType>
	using Alloc = perm_group::pooled_allocator<AllocInner<SizeType> >;
	template<typename SizeType>
	using BaseGroup = perm_group::generated_group<Alloc<SizeType>, perm_group::DupCheckerCompare>;
	template<typename SizeType>
	using Stab = perm_group::basic_stabilizer<Alloc<SizeType> >;
public:

	struct result_t {
	};

	struct instance_data_t {
	};

	template<typename SizeType>
	struct instance_data {
		std::unique_ptr<BaseGroup<SizeType> > g;
	};

	template<typename Config, typename TreeNode>
	struct InstanceData {
		using base_type = typename Base::InstanceData<Config, TreeNode>::type;
		using this_type = tagged_element<instance_data_t, instance_data<typename Config::SizeType> >;
		using type = typename tagged_list_concat<this_type, base_type>::type;
	};

	struct tree_data_t {
	};

	template<typename SizeType>
	struct tree_data {
		// The stabilizer representation. It is not valid for the root.
		// It is initialized in the update function.
		std::unique_ptr<Stab<SizeType> > stab;
		// How many of the base group gens we have used.
		// That is, if we update this and the update stops at an ancestor because no further permutations
		// can be added to a child, then this one is still updated as we have checked that amount of generators.
		// The base group always has the identity, which we don't care about.
		SizeType num_base_gens = 1;
		// Similarly this is the number of gens in the parent node for the last update.
		// The root doesn't use this variable.
		SizeType num_parent_gens = 1;
		// This means:
		// - If num_base_gens is too low, then we or some ancestor needs updating.
		// - After updating all ancestors, if num_parent_gens is too low, then we need to be updated.
		// Invariant: If num_base_gens is up-to-date, then num_parent_gens must be as well.
	};

	template<typename Config, typename TreeNode>
	struct TreeNodeData {
		using base_type = typename Base::TreeNodeData<Config, TreeNode>::type;
		using this_type = tagged_element<tree_data_t, tree_data<typename Config::SizeType> >;
		using type = typename tagged_list_concat<this_type, base_type>::type;
	};
public:

	template<typename State>
	void initialize(State &s) {
		// base group
		using SizeType = typename State::SizeType;
		const std::size_t pool_size = s.n;
		auto &i_data = get(instance_data_t(), s.data);
		i_data.g.reset(new BaseGroup<SizeType > (Alloc<SizeType>(pool_size, AllocInner<SizeType>(s.n))));
	}

	template<typename State>
	auto extract_result(State &s) {
		using SizeType = typename State::SizeType;
		using Result = std::unique_ptr<BaseGroup<SizeType> >;
		auto &i_data = get(instance_data_t(), s.data);
		return tagged_element<result_t, Result>{std::move(i_data.g)};
	}
public: // for aut_pruner_base

	template<typename State, typename TreeNode, typename AutPerm>
	void add_automorphism(State &s, TreeNode &t, const AutPerm &aut_new) {
		assert(perm_group::degree(aut_new) == s.n);
		auto &i_data = get(instance_data_t(), s.data);
		auto &g = *i_data.g;
		g.add_generator(aut_new);
	}

	template<typename State, typename TreeNode>
	bool need_update(State &s, TreeNode &t) {
		const auto &i_data = get(instance_data_t(), s.data);
		// early bail-out
		const bool trivial_group = i_data.g->generators().size() == 1;
		if(trivial_group) return false;

		const auto &t_data = get(tree_data_t(), t.data);
		assert(t_data.num_base_gens <= i_data.g->generators().size());
		return t_data.num_base_gens != i_data.g->generators().size();
	}

	template<typename State, typename TreeNode>
	auto update(const State &s, TreeNode &t) {
		using SizeType = typename State::SizeType;
		const auto &i_data = get(instance_data_t(), s.data);
		auto &t_data = get(tree_data_t(), t.data);
		if(const auto *t_parent = t.get_parent()) { // not the root
			if(!t_data.stab) {
				const auto new_v_idx_to_fix = t.pi.get(t.get_parent()->get_child_individualized_position());
				t_data.stab.reset(new Stab<SizeType>(new_v_idx_to_fix, i_data.g->get_allocator()));
			}
			const auto handle_group = [&](const auto &g) {
				const auto gens_parent = g.generator_ptrs();
				assert(gens_parent.size() >= t_data.num_parent_gens); // invariant
				const auto last_old = gens_parent.begin() + t_data.num_parent_gens;
				const auto old_size = t_data.stab->generators().size();
#ifdef GRAPH_CANON_AUT_BASE_DEBUG
				std::cout << "Updating tree node at level " << t.level << " with " << (gens_parent.end() - last_old) << " generators." << std::endl;
				for(auto d_iter = last_old; d_iter != gens_parent.end(); ++d_iter)
					perm_group::write_permutation_cycles(std::cout << "\t" << (d_iter - gens_parent.begin()) << "\t", **d_iter) << std::endl;
				std::cout << "\tcurrent gens:\n";
				for(const auto &p : t_data.stab->generators())
					perm_group::write_permutation_cycles(std::cout << "\t \t", p) << std::endl;
#endif
				t_data.stab->add_generators(gens_parent.begin(), last_old, gens_parent.end());
				const auto gens = t_data.stab->generators();
				t_data.num_parent_gens = gens_parent.size();
				t_data.num_base_gens = i_data.g->generators().size();
				assert(!t_parent->get_parent() || get(tree_data_t(), t_parent->data).num_base_gens == t_data.num_base_gens);
#ifdef GRAPH_CANON_AUT_BASE_DEBUG
				std::cout << "Updating tree node at level " << t.level << ", got " << (gens.end() - gens.begin() - old_size) << " new." << std::endl;
				for(auto d_iter = gens.begin() + old_size; d_iter != gens.end(); ++d_iter)
					perm_group::write_permutation_cycles(std::cout << "\t" << (d_iter - gens.begin()) << "\t", *d_iter) << std::endl;
#endif
				return make_aut_range(gens.begin() + old_size, gens.end());
			};
			// The root has no stabilizer, so the parent gens are those from the base group.
			if(t_parent->get_parent()) {
				const auto &t_parent_data = get(tree_data_t(), t_parent->data);
				assert(t_parent_data.stab);
				return handle_group(*t_parent_data.stab);
			} else {
				return handle_group(*i_data.g);
			}
		} else { // the root
			const auto gens = i_data.g->generators();
			assert(gens.size() >= t_data.num_base_gens); // invariant
			assert(gens.size() > t_data.num_base_gens); // else we shouldn't have called this function
			auto first = gens.begin() + t_data.num_base_gens;
			t_data.num_base_gens = gens.size();
			assert(t_data.num_parent_gens == 1);
			return make_aut_range(first, gens.end());
		}
	}
};

} // namespace graph_canon

#endif /* GRAPH_CANON_AUT_PRUNER_BASIC_HPP */
