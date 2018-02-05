#ifndef GRAPH_CANON_DETAIL_META_HPP
#define GRAPH_CANON_DETAIL_META_HPP

#include <functional>

namespace graph_canon {

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

namespace meta {

// Based on Peter Dimov: Simple C++11 metaprogramming 
// #############################################################################

// List
// -----------------------------------------------------------------------------

template<typename ...T>
struct list {
};

// Rename
// -----------------------------------------------------------------------------

template<typename A, template<typename...> class B>
struct rename_impl;

template<template<typename...> class A, typename ...T, template<typename...> class B>
struct rename_impl<A<T...>, B> {
	using type = B<T...>;
};

template<typename A, template<typename...> class B>
using rename = typename rename_impl<A, B>::type;

// Pop front
// -----------------------------------------------------------------------------

template<typename T>
struct pop_front_impl;

template<template<typename ...> class A, typename T, typename ...Ts>
struct pop_front_impl<A<T, Ts...> > {
	using type = A<Ts...>;
};

template<typename T>
using pop_front = typename pop_front_impl<T>::type;

// Front
// -----------------------------------------------------------------------------

template<typename T>
struct front_impl;

template<template<typename ...> class A, typename T, typename ...Ts>
struct front_impl<A<T, Ts...> > {
	using type = T;
};

template<typename T>
using front = typename front_impl<T>::type;

// Concat
// -----------------------------------------------------------------------------

template<typename L1, typename L2>
struct concat_impl;

template<template<typename...> class L1, template<typename...> class L2, typename ...L1s, typename ...L2s>
struct concat_impl<L1<L1s...>, L2<L2s...> > {
	using type = list<L1s..., L2s...>;
};

template<typename L1, typename L2>
using concat = typename concat_impl<L1, L2>::type;

// Size
// -----------------------------------------------------------------------------

template<typename T>
struct size;

template<template<typename...> class A, typename ...T>
struct size<A<T...> > : std::integral_constant<std::size_t, sizeof...(T)> {
};

// One of
// -----------------------------------------------------------------------------

template<typename T, typename List>
struct one_of_impl;

template<typename T, template<typename...> class List, typename ...Elems>
struct one_of_impl<T, List<Elems...> > {
	using type = typename disjunction<std::is_same<T, Elems>...>::type;
};

template<typename T, typename List>
using one_of = typename one_of_impl<T, List>::type;

// Optional
// -----------------------------------------------------------------------------

template<typename T, bool c>
struct optional_impl {
	using type = list<T>;
};

template<typename T>
struct optional_impl<T, false> {
	using type = list<>;
};

template<typename T, bool c>
using optional = typename optional_impl<T, c>::type;

// Copy if
// -----------------------------------------------------------------------------

template<typename List, template<typename> class P>
struct copy_if_impl;

template<template<typename...> class L, template<typename> class P>
struct copy_if_impl<L<>, P> {
	using type = list<>;
};

template<template<typename...> class L, template<typename> class P, typename T, typename ...Ts>
struct copy_if_impl<L<T, Ts...>, P> {
	using head = optional<T, P<T>::value>;
	using tail = typename copy_if_impl<L<Ts...>, P>::type;
	using type = concat<head, tail>;
};

template<typename List, template<typename> class P>
using copy_if = typename copy_if_impl<List, P>::type;

// Transform
// -----------------------------------------------------------------------------

template<template<typename> class F, typename List>
struct transform_impl;

template<template<typename> class F, template<typename...> class List, typename ...T>
struct transform_impl<F, List<T...> > {
	using type = List<F<T>...>;
};

template<template<typename> class F, typename List>
using transform = typename transform_impl<F, List>::type;

} // meta
} // namespace graph_canon

#endif /* GRAPH_CANON_DETAIL_META_HPP */