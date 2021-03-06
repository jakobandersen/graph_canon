#include "util.hpp" // from bin/
#include "graph.hpp"

#include <graph_canon/ordered_graph.hpp>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_concepts.hpp>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <random>

template<typename OrdGraph>
void check_other_api(const OrdGraph &gOrd) {
	const typename OrdGraph::Graph &g = gOrd.get_graph();
	(void) g;
	typename OrdGraph::IndexMap idx = gOrd.get_index_map();
	(void) idx;
}

template<typename OrdGraph, typename IndexMap, typename EdgeLess>
void check_incidence(const OrdGraph &g, IndexMap idx, EdgeLess edge_less) {
	check_other_api(g);
	BOOST_CONCEPT_ASSERT((boost::VertexListGraphConcept<OrdGraph>));
	BOOST_CONCEPT_ASSERT((boost::RandomAccessIteratorConcept<typename boost::graph_traits<OrdGraph>::vertex_iterator>));
	BOOST_CONCEPT_ASSERT((boost::IncidenceGraphConcept<OrdGraph>));
	BOOST_CONCEPT_ASSERT(
			(boost::RandomAccessIteratorConcept<typename boost::graph_traits<OrdGraph>::out_edge_iterator>));
	BOOST_CONCEPT_ASSERT((boost::AdjacencyGraphConcept<OrdGraph>));
	BOOST_CONCEPT_ASSERT(
			(boost::RandomAccessIteratorConcept<typename boost::graph_traits<OrdGraph>::adjacency_iterator>));
	if(num_vertices(g) <= 1) return;
	typename boost::graph_traits<OrdGraph>::vertex_iterator vIterPrev, vIter, vIterEnd;
	boost::tie(vIter, vIterEnd) = vertices(g);
	vIterPrev = vIter;
	++vIter;
	for(; vIter != vIterEnd; ++vIter) {
		BOOST_REQUIRE(get(idx, *vIterPrev) < get(idx, *vIter));
		vIterPrev = vIter;
	}
	for(boost::tie(vIter, vIterEnd) = vertices(g); vIter != vIterEnd; ++vIter) {
		if(out_degree(*vIter, g) <= 1) continue;
		typename boost::graph_traits<OrdGraph>::out_edge_iterator eIterPrev, eIter, eIterEnd;
		boost::tie(eIter, eIterEnd) = out_edges(*vIter, g);
		eIterPrev = eIter;
		++eIter;
		typename boost::graph_traits<OrdGraph>::adjacency_iterator adjIter, adjIterEnd;
		boost::tie(adjIter, adjIterEnd) = adjacent_vertices(*vIter, g);
		BOOST_REQUIRE(adjIter != adjIterEnd);
		BOOST_CHECK(target(*eIterPrev, g) == *adjIter);
		++adjIter;
		for(; eIter != eIterEnd; ++eIter) {
			BOOST_REQUIRE(adjIter != adjIterEnd);
			BOOST_REQUIRE(target(*eIter, g) == *adjIter);
			BOOST_REQUIRE(get(idx, target(*eIterPrev, g)) <= get(idx, target(*eIter, g)));
			if(get(idx, target(*eIterPrev, g)) == get(idx, target(*eIter, g))) {
				BOOST_REQUIRE(!edge_less(*eIter, *eIterPrev));
			}
			eIterPrev = eIter;
			++adjIter;
		}
		BOOST_REQUIRE(adjIter == adjIterEnd);
	}
}

