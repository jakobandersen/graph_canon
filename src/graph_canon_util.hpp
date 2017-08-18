#ifndef GRAPH_CANON_TEST_GRAPH_CANON_UTIL_HPP
#define GRAPH_CANON_TEST_GRAPH_CANON_UTIL_HPP

// define GRAPH_CANON_EDGE_LABELS to enable edge labels, and thereby also the
// expensive edge counter.

// define GRAPH_CANON_REFINE to the class name of the refinement algorithms in
// graph_canon namespace, modulo the "refine_" prefix.
#ifndef GRAPH_CANON_REFINE
#error "GRAPH_CANON_REFINE must be defined."
#endif

// define GRAPH_CANON_AUT_PRUNE to the class name of the automorphism pruner algorithms in
// graph_canon namespace, modulo the "aut_pruner_" prefix.
// If not defined, then no automorphism pruning will be done.

// define GRAPH_CANON_PARTIAL_LEAF to enable the partial leaf certificate visitor.
// define GRAPH_CANON_TRACE to enable the trace visitor.
// define GRAPH_CANON_QUOTIENT to enable the quotient graph visitor.
// define GRAPH_CANON_AUT_IMPLICIT to enable the aut_implicit visitor.
// define GRAPH_CANON_DEGREE_1 to enable the degree-1 visitor.

#define GRAPH_CANON_CAT_1(x, y) x ## y
#define GRAPH_CANON_CAT(x, y) GRAPH_CANON_CAT_1(x, y)

#include "util.hpp"

#include <graph_canon/aut/implicit_size_2.hpp>
#include <graph_canon/aut/pruner_basic.hpp>
#include <graph_canon/canonicalization.hpp>
#include <graph_canon/compare.hpp>
#include <graph_canon/invariant/cell_split.hpp>
#include <graph_canon/invariant/partial_leaf.hpp>
#include <graph_canon/invariant/quotient.hpp>
#include <graph_canon/ordered_graph.hpp>
#include <graph_canon/refine/WL_1.hpp>
#include <graph_canon/refine/degree_1.hpp>
#include <graph_canon/target_cell/f.hpp>
#include <graph_canon/target_cell/fl.hpp>
#include <graph_canon/target_cell/flmcr.hpp>
#include <graph_canon/target_cell/flm.hpp>
#include <graph_canon/tree_traversal/bfs-exp.hpp>
#include <graph_canon/tree_traversal/bfs-exp-m.hpp>
#include <graph_canon/dimacs_graph_io.hpp>
#include <graph_canon/util.hpp>

#include <boost/chrono/duration.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_utility.hpp> // for boost::print_graph
#include <boost/program_options.hpp>
#include <boost/random/mersenne_twister.hpp>

#include <iomanip>
#include <iostream>
#include <fstream>

namespace po = boost::program_options;

//------------------------------------------------------------------------------
// Stuff for making a canonicalisation program
//------------------------------------------------------------------------------

// LabelMode
//------------------------------------------------------------------------------

enum class LabelMode {
	None, Uniform
};

std::istream &operator>>(std::istream &s, LabelMode &mode) {
	std::string token;
	s >> token;
	if(token == "none") mode = LabelMode::None;
	else if(token == "uniform") mode = LabelMode::Uniform;
	else throw po::invalid_option_value("invalid label mode '" + token + "'");
	return s;
}

std::ostream &operator<<(std::ostream &s, LabelMode mode) {
	switch(mode) {
	case LabelMode::None: return s << "none";
	case LabelMode::Uniform: return s << "uniform";
	}
	return s;
}

// TargetCellSelector
//------------------------------------------------------------------------------

enum class TargetCellSelector {
	F, FL, FLM, FLMCR
};

std::istream &operator>>(std::istream &s, TargetCellSelector &selector) {
	std::string token;
	s >> token;
	if(token == "f") selector = TargetCellSelector::F;
	else if(token == "fl") selector = TargetCellSelector::FL;
	else if(token == "flm") selector = TargetCellSelector::FLM;
	else if(token == "flmcr") selector = TargetCellSelector::FLMCR;
	else throw po::invalid_option_value("invalid target cell selector '" + token + "'");
	return s;
}

