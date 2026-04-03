#ifndef DISJOINT_SET_HPP
#define DISJOINT_SET_HPP

#include <vector>

class DisjointSet {
 public:
  DisjointSet() = default;
  explicit DisjointSet(size_t vertices_number) : parent_(vertices_number), rank_(vertices_number) {}

  DisjointSet(const DisjointSet&) = default;
  DisjointSet(DisjointSet&&) = default;

  DisjointSet& operator=(const DisjointSet&) = default;
  DisjointSet& operator=(DisjointSet&&) = default;

  ~DisjointSet() = default;

  void makeSet(size_t vertex) {
    parent_[vertex] = vertex;
    rank_[vertex] = 0;
  }

  size_t findSet(size_t vertex) {
    if (vertex != parent_[vertex]) {
      parent_[vertex] = findSet(parent_[vertex]);
    }
    return parent_[vertex];
  }

  void unionSets(size_t vertex1, size_t vertex2) {
    size_t vertex1_root = findSet(vertex1);
    size_t vertex2_root = findSet(vertex2);

    if (vertex1_root != vertex2_root) {
      if (rank_[vertex1_root] < rank_[vertex2_root]) {
        std::swap(vertex1_root, vertex2_root);
      }
      parent_[vertex2_root] = vertex1_root;
      if (rank_[vertex1_root] == rank_[vertex2_root]) {
        rank_[vertex1_root]++;
      }
    }
  }

 private:
  std::vector<size_t> parent_;
  std::vector<size_t> rank_;
};

#endif  // DISJOINT_SET_HPP