#ifndef GRAPH_CANON_DETAIL_VISITOR_UTILS_HPP
#define GRAPH_CANON_DETAIL_VISITOR_UTILS_HPP

#include <boost/ref.hpp>

#include <functional>

namespace graph_canon {
namespace detail {

void for_each_neighbour(const auto &state, const auto &node, const auto v, const auto callback) {
	const auto oes = out_edges(v, state.g);
	for(auto e_iter = oes.first; e_iter != oes.second; ++e_iter) {
		const auto e_out = *e_iter;
		const auto v_target = target(e_out, state.g);
		const auto v_idx = state.idx[v_target];
		const auto v_pos = node.pi.get_inverse(v_idx);
		const auto target_element_cell = node.pi.get_cell_from_element(v_pos);
		const auto target_element_cell_end = node.pi.get_cell_end(target_element_cell);
		const bool is_singleton = target_element_cell + 1 == target_element_cell_end;
		if(is_singleton) continue;
		callback(e_out, v_pos, target_element_cell, target_element_cell_end);
	}
}

// C++17 stuff
// from http://en.cppreference.com
// -----------------------------------------------------------------------------

template<typename...>
struct conjunction : std::true_type {
};

template<typename B>
struct conjunction<B> : B {
};

template<typename B, typename ...Bs>
struct conjunction<B, Bs...>
: std::conditional_t<bool(B::value), conjunction<Bs...>, B> {
};

template<typename...>
struct disjunction : std::false_type {
};

template<typename B>
struct disjunction<B> : B {
};

template<typename B, typename ...Bs>
struct disjunction<B, Bs...>
: std::conditional_t<bool(B::value), B, disjunction<Bs...> > {
};

// Based on Peter Dimov: Simple C++11 metaprogramming 
// -----------------------------------------------------------------------------

template<typename ...T>
struct meta_list {
};

template<typename A, template<typename...> class B>
struct meta_rename_impl;

template<template<typename...> class A, typename ...T, template<typename...> class B>
struct meta_rename_impl<A<T...>, B> {
	using type = B<T...>;
};

template<typename A, template<typename...> class B>
using meta_rename = typename meta_rename_impl<A, B>::type;

template<typename T>
struct meta_pop_front;

template<template<typename ...> class A, typename T, typename ...Ts>
struct meta_pop_front<A<T, Ts...> > {
	using type = A<Ts...>;
};

template<typename T>
struct meta_front;

template<template<typename ...> class A, typename T, typename ...Ts>
struct meta_front<A<T, Ts...> > {
	using type = T;
};

template<typename List1, typename List2>
struct meta_concat;

template<template<typename...> class L1, template<typename...> class L2, typename ...L1s, typename ...L2s>
struct meta_concat<L1<L1s...>, L2<L2s...> > {
	using type = meta_list<L1s..., L2s...>;
};

template<typename T>
struct meta_size;

template<template<typename...> class A, typename ...T>
struct meta_size<A<T...> > {
	constexpr static std::size_t value = sizeof...(T);
};

template<typename T, typename List>
struct meta_one_of;

template<typename T, template<typename...> class List, typename ...Elems>
struct meta_one_of<T, List<Elems...> > {
	using type = typename disjunction<std::is_same<T, Elems>...>::type;
};

template<typename T, bool c>
struct meta_optional {
	using type = meta_list<T>;
};

template<typename T>
struct meta_optional<T, false> {
	using type = meta_list<>;
};

template<typename List, template<typename> class P>
struct meta_copy_if_impl;

template<template<typename...> class L, template<typename> class P>
struct meta_copy_if_impl<L<>, P> {
	using type = meta_list<>;
};

template<template<typename...> class L, template<typename> class P, typename T, typename ...Ts>
struct meta_copy_if_impl<L<T, Ts...>, P> {
	using head = typename meta_optional<T, P<T>::value>::type;
	using tail = typename meta_copy_if_impl<L<Ts...>, P>::type;
	using type = typename meta_concat<head, tail>::type;
};

template<typename List, template<typename> class P>
struct meta_copy_if {
	using type = typename meta_copy_if_impl<List, P>::type;
};

// unwrap visitor in reference_wrappers
// -----------------------------------------------------------------------------

template<typename T>
struct unwrap_visitor {
	using type = T;

	static T &get(T &t) {
		return t;
	}
};

template<typename T>
struct do_unwrap {
	using type = typename T::type;

	static type &get(T t) {
		return t.get();
	}
};

template<typename T>
struct unwrap_visitor<boost::reference_wrapper<T> > : do_unwrap<boost::reference_wrapper<T> > {
};

template<typename T>
struct unwrap_visitor<std::reference_wrapper<T> > : do_unwrap<std::reference_wrapper<T> > {
};

template<typename T>
decltype(auto) get_unwrapped(T &t) {
	return unwrap_visitor<T>::get(t);
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
	using type = typename std::conditional<unwrap_visitor<V>::type::can_select_target_cell::value,
			std::integral_constant<std::size_t, I>,
			typename target_cell_selector_index_impl<I + 1, Vs...>::type>::type;
};

template<typename ...Vs>
using target_cell_selector_index = typename target_cell_selector_index_impl<0, Vs...>::type;

template<typename V>
using pred_target_cell = typename unwrap_visitor<V>::type::can_select_target_cell;

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
	using type = typename std::conditional<unwrap_visitor<V>::type::can_explore_tree::value,
			std::integral_constant<std::size_t, I>,
			typename explore_tree_index_impl<I + 1, Vs...>::type>::type;
};

template<typename ...Vs>
using explore_tree_index = typename explore_tree_index_impl<0, Vs...>::type;

template<typename V>
using pred_tree_traversal = typename unwrap_visitor<V>::type::can_explore_tree;


// visitor duplication check
// -----------------------------------------------------------------------------

template<typename Visitor>
struct check_visitor_duplication_impl {
	using This = typename meta_front<Visitor>::type;
	using Rest = typename meta_pop_front<Visitor>::type;
	using Found = typename meta_one_of<This, Rest>::type;
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