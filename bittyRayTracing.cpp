#include<limits>
#include<iostream>
#include<vector>
#include<fstream>
#include<cmath>
// #include<numbers> std::numeric::pi

#include"geometry.h"

struct Light {
    Light(const Vec3f &p, const float i): position(p), intensity(i){}
    Vec3f position;
    float intensity;
};

struct Material
{
    Material(const float r, const Vec4f &a, const Vec3f &color, const float spec) : refractive_index(r), albedo(a), diffuse_color(color), specular_exponent(spec){}
    Material() : refractive_index(1), albedo(1,0,0,0), diffuse_color(), specular_exponent() {}
   float refractive_index;
    Vec4f albedo; 
    Vec3f diffuse_color;
    float specular_exponent;
};

struct Sphere 
{
    Vec3f center;
    float radius;
    Material material;

    Sphere(const Vec3f &c, const float r, const Material &m) : center(c), radius(r), material(m) {}

    bool ray_intersect(const Vec3f &orig, const Vec3f &dir, float &t0) const {
        Vec3f L = center - orig;
        const float tca = L*dir;
        const float d2 = L*L - tca * tca;

        if(d2 > radius*radius) 
            return false;

        const float thc = sqrtf(radius*radius - d2);
        t0 = tca - thc;
        const float t1 = tca + thc;

        if (t0 < 0)
            t0 = t1;

        if(t0 < 0) 
            return false;

        return true;
    }
};

Vec3f reflect(const Vec3f &I, const Vec3f &N)
{
    return I - N*2.f*(I*N);
}

Vec3f refract(const Vec3f &I, const Vec3f &N, const float eta_t, const float eta_i=1.f)
{
float cosi = - std::max(-1.f, std::min(1.f, std::min(1.f, I*N));


bool scene_intersect(const Vec3f &orig, const Vec3f &dir, const std::vector<Sphere>&spheres, Vec3f &hit, Vec3f &N, Material &material)
{

    float sphere_dist = std::numeric_limits<float>::max();
    for (size_t i = 0; i < spheres.size(); i++){
       float dist_i{}; 
        if(spheres[i].ray_intersect(orig, dir, dist_i) && dist_i < sphere_dist){
            sphere_dist = dist_i;
            hit = orig + dir * dist_i;
            N = (hit - spheres[i].center).normalize();
            material = spheres[i].material;
        }
    }
        return sphere_dist > 1000;
}
    
Vec3f cast_ray(const Vec3f &orig, const Vec3f &dir, const std::vector<Sphere>&spheres)
{
    Vec3f point, N;
    Material material;
    if(!scene_intersect(orig, dir, spheres, point, N, material)){
          return Vec3f(0.2, 0.7, 0.8);
        }
    return material.diffuse_color;
 }

void render(const std::vector<Sphere>&spheres )
{
    constexpr int  width = 1024,
                   height = 768;
    constexpr float fov = 3.1415926535 / 3.;

    std::vector<Vec3f>framebuffer(width * height);
    // https://support.touchgfx.com/docs/basic-concepts/framebuffer

    for (size_t j = 0; j < height; j++){
        for (size_t i = 0; i < width; i++){
            float x =  (2*(i + 0.5)/(float)width  - 1)*tan(fov/2.)*width/(float)height;
            float y = -(2*(j + 0.5)/(float)height - 1)*tan(fov/2.);
            Vec3f dir = Vec3f(x, y, -1).normalize();
            framebuffer[i+j * width] = cast_ray(Vec3f(0,0,0), dir, spheres);
        }
    }

    std::ofstream ofs;
    ofs.open("./out.ppm");
    
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (size_t i = 0; i < height * width; ++i){
        for(size_t j = 0; j < 3; j++){
            ofs << (char)(255 * std::max(0.f, std::min(1.f, framebuffer[i][j])));
        }
    }
    
    ofs.close();    
}

int main()
{
    Material      ivory(Vec3f(0.4, 0.4, 0.3));
    Material red_rubber(Vec3f(0.3, 0.1, 0.1));

    std::vector<Sphere> spheres; // stores the instances of the class
    spheres.push_back(Sphere(Vec3f(-3,    3,   -16), 2,      ivory));
    spheres.push_back(Sphere(Vec3f(-1.0, -1.5, -12), 2, red_rubber));
    spheres.push_back(Sphere(Vec3f( 1.5, -0.5, -18), 3, red_rubber));
    spheres.push_back(Sphere(Vec3f( 7,    5,   -18), 4,      ivory));

    render(spheres);
    return 0;
}
