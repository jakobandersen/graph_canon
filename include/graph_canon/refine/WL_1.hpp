#ifndef GRAPH_CANON_REFINE_WL_1_HPP
#define GRAPH_CANON_REFINE_WL_1_HPP

#include <graph_canon/sorting_utils.hpp>
#include <graph_canon/visitor/compound.hpp>

#include <boost/dynamic_bitset.hpp>

#include <cassert>
#include <utility>
#include <vector>

namespace graph_canon {

struct refine_WL_1 : null_visitor {
	static constexpr std::size_t refine_new_cell_type = 50;
public:

	enum struct refinee_type {
		unique_partitioning, equal_hit, duplicate_partitioning, sort
	};

	struct instance_data_t {
	};

	template<typename SizeType>
	struct instance_data {

		struct cell_data_ { // Various data, not inter-related.
			// Whether the cell is in the refiner queue.
			bool is_refiner = false;
			// The number of edges from a fixed refiner cell to each refinee cell.
			SizeType hit_count = 0;
			// For refinees of non-singleton refiners
			// -------------------------------------------
			// The max of all counters in the cell.
			SizeType max = 0;
			// How many counters have that max.
			SizeType max_count = 0;
			// How many counters have non-zero
			SizeType non_zero_count = 0;
			// We don't want to reset counters that don't need it.
			// This is at least refinee_begin, but may be set higher for skip 'zero' counters.
			SizeType first_non_zero;
			// How to handle the sorting of the cell
			refinee_type type;
		};

		struct refinee_cell {

			refinee_cell(SizeType first, SizeType last) : first(first), last(last) { }
		public:
			SizeType first, last;
		};
	public: // All this data must always be reset after use.
		// Queue of cells to use as refiner.
		std::vector<SizeType> refiner_cells;
		// The split positions collected at counting/sorting/scanning time.
		std::vector<SizeType> refined_beginnings;
		// The cells that contain neighbours of vertices in the refiner cell.
		std::vector<refinee_cell> refinee_cells;
		// Group of various data.
		std::vector<cell_data_> cell_data;
		// Edge counters
		std::vector<SizeType> counters;
	};

	template<typename Config, typename TreeNode>
	struct InstanceData {
		using type = tagged_list<instance_data_t, instance_data<typename Config::SizeType> >;
	};

	template<typename State>
	void initialize(State &state) {
		const auto n = state.n;
		auto &data = get(instance_data_t(), state.data);
		data.refiner_cells.reserve(n);
		data.refined_beginnings.reserve(n);
		data.refinee_cells.reserve(n);
		data.cell_data.resize(n);
		data.counters.resize(n);
	}

	template<typename State, typename TreeNode>
	RefinementResult refine(State &state, TreeNode &node) {
		using SizeType = typename State::SizeType;

		auto &i_data = get(instance_data_t(), state.data);
		auto &refiner_cells = i_data.refiner_cells;
		auto &cell_data = i_data.cell_data;

		RefinementResult result = RefinementResult::Unchanged;

		add_initial_refiner_cells(state, node);

		// Use each of the specified refiner cells to refine each cell.
		const auto clear_remaining_refiners = [&](auto i_refiner) {
			for(++i_refiner; i_refiner < refiner_cells.size(); ++i_refiner)
				cell_data[refiner_cells[i_refiner]].is_refiner = false;
		};
		// Don't use iterators as we add more refiner cells along the way.
		for(SizeType i_refiner = 0; i_refiner < refiner_cells.size(); ++i_refiner) {
			const auto refiner_begin = refiner_cells[i_refiner];
			const auto refiner_end = node.pi.get_cell_end(refiner_begin);
			const bool is_refiner_singleton = refiner_begin + 1 == refiner_end;
			cell_data[refiner_begin].is_refiner = false;

			const bool continue_ = [&]() {
				if(is_refiner_singleton && !State::ParallelEdges && !State::Loops)
					return refine_with_singleton_cell(state, node, refiner_begin, refiner_end, result);
				else
					return refine_with_cell(state, node, refiner_begin, refiner_end, result);
			}();

			if(!continue_) {
				clear_remaining_refiners(i_refiner);
				return RefinementResult::Abort;
			}
			if(node.pi.get_num_cells() == state.n) {
				clear_remaining_refiners(i_refiner);
				break;
			}
		}
		return result;
	}
private:

