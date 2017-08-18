// The algorithms are implemented using the resources [1-5], and the terminology follows
// primarily [3].
// 
// [1]
// PRACTICAL GRAPH ISOMORPHISM 
// Brendan D. McKay
// http://cs.anu.edu.au/people/bdm/nauty/pgi.pdf
//
// [2]
// McKay’s Canonical Graph Labeling Algorithm
// Stephen G. Hartke and A. J. Radcliﬀe
// http://www.math.unl.edu/~aradcliffe1/Papers/Canonical.pdf
//
// [3]
// Practical graph isomorphism, II
// Brendan D. McKay, Adolfo Piperno
// http://arxiv.org/abs/1301.1493
//
// [4]
// The webpage of nauty and Traces
// http://pallini.di.uniroma1.it/
//
// [5]
// Search Space Contraction in Canonical Labeling of Graphs
// Adolfo Piperno
// http://arxiv.org/abs/0804.4881


#ifndef GRAPH_CANON_CANONICALIZATION_HPP
#define GRAPH_CANON_CANONICALIZATION_HPP

#include <graph_canon/tagged_list.hpp>
#include <graph_canon/util.hpp>
#include <graph_canon/edge_handler/all_equal.hpp> // default
#include <graph_canon/visitor/compound.hpp>
#include <graph_canon/refine/WL_1.hpp> // default
#include <graph_canon/invariant/support.hpp> // currently always added
#include <graph_canon/detail/explicit_automorphism.hpp>
#include <graph_canon/detail/partition.hpp>
#include <graph_canon/detail/permuted_graph_view.hpp>
#include <graph_canon/detail/tree_node.hpp>

#include <boost/assert.hpp>
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>
#include <boost/graph/properties.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>

#include <iostream>
#include <vector>

namespace graph_canon {
namespace detail {

template<typename SizeType, typename EdgeHandlerCreator, bool ParallelEdges, bool Loops>
struct canonicalizer;

template<
typename SizeTypeT, bool ParallelEdgesV, bool LoopsV,
typename GraphT, typename IndexMapT,
typename EdgeHandlerT
>
struct config {
	using SizeType = SizeTypeT;
	static constexpr bool ParallelEdges = ParallelEdgesV;
	static constexpr bool Loops = LoopsV;
	using Graph = GraphT;
	using IndexMap = IndexMapT;
	using EdgeHandler = EdgeHandlerT;
public:
	using Permutation = std::vector<SizeType>;
	using Partition = partition<SizeType>;

	using Vertex = typename boost::graph_traits<Graph>::vertex_descriptor;
	using Edge = typename boost::graph_traits<Graph>::edge_descriptor;
};

//------------------------------------------------------------------------------
// canon_state
//------------------------------------------------------------------------------

template<typename Visitor, typename Config>
struct InstanceDataGenerator {

	template<typename TreeNode>
	struct apply {
		using type = typename unwrap_visitor<Visitor>::type::template InstanceData<Config, TreeNode>::type;
	};
};

template<typename State, typename Config>
struct TreeNodeDataGenerator {

	template<typename TreeNode>
	struct apply {
		// We want to give the tree nodes access to the state
		using Data = typename unwrap_visitor<typename State::Visitor>::type::template TreeNodeData<Config, TreeNode>::type;

		struct type {

			type(State &state) : state(state) { }
		public:
			Data data;
			State &state;
		public:

			template<typename Tag>
			friend decltype(auto) get(Tag, type &t) {
				return get(Tag(), t.data);
			}

			template<typename Tag>
			friend decltype(auto) get(Tag, const type &t) {
				return get(Tag(), t.data);
			}
		};
	};
};

template<typename ConfigT, typename VisitorT>
struct canon_state {
	using Config = ConfigT;
	using Visitor = VisitorT;
	using Self = canon_state<Config, Visitor>;
	using SizeType = typename Config::SizeType;
	using Graph = typename Config::Graph;
	using IndexMap = typename Config::IndexMap;
	using EdgeHandler = typename Config::EdgeHandler;
	static constexpr bool ParallelEdges = Config::ParallelEdges;
	static constexpr bool Loops = Config::Loops;

	using Partition = typename Config::Partition;
	using Permutation = typename Config::Permutation;
	using Vertex = typename Config::Vertex;
	using Edge = typename Config::Edge;
public:
	using TreeNode = tree_node<SizeType, TreeNodeDataGenerator<Self, Config> >;
	using OwnerPtr = typename TreeNode::OwnerPtr;
	using InstanceData = typename InstanceDataGenerator<Visitor, Config>::template apply<TreeNode>::type;
	using PermutedGraph = permuted_graph_view<Config, TreeNode>;
private:

