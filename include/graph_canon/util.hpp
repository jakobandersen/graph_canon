#ifndef GRAPH_CANON_UTIL_HPP
#define GRAPH_CANON_UTIL_HPP

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/property_map/property_map.hpp>

#include <cstdio>
#include <map>
#include <vector>

namespace graph_canon {

struct always_true {

	template<typename ...T>
	bool operator()(T&&...) const {
		return true;
	}
};

struct always_false {

	template<typename ...T>
	bool operator()(T&&...) const {
		return false;
	}
};

template<typename Prop>
struct property_less {

	property_less(Prop prop) : prop(prop) { }

	template<typename Key>
	bool operator()(const Key &lhs, const Key &rhs) const {
		return get(prop, lhs) < get(prop, rhs);
	}
private:
	Prop prop;
};

template<typename Prop>
property_less<Prop> make_property_less(Prop &&prop) {
	return property_less<Prop>(std::forward<Prop>(prop));
}

template<typename SizeType, typename Prop>
struct edge_counter_int_vector {
	typedef edge_counter_int_vector<SizeType, Prop> Self;

	edge_counter_int_vector(Prop prop, std::size_t max) : prop(prop), count(max) { }

	friend void clear(Self &c) {
		std::fill(c.count.begin(), c.count.end(), 0);
	}

	template<typename State>
	friend void add_edge(Self &c, const State &state, const typename State::Edge &e) {
		std::size_t p = get(c.prop, e);
		c.count[p]++;
	}

	friend bool operator<(const Self &lhs, const Self &rhs) {
		return lhs.count < rhs.count;
	}

	friend bool operator==(const Self &lhs, const Self &rhs) {
		return lhs.count == rhs.count;
	}

	//	template<typename Key>
	//	long long operator()(const Key &lhs, const Key &rhs) const {
	//		return get(prop, lhs) - get(prop, rhs);
	//	}
private:
	Prop prop;
	std::vector<SizeType> count;
};

template<typename SizeType, typename Prop>
edge_counter_int_vector<SizeType, Prop> make_edge_counter_int_vector(Prop &&prop, std::size_t max) {
	return edge_counter_int_vector<SizeType, Prop>(std::forward<Prop>(prop), max);
}

} // namespace graph_canon

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

struct graph_equal_reporter {

	void num_vertices_error() const { }

	template<typename Vertex, typename IdxMap>
	void vertex_comp_error(Vertex, Vertex, const IdxMap&, const IdxMap&) const { }

	template<typename Vertex, typename IdxMap>
	void out_degree_error(Vertex, Vertex, const IdxMap&, const IdxMap&) const { }

	template<typename Edge, typename IdxMap>
	void out_edge_error(Edge, Edge, const IdxMap&, const IdxMap&) const { }

	template<typename Edge, typename IdxMap>
	void edge_comp_error(Edge, Edge, const IdxMap&, const IdxMap&) const { }
};

namespace detail {

template<typename Graph, typename IdxMap, typename EdgeComp, typename Reporter>
struct graoh_equal_edge {
	typedef typename boost::graph_traits<Graph>::edge_descriptor Edge;

	graoh_equal_edge(const Graph &g1, const Graph &g2, const IdxMap &idx1, const IdxMap &idx2, EdgeComp edgeComp, Reporter reporter)
	: g1(g1), g2(g2), idx1(idx1), idx2(idx2), edgeComp(edgeComp), reporter(reporter) { }

	bool operator()(Edge e1, Edge e2) {
		if(idx1[target(e1, g1)] != idx2[target(e2, g2)]) {
			reporter.out_edge_error(e1, e2, idx1, idx2);
			return false;
		}
		if(!edgeComp(e1, e2)) {
			reporter.edge_comp_error(e1, e2, idx1, idx2);
			return false;
		}
		return true;
	}
public:
	const Graph &g1;
	const Graph &g2;
	const IdxMap &idx1;
	const IdxMap &idx2;
	EdgeComp edgeComp;
	Reporter reporter;
};

template<typename Graph, typename IdxMap, typename VertexComp, typename EdgeComp, typename Reporter>
struct graph_equal_vertex {
	typedef typename boost::graph_traits<Graph>::vertex_descriptor Vertex;
	typedef typename boost::graph_traits<Graph>::edge_descriptor Edge;

