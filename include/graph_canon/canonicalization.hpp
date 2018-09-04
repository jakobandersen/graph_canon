#ifndef GRAPH_CANON_CANONICALIZATION_HPP
#define GRAPH_CANON_CANONICALIZATION_HPP

#include <graph_canon/tagged_list.hpp>
#include <graph_canon/util.hpp>
#include <graph_canon/edge_handler/all_equal.hpp> // default
#include <graph_canon/visitor/compound.hpp>
#include <graph_canon/refine/WL_1.hpp> // default
#include <graph_canon/invariant/coordinator.hpp> // currently always added
#include <graph_canon/detail/explicit_automorphism.hpp>
#include <graph_canon/detail/partition.hpp>
#include <graph_canon/detail/permuted_graph_view.hpp>
#include <graph_canon/detail/tree_node.hpp>

#include <perm_group/permutation/built_in.hpp>

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

// rst: .. class:: template< \
// rst:            typename SizeTypeT, bool ParallelEdgesV, bool LoopsV, \
// rst:            typename GraphT, typename IndexMapT, \
// rst:            typename EdgeHandlerT> \
// rst:            config
// rst:
// rst:		A helper-class for holding type aliases.
// rst:

template<
typename SizeTypeT, bool ParallelEdgesV, bool LoopsV,
typename GraphT, typename IndexMapT,
typename EdgeHandlerT
>
struct config {
	// rst:		.. type:: SizeType = SizeTypeT
	// rst:
	// rst:			The integer type used as element type in arrays.
	using SizeType = SizeTypeT;
	// rst:		.. var:: static constexpr bool ParallelEdges = ParallelEdgesV
	// rst:
	// rst:			Indicator for whether the graph can have parallel edges.
	static constexpr bool ParallelEdges = ParallelEdgesV;
	// rst:		.. var:: static constexpr bool Loops = LoopsV
	// rst:
	// rst:			Indicator for whether the graph can have loop edge.
	static constexpr bool Loops = LoopsV;
	// rst:		.. type:: Graph = GraphT
	// rst:
	// rst:			The graph type given as input.
	using Graph = GraphT;
	// rst:		.. type:: IndexMap = IndexMapT
	// rst:
	// rst:			The type of `boost::ReadablePropertyMap` that maps vertices to indices.
	using IndexMap = IndexMapT;
	// rst:		.. type:: EdgeHandler = EdgeHandlerT
	// rst:
	// rst:			The type of :concept:`EdgeHandler <graph_canon::EdgeHandler>` used.
	using EdgeHandler = EdgeHandlerT;
public:
	// rst:		.. type:: Partition = detail::partition<SizeType>
	// rst:
	// rst:			The type of ordered partition used.
	using Partition = detail::partition<SizeType>;
	// rst:		.. type:: Vertex = typename boost::graph_traits<Graph>::vertex_descriptor
	using Vertex = typename boost::graph_traits<Graph>::vertex_descriptor;
	// rst:		.. type:: Edge = typename boost::graph_traits<Graph>::edge_descriptor
	using Edge = typename boost::graph_traits<Graph>::edge_descriptor;
};

namespace detail {

template<typename Visitor, typename Config>
struct InstanceDataGenerator {

	template<typename TreeNode>
	struct apply {
		using type = typename Visitor::template InstanceData<Config, TreeNode>::type;
	};
};

template<typename State, typename Config>
struct TreeNodeDataGenerator {

	template<typename TreeNode>
	struct apply {
		// We want to give the tree nodes access to the state
		using Data = typename State::Vis::template TreeNodeData<Config, TreeNode>::type;

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

} // namespace detail

// rst: .. class:: template<typename ConfigT, typename VisitorT> \
// rst:            canon_state
// rst:
// rst:		A class template representing he state of a canonicalization run.
// rst:		A reference to an object of this class will be passed to almost all functions in plugins.
// rst:		Note that the user is not supposed to instantiate an object of this class manually.
// rst:

template<typename ConfigT, typename VisitorT>
class canon_state {
	using Self = canon_state<ConfigT, VisitorT>;
	using Config = ConfigT;
	canon_state(const canon_state&) = delete;
	canon_state(canon_state&&) = delete;
	canon_state &operator=(const canon_state&) = delete;
	canon_state &operator=(canon_state&&) = delete;
public:
	// rst:		.. type:: Vis
	// rst:
	// rst:			The type of the complete `Visitor` type used.

