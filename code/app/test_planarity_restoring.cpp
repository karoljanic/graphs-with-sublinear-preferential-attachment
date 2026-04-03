#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

#include "../src/preferential_attachment_graph.hpp"
#include "../src/planarity.hpp"

using json = nlohmann::json;

std::vector<std::vector<graph::PAEdge>> minimalEdgesToRestorePlanarity(const graph::PreferentialAttachmentGraph& graph, size_t edge_node1, size_t edge_node2) {
  if (planarity::Planarity<graph::PANode, graph::PAEdge>::isPlanar(graph)) {
    return {{}};
  }

  std::vector<graph::PAEdge> edges;
  for (const auto& edge : graph.getEdges()) {
    if (edge.source == edge_node1 && edge.target == edge_node2) {
      continue;
    }
    
    if(edge.source == edge_node2 && edge.target == edge_node1) {
      continue;
    }
    
    edges.push_back(edge);
  }

  for (size_t edges_to_remove = 1; edges_to_remove < edges.size(); ++edges_to_remove) {
    std::vector<int> mask(edges.size(), 0);
    std::fill(mask.end() - edges_to_remove, mask.end(), 1);

    std::vector<std::vector<graph::PAEdge>> results;
    do {
      graph::PreferentialAttachmentGraph subgraph = graph;
      std::vector<graph::PAEdge> removed_edges;
      removed_edges.reserve(edges_to_remove);

      for (size_t i = 0; i < edges.size(); ++i) {
        if (mask[i] == 1) {
          subgraph.removeEdge(edges[i].source, edges[i].target);
          removed_edges.push_back(edges[i]);
        }
      }

      if (planarity::Planarity<graph::PANode, graph::PAEdge>::isPlanar(subgraph)) {
        results.push_back(removed_edges);
      }

    } while (std::next_permutation(mask.begin(), mask.end()));

    if (!results.empty()) {
      return results;
    }
  }

  return {edges};
}

int main(int argc, char* argv[]) {
  constexpr int kMinArgs = 2;
  if (argc < kMinArgs) {
    std::cerr << "Usage: " << argv[0] << " <input_file> <output_dir>" << std::endl;
    return 1;
  }

  graph::PreferentialAttachmentGraph graph;
  graph.loadFromFile(argv[1]);

  size_t repetition = 0;
  for (const auto& n1 : graph.getNodes()) {
    for (const auto& n2 : graph.getNodes()) {
      const size_t node1 = n1.id;
      const size_t node2 = n2.id;
      if (node1 >= node2) {
        continue;
      }

      if (graph.edgeExists(node1, node2)) {
        continue;
      }

      graph::PreferentialAttachmentGraph modified_graph = graph;
      modified_graph.addEdge(node1, node2);

      std::vector<std::vector<graph::PAEdge>> options = minimalEdgesToRestorePlanarity(modified_graph, node1, node2);

      std::ofstream output_file;
      output_file.open(argv[2] + std::to_string(repetition++) + ".json");

      json j;
      j["added_edge"] = { {"source", node1}, {"target", node2} };
      j["min_edges_to_remove"] = options.empty() ? 0 : options[0].size();
      j["removal_options"] = json::array();
      for (const auto& opt : options) {
        json opt_arr = json::array();
        for (const auto& e : opt) {
          opt_arr.push_back({{"source", e.source}, {"target", e.target}});
        }
        j["removal_options"].push_back(opt_arr);
      }

      output_file << j.dump(2);
    }
  }

  return 0;
}