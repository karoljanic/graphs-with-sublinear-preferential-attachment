#ifndef PLANAR_PREFERENTIAL_ATTACHMENT_GRAPH_HPP
#define PLANAR_PREFERENTIAL_ATTACHMENT_GRAPH_HPP

#include <algorithm>   // std::min_element, std::max_element
#include <functional>  // std::function
#include <iostream>    // std::cout, std::cerr
#include <limits>      // std::numeric_limits
#include <optional>    // std::optional
#include <random>      // std::mt19937, std::random_device, std::discrete_distribution
#include <vector>      // std::vector

#include "planarity.hpp"
#include "preferential_attachment_graph.hpp"
#include "sparse_graph.hpp"

namespace graph {
class PlanarPreferentialAttachmentGraph : public SparseGraph<PANode, PAEdge> {
 public:
  enum class EdgeRemovalStrategy {
    MIN_SUM_OF_MIN_EDGE_INDICES,
    MAX_SUM_OF_MIN_EDGE_INDICES,
    MIN_SUM_OF_MAX_EDGE_INDICES,
    MAX_SUM_OF_MAX_EDGE_INDICES,
    RANDOM
  };

  struct StepInfo {
    std::size_t step;
    std::size_t new_node_id;
    std::size_t edges_added;
    std::size_t edges_removed;
    bool was_planar_before_removal;
  };

  using FitnessFunction = std::function<double(std::size_t)>;

  PlanarPreferentialAttachmentGraph() = default;

  PlanarPreferentialAttachmentGraph(std::size_t num_nodes, std::size_t m_per_step, FitnessFunction fitness_fn = nullptr,
                                    EdgeRemovalStrategy strategy = EdgeRemovalStrategy::RANDOM,
                                    std::optional<unsigned int> seed = std::nullopt)
      : SparseGraph<PANode, PAEdge>{}, m_per_step_{m_per_step}, removal_strategy_{strategy} {

    if (fitness_fn) {
      fitness_fn_ = std::move(fitness_fn);
    }

    if (!fitness_fn_) {
      fitness_fn_ = [](std::size_t degree_val) {
        return static_cast<double>(degree_val);
      };
    }

    if (seed.has_value()) {
      gen_.seed(seed.value());
      seed_ = seed.value();
    }
    else {
      std::random_device rd_device;
      gen_.seed(rd_device());
    }

    generate_graph_stepwise(num_nodes);
  }

  PlanarPreferentialAttachmentGraph(const PlanarPreferentialAttachmentGraph&) = default;
  PlanarPreferentialAttachmentGraph(PlanarPreferentialAttachmentGraph&&) noexcept = default;

  PlanarPreferentialAttachmentGraph& operator=(const PlanarPreferentialAttachmentGraph&) = default;
  PlanarPreferentialAttachmentGraph& operator=(PlanarPreferentialAttachmentGraph&&) noexcept = default;

  ~PlanarPreferentialAttachmentGraph() override = default;

  [[nodiscard]] const std::vector<StepInfo>& getStepInfo() const { return step_info_; }

 private:
  std::size_t m_per_step_;
  FitnessFunction fitness_fn_;
  EdgeRemovalStrategy removal_strategy_;
  std::mt19937 gen_;
  std::vector<StepInfo> step_info_;
  std::optional<unsigned int> seed_;

