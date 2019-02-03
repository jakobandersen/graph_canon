#ifndef GRAPH_CANON_EDGE_HANDLER_ALL_EQUAL_HPP
#define GRAPH_CANON_EDGE_HANDLER_ALL_EQUAL_HPP

#include <graph_canon/sorting_utils.hpp>

#include <algorithm>
#include <vector>

namespace graph_canon {

// rst: .. class:: template<typename SizeType> \
// rst:            edge_handler_all_equal_impl
// rst:
// rst:		An `EdgeHandler` for edges without labels.
// rst:

template<typename SizeType>
struct edge_handler_all_equal_impl {
	static constexpr SizeType Max = 256;

	template<typename State>
	void initialize(const State &state) { }

	//========================================================================
	// Generic Refiner
	//========================================================================

	template<typename State, typename TreeNode, typename Edge>
	void add_edge(State &state, TreeNode &node, const SizeType cell, const SizeType cell_end, const Edge &e_out) { }

	template<bool ParallelEdges, bool Loops, typename Partition, typename Splits>
	void sort_equal_hit(Partition &pi, const SizeType cell, const SizeType cell_end,
			const SizeType max, Splits &splits) { }

	template<bool ParallelEdges, bool Loops, typename Partition, typename Splits>
	void sort_partitioned(Partition &pi, const SizeType cell, const SizeType cell_mid, const SizeType cell_end,
			const SizeType max_count, Splits &splits) { }

	template<bool ParallelEdges, bool Loops, typename Partition, typename Splits>
	void sort_duplicate_partitioned_equal_hit(Partition &pi, const SizeType cell, const SizeType cell_mid, const SizeType cell_end,
			const SizeType max, Splits &splits) { }

	template<bool ParallelEdges, bool Loops, typename Partition, typename Counters, typename Splits>
	void sort_duplicate_partitioned(Partition &pi, const SizeType cell, const SizeType cell_mid, const SizeType cell_end,
			const SizeType max, const SizeType max_count,
			const Counters &counters, Splits &splits) {
		sort_range<ParallelEdges, Loops>(pi, cell_mid, cell_end, max, counters, splits);
	}

	template<bool ParallelEdges, bool Loops, typename Partition, typename Counters, typename Splits>
	void sort(Partition &pi, const SizeType cell, const SizeType cell_end,
			const SizeType max, const SizeType max_count, SizeType &first_non_zero,
			const Counters &counters, Splits &splits) {
		assert(max_count != cell_end - cell); // handled by refiner
		assert(max > 1); // handled by refiner
		first_non_zero = sort_range<ParallelEdges, Loops>(pi, cell, cell_end, max, counters, splits);
	}
private:

	template<bool ParallelEdges, bool Loops, typename Partition, typename Counters, typename Splits>
	SizeType sort_range(Partition &pi, const SizeType idx_first, const SizeType idx_last,
			const SizeType max,
			const Counters &counters, Splits &splits) {
		const auto first = pi.begin() + idx_first;
		const auto last = pi.begin() + idx_last;
		// use counting sort to sort for low max_count
		if(max < Max) {
			const auto do_sort = [&](auto &sorter) {
				SizeType first_split;
				sorter(first, last, [&counters](const SizeType i) {
					return counters[i];
				}, [&splits, idx_first, idx_last, &first_split](const auto &end) {
					first_split = idx_first + end.front();
					auto prev = 0;
					const auto cell_size = idx_last - idx_first;
					for(auto iter = end.begin(); *iter != cell_size; prev = *iter, ++iter) {
						if(*iter == prev) {
							continue;
						}
						assert(*iter > prev);
								splits.push_back(idx_first + *iter);
					}
				}, [&pi](auto iter, const auto value) {
					pi.put_element_on_index(value, iter - pi.begin());
				});
				return first_split;
			};
			if(max < 4) return do_sort(sorter4);
			else return do_sort(sorter);
		}
		// fallback, just sort
		std::sort(first, last, [&counters](const auto a, const auto b) {
			return counters[a] < counters[b];
		});
		assert(counters[*first] != counters[*(last - 1)]);
		pi.reset_inverse(idx_first, idx_last);
		// Scan the refinee invariants and split it if needed, save new subset beginnings.
		for(SizeType i_refinee = idx_first + 1; i_refinee < idx_last; i_refinee++) {
			const SizeType refinee_prev_idx = pi.get(i_refinee - 1);
			const SizeType refinee_idx = pi.get(i_refinee);
			if(counters[refinee_prev_idx] < counters[refinee_idx])
				splits.push_back(i_refinee);
		}
		return idx_first; // TODO: we could check this
	}
public:

	template<typename State, typename TreeNode>
	void clear_cell(State &state, TreeNode &node, const SizeType cell, const SizeType cell_end) { }

	template<typename State, typename TreeNode>
	void clear_cell_aborted(State &state, TreeNode &node, const SizeType cell, const SizeType cell_end) { }

	//========================================================================
	// Singleton Refiner
	//========================================================================

	template<typename State, typename TreeNode, typename Edge>
	void add_edge_singleton_refiner(State &state, TreeNode &node, const SizeType cell, const SizeType cell_end, const Edge &e_out, const SizeType target_pos) { }

	template<bool ParallelEdges, bool Loops, typename Partition, typename Splits>
	void sort_singleton_refiner(Partition &pi, const SizeType cell, const SizeType cell_mid, const SizeType cell_end, Splits &splits) { }

	template<typename State, typename TreeNode>
	void clear_cell_singleton_refiner(State &state, TreeNode &node, const SizeType cell, const SizeType cell_end) { }

	template<typename State, typename TreeNode>
	void clear_cell_singleton_refiner_aborted(State &state, TreeNode &node, const SizeType cell, const SizeType cell_end) { }
private:
	counting_sorter<SizeType, 4> sorter4;
	counting_sorter<SizeType, Max> sorter;
public:

	template<typename State, typename Edge>
	long long compare(State &state, const Edge &e_left, const Edge &e_right) const {
		return 0;
	}
};

// rst: .. class:: edge_handler_all_equal
// rst:
// rst:		An `EdgeHandlerCreator` for the `edge_handler_all_equal_impl` `EdgeHandler` class.
// rst:

struct edge_handler_all_equal {
	template<typename SizeType>
	using type = edge_handler_all_equal_impl<SizeType>;

	template<typename SizeType>
	type<SizeType> make() {
		return {};
	}
};

} // namespace graph_canon

#endif /* GRAPH_CANON_EDGE_HANDLER_ALL_EQUAL_HPP */