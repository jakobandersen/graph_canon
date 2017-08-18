#ifndef GRAPH_CANON_TAGGED_LIST_HPP
#define GRAPH_CANON_TAGGED_LIST_HPP

#include <graph_canon/config.hpp>

namespace graph_canon {

struct tagged_list_empty {
};

template<typename Tag, typename T, typename Tail = tagged_list_empty>
struct tagged_list {
	T value;
	Tail base;
};

namespace detail {

template<typename Data, typename UTag>
struct tagged_list_lookup;

template<typename Tag>
struct tagged_list_lookup<tagged_list_empty, Tag> {
	static void get(tagged_list_empty) = delete;
};

template<>
template<typename Tag, typename T, typename Tail>
struct tagged_list_lookup<tagged_list<Tag, T, Tail>, Tag> {

	static T &get(tagged_list<Tag, T, Tail> &data) {
		return data.value;
	}

	static const T &get(const tagged_list<Tag, T, Tail> &data) {
		return data.value;
	}
};

template<>
template<typename Tag, typename T, typename Tail, typename UTag>
struct tagged_list_lookup<tagged_list<Tag, T, Tail>, UTag> {
	static auto get(tagged_list<Tag, T, Tail> &data) -> decltype(tagged_list_lookup<Tail, UTag>::get(data.base)) {
		return tagged_list_lookup<Tail, UTag>::get(data.base);
	}

	static auto get(const tagged_list<Tag, T, Tail> &data) -> decltype(tagged_list_lookup<Tail, UTag>::get(data.base)) {
		return tagged_list_lookup<Tail, UTag>::get(data.base);
	}
};

} // namespace detail

template<typename Data, typename UTag>
auto get(UTag, Data &data) -> decltype(detail::tagged_list_lookup<Data, UTag>::get(data)) {
	return detail::tagged_list_lookup<Data, UTag>::get(data);
}

template<typename Data, typename UTag>
auto get(UTag, const Data &data) -> decltype(detail::tagged_list_lookup<Data, UTag>::get(data)) {
	return detail::tagged_list_lookup<Data, UTag>::get(data);
}

template<typename Prefix, typename Suffix>
struct tagged_list_concat;

template<typename Suffix>
struct tagged_list_concat<tagged_list_empty, Suffix> {
	using type = Suffix;
};

template<typename Tag, typename T, typename Tail, typename Suffix>
struct tagged_list_concat<tagged_list<Tag, T, Tail>, Suffix> {
	using type = tagged_list<Tag, T, typename tagged_list_concat<Tail, Suffix>::type>;
};


template<typename ...TLs>
struct tagged_list_concat_many;

template<>
struct tagged_list_concat_many<> {
	using type = tagged_list_empty;
};

template<typename TL>
struct tagged_list_concat_many<TL> {
	using type = TL;
};

template<typename TL1, typename TL2, typename ...TLs>
struct tagged_list_concat_many<TL1, TL2, TLs...> {
	using tail = typename tagged_list_concat_many<TL2, TLs...>::type;
	using type = typename tagged_list_concat<TL1, tail>::type;
};

} // namespace graph_canon

#endif // GRAPH_CANON_TAGGED_LIST_HPP