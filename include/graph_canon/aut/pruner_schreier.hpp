#ifndef GRAPH_CANON_AUT_PRUNER_SCHREIER_HPP
#define GRAPH_CANON_AUT_PRUNER_SCHREIER_HPP

#include <graph_canon/aut/perm_moved_points.hpp>
#include <graph_canon/aut/pruner_base.hpp>

#include <perm_group/allocator/pooled.hpp>
#include <perm_group/allocator/raw_ptr.hpp>
#include <perm_group/group/generated.hpp>
#include <perm_group/group/schreier_stabilizer.hpp>
#include <perm_group/orbit.hpp>
#include <perm_group/permutation/built_in.hpp>
#include <perm_group/permutation/word.hpp>
#include <perm_group/transversal/explicit.hpp>

#include <cassert>
#include <vector>

//#define GRAPH_CANON_AUT_SCHREIER_DEBUG
//#define GRAPH_CANON_AUT_SCHREIER_DEBUG_GROUPS

#if defined(GRAPH_CANON_AUT_SCHREIER_DEBUG) || defined(GRAPH_CANON_AUT_SCHREIER_DEBUG_GROUPS)
#include <graph_canon/detail/io.hpp>
#include <perm_group/permutation/io.hpp>
#include <perm_group/group/io.hpp>
#include <iostream>
#endif

namespace graph_canon {
namespace detail {

template<typename tree_data_t, std::size_t N>
struct aut_pruner_schreier_DupCheckerUnroller {

	template<typename IData, typename TreeNode, typename Perm>
	static bool check(IData &i_data, TreeNode *t_parent, TreeNode *t_child, const Perm &p) {
		static_assert(N > 0, "");
		using ValueType = typename Perm::value_type;

		auto &word = i_data.word;
		const auto rest = [&](ValueType i) {
			i = perm_group::get(p, i);
			for(std::size_t k = 0; k != N; ++k)
				i = perm_group::get(*word[k], i);
			return i;
		};
		for(; t_child;
				t_parent = t_child,
				t_child = get(tree_data_t(), t_parent->data).next_child) {
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
			detail::printTreeNodeName(std::cout << "Schreier: \tchild = ", *t_child) << std::endl;
#endif
			const auto *stab = get(tree_data_t(), t_child->data).stab.get();
			const auto root = stab->fixed_element();
			const auto img = rest(root);
			if(img == root)
				continue;
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
			const std::string indent = "          \t";
			const auto base = stab->fixed_element();
			const auto &trans = stab->transversal();
			const auto &orbit = trans.orbit();
			std::cout << indent << "orbit(" << base << ") = ";
			std::copy(orbit.begin(), orbit.end(), std::ostream_iterator<int>(std::cout, " "));
			std::cout << std::endl;
			std::cout << indent << "trans(" << base << ") =" << std::endl;
			for(const auto o : orbit) {
				std::cout << indent << "  " << trans.predecessor(o) << " -> " << o << ": ";
				perm_group::write_permutation_cycles(std::cout, trans.from_element(o)) << std::endl;
			}
#endif
			const auto &o = stab->transversal().orbit();
			if(!o.isInOrbit(img)) {
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
				std::cout << "Schreier: \tfactor = nullptr" << std::endl;
#endif
				return false;
			}
			const auto pos = o.position(img);
			auto factor = stab->inverse_transversal(pos);
			i_data.word.push_back(factor);
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
			perm_group::write_permutation_cycles(std::cout << "Schreier: \tfactor = ", *factor) << std::endl;
			perm_group::write_permutation_cycles(std::cout << "Schreier: \trest = ", perm_group::mult(p, word)) << std::endl;
#endif
			return aut_pruner_schreier_DupCheckerUnroller<tree_data_t, N + 1 > ::check(i_data, t_parent, t_child, p);
		}
		assert(word.size() == N);

		const auto &t_parent_data = get(tree_data_t(), t_parent->data);
		const auto &stab = *t_parent_data.stab;
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
		bool debug_is_member = true;
		for(ValueType i = 0; i != stab.degree(); ++i) {
			if(perm_group::get(perm_group::mult(p, word), i) != i)
				debug_is_member = false;
		}
		std::cout << "          \tis_member = " << std::boolalpha << debug_is_member << std::endl;
#endif
		//		detail::printTreeNodeName(std::cout << "Schreier: \tfinal_dup_check in ", *t_parent) << std::endl;
		//		std::cout << "          \tnext_child = " << t_parent_data.next_child << std::endl;
		//		perm_group::write_permutation_cycles(std::cout << "          \trest = ", rest) << std::endl;
		//		perm_group::write_group(std::cout << "          \tgroup = ", *t_parent_data.stab) << std::endl;

		if(stab.generators().size() == 1) {
			// if it's the identity we have it
			for(ValueType i = 0; i != stab.degree(); ++i) {
				if(rest(i) != i)
					return false;
			}
			return true;
		} else { // fallback to compare against all generators
			for(const auto &g : stab.generators()) {
				const auto eq = [&]() {
					for(ValueType i = 0; i != stab.degree(); ++i) {
						if(rest(i) != perm_group::get(g, i))
							return false;
					}
					return true;
				}();
				if(eq) return true;
			}
			return false;
		}
	}
};

template<typename tree_data_t>
struct aut_pruner_schreier_DupCheckerUnroller<tree_data_t, 0> {

