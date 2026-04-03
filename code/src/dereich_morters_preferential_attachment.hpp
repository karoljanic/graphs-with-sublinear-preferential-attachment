#ifndef GRAPH_DERICH_MORTERS_PREFERENTIAL_ATTACHMENT_HPP
#define GRAPH_DERICH_MORTERS_PREFERENTIAL_ATTACHMENT_HPP

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

class DereichMortersPreferentialAttachmentGraph : public SparseGraph<PANode, PAEdge> {
 public:
  using FitnessFunction = std::function<double(std::size_t)>;

  DereichMortersPreferentialAttachmentGraph() = default;

  DereichMortersPreferentialAttachmentGraph(std::size_t num_nodes, FitnessFunction fitness_fn = nullptr,
                                            std::optional<unsigned int> seed = std::nullopt)
      : SparseGraph<PANode, PAEdge>{} {

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

  DereichMortersPreferentialAttachmentGraph(const DereichMortersPreferentialAttachmentGraph&) = default;
  DereichMortersPreferentialAttachmentGraph(DereichMortersPreferentialAttachmentGraph&&) noexcept = default;

  DereichMortersPreferentialAttachmentGraph& operator=(const DereichMortersPreferentialAttachmentGraph&) = default;
  DereichMortersPreferentialAttachmentGraph& operator=(DereichMortersPreferentialAttachmentGraph&&) noexcept = default;

  virtual ~DereichMortersPreferentialAttachmentGraph() = default;

 private:
  FitnessFunction fitness_fn_;

  void generate_graph(std::size_t num_nodes, std::optional<unsigned int> seed) {
    if (num_nodes == 0) {
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

    std::uniform_real_distribution<double> uniform_dist(0.0, 1.0);

    for (size_t new_node = 1; new_node < num_nodes; ++new_node) {
      this->add_node(new_node);

      for (size_t node = 0; node < new_node; ++node) {
        double p = fitness_fn_(this->getDegree(node)) / static_cast<double>(new_node);
        if (uniform_dist(gen) < p) {
          this->add_edge(new_node, node);
        }
      }
    }
  }
};
}  // namespace graph

#endif  // GRAPH_DERICH_MORTERS_PREFERENTIAL_ATTACHMENT_HPP
