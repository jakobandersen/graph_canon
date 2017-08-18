#ifndef GRAPH_CANON_COMPARE_HPP
#define	GRAPH_CANON_COMPARE_HPP

#include <graph_canon/ordered_graph.hpp>

namespace graph_canon {

struct graph_compare_null_visitor {

  void at_num_vertices() const { }

  template<typename VertexL, typename VertexR>
  void at_vertex_compare(VertexL, VertexR) const { }

  template<typename VertexL, typename VertexR>
  void at_out_degree(VertexL, VertexR) const { }

  template<typename EdgeL, typename EdgeR>
  void at_out_edge(EdgeL, EdgeR) const { }

  template<typename EdgeL, typename EdgeR>
  void at_edge_compare(EdgeL, EdgeR) const { }

  void at_end(bool) const { }
};

template<typename GraphL, typename IndexMapL, bool WithInEdgesL,
/**/ typename GraphR, typename IndexMapR, bool WithInEdgesR,
/**/ typename VertexEqual, typename EdgeEqual,
/**/ typename Visitor
>
bool ordered_graph_equal(
        const ordered_graph<GraphL, IndexMapL, WithInEdgesL> &g_left,
        const ordered_graph<GraphR, IndexMapR, WithInEdgesR> &g_right,
        VertexEqual vertex_equal, EdgeEqual edge_equal, Visitor &&visitor) {
  typedef boost::graph_traits<ordered_graph<GraphL, IndexMapL, WithInEdgesL> > TraitsL;
  typedef boost::graph_traits<ordered_graph<GraphR, IndexMapR, WithInEdgesR> > TraitsR;
  typename TraitsL::vertices_size_type n_left = num_vertices(g_left);
  typename TraitsR::vertices_size_type n_right = num_vertices(g_right);
  if(n_left != n_right) {
    visitor.at_num_vertices();
    return false;
  }
  typename TraitsL::vertex_iterator v_iter_left, v_iter_left_end;
  typename TraitsR::vertex_iterator v_iter_right = vertices(g_right).first;
  for(boost::tie(v_iter_left, v_iter_left_end) = vertices(g_left);
          v_iter_left != v_iter_left_end;
          ++v_iter_left, ++v_iter_right) {
    if(!vertex_equal(*v_iter_left, *v_iter_right)) {
      visitor.at_vertex_compare(*v_iter_left, *v_iter_right);
      return false;
    }
    typename TraitsL::degree_size_type d_left = out_degree(*v_iter_left, g_left);
    typename TraitsR::degree_size_type d_right = out_degree(*v_iter_right, g_right);
    if(d_left != d_right) {
      visitor.at_out_degree(*v_iter_left, *v_iter_right);
      return false;
    }
    typename TraitsL::out_edge_iterator e_iter_left, e_iter_left_end;
    typename TraitsR::out_edge_iterator e_iter_right = out_edges(*v_iter_right, g_right).first;
    for(boost::tie(e_iter_left, e_iter_left_end) = out_edges(*v_iter_left, g_left);
            e_iter_left != e_iter_left_end;
            ++e_iter_left, ++e_iter_right) {
      typename TraitsL::vertices_size_type v_idx_left = get(g_left.data.idx, target(*e_iter_left, g_left));
      typename TraitsR::vertices_size_type v_idx_right = get(g_right.data.idx, target(*e_iter_right, g_right));
      if(v_idx_left != v_idx_right) {
        visitor.at_out_edge(*e_iter_left, *e_iter_right);
        return false;
      }
      if(!edge_equal(*e_iter_left, *e_iter_right)) {
        visitor.at_edge_compare(*e_iter_left, *e_iter_right);
        return false;
      }
    }
  }
  visitor.at_end(true);
  return true;
}

template<typename GraphL, typename IndexMapL, bool WithInEdgesL,
/**/ typename GraphR, typename IndexMapR, bool WithInEdgesR,
/**/ typename VertexLess, typename EdgeLess,
/**/ typename VertexEqual, typename EdgeEqual
>
bool ordered_graph_less(
        const ordered_graph<GraphL, IndexMapL, WithInEdgesL> &g_left,
        const ordered_graph<GraphR, IndexMapR, WithInEdgesR> &g_right,
        VertexLess vertex_less, EdgeLess edge_less,
        VertexEqual vertex_equal, EdgeEqual edge_equal) {
  typedef boost::graph_traits<ordered_graph<GraphL, IndexMapL, WithInEdgesL> > TraitsL;
  typedef boost::graph_traits<ordered_graph<GraphR, IndexMapR, WithInEdgesR> > TraitsR;
  typename TraitsL::vertices_size_type n_left = num_vertices(g_left);
  typename TraitsR::vertices_size_type n_right = num_vertices(g_right);
  if(n_left != n_right) return n_left < n_right;
  typename TraitsL::vertex_iterator v_iter_left, v_iter_left_end;
  typename TraitsR::vertex_iterator v_iter_right;
  boost::tie(v_iter_left, v_iter_left_end) = vertices(g_left);
  v_iter_right = vertices(g_right).first;
  for(; v_iter_left != v_iter_left_end; ++v_iter_left, ++v_iter_right) {
    if(!vertex_equal(*v_iter_left, *v_iter_right)) return vertex_less(*v_iter_left, *v_iter_right);
    typename TraitsL::degree_size_type d_left = out_degree(*v_iter_left, g_left);
    typename TraitsR::degree_size_type d_right = out_degree(*v_iter_right, g_right);
    if(d_left != d_right) return d_left < d_right;
    typename TraitsL::out_edge_iterator e_iter_left, e_iter_left_end;
    typename TraitsR::out_edge_iterator e_iter_right;
    boost::tie(e_iter_left, e_iter_left_end) = out_edges(*v_iter_left, g_left);
    e_iter_right = out_edges(*v_iter_right, g_right).first;
    for(; e_iter_left != e_iter_left_end; ++e_iter_left, ++e_iter_right) {
      typename TraitsL::vertices_size_type v_idx_left = get(g_left.data.idx, target(*e_iter_left, g_left));
      typename TraitsR::vertices_size_type v_idx_right = get(g_right.data.idx, target(*e_iter_right, g_right));
      if(v_idx_left != v_idx_right) return v_idx_left < v_idx_right;
      if(!edge_equal(*e_iter_left, *e_iter_right)) return edge_less(*e_iter_left, *e_iter_right);
    }
  }
  return false;
}

} // namespace graph_canon

#endif	/* GRAPH_CANON_COMPARE_HPP */