	template<typename IData, typename TreeNode, typename Perm>
	static bool check(IData &i_data, TreeNode *t_parent, TreeNode *t_child, const Perm &p) {
		// do the first one specially as we sift p and not a compound permutation
		for(; t_child;
				t_parent = t_child,
				t_child = get(tree_data_t(), t_parent->data).next_child) {
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
			detail::printTreeNodeName(std::cout << "Schreier: \tchild = ", *t_child) << std::endl;
#endif
			const auto *stab = get(tree_data_t(), t_child->data).stab.get();
			const auto root = stab->fixed_element();
			const auto img = perm_group::get(p, root);
			if(img == root)
				continue;
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
			const std::string indent = "          \t";
			const auto base = stab->fixed_element();
			const auto &trans = stab->transversal();
			const auto &orbit = trans.orbit();
			std::cout << indent << "orbit(" << base << ") = ";
			std::copy(orbit.begin(), orbit.end(), std::ostream_iterator<int>(std::cout, " "));
			std::cout << std::endl;
			std::cout << indent << "trans(" << base << ") =" << std::endl;
			for(const auto o : orbit) {
				std::cout << indent << "  " << trans.predecessor(o) << " -> " << o << ": ";
				perm_group::write_permutation_cycles(std::cout, trans.from_element(o)) << std::endl;
			}
#endif
			const auto &o = stab->transversal().orbit();
			if(!o.isInOrbit(img)) {
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
				std::cout << "Schreier: \tfactor = nullptr" << std::endl;
#endif
				return false;
			}
			const auto pos = o.position(img);
			auto factor = stab->inverse_transversal(pos);
			i_data.word.push_back(factor);
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
			perm_group::write_permutation_cycles(std::cout << "Schreier: \tfactor = ", *factor) << std::endl;
			perm_group::write_permutation_cycles(std::cout << "Schreier: \trest = ", perm_group::mult(p, i_data.word)) << std::endl;
#endif
			t_parent = t_child;
			t_child = get(tree_data_t(), t_parent->data).next_child;
			return aut_pruner_schreier_DupCheckerUnroller<tree_data_t, 1>::check(i_data, t_parent, t_child, p);
		}
		const auto handleGroup = [&](const auto &group) {
			using ValueType = typename Perm::value_type;
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
			const bool debug_is_member = p == *group.generators().begin();
			std::cout << "          \tis_member = " << std::boolalpha << debug_is_member << std::endl;
#endif
			//		detail::printTreeNodeName(std::cout << "Schreier: \tfinal_dup_check in ", *t_parent) << std::endl;
			//		std::cout << "          \tnext_child = " << t_parent_data.next_child << std::endl;
			//		perm_group::write_permutation_cycles(std::cout << "          \trest = ", rest) << std::endl;
			//		perm_group::write_group(std::cout << "          \tgroup = ", *t_parent_data.stab) << std::endl;

			if(group.generators().size() == 1) {
				// if it's the identity we have it
				return p == *group.generators().begin();
			} else { // fallback to compare against all generators
				for(const auto &g : group.generators()) {
					if(p == g) return true;
				}
				return false;
			}
		};
		if(t_parent->get_parent()) {
			const auto &t_parent_data = get(tree_data_t(), t_parent->data);
			assert(t_parent_data.stab);
			const auto &stab = *t_parent_data.stab;
			return handleGroup(stab);
		} else {
			return handleGroup(*i_data.g);
		}
	}
};

template<typename tree_data_t>
struct aut_pruner_schreier_DupCheckerUnroller<tree_data_t, 8> {

