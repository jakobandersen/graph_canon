#ifndef GRAPH_CANON_UTIL_HPP
#define GRAPH_CANON_UTIL_HPP

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/property_map/property_map.hpp>

#include <cstdio>
#include <map>
#include <vector>

namespace graph_canon {

// rst: .. class:: always_true
// rst:
// rst:		A function object always returning `true`.
// rst:

struct always_true {
	// rst:		.. function:: template<typename ...T> \
	// rst:		              bool operator()(T&&...) const

	template<typename ...T>
	bool operator()(T&&...) const {
		return true;
	}
};

// rst: .. class:: always_false
// rst:
// rst:		A function object always returning `false`.
// rst:

struct always_false {
	// rst:		.. function:: template<typename ...T> \
	// rst:		              bool operator()(T&&...) const

	template<typename ...T>
	bool operator()(T&&...) const {
		return false;
	}
};

// rst: .. class:: template<typename Prop> \
// rst:            property_less
// rst:
// rst:		A less-than comparator on a given property map, satisfying the `boost::ReadablePropertyMapConcept`.
// rst:

template<typename Prop>
struct property_less {
	// rst:		.. function:: property_less(Prop prop)

	property_less(Prop prop) : prop(prop) { }

	// rst:		.. function:: template<typename Key> \
	// rst:		              bool operator()(const Key &lhs, const Key &rhs) const
	// rst:
	// rst:			:returns: get(prop, lhs) < get(prop, rhs)

	template<typename Key>
	bool operator()(const Key &lhs, const Key &rhs) const {
		return get(prop, lhs) < get(prop, rhs);
	}
public:
	// rst:		.. var:: Prop prop
	Prop prop;
};

// rst: .. function:: template<typename Prop> \
// rst:               property_less<Prop> make_property_less(Prop &&prop)
// rst:
// rst:		:returns: `property_less<Prop>(std::forward<Prop>(prop))`

template<typename Prop>
property_less<Prop> make_property_less(Prop &&prop) {
	return property_less<Prop>(std::forward<Prop>(prop));
}

// rst: .. function:: template<typename Graph, typename SizeType> \
// rst:               void permute_graph(const Graph &g_in, Graph &g_out, const std::vector<SizeType> &permutation)
// rst:
// rst:		Add all vertices and edges from `g_in` to `g_out`,
// rst:		but where the order of vertex addition is given by the `permutation`.
// rst:		It must be a permutation of the vertex indices of `g_in` and is interpreted as a map
// rst:		from the indices in `g_in` to the indices they will get in `g_out` (offset by `num_vertices(g_out)` before the call).

template<typename Graph, typename SizeType>
void permute_graph(const Graph &g_in, Graph &g_out, const std::vector<SizeType> &permutation) {
	using Vertex = typename boost::graph_traits<Graph>::vertex_descriptor;
	std::map<Vertex, Vertex> vertex_map;
	std::vector<SizeType> inverse(permutation.size());
	for(std::size_t i = 0; i < num_vertices(g_in); i++) inverse[permutation[i]] = i;

	for(std::size_t i = 0; i < num_vertices(g_in); i++) {
		Vertex orig_vertex = vertex(inverse[i], g_in);
		Vertex new_vertex = vertex_map[orig_vertex] = add_vertex(g_out);
		put(boost::vertex_all_t(), g_out, new_vertex, get(boost::vertex_all_t(), g_in, orig_vertex));
	}

	BGL_FORALL_EDGES_T(e, g_in, Graph) {
		auto e_new = add_edge(vertex_map[source(e, g_in)], vertex_map[target(e, g_in)], g_out);
		put(boost::edge_all_t(), g_out, e_new.first, get(boost::edge_all_t(), g_in, e));
	}
}

} // namespace graph_canon

#endif /* GRAPH_CANON_UTIL_HPP */