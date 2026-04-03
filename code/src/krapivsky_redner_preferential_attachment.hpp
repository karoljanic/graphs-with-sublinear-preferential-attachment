#ifndef GRAPH_KRAPIVSKY_REDNER_PREFERENTIAL_ATTACHMENT_HPP
#define GRAPH_KRAPIVSKY_REDNER_PREFERENTIAL_ATTACHMENT_HPP

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

class KrapivskyRednerPreferentialAttachmentGraph : public SparseGraph<PANode, PAEdge> {
 public:
  using FitnessFunction = std::function<double(std::size_t)>;

  KrapivskyRednerPreferentialAttachmentGraph() = default;

  KrapivskyRednerPreferentialAttachmentGraph(std::size_t num_nodes, std::size_t p_count, FitnessFunction fitness_fn = nullptr,
                                             std::optional<unsigned int> seed = std::nullopt)
      : SparseGraph<PANode, PAEdge>{}, p_count_{p_count} {

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

  KrapivskyRednerPreferentialAttachmentGraph(const KrapivskyRednerPreferentialAttachmentGraph&) = default;
  KrapivskyRednerPreferentialAttachmentGraph(KrapivskyRednerPreferentialAttachmentGraph&&) noexcept = default;

  KrapivskyRednerPreferentialAttachmentGraph& operator=(const KrapivskyRednerPreferentialAttachmentGraph&) = default;
  KrapivskyRednerPreferentialAttachmentGraph& operator=(KrapivskyRednerPreferentialAttachmentGraph&&) noexcept = default;

  virtual ~KrapivskyRednerPreferentialAttachmentGraph() = default;

 private:
  std::size_t p_count_;
  FitnessFunction fitness_fn_;

  void generate_graph(std::size_t num_nodes, std::optional<unsigned int> seed) {
    if (num_nodes == 0 || p_count_ == 0) {
      return;
    }

    std::mt19937 gen;
    if (seed.has_value()) {
      gen.seed(seed.value());
    }
    else {
      std::random_device rd_device;
      gen.seed(rd_device());
    }

    // Start with clique of p_count_ nodes
    for (size_t new_node = 0; new_node < p_count_; ++new_node) {
      this->addNode();
      for (size_t node = 0; node < new_node; ++node) {
        this->addEdge(new_node, node);
      }
    }

    for (size_t new_node = p_count_; new_node < num_nodes; ++new_node) {
      this->addNode();

      std::vector<double> fitness_values;
      fitness_values.reserve(new_node);
      for (size_t node = 0; node < new_node; ++node) {
        fitness_values.push_back(fitness_fn_(this->getDegree(node)));
      }

      std::discrete_distribution<size_t> distribution(fitness_values.begin(), fitness_values.end());

      for (size_t i = 0; i < p_count_; ++i) {
        size_t target_node = distribution(gen);
        this->addEdge(new_node, target_node);
      }
    }
  };

}  // namespace graph

#endif  // GRAPH_KRAPIVSKY_REDNER_PREFERENTIAL_ATTACHMENT_HPP