	template<typename IData, typename TreeNode, typename Perm>
	static bool check(IData &i_data, TreeNode *t_parent, TreeNode *t_child, const Perm &p) {
		auto &word = i_data.word;
		const auto rest = perm_group::mult(p, word); // note, this changes when word changes
		for(; t_child;
				t_parent = t_child,
				t_child = get(tree_data_t(), t_parent->data).next_child) {
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
			detail::printTreeNodeName(std::cout << "Schreier: \tchild = ", *t_child) << std::endl;
#endif
			const auto *stab = get(tree_data_t(), t_child->data).stab.get();
			const auto root = stab->fixed_element();
			const auto img = perm_group::get(rest, root);
			if(img == root)
				continue;
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
			const std::string indent = "          \t";
			const auto base = stab->fixed_element();
			const auto &trans = stab->transversal();
			const auto &orbit = trans.orbit();
			std::cout << indent << "orbit(" << base << ") = ";
			std::copy(orbit.begin(), orbit.end(), std::ostream_iterator<int>(std::cout, " "));
			std::cout << std::endl;
			std::cout << indent << "trans(" << base << ") =" << std::endl;
			for(const auto o : orbit) {
				std::cout << indent << "  " << trans.predecessor(o) << " -> " << o << ": ";
				perm_group::write_permutation_cycles(std::cout, trans.from_element(o)) << std::endl;
			}
#endif
			const auto &o = stab->transversal().orbit();
			if(!o.isInOrbit(img)) {
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
				std::cout << "Schreier: \tfactor = nullptr" << std::endl;
#endif
				return false;
			}
			const auto pos = o.position(img);
			auto factor = stab->inverse_transversal(pos);
			i_data.word.push_back(factor);
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
			perm_group::write_permutation_cycles(std::cout << "Schreier: \tfactor = ", *factor) << std::endl;
			perm_group::write_permutation_cycles(std::cout << "Schreier: \trest = ", rest) << std::endl;
#endif
		}

		const auto &t_parent_data = get(tree_data_t(), t_parent->data);
		const auto &stab = *t_parent_data.stab;
		using ValueType = typename Perm::value_type;
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
		bool debug_is_member = true;
		for(ValueType i = 0; i != stab.degree(); ++i) {
			if(perm_group::get(rest, i) != i)
				debug_is_member = false;
		}
		std::cout << "          \tis_member = " << std::boolalpha << debug_is_member << std::endl;
#endif
		//		detail::printTreeNodeName(std::cout << "Schreier: \tfinal_dup_check in ", *t_parent) << std::endl;
		//		std::cout << "          \tnext_child = " << t_parent_data.next_child << std::endl;
		//		perm_group::write_permutation_cycles(std::cout << "          \trest = ", rest) << std::endl;
		//		perm_group::write_group(std::cout << "          \tgroup = ", *t_parent_data.stab) << std::endl;

		if(stab.generators().size() == 1) {
			// if it's the identity we have it
			for(ValueType i = 0; i != stab.degree(); ++i) {
				if(perm_group::get(rest, i) != i)
					return false;
			}
			return true;
		} else { // fallback to compare against all generators
			for(const auto &g : stab.generators()) {
				const auto eq = [&]() {
					for(ValueType i = 0; i != stab.degree(); ++i) {
						if(perm_group::get(rest, i) != perm_group::get(g, i))
							return false;
					}
					return true;
				}();
				if(eq) return true;
			}
			return false;
		}
	}
};

} // namespace detail

// rst: .. class:: aut_pruner_schreier : aut_pruner_base<aut_pruner_schreier>
// rst:
// rst:		A `Visitor` for pruning the search tree based on automorphisms reported by other visitors.
// rst:		Stabilizers are fully computed using methods based on the Schreier-Sims algorithm.
// rst:

struct aut_pruner_schreier : aut_pruner_base<aut_pruner_schreier> {
	using Base = aut_pruner_base<aut_pruner_schreier>;
private:

	template<typename State, typename TreeNode>
	struct Adder {

		Adder(State *s, TreeNode *t_parent) : s(s), t_parent(t_parent) {
			assert(s);
			assert(t_parent);
		}

		void operator()(auto first, auto oldLast, auto oldNew) {
			generator_added(*s, *t_parent, first, oldLast, oldNew);
		}
	private:
		State *s;
		TreeNode *t_parent;
	};

	template<typename TreeNode, typename InstanceData>
	struct DupChecker {

		explicit DupChecker(TreeNode *t, const InstanceData *i_data) : t(t), i_data(i_data) {
			assert(t);
			assert(i_data);
		}

