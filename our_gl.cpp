#include <algorithm>
#include "our_gl.h"

mat<4, 4> ModelView, Viewport, Perspective;
std::vector<double> zbuffer;

void lookat(const vec3 eye, const vec3 center, const vec3 up) {
    vec3 n = normalized(eye - center);
    vec3 l = normalized((cross(up, n)));
    vec3 m = normalized((cross(n, l)));

    ModelView = mat<4, 4>{{{l.x, l.y, l.z, 0}, {m.x, m.y, m.z, 0}, {n.x, n.y, n.z, 0}, {0, 0, 0, 1}}} * // Camera Matrix
                mat<4, 4>{{{1, 0, 0, -center.x}, {0, 1, 0, -center.y}, {0, 0, 1, -center.z}, {0, 0, 0, 1}}}; // Translation Matrix
}

void init_viewport(const int x, const int y, const int w, const int h) {
    Viewport = {{{w/2.0, 0, 0, x + w/2.0}, {0, h/2.0, 0, y + h/2.0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};
}

void init_perspective(const double f) {
    Perspective = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, -1/f, 1}}};
}

void init_zbuffer(const int width, const int height) {
    zbuffer = std::vector(width * height, -1000.0);
}

void rasterize(const vec4 clip[3], const IShader &shader, std::vector<double> &zbuffer, TGAImage &framebuffer, TGAImage &actualZbuffer3) {
    vec4 ndc[3] = { clip[0] / clip[0].w, clip[1] / clip[1].w, clip[2] / clip[2].w }; // Normalized Device Coordinates
    vec2 screen[3] = { (Viewport * ndc[0]).xy(), (Viewport * ndc[1]).xy(), (Viewport * ndc[2]).xy() }; // Screen Coordinates

    mat<3, 3> ABC = {{ {screen[0].x, screen[0].y, 1.0}, {screen[1].x, screen[1].y, 1.0}, {screen[2].x, screen[2].y, 1.0} }}; // Three vertices
    if (ABC.det() < 1) return;

    auto [bbminx, bbmaxx] = std::minmax({screen[0].x, screen[1].x, screen[2].x}); // Bounding box for triangle
    auto [bbminy, bbmaxy] = std::minmax({screen[0].y, screen[1].y, screen[2].y}); // defined by the bottom left and top right points

    #pragma omp parallel for
    for (int x = std::max<int>(bbminx, 0); x <= std::min<int>(bbmaxx, framebuffer.width() - 1); x++) {
        for (int y = std::max<int>(bbminy, 0); y <= std::min<int>(bbmaxy, framebuffer.height() - 1); y++) {
            vec3 bc = ABC.invert_transpose() * vec3{static_cast<double>(x), static_cast<double>(y), 1.0};

            if (bc.x < 0 || bc.y < 0 || bc.z < 0) continue; // Negative barycentric coordinate means vertex outside triangle

            double z = bc * vec3{ ndc[0].z, ndc[1].z, ndc[2].z };
            if (z <= zbuffer[x + y * framebuffer.width()]) continue; // Depth check on whether current pixel is infront of already drawn pixel

            unsigned char z2 = static_cast<unsigned char>((z * 0.5 + 0.5) * 255);
            
            auto [discard, color] = shader.fragment(bc);
            if (discard) continue;

            zbuffer[x + y * framebuffer.width()] = z;

            /*
            // Red dot in the middle of every visible triangle. Cool way to see the model is still built from triangle.
            // Also you should never do direct comparisons with floats since they are not usually exactly equal when logically 
            // they should be. A fix is to have an epsilon value and compare it to their differences.
            double epsilon = 0.02;
            if (std::abs(bc.x - bc.y) < epsilon && std::abs(bc.x - bc.z) < epsilon) {
                framebuffer.set(x, y, {0, 0, 255, 255});
            } else {
                framebuffer.set(x, y, color);
            }
            */

            framebuffer.set(x, y, color);
            actualZbuffer3.set(x, y, {z2});
        }
    }
}