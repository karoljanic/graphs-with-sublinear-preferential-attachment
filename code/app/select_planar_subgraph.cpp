#include <cmath>
#include <fstream>
#include <iostream>
#include <string>

#include "../src/max_planar_subgraph.hpp"
#include "../src/preferential_attachment_graph.hpp"


int main(int argc, char* argv[]) {
  constexpr int kMinArgs = 3;
  if (argc < kMinArgs) {
    std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
    return 1;
  }

  const std::string input_file = argv[1];
  const std::string output_file = argv[2];

  graph::PreferentialAttachmentGraph graph;
  graph.loadFromFile(input_file);

  graph::PreferentialAttachmentGraph max_planar_subgraph;
  planarity::MaxPlanarSubgraph<graph::PANode, graph::PAEdge>::mstBased(graph, max_planar_subgraph);
  // planarity::MaxPlanarSubgraph<graph::PANode, graph::PAEdge>::cactusBased(graph, max_planar_subgraph);

  max_planar_subgraph.saveToFile(output_file);

  return 0;
}