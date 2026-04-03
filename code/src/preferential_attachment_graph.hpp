#ifndef GRAPH_PREFERENTIAL_ATTACHMENT_GRAPH_HPP
#define GRAPH_PREFERENTIAL_ATTACHMENT_GRAPH_HPP

#include <functional>  // std::function
#include <numeric>     // std::iota
#include <optional>    // std::optional
#include <random>      // std::mt19937, std::random_device, std::discrete_distribution
#include <vector>      // std::vector

#include "sparse_graph.hpp"

namespace graph {
struct PANode {
  std::size_t id;
};

struct PAEdge {
  std::size_t source;
  std::size_t target;
};

class PreferentialAttachmentGraph : public SparseGraph<PANode, PAEdge> {
 public:
  using FitnessFunction = std::function<double(std::size_t)>;

  PreferentialAttachmentGraph() = default;

  PreferentialAttachmentGraph(std::size_t num_nodes, std::size_t m_count, FitnessFunction fitness_fn = nullptr,
                              std::optional<unsigned int> seed = std::nullopt)
      : SparseGraph<PANode, PAEdge>{}, m_count_{m_count} {

    if (fitness_fn) {
      fitness_fn_ = std::move(fitness_fn);
    }

    if (!fitness_fn_) {
      fitness_fn_ = [](std::size_t degree_val) {
        return static_cast<double>(degree_val);
      };
    }

    generate_graph(num_nodes, seed);
  }

  PreferentialAttachmentGraph(const PreferentialAttachmentGraph&) = default;
  PreferentialAttachmentGraph(PreferentialAttachmentGraph&&) noexcept = default;

  PreferentialAttachmentGraph& operator=(const PreferentialAttachmentGraph&) = default;
  PreferentialAttachmentGraph& operator=(PreferentialAttachmentGraph&&) noexcept = default;

  ~PreferentialAttachmentGraph() override = default;

#ifdef PA_TRACK_WEIGHT_SUM
  [[nodiscard]] const std::vector<double>& getCumulativeWeights() const { return cumulative_weights_; }
#endif

#ifdef PA_TRACK_DEGREES
  [[nodiscard]] const std::vector<std::vector<std::size_t>>& getCumulativeDegrees() const { return cumulative_degrees_; }
#endif

 private:
  std::size_t m_count_;
  FitnessFunction fitness_fn_;

#ifdef PA_TRACK_WEIGHT_SUM
  std::vector<double> cumulative_weights_;
#endif

#ifdef PA_TRACK_DEGREES
  std::vector<std::vector<std::size_t>> cumulative_degrees_;
#endif

  void generate_graph(std::size_t num_nodes, std::optional<unsigned int> seed) {
    if (num_nodes == 0 || m_count_ == 0) {
      return;
    }

    const std::size_t total_tree_nodes = num_nodes * m_count_;

    std::vector<std::pair<std::size_t, std::size_t>> tree_edges;
    tree_edges.reserve(total_tree_nodes - 1);
    std::vector<std::size_t> temp_degrees(total_tree_nodes, 0);

    std::mt19937 gen;
    if (seed.has_value()) {
      gen.seed(seed.value());
    }
    else {
      std::random_device rd_device;
      gen.seed(rd_device());
    }

    for (std::size_t new_node = 1; new_node < total_tree_nodes; ++new_node) {
      std::vector<double> weights(new_node);
      double sum_weights = 0.0;

      for (std::size_t v_idx = 0; v_idx < new_node; ++v_idx) {
        const double weight = fitness_fn_(temp_degrees[v_idx]);
        weights[v_idx] = weight;
        sum_weights += weight;
      }

      if (sum_weights == 0.0) {
        std::fill(weights.begin(), weights.end(), 1.0);
      }

      std::discrete_distribution<std::size_t> dist(weights.begin(), weights.end());
      const std::size_t target = dist(gen);

      tree_edges.emplace_back(new_node, target);
      temp_degrees[new_node]++;
      temp_degrees[target]++;

#ifdef PA_TRACK_WEIGHT_SUM
      if (new_node > 1) {
        cumulative_weights_.push_back(sum_weights);
      }
#endif

#ifdef PA_TRACK_DEGREES
      std::vector<std::size_t> degree_count(temp_degrees.begin(), temp_degrees.end());
      cumulative_degrees_.push_back(std::move(degree_count));
#endif
    }

#ifdef PA_TRACK_WEIGHT_SUM
    double sum_weights = 0.0;
    for (std::size_t v_idx = 0; v_idx < total_tree_nodes; ++v_idx) {
      sum_weights += fitness_fn_(temp_degrees[v_idx]);
    }
    cumulative_weights_.push_back(sum_weights);
#endif

    for (std::size_t i = 0; i < num_nodes; ++i) {
      this->addNode();
    }

    for (const auto& edge : tree_edges) {
      const std::size_t new_u = edge.first / m_count_;
      const std::size_t new_v = edge.second / m_count_;

      if (new_u != new_v) {
        this->addEdge(new_u, new_v);
      }
    }
  }
};

}  // namespace graph

#endif  // GRAPH_PREFERENTIAL_ATTACHMENT_GRAPH_HPP
