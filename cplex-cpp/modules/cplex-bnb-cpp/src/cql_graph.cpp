
#include <include/cql_graph.h>

#include <fstream>
#include <sstream>
#include <utility>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <random>
#include <vector>
#include <map>
#include <iterator>
#include <limits>

#include <cassert>

CqlGraph::CqlGraph(const std::size_t n,
                   const std::size_t m,
                   std::vector<std::vector<bool>> matrix,
                   std::vector<std::set<uint64_t>> lists,
                   std::vector<std::bitset<1000>> matrix_b) : n_(n),
                                                              m_(m),
                                                              confusion_matrix_(std::move(matrix)),
                                                              adjacency_lists_(std::move(lists)),
                                                              confusion_matrix_bit_set_(std::move(matrix_b)) {}

CqlGraph CqlGraph::readGraph(const std::string &graphs_path, const std::string &graph_name) {
    std::size_t n = 0;
    std::size_t m = 0;
    std::vector<std::vector<bool>> confusion_matrix;
    std::vector<std::set<uint64_t>> adjacency_lists;
    // just big number
    std::vector<std::bitset<1000>> confusion_matrix_bit_set;

    std::fstream file;
    file.open(graphs_path + "/" + graph_name, std::fstream::in | std::fstream::out | std::fstream::app);

    std::string type;
    std::string current_line;
    std::pair<uint64_t, uint64_t> current_edge;

    while (std::getline(file, current_line)) {
        if (current_line[0] != 'p') {
            continue;
        } else {
            std::istringstream iss(current_line);
            std::vector<std::string> words;

            std::string tmp;
            while ((iss >> tmp)) {
                words.push_back(tmp);
            }
            n = std::stoul(words[words.size() - 2]);
            m = std::stoul(words[words.size() - 1]);

            break;
        }
    }

    adjacency_lists.resize(n, std::set<uint64_t>());
    confusion_matrix.resize(n, std::vector<bool>(n, false));
    confusion_matrix_bit_set.resize(n, std::bitset<1000>(false));

    while (std::getline(file, current_line)) {
        if (current_line[0] != 'e') {
            continue;
        }

        std::istringstream iss(current_line);
        iss >> type >> current_edge.first >> current_edge.second;

        adjacency_lists[current_edge.first - 1].insert(current_edge.second - 1);
        adjacency_lists[current_edge.second - 1].insert(current_edge.first - 1);

        confusion_matrix[current_edge.first - 1][current_edge.second - 1] = true;
        confusion_matrix[current_edge.second - 1][current_edge.first - 1] = true;

        confusion_matrix_bit_set[current_edge.first - 1][current_edge.second - 1] = true;
        confusion_matrix_bit_set[current_edge.second - 1][current_edge.first - 1] = true;
    }
    file.close();
    return CqlGraph(n, m, confusion_matrix, adjacency_lists, confusion_matrix_bit_set);
}

std::set<uint64_t> CqlGraph::getHeuristicMaxClique(const std::vector<uint64_t> &coloring) const {
    std::set<uint64_t> current_clique = std::set<uint64_t>();

    auto set_function = [&](uint64_t i, uint64_t j) {
        return (coloring[i] > coloring[j]) ||
               ((coloring[i] == coloring[j]) && (degree(i) > degree(j))) ||
               ((coloring[i] == coloring[j]) && (degree(i) == degree(j)) && i < j);
    };

    std::vector<uint64_t> vertices(n_);
    std::iota(vertices.begin(), vertices.end(), 0);
    std::sort(vertices.begin(), vertices.end(), set_function);

    std::bitset<1000> used;
    std::bitset<1000> candidates;

    candidates.set();

    while (true) {
        uint64_t best_candidate = 0;

        for (uint64_t v: vertices) {
            if (candidates[v] && !used[v]) {
                best_candidate = v;
                break;
            }
        }

        used[best_candidate] = true;
        current_clique.insert(best_candidate);

        candidates = candidates & confusion_matrix_bit_set_[best_candidate] & (~used);

        if (candidates.none()) {
            break;
        }
    }
    assert(isClique(current_clique));
    return current_clique;
}

// https://www.geeksforgeeks.org/graph-coloring-set-2-greedy-algorithm/
std::vector<uint64_t> CqlGraph::colorGraph(const NodesOrderingStrategy &strategy) const {
    std::vector<uint64_t> ordered_vertices = orderVertices(strategy);

    std::map<uint64_t, std::set<uint64_t>> coloring;
    std::vector<uint64_t> colored_vertices(n_, UINT64_MAX);

    std::vector<bool> used_colors(n_, false);

    for (const uint64_t v: ordered_vertices) {
        for (const uint64_t neighbor: adjacency_lists_[v]) {
            if (colored_vertices[neighbor] != UINT64_MAX) {
                used_colors[colored_vertices[neighbor]] = true;
            }
        }

        std::size_t min_color = 0;
        while (used_colors[min_color]) min_color++;

        colored_vertices[v] = min_color;
        coloring[min_color].insert(v);

        std::fill(used_colors.begin(), used_colors.end(), 0);
    }

    assert(isColoringCorrect(colored_vertices));

    return colored_vertices;
}

