#ifndef GRAPH_SPARSE_GRAPH_HPP
#define GRAPH_SPARSE_GRAPH_HPP

#include <algorithm>   // std::find_if, std::swap
#include <concepts>    // concepts
#include <cstddef>     // std::size_t
#include <fstream>     // std::ofstream
#include <functional>  // std::function
#include <iostream>    // std::cerr
#include <map>         // std::map
#include <queue>       // std::queue
#include <stack>       // std::stack
#include <stdexcept>   // std::invalid_argument, std::out_of_range
#include <string>      // std::string
#include <vector>      // std::vector

#include <nlohmann/json.hpp>
#include "graph.hpp"

using json = nlohmann::json;

namespace graph {
template <typename NodeType, typename EdgeType>
requires HasId<NodeType>&& HasSourceAndTarget<EdgeType> class SparseGraph : public Graph<NodeType, EdgeType> {
 public:
  SparseGraph() = default;

  SparseGraph(const SparseGraph&) = default;
  SparseGraph(SparseGraph&&) noexcept = default;

  SparseGraph& operator=(const SparseGraph&) = default;
  SparseGraph& operator=(SparseGraph&&) noexcept = default;

  ~SparseGraph() = default;

  std::size_t addNode() override {
    nodes_.emplace_back(NodeType{.id = nodes_.size()});
    adjacency_list_.emplace_back();
    return nodes_.size() - 1;
  }

  void addEdge(std::size_t node1_id, std::size_t node2_id) override {
    if (node1_id >= nodes_.size() || node2_id >= nodes_.size()) {
      throw std::out_of_range("Node ID is out of bounds!");
    }

    if (edgeExists(node1_id, node2_id)) {
      return;
    }

    EdgeType edge{};
    if (node1_id < node2_id) {
      edge.source = node1_id;
      edge.target = node2_id;
    }
    else {
      edge.source = node2_id;
      edge.target = node1_id;
    }

    adjacency_list_[node1_id].emplace_back(edges_.size());
    adjacency_list_[node2_id].emplace_back(edges_.size());
    edges_.emplace_back(edge);
    edge_exists_.emplace_back(true);

    ++active_edges_count_;
  }

  [[nodiscard]] bool edgeExists(std::size_t node1_id, std::size_t node2_id) const override {
    if (node1_id >= adjacency_list_.size() || node2_id >= adjacency_list_.size()) {
      return false;
    }

    const std::size_t source = std::min(node1_id, node2_id);
    const std::size_t target = std::max(node1_id, node2_id);

    return std::ranges::any_of(adjacency_list_[source], [&](std::size_t index) {
      return edges_[index].source == source && edges_[index].target == target;
    });
  }

  void removeEdge(std::size_t node1_id, std::size_t node2_id) override {
    const std::size_t source = std::min(node1_id, node2_id);
    const std::size_t target = std::max(node1_id, node2_id);

    std::size_t edge_index = std::numeric_limits<std::size_t>::max();

    for (auto iter = adjacency_list_[source].begin(); iter != adjacency_list_[source].end(); ++iter) {
      if (edges_[*iter].source == source && edges_[*iter].target == target) {
        edge_index = *iter;
        adjacency_list_[source].erase(iter);
        break;
      }
    }

    if (edge_index != std::numeric_limits<std::size_t>::max()) {
      for (auto iter = adjacency_list_[target].begin(); iter != adjacency_list_[target].end(); ++iter) {
        if (*iter == edge_index) {
          adjacency_list_[target].erase(iter);
          break;
        }
      }
      edge_exists_[edge_index] = false;
      --active_edges_count_;
    }
  }

  [[nodiscard]] NodeType& getNode(std::size_t node_id) override { return nodes_.at(node_id); }

  [[nodiscard]] NodeType& getLastAddedNode() override {
    if (nodes_.empty())
      throw std::logic_error("Graph has no nodes");
    return nodes_.back();
  }

  [[nodiscard]] std::vector<NodeType> getNodes() const override { return nodes_; }

  [[nodiscard]] EdgeType& getEdge(std::size_t source, std::size_t target) override {
    auto iter = std::find_if(adjacency_list_[source].begin(), adjacency_list_[source].end(), [&](std::size_t index) {
      return (edges_[index].source == source && edges_[index].target == target) ||
             (edges_[index].source == target && edges_[index].target == source);
    });

    if (iter != adjacency_list_[source].end()) {
      return edges_[*iter];
    }

    throw std::invalid_argument("The edge (" + std::to_string(source) + "," + std::to_string(target) + ") does not exist!");
  }

  [[nodiscard]] EdgeType& getLastAddedEdge() override {
    for (auto i = edges_.size(); i-- > 0;) {
      if (edge_exists_[i])
        return edges_[i];
    }
    throw std::logic_error("Graph has no active edges");
  }

  [[nodiscard]] std::vector<EdgeType> getEdges() const override {
    std::vector<EdgeType> existing_edges;
    existing_edges.reserve(active_edges_count_);
    for (std::size_t i = 0; i < edges_.size(); ++i) {
      if (edge_exists_[i]) {
        existing_edges.emplace_back(edges_[i]);
      }
    }
    return existing_edges;
  }

  [[nodiscard]] std::size_t getNodesNumber() const override { return nodes_.size(); }

  [[nodiscard]] std::size_t getEdgesNumber() const override { return active_edges_count_; }

  [[nodiscard]] double getDensity() const override {
    const std::size_t n = getNodesNumber();
    if (n <= 1)
      return 0.0;
    return static_cast<double>(2 * getEdgesNumber()) / (n * (n - 1));
  }

  [[nodiscard]] std::vector<NodeType> getNeighbours(std::size_t node_id) const override {
    std::vector<NodeType> neighbours;
    neighbours.reserve(adjacency_list_.at(node_id).size());
    for (const std::size_t index : adjacency_list_.at(node_id)) {
      if (edges_[index].source == node_id) {
        neighbours.emplace_back(nodes_[edges_[index].target]);
      }
      else {
        neighbours.emplace_back(nodes_[edges_[index].source]);
      }
    }
    return neighbours;
  }

  [[nodiscard]] std::vector<EdgeType> getAdjacentEdges(std::size_t node_id) const override {
    std::vector<EdgeType> edges;
    edges.reserve(adjacency_list_.at(node_id).size());
    for (const std::size_t index : adjacency_list_.at(node_id)) {
      edges.emplace_back(edges_[index]);
    }
    return edges;
  }

  [[nodiscard]] std::size_t getDegree(std::size_t node_id) const override { return adjacency_list_.at(node_id).size(); }

  [[nodiscard]] std::map<std::size_t, std::size_t> getDegreesHistogram() const override {
    std::map<std::size_t, std::size_t> histogram;
    for (const auto& node_edges : adjacency_list_) {
      histogram[node_edges.size()]++;
    }
    return histogram;
  }

  void dfs(std::size_t start_node_id, std::function<void(const NodeType&)> callback) const override {
    std::vector<bool> visited(getNodesNumber(), false);
    std::stack<std::size_t> stack;

    stack.push(start_node_id);
    while (!stack.empty()) {
      const std::size_t node_id = stack.top();
      stack.pop();

      if (!visited[node_id]) {
        callback(nodes_[node_id]);
        visited[node_id] = true;

        for (const std::size_t neighbor_index : adjacency_list_.at(node_id)) {
          const EdgeType& neighbor = edges_[neighbor_index];
          const std::size_t target_node = (neighbor.source == node_id) ? neighbor.target : neighbor.source;

          if (!visited[target_node]) {
            stack.push(target_node);
          }
        }
      }
    }
  }

  void bfs(std::size_t start_node_id, std::function<void(const NodeType&)> callback) const override {
    std::vector<bool> visited(getNodesNumber(), false);
    std::queue<std::size_t> queue;

    queue.push(start_node_id);
    visited[start_node_id] = true;

    while (!queue.empty()) {
      const std::size_t node_id = queue.front();
      queue.pop();
      callback(nodes_[node_id]);

      for (const std::size_t neighbor_index : adjacency_list_.at(node_id)) {
        const EdgeType& neighbor = edges_[neighbor_index];
        const std::size_t target_node = (neighbor.source == node_id) ? neighbor.target : neighbor.source;

        if (!visited[target_node]) {
          visited[target_node] = true;
          queue.push(target_node);
        }
      }
    }
  }

  void saveToFile(const std::string& filename) const override {
    json j;
    j["nodes"] = json::array();
    for (std::size_t i = 0; i < getNodesNumber(); ++i) {
      j["nodes"].push_back({{"id", nodes_[i].id}});
    }

    j["edges"] = json::array();
    for (std::size_t i = 0; i < edges_.size(); ++i) {
      if (edge_exists_[i]) {
        j["edges"].push_back({{"source", edges_[i].source}, {"target", edges_[i].target}});
      }
    }

    std::ofstream file(filename);
    file << j.dump(2);
  }

  int loadFromFile(const std::string& filename) override {
    try {
      std::ifstream input_file(filename);
      json graph_data;
      input_file >> graph_data;

      nodes_.clear();
      edges_.clear();
      edge_exists_.clear();
      adjacency_list_.clear();
      active_edges_count_ = 0;

      for (const auto& node : graph_data["nodes"]) {
        addNode();
      }

      for (const auto& edge : graph_data["edges"]) {
        addEdge(edge["source"], edge["target"]);
      }
    } catch (const std::exception& e) {
      std::cerr << "Error while reading the JSON file: " << e.what() << std::endl;
      return 1;
    }

    return 0;
  }

 private:
  std::vector<NodeType> nodes_;
  std::vector<EdgeType> edges_;
  std::vector<bool> edge_exists_;
  std::vector<std::vector<std::size_t>> adjacency_list_;

  std::size_t active_edges_count_ = 0;
};
}  // namespace graph

#endif  // GRAPH_SPARSE_GRAPH_HPP
