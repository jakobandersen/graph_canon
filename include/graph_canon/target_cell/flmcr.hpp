#ifndef GRAPH_CANON_TARGET_CELL_FLMCR_HPP
#define GRAPH_CANON_TARGET_CELL_FLMCR_HPP

#include <graph_canon/visitor/compound.hpp>

namespace graph_canon {

struct target_cell_flmcr : null_visitor {
	using can_select_target_cell = std::true_type;

	struct instance_data_t {
	};

	template<typename SizeType>
	struct instance_data {

		struct data_t {
			// per vertex
			SizeType count = 0;
			// per cell
			SizeType non_zero_count = 0; // during check of if non-uniform
			SizeType component;
			SizeType degree = 0;
			SizeType size = 0;
		};
	public:
		std::vector<data_t> data;
		std::vector<SizeType> cells;
	};

	template<typename Config, typename TreeNode>
	struct InstanceData {
		using type = tagged_list<instance_data_t, instance_data<typename Config::SizeType> >;
	};

	void initialize(auto &state) {
		get(instance_data_t(), state.data).data.resize(state.n);
	}

	template<typename State>
	std::size_t select_target_cell(State &state, const auto &t) {
		const auto result = find_cell(state, t);
		if(result != state.n) {
			return result;
		} else {
			for(std::size_t cell_begin = 0;
					cell_begin < state.n;
					cell_begin = t.pi.get_cell_end(cell_begin)) {
				std::size_t cell_end = t.pi.get_cell_end(cell_begin);
				if(cell_end - cell_begin > 1) {
					std::cout << "TargetCell   res=" << result << std::endl;
					return cell_begin;
				}
			}
			assert(false);
		}
	}
private:

	template<typename State>
	void find_non_uniform(State &state, const auto &t, const auto cell, const auto callback) {
		using SizeType = typename State::SizeType;
		auto &i_data = get(instance_data_t(), state.data);
		auto &data = i_data.data;
		auto &cells = i_data.cells;
		const SizeType v_idx = t.pi.get(cell);
		detail::for_each_neighbour(state, t, vertex(v_idx, state.g),
				[&](const auto e_out, const auto v_pos, const auto target_cell, const auto target_cell_end) {
					auto &count = data[v_pos].count;
					auto &non_zero_count = data[target_cell].non_zero_count;
					if(count == 0) {
						++non_zero_count;
					}
					if(non_zero_count == 1) {
						cells.push_back(target_cell);
					}
				});
		for(const auto cell_other : cells)
			callback(cell_other);

		// cleanup
		detail::for_each_neighbour(state, t, vertex(v_idx, state.g),
				[&](const auto e_out, const auto v_pos, const auto target_cell, const auto target_cell_end) {
					auto &count = data[v_pos].count;
					count = 0;
				});
		for(const auto cell : cells)
			data[cell].non_zero_count = 0;
		cells.clear();
	}

	template<typename State>
	std::size_t find_cell(State &state, const auto &t) {
		using SizeType = typename State::SizeType;
		const auto &pi = t.pi;
		auto &i_data = get(instance_data_t(), state.data);
		auto &data = i_data.data;
		{ // initialize components to be individual
			SizeType cell_end;
			for(SizeType cell = 0; cell != state.n; cell = cell_end) {
				cell_end = pi.get_cell_end(cell);
				if(cell + 1 == cell_end) continue;
				if(cell + 2 == cell_end) continue;
				data[cell].component = cell;
			}
		}
		{ // find the components, and record the each cells degree
			SizeType cell_end;
			for(SizeType cell = 0; cell != state.n; cell = cell_end) {
				cell_end = pi.get_cell_end(cell);
				if(cell + 1 == cell_end) continue;
				if(cell + 2 == cell_end) continue;
				find_non_uniform(state, t, cell, [&](const auto cell_other) {
					++data[cell].degree;
					if(cell_other > cell) {
						data[cell_other].component = data[cell].component;
								++data[cell_other].size;
					}
				});
			}
		}
		const SizeType comp = [&]() {
			SizeType cell_end;
			for(SizeType cell = 0; cell != state.n; cell = cell_end) {
				cell_end = pi.get_cell_end(cell);
				if(cell + 1 == cell_end) continue;
				if(cell + 2 == cell_end) continue;
				assert(data[cell].component == cell);
				return cell;
			}
			return state.n;
		}();
		if(comp == state.n) return state.n;
		// now find the first largest max degree cell in that component
		SizeType max_size = 1;
		SizeType max_joined = 0;
		SizeType result = state.n;
		SizeType cell_end;
		for(SizeType cell = comp; cell != state.n; cell = cell_end) {
			cell_end = pi.get_cell_end(cell);
			if(cell + 1 == cell_end) continue;
			if(cell + 2 == cell_end) continue;
			if(data[cell].component != comp) continue;
			const SizeType num_non_uniform = data[cell].degree;
			if(num_non_uniform > max_joined || result == state.n) { // TODO do proper init instead of this hax
				result = cell;
				max_joined = num_non_uniform;
				max_size = cell_end - cell;
			} else if(num_non_uniform == max_joined) {
				if(cell_end - cell > max_size) {
					result = cell;
					max_size = cell_end - cell;
				}
			}
		}
		assert(result != state.n);
		// cleanup
		for(SizeType cell_begin = 0; cell_begin != state.n; cell_begin = cell_end) {
			data[cell_begin].degree = 0;
			data[cell_begin].size = 0;
		}
		//		std::cout << "TargetCell   res=" << result << ", max_joined=" << max_joined << ", max_size=" << max_size << std::endl;
		return result;
	}
};

} // namespace graph_canon

#endif /* GRAPH_CANON_TARGET_CELL_FLMCR_HPP */