		template<typename Iter, typename Perm>
		bool operator()(Iter first, Iter last, const Perm &p) {
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
			detail::printTreeNodeName(std::cout << "Schreier: dup_check in ", *t) << " of ";
			perm_group::write_permutation_cycles(std::cout, p);
			std::cout << std::endl;
#endif
			// Do actual sifting.
			// The following is an adaptation of perm_group::stabilizer_chain::is_member_of_parent.
			// We do a best-effort sifting, i.e., when the chain ends, it's just too bad.
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
			std::cout << "Schreier: \tnon-trivial group, sift" << std::endl;
#endif
			auto &word = i_data->word;
			word.clear();
			TreeNode *t_parent = t;
			TreeNode *t_child = get(tree_data_t(), t_parent->data).next_child;
			return detail::aut_pruner_schreier_DupCheckerUnroller<tree_data_t, 0>::check(*i_data, t_parent, t_child, p);
		}
	public:
		TreeNode *t;
		const InstanceData *i_data;
	};
public:
	template<typename SizeType>
	using Perm = std::vector<SizeType>;
	template<typename SizeType>
	using AllocInner = perm_group::raw_ptr_allocator<Perm<SizeType> >;
	template<typename SizeType>
	using Alloc = perm_group::pooled_allocator<AllocInner<SizeType> >;
	template<typename SizeType>
	using Transversal = perm_group::transversal_explicit<Alloc<SizeType> >;
	template<typename SizeType, typename TreeNode, typename InstanceData>
	using BaseGroup = perm_group::generated_group<Alloc<SizeType>, DupChecker<TreeNode, InstanceData> >;
	template<typename SizeType, typename TreeNode, typename InstanceData>
	using Stab = perm_group::schreier_stabilizer<Transversal<SizeType>, DupChecker<TreeNode, InstanceData> >;
public:

	struct result_t {
	};

	struct instance_data_t {
	};

	template<typename SizeType, typename TreeNode>
	struct instance_data {
		using Group = BaseGroup<SizeType, TreeNode, instance_data>;
		std::unique_ptr<Group> g;
	public:
		// for avoiding too many reallocations
		mutable perm_group::permutation_word_fixed<typename Group::const_pointer> word;
	};

	template<typename Config, typename TreeNode>
	struct InstanceData {
		using base_type = typename Base::InstanceData<Config, TreeNode>::type;
		using this_type = tagged_element<instance_data_t, instance_data<typename Config::SizeType, TreeNode> >;
		using type = typename tagged_list_concat<this_type, base_type>::type;
	};

	struct tree_data_t {
	};

	template<typename SizeType, typename TreeNode>
	struct tree_data {
		// The stabilizer representation. It is not valid for the root.
		std::unique_ptr<Stab<SizeType, TreeNode, instance_data<SizeType, TreeNode> > > stab;
		// We need some child to be the next in our stabilizer chain.
		// Notes:
		// - If you are an ancestor of canon_leaf, your chain always goes that way.
		// - Never point to a destroyed child. The child is responsible for resetting.
		TreeNode *next_child = nullptr;
		// Make sure we know if we are part of the main chain to canon_leaf.
		// When no canon_leaf is present, then no main chain exists.
		bool is_main_chain = false;
		// When we add permutations in one tree node they may cause additions in their
		// next_child, recursively. However, children may start their own chains starting
		// from a parent node, and the parent may later redirect its next_child to another child.
		// A node therefore keeps track of how many of the parent generators it has pulled in.
		// Instead of having always to check up a chain if it is up to date, we also keep
		// track of how many base generators have been used in the last update.
		// A different aspect is how many generators we have told the pruner_base about.
		// We therefore have another pair of generator counts that track how many generators
		// were present when the pruner_base last asked for updates.
		SizeType num_base_gens_chain = 1, num_parent_gens_chain = 1;
		// Invariants:
		// - num_base_gens_chain <= base_group.generators().size()
		//   not equal <=> then some ancestor needs updating.
		// - If num_parent_gens_chain == parent.stab.generators.size()
		//   then num_base_gens_chain == base_group.generators().size()
		// Note: the cascading updates in the base group must update these counts.
		// The next two are for the relationship with aut_pruner_base.
		SizeType num_base_gens_reporting = 1, num_gens_reported = 1;
		// Invariants:
		// - num_base_gens_reporting <= num_base_gens_chain
		//   num_gens_reported <= stab.generators.size()
	};

	template<typename Config, typename TreeNode>
	struct TreeNodeData {
		using base_type = typename Base::TreeNodeData<Config, TreeNode>::type;
		using this_type = tagged_element<tree_data_t, tree_data<typename Config::SizeType, TreeNode> >;
		using type = typename tagged_list_concat<this_type, base_type>::type;
	};
public:

	template<typename State, typename TreeNode>
	bool tree_create_node_begin(State &s, TreeNode &t) {
		const bool res = Base::tree_create_node_begin(s, t);

		if(!t.get_parent()) {
			// we need the root to be there in order to create the base group
			using SizeType = typename State::SizeType;
			using IData = instance_data<SizeType, TreeNode>;
			const std::size_t pool_size = 4 * s.n;
			auto &i_data = get(instance_data_t(), s.data);
			Alloc<SizeType> alloc(pool_size, AllocInner<SizeType>(s.n));
			DupChecker<TreeNode, IData> dupChecker(&t, &i_data);
			i_data.g.reset(new BaseGroup<SizeType, TreeNode, IData>(alloc, dupChecker));
			i_data.word.reserve(s.n);
		}
		return res;
	}