std::ostream &operator<<(std::ostream &s, TargetCellSelector selector) {
	switch(selector) {
	case TargetCellSelector::F: return s << "f";
	case TargetCellSelector::FL: return s << "fl";
	case TargetCellSelector::FLM: return s << "flm";
	case TargetCellSelector::FLMCR: return s << "flmcr";
	default: assert(false);
	}
	return s;
}

// TreeTraversal
//------------------------------------------------------------------------------

enum class TreeTraversal {
	DFS, BFSExp, BFSExpM
};

std::istream &operator>>(std::istream &s, TreeTraversal &tree) {
	std::string token;
	s >> token;
	if(token == "dfs") tree = TreeTraversal::DFS;
	else if(token == "bfs-exp") tree = TreeTraversal::BFSExp;
	else if(token == "bfs-exp-m") tree = TreeTraversal::BFSExpM;
	else throw po::invalid_option_value("invalid tree traversal algorithm '" + token + "'");
	return s;
}

std::ostream &operator<<(std::ostream &s, TreeTraversal tree) {
	switch(tree) {
	case TreeTraversal::DFS: return s << "dfs";
	case TreeTraversal::BFSExp: return s << "bfs-exp";
	case TreeTraversal::BFSExpM: return s << "bfs-exp-m";
	}
	return s;
}

//------------------------------------------------------------------------------

struct Options {
	using Clock = boost::chrono::system_clock;

	void setSource(const std::string &src) {
		if(id.empty()) id = src;
	}

	void from(const po::variables_map &vm) {
		gen.seed(seed);
		if(vLabelMax == 0) vLabelMax = 1;
		if(eLabelMax == 0) eLabelMax = 1;
		directed = vm.count("directed") > 0;
		parallelEdges = vm.count("parallel-edges") > 0;
		loops = vm.count("loops") > 0;
	}

	std::ostream &printHeader(std::ostream &s) const {
		return s << id << "\ttarget-cell\ttree-traversal" << postHeader;
	}

	std::ostream &printValues(std::ostream &s) const {
		return s << id
				<< "\t" << std::setw(11) << std::left << targetCellSelector
				<< "\t" << std::setw(14) << std::left << treeTraversal
				<< postId;
	}
public:
	std::string id, postId, postHeader;
	boost::random::mt19937 gen;
	std::string dimacs;
	LabelMode vLabelMode, eLabelMode;
	std::size_t vLabelMax, eLabelMax;
	bool directed, parallelEdges, loops;
	std::size_t seed, rounds;
	// alg selection
	TargetCellSelector targetCellSelector;
	TreeTraversal treeTraversal;
	std::size_t max_mem;
};

struct dynamic_target_cell_selector : graph_canon::null_visitor {
	using can_select_target_cell = std::true_type;

	template<typename Config, typename TreeNode>
	struct InstanceData {
		using F = typename graph_canon::target_cell_f::InstanceData<Config, TreeNode>::type;
		using FL = typename graph_canon::target_cell_fl::InstanceData<Config, TreeNode>::type;
		using FLI = typename graph_canon::target_cell_flmcr::InstanceData<Config, TreeNode>::type;
		using FLM = typename graph_canon::target_cell_flm::InstanceData<Config, TreeNode>::type;
		using type = typename graph_canon::tagged_list_concat_many<F, FL, FLI, FLM>::type;
	};
public:

	dynamic_target_cell_selector(TargetCellSelector tcs) : tcs(tcs) { }

	void initialize(auto &s) {
		switch(tcs) {
		case TargetCellSelector::F:
			return graph_canon::target_cell_f().initialize(s);
		case TargetCellSelector::FL:
			return graph_canon::target_cell_fl().initialize(s);
		case TargetCellSelector::FLM:
			return graph_canon::target_cell_flm().initialize(s);
		case TargetCellSelector::FLMCR:
			return graph_canon::target_cell_flmcr().initialize(s);
		}
	}

