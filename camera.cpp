#include "common.hpp"

#include "ppm.hpp"
#include "ray.cpp"
#include "shape.cpp"

using namespace glm;

class Camera {

  private:
    dvec3 position;
    dvec3 direction;
    double FOV;

    dvec3 viewport_u;
    dvec3 viewport_v;

    dvec3 dx;
    dvec3 dy;

  public:
    // Camera() {
    //     this->position = {0, 0, 0};
    //     this->direction = {0, 0, -1};
    //     this->FOV = 90;
    // }

    Camera(dvec3 position, dvec3 direction, double FOV) {
        this->position = position;
        this->direction = normalize(direction);
        this->FOV = FOV;

        dmat4 cam_matrix = get_cam_matrix();

        viewport_u =
            cam_matrix * dvec4(VIEWPORT_WIDTH, 0, 0, 1.0) - dvec4(position, 0);

        viewport_v = cam_matrix * dvec4(0, -VIEWPORT_HEIGHT, 0, 1.0) -
                     dvec4(position, 0);

        dx = viewport_u * (1.0 / WIDTH);
        dy = viewport_v * (1.0 / HEIGHT);
    }

    const dvec3 &get_position() const { return position; }

    const dvec3 &get_direction() const { return direction; }

    const double &get_fov() const { return FOV; }

    double get_focal_length(double viewport_height) const {
        return (viewport_height / 2.0) / glm::tan(glm::radians(FOV / 2.0));
    }

    dvec3 get_left_top(double viewport_width, double viewport_height) const {
        double fz = get_focal_length(viewport_height);
        dmat4 cam_matrix = get_cam_matrix();
        dvec3 out = cam_matrix * dvec4{-viewport_width / 2.0,
                                       viewport_height / 2.0, -fz, 1.0};
        return out;
    }

    dmat4 get_cam_matrix() const {
        dvec3 world_up = vec3(0.0, 1.0, 0.0);
        dvec3 forward = -direction;
        dvec3 right = normalize(cross(world_up, forward));
        dvec3 up = cross(forward, right);

        dmat4 mat(1.0);

        mat[0] = dvec4(right, 0.0);
        mat[1] = dvec4(up, 0.0);
        mat[2] = dvec4(forward, 0.0);
        mat[3] = dvec4(position, 1.0f);

        return mat;
    }

    void render(Hittable &world, int bounces, int iterations) {
        std::cout << "CWD = " << std::filesystem::current_path() << "\n";

        dmat4 cam_matrix = get_cam_matrix();

        dvec3 viewport_u =
            cam_matrix * dvec4(VIEWPORT_WIDTH, 0, 0, 1.0) - dvec4(position, 0);

        dvec3 viewport_v = cam_matrix * dvec4(0, -VIEWPORT_HEIGHT, 0, 1.0) -
                           dvec4(position, 0);

        dvec3 dx = viewport_u * (1.0 / WIDTH);
        dvec3 dy = viewport_v * (1.0 / HEIGHT);

        dvec3 lt = get_left_top(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

        std::vector<dvec3> colors(WIDTH * HEIGHT);
        int count = 1;
        while (count <= iterations) {
            for (int y = 0; y < HEIGHT; y++) {
                for (int x = 0; x < WIDTH; x++) {
                    // shoot ray through pixel center
                    Ray ray = get_ray(x, y);

                    unsigned int seed = y * WIDTH + x + count * 9483984;

                    colors[y * WIDTH + x] =
                        colors[y * WIDTH + x] * (count - 1.0) +
                        trace_ray(world, ray, bounces, seed);
                    colors[y * WIDTH + x] /= (double)count;
                }
            }

            write_ppm("out.ppm", WIDTH, HEIGHT, colors);
            count++;
        }
    }

    Ray get_ray(int x, int y) {
        dvec3 lt = get_left_top(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
        double offset_x = random_double(-.5, .5);
        double offset_y = random_double(-.5, .5);
        dvec3 pos =
            lt + (x + 0.5 + offset_x) * (dx) + (y + 0.5 + offset_y) * dy;

        return {get_position(), pos - get_position()};
    }

    dvec3 trace_ray(const Hittable &world, Ray ray, int bounces, int seed) {
        dvec3 color{1, 1, 1};
        dvec3 light{0, 0, 0};

        for (int i = 0; i <= bounces; i++) {
            HitInfo hit;

            world.get_intersection(ray, hit);

            if (hit.did_hit) {
                // dvec3 N = min_hit.normal;
                // return 0.5 * dvec3(N.x + 1, N.y + 1, N.z + 1);
                // return hit_shape->get_material().color;
                const Material &m = hit.shape->get_material();
                dvec3 emmited_light = m.emission_color * m.emission_strength;
                light += emmited_light * color;
                color *= m.color;
                dvec3 diffuse_direction = hit.normal + random_unit_vector(seed);
                dvec3 specular_direction =
                    reflect(ray.get_direction(), hit.normal);
                dvec3 direction =
                    lerp(diffuse_direction, specular_direction, m.smoothness);

                ray = {hit.point, direction};
            } else {
                dvec3 unit_direction = normalize(ray.get_direction());
                double a = 0.5 * (unit_direction.y + 1.0);
                dvec3 environment_light = ((1.0 - a) * dvec3(1.0, 1.0, 1.0) +
                                           a * dvec3(0.5, 0.7, 1.0));
                // dvec3 environment_light{0, 0, 0};
                light += color * environment_light;
                break;
            }
        }
        return light;
    }
};