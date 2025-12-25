#ifndef OBJ_H
#define OBJ_H

#include "common.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

#include "shape.cpp"

inline int parse_vertex_index(const std::string &token) {
    std::stringstream ss(token);
    int index;
    ss >> index;
    return index - 1; // OBJ is 1-based
}

Mesh load_obj_triangles(const std::string &filename, Material material) {
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Failed to open OBJ file");
    }

    std::vector<dvec3> vertices;
    Mesh triangles{material};

    std::string line;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;

        // Vertex
        if (prefix == "v") {
            double x, y, z;
            ss >> x >> y >> z;
            vertices.emplace_back(x, y, z);
        }

        // Face
        else if (prefix == "f") {
            std::vector<int> face_indices;
            std::string token;

            while (ss >> token) {
                face_indices.push_back(parse_vertex_index(token));
            }

            // Fan triangulation:
            // (v0, v1, v2), (v0, v2, v3), ...
            for (size_t i = 1; i + 1 < face_indices.size(); ++i) {
                dvec3 a = vertices[face_indices[0]];
                dvec3 b = vertices[face_indices[i]];
                dvec3 c = vertices[face_indices[i + 1]];

                Triangle *triangle = new Triangle{a, b, c, material};

                triangles.add(triangle);
            }
        }
    }

    return triangles;
}

#endif