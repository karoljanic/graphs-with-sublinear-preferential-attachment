#define PA_TRACK_DEGREES

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

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
  if (repeat_count == 0) {
    std::cerr << "repeat_count must be > 0" << std::endl;
    return 1;
  }
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

  std::vector<std::vector<double>> aggregated_counts;
  std::size_t num_records = 0;

  for (std::size_t repeat = 0; repeat < repeat_count; ++repeat) {
    graph::PreferentialAttachmentGraph pa_graph(num_nodes, edges_per_node, fitness_fn);
    const auto& cum_degrees = pa_graph.getCumulativeDegrees();

    if (repeat == 0) {
      num_records = cum_degrees.size();
      aggregated_counts.resize(num_records);
      for (std::size_t i = 0; i < num_records; ++i) {
        const auto& degs = cum_degrees[i];
        aggregated_counts[i].assign(degs.size(), 0.0);
      }
    }
    else {
      if (cum_degrees.size() > num_records) {
        const std::size_t old = num_records;
        aggregated_counts.resize(cum_degrees.size());
        for (std::size_t i = old; i < cum_degrees.size(); ++i) {
          aggregated_counts[i].clear();
        }
        num_records = cum_degrees.size();
      }
    }

    for (std::size_t i = 0; i < cum_degrees.size(); ++i) {
      const auto& degs = cum_degrees[i];
      if (aggregated_counts[i].size() < degs.size()) {
        aggregated_counts[i].resize(degs.size(), 0.0);
      }
      for (std::size_t v = 0; v < degs.size(); ++v) {
        aggregated_counts[i][v] += static_cast<double>(degs[v]);
      }
    }
  }

  std::ofstream degrees_output_stream(output_file);
  for (std::size_t i = 0; i < aggregated_counts.size(); ++i) {
    const auto& vals = aggregated_counts[i];
    for (std::size_t v = 0; v < vals.size(); ++v) {
      const double avg = vals[v] / static_cast<double>(repeat_count);
      degrees_output_stream << " " << avg;
    }
    degrees_output_stream << std::endl;
  }

  return 0;
}