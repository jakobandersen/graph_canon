// In the examples we will load graphs in DIMACS format.
#include <graph_canon/dimacs_graph_io.hpp>
// Let's use a simplified interface for the canonicalization.
#include <graph_canon/shorthands.hpp>
// Provides for example the function object 'always_false'.
#include <graph_canon/util.hpp>

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
	// run the canonicalization,
	auto res = gc::canonicalize<false, false>(g, gc::make_default_visitor());
	const auto canon_order = pg::make_inverse(res.first);
	std::cout << "Canonical order is:";
	for(int i = 0; i != num_vertices(g); ++i)
		std::cout << " " << canon_order[i];
	std::cout << "\n";
}
