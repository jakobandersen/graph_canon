#ifndef BOOST_GRAPH_CANON_TEST_UTIL_HPP
#define BOOST_GRAPH_CANON_TEST_UTIL_HPP

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/properties.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>

#include <cassert>
#include <iostream>
#include <vector>

template <typename Generator>
struct random_functor {

	random_functor(Generator &g) : g(g) { }

	std::size_t operator()(std::size_t n) {
		boost::random::uniform_int_distribution<std::size_t> distrib(0, n - 1);
		return distrib(g);
	}
private:
	Generator &g;
};

template<typename Gen, typename T>
std::vector<T> make_random_permutation(Gen &gen, std::vector<T> seq) {
	random_functor<Gen> rand_fun(gen);
	std::random_shuffle(seq.begin(), seq.end(), rand_fun);
	return seq;
}

template<typename Graph, typename Gen>
void make_random_graph(Graph &g, Gen &gen, double edge_probability,
		std::size_t max_parallel_edges, double parallel_edge_probability) {
	assert((0 <= edge_probability) && (edge_probability <= 1));
	assert((0 <= parallel_edge_probability) && (parallel_edge_probability <= 1));

	auto dist_real = boost::random::uniform_real_distribution<double>(0.0, 1.0);

	const auto vs = vertices(g);
	for(auto u = vs.first; u != vs.second; ++u) {
		for(auto v = vs.first; v != vs.second; ++v) {
			if(dist_real(gen) <= edge_probability) {
				add_edge(*u, *v, g);
				for(std::size_t i = 1; i < max_parallel_edges; ++i) {
					if(dist_real(gen) <= parallel_edge_probability)
						add_edge(*u, *v, g);
				}
			}
		}
	}
}


// Print Graph
//------------------------------------------------------------------------------

template <typename Graph, typename VertexPrinter, typename OutEdgePrinter>
void printGraph(const Graph &g, VertexPrinter vPrinter, OutEdgePrinter eOutPrinter) {
	const auto vs = vertices(g);
	for(auto iter = vs.first; iter != vs.second; ++iter) {
		const auto v = *iter;
		vPrinter(std::cout, v, g);
		if(boost::is_directed(g))
			std::cout << " --> ";
		else std::cout << " <--> ";
		const auto oes = out_edges(v, g);
		for(auto e_iter = oes.first; e_iter != oes.second; ++e_iter) {
			const auto e = *e_iter;
			eOutPrinter(std::cout, e, g);
			std::cout << " ";
		}
		std::cout << std::endl;
	}
}

// Print Util
//------------------------------------------------------------------------------

template<typename Idx>
struct VertexPrinter {

	VertexPrinter(Idx idx, bool withVertexName) : idx(idx), withVertexName(withVertexName) { }

	template<typename Graph>
	void operator()(std::ostream &s, typename boost::graph_traits<Graph>::vertex_descriptor v, const Graph &g) const {
		s << get(idx, v);
		if(withVertexName) s << "(" << get(boost::vertex_name_t(), g, v) << ")";
	}
private:
	Idx idx;
	bool withVertexName;
};

template<typename Idx>
VertexPrinter<Idx> makeVertexPrinter(Idx idx, bool withVertexName) {
	return VertexPrinter<Idx>(idx, withVertexName);
}

template<typename Idx>
struct EdgePrinter {

	EdgePrinter(Idx idx, bool withEdgeName) : idx(idx), withEdgeName(withEdgeName) { }

	template<typename Graph>
	void operator()(std::ostream &s, typename boost::graph_traits<Graph>::edge_descriptor e, const Graph &g) const {
		s << get(idx, target(e, g));
		if(withEdgeName) s << "(" << get(boost::edge_name_t(), g, e) << ")";
	}
private:
	Idx idx;
	bool withEdgeName;
};

template<typename Idx>
EdgePrinter<Idx> makeEdgePrinter(Idx idx, bool withEdgeName) {
	return EdgePrinter<Idx>(idx, withEdgeName);
}

//------------------------------------------------------------------------------

template<typename Idx>
struct OrderedVertexPrinter {

	OrderedVertexPrinter(Idx idx, bool withVertexName) : idx(idx), withVertexName(withVertexName) { }

	template<typename Graph>
	void operator()(std::ostream &s, typename boost::graph_traits<Graph>::vertex_descriptor v, const Graph &g) const {
		s << get(idx, v);
		if(withVertexName) s << "(" << get(boost::vertex_name_t(), g.data.g, v) << ")";
	}
private:
	Idx idx;
	bool withVertexName;
};

template<typename Idx>
OrderedVertexPrinter<Idx> makeOrderedVertexPrinter(Idx idx, bool withVertexName) {
	return OrderedVertexPrinter<Idx>(idx, withVertexName);
}

template<typename Idx>
struct OrderedEdgePrinter {

	OrderedEdgePrinter(Idx idx, bool withEdgeName) : idx(idx), withEdgeName(withEdgeName) { }

	template<typename Graph>
	void operator()(std::ostream &s, typename boost::graph_traits<Graph>::edge_descriptor e, const Graph &g) const {
		s << get(idx, target(e, g));
		if(withEdgeName) s << "(" << get(boost::edge_name_t(), g.data.g, e) << ")";
	}
private:
	Idx idx;
	bool withEdgeName;
};

template<typename Idx>
OrderedEdgePrinter<Idx> makeOrderedEdgePrinter(Idx idx, bool withEdgeName) {
	return OrderedEdgePrinter<Idx>(idx, withEdgeName);
}

#endif /* BOOST_GRAPH_CANON_TEST_UTIL_HPP */