	using Vis = VisitorT;
	// rst:		.. type:: SizeType
	// rst:
	// rst:			The integer type used as element type in various arrays.
	using SizeType = typename Config::SizeType;
	// rst:		.. type:: Graph
	// rst:
	// rst:			The type of the input graph.
	using Graph = typename Config::Graph;
	// rst:		.. type:: IndexMap
	// rst:
	// rst:			The `ReadablePropertyMap` given to map vertices to indices.
	using IndexMap = typename Config::IndexMap;
	// rst:		.. type:: EHandler
	// rst:
	// rst:			The `EdgeHandler` type used.
	using EHandler = typename Config::EdgeHandler;
	// rst:		.. var:: static constexpr bool ParallelEdges
	// rst:
	// rst:			Configuration value for whether the given graph may have parallel edges or not.
	static constexpr bool ParallelEdges = Config::ParallelEdges;
	// rst:		.. var:: static constexpr bool Loops
	// rst:
	// rst:			Configuration value for whether the given graph may have loop edges or not.
	static constexpr bool Loops = Config::Loops;

	// rst:		.. type:: Partition
	// rst:
	// rst:			The type of `OrderedPartition` used in the algorithm.
	using Partition = typename Config::Partition;
	// rst:		.. type:: Vertex = typename boost::graph_traits<Graph>::vertex_descriptor
	using Vertex = typename boost::graph_traits<Graph>::vertex_descriptor;
	// rst:		.. type:: Edge = typename boost::graph_traits<Graph>::edge_descriptor
	using Edge = typename boost::graph_traits<Graph>::edge_descriptor;
	// rst:		.. type:: Perm = std::vector<SizeType>
	// rst:
	// rst:			The type of `perm_group::Permutation` returned from the algorithm
	using Perm = std::vector<SizeType>;
public:
	// rst:		.. type:: TreeNode
	// rst:
	// rst:			The type used for instantiating tree nodes.
	using TreeNode = detail::tree_node<SizeType, detail::TreeNodeDataGenerator<Self, Config> >;
	// rst:		.. type:: OwnerPtr
	// rst:
	// rst:			A smart-pointer type that keeps a tree node alive (e.g., by reference counting).
	using OwnerPtr = typename TreeNode::OwnerPtr;
	// rst:		.. type:: InstanceData
	// rst:
	// rst:			Instance data defined, e.g., by visitors, is all stored in an object of this type
	using InstanceData = typename detail::InstanceDataGenerator<Vis, Config>::template apply<TreeNode>::type;
private:
	using PermutedGraph = detail::permuted_graph_view<Config, TreeNode>;
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
			pi.set_cell_from_v_idx(cell);
	}
public:

