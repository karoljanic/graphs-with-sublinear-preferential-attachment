#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

#define PLANAR_PA_STEP_LOGGING
#include "../src/planar_preferential_attachment_graph.hpp"

using json = nlohmann::json;

/*
Fitness functions to test:
0. linear: f(k) = k
1. polynomial: f(k) = k^alpha
2. logarithmic: f(k) = log(k + 1)^alpha

Removal strategies to test:
0. MIN_SUM_OF_MIN_EDGE_INDICES: Pick solution where sum of min indices is minimal
1. MAX_SUM_OF_MIN_EDGE_INDICES: Pick solution where sum of min indices is maximal
2. MIN_SUM_OF_MAX_EDGE_INDICES: Pick solution where sum of max indices is minimal
3. MAX_SUM_OF_MAX_EDGE_INDICES: Pick solution where sum of max indices is maximal
4. RANDOM: Pick random solution
*/

int main(int argc, char* argv[]) {
  constexpr int kMinArgs = 6;
  if (argc < kMinArgs) {
    std::cerr << "Usage: " << argv[0]
              << " <output_file> <num_nodes> <m_per_step> <fitness_function_type> <removal_strategy> [alpha]" << std::endl;
    return 1;
  }

  const std::string output_file = argv[1];
  const std::size_t num_nodes = std::stoul(argv[2]);
  const std::size_t m_per_step = std::stoul(argv[3]);
  const int fitness_function_type = std::stoi(argv[4]);
  const int removal_strategy_type = std::stoi(argv[5]);
  const double alpha = argc > 6 ? std::stod(argv[6]) : 1.0;

  graph::PreferentialAttachmentGraph::FitnessFunction fitness_fn = [](std::size_t degree_k) {
    return static_cast<double>(degree_k);
  };  // default to linear

  if (fitness_function_type == 0) {
    fitness_fn = [](std::size_t degree_k) {
      return static_cast<double>(degree_k);
    };
  }
  else if (fitness_function_type == 1) {
    fitness_fn = [alpha](std::size_t degree_k) {
      return std::pow(static_cast<double>(degree_k), alpha);
    };
  }
  else if (fitness_function_type == 2) {
    fitness_fn = [alpha](std::size_t degree_k) {
      return std::pow(std::log(static_cast<double>(degree_k) + 1), alpha);
    };
  }

  graph::PlanarPreferentialAttachmentGraph::EdgeRemovalStrategy strategy =
      graph::PlanarPreferentialAttachmentGraph::EdgeRemovalStrategy::RANDOM;
  std::string strategy_name = "RANDOM";

  if (removal_strategy_type == 0) {
    strategy = graph::PlanarPreferentialAttachmentGraph::EdgeRemovalStrategy::MIN_SUM_OF_MIN_EDGE_INDICES;
    strategy_name = "MIN_SUM_OF_MIN_EDGE_INDICES";
  }
  else if (removal_strategy_type == 1) {
    strategy = graph::PlanarPreferentialAttachmentGraph::EdgeRemovalStrategy::MAX_SUM_OF_MIN_EDGE_INDICES;
    strategy_name = "MAX_SUM_OF_MIN_EDGE_INDICES";
  }
  else if (removal_strategy_type == 2) {
    strategy = graph::PlanarPreferentialAttachmentGraph::EdgeRemovalStrategy::MIN_SUM_OF_MAX_EDGE_INDICES;
    strategy_name = "MIN_SUM_OF_MAX_EDGE_INDICES";
  }
  else if (removal_strategy_type == 3) {
    strategy = graph::PlanarPreferentialAttachmentGraph::EdgeRemovalStrategy::MAX_SUM_OF_MAX_EDGE_INDICES;
    strategy_name = "MAX_SUM_OF_MAX_EDGE_INDICES";
  }
  else if (removal_strategy_type == 4) {
    strategy = graph::PlanarPreferentialAttachmentGraph::EdgeRemovalStrategy::RANDOM;
    strategy_name = "RANDOM";
  }

  std::cout << "Generating planar preferential attachment graph..." << std::endl;
  std::cout << "Nodes: " << num_nodes << ", Edges per step: " << m_per_step << std::endl;
  std::cout << "Removal strategy: " << strategy_name << std::endl;

  graph::PlanarPreferentialAttachmentGraph pa_graph(num_nodes, m_per_step, fitness_fn, strategy);

  const auto& step_info = pa_graph.getStepInfo();
  json stats = json::array();

  for (const auto& info : step_info) {
    json step_json;
    step_json["step"] = info.step;
    step_json["new_node_id"] = info.new_node_id;
    step_json["edges_added"] = info.edges_added;
    step_json["edges_removed"] = info.edges_removed;
    step_json["was_planar_before_removal"] = info.was_planar_before_removal;
    stats.push_back(step_json);
  }

  pa_graph.saveToFile(output_file);

  std::ifstream input_file(output_file);
  json graph_data = json::parse(input_file);
  input_file.close();

  graph_data["steps"] = stats;

  std::ofstream output_stream(output_file);
  output_stream << graph_data.dump(2);
  output_stream.close();

  std::cout << "\n=== Summary ===" << std::endl;
  std::cout << "Final graph:" << std::endl;
  std::cout << "  Nodes: " << pa_graph.getNodesNumber() << std::endl;
  std::cout << "  Edges: " << pa_graph.getEdgesNumber() << std::endl;

  std::size_t total_edges_added = 0;
  std::size_t total_edges_removed = 0;
  std::size_t non_planar_steps = 0;

  for (const auto& info : step_info) {
    total_edges_added += info.edges_added;
    total_edges_removed += info.edges_removed;
    if (!info.was_planar_before_removal) {
      non_planar_steps++;
    }
  }

  std::cout << "  Total edges added: " << total_edges_added << std::endl;
  std::cout << "  Total edges removed: " << total_edges_removed << std::endl;
  std::cout << "  Non-planar steps: " << non_planar_steps << std::endl;
  std::cout << "\nGraph saved to: " << output_file << std::endl;

  return 0;
}