	std::size_t select_target_cell(auto &s, auto &t) {
		switch(tcs) {
		case TargetCellSelector::F:
			return graph_canon::target_cell_f().select_target_cell(s, t);
		case TargetCellSelector::FL:
			return graph_canon::target_cell_fl().select_target_cell(s, t);
		case TargetCellSelector::FLM:
			return graph_canon::target_cell_flm().select_target_cell(s, t);
		case TargetCellSelector::FLMCR:
			return graph_canon::target_cell_flmcr().select_target_cell(s, t);
		}
	}
private:
	TargetCellSelector tcs;
};

struct dynamic_tree_traversal : graph_canon::null_visitor {
	using can_explore_tree = std::true_type;

	template<typename Config, typename TreeNode>
	struct InstanceData {
		using DFS = typename graph_canon::traversal_dfs::InstanceData<Config, TreeNode>::type;
		using BFSExp = typename graph_canon::traversal_bfs_exp::InstanceData<Config, TreeNode>::type;
		using BFSExpM = typename graph_canon::traversal_bfs_exp_m::InstanceData<Config, TreeNode>::type;
		using type = typename graph_canon::tagged_list_concat_many<DFS, BFSExp, BFSExpM>::type;
	};

	template<typename Config, typename TreeNode>
	struct TreeNodeData {
		using DFS = typename graph_canon::traversal_dfs::TreeNodeData<Config, TreeNode>::type;
		using BFSExp = typename graph_canon::traversal_bfs_exp::TreeNodeData<Config, TreeNode>::type;
		using BFSExpM = typename graph_canon::traversal_bfs_exp_m::TreeNodeData<Config, TreeNode>::type;
		using type = typename graph_canon::tagged_list_concat_many<DFS, BFSExp, BFSExpM>::type;
	};
public:

	dynamic_tree_traversal(TreeTraversal tt, std::size_t max_mem) : tt(tt), max_mem(max_mem) { }

	template<typename State>
	void initialize(State &state) {
		switch(tt) {
		case TreeTraversal::DFS:
			return graph_canon::traversal_dfs().initialize(state);
		case TreeTraversal::BFSExp:
			return graph_canon::traversal_bfs_exp().initialize(state);
		case TreeTraversal::BFSExpM:
			return graph_canon::traversal_bfs_exp_m(max_mem).initialize(state);
		}
	}

	template<typename State, typename TreeNode>
	bool tree_create_node_begin(State &state, TreeNode &t) {
		switch(tt) {
		case TreeTraversal::DFS:
			return graph_canon::traversal_dfs().tree_create_node_begin(state, t);
		case TreeTraversal::BFSExp:
			return graph_canon::traversal_bfs_exp().tree_create_node_begin(state, t);
		case TreeTraversal::BFSExpM:
			return graph_canon::traversal_bfs_exp_m(max_mem).tree_create_node_begin(state, t);
		}
	}

	template<typename State, typename TreeNode>
	void tree_destroy_node(State &state, TreeNode &t) {
		switch(tt) {
		case TreeTraversal::DFS:
			return graph_canon::traversal_dfs().tree_destroy_node(state, t);
		case TreeTraversal::BFSExp:
			return graph_canon::traversal_bfs_exp().tree_destroy_node(state, t);
		case TreeTraversal::BFSExpM:
			return graph_canon::traversal_bfs_exp_m(max_mem).tree_destroy_node(state, t);
		}
	}

	template<typename State, typename TreeNode>
	void tree_prune_node(const State &state, TreeNode &t) {
		switch(tt) {
		case TreeTraversal::DFS:
			return graph_canon::traversal_dfs().tree_prune_node(state, t);
		case TreeTraversal::BFSExp:
			return graph_canon::traversal_bfs_exp().tree_prune_node(state, t);
		case TreeTraversal::BFSExpM:
			return graph_canon::traversal_bfs_exp_m(max_mem).tree_prune_node(state, t);
		}
	}

