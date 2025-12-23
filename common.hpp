#ifndef COMMONS_H
#define COMMONS_H

#include <algorithm>
#include <concepts>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <vector>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
// #include <glm/vec4.hpp>

using namespace glm;

const int WIDTH = 1920;
const int HEIGHT = 1080;

const double ASPECT_RATIO = ((double)WIDTH) / ((double)HEIGHT);

const double VIEWPORT_HEIGHT = 2.0;
const double VIEWPORT_WIDTH = ASPECT_RATIO * VIEWPORT_HEIGHT;

inline double random_double() { return std::rand() / (RAND_MAX - 1.0); }

inline double random_double(double min, double max) {
    return min + (max - min) * random_double();
}

inline dvec3 random_unit_vector(unsigned int seed) {
    dvec3 candidate;
    std::srand(seed);
    while (true) {
        candidate = {random_double(-1, 1), random_double(-1, 1),
                     random_double(-1, 1)};

        // std::printf("(%f %f %f)\n", candidate.x, candidate.y, candidate.z);
        double len_sq = dot(candidate, candidate);

        if (len_sq < 1e-160 || len_sq > 1.0)
            continue;

        candidate = normalize(candidate);
        break;
    }
    return candidate;
}

inline dvec3 random_on_hemisphere(const dvec3 &normal, unsigned int seed) {
    dvec3 on_unit_sphere = random_unit_vector(seed);
    if (dot(on_unit_sphere, normal) >= 0.0)
        return on_unit_sphere;
    else
        return -on_unit_sphere;
}

inline dvec3 arbitrary_perpendicular(const dvec3 &vec) {
    double x_abs = abs(vec.x);
    double y_abs = abs(vec.y);
    double z_abs = abs(vec.z);

    if (x_abs <= y_abs && x_abs <= z_abs)
        return {0, -vec.z, vec.y};
    else if (y_abs <= x_abs && y_abs <= z_abs)
        return {-vec.z, 0, vec.x};
    else
        return {-vec.y, vec.x, 0};
}

inline dvec3 reflect(dvec3 vec, dvec3 normal) {
    return vec - 2 * dot(vec, normal) * normal;
}

template <typename T> inline T lerp(T start, T end, double a) {
    return (1 - a) * start + a * end;
}

#endif