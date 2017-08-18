#ifndef GRAPH_CANON_AUT_PRUNER_BASIC_HPP
#define GRAPH_CANON_AUT_PRUNER_BASIC_HPP

#include <graph_canon/aut/pruner_base.hpp>

#include <perm_group/allocator/raw_ptr.hpp>
#include <perm_group/group/generated.hpp>
#include <perm_group/group/basic_stabilizer.hpp>
#include <perm_group/orbit.hpp>
#include <perm_group/permutation/built_in.hpp>

#include <cassert>
#include <vector>

namespace graph_canon {

struct aut_pruner_basic : aut_pruner_base<aut_pruner_basic> {
	using Base = aut_pruner_base<aut_pruner_basic>;
public: // for aut_pruner_base
	template<typename SizeType>
	using BasePerm = std::vector<SizeType>;
	template<typename SizeType>
	using Perm = wrapped_perm<BasePerm<SizeType> >;
public:
	template<typename SizeType>
	using Alloc = perm_group::raw_ptr_allocator<Perm<SizeType> >;
	template<typename SizeType>
	using BaseGroup = perm_group::generated_group<Perm<SizeType>, Alloc<SizeType> >;
	template<typename SizeType>
	using Stab = perm_group::basic_updatable_stabilizer<BaseGroup<SizeType> >;
public:

	struct instance_data_t {
	};

	template<typename SizeType>
	struct instance_data {
		std::unique_ptr<BaseGroup<SizeType> > g;
		SizeType root_count = 1;
	};

	template<typename Config, typename TreeNode>
	struct InstanceData {
		using base_type = typename Base::InstanceData<Config, TreeNode>::type;
		using type = tagged_list<instance_data_t, instance_data<typename Config::SizeType>, base_type>;
	};

	struct tree_data_t {
	};

	template<typename SizeType>
	struct tree_data {
		std::unique_ptr<Stab<SizeType> > stab;
	};

	template<typename Config, typename TreeNode>
	struct TreeNodeData {
		using base_type = typename Base::TreeNodeData<Config, TreeNode>::type;
		using type = tagged_list<tree_data_t, tree_data<typename Config::SizeType>, base_type>;
	};
public:

	template<typename State>
	void initialize(State &s) {
		using SizeType = typename State::SizeType;
		assert(!get(instance_data_t(), s.data).g);
		get(instance_data_t(), s.data).g.reset(new BaseGroup<SizeType>(s.n));
	}
public: // for aut_pruner_base

	template<typename State, typename AutPerm>
	void add_automorphism(State &s, const AutPerm &aut_new) {
		using SizeType = typename State::SizeType;
		assert(perm_group::size(aut_new) == s.n);
		auto &i_data = get(instance_data_t(), s.data);
		auto &g = *i_data.g;
		std::vector<SizeType> aut(s.n);
		for(std::size_t i = 0; i < s.n; i++) {
			perm_group::put(aut, i, perm_group::get(aut_new, i));
		}
		for(const auto &gen : generators(g)) {
			if(aut == gen.get_perm()) return;
		}
		g.add_generator(Perm<SizeType>(std::move(aut)));
	}

	template<typename State, typename TreeNode>
	bool need_update(State &s, TreeNode &t) {
		if(t.get_parent()) {
			const auto &data = get(tree_data_t(), t.data);
			if(!data.stab) return true;
			return data.stab->need_update();
		} else { // the root
			return true;
		}
	}

	template<typename State, typename TreeNode>
	auto update(State &s, TreeNode &t) {
		using SizeType = typename State::SizeType;
		auto &i_data = get(instance_data_t(), s.data);
		auto &t_data = get(tree_data_t(), t.data);
		// the root doesn't need additional updating
		if(!t.get_parent()) {
			const auto &g = *i_data.g;
			const auto gens = generators(g);
			const auto first = begin(gens) + i_data.root_count;
			const auto last = end(gens);
			i_data.root_count = gens.size();
			return make_aut_range(first, last);
		}
		if(!t_data.stab) {
			const auto new_v_idx_to_fix = t.pi.get(t.get_parent()->child_refiner_cell);
			if(t.get_parent()->get_parent()) {
				const auto &parentData = get(tree_data_t(), t.get_parent()->data);
				t_data.stab.reset(new Stab<SizeType>(new_v_idx_to_fix, *parentData.stab));
			} else {
				const auto &instanceData = get(instance_data_t(), s.data);
				t_data.stab.reset(new Stab<SizeType>(new_v_idx_to_fix, *instanceData.g));
			}
			const auto gens = generators(*t_data.stab);
			// the first is always the id permutation
			return make_aut_range(begin(gens) + 1, end(gens));
		} else {
			const auto pre_gens = generators(*t_data.stab);
			const auto pre_size = std::distance(begin(pre_gens), end(pre_gens));
			t_data.stab->update();
			const auto post_gens = generators(*t_data.stab);
			assert(pre_size <= std::distance(begin(post_gens), end(post_gens)));
			return make_aut_range(begin(post_gens) + pre_size, end(post_gens));
		}
	}

	template<typename State, typename TreeNode>
	bool is_trivial(State &s, TreeNode &t) {
		if(!t.get_parent()) {
			const auto &instanceData = get(instance_data_t(), s.data);
			const auto gens = generators(*instanceData.g);
			return std::distance(gens.begin(), gens.end()) == 1;
		} else {
			const auto &data = get(tree_data_t(), t.data);
			const auto gens = generators(*data.stab);
			return std::distance(gens.begin(), gens.end()) == 1;
		}
	}
};

} // namespace graph_canon

#endif /* GRAPH_CANON_AUT_PRUNER_BASIC_HPP */