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
  constexpr int kMinArgs = 5;
  if (argc < kMinArgs) {
    std::cerr << "Usage: " << argv[0] << " <output_file> <num_nodes> <edges_per_node> <fitness_function_type> [alpha]"
              << std::endl;
    return 1;
  }

  const std::string output_file = argv[1];
  const std::size_t num_nodes = std::stoul(argv[2]);
  const std::size_t edges_per_node = std::stoul(argv[3]);
  const int fitness_function_type = std::stoi(argv[4]);

  graph::PreferentialAttachmentGraph::FitnessFunction fitness_fn = [](std::size_t degree_k) {
    return static_cast<double>(degree_k);
  };  // default to linear

  if (fitness_function_type == 0) {
    fitness_fn = [](std::size_t degree_k) {
      return static_cast<double>(degree_k);
    };
  }
  else if (fitness_function_type == 1) {
    const double alpha = argc > 5 ? std::stod(argv[5]) : 1.0;
    fitness_fn = [alpha](std::size_t degree_k) {
      return std::pow(static_cast<double>(degree_k), alpha);
    };
  }
  else if (fitness_function_type == 2) {
    const double alpha = argc > 5 ? std::stod(argv[5]) : 1.0;
    fitness_fn = [alpha](std::size_t degree_k) {
      return std::pow(std::log(static_cast<double>(degree_k) + 1), alpha);
    };
  }

  graph::PreferentialAttachmentGraph pa_graph(num_nodes, edges_per_node, fitness_fn);
  pa_graph.saveToFile(output_file);

  return 0;
}