	template<typename State>
	void explore_tree(State &state) {
		switch(tt) {
		case TreeTraversal::DFS:
			return graph_canon::traversal_dfs().explore_tree(state);
		case TreeTraversal::BFSExp:
			return graph_canon::traversal_bfs_exp().explore_tree(state);
		case TreeTraversal::BFSExpM:
			return graph_canon::traversal_bfs_exp_m(max_mem).explore_tree(state);
		}
	}
private:
	TreeTraversal tt;
	std::size_t max_mem;
};

template<typename Graph, typename VertexLess, typename EdgeHandler, typename Visitor>
auto canonicalize_call_alg(const Options &options, Graph &g, VertexLess vLess, EdgeHandler edgeHandler, Visitor visitor) {
	if(options.parallelEdges) {
		std::cerr << "Parallel edges are currently disabled." << std::endl;
		std::exit(1);
		//		if(options.loops) {
		//			graph_canon::canonicalizer<std::size_t, true, true> canonicalizer;
		//			return canonicalizer(g, get(boost::vertex_index_t(), g), vLess, edgeHandler, visitor, treeTraversal);
		//		} else {
		//			graph_canon::canonicalizer<std::size_t, true, false> canonicalizer;
		//			return canonicalizer(g, get(boost::vertex_index_t(), g), vLess, edgeHandler, visitor, treeTraversal);
		//		}
	} else {
		if(options.loops) {
			std::cerr << "Loop edges are currently disabled." << std::endl;
			std::exit(1);
			//			graph_canon::canonicalizer<std::size_t, false, true> canonicalizer;
			//			return canonicalizer(g, get(boost::vertex_index_t(), g), vLess, edgeHandler, visitor, treeTraversal);
		} else {
			graph_canon::canonicalizer<int, EdgeHandler, false, false> canonicalizer(edgeHandler);
			return canonicalizer(g, get(boost::vertex_index_t(), g), vLess, visitor);
		}
	}
}

template<typename Graph, typename VertexLess, typename EdgeHandler, typename Visitor>
auto canonicalize_refine(const Options &options, Graph &g, VertexLess vLess, EdgeHandler edgeHandler, Visitor visitor) {
	graph_canon::GRAPH_CANON_CAT(refine_, GRAPH_CANON_REFINE) refine;
	return canonicalize_call_alg(options, g, vLess, edgeHandler, graph_canon::make_visitor(refine, visitor));
}

template<typename Graph, typename VertexLess, typename EdgeHandler, typename Visitor>
auto canonicalize_switch_tree_traversal(const Options &options, Graph &g, VertexLess vLess, EdgeHandler edgeHandler, Visitor visitor) {
	auto tt = dynamic_tree_traversal(options.treeTraversal, options.max_mem);
	return canonicalize_refine(options, g, vLess, edgeHandler, graph_canon::make_visitor(tt, visitor));
}

template<typename Graph, typename VertexLess, typename EdgeHandler, typename Visitor>
auto canonicalize_switch_target_cell_selector(const Options &options, Graph &g, VertexLess vLess, EdgeHandler edgeHandler, Visitor visitor) {
	auto tc = dynamic_target_cell_selector(options.targetCellSelector);
	return canonicalize_switch_tree_traversal(options, g, vLess, edgeHandler, graph_canon::make_visitor(tc, visitor));
}

template<typename Graph, typename VertexLess, typename EdgeHandler, typename Visitor>
auto canonicalize_switch_degree_1(const Options &options, Graph &g, VertexLess vLess, EdgeHandler edgeHandler, Visitor visitor) {
#ifdef GRAPH_CANON_DEGREE_1
	graph_canon::refine_degree_1 degree_1;
	return canonicalize_switch_target_cell_selector(options, g, vLess, edgeHandler, graph_canon::make_visitor(boost::ref(degree_1), visitor));
#else
	return canonicalize_switch_target_cell_selector(options, g, vLess, edgeHandler, visitor);
#endif
}