std::vector<uint64_t> CqlGraph::orderVertices(const NodesOrderingStrategy &strategy) const {
    std::vector<uint64_t> vertices(n_, 0);
    std::iota(vertices.begin(), vertices.end(), 0);

    switch (strategy) {
        case NodesOrderingStrategy::INDEX:
            return vertices;
        case NodesOrderingStrategy::LARGEST_DEGREE_FIRST:
            return orderVerticesLatestDegreeFirst(vertices);
        case NodesOrderingStrategy::SMALLEST_DEGREE_FIRST:
            return orderVerticesSmallestDegreeFirst(vertices);
        case NodesOrderingStrategy::SMALLEST_DEGREE_SUPPORT_FIRST:
            return orderVerticesSmallestDegreeSupportFirst(vertices);
        case NodesOrderingStrategy::RANDOM:
            return orderVerticesRandom(vertices);
        case NodesOrderingStrategy::SATURATION_LARGEST_FIRST:
            return orderVerticesSaturationLargestFirst(vertices);
    }
    return std::vector<uint64_t>();
}

std::vector<uint64_t> CqlGraph::orderVerticesLatestDegreeFirst(std::vector<uint64_t> vertices) const {
    std::sort(vertices.begin(), vertices.end(), [&](uint64_t i, uint64_t j) { return degree(i) > degree(j); });
    return vertices;
}

std::vector<uint64_t> CqlGraph::orderVerticesSmallestDegreeFirst(std::vector<uint64_t> vertices) const {
    std::sort(vertices.begin(), vertices.end(), [&](uint64_t i, uint64_t j) { return degree(i) < degree(j); });
    return vertices;
}

std::vector<uint64_t> CqlGraph::verticesSupport() const {
    std::vector<uint64_t> supports(n_, 0);
    for (std::size_t i = 0; i < n_; ++i) {
        for (const uint64_t neighbor: adjacency_lists_[i]) {
            supports[i] += degree(neighbor);
        }
    }
    return supports;
}

std::vector<uint64_t> CqlGraph::orderVerticesSmallestDegreeSupportFirst(std::vector<uint64_t> vertices) const {
    std::vector<uint64_t> mutable_degrees(n_, 0);
    std::vector<uint64_t> support = verticesSupport();
    std::vector<uint64_t> ordered_vertices;

    for (std::size_t i = 0; i < n_; ++i) {
        mutable_degrees[i] = degree(i);
    }

    auto set_function = [&](uint64_t i, uint64_t j) {
        return (mutable_degrees[i] < mutable_degrees[j]) ||
               ((mutable_degrees[i] == mutable_degrees[j]) && (support[i] < support[j])) ||
               ((mutable_degrees[i] == mutable_degrees[j]) && (support[i] == support[j]) && i < j);
    };


    std::set<uint64_t, decltype(set_function)> degree_support_index_queue(set_function);
    degree_support_index_queue.insert(vertices.begin(), vertices.end());

    while (!degree_support_index_queue.empty()) {
        uint64_t next_vertex = *degree_support_index_queue.begin();
        degree_support_index_queue.erase(degree_support_index_queue.begin());

        mutable_degrees[next_vertex] = 0;

        ordered_vertices.push_back(next_vertex);

        for (const uint64_t neighbor: adjacency_lists_[next_vertex]) {
            // check if vertex already in ordered vertexes
            if (mutable_degrees[neighbor] != 0) {
                degree_support_index_queue.erase(neighbor);
                mutable_degrees[neighbor]--;
                // Let's don't connect mutable_degrees and support. It is simpler
                support[neighbor] -= degree(next_vertex);
                degree_support_index_queue.insert(neighbor);
            }
        }
    }
    return ordered_vertices;
}

std::vector<uint64_t> CqlGraph::orderVerticesRandom(std::vector<uint64_t> vertices) const {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(vertices.begin(), vertices.end(), g);
    return vertices;
}

std::vector<uint64_t> CqlGraph::orderVerticesSaturationLargestFirst(std::vector<uint64_t> vertices) const {
    std::vector<uint64_t> ordered_vertices;
    std::vector<uint64_t> saturation(n_, 0);

    auto set_function = [&](uint64_t i, uint64_t j) {
        return (saturation[i] > saturation[j]) ||
               ((saturation[i] == saturation[j]) && (degree(i) > degree(j))) ||
               ((saturation[i] == saturation[j]) && (degree(i) == degree(j)) && i < j);
    };

    std::set<uint64_t, decltype(set_function)> saturation_queue(set_function);
    saturation_queue.insert(vertices.begin(), vertices.end());

    while (!saturation_queue.empty()) {
        uint64_t next_vertex = *saturation_queue.begin();
        saturation_queue.erase(saturation_queue.begin());

        saturation[next_vertex] = -1;

        ordered_vertices.push_back(next_vertex);
        for (const uint64_t neighbor: adjacency_lists_[next_vertex]) {
            // check if vertex already in ordered vertexes
            if (saturation[next_vertex] != -1) {
                saturation_queue.erase(neighbor);
                saturation[neighbor]++;
                saturation_queue.insert(neighbor);
            }
        }
    }
    return ordered_vertices;
}

bool CqlGraph::isColoringCorrect(std::vector<uint64_t> coloring) const {
    for (std::size_t i = 0; i < n_; ++i) {
        uint64_t current_color = coloring[i];
        for (const uint64_t neighbor: adjacency_lists_[i]) {
            if (coloring[neighbor] == current_color) {
                return false;
            }
        }
    }
    return true;
}

bool CqlGraph::isClique(const std::set<uint64_t> &clique) const {
    for (const uint64_t v1: clique) {
        for (const uint64_t v2: clique) {
            if (v1 != v2 && !(confusion_matrix_bit_set_[v1][v2])) {
                return false;
            }
        }
    }
    return true;
}