  void generate_graph_stepwise(std::size_t num_nodes) {
    if (num_nodes == 0 || m_per_step_ == 0) {
      return;
    }

    std::size_t new_node_id = this->addNode();

    for (std::size_t step = 1; step < num_nodes; ++step) {
      new_node_id = this->addNode();

      std::size_t edges_added = 0;
      while (edges_added < std::min(m_per_step_, step)) {
        std::vector<double> weights(new_node_id);
        double sum_weights = 0.0;

        for (std::size_t i = 0; i < new_node_id; ++i) {
          const double weight = fitness_fn_(this->getDegree(i));
          weights[i] = weight;
          sum_weights += weight;
        }

        if (sum_weights <= 0.0) {
          std::fill(weights.begin(), weights.end(), 1.0);
        }

        std::discrete_distribution<std::size_t> dist(weights.begin(), weights.end());
        const std::size_t target = dist(gen_);

        if (!this->edgeExists(new_node_id, target)) {
          this->addEdge(new_node_id, target);
          edges_added++;
        }
      }

      const bool was_planar = planarity::Planarity<graph::PANode, graph::PAEdge>::isPlanar(*this);
      std::size_t edges_removed = 0;

      if (!was_planar) {
        auto edges_to_remove_all = planarity::Planarity<graph::PANode, graph::PAEdge>::minimalEdgesToRestorePlanarity(*this, new_node_id);
        if (!edges_to_remove_all.empty()) {
          const auto& edges_to_remove = selectEdgesBasedOnStrategy(edges_to_remove_all);

          for (const auto& edge : edges_to_remove) {
            this->removeEdge(edge.source, edge.target);
            edges_removed++;
          }
        }
      }

      StepInfo info{
          .step = step,
          .new_node_id = new_node_id,
          .edges_added = edges_added,
          .edges_removed = edges_removed,
          .was_planar_before_removal = was_planar,
      };
      step_info_.push_back(info);

#ifdef PLANAR_PA_STEP_LOGGING
      std::cout << "Step " << step << ": Added node " << new_node_id << " with " << edges_added
                << " edges. Planar: " << (was_planar ? "YES" : "NO") << " Removed " << edges_removed << " edges" << std::endl;
#endif
    }
  }

  std::vector<PAEdge> selectEdgesBasedOnStrategy(const std::vector<std::vector<PAEdge>>& all_solutions) {
    if (all_solutions.empty()) {
      return {};
    }

    if (all_solutions.size() == 1) {
      return all_solutions[0];
    }

    switch (removal_strategy_) {
      case EdgeRemovalStrategy::MIN_SUM_OF_MIN_EDGE_INDICES: {
        // Find solution where sum of min indices of all edges is minimal
        auto best_it = std::min_element(all_solutions.begin(), all_solutions.end(), [](const auto& sol_a, const auto& sol_b) {
          std::size_t sum_a = 0;
          std::size_t sum_b = 0;

          for (const auto& edge : sol_a) {
            sum_a += std::min(edge.source, edge.target);
          }
          for (const auto& edge : sol_b) {
            sum_b += std::min(edge.source, edge.target);
          }

          return sum_a < sum_b;
        });
        return *best_it;
      }

      case EdgeRemovalStrategy::MAX_SUM_OF_MIN_EDGE_INDICES: {
        // Find solution where sum of min indices of all edges is maximal
        auto best_it = std::max_element(all_solutions.begin(), all_solutions.end(), [](const auto& sol_a, const auto& sol_b) {
          std::size_t sum_a = 0;
          std::size_t sum_b = 0;

          for (const auto& edge : sol_a) {
            sum_a += std::min(edge.source, edge.target);
          }
          for (const auto& edge : sol_b) {
            sum_b += std::min(edge.source, edge.target);
          }

          return sum_a < sum_b;
        });
        return *best_it;
      }

      case EdgeRemovalStrategy::MIN_SUM_OF_MAX_EDGE_INDICES: {
        // Find solution where sum of max indices of all edges is minimal
        auto best_it = std::min_element(all_solutions.begin(), all_solutions.end(), [](const auto& sol_a, const auto& sol_b) {
          std::size_t sum_a = 0;
          std::size_t sum_b = 0;

          for (const auto& edge : sol_a) {
            sum_a += std::max(edge.source, edge.target);
          }
          for (const auto& edge : sol_b) {
            sum_b += std::max(edge.source, edge.target);
          }

          return sum_a < sum_b;
        });
        return *best_it;
      }

      case EdgeRemovalStrategy::MAX_SUM_OF_MAX_EDGE_INDICES: {
        // Find solution where sum of max indices of all edges is maximal
        auto best_it = std::max_element(all_solutions.begin(), all_solutions.end(), [](const auto& sol_a, const auto& sol_b) {
          std::size_t sum_a = 0;
          std::size_t sum_b = 0;

          for (const auto& edge : sol_a) {
            sum_a += std::max(edge.source, edge.target);
          }
          for (const auto& edge : sol_b) {
            sum_b += std::max(edge.source, edge.target);
          }

          return sum_a < sum_b;
        });
        return *best_it;
      }

      case EdgeRemovalStrategy::RANDOM: {
        // Pick random solution
        std::uniform_int_distribution<std::size_t> dist(0, all_solutions.size() - 1);
        return all_solutions[dist(gen_)];
      }

      default:
        return all_solutions[0];
    }
  }
};

}  // namespace graph

#endif  // PLANAR_PREFERENTIAL_ATTACHMENT_GRAPH_HPP