	template<typename VertexLess>
	canon_state(const Graph &g, IndexMap idx, EHandler &edge_handler, Vis visitor, VertexLess vertex_less, Partition &&pi)
	: g(g), n(num_vertices(g)), idx(idx), edge_handler(edge_handler), visitor(std::move(visitor)) {
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

	//private:

	bool make_equitable(TreeNode &node) {
		// returns false iff this leads to a worse leaf
		RefinementResult result;
		do {
			result = visitor.refine(*this, node);
		} while(result == RefinementResult::Again);
		return result != RefinementResult::Abort;
	}

	Perm get_canonical_permutation() {
		assert(canon_leaf);
		Perm p(n);
		std::copy(canon_leaf->pi.begin_inverse(), canon_leaf->pi.end_inverse(), p.begin());
		return p;
	}

	SizeType select_target_cell(TreeNode &t) {
		return visitor.select_target_cell(*this, t);
	}
public:

	// rst:		.. function:: void report_leaf(OwnerPtr node)
	// rst:
	// rst:			Submit a tree node as a valid leaf of the search tree.
	// rst:
	// rst:			Requires that the ordered partition represented by `node` is discrete,
	// rst:			and that `report_leaf` has not been called before with this node.
	// rst:
	// rst:			Several `Visitor` methods may be called from this function:
	// rst:
	// rst:			- `Visitor::tree_leaf`, always called.
	// rst:			- `Visitor::canon_new_best`, if this node will be the best candidate leaf afterwards.
	// rst:			- `Visitor::automorphism_leaf`, if this node represents a canonical representation,
	// rst:			  equal to the current best candidate.
	// rst:			- `Visitor::canon_worse`, if this nodes represents a worse representation than the current best candidate.

	void report_leaf(OwnerPtr node) {
		assert(node != canon_leaf);
		assert(node->pi.get_num_cells() == n);
		visitor.tree_leaf(*this, *node);
		if(!canon_leaf) { // canon_permuted_graph may still be valid if someone pruned our canon_leaf
			canon_leaf = node;
			visitor.canon_new_best(*this, static_cast<TreeNode*>(nullptr));
			return;
		}
		if(!canon_permuted_graph) { // this is our second leaf, so not even the first permuted graph were created
			assert(!extra_permuted_graph);
			canon_permuted_graph = new PermutedGraph(*this, canon_leaf);
			extra_permuted_graph = new PermutedGraph(*this, node);
		} else if(canon_permuted_graph->get_node() != canon_leaf) {
			// at least two leaves were discovered, canon_leaf was pruned, at least two leaves were discovered
			canon_permuted_graph->repermute(*this, canon_leaf);
		}
		if(extra_permuted_graph) {
			extra_permuted_graph->repermute(*this, node);
		} else {
			extra_permuted_graph = new PermutedGraph(*this, node);
		}
		auto cmp = PermutedGraph::compare(*this, *extra_permuted_graph, *canon_permuted_graph);
		if(cmp < 0) {
			std::swap(canon_permuted_graph, extra_permuted_graph);
			OwnerPtr previous = canon_leaf;
			canon_leaf = node;
			visitor.canon_new_best(*this, previous.get());
		} else if(cmp == 0) {
			detail::explicit_automorphism<Self> aut(*this, *node);
			visitor.automorphism_leaf(*this, *node, aut);
		} else {
			visitor.canon_worse(*this, *node);
		}
	}

	// rst:		.. function:: void prune_canon_leaf()
	// rst:
	// rst:			Prune the current best candidate.
	// rst:			Requires that at least one leaf will be added with `repoort_leaf` later.

	void prune_canon_leaf() {
		if(!canon_leaf) return;
		visitor.canon_prune(*this);
		canon_leaf = nullptr;
	}

	// rst:		.. function:: TreeNode *get_canon_leaf() const
	// rst:
	// rst:			:returns: a pointer to the tree node representing the current best candidate.

	TreeNode *get_canon_leaf() const {
		return canon_leaf.get();
	}
public:
	// rst:		.. var:: const Graph &g
	// rst:
	// rst:			The input graph.
	const Graph &g;
	// rst:		.. var:: const SizeType n = num_vertices(g)
	const SizeType n;
	// rst:		.. var:: const IndexMap idx
	// rst:
	// rst:			The given `ReadablePropertyMap` that maps vertices to indices.
	const IndexMap idx;
	// rst:		.. var:: InstanceData data
	// rst:
	// rst:			The aggregated data structure holding all instance data.
	// rst:			Use `get(my_tag(), data)` to access your data, tagged with some tag `my_tag`.
	InstanceData data; // should be the last calculated data to be deleted
	// rst:		.. var:: EHandler &edge_handler
	// rst:
	// rst:			A reference to the `EdgeHandler` used for this run.
	EHandler &edge_handler; // stored in the canonicalisation object, to reuse it
	// rst:		.. var:: Vis visitor
	// rst:
	// rst:			The complete `Visitor` used.
	Vis visitor;
	// rst:		.. var:: OwnerPtr root
	// rst:
	// rst:			A pointer to the root of the search tree.
	// rst:			Note that this pointer is only set after the root has been fully
	// rst:			constructed. Thus, in `Visitor` methods, if `root` is `nullptr`,
	// rst:			then you have probably been given a reference to the root as the current tree node.
	OwnerPtr root = nullptr;
private:
	OwnerPtr canon_leaf = nullptr; // best graph of all the best_by_invariant
	PermutedGraph *canon_permuted_graph = nullptr, *extra_permuted_graph = nullptr; // has owner pointers to their leaves
};

// rst: .. class:: template<typename SizeType, typename EdgeHandlerCreatorT, bool ParallelEdges, bool Loops> \
// rst:            canonicalizer
// rst:
// rst:		A reusable function object for canonicalizing graphs.
// rst:
// rst:		Requires `SizeType` to be an integer type and `EdgeHandlerCreatorT` to be an `EdgeHandlerCreator`.
// rst:

template<typename SizeType, typename EdgeHandlerCreatorT, bool ParallelEdges, bool Loops>
struct canonicalizer {
	BOOST_STATIC_ASSERT_MSG(boost::is_integral<SizeType>::value, "SizeType must be integral.");
	// rst:		.. type:: EdgeHandler = typename EdgeHandlerCreatorT::template type<SizeType>
	using EdgeHandler = typename EdgeHandlerCreatorT::template type<SizeType>;
public:

	// rst:		.. function:: canonicalizer(EdgeHandlerCreatorT edge_handler_creator)

	canonicalizer(EdgeHandlerCreatorT edge_handler_creator)
	: edge_handler(edge_handler_creator.template make<SizeType>()) { }

	// rst:		.. function:: template<typename Graph, typename IndexMap, typename VertexLess, typename Vis> \
	// rst:		              auto operator()(const Graph &g, IndexMap idx, VertexLess vertex_less, Vis visitor)
	// rst:
	// rst:			Compute a permutation that permutes the indices of the vertices of `g` into their canonical indices.
	// rst:
	// rst:			Requires:
	// rst:
	// rst:			- `IndexMap` must be a `ReadablePropertyMap` that maps the vertices of `g` into contiguous indices starting from 0.
	// rst:			- `VertexLess` must be a less-than predicate on the vertices of `g` that induces a strict weak ordering.
	// rst:			  The resulting canonical vertex order respects the ordering induced by `vertex_less`.
	// rst:			- `Vis` must be a `Visitor` type.
	// rst:
	// rst:			:returns: A `std::pair<std::vector<SizeType>, Data>` where `first` is the permutation,
	// rst:				and `second` is the auxiliary visitor data of an unspecified type `Data`.
	// rst:				See each visitor for what data they may return and what it is tagged with.
	// rst:				Use `get(the_tag(), res.second)` to access the data tagged with `the_tag` from the return value `res`.

