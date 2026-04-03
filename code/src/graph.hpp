#ifndef GRAPH_GRAPH_HPP
#define GRAPH_GRAPH_HPP

#include <concepts>    // concept, requires, std::convertible_to
#include <cstddef>     // std::size_t
#include <functional>  // std::function
#include <map>         // std::map
#include <string>      // std::string
#include <vector>      // std::vector

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace graph {
template <typename T>
concept HasId = requires(T t) {
  {t.id}->std::convertible_to<std::size_t>;
};

template <typename T>
concept HasSourceAndTarget = requires(T t) {
  {t.source}->std::convertible_to<std::size_t>;
  {t.target}->std::convertible_to<std::size_t>;
};

template <typename NodeType, typename EdgeType>
requires HasId<NodeType>&& HasSourceAndTarget<EdgeType> class Graph {
 public:
  Graph() = default;

  Graph(const Graph&) = default;
  Graph(Graph&&) noexcept = default;

  Graph& operator=(const Graph&) = default;
  Graph& operator=(Graph&&) noexcept = default;

  virtual ~Graph() = default;

  virtual std::size_t addNode() = 0;
  virtual void addEdge(std::size_t node1_id, std::size_t node2_id) = 0;
  [[nodiscard]] virtual bool edgeExists(std::size_t node1_id, std::size_t node2_id) const = 0;
  virtual void removeEdge(std::size_t node1_id, std::size_t node2_id) = 0;

  [[nodiscard]] virtual NodeType& getNode(std::size_t node_id) = 0;
  [[nodiscard]] virtual NodeType& getLastAddedNode() = 0;
  [[nodiscard]] virtual std::vector<NodeType> getNodes() const = 0;

  [[nodiscard]] virtual EdgeType& getEdge(std::size_t source, std::size_t target) = 0;
  [[nodiscard]] virtual EdgeType& getLastAddedEdge() = 0;
  [[nodiscard]] virtual std::vector<EdgeType> getEdges() const = 0;

  [[nodiscard]] virtual std::size_t getNodesNumber() const = 0;
  [[nodiscard]] virtual std::size_t getEdgesNumber() const = 0;
  [[nodiscard]] virtual double getDensity() const = 0;

  [[nodiscard]] virtual std::vector<NodeType> getNeighbours(std::size_t node_id) const = 0;
  [[nodiscard]] virtual std::vector<EdgeType> getAdjacentEdges(std::size_t node_id) const = 0;

  [[nodiscard]] virtual std::size_t getDegree(std::size_t node_id) const = 0;
  [[nodiscard]] virtual std::map<std::size_t, std::size_t> getDegreesHistogram() const = 0;

  virtual void dfs(std::size_t start_node_id, std::function<void(const NodeType&)> callback) const = 0;
  virtual void bfs(std::size_t start_node_id, std::function<void(const NodeType&)> callback) const = 0;

  virtual void saveToFile(const std::string& filename) const = 0;
  virtual int loadFromFile(const std::string& filename) = 0;
};
}  // namespace graph

#endif  // GRAPH_GRAPH_HPP
