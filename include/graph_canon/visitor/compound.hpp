#ifndef GRAPH_CANON_VISITOR_COMPOUND_HPP
#define GRAPH_CANON_VISITOR_COMPOUND_HPP

#include <graph_canon/visitor/visitor.hpp>

#include <graph_canon/detail/visitor_utils.hpp>

#include <perm_group/permutation/permutation.hpp>

#include <cassert>
#include <tuple>

namespace graph_canon {

// rst: .. class:: template<typename ...Visitors> \
// rst:            compound_visitor
// rst:
// rst:		A visitor aggregating a list of given visitors, `Visitors`.
// rst:		Use `make_visitor` to easily create such compound visitors.
// rst:

template<typename ...Visitors>
struct compound_visitor {
	using can_select_target_cell = disjunction<typename Visitors::can_select_target_cell...>;
	using can_explore_tree = disjunction<typename Visitors::can_explore_tree...>;

	template<typename Config, typename TreeNode>
	struct InstanceData {
		using type = typename tagged_list_concat<
				/**/ typename Visitors::template InstanceData<Config, TreeNode>::type...
				>::type;
	};

	template<typename Config, typename TreeNode>
	struct TreeNodeData {
		using type = typename tagged_list_concat<
				/**/ typename Visitors::template TreeNodeData<Config, TreeNode>::type...
				>::type;
	};
private:
	// the visitors need to be dependent typenames to postpone function lookup

	template<typename State, typename TreeNode, typename ...Vs>
	std::size_t select_target_cell_impl(State &state, TreeNode &t, std::tuple<Vs...> &visitors, std::true_type) {
		using Index = detail::target_cell_selector_index<Vs...>;
		return std::get<Index::value>(visitors).select_target_cell(state, t);
	}

	template<typename State, typename TreeNode, typename VsTuple>
	std::size_t select_target_cell_impl(State &state, TreeNode &t, VsTuple&, std::false_type) = delete;
public: // for selecting target cell

	template<typename State, typename TreeNode>
	std::size_t select_target_cell(State &state, TreeNode &t) {
		return select_target_cell_impl(state, t, visitors, can_select_target_cell());
	}
private:
	// the visitors need to be dependent typenames to postpone function lookup

	template<typename State, typename ...Vs>
	void explore_tree_impl(State &state, std::tuple<Vs...> &visitors, std::true_type) {
		using Index = detail::explore_tree_index<Vs...>;
		return std::get<Index::value>(visitors).explore_tree(state);
	}

	template<typename State, typename VsTuple>
	void explore_tree_impl(State &state, VsTuple&, std::false_type) = delete;
public: // for selecting target cell

	template<typename State>
	void explore_tree(State &state) {
		return explore_tree_impl(state, visitors, can_explore_tree());
	}
public:

	compound_visitor(Visitors ...visitors) : visitors(visitors...) { }

	template<typename State>
	void initialize(State &state) {
		detail::tuple_for_each(visitors, [&](auto &v) {
			v.initialize(state);
		});
	}

	template<typename State>
	auto extract_result(State &state) {
		return detail::extract_result(state, visitors);
	}
public:

	template<typename State, typename TreeNode>
	bool tree_create_node_begin(State &state, TreeNode &t) {
		// no short-circuiting
		bool res = true;
		detail::tuple_for_each(visitors, [&](auto &v) {
			const bool b = v.tree_create_node_begin(state, t);
			res = res && b;
		});
		return res;
	}

	template<typename State, typename TreeNode>
	bool tree_create_node_end(State &state, TreeNode &t) {
		// no short-circuiting
		bool res = true;
		detail::tuple_for_each(visitors, [&](auto &v) {
			const bool b = v.tree_create_node_end(state, t);
			res = res && b;
		});
		return res;
	}

	template<typename State, typename TreeNode>
	void tree_destroy_node(State &state, TreeNode &t) {
		detail::tuple_for_each(visitors, [&](auto &v) {
			v.tree_destroy_node(state, t);
		});
	}

	template<typename State, typename TreeNode>
	void tree_before_descend(State &state, TreeNode &t) {
		detail::tuple_for_each(visitors, [&](auto &v) {
			v.tree_before_descend(state, t);
		});
	}

