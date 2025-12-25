#ifndef SHAPE_I
#define SHAPE_I

#include <glm/gtc/matrix_transform.hpp>

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
        constexpr double epsilon = std::numeric_limits<double>::epsilon();
        info.t = std::numeric_limits<double>::max();

        for (Hittable *hittable : hittables) {
            HitInfo temp;
            hittable->get_intersection(ray, temp);

            if (temp.did_hit) {
                if (temp.t < info.t && temp.t > epsilon) {
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
    dvec3 na;
    dvec3 nb;
    dvec3 nc;
    dvec3 face_normal;

  public:
    Triangle(dvec3 a, dvec3 b, dvec3 c, dvec3 na, dvec3 nb, dvec3 nc,
             Material material)
        : a(a), b(b), c(c), na(na), nb(nb), nc(nc), Shape(material) {
        face_normal = normalize(cross(b - a, c - a));
        // this->na = na;
        // this->nb = face_normal;
        // this->nc = face_normal;
    }

    void set_normals(dvec3 na, dvec3 nb, dvec3 nc) {
        this->na = na;
        this->nb = nb;
        this->nc = nc;
    }

    // https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
    void get_intersection(Ray ray, HitInfo &info) const {
        info.shape = this;
        constexpr double eps = std::numeric_limits<double>::epsilon();
        dvec3 ab = b - a;
        dvec3 ac = c - a;
        dvec3 normal_vector = cross(ab, ac);
        dvec3 ao = ray.get_origin() - a;
        dvec3 dao = cross(ao, ray.get_direction());

        double determinant = -dot(ray.get_direction(), normal_vector);
        if (determinant < eps) {
            info.did_hit = false;
            return;
        }
        double invDet = 1.0 / determinant;

        // Calculate dst to triangle & barycentric coordinates of intersection

        double u = dot(ac, dao) * invDet;
        if (u < eps || u - eps > 1.0) {
            info.did_hit = false;
            return;
        }

        double v = -dot(ab, dao) * invDet;
        if (v < eps || (v + u - eps) > 1.0) {
            info.did_hit = false;
            return;
        }

        double dst = dot(ao, normal_vector) * invDet;
        if (dst < eps) {
            info.did_hit = false;
            return;
        }

        double w = 1 - u - v;

        // Initialize hit info
        info.did_hit = true;
        info.point = ray.offset(dst);
        info.normal = normalize(na * w + nb * u + nc * v);
        info.t = dst;
    }

    void shift(const dvec3 &offset) {
        a += offset;
        b += offset;
        c += offset;
    }

    void transform(const dmat4 &matrix) {
        a = matrix * dvec4(a, 1);
        b = matrix * dvec4(b, 1);
        c = matrix * dvec4(c, 1);

        dmat3 no_translation = matrix;
        dmat3 normal_matrix = transpose(inverse(no_translation));
        na = normalize(normal_matrix * na);
        nb = normalize(normal_matrix * nb);
        nc = normalize(normal_matrix * nc);
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
        constexpr double epsilon = 1e-6;

        info.t = std::numeric_limits<double>::max();

        for (Hittable *triangle : triangles) {
            HitInfo temp;
            triangle->get_intersection(ray, temp);

            if (temp.did_hit) {
                if (temp.t < info.t && temp.t > epsilon) {
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
        std::printf("%d\n", triangles.size());
        for (int i = 0; i < triangles.size(); i++) {
            Triangle *triangle = triangles[i];
            // std::printf("%d\n", i);
            triangle->shift(position);
        }
    }

    void set_rotation(const dvec3 &axis, double angle) {
        dmat4 transformation = identity<dmat4>();
        transformation = translate(transformation, position);
        transformation = rotate(transformation, angle, axis);
        transformation = translate(transformation, -position);

        for (Triangle *triangle : triangles) {
            triangle->transform(transformation);
        }
    }

    void set_scale(double factor) {
        dmat4 transformation = identity<dmat4>();
        transformation = translate(transformation, position);
        transformation = scale(transformation, {factor, factor, factor});
        transformation = translate(transformation, -position);

        for (Triangle *triangle : triangles) {
            triangle->transform(transformation);
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