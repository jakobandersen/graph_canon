#include "util.hpp"

#include <graph_canon/dimacs_graph_io.hpp>
#include <graph_canon/util.hpp>

#include <boost/graph/adjacency_list.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

#include <fstream>
#include <iostream>
#include <random>

namespace po = boost::program_options;

struct Options {
	std::string file;
	std::mt19937 gen;
	std::size_t seed;
	bool permute = false;
};

// rst: .. cpp:namespace:: graph_canon
// rst:
// rst: ``dimacs``
// rst: ####################################################
// rst:
// rst: .. program:: dimacs
// rst:
// rst: A program for checking the syntax of a graph in DIMACS format
// rst: (see :cpp:func:`read_dimacs_graph`),
// rst: and optionally print a permuted version of it.
// rst:

int main(int argc, char **argv) {
	Options options;
	po::options_description optionDesc("Options");
	optionDesc.add_options()
			// rst: .. option:: -h, --help
			// rst:
			// rst:		Print help message.
			("help,h", "Print help message.")
			// rst: .. option:: -f <file>, --file <file>
			// rst:
			// rst:		Filename with graph to read. Use ``-`` to read form stdin.
			("file,f", po::value<std::string>(&options.file)->required(), "Filename with graph to read. Use '-' to read from stdin.")
			// rst: .. option:: -s <seed>, --seed <seed>
			// rst:
			// rst:		Seed for random number generator. A default seed from :cpp:expr:`std::random_device` will be used if not specified.
			("seed,s", po::value<std::size_t>(&options.seed)->default_value(-1),
			"Seed for random number generator. A default seed from std::random_device will be used if not specified.")
			// rst: .. option:: --permute
			// rst:
			// rst:		Make a random permutation of the graph and print it to stdout.
			("permute", "Make a random permutation of the graph and print it to stdout.")
			;

	po::variables_map rawOptions;
	try {
		po::store(po::command_line_parser(argc, argv).options(optionDesc).allow_unregistered().run(), rawOptions);
	} catch(const po::error &e) {
		std::cout << e.what() << '\n';
		std::exit(1);
	}
	if(rawOptions.count("help")) {
		std::cout << "Load a graph in DIMACS format.\n";
		std::cout << optionDesc << "\n";
		return 0;
	}
	if(rawOptions.count("permute"))
		options.permute = true;
	if(options.seed == -1) {
		std::random_device dev;
		std::uniform_int_distribution<std::size_t> dist;
		options.seed = dist(dev);
	}
	options.gen.seed(options.seed);
	po::notify(rawOptions);

	using Graph = boost::adjacency_list < boost::vecS, boost::vecS, boost::undirectedS,
			boost::property<boost::vertex_name_t, std::size_t>,
			boost::property<boost::edge_name_t, std::size_t> >;
	const auto parHandler = [](unsigned int src, unsigned int tar) {
		return true;
	};
	const auto loopHandler = [](unsigned int v) {
		return true;
	};
	Graph g;
	bool res;
	if(options.file == "-") {
		res = graph_canon::read_dimacs_graph(std::cin, g, std::cerr, parHandler, loopHandler);
	} else {
		std::ifstream ifs(options.file);
		if(!ifs) {
			std::cerr << "Could not open file '" << options.file << "'.\n";
			std::exit(1);
		}
		res = graph_canon::read_dimacs_graph(ifs, g, std::cerr, parHandler, loopHandler);
	}
	if(!res) {
		std::cerr << "Could not parse input.\n";
		std::exit(1);
	}
	if(options.permute) {
		std::vector<std::size_t> id_permutation(num_vertices(g));
		for(std::size_t i = 0; i < num_vertices(g); i++) id_permutation[i] = i;
		std::vector<std::size_t> permutation = make_random_permutation(options.gen, id_permutation);
		Graph g_permuted;
		graph_canon::permute_graph(g, g_permuted, permutation);
		graph_canon::write_dimacs_graph(std::cout, g_permuted);
	}
	return 0;
}