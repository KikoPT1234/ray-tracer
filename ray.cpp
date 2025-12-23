#include "common.hpp"
#ifndef RAYS
#define RAYS

using namespace glm;

class Ray {
    dvec3 origin;
    dvec3 direction;

  public:
    Ray(dvec3 origin, dvec3 direction)
        : origin(origin), direction(normalize(direction)) {};

    dvec3 offset(double d) const { return origin + d * direction; }

    const dvec3 &get_origin() const { return origin; }

    const dvec3 &get_direction() const { return direction; }
};

#endif