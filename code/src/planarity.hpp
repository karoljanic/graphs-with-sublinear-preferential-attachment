#ifndef PLANARITY_HPP
#define PLANARITY_HPP

#include <vector>

#include <ogdf/basic/Graph.h>
#include <ogdf/planarity/MaximalPlanarSubgraphSimple.h>
#include <ogdf/planarity/PlanarSubgraphBoyerMyrvold.h>

#include "sparse_graph.hpp"

namespace planarity {
template <typename NodeType, typename EdgeType>
class Planarity {
 public:
  Planarity() = default;

  Planarity(const Planarity&) = default;
  Planarity(Planarity&&) = default;

  Planarity& operator=(const Planarity&) = default;
  Planarity& operator=(Planarity&&) = default;

  ~Planarity() = default;

  
  static bool isPlanar(const graph::SparseGraph<NodeType, EdgeType>& graph) {
    ogdf::Graph ogdf_graph;
    toOgdf(graph, ogdf_graph);

    return ogdf::isPlanar(ogdf_graph);
  }

  
  static void boyerMyrvoldPlanarSubgraph(const graph::SparseGraph<NodeType, EdgeType>& graph,
                                         graph::SparseGraph<NodeType, EdgeType>& subgraph) {
    ogdf::Graph ogdf_graph;
    toOgdf(graph, ogdf_graph);

    ogdf::PlanarSubgraphBoyerMyrvold psbm;
    ogdf::List<ogdf::edge> delEdges;
    psbm.call(ogdf_graph, delEdges);

    for (auto* edge : delEdges) {
      ogdf_graph.delEdge(edge);
    }

    subgraph = graph::SparseGraph<NodeType, EdgeType>();
    fromOgdf(ogdf_graph, subgraph);
  }

  
  static std::vector<std::vector<EdgeType>> minimalEdgesToRestorePlanarity(const graph::SparseGraph<NodeType, EdgeType>& graph,
                                                                           size_t last_added_node) {
    if (Planarity::isPlanar(graph)) {
      return {{}};
    }

    std::vector<EdgeType> edges;
    for (const auto& edge : graph.getEdges()) {
      if (edge.source == last_added_node || edge.target == last_added_node) {
        continue;
      }
      edges.push_back(edge);
    }

    for (size_t edges_to_remove = 1; edges_to_remove < edges.size(); ++edges_to_remove) {
      std::vector<int> mask(edges.size(), 0);
      std::fill(mask.end() - edges_to_remove, mask.end(), 1);

      std::vector<std::vector<EdgeType>> results;
      do {
        graph::SparseGraph<NodeType, EdgeType> subgraph = graph;
        std::vector<EdgeType> removed_edges;
        removed_edges.reserve(edges_to_remove);

        for (size_t i = 0; i < edges.size(); ++i) {
          if (mask[i] == 1) {
            subgraph.removeEdge(edges[i].source, edges[i].target);
            removed_edges.push_back(edges[i]);
          }
        }

        if (Planarity::isPlanar(subgraph)) {
          results.push_back(removed_edges);
        }

      } while (std::next_permutation(mask.begin(), mask.end()));

      if (!results.empty()) {
        return results;
      }
    }

    return {edges};
  }

 private:
  
  static void toOgdf(const graph::SparseGraph<NodeType, EdgeType>& graph, ogdf::Graph& ogdf_graph) {
    std::vector<ogdf::node> nodes(graph.getNodesNumber());
    for (size_t i = 0; i < graph.getNodesNumber(); ++i) {
      nodes[i] = ogdf_graph.newNode();
    }

    for (const auto& edge : graph.getEdges()) {
      ogdf_graph.newEdge(nodes[edge.source], nodes[edge.target]);
    }
  }

  
  static void fromOgdf(const ogdf::Graph& ogdf_graph, graph::SparseGraph<NodeType, EdgeType>& graph) {
    for (const auto& node : ogdf_graph.nodes) {
      graph.addNode();
    }

    for (const auto& edge : ogdf_graph.edges) {
      graph.addEdge(edge->source()->index(), edge->target()->index());
    }
  }
};
}  // namespace planarity

#endif  // PLANARITY_HPP