	template<typename State>
	void add_initial_refiner_cells(State &state, const auto &node) {
		using SizeType = typename State::SizeType;
		auto &i_data = get(instance_data_t(), state.data);
		auto &refiner_cells = i_data.refiner_cells;

		refiner_cells.clear();
		if(!node.get_parent()) {
			// the root, add all cells
			for(SizeType i = 0; i < state.n; i = node.pi.get_cell_end(i))
				refiner_cells.push_back(i);
		} else {
			// Thm. 2-7 and Sec. 2-9 of PGI:
			// just refine using the newly created individualized vertex
			refiner_cells.push_back(node.get_parent()->child_refiner_cell);
		}
		// mark them
		for(const auto refiner : refiner_cells)
			i_data.cell_data[refiner].is_refiner = true;
	}

	template<typename State>
	bool refine_with_singleton_cell(State &state, auto &node, const auto refiner_begin, const auto refiner_end, RefinementResult &result) {
		using SizeType = typename State::SizeType;

		auto &sorter = state.edge_handler;

		auto &i_data = get(instance_data_t(), state.data);
		auto &refinee_cells = i_data.refinee_cells;
		auto &cell_data = i_data.cell_data;

		refinee_cells.clear();
		// count neighbours
		count_neighbours_from_singleton(state, node, refiner_begin);

		// the refinee_cells were added in whatever order the vertex/edge iteration determined
		std::sort(refinee_cells.begin(), refinee_cells.end(), [](auto a, auto b) {
			return a.first < b.first;
		});

		const auto clear_refinee_data = [&](const auto refinee_mid) {
			for(const auto rp : refinee_cells)
				cell_data[rp.first].hit_count = 0;
			for(auto iter = refinee_cells.begin(); iter != refinee_mid; ++iter) {
				sorter.clear_cell_singleton_refiner(state, node, iter->first, iter->last);
			}
			for(auto iter = refinee_mid; iter != refinee_cells.end(); ++iter) {
				sorter.clear_cell_singleton_refiner_aborted(state, node, iter->first, iter->last);
			}
		};
		// cache the end in the start of each iteration so we don't try to refine the same area multiple times
		const auto refinee_last = refinee_cells.end();
		for(auto refinee_iter = refinee_cells.begin(); refinee_iter != refinee_last; ++refinee_iter) {
			const auto refinee_begin = refinee_iter->first;
			const auto refinee_end = refinee_iter->last;
			assert(refinee_begin <= refinee_end);
			assert(refinee_end - refinee_begin >= 2);
			const bool continue_ = handle_refinee(state, node, refinee_begin, refinee_end, refiner_begin, std::true_type(), result);
			if(!continue_) {
				// some cleanup
				clear_refinee_data(refinee_iter + 1);
				// (we don't need to set cell beginnings, they are per tree node, so we can trash them)
				return false;
			}
		} // for each refinee
		clear_refinee_data(refinee_cells.end());
		const auto continue_ = state.visitor.refine_refiner_done(state, node, refiner_begin, refiner_end);
		if(!continue_)
			return false;
		reset_cell_beginnings(state, node);
		return true;
	}