	template<typename State, typename TreeNode>
	void tree_create_child(State &state, TreeNode &t, std::size_t element_idx_to_individualise) {
		detail::tuple_for_each(visitors, [&](auto &v) {
			v.tree_create_child(state, t, element_idx_to_individualise);
		});
	}

	template<typename State, typename TreeNode>
	void tree_leaf(State &state, TreeNode &t) {
		detail::tuple_for_each(visitors, [&](auto &v) {
			v.tree_leaf(state, t);
		});
	}

	template<typename State, typename TreeNode>
	void tree_prune_node(State &state, TreeNode &t) {
		detail::tuple_for_each(visitors, [&](auto &v) {
			v.tree_prune_node(state, t);
		});
	}

	template<typename State>
	void canon_new_best(State &state) {
		detail::tuple_for_each(visitors, [&](auto &v) {
			v.canon_new_best(state);
		});
	}

	template<typename State, typename TreeNode>
	void canon_worse(State &state, TreeNode &t) {
		detail::tuple_for_each(visitors, [&](auto &v) {
			v.canon_worse(state, t);
		});
	}

	template<typename State>
	void canon_prune(State &state) {
		detail::tuple_for_each(visitors, [&](auto &v) {
			v.canon_prune(state);
		});
	}

	template<typename State, typename TreeNode, typename Perm>
	void automorphism_leaf(State &state, TreeNode &t, const Perm &aut) {
		BOOST_CONCEPT_ASSERT((perm_group::Permutation<Perm>));
		detail::tuple_for_each(visitors, [&](auto &v) {
			v.automorphism_leaf(state, t, aut);
		});
	}

	template<typename State, typename TreeNode, typename Perm>
	void automorphism_implicit(State &state, TreeNode &t, const Perm &aut, std::size_t tag) {
		BOOST_CONCEPT_ASSERT((perm_group::Permutation<Perm>));
		detail::tuple_for_each(visitors, [&](auto &v) {
			v.automorphism_implicit(state, t, aut, tag);
		});
	}

	template<typename State, typename TreeNode>
	RefinementResult refine(State &state, TreeNode &node) {
		// stop as soon as possible
		auto res = RefinementResult::Never;
		detail::tuple_for_each_lazy(visitors, [&](auto &v) {
			const auto r = v.refine(state, node);
			//		std::cout << "Refine: " << (sizeof...(Visitors) + 1);
			//		std::cout << " " << res;
			//		std::cout << " " << r;
			//		std::cout << std::endl;
			//		std::cout << "     " << typeid(v).name() << std::endl;
			if(r == RefinementResult::Abort) {
				res = RefinementResult::Abort;
				return false;
			} else if(res == RefinementResult::Never) {
				res = r;
				return true;
			} else if(r == RefinementResult::Never) {
				return true;
			} else if(res == RefinementResult::Again || r == RefinementResult::Again) {
				return true;
			}
			assert(res == RefinementResult::Unchanged || res == RefinementResult::Changed);
			assert(r == RefinementResult::Unchanged || r == RefinementResult::Changed);
			// Unchaged, Unchanged -> Unchanged
			// Changed, Unchanged -> Changed
			// Unchaged, Changed -> Again // the tail can affect the head
			// Changed, Changed -> Again // the tail can affect the head
			if(r == RefinementResult::Changed) {
				res = RefinementResult::Again;
			}
			return true;
		});
		return res;
	}

	template<typename State, typename TreeNode>
	void refine_cell_split_begin(State &state, TreeNode &t, std::size_t refiner_begin,
			std::size_t refinee_begin, std::size_t refinee_end) {
		detail::tuple_for_each(visitors, [&](auto &v) {
			v.refine_cell_split_begin(state, t, refiner_begin, refinee_begin, refinee_end);
		});
	}

	template<typename State, typename TreeNode>
	bool refine_new_cell(State &state, TreeNode &t, std::size_t new_cell, std::size_t type) {
		// no short-circuiting
		bool res = true;
		detail::tuple_for_each(visitors, [&](auto &v) {
			const bool b = v.refine_new_cell(state, t, new_cell, type);
			res = res && b;
		});
		return res;
	}

