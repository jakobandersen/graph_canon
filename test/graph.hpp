#ifndef GRAPHCANON_TEST_GRAPH_HPP
#define GRAPHCANON_TEST_GRAPH_HPP

#include <boost/graph/adjacency_list.hpp>

template<typename DirectedS, typename VertexProp = boost::no_property, typename EdgeProp = boost::no_property>
struct graph {
	using Underlying = boost::adjacency_list<boost::vecS, boost::vecS, DirectedS, VertexProp, EdgeProp>;
public: // Graph

	struct vertex_descriptor {
		vertex_descriptor() = default;

		vertex_descriptor(typename Underlying::vertex_descriptor v) : v(v) {}

		friend bool operator==(const vertex_descriptor &a, const vertex_descriptor &b) {
			return a.v == b.v;
		}

		friend bool operator!=(const vertex_descriptor &a, const vertex_descriptor &b) {
			return a.v != b.v;
		}

	public:
		typename Underlying::vertex_descriptor v;
	};

	struct edge_descriptor {
		edge_descriptor() = default;

		edge_descriptor(typename Underlying::edge_descriptor e) : e(e) {}

		friend bool operator==(const edge_descriptor &a, const edge_descriptor &b) {
			return a.e == b.e;
		}

		friend bool operator!=(const edge_descriptor &a, const edge_descriptor &b) {
			return a.e != b.e;
		}

	public:
		typename Underlying::edge_descriptor e;
	};

	using directed_category = typename Underlying::directed_category;
	using edge_parallel_category = typename Underlying::edge_parallel_category;
	using traversal_category = typename Underlying::traversal_category;
	// TODO: null_vertex
public: // VertexList
	struct vertex_iterator
			: boost::iterator_adaptor<vertex_iterator, typename Underlying::vertex_iterator,
					vertex_descriptor, boost::use_default, vertex_descriptor> {
		vertex_iterator(typename Underlying::vertex_iterator iter) : iter(iter) {}

	public:
		typename Underlying::vertex_iterator iter;
	};

	using vertices_size_type = typename Underlying::vertices_size_type;

	friend std::pair<vertex_iterator, vertex_iterator> vertices(const graph &g) {
		return vertices(g.g);
	}

	friend decltype(auto) num_vertices(const graph &g) {
		return num_vertices(g.g);
	}

public: // EdgeList
	struct edge_iterator
			: boost::iterator_adaptor<edge_iterator, typename Underlying::edge_iterator,
					edge_descriptor, boost::use_default, edge_descriptor> {
		edge_iterator(typename Underlying::edge_iterator iter) : iter(iter) {}

	public:
		typename Underlying::edge_iterator iter;
	};

	using edges_size_type = typename Underlying::edges_size_type;

	friend std::pair<edge_iterator, edge_iterator> edges(const graph &g) {
		return edges(g.g);
	}

	friend decltype(auto) num_edges(const graph &g) {
		return num_edges(g.g);
	}

	friend vertex_descriptor source(const edge_descriptor &e, const graph &g) {
		return source(e.e, g.g);
	}

	friend vertex_descriptor target(const edge_descriptor &e, const graph &g) {
		return target(e.e, g.g);
	}

public: // Incidence
	struct out_edge_iterator
			: boost::iterator_adaptor<out_edge_iterator, typename Underlying::out_edge_iterator,
					edge_descriptor, boost::use_default, edge_descriptor> {
		out_edge_iterator(typename Underlying::out_edge_iterator iter) : iter(iter) {}

	public:
		typename Underlying::out_edge_iterator iter;
	};

	using degree_size_type = typename Underlying::degree_size_type;

	friend std::pair<out_edge_iterator, out_edge_iterator> out_edges(const vertex_descriptor &v, const graph &g) {
		return out_edges(v.v, g.g);
	}

	friend decltype(auto) out_degree(const vertex_descriptor &v, const graph &g) {
		return out_degree(v.v, g.g);
	}

public: // Bidirectional
	struct in_edge_iterator
			: boost::iterator_adaptor<out_edge_iterator, typename Underlying::in_edge_iterator,
					edge_descriptor, boost::use_default, edge_descriptor> {
		in_edge_iterator(typename Underlying::in_edge_iterator iter) : iter(iter) {}

	public:
		typename Underlying::in_edge_iterator iter;
	};

	friend std::pair<in_edge_iterator, in_edge_iterator> in_edges(const vertex_descriptor &v, const graph &g) {
		return in_edges(v.v, g.g);
	}

	friend decltype(auto) in_degree(const vertex_descriptor &v, const graph &g) {
		return in_degree(v.v, g.g);
	}

	friend decltype(auto) degree(const vertex_descriptor &v, const graph &g) {
		return degree(v.v, g.g);
	}

public: // TODO: this should not be required
	friend vertex_descriptor vertex(vertices_size_type i, const graph &g) {
		return vertex(i, g.g);
	}

public:
	graph() = default;

	explicit graph(Underlying g) : g(std::move(g)) {}

public:
	Underlying g;
};

template<typename DirectedS, typename VertexProp = boost::no_property, typename EdgeProp = boost::no_property>
struct Idx {
	using directed_tag = DirectedS;
	using vertex_prop = VertexProp;
	using edge_prop = EdgeProp;
public:
	friend int get(Idx, typename graph<DirectedS, VertexProp, EdgeProp>::vertex_descriptor v) {
		return v.v;
	}
};

template<typename DirectedS, typename VertexProp, typename EdgeProp>
struct boost::property_traits<Idx<DirectedS, VertexProp, EdgeProp>> {
	using key_type = typename ::graph<DirectedS, VertexProp, EdgeProp>::vertex_descriptor;
	using value_type = int;
	using reference = value_type;
	using category = boost::readable_property_map_tag;
};

#endif //GRAPHCANON_TEST_GRAPH_HPP
