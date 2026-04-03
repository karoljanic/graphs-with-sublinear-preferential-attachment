#ifndef MAX_PLANAR_SUBGRAPH_HPP
#define MAX_PLANAR_SUBGRAPH_HPP

#include "disjoint_set.hpp"
#include "planarity.hpp"
#include "sparse_graph.hpp"

#include <ogdf/basic/Graph.h>

namespace planarity {
template <typename NodeType, typename EdgeType>
class MaxPlanarSubgraph {
 public:
  MaxPlanarSubgraph() = default;

  MaxPlanarSubgraph(const MaxPlanarSubgraph&) = default;
  MaxPlanarSubgraph(MaxPlanarSubgraph&&) = default;

  MaxPlanarSubgraph& operator=(const MaxPlanarSubgraph&) = default;
  MaxPlanarSubgraph& operator=(MaxPlanarSubgraph&&) = default;

  ~MaxPlanarSubgraph() = default;

  static void mstBased(const graph::SparseGraph<NodeType, EdgeType>& graph,
                       graph::SparseGraph<NodeType, EdgeType>& max_planar_subgraph) {
    copyVertices(graph, max_planar_subgraph);

    size_t max_deg_vertex = 0;
    for (size_t i = 0; i < graph.getNodesNumber(); ++i) {
      if (graph.getDegree(i) > graph.getDegree(max_deg_vertex)) {
        max_deg_vertex = i;
      }
    }

    // Prim's algorithm
    std::priority_queue<std::pair<size_t, size_t>, std::vector<std::pair<size_t, size_t>>, Compare> queue{Compare{graph}};
    std::vector<bool> visited(graph.getNodesNumber(), false);

    queue.push({max_deg_vertex, max_deg_vertex});

    while (!queue.empty()) {
      const auto [parent, node] = queue.top();
      queue.pop();

      if (visited[node]) {
        continue;
      }

      visited[node] = true;
      if (parent != node) {  // skip the first iteration
        max_planar_subgraph.addEdge(parent, node);
      }

      for (const auto& neighbor : graph.getNeighbours(node)) {
        if (!visited[neighbor.id]) {
          queue.push({node, neighbor.id});
        }
      }
    }

    maximizeSubgraph(graph, max_planar_subgraph);
  }

