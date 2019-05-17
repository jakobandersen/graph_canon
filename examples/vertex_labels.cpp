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
		// The DIMACS reader requires this property, where vertex labels are stored.
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
	// make a less-than predicate for comparing vertices by their label
	const auto vertex_less_manual = [&g](auto u, auto v) {
		// in this example we have stored the label in the vertex_name property of the graph
		return get(boost::vertex_name_t(), g, u) < get(boost::vertex_name_t(), g, v);
	};
	// but as it really is just a property, we can instead use the provided utility class:
	const auto vertex_less_shorthand = gc::make_property_less(get(boost::vertex_name_t(), g));
	// run the canonicalization, with our vertex predicate
	auto res = gc::canonicalize<false, false>(g, vertex_less_shorthand, gc::make_default_visitor());
	// we can now, for example, print the automorphism group to see that vertex labels are taken into account
	auto autGroupPtr = std::move(gc::get(gc::aut_pruner_basic::result_t(), res.second));
	std::cout << "Aut(G): ";
	pg::write_group(std::cout, *autGroupPtr);
	std::cout << "\n";
}