	template<typename State>
	bool refine_with_cell(State &state, auto &node, const auto refiner_begin, const auto refiner_end, RefinementResult &result) {
		using SizeType = typename State::SizeType;

		auto &sorter = state.edge_handler;

		auto &i_data = get(instance_data_t(), state.data);
		auto &refinee_cells = i_data.refinee_cells;
		auto &data = i_data.cell_data;
		auto &counters = i_data.counters;

		refinee_cells.clear();
		// count neighbours
		count_neighbours_from_cell(state, node, refiner_begin, refiner_end);

		// the refinee_cells were added in whatever order the vertex/edge iteration determined
		std::sort(refinee_cells.begin(), refinee_cells.end(), [](auto a, auto b) {
			return a.first < b.first;
		});

		// let's analyse he refinees, maybe we should redo the edges and do some online partitioning
		bool redo_neighbours = false;
		for(const auto &rp : refinee_cells) {
			const auto max = data[rp.first].max;
			const auto max_count = data[rp.first].max_count;
			if(max_count == rp.last - rp.first) { // have all been hit the same?
				data[rp.first].type = refinee_type::equal_hit;
			} else if(max == 1) {
				data[rp.first].type = refinee_type::unique_partitioning;
				data[rp.first].hit_count = 0;
				redo_neighbours = true;
			} else {
				const auto cell_size = rp.last - rp.first;
				//				const auto non_zero = data[rp.first].non_zero_count;
				// TODO: what should the heuristic be here?
				// let's say if we have at most 75% non-zero, then do partition
				if(data[rp.first].hit_count * 4 < 3 * cell_size) {
					data[rp.first].type = refinee_type::duplicate_partitioning;
					data[rp.first].hit_count = 0;
					redo_neighbours = true;
				} else {
					data[rp.first].type = refinee_type::sort;
				}
			}
		}
		if(redo_neighbours) {
			redo_neighbours_from_cell(state, node, refiner_begin, refiner_end);
		}

		const auto clear_refinee_data = [&](const auto refinee_mid) {
			for(const auto rp : refinee_cells) {
				data[rp.first].hit_count = 0;
				data[rp.first].max = 0;
				data[rp.first].max_count = 0;
				data[rp.first].non_zero_count = 0;
			}
			for(auto iter = refinee_cells.begin(); iter != refinee_mid; ++iter) {
				const auto cell = iter->first;
				const auto cell_end = iter->last;
				const auto first = node.pi.begin() + data[cell].first_non_zero;
				const auto last = node.pi.begin() + cell_end;
				for(auto iter = first; iter != last; ++iter)
					counters[*iter] = 0;
				sorter.clear_cell(state, node, cell, cell_end);
			}
			for(auto iter = refinee_mid; iter != refinee_cells.end(); ++iter) {
				const auto cell = iter->first;
				const auto cell_end = iter->last;
				const auto first = node.pi.begin() + cell;
				const auto last = node.pi.begin() + cell_end;
				for(auto iter = first; iter != last; ++iter)
					counters[*iter] = 0;
				sorter.clear_cell_aborted(state, node, cell, cell_end);
			}
		};
		// cache the end in the start of each iteration so we don't try to refine the same area multiple times
		const auto refinee_last = refinee_cells.end();
		for(auto refinee_iter = refinee_cells.begin(); refinee_iter != refinee_last; ++refinee_iter) {
			const auto refinee_begin = refinee_iter->first;
			const auto refinee_end = refinee_iter->last;
			assert(refinee_begin <= refinee_end);
			assert(refinee_end - refinee_begin >= 2);
			const bool continue_ = handle_refinee(state, node, refinee_begin, refinee_end, refiner_begin, std::false_type(), result);
			if(!continue_) {
				// some cleanup
				clear_refinee_data(refinee_iter + 1);
				// (we don't need to set cell beginnings, they are per tree node, so we can trash them)
				return false;
			}
		} // for each refinee
		clear_refinee_data(refinee_cells.end());
		// note: we could have refined our selfs, so use the provided refiner_end
		const auto continue_ = state.visitor.refine_refiner_done(state, node, refiner_begin, refiner_end);
		if(!continue_)
			return false;
		reset_cell_beginnings(state, node);
		return true;
	}