	struct edge_less {

		edge_less(const Graph &graph, const IdxMap &idx) : graph(graph), idx(idx) { }

		bool operator()(Edge e1, Edge e2) const {
			return idx[target(e1, graph)] < idx[target(e2, graph)];
		}
	public:
		const Graph &graph;
		const IdxMap &idx;
	};

	graph_equal_vertex(const Graph &g1, const Graph &g2, const IdxMap &idx1, const IdxMap &idx2,
			VertexComp vertexComp, EdgeComp edgeComp, Reporter reporter)
	: g1(g1), g2(g2), idx1(idx1), idx2(idx2), vertexComp(vertexComp), edgeComp(edgeComp), reporter(reporter) { }

	bool operator()(Vertex v1, Vertex v2) {
		if(!vertexComp(v1, v2)) {
			reporter.out_degree_error(v1, v2, idx1, idx2);
			return false;
		}
		// TODO: compare vertex properties by some functor

		// sort out-edges according to the order we have established
		std::vector<Edge> e1, e2;

		BGL_FORALL_OUTEDGES_T(v1, e, g1, Graph) {
			e1.push_back(e);
		}

		BGL_FORALL_OUTEDGES_T(v2, e, g2, Graph) {
			e2.push_back(e);
		}
		if(e1.size() != e2.size()) {
			reporter.out_degree_error(v1, v2, idx1, idx2);
			return false;
		}
		std::sort(e1.begin(), e1.end(), edge_less(g1, idx1));
		std::sort(e2.begin(), e2.end(), edge_less(g2, idx2));
		return std::equal(e1.begin(), e1.end(), e2.begin(), graoh_equal_edge<Graph, IdxMap, EdgeComp, Reporter>(g1, g2, idx1, idx2, edgeComp, reporter));
	}
public:
	const Graph &g1;
	const Graph &g2;
	const IdxMap &idx1;
	const IdxMap &idx2;
	VertexComp vertexComp;
	EdgeComp edgeComp;
	Reporter reporter;
};

} // namespace detail

template<typename Graph, typename VertexComp, typename EdgeComp, typename Reporter>
bool graph_equal(const Graph &g1, const Graph &g2, VertexComp vertexComp, EdgeComp edgeComp, Reporter reporter) {
	typedef typename boost::graph_traits<Graph>::vertex_iterator VIter;
	typedef typename boost::graph_traits<Graph>::vertex_descriptor Vertex;
	typedef std::map<Vertex, std::size_t> IdxMapMap;
	typedef boost::const_associative_property_map<IdxMapMap> IdxMap;

	if(num_vertices(g1) != num_vertices(g2)) {
		reporter.num_vertices_error();
		return false;
	}
	// establish an order based on the iteration order
	IdxMapMap idx1map, idx2map;
	std::size_t i = 0;
	for(VIter vi = vertices(g1).first; vi != vertices(g1).second; vi++) idx1map[*vi] = i++;
	i = 0;
	for(VIter vi = vertices(g2).first; vi != vertices(g2).second; vi++) idx2map[*vi] = i++;
	IdxMap idx1(idx1map), idx2(idx2map);
	// now compare the graphs
	return std::equal(vertices(g1).first, vertices(g1).second, vertices(g2).first,
			detail::graph_equal_vertex<Graph, IdxMap, VertexComp, EdgeComp, Reporter>(g1, g2, idx1, idx2, vertexComp, edgeComp, reporter));
}

template<typename Graph>
bool graph_equal(const Graph &g1, const Graph &g2) {
	return graph_equal(g1, g2, graph_canon::always_true(), graph_canon::always_true(), graph_equal_reporter());
}

#endif /* GRAPH_CANON_UTIL_HPP */