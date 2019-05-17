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
	// the result is a mapping from the input IDs to the canonical IDs, so let's invert it,
	const auto canon_order = pg::make_inverse(res.first);
	// so we now have a permutation from the canonical IDs to the input IDs:
	std::cout << "Canonical order is:";
	for(int i = 0; i != num_vertices(g); ++i)
		std::cout << " " << canon_order[i];
	std::cout << "\n";
	// wrap the result in a formal property map
	const auto idxMap = boost::make_iterator_property_map(res.first.cbegin(), get(boost::vertex_index_t(), g));
	// which we can then use to wrap the graph and make an ordered graph:
	const auto gOrdered = gc::make_ordered_graph<false>(g, idxMap, gc::always_false());
	// with gOrdered we can now iterate in canonical order, i.e., gOrdered is a canonical representation

	// but perhaps we want to transform it into a textual format, e.g., DIMACS:
	// note: the write_dimacs function requires an EdgeListGraph, which the ordered_graph is not
	// so we just manually do it
	std::cout << "Canonical DIMACS:\n";
	std::cout << "p edge " << num_vertices(g) << " " << num_edges(g) << '\n';
	for(const auto v : gc::as_range(vertices(gOrdered))) {
		// we need to use our canonical index map, not the underlying one in the original graph
		const auto vIdx = idxMap[v];
		for(const auto vAdj : gc::as_range(adjacent_vertices(v, gOrdered))) {
			const auto vAdjIdx = idxMap[vAdj];
			// it's an undirected graph, so only print each edge once
			if(vAdjIdx < vIdx) continue;
			std::cout << "e " << (vIdx + 1) << " " << (vAdjIdx + 1) << "\n";
		}
	}
}