template<typename OrdGraph, typename IndexMap, typename EdgeLess>
void check_bidirectional(const OrdGraph &g, IndexMap idx, EdgeLess edge_less) {
	check_incidence(g, idx, edge_less);
	BOOST_CONCEPT_ASSERT((boost::BidirectionalGraphConcept<OrdGraph>));
	if(false) { // missing stuff from BidirectionalGraphConcept
		typename boost::graph_traits<OrdGraph>::vertex_descriptor v;
		typename boost::graph_traits<OrdGraph>::degree_size_type n = 0;
		n += degree(v, g);
		// non-const
		OrdGraph &gNonConst = const_cast<OrdGraph &> (g);
		n = degree(v, gNonConst);
	}
	BOOST_CONCEPT_ASSERT((boost::RandomAccessIteratorConcept<typename boost::graph_traits<OrdGraph>::in_edge_iterator>));
	if(false) { // 'AdjacencyBidirectionalGraphConcept'
		typedef typename /*boost::graph_traits<*/OrdGraph/*>*/::inv_adjacency_iterator inv_adjacency_iterator;
		// TODO: typedef traversal_category
		BOOST_CONCEPT_ASSERT((boost::MultiPassInputIteratorConcept<inv_adjacency_iterator>));
		// TODO: BOOST_CONCEPT_ASSERT((Convertible<traversal_category, adjacency_graph_tag>));

		BOOST_STATIC_ASSERT((boost::mpl::not_<boost::is_same<inv_adjacency_iterator, void> >::value));
		typename boost::graph_traits<OrdGraph>::vertex_descriptor v;
		std::pair<inv_adjacency_iterator, inv_adjacency_iterator> p;
		p = inv_adjacent_vertices(v, g);
		v = *p.first;
		// non-const
		OrdGraph &gNonConst = const_cast<OrdGraph &> (g);
		p = inv_adjacent_vertices(v, gNonConst);
	}
	BOOST_CONCEPT_ASSERT(
			(boost::RandomAccessIteratorConcept<typename /*boost::graph_traits<*/OrdGraph/*>*/::inv_adjacency_iterator>));
	if(num_vertices(g) <= 1) return;
	typename boost::graph_traits<OrdGraph>::vertex_iterator vIter, vIterEnd;
	for(boost::tie(vIter, vIterEnd) = vertices(g); vIter != vIterEnd; ++vIter) {
		if(in_degree(*vIter, g) <= 1) continue;
		typename boost::graph_traits<OrdGraph>::in_edge_iterator eIterPrev, eIter, eIterEnd;
		boost::tie(eIter, eIterEnd) = in_edges(*vIter, g);
		eIterPrev = eIter;
		++eIter;
		typename /*boost::graph_traits<*/OrdGraph/*>*/::inv_adjacency_iterator adjIter, adjIterEnd;
		boost::tie(adjIter, adjIterEnd) = inv_adjacent_vertices(*vIter, g);
		BOOST_REQUIRE(adjIter != adjIterEnd);
		BOOST_REQUIRE(source(*eIterPrev, g) == *adjIter);
		++adjIter;
		for(; eIter != eIterEnd; ++eIter) {
			BOOST_REQUIRE(adjIter != adjIterEnd);
			BOOST_REQUIRE(source(*eIter, g) == *adjIter);
			BOOST_REQUIRE(get(idx, source(*eIterPrev, g)) <= get(idx, source(*eIter, g)));
			if(get(idx, source(*eIterPrev, g)) == get(idx, source(*eIter, g))) {
				BOOST_REQUIRE(!edge_less(*eIter, *eIterPrev));
			}
			eIterPrev = eIter;
			++adjIter;
		}
		BOOST_REQUIRE(adjIter == adjIterEnd);
	}
}

template<typename Graph, typename Gen>
void randomize_edge_name(Graph &g, Gen &gen, std::size_t max_edge_name) {
	std::uniform_int_distribution<std::size_t> dist(0, max_edge_name);

	BGL_FORALL_EDGES_T(e, g, Graph) {
			put(boost::edge_name_t(), g, e, dist(gen));
		}
}

template<typename Graph>
struct edge_less {
	using edge_descriptor = typename boost::graph_traits<Graph>::edge_descriptor;

	edge_less(const Graph &g) : g(g) {}

	bool operator()(const edge_descriptor &lhs, const edge_descriptor &rhs) const {
		return get(boost::edge_name_t(), g.g, lhs.e) < get(boost::edge_name_t(), g.g, rhs.e);
	}

private:
	const Graph &g;
};

template<typename Idx>
struct PermutedIndexMap {
	PermutedIndexMap(Idx idx, const std::vector<std::size_t> *v) : idx(idx), v(v) {}

