// In the examples we will load graphs in DIMACS format.
#include <graph_canon/dimacs_graph_io.hpp>
// We want to make a permuted view of the graph so we can iterate in canonical order.
#include <graph_canon/ordered_graph.hpp>
// Let's use a simplified interface for the canonicalization.
#include <graph_canon/shorthands.hpp>
// Provides for example the function object 'always_false' and the function 'as_range'.
#include <graph_canon/util.hpp>

#include <perm_group/group/io.hpp>

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
	// run the canonicalization, ignoring any labels, and using many defaults,
	auto res = gc::canonicalize<false, false>(g, gc::make_default_visitor());
	// res is a pair with the first entry being the mapping from input IDs to canonical IDs,
	// but the second entry is auxiliary data from the attached visitors.
	// Through gc::make_default_visitor() we know that aut_pruner_basic is attached,
	// so we can extract the automorphism group found during the canonicalisation:
	// it is a unique_ptr to the group, so we move it out of the result
	auto autGroupPtr = std::move(gc::get(gc::aut_pruner_basic::result_t(), res.second));
	// and we can now print the group as a generating set
	std::cout << "Aut(G): ";
	pg::write_group(std::cout, *autGroupPtr);
	std::cout << "\n";
}
