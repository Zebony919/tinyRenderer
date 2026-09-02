// Fucked up new math version, who knows whats going on

#define _USE_MATH_DEFINES
#include <cmath>
#include <tuple>
#include <algorithm>
#include <limits>
#include "geometry.h"
#include "model.h"
#include "tgaimage.h"

constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};

mat<4, 4> ModelView, Viewport, Perspective;

void lookat(const vec3 eye, const vec3 center, const vec3 up) {
    vec3 n = normalized(eye-center);
    vec3 l = normalized(cross(up, n));
    vec3 m = normalized(cross(n, l));

    ModelView = mat<4, 4>{{{l.x, l.y, l.z, 0}, {m.x, m.y, m.z, 0}, {n.x, n.y, n.z, 0}, {0, 0, 0, 1}}} *
                mat<4, 4>{{{1, 0, 0, -center.x}, {0, 1, 0, -center.y}, {0, 0, 1, -center.z}, {0, 0, 0, 1}}};
}

void perspective(const double f) {
    Perspective = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, -1/f, 1}}};
}

void viewport(const int x, const int y, const int w, const int h) {
    Viewport = {{{w/2.0, 0, 0, x + w/2.0}, {0, h/2.0, 0, y + h/2.0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};
}

void rasterize(const vec4 clip[3], std::vector<double> &zbuffer, TGAImage &framebuffer, const TGAColor color) {
    vec4 ndc[3] = { clip[0]/clip[0].w, clip[1]/clip[1].w, clip[2]/clip[2].w };
    vec2 screen[3] = { (Viewport * ndc[0]).xy(), (Viewport * ndc[1]).xy(), (Viewport * ndc[2]).xy() };

    mat<3, 3> ABC = {{ {screen[0].x, screen[0].y, 1.0}, {screen[1].x, screen[1].y, 1.0}, {screen[2].x, screen[2].y, 1.0}}};
    if (std::abs(ABC.det()) < 1e-9) return;

    auto [bbminx, bbmaxx] = std::minmax({screen[0].x, screen[1].x, screen[2].x});
    auto [bbminy, bbmaxy] = std::minmax({screen[0].y, screen[1].y, screen[2].y});

    #pragma omp parallel for
    for (int x = std::max<int>(bbminx, 0); x <= std::min<int>(bbmaxx, framebuffer.width() - 1); x++) {
        for (int y = std::max<int>(bbminy, 0); y <= std::min<int>(bbmaxy, framebuffer.height() - 1); y++) {
            vec3 bc = ABC.invert_transpose() * vec3{static_cast<double>(x), static_cast<double>(y), 1.0};
            double z = bc * vec3{ ndc[0].z, ndc[1].z, ndc[2].z };

            if (z <= zbuffer[x + y * framebuffer.width()]) continue;
            zbuffer[x + y * framebuffer.width()] = z;
            framebuffer.set(x, y, color);
        }
    }
}

double signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy) {
    return 0.5 * ((by - ay)*(bx + ax) + (cy - by)*(cx + bx) + (ay - cy)*(ax + cx));
    // Takes the sum of three trapezoids defined an edge, the x-axis, and two parallel lines going down
    // from each end of the edge to the x-axis. 
}

void drawTriangle(int ax, int ay, int az, int bx, int by, int bz, int cx, int cy, int cz, TGAImage &framebuffer, TGAImage &zbuffer, TGAColor color) {
    int bbminx = std::min(std::min(ax, bx), cx); // Lowest vertice
    int bbminy = std::min(std::min(ay, by), cy);
    int bbmaxx = std::max(std::max(ax, bx), cx); // Highest vertice
    int bbmaxy = std::max(std::max(ay, by), cy);
    double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);

    if (total_area < 1) return;

    #pragma omp parallel for // Allows for parallel iteration where the compiler can split up the for loop amongst multiple cores
    for (int x = bbminx; x <= bbmaxx; x++) {
        for (int y = bbminy; y <= bbmaxy; y++) {
            double alpha = signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            double beta = signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            double gamma = signed_triangle_area(x, y, ax, ay, bx, by) / total_area;

            if (alpha < 0 || beta < 0 || gamma < 0) continue; // Negative barycentric coordinate
            // If its negative then the pixel is outside the triangle bound

            unsigned char z = static_cast<unsigned char>(alpha * az + beta * bz + gamma * cz);
            if (z <= zbuffer.get(x, y)[0]) continue;

            zbuffer.set(x, y, {z});
            framebuffer.set(x, y, color);
        }
    }
}

// Better version since you call it once before the loop in main
// These are actually the three rotation matrices which you can multiply to alter more than one axis
// or just use one to rotate around a fixed axis
mat<3, 3> rotateY(double radian) {
    return {{{std::cos(radian), 0, std::sin(radian)}, {0,1,0}, {-std::sin(radian), 0, std::cos(radian)}}};
}

mat<3, 3> rotateX(double radian) {
    return {{{1, 0, 0}, {0, std::cos(radian), -std::sin(radian)}, {0, std::sin(radian), std::cos(radian)}}};
}

mat<3, 3> rotateZ(double radian) {
    return {{{std::cos(radian), -std::sin(radian), 0}, {std::sin(radian), std::cos(radian), 0}, {0, 0, 1}}};
}

vec3 perspective(vec3 v) {
    constexpr double camera = 3.0;
    return v / (1 - v.z / camera);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1; 
    }

    constexpr int width  = 800;
    constexpr int height = 800;
    constexpr vec3 eye{-1, 0, 2};
    constexpr vec3 center{0, 0, 0};
    constexpr vec3 up{0, 1, 0};

    lookat(eye, center, up);
    perspective(norm(eye-center));
    viewport(width/16, height/16, width * 7/8, height * 7/8);
    
    TGAImage framebuffer(width, height, TGAImage::RGB);
    std::vector<double> zbuffer(width * height, -std::numeric_limits<double>::max());

    for (int m = 1; m < argc; m++) {
        Model model(argv[m]);

        for (int i = 0; i < model.nfaces(); i++) {
            vec4 clip[3];

            for (int d : {0, 1, 2}) {
                vec4 v = model.vert(i, d);
                clip[d] = Perspective * ModelView * vec4{v.x, v.y, v.z, 1.0};
            }

            TGAColor rnd;
            for (int c = 0; c < 3; c++) rnd[c] = std::rand() % 255;
            rasterize(clip, zbuffer, framebuffer, rnd);
        }
    }
    
    framebuffer.write_tga_file("framebuffer.tga");
    
    std::cout << "Rendered successfully, wrote framebuffer.tga\n";
    return 0;
}