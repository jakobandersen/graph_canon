#include "graph.hpp"

#include <graph_canon/shorthands.hpp>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>


template<typename DirectedS>
void doIt() {
	graph<DirectedS> g;
	graph_canon::canonicalize<int, false, false>(
			g, Idx<DirectedS>(), graph_canon::always_false(),
			graph_canon::edge_handler_all_equal(),
			graph_canon::make_default_visitor());
}

BOOST_AUTO_TEST_CASE(test_main) {
	doIt<boost::undirectedS>();
	//	doIt<boost::bidirectionalS>();
}