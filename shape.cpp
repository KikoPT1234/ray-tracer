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

    ~HitList() {
        for (Hittable *hittable : hittables) {
            delete hittable;
        }
    }

    void add(Hittable *hittable) { hittables.push_back(hittable); }
};

class Shape : public Hittable {
  protected:
    Material material;
    Shape(Material material) : material(material) {}

  public:
    const Material &get_material() const { return material; }
    virtual ~Shape() = default;
    virtual void get_intersection(Ray ray, HitInfo &info) const = 0;
};

class Triangle : public Shape {
  private:
    dvec3 a;
    dvec3 b;
    dvec3 c;
    dvec3 normal;
    double D;

  public:
    Triangle(dvec3 a, dvec3 b, dvec3 c, Material material)
        : a(a), b(b), c(c), Shape(material) {
        normal = normalize(cross(b - a, c - a));
        D = dot(a, normal);
    }

    // https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
    void get_intersection(Ray ray, HitInfo &info) const {
        constexpr double epsilon = std::numeric_limits<double>::epsilon();

        dvec3 edge1 = b - a;
        dvec3 edge2 = c - a;
        dvec3 ray_cross_e2 = cross(ray.get_direction(), edge2);
        double determinant = dot(edge1, ray_cross_e2);

        if (determinant > -epsilon && determinant < epsilon) {
            info.did_hit = false;
            return;
        }

        double inv_det = 1.0 / determinant;
        dvec3 s = ray.get_origin() - a;
        double u = inv_det * dot(s, ray_cross_e2);

        if ((u < 0 && abs(u) > epsilon) || (u > 1 && abs(u - 1) > epsilon)) {
            info.did_hit = false;
            return;
        }

        dvec3 s_cross_e1 = cross(s, edge1);
        double v = inv_det * dot(ray.get_direction(), s_cross_e1);

        if ((v < 0 && abs(v) > epsilon) ||
            (u + v > 1 && abs(u + v - 1) > epsilon)) {
            info.did_hit = false;
            return;
        }

        double t = inv_det * dot(edge2, s_cross_e1);

        if (t <= epsilon) {
            info.did_hit = false;
            return;
        }

        info.did_hit = true;
        info.t = t;
        info.normal = normal;
        info.point = ray.offset(t);
        info.shape = this;
    }

    void shift(const dvec3 &offset) {
        a += offset;
        b += offset;
        c += offset;
    }
};

class Mesh : public Shape {

  private:
    std::vector<Triangle *> triangles;
    dvec3 position;

  public:
    Mesh(const dvec3 &position, Material material)
        : position(position), Shape(material) {}

    Mesh(Material material) : Shape(material) { position = {0, 0, 0}; }

    void get_intersection(Ray ray, HitInfo &info) const {
        info.t = std::numeric_limits<double>::max();

        for (Hittable *triangle : triangles) {
            HitInfo temp;
            triangle->get_intersection(ray, temp);

            if (temp.did_hit) {
                if (temp.t < info.t && temp.t > 0.001) {
                    info = temp;
                }
            }
        }
    }

    ~Mesh() {
        for (Triangle *triangle : triangles) {
            delete triangle;
        }
    }

    void add(Triangle *triangle) {
        triangle->shift(position);
        triangles.push_back(triangle);
    }

    void set_position(const dvec3 &position) {
        this->position = position;
        for (Triangle *triangle : triangles) {
            triangle->shift(position);
        }
    }
};

class Sphere : public Shape {
    double radius;
    dvec3 position;

  public:
    Sphere(dvec3 position, Material material, double radius)
        : Shape(material), position(position), radius(radius) {}

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

    const dvec3 &get_position() const { return position; }
};

#endif