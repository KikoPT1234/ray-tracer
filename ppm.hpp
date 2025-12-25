#ifndef PPM_H
#define PPM_H

#include "common.hpp"

using namespace glm;

double EXPOSURE = 2;

void write_ppm(const char *filename, int width, int height,
               const std::vector<dvec3> &pixels) {
    std::ofstream out(filename);
    if (!out)
        return;

    // PPM header
    out << "P3\n";
    out << width << " " << height << "\n";
    out << "255\n";

    auto aces = [](double x) {
        const double a = 2.51;
        const double b = 0.03;
        const double c = 2.43;
        const double d = 0.59;
        const double e = 0.14;
        return std::clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
    };

    auto gamma_correct = [](double x) { return pow(x, 1.0 / 2.2); };

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            dvec3 c = pixels[y * width + x];

            // exposed
            c *= EXPOSURE;

            // ACES tonemap
            c.r = aces(c.r);
            c.g = aces(c.g);
            c.b = aces(c.b);

            // // gamma correction
            c.r = gamma_correct(c.r);
            c.g = gamma_correct(c.g);
            c.b = gamma_correct(c.b);

            // clamp to [0, 1] and convert to [0, 255]
            int ir = static_cast<int>(255.999 * clamp(c.r, 0.0, 1.0));
            int ig = static_cast<int>(255.999 * clamp(c.g, 0.0, 1.0));
            int ib = static_cast<int>(255.999 * clamp(c.b, 0.0, 1.0));

            out << ir << " " << ig << " " << ib << "\n";
        }
    }

    std::printf("Written\n");
}

#endif