	template<typename State, typename TreeNode>
	bool tree_create_node_end(State &s, TreeNode &t) {
		const bool res = Base::tree_create_node_end(s, t);

		if(TreeNode * p = t.get_parent()) {
			// We generally assume that when you create a node, you want to continue with a child of that.
			// So unless our parent is in the main chain, make it point at us.
			auto &p_data = get(tree_data_t(), p->data);
			if(!p_data.is_main_chain) {
#if defined(GRAPH_CANON_AUT_SCHREIER_DEBUG) || defined(GRAPH_CANON_AUT_SCHREIER_DEBUG_GROUPS)
				std::cout << "Schreier: tree_create_node_end, setting next_child of parent" << std::endl;
				detail::printTreeNodeName(std::cout << "          parent = ", *p) << std::endl;
				detail::printTreeNodeName(std::cout << "          child  = ", t) << std::endl;
#endif
				p_data.next_child = &t;
			}
		}
		return res;
	}

	template<typename State, typename TreeNode>
	void tree_prune_node(State &state, TreeNode &t) {
		Base::tree_prune_node(state, t);

		if(TreeNode * p = t.get_parent()) {
			auto &p_data = get(tree_data_t(), p->data);
			if(p_data.next_child == &t)
				p_data.next_child = nullptr;
		}
		// let's delete our stabilizer to free up permutations
		get(tree_data_t(), t.data).stab.reset();
	}

	template<typename State, typename TreeNode>
	void tree_destroy_node(State &state, TreeNode &t) {
		Base::tree_destroy_node(state, t);

		if(TreeNode * p = t.get_parent()) {
			auto &p_data = get(tree_data_t(), p->data);
			if(p_data.next_child == &t)
				p_data.next_child = nullptr;
		}
	}

	template<typename State>
	auto extract_result(State &s) {
		using SizeType = typename State::SizeType;
		using Group = perm_group::generated_group<Alloc<SizeType> >;
		using Result = std::unique_ptr<Group>;
		auto &i_data = get(instance_data_t(), s.data);
		Result r = std::make_unique<Group>(i_data.g->get_allocator());
		for(const auto &p : i_data.g->generators())
			r->add_generator(p);
		return tagged_element<result_t, Result>{std::move(r)};
	}

	template<typename State, typename TreeNode>
	void canon_new_best(State &state, TreeNode *previous) {
		Base::canon_new_best(state, previous);

		for(TreeNode *t = previous; t; t = t->get_parent()) {
			auto &t_data = get(tree_data_t(), t->data);
			assert(t_data.is_main_chain);
			t_data.is_main_chain = false;
		}
		TreeNode *next_child = nullptr;
		for(TreeNode *t = state.get_canon_leaf(); t; t = t->get_parent()) {
			auto &t_data = get(tree_data_t(), t->data);
			assert(!t_data.is_main_chain);
			t_data.is_main_chain = true;
			t_data.next_child = next_child;
			next_child = t;
		}
	}

	template<typename State>
	void canon_prune(State &state) {
		Base::canon_prune(state);
		// clear main chain marking
		for(auto *t = state.get_canon_leaf(); t; t = t->get_parent()) {
			auto &t_data = get(tree_data_t(), t->data);
			assert(t_data.is_main_chain);
			t_data.is_main_chain = false;
		}
	}
public: // for aut_pruner_base

