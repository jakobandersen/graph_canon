#include "graph_canon_util.hpp"

#include <graph_canon/visitor/debug.hpp>
#include <graph_canon/visitor/stats.hpp>

struct TestOptions : Options {

	void from(const po::variables_map &vm) {
		Options::from(vm);
		last = vm.count("last") > 0;
		bool allDebug = vm.count("gall") > 0;
		debugTree = allDebug || vm.count("gtree") > 0;
		debugCanon = allDebug || vm.count("gcanon") > 0;
		debugAut = allDebug || vm.count("gaut") > 0;
		debugRefine = allDebug || vm.count("grefine") > 0;
		debugCompressed = vm.count("gcompressed") > 0;
		stats = vm.count("stats") > 0;
	}

	std::ostream &printHeader(std::ostream &s) const {
		return Options::printHeader(s);
	}

	std::ostream &printValues(std::ostream &s) const {
		return Options::printValues(s);
	}
public:
	bool last;
	bool debugTree, debugCanon, debugAut, debugRefine, debugCompressed;
	std::string graphDot, treeDot;
	bool stats;
};

struct ModeTest {

	template<typename Graph, typename Idx>
	struct PrintVisitor {
		typedef typename boost::graph_traits<Graph>::vertex_descriptor Vertex;
		typedef typename boost::graph_traits<Graph>::edge_descriptor Edge;
	public:

		PrintVisitor(const Graph &g1, const Graph &g2, const Idx &idx1, const Idx &idx2) : g1(g1), g2(g2), idx1(idx1), idx2(idx2) { }

		void at_num_vertices() const {
			std::cout << "num_vertices: " << num_vertices(g1) << " != " << num_vertices(g2) << std::endl;
		}

		void at_vertex_compare(Vertex v1, Vertex v2) const {
			std::cout << "vertexComp(" << idx1[v1] << "(" << v1 << ")" << ", " << idx2[v2] << "(" << v2 << ")" << ") == false" << std::endl;
		}

		void at_out_degree(Vertex v1, Vertex v2) const {
			std::cout << "num_out_edges: " << out_degree(v1, g1) << " != " << out_degree(v2, g2) << std::endl;
		}

		void at_out_edge(Edge e1, Edge e2) const {
			boost::print_graph(g1, idx1);
			std::cout << std::endl;
			boost::print_graph(g2, idx2);
			std::string edge = boost::is_same<typename boost::graph_traits<Graph>::directed_category, boost::undirected_tag>::value ? " -- " : " -> ";
			std::cout << "out_edge: " << get(idx1, source(e1, g1)) << edge << get(idx1, target(e1, g1))
					<< " != " << get(idx2, source(e2, g2)) << edge << get(idx2, target(e2, g2)) << std::endl;
		}

		void at_edge_compare(Edge e1, Edge e2) const {
			std::cout << "edgeComp() == false" << std::endl;
		}

		void at_end(bool) const { }
	private:
		const Graph &g1;
		const Graph &g2;
		const Idx &idx1;
		const Idx &idx2;
	};

	template<typename Graph, typename Idx>
	static PrintVisitor<Graph, Idx> makePrintVisitor(const Graph &g1, const Graph &g2, const Idx &idx1, const Idx &idx2) {
		return PrintVisitor<Graph, Idx>(g1, g2, idx1, idx2);
	}

	template<typename Graph, typename VertexLess, typename EdgeHandler, typename Visitor>
	auto canonicalize_switch_debug(bool printStuff, const TestOptions &options, Graph &g, VertexLess vLess, EdgeHandler edgeHandler, Visitor visitor) {
		//		if(options.debugCanon || options.debugRefine || options.debugTree) {
		auto debugVisitor = graph_canon::debug_visitor(options.debugTree && printStuff, options.debugCanon && printStuff, options.debugAut && printStuff,
				options.debugRefine && printStuff, options.debugCompressed && printStuff);
		auto newVisitor = graph_canon::make_visitor(visitor, boost::ref(debugVisitor));
		return canonicalize_switch_alg(options, g, vLess, edgeHandler, newVisitor);
		//		} else {
		//			return canonicalize_switch_alg(options, g, vLess, eCompare, visitor);
		//		}
	}

