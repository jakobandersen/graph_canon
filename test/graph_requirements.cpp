#include "graph.hpp"

#include <graph_canon/shorthands.hpp>
#include <boost/test/minimal.hpp>


template<typename DirectedS>
void doIt() {
	graph<DirectedS> g;
	graph_canon::canonicalize<int, false, false>(
			g, Idx<DirectedS>(), graph_canon::always_false(),
			graph_canon::edge_handler_all_equal(),
			graph_canon::make_default_visitor());
}

int test_main(int argc, char **argv) {
	doIt<boost::undirectedS>();
	//	doIt<boost::bidirectionalS>();
	return 0;
}