template<typename Graph, typename VertexLess, typename EdgeHandler, typename Visitor>
auto canonicalize_switch_aut_implicit(const Options &options, Graph &g, VertexLess vLess, EdgeHandler edgeHandler, Visitor visitor) {
#ifdef GRAPH_CANON_AUT_IMPLICIT
	graph_canon::aut_implicit_size_2 implicit_automorphisms;
	return canonicalize_switch_degree_1(options, g, vLess, edgeHandler, graph_canon::make_visitor(boost::ref(implicit_automorphisms), visitor));
#else
	return canonicalize_switch_degree_1(options, g, vLess, edgeHandler, visitor);
#endif
}

template<typename Graph, typename VertexLess, typename EdgeHandler, typename Visitor>
auto canonicalize_switch_aut_pruner(const Options &options, Graph &g, VertexLess vLess, EdgeHandler edgeHandler, Visitor visitor) {
#ifdef GRAPH_CANON_AUT_PRUNER
	graph_canon::GRAPH_CANON_CAT(aut_pruner_, GRAPH_CANON_AUT_PRUNER) pruner;
	return canonicalize_switch_aut_implicit(options, g, vLess, edgeHandler, graph_canon::make_visitor(pruner, visitor));
#else
	return canonicalize_switch_aut_implicit(options, g, vLess, edgeHandler, visitor);
#endif
}

template<typename Graph, typename VertexLess, typename EdgeHandler, typename Visitor>
auto canonicalize_switch_quotient(const Options &options, Graph &g, VertexLess vLess, EdgeHandler edgeHandler, Visitor visitor) {
#ifdef GRAPH_CANON_QUOTIENT
	return canonicalize_switch_aut_pruner(options, g, vLess, edgeHandler,
			graph_canon::make_visitor(visitor, graph_canon::invariant_quotient()));
#else
	return canonicalize_switch_aut_pruner(options, g, vLess, edgeHandler, visitor);
#endif
}

template<typename Graph, typename VertexLess, typename EdgeHandler, typename Visitor>
auto canonicalize_switch_trace(const Options &options, Graph &g, VertexLess vLess, EdgeHandler edgeHandler, Visitor visitor) {
#ifdef GRAPH_CANON_TRACE
	return canonicalize_switch_quotient(options, g, vLess, edgeHandler,
			graph_canon::make_visitor(visitor, graph_canon::invariant_cell_split()));
#else
	return canonicalize_switch_quotient(options, g, vLess, edgeHandler, visitor);
#endif
}

template<typename Graph, typename VertexLess, typename EdgeHandler, typename Visitor>
auto canonicalize_switch_partial_leaf(const Options &options, Graph &g, VertexLess vLess, EdgeHandler edgeHandler, Visitor visitor) {
#ifdef GRAPH_CANON_PARTIAL_LEAF
	return canonicalize_switch_trace(options, g, vLess, edgeHandler,
			graph_canon::make_visitor(visitor, graph_canon::invariant_partial_leaf()));
#else
	return canonicalize_switch_trace(options, g, vLess, edgeHandler, visitor);
#endif
}

template<typename Graph, typename VertexLess, typename EdgeHandler, typename Visitor>
auto canonicalize_switch_alg(const Options &options, Graph &g, VertexLess vLess, EdgeHandler edgeHandler, Visitor visitor) {
	return canonicalize_switch_partial_leaf(options, g, vLess, edgeHandler, visitor);
}

std::string getDefaultGraph() {
	return
	"p edge 10 14\n"
	"e 1 3\n"
	"e 1 4\n"
	"\n"
	"e 2 5\n"
	"e 2 6\n"
	"\n"
	"e 3 7\n"
	"e 3 10\n"
	"\n"
	"e 4 8\n"
	"e 4 9\n"
	"\n"
	"e 5 7\n"
	"e 5 9\n"
	"\n"
	"e 6 8\n"
	"e 6 10\n"
	"\n"
	"e 7 9\n"
	"\n"
	"e 8 10\n";
}

