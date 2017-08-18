#include "graph_canon_util.hpp"
#include "graph_canon/visitor/stats.hpp"

#include <boost/type_traits/is_same.hpp>

struct BenchmarkOptions : Options {

	void from(const po::variables_map &vm) {
		Options::from(vm);
	}

	std::ostream &printHeader(std::ostream &s) const {
		return Options::printHeader(s);
	}

	std::ostream &printValues(std::ostream &s) const {
		return Options::printValues(s);
	}
public:
	std::size_t time;
};

struct ModeBenchmark {

	template<typename Graph, typename VertexLess, typename EdgeHandler, typename Visitor>
	void canonicalize(const Options &options, std::size_t round, const Graph &g, VertexLess vLess, EdgeHandler edgeHandler, Visitor visitor) {
		canonicalize_switch_alg(options, g, vLess, edgeHandler, visitor);
	}

	template<typename Graph>
	void execute(BenchmarkOptions &options, const Graph &g) {
		Options::Clock::duration time(0);
		std::vector<std::size_t> id_permutation(num_vertices(g));
		for(std::size_t i = 0; i < num_vertices(g); i++) id_permutation[i] = i;
		options.printHeader(std::cout) << "	max-nodes	nodes	n	m	round	time (ms)" << std::endl;
		std::stringstream sPrefix;
		options.printValues(sPrefix);
		std::string prefix = sPrefix.str();
		for(std::size_t i = 1;
				i <= options.rounds
				&& boost::chrono::duration_cast<boost::chrono::seconds>(time).count() <= options.time;
				i++) {
			std::vector<std::size_t> permutation = make_random_permutation(options.gen, id_permutation);
			Graph g_permuted;
			permute_graph(g, g_permuted, permutation);
			auto stats = graph_canon::stats_visitor();
			Options::Clock::time_point start = Options::Clock::now();
			canonicalize(options, i, g_permuted,
					graph_canon::make_property_less(get(boost::vertex_name_t(), g_permuted)),
#ifdef GRAPH_CANON_EDGE_LABELS
					graph_canon::make_edge_counter_int_vector<std::size_t>(get(boost::edge_name_t(), g_permuted), options.eLabelMax),
#else
					graph_canon::edge_handler_all_equal(),
#endif
					std::ref(stats)
					);
			Options::Clock::duration dur = Options::Clock::now() - start;
			std::cout << prefix << "\t" << stats.max_num_tree_nodes << "\t" << stats.num_tree_nodes << "\t" << num_vertices(g) << "\t" << num_edges(g) << "\t" << i << "\t" << boost::chrono::duration_cast<boost::chrono::milliseconds>(dur).count() << std::endl;
			time += dur;
		}
	}
};

int main(int argc, char **argv) {
	std::string modeDesc =
			"Benchmark mode: perform at least <permutations> canonicalizations, "
			"and continue until at least <time> seconds has been used on canonicalization.";
	BenchmarkOptions options;
	po::options_description optionsDesc("Options for 'benchmark' mode");
	optionsDesc.add_options()
			("time,t", po::value<std::size_t>(&options.time)->default_value(60), "Minimum time (in seconds) spend on canonicalization.")
			;
	return common_main<ModeBenchmark>(argc, argv, options, optionsDesc, modeDesc);
}