	template<typename State, typename TreeNode, typename AutPerm>
	void add_automorphism(State &s, TreeNode &t, const AutPerm &aut_new) {
#if defined(GRAPH_CANON_AUT_SCHREIER_DEBUG) || defined(GRAPH_CANON_AUT_SCHREIER_DEBUG_GROUPS)
		const auto debug_print_root_chain = [&]() {
			const auto &i_data = get(instance_data_t(), s.data);
			perm_group::write_permutation_cycles(std::cout << "Schreier: add_automorphism group update (", aut_new) << ")" << std::endl;
			for(TreeNode *tt = s.root.get(); tt; tt = get(tree_data_t(), tt->data).next_child) {
				const auto &t_data = get(tree_data_t(), tt->data);
				detail::printTreeNodeName(std::cout << "          Data for ", *tt) << std::endl;
				std::cout << "          \tnum_base_gens_chain = " << t_data.num_base_gens_chain << std::endl;
				std::cout << "          \tnum_parent_gens_chain = " << t_data.num_parent_gens_chain << std::endl;
				std::cout << "          \tnum_base_gens_reporting = " << t_data.num_base_gens_reporting << std::endl;
				std::cout << "          \tnum_gens_reported = " << t_data.num_gens_reported << std::endl;
				if(!tt->get_parent()) {
					perm_group::write_group(std::cout << "          \tgroup = ", *i_data.g) << std::endl;
				} else {
					const auto *stab = t_data.stab.get();
					if(stab) {
						perm_group::write_group(std::cout << "          \tgroup = ", *stab) << std::endl;
						const std::string indent = "          \t";
						const auto base = stab->fixed_element();
						const auto &trans = stab->transversal();
						const auto &orbit = trans.orbit();
						std::cout << indent << "orbit(" << base << ") = ";
						std::copy(orbit.begin(), orbit.end(), std::ostream_iterator<int>(std::cout, " "));
						std::cout << std::endl;
						std::cout << indent << "trans(" << base << ") =" << std::endl;
						for(const auto o : orbit) {
							std::cout << indent << "  " << trans.predecessor(o) << " -> " << o << ": ";
							perm_group::write_permutation_cycles(std::cout, trans.from_element(o)) << std::endl;
						}
					} else {
						std::cout << "          \tgroup = nullptr" << std::endl;
					}
				}
			}
		};
#endif

		using SizeType = typename State::SizeType;
		assert(perm_group::degree(aut_new) == s.n);
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
		perm_group::write_permutation_cycles(std::cout << "Schreier: add_automorphism: ", aut_new) << std::endl;
#endif

		auto &i_data = get(instance_data_t(), s.data);
		auto &g = *i_data.g;
		TreeNode *root = s.root.get();
		if(!root) {
			// an automorphism could be found in the root, and the root pointer
			// will then not be assigned to the global state yet
			assert(!t.get_parent());
			root = &t;
		}
		auto &root_data = get(tree_data_t(), root->data);
		if(root_data.is_main_chain && false) {
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
			std::cout << "Schreier: \thas main chain" << std::endl;
#endif
			// just assertions
			for(TreeNode *tt = root; tt; tt = get(tree_data_t(), tt->data).next_child) {
				auto &tt_data = get(tree_data_t(), tt->data);
				assert(tt_data.num_base_gens_chain == g.generators().size());
				if(tt != root) {
					assert(tt_data.stab);
					if(tt->get_parent()->get_parent()) {
						auto &p_data = get(tree_data_t(), tt->get_parent()->data);
						assert(tt_data.num_parent_gens_chain == p_data.stab->generators().size());
					} else {
						assert(tt_data.num_parent_gens_chain == g.generators().size());
					}
				}
			}
			// do we need to do somthing else?
			assert(false);
		} else { // only if it's not the first automorphism
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
			std::cout << "Schreier: \tno main chain, updating current chain" << std::endl;
#endif
			update_chain_and_ancestors(s, *root);
		}

#if defined(GRAPH_CANON_AUT_SCHREIER_DEBUG) || defined(GRAPH_CANON_AUT_SCHREIER_DEBUG_GROUPS)
		std::cout << "Before addition" << std::endl;
		std::cout << "=====================================================" << std::endl;
		debug_print_root_chain();
		std::cout << "=====================================================" << std::endl;
#endif

		g.add_generator(aut_new, [&](auto first, auto oldLast, auto oldNew) {
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
			perm_group::write_permutation_cycles(std::cout << "Schreier: \tadded to base group, ", aut_new) << std::endl;
#endif
			Adder<State, TreeNode>(&s, root)(first, oldLast, oldNew);
		});
		// update the counts on the chain
		root_data.num_base_gens_chain = g.generators().size();
		TreeNode *child = root_data.next_child;
		if(child) {
			// the first child has parent gens in the base group
			auto &c_data = get(tree_data_t(), child->data);
			c_data.num_base_gens_chain = g.generators().size();
			c_data.num_parent_gens_chain = g.generators().size();
			// and now all a updated the same way
			for(TreeNode *child = c_data.next_child; child; child = get(tree_data_t(), child->data).next_child) {
				auto &c_data = get(tree_data_t(), child->data);
				auto &p_data = get(tree_data_t(), child->get_parent()->data);
				assert(p_data.stab);
				c_data.num_base_gens_chain = g.generators().size();
				c_data.num_parent_gens_chain = p_data.stab->generators().size();
			}
		}

#if defined(GRAPH_CANON_AUT_SCHREIER_DEBUG) || defined(GRAPH_CANON_AUT_SCHREIER_DEBUG_GROUPS)
		std::cout << "After addition" << std::endl;
		std::cout << "=====================================================" << std::endl;
		debug_print_root_chain();
		std::cout << "=====================================================" << std::endl;
#endif

		// just assertions
		for(TreeNode *tt = root; tt; tt = get(tree_data_t(), tt->data).next_child) {
			auto &tt_data = get(tree_data_t(), tt->data);
			assert(tt_data.num_base_gens_chain == g.generators().size());
			if(tt != root) {
				assert(tt_data.stab);
				if(tt->get_parent()->get_parent()) {
					auto &p_data = get(tree_data_t(), tt->get_parent()->data);
					assert(tt_data.num_parent_gens_chain == p_data.stab->generators().size());
				} else {
					assert(tt_data.num_parent_gens_chain == g.generators().size());
				}
			}
		}
	}