	template<typename VertexLess>
	void splitByPredicate(VertexLess vertex_less, Partition &pi) {
		const auto less = [this, &vertex_less](const auto u_idx, const auto v_idx) {
			return vertex_less(vertex(u_idx, g), vertex(v_idx, g));
		};
		SizeType refinee_end_idx; // = 0; // init to shut up compiler
		for(SizeType refinee_begin_idx = 0; refinee_begin_idx < n; refinee_begin_idx = refinee_end_idx) {
			refinee_end_idx = pi.get_cell_end(refinee_begin_idx);
			std::sort(pi.begin() + refinee_begin_idx, pi.begin() + refinee_end_idx, less);
			auto raii_splitter = pi.split_cell(refinee_begin_idx);
			for(SizeType i = refinee_begin_idx + 1; i < refinee_end_idx; ++i)
				if(less(pi.get(i - 1), pi.get(i)))
					raii_splitter.add_split(i);
		}
		pi.reset_inverse(0, n);
		for(SizeType cell = 0; cell != n; cell = pi.get_cell_end(cell))
			pi.set_element_to_cell(cell);
	}
public:

	template<typename VertexLess>
	canon_state(const Graph &g, IndexMap idx, EdgeHandler &edge_handler, Visitor visitor, VertexLess vertex_less, Partition &&pi)
	: g(g), n(num_vertices(g)), idx(idx), edge_handler(edge_handler), visitor(std::move(visitor)),
	root(nullptr),
	canon_leaf(nullptr), canon_permuted_graph(nullptr), extra_permuted_graph(nullptr) {
		this->edge_handler.initialize(*this);
		this->visitor.initialize(*this);
		// first let the user determine the order
		splitByPredicate(vertex_less, pi);
		// and let's do it by degree as well such that visitors can rely on this refinement
		const auto vertex_degree_less = [&g](const Vertex &u, const Vertex & v) {
			return degree(u, g) < degree(v, g);
		};
		splitByPredicate(vertex_degree_less, pi);
		root = TreeNode::make(std::move(pi), *this);
		assert(!root->get_is_pruned());
	}

	~canon_state() {
		delete canon_permuted_graph; // may deallocate a path in the tree
		delete extra_permuted_graph; // may deallocate a path in the tree
		// The rest is for debugging purposes:
		// make sure we actually have the very last owner pointers to the tree.
		canon_leaf = nullptr; // may deallocate a path in the tree
		assert(std::uncaught_exception() || root->get_ref_count() == 1);
		root = nullptr;
	}

	// returns false iff this leads to a worse leaf

	bool make_equitable(TreeNode &node) {
		RefinementResult result;
		//		std::cout << "make_equitable:" << std::endl;
		do {
			result = visitor.refine(*this, node);
			//			std::cout << "make_equitable round: " << result << std::endl << std::endl;
		} while(result == RefinementResult::Again);
		//		std::cout << "make_equitable end: " << result << std::endl;
		return result != RefinementResult::Abort;
	}

	Permutation get_canonical_permutation() {
		//    assert(canon_leaf);
		if(!canon_leaf) throw 1; //std::abort();
		Permutation p(n);
		std::copy(canon_leaf->pi.begin_inverse(), canon_leaf->pi.end_inverse(), p.begin());
		return p;
	}

	SizeType select_target_cell(TreeNode &t) {
		return visitor.select_target_cell(*this, t);
	}

	void add_terminal(OwnerPtr node) {
		assert(node != canon_leaf);
		visitor.tree_leaf(*this, *node);
		if(extra_permuted_graph) {
			extra_permuted_graph->repermute(*this, node);
		} else {
			extra_permuted_graph = new PermutedGraph(*this, node);
		}
		if(!canon_leaf) { // canon_permuted_graph may still be valid if someone pruned our canon_leaf
			canon_leaf = node;
			std::swap(canon_permuted_graph, extra_permuted_graph);
			visitor.canon_new_best(*this);
		} else {
			assert(extra_permuted_graph);
			assert(canon_permuted_graph);
			auto cmp = PermutedGraph::compare(*this, *extra_permuted_graph, *canon_permuted_graph);
			if(cmp < 0) {
				std::swap(canon_permuted_graph, extra_permuted_graph);
				canon_leaf = node;
				visitor.canon_new_best(*this);
			} else if(cmp == 0) {
				explicit_automorphism<Self> aut(*this, *node);
				visitor.automorphism_leaf(*this, *node, aut);
			} else {
				visitor.canon_worse(*this, *node);
			}
		}
	}

	void prune_canon_leaf() {
		if(!canon_leaf) return;
		visitor.canon_prune(*this);
		canon_leaf = nullptr;
	}