  static void cactusBased(const graph::SparseGraph<NodeType, EdgeType>& graph,
                          graph::SparseGraph<NodeType, EdgeType>& max_planar_subgraph) {
    copyVertices(graph, max_planar_subgraph);

    std::map<size_t, std::map<size_t, bool>> usedEdges;
    for (size_t i = 0; i < graph.getNodesNumber(); ++i) {
      for (const auto& neighbor : graph.getNeighbours(i)) {
        usedEdges[i][neighbor.id] = false;
      }
    }

    DisjointSet components{graph.getNodesNumber()};
    for (size_t node = 0; node < graph.getNodesNumber(); ++node) {
      components.makeSet(node);
    }

    // find all diamonds - two triangles sharing an edge
    std::vector<std::vector<size_t>> diamonds;
    for (const auto& edge : graph.getEdges()) {
      std::vector<size_t> tops;

      for (const auto& neighbor1 : graph.getNeighbours(edge.source)) {
        for (const auto& neighbor2 : graph.getNeighbours(edge.target)) {
          if (neighbor1.id == neighbor2.id) {
            tops.push_back(neighbor1.id);
          }
        }
      }

      for (size_t top1Index = 0; top1Index < tops.size(); ++top1Index) {
        for (size_t top2Index = top1Index + 1; top2Index < tops.size(); ++top2Index) {
          diamonds.push_back({edge.source, edge.target, tops[top1Index], tops[top2Index]});
        }
      }
    }

    std::sort(diamonds.begin(), diamonds.end(), [&graph](const std::vector<size_t>& lhs, const std::vector<size_t>& rhs) {
      const size_t weight_lhs = edgeWeight(graph, lhs[0], lhs[1]) + edgeWeight(graph, lhs[0], lhs[2]) +
                                edgeWeight(graph, lhs[0], lhs[3]) + edgeWeight(graph, lhs[1], lhs[2]) +
                                edgeWeight(graph, lhs[1], lhs[3]);
      const size_t weight_rhs = edgeWeight(graph, rhs[0], rhs[1]) + edgeWeight(graph, rhs[0], rhs[2]) +
                                edgeWeight(graph, rhs[0], rhs[3]) + edgeWeight(graph, rhs[1], rhs[2]) +
                                edgeWeight(graph, rhs[1], rhs[3]);
      return weight_lhs < weight_rhs;
    });

    for (const auto& diamond : diamonds) {
      if (components.findSet(diamond[0]) == components.findSet(diamond[1]) ||
          components.findSet(diamond[0]) == components.findSet(diamond[2]) ||
          components.findSet(diamond[0]) == components.findSet(diamond[3]) ||
          components.findSet(diamond[1]) == components.findSet(diamond[2]) ||
          components.findSet(diamond[1]) == components.findSet(diamond[3]) ||
          components.findSet(diamond[2]) == components.findSet(diamond[3])) {
        continue;
      }

      if (usedEdges[diamond[0]][diamond[1]] || usedEdges[diamond[0]][diamond[2]] || usedEdges[diamond[0]][diamond[3]] ||
          usedEdges[diamond[1]][diamond[2]] || usedEdges[diamond[1]][diamond[3]]) {
        continue;
      }

      addEdge(diamond[0], diamond[1], max_planar_subgraph, usedEdges);
      addEdge(diamond[0], diamond[2], max_planar_subgraph, usedEdges);
      addEdge(diamond[0], diamond[3], max_planar_subgraph, usedEdges);
      addEdge(diamond[1], diamond[2], max_planar_subgraph, usedEdges);
      addEdge(diamond[1], diamond[3], max_planar_subgraph, usedEdges);

      mergeComponents(components, {diamond[0], diamond[1], diamond[2], diamond[3]});
    }

    // find all triangles
    std::vector<std::vector<size_t>> triangles;
    for (const auto& edge : graph.getEdges()) {
      if (components.findSet(edge.source) == components.findSet(edge.target)) {
        continue;
      }

      if (usedEdges[edge.source][edge.target]) {
        continue;
      }

      for (const auto& neighbor1 : graph.getNeighbours(edge.source)) {
        for (const auto& neighbor2 : graph.getNeighbours(edge.target)) {
          if (neighbor1.id != neighbor2.id) {
            continue;
          }

          if (components.findSet(neighbor1.id) == components.findSet(edge.source) ||
              components.findSet(neighbor1.id) == components.findSet(edge.target)) {
            continue;
          }

          if (usedEdges[edge.source][neighbor1.id] || usedEdges[edge.target][neighbor1.id]) {
            continue;
          }

          triangles.push_back({edge.source, edge.target, neighbor1.id});
        }
      }
    }

    std::sort(triangles.begin(), triangles.end(), [&graph](const std::vector<size_t>& lhs, const std::vector<size_t>& rhs) {
      const size_t weight_lhs =
          edgeWeight(graph, lhs[0], lhs[1]) + edgeWeight(graph, lhs[0], lhs[2]) + edgeWeight(graph, lhs[1], lhs[2]);
      const size_t weight_rhs =
          edgeWeight(graph, rhs[0], rhs[1]) + edgeWeight(graph, rhs[0], rhs[2]) + edgeWeight(graph, rhs[1], rhs[2]);
      return weight_lhs < weight_rhs;
    });

    for (const auto& triangle : triangles) {
      if (components.findSet(triangle[0]) == components.findSet(triangle[1]) ||
          components.findSet(triangle[0]) == components.findSet(triangle[2]) ||
          components.findSet(triangle[1]) == components.findSet(triangle[2])) {
        continue;
      }

      if (usedEdges[triangle[0]][triangle[1]] || usedEdges[triangle[0]][triangle[2]] || usedEdges[triangle[1]][triangle[2]]) {
        continue;
      }

      addEdge(triangle[0], triangle[1], max_planar_subgraph, usedEdges);
      addEdge(triangle[0], triangle[2], max_planar_subgraph, usedEdges);
      addEdge(triangle[1], triangle[2], max_planar_subgraph, usedEdges);

      mergeComponents(components, {triangle[0], triangle[1], triangle[2]});
    }

    // find all remaining edges
    std::vector<EdgeType> remainingEdges;
    for (const auto& edge : graph.getEdges()) {
      if (components.findSet(edge.source) == components.findSet(edge.target)) {
        continue;
      }

      if (usedEdges[edge.source][edge.target]) {
        continue;
      }

      remainingEdges.push_back(edge);
    }

    std::sort(remainingEdges.begin(), remainingEdges.end(), [&graph](const EdgeType& lhs, const EdgeType& rhs) {
      return edgeWeight(graph, lhs.source, lhs.target) < edgeWeight(graph, rhs.source, rhs.target);
    });

    for (const auto& edge : remainingEdges) {
      if (components.findSet(edge.source) == components.findSet(edge.target)) {
        continue;
      }

      addEdge(edge.source, edge.target, max_planar_subgraph, usedEdges);

      mergeComponents(components, {edge.source, edge.target});
    }

    maximizeSubgraph(graph, max_planar_subgraph);
  }

