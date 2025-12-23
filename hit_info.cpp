#ifndef HIT_INFO_H
#define HIT_INFO_H

#include "common.hpp"

class Shape;

class HitInfo {
  public:
    bool did_hit = false;
    const Shape *shape;
    dvec3 point;
    dvec3 normal;
    double t;

    HitInfo() {}
    HitInfo(double t) : t(t) {}
};

#endif