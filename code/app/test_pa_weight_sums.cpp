#define PA_TRACK_WEIGHT_SUM

#include <cmath>
#include <cstddef>
#include <iostream>
#include <string>

#include "../src/preferential_attachment_graph.hpp"

/*
Fitness functions to test:
0. linear: f(k) = k
1. polynomial: f(k) = k^alpha
2. logarithmic: f(k) = log(k + 1)^alpha
*/

int main(int argc, char* argv[]) {
  constexpr int kMinArgs = 6;
  if (argc < kMinArgs) {
    std::cerr << "Usage: " << argv[0]
              << " <output_file> <repeat_count> <num_nodes> <edges_per_node> <fitness_function_type> [alpha]" << std::endl;
    return 1;
  }

  const std::string output_file = argv[1];
  const std::size_t repeat_count = std::stoul(argv[2]);
  const std::size_t num_nodes = std::stoul(argv[3]);
  const std::size_t edges_per_node = std::stoul(argv[4]);
  const int fitness_function_type = std::stoi(argv[5]);

  graph::PreferentialAttachmentGraph::FitnessFunction fitness_fn = [](std::size_t degree_k) {
    return static_cast<double>(degree_k);
  };  // default to linear

  if (fitness_function_type == 0) {
    fitness_fn = [](std::size_t degree_k) {
      return static_cast<double>(degree_k);
    };
  }
  else if (fitness_function_type == 1) {
    const double alpha = argc > kMinArgs ? std::stod(argv[6]) : 1.0;
    fitness_fn = [alpha](std::size_t degree_k) {
      return std::pow(static_cast<double>(degree_k), alpha);
    };
  }
  else if (fitness_function_type == 2) {
    const double alpha = argc > kMinArgs ? std::stod(argv[6]) : 1.0;
    fitness_fn = [alpha](std::size_t degree_k) {
      return std::pow(std::log(static_cast<double>(degree_k) + 1), alpha);
    };
  }

  const std::size_t total_edges = num_nodes * edges_per_node;

  std::vector<double> averaged_weight_sums;
  std::size_t recorded_len = 0;

  for (std::size_t repeat = 0; repeat < repeat_count; ++repeat) {
    graph::PreferentialAttachmentGraph pa_graph(num_nodes, edges_per_node, fitness_fn);
    const auto& cum_weights = pa_graph.getCumulativeWeights();
    if (repeat == 0) {
      recorded_len = cum_weights.size();
      averaged_weight_sums.assign(recorded_len, 0.0);
    }

    for (std::size_t i = 0; i < cum_weights.size(); ++i) {
      averaged_weight_sums[i] += cum_weights[i];
    }
  }

  std::ofstream output_stream(output_file);
  for (std::size_t i = 0; i < recorded_len; ++i) {
    output_stream << (averaged_weight_sums[i] / static_cast<double>(repeat_count)) << std::endl;
  }

  return 0;
}