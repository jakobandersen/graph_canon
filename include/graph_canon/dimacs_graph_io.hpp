#ifndef GRAPH_CANON_READ_DIMACS_GRAPH_HPP
#define GRAPH_CANON_READ_DIMACS_GRAPH_HPP

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>

#include <cstdio>
#include <string>
#include <vector>

namespace graph_canon {

// rst: .. function:: template<typename Graph> \
// rst:               void write_dimacs_graph(std::ostream &s, const Graph &g)
// rst:
// rst:		Write the given graph in DIMACS format (see :cpp:func:`read_dimacs_format`).
// rst:
// rst:		The vertices must have indices, i.e., the expression `get(boost::verex_index_t(), g)` must be valid.
// rst:		Currently the expression `get(boost::vertex_name_t(), g)` must also be valid and return a property map with integer values
// rst:		If any vertex have non-zero vertex name, then the graph is considered vertex labelled and the vertex names are written.

template<typename Graph>
void write_dimacs_graph(std::ostream &s, const Graph &g) {
	s << "p edge " << num_vertices(g) << " " << num_edges(g) << '\n';
	const auto vs = vertices(g);
	const bool withVertexLabels = std::any_of(vs.first, vs.second, [&g](const auto &v) {
		return get(boost::vertex_name_t(), g, v) != 0;
	});
	if(withVertexLabels) {
		for(auto iter = vs.first; iter != vs.second; ++iter)
			s << "n " << (get(boost::vertex_index_t(), g, *iter) + 1) << " " << get(boost::vertex_name_t(), g, *iter) << '\n';
	}
	const auto es = edges(g);
	for(auto iter = es.first; iter != es.second; ++iter)
		s << "e " << (get(boost::vertex_index_t(), g, source(*iter, g)) + 1)
		<< " " << (get(boost::vertex_index_t(), g, target(*iter, g)) + 1) << '\n';
}


// rst: .. function:: template<typename Graph, typename ParallelHandler, typename LoopHandler> \
// rst:               bool read_dimacs_graph(std::istream &s, Graph &graph, std::ostream &err, ParallelHandler parHandler, LoopHandler loopHandler)
// rst:
// rst:		Parse a graph in DIMACS format from `s` and store it in `graph`.
// rst:		Reading errors are written to `err`.
// rst:
// rst:		The unary predicate `loopHandler` is called with vertex indices where a loop edge has been parsed.
// rst:		The edge is added to the graph only if the handler returns `true`.
// rst:		The binary predicate `parHandler` must similarly evaluated parallel edges, if present.
// rst:
// rst:		The format is line-based. Empty lines and lines starting with ``c`` are ignored.
// rst:		The first non-ignored line must be on the form ``p edge <n> <m>`` where
// rst:		``<n>`` is the number of vertices in edge graph and
// rst:		``<m>`` is the number of edges.
// rst:		Afterwards there must be exactly ``<m>`` lines on the form
// rst:		``e <src> <tar>`` where ``<src>`` and ``<tar>`` are vertex IDs in the range 1 to :math:`n`,
// rst:		representing and edge.
// rst:
// rst:		In addition there may be any number of lines on the form ``n <id> <label>``
// rst:		where ``<id>`` is a vertex ID in the range 1 to :math:`n`,
// rst:		and ``<label>`` is a non-negative number that will be assigned as the label of the vertex.

template<typename Graph, typename ParallelHandler, typename LoopHandler>
bool read_dimacs_graph(std::istream &s, Graph &graph, std::ostream &err, ParallelHandler parHandler, LoopHandler loopHandler) {
	using Vertex = typename boost::graph_traits<Graph>::vertex_descriptor;
	std::string line;
	while(std::getline(s, line)) {
		if(line.empty()) continue;
		if(line[0] == 'c') continue;
		break;
	}
	if(line[0] != 'p') {
		err << "First line is not problem specification, it is '" << line << "'.\n";
		return false;
	}
	unsigned int n, m;
	if(std::sscanf(line.c_str(), "p edge %u %u", &n, &m) != 2) {
		err << "Could not parse problem specification '" << line << "'.\n";
		return false;
	}
	std::vector<Vertex> vertices(n);
	for(unsigned int i = 0; i < n; i++) {
		vertices[i] = add_vertex(graph);
		put(boost::vertex_name_t(), graph, vertices[i], 0);
	}
	unsigned int edgeCount = 0;
	while(std::getline(s, line)) {
		if(line.empty()) continue;
		if(line[0] == 'c') continue;
		if(line[0] == 'n') {
			unsigned int id, label;
			if(std::sscanf(line.c_str(), "n %u %u", &id, &label) != 2) {
				err << "Could not parse node line '" << line << "'.\n";
				return false;
			}
			if(id > n || id == 0) {
				err << "Invalid node index " << id << " in line '" << line << "'.\n";
				return false;
			}
			put(boost::vertex_name_t(), graph, vertices[id - 1], label);
		} else if(line[0] == 'e') {
			++edgeCount;
			unsigned int src, tar;
			if(std::sscanf(line.c_str(), "e %u %u", &src, &tar) != 2) {
				err << "Could not parse edge line '" << line << "'.\n";
				return false;
			}
			if(src > n || src == 0) {
				err << "Invalid source index " << src << " in line '" << line << "'.\n";
				return false;
			}
			if(tar > n || tar == 0) {
				err << "Invalid target index " << tar << " in line '" << line << "'.\n";
				return false;
			}
			if(src == tar) {
				if(!loopHandler(src)) continue;
			}
			const auto ep = edge(vertices[src - 1], vertices[tar - 1], graph);
			if(ep.second) {
				if(!parHandler(src, tar)) continue;
			}
			add_edge(vertices[src - 1], vertices[tar - 1], graph);
		} else {
			err << "Can not parse line '" << line << "'.\n";
			return false;
		}
	}
	if(edgeCount != m) {
		err << "Number of edge lines does not match number of edges specified.\n";
		return false;
	}
	return true;
}

} // namespace graph_canon

#endif /* GRAPH_CANON_READ_DIMACS_GRAPH_HPP */