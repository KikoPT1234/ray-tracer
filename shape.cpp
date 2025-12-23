#ifndef SHAPE_I
#define SHAPE_I

#include "hit_info.cpp"

#include "common.hpp"
#include "material.cpp"
#include "ray.cpp"

using namespace glm;

class Hittable {
  public:
    virtual ~Hittable() = default;
    virtual void get_intersection(Ray ray, HitInfo &info) const = 0;
};

class HitList : public Hittable {

  private:
    std::vector<Hittable *> hittables;

  public:
    void get_intersection(Ray ray, HitInfo &info) const {
        info.t = std::numeric_limits<double>::max();

        for (Hittable *hittable : hittables) {
            HitInfo temp;
            hittable->get_intersection(ray, temp);

            if (temp.did_hit) {
                if (temp.t < info.t && temp.t > 0.001) {
                    info = temp;
                }
            }
        }
    }

    void add(Hittable *hittable) { hittables.push_back(hittable); }
};

class Shape : public Hittable {
  protected:
    Material material;
    dvec3 position;
    Shape(dvec3 position, Material material)
        : position(position), material(material) {}

  public:
    const Material &get_material() const { return material; }
    const dvec3 &get_position() const { return position; }
    virtual ~Shape() = default;
    virtual void get_intersection(Ray ray, HitInfo &info) const = 0;
};

class Sphere : public Shape {
    double radius;

  public:
    Sphere(dvec3 position, Material material, double radius)
        : Shape(position, material), radius(radius) {}

    void get_intersection(Ray ray, HitInfo &info) const override {
        dvec3 op = position - ray.get_origin();
        dvec3 rd = ray.get_direction();
        double a = dot(rd, rd);
        double h = dot(rd, op);
        double c = dot(op, op) - radius * radius;

        double discriminant = h * h - a * c;
        if (discriminant < 0) {
            info.did_hit = false;
            return;
        }

        double x;
        if (discriminant == 0)
            x = h / a;
        else
            x = (h - sqrt(discriminant)) / a;

        if (x <= 1e-10)
            info.did_hit = false;
        else {
            info.did_hit = true;
            info.shape = this;
            info.t = x;
            info.point = ray.offset(x);
            info.normal = (info.point - get_position()) / radius;
        }
    }
};

#endif