	template<typename V>
	friend std::size_t get(PermutedIndexMap self, const V &v) {
		return (*self.v)[get(self.idx, v)];
	}

private:
	Idx idx;
	const std::vector<std::size_t> *v;
};

template<typename Idx>
struct boost::property_traits<PermutedIndexMap<Idx>> {
	using key_type = typename ::graph<
			typename Idx::directed_tag,
			typename Idx::vertex_prop,
			typename Idx::edge_prop>::vertex_descriptor;
	using value_type = int;
	using reference = value_type;
	using category = boost::readable_property_map_tag;
};

BOOST_AUTO_TEST_CASE(test_main) {
	const std::size_t num_vertices = 100;
	const double edge_probability = 0.5;
	const std::size_t max_parallel_edges = 3;
	const double parallel_edge_probability = 0.3;
	const std::size_t max_edge_name = 5;
	const std::size_t seed = std::random_device()();
	std::cout << "Seed: " << seed << std::endl;
	std::mt19937 gen(seed);

	using IdxStore = std::vector<std::size_t>;
	IdxStore idx;
	idx.reserve(num_vertices);
	for(std::size_t i = 0; i < num_vertices; ++i) idx.push_back(i);
	idx = make_random_permutation(gen, idx);

#define PROPS boost::no_property, boost::property<boost::edge_name_t, std::size_t>
	{ // undirected
		using Graph = graph<boost::undirectedS, PROPS >;
		using Idx = ::Idx<boost::undirectedS, PROPS >;
		using IdxMap = PermutedIndexMap<Idx>;
		BOOST_CONCEPT_ASSERT((boost::BidirectionalGraphConcept<Graph>));
		Graph g = [&]() {
			Graph::Underlying gg;
			make_random_graph(gg, gen, edge_probability, max_parallel_edges, parallel_edge_probability);
			randomize_edge_name(gg, gen, max_edge_name);
			return Graph(std::move(gg));
		}();
		edge_less<Graph> less(g);
		IdxMap permutedIdx(Idx(), &idx);
		{ // without in-edges
			using OrdGraph = graph_canon::ordered_graph<Graph, IdxMap>;
			OrdGraph gOrd(g, permutedIdx, less);
			check_incidence(gOrd, permutedIdx, less);
		}
		{ // with in-edges
			using OrdGraph = graph_canon::ordered_graph<Graph, IdxMap, true>;
			OrdGraph gOrd(g, permutedIdx, less);
			check_bidirectional(gOrd, permutedIdx, less);
		}
	}
	{ // directed
		using Graph = graph<boost::directedS, PROPS >;
		using Idx = ::Idx<boost::directedS, PROPS >;
		using IdxMap = PermutedIndexMap<Idx>;
		Graph g = [&]() {
			Graph::Underlying gg;
			make_random_graph(gg, gen, edge_probability, max_parallel_edges, parallel_edge_probability);
			randomize_edge_name(gg, gen, max_edge_name);
			return Graph(std::move(gg));
		}();
		edge_less<Graph> less(g);
		IdxMap permutedIdx(Idx(), &idx);
		using OrdGraph = graph_canon::ordered_graph<Graph, IdxMap>;
		OrdGraph gOrd(g, permutedIdx, less);
		check_incidence(gOrd, permutedIdx, less);
	}
	{ // bidirectional
		using Graph = ::graph<boost::bidirectionalS, PROPS >;
		using Idx = ::Idx<boost::bidirectionalS, PROPS >;
		using IdxMap = PermutedIndexMap<Idx>;
		Graph g = [&]() {
			Graph::Underlying gg;
			make_random_graph(gg, gen, edge_probability, max_parallel_edges, parallel_edge_probability);
			randomize_edge_name(gg, gen, max_edge_name);
			return Graph(std::move(gg));
		}();
		edge_less<Graph> less(g);
		IdxMap permutedIdx(Idx(), &idx);
		using OrdGraph = graph_canon::ordered_graph<Graph, IdxMap, true>;
		OrdGraph gOrd(g, permutedIdx, less);
		check_bidirectional(gOrd, permutedIdx, less);
	}
}