 private:
  static void maximizeSubgraph(const graph::SparseGraph<NodeType, EdgeType>& graph,
                               graph::SparseGraph<NodeType, EdgeType>& max_planar_subgraph) {
    std::vector<EdgeType> remainingEdges;
    for (const auto& edge : graph.getEdges()) {
      if (!max_planar_subgraph.edgeExists(edge.source, edge.target)) {
        remainingEdges.push_back(edge);
      }
    }

    std::sort(remainingEdges.begin(), remainingEdges.end(), [&graph](const EdgeType& lhs, const EdgeType& rhs) {
      return edgeWeight(graph, lhs.source, lhs.target) > edgeWeight(graph, rhs.source, rhs.target);
    });

    const size_t max_edges = 3 * max_planar_subgraph.getNodesNumber() - 6;

    for (const auto& edge : remainingEdges) {
      max_planar_subgraph.addEdge(edge.source, edge.target);
      if (!Planarity<NodeType, EdgeType>::isPlanar(max_planar_subgraph)) {
        max_planar_subgraph.removeEdge(edge.source, edge.target);
      }
      else {
        if (max_planar_subgraph.getEdgesNumber() > max_edges) {
          break;
        }
      }
    }
  }

  static void mergeComponents(DisjointSet& components, std::vector<size_t> ids) {
    size_t newId = ids[0];
    for (size_t i = 1; i < ids.size(); ++i) {
      components.unionSets(newId, ids[i]);
    }
  }

  static void copyVertices(const graph::SparseGraph<NodeType, EdgeType>& graph,
                           graph::SparseGraph<NodeType, EdgeType>& max_planar_subgraph) {
    for (size_t i = 0; i < graph.getNodesNumber(); ++i) {
      max_planar_subgraph.addNode();
    }
  }

  static void addEdge(size_t source, size_t target, graph::SparseGraph<NodeType, EdgeType>& max_planar_subgraph,
                      std::map<size_t, std::map<size_t, bool>>& usedEdges) {
    max_planar_subgraph.addEdge(source, target);
    usedEdges[source][target] = true;
    usedEdges[target][source] = true;
  }

  static size_t edgeWeight(const graph::SparseGraph<NodeType, EdgeType>& graph, size_t source, size_t target) {
    return graph.getDegree(source) * graph.getDegree(target);
  }

  struct Compare {
    explicit Compare(const graph::SparseGraph<NodeType, EdgeType>& graph) : graph_{graph} {}

    bool operator()(const std::pair<size_t, size_t>& lhs, const std::pair<size_t, size_t>& rhs) const {
      return edgeWeight(graph_, lhs.first, lhs.second) < edgeWeight(graph_, rhs.first, rhs.second);
    }

   private:
    const graph::SparseGraph<NodeType, EdgeType>& graph_;
  };
};
}  // namespace planarity
#endif  // MAX_PLANAR_SUBGRAPH_HPP