	template<typename Graph, typename VertexLess, typename EdgeHandler>
	auto canonicalize(const TestOptions &options, std::size_t round, const Graph &g, VertexLess vLess, EdgeHandler edgeHandler) {
		bool withStuff = (!options.last && round == 0) || (options.last && round == options.rounds);
		//		if(!withStuff) {
		//			return canonicalize_switch_alg(options, g, vLess, eCompare, graph_canon::null_visitor());
		//		} else { // apply all the options
		std::unique_ptr<std::ofstream> graphDot;
		if(withStuff && !options.graphDot.empty()) {
			graphDot.reset(new std::ofstream(options.graphDot));
			if(!*graphDot)
				throw std::runtime_error("Could not open graphDot file '" + options.graphDot + "'.");
		}
		std::unique_ptr<std::ofstream> treeDot;
		if(withStuff && !options.treeDot.empty()) {
			treeDot.reset(new std::ofstream(options.treeDot));
			if(!*treeDot)
				throw std::runtime_error("Could not open treeDot file '" + options.treeDot + "'.");
		}
		auto visitor = graph_canon::stats_visitor(treeDot.get());
		auto permutation = canonicalize_switch_debug(withStuff, options, g, vLess, edgeHandler, boost::ref(visitor));
		if(withStuff && options.stats) std::cout << "Stats:\n" << visitor;
		if(withStuff && graphDot) {
			std::ostream &s = *graphDot;
			bool isUndirected = boost::is_undirected_graph<Graph>::value;
			if(isUndirected) s << "graph";
			else s << "digraph";
			s << " g {\n";

			BGL_FORALL_VERTICES_T(v, g, Graph) {
				std::size_t vId = get(boost::vertex_index_t(), g, v);
				s << "\t" << vId << " [ label=\"" << vId << " | " << permutation[vId] << "\" ];\n";
			}

			BGL_FORALL_EDGES_T(e, g, Graph) {
				s << "\t" << get(boost::vertex_index_t(), g, source(e, g));
				if(isUndirected) s << " -- ";
				else s << " -> ";
				s << get(boost::vertex_index_t(), g, target(e, g)) << ";\n";
			}
			s << "}\n";
		}
		return std::make_tuple(permutation, visitor.max_num_tree_nodes, visitor.num_tree_nodes);
		//		}
	}

