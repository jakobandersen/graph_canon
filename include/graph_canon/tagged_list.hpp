#ifndef GRAPH_CANON_TAGGED_LIST_HPP
#define GRAPH_CANON_TAGGED_LIST_HPP

#include <graph_canon/config.hpp>
#include <graph_canon/detail/meta.hpp>

namespace graph_canon {

// rst: .. class:: template<typename TagT, typename T> \
// rst:            tagged_element
// rst:
// rst:		A wrapper for a value of type `T`, tagged with a type `Tag`.
// rst:

template<typename TagT, typename T>
struct tagged_element {
	// rst:		.. type:: Tag = TagT
	using Tag = TagT;
public:
	// rst:		.. var:: T value
	// rst:
	// rst:			The wrapped value.
	T value;
};

// rst: .. class:: template<typename ...Elems> \
// rst:            tagged_list
// rst:
// rst:		A tuple of tagged elements, e.g., specializations of `tagged_element`.

template<typename ...Elems>
struct tagged_list {
	std::tuple<Elems...> elems;
};

// Get
// -----------------------------------------------------------------------------

namespace detail {

template<typename Tag, typename ...Elems>
struct tagged_list_matches {
	template<typename Elem>
	using pred = typename std::is_same<Tag, typename Elem::Tag>::type;

	using type = meta::copy_if<tagged_list<Elems...>, pred>;
};

} // namespace detail


// rst: .. function:: template<typename Tag, typename ...Elems> \
// rst:               auto &get(Tag&&, tagged_list<Elems...> &t)
// rst:
// rst:		:returns: a reference to the value of a tagged element in `t`, tagged with type `Tag`.
// rst:			For example: `get(MyTag(), myTaggedList)`.

template<typename Tag, typename ...Elems>
auto &get(Tag&&, tagged_list<Elems...> &t) {
	using matches = typename detail::tagged_list_matches<Tag, Elems...>::type;
	constexpr auto size = meta::size<matches>::value;
	static_assert(size <= 1, "Ambiguous get, multiple matches.");
	static_assert(size >= 1, "No matches for given tag.");
	using elem = meta::front<matches>;
	return get<elem>(t.elems).value;
}

// rst: .. function:: template<typename Tag, typename ...Elems> \
// rst:               auto &get(Tag&&, const tagged_list<Elems...> &t)
// rst:
// rst:		:returns: a reference the value of a tagged element in `t`, tagged with type `Tag`.
// rst:			For example: `get(MyTag(), myTaggedList)`.

template<typename Tag, typename ...Elems>
const auto &get(Tag&&, const tagged_list<Elems...> &t) {
	using matches = typename detail::tagged_list_matches<Tag, Elems...>::type;
	constexpr auto size = meta::size<matches>::value;
	static_assert(size <= 1, "Ambiguous get, multiple matches.");
	static_assert(size >= 1, "No matches for given tag.");
	using elem = meta::front<matches>;
	return get<elem>(t.elems).value;
}

// Concat
// -----------------------------------------------------------------------------

template<typename ...Ls>
struct tagged_list_concat;

template<typename Tag, typename T, typename ...Ls>
struct tagged_list_concat<tagged_element<Tag, T>, Ls...> {
	using L = tagged_list<tagged_element<Tag, T> >;
	using type = typename tagged_list_concat<L, Ls...>::type;

	static type make(tagged_element<Tag, T> &&e, Ls&& ...ls) {
		return type{std::tuple_cat(std::tuple<tagged_element<Tag, T> >(std::move(e)), tagged_list_concat<Ls...>::make(std::move(ls)...).elems)};
	}
};

template<typename ...Elems, typename ...Ls>
struct tagged_list_concat<tagged_list<Elems...>, Ls...> {
	using L = typename tagged_list_concat<Ls...>::type;
	using type = typename tagged_list_concat<tagged_list<Elems...>, L>::type;

	static type make(tagged_list<Elems...> &&t, Ls&& ...ls) {
		return type{std::tuple_cat(std::move(t.elems), tagged_list_concat<Ls...>::make(std::move(ls)...).elems)};
	}
};

template<typename ...Elems1, typename ...Elems2>
struct tagged_list_concat<tagged_list<Elems1...>, tagged_list<Elems2...> > {
	using type = tagged_list<Elems1..., Elems2...>;

	static type make(tagged_list<Elems1...> &&t1, tagged_list<Elems2...> &&t2) {
		return type{std::tuple_cat(t1.elems, t2.elems)};
	}
};

template<>
struct tagged_list_concat<> {
	using type = tagged_list<>;

	static type make() {
		return {};
	}
};

// Make tagged list
// -----------------------------------------------------------------------------

template<typename ...Elems>
auto make_tagged_list(Elems&&... elems) {
	return tagged_list_concat<Elems...>::make(std::forward<Elems>(elems)...);
}

} // namespace graph_canon

#endif // GRAPH_CANON_TAGGED_LIST_HPP