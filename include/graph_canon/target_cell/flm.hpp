#ifndef GRAPH_CANON_TARGET_CELL_FLM_HPP
#define GRAPH_CANON_TARGET_CELL_FLM_HPP

#include <graph_canon/visitor/visitor.hpp>

namespace graph_canon {

// rst: .. class:: target_cell_flm
// rst:
// rst:		Visitor for selecting the first largest non-singleton cell,
// rst:		among those with a maximum number of non-uniformly joined neighbour cells.
// rst:
// rst:		The class is DefaultConstructible.
// rst:

struct target_cell_flm : null_visitor {
	using can_select_target_cell = std::true_type;

	struct instance_data_t {
	};

	template<typename SizeType>
	struct instance_data {

		struct data {
			// per vertex
			SizeType count = 0;
			// per cell
			SizeType non_zero_count = 0;
		};
	public:
		std::vector<data> data;
		std::vector<SizeType> cells;
	};

	template<typename Config, typename TreeNode>
	struct InstanceData {
		using type = tagged_element<instance_data_t, instance_data<typename Config::SizeType> >;
	};

	template<typename State>
	void initialize(State &state) {
		get(instance_data_t(), state.data).data.resize(state.n);
	}

	template<typename State, typename TreeNode>
	std::size_t select_target_cell(State &state, const TreeNode &t) {
		auto result = find_cell(state, t, [&](const auto cell, const auto cell_end) {
			return cell + 2 != cell_end;
		});

		if(result == state.n) {
			result = find_cell(state, t, [](const auto&... args) {
				return true;
			});
		}

		//		std::cout << "TargetCell   res=" << result << ", max_joined=" << max_joined << ", max_size=" << max_size << std::endl;
		return result;
	}
private:

	template<typename State, typename TreeNode, typename Predicate>
	std::size_t find_cell(State &state, const TreeNode &t, const Predicate pred) {
		using SizeType = typename State::SizeType;
		auto &i_data = get(instance_data_t(), state.data);
		auto &data = i_data.data;
		auto &cells = i_data.cells;
		const auto &pi = t.pi;
		SizeType max_size = 1;
		SizeType max_joined = 0;
		SizeType result = state.n;
		SizeType cell_end;
		for(SizeType cell_begin = 0; cell_begin != state.n; cell_begin = cell_end) {
			cell_end = pi.get_cell_end(cell_begin);
			if(cell_begin + 1 == cell_end) continue;
			if(!pred(cell_begin, cell_end)) continue;
			const SizeType v_idx = pi.get(cell_begin);
			const auto *begin_inverse = t.pi.begin_inverse();
			detail::for_each_neighbour(state, t, vertex(v_idx, state.g),
					[&](const auto e_out, const auto v_idx, const auto target_cell, const auto target_cell_end) {
						const auto v_pos = begin_inverse[v_idx];
						auto &count = data[v_pos].count;
						auto &non_zero_count = data[target_cell].non_zero_count;
						if(count == 0) {
							++non_zero_count;
						}
						if(non_zero_count == 1) {
							cells.push_back(target_cell);
						}
					});
			const auto num_non_uniform = std::count_if(cells.begin(), cells.end(), [&](const auto cell) {
				const auto cell_size = pi.get_cell_size(cell);
				return data[cell].non_zero_count < cell_size;
			});
			if(num_non_uniform > max_joined) {
				result = cell_begin;
				max_joined = num_non_uniform;
				max_size = cell_end - cell_begin;
			} else if(num_non_uniform == max_joined) {
				if(cell_end - cell_begin > max_size) {
					result = cell_begin;
					max_size = cell_end - cell_begin;
				}
			}

			// cleanup
			detail::for_each_neighbour(state, t, vertex(v_idx, state.g),
					[&](const auto e_out, const auto v_idx, const auto target_cell, const auto target_cell_end) {
						const auto v_pos = begin_inverse[v_idx];
						auto &count = data[v_pos].count;
						count = 0;
					});
			for(const auto cell : cells)
				data[cell].non_zero_count = 0;
			cells.clear();
		}
		return result;
	}
};

} // namespace graph_canon

#endif /* GRAPH_CANON_TARGET_CELL_FLM_HPP */