template<typename Graph>
void loadGraph(Options &options, Graph &g) {
	const auto parHandler = [&options](unsigned int src, unsigned int tar) {
		if(options.parallelEdges) return true;
		std::cerr << "Ignoring parallel edge (" << src << ", " << tar << "). Use --parallel-edges to recognise it." << std::endl;
		return false;
	};
	const auto loopHandler = [&options](unsigned int v) {
		if(options.loops) return true;
		std::cerr << "Ignoring loop on vertex " << v << ". Use --loops to recognise it." << std::endl;
		return false;
	};
	bool res;
	if(options.dimacs.empty()) {
		options.setSource("default");
		std::istringstream ss(getDefaultGraph());
		res = graph_canon::read_dimacs_graph(ss, g, std::cerr, parHandler, loopHandler);
	} else if(options.dimacs == "-") {
		options.setSource("stdin");
		res = graph_canon::read_dimacs_graph(std::cin, g, std::cerr, parHandler, loopHandler);
	} else {
		options.setSource(options.dimacs);
		std::ifstream ifs(options.dimacs);
		if(!ifs) throw std::runtime_error("Could not open file '" + options.dimacs + "'.");
		res = graph_canon::read_dimacs_graph(ifs, g, std::cerr, parHandler, loopHandler);
	}
	if(!res) throw std::runtime_error("Could not parse input.");
}

template<typename Graph>
void assignLabels(Options &options, Graph &g) {
	// vertices
	switch(options.vLabelMode) {
	case LabelMode::None:
		// isn't this redundant? boost::property seems to default construct data
		BGL_FORALL_VERTICES_T(v, g, Graph)
	{
		put(boost::vertex_name_t(), g, v, 0);
	}
		break;
	case LabelMode::Uniform:
	{
		assert(options.vLabelMax > 0);
		std::uniform_int_distribution<std::size_t> dist(0, options.vLabelMax - 1);

		BGL_FORALL_VERTICES_T(v, g, Graph) {
			put(boost::vertex_name_t(), g, v, dist(options.gen));
		}
	}
		break;
	}
	// edges
	if(options.eLabelMode == LabelMode::None) {
#ifdef GRAPH_CANON_EDGE_LABELS
		std::cout << "Bug: edge label argument not parsed correctly (none)." << std::endl;
		std::exit(1);
#endif
	} else {
#ifndef GRAPH_CANON_EDGE_LABELS
		std::cout << "Bug: edge label argument not parsed correctly (not none)." << std::endl;
		std::exit(1);
#endif
	}
	switch(options.eLabelMode) {
	case LabelMode::None:
		// isn't this redundant? boost::property seems to default construct data
		BGL_FORALL_EDGES_T(e, g, Graph)
	{
		put(boost::edge_name_t(), g, e, 0);
	}
		break;
	case LabelMode::Uniform:
	{
		assert(options.eLabelMax > 0);
		std::uniform_int_distribution<std::size_t> dist(0, options.eLabelMax - 1);

		BGL_FORALL_EDGES_T(e, g, Graph) {
			put(boost::edge_name_t(), g, e, dist(options.gen));
		}
	}
		break;
	}
}

template<typename Options, typename Executor>
void loadAndExecute(Options &options, Executor executor) {
	if(options.directed) {
		std::cout << "Directed Graphs are not yet supported." << std::endl;
		std::exit(1);
		//		boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
		//				boost::property<boost::vertex_name_t, std::size_t>,
		//				boost::property<boost::vertex_name_t, std::size_t>
		//				> g;
		//		loadGraph(options, g);
		//		assignLabels(options, g);
		//		executor.execute(options, graph);
	} else { // undirected
		boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
				boost::property<boost::vertex_name_t, std::size_t>,
				boost::property<boost::edge_name_t, std::size_t>
				> g;
		loadGraph(options, g);
		assignLabels(options, g);
		executor.execute(options, g);
	}
}