	template<typename State, typename TreeNode>
	bool need_update(State &s, TreeNode &t) {
		// that is, do we need to report more generators to aut_pruner_base
		// not whether the stab of this node needs updating
		const auto &i_data = get(instance_data_t(), s.data);
		// early bail-out
		//		const bool trivial_group = i_data.g->generators().size() == 1;
		//		if(trivial_group) return false;

		const auto &t_data = get(tree_data_t(), t.data);
		assert(t_data.num_base_gens_chain <= i_data.g->generators().size());
		assert(t_data.num_base_gens_reporting <= t_data.num_base_gens_chain);
		return t_data.num_base_gens_reporting != i_data.g->generators().size();
	}

	template<typename State, typename TreeNode>
	auto update(const State &s, TreeNode &t) {
		using SizeType = typename State::SizeType;
		const auto &i_data = get(instance_data_t(), s.data);
		auto &t_data = get(tree_data_t(), t.data);
		const auto *t_parent = t.get_parent();
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
		detail::printTreeNodeName(std::cout << "Schreier: update ", t) << std::endl;
#endif
		const auto g_gens = i_data.g->generators();
		if(!t_parent) { // the root
			assert(t_data.num_base_gens_chain == g_gens.size()); // the group of the root is the base group
			assert(t_data.num_base_gens_reporting <= g_gens.size()); // invariant
			assert(t_data.num_base_gens_reporting < g_gens.size()); // or we shouldn't have been called
			auto first = g_gens.begin() + t_data.num_base_gens_reporting;
			t_data.num_base_gens_reporting = g_gens.size();
			assert(t_data.num_parent_gens_chain == 1); // the root doesn't need this
			assert(t_data.num_gens_reported == 1); // the root doesn't use this
			return make_aut_range(first, g_gens.end());
		}
		// not the root
		assert(t_data.num_base_gens_chain <= g_gens.size()); // invariant
		assert(t_data.num_base_gens_reporting <= t_data.num_base_gens_chain); // invariant
		assert(t_data.num_base_gens_reporting < g_gens.size()); // or we shouldn't have been called
		if(t_parent->get_parent())
			assert(t_data.num_parent_gens_chain <= get(tree_data_t(), t_parent->data).stab->generators().size()); // invariant
		else
			assert(t_data.num_parent_gens_chain <= g_gens.size()); // invariant
		{ // make sure as much of the chain as possible is redirected here
			TreeNode *next_child = &t;
			for(TreeNode *tt = t.get_parent(); tt; tt = tt->get_parent()) {
				auto &t_data = get(tree_data_t(), tt->data);
				if(t_data.is_main_chain) break;
				t_data.next_child = next_child;
				next_child = tt;
			}
		}

		if(t_data.num_base_gens_chain != g_gens.size()) {
			TreeNode *tt = update_chain_and_ancestors(s, t);
			// update counts
			for(; tt->get_parent(); tt = tt->get_parent()) {
				auto &tt_data = get(tree_data_t(), tt->data);
				if(tt_data.num_base_gens_chain == g_gens.size())
					break;
				const SizeType parent_count =
						tt->get_parent()->get_parent()
						? get(tree_data_t(), tt->get_parent()->data).stab->generators().size()
						: g_gens.size();
				tt_data.num_base_gens_chain = g_gens.size();
				tt_data.num_parent_gens_chain = parent_count;
			}

			assert(t_data.num_base_gens_chain == g_gens.size()); // invariant
			if(t_parent->get_parent())
				assert(t_data.num_parent_gens_chain == get(tree_data_t(), t_parent->data).stab->generators().size()); // invariant
			else
				assert(t_data.num_parent_gens_chain == g_gens.size()); // invariant
		}
		// and now we are ready to provide automorphisms
		assert(t_data.stab);
		const auto t_gens = t_data.stab->generators();
		const auto old_size = t_data.num_gens_reported;
		t_data.num_base_gens_reporting = g_gens.size();
		t_data.num_gens_reported = t_gens.size();
		return make_aut_range(t_gens.begin() + old_size, t_gens.end());
	}
private:

	template<typename State, typename TreeNode, typename Iter>
	static void generator_added(State &s, TreeNode &t_parent, Iter first, Iter lastOld, Iter lastNew) {
		auto &t_parent_data = get(tree_data_t(), t_parent.data);
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
		detail::printTreeNodeName(std::cout << "Schreier: generator_added to ", t_parent) << std::endl;
		for(auto d_iter = lastOld; d_iter != lastNew; ++d_iter)
			perm_group::write_permutation_cycles(std::cout << "          ", **d_iter) << std::endl;
#endif
		TreeNode * t_child = t_parent_data.next_child;
		if(!t_child) {
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
			detail::printTreeNodeName(std::cout << "Schreier: \tno next_child in ", t_parent) << std::endl;
#endif
			// TODO: Try to look for another child?
			return;
		}
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
		detail::printTreeNodeName(std::cout << "Schreier: \tnext_child is ", *t_child) << std::endl;
#endif
		auto &t_child_data = get(tree_data_t(), t_child->data);
		Adder<State, TreeNode> adder(&s, t_child);
		if(auto *stab = t_child_data.stab.get()) {
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
			std::cout << "Schreier: \textending stab" << std::endl;
#endif
			stab->add_generators(first, lastOld, lastNew, adder);
		} else {
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
			std::cout << "Schreier: \tcreating stab" << std::endl;
#endif
			assert(false);
			//			const auto new_v_idx_to_fix = t_child->pi.get(t_parent->get_child_individualized_position());
			//			Alloc<SizeType> alloc = i_data.g->get_allocator();
			//			using IData = instance_data<SizeType, TreeNode>;
			//			DupChecker<TreeNode, IData> dupChecker(&t, &i_data);
			//#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
			//			std::cout << "Schreier: \tcreating stabilizer of " << new_v_idx_to_fix << std::endl;
			//#endif
			//			t_data.stab.reset(new Stab<SizeType, TreeNode, IData>(new_v_idx_to_fix, alloc, dupChecker));
		}
	}

	template<typename State, typename TreeNode>
	TreeNode *update_chain_and_ancestors(State &s, TreeNode &t) {
		// Find the end of the chain from t, and then update everything up to the root.
		// Returns the end of the chain.
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
		detail::printTreeNodeName(std::cout << "Schreier: update_chain_and_ancestors of ", t) << std::endl;
#endif
		using SizeType = typename State::SizeType;
		auto &i_data = get(instance_data_t(), s.data);
		auto &g = *i_data.g;

		TreeNode *leaf = &t;
		while(true) {
			auto &c_data = get(tree_data_t(), leaf->data);
			if(leaf->get_parent() && !c_data.stab) {
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
				detail::printTreeNodeName(std::cout << "Schreier: \tcreating stabilizer in ", *leaf) << std::endl;
#endif
				const auto new_v_idx_to_fix = leaf->pi.get(leaf->get_parent()->get_child_individualized_position());
				Alloc<SizeType> alloc = g.get_allocator();
				using IData = instance_data<SizeType, TreeNode>;
				DupChecker<TreeNode, IData> dupChecker(leaf, &i_data);
				c_data.stab.reset(new Stab<SizeType, TreeNode, IData>(new_v_idx_to_fix, alloc, dupChecker));
			}
			if(!c_data.next_child) break;
			leaf = c_data.next_child;
		}
		// and now go up
		for(TreeNode *child = leaf; child->get_parent(); child = child->get_parent()) {
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
			detail::printTreeNodeName(std::cout << "Schreier: \tupdating ", *child) << std::endl;
#endif
			auto &c_data = get(tree_data_t(), child->data);
			if(c_data.num_base_gens_chain == g.generators().size()) {
#ifdef GRAPH_CANON_AUT_SCHREIER_DEBUG
				detail::printTreeNodeName(std::cout << "Schreier: \tnum_base_gens is up to date, no more update needed", *child) << std::endl;
#endif     
				break;
			}
			// do actual update
			assert(child->get_parent());
			const auto handle_group = [&](const auto &g) {
				const auto gens = g.generator_ptrs();
				const auto lastOld_id = c_data.num_parent_gens_chain;
				c_data.stab->add_generators(gens.begin(), gens.begin() + lastOld_id, gens.end(),
						[&](auto first, auto oldLast, auto oldNew) {
							Adder<State, TreeNode>(&s, child)(first, oldLast, oldNew);
						});
			};
			if(child->get_parent()->get_parent()) {
				const auto &p_data = get(tree_data_t(), child->get_parent()->data);
				handle_group(*p_data.stab);
			} else {
				handle_group(*i_data.g);
			}
		}
		return leaf;
	}
};

} // namespace graph_canon

#endif /* GRAPH_CANON_AUT_PRUNER_SCHREIER_HPP */