	template<typename State>
	void count_neighbours_from_singleton(State &state, auto &node, const auto refiner_begin) {
		assert(!State::ParallelEdges);
		assert(!State::Loops);

		auto &sorter = state.edge_handler;
		auto &i_data = get(instance_data_t(), state.data);
		auto &refinee_cells = i_data.refinee_cells;
		auto &cell_data = i_data.cell_data;

		auto &pi = node.pi;

		const auto v = vertex(*(pi.begin() + refiner_begin), state.g);
		for_each_neighbour(state, node, v, [&](const auto e_out, const auto v_pos, const auto cell, const auto cell_end) {
			auto &hit_count = cell_data[cell].hit_count;
			const bool has_been_hit = hit_count != 0;
			if(!has_been_hit) {
				refinee_cells.emplace_back(cell, cell_end);
			}
			++hit_count;
			assert(v_pos >= cell);
			assert(v_pos < cell_end);
			assert(hit_count <= cell_end - cell);
			const auto target_pos = cell_end - hit_count;
			if(v_pos != target_pos) { // don't do identity swaps
				const auto elem = pi.get(v_pos);
						const auto target_elem = pi.get(target_pos);
						pi.put_element_on_index(elem, target_pos);
						pi.put_element_on_index(target_elem, v_pos);
			}
			sorter.add_edge_singleton_refiner(state, node, cell, cell_end, e_out, target_pos);
		});
	}

	template<typename State>
	void count_neighbours_from_cell(State &state, auto &node, const auto refiner_begin, const auto refiner_end) {
		auto &sorter = state.edge_handler;
		auto &i_data = get(instance_data_t(), state.data);
		auto &refinee_cells = i_data.refinee_cells;
		auto &data = i_data.cell_data;
		auto &counters = i_data.counters;

		auto &pi = node.pi;

		const auto refiner_last = pi.begin() + refiner_end;
		for(auto v_iter = pi.begin() + refiner_begin; v_iter != refiner_last; ++v_iter) {
			const auto v = vertex(*v_iter, state.g);
			for_each_neighbour(state, node, v, [&](const auto e_out, const auto v_pos, const auto cell, const auto cell_end) {
				auto &hit_count = data[cell].hit_count;
				const bool has_been_hit = hit_count != 0;
				if(!has_been_hit) {
					refinee_cells.emplace_back(cell, cell_end);
				}
				++hit_count;
				const auto v_target = target(e_out, state.g);
				const auto v_idx = state.idx[v_target];
				auto &c = counters[v_idx];
				if(c == 0) {
					++data[cell].non_zero_count;
				}
				++c;
				if(c == data[cell].max) {
					++data[cell].max_count;
				} else if(c > data[cell].max) {
					data[cell].max = c;
							data[cell].max_count = 1;
				}
				sorter.add_edge(state, node, cell, cell_end, e_out);
			});
		}
	}

	template<typename State>
	void redo_neighbours_from_cell(State &state, auto &node, const auto refiner_begin, const auto refiner_end) {
		auto &i_data = get(instance_data_t(), state.data);
		auto &data = i_data.cell_data;

		auto &pi = node.pi;

		const auto refiner_last = pi.begin() + refiner_end;
		for(auto refiner_iter = pi.begin() + refiner_begin; refiner_iter != refiner_last; ++refiner_iter) {
			const auto v = vertex(*refiner_iter, state.g);
			for_each_neighbour(state, node, v, [&](const auto e_out, const auto v_pos, const auto cell, const auto cell_end) {
				auto &hit_count = data[cell].hit_count;
				switch(data[cell].type) {
				case refinee_type::unique_partitioning:
				{
					++hit_count;
							assert(v_pos >= cell);
							assert(v_pos < cell_end);
							assert(hit_count <= cell_end - cell);
							const auto target_pos = cell_end - hit_count;
					if(v_pos != target_pos) { // don't do identity swaps
						const auto elem = pi.get(v_pos);
								const auto target_elem = pi.get(target_pos);
								pi.put_element_on_index(elem, target_pos);
								pi.put_element_on_index(target_elem, v_pos);
					}
				}
					break;
				case refinee_type::equal_hit:
					break;
				case refinee_type::duplicate_partitioning:
				{
					assert(v_pos >= cell);
							assert(v_pos < cell_end);
							assert(hit_count <= cell_end - cell);
					if(v_pos >= cell_end - hit_count) {
						// duplicate
					} else {
						++hit_count;
								assert(hit_count <= cell_end - cell);
								const auto target_pos = cell_end - hit_count;
						if(v_pos != target_pos) { // don't do identity swaps
							const auto elem = pi.get(v_pos);
									const auto target_elem = pi.get(target_pos);
									pi.put_element_on_index(elem, target_pos);
									pi.put_element_on_index(target_elem, v_pos);
						}
					}
				}
					break;
				case refinee_type::sort:
					break;
				}
			});
		}
	}