	template<typename Graph>
	void execute(TestOptions &options, const Graph &g) {
		Options::Clock::duration time(0);
		Options::Clock::time_point start = Options::Clock::now();
		options.printHeader(std::cout) << "	max-nodes	nodes	n	m	round	time (ms)" << std::endl;
		auto canon_res = canonicalize(options, 0, g,
				graph_canon::make_property_less(get(boost::vertex_name_t(), g)),
#ifdef GRAPH_CANON_EDGE_LABELS
				graph_canon::make_edge_counter_int_vector<std::size_t>(get(boost::edge_name_t(), g), options.eLabelMax)
#else
				graph_canon::edge_handler_all_equal()
#endif
				);
		time += Options::Clock::now() - start;
		const auto idx = std::get<0>(canon_res);
		const auto maxTreeNodes = std::get<1>(canon_res);
		const auto numTreeNodes = std::get<2>(canon_res);
		options.printValues(std::cout) << "\t" << maxTreeNodes << "\t" << numTreeNodes << "\t" << num_vertices(g) << "\t" << num_edges(g) << "\t"
				<< 0 << "\t" << boost::chrono::duration_cast<boost::chrono::milliseconds>(time).count() << std::endl;
		auto idxMap = boost::make_iterator_property_map(idx.cbegin(), get(boost::vertex_index_t(), g));
		graph_canon::ordered_graph<Graph, decltype(idxMap) > orderedInputCanon(g, idxMap,
				graph_canon::make_property_less(get(boost::edge_name_t(), g)));

		std::vector<std::size_t> id_permutation(idx.size());
		for(std::size_t i = 0; i < idx.size(); i++) id_permutation[i] = i;
		for(std::size_t i = 1; i <= options.rounds; i++) {
			std::vector<std::size_t> permutation = make_random_permutation(options.gen, id_permutation);
			Graph g_permuted;
			permute_graph(g, g_permuted, permutation);
			Options::Clock::time_point start = Options::Clock::now();
			auto canon_res = canonicalize(options, i, g_permuted,
					graph_canon::make_property_less(get(boost::vertex_name_t(), g_permuted)),
#ifdef GRAPH_CANON_EDGE_LABELS
					graph_canon::make_edge_counter_int_vector<std::size_t>(get(boost::edge_name_t(), g_permuted), options.eLabelMax)
#else
					graph_canon::edge_handler_all_equal()
#endif
					);
			Options::Clock::duration dur = Options::Clock::now() - start;
			const auto idxPermuted = std::get<0>(canon_res);
			const auto maxTreeNodes = std::get<1>(canon_res);
			const auto numTreeNodes = std::get<2>(canon_res);
			options.printValues(std::cout) << "\t" << maxTreeNodes << "\t" << numTreeNodes << "\t" << num_vertices(g) << "\t" << num_edges(g) << "\t"
					<< i << "\t" << boost::chrono::duration_cast<boost::chrono::milliseconds>(dur).count() << std::endl;
			time += dur;
			auto idxPermutedMap = boost::make_iterator_property_map(idxPermuted.cbegin(), get(boost::vertex_index_t(), g_permuted));
			graph_canon::ordered_graph<Graph, decltype(idxPermutedMap) > orderedPermutedCanon(g_permuted, idxPermutedMap,
					graph_canon::make_property_less(get(boost::edge_name_t(), g_permuted)));

			using Vertex = typename boost::graph_traits<Graph>::vertex_descriptor;
			using Edge = typename boost::graph_traits<Graph>::edge_descriptor;
			auto vEqual = [&options, &g, &g_permuted](Vertex vLeft, Vertex vRight) {
				if(options.vLabelMode == LabelMode::None) return true;
				return get(boost::vertex_name_t(), g, vLeft)
						== get(boost::vertex_name_t(), g_permuted, vRight);
			};
			auto eEqual = [&options, &g, &g_permuted](Edge eLeft, Edge eRight) {
				if(options.eLabelMode == LabelMode::None) return true;
				return get(boost::edge_name_t(), g, eLeft)
						== get(boost::edge_name_t(), g_permuted, eRight);
			};

			if(!graph_canon::ordered_graph_equal(orderedInputCanon, orderedPermutedCanon,
					vEqual, eEqual,
					makePrintVisitor(orderedInputCanon, orderedPermutedCanon, idxMap, idxPermutedMap))) {
				bool withVertexName = options.vLabelMode != LabelMode::None;
				bool withEdgeName = options.eLabelMode != LabelMode::None;
				std::cout << "g_input_canon = C(g_input)\n";
				std::cout << "g_permuted = g_input^e\t(e is a random permutation)\n";
				std::cout << "g_permuted_canon = g_permuted^e^y = C(g_permuted^e)\n";
				std::cout << "but g_input_canon != g_permuted_canon\n";
				std::cout << "i =";
				for(std::size_t i = 0; i < permutation.size(); i++) std::cout << " " << i;
				std::cout << std::endl;
				std::cout << "e =";
				for(std::size_t i = 0; i < permutation.size(); i++) std::cout << " " << permutation[i];
				std::cout << std::endl;
				std::cout << "y =";
				for(std::size_t i = 0; i < idxPermuted.size(); i++) std::cout << " " << idxPermuted[i];
				std::cout << std::endl;
				std::cout << "g_input:" << std::endl;
				printGraph(g,
						makeVertexPrinter(get(boost::vertex_index_t(), g), withVertexName),
						makeEdgePrinter(get(boost::vertex_index_t(), g), withEdgeName));
				std::cout << "g_permuted:" << std::endl;
				printGraph(g_permuted,
						makeVertexPrinter(get(boost::vertex_index_t(), g_permuted), withVertexName),
						makeEdgePrinter(get(boost::vertex_index_t(), g_permuted), withEdgeName));
				std::cout << "g_input_canon:" << std::endl;
				printGraph(orderedInputCanon,
						makeOrderedVertexPrinter(idxMap, withVertexName),
						makeOrderedEdgePrinter(idxMap, withEdgeName));
				std::cout << "g_permuted_canon:" << std::endl;
				printGraph(orderedPermutedCanon,
						makeOrderedVertexPrinter(idxPermutedMap, withVertexName),
						makeOrderedEdgePrinter(idxPermutedMap, withEdgeName));
				throw std::logic_error("Result error");
			}
			// just for testing the ordered graph compare less
			auto vLess = [&options, &g, &g_permuted](Vertex vLeft, Vertex vRight) {
				if(options.vLabelMode == LabelMode::None) return true;
				return get(boost::vertex_name_t(), g, vLeft)
						< get(boost::vertex_name_t(), g_permuted, vRight);
			};
			auto eLess = [&options, &g, &g_permuted](Edge eLeft, Edge eRight) {
				if(options.eLabelMode == LabelMode::None) return true;
				return get(boost::edge_name_t(), g, eLeft)
						< get(boost::edge_name_t(), g_permuted, eRight);
			};
			bool less = graph_canon::ordered_graph_less(orderedInputCanon, orderedPermutedCanon, vLess, eLess, vEqual, eEqual);
			(void) less;
		}
		std::size_t totalTime = boost::chrono::duration_cast<boost::chrono::milliseconds>(time).count();
		std::cout << "Time: " << totalTime << " ms (" << (static_cast<double> (totalTime) / (options.rounds + 1))
				<< " ms, " << (options.rounds + 1) << " rounds)" << std::endl;
	}
};

int main(int argc, char **argv) {
	std::string modeDesc =
			"Test mode: first canonicalize the input graph as is, "
			"and verify the result of all subsequent canonicalizations. "
			"Debugging and statistics visitors can be attached.";
	TestOptions options;
	po::options_description optionsDesc("Options for 'test' mode");
	optionsDesc.add_options()
			("last", "Apply debug and output options to the last permuted graph instead of the input graph.")
			("graph-dot", po::value<std::string>(&options.graphDot), "Print graph in dot format, with the original and canonical indices to this file.")
			("tree-dot", po::value<std::string>(&options.treeDot), "Print the search tree from a canonicalization run.")
			("gall,g", "Print all debug information.")
			("gtree", "Print debug information related to tree traversal.")
			("gcanon", "Print debug information related to selecting a canonical leaf in the tree.")
			("gaut", "Print debug information related to automorphism discovery.")
			("grefine", "Print debug information related to the refinement procedure.")
			("gcompressed", "Print debug information in a shorter format.")
			("stats", "Print staticstics from one canonicalization run.")
			;
	return common_main<ModeTest>(argc, argv, options, optionsDesc, modeDesc);
}