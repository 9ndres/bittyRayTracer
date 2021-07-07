#include <limits>
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
// #include<numbers> std::numeric::pi

#include"geometry.h"

struct Light {
    Light(const geometry::Vec3f& p, const float i) :position{ p }, intensity{ i } { }
    geometry::Vec3f position;
    float intensity;
};

struct Material {
    Material(const float r, const geometry::Vec4f& a, const geometry::Vec3f& color, const float spec);
    Material() : refractive_index{ 1 }, albedo{ 1,0,0,0 }, diffuse_color(), specular_exponent() {}
    float refractive_index;
    geometry::Vec4f albedo; 
    geometry::Vec3f diffuse_color;
    float specular_exponent;
};

Material::Material(const float r, const geometry::Vec4f& a, const geometry::Vec3f& color, const float spec) 
            :refractive_index{ r }, albedo{ a }, diffuse_color { color }, specular_exponent{ spec }
{
}

struct Sphere {
    geometry::Vec3f center;
    float radius;
    Material material;

    Sphere(const geometry::Vec3f &c, const float r, const Material &m) : center(c), radius(r), material(m) {}

    bool ray_intersect(const geometry::Vec3f &orig, const geometry::Vec3f &dir, float &t0) const;
};

bool Sphere::ray_intersect(const geometry::Vec3f &orig, const geometry::Vec3f &dir, float &t0) const {
        geometry::Vec3f L = center - orig;
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

geometry::Vec3f reflect(const geometry::Vec3f& I, const geometry::Vec3f& N) {
    return I - N*2.f*(I*N);
}

geometry::Vec3f refract(const geometry::Vec3f& I, const geometry::Vec3f& N, 
            const float eta_t, const float eta_i= 1.f) {                
    float cosi = - std::max(-1.f, std::min(1.f, std::min(1.f, I*N)));
    if  (cosi<0)
        return refract(I, N, eta_t, eta_i);

    float  eta = eta_i / eta_t;
    float k = 1 - eta*eta*(1 - cosi*cosi);

    return k < 0 
    ? geometry::Vec3f(1,0,0) 
    : I * eta + N * (eta * cosi - sqrtf(k)); 
}              

bool scene_intersect(const geometry::Vec3f& orig, const geometry::Vec3f& dir, const std::vector<Sphere>& spheres,
                     geometry::Vec3f& hit, geometry::Vec3f& N, Material& material) {

    float sphere_dist = std::numeric_limits<float>::max();

    for (size_t i = 0; i < spheres.size(); i++) {
       float dist_i = 0; 
        if (spheres[i].ray_intersect(orig, dir, dist_i) && dist_i < sphere_dist) {
            sphere_dist = dist_i;
            hit = orig + dir * dist_i;
            N = (hit - spheres[i].center).normalize();
            material = spheres[i].material;
        }
    }

    float checkboard = std::numeric_limits<float>::max();

    if (abs(dir.y) > 1e-3) {
        float d = -(orig.y+4)/dir.y;
        geometry::Vec3f pt = orig + dir * dir;
        if (d > 0 && abs(pt.y) <10 && pt.z < -10 && pt.z > -30 && d < sphere_dist) {
            float checkboard_dist = d;
             hit = pt;
             N = geometry::Vec3f(0, 1, 0);
             material.diffuse_color = (int(.5*hit.x+1000) + int(.5*hit.z)) & 1 
             ? geometry::Vec3f(.3, .3, .3) 
             : geometry::Vec3f(.3, .2, .1); 
        }
    }        
    return std::min(sphere_dist, checkboard_dist) < 1000;
}
    
geometry::Vec3f cast_ray(const geometry::Vec3f& orig, const geometry::Vec3f& dir,
                         const std::vector<Sphere>& spheres) {
    geometry::Vec3f point, N;
    Material material;

    if(depth > 4 || !scene_intersect(orig, dir, spheres, point, N, material)){
          return Vec3f(0.2, 0.7, 0.8);
    
    }

    geometry::Vec3f reflect_dir = reflect(dir, N).normalize();
    geometry::Vec3f refract_dir = refract(dir, N, material.refractive_index).normalize();
    geometry::Vec3f reflect_orig = reflect_dir * N < 0 
    ? point - N*1e-3 
    : point + N*1e-3; // offset the original point to avoid occlusion by the object itself   
    geometry::Vec3f refract_orig = refract_dir * N < 0 
    ? point - N*1e-3 
    : point + N*1e-3;
    geometry::Vec3f reflect_color = cast_ray(reflect_orig, reflect_dir, spheres, lights, depth + 1);
    geometry::Vec3f refract_color = cast_ray(refract_orig, refract_dir, spheres, lights, depth + 1);

    float diffuse_light_intensity = 0, specular_light_intensity = 0;

    for (size_t i=0; i < lights.size(); i++) {
     geometry::Vec3f light_dir      = (lights[i].position - point).normalize();
     float light_distance = (lights[i].position - point).norm();
     shadow_orig = light_dir*N < 0 ? point - N*1e-3 : point + N*1e-3; // checking if the point lies in the shadow of the lights[i]
     geometry::Vec3f shadow_pt, shadow_N;
     Material tmpmaterial;

    if (scene_intersect(shadow_orig, light_dir, spheres, shadow_pt, shadow_N, tmpmaterial)
     && (shadow_pt-shadow_orig).norm() < light_distance) continue;       
          diffuse_light_intensity  += lights[i].intensity * std::max(0.f, light_dir*N);
          specular_light_intensity += powf(std::max(0.f, -reflect(-light_dir, N)*dir), material.specular_exponent)*lights[i].intensity;
          }

    return material.diffuse_color * diffuse_light_intensity * material.albedo[0] + 
    geometry::Vec3f(1., 1., 1.) * specular_light_intensity * material.albedo[1] 
    + reflect_color*material.albedo[2] + refract_color*material.albedo[3];
}

void render(const std::vector<Sphere>&spheres ) {
    constexpr int  width = 1024, height = 768;
    constexpr float fov = 3.1415926535 / 3.;

    std::vector<geometry::Vec3f>framebuffer(width * height);
    // https://support.touchgfx.com/docs/basic-concepts/framebuffer

    for (size_t j = 0; j < height; j++){
        for (size_t i = 0; i < width; i++){
            float x =  (2*(i + 0.5)/(float)width  - 1)*tan(fov/2.)*width/(float)height;
            float y = -(2*(j + 0.5)/(float)height - 1)*tan(fov/2.);
            geometry::Vec3f dir = geometry::Vec3f(x, y, -1).normalize();
            framebuffer[i+j * width] = cast_ray(geometry::Vec3f(0,0,0), dir, spheres);
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
}

int main() {
    Material ivory(Vec3f(0.4, 0.4, 0.3));
    Material red_rubber(Vec3f(0.3, 0.1, 0.1));

    std::vector<Sphere> spheres; // stores the instances of the class
    spheres.push_back(Sphere(Vec3f(-3,    3,   -16), 2, ivory));
    spheres.push_back(Sphere(Vec3f(-1.0, -1.5, -12), 2, red_rubber));
    spheres.push_back(Sphere(Vec3f( 1.5, -0.5, -18), 3, red_rubber));
    spheres.push_back(Sphere(Vec3f( 7,    5,   -18), 4, ivory));

    render(spheres);
    return 0;
}
