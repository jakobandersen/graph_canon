#ifndef GRAPH_CANON_DETAIL_VISITOR_UTILS_HPP
#define GRAPH_CANON_DETAIL_VISITOR_UTILS_HPP

#include <graph_canon/detail/meta.hpp>

#include <boost/ref.hpp>

namespace graph_canon {
namespace detail {

template<typename State, typename TreeNode, typename Vertex, typename Callback>
inline void for_each_neighbour(const State &state, const TreeNode &node, const Vertex v, const Callback callback) {
	const auto &pi = node.pi;
	const auto *begin_cell_from_v_idx = pi.begin_cell_from_v_idx();
	const auto *begin_cell_end = pi.begin_cell_end();
	const auto oes = out_edges(v, state.g);
	for(auto e_iter = oes.first; e_iter != oes.second; ++e_iter) {
		const auto e_out = *e_iter;
		const auto v_target = target(e_out, state.g);
		const auto v_idx = get(state.idx, v_target);
		const auto target_element_cell = begin_cell_from_v_idx[v_idx];
		const auto target_element_cell_end = begin_cell_end[target_element_cell];
		const bool is_singleton = target_element_cell + 1 == target_element_cell_end;
		if(is_singleton) continue;
		callback(e_out, v_idx, target_element_cell, target_element_cell_end);
	}
}

// extract result
// -----------------------------------------------------------------------------

template<typename State, typename T, std::size_t ...I>
auto extract_result_impl(State &state, T &&t, std::integer_sequence<std::size_t, I...>) {
	return make_tagged_list(std::get<I>(t).extract_result(state)...);
}

template<typename State, typename ...Vs>
auto extract_result(State &state, std::tuple<Vs...> &t) {
	constexpr std::size_t Size = sizeof...(Vs);
	return extract_result_impl(state, t, std::make_index_sequence<Size>());
}

// tuple for each
// -----------------------------------------------------------------------------

template<typename T, typename F, std::size_t ...I>
void tuple_for_each_impl(T &&t, F f, std::integer_sequence<std::size_t, I...>) {
	int unused[] = {(f(std::get<I>(t)), 0)...};
	(void) unused;
}

template<typename ...Vs, typename F>
void tuple_for_each(std::tuple<Vs...> &t, F f) {
	constexpr std::size_t Size = sizeof...(Vs);
	tuple_for_each_impl(t, f, std::make_index_sequence<Size>());
}

// tuple for each lazy
// -----------------------------------------------------------------------------

template<std::size_t I, std::size_t N, typename F, typename ...Vs>
struct tuple_for_each_lazy_impl {

	static bool apply(std::tuple<Vs...> &t, F f) {
		const bool continue_ = f(std::get<I>(t));
		if(!continue_) return false;
		else return tuple_for_each_lazy_impl < I + 1, N, F, Vs...>::apply(t, f);
	}
};

template<std::size_t N, typename F, typename ...Vs>
struct tuple_for_each_lazy_impl<N, N, F, Vs...> {

	static bool apply(std::tuple<Vs...> &t, F f) {
		return false;
	}
};

template<typename ...Vs, typename F>
void tuple_for_each_lazy(std::tuple<Vs...> &t, F f) {
	tuple_for_each_lazy_impl < 0, sizeof...(Vs), F, Vs...>::apply(t, f);
}

// target cell selector
// -----------------------------------------------------------------------------

template<std::size_t I, typename ...Vs>
struct target_cell_selector_index_impl;

template<std::size_t I>
struct target_cell_selector_index_impl<I> {
	using type = void;
};

template<std::size_t I, typename V, typename ...Vs>
struct target_cell_selector_index_impl<I, V, Vs...> {
	using type = typename std::conditional<V::can_select_target_cell::value,
			std::integral_constant<std::size_t, I>,
			typename target_cell_selector_index_impl<I + 1, Vs...>::type>::type;
};

template<typename ...Vs>
using target_cell_selector_index = typename target_cell_selector_index_impl<0, Vs...>::type;

template<typename V>
using pred_target_cell = typename V::can_select_target_cell;

// tree traversal
// -----------------------------------------------------------------------------

template<std::size_t I, typename ...Vs>
struct explore_tree_index_impl;

template<std::size_t I>
struct explore_tree_index_impl<I> {
	using type = void;
};

template<std::size_t I, typename V, typename ...Vs>
struct explore_tree_index_impl<I, V, Vs...> {
	using type = typename std::conditional<V::can_explore_tree::value,
			std::integral_constant<std::size_t, I>,
			typename explore_tree_index_impl<I + 1, Vs...>::type>::type;
};

template<typename ...Vs>
using explore_tree_index = typename explore_tree_index_impl<0, Vs...>::type;

template<typename V>
using pred_tree_traversal = typename V::can_explore_tree;


// visitor duplication check
// -----------------------------------------------------------------------------

template<typename Visitor>
struct check_visitor_duplication_impl {
	using This = meta::front<Visitor>;
	using Rest = meta::pop_front<Visitor>;
	using Found = meta::one_of<This, Rest>;
	using Next = typename check_visitor_duplication_impl<Rest>::type;
	using type = typename std::conditional<Found::value, This, Next>::type;
};

template<template<typename...> class Visitor>
struct check_visitor_duplication_impl<Visitor<> > {
	using type = void;
};

template<typename Visitor>
using check_visitor_duplication = typename check_visitor_duplication_impl<Visitor>::type;

} // namespace detail
} // namespace graph_canon

#endif /* GRAPH_CANON_DETAIL_VISITOR_UTILS_HPP */