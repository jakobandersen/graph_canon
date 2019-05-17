// In the examples we will load graphs in DIMACS format.
#include <graph_canon/dimacs_graph_io.hpp>
// We want to make a permuted view of the graph so we can iterate in canonical order.
#include <graph_canon/ordered_graph.hpp>
// Let's use a simplified interface for the canonicalization.
#include <graph_canon/shorthands.hpp>
// Provides for example the function object 'always_false' and the function 'as_range'.
#include <graph_canon/util.hpp>
// We want to attach these extra visitors.
#include <graph_canon/visitor/debug.hpp>
#include <graph_canon/visitor/stats.hpp>

#include <boost/graph/adjacency_list.hpp>

#include <fstream>
#include <iostream>

using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
		// The DIMACS reader requires this property.
		// In real uses one probably has some other properties instead.
		boost::property<boost::vertex_name_t, int> >;

namespace gc = graph_canon;
namespace pg = perm_group;

int main(int argc, char **argv) {
	if(argc < 2) {
		std::cerr << "Missing input file.\n";
		std::exit(1);
	}
	std::ifstream ifs(argv[1]);
	if(!ifs) {
		std::cerr << "Could not open file '" << argv[1] << "'.\n";
		std::exit(1);
	}
	Graph g;
	// parse the given file, but error on both parallel edges and loops
	gc::read_dimacs_graph(ifs, g, std::cerr, gc::always_false(), gc::always_false());

	// let's open a few files to get data out of the stats and debug visitors
	std::ofstream fDot("tree.dot"); // print the search tree
	std::ofstream fJson("can.json"); // an event log for later visualization

	// set up our visitor as the composition of multiple visitors
	const auto vis = gc::make_visitor(
			// a bunch of usually good visitors
			gc::make_default_visitor(),
			// the stats visitor, where we here give a pointer to the file for the tree visualization
			gc::stats_visitor(&fDot),
			// the debug visitor, where we can select which log messages we want printed to std::cout,
			// and a file where an event log is printed in json format
			gc::debug_visitor(true, true, true, true, false, &fJson));
	// run the canonicalization with our visitor, ignoring any labels, and still using some defaults,
	auto res = gc::canonicalize<false, false>(g, vis);
	// this should have printed a ton of information to std::cout than one can sift through

	// res is a pair with the first entry being the mapping from input IDs to canonical IDs,
	// but the second entry is auxiliary data from the attached visitors.
	// The stats visitor records statistics and returns them, so let's extract them:
	auto stats = gc::get(gc::stats_visitor::result_t(), res.second);
	// and we can simply print them
	std::cout << "Stats:\n" << stats;

	// tree.dot should now be a Graphviz file with a static depicition of the search tree,
	// while can.json contains information for creating a dynamic visualization
	// using https://jakobandersen.github.io/graph_canon_vis/
}
