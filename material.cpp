#ifndef MATERIAL_H
#define MATERIAL_H

#include "common.hpp"

using namespace glm;

struct Material {
    dvec3 color;
    dvec3 emission_color;
    double emission_strength;
    double smoothness;
};

#endif