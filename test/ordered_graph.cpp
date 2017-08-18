#include "../src/util.hpp"

#include <graph_canon/ordered_graph.hpp>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/property_map/property_map_iterator.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/random_device.hpp>

#include <iostream>

template<typename OrdGraph, typename IndexMap, typename EdgeLess>
void check_incidence(const OrdGraph &g, IndexMap idx, EdgeLess edge_less) {
	BOOST_CONCEPT_ASSERT((boost::VertexListGraphConcept<OrdGraph>));
	BOOST_CONCEPT_ASSERT((boost::RandomAccessIteratorConcept<typename boost::graph_traits<OrdGraph>::vertex_iterator>));
	BOOST_CONCEPT_ASSERT((boost::IncidenceGraphConcept<OrdGraph>));
	BOOST_CONCEPT_ASSERT((boost::RandomAccessIteratorConcept<typename boost::graph_traits<OrdGraph>::out_edge_iterator>));
	BOOST_CONCEPT_ASSERT((boost::AdjacencyGraphConcept<OrdGraph>));
	BOOST_CONCEPT_ASSERT((boost::RandomAccessIteratorConcept<typename boost::graph_traits<OrdGraph>::adjacency_iterator>));
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
		OrdGraph &gNonConst = const_cast<OrdGraph&> (g);
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
		OrdGraph &gNonConst = const_cast<OrdGraph&> (g);
		p = inv_adjacent_vertices(v, gNonConst);
	}
	BOOST_CONCEPT_ASSERT((boost::RandomAccessIteratorConcept<typename /*boost::graph_traits<*/OrdGraph/*>*/::inv_adjacency_iterator>));
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
	boost::random::uniform_int_distribution<std::size_t> dist(0, max_edge_name);

	BGL_FORALL_EDGES_T(e, g, Graph) {
		put(boost::edge_name_t(), g, e, dist(gen));
	}
}

template<typename Graph>
struct edge_less {
	typedef typename boost::graph_traits<Graph>::edge_descriptor edge_descriptor;

	edge_less(const Graph &g) : g(g) { }

	bool operator()(const edge_descriptor &lhs, const edge_descriptor &rhs) const {
		return get(boost::edge_name_t(), g, lhs) < get(boost::edge_name_t(), g, rhs);
	}
private:
	const Graph &g;
};

int test_main(int argc, char **argv) {
	const std::size_t num_vertices = 100;
	const double edge_probability = 0.5;
	const std::size_t max_parallel_edges = 3;
	const double parallel_edge_probability = 0.3;
	const std::size_t max_edge_name = 5;
	const std::size_t seed = boost::random::random_device()();
	std::cout << "Seed: " << seed << std::endl;
	boost::mt19937 gen(seed);

	typedef std::vector<std::size_t> IdxStore;
	IdxStore idx;
	idx.reserve(num_vertices);
	for(std::size_t i = 0; i < num_vertices; ++i) idx.push_back(i);
	idx = make_random_permutation(gen, idx);

	{ // undirected
		typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
				boost::no_property, boost::property<boost::edge_name_t, std::size_t> > Graph;
		typedef boost::iterator_property_map<IdxStore::const_iterator, boost::property_map<Graph, boost::vertex_index_t>::type> IdxMap;
		BOOST_CONCEPT_ASSERT((boost::BidirectionalGraphConcept<Graph>));
		Graph g(num_vertices);
		make_random_graph(g, gen, edge_probability, max_parallel_edges, parallel_edge_probability);
		randomize_edge_name(g, gen, max_edge_name);
		edge_less<Graph> less(g);
		IdxMap permutedIdx(idx.begin(), get(boost::vertex_index_t(), g));
		{ // without in-edges
			typedef graph_canon::ordered_graph<Graph, IdxMap> OrdGraph;
			OrdGraph gOrd(g, permutedIdx, less);
			check_incidence(gOrd, permutedIdx, less);
		}
		{ // with in-edges
			typedef graph_canon::ordered_graph<Graph, IdxMap, true> OrdGraph;
			OrdGraph gOrd(g, permutedIdx, less);
			check_bidirectional(gOrd, permutedIdx, less);
		}
	}
	{ // directed
		typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
				boost::no_property, boost::property<boost::edge_name_t, std::size_t> > Graph;
		typedef boost::iterator_property_map<IdxStore::const_iterator, boost::property_map<Graph, boost::vertex_index_t>::type> IdxMap;
		Graph g(num_vertices);
		make_random_graph(g, gen, edge_probability, max_parallel_edges, parallel_edge_probability);
		randomize_edge_name(g, gen, max_edge_name);
		edge_less<Graph> less(g);
		IdxMap permutedIdx(idx.begin(), get(boost::vertex_index_t(), g));
		typedef graph_canon::ordered_graph<Graph, IdxMap> OrdGraph;
		OrdGraph gOrd(g, permutedIdx, less);
		check_incidence(gOrd, permutedIdx, less);
	}
	{ // bidirectional
		typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS,
				boost::no_property, boost::property<boost::edge_name_t, std::size_t> > Graph;
		typedef boost::iterator_property_map<IdxStore::const_iterator, boost::property_map<Graph, boost::vertex_index_t>::type> IdxMap;
		Graph g(num_vertices);
		make_random_graph(g, gen, edge_probability, max_parallel_edges, parallel_edge_probability);
		randomize_edge_name(g, gen, max_edge_name);
		edge_less<Graph> less(g);
		IdxMap permutedIdx(idx.begin(), get(boost::vertex_index_t(), g));
		typedef graph_canon::ordered_graph<Graph, IdxMap, true> OrdGraph;
		OrdGraph gOrd(g, permutedIdx, less);
		check_bidirectional(gOrd, permutedIdx, less);
	}
	return 0;
}