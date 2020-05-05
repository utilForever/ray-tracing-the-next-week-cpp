// Copyright (c) 2020 Chris Ohk

// I am making my contributions/submissions to this project solely in my
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

// It is based on Ray Tracing in One Weekend book.
// References: https://raytracing.github.io

#include "camera.hpp"
#include "checker_texture.hpp"
#include "common.hpp"
#include "dielectric.hpp"
#include "hittable_list.hpp"
#include "lambertian.hpp"
#include "metal.hpp"
#include "moving_sphere.hpp"
#include "noise_texture.hpp"
#include "solid_color.hpp"
#include "sphere.hpp"

#include <iostream>

vec3 ray_color(const ray& r, const hittable& world, int depth)
{
    hit_record rec;

    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0)
    {
        return vec3{0, 0, 0};
    }

    if (world.hit(r, 0.001, infinity, rec))
    {
        ray scattered;
        vec3 attenuation;

        if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
        {
            return attenuation * ray_color(scattered, world, depth - 1);
        }

        return vec3{0, 0, 0};
    }

    const vec3 unit_direction = unit_vector(r.direction());
    const auto t = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - t) * vec3{1.0, 1.0, 1.0} + t * vec3{0.5, 0.7, 1.0};
}

hittable_list random_scene()
{
    hittable_list world;

    auto checker = std::make_shared<checker_texture>(
        std::make_shared<solid_color>(0.2, 0.3, 0.1),
        std::make_shared<solid_color>(0.9, 0.9, 0.9));
    world.add(std::make_shared<sphere>(point3{0, -1000, 0}, 1000,
                                       std::make_shared<lambertian>(checker)));

    for (int a = -10; a < 10; ++a)
    {
        for (int b = -10; b < 10; ++b)
        {
            const auto choose_mat = random_double();
            vec3 center(a + 0.9 * random_double(), 0.2,
                        b + 0.9 * random_double());

            if ((center - vec3{4, .2, 0}).length() > 0.9)
            {
                if (choose_mat < 0.8)
                {
                    // diffuse
                    auto albedo = vec3::random() * vec3::random();
                    world.add(std::make_shared<moving_sphere>(
                        center, center + vec3{0, random_double(0, .5), 0}, 0.0,
                        1.0, 0.2,
                        std::make_shared<lambertian>(
                            std::make_shared<solid_color>(albedo))));
                }
                else if (choose_mat < 0.95)
                {
                    // metal
                    auto albedo = vec3::random(.5, 1);
                    auto fuzz = random_double(0, .5);
                    world.add(std::make_shared<sphere>(
                        center, 0.2, std::make_shared<metal>(albedo, fuzz)));
                }
                else
                {
                    // glass
                    world.add(std::make_shared<sphere>(
                        center, 0.2, std::make_shared<dielectric>(1.5)));
                }
            }
        }
    }

    world.add(std::make_shared<sphere>(vec3{0, 1, 0}, 1.0,
                                       std::make_shared<dielectric>(1.5)));
    world.add(std::make_shared<sphere>(
        vec3{-4, 1, 0}, 1.0,
        std::make_shared<lambertian>(
            std::make_shared<solid_color>(0.4, 0.2, 0.1))));
    world.add(std::make_shared<sphere>(
        vec3{4, 1, 0}, 1.0, std::make_shared<metal>(vec3{0.7, 0.6, 0.5}, 0.0)));

    return world;
}

hittable_list two_spheres()
{
    hittable_list objects;

    auto checker = std::make_shared<checker_texture>(
        std::make_shared<solid_color>(0.2, 0.3, 0.1),
        std::make_shared<solid_color>(0.9, 0.9, 0.9));

    objects.add(std::make_shared<sphere>(
        point3(0, -10, 0), 10, std::make_shared<lambertian>(checker)));
    objects.add(std::make_shared<sphere>(
        point3(0, 10, 0), 10, std::make_shared<lambertian>(checker)));

    return objects;
}

hittable_list two_perlin_spheres()
{
    hittable_list objects;

    const auto pertext = std::make_shared<noise_texture>(4.0);

    objects.add(std::make_shared<sphere>(
        point3(0, -1000, 0), 1000, std::make_shared<lambertian>(pertext)));
    objects.add(std::make_shared<sphere>(
        point3(0, 2, 0), 2, std::make_shared<lambertian>(pertext)));

    return objects;
}

int main()
{
    const int image_width = 800;
    const int image_height = 400;
    const int samples_per_pixel = 100;
    const int max_depth = 50;
    const auto aspect_ratio = static_cast<double>(image_width) / image_height;

    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    const auto world = two_perlin_spheres();

    const vec3 lookfrom{13, 2, 3};
    const vec3 lookat{0, 0, 0};
    const vec3 vup{0, 1, 0};
    const auto dist_to_focus = 10.0;
    const auto aperture = 0.0;

    const camera cam(lookfrom, lookat, vup, 20, aspect_ratio, aperture,
                     dist_to_focus, 0.0, 1.0);

    for (int j = image_height - 1; j >= 0; --j)
    {
        std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;

        for (int i = 0; i < image_width; ++i)
        {
            vec3 color{0, 0, 0};

            for (int s = 0; s < samples_per_pixel; ++s)
            {
                const auto u = (i + random_double()) / image_width;
                const auto v = (j + random_double()) / image_height;
                ray r = cam.get_ray(u, v);
                color += ray_color(r, world, max_depth);
            }

            color.write_color(std::cout, samples_per_pixel);
        }
    }

    std::cerr << "\nDone.\n";

    return 0;
}