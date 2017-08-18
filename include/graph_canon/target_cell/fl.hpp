#ifndef GRAPH_CANON_TARGET_CELL_FL_HPP
#define GRAPH_CANON_TARGET_CELL_FL_HPP

#include <graph_canon/visitor/compound.hpp>

namespace graph_canon {

struct target_cell_fl : null_visitor {
  using can_select_target_cell = std::true_type;

  template<typename State, typename TreeNode>
  std::size_t select_target_cell(const State &state, const TreeNode &t) const {
    const auto &pi = t.pi;
    std::size_t max_size = 1;
    std::size_t result = state.n;
    for(std::size_t cell_begin = 0;
            cell_begin < state.n;
            cell_begin = pi.get_cell_end(cell_begin)) {
      std::size_t cell_end = pi.get_cell_end(cell_begin);
      if(cell_end - cell_begin > max_size) {
        result = cell_begin;
        max_size = cell_end - cell_begin;
      }
    }
    return result;
  }
};

} // namespace graph_canon

#endif /* GRAPH_CANON_TARGET_CELL_FL_HPP */