// We want to do lexicographical comparison of graphs in canonical order.
#include <graph_canon/compare.hpp>
// In the examples we will load graphs in DIMACS format.
#include <graph_canon/dimacs_graph_io.hpp>
// We want to make a permuted view of the graph so we can iterate in canonical order.
#include <graph_canon/ordered_graph.hpp>
// Let's use a simplified interface for the canonicalization.
#include <graph_canon/shorthands.hpp>
// Provides for example the function object 'always_false' and the function 'as_range'.
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
	if(argc < 3) {
		std::cerr << "Missing input file(s).\n";
		std::exit(1);
	}
	std::ifstream ifs1(argv[1]);
	std::ifstream ifs2(argv[2]);
	if(!ifs1) {
		std::cerr << "Could not open file 1 '" << argv[1] << "'.\n";
		std::exit(1);
	}
	if(!ifs2) {
		std::cerr << "Could not open file 1 '" << argv[2] << "'.\n";
		std::exit(1);
	}
	Graph g1, g2;
	// parse the given files, but error on both parallel edges and loops
	gc::read_dimacs_graph(ifs1, g1, std::cerr, gc::always_false(), gc::always_false());
	gc::read_dimacs_graph(ifs2, g2, std::cerr, gc::always_false(), gc::always_false());
	// run the canonicalization, ignoring any labels, and using many defaults,
	auto res1 = gc::canonicalize<false, false>(g1, gc::make_default_visitor());
	auto res2 = gc::canonicalize<false, false>(g2, gc::make_default_visitor());
	// wrap the result in a formal property map
	const auto idxMap1 = boost::make_iterator_property_map(res1.first.cbegin(), get(boost::vertex_index_t(), g1));
	const auto idxMap2 = boost::make_iterator_property_map(res2.first.cbegin(), get(boost::vertex_index_t(), g2));
	// which we can then use to wrap the graph and make an ordered graph:
	const auto gOrdered1 = gc::make_ordered_graph<false>(g1, idxMap1, gc::always_false());
	const auto gOrdered2 = gc::make_ordered_graph<false>(g2, idxMap2, gc::always_false());
	// with gOrdered1/2 we can now iterate in canonical order, i.e., gOrdered is a canonical representation
	// note: if we were to check isomorphism against many graphs,
	//       we would just keep res, idxMap, and gOrdered around and only perform the next step.

	// we can now compare gOrdered1 and gOrdered2 in lexicographical order
	const bool isomorphic = gc::ordered_graph_equal(
			// the two graphs
			gOrdered1, gOrdered2,
			// vertices are always equal (i.e., no vertex labels)
			gc::always_true(),
			// edges are always equal (i.e., no edge labels)
			gc::always_true(),
			// and we don't care why they are not equal if that is the case
			gc::graph_compare_null_visitor());
	std::cout << "Isomorphic: " << std::boolalpha << isomorphic << "\n";

	// we can also use the canonical order to define a total order on graphs
	const bool smaller = gc::ordered_graph_less(gOrdered1, gOrdered2,
			// VertexLess and EdgeLess: we don't have labels, so no vertex/edge is smaller than another
			gc::always_false(), gc::always_false(),
			// VertexEqual and EdgeEqal: without labels they are always equal
			gc::always_true(), gc::always_true());
	std::cout << "Smaller: " << std::boolalpha << smaller << "\n";
}
