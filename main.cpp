#include "common.hpp"

#include "camera.cpp"
#include "ppm.hpp"
#include "ray.cpp"
#include "shape.cpp"

using namespace glm;

int main(int, char **) {

    HitList world;

    // Red
    Material m1{dvec3{0.8, .1, 0.2}, dvec3{1, 1, 1}, 0, .9};
    Sphere sphere1{dvec3{0, 0, -10}, m1, 4};

    // Green
    Material m2{dvec3{.2, 0.8, 0.3}, dvec3{1, 1, 1}, 0, .9};
    Sphere sphere2{dvec3{2, 8, -14}, m2, 4};

    // Blue
    Material m3{dvec3{.2, 0.4, 0.7}, dvec3{1, 1, 1}, 0, .9};
    Sphere sphere3{dvec3{0, -50, -7}, m3, 46};

    // White
    Material m4{dvec3{1, 1, 1}, dvec3{1, 1, 1}, 0, .9};
    Sphere sphere4{dvec3{2, 8, -6}, m4, 4};

    // Sun
    Material m5{dvec3{0, 0, 0}, dvec3{1, 1, 1}, 5, 0};
    Sphere sphere5{dvec3{10, 0, -15}, m5, 4};

    world.add(&sphere1);
    world.add(&sphere2);
    world.add(&sphere3);
    world.add(&sphere4);
    world.add(&sphere5);

    Camera camera{{-15, 5, -30}, {10, 0, 10}, 70};

    camera.render(world, 10, 1000);
}