	template<typename State>
	bool handle_refinee(State &state, auto &node, const auto cell, const auto cell_end,
			const auto refiner_begin, const auto is_refiner_singleton, RefinementResult &result) {
		using SizeType = typename State::SizeType;
		constexpr bool ParallelEdges = State::ParallelEdges;
		constexpr bool Loops = State::Loops;
		auto &visitor = state.visitor;
		auto &sorter = state.edge_handler;
		auto &pi = node.pi;

		auto &i_data = get(instance_data_t(), state.data);
		auto &refined_beginnings = i_data.refined_beginnings;
		auto &data = i_data.cell_data;

		refined_beginnings.clear();
		if(is_refiner_singleton) {
			const auto hit_count = data[cell].hit_count;
			assert(hit_count > 0); // otherwise it wouldn't be a refinee
			const auto cell_mid = cell_end - hit_count;
			// if they were all hit, we don't have an initial split
			if(cell_mid != cell)
				refined_beginnings.push_back(cell_mid);
			// the sorter may have additional splits
			sorter.template sort_singleton_refiner<ParallelEdges, Loops>(pi, cell, cell_mid, cell_end, refined_beginnings);
		} else {
			const auto max = data[cell].max;
			const auto max_count = data[cell].max_count;
			assert(max > 0);
			switch(data[cell].type) {
			case refinee_type::unique_partitioning:
			{
				assert(max == 1);
				const auto hit_count = data[cell].hit_count;
				assert(hit_count > 0); // otherwise it wouldn't be a refinee
				const auto cell_mid = cell_end - hit_count;
				assert(cell_mid != cell); // otherwise we would be equally hit
				refined_beginnings.push_back(cell_mid);
				data[cell].first_non_zero = cell_mid;
				sorter.template sort_partitioned<ParallelEdges, Loops>(pi, cell, cell_mid, cell_end,
						max_count, refined_beginnings);
			}
				break;
			case refinee_type::equal_hit:
				assert(max_count == cell_end - cell);
				data[cell].first_non_zero = cell;
				sorter.template sort_equal_hit<ParallelEdges, Loops>(pi, cell, cell_end,
						max, refined_beginnings);
				break;
			case refinee_type::duplicate_partitioning:
			{
				// hit_count is not the real one, but indicates the split
				const auto hit_count = data[cell].hit_count;
				assert(hit_count > 0); // otherwise it wouldn't be a refinee
				const auto cell_mid = cell_end - hit_count;
				if(cell_mid != cell)
					refined_beginnings.push_back(cell_mid);
				data[cell].first_non_zero = cell_mid;
				assert(max_count <= hit_count);
				if(max_count == hit_count) {
					sorter.template sort_duplicate_partitioned_equal_hit<ParallelEdges, Loops>(pi, cell, cell_mid, cell_end,
							max, refined_beginnings);
				} else {
					sorter.template sort_duplicate_partitioned<ParallelEdges, Loops>(pi, cell, cell_mid, cell_end,
							max, max_count,
							i_data.counters, refined_beginnings);
				}
				break;
			}
			case refinee_type::sort:
				// the sorter must provide all the split positions,
				// and set first_non_zero
				sorter.template sort<ParallelEdges, Loops>(pi, cell, cell_end,
						max, max_count,
						data[cell].first_non_zero,
						i_data.counters, refined_beginnings);
				break;
			}
		} // if is_refiner_singleton

		if(refined_beginnings.empty()) {
			const auto &hit_count = data[cell].hit_count;
			return visitor.refine_quotient_edge(state, node, refiner_begin, cell, hit_count);
		}
		result = RefinementResult::Changed;
		visitor.refine_cell_split_begin(state, node, refiner_begin, cell, cell_end);
		{ // first define the boundaries of the new cells
			auto raii_cell_splitter = pi.split_cell(cell);
			for(const SizeType new_cell_begin : refined_beginnings)
				raii_cell_splitter.add_split(new_cell_begin);
		}
		// and now report the new cell beginnings
		for(const SizeType new_cell_begin : refined_beginnings) {
			const bool continue_ = visitor.refine_new_cell(state, node, new_cell_begin, refine_new_cell_type);
			if(!continue_) {
				visitor.refine_abort(state, node);
				return false;
			}
		}
		visitor.refine_cell_split_end(state, node, refiner_begin, cell, cell_end);

		add_refiner_cells(state, pi, cell);
		return true;
	}
public:

	template<typename State>
	void add_refiner_cells(State &state, const auto &pi, const auto refinee_begin) {
		using SizeType = typename State::SizeType;
		auto &i_data = get(instance_data_t(), state.data);
		auto &refiner_cells = i_data.refiner_cells;
		auto &refined_beginnings = i_data.refined_beginnings;
		auto &cell_data = i_data.cell_data;

		// now fix refiner_cells; if refinee_begin
		const bool refinee_in_refiner_cells = cell_data[refinee_begin].is_refiner;
		// see algorithm 1 in [3]
		if(refinee_in_refiner_cells) {
			// refinee_begin is still a valid cell, and is already in refiner_cells
			// add the rest
			for(const auto new_refiner : refined_beginnings) {
				refiner_cells.push_back(new_refiner);
				cell_data[new_refiner].is_refiner = true;
			}
		} else {
			// add all but one of the cells, so let's skip one of the largest cells
			SizeType max_cell_index = -1;
			SizeType max_cell_size = pi.get_cell_size(refinee_begin);
			for(SizeType i_refined_begin = 0; i_refined_begin < refined_beginnings.size(); ++i_refined_begin) {
				const auto size = pi.get_cell_size(refined_beginnings[i_refined_begin]);
				if(size > max_cell_size) {
					max_cell_size = size;
					max_cell_index = i_refined_begin;
				}
			}
			if(max_cell_index == -1) {
				for(const SizeType new_refiner : refined_beginnings) {
					refiner_cells.push_back(new_refiner);
					cell_data[new_refiner].is_refiner = true;
				}
			} else {
				refiner_cells.push_back(refinee_begin);
				cell_data[refinee_begin].is_refiner = true;
				for(SizeType i_refined_begin = 0; i_refined_begin < max_cell_index; i_refined_begin++) {
					const SizeType new_refiner = refined_beginnings[i_refined_begin];
					refiner_cells.push_back(new_refiner);
					cell_data[new_refiner].is_refiner = true;
				}
				for(SizeType i_refined_begin = max_cell_index + 1; i_refined_begin < refined_beginnings.size(); i_refined_begin++) {
					const SizeType new_refiner = refined_beginnings[i_refined_begin];
					refiner_cells.push_back(new_refiner);
					cell_data[new_refiner].is_refiner = true;
				}
			} // if largest cell is the original
		} // if refinee is in refiner_cells
	}

	template<typename State>
	void reset_cell_beginnings(State &state, auto &node) {
		using SizeType = typename State::SizeType;
		const auto &pi = node.pi;
		auto &i_data = get(instance_data_t(), state.data);
		auto &refinee_cells = i_data.refinee_cells;

		for(const auto rp : refinee_cells) {
			const auto refinee_end = rp.last;
			auto current_cell = rp.first;
			auto current_cell_end = pi.get_cell_end(current_cell);
			// if no splits, nothing to be done
			if(current_cell_end == refinee_end) continue;
			// but the first cell is also correct
			current_cell = current_cell_end;
			for(; current_cell != refinee_end; current_cell = current_cell_end) {
				current_cell_end = pi.get_cell_end(current_cell);
				node.pi.set_element_to_cell(current_cell);
			}
			for(auto c = rp.first; c != rp.last; c = node.pi.get_cell_end(c)) {
				for(auto i = c; i != node.pi.get_cell_end(c); ++i) {
					assert(node.pi.get_cell_from_element(i) == c);
				}
			}
		}
	}
};

} // namespace graph_canon

#endif /* GRAPH_CANON_REFINE_WL_1_HPP */