template<typename Executor, typename Options>
int common_main(int argc, char **argv, Options &options, po::options_description &modeOptionsDesc, const std::string &modeDesc) {
	po::options_description generalOptionDesc("General Options"), optionDesc(modeDesc + "\n\nOptions");
	generalOptionDesc.add_options()
			("help,h", "Print help message.")
			("id", po::value<std::string>(&options.id), "The id to print in the beginning of every output line. "
			"It defaults to either 'default', 'stdin', or the given filename.")
			("postId", po::value<std::string>(&options.postId), "The id to print in the end of every data output line. "
			"It defaults to the empty string.")
			("postHeader", po::value<std::string>(&options.postHeader), "The id to print in the end of every header output line. "
			"It defaults to the empty string.")
			("seed,s", po::value<std::size_t>(&options.seed)->default_value(static_cast<std::size_t> (std::time(0))),
			"Seed for random number generator. The default value is 'static_cast<std::size_t> (std::time(0))'.")
			("permutations,p", po::value<std::size_t>(&options.rounds)->default_value(5), "Number of random permutations to try.")
			("dimacs,f", po::value<std::string>(&options.dimacs), "File with graph in DIMACS format. Use '-' to read from stdin.")
			("vertex-labels", po::value<LabelMode>(&options.vLabelMode)->default_value(LabelMode::None),
			"  'none': vertices are unlabelled.\n"
			"  'uniform': vertices are labelled with a uniformly random number in [0, vertex-label-max[.")
			("vertex-label-max", po::value<std::size_t>(&options.vLabelMax)->default_value(5))
			("edge-labels", po::value<LabelMode>(&options.eLabelMode)->default_value(LabelMode::None),
			"  'none': edges are unlabelled.\n"
			"  'uniform': edges are labelled with a uniformly random number in [0, edge-label-max[.")
			("edge-label-max", po::value<std::size_t>(&options.eLabelMax)->default_value(5))
			("directed,d", "Interpret the graph as a directed graph.")
			("parallel-edges", "Allow parallel edges, otherwise they are ignored.")
			("loops", "Allow loop edges, otherwise they are ignored.")
			("ftarget-cell", po::value<TargetCellSelector>(&options.targetCellSelector)->default_value(TargetCellSelector::FLM),
			"The algorithm for selecting target cells.\n"
			" 'f': select the first non-trivial cell.\n"
			" 'fl': select the first of the largest cell.\n"
			" 'flm': select the first of the largest cell of those with maximum non-uniformly joined degree.\n"
			" 'flmcr': select the first of the largest cell of those with maximum non-uniformly joined degree, subject to non-uniform component recursion."
			)
			("ftree-traversal", po::value<TreeTraversal>(&options.treeTraversal)->default_value(TreeTraversal::BFSExp),
			"The algorithm for exploring the search tree.\n"
			" 'dfs': Depth-first traversal.\n"
			" 'bfs-exp': Breadth-first traversal with 1 experimental path per tree vertex.\n"
			" 'bfs-exp-m': Breadth-first traversal with 1 experimental path per tree vertex.< Limited by memory specified by -m.")
			("m,memory", po::value<std::size_t>(&options.max_mem)->default_value(4 * 1024),
			"Memory limit (MB) before the tree traversal bfs-exp-m switches to DFS mode.");
	optionDesc.add(generalOptionDesc).add(modeOptionsDesc);

	po::positional_options_description positionalDesc; // for disallowing extra arguments

	po::variables_map rawOptions;
	try {
		po::store(po::command_line_parser(argc, argv).options(optionDesc).positional(positionalDesc).run(), rawOptions);
	} catch(const po::error &e) {
		std::cout << e.what() << '\n';
		std::exit(1);
	}
	po::notify(rawOptions);
	if(rawOptions.count("help")) {
		std::cout << "Canonicalize <permutations> random permutations of the input graph." << std::endl;
		std::cout << optionDesc << std::endl;
		return 0;
	}
	options.from(rawOptions);
	std::cout << "seed = " << options.seed << std::endl;
	loadAndExecute(options, Executor());
	return 0;
}

#endif /* GRAPH_CANON_TEST_GRAPH_CANON_UTIL_HPP */