	TreeNode *get_canon_leaf() const {
		return canon_leaf.get();
	}
public:
	const Graph &g;
	SizeType n; // num_vertices(g)
	IndexMap idx;
	InstanceData data; // should be the last calculated data to be deleted
	EdgeHandler &edge_handler; // stored in the canonicalisation object, to reuse it
	Visitor visitor;
	OwnerPtr root;
private:
	OwnerPtr canon_leaf; // best graph of all the best_by_invariant
	PermutedGraph *canon_permuted_graph, *extra_permuted_graph; // has owner pointers to their leaves
};

} // namespace detail

template<typename SizeType, typename EdgeHandlerCreator, bool ParallelEdges, bool Loops>
struct canonicalizer {
	BOOST_STATIC_ASSERT_MSG(boost::is_integral<SizeType>::value, "SizeType must be integral.");
	using EdgeHandler = typename EdgeHandlerCreator::template type<SizeType>;
public:

	canonicalizer(EdgeHandlerCreator edge_handler_creator)
	: edge_handler(edge_handler_creator.template make<SizeType>()) { }

	template<typename Graph, typename IndexMap, typename VertexLess, typename Visitor>
	auto operator()(const Graph &g, IndexMap idx, VertexLess vertex_less, Visitor visitor) {
		// static checks
		BOOST_CONCEPT_ASSERT((boost::ReadablePropertyMapConcept<IndexMap, typename boost::graph_traits<Graph>::vertex_descriptor>));
		using IndexMapValue = typename boost::property_traits<IndexMap>::value_type;
		BOOST_STATIC_ASSERT((boost::is_convertible<IndexMapValue, SizeType>::value));
		// only undirected graphs for now
		BOOST_STATIC_ASSERT((boost::is_convertible<typename boost::graph_traits<Graph>::directed_category, boost::undirected_tag>::value));
		// don't have duplicate visitors
		auto visitor_with_inv = make_visitor(invariant_support(), visitor);
		using VisitorWithInv = decltype(visitor_with_inv);
		using DuplicateVisitor = detail::check_visitor_duplication<VisitorWithInv>;
		static_assert(std::is_same<DuplicateVisitor, void>::value, "Visitors must be unique.");
		// there must be a target cell selector, and only one
		static_assert(VisitorWithInv::can_select_target_cell::value, "The visitor must be able to select target cells.");
		using TargetCellSelectors = typename detail::meta_copy_if<VisitorWithInv, detail::pred_target_cell>::type;
		static_assert(detail::meta_size<TargetCellSelectors>::value == 1, "There can be only one target cell selector.");
		// there must be a tree explorer, and only one
		static_assert(VisitorWithInv::can_explore_tree::value, "The visitor must be able to explore the search tree.");
		using TreeTraversers = typename detail::meta_copy_if<VisitorWithInv, detail::pred_tree_traversal>::type;
		static_assert(detail::meta_size<TreeTraversers>::value == 1, "There can be only one tree traverser.");
		// dynamic checks
		BOOST_ASSERT_MSG(num_vertices(g) <= std::numeric_limits<SizeType>::max(), "SizeType is too narrow for this graph.");
		BOOST_ASSERT_MSG(num_edges(g) <= std::numeric_limits<SizeType>::max(), "SizeType is too narrow for this graph.");


		using Config = detail::config<SizeType, ParallelEdges, Loops, Graph, IndexMap, EdgeHandler>;
		using Permutation = typename Config::Permutation;
		using Partition = typename Config::Partition;

		if(num_vertices(g) == 0) return Permutation();
		// Create initial partition
		Partition pi(num_vertices(g));
		const auto vs = vertices(g);
		for(auto iter = vs.first; iter != vs.second; ++iter) {
			const auto v = *iter;
			pi.put_element_on_index(v, get(idx, v));
		}

		// Create and explore tree
		detail::canon_state<Config, VisitorWithInv>
				state(g, idx, edge_handler, visitor_with_inv, vertex_less, std::move(pi));
		visitor_with_inv.explore_tree(state);
		return state.get_canonical_permutation();
	}
private:
	EdgeHandler edge_handler;
};

template<
typename SizeType, bool ParallelEdges, bool Loops,
typename Graph, typename IndexMap,
typename VertexLess, typename EdgeHandler,
typename Visitor
>
std::vector<SizeType>
canonicalization(const Graph &graph, IndexMap idx, VertexLess vertex_less, EdgeHandler edge_handler, Visitor visitor) {
	return canonicalizer<SizeType, EdgeHandler, ParallelEdges, Loops>(edge_handler) (graph, idx, vertex_less, visitor);
}

// - Use graph_traits<Graph>::vertices_size_type as SizeType
// - Use default build-in index map
// - Assume all vertices are equivalent
// - Assume all edges are equivalent

template<typename Graph, bool ParallelEdges, bool Loops>
std::vector<typename boost::graph_traits<Graph>::vertices_size_type>
canonicalization(const Graph &graph) {
	using SizeType = typename boost::graph_traits<Graph>::vertices_size_type;
	return canonicalization<SizeType, ParallelEdges, Loops>(graph, get(boost::vertex_index_t(), graph), always_false(), edge_handler_all_equal(), make_visitor());
}

} // namespace graph_canon

#endif // GRAPH_CANON_CANONICALIZATION_HPP
