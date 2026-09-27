#ifndef OBJ_H
#define OBJ_H

#include "common.hpp"

#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "scene.cpp"
#include "shape.cpp"

struct ObjIndex {
    int v = -1;
    int vn = -1;
};

static ObjIndex parse_face_token(const std::string &token) {
    ObjIndex idx;
    std::stringstream ss(token);
    std::string part;

    // v
    std::getline(ss, part, '/');
    idx.v = std::stoi(part) - 1;

    // vt (ignored)
    if (ss.peek() == '/')
        ss.get();
    else
        std::getline(ss, part, '/');

    // vn
    if (std::getline(ss, part, '/')) {
        if (!part.empty())
            idx.vn = std::stoi(part) - 1;
    }

    return idx;
}

// Loads an OBJ file as a list of GPU triangles, placed in the world by
// `transform`. Faces without normals get the geometric normal from their
// winding.
std::vector<GPUTriangle> load_obj(const std::string &filename,
                                  const GPUMaterial &material,
                                  const mat4 &transform = identity<mat4>()) {
    std::vector<GPUTriangle> triangles;

    std::vector<vec3> positions;
    std::vector<vec3> normals;

    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Failed to open OBJ file: " << filename << "\n";
        return triangles;
    }

    mat3 normal_transform = transpose(inverse(mat3(transform)));

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string type;
        ss >> type;

        // Vertex position
        if (type == "v") {
            vec3 v;
            ss >> v.x >> v.y >> v.z;
            positions.push_back(vec3(transform * vec4(v, 1)));
        }

        // Vertex normal
        else if (type == "vn") {
            vec3 n;
            ss >> n.x >> n.y >> n.z;
            normals.push_back(normalize(normal_transform * n));
        }

        // Face
        else if (type == "f") {
            std::vector<ObjIndex> face;

            std::string token;
            while (ss >> token) {
                face.push_back(parse_face_token(token));
            }

            if (face.size() < 3)
                continue;

            // Fan triangulation: (0, i, i+1)
            for (size_t i = 1; i + 1 < face.size(); ++i) {
                const ObjIndex *indices[3] = {&face[0], &face[i], &face[i + 1]};

                vec3 v[3];
                for (int j = 0; j < 3; j++)
                    v[j] = positions[indices[j]->v];

                vec3 face_normal = normalize(cross(v[1] - v[0], v[2] - v[0]));

                vec3 n[3];
                for (int j = 0; j < 3; j++)
                    n[j] = (indices[j]->vn >= 0) ? normals[indices[j]->vn]
                                                 : face_normal;

                triangles.emplace_back(v[0], v[1], v[2], n[0], n[1], n[2],
                                       material);
            }
        }
    }

    return triangles;
}

Mesh load_obj_triangles(const std::string &filename, const Material &material) {
    Mesh mesh{material};

    std::vector<glm::dvec3> positions;
    std::vector<glm::dvec3> normals;

    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Failed to open OBJ file: " << filename << "\n";
        return mesh;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string type;
        ss >> type;

        // Vertex position
        if (type == "v") {
            glm::dvec3 v;
            ss >> v.x >> v.y >> v.z;
            positions.push_back(v);
        }

        // Vertex normal
        else if (type == "vn") {
            glm::dvec3 n;
            ss >> n.x >> n.y >> n.z;
            normals.push_back(glm::normalize(n));
        }

        // Face
        else if (type == "f") {
            std::vector<ObjIndex> face;

            std::string token;
            while (ss >> token) {
                face.push_back(parse_face_token(token));
            }

            if (face.size() < 3)
                continue;

            // Fan triangulation: (0, i, i+1)
            for (size_t i = 1; i + 1 < face.size(); ++i) {
                const ObjIndex &i0 = face[0];
                const ObjIndex &i1 = face[i];
                const ObjIndex &i2 = face[i + 1];

                glm::dvec3 a = positions[i0.v];
                glm::dvec3 b = positions[i1.v];
                glm::dvec3 c = positions[i2.v];

                glm::dvec3 na = (i0.vn >= 0) ? normals[i0.vn] : glm::dvec3(0);
                glm::dvec3 nb = (i1.vn >= 0) ? normals[i1.vn] : glm::dvec3(0);
                glm::dvec3 nc = (i2.vn >= 0) ? normals[i2.vn] : glm::dvec3(0);

                Triangle *tri = new Triangle(a, b, c, na, nb, nc, material);

                mesh.add(tri);
            }
        }
    }

    return mesh;
}

#endif