	template<typename State, typename TreeNode>
	void refine_cell_split_end(State &state, TreeNode &t, std::size_t refiner_begin,
			std::size_t refinee_begin, std::size_t refinee_end) {
		detail::tuple_for_each(visitors, [&](auto &v) {
			v.refine_cell_split_end(state, t, refiner_begin, refinee_begin, refinee_end);
		});
	}

	template<typename State, typename TreeNode>
	bool refine_quotient_edge(State &state, TreeNode &t, std::size_t refiner, std::size_t refinee, std::size_t count) {
		// no short-circuiting
		bool res = true;
		detail::tuple_for_each(visitors, [&](auto &v) {
			const bool b = v.refine_quotient_edge(state, t, refiner, refinee, count);
			res = res && b;
		});
		return res;
	}

	template<typename State, typename TreeNode>
	bool refine_refiner_done(State &state, TreeNode &t, const std::size_t refiner, const std::size_t refiner_end) {
		// no short-circuiting
		bool res = true;
		detail::tuple_for_each(visitors, [&](auto &v) {
			const bool b = v.refine_refiner_done(state, t, refiner, refiner_end);
			res = res && b;
		});
		return res;
	}

	template<typename State, typename TreeNode>
	void refine_abort(State &state, TreeNode &t) {
		detail::tuple_for_each(visitors, [&](auto &v) {
			v.refine_abort(state, t);
		});
	}

	template<typename State, typename TreeNode>
	void invariant_better(State &state, TreeNode &t) {
		detail::tuple_for_each(visitors, [&](auto &v) {
			v.invariant_better(state, t);
		});
	}
public:
	std::tuple<Visitors...> visitors;
};

template<>
struct compound_visitor<> : null_visitor {
};


// make_visitor
//------------------------------------------------------------------------------

namespace detail {

template<typename V>
struct flatten_visitor {

	static auto make(V v) {
		return std::tuple<V>(v);
	}
};

template<typename ...V>
struct flatten_visitor<compound_visitor<V...> > {

	template<std::size_t ...Idx>
	static auto make_impl(compound_visitor<V...> v, std::integer_sequence<std::size_t, Idx...>) {
		using T = std::tuple<V...>;
		return std::tuple_cat(
				flatten_visitor<
				/**/ typename std::tuple_element<Idx, T>::type
				>::make(std::get<Idx>(v.visitors))...
				);
	}

	static auto make(compound_visitor<V...> v) {
		return make_impl(v, std::make_index_sequence<sizeof...(V)>());
	}
};

template<>
struct flatten_visitor<compound_visitor<> > {

	static auto make(compound_visitor<> v) {
		return std::tuple<>();
	}
};

template<typename ...V>
struct visitor_from_tuple_impl {

	template<std::size_t ...Idx>
	static auto make_impl(std::tuple<V...> t, std::integer_sequence<std::size_t, Idx...>) {
		return compound_visitor<V...>(std::get<Idx>(t)...);
	}

	static auto make(std::tuple<V...> t) {
		return make_impl(t, std::make_index_sequence<sizeof...(V)>());
	}
};

template<>
struct visitor_from_tuple_impl<> {

	static auto make(std::tuple<> t) {
		return compound_visitor<>();
	}
};

template<typename ...V>
auto visitor_from_tuple(std::tuple<V...> t) {
	return visitor_from_tuple_impl<V...>::make(t);
}

} // detail

// rst: .. function:: template<typename ...Visitors> \
// rst:               auto make_visitor(Visitors ...visitors)
// rst:
// rst:		Before creation any given `compound_visitor` is inlined (recursively)
// rst:		such that the resulting `compound_visitor` contains only non-compound visitors.
// rst:
// rst:		:returns: a `compound_visitor` with all visitors contained in the given `visitors` list.
// rst:

template<typename ...Visitors>
auto make_visitor(Visitors ...visitors) {
	auto v = compound_visitor<Visitors...>(visitors...);
	auto t = detail::flatten_visitor<compound_visitor<Visitors...> >::make(v);
	auto res = detail::visitor_from_tuple(t);
	using Res = decltype(res);
	using DuplicateVisitor = detail::check_visitor_duplication<Res>;
	static_assert(std::is_same<DuplicateVisitor, void>::value, "Visitors must be unique.");
	return res;
}

} // namespace graph_canon

#endif /* GRAPH_CANON_VISITOR_COMPOUND_HPP */