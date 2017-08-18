#ifndef GRAPH_CANON_EDGE_HANDLER_ALL_EQUAL_HPP
#define GRAPH_CANON_EDGE_HANDLER_ALL_EQUAL_HPP

#include <graph_canon/sorting_utils.hpp>

#include <algorithm>
#include <vector>

namespace graph_canon {

template<typename SizeType>
struct edge_handler_all_equal_impl {
	static constexpr SizeType Max = 256;

	void initialize(const auto &state) { }

	//========================================================================
	// Generic Refiner
	//========================================================================

	void add_edge(auto &state, auto &node, const SizeType cell, const SizeType cell_end, const auto &e_out) { }

	template<bool ParallelEdges, bool Loops, typename Partition>
	void sort_equal_hit(Partition &pi, const SizeType cell, const SizeType cell_end,
			const SizeType max, auto &splits) { }

	template<bool ParallelEdges, bool Loops, typename Partition>
	void sort_partitioned(Partition &pi, const SizeType cell, const SizeType cell_mid, const SizeType cell_end,
			const SizeType max_count, auto &splits) { }

	template<bool ParallelEdges, bool Loops, typename Partition>
	void sort_duplicate_partitioned_equal_hit(Partition &pi, const SizeType cell, const SizeType cell_mid, const SizeType cell_end,
			const SizeType max, auto &splits) { }

	template<bool ParallelEdges, bool Loops, typename Partition>
	void sort_duplicate_partitioned(Partition &pi, const SizeType cell, const SizeType cell_mid, const SizeType cell_end,
			const SizeType max, const SizeType max_count,
			const auto &counters, auto &splits) {
		sort_range<ParallelEdges, Loops>(pi, cell_mid, cell_end, max, counters, splits);
	}

	template<bool ParallelEdges, bool Loops, typename Partition>
	void sort(Partition &pi, const SizeType cell, const SizeType cell_end,
			const SizeType max, const SizeType max_count, SizeType &first_non_zero,
			const auto &counters, auto &splits) {
		assert(max_count != cell_end - cell); // handled by refiner
		assert(max > 1); // handled by refiner
		first_non_zero = sort_range<ParallelEdges, Loops>(pi, cell, cell_end, max, counters, splits);
	}
private:

	template<bool ParallelEdges, bool Loops, typename Partition>
	SizeType sort_range(Partition &pi, const SizeType idx_first, const SizeType idx_last,
			const SizeType max,
			const auto &counters, auto &splits) {
		const auto first = pi.begin() + idx_first;
		const auto last = pi.begin() + idx_last;
		// use counting sort to sort for low max_count
		if(max < Max) {
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

	void clear_cell(auto &state, auto &node, const SizeType cell, const SizeType cell_end) { }

	void clear_cell_aborted(auto &state, auto &node, const SizeType cell, const SizeType cell_end) { }

	//========================================================================
	// Singleton Refiner
	//========================================================================

	void add_edge_singleton_refiner(auto &state, auto &node, const SizeType cell, const SizeType cell_end, const auto &e_out, const SizeType target_pos) { }

	template<bool ParallelEdges, bool Loops, typename Partition, typename Splits>
	void sort_singleton_refiner(Partition &pi, const SizeType cell, const SizeType cell_mid, const SizeType cell_end, Splits &splits) { }

	void clear_cell_singleton_refiner(auto &state, auto &node, const SizeType cell, const SizeType cell_end) { }

	void clear_cell_singleton_refiner_aborted(auto &state, auto &node, const SizeType cell, const SizeType cell_end) { }
private:
	counting_sorter<SizeType, Max> sorter;
public:

	long long compare(auto &state, const auto e_left, const auto e_right) const {
		return 0;
	}
};

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