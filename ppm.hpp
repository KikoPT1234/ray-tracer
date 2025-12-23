#ifndef PPM_H
#define PPM_H

#include "common.hpp"

using namespace glm;

void write_ppm(const char *filename, int width, int height,
               const std::vector<dvec3> &pixels) {
    std::ofstream out(filename);
    if (!out)
        return;

    // PPM header
    out << "P3\n";
    out << width << " " << height << "\n";
    out << "255\n";

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const dvec3 &c = pixels[y * width + x];

            // clamp to [0, 1] and convert to [0, 255]
            int ir = static_cast<int>(255.999f * std::clamp(c.r, 0.0, 1.0));
            int ig = static_cast<int>(255.999f * std::clamp(c.g, 0.0, 1.0));
            int ib = static_cast<int>(255.999f * std::clamp(c.b, 0.0, 1.0));

            out << ir << " " << ig << " " << ib << "\n";
        }
    }
}

#endif