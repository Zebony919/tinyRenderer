#include "tgaimage.h"
#include "geometry.h"

void lookat(const vec3 eye, const vec3 center, const vec3 up);
void init_perspective(const double f);
void init_viewport(const int x, const int y, const int w, const int h);
void init_zbuffer(const int width, const int height);

struct IShader {
    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const = 0;
};

typedef vec4 Triangle[3]; // A triangle primitive being made of three points/verices
void rasterize(const vec4 clip[3], const IShader &shader, std::vector<double> &zbuffer, TGAImage &framebuffer, TGAImage &actualZbuffer);