	template<typename Graph, typename IndexMap, typename VertexLess, typename Vis>
	auto operator()(const Graph &g, IndexMap idx, VertexLess vertex_less, Vis visitor) {
		// static checks
		BOOST_CONCEPT_ASSERT((boost::ReadablePropertyMapConcept<IndexMap, typename boost::graph_traits<Graph>::vertex_descriptor>));
		using IndexMapValue = typename boost::property_traits<IndexMap>::value_type;
		BOOST_STATIC_ASSERT((boost::is_convertible<IndexMapValue, SizeType>::value));
		// only undirected graphs for now
		BOOST_STATIC_ASSERT((boost::is_convertible<typename boost::graph_traits<Graph>::directed_category, boost::undirected_tag>::value));
		// don't have duplicate visitors
		auto visitor_with_inv = make_visitor(invariant_coordinator(), visitor);
		using VisitorWithInv = decltype(visitor_with_inv);
		using DuplicateVisitor = detail::check_visitor_duplication<VisitorWithInv>;
		static_assert(std::is_same<DuplicateVisitor, void>::value, "Visitors must be unique.");
		// there must be a target cell selector, and only one
		static_assert(VisitorWithInv::can_select_target_cell::value, "The visitor must be able to select target cells.");
		using TargetCellSelectors = meta::copy_if<VisitorWithInv, detail::pred_target_cell>;
		static_assert(meta::size<TargetCellSelectors>::value == 1, "There can be only one target cell selector.");
		// there must be a tree explorer, and only one
		static_assert(VisitorWithInv::can_explore_tree::value, "The visitor must be able to explore the search tree.");
		using TreeTraversers = meta::copy_if<VisitorWithInv, detail::pred_tree_traversal>;
		static_assert(meta::size<TreeTraversers>::value == 1, "There can be only one tree traverser.");
		// dynamic checks
		BOOST_ASSERT_MSG(num_vertices(g) <= std::numeric_limits<SizeType>::max(), "SizeType is too narrow for this graph.");
		BOOST_ASSERT_MSG(num_edges(g) <= std::numeric_limits<SizeType>::max(), "SizeType is too narrow for this graph.");

		
		using Config = config<SizeType, ParallelEdges, Loops, Graph, IndexMap, EdgeHandler>;
		using Partition = typename Config::Partition;

		// Create initial partition
		Partition pi(num_vertices(g));
		const auto vs = vertices(g);
		for(auto iter = vs.first; iter != vs.second; ++iter) {
			const auto v = *iter;
			pi.put_element_on_index(v, get(idx, v));
		}

		// Create and explore tree
		canon_state<Config, VisitorWithInv>
				state(g, idx, edge_handler, visitor_with_inv, vertex_less, std::move(pi));
		visitor_with_inv.explore_tree(state);
		return std::make_pair(state.get_canonical_permutation(), visitor_with_inv.extract_result(state));
	}
private:
	EdgeHandler edge_handler;
};

// rst: .. function:: template<typename SizeType, bool ParallelEdges, bool Loops, typename Graph, typename IndexMap, \
// rst:               typename VertexLess, typename EdgeHandlerCreatorT, typename Visitor> \
// rst:               auto canonicalize(const Graph &graph, IndexMap idx, VertexLess vertex_less, EdgeHandlerCreatorT edge_handler_creator, Visitor visitor)
// rst:
// rst:		A shorhand function for canonicalization.
// rst:		The `SizeType`, `ParallelEdges`, and `Loops` must be specified explicitly while remaining template parameters can be deduced.
// rst:
// rst:		See :expr:`canonicalizer::operator()`.

template<
typename SizeType, bool ParallelEdges, bool Loops,
typename Graph, typename IndexMap,
typename VertexLess, typename EdgeHandlerCreatorT,
typename Visitor
>
auto canonicalize(const Graph &graph, IndexMap idx, VertexLess vertex_less, EdgeHandlerCreatorT edge_handler_creator, Visitor visitor) {
	return canonicalizer<SizeType, EdgeHandlerCreatorT, ParallelEdges, Loops>(edge_handler_creator) (graph, idx, vertex_less, visitor);
}

} // namespace graph_canon

#endif // GRAPH_CANON_CANONICALIZATION_HPP
